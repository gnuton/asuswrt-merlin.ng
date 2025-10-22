/*
 * BCMSDH Function Driver for the native SDIO/MMC driver in the Linux Kernel
 *
 * Copyright (C) 2024, Broadcom. All Rights Reserved.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 *
 * <<Broadcom-WL-IPTag/Dual>>
 *
 * $Id: bcmsdh_sdmmc.h 823883 2023-04-13 17:33:08Z $
 */

#ifndef __BCMSDH_SDMMC_H__
#define __BCMSDH_SDMMC_H__

#ifdef BCMDBG
#define sd_err(x)	do { if (sd_msglevel & SDH_ERROR_VAL) printf x; } while (0)
#define sd_trace(x)	do { if (sd_msglevel & SDH_TRACE_VAL) printf x; } while (0)
#define sd_info(x)	do { if (sd_msglevel & SDH_INFO_VAL)  printf x; } while (0)
#define sd_debug(x)	do { if (sd_msglevel & SDH_DEBUG_VAL) printf x; } while (0)
#define sd_data(x)	do { if (sd_msglevel & SDH_DATA_VAL)  printf x; } while (0)
#define sd_ctrl(x)	do { if (sd_msglevel & SDH_CTRL_VAL)  printf x; } while (0)
#else
#define sd_err(x)	do { if (sd_msglevel & SDH_ERROR_VAL) printf x; } while (0)
#define sd_trace(x)
#define sd_info(x)
#define sd_debug(x)
#define sd_data(x)
#define sd_ctrl(x)
#endif // endif

#define sd_sync_dma(sd, read, nbytes)
#define sd_init_dma(sd)
#define sd_ack_intr(sd)
#define sd_wakeup(sd);

#ifdef BCMPERFSTATS
#define sd_log(x)	do { if (sd_msglevel & SDH_LOG_VAL)	 bcmlog x; } while (0)
#else
#define sd_log(x)
#endif // endif

#define SDIOH_ASSERT(exp) \
	do { if (!(exp)) \
		printf("!!!ASSERT fail: file %s lines %d", __FILE__, __LINE__); \
	} while (0)

#define BLOCK_SIZE_4318 64
#define BLOCK_SIZE_4328 512

/* internal return code */
#define SUCCESS	0
#define ERROR	1

/* private bus modes */
#define SDIOH_MODE_SD4		2
#define CLIENT_INTR			0x100	/* Get rid of this! */
#define SDIOH_SDMMC_MAX_SG_ENTRIES	32

struct sdioh_info {
	osl_t		*osh;			/* osh handler */
	void		*bcmsdh;		/* upper layer handle */
	bool		client_intr_enabled;	/* interrupt connnected flag */
	bool		intr_handler_valid;	/* client driver interrupt handler valid */
	sdioh_cb_fn_t	intr_handler;		/* registered interrupt handler */
	void		*intr_handler_arg;	/* argument to call interrupt handler */
	uint16		intmask;		/* Current active interrupts */

	int		intrcount;		/* Client interrupts */
	bool		sd_use_dma;		/* DMA on CMD53 */
	bool		sd_blockmode;		/* sd_blockmode == FALSE => 64 Byte Cmd 53s. */
						/*  Must be on for sd_multiblock to be effective */
	bool		use_client_ints;	/* If this is false, make sure to restore */
	int		sd_mode;		/* SD1/SD4/SPI */
	int		client_block_size[SDIOD_MAX_IOFUNCS];		/* Blocksize */
	uint8		num_funcs;		/* Supported funcs on client */
	uint32		com_cis_ptr;
	uint32		func_cis_ptr[SDIOD_MAX_IOFUNCS];
	bool		use_rxchain;
	struct scatterlist	sg_list[SDIOH_SDMMC_MAX_SG_ENTRIES];
	struct sdio_func	fake_func0;
	struct sdio_func	*func[SDIOD_MAX_IOFUNCS];
	uint		sd_clk_rate;
};

/************************************************************
 * Internal interfaces: per-port references into bcmsdh_sdmmc.c
 */

/* Global message bits */
extern uint sd_msglevel;

/* OS-independent interrupt handler */
extern bool check_client_intr(sdioh_info_t *sd);

/* Core interrupt enable/disable of device interrupts */
extern void sdioh_sdmmc_devintr_on(sdioh_info_t *sd);
extern void sdioh_sdmmc_devintr_off(sdioh_info_t *sd);

/**************************************************************
 * Internal interfaces: bcmsdh_sdmmc.c references to per-port code
 */

/* Register mapping routines */
extern uint32 *sdioh_sdmmc_reg_map(osl_t *osh, int32 addr, int size);
extern void sdioh_sdmmc_reg_unmap(osl_t *osh, int32 addr, int size);

/* Interrupt (de)registration routines */
extern int sdioh_sdmmc_register_irq(sdioh_info_t *sd, uint irq);
extern void sdioh_sdmmc_free_irq(uint irq, sdioh_info_t *sd);

extern sdioh_info_t *sdioh_attach(osl_t *osh, struct sdio_func *func);
extern SDIOH_API_RC sdioh_detach(osl_t *osh, sdioh_info_t *sd);
#endif /* __BCMSDH_SDMMC_H__ */
