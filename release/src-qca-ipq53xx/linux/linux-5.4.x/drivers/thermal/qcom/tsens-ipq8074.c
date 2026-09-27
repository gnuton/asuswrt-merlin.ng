/*
 * Copyright (c) 2015, 2017, 2020 The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/bitops.h>
#include <linux/regmap.h>
#include <linux/thermal.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include "tsens.h"

/* TSENS register data */
#define TSENS_CNTL_ADDR			0x4
#define TSENS_TRDY_TIMEOUT_US		20000
#define TSENS_THRESHOLD_MAX_CODE	0x3ff
#define TSENS_THRESHOLD_MIN_CODE	0x0
#define TSENS_SN_CTRL_EN		BIT(0)
#define TSENS_SN_SW_RST			BIT(1)
#define TSENS_SN_CODE_OR_TEMP		BIT(21)

#define TSENS_MEASURE_PERIOD		0x8

#define TSENS_TM_TRDY			0x00e4
#define TSENS_TRDY_MASK			BIT(0)
#define TSENS_TM_CODE_BIT_MASK		0xfff
#define TSENS_TM_CODE_SIGN_BIT		0x800

#define TSENS_TM_INT_EN			0x0004
#define TSENS_TM_CRITICAL_INT_EN	BIT(2)
#define TSENS_TM_UPPER_INT_EN		BIT(1)
#define TSENS_TM_LOWER_INT_EN		BIT(0)

#define TSENS_TM_UPPER_INT_MASK(n)	(((n) & 0xffff0000) >> 16)
#define TSENS_TM_LOWER_INT_MASK(n)	((n) & 0xffff)
#define TSENS_TM_UPPER_LOWER_INT_STATUS		0x0008
#define TSENS_TM_UPPER_LOWER_INT_CLEAR		0x000c
#define TSENS_TM_UPPER_INT_CLEAR_SET(n)		(1 << (n + 16))
#define TSENS_TM_LOWER_INT_CLEAR_SET(n)		(1 << n)
#define TSENS_TM_UPPER_LOWER_INT_MASK		0x0010
#define TSENS_TM_UPPER_INT_SET(n)		(1 << (n + 16))
#define TSENS_TM_LOWER_INT_SET(n)               (1 << n)

#define TSENS_TM_CRITICAL_INT_STATUS		0x0014
#define TSENS_TM_CRITICAL_INT_CLEAR		0x0018
#define TSENS_TM_CRITICAL_INT_CLEAR_SET(n)	(1 << n)
#define TSENS_TM_CRITICAL_INT_MASK		0x001c

#define TSENS_TM_UPPER_LOWER_THRESHOLD(n)	((n * 4) + 0x0020)
#define TSENS_TM_UPPER_THRESHOLD_SET(n)		((n) << 12)
#define TSENS_TM_UPPER_THRESHOLD_VALUE_SHIFT(n)	((n) >> 12)
#define TSENS_TM_LOWER_THRESHOLD_VALUE(n)	((n) & 0xfff)
#define TSENS_TM_UPPER_THRESHOLD_VALUE(n)	(((n) & 0xfff000) >> 12)
#define TSENS_TM_UPPER_THRESHOLD_MASK		0xfff000
#define TSENS_TM_LOWER_THRESHOLD_MASK		0xfff
#define TSENS_TM_LOWER_THRESHOLD_CLEAR		0xfff000
#define TSENS_TM_UPPER_THRESHOLD_CLEAR		0xfff
#define TSENS_TM_UPPER_THRESHOLD_SHIFT		12

#define TSENS_TM_SN_CRITICAL_THRESHOLD_MASK	0xfff
#define TSENS_TM_SN_CRITICAL_THRESHOLD(n)	((n * 4) + 0x0060)
#define TSENS_TM_SN_CRITICAL_THRESHOLD_VALUE(n)	((n) & 0xfff)
#define TSENS_TM_SN_STATUS			0x00a0
#define TSENS_TM_SN_STATUS_VALID_BIT		BIT(21)
#define TSENS_TM_SN_STATUS_CRITICAL_STATUS	BIT(19)
#define TSENS_TM_SN_STATUS_UPPER_STATUS		BIT(18)
#define TSENS_TM_SN_STATUS_LOWER_STATUS		BIT(17)
#define TSENS_TM_SN_LAST_TEMP_MASK		0xfff

#define TSENS_SN_CONVERSION(n)			((n * 4) + 0x0060)

#define MAX_SENSOR				16
#define VALID_SENSOR_START_IDX			4
#define MAX_TEMP				204000 /* milliCelcius */
#define MIN_TEMP				0   /* milliCelcius */

#define BASE0_MASK				0x001FF800
#define BASE0_SHIFT				11
#define BASE1_MASK				0x7FE00000
#define BASE1_SHIFT				21
#define TSENS11_OFFSET				0x00F000
#define TSENS11_SHIFT				12
#define TSENS12_OFFSET				0x0F0000
#define TSENS12_SHIFT				16
#define TSENS13_OFFSET				0xF00000
#define TSENS13_SHIFT				20
#define TSENS14_OFFSET				0x078000
#define TSENS14_SHIFT				15
#define TSENS15_OFFSET				0x780000
#define TSENS15_SHIFT				19
#define TSENS_CAL_SHIFT				3
#define TSENS_CAL_SHIFT_SHIFT			23
#define TSENS_ONE_POINT_SLOPE			0xCD0
#define TSENS_ONE_PT_CZERO_CONST		94
#define TSENS_TWO_POINT_SLOPE			921600
#define TSENS_SLOPE_SHIFT			10
#define TSENS_CONVERSION_DEFAULT		0x1b3416a
#define TSENS_SENSOR_MASK			0x7C000

#define CAL_SEL_MASK 0x700
#define CAL_SEL_SHIFT 8

struct low_temp_notification {
	int temp;
	low_temp_notif_fn cb;
};

static struct low_temp_notification low_temp_notif[MAX_SENSOR];
static int up_thres_backup[MAX_SENSOR];
static bool int_clr_deassert_quirk;

/* Trips: from very hot to very cold */
enum tsens_trip_type {
	TSENS_TRIP_STAGE3 = 0, /* Critical high */
	TSENS_TRIP_STAGE2,     /* Configurable high */
	TSENS_TRIP_STAGE1,     /* Configurable low */
	TSENS_TRIP_STAGE0,     /* Critical low */
	TSENS_TRIP_NUM,
};

static int init_tsens_ctrl(struct tsens_priv *tmdev);
static int get_temp_ipq807x(struct tsens_priv *tmdev, int id, int *temp);
static int set_trip_temp(struct tsens_priv *tmdev, int sensor,
					enum tsens_trip_type trip, int temp);
static int set_trip_mode(struct tsens_priv *tmdev, int sensor, int trip,
					enum thermal_trip_activation_mode mode);
static struct tsens_priv *g_tmdev;

int register_low_temp_notif(int sensor, int cold_temp, low_temp_notif_fn fn)
{
	int rc;

	if (sensor < 0 || sensor >= MAX_SENSOR)
		return -EINVAL;

	low_temp_notif[sensor].temp = cold_temp;
	low_temp_notif[sensor].cb = fn;

	if (!g_tmdev) {
		pr_info("g_tmdevn not initialzied.\n");
		return -EINVAL;
	}

	rc = set_trip_temp(g_tmdev, sensor, TSENS_TRIP_STAGE1, cold_temp);
	if (rc) {
		pr_info("Failed to set cold trip temp.\n");
		return rc;
	}

	rc = set_trip_mode(g_tmdev, sensor, TSENS_TRIP_STAGE1,
					THERMAL_TRIP_ACTIVATION_ENABLED);
	if (rc) {
		pr_info("Failed to enable cold trip point.\n");
		return rc;
	}

	return 0;
}

bool is_sensor_used_internally(int sensor)
{
	int i;

	if (sensor < 0 || sensor >= MAX_SENSOR)
		return false;

	for (i = 0; i < MAX_SENSOR; i++) {
		if (low_temp_notif[sensor].cb)
			return true;
	}

	return false;
}

/*
 * Returns Trip temp in millidegree Celcius
 * Note: IPQ807x does not support -ve trip temperatures
 */
static int get_trip_temp(struct tsens_priv *tmdev, int sensor,
						enum tsens_trip_type trip)
{
	unsigned int reg_th;

	if (!tmdev)
		return -EINVAL;

	if ((sensor < 0) || (sensor > (MAX_SENSOR - 1)))
		return -EINVAL;

	switch (trip) {
	case TSENS_TRIP_STAGE3:
		regmap_read(tmdev->tm_map,
			TSENS_TM_SN_CRITICAL_THRESHOLD(sensor), &reg_th);
		return (TSENS_TM_SN_CRITICAL_THRESHOLD_VALUE(reg_th))*100;
	case TSENS_TRIP_STAGE2:
		regmap_read(tmdev->tm_map,
			TSENS_TM_UPPER_LOWER_THRESHOLD(sensor), &reg_th);
		return (TSENS_TM_UPPER_THRESHOLD_VALUE(reg_th))*100;
	case TSENS_TRIP_STAGE1:
		regmap_read(tmdev->tm_map,
			TSENS_TM_UPPER_LOWER_THRESHOLD(sensor), &reg_th);
		return (TSENS_TM_LOWER_THRESHOLD_VALUE(reg_th))*100;
	default:
		return -EINVAL;
	}

	return -EINVAL;
}

/*
 * Set Trip temp in millidegree Celcius
 * Note: IPQ807x does not support -ve trip temperatures
 */
static int set_trip_temp(struct tsens_priv *tmdev, int sensor,
					enum tsens_trip_type trip, int temp)
{
	u32 reg_th, th_cri, th_hi, th_lo, reg_th_offset, reg_cri_th_offset;

	if (!tmdev)
		return -EINVAL;

	if ((sensor < 0) || (sensor > (MAX_SENSOR - 1)))
		return -EINVAL;

	if ((temp < MIN_TEMP) && (temp > MAX_TEMP))
		return -EINVAL;

	/* convert temp from millicelsius to decicelsius */
	temp = temp/100;

	reg_th_offset = TSENS_TM_UPPER_LOWER_THRESHOLD(sensor);
	reg_cri_th_offset = TSENS_TM_SN_CRITICAL_THRESHOLD(sensor);

	regmap_read(tmdev->tm_map,
		TSENS_TM_SN_CRITICAL_THRESHOLD(sensor), &th_cri);
	regmap_read(tmdev->tm_map,
		TSENS_TM_UPPER_LOWER_THRESHOLD(sensor), &reg_th);

	th_hi = TSENS_TM_UPPER_THRESHOLD_VALUE(reg_th);
	th_lo = TSENS_TM_LOWER_THRESHOLD_VALUE(reg_th);

	switch (trip) {
	case TSENS_TRIP_STAGE3:
		if (temp < th_hi)
			return -EINVAL;

		reg_th_offset = reg_cri_th_offset;
		reg_th = temp;
		break;
	case TSENS_TRIP_STAGE2:
		if ((temp <= th_lo) || (temp >= th_cri))
			return -EINVAL;

		temp = TSENS_TM_UPPER_THRESHOLD_SET(temp);
		reg_th &= TSENS_TM_UPPER_THRESHOLD_CLEAR;
		reg_th |= temp;
		break;
	case TSENS_TRIP_STAGE1:
		if (temp >= th_hi)
			return -EINVAL;

		reg_th &= TSENS_TM_LOWER_THRESHOLD_CLEAR;
		reg_th |= temp;
		break;
	default:
		return -EINVAL;
	}

	regmap_write(tmdev->tm_map, reg_th_offset, reg_th);

	/* Sync registers */
	mb();

	return 0;
}

static int set_trip_mode(struct tsens_priv *tmdev, int sensor, int trip,
					enum thermal_trip_activation_mode mode)
{
	unsigned int reg_val, reg_offset;

	if (!tmdev)
		return -EINVAL;

	if ((sensor < 0) || (sensor > (MAX_SENSOR - 1)))
		return -EINVAL;

	switch (trip) {
	case TSENS_TRIP_STAGE3:
		reg_offset = TSENS_TM_CRITICAL_INT_MASK;
		regmap_read(tmdev->tm_map, reg_offset, &reg_val);
		if (mode == THERMAL_TRIP_ACTIVATION_DISABLED)
			reg_val = reg_val | (1 << sensor);
		else
			reg_val = reg_val & ~(1 << sensor);
		break;
	case TSENS_TRIP_STAGE2:
		reg_offset = TSENS_TM_UPPER_LOWER_INT_MASK;
		regmap_read(tmdev->tm_map, reg_offset, &reg_val);
		if (mode == THERMAL_TRIP_ACTIVATION_DISABLED)
			reg_val = reg_val | (TSENS_TM_UPPER_INT_SET(sensor));
		else
			reg_val = reg_val & ~(TSENS_TM_UPPER_INT_SET(sensor));
		break;
	case TSENS_TRIP_STAGE1:
		reg_offset = TSENS_TM_UPPER_LOWER_INT_MASK;
		regmap_read(tmdev->tm_map, reg_offset, &reg_val);
		if (mode == THERMAL_TRIP_ACTIVATION_DISABLED)
			reg_val = reg_val | (1 << sensor);
		else
			reg_val = reg_val & ~(1 << sensor);
		break;
	default:
		return -EINVAL;
	}

	regmap_write(tmdev->tm_map, reg_offset, reg_val);

	/* Sync registers */
	mb();
	return 0;
}

static int __maybe_unused set_temp_trips(struct tsens_priv *tmdev, int sensor, int low_th, int high_th)
{
	u32 reg_th, reg_th_offset;
	int cur_temp;

	if (!tmdev)
		return -EINVAL;

	if ((sensor < 0) || (sensor > (MAX_SENSOR - 1)))
		return -EINVAL;

	get_temp_ipq807x(tmdev, sensor, &cur_temp);
	/* convert temp to decicelsius, HW unit */
	cur_temp = cur_temp / 100;

	reg_th_offset = TSENS_TM_UPPER_LOWER_THRESHOLD(sensor);

	regmap_read(tmdev->tm_map,
			TSENS_TM_UPPER_LOWER_THRESHOLD(sensor), &reg_th);

	/*
	 * Disable low-temp if "low" is too small. As per thermal framework
	 * API, we use -INT_MAX rather than INT_MIN.
	 */
	if (low_th <= -INT_MAX) {
		set_trip_mode(tmdev, sensor, TSENS_TRIP_STAGE1, THERMAL_TRIP_ACTIVATION_DISABLED);

		/* clear lower threshold values */
		reg_th &= TSENS_TM_LOWER_THRESHOLD_CLEAR;
		regmap_write(tmdev->tm_map, reg_th_offset, reg_th);

	} else {
		low_th = low_th / 100;
		reg_th &= TSENS_TM_LOWER_THRESHOLD_CLEAR;
		reg_th |= low_th;

		regmap_write(tmdev->tm_map, reg_th_offset, reg_th);

		if (low_th > cur_temp)
			/* Disable low temp interrupt when cur_temp is below lower threshold */
			set_trip_mode(tmdev, sensor, TSENS_TRIP_STAGE1, THERMAL_TRIP_ACTIVATION_DISABLED);
		else
			/* enable low temp interrupt when cur_temp is greater than lower threshold */
			set_trip_mode(tmdev, sensor, TSENS_TRIP_STAGE1, THERMAL_TRIP_ACTIVATION_ENABLED);
	}

	/* Disable high-temp if "high" is too big. */
	if (high_th == INT_MAX) {
		set_trip_mode(tmdev, sensor, TSENS_TRIP_STAGE2, THERMAL_TRIP_ACTIVATION_DISABLED);
	} else {
		high_th = high_th / 100;
		high_th = TSENS_TM_UPPER_THRESHOLD_SET(high_th);
		reg_th &= TSENS_TM_UPPER_THRESHOLD_CLEAR;
		reg_th |= high_th;

		regmap_write(tmdev->tm_map, reg_th_offset, reg_th);
	}

	/* Sync registers */
	mb();

	return 0;
}

static void notify_uspace_tsens_fn(struct work_struct *work)
{
	struct tsens_sensor *s = container_of(work, struct tsens_sensor,
								notify_work);

	if (!s || !s->tzd)
		/* Do nothing. TSENS driver has not been registered yet */
		return;

	sysfs_notify(&s->tzd->device.kobj, NULL, "type");
}

/*
 * This function handles cold condition.
 * More specifically when the temperature drops below 0c and
 * when the temperature is raising from 0c to +ve temperature.
 */
static void handle_cold_condition(struct tsens_priv *tmdev,
						int sensor, int dropping)
{
	int temp, i;
	int cold_cond_enter_threshold, cold_cond_exit_threshold;

	if (sensor < 0 || sensor >= MAX_SENSOR)
		return;

	if (!is_sensor_used_internally(sensor))
		return;

	cold_cond_enter_threshold = low_temp_notif[sensor].temp;
	cold_cond_exit_threshold = cold_cond_enter_threshold + 5;

	if (get_temp_ipq807x(tmdev, sensor, &temp))
		return;

	if (dropping && (temp <= cold_cond_enter_threshold)) {
		/* Disable lower threshold interrupt
		 * to avoid interrupt flooding
		*/
		set_trip_mode(tmdev, sensor, TSENS_TRIP_STAGE1,
					THERMAL_TRIP_ACTIVATION_DISABLED);

		/* Backup current upper threshold temperature */
		up_thres_backup[sensor] = get_trip_temp(tmdev, sensor,
					TSENS_TRIP_STAGE2);

		/* Set cold condition exit temperature as upper threshold */
		set_trip_temp(tmdev, sensor, TSENS_TRIP_STAGE2,
					cold_cond_exit_threshold);

	} else if (!dropping && (temp >= cold_cond_exit_threshold)) {
		/* Activate lower threshold interrupt */
		set_trip_mode(tmdev, sensor, TSENS_TRIP_STAGE1,
					THERMAL_TRIP_ACTIVATION_ENABLED);

		/* Restore previous trip temp */
		set_trip_temp(tmdev, sensor, TSENS_TRIP_STAGE2,
					up_thres_backup[sensor]);
	} else /* Do nothing */
		return;

	/* Notify to registered functions */
	for (i = 0; i < MAX_SENSOR; i++) {
		if (low_temp_notif[i].cb)
			low_temp_notif[i].cb(sensor, temp, dropping);
        }
}

static void tsens_scheduler_fn(struct work_struct *work)
{
	struct tsens_priv *tmdev = container_of(work, struct tsens_priv,
					tsens_work);
	int i, reg_thr, temp, th_upper = 0, th_lower = 0;
	u32 reg_val, reg_addr;

	/*Check whether TSENS is enabled */
	regmap_read(tmdev->tm_map, TSENS_CNTL_ADDR, &reg_val);
	if (!(reg_val & TSENS_SN_CTRL_EN))
		return;

	/* Iterate through all sensors */
	for (i = 0; i < tmdev->num_sensors; i++) {

		/* Reset for each iteration */
		reg_thr = th_upper = th_lower = 0;

		regmap_read(tmdev->tm_map, tmdev->sensor[i].status, &reg_val);

		/* Check whether the temp is valid */
		if (!(reg_val & TSENS_TM_SN_STATUS_VALID_BIT))
			continue;

		/* Check whether upper threshold is hit */
		if (reg_val & TSENS_TM_SN_STATUS_UPPER_STATUS) {
			reg_thr |= TSENS_TM_UPPER_INT_CLEAR_SET(i);
			reg_addr = TSENS_TM_UPPER_LOWER_INT_CLEAR;
			th_upper = 1;
		}
		/* Check whether lower threshold is hit */
		if (reg_val & TSENS_TM_SN_STATUS_LOWER_STATUS) {
			reg_thr |= TSENS_TM_LOWER_INT_CLEAR_SET(i);
			reg_addr = TSENS_TM_UPPER_LOWER_INT_CLEAR;
			th_lower = 1;
		}

		if (th_upper || th_lower) {
			handle_cold_condition( tmdev, i, th_lower);
			regmap_write(tmdev->tm_map, reg_addr, reg_thr);
#ifdef CONFIG_CPU_THERMAL
			/* If CPUFreq cooling is enabled, then notify Thermal framework */
			thermal_zone_device_update(tmdev->sensor[i].tzd, THERMAL_EVENT_UNSPECIFIED);
#else
			/* Notify user space */
			schedule_work(&tmdev->sensor[i].notify_work);
#endif

			if (int_clr_deassert_quirk)
				regmap_write(tmdev->tm_map, reg_addr, 0);

			if (!get_temp_ipq807x(tmdev, i, &temp))
				pr_debug("Trigger (%d millidegrees) for sensor %d\n",
					temp, i);
		}
	}

	/* Sync registers */
	mb();
}

static irqreturn_t tsens_isr(int irq, void *data)
{
	struct tsens_priv *tmdev = data;


	schedule_work(&tmdev->tsens_work);
	return IRQ_HANDLED;
}

static int init_ipq807x(struct tsens_priv *tmdev)
{
	int ret, i;

	init_common(tmdev);
	if (!tmdev->tm_map)
		return -ENODEV;

	if (device_property_read_bool(tmdev->dev, "tsens-calibration")) {
		ret = init_tsens_ctrl(tmdev);
		if (ret)
			return ret;
	}

	/* Store all sensor address for future use */
	for (i = 0; i < tmdev->num_sensors; i++) {
		tmdev->sensor[i].status = TSENS_TM_SN_STATUS + (i * 4);
		INIT_WORK(&tmdev->sensor[i].notify_work,
						notify_uspace_tsens_fn);
	}

	/* Enable interrupt registers */
	regmap_write(tmdev->tm_map, TSENS_TM_INT_EN, TSENS_TM_CRITICAL_INT_EN
			| TSENS_TM_UPPER_INT_EN	| TSENS_TM_LOWER_INT_EN);

	/* Init tsens worker thread */
	INIT_WORK(&tmdev->tsens_work, tsens_scheduler_fn);

	/* Register and enable the ISR */
	ret = devm_request_irq(tmdev->dev, tmdev->tsens_irq, tsens_isr,
			IRQF_TRIGGER_RISING, "tsens_interrupt", tmdev);
	if (ret < 0) {
		pr_err("%s: request_irq FAIL: %d\n", __func__, ret);
		return ret;
	}
	g_tmdev = tmdev;
	enable_irq_wake(tmdev->tsens_irq);

	int_clr_deassert_quirk = device_property_read_bool(tmdev->dev,
				"tsens-up-low-int-clr-deassert-quirk");
	/* Sync registers */
	mb();
	return 0;
}

static int panic_notify_ipq807x(struct tsens_priv *tmdev, int id)
{
	u32 code, trdy;
	const struct tsens_sensor *s = &tmdev->sensor[id];
	unsigned int try = 0;

	if (!s)
		return -EINVAL;

	if ((s->id < 0) || (s->id > (MAX_SENSOR - 1)))
		return -EINVAL;

	for(try = 0; try < 100; try++)
	{
		try++;
		trdy = readl(tmdev->iomem_base + TSENS_TM_TRDY);

		if (!(trdy & TSENS_TRDY_MASK))
			continue;

		code = readl(tmdev->iomem_base + s->status);

		/* Check whether the temp is valid */
		if (!(code & TSENS_TM_SN_STATUS_VALID_BIT))
			continue;

		pr_emerg("The reading for sensor %d is 0x%08x\n", s->id, code);
		return 0;
	}

	pr_emerg("Couldn't get reading for sensor %d\n", s->id);
	return -EINVAL;
}

static int get_temp_ipq807x(struct tsens_priv *tmdev, int id, int *temp)
{
	int ret, last_temp;
	u32 code, trdy;
	const struct tsens_sensor *s = &tmdev->sensor[id];
	unsigned long timeout;

	if (!s)
		return -EINVAL;

	if ((s->id < 0) || (s->id > (MAX_SENSOR - 1)))
		return -EINVAL;

	timeout = jiffies + usecs_to_jiffies(TSENS_TRDY_TIMEOUT_US);
	do {
		ret = regmap_read(tmdev->tm_map, TSENS_TM_TRDY, &trdy);
		if (ret)
			return ret;

		if (!(trdy & TSENS_TRDY_MASK))
			continue;

		ret = regmap_read(tmdev->tm_map, s->status, &code);
		if (ret)
			return ret;

		/* Check whether the temp is valid */
		if (!(code & TSENS_TM_SN_STATUS_VALID_BIT))
			continue;

		last_temp = code & TSENS_TM_SN_LAST_TEMP_MASK;

		if (last_temp & TSENS_TM_CODE_SIGN_BIT)
			/* Sign extension for negative value */
			last_temp |= (~(TSENS_TM_CODE_BIT_MASK));

		/* convert temp from decicelsius to millicelsius */
		*temp = last_temp*100;

		return 0;
	} while (time_before(jiffies, timeout));

	return -ETIMEDOUT;
}

static int __maybe_unused set_trip_temp_ipq807x(void *data, int trip, int temp)
{
	const struct tsens_sensor *s = data;

	if (!s)
		return -EINVAL;

	if (is_sensor_used_internally(s->id))
		return -EINVAL;

	return set_trip_temp(s->priv, s->id, trip, temp);
}

static int __maybe_unused set_trip_activate_ipq807x(void *data, int trip,
					enum thermal_trip_activation_mode mode)
{
	const struct tsens_sensor *s = data;

	if (!s)
		return -EINVAL;

	if (is_sensor_used_internally(s->id))
		return -EINVAL;

	return set_trip_mode(s->priv, s->id, trip, mode);
}

static int __maybe_unused set_temp_trips_ipq807x(void *data, int low, int high)
{
	const struct tsens_sensor *s = data;

	if (!s)
		return -EINVAL;

	if (is_sensor_used_internally(s->id))
		return -EINVAL;

	return set_temp_trips(s->priv, s->id, low, high);
}

static int calibrate(struct tsens_priv *tmdev)
{
	u32 base0 = 0, base1 = 0;
	u32 tsens_offset[MAX_SENSOR];
	u32 mode = 0, slope = 0, czero = 0, val = 0;
	u32 *qfprom_cal_sel, *qfprom_cdata;
	int i, sensor = 11;
	int temp;


	qfprom_cal_sel = (u32 *)qfprom_read(tmdev->dev, "cal_sel");
	if (IS_ERR(qfprom_cal_sel))
		return PTR_ERR(qfprom_cal_sel);

	mode = (qfprom_cal_sel[0] & CAL_SEL_MASK) >> CAL_SEL_SHIFT;
	dev_dbg(tmdev->dev, "calibration mode is %d\n", mode);

	base0 = (qfprom_cal_sel[0] & BASE0_MASK) >> BASE0_SHIFT;
	base1 = (qfprom_cal_sel[0] & BASE1_MASK) >> BASE1_SHIFT;

	qfprom_cdata = (u32 *)qfprom_read(tmdev->dev, "calib_offset");
	if (IS_ERR(qfprom_cdata))
		return PTR_ERR(qfprom_cdata);

	temp =  qfprom_cdata[0];
	tsens_offset[sensor++] = (temp & TSENS11_OFFSET) >> TSENS11_SHIFT;
	tsens_offset[sensor++] = (temp & TSENS12_OFFSET) >> TSENS12_SHIFT;
	tsens_offset[sensor++] = (temp & TSENS13_OFFSET) >> TSENS13_SHIFT;
	temp =  qfprom_cdata[2];
	tsens_offset[sensor++] = (temp & TSENS14_OFFSET) >> TSENS14_SHIFT;
	tsens_offset[sensor++] = (temp & TSENS15_OFFSET) >> TSENS15_SHIFT;

	switch (mode) {
	case TWO_PT_CALIB:
		slope = (TSENS_TWO_POINT_SLOPE / (base1 - base0));

		for (i = 11; i < MAX_SENSOR; i++) {
			czero = (base0 + tsens_offset[i] -
					((base1 - base0) / 3));
			val = (TSENS_CAL_SHIFT << TSENS_CAL_SHIFT_SHIFT)
				| (slope << TSENS_SLOPE_SHIFT) | czero;

			regmap_write(tmdev->srot_map,
				TSENS_SN_CONVERSION(i), val);
		}
		break;
	case ONE_PT_CALIB2:
		for (i = 11; i < MAX_SENSOR; i++) {
			czero = base0 + tsens_offset[i] -
					TSENS_ONE_PT_CZERO_CONST;
			val = (TSENS_CAL_SHIFT << TSENS_CAL_SHIFT_SHIFT)
				| (TSENS_ONE_POINT_SLOPE << TSENS_SLOPE_SHIFT)
				| czero;

			regmap_write(tmdev->srot_map,
				TSENS_SN_CONVERSION(i), val);
		}
		break;
	default:
		for (i = 11; i < MAX_SENSOR; i++) {
			regmap_write(tmdev->srot_map,
					TSENS_SN_CONVERSION(i),
					TSENS_CONVERSION_DEFAULT);
		}
		pr_info("Using default calibration\n");
		break;
	}

	kfree(qfprom_cdata);
	kfree(qfprom_cal_sel);
	return 0;
}

static int init_tsens_ctrl(struct tsens_priv *tmdev)
{
	int ret, reg_val;

	regmap_write(tmdev->srot_map, TSENS_CNTL_ADDR, TSENS_SN_SW_RST);
	regmap_read(tmdev->srot_map, TSENS_CNTL_ADDR, &reg_val);
	reg_val &= ~TSENS_SN_CTRL_EN;
	regmap_write(tmdev->srot_map, TSENS_CNTL_ADDR, reg_val);
	regmap_write(tmdev->srot_map, TSENS_MEASURE_PERIOD, 0x1);
	reg_val |= TSENS_SENSOR_MASK;
	regmap_write(tmdev->srot_map, TSENS_CNTL_ADDR, reg_val);
	reg_val |= TSENS_SN_CODE_OR_TEMP;
	regmap_write(tmdev->srot_map, TSENS_CNTL_ADDR, reg_val);
	reg_val &= ~TSENS_SN_SW_RST;
	reg_val |= TSENS_SN_CTRL_EN;
	regmap_write(tmdev->srot_map, TSENS_CNTL_ADDR, reg_val);

	ret = calibrate(tmdev);
	return ret;
}

const struct tsens_ops ops_ipq807x = {
	.init		= init_ipq807x,
	.get_temp	= get_temp_ipq807x,
	.panic_notify = panic_notify_ipq807x,
#ifdef CONFIG_CPU_THERMAL
	.set_temp_trips = set_temp_trips_ipq807x,
#else
	.set_trip_temp	= set_trip_temp_ipq807x,
	.set_trip_activate = set_trip_activate_ipq807x,
#endif
};
