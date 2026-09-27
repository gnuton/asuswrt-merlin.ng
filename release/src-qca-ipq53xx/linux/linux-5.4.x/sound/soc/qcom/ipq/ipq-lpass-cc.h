/*
 * Copyright (c) 2021, The Linux Foundation. All rights reserved.
 *
 * Copyright (c) 2021 Qualcomm Innovation Center, Inc. All rights reserved.
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

#ifndef __IPQ_CC_H__
#define __IPQ_CC_H__

/*----------------------------------------------------------------------------
 * MODULE: LPASS_CC
 *--------------------------------------------------------------------------*/

#define LPASS_CC_REG_BASE                                                                                             (LPASS_BASE + 0x00000000)
#define LPASS_CC_REG_BASE_SIZE                                                                                        0x48000
#define LPASS_CC_REG_BASE_USED                                                                                        0x47004

#define HWIO_LPASS_LPAAUDIO_PLL_MODE_ADDR(x)                                                                          ((x) + 0x0)
#define HWIO_LPASS_LPAAUDIO_PLL_MODE_RMSK                                                                             0xffffffff
#define HWIO_LPASS_LPAAUDIO_PLL_MODE_IN(x)            \
                in_dword(HWIO_LPASS_LPAAUDIO_PLL_MODE_ADDR(x))
#define HWIO_LPASS_LPAAUDIO_PLL_MODE_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPAAUDIO_PLL_MODE_ADDR(x), m)
#define HWIO_LPASS_LPAAUDIO_PLL_MODE_OUT(x, v)            \
                out_dword(HWIO_LPASS_LPAAUDIO_PLL_MODE_ADDR(x),v)
#define HWIO_LPASS_LPAAUDIO_PLL_MODE_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_LPAAUDIO_PLL_MODE_ADDR(x),m,v,HWIO_LPASS_LPAAUDIO_PLL_MODE_IN(x))
#define HWIO_LPASS_LPAAUDIO_PLL_MODE_PLL_LOCK_DET_BMSK                                                                0x80000000
#define HWIO_LPASS_LPAAUDIO_PLL_MODE_PLL_LOCK_DET_SHFT                                                                        31
#define HWIO_LPASS_LPAAUDIO_PLL_MODE_PLL_ACTIVE_FLAG_BMSK                                                             0x40000000
#define HWIO_LPASS_LPAAUDIO_PLL_MODE_PLL_ACTIVE_FLAG_SHFT                                                                     30
#define HWIO_LPASS_LPAAUDIO_PLL_MODE_PLL_ACK_LATCH_BMSK                                                               0x20000000
#define HWIO_LPASS_LPAAUDIO_PLL_MODE_PLL_ACK_LATCH_SHFT                                                                       29
#define HWIO_LPASS_LPAAUDIO_PLL_MODE_RESERVE_BITS28_24_BMSK                                                           0x1f000000
#define HWIO_LPASS_LPAAUDIO_PLL_MODE_RESERVE_BITS28_24_SHFT                                                                   24
#define HWIO_LPASS_LPAAUDIO_PLL_MODE_PLL_HW_UPDATE_LOGIC_BYPASS_BMSK                                                    0x800000
#define HWIO_LPASS_LPAAUDIO_PLL_MODE_PLL_HW_UPDATE_LOGIC_BYPASS_SHFT                                                          23
#define HWIO_LPASS_LPAAUDIO_PLL_MODE_PLL_UPDATE_BMSK                                                                    0x400000
#define HWIO_LPASS_LPAAUDIO_PLL_MODE_PLL_UPDATE_SHFT                                                                          22
#define HWIO_LPASS_LPAAUDIO_PLL_MODE_PLL_VOTE_FSM_RESET_BMSK                                                            0x200000
#define HWIO_LPASS_LPAAUDIO_PLL_MODE_PLL_VOTE_FSM_RESET_SHFT                                                                  21
#define HWIO_LPASS_LPAAUDIO_PLL_MODE_PLL_VOTE_FSM_ENA_BMSK                                                              0x100000
#define HWIO_LPASS_LPAAUDIO_PLL_MODE_PLL_VOTE_FSM_ENA_SHFT                                                                    20
#define HWIO_LPASS_LPAAUDIO_PLL_MODE_PLL_BIAS_COUNT_BMSK                                                                 0xfc000
#define HWIO_LPASS_LPAAUDIO_PLL_MODE_PLL_BIAS_COUNT_SHFT                                                                      14
#define HWIO_LPASS_LPAAUDIO_PLL_MODE_PLL_LOCK_COUNT_BMSK                                                                  0x3f00
#define HWIO_LPASS_LPAAUDIO_PLL_MODE_PLL_LOCK_COUNT_SHFT                                                                       8
#define HWIO_LPASS_LPAAUDIO_PLL_MODE_RESERVE_BITS7_4_BMSK                                                                   0xf0
#define HWIO_LPASS_LPAAUDIO_PLL_MODE_RESERVE_BITS7_4_SHFT                                                                      4
#define HWIO_LPASS_LPAAUDIO_PLL_MODE_PLL_PLLTEST_BMSK                                                                        0x8
#define HWIO_LPASS_LPAAUDIO_PLL_MODE_PLL_PLLTEST_SHFT                                                                          3
#define HWIO_LPASS_LPAAUDIO_PLL_MODE_PLL_RESET_N_BMSK                                                                        0x4
#define HWIO_LPASS_LPAAUDIO_PLL_MODE_PLL_RESET_N_SHFT                                                                          2
#define HWIO_LPASS_LPAAUDIO_PLL_MODE_PLL_BYPASSNL_BMSK                                                                       0x2
#define HWIO_LPASS_LPAAUDIO_PLL_MODE_PLL_BYPASSNL_SHFT                                                                         1
#define HWIO_LPASS_LPAAUDIO_PLL_MODE_PLL_OUTCTRL_BMSK                                                                        0x1
#define HWIO_LPASS_LPAAUDIO_PLL_MODE_PLL_OUTCTRL_SHFT                                                                          0

#define HWIO_LPASS_LPAAUDIO_PLL_L_VAL_ADDR(x)                                                                         ((x) + 0x4)
#define HWIO_LPASS_LPAAUDIO_PLL_L_VAL_RMSK                                                                            0xffffffff
#define HWIO_LPASS_LPAAUDIO_PLL_L_VAL_IN(x)            \
                in_dword(HWIO_LPASS_LPAAUDIO_PLL_L_VAL_ADDR(x))
#define HWIO_LPASS_LPAAUDIO_PLL_L_VAL_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPAAUDIO_PLL_L_VAL_ADDR(x), m)
#define HWIO_LPASS_LPAAUDIO_PLL_L_VAL_OUT(x, v)            \
                out_dword(HWIO_LPASS_LPAAUDIO_PLL_L_VAL_ADDR(x),v)
#define HWIO_LPASS_LPAAUDIO_PLL_L_VAL_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_LPAAUDIO_PLL_L_VAL_ADDR(x),m,v,HWIO_LPASS_LPAAUDIO_PLL_L_VAL_IN(x))
#define HWIO_LPASS_LPAAUDIO_PLL_L_VAL_RESERVE_BITS_31_8_BMSK                                                          0xffff0000
#define HWIO_LPASS_LPAAUDIO_PLL_L_VAL_RESERVE_BITS_31_8_SHFT                                                                  16
#define HWIO_LPASS_LPAAUDIO_PLL_L_VAL_PLL_L_BMSK                                                                          0xffff
#define HWIO_LPASS_LPAAUDIO_PLL_L_VAL_PLL_L_SHFT                                                                               0

#define HWIO_LPASS_LPAAUDIO_PLL_ALPHA_VAL_ADDR(x)                                                                     ((x) + 0x8)
#define HWIO_LPASS_LPAAUDIO_PLL_ALPHA_VAL_RMSK                                                                        0xffffffff
#define HWIO_LPASS_LPAAUDIO_PLL_ALPHA_VAL_IN(x)            \
                in_dword(HWIO_LPASS_LPAAUDIO_PLL_ALPHA_VAL_ADDR(x))
#define HWIO_LPASS_LPAAUDIO_PLL_ALPHA_VAL_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPAAUDIO_PLL_ALPHA_VAL_ADDR(x), m)
#define HWIO_LPASS_LPAAUDIO_PLL_ALPHA_VAL_OUT(x, v)            \
                out_dword(HWIO_LPASS_LPAAUDIO_PLL_ALPHA_VAL_ADDR(x),v)
#define HWIO_LPASS_LPAAUDIO_PLL_ALPHA_VAL_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_LPAAUDIO_PLL_ALPHA_VAL_ADDR(x),m,v,HWIO_LPASS_LPAAUDIO_PLL_ALPHA_VAL_IN(x))
#define HWIO_LPASS_LPAAUDIO_PLL_ALPHA_VAL_PLL_ALPHA_31_0_BMSK                                                         0xffffffff
#define HWIO_LPASS_LPAAUDIO_PLL_ALPHA_VAL_PLL_ALPHA_31_0_SHFT                                                                  0

#define HWIO_LPASS_LPAAUDIO_PLL_ALPHA_VAL_U_ADDR(x)                                                                   ((x) + 0xc)
#define HWIO_LPASS_LPAAUDIO_PLL_ALPHA_VAL_U_RMSK                                                                      0xffffffff
#define HWIO_LPASS_LPAAUDIO_PLL_ALPHA_VAL_U_IN(x)            \
                in_dword(HWIO_LPASS_LPAAUDIO_PLL_ALPHA_VAL_U_ADDR(x))
#define HWIO_LPASS_LPAAUDIO_PLL_ALPHA_VAL_U_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPAAUDIO_PLL_ALPHA_VAL_U_ADDR(x), m)
#define HWIO_LPASS_LPAAUDIO_PLL_ALPHA_VAL_U_OUT(x, v)            \
                out_dword(HWIO_LPASS_LPAAUDIO_PLL_ALPHA_VAL_U_ADDR(x),v)
#define HWIO_LPASS_LPAAUDIO_PLL_ALPHA_VAL_U_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_LPAAUDIO_PLL_ALPHA_VAL_U_ADDR(x),m,v,HWIO_LPASS_LPAAUDIO_PLL_ALPHA_VAL_U_IN(x))
#define HWIO_LPASS_LPAAUDIO_PLL_ALPHA_VAL_U_RESERVE_BITS_31_8_BMSK                                                    0xffffff00
#define HWIO_LPASS_LPAAUDIO_PLL_ALPHA_VAL_U_RESERVE_BITS_31_8_SHFT                                                             8
#define HWIO_LPASS_LPAAUDIO_PLL_ALPHA_VAL_U_PLL_ALPHA_39_32_BMSK                                                            0xff
#define HWIO_LPASS_LPAAUDIO_PLL_ALPHA_VAL_U_PLL_ALPHA_39_32_SHFT                                                               0

#define HWIO_LPASS_LPAAUDIO_PLL_USER_CTL_ADDR(x)                                                                      ((x) + 0x10)
#define HWIO_LPASS_LPAAUDIO_PLL_USER_CTL_RMSK                                                                         0xffffffff
#define HWIO_LPASS_LPAAUDIO_PLL_USER_CTL_IN(x)            \
                in_dword(HWIO_LPASS_LPAAUDIO_PLL_USER_CTL_ADDR(x))
#define HWIO_LPASS_LPAAUDIO_PLL_USER_CTL_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPAAUDIO_PLL_USER_CTL_ADDR(x), m)
#define HWIO_LPASS_LPAAUDIO_PLL_USER_CTL_OUT(x, v)            \
                out_dword(HWIO_LPASS_LPAAUDIO_PLL_USER_CTL_ADDR(x),v)
#define HWIO_LPASS_LPAAUDIO_PLL_USER_CTL_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_LPAAUDIO_PLL_USER_CTL_ADDR(x),m,v,HWIO_LPASS_LPAAUDIO_PLL_USER_CTL_IN(x))
#define HWIO_LPASS_LPAAUDIO_PLL_USER_CTL_RESERVE_BITS31_28_BMSK                                                       0xf0000000
#define HWIO_LPASS_LPAAUDIO_PLL_USER_CTL_RESERVE_BITS31_28_SHFT                                                               28
#define HWIO_LPASS_LPAAUDIO_PLL_USER_CTL_SSC_MODE_CONTROL_BMSK                                                         0x8000000
#define HWIO_LPASS_LPAAUDIO_PLL_USER_CTL_SSC_MODE_CONTROL_SHFT                                                                27
#define HWIO_LPASS_LPAAUDIO_PLL_USER_CTL_RESERVE_BITS26_25_BMSK                                                        0x6000000
#define HWIO_LPASS_LPAAUDIO_PLL_USER_CTL_RESERVE_BITS26_25_SHFT                                                               25
#define HWIO_LPASS_LPAAUDIO_PLL_USER_CTL_ALPHA_EN_BMSK                                                                 0x1000000
#define HWIO_LPASS_LPAAUDIO_PLL_USER_CTL_ALPHA_EN_SHFT                                                                        24
#define HWIO_LPASS_LPAAUDIO_PLL_USER_CTL_RESERVE_BITS23_22_BMSK                                                         0xc00000
#define HWIO_LPASS_LPAAUDIO_PLL_USER_CTL_RESERVE_BITS23_22_SHFT                                                               22
#define HWIO_LPASS_LPAAUDIO_PLL_USER_CTL_VCO_SEL_BMSK                                                                   0x300000
#define HWIO_LPASS_LPAAUDIO_PLL_USER_CTL_VCO_SEL_SHFT                                                                         20
#define HWIO_LPASS_LPAAUDIO_PLL_USER_CTL_RESERVE_BITS19_18_BMSK                                                          0xf8000
#define HWIO_LPASS_LPAAUDIO_PLL_USER_CTL_RESERVE_BITS19_18_SHFT                                                               15
#define HWIO_LPASS_LPAAUDIO_PLL_USER_CTL_PRE_DIV_RATIO_BMSK                                                               0x7000
#define HWIO_LPASS_LPAAUDIO_PLL_USER_CTL_PRE_DIV_RATIO_SHFT                                                                   12
#define HWIO_LPASS_LPAAUDIO_PLL_USER_CTL_POST_DIV_RATIO_BMSK                                                               0xf00
#define HWIO_LPASS_LPAAUDIO_PLL_USER_CTL_POST_DIV_RATIO_SHFT                                                                   8
#define HWIO_LPASS_LPAAUDIO_PLL_USER_CTL_OUTPUT_INV_BMSK                                                                    0x80
#define HWIO_LPASS_LPAAUDIO_PLL_USER_CTL_OUTPUT_INV_SHFT                                                                       7
#define HWIO_LPASS_LPAAUDIO_PLL_USER_CTL_RESERVE_BITS6_5_BMSK                                                               0x60
#define HWIO_LPASS_LPAAUDIO_PLL_USER_CTL_RESERVE_BITS6_5_SHFT                                                                  5
#define HWIO_LPASS_LPAAUDIO_PLL_USER_CTL_PLLOUT_LV_TEST_BMSK                                                                0x10
#define HWIO_LPASS_LPAAUDIO_PLL_USER_CTL_PLLOUT_LV_TEST_SHFT                                                                   4
#define HWIO_LPASS_LPAAUDIO_PLL_USER_CTL_PLLOUT_LV_EARLY_BMSK                                                                0x8
#define HWIO_LPASS_LPAAUDIO_PLL_USER_CTL_PLLOUT_LV_EARLY_SHFT                                                                  3
#define HWIO_LPASS_LPAAUDIO_PLL_USER_CTL_PLLOUT_LV_AUX2_BMSK                                                                 0x4
#define HWIO_LPASS_LPAAUDIO_PLL_USER_CTL_PLLOUT_LV_AUX2_SHFT                                                                   2
#define HWIO_LPASS_LPAAUDIO_PLL_USER_CTL_PLLOUT_LV_AUX_BMSK                                                                  0x2
#define HWIO_LPASS_LPAAUDIO_PLL_USER_CTL_PLLOUT_LV_AUX_SHFT                                                                    1
#define HWIO_LPASS_LPAAUDIO_PLL_USER_CTL_PLLOUT_LV_MAIN_BMSK                                                                 0x1
#define HWIO_LPASS_LPAAUDIO_PLL_USER_CTL_PLLOUT_LV_MAIN_SHFT                                                                   0

#define HWIO_LPASS_LPAAUDIO_PLL_USER_CTL_U_ADDR(x)                                                                    ((x) + 0x14)
#define HWIO_LPASS_LPAAUDIO_PLL_USER_CTL_U_RMSK                                                                       0xffffffff
#define HWIO_LPASS_LPAAUDIO_PLL_USER_CTL_U_IN(x)            \
                in_dword(HWIO_LPASS_LPAAUDIO_PLL_USER_CTL_U_ADDR(x))
#define HWIO_LPASS_LPAAUDIO_PLL_USER_CTL_U_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPAAUDIO_PLL_USER_CTL_U_ADDR(x), m)
#define HWIO_LPASS_LPAAUDIO_PLL_USER_CTL_U_OUT(x, v)            \
                out_dword(HWIO_LPASS_LPAAUDIO_PLL_USER_CTL_U_ADDR(x),v)
#define HWIO_LPASS_LPAAUDIO_PLL_USER_CTL_U_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_LPAAUDIO_PLL_USER_CTL_U_ADDR(x),m,v,HWIO_LPASS_LPAAUDIO_PLL_USER_CTL_U_IN(x))
#define HWIO_LPASS_LPAAUDIO_PLL_USER_CTL_U_CAL_L_VAL_BMSK                                                             0xffff0000
#define HWIO_LPASS_LPAAUDIO_PLL_USER_CTL_U_CAL_L_VAL_SHFT                                                                     16
#define HWIO_LPASS_LPAAUDIO_PLL_USER_CTL_U_COR_INDX_BMSK                                                                  0xc000
#define HWIO_LPASS_LPAAUDIO_PLL_USER_CTL_U_COR_INDX_SHFT                                                                      14
#define HWIO_LPASS_LPAAUDIO_PLL_USER_CTL_U_PLL_TYPE2_MODE_SEL_BMSK                                                        0x2000
#define HWIO_LPASS_LPAAUDIO_PLL_USER_CTL_U_PLL_TYPE2_MODE_SEL_SHFT                                                            13
#define HWIO_LPASS_LPAAUDIO_PLL_USER_CTL_U_PLL_TYPE2_MODE_TDC_SEL_BMSK                                                    0x1000
#define HWIO_LPASS_LPAAUDIO_PLL_USER_CTL_U_PLL_TYPE2_MODE_TDC_SEL_SHFT                                                        12
#define HWIO_LPASS_LPAAUDIO_PLL_USER_CTL_U_LATCH_INTERFACE_BYPASS_BMSK                                                     0x800
#define HWIO_LPASS_LPAAUDIO_PLL_USER_CTL_U_LATCH_INTERFACE_BYPASS_SHFT                                                        11
#define HWIO_LPASS_LPAAUDIO_PLL_USER_CTL_U_STATUS_REGISTER_BMSK                                                            0x700
#define HWIO_LPASS_LPAAUDIO_PLL_USER_CTL_U_STATUS_REGISTER_SHFT                                                                8
#define HWIO_LPASS_LPAAUDIO_PLL_USER_CTL_U_DSM_BMSK                                                                         0x80
#define HWIO_LPASS_LPAAUDIO_PLL_USER_CTL_U_DSM_SHFT                                                                            7
#define HWIO_LPASS_LPAAUDIO_PLL_USER_CTL_U_WRITE_STATE_BMSK                                                                 0x40
#define HWIO_LPASS_LPAAUDIO_PLL_USER_CTL_U_WRITE_STATE_SHFT                                                                    6
#define HWIO_LPASS_LPAAUDIO_PLL_USER_CTL_U_TARGET_CTL_BMSK                                                                  0x38
#define HWIO_LPASS_LPAAUDIO_PLL_USER_CTL_U_TARGET_CTL_SHFT                                                                     3
#define HWIO_LPASS_LPAAUDIO_PLL_USER_CTL_U_LOCK_DET_BMSK                                                                     0x4
#define HWIO_LPASS_LPAAUDIO_PLL_USER_CTL_U_LOCK_DET_SHFT                                                                       2
#define HWIO_LPASS_LPAAUDIO_PLL_USER_CTL_U_FREEZE_PLL_BMSK                                                                   0x2
#define HWIO_LPASS_LPAAUDIO_PLL_USER_CTL_U_FREEZE_PLL_SHFT                                                                     1
#define HWIO_LPASS_LPAAUDIO_PLL_USER_CTL_U_TOGGLE_DET_BMSK                                                                   0x1
#define HWIO_LPASS_LPAAUDIO_PLL_USER_CTL_U_TOGGLE_DET_SHFT                                                                     0

#define HWIO_LPASS_LPAAUDIO_PLL_CONFIG_CTL_ADDR(x)                                                                    ((x) + 0x18)
#define HWIO_LPASS_LPAAUDIO_PLL_CONFIG_CTL_RMSK                                                                       0xffffffff
#define HWIO_LPASS_LPAAUDIO_PLL_CONFIG_CTL_IN(x)            \
                in_dword(HWIO_LPASS_LPAAUDIO_PLL_CONFIG_CTL_ADDR(x))
#define HWIO_LPASS_LPAAUDIO_PLL_CONFIG_CTL_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPAAUDIO_PLL_CONFIG_CTL_ADDR(x), m)
#define HWIO_LPASS_LPAAUDIO_PLL_CONFIG_CTL_OUT(x, v)            \
                out_dword(HWIO_LPASS_LPAAUDIO_PLL_CONFIG_CTL_ADDR(x),v)
#define HWIO_LPASS_LPAAUDIO_PLL_CONFIG_CTL_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_LPAAUDIO_PLL_CONFIG_CTL_ADDR(x),m,v,HWIO_LPASS_LPAAUDIO_PLL_CONFIG_CTL_IN(x))
#define HWIO_LPASS_LPAAUDIO_PLL_CONFIG_CTL_SINGLE_DMET_MODE_ENABLE_BMSK                                               0x80000000
#define HWIO_LPASS_LPAAUDIO_PLL_CONFIG_CTL_SINGLE_DMET_MODE_ENABLE_SHFT                                                       31
#define HWIO_LPASS_LPAAUDIO_PLL_CONFIG_CTL_DMET_WINDOW_ENABLE_BMSK                                                    0x40000000
#define HWIO_LPASS_LPAAUDIO_PLL_CONFIG_CTL_DMET_WINDOW_ENABLE_SHFT                                                            30
#define HWIO_LPASS_LPAAUDIO_PLL_CONFIG_CTL_DELAY_SEL_BMSK                                                             0x30000000
#define HWIO_LPASS_LPAAUDIO_PLL_CONFIG_CTL_DELAY_SEL_SHFT                                                                     28
#define HWIO_LPASS_LPAAUDIO_PLL_CONFIG_CTL_KIBB_BMSK                                                                   0xe000000
#define HWIO_LPASS_LPAAUDIO_PLL_CONFIG_CTL_KIBB_SHFT                                                                          25
#define HWIO_LPASS_LPAAUDIO_PLL_CONFIG_CTL_KPBB_BMSK                                                                   0x1e00000
#define HWIO_LPASS_LPAAUDIO_PLL_CONFIG_CTL_KPBB_SHFT                                                                          21
#define HWIO_LPASS_LPAAUDIO_PLL_CONFIG_CTL_BB_MODE_BMSK                                                                 0x100000
#define HWIO_LPASS_LPAAUDIO_PLL_CONFIG_CTL_BB_MODE_SHFT                                                                       20
#define HWIO_LPASS_LPAAUDIO_PLL_CONFIG_CTL_LOCK_DET_THRESHOLD_BMSK                                                       0xff000
#define HWIO_LPASS_LPAAUDIO_PLL_CONFIG_CTL_LOCK_DET_THRESHOLD_SHFT                                                            12
#define HWIO_LPASS_LPAAUDIO_PLL_CONFIG_CTL_LOCK_DET_SAMPLE_SIZE_BMSK                                                       0xf00
#define HWIO_LPASS_LPAAUDIO_PLL_CONFIG_CTL_LOCK_DET_SAMPLE_SIZE_SHFT                                                           8
#define HWIO_LPASS_LPAAUDIO_PLL_CONFIG_CTL_MIN_GLITCH_THRESHOLD_BMSK                                                        0xc0
#define HWIO_LPASS_LPAAUDIO_PLL_CONFIG_CTL_MIN_GLITCH_THRESHOLD_SHFT                                                           6
#define HWIO_LPASS_LPAAUDIO_PLL_CONFIG_CTL_REF_CYCLE_BMSK                                                                   0x30
#define HWIO_LPASS_LPAAUDIO_PLL_CONFIG_CTL_REF_CYCLE_SHFT                                                                      4
#define HWIO_LPASS_LPAAUDIO_PLL_CONFIG_CTL_KFN_BMSK                                                                          0xf
#define HWIO_LPASS_LPAAUDIO_PLL_CONFIG_CTL_KFN_SHFT                                                                            0

#define HWIO_LPASS_LPAAUDIO_PLL_TEST_CTL_ADDR(x)                                                                      ((x) + 0x1c)
#define HWIO_LPASS_LPAAUDIO_PLL_TEST_CTL_RMSK                                                                         0xffffffff
#define HWIO_LPASS_LPAAUDIO_PLL_TEST_CTL_IN(x)            \
                in_dword(HWIO_LPASS_LPAAUDIO_PLL_TEST_CTL_ADDR(x))
#define HWIO_LPASS_LPAAUDIO_PLL_TEST_CTL_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPAAUDIO_PLL_TEST_CTL_ADDR(x), m)
#define HWIO_LPASS_LPAAUDIO_PLL_TEST_CTL_OUT(x, v)            \
                out_dword(HWIO_LPASS_LPAAUDIO_PLL_TEST_CTL_ADDR(x),v)
#define HWIO_LPASS_LPAAUDIO_PLL_TEST_CTL_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_LPAAUDIO_PLL_TEST_CTL_ADDR(x),m,v,HWIO_LPASS_LPAAUDIO_PLL_TEST_CTL_IN(x))
#define HWIO_LPASS_LPAAUDIO_PLL_TEST_CTL_BIAS_GEN_TRIM_BMSK                                                           0xe0000000
#define HWIO_LPASS_LPAAUDIO_PLL_TEST_CTL_BIAS_GEN_TRIM_SHFT                                                                   29
#define HWIO_LPASS_LPAAUDIO_PLL_TEST_CTL_PROCESS_CALB_BMSK                                                            0x1c000000
#define HWIO_LPASS_LPAAUDIO_PLL_TEST_CTL_PROCESS_CALB_SHFT                                                                    26
#define HWIO_LPASS_LPAAUDIO_PLL_TEST_CTL_OVERRIDE_PROCESS_CALB_BMSK                                                    0x2000000
#define HWIO_LPASS_LPAAUDIO_PLL_TEST_CTL_OVERRIDE_PROCESS_CALB_SHFT                                                           25
#define HWIO_LPASS_LPAAUDIO_PLL_TEST_CTL_FINE_FCW_BMSK                                                                 0x1f00000
#define HWIO_LPASS_LPAAUDIO_PLL_TEST_CTL_FINE_FCW_SHFT                                                                        20
#define HWIO_LPASS_LPAAUDIO_PLL_TEST_CTL_OVERRIDE_FINE_FCW_BMSK                                                          0x80000
#define HWIO_LPASS_LPAAUDIO_PLL_TEST_CTL_OVERRIDE_FINE_FCW_SHFT                                                               19
#define HWIO_LPASS_LPAAUDIO_PLL_TEST_CTL_COARSE_FCW_BMSK                                                                 0x7e000
#define HWIO_LPASS_LPAAUDIO_PLL_TEST_CTL_COARSE_FCW_SHFT                                                                      13
#define HWIO_LPASS_LPAAUDIO_PLL_TEST_CTL_OVERRIDE_COARSE_BMSK                                                             0x1000
#define HWIO_LPASS_LPAAUDIO_PLL_TEST_CTL_OVERRIDE_COARSE_SHFT                                                                 12
#define HWIO_LPASS_LPAAUDIO_PLL_TEST_CTL_DISABLE_LFSR_BMSK                                                                 0x800
#define HWIO_LPASS_LPAAUDIO_PLL_TEST_CTL_DISABLE_LFSR_SHFT                                                                    11
#define HWIO_LPASS_LPAAUDIO_PLL_TEST_CTL_DTEST_SEL_BMSK                                                                    0x700
#define HWIO_LPASS_LPAAUDIO_PLL_TEST_CTL_DTEST_SEL_SHFT                                                                        8
#define HWIO_LPASS_LPAAUDIO_PLL_TEST_CTL_DTEST_EN_BMSK                                                                      0x80
#define HWIO_LPASS_LPAAUDIO_PLL_TEST_CTL_DTEST_EN_SHFT                                                                         7
#define HWIO_LPASS_LPAAUDIO_PLL_TEST_CTL_LD_PLL_TH_BMSK                                                                     0x60
#define HWIO_LPASS_LPAAUDIO_PLL_TEST_CTL_LD_PLL_TH_SHFT                                                                        5
#define HWIO_LPASS_LPAAUDIO_PLL_TEST_CTL_LOCK_DET_SEL_BMSK                                                                  0x10
#define HWIO_LPASS_LPAAUDIO_PLL_TEST_CTL_LOCK_DET_SEL_SHFT                                                                     4
#define HWIO_LPASS_LPAAUDIO_PLL_TEST_CTL_ATEST0_SEL_BMSK                                                                     0xc
#define HWIO_LPASS_LPAAUDIO_PLL_TEST_CTL_ATEST0_SEL_SHFT                                                                       2
#define HWIO_LPASS_LPAAUDIO_PLL_TEST_CTL_BB_LOOP_GAIN_SEL_BMSK                                                               0x2
#define HWIO_LPASS_LPAAUDIO_PLL_TEST_CTL_BB_LOOP_GAIN_SEL_SHFT                                                                 1
#define HWIO_LPASS_LPAAUDIO_PLL_TEST_CTL_ATEST0_EN_BMSK                                                                      0x1
#define HWIO_LPASS_LPAAUDIO_PLL_TEST_CTL_ATEST0_EN_SHFT                                                                        0

#define HWIO_LPASS_LPAAUDIO_PLL_TEST_CTL_U_ADDR(x)                                                                    ((x) + 0x20)
#define HWIO_LPASS_LPAAUDIO_PLL_TEST_CTL_U_RMSK                                                                       0xffffffff
#define HWIO_LPASS_LPAAUDIO_PLL_TEST_CTL_U_IN(x)            \
                in_dword(HWIO_LPASS_LPAAUDIO_PLL_TEST_CTL_U_ADDR(x))
#define HWIO_LPASS_LPAAUDIO_PLL_TEST_CTL_U_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPAAUDIO_PLL_TEST_CTL_U_ADDR(x), m)
#define HWIO_LPASS_LPAAUDIO_PLL_TEST_CTL_U_OUT(x, v)            \
                out_dword(HWIO_LPASS_LPAAUDIO_PLL_TEST_CTL_U_ADDR(x),v)
#define HWIO_LPASS_LPAAUDIO_PLL_TEST_CTL_U_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_LPAAUDIO_PLL_TEST_CTL_U_ADDR(x),m,v,HWIO_LPASS_LPAAUDIO_PLL_TEST_CTL_U_IN(x))
#define HWIO_LPASS_LPAAUDIO_PLL_TEST_CTL_U_IFIXEDSCALE_BMSK                                                           0xc0000000
#define HWIO_LPASS_LPAAUDIO_PLL_TEST_CTL_U_IFIXEDSCALE_SHFT                                                                   30
#define HWIO_LPASS_LPAAUDIO_PLL_TEST_CTL_U_IBGSCALE_BMSK                                                              0x20000000
#define HWIO_LPASS_LPAAUDIO_PLL_TEST_CTL_U_IBGSCALE_SHFT                                                                      29
#define HWIO_LPASS_LPAAUDIO_PLL_TEST_CTL_U_MSB_LD_PLL_SAM_BMSK                                                        0x10000000
#define HWIO_LPASS_LPAAUDIO_PLL_TEST_CTL_U_MSB_LD_PLL_SAM_SHFT                                                                28
#define HWIO_LPASS_LPAAUDIO_PLL_TEST_CTL_U_IREFSCALE_BMSK                                                              0xc000000
#define HWIO_LPASS_LPAAUDIO_PLL_TEST_CTL_U_IREFSCALE_SHFT                                                                     26
#define HWIO_LPASS_LPAAUDIO_PLL_TEST_CTL_U_VCASSCALE_BMSK                                                              0x3000000
#define HWIO_LPASS_LPAAUDIO_PLL_TEST_CTL_U_VCASSCALE_SHFT                                                                     24
#define HWIO_LPASS_LPAAUDIO_PLL_TEST_CTL_U_IDCO_SCALE_BMSK                                                              0xc00000
#define HWIO_LPASS_LPAAUDIO_PLL_TEST_CTL_U_IDCO_SCALE_SHFT                                                                    22
#define HWIO_LPASS_LPAAUDIO_PLL_TEST_CTL_U_GLITCH_DET_CNT_LIMIT_BMSK                                                    0x300000
#define HWIO_LPASS_LPAAUDIO_PLL_TEST_CTL_U_GLITCH_DET_CNT_LIMIT_SHFT                                                          20
#define HWIO_LPASS_LPAAUDIO_PLL_TEST_CTL_U_DIS_GLITCH_PREVENTION_BMSK                                                    0x80000
#define HWIO_LPASS_LPAAUDIO_PLL_TEST_CTL_U_DIS_GLITCH_PREVENTION_SHFT                                                         19
#define HWIO_LPASS_LPAAUDIO_PLL_TEST_CTL_U_DTEST_MODE_SEL_U_BMSK                                                         0x60000
#define HWIO_LPASS_LPAAUDIO_PLL_TEST_CTL_U_DTEST_MODE_SEL_U_SHFT                                                              17
#define HWIO_LPASS_LPAAUDIO_PLL_TEST_CTL_U_DITHER_SEL_BMSK                                                               0x18000
#define HWIO_LPASS_LPAAUDIO_PLL_TEST_CTL_U_DITHER_SEL_SHFT                                                                    15
#define HWIO_LPASS_LPAAUDIO_PLL_TEST_CTL_U_SEL_2B_3B_CAL_BMSK                                                             0x4000
#define HWIO_LPASS_LPAAUDIO_PLL_TEST_CTL_U_SEL_2B_3B_CAL_SHFT                                                                 14
#define HWIO_LPASS_LPAAUDIO_PLL_TEST_CTL_U_OVERRIDE_FINE_FCW_MSB_BMSK                                                     0x2000
#define HWIO_LPASS_LPAAUDIO_PLL_TEST_CTL_U_OVERRIDE_FINE_FCW_MSB_SHFT                                                         13
#define HWIO_LPASS_LPAAUDIO_PLL_TEST_CTL_U_DTEST_MODE_SEL_BMSK                                                            0x1800
#define HWIO_LPASS_LPAAUDIO_PLL_TEST_CTL_U_DTEST_MODE_SEL_SHFT                                                                11
#define HWIO_LPASS_LPAAUDIO_PLL_TEST_CTL_U_NMO_OSC_SEL_BMSK                                                                0x600
#define HWIO_LPASS_LPAAUDIO_PLL_TEST_CTL_U_NMO_OSC_SEL_SHFT                                                                    9
#define HWIO_LPASS_LPAAUDIO_PLL_TEST_CTL_U_NMO_EN_BMSK                                                                     0x100
#define HWIO_LPASS_LPAAUDIO_PLL_TEST_CTL_U_NMO_EN_SHFT                                                                         8
#define HWIO_LPASS_LPAAUDIO_PLL_TEST_CTL_U_NOISE_MAG_BMSK                                                                   0xe0
#define HWIO_LPASS_LPAAUDIO_PLL_TEST_CTL_U_NOISE_MAG_SHFT                                                                      5
#define HWIO_LPASS_LPAAUDIO_PLL_TEST_CTL_U_NOISE_GEN_BMSK                                                                   0x10
#define HWIO_LPASS_LPAAUDIO_PLL_TEST_CTL_U_NOISE_GEN_SHFT                                                                      4
#define HWIO_LPASS_LPAAUDIO_PLL_TEST_CTL_U_OSC_BIAS_GND_BMSK                                                                 0x8
#define HWIO_LPASS_LPAAUDIO_PLL_TEST_CTL_U_OSC_BIAS_GND_SHFT                                                                   3
#define HWIO_LPASS_LPAAUDIO_PLL_TEST_CTL_U_PLL_TEST_OUT_SEL_BMSK                                                             0x6
#define HWIO_LPASS_LPAAUDIO_PLL_TEST_CTL_U_PLL_TEST_OUT_SEL_SHFT                                                               1
#define HWIO_LPASS_LPAAUDIO_PLL_TEST_CTL_U_CAL_CODE_UPDATE_BMSK                                                              0x1
#define HWIO_LPASS_LPAAUDIO_PLL_TEST_CTL_U_CAL_CODE_UPDATE_SHFT                                                                0

#define HWIO_LPASS_LPAAUDIO_PLL_STATUS_ADDR(x)                                                                        ((x) + 0x24)
#define HWIO_LPASS_LPAAUDIO_PLL_STATUS_RMSK                                                                           0xffffffff
#define HWIO_LPASS_LPAAUDIO_PLL_STATUS_IN(x)            \
                in_dword(HWIO_LPASS_LPAAUDIO_PLL_STATUS_ADDR(x))
#define HWIO_LPASS_LPAAUDIO_PLL_STATUS_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPAAUDIO_PLL_STATUS_ADDR(x), m)
#define HWIO_LPASS_LPAAUDIO_PLL_STATUS_STATUS_31_0_BMSK                                                               0xffffffff
#define HWIO_LPASS_LPAAUDIO_PLL_STATUS_STATUS_31_0_SHFT                                                                        0

#define HWIO_LPASS_LPAAUDIO_PLL_FREQ_CTL_ADDR(x)                                                                      ((x) + 0x28)
#define HWIO_LPASS_LPAAUDIO_PLL_FREQ_CTL_RMSK                                                                         0xffffffff
#define HWIO_LPASS_LPAAUDIO_PLL_FREQ_CTL_IN(x)            \
                in_dword(HWIO_LPASS_LPAAUDIO_PLL_FREQ_CTL_ADDR(x))
#define HWIO_LPASS_LPAAUDIO_PLL_FREQ_CTL_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPAAUDIO_PLL_FREQ_CTL_ADDR(x), m)
#define HWIO_LPASS_LPAAUDIO_PLL_FREQ_CTL_OUT(x, v)            \
                out_dword(HWIO_LPASS_LPAAUDIO_PLL_FREQ_CTL_ADDR(x),v)
#define HWIO_LPASS_LPAAUDIO_PLL_FREQ_CTL_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_LPAAUDIO_PLL_FREQ_CTL_ADDR(x),m,v,HWIO_LPASS_LPAAUDIO_PLL_FREQ_CTL_IN(x))
#define HWIO_LPASS_LPAAUDIO_PLL_FREQ_CTL_FREQUENCY_CTL_WORD_BMSK                                                      0xffffffff
#define HWIO_LPASS_LPAAUDIO_PLL_FREQ_CTL_FREQUENCY_CTL_WORD_SHFT                                                               0

#define HWIO_LPASS_LPAAUDIO_PLL_CLK_CGC_EN_ADDR(x)                                                                    ((x) + 0x2c)
#define HWIO_LPASS_LPAAUDIO_PLL_CLK_CGC_EN_RMSK                                                                             0x1f
#define HWIO_LPASS_LPAAUDIO_PLL_CLK_CGC_EN_IN(x)            \
                in_dword(HWIO_LPASS_LPAAUDIO_PLL_CLK_CGC_EN_ADDR(x))
#define HWIO_LPASS_LPAAUDIO_PLL_CLK_CGC_EN_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPAAUDIO_PLL_CLK_CGC_EN_ADDR(x), m)
#define HWIO_LPASS_LPAAUDIO_PLL_CLK_CGC_EN_OUT(x, v)            \
                out_dword(HWIO_LPASS_LPAAUDIO_PLL_CLK_CGC_EN_ADDR(x),v)
#define HWIO_LPASS_LPAAUDIO_PLL_CLK_CGC_EN_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_LPAAUDIO_PLL_CLK_CGC_EN_ADDR(x),m,v,HWIO_LPASS_LPAAUDIO_PLL_CLK_CGC_EN_IN(x))
#define HWIO_LPASS_LPAAUDIO_PLL_CLK_CGC_EN_CLK_AUX2_CDIV2_CGC_EN_BMSK                                                       0x10
#define HWIO_LPASS_LPAAUDIO_PLL_CLK_CGC_EN_CLK_AUX2_CDIV2_CGC_EN_SHFT                                                          4
#define HWIO_LPASS_LPAAUDIO_PLL_CLK_CGC_EN_CLK_AUX_CDIV5_CGC_EN_BMSK                                                         0x8
#define HWIO_LPASS_LPAAUDIO_PLL_CLK_CGC_EN_CLK_AUX_CDIV5_CGC_EN_SHFT                                                           3
#define HWIO_LPASS_LPAAUDIO_PLL_CLK_CGC_EN_CLK_AUX_CDIV3_CGC_EN_BMSK                                                         0x4
#define HWIO_LPASS_LPAAUDIO_PLL_CLK_CGC_EN_CLK_AUX_CDIV3_CGC_EN_SHFT                                                           2
#define HWIO_LPASS_LPAAUDIO_PLL_CLK_CGC_EN_CLK_MAIN_SVS_CGC_EN_BMSK                                                          0x2
#define HWIO_LPASS_LPAAUDIO_PLL_CLK_CGC_EN_CLK_MAIN_SVS_CGC_EN_SHFT                                                            1
#define HWIO_LPASS_LPAAUDIO_PLL_CLK_CGC_EN_CLK_MAIN_CGC_EN_BMSK                                                              0x1
#define HWIO_LPASS_LPAAUDIO_PLL_CLK_CGC_EN_CLK_MAIN_CGC_EN_SHFT                                                                0

#define HWIO_LPASS_LPAAUDIO_PLL_CLK_DIV_ADDR(x)                                                                       ((x) + 0x30)
#define HWIO_LPASS_LPAAUDIO_PLL_CLK_DIV_RMSK                                                                               0x1ff
#define HWIO_LPASS_LPAAUDIO_PLL_CLK_DIV_IN(x)            \
                in_dword(HWIO_LPASS_LPAAUDIO_PLL_CLK_DIV_ADDR(x))
#define HWIO_LPASS_LPAAUDIO_PLL_CLK_DIV_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPAAUDIO_PLL_CLK_DIV_ADDR(x), m)
#define HWIO_LPASS_LPAAUDIO_PLL_CLK_DIV_OUT(x, v)            \
                out_dword(HWIO_LPASS_LPAAUDIO_PLL_CLK_DIV_ADDR(x),v)
#define HWIO_LPASS_LPAAUDIO_PLL_CLK_DIV_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_LPAAUDIO_PLL_CLK_DIV_ADDR(x),m,v,HWIO_LPASS_LPAAUDIO_PLL_CLK_DIV_IN(x))
#define HWIO_LPASS_LPAAUDIO_PLL_CLK_DIV_CLK_AUX2_DIV_BMSK                                                                  0x180
#define HWIO_LPASS_LPAAUDIO_PLL_CLK_DIV_CLK_AUX2_DIV_SHFT                                                                      7
#define HWIO_LPASS_LPAAUDIO_PLL_CLK_DIV_RESERVE_BITS6_BMSK                                                                  0x40
#define HWIO_LPASS_LPAAUDIO_PLL_CLK_DIV_RESERVE_BITS6_SHFT                                                                     6
#define HWIO_LPASS_LPAAUDIO_PLL_CLK_DIV_CLK_AUX_DIV3_BMSK                                                                   0x3c
#define HWIO_LPASS_LPAAUDIO_PLL_CLK_DIV_CLK_AUX_DIV3_SHFT                                                                      2
#define HWIO_LPASS_LPAAUDIO_PLL_CLK_DIV_CLK_MAIN_DIV_BMSK                                                                    0x3
#define HWIO_LPASS_LPAAUDIO_PLL_CLK_DIV_CLK_MAIN_DIV_SHFT                                                                      0

#define HWIO_LPASS_LCC_LPAAUDIO_PLL_OUT_AUX_CDIVR_ADDR(x)                                                             ((x) + 0x34)
#define HWIO_LPASS_LCC_LPAAUDIO_PLL_OUT_AUX_CDIVR_RMSK                                                                       0xf
#define HWIO_LPASS_LCC_LPAAUDIO_PLL_OUT_AUX_CDIVR_IN(x)            \
                in_dword(HWIO_LPASS_LCC_LPAAUDIO_PLL_OUT_AUX_CDIVR_ADDR(x))
#define HWIO_LPASS_LCC_LPAAUDIO_PLL_OUT_AUX_CDIVR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LCC_LPAAUDIO_PLL_OUT_AUX_CDIVR_ADDR(x), m)
#define HWIO_LPASS_LCC_LPAAUDIO_PLL_OUT_AUX_CDIVR_OUT(x, v)            \
                out_dword(HWIO_LPASS_LCC_LPAAUDIO_PLL_OUT_AUX_CDIVR_ADDR(x),v)
#define HWIO_LPASS_LCC_LPAAUDIO_PLL_OUT_AUX_CDIVR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_LCC_LPAAUDIO_PLL_OUT_AUX_CDIVR_ADDR(x),m,v,HWIO_LPASS_LCC_LPAAUDIO_PLL_OUT_AUX_CDIVR_IN(x))
#define HWIO_LPASS_LCC_LPAAUDIO_PLL_OUT_AUX_CDIVR_CLK_DIV_BMSK                                                               0xf
#define HWIO_LPASS_LCC_LPAAUDIO_PLL_OUT_AUX_CDIVR_CLK_DIV_SHFT                                                                 0

#define HWIO_LPASS_LCC_LPAAUDIO_PLL_OUT_AUX2_CDIVR_ADDR(x)                                                            ((x) + 0x38)
#define HWIO_LPASS_LCC_LPAAUDIO_PLL_OUT_AUX2_CDIVR_RMSK                                                                      0x3
#define HWIO_LPASS_LCC_LPAAUDIO_PLL_OUT_AUX2_CDIVR_IN(x)            \
                in_dword(HWIO_LPASS_LCC_LPAAUDIO_PLL_OUT_AUX2_CDIVR_ADDR(x))
#define HWIO_LPASS_LCC_LPAAUDIO_PLL_OUT_AUX2_CDIVR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LCC_LPAAUDIO_PLL_OUT_AUX2_CDIVR_ADDR(x), m)
#define HWIO_LPASS_LCC_LPAAUDIO_PLL_OUT_AUX2_CDIVR_OUT(x, v)            \
                out_dword(HWIO_LPASS_LCC_LPAAUDIO_PLL_OUT_AUX2_CDIVR_ADDR(x),v)
#define HWIO_LPASS_LCC_LPAAUDIO_PLL_OUT_AUX2_CDIVR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_LCC_LPAAUDIO_PLL_OUT_AUX2_CDIVR_ADDR(x),m,v,HWIO_LPASS_LCC_LPAAUDIO_PLL_OUT_AUX2_CDIVR_IN(x))
#define HWIO_LPASS_LCC_LPAAUDIO_PLL_OUT_AUX2_CDIVR_CLK_DIV_BMSK                                                              0x3
#define HWIO_LPASS_LCC_LPAAUDIO_PLL_OUT_AUX2_CDIVR_CLK_DIV_SHFT                                                                0

#define HWIO_LPASS_LPAAUDIO_DIG_PLL_MODE_ADDR(x)                                                                      ((x) + 0x1000)
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_MODE_RMSK                                                                         0xffffffff
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_MODE_IN(x)            \
                in_dword(HWIO_LPASS_LPAAUDIO_DIG_PLL_MODE_ADDR(x))
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_MODE_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPAAUDIO_DIG_PLL_MODE_ADDR(x), m)
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_MODE_OUT(x, v)            \
                out_dword(HWIO_LPASS_LPAAUDIO_DIG_PLL_MODE_ADDR(x),v)
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_MODE_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_LPAAUDIO_DIG_PLL_MODE_ADDR(x),m,v,HWIO_LPASS_LPAAUDIO_DIG_PLL_MODE_IN(x))
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_MODE_PLL_LOCK_DET_BMSK                                                            0x80000000
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_MODE_PLL_LOCK_DET_SHFT                                                                    31
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_MODE_PLL_ACTIVE_FLAG_BMSK                                                         0x40000000
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_MODE_PLL_ACTIVE_FLAG_SHFT                                                                 30
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_MODE_PLL_ACK_LATCH_BMSK                                                           0x20000000
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_MODE_PLL_ACK_LATCH_SHFT                                                                   29
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_MODE_RESERVE_BITS28_24_BMSK                                                       0x1f000000
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_MODE_RESERVE_BITS28_24_SHFT                                                               24
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_MODE_PLL_HW_UPDATE_LOGIC_BYPASS_BMSK                                                0x800000
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_MODE_PLL_HW_UPDATE_LOGIC_BYPASS_SHFT                                                      23
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_MODE_PLL_UPDATE_BMSK                                                                0x400000
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_MODE_PLL_UPDATE_SHFT                                                                      22
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_MODE_PLL_VOTE_FSM_RESET_BMSK                                                        0x200000
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_MODE_PLL_VOTE_FSM_RESET_SHFT                                                              21
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_MODE_PLL_VOTE_FSM_ENA_BMSK                                                          0x100000
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_MODE_PLL_VOTE_FSM_ENA_SHFT                                                                20
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_MODE_PLL_BIAS_COUNT_BMSK                                                             0xfc000
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_MODE_PLL_BIAS_COUNT_SHFT                                                                  14
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_MODE_PLL_LOCK_COUNT_BMSK                                                              0x3f00
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_MODE_PLL_LOCK_COUNT_SHFT                                                                   8
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_MODE_RESERVE_BITS7_4_BMSK                                                               0xf0
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_MODE_RESERVE_BITS7_4_SHFT                                                                  4
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_MODE_PLL_PLLTEST_BMSK                                                                    0x8
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_MODE_PLL_PLLTEST_SHFT                                                                      3
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_MODE_PLL_RESET_N_BMSK                                                                    0x4
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_MODE_PLL_RESET_N_SHFT                                                                      2
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_MODE_PLL_BYPASSNL_BMSK                                                                   0x2
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_MODE_PLL_BYPASSNL_SHFT                                                                     1
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_MODE_PLL_OUTCTRL_BMSK                                                                    0x1
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_MODE_PLL_OUTCTRL_SHFT                                                                      0

#define HWIO_LPASS_LPAAUDIO_DIG_PLL_L_VAL_ADDR(x)                                                                     ((x) + 0x1004)
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_L_VAL_RMSK                                                                        0xffffffff
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_L_VAL_IN(x)            \
                in_dword(HWIO_LPASS_LPAAUDIO_DIG_PLL_L_VAL_ADDR(x))
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_L_VAL_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPAAUDIO_DIG_PLL_L_VAL_ADDR(x), m)
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_L_VAL_OUT(x, v)            \
                out_dword(HWIO_LPASS_LPAAUDIO_DIG_PLL_L_VAL_ADDR(x),v)
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_L_VAL_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_LPAAUDIO_DIG_PLL_L_VAL_ADDR(x),m,v,HWIO_LPASS_LPAAUDIO_DIG_PLL_L_VAL_IN(x))
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_L_VAL_RESERVE_BITS_31_16_BMSK                                                     0xffff0000
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_L_VAL_RESERVE_BITS_31_16_SHFT                                                             16
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_L_VAL_PLL_L_BMSK                                                                      0xffff
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_L_VAL_PLL_L_SHFT                                                                           0

#define HWIO_LPASS_LPAAUDIO_DIG_PLL_ALPHA_VAL_ADDR(x)                                                                 ((x) + 0x1008)
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_ALPHA_VAL_RMSK                                                                    0xffffffff
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_ALPHA_VAL_IN(x)            \
                in_dword(HWIO_LPASS_LPAAUDIO_DIG_PLL_ALPHA_VAL_ADDR(x))
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_ALPHA_VAL_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPAAUDIO_DIG_PLL_ALPHA_VAL_ADDR(x), m)
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_ALPHA_VAL_OUT(x, v)            \
                out_dword(HWIO_LPASS_LPAAUDIO_DIG_PLL_ALPHA_VAL_ADDR(x),v)
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_ALPHA_VAL_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_LPAAUDIO_DIG_PLL_ALPHA_VAL_ADDR(x),m,v,HWIO_LPASS_LPAAUDIO_DIG_PLL_ALPHA_VAL_IN(x))
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_ALPHA_VAL_PLL_ALPHA_31_0_BMSK                                                     0xffffffff
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_ALPHA_VAL_PLL_ALPHA_31_0_SHFT                                                              0

#define HWIO_LPASS_LPAAUDIO_DIG_PLL_ALPHA_VAL_U_ADDR(x)                                                               ((x) + 0x100c)
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_ALPHA_VAL_U_RMSK                                                                  0xffffffff
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_ALPHA_VAL_U_IN(x)            \
                in_dword(HWIO_LPASS_LPAAUDIO_DIG_PLL_ALPHA_VAL_U_ADDR(x))
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_ALPHA_VAL_U_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPAAUDIO_DIG_PLL_ALPHA_VAL_U_ADDR(x), m)
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_ALPHA_VAL_U_OUT(x, v)            \
                out_dword(HWIO_LPASS_LPAAUDIO_DIG_PLL_ALPHA_VAL_U_ADDR(x),v)
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_ALPHA_VAL_U_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_LPAAUDIO_DIG_PLL_ALPHA_VAL_U_ADDR(x),m,v,HWIO_LPASS_LPAAUDIO_DIG_PLL_ALPHA_VAL_U_IN(x))
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_ALPHA_VAL_U_RESERVE_BITS_31_8_BMSK                                                0xffffff00
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_ALPHA_VAL_U_RESERVE_BITS_31_8_SHFT                                                         8
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_ALPHA_VAL_U_PLL_ALPHA_39_32_BMSK                                                        0xff
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_ALPHA_VAL_U_PLL_ALPHA_39_32_SHFT                                                           0

#define HWIO_LPASS_LPAAUDIO_DIG_PLL_USER_CTL_ADDR(x)                                                                  ((x) + 0x1010)
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_USER_CTL_RMSK                                                                     0xffffffff
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_USER_CTL_IN(x)            \
                in_dword(HWIO_LPASS_LPAAUDIO_DIG_PLL_USER_CTL_ADDR(x))
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_USER_CTL_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPAAUDIO_DIG_PLL_USER_CTL_ADDR(x), m)
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_USER_CTL_OUT(x, v)            \
                out_dword(HWIO_LPASS_LPAAUDIO_DIG_PLL_USER_CTL_ADDR(x),v)
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_USER_CTL_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_LPAAUDIO_DIG_PLL_USER_CTL_ADDR(x),m,v,HWIO_LPASS_LPAAUDIO_DIG_PLL_USER_CTL_IN(x))
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_USER_CTL_RESERVE_BITS31_28_BMSK                                                   0xf0000000
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_USER_CTL_RESERVE_BITS31_28_SHFT                                                           28
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_USER_CTL_SSC_MODE_CONTROL_BMSK                                                     0x8000000
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_USER_CTL_SSC_MODE_CONTROL_SHFT                                                            27
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_USER_CTL_RESERVE_BITS26_25_BMSK                                                    0x6000000
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_USER_CTL_RESERVE_BITS26_25_SHFT                                                           25
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_USER_CTL_ALPHA_EN_BMSK                                                             0x1000000
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_USER_CTL_ALPHA_EN_SHFT                                                                    24
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_USER_CTL_RESERVE_BITS23_22_BMSK                                                     0xc00000
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_USER_CTL_RESERVE_BITS23_22_SHFT                                                           22
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_USER_CTL_VCO_SEL_BMSK                                                               0x300000
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_USER_CTL_VCO_SEL_SHFT                                                                     20
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_USER_CTL_RESERVE_BITS19_18_BMSK                                                      0xc0000
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_USER_CTL_RESERVE_BITS19_18_SHFT                                                           18
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_USER_CTL_AUX_POST_DIV_RATIO_BMSK                                                     0x38000
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_USER_CTL_AUX_POST_DIV_RATIO_SHFT                                                          15
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_USER_CTL_PRE_DIV_RATIO_BMSK                                                           0x7000
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_USER_CTL_PRE_DIV_RATIO_SHFT                                                               12
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_USER_CTL_POST_DIV_RATIO_BMSK                                                           0xf00
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_USER_CTL_POST_DIV_RATIO_SHFT                                                               8
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_USER_CTL_OUTPUT_INV_BMSK                                                                0x80
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_USER_CTL_OUTPUT_INV_SHFT                                                                   7
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_USER_CTL_RESERVE_BITS6_5_BMSK                                                           0x60
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_USER_CTL_RESERVE_BITS6_5_SHFT                                                              5
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_USER_CTL_PLLOUT_LV_TEST_BMSK                                                            0x10
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_USER_CTL_PLLOUT_LV_TEST_SHFT                                                               4
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_USER_CTL_PLLOUT_LV_EARLY_BMSK                                                            0x8
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_USER_CTL_PLLOUT_LV_EARLY_SHFT                                                              3
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_USER_CTL_PLLOUT_LV_AUX2_BMSK                                                             0x4
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_USER_CTL_PLLOUT_LV_AUX2_SHFT                                                               2
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_USER_CTL_PLLOUT_LV_AUX_BMSK                                                              0x2
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_USER_CTL_PLLOUT_LV_AUX_SHFT                                                                1
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_USER_CTL_PLLOUT_LV_MAIN_BMSK                                                             0x1
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_USER_CTL_PLLOUT_LV_MAIN_SHFT                                                               0

#define HWIO_LPASS_LPAAUDIO_DIG_PLL_USER_CTL_U_ADDR(x)                                                                ((x) + 0x1014)
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_USER_CTL_U_RMSK                                                                   0xffffffff
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_USER_CTL_U_IN(x)            \
                in_dword(HWIO_LPASS_LPAAUDIO_DIG_PLL_USER_CTL_U_ADDR(x))
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_USER_CTL_U_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPAAUDIO_DIG_PLL_USER_CTL_U_ADDR(x), m)
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_USER_CTL_U_OUT(x, v)            \
                out_dword(HWIO_LPASS_LPAAUDIO_DIG_PLL_USER_CTL_U_ADDR(x),v)
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_USER_CTL_U_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_LPAAUDIO_DIG_PLL_USER_CTL_U_ADDR(x),m,v,HWIO_LPASS_LPAAUDIO_DIG_PLL_USER_CTL_U_IN(x))
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_USER_CTL_U_CAL_L_VAL_BMSK                                                         0xffff0000
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_USER_CTL_U_CAL_L_VAL_SHFT                                                                 16
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_USER_CTL_U_COR_INDX_BMSK                                                              0xc000
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_USER_CTL_U_COR_INDX_SHFT                                                                  14
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_USER_CTL_U_PLL_TYPE2_MODE_SEL_BMSK                                                    0x2000
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_USER_CTL_U_PLL_TYPE2_MODE_SEL_SHFT                                                        13
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_USER_CTL_U_PLL_TYPE2_MODE_TDC_SEL_BMSK                                                0x1000
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_USER_CTL_U_PLL_TYPE2_MODE_TDC_SEL_SHFT                                                    12
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_USER_CTL_U_LATCH_INTERFACE_BYPASS_BMSK                                                 0x800
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_USER_CTL_U_LATCH_INTERFACE_BYPASS_SHFT                                                    11
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_USER_CTL_U_STATUS_REGISTER_BMSK                                                        0x700
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_USER_CTL_U_STATUS_REGISTER_SHFT                                                            8
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_USER_CTL_U_DSM_BMSK                                                                     0x80
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_USER_CTL_U_DSM_SHFT                                                                        7
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_USER_CTL_U_WRITE_STATE_BMSK                                                             0x40
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_USER_CTL_U_WRITE_STATE_SHFT                                                                6
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_USER_CTL_U_TARGET_CTL_BMSK                                                              0x38
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_USER_CTL_U_TARGET_CTL_SHFT                                                                 3
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_USER_CTL_U_LOCK_DET_BMSK                                                                 0x4
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_USER_CTL_U_LOCK_DET_SHFT                                                                   2
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_USER_CTL_U_FREEZE_PLL_BMSK                                                               0x2
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_USER_CTL_U_FREEZE_PLL_SHFT                                                                 1
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_USER_CTL_U_TOGGLE_DET_BMSK                                                               0x1
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_USER_CTL_U_TOGGLE_DET_SHFT                                                                 0

#define HWIO_LPASS_LPAAUDIO_DIG_PLL_CONFIG_CTL_ADDR(x)                                                                ((x) + 0x1018)
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_CONFIG_CTL_RMSK                                                                   0xffffffff
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_CONFIG_CTL_IN(x)            \
                in_dword(HWIO_LPASS_LPAAUDIO_DIG_PLL_CONFIG_CTL_ADDR(x))
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_CONFIG_CTL_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPAAUDIO_DIG_PLL_CONFIG_CTL_ADDR(x), m)
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_CONFIG_CTL_OUT(x, v)            \
                out_dword(HWIO_LPASS_LPAAUDIO_DIG_PLL_CONFIG_CTL_ADDR(x),v)
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_CONFIG_CTL_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_LPAAUDIO_DIG_PLL_CONFIG_CTL_ADDR(x),m,v,HWIO_LPASS_LPAAUDIO_DIG_PLL_CONFIG_CTL_IN(x))
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_CONFIG_CTL_SINGLE_DMET_MODE_ENABLE_BMSK                                           0x80000000
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_CONFIG_CTL_SINGLE_DMET_MODE_ENABLE_SHFT                                                   31
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_CONFIG_CTL_DMET_WINDOW_ENABLE_BMSK                                                0x40000000
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_CONFIG_CTL_DMET_WINDOW_ENABLE_SHFT                                                        30
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_CONFIG_CTL_DELAY_SEL_BMSK                                                         0x30000000
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_CONFIG_CTL_DELAY_SEL_SHFT                                                                 28
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_CONFIG_CTL_KIBB_BMSK                                                               0xe000000
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_CONFIG_CTL_KIBB_SHFT                                                                      25
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_CONFIG_CTL_KPBB_BMSK                                                               0x1e00000
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_CONFIG_CTL_KPBB_SHFT                                                                      21
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_CONFIG_CTL_BB_MODE_BMSK                                                             0x100000
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_CONFIG_CTL_BB_MODE_SHFT                                                                   20
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_CONFIG_CTL_LOCK_DET_THRESHOLD_BMSK                                                   0xff000
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_CONFIG_CTL_LOCK_DET_THRESHOLD_SHFT                                                        12
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_CONFIG_CTL_LOCK_DET_SAMPLE_SIZE_BMSK                                                   0xf00
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_CONFIG_CTL_LOCK_DET_SAMPLE_SIZE_SHFT                                                       8
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_CONFIG_CTL_MIN_GLITCH_THRESHOLD_BMSK                                                    0xc0
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_CONFIG_CTL_MIN_GLITCH_THRESHOLD_SHFT                                                       6
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_CONFIG_CTL_REF_CYCLE_BMSK                                                               0x30
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_CONFIG_CTL_REF_CYCLE_SHFT                                                                  4
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_CONFIG_CTL_KFN_BMSK                                                                      0xf
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_CONFIG_CTL_KFN_SHFT                                                                        0

#define HWIO_LPASS_LPAAUDIO_DIG_PLL_TEST_CTL_ADDR(x)                                                                  ((x) + 0x101c)
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_TEST_CTL_RMSK                                                                     0xffffffff
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_TEST_CTL_IN(x)            \
                in_dword(HWIO_LPASS_LPAAUDIO_DIG_PLL_TEST_CTL_ADDR(x))
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_TEST_CTL_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPAAUDIO_DIG_PLL_TEST_CTL_ADDR(x), m)
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_TEST_CTL_OUT(x, v)            \
                out_dword(HWIO_LPASS_LPAAUDIO_DIG_PLL_TEST_CTL_ADDR(x),v)
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_TEST_CTL_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_LPAAUDIO_DIG_PLL_TEST_CTL_ADDR(x),m,v,HWIO_LPASS_LPAAUDIO_DIG_PLL_TEST_CTL_IN(x))
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_TEST_CTL_BIAS_GEN_TRIM_BMSK                                                       0xe0000000
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_TEST_CTL_BIAS_GEN_TRIM_SHFT                                                               29
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_TEST_CTL_PROCESS_CALB_BMSK                                                        0x1c000000
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_TEST_CTL_PROCESS_CALB_SHFT                                                                26
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_TEST_CTL_OVERRIDE_PROCESS_CALB_BMSK                                                0x2000000
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_TEST_CTL_OVERRIDE_PROCESS_CALB_SHFT                                                       25
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_TEST_CTL_FINE_FCW_BMSK                                                             0x1f00000
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_TEST_CTL_FINE_FCW_SHFT                                                                    20
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_TEST_CTL_OVERRIDE_FINE_FCW_BMSK                                                      0x80000
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_TEST_CTL_OVERRIDE_FINE_FCW_SHFT                                                           19
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_TEST_CTL_COARSE_FCW_BMSK                                                             0x7e000
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_TEST_CTL_COARSE_FCW_SHFT                                                                  13
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_TEST_CTL_OVERRIDE_COARSE_BMSK                                                         0x1000
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_TEST_CTL_OVERRIDE_COARSE_SHFT                                                             12
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_TEST_CTL_DISABLE_LFSR_BMSK                                                             0x800
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_TEST_CTL_DISABLE_LFSR_SHFT                                                                11
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_TEST_CTL_DTEST_SEL_BMSK                                                                0x700
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_TEST_CTL_DTEST_SEL_SHFT                                                                    8
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_TEST_CTL_DTEST_EN_BMSK                                                                  0x80
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_TEST_CTL_DTEST_EN_SHFT                                                                     7
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_TEST_CTL_LD_PLL_TH_BMSK                                                                 0x60
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_TEST_CTL_LD_PLL_TH_SHFT                                                                    5
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_TEST_CTL_LOCK_DET_SEL_BMSK                                                              0x10
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_TEST_CTL_LOCK_DET_SEL_SHFT                                                                 4
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_TEST_CTL_ATEST0_SEL_BMSK                                                                 0xc
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_TEST_CTL_ATEST0_SEL_SHFT                                                                   2
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_TEST_CTL_BB_LOOP_GAIN_SEL_BMSK                                                           0x2
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_TEST_CTL_BB_LOOP_GAIN_SEL_SHFT                                                             1
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_TEST_CTL_ATEST0_EN_BMSK                                                                  0x1
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_TEST_CTL_ATEST0_EN_SHFT                                                                    0

#define HWIO_LPASS_LPAAUDIO_DIG_PLL_TEST_CTL_U_ADDR(x)                                                                ((x) + 0x1020)
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_TEST_CTL_U_RMSK                                                                   0xffffffff
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_TEST_CTL_U_IN(x)            \
                in_dword(HWIO_LPASS_LPAAUDIO_DIG_PLL_TEST_CTL_U_ADDR(x))
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_TEST_CTL_U_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPAAUDIO_DIG_PLL_TEST_CTL_U_ADDR(x), m)
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_TEST_CTL_U_OUT(x, v)            \
                out_dword(HWIO_LPASS_LPAAUDIO_DIG_PLL_TEST_CTL_U_ADDR(x),v)
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_TEST_CTL_U_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_LPAAUDIO_DIG_PLL_TEST_CTL_U_ADDR(x),m,v,HWIO_LPASS_LPAAUDIO_DIG_PLL_TEST_CTL_U_IN(x))
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_TEST_CTL_U_IFIXEDSCALE_BMSK                                                       0xc0000000
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_TEST_CTL_U_IFIXEDSCALE_SHFT                                                               30
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_TEST_CTL_U_IBGSCALE_BMSK                                                          0x20000000
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_TEST_CTL_U_IBGSCALE_SHFT                                                                  29
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_TEST_CTL_U_MSB_LD_PLL_SAM_BMSK                                                    0x10000000
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_TEST_CTL_U_MSB_LD_PLL_SAM_SHFT                                                            28
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_TEST_CTL_U_IREFSCALE_BMSK                                                          0xc000000
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_TEST_CTL_U_IREFSCALE_SHFT                                                                 26
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_TEST_CTL_U_VCASSCALE_BMSK                                                          0x3000000
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_TEST_CTL_U_VCASSCALE_SHFT                                                                 24
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_TEST_CTL_U_IDCO_SCALE_BMSK                                                          0xc00000
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_TEST_CTL_U_IDCO_SCALE_SHFT                                                                22
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_TEST_CTL_U_GLITCH_DET_CNT_LIMIT_BMSK                                                0x300000
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_TEST_CTL_U_GLITCH_DET_CNT_LIMIT_SHFT                                                      20
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_TEST_CTL_U_DIS_GLITCH_PREVENTION_BMSK                                                0x80000
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_TEST_CTL_U_DIS_GLITCH_PREVENTION_SHFT                                                     19
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_TEST_CTL_U_DTEST_MODE_SEL_U_BMSK                                                     0x60000
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_TEST_CTL_U_DTEST_MODE_SEL_U_SHFT                                                          17
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_TEST_CTL_U_DITHER_SEL_BMSK                                                           0x18000
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_TEST_CTL_U_DITHER_SEL_SHFT                                                                15
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_TEST_CTL_U_SEL_2B_3B_CAL_BMSK                                                         0x4000
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_TEST_CTL_U_SEL_2B_3B_CAL_SHFT                                                             14
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_TEST_CTL_U_OVERRIDE_FINE_FCW_MSB_BMSK                                                 0x2000
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_TEST_CTL_U_OVERRIDE_FINE_FCW_MSB_SHFT                                                     13
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_TEST_CTL_U_DTEST_MODE_SEL_BMSK                                                        0x1800
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_TEST_CTL_U_DTEST_MODE_SEL_SHFT                                                            11
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_TEST_CTL_U_NMO_OSC_SEL_BMSK                                                            0x600
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_TEST_CTL_U_NMO_OSC_SEL_SHFT                                                                9
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_TEST_CTL_U_NMO_EN_BMSK                                                                 0x100
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_TEST_CTL_U_NMO_EN_SHFT                                                                     8
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_TEST_CTL_U_NOISE_MAG_BMSK                                                               0xe0
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_TEST_CTL_U_NOISE_MAG_SHFT                                                                  5
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_TEST_CTL_U_NOISE_GEN_BMSK                                                               0x10
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_TEST_CTL_U_NOISE_GEN_SHFT                                                                  4
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_TEST_CTL_U_OSC_BIAS_GND_BMSK                                                             0x8
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_TEST_CTL_U_OSC_BIAS_GND_SHFT                                                               3
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_TEST_CTL_U_PLL_TEST_OUT_SEL_BMSK                                                         0x6
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_TEST_CTL_U_PLL_TEST_OUT_SEL_SHFT                                                           1
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_TEST_CTL_U_CAL_CODE_UPDATE_BMSK                                                          0x1
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_TEST_CTL_U_CAL_CODE_UPDATE_SHFT                                                            0

#define HWIO_LPASS_LPAAUDIO_DIG_PLL_STATUS_ADDR(x)                                                                    ((x) + 0x1024)
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_STATUS_RMSK                                                                       0xffffffff
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_STATUS_IN(x)            \
                in_dword(HWIO_LPASS_LPAAUDIO_DIG_PLL_STATUS_ADDR(x))
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_STATUS_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPAAUDIO_DIG_PLL_STATUS_ADDR(x), m)
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_STATUS_STATUS_31_0_BMSK                                                           0xffffffff
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_STATUS_STATUS_31_0_SHFT                                                                    0

#define HWIO_LPASS_LPAAUDIO_DIG_PLL_FREQ_CTL_ADDR(x)                                                                  ((x) + 0x1028)
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_FREQ_CTL_RMSK                                                                     0xffffffff
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_FREQ_CTL_IN(x)            \
                in_dword(HWIO_LPASS_LPAAUDIO_DIG_PLL_FREQ_CTL_ADDR(x))
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_FREQ_CTL_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPAAUDIO_DIG_PLL_FREQ_CTL_ADDR(x), m)
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_FREQ_CTL_OUT(x, v)            \
                out_dword(HWIO_LPASS_LPAAUDIO_DIG_PLL_FREQ_CTL_ADDR(x),v)
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_FREQ_CTL_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_LPAAUDIO_DIG_PLL_FREQ_CTL_ADDR(x),m,v,HWIO_LPASS_LPAAUDIO_DIG_PLL_FREQ_CTL_IN(x))
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_FREQ_CTL_FREQUENCY_CTL_WORD_BMSK                                                  0xffffffff
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_FREQ_CTL_FREQUENCY_CTL_WORD_SHFT                                                           0

#define HWIO_LPASS_LPAAUDIO_DIG_PLL_CLK_CGC_EN_ADDR(x)                                                                ((x) + 0x102c)
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_CLK_CGC_EN_RMSK                                                                          0xf
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_CLK_CGC_EN_IN(x)            \
                in_dword(HWIO_LPASS_LPAAUDIO_DIG_PLL_CLK_CGC_EN_ADDR(x))
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_CLK_CGC_EN_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPAAUDIO_DIG_PLL_CLK_CGC_EN_ADDR(x), m)
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_CLK_CGC_EN_OUT(x, v)            \
                out_dword(HWIO_LPASS_LPAAUDIO_DIG_PLL_CLK_CGC_EN_ADDR(x),v)
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_CLK_CGC_EN_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_LPAAUDIO_DIG_PLL_CLK_CGC_EN_ADDR(x),m,v,HWIO_LPASS_LPAAUDIO_DIG_PLL_CLK_CGC_EN_IN(x))
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_CLK_CGC_EN_CLK_AUX_CGC_EN_BMSK                                                           0x8
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_CLK_CGC_EN_CLK_AUX_CGC_EN_SHFT                                                             3
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_CLK_CGC_EN_CLK_MAIN_CGC_EN_BMSK                                                          0x4
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_CLK_CGC_EN_CLK_MAIN_CGC_EN_SHFT                                                            2
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_CLK_CGC_EN_CLK_EARLY_SVS_CGC_EN_BMSK                                                     0x2
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_CLK_CGC_EN_CLK_EARLY_SVS_CGC_EN_SHFT                                                       1
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_CLK_CGC_EN_CLK_EARLY_CGC_EN_BMSK                                                         0x1
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_CLK_CGC_EN_CLK_EARLY_CGC_EN_SHFT                                                           0

#define HWIO_LPASS_LPAAUDIO_DIG_PLL_CLK_DIV_ADDR(x)                                                                   ((x) + 0x1030)
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_CLK_DIV_RMSK                                                                             0x3
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_CLK_DIV_IN(x)            \
                in_dword(HWIO_LPASS_LPAAUDIO_DIG_PLL_CLK_DIV_ADDR(x))
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_CLK_DIV_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPAAUDIO_DIG_PLL_CLK_DIV_ADDR(x), m)
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_CLK_DIV_OUT(x, v)            \
                out_dword(HWIO_LPASS_LPAAUDIO_DIG_PLL_CLK_DIV_ADDR(x),v)
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_CLK_DIV_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_LPAAUDIO_DIG_PLL_CLK_DIV_ADDR(x),m,v,HWIO_LPASS_LPAAUDIO_DIG_PLL_CLK_DIV_IN(x))
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_CLK_DIV_CLK_MAIN_DIV_BMSK                                                                0x3
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_CLK_DIV_CLK_MAIN_DIV_SHFT                                                                  0

#define HWIO_LPASS_LPAAUDIO_PLL_REF_CLK_SRC_SEL_ADDR(x)                                                               ((x) + 0x2000)
#define HWIO_LPASS_LPAAUDIO_PLL_REF_CLK_SRC_SEL_RMSK                                                                         0x3
#define HWIO_LPASS_LPAAUDIO_PLL_REF_CLK_SRC_SEL_IN(x)            \
                in_dword(HWIO_LPASS_LPAAUDIO_PLL_REF_CLK_SRC_SEL_ADDR(x))
#define HWIO_LPASS_LPAAUDIO_PLL_REF_CLK_SRC_SEL_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPAAUDIO_PLL_REF_CLK_SRC_SEL_ADDR(x), m)
#define HWIO_LPASS_LPAAUDIO_PLL_REF_CLK_SRC_SEL_OUT(x, v)            \
                out_dword(HWIO_LPASS_LPAAUDIO_PLL_REF_CLK_SRC_SEL_ADDR(x),v)
#define HWIO_LPASS_LPAAUDIO_PLL_REF_CLK_SRC_SEL_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_LPAAUDIO_PLL_REF_CLK_SRC_SEL_ADDR(x),m,v,HWIO_LPASS_LPAAUDIO_PLL_REF_CLK_SRC_SEL_IN(x))
#define HWIO_LPASS_LPAAUDIO_PLL_REF_CLK_SRC_SEL_PLL_CLK_SRC_SEL_BMSK                                                         0x3
#define HWIO_LPASS_LPAAUDIO_PLL_REF_CLK_SRC_SEL_PLL_CLK_SRC_SEL_SHFT                                                           0

#define HWIO_LPASS_LPAAUDIO_DIG_PLL_REF_CLK_SRC_SEL_ADDR(x)                                                           ((x) + 0x3000)
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_REF_CLK_SRC_SEL_RMSK                                                                     0x3
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_REF_CLK_SRC_SEL_IN(x)            \
                in_dword(HWIO_LPASS_LPAAUDIO_DIG_PLL_REF_CLK_SRC_SEL_ADDR(x))
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_REF_CLK_SRC_SEL_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPAAUDIO_DIG_PLL_REF_CLK_SRC_SEL_ADDR(x), m)
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_REF_CLK_SRC_SEL_OUT(x, v)            \
                out_dword(HWIO_LPASS_LPAAUDIO_DIG_PLL_REF_CLK_SRC_SEL_ADDR(x),v)
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_REF_CLK_SRC_SEL_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_LPAAUDIO_DIG_PLL_REF_CLK_SRC_SEL_ADDR(x),m,v,HWIO_LPASS_LPAAUDIO_DIG_PLL_REF_CLK_SRC_SEL_IN(x))
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_REF_CLK_SRC_SEL_PLL_CLK_SRC_SEL_BMSK                                                     0x3
#define HWIO_LPASS_LPAAUDIO_DIG_PLL_REF_CLK_SRC_SEL_PLL_CLK_SRC_SEL_SHFT                                                       0

#define HWIO_LPASS_JBIST_TEST_CTL_ADDR(x)                                                                             ((x) + 0x3e14)
#define HWIO_LPASS_JBIST_TEST_CTL_RMSK                                                                                0xffffffff
#define HWIO_LPASS_JBIST_TEST_CTL_IN(x)            \
                in_dword(HWIO_LPASS_JBIST_TEST_CTL_ADDR(x))
#define HWIO_LPASS_JBIST_TEST_CTL_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_JBIST_TEST_CTL_ADDR(x), m)
#define HWIO_LPASS_JBIST_TEST_CTL_OUT(x, v)            \
                out_dword(HWIO_LPASS_JBIST_TEST_CTL_ADDR(x),v)
#define HWIO_LPASS_JBIST_TEST_CTL_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_JBIST_TEST_CTL_ADDR(x),m,v,HWIO_LPASS_JBIST_TEST_CTL_IN(x))
#define HWIO_LPASS_JBIST_TEST_CTL_JBIST_TEST_CTL_BMSK                                                                 0xffffffff
#define HWIO_LPASS_JBIST_TEST_CTL_JBIST_TEST_CTL_SHFT                                                                          0

#define HWIO_LPASS_QDSP6SS_PLL_REF_CLK_SRC_SEL_ADDR(x)                                                                ((x) + 0x4000)
#define HWIO_LPASS_QDSP6SS_PLL_REF_CLK_SRC_SEL_RMSK                                                                          0x1
#define HWIO_LPASS_QDSP6SS_PLL_REF_CLK_SRC_SEL_IN(x)            \
                in_dword(HWIO_LPASS_QDSP6SS_PLL_REF_CLK_SRC_SEL_ADDR(x))
#define HWIO_LPASS_QDSP6SS_PLL_REF_CLK_SRC_SEL_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_QDSP6SS_PLL_REF_CLK_SRC_SEL_ADDR(x), m)
#define HWIO_LPASS_QDSP6SS_PLL_REF_CLK_SRC_SEL_OUT(x, v)            \
                out_dword(HWIO_LPASS_QDSP6SS_PLL_REF_CLK_SRC_SEL_ADDR(x),v)
#define HWIO_LPASS_QDSP6SS_PLL_REF_CLK_SRC_SEL_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_QDSP6SS_PLL_REF_CLK_SRC_SEL_ADDR(x),m,v,HWIO_LPASS_QDSP6SS_PLL_REF_CLK_SRC_SEL_IN(x))
#define HWIO_LPASS_QDSP6SS_PLL_REF_CLK_SRC_SEL_PLL_CLK_SRC_SEL_BMSK                                                          0x1
#define HWIO_LPASS_QDSP6SS_PLL_REF_CLK_SRC_SEL_PLL_CLK_SRC_SEL_SHFT                                                            0

#define HWIO_LPASS_LCC_APPS_VOTE_LPASS_CORE_GDS_ADDR(x)                                                               ((x) + 0x5000)
#define HWIO_LPASS_LCC_APPS_VOTE_LPASS_CORE_GDS_RMSK                                                                  0x80000001
#define HWIO_LPASS_LCC_APPS_VOTE_LPASS_CORE_GDS_IN(x)            \
                in_dword(HWIO_LPASS_LCC_APPS_VOTE_LPASS_CORE_GDS_ADDR(x))
#define HWIO_LPASS_LCC_APPS_VOTE_LPASS_CORE_GDS_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LCC_APPS_VOTE_LPASS_CORE_GDS_ADDR(x), m)
#define HWIO_LPASS_LCC_APPS_VOTE_LPASS_CORE_GDS_OUT(x, v)            \
                out_dword(HWIO_LPASS_LCC_APPS_VOTE_LPASS_CORE_GDS_ADDR(x),v)
#define HWIO_LPASS_LCC_APPS_VOTE_LPASS_CORE_GDS_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_LCC_APPS_VOTE_LPASS_CORE_GDS_ADDR(x),m,v,HWIO_LPASS_LCC_APPS_VOTE_LPASS_CORE_GDS_IN(x))
#define HWIO_LPASS_LCC_APPS_VOTE_LPASS_CORE_GDS_PWR_ON_BMSK                                                           0x80000000
#define HWIO_LPASS_LCC_APPS_VOTE_LPASS_CORE_GDS_PWR_ON_SHFT                                                                   31
#define HWIO_LPASS_LCC_APPS_VOTE_LPASS_CORE_GDS_SW_COLLAPSE_BMSK                                                             0x1
#define HWIO_LPASS_LCC_APPS_VOTE_LPASS_CORE_GDS_SW_COLLAPSE_SHFT                                                               0

#define HWIO_LPASS_LCC_Q6_VOTE_LPASS_CORE_GDS_ADDR(x)                                                                 ((x) + 0x6000)
#define HWIO_LPASS_LCC_Q6_VOTE_LPASS_CORE_GDS_RMSK                                                                    0x80000001
#define HWIO_LPASS_LCC_Q6_VOTE_LPASS_CORE_GDS_IN(x)            \
                in_dword(HWIO_LPASS_LCC_Q6_VOTE_LPASS_CORE_GDS_ADDR(x))
#define HWIO_LPASS_LCC_Q6_VOTE_LPASS_CORE_GDS_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LCC_Q6_VOTE_LPASS_CORE_GDS_ADDR(x), m)
#define HWIO_LPASS_LCC_Q6_VOTE_LPASS_CORE_GDS_OUT(x, v)            \
                out_dword(HWIO_LPASS_LCC_Q6_VOTE_LPASS_CORE_GDS_ADDR(x),v)
#define HWIO_LPASS_LCC_Q6_VOTE_LPASS_CORE_GDS_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_LCC_Q6_VOTE_LPASS_CORE_GDS_ADDR(x),m,v,HWIO_LPASS_LCC_Q6_VOTE_LPASS_CORE_GDS_IN(x))
#define HWIO_LPASS_LCC_Q6_VOTE_LPASS_CORE_GDS_PWR_ON_BMSK                                                             0x80000000
#define HWIO_LPASS_LCC_Q6_VOTE_LPASS_CORE_GDS_PWR_ON_SHFT                                                                     31
#define HWIO_LPASS_LCC_Q6_VOTE_LPASS_CORE_GDS_SW_COLLAPSE_BMSK                                                               0x1
#define HWIO_LPASS_LCC_Q6_VOTE_LPASS_CORE_GDS_SW_COLLAPSE_SHFT                                                                 0

#define HWIO_LPASS_LCC_OTHER_VOTE_LPASS_CORE_GDS_ADDR(x)                                                              ((x) + 0x7000)
#define HWIO_LPASS_LCC_OTHER_VOTE_LPASS_CORE_GDS_RMSK                                                                 0x80000001
#define HWIO_LPASS_LCC_OTHER_VOTE_LPASS_CORE_GDS_IN(x)            \
                in_dword(HWIO_LPASS_LCC_OTHER_VOTE_LPASS_CORE_GDS_ADDR(x))
#define HWIO_LPASS_LCC_OTHER_VOTE_LPASS_CORE_GDS_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LCC_OTHER_VOTE_LPASS_CORE_GDS_ADDR(x), m)
#define HWIO_LPASS_LCC_OTHER_VOTE_LPASS_CORE_GDS_OUT(x, v)            \
                out_dword(HWIO_LPASS_LCC_OTHER_VOTE_LPASS_CORE_GDS_ADDR(x),v)
#define HWIO_LPASS_LCC_OTHER_VOTE_LPASS_CORE_GDS_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_LCC_OTHER_VOTE_LPASS_CORE_GDS_ADDR(x),m,v,HWIO_LPASS_LCC_OTHER_VOTE_LPASS_CORE_GDS_IN(x))
#define HWIO_LPASS_LCC_OTHER_VOTE_LPASS_CORE_GDS_PWR_ON_BMSK                                                          0x80000000
#define HWIO_LPASS_LCC_OTHER_VOTE_LPASS_CORE_GDS_PWR_ON_SHFT                                                                  31
#define HWIO_LPASS_LCC_OTHER_VOTE_LPASS_CORE_GDS_SW_COLLAPSE_BMSK                                                            0x1
#define HWIO_LPASS_LCC_OTHER_VOTE_LPASS_CORE_GDS_SW_COLLAPSE_SHFT                                                              0

#define HWIO_LPASS_AUDIO_CORE_BCR_ADDR(x)                                                                             ((x) + 0x8000)
#define HWIO_LPASS_AUDIO_CORE_BCR_RMSK                                                                                0xf0000007
#define HWIO_LPASS_AUDIO_CORE_BCR_IN(x)            \
                in_dword(HWIO_LPASS_AUDIO_CORE_BCR_ADDR(x))
#define HWIO_LPASS_AUDIO_CORE_BCR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AUDIO_CORE_BCR_ADDR(x), m)
#define HWIO_LPASS_AUDIO_CORE_BCR_OUT(x, v)            \
                out_dword(HWIO_LPASS_AUDIO_CORE_BCR_ADDR(x),v)
#define HWIO_LPASS_AUDIO_CORE_BCR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AUDIO_CORE_BCR_ADDR(x),m,v,HWIO_LPASS_AUDIO_CORE_BCR_IN(x))
#define HWIO_LPASS_AUDIO_CORE_BCR_DFD_STATUS_BMSK                                                                     0x80000000
#define HWIO_LPASS_AUDIO_CORE_BCR_DFD_STATUS_SHFT                                                                             31
#define HWIO_LPASS_AUDIO_CORE_BCR_BCR_RESET_STATE_BMSK                                                                0x60000000
#define HWIO_LPASS_AUDIO_CORE_BCR_BCR_RESET_STATE_SHFT                                                                        29
#define HWIO_LPASS_AUDIO_CORE_BCR_LPASS_BUS_ABORT_ACK_STATUS_BMSK                                                     0x10000000
#define HWIO_LPASS_AUDIO_CORE_BCR_LPASS_BUS_ABORT_ACK_STATUS_SHFT                                                             28
#define HWIO_LPASS_AUDIO_CORE_BCR_FORCE_RESET_BMSK                                                                           0x4
#define HWIO_LPASS_AUDIO_CORE_BCR_FORCE_RESET_SHFT                                                                             2
#define HWIO_LPASS_AUDIO_CORE_BCR_DFD_EN_BMSK                                                                                0x2
#define HWIO_LPASS_AUDIO_CORE_BCR_DFD_EN_SHFT                                                                                  1
#define HWIO_LPASS_AUDIO_CORE_BCR_BLK_ARES_BMSK                                                                              0x1
#define HWIO_LPASS_AUDIO_CORE_BCR_BLK_ARES_SHFT                                                                                0

#define HWIO_LPASS_AUDIO_CORE_BCR_SLP_CBCR_ADDR(x)                                                                    ((x) + 0x8004)
#define HWIO_LPASS_AUDIO_CORE_BCR_SLP_CBCR_RMSK                                                                       0x80000003
#define HWIO_LPASS_AUDIO_CORE_BCR_SLP_CBCR_IN(x)            \
                in_dword(HWIO_LPASS_AUDIO_CORE_BCR_SLP_CBCR_ADDR(x))
#define HWIO_LPASS_AUDIO_CORE_BCR_SLP_CBCR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AUDIO_CORE_BCR_SLP_CBCR_ADDR(x), m)
#define HWIO_LPASS_AUDIO_CORE_BCR_SLP_CBCR_OUT(x, v)            \
                out_dword(HWIO_LPASS_AUDIO_CORE_BCR_SLP_CBCR_ADDR(x),v)
#define HWIO_LPASS_AUDIO_CORE_BCR_SLP_CBCR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AUDIO_CORE_BCR_SLP_CBCR_ADDR(x),m,v,HWIO_LPASS_AUDIO_CORE_BCR_SLP_CBCR_IN(x))
#define HWIO_LPASS_AUDIO_CORE_BCR_SLP_CBCR_CLK_OFF_BMSK                                                               0x80000000
#define HWIO_LPASS_AUDIO_CORE_BCR_SLP_CBCR_CLK_OFF_SHFT                                                                       31
#define HWIO_LPASS_AUDIO_CORE_BCR_SLP_CBCR_HW_CTL_BMSK                                                                       0x2
#define HWIO_LPASS_AUDIO_CORE_BCR_SLP_CBCR_HW_CTL_SHFT                                                                         1
#define HWIO_LPASS_AUDIO_CORE_BCR_SLP_CBCR_CLK_ENABLE_BMSK                                                                   0x1
#define HWIO_LPASS_AUDIO_CORE_BCR_SLP_CBCR_CLK_ENABLE_SHFT                                                                     0

#define HWIO_LPASS_AUDIO_WRAPPER_BCR_ADDR(x)                                                                          ((x) + 0x9000)
#define HWIO_LPASS_AUDIO_WRAPPER_BCR_RMSK                                                                             0x80000003
#define HWIO_LPASS_AUDIO_WRAPPER_BCR_IN(x)            \
                in_dword(HWIO_LPASS_AUDIO_WRAPPER_BCR_ADDR(x))
#define HWIO_LPASS_AUDIO_WRAPPER_BCR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AUDIO_WRAPPER_BCR_ADDR(x), m)
#define HWIO_LPASS_AUDIO_WRAPPER_BCR_OUT(x, v)            \
                out_dword(HWIO_LPASS_AUDIO_WRAPPER_BCR_ADDR(x),v)
#define HWIO_LPASS_AUDIO_WRAPPER_BCR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AUDIO_WRAPPER_BCR_ADDR(x),m,v,HWIO_LPASS_AUDIO_WRAPPER_BCR_IN(x))
#define HWIO_LPASS_AUDIO_WRAPPER_BCR_DFD_STATUS_BMSK                                                                  0x80000000
#define HWIO_LPASS_AUDIO_WRAPPER_BCR_DFD_STATUS_SHFT                                                                          31
#define HWIO_LPASS_AUDIO_WRAPPER_BCR_DFD_EN_BMSK                                                                             0x2
#define HWIO_LPASS_AUDIO_WRAPPER_BCR_DFD_EN_SHFT                                                                               1
#define HWIO_LPASS_AUDIO_WRAPPER_BCR_BLK_ARES_BMSK                                                                           0x1
#define HWIO_LPASS_AUDIO_WRAPPER_BCR_BLK_ARES_SHFT                                                                             0

#define HWIO_LPASS_Q6SS_BCR_ADDR(x)                                                                                   ((x) + 0xa000)
#define HWIO_LPASS_Q6SS_BCR_RMSK                                                                                      0x80000003
#define HWIO_LPASS_Q6SS_BCR_IN(x)            \
                in_dword(HWIO_LPASS_Q6SS_BCR_ADDR(x))
#define HWIO_LPASS_Q6SS_BCR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_Q6SS_BCR_ADDR(x), m)
#define HWIO_LPASS_Q6SS_BCR_OUT(x, v)            \
                out_dword(HWIO_LPASS_Q6SS_BCR_ADDR(x),v)
#define HWIO_LPASS_Q6SS_BCR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_Q6SS_BCR_ADDR(x),m,v,HWIO_LPASS_Q6SS_BCR_IN(x))
#define HWIO_LPASS_Q6SS_BCR_DFD_STATUS_BMSK                                                                           0x80000000
#define HWIO_LPASS_Q6SS_BCR_DFD_STATUS_SHFT                                                                                   31
#define HWIO_LPASS_Q6SS_BCR_DFD_EN_BMSK                                                                                      0x2
#define HWIO_LPASS_Q6SS_BCR_DFD_EN_SHFT                                                                                        1
#define HWIO_LPASS_Q6SS_BCR_BLK_ARES_BMSK                                                                                    0x1
#define HWIO_LPASS_Q6SS_BCR_BLK_ARES_SHFT                                                                                      0

#define HWIO_LPASS_Q6SS_BCR_SLP_CBCR_ADDR(x)                                                                          ((x) + 0xa004)
#define HWIO_LPASS_Q6SS_BCR_SLP_CBCR_RMSK                                                                             0x80000003
#define HWIO_LPASS_Q6SS_BCR_SLP_CBCR_IN(x)            \
                in_dword(HWIO_LPASS_Q6SS_BCR_SLP_CBCR_ADDR(x))
#define HWIO_LPASS_Q6SS_BCR_SLP_CBCR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_Q6SS_BCR_SLP_CBCR_ADDR(x), m)
#define HWIO_LPASS_Q6SS_BCR_SLP_CBCR_OUT(x, v)            \
                out_dword(HWIO_LPASS_Q6SS_BCR_SLP_CBCR_ADDR(x),v)
#define HWIO_LPASS_Q6SS_BCR_SLP_CBCR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_Q6SS_BCR_SLP_CBCR_ADDR(x),m,v,HWIO_LPASS_Q6SS_BCR_SLP_CBCR_IN(x))
#define HWIO_LPASS_Q6SS_BCR_SLP_CBCR_CLK_OFF_BMSK                                                                     0x80000000
#define HWIO_LPASS_Q6SS_BCR_SLP_CBCR_CLK_OFF_SHFT                                                                             31
#define HWIO_LPASS_Q6SS_BCR_SLP_CBCR_HW_CTL_BMSK                                                                             0x2
#define HWIO_LPASS_Q6SS_BCR_SLP_CBCR_HW_CTL_SHFT                                                                               1
#define HWIO_LPASS_Q6SS_BCR_SLP_CBCR_CLK_ENABLE_BMSK                                                                         0x1
#define HWIO_LPASS_Q6SS_BCR_SLP_CBCR_CLK_ENABLE_SHFT                                                                           0

#define HWIO_LPASS_AUDIO_CORE_GDSCR_ADDR(x)                                                                           ((x) + 0xb000)
#define HWIO_LPASS_AUDIO_CORE_GDSCR_RMSK                                                                              0xf8ffffff
#define HWIO_LPASS_AUDIO_CORE_GDSCR_IN(x)            \
                in_dword(HWIO_LPASS_AUDIO_CORE_GDSCR_ADDR(x))
#define HWIO_LPASS_AUDIO_CORE_GDSCR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AUDIO_CORE_GDSCR_ADDR(x), m)
#define HWIO_LPASS_AUDIO_CORE_GDSCR_OUT(x, v)            \
                out_dword(HWIO_LPASS_AUDIO_CORE_GDSCR_ADDR(x),v)
#define HWIO_LPASS_AUDIO_CORE_GDSCR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AUDIO_CORE_GDSCR_ADDR(x),m,v,HWIO_LPASS_AUDIO_CORE_GDSCR_IN(x))
#define HWIO_LPASS_AUDIO_CORE_GDSCR_PWR_ON_BMSK                                                                       0x80000000
#define HWIO_LPASS_AUDIO_CORE_GDSCR_PWR_ON_SHFT                                                                               31
#define HWIO_LPASS_AUDIO_CORE_GDSCR_GDSC_STATE_BMSK                                                                   0x78000000
#define HWIO_LPASS_AUDIO_CORE_GDSCR_GDSC_STATE_SHFT                                                                           27
#define HWIO_LPASS_AUDIO_CORE_GDSCR_EN_REST_WAIT_BMSK                                                                   0xf00000
#define HWIO_LPASS_AUDIO_CORE_GDSCR_EN_REST_WAIT_SHFT                                                                         20
#define HWIO_LPASS_AUDIO_CORE_GDSCR_EN_FEW_WAIT_BMSK                                                                     0xf0000
#define HWIO_LPASS_AUDIO_CORE_GDSCR_EN_FEW_WAIT_SHFT                                                                          16
#define HWIO_LPASS_AUDIO_CORE_GDSCR_CLK_DIS_WAIT_BMSK                                                                     0xf000
#define HWIO_LPASS_AUDIO_CORE_GDSCR_CLK_DIS_WAIT_SHFT                                                                         12
#define HWIO_LPASS_AUDIO_CORE_GDSCR_RETAIN_FF_ENABLE_BMSK                                                                  0x800
#define HWIO_LPASS_AUDIO_CORE_GDSCR_RETAIN_FF_ENABLE_SHFT                                                                     11
#define HWIO_LPASS_AUDIO_CORE_GDSCR_RESTORE_BMSK                                                                           0x400
#define HWIO_LPASS_AUDIO_CORE_GDSCR_RESTORE_SHFT                                                                              10
#define HWIO_LPASS_AUDIO_CORE_GDSCR_SAVE_BMSK                                                                              0x200
#define HWIO_LPASS_AUDIO_CORE_GDSCR_SAVE_SHFT                                                                                  9
#define HWIO_LPASS_AUDIO_CORE_GDSCR_RETAIN_BMSK                                                                            0x100
#define HWIO_LPASS_AUDIO_CORE_GDSCR_RETAIN_SHFT                                                                                8
#define HWIO_LPASS_AUDIO_CORE_GDSCR_EN_REST_BMSK                                                                            0x80
#define HWIO_LPASS_AUDIO_CORE_GDSCR_EN_REST_SHFT                                                                               7
#define HWIO_LPASS_AUDIO_CORE_GDSCR_EN_FEW_BMSK                                                                             0x40
#define HWIO_LPASS_AUDIO_CORE_GDSCR_EN_FEW_SHFT                                                                                6
#define HWIO_LPASS_AUDIO_CORE_GDSCR_CLAMP_IO_BMSK                                                                           0x20
#define HWIO_LPASS_AUDIO_CORE_GDSCR_CLAMP_IO_SHFT                                                                              5
#define HWIO_LPASS_AUDIO_CORE_GDSCR_CLK_DISABLE_BMSK                                                                        0x10
#define HWIO_LPASS_AUDIO_CORE_GDSCR_CLK_DISABLE_SHFT                                                                           4
#define HWIO_LPASS_AUDIO_CORE_GDSCR_PD_ARES_BMSK                                                                             0x8
#define HWIO_LPASS_AUDIO_CORE_GDSCR_PD_ARES_SHFT                                                                               3
#define HWIO_LPASS_AUDIO_CORE_GDSCR_SW_OVERRIDE_BMSK                                                                         0x4
#define HWIO_LPASS_AUDIO_CORE_GDSCR_SW_OVERRIDE_SHFT                                                                           2
#define HWIO_LPASS_AUDIO_CORE_GDSCR_HW_CONTROL_BMSK                                                                          0x2
#define HWIO_LPASS_AUDIO_CORE_GDSCR_HW_CONTROL_SHFT                                                                            1
#define HWIO_LPASS_AUDIO_CORE_GDSCR_SW_COLLAPSE_BMSK                                                                         0x1
#define HWIO_LPASS_AUDIO_CORE_GDSCR_SW_COLLAPSE_SHFT                                                                           0

#define HWIO_LPASS_AUDIO_CORE_CFG_GDSCR_ADDR(x)                                                                       ((x) + 0xb004)
#define HWIO_LPASS_AUDIO_CORE_CFG_GDSCR_RMSK                                                                          0xffffffff
#define HWIO_LPASS_AUDIO_CORE_CFG_GDSCR_IN(x)            \
                in_dword(HWIO_LPASS_AUDIO_CORE_CFG_GDSCR_ADDR(x))
#define HWIO_LPASS_AUDIO_CORE_CFG_GDSCR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AUDIO_CORE_CFG_GDSCR_ADDR(x), m)
#define HWIO_LPASS_AUDIO_CORE_CFG_GDSCR_OUT(x, v)            \
                out_dword(HWIO_LPASS_AUDIO_CORE_CFG_GDSCR_ADDR(x),v)
#define HWIO_LPASS_AUDIO_CORE_CFG_GDSCR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AUDIO_CORE_CFG_GDSCR_ADDR(x),m,v,HWIO_LPASS_AUDIO_CORE_CFG_GDSCR_IN(x))
#define HWIO_LPASS_AUDIO_CORE_CFG_GDSCR_GDSC_SPARE_CTRL_IN_BMSK                                                       0xf0000000
#define HWIO_LPASS_AUDIO_CORE_CFG_GDSCR_GDSC_SPARE_CTRL_IN_SHFT                                                               28
#define HWIO_LPASS_AUDIO_CORE_CFG_GDSCR_GDSC_SPARE_CTRL_OUT_BMSK                                                       0xc000000
#define HWIO_LPASS_AUDIO_CORE_CFG_GDSCR_GDSC_SPARE_CTRL_OUT_SHFT                                                              26
#define HWIO_LPASS_AUDIO_CORE_CFG_GDSCR_GDSC_PWR_UP_START_BMSK                                                         0x2000000
#define HWIO_LPASS_AUDIO_CORE_CFG_GDSCR_GDSC_PWR_UP_START_SHFT                                                                25
#define HWIO_LPASS_AUDIO_CORE_CFG_GDSCR_GDSC_PWR_DWN_START_BMSK                                                        0x1000000
#define HWIO_LPASS_AUDIO_CORE_CFG_GDSCR_GDSC_PWR_DWN_START_SHFT                                                               24
#define HWIO_LPASS_AUDIO_CORE_CFG_GDSCR_GDSC_CFG_FSM_STATE_STATUS_BMSK                                                  0xf00000
#define HWIO_LPASS_AUDIO_CORE_CFG_GDSCR_GDSC_CFG_FSM_STATE_STATUS_SHFT                                                        20
#define HWIO_LPASS_AUDIO_CORE_CFG_GDSCR_GDSC_MEM_PWR_ACK_STATUS_BMSK                                                     0x80000
#define HWIO_LPASS_AUDIO_CORE_CFG_GDSCR_GDSC_MEM_PWR_ACK_STATUS_SHFT                                                          19
#define HWIO_LPASS_AUDIO_CORE_CFG_GDSCR_GDSC_ENR_ACK_STATUS_BMSK                                                         0x40000
#define HWIO_LPASS_AUDIO_CORE_CFG_GDSCR_GDSC_ENR_ACK_STATUS_SHFT                                                              18
#define HWIO_LPASS_AUDIO_CORE_CFG_GDSCR_GDSC_ENF_ACK_STATUS_BMSK                                                         0x20000
#define HWIO_LPASS_AUDIO_CORE_CFG_GDSCR_GDSC_ENF_ACK_STATUS_SHFT                                                              17
#define HWIO_LPASS_AUDIO_CORE_CFG_GDSCR_GDSC_POWER_UP_COMPLETE_BMSK                                                      0x10000
#define HWIO_LPASS_AUDIO_CORE_CFG_GDSCR_GDSC_POWER_UP_COMPLETE_SHFT                                                           16
#define HWIO_LPASS_AUDIO_CORE_CFG_GDSCR_GDSC_POWER_DOWN_COMPLETE_BMSK                                                     0x8000
#define HWIO_LPASS_AUDIO_CORE_CFG_GDSCR_GDSC_POWER_DOWN_COMPLETE_SHFT                                                         15
#define HWIO_LPASS_AUDIO_CORE_CFG_GDSCR_SOFTWARE_CONTROL_OVERRIDE_BMSK                                                    0x7800
#define HWIO_LPASS_AUDIO_CORE_CFG_GDSCR_SOFTWARE_CONTROL_OVERRIDE_SHFT                                                        11
#define HWIO_LPASS_AUDIO_CORE_CFG_GDSCR_GDSC_HANDSHAKE_DIS_BMSK                                                            0x400
#define HWIO_LPASS_AUDIO_CORE_CFG_GDSCR_GDSC_HANDSHAKE_DIS_SHFT                                                               10
#define HWIO_LPASS_AUDIO_CORE_CFG_GDSCR_GDSC_MEM_PERI_FORCE_IN_SW_BMSK                                                     0x200
#define HWIO_LPASS_AUDIO_CORE_CFG_GDSCR_GDSC_MEM_PERI_FORCE_IN_SW_SHFT                                                         9
#define HWIO_LPASS_AUDIO_CORE_CFG_GDSCR_GDSC_MEM_CORE_FORCE_IN_SW_BMSK                                                     0x100
#define HWIO_LPASS_AUDIO_CORE_CFG_GDSCR_GDSC_MEM_CORE_FORCE_IN_SW_SHFT                                                         8
#define HWIO_LPASS_AUDIO_CORE_CFG_GDSCR_GDSC_PHASE_RESET_EN_SW_BMSK                                                         0x80
#define HWIO_LPASS_AUDIO_CORE_CFG_GDSCR_GDSC_PHASE_RESET_EN_SW_SHFT                                                            7
#define HWIO_LPASS_AUDIO_CORE_CFG_GDSCR_GDSC_PHASE_RESET_DELAY_COUNT_SW_BMSK                                                0x60
#define HWIO_LPASS_AUDIO_CORE_CFG_GDSCR_GDSC_PHASE_RESET_DELAY_COUNT_SW_SHFT                                                   5
#define HWIO_LPASS_AUDIO_CORE_CFG_GDSCR_GDSC_PSCBC_PWR_DWN_SW_BMSK                                                          0x10
#define HWIO_LPASS_AUDIO_CORE_CFG_GDSCR_GDSC_PSCBC_PWR_DWN_SW_SHFT                                                             4
#define HWIO_LPASS_AUDIO_CORE_CFG_GDSCR_UNCLAMP_IO_SOFTWARE_OVERRIDE_BMSK                                                    0x8
#define HWIO_LPASS_AUDIO_CORE_CFG_GDSCR_UNCLAMP_IO_SOFTWARE_OVERRIDE_SHFT                                                      3
#define HWIO_LPASS_AUDIO_CORE_CFG_GDSCR_SAVE_RESTORE_SOFTWARE_OVERRIDE_BMSK                                                  0x4
#define HWIO_LPASS_AUDIO_CORE_CFG_GDSCR_SAVE_RESTORE_SOFTWARE_OVERRIDE_SHFT                                                    2
#define HWIO_LPASS_AUDIO_CORE_CFG_GDSCR_CLAMP_IO_SOFTWARE_OVERRIDE_BMSK                                                      0x2
#define HWIO_LPASS_AUDIO_CORE_CFG_GDSCR_CLAMP_IO_SOFTWARE_OVERRIDE_SHFT                                                        1
#define HWIO_LPASS_AUDIO_CORE_CFG_GDSCR_DISABLE_CLK_SOFTWARE_OVERRIDE_BMSK                                                   0x1
#define HWIO_LPASS_AUDIO_CORE_CFG_GDSCR_DISABLE_CLK_SOFTWARE_OVERRIDE_SHFT                                                     0

#define HWIO_LPASS_AUDIO_CORE_CFG2_GDSCR_ADDR(x)                                                                      ((x) + 0xb008)
#define HWIO_LPASS_AUDIO_CORE_CFG2_GDSCR_RMSK                                                                            0x1ffff
#define HWIO_LPASS_AUDIO_CORE_CFG2_GDSCR_IN(x)            \
                in_dword(HWIO_LPASS_AUDIO_CORE_CFG2_GDSCR_ADDR(x))
#define HWIO_LPASS_AUDIO_CORE_CFG2_GDSCR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AUDIO_CORE_CFG2_GDSCR_ADDR(x), m)
#define HWIO_LPASS_AUDIO_CORE_CFG2_GDSCR_OUT(x, v)            \
                out_dword(HWIO_LPASS_AUDIO_CORE_CFG2_GDSCR_ADDR(x),v)
#define HWIO_LPASS_AUDIO_CORE_CFG2_GDSCR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AUDIO_CORE_CFG2_GDSCR_ADDR(x),m,v,HWIO_LPASS_AUDIO_CORE_CFG2_GDSCR_IN(x))
#define HWIO_LPASS_AUDIO_CORE_CFG2_GDSCR_GDSC_CLAMP_MEM_SW_BMSK                                                          0x10000
#define HWIO_LPASS_AUDIO_CORE_CFG2_GDSCR_GDSC_CLAMP_MEM_SW_SHFT                                                               16
#define HWIO_LPASS_AUDIO_CORE_CFG2_GDSCR_DLY_MEM_PWR_UP_BMSK                                                              0xf000
#define HWIO_LPASS_AUDIO_CORE_CFG2_GDSCR_DLY_MEM_PWR_UP_SHFT                                                                  12
#define HWIO_LPASS_AUDIO_CORE_CFG2_GDSCR_DLY_DEASSERT_CLAMP_MEM_BMSK                                                       0xf00
#define HWIO_LPASS_AUDIO_CORE_CFG2_GDSCR_DLY_DEASSERT_CLAMP_MEM_SHFT                                                           8
#define HWIO_LPASS_AUDIO_CORE_CFG2_GDSCR_DLY_ASSERT_CLAMP_MEM_BMSK                                                          0xf0
#define HWIO_LPASS_AUDIO_CORE_CFG2_GDSCR_DLY_ASSERT_CLAMP_MEM_SHFT                                                             4
#define HWIO_LPASS_AUDIO_CORE_CFG2_GDSCR_MEM_PWR_DWN_TIMEOUT_BMSK                                                            0xf
#define HWIO_LPASS_AUDIO_CORE_CFG2_GDSCR_MEM_PWR_DWN_TIMEOUT_SHFT                                                              0

#define HWIO_LPASS_AUDIO_CORE_GDSC_XO_CBCR_ADDR(x)                                                                    ((x) + 0xb00c)
#define HWIO_LPASS_AUDIO_CORE_GDSC_XO_CBCR_RMSK                                                                       0x80000003
#define HWIO_LPASS_AUDIO_CORE_GDSC_XO_CBCR_IN(x)            \
                in_dword(HWIO_LPASS_AUDIO_CORE_GDSC_XO_CBCR_ADDR(x))
#define HWIO_LPASS_AUDIO_CORE_GDSC_XO_CBCR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AUDIO_CORE_GDSC_XO_CBCR_ADDR(x), m)
#define HWIO_LPASS_AUDIO_CORE_GDSC_XO_CBCR_OUT(x, v)            \
                out_dword(HWIO_LPASS_AUDIO_CORE_GDSC_XO_CBCR_ADDR(x),v)
#define HWIO_LPASS_AUDIO_CORE_GDSC_XO_CBCR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AUDIO_CORE_GDSC_XO_CBCR_ADDR(x),m,v,HWIO_LPASS_AUDIO_CORE_GDSC_XO_CBCR_IN(x))
#define HWIO_LPASS_AUDIO_CORE_GDSC_XO_CBCR_CLK_OFF_BMSK                                                               0x80000000
#define HWIO_LPASS_AUDIO_CORE_GDSC_XO_CBCR_CLK_OFF_SHFT                                                                       31
#define HWIO_LPASS_AUDIO_CORE_GDSC_XO_CBCR_HW_CTL_BMSK                                                                       0x2
#define HWIO_LPASS_AUDIO_CORE_GDSC_XO_CBCR_HW_CTL_SHFT                                                                         1
#define HWIO_LPASS_AUDIO_CORE_GDSC_XO_CBCR_CLK_ENABLE_BMSK                                                                   0x1
#define HWIO_LPASS_AUDIO_CORE_GDSC_XO_CBCR_CLK_ENABLE_SHFT                                                                     0

#define HWIO_LPASS_AUDIO_CORE_GDS_HW_CTRL_ADDR(x)                                                                     ((x) + 0xb010)
#define HWIO_LPASS_AUDIO_CORE_GDS_HW_CTRL_RMSK                                                                        0xffffffff
#define HWIO_LPASS_AUDIO_CORE_GDS_HW_CTRL_IN(x)            \
                in_dword(HWIO_LPASS_AUDIO_CORE_GDS_HW_CTRL_ADDR(x))
#define HWIO_LPASS_AUDIO_CORE_GDS_HW_CTRL_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AUDIO_CORE_GDS_HW_CTRL_ADDR(x), m)
#define HWIO_LPASS_AUDIO_CORE_GDS_HW_CTRL_OUT(x, v)            \
                out_dword(HWIO_LPASS_AUDIO_CORE_GDS_HW_CTRL_ADDR(x),v)
#define HWIO_LPASS_AUDIO_CORE_GDS_HW_CTRL_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AUDIO_CORE_GDS_HW_CTRL_ADDR(x),m,v,HWIO_LPASS_AUDIO_CORE_GDS_HW_CTRL_IN(x))
#define HWIO_LPASS_AUDIO_CORE_GDS_HW_CTRL_POWER_ON_STATUS_BMSK                                                        0x80000000
#define HWIO_LPASS_AUDIO_CORE_GDS_HW_CTRL_POWER_ON_STATUS_SHFT                                                                31
#define HWIO_LPASS_AUDIO_CORE_GDS_HW_CTRL_HALT1_PWR_DOWN_ACK_STATUS_BMSK                                              0x78000000
#define HWIO_LPASS_AUDIO_CORE_GDS_HW_CTRL_HALT1_PWR_DOWN_ACK_STATUS_SHFT                                                      27
#define HWIO_LPASS_AUDIO_CORE_GDS_HW_CTRL_HALT1_PWR_UP_ACK_STATUS_BMSK                                                 0x7800000
#define HWIO_LPASS_AUDIO_CORE_GDS_HW_CTRL_HALT1_PWR_UP_ACK_STATUS_SHFT                                                        23
#define HWIO_LPASS_AUDIO_CORE_GDS_HW_CTRL_HALT2_PWR_DOWN_ACK_STATUS_BMSK                                                0x780000
#define HWIO_LPASS_AUDIO_CORE_GDS_HW_CTRL_HALT2_PWR_DOWN_ACK_STATUS_SHFT                                                      19
#define HWIO_LPASS_AUDIO_CORE_GDS_HW_CTRL_HALT2_PWR_UP_ACK_STATUS_BMSK                                                   0x78000
#define HWIO_LPASS_AUDIO_CORE_GDS_HW_CTRL_HALT2_PWR_UP_ACK_STATUS_SHFT                                                        15
#define HWIO_LPASS_AUDIO_CORE_GDS_HW_CTRL_COLLAPSE_OUT_BMSK                                                               0x4000
#define HWIO_LPASS_AUDIO_CORE_GDS_HW_CTRL_COLLAPSE_OUT_SHFT                                                                   14
#define HWIO_LPASS_AUDIO_CORE_GDS_HW_CTRL_RESERVE_BITS13_BMSK                                                             0x2000
#define HWIO_LPASS_AUDIO_CORE_GDS_HW_CTRL_RESERVE_BITS13_SHFT                                                                 13
#define HWIO_LPASS_AUDIO_CORE_GDS_HW_CTRL_HALT_ACK_TIME_OUT_BMSK                                                          0x1fe0
#define HWIO_LPASS_AUDIO_CORE_GDS_HW_CTRL_HALT_ACK_TIME_OUT_SHFT                                                               5
#define HWIO_LPASS_AUDIO_CORE_GDS_HW_CTRL_GDS_HW_STATE_BMSK                                                                 0x1e
#define HWIO_LPASS_AUDIO_CORE_GDS_HW_CTRL_GDS_HW_STATE_SHFT                                                                    1
#define HWIO_LPASS_AUDIO_CORE_GDS_HW_CTRL_SW_OVERRIDE_BMSK                                                                   0x1
#define HWIO_LPASS_AUDIO_CORE_GDS_HW_CTRL_SW_OVERRIDE_SHFT                                                                     0

#define HWIO_LPASS_AUDIO_CORE_GDS_HW_DVM_CTRL_ADDR(x)                                                                 ((x) + 0xb014)
#define HWIO_LPASS_AUDIO_CORE_GDS_HW_DVM_CTRL_RMSK                                                                    0xc0000001
#define HWIO_LPASS_AUDIO_CORE_GDS_HW_DVM_CTRL_IN(x)            \
                in_dword(HWIO_LPASS_AUDIO_CORE_GDS_HW_DVM_CTRL_ADDR(x))
#define HWIO_LPASS_AUDIO_CORE_GDS_HW_DVM_CTRL_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AUDIO_CORE_GDS_HW_DVM_CTRL_ADDR(x), m)
#define HWIO_LPASS_AUDIO_CORE_GDS_HW_DVM_CTRL_OUT(x, v)            \
                out_dword(HWIO_LPASS_AUDIO_CORE_GDS_HW_DVM_CTRL_ADDR(x),v)
#define HWIO_LPASS_AUDIO_CORE_GDS_HW_DVM_CTRL_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AUDIO_CORE_GDS_HW_DVM_CTRL_ADDR(x),m,v,HWIO_LPASS_AUDIO_CORE_GDS_HW_DVM_CTRL_IN(x))
#define HWIO_LPASS_AUDIO_CORE_GDS_HW_DVM_CTRL_DVM_HALT1_PWR_DOWN_ACK_STATUS_BMSK                                      0x80000000
#define HWIO_LPASS_AUDIO_CORE_GDS_HW_DVM_CTRL_DVM_HALT1_PWR_DOWN_ACK_STATUS_SHFT                                              31
#define HWIO_LPASS_AUDIO_CORE_GDS_HW_DVM_CTRL_DVM_HALT1_PWR_UP_ACK_STATUS_BMSK                                        0x40000000
#define HWIO_LPASS_AUDIO_CORE_GDS_HW_DVM_CTRL_DVM_HALT1_PWR_UP_ACK_STATUS_SHFT                                                30
#define HWIO_LPASS_AUDIO_CORE_GDS_HW_DVM_CTRL_DVM_HALT1_REQ_SW_BMSK                                                          0x1
#define HWIO_LPASS_AUDIO_CORE_GDS_HW_DVM_CTRL_DVM_HALT1_REQ_SW_SHFT                                                            0

#define HWIO_LPASS_LPAIF_SPKR_CMD_RCGR_ADDR(x)                                                                        ((x) + 0xf000)
#define HWIO_LPASS_LPAIF_SPKR_CMD_RCGR_RMSK                                                                           0x800000f3
#define HWIO_LPASS_LPAIF_SPKR_CMD_RCGR_IN(x)            \
                in_dword(HWIO_LPASS_LPAIF_SPKR_CMD_RCGR_ADDR(x))
#define HWIO_LPASS_LPAIF_SPKR_CMD_RCGR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPAIF_SPKR_CMD_RCGR_ADDR(x), m)
#define HWIO_LPASS_LPAIF_SPKR_CMD_RCGR_OUT(x, v)            \
                out_dword(HWIO_LPASS_LPAIF_SPKR_CMD_RCGR_ADDR(x),v)
#define HWIO_LPASS_LPAIF_SPKR_CMD_RCGR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_LPAIF_SPKR_CMD_RCGR_ADDR(x),m,v,HWIO_LPASS_LPAIF_SPKR_CMD_RCGR_IN(x))
#define HWIO_LPASS_LPAIF_SPKR_CMD_RCGR_ROOT_OFF_BMSK                                                                  0x80000000
#define HWIO_LPASS_LPAIF_SPKR_CMD_RCGR_ROOT_OFF_SHFT                                                                          31
#define HWIO_LPASS_LPAIF_SPKR_CMD_RCGR_DIRTY_D_BMSK                                                                         0x80
#define HWIO_LPASS_LPAIF_SPKR_CMD_RCGR_DIRTY_D_SHFT                                                                            7
#define HWIO_LPASS_LPAIF_SPKR_CMD_RCGR_DIRTY_N_BMSK                                                                         0x40
#define HWIO_LPASS_LPAIF_SPKR_CMD_RCGR_DIRTY_N_SHFT                                                                            6
#define HWIO_LPASS_LPAIF_SPKR_CMD_RCGR_DIRTY_M_BMSK                                                                         0x20
#define HWIO_LPASS_LPAIF_SPKR_CMD_RCGR_DIRTY_M_SHFT                                                                            5
#define HWIO_LPASS_LPAIF_SPKR_CMD_RCGR_DIRTY_CFG_RCGR_BMSK                                                                  0x10
#define HWIO_LPASS_LPAIF_SPKR_CMD_RCGR_DIRTY_CFG_RCGR_SHFT                                                                     4
#define HWIO_LPASS_LPAIF_SPKR_CMD_RCGR_ROOT_EN_BMSK                                                                          0x2
#define HWIO_LPASS_LPAIF_SPKR_CMD_RCGR_ROOT_EN_SHFT                                                                            1
#define HWIO_LPASS_LPAIF_SPKR_CMD_RCGR_UPDATE_BMSK                                                                           0x1
#define HWIO_LPASS_LPAIF_SPKR_CMD_RCGR_UPDATE_SHFT                                                                             0

#define HWIO_LPASS_LPAIF_SPKR_CFG_RCGR_ADDR(x)                                                                        ((x) + 0xf004)
#define HWIO_LPASS_LPAIF_SPKR_CFG_RCGR_RMSK                                                                               0x771f
#define HWIO_LPASS_LPAIF_SPKR_CFG_RCGR_IN(x)            \
                in_dword(HWIO_LPASS_LPAIF_SPKR_CFG_RCGR_ADDR(x))
#define HWIO_LPASS_LPAIF_SPKR_CFG_RCGR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPAIF_SPKR_CFG_RCGR_ADDR(x), m)
#define HWIO_LPASS_LPAIF_SPKR_CFG_RCGR_OUT(x, v)            \
                out_dword(HWIO_LPASS_LPAIF_SPKR_CFG_RCGR_ADDR(x),v)
#define HWIO_LPASS_LPAIF_SPKR_CFG_RCGR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_LPAIF_SPKR_CFG_RCGR_ADDR(x),m,v,HWIO_LPASS_LPAIF_SPKR_CFG_RCGR_IN(x))
#define HWIO_LPASS_LPAIF_SPKR_CFG_RCGR_ALT_SRC_SEL_BMSK                                                                   0x4000
#define HWIO_LPASS_LPAIF_SPKR_CFG_RCGR_ALT_SRC_SEL_SHFT                                                                       14
#define HWIO_LPASS_LPAIF_SPKR_CFG_RCGR_MODE_BMSK                                                                          0x3000
#define HWIO_LPASS_LPAIF_SPKR_CFG_RCGR_MODE_SHFT                                                                              12
#define HWIO_LPASS_LPAIF_SPKR_CFG_RCGR_SRC_SEL_BMSK                                                                        0x700
#define HWIO_LPASS_LPAIF_SPKR_CFG_RCGR_SRC_SEL_SHFT                                                                            8
#define HWIO_LPASS_LPAIF_SPKR_CFG_RCGR_SRC_DIV_BMSK                                                                         0x1f
#define HWIO_LPASS_LPAIF_SPKR_CFG_RCGR_SRC_DIV_SHFT                                                                            0

#define HWIO_LPASS_LPAIF_SPKR_M_ADDR(x)                                                                               ((x) + 0xf008)
#define HWIO_LPASS_LPAIF_SPKR_M_RMSK                                                                                        0xff
#define HWIO_LPASS_LPAIF_SPKR_M_IN(x)            \
                in_dword(HWIO_LPASS_LPAIF_SPKR_M_ADDR(x))
#define HWIO_LPASS_LPAIF_SPKR_M_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPAIF_SPKR_M_ADDR(x), m)
#define HWIO_LPASS_LPAIF_SPKR_M_OUT(x, v)            \
                out_dword(HWIO_LPASS_LPAIF_SPKR_M_ADDR(x),v)
#define HWIO_LPASS_LPAIF_SPKR_M_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_LPAIF_SPKR_M_ADDR(x),m,v,HWIO_LPASS_LPAIF_SPKR_M_IN(x))
#define HWIO_LPASS_LPAIF_SPKR_M_M_BMSK                                                                                      0xff
#define HWIO_LPASS_LPAIF_SPKR_M_M_SHFT                                                                                         0

#define HWIO_LPASS_LPAIF_SPKR_N_ADDR(x)                                                                               ((x) + 0xf00c)
#define HWIO_LPASS_LPAIF_SPKR_N_RMSK                                                                                        0xff
#define HWIO_LPASS_LPAIF_SPKR_N_IN(x)            \
                in_dword(HWIO_LPASS_LPAIF_SPKR_N_ADDR(x))
#define HWIO_LPASS_LPAIF_SPKR_N_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPAIF_SPKR_N_ADDR(x), m)
#define HWIO_LPASS_LPAIF_SPKR_N_OUT(x, v)            \
                out_dword(HWIO_LPASS_LPAIF_SPKR_N_ADDR(x),v)
#define HWIO_LPASS_LPAIF_SPKR_N_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_LPAIF_SPKR_N_ADDR(x),m,v,HWIO_LPASS_LPAIF_SPKR_N_IN(x))
#define HWIO_LPASS_LPAIF_SPKR_N_NOT_N_MINUS_M_BMSK                                                                          0xff
#define HWIO_LPASS_LPAIF_SPKR_N_NOT_N_MINUS_M_SHFT                                                                             0

#define HWIO_LPASS_LPAIF_SPKR_D_ADDR(x)                                                                               ((x) + 0xf010)
#define HWIO_LPASS_LPAIF_SPKR_D_RMSK                                                                                        0xff
#define HWIO_LPASS_LPAIF_SPKR_D_IN(x)            \
                in_dword(HWIO_LPASS_LPAIF_SPKR_D_ADDR(x))
#define HWIO_LPASS_LPAIF_SPKR_D_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPAIF_SPKR_D_ADDR(x), m)
#define HWIO_LPASS_LPAIF_SPKR_D_OUT(x, v)            \
                out_dword(HWIO_LPASS_LPAIF_SPKR_D_ADDR(x),v)
#define HWIO_LPASS_LPAIF_SPKR_D_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_LPAIF_SPKR_D_ADDR(x),m,v,HWIO_LPASS_LPAIF_SPKR_D_IN(x))
#define HWIO_LPASS_LPAIF_SPKR_D_NOT_2D_BMSK                                                                                 0xff
#define HWIO_LPASS_LPAIF_SPKR_D_NOT_2D_SHFT                                                                                    0

#define HWIO_LPASS_AUDIO_CORE_LPAIF_CODEC_SPKR_OSR_CBCR_ADDR(x)                                                       ((x) + 0xf014)
#define HWIO_LPASS_AUDIO_CORE_LPAIF_CODEC_SPKR_OSR_CBCR_RMSK                                                          0x80000003
#define HWIO_LPASS_AUDIO_CORE_LPAIF_CODEC_SPKR_OSR_CBCR_IN(x)            \
                in_dword(HWIO_LPASS_AUDIO_CORE_LPAIF_CODEC_SPKR_OSR_CBCR_ADDR(x))
#define HWIO_LPASS_AUDIO_CORE_LPAIF_CODEC_SPKR_OSR_CBCR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AUDIO_CORE_LPAIF_CODEC_SPKR_OSR_CBCR_ADDR(x), m)
#define HWIO_LPASS_AUDIO_CORE_LPAIF_CODEC_SPKR_OSR_CBCR_OUT(x, v)            \
                out_dword(HWIO_LPASS_AUDIO_CORE_LPAIF_CODEC_SPKR_OSR_CBCR_ADDR(x),v)
#define HWIO_LPASS_AUDIO_CORE_LPAIF_CODEC_SPKR_OSR_CBCR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AUDIO_CORE_LPAIF_CODEC_SPKR_OSR_CBCR_ADDR(x),m,v,HWIO_LPASS_AUDIO_CORE_LPAIF_CODEC_SPKR_OSR_CBCR_IN(x))
#define HWIO_LPASS_AUDIO_CORE_LPAIF_CODEC_SPKR_OSR_CBCR_CLK_OFF_BMSK                                                  0x80000000
#define HWIO_LPASS_AUDIO_CORE_LPAIF_CODEC_SPKR_OSR_CBCR_CLK_OFF_SHFT                                                          31
#define HWIO_LPASS_AUDIO_CORE_LPAIF_CODEC_SPKR_OSR_CBCR_HW_CTL_BMSK                                                          0x2
#define HWIO_LPASS_AUDIO_CORE_LPAIF_CODEC_SPKR_OSR_CBCR_HW_CTL_SHFT                                                            1
#define HWIO_LPASS_AUDIO_CORE_LPAIF_CODEC_SPKR_OSR_CBCR_CLK_ENABLE_BMSK                                                      0x1
#define HWIO_LPASS_AUDIO_CORE_LPAIF_CODEC_SPKR_OSR_CBCR_CLK_ENABLE_SHFT                                                        0

#define HWIO_LPASS_AUDIO_CORE_LPAIF_CODEC_SPKR_IBIT_CBCR_ADDR(x)                                                      ((x) + 0xf018)
#define HWIO_LPASS_AUDIO_CORE_LPAIF_CODEC_SPKR_IBIT_CBCR_RMSK                                                         0x81ff0003
#define HWIO_LPASS_AUDIO_CORE_LPAIF_CODEC_SPKR_IBIT_CBCR_IN(x)            \
                in_dword(HWIO_LPASS_AUDIO_CORE_LPAIF_CODEC_SPKR_IBIT_CBCR_ADDR(x))
#define HWIO_LPASS_AUDIO_CORE_LPAIF_CODEC_SPKR_IBIT_CBCR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AUDIO_CORE_LPAIF_CODEC_SPKR_IBIT_CBCR_ADDR(x), m)
#define HWIO_LPASS_AUDIO_CORE_LPAIF_CODEC_SPKR_IBIT_CBCR_OUT(x, v)            \
                out_dword(HWIO_LPASS_AUDIO_CORE_LPAIF_CODEC_SPKR_IBIT_CBCR_ADDR(x),v)
#define HWIO_LPASS_AUDIO_CORE_LPAIF_CODEC_SPKR_IBIT_CBCR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AUDIO_CORE_LPAIF_CODEC_SPKR_IBIT_CBCR_ADDR(x),m,v,HWIO_LPASS_AUDIO_CORE_LPAIF_CODEC_SPKR_IBIT_CBCR_IN(x))
#define HWIO_LPASS_AUDIO_CORE_LPAIF_CODEC_SPKR_IBIT_CBCR_CLK_OFF_BMSK                                                 0x80000000
#define HWIO_LPASS_AUDIO_CORE_LPAIF_CODEC_SPKR_IBIT_CBCR_CLK_OFF_SHFT                                                         31
#define HWIO_LPASS_AUDIO_CORE_LPAIF_CODEC_SPKR_IBIT_CBCR_CLK_DIV_BMSK                                                  0x1ff0000
#define HWIO_LPASS_AUDIO_CORE_LPAIF_CODEC_SPKR_IBIT_CBCR_CLK_DIV_SHFT                                                         16
#define HWIO_LPASS_AUDIO_CORE_LPAIF_CODEC_SPKR_IBIT_CBCR_HW_CTL_BMSK                                                         0x2
#define HWIO_LPASS_AUDIO_CORE_LPAIF_CODEC_SPKR_IBIT_CBCR_HW_CTL_SHFT                                                           1
#define HWIO_LPASS_AUDIO_CORE_LPAIF_CODEC_SPKR_IBIT_CBCR_CLK_ENABLE_BMSK                                                     0x1
#define HWIO_LPASS_AUDIO_CORE_LPAIF_CODEC_SPKR_IBIT_CBCR_CLK_ENABLE_SHFT                                                       0

#define HWIO_LPASS_AUDIO_CORE_LPAIF_CODEC_SPKR_EBIT_CBCR_ADDR(x)                                                      ((x) + 0xf01c)
#define HWIO_LPASS_AUDIO_CORE_LPAIF_CODEC_SPKR_EBIT_CBCR_RMSK                                                         0x80000003
#define HWIO_LPASS_AUDIO_CORE_LPAIF_CODEC_SPKR_EBIT_CBCR_IN(x)            \
                in_dword(HWIO_LPASS_AUDIO_CORE_LPAIF_CODEC_SPKR_EBIT_CBCR_ADDR(x))
#define HWIO_LPASS_AUDIO_CORE_LPAIF_CODEC_SPKR_EBIT_CBCR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AUDIO_CORE_LPAIF_CODEC_SPKR_EBIT_CBCR_ADDR(x), m)
#define HWIO_LPASS_AUDIO_CORE_LPAIF_CODEC_SPKR_EBIT_CBCR_OUT(x, v)            \
                out_dword(HWIO_LPASS_AUDIO_CORE_LPAIF_CODEC_SPKR_EBIT_CBCR_ADDR(x),v)
#define HWIO_LPASS_AUDIO_CORE_LPAIF_CODEC_SPKR_EBIT_CBCR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AUDIO_CORE_LPAIF_CODEC_SPKR_EBIT_CBCR_ADDR(x),m,v,HWIO_LPASS_AUDIO_CORE_LPAIF_CODEC_SPKR_EBIT_CBCR_IN(x))
#define HWIO_LPASS_AUDIO_CORE_LPAIF_CODEC_SPKR_EBIT_CBCR_CLK_OFF_BMSK                                                 0x80000000
#define HWIO_LPASS_AUDIO_CORE_LPAIF_CODEC_SPKR_EBIT_CBCR_CLK_OFF_SHFT                                                         31
#define HWIO_LPASS_AUDIO_CORE_LPAIF_CODEC_SPKR_EBIT_CBCR_HW_CTL_BMSK                                                         0x2
#define HWIO_LPASS_AUDIO_CORE_LPAIF_CODEC_SPKR_EBIT_CBCR_HW_CTL_SHFT                                                           1
#define HWIO_LPASS_AUDIO_CORE_LPAIF_CODEC_SPKR_EBIT_CBCR_CLK_ENABLE_BMSK                                                     0x1
#define HWIO_LPASS_AUDIO_CORE_LPAIF_CODEC_SPKR_EBIT_CBCR_CLK_ENABLE_SHFT                                                       0

#define HWIO_LPASS_AUDIO_CORE_LPAIF_CODEC_SPKR_EXT_CLK_DETECT_ADDR(x)                                                 ((x) + 0xf020)
#define HWIO_LPASS_AUDIO_CORE_LPAIF_CODEC_SPKR_EXT_CLK_DETECT_RMSK                                                    0x80007f13
#define HWIO_LPASS_AUDIO_CORE_LPAIF_CODEC_SPKR_EXT_CLK_DETECT_IN(x)            \
                in_dword(HWIO_LPASS_AUDIO_CORE_LPAIF_CODEC_SPKR_EXT_CLK_DETECT_ADDR(x))
#define HWIO_LPASS_AUDIO_CORE_LPAIF_CODEC_SPKR_EXT_CLK_DETECT_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AUDIO_CORE_LPAIF_CODEC_SPKR_EXT_CLK_DETECT_ADDR(x), m)
#define HWIO_LPASS_AUDIO_CORE_LPAIF_CODEC_SPKR_EXT_CLK_DETECT_OUT(x, v)            \
                out_dword(HWIO_LPASS_AUDIO_CORE_LPAIF_CODEC_SPKR_EXT_CLK_DETECT_ADDR(x),v)
#define HWIO_LPASS_AUDIO_CORE_LPAIF_CODEC_SPKR_EXT_CLK_DETECT_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AUDIO_CORE_LPAIF_CODEC_SPKR_EXT_CLK_DETECT_ADDR(x),m,v,HWIO_LPASS_AUDIO_CORE_LPAIF_CODEC_SPKR_EXT_CLK_DETECT_IN(x))
#define HWIO_LPASS_AUDIO_CORE_LPAIF_CODEC_SPKR_EXT_CLK_DETECT_CLK_INACTIVE_IRQ_BMSK                                   0x80000000
#define HWIO_LPASS_AUDIO_CORE_LPAIF_CODEC_SPKR_EXT_CLK_DETECT_CLK_INACTIVE_IRQ_SHFT                                           31
#define HWIO_LPASS_AUDIO_CORE_LPAIF_CODEC_SPKR_EXT_CLK_DETECT_CLK_COUNTER_VALUE_BMSK                                      0x7f00
#define HWIO_LPASS_AUDIO_CORE_LPAIF_CODEC_SPKR_EXT_CLK_DETECT_CLK_COUNTER_VALUE_SHFT                                           8
#define HWIO_LPASS_AUDIO_CORE_LPAIF_CODEC_SPKR_EXT_CLK_DETECT_IRQ_CLEAR_BMSK                                                0x10
#define HWIO_LPASS_AUDIO_CORE_LPAIF_CODEC_SPKR_EXT_CLK_DETECT_IRQ_CLEAR_SHFT                                                   4
#define HWIO_LPASS_AUDIO_CORE_LPAIF_CODEC_SPKR_EXT_CLK_DETECT_CLK_COUNTER_EN_BMSK                                            0x2
#define HWIO_LPASS_AUDIO_CORE_LPAIF_CODEC_SPKR_EXT_CLK_DETECT_CLK_COUNTER_EN_SHFT                                              1
#define HWIO_LPASS_AUDIO_CORE_LPAIF_CODEC_SPKR_EXT_CLK_DETECT_IRQ_EN_BMSK                                                    0x1
#define HWIO_LPASS_AUDIO_CORE_LPAIF_CODEC_SPKR_EXT_CLK_DETECT_IRQ_EN_SHFT                                                      0

#define HWIO_LPASS_LPAIF_PRI_CMD_RCGR_ADDR(x)                                                                         ((x) + 0x10000)
#define HWIO_LPASS_LPAIF_PRI_CMD_RCGR_RMSK                                                                            0x800000f3
#define HWIO_LPASS_LPAIF_PRI_CMD_RCGR_IN(x)            \
                in_dword(HWIO_LPASS_LPAIF_PRI_CMD_RCGR_ADDR(x))
#define HWIO_LPASS_LPAIF_PRI_CMD_RCGR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPAIF_PRI_CMD_RCGR_ADDR(x), m)
#define HWIO_LPASS_LPAIF_PRI_CMD_RCGR_OUT(x, v)            \
                out_dword(HWIO_LPASS_LPAIF_PRI_CMD_RCGR_ADDR(x),v)
#define HWIO_LPASS_LPAIF_PRI_CMD_RCGR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_LPAIF_PRI_CMD_RCGR_ADDR(x),m,v,HWIO_LPASS_LPAIF_PRI_CMD_RCGR_IN(x))
#define HWIO_LPASS_LPAIF_PRI_CMD_RCGR_ROOT_OFF_BMSK                                                                   0x80000000
#define HWIO_LPASS_LPAIF_PRI_CMD_RCGR_ROOT_OFF_SHFT                                                                           31
#define HWIO_LPASS_LPAIF_PRI_CMD_RCGR_DIRTY_D_BMSK                                                                          0x80
#define HWIO_LPASS_LPAIF_PRI_CMD_RCGR_DIRTY_D_SHFT                                                                             7
#define HWIO_LPASS_LPAIF_PRI_CMD_RCGR_DIRTY_N_BMSK                                                                          0x40
#define HWIO_LPASS_LPAIF_PRI_CMD_RCGR_DIRTY_N_SHFT                                                                             6
#define HWIO_LPASS_LPAIF_PRI_CMD_RCGR_DIRTY_M_BMSK                                                                          0x20
#define HWIO_LPASS_LPAIF_PRI_CMD_RCGR_DIRTY_M_SHFT                                                                             5
#define HWIO_LPASS_LPAIF_PRI_CMD_RCGR_DIRTY_CFG_RCGR_BMSK                                                                   0x10
#define HWIO_LPASS_LPAIF_PRI_CMD_RCGR_DIRTY_CFG_RCGR_SHFT                                                                      4
#define HWIO_LPASS_LPAIF_PRI_CMD_RCGR_ROOT_EN_BMSK                                                                           0x2
#define HWIO_LPASS_LPAIF_PRI_CMD_RCGR_ROOT_EN_SHFT                                                                             1
#define HWIO_LPASS_LPAIF_PRI_CMD_RCGR_UPDATE_BMSK                                                                            0x1
#define HWIO_LPASS_LPAIF_PRI_CMD_RCGR_UPDATE_SHFT                                                                              0

#define HWIO_LPASS_LPAIF_PRI_CFG_RCGR_ADDR(x)                                                                         ((x) + 0x10004)
#define HWIO_LPASS_LPAIF_PRI_CFG_RCGR_RMSK                                                                                0x771f
#define HWIO_LPASS_LPAIF_PRI_CFG_RCGR_IN(x)            \
                in_dword(HWIO_LPASS_LPAIF_PRI_CFG_RCGR_ADDR(x))
#define HWIO_LPASS_LPAIF_PRI_CFG_RCGR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPAIF_PRI_CFG_RCGR_ADDR(x), m)
#define HWIO_LPASS_LPAIF_PRI_CFG_RCGR_OUT(x, v)            \
                out_dword(HWIO_LPASS_LPAIF_PRI_CFG_RCGR_ADDR(x),v)
#define HWIO_LPASS_LPAIF_PRI_CFG_RCGR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_LPAIF_PRI_CFG_RCGR_ADDR(x),m,v,HWIO_LPASS_LPAIF_PRI_CFG_RCGR_IN(x))
#define HWIO_LPASS_LPAIF_PRI_CFG_RCGR_ALT_SRC_SEL_BMSK                                                                    0x4000
#define HWIO_LPASS_LPAIF_PRI_CFG_RCGR_ALT_SRC_SEL_SHFT                                                                        14
#define HWIO_LPASS_LPAIF_PRI_CFG_RCGR_MODE_BMSK                                                                           0x3000
#define HWIO_LPASS_LPAIF_PRI_CFG_RCGR_MODE_SHFT                                                                               12
#define HWIO_LPASS_LPAIF_PRI_CFG_RCGR_SRC_SEL_BMSK                                                                         0x700
#define HWIO_LPASS_LPAIF_PRI_CFG_RCGR_SRC_SEL_SHFT                                                                             8
#define HWIO_LPASS_LPAIF_PRI_CFG_RCGR_SRC_DIV_BMSK                                                                          0x1f
#define HWIO_LPASS_LPAIF_PRI_CFG_RCGR_SRC_DIV_SHFT                                                                             0

#define HWIO_LPASS_LPAIF_PRI_M_ADDR(x)                                                                                ((x) + 0x10008)
#define HWIO_LPASS_LPAIF_PRI_M_RMSK                                                                                       0xffff
#define HWIO_LPASS_LPAIF_PRI_M_IN(x)            \
                in_dword(HWIO_LPASS_LPAIF_PRI_M_ADDR(x))
#define HWIO_LPASS_LPAIF_PRI_M_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPAIF_PRI_M_ADDR(x), m)
#define HWIO_LPASS_LPAIF_PRI_M_OUT(x, v)            \
                out_dword(HWIO_LPASS_LPAIF_PRI_M_ADDR(x),v)
#define HWIO_LPASS_LPAIF_PRI_M_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_LPAIF_PRI_M_ADDR(x),m,v,HWIO_LPASS_LPAIF_PRI_M_IN(x))
#define HWIO_LPASS_LPAIF_PRI_M_M_BMSK                                                                                     0xffff
#define HWIO_LPASS_LPAIF_PRI_M_M_SHFT                                                                                          0

#define HWIO_LPASS_LPAIF_PRI_N_ADDR(x)                                                                                ((x) + 0x1000c)
#define HWIO_LPASS_LPAIF_PRI_N_RMSK                                                                                       0xffff
#define HWIO_LPASS_LPAIF_PRI_N_IN(x)            \
                in_dword(HWIO_LPASS_LPAIF_PRI_N_ADDR(x))
#define HWIO_LPASS_LPAIF_PRI_N_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPAIF_PRI_N_ADDR(x), m)
#define HWIO_LPASS_LPAIF_PRI_N_OUT(x, v)            \
                out_dword(HWIO_LPASS_LPAIF_PRI_N_ADDR(x),v)
#define HWIO_LPASS_LPAIF_PRI_N_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_LPAIF_PRI_N_ADDR(x),m,v,HWIO_LPASS_LPAIF_PRI_N_IN(x))
#define HWIO_LPASS_LPAIF_PRI_N_NOT_N_MINUS_M_BMSK                                                                         0xffff
#define HWIO_LPASS_LPAIF_PRI_N_NOT_N_MINUS_M_SHFT                                                                              0

#define HWIO_LPASS_LPAIF_PRI_D_ADDR(x)                                                                                ((x) + 0x10010)
#define HWIO_LPASS_LPAIF_PRI_D_RMSK                                                                                       0xffff
#define HWIO_LPASS_LPAIF_PRI_D_IN(x)            \
                in_dword(HWIO_LPASS_LPAIF_PRI_D_ADDR(x))
#define HWIO_LPASS_LPAIF_PRI_D_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPAIF_PRI_D_ADDR(x), m)
#define HWIO_LPASS_LPAIF_PRI_D_OUT(x, v)            \
                out_dword(HWIO_LPASS_LPAIF_PRI_D_ADDR(x),v)
#define HWIO_LPASS_LPAIF_PRI_D_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_LPAIF_PRI_D_ADDR(x),m,v,HWIO_LPASS_LPAIF_PRI_D_IN(x))
#define HWIO_LPASS_LPAIF_PRI_D_NOT_2D_BMSK                                                                                0xffff
#define HWIO_LPASS_LPAIF_PRI_D_NOT_2D_SHFT                                                                                     0

#define HWIO_LPASS_AUDIO_CORE_LPAIF_PRI_IBIT_CBCR_ADDR(x)                                                             ((x) + 0x10018)
#define HWIO_LPASS_AUDIO_CORE_LPAIF_PRI_IBIT_CBCR_RMSK                                                                0x81ff0003
#define HWIO_LPASS_AUDIO_CORE_LPAIF_PRI_IBIT_CBCR_IN(x)            \
                in_dword(HWIO_LPASS_AUDIO_CORE_LPAIF_PRI_IBIT_CBCR_ADDR(x))
#define HWIO_LPASS_AUDIO_CORE_LPAIF_PRI_IBIT_CBCR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AUDIO_CORE_LPAIF_PRI_IBIT_CBCR_ADDR(x), m)
#define HWIO_LPASS_AUDIO_CORE_LPAIF_PRI_IBIT_CBCR_OUT(x, v)            \
                out_dword(HWIO_LPASS_AUDIO_CORE_LPAIF_PRI_IBIT_CBCR_ADDR(x),v)
#define HWIO_LPASS_AUDIO_CORE_LPAIF_PRI_IBIT_CBCR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AUDIO_CORE_LPAIF_PRI_IBIT_CBCR_ADDR(x),m,v,HWIO_LPASS_AUDIO_CORE_LPAIF_PRI_IBIT_CBCR_IN(x))
#define HWIO_LPASS_AUDIO_CORE_LPAIF_PRI_IBIT_CBCR_CLK_OFF_BMSK                                                        0x80000000
#define HWIO_LPASS_AUDIO_CORE_LPAIF_PRI_IBIT_CBCR_CLK_OFF_SHFT                                                                31
#define HWIO_LPASS_AUDIO_CORE_LPAIF_PRI_IBIT_CBCR_CLK_DIV_BMSK                                                         0x1ff0000
#define HWIO_LPASS_AUDIO_CORE_LPAIF_PRI_IBIT_CBCR_CLK_DIV_SHFT                                                                16
#define HWIO_LPASS_AUDIO_CORE_LPAIF_PRI_IBIT_CBCR_HW_CTL_BMSK                                                                0x2
#define HWIO_LPASS_AUDIO_CORE_LPAIF_PRI_IBIT_CBCR_HW_CTL_SHFT                                                                  1
#define HWIO_LPASS_AUDIO_CORE_LPAIF_PRI_IBIT_CBCR_CLK_ENABLE_BMSK                                                            0x1
#define HWIO_LPASS_AUDIO_CORE_LPAIF_PRI_IBIT_CBCR_CLK_ENABLE_SHFT                                                              0

#define HWIO_LPASS_AUDIO_CORE_LPAIF_PRI_EBIT_CBCR_ADDR(x)                                                             ((x) + 0x1001c)
#define HWIO_LPASS_AUDIO_CORE_LPAIF_PRI_EBIT_CBCR_RMSK                                                                0x80000003
#define HWIO_LPASS_AUDIO_CORE_LPAIF_PRI_EBIT_CBCR_IN(x)            \
                in_dword(HWIO_LPASS_AUDIO_CORE_LPAIF_PRI_EBIT_CBCR_ADDR(x))
#define HWIO_LPASS_AUDIO_CORE_LPAIF_PRI_EBIT_CBCR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AUDIO_CORE_LPAIF_PRI_EBIT_CBCR_ADDR(x), m)
#define HWIO_LPASS_AUDIO_CORE_LPAIF_PRI_EBIT_CBCR_OUT(x, v)            \
                out_dword(HWIO_LPASS_AUDIO_CORE_LPAIF_PRI_EBIT_CBCR_ADDR(x),v)
#define HWIO_LPASS_AUDIO_CORE_LPAIF_PRI_EBIT_CBCR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AUDIO_CORE_LPAIF_PRI_EBIT_CBCR_ADDR(x),m,v,HWIO_LPASS_AUDIO_CORE_LPAIF_PRI_EBIT_CBCR_IN(x))
#define HWIO_LPASS_AUDIO_CORE_LPAIF_PRI_EBIT_CBCR_CLK_OFF_BMSK                                                        0x80000000
#define HWIO_LPASS_AUDIO_CORE_LPAIF_PRI_EBIT_CBCR_CLK_OFF_SHFT                                                                31
#define HWIO_LPASS_AUDIO_CORE_LPAIF_PRI_EBIT_CBCR_HW_CTL_BMSK                                                                0x2
#define HWIO_LPASS_AUDIO_CORE_LPAIF_PRI_EBIT_CBCR_HW_CTL_SHFT                                                                  1
#define HWIO_LPASS_AUDIO_CORE_LPAIF_PRI_EBIT_CBCR_CLK_ENABLE_BMSK                                                            0x1
#define HWIO_LPASS_AUDIO_CORE_LPAIF_PRI_EBIT_CBCR_CLK_ENABLE_SHFT                                                              0

#define HWIO_LPASS_AUDIO_CORE_LPAIF_PRI_CLK_INV_ADDR(x)                                                               ((x) + 0x10020)
#define HWIO_LPASS_AUDIO_CORE_LPAIF_PRI_CLK_INV_RMSK                                                                         0x3
#define HWIO_LPASS_AUDIO_CORE_LPAIF_PRI_CLK_INV_IN(x)            \
                in_dword(HWIO_LPASS_AUDIO_CORE_LPAIF_PRI_CLK_INV_ADDR(x))
#define HWIO_LPASS_AUDIO_CORE_LPAIF_PRI_CLK_INV_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AUDIO_CORE_LPAIF_PRI_CLK_INV_ADDR(x), m)
#define HWIO_LPASS_AUDIO_CORE_LPAIF_PRI_CLK_INV_OUT(x, v)            \
                out_dword(HWIO_LPASS_AUDIO_CORE_LPAIF_PRI_CLK_INV_ADDR(x),v)
#define HWIO_LPASS_AUDIO_CORE_LPAIF_PRI_CLK_INV_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AUDIO_CORE_LPAIF_PRI_CLK_INV_ADDR(x),m,v,HWIO_LPASS_AUDIO_CORE_LPAIF_PRI_CLK_INV_IN(x))
#define HWIO_LPASS_AUDIO_CORE_LPAIF_PRI_CLK_INV_INV_EXT_CLK_BMSK                                                             0x2
#define HWIO_LPASS_AUDIO_CORE_LPAIF_PRI_CLK_INV_INV_EXT_CLK_SHFT                                                               1
#define HWIO_LPASS_AUDIO_CORE_LPAIF_PRI_CLK_INV_INV_INT_CLK_BMSK                                                             0x1
#define HWIO_LPASS_AUDIO_CORE_LPAIF_PRI_CLK_INV_INV_INT_CLK_SHFT                                                               0

#define HWIO_LPASS_AUDIO_CORE_LPAIF_PRI_EXT_CLK_DETECT_ADDR(x)                                                        ((x) + 0x10024)
#define HWIO_LPASS_AUDIO_CORE_LPAIF_PRI_EXT_CLK_DETECT_RMSK                                                           0x80007f13
#define HWIO_LPASS_AUDIO_CORE_LPAIF_PRI_EXT_CLK_DETECT_IN(x)            \
                in_dword(HWIO_LPASS_AUDIO_CORE_LPAIF_PRI_EXT_CLK_DETECT_ADDR(x))
#define HWIO_LPASS_AUDIO_CORE_LPAIF_PRI_EXT_CLK_DETECT_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AUDIO_CORE_LPAIF_PRI_EXT_CLK_DETECT_ADDR(x), m)
#define HWIO_LPASS_AUDIO_CORE_LPAIF_PRI_EXT_CLK_DETECT_OUT(x, v)            \
                out_dword(HWIO_LPASS_AUDIO_CORE_LPAIF_PRI_EXT_CLK_DETECT_ADDR(x),v)
#define HWIO_LPASS_AUDIO_CORE_LPAIF_PRI_EXT_CLK_DETECT_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AUDIO_CORE_LPAIF_PRI_EXT_CLK_DETECT_ADDR(x),m,v,HWIO_LPASS_AUDIO_CORE_LPAIF_PRI_EXT_CLK_DETECT_IN(x))
#define HWIO_LPASS_AUDIO_CORE_LPAIF_PRI_EXT_CLK_DETECT_CLK_INACTIVE_IRQ_BMSK                                          0x80000000
#define HWIO_LPASS_AUDIO_CORE_LPAIF_PRI_EXT_CLK_DETECT_CLK_INACTIVE_IRQ_SHFT                                                  31
#define HWIO_LPASS_AUDIO_CORE_LPAIF_PRI_EXT_CLK_DETECT_CLK_COUNTER_VALUE_BMSK                                             0x7f00
#define HWIO_LPASS_AUDIO_CORE_LPAIF_PRI_EXT_CLK_DETECT_CLK_COUNTER_VALUE_SHFT                                                  8
#define HWIO_LPASS_AUDIO_CORE_LPAIF_PRI_EXT_CLK_DETECT_IRQ_CLEAR_BMSK                                                       0x10
#define HWIO_LPASS_AUDIO_CORE_LPAIF_PRI_EXT_CLK_DETECT_IRQ_CLEAR_SHFT                                                          4
#define HWIO_LPASS_AUDIO_CORE_LPAIF_PRI_EXT_CLK_DETECT_CLK_COUNTER_EN_BMSK                                                   0x2
#define HWIO_LPASS_AUDIO_CORE_LPAIF_PRI_EXT_CLK_DETECT_CLK_COUNTER_EN_SHFT                                                     1
#define HWIO_LPASS_AUDIO_CORE_LPAIF_PRI_EXT_CLK_DETECT_IRQ_EN_BMSK                                                           0x1
#define HWIO_LPASS_AUDIO_CORE_LPAIF_PRI_EXT_CLK_DETECT_IRQ_EN_SHFT                                                             0

#define HWIO_LPASS_LPAIF_SEC_CMD_RCGR_ADDR(x)                                                                         ((x) + 0x11000)
#define HWIO_LPASS_LPAIF_SEC_CMD_RCGR_RMSK                                                                            0x800000f3
#define HWIO_LPASS_LPAIF_SEC_CMD_RCGR_IN(x)            \
                in_dword(HWIO_LPASS_LPAIF_SEC_CMD_RCGR_ADDR(x))
#define HWIO_LPASS_LPAIF_SEC_CMD_RCGR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPAIF_SEC_CMD_RCGR_ADDR(x), m)
#define HWIO_LPASS_LPAIF_SEC_CMD_RCGR_OUT(x, v)            \
                out_dword(HWIO_LPASS_LPAIF_SEC_CMD_RCGR_ADDR(x),v)
#define HWIO_LPASS_LPAIF_SEC_CMD_RCGR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_LPAIF_SEC_CMD_RCGR_ADDR(x),m,v,HWIO_LPASS_LPAIF_SEC_CMD_RCGR_IN(x))
#define HWIO_LPASS_LPAIF_SEC_CMD_RCGR_ROOT_OFF_BMSK                                                                   0x80000000
#define HWIO_LPASS_LPAIF_SEC_CMD_RCGR_ROOT_OFF_SHFT                                                                           31
#define HWIO_LPASS_LPAIF_SEC_CMD_RCGR_DIRTY_D_BMSK                                                                          0x80
#define HWIO_LPASS_LPAIF_SEC_CMD_RCGR_DIRTY_D_SHFT                                                                             7
#define HWIO_LPASS_LPAIF_SEC_CMD_RCGR_DIRTY_N_BMSK                                                                          0x40
#define HWIO_LPASS_LPAIF_SEC_CMD_RCGR_DIRTY_N_SHFT                                                                             6
#define HWIO_LPASS_LPAIF_SEC_CMD_RCGR_DIRTY_M_BMSK                                                                          0x20
#define HWIO_LPASS_LPAIF_SEC_CMD_RCGR_DIRTY_M_SHFT                                                                             5
#define HWIO_LPASS_LPAIF_SEC_CMD_RCGR_DIRTY_CFG_RCGR_BMSK                                                                   0x10
#define HWIO_LPASS_LPAIF_SEC_CMD_RCGR_DIRTY_CFG_RCGR_SHFT                                                                      4
#define HWIO_LPASS_LPAIF_SEC_CMD_RCGR_ROOT_EN_BMSK                                                                           0x2
#define HWIO_LPASS_LPAIF_SEC_CMD_RCGR_ROOT_EN_SHFT                                                                             1
#define HWIO_LPASS_LPAIF_SEC_CMD_RCGR_UPDATE_BMSK                                                                            0x1
#define HWIO_LPASS_LPAIF_SEC_CMD_RCGR_UPDATE_SHFT                                                                              0

#define HWIO_LPASS_LPAIF_SEC_CFG_RCGR_ADDR(x)                                                                         ((x) + 0x11004)
#define HWIO_LPASS_LPAIF_SEC_CFG_RCGR_RMSK                                                                                0x771f
#define HWIO_LPASS_LPAIF_SEC_CFG_RCGR_IN(x)            \
                in_dword(HWIO_LPASS_LPAIF_SEC_CFG_RCGR_ADDR(x))
#define HWIO_LPASS_LPAIF_SEC_CFG_RCGR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPAIF_SEC_CFG_RCGR_ADDR(x), m)
#define HWIO_LPASS_LPAIF_SEC_CFG_RCGR_OUT(x, v)            \
                out_dword(HWIO_LPASS_LPAIF_SEC_CFG_RCGR_ADDR(x),v)
#define HWIO_LPASS_LPAIF_SEC_CFG_RCGR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_LPAIF_SEC_CFG_RCGR_ADDR(x),m,v,HWIO_LPASS_LPAIF_SEC_CFG_RCGR_IN(x))
#define HWIO_LPASS_LPAIF_SEC_CFG_RCGR_ALT_SRC_SEL_BMSK                                                                    0x4000
#define HWIO_LPASS_LPAIF_SEC_CFG_RCGR_ALT_SRC_SEL_SHFT                                                                        14
#define HWIO_LPASS_LPAIF_SEC_CFG_RCGR_MODE_BMSK                                                                           0x3000
#define HWIO_LPASS_LPAIF_SEC_CFG_RCGR_MODE_SHFT                                                                               12
#define HWIO_LPASS_LPAIF_SEC_CFG_RCGR_SRC_SEL_BMSK                                                                         0x700
#define HWIO_LPASS_LPAIF_SEC_CFG_RCGR_SRC_SEL_SHFT                                                                             8
#define HWIO_LPASS_LPAIF_SEC_CFG_RCGR_SRC_DIV_BMSK                                                                          0x1f
#define HWIO_LPASS_LPAIF_SEC_CFG_RCGR_SRC_DIV_SHFT                                                                             0

#define HWIO_LPASS_LPAIF_SEC_M_ADDR(x)                                                                                ((x) + 0x11008)
#define HWIO_LPASS_LPAIF_SEC_M_RMSK                                                                                       0xffff
#define HWIO_LPASS_LPAIF_SEC_M_IN(x)            \
                in_dword(HWIO_LPASS_LPAIF_SEC_M_ADDR(x))
#define HWIO_LPASS_LPAIF_SEC_M_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPAIF_SEC_M_ADDR(x), m)
#define HWIO_LPASS_LPAIF_SEC_M_OUT(x, v)            \
                out_dword(HWIO_LPASS_LPAIF_SEC_M_ADDR(x),v)
#define HWIO_LPASS_LPAIF_SEC_M_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_LPAIF_SEC_M_ADDR(x),m,v,HWIO_LPASS_LPAIF_SEC_M_IN(x))
#define HWIO_LPASS_LPAIF_SEC_M_M_BMSK                                                                                     0xffff
#define HWIO_LPASS_LPAIF_SEC_M_M_SHFT                                                                                          0

#define HWIO_LPASS_LPAIF_SEC_N_ADDR(x)                                                                                ((x) + 0x1100c)
#define HWIO_LPASS_LPAIF_SEC_N_RMSK                                                                                       0xffff
#define HWIO_LPASS_LPAIF_SEC_N_IN(x)            \
                in_dword(HWIO_LPASS_LPAIF_SEC_N_ADDR(x))
#define HWIO_LPASS_LPAIF_SEC_N_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPAIF_SEC_N_ADDR(x), m)
#define HWIO_LPASS_LPAIF_SEC_N_OUT(x, v)            \
                out_dword(HWIO_LPASS_LPAIF_SEC_N_ADDR(x),v)
#define HWIO_LPASS_LPAIF_SEC_N_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_LPAIF_SEC_N_ADDR(x),m,v,HWIO_LPASS_LPAIF_SEC_N_IN(x))
#define HWIO_LPASS_LPAIF_SEC_N_NOT_N_MINUS_M_BMSK                                                                         0xffff
#define HWIO_LPASS_LPAIF_SEC_N_NOT_N_MINUS_M_SHFT                                                                              0

#define HWIO_LPASS_LPAIF_SEC_D_ADDR(x)                                                                                ((x) + 0x11010)
#define HWIO_LPASS_LPAIF_SEC_D_RMSK                                                                                       0xffff
#define HWIO_LPASS_LPAIF_SEC_D_IN(x)            \
                in_dword(HWIO_LPASS_LPAIF_SEC_D_ADDR(x))
#define HWIO_LPASS_LPAIF_SEC_D_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPAIF_SEC_D_ADDR(x), m)
#define HWIO_LPASS_LPAIF_SEC_D_OUT(x, v)            \
                out_dword(HWIO_LPASS_LPAIF_SEC_D_ADDR(x),v)
#define HWIO_LPASS_LPAIF_SEC_D_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_LPAIF_SEC_D_ADDR(x),m,v,HWIO_LPASS_LPAIF_SEC_D_IN(x))
#define HWIO_LPASS_LPAIF_SEC_D_NOT_2D_BMSK                                                                                0xffff
#define HWIO_LPASS_LPAIF_SEC_D_NOT_2D_SHFT                                                                                     0

#define HWIO_LPASS_AUDIO_CORE_LPAIF_SEC_IBIT_CBCR_ADDR(x)                                                             ((x) + 0x11018)
#define HWIO_LPASS_AUDIO_CORE_LPAIF_SEC_IBIT_CBCR_RMSK                                                                0x81ff0003
#define HWIO_LPASS_AUDIO_CORE_LPAIF_SEC_IBIT_CBCR_IN(x)            \
                in_dword(HWIO_LPASS_AUDIO_CORE_LPAIF_SEC_IBIT_CBCR_ADDR(x))
#define HWIO_LPASS_AUDIO_CORE_LPAIF_SEC_IBIT_CBCR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AUDIO_CORE_LPAIF_SEC_IBIT_CBCR_ADDR(x), m)
#define HWIO_LPASS_AUDIO_CORE_LPAIF_SEC_IBIT_CBCR_OUT(x, v)            \
                out_dword(HWIO_LPASS_AUDIO_CORE_LPAIF_SEC_IBIT_CBCR_ADDR(x),v)
#define HWIO_LPASS_AUDIO_CORE_LPAIF_SEC_IBIT_CBCR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AUDIO_CORE_LPAIF_SEC_IBIT_CBCR_ADDR(x),m,v,HWIO_LPASS_AUDIO_CORE_LPAIF_SEC_IBIT_CBCR_IN(x))
#define HWIO_LPASS_AUDIO_CORE_LPAIF_SEC_IBIT_CBCR_CLK_OFF_BMSK                                                        0x80000000
#define HWIO_LPASS_AUDIO_CORE_LPAIF_SEC_IBIT_CBCR_CLK_OFF_SHFT                                                                31
#define HWIO_LPASS_AUDIO_CORE_LPAIF_SEC_IBIT_CBCR_CLK_DIV_BMSK                                                         0x1ff0000
#define HWIO_LPASS_AUDIO_CORE_LPAIF_SEC_IBIT_CBCR_CLK_DIV_SHFT                                                                16
#define HWIO_LPASS_AUDIO_CORE_LPAIF_SEC_IBIT_CBCR_HW_CTL_BMSK                                                                0x2
#define HWIO_LPASS_AUDIO_CORE_LPAIF_SEC_IBIT_CBCR_HW_CTL_SHFT                                                                  1
#define HWIO_LPASS_AUDIO_CORE_LPAIF_SEC_IBIT_CBCR_CLK_ENABLE_BMSK                                                            0x1
#define HWIO_LPASS_AUDIO_CORE_LPAIF_SEC_IBIT_CBCR_CLK_ENABLE_SHFT                                                              0

#define HWIO_LPASS_AUDIO_CORE_LPAIF_SEC_EBIT_CBCR_ADDR(x)                                                             ((x) + 0x1101c)
#define HWIO_LPASS_AUDIO_CORE_LPAIF_SEC_EBIT_CBCR_RMSK                                                                0x80000003
#define HWIO_LPASS_AUDIO_CORE_LPAIF_SEC_EBIT_CBCR_IN(x)            \
                in_dword(HWIO_LPASS_AUDIO_CORE_LPAIF_SEC_EBIT_CBCR_ADDR(x))
#define HWIO_LPASS_AUDIO_CORE_LPAIF_SEC_EBIT_CBCR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AUDIO_CORE_LPAIF_SEC_EBIT_CBCR_ADDR(x), m)
#define HWIO_LPASS_AUDIO_CORE_LPAIF_SEC_EBIT_CBCR_OUT(x, v)            \
                out_dword(HWIO_LPASS_AUDIO_CORE_LPAIF_SEC_EBIT_CBCR_ADDR(x),v)
#define HWIO_LPASS_AUDIO_CORE_LPAIF_SEC_EBIT_CBCR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AUDIO_CORE_LPAIF_SEC_EBIT_CBCR_ADDR(x),m,v,HWIO_LPASS_AUDIO_CORE_LPAIF_SEC_EBIT_CBCR_IN(x))
#define HWIO_LPASS_AUDIO_CORE_LPAIF_SEC_EBIT_CBCR_CLK_OFF_BMSK                                                        0x80000000
#define HWIO_LPASS_AUDIO_CORE_LPAIF_SEC_EBIT_CBCR_CLK_OFF_SHFT                                                                31
#define HWIO_LPASS_AUDIO_CORE_LPAIF_SEC_EBIT_CBCR_HW_CTL_BMSK                                                                0x2
#define HWIO_LPASS_AUDIO_CORE_LPAIF_SEC_EBIT_CBCR_HW_CTL_SHFT                                                                  1
#define HWIO_LPASS_AUDIO_CORE_LPAIF_SEC_EBIT_CBCR_CLK_ENABLE_BMSK                                                            0x1
#define HWIO_LPASS_AUDIO_CORE_LPAIF_SEC_EBIT_CBCR_CLK_ENABLE_SHFT                                                              0

#define HWIO_LPASS_AUDIO_CORE_LPAIF_SEC_CLK_INV_ADDR(x)                                                               ((x) + 0x11020)
#define HWIO_LPASS_AUDIO_CORE_LPAIF_SEC_CLK_INV_RMSK                                                                         0x3
#define HWIO_LPASS_AUDIO_CORE_LPAIF_SEC_CLK_INV_IN(x)            \
                in_dword(HWIO_LPASS_AUDIO_CORE_LPAIF_SEC_CLK_INV_ADDR(x))
#define HWIO_LPASS_AUDIO_CORE_LPAIF_SEC_CLK_INV_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AUDIO_CORE_LPAIF_SEC_CLK_INV_ADDR(x), m)
#define HWIO_LPASS_AUDIO_CORE_LPAIF_SEC_CLK_INV_OUT(x, v)            \
                out_dword(HWIO_LPASS_AUDIO_CORE_LPAIF_SEC_CLK_INV_ADDR(x),v)
#define HWIO_LPASS_AUDIO_CORE_LPAIF_SEC_CLK_INV_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AUDIO_CORE_LPAIF_SEC_CLK_INV_ADDR(x),m,v,HWIO_LPASS_AUDIO_CORE_LPAIF_SEC_CLK_INV_IN(x))
#define HWIO_LPASS_AUDIO_CORE_LPAIF_SEC_CLK_INV_INV_EXT_CLK_BMSK                                                             0x2
#define HWIO_LPASS_AUDIO_CORE_LPAIF_SEC_CLK_INV_INV_EXT_CLK_SHFT                                                               1
#define HWIO_LPASS_AUDIO_CORE_LPAIF_SEC_CLK_INV_INV_INT_CLK_BMSK                                                             0x1
#define HWIO_LPASS_AUDIO_CORE_LPAIF_SEC_CLK_INV_INV_INT_CLK_SHFT                                                               0

#define HWIO_LPASS_AUDIO_CORE_LPAIF_SEC_EXT_CLK_DETECT_ADDR(x)                                                        ((x) + 0x11024)
#define HWIO_LPASS_AUDIO_CORE_LPAIF_SEC_EXT_CLK_DETECT_RMSK                                                           0x80007f13
#define HWIO_LPASS_AUDIO_CORE_LPAIF_SEC_EXT_CLK_DETECT_IN(x)            \
                in_dword(HWIO_LPASS_AUDIO_CORE_LPAIF_SEC_EXT_CLK_DETECT_ADDR(x))
#define HWIO_LPASS_AUDIO_CORE_LPAIF_SEC_EXT_CLK_DETECT_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AUDIO_CORE_LPAIF_SEC_EXT_CLK_DETECT_ADDR(x), m)
#define HWIO_LPASS_AUDIO_CORE_LPAIF_SEC_EXT_CLK_DETECT_OUT(x, v)            \
                out_dword(HWIO_LPASS_AUDIO_CORE_LPAIF_SEC_EXT_CLK_DETECT_ADDR(x),v)
#define HWIO_LPASS_AUDIO_CORE_LPAIF_SEC_EXT_CLK_DETECT_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AUDIO_CORE_LPAIF_SEC_EXT_CLK_DETECT_ADDR(x),m,v,HWIO_LPASS_AUDIO_CORE_LPAIF_SEC_EXT_CLK_DETECT_IN(x))
#define HWIO_LPASS_AUDIO_CORE_LPAIF_SEC_EXT_CLK_DETECT_CLK_INACTIVE_IRQ_BMSK                                          0x80000000
#define HWIO_LPASS_AUDIO_CORE_LPAIF_SEC_EXT_CLK_DETECT_CLK_INACTIVE_IRQ_SHFT                                                  31
#define HWIO_LPASS_AUDIO_CORE_LPAIF_SEC_EXT_CLK_DETECT_CLK_COUNTER_VALUE_BMSK                                             0x7f00
#define HWIO_LPASS_AUDIO_CORE_LPAIF_SEC_EXT_CLK_DETECT_CLK_COUNTER_VALUE_SHFT                                                  8
#define HWIO_LPASS_AUDIO_CORE_LPAIF_SEC_EXT_CLK_DETECT_IRQ_CLEAR_BMSK                                                       0x10
#define HWIO_LPASS_AUDIO_CORE_LPAIF_SEC_EXT_CLK_DETECT_IRQ_CLEAR_SHFT                                                          4
#define HWIO_LPASS_AUDIO_CORE_LPAIF_SEC_EXT_CLK_DETECT_CLK_COUNTER_EN_BMSK                                                   0x2
#define HWIO_LPASS_AUDIO_CORE_LPAIF_SEC_EXT_CLK_DETECT_CLK_COUNTER_EN_SHFT                                                     1
#define HWIO_LPASS_AUDIO_CORE_LPAIF_SEC_EXT_CLK_DETECT_IRQ_EN_BMSK                                                           0x1
#define HWIO_LPASS_AUDIO_CORE_LPAIF_SEC_EXT_CLK_DETECT_IRQ_EN_SHFT                                                             0

#define HWIO_LPASS_LPAIF_TER_CMD_RCGR_ADDR(x)                                                                         ((x) + 0x12000)
#define HWIO_LPASS_LPAIF_TER_CMD_RCGR_RMSK                                                                            0x800000f3
#define HWIO_LPASS_LPAIF_TER_CMD_RCGR_IN(x)            \
                in_dword(HWIO_LPASS_LPAIF_TER_CMD_RCGR_ADDR(x))
#define HWIO_LPASS_LPAIF_TER_CMD_RCGR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPAIF_TER_CMD_RCGR_ADDR(x), m)
#define HWIO_LPASS_LPAIF_TER_CMD_RCGR_OUT(x, v)            \
                out_dword(HWIO_LPASS_LPAIF_TER_CMD_RCGR_ADDR(x),v)
#define HWIO_LPASS_LPAIF_TER_CMD_RCGR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_LPAIF_TER_CMD_RCGR_ADDR(x),m,v,HWIO_LPASS_LPAIF_TER_CMD_RCGR_IN(x))
#define HWIO_LPASS_LPAIF_TER_CMD_RCGR_ROOT_OFF_BMSK                                                                   0x80000000
#define HWIO_LPASS_LPAIF_TER_CMD_RCGR_ROOT_OFF_SHFT                                                                           31
#define HWIO_LPASS_LPAIF_TER_CMD_RCGR_DIRTY_D_BMSK                                                                          0x80
#define HWIO_LPASS_LPAIF_TER_CMD_RCGR_DIRTY_D_SHFT                                                                             7
#define HWIO_LPASS_LPAIF_TER_CMD_RCGR_DIRTY_N_BMSK                                                                          0x40
#define HWIO_LPASS_LPAIF_TER_CMD_RCGR_DIRTY_N_SHFT                                                                             6
#define HWIO_LPASS_LPAIF_TER_CMD_RCGR_DIRTY_M_BMSK                                                                          0x20
#define HWIO_LPASS_LPAIF_TER_CMD_RCGR_DIRTY_M_SHFT                                                                             5
#define HWIO_LPASS_LPAIF_TER_CMD_RCGR_DIRTY_CFG_RCGR_BMSK                                                                   0x10
#define HWIO_LPASS_LPAIF_TER_CMD_RCGR_DIRTY_CFG_RCGR_SHFT                                                                      4
#define HWIO_LPASS_LPAIF_TER_CMD_RCGR_ROOT_EN_BMSK                                                                           0x2
#define HWIO_LPASS_LPAIF_TER_CMD_RCGR_ROOT_EN_SHFT                                                                             1
#define HWIO_LPASS_LPAIF_TER_CMD_RCGR_UPDATE_BMSK                                                                            0x1
#define HWIO_LPASS_LPAIF_TER_CMD_RCGR_UPDATE_SHFT                                                                              0

#define HWIO_LPASS_LPAIF_TER_CFG_RCGR_ADDR(x)                                                                         ((x) + 0x12004)
#define HWIO_LPASS_LPAIF_TER_CFG_RCGR_RMSK                                                                                0x771f
#define HWIO_LPASS_LPAIF_TER_CFG_RCGR_IN(x)            \
                in_dword(HWIO_LPASS_LPAIF_TER_CFG_RCGR_ADDR(x))
#define HWIO_LPASS_LPAIF_TER_CFG_RCGR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPAIF_TER_CFG_RCGR_ADDR(x), m)
#define HWIO_LPASS_LPAIF_TER_CFG_RCGR_OUT(x, v)            \
                out_dword(HWIO_LPASS_LPAIF_TER_CFG_RCGR_ADDR(x),v)
#define HWIO_LPASS_LPAIF_TER_CFG_RCGR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_LPAIF_TER_CFG_RCGR_ADDR(x),m,v,HWIO_LPASS_LPAIF_TER_CFG_RCGR_IN(x))
#define HWIO_LPASS_LPAIF_TER_CFG_RCGR_ALT_SRC_SEL_BMSK                                                                    0x4000
#define HWIO_LPASS_LPAIF_TER_CFG_RCGR_ALT_SRC_SEL_SHFT                                                                        14
#define HWIO_LPASS_LPAIF_TER_CFG_RCGR_MODE_BMSK                                                                           0x3000
#define HWIO_LPASS_LPAIF_TER_CFG_RCGR_MODE_SHFT                                                                               12
#define HWIO_LPASS_LPAIF_TER_CFG_RCGR_SRC_SEL_BMSK                                                                         0x700
#define HWIO_LPASS_LPAIF_TER_CFG_RCGR_SRC_SEL_SHFT                                                                             8
#define HWIO_LPASS_LPAIF_TER_CFG_RCGR_SRC_DIV_BMSK                                                                          0x1f
#define HWIO_LPASS_LPAIF_TER_CFG_RCGR_SRC_DIV_SHFT                                                                             0

#define HWIO_LPASS_LPAIF_TER_M_ADDR(x)                                                                                ((x) + 0x12008)
#define HWIO_LPASS_LPAIF_TER_M_RMSK                                                                                       0xffff
#define HWIO_LPASS_LPAIF_TER_M_IN(x)            \
                in_dword(HWIO_LPASS_LPAIF_TER_M_ADDR(x))
#define HWIO_LPASS_LPAIF_TER_M_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPAIF_TER_M_ADDR(x), m)
#define HWIO_LPASS_LPAIF_TER_M_OUT(x, v)            \
                out_dword(HWIO_LPASS_LPAIF_TER_M_ADDR(x),v)
#define HWIO_LPASS_LPAIF_TER_M_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_LPAIF_TER_M_ADDR(x),m,v,HWIO_LPASS_LPAIF_TER_M_IN(x))
#define HWIO_LPASS_LPAIF_TER_M_M_BMSK                                                                                     0xffff
#define HWIO_LPASS_LPAIF_TER_M_M_SHFT                                                                                          0

#define HWIO_LPASS_LPAIF_TER_N_ADDR(x)                                                                                ((x) + 0x1200c)
#define HWIO_LPASS_LPAIF_TER_N_RMSK                                                                                       0xffff
#define HWIO_LPASS_LPAIF_TER_N_IN(x)            \
                in_dword(HWIO_LPASS_LPAIF_TER_N_ADDR(x))
#define HWIO_LPASS_LPAIF_TER_N_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPAIF_TER_N_ADDR(x), m)
#define HWIO_LPASS_LPAIF_TER_N_OUT(x, v)            \
                out_dword(HWIO_LPASS_LPAIF_TER_N_ADDR(x),v)
#define HWIO_LPASS_LPAIF_TER_N_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_LPAIF_TER_N_ADDR(x),m,v,HWIO_LPASS_LPAIF_TER_N_IN(x))
#define HWIO_LPASS_LPAIF_TER_N_NOT_N_MINUS_M_BMSK                                                                         0xffff
#define HWIO_LPASS_LPAIF_TER_N_NOT_N_MINUS_M_SHFT                                                                              0

#define HWIO_LPASS_LPAIF_TER_D_ADDR(x)                                                                                ((x) + 0x12010)
#define HWIO_LPASS_LPAIF_TER_D_RMSK                                                                                       0xffff
#define HWIO_LPASS_LPAIF_TER_D_IN(x)            \
                in_dword(HWIO_LPASS_LPAIF_TER_D_ADDR(x))
#define HWIO_LPASS_LPAIF_TER_D_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPAIF_TER_D_ADDR(x), m)
#define HWIO_LPASS_LPAIF_TER_D_OUT(x, v)            \
                out_dword(HWIO_LPASS_LPAIF_TER_D_ADDR(x),v)
#define HWIO_LPASS_LPAIF_TER_D_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_LPAIF_TER_D_ADDR(x),m,v,HWIO_LPASS_LPAIF_TER_D_IN(x))
#define HWIO_LPASS_LPAIF_TER_D_NOT_2D_BMSK                                                                                0xffff
#define HWIO_LPASS_LPAIF_TER_D_NOT_2D_SHFT                                                                                     0

#define HWIO_LPASS_AUDIO_CORE_LPAIF_TER_IBIT_CBCR_ADDR(x)                                                             ((x) + 0x12018)
#define HWIO_LPASS_AUDIO_CORE_LPAIF_TER_IBIT_CBCR_RMSK                                                                0x81ff0003
#define HWIO_LPASS_AUDIO_CORE_LPAIF_TER_IBIT_CBCR_IN(x)            \
                in_dword(HWIO_LPASS_AUDIO_CORE_LPAIF_TER_IBIT_CBCR_ADDR(x))
#define HWIO_LPASS_AUDIO_CORE_LPAIF_TER_IBIT_CBCR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AUDIO_CORE_LPAIF_TER_IBIT_CBCR_ADDR(x), m)
#define HWIO_LPASS_AUDIO_CORE_LPAIF_TER_IBIT_CBCR_OUT(x, v)            \
                out_dword(HWIO_LPASS_AUDIO_CORE_LPAIF_TER_IBIT_CBCR_ADDR(x),v)
#define HWIO_LPASS_AUDIO_CORE_LPAIF_TER_IBIT_CBCR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AUDIO_CORE_LPAIF_TER_IBIT_CBCR_ADDR(x),m,v,HWIO_LPASS_AUDIO_CORE_LPAIF_TER_IBIT_CBCR_IN(x))
#define HWIO_LPASS_AUDIO_CORE_LPAIF_TER_IBIT_CBCR_CLK_OFF_BMSK                                                        0x80000000
#define HWIO_LPASS_AUDIO_CORE_LPAIF_TER_IBIT_CBCR_CLK_OFF_SHFT                                                                31
#define HWIO_LPASS_AUDIO_CORE_LPAIF_TER_IBIT_CBCR_CLK_DIV_BMSK                                                         0x1ff0000
#define HWIO_LPASS_AUDIO_CORE_LPAIF_TER_IBIT_CBCR_CLK_DIV_SHFT                                                                16
#define HWIO_LPASS_AUDIO_CORE_LPAIF_TER_IBIT_CBCR_HW_CTL_BMSK                                                                0x2
#define HWIO_LPASS_AUDIO_CORE_LPAIF_TER_IBIT_CBCR_HW_CTL_SHFT                                                                  1
#define HWIO_LPASS_AUDIO_CORE_LPAIF_TER_IBIT_CBCR_CLK_ENABLE_BMSK                                                            0x1
#define HWIO_LPASS_AUDIO_CORE_LPAIF_TER_IBIT_CBCR_CLK_ENABLE_SHFT                                                              0

#define HWIO_LPASS_AUDIO_CORE_LPAIF_TER_EBIT_CBCR_ADDR(x)                                                             ((x) + 0x1201c)
#define HWIO_LPASS_AUDIO_CORE_LPAIF_TER_EBIT_CBCR_RMSK                                                                0x80000003
#define HWIO_LPASS_AUDIO_CORE_LPAIF_TER_EBIT_CBCR_IN(x)            \
                in_dword(HWIO_LPASS_AUDIO_CORE_LPAIF_TER_EBIT_CBCR_ADDR(x))
#define HWIO_LPASS_AUDIO_CORE_LPAIF_TER_EBIT_CBCR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AUDIO_CORE_LPAIF_TER_EBIT_CBCR_ADDR(x), m)
#define HWIO_LPASS_AUDIO_CORE_LPAIF_TER_EBIT_CBCR_OUT(x, v)            \
                out_dword(HWIO_LPASS_AUDIO_CORE_LPAIF_TER_EBIT_CBCR_ADDR(x),v)
#define HWIO_LPASS_AUDIO_CORE_LPAIF_TER_EBIT_CBCR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AUDIO_CORE_LPAIF_TER_EBIT_CBCR_ADDR(x),m,v,HWIO_LPASS_AUDIO_CORE_LPAIF_TER_EBIT_CBCR_IN(x))
#define HWIO_LPASS_AUDIO_CORE_LPAIF_TER_EBIT_CBCR_CLK_OFF_BMSK                                                        0x80000000
#define HWIO_LPASS_AUDIO_CORE_LPAIF_TER_EBIT_CBCR_CLK_OFF_SHFT                                                                31
#define HWIO_LPASS_AUDIO_CORE_LPAIF_TER_EBIT_CBCR_HW_CTL_BMSK                                                                0x2
#define HWIO_LPASS_AUDIO_CORE_LPAIF_TER_EBIT_CBCR_HW_CTL_SHFT                                                                  1
#define HWIO_LPASS_AUDIO_CORE_LPAIF_TER_EBIT_CBCR_CLK_ENABLE_BMSK                                                            0x1
#define HWIO_LPASS_AUDIO_CORE_LPAIF_TER_EBIT_CBCR_CLK_ENABLE_SHFT                                                              0

#define HWIO_LPASS_AUDIO_CORE_LPAIF_TER_CLK_INV_ADDR(x)                                                               ((x) + 0x12020)
#define HWIO_LPASS_AUDIO_CORE_LPAIF_TER_CLK_INV_RMSK                                                                         0x3
#define HWIO_LPASS_AUDIO_CORE_LPAIF_TER_CLK_INV_IN(x)            \
                in_dword(HWIO_LPASS_AUDIO_CORE_LPAIF_TER_CLK_INV_ADDR(x))
#define HWIO_LPASS_AUDIO_CORE_LPAIF_TER_CLK_INV_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AUDIO_CORE_LPAIF_TER_CLK_INV_ADDR(x), m)
#define HWIO_LPASS_AUDIO_CORE_LPAIF_TER_CLK_INV_OUT(x, v)            \
                out_dword(HWIO_LPASS_AUDIO_CORE_LPAIF_TER_CLK_INV_ADDR(x),v)
#define HWIO_LPASS_AUDIO_CORE_LPAIF_TER_CLK_INV_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AUDIO_CORE_LPAIF_TER_CLK_INV_ADDR(x),m,v,HWIO_LPASS_AUDIO_CORE_LPAIF_TER_CLK_INV_IN(x))
#define HWIO_LPASS_AUDIO_CORE_LPAIF_TER_CLK_INV_INV_EXT_CLK_BMSK                                                             0x2
#define HWIO_LPASS_AUDIO_CORE_LPAIF_TER_CLK_INV_INV_EXT_CLK_SHFT                                                               1
#define HWIO_LPASS_AUDIO_CORE_LPAIF_TER_CLK_INV_INV_INT_CLK_BMSK                                                             0x1
#define HWIO_LPASS_AUDIO_CORE_LPAIF_TER_CLK_INV_INV_INT_CLK_SHFT                                                               0

#define HWIO_LPASS_AUDIO_CORE_LPAIF_TER_EXT_CLK_DETECT_ADDR(x)                                                        ((x) + 0x12024)
#define HWIO_LPASS_AUDIO_CORE_LPAIF_TER_EXT_CLK_DETECT_RMSK                                                           0x80007f13
#define HWIO_LPASS_AUDIO_CORE_LPAIF_TER_EXT_CLK_DETECT_IN(x)            \
                in_dword(HWIO_LPASS_AUDIO_CORE_LPAIF_TER_EXT_CLK_DETECT_ADDR(x))
#define HWIO_LPASS_AUDIO_CORE_LPAIF_TER_EXT_CLK_DETECT_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AUDIO_CORE_LPAIF_TER_EXT_CLK_DETECT_ADDR(x), m)
#define HWIO_LPASS_AUDIO_CORE_LPAIF_TER_EXT_CLK_DETECT_OUT(x, v)            \
                out_dword(HWIO_LPASS_AUDIO_CORE_LPAIF_TER_EXT_CLK_DETECT_ADDR(x),v)
#define HWIO_LPASS_AUDIO_CORE_LPAIF_TER_EXT_CLK_DETECT_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AUDIO_CORE_LPAIF_TER_EXT_CLK_DETECT_ADDR(x),m,v,HWIO_LPASS_AUDIO_CORE_LPAIF_TER_EXT_CLK_DETECT_IN(x))
#define HWIO_LPASS_AUDIO_CORE_LPAIF_TER_EXT_CLK_DETECT_CLK_INACTIVE_IRQ_BMSK                                          0x80000000
#define HWIO_LPASS_AUDIO_CORE_LPAIF_TER_EXT_CLK_DETECT_CLK_INACTIVE_IRQ_SHFT                                                  31
#define HWIO_LPASS_AUDIO_CORE_LPAIF_TER_EXT_CLK_DETECT_CLK_COUNTER_VALUE_BMSK                                             0x7f00
#define HWIO_LPASS_AUDIO_CORE_LPAIF_TER_EXT_CLK_DETECT_CLK_COUNTER_VALUE_SHFT                                                  8
#define HWIO_LPASS_AUDIO_CORE_LPAIF_TER_EXT_CLK_DETECT_IRQ_CLEAR_BMSK                                                       0x10
#define HWIO_LPASS_AUDIO_CORE_LPAIF_TER_EXT_CLK_DETECT_IRQ_CLEAR_SHFT                                                          4
#define HWIO_LPASS_AUDIO_CORE_LPAIF_TER_EXT_CLK_DETECT_CLK_COUNTER_EN_BMSK                                                   0x2
#define HWIO_LPASS_AUDIO_CORE_LPAIF_TER_EXT_CLK_DETECT_CLK_COUNTER_EN_SHFT                                                     1
#define HWIO_LPASS_AUDIO_CORE_LPAIF_TER_EXT_CLK_DETECT_IRQ_EN_BMSK                                                           0x1
#define HWIO_LPASS_AUDIO_CORE_LPAIF_TER_EXT_CLK_DETECT_IRQ_EN_SHFT                                                             0

#define HWIO_LPASS_LPAIF_QUAD_CMD_RCGR_ADDR(x)                                                                        ((x) + 0x13000)
#define HWIO_LPASS_LPAIF_QUAD_CMD_RCGR_RMSK                                                                           0x800000f3
#define HWIO_LPASS_LPAIF_QUAD_CMD_RCGR_IN(x)            \
                in_dword(HWIO_LPASS_LPAIF_QUAD_CMD_RCGR_ADDR(x))
#define HWIO_LPASS_LPAIF_QUAD_CMD_RCGR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPAIF_QUAD_CMD_RCGR_ADDR(x), m)
#define HWIO_LPASS_LPAIF_QUAD_CMD_RCGR_OUT(x, v)            \
                out_dword(HWIO_LPASS_LPAIF_QUAD_CMD_RCGR_ADDR(x),v)
#define HWIO_LPASS_LPAIF_QUAD_CMD_RCGR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_LPAIF_QUAD_CMD_RCGR_ADDR(x),m,v,HWIO_LPASS_LPAIF_QUAD_CMD_RCGR_IN(x))
#define HWIO_LPASS_LPAIF_QUAD_CMD_RCGR_ROOT_OFF_BMSK                                                                  0x80000000
#define HWIO_LPASS_LPAIF_QUAD_CMD_RCGR_ROOT_OFF_SHFT                                                                          31
#define HWIO_LPASS_LPAIF_QUAD_CMD_RCGR_DIRTY_D_BMSK                                                                         0x80
#define HWIO_LPASS_LPAIF_QUAD_CMD_RCGR_DIRTY_D_SHFT                                                                            7
#define HWIO_LPASS_LPAIF_QUAD_CMD_RCGR_DIRTY_N_BMSK                                                                         0x40
#define HWIO_LPASS_LPAIF_QUAD_CMD_RCGR_DIRTY_N_SHFT                                                                            6
#define HWIO_LPASS_LPAIF_QUAD_CMD_RCGR_DIRTY_M_BMSK                                                                         0x20
#define HWIO_LPASS_LPAIF_QUAD_CMD_RCGR_DIRTY_M_SHFT                                                                            5
#define HWIO_LPASS_LPAIF_QUAD_CMD_RCGR_DIRTY_CFG_RCGR_BMSK                                                                  0x10
#define HWIO_LPASS_LPAIF_QUAD_CMD_RCGR_DIRTY_CFG_RCGR_SHFT                                                                     4
#define HWIO_LPASS_LPAIF_QUAD_CMD_RCGR_ROOT_EN_BMSK                                                                          0x2
#define HWIO_LPASS_LPAIF_QUAD_CMD_RCGR_ROOT_EN_SHFT                                                                            1
#define HWIO_LPASS_LPAIF_QUAD_CMD_RCGR_UPDATE_BMSK                                                                           0x1
#define HWIO_LPASS_LPAIF_QUAD_CMD_RCGR_UPDATE_SHFT                                                                             0

#define HWIO_LPASS_LPAIF_QUAD_CFG_RCGR_ADDR(x)                                                                        ((x) + 0x13004)
#define HWIO_LPASS_LPAIF_QUAD_CFG_RCGR_RMSK                                                                               0x771f
#define HWIO_LPASS_LPAIF_QUAD_CFG_RCGR_IN(x)            \
                in_dword(HWIO_LPASS_LPAIF_QUAD_CFG_RCGR_ADDR(x))
#define HWIO_LPASS_LPAIF_QUAD_CFG_RCGR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPAIF_QUAD_CFG_RCGR_ADDR(x), m)
#define HWIO_LPASS_LPAIF_QUAD_CFG_RCGR_OUT(x, v)            \
                out_dword(HWIO_LPASS_LPAIF_QUAD_CFG_RCGR_ADDR(x),v)
#define HWIO_LPASS_LPAIF_QUAD_CFG_RCGR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_LPAIF_QUAD_CFG_RCGR_ADDR(x),m,v,HWIO_LPASS_LPAIF_QUAD_CFG_RCGR_IN(x))
#define HWIO_LPASS_LPAIF_QUAD_CFG_RCGR_ALT_SRC_SEL_BMSK                                                                   0x4000
#define HWIO_LPASS_LPAIF_QUAD_CFG_RCGR_ALT_SRC_SEL_SHFT                                                                       14
#define HWIO_LPASS_LPAIF_QUAD_CFG_RCGR_MODE_BMSK                                                                          0x3000
#define HWIO_LPASS_LPAIF_QUAD_CFG_RCGR_MODE_SHFT                                                                              12
#define HWIO_LPASS_LPAIF_QUAD_CFG_RCGR_SRC_SEL_BMSK                                                                        0x700
#define HWIO_LPASS_LPAIF_QUAD_CFG_RCGR_SRC_SEL_SHFT                                                                            8
#define HWIO_LPASS_LPAIF_QUAD_CFG_RCGR_SRC_DIV_BMSK                                                                         0x1f
#define HWIO_LPASS_LPAIF_QUAD_CFG_RCGR_SRC_DIV_SHFT                                                                            0

#define HWIO_LPASS_LPAIF_QUAD_M_ADDR(x)                                                                               ((x) + 0x13008)
#define HWIO_LPASS_LPAIF_QUAD_M_RMSK                                                                                      0xffff
#define HWIO_LPASS_LPAIF_QUAD_M_IN(x)            \
                in_dword(HWIO_LPASS_LPAIF_QUAD_M_ADDR(x))
#define HWIO_LPASS_LPAIF_QUAD_M_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPAIF_QUAD_M_ADDR(x), m)
#define HWIO_LPASS_LPAIF_QUAD_M_OUT(x, v)            \
                out_dword(HWIO_LPASS_LPAIF_QUAD_M_ADDR(x),v)
#define HWIO_LPASS_LPAIF_QUAD_M_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_LPAIF_QUAD_M_ADDR(x),m,v,HWIO_LPASS_LPAIF_QUAD_M_IN(x))
#define HWIO_LPASS_LPAIF_QUAD_M_M_BMSK                                                                                    0xffff
#define HWIO_LPASS_LPAIF_QUAD_M_M_SHFT                                                                                         0

#define HWIO_LPASS_LPAIF_QUAD_N_ADDR(x)                                                                               ((x) + 0x1300c)
#define HWIO_LPASS_LPAIF_QUAD_N_RMSK                                                                                      0xffff
#define HWIO_LPASS_LPAIF_QUAD_N_IN(x)            \
                in_dword(HWIO_LPASS_LPAIF_QUAD_N_ADDR(x))
#define HWIO_LPASS_LPAIF_QUAD_N_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPAIF_QUAD_N_ADDR(x), m)
#define HWIO_LPASS_LPAIF_QUAD_N_OUT(x, v)            \
                out_dword(HWIO_LPASS_LPAIF_QUAD_N_ADDR(x),v)
#define HWIO_LPASS_LPAIF_QUAD_N_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_LPAIF_QUAD_N_ADDR(x),m,v,HWIO_LPASS_LPAIF_QUAD_N_IN(x))
#define HWIO_LPASS_LPAIF_QUAD_N_NOT_N_MINUS_M_BMSK                                                                        0xffff
#define HWIO_LPASS_LPAIF_QUAD_N_NOT_N_MINUS_M_SHFT                                                                             0

#define HWIO_LPASS_LPAIF_QUAD_D_ADDR(x)                                                                               ((x) + 0x13010)
#define HWIO_LPASS_LPAIF_QUAD_D_RMSK                                                                                      0xffff
#define HWIO_LPASS_LPAIF_QUAD_D_IN(x)            \
                in_dword(HWIO_LPASS_LPAIF_QUAD_D_ADDR(x))
#define HWIO_LPASS_LPAIF_QUAD_D_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPAIF_QUAD_D_ADDR(x), m)
#define HWIO_LPASS_LPAIF_QUAD_D_OUT(x, v)            \
                out_dword(HWIO_LPASS_LPAIF_QUAD_D_ADDR(x),v)
#define HWIO_LPASS_LPAIF_QUAD_D_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_LPAIF_QUAD_D_ADDR(x),m,v,HWIO_LPASS_LPAIF_QUAD_D_IN(x))
#define HWIO_LPASS_LPAIF_QUAD_D_NOT_2D_BMSK                                                                               0xffff
#define HWIO_LPASS_LPAIF_QUAD_D_NOT_2D_SHFT                                                                                    0

#define HWIO_LPASS_AUDIO_CORE_LPAIF_QUAD_IBIT_CBCR_ADDR(x)                                                            ((x) + 0x13018)
#define HWIO_LPASS_AUDIO_CORE_LPAIF_QUAD_IBIT_CBCR_RMSK                                                               0x81ff0003
#define HWIO_LPASS_AUDIO_CORE_LPAIF_QUAD_IBIT_CBCR_IN(x)            \
                in_dword(HWIO_LPASS_AUDIO_CORE_LPAIF_QUAD_IBIT_CBCR_ADDR(x))
#define HWIO_LPASS_AUDIO_CORE_LPAIF_QUAD_IBIT_CBCR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AUDIO_CORE_LPAIF_QUAD_IBIT_CBCR_ADDR(x), m)
#define HWIO_LPASS_AUDIO_CORE_LPAIF_QUAD_IBIT_CBCR_OUT(x, v)            \
                out_dword(HWIO_LPASS_AUDIO_CORE_LPAIF_QUAD_IBIT_CBCR_ADDR(x),v)
#define HWIO_LPASS_AUDIO_CORE_LPAIF_QUAD_IBIT_CBCR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AUDIO_CORE_LPAIF_QUAD_IBIT_CBCR_ADDR(x),m,v,HWIO_LPASS_AUDIO_CORE_LPAIF_QUAD_IBIT_CBCR_IN(x))
#define HWIO_LPASS_AUDIO_CORE_LPAIF_QUAD_IBIT_CBCR_CLK_OFF_BMSK                                                       0x80000000
#define HWIO_LPASS_AUDIO_CORE_LPAIF_QUAD_IBIT_CBCR_CLK_OFF_SHFT                                                               31
#define HWIO_LPASS_AUDIO_CORE_LPAIF_QUAD_IBIT_CBCR_CLK_DIV_BMSK                                                        0x1ff0000
#define HWIO_LPASS_AUDIO_CORE_LPAIF_QUAD_IBIT_CBCR_CLK_DIV_SHFT                                                               16
#define HWIO_LPASS_AUDIO_CORE_LPAIF_QUAD_IBIT_CBCR_HW_CTL_BMSK                                                               0x2
#define HWIO_LPASS_AUDIO_CORE_LPAIF_QUAD_IBIT_CBCR_HW_CTL_SHFT                                                                 1
#define HWIO_LPASS_AUDIO_CORE_LPAIF_QUAD_IBIT_CBCR_CLK_ENABLE_BMSK                                                           0x1
#define HWIO_LPASS_AUDIO_CORE_LPAIF_QUAD_IBIT_CBCR_CLK_ENABLE_SHFT                                                             0

#define HWIO_LPASS_AUDIO_CORE_LPAIF_QUAD_EBIT_CBCR_ADDR(x)                                                            ((x) + 0x1301c)
#define HWIO_LPASS_AUDIO_CORE_LPAIF_QUAD_EBIT_CBCR_RMSK                                                               0x80000003
#define HWIO_LPASS_AUDIO_CORE_LPAIF_QUAD_EBIT_CBCR_IN(x)            \
                in_dword(HWIO_LPASS_AUDIO_CORE_LPAIF_QUAD_EBIT_CBCR_ADDR(x))
#define HWIO_LPASS_AUDIO_CORE_LPAIF_QUAD_EBIT_CBCR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AUDIO_CORE_LPAIF_QUAD_EBIT_CBCR_ADDR(x), m)
#define HWIO_LPASS_AUDIO_CORE_LPAIF_QUAD_EBIT_CBCR_OUT(x, v)            \
                out_dword(HWIO_LPASS_AUDIO_CORE_LPAIF_QUAD_EBIT_CBCR_ADDR(x),v)
#define HWIO_LPASS_AUDIO_CORE_LPAIF_QUAD_EBIT_CBCR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AUDIO_CORE_LPAIF_QUAD_EBIT_CBCR_ADDR(x),m,v,HWIO_LPASS_AUDIO_CORE_LPAIF_QUAD_EBIT_CBCR_IN(x))
#define HWIO_LPASS_AUDIO_CORE_LPAIF_QUAD_EBIT_CBCR_CLK_OFF_BMSK                                                       0x80000000
#define HWIO_LPASS_AUDIO_CORE_LPAIF_QUAD_EBIT_CBCR_CLK_OFF_SHFT                                                               31
#define HWIO_LPASS_AUDIO_CORE_LPAIF_QUAD_EBIT_CBCR_HW_CTL_BMSK                                                               0x2
#define HWIO_LPASS_AUDIO_CORE_LPAIF_QUAD_EBIT_CBCR_HW_CTL_SHFT                                                                 1
#define HWIO_LPASS_AUDIO_CORE_LPAIF_QUAD_EBIT_CBCR_CLK_ENABLE_BMSK                                                           0x1
#define HWIO_LPASS_AUDIO_CORE_LPAIF_QUAD_EBIT_CBCR_CLK_ENABLE_SHFT                                                             0

#define HWIO_LPASS_AUDIO_CORE_LPAIF_QUAD_CLK_INV_ADDR(x)                                                              ((x) + 0x13020)
#define HWIO_LPASS_AUDIO_CORE_LPAIF_QUAD_CLK_INV_RMSK                                                                        0x3
#define HWIO_LPASS_AUDIO_CORE_LPAIF_QUAD_CLK_INV_IN(x)            \
                in_dword(HWIO_LPASS_AUDIO_CORE_LPAIF_QUAD_CLK_INV_ADDR(x))
#define HWIO_LPASS_AUDIO_CORE_LPAIF_QUAD_CLK_INV_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AUDIO_CORE_LPAIF_QUAD_CLK_INV_ADDR(x), m)
#define HWIO_LPASS_AUDIO_CORE_LPAIF_QUAD_CLK_INV_OUT(x, v)            \
                out_dword(HWIO_LPASS_AUDIO_CORE_LPAIF_QUAD_CLK_INV_ADDR(x),v)
#define HWIO_LPASS_AUDIO_CORE_LPAIF_QUAD_CLK_INV_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AUDIO_CORE_LPAIF_QUAD_CLK_INV_ADDR(x),m,v,HWIO_LPASS_AUDIO_CORE_LPAIF_QUAD_CLK_INV_IN(x))
#define HWIO_LPASS_AUDIO_CORE_LPAIF_QUAD_CLK_INV_INV_EXT_CLK_BMSK                                                            0x2
#define HWIO_LPASS_AUDIO_CORE_LPAIF_QUAD_CLK_INV_INV_EXT_CLK_SHFT                                                              1
#define HWIO_LPASS_AUDIO_CORE_LPAIF_QUAD_CLK_INV_INV_INT_CLK_BMSK                                                            0x1
#define HWIO_LPASS_AUDIO_CORE_LPAIF_QUAD_CLK_INV_INV_INT_CLK_SHFT                                                              0

#define HWIO_LPASS_AUDIO_CORE_LPAIF_QUAD_EXT_CLK_DETECT_ADDR(x)                                                       ((x) + 0x13024)
#define HWIO_LPASS_AUDIO_CORE_LPAIF_QUAD_EXT_CLK_DETECT_RMSK                                                          0x80007f13
#define HWIO_LPASS_AUDIO_CORE_LPAIF_QUAD_EXT_CLK_DETECT_IN(x)            \
                in_dword(HWIO_LPASS_AUDIO_CORE_LPAIF_QUAD_EXT_CLK_DETECT_ADDR(x))
#define HWIO_LPASS_AUDIO_CORE_LPAIF_QUAD_EXT_CLK_DETECT_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AUDIO_CORE_LPAIF_QUAD_EXT_CLK_DETECT_ADDR(x), m)
#define HWIO_LPASS_AUDIO_CORE_LPAIF_QUAD_EXT_CLK_DETECT_OUT(x, v)            \
                out_dword(HWIO_LPASS_AUDIO_CORE_LPAIF_QUAD_EXT_CLK_DETECT_ADDR(x),v)
#define HWIO_LPASS_AUDIO_CORE_LPAIF_QUAD_EXT_CLK_DETECT_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AUDIO_CORE_LPAIF_QUAD_EXT_CLK_DETECT_ADDR(x),m,v,HWIO_LPASS_AUDIO_CORE_LPAIF_QUAD_EXT_CLK_DETECT_IN(x))
#define HWIO_LPASS_AUDIO_CORE_LPAIF_QUAD_EXT_CLK_DETECT_CLK_INACTIVE_IRQ_BMSK                                         0x80000000
#define HWIO_LPASS_AUDIO_CORE_LPAIF_QUAD_EXT_CLK_DETECT_CLK_INACTIVE_IRQ_SHFT                                                 31
#define HWIO_LPASS_AUDIO_CORE_LPAIF_QUAD_EXT_CLK_DETECT_CLK_COUNTER_VALUE_BMSK                                            0x7f00
#define HWIO_LPASS_AUDIO_CORE_LPAIF_QUAD_EXT_CLK_DETECT_CLK_COUNTER_VALUE_SHFT                                                 8
#define HWIO_LPASS_AUDIO_CORE_LPAIF_QUAD_EXT_CLK_DETECT_IRQ_CLEAR_BMSK                                                      0x10
#define HWIO_LPASS_AUDIO_CORE_LPAIF_QUAD_EXT_CLK_DETECT_IRQ_CLEAR_SHFT                                                         4
#define HWIO_LPASS_AUDIO_CORE_LPAIF_QUAD_EXT_CLK_DETECT_CLK_COUNTER_EN_BMSK                                                  0x2
#define HWIO_LPASS_AUDIO_CORE_LPAIF_QUAD_EXT_CLK_DETECT_CLK_COUNTER_EN_SHFT                                                    1
#define HWIO_LPASS_AUDIO_CORE_LPAIF_QUAD_EXT_CLK_DETECT_IRQ_EN_BMSK                                                          0x1
#define HWIO_LPASS_AUDIO_CORE_LPAIF_QUAD_EXT_CLK_DETECT_IRQ_EN_SHFT                                                            0

#define HWIO_LPASS_AON_CMD_RCGR_ADDR(x)                                                                               ((x) + 0x14000)
#define HWIO_LPASS_AON_CMD_RCGR_RMSK                                                                                  0x800000f3
#define HWIO_LPASS_AON_CMD_RCGR_IN(x)            \
                in_dword(HWIO_LPASS_AON_CMD_RCGR_ADDR(x))
#define HWIO_LPASS_AON_CMD_RCGR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AON_CMD_RCGR_ADDR(x), m)
#define HWIO_LPASS_AON_CMD_RCGR_OUT(x, v)            \
                out_dword(HWIO_LPASS_AON_CMD_RCGR_ADDR(x),v)
#define HWIO_LPASS_AON_CMD_RCGR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AON_CMD_RCGR_ADDR(x),m,v,HWIO_LPASS_AON_CMD_RCGR_IN(x))
#define HWIO_LPASS_AON_CMD_RCGR_ROOT_OFF_BMSK                                                                         0x80000000
#define HWIO_LPASS_AON_CMD_RCGR_ROOT_OFF_SHFT                                                                                 31
#define HWIO_LPASS_AON_CMD_RCGR_DIRTY_D_BMSK                                                                                0x80
#define HWIO_LPASS_AON_CMD_RCGR_DIRTY_D_SHFT                                                                                   7
#define HWIO_LPASS_AON_CMD_RCGR_DIRTY_N_BMSK                                                                                0x40
#define HWIO_LPASS_AON_CMD_RCGR_DIRTY_N_SHFT                                                                                   6
#define HWIO_LPASS_AON_CMD_RCGR_DIRTY_M_BMSK                                                                                0x20
#define HWIO_LPASS_AON_CMD_RCGR_DIRTY_M_SHFT                                                                                   5
#define HWIO_LPASS_AON_CMD_RCGR_DIRTY_CFG_RCGR_BMSK                                                                         0x10
#define HWIO_LPASS_AON_CMD_RCGR_DIRTY_CFG_RCGR_SHFT                                                                            4
#define HWIO_LPASS_AON_CMD_RCGR_ROOT_EN_BMSK                                                                                 0x2
#define HWIO_LPASS_AON_CMD_RCGR_ROOT_EN_SHFT                                                                                   1
#define HWIO_LPASS_AON_CMD_RCGR_UPDATE_BMSK                                                                                  0x1
#define HWIO_LPASS_AON_CMD_RCGR_UPDATE_SHFT                                                                                    0

#define HWIO_LPASS_AON_CFG_RCGR_ADDR(x)                                                                               ((x) + 0x14004)
#define HWIO_LPASS_AON_CFG_RCGR_RMSK                                                                                      0x771f
#define HWIO_LPASS_AON_CFG_RCGR_IN(x)            \
                in_dword(HWIO_LPASS_AON_CFG_RCGR_ADDR(x))
#define HWIO_LPASS_AON_CFG_RCGR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AON_CFG_RCGR_ADDR(x), m)
#define HWIO_LPASS_AON_CFG_RCGR_OUT(x, v)            \
                out_dword(HWIO_LPASS_AON_CFG_RCGR_ADDR(x),v)
#define HWIO_LPASS_AON_CFG_RCGR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AON_CFG_RCGR_ADDR(x),m,v,HWIO_LPASS_AON_CFG_RCGR_IN(x))
#define HWIO_LPASS_AON_CFG_RCGR_ALT_SRC_SEL_BMSK                                                                          0x4000
#define HWIO_LPASS_AON_CFG_RCGR_ALT_SRC_SEL_SHFT                                                                              14
#define HWIO_LPASS_AON_CFG_RCGR_MODE_BMSK                                                                                 0x3000
#define HWIO_LPASS_AON_CFG_RCGR_MODE_SHFT                                                                                     12
#define HWIO_LPASS_AON_CFG_RCGR_SRC_SEL_BMSK                                                                               0x700
#define HWIO_LPASS_AON_CFG_RCGR_SRC_SEL_SHFT                                                                                   8
#define HWIO_LPASS_AON_CFG_RCGR_SRC_DIV_BMSK                                                                                0x1f
#define HWIO_LPASS_AON_CFG_RCGR_SRC_DIV_SHFT                                                                                   0

#define HWIO_LPASS_AON_M_ADDR(x)                                                                                      ((x) + 0x14008)
#define HWIO_LPASS_AON_M_RMSK                                                                                               0xff
#define HWIO_LPASS_AON_M_IN(x)            \
                in_dword(HWIO_LPASS_AON_M_ADDR(x))
#define HWIO_LPASS_AON_M_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AON_M_ADDR(x), m)
#define HWIO_LPASS_AON_M_OUT(x, v)            \
                out_dword(HWIO_LPASS_AON_M_ADDR(x),v)
#define HWIO_LPASS_AON_M_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AON_M_ADDR(x),m,v,HWIO_LPASS_AON_M_IN(x))
#define HWIO_LPASS_AON_M_M_BMSK                                                                                             0xff
#define HWIO_LPASS_AON_M_M_SHFT                                                                                                0

#define HWIO_LPASS_AON_N_ADDR(x)                                                                                      ((x) + 0x1400c)
#define HWIO_LPASS_AON_N_RMSK                                                                                               0xff
#define HWIO_LPASS_AON_N_IN(x)            \
                in_dword(HWIO_LPASS_AON_N_ADDR(x))
#define HWIO_LPASS_AON_N_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AON_N_ADDR(x), m)
#define HWIO_LPASS_AON_N_OUT(x, v)            \
                out_dword(HWIO_LPASS_AON_N_ADDR(x),v)
#define HWIO_LPASS_AON_N_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AON_N_ADDR(x),m,v,HWIO_LPASS_AON_N_IN(x))
#define HWIO_LPASS_AON_N_NOT_N_MINUS_M_BMSK                                                                                 0xff
#define HWIO_LPASS_AON_N_NOT_N_MINUS_M_SHFT                                                                                    0

#define HWIO_LPASS_AON_D_ADDR(x)                                                                                      ((x) + 0x14010)
#define HWIO_LPASS_AON_D_RMSK                                                                                               0xff
#define HWIO_LPASS_AON_D_IN(x)            \
                in_dword(HWIO_LPASS_AON_D_ADDR(x))
#define HWIO_LPASS_AON_D_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AON_D_ADDR(x), m)
#define HWIO_LPASS_AON_D_OUT(x, v)            \
                out_dword(HWIO_LPASS_AON_D_ADDR(x),v)
#define HWIO_LPASS_AON_D_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AON_D_ADDR(x),m,v,HWIO_LPASS_AON_D_IN(x))
#define HWIO_LPASS_AON_D_NOT_2D_BMSK                                                                                        0xff
#define HWIO_LPASS_AON_D_NOT_2D_SHFT                                                                                           0

#define HWIO_LPASS_AON_CMD_DFSR_ADDR(x)                                                                               ((x) + 0x14014)
#define HWIO_LPASS_AON_CMD_DFSR_RMSK                                                                                      0xbfef
#define HWIO_LPASS_AON_CMD_DFSR_IN(x)            \
                in_dword(HWIO_LPASS_AON_CMD_DFSR_ADDR(x))
#define HWIO_LPASS_AON_CMD_DFSR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AON_CMD_DFSR_ADDR(x), m)
#define HWIO_LPASS_AON_CMD_DFSR_OUT(x, v)            \
                out_dword(HWIO_LPASS_AON_CMD_DFSR_ADDR(x),v)
#define HWIO_LPASS_AON_CMD_DFSR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AON_CMD_DFSR_ADDR(x),m,v,HWIO_LPASS_AON_CMD_DFSR_IN(x))
#define HWIO_LPASS_AON_CMD_DFSR_RCG_SW_CTRL_BMSK                                                                          0x8000
#define HWIO_LPASS_AON_CMD_DFSR_RCG_SW_CTRL_SHFT                                                                              15
#define HWIO_LPASS_AON_CMD_DFSR_SW_PERF_STATE_BMSK                                                                        0x3800
#define HWIO_LPASS_AON_CMD_DFSR_SW_PERF_STATE_SHFT                                                                            11
#define HWIO_LPASS_AON_CMD_DFSR_SW_OVERRIDE_BMSK                                                                           0x400
#define HWIO_LPASS_AON_CMD_DFSR_SW_OVERRIDE_SHFT                                                                              10
#define HWIO_LPASS_AON_CMD_DFSR_PERF_STATE_UPDATE_STATUS_BMSK                                                              0x200
#define HWIO_LPASS_AON_CMD_DFSR_PERF_STATE_UPDATE_STATUS_SHFT                                                                  9
#define HWIO_LPASS_AON_CMD_DFSR_DFS_FSM_STATE_BMSK                                                                         0x1c0
#define HWIO_LPASS_AON_CMD_DFSR_DFS_FSM_STATE_SHFT                                                                             6
#define HWIO_LPASS_AON_CMD_DFSR_HW_CLK_CONTROL_BMSK                                                                         0x20
#define HWIO_LPASS_AON_CMD_DFSR_HW_CLK_CONTROL_SHFT                                                                            5
#define HWIO_LPASS_AON_CMD_DFSR_CURR_PERF_STATE_BMSK                                                                         0xe
#define HWIO_LPASS_AON_CMD_DFSR_CURR_PERF_STATE_SHFT                                                                           1
#define HWIO_LPASS_AON_CMD_DFSR_DFS_EN_BMSK                                                                                  0x1
#define HWIO_LPASS_AON_CMD_DFSR_DFS_EN_SHFT                                                                                    0

#define HWIO_LPASS_AON_PERF0_DFSR_ADDR(x)                                                                             ((x) + 0x14018)
#define HWIO_LPASS_AON_PERF0_DFSR_RMSK                                                                                    0x371f
#define HWIO_LPASS_AON_PERF0_DFSR_IN(x)            \
                in_dword(HWIO_LPASS_AON_PERF0_DFSR_ADDR(x))
#define HWIO_LPASS_AON_PERF0_DFSR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AON_PERF0_DFSR_ADDR(x), m)
#define HWIO_LPASS_AON_PERF0_DFSR_OUT(x, v)            \
                out_dword(HWIO_LPASS_AON_PERF0_DFSR_ADDR(x),v)
#define HWIO_LPASS_AON_PERF0_DFSR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AON_PERF0_DFSR_ADDR(x),m,v,HWIO_LPASS_AON_PERF0_DFSR_IN(x))
#define HWIO_LPASS_AON_PERF0_DFSR_MODE_BMSK                                                                               0x3000
#define HWIO_LPASS_AON_PERF0_DFSR_MODE_SHFT                                                                                   12
#define HWIO_LPASS_AON_PERF0_DFSR_SRC_SEL_BMSK                                                                             0x700
#define HWIO_LPASS_AON_PERF0_DFSR_SRC_SEL_SHFT                                                                                 8
#define HWIO_LPASS_AON_PERF0_DFSR_SRC_DIV_BMSK                                                                              0x1f
#define HWIO_LPASS_AON_PERF0_DFSR_SRC_DIV_SHFT                                                                                 0

#define HWIO_LPASS_AON_PERF1_DFSR_ADDR(x)                                                                             ((x) + 0x1401c)
#define HWIO_LPASS_AON_PERF1_DFSR_RMSK                                                                                    0x371f
#define HWIO_LPASS_AON_PERF1_DFSR_IN(x)            \
                in_dword(HWIO_LPASS_AON_PERF1_DFSR_ADDR(x))
#define HWIO_LPASS_AON_PERF1_DFSR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AON_PERF1_DFSR_ADDR(x), m)
#define HWIO_LPASS_AON_PERF1_DFSR_OUT(x, v)            \
                out_dword(HWIO_LPASS_AON_PERF1_DFSR_ADDR(x),v)
#define HWIO_LPASS_AON_PERF1_DFSR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AON_PERF1_DFSR_ADDR(x),m,v,HWIO_LPASS_AON_PERF1_DFSR_IN(x))
#define HWIO_LPASS_AON_PERF1_DFSR_MODE_BMSK                                                                               0x3000
#define HWIO_LPASS_AON_PERF1_DFSR_MODE_SHFT                                                                                   12
#define HWIO_LPASS_AON_PERF1_DFSR_SRC_SEL_BMSK                                                                             0x700
#define HWIO_LPASS_AON_PERF1_DFSR_SRC_SEL_SHFT                                                                                 8
#define HWIO_LPASS_AON_PERF1_DFSR_SRC_DIV_BMSK                                                                              0x1f
#define HWIO_LPASS_AON_PERF1_DFSR_SRC_DIV_SHFT                                                                                 0

#define HWIO_LPASS_AON_PERF2_DFSR_ADDR(x)                                                                             ((x) + 0x14020)
#define HWIO_LPASS_AON_PERF2_DFSR_RMSK                                                                                    0x371f
#define HWIO_LPASS_AON_PERF2_DFSR_IN(x)            \
                in_dword(HWIO_LPASS_AON_PERF2_DFSR_ADDR(x))
#define HWIO_LPASS_AON_PERF2_DFSR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AON_PERF2_DFSR_ADDR(x), m)
#define HWIO_LPASS_AON_PERF2_DFSR_OUT(x, v)            \
                out_dword(HWIO_LPASS_AON_PERF2_DFSR_ADDR(x),v)
#define HWIO_LPASS_AON_PERF2_DFSR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AON_PERF2_DFSR_ADDR(x),m,v,HWIO_LPASS_AON_PERF2_DFSR_IN(x))
#define HWIO_LPASS_AON_PERF2_DFSR_MODE_BMSK                                                                               0x3000
#define HWIO_LPASS_AON_PERF2_DFSR_MODE_SHFT                                                                                   12
#define HWIO_LPASS_AON_PERF2_DFSR_SRC_SEL_BMSK                                                                             0x700
#define HWIO_LPASS_AON_PERF2_DFSR_SRC_SEL_SHFT                                                                                 8
#define HWIO_LPASS_AON_PERF2_DFSR_SRC_DIV_BMSK                                                                              0x1f
#define HWIO_LPASS_AON_PERF2_DFSR_SRC_DIV_SHFT                                                                                 0

#define HWIO_LPASS_AON_PERF3_DFSR_ADDR(x)                                                                             ((x) + 0x14024)
#define HWIO_LPASS_AON_PERF3_DFSR_RMSK                                                                                    0x371f
#define HWIO_LPASS_AON_PERF3_DFSR_IN(x)            \
                in_dword(HWIO_LPASS_AON_PERF3_DFSR_ADDR(x))
#define HWIO_LPASS_AON_PERF3_DFSR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AON_PERF3_DFSR_ADDR(x), m)
#define HWIO_LPASS_AON_PERF3_DFSR_OUT(x, v)            \
                out_dword(HWIO_LPASS_AON_PERF3_DFSR_ADDR(x),v)
#define HWIO_LPASS_AON_PERF3_DFSR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AON_PERF3_DFSR_ADDR(x),m,v,HWIO_LPASS_AON_PERF3_DFSR_IN(x))
#define HWIO_LPASS_AON_PERF3_DFSR_MODE_BMSK                                                                               0x3000
#define HWIO_LPASS_AON_PERF3_DFSR_MODE_SHFT                                                                                   12
#define HWIO_LPASS_AON_PERF3_DFSR_SRC_SEL_BMSK                                                                             0x700
#define HWIO_LPASS_AON_PERF3_DFSR_SRC_SEL_SHFT                                                                                 8
#define HWIO_LPASS_AON_PERF3_DFSR_SRC_DIV_BMSK                                                                              0x1f
#define HWIO_LPASS_AON_PERF3_DFSR_SRC_DIV_SHFT                                                                                 0

#define HWIO_LPASS_AON_PERF4_DFSR_ADDR(x)                                                                             ((x) + 0x14028)
#define HWIO_LPASS_AON_PERF4_DFSR_RMSK                                                                                    0x371f
#define HWIO_LPASS_AON_PERF4_DFSR_IN(x)            \
                in_dword(HWIO_LPASS_AON_PERF4_DFSR_ADDR(x))
#define HWIO_LPASS_AON_PERF4_DFSR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AON_PERF4_DFSR_ADDR(x), m)
#define HWIO_LPASS_AON_PERF4_DFSR_OUT(x, v)            \
                out_dword(HWIO_LPASS_AON_PERF4_DFSR_ADDR(x),v)
#define HWIO_LPASS_AON_PERF4_DFSR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AON_PERF4_DFSR_ADDR(x),m,v,HWIO_LPASS_AON_PERF4_DFSR_IN(x))
#define HWIO_LPASS_AON_PERF4_DFSR_MODE_BMSK                                                                               0x3000
#define HWIO_LPASS_AON_PERF4_DFSR_MODE_SHFT                                                                                   12
#define HWIO_LPASS_AON_PERF4_DFSR_SRC_SEL_BMSK                                                                             0x700
#define HWIO_LPASS_AON_PERF4_DFSR_SRC_SEL_SHFT                                                                                 8
#define HWIO_LPASS_AON_PERF4_DFSR_SRC_DIV_BMSK                                                                              0x1f
#define HWIO_LPASS_AON_PERF4_DFSR_SRC_DIV_SHFT                                                                                 0

#define HWIO_LPASS_AON_PERF5_DFSR_ADDR(x)                                                                             ((x) + 0x1402c)
#define HWIO_LPASS_AON_PERF5_DFSR_RMSK                                                                                    0x371f
#define HWIO_LPASS_AON_PERF5_DFSR_IN(x)            \
                in_dword(HWIO_LPASS_AON_PERF5_DFSR_ADDR(x))
#define HWIO_LPASS_AON_PERF5_DFSR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AON_PERF5_DFSR_ADDR(x), m)
#define HWIO_LPASS_AON_PERF5_DFSR_OUT(x, v)            \
                out_dword(HWIO_LPASS_AON_PERF5_DFSR_ADDR(x),v)
#define HWIO_LPASS_AON_PERF5_DFSR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AON_PERF5_DFSR_ADDR(x),m,v,HWIO_LPASS_AON_PERF5_DFSR_IN(x))
#define HWIO_LPASS_AON_PERF5_DFSR_MODE_BMSK                                                                               0x3000
#define HWIO_LPASS_AON_PERF5_DFSR_MODE_SHFT                                                                                   12
#define HWIO_LPASS_AON_PERF5_DFSR_SRC_SEL_BMSK                                                                             0x700
#define HWIO_LPASS_AON_PERF5_DFSR_SRC_SEL_SHFT                                                                                 8
#define HWIO_LPASS_AON_PERF5_DFSR_SRC_DIV_BMSK                                                                              0x1f
#define HWIO_LPASS_AON_PERF5_DFSR_SRC_DIV_SHFT                                                                                 0

#define HWIO_LPASS_AON_PERF6_DFSR_ADDR(x)                                                                             ((x) + 0x14030)
#define HWIO_LPASS_AON_PERF6_DFSR_RMSK                                                                                    0x371f
#define HWIO_LPASS_AON_PERF6_DFSR_IN(x)            \
                in_dword(HWIO_LPASS_AON_PERF6_DFSR_ADDR(x))
#define HWIO_LPASS_AON_PERF6_DFSR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AON_PERF6_DFSR_ADDR(x), m)
#define HWIO_LPASS_AON_PERF6_DFSR_OUT(x, v)            \
                out_dword(HWIO_LPASS_AON_PERF6_DFSR_ADDR(x),v)
#define HWIO_LPASS_AON_PERF6_DFSR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AON_PERF6_DFSR_ADDR(x),m,v,HWIO_LPASS_AON_PERF6_DFSR_IN(x))
#define HWIO_LPASS_AON_PERF6_DFSR_MODE_BMSK                                                                               0x3000
#define HWIO_LPASS_AON_PERF6_DFSR_MODE_SHFT                                                                                   12
#define HWIO_LPASS_AON_PERF6_DFSR_SRC_SEL_BMSK                                                                             0x700
#define HWIO_LPASS_AON_PERF6_DFSR_SRC_SEL_SHFT                                                                                 8
#define HWIO_LPASS_AON_PERF6_DFSR_SRC_DIV_BMSK                                                                              0x1f
#define HWIO_LPASS_AON_PERF6_DFSR_SRC_DIV_SHFT                                                                                 0

#define HWIO_LPASS_AON_PERF7_DFSR_ADDR(x)                                                                             ((x) + 0x14034)
#define HWIO_LPASS_AON_PERF7_DFSR_RMSK                                                                                    0x371f
#define HWIO_LPASS_AON_PERF7_DFSR_IN(x)            \
                in_dword(HWIO_LPASS_AON_PERF7_DFSR_ADDR(x))
#define HWIO_LPASS_AON_PERF7_DFSR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AON_PERF7_DFSR_ADDR(x), m)
#define HWIO_LPASS_AON_PERF7_DFSR_OUT(x, v)            \
                out_dword(HWIO_LPASS_AON_PERF7_DFSR_ADDR(x),v)
#define HWIO_LPASS_AON_PERF7_DFSR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AON_PERF7_DFSR_ADDR(x),m,v,HWIO_LPASS_AON_PERF7_DFSR_IN(x))
#define HWIO_LPASS_AON_PERF7_DFSR_MODE_BMSK                                                                               0x3000
#define HWIO_LPASS_AON_PERF7_DFSR_MODE_SHFT                                                                                   12
#define HWIO_LPASS_AON_PERF7_DFSR_SRC_SEL_BMSK                                                                             0x700
#define HWIO_LPASS_AON_PERF7_DFSR_SRC_SEL_SHFT                                                                                 8
#define HWIO_LPASS_AON_PERF7_DFSR_SRC_DIV_BMSK                                                                              0x1f
#define HWIO_LPASS_AON_PERF7_DFSR_SRC_DIV_SHFT                                                                                 0

#define HWIO_LPASS_AON_PERF0_M_DFSR_ADDR(x)                                                                           ((x) + 0x14038)
#define HWIO_LPASS_AON_PERF0_M_DFSR_RMSK                                                                                    0xff
#define HWIO_LPASS_AON_PERF0_M_DFSR_IN(x)            \
                in_dword(HWIO_LPASS_AON_PERF0_M_DFSR_ADDR(x))
#define HWIO_LPASS_AON_PERF0_M_DFSR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AON_PERF0_M_DFSR_ADDR(x), m)
#define HWIO_LPASS_AON_PERF0_M_DFSR_OUT(x, v)            \
                out_dword(HWIO_LPASS_AON_PERF0_M_DFSR_ADDR(x),v)
#define HWIO_LPASS_AON_PERF0_M_DFSR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AON_PERF0_M_DFSR_ADDR(x),m,v,HWIO_LPASS_AON_PERF0_M_DFSR_IN(x))
#define HWIO_LPASS_AON_PERF0_M_DFSR_M_BMSK                                                                                  0xff
#define HWIO_LPASS_AON_PERF0_M_DFSR_M_SHFT                                                                                     0

#define HWIO_LPASS_AON_PERF1_M_DFSR_ADDR(x)                                                                           ((x) + 0x1403c)
#define HWIO_LPASS_AON_PERF1_M_DFSR_RMSK                                                                                    0xff
#define HWIO_LPASS_AON_PERF1_M_DFSR_IN(x)            \
                in_dword(HWIO_LPASS_AON_PERF1_M_DFSR_ADDR(x))
#define HWIO_LPASS_AON_PERF1_M_DFSR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AON_PERF1_M_DFSR_ADDR(x), m)
#define HWIO_LPASS_AON_PERF1_M_DFSR_OUT(x, v)            \
                out_dword(HWIO_LPASS_AON_PERF1_M_DFSR_ADDR(x),v)
#define HWIO_LPASS_AON_PERF1_M_DFSR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AON_PERF1_M_DFSR_ADDR(x),m,v,HWIO_LPASS_AON_PERF1_M_DFSR_IN(x))
#define HWIO_LPASS_AON_PERF1_M_DFSR_M_BMSK                                                                                  0xff
#define HWIO_LPASS_AON_PERF1_M_DFSR_M_SHFT                                                                                     0

#define HWIO_LPASS_AON_PERF2_M_DFSR_ADDR(x)                                                                           ((x) + 0x14040)
#define HWIO_LPASS_AON_PERF2_M_DFSR_RMSK                                                                                    0xff
#define HWIO_LPASS_AON_PERF2_M_DFSR_IN(x)            \
                in_dword(HWIO_LPASS_AON_PERF2_M_DFSR_ADDR(x))
#define HWIO_LPASS_AON_PERF2_M_DFSR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AON_PERF2_M_DFSR_ADDR(x), m)
#define HWIO_LPASS_AON_PERF2_M_DFSR_OUT(x, v)            \
                out_dword(HWIO_LPASS_AON_PERF2_M_DFSR_ADDR(x),v)
#define HWIO_LPASS_AON_PERF2_M_DFSR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AON_PERF2_M_DFSR_ADDR(x),m,v,HWIO_LPASS_AON_PERF2_M_DFSR_IN(x))
#define HWIO_LPASS_AON_PERF2_M_DFSR_M_BMSK                                                                                  0xff
#define HWIO_LPASS_AON_PERF2_M_DFSR_M_SHFT                                                                                     0

#define HWIO_LPASS_AON_PERF3_M_DFSR_ADDR(x)                                                                           ((x) + 0x14044)
#define HWIO_LPASS_AON_PERF3_M_DFSR_RMSK                                                                                    0xff
#define HWIO_LPASS_AON_PERF3_M_DFSR_IN(x)            \
                in_dword(HWIO_LPASS_AON_PERF3_M_DFSR_ADDR(x))
#define HWIO_LPASS_AON_PERF3_M_DFSR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AON_PERF3_M_DFSR_ADDR(x), m)
#define HWIO_LPASS_AON_PERF3_M_DFSR_OUT(x, v)            \
                out_dword(HWIO_LPASS_AON_PERF3_M_DFSR_ADDR(x),v)
#define HWIO_LPASS_AON_PERF3_M_DFSR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AON_PERF3_M_DFSR_ADDR(x),m,v,HWIO_LPASS_AON_PERF3_M_DFSR_IN(x))
#define HWIO_LPASS_AON_PERF3_M_DFSR_M_BMSK                                                                                  0xff
#define HWIO_LPASS_AON_PERF3_M_DFSR_M_SHFT                                                                                     0

#define HWIO_LPASS_AON_PERF4_M_DFSR_ADDR(x)                                                                           ((x) + 0x14048)
#define HWIO_LPASS_AON_PERF4_M_DFSR_RMSK                                                                                    0xff
#define HWIO_LPASS_AON_PERF4_M_DFSR_IN(x)            \
                in_dword(HWIO_LPASS_AON_PERF4_M_DFSR_ADDR(x))
#define HWIO_LPASS_AON_PERF4_M_DFSR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AON_PERF4_M_DFSR_ADDR(x), m)
#define HWIO_LPASS_AON_PERF4_M_DFSR_OUT(x, v)            \
                out_dword(HWIO_LPASS_AON_PERF4_M_DFSR_ADDR(x),v)
#define HWIO_LPASS_AON_PERF4_M_DFSR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AON_PERF4_M_DFSR_ADDR(x),m,v,HWIO_LPASS_AON_PERF4_M_DFSR_IN(x))
#define HWIO_LPASS_AON_PERF4_M_DFSR_M_BMSK                                                                                  0xff
#define HWIO_LPASS_AON_PERF4_M_DFSR_M_SHFT                                                                                     0

#define HWIO_LPASS_AON_PERF5_M_DFSR_ADDR(x)                                                                           ((x) + 0x1404c)
#define HWIO_LPASS_AON_PERF5_M_DFSR_RMSK                                                                                    0xff
#define HWIO_LPASS_AON_PERF5_M_DFSR_IN(x)            \
                in_dword(HWIO_LPASS_AON_PERF5_M_DFSR_ADDR(x))
#define HWIO_LPASS_AON_PERF5_M_DFSR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AON_PERF5_M_DFSR_ADDR(x), m)
#define HWIO_LPASS_AON_PERF5_M_DFSR_OUT(x, v)            \
                out_dword(HWIO_LPASS_AON_PERF5_M_DFSR_ADDR(x),v)
#define HWIO_LPASS_AON_PERF5_M_DFSR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AON_PERF5_M_DFSR_ADDR(x),m,v,HWIO_LPASS_AON_PERF5_M_DFSR_IN(x))
#define HWIO_LPASS_AON_PERF5_M_DFSR_M_BMSK                                                                                  0xff
#define HWIO_LPASS_AON_PERF5_M_DFSR_M_SHFT                                                                                     0

#define HWIO_LPASS_AON_PERF6_M_DFSR_ADDR(x)                                                                           ((x) + 0x14050)
#define HWIO_LPASS_AON_PERF6_M_DFSR_RMSK                                                                                    0xff
#define HWIO_LPASS_AON_PERF6_M_DFSR_IN(x)            \
                in_dword(HWIO_LPASS_AON_PERF6_M_DFSR_ADDR(x))
#define HWIO_LPASS_AON_PERF6_M_DFSR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AON_PERF6_M_DFSR_ADDR(x), m)
#define HWIO_LPASS_AON_PERF6_M_DFSR_OUT(x, v)            \
                out_dword(HWIO_LPASS_AON_PERF6_M_DFSR_ADDR(x),v)
#define HWIO_LPASS_AON_PERF6_M_DFSR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AON_PERF6_M_DFSR_ADDR(x),m,v,HWIO_LPASS_AON_PERF6_M_DFSR_IN(x))
#define HWIO_LPASS_AON_PERF6_M_DFSR_M_BMSK                                                                                  0xff
#define HWIO_LPASS_AON_PERF6_M_DFSR_M_SHFT                                                                                     0

#define HWIO_LPASS_AON_PERF7_M_DFSR_ADDR(x)                                                                           ((x) + 0x14054)
#define HWIO_LPASS_AON_PERF7_M_DFSR_RMSK                                                                                    0xff
#define HWIO_LPASS_AON_PERF7_M_DFSR_IN(x)            \
                in_dword(HWIO_LPASS_AON_PERF7_M_DFSR_ADDR(x))
#define HWIO_LPASS_AON_PERF7_M_DFSR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AON_PERF7_M_DFSR_ADDR(x), m)
#define HWIO_LPASS_AON_PERF7_M_DFSR_OUT(x, v)            \
                out_dword(HWIO_LPASS_AON_PERF7_M_DFSR_ADDR(x),v)
#define HWIO_LPASS_AON_PERF7_M_DFSR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AON_PERF7_M_DFSR_ADDR(x),m,v,HWIO_LPASS_AON_PERF7_M_DFSR_IN(x))
#define HWIO_LPASS_AON_PERF7_M_DFSR_M_BMSK                                                                                  0xff
#define HWIO_LPASS_AON_PERF7_M_DFSR_M_SHFT                                                                                     0

#define HWIO_LPASS_AON_PERF0_N_DFSR_ADDR(x)                                                                           ((x) + 0x14058)
#define HWIO_LPASS_AON_PERF0_N_DFSR_RMSK                                                                                    0xff
#define HWIO_LPASS_AON_PERF0_N_DFSR_IN(x)            \
                in_dword(HWIO_LPASS_AON_PERF0_N_DFSR_ADDR(x))
#define HWIO_LPASS_AON_PERF0_N_DFSR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AON_PERF0_N_DFSR_ADDR(x), m)
#define HWIO_LPASS_AON_PERF0_N_DFSR_OUT(x, v)            \
                out_dword(HWIO_LPASS_AON_PERF0_N_DFSR_ADDR(x),v)
#define HWIO_LPASS_AON_PERF0_N_DFSR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AON_PERF0_N_DFSR_ADDR(x),m,v,HWIO_LPASS_AON_PERF0_N_DFSR_IN(x))
#define HWIO_LPASS_AON_PERF0_N_DFSR_NOT_N_MINUS_M_BMSK                                                                      0xff
#define HWIO_LPASS_AON_PERF0_N_DFSR_NOT_N_MINUS_M_SHFT                                                                         0

#define HWIO_LPASS_AON_PERF1_N_DFSR_ADDR(x)                                                                           ((x) + 0x1405c)
#define HWIO_LPASS_AON_PERF1_N_DFSR_RMSK                                                                                    0xff
#define HWIO_LPASS_AON_PERF1_N_DFSR_IN(x)            \
                in_dword(HWIO_LPASS_AON_PERF1_N_DFSR_ADDR(x))
#define HWIO_LPASS_AON_PERF1_N_DFSR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AON_PERF1_N_DFSR_ADDR(x), m)
#define HWIO_LPASS_AON_PERF1_N_DFSR_OUT(x, v)            \
                out_dword(HWIO_LPASS_AON_PERF1_N_DFSR_ADDR(x),v)
#define HWIO_LPASS_AON_PERF1_N_DFSR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AON_PERF1_N_DFSR_ADDR(x),m,v,HWIO_LPASS_AON_PERF1_N_DFSR_IN(x))
#define HWIO_LPASS_AON_PERF1_N_DFSR_NOT_N_MINUS_M_BMSK                                                                      0xff
#define HWIO_LPASS_AON_PERF1_N_DFSR_NOT_N_MINUS_M_SHFT                                                                         0

#define HWIO_LPASS_AON_PERF2_N_DFSR_ADDR(x)                                                                           ((x) + 0x14060)
#define HWIO_LPASS_AON_PERF2_N_DFSR_RMSK                                                                                    0xff
#define HWIO_LPASS_AON_PERF2_N_DFSR_IN(x)            \
                in_dword(HWIO_LPASS_AON_PERF2_N_DFSR_ADDR(x))
#define HWIO_LPASS_AON_PERF2_N_DFSR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AON_PERF2_N_DFSR_ADDR(x), m)
#define HWIO_LPASS_AON_PERF2_N_DFSR_OUT(x, v)            \
                out_dword(HWIO_LPASS_AON_PERF2_N_DFSR_ADDR(x),v)
#define HWIO_LPASS_AON_PERF2_N_DFSR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AON_PERF2_N_DFSR_ADDR(x),m,v,HWIO_LPASS_AON_PERF2_N_DFSR_IN(x))
#define HWIO_LPASS_AON_PERF2_N_DFSR_NOT_N_MINUS_M_BMSK                                                                      0xff
#define HWIO_LPASS_AON_PERF2_N_DFSR_NOT_N_MINUS_M_SHFT                                                                         0

#define HWIO_LPASS_AON_PERF3_N_DFSR_ADDR(x)                                                                           ((x) + 0x14064)
#define HWIO_LPASS_AON_PERF3_N_DFSR_RMSK                                                                                    0xff
#define HWIO_LPASS_AON_PERF3_N_DFSR_IN(x)            \
                in_dword(HWIO_LPASS_AON_PERF3_N_DFSR_ADDR(x))
#define HWIO_LPASS_AON_PERF3_N_DFSR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AON_PERF3_N_DFSR_ADDR(x), m)
#define HWIO_LPASS_AON_PERF3_N_DFSR_OUT(x, v)            \
                out_dword(HWIO_LPASS_AON_PERF3_N_DFSR_ADDR(x),v)
#define HWIO_LPASS_AON_PERF3_N_DFSR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AON_PERF3_N_DFSR_ADDR(x),m,v,HWIO_LPASS_AON_PERF3_N_DFSR_IN(x))
#define HWIO_LPASS_AON_PERF3_N_DFSR_NOT_N_MINUS_M_BMSK                                                                      0xff
#define HWIO_LPASS_AON_PERF3_N_DFSR_NOT_N_MINUS_M_SHFT                                                                         0

#define HWIO_LPASS_AON_PERF4_N_DFSR_ADDR(x)                                                                           ((x) + 0x14068)
#define HWIO_LPASS_AON_PERF4_N_DFSR_RMSK                                                                                    0xff
#define HWIO_LPASS_AON_PERF4_N_DFSR_IN(x)            \
                in_dword(HWIO_LPASS_AON_PERF4_N_DFSR_ADDR(x))
#define HWIO_LPASS_AON_PERF4_N_DFSR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AON_PERF4_N_DFSR_ADDR(x), m)
#define HWIO_LPASS_AON_PERF4_N_DFSR_OUT(x, v)            \
                out_dword(HWIO_LPASS_AON_PERF4_N_DFSR_ADDR(x),v)
#define HWIO_LPASS_AON_PERF4_N_DFSR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AON_PERF4_N_DFSR_ADDR(x),m,v,HWIO_LPASS_AON_PERF4_N_DFSR_IN(x))
#define HWIO_LPASS_AON_PERF4_N_DFSR_NOT_N_MINUS_M_BMSK                                                                      0xff
#define HWIO_LPASS_AON_PERF4_N_DFSR_NOT_N_MINUS_M_SHFT                                                                         0

#define HWIO_LPASS_AON_PERF5_N_DFSR_ADDR(x)                                                                           ((x) + 0x1406c)
#define HWIO_LPASS_AON_PERF5_N_DFSR_RMSK                                                                                    0xff
#define HWIO_LPASS_AON_PERF5_N_DFSR_IN(x)            \
                in_dword(HWIO_LPASS_AON_PERF5_N_DFSR_ADDR(x))
#define HWIO_LPASS_AON_PERF5_N_DFSR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AON_PERF5_N_DFSR_ADDR(x), m)
#define HWIO_LPASS_AON_PERF5_N_DFSR_OUT(x, v)            \
                out_dword(HWIO_LPASS_AON_PERF5_N_DFSR_ADDR(x),v)
#define HWIO_LPASS_AON_PERF5_N_DFSR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AON_PERF5_N_DFSR_ADDR(x),m,v,HWIO_LPASS_AON_PERF5_N_DFSR_IN(x))
#define HWIO_LPASS_AON_PERF5_N_DFSR_NOT_N_MINUS_M_BMSK                                                                      0xff
#define HWIO_LPASS_AON_PERF5_N_DFSR_NOT_N_MINUS_M_SHFT                                                                         0

#define HWIO_LPASS_AON_PERF6_N_DFSR_ADDR(x)                                                                           ((x) + 0x14070)
#define HWIO_LPASS_AON_PERF6_N_DFSR_RMSK                                                                                    0xff
#define HWIO_LPASS_AON_PERF6_N_DFSR_IN(x)            \
                in_dword(HWIO_LPASS_AON_PERF6_N_DFSR_ADDR(x))
#define HWIO_LPASS_AON_PERF6_N_DFSR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AON_PERF6_N_DFSR_ADDR(x), m)
#define HWIO_LPASS_AON_PERF6_N_DFSR_OUT(x, v)            \
                out_dword(HWIO_LPASS_AON_PERF6_N_DFSR_ADDR(x),v)
#define HWIO_LPASS_AON_PERF6_N_DFSR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AON_PERF6_N_DFSR_ADDR(x),m,v,HWIO_LPASS_AON_PERF6_N_DFSR_IN(x))
#define HWIO_LPASS_AON_PERF6_N_DFSR_NOT_N_MINUS_M_BMSK                                                                      0xff
#define HWIO_LPASS_AON_PERF6_N_DFSR_NOT_N_MINUS_M_SHFT                                                                         0

#define HWIO_LPASS_AON_PERF7_N_DFSR_ADDR(x)                                                                           ((x) + 0x14074)
#define HWIO_LPASS_AON_PERF7_N_DFSR_RMSK                                                                                    0xff
#define HWIO_LPASS_AON_PERF7_N_DFSR_IN(x)            \
                in_dword(HWIO_LPASS_AON_PERF7_N_DFSR_ADDR(x))
#define HWIO_LPASS_AON_PERF7_N_DFSR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AON_PERF7_N_DFSR_ADDR(x), m)
#define HWIO_LPASS_AON_PERF7_N_DFSR_OUT(x, v)            \
                out_dword(HWIO_LPASS_AON_PERF7_N_DFSR_ADDR(x),v)
#define HWIO_LPASS_AON_PERF7_N_DFSR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AON_PERF7_N_DFSR_ADDR(x),m,v,HWIO_LPASS_AON_PERF7_N_DFSR_IN(x))
#define HWIO_LPASS_AON_PERF7_N_DFSR_NOT_N_MINUS_M_BMSK                                                                      0xff
#define HWIO_LPASS_AON_PERF7_N_DFSR_NOT_N_MINUS_M_SHFT                                                                         0

#define HWIO_LPASS_AON_PERF0_D_DFSR_ADDR(x)                                                                           ((x) + 0x14078)
#define HWIO_LPASS_AON_PERF0_D_DFSR_RMSK                                                                                    0xff
#define HWIO_LPASS_AON_PERF0_D_DFSR_IN(x)            \
                in_dword(HWIO_LPASS_AON_PERF0_D_DFSR_ADDR(x))
#define HWIO_LPASS_AON_PERF0_D_DFSR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AON_PERF0_D_DFSR_ADDR(x), m)
#define HWIO_LPASS_AON_PERF0_D_DFSR_OUT(x, v)            \
                out_dword(HWIO_LPASS_AON_PERF0_D_DFSR_ADDR(x),v)
#define HWIO_LPASS_AON_PERF0_D_DFSR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AON_PERF0_D_DFSR_ADDR(x),m,v,HWIO_LPASS_AON_PERF0_D_DFSR_IN(x))
#define HWIO_LPASS_AON_PERF0_D_DFSR_NOT_2D_BMSK                                                                             0xff
#define HWIO_LPASS_AON_PERF0_D_DFSR_NOT_2D_SHFT                                                                                0

#define HWIO_LPASS_AON_PERF1_D_DFSR_ADDR(x)                                                                           ((x) + 0x1407c)
#define HWIO_LPASS_AON_PERF1_D_DFSR_RMSK                                                                                    0xff
#define HWIO_LPASS_AON_PERF1_D_DFSR_IN(x)            \
                in_dword(HWIO_LPASS_AON_PERF1_D_DFSR_ADDR(x))
#define HWIO_LPASS_AON_PERF1_D_DFSR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AON_PERF1_D_DFSR_ADDR(x), m)
#define HWIO_LPASS_AON_PERF1_D_DFSR_OUT(x, v)            \
                out_dword(HWIO_LPASS_AON_PERF1_D_DFSR_ADDR(x),v)
#define HWIO_LPASS_AON_PERF1_D_DFSR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AON_PERF1_D_DFSR_ADDR(x),m,v,HWIO_LPASS_AON_PERF1_D_DFSR_IN(x))
#define HWIO_LPASS_AON_PERF1_D_DFSR_NOT_2D_BMSK                                                                             0xff
#define HWIO_LPASS_AON_PERF1_D_DFSR_NOT_2D_SHFT                                                                                0

#define HWIO_LPASS_AON_PERF2_D_DFSR_ADDR(x)                                                                           ((x) + 0x14080)
#define HWIO_LPASS_AON_PERF2_D_DFSR_RMSK                                                                                    0xff
#define HWIO_LPASS_AON_PERF2_D_DFSR_IN(x)            \
                in_dword(HWIO_LPASS_AON_PERF2_D_DFSR_ADDR(x))
#define HWIO_LPASS_AON_PERF2_D_DFSR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AON_PERF2_D_DFSR_ADDR(x), m)
#define HWIO_LPASS_AON_PERF2_D_DFSR_OUT(x, v)            \
                out_dword(HWIO_LPASS_AON_PERF2_D_DFSR_ADDR(x),v)
#define HWIO_LPASS_AON_PERF2_D_DFSR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AON_PERF2_D_DFSR_ADDR(x),m,v,HWIO_LPASS_AON_PERF2_D_DFSR_IN(x))
#define HWIO_LPASS_AON_PERF2_D_DFSR_NOT_2D_BMSK                                                                             0xff
#define HWIO_LPASS_AON_PERF2_D_DFSR_NOT_2D_SHFT                                                                                0

#define HWIO_LPASS_AON_PERF3_D_DFSR_ADDR(x)                                                                           ((x) + 0x14084)
#define HWIO_LPASS_AON_PERF3_D_DFSR_RMSK                                                                                    0xff
#define HWIO_LPASS_AON_PERF3_D_DFSR_IN(x)            \
                in_dword(HWIO_LPASS_AON_PERF3_D_DFSR_ADDR(x))
#define HWIO_LPASS_AON_PERF3_D_DFSR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AON_PERF3_D_DFSR_ADDR(x), m)
#define HWIO_LPASS_AON_PERF3_D_DFSR_OUT(x, v)            \
                out_dword(HWIO_LPASS_AON_PERF3_D_DFSR_ADDR(x),v)
#define HWIO_LPASS_AON_PERF3_D_DFSR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AON_PERF3_D_DFSR_ADDR(x),m,v,HWIO_LPASS_AON_PERF3_D_DFSR_IN(x))
#define HWIO_LPASS_AON_PERF3_D_DFSR_NOT_2D_BMSK                                                                             0xff
#define HWIO_LPASS_AON_PERF3_D_DFSR_NOT_2D_SHFT                                                                                0

#define HWIO_LPASS_AON_PERF4_D_DFSR_ADDR(x)                                                                           ((x) + 0x14088)
#define HWIO_LPASS_AON_PERF4_D_DFSR_RMSK                                                                                    0xff
#define HWIO_LPASS_AON_PERF4_D_DFSR_IN(x)            \
                in_dword(HWIO_LPASS_AON_PERF4_D_DFSR_ADDR(x))
#define HWIO_LPASS_AON_PERF4_D_DFSR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AON_PERF4_D_DFSR_ADDR(x), m)
#define HWIO_LPASS_AON_PERF4_D_DFSR_OUT(x, v)            \
                out_dword(HWIO_LPASS_AON_PERF4_D_DFSR_ADDR(x),v)
#define HWIO_LPASS_AON_PERF4_D_DFSR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AON_PERF4_D_DFSR_ADDR(x),m,v,HWIO_LPASS_AON_PERF4_D_DFSR_IN(x))
#define HWIO_LPASS_AON_PERF4_D_DFSR_NOT_2D_BMSK                                                                             0xff
#define HWIO_LPASS_AON_PERF4_D_DFSR_NOT_2D_SHFT                                                                                0

#define HWIO_LPASS_AON_PERF5_D_DFSR_ADDR(x)                                                                           ((x) + 0x1408c)
#define HWIO_LPASS_AON_PERF5_D_DFSR_RMSK                                                                                    0xff
#define HWIO_LPASS_AON_PERF5_D_DFSR_IN(x)            \
                in_dword(HWIO_LPASS_AON_PERF5_D_DFSR_ADDR(x))
#define HWIO_LPASS_AON_PERF5_D_DFSR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AON_PERF5_D_DFSR_ADDR(x), m)
#define HWIO_LPASS_AON_PERF5_D_DFSR_OUT(x, v)            \
                out_dword(HWIO_LPASS_AON_PERF5_D_DFSR_ADDR(x),v)
#define HWIO_LPASS_AON_PERF5_D_DFSR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AON_PERF5_D_DFSR_ADDR(x),m,v,HWIO_LPASS_AON_PERF5_D_DFSR_IN(x))
#define HWIO_LPASS_AON_PERF5_D_DFSR_NOT_2D_BMSK                                                                             0xff
#define HWIO_LPASS_AON_PERF5_D_DFSR_NOT_2D_SHFT                                                                                0

#define HWIO_LPASS_AON_PERF6_D_DFSR_ADDR(x)                                                                           ((x) + 0x14090)
#define HWIO_LPASS_AON_PERF6_D_DFSR_RMSK                                                                                    0xff
#define HWIO_LPASS_AON_PERF6_D_DFSR_IN(x)            \
                in_dword(HWIO_LPASS_AON_PERF6_D_DFSR_ADDR(x))
#define HWIO_LPASS_AON_PERF6_D_DFSR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AON_PERF6_D_DFSR_ADDR(x), m)
#define HWIO_LPASS_AON_PERF6_D_DFSR_OUT(x, v)            \
                out_dword(HWIO_LPASS_AON_PERF6_D_DFSR_ADDR(x),v)
#define HWIO_LPASS_AON_PERF6_D_DFSR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AON_PERF6_D_DFSR_ADDR(x),m,v,HWIO_LPASS_AON_PERF6_D_DFSR_IN(x))
#define HWIO_LPASS_AON_PERF6_D_DFSR_NOT_2D_BMSK                                                                             0xff
#define HWIO_LPASS_AON_PERF6_D_DFSR_NOT_2D_SHFT                                                                                0

#define HWIO_LPASS_AON_PERF7_D_DFSR_ADDR(x)                                                                           ((x) + 0x14094)
#define HWIO_LPASS_AON_PERF7_D_DFSR_RMSK                                                                                    0xff
#define HWIO_LPASS_AON_PERF7_D_DFSR_IN(x)            \
                in_dword(HWIO_LPASS_AON_PERF7_D_DFSR_ADDR(x))
#define HWIO_LPASS_AON_PERF7_D_DFSR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AON_PERF7_D_DFSR_ADDR(x), m)
#define HWIO_LPASS_AON_PERF7_D_DFSR_OUT(x, v)            \
                out_dword(HWIO_LPASS_AON_PERF7_D_DFSR_ADDR(x),v)
#define HWIO_LPASS_AON_PERF7_D_DFSR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AON_PERF7_D_DFSR_ADDR(x),m,v,HWIO_LPASS_AON_PERF7_D_DFSR_IN(x))
#define HWIO_LPASS_AON_PERF7_D_DFSR_NOT_2D_BMSK                                                                             0xff
#define HWIO_LPASS_AON_PERF7_D_DFSR_NOT_2D_SHFT                                                                                0

#define HWIO_LPASS_AUDIO_WRAPPER_AON_CBCR_ADDR(x)                                                                     ((x) + 0x14098)
#define HWIO_LPASS_AUDIO_WRAPPER_AON_CBCR_RMSK                                                                        0x80000003
#define HWIO_LPASS_AUDIO_WRAPPER_AON_CBCR_IN(x)            \
                in_dword(HWIO_LPASS_AUDIO_WRAPPER_AON_CBCR_ADDR(x))
#define HWIO_LPASS_AUDIO_WRAPPER_AON_CBCR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AUDIO_WRAPPER_AON_CBCR_ADDR(x), m)
#define HWIO_LPASS_AUDIO_WRAPPER_AON_CBCR_OUT(x, v)            \
                out_dword(HWIO_LPASS_AUDIO_WRAPPER_AON_CBCR_ADDR(x),v)
#define HWIO_LPASS_AUDIO_WRAPPER_AON_CBCR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AUDIO_WRAPPER_AON_CBCR_ADDR(x),m,v,HWIO_LPASS_AUDIO_WRAPPER_AON_CBCR_IN(x))
#define HWIO_LPASS_AUDIO_WRAPPER_AON_CBCR_CLK_OFF_BMSK                                                                0x80000000
#define HWIO_LPASS_AUDIO_WRAPPER_AON_CBCR_CLK_OFF_SHFT                                                                        31
#define HWIO_LPASS_AUDIO_WRAPPER_AON_CBCR_HW_CTL_BMSK                                                                        0x2
#define HWIO_LPASS_AUDIO_WRAPPER_AON_CBCR_HW_CTL_SHFT                                                                          1
#define HWIO_LPASS_AUDIO_WRAPPER_AON_CBCR_CLK_ENABLE_BMSK                                                                    0x1
#define HWIO_LPASS_AUDIO_WRAPPER_AON_CBCR_CLK_ENABLE_SHFT                                                                      0

#define HWIO_LPASS_ATIME_CMD_RCGR_ADDR(x)                                                                             ((x) + 0x15000)
#define HWIO_LPASS_ATIME_CMD_RCGR_RMSK                                                                                0x800000f3
#define HWIO_LPASS_ATIME_CMD_RCGR_IN(x)            \
                in_dword(HWIO_LPASS_ATIME_CMD_RCGR_ADDR(x))
#define HWIO_LPASS_ATIME_CMD_RCGR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_ATIME_CMD_RCGR_ADDR(x), m)
#define HWIO_LPASS_ATIME_CMD_RCGR_OUT(x, v)            \
                out_dword(HWIO_LPASS_ATIME_CMD_RCGR_ADDR(x),v)
#define HWIO_LPASS_ATIME_CMD_RCGR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_ATIME_CMD_RCGR_ADDR(x),m,v,HWIO_LPASS_ATIME_CMD_RCGR_IN(x))
#define HWIO_LPASS_ATIME_CMD_RCGR_ROOT_OFF_BMSK                                                                       0x80000000
#define HWIO_LPASS_ATIME_CMD_RCGR_ROOT_OFF_SHFT                                                                               31
#define HWIO_LPASS_ATIME_CMD_RCGR_DIRTY_D_BMSK                                                                              0x80
#define HWIO_LPASS_ATIME_CMD_RCGR_DIRTY_D_SHFT                                                                                 7
#define HWIO_LPASS_ATIME_CMD_RCGR_DIRTY_N_BMSK                                                                              0x40
#define HWIO_LPASS_ATIME_CMD_RCGR_DIRTY_N_SHFT                                                                                 6
#define HWIO_LPASS_ATIME_CMD_RCGR_DIRTY_M_BMSK                                                                              0x20
#define HWIO_LPASS_ATIME_CMD_RCGR_DIRTY_M_SHFT                                                                                 5
#define HWIO_LPASS_ATIME_CMD_RCGR_DIRTY_CFG_RCGR_BMSK                                                                       0x10
#define HWIO_LPASS_ATIME_CMD_RCGR_DIRTY_CFG_RCGR_SHFT                                                                          4
#define HWIO_LPASS_ATIME_CMD_RCGR_ROOT_EN_BMSK                                                                               0x2
#define HWIO_LPASS_ATIME_CMD_RCGR_ROOT_EN_SHFT                                                                                 1
#define HWIO_LPASS_ATIME_CMD_RCGR_UPDATE_BMSK                                                                                0x1
#define HWIO_LPASS_ATIME_CMD_RCGR_UPDATE_SHFT                                                                                  0

#define HWIO_LPASS_ATIME_CFG_RCGR_ADDR(x)                                                                             ((x) + 0x15004)
#define HWIO_LPASS_ATIME_CFG_RCGR_RMSK                                                                                    0x771f
#define HWIO_LPASS_ATIME_CFG_RCGR_IN(x)            \
                in_dword(HWIO_LPASS_ATIME_CFG_RCGR_ADDR(x))
#define HWIO_LPASS_ATIME_CFG_RCGR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_ATIME_CFG_RCGR_ADDR(x), m)
#define HWIO_LPASS_ATIME_CFG_RCGR_OUT(x, v)            \
                out_dword(HWIO_LPASS_ATIME_CFG_RCGR_ADDR(x),v)
#define HWIO_LPASS_ATIME_CFG_RCGR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_ATIME_CFG_RCGR_ADDR(x),m,v,HWIO_LPASS_ATIME_CFG_RCGR_IN(x))
#define HWIO_LPASS_ATIME_CFG_RCGR_ALT_SRC_SEL_BMSK                                                                        0x4000
#define HWIO_LPASS_ATIME_CFG_RCGR_ALT_SRC_SEL_SHFT                                                                            14
#define HWIO_LPASS_ATIME_CFG_RCGR_MODE_BMSK                                                                               0x3000
#define HWIO_LPASS_ATIME_CFG_RCGR_MODE_SHFT                                                                                   12
#define HWIO_LPASS_ATIME_CFG_RCGR_SRC_SEL_BMSK                                                                             0x700
#define HWIO_LPASS_ATIME_CFG_RCGR_SRC_SEL_SHFT                                                                                 8
#define HWIO_LPASS_ATIME_CFG_RCGR_SRC_DIV_BMSK                                                                              0x1f
#define HWIO_LPASS_ATIME_CFG_RCGR_SRC_DIV_SHFT                                                                                 0

#define HWIO_LPASS_ATIME_M_ADDR(x)                                                                                    ((x) + 0x15008)
#define HWIO_LPASS_ATIME_M_RMSK                                                                                             0xff
#define HWIO_LPASS_ATIME_M_IN(x)            \
                in_dword(HWIO_LPASS_ATIME_M_ADDR(x))
#define HWIO_LPASS_ATIME_M_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_ATIME_M_ADDR(x), m)
#define HWIO_LPASS_ATIME_M_OUT(x, v)            \
                out_dword(HWIO_LPASS_ATIME_M_ADDR(x),v)
#define HWIO_LPASS_ATIME_M_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_ATIME_M_ADDR(x),m,v,HWIO_LPASS_ATIME_M_IN(x))
#define HWIO_LPASS_ATIME_M_M_BMSK                                                                                           0xff
#define HWIO_LPASS_ATIME_M_M_SHFT                                                                                              0

#define HWIO_LPASS_ATIME_N_ADDR(x)                                                                                    ((x) + 0x1500c)
#define HWIO_LPASS_ATIME_N_RMSK                                                                                             0xff
#define HWIO_LPASS_ATIME_N_IN(x)            \
                in_dword(HWIO_LPASS_ATIME_N_ADDR(x))
#define HWIO_LPASS_ATIME_N_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_ATIME_N_ADDR(x), m)
#define HWIO_LPASS_ATIME_N_OUT(x, v)            \
                out_dword(HWIO_LPASS_ATIME_N_ADDR(x),v)
#define HWIO_LPASS_ATIME_N_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_ATIME_N_ADDR(x),m,v,HWIO_LPASS_ATIME_N_IN(x))
#define HWIO_LPASS_ATIME_N_NOT_N_MINUS_M_BMSK                                                                               0xff
#define HWIO_LPASS_ATIME_N_NOT_N_MINUS_M_SHFT                                                                                  0

#define HWIO_LPASS_ATIME_D_ADDR(x)                                                                                    ((x) + 0x15010)
#define HWIO_LPASS_ATIME_D_RMSK                                                                                             0xff
#define HWIO_LPASS_ATIME_D_IN(x)            \
                in_dword(HWIO_LPASS_ATIME_D_ADDR(x))
#define HWIO_LPASS_ATIME_D_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_ATIME_D_ADDR(x), m)
#define HWIO_LPASS_ATIME_D_OUT(x, v)            \
                out_dword(HWIO_LPASS_ATIME_D_ADDR(x),v)
#define HWIO_LPASS_ATIME_D_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_ATIME_D_ADDR(x),m,v,HWIO_LPASS_ATIME_D_IN(x))
#define HWIO_LPASS_ATIME_D_NOT_2D_BMSK                                                                                      0xff
#define HWIO_LPASS_ATIME_D_NOT_2D_SHFT                                                                                         0

#define HWIO_LPASS_AUDIO_CORE_AVSYNC_ATIME_CBCR_ADDR(x)                                                               ((x) + 0x15014)
#define HWIO_LPASS_AUDIO_CORE_AVSYNC_ATIME_CBCR_RMSK                                                                  0x80000003
#define HWIO_LPASS_AUDIO_CORE_AVSYNC_ATIME_CBCR_IN(x)            \
                in_dword(HWIO_LPASS_AUDIO_CORE_AVSYNC_ATIME_CBCR_ADDR(x))
#define HWIO_LPASS_AUDIO_CORE_AVSYNC_ATIME_CBCR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AUDIO_CORE_AVSYNC_ATIME_CBCR_ADDR(x), m)
#define HWIO_LPASS_AUDIO_CORE_AVSYNC_ATIME_CBCR_OUT(x, v)            \
                out_dword(HWIO_LPASS_AUDIO_CORE_AVSYNC_ATIME_CBCR_ADDR(x),v)
#define HWIO_LPASS_AUDIO_CORE_AVSYNC_ATIME_CBCR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AUDIO_CORE_AVSYNC_ATIME_CBCR_ADDR(x),m,v,HWIO_LPASS_AUDIO_CORE_AVSYNC_ATIME_CBCR_IN(x))
#define HWIO_LPASS_AUDIO_CORE_AVSYNC_ATIME_CBCR_CLK_OFF_BMSK                                                          0x80000000
#define HWIO_LPASS_AUDIO_CORE_AVSYNC_ATIME_CBCR_CLK_OFF_SHFT                                                                  31
#define HWIO_LPASS_AUDIO_CORE_AVSYNC_ATIME_CBCR_HW_CTL_BMSK                                                                  0x2
#define HWIO_LPASS_AUDIO_CORE_AVSYNC_ATIME_CBCR_HW_CTL_SHFT                                                                    1
#define HWIO_LPASS_AUDIO_CORE_AVSYNC_ATIME_CBCR_CLK_ENABLE_BMSK                                                              0x1
#define HWIO_LPASS_AUDIO_CORE_AVSYNC_ATIME_CBCR_CLK_ENABLE_SHFT                                                                0

#define HWIO_LPASS_RESAMPLER_CMD_RCGR_ADDR(x)                                                                         ((x) + 0x16000)
#define HWIO_LPASS_RESAMPLER_CMD_RCGR_RMSK                                                                            0x800000f3
#define HWIO_LPASS_RESAMPLER_CMD_RCGR_IN(x)            \
                in_dword(HWIO_LPASS_RESAMPLER_CMD_RCGR_ADDR(x))
#define HWIO_LPASS_RESAMPLER_CMD_RCGR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_RESAMPLER_CMD_RCGR_ADDR(x), m)
#define HWIO_LPASS_RESAMPLER_CMD_RCGR_OUT(x, v)            \
                out_dword(HWIO_LPASS_RESAMPLER_CMD_RCGR_ADDR(x),v)
#define HWIO_LPASS_RESAMPLER_CMD_RCGR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_RESAMPLER_CMD_RCGR_ADDR(x),m,v,HWIO_LPASS_RESAMPLER_CMD_RCGR_IN(x))
#define HWIO_LPASS_RESAMPLER_CMD_RCGR_ROOT_OFF_BMSK                                                                   0x80000000
#define HWIO_LPASS_RESAMPLER_CMD_RCGR_ROOT_OFF_SHFT                                                                           31
#define HWIO_LPASS_RESAMPLER_CMD_RCGR_DIRTY_D_BMSK                                                                          0x80
#define HWIO_LPASS_RESAMPLER_CMD_RCGR_DIRTY_D_SHFT                                                                             7
#define HWIO_LPASS_RESAMPLER_CMD_RCGR_DIRTY_N_BMSK                                                                          0x40
#define HWIO_LPASS_RESAMPLER_CMD_RCGR_DIRTY_N_SHFT                                                                             6
#define HWIO_LPASS_RESAMPLER_CMD_RCGR_DIRTY_M_BMSK                                                                          0x20
#define HWIO_LPASS_RESAMPLER_CMD_RCGR_DIRTY_M_SHFT                                                                             5
#define HWIO_LPASS_RESAMPLER_CMD_RCGR_DIRTY_CFG_RCGR_BMSK                                                                   0x10
#define HWIO_LPASS_RESAMPLER_CMD_RCGR_DIRTY_CFG_RCGR_SHFT                                                                      4
#define HWIO_LPASS_RESAMPLER_CMD_RCGR_ROOT_EN_BMSK                                                                           0x2
#define HWIO_LPASS_RESAMPLER_CMD_RCGR_ROOT_EN_SHFT                                                                             1
#define HWIO_LPASS_RESAMPLER_CMD_RCGR_UPDATE_BMSK                                                                            0x1
#define HWIO_LPASS_RESAMPLER_CMD_RCGR_UPDATE_SHFT                                                                              0

#define HWIO_LPASS_RESAMPLER_CFG_RCGR_ADDR(x)                                                                         ((x) + 0x16004)
#define HWIO_LPASS_RESAMPLER_CFG_RCGR_RMSK                                                                                0x771f
#define HWIO_LPASS_RESAMPLER_CFG_RCGR_IN(x)            \
                in_dword(HWIO_LPASS_RESAMPLER_CFG_RCGR_ADDR(x))
#define HWIO_LPASS_RESAMPLER_CFG_RCGR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_RESAMPLER_CFG_RCGR_ADDR(x), m)
#define HWIO_LPASS_RESAMPLER_CFG_RCGR_OUT(x, v)            \
                out_dword(HWIO_LPASS_RESAMPLER_CFG_RCGR_ADDR(x),v)
#define HWIO_LPASS_RESAMPLER_CFG_RCGR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_RESAMPLER_CFG_RCGR_ADDR(x),m,v,HWIO_LPASS_RESAMPLER_CFG_RCGR_IN(x))
#define HWIO_LPASS_RESAMPLER_CFG_RCGR_ALT_SRC_SEL_BMSK                                                                    0x4000
#define HWIO_LPASS_RESAMPLER_CFG_RCGR_ALT_SRC_SEL_SHFT                                                                        14
#define HWIO_LPASS_RESAMPLER_CFG_RCGR_MODE_BMSK                                                                           0x3000
#define HWIO_LPASS_RESAMPLER_CFG_RCGR_MODE_SHFT                                                                               12
#define HWIO_LPASS_RESAMPLER_CFG_RCGR_SRC_SEL_BMSK                                                                         0x700
#define HWIO_LPASS_RESAMPLER_CFG_RCGR_SRC_SEL_SHFT                                                                             8
#define HWIO_LPASS_RESAMPLER_CFG_RCGR_SRC_DIV_BMSK                                                                          0x1f
#define HWIO_LPASS_RESAMPLER_CFG_RCGR_SRC_DIV_SHFT                                                                             0

#define HWIO_LPASS_RESAMPLER_M_ADDR(x)                                                                                ((x) + 0x16008)
#define HWIO_LPASS_RESAMPLER_M_RMSK                                                                                         0xff
#define HWIO_LPASS_RESAMPLER_M_IN(x)            \
                in_dword(HWIO_LPASS_RESAMPLER_M_ADDR(x))
#define HWIO_LPASS_RESAMPLER_M_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_RESAMPLER_M_ADDR(x), m)
#define HWIO_LPASS_RESAMPLER_M_OUT(x, v)            \
                out_dword(HWIO_LPASS_RESAMPLER_M_ADDR(x),v)
#define HWIO_LPASS_RESAMPLER_M_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_RESAMPLER_M_ADDR(x),m,v,HWIO_LPASS_RESAMPLER_M_IN(x))
#define HWIO_LPASS_RESAMPLER_M_M_BMSK                                                                                       0xff
#define HWIO_LPASS_RESAMPLER_M_M_SHFT                                                                                          0

#define HWIO_LPASS_RESAMPLER_N_ADDR(x)                                                                                ((x) + 0x1600c)
#define HWIO_LPASS_RESAMPLER_N_RMSK                                                                                         0xff
#define HWIO_LPASS_RESAMPLER_N_IN(x)            \
                in_dword(HWIO_LPASS_RESAMPLER_N_ADDR(x))
#define HWIO_LPASS_RESAMPLER_N_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_RESAMPLER_N_ADDR(x), m)
#define HWIO_LPASS_RESAMPLER_N_OUT(x, v)            \
                out_dword(HWIO_LPASS_RESAMPLER_N_ADDR(x),v)
#define HWIO_LPASS_RESAMPLER_N_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_RESAMPLER_N_ADDR(x),m,v,HWIO_LPASS_RESAMPLER_N_IN(x))
#define HWIO_LPASS_RESAMPLER_N_NOT_N_MINUS_M_BMSK                                                                           0xff
#define HWIO_LPASS_RESAMPLER_N_NOT_N_MINUS_M_SHFT                                                                              0

#define HWIO_LPASS_RESAMPLER_D_ADDR(x)                                                                                ((x) + 0x16010)
#define HWIO_LPASS_RESAMPLER_D_RMSK                                                                                         0xff
#define HWIO_LPASS_RESAMPLER_D_IN(x)            \
                in_dword(HWIO_LPASS_RESAMPLER_D_ADDR(x))
#define HWIO_LPASS_RESAMPLER_D_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_RESAMPLER_D_ADDR(x), m)
#define HWIO_LPASS_RESAMPLER_D_OUT(x, v)            \
                out_dword(HWIO_LPASS_RESAMPLER_D_ADDR(x),v)
#define HWIO_LPASS_RESAMPLER_D_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_RESAMPLER_D_ADDR(x),m,v,HWIO_LPASS_RESAMPLER_D_IN(x))
#define HWIO_LPASS_RESAMPLER_D_NOT_2D_BMSK                                                                                  0xff
#define HWIO_LPASS_RESAMPLER_D_NOT_2D_SHFT                                                                                     0

#define HWIO_LPASS_RESAMPLER_CMD_DFSR_ADDR(x)                                                                         ((x) + 0x16014)
#define HWIO_LPASS_RESAMPLER_CMD_DFSR_RMSK                                                                                0xbfef
#define HWIO_LPASS_RESAMPLER_CMD_DFSR_IN(x)            \
                in_dword(HWIO_LPASS_RESAMPLER_CMD_DFSR_ADDR(x))
#define HWIO_LPASS_RESAMPLER_CMD_DFSR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_RESAMPLER_CMD_DFSR_ADDR(x), m)
#define HWIO_LPASS_RESAMPLER_CMD_DFSR_OUT(x, v)            \
                out_dword(HWIO_LPASS_RESAMPLER_CMD_DFSR_ADDR(x),v)
#define HWIO_LPASS_RESAMPLER_CMD_DFSR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_RESAMPLER_CMD_DFSR_ADDR(x),m,v,HWIO_LPASS_RESAMPLER_CMD_DFSR_IN(x))
#define HWIO_LPASS_RESAMPLER_CMD_DFSR_RCG_SW_CTRL_BMSK                                                                    0x8000
#define HWIO_LPASS_RESAMPLER_CMD_DFSR_RCG_SW_CTRL_SHFT                                                                        15
#define HWIO_LPASS_RESAMPLER_CMD_DFSR_SW_PERF_STATE_BMSK                                                                  0x3800
#define HWIO_LPASS_RESAMPLER_CMD_DFSR_SW_PERF_STATE_SHFT                                                                      11
#define HWIO_LPASS_RESAMPLER_CMD_DFSR_SW_OVERRIDE_BMSK                                                                     0x400
#define HWIO_LPASS_RESAMPLER_CMD_DFSR_SW_OVERRIDE_SHFT                                                                        10
#define HWIO_LPASS_RESAMPLER_CMD_DFSR_PERF_STATE_UPDATE_STATUS_BMSK                                                        0x200
#define HWIO_LPASS_RESAMPLER_CMD_DFSR_PERF_STATE_UPDATE_STATUS_SHFT                                                            9
#define HWIO_LPASS_RESAMPLER_CMD_DFSR_DFS_FSM_STATE_BMSK                                                                   0x1c0
#define HWIO_LPASS_RESAMPLER_CMD_DFSR_DFS_FSM_STATE_SHFT                                                                       6
#define HWIO_LPASS_RESAMPLER_CMD_DFSR_HW_CLK_CONTROL_BMSK                                                                   0x20
#define HWIO_LPASS_RESAMPLER_CMD_DFSR_HW_CLK_CONTROL_SHFT                                                                      5
#define HWIO_LPASS_RESAMPLER_CMD_DFSR_CURR_PERF_STATE_BMSK                                                                   0xe
#define HWIO_LPASS_RESAMPLER_CMD_DFSR_CURR_PERF_STATE_SHFT                                                                     1
#define HWIO_LPASS_RESAMPLER_CMD_DFSR_DFS_EN_BMSK                                                                            0x1
#define HWIO_LPASS_RESAMPLER_CMD_DFSR_DFS_EN_SHFT                                                                              0

#define HWIO_LPASS_RESAMPLER_PERF0_DFSR_ADDR(x)                                                                       ((x) + 0x16018)
#define HWIO_LPASS_RESAMPLER_PERF0_DFSR_RMSK                                                                              0x371f
#define HWIO_LPASS_RESAMPLER_PERF0_DFSR_IN(x)            \
                in_dword(HWIO_LPASS_RESAMPLER_PERF0_DFSR_ADDR(x))
#define HWIO_LPASS_RESAMPLER_PERF0_DFSR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_RESAMPLER_PERF0_DFSR_ADDR(x), m)
#define HWIO_LPASS_RESAMPLER_PERF0_DFSR_OUT(x, v)            \
                out_dword(HWIO_LPASS_RESAMPLER_PERF0_DFSR_ADDR(x),v)
#define HWIO_LPASS_RESAMPLER_PERF0_DFSR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_RESAMPLER_PERF0_DFSR_ADDR(x),m,v,HWIO_LPASS_RESAMPLER_PERF0_DFSR_IN(x))
#define HWIO_LPASS_RESAMPLER_PERF0_DFSR_MODE_BMSK                                                                         0x3000
#define HWIO_LPASS_RESAMPLER_PERF0_DFSR_MODE_SHFT                                                                             12
#define HWIO_LPASS_RESAMPLER_PERF0_DFSR_SRC_SEL_BMSK                                                                       0x700
#define HWIO_LPASS_RESAMPLER_PERF0_DFSR_SRC_SEL_SHFT                                                                           8
#define HWIO_LPASS_RESAMPLER_PERF0_DFSR_SRC_DIV_BMSK                                                                        0x1f
#define HWIO_LPASS_RESAMPLER_PERF0_DFSR_SRC_DIV_SHFT                                                                           0

#define HWIO_LPASS_RESAMPLER_PERF1_DFSR_ADDR(x)                                                                       ((x) + 0x1601c)
#define HWIO_LPASS_RESAMPLER_PERF1_DFSR_RMSK                                                                              0x371f
#define HWIO_LPASS_RESAMPLER_PERF1_DFSR_IN(x)            \
                in_dword(HWIO_LPASS_RESAMPLER_PERF1_DFSR_ADDR(x))
#define HWIO_LPASS_RESAMPLER_PERF1_DFSR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_RESAMPLER_PERF1_DFSR_ADDR(x), m)
#define HWIO_LPASS_RESAMPLER_PERF1_DFSR_OUT(x, v)            \
                out_dword(HWIO_LPASS_RESAMPLER_PERF1_DFSR_ADDR(x),v)
#define HWIO_LPASS_RESAMPLER_PERF1_DFSR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_RESAMPLER_PERF1_DFSR_ADDR(x),m,v,HWIO_LPASS_RESAMPLER_PERF1_DFSR_IN(x))
#define HWIO_LPASS_RESAMPLER_PERF1_DFSR_MODE_BMSK                                                                         0x3000
#define HWIO_LPASS_RESAMPLER_PERF1_DFSR_MODE_SHFT                                                                             12
#define HWIO_LPASS_RESAMPLER_PERF1_DFSR_SRC_SEL_BMSK                                                                       0x700
#define HWIO_LPASS_RESAMPLER_PERF1_DFSR_SRC_SEL_SHFT                                                                           8
#define HWIO_LPASS_RESAMPLER_PERF1_DFSR_SRC_DIV_BMSK                                                                        0x1f
#define HWIO_LPASS_RESAMPLER_PERF1_DFSR_SRC_DIV_SHFT                                                                           0

#define HWIO_LPASS_RESAMPLER_PERF2_DFSR_ADDR(x)                                                                       ((x) + 0x16020)
#define HWIO_LPASS_RESAMPLER_PERF2_DFSR_RMSK                                                                              0x371f
#define HWIO_LPASS_RESAMPLER_PERF2_DFSR_IN(x)            \
                in_dword(HWIO_LPASS_RESAMPLER_PERF2_DFSR_ADDR(x))
#define HWIO_LPASS_RESAMPLER_PERF2_DFSR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_RESAMPLER_PERF2_DFSR_ADDR(x), m)
#define HWIO_LPASS_RESAMPLER_PERF2_DFSR_OUT(x, v)            \
                out_dword(HWIO_LPASS_RESAMPLER_PERF2_DFSR_ADDR(x),v)
#define HWIO_LPASS_RESAMPLER_PERF2_DFSR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_RESAMPLER_PERF2_DFSR_ADDR(x),m,v,HWIO_LPASS_RESAMPLER_PERF2_DFSR_IN(x))
#define HWIO_LPASS_RESAMPLER_PERF2_DFSR_MODE_BMSK                                                                         0x3000
#define HWIO_LPASS_RESAMPLER_PERF2_DFSR_MODE_SHFT                                                                             12
#define HWIO_LPASS_RESAMPLER_PERF2_DFSR_SRC_SEL_BMSK                                                                       0x700
#define HWIO_LPASS_RESAMPLER_PERF2_DFSR_SRC_SEL_SHFT                                                                           8
#define HWIO_LPASS_RESAMPLER_PERF2_DFSR_SRC_DIV_BMSK                                                                        0x1f
#define HWIO_LPASS_RESAMPLER_PERF2_DFSR_SRC_DIV_SHFT                                                                           0

#define HWIO_LPASS_RESAMPLER_PERF3_DFSR_ADDR(x)                                                                       ((x) + 0x16024)
#define HWIO_LPASS_RESAMPLER_PERF3_DFSR_RMSK                                                                              0x371f
#define HWIO_LPASS_RESAMPLER_PERF3_DFSR_IN(x)            \
                in_dword(HWIO_LPASS_RESAMPLER_PERF3_DFSR_ADDR(x))
#define HWIO_LPASS_RESAMPLER_PERF3_DFSR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_RESAMPLER_PERF3_DFSR_ADDR(x), m)
#define HWIO_LPASS_RESAMPLER_PERF3_DFSR_OUT(x, v)            \
                out_dword(HWIO_LPASS_RESAMPLER_PERF3_DFSR_ADDR(x),v)
#define HWIO_LPASS_RESAMPLER_PERF3_DFSR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_RESAMPLER_PERF3_DFSR_ADDR(x),m,v,HWIO_LPASS_RESAMPLER_PERF3_DFSR_IN(x))
#define HWIO_LPASS_RESAMPLER_PERF3_DFSR_MODE_BMSK                                                                         0x3000
#define HWIO_LPASS_RESAMPLER_PERF3_DFSR_MODE_SHFT                                                                             12
#define HWIO_LPASS_RESAMPLER_PERF3_DFSR_SRC_SEL_BMSK                                                                       0x700
#define HWIO_LPASS_RESAMPLER_PERF3_DFSR_SRC_SEL_SHFT                                                                           8
#define HWIO_LPASS_RESAMPLER_PERF3_DFSR_SRC_DIV_BMSK                                                                        0x1f
#define HWIO_LPASS_RESAMPLER_PERF3_DFSR_SRC_DIV_SHFT                                                                           0

#define HWIO_LPASS_RESAMPLER_PERF4_DFSR_ADDR(x)                                                                       ((x) + 0x16028)
#define HWIO_LPASS_RESAMPLER_PERF4_DFSR_RMSK                                                                              0x371f
#define HWIO_LPASS_RESAMPLER_PERF4_DFSR_IN(x)            \
                in_dword(HWIO_LPASS_RESAMPLER_PERF4_DFSR_ADDR(x))
#define HWIO_LPASS_RESAMPLER_PERF4_DFSR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_RESAMPLER_PERF4_DFSR_ADDR(x), m)
#define HWIO_LPASS_RESAMPLER_PERF4_DFSR_OUT(x, v)            \
                out_dword(HWIO_LPASS_RESAMPLER_PERF4_DFSR_ADDR(x),v)
#define HWIO_LPASS_RESAMPLER_PERF4_DFSR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_RESAMPLER_PERF4_DFSR_ADDR(x),m,v,HWIO_LPASS_RESAMPLER_PERF4_DFSR_IN(x))
#define HWIO_LPASS_RESAMPLER_PERF4_DFSR_MODE_BMSK                                                                         0x3000
#define HWIO_LPASS_RESAMPLER_PERF4_DFSR_MODE_SHFT                                                                             12
#define HWIO_LPASS_RESAMPLER_PERF4_DFSR_SRC_SEL_BMSK                                                                       0x700
#define HWIO_LPASS_RESAMPLER_PERF4_DFSR_SRC_SEL_SHFT                                                                           8
#define HWIO_LPASS_RESAMPLER_PERF4_DFSR_SRC_DIV_BMSK                                                                        0x1f
#define HWIO_LPASS_RESAMPLER_PERF4_DFSR_SRC_DIV_SHFT                                                                           0

#define HWIO_LPASS_RESAMPLER_PERF5_DFSR_ADDR(x)                                                                       ((x) + 0x1602c)
#define HWIO_LPASS_RESAMPLER_PERF5_DFSR_RMSK                                                                              0x371f
#define HWIO_LPASS_RESAMPLER_PERF5_DFSR_IN(x)            \
                in_dword(HWIO_LPASS_RESAMPLER_PERF5_DFSR_ADDR(x))
#define HWIO_LPASS_RESAMPLER_PERF5_DFSR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_RESAMPLER_PERF5_DFSR_ADDR(x), m)
#define HWIO_LPASS_RESAMPLER_PERF5_DFSR_OUT(x, v)            \
                out_dword(HWIO_LPASS_RESAMPLER_PERF5_DFSR_ADDR(x),v)
#define HWIO_LPASS_RESAMPLER_PERF5_DFSR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_RESAMPLER_PERF5_DFSR_ADDR(x),m,v,HWIO_LPASS_RESAMPLER_PERF5_DFSR_IN(x))
#define HWIO_LPASS_RESAMPLER_PERF5_DFSR_MODE_BMSK                                                                         0x3000
#define HWIO_LPASS_RESAMPLER_PERF5_DFSR_MODE_SHFT                                                                             12
#define HWIO_LPASS_RESAMPLER_PERF5_DFSR_SRC_SEL_BMSK                                                                       0x700
#define HWIO_LPASS_RESAMPLER_PERF5_DFSR_SRC_SEL_SHFT                                                                           8
#define HWIO_LPASS_RESAMPLER_PERF5_DFSR_SRC_DIV_BMSK                                                                        0x1f
#define HWIO_LPASS_RESAMPLER_PERF5_DFSR_SRC_DIV_SHFT                                                                           0

#define HWIO_LPASS_RESAMPLER_PERF6_DFSR_ADDR(x)                                                                       ((x) + 0x16030)
#define HWIO_LPASS_RESAMPLER_PERF6_DFSR_RMSK                                                                              0x371f
#define HWIO_LPASS_RESAMPLER_PERF6_DFSR_IN(x)            \
                in_dword(HWIO_LPASS_RESAMPLER_PERF6_DFSR_ADDR(x))
#define HWIO_LPASS_RESAMPLER_PERF6_DFSR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_RESAMPLER_PERF6_DFSR_ADDR(x), m)
#define HWIO_LPASS_RESAMPLER_PERF6_DFSR_OUT(x, v)            \
                out_dword(HWIO_LPASS_RESAMPLER_PERF6_DFSR_ADDR(x),v)
#define HWIO_LPASS_RESAMPLER_PERF6_DFSR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_RESAMPLER_PERF6_DFSR_ADDR(x),m,v,HWIO_LPASS_RESAMPLER_PERF6_DFSR_IN(x))
#define HWIO_LPASS_RESAMPLER_PERF6_DFSR_MODE_BMSK                                                                         0x3000
#define HWIO_LPASS_RESAMPLER_PERF6_DFSR_MODE_SHFT                                                                             12
#define HWIO_LPASS_RESAMPLER_PERF6_DFSR_SRC_SEL_BMSK                                                                       0x700
#define HWIO_LPASS_RESAMPLER_PERF6_DFSR_SRC_SEL_SHFT                                                                           8
#define HWIO_LPASS_RESAMPLER_PERF6_DFSR_SRC_DIV_BMSK                                                                        0x1f
#define HWIO_LPASS_RESAMPLER_PERF6_DFSR_SRC_DIV_SHFT                                                                           0

#define HWIO_LPASS_RESAMPLER_PERF7_DFSR_ADDR(x)                                                                       ((x) + 0x16034)
#define HWIO_LPASS_RESAMPLER_PERF7_DFSR_RMSK                                                                              0x371f
#define HWIO_LPASS_RESAMPLER_PERF7_DFSR_IN(x)            \
                in_dword(HWIO_LPASS_RESAMPLER_PERF7_DFSR_ADDR(x))
#define HWIO_LPASS_RESAMPLER_PERF7_DFSR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_RESAMPLER_PERF7_DFSR_ADDR(x), m)
#define HWIO_LPASS_RESAMPLER_PERF7_DFSR_OUT(x, v)            \
                out_dword(HWIO_LPASS_RESAMPLER_PERF7_DFSR_ADDR(x),v)
#define HWIO_LPASS_RESAMPLER_PERF7_DFSR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_RESAMPLER_PERF7_DFSR_ADDR(x),m,v,HWIO_LPASS_RESAMPLER_PERF7_DFSR_IN(x))
#define HWIO_LPASS_RESAMPLER_PERF7_DFSR_MODE_BMSK                                                                         0x3000
#define HWIO_LPASS_RESAMPLER_PERF7_DFSR_MODE_SHFT                                                                             12
#define HWIO_LPASS_RESAMPLER_PERF7_DFSR_SRC_SEL_BMSK                                                                       0x700
#define HWIO_LPASS_RESAMPLER_PERF7_DFSR_SRC_SEL_SHFT                                                                           8
#define HWIO_LPASS_RESAMPLER_PERF7_DFSR_SRC_DIV_BMSK                                                                        0x1f
#define HWIO_LPASS_RESAMPLER_PERF7_DFSR_SRC_DIV_SHFT                                                                           0

#define HWIO_LPASS_RESAMPLER_PERF0_M_DFSR_ADDR(x)                                                                     ((x) + 0x16038)
#define HWIO_LPASS_RESAMPLER_PERF0_M_DFSR_RMSK                                                                              0xff
#define HWIO_LPASS_RESAMPLER_PERF0_M_DFSR_IN(x)            \
                in_dword(HWIO_LPASS_RESAMPLER_PERF0_M_DFSR_ADDR(x))
#define HWIO_LPASS_RESAMPLER_PERF0_M_DFSR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_RESAMPLER_PERF0_M_DFSR_ADDR(x), m)
#define HWIO_LPASS_RESAMPLER_PERF0_M_DFSR_OUT(x, v)            \
                out_dword(HWIO_LPASS_RESAMPLER_PERF0_M_DFSR_ADDR(x),v)
#define HWIO_LPASS_RESAMPLER_PERF0_M_DFSR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_RESAMPLER_PERF0_M_DFSR_ADDR(x),m,v,HWIO_LPASS_RESAMPLER_PERF0_M_DFSR_IN(x))
#define HWIO_LPASS_RESAMPLER_PERF0_M_DFSR_M_BMSK                                                                            0xff
#define HWIO_LPASS_RESAMPLER_PERF0_M_DFSR_M_SHFT                                                                               0

#define HWIO_LPASS_RESAMPLER_PERF1_M_DFSR_ADDR(x)                                                                     ((x) + 0x1603c)
#define HWIO_LPASS_RESAMPLER_PERF1_M_DFSR_RMSK                                                                              0xff
#define HWIO_LPASS_RESAMPLER_PERF1_M_DFSR_IN(x)            \
                in_dword(HWIO_LPASS_RESAMPLER_PERF1_M_DFSR_ADDR(x))
#define HWIO_LPASS_RESAMPLER_PERF1_M_DFSR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_RESAMPLER_PERF1_M_DFSR_ADDR(x), m)
#define HWIO_LPASS_RESAMPLER_PERF1_M_DFSR_OUT(x, v)            \
                out_dword(HWIO_LPASS_RESAMPLER_PERF1_M_DFSR_ADDR(x),v)
#define HWIO_LPASS_RESAMPLER_PERF1_M_DFSR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_RESAMPLER_PERF1_M_DFSR_ADDR(x),m,v,HWIO_LPASS_RESAMPLER_PERF1_M_DFSR_IN(x))
#define HWIO_LPASS_RESAMPLER_PERF1_M_DFSR_M_BMSK                                                                            0xff
#define HWIO_LPASS_RESAMPLER_PERF1_M_DFSR_M_SHFT                                                                               0

#define HWIO_LPASS_RESAMPLER_PERF2_M_DFSR_ADDR(x)                                                                     ((x) + 0x16040)
#define HWIO_LPASS_RESAMPLER_PERF2_M_DFSR_RMSK                                                                              0xff
#define HWIO_LPASS_RESAMPLER_PERF2_M_DFSR_IN(x)            \
                in_dword(HWIO_LPASS_RESAMPLER_PERF2_M_DFSR_ADDR(x))
#define HWIO_LPASS_RESAMPLER_PERF2_M_DFSR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_RESAMPLER_PERF2_M_DFSR_ADDR(x), m)
#define HWIO_LPASS_RESAMPLER_PERF2_M_DFSR_OUT(x, v)            \
                out_dword(HWIO_LPASS_RESAMPLER_PERF2_M_DFSR_ADDR(x),v)
#define HWIO_LPASS_RESAMPLER_PERF2_M_DFSR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_RESAMPLER_PERF2_M_DFSR_ADDR(x),m,v,HWIO_LPASS_RESAMPLER_PERF2_M_DFSR_IN(x))
#define HWIO_LPASS_RESAMPLER_PERF2_M_DFSR_M_BMSK                                                                            0xff
#define HWIO_LPASS_RESAMPLER_PERF2_M_DFSR_M_SHFT                                                                               0

#define HWIO_LPASS_RESAMPLER_PERF3_M_DFSR_ADDR(x)                                                                     ((x) + 0x16044)
#define HWIO_LPASS_RESAMPLER_PERF3_M_DFSR_RMSK                                                                              0xff
#define HWIO_LPASS_RESAMPLER_PERF3_M_DFSR_IN(x)            \
                in_dword(HWIO_LPASS_RESAMPLER_PERF3_M_DFSR_ADDR(x))
#define HWIO_LPASS_RESAMPLER_PERF3_M_DFSR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_RESAMPLER_PERF3_M_DFSR_ADDR(x), m)
#define HWIO_LPASS_RESAMPLER_PERF3_M_DFSR_OUT(x, v)            \
                out_dword(HWIO_LPASS_RESAMPLER_PERF3_M_DFSR_ADDR(x),v)
#define HWIO_LPASS_RESAMPLER_PERF3_M_DFSR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_RESAMPLER_PERF3_M_DFSR_ADDR(x),m,v,HWIO_LPASS_RESAMPLER_PERF3_M_DFSR_IN(x))
#define HWIO_LPASS_RESAMPLER_PERF3_M_DFSR_M_BMSK                                                                            0xff
#define HWIO_LPASS_RESAMPLER_PERF3_M_DFSR_M_SHFT                                                                               0

#define HWIO_LPASS_RESAMPLER_PERF4_M_DFSR_ADDR(x)                                                                     ((x) + 0x16048)
#define HWIO_LPASS_RESAMPLER_PERF4_M_DFSR_RMSK                                                                              0xff
#define HWIO_LPASS_RESAMPLER_PERF4_M_DFSR_IN(x)            \
                in_dword(HWIO_LPASS_RESAMPLER_PERF4_M_DFSR_ADDR(x))
#define HWIO_LPASS_RESAMPLER_PERF4_M_DFSR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_RESAMPLER_PERF4_M_DFSR_ADDR(x), m)
#define HWIO_LPASS_RESAMPLER_PERF4_M_DFSR_OUT(x, v)            \
                out_dword(HWIO_LPASS_RESAMPLER_PERF4_M_DFSR_ADDR(x),v)
#define HWIO_LPASS_RESAMPLER_PERF4_M_DFSR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_RESAMPLER_PERF4_M_DFSR_ADDR(x),m,v,HWIO_LPASS_RESAMPLER_PERF4_M_DFSR_IN(x))
#define HWIO_LPASS_RESAMPLER_PERF4_M_DFSR_M_BMSK                                                                            0xff
#define HWIO_LPASS_RESAMPLER_PERF4_M_DFSR_M_SHFT                                                                               0

#define HWIO_LPASS_RESAMPLER_PERF5_M_DFSR_ADDR(x)                                                                     ((x) + 0x1604c)
#define HWIO_LPASS_RESAMPLER_PERF5_M_DFSR_RMSK                                                                              0xff
#define HWIO_LPASS_RESAMPLER_PERF5_M_DFSR_IN(x)            \
                in_dword(HWIO_LPASS_RESAMPLER_PERF5_M_DFSR_ADDR(x))
#define HWIO_LPASS_RESAMPLER_PERF5_M_DFSR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_RESAMPLER_PERF5_M_DFSR_ADDR(x), m)
#define HWIO_LPASS_RESAMPLER_PERF5_M_DFSR_OUT(x, v)            \
                out_dword(HWIO_LPASS_RESAMPLER_PERF5_M_DFSR_ADDR(x),v)
#define HWIO_LPASS_RESAMPLER_PERF5_M_DFSR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_RESAMPLER_PERF5_M_DFSR_ADDR(x),m,v,HWIO_LPASS_RESAMPLER_PERF5_M_DFSR_IN(x))
#define HWIO_LPASS_RESAMPLER_PERF5_M_DFSR_M_BMSK                                                                            0xff
#define HWIO_LPASS_RESAMPLER_PERF5_M_DFSR_M_SHFT                                                                               0

#define HWIO_LPASS_RESAMPLER_PERF6_M_DFSR_ADDR(x)                                                                     ((x) + 0x16050)
#define HWIO_LPASS_RESAMPLER_PERF6_M_DFSR_RMSK                                                                              0xff
#define HWIO_LPASS_RESAMPLER_PERF6_M_DFSR_IN(x)            \
                in_dword(HWIO_LPASS_RESAMPLER_PERF6_M_DFSR_ADDR(x))
#define HWIO_LPASS_RESAMPLER_PERF6_M_DFSR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_RESAMPLER_PERF6_M_DFSR_ADDR(x), m)
#define HWIO_LPASS_RESAMPLER_PERF6_M_DFSR_OUT(x, v)            \
                out_dword(HWIO_LPASS_RESAMPLER_PERF6_M_DFSR_ADDR(x),v)
#define HWIO_LPASS_RESAMPLER_PERF6_M_DFSR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_RESAMPLER_PERF6_M_DFSR_ADDR(x),m,v,HWIO_LPASS_RESAMPLER_PERF6_M_DFSR_IN(x))
#define HWIO_LPASS_RESAMPLER_PERF6_M_DFSR_M_BMSK                                                                            0xff
#define HWIO_LPASS_RESAMPLER_PERF6_M_DFSR_M_SHFT                                                                               0

#define HWIO_LPASS_RESAMPLER_PERF7_M_DFSR_ADDR(x)                                                                     ((x) + 0x16054)
#define HWIO_LPASS_RESAMPLER_PERF7_M_DFSR_RMSK                                                                              0xff
#define HWIO_LPASS_RESAMPLER_PERF7_M_DFSR_IN(x)            \
                in_dword(HWIO_LPASS_RESAMPLER_PERF7_M_DFSR_ADDR(x))
#define HWIO_LPASS_RESAMPLER_PERF7_M_DFSR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_RESAMPLER_PERF7_M_DFSR_ADDR(x), m)
#define HWIO_LPASS_RESAMPLER_PERF7_M_DFSR_OUT(x, v)            \
                out_dword(HWIO_LPASS_RESAMPLER_PERF7_M_DFSR_ADDR(x),v)
#define HWIO_LPASS_RESAMPLER_PERF7_M_DFSR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_RESAMPLER_PERF7_M_DFSR_ADDR(x),m,v,HWIO_LPASS_RESAMPLER_PERF7_M_DFSR_IN(x))
#define HWIO_LPASS_RESAMPLER_PERF7_M_DFSR_M_BMSK                                                                            0xff
#define HWIO_LPASS_RESAMPLER_PERF7_M_DFSR_M_SHFT                                                                               0

#define HWIO_LPASS_RESAMPLER_PERF0_N_DFSR_ADDR(x)                                                                     ((x) + 0x16058)
#define HWIO_LPASS_RESAMPLER_PERF0_N_DFSR_RMSK                                                                              0xff
#define HWIO_LPASS_RESAMPLER_PERF0_N_DFSR_IN(x)            \
                in_dword(HWIO_LPASS_RESAMPLER_PERF0_N_DFSR_ADDR(x))
#define HWIO_LPASS_RESAMPLER_PERF0_N_DFSR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_RESAMPLER_PERF0_N_DFSR_ADDR(x), m)
#define HWIO_LPASS_RESAMPLER_PERF0_N_DFSR_OUT(x, v)            \
                out_dword(HWIO_LPASS_RESAMPLER_PERF0_N_DFSR_ADDR(x),v)
#define HWIO_LPASS_RESAMPLER_PERF0_N_DFSR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_RESAMPLER_PERF0_N_DFSR_ADDR(x),m,v,HWIO_LPASS_RESAMPLER_PERF0_N_DFSR_IN(x))
#define HWIO_LPASS_RESAMPLER_PERF0_N_DFSR_NOT_N_MINUS_M_BMSK                                                                0xff
#define HWIO_LPASS_RESAMPLER_PERF0_N_DFSR_NOT_N_MINUS_M_SHFT                                                                   0

#define HWIO_LPASS_RESAMPLER_PERF1_N_DFSR_ADDR(x)                                                                     ((x) + 0x1605c)
#define HWIO_LPASS_RESAMPLER_PERF1_N_DFSR_RMSK                                                                              0xff
#define HWIO_LPASS_RESAMPLER_PERF1_N_DFSR_IN(x)            \
                in_dword(HWIO_LPASS_RESAMPLER_PERF1_N_DFSR_ADDR(x))
#define HWIO_LPASS_RESAMPLER_PERF1_N_DFSR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_RESAMPLER_PERF1_N_DFSR_ADDR(x), m)
#define HWIO_LPASS_RESAMPLER_PERF1_N_DFSR_OUT(x, v)            \
                out_dword(HWIO_LPASS_RESAMPLER_PERF1_N_DFSR_ADDR(x),v)
#define HWIO_LPASS_RESAMPLER_PERF1_N_DFSR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_RESAMPLER_PERF1_N_DFSR_ADDR(x),m,v,HWIO_LPASS_RESAMPLER_PERF1_N_DFSR_IN(x))
#define HWIO_LPASS_RESAMPLER_PERF1_N_DFSR_NOT_N_MINUS_M_BMSK                                                                0xff
#define HWIO_LPASS_RESAMPLER_PERF1_N_DFSR_NOT_N_MINUS_M_SHFT                                                                   0

#define HWIO_LPASS_RESAMPLER_PERF2_N_DFSR_ADDR(x)                                                                     ((x) + 0x16060)
#define HWIO_LPASS_RESAMPLER_PERF2_N_DFSR_RMSK                                                                              0xff
#define HWIO_LPASS_RESAMPLER_PERF2_N_DFSR_IN(x)            \
                in_dword(HWIO_LPASS_RESAMPLER_PERF2_N_DFSR_ADDR(x))
#define HWIO_LPASS_RESAMPLER_PERF2_N_DFSR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_RESAMPLER_PERF2_N_DFSR_ADDR(x), m)
#define HWIO_LPASS_RESAMPLER_PERF2_N_DFSR_OUT(x, v)            \
                out_dword(HWIO_LPASS_RESAMPLER_PERF2_N_DFSR_ADDR(x),v)
#define HWIO_LPASS_RESAMPLER_PERF2_N_DFSR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_RESAMPLER_PERF2_N_DFSR_ADDR(x),m,v,HWIO_LPASS_RESAMPLER_PERF2_N_DFSR_IN(x))
#define HWIO_LPASS_RESAMPLER_PERF2_N_DFSR_NOT_N_MINUS_M_BMSK                                                                0xff
#define HWIO_LPASS_RESAMPLER_PERF2_N_DFSR_NOT_N_MINUS_M_SHFT                                                                   0

#define HWIO_LPASS_RESAMPLER_PERF3_N_DFSR_ADDR(x)                                                                     ((x) + 0x16064)
#define HWIO_LPASS_RESAMPLER_PERF3_N_DFSR_RMSK                                                                              0xff
#define HWIO_LPASS_RESAMPLER_PERF3_N_DFSR_IN(x)            \
                in_dword(HWIO_LPASS_RESAMPLER_PERF3_N_DFSR_ADDR(x))
#define HWIO_LPASS_RESAMPLER_PERF3_N_DFSR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_RESAMPLER_PERF3_N_DFSR_ADDR(x), m)
#define HWIO_LPASS_RESAMPLER_PERF3_N_DFSR_OUT(x, v)            \
                out_dword(HWIO_LPASS_RESAMPLER_PERF3_N_DFSR_ADDR(x),v)
#define HWIO_LPASS_RESAMPLER_PERF3_N_DFSR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_RESAMPLER_PERF3_N_DFSR_ADDR(x),m,v,HWIO_LPASS_RESAMPLER_PERF3_N_DFSR_IN(x))
#define HWIO_LPASS_RESAMPLER_PERF3_N_DFSR_NOT_N_MINUS_M_BMSK                                                                0xff
#define HWIO_LPASS_RESAMPLER_PERF3_N_DFSR_NOT_N_MINUS_M_SHFT                                                                   0

#define HWIO_LPASS_RESAMPLER_PERF4_N_DFSR_ADDR(x)                                                                     ((x) + 0x16068)
#define HWIO_LPASS_RESAMPLER_PERF4_N_DFSR_RMSK                                                                              0xff
#define HWIO_LPASS_RESAMPLER_PERF4_N_DFSR_IN(x)            \
                in_dword(HWIO_LPASS_RESAMPLER_PERF4_N_DFSR_ADDR(x))
#define HWIO_LPASS_RESAMPLER_PERF4_N_DFSR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_RESAMPLER_PERF4_N_DFSR_ADDR(x), m)
#define HWIO_LPASS_RESAMPLER_PERF4_N_DFSR_OUT(x, v)            \
                out_dword(HWIO_LPASS_RESAMPLER_PERF4_N_DFSR_ADDR(x),v)
#define HWIO_LPASS_RESAMPLER_PERF4_N_DFSR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_RESAMPLER_PERF4_N_DFSR_ADDR(x),m,v,HWIO_LPASS_RESAMPLER_PERF4_N_DFSR_IN(x))
#define HWIO_LPASS_RESAMPLER_PERF4_N_DFSR_NOT_N_MINUS_M_BMSK                                                                0xff
#define HWIO_LPASS_RESAMPLER_PERF4_N_DFSR_NOT_N_MINUS_M_SHFT                                                                   0

#define HWIO_LPASS_RESAMPLER_PERF5_N_DFSR_ADDR(x)                                                                     ((x) + 0x1606c)
#define HWIO_LPASS_RESAMPLER_PERF5_N_DFSR_RMSK                                                                              0xff
#define HWIO_LPASS_RESAMPLER_PERF5_N_DFSR_IN(x)            \
                in_dword(HWIO_LPASS_RESAMPLER_PERF5_N_DFSR_ADDR(x))
#define HWIO_LPASS_RESAMPLER_PERF5_N_DFSR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_RESAMPLER_PERF5_N_DFSR_ADDR(x), m)
#define HWIO_LPASS_RESAMPLER_PERF5_N_DFSR_OUT(x, v)            \
                out_dword(HWIO_LPASS_RESAMPLER_PERF5_N_DFSR_ADDR(x),v)
#define HWIO_LPASS_RESAMPLER_PERF5_N_DFSR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_RESAMPLER_PERF5_N_DFSR_ADDR(x),m,v,HWIO_LPASS_RESAMPLER_PERF5_N_DFSR_IN(x))
#define HWIO_LPASS_RESAMPLER_PERF5_N_DFSR_NOT_N_MINUS_M_BMSK                                                                0xff
#define HWIO_LPASS_RESAMPLER_PERF5_N_DFSR_NOT_N_MINUS_M_SHFT                                                                   0

#define HWIO_LPASS_RESAMPLER_PERF6_N_DFSR_ADDR(x)                                                                     ((x) + 0x16070)
#define HWIO_LPASS_RESAMPLER_PERF6_N_DFSR_RMSK                                                                              0xff
#define HWIO_LPASS_RESAMPLER_PERF6_N_DFSR_IN(x)            \
                in_dword(HWIO_LPASS_RESAMPLER_PERF6_N_DFSR_ADDR(x))
#define HWIO_LPASS_RESAMPLER_PERF6_N_DFSR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_RESAMPLER_PERF6_N_DFSR_ADDR(x), m)
#define HWIO_LPASS_RESAMPLER_PERF6_N_DFSR_OUT(x, v)            \
                out_dword(HWIO_LPASS_RESAMPLER_PERF6_N_DFSR_ADDR(x),v)
#define HWIO_LPASS_RESAMPLER_PERF6_N_DFSR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_RESAMPLER_PERF6_N_DFSR_ADDR(x),m,v,HWIO_LPASS_RESAMPLER_PERF6_N_DFSR_IN(x))
#define HWIO_LPASS_RESAMPLER_PERF6_N_DFSR_NOT_N_MINUS_M_BMSK                                                                0xff
#define HWIO_LPASS_RESAMPLER_PERF6_N_DFSR_NOT_N_MINUS_M_SHFT                                                                   0

#define HWIO_LPASS_RESAMPLER_PERF7_N_DFSR_ADDR(x)                                                                     ((x) + 0x16074)
#define HWIO_LPASS_RESAMPLER_PERF7_N_DFSR_RMSK                                                                              0xff
#define HWIO_LPASS_RESAMPLER_PERF7_N_DFSR_IN(x)            \
                in_dword(HWIO_LPASS_RESAMPLER_PERF7_N_DFSR_ADDR(x))
#define HWIO_LPASS_RESAMPLER_PERF7_N_DFSR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_RESAMPLER_PERF7_N_DFSR_ADDR(x), m)
#define HWIO_LPASS_RESAMPLER_PERF7_N_DFSR_OUT(x, v)            \
                out_dword(HWIO_LPASS_RESAMPLER_PERF7_N_DFSR_ADDR(x),v)
#define HWIO_LPASS_RESAMPLER_PERF7_N_DFSR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_RESAMPLER_PERF7_N_DFSR_ADDR(x),m,v,HWIO_LPASS_RESAMPLER_PERF7_N_DFSR_IN(x))
#define HWIO_LPASS_RESAMPLER_PERF7_N_DFSR_NOT_N_MINUS_M_BMSK                                                                0xff
#define HWIO_LPASS_RESAMPLER_PERF7_N_DFSR_NOT_N_MINUS_M_SHFT                                                                   0

#define HWIO_LPASS_RESAMPLER_PERF0_D_DFSR_ADDR(x)                                                                     ((x) + 0x16078)
#define HWIO_LPASS_RESAMPLER_PERF0_D_DFSR_RMSK                                                                              0xff
#define HWIO_LPASS_RESAMPLER_PERF0_D_DFSR_IN(x)            \
                in_dword(HWIO_LPASS_RESAMPLER_PERF0_D_DFSR_ADDR(x))
#define HWIO_LPASS_RESAMPLER_PERF0_D_DFSR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_RESAMPLER_PERF0_D_DFSR_ADDR(x), m)
#define HWIO_LPASS_RESAMPLER_PERF0_D_DFSR_OUT(x, v)            \
                out_dword(HWIO_LPASS_RESAMPLER_PERF0_D_DFSR_ADDR(x),v)
#define HWIO_LPASS_RESAMPLER_PERF0_D_DFSR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_RESAMPLER_PERF0_D_DFSR_ADDR(x),m,v,HWIO_LPASS_RESAMPLER_PERF0_D_DFSR_IN(x))
#define HWIO_LPASS_RESAMPLER_PERF0_D_DFSR_NOT_2D_BMSK                                                                       0xff
#define HWIO_LPASS_RESAMPLER_PERF0_D_DFSR_NOT_2D_SHFT                                                                          0

#define HWIO_LPASS_RESAMPLER_PERF1_D_DFSR_ADDR(x)                                                                     ((x) + 0x1607c)
#define HWIO_LPASS_RESAMPLER_PERF1_D_DFSR_RMSK                                                                              0xff
#define HWIO_LPASS_RESAMPLER_PERF1_D_DFSR_IN(x)            \
                in_dword(HWIO_LPASS_RESAMPLER_PERF1_D_DFSR_ADDR(x))
#define HWIO_LPASS_RESAMPLER_PERF1_D_DFSR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_RESAMPLER_PERF1_D_DFSR_ADDR(x), m)
#define HWIO_LPASS_RESAMPLER_PERF1_D_DFSR_OUT(x, v)            \
                out_dword(HWIO_LPASS_RESAMPLER_PERF1_D_DFSR_ADDR(x),v)
#define HWIO_LPASS_RESAMPLER_PERF1_D_DFSR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_RESAMPLER_PERF1_D_DFSR_ADDR(x),m,v,HWIO_LPASS_RESAMPLER_PERF1_D_DFSR_IN(x))
#define HWIO_LPASS_RESAMPLER_PERF1_D_DFSR_NOT_2D_BMSK                                                                       0xff
#define HWIO_LPASS_RESAMPLER_PERF1_D_DFSR_NOT_2D_SHFT                                                                          0

#define HWIO_LPASS_RESAMPLER_PERF2_D_DFSR_ADDR(x)                                                                     ((x) + 0x16080)
#define HWIO_LPASS_RESAMPLER_PERF2_D_DFSR_RMSK                                                                              0xff
#define HWIO_LPASS_RESAMPLER_PERF2_D_DFSR_IN(x)            \
                in_dword(HWIO_LPASS_RESAMPLER_PERF2_D_DFSR_ADDR(x))
#define HWIO_LPASS_RESAMPLER_PERF2_D_DFSR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_RESAMPLER_PERF2_D_DFSR_ADDR(x), m)
#define HWIO_LPASS_RESAMPLER_PERF2_D_DFSR_OUT(x, v)            \
                out_dword(HWIO_LPASS_RESAMPLER_PERF2_D_DFSR_ADDR(x),v)
#define HWIO_LPASS_RESAMPLER_PERF2_D_DFSR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_RESAMPLER_PERF2_D_DFSR_ADDR(x),m,v,HWIO_LPASS_RESAMPLER_PERF2_D_DFSR_IN(x))
#define HWIO_LPASS_RESAMPLER_PERF2_D_DFSR_NOT_2D_BMSK                                                                       0xff
#define HWIO_LPASS_RESAMPLER_PERF2_D_DFSR_NOT_2D_SHFT                                                                          0

#define HWIO_LPASS_RESAMPLER_PERF3_D_DFSR_ADDR(x)                                                                     ((x) + 0x16084)
#define HWIO_LPASS_RESAMPLER_PERF3_D_DFSR_RMSK                                                                              0xff
#define HWIO_LPASS_RESAMPLER_PERF3_D_DFSR_IN(x)            \
                in_dword(HWIO_LPASS_RESAMPLER_PERF3_D_DFSR_ADDR(x))
#define HWIO_LPASS_RESAMPLER_PERF3_D_DFSR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_RESAMPLER_PERF3_D_DFSR_ADDR(x), m)
#define HWIO_LPASS_RESAMPLER_PERF3_D_DFSR_OUT(x, v)            \
                out_dword(HWIO_LPASS_RESAMPLER_PERF3_D_DFSR_ADDR(x),v)
#define HWIO_LPASS_RESAMPLER_PERF3_D_DFSR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_RESAMPLER_PERF3_D_DFSR_ADDR(x),m,v,HWIO_LPASS_RESAMPLER_PERF3_D_DFSR_IN(x))
#define HWIO_LPASS_RESAMPLER_PERF3_D_DFSR_NOT_2D_BMSK                                                                       0xff
#define HWIO_LPASS_RESAMPLER_PERF3_D_DFSR_NOT_2D_SHFT                                                                          0

#define HWIO_LPASS_RESAMPLER_PERF4_D_DFSR_ADDR(x)                                                                     ((x) + 0x16088)
#define HWIO_LPASS_RESAMPLER_PERF4_D_DFSR_RMSK                                                                              0xff
#define HWIO_LPASS_RESAMPLER_PERF4_D_DFSR_IN(x)            \
                in_dword(HWIO_LPASS_RESAMPLER_PERF4_D_DFSR_ADDR(x))
#define HWIO_LPASS_RESAMPLER_PERF4_D_DFSR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_RESAMPLER_PERF4_D_DFSR_ADDR(x), m)
#define HWIO_LPASS_RESAMPLER_PERF4_D_DFSR_OUT(x, v)            \
                out_dword(HWIO_LPASS_RESAMPLER_PERF4_D_DFSR_ADDR(x),v)
#define HWIO_LPASS_RESAMPLER_PERF4_D_DFSR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_RESAMPLER_PERF4_D_DFSR_ADDR(x),m,v,HWIO_LPASS_RESAMPLER_PERF4_D_DFSR_IN(x))
#define HWIO_LPASS_RESAMPLER_PERF4_D_DFSR_NOT_2D_BMSK                                                                       0xff
#define HWIO_LPASS_RESAMPLER_PERF4_D_DFSR_NOT_2D_SHFT                                                                          0

#define HWIO_LPASS_RESAMPLER_PERF5_D_DFSR_ADDR(x)                                                                     ((x) + 0x1608c)
#define HWIO_LPASS_RESAMPLER_PERF5_D_DFSR_RMSK                                                                              0xff
#define HWIO_LPASS_RESAMPLER_PERF5_D_DFSR_IN(x)            \
                in_dword(HWIO_LPASS_RESAMPLER_PERF5_D_DFSR_ADDR(x))
#define HWIO_LPASS_RESAMPLER_PERF5_D_DFSR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_RESAMPLER_PERF5_D_DFSR_ADDR(x), m)
#define HWIO_LPASS_RESAMPLER_PERF5_D_DFSR_OUT(x, v)            \
                out_dword(HWIO_LPASS_RESAMPLER_PERF5_D_DFSR_ADDR(x),v)
#define HWIO_LPASS_RESAMPLER_PERF5_D_DFSR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_RESAMPLER_PERF5_D_DFSR_ADDR(x),m,v,HWIO_LPASS_RESAMPLER_PERF5_D_DFSR_IN(x))
#define HWIO_LPASS_RESAMPLER_PERF5_D_DFSR_NOT_2D_BMSK                                                                       0xff
#define HWIO_LPASS_RESAMPLER_PERF5_D_DFSR_NOT_2D_SHFT                                                                          0

#define HWIO_LPASS_RESAMPLER_PERF6_D_DFSR_ADDR(x)                                                                     ((x) + 0x16090)
#define HWIO_LPASS_RESAMPLER_PERF6_D_DFSR_RMSK                                                                              0xff
#define HWIO_LPASS_RESAMPLER_PERF6_D_DFSR_IN(x)            \
                in_dword(HWIO_LPASS_RESAMPLER_PERF6_D_DFSR_ADDR(x))
#define HWIO_LPASS_RESAMPLER_PERF6_D_DFSR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_RESAMPLER_PERF6_D_DFSR_ADDR(x), m)
#define HWIO_LPASS_RESAMPLER_PERF6_D_DFSR_OUT(x, v)            \
                out_dword(HWIO_LPASS_RESAMPLER_PERF6_D_DFSR_ADDR(x),v)
#define HWIO_LPASS_RESAMPLER_PERF6_D_DFSR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_RESAMPLER_PERF6_D_DFSR_ADDR(x),m,v,HWIO_LPASS_RESAMPLER_PERF6_D_DFSR_IN(x))
#define HWIO_LPASS_RESAMPLER_PERF6_D_DFSR_NOT_2D_BMSK                                                                       0xff
#define HWIO_LPASS_RESAMPLER_PERF6_D_DFSR_NOT_2D_SHFT                                                                          0

#define HWIO_LPASS_RESAMPLER_PERF7_D_DFSR_ADDR(x)                                                                     ((x) + 0x16094)
#define HWIO_LPASS_RESAMPLER_PERF7_D_DFSR_RMSK                                                                              0xff
#define HWIO_LPASS_RESAMPLER_PERF7_D_DFSR_IN(x)            \
                in_dword(HWIO_LPASS_RESAMPLER_PERF7_D_DFSR_ADDR(x))
#define HWIO_LPASS_RESAMPLER_PERF7_D_DFSR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_RESAMPLER_PERF7_D_DFSR_ADDR(x), m)
#define HWIO_LPASS_RESAMPLER_PERF7_D_DFSR_OUT(x, v)            \
                out_dword(HWIO_LPASS_RESAMPLER_PERF7_D_DFSR_ADDR(x),v)
#define HWIO_LPASS_RESAMPLER_PERF7_D_DFSR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_RESAMPLER_PERF7_D_DFSR_ADDR(x),m,v,HWIO_LPASS_RESAMPLER_PERF7_D_DFSR_IN(x))
#define HWIO_LPASS_RESAMPLER_PERF7_D_DFSR_NOT_2D_BMSK                                                                       0xff
#define HWIO_LPASS_RESAMPLER_PERF7_D_DFSR_NOT_2D_SHFT                                                                          0

#define HWIO_LPASS_AUDIO_CORE_RESAMPLER_CBCR_ADDR(x)                                                                  ((x) + 0x16098)
#define HWIO_LPASS_AUDIO_CORE_RESAMPLER_CBCR_RMSK                                                                     0x80007ff3
#define HWIO_LPASS_AUDIO_CORE_RESAMPLER_CBCR_IN(x)            \
                in_dword(HWIO_LPASS_AUDIO_CORE_RESAMPLER_CBCR_ADDR(x))
#define HWIO_LPASS_AUDIO_CORE_RESAMPLER_CBCR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AUDIO_CORE_RESAMPLER_CBCR_ADDR(x), m)
#define HWIO_LPASS_AUDIO_CORE_RESAMPLER_CBCR_OUT(x, v)            \
                out_dword(HWIO_LPASS_AUDIO_CORE_RESAMPLER_CBCR_ADDR(x),v)
#define HWIO_LPASS_AUDIO_CORE_RESAMPLER_CBCR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AUDIO_CORE_RESAMPLER_CBCR_ADDR(x),m,v,HWIO_LPASS_AUDIO_CORE_RESAMPLER_CBCR_IN(x))
#define HWIO_LPASS_AUDIO_CORE_RESAMPLER_CBCR_CLK_OFF_BMSK                                                             0x80000000
#define HWIO_LPASS_AUDIO_CORE_RESAMPLER_CBCR_CLK_OFF_SHFT                                                                     31
#define HWIO_LPASS_AUDIO_CORE_RESAMPLER_CBCR_FORCE_MEM_CORE_ON_BMSK                                                       0x4000
#define HWIO_LPASS_AUDIO_CORE_RESAMPLER_CBCR_FORCE_MEM_CORE_ON_SHFT                                                           14
#define HWIO_LPASS_AUDIO_CORE_RESAMPLER_CBCR_FORCE_MEM_PERIPH_ON_BMSK                                                     0x2000
#define HWIO_LPASS_AUDIO_CORE_RESAMPLER_CBCR_FORCE_MEM_PERIPH_ON_SHFT                                                         13
#define HWIO_LPASS_AUDIO_CORE_RESAMPLER_CBCR_FORCE_MEM_PERIPH_OFF_BMSK                                                    0x1000
#define HWIO_LPASS_AUDIO_CORE_RESAMPLER_CBCR_FORCE_MEM_PERIPH_OFF_SHFT                                                        12
#define HWIO_LPASS_AUDIO_CORE_RESAMPLER_CBCR_WAKEUP_BMSK                                                                   0xf00
#define HWIO_LPASS_AUDIO_CORE_RESAMPLER_CBCR_WAKEUP_SHFT                                                                       8
#define HWIO_LPASS_AUDIO_CORE_RESAMPLER_CBCR_SLEEP_BMSK                                                                     0xf0
#define HWIO_LPASS_AUDIO_CORE_RESAMPLER_CBCR_SLEEP_SHFT                                                                        4
#define HWIO_LPASS_AUDIO_CORE_RESAMPLER_CBCR_HW_CTL_BMSK                                                                     0x2
#define HWIO_LPASS_AUDIO_CORE_RESAMPLER_CBCR_HW_CTL_SHFT                                                                       1
#define HWIO_LPASS_AUDIO_CORE_RESAMPLER_CBCR_CLK_ENABLE_BMSK                                                                 0x1
#define HWIO_LPASS_AUDIO_CORE_RESAMPLER_CBCR_CLK_ENABLE_SHFT                                                                   0

#define HWIO_LPASS_AUD_SLIMBUS_CMD_RCGR_ADDR(x)                                                                       ((x) + 0x17000)
#define HWIO_LPASS_AUD_SLIMBUS_CMD_RCGR_RMSK                                                                          0x800000f3
#define HWIO_LPASS_AUD_SLIMBUS_CMD_RCGR_IN(x)            \
                in_dword(HWIO_LPASS_AUD_SLIMBUS_CMD_RCGR_ADDR(x))
#define HWIO_LPASS_AUD_SLIMBUS_CMD_RCGR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AUD_SLIMBUS_CMD_RCGR_ADDR(x), m)
#define HWIO_LPASS_AUD_SLIMBUS_CMD_RCGR_OUT(x, v)            \
                out_dword(HWIO_LPASS_AUD_SLIMBUS_CMD_RCGR_ADDR(x),v)
#define HWIO_LPASS_AUD_SLIMBUS_CMD_RCGR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AUD_SLIMBUS_CMD_RCGR_ADDR(x),m,v,HWIO_LPASS_AUD_SLIMBUS_CMD_RCGR_IN(x))
#define HWIO_LPASS_AUD_SLIMBUS_CMD_RCGR_ROOT_OFF_BMSK                                                                 0x80000000
#define HWIO_LPASS_AUD_SLIMBUS_CMD_RCGR_ROOT_OFF_SHFT                                                                         31
#define HWIO_LPASS_AUD_SLIMBUS_CMD_RCGR_DIRTY_D_BMSK                                                                        0x80
#define HWIO_LPASS_AUD_SLIMBUS_CMD_RCGR_DIRTY_D_SHFT                                                                           7
#define HWIO_LPASS_AUD_SLIMBUS_CMD_RCGR_DIRTY_N_BMSK                                                                        0x40
#define HWIO_LPASS_AUD_SLIMBUS_CMD_RCGR_DIRTY_N_SHFT                                                                           6
#define HWIO_LPASS_AUD_SLIMBUS_CMD_RCGR_DIRTY_M_BMSK                                                                        0x20
#define HWIO_LPASS_AUD_SLIMBUS_CMD_RCGR_DIRTY_M_SHFT                                                                           5
#define HWIO_LPASS_AUD_SLIMBUS_CMD_RCGR_DIRTY_CFG_RCGR_BMSK                                                                 0x10
#define HWIO_LPASS_AUD_SLIMBUS_CMD_RCGR_DIRTY_CFG_RCGR_SHFT                                                                    4
#define HWIO_LPASS_AUD_SLIMBUS_CMD_RCGR_ROOT_EN_BMSK                                                                         0x2
#define HWIO_LPASS_AUD_SLIMBUS_CMD_RCGR_ROOT_EN_SHFT                                                                           1
#define HWIO_LPASS_AUD_SLIMBUS_CMD_RCGR_UPDATE_BMSK                                                                          0x1
#define HWIO_LPASS_AUD_SLIMBUS_CMD_RCGR_UPDATE_SHFT                                                                            0

#define HWIO_LPASS_AUD_SLIMBUS_CFG_RCGR_ADDR(x)                                                                       ((x) + 0x17004)
#define HWIO_LPASS_AUD_SLIMBUS_CFG_RCGR_RMSK                                                                              0x771f
#define HWIO_LPASS_AUD_SLIMBUS_CFG_RCGR_IN(x)            \
                in_dword(HWIO_LPASS_AUD_SLIMBUS_CFG_RCGR_ADDR(x))
#define HWIO_LPASS_AUD_SLIMBUS_CFG_RCGR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AUD_SLIMBUS_CFG_RCGR_ADDR(x), m)
#define HWIO_LPASS_AUD_SLIMBUS_CFG_RCGR_OUT(x, v)            \
                out_dword(HWIO_LPASS_AUD_SLIMBUS_CFG_RCGR_ADDR(x),v)
#define HWIO_LPASS_AUD_SLIMBUS_CFG_RCGR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AUD_SLIMBUS_CFG_RCGR_ADDR(x),m,v,HWIO_LPASS_AUD_SLIMBUS_CFG_RCGR_IN(x))
#define HWIO_LPASS_AUD_SLIMBUS_CFG_RCGR_ALT_SRC_SEL_BMSK                                                                  0x4000
#define HWIO_LPASS_AUD_SLIMBUS_CFG_RCGR_ALT_SRC_SEL_SHFT                                                                      14
#define HWIO_LPASS_AUD_SLIMBUS_CFG_RCGR_MODE_BMSK                                                                         0x3000
#define HWIO_LPASS_AUD_SLIMBUS_CFG_RCGR_MODE_SHFT                                                                             12
#define HWIO_LPASS_AUD_SLIMBUS_CFG_RCGR_SRC_SEL_BMSK                                                                       0x700
#define HWIO_LPASS_AUD_SLIMBUS_CFG_RCGR_SRC_SEL_SHFT                                                                           8
#define HWIO_LPASS_AUD_SLIMBUS_CFG_RCGR_SRC_DIV_BMSK                                                                        0x1f
#define HWIO_LPASS_AUD_SLIMBUS_CFG_RCGR_SRC_DIV_SHFT                                                                           0

#define HWIO_LPASS_AUD_SLIMBUS_M_ADDR(x)                                                                              ((x) + 0x17008)
#define HWIO_LPASS_AUD_SLIMBUS_M_RMSK                                                                                       0xff
#define HWIO_LPASS_AUD_SLIMBUS_M_IN(x)            \
                in_dword(HWIO_LPASS_AUD_SLIMBUS_M_ADDR(x))
#define HWIO_LPASS_AUD_SLIMBUS_M_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AUD_SLIMBUS_M_ADDR(x), m)
#define HWIO_LPASS_AUD_SLIMBUS_M_OUT(x, v)            \
                out_dword(HWIO_LPASS_AUD_SLIMBUS_M_ADDR(x),v)
#define HWIO_LPASS_AUD_SLIMBUS_M_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AUD_SLIMBUS_M_ADDR(x),m,v,HWIO_LPASS_AUD_SLIMBUS_M_IN(x))
#define HWIO_LPASS_AUD_SLIMBUS_M_M_BMSK                                                                                     0xff
#define HWIO_LPASS_AUD_SLIMBUS_M_M_SHFT                                                                                        0

#define HWIO_LPASS_AUD_SLIMBUS_N_ADDR(x)                                                                              ((x) + 0x1700c)
#define HWIO_LPASS_AUD_SLIMBUS_N_RMSK                                                                                       0xff
#define HWIO_LPASS_AUD_SLIMBUS_N_IN(x)            \
                in_dword(HWIO_LPASS_AUD_SLIMBUS_N_ADDR(x))
#define HWIO_LPASS_AUD_SLIMBUS_N_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AUD_SLIMBUS_N_ADDR(x), m)
#define HWIO_LPASS_AUD_SLIMBUS_N_OUT(x, v)            \
                out_dword(HWIO_LPASS_AUD_SLIMBUS_N_ADDR(x),v)
#define HWIO_LPASS_AUD_SLIMBUS_N_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AUD_SLIMBUS_N_ADDR(x),m,v,HWIO_LPASS_AUD_SLIMBUS_N_IN(x))
#define HWIO_LPASS_AUD_SLIMBUS_N_NOT_N_MINUS_M_BMSK                                                                         0xff
#define HWIO_LPASS_AUD_SLIMBUS_N_NOT_N_MINUS_M_SHFT                                                                            0

#define HWIO_LPASS_AUD_SLIMBUS_D_ADDR(x)                                                                              ((x) + 0x17010)
#define HWIO_LPASS_AUD_SLIMBUS_D_RMSK                                                                                       0xff
#define HWIO_LPASS_AUD_SLIMBUS_D_IN(x)            \
                in_dword(HWIO_LPASS_AUD_SLIMBUS_D_ADDR(x))
#define HWIO_LPASS_AUD_SLIMBUS_D_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AUD_SLIMBUS_D_ADDR(x), m)
#define HWIO_LPASS_AUD_SLIMBUS_D_OUT(x, v)            \
                out_dword(HWIO_LPASS_AUD_SLIMBUS_D_ADDR(x),v)
#define HWIO_LPASS_AUD_SLIMBUS_D_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AUD_SLIMBUS_D_ADDR(x),m,v,HWIO_LPASS_AUD_SLIMBUS_D_IN(x))
#define HWIO_LPASS_AUD_SLIMBUS_D_NOT_2D_BMSK                                                                                0xff
#define HWIO_LPASS_AUD_SLIMBUS_D_NOT_2D_SHFT                                                                                   0

#define HWIO_LPASS_AUDIO_CORE_AUD_SLIMBUS_CBCR_ADDR(x)                                                                ((x) + 0x17014)
#define HWIO_LPASS_AUDIO_CORE_AUD_SLIMBUS_CBCR_RMSK                                                                   0x80000003
#define HWIO_LPASS_AUDIO_CORE_AUD_SLIMBUS_CBCR_IN(x)            \
                in_dword(HWIO_LPASS_AUDIO_CORE_AUD_SLIMBUS_CBCR_ADDR(x))
#define HWIO_LPASS_AUDIO_CORE_AUD_SLIMBUS_CBCR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AUDIO_CORE_AUD_SLIMBUS_CBCR_ADDR(x), m)
#define HWIO_LPASS_AUDIO_CORE_AUD_SLIMBUS_CBCR_OUT(x, v)            \
                out_dword(HWIO_LPASS_AUDIO_CORE_AUD_SLIMBUS_CBCR_ADDR(x),v)
#define HWIO_LPASS_AUDIO_CORE_AUD_SLIMBUS_CBCR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AUDIO_CORE_AUD_SLIMBUS_CBCR_ADDR(x),m,v,HWIO_LPASS_AUDIO_CORE_AUD_SLIMBUS_CBCR_IN(x))
#define HWIO_LPASS_AUDIO_CORE_AUD_SLIMBUS_CBCR_CLK_OFF_BMSK                                                           0x80000000
#define HWIO_LPASS_AUDIO_CORE_AUD_SLIMBUS_CBCR_CLK_OFF_SHFT                                                                   31
#define HWIO_LPASS_AUDIO_CORE_AUD_SLIMBUS_CBCR_HW_CTL_BMSK                                                                   0x2
#define HWIO_LPASS_AUDIO_CORE_AUD_SLIMBUS_CBCR_HW_CTL_SHFT                                                                     1
#define HWIO_LPASS_AUDIO_CORE_AUD_SLIMBUS_CBCR_CLK_ENABLE_BMSK                                                               0x1
#define HWIO_LPASS_AUDIO_CORE_AUD_SLIMBUS_CBCR_CLK_ENABLE_SHFT                                                                 0

#define HWIO_LPASS_AUDIO_CORE_AUD_SLIMBUS_CORE_CBCR_ADDR(x)                                                           ((x) + 0x17018)
#define HWIO_LPASS_AUDIO_CORE_AUD_SLIMBUS_CORE_CBCR_RMSK                                                              0x80007ff3
#define HWIO_LPASS_AUDIO_CORE_AUD_SLIMBUS_CORE_CBCR_IN(x)            \
                in_dword(HWIO_LPASS_AUDIO_CORE_AUD_SLIMBUS_CORE_CBCR_ADDR(x))
#define HWIO_LPASS_AUDIO_CORE_AUD_SLIMBUS_CORE_CBCR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AUDIO_CORE_AUD_SLIMBUS_CORE_CBCR_ADDR(x), m)
#define HWIO_LPASS_AUDIO_CORE_AUD_SLIMBUS_CORE_CBCR_OUT(x, v)            \
                out_dword(HWIO_LPASS_AUDIO_CORE_AUD_SLIMBUS_CORE_CBCR_ADDR(x),v)
#define HWIO_LPASS_AUDIO_CORE_AUD_SLIMBUS_CORE_CBCR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AUDIO_CORE_AUD_SLIMBUS_CORE_CBCR_ADDR(x),m,v,HWIO_LPASS_AUDIO_CORE_AUD_SLIMBUS_CORE_CBCR_IN(x))
#define HWIO_LPASS_AUDIO_CORE_AUD_SLIMBUS_CORE_CBCR_CLK_OFF_BMSK                                                      0x80000000
#define HWIO_LPASS_AUDIO_CORE_AUD_SLIMBUS_CORE_CBCR_CLK_OFF_SHFT                                                              31
#define HWIO_LPASS_AUDIO_CORE_AUD_SLIMBUS_CORE_CBCR_FORCE_MEM_CORE_ON_BMSK                                                0x4000
#define HWIO_LPASS_AUDIO_CORE_AUD_SLIMBUS_CORE_CBCR_FORCE_MEM_CORE_ON_SHFT                                                    14
#define HWIO_LPASS_AUDIO_CORE_AUD_SLIMBUS_CORE_CBCR_FORCE_MEM_PERIPH_ON_BMSK                                              0x2000
#define HWIO_LPASS_AUDIO_CORE_AUD_SLIMBUS_CORE_CBCR_FORCE_MEM_PERIPH_ON_SHFT                                                  13
#define HWIO_LPASS_AUDIO_CORE_AUD_SLIMBUS_CORE_CBCR_FORCE_MEM_PERIPH_OFF_BMSK                                             0x1000
#define HWIO_LPASS_AUDIO_CORE_AUD_SLIMBUS_CORE_CBCR_FORCE_MEM_PERIPH_OFF_SHFT                                                 12
#define HWIO_LPASS_AUDIO_CORE_AUD_SLIMBUS_CORE_CBCR_WAKEUP_BMSK                                                            0xf00
#define HWIO_LPASS_AUDIO_CORE_AUD_SLIMBUS_CORE_CBCR_WAKEUP_SHFT                                                                8
#define HWIO_LPASS_AUDIO_CORE_AUD_SLIMBUS_CORE_CBCR_SLEEP_BMSK                                                              0xf0
#define HWIO_LPASS_AUDIO_CORE_AUD_SLIMBUS_CORE_CBCR_SLEEP_SHFT                                                                 4
#define HWIO_LPASS_AUDIO_CORE_AUD_SLIMBUS_CORE_CBCR_HW_CTL_BMSK                                                              0x2
#define HWIO_LPASS_AUDIO_CORE_AUD_SLIMBUS_CORE_CBCR_HW_CTL_SHFT                                                                1
#define HWIO_LPASS_AUDIO_CORE_AUD_SLIMBUS_CORE_CBCR_CLK_ENABLE_BMSK                                                          0x1
#define HWIO_LPASS_AUDIO_CORE_AUD_SLIMBUS_CORE_CBCR_CLK_ENABLE_SHFT                                                            0

#define HWIO_LPASS_AUDIO_CORE_AUD_SLIMBUS_NPL_CBCR_ADDR(x)                                                            ((x) + 0x1701c)
#define HWIO_LPASS_AUDIO_CORE_AUD_SLIMBUS_NPL_CBCR_RMSK                                                               0x80000003
#define HWIO_LPASS_AUDIO_CORE_AUD_SLIMBUS_NPL_CBCR_IN(x)            \
                in_dword(HWIO_LPASS_AUDIO_CORE_AUD_SLIMBUS_NPL_CBCR_ADDR(x))
#define HWIO_LPASS_AUDIO_CORE_AUD_SLIMBUS_NPL_CBCR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AUDIO_CORE_AUD_SLIMBUS_NPL_CBCR_ADDR(x), m)
#define HWIO_LPASS_AUDIO_CORE_AUD_SLIMBUS_NPL_CBCR_OUT(x, v)            \
                out_dword(HWIO_LPASS_AUDIO_CORE_AUD_SLIMBUS_NPL_CBCR_ADDR(x),v)
#define HWIO_LPASS_AUDIO_CORE_AUD_SLIMBUS_NPL_CBCR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AUDIO_CORE_AUD_SLIMBUS_NPL_CBCR_ADDR(x),m,v,HWIO_LPASS_AUDIO_CORE_AUD_SLIMBUS_NPL_CBCR_IN(x))
#define HWIO_LPASS_AUDIO_CORE_AUD_SLIMBUS_NPL_CBCR_CLK_OFF_BMSK                                                       0x80000000
#define HWIO_LPASS_AUDIO_CORE_AUD_SLIMBUS_NPL_CBCR_CLK_OFF_SHFT                                                               31
#define HWIO_LPASS_AUDIO_CORE_AUD_SLIMBUS_NPL_CBCR_HW_CTL_BMSK                                                               0x2
#define HWIO_LPASS_AUDIO_CORE_AUD_SLIMBUS_NPL_CBCR_HW_CTL_SHFT                                                                 1
#define HWIO_LPASS_AUDIO_CORE_AUD_SLIMBUS_NPL_CBCR_CLK_ENABLE_BMSK                                                           0x1
#define HWIO_LPASS_AUDIO_CORE_AUD_SLIMBUS_NPL_CBCR_CLK_ENABLE_SHFT                                                             0

#define HWIO_LPASS_QCA_SLIMBUS_CMD_RCGR_ADDR(x)                                                                       ((x) + 0x18000)
#define HWIO_LPASS_QCA_SLIMBUS_CMD_RCGR_RMSK                                                                          0x800000f3
#define HWIO_LPASS_QCA_SLIMBUS_CMD_RCGR_IN(x)            \
                in_dword(HWIO_LPASS_QCA_SLIMBUS_CMD_RCGR_ADDR(x))
#define HWIO_LPASS_QCA_SLIMBUS_CMD_RCGR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_QCA_SLIMBUS_CMD_RCGR_ADDR(x), m)
#define HWIO_LPASS_QCA_SLIMBUS_CMD_RCGR_OUT(x, v)            \
                out_dword(HWIO_LPASS_QCA_SLIMBUS_CMD_RCGR_ADDR(x),v)
#define HWIO_LPASS_QCA_SLIMBUS_CMD_RCGR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_QCA_SLIMBUS_CMD_RCGR_ADDR(x),m,v,HWIO_LPASS_QCA_SLIMBUS_CMD_RCGR_IN(x))
#define HWIO_LPASS_QCA_SLIMBUS_CMD_RCGR_ROOT_OFF_BMSK                                                                 0x80000000
#define HWIO_LPASS_QCA_SLIMBUS_CMD_RCGR_ROOT_OFF_SHFT                                                                         31
#define HWIO_LPASS_QCA_SLIMBUS_CMD_RCGR_DIRTY_D_BMSK                                                                        0x80
#define HWIO_LPASS_QCA_SLIMBUS_CMD_RCGR_DIRTY_D_SHFT                                                                           7
#define HWIO_LPASS_QCA_SLIMBUS_CMD_RCGR_DIRTY_N_BMSK                                                                        0x40
#define HWIO_LPASS_QCA_SLIMBUS_CMD_RCGR_DIRTY_N_SHFT                                                                           6
#define HWIO_LPASS_QCA_SLIMBUS_CMD_RCGR_DIRTY_M_BMSK                                                                        0x20
#define HWIO_LPASS_QCA_SLIMBUS_CMD_RCGR_DIRTY_M_SHFT                                                                           5
#define HWIO_LPASS_QCA_SLIMBUS_CMD_RCGR_DIRTY_CFG_RCGR_BMSK                                                                 0x10
#define HWIO_LPASS_QCA_SLIMBUS_CMD_RCGR_DIRTY_CFG_RCGR_SHFT                                                                    4
#define HWIO_LPASS_QCA_SLIMBUS_CMD_RCGR_ROOT_EN_BMSK                                                                         0x2
#define HWIO_LPASS_QCA_SLIMBUS_CMD_RCGR_ROOT_EN_SHFT                                                                           1
#define HWIO_LPASS_QCA_SLIMBUS_CMD_RCGR_UPDATE_BMSK                                                                          0x1
#define HWIO_LPASS_QCA_SLIMBUS_CMD_RCGR_UPDATE_SHFT                                                                            0

#define HWIO_LPASS_QCA_SLIMBUS_CFG_RCGR_ADDR(x)                                                                       ((x) + 0x18004)
#define HWIO_LPASS_QCA_SLIMBUS_CFG_RCGR_RMSK                                                                              0x771f
#define HWIO_LPASS_QCA_SLIMBUS_CFG_RCGR_IN(x)            \
                in_dword(HWIO_LPASS_QCA_SLIMBUS_CFG_RCGR_ADDR(x))
#define HWIO_LPASS_QCA_SLIMBUS_CFG_RCGR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_QCA_SLIMBUS_CFG_RCGR_ADDR(x), m)
#define HWIO_LPASS_QCA_SLIMBUS_CFG_RCGR_OUT(x, v)            \
                out_dword(HWIO_LPASS_QCA_SLIMBUS_CFG_RCGR_ADDR(x),v)
#define HWIO_LPASS_QCA_SLIMBUS_CFG_RCGR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_QCA_SLIMBUS_CFG_RCGR_ADDR(x),m,v,HWIO_LPASS_QCA_SLIMBUS_CFG_RCGR_IN(x))
#define HWIO_LPASS_QCA_SLIMBUS_CFG_RCGR_ALT_SRC_SEL_BMSK                                                                  0x4000
#define HWIO_LPASS_QCA_SLIMBUS_CFG_RCGR_ALT_SRC_SEL_SHFT                                                                      14
#define HWIO_LPASS_QCA_SLIMBUS_CFG_RCGR_MODE_BMSK                                                                         0x3000
#define HWIO_LPASS_QCA_SLIMBUS_CFG_RCGR_MODE_SHFT                                                                             12
#define HWIO_LPASS_QCA_SLIMBUS_CFG_RCGR_SRC_SEL_BMSK                                                                       0x700
#define HWIO_LPASS_QCA_SLIMBUS_CFG_RCGR_SRC_SEL_SHFT                                                                           8
#define HWIO_LPASS_QCA_SLIMBUS_CFG_RCGR_SRC_DIV_BMSK                                                                        0x1f
#define HWIO_LPASS_QCA_SLIMBUS_CFG_RCGR_SRC_DIV_SHFT                                                                           0

#define HWIO_LPASS_QCA_SLIMBUS_M_ADDR(x)                                                                              ((x) + 0x18008)
#define HWIO_LPASS_QCA_SLIMBUS_M_RMSK                                                                                       0xff
#define HWIO_LPASS_QCA_SLIMBUS_M_IN(x)            \
                in_dword(HWIO_LPASS_QCA_SLIMBUS_M_ADDR(x))
#define HWIO_LPASS_QCA_SLIMBUS_M_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_QCA_SLIMBUS_M_ADDR(x), m)
#define HWIO_LPASS_QCA_SLIMBUS_M_OUT(x, v)            \
                out_dword(HWIO_LPASS_QCA_SLIMBUS_M_ADDR(x),v)
#define HWIO_LPASS_QCA_SLIMBUS_M_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_QCA_SLIMBUS_M_ADDR(x),m,v,HWIO_LPASS_QCA_SLIMBUS_M_IN(x))
#define HWIO_LPASS_QCA_SLIMBUS_M_M_BMSK                                                                                     0xff
#define HWIO_LPASS_QCA_SLIMBUS_M_M_SHFT                                                                                        0

#define HWIO_LPASS_QCA_SLIMBUS_N_ADDR(x)                                                                              ((x) + 0x1800c)
#define HWIO_LPASS_QCA_SLIMBUS_N_RMSK                                                                                       0xff
#define HWIO_LPASS_QCA_SLIMBUS_N_IN(x)            \
                in_dword(HWIO_LPASS_QCA_SLIMBUS_N_ADDR(x))
#define HWIO_LPASS_QCA_SLIMBUS_N_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_QCA_SLIMBUS_N_ADDR(x), m)
#define HWIO_LPASS_QCA_SLIMBUS_N_OUT(x, v)            \
                out_dword(HWIO_LPASS_QCA_SLIMBUS_N_ADDR(x),v)
#define HWIO_LPASS_QCA_SLIMBUS_N_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_QCA_SLIMBUS_N_ADDR(x),m,v,HWIO_LPASS_QCA_SLIMBUS_N_IN(x))
#define HWIO_LPASS_QCA_SLIMBUS_N_NOT_N_MINUS_M_BMSK                                                                         0xff
#define HWIO_LPASS_QCA_SLIMBUS_N_NOT_N_MINUS_M_SHFT                                                                            0

#define HWIO_LPASS_QCA_SLIMBUS_D_ADDR(x)                                                                              ((x) + 0x18010)
#define HWIO_LPASS_QCA_SLIMBUS_D_RMSK                                                                                       0xff
#define HWIO_LPASS_QCA_SLIMBUS_D_IN(x)            \
                in_dword(HWIO_LPASS_QCA_SLIMBUS_D_ADDR(x))
#define HWIO_LPASS_QCA_SLIMBUS_D_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_QCA_SLIMBUS_D_ADDR(x), m)
#define HWIO_LPASS_QCA_SLIMBUS_D_OUT(x, v)            \
                out_dword(HWIO_LPASS_QCA_SLIMBUS_D_ADDR(x),v)
#define HWIO_LPASS_QCA_SLIMBUS_D_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_QCA_SLIMBUS_D_ADDR(x),m,v,HWIO_LPASS_QCA_SLIMBUS_D_IN(x))
#define HWIO_LPASS_QCA_SLIMBUS_D_NOT_2D_BMSK                                                                                0xff
#define HWIO_LPASS_QCA_SLIMBUS_D_NOT_2D_SHFT                                                                                   0

#define HWIO_LPASS_AUDIO_CORE_QCA_SLIMBUS_CBCR_ADDR(x)                                                                ((x) + 0x18014)
#define HWIO_LPASS_AUDIO_CORE_QCA_SLIMBUS_CBCR_RMSK                                                                   0x80000003
#define HWIO_LPASS_AUDIO_CORE_QCA_SLIMBUS_CBCR_IN(x)            \
                in_dword(HWIO_LPASS_AUDIO_CORE_QCA_SLIMBUS_CBCR_ADDR(x))
#define HWIO_LPASS_AUDIO_CORE_QCA_SLIMBUS_CBCR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AUDIO_CORE_QCA_SLIMBUS_CBCR_ADDR(x), m)
#define HWIO_LPASS_AUDIO_CORE_QCA_SLIMBUS_CBCR_OUT(x, v)            \
                out_dword(HWIO_LPASS_AUDIO_CORE_QCA_SLIMBUS_CBCR_ADDR(x),v)
#define HWIO_LPASS_AUDIO_CORE_QCA_SLIMBUS_CBCR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AUDIO_CORE_QCA_SLIMBUS_CBCR_ADDR(x),m,v,HWIO_LPASS_AUDIO_CORE_QCA_SLIMBUS_CBCR_IN(x))
#define HWIO_LPASS_AUDIO_CORE_QCA_SLIMBUS_CBCR_CLK_OFF_BMSK                                                           0x80000000
#define HWIO_LPASS_AUDIO_CORE_QCA_SLIMBUS_CBCR_CLK_OFF_SHFT                                                                   31
#define HWIO_LPASS_AUDIO_CORE_QCA_SLIMBUS_CBCR_HW_CTL_BMSK                                                                   0x2
#define HWIO_LPASS_AUDIO_CORE_QCA_SLIMBUS_CBCR_HW_CTL_SHFT                                                                     1
#define HWIO_LPASS_AUDIO_CORE_QCA_SLIMBUS_CBCR_CLK_ENABLE_BMSK                                                               0x1
#define HWIO_LPASS_AUDIO_CORE_QCA_SLIMBUS_CBCR_CLK_ENABLE_SHFT                                                                 0

#define HWIO_LPASS_AUDIO_CORE_QCA_SLIMBUS_CORE_CBCR_ADDR(x)                                                           ((x) + 0x18018)
#define HWIO_LPASS_AUDIO_CORE_QCA_SLIMBUS_CORE_CBCR_RMSK                                                              0x80007ff3
#define HWIO_LPASS_AUDIO_CORE_QCA_SLIMBUS_CORE_CBCR_IN(x)            \
                in_dword(HWIO_LPASS_AUDIO_CORE_QCA_SLIMBUS_CORE_CBCR_ADDR(x))
#define HWIO_LPASS_AUDIO_CORE_QCA_SLIMBUS_CORE_CBCR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AUDIO_CORE_QCA_SLIMBUS_CORE_CBCR_ADDR(x), m)
#define HWIO_LPASS_AUDIO_CORE_QCA_SLIMBUS_CORE_CBCR_OUT(x, v)            \
                out_dword(HWIO_LPASS_AUDIO_CORE_QCA_SLIMBUS_CORE_CBCR_ADDR(x),v)
#define HWIO_LPASS_AUDIO_CORE_QCA_SLIMBUS_CORE_CBCR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AUDIO_CORE_QCA_SLIMBUS_CORE_CBCR_ADDR(x),m,v,HWIO_LPASS_AUDIO_CORE_QCA_SLIMBUS_CORE_CBCR_IN(x))
#define HWIO_LPASS_AUDIO_CORE_QCA_SLIMBUS_CORE_CBCR_CLK_OFF_BMSK                                                      0x80000000
#define HWIO_LPASS_AUDIO_CORE_QCA_SLIMBUS_CORE_CBCR_CLK_OFF_SHFT                                                              31
#define HWIO_LPASS_AUDIO_CORE_QCA_SLIMBUS_CORE_CBCR_FORCE_MEM_CORE_ON_BMSK                                                0x4000
#define HWIO_LPASS_AUDIO_CORE_QCA_SLIMBUS_CORE_CBCR_FORCE_MEM_CORE_ON_SHFT                                                    14
#define HWIO_LPASS_AUDIO_CORE_QCA_SLIMBUS_CORE_CBCR_FORCE_MEM_PERIPH_ON_BMSK                                              0x2000
#define HWIO_LPASS_AUDIO_CORE_QCA_SLIMBUS_CORE_CBCR_FORCE_MEM_PERIPH_ON_SHFT                                                  13
#define HWIO_LPASS_AUDIO_CORE_QCA_SLIMBUS_CORE_CBCR_FORCE_MEM_PERIPH_OFF_BMSK                                             0x1000
#define HWIO_LPASS_AUDIO_CORE_QCA_SLIMBUS_CORE_CBCR_FORCE_MEM_PERIPH_OFF_SHFT                                                 12
#define HWIO_LPASS_AUDIO_CORE_QCA_SLIMBUS_CORE_CBCR_WAKEUP_BMSK                                                            0xf00
#define HWIO_LPASS_AUDIO_CORE_QCA_SLIMBUS_CORE_CBCR_WAKEUP_SHFT                                                                8
#define HWIO_LPASS_AUDIO_CORE_QCA_SLIMBUS_CORE_CBCR_SLEEP_BMSK                                                              0xf0
#define HWIO_LPASS_AUDIO_CORE_QCA_SLIMBUS_CORE_CBCR_SLEEP_SHFT                                                                 4
#define HWIO_LPASS_AUDIO_CORE_QCA_SLIMBUS_CORE_CBCR_HW_CTL_BMSK                                                              0x2
#define HWIO_LPASS_AUDIO_CORE_QCA_SLIMBUS_CORE_CBCR_HW_CTL_SHFT                                                                1
#define HWIO_LPASS_AUDIO_CORE_QCA_SLIMBUS_CORE_CBCR_CLK_ENABLE_BMSK                                                          0x1
#define HWIO_LPASS_AUDIO_CORE_QCA_SLIMBUS_CORE_CBCR_CLK_ENABLE_SHFT                                                            0

#define HWIO_LPASS_LPAIF_PCMOE_CMD_RCGR_ADDR(x)                                                                       ((x) + 0x19000)
#define HWIO_LPASS_LPAIF_PCMOE_CMD_RCGR_RMSK                                                                          0x800000f3
#define HWIO_LPASS_LPAIF_PCMOE_CMD_RCGR_IN(x)            \
                in_dword(HWIO_LPASS_LPAIF_PCMOE_CMD_RCGR_ADDR(x))
#define HWIO_LPASS_LPAIF_PCMOE_CMD_RCGR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPAIF_PCMOE_CMD_RCGR_ADDR(x), m)
#define HWIO_LPASS_LPAIF_PCMOE_CMD_RCGR_OUT(x, v)            \
                out_dword(HWIO_LPASS_LPAIF_PCMOE_CMD_RCGR_ADDR(x),v)
#define HWIO_LPASS_LPAIF_PCMOE_CMD_RCGR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_LPAIF_PCMOE_CMD_RCGR_ADDR(x),m,v,HWIO_LPASS_LPAIF_PCMOE_CMD_RCGR_IN(x))
#define HWIO_LPASS_LPAIF_PCMOE_CMD_RCGR_ROOT_OFF_BMSK                                                                 0x80000000
#define HWIO_LPASS_LPAIF_PCMOE_CMD_RCGR_ROOT_OFF_SHFT                                                                         31
#define HWIO_LPASS_LPAIF_PCMOE_CMD_RCGR_DIRTY_D_BMSK                                                                        0x80
#define HWIO_LPASS_LPAIF_PCMOE_CMD_RCGR_DIRTY_D_SHFT                                                                           7
#define HWIO_LPASS_LPAIF_PCMOE_CMD_RCGR_DIRTY_N_BMSK                                                                        0x40
#define HWIO_LPASS_LPAIF_PCMOE_CMD_RCGR_DIRTY_N_SHFT                                                                           6
#define HWIO_LPASS_LPAIF_PCMOE_CMD_RCGR_DIRTY_M_BMSK                                                                        0x20
#define HWIO_LPASS_LPAIF_PCMOE_CMD_RCGR_DIRTY_M_SHFT                                                                           5
#define HWIO_LPASS_LPAIF_PCMOE_CMD_RCGR_DIRTY_CFG_RCGR_BMSK                                                                 0x10
#define HWIO_LPASS_LPAIF_PCMOE_CMD_RCGR_DIRTY_CFG_RCGR_SHFT                                                                    4
#define HWIO_LPASS_LPAIF_PCMOE_CMD_RCGR_ROOT_EN_BMSK                                                                         0x2
#define HWIO_LPASS_LPAIF_PCMOE_CMD_RCGR_ROOT_EN_SHFT                                                                           1
#define HWIO_LPASS_LPAIF_PCMOE_CMD_RCGR_UPDATE_BMSK                                                                          0x1
#define HWIO_LPASS_LPAIF_PCMOE_CMD_RCGR_UPDATE_SHFT                                                                            0

#define HWIO_LPASS_LPAIF_PCMOE_CFG_RCGR_ADDR(x)                                                                       ((x) + 0x19004)
#define HWIO_LPASS_LPAIF_PCMOE_CFG_RCGR_RMSK                                                                              0x771f
#define HWIO_LPASS_LPAIF_PCMOE_CFG_RCGR_IN(x)            \
                in_dword(HWIO_LPASS_LPAIF_PCMOE_CFG_RCGR_ADDR(x))
#define HWIO_LPASS_LPAIF_PCMOE_CFG_RCGR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPAIF_PCMOE_CFG_RCGR_ADDR(x), m)
#define HWIO_LPASS_LPAIF_PCMOE_CFG_RCGR_OUT(x, v)            \
                out_dword(HWIO_LPASS_LPAIF_PCMOE_CFG_RCGR_ADDR(x),v)
#define HWIO_LPASS_LPAIF_PCMOE_CFG_RCGR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_LPAIF_PCMOE_CFG_RCGR_ADDR(x),m,v,HWIO_LPASS_LPAIF_PCMOE_CFG_RCGR_IN(x))
#define HWIO_LPASS_LPAIF_PCMOE_CFG_RCGR_ALT_SRC_SEL_BMSK                                                                  0x4000
#define HWIO_LPASS_LPAIF_PCMOE_CFG_RCGR_ALT_SRC_SEL_SHFT                                                                      14
#define HWIO_LPASS_LPAIF_PCMOE_CFG_RCGR_MODE_BMSK                                                                         0x3000
#define HWIO_LPASS_LPAIF_PCMOE_CFG_RCGR_MODE_SHFT                                                                             12
#define HWIO_LPASS_LPAIF_PCMOE_CFG_RCGR_SRC_SEL_BMSK                                                                       0x700
#define HWIO_LPASS_LPAIF_PCMOE_CFG_RCGR_SRC_SEL_SHFT                                                                           8
#define HWIO_LPASS_LPAIF_PCMOE_CFG_RCGR_SRC_DIV_BMSK                                                                        0x1f
#define HWIO_LPASS_LPAIF_PCMOE_CFG_RCGR_SRC_DIV_SHFT                                                                           0

#define HWIO_LPASS_LPAIF_PCMOE_M_ADDR(x)                                                                              ((x) + 0x19008)
#define HWIO_LPASS_LPAIF_PCMOE_M_RMSK                                                                                       0xff
#define HWIO_LPASS_LPAIF_PCMOE_M_IN(x)            \
                in_dword(HWIO_LPASS_LPAIF_PCMOE_M_ADDR(x))
#define HWIO_LPASS_LPAIF_PCMOE_M_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPAIF_PCMOE_M_ADDR(x), m)
#define HWIO_LPASS_LPAIF_PCMOE_M_OUT(x, v)            \
                out_dword(HWIO_LPASS_LPAIF_PCMOE_M_ADDR(x),v)
#define HWIO_LPASS_LPAIF_PCMOE_M_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_LPAIF_PCMOE_M_ADDR(x),m,v,HWIO_LPASS_LPAIF_PCMOE_M_IN(x))
#define HWIO_LPASS_LPAIF_PCMOE_M_M_BMSK                                                                                     0xff
#define HWIO_LPASS_LPAIF_PCMOE_M_M_SHFT                                                                                        0

#define HWIO_LPASS_LPAIF_PCMOE_N_ADDR(x)                                                                              ((x) + 0x1900c)
#define HWIO_LPASS_LPAIF_PCMOE_N_RMSK                                                                                       0xff
#define HWIO_LPASS_LPAIF_PCMOE_N_IN(x)            \
                in_dword(HWIO_LPASS_LPAIF_PCMOE_N_ADDR(x))
#define HWIO_LPASS_LPAIF_PCMOE_N_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPAIF_PCMOE_N_ADDR(x), m)
#define HWIO_LPASS_LPAIF_PCMOE_N_OUT(x, v)            \
                out_dword(HWIO_LPASS_LPAIF_PCMOE_N_ADDR(x),v)
#define HWIO_LPASS_LPAIF_PCMOE_N_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_LPAIF_PCMOE_N_ADDR(x),m,v,HWIO_LPASS_LPAIF_PCMOE_N_IN(x))
#define HWIO_LPASS_LPAIF_PCMOE_N_NOT_N_MINUS_M_BMSK                                                                         0xff
#define HWIO_LPASS_LPAIF_PCMOE_N_NOT_N_MINUS_M_SHFT                                                                            0

#define HWIO_LPASS_LPAIF_PCMOE_D_ADDR(x)                                                                              ((x) + 0x19010)
#define HWIO_LPASS_LPAIF_PCMOE_D_RMSK                                                                                       0xff
#define HWIO_LPASS_LPAIF_PCMOE_D_IN(x)            \
                in_dword(HWIO_LPASS_LPAIF_PCMOE_D_ADDR(x))
#define HWIO_LPASS_LPAIF_PCMOE_D_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPAIF_PCMOE_D_ADDR(x), m)
#define HWIO_LPASS_LPAIF_PCMOE_D_OUT(x, v)            \
                out_dword(HWIO_LPASS_LPAIF_PCMOE_D_ADDR(x),v)
#define HWIO_LPASS_LPAIF_PCMOE_D_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_LPAIF_PCMOE_D_ADDR(x),m,v,HWIO_LPASS_LPAIF_PCMOE_D_IN(x))
#define HWIO_LPASS_LPAIF_PCMOE_D_NOT_2D_BMSK                                                                                0xff
#define HWIO_LPASS_LPAIF_PCMOE_D_NOT_2D_SHFT                                                                                   0

#define HWIO_LPASS_AUDIO_CORE_LPAIF_PCM_DATA_OE_CBCR_ADDR(x)                                                          ((x) + 0x19014)
#define HWIO_LPASS_AUDIO_CORE_LPAIF_PCM_DATA_OE_CBCR_RMSK                                                             0x80000003
#define HWIO_LPASS_AUDIO_CORE_LPAIF_PCM_DATA_OE_CBCR_IN(x)            \
                in_dword(HWIO_LPASS_AUDIO_CORE_LPAIF_PCM_DATA_OE_CBCR_ADDR(x))
#define HWIO_LPASS_AUDIO_CORE_LPAIF_PCM_DATA_OE_CBCR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AUDIO_CORE_LPAIF_PCM_DATA_OE_CBCR_ADDR(x), m)
#define HWIO_LPASS_AUDIO_CORE_LPAIF_PCM_DATA_OE_CBCR_OUT(x, v)            \
                out_dword(HWIO_LPASS_AUDIO_CORE_LPAIF_PCM_DATA_OE_CBCR_ADDR(x),v)
#define HWIO_LPASS_AUDIO_CORE_LPAIF_PCM_DATA_OE_CBCR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AUDIO_CORE_LPAIF_PCM_DATA_OE_CBCR_ADDR(x),m,v,HWIO_LPASS_AUDIO_CORE_LPAIF_PCM_DATA_OE_CBCR_IN(x))
#define HWIO_LPASS_AUDIO_CORE_LPAIF_PCM_DATA_OE_CBCR_CLK_OFF_BMSK                                                     0x80000000
#define HWIO_LPASS_AUDIO_CORE_LPAIF_PCM_DATA_OE_CBCR_CLK_OFF_SHFT                                                             31
#define HWIO_LPASS_AUDIO_CORE_LPAIF_PCM_DATA_OE_CBCR_HW_CTL_BMSK                                                             0x2
#define HWIO_LPASS_AUDIO_CORE_LPAIF_PCM_DATA_OE_CBCR_HW_CTL_SHFT                                                               1
#define HWIO_LPASS_AUDIO_CORE_LPAIF_PCM_DATA_OE_CBCR_CLK_ENABLE_BMSK                                                         0x1
#define HWIO_LPASS_AUDIO_CORE_LPAIF_PCM_DATA_OE_CBCR_CLK_ENABLE_SHFT                                                           0

#define HWIO_LPASS_SLEEP_CMD_RCGR_ADDR(x)                                                                             ((x) + 0x1a000)
#define HWIO_LPASS_SLEEP_CMD_RCGR_RMSK                                                                                0x800000f3
#define HWIO_LPASS_SLEEP_CMD_RCGR_IN(x)            \
                in_dword(HWIO_LPASS_SLEEP_CMD_RCGR_ADDR(x))
#define HWIO_LPASS_SLEEP_CMD_RCGR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_SLEEP_CMD_RCGR_ADDR(x), m)
#define HWIO_LPASS_SLEEP_CMD_RCGR_OUT(x, v)            \
                out_dword(HWIO_LPASS_SLEEP_CMD_RCGR_ADDR(x),v)
#define HWIO_LPASS_SLEEP_CMD_RCGR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_SLEEP_CMD_RCGR_ADDR(x),m,v,HWIO_LPASS_SLEEP_CMD_RCGR_IN(x))
#define HWIO_LPASS_SLEEP_CMD_RCGR_ROOT_OFF_BMSK                                                                       0x80000000
#define HWIO_LPASS_SLEEP_CMD_RCGR_ROOT_OFF_SHFT                                                                               31
#define HWIO_LPASS_SLEEP_CMD_RCGR_DIRTY_D_BMSK                                                                              0x80
#define HWIO_LPASS_SLEEP_CMD_RCGR_DIRTY_D_SHFT                                                                                 7
#define HWIO_LPASS_SLEEP_CMD_RCGR_DIRTY_N_BMSK                                                                              0x40
#define HWIO_LPASS_SLEEP_CMD_RCGR_DIRTY_N_SHFT                                                                                 6
#define HWIO_LPASS_SLEEP_CMD_RCGR_DIRTY_M_BMSK                                                                              0x20
#define HWIO_LPASS_SLEEP_CMD_RCGR_DIRTY_M_SHFT                                                                                 5
#define HWIO_LPASS_SLEEP_CMD_RCGR_DIRTY_CFG_RCGR_BMSK                                                                       0x10
#define HWIO_LPASS_SLEEP_CMD_RCGR_DIRTY_CFG_RCGR_SHFT                                                                          4
#define HWIO_LPASS_SLEEP_CMD_RCGR_ROOT_EN_BMSK                                                                               0x2
#define HWIO_LPASS_SLEEP_CMD_RCGR_ROOT_EN_SHFT                                                                                 1
#define HWIO_LPASS_SLEEP_CMD_RCGR_UPDATE_BMSK                                                                                0x1
#define HWIO_LPASS_SLEEP_CMD_RCGR_UPDATE_SHFT                                                                                  0

#define HWIO_LPASS_SLEEP_CFG_RCGR_ADDR(x)                                                                             ((x) + 0x1a004)
#define HWIO_LPASS_SLEEP_CFG_RCGR_RMSK                                                                                   0x1371f
#define HWIO_LPASS_SLEEP_CFG_RCGR_IN(x)            \
                in_dword(HWIO_LPASS_SLEEP_CFG_RCGR_ADDR(x))
#define HWIO_LPASS_SLEEP_CFG_RCGR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_SLEEP_CFG_RCGR_ADDR(x), m)
#define HWIO_LPASS_SLEEP_CFG_RCGR_OUT(x, v)            \
                out_dword(HWIO_LPASS_SLEEP_CFG_RCGR_ADDR(x),v)
#define HWIO_LPASS_SLEEP_CFG_RCGR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_SLEEP_CFG_RCGR_ADDR(x),m,v,HWIO_LPASS_SLEEP_CFG_RCGR_IN(x))
#define HWIO_LPASS_SLEEP_CFG_RCGR_RCGLITE_DISABLE_BMSK                                                                   0x10000
#define HWIO_LPASS_SLEEP_CFG_RCGR_RCGLITE_DISABLE_SHFT                                                                        16
#define HWIO_LPASS_SLEEP_CFG_RCGR_MODE_BMSK                                                                               0x3000
#define HWIO_LPASS_SLEEP_CFG_RCGR_MODE_SHFT                                                                                   12
#define HWIO_LPASS_SLEEP_CFG_RCGR_SRC_SEL_BMSK                                                                             0x700
#define HWIO_LPASS_SLEEP_CFG_RCGR_SRC_SEL_SHFT                                                                                 8
#define HWIO_LPASS_SLEEP_CFG_RCGR_SRC_DIV_BMSK                                                                              0x1f
#define HWIO_LPASS_SLEEP_CFG_RCGR_SRC_DIV_SHFT                                                                                 0

#define HWIO_LPASS_XO_CMD_RCGR_ADDR(x)                                                                                ((x) + 0x1b000)
#define HWIO_LPASS_XO_CMD_RCGR_RMSK                                                                                   0x800000f3
#define HWIO_LPASS_XO_CMD_RCGR_IN(x)            \
                in_dword(HWIO_LPASS_XO_CMD_RCGR_ADDR(x))
#define HWIO_LPASS_XO_CMD_RCGR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_XO_CMD_RCGR_ADDR(x), m)
#define HWIO_LPASS_XO_CMD_RCGR_OUT(x, v)            \
                out_dword(HWIO_LPASS_XO_CMD_RCGR_ADDR(x),v)
#define HWIO_LPASS_XO_CMD_RCGR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_XO_CMD_RCGR_ADDR(x),m,v,HWIO_LPASS_XO_CMD_RCGR_IN(x))
#define HWIO_LPASS_XO_CMD_RCGR_ROOT_OFF_BMSK                                                                          0x80000000
#define HWIO_LPASS_XO_CMD_RCGR_ROOT_OFF_SHFT                                                                                  31
#define HWIO_LPASS_XO_CMD_RCGR_DIRTY_D_BMSK                                                                                 0x80
#define HWIO_LPASS_XO_CMD_RCGR_DIRTY_D_SHFT                                                                                    7
#define HWIO_LPASS_XO_CMD_RCGR_DIRTY_N_BMSK                                                                                 0x40
#define HWIO_LPASS_XO_CMD_RCGR_DIRTY_N_SHFT                                                                                    6
#define HWIO_LPASS_XO_CMD_RCGR_DIRTY_M_BMSK                                                                                 0x20
#define HWIO_LPASS_XO_CMD_RCGR_DIRTY_M_SHFT                                                                                    5
#define HWIO_LPASS_XO_CMD_RCGR_DIRTY_CFG_RCGR_BMSK                                                                          0x10
#define HWIO_LPASS_XO_CMD_RCGR_DIRTY_CFG_RCGR_SHFT                                                                             4
#define HWIO_LPASS_XO_CMD_RCGR_ROOT_EN_BMSK                                                                                  0x2
#define HWIO_LPASS_XO_CMD_RCGR_ROOT_EN_SHFT                                                                                    1
#define HWIO_LPASS_XO_CMD_RCGR_UPDATE_BMSK                                                                                   0x1
#define HWIO_LPASS_XO_CMD_RCGR_UPDATE_SHFT                                                                                     0

#define HWIO_LPASS_XO_CFG_RCGR_ADDR(x)                                                                                ((x) + 0x1b004)
#define HWIO_LPASS_XO_CFG_RCGR_RMSK                                                                                      0x1771f
#define HWIO_LPASS_XO_CFG_RCGR_IN(x)            \
                in_dword(HWIO_LPASS_XO_CFG_RCGR_ADDR(x))
#define HWIO_LPASS_XO_CFG_RCGR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_XO_CFG_RCGR_ADDR(x), m)
#define HWIO_LPASS_XO_CFG_RCGR_OUT(x, v)            \
                out_dword(HWIO_LPASS_XO_CFG_RCGR_ADDR(x),v)
#define HWIO_LPASS_XO_CFG_RCGR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_XO_CFG_RCGR_ADDR(x),m,v,HWIO_LPASS_XO_CFG_RCGR_IN(x))
#define HWIO_LPASS_XO_CFG_RCGR_RCGLITE_DISABLE_BMSK                                                                      0x10000
#define HWIO_LPASS_XO_CFG_RCGR_RCGLITE_DISABLE_SHFT                                                                           16
#define HWIO_LPASS_XO_CFG_RCGR_ALT_SRC_SEL_BMSK                                                                           0x4000
#define HWIO_LPASS_XO_CFG_RCGR_ALT_SRC_SEL_SHFT                                                                               14
#define HWIO_LPASS_XO_CFG_RCGR_MODE_BMSK                                                                                  0x3000
#define HWIO_LPASS_XO_CFG_RCGR_MODE_SHFT                                                                                      12
#define HWIO_LPASS_XO_CFG_RCGR_SRC_SEL_BMSK                                                                                0x700
#define HWIO_LPASS_XO_CFG_RCGR_SRC_SEL_SHFT                                                                                    8
#define HWIO_LPASS_XO_CFG_RCGR_SRC_DIV_BMSK                                                                                 0x1f
#define HWIO_LPASS_XO_CFG_RCGR_SRC_DIV_SHFT                                                                                    0

#define HWIO_LPASS_AUDIO_CORE_AVSYNC_STC_CBCR_ADDR(x)                                                                 ((x) + 0x1c000)
#define HWIO_LPASS_AUDIO_CORE_AVSYNC_STC_CBCR_RMSK                                                                    0x80000003
#define HWIO_LPASS_AUDIO_CORE_AVSYNC_STC_CBCR_IN(x)            \
                in_dword(HWIO_LPASS_AUDIO_CORE_AVSYNC_STC_CBCR_ADDR(x))
#define HWIO_LPASS_AUDIO_CORE_AVSYNC_STC_CBCR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AUDIO_CORE_AVSYNC_STC_CBCR_ADDR(x), m)
#define HWIO_LPASS_AUDIO_CORE_AVSYNC_STC_CBCR_OUT(x, v)            \
                out_dword(HWIO_LPASS_AUDIO_CORE_AVSYNC_STC_CBCR_ADDR(x),v)
#define HWIO_LPASS_AUDIO_CORE_AVSYNC_STC_CBCR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AUDIO_CORE_AVSYNC_STC_CBCR_ADDR(x),m,v,HWIO_LPASS_AUDIO_CORE_AVSYNC_STC_CBCR_IN(x))
#define HWIO_LPASS_AUDIO_CORE_AVSYNC_STC_CBCR_CLK_OFF_BMSK                                                            0x80000000
#define HWIO_LPASS_AUDIO_CORE_AVSYNC_STC_CBCR_CLK_OFF_SHFT                                                                    31
#define HWIO_LPASS_AUDIO_CORE_AVSYNC_STC_CBCR_HW_CTL_BMSK                                                                    0x2
#define HWIO_LPASS_AUDIO_CORE_AVSYNC_STC_CBCR_HW_CTL_SHFT                                                                      1
#define HWIO_LPASS_AUDIO_CORE_AVSYNC_STC_CBCR_CLK_ENABLE_BMSK                                                                0x1
#define HWIO_LPASS_AUDIO_CORE_AVSYNC_STC_CBCR_CLK_ENABLE_SHFT                                                                  0

#define HWIO_LPASS_CORE_CMD_RCGR_ADDR(x)                                                                              ((x) + 0x1d000)
#define HWIO_LPASS_CORE_CMD_RCGR_RMSK                                                                                 0x800000f3
#define HWIO_LPASS_CORE_CMD_RCGR_IN(x)            \
                in_dword(HWIO_LPASS_CORE_CMD_RCGR_ADDR(x))
#define HWIO_LPASS_CORE_CMD_RCGR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_CORE_CMD_RCGR_ADDR(x), m)
#define HWIO_LPASS_CORE_CMD_RCGR_OUT(x, v)            \
                out_dword(HWIO_LPASS_CORE_CMD_RCGR_ADDR(x),v)
#define HWIO_LPASS_CORE_CMD_RCGR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_CORE_CMD_RCGR_ADDR(x),m,v,HWIO_LPASS_CORE_CMD_RCGR_IN(x))
#define HWIO_LPASS_CORE_CMD_RCGR_ROOT_OFF_BMSK                                                                        0x80000000
#define HWIO_LPASS_CORE_CMD_RCGR_ROOT_OFF_SHFT                                                                                31
#define HWIO_LPASS_CORE_CMD_RCGR_DIRTY_D_BMSK                                                                               0x80
#define HWIO_LPASS_CORE_CMD_RCGR_DIRTY_D_SHFT                                                                                  7
#define HWIO_LPASS_CORE_CMD_RCGR_DIRTY_N_BMSK                                                                               0x40
#define HWIO_LPASS_CORE_CMD_RCGR_DIRTY_N_SHFT                                                                                  6
#define HWIO_LPASS_CORE_CMD_RCGR_DIRTY_M_BMSK                                                                               0x20
#define HWIO_LPASS_CORE_CMD_RCGR_DIRTY_M_SHFT                                                                                  5
#define HWIO_LPASS_CORE_CMD_RCGR_DIRTY_CFG_RCGR_BMSK                                                                        0x10
#define HWIO_LPASS_CORE_CMD_RCGR_DIRTY_CFG_RCGR_SHFT                                                                           4
#define HWIO_LPASS_CORE_CMD_RCGR_ROOT_EN_BMSK                                                                                0x2
#define HWIO_LPASS_CORE_CMD_RCGR_ROOT_EN_SHFT                                                                                  1
#define HWIO_LPASS_CORE_CMD_RCGR_UPDATE_BMSK                                                                                 0x1
#define HWIO_LPASS_CORE_CMD_RCGR_UPDATE_SHFT                                                                                   0

#define HWIO_LPASS_CORE_CFG_RCGR_ADDR(x)                                                                              ((x) + 0x1d004)
#define HWIO_LPASS_CORE_CFG_RCGR_RMSK                                                                                     0x771f
#define HWIO_LPASS_CORE_CFG_RCGR_IN(x)            \
                in_dword(HWIO_LPASS_CORE_CFG_RCGR_ADDR(x))
#define HWIO_LPASS_CORE_CFG_RCGR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_CORE_CFG_RCGR_ADDR(x), m)
#define HWIO_LPASS_CORE_CFG_RCGR_OUT(x, v)            \
                out_dword(HWIO_LPASS_CORE_CFG_RCGR_ADDR(x),v)
#define HWIO_LPASS_CORE_CFG_RCGR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_CORE_CFG_RCGR_ADDR(x),m,v,HWIO_LPASS_CORE_CFG_RCGR_IN(x))
#define HWIO_LPASS_CORE_CFG_RCGR_ALT_SRC_SEL_BMSK                                                                         0x4000
#define HWIO_LPASS_CORE_CFG_RCGR_ALT_SRC_SEL_SHFT                                                                             14
#define HWIO_LPASS_CORE_CFG_RCGR_MODE_BMSK                                                                                0x3000
#define HWIO_LPASS_CORE_CFG_RCGR_MODE_SHFT                                                                                    12
#define HWIO_LPASS_CORE_CFG_RCGR_SRC_SEL_BMSK                                                                              0x700
#define HWIO_LPASS_CORE_CFG_RCGR_SRC_SEL_SHFT                                                                                  8
#define HWIO_LPASS_CORE_CFG_RCGR_SRC_DIV_BMSK                                                                               0x1f
#define HWIO_LPASS_CORE_CFG_RCGR_SRC_DIV_SHFT                                                                                  0

#define HWIO_LPASS_CORE_M_ADDR(x)                                                                                     ((x) + 0x1d008)
#define HWIO_LPASS_CORE_M_RMSK                                                                                              0xff
#define HWIO_LPASS_CORE_M_IN(x)            \
                in_dword(HWIO_LPASS_CORE_M_ADDR(x))
#define HWIO_LPASS_CORE_M_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_CORE_M_ADDR(x), m)
#define HWIO_LPASS_CORE_M_OUT(x, v)            \
                out_dword(HWIO_LPASS_CORE_M_ADDR(x),v)
#define HWIO_LPASS_CORE_M_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_CORE_M_ADDR(x),m,v,HWIO_LPASS_CORE_M_IN(x))
#define HWIO_LPASS_CORE_M_M_BMSK                                                                                            0xff
#define HWIO_LPASS_CORE_M_M_SHFT                                                                                               0

#define HWIO_LPASS_CORE_N_ADDR(x)                                                                                     ((x) + 0x1d00c)
#define HWIO_LPASS_CORE_N_RMSK                                                                                              0xff
#define HWIO_LPASS_CORE_N_IN(x)            \
                in_dword(HWIO_LPASS_CORE_N_ADDR(x))
#define HWIO_LPASS_CORE_N_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_CORE_N_ADDR(x), m)
#define HWIO_LPASS_CORE_N_OUT(x, v)            \
                out_dword(HWIO_LPASS_CORE_N_ADDR(x),v)
#define HWIO_LPASS_CORE_N_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_CORE_N_ADDR(x),m,v,HWIO_LPASS_CORE_N_IN(x))
#define HWIO_LPASS_CORE_N_NOT_N_MINUS_M_BMSK                                                                                0xff
#define HWIO_LPASS_CORE_N_NOT_N_MINUS_M_SHFT                                                                                   0

#define HWIO_LPASS_CORE_D_ADDR(x)                                                                                     ((x) + 0x1d010)
#define HWIO_LPASS_CORE_D_RMSK                                                                                              0xff
#define HWIO_LPASS_CORE_D_IN(x)            \
                in_dword(HWIO_LPASS_CORE_D_ADDR(x))
#define HWIO_LPASS_CORE_D_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_CORE_D_ADDR(x), m)
#define HWIO_LPASS_CORE_D_OUT(x, v)            \
                out_dword(HWIO_LPASS_CORE_D_ADDR(x),v)
#define HWIO_LPASS_CORE_D_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_CORE_D_ADDR(x),m,v,HWIO_LPASS_CORE_D_IN(x))
#define HWIO_LPASS_CORE_D_NOT_2D_BMSK                                                                                       0xff
#define HWIO_LPASS_CORE_D_NOT_2D_SHFT                                                                                          0

#define HWIO_LPASS_CORE_CMD_DFSR_ADDR(x)                                                                              ((x) + 0x1d014)
#define HWIO_LPASS_CORE_CMD_DFSR_RMSK                                                                                     0xbfef
#define HWIO_LPASS_CORE_CMD_DFSR_IN(x)            \
                in_dword(HWIO_LPASS_CORE_CMD_DFSR_ADDR(x))
#define HWIO_LPASS_CORE_CMD_DFSR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_CORE_CMD_DFSR_ADDR(x), m)
#define HWIO_LPASS_CORE_CMD_DFSR_OUT(x, v)            \
                out_dword(HWIO_LPASS_CORE_CMD_DFSR_ADDR(x),v)
#define HWIO_LPASS_CORE_CMD_DFSR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_CORE_CMD_DFSR_ADDR(x),m,v,HWIO_LPASS_CORE_CMD_DFSR_IN(x))
#define HWIO_LPASS_CORE_CMD_DFSR_RCG_SW_CTRL_BMSK                                                                         0x8000
#define HWIO_LPASS_CORE_CMD_DFSR_RCG_SW_CTRL_SHFT                                                                             15
#define HWIO_LPASS_CORE_CMD_DFSR_SW_PERF_STATE_BMSK                                                                       0x3800
#define HWIO_LPASS_CORE_CMD_DFSR_SW_PERF_STATE_SHFT                                                                           11
#define HWIO_LPASS_CORE_CMD_DFSR_SW_OVERRIDE_BMSK                                                                          0x400
#define HWIO_LPASS_CORE_CMD_DFSR_SW_OVERRIDE_SHFT                                                                             10
#define HWIO_LPASS_CORE_CMD_DFSR_PERF_STATE_UPDATE_STATUS_BMSK                                                             0x200
#define HWIO_LPASS_CORE_CMD_DFSR_PERF_STATE_UPDATE_STATUS_SHFT                                                                 9
#define HWIO_LPASS_CORE_CMD_DFSR_DFS_FSM_STATE_BMSK                                                                        0x1c0
#define HWIO_LPASS_CORE_CMD_DFSR_DFS_FSM_STATE_SHFT                                                                            6
#define HWIO_LPASS_CORE_CMD_DFSR_HW_CLK_CONTROL_BMSK                                                                        0x20
#define HWIO_LPASS_CORE_CMD_DFSR_HW_CLK_CONTROL_SHFT                                                                           5
#define HWIO_LPASS_CORE_CMD_DFSR_CURR_PERF_STATE_BMSK                                                                        0xe
#define HWIO_LPASS_CORE_CMD_DFSR_CURR_PERF_STATE_SHFT                                                                          1
#define HWIO_LPASS_CORE_CMD_DFSR_DFS_EN_BMSK                                                                                 0x1
#define HWIO_LPASS_CORE_CMD_DFSR_DFS_EN_SHFT                                                                                   0

#define HWIO_LPASS_CORE_PERF0_DFSR_ADDR(x)                                                                            ((x) + 0x1d018)
#define HWIO_LPASS_CORE_PERF0_DFSR_RMSK                                                                                   0x371f
#define HWIO_LPASS_CORE_PERF0_DFSR_IN(x)            \
                in_dword(HWIO_LPASS_CORE_PERF0_DFSR_ADDR(x))
#define HWIO_LPASS_CORE_PERF0_DFSR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_CORE_PERF0_DFSR_ADDR(x), m)
#define HWIO_LPASS_CORE_PERF0_DFSR_OUT(x, v)            \
                out_dword(HWIO_LPASS_CORE_PERF0_DFSR_ADDR(x),v)
#define HWIO_LPASS_CORE_PERF0_DFSR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_CORE_PERF0_DFSR_ADDR(x),m,v,HWIO_LPASS_CORE_PERF0_DFSR_IN(x))
#define HWIO_LPASS_CORE_PERF0_DFSR_MODE_BMSK                                                                              0x3000
#define HWIO_LPASS_CORE_PERF0_DFSR_MODE_SHFT                                                                                  12
#define HWIO_LPASS_CORE_PERF0_DFSR_SRC_SEL_BMSK                                                                            0x700
#define HWIO_LPASS_CORE_PERF0_DFSR_SRC_SEL_SHFT                                                                                8
#define HWIO_LPASS_CORE_PERF0_DFSR_SRC_DIV_BMSK                                                                             0x1f
#define HWIO_LPASS_CORE_PERF0_DFSR_SRC_DIV_SHFT                                                                                0

#define HWIO_LPASS_CORE_PERF1_DFSR_ADDR(x)                                                                            ((x) + 0x1d01c)
#define HWIO_LPASS_CORE_PERF1_DFSR_RMSK                                                                                   0x371f
#define HWIO_LPASS_CORE_PERF1_DFSR_IN(x)            \
                in_dword(HWIO_LPASS_CORE_PERF1_DFSR_ADDR(x))
#define HWIO_LPASS_CORE_PERF1_DFSR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_CORE_PERF1_DFSR_ADDR(x), m)
#define HWIO_LPASS_CORE_PERF1_DFSR_OUT(x, v)            \
                out_dword(HWIO_LPASS_CORE_PERF1_DFSR_ADDR(x),v)
#define HWIO_LPASS_CORE_PERF1_DFSR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_CORE_PERF1_DFSR_ADDR(x),m,v,HWIO_LPASS_CORE_PERF1_DFSR_IN(x))
#define HWIO_LPASS_CORE_PERF1_DFSR_MODE_BMSK                                                                              0x3000
#define HWIO_LPASS_CORE_PERF1_DFSR_MODE_SHFT                                                                                  12
#define HWIO_LPASS_CORE_PERF1_DFSR_SRC_SEL_BMSK                                                                            0x700
#define HWIO_LPASS_CORE_PERF1_DFSR_SRC_SEL_SHFT                                                                                8
#define HWIO_LPASS_CORE_PERF1_DFSR_SRC_DIV_BMSK                                                                             0x1f
#define HWIO_LPASS_CORE_PERF1_DFSR_SRC_DIV_SHFT                                                                                0

#define HWIO_LPASS_CORE_PERF2_DFSR_ADDR(x)                                                                            ((x) + 0x1d020)
#define HWIO_LPASS_CORE_PERF2_DFSR_RMSK                                                                                   0x371f
#define HWIO_LPASS_CORE_PERF2_DFSR_IN(x)            \
                in_dword(HWIO_LPASS_CORE_PERF2_DFSR_ADDR(x))
#define HWIO_LPASS_CORE_PERF2_DFSR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_CORE_PERF2_DFSR_ADDR(x), m)
#define HWIO_LPASS_CORE_PERF2_DFSR_OUT(x, v)            \
                out_dword(HWIO_LPASS_CORE_PERF2_DFSR_ADDR(x),v)
#define HWIO_LPASS_CORE_PERF2_DFSR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_CORE_PERF2_DFSR_ADDR(x),m,v,HWIO_LPASS_CORE_PERF2_DFSR_IN(x))
#define HWIO_LPASS_CORE_PERF2_DFSR_MODE_BMSK                                                                              0x3000
#define HWIO_LPASS_CORE_PERF2_DFSR_MODE_SHFT                                                                                  12
#define HWIO_LPASS_CORE_PERF2_DFSR_SRC_SEL_BMSK                                                                            0x700
#define HWIO_LPASS_CORE_PERF2_DFSR_SRC_SEL_SHFT                                                                                8
#define HWIO_LPASS_CORE_PERF2_DFSR_SRC_DIV_BMSK                                                                             0x1f
#define HWIO_LPASS_CORE_PERF2_DFSR_SRC_DIV_SHFT                                                                                0

#define HWIO_LPASS_CORE_PERF3_DFSR_ADDR(x)                                                                            ((x) + 0x1d024)
#define HWIO_LPASS_CORE_PERF3_DFSR_RMSK                                                                                   0x371f
#define HWIO_LPASS_CORE_PERF3_DFSR_IN(x)            \
                in_dword(HWIO_LPASS_CORE_PERF3_DFSR_ADDR(x))
#define HWIO_LPASS_CORE_PERF3_DFSR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_CORE_PERF3_DFSR_ADDR(x), m)
#define HWIO_LPASS_CORE_PERF3_DFSR_OUT(x, v)            \
                out_dword(HWIO_LPASS_CORE_PERF3_DFSR_ADDR(x),v)
#define HWIO_LPASS_CORE_PERF3_DFSR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_CORE_PERF3_DFSR_ADDR(x),m,v,HWIO_LPASS_CORE_PERF3_DFSR_IN(x))
#define HWIO_LPASS_CORE_PERF3_DFSR_MODE_BMSK                                                                              0x3000
#define HWIO_LPASS_CORE_PERF3_DFSR_MODE_SHFT                                                                                  12
#define HWIO_LPASS_CORE_PERF3_DFSR_SRC_SEL_BMSK                                                                            0x700
#define HWIO_LPASS_CORE_PERF3_DFSR_SRC_SEL_SHFT                                                                                8
#define HWIO_LPASS_CORE_PERF3_DFSR_SRC_DIV_BMSK                                                                             0x1f
#define HWIO_LPASS_CORE_PERF3_DFSR_SRC_DIV_SHFT                                                                                0

#define HWIO_LPASS_CORE_PERF4_DFSR_ADDR(x)                                                                            ((x) + 0x1d028)
#define HWIO_LPASS_CORE_PERF4_DFSR_RMSK                                                                                   0x371f
#define HWIO_LPASS_CORE_PERF4_DFSR_IN(x)            \
                in_dword(HWIO_LPASS_CORE_PERF4_DFSR_ADDR(x))
#define HWIO_LPASS_CORE_PERF4_DFSR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_CORE_PERF4_DFSR_ADDR(x), m)
#define HWIO_LPASS_CORE_PERF4_DFSR_OUT(x, v)            \
                out_dword(HWIO_LPASS_CORE_PERF4_DFSR_ADDR(x),v)
#define HWIO_LPASS_CORE_PERF4_DFSR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_CORE_PERF4_DFSR_ADDR(x),m,v,HWIO_LPASS_CORE_PERF4_DFSR_IN(x))
#define HWIO_LPASS_CORE_PERF4_DFSR_MODE_BMSK                                                                              0x3000
#define HWIO_LPASS_CORE_PERF4_DFSR_MODE_SHFT                                                                                  12
#define HWIO_LPASS_CORE_PERF4_DFSR_SRC_SEL_BMSK                                                                            0x700
#define HWIO_LPASS_CORE_PERF4_DFSR_SRC_SEL_SHFT                                                                                8
#define HWIO_LPASS_CORE_PERF4_DFSR_SRC_DIV_BMSK                                                                             0x1f
#define HWIO_LPASS_CORE_PERF4_DFSR_SRC_DIV_SHFT                                                                                0

#define HWIO_LPASS_CORE_PERF5_DFSR_ADDR(x)                                                                            ((x) + 0x1d02c)
#define HWIO_LPASS_CORE_PERF5_DFSR_RMSK                                                                                   0x371f
#define HWIO_LPASS_CORE_PERF5_DFSR_IN(x)            \
                in_dword(HWIO_LPASS_CORE_PERF5_DFSR_ADDR(x))
#define HWIO_LPASS_CORE_PERF5_DFSR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_CORE_PERF5_DFSR_ADDR(x), m)
#define HWIO_LPASS_CORE_PERF5_DFSR_OUT(x, v)            \
                out_dword(HWIO_LPASS_CORE_PERF5_DFSR_ADDR(x),v)
#define HWIO_LPASS_CORE_PERF5_DFSR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_CORE_PERF5_DFSR_ADDR(x),m,v,HWIO_LPASS_CORE_PERF5_DFSR_IN(x))
#define HWIO_LPASS_CORE_PERF5_DFSR_MODE_BMSK                                                                              0x3000
#define HWIO_LPASS_CORE_PERF5_DFSR_MODE_SHFT                                                                                  12
#define HWIO_LPASS_CORE_PERF5_DFSR_SRC_SEL_BMSK                                                                            0x700
#define HWIO_LPASS_CORE_PERF5_DFSR_SRC_SEL_SHFT                                                                                8
#define HWIO_LPASS_CORE_PERF5_DFSR_SRC_DIV_BMSK                                                                             0x1f
#define HWIO_LPASS_CORE_PERF5_DFSR_SRC_DIV_SHFT                                                                                0

#define HWIO_LPASS_CORE_PERF6_DFSR_ADDR(x)                                                                            ((x) + 0x1d030)
#define HWIO_LPASS_CORE_PERF6_DFSR_RMSK                                                                                   0x371f
#define HWIO_LPASS_CORE_PERF6_DFSR_IN(x)            \
                in_dword(HWIO_LPASS_CORE_PERF6_DFSR_ADDR(x))
#define HWIO_LPASS_CORE_PERF6_DFSR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_CORE_PERF6_DFSR_ADDR(x), m)
#define HWIO_LPASS_CORE_PERF6_DFSR_OUT(x, v)            \
                out_dword(HWIO_LPASS_CORE_PERF6_DFSR_ADDR(x),v)
#define HWIO_LPASS_CORE_PERF6_DFSR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_CORE_PERF6_DFSR_ADDR(x),m,v,HWIO_LPASS_CORE_PERF6_DFSR_IN(x))
#define HWIO_LPASS_CORE_PERF6_DFSR_MODE_BMSK                                                                              0x3000
#define HWIO_LPASS_CORE_PERF6_DFSR_MODE_SHFT                                                                                  12
#define HWIO_LPASS_CORE_PERF6_DFSR_SRC_SEL_BMSK                                                                            0x700
#define HWIO_LPASS_CORE_PERF6_DFSR_SRC_SEL_SHFT                                                                                8
#define HWIO_LPASS_CORE_PERF6_DFSR_SRC_DIV_BMSK                                                                             0x1f
#define HWIO_LPASS_CORE_PERF6_DFSR_SRC_DIV_SHFT                                                                                0

#define HWIO_LPASS_CORE_PERF7_DFSR_ADDR(x)                                                                            ((x) + 0x1d034)
#define HWIO_LPASS_CORE_PERF7_DFSR_RMSK                                                                                   0x371f
#define HWIO_LPASS_CORE_PERF7_DFSR_IN(x)            \
                in_dword(HWIO_LPASS_CORE_PERF7_DFSR_ADDR(x))
#define HWIO_LPASS_CORE_PERF7_DFSR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_CORE_PERF7_DFSR_ADDR(x), m)
#define HWIO_LPASS_CORE_PERF7_DFSR_OUT(x, v)            \
                out_dword(HWIO_LPASS_CORE_PERF7_DFSR_ADDR(x),v)
#define HWIO_LPASS_CORE_PERF7_DFSR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_CORE_PERF7_DFSR_ADDR(x),m,v,HWIO_LPASS_CORE_PERF7_DFSR_IN(x))
#define HWIO_LPASS_CORE_PERF7_DFSR_MODE_BMSK                                                                              0x3000
#define HWIO_LPASS_CORE_PERF7_DFSR_MODE_SHFT                                                                                  12
#define HWIO_LPASS_CORE_PERF7_DFSR_SRC_SEL_BMSK                                                                            0x700
#define HWIO_LPASS_CORE_PERF7_DFSR_SRC_SEL_SHFT                                                                                8
#define HWIO_LPASS_CORE_PERF7_DFSR_SRC_DIV_BMSK                                                                             0x1f
#define HWIO_LPASS_CORE_PERF7_DFSR_SRC_DIV_SHFT                                                                                0

#define HWIO_LPASS_CORE_PERF0_M_DFSR_ADDR(x)                                                                          ((x) + 0x1d038)
#define HWIO_LPASS_CORE_PERF0_M_DFSR_RMSK                                                                                   0xff
#define HWIO_LPASS_CORE_PERF0_M_DFSR_IN(x)            \
                in_dword(HWIO_LPASS_CORE_PERF0_M_DFSR_ADDR(x))
#define HWIO_LPASS_CORE_PERF0_M_DFSR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_CORE_PERF0_M_DFSR_ADDR(x), m)
#define HWIO_LPASS_CORE_PERF0_M_DFSR_OUT(x, v)            \
                out_dword(HWIO_LPASS_CORE_PERF0_M_DFSR_ADDR(x),v)
#define HWIO_LPASS_CORE_PERF0_M_DFSR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_CORE_PERF0_M_DFSR_ADDR(x),m,v,HWIO_LPASS_CORE_PERF0_M_DFSR_IN(x))
#define HWIO_LPASS_CORE_PERF0_M_DFSR_M_BMSK                                                                                 0xff
#define HWIO_LPASS_CORE_PERF0_M_DFSR_M_SHFT                                                                                    0

#define HWIO_LPASS_CORE_PERF1_M_DFSR_ADDR(x)                                                                          ((x) + 0x1d03c)
#define HWIO_LPASS_CORE_PERF1_M_DFSR_RMSK                                                                                   0xff
#define HWIO_LPASS_CORE_PERF1_M_DFSR_IN(x)            \
                in_dword(HWIO_LPASS_CORE_PERF1_M_DFSR_ADDR(x))
#define HWIO_LPASS_CORE_PERF1_M_DFSR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_CORE_PERF1_M_DFSR_ADDR(x), m)
#define HWIO_LPASS_CORE_PERF1_M_DFSR_OUT(x, v)            \
                out_dword(HWIO_LPASS_CORE_PERF1_M_DFSR_ADDR(x),v)
#define HWIO_LPASS_CORE_PERF1_M_DFSR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_CORE_PERF1_M_DFSR_ADDR(x),m,v,HWIO_LPASS_CORE_PERF1_M_DFSR_IN(x))
#define HWIO_LPASS_CORE_PERF1_M_DFSR_M_BMSK                                                                                 0xff
#define HWIO_LPASS_CORE_PERF1_M_DFSR_M_SHFT                                                                                    0

#define HWIO_LPASS_CORE_PERF2_M_DFSR_ADDR(x)                                                                          ((x) + 0x1d040)
#define HWIO_LPASS_CORE_PERF2_M_DFSR_RMSK                                                                                   0xff
#define HWIO_LPASS_CORE_PERF2_M_DFSR_IN(x)            \
                in_dword(HWIO_LPASS_CORE_PERF2_M_DFSR_ADDR(x))
#define HWIO_LPASS_CORE_PERF2_M_DFSR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_CORE_PERF2_M_DFSR_ADDR(x), m)
#define HWIO_LPASS_CORE_PERF2_M_DFSR_OUT(x, v)            \
                out_dword(HWIO_LPASS_CORE_PERF2_M_DFSR_ADDR(x),v)
#define HWIO_LPASS_CORE_PERF2_M_DFSR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_CORE_PERF2_M_DFSR_ADDR(x),m,v,HWIO_LPASS_CORE_PERF2_M_DFSR_IN(x))
#define HWIO_LPASS_CORE_PERF2_M_DFSR_M_BMSK                                                                                 0xff
#define HWIO_LPASS_CORE_PERF2_M_DFSR_M_SHFT                                                                                    0

#define HWIO_LPASS_CORE_PERF3_M_DFSR_ADDR(x)                                                                          ((x) + 0x1d044)
#define HWIO_LPASS_CORE_PERF3_M_DFSR_RMSK                                                                                   0xff
#define HWIO_LPASS_CORE_PERF3_M_DFSR_IN(x)            \
                in_dword(HWIO_LPASS_CORE_PERF3_M_DFSR_ADDR(x))
#define HWIO_LPASS_CORE_PERF3_M_DFSR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_CORE_PERF3_M_DFSR_ADDR(x), m)
#define HWIO_LPASS_CORE_PERF3_M_DFSR_OUT(x, v)            \
                out_dword(HWIO_LPASS_CORE_PERF3_M_DFSR_ADDR(x),v)
#define HWIO_LPASS_CORE_PERF3_M_DFSR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_CORE_PERF3_M_DFSR_ADDR(x),m,v,HWIO_LPASS_CORE_PERF3_M_DFSR_IN(x))
#define HWIO_LPASS_CORE_PERF3_M_DFSR_M_BMSK                                                                                 0xff
#define HWIO_LPASS_CORE_PERF3_M_DFSR_M_SHFT                                                                                    0

#define HWIO_LPASS_CORE_PERF4_M_DFSR_ADDR(x)                                                                          ((x) + 0x1d048)
#define HWIO_LPASS_CORE_PERF4_M_DFSR_RMSK                                                                                   0xff
#define HWIO_LPASS_CORE_PERF4_M_DFSR_IN(x)            \
                in_dword(HWIO_LPASS_CORE_PERF4_M_DFSR_ADDR(x))
#define HWIO_LPASS_CORE_PERF4_M_DFSR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_CORE_PERF4_M_DFSR_ADDR(x), m)
#define HWIO_LPASS_CORE_PERF4_M_DFSR_OUT(x, v)            \
                out_dword(HWIO_LPASS_CORE_PERF4_M_DFSR_ADDR(x),v)
#define HWIO_LPASS_CORE_PERF4_M_DFSR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_CORE_PERF4_M_DFSR_ADDR(x),m,v,HWIO_LPASS_CORE_PERF4_M_DFSR_IN(x))
#define HWIO_LPASS_CORE_PERF4_M_DFSR_M_BMSK                                                                                 0xff
#define HWIO_LPASS_CORE_PERF4_M_DFSR_M_SHFT                                                                                    0

#define HWIO_LPASS_CORE_PERF5_M_DFSR_ADDR(x)                                                                          ((x) + 0x1d04c)
#define HWIO_LPASS_CORE_PERF5_M_DFSR_RMSK                                                                                   0xff
#define HWIO_LPASS_CORE_PERF5_M_DFSR_IN(x)            \
                in_dword(HWIO_LPASS_CORE_PERF5_M_DFSR_ADDR(x))
#define HWIO_LPASS_CORE_PERF5_M_DFSR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_CORE_PERF5_M_DFSR_ADDR(x), m)
#define HWIO_LPASS_CORE_PERF5_M_DFSR_OUT(x, v)            \
                out_dword(HWIO_LPASS_CORE_PERF5_M_DFSR_ADDR(x),v)
#define HWIO_LPASS_CORE_PERF5_M_DFSR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_CORE_PERF5_M_DFSR_ADDR(x),m,v,HWIO_LPASS_CORE_PERF5_M_DFSR_IN(x))
#define HWIO_LPASS_CORE_PERF5_M_DFSR_M_BMSK                                                                                 0xff
#define HWIO_LPASS_CORE_PERF5_M_DFSR_M_SHFT                                                                                    0

#define HWIO_LPASS_CORE_PERF6_M_DFSR_ADDR(x)                                                                          ((x) + 0x1d050)
#define HWIO_LPASS_CORE_PERF6_M_DFSR_RMSK                                                                                   0xff
#define HWIO_LPASS_CORE_PERF6_M_DFSR_IN(x)            \
                in_dword(HWIO_LPASS_CORE_PERF6_M_DFSR_ADDR(x))
#define HWIO_LPASS_CORE_PERF6_M_DFSR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_CORE_PERF6_M_DFSR_ADDR(x), m)
#define HWIO_LPASS_CORE_PERF6_M_DFSR_OUT(x, v)            \
                out_dword(HWIO_LPASS_CORE_PERF6_M_DFSR_ADDR(x),v)
#define HWIO_LPASS_CORE_PERF6_M_DFSR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_CORE_PERF6_M_DFSR_ADDR(x),m,v,HWIO_LPASS_CORE_PERF6_M_DFSR_IN(x))
#define HWIO_LPASS_CORE_PERF6_M_DFSR_M_BMSK                                                                                 0xff
#define HWIO_LPASS_CORE_PERF6_M_DFSR_M_SHFT                                                                                    0

#define HWIO_LPASS_CORE_PERF7_M_DFSR_ADDR(x)                                                                          ((x) + 0x1d054)
#define HWIO_LPASS_CORE_PERF7_M_DFSR_RMSK                                                                                   0xff
#define HWIO_LPASS_CORE_PERF7_M_DFSR_IN(x)            \
                in_dword(HWIO_LPASS_CORE_PERF7_M_DFSR_ADDR(x))
#define HWIO_LPASS_CORE_PERF7_M_DFSR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_CORE_PERF7_M_DFSR_ADDR(x), m)
#define HWIO_LPASS_CORE_PERF7_M_DFSR_OUT(x, v)            \
                out_dword(HWIO_LPASS_CORE_PERF7_M_DFSR_ADDR(x),v)
#define HWIO_LPASS_CORE_PERF7_M_DFSR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_CORE_PERF7_M_DFSR_ADDR(x),m,v,HWIO_LPASS_CORE_PERF7_M_DFSR_IN(x))
#define HWIO_LPASS_CORE_PERF7_M_DFSR_M_BMSK                                                                                 0xff
#define HWIO_LPASS_CORE_PERF7_M_DFSR_M_SHFT                                                                                    0

#define HWIO_LPASS_CORE_PERF0_N_DFSR_ADDR(x)                                                                          ((x) + 0x1d058)
#define HWIO_LPASS_CORE_PERF0_N_DFSR_RMSK                                                                                   0xff
#define HWIO_LPASS_CORE_PERF0_N_DFSR_IN(x)            \
                in_dword(HWIO_LPASS_CORE_PERF0_N_DFSR_ADDR(x))
#define HWIO_LPASS_CORE_PERF0_N_DFSR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_CORE_PERF0_N_DFSR_ADDR(x), m)
#define HWIO_LPASS_CORE_PERF0_N_DFSR_OUT(x, v)            \
                out_dword(HWIO_LPASS_CORE_PERF0_N_DFSR_ADDR(x),v)
#define HWIO_LPASS_CORE_PERF0_N_DFSR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_CORE_PERF0_N_DFSR_ADDR(x),m,v,HWIO_LPASS_CORE_PERF0_N_DFSR_IN(x))
#define HWIO_LPASS_CORE_PERF0_N_DFSR_NOT_N_MINUS_M_BMSK                                                                     0xff
#define HWIO_LPASS_CORE_PERF0_N_DFSR_NOT_N_MINUS_M_SHFT                                                                        0

#define HWIO_LPASS_CORE_PERF1_N_DFSR_ADDR(x)                                                                          ((x) + 0x1d05c)
#define HWIO_LPASS_CORE_PERF1_N_DFSR_RMSK                                                                                   0xff
#define HWIO_LPASS_CORE_PERF1_N_DFSR_IN(x)            \
                in_dword(HWIO_LPASS_CORE_PERF1_N_DFSR_ADDR(x))
#define HWIO_LPASS_CORE_PERF1_N_DFSR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_CORE_PERF1_N_DFSR_ADDR(x), m)
#define HWIO_LPASS_CORE_PERF1_N_DFSR_OUT(x, v)            \
                out_dword(HWIO_LPASS_CORE_PERF1_N_DFSR_ADDR(x),v)
#define HWIO_LPASS_CORE_PERF1_N_DFSR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_CORE_PERF1_N_DFSR_ADDR(x),m,v,HWIO_LPASS_CORE_PERF1_N_DFSR_IN(x))
#define HWIO_LPASS_CORE_PERF1_N_DFSR_NOT_N_MINUS_M_BMSK                                                                     0xff
#define HWIO_LPASS_CORE_PERF1_N_DFSR_NOT_N_MINUS_M_SHFT                                                                        0

#define HWIO_LPASS_CORE_PERF2_N_DFSR_ADDR(x)                                                                          ((x) + 0x1d060)
#define HWIO_LPASS_CORE_PERF2_N_DFSR_RMSK                                                                                   0xff
#define HWIO_LPASS_CORE_PERF2_N_DFSR_IN(x)            \
                in_dword(HWIO_LPASS_CORE_PERF2_N_DFSR_ADDR(x))
#define HWIO_LPASS_CORE_PERF2_N_DFSR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_CORE_PERF2_N_DFSR_ADDR(x), m)
#define HWIO_LPASS_CORE_PERF2_N_DFSR_OUT(x, v)            \
                out_dword(HWIO_LPASS_CORE_PERF2_N_DFSR_ADDR(x),v)
#define HWIO_LPASS_CORE_PERF2_N_DFSR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_CORE_PERF2_N_DFSR_ADDR(x),m,v,HWIO_LPASS_CORE_PERF2_N_DFSR_IN(x))
#define HWIO_LPASS_CORE_PERF2_N_DFSR_NOT_N_MINUS_M_BMSK                                                                     0xff
#define HWIO_LPASS_CORE_PERF2_N_DFSR_NOT_N_MINUS_M_SHFT                                                                        0

#define HWIO_LPASS_CORE_PERF3_N_DFSR_ADDR(x)                                                                          ((x) + 0x1d064)
#define HWIO_LPASS_CORE_PERF3_N_DFSR_RMSK                                                                                   0xff
#define HWIO_LPASS_CORE_PERF3_N_DFSR_IN(x)            \
                in_dword(HWIO_LPASS_CORE_PERF3_N_DFSR_ADDR(x))
#define HWIO_LPASS_CORE_PERF3_N_DFSR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_CORE_PERF3_N_DFSR_ADDR(x), m)
#define HWIO_LPASS_CORE_PERF3_N_DFSR_OUT(x, v)            \
                out_dword(HWIO_LPASS_CORE_PERF3_N_DFSR_ADDR(x),v)
#define HWIO_LPASS_CORE_PERF3_N_DFSR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_CORE_PERF3_N_DFSR_ADDR(x),m,v,HWIO_LPASS_CORE_PERF3_N_DFSR_IN(x))
#define HWIO_LPASS_CORE_PERF3_N_DFSR_NOT_N_MINUS_M_BMSK                                                                     0xff
#define HWIO_LPASS_CORE_PERF3_N_DFSR_NOT_N_MINUS_M_SHFT                                                                        0

#define HWIO_LPASS_CORE_PERF4_N_DFSR_ADDR(x)                                                                          ((x) + 0x1d068)
#define HWIO_LPASS_CORE_PERF4_N_DFSR_RMSK                                                                                   0xff
#define HWIO_LPASS_CORE_PERF4_N_DFSR_IN(x)            \
                in_dword(HWIO_LPASS_CORE_PERF4_N_DFSR_ADDR(x))
#define HWIO_LPASS_CORE_PERF4_N_DFSR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_CORE_PERF4_N_DFSR_ADDR(x), m)
#define HWIO_LPASS_CORE_PERF4_N_DFSR_OUT(x, v)            \
                out_dword(HWIO_LPASS_CORE_PERF4_N_DFSR_ADDR(x),v)
#define HWIO_LPASS_CORE_PERF4_N_DFSR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_CORE_PERF4_N_DFSR_ADDR(x),m,v,HWIO_LPASS_CORE_PERF4_N_DFSR_IN(x))
#define HWIO_LPASS_CORE_PERF4_N_DFSR_NOT_N_MINUS_M_BMSK                                                                     0xff
#define HWIO_LPASS_CORE_PERF4_N_DFSR_NOT_N_MINUS_M_SHFT                                                                        0

#define HWIO_LPASS_CORE_PERF5_N_DFSR_ADDR(x)                                                                          ((x) + 0x1d06c)
#define HWIO_LPASS_CORE_PERF5_N_DFSR_RMSK                                                                                   0xff
#define HWIO_LPASS_CORE_PERF5_N_DFSR_IN(x)            \
                in_dword(HWIO_LPASS_CORE_PERF5_N_DFSR_ADDR(x))
#define HWIO_LPASS_CORE_PERF5_N_DFSR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_CORE_PERF5_N_DFSR_ADDR(x), m)
#define HWIO_LPASS_CORE_PERF5_N_DFSR_OUT(x, v)            \
                out_dword(HWIO_LPASS_CORE_PERF5_N_DFSR_ADDR(x),v)
#define HWIO_LPASS_CORE_PERF5_N_DFSR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_CORE_PERF5_N_DFSR_ADDR(x),m,v,HWIO_LPASS_CORE_PERF5_N_DFSR_IN(x))
#define HWIO_LPASS_CORE_PERF5_N_DFSR_NOT_N_MINUS_M_BMSK                                                                     0xff
#define HWIO_LPASS_CORE_PERF5_N_DFSR_NOT_N_MINUS_M_SHFT                                                                        0

#define HWIO_LPASS_CORE_PERF6_N_DFSR_ADDR(x)                                                                          ((x) + 0x1d070)
#define HWIO_LPASS_CORE_PERF6_N_DFSR_RMSK                                                                                   0xff
#define HWIO_LPASS_CORE_PERF6_N_DFSR_IN(x)            \
                in_dword(HWIO_LPASS_CORE_PERF6_N_DFSR_ADDR(x))
#define HWIO_LPASS_CORE_PERF6_N_DFSR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_CORE_PERF6_N_DFSR_ADDR(x), m)
#define HWIO_LPASS_CORE_PERF6_N_DFSR_OUT(x, v)            \
                out_dword(HWIO_LPASS_CORE_PERF6_N_DFSR_ADDR(x),v)
#define HWIO_LPASS_CORE_PERF6_N_DFSR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_CORE_PERF6_N_DFSR_ADDR(x),m,v,HWIO_LPASS_CORE_PERF6_N_DFSR_IN(x))
#define HWIO_LPASS_CORE_PERF6_N_DFSR_NOT_N_MINUS_M_BMSK                                                                     0xff
#define HWIO_LPASS_CORE_PERF6_N_DFSR_NOT_N_MINUS_M_SHFT                                                                        0

#define HWIO_LPASS_CORE_PERF7_N_DFSR_ADDR(x)                                                                          ((x) + 0x1d074)
#define HWIO_LPASS_CORE_PERF7_N_DFSR_RMSK                                                                                   0xff
#define HWIO_LPASS_CORE_PERF7_N_DFSR_IN(x)            \
                in_dword(HWIO_LPASS_CORE_PERF7_N_DFSR_ADDR(x))
#define HWIO_LPASS_CORE_PERF7_N_DFSR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_CORE_PERF7_N_DFSR_ADDR(x), m)
#define HWIO_LPASS_CORE_PERF7_N_DFSR_OUT(x, v)            \
                out_dword(HWIO_LPASS_CORE_PERF7_N_DFSR_ADDR(x),v)
#define HWIO_LPASS_CORE_PERF7_N_DFSR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_CORE_PERF7_N_DFSR_ADDR(x),m,v,HWIO_LPASS_CORE_PERF7_N_DFSR_IN(x))
#define HWIO_LPASS_CORE_PERF7_N_DFSR_NOT_N_MINUS_M_BMSK                                                                     0xff
#define HWIO_LPASS_CORE_PERF7_N_DFSR_NOT_N_MINUS_M_SHFT                                                                        0

#define HWIO_LPASS_CORE_PERF0_D_DFSR_ADDR(x)                                                                          ((x) + 0x1d078)
#define HWIO_LPASS_CORE_PERF0_D_DFSR_RMSK                                                                                   0xff
#define HWIO_LPASS_CORE_PERF0_D_DFSR_IN(x)            \
                in_dword(HWIO_LPASS_CORE_PERF0_D_DFSR_ADDR(x))
#define HWIO_LPASS_CORE_PERF0_D_DFSR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_CORE_PERF0_D_DFSR_ADDR(x), m)
#define HWIO_LPASS_CORE_PERF0_D_DFSR_OUT(x, v)            \
                out_dword(HWIO_LPASS_CORE_PERF0_D_DFSR_ADDR(x),v)
#define HWIO_LPASS_CORE_PERF0_D_DFSR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_CORE_PERF0_D_DFSR_ADDR(x),m,v,HWIO_LPASS_CORE_PERF0_D_DFSR_IN(x))
#define HWIO_LPASS_CORE_PERF0_D_DFSR_NOT_2D_BMSK                                                                            0xff
#define HWIO_LPASS_CORE_PERF0_D_DFSR_NOT_2D_SHFT                                                                               0

#define HWIO_LPASS_CORE_PERF1_D_DFSR_ADDR(x)                                                                          ((x) + 0x1d07c)
#define HWIO_LPASS_CORE_PERF1_D_DFSR_RMSK                                                                                   0xff
#define HWIO_LPASS_CORE_PERF1_D_DFSR_IN(x)            \
                in_dword(HWIO_LPASS_CORE_PERF1_D_DFSR_ADDR(x))
#define HWIO_LPASS_CORE_PERF1_D_DFSR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_CORE_PERF1_D_DFSR_ADDR(x), m)
#define HWIO_LPASS_CORE_PERF1_D_DFSR_OUT(x, v)            \
                out_dword(HWIO_LPASS_CORE_PERF1_D_DFSR_ADDR(x),v)
#define HWIO_LPASS_CORE_PERF1_D_DFSR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_CORE_PERF1_D_DFSR_ADDR(x),m,v,HWIO_LPASS_CORE_PERF1_D_DFSR_IN(x))
#define HWIO_LPASS_CORE_PERF1_D_DFSR_NOT_2D_BMSK                                                                            0xff
#define HWIO_LPASS_CORE_PERF1_D_DFSR_NOT_2D_SHFT                                                                               0

#define HWIO_LPASS_CORE_PERF2_D_DFSR_ADDR(x)                                                                          ((x) + 0x1d080)
#define HWIO_LPASS_CORE_PERF2_D_DFSR_RMSK                                                                                   0xff
#define HWIO_LPASS_CORE_PERF2_D_DFSR_IN(x)            \
                in_dword(HWIO_LPASS_CORE_PERF2_D_DFSR_ADDR(x))
#define HWIO_LPASS_CORE_PERF2_D_DFSR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_CORE_PERF2_D_DFSR_ADDR(x), m)
#define HWIO_LPASS_CORE_PERF2_D_DFSR_OUT(x, v)            \
                out_dword(HWIO_LPASS_CORE_PERF2_D_DFSR_ADDR(x),v)
#define HWIO_LPASS_CORE_PERF2_D_DFSR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_CORE_PERF2_D_DFSR_ADDR(x),m,v,HWIO_LPASS_CORE_PERF2_D_DFSR_IN(x))
#define HWIO_LPASS_CORE_PERF2_D_DFSR_NOT_2D_BMSK                                                                            0xff
#define HWIO_LPASS_CORE_PERF2_D_DFSR_NOT_2D_SHFT                                                                               0

#define HWIO_LPASS_CORE_PERF3_D_DFSR_ADDR(x)                                                                          ((x) + 0x1d084)
#define HWIO_LPASS_CORE_PERF3_D_DFSR_RMSK                                                                                   0xff
#define HWIO_LPASS_CORE_PERF3_D_DFSR_IN(x)            \
                in_dword(HWIO_LPASS_CORE_PERF3_D_DFSR_ADDR(x))
#define HWIO_LPASS_CORE_PERF3_D_DFSR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_CORE_PERF3_D_DFSR_ADDR(x), m)
#define HWIO_LPASS_CORE_PERF3_D_DFSR_OUT(x, v)            \
                out_dword(HWIO_LPASS_CORE_PERF3_D_DFSR_ADDR(x),v)
#define HWIO_LPASS_CORE_PERF3_D_DFSR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_CORE_PERF3_D_DFSR_ADDR(x),m,v,HWIO_LPASS_CORE_PERF3_D_DFSR_IN(x))
#define HWIO_LPASS_CORE_PERF3_D_DFSR_NOT_2D_BMSK                                                                            0xff
#define HWIO_LPASS_CORE_PERF3_D_DFSR_NOT_2D_SHFT                                                                               0

#define HWIO_LPASS_CORE_PERF4_D_DFSR_ADDR(x)                                                                          ((x) + 0x1d088)
#define HWIO_LPASS_CORE_PERF4_D_DFSR_RMSK                                                                                   0xff
#define HWIO_LPASS_CORE_PERF4_D_DFSR_IN(x)            \
                in_dword(HWIO_LPASS_CORE_PERF4_D_DFSR_ADDR(x))
#define HWIO_LPASS_CORE_PERF4_D_DFSR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_CORE_PERF4_D_DFSR_ADDR(x), m)
#define HWIO_LPASS_CORE_PERF4_D_DFSR_OUT(x, v)            \
                out_dword(HWIO_LPASS_CORE_PERF4_D_DFSR_ADDR(x),v)
#define HWIO_LPASS_CORE_PERF4_D_DFSR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_CORE_PERF4_D_DFSR_ADDR(x),m,v,HWIO_LPASS_CORE_PERF4_D_DFSR_IN(x))
#define HWIO_LPASS_CORE_PERF4_D_DFSR_NOT_2D_BMSK                                                                            0xff
#define HWIO_LPASS_CORE_PERF4_D_DFSR_NOT_2D_SHFT                                                                               0

#define HWIO_LPASS_CORE_PERF5_D_DFSR_ADDR(x)                                                                          ((x) + 0x1d08c)
#define HWIO_LPASS_CORE_PERF5_D_DFSR_RMSK                                                                                   0xff
#define HWIO_LPASS_CORE_PERF5_D_DFSR_IN(x)            \
                in_dword(HWIO_LPASS_CORE_PERF5_D_DFSR_ADDR(x))
#define HWIO_LPASS_CORE_PERF5_D_DFSR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_CORE_PERF5_D_DFSR_ADDR(x), m)
#define HWIO_LPASS_CORE_PERF5_D_DFSR_OUT(x, v)            \
                out_dword(HWIO_LPASS_CORE_PERF5_D_DFSR_ADDR(x),v)
#define HWIO_LPASS_CORE_PERF5_D_DFSR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_CORE_PERF5_D_DFSR_ADDR(x),m,v,HWIO_LPASS_CORE_PERF5_D_DFSR_IN(x))
#define HWIO_LPASS_CORE_PERF5_D_DFSR_NOT_2D_BMSK                                                                            0xff
#define HWIO_LPASS_CORE_PERF5_D_DFSR_NOT_2D_SHFT                                                                               0

#define HWIO_LPASS_CORE_PERF6_D_DFSR_ADDR(x)                                                                          ((x) + 0x1d090)
#define HWIO_LPASS_CORE_PERF6_D_DFSR_RMSK                                                                                   0xff
#define HWIO_LPASS_CORE_PERF6_D_DFSR_IN(x)            \
                in_dword(HWIO_LPASS_CORE_PERF6_D_DFSR_ADDR(x))
#define HWIO_LPASS_CORE_PERF6_D_DFSR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_CORE_PERF6_D_DFSR_ADDR(x), m)
#define HWIO_LPASS_CORE_PERF6_D_DFSR_OUT(x, v)            \
                out_dword(HWIO_LPASS_CORE_PERF6_D_DFSR_ADDR(x),v)
#define HWIO_LPASS_CORE_PERF6_D_DFSR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_CORE_PERF6_D_DFSR_ADDR(x),m,v,HWIO_LPASS_CORE_PERF6_D_DFSR_IN(x))
#define HWIO_LPASS_CORE_PERF6_D_DFSR_NOT_2D_BMSK                                                                            0xff
#define HWIO_LPASS_CORE_PERF6_D_DFSR_NOT_2D_SHFT                                                                               0

#define HWIO_LPASS_CORE_PERF7_D_DFSR_ADDR(x)                                                                          ((x) + 0x1d094)
#define HWIO_LPASS_CORE_PERF7_D_DFSR_RMSK                                                                                   0xff
#define HWIO_LPASS_CORE_PERF7_D_DFSR_IN(x)            \
                in_dword(HWIO_LPASS_CORE_PERF7_D_DFSR_ADDR(x))
#define HWIO_LPASS_CORE_PERF7_D_DFSR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_CORE_PERF7_D_DFSR_ADDR(x), m)
#define HWIO_LPASS_CORE_PERF7_D_DFSR_OUT(x, v)            \
                out_dword(HWIO_LPASS_CORE_PERF7_D_DFSR_ADDR(x),v)
#define HWIO_LPASS_CORE_PERF7_D_DFSR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_CORE_PERF7_D_DFSR_ADDR(x),m,v,HWIO_LPASS_CORE_PERF7_D_DFSR_IN(x))
#define HWIO_LPASS_CORE_PERF7_D_DFSR_NOT_2D_BMSK                                                                            0xff
#define HWIO_LPASS_CORE_PERF7_D_DFSR_NOT_2D_SHFT                                                                               0

#define HWIO_LPASS_AUDIO_CORE_LPM_CORE_CBCR_ADDR(x)                                                                   ((x) + 0x1e000)
#define HWIO_LPASS_AUDIO_CORE_LPM_CORE_CBCR_RMSK                                                                      0x80007ff3
#define HWIO_LPASS_AUDIO_CORE_LPM_CORE_CBCR_IN(x)            \
                in_dword(HWIO_LPASS_AUDIO_CORE_LPM_CORE_CBCR_ADDR(x))
#define HWIO_LPASS_AUDIO_CORE_LPM_CORE_CBCR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AUDIO_CORE_LPM_CORE_CBCR_ADDR(x), m)
#define HWIO_LPASS_AUDIO_CORE_LPM_CORE_CBCR_OUT(x, v)            \
                out_dword(HWIO_LPASS_AUDIO_CORE_LPM_CORE_CBCR_ADDR(x),v)
#define HWIO_LPASS_AUDIO_CORE_LPM_CORE_CBCR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AUDIO_CORE_LPM_CORE_CBCR_ADDR(x),m,v,HWIO_LPASS_AUDIO_CORE_LPM_CORE_CBCR_IN(x))
#define HWIO_LPASS_AUDIO_CORE_LPM_CORE_CBCR_CLK_OFF_BMSK                                                              0x80000000
#define HWIO_LPASS_AUDIO_CORE_LPM_CORE_CBCR_CLK_OFF_SHFT                                                                      31
#define HWIO_LPASS_AUDIO_CORE_LPM_CORE_CBCR_FORCE_MEM_CORE_ON_BMSK                                                        0x4000
#define HWIO_LPASS_AUDIO_CORE_LPM_CORE_CBCR_FORCE_MEM_CORE_ON_SHFT                                                            14
#define HWIO_LPASS_AUDIO_CORE_LPM_CORE_CBCR_FORCE_MEM_PERIPH_ON_BMSK                                                      0x2000
#define HWIO_LPASS_AUDIO_CORE_LPM_CORE_CBCR_FORCE_MEM_PERIPH_ON_SHFT                                                          13
#define HWIO_LPASS_AUDIO_CORE_LPM_CORE_CBCR_FORCE_MEM_PERIPH_OFF_BMSK                                                     0x1000
#define HWIO_LPASS_AUDIO_CORE_LPM_CORE_CBCR_FORCE_MEM_PERIPH_OFF_SHFT                                                         12
#define HWIO_LPASS_AUDIO_CORE_LPM_CORE_CBCR_WAKEUP_BMSK                                                                    0xf00
#define HWIO_LPASS_AUDIO_CORE_LPM_CORE_CBCR_WAKEUP_SHFT                                                                        8
#define HWIO_LPASS_AUDIO_CORE_LPM_CORE_CBCR_SLEEP_BMSK                                                                      0xf0
#define HWIO_LPASS_AUDIO_CORE_LPM_CORE_CBCR_SLEEP_SHFT                                                                         4
#define HWIO_LPASS_AUDIO_CORE_LPM_CORE_CBCR_HW_CTL_BMSK                                                                      0x2
#define HWIO_LPASS_AUDIO_CORE_LPM_CORE_CBCR_HW_CTL_SHFT                                                                        1
#define HWIO_LPASS_AUDIO_CORE_LPM_CORE_CBCR_CLK_ENABLE_BMSK                                                                  0x1
#define HWIO_LPASS_AUDIO_CORE_LPM_CORE_CBCR_CLK_ENABLE_SHFT                                                                    0

#define HWIO_LPASS_AUDIO_CORE_LPM_MEM0_CORE_CBCR_ADDR(x)                                                              ((x) + 0x1e004)
#define HWIO_LPASS_AUDIO_CORE_LPM_MEM0_CORE_CBCR_RMSK                                                                 0x80007ff3
#define HWIO_LPASS_AUDIO_CORE_LPM_MEM0_CORE_CBCR_IN(x)            \
                in_dword(HWIO_LPASS_AUDIO_CORE_LPM_MEM0_CORE_CBCR_ADDR(x))
#define HWIO_LPASS_AUDIO_CORE_LPM_MEM0_CORE_CBCR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AUDIO_CORE_LPM_MEM0_CORE_CBCR_ADDR(x), m)
#define HWIO_LPASS_AUDIO_CORE_LPM_MEM0_CORE_CBCR_OUT(x, v)            \
                out_dword(HWIO_LPASS_AUDIO_CORE_LPM_MEM0_CORE_CBCR_ADDR(x),v)
#define HWIO_LPASS_AUDIO_CORE_LPM_MEM0_CORE_CBCR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AUDIO_CORE_LPM_MEM0_CORE_CBCR_ADDR(x),m,v,HWIO_LPASS_AUDIO_CORE_LPM_MEM0_CORE_CBCR_IN(x))
#define HWIO_LPASS_AUDIO_CORE_LPM_MEM0_CORE_CBCR_CLK_OFF_BMSK                                                         0x80000000
#define HWIO_LPASS_AUDIO_CORE_LPM_MEM0_CORE_CBCR_CLK_OFF_SHFT                                                                 31
#define HWIO_LPASS_AUDIO_CORE_LPM_MEM0_CORE_CBCR_FORCE_MEM_CORE_ON_BMSK                                                   0x4000
#define HWIO_LPASS_AUDIO_CORE_LPM_MEM0_CORE_CBCR_FORCE_MEM_CORE_ON_SHFT                                                       14
#define HWIO_LPASS_AUDIO_CORE_LPM_MEM0_CORE_CBCR_FORCE_MEM_PERIPH_ON_BMSK                                                 0x2000
#define HWIO_LPASS_AUDIO_CORE_LPM_MEM0_CORE_CBCR_FORCE_MEM_PERIPH_ON_SHFT                                                     13
#define HWIO_LPASS_AUDIO_CORE_LPM_MEM0_CORE_CBCR_FORCE_MEM_PERIPH_OFF_BMSK                                                0x1000
#define HWIO_LPASS_AUDIO_CORE_LPM_MEM0_CORE_CBCR_FORCE_MEM_PERIPH_OFF_SHFT                                                    12
#define HWIO_LPASS_AUDIO_CORE_LPM_MEM0_CORE_CBCR_WAKEUP_BMSK                                                               0xf00
#define HWIO_LPASS_AUDIO_CORE_LPM_MEM0_CORE_CBCR_WAKEUP_SHFT                                                                   8
#define HWIO_LPASS_AUDIO_CORE_LPM_MEM0_CORE_CBCR_SLEEP_BMSK                                                                 0xf0
#define HWIO_LPASS_AUDIO_CORE_LPM_MEM0_CORE_CBCR_SLEEP_SHFT                                                                    4
#define HWIO_LPASS_AUDIO_CORE_LPM_MEM0_CORE_CBCR_HW_CTL_BMSK                                                                 0x2
#define HWIO_LPASS_AUDIO_CORE_LPM_MEM0_CORE_CBCR_HW_CTL_SHFT                                                                   1
#define HWIO_LPASS_AUDIO_CORE_LPM_MEM0_CORE_CBCR_CLK_ENABLE_BMSK                                                             0x1
#define HWIO_LPASS_AUDIO_CORE_LPM_MEM0_CORE_CBCR_CLK_ENABLE_SHFT                                                               0

#define HWIO_LPASS_AUDIO_CORE_LPM_MEM1_CORE_CBCR_ADDR(x)                                                              ((x) + 0x1e008)
#define HWIO_LPASS_AUDIO_CORE_LPM_MEM1_CORE_CBCR_RMSK                                                                 0x80007ff3
#define HWIO_LPASS_AUDIO_CORE_LPM_MEM1_CORE_CBCR_IN(x)            \
                in_dword(HWIO_LPASS_AUDIO_CORE_LPM_MEM1_CORE_CBCR_ADDR(x))
#define HWIO_LPASS_AUDIO_CORE_LPM_MEM1_CORE_CBCR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AUDIO_CORE_LPM_MEM1_CORE_CBCR_ADDR(x), m)
#define HWIO_LPASS_AUDIO_CORE_LPM_MEM1_CORE_CBCR_OUT(x, v)            \
                out_dword(HWIO_LPASS_AUDIO_CORE_LPM_MEM1_CORE_CBCR_ADDR(x),v)
#define HWIO_LPASS_AUDIO_CORE_LPM_MEM1_CORE_CBCR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AUDIO_CORE_LPM_MEM1_CORE_CBCR_ADDR(x),m,v,HWIO_LPASS_AUDIO_CORE_LPM_MEM1_CORE_CBCR_IN(x))
#define HWIO_LPASS_AUDIO_CORE_LPM_MEM1_CORE_CBCR_CLK_OFF_BMSK                                                         0x80000000
#define HWIO_LPASS_AUDIO_CORE_LPM_MEM1_CORE_CBCR_CLK_OFF_SHFT                                                                 31
#define HWIO_LPASS_AUDIO_CORE_LPM_MEM1_CORE_CBCR_FORCE_MEM_CORE_ON_BMSK                                                   0x4000
#define HWIO_LPASS_AUDIO_CORE_LPM_MEM1_CORE_CBCR_FORCE_MEM_CORE_ON_SHFT                                                       14
#define HWIO_LPASS_AUDIO_CORE_LPM_MEM1_CORE_CBCR_FORCE_MEM_PERIPH_ON_BMSK                                                 0x2000
#define HWIO_LPASS_AUDIO_CORE_LPM_MEM1_CORE_CBCR_FORCE_MEM_PERIPH_ON_SHFT                                                     13
#define HWIO_LPASS_AUDIO_CORE_LPM_MEM1_CORE_CBCR_FORCE_MEM_PERIPH_OFF_BMSK                                                0x1000
#define HWIO_LPASS_AUDIO_CORE_LPM_MEM1_CORE_CBCR_FORCE_MEM_PERIPH_OFF_SHFT                                                    12
#define HWIO_LPASS_AUDIO_CORE_LPM_MEM1_CORE_CBCR_WAKEUP_BMSK                                                               0xf00
#define HWIO_LPASS_AUDIO_CORE_LPM_MEM1_CORE_CBCR_WAKEUP_SHFT                                                                   8
#define HWIO_LPASS_AUDIO_CORE_LPM_MEM1_CORE_CBCR_SLEEP_BMSK                                                                 0xf0
#define HWIO_LPASS_AUDIO_CORE_LPM_MEM1_CORE_CBCR_SLEEP_SHFT                                                                    4
#define HWIO_LPASS_AUDIO_CORE_LPM_MEM1_CORE_CBCR_HW_CTL_BMSK                                                                 0x2
#define HWIO_LPASS_AUDIO_CORE_LPM_MEM1_CORE_CBCR_HW_CTL_SHFT                                                                   1
#define HWIO_LPASS_AUDIO_CORE_LPM_MEM1_CORE_CBCR_CLK_ENABLE_BMSK                                                             0x1
#define HWIO_LPASS_AUDIO_CORE_LPM_MEM1_CORE_CBCR_CLK_ENABLE_SHFT                                                               0

#define HWIO_LPASS_AUDIO_CORE_LPM_MEM2_CORE_CBCR_ADDR(x)                                                              ((x) + 0x1e00c)
#define HWIO_LPASS_AUDIO_CORE_LPM_MEM2_CORE_CBCR_RMSK                                                                 0x80007ff3
#define HWIO_LPASS_AUDIO_CORE_LPM_MEM2_CORE_CBCR_IN(x)            \
                in_dword(HWIO_LPASS_AUDIO_CORE_LPM_MEM2_CORE_CBCR_ADDR(x))
#define HWIO_LPASS_AUDIO_CORE_LPM_MEM2_CORE_CBCR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AUDIO_CORE_LPM_MEM2_CORE_CBCR_ADDR(x), m)
#define HWIO_LPASS_AUDIO_CORE_LPM_MEM2_CORE_CBCR_OUT(x, v)            \
                out_dword(HWIO_LPASS_AUDIO_CORE_LPM_MEM2_CORE_CBCR_ADDR(x),v)
#define HWIO_LPASS_AUDIO_CORE_LPM_MEM2_CORE_CBCR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AUDIO_CORE_LPM_MEM2_CORE_CBCR_ADDR(x),m,v,HWIO_LPASS_AUDIO_CORE_LPM_MEM2_CORE_CBCR_IN(x))
#define HWIO_LPASS_AUDIO_CORE_LPM_MEM2_CORE_CBCR_CLK_OFF_BMSK                                                         0x80000000
#define HWIO_LPASS_AUDIO_CORE_LPM_MEM2_CORE_CBCR_CLK_OFF_SHFT                                                                 31
#define HWIO_LPASS_AUDIO_CORE_LPM_MEM2_CORE_CBCR_FORCE_MEM_CORE_ON_BMSK                                                   0x4000
#define HWIO_LPASS_AUDIO_CORE_LPM_MEM2_CORE_CBCR_FORCE_MEM_CORE_ON_SHFT                                                       14
#define HWIO_LPASS_AUDIO_CORE_LPM_MEM2_CORE_CBCR_FORCE_MEM_PERIPH_ON_BMSK                                                 0x2000
#define HWIO_LPASS_AUDIO_CORE_LPM_MEM2_CORE_CBCR_FORCE_MEM_PERIPH_ON_SHFT                                                     13
#define HWIO_LPASS_AUDIO_CORE_LPM_MEM2_CORE_CBCR_FORCE_MEM_PERIPH_OFF_BMSK                                                0x1000
#define HWIO_LPASS_AUDIO_CORE_LPM_MEM2_CORE_CBCR_FORCE_MEM_PERIPH_OFF_SHFT                                                    12
#define HWIO_LPASS_AUDIO_CORE_LPM_MEM2_CORE_CBCR_WAKEUP_BMSK                                                               0xf00
#define HWIO_LPASS_AUDIO_CORE_LPM_MEM2_CORE_CBCR_WAKEUP_SHFT                                                                   8
#define HWIO_LPASS_AUDIO_CORE_LPM_MEM2_CORE_CBCR_SLEEP_BMSK                                                                 0xf0
#define HWIO_LPASS_AUDIO_CORE_LPM_MEM2_CORE_CBCR_SLEEP_SHFT                                                                    4
#define HWIO_LPASS_AUDIO_CORE_LPM_MEM2_CORE_CBCR_HW_CTL_BMSK                                                                 0x2
#define HWIO_LPASS_AUDIO_CORE_LPM_MEM2_CORE_CBCR_HW_CTL_SHFT                                                                   1
#define HWIO_LPASS_AUDIO_CORE_LPM_MEM2_CORE_CBCR_CLK_ENABLE_BMSK                                                             0x1
#define HWIO_LPASS_AUDIO_CORE_LPM_MEM2_CORE_CBCR_CLK_ENABLE_SHFT                                                               0

#define HWIO_LPASS_AUDIO_CORE_LPM_MEM3_CORE_CBCR_ADDR(x)                                                              ((x) + 0x1e010)
#define HWIO_LPASS_AUDIO_CORE_LPM_MEM3_CORE_CBCR_RMSK                                                                 0x80007ff3
#define HWIO_LPASS_AUDIO_CORE_LPM_MEM3_CORE_CBCR_IN(x)            \
                in_dword(HWIO_LPASS_AUDIO_CORE_LPM_MEM3_CORE_CBCR_ADDR(x))
#define HWIO_LPASS_AUDIO_CORE_LPM_MEM3_CORE_CBCR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AUDIO_CORE_LPM_MEM3_CORE_CBCR_ADDR(x), m)
#define HWIO_LPASS_AUDIO_CORE_LPM_MEM3_CORE_CBCR_OUT(x, v)            \
                out_dword(HWIO_LPASS_AUDIO_CORE_LPM_MEM3_CORE_CBCR_ADDR(x),v)
#define HWIO_LPASS_AUDIO_CORE_LPM_MEM3_CORE_CBCR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AUDIO_CORE_LPM_MEM3_CORE_CBCR_ADDR(x),m,v,HWIO_LPASS_AUDIO_CORE_LPM_MEM3_CORE_CBCR_IN(x))
#define HWIO_LPASS_AUDIO_CORE_LPM_MEM3_CORE_CBCR_CLK_OFF_BMSK                                                         0x80000000
#define HWIO_LPASS_AUDIO_CORE_LPM_MEM3_CORE_CBCR_CLK_OFF_SHFT                                                                 31
#define HWIO_LPASS_AUDIO_CORE_LPM_MEM3_CORE_CBCR_FORCE_MEM_CORE_ON_BMSK                                                   0x4000
#define HWIO_LPASS_AUDIO_CORE_LPM_MEM3_CORE_CBCR_FORCE_MEM_CORE_ON_SHFT                                                       14
#define HWIO_LPASS_AUDIO_CORE_LPM_MEM3_CORE_CBCR_FORCE_MEM_PERIPH_ON_BMSK                                                 0x2000
#define HWIO_LPASS_AUDIO_CORE_LPM_MEM3_CORE_CBCR_FORCE_MEM_PERIPH_ON_SHFT                                                     13
#define HWIO_LPASS_AUDIO_CORE_LPM_MEM3_CORE_CBCR_FORCE_MEM_PERIPH_OFF_BMSK                                                0x1000
#define HWIO_LPASS_AUDIO_CORE_LPM_MEM3_CORE_CBCR_FORCE_MEM_PERIPH_OFF_SHFT                                                    12
#define HWIO_LPASS_AUDIO_CORE_LPM_MEM3_CORE_CBCR_WAKEUP_BMSK                                                               0xf00
#define HWIO_LPASS_AUDIO_CORE_LPM_MEM3_CORE_CBCR_WAKEUP_SHFT                                                                   8
#define HWIO_LPASS_AUDIO_CORE_LPM_MEM3_CORE_CBCR_SLEEP_BMSK                                                                 0xf0
#define HWIO_LPASS_AUDIO_CORE_LPM_MEM3_CORE_CBCR_SLEEP_SHFT                                                                    4
#define HWIO_LPASS_AUDIO_CORE_LPM_MEM3_CORE_CBCR_HW_CTL_BMSK                                                                 0x2
#define HWIO_LPASS_AUDIO_CORE_LPM_MEM3_CORE_CBCR_HW_CTL_SHFT                                                                   1
#define HWIO_LPASS_AUDIO_CORE_LPM_MEM3_CORE_CBCR_CLK_ENABLE_BMSK                                                             0x1
#define HWIO_LPASS_AUDIO_CORE_LPM_MEM3_CORE_CBCR_CLK_ENABLE_SHFT                                                               0

#define HWIO_LPASS_AUDIO_CORE_LPM_MEM4_CORE_CBCR_ADDR(x)                                                              ((x) + 0x1e014)
#define HWIO_LPASS_AUDIO_CORE_LPM_MEM4_CORE_CBCR_RMSK                                                                 0x80007ff3
#define HWIO_LPASS_AUDIO_CORE_LPM_MEM4_CORE_CBCR_IN(x)            \
                in_dword(HWIO_LPASS_AUDIO_CORE_LPM_MEM4_CORE_CBCR_ADDR(x))
#define HWIO_LPASS_AUDIO_CORE_LPM_MEM4_CORE_CBCR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AUDIO_CORE_LPM_MEM4_CORE_CBCR_ADDR(x), m)
#define HWIO_LPASS_AUDIO_CORE_LPM_MEM4_CORE_CBCR_OUT(x, v)            \
                out_dword(HWIO_LPASS_AUDIO_CORE_LPM_MEM4_CORE_CBCR_ADDR(x),v)
#define HWIO_LPASS_AUDIO_CORE_LPM_MEM4_CORE_CBCR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AUDIO_CORE_LPM_MEM4_CORE_CBCR_ADDR(x),m,v,HWIO_LPASS_AUDIO_CORE_LPM_MEM4_CORE_CBCR_IN(x))
#define HWIO_LPASS_AUDIO_CORE_LPM_MEM4_CORE_CBCR_CLK_OFF_BMSK                                                         0x80000000
#define HWIO_LPASS_AUDIO_CORE_LPM_MEM4_CORE_CBCR_CLK_OFF_SHFT                                                                 31
#define HWIO_LPASS_AUDIO_CORE_LPM_MEM4_CORE_CBCR_FORCE_MEM_CORE_ON_BMSK                                                   0x4000
#define HWIO_LPASS_AUDIO_CORE_LPM_MEM4_CORE_CBCR_FORCE_MEM_CORE_ON_SHFT                                                       14
#define HWIO_LPASS_AUDIO_CORE_LPM_MEM4_CORE_CBCR_FORCE_MEM_PERIPH_ON_BMSK                                                 0x2000
#define HWIO_LPASS_AUDIO_CORE_LPM_MEM4_CORE_CBCR_FORCE_MEM_PERIPH_ON_SHFT                                                     13
#define HWIO_LPASS_AUDIO_CORE_LPM_MEM4_CORE_CBCR_FORCE_MEM_PERIPH_OFF_BMSK                                                0x1000
#define HWIO_LPASS_AUDIO_CORE_LPM_MEM4_CORE_CBCR_FORCE_MEM_PERIPH_OFF_SHFT                                                    12
#define HWIO_LPASS_AUDIO_CORE_LPM_MEM4_CORE_CBCR_WAKEUP_BMSK                                                               0xf00
#define HWIO_LPASS_AUDIO_CORE_LPM_MEM4_CORE_CBCR_WAKEUP_SHFT                                                                   8
#define HWIO_LPASS_AUDIO_CORE_LPM_MEM4_CORE_CBCR_SLEEP_BMSK                                                                 0xf0
#define HWIO_LPASS_AUDIO_CORE_LPM_MEM4_CORE_CBCR_SLEEP_SHFT                                                                    4
#define HWIO_LPASS_AUDIO_CORE_LPM_MEM4_CORE_CBCR_HW_CTL_BMSK                                                                 0x2
#define HWIO_LPASS_AUDIO_CORE_LPM_MEM4_CORE_CBCR_HW_CTL_SHFT                                                                   1
#define HWIO_LPASS_AUDIO_CORE_LPM_MEM4_CORE_CBCR_CLK_ENABLE_BMSK                                                             0x1
#define HWIO_LPASS_AUDIO_CORE_LPM_MEM4_CORE_CBCR_CLK_ENABLE_SHFT                                                               0

#define HWIO_LPASS_AUDIO_CORE_CORE_CBCR_ADDR(x)                                                                       ((x) + 0x1f000)
#define HWIO_LPASS_AUDIO_CORE_CORE_CBCR_RMSK                                                                          0x80000003
#define HWIO_LPASS_AUDIO_CORE_CORE_CBCR_IN(x)            \
                in_dword(HWIO_LPASS_AUDIO_CORE_CORE_CBCR_ADDR(x))
#define HWIO_LPASS_AUDIO_CORE_CORE_CBCR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AUDIO_CORE_CORE_CBCR_ADDR(x), m)
#define HWIO_LPASS_AUDIO_CORE_CORE_CBCR_OUT(x, v)            \
                out_dword(HWIO_LPASS_AUDIO_CORE_CORE_CBCR_ADDR(x),v)
#define HWIO_LPASS_AUDIO_CORE_CORE_CBCR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AUDIO_CORE_CORE_CBCR_ADDR(x),m,v,HWIO_LPASS_AUDIO_CORE_CORE_CBCR_IN(x))
#define HWIO_LPASS_AUDIO_CORE_CORE_CBCR_CLK_OFF_BMSK                                                                  0x80000000
#define HWIO_LPASS_AUDIO_CORE_CORE_CBCR_CLK_OFF_SHFT                                                                          31
#define HWIO_LPASS_AUDIO_CORE_CORE_CBCR_HW_CTL_BMSK                                                                          0x2
#define HWIO_LPASS_AUDIO_CORE_CORE_CBCR_HW_CTL_SHFT                                                                            1
#define HWIO_LPASS_AUDIO_CORE_CORE_CBCR_CLK_ENABLE_BMSK                                                                      0x1
#define HWIO_LPASS_AUDIO_CORE_CORE_CBCR_CLK_ENABLE_SHFT                                                                        0

#define HWIO_LPASS_EXT_MCLK0_CMD_RCGR_ADDR(x)                                                                         ((x) + 0x20000)
#define HWIO_LPASS_EXT_MCLK0_CMD_RCGR_RMSK                                                                            0x800000f3
#define HWIO_LPASS_EXT_MCLK0_CMD_RCGR_IN(x)            \
                in_dword(HWIO_LPASS_EXT_MCLK0_CMD_RCGR_ADDR(x))
#define HWIO_LPASS_EXT_MCLK0_CMD_RCGR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_EXT_MCLK0_CMD_RCGR_ADDR(x), m)
#define HWIO_LPASS_EXT_MCLK0_CMD_RCGR_OUT(x, v)            \
                out_dword(HWIO_LPASS_EXT_MCLK0_CMD_RCGR_ADDR(x),v)
#define HWIO_LPASS_EXT_MCLK0_CMD_RCGR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_EXT_MCLK0_CMD_RCGR_ADDR(x),m,v,HWIO_LPASS_EXT_MCLK0_CMD_RCGR_IN(x))
#define HWIO_LPASS_EXT_MCLK0_CMD_RCGR_ROOT_OFF_BMSK                                                                   0x80000000
#define HWIO_LPASS_EXT_MCLK0_CMD_RCGR_ROOT_OFF_SHFT                                                                           31
#define HWIO_LPASS_EXT_MCLK0_CMD_RCGR_DIRTY_D_BMSK                                                                          0x80
#define HWIO_LPASS_EXT_MCLK0_CMD_RCGR_DIRTY_D_SHFT                                                                             7
#define HWIO_LPASS_EXT_MCLK0_CMD_RCGR_DIRTY_N_BMSK                                                                          0x40
#define HWIO_LPASS_EXT_MCLK0_CMD_RCGR_DIRTY_N_SHFT                                                                             6
#define HWIO_LPASS_EXT_MCLK0_CMD_RCGR_DIRTY_M_BMSK                                                                          0x20
#define HWIO_LPASS_EXT_MCLK0_CMD_RCGR_DIRTY_M_SHFT                                                                             5
#define HWIO_LPASS_EXT_MCLK0_CMD_RCGR_DIRTY_CFG_RCGR_BMSK                                                                   0x10
#define HWIO_LPASS_EXT_MCLK0_CMD_RCGR_DIRTY_CFG_RCGR_SHFT                                                                      4
#define HWIO_LPASS_EXT_MCLK0_CMD_RCGR_ROOT_EN_BMSK                                                                           0x2
#define HWIO_LPASS_EXT_MCLK0_CMD_RCGR_ROOT_EN_SHFT                                                                             1
#define HWIO_LPASS_EXT_MCLK0_CMD_RCGR_UPDATE_BMSK                                                                            0x1
#define HWIO_LPASS_EXT_MCLK0_CMD_RCGR_UPDATE_SHFT                                                                              0

#define HWIO_LPASS_EXT_MCLK0_CFG_RCGR_ADDR(x)                                                                         ((x) + 0x20004)
#define HWIO_LPASS_EXT_MCLK0_CFG_RCGR_RMSK                                                                                0x771f
#define HWIO_LPASS_EXT_MCLK0_CFG_RCGR_IN(x)            \
                in_dword(HWIO_LPASS_EXT_MCLK0_CFG_RCGR_ADDR(x))
#define HWIO_LPASS_EXT_MCLK0_CFG_RCGR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_EXT_MCLK0_CFG_RCGR_ADDR(x), m)
#define HWIO_LPASS_EXT_MCLK0_CFG_RCGR_OUT(x, v)            \
                out_dword(HWIO_LPASS_EXT_MCLK0_CFG_RCGR_ADDR(x),v)
#define HWIO_LPASS_EXT_MCLK0_CFG_RCGR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_EXT_MCLK0_CFG_RCGR_ADDR(x),m,v,HWIO_LPASS_EXT_MCLK0_CFG_RCGR_IN(x))
#define HWIO_LPASS_EXT_MCLK0_CFG_RCGR_ALT_SRC_SEL_BMSK                                                                    0x4000
#define HWIO_LPASS_EXT_MCLK0_CFG_RCGR_ALT_SRC_SEL_SHFT                                                                        14
#define HWIO_LPASS_EXT_MCLK0_CFG_RCGR_MODE_BMSK                                                                           0x3000
#define HWIO_LPASS_EXT_MCLK0_CFG_RCGR_MODE_SHFT                                                                               12
#define HWIO_LPASS_EXT_MCLK0_CFG_RCGR_SRC_SEL_BMSK                                                                         0x700
#define HWIO_LPASS_EXT_MCLK0_CFG_RCGR_SRC_SEL_SHFT                                                                             8
#define HWIO_LPASS_EXT_MCLK0_CFG_RCGR_SRC_DIV_BMSK                                                                          0x1f
#define HWIO_LPASS_EXT_MCLK0_CFG_RCGR_SRC_DIV_SHFT                                                                             0

#define HWIO_LPASS_EXT_MCLK0_M_ADDR(x)                                                                                ((x) + 0x20008)
#define HWIO_LPASS_EXT_MCLK0_M_RMSK                                                                                         0xff
#define HWIO_LPASS_EXT_MCLK0_M_IN(x)            \
                in_dword(HWIO_LPASS_EXT_MCLK0_M_ADDR(x))
#define HWIO_LPASS_EXT_MCLK0_M_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_EXT_MCLK0_M_ADDR(x), m)
#define HWIO_LPASS_EXT_MCLK0_M_OUT(x, v)            \
                out_dword(HWIO_LPASS_EXT_MCLK0_M_ADDR(x),v)
#define HWIO_LPASS_EXT_MCLK0_M_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_EXT_MCLK0_M_ADDR(x),m,v,HWIO_LPASS_EXT_MCLK0_M_IN(x))
#define HWIO_LPASS_EXT_MCLK0_M_M_BMSK                                                                                       0xff
#define HWIO_LPASS_EXT_MCLK0_M_M_SHFT                                                                                          0

#define HWIO_LPASS_EXT_MCLK0_N_ADDR(x)                                                                                ((x) + 0x2000c)
#define HWIO_LPASS_EXT_MCLK0_N_RMSK                                                                                         0xff
#define HWIO_LPASS_EXT_MCLK0_N_IN(x)            \
                in_dword(HWIO_LPASS_EXT_MCLK0_N_ADDR(x))
#define HWIO_LPASS_EXT_MCLK0_N_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_EXT_MCLK0_N_ADDR(x), m)
#define HWIO_LPASS_EXT_MCLK0_N_OUT(x, v)            \
                out_dword(HWIO_LPASS_EXT_MCLK0_N_ADDR(x),v)
#define HWIO_LPASS_EXT_MCLK0_N_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_EXT_MCLK0_N_ADDR(x),m,v,HWIO_LPASS_EXT_MCLK0_N_IN(x))
#define HWIO_LPASS_EXT_MCLK0_N_NOT_N_MINUS_M_BMSK                                                                           0xff
#define HWIO_LPASS_EXT_MCLK0_N_NOT_N_MINUS_M_SHFT                                                                              0

#define HWIO_LPASS_EXT_MCLK0_D_ADDR(x)                                                                                ((x) + 0x20010)
#define HWIO_LPASS_EXT_MCLK0_D_RMSK                                                                                         0xff
#define HWIO_LPASS_EXT_MCLK0_D_IN(x)            \
                in_dword(HWIO_LPASS_EXT_MCLK0_D_ADDR(x))
#define HWIO_LPASS_EXT_MCLK0_D_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_EXT_MCLK0_D_ADDR(x), m)
#define HWIO_LPASS_EXT_MCLK0_D_OUT(x, v)            \
                out_dword(HWIO_LPASS_EXT_MCLK0_D_ADDR(x),v)
#define HWIO_LPASS_EXT_MCLK0_D_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_EXT_MCLK0_D_ADDR(x),m,v,HWIO_LPASS_EXT_MCLK0_D_IN(x))
#define HWIO_LPASS_EXT_MCLK0_D_NOT_2D_BMSK                                                                                  0xff
#define HWIO_LPASS_EXT_MCLK0_D_NOT_2D_SHFT                                                                                     0

#define HWIO_LPASS_AUDIO_WRAPPER_EXT_MCLK0_CBCR_ADDR(x)                                                               ((x) + 0x20014)
#define HWIO_LPASS_AUDIO_WRAPPER_EXT_MCLK0_CBCR_RMSK                                                                  0x80000003
#define HWIO_LPASS_AUDIO_WRAPPER_EXT_MCLK0_CBCR_IN(x)            \
                in_dword(HWIO_LPASS_AUDIO_WRAPPER_EXT_MCLK0_CBCR_ADDR(x))
#define HWIO_LPASS_AUDIO_WRAPPER_EXT_MCLK0_CBCR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AUDIO_WRAPPER_EXT_MCLK0_CBCR_ADDR(x), m)
#define HWIO_LPASS_AUDIO_WRAPPER_EXT_MCLK0_CBCR_OUT(x, v)            \
                out_dword(HWIO_LPASS_AUDIO_WRAPPER_EXT_MCLK0_CBCR_ADDR(x),v)
#define HWIO_LPASS_AUDIO_WRAPPER_EXT_MCLK0_CBCR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AUDIO_WRAPPER_EXT_MCLK0_CBCR_ADDR(x),m,v,HWIO_LPASS_AUDIO_WRAPPER_EXT_MCLK0_CBCR_IN(x))
#define HWIO_LPASS_AUDIO_WRAPPER_EXT_MCLK0_CBCR_CLK_OFF_BMSK                                                          0x80000000
#define HWIO_LPASS_AUDIO_WRAPPER_EXT_MCLK0_CBCR_CLK_OFF_SHFT                                                                  31
#define HWIO_LPASS_AUDIO_WRAPPER_EXT_MCLK0_CBCR_HW_CTL_BMSK                                                                  0x2
#define HWIO_LPASS_AUDIO_WRAPPER_EXT_MCLK0_CBCR_HW_CTL_SHFT                                                                    1
#define HWIO_LPASS_AUDIO_WRAPPER_EXT_MCLK0_CBCR_CLK_ENABLE_BMSK                                                              0x1
#define HWIO_LPASS_AUDIO_WRAPPER_EXT_MCLK0_CBCR_CLK_ENABLE_SHFT                                                                0

#define HWIO_LPASS_EXT_MCLK1_CMD_RCGR_ADDR(x)                                                                         ((x) + 0x21000)
#define HWIO_LPASS_EXT_MCLK1_CMD_RCGR_RMSK                                                                            0x800000f3
#define HWIO_LPASS_EXT_MCLK1_CMD_RCGR_IN(x)            \
                in_dword(HWIO_LPASS_EXT_MCLK1_CMD_RCGR_ADDR(x))
#define HWIO_LPASS_EXT_MCLK1_CMD_RCGR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_EXT_MCLK1_CMD_RCGR_ADDR(x), m)
#define HWIO_LPASS_EXT_MCLK1_CMD_RCGR_OUT(x, v)            \
                out_dword(HWIO_LPASS_EXT_MCLK1_CMD_RCGR_ADDR(x),v)
#define HWIO_LPASS_EXT_MCLK1_CMD_RCGR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_EXT_MCLK1_CMD_RCGR_ADDR(x),m,v,HWIO_LPASS_EXT_MCLK1_CMD_RCGR_IN(x))
#define HWIO_LPASS_EXT_MCLK1_CMD_RCGR_ROOT_OFF_BMSK                                                                   0x80000000
#define HWIO_LPASS_EXT_MCLK1_CMD_RCGR_ROOT_OFF_SHFT                                                                           31
#define HWIO_LPASS_EXT_MCLK1_CMD_RCGR_DIRTY_D_BMSK                                                                          0x80
#define HWIO_LPASS_EXT_MCLK1_CMD_RCGR_DIRTY_D_SHFT                                                                             7
#define HWIO_LPASS_EXT_MCLK1_CMD_RCGR_DIRTY_N_BMSK                                                                          0x40
#define HWIO_LPASS_EXT_MCLK1_CMD_RCGR_DIRTY_N_SHFT                                                                             6
#define HWIO_LPASS_EXT_MCLK1_CMD_RCGR_DIRTY_M_BMSK                                                                          0x20
#define HWIO_LPASS_EXT_MCLK1_CMD_RCGR_DIRTY_M_SHFT                                                                             5
#define HWIO_LPASS_EXT_MCLK1_CMD_RCGR_DIRTY_CFG_RCGR_BMSK                                                                   0x10
#define HWIO_LPASS_EXT_MCLK1_CMD_RCGR_DIRTY_CFG_RCGR_SHFT                                                                      4
#define HWIO_LPASS_EXT_MCLK1_CMD_RCGR_ROOT_EN_BMSK                                                                           0x2
#define HWIO_LPASS_EXT_MCLK1_CMD_RCGR_ROOT_EN_SHFT                                                                             1
#define HWIO_LPASS_EXT_MCLK1_CMD_RCGR_UPDATE_BMSK                                                                            0x1
#define HWIO_LPASS_EXT_MCLK1_CMD_RCGR_UPDATE_SHFT                                                                              0

#define HWIO_LPASS_EXT_MCLK1_CFG_RCGR_ADDR(x)                                                                         ((x) + 0x21004)
#define HWIO_LPASS_EXT_MCLK1_CFG_RCGR_RMSK                                                                                0x771f
#define HWIO_LPASS_EXT_MCLK1_CFG_RCGR_IN(x)            \
                in_dword(HWIO_LPASS_EXT_MCLK1_CFG_RCGR_ADDR(x))
#define HWIO_LPASS_EXT_MCLK1_CFG_RCGR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_EXT_MCLK1_CFG_RCGR_ADDR(x), m)
#define HWIO_LPASS_EXT_MCLK1_CFG_RCGR_OUT(x, v)            \
                out_dword(HWIO_LPASS_EXT_MCLK1_CFG_RCGR_ADDR(x),v)
#define HWIO_LPASS_EXT_MCLK1_CFG_RCGR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_EXT_MCLK1_CFG_RCGR_ADDR(x),m,v,HWIO_LPASS_EXT_MCLK1_CFG_RCGR_IN(x))
#define HWIO_LPASS_EXT_MCLK1_CFG_RCGR_ALT_SRC_SEL_BMSK                                                                    0x4000
#define HWIO_LPASS_EXT_MCLK1_CFG_RCGR_ALT_SRC_SEL_SHFT                                                                        14
#define HWIO_LPASS_EXT_MCLK1_CFG_RCGR_MODE_BMSK                                                                           0x3000
#define HWIO_LPASS_EXT_MCLK1_CFG_RCGR_MODE_SHFT                                                                               12
#define HWIO_LPASS_EXT_MCLK1_CFG_RCGR_SRC_SEL_BMSK                                                                         0x700
#define HWIO_LPASS_EXT_MCLK1_CFG_RCGR_SRC_SEL_SHFT                                                                             8
#define HWIO_LPASS_EXT_MCLK1_CFG_RCGR_SRC_DIV_BMSK                                                                          0x1f
#define HWIO_LPASS_EXT_MCLK1_CFG_RCGR_SRC_DIV_SHFT                                                                             0

#define HWIO_LPASS_EXT_MCLK1_M_ADDR(x)                                                                                ((x) + 0x21008)
#define HWIO_LPASS_EXT_MCLK1_M_RMSK                                                                                         0xff
#define HWIO_LPASS_EXT_MCLK1_M_IN(x)            \
                in_dword(HWIO_LPASS_EXT_MCLK1_M_ADDR(x))
#define HWIO_LPASS_EXT_MCLK1_M_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_EXT_MCLK1_M_ADDR(x), m)
#define HWIO_LPASS_EXT_MCLK1_M_OUT(x, v)            \
                out_dword(HWIO_LPASS_EXT_MCLK1_M_ADDR(x),v)
#define HWIO_LPASS_EXT_MCLK1_M_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_EXT_MCLK1_M_ADDR(x),m,v,HWIO_LPASS_EXT_MCLK1_M_IN(x))
#define HWIO_LPASS_EXT_MCLK1_M_M_BMSK                                                                                       0xff
#define HWIO_LPASS_EXT_MCLK1_M_M_SHFT                                                                                          0

#define HWIO_LPASS_EXT_MCLK1_N_ADDR(x)                                                                                ((x) + 0x2100c)
#define HWIO_LPASS_EXT_MCLK1_N_RMSK                                                                                         0xff
#define HWIO_LPASS_EXT_MCLK1_N_IN(x)            \
                in_dword(HWIO_LPASS_EXT_MCLK1_N_ADDR(x))
#define HWIO_LPASS_EXT_MCLK1_N_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_EXT_MCLK1_N_ADDR(x), m)
#define HWIO_LPASS_EXT_MCLK1_N_OUT(x, v)            \
                out_dword(HWIO_LPASS_EXT_MCLK1_N_ADDR(x),v)
#define HWIO_LPASS_EXT_MCLK1_N_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_EXT_MCLK1_N_ADDR(x),m,v,HWIO_LPASS_EXT_MCLK1_N_IN(x))
#define HWIO_LPASS_EXT_MCLK1_N_NOT_N_MINUS_M_BMSK                                                                           0xff
#define HWIO_LPASS_EXT_MCLK1_N_NOT_N_MINUS_M_SHFT                                                                              0

#define HWIO_LPASS_EXT_MCLK1_D_ADDR(x)                                                                                ((x) + 0x21010)
#define HWIO_LPASS_EXT_MCLK1_D_RMSK                                                                                         0xff
#define HWIO_LPASS_EXT_MCLK1_D_IN(x)            \
                in_dword(HWIO_LPASS_EXT_MCLK1_D_ADDR(x))
#define HWIO_LPASS_EXT_MCLK1_D_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_EXT_MCLK1_D_ADDR(x), m)
#define HWIO_LPASS_EXT_MCLK1_D_OUT(x, v)            \
                out_dword(HWIO_LPASS_EXT_MCLK1_D_ADDR(x),v)
#define HWIO_LPASS_EXT_MCLK1_D_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_EXT_MCLK1_D_ADDR(x),m,v,HWIO_LPASS_EXT_MCLK1_D_IN(x))
#define HWIO_LPASS_EXT_MCLK1_D_NOT_2D_BMSK                                                                                  0xff
#define HWIO_LPASS_EXT_MCLK1_D_NOT_2D_SHFT                                                                                     0

#define HWIO_LPASS_AUDIO_WRAPPER_EXT_MCLK1_CBCR_ADDR(x)                                                               ((x) + 0x21014)
#define HWIO_LPASS_AUDIO_WRAPPER_EXT_MCLK1_CBCR_RMSK                                                                  0x80000003
#define HWIO_LPASS_AUDIO_WRAPPER_EXT_MCLK1_CBCR_IN(x)            \
                in_dword(HWIO_LPASS_AUDIO_WRAPPER_EXT_MCLK1_CBCR_ADDR(x))
#define HWIO_LPASS_AUDIO_WRAPPER_EXT_MCLK1_CBCR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AUDIO_WRAPPER_EXT_MCLK1_CBCR_ADDR(x), m)
#define HWIO_LPASS_AUDIO_WRAPPER_EXT_MCLK1_CBCR_OUT(x, v)            \
                out_dword(HWIO_LPASS_AUDIO_WRAPPER_EXT_MCLK1_CBCR_ADDR(x),v)
#define HWIO_LPASS_AUDIO_WRAPPER_EXT_MCLK1_CBCR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AUDIO_WRAPPER_EXT_MCLK1_CBCR_ADDR(x),m,v,HWIO_LPASS_AUDIO_WRAPPER_EXT_MCLK1_CBCR_IN(x))
#define HWIO_LPASS_AUDIO_WRAPPER_EXT_MCLK1_CBCR_CLK_OFF_BMSK                                                          0x80000000
#define HWIO_LPASS_AUDIO_WRAPPER_EXT_MCLK1_CBCR_CLK_OFF_SHFT                                                                  31
#define HWIO_LPASS_AUDIO_WRAPPER_EXT_MCLK1_CBCR_HW_CTL_BMSK                                                                  0x2
#define HWIO_LPASS_AUDIO_WRAPPER_EXT_MCLK1_CBCR_HW_CTL_SHFT                                                                    1
#define HWIO_LPASS_AUDIO_WRAPPER_EXT_MCLK1_CBCR_CLK_ENABLE_BMSK                                                              0x1
#define HWIO_LPASS_AUDIO_WRAPPER_EXT_MCLK1_CBCR_CLK_ENABLE_SHFT                                                                0

#define HWIO_LPASS_EXT_MCLK2_CMD_RCGR_ADDR(x)                                                                         ((x) + 0x22000)
#define HWIO_LPASS_EXT_MCLK2_CMD_RCGR_RMSK                                                                            0x800000f3
#define HWIO_LPASS_EXT_MCLK2_CMD_RCGR_IN(x)            \
                in_dword(HWIO_LPASS_EXT_MCLK2_CMD_RCGR_ADDR(x))
#define HWIO_LPASS_EXT_MCLK2_CMD_RCGR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_EXT_MCLK2_CMD_RCGR_ADDR(x), m)
#define HWIO_LPASS_EXT_MCLK2_CMD_RCGR_OUT(x, v)            \
                out_dword(HWIO_LPASS_EXT_MCLK2_CMD_RCGR_ADDR(x),v)
#define HWIO_LPASS_EXT_MCLK2_CMD_RCGR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_EXT_MCLK2_CMD_RCGR_ADDR(x),m,v,HWIO_LPASS_EXT_MCLK2_CMD_RCGR_IN(x))
#define HWIO_LPASS_EXT_MCLK2_CMD_RCGR_ROOT_OFF_BMSK                                                                   0x80000000
#define HWIO_LPASS_EXT_MCLK2_CMD_RCGR_ROOT_OFF_SHFT                                                                           31
#define HWIO_LPASS_EXT_MCLK2_CMD_RCGR_DIRTY_D_BMSK                                                                          0x80
#define HWIO_LPASS_EXT_MCLK2_CMD_RCGR_DIRTY_D_SHFT                                                                             7
#define HWIO_LPASS_EXT_MCLK2_CMD_RCGR_DIRTY_N_BMSK                                                                          0x40
#define HWIO_LPASS_EXT_MCLK2_CMD_RCGR_DIRTY_N_SHFT                                                                             6
#define HWIO_LPASS_EXT_MCLK2_CMD_RCGR_DIRTY_M_BMSK                                                                          0x20
#define HWIO_LPASS_EXT_MCLK2_CMD_RCGR_DIRTY_M_SHFT                                                                             5
#define HWIO_LPASS_EXT_MCLK2_CMD_RCGR_DIRTY_CFG_RCGR_BMSK                                                                   0x10
#define HWIO_LPASS_EXT_MCLK2_CMD_RCGR_DIRTY_CFG_RCGR_SHFT                                                                      4
#define HWIO_LPASS_EXT_MCLK2_CMD_RCGR_ROOT_EN_BMSK                                                                           0x2
#define HWIO_LPASS_EXT_MCLK2_CMD_RCGR_ROOT_EN_SHFT                                                                             1
#define HWIO_LPASS_EXT_MCLK2_CMD_RCGR_UPDATE_BMSK                                                                            0x1
#define HWIO_LPASS_EXT_MCLK2_CMD_RCGR_UPDATE_SHFT                                                                              0

#define HWIO_LPASS_EXT_MCLK2_CFG_RCGR_ADDR(x)                                                                         ((x) + 0x22004)
#define HWIO_LPASS_EXT_MCLK2_CFG_RCGR_RMSK                                                                                0x771f
#define HWIO_LPASS_EXT_MCLK2_CFG_RCGR_IN(x)            \
                in_dword(HWIO_LPASS_EXT_MCLK2_CFG_RCGR_ADDR(x))
#define HWIO_LPASS_EXT_MCLK2_CFG_RCGR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_EXT_MCLK2_CFG_RCGR_ADDR(x), m)
#define HWIO_LPASS_EXT_MCLK2_CFG_RCGR_OUT(x, v)            \
                out_dword(HWIO_LPASS_EXT_MCLK2_CFG_RCGR_ADDR(x),v)
#define HWIO_LPASS_EXT_MCLK2_CFG_RCGR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_EXT_MCLK2_CFG_RCGR_ADDR(x),m,v,HWIO_LPASS_EXT_MCLK2_CFG_RCGR_IN(x))
#define HWIO_LPASS_EXT_MCLK2_CFG_RCGR_ALT_SRC_SEL_BMSK                                                                    0x4000
#define HWIO_LPASS_EXT_MCLK2_CFG_RCGR_ALT_SRC_SEL_SHFT                                                                        14
#define HWIO_LPASS_EXT_MCLK2_CFG_RCGR_MODE_BMSK                                                                           0x3000
#define HWIO_LPASS_EXT_MCLK2_CFG_RCGR_MODE_SHFT                                                                               12
#define HWIO_LPASS_EXT_MCLK2_CFG_RCGR_SRC_SEL_BMSK                                                                         0x700
#define HWIO_LPASS_EXT_MCLK2_CFG_RCGR_SRC_SEL_SHFT                                                                             8
#define HWIO_LPASS_EXT_MCLK2_CFG_RCGR_SRC_DIV_BMSK                                                                          0x1f
#define HWIO_LPASS_EXT_MCLK2_CFG_RCGR_SRC_DIV_SHFT                                                                             0

#define HWIO_LPASS_EXT_MCLK2_M_ADDR(x)                                                                                ((x) + 0x22008)
#define HWIO_LPASS_EXT_MCLK2_M_RMSK                                                                                         0xff
#define HWIO_LPASS_EXT_MCLK2_M_IN(x)            \
                in_dword(HWIO_LPASS_EXT_MCLK2_M_ADDR(x))
#define HWIO_LPASS_EXT_MCLK2_M_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_EXT_MCLK2_M_ADDR(x), m)
#define HWIO_LPASS_EXT_MCLK2_M_OUT(x, v)            \
                out_dword(HWIO_LPASS_EXT_MCLK2_M_ADDR(x),v)
#define HWIO_LPASS_EXT_MCLK2_M_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_EXT_MCLK2_M_ADDR(x),m,v,HWIO_LPASS_EXT_MCLK2_M_IN(x))
#define HWIO_LPASS_EXT_MCLK2_M_M_BMSK                                                                                       0xff
#define HWIO_LPASS_EXT_MCLK2_M_M_SHFT                                                                                          0

#define HWIO_LPASS_EXT_MCLK2_N_ADDR(x)                                                                                ((x) + 0x2200c)
#define HWIO_LPASS_EXT_MCLK2_N_RMSK                                                                                         0xff
#define HWIO_LPASS_EXT_MCLK2_N_IN(x)            \
                in_dword(HWIO_LPASS_EXT_MCLK2_N_ADDR(x))
#define HWIO_LPASS_EXT_MCLK2_N_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_EXT_MCLK2_N_ADDR(x), m)
#define HWIO_LPASS_EXT_MCLK2_N_OUT(x, v)            \
                out_dword(HWIO_LPASS_EXT_MCLK2_N_ADDR(x),v)
#define HWIO_LPASS_EXT_MCLK2_N_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_EXT_MCLK2_N_ADDR(x),m,v,HWIO_LPASS_EXT_MCLK2_N_IN(x))
#define HWIO_LPASS_EXT_MCLK2_N_NOT_N_MINUS_M_BMSK                                                                           0xff
#define HWIO_LPASS_EXT_MCLK2_N_NOT_N_MINUS_M_SHFT                                                                              0

#define HWIO_LPASS_EXT_MCLK2_D_ADDR(x)                                                                                ((x) + 0x22010)
#define HWIO_LPASS_EXT_MCLK2_D_RMSK                                                                                         0xff
#define HWIO_LPASS_EXT_MCLK2_D_IN(x)            \
                in_dword(HWIO_LPASS_EXT_MCLK2_D_ADDR(x))
#define HWIO_LPASS_EXT_MCLK2_D_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_EXT_MCLK2_D_ADDR(x), m)
#define HWIO_LPASS_EXT_MCLK2_D_OUT(x, v)            \
                out_dword(HWIO_LPASS_EXT_MCLK2_D_ADDR(x),v)
#define HWIO_LPASS_EXT_MCLK2_D_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_EXT_MCLK2_D_ADDR(x),m,v,HWIO_LPASS_EXT_MCLK2_D_IN(x))
#define HWIO_LPASS_EXT_MCLK2_D_NOT_2D_BMSK                                                                                  0xff
#define HWIO_LPASS_EXT_MCLK2_D_NOT_2D_SHFT                                                                                     0

#define HWIO_LPASS_AUDIO_WRAPPER_EXT_MCLK2_CBCR_ADDR(x)                                                               ((x) + 0x22014)
#define HWIO_LPASS_AUDIO_WRAPPER_EXT_MCLK2_CBCR_RMSK                                                                  0x80000003
#define HWIO_LPASS_AUDIO_WRAPPER_EXT_MCLK2_CBCR_IN(x)            \
                in_dword(HWIO_LPASS_AUDIO_WRAPPER_EXT_MCLK2_CBCR_ADDR(x))
#define HWIO_LPASS_AUDIO_WRAPPER_EXT_MCLK2_CBCR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AUDIO_WRAPPER_EXT_MCLK2_CBCR_ADDR(x), m)
#define HWIO_LPASS_AUDIO_WRAPPER_EXT_MCLK2_CBCR_OUT(x, v)            \
                out_dword(HWIO_LPASS_AUDIO_WRAPPER_EXT_MCLK2_CBCR_ADDR(x),v)
#define HWIO_LPASS_AUDIO_WRAPPER_EXT_MCLK2_CBCR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AUDIO_WRAPPER_EXT_MCLK2_CBCR_ADDR(x),m,v,HWIO_LPASS_AUDIO_WRAPPER_EXT_MCLK2_CBCR_IN(x))
#define HWIO_LPASS_AUDIO_WRAPPER_EXT_MCLK2_CBCR_CLK_OFF_BMSK                                                          0x80000000
#define HWIO_LPASS_AUDIO_WRAPPER_EXT_MCLK2_CBCR_CLK_OFF_SHFT                                                                  31
#define HWIO_LPASS_AUDIO_WRAPPER_EXT_MCLK2_CBCR_HW_CTL_BMSK                                                                  0x2
#define HWIO_LPASS_AUDIO_WRAPPER_EXT_MCLK2_CBCR_HW_CTL_SHFT                                                                    1
#define HWIO_LPASS_AUDIO_WRAPPER_EXT_MCLK2_CBCR_CLK_ENABLE_BMSK                                                              0x1
#define HWIO_LPASS_AUDIO_WRAPPER_EXT_MCLK2_CBCR_CLK_ENABLE_SHFT                                                                0

#define HWIO_LPASS_AUDIO_CORE_SYSNOC_MPORT_CORE_CBCR_ADDR(x)                                                          ((x) + 0x23000)
#define HWIO_LPASS_AUDIO_CORE_SYSNOC_MPORT_CORE_CBCR_RMSK                                                             0x80000003
#define HWIO_LPASS_AUDIO_CORE_SYSNOC_MPORT_CORE_CBCR_IN(x)            \
                in_dword(HWIO_LPASS_AUDIO_CORE_SYSNOC_MPORT_CORE_CBCR_ADDR(x))
#define HWIO_LPASS_AUDIO_CORE_SYSNOC_MPORT_CORE_CBCR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AUDIO_CORE_SYSNOC_MPORT_CORE_CBCR_ADDR(x), m)
#define HWIO_LPASS_AUDIO_CORE_SYSNOC_MPORT_CORE_CBCR_OUT(x, v)            \
                out_dword(HWIO_LPASS_AUDIO_CORE_SYSNOC_MPORT_CORE_CBCR_ADDR(x),v)
#define HWIO_LPASS_AUDIO_CORE_SYSNOC_MPORT_CORE_CBCR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AUDIO_CORE_SYSNOC_MPORT_CORE_CBCR_ADDR(x),m,v,HWIO_LPASS_AUDIO_CORE_SYSNOC_MPORT_CORE_CBCR_IN(x))
#define HWIO_LPASS_AUDIO_CORE_SYSNOC_MPORT_CORE_CBCR_CLK_OFF_BMSK                                                     0x80000000
#define HWIO_LPASS_AUDIO_CORE_SYSNOC_MPORT_CORE_CBCR_CLK_OFF_SHFT                                                             31
#define HWIO_LPASS_AUDIO_CORE_SYSNOC_MPORT_CORE_CBCR_HW_CTL_BMSK                                                             0x2
#define HWIO_LPASS_AUDIO_CORE_SYSNOC_MPORT_CORE_CBCR_HW_CTL_SHFT                                                               1
#define HWIO_LPASS_AUDIO_CORE_SYSNOC_MPORT_CORE_CBCR_CLK_ENABLE_BMSK                                                         0x1
#define HWIO_LPASS_AUDIO_CORE_SYSNOC_MPORT_CORE_CBCR_CLK_ENABLE_SHFT                                                           0

#define HWIO_LPASS_AUDIO_CORE_SYSNOC_SWAY_SNOC_CBCR_ADDR(x)                                                           ((x) + 0x24000)
#define HWIO_LPASS_AUDIO_CORE_SYSNOC_SWAY_SNOC_CBCR_RMSK                                                              0x80000003
#define HWIO_LPASS_AUDIO_CORE_SYSNOC_SWAY_SNOC_CBCR_IN(x)            \
                in_dword(HWIO_LPASS_AUDIO_CORE_SYSNOC_SWAY_SNOC_CBCR_ADDR(x))
#define HWIO_LPASS_AUDIO_CORE_SYSNOC_SWAY_SNOC_CBCR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AUDIO_CORE_SYSNOC_SWAY_SNOC_CBCR_ADDR(x), m)
#define HWIO_LPASS_AUDIO_CORE_SYSNOC_SWAY_SNOC_CBCR_OUT(x, v)            \
                out_dword(HWIO_LPASS_AUDIO_CORE_SYSNOC_SWAY_SNOC_CBCR_ADDR(x),v)
#define HWIO_LPASS_AUDIO_CORE_SYSNOC_SWAY_SNOC_CBCR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AUDIO_CORE_SYSNOC_SWAY_SNOC_CBCR_ADDR(x),m,v,HWIO_LPASS_AUDIO_CORE_SYSNOC_SWAY_SNOC_CBCR_IN(x))
#define HWIO_LPASS_AUDIO_CORE_SYSNOC_SWAY_SNOC_CBCR_CLK_OFF_BMSK                                                      0x80000000
#define HWIO_LPASS_AUDIO_CORE_SYSNOC_SWAY_SNOC_CBCR_CLK_OFF_SHFT                                                              31
#define HWIO_LPASS_AUDIO_CORE_SYSNOC_SWAY_SNOC_CBCR_HW_CTL_BMSK                                                              0x2
#define HWIO_LPASS_AUDIO_CORE_SYSNOC_SWAY_SNOC_CBCR_HW_CTL_SHFT                                                                1
#define HWIO_LPASS_AUDIO_CORE_SYSNOC_SWAY_SNOC_CBCR_CLK_ENABLE_BMSK                                                          0x1
#define HWIO_LPASS_AUDIO_CORE_SYSNOC_SWAY_SNOC_CBCR_CLK_ENABLE_SHFT                                                            0

#define HWIO_LPASS_AUDIO_WRAPPER_SYSNOC_SWAY_AON_CBCR_ADDR(x)                                                         ((x) + 0x25000)
#define HWIO_LPASS_AUDIO_WRAPPER_SYSNOC_SWAY_AON_CBCR_RMSK                                                            0x80000003
#define HWIO_LPASS_AUDIO_WRAPPER_SYSNOC_SWAY_AON_CBCR_IN(x)            \
                in_dword(HWIO_LPASS_AUDIO_WRAPPER_SYSNOC_SWAY_AON_CBCR_ADDR(x))
#define HWIO_LPASS_AUDIO_WRAPPER_SYSNOC_SWAY_AON_CBCR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AUDIO_WRAPPER_SYSNOC_SWAY_AON_CBCR_ADDR(x), m)
#define HWIO_LPASS_AUDIO_WRAPPER_SYSNOC_SWAY_AON_CBCR_OUT(x, v)            \
                out_dword(HWIO_LPASS_AUDIO_WRAPPER_SYSNOC_SWAY_AON_CBCR_ADDR(x),v)
#define HWIO_LPASS_AUDIO_WRAPPER_SYSNOC_SWAY_AON_CBCR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AUDIO_WRAPPER_SYSNOC_SWAY_AON_CBCR_ADDR(x),m,v,HWIO_LPASS_AUDIO_WRAPPER_SYSNOC_SWAY_AON_CBCR_IN(x))
#define HWIO_LPASS_AUDIO_WRAPPER_SYSNOC_SWAY_AON_CBCR_CLK_OFF_BMSK                                                    0x80000000
#define HWIO_LPASS_AUDIO_WRAPPER_SYSNOC_SWAY_AON_CBCR_CLK_OFF_SHFT                                                            31
#define HWIO_LPASS_AUDIO_WRAPPER_SYSNOC_SWAY_AON_CBCR_HW_CTL_BMSK                                                            0x2
#define HWIO_LPASS_AUDIO_WRAPPER_SYSNOC_SWAY_AON_CBCR_HW_CTL_SHFT                                                              1
#define HWIO_LPASS_AUDIO_WRAPPER_SYSNOC_SWAY_AON_CBCR_CLK_ENABLE_BMSK                                                        0x1
#define HWIO_LPASS_AUDIO_WRAPPER_SYSNOC_SWAY_AON_CBCR_CLK_ENABLE_SHFT                                                          0

#define HWIO_LPASS_Q6SS_AHBM_AON_CBCR_ADDR(x)                                                                         ((x) + 0x26000)
#define HWIO_LPASS_Q6SS_AHBM_AON_CBCR_RMSK                                                                            0x80000003
#define HWIO_LPASS_Q6SS_AHBM_AON_CBCR_IN(x)            \
                in_dword(HWIO_LPASS_Q6SS_AHBM_AON_CBCR_ADDR(x))
#define HWIO_LPASS_Q6SS_AHBM_AON_CBCR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_Q6SS_AHBM_AON_CBCR_ADDR(x), m)
#define HWIO_LPASS_Q6SS_AHBM_AON_CBCR_OUT(x, v)            \
                out_dword(HWIO_LPASS_Q6SS_AHBM_AON_CBCR_ADDR(x),v)
#define HWIO_LPASS_Q6SS_AHBM_AON_CBCR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_Q6SS_AHBM_AON_CBCR_ADDR(x),m,v,HWIO_LPASS_Q6SS_AHBM_AON_CBCR_IN(x))
#define HWIO_LPASS_Q6SS_AHBM_AON_CBCR_CLK_OFF_BMSK                                                                    0x80000000
#define HWIO_LPASS_Q6SS_AHBM_AON_CBCR_CLK_OFF_SHFT                                                                            31
#define HWIO_LPASS_Q6SS_AHBM_AON_CBCR_HW_CTL_BMSK                                                                            0x2
#define HWIO_LPASS_Q6SS_AHBM_AON_CBCR_HW_CTL_SHFT                                                                              1
#define HWIO_LPASS_Q6SS_AHBM_AON_CBCR_CLK_ENABLE_BMSK                                                                        0x1
#define HWIO_LPASS_Q6SS_AHBM_AON_CBCR_CLK_ENABLE_SHFT                                                                          0

#define HWIO_LPASS_AUDIO_CORE_AXIM_CBCR_ADDR(x)                                                                       ((x) + 0x27000)
#define HWIO_LPASS_AUDIO_CORE_AXIM_CBCR_RMSK                                                                          0x80000003
#define HWIO_LPASS_AUDIO_CORE_AXIM_CBCR_IN(x)            \
                in_dword(HWIO_LPASS_AUDIO_CORE_AXIM_CBCR_ADDR(x))
#define HWIO_LPASS_AUDIO_CORE_AXIM_CBCR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AUDIO_CORE_AXIM_CBCR_ADDR(x), m)
#define HWIO_LPASS_AUDIO_CORE_AXIM_CBCR_OUT(x, v)            \
                out_dword(HWIO_LPASS_AUDIO_CORE_AXIM_CBCR_ADDR(x),v)
#define HWIO_LPASS_AUDIO_CORE_AXIM_CBCR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AUDIO_CORE_AXIM_CBCR_ADDR(x),m,v,HWIO_LPASS_AUDIO_CORE_AXIM_CBCR_IN(x))
#define HWIO_LPASS_AUDIO_CORE_AXIM_CBCR_CLK_OFF_BMSK                                                                  0x80000000
#define HWIO_LPASS_AUDIO_CORE_AXIM_CBCR_CLK_OFF_SHFT                                                                          31
#define HWIO_LPASS_AUDIO_CORE_AXIM_CBCR_HW_CTL_BMSK                                                                          0x2
#define HWIO_LPASS_AUDIO_CORE_AXIM_CBCR_HW_CTL_SHFT                                                                            1
#define HWIO_LPASS_AUDIO_CORE_AXIM_CBCR_CLK_ENABLE_BMSK                                                                      0x1
#define HWIO_LPASS_AUDIO_CORE_AXIM_CBCR_CLK_ENABLE_SHFT                                                                        0

#define HWIO_LPASS_AUDIO_CORE_QDSP_SWAY_AON_CBCR_ADDR(x)                                                              ((x) + 0x28000)
#define HWIO_LPASS_AUDIO_CORE_QDSP_SWAY_AON_CBCR_RMSK                                                                 0x80000003
#define HWIO_LPASS_AUDIO_CORE_QDSP_SWAY_AON_CBCR_IN(x)            \
                in_dword(HWIO_LPASS_AUDIO_CORE_QDSP_SWAY_AON_CBCR_ADDR(x))
#define HWIO_LPASS_AUDIO_CORE_QDSP_SWAY_AON_CBCR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AUDIO_CORE_QDSP_SWAY_AON_CBCR_ADDR(x), m)
#define HWIO_LPASS_AUDIO_CORE_QDSP_SWAY_AON_CBCR_OUT(x, v)            \
                out_dword(HWIO_LPASS_AUDIO_CORE_QDSP_SWAY_AON_CBCR_ADDR(x),v)
#define HWIO_LPASS_AUDIO_CORE_QDSP_SWAY_AON_CBCR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AUDIO_CORE_QDSP_SWAY_AON_CBCR_ADDR(x),m,v,HWIO_LPASS_AUDIO_CORE_QDSP_SWAY_AON_CBCR_IN(x))
#define HWIO_LPASS_AUDIO_CORE_QDSP_SWAY_AON_CBCR_CLK_OFF_BMSK                                                         0x80000000
#define HWIO_LPASS_AUDIO_CORE_QDSP_SWAY_AON_CBCR_CLK_OFF_SHFT                                                                 31
#define HWIO_LPASS_AUDIO_CORE_QDSP_SWAY_AON_CBCR_HW_CTL_BMSK                                                                 0x2
#define HWIO_LPASS_AUDIO_CORE_QDSP_SWAY_AON_CBCR_HW_CTL_SHFT                                                                   1
#define HWIO_LPASS_AUDIO_CORE_QDSP_SWAY_AON_CBCR_CLK_ENABLE_BMSK                                                             0x1
#define HWIO_LPASS_AUDIO_CORE_QDSP_SWAY_AON_CBCR_CLK_ENABLE_SHFT                                                               0

#define HWIO_LPASS_AUDIO_WRAPPER_SYSNOC_SWAY_SNOC_CBCR_ADDR(x)                                                        ((x) + 0x29000)
#define HWIO_LPASS_AUDIO_WRAPPER_SYSNOC_SWAY_SNOC_CBCR_RMSK                                                           0x80000003
#define HWIO_LPASS_AUDIO_WRAPPER_SYSNOC_SWAY_SNOC_CBCR_IN(x)            \
                in_dword(HWIO_LPASS_AUDIO_WRAPPER_SYSNOC_SWAY_SNOC_CBCR_ADDR(x))
#define HWIO_LPASS_AUDIO_WRAPPER_SYSNOC_SWAY_SNOC_CBCR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AUDIO_WRAPPER_SYSNOC_SWAY_SNOC_CBCR_ADDR(x), m)
#define HWIO_LPASS_AUDIO_WRAPPER_SYSNOC_SWAY_SNOC_CBCR_OUT(x, v)            \
                out_dword(HWIO_LPASS_AUDIO_WRAPPER_SYSNOC_SWAY_SNOC_CBCR_ADDR(x),v)
#define HWIO_LPASS_AUDIO_WRAPPER_SYSNOC_SWAY_SNOC_CBCR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AUDIO_WRAPPER_SYSNOC_SWAY_SNOC_CBCR_ADDR(x),m,v,HWIO_LPASS_AUDIO_WRAPPER_SYSNOC_SWAY_SNOC_CBCR_IN(x))
#define HWIO_LPASS_AUDIO_WRAPPER_SYSNOC_SWAY_SNOC_CBCR_CLK_OFF_BMSK                                                   0x80000000
#define HWIO_LPASS_AUDIO_WRAPPER_SYSNOC_SWAY_SNOC_CBCR_CLK_OFF_SHFT                                                           31
#define HWIO_LPASS_AUDIO_WRAPPER_SYSNOC_SWAY_SNOC_CBCR_HW_CTL_BMSK                                                           0x2
#define HWIO_LPASS_AUDIO_WRAPPER_SYSNOC_SWAY_SNOC_CBCR_HW_CTL_SHFT                                                             1
#define HWIO_LPASS_AUDIO_WRAPPER_SYSNOC_SWAY_SNOC_CBCR_CLK_ENABLE_BMSK                                                       0x1
#define HWIO_LPASS_AUDIO_WRAPPER_SYSNOC_SWAY_SNOC_CBCR_CLK_ENABLE_SHFT                                                         0

#define HWIO_LPASS_Q6SS_ALT_RESET_AON_CBCR_ADDR(x)                                                                    ((x) + 0x2a000)
#define HWIO_LPASS_Q6SS_ALT_RESET_AON_CBCR_RMSK                                                                       0x80000003
#define HWIO_LPASS_Q6SS_ALT_RESET_AON_CBCR_IN(x)            \
                in_dword(HWIO_LPASS_Q6SS_ALT_RESET_AON_CBCR_ADDR(x))
#define HWIO_LPASS_Q6SS_ALT_RESET_AON_CBCR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_Q6SS_ALT_RESET_AON_CBCR_ADDR(x), m)
#define HWIO_LPASS_Q6SS_ALT_RESET_AON_CBCR_OUT(x, v)            \
                out_dword(HWIO_LPASS_Q6SS_ALT_RESET_AON_CBCR_ADDR(x),v)
#define HWIO_LPASS_Q6SS_ALT_RESET_AON_CBCR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_Q6SS_ALT_RESET_AON_CBCR_ADDR(x),m,v,HWIO_LPASS_Q6SS_ALT_RESET_AON_CBCR_IN(x))
#define HWIO_LPASS_Q6SS_ALT_RESET_AON_CBCR_CLK_OFF_BMSK                                                               0x80000000
#define HWIO_LPASS_Q6SS_ALT_RESET_AON_CBCR_CLK_OFF_SHFT                                                                       31
#define HWIO_LPASS_Q6SS_ALT_RESET_AON_CBCR_HW_CTL_BMSK                                                                       0x2
#define HWIO_LPASS_Q6SS_ALT_RESET_AON_CBCR_HW_CTL_SHFT                                                                         1
#define HWIO_LPASS_Q6SS_ALT_RESET_AON_CBCR_CLK_ENABLE_BMSK                                                                   0x1
#define HWIO_LPASS_Q6SS_ALT_RESET_AON_CBCR_CLK_ENABLE_SHFT                                                                     0

#define HWIO_LPASS_Q6SS_ALT_RESET_CTL_ADDR(x)                                                                         ((x) + 0x2a004)
#define HWIO_LPASS_Q6SS_ALT_RESET_CTL_RMSK                                                                                   0x1
#define HWIO_LPASS_Q6SS_ALT_RESET_CTL_IN(x)            \
                in_dword(HWIO_LPASS_Q6SS_ALT_RESET_CTL_ADDR(x))
#define HWIO_LPASS_Q6SS_ALT_RESET_CTL_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_Q6SS_ALT_RESET_CTL_ADDR(x), m)
#define HWIO_LPASS_Q6SS_ALT_RESET_CTL_OUT(x, v)            \
                out_dword(HWIO_LPASS_Q6SS_ALT_RESET_CTL_ADDR(x),v)
#define HWIO_LPASS_Q6SS_ALT_RESET_CTL_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_Q6SS_ALT_RESET_CTL_ADDR(x),m,v,HWIO_LPASS_Q6SS_ALT_RESET_CTL_IN(x))
#define HWIO_LPASS_Q6SS_ALT_RESET_CTL_ALT_ARES_BYPASS_BMSK                                                                   0x1
#define HWIO_LPASS_Q6SS_ALT_RESET_CTL_ALT_ARES_BYPASS_SHFT                                                                     0

#define HWIO_LPASS_Q6SS_Q6_AXIM_CBCR_ADDR(x)                                                                          ((x) + 0x2d000)
#define HWIO_LPASS_Q6SS_Q6_AXIM_CBCR_RMSK                                                                             0x80000003
#define HWIO_LPASS_Q6SS_Q6_AXIM_CBCR_IN(x)            \
                in_dword(HWIO_LPASS_Q6SS_Q6_AXIM_CBCR_ADDR(x))
#define HWIO_LPASS_Q6SS_Q6_AXIM_CBCR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_Q6SS_Q6_AXIM_CBCR_ADDR(x), m)
#define HWIO_LPASS_Q6SS_Q6_AXIM_CBCR_OUT(x, v)            \
                out_dword(HWIO_LPASS_Q6SS_Q6_AXIM_CBCR_ADDR(x),v)
#define HWIO_LPASS_Q6SS_Q6_AXIM_CBCR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_Q6SS_Q6_AXIM_CBCR_ADDR(x),m,v,HWIO_LPASS_Q6SS_Q6_AXIM_CBCR_IN(x))
#define HWIO_LPASS_Q6SS_Q6_AXIM_CBCR_CLK_OFF_BMSK                                                                     0x80000000
#define HWIO_LPASS_Q6SS_Q6_AXIM_CBCR_CLK_OFF_SHFT                                                                             31
#define HWIO_LPASS_Q6SS_Q6_AXIM_CBCR_HW_CTL_BMSK                                                                             0x2
#define HWIO_LPASS_Q6SS_Q6_AXIM_CBCR_HW_CTL_SHFT                                                                               1
#define HWIO_LPASS_Q6SS_Q6_AXIM_CBCR_CLK_ENABLE_BMSK                                                                         0x1
#define HWIO_LPASS_Q6SS_Q6_AXIM_CBCR_CLK_ENABLE_SHFT                                                                           0

#define HWIO_LPASS_Q6SS_AHBS_AON_CBCR_ADDR(x)                                                                         ((x) + 0x33000)
#define HWIO_LPASS_Q6SS_AHBS_AON_CBCR_RMSK                                                                            0x80000003
#define HWIO_LPASS_Q6SS_AHBS_AON_CBCR_IN(x)            \
                in_dword(HWIO_LPASS_Q6SS_AHBS_AON_CBCR_ADDR(x))
#define HWIO_LPASS_Q6SS_AHBS_AON_CBCR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_Q6SS_AHBS_AON_CBCR_ADDR(x), m)
#define HWIO_LPASS_Q6SS_AHBS_AON_CBCR_OUT(x, v)            \
                out_dword(HWIO_LPASS_Q6SS_AHBS_AON_CBCR_ADDR(x),v)
#define HWIO_LPASS_Q6SS_AHBS_AON_CBCR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_Q6SS_AHBS_AON_CBCR_ADDR(x),m,v,HWIO_LPASS_Q6SS_AHBS_AON_CBCR_IN(x))
#define HWIO_LPASS_Q6SS_AHBS_AON_CBCR_CLK_OFF_BMSK                                                                    0x80000000
#define HWIO_LPASS_Q6SS_AHBS_AON_CBCR_CLK_OFF_SHFT                                                                            31
#define HWIO_LPASS_Q6SS_AHBS_AON_CBCR_HW_CTL_BMSK                                                                            0x2
#define HWIO_LPASS_Q6SS_AHBS_AON_CBCR_HW_CTL_SHFT                                                                              1
#define HWIO_LPASS_Q6SS_AHBS_AON_CBCR_CLK_ENABLE_BMSK                                                                        0x1
#define HWIO_LPASS_Q6SS_AHBS_AON_CBCR_CLK_ENABLE_SHFT                                                                          0

#define HWIO_LPASS_QOS_FIXED_LAT_COUNTER_CMD_RCGR_ADDR(x)                                                             ((x) + 0x35000)
#define HWIO_LPASS_QOS_FIXED_LAT_COUNTER_CMD_RCGR_RMSK                                                                0x800000f3
#define HWIO_LPASS_QOS_FIXED_LAT_COUNTER_CMD_RCGR_IN(x)            \
                in_dword(HWIO_LPASS_QOS_FIXED_LAT_COUNTER_CMD_RCGR_ADDR(x))
#define HWIO_LPASS_QOS_FIXED_LAT_COUNTER_CMD_RCGR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_QOS_FIXED_LAT_COUNTER_CMD_RCGR_ADDR(x), m)
#define HWIO_LPASS_QOS_FIXED_LAT_COUNTER_CMD_RCGR_OUT(x, v)            \
                out_dword(HWIO_LPASS_QOS_FIXED_LAT_COUNTER_CMD_RCGR_ADDR(x),v)
#define HWIO_LPASS_QOS_FIXED_LAT_COUNTER_CMD_RCGR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_QOS_FIXED_LAT_COUNTER_CMD_RCGR_ADDR(x),m,v,HWIO_LPASS_QOS_FIXED_LAT_COUNTER_CMD_RCGR_IN(x))
#define HWIO_LPASS_QOS_FIXED_LAT_COUNTER_CMD_RCGR_ROOT_OFF_BMSK                                                       0x80000000
#define HWIO_LPASS_QOS_FIXED_LAT_COUNTER_CMD_RCGR_ROOT_OFF_SHFT                                                               31
#define HWIO_LPASS_QOS_FIXED_LAT_COUNTER_CMD_RCGR_DIRTY_D_BMSK                                                              0x80
#define HWIO_LPASS_QOS_FIXED_LAT_COUNTER_CMD_RCGR_DIRTY_D_SHFT                                                                 7
#define HWIO_LPASS_QOS_FIXED_LAT_COUNTER_CMD_RCGR_DIRTY_N_BMSK                                                              0x40
#define HWIO_LPASS_QOS_FIXED_LAT_COUNTER_CMD_RCGR_DIRTY_N_SHFT                                                                 6
#define HWIO_LPASS_QOS_FIXED_LAT_COUNTER_CMD_RCGR_DIRTY_M_BMSK                                                              0x20
#define HWIO_LPASS_QOS_FIXED_LAT_COUNTER_CMD_RCGR_DIRTY_M_SHFT                                                                 5
#define HWIO_LPASS_QOS_FIXED_LAT_COUNTER_CMD_RCGR_DIRTY_CFG_RCGR_BMSK                                                       0x10
#define HWIO_LPASS_QOS_FIXED_LAT_COUNTER_CMD_RCGR_DIRTY_CFG_RCGR_SHFT                                                          4
#define HWIO_LPASS_QOS_FIXED_LAT_COUNTER_CMD_RCGR_ROOT_EN_BMSK                                                               0x2
#define HWIO_LPASS_QOS_FIXED_LAT_COUNTER_CMD_RCGR_ROOT_EN_SHFT                                                                 1
#define HWIO_LPASS_QOS_FIXED_LAT_COUNTER_CMD_RCGR_UPDATE_BMSK                                                                0x1
#define HWIO_LPASS_QOS_FIXED_LAT_COUNTER_CMD_RCGR_UPDATE_SHFT                                                                  0

#define HWIO_LPASS_QOS_FIXED_LAT_COUNTER_CFG_RCGR_ADDR(x)                                                             ((x) + 0x35004)
#define HWIO_LPASS_QOS_FIXED_LAT_COUNTER_CFG_RCGR_RMSK                                                                    0x771f
#define HWIO_LPASS_QOS_FIXED_LAT_COUNTER_CFG_RCGR_IN(x)            \
                in_dword(HWIO_LPASS_QOS_FIXED_LAT_COUNTER_CFG_RCGR_ADDR(x))
#define HWIO_LPASS_QOS_FIXED_LAT_COUNTER_CFG_RCGR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_QOS_FIXED_LAT_COUNTER_CFG_RCGR_ADDR(x), m)
#define HWIO_LPASS_QOS_FIXED_LAT_COUNTER_CFG_RCGR_OUT(x, v)            \
                out_dword(HWIO_LPASS_QOS_FIXED_LAT_COUNTER_CFG_RCGR_ADDR(x),v)
#define HWIO_LPASS_QOS_FIXED_LAT_COUNTER_CFG_RCGR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_QOS_FIXED_LAT_COUNTER_CFG_RCGR_ADDR(x),m,v,HWIO_LPASS_QOS_FIXED_LAT_COUNTER_CFG_RCGR_IN(x))
#define HWIO_LPASS_QOS_FIXED_LAT_COUNTER_CFG_RCGR_ALT_SRC_SEL_BMSK                                                        0x4000
#define HWIO_LPASS_QOS_FIXED_LAT_COUNTER_CFG_RCGR_ALT_SRC_SEL_SHFT                                                            14
#define HWIO_LPASS_QOS_FIXED_LAT_COUNTER_CFG_RCGR_MODE_BMSK                                                               0x3000
#define HWIO_LPASS_QOS_FIXED_LAT_COUNTER_CFG_RCGR_MODE_SHFT                                                                   12
#define HWIO_LPASS_QOS_FIXED_LAT_COUNTER_CFG_RCGR_SRC_SEL_BMSK                                                             0x700
#define HWIO_LPASS_QOS_FIXED_LAT_COUNTER_CFG_RCGR_SRC_SEL_SHFT                                                                 8
#define HWIO_LPASS_QOS_FIXED_LAT_COUNTER_CFG_RCGR_SRC_DIV_BMSK                                                              0x1f
#define HWIO_LPASS_QOS_FIXED_LAT_COUNTER_CFG_RCGR_SRC_DIV_SHFT                                                                 0

#define HWIO_LPASS_QOS_FIXED_LAT_COUNTER_M_ADDR(x)                                                                    ((x) + 0x35008)
#define HWIO_LPASS_QOS_FIXED_LAT_COUNTER_M_RMSK                                                                             0xff
#define HWIO_LPASS_QOS_FIXED_LAT_COUNTER_M_IN(x)            \
                in_dword(HWIO_LPASS_QOS_FIXED_LAT_COUNTER_M_ADDR(x))
#define HWIO_LPASS_QOS_FIXED_LAT_COUNTER_M_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_QOS_FIXED_LAT_COUNTER_M_ADDR(x), m)
#define HWIO_LPASS_QOS_FIXED_LAT_COUNTER_M_OUT(x, v)            \
                out_dword(HWIO_LPASS_QOS_FIXED_LAT_COUNTER_M_ADDR(x),v)
#define HWIO_LPASS_QOS_FIXED_LAT_COUNTER_M_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_QOS_FIXED_LAT_COUNTER_M_ADDR(x),m,v,HWIO_LPASS_QOS_FIXED_LAT_COUNTER_M_IN(x))
#define HWIO_LPASS_QOS_FIXED_LAT_COUNTER_M_M_BMSK                                                                           0xff
#define HWIO_LPASS_QOS_FIXED_LAT_COUNTER_M_M_SHFT                                                                              0

#define HWIO_LPASS_QOS_FIXED_LAT_COUNTER_N_ADDR(x)                                                                    ((x) + 0x3500c)
#define HWIO_LPASS_QOS_FIXED_LAT_COUNTER_N_RMSK                                                                             0xff
#define HWIO_LPASS_QOS_FIXED_LAT_COUNTER_N_IN(x)            \
                in_dword(HWIO_LPASS_QOS_FIXED_LAT_COUNTER_N_ADDR(x))
#define HWIO_LPASS_QOS_FIXED_LAT_COUNTER_N_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_QOS_FIXED_LAT_COUNTER_N_ADDR(x), m)
#define HWIO_LPASS_QOS_FIXED_LAT_COUNTER_N_OUT(x, v)            \
                out_dword(HWIO_LPASS_QOS_FIXED_LAT_COUNTER_N_ADDR(x),v)
#define HWIO_LPASS_QOS_FIXED_LAT_COUNTER_N_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_QOS_FIXED_LAT_COUNTER_N_ADDR(x),m,v,HWIO_LPASS_QOS_FIXED_LAT_COUNTER_N_IN(x))
#define HWIO_LPASS_QOS_FIXED_LAT_COUNTER_N_NOT_N_MINUS_M_BMSK                                                               0xff
#define HWIO_LPASS_QOS_FIXED_LAT_COUNTER_N_NOT_N_MINUS_M_SHFT                                                                  0

#define HWIO_LPASS_QOS_FIXED_LAT_COUNTER_D_ADDR(x)                                                                    ((x) + 0x35010)
#define HWIO_LPASS_QOS_FIXED_LAT_COUNTER_D_RMSK                                                                             0xff
#define HWIO_LPASS_QOS_FIXED_LAT_COUNTER_D_IN(x)            \
                in_dword(HWIO_LPASS_QOS_FIXED_LAT_COUNTER_D_ADDR(x))
#define HWIO_LPASS_QOS_FIXED_LAT_COUNTER_D_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_QOS_FIXED_LAT_COUNTER_D_ADDR(x), m)
#define HWIO_LPASS_QOS_FIXED_LAT_COUNTER_D_OUT(x, v)            \
                out_dword(HWIO_LPASS_QOS_FIXED_LAT_COUNTER_D_ADDR(x),v)
#define HWIO_LPASS_QOS_FIXED_LAT_COUNTER_D_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_QOS_FIXED_LAT_COUNTER_D_ADDR(x),m,v,HWIO_LPASS_QOS_FIXED_LAT_COUNTER_D_IN(x))
#define HWIO_LPASS_QOS_FIXED_LAT_COUNTER_D_NOT_2D_BMSK                                                                      0xff
#define HWIO_LPASS_QOS_FIXED_LAT_COUNTER_D_NOT_2D_SHFT                                                                         0

#define HWIO_LPASS_AUDIO_WRAPPER_QOS_AHBS_AON_CBCR_ADDR(x)                                                            ((x) + 0x35014)
#define HWIO_LPASS_AUDIO_WRAPPER_QOS_AHBS_AON_CBCR_RMSK                                                               0x80000003
#define HWIO_LPASS_AUDIO_WRAPPER_QOS_AHBS_AON_CBCR_IN(x)            \
                in_dword(HWIO_LPASS_AUDIO_WRAPPER_QOS_AHBS_AON_CBCR_ADDR(x))
#define HWIO_LPASS_AUDIO_WRAPPER_QOS_AHBS_AON_CBCR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AUDIO_WRAPPER_QOS_AHBS_AON_CBCR_ADDR(x), m)
#define HWIO_LPASS_AUDIO_WRAPPER_QOS_AHBS_AON_CBCR_OUT(x, v)            \
                out_dword(HWIO_LPASS_AUDIO_WRAPPER_QOS_AHBS_AON_CBCR_ADDR(x),v)
#define HWIO_LPASS_AUDIO_WRAPPER_QOS_AHBS_AON_CBCR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AUDIO_WRAPPER_QOS_AHBS_AON_CBCR_ADDR(x),m,v,HWIO_LPASS_AUDIO_WRAPPER_QOS_AHBS_AON_CBCR_IN(x))
#define HWIO_LPASS_AUDIO_WRAPPER_QOS_AHBS_AON_CBCR_CLK_OFF_BMSK                                                       0x80000000
#define HWIO_LPASS_AUDIO_WRAPPER_QOS_AHBS_AON_CBCR_CLK_OFF_SHFT                                                               31
#define HWIO_LPASS_AUDIO_WRAPPER_QOS_AHBS_AON_CBCR_HW_CTL_BMSK                                                               0x2
#define HWIO_LPASS_AUDIO_WRAPPER_QOS_AHBS_AON_CBCR_HW_CTL_SHFT                                                                 1
#define HWIO_LPASS_AUDIO_WRAPPER_QOS_AHBS_AON_CBCR_CLK_ENABLE_BMSK                                                           0x1
#define HWIO_LPASS_AUDIO_WRAPPER_QOS_AHBS_AON_CBCR_CLK_ENABLE_SHFT                                                             0

#define HWIO_LPASS_AUDIO_WRAPPER_QOS_XO_LAT_COUNTER_CBCR_ADDR(x)                                                      ((x) + 0x35018)
#define HWIO_LPASS_AUDIO_WRAPPER_QOS_XO_LAT_COUNTER_CBCR_RMSK                                                         0x80000003
#define HWIO_LPASS_AUDIO_WRAPPER_QOS_XO_LAT_COUNTER_CBCR_IN(x)            \
                in_dword(HWIO_LPASS_AUDIO_WRAPPER_QOS_XO_LAT_COUNTER_CBCR_ADDR(x))
#define HWIO_LPASS_AUDIO_WRAPPER_QOS_XO_LAT_COUNTER_CBCR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AUDIO_WRAPPER_QOS_XO_LAT_COUNTER_CBCR_ADDR(x), m)
#define HWIO_LPASS_AUDIO_WRAPPER_QOS_XO_LAT_COUNTER_CBCR_OUT(x, v)            \
                out_dword(HWIO_LPASS_AUDIO_WRAPPER_QOS_XO_LAT_COUNTER_CBCR_ADDR(x),v)
#define HWIO_LPASS_AUDIO_WRAPPER_QOS_XO_LAT_COUNTER_CBCR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AUDIO_WRAPPER_QOS_XO_LAT_COUNTER_CBCR_ADDR(x),m,v,HWIO_LPASS_AUDIO_WRAPPER_QOS_XO_LAT_COUNTER_CBCR_IN(x))
#define HWIO_LPASS_AUDIO_WRAPPER_QOS_XO_LAT_COUNTER_CBCR_CLK_OFF_BMSK                                                 0x80000000
#define HWIO_LPASS_AUDIO_WRAPPER_QOS_XO_LAT_COUNTER_CBCR_CLK_OFF_SHFT                                                         31
#define HWIO_LPASS_AUDIO_WRAPPER_QOS_XO_LAT_COUNTER_CBCR_HW_CTL_BMSK                                                         0x2
#define HWIO_LPASS_AUDIO_WRAPPER_QOS_XO_LAT_COUNTER_CBCR_HW_CTL_SHFT                                                           1
#define HWIO_LPASS_AUDIO_WRAPPER_QOS_XO_LAT_COUNTER_CBCR_CLK_ENABLE_BMSK                                                     0x1
#define HWIO_LPASS_AUDIO_WRAPPER_QOS_XO_LAT_COUNTER_CBCR_CLK_ENABLE_SHFT                                                       0

#define HWIO_LPASS_AUDIO_WRAPPER_QOS_DMONITOR_FIXED_LAT_COUNTER_CBCR_ADDR(x)                                          ((x) + 0x3501c)
#define HWIO_LPASS_AUDIO_WRAPPER_QOS_DMONITOR_FIXED_LAT_COUNTER_CBCR_RMSK                                             0x80000003
#define HWIO_LPASS_AUDIO_WRAPPER_QOS_DMONITOR_FIXED_LAT_COUNTER_CBCR_IN(x)            \
                in_dword(HWIO_LPASS_AUDIO_WRAPPER_QOS_DMONITOR_FIXED_LAT_COUNTER_CBCR_ADDR(x))
#define HWIO_LPASS_AUDIO_WRAPPER_QOS_DMONITOR_FIXED_LAT_COUNTER_CBCR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AUDIO_WRAPPER_QOS_DMONITOR_FIXED_LAT_COUNTER_CBCR_ADDR(x), m)
#define HWIO_LPASS_AUDIO_WRAPPER_QOS_DMONITOR_FIXED_LAT_COUNTER_CBCR_OUT(x, v)            \
                out_dword(HWIO_LPASS_AUDIO_WRAPPER_QOS_DMONITOR_FIXED_LAT_COUNTER_CBCR_ADDR(x),v)
#define HWIO_LPASS_AUDIO_WRAPPER_QOS_DMONITOR_FIXED_LAT_COUNTER_CBCR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AUDIO_WRAPPER_QOS_DMONITOR_FIXED_LAT_COUNTER_CBCR_ADDR(x),m,v,HWIO_LPASS_AUDIO_WRAPPER_QOS_DMONITOR_FIXED_LAT_COUNTER_CBCR_IN(x))
#define HWIO_LPASS_AUDIO_WRAPPER_QOS_DMONITOR_FIXED_LAT_COUNTER_CBCR_CLK_OFF_BMSK                                     0x80000000
#define HWIO_LPASS_AUDIO_WRAPPER_QOS_DMONITOR_FIXED_LAT_COUNTER_CBCR_CLK_OFF_SHFT                                             31
#define HWIO_LPASS_AUDIO_WRAPPER_QOS_DMONITOR_FIXED_LAT_COUNTER_CBCR_HW_CTL_BMSK                                             0x2
#define HWIO_LPASS_AUDIO_WRAPPER_QOS_DMONITOR_FIXED_LAT_COUNTER_CBCR_HW_CTL_SHFT                                               1
#define HWIO_LPASS_AUDIO_WRAPPER_QOS_DMONITOR_FIXED_LAT_COUNTER_CBCR_CLK_ENABLE_BMSK                                         0x1
#define HWIO_LPASS_AUDIO_WRAPPER_QOS_DMONITOR_FIXED_LAT_COUNTER_CBCR_CLK_ENABLE_SHFT                                           0

#define HWIO_LPASS_AUDIO_WRAPPER_QOS_DANGER_FIXED_LAT_COUNTER_CBCR_ADDR(x)                                            ((x) + 0x35020)
#define HWIO_LPASS_AUDIO_WRAPPER_QOS_DANGER_FIXED_LAT_COUNTER_CBCR_RMSK                                               0x80000003
#define HWIO_LPASS_AUDIO_WRAPPER_QOS_DANGER_FIXED_LAT_COUNTER_CBCR_IN(x)            \
                in_dword(HWIO_LPASS_AUDIO_WRAPPER_QOS_DANGER_FIXED_LAT_COUNTER_CBCR_ADDR(x))
#define HWIO_LPASS_AUDIO_WRAPPER_QOS_DANGER_FIXED_LAT_COUNTER_CBCR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AUDIO_WRAPPER_QOS_DANGER_FIXED_LAT_COUNTER_CBCR_ADDR(x), m)
#define HWIO_LPASS_AUDIO_WRAPPER_QOS_DANGER_FIXED_LAT_COUNTER_CBCR_OUT(x, v)            \
                out_dword(HWIO_LPASS_AUDIO_WRAPPER_QOS_DANGER_FIXED_LAT_COUNTER_CBCR_ADDR(x),v)
#define HWIO_LPASS_AUDIO_WRAPPER_QOS_DANGER_FIXED_LAT_COUNTER_CBCR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AUDIO_WRAPPER_QOS_DANGER_FIXED_LAT_COUNTER_CBCR_ADDR(x),m,v,HWIO_LPASS_AUDIO_WRAPPER_QOS_DANGER_FIXED_LAT_COUNTER_CBCR_IN(x))
#define HWIO_LPASS_AUDIO_WRAPPER_QOS_DANGER_FIXED_LAT_COUNTER_CBCR_CLK_OFF_BMSK                                       0x80000000
#define HWIO_LPASS_AUDIO_WRAPPER_QOS_DANGER_FIXED_LAT_COUNTER_CBCR_CLK_OFF_SHFT                                               31
#define HWIO_LPASS_AUDIO_WRAPPER_QOS_DANGER_FIXED_LAT_COUNTER_CBCR_HW_CTL_BMSK                                               0x2
#define HWIO_LPASS_AUDIO_WRAPPER_QOS_DANGER_FIXED_LAT_COUNTER_CBCR_HW_CTL_SHFT                                                 1
#define HWIO_LPASS_AUDIO_WRAPPER_QOS_DANGER_FIXED_LAT_COUNTER_CBCR_CLK_ENABLE_BMSK                                           0x1
#define HWIO_LPASS_AUDIO_WRAPPER_QOS_DANGER_FIXED_LAT_COUNTER_CBCR_CLK_ENABLE_SHFT                                             0

#define HWIO_LPASS_AUDIO_CORE_QOS_DMONITOR_FIXED_LAT_COUNTER_CBCR_ADDR(x)                                             ((x) + 0x36000)
#define HWIO_LPASS_AUDIO_CORE_QOS_DMONITOR_FIXED_LAT_COUNTER_CBCR_RMSK                                                0x80000003
#define HWIO_LPASS_AUDIO_CORE_QOS_DMONITOR_FIXED_LAT_COUNTER_CBCR_IN(x)            \
                in_dword(HWIO_LPASS_AUDIO_CORE_QOS_DMONITOR_FIXED_LAT_COUNTER_CBCR_ADDR(x))
#define HWIO_LPASS_AUDIO_CORE_QOS_DMONITOR_FIXED_LAT_COUNTER_CBCR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AUDIO_CORE_QOS_DMONITOR_FIXED_LAT_COUNTER_CBCR_ADDR(x), m)
#define HWIO_LPASS_AUDIO_CORE_QOS_DMONITOR_FIXED_LAT_COUNTER_CBCR_OUT(x, v)            \
                out_dword(HWIO_LPASS_AUDIO_CORE_QOS_DMONITOR_FIXED_LAT_COUNTER_CBCR_ADDR(x),v)
#define HWIO_LPASS_AUDIO_CORE_QOS_DMONITOR_FIXED_LAT_COUNTER_CBCR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AUDIO_CORE_QOS_DMONITOR_FIXED_LAT_COUNTER_CBCR_ADDR(x),m,v,HWIO_LPASS_AUDIO_CORE_QOS_DMONITOR_FIXED_LAT_COUNTER_CBCR_IN(x))
#define HWIO_LPASS_AUDIO_CORE_QOS_DMONITOR_FIXED_LAT_COUNTER_CBCR_CLK_OFF_BMSK                                        0x80000000
#define HWIO_LPASS_AUDIO_CORE_QOS_DMONITOR_FIXED_LAT_COUNTER_CBCR_CLK_OFF_SHFT                                                31
#define HWIO_LPASS_AUDIO_CORE_QOS_DMONITOR_FIXED_LAT_COUNTER_CBCR_HW_CTL_BMSK                                                0x2
#define HWIO_LPASS_AUDIO_CORE_QOS_DMONITOR_FIXED_LAT_COUNTER_CBCR_HW_CTL_SHFT                                                  1
#define HWIO_LPASS_AUDIO_CORE_QOS_DMONITOR_FIXED_LAT_COUNTER_CBCR_CLK_ENABLE_BMSK                                            0x1
#define HWIO_LPASS_AUDIO_CORE_QOS_DMONITOR_FIXED_LAT_COUNTER_CBCR_CLK_ENABLE_SHFT                                              0

#define HWIO_LPASS_GDS_HW_CTRL_SEQUENCE_ABORT_IRQ_STATUS_Q6_ADDR(x)                                                   ((x) + 0x37004)
#define HWIO_LPASS_GDS_HW_CTRL_SEQUENCE_ABORT_IRQ_STATUS_Q6_RMSK                                                             0x1
#define HWIO_LPASS_GDS_HW_CTRL_SEQUENCE_ABORT_IRQ_STATUS_Q6_IN(x)            \
                in_dword(HWIO_LPASS_GDS_HW_CTRL_SEQUENCE_ABORT_IRQ_STATUS_Q6_ADDR(x))
#define HWIO_LPASS_GDS_HW_CTRL_SEQUENCE_ABORT_IRQ_STATUS_Q6_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_GDS_HW_CTRL_SEQUENCE_ABORT_IRQ_STATUS_Q6_ADDR(x), m)
#define HWIO_LPASS_GDS_HW_CTRL_SEQUENCE_ABORT_IRQ_STATUS_Q6_OUT(x, v)            \
                out_dword(HWIO_LPASS_GDS_HW_CTRL_SEQUENCE_ABORT_IRQ_STATUS_Q6_ADDR(x),v)
#define HWIO_LPASS_GDS_HW_CTRL_SEQUENCE_ABORT_IRQ_STATUS_Q6_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_GDS_HW_CTRL_SEQUENCE_ABORT_IRQ_STATUS_Q6_ADDR(x),m,v,HWIO_LPASS_GDS_HW_CTRL_SEQUENCE_ABORT_IRQ_STATUS_Q6_IN(x))
#define HWIO_LPASS_GDS_HW_CTRL_SEQUENCE_ABORT_IRQ_STATUS_Q6_LPASS_CORE_GDS_HW_CTRL_SEQUENCE_ABORT_IRQ_BMSK                   0x1
#define HWIO_LPASS_GDS_HW_CTRL_SEQUENCE_ABORT_IRQ_STATUS_Q6_LPASS_CORE_GDS_HW_CTRL_SEQUENCE_ABORT_IRQ_SHFT                     0

#define HWIO_LPASS_GDS_HW_CTRL_SEQUENCE_ABORT_IRQ_ENABLE_Q6_ADDR(x)                                                   ((x) + 0x37008)
#define HWIO_LPASS_GDS_HW_CTRL_SEQUENCE_ABORT_IRQ_ENABLE_Q6_RMSK                                                             0x1
#define HWIO_LPASS_GDS_HW_CTRL_SEQUENCE_ABORT_IRQ_ENABLE_Q6_IN(x)            \
                in_dword(HWIO_LPASS_GDS_HW_CTRL_SEQUENCE_ABORT_IRQ_ENABLE_Q6_ADDR(x))
#define HWIO_LPASS_GDS_HW_CTRL_SEQUENCE_ABORT_IRQ_ENABLE_Q6_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_GDS_HW_CTRL_SEQUENCE_ABORT_IRQ_ENABLE_Q6_ADDR(x), m)
#define HWIO_LPASS_GDS_HW_CTRL_SEQUENCE_ABORT_IRQ_ENABLE_Q6_OUT(x, v)            \
                out_dword(HWIO_LPASS_GDS_HW_CTRL_SEQUENCE_ABORT_IRQ_ENABLE_Q6_ADDR(x),v)
#define HWIO_LPASS_GDS_HW_CTRL_SEQUENCE_ABORT_IRQ_ENABLE_Q6_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_GDS_HW_CTRL_SEQUENCE_ABORT_IRQ_ENABLE_Q6_ADDR(x),m,v,HWIO_LPASS_GDS_HW_CTRL_SEQUENCE_ABORT_IRQ_ENABLE_Q6_IN(x))
#define HWIO_LPASS_GDS_HW_CTRL_SEQUENCE_ABORT_IRQ_ENABLE_Q6_LPASS_CORE_SEQUENCE_ABORT_IRQ_EN_BMSK                            0x1
#define HWIO_LPASS_GDS_HW_CTRL_SEQUENCE_ABORT_IRQ_ENABLE_Q6_LPASS_CORE_SEQUENCE_ABORT_IRQ_EN_SHFT                              0

#define HWIO_LPASS_GDS_HW_CTRL_SEQUENCE_ABORT_IRQ_STATUS_APPS_ADDR(x)                                                 ((x) + 0x38004)
#define HWIO_LPASS_GDS_HW_CTRL_SEQUENCE_ABORT_IRQ_STATUS_APPS_RMSK                                                           0x1
#define HWIO_LPASS_GDS_HW_CTRL_SEQUENCE_ABORT_IRQ_STATUS_APPS_IN(x)            \
                in_dword(HWIO_LPASS_GDS_HW_CTRL_SEQUENCE_ABORT_IRQ_STATUS_APPS_ADDR(x))
#define HWIO_LPASS_GDS_HW_CTRL_SEQUENCE_ABORT_IRQ_STATUS_APPS_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_GDS_HW_CTRL_SEQUENCE_ABORT_IRQ_STATUS_APPS_ADDR(x), m)
#define HWIO_LPASS_GDS_HW_CTRL_SEQUENCE_ABORT_IRQ_STATUS_APPS_OUT(x, v)            \
                out_dword(HWIO_LPASS_GDS_HW_CTRL_SEQUENCE_ABORT_IRQ_STATUS_APPS_ADDR(x),v)
#define HWIO_LPASS_GDS_HW_CTRL_SEQUENCE_ABORT_IRQ_STATUS_APPS_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_GDS_HW_CTRL_SEQUENCE_ABORT_IRQ_STATUS_APPS_ADDR(x),m,v,HWIO_LPASS_GDS_HW_CTRL_SEQUENCE_ABORT_IRQ_STATUS_APPS_IN(x))
#define HWIO_LPASS_GDS_HW_CTRL_SEQUENCE_ABORT_IRQ_STATUS_APPS_LPASS_CORE_GDS_HW_CTRL_SEQUENCE_ABORT_IRQ_BMSK                 0x1
#define HWIO_LPASS_GDS_HW_CTRL_SEQUENCE_ABORT_IRQ_STATUS_APPS_LPASS_CORE_GDS_HW_CTRL_SEQUENCE_ABORT_IRQ_SHFT                   0

#define HWIO_LPASS_GDS_HW_CTRL_SEQUENCE_ABORT_IRQ_ENABLE_APPS_ADDR(x)                                                 ((x) + 0x38008)
#define HWIO_LPASS_GDS_HW_CTRL_SEQUENCE_ABORT_IRQ_ENABLE_APPS_RMSK                                                           0x1
#define HWIO_LPASS_GDS_HW_CTRL_SEQUENCE_ABORT_IRQ_ENABLE_APPS_IN(x)            \
                in_dword(HWIO_LPASS_GDS_HW_CTRL_SEQUENCE_ABORT_IRQ_ENABLE_APPS_ADDR(x))
#define HWIO_LPASS_GDS_HW_CTRL_SEQUENCE_ABORT_IRQ_ENABLE_APPS_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_GDS_HW_CTRL_SEQUENCE_ABORT_IRQ_ENABLE_APPS_ADDR(x), m)
#define HWIO_LPASS_GDS_HW_CTRL_SEQUENCE_ABORT_IRQ_ENABLE_APPS_OUT(x, v)            \
                out_dword(HWIO_LPASS_GDS_HW_CTRL_SEQUENCE_ABORT_IRQ_ENABLE_APPS_ADDR(x),v)
#define HWIO_LPASS_GDS_HW_CTRL_SEQUENCE_ABORT_IRQ_ENABLE_APPS_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_GDS_HW_CTRL_SEQUENCE_ABORT_IRQ_ENABLE_APPS_ADDR(x),m,v,HWIO_LPASS_GDS_HW_CTRL_SEQUENCE_ABORT_IRQ_ENABLE_APPS_IN(x))
#define HWIO_LPASS_GDS_HW_CTRL_SEQUENCE_ABORT_IRQ_ENABLE_APPS_LPASS_CORE_SEQUENCE_ABORT_IRQ_EN_BMSK                          0x1
#define HWIO_LPASS_GDS_HW_CTRL_SEQUENCE_ABORT_IRQ_ENABLE_APPS_LPASS_CORE_SEQUENCE_ABORT_IRQ_EN_SHFT                            0

#define HWIO_LPASS_AUDIO_WRAPPER_BUS_TIMEOUT_AON_CBCR_ADDR(x)                                                         ((x) + 0x3a000)
#define HWIO_LPASS_AUDIO_WRAPPER_BUS_TIMEOUT_AON_CBCR_RMSK                                                            0x80000003
#define HWIO_LPASS_AUDIO_WRAPPER_BUS_TIMEOUT_AON_CBCR_IN(x)            \
                in_dword(HWIO_LPASS_AUDIO_WRAPPER_BUS_TIMEOUT_AON_CBCR_ADDR(x))
#define HWIO_LPASS_AUDIO_WRAPPER_BUS_TIMEOUT_AON_CBCR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AUDIO_WRAPPER_BUS_TIMEOUT_AON_CBCR_ADDR(x), m)
#define HWIO_LPASS_AUDIO_WRAPPER_BUS_TIMEOUT_AON_CBCR_OUT(x, v)            \
                out_dword(HWIO_LPASS_AUDIO_WRAPPER_BUS_TIMEOUT_AON_CBCR_ADDR(x),v)
#define HWIO_LPASS_AUDIO_WRAPPER_BUS_TIMEOUT_AON_CBCR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AUDIO_WRAPPER_BUS_TIMEOUT_AON_CBCR_ADDR(x),m,v,HWIO_LPASS_AUDIO_WRAPPER_BUS_TIMEOUT_AON_CBCR_IN(x))
#define HWIO_LPASS_AUDIO_WRAPPER_BUS_TIMEOUT_AON_CBCR_CLK_OFF_BMSK                                                    0x80000000
#define HWIO_LPASS_AUDIO_WRAPPER_BUS_TIMEOUT_AON_CBCR_CLK_OFF_SHFT                                                            31
#define HWIO_LPASS_AUDIO_WRAPPER_BUS_TIMEOUT_AON_CBCR_HW_CTL_BMSK                                                            0x2
#define HWIO_LPASS_AUDIO_WRAPPER_BUS_TIMEOUT_AON_CBCR_HW_CTL_SHFT                                                              1
#define HWIO_LPASS_AUDIO_WRAPPER_BUS_TIMEOUT_AON_CBCR_CLK_ENABLE_BMSK                                                        0x1
#define HWIO_LPASS_AUDIO_WRAPPER_BUS_TIMEOUT_AON_CBCR_CLK_ENABLE_SHFT                                                          0

#define HWIO_LPASS_AUDIO_CORE_BUS_TIMEOUT_CORE_CBCR_ADDR(x)                                                           ((x) + 0x3b000)
#define HWIO_LPASS_AUDIO_CORE_BUS_TIMEOUT_CORE_CBCR_RMSK                                                              0x80000003
#define HWIO_LPASS_AUDIO_CORE_BUS_TIMEOUT_CORE_CBCR_IN(x)            \
                in_dword(HWIO_LPASS_AUDIO_CORE_BUS_TIMEOUT_CORE_CBCR_ADDR(x))
#define HWIO_LPASS_AUDIO_CORE_BUS_TIMEOUT_CORE_CBCR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AUDIO_CORE_BUS_TIMEOUT_CORE_CBCR_ADDR(x), m)
#define HWIO_LPASS_AUDIO_CORE_BUS_TIMEOUT_CORE_CBCR_OUT(x, v)            \
                out_dword(HWIO_LPASS_AUDIO_CORE_BUS_TIMEOUT_CORE_CBCR_ADDR(x),v)
#define HWIO_LPASS_AUDIO_CORE_BUS_TIMEOUT_CORE_CBCR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AUDIO_CORE_BUS_TIMEOUT_CORE_CBCR_ADDR(x),m,v,HWIO_LPASS_AUDIO_CORE_BUS_TIMEOUT_CORE_CBCR_IN(x))
#define HWIO_LPASS_AUDIO_CORE_BUS_TIMEOUT_CORE_CBCR_CLK_OFF_BMSK                                                      0x80000000
#define HWIO_LPASS_AUDIO_CORE_BUS_TIMEOUT_CORE_CBCR_CLK_OFF_SHFT                                                              31
#define HWIO_LPASS_AUDIO_CORE_BUS_TIMEOUT_CORE_CBCR_HW_CTL_BMSK                                                              0x2
#define HWIO_LPASS_AUDIO_CORE_BUS_TIMEOUT_CORE_CBCR_HW_CTL_SHFT                                                                1
#define HWIO_LPASS_AUDIO_CORE_BUS_TIMEOUT_CORE_CBCR_CLK_ENABLE_BMSK                                                          0x1
#define HWIO_LPASS_AUDIO_CORE_BUS_TIMEOUT_CORE_CBCR_CLK_ENABLE_SHFT                                                            0

#define HWIO_LPASS_JBIST_PLL_SRC_SEL_ADDR(x)                                                                          ((x) + 0x3e000)
#define HWIO_LPASS_JBIST_PLL_SRC_SEL_RMSK                                                                                    0x3
#define HWIO_LPASS_JBIST_PLL_SRC_SEL_IN(x)            \
                in_dword(HWIO_LPASS_JBIST_PLL_SRC_SEL_ADDR(x))
#define HWIO_LPASS_JBIST_PLL_SRC_SEL_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_JBIST_PLL_SRC_SEL_ADDR(x), m)
#define HWIO_LPASS_JBIST_PLL_SRC_SEL_OUT(x, v)            \
                out_dword(HWIO_LPASS_JBIST_PLL_SRC_SEL_ADDR(x),v)
#define HWIO_LPASS_JBIST_PLL_SRC_SEL_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_JBIST_PLL_SRC_SEL_ADDR(x),m,v,HWIO_LPASS_JBIST_PLL_SRC_SEL_IN(x))
#define HWIO_LPASS_JBIST_PLL_SRC_SEL_JBIST_PLL_CLK_SRC_SEL_BMSK                                                              0x3
#define HWIO_LPASS_JBIST_PLL_SRC_SEL_JBIST_PLL_CLK_SRC_SEL_SHFT                                                                0

#define HWIO_LPASS_JBIST_MODE_ADDR(x)                                                                                 ((x) + 0x3e004)
#define HWIO_LPASS_JBIST_MODE_RMSK                                                                                    0xffffffff
#define HWIO_LPASS_JBIST_MODE_IN(x)            \
                in_dword(HWIO_LPASS_JBIST_MODE_ADDR(x))
#define HWIO_LPASS_JBIST_MODE_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_JBIST_MODE_ADDR(x), m)
#define HWIO_LPASS_JBIST_MODE_OUT(x, v)            \
                out_dword(HWIO_LPASS_JBIST_MODE_ADDR(x),v)
#define HWIO_LPASS_JBIST_MODE_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_JBIST_MODE_ADDR(x),m,v,HWIO_LPASS_JBIST_MODE_IN(x))
#define HWIO_LPASS_JBIST_MODE_RESERVE_BITS31_4_BMSK                                                                   0xfffffff0
#define HWIO_LPASS_JBIST_MODE_RESERVE_BITS31_4_SHFT                                                                            4
#define HWIO_LPASS_JBIST_MODE_START_MEAS_BMSK                                                                                0x8
#define HWIO_LPASS_JBIST_MODE_START_MEAS_SHFT                                                                                  3
#define HWIO_LPASS_JBIST_MODE_JBIST_TEST_BMSK                                                                                0x4
#define HWIO_LPASS_JBIST_MODE_JBIST_TEST_SHFT                                                                                  2
#define HWIO_LPASS_JBIST_MODE_RESET_N_BMSK                                                                                   0x2
#define HWIO_LPASS_JBIST_MODE_RESET_N_SHFT                                                                                     1
#define HWIO_LPASS_JBIST_MODE_SLEEP_N_BMSK                                                                                   0x1
#define HWIO_LPASS_JBIST_MODE_SLEEP_N_SHFT                                                                                     0

#define HWIO_LPASS_JBIST_CONFIG_CTL_ADDR(x)                                                                           ((x) + 0x3e008)
#define HWIO_LPASS_JBIST_CONFIG_CTL_RMSK                                                                              0xffffffff
#define HWIO_LPASS_JBIST_CONFIG_CTL_IN(x)            \
                in_dword(HWIO_LPASS_JBIST_CONFIG_CTL_ADDR(x))
#define HWIO_LPASS_JBIST_CONFIG_CTL_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_JBIST_CONFIG_CTL_ADDR(x), m)
#define HWIO_LPASS_JBIST_CONFIG_CTL_OUT(x, v)            \
                out_dword(HWIO_LPASS_JBIST_CONFIG_CTL_ADDR(x),v)
#define HWIO_LPASS_JBIST_CONFIG_CTL_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_JBIST_CONFIG_CTL_ADDR(x),m,v,HWIO_LPASS_JBIST_CONFIG_CTL_IN(x))
#define HWIO_LPASS_JBIST_CONFIG_CTL_JBIST_CONFIG_CTL_BMSK                                                             0xffffffff
#define HWIO_LPASS_JBIST_CONFIG_CTL_JBIST_CONFIG_CTL_SHFT                                                                      0

#define HWIO_LPASS_JBIST_USER_CTL_ADDR(x)                                                                             ((x) + 0x3e00c)
#define HWIO_LPASS_JBIST_USER_CTL_RMSK                                                                                0xffffffff
#define HWIO_LPASS_JBIST_USER_CTL_IN(x)            \
                in_dword(HWIO_LPASS_JBIST_USER_CTL_ADDR(x))
#define HWIO_LPASS_JBIST_USER_CTL_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_JBIST_USER_CTL_ADDR(x), m)
#define HWIO_LPASS_JBIST_USER_CTL_OUT(x, v)            \
                out_dword(HWIO_LPASS_JBIST_USER_CTL_ADDR(x),v)
#define HWIO_LPASS_JBIST_USER_CTL_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_JBIST_USER_CTL_ADDR(x),m,v,HWIO_LPASS_JBIST_USER_CTL_IN(x))
#define HWIO_LPASS_JBIST_USER_CTL_JBIST_USER_CTL_BMSK                                                                 0xffffffff
#define HWIO_LPASS_JBIST_USER_CTL_JBIST_USER_CTL_SHFT                                                                          0

#define HWIO_LPASS_JBIST_USER_CTL_U_ADDR(x)                                                                           ((x) + 0x3e010)
#define HWIO_LPASS_JBIST_USER_CTL_U_RMSK                                                                              0xffffffff
#define HWIO_LPASS_JBIST_USER_CTL_U_IN(x)            \
                in_dword(HWIO_LPASS_JBIST_USER_CTL_U_ADDR(x))
#define HWIO_LPASS_JBIST_USER_CTL_U_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_JBIST_USER_CTL_U_ADDR(x), m)
#define HWIO_LPASS_JBIST_USER_CTL_U_OUT(x, v)            \
                out_dword(HWIO_LPASS_JBIST_USER_CTL_U_ADDR(x),v)
#define HWIO_LPASS_JBIST_USER_CTL_U_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_JBIST_USER_CTL_U_ADDR(x),m,v,HWIO_LPASS_JBIST_USER_CTL_U_IN(x))
#define HWIO_LPASS_JBIST_USER_CTL_U_JBIST_USER_CTL_U_BMSK                                                             0xffffffff
#define HWIO_LPASS_JBIST_USER_CTL_U_JBIST_USER_CTL_U_SHFT                                                                      0

#define HWIO_LPASS_JBIST_STATUS_ADDR(x)                                                                               ((x) + 0x3e018)
#define HWIO_LPASS_JBIST_STATUS_RMSK                                                                                  0xffffffff
#define HWIO_LPASS_JBIST_STATUS_IN(x)            \
                in_dword(HWIO_LPASS_JBIST_STATUS_ADDR(x))
#define HWIO_LPASS_JBIST_STATUS_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_JBIST_STATUS_ADDR(x), m)
#define HWIO_LPASS_JBIST_STATUS_JBIST_STATUS_BMSK                                                                     0xffffffff
#define HWIO_LPASS_JBIST_STATUS_JBIST_STATUS_SHFT                                                                              0

#define HWIO_LPASS_JBIST_MEAS_DONE_ADDR(x)                                                                            ((x) + 0x3e01c)
#define HWIO_LPASS_JBIST_MEAS_DONE_RMSK                                                                               0xffffffff
#define HWIO_LPASS_JBIST_MEAS_DONE_IN(x)            \
                in_dword(HWIO_LPASS_JBIST_MEAS_DONE_ADDR(x))
#define HWIO_LPASS_JBIST_MEAS_DONE_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_JBIST_MEAS_DONE_ADDR(x), m)
#define HWIO_LPASS_JBIST_MEAS_DONE_OUT(x, v)            \
                out_dword(HWIO_LPASS_JBIST_MEAS_DONE_ADDR(x),v)
#define HWIO_LPASS_JBIST_MEAS_DONE_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_JBIST_MEAS_DONE_ADDR(x),m,v,HWIO_LPASS_JBIST_MEAS_DONE_IN(x))
#define HWIO_LPASS_JBIST_MEAS_DONE_RESERVE_BITS31_1_BMSK                                                              0xfffffffe
#define HWIO_LPASS_JBIST_MEAS_DONE_RESERVE_BITS31_1_SHFT                                                                       1
#define HWIO_LPASS_JBIST_MEAS_DONE_JBIST_MEAS_DONE_BMSK                                                                      0x1
#define HWIO_LPASS_JBIST_MEAS_DONE_JBIST_MEAS_DONE_SHFT                                                                        0

#define HWIO_LPASS_AUDIO_CORE_LPAIF_RXTX_WR_MEM_CBCR_ADDR(x)                                                          ((x) + 0x40000)
#define HWIO_LPASS_AUDIO_CORE_LPAIF_RXTX_WR_MEM_CBCR_RMSK                                                             0x80007ff3
#define HWIO_LPASS_AUDIO_CORE_LPAIF_RXTX_WR_MEM_CBCR_IN(x)            \
                in_dword(HWIO_LPASS_AUDIO_CORE_LPAIF_RXTX_WR_MEM_CBCR_ADDR(x))
#define HWIO_LPASS_AUDIO_CORE_LPAIF_RXTX_WR_MEM_CBCR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AUDIO_CORE_LPAIF_RXTX_WR_MEM_CBCR_ADDR(x), m)
#define HWIO_LPASS_AUDIO_CORE_LPAIF_RXTX_WR_MEM_CBCR_OUT(x, v)            \
                out_dword(HWIO_LPASS_AUDIO_CORE_LPAIF_RXTX_WR_MEM_CBCR_ADDR(x),v)
#define HWIO_LPASS_AUDIO_CORE_LPAIF_RXTX_WR_MEM_CBCR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AUDIO_CORE_LPAIF_RXTX_WR_MEM_CBCR_ADDR(x),m,v,HWIO_LPASS_AUDIO_CORE_LPAIF_RXTX_WR_MEM_CBCR_IN(x))
#define HWIO_LPASS_AUDIO_CORE_LPAIF_RXTX_WR_MEM_CBCR_CLK_OFF_BMSK                                                     0x80000000
#define HWIO_LPASS_AUDIO_CORE_LPAIF_RXTX_WR_MEM_CBCR_CLK_OFF_SHFT                                                             31
#define HWIO_LPASS_AUDIO_CORE_LPAIF_RXTX_WR_MEM_CBCR_FORCE_MEM_CORE_ON_BMSK                                               0x4000
#define HWIO_LPASS_AUDIO_CORE_LPAIF_RXTX_WR_MEM_CBCR_FORCE_MEM_CORE_ON_SHFT                                                   14
#define HWIO_LPASS_AUDIO_CORE_LPAIF_RXTX_WR_MEM_CBCR_FORCE_MEM_PERIPH_ON_BMSK                                             0x2000
#define HWIO_LPASS_AUDIO_CORE_LPAIF_RXTX_WR_MEM_CBCR_FORCE_MEM_PERIPH_ON_SHFT                                                 13
#define HWIO_LPASS_AUDIO_CORE_LPAIF_RXTX_WR_MEM_CBCR_FORCE_MEM_PERIPH_OFF_BMSK                                            0x1000
#define HWIO_LPASS_AUDIO_CORE_LPAIF_RXTX_WR_MEM_CBCR_FORCE_MEM_PERIPH_OFF_SHFT                                                12
#define HWIO_LPASS_AUDIO_CORE_LPAIF_RXTX_WR_MEM_CBCR_WAKEUP_BMSK                                                           0xf00
#define HWIO_LPASS_AUDIO_CORE_LPAIF_RXTX_WR_MEM_CBCR_WAKEUP_SHFT                                                               8
#define HWIO_LPASS_AUDIO_CORE_LPAIF_RXTX_WR_MEM_CBCR_SLEEP_BMSK                                                             0xf0
#define HWIO_LPASS_AUDIO_CORE_LPAIF_RXTX_WR_MEM_CBCR_SLEEP_SHFT                                                                4
#define HWIO_LPASS_AUDIO_CORE_LPAIF_RXTX_WR_MEM_CBCR_HW_CTL_BMSK                                                             0x2
#define HWIO_LPASS_AUDIO_CORE_LPAIF_RXTX_WR_MEM_CBCR_HW_CTL_SHFT                                                               1
#define HWIO_LPASS_AUDIO_CORE_LPAIF_RXTX_WR_MEM_CBCR_CLK_ENABLE_BMSK                                                         0x1
#define HWIO_LPASS_AUDIO_CORE_LPAIF_RXTX_WR_MEM_CBCR_CLK_ENABLE_SHFT                                                           0

#define HWIO_LPASS_AUDIO_CORE_LPAIF_RXTX_RD_MEM_CBCR_ADDR(x)                                                          ((x) + 0x40004)
#define HWIO_LPASS_AUDIO_CORE_LPAIF_RXTX_RD_MEM_CBCR_RMSK                                                             0x80007ff3
#define HWIO_LPASS_AUDIO_CORE_LPAIF_RXTX_RD_MEM_CBCR_IN(x)            \
                in_dword(HWIO_LPASS_AUDIO_CORE_LPAIF_RXTX_RD_MEM_CBCR_ADDR(x))
#define HWIO_LPASS_AUDIO_CORE_LPAIF_RXTX_RD_MEM_CBCR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AUDIO_CORE_LPAIF_RXTX_RD_MEM_CBCR_ADDR(x), m)
#define HWIO_LPASS_AUDIO_CORE_LPAIF_RXTX_RD_MEM_CBCR_OUT(x, v)            \
                out_dword(HWIO_LPASS_AUDIO_CORE_LPAIF_RXTX_RD_MEM_CBCR_ADDR(x),v)
#define HWIO_LPASS_AUDIO_CORE_LPAIF_RXTX_RD_MEM_CBCR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AUDIO_CORE_LPAIF_RXTX_RD_MEM_CBCR_ADDR(x),m,v,HWIO_LPASS_AUDIO_CORE_LPAIF_RXTX_RD_MEM_CBCR_IN(x))
#define HWIO_LPASS_AUDIO_CORE_LPAIF_RXTX_RD_MEM_CBCR_CLK_OFF_BMSK                                                     0x80000000
#define HWIO_LPASS_AUDIO_CORE_LPAIF_RXTX_RD_MEM_CBCR_CLK_OFF_SHFT                                                             31
#define HWIO_LPASS_AUDIO_CORE_LPAIF_RXTX_RD_MEM_CBCR_FORCE_MEM_CORE_ON_BMSK                                               0x4000
#define HWIO_LPASS_AUDIO_CORE_LPAIF_RXTX_RD_MEM_CBCR_FORCE_MEM_CORE_ON_SHFT                                                   14
#define HWIO_LPASS_AUDIO_CORE_LPAIF_RXTX_RD_MEM_CBCR_FORCE_MEM_PERIPH_ON_BMSK                                             0x2000
#define HWIO_LPASS_AUDIO_CORE_LPAIF_RXTX_RD_MEM_CBCR_FORCE_MEM_PERIPH_ON_SHFT                                                 13
#define HWIO_LPASS_AUDIO_CORE_LPAIF_RXTX_RD_MEM_CBCR_FORCE_MEM_PERIPH_OFF_BMSK                                            0x1000
#define HWIO_LPASS_AUDIO_CORE_LPAIF_RXTX_RD_MEM_CBCR_FORCE_MEM_PERIPH_OFF_SHFT                                                12
#define HWIO_LPASS_AUDIO_CORE_LPAIF_RXTX_RD_MEM_CBCR_WAKEUP_BMSK                                                           0xf00
#define HWIO_LPASS_AUDIO_CORE_LPAIF_RXTX_RD_MEM_CBCR_WAKEUP_SHFT                                                               8
#define HWIO_LPASS_AUDIO_CORE_LPAIF_RXTX_RD_MEM_CBCR_SLEEP_BMSK                                                             0xf0
#define HWIO_LPASS_AUDIO_CORE_LPAIF_RXTX_RD_MEM_CBCR_SLEEP_SHFT                                                                4
#define HWIO_LPASS_AUDIO_CORE_LPAIF_RXTX_RD_MEM_CBCR_HW_CTL_BMSK                                                             0x2
#define HWIO_LPASS_AUDIO_CORE_LPAIF_RXTX_RD_MEM_CBCR_HW_CTL_SHFT                                                               1
#define HWIO_LPASS_AUDIO_CORE_LPAIF_RXTX_RD_MEM_CBCR_CLK_ENABLE_BMSK                                                         0x1
#define HWIO_LPASS_AUDIO_CORE_LPAIF_RXTX_RD_MEM_CBCR_CLK_ENABLE_SHFT                                                           0

#define HWIO_LPASS_VA_CMD_RCGR_ADDR(x)                                                                                ((x) + 0x42000)
#define HWIO_LPASS_VA_CMD_RCGR_RMSK                                                                                   0x800000f3
#define HWIO_LPASS_VA_CMD_RCGR_IN(x)            \
                in_dword(HWIO_LPASS_VA_CMD_RCGR_ADDR(x))
#define HWIO_LPASS_VA_CMD_RCGR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_VA_CMD_RCGR_ADDR(x), m)
#define HWIO_LPASS_VA_CMD_RCGR_OUT(x, v)            \
                out_dword(HWIO_LPASS_VA_CMD_RCGR_ADDR(x),v)
#define HWIO_LPASS_VA_CMD_RCGR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_VA_CMD_RCGR_ADDR(x),m,v,HWIO_LPASS_VA_CMD_RCGR_IN(x))
#define HWIO_LPASS_VA_CMD_RCGR_ROOT_OFF_BMSK                                                                          0x80000000
#define HWIO_LPASS_VA_CMD_RCGR_ROOT_OFF_SHFT                                                                                  31
#define HWIO_LPASS_VA_CMD_RCGR_DIRTY_D_BMSK                                                                                 0x80
#define HWIO_LPASS_VA_CMD_RCGR_DIRTY_D_SHFT                                                                                    7
#define HWIO_LPASS_VA_CMD_RCGR_DIRTY_N_BMSK                                                                                 0x40
#define HWIO_LPASS_VA_CMD_RCGR_DIRTY_N_SHFT                                                                                    6
#define HWIO_LPASS_VA_CMD_RCGR_DIRTY_M_BMSK                                                                                 0x20
#define HWIO_LPASS_VA_CMD_RCGR_DIRTY_M_SHFT                                                                                    5
#define HWIO_LPASS_VA_CMD_RCGR_DIRTY_CFG_RCGR_BMSK                                                                          0x10
#define HWIO_LPASS_VA_CMD_RCGR_DIRTY_CFG_RCGR_SHFT                                                                             4
#define HWIO_LPASS_VA_CMD_RCGR_ROOT_EN_BMSK                                                                                  0x2
#define HWIO_LPASS_VA_CMD_RCGR_ROOT_EN_SHFT                                                                                    1
#define HWIO_LPASS_VA_CMD_RCGR_UPDATE_BMSK                                                                                   0x1
#define HWIO_LPASS_VA_CMD_RCGR_UPDATE_SHFT                                                                                     0

#define HWIO_LPASS_VA_CFG_RCGR_ADDR(x)                                                                                ((x) + 0x42004)
#define HWIO_LPASS_VA_CFG_RCGR_RMSK                                                                                       0x771f
#define HWIO_LPASS_VA_CFG_RCGR_IN(x)            \
                in_dword(HWIO_LPASS_VA_CFG_RCGR_ADDR(x))
#define HWIO_LPASS_VA_CFG_RCGR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_VA_CFG_RCGR_ADDR(x), m)
#define HWIO_LPASS_VA_CFG_RCGR_OUT(x, v)            \
                out_dword(HWIO_LPASS_VA_CFG_RCGR_ADDR(x),v)
#define HWIO_LPASS_VA_CFG_RCGR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_VA_CFG_RCGR_ADDR(x),m,v,HWIO_LPASS_VA_CFG_RCGR_IN(x))
#define HWIO_LPASS_VA_CFG_RCGR_ALT_SRC_SEL_BMSK                                                                           0x4000
#define HWIO_LPASS_VA_CFG_RCGR_ALT_SRC_SEL_SHFT                                                                               14
#define HWIO_LPASS_VA_CFG_RCGR_MODE_BMSK                                                                                  0x3000
#define HWIO_LPASS_VA_CFG_RCGR_MODE_SHFT                                                                                      12
#define HWIO_LPASS_VA_CFG_RCGR_SRC_SEL_BMSK                                                                                0x700
#define HWIO_LPASS_VA_CFG_RCGR_SRC_SEL_SHFT                                                                                    8
#define HWIO_LPASS_VA_CFG_RCGR_SRC_DIV_BMSK                                                                                 0x1f
#define HWIO_LPASS_VA_CFG_RCGR_SRC_DIV_SHFT                                                                                    0

#define HWIO_LPASS_VA_M_ADDR(x)                                                                                       ((x) + 0x42008)
#define HWIO_LPASS_VA_M_RMSK                                                                                                0xff
#define HWIO_LPASS_VA_M_IN(x)            \
                in_dword(HWIO_LPASS_VA_M_ADDR(x))
#define HWIO_LPASS_VA_M_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_VA_M_ADDR(x), m)
#define HWIO_LPASS_VA_M_OUT(x, v)            \
                out_dword(HWIO_LPASS_VA_M_ADDR(x),v)
#define HWIO_LPASS_VA_M_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_VA_M_ADDR(x),m,v,HWIO_LPASS_VA_M_IN(x))
#define HWIO_LPASS_VA_M_M_BMSK                                                                                              0xff
#define HWIO_LPASS_VA_M_M_SHFT                                                                                                 0

#define HWIO_LPASS_VA_N_ADDR(x)                                                                                       ((x) + 0x4200c)
#define HWIO_LPASS_VA_N_RMSK                                                                                                0xff
#define HWIO_LPASS_VA_N_IN(x)            \
                in_dword(HWIO_LPASS_VA_N_ADDR(x))
#define HWIO_LPASS_VA_N_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_VA_N_ADDR(x), m)
#define HWIO_LPASS_VA_N_OUT(x, v)            \
                out_dword(HWIO_LPASS_VA_N_ADDR(x),v)
#define HWIO_LPASS_VA_N_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_VA_N_ADDR(x),m,v,HWIO_LPASS_VA_N_IN(x))
#define HWIO_LPASS_VA_N_NOT_N_MINUS_M_BMSK                                                                                  0xff
#define HWIO_LPASS_VA_N_NOT_N_MINUS_M_SHFT                                                                                     0

#define HWIO_LPASS_VA_D_ADDR(x)                                                                                       ((x) + 0x42010)
#define HWIO_LPASS_VA_D_RMSK                                                                                                0xff
#define HWIO_LPASS_VA_D_IN(x)            \
                in_dword(HWIO_LPASS_VA_D_ADDR(x))
#define HWIO_LPASS_VA_D_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_VA_D_ADDR(x), m)
#define HWIO_LPASS_VA_D_OUT(x, v)            \
                out_dword(HWIO_LPASS_VA_D_ADDR(x),v)
#define HWIO_LPASS_VA_D_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_VA_D_ADDR(x),m,v,HWIO_LPASS_VA_D_IN(x))
#define HWIO_LPASS_VA_D_NOT_2D_BMSK                                                                                         0xff
#define HWIO_LPASS_VA_D_NOT_2D_SHFT                                                                                            0

#define HWIO_LPASS_AUDIO_CORE_VA_2X_CBCR_ADDR(x)                                                                      ((x) + 0x42014)
#define HWIO_LPASS_AUDIO_CORE_VA_2X_CBCR_RMSK                                                                         0x80000003
#define HWIO_LPASS_AUDIO_CORE_VA_2X_CBCR_IN(x)            \
                in_dword(HWIO_LPASS_AUDIO_CORE_VA_2X_CBCR_ADDR(x))
#define HWIO_LPASS_AUDIO_CORE_VA_2X_CBCR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AUDIO_CORE_VA_2X_CBCR_ADDR(x), m)
#define HWIO_LPASS_AUDIO_CORE_VA_2X_CBCR_OUT(x, v)            \
                out_dword(HWIO_LPASS_AUDIO_CORE_VA_2X_CBCR_ADDR(x),v)
#define HWIO_LPASS_AUDIO_CORE_VA_2X_CBCR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AUDIO_CORE_VA_2X_CBCR_ADDR(x),m,v,HWIO_LPASS_AUDIO_CORE_VA_2X_CBCR_IN(x))
#define HWIO_LPASS_AUDIO_CORE_VA_2X_CBCR_CLK_OFF_BMSK                                                                 0x80000000
#define HWIO_LPASS_AUDIO_CORE_VA_2X_CBCR_CLK_OFF_SHFT                                                                         31
#define HWIO_LPASS_AUDIO_CORE_VA_2X_CBCR_HW_CTL_BMSK                                                                         0x2
#define HWIO_LPASS_AUDIO_CORE_VA_2X_CBCR_HW_CTL_SHFT                                                                           1
#define HWIO_LPASS_AUDIO_CORE_VA_2X_CBCR_CLK_ENABLE_BMSK                                                                     0x1
#define HWIO_LPASS_AUDIO_CORE_VA_2X_CBCR_CLK_ENABLE_SHFT                                                                       0

#define HWIO_LPASS_CDIV_VA_DIV_CDIVR_ADDR(x)                                                                          ((x) + 0x42018)
#define HWIO_LPASS_CDIV_VA_DIV_CDIVR_RMSK                                                                                    0xf
#define HWIO_LPASS_CDIV_VA_DIV_CDIVR_IN(x)            \
                in_dword(HWIO_LPASS_CDIV_VA_DIV_CDIVR_ADDR(x))
#define HWIO_LPASS_CDIV_VA_DIV_CDIVR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_CDIV_VA_DIV_CDIVR_ADDR(x), m)
#define HWIO_LPASS_CDIV_VA_DIV_CDIVR_OUT(x, v)            \
                out_dword(HWIO_LPASS_CDIV_VA_DIV_CDIVR_ADDR(x),v)
#define HWIO_LPASS_CDIV_VA_DIV_CDIVR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_CDIV_VA_DIV_CDIVR_ADDR(x),m,v,HWIO_LPASS_CDIV_VA_DIV_CDIVR_IN(x))
#define HWIO_LPASS_CDIV_VA_DIV_CDIVR_CLK_DIV_BMSK                                                                            0xf
#define HWIO_LPASS_CDIV_VA_DIV_CDIVR_CLK_DIV_SHFT                                                                              0

#define HWIO_LPASS_AUDIO_CORE_VA_CBCR_ADDR(x)                                                                         ((x) + 0x4201c)
#define HWIO_LPASS_AUDIO_CORE_VA_CBCR_RMSK                                                                            0x80000003
#define HWIO_LPASS_AUDIO_CORE_VA_CBCR_IN(x)            \
                in_dword(HWIO_LPASS_AUDIO_CORE_VA_CBCR_ADDR(x))
#define HWIO_LPASS_AUDIO_CORE_VA_CBCR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AUDIO_CORE_VA_CBCR_ADDR(x), m)
#define HWIO_LPASS_AUDIO_CORE_VA_CBCR_OUT(x, v)            \
                out_dword(HWIO_LPASS_AUDIO_CORE_VA_CBCR_ADDR(x),v)
#define HWIO_LPASS_AUDIO_CORE_VA_CBCR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AUDIO_CORE_VA_CBCR_ADDR(x),m,v,HWIO_LPASS_AUDIO_CORE_VA_CBCR_IN(x))
#define HWIO_LPASS_AUDIO_CORE_VA_CBCR_CLK_OFF_BMSK                                                                    0x80000000
#define HWIO_LPASS_AUDIO_CORE_VA_CBCR_CLK_OFF_SHFT                                                                            31
#define HWIO_LPASS_AUDIO_CORE_VA_CBCR_HW_CTL_BMSK                                                                            0x2
#define HWIO_LPASS_AUDIO_CORE_VA_CBCR_HW_CTL_SHFT                                                                              1
#define HWIO_LPASS_AUDIO_CORE_VA_CBCR_CLK_ENABLE_BMSK                                                                        0x1
#define HWIO_LPASS_AUDIO_CORE_VA_CBCR_CLK_ENABLE_SHFT                                                                          0

#define HWIO_LPASS_TX_MCLK_CMD_RCGR_ADDR(x)                                                                           ((x) + 0x43000)
#define HWIO_LPASS_TX_MCLK_CMD_RCGR_RMSK                                                                              0x800000f3
#define HWIO_LPASS_TX_MCLK_CMD_RCGR_IN(x)            \
                in_dword(HWIO_LPASS_TX_MCLK_CMD_RCGR_ADDR(x))
#define HWIO_LPASS_TX_MCLK_CMD_RCGR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_TX_MCLK_CMD_RCGR_ADDR(x), m)
#define HWIO_LPASS_TX_MCLK_CMD_RCGR_OUT(x, v)            \
                out_dword(HWIO_LPASS_TX_MCLK_CMD_RCGR_ADDR(x),v)
#define HWIO_LPASS_TX_MCLK_CMD_RCGR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_TX_MCLK_CMD_RCGR_ADDR(x),m,v,HWIO_LPASS_TX_MCLK_CMD_RCGR_IN(x))
#define HWIO_LPASS_TX_MCLK_CMD_RCGR_ROOT_OFF_BMSK                                                                     0x80000000
#define HWIO_LPASS_TX_MCLK_CMD_RCGR_ROOT_OFF_SHFT                                                                             31
#define HWIO_LPASS_TX_MCLK_CMD_RCGR_DIRTY_D_BMSK                                                                            0x80
#define HWIO_LPASS_TX_MCLK_CMD_RCGR_DIRTY_D_SHFT                                                                               7
#define HWIO_LPASS_TX_MCLK_CMD_RCGR_DIRTY_N_BMSK                                                                            0x40
#define HWIO_LPASS_TX_MCLK_CMD_RCGR_DIRTY_N_SHFT                                                                               6
#define HWIO_LPASS_TX_MCLK_CMD_RCGR_DIRTY_M_BMSK                                                                            0x20
#define HWIO_LPASS_TX_MCLK_CMD_RCGR_DIRTY_M_SHFT                                                                               5
#define HWIO_LPASS_TX_MCLK_CMD_RCGR_DIRTY_CFG_RCGR_BMSK                                                                     0x10
#define HWIO_LPASS_TX_MCLK_CMD_RCGR_DIRTY_CFG_RCGR_SHFT                                                                        4
#define HWIO_LPASS_TX_MCLK_CMD_RCGR_ROOT_EN_BMSK                                                                             0x2
#define HWIO_LPASS_TX_MCLK_CMD_RCGR_ROOT_EN_SHFT                                                                               1
#define HWIO_LPASS_TX_MCLK_CMD_RCGR_UPDATE_BMSK                                                                              0x1
#define HWIO_LPASS_TX_MCLK_CMD_RCGR_UPDATE_SHFT                                                                                0

#define HWIO_LPASS_TX_MCLK_CFG_RCGR_ADDR(x)                                                                           ((x) + 0x43004)
#define HWIO_LPASS_TX_MCLK_CFG_RCGR_RMSK                                                                                  0x771f
#define HWIO_LPASS_TX_MCLK_CFG_RCGR_IN(x)            \
                in_dword(HWIO_LPASS_TX_MCLK_CFG_RCGR_ADDR(x))
#define HWIO_LPASS_TX_MCLK_CFG_RCGR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_TX_MCLK_CFG_RCGR_ADDR(x), m)
#define HWIO_LPASS_TX_MCLK_CFG_RCGR_OUT(x, v)            \
                out_dword(HWIO_LPASS_TX_MCLK_CFG_RCGR_ADDR(x),v)
#define HWIO_LPASS_TX_MCLK_CFG_RCGR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_TX_MCLK_CFG_RCGR_ADDR(x),m,v,HWIO_LPASS_TX_MCLK_CFG_RCGR_IN(x))
#define HWIO_LPASS_TX_MCLK_CFG_RCGR_ALT_SRC_SEL_BMSK                                                                      0x4000
#define HWIO_LPASS_TX_MCLK_CFG_RCGR_ALT_SRC_SEL_SHFT                                                                          14
#define HWIO_LPASS_TX_MCLK_CFG_RCGR_MODE_BMSK                                                                             0x3000
#define HWIO_LPASS_TX_MCLK_CFG_RCGR_MODE_SHFT                                                                                 12
#define HWIO_LPASS_TX_MCLK_CFG_RCGR_SRC_SEL_BMSK                                                                           0x700
#define HWIO_LPASS_TX_MCLK_CFG_RCGR_SRC_SEL_SHFT                                                                               8
#define HWIO_LPASS_TX_MCLK_CFG_RCGR_SRC_DIV_BMSK                                                                            0x1f
#define HWIO_LPASS_TX_MCLK_CFG_RCGR_SRC_DIV_SHFT                                                                               0

#define HWIO_LPASS_TX_MCLK_M_ADDR(x)                                                                                  ((x) + 0x43008)
#define HWIO_LPASS_TX_MCLK_M_RMSK                                                                                           0xff
#define HWIO_LPASS_TX_MCLK_M_IN(x)            \
                in_dword(HWIO_LPASS_TX_MCLK_M_ADDR(x))
#define HWIO_LPASS_TX_MCLK_M_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_TX_MCLK_M_ADDR(x), m)
#define HWIO_LPASS_TX_MCLK_M_OUT(x, v)            \
                out_dword(HWIO_LPASS_TX_MCLK_M_ADDR(x),v)
#define HWIO_LPASS_TX_MCLK_M_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_TX_MCLK_M_ADDR(x),m,v,HWIO_LPASS_TX_MCLK_M_IN(x))
#define HWIO_LPASS_TX_MCLK_M_M_BMSK                                                                                         0xff
#define HWIO_LPASS_TX_MCLK_M_M_SHFT                                                                                            0

#define HWIO_LPASS_TX_MCLK_N_ADDR(x)                                                                                  ((x) + 0x4300c)
#define HWIO_LPASS_TX_MCLK_N_RMSK                                                                                           0xff
#define HWIO_LPASS_TX_MCLK_N_IN(x)            \
                in_dword(HWIO_LPASS_TX_MCLK_N_ADDR(x))
#define HWIO_LPASS_TX_MCLK_N_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_TX_MCLK_N_ADDR(x), m)
#define HWIO_LPASS_TX_MCLK_N_OUT(x, v)            \
                out_dword(HWIO_LPASS_TX_MCLK_N_ADDR(x),v)
#define HWIO_LPASS_TX_MCLK_N_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_TX_MCLK_N_ADDR(x),m,v,HWIO_LPASS_TX_MCLK_N_IN(x))
#define HWIO_LPASS_TX_MCLK_N_NOT_N_MINUS_M_BMSK                                                                             0xff
#define HWIO_LPASS_TX_MCLK_N_NOT_N_MINUS_M_SHFT                                                                                0

#define HWIO_LPASS_TX_MCLK_D_ADDR(x)                                                                                  ((x) + 0x43010)
#define HWIO_LPASS_TX_MCLK_D_RMSK                                                                                           0xff
#define HWIO_LPASS_TX_MCLK_D_IN(x)            \
                in_dword(HWIO_LPASS_TX_MCLK_D_ADDR(x))
#define HWIO_LPASS_TX_MCLK_D_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_TX_MCLK_D_ADDR(x), m)
#define HWIO_LPASS_TX_MCLK_D_OUT(x, v)            \
                out_dword(HWIO_LPASS_TX_MCLK_D_ADDR(x),v)
#define HWIO_LPASS_TX_MCLK_D_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_TX_MCLK_D_ADDR(x),m,v,HWIO_LPASS_TX_MCLK_D_IN(x))
#define HWIO_LPASS_TX_MCLK_D_NOT_2D_BMSK                                                                                    0xff
#define HWIO_LPASS_TX_MCLK_D_NOT_2D_SHFT                                                                                       0

#define HWIO_LPASS_AUDIO_CORE_TX_MCLK_2X_CBCR_ADDR(x)                                                                 ((x) + 0x43014)
#define HWIO_LPASS_AUDIO_CORE_TX_MCLK_2X_CBCR_RMSK                                                                    0x80000003
#define HWIO_LPASS_AUDIO_CORE_TX_MCLK_2X_CBCR_IN(x)            \
                in_dword(HWIO_LPASS_AUDIO_CORE_TX_MCLK_2X_CBCR_ADDR(x))
#define HWIO_LPASS_AUDIO_CORE_TX_MCLK_2X_CBCR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AUDIO_CORE_TX_MCLK_2X_CBCR_ADDR(x), m)
#define HWIO_LPASS_AUDIO_CORE_TX_MCLK_2X_CBCR_OUT(x, v)            \
                out_dword(HWIO_LPASS_AUDIO_CORE_TX_MCLK_2X_CBCR_ADDR(x),v)
#define HWIO_LPASS_AUDIO_CORE_TX_MCLK_2X_CBCR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AUDIO_CORE_TX_MCLK_2X_CBCR_ADDR(x),m,v,HWIO_LPASS_AUDIO_CORE_TX_MCLK_2X_CBCR_IN(x))
#define HWIO_LPASS_AUDIO_CORE_TX_MCLK_2X_CBCR_CLK_OFF_BMSK                                                            0x80000000
#define HWIO_LPASS_AUDIO_CORE_TX_MCLK_2X_CBCR_CLK_OFF_SHFT                                                                    31
#define HWIO_LPASS_AUDIO_CORE_TX_MCLK_2X_CBCR_HW_CTL_BMSK                                                                    0x2
#define HWIO_LPASS_AUDIO_CORE_TX_MCLK_2X_CBCR_HW_CTL_SHFT                                                                      1
#define HWIO_LPASS_AUDIO_CORE_TX_MCLK_2X_CBCR_CLK_ENABLE_BMSK                                                                0x1
#define HWIO_LPASS_AUDIO_CORE_TX_MCLK_2X_CBCR_CLK_ENABLE_SHFT                                                                  0

#define HWIO_LPASS_CDIV_TX_MCLK_DIV_CDIVR_ADDR(x)                                                                     ((x) + 0x43018)
#define HWIO_LPASS_CDIV_TX_MCLK_DIV_CDIVR_RMSK                                                                               0xf
#define HWIO_LPASS_CDIV_TX_MCLK_DIV_CDIVR_IN(x)            \
                in_dword(HWIO_LPASS_CDIV_TX_MCLK_DIV_CDIVR_ADDR(x))
#define HWIO_LPASS_CDIV_TX_MCLK_DIV_CDIVR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_CDIV_TX_MCLK_DIV_CDIVR_ADDR(x), m)
#define HWIO_LPASS_CDIV_TX_MCLK_DIV_CDIVR_OUT(x, v)            \
                out_dword(HWIO_LPASS_CDIV_TX_MCLK_DIV_CDIVR_ADDR(x),v)
#define HWIO_LPASS_CDIV_TX_MCLK_DIV_CDIVR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_CDIV_TX_MCLK_DIV_CDIVR_ADDR(x),m,v,HWIO_LPASS_CDIV_TX_MCLK_DIV_CDIVR_IN(x))
#define HWIO_LPASS_CDIV_TX_MCLK_DIV_CDIVR_CLK_DIV_BMSK                                                                       0xf
#define HWIO_LPASS_CDIV_TX_MCLK_DIV_CDIVR_CLK_DIV_SHFT                                                                         0

#define HWIO_LPASS_AUDIO_CORE_TX_MCLK_CBCR_ADDR(x)                                                                    ((x) + 0x4301c)
#define HWIO_LPASS_AUDIO_CORE_TX_MCLK_CBCR_RMSK                                                                       0x80000003
#define HWIO_LPASS_AUDIO_CORE_TX_MCLK_CBCR_IN(x)            \
                in_dword(HWIO_LPASS_AUDIO_CORE_TX_MCLK_CBCR_ADDR(x))
#define HWIO_LPASS_AUDIO_CORE_TX_MCLK_CBCR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AUDIO_CORE_TX_MCLK_CBCR_ADDR(x), m)
#define HWIO_LPASS_AUDIO_CORE_TX_MCLK_CBCR_OUT(x, v)            \
                out_dword(HWIO_LPASS_AUDIO_CORE_TX_MCLK_CBCR_ADDR(x),v)
#define HWIO_LPASS_AUDIO_CORE_TX_MCLK_CBCR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AUDIO_CORE_TX_MCLK_CBCR_ADDR(x),m,v,HWIO_LPASS_AUDIO_CORE_TX_MCLK_CBCR_IN(x))
#define HWIO_LPASS_AUDIO_CORE_TX_MCLK_CBCR_CLK_OFF_BMSK                                                               0x80000000
#define HWIO_LPASS_AUDIO_CORE_TX_MCLK_CBCR_CLK_OFF_SHFT                                                                       31
#define HWIO_LPASS_AUDIO_CORE_TX_MCLK_CBCR_HW_CTL_BMSK                                                                       0x2
#define HWIO_LPASS_AUDIO_CORE_TX_MCLK_CBCR_HW_CTL_SHFT                                                                         1
#define HWIO_LPASS_AUDIO_CORE_TX_MCLK_CBCR_CLK_ENABLE_BMSK                                                                   0x1
#define HWIO_LPASS_AUDIO_CORE_TX_MCLK_CBCR_CLK_ENABLE_SHFT                                                                     0

#define HWIO_LPASS_TX_MCLK_MODE_MUXSEL_ADDR(x)                                                                        ((x) + 0x43020)
#define HWIO_LPASS_TX_MCLK_MODE_MUXSEL_RMSK                                                                                  0x1
#define HWIO_LPASS_TX_MCLK_MODE_MUXSEL_IN(x)            \
                in_dword(HWIO_LPASS_TX_MCLK_MODE_MUXSEL_ADDR(x))
#define HWIO_LPASS_TX_MCLK_MODE_MUXSEL_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_TX_MCLK_MODE_MUXSEL_ADDR(x), m)
#define HWIO_LPASS_TX_MCLK_MODE_MUXSEL_OUT(x, v)            \
                out_dword(HWIO_LPASS_TX_MCLK_MODE_MUXSEL_ADDR(x),v)
#define HWIO_LPASS_TX_MCLK_MODE_MUXSEL_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_TX_MCLK_MODE_MUXSEL_ADDR(x),m,v,HWIO_LPASS_TX_MCLK_MODE_MUXSEL_IN(x))
#define HWIO_LPASS_TX_MCLK_MODE_MUXSEL_SEL_BMSK                                                                              0x1
#define HWIO_LPASS_TX_MCLK_MODE_MUXSEL_SEL_SHFT                                                                                0

#define HWIO_LPASS_WSA_MCLK_CMD_RCGR_ADDR(x)                                                                          ((x) + 0x44000)
#define HWIO_LPASS_WSA_MCLK_CMD_RCGR_RMSK                                                                             0x800000f3
#define HWIO_LPASS_WSA_MCLK_CMD_RCGR_IN(x)            \
                in_dword(HWIO_LPASS_WSA_MCLK_CMD_RCGR_ADDR(x))
#define HWIO_LPASS_WSA_MCLK_CMD_RCGR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_WSA_MCLK_CMD_RCGR_ADDR(x), m)
#define HWIO_LPASS_WSA_MCLK_CMD_RCGR_OUT(x, v)            \
                out_dword(HWIO_LPASS_WSA_MCLK_CMD_RCGR_ADDR(x),v)
#define HWIO_LPASS_WSA_MCLK_CMD_RCGR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_WSA_MCLK_CMD_RCGR_ADDR(x),m,v,HWIO_LPASS_WSA_MCLK_CMD_RCGR_IN(x))
#define HWIO_LPASS_WSA_MCLK_CMD_RCGR_ROOT_OFF_BMSK                                                                    0x80000000
#define HWIO_LPASS_WSA_MCLK_CMD_RCGR_ROOT_OFF_SHFT                                                                            31
#define HWIO_LPASS_WSA_MCLK_CMD_RCGR_DIRTY_D_BMSK                                                                           0x80
#define HWIO_LPASS_WSA_MCLK_CMD_RCGR_DIRTY_D_SHFT                                                                              7
#define HWIO_LPASS_WSA_MCLK_CMD_RCGR_DIRTY_N_BMSK                                                                           0x40
#define HWIO_LPASS_WSA_MCLK_CMD_RCGR_DIRTY_N_SHFT                                                                              6
#define HWIO_LPASS_WSA_MCLK_CMD_RCGR_DIRTY_M_BMSK                                                                           0x20
#define HWIO_LPASS_WSA_MCLK_CMD_RCGR_DIRTY_M_SHFT                                                                              5
#define HWIO_LPASS_WSA_MCLK_CMD_RCGR_DIRTY_CFG_RCGR_BMSK                                                                    0x10
#define HWIO_LPASS_WSA_MCLK_CMD_RCGR_DIRTY_CFG_RCGR_SHFT                                                                       4
#define HWIO_LPASS_WSA_MCLK_CMD_RCGR_ROOT_EN_BMSK                                                                            0x2
#define HWIO_LPASS_WSA_MCLK_CMD_RCGR_ROOT_EN_SHFT                                                                              1
#define HWIO_LPASS_WSA_MCLK_CMD_RCGR_UPDATE_BMSK                                                                             0x1
#define HWIO_LPASS_WSA_MCLK_CMD_RCGR_UPDATE_SHFT                                                                               0

#define HWIO_LPASS_WSA_MCLK_CFG_RCGR_ADDR(x)                                                                          ((x) + 0x44004)
#define HWIO_LPASS_WSA_MCLK_CFG_RCGR_RMSK                                                                                 0x771f
#define HWIO_LPASS_WSA_MCLK_CFG_RCGR_IN(x)            \
                in_dword(HWIO_LPASS_WSA_MCLK_CFG_RCGR_ADDR(x))
#define HWIO_LPASS_WSA_MCLK_CFG_RCGR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_WSA_MCLK_CFG_RCGR_ADDR(x), m)
#define HWIO_LPASS_WSA_MCLK_CFG_RCGR_OUT(x, v)            \
                out_dword(HWIO_LPASS_WSA_MCLK_CFG_RCGR_ADDR(x),v)
#define HWIO_LPASS_WSA_MCLK_CFG_RCGR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_WSA_MCLK_CFG_RCGR_ADDR(x),m,v,HWIO_LPASS_WSA_MCLK_CFG_RCGR_IN(x))
#define HWIO_LPASS_WSA_MCLK_CFG_RCGR_ALT_SRC_SEL_BMSK                                                                     0x4000
#define HWIO_LPASS_WSA_MCLK_CFG_RCGR_ALT_SRC_SEL_SHFT                                                                         14
#define HWIO_LPASS_WSA_MCLK_CFG_RCGR_MODE_BMSK                                                                            0x3000
#define HWIO_LPASS_WSA_MCLK_CFG_RCGR_MODE_SHFT                                                                                12
#define HWIO_LPASS_WSA_MCLK_CFG_RCGR_SRC_SEL_BMSK                                                                          0x700
#define HWIO_LPASS_WSA_MCLK_CFG_RCGR_SRC_SEL_SHFT                                                                              8
#define HWIO_LPASS_WSA_MCLK_CFG_RCGR_SRC_DIV_BMSK                                                                           0x1f
#define HWIO_LPASS_WSA_MCLK_CFG_RCGR_SRC_DIV_SHFT                                                                              0

#define HWIO_LPASS_WSA_MCLK_M_ADDR(x)                                                                                 ((x) + 0x44008)
#define HWIO_LPASS_WSA_MCLK_M_RMSK                                                                                          0xff
#define HWIO_LPASS_WSA_MCLK_M_IN(x)            \
                in_dword(HWIO_LPASS_WSA_MCLK_M_ADDR(x))
#define HWIO_LPASS_WSA_MCLK_M_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_WSA_MCLK_M_ADDR(x), m)
#define HWIO_LPASS_WSA_MCLK_M_OUT(x, v)            \
                out_dword(HWIO_LPASS_WSA_MCLK_M_ADDR(x),v)
#define HWIO_LPASS_WSA_MCLK_M_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_WSA_MCLK_M_ADDR(x),m,v,HWIO_LPASS_WSA_MCLK_M_IN(x))
#define HWIO_LPASS_WSA_MCLK_M_M_BMSK                                                                                        0xff
#define HWIO_LPASS_WSA_MCLK_M_M_SHFT                                                                                           0

#define HWIO_LPASS_WSA_MCLK_N_ADDR(x)                                                                                 ((x) + 0x4400c)
#define HWIO_LPASS_WSA_MCLK_N_RMSK                                                                                          0xff
#define HWIO_LPASS_WSA_MCLK_N_IN(x)            \
                in_dword(HWIO_LPASS_WSA_MCLK_N_ADDR(x))
#define HWIO_LPASS_WSA_MCLK_N_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_WSA_MCLK_N_ADDR(x), m)
#define HWIO_LPASS_WSA_MCLK_N_OUT(x, v)            \
                out_dword(HWIO_LPASS_WSA_MCLK_N_ADDR(x),v)
#define HWIO_LPASS_WSA_MCLK_N_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_WSA_MCLK_N_ADDR(x),m,v,HWIO_LPASS_WSA_MCLK_N_IN(x))
#define HWIO_LPASS_WSA_MCLK_N_NOT_N_MINUS_M_BMSK                                                                            0xff
#define HWIO_LPASS_WSA_MCLK_N_NOT_N_MINUS_M_SHFT                                                                               0

#define HWIO_LPASS_WSA_MCLK_D_ADDR(x)                                                                                 ((x) + 0x44010)
#define HWIO_LPASS_WSA_MCLK_D_RMSK                                                                                          0xff
#define HWIO_LPASS_WSA_MCLK_D_IN(x)            \
                in_dword(HWIO_LPASS_WSA_MCLK_D_ADDR(x))
#define HWIO_LPASS_WSA_MCLK_D_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_WSA_MCLK_D_ADDR(x), m)
#define HWIO_LPASS_WSA_MCLK_D_OUT(x, v)            \
                out_dword(HWIO_LPASS_WSA_MCLK_D_ADDR(x),v)
#define HWIO_LPASS_WSA_MCLK_D_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_WSA_MCLK_D_ADDR(x),m,v,HWIO_LPASS_WSA_MCLK_D_IN(x))
#define HWIO_LPASS_WSA_MCLK_D_NOT_2D_BMSK                                                                                   0xff
#define HWIO_LPASS_WSA_MCLK_D_NOT_2D_SHFT                                                                                      0

#define HWIO_LPASS_AUDIO_CORE_WSA_MCLK_2X_CBCR_ADDR(x)                                                                ((x) + 0x44014)
#define HWIO_LPASS_AUDIO_CORE_WSA_MCLK_2X_CBCR_RMSK                                                                   0x80000003
#define HWIO_LPASS_AUDIO_CORE_WSA_MCLK_2X_CBCR_IN(x)            \
                in_dword(HWIO_LPASS_AUDIO_CORE_WSA_MCLK_2X_CBCR_ADDR(x))
#define HWIO_LPASS_AUDIO_CORE_WSA_MCLK_2X_CBCR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AUDIO_CORE_WSA_MCLK_2X_CBCR_ADDR(x), m)
#define HWIO_LPASS_AUDIO_CORE_WSA_MCLK_2X_CBCR_OUT(x, v)            \
                out_dword(HWIO_LPASS_AUDIO_CORE_WSA_MCLK_2X_CBCR_ADDR(x),v)
#define HWIO_LPASS_AUDIO_CORE_WSA_MCLK_2X_CBCR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AUDIO_CORE_WSA_MCLK_2X_CBCR_ADDR(x),m,v,HWIO_LPASS_AUDIO_CORE_WSA_MCLK_2X_CBCR_IN(x))
#define HWIO_LPASS_AUDIO_CORE_WSA_MCLK_2X_CBCR_CLK_OFF_BMSK                                                           0x80000000
#define HWIO_LPASS_AUDIO_CORE_WSA_MCLK_2X_CBCR_CLK_OFF_SHFT                                                                   31
#define HWIO_LPASS_AUDIO_CORE_WSA_MCLK_2X_CBCR_HW_CTL_BMSK                                                                   0x2
#define HWIO_LPASS_AUDIO_CORE_WSA_MCLK_2X_CBCR_HW_CTL_SHFT                                                                     1
#define HWIO_LPASS_AUDIO_CORE_WSA_MCLK_2X_CBCR_CLK_ENABLE_BMSK                                                               0x1
#define HWIO_LPASS_AUDIO_CORE_WSA_MCLK_2X_CBCR_CLK_ENABLE_SHFT                                                                 0

#define HWIO_LPASS_CDIV_WSA_MCLK_DIV_CDIVR_ADDR(x)                                                                    ((x) + 0x44018)
#define HWIO_LPASS_CDIV_WSA_MCLK_DIV_CDIVR_RMSK                                                                              0xf
#define HWIO_LPASS_CDIV_WSA_MCLK_DIV_CDIVR_IN(x)            \
                in_dword(HWIO_LPASS_CDIV_WSA_MCLK_DIV_CDIVR_ADDR(x))
#define HWIO_LPASS_CDIV_WSA_MCLK_DIV_CDIVR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_CDIV_WSA_MCLK_DIV_CDIVR_ADDR(x), m)
#define HWIO_LPASS_CDIV_WSA_MCLK_DIV_CDIVR_OUT(x, v)            \
                out_dword(HWIO_LPASS_CDIV_WSA_MCLK_DIV_CDIVR_ADDR(x),v)
#define HWIO_LPASS_CDIV_WSA_MCLK_DIV_CDIVR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_CDIV_WSA_MCLK_DIV_CDIVR_ADDR(x),m,v,HWIO_LPASS_CDIV_WSA_MCLK_DIV_CDIVR_IN(x))
#define HWIO_LPASS_CDIV_WSA_MCLK_DIV_CDIVR_CLK_DIV_BMSK                                                                      0xf
#define HWIO_LPASS_CDIV_WSA_MCLK_DIV_CDIVR_CLK_DIV_SHFT                                                                        0

#define HWIO_LPASS_AUDIO_CORE_WSA_MCLK_CBCR_ADDR(x)                                                                   ((x) + 0x4401c)
#define HWIO_LPASS_AUDIO_CORE_WSA_MCLK_CBCR_RMSK                                                                      0x80000003
#define HWIO_LPASS_AUDIO_CORE_WSA_MCLK_CBCR_IN(x)            \
                in_dword(HWIO_LPASS_AUDIO_CORE_WSA_MCLK_CBCR_ADDR(x))
#define HWIO_LPASS_AUDIO_CORE_WSA_MCLK_CBCR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AUDIO_CORE_WSA_MCLK_CBCR_ADDR(x), m)
#define HWIO_LPASS_AUDIO_CORE_WSA_MCLK_CBCR_OUT(x, v)            \
                out_dword(HWIO_LPASS_AUDIO_CORE_WSA_MCLK_CBCR_ADDR(x),v)
#define HWIO_LPASS_AUDIO_CORE_WSA_MCLK_CBCR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AUDIO_CORE_WSA_MCLK_CBCR_ADDR(x),m,v,HWIO_LPASS_AUDIO_CORE_WSA_MCLK_CBCR_IN(x))
#define HWIO_LPASS_AUDIO_CORE_WSA_MCLK_CBCR_CLK_OFF_BMSK                                                              0x80000000
#define HWIO_LPASS_AUDIO_CORE_WSA_MCLK_CBCR_CLK_OFF_SHFT                                                                      31
#define HWIO_LPASS_AUDIO_CORE_WSA_MCLK_CBCR_HW_CTL_BMSK                                                                      0x2
#define HWIO_LPASS_AUDIO_CORE_WSA_MCLK_CBCR_HW_CTL_SHFT                                                                        1
#define HWIO_LPASS_AUDIO_CORE_WSA_MCLK_CBCR_CLK_ENABLE_BMSK                                                                  0x1
#define HWIO_LPASS_AUDIO_CORE_WSA_MCLK_CBCR_CLK_ENABLE_SHFT                                                                    0

#define HWIO_LPASS_WSA_MCLK_MODE_MUXSEL_ADDR(x)                                                                       ((x) + 0x44020)
#define HWIO_LPASS_WSA_MCLK_MODE_MUXSEL_RMSK                                                                                 0x1
#define HWIO_LPASS_WSA_MCLK_MODE_MUXSEL_IN(x)            \
                in_dword(HWIO_LPASS_WSA_MCLK_MODE_MUXSEL_ADDR(x))
#define HWIO_LPASS_WSA_MCLK_MODE_MUXSEL_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_WSA_MCLK_MODE_MUXSEL_ADDR(x), m)
#define HWIO_LPASS_WSA_MCLK_MODE_MUXSEL_OUT(x, v)            \
                out_dword(HWIO_LPASS_WSA_MCLK_MODE_MUXSEL_ADDR(x),v)
#define HWIO_LPASS_WSA_MCLK_MODE_MUXSEL_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_WSA_MCLK_MODE_MUXSEL_ADDR(x),m,v,HWIO_LPASS_WSA_MCLK_MODE_MUXSEL_IN(x))
#define HWIO_LPASS_WSA_MCLK_MODE_MUXSEL_SEL_BMSK                                                                             0x1
#define HWIO_LPASS_WSA_MCLK_MODE_MUXSEL_SEL_SHFT                                                                               0

#define HWIO_LPASS_AUDIO_WRAPPER_RSCC_XO_CBCR_ADDR(x)                                                                 ((x) + 0x45000)
#define HWIO_LPASS_AUDIO_WRAPPER_RSCC_XO_CBCR_RMSK                                                                    0x80000003
#define HWIO_LPASS_AUDIO_WRAPPER_RSCC_XO_CBCR_IN(x)            \
                in_dword(HWIO_LPASS_AUDIO_WRAPPER_RSCC_XO_CBCR_ADDR(x))
#define HWIO_LPASS_AUDIO_WRAPPER_RSCC_XO_CBCR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AUDIO_WRAPPER_RSCC_XO_CBCR_ADDR(x), m)
#define HWIO_LPASS_AUDIO_WRAPPER_RSCC_XO_CBCR_OUT(x, v)            \
                out_dword(HWIO_LPASS_AUDIO_WRAPPER_RSCC_XO_CBCR_ADDR(x),v)
#define HWIO_LPASS_AUDIO_WRAPPER_RSCC_XO_CBCR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AUDIO_WRAPPER_RSCC_XO_CBCR_ADDR(x),m,v,HWIO_LPASS_AUDIO_WRAPPER_RSCC_XO_CBCR_IN(x))
#define HWIO_LPASS_AUDIO_WRAPPER_RSCC_XO_CBCR_CLK_OFF_BMSK                                                            0x80000000
#define HWIO_LPASS_AUDIO_WRAPPER_RSCC_XO_CBCR_CLK_OFF_SHFT                                                                    31
#define HWIO_LPASS_AUDIO_WRAPPER_RSCC_XO_CBCR_HW_CTL_BMSK                                                                    0x2
#define HWIO_LPASS_AUDIO_WRAPPER_RSCC_XO_CBCR_HW_CTL_SHFT                                                                      1
#define HWIO_LPASS_AUDIO_WRAPPER_RSCC_XO_CBCR_CLK_ENABLE_BMSK                                                                0x1
#define HWIO_LPASS_AUDIO_WRAPPER_RSCC_XO_CBCR_CLK_ENABLE_SHFT                                                                  0

#define HWIO_LPASS_AUDIO_WRAPPER_RSCC_AON_CBCR_ADDR(x)                                                                ((x) + 0x45004)
#define HWIO_LPASS_AUDIO_WRAPPER_RSCC_AON_CBCR_RMSK                                                                   0x80000003
#define HWIO_LPASS_AUDIO_WRAPPER_RSCC_AON_CBCR_IN(x)            \
                in_dword(HWIO_LPASS_AUDIO_WRAPPER_RSCC_AON_CBCR_ADDR(x))
#define HWIO_LPASS_AUDIO_WRAPPER_RSCC_AON_CBCR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_AUDIO_WRAPPER_RSCC_AON_CBCR_ADDR(x), m)
#define HWIO_LPASS_AUDIO_WRAPPER_RSCC_AON_CBCR_OUT(x, v)            \
                out_dword(HWIO_LPASS_AUDIO_WRAPPER_RSCC_AON_CBCR_ADDR(x),v)
#define HWIO_LPASS_AUDIO_WRAPPER_RSCC_AON_CBCR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_AUDIO_WRAPPER_RSCC_AON_CBCR_ADDR(x),m,v,HWIO_LPASS_AUDIO_WRAPPER_RSCC_AON_CBCR_IN(x))
#define HWIO_LPASS_AUDIO_WRAPPER_RSCC_AON_CBCR_CLK_OFF_BMSK                                                           0x80000000
#define HWIO_LPASS_AUDIO_WRAPPER_RSCC_AON_CBCR_CLK_OFF_SHFT                                                                   31
#define HWIO_LPASS_AUDIO_WRAPPER_RSCC_AON_CBCR_HW_CTL_BMSK                                                                   0x2
#define HWIO_LPASS_AUDIO_WRAPPER_RSCC_AON_CBCR_HW_CTL_SHFT                                                                     1
#define HWIO_LPASS_AUDIO_WRAPPER_RSCC_AON_CBCR_CLK_ENABLE_BMSK                                                               0x1
#define HWIO_LPASS_AUDIO_WRAPPER_RSCC_AON_CBCR_CLK_ENABLE_SHFT                                                                 0

#define HWIO_LPASS_LPASS_CC_DEBUG_MUX_MUXR_ADDR(x)                                                                    ((x) + 0x46000)
#define HWIO_LPASS_LPASS_CC_DEBUG_MUX_MUXR_RMSK                                                                           0xffff
#define HWIO_LPASS_LPASS_CC_DEBUG_MUX_MUXR_IN(x)            \
                in_dword(HWIO_LPASS_LPASS_CC_DEBUG_MUX_MUXR_ADDR(x))
#define HWIO_LPASS_LPASS_CC_DEBUG_MUX_MUXR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPASS_CC_DEBUG_MUX_MUXR_ADDR(x), m)
#define HWIO_LPASS_LPASS_CC_DEBUG_MUX_MUXR_OUT(x, v)            \
                out_dword(HWIO_LPASS_LPASS_CC_DEBUG_MUX_MUXR_ADDR(x),v)
#define HWIO_LPASS_LPASS_CC_DEBUG_MUX_MUXR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_LPASS_CC_DEBUG_MUX_MUXR_ADDR(x),m,v,HWIO_LPASS_LPASS_CC_DEBUG_MUX_MUXR_IN(x))
#define HWIO_LPASS_LPASS_CC_DEBUG_MUX_MUXR_MUX_SEL_BMSK                                                                   0xffff
#define HWIO_LPASS_LPASS_CC_DEBUG_MUX_MUXR_MUX_SEL_SHFT                                                                        0

#define HWIO_LPASS_LPASS_CC_DEBUG_DIV_CDIVR_ADDR(x)                                                                   ((x) + 0x46004)
#define HWIO_LPASS_LPASS_CC_DEBUG_DIV_CDIVR_RMSK                                                                             0x3
#define HWIO_LPASS_LPASS_CC_DEBUG_DIV_CDIVR_IN(x)            \
                in_dword(HWIO_LPASS_LPASS_CC_DEBUG_DIV_CDIVR_ADDR(x))
#define HWIO_LPASS_LPASS_CC_DEBUG_DIV_CDIVR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPASS_CC_DEBUG_DIV_CDIVR_ADDR(x), m)
#define HWIO_LPASS_LPASS_CC_DEBUG_DIV_CDIVR_OUT(x, v)            \
                out_dword(HWIO_LPASS_LPASS_CC_DEBUG_DIV_CDIVR_ADDR(x),v)
#define HWIO_LPASS_LPASS_CC_DEBUG_DIV_CDIVR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_LPASS_CC_DEBUG_DIV_CDIVR_ADDR(x),m,v,HWIO_LPASS_LPASS_CC_DEBUG_DIV_CDIVR_IN(x))
#define HWIO_LPASS_LPASS_CC_DEBUG_DIV_CDIVR_CLK_DIV_BMSK                                                                     0x3
#define HWIO_LPASS_LPASS_CC_DEBUG_DIV_CDIVR_CLK_DIV_SHFT                                                                       0

#define HWIO_LPASS_LPASS_CC_DEBUG_CBCR_ADDR(x)                                                                        ((x) + 0x46008)
#define HWIO_LPASS_LPASS_CC_DEBUG_CBCR_RMSK                                                                           0x80000001
#define HWIO_LPASS_LPASS_CC_DEBUG_CBCR_IN(x)            \
                in_dword(HWIO_LPASS_LPASS_CC_DEBUG_CBCR_ADDR(x))
#define HWIO_LPASS_LPASS_CC_DEBUG_CBCR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPASS_CC_DEBUG_CBCR_ADDR(x), m)
#define HWIO_LPASS_LPASS_CC_DEBUG_CBCR_OUT(x, v)            \
                out_dword(HWIO_LPASS_LPASS_CC_DEBUG_CBCR_ADDR(x),v)
#define HWIO_LPASS_LPASS_CC_DEBUG_CBCR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_LPASS_CC_DEBUG_CBCR_ADDR(x),m,v,HWIO_LPASS_LPASS_CC_DEBUG_CBCR_IN(x))
#define HWIO_LPASS_LPASS_CC_DEBUG_CBCR_CLK_OFF_BMSK                                                                   0x80000000
#define HWIO_LPASS_LPASS_CC_DEBUG_CBCR_CLK_OFF_SHFT                                                                           31
#define HWIO_LPASS_LPASS_CC_DEBUG_CBCR_CLK_ENABLE_BMSK                                                                       0x1
#define HWIO_LPASS_LPASS_CC_DEBUG_CBCR_CLK_ENABLE_SHFT                                                                         0

#define HWIO_LPASS_LPASS_CC_PLL_TEST_MUX_MUXR_ADDR(x)                                                                 ((x) + 0x4600c)
#define HWIO_LPASS_LPASS_CC_PLL_TEST_MUX_MUXR_RMSK                                                                           0x3
#define HWIO_LPASS_LPASS_CC_PLL_TEST_MUX_MUXR_IN(x)            \
                in_dword(HWIO_LPASS_LPASS_CC_PLL_TEST_MUX_MUXR_ADDR(x))
#define HWIO_LPASS_LPASS_CC_PLL_TEST_MUX_MUXR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPASS_CC_PLL_TEST_MUX_MUXR_ADDR(x), m)
#define HWIO_LPASS_LPASS_CC_PLL_TEST_MUX_MUXR_OUT(x, v)            \
                out_dword(HWIO_LPASS_LPASS_CC_PLL_TEST_MUX_MUXR_ADDR(x),v)
#define HWIO_LPASS_LPASS_CC_PLL_TEST_MUX_MUXR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_LPASS_CC_PLL_TEST_MUX_MUXR_ADDR(x),m,v,HWIO_LPASS_LPASS_CC_PLL_TEST_MUX_MUXR_IN(x))
#define HWIO_LPASS_LPASS_CC_PLL_TEST_MUX_MUXR_MUX_SEL_BMSK                                                                   0x3
#define HWIO_LPASS_LPASS_CC_PLL_TEST_MUX_MUXR_MUX_SEL_SHFT                                                                     0

#define HWIO_LPASS_LPASS_CC_PLL_TEST_DIV_CDIVR_ADDR(x)                                                                ((x) + 0x46010)
#define HWIO_LPASS_LPASS_CC_PLL_TEST_DIV_CDIVR_RMSK                                                                          0x3
#define HWIO_LPASS_LPASS_CC_PLL_TEST_DIV_CDIVR_IN(x)            \
                in_dword(HWIO_LPASS_LPASS_CC_PLL_TEST_DIV_CDIVR_ADDR(x))
#define HWIO_LPASS_LPASS_CC_PLL_TEST_DIV_CDIVR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPASS_CC_PLL_TEST_DIV_CDIVR_ADDR(x), m)
#define HWIO_LPASS_LPASS_CC_PLL_TEST_DIV_CDIVR_OUT(x, v)            \
                out_dword(HWIO_LPASS_LPASS_CC_PLL_TEST_DIV_CDIVR_ADDR(x),v)
#define HWIO_LPASS_LPASS_CC_PLL_TEST_DIV_CDIVR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_LPASS_CC_PLL_TEST_DIV_CDIVR_ADDR(x),m,v,HWIO_LPASS_LPASS_CC_PLL_TEST_DIV_CDIVR_IN(x))
#define HWIO_LPASS_LPASS_CC_PLL_TEST_DIV_CDIVR_CLK_DIV_BMSK                                                                  0x3
#define HWIO_LPASS_LPASS_CC_PLL_TEST_DIV_CDIVR_CLK_DIV_SHFT                                                                    0

#define HWIO_LPASS_LPASS_CC_PLL_TEST_CBCR_ADDR(x)                                                                     ((x) + 0x46014)
#define HWIO_LPASS_LPASS_CC_PLL_TEST_CBCR_RMSK                                                                        0x80000001
#define HWIO_LPASS_LPASS_CC_PLL_TEST_CBCR_IN(x)            \
                in_dword(HWIO_LPASS_LPASS_CC_PLL_TEST_CBCR_ADDR(x))
#define HWIO_LPASS_LPASS_CC_PLL_TEST_CBCR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPASS_CC_PLL_TEST_CBCR_ADDR(x), m)
#define HWIO_LPASS_LPASS_CC_PLL_TEST_CBCR_OUT(x, v)            \
                out_dword(HWIO_LPASS_LPASS_CC_PLL_TEST_CBCR_ADDR(x),v)
#define HWIO_LPASS_LPASS_CC_PLL_TEST_CBCR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_LPASS_CC_PLL_TEST_CBCR_ADDR(x),m,v,HWIO_LPASS_LPASS_CC_PLL_TEST_CBCR_IN(x))
#define HWIO_LPASS_LPASS_CC_PLL_TEST_CBCR_CLK_OFF_BMSK                                                                0x80000000
#define HWIO_LPASS_LPASS_CC_PLL_TEST_CBCR_CLK_OFF_SHFT                                                                        31
#define HWIO_LPASS_LPASS_CC_PLL_TEST_CBCR_CLK_ENABLE_BMSK                                                                    0x1
#define HWIO_LPASS_LPASS_CC_PLL_TEST_CBCR_CLK_ENABLE_SHFT                                                                      0

#define HWIO_LPASS_LPASS_CC_PLL_RESET_N_MUXR_ADDR(x)                                                                  ((x) + 0x46018)
#define HWIO_LPASS_LPASS_CC_PLL_RESET_N_MUXR_RMSK                                                                            0x7
#define HWIO_LPASS_LPASS_CC_PLL_RESET_N_MUXR_IN(x)            \
                in_dword(HWIO_LPASS_LPASS_CC_PLL_RESET_N_MUXR_ADDR(x))
#define HWIO_LPASS_LPASS_CC_PLL_RESET_N_MUXR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPASS_CC_PLL_RESET_N_MUXR_ADDR(x), m)
#define HWIO_LPASS_LPASS_CC_PLL_RESET_N_MUXR_OUT(x, v)            \
                out_dword(HWIO_LPASS_LPASS_CC_PLL_RESET_N_MUXR_ADDR(x),v)
#define HWIO_LPASS_LPASS_CC_PLL_RESET_N_MUXR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_LPASS_CC_PLL_RESET_N_MUXR_ADDR(x),m,v,HWIO_LPASS_LPASS_CC_PLL_RESET_N_MUXR_IN(x))
#define HWIO_LPASS_LPASS_CC_PLL_RESET_N_MUXR_MUX_SEL_BMSK                                                                    0x7
#define HWIO_LPASS_LPASS_CC_PLL_RESET_N_MUXR_MUX_SEL_SHFT                                                                      0

#define HWIO_LPASS_LPASS_CC_PLL_BYPASSNL_MUXR_ADDR(x)                                                                 ((x) + 0x46020)
#define HWIO_LPASS_LPASS_CC_PLL_BYPASSNL_MUXR_RMSK                                                                           0x7
#define HWIO_LPASS_LPASS_CC_PLL_BYPASSNL_MUXR_IN(x)            \
                in_dword(HWIO_LPASS_LPASS_CC_PLL_BYPASSNL_MUXR_ADDR(x))
#define HWIO_LPASS_LPASS_CC_PLL_BYPASSNL_MUXR_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPASS_CC_PLL_BYPASSNL_MUXR_ADDR(x), m)
#define HWIO_LPASS_LPASS_CC_PLL_BYPASSNL_MUXR_OUT(x, v)            \
                out_dword(HWIO_LPASS_LPASS_CC_PLL_BYPASSNL_MUXR_ADDR(x),v)
#define HWIO_LPASS_LPASS_CC_PLL_BYPASSNL_MUXR_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_LPASS_CC_PLL_BYPASSNL_MUXR_ADDR(x),m,v,HWIO_LPASS_LPASS_CC_PLL_BYPASSNL_MUXR_IN(x))
#define HWIO_LPASS_LPASS_CC_PLL_BYPASSNL_MUXR_MUX_SEL_BMSK                                                                   0x7
#define HWIO_LPASS_LPASS_CC_PLL_BYPASSNL_MUXR_MUX_SEL_SHFT                                                                     0

#define HWIO_LPASS_TEST_BUS_SEL_ADDR(x)                                                                               ((x) + 0x46024)
#define HWIO_LPASS_TEST_BUS_SEL_RMSK                                                                                         0x3
#define HWIO_LPASS_TEST_BUS_SEL_IN(x)            \
                in_dword(HWIO_LPASS_TEST_BUS_SEL_ADDR(x))
#define HWIO_LPASS_TEST_BUS_SEL_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_TEST_BUS_SEL_ADDR(x), m)
#define HWIO_LPASS_TEST_BUS_SEL_OUT(x, v)            \
                out_dword(HWIO_LPASS_TEST_BUS_SEL_ADDR(x),v)
#define HWIO_LPASS_TEST_BUS_SEL_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_TEST_BUS_SEL_ADDR(x),m,v,HWIO_LPASS_TEST_BUS_SEL_IN(x))
#define HWIO_LPASS_TEST_BUS_SEL_SEL_BMSK                                                                                     0x3
#define HWIO_LPASS_TEST_BUS_SEL_SEL_SHFT                                                                                       0

#define HWIO_LPASS_LPASS_CC_SPARE_REG_ADDR(x)                                                                         ((x) + 0x47000)
#define HWIO_LPASS_LPASS_CC_SPARE_REG_RMSK                                                                            0xffffffff
#define HWIO_LPASS_LPASS_CC_SPARE_REG_IN(x)            \
                in_dword(HWIO_LPASS_LPASS_CC_SPARE_REG_ADDR(x))
#define HWIO_LPASS_LPASS_CC_SPARE_REG_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPASS_CC_SPARE_REG_ADDR(x), m)
#define HWIO_LPASS_LPASS_CC_SPARE_REG_OUT(x, v)            \
                out_dword(HWIO_LPASS_LPASS_CC_SPARE_REG_ADDR(x),v)
#define HWIO_LPASS_LPASS_CC_SPARE_REG_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_LPASS_CC_SPARE_REG_ADDR(x),m,v,HWIO_LPASS_LPASS_CC_SPARE_REG_IN(x))
#define HWIO_LPASS_LPASS_CC_SPARE_REG_SPARE_BMSK                                                                      0xffffffff
#define HWIO_LPASS_LPASS_CC_SPARE_REG_SPARE_SHFT                                                                               0

#define HWIO_LPASS_LPASS_CC_SPARE1_REG_ADDR(x)                                                                        ((x) + 0x47004)
#define HWIO_LPASS_LPASS_CC_SPARE1_REG_RMSK                                                                           0xffffffff
#define HWIO_LPASS_LPASS_CC_SPARE1_REG_IN(x)            \
                in_dword(HWIO_LPASS_LPASS_CC_SPARE1_REG_ADDR(x))
#define HWIO_LPASS_LPASS_CC_SPARE1_REG_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPASS_CC_SPARE1_REG_ADDR(x), m)
#define HWIO_LPASS_LPASS_CC_SPARE1_REG_OUT(x, v)            \
                out_dword(HWIO_LPASS_LPASS_CC_SPARE1_REG_ADDR(x),v)
#define HWIO_LPASS_LPASS_CC_SPARE1_REG_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_LPASS_CC_SPARE1_REG_ADDR(x),m,v,HWIO_LPASS_LPASS_CC_SPARE1_REG_IN(x))
#define HWIO_LPASS_LPASS_CC_SPARE1_REG_SPARE1_BMSK                                                                    0xffffffff
#define HWIO_LPASS_LPASS_CC_SPARE1_REG_SPARE1_SHFT                                                                             0


/*
 * TCSR
 */
#define HWIO_LPASS_TCSR_QOS_CTL_ADDR(x)				((x)  + 0x9D000)
#define HWIO_LPASS_AUDIO_CORE_LPAIF_PRI_MODE_MUXSEL_ADDR(x)	((x)  + 0x8A000)
#define HWIO_LPASS_AUDIO_CORE_LPAIF_SEC_MODE_MUXSEL_ADDR(x)	((x)  + 0x8B000)
/*
 * CSR
 */
#define HWIO_LPASS_AUDIO_CORE_QOS_CTL_ADDR(x)			((x) + 0x21B000)
#define HWIO_LPASS_AUDIO_CORE_QOS_CORE_CGCR_ADDR(x)		((x) + 0x20B000)

/*
 * CC register
 */

#define HWIO_LPASS_LPM_CTL_ADDR(x)				((x) + 0x203000)
#define HWIO_LPASS_LPAIF_CTL_ADDR(x)				((x) + 0x205000)
#endif /* __IPQ_CC_H__ */
