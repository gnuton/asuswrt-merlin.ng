// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019, Linaro Limited.
 * Copyright (c) 2015, 2017, 2021 The Linux Foundation. All rights reserved.
 *
 */

#include <linux/bitops.h>
#include <linux/regmap.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/thermal.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include "tsens.h"

/* TSENS register data */
#define TSENS_CNTL_ADDR			0x4
#define TSENS_CONFIG_ADDR		0x50
#define TSENS_TRDY_TIMEOUT_US		20000
#define TSENS_THRESHOLD_MAX_CODE	0x3ff
#define TSENS_THRESHOLD_MIN_CODE	0x0
#define TSENS_SN_CTRL_EN		BIT(0)

#define TSENS_TM_INT_EN_ADDR		0x0
#define TSENS_TM_INT_EN			BIT(0)

#define TSENS_TM_SN_STATUS			0x0044
#define TSENS_TM_SN_STATUS_VALID_BIT		BIT(14)
#define TSENS_TM_SN_STATUS_CRITICAL_STATUS	BIT(19)
#define TSENS_TM_SN_STATUS_UPPER_STATUS		BIT(12)
#define TSENS_TM_SN_STATUS_LOWER_STATUS		BIT(11)
#define TSENS_TM_SN_LAST_TEMP_MASK		0x3ff

#define TSENS_TM_UPPER_LOWER_STATUS_CTRL(n)	((0x4 * n) + 0x0004)
#define TSENS_TM_UPPER_STATUS_CLEAR		BIT(21)
#define TSENS_TM_LOWER_STATUS_CLEAR		BIT(20)
#define TSENS_TM_UPPER_THRESHOLD_SET(n)		((n) << 10)
#define TSENS_TM_UPPER_THRESHOLD_MASK		0xffc00
#define TSENS_TM_LOWER_THRESHOLD_MASK		0x3ff
#define TSENS_TM_UPPER_THRESHOLD_VALUE(n)	(((n) & TSENS_TM_UPPER_THRESHOLD_MASK) >> 10)
#define TSENS_TM_LOWER_THRESHOLD_VALUE(n)	((n) & TSENS_TM_LOWER_THRESHOLD_MASK)

#define MAX_SENSOR				5

/*
 * ADC is 10 bits long so it can't go beyond 1023
 */
#define MAX_ADC		1023
#define MIN_ADC		0

#define MAX_TEMP	243 /* Temperature for adc code 1023 */
#define MIN_TEMP	-77 /* Temperature for adc code 0 */

/* eeprom layout data for ipq5018 */
#define S0_P1_MASK_4_0	0xf8000000
#define BASE0_MASK	0x0007f800
#define BASE1_MASK	0x07f80000
#define CAL_SEL_MASK	0x00000700

#define S3_P1_MASK_0	0x80000000
#define S2_P2_MASK	0x7e000000
#define S2_P1_MASK	0x01f80000
#define S1_P2_MASK	0x0007e000
#define S1_P1_MASK	0x00001f80
#define S0_P2_MASK	0x0000007e
#define S0_P1_MASK_5	0x00000001

#define S4_P1_MASK	0x0001f800
#define S3_P2_MASK	0x000007e0
#define S3_P1_MASK_5_1	0x0000001f

#define S4_P2_MASK	0x0000003f

#define S0_P1_SHIFT_4_0	27
#define BASE0_SHIFT	11
#define BASE1_SHIFT	19
#define CAL_SEL_SHIFT	8

#define S3_P1_SHIFT_0	31
#define S2_P2_SHIFT	25
#define S2_P1_SHIFT	19
#define S1_P2_SHIFT	13
#define S1_P1_SHIFT	7
#define S0_P2_SHIFT	1
#define S0_P1_SHIFT_5	0

#define S4_P1_SHIFT	11
#define S3_P2_SHIFT	5
#define S3_P1_SHIFT_5_1	0

#define S4_P2_SHIFT	0

#define CONFIG			0x9c
#define CONFIG_MASK		0xf
#define CONFIG_SHIFT		0

#define EN                      BIT(0)
#define SW_RST                  BIT(1)
#define SENSOR0_EN              BIT(3)
#define SLP_CLK_ENA             BIT(26)
#define MEASURE_PERIOD          1
#define SENSOR0_SHIFT           3

static int get_temp_ipq5018(struct tsens_priv *tmdev, int id, int *temp);

int code2degc_lut_degc[MAX_SENSOR][1024];

/* Trips: from very hot to very cold */
/* Maintain same enum values as in tsens-ipq807x.c:enum tsens_trip_type */
enum tsens_trip_type {
	TSENS_TRIP_STAGE3 = 0, /* Critical high, not available in IPQ5018 */
	TSENS_TRIP_STAGE2 = 1, /* Configurable high */
	TSENS_TRIP_STAGE1,     /* Configurable low */
	TSENS_TRIP_STAGE0,     /* Critical low, not available in IPQ5018 */
	TSENS_TRIP_NUM,
};

int degc_to_code(int temp, int sensor)
{
	int code;

	for (code = 0; code < 1024; code++) {
		if (temp == code2degc_lut_degc[sensor][code])
			return code;
	}

	return MAX_TEMP;
}

/*
 * Set Trip temp in millidegree Celcius
 */
static int set_trip_temp(struct tsens_priv *tmdev, int sensor,
					enum tsens_trip_type trip, int temp)
{
	u32 reg_th, th_lo, th_hi, reg_th_offset, curr_temp, ret;

	if (!tmdev)
		return -EINVAL;

	if ((sensor < 0) || (sensor > (MAX_SENSOR - 1)))
		return -EINVAL;

	/* convert temp from millicelsius to celsius */
	temp = temp/1000;

	if ((temp < MIN_TEMP) || (temp > MAX_TEMP))
		return -EINVAL;

	temp = degc_to_code(temp, sensor);

	reg_th_offset = TSENS_TM_UPPER_LOWER_STATUS_CTRL(sensor);
	regmap_read(tmdev->tm_map,
		TSENS_TM_UPPER_LOWER_STATUS_CTRL(sensor), &reg_th);

	th_hi = TSENS_TM_UPPER_THRESHOLD_VALUE(reg_th);
	th_lo = TSENS_TM_LOWER_THRESHOLD_VALUE(reg_th);

	switch (trip) {
	case TSENS_TRIP_STAGE2:
		if (temp <= th_lo)
			return -EINVAL;

		temp = TSENS_TM_UPPER_THRESHOLD_SET(temp);
		/* Don't change lower threshold */
		reg_th &= TSENS_TM_LOWER_THRESHOLD_MASK;
		reg_th |= temp;
		break;
	case TSENS_TRIP_STAGE1:
		if (temp >= th_hi)
			return -EINVAL;
	/*
	 * IPQ50XX tsens doesn't support mask interrupt features
	 * so if lower threshold interrupt trigger value set above
	 * the current temp, interrupt triggers continously
	 * To avoid this condition, adding this check before updating
	 * lower threshold interrupt trigger temp value.
	 */
		ret = get_temp_ipq5018(tmdev, sensor, &curr_temp);
		if (ret){
			return ret;
		} else {
			/* convert millicelsius to celsius */
			curr_temp = curr_temp/1000;
			curr_temp = degc_to_code(curr_temp, sensor);
			if (temp >= curr_temp) {
				pr_debug("Skipping setting lower threshold"
					"temp %d higher than current temp %d\n",
					temp, curr_temp);
				goto skip;
			}
		}
		/* Don't change upper threshold */
		reg_th &= TSENS_TM_UPPER_THRESHOLD_MASK;
		reg_th |= temp;
		break;
	default:
		return -EINVAL;
	}

	regmap_write(tmdev->tm_map, TSENS_TM_INT_EN_ADDR, 0);
	regmap_write(tmdev->tm_map, reg_th_offset, reg_th);
	regmap_write(tmdev->tm_map, TSENS_TM_INT_EN_ADDR, TSENS_TM_INT_EN);

	/* Sync registers */
	mb();
skip:
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

	get_temp_ipq5018(tmdev, sensor, &cur_temp);

	reg_th_offset = TSENS_TM_UPPER_LOWER_STATUS_CTRL(sensor);

	regmap_read(tmdev->tm_map, reg_th_offset, &reg_th);

	/*
	 * Clear low-temp if "low" is too small. As per thermal framework
	 * API, we use -INT_MAX rather than INT_MIN.
	 */
	if (low_th <= -INT_MAX) {
		/* clear lower threshold values */
		reg_th &= TSENS_TM_UPPER_THRESHOLD_MASK;
		regmap_write(tmdev->tm_map, reg_th_offset, reg_th);

	} else if (low_th < cur_temp) {
		low_th = low_th / 1000;
		low_th = degc_to_code(low_th, sensor);
		reg_th &= TSENS_TM_UPPER_THRESHOLD_MASK;
		reg_th |= low_th;

		regmap_write(tmdev->tm_map, reg_th_offset, reg_th);
	}

	if (high_th != INT_MAX) {
		high_th = high_th / 1000;
		high_th = degc_to_code(high_th, sensor);
		high_th = TSENS_TM_UPPER_THRESHOLD_SET(high_th);
		reg_th &= TSENS_TM_LOWER_THRESHOLD_MASK;
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

static void tsens_scheduler_fn(struct work_struct *work)
{
	struct tsens_priv *tmdev = container_of(work, struct tsens_priv,
					tsens_work);
	int i, reg_thr, temp, th_upper = 0, th_lower = 0;
	u32 reg_val, reg_addr;

	/*Check whether TSENS is enabled */
	regmap_read(tmdev->srot_map, TSENS_CNTL_ADDR, &reg_val);
	if (!(reg_val & TSENS_SN_CTRL_EN))
		return;

	/* Iterate through all sensors */
	for (i = 0; i < tmdev->num_sensors; i++) {
		/* Reset for each iteration */
		reg_thr = th_upper = th_lower = 0;

		regmap_read(tmdev->tm_map, tmdev->sensor[i].status, &reg_val);
		/* read the current value of status ctrl regiser */
		regmap_read(tmdev->tm_map,
			TSENS_TM_UPPER_LOWER_STATUS_CTRL(i), &reg_thr);

		/* Check whether the temp is valid */
		if (!(reg_val & TSENS_TM_SN_STATUS_VALID_BIT))
			continue;

		/* Check whether upper threshold is hit */
		if (reg_val & TSENS_TM_SN_STATUS_UPPER_STATUS) {
			reg_thr |= TSENS_TM_UPPER_STATUS_CLEAR;
			reg_addr = TSENS_TM_UPPER_LOWER_STATUS_CTRL(i);
			th_upper = 1;
		}
		/* Check whether lower threshold is hit */
		if (reg_val & TSENS_TM_SN_STATUS_LOWER_STATUS) {
			reg_thr |= TSENS_TM_LOWER_STATUS_CLEAR;
			reg_addr = TSENS_TM_UPPER_LOWER_STATUS_CTRL(i);
			th_lower = 1;
		}

		if (th_upper || th_lower) {
			regmap_write(tmdev->tm_map, reg_addr, reg_thr);
#ifdef CONFIG_CPU_THERMAL
			/* If CPUFreq cooling is enabled, then notify Thermal framework */
			thermal_zone_device_update(tmdev->sensor[i].tzd, THERMAL_EVENT_UNSPECIFIED);
			regmap_read(tmdev->tm_map, reg_addr, &reg_thr);
#else
			/* Notify user space */
			schedule_work(&tmdev->sensor[i].notify_work);
#endif

			if (th_lower &&
				((reg_thr & TSENS_TM_LOWER_THRESHOLD_MASK) >=
				(reg_val & TSENS_TM_LOWER_THRESHOLD_MASK))){
			/*
			 * if lower threshold temp is higher than current temp
			 * clearing lower threshold to avoid flooding of the
			 * lower threshold interrupt.
			 */
				reg_thr &= ~(TSENS_TM_LOWER_THRESHOLD_MASK);
			}
			/* clear both clear interupt status bit */
			reg_thr &= ~(TSENS_TM_LOWER_STATUS_CLEAR ||
					TSENS_TM_UPPER_STATUS_CLEAR);

			regmap_write(tmdev->tm_map, reg_addr, reg_thr);

			if (!get_temp_ipq5018(tmdev, i, &temp))
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

static int init_ipq5018(struct tsens_priv *tmdev)
{
	int ret;
	int i;
	u32 reg_cntl;

	init_common(tmdev);
	if (!tmdev->tm_map)
		return -ENODEV;

	/* Store all sensor address for future use */
	for (i = 0; i < tmdev->num_sensors; i++) {
		tmdev->sensor[i].status = TSENS_TM_SN_STATUS + (i * 4);
		INIT_WORK(&tmdev->sensor[i].notify_work,
						notify_uspace_tsens_fn);
	}

	reg_cntl = SW_RST;
	ret = regmap_update_bits(tmdev->srot_map, TSENS_CNTL_ADDR, SW_RST, reg_cntl);
	if (ret)
		return ret;

	reg_cntl |= SLP_CLK_ENA | (MEASURE_PERIOD << 18);
	reg_cntl &= ~SW_RST;
	ret = regmap_update_bits(tmdev->srot_map, TSENS_CONFIG_ADDR,
				CONFIG_MASK, CONFIG);

	reg_cntl |= GENMASK(MAX_SENSOR - 1, 0) << SENSOR0_SHIFT;
	ret = regmap_write(tmdev->srot_map, TSENS_CNTL_ADDR, reg_cntl);
	if (ret)
		return ret;

	reg_cntl |= EN;
	ret = regmap_write(tmdev->srot_map, TSENS_CNTL_ADDR, reg_cntl);
	if (ret)
		return ret;

	/* Init tsens worker thread */
	INIT_WORK(&tmdev->tsens_work, tsens_scheduler_fn);

	/* Register and enable the ISR */
	ret = devm_request_irq(tmdev->dev, tmdev->tsens_irq, tsens_isr,
			IRQF_TRIGGER_RISING, "tsens_interrupt", tmdev);
	if (ret < 0) {
		pr_err("%s: request_irq FAIL: %d\n", __func__, ret);
		return ret;
	}
	enable_irq_wake(tmdev->tsens_irq);

	regmap_write(tmdev->tm_map, TSENS_TM_INT_EN_ADDR, TSENS_TM_INT_EN);

	/* Sync registers */
	mb();

	return 0;
}

static inline int code_to_degc(u32 adc_code, const struct tsens_sensor *s)
{
	int degc, num, den;

	num = (adc_code * SLOPE_FACTOR) - s->offset;
	den = s->slope;

	if (num > 0)
		degc = num + (den / 2);
	else if (num < 0)
		degc = num - (den / 2);
	else
		degc = num;

	degc /= den;

	return degc;
}

static int calibrate_ipq5018(struct tsens_priv *tmdev)
{
	u32 base0 = 0, base1 = 0;
	u32 p1[MAX_SENSOR], p2[MAX_SENSOR];
	u32 mode = 0, lsb = 0, msb = 0;
	u32 *qfprom_cdata;
	int i, sensor;

	qfprom_cdata = (u32 *)qfprom_read(tmdev->dev, "calib");
	if (IS_ERR(qfprom_cdata))
		return PTR_ERR(qfprom_cdata);

	mode = (qfprom_cdata[0] & CAL_SEL_MASK) >> CAL_SEL_SHIFT;

	dev_dbg(tmdev->dev, "calibration mode is %d\n", mode);

	switch (mode) {
	case TWO_PT_CALIB:
		base1 = (qfprom_cdata[0] & BASE1_MASK) >> BASE1_SHIFT;
		p2[0] = (qfprom_cdata[1] & S0_P2_MASK) >> S0_P2_SHIFT;
		p2[1] = (qfprom_cdata[1] & S1_P2_MASK) >> S1_P2_SHIFT;
		p2[2] = (qfprom_cdata[1] & S2_P2_MASK) >> S2_P2_SHIFT;
		p2[3] = (qfprom_cdata[2] & S3_P2_MASK) >> S3_P2_SHIFT;
		p2[4] = (qfprom_cdata[3] & S4_P2_MASK) >> S4_P2_SHIFT;
		for (i = 0; i < MAX_SENSOR; i++)
			p2[i] = ((base1 + p2[i]) << 2);
		/* Fall through */
	case ONE_PT_CALIB2:
		base0 = (qfprom_cdata[0] & BASE0_MASK) >> BASE0_SHIFT;
		/* This value is split over two registers, 5 bits and 1 bit */
		lsb = (qfprom_cdata[0] & S0_P1_MASK_4_0) >> S0_P1_SHIFT_4_0;
		msb = (qfprom_cdata[1] & S0_P1_MASK_5) >> S0_P1_SHIFT_5;
		p1[0] = msb << 5 | lsb;
		p1[1] = (qfprom_cdata[1] & S1_P1_MASK) >> S1_P1_SHIFT;
		p1[2] = (qfprom_cdata[1] & S2_P1_MASK) >> S2_P1_SHIFT;
		/* This value is split over two registers, 1 bit and 5 bits */
		lsb = (qfprom_cdata[1] & S3_P1_MASK_0) >> S3_P1_SHIFT_0;
		msb = (qfprom_cdata[2] & S3_P1_MASK_5_1) >> S3_P1_SHIFT_5_1;
		p1[3] = msb << 1 | lsb;
		p1[4] = (qfprom_cdata[2] & S4_P1_MASK) >> S4_P1_SHIFT;
		for (i = 0; i < MAX_SENSOR; i++)
			p1[i] = (((base0) + p1[i]) << 2);
		break;
	default:
		pr_info("Using default calibration\n");
		p1[0] = 403;
		p2[0] = 688;
		p1[1] = 390;
		p2[1] = 674;
		p1[2] = 341;
		p2[2] = 635;
		p1[3] = 387;
		p2[3] = 673;
		p1[4] = 347;
		p2[4] = 639;
		break;
	}

	compute_intercept_slope(tmdev, p1, p2, mode);
	kfree(qfprom_cdata);

	for (sensor = 0; sensor < tmdev->num_sensors; sensor++) {
		for (i = MIN_ADC; i <= MAX_ADC; i++) {
			code2degc_lut_degc[sensor][i] = code_to_degc(i,
							&tmdev->sensor[sensor]);
		}
	}

	return 0;
}

static int __maybe_unused set_trip_temp_ipq5018(void *data, int trip, int temp)
{
	const struct tsens_sensor *s = data;

	if (!s)
		return -EINVAL;

	return set_trip_temp(s->priv, s->id, trip, temp);
}

static int __maybe_unused set_temp_trips_ipq5018(void *data, int low, int high)
{
	const struct tsens_sensor *s = data;

	if (!s)
		return -EINVAL;

	return set_temp_trips(s->priv, s->id, low, high);
}

static int get_temp_ipq5018(struct tsens_priv *tmdev, int id, int *temp)
{
	const struct tsens_sensor *s = &tmdev->sensor[id];
	u32 code;
	unsigned int sensor_addr;
	int ret, last_temp = 0, last_temp2 = 0, last_temp3 = 0;

	sensor_addr = TSENS_TM_SN_STATUS + s->hw_id * 4;
	ret = regmap_read(tmdev->tm_map, sensor_addr, &code);
	if (ret)
		return ret;
	last_temp = code & TSENS_TM_SN_LAST_TEMP_MASK;
	if (code & TSENS_TM_SN_STATUS_VALID_BIT)
		goto done;

	/* Try a second time */
	ret = regmap_read(tmdev->tm_map, sensor_addr, &code);
	if (ret)
		return ret;
	if (code & TSENS_TM_SN_STATUS_VALID_BIT) {
		last_temp = code & TSENS_TM_SN_LAST_TEMP_MASK;
		goto done;
	} else {
		last_temp2 = code & TSENS_TM_SN_LAST_TEMP_MASK;
	}

	/* Try a third/last time */
	ret = regmap_read(tmdev->tm_map, sensor_addr, &code);
	if (ret)
		return ret;
	if (code & TSENS_TM_SN_STATUS_VALID_BIT) {
		last_temp = code & TSENS_TM_SN_LAST_TEMP_MASK;
		goto done;
	} else {
		last_temp3 = code & TSENS_TM_SN_LAST_TEMP_MASK;
	}

	if (last_temp == last_temp2)
		last_temp = last_temp2;
	else if (last_temp2 == last_temp3)
		last_temp = last_temp3;
done:
	/* Convert temperature from ADC code to milliCelsius */
	*temp = code_to_degc(last_temp, s) * 1000;

	return 0;
}

const struct tsens_ops ops_ipq5018 = {
	.init		= init_ipq5018,
	.calibrate	= calibrate_ipq5018,
	.get_temp	= get_temp_ipq5018,
#ifdef CONFIG_CPU_THERMAL
	.set_temp_trips = set_temp_trips_ipq5018,
#else
	.set_trip_temp	= set_trip_temp_ipq5018,
#endif
};
