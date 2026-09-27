/*
 * Copyright (c) 2021 The Linux Foundation. All rights reserved.
 *
 * Copyright (c) 2021 Qualcomm Innovation Center, Inc. All rights reserved.
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
 */

#include <linux/init.h>
#include <linux/err.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/time.h>
#include <linux/wait.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/platform_device.h>
#include <sound/pcm.h>
#include <linux/clk.h>
#include <linux/gpio.h>
#include <linux/irq.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/wait.h>
#include <linux/io.h>
#include <linux/of_device.h>

#include "ipq-lpass.h"
#include "ipq-lpass-tdm-pcm.h"

#define DEFAULT_CLK_RATE		2048000
#define PCM_VOICE_LOOPBACK_BUFFER_SIZE	0x1000
#define PCM_VOICE_LOOPBACK_INTR_SIZE	0x800

/*
 * Global Constant Definitions
 */
void __iomem *ipq_lpass_lpaif_base;
void __iomem *ipq_lpass_lpm_base;

/*
 * Static Variable Definitions
 */
static atomic_t data_avail;
static atomic_t rx_add;
static struct lpass_dma_buffer *rx_dma_buffer;
static struct lpass_dma_buffer *tx_dma_buffer;
static struct platform_device *pcm_pdev;
static spinlock_t pcm_lock;
static struct ipq_lpass_pcm_params *pcm_params;
static uint32_t voice_loopback;

static DECLARE_WAIT_QUEUE_HEAD(pcm_q);

static uint32_t ipq_lpass_pcm_get_dataptr(struct lpass_dma_buffer *buffer,
							uint32_t addr)
{
	uint32_t dataptr, offset, dma_at;
	uint32_t no_of_buffers = buffer->no_of_buffers;

	/* debug purpose */
	buffer->dma_last_curr_addr = addr;

	offset = addr - buffer->dma_base_address;
	dma_at = (offset / (buffer->dma_buffer_size / no_of_buffers));
	dataptr = (((dma_at + (no_of_buffers - 1)) % no_of_buffers));

	return dataptr;
}

/*
 * FUNCTION: ipq_lpass_pcm_irq_handler
 *
 * DESCRIPTION: pcm dma tx irq handler
 *
 * RETURN VALUE: none
 */
static irqreturn_t ipq_lpass_pcm_irq_handler(int intrsrc, void *data)
{
	uint32_t status = 0;
	uint32_t curr_addr;
	uint32_t buffer_idx;
	struct lpass_irq_buffer *buffer = (struct lpass_irq_buffer *)data;
	struct lpass_dma_buffer *rx_buffer = buffer->rx_buffer;

	ipq_lpass_dma_read_interrupt_status(ipq_lpass_lpaif_base,
						INTERRUPT_CHANNEL0, &status);
	while (status) {
/*
 * Both Tx and Rx interrupt use same IRQ number
 * so Calculating RX buffer pointer is sufficient
 * since RX and Tx configure same buffer size
 * and interrupt size
 */
		if (status & (0x8000)) {
			ipq_lpass_dma_get_curr_addr(ipq_lpass_lpaif_base,
						rx_buffer->idx,
						rx_buffer->dir,
						&curr_addr);

			buffer_idx = ipq_lpass_pcm_get_dataptr(rx_buffer,
								curr_addr);

			atomic_set(&rx_add, buffer_idx);
			atomic_set(&data_avail, 1);
			wake_up_interruptible(&pcm_q);
		}

		ipq_lpass_dma_clear_interrupt(ipq_lpass_lpaif_base,
					INTERRUPT_CHANNEL0, status);
		status = 0;

		ipq_lpass_dma_read_interrupt_status(ipq_lpass_lpaif_base,
						INTERRUPT_CHANNEL0, &status);
	}
	return IRQ_HANDLED;
}

/*
 * FUNCTION: ipq_lpass_pcm_validate_params
 *
 * DESCRIPTION: validates the input parameters
 *
 * RETURN VALUE: error if any
 */
uint32_t ipq_lpass_pcm_validate_params(struct ipq_lpass_pcm_params *params,
					struct ipq_lpass_pcm_config *config)
{
	int32_t i, slot_mask;

	memset(config, 0, sizeof(*config));

	if (!params) {
		pr_err("%s: Invalid Params.\n", __func__);
		return -EINVAL;
	}

	if (!((params->bit_width == 8) ||
		(params->bit_width == 16))) {
		pr_err("%s: Invalid Bitwidth %d.\n",
				__func__, params->bit_width);
		return -EINVAL;
	}

	if ((params->rate <  IPQ_PCM_SAMPLING_RATE_MIN) ||
		(params->rate > IPQ_PCM_SAMPLING_RATE_MAX)) {
		pr_err("%s: Invalid sampling rate %d.\n",
				__func__, params->rate);
		return -EINVAL;
	}

	if (params->slot_count >
			IPQ_PCM_MAX_SLOTS_PER_FRAME){
		pr_err("%s: Invalid nslots per frame %d.\n",
				__func__, params->slot_count);
		return -EINVAL;
	}

	if (256 < (params->slot_count * params->bit_width)){
		pr_err("%s: Invalid nbits per frame %d.\n",
				 __func__,
				params->slot_count * params->bit_width );
		return -EINVAL;
	}

	slot_mask = 0;

	for (i = 0; i < params->active_slot_count; ++i) {
		slot_mask |= (1 << params->tx_slots[i]);
	}

	if (slot_mask == 0) {
		pr_err("%s: Invalid active slot %d.\n",
			 __func__,
			params->active_slot_count);
		return -EINVAL;
	}

	config->slot_mask = slot_mask;
	config->bit_width = params->bit_width;
	config->slot_count = params->slot_count;
	config->slot_width = params->bit_width;
	config->sync_type = TDM_SHORT_SYNC_TYPE;
	config->ctrl_data_oe = TDM_CTRL_DATA_OE_ENABLE;
	config->invert_sync = TDM_LONG_SYNC_NORMAL;
	config->sync_delay = TDM_DATA_DELAY_0_CYCLE;

	return 0;
}

static void ipq_lpass_dma_config_init(struct lpass_dma_buffer *buffer)
{
	uint32_t wps_count = 0;
	struct lpass_dma_config dma_config;

	if (buffer->bit_width != 8)
		wps_count = (buffer->num_channels *
				buffer->bytes_per_sample) >> 2;
	if (0 == wps_count){
		wps_count = 1;
	}

	if (buffer->watermark != 1) {
		if (0 == (buffer->period_count_in_word32 &
				PCM_DMA_BUFFER_16BYTE_ALIGNMENT)){
			dma_config.burst_size = 16;
		} else if (0 == (buffer->period_count_in_word32 &
				PCM_DMA_BUFFER_8BYTE_ALIGNMENT)){
			dma_config.burst_size = 8;
		} else if (0 == (buffer->period_count_in_word32 &
				PCM_DMA_BUFFER_4BYTE_ALIGNMENT)){
			dma_config.burst_size = 4;
		} else {
			dma_config.burst_size = 1;
		}
	} else {
		dma_config.burst_size = 1;
	}

	dma_config.buffer_len = buffer->dma_buffer_size/sizeof(uint32_t);
	dma_config.buffer_start_addr = buffer->dma_base_address;
	dma_config.dma_int_per_cnt = buffer->period_count_in_word32;
	dma_config.wps_count = wps_count;
	dma_config.watermark = buffer->watermark;
	dma_config.ifconfig = buffer->ifconfig;
	dma_config.idx = buffer->idx;
	if (dma_config.burst_size >= 8) {
		dma_config.burst8_en = 1;
		dma_config.burst_size = 1;
	}
	dma_config.burst16_en = 0;
	dma_config.dir =  buffer->dir;
	dma_config.lpaif_base =  ipq_lpass_lpaif_base;

	ipq_lpass_disable_dma_channel(ipq_lpass_lpaif_base,
					buffer->idx, buffer->dir);

	ipq_lpass_config_dma_channel(&dma_config);

	ipq_lpass_enable_dma_channel(ipq_lpass_lpaif_base,
					buffer->idx, buffer->dir);
}

static void ipq_lpass_dma_enable(struct lpass_dma_buffer *buffer)
{
	ipq_lpass_dma_clear_interrupt_config(ipq_lpass_lpaif_base, buffer->dir,
						buffer->idx, buffer->intr_id);

	ipq_lpass_dma_enable_interrupt(ipq_lpass_lpaif_base, buffer->dir,
						buffer->idx, buffer->intr_id);
}

static uint32_t ipq_lpass_prepare_dma_buffer(uint32_t actual_buffer_size,
						uint32_t buffer_max_size)
{
	uint32_t circular_buff_size;
	uint32_t no_of_buff;

	no_of_buff = MAX_PCM_DMA_BUFFERS;
/* Bufer size is updated based on configuration*/
	circular_buff_size = no_of_buff * actual_buffer_size;

	while(circular_buff_size > buffer_max_size ||
		circular_buff_size & PCM_DMA_BUFFER_16BYTE_ALIGNMENT){
		no_of_buff >>= 1;
		circular_buff_size = no_of_buff * actual_buffer_size;
	}

	return circular_buff_size;
}

static void ipq_lpass_fill_tx_data(uint32_t *tx_buff, uint32_t size,
					struct ipq_lpass_pcm_params *params)
{
	uint i,slot = 0;

	slot = params->active_slot_count;
	for (i = 0; i < size / 4; ) {
		for (slot = 0; slot < params->active_slot_count; slot++) {
			if (params->bit_width == 16) {
				tx_buff[i] = params->tx_slots[slot] << 16;
			} else {
				tx_buff[i] = params->tx_slots[slot] << 8;
			}
			i++;
		}
	}
}

static void __iomem *ipq_lpass_phy_virt_lpm(uint32_t phy_addr)
{
	return (ipq_lpass_lpm_base + (phy_addr - IPQ_LPASS_LPM_BASE));
}

static int ipq_lpass_setup_bit_clock(uint32_t clk_rate)
{

/*
 * set clock rate for PRI & SEC
 * PRI is slave mode and seondary is master
 */
	ipq_lpass_lpaif_muxsetup(INTERFACE_PRIMARY, TDM_MODE_SLAVE,
				INVERT_INT_CLK, LPAIF_SLAVE_MODE_MUXSEL);

	if (ipq_lpass_set_clk_rate(INTERFACE_SECONDARY, clk_rate) != 0){
		pr_err("%s: Bit clk set Failed \n",
				__func__);
		return -EINVAL;
	} else {
		ipq_lpass_lpaif_muxsetup(INTERFACE_SECONDARY, TDM_MODE_MASTER,
				INVERT_INT_EXT_CLK, LPAIF_MASTER_MODE_MUXSEL);
	}

	return 0;

}

/*
 * FUNCTION: ipq_lpass_pcm_init
 *
 * DESCRIPTION: initializes PCM interface and MBOX interface
 *
 * RETURN VALUE: error if any
 */
int ipq_pcm_init(struct ipq_lpass_pcm_params *params)
{
	struct ipq_lpass_pcm_config config;
	int ret;
	uint32_t clk_rate;
	uint32_t temp_lpm_base;
	uint32_t int_samples_per_period;
	uint32_t bytes_per_sample;
	uint32_t samples_per_interrupt;
	uint32_t circular_buffer;
	uint32_t bytes_per_sample_intr;
	uint32_t dword_per_sample_intr;
	uint32_t no_of_buffers;
	uint32_t watermark = DEAFULT_PCM_WATERMARK;

	if ((rx_dma_buffer == NULL ) || (tx_dma_buffer == NULL))
		return -EPERM;

	ret = ipq_lpass_pcm_validate_params(params, &config);
	if (ret)
		return ret;

	atomic_set(&data_avail, 0);

	pcm_params = params;
	temp_lpm_base = IPQ_LPASS_LPM_BASE;

	int_samples_per_period = params->rate / 1000;
	bytes_per_sample = BYTES_PER_CHANNEL(params->bit_width);
	samples_per_interrupt = IPQ_LPASS_PCM_SAMPLES(int_samples_per_period,
					params->active_slot_count,
					bytes_per_sample);

	if (rx_dma_buffer->dma_memory_type == DMA_MEMORY_LPM ||
		tx_dma_buffer->dma_memory_type == DMA_MEMORY_LPM) {
		circular_buffer = ipq_lpass_prepare_dma_buffer(
					samples_per_interrupt,
					LPASS_DMA_BUFFER_SIZE);
	} else {
		circular_buffer = ipq_lpass_prepare_dma_buffer(
					samples_per_interrupt,
					rx_dma_buffer->max_size);
	}
	if (circular_buffer == 0) {
		pr_err("%s: Error at circular buffer calculation\n",
				__func__);
		return -ENOMEM;
	}

	no_of_buffers = circular_buffer / samples_per_interrupt;

	if (voice_loopback == 1)
		no_of_buffers = PCM_VOICE_LOOPBACK_BUFFER_SIZE /
					PCM_VOICE_LOOPBACK_INTR_SIZE;

	clk_rate = params->bit_width * params->rate * params->slot_count;
	ret = ipq_lpass_setup_bit_clock(clk_rate);
	if (ret)
		return ret;

	bytes_per_sample_intr = int_samples_per_period * bytes_per_sample *
					params->active_slot_count;

	dword_per_sample_intr = bytes_per_sample_intr >> 2;

	if((DEAFULT_PCM_WATERMARK >= dword_per_sample_intr) ||
		(dword_per_sample_intr & 0x3)  ){
		watermark = 1;
	}

/*
 * DMA Rx buffer
 */
	rx_dma_buffer->idx = DMA_CHANNEL0;
	rx_dma_buffer->dir = LPASS_HW_DMA_SOURCE;
	rx_dma_buffer->bytes_per_sample = bytes_per_sample;
	rx_dma_buffer->bit_width = params->bit_width;
	rx_dma_buffer->int_samples_per_period = int_samples_per_period;
	rx_dma_buffer->num_channels = params->active_slot_count;
	rx_dma_buffer->single_buf_size =
		(voice_loopback == 1)? PCM_VOICE_LOOPBACK_INTR_SIZE :
						samples_per_interrupt;
/*
 * set the period count in double words
 */
	rx_dma_buffer->period_count_in_word32 =
			rx_dma_buffer->single_buf_size / sizeof(uint32_t);
/*
 * total buffer size for all DMA buffers
 */
	rx_dma_buffer->dma_buffer_size =
		(voice_loopback == 1) ? PCM_VOICE_LOOPBACK_BUFFER_SIZE :
						circular_buffer;
	rx_dma_buffer->no_of_buffers = no_of_buffers;
	if (rx_dma_buffer->dma_memory_type == DMA_MEMORY_LPM) {
		rx_dma_buffer->dma_base_address = temp_lpm_base;
		rx_dma_buffer->dma_last_curr_addr = temp_lpm_base;
		rx_dma_buffer->dma_buffer = NULL;
	}
	rx_dma_buffer->watermark = watermark;
	rx_dma_buffer->ifconfig = INTERFACE_PRIMARY;
	rx_dma_buffer->intr_id = INTERRUPT_CHANNEL0;
	if (voice_loopback == 0)
		temp_lpm_base += LPASS_DMA_BUFFER_SIZE;
	atomic_set(&rx_add, 0);

/*
 * DMA Tx buffer
 */
	tx_dma_buffer->idx = DMA_CHANNEL1;
	tx_dma_buffer->dir = LPASS_HW_DMA_SINK;
	tx_dma_buffer->bytes_per_sample = bytes_per_sample;
	tx_dma_buffer->bit_width = params->bit_width;
	tx_dma_buffer->int_samples_per_period = int_samples_per_period;
	tx_dma_buffer->num_channels = params->active_slot_count;
	tx_dma_buffer->single_buf_size =
		(voice_loopback == 1)? PCM_VOICE_LOOPBACK_INTR_SIZE :
						samples_per_interrupt;
/*
 * set the period count in double words
 */
	tx_dma_buffer->period_count_in_word32 =
			tx_dma_buffer->single_buf_size / sizeof(uint32_t);
/*
 * total buffer size for all DMA buffers
 */
	tx_dma_buffer->dma_buffer_size =
		(voice_loopback == 1) ? PCM_VOICE_LOOPBACK_BUFFER_SIZE :
						circular_buffer;
	tx_dma_buffer->no_of_buffers = no_of_buffers;
	if (tx_dma_buffer->dma_memory_type == DMA_MEMORY_LPM) {
		tx_dma_buffer->dma_base_address = temp_lpm_base;
		tx_dma_buffer->dma_last_curr_addr = temp_lpm_base;
		tx_dma_buffer->dma_buffer = NULL;
	}
	tx_dma_buffer->watermark = watermark;
	tx_dma_buffer->ifconfig = INTERFACE_SECONDARY;
	tx_dma_buffer->intr_id = INTERRUPT_CHANNEL0;

	ipq_lpass_fill_tx_data(
		tx_dma_buffer->dma_memory_type == DMA_MEMORY_LPM ?
			(uint32_t *)ipq_lpass_phy_virt_lpm(
				tx_dma_buffer->dma_base_address) :
			(uint32_t *)tx_dma_buffer->dma_buffer,
			tx_dma_buffer->dma_buffer_size,
				params);
/*
 * TDM/PCM , Primary PCM support only RX mode
 * Secondary PCM support only TX mode
 */

	ipq_lpass_dma_config_init(rx_dma_buffer);
	ipq_lpass_dma_config_init(tx_dma_buffer);
	ipq_lpass_pcm_reset(ipq_lpass_lpaif_base,
				SECONDARY, LPASS_HW_DMA_SINK);
	ipq_lpass_pcm_reset(ipq_lpass_lpaif_base,
				PRIMARY, LPASS_HW_DMA_SOURCE);
/*
 * Tx mode support in Sconday interface.
 * configure secondary as TDM master mode
 */
	config.sync_src = TDM_MODE_MASTER;
	ipq_lpass_pcm_config(&config, ipq_lpass_lpaif_base,
				SECONDARY, LPASS_HW_DMA_SINK);
/*
 * Rx mode support in Primary interface
 * configure primary as TDM slave mode
 */
	config.sync_src = TDM_MODE_SLAVE;
	ipq_lpass_pcm_config(&config, ipq_lpass_lpaif_base,
				PRIMARY, LPASS_HW_DMA_SOURCE);

	ipq_lpass_pcm_reset_release(ipq_lpass_lpaif_base,
				SECONDARY, LPASS_HW_DMA_SINK);
	ipq_lpass_pcm_reset_release(ipq_lpass_lpaif_base,
				PRIMARY, LPASS_HW_DMA_SOURCE);

	ipq_lpass_dma_enable(rx_dma_buffer);
	ipq_lpass_dma_enable(tx_dma_buffer);

	ipq_lpass_pcm_enable(ipq_lpass_lpaif_base,
				PRIMARY, LPASS_HW_DMA_SOURCE);
	ipq_lpass_pcm_enable(ipq_lpass_lpaif_base,
				SECONDARY, LPASS_HW_DMA_SINK);

	return ret;
}
EXPORT_SYMBOL(ipq_pcm_init);

/*
 * FUNCTION: ipq_lpass_pcm_data
 *
 * DESCRIPTION: calculate the free tx buffer and full rx buffers for use by the
 *		upper layer
 *
 * RETURN VALUE: returns the rx and tx buffer pointers and the size to fill or
 *		read
 */
uint32_t ipq_pcm_data(uint8_t **rx_buf, uint8_t **tx_buf)
{
	unsigned long flag;
	uint32_t size;
	uint32_t buffer_index;
	uint32_t offset;
	uint32_t txcurr_addr;
	uint32_t rxcurr_addr;

	wait_event_interruptible(pcm_q, atomic_read(&data_avail) != 0);
	atomic_set(&data_avail, 0);
	buffer_index = atomic_read(&rx_add);

	offset = (rx_dma_buffer->dma_buffer_size /
			rx_dma_buffer->no_of_buffers) * buffer_index;

	spin_lock_irqsave(&pcm_lock, flag);
	if (rx_dma_buffer->dma_memory_type == DMA_MEMORY_LPM){
		rxcurr_addr = rx_dma_buffer->dma_base_address + offset;
		*rx_buf = (uint8_t *)ipq_lpass_phy_virt_lpm(rxcurr_addr);
	} else {
		*rx_buf = rx_dma_buffer->dma_buffer + offset;
	}
	if (tx_dma_buffer->dma_memory_type == DMA_MEMORY_LPM){
		txcurr_addr = tx_dma_buffer->dma_base_address + offset;
		*tx_buf = (uint8_t *)ipq_lpass_phy_virt_lpm(txcurr_addr);
	} else {
		*tx_buf = tx_dma_buffer->dma_buffer + offset;
	}
	size = rx_dma_buffer->single_buf_size;
	spin_unlock_irqrestore(&pcm_lock, flag);

	return size;
}
EXPORT_SYMBOL(ipq_pcm_data);

/*
 * FUNCTION: ipq_lpass_pcm_done
 *
 * DESCRIPTION: this api tells the PCM that the upper layer has finished
 *		updating the Tx buffer
 *
 * RETURN VALUE: none
 */
void ipq_pcm_done(void)
{
	atomic_set(&data_avail, 0);
}
EXPORT_SYMBOL(ipq_pcm_done);

void ipq_pcm_send_event(void)
{
	atomic_set(&data_avail, 1);
	wake_up_interruptible(&pcm_q);
}
EXPORT_SYMBOL(ipq_pcm_send_event);

static void ipq_lpass_pcm_dma_deinit(struct lpass_dma_buffer *buffer)
{
	ipq_lpass_dma_clear_interrupt_config(ipq_lpass_lpaif_base, buffer->dir,
						buffer->idx, buffer->intr_id);

	ipq_lpass_dma_disable_interrupt(ipq_lpass_lpaif_base, buffer->dir,
						buffer->idx, buffer->intr_id);

	ipq_lpass_disable_dma_channel(ipq_lpass_lpaif_base, buffer->idx,
						buffer->dir);

	ipq_lpass_pcm_disable(ipq_lpass_lpaif_base, buffer->ifconfig,
					buffer->dir);
}

static int ipq_lpass_allocate_dma_buffer(struct device *dev,
					struct lpass_dma_buffer *buffer)
{
	buffer->dma_buffer = dma_alloc_coherent(dev, buffer->max_size,
				&buffer->dma_addr, GFP_KERNEL);
	if (!buffer->dma_buffer)
		return -ENOMEM;
	else
		buffer->dma_base_address = buffer->dma_addr;

	return 0;
}

static void ipq_lpass_clear_dma_buffer( struct device *dev,
					struct lpass_dma_buffer *buffer)
{
	if (buffer->dma_buffer != NULL) {
		dma_free_coherent(dev, buffer->max_size,
			buffer->dma_buffer, buffer->dma_addr);
		buffer->dma_buffer = NULL;
	}
}

/*
 * FUNCTION: ipq_lpass_pcm_deinit
 *
 * DESCRIPTION: deinitialization api, clean up everything
 *
 * RETURN VALUE: none
 */
void ipq_pcm_deinit(struct ipq_lpass_pcm_params *params)
{
	ipq_lpass_pcm_dma_deinit(rx_dma_buffer);
	ipq_lpass_pcm_dma_deinit(tx_dma_buffer);
}
EXPORT_SYMBOL(ipq_pcm_deinit);

static const struct of_device_id qca_raw_match_table[] = {
	{ .compatible = "qca,ipq5018-lpass-pcm" },
	{},
};

/*
 * FUNCTION: ipq_lpass_pcm_driver_probe
 *
 * DESCRIPTION: very basic one time activities
 *
 * RETURN VALUE: error if any
 */
static int ipq_lpass_pcm_driver_probe(struct platform_device *pdev)
{
	uint32_t irq;
	uint32_t voice_lb = 0;
	struct resource *res;
	const struct of_device_id *match;
	uint32_t single_buf_size_max;
	uint32_t max_size;
	struct lpass_irq_buffer *irq_buffer;
	const char *playback_memory, *capture_memory;
	int ret;

	match = of_match_device(qca_raw_match_table, &pdev->dev);
	if (!match)
		return -ENODEV;
	if (!pdev)
		return -EINVAL;

	pcm_pdev = pdev;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	ipq_lpass_lpaif_base = devm_ioremap_resource(&pdev->dev, res);

	if (IS_ERR(ipq_lpass_lpaif_base))
		return PTR_ERR(ipq_lpass_lpaif_base);

	ipq_lpass_lpm_base = ioremap_nocache(IPQ_LPASS_LPM_BASE,
				IPQ_LPASS_LPM_SIZE);

	if (IS_ERR(ipq_lpass_lpm_base))
		return PTR_ERR(ipq_lpass_lpm_base);

	if (!pdev->dev.coherent_dma_mask)
		pdev->dev.coherent_dma_mask = DMA_BIT_MASK(32);

	pr_info("%s : Lpaif version : 0x%x\n",
			 __func__,readl(ipq_lpass_lpaif_base));

	/* Allocate memory for rx  and tx instance */
	rx_dma_buffer = kzalloc(sizeof(struct lpass_dma_buffer), GFP_KERNEL);
	if (rx_dma_buffer == NULL) {
		pr_err("%s: Error in allocating mem for rx_dma_buffer\n",
				__func__);
		return -ENOMEM;
	}

	tx_dma_buffer = kzalloc(sizeof(struct lpass_dma_buffer), GFP_KERNEL);
	if (tx_dma_buffer == NULL) {
		pr_err("%s: Error in allocating mem for tx_dma_buffer\n",
				__func__);
		kfree(rx_dma_buffer);
		return -ENOMEM;
	}

	irq_buffer = kzalloc(sizeof(struct lpass_irq_buffer), GFP_KERNEL);
	if (irq_buffer == NULL) {
		pr_err("%s: Error in allocating mem for irq_buffer\n",
				__func__);
		kfree(rx_dma_buffer);
		kfree(tx_dma_buffer);
		return -ENOMEM;
	}

	/* allocate DMA buffers of max size */
	single_buf_size_max = IPQ_PCM_SAMPLES_PER_PERIOD(
					IPQ_PCM_SAMPLING_RATE_MAX) *
					MAX_PCM_SAMPLES *
					IPQ_PCM_BYTES_PER_SAMPLE_MAX *
					IPQ_PCM_MAX_SLOTS;

	max_size = single_buf_size_max * MAX_PCM_DMA_BUFFERS;

	memset(rx_dma_buffer, 0, sizeof(*rx_dma_buffer));
	memset(tx_dma_buffer, 0, sizeof(*tx_dma_buffer));

	of_property_read_u32(pdev->dev.of_node, "voice_loopback", &voice_lb);
	voice_loopback = voice_lb;

	if (voice_loopback) {
	/* Setting LPM if voice_loopback enabled */
		tx_dma_buffer->dma_memory_type = DMA_MEMORY_LPM;
		rx_dma_buffer->dma_memory_type = DMA_MEMORY_LPM;
	} else {
		ret = of_property_read_string(pdev->dev.of_node,
					"capture_memory", &capture_memory);
		if (ret) {
			pr_err("lpass: couldn't read capture_memory: %d"
					" configure LPM as default\n", ret);
			rx_dma_buffer->dma_memory_type = DMA_MEMORY_LPM;
		} else {
			if (!strncmp(capture_memory, "lpm", 3)) {
				rx_dma_buffer->dma_memory_type = DMA_MEMORY_LPM;
			} else if (!strncmp(capture_memory, "ddr", 3)) {
				rx_dma_buffer->dma_memory_type = DMA_MEMORY_DDR;
			} else {
				rx_dma_buffer->dma_memory_type = DMA_MEMORY_LPM;
			}
		}

		ret = of_property_read_string(pdev->dev.of_node,
					"playback_memory", &playback_memory);
		if (ret) {
			pr_err("lpass: couldn't read playback_memory: %d"
					" configure LPM as default\n", ret);
			tx_dma_buffer->dma_memory_type = DMA_MEMORY_LPM;
		} else {
			if (!strncmp(playback_memory, "lpm", 3)) {
				tx_dma_buffer->dma_memory_type = DMA_MEMORY_LPM;
			} else if (!strncmp(playback_memory, "ddr", 3)) {
				tx_dma_buffer->dma_memory_type = DMA_MEMORY_DDR;
			} else {
				tx_dma_buffer->dma_memory_type = DMA_MEMORY_LPM;
			}
		}
	}

	if (rx_dma_buffer->dma_memory_type == DMA_MEMORY_DDR) {
		rx_dma_buffer->max_size = max_size;
		ret = ipq_lpass_allocate_dma_buffer(&pdev->dev, rx_dma_buffer);
		if (ret) {
			pr_err("lpass: rx: no enough memory"
				" use LPM instead : %d\n", ret);
			rx_dma_buffer->dma_memory_type = DMA_MEMORY_LPM;
		}
	} else {
		rx_dma_buffer->dma_buffer = NULL;
	}

	if (tx_dma_buffer->dma_memory_type == DMA_MEMORY_DDR) {
		tx_dma_buffer->max_size = max_size;
		ret = ipq_lpass_allocate_dma_buffer(&pdev->dev, tx_dma_buffer);
		if (ret) {
			pr_err("lpass: tx: no enough memory"
				" use LPM instead : %d\n", ret);
			tx_dma_buffer->dma_memory_type = DMA_MEMORY_LPM;
		}
	} else {
		tx_dma_buffer->dma_buffer = NULL;
	}

	irq_buffer->rx_buffer = rx_dma_buffer;
	irq_buffer->tx_buffer = tx_dma_buffer;
	platform_set_drvdata(pdev, irq_buffer);

/*
 * Set interrupt
 */
	irq = platform_get_irq_byname(pdev, "out0");
	if (irq < 0) {
		dev_err(&pdev->dev, "Failed to get irq by name (%d)\n",
				irq);
	} else {
		ret = devm_request_irq(&pdev->dev,
					irq,
					ipq_lpass_pcm_irq_handler,
					0,
					"ipq-lpass",
					irq_buffer);
		if (ret) {
			dev_err(&pdev->dev,
				"request_irq failed with ret: %d\n", ret);
		return ret;
		}
	}

	spin_lock_init(&pcm_lock);

	return 0;
}

/*
 * FUNCTION: ipq_lpass_pcm_driver_remove
 *
 * DESCRIPTION: clean up
 *
 * RETURN VALUE: error if any
 */
static int ipq_lpass_pcm_driver_remove(struct platform_device *pdev)
{
	struct lpass_irq_buffer *resource = platform_get_drvdata(pdev);

	ipq_pcm_deinit(pcm_params);

	if(rx_dma_buffer != NULL) {
		ipq_lpass_clear_dma_buffer(&pcm_pdev->dev, rx_dma_buffer);
	}
	if(tx_dma_buffer != NULL) {
		ipq_lpass_clear_dma_buffer(&pcm_pdev->dev, tx_dma_buffer);
	}

	if (rx_dma_buffer)
		kfree(rx_dma_buffer);

	if (tx_dma_buffer)
		kfree(tx_dma_buffer);

	if (resource)
		kfree(resource);

	return 0;
}

/*
 * DESCRIPTION OF PCM RAW MODULE
 */

#define DRIVER_NAME "ipq_lpass_pcm_raw"

static struct platform_driver ipq_lpass_pcm_raw_driver = {
	.probe		= ipq_lpass_pcm_driver_probe,
	.remove		= ipq_lpass_pcm_driver_remove,
	.driver		= {
		.name		= DRIVER_NAME,
		.of_match_table = qca_raw_match_table,
	},
};

module_platform_driver(ipq_lpass_pcm_raw_driver);

MODULE_ALIAS(DRIVER_NAME);
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("QCA RAW PCM VoIP Platform Driver");
