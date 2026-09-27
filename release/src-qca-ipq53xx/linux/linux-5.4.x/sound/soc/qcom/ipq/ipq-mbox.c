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

#include <linux/dma-mapping.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/mm.h>
#include <linux/delay.h>
#include <sound/soc.h>
#include <linux/of_device.h>

#include "ipq-mbox.h"

enum {
	CHN_DISABLED = 0x00,
	CHN_ENABLED = 0x01, /* from dtsi */
	CHN_STARTED = 0x02, /* dma inited */
	CHN_STATUS_DISABLE = 0xFF,
};

static struct ipq_mbox_pdata {
	uint32_t ipq_desc_size;
	enum ipq_hw_type ipq_hw;
	uint32_t ipq_mbox_mask;
} ipq_mbox_info;

static struct ipq_mbox_rt_priv *mbox_rtime[ADSS_MBOX_NR_CHANNELS];

static inline uint32_t ipq_get_desc_size(void)
{
	struct ipq_mbox_desc *desc;

	ipq_mbox_info.ipq_desc_size =
			(sizeof(desc[0]) - sizeof(desc[0].vuc_dword));

	return ipq_mbox_info.ipq_desc_size;
}

#define ipq_increment_desc(ptr, n) ipq_increment_desc_ptr((void *)ptr, n)

inline struct ipq_mbox_desc *ipq_increment_desc_ptr(void *desc_ptr,
							int no_of_desc)
{
	return (struct ipq_mbox_desc *)((uint8_t *)desc_ptr +
				(ipq_mbox_info.ipq_desc_size * no_of_desc));
}

struct ipq_mbox_desc
	*ipq_mbox_get_last_played(unsigned int channel_id)
{
	struct ipq_mbox_desc *desc, *prev;
	unsigned int ndescs, i;
	uint32_t index;
	uint32_t dir;

	index = ipq_convert_id_to_channel(channel_id);
	dir = ipq_convert_id_to_dir(channel_id);

	if (!mbox_rtime[index])
		return NULL;

	ndescs = mbox_rtime[index]->dir_priv[dir].ndescs;
	/* Point to the last desc */
	prev =
	ipq_increment_desc(mbox_rtime[index]->dir_priv[dir].dma_virt_head,
							(ndescs - 1));

	/* Point to the first desc */
	desc = &mbox_rtime[index]->dir_priv[dir].dma_virt_head[0];

	for (i = 0; i < ndescs; i++) {
		if (desc->OWN == 1 && prev->OWN == 0)
			return desc;
		prev = desc;
		desc = ipq_increment_desc(desc, 1);
	}

	/* If we didn't find the last played buffer, return NULL */
	return NULL;
}
EXPORT_SYMBOL(ipq_mbox_get_last_played);

uint32_t ipq_mbox_get_elapsed_size(uint32_t channel_id)
{
	struct ipq_mbox_desc *desc;
	unsigned int i, size_played = 0;
	uint32_t index;
	uint32_t dir;

	index = ipq_convert_id_to_channel(channel_id);
	dir = ipq_convert_id_to_dir(channel_id);

	if (!mbox_rtime[index])
		return size_played;

	desc = mbox_rtime[index]->dir_priv[dir].dma_virt_head;

	for (i = 0; i < mbox_rtime[index]->dir_priv[dir].ndescs; i++) {
		if (desc->OWN == 0) {
			desc->OWN = 1;
			desc->ei = 1;
			size_played += desc->size;
		}
		desc = ipq_increment_desc(desc, 1);
	}

	return size_played;
}
EXPORT_SYMBOL(ipq_mbox_get_elapsed_size);

static struct ipq_mbox_desc *get_next(
					struct ipq_mbox_rt_dir_priv *rtdir,
					struct ipq_mbox_desc *desc)
{
	struct ipq_mbox_desc *end;

	end = ipq_increment_desc(rtdir->dma_virt_head, rtdir->ndescs);

	desc = ipq_increment_desc(desc, 1);

	if (desc >= end)
		desc = rtdir->dma_virt_head;

	return desc;
}

void ipq_mbox_desc_own(u32 channel_id, int desc_no, int own)
{
	struct ipq_mbox_desc *desc;
	struct ipq_mbox_rt_dir_priv *rtdir;
	u32 chan;
	u32 dir;

	chan = ipq_convert_id_to_channel(channel_id);
	dir = ipq_convert_id_to_dir(channel_id);

	rtdir = &mbox_rtime[chan]->dir_priv[dir];

	desc = rtdir->dma_virt_head;
	desc = ipq_increment_desc(desc, desc_no);

	rtdir->write = desc_no;

	desc->OWN = own;
	desc->ei = 1;
}
EXPORT_SYMBOL(ipq_mbox_desc_own);

void ipq_mbox_multi_desc_own(u32 channel_id, int desc_no,
				int own, int no_of_desc)
{
	struct ipq_mbox_desc *desc;
	struct ipq_mbox_rt_dir_priv *rtdir;
	u32 chan;
	u32 dir;
	uint32_t i;

	chan = ipq_convert_id_to_channel(channel_id);
	dir = ipq_convert_id_to_dir(channel_id);

	rtdir = &mbox_rtime[chan]->dir_priv[dir];

	desc = rtdir->dma_virt_head;
	desc = ipq_increment_desc(desc, desc_no);

	for (i = 0; i < no_of_desc; i++) {
		rtdir->write = desc_no + i;
		desc->OWN = own;
		desc->ei = 1;
		desc = ipq_increment_desc(desc, 1);
	}
}
EXPORT_SYMBOL(ipq_mbox_multi_desc_own);

u32 ipq_mbox_get_played_offset(u32 channel_id)
{
	struct ipq_mbox_desc *desc, *write;
	struct ipq_mbox_rt_dir_priv *rtdir;
	unsigned int size_played = 0, desc_cnt = 0;
	u32 chan;
	u32 dir;

	chan = ipq_convert_id_to_channel(channel_id);
	dir = ipq_convert_id_to_dir(channel_id);

	rtdir = &mbox_rtime[chan]->dir_priv[dir];

	desc = rtdir->dma_virt_head;
	write = ipq_increment_desc(desc, rtdir->write);

	desc = ipq_increment_desc(desc, rtdir->read);

	do {
		size_played = desc->size;
		rtdir->read = (rtdir->read + 1) % rtdir->ndescs;

		if (desc == write)
			break;

		desc = get_next(rtdir, desc);
		desc_cnt++;
	} while (desc->OWN == 0 && desc_cnt < rtdir->ndescs);

	return size_played * rtdir->read;
}
EXPORT_SYMBOL(ipq_mbox_get_played_offset);

uint32_t ipq_mbox_get_played_offset_set_own(u32 channel_id)
{
	struct ipq_mbox_desc *desc, *last_played, *prev;
	struct ipq_mbox_rt_dir_priv *rtdir;
	unsigned int i, desc_own, size_played = 0;
	u32 chan, dir;

	chan = ipq_convert_id_to_channel(channel_id);
	dir = ipq_convert_id_to_dir(channel_id);

	rtdir = &mbox_rtime[chan]->dir_priv[dir];
	last_played = NULL;

	/* Point to the last desc */
	prev = ipq_increment_desc(rtdir->dma_virt_head, (rtdir->ndescs - 1));
	desc_own = prev->OWN;

	/* point to first desc */
	desc = &rtdir->dma_virt_head[0];

	for (i = 0; i < rtdir->ndescs; i++) {
		if (prev->OWN == 0) {
			if (i == (rtdir->ndescs - 1)) {
				if (desc_own == 1)
					last_played = desc;
			} else if (desc->OWN == 1) {
				last_played = desc;
			}
			prev->OWN = 1;
			prev->ei = 1;
		}
		prev = desc;
		desc = ipq_increment_desc(desc, 1);
	}
	if (last_played) {
		desc = &rtdir->dma_virt_head[0];
		size_played = last_played->BufPtr - desc->BufPtr;
	} else {
		pr_debug("%s last played buf not found\n", __func__);
		rtdir->last_played_is_null++;
	}

	return size_played;
}
EXPORT_SYMBOL(ipq_mbox_get_played_offset_set_own);

int ipq_mbox_fifo_reset(int channel_id)
{
	void __iomem *mbox_reg;
	u32 chan, dir;

	chan = ipq_convert_id_to_channel(channel_id);
	dir = ipq_convert_id_to_dir(channel_id);

	if (!mbox_rtime[chan])
		return -EINVAL;

	mbox_reg = mbox_rtime[chan]->mbox_reg_base;

	switch (dir) {
	case PLAYBACK:
		writel(MBOX_FIFO_RESET_TX_INIT,
			 mbox_reg + ADSS_MBOXn_MBOX_FIFO_RESET_REG);
		break;
	case CAPTURE:
		writel(MBOX_FIFO_RESET_RX_INIT,
			 mbox_reg + ADSS_MBOXn_MBOX_FIFO_RESET_REG);
		break;
	}

	return 0;
}
EXPORT_SYMBOL(ipq_mbox_fifo_reset);

int ipq_mbox_dma_start(int channel_id)
{
	void __iomem *mbox_reg;
	u32 chan, dir;

	chan = ipq_convert_id_to_channel(channel_id);
	dir = ipq_convert_id_to_dir(channel_id);

	if (!mbox_rtime[chan])
		return -EINVAL;

	mbox_reg = mbox_rtime[chan]->mbox_reg_base;
	mbox_rtime[chan]->mbox_started = 1;

	switch (dir) {
	case PLAYBACK:
		writel(ADSS_MBOXn_DMA_RX_CONTROL_START,
			mbox_reg + ADSS_MBOXn_MBOXn_DMA_RX_CONTROL_REG);
		break;

	case CAPTURE:
		writel(ADSS_MBOXn_DMA_TX_CONTROL_START,
			mbox_reg + ADSS_MBOXn_MBOXn_DMA_TX_CONTROL_REG);
		break;
	}

	return 0;
}
EXPORT_SYMBOL(ipq_mbox_dma_start);

int ipq_mbox_dma_resume(int channel_id)
{
	void __iomem *mbox_reg;
	u32 chan, dir;

	chan = ipq_convert_id_to_channel(channel_id);
	dir = ipq_convert_id_to_dir(channel_id);

	if (!mbox_rtime[chan])
		return -EINVAL;

	mbox_reg = mbox_rtime[chan]->mbox_reg_base;

	switch (dir) {
	case PLAYBACK:
		writel(ADSS_MBOXn_DMA_RX_CONTROL_RESUME,
			mbox_reg + ADSS_MBOXn_MBOXn_DMA_RX_CONTROL_REG);
		break;

	case CAPTURE:
		writel(ADSS_MBOXn_DMA_TX_CONTROL_RESUME,
			mbox_reg + ADSS_MBOXn_MBOXn_DMA_TX_CONTROL_REG);
		break;
	}

	return 0;
}
EXPORT_SYMBOL(ipq_mbox_dma_resume);

int ipq_mbox_dma_stop(int channel_id, u32 delay_in_ms)
{
	void __iomem *mbox_reg;
	struct ipq_mbox_rt_dir_priv *mbox_cb;
	u32 chan, dir;

	chan = ipq_convert_id_to_channel(channel_id);
	dir = ipq_convert_id_to_dir(channel_id);

	if (!mbox_rtime[chan])
		return -EINVAL;

	mbox_reg = mbox_rtime[chan]->mbox_reg_base;
	mbox_rtime[chan]->mbox_started = 0;

	switch (dir) {
	case PLAYBACK:
		writel(ADSS_MBOXn_DMA_RX_CONTROL_STOP,
			mbox_reg + ADSS_MBOXn_MBOXn_DMA_RX_CONTROL_REG);
		break;

	case CAPTURE:
		writel(ADSS_MBOXn_DMA_TX_CONTROL_STOP,
			mbox_reg + ADSS_MBOXn_MBOXn_DMA_TX_CONTROL_REG);
		break;
	}

	/*
	 * Per the documentation:
	 *	______________________________________________________
	 *	Programming a one to this bit causes the DMA engine to
	 *	stop transferring any more data from this descriptor
	 *	chain.  If a transfer is already in progress DMA enters
	 *	a stop state after completing the current descriptor
	 *	______________________________________________________
	 */
	mdelay(delay_in_ms);

	mbox_cb = &mbox_rtime[chan]->dir_priv[dir];
	return 0;
}
EXPORT_SYMBOL(ipq_mbox_dma_stop);

static bool ipq_is_other_chn_active(u32 chan, u32 dir)
{
	if (dir == PLAYBACK)
		return (test_bit(CHN_STARTED,
			&mbox_rtime[chan]->dir_priv[CAPTURE].status));
	else
		return (test_bit(CHN_STARTED,
			&mbox_rtime[chan]->dir_priv[PLAYBACK].status));
}

int ipq_mbox_dma_reset_swap(int channel_id)
{
	unsigned int val;
	void __iomem *mbox_reg;
	u32 chan;

	chan = ipq_convert_id_to_channel(channel_id);

	if (!mbox_rtime[chan])
		return -EINVAL;

	mbox_reg = mbox_rtime[chan]->mbox_reg_base;

	val = readl(mbox_reg + ADSS_MBOXn_MBOX_DMA_POLICY_REG);
	val &= ~(MBOX_DMA_POLICY_RXD_END_SWAP | MBOX_DMA_POLICY_RXD_16BIT_SWAP);

	writel(val, mbox_reg + ADSS_MBOXn_MBOX_DMA_POLICY_REG);

	return 0;
}
EXPORT_SYMBOL(ipq_mbox_dma_reset_swap);

int ipq_mbox_dma_swap(int channel_id, snd_pcm_format_t format)
{
	unsigned int val;
	void __iomem *mbox_reg;
	u32 chan;

	chan = ipq_convert_id_to_channel(channel_id);

	if (!mbox_rtime[chan])
		return -EINVAL;

	mbox_reg = mbox_rtime[chan]->mbox_reg_base;

	val = readl(mbox_reg + ADSS_MBOXn_MBOX_DMA_POLICY_REG);
	switch (format) {
	case SNDRV_PCM_FORMAT_S16_LE:
	case SNDRV_PCM_FORMAT_S16_BE:
		val |= MBOX_DMA_POLICY_RXD_16BIT_SWAP;
		break;
	case SNDRV_PCM_FORMAT_S24_3LE:
	case SNDRV_PCM_FORMAT_S24_3BE:
		val |= MBOX_DMA_POLICY_RXD_END_SWAP;
		break;
	default:
		/* Nothing to do */
		break;
	}
	writel(val, mbox_reg + ADSS_MBOXn_MBOX_DMA_POLICY_REG);

	return 0;
}
EXPORT_SYMBOL(ipq_mbox_dma_swap);

static void ipq_mbox_intr_en(void __iomem *mbox_reg, unsigned int mask)
{
	unsigned int val;

	val = readl(mbox_reg + ADSS_MBOXn_MBOX_INT_ENABLE_REG);
	val |= mask;
	writel(val, mbox_reg + ADSS_MBOXn_MBOX_INT_ENABLE_REG);
}

static void ipq_mbox_intr_disable(void __iomem *mbox_reg, unsigned int mask)
{
	unsigned int val;

	val = readl(mbox_reg + ADSS_MBOXn_MBOX_INT_ENABLE_REG);
	val &= ~mask;
	writel(val, mbox_reg + ADSS_MBOXn_MBOX_INT_ENABLE_REG);
}

int ipq_mbox_dma_prepare(int channel_id)
{
	struct ipq_mbox_desc *desc;
	unsigned int val;
	void __iomem *mbox_reg;
	dma_addr_t phys_addr;
	u32 chan, dir;
	struct ipq_mbox_rt_dir_priv *mbox_cb;

	chan = ipq_convert_id_to_channel(channel_id);
	dir = ipq_convert_id_to_dir(channel_id);

	if (!mbox_rtime[chan])
		return -EINVAL;

	mbox_reg = mbox_rtime[chan]->mbox_reg_base;
	mbox_cb = &mbox_rtime[chan]->dir_priv[dir];

	/* do not reset DMA registers if the other direction is active */
	if (!ipq_is_other_chn_active(chan, dir)) {

		val = readl(mbox_reg + ADSS_MBOXn_MBOX_DMA_POLICY_REG);
		val |= MBOX_DMA_POLICY_SW_RESET;
		writel(val, mbox_reg + ADSS_MBOXn_MBOX_DMA_POLICY_REG);
		val &= ~MBOX_DMA_POLICY_SW_RESET;
		writel(val, mbox_reg + ADSS_MBOXn_MBOX_DMA_POLICY_REG);
	}

	desc = mbox_cb->dma_virt_head;
	phys_addr = mbox_cb->dma_phys_head;
	val = readl(mbox_reg + ADSS_MBOXn_MBOX_DMA_POLICY_REG);

	if (dir == PLAYBACK) {
		/* Request the DMA channel to the controller */
		val |= MBOX_DMA_POLICY_RX_INT_TYPE;

		/* The direction is indicated from the DMA engine perspective
		 * i.e. we'll be using the RX registers for Playback and
		 * the TX registers for capture
		 */

		val |= ADSS_MBOX_DMA_POLICY_SRAM_AC(phys_addr);
		writel(val, mbox_reg + ADSS_MBOXn_MBOX_DMA_POLICY_REG);
		writel((phys_addr & ipq_mbox_info.ipq_mbox_mask),
			mbox_reg + ADSS_MBOXn_MBOXn_DMA_RX_DESCRIPTOR_BASE_REG);
		ipq_mbox_intr_en(mbox_reg, MBOX_INT_ENABLE_RX_DMA_COMPLETE);
	} else {

		val |= MBOX_DMA_POLICY_TX_INT_TYPE |
			ADSS_MBOX_DMA_POLICY_TX_FIFO_THRESHOLD(6);
		val |= ADSS_MBOX_DMA_POLICY_SRAM_AC(phys_addr);
		writel(val, mbox_reg + ADSS_MBOXn_MBOX_DMA_POLICY_REG);
		writel((phys_addr & ipq_mbox_info.ipq_mbox_mask),
		mbox_reg + ADSS_MBOXn_MBOXn_DMA_TX_DESCRIPTOR_BASE_REG);
		ipq_mbox_intr_en(mbox_reg, MBOX_INT_ENABLE_TX_DMA_COMPLETE);
	}

	return 0;
}
EXPORT_SYMBOL(ipq_mbox_dma_prepare);

void ipq_mbox_vuc_setup(int channel_id)
{
	uint32_t index, dir;
	struct ipq_mbox_desc *desc;
	int ndescs;
	int i;

	index = ipq_convert_id_to_channel(channel_id);
	dir = ipq_convert_id_to_dir(channel_id);
	ndescs = mbox_rtime[index]->dir_priv[dir].ndescs;
	desc = mbox_rtime[index]->dir_priv[dir].dma_virt_head;

	/* Copy VUC from previous descriptors */
	for (i = 0; i < ndescs; i++) {
		/* Setup V bits as 1, Acc to IEC 60958-3 Standard
		 *    for non PCM data, we need to set invalid for
		 *     both channels
		 * There are 6 DWORDS (192 bits) for Channel A
		 * and 6 DWORDS (192 bits) for channel B
		 */
		desc[i].vuc_dword[CHANNEL_A_VDWORD_1] = ADSS_MBOX_INVALID_PCM;
		desc[i].vuc_dword[CHANNEL_A_VDWORD_2] = ADSS_MBOX_INVALID_PCM;
		desc[i].vuc_dword[CHANNEL_A_VDWORD_3] = ADSS_MBOX_INVALID_PCM;
		desc[i].vuc_dword[CHANNEL_A_VDWORD_4] = ADSS_MBOX_INVALID_PCM;
		desc[i].vuc_dword[CHANNEL_A_VDWORD_5] = ADSS_MBOX_INVALID_PCM;
		desc[i].vuc_dword[CHANNEL_A_VDWORD_6] = ADSS_MBOX_INVALID_PCM;
		desc[i].vuc_dword[CHANNEL_B_VDWORD_1] = ADSS_MBOX_INVALID_PCM;
		desc[i].vuc_dword[CHANNEL_B_VDWORD_2] = ADSS_MBOX_INVALID_PCM;
		desc[i].vuc_dword[CHANNEL_B_VDWORD_3] = ADSS_MBOX_INVALID_PCM;
		desc[i].vuc_dword[CHANNEL_B_VDWORD_4] = ADSS_MBOX_INVALID_PCM;
		desc[i].vuc_dword[CHANNEL_B_VDWORD_5] = ADSS_MBOX_INVALID_PCM;
		desc[i].vuc_dword[CHANNEL_B_VDWORD_6] = ADSS_MBOX_INVALID_PCM;

		/* Now setup C bits, acc to IEC-60958-3 */
		desc[i].vuc_dword[CHANNEL_A_CDWORD_1] = SPDIF_CONSUMER_COMPRESD;
		desc[i].vuc_dword[CHANNEL_B_CDWORD_2] = SPDIF_CONSUMER_COMPRESD;
	}
}
EXPORT_SYMBOL(ipq_mbox_vuc_setup);

int ipq_mbox_form_ring(int channel_id, dma_addr_t baseaddr, u8 *area,
			   int period_bytes, int bufsize, int own_bit)
{
	struct ipq_mbox_desc *desc, *_desc_p;
	dma_addr_t desc_p, baseaddr_const;
	unsigned int i, ndescs;
	u32 chan, dir;
	struct ipq_mbox_rt_dir_priv *mbox_cb;

	chan = ipq_convert_id_to_channel(channel_id);
	dir = ipq_convert_id_to_dir(channel_id);

	if (!mbox_rtime[chan])
		return -EINVAL;

	mbox_cb = &mbox_rtime[chan]->dir_priv[dir];
	ndescs = DIV_ROUND_UP(bufsize, period_bytes);

	desc = (struct ipq_mbox_desc *)(area + (ndescs * period_bytes));
	desc_p = baseaddr + (ndescs * period_bytes);

	/*
	 * Finding whether duplicate desc entries are required or not should
	 * be done after desc is initialized. Else if ndescs are less than
	 * MBOX_MIN_DESC_NUM and duplicate desc entry is calculated before desc
	 * initialization, then desc will point to a region beyond allocated
	 * coherent memory.
	 */
	ndescs = ipq_get_mbox_descs_duplicate(ndescs);

	memset(desc, 0, ndescs * sizeof(struct ipq_mbox_desc));

	mbox_cb->read = 0;
	mbox_cb->write = 0;
	mbox_cb->ndescs = ndescs;
	mbox_cb->dma_virt_head = desc;
	mbox_cb->dma_phys_head = desc_p;
	_desc_p = (struct ipq_mbox_desc *)desc_p;

	baseaddr_const = baseaddr;

	for (i = 0; i < ndescs; i++) {
		desc->OWN = own_bit;
		desc->ei = 1;
		desc->BufPtr = (baseaddr & ipq_mbox_info.ipq_mbox_mask);
		desc->NextPtr = (unsigned long)ipq_increment_desc(_desc_p,
							((i + 1) % ndescs));
		desc->size = period_bytes;
		desc->length = desc->size;
		baseaddr += period_bytes;

		if (baseaddr >= (baseaddr_const + bufsize)) {
			if (bufsize % period_bytes)
				desc->size = bufsize % period_bytes;
			else
				desc->size = period_bytes;

			baseaddr = baseaddr_const;
		}
		desc = ipq_increment_desc(desc, 1);
	}

	return 0;
}
EXPORT_SYMBOL(ipq_mbox_form_ring);

int ipq_mbox_dma_release(int channel_id)
{
	u32 chan, dir;
	struct ipq_mbox_rt_dir_priv *mbox_cb;

	chan = ipq_convert_id_to_channel(channel_id);
	dir = ipq_convert_id_to_dir(channel_id);
	mbox_cb = &mbox_rtime[chan]->dir_priv[dir];

	if (test_bit(CHN_STARTED, &mbox_cb->status)) {
		ipq_mbox_intr_disable(mbox_rtime[chan]->mbox_reg_base,
				(MBOX_INT_ENABLE_TX_DMA_COMPLETE |
					MBOX_INT_ENABLE_RX_DMA_COMPLETE));
		/*
		 * ALSA framework calls ipq_mbox_dma_stop() before
		 * calling close API.
		 */
		mbox_cb->dma_virt_head = NULL;

		clear_bit(CHN_STARTED, &mbox_cb->status);
		return 0;
	}

	mbox_cb->read = 0;
	mbox_cb->write = 0;

	return -ENXIO;
}
EXPORT_SYMBOL(ipq_mbox_dma_release);

static void irq_proc_status(struct ipq_mbox_rt_dir_priv *priv, int irq,
			    u32 status, int cb, int stats, u32 *mask, u32 bit)
{
	if (status & bit) {
		*mask |= bit;
		if (cb && priv->callback)
			priv->callback(irq, priv->dai_priv);

		if (stats) {
			ipq_mbox_dma_start(priv->channel_id);
			priv->err_stats++;
		}
	}
}

static irqreturn_t ipq_mbox_dma_irq(int irq, void *dev_id)
{
	unsigned int status, mask = 0;
	struct ipq_mbox_rt_priv *curr_rtime = dev_id;
	void __iomem *mbox_reg = curr_rtime->mbox_reg_base;
	struct ipq_mbox_rt_dir_priv *p = &curr_rtime->dir_priv[PLAYBACK];
	struct ipq_mbox_rt_dir_priv *c = &curr_rtime->dir_priv[CAPTURE];

	status = readl(mbox_reg + ADSS_MBOXn_MBOX_INT_STATUS_REG);

	irq_proc_status(p, irq, status, 1, 0, &mask,
					MBOX_INT_STATUS_RX_DMA_COMPLETE);
	irq_proc_status(c, irq, status, 1, 0, &mask,
					MBOX_INT_STATUS_TX_DMA_COMPLETE);
	irq_proc_status(p, irq, status, 0, 1, &mask,
					MBOX_INT_STATUS_RX_UNDERFLOW);
	irq_proc_status(p, irq, status, 0, 1, &mask,
					MBOX_INT_STATUS_RX_FIFO_UNDERFLOW);
	irq_proc_status(c, irq, status, 0, 1, &mask,
					MBOX_INT_STATUS_TX_OVERFLOW);
	irq_proc_status(c, irq, status, 0, 1, &mask,
					MBOX_INT_STATUS_TX_FIFO_OVERFLOW);

	if (mask) {
		writel(mask, mbox_reg + ADSS_MBOXn_MBOX_INT_STATUS_REG);
		return IRQ_HANDLED;
	}

	return IRQ_NONE;
}

int ipq_mbox_dma_deinit(u32 channel_id)
{
	u32 chan, dir;
	struct ipq_mbox_rt_dir_priv *mbox_cb;

	chan = ipq_convert_id_to_channel(channel_id);
	dir = ipq_convert_id_to_dir(channel_id);

	mbox_cb = &mbox_rtime[chan]->dir_priv[dir];
	if (test_bit(CHN_STARTED, &mbox_cb->status))
		clear_bit(CHN_STARTED, &mbox_cb->status);

	mbox_cb->dai_priv = NULL;
	mbox_cb->callback = NULL;
	mbox_cb->dev = NULL;

	return 0;
}
EXPORT_SYMBOL(ipq_mbox_dma_deinit);

int ipq_mbox_dma_init(struct device *dev, int channel_id,
			irq_handler_t callback, void *private_data)
{
	u32 chan;
	u32 dir;
	struct ipq_mbox_rt_dir_priv *mbox_cb;

	chan = ipq_convert_id_to_channel(channel_id);
	dir = ipq_convert_id_to_dir(channel_id);

	if (chan  >= ADSS_MBOX_NR_CHANNELS)
		return -EINVAL;

	if (!mbox_rtime[chan])
		return -EINVAL;

	mbox_cb = &mbox_rtime[chan]->dir_priv[dir];

	if (!(mbox_cb->status & CHN_ENABLED))
		return -EINVAL;

	if (test_and_set_bit(CHN_STARTED, &mbox_cb->status))
		return -EBUSY;

	mbox_cb->dai_priv = private_data;
	mbox_cb->callback = callback;
	mbox_cb->dev = dev;
	ipq_get_desc_size();

	return 0;
}
EXPORT_SYMBOL(ipq_mbox_dma_init);

static const struct of_device_id ipq_mbox_table[] = {
	{ .compatible = "qca,ipq8074-mbox", .data = (void *)IPQ8074 },
	{},
};

static int ipq_mbox_probe(struct platform_device *pdev)
{
	struct device_node *np;
	int irq;
	u32 tx_channel;
	u32 rx_channel;
	u32 id;
	void __iomem *reg_base;
	struct resource *res;
	int rc;
	const struct of_device_id *match;

	match = of_match_device(ipq_mbox_table, &pdev->dev);
	if (!match)
		return -ENODEV;

	ipq_mbox_info.ipq_hw = (enum ipq_hw_type)match->data;

	np = pdev->dev.of_node;

	if (of_property_read_u32(np, "dma-index", &id)) {
		dev_err(&pdev->dev,
			"unable to read (dma-index) from device node %s\n",
			np->name);
		return -EINVAL;
	}

	if (id >= ADSS_MBOX_NR_CHANNELS)
		return -EINVAL;

	if (of_property_read_u32(np, "tx-channel", &tx_channel))
		tx_channel = CHN_STATUS_DISABLE;

	if (of_property_read_u32(np, "rx-channel", &rx_channel))
		rx_channel = CHN_STATUS_DISABLE;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(&pdev->dev, "%s: %d: Error getting mbox resource\n",
						__func__, __LINE__);
		return -EINVAL;
	}

	/*
	 * Read interrupt and store
	 */
	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		dev_err(&pdev->dev, "%s: MBOX %d IRQ %d is not provided\n",
						__func__, id, irq);
		return irq;
	}

	reg_base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(reg_base))
		return PTR_ERR(reg_base);

	mbox_rtime[id] = devm_kzalloc(&pdev->dev,
			sizeof(struct ipq_mbox_rt_priv), GFP_KERNEL);
	if (!mbox_rtime[id])
		return -ENOMEM;

	rc = devm_request_irq(&pdev->dev, irq, ipq_mbox_dma_irq, 0,
				"ipq-mbox", mbox_rtime[id]);
	if (rc) {
		dev_err(&pdev->dev, "request_irq() failed with ret: %d\n", rc);
		return rc;
	}

	mbox_rtime[id]->mbox_reg_base = reg_base;
	mbox_rtime[id]->dir_priv[PLAYBACK].channel_id = tx_channel;
	mbox_rtime[id]->dir_priv[CAPTURE].channel_id = rx_channel;
	mbox_rtime[id]->dir_priv[PLAYBACK].status =
		(tx_channel == CHN_STATUS_DISABLE) ? CHN_DISABLED : CHN_ENABLED;
	mbox_rtime[id]->dir_priv[CAPTURE].status =
		(rx_channel == CHN_STATUS_DISABLE) ? CHN_DISABLED : CHN_ENABLED;
	mbox_rtime[id]->irq_no = irq;

	ipq_mbox_info.ipq_mbox_mask = DMA_BIT_MASK(32);

	return 0;
}

static struct platform_driver ipq_mbox_driver = {
	.probe = ipq_mbox_probe,
	.driver = {
		.name = "ipq-mbox",
		.of_match_table = ipq_mbox_table,
	},
};

module_platform_driver(ipq_mbox_driver);

MODULE_ALIAS("platform:ipq-mbox");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("IPQ MBOX DRIVER");
