// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2013 Broadcom
 */
/* SPDX-License-Identifier: GPL-2.0+
 *
 * Copyright 2019 Broadcom Ltd.
 */
/*

*/

/*****************************************************************************
 *  Description:
 *      Common code for PMC Linux, U-Boot, and ATF drivers 
 *****************************************************************************/

#include "pmc_drv.h"
#include "asm/arch/BPCM.h"
#include "asm/arch/misc.h"
#include "command.h"

#if IS_BCMCHIP(6858) || defined(PMC_IMPL_3_X)
// #include "bcm_otp.h" // TODO: add back this after implenting otp util
#include "clk_rst.h"
#endif

int pmc_mode = PMC_MODE_DQM;

#ifdef PMC_LOG_IN_DTCM
void pmc_show_log_item(void)
{
	unsigned short i, item_sz;
	volatile char *dtcm = (char *) g_pmc->dtcm_base;  
	volatile Pmc *pmc = (volatile Pmc *)g_pmc->pmc_base;

	item_sz = pmc->ctrl.hostMboxOut >> 16;

	for (i = 0; i < item_sz; i++)
		printk("%c", dtcm[i]);  

	if (item_sz) // reset the item_sz to 0
		pmc->ctrl.hostMboxOut &= 0xffff;
}

void pmc_save_log_item(void)
{
	char *buf = (char *) phys_to_virt(CFG_BOOT_PMC_LOG_ADDR);
	unsigned short *plen = (unsigned short *) buf;
	char *dst = buf + sizeof(*plen) + *plen;
	volatile char *dtcm = (char *) g_pmc->dtcm_base; 
	volatile Pmc *pmc = (Pmc *)g_pmc->pmc_base;
	unsigned short item_sz = pmc->ctrl.hostMboxOut >> 16;
	int i = 0;
	
	while (i < item_sz && *plen < CFG_BOOT_PMC_LOG_SIZE - sizeof(*plen)) {
		dst[i] = dtcm[i];
		i++;
		(*plen)++;
	}

	if (item_sz) // reset the item_sz to 0
		pmc->ctrl.hostMboxOut &= 0xffff;
}

void pmc_show_live_log(void)
{
	volatile Pmc *pmc = (volatile Pmc *)g_pmc->pmc_base;
	printk("start showing pmc firmware live log, "
			"press any key to stop ...\n");
	pmc->ctrl.hostMboxOut = 1; // request sync dtcm log
	while (!console_status())
		pmc_show_log_item();
	pmc->ctrl.hostMboxOut = 0; // ignore dtcm log
	printk("\n... key pressed, stop showing pmc firmware live log.\n");
}
#endif // #ifdef PMC_LOG_IN_DTCM

static int SendAndWait(TCommand * cmd, TCommand * rsp)
{
#if defined(PMC_ON_HOSTCPU) || defined(PMC_DIRECT_MODE_ONLY)
	rsp->u.cmdGenericParams.params[0] = 0;
	rsp->u.cmdGenericParams.params[1] = 0;
	return 0;
#else
#if IS_BCMCHIP(63138)
	/* translate new cmdID into old cmdID that the pmc will understand NB:
	 * requires cmdIDs below to be the new versions
	 */

	static const unsigned char newToOldcmdIDMap[] = {
		[cmdSetRunState] = 64,	// cmdSetRunState,
		[cmdSetPowerState] = 65,	// cmdSetPowerState,
		[cmdShutdownAllowed] = 66,	// cmdShutdownAllowed,
		[cmdGetSelect0] = 67,	// cmdGetSelect0,
		[cmdGetSelect3] = 68,	// cmdGetSelect3,
		[cmdGetAvsDisableState] = 69,	// cmdGetAvsDisableState,
		[cmdGetPVT] = 70,	// cmdGetPVT,
		[cmdPowerDevOnOff] = 129,	// cmdPowerDevOnOff,
		[cmdPowerZoneOnOff] = 130,	// cmdPowerZoneOnOff,
		[cmdResetDevice] = 131,	// cmdResetDevice,
		[cmdResetZone] = 132,	// cmdResetZone,
		[cmdAllocateG2UDQM] = 133,	// cmdAllocateG2UDQM,
		[cmdQSMAvailable] = 134,	// cmdQSMAvailable,
		[cmdRevision] = 135,	// cmdRevision,
	};

	static int pmc_remap = 0;
#endif
	static uint32_t reqdID = 1;
	int status = kPMC_COMMAND_TIMEOUT;
	TCommand dummy;
	volatile Pmc *pmc = (volatile Pmc *)g_pmc->pmc_base;

#if defined(BOOT_MEMC_SRAM)
	reqdID = 1;
#endif

	/* clear previous rsp data if any */
	while (pmc->dqm.notEmptySts & PMC_DQM_RPL_STS) {
		if (!rsp)
			rsp = &dummy;

		rsp->word0.Reg32 = pmc->dqmQData[PMC_DQM_RPL_NUM].word[0];
		rsp->word1.Reg32 = pmc->dqmQData[PMC_DQM_RPL_NUM].word[1];
		rsp->u.cmdGenericParams.params[0] =
		    pmc->dqmQData[PMC_DQM_RPL_NUM].word[2];
		rsp->u.cmdGenericParams.params[1] =
		    pmc->dqmQData[PMC_DQM_RPL_NUM].word[3];

		printk
		    ("PMC reqdID=%d previous rsp.word[0-3]=0x[%08x %08x %08x %08x] status=%d\n",
		     reqdID, rsp->word0.Reg32, rsp->word1.Reg32,
		     rsp->u.cmdGenericParams.params[0],
		     rsp->u.cmdGenericParams.params[1], rsp->word0.Bits.error);
	}

#if IS_BCMCHIP(63138)
	if (pmc_remap && cmd->word0.Bits.cmdID < sizeof newToOldcmdIDMap &&
	    newToOldcmdIDMap[cmd->word0.Bits.cmdID])
		cmd->word0.Bits.cmdID = newToOldcmdIDMap[cmd->word0.Bits.cmdID];
#endif

#ifdef PMC_LOG_IN_DTCM
	if (cmd->word0.Bits.cmdID == cmdCloseAVS)
		pmc->ctrl.hostMboxOut = 1; // request sync dtcm log
#endif

	cmd->word0.Bits.msgID = reqdID;

	/* send the command */
	pmc->dqmQData[PMC_DQM_REQ_NUM].word[0] = cmd->word0.Reg32;
	pmc->dqmQData[PMC_DQM_REQ_NUM].word[1] = cmd->word1.Reg32;
	pmc->dqmQData[PMC_DQM_REQ_NUM].word[2] =
	    cmd->u.cmdGenericParams.params[0];
	pmc->dqmQData[PMC_DQM_REQ_NUM].word[3] =
	    cmd->u.cmdGenericParams.params[1];

#ifdef CONFIG_BRCM_IKOS
	/* We do not enable PMC TIMER here for IKOS, or it will wait forever */
	while (!(pmc->dqm.notEmptySts & PMC_DQM_RPL_STS)) ;
#elif defined(PMC_IMPL_3_X)
#ifdef PMC_LOG_IN_DTCM
	if (cmd->word0.Bits.cmdID == cmdCloseAVS) {
		while (!(pmc->dqm.notEmptySts & PMC_DQM_RPL_STS))
			pmc_show_log_item();

		pmc->ctrl.hostMboxOut = 0; // ignore dtcm log
	} 
	else
#endif // #ifdef PMC_LOG_IN_DTCM
	{
		pmc->ctrl.gpTmr0Ctl = ((1 << 31) | (1 << 29) |
				((400000 << 1) & 0x1fffffff));	// 400ms

		while (!(pmc->dqm.notEmptySts & PMC_DQM_RPL_STS) &&
				(pmc->ctrl.gpTmr0Ctl & (1 << 31))) ;
	}
#else
	pmc->ctrl.gpTmr2Ctl = ((1 << 31) | (1 << 29) | 400000);	// 400ms

	while (!(pmc->dqm.notEmptySts & PMC_DQM_RPL_STS) &&
	       (pmc->ctrl.gpTmr2Ctl & (1 << 31))) {
#if (IS_BCMCHIP(63148) || IS_BCMCHIP(4908))
		/* Do not tight poll the PMC registers for longer command */
		if (cmd->word0.Bits.cmdID == cmdCloseAVS)
			udelay(1000);
#endif
	}
#endif /* CONFIG_BRCM_IKOS */

	if (pmc->dqm.notEmptySts & PMC_DQM_RPL_STS) {
		if (!rsp)
			rsp = &dummy;

		/* command didn't timeout, fill in the response */
		rsp->word0.Reg32 = pmc->dqmQData[PMC_DQM_RPL_NUM].word[0];
		rsp->word1.Reg32 = pmc->dqmQData[PMC_DQM_RPL_NUM].word[1];
		rsp->u.cmdGenericParams.params[0] =
		    pmc->dqmQData[PMC_DQM_RPL_NUM].word[2];
		rsp->u.cmdGenericParams.params[1] =
		    pmc->dqmQData[PMC_DQM_RPL_NUM].word[3];

		if (rsp->word0.Bits.msgID == reqdID)
			status = rsp->word0.Bits.error;
		else
			status = kPMC_MESSAGE_ID_MISMATCH;

		if (status != kPMC_NO_ERROR)
			printk
			    ("PMC reqdID=%d error=%d rsp.word[0-3]=0x[%08x %08x %08x %08x]\n",
			     reqdID, status, rsp->word0.Reg32, rsp->word1.Reg32,
			     rsp->u.cmdGenericParams.params[0],
			     rsp->u.cmdGenericParams.params[1]);
	}

	reqdID = (reqdID + 1) & 0xff;

	return status;
#endif
}

static int SendCmd(TCommand * cmd, int cmdID, int devAddr, int zone, int island,
		   TCommand * rsp)
{
	cmd->word0.Reg32 = 0;
	cmd->word0.Bits.cmdID = cmdID;
	cmd->word1.Reg32 = 0;
	cmd->word1.Bits.devAddr = devAddr;
	cmd->word1.Bits.zoneIdx = zone;
	cmd->word1.Bits.island = island;

	return SendAndWait(cmd, rsp);
}

int SendCommand(int cmdID, int devAddr, int zone, int island, uint32_t word2,
		uint32_t word3, TCommand * rsp)
{
	TCommand cmd;

	cmd.u.cmdGenericParams.params[0] = word2;
	cmd.u.cmdGenericParams.params[1] = word3;

	return SendCmd(&cmd, cmdID, devAddr, zone, island, rsp);
}

int GetRevision(uint32_t * change, uint32_t * revision)
{
	if (pmc_mode == PMC_MODE_DQM) {
		TCommand rsp;
		int status = SendCommand(cmdRevision, 0, 0, 0, 0, 0, &rsp);

		if (status == kPMC_NO_ERROR) {
			*change = rsp.u.cmdResponse.word2;
			*revision = rsp.u.cmdResponse.word3;
		}

		return status;
	} else
		return kPMC_INVALID_COMMAND;
}

#if defined(PMC_IMPL_3_X) || defined(PMC_ON_HOSTCPU) || defined(PMC_DIRECT_MODE_ONLY)
#define KEYHOLE_IDX 0

int read_bpcm_reg_direct(int devAddr, int wordOffset, uint32_t * value)
{
	int status = kPMC_NO_ERROR;
	int bus = (devAddr >> PMB_BUS_ID_SHIFT) & 0x3;
	uint32_t address, ctlSts;
	volatile PmbBus *pmb;
	volatile keyholeReg *keyhole;

#if defined(PMC_ON_HOSTCPU) || defined(PMC_DIRECT_MODE_ONLY)
	pmb = &((volatile Pmc *)(g_pmc->pmc_base))->pmb;
#else
	pmb = &((volatile Procmon *)(g_pmc->procmon_base))->pmb;
#endif
	keyhole = &pmb->keyhole[KEYHOLE_IDX];

	address =
	    ((devAddr & 0xff) * ((pmb->config >> PMB_NUM_REGS_SHIFT) & PMB_NUM_REGS_MASK)) |
	    (wordOffset);

	keyhole->control =
	    PMC_PMBM_START | (bus << PMC_PMBM_BUS_SHIFT) | (PMC_PMBM_Read) | address;
	ctlSts = keyhole->control;
	while (ctlSts & PMC_PMBM_BUSY)
		ctlSts = keyhole->control;	/*wait for completion */

	if (ctlSts & PMC_PMBM_TIMEOUT)
		status = kPMC_COMMAND_TIMEOUT;
	else
		*value = keyhole->rd_data;

	return status;
}

int write_bpcm_reg_direct(int devAddr, int wordOffset, uint32_t value)
{
	int status = kPMC_NO_ERROR;
	int bus = (devAddr >> PMB_BUS_ID_SHIFT) & 0x3;
	uint32_t address, ctlSts;
	volatile PmbBus *pmb;
	volatile keyholeReg *keyhole;

#if defined(PMC_ON_HOSTCPU) || defined(PMC_DIRECT_MODE_ONLY)
	pmb = &((volatile Pmc *)(g_pmc->pmc_base))->pmb;
#else
	pmb = &((volatile Procmon *)(g_pmc->procmon_base))->pmb;
#endif
	keyhole = &pmb->keyhole[KEYHOLE_IDX];

	address =
	    ((devAddr & 0xff) * ((pmb->config >> PMB_NUM_REGS_SHIFT) & PMB_NUM_REGS_MASK)) |
	    (wordOffset);
	keyhole->wr_data = value;
	keyhole->control =
	    PMC_PMBM_START | (bus << PMC_PMBM_BUS_SHIFT) | (PMC_PMBM_Write) | address;

	ctlSts = keyhole->control;
	while (ctlSts & PMC_PMBM_BUSY)
		ctlSts = keyhole->control;	/*wait for completion */

	if (ctlSts & PMC_PMBM_TIMEOUT)
		status = kPMC_COMMAND_TIMEOUT;

	return status;
}
#if !defined(PMC_DIRECT_MODE_ONLY)
static int is_pvtmon_enabled = 0;
static void pvtmon_enable(void)
{
	uint32_t index;
	uint32_t target;
#define PVTCLKDIV (5 << 8)

	// set up analog hardware to enable counting
	target = PVTCLKDIV | 4;	// 4 = clk_en|!pwr_dn|rstb
	write_bpcm_reg_direct(PMB_ADDR_PVTMON, 17, target);
	for (index = 0; index < 100000; index++) ;
	target = PVTCLKDIV | 5;	// 5 = clk_en|!pwr_dn|!rstb
	write_bpcm_reg_direct(PMB_ADDR_PVTMON, 17, target);

	// set sample size of ALL counters except TEST (7)
	// set enable bit for ONLY 0 (temperature and external) and V1p0<0> - these will be the only ones we use during Match and Closure
	for (index = 0; index < 8; index++) {
		target = (0x5 << 24) | 0x80000000;	//pvtmon samples (32 [2^5]) + enable
		write_bpcm_reg_direct(PMB_ADDR_PVTMON, 24 + index, target);
		read_bpcm_reg_direct(PMB_ADDR_PVTMON, 24 + index, &target);	// read once to clear valid bit
	}

	// enable accumulation:
	// 0x00000801 = skip_len = 8, enable accumulation
	target = 0x00000801;
	write_bpcm_reg_direct(PMB_ADDR_PVTMON, 20, target);
	is_pvtmon_enabled = 1;
}

#ifndef unlikely
#define unlikely(x)	(x)
#endif
static int read_pvt_direct(int index, int *val)
{
	int status;
	uint32_t target;

	// assuming PVTMON already enabled in DQM mode
	if (unlikely((pmc_mode != PMC_MODE_DQM) && !is_pvtmon_enabled))
		pvtmon_enable();

	status = read_bpcm_reg_direct(PMB_ADDR_PVTMON, 24 + index, &target);
	if (unlikely(status))
		goto EXIT;

	while (!(target & (1 << 18))) {
		// the value SHOULD be valid immediatly, but just in case...
		status =
		    read_bpcm_reg_direct(PMB_ADDR_PVTMON, 24 + index, &target);
		if (unlikely(status))
			goto EXIT;
	}

	*val = target & 0x3ff;

EXIT:
	return status;
}

int GetPVT(int sel, int island, int *value)
{
	return read_pvt_direct(sel, value);
}
#else
int GetPVT(int sel, int island, int *value)
{
	return -1;
}
#endif
#else // #if defined(PMC_IMPL_3_X) || defined(PMC_ON_HOSTCPU)
int read_bpcm_reg_direct(int devAddr, int wordOffset, uint32_t * value)
{
	int status = kPMC_NO_ERROR;
	int bus = (devAddr >> PMB_BUS_ID_SHIFT) & 0x3;
	Procmon *procmon = (Procmon *)g_pmc->procmon_base;
	volatile PMBMaster *pmbm_ptr = &(procmon->PMBM[bus]);

	if (bus >= PMB_BUS_MAX)
		return kPMC_INVALID_BUS;

	/* Make sure PMBM is not busy */

	pmbm_ptr->ctrl = PMC_PMBM_START | PMC_PMBM_Read |
	    ((devAddr & 0xff) << 12) | wordOffset;

	while (pmbm_ptr->ctrl & PMC_PMBM_START) ;

	if (pmbm_ptr->ctrl & PMC_PMBM_TIMEOUT)
		status = kPMC_COMMAND_TIMEOUT;
	else
		*value = pmbm_ptr->rd_data;

	return status;
}

int write_bpcm_reg_direct(int devAddr, int wordOffset, uint32_t value)
{
	int bus = (devAddr >> PMB_BUS_ID_SHIFT) & 0x3;
	int status = kPMC_NO_ERROR;

	Procmon *procmon = (Procmon *)g_pmc->procmon_base;
	volatile PMBMaster *pmbm_ptr = &(procmon->PMBM[bus]);

	if (bus >= PMB_BUS_MAX)
		return kPMC_INVALID_BUS;

	pmbm_ptr->wr_data = value;
	pmbm_ptr->ctrl = PMC_PMBM_START | PMC_PMBM_Write |
	    ((devAddr & 0xff) << 12) | wordOffset;

	while (pmbm_ptr->ctrl & PMC_PMBM_START) ;

	if (pmbm_ptr->ctrl & PMC_PMBM_TIMEOUT)
		status = kPMC_COMMAND_TIMEOUT;

	return status;
}

int GetPVT(int sel, int island, int *value)
{
	if (pmc_mode == PMC_MODE_DQM) {
		TCommand rsp;
		int status = SendCommand(cmdGetPVT, 0, 0, island, sel, 0, &rsp);

		if (status == kPMC_NO_ERROR)
			*value = rsp.u.cmdResponse.word2;
		return status;
	} else
		return kPMC_INVALID_COMMAND;
}
#endif // #if defined(PMC_IMPL_3_X) || defined(PMC_ON_HOSTCPU)

/* GetRCalSetting reads resistor value and calculates the calibration setting for the SGMII, PCIe, SATA
   and USB HW blocks that requires resistor calibration to meet specification requirement.
   The HW driver should call this function and write the value to calibration register during initialzation

   input param: resistor - the resistor type that specific HW calibration care about:
   inout param: rcal - 4 bit RCAL value [0 -15] representing the increment or decrement to the internal resistor.   
   return: kPMC_NO_ERROR or kPMC_INVALID_COMMAND if error condition
*/
int GetRCalSetting(int resistor, int *rcal)
{
#if defined(PMC_GETRCAL_SUPPORT) 
	int res_int, res_ext, ratio, ratio1;
	int rc = kPMC_NO_ERROR;

	if (pmc_mode == PMC_MODE_DQM) {
#if IS_BCMCHIP(63178) || IS_BCMCHIP(47622) || IS_BCMCHIP(6756)
		TCommand rsp;
		int status;

		/* make sure the resistor selection is valid */
		/* Not supporting the other resistors because there is no room in DQM RSP */
		if (resistor < RCAL_1UM_HORZ || resistor > RCAL_1UM_VERT)
			return kPMC_INVALID_COMMAND;

		status = SendCommand(cmdGetRMON, 0, 0, 0, 0, 0, &rsp);
		if (status != kPMC_NO_ERROR)
			return kPMC_INVALID_COMMAND;

		/* make sure the resistor data is collected by PMC */
		if (!(rsp.u.cmdResponse.word3 & (1 << 16)))
			return kPMC_INVALID_STATE;

		res_int = rsp.u.cmdResponse.word2;
		res_ext = rsp.u.cmdResponse.word3 & 0xffff;
#else
        Procmon *procmon = (Procmon *)g_pmc->procmon_base;

		/* make sure the resistor selection is valid */
		if (resistor < RCAL_0P25UM_HORZ || resistor > RCAL_1UM_VERT)
			return kPMC_INVALID_COMMAND;

		/* make sure the resistor data is collected by PMC */
		if ((procmon->Misc.
		     misc[PMMISC_RMON_EXT_REG] & PMMISC_RMON_VALID_MASK) == 0)
			return kPMC_INVALID_COMMAND;

		res_int = procmon->Misc.misc[resistor >> 1];
		res_ext = (procmon->Misc.misc[PMMISC_RMON_EXT_REG]) & 0xffff;
#endif
		if (resistor % 2)
			res_int >>= 16;
		res_int &= 0xffff;

		/* Return error if the res_ext saturated such as
		   the ext resistor is not available */
		if (res_ext > 0x3a0) {
			printk("%s:res_ext value 0x%x is saturated!\n",
			       __func__, res_ext);
			return kPMC_INVALID_STATE;
		}

		/* Ratio = CLAMP((INT) (128.0 * V(REXT)/V(RINT)), 0, 255) */
		ratio = (128 * res_ext) / res_int;
		if (ratio > 255)
			ratio = 255;

		/* Ratio1 = CLAMP(128 - (Ratio - 128) * 4, 0, 255) */
		ratio1 = (128 - (ratio - 128) * 4);
		if (ratio1 < 0)
			ratio1 = 0;
		if (ratio1 > 255)
			ratio1 = 255;

		/* convert to 4 bit rcal setting value */
		*rcal = (ratio1 >> 4) & 0xf;
#if 1
		printk
		    ("getrcal for res select %d, int %d, ext %d, ratio %d ratio1 %d, rcal %d\n",
		     resistor, res_int, res_ext, ratio, ratio1, *rcal);
#endif
	} else {
		/* not supported if PMC is not running for now. To support that, need to copy the PMC rom 
		   code to read out resistor value manually */
		rc = kPMC_INVALID_COMMAND;
	}

	return rc;
#else
    return kPMC_INVALID_COMMAND;
#endif
}

/* note: all the [Read|Write][BPCM|Zone]Register functions are different from
 * how they are defined in firmware code.  In the driver code, it takes in
 * wordOffset as the argument, but in the firmware code, it uses byteOffset */
int ReadBPCMRegister(int devAddr, int wordOffset, uint32_t * value)
{
	int status = kPMC_INVALID_STATE;

	if (PMC_ACCESS_BPCM_DIRECT || pmc_mode == PMC_MODE_PMB_DIRECT) {
		status = read_bpcm_reg_direct(devAddr, wordOffset, value);
	} else if (pmc_mode == PMC_MODE_DQM) {
		TCommand rsp;
		status =
		    SendCommand(cmdReadBpcmReg, devAddr, 0, 0, wordOffset, 0,
				&rsp);

		if (status == kPMC_NO_ERROR)
			*value = rsp.u.cmdResponse.word2;
	}

	return status;
}

int ReadZoneRegister(int devAddr, int zone, int wordOffset, uint32_t * value)
{
	int status = kPMC_INVALID_STATE;

	if ((unsigned)wordOffset >= 4)
		return kPMC_INVALID_PARAM;

	if (PMC_ACCESS_BPCM_DIRECT || pmc_mode == PMC_MODE_PMB_DIRECT)
		return ReadBPCMRegister(devAddr,
					BPCMRegOffset(zones[zone].control) +
					wordOffset, value);

	if (pmc_mode == PMC_MODE_DQM) {
		TCommand rsp;

		status =
		    SendCommand(cmdReadZoneReg, devAddr, zone, 0, wordOffset, 0,
				&rsp);
		if (status == kPMC_NO_ERROR)
			*value = rsp.u.cmdResponse.word2;
	}

	return status;
}

int WriteBPCMRegister(int devAddr, int wordOffset, uint32_t value)
{
	int status = kPMC_INVALID_STATE;

	if (PMC_ACCESS_BPCM_DIRECT || pmc_mode == PMC_MODE_PMB_DIRECT) {
		status = write_bpcm_reg_direct(devAddr, wordOffset, value);
	} else if (pmc_mode == PMC_MODE_DQM) {
		status =
		    SendCommand(cmdWriteBpcmReg, devAddr, 0, 0, wordOffset,
				value, 0);
	}

	return status;
}

int WriteZoneRegister(int devAddr, int zone, int wordOffset, uint32_t value)
{
	if ((unsigned)wordOffset >= 4)
		return kPMC_INVALID_PARAM;

	if (PMC_ACCESS_BPCM_DIRECT || pmc_mode == PMC_MODE_PMB_DIRECT)
		return WriteBPCMRegister(devAddr,
					 BPCMRegOffset(zones[zone].control) +
					 wordOffset, value);

	if (pmc_mode == PMC_MODE_DQM)
		return SendCommand(cmdWriteZoneReg, devAddr, zone, 0,
				   wordOffset, value, 0);

	return kPMC_INVALID_STATE;
}

int PowerOnDevice(int devAddr)
{
	if (PMC_ACCESS_BPCM_DIRECT || pmc_mode == PMC_MODE_PMB_DIRECT) {
		int ix, status;
		BPCM_CAPABILITES_REG capabilities;

		status =
		    ReadBPCMRegister(devAddr, BPCMRegOffset(capabilities),
				     &capabilities.Reg32);
		for (ix = 0;
		     (ix < capabilities.Bits.num_zones)
		     && (status == kPMC_NO_ERROR); ix++) {
			status = PowerOnZone(devAddr, ix);
		}

		return status;
	}

	if (pmc_mode == PMC_MODE_DQM) {
		TCommand cmd = {0};
		cmd.u.cmdPowerDevice.state = 1;
		return SendCmd(&cmd, cmdPowerDevOnOff, devAddr, 0, 0, 0);
	}

	return kPMC_INVALID_STATE;
}

int PowerOffDevice(int devAddr, int repower)
{
	if (PMC_ACCESS_BPCM_DIRECT || pmc_mode == PMC_MODE_PMB_DIRECT) {
		int ix, status;
		BPCM_CAPABILITES_REG capabilities;

		status =
		    ReadBPCMRegister(devAddr, BPCMRegOffset(capabilities),
				     &capabilities.Reg32);
		for (ix = capabilities.Bits.num_zones - 1;
		      status == kPMC_NO_ERROR && ix >= 0; ix--) {
			status = PowerOffZone(devAddr, ix);
		}

		return status;
	}

	if (pmc_mode == PMC_MODE_DQM) {
		TCommand cmd = {0};
		cmd.u.cmdPowerDevice.state = 0;
		cmd.u.cmdPowerDevice.restore = repower;
		return SendCmd(&cmd, cmdPowerDevOnOff, devAddr, 0, 0, 0);
	}

	return kPMC_INVALID_STATE;
}

int PowerOnZone(int devAddr, int zone)
{
	BPCM_PWR_ZONE_N_CONTROL ctrl;
	BPCM_PWR_ZONE_N_STATUS sts;
	int status;

#if IS_BCMCHIP(6858)
	/* Do not use DQM command cmdPowerZoneOnOff for non 6858 because this command is only available if a
	   PMC application has been uploaded to expand the PMC boot rom functionality */
	if (pmc_mode == PMC_MODE_DQM) {
		TCommand cmd = {0};
		cmd.u.cmdStateOnlyParam.state = 1;
		return SendCmd(&cmd, cmdPowerZoneOnOff, devAddr, zone, 0, 0);
	}
#endif

	status =
	    ReadBPCMRegister(devAddr, BPCMZoneCtrlRegOffset(zone), &ctrl.Reg32);
	status +=
		ReadBPCMRegister(devAddr, BPCMZoneStsRegOffset(zone), &sts.Reg32);

	if (status == kPMC_NO_ERROR && sts.Bits.pwr_on_state == 0) {
		ctrl.Bits.pwr_dn_req = 0;
		ctrl.Bits.dpg_ctl_en = 1;
		ctrl.Bits.pwr_up_req = 1;
		ctrl.Bits.mem_pwr_ctl_en = 1;
		ctrl.Bits.blk_reset_assert = 1;
		status =
			WriteBPCMRegister(devAddr,  BPCMZoneCtrlRegOffset(zone), ctrl.Reg32);
	}
	return status;
}

int PowerOffZone(int devAddr, int zone)
{
	BPCM_PWR_ZONE_N_CONTROL ctrl;
	int status;

#if IS_BCMCHIP(6858)
	/* Do not use DQM command cmdPowerZoneOnOff for non 6858 because this command is only available if a
	   PMC application has been uploaded to expand the PMC boot rom functionality */
	if (pmc_mode == PMC_MODE_DQM) {
		TCommand cmd = {0};
		cmd.u.cmdStateOnlyParam.state = 0;
		return SendCmd(&cmd, cmdPowerZoneOnOff, devAddr, zone, 0, 0);
	}
#endif

	status =
	    ReadBPCMRegister(devAddr, BPCMZoneCtrlRegOffset(zone), &ctrl.Reg32);
	if (status == kPMC_NO_ERROR) {
		ctrl.Bits.pwr_dn_req = 1;
		ctrl.Bits.pwr_up_req = 0;
		status =
		    WriteBPCMRegister(devAddr, BPCMZoneCtrlRegOffset(zone), ctrl.Reg32);
	}
	return status;
}

int ResetDevice(int devAddr)
{
	/* all zones had their blk_reset_assert bits set at initial config time */
	BPCM_PWR_ZONE_N_STATUS sts;
	int status;

#if IS_BCMCHIP(6858)
	/* Do not use DQM command cmdResetDevice for non 6858 because this command is only available if a
	   PMC application has been uploaded to expand the PMC boot rom functionality */
	if (pmc_mode == PMC_MODE_DQM)
		return SendCommand(cmdResetDevice, devAddr, 0, 0, 0, 0, 0);
#endif

	status = PowerOffDevice(devAddr, 0);
	do {
		status =
		    ReadBPCMRegister(devAddr, BPCMZoneStsRegOffset(0), &sts.Reg32);
	} while ((sts.Bits.pwr_off_state != 1) && (status == kPMC_NO_ERROR));
	if (status == kPMC_NO_ERROR)
		status = PowerOnDevice(devAddr);
	return status;
}

int ResetZone(int devAddr, int zone)
{
	BPCM_PWR_ZONE_N_STATUS sts;
	int status;

#if IS_BCMCHIP(6858)
	/* Do not use DQM command cmdResetZone for non 6858 because this command is only available if a
	   PMC application has been uploaded to expand the PMC boot rom functionality */
	if (pmc_mode == PMC_MODE_DQM)
		return SendCommand(cmdResetZone, devAddr, zone, 0, 0, 0, 0);
#endif

	status = PowerOffZone(devAddr, zone);
	do {
		status =
		    ReadBPCMRegister(devAddr, BPCMZoneStsRegOffset(zone), &sts.Reg32);
	} while ((sts.Bits.pwr_off_state != 1) && (status == kPMC_NO_ERROR));
	if (status == kPMC_NO_ERROR)
		status = PowerOnZone(devAddr, zone);
	return status;
}

/* close AVS with margin slow, fast, max, min (mV) */
int CloseAVS(int island, unsigned short margin_mv_slow,
	     unsigned short margin_mv_fast, unsigned short maximum_mv,
	     unsigned short minimum_mv)
{
	if (pmc_mode == PMC_MODE_DQM) 
    {
		TCommand rsp;
		int status;

#if defined(PMC_IMPL_3_X) || IS_BCMCHIP(63158)
		TCommandCloseAVS ca;

		if (minimum_mv && maximum_mv && (minimum_mv > maximum_mv))
			return kPMC_INVALID_PARAM;

		ca.margin_mv_slow = margin_mv_slow;
		ca.margin_mv_fast = margin_mv_fast;
		ca.maximum_mv = maximum_mv;
		ca.minimum_mv = minimum_mv;

		status = SendCommand(cmdCloseAVS, 0, 0, island,
				     ca.word2, ca.word3, &rsp);
#else
		status = SendCommand(cmdCloseAVS, 0, 0, island,
				     margin_mv_slow, margin_mv_fast, &rsp);
#endif

		return status;
	}
    else
    {
#if IS_BCMCHIP(6855)
    typedef union 
    {
        struct
        {
            uint32_t slow_marg : 16; //[00:15]
            uint32_t fast_marg : 16; //[16:31]
        } Bits;
        uint32_t Reg32;
    } MARGINS_TEST;
#define MARG ((volatile MARGINS_TEST *)0xff802628)
#define MARG_CMD ((volatile uint32_t *)0xff80262c)
        MARG->Bits.slow_marg = margin_mv_slow;
        MARG->Bits.fast_marg = margin_mv_fast;
        *MARG_CMD = 0x7e57fa57;
        return kPMC_NO_ERROR;
#else
		return kPMC_INVALID_COMMAND;
#endif        
    }
}

#if !defined(PMC_ON_HOSTCPU) && !defined(PMC_DIRECT_MODE_ONLY) 
int PMCcmd(int arg[4])
{
	TCommand *cmd = (TCommand *) arg;

	return SendAndWait(cmd, cmd);
}

#ifdef PMC_IMPL_3_X
static void pmc_dump_misc_block_regs(void) 
{
	static const struct {
        uint32_t base_idx;
		uint32_t offset;
		uint32_t count;
		const char *name;
	} regs[] = {
		{ 0, offsetof(MaestroMisc, coreCtrl) + offsetof(CoreCtrl,coreEnable),
			1, "corectrl_core_enable" },
		{ 0, offsetof(MaestroMisc, coreCtrl) + offsetof(CoreCtrl,coreResetCause),
			1, "corectrl_core_reset_cause" },
		{ 0, offsetof(MaestroMisc, coreCtrl) + offsetof(CoreCtrl,sysFlg0Status),
			1, "corectrl_sys_flg0_status" },
		{ 0, offsetof(MaestroMisc, coreCtrl) + offsetof(CoreCtrl,usrFlg0Status),
			1, "corectrl_usr_flg0_status" },
		{ 0, offsetof(MaestroMisc, coreCtrl) + offsetof(CoreCtrl,resetVector),
			1, "corectrl_reset_vector" },
		{ 0, offsetof(MaestroMisc, coreState) + offsetof(CoreState, sysMbx),
			8, "corestate_sys_mbx0..7" },
		{ 0, offsetof(MaestroMisc, coreState) + offsetof(CoreState, usrMbx),
			8, "corestate_usr_mbx0..7" },
		{ 0, offsetof(MaestroMisc, profile) + offsetof(CoreProfile, lastConfPcLo),
			1, "profile_last_conf_pc_lo" },
		{ 0, offsetof(MaestroMisc, profile) + offsetof(CoreProfile, lastPcLo),
			1, "profile_last_pc_lo" },
        { 1, offsetof(Pmc, ctrl) + offsetof(PmcCtrlReg, scratch), 1, "control_scratch" },
	};
	volatile uint8_t *bases[] ={(volatile uint8_t *)g_pmc->maestro_base,
                                (volatile uint8_t *)g_pmc->pmc_base};

	int i, j;

	for (i = 0; i < sizeof(regs) / sizeof(regs[0]); i++) {
		printk("\tmd.l 0x%p %x ## %s",
				(bases[regs[i].base_idx] + regs[i].offset), regs[i].count, regs[i].name);
		for (j = 0; j < regs[i].count; j++) {
			if (!(j%4)) printk("\n%p:", (bases[regs[i].base_idx] + regs[i].offset+j*4));
			printk(" %08x", *((uint32_t *)(bases[regs[i].base_idx] + regs[i].offset + j*4)));
		}
		printk("\n");
	}
}
#endif

void WaitPmc(int runState, void *pmc_log)
{
	int cur, failed;
	volatile Pmc *pmc = (volatile Pmc *)g_pmc->pmc_base;
#ifdef PMC_IMPL_3_X
	volatile MaestroMisc *maestro = (volatile MaestroMisc *)g_pmc->maestro_base;
	char *log_buffer_start = (char *)(pmc_log)+4;
	char *log_buffer_itter = log_buffer_start;
#endif

	if (pmc_mode != PMC_MODE_DQM)
		return;

	failed = 0;
#if PMC_BOOT_TMO_SECONDS
	pmc->ctrl.gpTmr0Ctl = (1 << 31) | (1 << 29) |
			(((PMC_BOOT_TMO_SECONDS * 1000000) << 1) & 0x1fffffff);
#endif

	do {
#ifdef PMC_LOG_IN_DTCM
		pmc_save_log_item();
#endif
		cur = pmc->ctrl.hostMboxIn;
#ifdef PMC_IMPL_3_X
#define PMC_CHIP_NOT_VALID (0x7<<5)
		if ((cur & PMC_CHIP_NOT_VALID) == PMC_CHIP_NOT_VALID) {
			printk("**ERR**: PMC firmware is not compatible to this chip\n");
			failed = 1;
			maestro->coreCtrl.coreEnable = 0;
			break;
		}
#define PMC_AVS_DISABLED (0x1<<7)
		if ((cur & PMC_AVS_DISABLED) == PMC_AVS_DISABLED) {
            printk("WARNING**: AVS Disabled\n");
            break;
        }

		if (!maestro->coreCtrl.coreEnable) {
			printk("**ERR**: PMC firmware crashed\n");
			failed = 1;
			pmc_dump_misc_block_regs();
			break;
		}
#if PMC_BOOT_TMO_SECONDS
		if (!(pmc->ctrl.gpTmr0Ctl & (1 << 31))) {
			printk("**ERR**: %u seconds passed\n", PMC_BOOT_TMO_SECONDS);
			failed = 1;
			maestro->coreCtrl.coreEnable = 0;
			pmc_dump_misc_block_regs();
			break;
		}
#endif

#else /* #ifdef PMC_IMPL_3_X */
		cur >>= 24;
		/* Check if PMC is failing */
		if ((cur & 7) == kPMCRunStateStalled) {
			failed = 1;
			/* Leave PMC in reset state */
			pmc->ctrl.softResets = 0x1;
			break;
		}
#endif /* #ifdef PMC_IMPL_3_X */
	} while ((cur & 7) != runState);

#ifdef PMC_IMPL_3_X
    if (pmc_log)
    {
        if (*(unsigned int *)(log_buffer_start - 4) == 0xc0ffee55) 
        {
            for (; log_buffer_itter < log_buffer_start + CFG_BOOT_PMC_LOG_SIZE; log_buffer_itter++)
            {
                if (*log_buffer_itter == 0xff)
                    continue;
                if (*log_buffer_itter == 0x0)
                    break;
                printk("%c", *log_buffer_itter);
            }

        }
    }
#endif /* #ifdef PMC_IMPL_3_X */

	if (!failed) return;

	printk("**ERR**: PMC firmware failed to boot\n"
		"\tPress any key to continue ...\n");
	while (!console_status()) ;
	/* PMC is now in direct mode */
	pmc_mode = PMC_MODE_PMB_DIRECT;
}
#endif

/* new pmc firmware implements stall command */
/* state indicated by stalled bit in run status */

#define PMC_STALLED (1 << 30)

/* return value doesn't appear to be used */
int StallPmc(void)
{
#if defined(PMC_STALL_SUPPORT) 
	TCommand rsp;
	volatile Pmc *pmc = (volatile Pmc *)g_pmc->pmc_base;

	/* ignore if pmc not booted from flash or already stalled */
	if (pmc_mode == PMC_MODE_PMB_DIRECT ||
#if defined MISC_STRAP_BUS_PMC_BOOT_FLASH
	    (g_pmc->strap & MISC_STRAP_BUS_PMC_BOOT_FLASH) == 0 ||
#else
	    (g_pmc->strap & MISC_STRAP_BUS_PMC_BOOT_FLASH_N) != 0 ||
#endif
	    pmc->ctrl.hostMboxIn & PMC_STALLED)
		return 0;

	/* return non-zero if stall command fails */
	return SendCommand(cmdStall, 0, 0, 0, 0, 0, &rsp);
#else
    return kPMC_INVALID_COMMAND;
#endif
}

/* return value doesn't appear to be used */
int UnstallPmc(void)
{
#if defined(PMC_STALL_SUPPORT) 
	volatile Pmc *pmc = (volatile Pmc *)g_pmc->pmc_base;

	/* clear stalled bit if pmc booted from flash */
	if ((pmc_mode != PMC_MODE_PMB_DIRECT) &&
#if defined MISC_STRAP_BUS_PMC_BOOT_FLASH
	    ((g_pmc->strap & MISC_STRAP_BUS_PMC_BOOT_FLASH) != 0))
#else
	    ((g_pmc->strap & MISC_STRAP_BUS_PMC_BOOT_FLASH_N) == 0))
#endif
	    pmc->ctrl.hostMboxIn &= ~PMC_STALLED;

#endif
	return 0;
}

// initalize pmc_mode (possibly) before printk available
void pmc_initmode(void)
{
	pmc_mode = PMC_MODE_PMB_DIRECT;

#if defined(PMC_IMPL_1_X)
	#if defined MISC_STRAP_BUS_PMC_ROM_BOOT
	{
		if ((g_pmc->strap & MISC_STRAP_BUS_PMC_ROM_BOOT) !=0) 
			pmc_mode = PMC_MODE_DQM;
	}
	#else 
	{
		volatile Pmc *pmc = (volatile Pmc *)g_pmc->pmc_base;
		/* MIPS based PMC */
		if (pmc->ctrl.softResets == 0)
			pmc_mode = PMC_MODE_DQM;
	}
	#endif // #if defined MISC_STRAP_BUS_PMC_ROM_BOOT
#elif defined(PMC_IMPL_3_X)
	{
		volatile MaestroMisc *maestro = (volatile MaestroMisc *)g_pmc->maestro_base;

		/* Maestro based PMC */
		if (maestro->coreCtrl.coreEnable == 1)
			pmc_mode = PMC_MODE_DQM;
	}
#endif // #if defined PMC_IMPL_1_X
}

void pmc_reset(void)
{
#if IS_BCMCHIP(63158)
    volatile Pmc *pmc = (volatile Pmc *)g_pmc->pmc_base;
	// First, make sure PMC core is held in reset
	pmc->ctrl.softResets = 0x1;
	// Set PVTMON in non-AVS mode
	pmc->pvtmon[0].cfg_lo = pmc->pvtmon[0].cfg_lo & ~(0x7 << 10);
	pmc->pvtmon[1].cfg_lo = pmc->pvtmon[1].cfg_lo & ~(0x7 << 10);
	// PMC now in direct mode
	pmc_mode = PMC_MODE_PMB_DIRECT;
#endif
}

int pmc_convert_pvtmon(int sel, int value)
{
#if IS_BCMCHIP(63146) || IS_BCMCHIP(4912) || IS_BCMCHIP(6813) || \
	IS_BCMCHIP(6888) || IS_BCMCHIP(6765) || IS_BCMCHIP(6766)
	switch (sel) {
	case kTEMPERATURE:	// convert value to milli-degree Celsius
		return (45000000 - 54956 * value) / 100;
	case kV_0p85_0:		// convert value to milli-voltage
	case kV_0p85_1:
	case kV_VIN:
	case kV_1p00_1:
		return 9442 * value / (8 * 1024);
	case kV_1p80:
		return 9442 * value / (4 * 1024);
	case kV_3p30:
		return 9442 * value / (2 * 1024);
	case kTEST:
		return 9442 * value / 1024;
	}
#else // #if IS_BCMCHIP(63146) || IS_BCMCHIP(4912) || IS_BCMCHIP(6813) ...
	switch (sel) {
	case kTEMPERATURE:	// convert value to milli-degree Celsius
#if IS_BCMCHIP(63148)		// pvt2
		return (38887551 - 466415 * value / 10) / 100;
#elif defined(PMC_IMPL_3_X) || IS_BCMCHIP(6878) || IS_BCMCHIP(6855)
		return (41335000 - 49055 * value) / 100;
#else
		return (41004000 - 48705 * value) / 100;
#endif
	case kV_0p85_0:	// convert value to milli-voltage
	case kV_0p85_1:
		return 880 * value * 10 / (10 * 1024);
	case kV_VIN:
	case kV_1p00_1:
		return 880 * value * 10 / (7 * 1024);
	case kV_1p80:
		return 880 * value * 10 / (4 * 1024);
	case kV_3p30:
		return 880 * value * 10 / (2 * 1024);
	case kTEST:
		return 880 * value / 1024;
	}
#endif // #if IS_BCMCHIP(63146) || IS_BCMCHIP(4912) || IS_BCMCHIP(6813) ...

	return -1;
}

int pmc_get_tracktemp(int *status)
{
	TCommand rsp;
	int ret;

	if (pmc_mode != PMC_MODE_DQM)
		return kPMC_INVALID_COMMAND;

	ret = SendCommand(cmdGetTrackTemp, 0, 0, 0, 0, 0, &rsp);
	if (ret == kPMC_NO_ERROR)
		*status = ! !rsp.u.cmdResponse.word2;

	return ret;
}

int pmc_set_tracktemp(int enable)
{
	if (pmc_mode != PMC_MODE_DQM)
		return kPMC_INVALID_COMMAND;

	return SendCommand(cmdSetTrackTemp, 0, 0, 0, ! !enable, 0, NULL);
}
