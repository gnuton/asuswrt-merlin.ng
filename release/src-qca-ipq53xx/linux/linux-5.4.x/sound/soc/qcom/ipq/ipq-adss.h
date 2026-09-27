/*
 * Copyright (c) 2015-2016, The Linux Foundation. All rights reserved.
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

#ifndef IPQ_ADSS_H
#define IPQ_ADSS_H

#include <sound/pcm.h>

/* ADSS AUDIO Registers */

#define ADSS_BASE	0x7700000
#define ADSS_RANGE	0x20000

/* ADSS_AUDIO_LOCAL_REG Registers */

#define ADSS_GLB_PCM_RST_REG			0x0
#define GLB_PCM_RST_CTRL(x)			(x << 0)

#define ADSS_GLB_PCM_MBOX_CTRL_REG		0x0C

#define ADSS_GLB_CHIP_CTRL_I2S_REG		0x10
#define GLB_CHIP_CTRL_I2S_INTERFACE_EN		BIT(0)
#define GLB_CHIP_CTRL_I2S_STEREO0_GLB_EN	BIT(1)
#define GLB_CHIP_CTRL_I2S_STEREO1_GLB_EN	BIT(2)
#define GLB_CHIP_CTRL_I2S_STEREO2_GLB_EN	BIT(3)

#define ADSS_GLB_I2S_RST_REG		0x14
#define GLB_I2S_RST_CTRL_MBOX0		BIT(0)
#define GLB_I2S_RST_CTRL_I2S0		BIT(1)
#define GLB_I2S_RST_CTRL_MBOX3		BIT(2)
/* IPQ8074 bit fields ADSS_GLB_I2S_RST_REG */
#define GLB_I2S_RESET_VAL_8074		0xFF
#define GLB_I2S_RST_MBOX_RESET_MASK	0x5
#define GLB_I2S_RST_CTRL_I2S3		BIT(3)
#define GLB_I2S_RST_CTRL_TXMCLK		BIT(4)
#define GLB_I2S_RST_CTRL_TXBCLK		BIT(5)
#define GLB_I2S_RST_CTRL_RXMCLK		BIT(6)
#define GLB_I2S_RST_CTRL_RXBCLK		BIT(7)

#define ADSS_GLB_CLK_I2S_CTRL_REG	0x18
#define GLB_CLK_I2S_CTRL_TX_BCLK_OE	BIT(28)
#define GLB_CLK_I2S_CTRL_RX_BCLK_OE	BIT(27)
#define GLB_CLK_I2S_CTRL_RX_MCLK_OE	BIT(16)
#define GLB_CLK_I2S_CTRL_TX_MCLK_OE	BIT(17)
/* IPQ8074 bit fields */
#define GLB_CLK_I2S_CTRL_I2S3_FS_OE	BIT(15)
#define GLB_CLK_I2S_CTRL_I2S0_FS_OE	BIT(14)
#define GLB_CLK_I2S_CTRL_I2S3_RXD_OE	BIT(13)
#define GLB_CLK_I2S_CTRL_I2S0_TXD_OE	BIT(12)


#define ADSS_GLB_TDM_CTRL_REG			0x1C
#define GLB_TDM_CTRL_TX_CHAN_NUM(x)	(x << 0)
#define GLB_TDM_CTRL_TX_CHAN_NUM_MASK	0xF
#define GLB_TDM_CTRL_TX_SYNC_NUM(x)	(x << 4)
#define GLB_TDM_CTRL_TX_SYNC_NUM_MASK	(0x1F << 4)
#define GLB_TDM_CTRL_RX_CHAN_NUM(x)	(x << 16)
#define GLB_TDM_CTRL_RX_CHAN_NUM_MASK	(0xF << 16)
#define GLB_TDM_CTRL_RX_SYNC_NUM(x)	(x << 20)
#define GLB_TDM_CTRL_RX_SYNC_NUM_MASK	(0x1F << 20)
#define GLB_TDM_CTRL_TX_DELAY			BIT(25)
#define GLB_TDM_CTRL_RX_DELAY			BIT(26)

#define ADSS_GLB_PWM0_CTRL_REG		0x20
#define ADSS_GLB_PWM1_CTRL_REG		0x24
#define ADSS_GLB_PWM2_CTRL_REG		0x28
#define ADSS_GLB_PWM3_CTRL_REG		0x2C

#define ADSS_GLB_AUDIO_MODE_REG		0x30
#define GLB_AUDIO_MODE_RECV_MASK	BIT(2)
#define GLB_AUDIO_MODE_XMIT_MASK	BIT(0)
#define GLB_AUDIO_MODE_RECV_I2S		(0 << 2)
#define GLB_AUDIO_MODE_RECV_TDM		BIT(2)
#define GLB_AUDIO_MODE_XMIT_I2S		(0 << 0)
#define GLB_AUDIO_MODE_XMIT_TDM		BIT(0)
#define GLB_AUDIO_MODE_I2S0_TXD_OE	(7 << 4)
#define GLB_AUDIO_MODE_I2S0_FS_OE	BIT(7)
#define GLB_AUDIO_MODE_I2S3_FS_OE	BIT(8)
#define GLB_AUDIO_MODE_I2S3_RXD_OE	BIT(9)
#define GLB_AUDIO_MODE_SPDIF_OUT_OE	BIT(10)
#define GLB_AUDIO_MODE_B1K			BIT(28)

#define ADSS_MBOX_STEREO_AUDIO_BASE		(ADSS_BASE + 0x8000)

/* ADSS_MBOX_STEREO_AUDIO_BASE + 0x0 */
#define ADSS_MBOX0_AUDIO_BASE				0x0
#define ADSS_MBOX1_AUDIO_BASE				0x2000
#define ADSS_MBOX2_AUDIO_BASE				0x4000
#define ADSS_MBOX3_AUDIO_BASE				0x6000

#define ADSS_MBOXn_MBOX_FIFO0_REG			0x0
#define MBOX_FIFO_RESET_TX_INIT				BIT(0)
#define MBOX_FIFO_RESET_RX_INIT				BIT(2)

#define ADSS_MBOXn_MBOX_FIFO_STATUS0_REG		0x08

#define ADSS_MBOXn_MBOX_DMA_POLICY_REG			0x10
#define MBOX_DMA_POLICY_SW_RESET			BIT(31)
#define MBOX_DMA_POLICY_TX_INT_TYPE			BIT(17)
#define MBOX_DMA_POLICY_RX_INT_TYPE			BIT(16)
#define MBOX_DMA_POLICY_RXD_16BIT_SWAP			BIT(10)
#define MBOX_DMA_POLICY_RXD_END_SWAP			BIT(8)
#define ADSS_MBOX_DMA_POLICY_SRAM_AC(x)		((((x) >> 28) & 0xf) << 12)
#define ADSS_MBOX_DMA_POLICY_TX_FIFO_THRESHOLD(x)	((((x) & 0xf) << 4))

#define ADSS_MBOXn_MBOXn_DMA_RX_DESCRIPTOR_BASE_REG	0x18

#define ADSS_MBOXn_MBOXn_DMA_RX_CONTROL_REG		0x1C
#define ADSS_MBOXn_DMA_RX_CONTROL_STOP			BIT(0)
#define ADSS_MBOXn_DMA_RX_CONTROL_START			BIT(1)
#define ADSS_MBOXn_DMA_RX_CONTROL_RESUME		BIT(2)

#define ADSS_MBOXn_MBOXn_DMA_TX_DESCRIPTOR_BASE_REG	0x20

#define ADSS_MBOXn_MBOXn_DMA_TX_CONTROL_REG		0x24
#define ADSS_MBOXn_DMA_TX_CONTROL_STOP			BIT(0)
#define ADSS_MBOXn_DMA_TX_CONTROL_START			BIT(1)
#define ADSS_MBOXn_DMA_TX_CONTROL_RESUME		BIT(2)

#define ADSS_MBOXn_MBOX_FRAME_REG			0x38
#define ADSS_MBOXn_FIFO_TIMEOUT_REG			0x40

#define ADSS_MBOXn_MBOX_INT_STATUS_REG			0x44
#define MBOX_INT_STATUS_TX_DMA_COMPLETE			BIT(6)
#define MBOX_INT_STATUS_RX_DMA_COMPLETE			BIT(10)

#define ADSS_MBOXn_MBOX_INT_ENABLE_REG			0x4C
#define MBOX_INT_ENABLE_RX_DMA_COMPLETE			BIT(10)
#define MBOX_INT_STATUS_RX_UNDERFLOW			BIT(4)
#define MBOX_INT_STATUS_RX_FIFO_UNDERFLOW		BIT(12)
#define MBOX_INT_ENABLE_TX_DMA_COMPLETE			BIT(6)
#define MBOX_INT_STATUS_TX_OVERFLOW			BIT(5)
#define MBOX_INT_STATUS_TX_FIFO_OVERFLOW		BIT(13)

#define ADSS_MBOXn_MBOX_FIFO_RESET_REG			0x58
#define MBOX_FIFO_RESET_TX_INIT				BIT(0)
#define MBOX_FIFO_RESET_RX_INIT				BIT(2)

#define ADSS_MBOXn_MBOX_DEBUG_CHAIN0_REG		0x60
#define ADSS_MBOXn_MBOX_DEBUG_CHAIN1_REG		0x64
#define ADSS_MBOXn_MBOX_DEBUG_CHAIN0_SIGNALS_REG	0x68
#define ADSS_MBOXn_MBOX_DEBUG_CHAIN1_SIGNALS_REG	0x6C

/* ADSS_STEREO0_AUDIO_STEREO_REG Registers */

#define ADSS_STEREO0_AUDIO_BASE		0x9000
#define ADSS_STEREO1_AUDIO_BASE		0xB000
#define ADSS_STEREO2_AUDIO_BASE		0xD000
#define ADSS_STEREO3_AUDIO_BASE		0xF000

#define STEREO0_OFFSET			0x0
#define STEREO1_OFFSET			0x2000
#define STEREO2_OFFSET			0x4000
#define STEREO3_OFFSET			0x6000

#define ADSS_STEREOn_STEREO0_CONFIG_REG			0x0
#define STEREOn_CONFIG_MIC_SWAP				BIT(24)
#define STEREOn_CONFIG_SPDIF_ENABLE			BIT(23)
#define STEREOn_CONFIG_ENABLE				BIT(21)
#define STEREOn_CONFIG_MIC_RESET			BIT(20)
#define STEREOn_CONFIG_RESET				BIT(19)
#define STEREOn_CONFIG_I2S_DELAY			(0 << 18)
#define STEREOn_CONFIG_PCM_SWAP				BIT(17)
#define STEREOn_CONFIG_MIC_WORD_SIZE_32			BIT(16)
#define STEREOn_CONFIG_MIC_WORD_SIZE_16			(0 << 16)
#define STEREOn_CONFIG_STEREO_MODE			(0 << 14)
#define STEREOn_CONFIG_MONO_MODE			BIT(14)
#define STEREOn_CONFIG_STEREO_MONO_MASK			(3 << 14)
#define STEREOn_CONFIG_DATA_WORD_SIZE(x)		((x) << 12)
#define STEREOn_CONFIG_DATA_WORD_SIZE_MASK		(3 << 12)
#define STEREOn_CONFIG_I2S_WORD_SIZE_32			BIT(11)
#define STEREOn_CONFIG_I2S_WORD_SIZE_16			(0 << 11)
#define STEREOn_CONFIG_MCK_SEL				BIT(10)
#define STEREOn_CONFIG_SAMPLE_CNT_CLEAR_TYPE		BIT(9)
#define STEREOn_CONFIG_MASTER				BIT(8)

#define MAX_STEREO_ENTRIES	4
#define TDM_SYNC_NUM		2
#define TDM_DELAY		0
#define MCLK_MULTI		4

/* ADSS_AUDIO_PCM_REG Registers */

#define ADSS_AUDIO_PCM_REG_BASE		(ADSS_BASE + 0x4000)

#define AADSS_PCM_BITMAP_REG		0x0

#define AADSS_PCM_CTRL_REG		0x04
#define PCM_CTRL_TX2RX_LP_EN(x)		(x << 31)
#define PCM_CTRL_RX2TX_LP_EN(x)		(x << 30)
#define PCM_CTRL_CPU_MODE(x)		(x << 29)
#define PCM_CTRL_PCM_GCLK_EN(x)		(x << 28)
#define PCM_CTRL_FRAME_SYNC_LEN(x)	(x << 26)
#define PCM_CTRL_PCM_CLK_MODE(x)	(x << 25)
#define PCM_CTRL_PCM_SLOT_MODE(x)	(x << 24)
#define PCM_CTRL_PCM_DCLK_MODE(x)	(x << 4)
#define PCM_CTRL_PCM_PHASE_MASK		0xF
#define PCM_CTRL_PCM_TX_PHASE(x)	(x << 2)
#define PCM_CTRL_PCM_RX_PHASE(x)	(x << 0)

#define AADSS_PCM_OFFSET_REG		0x08
#define AADSS_PCM_START_REG		0x0C
#define AADSS_PCM_INT_STATUS_REG	0x10
#define AADSS_PCM_INT_ENABLE_REG	0x14
#define AADSS_PCM_RX_DATA_8BIT_REG	0x18
#define AADSS_PCM_TX_DATA_8BIT_REG	0x1C
#define AADSS_PCM_DIVIDER_REG		0x20
#define AADSS_PCM_TH_REG		0x24
#define PCM_TH_PCM_TX_THRESH(x)		(x << 0)
#define PCM_TH_PCM_RX_THRESH(x)		(x << 9)
#define AADSS_PCM_FIFO_CNT_REG		0x28
#define AADSS_PCM_FIFO_ERR_SLOT_REG	0x2C
#define AADSS_PCM_RX_DATA_16BIT_REG	0x30
#define AADSS_PCM_TX_DATA_16BIT_REG	0x34

/* I2S Parameters */
#define IPQ_I2S_NO_OF_PERIODS	(130)
#define IPQ_I2S_PERIOD_BYTES_MIN	(4032)
#define IPQ_I2S_BUFF_SIZE		(IPQ_I2S_PERIOD_BYTES_MIN * \
						IPQ_I2S_NO_OF_PERIODS)
#define IPQ_I2S_CAPTURE_BUFF_SIZE	(IPQ_I2S_PERIOD_BYTES_MIN * \
						IPQ_I2S_NO_OF_PERIODS)

/* TDM Parameters */
#define IPQ_TDM_NO_OF_PERIODS	(260)
#define IPQ_TDM_PERIOD_BYTES_MIN	(4032)
#define IPQ_TDM_BUFF_SIZE		(IPQ_TDM_PERIOD_BYTES_MIN * \
						IPQ_TDM_NO_OF_PERIODS)
#define IPQ_TDM_CAPTURE_BUFF_SIZE	(IPQ_TDM_PERIOD_BYTES_MIN * \
						IPQ_TDM_NO_OF_PERIODS)

/* SPDIF area */


/* ADSS_MBOXSPDIFIN_AUDIO_MBOX_REG Registers */

#define ADSS_MBOXSPDIFIN_AUDIO_MBOX_REG_BASE	(ADSS_BASE + 0x6000)

#define AADSS_MBOXSPDIFIN_MBOX_FIFO0_REG	\
	((ADSS_MBOXSPDIFIN_AUDIO_MBOX_REG_BASE) + 0x0)

#define AADSS_MBOXSPDIFIN_MBOX_FIFO_STATUS0_REG	\
	((ADSS_MBOXSPDIFIN_AUDIO_MBOX_REG_BASE) + 0x08)

#define AADSS_MBOXSPDIFIN_MBOX_DMA_POLICY_REG	\
	((ADSS_MBOXSPDIFIN_AUDIO_MBOX_REG_BASE) + 0x10)

#define AADSS_MBOXSPDIFIN_MBOX0_DMA_RX_DESCRIPTOR_BASE_REG \
	((ADSS_MBOXSPDIFIN_AUDIO_MBOX_REG_BASE) + 0x18)

#define AADSS_MBOXSPDIFIN_MBOX0_DMA_RX_CONTROL_REG	\
	((ADSS_MBOXSPDIFIN_AUDIO_MBOX_REG_BASE) + 0x1C)

#define AADSS_MBOXSPDIFIN_MBOX0_DMA_TX_DESCRIPTOR_BASE_REG \
	((ADSS_MBOXSPDIFIN_AUDIO_MBOX_REG_BASE) + 0x20)

#define AADSS_MBOXSPDIFIN_MBOX0_DMA_TX_CONTROL_REG	\
	((ADSS_MBOXSPDIFIN_AUDIO_MBOX_REG_BASE) + 0x24)

#define AADSS_MBOXSPDIFIN_MBOX_FRAME_REG	\
	((ADSS_MBOXSPDIFIN_AUDIO_MBOX_REG_BASE) + 0x38)

#define AADSS_MBOXSPDIFIN_FIFO_TIMEOUT_REG	\
	((ADSS_MBOXSPDIFIN_AUDIO_MBOX_REG_BASE) + 0x40)

#define AADSS_MBOXSPDIFIN_MBOX_INT_STATUS_REG	\
	((ADSS_MBOXSPDIFIN_AUDIO_MBOX_REG_BASE) + 0x44)

#define AADSS_MBOXSPDIFIN_MBOX_INT_ENABLE_REG	\
	((ADSS_MBOXSPDIFIN_AUDIO_MBOX_REG_BASE) + 0x4C)

#define AADSS_MBOXSPDIFIN_MBOX_FIFO_RESET_REG	\
	((ADSS_MBOXSPDIFIN_AUDIO_MBOX_REG_BASE) + 0x58)

#define AADSS_MBOXSPDIFIN_MBOX_DEBUG_CHAIN0_REG	\
	((ADSS_MBOXSPDIFIN_AUDIO_MBOX_REG_BASE) + 0x60)

#define AADSS_MBOXSPDIFIN_MBOX_DEBUG_CHAIN1_REG	\
	((ADSS_MBOXSPDIFIN_AUDIO_MBOX_REG_BASE) + 0x64)

#define AADSS_MBOXSPDIFIN_MBOX_DEBUG_CHAIN0_SIGNALS_REG	\
	((ADSS_MBOXSPDIFIN_AUDIO_MBOX_REG_BASE) + 0x68)

#define AADSS_MBOXSPDIFIN_MBOX_DEBUG_CHAIN1_SIGNALS_REG	\
	((ADSS_MBOXSPDIFIN_AUDIO_MBOX_REG_BASE) + 0x6C)

/* ADSS_SPDIFIN_AUDIO_SPDIF_BASE Registers */

#define ADSS_SPDIFIN_SPDIF_CTRL_REG		(0x00)
#define SPDIF_CTRL_INTREQ_MASK			BIT(31)
#define SPDIF_CTRL_BEGIN_MASK			BIT(30)
#define SPDIF_CTRL_LOCK_MASK			BIT(29)
#define SPDIF_CTRL_SYNCERR_MASK			BIT(28)
#define SPDIF_CTRL_AFULL_MASK			BIT(27)
#define SPDIF_CTRL_FULL_MASK			BIT(26)
#define SPDIF_CTRL_AEMPTY_MASK			BIT(25)
#define SPDIF_CTRL_EMPTY_MASK			BIT(24)
#define SPDIF_CTRL_OVRERR_MASK			BIT(23)
#define SPDIF_CTRL_UNDERR_MASK			BIT(22)
#define SPDIF_CTRL_PARITY_MASK			BIT(21)
#define SPDIF_CTRL_USE_FIFO_IF			BIT(19)
#define SPDIF_CTRL_SETPREAMBB			BIT(18)
#define SPDIF_CTRL_DUPLICATE			BIT(17)
#define SPDIF_CTRL_CHANNEL_MODE			BIT(16)
#define SPDIF_CTRL_VALIDITYCHECK		BIT(15)
#define SPDIF_CTRL_PARITYGEN			BIT(14)
#define SPDIF_CTRL_PARITYCHECK			BIT(13)
#define SPDIF_CTRL_TR_MODE			BIT(12)
#define SPDIF_CTRL_CLK_ENABLE			BIT(11)
#define SPDIF_CTRL_FIFO_ENABLE			BIT(10)
#define SPDIF_CTRL_SPDIF_ENABLE			BIT(9)
#define SPDIF_CTRL_SFR_ENABLE			BIT(8)
#define SPDIF_CTRL_TSAMPLERATE			BIT(7)

#define ADSS_SPDIFIN_STEREO0_VOLUME		(0x04)
#define ADSS_SPDIFIN_FIFO_CTRL_REG		(0x08)
#define ADSS_SPDIFIN_START_REG_REG		(0x0C)
#define ADSS_SPDIFIN_SELFIFO_REG		(0x10)


#define ADSS_AUDIO_SPDIF_MISC_REG		0x150
#define AUDIO_SPDIF_MISC_AUTO_SCALE_DIV_MASK	(0xF << 1)
#define AUDIO_SPDIF_MISC_AUTO_SCALE_DIV(x)	(x << 1)

#define ADSS_AUDIO_SPDIF_CBCR_REG		0x154

#define ADSS_AUDIO_SPDIFDIV2_MISC_REG		0x158
#define AUDIO_SPDIFDIV2_MISC_AUTO_SCALE_DIV_MASK	(0xF << 1)
#define AUDIO_SPDIFDIV2_MISC_AUTO_SCALE_DIV(x)	(x << 1)

#define ADSS_AUDIO_SPDIFDIV2_CBCR_REG		0x15C
#define ADSS_AUDIO_SPDIFINFAST_CMD_RCGR_REG	0x1E0
#define AUDIO_SPDIFINFAST_CMD_RCGR_ROOT_EN	(1 << 1)
#define AUDIO_SPDIFINFAST_CMD_RCGR_UPDATE	(1 << 0)

#define ADSS_AUDIO_SPDIFINFAST_CFG_RCGR_REG	0x1E4
#define AUDIO_SPDIFINFAST_CFG_RCGR_SRC_SEL(x)	(x << 8)
#define AUDIO_SPDIFINFAST_CFG_RCGR_SRC_DIV(x)	(x << 0)

#define ADSS_AUDIO_SPDIFINFAST_CBCR_REG		0x1EC
#define AUDIO_SPDIFINFAST			49152000

#define ADSS_AUDIO_PCM_CMD_RCGR_REG		0x1A0

#define ADSS_AUDIO_PCM_CFG_RCGR_REG		0x1A4
#define AUDIO_PCM_CFG_RCGR_SRC_SEL(x)		(x << 8)
#define AUDIO_PCM_CFG_RGCR_SRC_DIV(x)		(x << 0)

#define ADSS_AUDIO_PCM_MISC_REG			0x1A8
#define AUDIO_PCM_MISC_AUTO_SCALE_DIV(x)	(x << 0)

#define ADSS_AUDIO_PCM_CBCR_REG			0x1AC
#define ADSS_AUDIO_ZSI_CBCR_REG			0x1B8

#define SNDRV_PCM_FMTBIT_S24_3			SNDRV_PCM_FMTBIT_S24_3LE

/* Enumerations */
enum ipq_hw_type {
	IPQ8074
};

enum intf {
	I2S,
	TDM,
	SPDIF,
	I2S1,
	I2S2,
	MAX_INTF
};

enum dir {
	PLAYBACK,
	CAPTURE
};

enum cfg {
	DISABLE,
	ENABLE
};

/* Supported Channels */
enum channels {
	CH_STEREO = 2,
	CH_3_1 = 4,
	CH_5_1 = 6,
	CH_7_1 = 8
};

enum ipq_samp_freq {
	FREQ_8000 = 8000,
	FREQ_11025 = 11025,
	FREQ_16000 = 16000,
	FREQ_22050 = 22050,
	FREQ_32000 = 32000,
	FREQ_44100 = 44100,
	FREQ_48000 = 48000,
	FREQ_64000 = 64000,
	FREQ_88200 = 88200,
	FREQ_96000 = 96000,
	FREQ_176400 = 176400,
	FREQ_192000 = 192000,
};

#define RATE_16000_96000 \
		(SNDRV_PCM_RATE_16000 | SNDRV_PCM_RATE_22050 |\
		SNDRV_PCM_RATE_32000 | SNDRV_PCM_RATE_44100 |\
		SNDRV_PCM_RATE_48000 | SNDRV_PCM_RATE_64000 |\
		SNDRV_PCM_RATE_88200 | SNDRV_PCM_RATE_96000)

enum stereo_ch {
	STEREO0,
	STEREO1,
	STEREO2,
	STEREO3
};

enum bit_width {
	__BIT_8 = 8,
	__BIT_16 = 16,
	__BIT_24 = 24,
	__BIT_32 = 32,
	__BIT_INVAL = -1
};

struct ipq_intf_pdata {
	uint32_t data;
	uint32_t hw;
};

/* Structure to hold the Register/bitfield
 * changes between ipq SoCs
 */
struct ipq_regs_arr {
	uint32_t reg;
	uint32_t mask;
};

struct ipq_configs {
	struct ipq_regs_arr txd_oe;
	struct ipq_regs_arr rxd_oe;
	struct ipq_regs_arr i2s0_fs_oe;
	struct ipq_regs_arr i2s3_fs_oe;
	struct ipq_regs_arr i2s_reset_val;
};

/* ADSS APIs */
extern void ipq_glb_audio_mode(int mode, int dir);
extern void ipq_glb_tx_data_port_en(uint32_t enable);
extern void ipq_glb_rx_data_port_en(uint32_t enable);
extern void ipq_glb_tx_framesync_port_en(uint32_t enable);
extern void ipq_glb_rx_framesync_port_en(uint32_t enable);
extern void ipq_glb_clk_enable_oe(uint32_t dir);
extern void ipq_glb_pcm_rst(uint32_t enable);
extern void ipq_pcm_clk_cfg(uint32_t rate);
extern void ipq_pcm_clk_enable(void);
extern void ipq_glb_mbox_reset(void);

extern void ipq_audio_adss_init(void);
/* APIs to read/write ADSS registers */
extern void ipq_audio_adss_writel(uint32_t val, uint32_t offset);
extern uint32_t ipq_audio_adss_readl(uint32_t offset);

/* Stereo APIs */
extern void ipq_stereo_config_reset(uint32_t reset, uint32_t stereo_offset);
extern void ipq_stereo_config_mic_reset(uint32_t reset,
					uint32_t stereo_offset);
extern void ipq_stereo_config_enable(uint32_t enable,
					uint32_t stereo_offset);
extern int ipq_cfg_bit_width(uint32_t bit_width, uint32_t stereo_offset);
extern void ipq_config_stereo_mode(uint32_t mode, uint32_t stereo_offset);
extern void ipq_config_master(uint32_t enable, uint32_t stereo_offset);
extern void ipq_config_mclk_sel(uint32_t stereo_offset, uint32_t val);
extern void ipq_config_sample_cnt_clear_type(uint32_t stereo_offset);

/* APIs in DAI driver */
extern int ipq_get_mbox_id(struct snd_pcm_substream *substream, int intf);
extern int ipq_get_stereo_id(struct snd_pcm_substream *substream, int intf);
extern u32 ipq_get_act_bit_width(u32 bit_width);
extern void ipq_glb_tdm_ctrl_ch_num(uint32_t val, uint32_t dir);
extern void ipq_glb_tdm_ctrl_sync_num(uint32_t val, uint32_t dir);
extern void ipq_glb_tdm_ctrl_delay(uint32_t delay, uint32_t dir);

void ipq_glb_audio_mode_B1K(void);

#endif
