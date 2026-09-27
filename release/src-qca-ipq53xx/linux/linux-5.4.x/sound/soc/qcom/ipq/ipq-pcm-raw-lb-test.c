/* Copyright (c) 2012-2013,2015-2016, The Linux Foundation. All rights reserved.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

#include <linux/init.h>
#include <linux/sched.h>
#include <uapi/linux/sched/types.h>
#include <linux/err.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/time.h>
#include <linux/wait.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/string.h>
#include <linux/platform_device.h>
#include <linux/of_device.h>
#ifdef CONFIG_SND_SOC_IPQ_LPASS
#include "ipq-lpass-tdm-pcm.h"
#else
#include "ipq-pcm-raw.h"
#endif

/*
 * This is an external loopback test module for PCM interface.
 * For this to work, the Rx and Tx should be shorted in the
 * SLIC header.
 * This test module exposes a sysfs interface to start\stop the
 * tests. This module sends a sequence of numbers starting from
 * 0 to 255 and wraps around to 0 and continues the sequence.
 * The received data is then compared for the sequence. Any errors
 * found are reported immediately. When test stop, a final statistics
 * of how much passed and failed is displayed.
 */

static void pcm_start_test(void);
static void ipq8074_pcm_fill_data(uint8_t *tx_buff, uint32_t size);
static void ipq5018_pcm_fill_data(uint32_t *tx_buff, uint32_t size);

/* the test configurations supported */
#define PCM_LBTEST_8BIT_8KHZ_4CH_TX_TO_RX	1
#define PCM_LBTEST_8BIT_8KHZ_4CH_RX_TO_TX	101
#define PCM_LBTEST_16BIT_8KHZ_2CH_TX_TO_RX	2
#define PCM_LBTEST_16BIT_8KHZ_2CH_RX_TO_TX	201
#define PCM_LBTEST_16BIT_8KHZ_4CH_TX_TO_RX	3
#define PCM_LBTEST_16BIT_8KHZ_4CH_RX_TO_TX	301
#define PCM_LBTEST_8BIT_16KHZ_4CH_TX_TO_RX	4
#define PCM_LBTEST_8BIT_16KHZ_4CH_RX_TO_TX	401
#define PCM_LBTEST_16BIT_16KHZ_2CH_TX_TO_RX	5
#define PCM_LBTEST_16BIT_16KHZ_2CH_RX_TO_TX	501
#define PCM_LBTEST_16BIT_16KHZ_4CH_TX_TO_RX	6
#define PCM_LBTEST_16BIT_16KHZ_4CH_RX_TO_TX	601
#define PCM_LBTEST_16BIT_8KHZ_16CH_TX_TO_RX	7
#define PCM_LBTEST_16BIT_8KHZ_16CH_RX_TO_TX	701
/* The max value for loopback test config is 601(3 digits + 1 null byte)
 * This macro needs to be updated when more configs are added.
 */
#define PCM_LBTEST_CFG_MAX_DIG_COUNT		4

#define IS_PCM_LBTEST_RX_TO_TX(config)					\
		((config == PCM_LBTEST_8BIT_8KHZ_4CH_RX_TO_TX) ||	\
		(config == PCM_LBTEST_16BIT_8KHZ_2CH_RX_TO_TX) ||	\
		(config == PCM_LBTEST_16BIT_8KHZ_4CH_RX_TO_TX) ||	\
		(config == PCM_LBTEST_8BIT_16KHZ_4CH_RX_TO_TX) ||	\
		(config == PCM_LBTEST_16BIT_16KHZ_2CH_RX_TO_TX) ||	\
		(config == PCM_LBTEST_16BIT_16KHZ_4CH_RX_TO_TX) ||	\
		(config == PCM_LBTEST_16BIT_8KHZ_16CH_RX_TO_TX))

#define LOOPBACK_FAIL_THRESHOLD		200

struct pcm_lb_test_ctx {
	uint32_t failed;
	uint32_t passed;
	uint32_t *rx_buf;
	uint8_t *last_rx_buff;
	int running;
	uint16_t tx_data;
	uint16_t expected_rx_seq;
	int read_count;
	struct task_struct *task;
};

static struct pcm_lb_test_ctx ctx;
static unsigned long int start;
#ifdef CONFIG_SND_SOC_IPQ_LPASS
struct ipq_lpass_pcm_params cfg_params;
#else
struct ipq_pcm_params cfg_params;
#define IPQ5018				2
#define IPQ9574				3
#endif
uint8_t *prev_buf;
static enum ipq_hw_type ipq_hw;

static ssize_t pcmlb_show(struct device_driver *driver,
						char *buff)
{
	return snprintf(buff, PCM_LBTEST_CFG_MAX_DIG_COUNT, "%ld", start);
}

static ssize_t pcmlb_store(struct device_driver *driver,
				const char *buff, size_t count)
{
	if (kstrtoul(buff, 0, &start)) {
		pr_err("%s: invalid lb value\n", __func__);
		return -EINVAL;
	}
	pcm_start_test();
	return count;
}

static DRIVER_ATTR_RW(pcmlb);

static int pcm_lb_drv_attr_init(struct platform_device *pdev)
{
	start = 0;
	return driver_create_file(pdev->dev.driver, &driver_attr_pcmlb);
}

static void pcm_lb_drv_attr_deinit(struct platform_device *pdev)
{
	driver_remove_file(pdev->dev.driver, &driver_attr_pcmlb);
}

uint32_t pcm_read_write(void)
{
	uint8_t *rx_buff;
	uint8_t *tx_buff;
	uint32_t size;

	size = ipq_pcm_data(&rx_buff, &tx_buff);
	prev_buf = ctx.last_rx_buff;
	ctx.last_rx_buff = rx_buff;
	ctx.rx_buf = (uint32_t *)rx_buff;

	if (IS_PCM_LBTEST_RX_TO_TX(start)) {
		/* Redirect Rx data to Tx */
		memcpy(tx_buff, rx_buff, size);
	} else {
		/* get current Tx buffer and write the pattern
		* We will write 1, 2, 3, ..., 255, 1, 2, 3...
		*/
		if (ipq_hw == IPQ5018 || ipq_hw == IPQ9574)
			ipq5018_pcm_fill_data((uint32_t *)tx_buff,
						(size / sizeof(uint32_t)));
		else
			ipq8074_pcm_fill_data(tx_buff, size);
	}
	ipq_pcm_done();
	return size;
}

uint32_t pcm_init(void)
{
	uint32_t ret = 0;

	switch (start) {
	case PCM_LBTEST_8BIT_8KHZ_4CH_TX_TO_RX:
	case PCM_LBTEST_8BIT_8KHZ_4CH_RX_TO_TX:
		cfg_params.bit_width = 8;
		cfg_params.rate = 8000;
		cfg_params.slot_count = 32;
		cfg_params.active_slot_count = 4;
		cfg_params.tx_slots[0] = 0;
		cfg_params.tx_slots[1] = 1;
		cfg_params.rx_slots[0] = 0;
		cfg_params.rx_slots[1] = 1;
		cfg_params.tx_slots[2] = 2;
		cfg_params.tx_slots[3] = 3;
		cfg_params.rx_slots[2] = 2;
		cfg_params.rx_slots[3] = 3;
		ret = ipq_pcm_init(&cfg_params);
		break;

	case PCM_LBTEST_16BIT_8KHZ_2CH_TX_TO_RX:
	case PCM_LBTEST_16BIT_8KHZ_2CH_RX_TO_TX:
		cfg_params.bit_width = 16;
		cfg_params.rate = 8000;
		cfg_params.slot_count = 16;
		cfg_params.active_slot_count = 2;
		cfg_params.tx_slots[0] = 0;
		cfg_params.tx_slots[1] = (ipq_hw == IPQ5018 ||
						ipq_hw == IPQ9574)? 1 : 3;
		cfg_params.rx_slots[0] = 0;
		cfg_params.rx_slots[1] = (ipq_hw == IPQ5018 ||
						ipq_hw == IPQ9574)? 1 : 3;
		ret = ipq_pcm_init(&cfg_params);
		break;

	case PCM_LBTEST_16BIT_8KHZ_4CH_TX_TO_RX:
	case PCM_LBTEST_16BIT_8KHZ_4CH_RX_TO_TX:
		cfg_params.bit_width = 16;
		cfg_params.rate = 8000;
		cfg_params.slot_count = 16;
		cfg_params.active_slot_count = 4;
		cfg_params.tx_slots[0] = 0;
		cfg_params.tx_slots[1] = 1;
		cfg_params.tx_slots[2] = 8;
		cfg_params.tx_slots[3] = 9;
		cfg_params.rx_slots[0] = 0;
		cfg_params.rx_slots[1] = 1;
		cfg_params.rx_slots[2] = 8;
		cfg_params.rx_slots[3] = 9;
		ret = ipq_pcm_init(&cfg_params);
		break;

	case PCM_LBTEST_8BIT_16KHZ_4CH_TX_TO_RX:
	case PCM_LBTEST_8BIT_16KHZ_4CH_RX_TO_TX:
		cfg_params.bit_width = 8;
		cfg_params.rate = 16000;
		cfg_params.slot_count = 32;
		cfg_params.active_slot_count = 4;
		cfg_params.tx_slots[0] = 0;
		cfg_params.tx_slots[1] = 1;
		cfg_params.rx_slots[0] = 0;
		cfg_params.rx_slots[1] = 1;
		cfg_params.tx_slots[2] = 2;
		cfg_params.tx_slots[3] = 3;
		cfg_params.rx_slots[2] = 2;
		cfg_params.rx_slots[3] = 3;
		ret = ipq_pcm_init(&cfg_params);
		break;

	case PCM_LBTEST_16BIT_16KHZ_2CH_TX_TO_RX:
	case PCM_LBTEST_16BIT_16KHZ_2CH_RX_TO_TX:
		cfg_params.bit_width = 16;
		cfg_params.rate = 16000;
		cfg_params.slot_count = 16;
		cfg_params.active_slot_count = 2;
		cfg_params.tx_slots[0] = 0;
		cfg_params.tx_slots[1] = 3;
		cfg_params.rx_slots[0] = 0;
		cfg_params.rx_slots[1] = 3;
		ret = ipq_pcm_init(&cfg_params);
		break;

	case PCM_LBTEST_16BIT_16KHZ_4CH_TX_TO_RX:
	case PCM_LBTEST_16BIT_16KHZ_4CH_RX_TO_TX:
		cfg_params.bit_width = 16;
		cfg_params.rate = 16000;
		cfg_params.slot_count = 16;
		cfg_params.active_slot_count = 4;
		cfg_params.tx_slots[0] = 0;
		cfg_params.tx_slots[1] = 1;
		cfg_params.tx_slots[2] = 8;
		cfg_params.tx_slots[3] = 9;
		cfg_params.rx_slots[0] = 0;
		cfg_params.rx_slots[1] = 1;
		cfg_params.rx_slots[2] = 8;
		cfg_params.rx_slots[3] = 9;
		ret = ipq_pcm_init(&cfg_params);
		break;

	case PCM_LBTEST_16BIT_8KHZ_16CH_TX_TO_RX:
	case PCM_LBTEST_16BIT_8KHZ_16CH_RX_TO_TX:
		cfg_params.bit_width = 16;
		cfg_params.rate = 8000;
		cfg_params.slot_count = 16;
		cfg_params.active_slot_count = 16;
		cfg_params.tx_slots[0] = 9;
		cfg_params.tx_slots[1] = 5;
		cfg_params.tx_slots[2] = 0;
		cfg_params.tx_slots[3] = 7;
		cfg_params.tx_slots[4] = 10;
		cfg_params.tx_slots[5] = 3;
		cfg_params.tx_slots[6] = 2;
		cfg_params.tx_slots[7] = 8;
		cfg_params.tx_slots[8] = 11;
		cfg_params.tx_slots[9] = 14;
		cfg_params.tx_slots[10] = 6;
		cfg_params.tx_slots[11] = 1;
		cfg_params.tx_slots[12] = 15;
		cfg_params.tx_slots[13] = 13;
		cfg_params.tx_slots[14] = 12;
		cfg_params.tx_slots[15] = 4;
		cfg_params.rx_slots[0] = 9;
		cfg_params.rx_slots[1] = 5;
		cfg_params.rx_slots[2] = 0;
		cfg_params.rx_slots[3] = 7;
		cfg_params.rx_slots[4] = 10;
		cfg_params.rx_slots[5] = 3;
		cfg_params.rx_slots[6] = 2;
		cfg_params.rx_slots[7] = 8;
		cfg_params.rx_slots[8] = 11;
		cfg_params.rx_slots[9] = 14;
		cfg_params.rx_slots[10] = 6;
		cfg_params.rx_slots[11] = 1;
		cfg_params.rx_slots[12] = 15;
		cfg_params.rx_slots[13] = 13;
		cfg_params.rx_slots[14] = 12;
		cfg_params.rx_slots[15] = 4;
		ret = ipq_pcm_init(&cfg_params);
		break;

	default:
		ret = -EINVAL;
		pr_err("Unknown configuration\n");
	}

	return ret;
}

void pcm_deinit(void)
{
	memset((void *)&ctx, 0, sizeof(ctx));
	start = 0;
	ipq_pcm_deinit(&cfg_params);
}

void process_read(uint32_t size)
{
	uint32_t index;
	static uint32_t continuous_failures;
	uint32_t *data_u32;
	uint16_t *data_u16;
	uint16_t val;
	uint16_t expected_val, rec_val;
	/* get out if test stopped */
	if (start == 0)
		return;

	ctx.read_count++;

	if (ctx.read_count <= LOOPBACK_SKIP_COUNT) {
		/*
		 * As soon as do pcm init, the DMA would start. So the initial
		 * few rw till the 1st Rx is called will be 0's, so we skip
		 * few reads so that our loopback settles down.
		 * Note: our 1st loopback Tx is only after an RX is called.
		 */
		return;
	} else if (ctx.read_count == (LOOPBACK_SKIP_COUNT + 1)) {
		/*
		 * our loopback should have settled, so start looking for the
		 * sequence from here. we check only for the data, not for slot
		 */
		if (ipq_hw == IPQ5018 || ipq_hw == IPQ9574) {
			ctx.expected_rx_seq = ((uint32_t *)ctx.rx_buf)[0]
								& 0xFFFF;
		} else {
			if (cfg_params.bit_width == 16)
				ctx.expected_rx_seq = ((uint32_t *)ctx.last_rx_buff)[0]
								& 0xFFFF;
			else
				ctx.expected_rx_seq = ((uint16_t *)ctx.last_rx_buff)[0]
								& 0xFF;
		}
	}

	if (ipq_hw == IPQ5018 || ipq_hw == IPQ9574) {
		data_u32 = ctx.rx_buf;
	} else {
		data_u32 = (uint32_t *)ctx.last_rx_buff;
		data_u16 = (uint16_t *)ctx.last_rx_buff;
	}
	val = ctx.expected_rx_seq;

	if (cfg_params.bit_width == 16 || ipq_hw == IPQ5018 || ipq_hw == IPQ9574)
		size = size / 4; /* as we are checking data as uint32 */
	else
		size = size / 2;

	for (index = 0; index < size; index++) {
		if (cfg_params.bit_width == 16 || ipq_hw == IPQ5018 || ipq_hw == IPQ9574) {
			expected_val = val;
			rec_val = data_u32[index] & (0xFFFF);
		} else {
			expected_val = val % 256;
			rec_val = data_u16[index] & (0xFF);
		}
		if (expected_val != rec_val) {
			if (ipq_hw == IPQ5018 || ipq_hw == IPQ9574)
				pr_err("\n Rx(%d) Failed at index %d:"
					" Expected : 0x%x Received : 0x%x"
					" index: 0x%x\n", ctx.read_count,
					index, expected_val, rec_val, index);
			else
				pr_err("\nRx(%d) Failed at index %d:"
					" Expected : 0x%x Received : 0x%x "
					" Data: 0x%x\n buf_addr: %p "
					" prev_buf_addr: %p\n", ctx.read_count,
					index, expected_val, rec_val,
					((cfg_params.bit_width == 16) ?
					data_u32[index] : data_u16[index]),
					(void *)ctx.last_rx_buff,
					(void *)prev_buf);
		break;
		}
		val++;
	}

	ctx.expected_rx_seq += size;

	if (index == size) {
		ctx.passed++;
		continuous_failures = 0;
	} else {
		ctx.failed++;
		continuous_failures++;
	}

	/* Abort if there are more failures */
	if (continuous_failures >= LOOPBACK_FAIL_THRESHOLD) {
		pr_err("\nAborting loopback test as there are %d"
				" continuous failures\n", continuous_failures);
		continuous_failures = 0;
		ctx.running = 0; /* stops test thread (current) */
	}
}

static void ipq8074_pcm_fill_data(uint8_t *tx_buff, uint32_t size)
{
	uint32_t i, slot, size_act;
	uint32_t *buffer_32;
	uint16_t *buffer_16;

	buffer_32 = (uint32_t *)tx_buff;
	buffer_16 = (uint16_t *)tx_buff;

	/* get out if test stopped */
	if (ctx.running == 0)
		return;

#ifndef CONFIG_SND_SOC_IPQ_LPASS
	size_act = size / IPQ8074_PCM_BYTES_PER_SAMPLE(cfg_params.bit_width);
#else
	size_act = size;
#endif
	for (i = 0; i < size_act; ) {
		for (slot = 0; slot < cfg_params.active_slot_count; slot++) {
			if (cfg_params.bit_width == 16) {
				buffer_32[i] = ctx.tx_data;
				buffer_32[i] |= cfg_params.tx_slots[slot] << 16;
			} else {
				buffer_16[i] = ctx.tx_data % 256;
				buffer_16[i] |= cfg_params.tx_slots[slot] << 8;
				buffer_16[i] |= 1 << 15; /* valid bit */
			}
			ctx.tx_data++;
			i++;
		}
	}
}

static void ipq5018_pcm_fill_data(uint32_t *tx_buff, uint32_t size)
{
	uint32_t i, slot = 0;

	/* get out if test stopped */
	if (ctx.running == 0)
		return;

	for (i = 0; i < size;) {
		tx_buff[i] = ctx.tx_data++;
		++i;
		slot = slot;
	}
}

int pcm_test_rw(void *data)
{
	uint32_t ret;
	uint32_t size;
	struct sched_param param;

	/*
	 * set test thread priority as 90, this is to align with what
	 * D2 VOIP stack does.
	 */
	param.sched_priority = 90;
	ret = sched_setscheduler(ctx.task, SCHED_FIFO, &param);
	if (ret)
		pr_err("%s : Error setting priority, error: %d\n",
						__func__, ret);

	ret = pcm_init();
	if (ret) {
		pr_err("Pcm init failed %d\n", ret);
		return ret;
	}

	pr_notice("%s : Test thread started\n", __func__);
	ctx.running = 1;

	while (ctx.running) {
		size = pcm_read_write();
		if (!IS_PCM_LBTEST_RX_TO_TX(start))
			process_read(size);
	}
	pr_notice("%s : Test Thread stopped\n", __func__);
	/* for rx to tx loopback, we cannot detect failures */
	if (!IS_PCM_LBTEST_RX_TO_TX(start))
		pr_notice("\nPassed : %d, Failed : %d\n",
					ctx.passed, ctx.failed);
	pcm_deinit();
	return 0;
}

static void pcm_start_test(void)
{
	pr_notice("%s : %ld\n", __func__, start);
	if (start) {
		if (ctx.running) {
			pr_notice("%s : Test already running\n", __func__);
		} else {
			ctx.task = kthread_create(&pcm_test_rw,
						NULL, "PCMTest");
			if (ctx.task)
				wake_up_process(ctx.task);
		}
	} else {
		if (ctx.running) {
			pr_notice("%s : Stopping test\n", __func__);
			ctx.running = 0;
			start = 0;
			ipq_pcm_send_event();
			/* wait sufficient time for test thread to finish */
			mdelay(2000);
		} else
			pr_notice("%s : Test already stopped\n", __func__);
	}
}

static const struct of_device_id qca_raw_lb_match_table[] = {
	{ .compatible = "qca,ipq8074-pcm-lb", .data = (void *)IPQ8074 },
	{ .compatible = "qca,ipq5018-pcm-lb", .data = (void *)IPQ5018 },
	{ .compatible = "qca,ipq9574-pcm-lb", .data = (void *)IPQ9574 },
	{},
};

static int ipq_pcm_lb_probe(struct platform_device *pdev)
{
	const struct of_device_id *match;

	start = 0;

	match = of_match_device(qca_raw_lb_match_table, &pdev->dev);
	if (!match)
		return -ENODEV;

	ipq_hw = (enum ipq_hw_type)match->data;

	return pcm_lb_drv_attr_init(pdev);
}

static int ipq_pcm_lb_remove(struct platform_device *pdev)
{
	pcm_lb_drv_attr_deinit(pdev);
	if (ctx.running) {
		ctx.running = 0;
		/* wait sufficient time for test thread to finish */
		mdelay(2000);
	}
	return 0;
}

#define DRIVER_NAME "ipq_pcm_raw_lb"

static struct platform_driver ipq_pcm_raw_driver_test = {
	.probe          = ipq_pcm_lb_probe,
	.remove         = ipq_pcm_lb_remove,
	.driver = {
		.name           = DRIVER_NAME,
		.of_match_table = qca_raw_lb_match_table,
	},
};

module_platform_driver(ipq_pcm_raw_driver_test);

MODULE_ALIAS(DRIVER_NAME);
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("QTI RAW PCM VoIP Platform Driver Loopback Test");
