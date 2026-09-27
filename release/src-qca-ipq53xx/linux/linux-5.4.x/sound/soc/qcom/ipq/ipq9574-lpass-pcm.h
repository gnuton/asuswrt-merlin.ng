/*
 * Copyright (c) 2021 The Linux Foundation. All rights reserved.
 *
 * Copyright (c) 2021-2022 Qualcomm Innovation Center, Inc. All rights reserved.
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

#ifndef _IPQ_LPASS_TDM_PCM_H
#define _IPQ_LPASS_TDM_PCM_H

#define IPQ_LPASS_LPM_BASE			0x0A250000
#define IPQ_LPASS_LPM_PCM0_BASE			0x0A250000
#define IPQ_LPASS_LPM_PCM1_BASE			0x0A252000
#define IPQ_LPASS_LPM_PCM0_SIZE			0x2000
#define IPQ_LPASS_LPM_PCM1_SIZE			0x4000
#define IPQ_LPASS_LPM_SIZE			0x4000
#define IPQ_LPASS_LPM_START			IPQ_LPASS_LPM_BASE
#define IPQ_LPASS_LPM_END			(IPQ_LPASS_LPM_BASE + \
							IPQ_LPASS_LPM_SIZE)

#define LPASS_DMA_BUFFER_DUAL_PCM_SIZE		0x1000
#define LPASS_DMA_BUFFER_SIZE			0x2000
#define LPASS_BUFFER_SIZE			0x800
#define LPASS_DMA_CH0_STATUS			0x8000
#define LPASS_DMA_CH1_STATUS			0x40000
#define IPQ_LPASS_MAX_PCM_INTERFACE		2

#define LOOPBACK_SKIP_COUNT			32

#define MAX_PCM_DMA_BUFFERS			16
#define MAX_PCM_SAMPLES				10
#define PCM_DMA_BUFFER_16BYTE_ALIGNMENT		0xF
#define PCM_DMA_BUFFER_8BYTE_ALIGNMENT		7
#define PCM_DMA_BUFFER_4BYTE_ALIGNMENT		3
#define TDM_DMA_BUFFER_OFFSET_ALIGNMENT		0x1F
#define IPQ_PCM_MAX_SLOTS			32
#define DEAFULT_PCM_WATERMARK			8
#define IPQ_LPASS_MAX_LPM_BLK			4
#define BYTES_PER_CHANNEL(a_bit_width)	(a_bit_width > 16) ? 4 : 2
#define IPQ_LPASS_PCM_SAMPLES(_rate, _channel, _bytes_per_channel)	\
						(_rate * _channel *	\
						_bytes_per_channel *	\
						MAX_PCM_SAMPLES)

#define IPQ_PCM_SAMPLES_PER_PERIOD(_rate)	(_rate / 1000)
#define IPQ_PCM_BYTES_PER_SAMPLE_MAX		4
#define IPQ_PCM_MAX_CHANNEL_CNT			32
#define IPQ_PCM_MAX_SLOTS_PER_FRAME		32

/** Short (one-bit) Synchronization mode. */
#define PCM_SHORT_SYNC_BIT_MODE		0

/** Long Synchronization mode. */
#define PCM_LONG_SYNC_MODE			1

/** Short (one-slot) Synchronization mode. */
#define PCM_SHORT_SYNC_SLOT_MODE		2

/** Synchronization source is external. */
#define PCM_SYNC_SRC_EXTERNAL			0

/** Synchronization source is internal. */
#define PCM_SYNC_SRC_INTERNAL			1

/** Disable sharing of the data-out signal. */
#define PCM_CTRL_DATA_OE_DISABLE		0

/** Enable sharing of the data-out signal. */
#define PCM_CTRL_DATA_OE_ENABLE			1

/** Normal synchronization. */
#define PCM_SYNC_NORMAL				0

/** Invert the synchronization. */
#define PCM_SYNC_INVERT				1

#define PCM_32BIT_FORMAT			31

#define PCM_16BIT_FORMAT			15

#define PCM_FORMAT_32				27

#define SHIFT_FACTOR		(PCM_32BIT_FORMAT - PCM_FORMAT_32)

/** Zero-bit clock cycle synchronization data delay. */
#define PCM_DATA_DELAY_0_BCLK_CYCLE		2

/** One-bit clock cycle synchronization data delay. */
#define PCM_DATA_DELAY_1_BCLK_CYCLE		1

/** Two-bit clock cycle synchronization data delay. @newpage */
#define PCM_DATA_DELAY_2_BCLK_CYCLE		0

#define PCM_SAMPLE_RATE_8K			8000
#define PCM_SAMPLE_RATE_11_025K			11025
#define PCM_SAMPLE_RATE_12K			12000
#define PCM_SAMPLE_RATE_16K			16000
#define PCM_SAMPLE_RATE_22_05K			22050
#define PCM_SAMPLE_RATE_24K			24000
#define PCM_SAMPLE_RATE_32K			32000
#define PCM_SAMPLE_RATE_44_1K			44100
#define PCM_SAMPLE_RATE_88_2K			88200
#define PCM_SAMPLE_RATE_48K			48000
#define PCM_SAMPLE_RATE_96K			96000
#define PCM_SAMPLE_RATE_176_4K			176400
#define PCM_SAMPLE_RATE_192K			192000
#define PCM_SAMPLE_RATE_352_8K			352800
#define PCM_SAMPLE_RATE_384K			384000
#define IPQ_LPASS_MAX_INTERFACE			2

enum ipq_hw_type {
	IPQ5018,
	IPQ9574,
	IPQ8074,
	IPQ5332
};

enum ipq_pcm_sampling_rate {
	IPQ_PCM_SAMPLING_RATE_8KHZ = 8000,
	IPQ_PCM_SAMPLING_RATE_16KHZ = 16000,
	IPQ_PCM_SAMPLING_RATE_MIN = IPQ_PCM_SAMPLING_RATE_8KHZ,
	IPQ_PCM_SAMPLING_RATE_MAX = IPQ_PCM_SAMPLING_RATE_16KHZ,
};

enum ipq_pcm_memory_type {
	DMA_MEMORY_LPM = 1,
	DMA_MEMORY_DDR
};

enum {
	DMA_CHANNEL0 = 0,
	DMA_CHANNEL1
};

enum {
	INTERRUPT_CHANNEL0 = 0,
	INTERRUPT_CHANNEL1
};

enum {
	PRIMARY = 0,
	SECONDARY
};

struct lpass_dma_buffer {
	uint16_t idx;
	uint16_t dir;
	uint16_t ifconfig;
	uint16_t frame;
	uint16_t intr_id;
	uint8_t  num_channels;
	uint8_t  bit_width;
	uint32_t bytes_per_channel;
	uint32_t period_count_in_word32;
	uint32_t bytes_per_sample;
	uint8_t	 *dma_buffer;
	uint32_t dma_memory_type;
	uint32_t dma_buffer_size;
	uint32_t dma_base_address;
	uint32_t dma_last_curr_addr;
	uint32_t watermark;
	uint32_t no_of_buffers;
	uint32_t single_buf_size;
	uint32_t int_samples_per_period;
	uint32_t max_size;
	dma_addr_t dma_addr;
};

struct ipq_lpass_pcm_global_config {
	struct lpass_dma_buffer *rx_dma_buffer;
	struct lpass_dma_buffer *tx_dma_buffer;
	struct ipq_lpass_pcm_params *pcm_params;
	uint32_t voice_loopback;
	uint32_t slave;
	uint32_t lpass_lpm_base_phy_addr;
	uint32_t dma_read_intr_status;
	void __iomem *ipq_lpass_lpm_base;
	atomic_t data_avail;
	atomic_t rx_add;
	wait_queue_head_t pcm_q;
};

struct lpass_irq_buffer {
	uint32_t pcm_index;
	struct lpass_dma_buffer *rx_buffer;
	struct lpass_dma_buffer *tx_buffer;
};

struct ipq_lpass_wr_rd_dma_config
{
	uint32_t tx_idx;
	uint32_t tx_dir;
	uint32_t tx_intr_id;
	uint32_t tx_ifconfig;
	uint32_t tx_buffer_start_addr;
	uint32_t rx_idx;
	uint32_t rx_dir;
	uint32_t rx_intr_id;
	uint32_t rx_ifconfig;
	uint32_t rx_buffer_start_addr;
	uint32_t read_intr_status;
};

struct ipq_lpass_pcm_tdm_config {
	uint32_t sync_src;
	uint32_t pcm_index;
	uint32_t dir;
	uint32_t invert_sync;
	uint32_t sync_type;
	uint32_t sync_delay;
	uint32_t ctrl_data_oe;
};

struct ipq_lpass_props {
	uint32_t npcm;
	struct ipq_lpass_pcm_tdm_config pcm_config[IPQ_LPASS_MAX_INTERFACE];
	struct ipq_lpass_wr_rd_dma_config dma_config[IPQ_LPASS_MAX_INTERFACE];
};


struct ipq_lpass_pcm_params {
	uint32_t bit_width;
	uint32_t rate;
	uint32_t slot_count;
	uint32_t pcm_index;
	uint32_t active_slot_count;
	uint32_t tx_slots[IPQ_PCM_MAX_SLOTS];
	uint32_t rx_slots[IPQ_PCM_MAX_SLOTS];
};

int ipq_pcm_init(struct ipq_lpass_pcm_params *params);
void ipq_pcm_deinit(struct ipq_lpass_pcm_params *params);
uint32_t ipq_pcm_data(uint8_t **rx_buf, uint8_t **tx_buf, int pcm_index);
void ipq_pcm_done(int pcm_index);
void ipq_pcm_send_event(int pcm_index);

#endif /*_IPQ_LPASS_TDM_PCM_H*/
