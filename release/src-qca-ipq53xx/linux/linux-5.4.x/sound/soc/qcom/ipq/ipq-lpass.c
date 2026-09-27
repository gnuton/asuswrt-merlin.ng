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
 *
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/bitops.h>
#include <sound/pcm.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/reset.h>
#include <linux/spinlock.h>
#include <linux/of_device.h>
#include <linux/clk.h>
#include <linux/reset.h>

#include "ipq-lpass.h"
#include "ipq-lpass-cc.h"
#ifdef CONFIG_SND_SOC_IPQ9574_LPASS_PCM_RAW
#include "ipq9574-lpass-pcm.h"
#else
#include "ipq-lpass-tdm-pcm.h"
#endif


static void __iomem *sg_ipq_lpass_base;
static enum ipq_hw_type ipq_hw;

static void ipq_lpass_reg_update(void __iomem *register_addr, uint32_t mask,
					uint32_t value, bool f_writeonly)
{
	uint32_t temp = 0;

	if (f_writeonly == false) {
		temp = readl(register_addr);
	}
	mask = ~mask;
	/*
	 * Zero-out the fields that will be udpated
	 */
	temp = temp & mask;
	/*
	 * Update with new values
	 */
	temp = temp | value;
	/*
	 * write new value to HW reg
	 */
	writel(temp, register_addr);
}

static void ipq_lpass_cc_update(void __iomem *register_addr, uint32_t value,
					bool  f_update)
{
	uint32_t temp = readl(register_addr);

	temp = temp | value;
	writel(temp, register_addr);

	if (f_update) {
		temp = readl(register_addr);
		temp &= ~0x1;
		temp |= 0x1;
		writel(temp, register_addr);
	}
}

void ipq_lpass_pcm_reset(void __iomem *lpaif_base,
				uint32_t pcm_index, uint32_t dir)
{
	uint32_t mask,value;

	mask = HWIO_LPASS_LPAIF_PCM_CTLa_RESET_BMSK;
	value = HWIO_LPASS_LPAIF_PCM_CTLa_RESET_ENABLE_FVAL <<
			HWIO_LPASS_LPAIF_PCM_CTLa_RESET_SHFT;

	if (TDM_SINK == dir) {
		value |= HWIO_LPASS_LPAIF_PCM_CTLa_RESET_TX_ENABLE_FVAL <<
				HWIO_LPASS_LPAIF_PCM_CTLa_RESET_TX_SHFT;
		mask |= HWIO_LPASS_LPAIF_PCM_CTLa_RESET_TX_BMSK;
	} else {
		value |= HWIO_LPASS_LPAIF_PCM_CTLa_ENABLE_RX_ENABLE_FVAL <<
				HWIO_LPASS_LPAIF_PCM_CTLa_RESET_RX_SHFT;
		mask |= HWIO_LPASS_LPAIF_PCM_CTLa_RESET_RX_BMSK;
	}
	ipq_lpass_reg_update(
		HWIO_LPASS_LPAIF_PCM_CTLa_ADDR(lpaif_base, pcm_index),
		mask, value, 0);
}
EXPORT_SYMBOL(ipq_lpass_pcm_reset);

void ipq_lpass_pcm_reset_release(void __iomem *lpaif_base, uint32_t pcm_index,
						uint32_t dir)
{
	uint32_t mask,value;

	mask = HWIO_LPASS_LPAIF_PCM_CTLa_RESET_BMSK;
	value = HWIO_LPASS_LPAIF_PCM_CTLa_RESET_DISABLE_FVAL <<
			HWIO_LPASS_LPAIF_PCM_CTLa_RESET_SHFT;

	if (TDM_SINK == dir) {
		value |= HWIO_LPASS_LPAIF_PCM_CTLa_RESET_TX_DISABLE_FVAL <<
			HWIO_LPASS_LPAIF_PCM_CTLa_RESET_TX_SHFT;
		mask |= HWIO_LPASS_LPAIF_PCM_CTLa_RESET_TX_BMSK;
	} else {
		value |= HWIO_LPASS_LPAIF_PCM_CTLa_RESET_RX_DISABLE_FVAL <<
			HWIO_LPASS_LPAIF_PCM_CTLa_RESET_RX_SHFT;
		mask |= HWIO_LPASS_LPAIF_PCM_CTLa_RESET_RX_BMSK;
	}

	ipq_lpass_reg_update(HWIO_LPASS_LPAIF_PCM_CTLa_ADDR(lpaif_base,
				pcm_index), mask, value, 0);
}
EXPORT_SYMBOL(ipq_lpass_pcm_reset_release);

void ipq_lpass_pcm_config(struct ipq_lpass_pcm_config *configPtr,
				void __iomem *lpaif_base, uint32_t pcm_index,
				uint32_t dir)
{
	uint32_t mask, value;
	uint32_t regOffset;
	uint32_t bits_per_frame;

	writel(0x1,HWIO_LPASS_LPAIF_PCM_I2S_SELa_ADDR(lpaif_base, pcm_index));

	value = 0;

	mask =  HWIO_LPASS_LPAIF_PCM_CTLa_SYNC_SRC_BMSK |
			HWIO_LPASS_LPAIF_PCM_CTLa_AUX_MODE_BMSK |
			HWIO_LPASS_LPAIF_PCM_CTLa_CTRL_DATA_OE_BMSK;

	if (TDM_SINK == dir) {
		mask |= HWIO_LPASS_LPAIF_PCM_CTLa_TPCM_WIDTH_BMSK;
		if (configPtr->bit_width == 16)
			value |= (0x1
				<< HWIO_LPASS_LPAIF_PCM_CTLa_TPCM_WIDTH_SHFT);
	} else {
		mask |= HWIO_LPASS_LPAIF_PCM_CTLa_RPCM_WIDTH_BMSK;
		if (configPtr->bit_width == 16)
			value |= (0x1
				<< HWIO_LPASS_LPAIF_PCM_CTLa_RPCM_WIDTH_SHFT);
	}

	switch (configPtr->sync_src) {
	case TDM_MODE_MASTER:
		value |= (HWIO_LPASS_LPAIF_PCM_CTLa_SYNC_SRC_INTERNAL_FVAL
				<< HWIO_LPASS_LPAIF_PCM_CTLa_SYNC_SRC_SHFT);
	break;
	case TDM_MODE_SLAVE:
		value |= (HWIO_LPASS_LPAIF_PCM_CTLa_SYNC_SRC_EXTERNAL_FVAL
				<< HWIO_LPASS_LPAIF_PCM_CTLa_SYNC_SRC_SHFT);
	break;
	default:
	break;
	}

	switch (configPtr->sync_type) {
	case TDM_SHORT_SYNC_TYPE:
		value |= (HWIO_LPASS_LPAIF_PCM_CTLa_AUX_MODE_PCM_FVAL
				<< HWIO_LPASS_LPAIF_PCM_CTLa_AUX_MODE_SHFT);
	break;
	case TDM_LONG_SYNC_TYPE:
		value |= (HWIO_LPASS_LPAIF_PCM_CTLa_AUX_MODE_AUX_FVAL
				<< HWIO_LPASS_LPAIF_PCM_CTLa_AUX_MODE_SHFT);
	break;
	case TDM_SLOT_SYNC_TYPE:
		value |= (HWIO_LPASS_LPAIF_PCM_CTLa_AUX_MODE_PCM_FVAL
				<< HWIO_LPASS_LPAIF_PCM_CTLa_AUX_MODE_SHFT);
		value |= (HWIO_LPASS_LPAIF_PCM_CTLa_ONE_SLOT_SYNC_EN_ENABLE_FVAL
				<< HWIO_LPASS_LPAIF_PCM_CTLa_ONE_SLOT_SYNC_EN_SHFT);
	break;
	default:
	break;
	}

	switch (configPtr->ctrl_data_oe) {
	case TDM_CTRL_DATA_OE_DISABLE:
		value |= (TDM_CTRL_DATA_OE_DISABLE
				<< HWIO_LPASS_LPAIF_PCM_CTLa_CTRL_DATA_OE_SHFT);
	break;
	case TDM_CTRL_DATA_OE_ENABLE:
		value |= (TDM_CTRL_DATA_OE_ENABLE
			<< HWIO_LPASS_LPAIF_PCM_CTLa_CTRL_DATA_OE_SHFT);
	break;
	default:
	break;
	}

	regOffset = HWIO_LPASS_LPAIF_PCM_CTLa_OFFS(pcm_index);
	ipq_lpass_reg_update(lpaif_base + regOffset, mask, value, 0);

	mask = HWIO_LPASS_LPAIF_PCM_TDM_CTL_a_ENABLE_TDM_BMSK |
		HWIO_LPASS_LPAIF_PCM_TDM_CTL_a_TDM_SYNC_DELAY_BMSK |
		HWIO_LPASS_LPAIF_PCM_TDM_CTL_a_TDM_RATE_BMSK |
		HWIO_LPASS_LPAIF_PCM_TDM_CTL_a_ENABLE_DIFF_SAMPLE_WIDTH_BMSK;

	value  = HWIO_LPASS_LPAIF_PCM_TDM_CTL_a_ENABLE_TDM_ENABLE_FVAL
			<< HWIO_LPASS_LPAIF_PCM_TDM_CTL_a_ENABLE_TDM_SHFT;

	switch (configPtr->sync_delay) {
	case TDM_DATA_DELAY_0_CYCLE:
		value |= HWIO_LPASS_LPAIF_PCM_TDM_CTL_a_TDM_SYNC_DELAY_DELAY_0_CYCLE_FVAL
				<< HWIO_LPASS_LPAIF_PCM_TDM_CTL_a_TDM_SYNC_DELAY_SHFT;
	break;
	case TDM_DATA_DELAY_1_CYCLE:
	default:
		value |= HWIO_LPASS_LPAIF_PCM_TDM_CTL_a_TDM_SYNC_DELAY_DELAY_1_CYCLE_FVAL
				<< HWIO_LPASS_LPAIF_PCM_TDM_CTL_a_TDM_SYNC_DELAY_SHFT;
	break;
	case TDM_DATA_DELAY_2_CYCLE:
		value |= HWIO_LPASS_LPAIF_PCM_TDM_CTL_a_TDM_SYNC_DELAY_DELAY_2_CYCLE_FVAL
				<< HWIO_LPASS_LPAIF_PCM_TDM_CTL_a_TDM_SYNC_DELAY_SHFT;
	break;
	}

	bits_per_frame = configPtr->slot_count * configPtr->slot_width;

	value |= (bits_per_frame - 1) <<
			HWIO_LPASS_LPAIF_PCM_TDM_CTL_a_TDM_RATE_SHFT;

	if (configPtr->bit_width != configPtr->slot_width) {
		value |= HWIO_LPASS_LPAIF_PCM_TDM_CTL_a_ENABLE_DIFF_SAMPLE_WIDTH_ENABLE_FVAL
				<< HWIO_LPASS_LPAIF_PCM_TDM_CTL_a_ENABLE_DIFF_SAMPLE_WIDTH_SHFT;
	} else {
		value |= HWIO_LPASS_LPAIF_PCM_TDM_CTL_a_ENABLE_DIFF_SAMPLE_WIDTH_DISABLE_FVAL
				<< HWIO_LPASS_LPAIF_PCM_TDM_CTL_a_ENABLE_DIFF_SAMPLE_WIDTH_SHFT;
	}

	if (TDM_SINK == dir) {
		mask |= HWIO_LPASS_LPAIF_PCM_TDM_CTL_a_TDM_INV_TPCM_SYNC_BMSK;
		value |= configPtr->invert_sync
			<< HWIO_LPASS_LPAIF_PCM_TDM_CTL_a_TDM_INV_TPCM_SYNC_SHFT;

		mask |= HWIO_LPASS_LPAIF_PCM_TDM_CTL_a_TDM_TPCM_WIDTH_BMSK;
		value |= (configPtr->bit_width - 1)
			<< HWIO_LPASS_LPAIF_PCM_TDM_CTL_a_TDM_TPCM_WIDTH_SHFT;
	} else {
		mask |= HWIO_LPASS_LPAIF_PCM_TDM_CTL_a_TDM_INV_RPCM_SYNC_BMSK;
		value |= configPtr->invert_sync
			<< HWIO_LPASS_LPAIF_PCM_TDM_CTL_a_TDM_INV_RPCM_SYNC_SHFT;

		mask |= HWIO_LPASS_LPAIF_PCM_TDM_CTL_a_TDM_RPCM_WIDTH_BMSK;
		value |= (configPtr->bit_width - 1)
			<< HWIO_LPASS_LPAIF_PCM_TDM_CTL_a_TDM_RPCM_WIDTH_SHFT;
	}
	regOffset = HWIO_LPASS_LPAIF_PCM_TDM_CTL_a_OFFS(pcm_index);
	ipq_lpass_reg_update(lpaif_base + regOffset, mask, value, 0);

	if (configPtr->bit_width != configPtr->slot_width) {
		if (TDM_SINK == dir) {
			mask = HWIO_LPASS_LPAIF_PCM_TDM_SAMPLE_WIDTH_a_TDM_TPCM_SAMPLE_WIDTH_BMSK;
			value = (configPtr->slot_width - 1)
				<< HWIO_LPASS_LPAIF_PCM_TDM_SAMPLE_WIDTH_a_TDM_TPCM_SAMPLE_WIDTH_SHFT;
		} else {
			mask = HWIO_LPASS_LPAIF_PCM_TDM_SAMPLE_WIDTH_a_TDM_RPCM_SAMPLE_WIDTH_BMSK;
			value = (configPtr->slot_width - 1)
				<< HWIO_LPASS_LPAIF_PCM_TDM_SAMPLE_WIDTH_a_TDM_RPCM_SAMPLE_WIDTH_SHFT;
		}
	regOffset = HWIO_LPASS_LPAIF_PCM_TDM_SAMPLE_WIDTH_a_OFFS(pcm_index);
	ipq_lpass_reg_update(lpaif_base + regOffset, mask, value, 0);
	}

	if (TDM_SINK == dir) {
		regOffset = HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_OFFS(pcm_index);
		mask = HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_RMSK;
	} else {
		regOffset = HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_OFFS(pcm_index);
		mask = HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RMSK;
	}
	value = configPtr->slot_mask;
	ipq_lpass_reg_update(lpaif_base + regOffset, mask, value, 0);

	mask = HWIO_LPASS_LPAIF_PCM_LANE_CONFIG_a_LANE0_EN_BMSK |
		HWIO_LPASS_LPAIF_PCM_LANE_CONFIG_a_LANE0_DIR_BMSK;

	value = (HWIO_LPASS_LPAIF_PCM_LANE_CONFIG_a_LANE0_EN_ENABLE_FVAL <<
			HWIO_LPASS_LPAIF_PCM_LANE_CONFIG_a_LANE0_EN_SHFT);

	if (ipq_hw == IPQ9574 || ipq_hw == IPQ5332) {
		mask |= HWIO_LPASS_LPAIF_PCM_LANE_CONFIG_a_LANE1_EN_BMSK |
			HWIO_LPASS_LPAIF_PCM_LANE_CONFIG_a_LANE1_DIR_BMSK;
		value |= (HWIO_LPASS_LPAIF_PCM_LANE_CONFIG_a_LANE1_EN_ENABLE_FVAL <<
				HWIO_LPASS_LPAIF_PCM_LANE_CONFIG_a_LANE1_EN_SHFT);
	}
	regOffset = HWIO_LPASS_LPAIF_PCM_LANE_CONFIG_a_OFFS(pcm_index);

	if (ipq_hw == IPQ9574 || ipq_hw == IPQ5332) {
		if (TDM_SINK == dir) {
			value |= (HWIO_LPASS_LPAIF_PCM_LANE_CONFIG_a_LANE0_DIR_SPKR_FVAL
				<< HWIO_LPASS_LPAIF_PCM_LANE_CONFIG_a_LANE0_DIR_SHFT);
		} else {
			value |= (HWIO_LPASS_LPAIF_PCM_LANE_CONFIG_a_LANE1_DIR_MIC_FVAL
				<< HWIO_LPASS_LPAIF_PCM_LANE_CONFIG_a_LANE1_DIR_SHFT);
		}
	} else {
		if (TDM_SINK == dir) {
			value |= (HWIO_LPASS_LPAIF_PCM_LANE_CONFIG_a_LANE0_DIR_SPKR_FVAL
				<< HWIO_LPASS_LPAIF_PCM_LANE_CONFIG_a_LANE0_DIR_SHFT);
		} else {
			value |= (HWIO_LPASS_LPAIF_PCM_LANE_CONFIG_a_LANE0_DIR_MIC_FVAL
				<< HWIO_LPASS_LPAIF_PCM_LANE_CONFIG_a_LANE0_DIR_SHFT);
		}
	}

	ipq_lpass_reg_update(lpaif_base + regOffset, mask, value, 1);
}
EXPORT_SYMBOL(ipq_lpass_pcm_config);

void ipq_lpass_pcm_enable(void __iomem *lpaif_base,
				uint32_t pcm_index, uint32_t dir)
{
	uint32_t mask,value;

	if (TDM_SINK == dir) {
		value = HWIO_LPASS_LPAIF_PCM_CTLa_ENABLE_TX_ENABLE_FVAL <<
				HWIO_LPASS_LPAIF_PCM_CTLa_ENABLE_TX_SHFT;
		mask = HWIO_LPASS_LPAIF_PCM_CTLa_ENABLE_TX_BMSK;
	} else {
		value = HWIO_LPASS_LPAIF_PCM_CTLa_ENABLE_RX_ENABLE_FVAL <<
				HWIO_LPASS_LPAIF_PCM_CTLa_ENABLE_RX_SHFT;
		mask = HWIO_LPASS_LPAIF_PCM_CTLa_ENABLE_RX_BMSK;
	}

	value |=  HWIO_LPASS_LPAIF_PCM_CTLa_ENABLE_ENABLE_FVAL <<
			HWIO_LPASS_LPAIF_PCM_CTLa_ENABLE_SHFT;

	mask |= HWIO_LPASS_LPAIF_PCM_CTLa_ENABLE_BMSK;

	ipq_lpass_reg_update(
		HWIO_LPASS_LPAIF_PCM_CTLa_ADDR(lpaif_base, pcm_index),
		mask, value, 0);
}

EXPORT_SYMBOL(ipq_lpass_pcm_enable);

void ipq_lpass_pcm_disable(void __iomem *lpaif_base,
					uint32_t pcm_index, uint32_t dir)
{
	uint32_t mask,value;

	if (TDM_SINK == dir) {
		value = HWIO_LPASS_LPAIF_PCM_CTLa_ENABLE_TX_DISABLE_FVAL <<
				HWIO_LPASS_LPAIF_PCM_CTLa_ENABLE_TX_SHFT;
		mask = HWIO_LPASS_LPAIF_PCM_CTLa_ENABLE_TX_BMSK;
	} else {
		value = HWIO_LPASS_LPAIF_PCM_CTLa_ENABLE_RX_DISABLE_FVAL <<
				HWIO_LPASS_LPAIF_PCM_CTLa_ENABLE_RX_SHFT;
		mask = HWIO_LPASS_LPAIF_PCM_CTLa_ENABLE_RX_BMSK;
	}

	ipq_lpass_reg_update(HWIO_LPASS_LPAIF_PCM_CTLa_ADDR(lpaif_base,
			pcm_index), mask, value, 0);
}
EXPORT_SYMBOL(ipq_lpass_pcm_disable);

void ipq_lpass_pcm_enable_loopback(void __iomem *lpaif_base,
						uint32_t pcm_index)
{
	uint32_t mask,value;

	mask = HWIO_LPASS_LPAIF_PCM_CTLa_LOOPBACK_BMSK;
	value = (1 << HWIO_LPASS_LPAIF_PCM_CTLa_LOOPBACK_SHFT);

	ipq_lpass_reg_update(HWIO_LPASS_LPAIF_PCM_CTLa_ADDR(lpaif_base,
		pcm_index), mask, value, 0);
}
EXPORT_SYMBOL(ipq_lpass_pcm_enable_loopback);

void ipq_lpass_enable_dma_channel(void __iomem *lpaif_base, uint32_t dma_idx,
							uint32_t dma_dir)
{
	uint32_t mask, value, offset;

	if (LPASS_HW_DMA_SINK == dma_dir) {
		mask = HWIO_LPASS_LPAIF_RDDMA_CTLa_ENABLE_ON_FVAL;
		value =
			HWIO_LPASS_LPAIF_RDDMA_CTLa_ENABLE_BMSK <<
			HWIO_LPASS_LPAIF_RDDMA_CTLa_ENABLE_SHFT;
		offset = HWIO_LPASS_LPAIF_RDDMA_CTLa_OFFS(dma_idx);
	} else {
		mask = HWIO_LPASS_LPAIF_WRDMA_CTLa_ENABLE_ON_FVAL;
		value =
			HWIO_LPASS_LPAIF_WRDMA_CTLa_ENABLE_BMSK <<
			HWIO_LPASS_LPAIF_WRDMA_CTLa_ENABLE_SHFT;
		offset = HWIO_LPASS_LPAIF_WRDMA_CTLa_OFFS(dma_idx);
	}
	ipq_lpass_reg_update(lpaif_base + offset, mask, value, 0);
}
EXPORT_SYMBOL(ipq_lpass_enable_dma_channel);

void ipq_lpass_disable_dma_channel(void __iomem *lpaif_base, uint32_t dma_idx,
						uint32_t dma_dir)
{
	uint32_t mask, value, offset;

	if (LPASS_HW_DMA_SINK == dma_dir) {
		mask = HWIO_LPASS_LPAIF_RDDMA_CTLa_ENABLE_ON_FVAL |
			HWIO_LPASS_LPAIF_RDDMA_CTLa_AUDIO_INTF_BMSK;
		value = (HWIO_LPASS_LPAIF_RDDMA_CTLa_ENABLE_OFF_FVAL <<
					HWIO_LPASS_LPAIF_RDDMA_CTLa_ENABLE_SHFT)|
			(HWIO_LPASS_LPAIF_RDDMA_CTLa_AUDIO_INTF_NONE_FVAL <<
				HWIO_LPASS_LPAIF_RDDMA_CTLa_AUDIO_INTF_SHFT);
		offset = HWIO_LPASS_LPAIF_RDDMA_CTLa_OFFS(dma_idx);
	} else {
		mask = HWIO_LPASS_LPAIF_WRDMA_CTLa_ENABLE_ON_FVAL |
				HWIO_LPASS_LPAIF_WRDMA_CTLa_AUDIO_INTF_BMSK;
		value = (HWIO_LPASS_LPAIF_WRDMA_CTLa_ENABLE_OFF_FVAL <<
					HWIO_LPASS_LPAIF_WRDMA_CTLa_ENABLE_SHFT)|
		(HWIO_LPASS_LPAIF_WRDMA_CTLa_AUDIO_INTF_NONE_FVAL <<
			HWIO_LPASS_LPAIF_WRDMA_CTLa_AUDIO_INTF_SHFT);
		offset = HWIO_LPASS_LPAIF_WRDMA_CTLa_OFFS(dma_idx);
	}
	ipq_lpass_reg_update(lpaif_base + offset, mask,value,0);
}
EXPORT_SYMBOL(ipq_lpass_disable_dma_channel);

void ipq_lpass_dma_clear_interrupt_config(void __iomem *lpaif_base,
						uint32_t dma_dir,
							uint32_t dma_idx,
							uint32_t dma_intr_idx)
{
	uint32_t mask;
	uint32_t clear_mask = 0;

	if (LPASS_HW_DMA_SINK == dma_dir) {
		switch (dma_idx) {
		case 0:
			clear_mask = (HWIO_LPASS_LPAIF_IRQ_CLEARa_PER_RDDMA_CH0_BMSK
				| HWIO_LPASS_LPAIF_IRQ_CLEARa_UNDR_RDDMA_CH0_BMSK
				| HWIO_LPASS_LPAIF_IRQ_CLEARa_ERR_RDDMA_CH0_BMSK);
		break;
		case 1:
			clear_mask = (HWIO_LPASS_LPAIF_IRQ_CLEARa_PER_RDDMA_CH1_BMSK
				| HWIO_LPASS_LPAIF_IRQ_CLEARa_UNDR_RDDMA_CH1_BMSK
				| HWIO_LPASS_LPAIF_IRQ_CLEARa_ERR_RDDMA_CH1_BMSK);
		break;
		default:
		break;
		}
	} else {
		switch (dma_idx) {
		case 0:
			clear_mask = (HWIO_LPASS_LPAIF_IRQ_CLEARa_PER_WRDMA_CH0_BMSK
				| HWIO_LPASS_LPAIF_IRQ_CLEARa_OVR_WRDMA_CH0_BMSK
				| HWIO_LPASS_LPAIF_IRQ_CLEARa_ERR_WRDMA_CH0_BMSK);
		break;
		case 1:
			clear_mask = (HWIO_LPASS_LPAIF_IRQ_CLEARa_PER_WRDMA_CH1_BMSK
				| HWIO_LPASS_LPAIF_IRQ_CLEARa_OVR_WRDMA_CH1_BMSK
				| HWIO_LPASS_LPAIF_IRQ_CLEARa_ERR_WRDMA_CH1_BMSK);
		break;
		default:
		break;
		}
	}
	mask = 0x0;

	ipq_lpass_reg_update(HWIO_LPASS_LPAIF_IRQ_CLEARa_ADDR(
				lpaif_base, dma_intr_idx), mask, clear_mask, 1);
}
EXPORT_SYMBOL(ipq_lpass_dma_clear_interrupt_config);

void ipq_lpass_dma_disable_interrupt(void __iomem *lpaif_base, uint32_t dma_dir,
						uint32_t dma_idx,
							uint32_t dma_intr_idx)
{
	uint32_t mask=0;

	if (LPASS_HW_DMA_SINK == dma_dir) {
		switch (dma_idx) {
		case 0:
			mask = (HWIO_LPASS_LPAIF_IRQ_ENa_PER_RDDMA_CH0_BMSK
				| HWIO_LPASS_LPAIF_IRQ_ENa_UNDR_RDDMA_CH0_BMSK
				| HWIO_LPASS_LPAIF_IRQ_ENa_ERR_RDDMA_CH0_BMSK);
		break;
		case 1:
			mask = (HWIO_LPASS_LPAIF_IRQ_ENa_PER_RDDMA_CH1_BMSK
				| HWIO_LPASS_LPAIF_IRQ_ENa_UNDR_RDDMA_CH1_BMSK
				| HWIO_LPASS_LPAIF_IRQ_ENa_ERR_RDDMA_CH1_BMSK);
		break;
		default:
		break;
		}
	} else {
		switch (dma_idx) {
		case 0:
			mask = (HWIO_LPASS_LPAIF_IRQ_ENa_PER_WRDMA_CH0_BMSK
				| HWIO_LPASS_LPAIF_IRQ_ENa_OVR_WRDMA_CH0_BMSK
				| HWIO_LPASS_LPAIF_IRQ_ENa_ERR_WRDMA_CH0_BMSK);
		break;
		case 1:
			mask = (HWIO_LPASS_LPAIF_IRQ_ENa_PER_WRDMA_CH1_BMSK
				| HWIO_LPASS_LPAIF_IRQ_ENa_OVR_WRDMA_CH1_BMSK
				| HWIO_LPASS_LPAIF_IRQ_ENa_ERR_WRDMA_CH1_BMSK);
		break;
		default:
		break;
		}
	}

	ipq_lpass_reg_update(HWIO_LPASS_LPAIF_IRQ_ENa_ADDR(
				lpaif_base, dma_intr_idx),
				mask, 0x0, 0);
}
EXPORT_SYMBOL(ipq_lpass_dma_disable_interrupt);

void ipq_lpass_dma_enable_interrupt(void __iomem *lpaif_base,
						uint32_t dma_dir,
						uint32_t dma_idx,
						uint32_t dma_intr_idx)
{
	uint32_t mask = 0;

	if (LPASS_HW_DMA_SINK == dma_dir) {
		switch (dma_idx) {
		case 0:
			mask = (HWIO_LPASS_LPAIF_IRQ_ENa_PER_RDDMA_CH0_BMSK
				| HWIO_LPASS_LPAIF_IRQ_ENa_UNDR_RDDMA_CH0_BMSK
				| HWIO_LPASS_LPAIF_IRQ_ENa_ERR_RDDMA_CH0_BMSK);
		break;
		case 1:
			mask = (HWIO_LPASS_LPAIF_IRQ_ENa_PER_RDDMA_CH1_BMSK
				| HWIO_LPASS_LPAIF_IRQ_ENa_UNDR_RDDMA_CH1_BMSK
				| HWIO_LPASS_LPAIF_IRQ_ENa_ERR_RDDMA_CH1_BMSK);
		break;
		default:
		break;
		}
	} else {
		switch (dma_idx) {
		case 0:
			mask = (HWIO_LPASS_LPAIF_IRQ_ENa_PER_WRDMA_CH0_BMSK
				| HWIO_LPASS_LPAIF_IRQ_ENa_OVR_WRDMA_CH0_BMSK
				| HWIO_LPASS_LPAIF_IRQ_ENa_ERR_WRDMA_CH0_BMSK);
		break;
		case 1:
			mask = (HWIO_LPASS_LPAIF_IRQ_ENa_PER_WRDMA_CH1_BMSK
				| HWIO_LPASS_LPAIF_IRQ_ENa_OVR_WRDMA_CH1_BMSK
				| HWIO_LPASS_LPAIF_IRQ_ENa_ERR_WRDMA_CH1_BMSK);
		break;
		default:
		break;
		}
	}

	ipq_lpass_reg_update(HWIO_LPASS_LPAIF_IRQ_ENa_ADDR(
				lpaif_base, dma_intr_idx), mask, mask, 0);
}
EXPORT_SYMBOL(ipq_lpass_dma_enable_interrupt);

void ipq_lpass_get_dma_fifo_count(void __iomem *lpaif_base,
					uint32_t *fifo_cnt_ptr,
					uint32_t dma_dir, uint32_t dma_idx)
{
	uint32_t fifo_count = 0;
	if (LPASS_HW_DMA_SINK == dma_dir) {
		fifo_count = readl(lpaif_base +
				HWIO_LPASS_LPAIF_RDDMA_PERa_OFFS(dma_idx));
		*fifo_cnt_ptr = (fifo_count &
			HWIO_LPASS_LPAIF_RDDMA_PERa_PERIOD_FIFO_BMSK) >>
				HWIO_LPASS_LPAIF_RDDMA_PERa_PERIOD_FIFO_SHFT;
	} else {
		fifo_count = readl(lpaif_base +
				HWIO_LPASS_LPAIF_WRDMA_PERa_OFFS(dma_idx));
		*fifo_cnt_ptr = (fifo_count &
			HWIO_LPASS_LPAIF_WRDMA_PERa_PERIOD_FIFO_BMSK) >>
				HWIO_LPASS_LPAIF_WRDMA_PERa_PERIOD_FIFO_SHFT;
	}
}
EXPORT_SYMBOL(ipq_lpass_get_dma_fifo_count);

static void ipq_lpass_dma_reset_enable(void __iomem *lpaif_base,
					uint32_t dma_dir, uint32_t dma_idx)
{
	uint32_t mask, value, offset;

	if (LPASS_HW_DMA_SINK == dma_dir) {
		mask = HWIO_LPASS_LPAIF_RDDMA_CTLa_RESET_BMSK;
		value = HWIO_LPASS_LPAIF_RDDMA_CTLa_RESET_ENABLE_FVAL <<
				HWIO_LPASS_LPAIF_RDDMA_CTLa_RESET_SHFT;
		offset = HWIO_LPASS_LPAIF_RDDMA_CTLa_OFFS(dma_idx);
	} else {
		mask = HWIO_LPASS_LPAIF_WRDMA_CTLa_RESET_BMSK;
		value = HWIO_LPASS_LPAIF_WRDMA_CTLa_RESET_ENABLE_FVAL <<
				HWIO_LPASS_LPAIF_WRDMA_CTLa_RESET_SHFT;
		offset = HWIO_LPASS_LPAIF_WRDMA_CTLa_OFFS(dma_idx);
	}
	ipq_lpass_reg_update(lpaif_base + offset, mask, value, true);
}

static void ipq_lpass_dma_reset_release(void __iomem *lpaif_base,
					uint32_t dma_dir, uint32_t dma_idx)
{
	uint32_t mask, value, offset, reg_val, count = 20;

	if (LPASS_HW_DMA_SINK == dma_dir) {
		mask = HWIO_LPASS_LPAIF_RDDMA_CTLa_RESET_BMSK;
		value = HWIO_LPASS_LPAIF_RDDMA_CTLa_RESET_DISABLE_FVAL <<
				HWIO_LPASS_LPAIF_RDDMA_CTLa_RESET_SHFT;
		offset = HWIO_LPASS_LPAIF_RDDMA_CTLa_OFFS(dma_idx);
	} else {
		mask = HWIO_LPASS_LPAIF_WRDMA_CTLa_RESET_BMSK;
		value = HWIO_LPASS_LPAIF_WRDMA_CTLa_RESET_DISABLE_FVAL <<
				HWIO_LPASS_LPAIF_WRDMA_CTLa_RESET_SHFT;
		offset = HWIO_LPASS_LPAIF_WRDMA_CTLa_OFFS(dma_idx);
	}
	ipq_lpass_reg_update(lpaif_base + offset, mask, value, true);
	reg_val = readl(lpaif_base + offset);

	while ((reg_val & BIT(31)) && count != 0) {
		ipq_lpass_reg_update(lpaif_base + offset, mask, value, true);
		reg_val = readl(lpaif_base + offset);
		count--;
		mdelay(1);
	}
}

static void ipq_lpass_dma_config_channel_sink(struct lpass_dma_config *config)
{
	uint32_t mask, value;

	mask = HWIO_LPASS_LPAIF_RDDMA_BASEa_BASE_ADDR_BMSK;
	value = (config->buffer_start_addr >>
			HWIO_LPASS_LPAIF_RDDMA_BASEa_BASE_ADDR_SHFT)
				<< HWIO_LPASS_LPAIF_RDDMA_BASEa_BASE_ADDR_SHFT;

	ipq_lpass_reg_update(HWIO_LPASS_LPAIF_RDDMA_BASEa_ADDR(
			config->lpaif_base,config->idx), mask, value, 0);

	mask = HWIO_LPASS_LPAIF_RDDMA_BUFF_LENa_LENGTH_BMSK;
	value = (config->buffer_len-1) ;

	ipq_lpass_reg_update(HWIO_LPASS_LPAIF_RDDMA_BUFF_LENa_ADDR(
			config->lpaif_base,config->idx), mask, value, 0);

	mask = HWIO_LPASS_LPAIF_RDDMA_PER_LENa_LENGTH_BMSK;
	value = (config->dma_int_per_cnt-1) <<
				HWIO_LPASS_LPAIF_RDDMA_PER_LENa_LENGTH_SHFT;

	ipq_lpass_reg_update(HWIO_LPASS_LPAIF_RDDMA_PER_LENa_ADDR(
			config->lpaif_base,config->idx), mask, value, 0);

	mask = HWIO_LPASS_LPAIF_RDDMA_CTLa_BURST_EN_BMSK |
		HWIO_LPASS_LPAIF_RDDMA_CTLa_AUDIO_INTF_BMSK |
		HWIO_LPASS_LPAIF_RDDMA_CTLa_FIFO_WATERMRK_BMSK |
		HWIO_LPASS_LPAIF_RDDMA_CTLa_WPSCNT_BMSK |
		HWIO_LPASS_LPAIF_RDDMA_CTLa_DYNAMIC_CLOCK_BMSK |
		HWIO_LPASS_LPAIF_RDDMA_CTLa_BURST8_EN_BMSK |
		HWIO_LPASS_LPAIF_RDDMA_CTLa_BURST16_EN_BMSK;

	value = (config->ifconfig) <<
			HWIO_LPASS_LPAIF_RDDMA_CTLa_AUDIO_INTF_SHFT;

	if  (ipq_hw == IPQ9574 || ipq_hw == IPQ5332) {
		mask |= HWIO_LPASS_LPAIF_RDDMA_CTLa_ENABLE_BMSK;
		value |= (0x1 << HWIO_LPASS_LPAIF_RDDMA_CTLa_ENABLE_SHFT);
	}

	if (config->watermark)
		value |=((config->watermark-1) <<
				HWIO_LPASS_LPAIF_RDDMA_CTLa_FIFO_WATERMRK_SHFT);

	value |= (0x1 << HWIO_LPASS_LPAIF_RDDMA_CTLa_DYNAMIC_CLOCK_SHFT);

	if ((HWIO_LPASS_LPAIF_RDDMA_CTLa_FIFO_WATERMRK_ENUM_8_FVAL + 1) <
		config->dma_int_per_cnt) {
	/*
	 * enable BURST basing on burst_size
	 */
		switch (config->burst_size) {
		case 4:
		case 8:
		case 16:
			value |= (HWIO_LPASS_LPAIF_RDDMA_CTLa_BURST_EN_INCR4_FVAL <<
					HWIO_LPASS_LPAIF_RDDMA_CTLa_BURST_EN_SHFT);
		break;
		case 1:
			value |= (HWIO_LPASS_LPAIF_RDDMA_CTLa_BURST_EN_SINGLE_FVAL <<
					HWIO_LPASS_LPAIF_RDDMA_CTLa_BURST_EN_SHFT);
		break;
		default:
		break;
		}
	} else {
		value |= (HWIO_LPASS_LPAIF_RDDMA_CTLa_BURST_EN_SINGLE_FVAL <<
					HWIO_LPASS_LPAIF_RDDMA_CTLa_BURST_EN_SHFT);
	}

	if (config->wps_count)
		value |=  ((config->wps_count - 1) <<
			HWIO_LPASS_LPAIF_RDDMA_CTLa_WPSCNT_SHFT);

	value |= (config->burst8_en) <<
			HWIO_LPASS_LPAIF_RDDMA_CTLa_BURST8_EN_SHFT;

	value |= (config->burst16_en) <<
			HWIO_LPASS_LPAIF_RDDMA_CTLa_BURST16_EN_SHFT;

	ipq_lpass_reg_update(HWIO_LPASS_LPAIF_RDDMA_CTLa_ADDR(
			config->lpaif_base,config->idx), mask, value, true);
}

static void ipq_lpass_dma_config_channel_source(struct lpass_dma_config *config)
{
	uint32_t mask, value;

	mask = HWIO_LPASS_LPAIF_WRDMA_BASEa_BASE_ADDR_BMSK;
	value = (config->buffer_start_addr >>
			HWIO_LPASS_LPAIF_WRDMA_BASEa_BASE_ADDR_SHFT)
				<< HWIO_LPASS_LPAIF_WRDMA_BASEa_BASE_ADDR_SHFT;

	ipq_lpass_reg_update(HWIO_LPASS_LPAIF_WRDMA_BASEa_ADDR(
			config->lpaif_base,config->idx), mask, value, 0);

	mask = HWIO_LPASS_LPAIF_WRDMA_BUFF_LENa_LENGTH_BMSK;
	value = (config->buffer_len-1);

	ipq_lpass_reg_update(HWIO_LPASS_LPAIF_WRDMA_BUFF_LENa_ADDR(
			config->lpaif_base,config->idx), mask, value, 0);

	mask = HWIO_LPASS_LPAIF_WRDMA_PER_LENa_LENGTH_BMSK;
	value=(config->dma_int_per_cnt-1) <<
			HWIO_LPASS_LPAIF_WRDMA_PER_LENa_LENGTH_SHFT;

	ipq_lpass_reg_update(HWIO_LPASS_LPAIF_WRDMA_PER_LENa_ADDR(
			config->lpaif_base,config->idx), mask, value, 0);

	mask = HWIO_LPASS_LPAIF_WRDMA_CTLa_BURST_EN_BMSK |
		HWIO_LPASS_LPAIF_WRDMA_CTLa_AUDIO_INTF_BMSK |
		HWIO_LPASS_LPAIF_WRDMA_CTLa_FIFO_WATERMRK_BMSK |
		HWIO_LPASS_LPAIF_WRDMA_CTLa_WPSCNT_BMSK |
		HWIO_LPASS_LPAIF_WRDMA_CTLa_DYNAMIC_CLOCK_BMSK |
		HWIO_LPASS_LPAIF_WRDMA_CTLa_BURST8_EN_BMSK |
		HWIO_LPASS_LPAIF_WRDMA_CTLa_BURST16_EN_BMSK;

	value = (config->ifconfig) <<
			HWIO_LPASS_LPAIF_WRDMA_CTLa_AUDIO_INTF_SHFT;

	if ( ipq_hw == IPQ9574 || ipq_hw == IPQ5332) {
		mask |= HWIO_LPASS_LPAIF_WRDMA_CTLa_ENABLE_BMSK;
		value |= (0x1 << HWIO_LPASS_LPAIF_WRDMA_CTLa_ENABLE_SHFT);
	}
	if (config->watermark)
		value |=((config->watermark-1) <<
				HWIO_LPASS_LPAIF_WRDMA_CTLa_FIFO_WATERMRK_SHFT);

	value |= (0x1 << HWIO_LPASS_LPAIF_WRDMA_CTLa_DYNAMIC_CLOCK_SHFT);

	if ((HWIO_LPASS_LPAIF_WRDMA_CTLa_FIFO_WATERMRK_ENUM_8_FVAL + 1) <
		config->dma_int_per_cnt) {
	/*
	 * enable BURST basing on burst_size
	 */
		switch (config->burst_size) {
		case 4:
		case 8:
		case 16:
			value |= (HWIO_LPASS_LPAIF_WRDMA_CTLa_BURST_EN_INCR4_FVAL <<
					HWIO_LPASS_LPAIF_WRDMA_CTLa_BURST_EN_SHFT);
		break;
		case 1:
			value |= (HWIO_LPASS_LPAIF_WRDMA_CTLa_BURST_EN_SINGLE_FVAL <<
					HWIO_LPASS_LPAIF_WRDMA_CTLa_BURST_EN_SHFT);
		break;
		default:
		break;
		}
	} else {
		value |= (HWIO_LPASS_LPAIF_WRDMA_CTLa_BURST_EN_SINGLE_FVAL <<
					HWIO_LPASS_LPAIF_WRDMA_CTLa_BURST_EN_SHFT);
	}

	if (config->wps_count)
		value |=  ((config->wps_count - 1) <<
				HWIO_LPASS_LPAIF_WRDMA_CTLa_WPSCNT_SHFT);

	value |= (config->burst8_en) <<
			HWIO_LPASS_LPAIF_WRDMA_CTLa_BURST8_EN_SHFT;

	value |= (config->burst16_en) <<
			HWIO_LPASS_LPAIF_WRDMA_CTLa_BURST16_EN_SHFT;

	ipq_lpass_reg_update(HWIO_LPASS_LPAIF_WRDMA_CTLa_ADDR(
			config->lpaif_base,config->idx), mask, value, true);
}

void ipq_lpass_config_dma_channel(struct lpass_dma_config *config)
{
	ipq_lpass_dma_reset_enable(config->lpaif_base,
					config->dir, config->idx);

	ipq_lpass_dma_reset_release(config->lpaif_base,
					config->dir, config->idx);

	if (LPASS_HW_DMA_SINK == config->dir) {
		ipq_lpass_dma_config_channel_sink(config);
	} else {
		ipq_lpass_dma_config_channel_source(config);
	}

}
EXPORT_SYMBOL(ipq_lpass_config_dma_channel);

void ipq_lpass_dma_read_interrupt_status(void __iomem *lpaif_base,uint32_t dma_intr_idx,uint32_t *status)
{
	uint32_t read_status;
	read_status = readl(HWIO_LPASS_LPAIF_IRQ_STATa_ADDR(lpaif_base,
					dma_intr_idx));
	*status = read_status;
}
EXPORT_SYMBOL(ipq_lpass_dma_read_interrupt_status);

void ipq_lpass_dma_clear_interrupt(void __iomem *lpaif_base,
					uint32_t dma_intr_idx, uint32_t value)
{
	uint32_t mask;

	mask = value;

	ipq_lpass_reg_update(HWIO_LPASS_LPAIF_IRQ_CLEARa_ADDR(lpaif_base,
				dma_intr_idx), mask, value, 1);
}
EXPORT_SYMBOL(ipq_lpass_dma_clear_interrupt);

void ipq_lpass_dma_get_curr_addr(void __iomem *lpaif_base,
			uint32_t dma_idx, uint32_t dma_dir, uint32_t *curr_addr)
{
	if (LPASS_HW_DMA_SINK == dma_dir) {
		*curr_addr = readl(HWIO_LPASS_LPAIF_RDDMA_CURR_ADDRa_ADDR(
					lpaif_base, dma_idx));
	} else {
		*curr_addr = readl(HWIO_LPASS_LPAIF_WRDMA_CURR_ADDRa_ADDR(
					lpaif_base, dma_idx));
	}
}
EXPORT_SYMBOL(ipq_lpass_dma_get_curr_addr);

void ipq_lpass_dma_reset(void __iomem *lpaif_base,uint32_t dma_idx,uint32_t dma_dir)
{
	uint32_t mask,value;

	if (LPASS_HW_DMA_SINK == dma_dir) {
		mask = HWIO_LPASS_LPAIF_RDDMA_CTLa_RMSK;
		value =  HWIO_LPASS_LPAIF_RDDMA_CTLa_POR;
		ipq_lpass_reg_update(HWIO_LPASS_LPAIF_RDDMA_CTLa_ADDR(
			lpaif_base, dma_idx), mask, value, 0);

	} else {
		mask = HWIO_LPASS_LPAIF_WRDMA_CTLa_RMSK;
		value = HWIO_LPASS_LPAIF_WRDMA_CTLa_POR;
		ipq_lpass_reg_update(HWIO_LPASS_LPAIF_WRDMA_CTLa_ADDR(
			lpaif_base, dma_idx), mask, value, 0);
	}
}
EXPORT_SYMBOL(ipq_lpass_dma_reset);

static void ipq_lpass_setclk_lpaif_pri(uint32_t m, uint32_t n, uint32_t d,
					uint32_t clkdiv, uint32_t srcdiv,
					uint32_t src, uint32_t mode)
{
	writel(m, HWIO_LPASS_LPAIF_PRI_M_ADDR(sg_ipq_lpass_base));
	writel(n, HWIO_LPASS_LPAIF_PRI_N_ADDR(sg_ipq_lpass_base));
	writel(d, HWIO_LPASS_LPAIF_PRI_D_ADDR(sg_ipq_lpass_base));
	ipq_lpass_cc_update(HWIO_LPASS_LPAIF_PRI_CFG_RCGR_ADDR(
			sg_ipq_lpass_base),
			(mode << HWIO_LPASS_LPAIF_PRI_CFG_RCGR_MODE_SHFT) |
			(src << HWIO_LPASS_LPAIF_PRI_CFG_RCGR_SRC_SEL_SHFT) |
			(srcdiv << HWIO_LPASS_LPAIF_PRI_CFG_RCGR_SRC_DIV_SHFT),
			0);
	ipq_lpass_cc_update(HWIO_LPASS_AUDIO_CORE_LPAIF_PRI_IBIT_CBCR_ADDR(
			sg_ipq_lpass_base),
			clkdiv << HWIO_LPASS_AUDIO_CORE_LPAIF_PRI_IBIT_CBCR_CLK_DIV_SHFT,
			0);
	ipq_lpass_cc_update(HWIO_LPASS_LPAIF_PRI_CMD_RCGR_ADDR(
			sg_ipq_lpass_base), 0x0, 1);

}

static void ipq_lpass_setclk_lpaif_sec(uint32_t m, uint32_t n, uint32_t d,
					uint32_t clkdiv, uint32_t srcdiv,
						uint32_t src, uint32_t mode)
{
	writel(m, HWIO_LPASS_LPAIF_SEC_M_ADDR(sg_ipq_lpass_base));
	writel(n, HWIO_LPASS_LPAIF_SEC_N_ADDR(sg_ipq_lpass_base));
	writel(d, HWIO_LPASS_LPAIF_SEC_D_ADDR(sg_ipq_lpass_base));
	ipq_lpass_cc_update(HWIO_LPASS_LPAIF_SEC_CFG_RCGR_ADDR(
			sg_ipq_lpass_base),
			(mode << HWIO_LPASS_LPAIF_SEC_CFG_RCGR_MODE_SHFT) |
			(src << HWIO_LPASS_LPAIF_SEC_CFG_RCGR_SRC_SEL_SHFT) |
			(srcdiv << HWIO_LPASS_LPAIF_SEC_CFG_RCGR_SRC_DIV_SHFT),
			0);
	ipq_lpass_cc_update(HWIO_LPASS_AUDIO_CORE_LPAIF_SEC_IBIT_CBCR_ADDR(
			sg_ipq_lpass_base),
			clkdiv << HWIO_LPASS_AUDIO_CORE_LPAIF_SEC_IBIT_CBCR_CLK_DIV_SHFT,
			0);
	ipq_lpass_cc_update(HWIO_LPASS_LPAIF_SEC_CMD_RCGR_ADDR(
			sg_ipq_lpass_base), 0x0, 1);
}

uint32_t ipq_lpass_set_clk_rate(uint32_t intf, uint32_t clk)
{
	uint32_t M, N, SRC;
	uint32_t D, preDiv=0;

	SRC = SRC_HB_INT_AUDPLL_AUX1;

	switch (clk) {
	case 64000:	//0.064 MHz
		M = 1;
		N = 0xf880;
		D = 0xf87f;
	break;
	case 88200:	//0.0882 MHz
		M = 1;
		N = 0xfb00;
		D = 0xfaff;
	break;
	case 96000:	//0.096 MHz
		M = 1;
		N = 0xfb00;
		D = 0xfaff;
	break;
	case 128000:	//0.128 MHz
		M = 1;
		N = 0xfc40;
		D = 0xfc3f;
	break;
	case 160000:   //0.160 MHz
		M = 1;      //0x01;
		N = 0xfd00;
		D = 0xfcff;
	break;
	case 176400:   //0.1764 MHz
		M = 1;      //0x1;
		N = 0xfd80;
		D = 0xfd7f;
	break;
	case 192000:   //0.192 MHz
		M = 1;      //0x01;
		N = 0xfd80;
		D = 0xfd7f;
	break;
	case 200000:   //0.200 MHz
		M = 5;      //0x5;
		N = 0xf404;
		D = 0xf3ff;
	break;
	case 256000:   //0.256 MHz
		M = 1;      //0x01;
		N = 0xfe20;
		D = 0xfe1f;
	break;
	case 320000:   //0.320 MHz
		M = 1;      //0x01;
		N = 0xfe80;
		D = 0xfe7f;
	break;
	case 352800:   //0.3528 MHz
		M = 1;   //0x1;
		N = 0xfec0;
		D = 0xfebf;
	break;
	case 384000:   //0.384 MHz
		M = 1;      //0x01;
		N = 0xfec0;
		D = 0xfebf;
	break;
	case 400000:   //0.400 MHz
		M = 5;      //0x5;
		N = 0xfa04;
		D = 0xf9ff;
	break;
	case 512000:   //0.512 MHz
		M = 1;      //0x01;
		N = 0xff10;
		D = 0xff0f;
	break;
	case 529200:   //0.5292 MHz
		M = 3;      //0x3;
		N = 0xfd82;
		D = 0xfd7f;
	break;
	case 576000:   //0.576 MHz
		M = 1;     //0x1
		N = 0xff3c;
		D = 0xff3b;
	break;
	case 640000:   //0.640 MHz
		M = 1;      //0x01;
		N = 0xff40;
		D = 0xff3f;
	break;
	case 705600:   //0.7056 MHz
		M = 1;      //0x1;
		N = 0xff60;
		D = 0xff5f;
	break;
	case 768000:   //0.768 MHz
		M = 1;      //0x1;
		N = 0xff60;
		D = 0xff5f;
	break;
	case 800000:   //0.800 MHz
		M = 5;      //0x5;
		N = 0xfd04;
		D = 0xfcff;
	break;
	case 1024000:  //1.024 MHz
		M = 1;      //0x1;
		N = 0xff88;
		D = 0xff87;
	break;
	case 1058400://1.0584 MHz
		M = 3;      //0x3;
		N = 0xfec2;
		D = 0xfebf;
	break;
	case 1152000://1.152 MHz
		M = 1;      //0x1;
		N = 0xff9e;
		D = 0xff9d;
	break;
	case 1280000:  //1.280 MHz
		M = 1;      //0x01;
		N = 0xffa0;
		D = 0xff9f;
	break;
	case 1411200:  //1.4112 MHz
		M = 1;      //0x1;
		N = 0xffb0;
		D = 0xffaf;
	break;
	case 1536000:  //1.536 MHz
		M = 1;      //0x1;
		N = 0xffb0;
		D = 0xffaf;
	break;
	case 1600000:  //1.600 MH
		M = 5;      //0x5;
		N = 0xfe84;
		D = 0xfe7f;
	break;
	case 2048000:  //2.048 MHz
		M = 1;      //0x1;
		N = 0xffc4;
		D = 0xffc3;
	break;
	case 2116800:  //2.1168 MHz
		M = 3;      //0x3;
		N = 0xff62;
		D = 0xff5f;
	break;
	case 2304000:  //2.304 MHz
		M = 1;      //0x1;
		N = 0xffcf;
		D = 0xffce;
	break;
	case 2560000:  //2.560 MHz
		M = 1;      //0x01;
		N = 0xffd0;
		D = 0xffcf;
	break;
	case 2822400:  //2.8224 MHz
		M = 1;      //0x1;
		N = 0xffd8;
		D = 0xffd7;
	break;
	case 3072000:  //3.072 MHz
		M = 1;      //0x1;
		N = 0xffd8;
		D = 0xffd7;
	break;
	case 3200000:  //3.200 MHz
		M = 5;      //0x5;
		N = 0xff44;
		D = 0xff3f;
	break;
	case 4096000:  //4.096 MHz
		M = 1;      //0x1;
		N = 0xffe2;
		D = 0xffe1;
	break;
	case 4233600:  //4.2336 MHz
		M = 3;      //0x3;
		N = 0xffb2;
		D = 0xffaf;
	break;
	case 4608000:  //4.608 MHz
		M = 2;      //0x2;
		N = 0xffd0;
		D = 0xffce;
	break;
	case 5120000:  //5.120 MHz
		M = 1;      //0x01;
		N = 0xffe8;
		D = 0xffe7;
	break;
	case 5644800:  //5.6448 MHz
		M = 1;      //0x1;
		N = 0xffec;
		D = 0xffeb;
	break;
	case 6144000:  //6.144 MHz
		M = 1;      //0x1;
		N = 0xffec;
		D = 0xffeb;
	break;
	case 6400000:  //6.400 MHz
		M = 5;      //0x5;
		N = 0xffa4;
		D = 0xff9f;
	break;
	case 8192000:  //8.192 MHz
		M = 1;      //0x1;
		N = 0xfff1;
		D = 0xfff0;
	break;
	case 8467200:  //8.4672 MHz
		M = 3;      //0x3;
		N = 0xffda;
		D = 0xffd7;
	break;
	case 9216000:  //9.216 MHz
		SRC = SRC_HB_INT_DIGPLL_AUX1; /* Digital PLL divided by 5 -> 122.88MHz */
		M = 3;      //0x3;
		N = 0xffda;
		D = 0xffd7;
	break;
	case 11289600: //11.2896 MHz
		M = 1;      //0x1;
		N = 0xfff6;
		D = 0xfff5;
	break;
	case 12288000: //12.288 MHz
		M = 1;      //0x1;
		N = 0xfff6;
		D = 0xfff5;
	break;
	case 16384000: //16.384 MHz
		SRC = SRC_HB_INT_DIGPLL_AUX1; /* Digital PLL divided by 5 -> 122.88MHz */
		M = 2;      //0x2;
		N = 0xfff2;
		D = 0xfff0;
	break;
	case 18432000: //18.432 MHz
	SRC = SRC_HB_INT_DIGPLL_AUX1; /* Digital PLL divided by 5 -> 122.88MHz */
		M = 3;      //0x3;
		N = 0xffee;
		D = 0xffeb;
	break;
	case 22579200: //22.5792 MHz
		M = 1;      //0x1;
		N = 0xfffb;
		D = 0xfffa;
	break;
	case 24576000: //24.576 MHz
		M = 1;      //0x1;
		N = 0xfffb;
		D = 0xfffa;
	break;
	case 36864000: //36.864 MHz
		SRC = SRC_HB_INT_DIGPLL_AUX1;
		M = 2;      //0x2;
		N = 0xfffa;
		D = 0xfff8;
	break;
	case 49152000: //49.152 MHz
		M = 2;      //0x2;
		N = 0xfffc;
		D = 0xfffa;
	break;
	case 73728000: //73.728 MHz
		SRC = SRC_HB_INT_DIGPLL_AUX1;
		M = 3;      //0x3;
		N = 0xfffd;
		D = 0xfffb;
	break;
	default:
		pr_err("unsupported clock frequency = %dHz\n",clk);
		return -EINVAL;
	break;
	}

	switch (intf) {
	case INTERFACE_PRIMARY:
		ipq_lpass_setclk_lpaif_pri(M, N, D, 0, preDiv, SRC, 2);
	break;
	case INTERFACE_SECONDARY:
		ipq_lpass_setclk_lpaif_sec(M, N, D, 0, preDiv, SRC, 2);
	break;
	default:
		pr_err("unsupported device ID \n");
		return -EINVAL;
	break;
	}
	return 0;
}
EXPORT_SYMBOL(ipq_lpass_set_clk_rate);

void ipq_lpass_lpaif_muxsetup(uint32_t intf, uint32_t mode, uint32_t val, uint32_t src)
{
	switch (intf) {
	case INTERFACE_PRIMARY:
		writel(val, HWIO_LPASS_AUDIO_CORE_LPAIF_PRI_CLK_INV_ADDR(
				sg_ipq_lpass_base));
		writel(src, HWIO_LPASS_AUDIO_CORE_LPAIF_PRI_MODE_MUXSEL_ADDR(
				sg_ipq_lpass_base));
	break;
	case INTERFACE_SECONDARY:
		writel(val, HWIO_LPASS_AUDIO_CORE_LPAIF_SEC_CLK_INV_ADDR(
				sg_ipq_lpass_base));
		writel(src, HWIO_LPASS_AUDIO_CORE_LPAIF_SEC_MODE_MUXSEL_ADDR(
				sg_ipq_lpass_base));
	default:
	break;
	}
}
EXPORT_SYMBOL(ipq_lpass_lpaif_muxsetup);

static uint32_t ipq_lpass_calculatespark_pll_vco_v1(struct ipq_lpass_pll p)
{
	uint32_t vco_sel = 0;
	uint32_t temp_l;

	if (p.pre_div != 0) {
		temp_l = p.l / p.pre_div;
	} else {
		temp_l = p.l;
	}

	if (temp_l > 29 && temp_l < 59) {
		vco_sel = 0;
	} else if (temp_l > 21 && temp_l < 44) {
		vco_sel = 1;
	} else if (temp_l > 14 && temp_l < 29) {
		vco_sel = 2;
	} else if (temp_l > 6 && temp_l < 14) {
		vco_sel = 3;
	} else {
		temp_l = 0;
	}

	return vco_sel;
}

static uint32_t ipq_lpass_calculatespark_pll_vco_v2(struct ipq_lpass_pll p)
{
	uint32_t vco_sel = 0;
	uint32_t temp_l;

	if (p.pre_div != 0) {
		temp_l = p.l / p.pre_div;
	} else {
		temp_l = p.l;
	}

	if (temp_l > 42 && temp_l < 83) {
		vco_sel = 0;
	} else if (temp_l > 31 && temp_l < 63) {
		vco_sel = 1;
	} else if (temp_l > 20 && temp_l < 42) {
		vco_sel = 2;
	} else if (temp_l > 10 && temp_l < 21) {
		vco_sel = 3;
	} else {
		temp_l = 0;
	}

	return vco_sel;
}

static void ipq_lpass_setup_audio_pll(struct ipq_lpass_pll pll)
{
	uint32_t i		= 0;
	uint32_t value		= 0;
	uint32_t alpha_en 	= 0;
	uint32_t vco_sel 	= 0;
	uint32_t reg_val 	= 0;

	if (pll.alpha!=0) {
		alpha_en = 1;
	}

	if(!(ipq_hw == IPQ5332))
		vco_sel = ipq_lpass_calculatespark_pll_vco_v1(pll);

	value = readl(HWIO_LPASS_LPAAUDIO_PLL_MODE_ADDR(sg_ipq_lpass_base));

	if (value == 0x0) {
		writel(0x0,
			HWIO_LPASS_LPAAUDIO_PLL_REF_CLK_SRC_SEL_ADDR(
				sg_ipq_lpass_base));
		ipq_lpass_cc_update(HWIO_LPASS_LPAAUDIO_PLL_USER_CTL_U_ADDR(
			sg_ipq_lpass_base),
			(1 << HWIO_LPASS_LPAAUDIO_PLL_USER_CTL_U_LOCK_DET_SHFT),
			0);
		ipq_lpass_cc_update(HWIO_LPASS_LPAAUDIO_PLL_L_VAL_ADDR(
					sg_ipq_lpass_base), pll.l, 0);

		writel(pll.alpha, HWIO_LPASS_LPAAUDIO_PLL_ALPHA_VAL_ADDR(
					sg_ipq_lpass_base));

		writel(pll.alpha_u, HWIO_LPASS_LPAAUDIO_PLL_ALPHA_VAL_U_ADDR(
					sg_ipq_lpass_base));

		reg_val = ((vco_sel << HWIO_LPASS_LPAAUDIO_PLL_USER_CTL_VCO_SEL_SHFT) |
				(pll.post_div << HWIO_LPASS_LPAAUDIO_PLL_USER_CTL_POST_DIV_RATIO_SHFT) |
				(alpha_en << HWIO_LPASS_LPAAUDIO_PLL_USER_CTL_ALPHA_EN_SHFT));

		ipq_lpass_cc_update(HWIO_LPASS_LPAAUDIO_PLL_USER_CTL_ADDR(
					sg_ipq_lpass_base), reg_val, 0);

		ipq_lpass_cc_update(HWIO_LPASS_LPAAUDIO_PLL_MODE_ADDR(
					sg_ipq_lpass_base),
					(1 << HWIO_LPASS_LPAAUDIO_PLL_MODE_PLL_BYPASSNL_SHFT),
					0);

		for (i=0; i < pll.pll_reset_wait; i++) {
			mdelay(1);
			value = readl(HWIO_LPASS_LPAAUDIO_PLL_STATUS_ADDR(
					sg_ipq_lpass_base));
		}

		ipq_lpass_cc_update(HWIO_LPASS_LPAAUDIO_PLL_MODE_ADDR(
			sg_ipq_lpass_base),
			(1 << HWIO_LPASS_LPAAUDIO_PLL_MODE_PLL_RESET_N_SHFT),
			0);

		for (i=0; i < pll.pll_lock_wait; i++) {
			mdelay(1);
			value = readl(HWIO_LPASS_LPAAUDIO_PLL_STATUS_ADDR(
						sg_ipq_lpass_base));
		}
	}
}

static void ipq_lpass_Setup_dig_pll(struct ipq_lpass_pll pll)
{
	int i		= 0;
	uint32_t value	= 0;
	uint32_t alpha_en = 0;
	uint32_t vco_sel = 0;
	uint32_t reg_val = 0;
	if (pll.alpha != 0) {
		alpha_en = 1;
	}

	if(ipq_hw == IPQ5332)
		vco_sel = ipq_lpass_calculatespark_pll_vco_v2(pll);
	else
		vco_sel = ipq_lpass_calculatespark_pll_vco_v1(pll);

	value = readl(HWIO_LPASS_LPAAUDIO_DIG_PLL_MODE_ADDR(sg_ipq_lpass_base));

	if (value == 0x0) {
		writel(pll.src,
			HWIO_LPASS_LPAAUDIO_DIG_PLL_REF_CLK_SRC_SEL_ADDR(
			sg_ipq_lpass_base));

		writel(pll.l,
			HWIO_LPASS_LPAAUDIO_DIG_PLL_L_VAL_ADDR(
			sg_ipq_lpass_base));

		writel(pll.alpha,
			HWIO_LPASS_LPAAUDIO_DIG_PLL_ALPHA_VAL_ADDR(
			sg_ipq_lpass_base));

		writel(pll.alpha_u,
			HWIO_LPASS_LPAAUDIO_DIG_PLL_ALPHA_VAL_U_ADDR(
			sg_ipq_lpass_base));

		reg_val = ((vco_sel << HWIO_LPASS_LPAAUDIO_DIG_PLL_USER_CTL_VCO_SEL_SHFT) |
				(pll.post_div << HWIO_LPASS_LPAAUDIO_DIG_PLL_USER_CTL_POST_DIV_RATIO_SHFT) |
				(alpha_en << HWIO_LPASS_LPAAUDIO_DIG_PLL_USER_CTL_ALPHA_EN_SHFT));

		ipq_lpass_cc_update(HWIO_LPASS_LPAAUDIO_DIG_PLL_USER_CTL_ADDR(
					sg_ipq_lpass_base), reg_val, 0);

		if (pll.pll_vote_fsm_ena == 0) {
			ipq_lpass_cc_update(HWIO_LPASS_LPAAUDIO_DIG_PLL_MODE_ADDR(
				sg_ipq_lpass_base),
				(1 << HWIO_LPASS_LPAAUDIO_DIG_PLL_MODE_PLL_BYPASSNL_SHFT),
				0);

			for (i=0; i<pll.pll_reset_wait; i++) {
				value = readl(HWIO_LPASS_LPAAUDIO_DIG_PLL_STATUS_ADDR(sg_ipq_lpass_base));
				mdelay(1);
			}

			ipq_lpass_cc_update(HWIO_LPASS_LPAAUDIO_DIG_PLL_MODE_ADDR(
				sg_ipq_lpass_base),
				(1 << HWIO_LPASS_LPAAUDIO_DIG_PLL_MODE_PLL_UPDATE_SHFT),
				0);

			for (i=0; i<pll.pll_reset_wait; i++) {
				value = readl(HWIO_LPASS_LPAAUDIO_DIG_PLL_STATUS_ADDR(sg_ipq_lpass_base));
				mdelay(1);
			}

			ipq_lpass_cc_update(HWIO_LPASS_LPAAUDIO_DIG_PLL_MODE_ADDR(
				sg_ipq_lpass_base),
				(1 << HWIO_LPASS_LPAAUDIO_DIG_PLL_MODE_PLL_RESET_N_SHFT),
				0);

			for (i=0; i<pll.pll_reset_wait; i++) {
				value = readl(HWIO_LPASS_LPAAUDIO_DIG_PLL_STATUS_ADDR(sg_ipq_lpass_base));
				mdelay(1);
			}
		}
	}
}

static void ipq_lpass_pll_lock_wait(void)
{
	uint32_t count;
	struct ipq_lpass_plllock lock;
	lock.lock_time    = 200;
	lock.value      = 0;
	lock.timer_type   = 1;

	lock.value = readl(HWIO_LPASS_LPAAUDIO_DIG_PLL_MODE_ADDR(sg_ipq_lpass_base))
			& HWIO_LPASS_LPAAUDIO_DIG_PLL_MODE_PLL_LOCK_DET_BMSK;

	count = 0;
	while (lock.value != HWIO_LPASS_LPAAUDIO_DIG_PLL_MODE_PLL_LOCK_DET_BMSK) {
		mdelay(1);
		lock.value = readl(
				HWIO_LPASS_LPAAUDIO_DIG_PLL_MODE_ADDR(
				sg_ipq_lpass_base)) &
			HWIO_LPASS_LPAAUDIO_DIG_PLL_MODE_PLL_LOCK_DET_BMSK;
		if (count == 20)
			break;
		else
			++count;
	}
	if (lock.value != HWIO_LPASS_LPAAUDIO_DIG_PLL_MODE_PLL_LOCK_DET_BMSK)
		pr_info("%s Audio PLL not locked \n", __func__);

	ipq_lpass_cc_update(HWIO_LPASS_LPAAUDIO_DIG_PLL_MODE_ADDR(
			sg_ipq_lpass_base),
			(1 << HWIO_LPASS_LPAAUDIO_DIG_PLL_MODE_PLL_OUTCTRL_SHFT)
			, 0);
	ipq_lpass_cc_update(HWIO_LPASS_LPAAUDIO_DIG_PLL_USER_CTL_ADDR(
		sg_ipq_lpass_base),
		((1 << HWIO_LPASS_LPAAUDIO_DIG_PLL_USER_CTL_PLLOUT_LV_AUX_SHFT) |
		(1 << HWIO_LPASS_LPAAUDIO_DIG_PLL_USER_CTL_PLLOUT_LV_AUX2_SHFT) |
		(1 << HWIO_LPASS_LPAAUDIO_DIG_PLL_USER_CTL_PLLOUT_LV_MAIN_SHFT) |
		(1 << HWIO_LPASS_LPAAUDIO_DIG_PLL_USER_CTL_PLLOUT_LV_EARLY_SHFT)),
		0);

	lock.value = readl(HWIO_LPASS_LPAAUDIO_PLL_MODE_ADDR(sg_ipq_lpass_base)) &
			HWIO_LPASS_LPAAUDIO_PLL_MODE_PLL_LOCK_DET_BMSK;

	count = 0;
	while (lock.value != HWIO_LPASS_LPAAUDIO_PLL_MODE_PLL_LOCK_DET_BMSK) {
		mdelay(1);
		lock.value = readl(HWIO_LPASS_LPAAUDIO_PLL_MODE_ADDR(sg_ipq_lpass_base)) &
			HWIO_LPASS_LPAAUDIO_PLL_MODE_PLL_LOCK_DET_BMSK;
		if (count == 20)
			break;
		else
			++count;
	}
	if (lock.value != HWIO_LPASS_LPAAUDIO_PLL_MODE_PLL_LOCK_DET_BMSK)
		pr_info("%s Digital PLL not locked \n", __func__);

	ipq_lpass_cc_update(HWIO_LPASS_LPAAUDIO_PLL_MODE_ADDR(
			sg_ipq_lpass_base),
			(1 << HWIO_LPASS_LPAAUDIO_PLL_MODE_PLL_OUTCTRL_SHFT)
			, 0);

	ipq_lpass_cc_update(HWIO_LPASS_LPAAUDIO_PLL_USER_CTL_ADDR(
		sg_ipq_lpass_base),
		((1 << HWIO_LPASS_LPAAUDIO_PLL_USER_CTL_PLLOUT_LV_AUX_SHFT) |
		(1 << HWIO_LPASS_LPAAUDIO_PLL_USER_CTL_PLLOUT_LV_AUX2_SHFT) |
		(1 << HWIO_LPASS_LPAAUDIO_PLL_USER_CTL_PLLOUT_LV_MAIN_SHFT) |
		(1 << HWIO_LPASS_LPAAUDIO_PLL_USER_CTL_PLLOUT_LV_EARLY_SHFT)),
		0);
}

static void ipq_lpass_core_pwrctl(void)
{
	uint32_t val, i;

	ipq_lpass_cc_update(HWIO_LPASS_AUDIO_CORE_GDSCR_ADDR(
		sg_ipq_lpass_base),
		(0x5 << HWIO_LPASS_AUDIO_CORE_GDSCR_CLK_DIS_WAIT_SHFT),
		0);

	ipq_lpass_cc_update(HWIO_LPASS_AUDIO_CORE_RESAMPLER_CBCR_ADDR(
		sg_ipq_lpass_base),
		1 << HWIO_LPASS_AUDIO_CORE_RESAMPLER_CBCR_SLEEP_SHFT,
		0);

	ipq_lpass_cc_update(HWIO_LPASS_AUDIO_CORE_LPM_CORE_CBCR_ADDR(
		sg_ipq_lpass_base),
		0x1 << HWIO_LPASS_AUDIO_CORE_LPM_CORE_CBCR_SLEEP_SHFT |
		0x2 << HWIO_LPASS_AUDIO_CORE_LPM_CORE_CBCR_WAKEUP_SHFT,
		0);

	val = readl(HWIO_LPASS_AUDIO_CORE_GDSCR_ADDR(sg_ipq_lpass_base));
	val &= ~(1 << 0);
	writel(val, HWIO_LPASS_AUDIO_CORE_GDSCR_ADDR(sg_ipq_lpass_base));

	for (i = 0; i < 30; ++i) {
		val = readl(HWIO_LPASS_AUDIO_CORE_GDSCR_ADDR(
				sg_ipq_lpass_base));
		if (val & 0x80000000)
			break;
		mdelay(20);
	}
}

static void ipq_lpass_clk_init(void)
{
	struct ipq_lpass_pll lpasspll, lpassdpll;
	uint32_t bus_srcdiv	= 0;
	uint32_t aon_srcdiv	= 0;
	uint32_t res_srcdiv	= 0;
	uint32_t mi2_srcdiv	= 0;
	uint32_t fix_srcdiv	= (ipq_hw == IPQ5332) ? 4 : 0;

/*
 * LPA AUDIO PLL
 * 614.4MHz
*/
	lpasspll.l		= 25;
	lpasspll.post_div	= 0;
	lpasspll.alpha		= 0x99999999;
	lpasspll.alpha_u	= 0x99;
	lpasspll.pre_div	= 0;
	lpasspll.src		= 0; /* LPASS_SRC_CXO */
	lpasspll.app_vote	= 0;
	lpasspll.q6_vote	= 0;
	lpasspll.rpm_vote	= 0;
	lpasspll.pll_reset_wait	= 15;
	lpasspll.pll_lock_wait	= 5;
	lpasspll.pll_vote_fsm_ena = 0;
	lpasspll.pll_type	= 2; /* LPASSPLL2 */

/*
 * DIG PLL
 * 614.4MHz
*/
	lpassdpll.l		= 25;
	lpassdpll.post_div	= 1;
	lpassdpll.alpha		= 0x99999999;
	lpassdpll.alpha_u	= 0x99;
	lpassdpll.pre_div	= 0;
	lpassdpll.src		= 0; /*LPASS_SRC_CXO*/
	lpassdpll.app_vote	= 0;
	lpassdpll.q6_vote	= 0;
	lpassdpll.rpm_vote	= 0;
	lpassdpll.pll_reset_wait = 15;
	lpassdpll.pll_lock_wait	= 5;
	lpassdpll.pll_vote_fsm_ena = 0;
	lpassdpll.pll_type	= 0; /*LPASSPLL0 */

	ipq_lpass_cc_update(HWIO_LPASS_LPAIF_SPKR_CMD_RCGR_ADDR(
			sg_ipq_lpass_base), 0x2, 1);

	ipq_lpass_cc_update(HWIO_LPASS_LPAIF_PRI_CMD_RCGR_ADDR(
			sg_ipq_lpass_base), 0x2, 1);

	ipq_lpass_cc_update(HWIO_LPASS_LPAIF_SEC_CMD_RCGR_ADDR(
			sg_ipq_lpass_base), 0x2, 1);

	ipq_lpass_cc_update(HWIO_LPASS_LPAIF_TER_CMD_RCGR_ADDR(
			sg_ipq_lpass_base), 0x2, 1);

	ipq_lpass_cc_update(HWIO_LPASS_LPAIF_QUAD_CMD_RCGR_ADDR(
			sg_ipq_lpass_base), 0x2, 1);

	ipq_lpass_cc_update(HWIO_LPASS_AON_CMD_RCGR_ADDR(
			sg_ipq_lpass_base), 0x2, 1);

	ipq_lpass_cc_update(HWIO_LPASS_ATIME_CMD_RCGR_ADDR(
			sg_ipq_lpass_base), 0x2, 1);

	ipq_lpass_cc_update(HWIO_LPASS_RESAMPLER_CMD_RCGR_ADDR(
			sg_ipq_lpass_base), 0x2, 1);

	ipq_lpass_cc_update(HWIO_LPASS_LPAIF_PCMOE_CMD_RCGR_ADDR(
			sg_ipq_lpass_base), 0x2, 1);

	ipq_lpass_cc_update(HWIO_LPASS_XO_CMD_RCGR_ADDR(
			sg_ipq_lpass_base), 0x2, 1);

	ipq_lpass_cc_update(HWIO_LPASS_CORE_CMD_RCGR_ADDR(
			sg_ipq_lpass_base), 0x2, 1);

	ipq_lpass_cc_update(HWIO_LPASS_EXT_MCLK0_CMD_RCGR_ADDR(
			sg_ipq_lpass_base), 0x2, 1);

	ipq_lpass_cc_update(HWIO_LPASS_EXT_MCLK1_CMD_RCGR_ADDR(
			sg_ipq_lpass_base), 0x2, 1);

	ipq_lpass_cc_update(HWIO_LPASS_EXT_MCLK2_CMD_RCGR_ADDR(
			sg_ipq_lpass_base), 0x2, 1);

	ipq_lpass_cc_update(HWIO_LPASS_QOS_FIXED_LAT_COUNTER_CMD_RCGR_ADDR(
			sg_ipq_lpass_base), 0x2, 1);

	ipq_lpass_cc_update(HWIO_LPASS_Q6SS_AHBS_AON_CBCR_ADDR(
			sg_ipq_lpass_base), 0x1, 0);

	ipq_lpass_setup_audio_pll(lpasspll);

	ipq_lpass_Setup_dig_pll(lpassdpll);

	ipq_lpass_setclk_lpaif_pri(0x0001,0xFFFB, 0xFFFA, 0, mi2_srcdiv, 6, 2);

	ipq_lpass_setclk_lpaif_sec(0x0001,0xFFFB, 0xFFFA, 0, mi2_srcdiv, 6, 2);


	writel(0x01, HWIO_LPASS_LPAIF_SPKR_M_ADDR(sg_ipq_lpass_base));
	writel(0xFB, HWIO_LPASS_LPAIF_SPKR_N_ADDR(sg_ipq_lpass_base));
	writel(0xFA, HWIO_LPASS_LPAIF_SPKR_D_ADDR(sg_ipq_lpass_base));
	ipq_lpass_cc_update(HWIO_LPASS_LPAIF_SPKR_CFG_RCGR_ADDR(
			sg_ipq_lpass_base),
			(2 << HWIO_LPASS_LPAIF_SPKR_CFG_RCGR_MODE_SHFT) |
			(6 << HWIO_LPASS_LPAIF_SPKR_CFG_RCGR_SRC_SEL_SHFT) |
			(mi2_srcdiv << HWIO_LPASS_LPAIF_SPKR_CFG_RCGR_SRC_DIV_SHFT),
			0);

	ipq_lpass_cc_update(HWIO_LPASS_AUDIO_CORE_LPAIF_CODEC_SPKR_IBIT_CBCR_ADDR(
			sg_ipq_lpass_base),
			0 << HWIO_LPASS_AUDIO_CORE_LPAIF_CODEC_SPKR_IBIT_CBCR_CLK_DIV_SHFT,
			0);

	ipq_lpass_cc_update(HWIO_LPASS_LPAIF_SPKR_CMD_RCGR_ADDR(
			sg_ipq_lpass_base), 0x0, 1);

	writel(0x01, HWIO_LPASS_LPAIF_PCMOE_M_ADDR(sg_ipq_lpass_base));
	writel(0xFB, HWIO_LPASS_LPAIF_PCMOE_N_ADDR(sg_ipq_lpass_base));
	writel(0xFA, HWIO_LPASS_LPAIF_PCMOE_D_ADDR(sg_ipq_lpass_base));
	ipq_lpass_cc_update(HWIO_LPASS_LPAIF_PCMOE_CFG_RCGR_ADDR(
			sg_ipq_lpass_base),
			(0 << HWIO_LPASS_LPAIF_PCMOE_CFG_RCGR_MODE_SHFT) |
			(6 << HWIO_LPASS_LPAIF_PCMOE_CFG_RCGR_SRC_SEL_SHFT) |
			(mi2_srcdiv << HWIO_LPASS_LPAIF_PCMOE_CFG_RCGR_SRC_DIV_SHFT),
			0);
	ipq_lpass_cc_update(HWIO_LPASS_LPAIF_PCMOE_CMD_RCGR_ADDR(
			sg_ipq_lpass_base), 0x0, 1);

	writel((ipq_hw == IPQ5332) ? 0x0 : 0x01, HWIO_LPASS_QOS_FIXED_LAT_COUNTER_M_ADDR(sg_ipq_lpass_base));
	writel((ipq_hw == IPQ5332) ? 0x0 : 0xFC, HWIO_LPASS_QOS_FIXED_LAT_COUNTER_N_ADDR(sg_ipq_lpass_base));
	writel((ipq_hw == IPQ5332) ? 0x0 : 0xFB, HWIO_LPASS_QOS_FIXED_LAT_COUNTER_D_ADDR(sg_ipq_lpass_base));
	ipq_lpass_cc_update(HWIO_LPASS_QOS_FIXED_LAT_COUNTER_CFG_RCGR_ADDR(
			sg_ipq_lpass_base),
			(0 << HWIO_LPASS_QOS_FIXED_LAT_COUNTER_CFG_RCGR_MODE_SHFT) |
			(5 << HWIO_LPASS_QOS_FIXED_LAT_COUNTER_CFG_RCGR_SRC_SEL_SHFT) |
			(fix_srcdiv << HWIO_LPASS_QOS_FIXED_LAT_COUNTER_CFG_RCGR_SRC_DIV_SHFT),
			0);
	ipq_lpass_cc_update(HWIO_LPASS_QOS_FIXED_LAT_COUNTER_CMD_RCGR_ADDR(
			sg_ipq_lpass_base), 0x0, 1);

	writel((ipq_hw == IPQ5332) ? 0x0 : 0x01, HWIO_LPASS_ATIME_M_ADDR(sg_ipq_lpass_base));
	writel((ipq_hw == IPQ5332) ? 0x0 : 0xFD, HWIO_LPASS_ATIME_N_ADDR(sg_ipq_lpass_base));
	writel((ipq_hw == IPQ5332) ? 0x0 : 0xFC, HWIO_LPASS_ATIME_D_ADDR(sg_ipq_lpass_base));
	ipq_lpass_cc_update(HWIO_LPASS_ATIME_CFG_RCGR_ADDR(
			sg_ipq_lpass_base),
			(0 << HWIO_LPASS_ATIME_CFG_RCGR_MODE_SHFT) |
			(5 << HWIO_LPASS_ATIME_CFG_RCGR_SRC_SEL_SHFT) |
			(fix_srcdiv << HWIO_LPASS_ATIME_CFG_RCGR_SRC_DIV_SHFT),
			0);
	ipq_lpass_cc_update(HWIO_LPASS_ATIME_CMD_RCGR_ADDR(
			sg_ipq_lpass_base), 0x0, 1);

	ipq_lpass_pll_lock_wait();

	writel(0x01, HWIO_LPASS_CORE_M_ADDR(sg_ipq_lpass_base));
	writel(0xFC, HWIO_LPASS_CORE_N_ADDR(sg_ipq_lpass_base));
	writel(0xFB, HWIO_LPASS_CORE_D_ADDR(sg_ipq_lpass_base));
	ipq_lpass_cc_update(HWIO_LPASS_CORE_CFG_RCGR_ADDR(
			sg_ipq_lpass_base),
			(2 << HWIO_LPASS_ATIME_CFG_RCGR_MODE_SHFT) |
			(1 << HWIO_LPASS_ATIME_CFG_RCGR_SRC_SEL_SHFT) |
			(bus_srcdiv << HWIO_LPASS_ATIME_CFG_RCGR_SRC_DIV_SHFT),
			0);
	ipq_lpass_cc_update(HWIO_LPASS_CORE_CMD_RCGR_ADDR(
			sg_ipq_lpass_base), 0x0, 1);

	ipq_lpass_cc_update(HWIO_LPASS_SLEEP_CFG_RCGR_ADDR(
			sg_ipq_lpass_base),
			(0 << HWIO_LPASS_SLEEP_CFG_RCGR_MODE_SHFT) |
			(1 << HWIO_LPASS_SLEEP_CFG_RCGR_SRC_SEL_SHFT) |
			(1 << HWIO_LPASS_SLEEP_CFG_RCGR_SRC_DIV_SHFT),
			0);
	ipq_lpass_cc_update(HWIO_LPASS_SLEEP_CMD_RCGR_ADDR(
			sg_ipq_lpass_base), 0x0, 1);

	writel(0x01, HWIO_LPASS_AON_M_ADDR(sg_ipq_lpass_base));
	writel(0xFD, HWIO_LPASS_AON_N_ADDR(sg_ipq_lpass_base));
	writel(0xFC, HWIO_LPASS_AON_D_ADDR(sg_ipq_lpass_base));
	ipq_lpass_cc_update(HWIO_LPASS_AON_CFG_RCGR_ADDR(
			sg_ipq_lpass_base),
			(2 << HWIO_LPASS_AON_CFG_RCGR_MODE_SHFT) |
			(1 << HWIO_LPASS_AON_CFG_RCGR_SRC_SEL_SHFT) |
			(aon_srcdiv << HWIO_LPASS_AON_CFG_RCGR_SRC_DIV_SHFT),
			0);
	ipq_lpass_cc_update(HWIO_LPASS_AON_CMD_RCGR_ADDR(
			sg_ipq_lpass_base), 0x0, 1);

	writel(0x01, HWIO_LPASS_RESAMPLER_M_ADDR(sg_ipq_lpass_base));
	writel(0xFE, HWIO_LPASS_RESAMPLER_N_ADDR(sg_ipq_lpass_base));
	writel(0xFD, HWIO_LPASS_RESAMPLER_D_ADDR(sg_ipq_lpass_base));
	ipq_lpass_cc_update(HWIO_LPASS_RESAMPLER_CFG_RCGR_ADDR(
			sg_ipq_lpass_base),
			(2 << HWIO_LPASS_RESAMPLER_CFG_RCGR_MODE_SHFT) |
			(1 << HWIO_LPASS_RESAMPLER_CFG_RCGR_SRC_SEL_SHFT) |
			(res_srcdiv << HWIO_LPASS_RESAMPLER_CFG_RCGR_SRC_DIV_SHFT),
			0);
	ipq_lpass_cc_update(HWIO_LPASS_RESAMPLER_CMD_RCGR_ADDR(
			sg_ipq_lpass_base), 0x0, 1);

	writel(0x1,
		HWIO_LPASS_AUDIO_CORE_BCR_SLP_CBCR_ADDR(sg_ipq_lpass_base));

	writel(0x1,
		HWIO_LPASS_AUDIO_CORE_CORE_CBCR_ADDR(sg_ipq_lpass_base));

	writel(0x1,
		HWIO_LPASS_AUDIO_CORE_LPM_CORE_CBCR_ADDR(sg_ipq_lpass_base));

	writel(0x1,
		HWIO_LPASS_AUDIO_CORE_RESAMPLER_CBCR_ADDR(sg_ipq_lpass_base));

	writel(0x1,
		HWIO_LPASS_AUDIO_CORE_BCR_SLP_CBCR_ADDR(sg_ipq_lpass_base));

	writel(0x1, HWIO_LPASS_AUDIO_CORE_AXIM_CBCR_ADDR(sg_ipq_lpass_base));

	writel(0x1,
		HWIO_LPASS_AUDIO_CORE_QDSP_SWAY_AON_CBCR_ADDR(sg_ipq_lpass_base));

	writel(0x1,
		HWIO_LPASS_AUDIO_CORE_AVSYNC_ATIME_CBCR_ADDR(sg_ipq_lpass_base));

	writel(0x1,
		HWIO_LPASS_AUDIO_CORE_AVSYNC_STC_CBCR_ADDR(sg_ipq_lpass_base));

	writel(0x1,
		HWIO_LPASS_AUDIO_CORE_SYSNOC_MPORT_CORE_CBCR_ADDR(sg_ipq_lpass_base));

	ipq_lpass_core_pwrctl();

	writel(0x1, HWIO_LPASS_TCSR_QOS_CTL_ADDR(sg_ipq_lpass_base));

	writel(0x1,
		HWIO_LPASS_AUDIO_WRAPPER_QOS_AHBS_AON_CBCR_ADDR(sg_ipq_lpass_base));

	writel(0x1,
		HWIO_LPASS_AUDIO_CORE_QOS_CTL_ADDR(sg_ipq_lpass_base));

	writel(0x1,
		HWIO_LPASS_AUDIO_CORE_QOS_CORE_CGCR_ADDR(sg_ipq_lpass_base));

	writel(0x1,
		HWIO_LPASS_AUDIO_WRAPPER_EXT_MCLK0_CBCR_ADDR(sg_ipq_lpass_base));

	writel(0x1,
		HWIO_LPASS_AUDIO_WRAPPER_EXT_MCLK1_CBCR_ADDR(sg_ipq_lpass_base));

	writel(0x1,
		HWIO_LPASS_AUDIO_WRAPPER_EXT_MCLK2_CBCR_ADDR(sg_ipq_lpass_base));

	writel(0x1,
		HWIO_LPASS_AUDIO_CORE_LPAIF_CODEC_SPKR_EBIT_CBCR_ADDR(
			sg_ipq_lpass_base));

	writel(0x1,
		HWIO_LPASS_AUDIO_CORE_LPAIF_CODEC_SPKR_IBIT_CBCR_ADDR(
			sg_ipq_lpass_base));

	writel(0x1,
		HWIO_LPASS_AUDIO_CORE_LPAIF_CODEC_SPKR_OSR_CBCR_ADDR(
			sg_ipq_lpass_base));

	writel(0x1,
		HWIO_LPASS_AUDIO_CORE_LPAIF_PRI_EBIT_CBCR_ADDR(
			sg_ipq_lpass_base));

	writel(0x1,
		HWIO_LPASS_AUDIO_CORE_LPAIF_PRI_IBIT_CBCR_ADDR(
			sg_ipq_lpass_base));

	writel(0x1,
		HWIO_LPASS_AUDIO_CORE_LPAIF_SEC_EBIT_CBCR_ADDR(
			sg_ipq_lpass_base));

	writel(0x1,
		HWIO_LPASS_AUDIO_CORE_LPAIF_SEC_IBIT_CBCR_ADDR(
			sg_ipq_lpass_base));

	writel(0x1, HWIO_LPASS_AUDIO_CORE_LPAIF_PCM_DATA_OE_CBCR_ADDR(
			sg_ipq_lpass_base));

	writel(0x1,
		HWIO_LPASS_AUDIO_WRAPPER_QOS_XO_LAT_COUNTER_CBCR_ADDR(
			sg_ipq_lpass_base));

	writel(0x1,
		HWIO_LPASS_AUDIO_WRAPPER_QOS_DMONITOR_FIXED_LAT_COUNTER_CBCR_ADDR(sg_ipq_lpass_base));

	writel(0x1,
		HWIO_LPASS_AUDIO_WRAPPER_QOS_DANGER_FIXED_LAT_COUNTER_CBCR_ADDR(sg_ipq_lpass_base));

	writel(0x1,
		HWIO_LPASS_AUDIO_CORE_QOS_DMONITOR_FIXED_LAT_COUNTER_CBCR_ADDR(sg_ipq_lpass_base));

	writel(0x1,
		HWIO_LPASS_AUDIO_CORE_LPAIF_RXTX_WR_MEM_CBCR_ADDR(
			sg_ipq_lpass_base));

	writel(0x1,
		HWIO_LPASS_AUDIO_CORE_LPAIF_RXTX_RD_MEM_CBCR_ADDR(
			sg_ipq_lpass_base));

/*
 * debug
 */
	writel(0x1,
		HWIO_LPASS_AUDIO_WRAPPER_BUS_TIMEOUT_AON_CBCR_ADDR(
			sg_ipq_lpass_base));

	writel(0x1,
		HWIO_LPASS_AUDIO_CORE_BUS_TIMEOUT_CORE_CBCR_ADDR(
			sg_ipq_lpass_base));
}

void __iomem *ipq_lpass_phy_to_virt(uint32_t phy_addr)
{
	return (sg_ipq_lpass_base + (phy_addr - LPASS_BASE));
}
EXPORT_SYMBOL(ipq_lpass_phy_to_virt);

static void ipq_lpass_lpm_lpaif_reset(void)
{
	writel((1 << 31), HWIO_LPASS_LPM_CTL_ADDR(sg_ipq_lpass_base));

	writel(0x0, HWIO_LPASS_LPM_CTL_ADDR(sg_ipq_lpass_base));

	writel((1 << 31), HWIO_LPASS_LPAIF_CTL_ADDR(sg_ipq_lpass_base));

	writel(0x0, HWIO_LPASS_LPAIF_CTL_ADDR(sg_ipq_lpass_base));

}

static const struct of_device_id ipq_lpass_id_table[] = {
	{ .compatible = "qca,lpass-ipq5018", .data = (void *)IPQ5018 },
	{ .compatible = "qca,lpass-ipq9574", .data = (void *)IPQ9574 },
	{ .compatible = "qca,lpass-ipq5332", .data = (void *)IPQ5332 },
	{},
};
MODULE_DEVICE_TABLE(of, ipq_lpass_id_table);

static int ipq_lpass_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct resource *res;
	struct lpass_res *resource;
	const struct of_device_id *match;
	int ret;

	match = of_match_device(ipq_lpass_id_table, &pdev->dev);
	if (!match)
		return -ENODEV;
	ipq_hw = (enum ipq_hw_type)match->data;
	resource = devm_kzalloc(dev, sizeof(*resource), GFP_KERNEL);
	if (!resource)
		return -ENOMEM;

	pr_info("%s init \n", __func__);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);

	sg_ipq_lpass_base = devm_ioremap_resource(&pdev->dev, res);

	if (IS_ERR(sg_ipq_lpass_base))
		return PTR_ERR(sg_ipq_lpass_base);
	if ((ipq_hw == IPQ9574) || (ipq_hw == IPQ5332)) {
/*
 * clock init for IPQ9574 and IPQ5332
 */
		resource->sway_clk = devm_clk_get(dev, "sway");
		if (IS_ERR(resource->sway_clk))
			return PTR_ERR(resource->sway_clk);

		resource->axi_core_clk = devm_clk_get(dev, "axim");
		if (IS_ERR(resource->axi_core_clk))
			return PTR_ERR(resource->axi_core_clk);

		resource->snoc_cfg_clk = devm_clk_get(dev, "snoc_cfg");
		if (IS_ERR(resource->snoc_cfg_clk))
			return PTR_ERR(resource->snoc_cfg_clk);

		resource->pcnoc_clk = devm_clk_get(dev, "pcnoc");
		if (IS_ERR(resource->pcnoc_clk))
			return PTR_ERR(resource->pcnoc_clk);

		resource->reset = devm_reset_control_get(dev, "lpass");
		if (IS_ERR(resource->reset))
			return PTR_ERR(resource->reset);

		reset_control_assert(resource->reset);
		wmb(); /* ensure data is written to hw register */
		usleep_range(1, 5);
		reset_control_deassert(resource->reset);
		wmb(); /* ensure data is written to hw register */

		ret = clk_prepare_enable(resource->sway_clk);
		if (ret) {
			dev_err(dev, "cannot prepare/enable sway_clk clock\n");
			goto err_clk_sway;
		}

		if (ipq_hw == IPQ5332) {
			ret = clk_set_rate(resource->sway_clk, 133333334);
			if (ret) {
				dev_err(dev, "AXI rate set failed (%d)\n", ret);
				goto err_clk_sway;
			}
		}

		ret = clk_prepare_enable(resource->axi_core_clk);
		if (ret) {
			dev_err(dev, "cannot prepare/enable axi_core_clk clock\n");
			goto err_clk_axi;
		}

		ret = clk_prepare_enable(resource->snoc_cfg_clk);
		if (ret) {
			dev_err(dev, "cannot prepare/enable snoc_cfg_clk clock\n");
			goto err_clk_snoc;
		}

		ret = clk_prepare_enable(resource->pcnoc_clk);
		if (ret) {
			dev_err(dev, "cannot prepare/enable pcnoc_clk clock\n");
			goto err_clk_pcnoc;
		}

	} else {
		/*
		   clock init for Mapple
		 */
		resource->axi_snoc_clk = devm_clk_get(dev, "snoc_axim");
		if (IS_ERR(resource->axi_snoc_clk))
			return PTR_ERR(resource->axi_snoc_clk);

		resource->sway_snoc_clk = devm_clk_get(dev, "snoc_sway");
		if (IS_ERR(resource->sway_snoc_clk))
			return PTR_ERR(resource->sway_snoc_clk);

		resource->axi_core_clk = devm_clk_get(dev, "axim");
		if (IS_ERR(resource->axi_core_clk))
			return PTR_ERR(resource->axi_core_clk);

		resource->sway_clk = devm_clk_get(dev, "sway");
		if (IS_ERR(resource->sway_clk))
			return PTR_ERR(resource->sway_clk);

		resource->reset = devm_reset_control_get(dev, "lpass");
		if (IS_ERR(resource->reset))
			return PTR_ERR(resource->reset);

		ret = reset_control_deassert(resource->reset);
		if (ret) {
			dev_err(dev, "cannot deassert  reset\n");
			return ret;
		}

		ret = clk_prepare_enable(resource->axi_snoc_clk);
		if (ret) {
			dev_err(dev, "cannot prepare/enable axi_snoc_clk clock\n");
			goto err_clk_axi_snoc;
		}

		ret = clk_prepare_enable(resource->sway_snoc_clk);
		if (ret) {
			dev_err(dev, "cannot prepare/enable sway_snoc_clk clock\n");
			goto err_clk_sway_snoc;
		}

		ret = clk_prepare_enable(resource->axi_core_clk);
		if (ret) {
			dev_err(dev, "cannot prepare/enable axi_core_clk clock\n");
			goto err_clk_axi;
		}

		ret = clk_set_rate(resource->axi_core_clk, 133333334);
		if (ret) {
			dev_err(dev, "AXI rate set failed (%d)\n", ret);
			goto err_clk_axi;
		}

		ret = clk_prepare_enable(resource->sway_clk);
		if (ret) {
			dev_err(dev, "cannot prepare/enable sway_clk clock\n");
			goto err_clk_sway;
		}

		ret = clk_set_rate(resource->sway_clk, 66666667);
		if (ret) {
			dev_err(dev, "AXI rate set failed (%d)\n", ret);
			goto err_clk_sway;
		}
	}

	platform_set_drvdata(pdev, resource);

	ipq_lpass_clk_init();

	ipq_lpass_lpm_lpaif_reset();

	return 0;
err_clk_sway:
	clk_disable_unprepare(resource->sway_clk);
err_clk_axi:
	clk_disable_unprepare(resource->axi_core_clk);
err_clk_sway_snoc:
	clk_disable_unprepare(resource->sway_snoc_clk);
err_clk_axi_snoc:
	clk_disable_unprepare(resource->axi_snoc_clk);
err_clk_snoc:
	clk_disable_unprepare(resource->snoc_cfg_clk);
err_clk_pcnoc:
	clk_disable_unprepare(resource->pcnoc_clk);

	return ret;
}

static int ipq_lpass_remove(struct platform_device *pdev)
{
	struct lpass_res *resource = platform_get_drvdata(pdev);

	clk_disable_unprepare(resource->sway_clk);
	clk_disable_unprepare(resource->axi_core_clk);
	if (ipq_hw == IPQ9574 || ipq_hw == IPQ5332) {
		clk_disable_unprepare(resource->snoc_cfg_clk);
		clk_disable_unprepare(resource->pcnoc_clk);
	} else {
		clk_disable_unprepare(resource->axi_snoc_clk);
		clk_disable_unprepare(resource->sway_snoc_clk);
	}
	reset_control_assert(resource->reset);
	return 0;
}

static struct platform_driver ipq_lpass_driver = {
	.probe = ipq_lpass_probe,
	.remove = ipq_lpass_remove,
	.driver = {
		.name = "ipq-lpass",
		.of_match_table = ipq_lpass_id_table,
	},
};

module_platform_driver(ipq_lpass_driver);

MODULE_ALIAS("platform:ipq-lpass");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("IPQ Audio subsytem driver");
