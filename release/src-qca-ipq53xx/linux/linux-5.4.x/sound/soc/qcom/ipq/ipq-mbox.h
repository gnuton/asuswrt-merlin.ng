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

#ifndef _IPQ_MBOX_H_
#define _IPQ_MBOX_H_

#include <sound/soc.h>

#include "ipq-adss.h"

#define ADSS_MBOX_INVALID_PCM			(0xFFFFFFFF)
#define ADSS_MBOX_REG_BASE			(0x7700000 + 0x6000)
#define ADSS_MBOX_RANGE				(0xFA000)
#define ADSS_MBOX_SPDIF_IRQ			(163 + 32)
#define ADSS_MBOX0_IRQ				(156 + 32)
#define ADSS_MBOX1_IRQ				(157 + 32)
#define ADSS_MBOX2_IRQ				(158 + 32)
#define ADSS_MBOX3_IRQ				(159 + 32)

#define CHANNEL_A_VDWORD_START 0
#define CHANNEL_B_VDWORD_START 18

#define CHANNEL_A_VDWORD_1 (CHANNEL_A_VDWORD_START + 0)
#define CHANNEL_A_VDWORD_2 (CHANNEL_A_VDWORD_START + 1)
#define CHANNEL_A_VDWORD_3 (CHANNEL_A_VDWORD_START + 2)
#define CHANNEL_A_VDWORD_4 (CHANNEL_A_VDWORD_START + 3)
#define CHANNEL_A_VDWORD_5 (CHANNEL_A_VDWORD_START + 4)
#define CHANNEL_A_VDWORD_6 (CHANNEL_A_VDWORD_START + 5)

#define CHANNEL_B_VDWORD_1 (CHANNEL_B_VDWORD_START + 0)
#define CHANNEL_B_VDWORD_2 (CHANNEL_B_VDWORD_START + 1)
#define CHANNEL_B_VDWORD_3 (CHANNEL_B_VDWORD_START + 2)
#define CHANNEL_B_VDWORD_4 (CHANNEL_B_VDWORD_START + 3)
#define CHANNEL_B_VDWORD_5 (CHANNEL_B_VDWORD_START + 4)
#define CHANNEL_B_VDWORD_6 (CHANNEL_B_VDWORD_START + 5)

#define CHANNEL_A_CDWORD_START 12
#define CHANNEL_B_CDWORD_START 30

#define CHANNEL_A_CDWORD_1 (CHANNEL_A_CDWORD_START + 0)
#define CHANNEL_B_CDWORD_2 (CHANNEL_B_CDWORD_START + 0)

/* Acc to IEC 60958-3, bit 0.0 = 0 is consumer
 *		       bit 0.1 = 1is compressed playback
 *		       bit 3.0 = 1 is sampling freq No specified
 */
#define SPDIF_CONSUMER_COMPRESD 0x01000006

/*
 * When the mailbox operation is started, the mailbox would get one descriptor
 * for the current data transfer and prefetch one more descriptor. When less
 * than 10 descriptors are configured, then it is possible that before the CPU
 * handles the interrupt, the mailbox could check the pre fetched descriptor
 * and stop the DMA transfer.
 * To handle this, the design is to use multiple descriptors, but they would
 * point to the same buffer address. This way  more number of descriptors
 * would satisfy the mbox requirement, and reusing the buffer address would
 * satisfy the upper layer's buffer requirement.
 *
 * The value of 10 descriptors was derived from trial and error testing
 * for minimum number of descriptors that would result in MBOX operations
 * without stopping.
 */
#define MBOX_MIN_DESC_NUM	10

enum {
	ADSS_MBOX_NR_CHANNELS = 5,
};

struct ipq_mbox_desc {
	unsigned int	length	: 12,	/* bit 11-00 */
			size	: 12,	/* bit 23-12 */
			vuc	: 1,	/* bit 24 */
			ei	: 1,	/* bit 25 */
			rsvd1	: 4,	/* bit 29-26 */
			EOM	: 1,	/* bit 30 */
			OWN	: 1;	/* bit 31 */
	unsigned int	BufPtr;
	unsigned int	NextPtr;
	unsigned int	vuc_dword[36];
};

struct ipq_mbox_rt_dir_priv {
	/* Desc array in virtual space */
	struct ipq_mbox_desc *dma_virt_head;

	/* Desc array for DMA */
	dma_addr_t dma_phys_head;
	struct device *dev;
	unsigned int ndescs;
	irq_handler_t callback;
	void *dai_priv;
	unsigned long status;
	u32 channel_id;
	u32 err_stats;
	u32 last_played_is_null;
	u32 write;
	u32 read;
};

struct ipq_mbox_rt_priv {
	int irq_no;
	void __iomem *mbox_reg_base;
	struct ipq_mbox_rt_dir_priv dir_priv[2];
	int mbox_started;
};

/* Replaces struct ath_i2s_softc */
struct ipq_pcm_pltfm_priv {
	struct snd_pcm_substream *playback;
	struct snd_pcm_substream *capture;
};

int ipq_mbox_fifo_reset(int channel_id);
int ipq_mbox_dma_start(int channel_id);
int ipq_mbox_dma_stop(int channel_id, u32 delay_in_ms);
int ipq_mbox_dma_reset_swap(int channel_id);
int ipq_mbox_dma_swap(int channel_id, snd_pcm_format_t format);
int ipq_mbox_dma_prepare(int channel_id);
int ipq_mbox_dma_resume(int channel_id);
int ipq_mbox_form_ring(int channel_id, dma_addr_t baseaddr, u8 *base,
				int period_bytes, int bufsize, int own_bit);
int ipq_mbox_dma_release(int channel);
int ipq_mbox_dma_init(struct device *dev, int channel_id,
	irq_handler_t callback, void *private_data);
void ipq_mbox_vuc_setup(int channel_id);
u32 ipq_mbox_get_played_offset(u32 channel_id);
int ipq_mbox_dma_deinit(u32 channel_id);
void ipq_mbox_desc_own(u32 channel_id, int desc_no, int own);
struct ipq_mbox_desc *ipq_mbox_get_last_played(unsigned int channel_id);
uint32_t ipq_mbox_get_elapsed_size(uint32_t channel_id);
void ipq_mbox_vuc_setup(int channel_id);
uint32_t ipq_mbox_get_played_offset_set_own(u32 channel_id);
void ipq_mbox_multi_desc_own(u32 channel_id, int desc_no,
					int own, int no_of_desc);

static inline u32 ipq_convert_id_to_channel(u32 id)
{
	return (id / 2);
}

static inline u32 ipq_convert_id_to_dir(u32 id)
{
	return (id % 2);
}

/*
 * If number of mbox descriptors are less than MBOX_MIN_DESC_NUM
 * there should be duplicate mbox descriptors in order to be compliant
 * with the mbox operation logic described at the definition of
 * macro MBOX_MIN_DESC_NUM.
 */
static inline int ipq_get_mbox_descs_duplicate(int ndescs)
{
	int repeat_cnt;

	if (ndescs < MBOX_MIN_DESC_NUM) {
		repeat_cnt = MBOX_MIN_DESC_NUM / ndescs;
		if (MBOX_MIN_DESC_NUM % ndescs)
			repeat_cnt++;
		ndescs *= repeat_cnt;
	}

	return ndescs;
}

#endif /* _IPQ_MBOX_H_ */
