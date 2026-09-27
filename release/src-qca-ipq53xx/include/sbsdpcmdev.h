/*
 * Broadcom SiliconBackplane SDIO/PCMCIA hardware-specific
 * device core support
 *
 * Copyright (C) 2009, Broadcom Corporation
 * All Rights Reserved.
 * 
 * THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
 * KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
 * SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
 *
 * $Id: sbsdpcmdev.h,v 13.31.2.2 2008/07/22 21:45:46 Exp $
 */

#ifndef	_sbsdpcmdev_h_
#define	_sbsdpcmdev_h_

/* cpp contortions to concatenate w/arg prescan */
#ifndef PAD
#define	_PADLINE(line)	pad ## line
#define	_XSTR(line)	_PADLINE(line)
#define	PAD		_XSTR(__LINE__)
#endif	/* PAD */


/* core registers */
typedef volatile struct {
	uint32 corecontrol;		/* CoreControl, 0x000, rev8 */
	uint32 corestatus;		/* CoreStatus, 0x004, rev8  */
	uint32 PAD[1];
	uint32 biststatus;		/* BistStatus, 0x00c, rev8  */

	/* PCMCIA access */
	uint16 pcmciamesportaladdr;	/* PcmciaMesPortalAddr, 0x010, rev8   */
	uint16 PAD[1];
	uint16 pcmciamesportalmask;	/* PcmciaMesPortalMask, 0x014, rev8   */
	uint16 PAD[1];
	uint16 pcmciawrframebc;		/* PcmciaWrFrameBC, 0x018, rev8   */
	uint16 PAD[1];
	uint16 pcmciaunderflowtimer;	/* PcmciaUnderflowTimer, 0x01c, rev8   */
	uint16 PAD[1];

	/* interrupt */
	uint32 intstatus;		/* IntStatus, 0x020, rev8   */
	uint32 hostintmask;		/* IntHostMask, 0x024, rev8   */
	uint32 intmask;			/* IntSbMask, 0x028, rev8   */
	uint32 sbintstatus;		/* SBIntStatus, 0x02c, rev8   */
	uint32 sbintmask;		/* SBIntMask, 0x030, rev8   */
	uint32 PAD[3];
	uint32 tosbmailbox;		/* ToSBMailbox, 0x040, rev8   */
	uint32 tohostmailbox;		/* ToHostMailbox, 0x044, rev8   */
	uint32 tosbmailboxdata;		/* ToSbMailboxData, 0x048, rev8   */
	uint32 tohostmailboxdata;	/* ToHostMailboxData, 0x04c, rev8   */

	/* synchronized access to registers in SDIO clock domain */
	uint32 sdioaccess;		/* SdioAccess, 0x050, rev8   */
	uint32 PAD[3];

	/* PCMCIA frame control */
	uint8 pcmciaframectrl;		/* pcmciaFrameCtrl, 0x060, rev8   */
	uint8 PAD[3];
	uint8 pcmciawatermark;		/* pcmciaWaterMark, 0x064, rev8   */
	uint8 PAD[155];

	/* interrupt batching control */
	uint32 intrcvlazy;		/* IntRcvLazy, 0x100, rev8 */
	uint32 PAD[3];

	/* counters */
	uint32 cmd52rd;			/* Cmd52RdCount, 0x110, rev8, SDIO: cmd52 reads */
	uint32 cmd52wr;			/* Cmd52WrCount, 0x114, rev8, SDIO: cmd52 writes */
	uint32 cmd53rd;			/* Cmd53RdCount, 0x118, rev8, SDIO: cmd53 reads */
	uint32 cmd53wr;			/* Cmd53WrCount, 0x11c, rev8, SDIO: cmd53 writes */
	uint32 abort;			/* AbortCount, 0x120, rev8, SDIO: aborts */
	uint32 datacrcerror;		/* DataCrcErrorCount, 0x124, rev8, SDIO: frames w/bad CRC */
	uint32 rdoutofsync;		/* RdOutOfSyncCount, 0x128, rev8, SDIO/PCMCIA: Rd Frm OOS */
	uint32 wroutofsync;		/* RdOutOfSyncCount, 0x12c, rev8, SDIO/PCMCIA: Wr Frm OOS */
	uint32 writebusy;		/* WriteBusyCount, 0x130, rev8, SDIO: dev asserted "busy" */
	uint32 readwait;		/* ReadWaitCount, 0x134, rev8, SDIO: read: no data avail */
	uint32 readterm;		/* ReadTermCount, 0x138, rev8, SDIO: rd frm terminates */
	uint32 writeterm;		/* WriteTermCount, 0x13c, rev8, SDIO: wr frm terminates */
	uint32 PAD[40];
	uint32 clockctlstatus;		/* ClockCtlStatus, 0x1e0, rev8 */
	uint32 PAD[7];

	/* DMA engines */
	dma32regp_t dmaregs;		/* DMA Regs, 0x200-0x21c, rev8 */
	dma32diag_t dmafifo;		/* DMA Diagnostic Regs, 0x220-0x22c */
	uint32 PAD[116];

	/* SDIO/PCMCIA CIS region */
	char cis[512];			/* 512 byte CIS, 0x400-0x5ff, rev6 */

	/* PCMCIA function control registers */
	char pcmciafcr[256];		/* PCMCIA FCR, 0x600-6ff, rev6 */
	uint16 PAD[55];

	/* PCMCIA backplane access */
	uint16 backplanecsr;		/* BackplaneCSR, 0x76E, rev6 */
	uint16 backplaneaddr0;		/* BackplaneAddr0, 0x770, rev6 */
	uint16 backplaneaddr1;		/* BackplaneAddr1, 0x772, rev6 */
	uint16 backplaneaddr2;		/* BackplaneAddr2, 0x774, rev6 */
	uint16 backplaneaddr3;		/* BackplaneAddr3, 0x776, rev6 */
	uint16 backplanedata0;		/* BackplaneData0, 0x778, rev6 */
	uint16 backplanedata1;		/* BackplaneData1, 0x77a, rev6 */
	uint16 backplanedata2;		/* BackplaneData2, 0x77c, rev6 */
	uint16 backplanedata3;		/* BackplaneData3, 0x77e, rev6 */
	uint16 PAD[31];

	/* sprom "size" & "blank" info */
	uint16 spromstatus;		/* SPROMStatus, 0x7BE, rev2 */
	uint32 PAD[464];

	/* Sonics SiliconBackplane registers */
	sbconfig_t sbconfig;		/* SbConfig Regs, 0xf00-0xfff, rev8 */
} sdpcmd_regs_t;

/* corecontrol */
#define CC_CISRDY	(1L << 0)	/* CIS Ready */
#define CC_BPRESEN	(1L << 1)	/* CCCR RES signal causes backplane reset */
#define CC_F2RDY	(1L << 2)	/* set CCCR IOR2 bit */
#define CC_CLRPADSISO	(1L << 3)	/* clear SDIO pads isolation bit (rev 11) */

/* corestatus */
#define CS_PCMCIAMODE	(1L << 0)	/* Device Mode; 0=SDIO, 1=PCMCIA */
#define CS_SMARTDEV	(1L << 1)	/* 1=smartDev enabled */
#define CS_F2ENABLED	(1L << 2)	/* 1=host has enabled the device */

#define PCMCIA_MES_PA_MASK	0x7fff	/* PCMCIA Message Portal Address Mask */
#define PCMCIA_MES_PM_MASK	0x7fff	/* PCMCIA Message Portal Mask Mask */
#define PCMCIA_WFBC_MASK	0xffff	/* PCMCIA Write Frame Byte Count Mask */
#define PCMCIA_UT_MASK		0x07ff	/* PCMCIA Underflow Timer Mask */

/* intstatus - hw defs */
#define I_SMB_SW0	(1L << 0)	/* To SB Mail S/W interrupt 0 */
#define I_SMB_SW1	(1L << 1)	/* To SB Mail S/W interrupt 1 */
#define I_SMB_SW2	(1L << 2)	/* To SB Mail S/W interrupt 2 */
#define I_SMB_SW3	(1L << 3)	/* To SB Mail S/W interrupt 3 */
#define I_SMB_SW_MASK	0x0000000f	/* To SB Mail S/W interrupts mask */
#define I_SMB_SW_SHIFT	0		/* To SB Mail S/W interrupts shift */
#define I_HMB_SW0	(1L << 4)	/* To Host Mail S/W interrupt 0 */
#define I_HMB_SW1	(1L << 5)	/* To Host Mail S/W interrupt 1 */
#define I_HMB_SW2	(1L << 6)	/* To Host Mail S/W interrupt 2 */
#define I_HMB_SW3	(1L << 7)	/* To Host Mail S/W interrupt 3 */
#define I_HMB_SW_MASK	0x000000f0	/* To Host Mail S/W interrupts mask */
#define I_HMB_SW_SHIFT	4		/* To Host Mail S/W interrupts shift */
#define I_WR_OOSYNC	(1L << 8)	/* Write Frame Out Of Sync */
#define I_RD_OOSYNC	(1L << 9)	/* Read Frame Out Of Sync */
#define	I_PC		(1L << 10)	/* descriptor error */
#define	I_PD		(1L << 11)	/* data error */
#define	I_DE		(1L << 12)	/* Descriptor protocol Error */
#define	I_RU		(1L << 13)	/* Receive descriptor Underflow */
#define	I_RO		(1L << 14)	/* Receive fifo Overflow */
#define	I_XU		(1L << 15)	/* Transmit fifo Underflow */
#define	I_RI		(1L << 16)	/* Receive Interrupt */
#define I_BUSPWR	(1L << 17)	/* SDIO Bus Power Change (rev 9) */
#define	I_XI		(1L << 24)	/* Transmit Interrupt */
#define I_RF_TERM	(1L << 25)	/* Read Frame Terminate */
#define I_WF_TERM	(1L << 26)	/* Write Frame Terminate */
#define I_PCMCIA_XU	(1L << 27)	/* PCMCIA Transmit FIFO Underflow */
#define I_SBINT		(1L << 28)	/* sbintstatus Interrupt */
#define I_CHIPACTIVE	(1L << 29)	/* chip transitioned from doze to active state */
#define I_SRESET	(1L << 30)	/* CCCR RES interrupt */
#define I_IOE2		(1L << 31)	/* CCCR IOE2 Bit Changed */
#define	I_ERRORS	(I_PC | I_PD | I_DE | I_RU | I_RO | I_XU)	/* DMA Errors */
#define I_DMA		(I_RI | I_XI | I_ERRORS)

/* sbintstatus */
#define I_SB_SERR	(1 << 8)	/* Backplane SError (write) */
#define I_SB_RESPERR	(1 << 9)	/* Backplane Response Error (read) */
#define I_SB_SPROMERR	(1 << 10)	/* Error accessing the sprom */

/* sdioaccess */
#define SDA_DATA_MASK	0x000000ff	/* Read/Write Data Mask */
#define SDA_ADDR_MASK	0x000fff00	/* Read/Write Address Mask */
#define SDA_ADDR_SHIFT	8		/* Read/Write Address Shift */
#define SDA_WRITE	0x01000000	/* Write bit  */
#define SDA_READ	0x00000000	/* Write bit cleared for Read */
#define SDA_BUSY	0x80000000	/* Busy bit */

/* sdioaccess-accessible register address spaces */
#define SDA_CCCR_SPACE		0x000	/* sdioAccess CCCR register space */
#define SDA_F1_FBR_SPACE	0x100	/* sdioAccess F1 FBR register space */
#define SDA_F2_FBR_SPACE	0x200	/* sdioAccess F2 FBR register space */
#define SDA_F1_REG_SPACE	0x300	/* sdioAccess F1 core-specific register space */

/* SDA_F1_REG_SPACE sdioaccess-accessible F1 reg space register offsets */
#define SDA_CHIPCONTROLDATA	0x006	/* ChipControlData */
#define SDA_CHIPCONTROLENAB	0x007	/* ChipControlEnable */
#define SDA_F2WATERMARK		0x008	/* Function 2 Watermark */
#define SDA_DEVICECONTROL	0x009	/* DeviceControl */
#define SDA_SBADDRLOW		0x00a	/* SbAddrLow */
#define SDA_SBADDRMID		0x00b	/* SbAddrMid */
#define SDA_SBADDRHIGH		0x00c	/* SbAddrHigh */
#define SDA_FRAMECTRL		0x00d	/* FrameCtrl */
#define SDA_CHIPCLOCKCSR	0x00e	/* ChipClockCSR */
#define SDA_SDIOPULLUP		0x00f	/* SdioPullUp */
#define SDA_SDIOWRFRAMEBCLOW	0x019	/* SdioWrFrameBCLow */
#define SDA_SDIOWRFRAMEBCHIGH	0x01a	/* SdioWrFrameBCHigh */
#define SDA_SDIORDFRAMEBCLOW	0x01b	/* SdioRdFrameBCLow */
#define SDA_SDIORDFRAMEBCHIGH	0x01c	/* SdioRdFrameBCHigh */

/* SDA_F2WATERMARK */
#define SDA_F2WATERMARK_MASK	0x7f	/* F2Watermark Mask */

/* SDA_SBADDRLOW */
#define SDA_SBADDRLOW_MASK	0x80	/* SbAddrLow Mask */

/* SDA_SBADDRMID */
#define SDA_SBADDRMID_MASK	0xff	/* SbAddrMid Mask */

/* SDA_SBADDRHIGH */
#define SDA_SBADDRHIGH_MASK	0xff	/* SbAddrHigh Mask */

/* SDA_FRAMECTRL */
#define SFC_RF_TERM	(1 << 0)	/* Read Frame Terminate */
#define SFC_WF_TERM	(1 << 1)	/* Write Frame Terminate */
#define SFC_CRC4WOOS	(1 << 2)	/* HW reports CRC error for write out of sync */
#define SFC_ABORTALL	(1 << 3)	/* Abort cancels all in-progress frames */

/* pcmciaframectrl */
#define PFC_RF_TERM	(1 << 0)	/* Read Frame Terminate */
#define PFC_WF_TERM	(1 << 1)	/* Write Frame Terminate */

/* intrcvlazy */
#define	IRL_TO_MASK	0x00ffffff	/* timeout */
#define	IRL_FC_MASK	0xff000000	/* frame count */
#define	IRL_FC_SHIFT	24		/* frame count */

/* rx header */
typedef volatile struct {
	uint16 len;
	uint16 flags;
} sdpcmd_rxh_t;

/* rx header flags */
#define RXF_CRC		0x0001		/* CRC error detected */
#define RXF_WOOS	0x0002		/* write frame out of sync */
#define RXF_WF_TERM	0x0004		/* write frame terminated */
#define RXF_ABORT	0x0008		/* write frame aborted */
#define RXF_DISCARD	(RXF_CRC | RXF_WOOS | RXF_WF_TERM | RXF_ABORT)	/* bad frame */

/* HW frame tag */
#define SDPCM_FRAMETAG_LEN	4	/* HW frametag: 2 bytes len, 2 bytes check val */

/*
 * *******************************************************************
 *                     SOFTWARE DEFINITIONS
 * *******************************************************************
 */

/* intstatus register */
#define I_SMB_NAK	I_SMB_SW0	/* 0x1 - To SB Mailbox Frame NAK */
#define I_SMB_INT_ACK	I_SMB_SW1	/* 0x2 - To SB Mailbox Host Interrupt ACK */
#define I_SMB_USE_OOB	I_SMB_SW2	/* 0x4 - To SB Mailbox Use OOB Wakeup */
#define I_SMB_DEV_INT	I_SMB_SW3	/* 0x8 - To SB Mailbox Miscellaneous Interrupt */

#define I_HMB_FC_STATE	I_HMB_SW0	/* 0x10 - To Host Mailbox Flow Control State */
#define I_HMB_FC_CHANGE	I_HMB_SW1	/* 0x20 - To Host Mailbox Flow Control State Changed */
#define I_HMB_FRAME_IND	I_HMB_SW2	/* 0x40 - To Host Mailbox Frame Indication */
#define I_HMB_HOST_INT	I_HMB_SW3	/* 0x80 - To Host Mailbox Miscellaneous Interrupt */

#define I_TOSBMAIL	(I_SMB_NAK | I_SMB_INT_ACK | I_SMB_USE_OOB | I_SMB_DEV_INT)
#define I_TOHOSTMAIL	(I_HMB_FC_CHANGE | I_HMB_FRAME_IND | I_HMB_HOST_INT)

/* tosbmailbox */
#define SMB_NAK		(1L << 0)	/* To SB Mailbox Frame NAK */
#define SMB_INT_ACK	(1L << 1)	/* To SB Mailbox Host Interrupt ACK */
#define SMB_USE_OOB	(1L << 2)	/* To SB Mailbox Use OOB Wakeup */
#define SMB_DEV_INT	(1L << 3)	/* To SB Mailbox Miscellaneous Interrupt */
#define SMB_MASK	0x0000000f	/* To SB Mailbox Mask */

/* tohostmailbox */
#define HMB_FC_ON	(1L << 0)	/* To Host Mailbox Flow Control State=ON */
#define HMB_FC_CHANGE	(1L << 1)	/* To Host Mailbox Flow Control State Changed */
#define HMB_FRAME_IND	(1L << 2)	/* To Host Mailbox Frame Indication */
#define HMB_HOST_INT	(1L << 3)	/* To Host Mailbox Miscellaneous Interrupt */
#define HMB_MASK	0x0000000f	/* To Host Mailbox Mask */

/* tohostmailboxdata */
#define HMB_DATA_NAKHANDLED	1	/* we're ready to retransmit NAK'd frame to host */
#define HMB_DATA_DEVREADY	2	/* we're ready to to talk to host after enable */
#define HMB_DATA_FC		4	/* per prio flowcontrol update flag to host */
#define HMB_DATA_FWREADY	8	/* firmware is ready for protocol activity */

#define HMB_DATA_FCDATA_MASK	0xff	/* per prio flowcontrol data */
#define HMB_DATA_FCDATA_SHIFT	24	/* per prio flowcontrol data */

#define HMB_DATA_VERSION_MASK	0xff	/* device protocol version (with devready) */
#define HMB_DATA_VERSION_SHIFT	16	/* device protocol version (with devready) */

/* tosbmailboxdata */
#define SMB_DATA_VERSION_MASK	0xff	/* host protocol version (with F2 enable) */
#define SMB_DATA_VERSION_SHIFT	16	/* host protocol version (with F2 enable) */

/* current protocol version */
#define SDPCM_PROT_VERSION	4

/* SW frame header */
#define SDPCM_SEQUENCE_MASK		0x000000ff	/* Sequence Number Mask */
#define SDPCM_PACKET_SEQUENCE(p) (((uint8 *)p)[0] & 0xff) /* p starts w/SW Header */

#define SDPCM_CHANNEL_MASK		0x00000f00	/* Channel Number Mask */
#define SDPCM_CHANNEL_SHIFT		8		/* Channel Number Shift */
#define SDPCM_PACKET_CHANNEL(p) (((uint8 *)p)[1] & 0x0f) /* p starts w/SW Header */

#define SDPCM_FLAGS_MASK		0x0000f000	/* Mask of flag bits */
#define SDPCM_FLAGS_SHIFT		12		/* Flag bits shift */
#define SDPCM_PACKET_FLAGS(p) ((((uint8 *)p)[1] & 0xf0) >> 4) /* p starts w/SW Header */

/* Next Read Len: lookahead length of next frame, in 16-byte units (rounded up) */
#define SDPCM_NEXTLEN_MASK		0x00ff0000	/* Next Read Len Mask */
#define SDPCM_NEXTLEN_SHIFT		16		/* Next Read Len Shift */
#define SDPCM_NEXTLEN_VALUE(p) ((((uint8 *)p)[2] & 0xff) << 4) /* p starts w/SW Header */
#define SDPCM_NEXTLEN_OFFSET		2

/* Data Offset from SOF (HW Tag, SW Tag, Pad) */
#define SDPCM_DOFFSET_OFFSET		3		/* Data Offset */
#define SDPCM_DOFFSET_VALUE(p) 		(((uint8 *)p)[SDPCM_DOFFSET_OFFSET] & 0xff)
#define SDPCM_DOFFSET_MASK		0xff000000
#define SDPCM_DOFFSET_SHIFT		24

#define SDPCM_FCMASK_OFFSET		4		/* Flow control */
#define SDPCM_FCMASK_VALUE(p)		(((uint8 *)p)[SDPCM_FCMASK_OFFSET ] & 0xff)
#define SDPCM_WINDOW_OFFSET		5		/* Credit based fc */
#define SDPCM_WINDOW_VALUE(p)		(((uint8 *)p)[SDPCM_WINDOW_OFFSET] & 0xff)
#define SDPCM_VERSION_OFFSET		6		/* Version # */
#define SDPCM_VERSION_VALUE(p)		(((uint8 *)p)[SDPCM_VERSION_OFFSET] & 0xff)
#define SDPCM_UNUSED_OFFSET		7		/* Spare */
#define SDPCM_UNUSED_VALUE(p)		(((uint8 *)p)[SDPCM_UNUSED_OFFSET] & 0xff)

#define SDPCM_SWHEADER_LEN	8	/* SW header is 64 bits */

/* logical channel numbers */
#define SDPCM_CONTROL_CHANNEL	0	/* Control Request/Response Channel Id */
#define SDPCM_EVENT_CHANNEL	1	/* Asyc Event Indication Channel Id */
#define SDPCM_DATA_CHANNEL	2	/* Data Xmit/Recv Channel Id */
#define SDPCM_GLOM_CHANNEL	3	/* For coalesced packets (superframes) */
#define SDPCM_TEST_CHANNEL	15	/* Reserved for test/debug packets */
#define SDPCM_MAX_CHANNEL	15

#define SDPCM_SEQUENCE_WRAP	256	/* wrap-around val for eight-bit frame seq number */

/* For GLOM_CHANNEL frames, use a flag to indicate descriptor frame */
#define SDPCM_GLOMDESC_FLAG	0x00008000	/* Superframe descriptor mask */
#define SDPCM_GLOMDESC(p)	(((uint8 *)p)[1] & 0x80)

/* For TEST_CHANNEL packets, define another 4-byte header */
#define SDPCM_TEST_HDRLEN	4	/* Generally: Cmd(1), Ext(1), Len(2);
					 * Semantics of Ext byte depend on command.
					 * Len is current or requested frame length, not
					 * including test header; sent little-endian.
					 */
#define SDPCM_TEST_DISCARD	0x01	/* Receiver discards. Ext is a pattern id. */
#define SDPCM_TEST_ECHOREQ	0x02	/* Echo request. Ext is a pattern id. */
#define SDPCM_TEST_ECHORSP	0x03	/* Echo response. Ext is a pattern id. */
#define SDPCM_TEST_BURST	0x04	/* Receiver to send a burst. Ext is a frame count */
#define SDPCM_TEST_SEND		0x05	/* Receiver sets send mode. Ext is boolean on/off */

/* Handy macro for filling in datagen packets with a pattern */
#define SDPCM_TEST_FILL(byteno, id)	((uint8)(id + byteno))


/* software counters (copy of hardware counters plus additional ones) */
typedef volatile struct {
	uint32 cmd52rd;		/* Cmd52RdCount, SDIO: cmd52 reads */
	uint32 cmd52wr;		/* Cmd52WrCount, SDIO: cmd52 writes */
	uint32 cmd53rd;		/* Cmd53RdCount, SDIO: cmd53 reads */
	uint32 cmd53wr;		/* Cmd53WrCount, SDIO: cmd53 writes */
	uint32 abort;		/* AbortCount, SDIO: aborts */
	uint32 datacrcerror;	/* DataCrcErrorCount, SDIO: frames w/CRC error */
	uint32 rdoutofsync;	/* RdOutOfSyncCount, SDIO/PCMCIA: Rd Frm out of sync */
	uint32 wroutofsync;	/* RdOutOfSyncCount, SDIO/PCMCIA: Wr Frm out of sync */
	uint32 writebusy;	/* WriteBusyCount, SDIO: device asserted "busy" */
	uint32 readwait;	/* ReadWaitCount, SDIO: no data ready for a read cmd */
	uint32 readterm;	/* ReadTermCount, SDIO: read frame termination cmds */
	uint32 writeterm;	/* WriteTermCount, SDIO: write frames termination cmds */
	uint32 rxdescuflo;	/* receive descriptor underflows */
	uint32 rxfifooflo;	/* receive fifo overflows */
	uint32 txfifouflo;	/* transmit fifo underflows */
	uint32 runt;		/* runt (too short) frames recv'd from bus */
	uint32 badlen;		/* frame's rxh len does not match its hw tag len */
	uint32 badcksum;	/* frame's hw tag chksum doesn't agree with len value */
	uint32 seqbreak;	/* break in sequence # space from one rx frame to the next */
	uint32 rxfcrc;		/* frame rx header indicates crc error */
	uint32 rxfwoos;		/* frame rx header indicates write out of sync */
	uint32 rxfwft;		/* frame rx header indicates write frame termination */
	uint32 rxfabort;	/* frame rx header indicates frame aborted */
	uint32 woosint;		/* write out of sync interrupt */
	uint32 roosint;		/* read out of sync interrupt */
	uint32 rftermint;	/* read frame terminate interrupt */
	uint32 wftermint;	/* write frame terminate interrupt */
} sdpcmd_cnt_t;

#endif	/* _sbsdpcmdev_h_ */
