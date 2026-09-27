/*
 * Copyright (c) 2015-2016 The Linux Foundation. All rights reserved.
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

#ifndef _IPQ_PCM_RAW_H
#define _IPQ_PCM_RAW_H

#include "ipq-mbox.h"
#include "ipq-adss.h"

#define NUM_BUFFERS		40
#define LOOPBACK_SKIP_COUNT	\
		(ipq_get_mbox_descs_duplicate(NUM_BUFFERS) * 3)

#define IPQ_PCM_SAMPLES_PER_10MS(rate) ((rate / 1000) * 10)

/* if the bit width is 16, two more bytes are required for the slot
 * information, which makes the bytes_per_sample as 4.
 * if the bit width is 8, one more byte is required for the slot
 * information and valid bit. The rest two bytes of the 32 bit data is
 * unused. So in this case too, So the number of bytes is 4
 */
#define IPQ8074_PCM_BYTES_PER_SAMPLE(bw) (bw == 8 ? 2 : 4)
#define IPQ_PCM_BYTES_PER_SAMPLE_MAX	4

#define IPQ_PCM_MAX_SLOTS	4

struct pcm_context {
	uint32_t pcm_started;
	uint8_t needs_deinit;
};

struct voice_dma_buffer {
	dma_addr_t addr;
	size_t size_max;
	size_t single_buf_size;
	uint32_t channel_id;
	unsigned char *area;
};

enum ipq_pcm_bits_in_frame {
	IPQ_PCM_BITS_IN_FRAME_256 = 256,
};

enum ipq_pcm_bit_width {
	IPQ_PCM_BIT_WIDTH_8 = 8,
	IPQ_PCM_BIT_WIDTH_16 = 16,
};

enum ipq_pcm_sampling_rate {
	IPQ_PCM_SAMPLING_RATE_8KHZ = 8000,
	IPQ_PCM_SAMPLING_RATE_16KHZ = 16000,
	IPQ_PCM_SAMPLING_RATE_MIN = IPQ_PCM_SAMPLING_RATE_8KHZ,
	IPQ_PCM_SAMPLING_RATE_MAX = IPQ_PCM_SAMPLING_RATE_16KHZ,
};

enum ipq_pcm_slots_per_frame {
	IPQ_PCM_SLOTS_16 = 16,
	IPQ_PCM_SLOTS_32 = 32,
};

struct ipq_pcm_params {
	uint32_t bit_width;
	uint32_t rate;
	uint32_t slot_count;
	uint32_t active_slot_count;
	uint32_t tx_slots[IPQ_PCM_MAX_SLOTS];
	uint32_t rx_slots[IPQ_PCM_MAX_SLOTS];
};

int ipq_pcm_init(struct ipq_pcm_params *params);
void ipq_pcm_deinit(struct ipq_pcm_params *params);
uint32_t ipq_pcm_data(uint8_t **rx_buf, uint8_t **tx_buf);
void ipq_pcm_done(void);
void ipq_pcm_send_event(void);

#endif /*_IPQ_PCM_RAW_H */
