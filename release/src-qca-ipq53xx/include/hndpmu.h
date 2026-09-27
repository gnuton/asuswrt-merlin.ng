/*
 * HND SiliconBackplane PMU support.
 *
 * Copyright (C) 2009, Broadcom Corporation
 * All Rights Reserved.
 * 
 * THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
 * KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
 * SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
 *
 * $Id: hndpmu.h,v 13.19.2.4 2009/06/18 12:19:28 Exp $
 */

#ifndef _hndpmu_h_
#define _hndpmu_h_

#define SET_LDO_VOLTAGE_LDO1	1
#define SET_LDO_VOLTAGE_LDO2	2
#define SET_LDO_VOLTAGE_LDO3	3
#define SET_LDO_VOLTAGE_PAREF	4
#define SET_LDO_VOLTAGE_CLDO_PWM	5
#define SET_LDO_VOLTAGE_CLDO_BURST	6
#define SET_LDO_VOLTAGE_CBUCK_PWM	7
#define SET_LDO_VOLTAGE_CBUCK_BURST	8

extern void si_pmu_init(si_t *sih, osl_t *osh);
extern void si_pmu_chip_init(si_t *sih, osl_t *osh);
extern void si_pmu_pll_init(si_t *sih, osl_t *osh, uint32 xtalfreq);
extern void si_pmu_res_init(si_t *sih, osl_t *osh);
extern void si_pmu_swreg_init(si_t *sih, osl_t *osh);

extern uint32 si_pmu_force_ilp(si_t *sih, osl_t *osh, bool force);

extern uint32 si_pmu_si_clock(si_t *sih, osl_t *osh);
extern uint32 si_pmu_cpu_clock(si_t *sih, osl_t *osh);
extern uint32 si_pmu_mem_clock(si_t *sih, osl_t *osh);
extern uint32 si_pmu_alp_clock(si_t *sih, osl_t *osh);
extern uint32 si_pmu_ilp_clock(si_t *sih, osl_t *osh);

extern void si_pmu_set_switcher_voltage(si_t *sih, osl_t *osh, uint8 bb_voltage, uint8 rf_voltage);
extern void si_pmu_set_ldo_voltage(si_t *sih, osl_t *osh, uint8 ldo, uint8 voltage);
extern void si_pmu_paref_ldo_enable(si_t *sih, osl_t *osh, bool enable);
extern uint16 si_pmu_fast_pwrup_delay(si_t *sih, osl_t *osh);
extern void si_pmu_rcal(si_t *sih, osl_t *osh);
extern void si_pmu_pllcontrol(si_t *sih, uint reg, uint32 mask, uint32 val);
extern void si_pmu_pllupd(si_t *sih);
extern void si_pmu_spuravoid(si_t *sih, osl_t *osh, bool spuravoid);
extern void si_pmu_gband_spurwar(si_t *sih, osl_t *osh);

extern bool si_pmu_is_otp_powered(si_t *sih, osl_t *osh);

extern uint32 si_pmu_waitforclk_on_backplane(si_t *sih, osl_t *osh, uint32 clk, uint32 delay);

extern void si_pmu_otp_power(si_t *sih, osl_t *osh, bool on);
extern void si_sdiod_drive_strength_init(si_t *sih, osl_t *osh, uint32 drivestrength);
extern void si_pmu_chipcontrol(si_t *sih, uint reg, uint32 mask, uint32 val);


#endif /* _hndpmu_h_ */
