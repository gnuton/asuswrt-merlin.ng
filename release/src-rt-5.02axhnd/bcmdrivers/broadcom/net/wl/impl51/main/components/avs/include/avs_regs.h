/*
 * Adaptive Voltage Scaling
 *
 * Copyright 2020 Broadcom
 *
 * This program is the proprietary software of Broadcom and/or
 * its licensors, and may only be used, duplicated, modified or distributed
 * pursuant to the terms and conditions of a separate, written license
 * agreement executed between you and Broadcom (an "Authorized License").
 * Except as set forth in an Authorized License, Broadcom grants no license
 * (express or implied), right to use, or waiver of any kind with respect to
 * the Software, and Broadcom expressly reserves all rights in and to the
 * Software and all intellectual property rights therein.  IF YOU HAVE NO
 * AUTHORIZED LICENSE, THEN YOU HAVE NO RIGHT TO USE THIS SOFTWARE IN ANY
 * WAY, AND SHOULD IMMEDIATELY NOTIFY BROADCOM AND DISCONTINUE ALL USE OF
 * THE SOFTWARE.
 *
 * Except as expressly set forth in the Authorized License,
 *
 * 1. This program, including its structure, sequence and organization,
 * constitutes the valuable trade secrets of Broadcom, and you shall use
 * all reasonable efforts to protect the confidentiality thereof, and to
 * use this information only in connection with your use of Broadcom
 * integrated circuit products.
 *
 * 2. TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED
 * "AS IS" AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES,
 * REPRESENTATIONS OR WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR
 * OTHERWISE, WITH RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY
 * DISCLAIMS ANY AND ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY,
 * NONINFRINGEMENT, FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES,
 * ACCURACY OR COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR
 * CORRESPONDENCE TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING
 * OUT OF USE OR PERFORMANCE OF THE SOFTWARE.
 *
 * 3. TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL
 * BROADCOM OR ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL,
 * SPECIAL, INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR
 * IN ANY WAY RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN
 * IF BROADCOM HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES; OR (ii)
 * ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE SOFTWARE ITSELF
 * OR U.S. $1, WHICHEVER IS GREATER. THESE LIMITATIONS SHALL APPLY
 * NOTWITHSTANDING ANY FAILURE OF ESSENTIAL PURPOSE OF ANY LIMITED REMEDY.
 *
 * <<Broadcom-WL-IPTag/Proprietary:>>
 *
 * $Id$
 */

/* This is the set of registers definitions used by this code */

#ifndef _REGS_H_
#define _REGS_H_

/* from: bchp_common.h */
#define BCHP_AVS_UART_REG_START                            0x28020000
#define BCHP_AVS_PMB_S_000_REG_START                       0x28024000
#define BCHP_AVS_PMB_S_001_REG_START                       0x28024040

#define BCHP_AVS_TMON_TEMPERATURE_MEASUREMENT_STATUS		0x28021500
#define BCHP_AVS_TMON_TEMPERATURE_MEASUREMENT_VALID		0x00000800
#define BCHP_AVS_TMON_TEMPERATURE_MEASUREMENT_DATA_MASK		0x000007ff

//#define BCHP_AVS_TMON_ENABLE_OVER_TEMPERATURE_RESET    0x28021504 /* Enable over temperature reset */
//#define BCHP_AVS_TMON_TEMPERATURE_RESET_THRESHOLD      0x28021508 /* Represent threshold for over temperature reset */

#define BCHP_AVS_HW_MNTR_SW_CONTROLS                   0x28022000 /* Software control command registers for AVS */
#define BCHP_AVS_HW_MNTR_SW_MEASUREMENT_UNIT_BUSY      0x28022004 /* Indicate measurement unit is busy and SW should not de-assert sw_takeover while this is asserted */
#define BCHP_AVS_HW_MNTR_MEASUREMENTS_INIT_PVT_MNTR    0x28022008 /* Software to reset the pvt monitors measurements' valid bits in RO registers */
#define BCHP_AVS_HW_MNTR_MEASUREMENTS_INIT_CEN_ROSC_0  0x2802200c /* Software to reset the central roscs measurements' valid bits in RO registers */
#define BCHP_AVS_HW_MNTR_MEASUREMENTS_INIT_CEN_ROSC_1  0x28022010 /* Software to reset the central roscs measurements' valid bits in RO registers */
#define BCHP_AVS_HW_MNTR_SEQUENCER_INIT                0x28022038 /* Initialize the sensor sequencer */
#define BCHP_AVS_HW_MNTR_SEQUENCER_MASK_PVT_MNTR       0x2802203c /* Indicate which PVT Monitor measurements should  be masked(skipped) in the measurement sequence */
#define BCHP_AVS_HW_MNTR_SEQUENCER_MASK_CEN_ROSC_0     0x28022040 /* Indicate which central ring oscillators should  be masked(skipped) in the measurement sequence */
#define BCHP_AVS_HW_MNTR_SEQUENCER_MASK_CEN_ROSC_1     0x28022044 /* Indicate which central ring oscillators should  be masked(skipped) in the measurement sequence */
#define BCHP_AVS_HW_MNTR_ENABLE_DEFAULT_CEN_ROSC_0     0x2802206c /* Enabling/Disabling of central ring oscillators */
#define BCHP_AVS_HW_MNTR_ENABLE_DEFAULT_CEN_ROSC_1     0x28022070 /* Enabling/Disabling of central ring oscillators */
#define BCHP_AVS_HW_MNTR_ROSC_MEASUREMENT_TIME_CONTROL 0x28022074 /* Control the time taken for a rosc/pwd measurement */
#define BCHP_AVS_HW_MNTR_ROSC_COUNTING_MODE            0x28022078 /* Control the counting event for rosc signal counter */
#define BCHP_AVS_HW_MNTR_SW_CONTROLS_sw_sensor_idx_SHIFT           8
#define BCHP_AVS_HW_MNTR_SW_CONTROLS_sw_do_measure_MASK            0x00000002
#define BCHP_AVS_HW_MNTR_SW_CONTROLS_sw_takeover_MASK              0x00000001
#define BCHP_AVS_HW_MNTR_SW_MEASUREMENT_UNIT_BUSY_busy_MASK        0x00000001
#define BCHP_AVS_HW_MNTR_MEASUREMENTS_INIT_PVT_MNTR_m_init_pvt_mntr_MASK 0x000000ff
#define BCHP_AVS_HW_MNTR_MEASUREMENTS_INIT_CEN_ROSC_0_m_init_cen_rosc_MASK 0xffffffff
#define BCHP_AVS_HW_MNTR_MEASUREMENTS_INIT_CEN_ROSC_1_m_init_cen_rosc_MASK 0x0000000f
#define BCHP_AVS_HW_MNTR_ENABLE_DEFAULT_CEN_ROSC_0_cen_rosc_enable_default_MASK 0xffffffff
#define BCHP_AVS_HW_MNTR_ENABLE_DEFAULT_CEN_ROSC_1_cen_rosc_enable_default_MASK 0x0000000f

#define BCHP_AVS_PMB_S_000_AVS_ROSC_CONTROL			0x28024010 /* Ring Ocsillator Control Register */
#define BCHP_AVS_PMB_S_000_AVS_ROSC_CONTROL_RO_EN_S		0x00000001 /* Ring Oscillator Enable */
#define BCHP_AVS_PMB_S_000_AVS_ROSC_CONTROL_RO_EN_H		0x00000002 /* Ring Oscillator Enable */
#define BCHP_AVS_PMB_S_000_AVS_ROSC_CONTROL_ECTR_EN_S		0x00000004 /* Event Counter Enable */
#define BCHP_AVS_PMB_S_000_AVS_ROSC_CONTROL_ECTR_EN_H		0x00000008 /* Event Counter Enable */
#define BCHP_AVS_PMB_S_000_AVS_ROSC_CONTROL_THRSH_EN_S		0x00000010 /* Threshold Enable */
#define BCHP_AVS_PMB_S_000_AVS_ROSC_CONTROL_THRSH_EN_H		0x00000020 /* Threshold Enable */
#define BCHP_AVS_PMB_S_000_AVS_ROSC_CONTROL_CONTINUOUS_S	0x00000040 /* Continuous Count Mode Enable */
#define BCHP_AVS_PMB_S_000_AVS_ROSC_CONTROL_CONTINUOUS_H	0x00000080 /* Continuous Count Mode Enable */
#define BCHP_AVS_PMB_S_000_AVS_ROSC_CONTROL_TEST_INTERVAL_MASK	0xffff0080
#define BCHP_AVS_PMB_S_000_AVS_ROSC_CONTROL_TEST_INTERVAL_SHIFT	16
#define BCHP_AVS_PMB_S_000_AVS_ROSC_CONTROL_VALID_S		0x00001000
#define BCHP_AVS_PMB_S_000_AVS_ROSC_CONTROL_VALID_H		0x00004000

#define BCHP_AVS_PMB_S_000_AVS_ROSC_H_THRESHOLD  0x28024014 /* Event Counter Threshold Register for ROSC_H */
#define BCHP_AVS_PMB_S_000_AVS_ROSC_S_THRESHOLD  0x28024018 /* Event Counter Threshold Register for ROSC_S */
#define BCHP_AVS_PMB_S_000_AVS_ROSC_COUNT        0x2802401c /* Event Counter Count Register */
#define BCHP_AVS_PMB_S_000_AVS_ROSC_H_THRESHOLD_THRESH_HI_MASK     0xffff0000
#define BCHP_AVS_PMB_S_000_AVS_ROSC_H_THRESHOLD_THRESH_HI_SHIFT    16
#define BCHP_AVS_PMB_S_000_AVS_ROSC_H_THRESHOLD_THRESH_LO_MASK     0x0000ffff
#define BCHP_AVS_PMB_S_000_AVS_ROSC_H_THRESHOLD_THRESH_LO_SHIFT    0
#define BCHP_AVS_PMB_S_000_AVS_ROSC_S_THRESHOLD_THRESH_HI_MASK     0xffff0000
#define BCHP_AVS_PMB_S_000_AVS_ROSC_S_THRESHOLD_THRESH_HI_SHIFT    16
#define BCHP_AVS_PMB_S_000_AVS_ROSC_S_THRESHOLD_THRESH_LO_MASK     0x0000ffff
#define BCHP_AVS_PMB_S_000_AVS_ROSC_S_THRESHOLD_THRESH_LO_SHIFT    0
#define BCHP_AVS_PMB_S_000_AVS_ROSC_COUNT_COUNT_H_MASK             0xffff0000
#define BCHP_AVS_PMB_S_000_AVS_ROSC_COUNT_COUNT_H_SHIFT            16
#define BCHP_AVS_PMB_S_000_AVS_ROSC_COUNT_COUNT_S_MASK             0x0000ffff
#define BCHP_AVS_PMB_S_000_AVS_ROSC_COUNT_COUNT_S_SHIFT            0

#define BCHP_AVS_PVT_MNTR_CONFIG_PVT_MNTR_CTRL   0x28022100 /* Control bits for PVT monitor */

#define BCHP_AVS_PVT_MNTR_CONFIG_PVT_MNTR_CTRL_DAC_ENABLE		0x1000
#define BCHP_AVS_PVT_MNTR_CONFIG_PVT_MNTR_CTRL_MODE_VTMON		0x0
#define BCHP_AVS_PVT_MNTR_CONFIG_PVT_MNTR_CTRL_MODE_VTMON_DAC		0x1
#define BCHP_AVS_PVT_MNTR_CONFIG_PVT_MNTR_CTRL_MODE_BURNIN		0x2
#define BCHP_AVS_PVT_MNTR_CONFIG_PVT_MNTR_CTRL_MODE_DAC_DRIVE		0x3
#define BCHP_AVS_PVT_MNTR_CONFIG_PVT_MNTR_CTRL_MODE_MEASURE_PAD_ADC	0x4
#define BCHP_AVS_PVT_MNTR_CONFIG_PVT_MNTR_CTRL_MODE_MEASURE_PAD_DAC	0x5
#define BCHP_AVS_PVT_MNTR_CONFIG_PVT_MNTR_CTRL_MODE_RMON		0x6
#define BCHP_AVS_PVT_MNTR_CONFIG_PVT_MNTR_CTRL_MODE_EXPERT		0x7
#define BCHP_AVS_PVT_MNTR_CONFIG_PVT_MNTR_CTRL_MODE_MASK		0x380
#define BCHP_AVS_PVT_MNTR_CONFIG_PVT_MNTR_CTRL_MODE_SHIFT		7

#define BCHP_AVS_PVT_MNTR_CONFIG_DAC_CODE_PROGRAMMING_ENABLE 0x28022110 /* SW must set this bit to 1 to modify DAC_CODE, MIN_DAC_CODE and MAX_DAC_CODE */
#define BCHP_AVS_PVT_MNTR_CONFIG_DAC_CODE        0x28022114 /* Represents the input code of the DAC */
#define BCHP_AVS_PVT_MNTR_CONFIG_MIN_DAC_CODE    0x28022118 /* SW may set this to a lower value to prevent the DAC from driving too high of a voltage */
#define BCHP_AVS_PVT_MNTR_CONFIG_MAX_DAC_CODE    0x2802211c /* SW may set this to a higher value to prevent the DAC from driving too low of a voltage */
#define BCHP_AVS_PVT_MNTR_CONFIG_DAC_CODE_dac_code_MASK            0x000003ff

#define BCHP_AVS_RO_REGISTERS_0_PVT_TEMPERATURE_MNTR_STATUS 0x28022200 /* Indicate PVT monitor sel 000(Temperature Monitoring) measurements data, validity of data and measurement done status */
#define BCHP_AVS_RO_REGISTERS_0_PVT_0P85V_0_MNTR_STATUS     0x28022204 /* Indicate PVT monitor sel 001(0p85V_0 Monitoring) measurements data, validity of data and measurement done status */
#define BCHP_AVS_RO_REGISTERS_0_PVT_0P85V_1_MNTR_STATUS     0x28022208 /* Indicate PVT monitor sel 010(0p85V_1 Monitoring) measurements data, validity of data and measurement done status */
#define BCHP_AVS_RO_REGISTERS_0_PVT_1V_0_MNTR_STATUS        0x2802220c /* Indicate PVT monitor sel 011(1V_0 Monitoring) measurements data, validity of data and measurement done status */
#define BCHP_AVS_RO_REGISTERS_0_PVT_1V_1_MNTR_STATUS        0x28022210 /* Indicate PVT monitor sel 100(1V_1 Monitoring) measurements data, validity of data and measurement done status */
#define BCHP_AVS_RO_REGISTERS_0_PVT_1p8V_MNTR_STATUS        0x28022214 /* Indicate PVT monitor sel 101(1p8V Monitoring) measurements data, validity of data and measurement done status */
#define BCHP_AVS_RO_REGISTERS_0_PVT_3p3V_MNTR_STATUS        0x28022218 /* Indicate PVT monitor sel 110(3p3V Monitoring) measurements data, validity of data and measurement done status */
#define BCHP_AVS_RO_REGISTERS_0_PVT_TESTMODE_MNTR_STATUS    0x2802221c /* Indicate PVT monitor sel 111(Testmode Monitoring) measurements data, validity of data and measurement done status */
#define BCHP_AVS_RO_REGISTERS_0_CEN_ROSC_STATUS_0           0x28022220 /* Indicate central rosc 0 measurement data and validity of data */
#define BCHP_AVS_RO_REGISTERS_0_PVT_TESTMODE_MNTR_STATUS_done_MASK 0x00010000
#define BCHP_AVS_RO_REGISTERS_0_PVT_TESTMODE_MNTR_STATUS_valid_data_MASK 0x00000400
#define BCHP_AVS_RO_REGISTERS_0_PVT_TESTMODE_MNTR_STATUS_data_MASK 0x000003ff
#define BCHP_AVS_RO_REGISTERS_0_CEN_ROSC_STATUS_0_data_MASK        0x00007fff

#define BCHP_AVS_ROSC_THRESHOLD_1_THRESHOLD1_CEN_ROSC_0    0x28022d00 /* Threshold1 for central rosc number 0 */
#define BCHP_AVS_ROSC_THRESHOLD_1_CEN_ROSC_THRESHOLD1_EN_0 0x28022dd0 /* Software to program a mask to indicate which central roscs are to be tested against the threshold1 */
#define BCHP_AVS_ROSC_THRESHOLD_1_CEN_ROSC_THRESHOLD1_EN_1 0x28022dd4 /* Software to program a mask to indicate which central roscs are to be tested against the threshold1 */
#define BCHP_AVS_ROSC_THRESHOLD_2_THRESHOLD2_CEN_ROSC_0    0x28022e00 /* Threshold2 for central rosc number 0 */
#define BCHP_AVS_ROSC_THRESHOLD_2_CEN_ROSC_THRESHOLD2_EN_0 0x28022ed0 /* Software to program a mask to indicate which central roscs are to be tested against the threshold2 */
#define BCHP_AVS_ROSC_THRESHOLD_2_CEN_ROSC_THRESHOLD2_EN_1 0x28022ed4 /* Software to program a mask to indicate which central roscs are to be tested against the threshold2 */

#define BCHP_AVS_TOP_CTRL_MEMORY_ASSIST          0x28021800 /* MEMORY ASSIST */
#define BCHP_AVS_TOP_CTRL_VTRAP_STATUS           0x28021814 /* VTRAP STATUS */
#define BCHP_AVS_TOP_CTRL_VTRAP_STATUS_CLEAR     0x28021818 /* VTRAP STATUS CLEAR */
#define BCHP_AVS_TOP_CTRL_OTP_STATUS             0x28021820 /* OTP STATUS */
#define BCHP_AVS_TOP_CTRL_MEMORY_ASSIST_AVS_TOP_LVM_LL_MASK        0x00000004
#define BCHP_AVS_TOP_CTRL_MEMORY_ASSIST_AVS_TOP_LVM_LL_SHIFT       2
#define BCHP_AVS_TOP_CTRL_OTP_STATUS_OTP_VTRAP_ENABLE_MASK         0x00000400
#define BCHP_AVS_TOP_CTRL_OTP_STATUS_OTP_VTRAP_ENABLE_SHIFT        10
#define BCHP_AVS_TOP_CTRL_OTP_STATUS_OTP_VTRAP_TRIM_CODE_MASK      0x00fff800
#define BCHP_AVS_TOP_CTRL_OTP_STATUS_OTP_VTRAP_TRIM_CODE_SHIFT     11

#define BCHP_AVS_TOP_CTRL_VTRAP_STATUS_THRESHOLD_WARNING0_STATUS_MASK 0x00000010
#define BCHP_AVS_TOP_CTRL_VTRAP_STATUS_THRESHOLD_WARNING1_STATUS_MASK 0x00000004
#define BCHP_AVS_TOP_CTRL_VTRAP_STATUS_CLEAR_THRESHOLD_WARNING0_STATUS_CLEAR_MASK 0x00000010
#define BCHP_AVS_TOP_CTRL_VTRAP_STATUS_CLEAR_THRESHOLD_WARNING1_STATUS_CLEAR_MASK 0x00000004

/* Enable this define to verify code that support these items */
#define BCHP_AVS_TOP_CTRL_ADC_SEL                0x00211860 /* [RW] ADC SEL */			// ALWIN: no match with HW
#define BCHP_AVS_TOP_CTRL_PVT_MNTR1_DAC_CODE     0x0021184c /* [RW] PVT_MNTR1 DAC_CODE */	// ALWIN: no match with HW
#define BCHP_AVS_TOP_CTRL_PVT_MNTR2_DAC_CODE     0x00211850 /* [RW] PVT_MNTR2 DAC_CODE */	// ALWIN: no match with HW
#define BCHP_AVS_TOP_CTRL_PVT_MNTR3_DAC_CODE     0x00211854 /* [RW] PVT_MNTR3 DAC_CODE */	// ALWIN: no match with HW
#define BCHP_AVS_TOP_CTRL_PVT_MNTR4_DAC_CODE     0x00211858 /* [RW] PVT_MNTR4 DAC_CODE */	// ALWIN: no match with HW
#define BCHP_AVS_TOP_CTRL_PVT_MNTR5_DAC_CODE     0x0021185c /* [RW] PVT_MNTR5 DAC_CODE */	// ALWIN: no match with HW

/* All of the PMB blocks are fixed sizes of this amount */
#define BCHP_AVS_PMB_OFFSET (BCHP_AVS_PMB_S_001_REG_START - BCHP_AVS_PMB_S_000_REG_START)

#endif /*_REGS_H_*/
