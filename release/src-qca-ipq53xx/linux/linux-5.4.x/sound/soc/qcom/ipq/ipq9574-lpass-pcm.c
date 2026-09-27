/*
 * Copyright (c) 2021 The Linux Foundation. All rights reserved.
 *
 * Copyright (c) 2021-2023 Qualcomm Innovation Center, Inc. All rights reserved.
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
#include <linux/of_irq.h>
#include <linux/of_address.h>

#include "ipq-lpass.h"
#include "ipq9574-lpass-pcm.h"

#define DEFAULT_CLK_RATE		2048000
#define PCM_VOICE_LOOPBACK_BUFFER_SIZE	0x1000
#define PCM_VOICE_LOOPBACK_INTR_SIZE	0x800
/*
 * Global Constant Definitions
 */
void __iomem *ipq_lpass_lpaif_base;

/*
 * Static Variable Definitions
 */
static struct platform_device *pcm_pdev;
static spinlock_t pcm_lock;
static uint32_t pcm_dual_instance = 0;
static struct ipq_lpass_pcm_global_config pcm_conf[IPQ_LPASS_MAX_PCM_INTERFACE];

static struct ipq_lpass_props ipq9574_lpass_pcm_cfg = {
	.npcm			=	IPQ_LPASS_MAX_PCM_INTERFACE,
	{
		{
			.sync_src               =       TDM_MODE_MASTER,
			.pcm_index              =       PRIMARY,
			.dir                    =       LPASS_HW_DMA_SINK,
			.invert_sync            =       TDM_LONG_SYNC_NORMAL,
			.sync_type              =       TDM_SHORT_SYNC_TYPE,
			.sync_delay             =       TDM_DATA_DELAY_0_CYCLE,
			.ctrl_data_oe           =       TDM_CTRL_DATA_OE_ENABLE,
		},
		{
			.sync_src               =       TDM_MODE_MASTER,
			.pcm_index              =       SECONDARY,
			.dir                    =       LPASS_HW_DMA_SINK,
			.invert_sync            =       TDM_LONG_SYNC_NORMAL,
			.sync_type              =       TDM_SHORT_SYNC_TYPE,
			.sync_delay             =       TDM_DATA_DELAY_0_CYCLE,
			.ctrl_data_oe           =       TDM_CTRL_DATA_OE_ENABLE,
		},
	},
	{
		{
			.tx_idx                    =       DMA_CHANNEL0,
			.tx_dir                    =       LPASS_HW_DMA_SINK,
			.tx_intr_id                =       INTERRUPT_CHANNEL0,
			.tx_ifconfig               =       INTERFACE_PRIMARY,

			.rx_idx                    =       DMA_CHANNEL0,
			.rx_dir                    =       LPASS_HW_DMA_SOURCE,
			.rx_intr_id                =       INTERRUPT_CHANNEL0,
			.rx_ifconfig               =       INTERFACE_PRIMARY,
			.read_intr_status          =       LPASS_DMA_CH0_STATUS,
		},
		{
			.tx_idx                    =       DMA_CHANNEL1,
			.tx_dir                    =       LPASS_HW_DMA_SINK,
			.tx_intr_id                =       INTERRUPT_CHANNEL1,
			.tx_ifconfig               =       INTERFACE_SECONDARY,

			.rx_idx                    =       DMA_CHANNEL1,
			.rx_dir                    =       LPASS_HW_DMA_SOURCE,
			.rx_intr_id                =       INTERRUPT_CHANNEL1,
			.rx_ifconfig               =       INTERFACE_SECONDARY,
			.read_intr_status          =       LPASS_DMA_CH1_STATUS,
		},
	},
};

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
	uint32_t intr_id = pcm_conf[buffer->pcm_index].rx_dma_buffer->intr_id;

	ipq_lpass_dma_read_interrupt_status(ipq_lpass_lpaif_base,
						intr_id, &status);
	while (status) {
/*
 * Both Tx and Rx interrupt use same IRQ number
 * so Calculating RX buffer pointer is sufficient
 * since RX and Tx configure same buffer size
 * and interrupt size
 */
		if (status & pcm_conf[buffer->pcm_index].dma_read_intr_status) {
			ipq_lpass_dma_get_curr_addr(ipq_lpass_lpaif_base,
					rx_buffer->idx,
					rx_buffer->dir,
					&curr_addr);

			buffer_idx = ipq_lpass_pcm_get_dataptr(rx_buffer,
					curr_addr);

			atomic_set(&pcm_conf[buffer->pcm_index].rx_add,
								buffer_idx);
			atomic_set(&pcm_conf[buffer->pcm_index].data_avail, 1);
			wake_up_interruptible(&pcm_conf[buffer->pcm_index].pcm_q);
		}

		ipq_lpass_dma_clear_interrupt(ipq_lpass_lpaif_base,
				intr_id, status);
		status = 0;

		ipq_lpass_dma_read_interrupt_status(ipq_lpass_lpaif_base,
				intr_id, &status);
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
			IPQ_PCM_MAX_SLOTS_PER_FRAME) {
		pr_err("%s: Invalid nslots per frame %d.\n",
				__func__, params->slot_count);
		return -EINVAL;
	}

	if (256 < (params->slot_count * params->bit_width)) {
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

	return 0;
}

static void ipq_lpass_dma_config_init(struct lpass_dma_buffer *buffer)
{
	uint32_t wps_count = 0;
	struct lpass_dma_config dma_config;

	if (buffer->bit_width != 8)
		wps_count = (buffer->num_channels *
				buffer->bytes_per_sample) >> 2;
	if (0 == wps_count) {
		wps_count = 1;
	}

	if (buffer->watermark != 1) {
		if (0 == (buffer->period_count_in_word32 &
				PCM_DMA_BUFFER_16BYTE_ALIGNMENT)) {
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
	dma_config.burst8_en = 0;
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

static void __iomem *ipq_lpass_phy_virt_lpm(uint32_t phy_addr,
				uint32_t lpm_base, void __iomem *base_addr)
{
	return (base_addr + (phy_addr - lpm_base));
}


static int ipq9574_lpass_setup_bit_clock(uint32_t clk_rate,
				struct ipq_lpass_pcm_config config)
{
/*
* set clock rate for PCM Interface
* Enable by default pcm interfaces as master mode
*
*/
	uint32_t intf;

	if (config.pcm_index == PRIMARY)
		intf = INTERFACE_PRIMARY;
	else
		intf = INTERFACE_SECONDARY;

	if (config.sync_src == TDM_MODE_MASTER) {
		ipq_lpass_lpaif_muxsetup(intf, TDM_MODE_MASTER,
				NO_INVERSION, LPAIF_MASTER_MODE_MUXSEL);

		if (ipq_lpass_set_clk_rate(intf, clk_rate) != 0) {
			pr_err("%s: Bit clk set Failed \n", __func__);
			return -EINVAL;
		}
	} else {
		ipq_lpass_lpaif_muxsetup(intf, TDM_MODE_SLAVE,
				NO_INVERSION, LPAIF_SLAVE_MODE_MUXSEL);
	}
	return 0;
}

static void ipq9574_lpass_pcm_update_config( struct ipq_lpass_props *data,
			struct ipq_lpass_pcm_config *config, int pcm_index)
{
	struct ipq_lpass_pcm_tdm_config *pcm_config;
	struct ipq_lpass_wr_rd_dma_config *dma_config;

	pcm_config = &data->pcm_config[pcm_index];
	if (pcm_conf[pcm_index].slave == 1)
		 pcm_config->sync_src = TDM_MODE_SLAVE;
	config->sync_src = pcm_config->sync_src;
	config->pcm_index = pcm_config->pcm_index;
	config->dir = pcm_config->dir;
	config->invert_sync = pcm_config->invert_sync;
	config->sync_type = pcm_config->sync_type;
	config->sync_delay = pcm_config->sync_delay;
	config->ctrl_data_oe = pcm_config->ctrl_data_oe;

	dma_config = &data->dma_config[pcm_index];
	pcm_conf[pcm_index].rx_dma_buffer->idx = dma_config->rx_idx;
	pcm_conf[pcm_index].rx_dma_buffer->dir = dma_config->rx_dir;
	pcm_conf[pcm_index].rx_dma_buffer->ifconfig = dma_config->rx_ifconfig;
	pcm_conf[pcm_index].rx_dma_buffer->intr_id = dma_config->rx_intr_id;
	pcm_conf[pcm_index].rx_dma_buffer->watermark = DEAFULT_PCM_WATERMARK;
	pcm_conf[pcm_index].dma_read_intr_status = dma_config->read_intr_status;

	pcm_conf[pcm_index].tx_dma_buffer->idx = dma_config->tx_idx;
	pcm_conf[pcm_index].tx_dma_buffer->dir = dma_config->tx_dir;
	pcm_conf[pcm_index].tx_dma_buffer->ifconfig = dma_config->tx_ifconfig;
	pcm_conf[pcm_index].tx_dma_buffer->intr_id = dma_config->tx_intr_id;
	pcm_conf[pcm_index].tx_dma_buffer->watermark = DEAFULT_PCM_WATERMARK;

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
	uint32_t clk_rate;
	uint32_t temp_lpm_base;
	uint32_t int_samples_per_period;
	uint32_t bytes_per_sample;
	uint32_t samples_per_interrupt;
	uint32_t circular_buffer;
	uint32_t bytes_per_sample_intr;
	uint32_t dword_per_sample_intr;
	uint32_t no_of_buffers;
	int pcm_index = params->pcm_index;
	int ret;

	if ((pcm_conf[pcm_index].rx_dma_buffer == NULL ) ||
			(pcm_conf[pcm_index].tx_dma_buffer == NULL)) {
		pr_debug("Error : Dma Buffer not allocated \n");
		return -EPERM;
	}

	ret = ipq_lpass_pcm_validate_params(params, &config);
	if (ret) {
		pr_debug("Error: Failed to validate pcm parameters\n");
		return ret;
	}


	atomic_set(&pcm_conf[pcm_index].data_avail, 0);
	temp_lpm_base = pcm_conf[pcm_index].lpass_lpm_base_phy_addr;

	pcm_conf[pcm_index].pcm_params = params;

	int_samples_per_period = params->rate / 1000;
	bytes_per_sample = BYTES_PER_CHANNEL(params->bit_width);
	samples_per_interrupt = IPQ_LPASS_PCM_SAMPLES(int_samples_per_period,
					params->active_slot_count,
					bytes_per_sample);

	if (pcm_conf[pcm_index].rx_dma_buffer->dma_memory_type == DMA_MEMORY_LPM
		|| pcm_conf[pcm_index].tx_dma_buffer->dma_memory_type ==
						DMA_MEMORY_LPM) {
		circular_buffer = ipq_lpass_prepare_dma_buffer(
			samples_per_interrupt, (pcm_dual_instance) == 1 ?
			LPASS_DMA_BUFFER_DUAL_PCM_SIZE  : LPASS_DMA_BUFFER_SIZE);

	} else {
		circular_buffer = ipq_lpass_prepare_dma_buffer(
				samples_per_interrupt,
				pcm_conf[pcm_index].rx_dma_buffer->max_size);
	}
	if (circular_buffer == 0) {
		pr_err("%s: Error at circular buffer calculation\n",
				__func__);
		return -ENOMEM;
	}

	no_of_buffers = circular_buffer / (samples_per_interrupt );

	if (pcm_conf[pcm_index].voice_loopback == 1)
		no_of_buffers = PCM_VOICE_LOOPBACK_BUFFER_SIZE /
					PCM_VOICE_LOOPBACK_INTR_SIZE;

	clk_rate = params->bit_width * params->rate * params->slot_count;

	bytes_per_sample_intr = int_samples_per_period * bytes_per_sample *
					params->active_slot_count;

	dword_per_sample_intr = bytes_per_sample_intr >> 2;

	ipq9574_lpass_pcm_update_config(&ipq9574_lpass_pcm_cfg, &config,
								pcm_index);

	ret = ipq9574_lpass_setup_bit_clock(clk_rate, config);
	if (ret)
		return ret;
/*
 * DMA Rx buffer
 */
	pcm_conf[pcm_index].rx_dma_buffer->bytes_per_sample = bytes_per_sample;
	pcm_conf[pcm_index].rx_dma_buffer->bit_width = params->bit_width;
	pcm_conf[pcm_index].rx_dma_buffer->int_samples_per_period =
						int_samples_per_period;
	pcm_conf[pcm_index].rx_dma_buffer->num_channels =
						params->active_slot_count;
	pcm_conf[pcm_index].rx_dma_buffer->single_buf_size =
		(pcm_conf[pcm_index].voice_loopback == 1)?
			PCM_VOICE_LOOPBACK_INTR_SIZE :samples_per_interrupt;
/*
 * set the period count in double words
 */
	pcm_conf[pcm_index].rx_dma_buffer->period_count_in_word32 =
		pcm_conf[pcm_index].rx_dma_buffer->single_buf_size /
							sizeof(uint32_t);
/*
 * total buffer size for all DMA buffers
 */
	pcm_conf[pcm_index].rx_dma_buffer->dma_buffer_size =
		(pcm_conf[pcm_index].voice_loopback == 1) ?
			PCM_VOICE_LOOPBACK_BUFFER_SIZE : circular_buffer;
	pcm_conf[pcm_index].rx_dma_buffer->no_of_buffers = no_of_buffers;
	if (pcm_conf[pcm_index].rx_dma_buffer->dma_memory_type ==
					DMA_MEMORY_LPM) {
		pcm_conf[pcm_index].rx_dma_buffer->dma_base_address =
						temp_lpm_base;
		pcm_conf[pcm_index].rx_dma_buffer->dma_last_curr_addr =
						temp_lpm_base;
		pcm_conf[pcm_index].rx_dma_buffer->dma_buffer = NULL;
	}
	if (pcm_conf[pcm_index].voice_loopback == 0)
		temp_lpm_base += (pcm_dual_instance == 1 ) ?
			LPASS_DMA_BUFFER_DUAL_PCM_SIZE : LPASS_DMA_BUFFER_SIZE;
	atomic_set(&pcm_conf[pcm_index].rx_add, 0);

/*
 * DMA Tx buffer
 */
	pcm_conf[pcm_index].tx_dma_buffer->bytes_per_sample = bytes_per_sample;
	pcm_conf[pcm_index].tx_dma_buffer->bit_width = params->bit_width;
	pcm_conf[pcm_index].tx_dma_buffer->int_samples_per_period =
					int_samples_per_period;
	pcm_conf[pcm_index].tx_dma_buffer->num_channels =
					params->active_slot_count;
	pcm_conf[pcm_index].tx_dma_buffer->single_buf_size =
		(pcm_conf[pcm_index].voice_loopback == 1)?
			PCM_VOICE_LOOPBACK_INTR_SIZE : samples_per_interrupt;
/*
 * set the period count in double words
 */
	pcm_conf[pcm_index].tx_dma_buffer->period_count_in_word32 =
			pcm_conf[pcm_index].tx_dma_buffer->single_buf_size
							/ sizeof(uint32_t);
/*
 * total buffer size for all DMA buffers
 */
	pcm_conf[pcm_index].tx_dma_buffer->dma_buffer_size =
		(pcm_conf[pcm_index].voice_loopback == 1) ?
			PCM_VOICE_LOOPBACK_BUFFER_SIZE : circular_buffer;
	pcm_conf[pcm_index].tx_dma_buffer->no_of_buffers = no_of_buffers;
	if (pcm_conf[pcm_index].tx_dma_buffer->dma_memory_type
					== DMA_MEMORY_LPM) {
		pcm_conf[pcm_index].tx_dma_buffer->dma_base_address =
						temp_lpm_base;
		pcm_conf[pcm_index].tx_dma_buffer->dma_last_curr_addr =
						temp_lpm_base;
		pcm_conf[pcm_index].tx_dma_buffer->dma_buffer = NULL;
	}

/*
 * TDM/PCM , Primary PCM support only RX mode
 * Secondary PCM support only TX mode
 */

	ipq_lpass_dma_config_init(pcm_conf[pcm_index].rx_dma_buffer);
	ipq_lpass_dma_config_init(pcm_conf[pcm_index].tx_dma_buffer);
	ipq_lpass_pcm_reset(ipq_lpass_lpaif_base,
				config.pcm_index, LPASS_HW_DMA_SINK);
	ipq_lpass_pcm_reset(ipq_lpass_lpaif_base,
				config.pcm_index, LPASS_HW_DMA_SOURCE);
/*
 * Tx mode support in Sconday interface.
 * configure secondary as TDM master mode
 */
	ipq_lpass_pcm_config(&config, ipq_lpass_lpaif_base,
				config.pcm_index, LPASS_HW_DMA_SINK);
/*
 * Rx mode support in Primary interface
 * configure primary as TDM slave mode
 */
	ipq_lpass_pcm_config(&config, ipq_lpass_lpaif_base,
				config.pcm_index, LPASS_HW_DMA_SOURCE);

	ipq_lpass_pcm_reset_release(ipq_lpass_lpaif_base,
				config.pcm_index, LPASS_HW_DMA_SINK);
	ipq_lpass_pcm_reset_release(ipq_lpass_lpaif_base,
				config.pcm_index, LPASS_HW_DMA_SOURCE);

	ipq_lpass_dma_enable(pcm_conf[pcm_index].rx_dma_buffer);
	ipq_lpass_dma_enable(pcm_conf[pcm_index].tx_dma_buffer);

	ipq_lpass_pcm_enable(ipq_lpass_lpaif_base,
				config.pcm_index, LPASS_HW_DMA_SOURCE);
	ipq_lpass_pcm_enable(ipq_lpass_lpaif_base,
				config.pcm_index, LPASS_HW_DMA_SINK);

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
uint32_t ipq_pcm_data(uint8_t **rx_buf, uint8_t **tx_buf, int pcm_index)
{
	unsigned long flag;
	uint32_t size;
	uint32_t buffer_index;
	uint32_t offset;
	uint32_t txcurr_addr;
	uint32_t rxcurr_addr;
	uint32_t lpm_base;

	wait_event_interruptible(pcm_conf[pcm_index].pcm_q,
			atomic_read(&pcm_conf[pcm_index].data_avail) != 0);
	lpm_base = pcm_conf[pcm_index].lpass_lpm_base_phy_addr;

	atomic_set(&pcm_conf[pcm_index].data_avail, 0);
	buffer_index = atomic_read(&pcm_conf[pcm_index].rx_add);

	offset = (pcm_conf[pcm_index].rx_dma_buffer->dma_buffer_size /
		pcm_conf[pcm_index].rx_dma_buffer->no_of_buffers) * buffer_index;

	spin_lock_irqsave(&pcm_lock, flag);
	if (pcm_conf[pcm_index].rx_dma_buffer->dma_memory_type ==
						DMA_MEMORY_LPM) {
		rxcurr_addr = pcm_conf[pcm_index].rx_dma_buffer->dma_base_address
								+ offset;
		*rx_buf = (uint8_t *)ipq_lpass_phy_virt_lpm(rxcurr_addr, lpm_base,
				pcm_conf[pcm_index].ipq_lpass_lpm_base);
	} else {
		*rx_buf = pcm_conf[pcm_index].rx_dma_buffer->dma_buffer + offset;
	}

	if (pcm_conf[pcm_index].tx_dma_buffer->dma_memory_type ==
						DMA_MEMORY_LPM) {
		txcurr_addr = pcm_conf[pcm_index].tx_dma_buffer->dma_base_address
								+ offset;
		*tx_buf = (uint8_t *)ipq_lpass_phy_virt_lpm(txcurr_addr, lpm_base,
				pcm_conf[pcm_index].ipq_lpass_lpm_base);
	} else {
		*tx_buf = pcm_conf[pcm_index].tx_dma_buffer->dma_buffer + offset;
	}

	size = pcm_conf[pcm_index].rx_dma_buffer->single_buf_size;
	spin_unlock_irqrestore(&pcm_lock, flag);

	return size;
}
EXPORT_SYMBOL(ipq_pcm_data);

/*
 * FUNCTION: ipq_pcm_done
 *
 * DESCRIPTION: this api tells the PCM that the upper layer has finished
 *		updating the Tx buffer
 *
 * RETURN VALUE: none
 */
void ipq_pcm_done(int pcm_index)
{
	atomic_set(&pcm_conf[pcm_index].data_avail, 0);
}
EXPORT_SYMBOL(ipq_pcm_done);

void ipq_pcm_send_event(int pcm_index)
{
	atomic_set(&pcm_conf[pcm_index].data_avail, 1);
	if (pcm_index == PRIMARY)
		wake_up_interruptible(&pcm_conf[pcm_index].pcm_q);
	else
		wake_up_interruptible(&pcm_conf[pcm_index].pcm_q);

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
	ipq_lpass_pcm_dma_deinit(pcm_conf[params->pcm_index].rx_dma_buffer);
	ipq_lpass_pcm_dma_deinit(pcm_conf[params->pcm_index].tx_dma_buffer);
}
EXPORT_SYMBOL(ipq_pcm_deinit);

static const struct of_device_id qca_raw_match_table[] = {
	{ .compatible = "qca,ipq9574-lpass-pcm", .data = &ipq9574_lpass_pcm_cfg },
	{ .compatible = "qca,ipq5332-lpass-pcm", .data = &ipq9574_lpass_pcm_cfg },
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
	uint32_t voice_lb = 0, slave_lb = 0;
	struct resource *res;
	const struct of_device_id *match;
	struct device_node *node;
	uint32_t single_buf_size_max;
	uint32_t max_size;
	struct lpass_irq_buffer *irq_buffer;
	struct device_node *np;
	struct pinctrl *pintctrl;
	struct pinctrl_state *pin_state;
	const char *playback_memory, *capture_memory;
	const char *pcm0_status = NULL;
	const char *pcm1_status = NULL;
	char irq_name[5];
	int pcm_index = 0;
	int ret;

	match = of_match_device(qca_raw_match_table, &pdev->dev);
	if (!match)
		return -ENODEV;
	if (!pdev)
		return -EINVAL;

	pcm_pdev = pdev;

	pintctrl = devm_pinctrl_get(&pdev->dev);

	np = of_find_node_by_name(NULL, "pcm0");
	if (np) {
		of_property_read_string(np, "status", &pcm0_status);
	}
	np = of_find_node_by_name(NULL, "pcm1");
	if (np) {
		of_property_read_string(np, "status", &pcm1_status);
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (res) {
		ipq_lpass_lpaif_base =ioremap(res->start, resource_size(res));

		if (!ipq_lpass_lpaif_base)
		{
			pr_err("%s: Failed to ioremap Lapif Base Address\n",
					__func__);
			return PTR_ERR(ipq_lpass_lpaif_base);
		}
		pr_info("%s : Lpaif version : 0x%x\n",
				__func__,readl(ipq_lpass_lpaif_base));
	}

	if ((pcm0_status != NULL && pcm1_status != NULL) &&
			!strncmp(pcm0_status, "ok", 2) && !strncmp(pcm1_status, "ok", 2)) {
		pcm_conf[PRIMARY].lpass_lpm_base_phy_addr =
						IPQ_LPASS_LPM_PCM0_BASE;
		pcm_conf[PRIMARY].ipq_lpass_lpm_base =
			ioremap_nocache(IPQ_LPASS_LPM_PCM0_BASE,
					IPQ_LPASS_LPM_PCM0_SIZE);
		pcm_conf[SECONDARY].lpass_lpm_base_phy_addr =
						IPQ_LPASS_LPM_PCM1_BASE;
		pcm_conf[SECONDARY].ipq_lpass_lpm_base =
			ioremap_nocache(IPQ_LPASS_LPM_PCM1_BASE,
				IPQ_LPASS_LPM_PCM1_SIZE);
		pcm_dual_instance = 1;
	}

	if (!pdev->dev.coherent_dma_mask)
		pdev->dev.coherent_dma_mask = DMA_BIT_MASK(32);

	/* allocate DMA buffers of max size */
	single_buf_size_max = IPQ_PCM_SAMPLES_PER_PERIOD(
					IPQ_PCM_SAMPLING_RATE_MAX) *
					MAX_PCM_SAMPLES *
					IPQ_PCM_BYTES_PER_SAMPLE_MAX *
					IPQ_PCM_MAX_SLOTS;

	max_size = single_buf_size_max * MAX_PCM_DMA_BUFFERS;

	for_each_available_child_of_node(pdev->dev.of_node, node) {

		pcm_index = node->name[strlen(node->name)-1] - '0';
		if (!(pcm_index >= 0 && pcm_index < IPQ_LPASS_MAX_PCM_INTERFACE)) {
			pr_err("%s : Failed to extract a pcm index \n", __func__);
			return -EINVAL;
		}

		if (pcm_dual_instance == 0) {
			pcm_conf[pcm_index].ipq_lpass_lpm_base =
			ioremap_nocache(IPQ_LPASS_LPM_BASE, IPQ_LPASS_LPM_SIZE);
			pcm_conf[pcm_index].lpass_lpm_base_phy_addr =
							IPQ_LPASS_LPM_BASE;
		}
		if (pcm_index == 0) {
			pin_state = pinctrl_lookup_state(pintctrl, "primary");

			if (IS_ERR(pin_state)) {
				pr_err("audio pinctrl state not available\n");
				return PTR_ERR(pin_state);
			}
			pinctrl_select_state(pintctrl, pin_state);
		}
		else if (pcm_index == 1) {
			pin_state = pinctrl_lookup_state(pintctrl, "secondary");

			if (IS_ERR(pin_state)) {
				pr_err("audio pinctrl state not available\n");
				return PTR_ERR(pin_state);
			}
			pinctrl_select_state(pintctrl, pin_state);
		}

		snprintf(irq_name, 5, "out%d", pcm_index);
		init_waitqueue_head(&pcm_conf[pcm_index].pcm_q);

		of_property_read_u32(node, "voice_loopback", &voice_lb);
		pcm_conf[pcm_index].voice_loopback = voice_lb;

		of_property_read_u32(node, "slave", &slave_lb);
		pcm_conf[pcm_index].slave = slave_lb;

		if(pcm_conf[pcm_index].slave)
			pr_info("PCM%d : Slave\n", pcm_index);
		else
			pr_info("PCM%d : Master\n", pcm_index);

		pcm_conf[pcm_index].rx_dma_buffer =
			kzalloc(sizeof(struct lpass_dma_buffer), GFP_KERNEL);
		if (pcm_conf[pcm_index].rx_dma_buffer == NULL) {
			pr_err("%s: Error in allocating mem for rx_dma_buffer\n",
					__func__);
			return -ENOMEM;
		}
		pcm_conf[pcm_index].tx_dma_buffer =
			kzalloc(sizeof(struct lpass_dma_buffer), GFP_KERNEL);
		if ( pcm_conf[pcm_index].tx_dma_buffer == NULL) {
			pr_err("%s: Error in allocating mem for tx_dma_buffer\n",
					__func__);
			return -ENOMEM;
		}

		irq_buffer = kzalloc(sizeof(struct lpass_irq_buffer), GFP_KERNEL);
		if (irq_buffer == NULL) {
			pr_err("%s: Error in allocating mem for irq_buffer\n",
					__func__);
			kfree(pcm_conf[pcm_index].rx_dma_buffer);
			kfree(pcm_conf[pcm_index].tx_dma_buffer);
			return -ENOMEM;
		}

		memset(pcm_conf[pcm_index].rx_dma_buffer, 0,
				sizeof(*pcm_conf[pcm_index].rx_dma_buffer));
		memset(pcm_conf[pcm_index].tx_dma_buffer, 0,
				sizeof(*pcm_conf[pcm_index].tx_dma_buffer));

		if (pcm_conf[pcm_index].voice_loopback) {
			pcm_conf[pcm_index].tx_dma_buffer->dma_memory_type =
							DMA_MEMORY_LPM;
			pcm_conf[pcm_index].rx_dma_buffer->dma_memory_type =
							DMA_MEMORY_LPM;
		} else {
			ret = of_property_read_string(node,
					"capture_memory", &capture_memory);
			if (ret) {
				pr_err("lpass: couldn't read capture_memory: %d"
					" configure LPM as default\n", ret);
				pcm_conf[pcm_index].rx_dma_buffer->dma_memory_type
							= DMA_MEMORY_LPM;
			} else {
				if (!strncmp(capture_memory, "lpm", 3)) {
					pcm_conf[pcm_index].rx_dma_buffer->
						dma_memory_type = DMA_MEMORY_LPM;
				} else if (!strncmp(capture_memory, "ddr", 3)) {
					pcm_conf[pcm_index].rx_dma_buffer->
						dma_memory_type = DMA_MEMORY_DDR;
				} else {
					pcm_conf[pcm_index].rx_dma_buffer->
						dma_memory_type = DMA_MEMORY_LPM;
				}
			}

			ret = of_property_read_string(node,
					"playback_memory", &playback_memory);
			if (ret) {
				pr_err("lpass: couldn't read playback_memory: %d"
						" configure LPM as default\n", ret);
				pcm_conf[pcm_index].tx_dma_buffer->dma_memory_type
								= DMA_MEMORY_LPM;
			} else {
				if (!strncmp(playback_memory, "lpm", 3)) {
					pcm_conf[pcm_index].tx_dma_buffer->
						dma_memory_type = DMA_MEMORY_LPM;
				} else if (!strncmp(playback_memory, "ddr", 3)) {
					pcm_conf[pcm_index].tx_dma_buffer->
						dma_memory_type = DMA_MEMORY_DDR;
				} else {
					pcm_conf[pcm_index].tx_dma_buffer->
						dma_memory_type = DMA_MEMORY_LPM;
				}
			}
		}

		if (pcm_conf[pcm_index].rx_dma_buffer->dma_memory_type ==
							DMA_MEMORY_DDR) {
			pcm_conf[pcm_index]. rx_dma_buffer->max_size = max_size;
			ret = ipq_lpass_allocate_dma_buffer(&pdev->dev,
					pcm_conf[pcm_index].rx_dma_buffer);
			if (ret) {
				pr_err("lpass: rx: no enough memory"
						" use LPM instead : %d\n", ret);
				pcm_conf[pcm_index].rx_dma_buffer->
						dma_memory_type = DMA_MEMORY_LPM;
			}
		} else {
			pcm_conf[pcm_index].rx_dma_buffer->dma_buffer = NULL;
		}

		if (pcm_conf[pcm_index].tx_dma_buffer->dma_memory_type ==
						DMA_MEMORY_DDR) {
			pcm_conf[pcm_index].tx_dma_buffer->max_size = max_size;
			ret = ipq_lpass_allocate_dma_buffer(&pdev->dev,
					pcm_conf[pcm_index].tx_dma_buffer);
			if (ret) {
				pr_err("lpass: tx: no enough memory"
						" use LPM instead : %d\n", ret);
				pcm_conf[pcm_index].tx_dma_buffer->
						dma_memory_type = DMA_MEMORY_LPM;
			}
		} else {
			pcm_conf[pcm_index].tx_dma_buffer->dma_buffer = NULL;
		}
		irq_buffer->pcm_index = pcm_index;

		irq_buffer->rx_buffer = pcm_conf[pcm_index].rx_dma_buffer;
		irq_buffer->tx_buffer = pcm_conf[pcm_index].tx_dma_buffer;
		platform_set_drvdata(pdev, irq_buffer);
		irq = of_irq_get_byname(node, irq_name);
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
	int index = 0;
	struct lpass_irq_buffer *resource = platform_get_drvdata(pdev);

	for (index = 0; index < IPQ_LPASS_MAX_PCM_INTERFACE; index++) {

		ipq_pcm_deinit(pcm_conf[index].pcm_params);
		if (pcm_conf[index].rx_dma_buffer != NULL) {
			ipq_lpass_clear_dma_buffer(&pcm_pdev->dev,
					pcm_conf[index].rx_dma_buffer);
		}
		if (pcm_conf[index].tx_dma_buffer != NULL) {
			ipq_lpass_clear_dma_buffer(&pcm_pdev->dev,
					pcm_conf[index].tx_dma_buffer);
		}

		if (pcm_conf[index].rx_dma_buffer)
			kfree(pcm_conf[index].rx_dma_buffer);

		if (pcm_conf[index].tx_dma_buffer)
			kfree(pcm_conf[index].tx_dma_buffer);
	}
	if (resource)
		kfree(resource);

	return 0;
}

/*
 * DESCRIPTION OF PCM RAW MODULE
 */

#define DRIVER_NAME "ipq9574_lpass_pcm_raw"

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
MODULE_DESCRIPTION("QTI RAW PCM VoIP Platform Driver");
