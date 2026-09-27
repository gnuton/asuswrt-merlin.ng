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


/*----------------------------------------------------------------------------
 * MODULE: LPASS_LPA_IF
 *--------------------------------------------------------------------------*/

#define LPASS_BASE								0x0A000000

#define LPASS_LPA_IF_REG_BASE                                                       (LPASS_BASE            + 0x003c0000)
#define LPASS_LPA_IF_REG_BASE_SIZE                                                  0x29000
#define LPASS_LPA_IF_REG_BASE_USED                                                  0x28000
#define LPASS_LPA_IF_REG_BASE_OFFS                                                  0x003c0000

#define HWIO_LPASS_LPAIF_VERSION_ADDR(x)                                            ((x) + 0x0)
#define HWIO_LPASS_LPAIF_VERSION_OFFS                                               (0x0)
#define HWIO_LPASS_LPAIF_VERSION_RMSK                                               0xffffffff
#define HWIO_LPASS_LPAIF_VERSION_IN(x)            \
                in_dword(HWIO_LPASS_LPAIF_VERSION_ADDR(x))
#define HWIO_LPASS_LPAIF_VERSION_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPAIF_VERSION_ADDR(x), m)
#define HWIO_LPASS_LPAIF_VERSION_MAJOR_BMSK                                         0xf0000000
#define HWIO_LPASS_LPAIF_VERSION_MAJOR_SHFT                                                 28
#define HWIO_LPASS_LPAIF_VERSION_MINOR_BMSK                                          0xfff0000
#define HWIO_LPASS_LPAIF_VERSION_MINOR_SHFT                                                 16
#define HWIO_LPASS_LPAIF_VERSION_STEP_BMSK                                              0xffff
#define HWIO_LPASS_LPAIF_VERSION_STEP_SHFT                                                   0

#define HWIO_LPASS_LPAIF_HW_CONFIG_ADDR(x)                                          ((x) + 0x4)
#define HWIO_LPASS_LPAIF_HW_CONFIG_OFFS                                             (0x4)
#define HWIO_LPASS_LPAIF_HW_CONFIG_RMSK                                             0x7fffffff
#define HWIO_LPASS_LPAIF_HW_CONFIG_IN(x)            \
                in_dword(HWIO_LPASS_LPAIF_HW_CONFIG_ADDR(x))
#define HWIO_LPASS_LPAIF_HW_CONFIG_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPAIF_HW_CONFIG_ADDR(x), m)
#define HWIO_LPASS_LPAIF_HW_CONFIG_SIF_EN_BMSK                                      0x40000000
#define HWIO_LPASS_LPAIF_HW_CONFIG_SIF_EN_SHFT                                              30
#define HWIO_LPASS_LPAIF_HW_CONFIG_QUAD_I2S_DIR_BMSK                                0x30000000
#define HWIO_LPASS_LPAIF_HW_CONFIG_QUAD_I2S_DIR_SHFT                                        28
#define HWIO_LPASS_LPAIF_HW_CONFIG_QUAD_I2S_EN_BMSK                                  0x8000000
#define HWIO_LPASS_LPAIF_HW_CONFIG_QUAD_I2S_EN_SHFT                                         27
#define HWIO_LPASS_LPAIF_HW_CONFIG_TER_I2S_LINES_BMSK                                0x7000000
#define HWIO_LPASS_LPAIF_HW_CONFIG_TER_I2S_LINES_SHFT                                       24
#define HWIO_LPASS_LPAIF_HW_CONFIG_TER_I2S_DIR_BMSK                                   0xc00000
#define HWIO_LPASS_LPAIF_HW_CONFIG_TER_I2S_DIR_SHFT                                         22
#define HWIO_LPASS_LPAIF_HW_CONFIG_TER_I2S_EN_BMSK                                    0x200000
#define HWIO_LPASS_LPAIF_HW_CONFIG_TER_I2S_EN_SHFT                                          21
#define HWIO_LPASS_LPAIF_HW_CONFIG_SEC_I2S_LINES_BMSK                                 0x1c0000
#define HWIO_LPASS_LPAIF_HW_CONFIG_SEC_I2S_LINES_SHFT                                       18
#define HWIO_LPASS_LPAIF_HW_CONFIG_SEC_I2S_DIR_BMSK                                    0x30000
#define HWIO_LPASS_LPAIF_HW_CONFIG_SEC_I2S_DIR_SHFT                                         16
#define HWIO_LPASS_LPAIF_HW_CONFIG_SEC_I2S_EN_BMSK                                      0x8000
#define HWIO_LPASS_LPAIF_HW_CONFIG_SEC_I2S_EN_SHFT                                          15
#define HWIO_LPASS_LPAIF_HW_CONFIG_PRI_I2S_LINES_BMSK                                   0x7000
#define HWIO_LPASS_LPAIF_HW_CONFIG_PRI_I2S_LINES_SHFT                                       12
#define HWIO_LPASS_LPAIF_HW_CONFIG_PRI_I2S_DIR_BMSK                                      0xc00
#define HWIO_LPASS_LPAIF_HW_CONFIG_PRI_I2S_DIR_SHFT                                         10
#define HWIO_LPASS_LPAIF_HW_CONFIG_PRI_I2S_EN_BMSK                                       0x200
#define HWIO_LPASS_LPAIF_HW_CONFIG_PRI_I2S_EN_SHFT                                           9
#define HWIO_LPASS_LPAIF_HW_CONFIG_QUI_I2S_LINES_BMSK                                    0x1c0
#define HWIO_LPASS_LPAIF_HW_CONFIG_QUI_I2S_LINES_SHFT                                        6
#define HWIO_LPASS_LPAIF_HW_CONFIG_QUI_I2S_DIR_BMSK                                       0x30
#define HWIO_LPASS_LPAIF_HW_CONFIG_QUI_I2S_DIR_SHFT                                          4
#define HWIO_LPASS_LPAIF_HW_CONFIG_QUI_I2S_EN_BMSK                                         0x8
#define HWIO_LPASS_LPAIF_HW_CONFIG_QUI_I2S_EN_SHFT                                           3
#define HWIO_LPASS_LPAIF_HW_CONFIG_QUAD_I2S_LINES_BMSK                                     0x7
#define HWIO_LPASS_LPAIF_HW_CONFIG_QUAD_I2S_LINES_SHFT                                       0

#define HWIO_LPASS_LPAIF_HW_CONFIG2_ADDR(x)                                         ((x) + 0x8)
#define HWIO_LPASS_LPAIF_HW_CONFIG2_OFFS                                            (0x8)
#define HWIO_LPASS_LPAIF_HW_CONFIG2_RMSK                                            0xffffffff
#define HWIO_LPASS_LPAIF_HW_CONFIG2_IN(x)            \
                in_dword(HWIO_LPASS_LPAIF_HW_CONFIG2_ADDR(x))
#define HWIO_LPASS_LPAIF_HW_CONFIG2_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPAIF_HW_CONFIG2_ADDR(x), m)
#define HWIO_LPASS_LPAIF_HW_CONFIG2_LPAIF_AXI_BMSK                                  0x80000000
#define HWIO_LPASS_LPAIF_HW_CONFIG2_LPAIF_AXI_SHFT                                          31
#define HWIO_LPASS_LPAIF_HW_CONFIG2_CODEC_TX8_INTF_EN_BMSK                          0x40000000
#define HWIO_LPASS_LPAIF_HW_CONFIG2_CODEC_TX8_INTF_EN_SHFT                                  30
#define HWIO_LPASS_LPAIF_HW_CONFIG2_CODEC_TX7_INTF_EN_BMSK                          0x20000000
#define HWIO_LPASS_LPAIF_HW_CONFIG2_CODEC_TX7_INTF_EN_SHFT                                  29
#define HWIO_LPASS_LPAIF_HW_CONFIG2_CODEC_TX6_INTF_EN_BMSK                          0x10000000
#define HWIO_LPASS_LPAIF_HW_CONFIG2_CODEC_TX6_INTF_EN_SHFT                                  28
#define HWIO_LPASS_LPAIF_HW_CONFIG2_CODEC_TX5_INTF_EN_BMSK                           0x8000000
#define HWIO_LPASS_LPAIF_HW_CONFIG2_CODEC_TX5_INTF_EN_SHFT                                  27
#define HWIO_LPASS_LPAIF_HW_CONFIG2_CODEC_TX4_INTF_EN_BMSK                           0x4000000
#define HWIO_LPASS_LPAIF_HW_CONFIG2_CODEC_TX4_INTF_EN_SHFT                                  26
#define HWIO_LPASS_LPAIF_HW_CONFIG2_CODEC_TX3_INTF_EN_BMSK                           0x2000000
#define HWIO_LPASS_LPAIF_HW_CONFIG2_CODEC_TX3_INTF_EN_SHFT                                  25
#define HWIO_LPASS_LPAIF_HW_CONFIG2_CODEC_TX2_INTF_EN_BMSK                           0x1000000
#define HWIO_LPASS_LPAIF_HW_CONFIG2_CODEC_TX2_INTF_EN_SHFT                                  24
#define HWIO_LPASS_LPAIF_HW_CONFIG2_CODEC_TX1_INTF_EN_BMSK                            0x800000
#define HWIO_LPASS_LPAIF_HW_CONFIG2_CODEC_TX1_INTF_EN_SHFT                                  23
#define HWIO_LPASS_LPAIF_HW_CONFIG2_CODEC_TX0_INTF_EN_BMSK                            0x400000
#define HWIO_LPASS_LPAIF_HW_CONFIG2_CODEC_TX0_INTF_EN_SHFT                                  22
#define HWIO_LPASS_LPAIF_HW_CONFIG2_CODEC_RX9_INTF_EN_BMSK                            0x200000
#define HWIO_LPASS_LPAIF_HW_CONFIG2_CODEC_RX9_INTF_EN_SHFT                                  21
#define HWIO_LPASS_LPAIF_HW_CONFIG2_CODEC_RX8_INTF_EN_BMSK                            0x100000
#define HWIO_LPASS_LPAIF_HW_CONFIG2_CODEC_RX8_INTF_EN_SHFT                                  20
#define HWIO_LPASS_LPAIF_HW_CONFIG2_CODEC_RX7_INTF_EN_BMSK                             0x80000
#define HWIO_LPASS_LPAIF_HW_CONFIG2_CODEC_RX7_INTF_EN_SHFT                                  19
#define HWIO_LPASS_LPAIF_HW_CONFIG2_CODEC_RX6_INTF_EN_BMSK                             0x40000
#define HWIO_LPASS_LPAIF_HW_CONFIG2_CODEC_RX6_INTF_EN_SHFT                                  18
#define HWIO_LPASS_LPAIF_HW_CONFIG2_CODEC_RX5_INTF_EN_BMSK                             0x20000
#define HWIO_LPASS_LPAIF_HW_CONFIG2_CODEC_RX5_INTF_EN_SHFT                                  17
#define HWIO_LPASS_LPAIF_HW_CONFIG2_CODEC_RX4_INTF_EN_BMSK                             0x10000
#define HWIO_LPASS_LPAIF_HW_CONFIG2_CODEC_RX4_INTF_EN_SHFT                                  16
#define HWIO_LPASS_LPAIF_HW_CONFIG2_CODEC_RX3_INTF_EN_BMSK                              0x8000
#define HWIO_LPASS_LPAIF_HW_CONFIG2_CODEC_RX3_INTF_EN_SHFT                                  15
#define HWIO_LPASS_LPAIF_HW_CONFIG2_CODEC_RX2_INTF_EN_BMSK                              0x4000
#define HWIO_LPASS_LPAIF_HW_CONFIG2_CODEC_RX2_INTF_EN_SHFT                                  14
#define HWIO_LPASS_LPAIF_HW_CONFIG2_CODEC_RX1_INTF_EN_BMSK                              0x2000
#define HWIO_LPASS_LPAIF_HW_CONFIG2_CODEC_RX1_INTF_EN_SHFT                                  13
#define HWIO_LPASS_LPAIF_HW_CONFIG2_CODEC_RX0_INTF_EN_BMSK                              0x1000
#define HWIO_LPASS_LPAIF_HW_CONFIG2_CODEC_RX0_INTF_EN_SHFT                                  12
#define HWIO_LPASS_LPAIF_HW_CONFIG2_QUAD_PCM_EN_BMSK                                     0x800
#define HWIO_LPASS_LPAIF_HW_CONFIG2_QUAD_PCM_EN_SHFT                                        11
#define HWIO_LPASS_LPAIF_HW_CONFIG2_TER_PCM_EN_BMSK                                      0x400
#define HWIO_LPASS_LPAIF_HW_CONFIG2_TER_PCM_EN_SHFT                                         10
#define HWIO_LPASS_LPAIF_HW_CONFIG2_SEC_RATE_DET_EN_BMSK                                 0x200
#define HWIO_LPASS_LPAIF_HW_CONFIG2_SEC_RATE_DET_EN_SHFT                                     9
#define HWIO_LPASS_LPAIF_HW_CONFIG2_PRI_RATE_DET_EN_BMSK                                 0x100
#define HWIO_LPASS_LPAIF_HW_CONFIG2_PRI_RATE_DET_EN_SHFT                                     8
#define HWIO_LPASS_LPAIF_HW_CONFIG2_NUM_RDDMA_BMSK                                        0xf0
#define HWIO_LPASS_LPAIF_HW_CONFIG2_NUM_RDDMA_SHFT                                           4
#define HWIO_LPASS_LPAIF_HW_CONFIG2_NUM_WRDMA_BMSK                                         0xf
#define HWIO_LPASS_LPAIF_HW_CONFIG2_NUM_WRDMA_SHFT                                           0

#define HWIO_LPASS_LPAIF_HW_CONFIG3_ADDR(x)                                         ((x) + 0x10)
#define HWIO_LPASS_LPAIF_HW_CONFIG3_OFFS                                            (0x10)
#define HWIO_LPASS_LPAIF_HW_CONFIG3_RMSK                                                  0x7f
#define HWIO_LPASS_LPAIF_HW_CONFIG3_IN(x)            \
                in_dword(HWIO_LPASS_LPAIF_HW_CONFIG3_ADDR(x))
#define HWIO_LPASS_LPAIF_HW_CONFIG3_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPAIF_HW_CONFIG3_ADDR(x), m)
#define HWIO_LPASS_LPAIF_HW_CONFIG3_SEC_PCM_EN_BMSK                                       0x40
#define HWIO_LPASS_LPAIF_HW_CONFIG3_SEC_PCM_EN_SHFT                                          6
#define HWIO_LPASS_LPAIF_HW_CONFIG3_PRI_PCM_EN_BMSK                                       0x20
#define HWIO_LPASS_LPAIF_HW_CONFIG3_PRI_PCM_EN_SHFT                                          5
#define HWIO_LPASS_LPAIF_HW_CONFIG3_INTF_GRP1_EN_BMSK                                     0x10
#define HWIO_LPASS_LPAIF_HW_CONFIG3_INTF_GRP1_EN_SHFT                                        4
#define HWIO_LPASS_LPAIF_HW_CONFIG3_INTF_GRP0_EN_BMSK                                      0x8
#define HWIO_LPASS_LPAIF_HW_CONFIG3_INTF_GRP0_EN_SHFT                                        3
#define HWIO_LPASS_LPAIF_HW_CONFIG3_SEC_SPDIFRX_EN_BMSK                                    0x4
#define HWIO_LPASS_LPAIF_HW_CONFIG3_SEC_SPDIFRX_EN_SHFT                                      2
#define HWIO_LPASS_LPAIF_HW_CONFIG3_PRI_SPDIFRX_EN_BMSK                                    0x2
#define HWIO_LPASS_LPAIF_HW_CONFIG3_PRI_SPDIFRX_EN_SHFT                                      1
#define HWIO_LPASS_LPAIF_HW_CONFIG3_QUI_PCM_EN_BMSK                                        0x1
#define HWIO_LPASS_LPAIF_HW_CONFIG3_QUI_PCM_EN_SHFT                                          0

#define HWIO_LPASS_LPAIF_SPARE_ADDR(x)                                              ((x) + 0x100)
#define HWIO_LPASS_LPAIF_SPARE_OFFS                                                 (0x100)
#define HWIO_LPASS_LPAIF_SPARE_RMSK                                                 0xffffffff
#define HWIO_LPASS_LPAIF_SPARE_IN(x)            \
                in_dword(HWIO_LPASS_LPAIF_SPARE_ADDR(x))
#define HWIO_LPASS_LPAIF_SPARE_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPAIF_SPARE_ADDR(x), m)
#define HWIO_LPASS_LPAIF_SPARE_OUT(x, v)            \
                out_dword(HWIO_LPASS_LPAIF_SPARE_ADDR(x),v)
#define HWIO_LPASS_LPAIF_SPARE_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_LPAIF_SPARE_ADDR(x),m,v,HWIO_LPASS_LPAIF_SPARE_IN(x))
#define HWIO_LPASS_LPAIF_SPARE_SPARE_BMSK                                           0xffffffff
#define HWIO_LPASS_LPAIF_SPARE_SPARE_SHFT                                                    0

#define HWIO_LPASS_LPAIF_PCM_I2S_SELa_ADDR(base,a)                                  ((base) + 0X1200 + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_PCM_I2S_SELa_OFFS(a)                                       (0X1200 + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_PCM_I2S_SELa_RMSK                                                 0x1
#define HWIO_LPASS_LPAIF_PCM_I2S_SELa_MAXa                                                   2
#define HWIO_LPASS_LPAIF_PCM_I2S_SELa_INI(base,a)                \
                in_dword_masked(HWIO_LPASS_LPAIF_PCM_I2S_SELa_ADDR(base,a), HWIO_LPASS_LPAIF_PCM_I2S_SELa_RMSK)
#define HWIO_LPASS_LPAIF_PCM_I2S_SELa_INMI(base,a,mask)        \
                in_dword_masked(HWIO_LPASS_LPAIF_PCM_I2S_SELa_ADDR(base,a), mask)
#define HWIO_LPASS_LPAIF_PCM_I2S_SELa_OUTI(base,a,val)        \
                out_dword(HWIO_LPASS_LPAIF_PCM_I2S_SELa_ADDR(base,a),val)
#define HWIO_LPASS_LPAIF_PCM_I2S_SELa_OUTMI(base,a,mask,val) \
                out_dword_masked_ns(HWIO_LPASS_LPAIF_PCM_I2S_SELa_ADDR(base,a),mask,val,HWIO_LPASS_LPAIF_PCM_I2S_SELa_INI(base,a))
#define HWIO_LPASS_LPAIF_PCM_I2S_SELa_SEL_BMSK                                             0x1
#define HWIO_LPASS_LPAIF_PCM_I2S_SELa_SEL_SHFT                                               0
#define HWIO_LPASS_LPAIF_PCM_I2S_SELa_SEL_I2S_SRC_FVAL                                     0x0
#define HWIO_LPASS_LPAIF_PCM_I2S_SELa_SEL_PCM_SRC_FVAL                                     0x1

#define HWIO_LPASS_LPAIF_PCM_CTLa_ADDR(base,a)                                      ((base) + 0X1500 + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_PCM_CTLa_OFFS(a)                                           (0X1500 + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_PCM_CTLa_RMSK                                              0x9f0ffc00
#define HWIO_LPASS_LPAIF_PCM_CTLa_MAXa                                                       2
#define HWIO_LPASS_LPAIF_PCM_CTLa_INI(base,a)                \
                in_dword_masked(HWIO_LPASS_LPAIF_PCM_CTLa_ADDR(base,a), HWIO_LPASS_LPAIF_PCM_CTLa_RMSK)
#define HWIO_LPASS_LPAIF_PCM_CTLa_INMI(base,a,mask)        \
                in_dword_masked(HWIO_LPASS_LPAIF_PCM_CTLa_ADDR(base,a), mask)
#define HWIO_LPASS_LPAIF_PCM_CTLa_OUTI(base,a,val)        \
                out_dword(HWIO_LPASS_LPAIF_PCM_CTLa_ADDR(base,a),val)
#define HWIO_LPASS_LPAIF_PCM_CTLa_OUTMI(base,a,mask,val) \
                out_dword_masked_ns(HWIO_LPASS_LPAIF_PCM_CTLa_ADDR(base,a),mask,val,HWIO_LPASS_LPAIF_PCM_CTLa_INI(base,a))
#define HWIO_LPASS_LPAIF_PCM_CTLa_RESET_BMSK                                        0x80000000
#define HWIO_LPASS_LPAIF_PCM_CTLa_RESET_SHFT                                                31
#define HWIO_LPASS_LPAIF_PCM_CTLa_RESET_DISABLE_FVAL                                       0x0
#define HWIO_LPASS_LPAIF_PCM_CTLa_RESET_ENABLE_FVAL                                        0x1
#define HWIO_LPASS_LPAIF_PCM_CTLa_RESET_RX_BMSK                                     0x10000000
#define HWIO_LPASS_LPAIF_PCM_CTLa_RESET_RX_SHFT                                             28
#define HWIO_LPASS_LPAIF_PCM_CTLa_RESET_RX_DISABLE_FVAL                                    0x0
#define HWIO_LPASS_LPAIF_PCM_CTLa_RESET_RX_ENABLE_FVAL                                     0x1
#define HWIO_LPASS_LPAIF_PCM_CTLa_RESET_TX_BMSK                                      0x8000000
#define HWIO_LPASS_LPAIF_PCM_CTLa_RESET_TX_SHFT                                             27
#define HWIO_LPASS_LPAIF_PCM_CTLa_RESET_TX_DISABLE_FVAL                                    0x0
#define HWIO_LPASS_LPAIF_PCM_CTLa_RESET_TX_ENABLE_FVAL                                     0x1
#define HWIO_LPASS_LPAIF_PCM_CTLa_ENABLE_RX_BMSK                                     0x4000000
#define HWIO_LPASS_LPAIF_PCM_CTLa_ENABLE_RX_SHFT                                            26
#define HWIO_LPASS_LPAIF_PCM_CTLa_ENABLE_RX_DISABLE_FVAL                                   0x0
#define HWIO_LPASS_LPAIF_PCM_CTLa_ENABLE_RX_ENABLE_FVAL                                    0x1
#define HWIO_LPASS_LPAIF_PCM_CTLa_ENABLE_TX_BMSK                                     0x2000000
#define HWIO_LPASS_LPAIF_PCM_CTLa_ENABLE_TX_SHFT                                            25
#define HWIO_LPASS_LPAIF_PCM_CTLa_ENABLE_TX_DISABLE_FVAL                                   0x0
#define HWIO_LPASS_LPAIF_PCM_CTLa_ENABLE_TX_ENABLE_FVAL                                    0x1
#define HWIO_LPASS_LPAIF_PCM_CTLa_ENABLE_BMSK                                        0x1000000
#define HWIO_LPASS_LPAIF_PCM_CTLa_ENABLE_SHFT                                               24
#define HWIO_LPASS_LPAIF_PCM_CTLa_ENABLE_DISABLE_FVAL                                      0x0
#define HWIO_LPASS_LPAIF_PCM_CTLa_ENABLE_ENABLE_FVAL                                       0x1
#define HWIO_LPASS_LPAIF_PCM_CTLa_ONE_SLOT_SYNC_EN_BMSK                                0x80000
#define HWIO_LPASS_LPAIF_PCM_CTLa_ONE_SLOT_SYNC_EN_SHFT                                     19
#define HWIO_LPASS_LPAIF_PCM_CTLa_ONE_SLOT_SYNC_EN_DISABLE_FVAL                            0x0
#define HWIO_LPASS_LPAIF_PCM_CTLa_ONE_SLOT_SYNC_EN_ENABLE_FVAL                             0x1
#define HWIO_LPASS_LPAIF_PCM_CTLa_CTRL_DATA_OE_BMSK                                    0x40000
#define HWIO_LPASS_LPAIF_PCM_CTLa_CTRL_DATA_OE_SHFT                                         18
#define HWIO_LPASS_LPAIF_PCM_CTLa_RATE_BMSK                                            0x38000
#define HWIO_LPASS_LPAIF_PCM_CTLa_RATE_SHFT                                                 15
#define HWIO_LPASS_LPAIF_PCM_CTLa_RATE_ENUM_8_FVAL                                         0x0
#define HWIO_LPASS_LPAIF_PCM_CTLa_RATE_ENUM_16_FVAL                                        0x1
#define HWIO_LPASS_LPAIF_PCM_CTLa_RATE_ENUM_32_FVAL                                        0x2
#define HWIO_LPASS_LPAIF_PCM_CTLa_RATE_ENUM_64_FVAL                                        0x3
#define HWIO_LPASS_LPAIF_PCM_CTLa_RATE_ENUM_128_FVAL                                       0x4
#define HWIO_LPASS_LPAIF_PCM_CTLa_RATE_ENUM_256_FVAL                                       0x5
#define HWIO_LPASS_LPAIF_PCM_CTLa_LOOPBACK_BMSK                                         0x4000
#define HWIO_LPASS_LPAIF_PCM_CTLa_LOOPBACK_SHFT                                             14
#define HWIO_LPASS_LPAIF_PCM_CTLa_SYNC_SRC_BMSK                                         0x2000
#define HWIO_LPASS_LPAIF_PCM_CTLa_SYNC_SRC_SHFT                                             13
#define HWIO_LPASS_LPAIF_PCM_CTLa_SYNC_SRC_EXTERNAL_FVAL                                   0x0
#define HWIO_LPASS_LPAIF_PCM_CTLa_SYNC_SRC_INTERNAL_FVAL                                   0x1
#define HWIO_LPASS_LPAIF_PCM_CTLa_AUX_MODE_BMSK                                         0x1000
#define HWIO_LPASS_LPAIF_PCM_CTLa_AUX_MODE_SHFT                                             12
#define HWIO_LPASS_LPAIF_PCM_CTLa_AUX_MODE_PCM_FVAL                                        0x0
#define HWIO_LPASS_LPAIF_PCM_CTLa_AUX_MODE_AUX_FVAL                                        0x1
#define HWIO_LPASS_LPAIF_PCM_CTLa_RPCM_WIDTH_BMSK                                        0x800
#define HWIO_LPASS_LPAIF_PCM_CTLa_RPCM_WIDTH_SHFT                                           11
#define HWIO_LPASS_LPAIF_PCM_CTLa_RPCM_WIDTH_ENUM_8_BIT_FVAL                               0x0
#define HWIO_LPASS_LPAIF_PCM_CTLa_RPCM_WIDTH_ENUM_16_BIT_FVAL                              0x1
#define HWIO_LPASS_LPAIF_PCM_CTLa_TPCM_WIDTH_BMSK                                        0x400
#define HWIO_LPASS_LPAIF_PCM_CTLa_TPCM_WIDTH_SHFT                                           10
#define HWIO_LPASS_LPAIF_PCM_CTLa_TPCM_WIDTH_ENUM_8_BIT_FVAL                               0x0
#define HWIO_LPASS_LPAIF_PCM_CTLa_TPCM_WIDTH_ENUM_16_BIT_FVAL                              0x1

#define HWIO_LPASS_LPAIF_PCM_TDM_CTL_a_ADDR(base,a)                                 ((base) + 0X1518 + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_PCM_TDM_CTL_a_OFFS(a)                                      (0X1518 + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_PCM_TDM_CTL_a_RMSK                                         0x7e07ffff
#define HWIO_LPASS_LPAIF_PCM_TDM_CTL_a_MAXa                                                  2
#define HWIO_LPASS_LPAIF_PCM_TDM_CTL_a_INI(base,a)                \
                in_dword_masked(HWIO_LPASS_LPAIF_PCM_TDM_CTL_a_ADDR(base,a), HWIO_LPASS_LPAIF_PCM_TDM_CTL_a_RMSK)
#define HWIO_LPASS_LPAIF_PCM_TDM_CTL_a_INMI(base,a,mask)        \
                in_dword_masked(HWIO_LPASS_LPAIF_PCM_TDM_CTL_a_ADDR(base,a), mask)
#define HWIO_LPASS_LPAIF_PCM_TDM_CTL_a_OUTI(base,a,val)        \
                out_dword(HWIO_LPASS_LPAIF_PCM_TDM_CTL_a_ADDR(base,a),val)
#define HWIO_LPASS_LPAIF_PCM_TDM_CTL_a_OUTMI(base,a,mask,val) \
                out_dword_masked_ns(HWIO_LPASS_LPAIF_PCM_TDM_CTL_a_ADDR(base,a),mask,val,HWIO_LPASS_LPAIF_PCM_TDM_CTL_a_INI(base,a))
#define HWIO_LPASS_LPAIF_PCM_TDM_CTL_a_ENABLE_TDM_BMSK                              0x40000000
#define HWIO_LPASS_LPAIF_PCM_TDM_CTL_a_ENABLE_TDM_SHFT                                      30
#define HWIO_LPASS_LPAIF_PCM_TDM_CTL_a_ENABLE_TDM_DISABLE_FVAL                             0x0
#define HWIO_LPASS_LPAIF_PCM_TDM_CTL_a_ENABLE_TDM_ENABLE_FVAL                              0x1
#define HWIO_LPASS_LPAIF_PCM_TDM_CTL_a_ENABLE_DIFF_SAMPLE_WIDTH_BMSK                0x20000000
#define HWIO_LPASS_LPAIF_PCM_TDM_CTL_a_ENABLE_DIFF_SAMPLE_WIDTH_SHFT                        29
#define HWIO_LPASS_LPAIF_PCM_TDM_CTL_a_ENABLE_DIFF_SAMPLE_WIDTH_DISABLE_FVAL               0x0
#define HWIO_LPASS_LPAIF_PCM_TDM_CTL_a_ENABLE_DIFF_SAMPLE_WIDTH_ENABLE_FVAL                0x1
#define HWIO_LPASS_LPAIF_PCM_TDM_CTL_a_TDM_INV_TPCM_SYNC_BMSK                       0x10000000
#define HWIO_LPASS_LPAIF_PCM_TDM_CTL_a_TDM_INV_TPCM_SYNC_SHFT                               28
#define HWIO_LPASS_LPAIF_PCM_TDM_CTL_a_TDM_INV_RPCM_SYNC_BMSK                        0x8000000
#define HWIO_LPASS_LPAIF_PCM_TDM_CTL_a_TDM_INV_RPCM_SYNC_SHFT                               27
#define HWIO_LPASS_LPAIF_PCM_TDM_CTL_a_TDM_SYNC_DELAY_BMSK                           0x6000000
#define HWIO_LPASS_LPAIF_PCM_TDM_CTL_a_TDM_SYNC_DELAY_SHFT                                  25
#define HWIO_LPASS_LPAIF_PCM_TDM_CTL_a_TDM_SYNC_DELAY_DELAY_2_CYCLE_FVAL                   0x0
#define HWIO_LPASS_LPAIF_PCM_TDM_CTL_a_TDM_SYNC_DELAY_DELAY_1_CYCLE_FVAL                   0x1
#define HWIO_LPASS_LPAIF_PCM_TDM_CTL_a_TDM_SYNC_DELAY_DELAY_0_CYCLE_FVAL                   0x2
#define HWIO_LPASS_LPAIF_PCM_TDM_CTL_a_TDM_TPCM_WIDTH_BMSK                             0x7c000
#define HWIO_LPASS_LPAIF_PCM_TDM_CTL_a_TDM_TPCM_WIDTH_SHFT                                  14
#define HWIO_LPASS_LPAIF_PCM_TDM_CTL_a_TDM_RPCM_WIDTH_BMSK                              0x3e00
#define HWIO_LPASS_LPAIF_PCM_TDM_CTL_a_TDM_RPCM_WIDTH_SHFT                                   9
#define HWIO_LPASS_LPAIF_PCM_TDM_CTL_a_TDM_RATE_BMSK                                     0x1ff
#define HWIO_LPASS_LPAIF_PCM_TDM_CTL_a_TDM_RATE_SHFT                                         0

#define HWIO_LPASS_LPAIF_PCM_TDM_SAMPLE_WIDTH_a_ADDR(base,a)                        ((base) + 0X151C + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_PCM_TDM_SAMPLE_WIDTH_a_OFFS(a)                             (0X151C + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_PCM_TDM_SAMPLE_WIDTH_a_RMSK                                     0x3ff
#define HWIO_LPASS_LPAIF_PCM_TDM_SAMPLE_WIDTH_a_MAXa                                         2
#define HWIO_LPASS_LPAIF_PCM_TDM_SAMPLE_WIDTH_a_INI(base,a)                \
                in_dword_masked(HWIO_LPASS_LPAIF_PCM_TDM_SAMPLE_WIDTH_a_ADDR(base,a), HWIO_LPASS_LPAIF_PCM_TDM_SAMPLE_WIDTH_a_RMSK)
#define HWIO_LPASS_LPAIF_PCM_TDM_SAMPLE_WIDTH_a_INMI(base,a,mask)        \
                in_dword_masked(HWIO_LPASS_LPAIF_PCM_TDM_SAMPLE_WIDTH_a_ADDR(base,a), mask)
#define HWIO_LPASS_LPAIF_PCM_TDM_SAMPLE_WIDTH_a_OUTI(base,a,val)        \
                out_dword(HWIO_LPASS_LPAIF_PCM_TDM_SAMPLE_WIDTH_a_ADDR(base,a),val)
#define HWIO_LPASS_LPAIF_PCM_TDM_SAMPLE_WIDTH_a_OUTMI(base,a,mask,val) \
                out_dword_masked_ns(HWIO_LPASS_LPAIF_PCM_TDM_SAMPLE_WIDTH_a_ADDR(base,a),mask,val,HWIO_LPASS_LPAIF_PCM_TDM_SAMPLE_WIDTH_a_INI(base,a))
#define HWIO_LPASS_LPAIF_PCM_TDM_SAMPLE_WIDTH_a_TDM_TPCM_SAMPLE_WIDTH_BMSK               0x3e0
#define HWIO_LPASS_LPAIF_PCM_TDM_SAMPLE_WIDTH_a_TDM_TPCM_SAMPLE_WIDTH_SHFT                   5
#define HWIO_LPASS_LPAIF_PCM_TDM_SAMPLE_WIDTH_a_TDM_RPCM_SAMPLE_WIDTH_BMSK                0x1f
#define HWIO_LPASS_LPAIF_PCM_TDM_SAMPLE_WIDTH_a_TDM_RPCM_SAMPLE_WIDTH_SHFT                   0

#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_ADDR(base,a)                           ((base) + 0X1520 + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_OFFS(a)                                (0X1520 + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RMSK                                   0xffffffff
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_MAXa                                            2
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_INI(base,a)                \
                in_dword_masked(HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_ADDR(base,a), HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RMSK)
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_INMI(base,a,mask)        \
                in_dword_masked(HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_ADDR(base,a), mask)
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_OUTI(base,a,val)        \
                out_dword(HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_ADDR(base,a),val)
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_OUTMI(base,a,mask,val) \
                out_dword_masked_ns(HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_ADDR(base,a),mask,val,HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_INI(base,a))
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT31_EN_BMSK                    0x80000000
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT31_EN_SHFT                            31
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT31_EN_DISABLE_FVAL                   0x0
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT31_EN_ENABLE_FVAL                    0x1
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT30_EN_BMSK                    0x40000000
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT30_EN_SHFT                            30
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT30_EN_DISABLE_FVAL                   0x0
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT30_EN_ENABLE_FVAL                    0x1
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT29_EN_BMSK                    0x20000000
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT29_EN_SHFT                            29
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT29_EN_DISABLE_FVAL                   0x0
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT29_EN_ENABLE_FVAL                    0x1
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT28_EN_BMSK                    0x10000000
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT28_EN_SHFT                            28
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT28_EN_DISABLE_FVAL                   0x0
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT28_EN_ENABLE_FVAL                    0x1
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT27_EN_BMSK                     0x8000000
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT27_EN_SHFT                            27
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT27_EN_DISABLE_FVAL                   0x0
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT27_EN_ENABLE_FVAL                    0x1
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT26_EN_BMSK                     0x4000000
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT26_EN_SHFT                            26
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT26_EN_DISABLE_FVAL                   0x0
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT26_EN_ENABLE_FVAL                    0x1
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT25_EN_BMSK                     0x2000000
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT25_EN_SHFT                            25
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT25_EN_DISABLE_FVAL                   0x0
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT25_EN_ENABLE_FVAL                    0x1
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT24_EN_BMSK                     0x1000000
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT24_EN_SHFT                            24
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT24_EN_DISABLE_FVAL                   0x0
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT24_EN_ENABLE_FVAL                    0x1
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT23_EN_BMSK                      0x800000
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT23_EN_SHFT                            23
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT23_EN_DISABLE_FVAL                   0x0
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT23_EN_ENABLE_FVAL                    0x1
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT22_EN_BMSK                      0x400000
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT22_EN_SHFT                            22
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT22_EN_DISABLE_FVAL                   0x0
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT22_EN_ENABLE_FVAL                    0x1
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT21_EN_BMSK                      0x200000
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT21_EN_SHFT                            21
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT21_EN_DISABLE_FVAL                   0x0
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT21_EN_ENABLE_FVAL                    0x1
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT20_EN_BMSK                      0x100000
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT20_EN_SHFT                            20
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT20_EN_DISABLE_FVAL                   0x0
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT20_EN_ENABLE_FVAL                    0x1
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT19_EN_BMSK                       0x80000
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT19_EN_SHFT                            19
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT19_EN_DISABLE_FVAL                   0x0
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT19_EN_ENABLE_FVAL                    0x1
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT18_EN_BMSK                       0x40000
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT18_EN_SHFT                            18
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT18_EN_DISABLE_FVAL                   0x0
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT18_EN_ENABLE_FVAL                    0x1
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT17_EN_BMSK                       0x20000
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT17_EN_SHFT                            17
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT17_EN_DISABLE_FVAL                   0x0
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT17_EN_ENABLE_FVAL                    0x1
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT16_EN_BMSK                       0x10000
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT16_EN_SHFT                            16
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT16_EN_DISABLE_FVAL                   0x0
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT16_EN_ENABLE_FVAL                    0x1
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT15_EN_BMSK                        0x8000
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT15_EN_SHFT                            15
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT15_EN_DISABLE_FVAL                   0x0
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT15_EN_ENABLE_FVAL                    0x1
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT14_EN_BMSK                        0x4000
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT14_EN_SHFT                            14
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT14_EN_DISABLE_FVAL                   0x0
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT14_EN_ENABLE_FVAL                    0x1
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT13_EN_BMSK                        0x2000
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT13_EN_SHFT                            13
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT13_EN_DISABLE_FVAL                   0x0
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT13_EN_ENABLE_FVAL                    0x1
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT12_EN_BMSK                        0x1000
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT12_EN_SHFT                            12
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT12_EN_DISABLE_FVAL                   0x0
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT12_EN_ENABLE_FVAL                    0x1
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT11_EN_BMSK                         0x800
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT11_EN_SHFT                            11
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT11_EN_DISABLE_FVAL                   0x0
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT11_EN_ENABLE_FVAL                    0x1
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT10_EN_BMSK                         0x400
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT10_EN_SHFT                            10
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT10_EN_DISABLE_FVAL                   0x0
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT10_EN_ENABLE_FVAL                    0x1
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT9_EN_BMSK                          0x200
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT9_EN_SHFT                              9
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT9_EN_DISABLE_FVAL                    0x0
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT9_EN_ENABLE_FVAL                     0x1
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT8_EN_BMSK                          0x100
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT8_EN_SHFT                              8
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT8_EN_DISABLE_FVAL                    0x0
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT8_EN_ENABLE_FVAL                     0x1
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT7_EN_BMSK                           0x80
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT7_EN_SHFT                              7
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT7_EN_DISABLE_FVAL                    0x0
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT7_EN_ENABLE_FVAL                     0x1
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT6_EN_BMSK                           0x40
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT6_EN_SHFT                              6
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT6_EN_DISABLE_FVAL                    0x0
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT6_EN_ENABLE_FVAL                     0x1
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT5_EN_BMSK                           0x20
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT5_EN_SHFT                              5
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT5_EN_DISABLE_FVAL                    0x0
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT5_EN_ENABLE_FVAL                     0x1
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT4_EN_BMSK                           0x10
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT4_EN_SHFT                              4
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT4_EN_DISABLE_FVAL                    0x0
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT4_EN_ENABLE_FVAL                     0x1
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT3_EN_BMSK                            0x8
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT3_EN_SHFT                              3
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT3_EN_DISABLE_FVAL                    0x0
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT3_EN_ENABLE_FVAL                     0x1
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT2_EN_BMSK                            0x4
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT2_EN_SHFT                              2
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT2_EN_DISABLE_FVAL                    0x0
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT2_EN_ENABLE_FVAL                     0x1
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT1_EN_BMSK                            0x2
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT1_EN_SHFT                              1
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT1_EN_DISABLE_FVAL                    0x0
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT1_EN_ENABLE_FVAL                     0x1
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT0_EN_BMSK                            0x1
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT0_EN_SHFT                              0
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT0_EN_DISABLE_FVAL                    0x0
#define HWIO_LPASS_LPAIF_PCM_RPCM_SLOT_NUM_a_RPCM_SLOT0_EN_ENABLE_FVAL                     0x1

#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_ADDR(base,a)                           ((base) + 0X1524 + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_OFFS(a)                                (0X1524 + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_RMSK                                   0xffffffff
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_MAXa                                            2
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_INI(base,a)                \
                in_dword_masked(HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_ADDR(base,a), HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_RMSK)
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_INMI(base,a,mask)        \
                in_dword_masked(HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_ADDR(base,a), mask)
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_OUTI(base,a,val)        \
                out_dword(HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_ADDR(base,a),val)
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_OUTMI(base,a,mask,val) \
                out_dword_masked_ns(HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_ADDR(base,a),mask,val,HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_INI(base,a))
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT31_EN_BMSK                    0x80000000
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT31_EN_SHFT                            31
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT31_EN_DISABLE_FVAL                   0x0
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT31_EN_ENABLE_FVAL                    0x1
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT30_EN_BMSK                    0x40000000
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT30_EN_SHFT                            30
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT30_EN_DISABLE_FVAL                   0x0
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT30_EN_ENABLE_FVAL                    0x1
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT29_EN_BMSK                    0x20000000
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT29_EN_SHFT                            29
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT29_EN_DISABLE_FVAL                   0x0
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT29_EN_ENABLE_FVAL                    0x1
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT28_EN_BMSK                    0x10000000
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT28_EN_SHFT                            28
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT28_EN_DISABLE_FVAL                   0x0
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT28_EN_ENABLE_FVAL                    0x1
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT27_EN_BMSK                     0x8000000
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT27_EN_SHFT                            27
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT27_EN_DISABLE_FVAL                   0x0
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT27_EN_ENABLE_FVAL                    0x1
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT26_EN_BMSK                     0x4000000
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT26_EN_SHFT                            26
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT26_EN_DISABLE_FVAL                   0x0
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT26_EN_ENABLE_FVAL                    0x1
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT25_EN_BMSK                     0x2000000
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT25_EN_SHFT                            25
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT25_EN_DISABLE_FVAL                   0x0
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT25_EN_ENABLE_FVAL                    0x1
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT24_EN_BMSK                     0x1000000
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT24_EN_SHFT                            24
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT24_EN_DISABLE_FVAL                   0x0
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT24_EN_ENABLE_FVAL                    0x1
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT23_EN_BMSK                      0x800000
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT23_EN_SHFT                            23
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT23_EN_DISABLE_FVAL                   0x0
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT23_EN_ENABLE_FVAL                    0x1
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT22_EN_BMSK                      0x400000
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT22_EN_SHFT                            22
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT22_EN_DISABLE_FVAL                   0x0
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT22_EN_ENABLE_FVAL                    0x1
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT21_EN_BMSK                      0x200000
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT21_EN_SHFT                            21
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT21_EN_DISABLE_FVAL                   0x0
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT21_EN_ENABLE_FVAL                    0x1
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT20_EN_BMSK                      0x100000
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT20_EN_SHFT                            20
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT20_EN_DISABLE_FVAL                   0x0
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT20_EN_ENABLE_FVAL                    0x1
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT19_EN_BMSK                       0x80000
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT19_EN_SHFT                            19
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT19_EN_DISABLE_FVAL                   0x0
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT19_EN_ENABLE_FVAL                    0x1
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT18_EN_BMSK                       0x40000
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT18_EN_SHFT                            18
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT18_EN_DISABLE_FVAL                   0x0
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT18_EN_ENABLE_FVAL                    0x1
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT17_EN_BMSK                       0x20000
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT17_EN_SHFT                            17
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT17_EN_DISABLE_FVAL                   0x0
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT17_EN_ENABLE_FVAL                    0x1
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT16_EN_BMSK                       0x10000
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT16_EN_SHFT                            16
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT16_EN_DISABLE_FVAL                   0x0
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT16_EN_ENABLE_FVAL                    0x1
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT15_EN_BMSK                        0x8000
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT15_EN_SHFT                            15
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT15_EN_DISABLE_FVAL                   0x0
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT15_EN_ENABLE_FVAL                    0x1
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT14_EN_BMSK                        0x4000
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT14_EN_SHFT                            14
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT14_EN_DISABLE_FVAL                   0x0
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT14_EN_ENABLE_FVAL                    0x1
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT13_EN_BMSK                        0x2000
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT13_EN_SHFT                            13
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT13_EN_DISABLE_FVAL                   0x0
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT13_EN_ENABLE_FVAL                    0x1
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT12_EN_BMSK                        0x1000
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT12_EN_SHFT                            12
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT12_EN_DISABLE_FVAL                   0x0
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT12_EN_ENABLE_FVAL                    0x1
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT11_EN_BMSK                         0x800
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT11_EN_SHFT                            11
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT11_EN_DISABLE_FVAL                   0x0
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT11_EN_ENABLE_FVAL                    0x1
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT10_EN_BMSK                         0x400
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT10_EN_SHFT                            10
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT10_EN_DISABLE_FVAL                   0x0
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT10_EN_ENABLE_FVAL                    0x1
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT9_EN_BMSK                          0x200
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT9_EN_SHFT                              9
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT9_EN_DISABLE_FVAL                    0x0
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT9_EN_ENABLE_FVAL                     0x1
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT8_EN_BMSK                          0x100
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT8_EN_SHFT                              8
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT8_EN_DISABLE_FVAL                    0x0
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT8_EN_ENABLE_FVAL                     0x1
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT7_EN_BMSK                           0x80
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT7_EN_SHFT                              7
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT7_EN_DISABLE_FVAL                    0x0
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT7_EN_ENABLE_FVAL                     0x1
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT6_EN_BMSK                           0x40
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT6_EN_SHFT                              6
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT6_EN_DISABLE_FVAL                    0x0
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT6_EN_ENABLE_FVAL                     0x1
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT5_EN_BMSK                           0x20
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT5_EN_SHFT                              5
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT5_EN_DISABLE_FVAL                    0x0
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT5_EN_ENABLE_FVAL                     0x1
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT4_EN_BMSK                           0x10
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT4_EN_SHFT                              4
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT4_EN_DISABLE_FVAL                    0x0
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT4_EN_ENABLE_FVAL                     0x1
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT3_EN_BMSK                            0x8
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT3_EN_SHFT                              3
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT3_EN_DISABLE_FVAL                    0x0
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT3_EN_ENABLE_FVAL                     0x1
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT2_EN_BMSK                            0x4
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT2_EN_SHFT                              2
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT2_EN_DISABLE_FVAL                    0x0
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT2_EN_ENABLE_FVAL                     0x1
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT1_EN_BMSK                            0x2
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT1_EN_SHFT                              1
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT1_EN_DISABLE_FVAL                    0x0
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT1_EN_ENABLE_FVAL                     0x1
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT0_EN_BMSK                            0x1
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT0_EN_SHFT                              0
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT0_EN_DISABLE_FVAL                    0x0
#define HWIO_LPASS_LPAIF_PCM_TPCM_SLOT_NUM_a_TPCM_SLOT0_EN_ENABLE_FVAL                     0x1

#define HWIO_LPASS_LPAIF_PCM_LANE_CONFIG_a_ADDR(base,a)                             ((base) + 0X1528 + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_PCM_LANE_CONFIG_a_OFFS(a)                                  (0X1528 + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_PCM_LANE_CONFIG_a_RMSK                                       0xff00ff
#define HWIO_LPASS_LPAIF_PCM_LANE_CONFIG_a_MAXa                                              2
#define HWIO_LPASS_LPAIF_PCM_LANE_CONFIG_a_INI(base,a)                \
                in_dword_masked(HWIO_LPASS_LPAIF_PCM_LANE_CONFIG_a_ADDR(base,a), HWIO_LPASS_LPAIF_PCM_LANE_CONFIG_a_RMSK)
#define HWIO_LPASS_LPAIF_PCM_LANE_CONFIG_a_INMI(base,a,mask)        \
                in_dword_masked(HWIO_LPASS_LPAIF_PCM_LANE_CONFIG_a_ADDR(base,a), mask)
#define HWIO_LPASS_LPAIF_PCM_LANE_CONFIG_a_OUTI(base,a,val)        \
                out_dword(HWIO_LPASS_LPAIF_PCM_LANE_CONFIG_a_ADDR(base,a),val)
#define HWIO_LPASS_LPAIF_PCM_LANE_CONFIG_a_OUTMI(base,a,mask,val) \
                out_dword_masked_ns(HWIO_LPASS_LPAIF_PCM_LANE_CONFIG_a_ADDR(base,a),mask,val,HWIO_LPASS_LPAIF_PCM_LANE_CONFIG_a_INI(base,a))
#define HWIO_LPASS_LPAIF_PCM_LANE_CONFIG_a_LANE7_EN_BMSK                              0x800000
#define HWIO_LPASS_LPAIF_PCM_LANE_CONFIG_a_LANE7_EN_SHFT                                    23
#define HWIO_LPASS_LPAIF_PCM_LANE_CONFIG_a_LANE7_EN_DISABLE_FVAL                           0x0
#define HWIO_LPASS_LPAIF_PCM_LANE_CONFIG_a_LANE7_EN_ENABLE_FVAL                            0x1
#define HWIO_LPASS_LPAIF_PCM_LANE_CONFIG_a_LANE6_EN_BMSK                              0x400000
#define HWIO_LPASS_LPAIF_PCM_LANE_CONFIG_a_LANE6_EN_SHFT                                    22
#define HWIO_LPASS_LPAIF_PCM_LANE_CONFIG_a_LANE6_EN_DISABLE_FVAL                           0x0
#define HWIO_LPASS_LPAIF_PCM_LANE_CONFIG_a_LANE6_EN_ENABLE_FVAL                            0x1
#define HWIO_LPASS_LPAIF_PCM_LANE_CONFIG_a_LANE5_EN_BMSK                              0x200000
#define HWIO_LPASS_LPAIF_PCM_LANE_CONFIG_a_LANE5_EN_SHFT                                    21
#define HWIO_LPASS_LPAIF_PCM_LANE_CONFIG_a_LANE5_EN_DISABLE_FVAL                           0x0
#define HWIO_LPASS_LPAIF_PCM_LANE_CONFIG_a_LANE5_EN_ENABLE_FVAL                            0x1
#define HWIO_LPASS_LPAIF_PCM_LANE_CONFIG_a_LANE4_EN_BMSK                              0x100000
#define HWIO_LPASS_LPAIF_PCM_LANE_CONFIG_a_LANE4_EN_SHFT                                    20
#define HWIO_LPASS_LPAIF_PCM_LANE_CONFIG_a_LANE4_EN_DISABLE_FVAL                           0x0
#define HWIO_LPASS_LPAIF_PCM_LANE_CONFIG_a_LANE4_EN_ENABLE_FVAL                            0x1
#define HWIO_LPASS_LPAIF_PCM_LANE_CONFIG_a_LANE3_EN_BMSK                               0x80000
#define HWIO_LPASS_LPAIF_PCM_LANE_CONFIG_a_LANE3_EN_SHFT                                    19
#define HWIO_LPASS_LPAIF_PCM_LANE_CONFIG_a_LANE3_EN_DISABLE_FVAL                           0x0
#define HWIO_LPASS_LPAIF_PCM_LANE_CONFIG_a_LANE3_EN_ENABLE_FVAL                            0x1
#define HWIO_LPASS_LPAIF_PCM_LANE_CONFIG_a_LANE2_EN_BMSK                               0x40000
#define HWIO_LPASS_LPAIF_PCM_LANE_CONFIG_a_LANE2_EN_SHFT                                    18
#define HWIO_LPASS_LPAIF_PCM_LANE_CONFIG_a_LANE2_EN_DISABLE_FVAL                           0x0
#define HWIO_LPASS_LPAIF_PCM_LANE_CONFIG_a_LANE2_EN_ENABLE_FVAL                            0x1
#define HWIO_LPASS_LPAIF_PCM_LANE_CONFIG_a_LANE1_EN_BMSK                               0x20000
#define HWIO_LPASS_LPAIF_PCM_LANE_CONFIG_a_LANE1_EN_SHFT                                    17
#define HWIO_LPASS_LPAIF_PCM_LANE_CONFIG_a_LANE1_EN_DISABLE_FVAL                           0x0
#define HWIO_LPASS_LPAIF_PCM_LANE_CONFIG_a_LANE1_EN_ENABLE_FVAL                            0x1
#define HWIO_LPASS_LPAIF_PCM_LANE_CONFIG_a_LANE0_EN_BMSK                               0x10000
#define HWIO_LPASS_LPAIF_PCM_LANE_CONFIG_a_LANE0_EN_SHFT                                    16
#define HWIO_LPASS_LPAIF_PCM_LANE_CONFIG_a_LANE0_EN_DISABLE_FVAL                           0x0
#define HWIO_LPASS_LPAIF_PCM_LANE_CONFIG_a_LANE0_EN_ENABLE_FVAL                            0x1
#define HWIO_LPASS_LPAIF_PCM_LANE_CONFIG_a_LANE7_DIR_BMSK                                 0x80
#define HWIO_LPASS_LPAIF_PCM_LANE_CONFIG_a_LANE7_DIR_SHFT                                    7
#define HWIO_LPASS_LPAIF_PCM_LANE_CONFIG_a_LANE7_DIR_SPKR_FVAL                             0x0
#define HWIO_LPASS_LPAIF_PCM_LANE_CONFIG_a_LANE7_DIR_MIC_FVAL                              0x1
#define HWIO_LPASS_LPAIF_PCM_LANE_CONFIG_a_LANE6_DIR_BMSK                                 0x40
#define HWIO_LPASS_LPAIF_PCM_LANE_CONFIG_a_LANE6_DIR_SHFT                                    6
#define HWIO_LPASS_LPAIF_PCM_LANE_CONFIG_a_LANE6_DIR_SPKR_FVAL                             0x0
#define HWIO_LPASS_LPAIF_PCM_LANE_CONFIG_a_LANE6_DIR_MIC_FVAL                              0x1
#define HWIO_LPASS_LPAIF_PCM_LANE_CONFIG_a_LANE5_DIR_BMSK                                 0x20
#define HWIO_LPASS_LPAIF_PCM_LANE_CONFIG_a_LANE5_DIR_SHFT                                    5
#define HWIO_LPASS_LPAIF_PCM_LANE_CONFIG_a_LANE5_DIR_SPKR_FVAL                             0x0
#define HWIO_LPASS_LPAIF_PCM_LANE_CONFIG_a_LANE5_DIR_MIC_FVAL                              0x1
#define HWIO_LPASS_LPAIF_PCM_LANE_CONFIG_a_LANE4_DIR_BMSK                                 0x10
#define HWIO_LPASS_LPAIF_PCM_LANE_CONFIG_a_LANE4_DIR_SHFT                                    4
#define HWIO_LPASS_LPAIF_PCM_LANE_CONFIG_a_LANE4_DIR_SPKR_FVAL                             0x0
#define HWIO_LPASS_LPAIF_PCM_LANE_CONFIG_a_LANE4_DIR_MIC_FVAL                              0x1
#define HWIO_LPASS_LPAIF_PCM_LANE_CONFIG_a_LANE3_DIR_BMSK                                  0x8
#define HWIO_LPASS_LPAIF_PCM_LANE_CONFIG_a_LANE3_DIR_SHFT                                    3
#define HWIO_LPASS_LPAIF_PCM_LANE_CONFIG_a_LANE3_DIR_SPKR_FVAL                             0x0
#define HWIO_LPASS_LPAIF_PCM_LANE_CONFIG_a_LANE3_DIR_MIC_FVAL                              0x1
#define HWIO_LPASS_LPAIF_PCM_LANE_CONFIG_a_LANE2_DIR_BMSK                                  0x4
#define HWIO_LPASS_LPAIF_PCM_LANE_CONFIG_a_LANE2_DIR_SHFT                                    2
#define HWIO_LPASS_LPAIF_PCM_LANE_CONFIG_a_LANE2_DIR_SPKR_FVAL                             0x0
#define HWIO_LPASS_LPAIF_PCM_LANE_CONFIG_a_LANE2_DIR_MIC_FVAL                              0x1
#define HWIO_LPASS_LPAIF_PCM_LANE_CONFIG_a_LANE1_DIR_BMSK                                  0x2
#define HWIO_LPASS_LPAIF_PCM_LANE_CONFIG_a_LANE1_DIR_SHFT                                    1
#define HWIO_LPASS_LPAIF_PCM_LANE_CONFIG_a_LANE1_DIR_SPKR_FVAL                             0x0
#define HWIO_LPASS_LPAIF_PCM_LANE_CONFIG_a_LANE1_DIR_MIC_FVAL                              0x1
#define HWIO_LPASS_LPAIF_PCM_LANE_CONFIG_a_LANE0_DIR_BMSK                                  0x1
#define HWIO_LPASS_LPAIF_PCM_LANE_CONFIG_a_LANE0_DIR_SHFT                                    0
#define HWIO_LPASS_LPAIF_PCM_LANE_CONFIG_a_LANE0_DIR_SPKR_FVAL                             0x0
#define HWIO_LPASS_LPAIF_PCM_LANE_CONFIG_a_LANE0_DIR_MIC_FVAL                              0x1

#define HWIO_LPASS_LPAIF_IRQ_ENa_ADDR(base,a)                                       ((base) + 0X9000 + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_IRQ_ENa_OFFS(a)                                            (0X9000 + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_IRQ_ENa_RMSK                                               0xffffffff
#define HWIO_LPASS_LPAIF_IRQ_ENa_MAXa                                                        2
#define HWIO_LPASS_LPAIF_IRQ_ENa_INI(base,a)                \
                in_dword_masked(HWIO_LPASS_LPAIF_IRQ_ENa_ADDR(base,a), HWIO_LPASS_LPAIF_IRQ_ENa_RMSK)
#define HWIO_LPASS_LPAIF_IRQ_ENa_INMI(base,a,mask)        \
                in_dword_masked(HWIO_LPASS_LPAIF_IRQ_ENa_ADDR(base,a), mask)
#define HWIO_LPASS_LPAIF_IRQ_ENa_OUTI(base,a,val)        \
                out_dword(HWIO_LPASS_LPAIF_IRQ_ENa_ADDR(base,a),val)
#define HWIO_LPASS_LPAIF_IRQ_ENa_OUTMI(base,a,mask,val) \
                out_dword_masked_ns(HWIO_LPASS_LPAIF_IRQ_ENa_ADDR(base,a),mask,val,HWIO_LPASS_LPAIF_IRQ_ENa_INI(base,a))
#define HWIO_LPASS_LPAIF_IRQ_ENa_SEC_RD_NO_RATE_BMSK                                0x80000000
#define HWIO_LPASS_LPAIF_IRQ_ENa_SEC_RD_NO_RATE_SHFT                                        31
#define HWIO_LPASS_LPAIF_IRQ_ENa_SEC_RD_DIFF_RATE_BMSK                              0x40000000
#define HWIO_LPASS_LPAIF_IRQ_ENa_SEC_RD_DIFF_RATE_SHFT                                      30
#define HWIO_LPASS_LPAIF_IRQ_ENa_PRI_RD_NO_RATE_BMSK                                0x20000000
#define HWIO_LPASS_LPAIF_IRQ_ENa_PRI_RD_NO_RATE_SHFT                                        29
#define HWIO_LPASS_LPAIF_IRQ_ENa_PRI_RD_DIFF_RATE_BMSK                              0x10000000
#define HWIO_LPASS_LPAIF_IRQ_ENa_PRI_RD_DIFF_RATE_SHFT                                      28
#define HWIO_LPASS_LPAIF_IRQ_ENa_FRM_REF_BMSK                                        0x8000000
#define HWIO_LPASS_LPAIF_IRQ_ENa_FRM_REF_SHFT                                               27
#define HWIO_LPASS_LPAIF_IRQ_ENa_ERR_WRDMA_CH3_BMSK                                  0x4000000
#define HWIO_LPASS_LPAIF_IRQ_ENa_ERR_WRDMA_CH3_SHFT                                         26
#define HWIO_LPASS_LPAIF_IRQ_ENa_OVR_WRDMA_CH3_BMSK                                  0x2000000
#define HWIO_LPASS_LPAIF_IRQ_ENa_OVR_WRDMA_CH3_SHFT                                         25
#define HWIO_LPASS_LPAIF_IRQ_ENa_PER_WRDMA_CH3_BMSK                                  0x1000000
#define HWIO_LPASS_LPAIF_IRQ_ENa_PER_WRDMA_CH3_SHFT                                         24
#define HWIO_LPASS_LPAIF_IRQ_ENa_ERR_WRDMA_CH2_BMSK                                   0x800000
#define HWIO_LPASS_LPAIF_IRQ_ENa_ERR_WRDMA_CH2_SHFT                                         23
#define HWIO_LPASS_LPAIF_IRQ_ENa_OVR_WRDMA_CH2_BMSK                                   0x400000
#define HWIO_LPASS_LPAIF_IRQ_ENa_OVR_WRDMA_CH2_SHFT                                         22
#define HWIO_LPASS_LPAIF_IRQ_ENa_PER_WRDMA_CH2_BMSK                                   0x200000
#define HWIO_LPASS_LPAIF_IRQ_ENa_PER_WRDMA_CH2_SHFT                                         21
#define HWIO_LPASS_LPAIF_IRQ_ENa_ERR_WRDMA_CH1_BMSK                                   0x100000
#define HWIO_LPASS_LPAIF_IRQ_ENa_ERR_WRDMA_CH1_SHFT                                         20
#define HWIO_LPASS_LPAIF_IRQ_ENa_OVR_WRDMA_CH1_BMSK                                    0x80000
#define HWIO_LPASS_LPAIF_IRQ_ENa_OVR_WRDMA_CH1_SHFT                                         19
#define HWIO_LPASS_LPAIF_IRQ_ENa_PER_WRDMA_CH1_BMSK                                    0x40000
#define HWIO_LPASS_LPAIF_IRQ_ENa_PER_WRDMA_CH1_SHFT                                         18
#define HWIO_LPASS_LPAIF_IRQ_ENa_ERR_WRDMA_CH0_BMSK                                    0x20000
#define HWIO_LPASS_LPAIF_IRQ_ENa_ERR_WRDMA_CH0_SHFT                                         17
#define HWIO_LPASS_LPAIF_IRQ_ENa_OVR_WRDMA_CH0_BMSK                                    0x10000
#define HWIO_LPASS_LPAIF_IRQ_ENa_OVR_WRDMA_CH0_SHFT                                         16
#define HWIO_LPASS_LPAIF_IRQ_ENa_PER_WRDMA_CH0_BMSK                                     0x8000
#define HWIO_LPASS_LPAIF_IRQ_ENa_PER_WRDMA_CH0_SHFT                                         15
#define HWIO_LPASS_LPAIF_IRQ_ENa_ERR_RDDMA_CH4_BMSK                                     0x4000
#define HWIO_LPASS_LPAIF_IRQ_ENa_ERR_RDDMA_CH4_SHFT                                         14
#define HWIO_LPASS_LPAIF_IRQ_ENa_UNDR_RDDMA_CH4_BMSK                                    0x2000
#define HWIO_LPASS_LPAIF_IRQ_ENa_UNDR_RDDMA_CH4_SHFT                                        13
#define HWIO_LPASS_LPAIF_IRQ_ENa_PER_RDDMA_CH4_BMSK                                     0x1000
#define HWIO_LPASS_LPAIF_IRQ_ENa_PER_RDDMA_CH4_SHFT                                         12
#define HWIO_LPASS_LPAIF_IRQ_ENa_ERR_RDDMA_CH3_BMSK                                      0x800
#define HWIO_LPASS_LPAIF_IRQ_ENa_ERR_RDDMA_CH3_SHFT                                         11
#define HWIO_LPASS_LPAIF_IRQ_ENa_UNDR_RDDMA_CH3_BMSK                                     0x400
#define HWIO_LPASS_LPAIF_IRQ_ENa_UNDR_RDDMA_CH3_SHFT                                        10
#define HWIO_LPASS_LPAIF_IRQ_ENa_PER_RDDMA_CH3_BMSK                                      0x200
#define HWIO_LPASS_LPAIF_IRQ_ENa_PER_RDDMA_CH3_SHFT                                          9
#define HWIO_LPASS_LPAIF_IRQ_ENa_ERR_RDDMA_CH2_BMSK                                      0x100
#define HWIO_LPASS_LPAIF_IRQ_ENa_ERR_RDDMA_CH2_SHFT                                          8
#define HWIO_LPASS_LPAIF_IRQ_ENa_UNDR_RDDMA_CH2_BMSK                                      0x80
#define HWIO_LPASS_LPAIF_IRQ_ENa_UNDR_RDDMA_CH2_SHFT                                         7
#define HWIO_LPASS_LPAIF_IRQ_ENa_PER_RDDMA_CH2_BMSK                                       0x40
#define HWIO_LPASS_LPAIF_IRQ_ENa_PER_RDDMA_CH2_SHFT                                          6
#define HWIO_LPASS_LPAIF_IRQ_ENa_ERR_RDDMA_CH1_BMSK                                       0x20
#define HWIO_LPASS_LPAIF_IRQ_ENa_ERR_RDDMA_CH1_SHFT                                          5
#define HWIO_LPASS_LPAIF_IRQ_ENa_UNDR_RDDMA_CH1_BMSK                                      0x10
#define HWIO_LPASS_LPAIF_IRQ_ENa_UNDR_RDDMA_CH1_SHFT                                         4
#define HWIO_LPASS_LPAIF_IRQ_ENa_PER_RDDMA_CH1_BMSK                                        0x8
#define HWIO_LPASS_LPAIF_IRQ_ENa_PER_RDDMA_CH1_SHFT                                          3
#define HWIO_LPASS_LPAIF_IRQ_ENa_ERR_RDDMA_CH0_BMSK                                        0x4
#define HWIO_LPASS_LPAIF_IRQ_ENa_ERR_RDDMA_CH0_SHFT                                          2
#define HWIO_LPASS_LPAIF_IRQ_ENa_UNDR_RDDMA_CH0_BMSK                                       0x2
#define HWIO_LPASS_LPAIF_IRQ_ENa_UNDR_RDDMA_CH0_SHFT                                         1
#define HWIO_LPASS_LPAIF_IRQ_ENa_PER_RDDMA_CH0_BMSK                                        0x1
#define HWIO_LPASS_LPAIF_IRQ_ENa_PER_RDDMA_CH0_SHFT                                          0

#define HWIO_LPASS_LPAIF_IRQ_STATa_ADDR(base,a)                                     ((base) + 0X9004 + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_IRQ_STATa_OFFS(a)                                          (0X9004 + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_IRQ_STATa_RMSK                                             0xffffffff
#define HWIO_LPASS_LPAIF_IRQ_STATa_MAXa                                                      2
#define HWIO_LPASS_LPAIF_IRQ_STATa_INI(base,a)                \
                in_dword_masked(HWIO_LPASS_LPAIF_IRQ_STATa_ADDR(base,a), HWIO_LPASS_LPAIF_IRQ_STATa_RMSK)
#define HWIO_LPASS_LPAIF_IRQ_STATa_INMI(base,a,mask)        \
                in_dword_masked(HWIO_LPASS_LPAIF_IRQ_STATa_ADDR(base,a), mask)
#define HWIO_LPASS_LPAIF_IRQ_STATa_SEC_RD_NO_RATE_BMSK                              0x80000000
#define HWIO_LPASS_LPAIF_IRQ_STATa_SEC_RD_NO_RATE_SHFT                                      31
#define HWIO_LPASS_LPAIF_IRQ_STATa_SEC_RD_DIFF_RATE_BMSK                            0x40000000
#define HWIO_LPASS_LPAIF_IRQ_STATa_SEC_RD_DIFF_RATE_SHFT                                    30
#define HWIO_LPASS_LPAIF_IRQ_STATa_PRI_RD_NO_RATE_BMSK                              0x20000000
#define HWIO_LPASS_LPAIF_IRQ_STATa_PRI_RD_NO_RATE_SHFT                                      29
#define HWIO_LPASS_LPAIF_IRQ_STATa_PRI_RD_DIFF_RATE_BMSK                            0x10000000
#define HWIO_LPASS_LPAIF_IRQ_STATa_PRI_RD_DIFF_RATE_SHFT                                    28
#define HWIO_LPASS_LPAIF_IRQ_STATa_FRM_REF_BMSK                                      0x8000000
#define HWIO_LPASS_LPAIF_IRQ_STATa_FRM_REF_SHFT                                             27
#define HWIO_LPASS_LPAIF_IRQ_STATa_ERR_WRDMA_CH3_BMSK                                0x4000000
#define HWIO_LPASS_LPAIF_IRQ_STATa_ERR_WRDMA_CH3_SHFT                                       26
#define HWIO_LPASS_LPAIF_IRQ_STATa_OVR_WRDMA_CH3_BMSK                                0x2000000
#define HWIO_LPASS_LPAIF_IRQ_STATa_OVR_WRDMA_CH3_SHFT                                       25
#define HWIO_LPASS_LPAIF_IRQ_STATa_PER_WRDMA_CH3_BMSK                                0x1000000
#define HWIO_LPASS_LPAIF_IRQ_STATa_PER_WRDMA_CH3_SHFT                                       24
#define HWIO_LPASS_LPAIF_IRQ_STATa_ERR_WRDMA_CH2_BMSK                                 0x800000
#define HWIO_LPASS_LPAIF_IRQ_STATa_ERR_WRDMA_CH2_SHFT                                       23
#define HWIO_LPASS_LPAIF_IRQ_STATa_OVR_WRDMA_CH2_BMSK                                 0x400000
#define HWIO_LPASS_LPAIF_IRQ_STATa_OVR_WRDMA_CH2_SHFT                                       22
#define HWIO_LPASS_LPAIF_IRQ_STATa_PER_WRDMA_CH2_BMSK                                 0x200000
#define HWIO_LPASS_LPAIF_IRQ_STATa_PER_WRDMA_CH2_SHFT                                       21
#define HWIO_LPASS_LPAIF_IRQ_STATa_ERR_WRDMA_CH1_BMSK                                 0x100000
#define HWIO_LPASS_LPAIF_IRQ_STATa_ERR_WRDMA_CH1_SHFT                                       20
#define HWIO_LPASS_LPAIF_IRQ_STATa_OVR_WRDMA_CH1_BMSK                                  0x80000
#define HWIO_LPASS_LPAIF_IRQ_STATa_OVR_WRDMA_CH1_SHFT                                       19
#define HWIO_LPASS_LPAIF_IRQ_STATa_PER_WRDMA_CH1_BMSK                                  0x40000
#define HWIO_LPASS_LPAIF_IRQ_STATa_PER_WRDMA_CH1_SHFT                                       18
#define HWIO_LPASS_LPAIF_IRQ_STATa_ERR_WRDMA_CH0_BMSK                                  0x20000
#define HWIO_LPASS_LPAIF_IRQ_STATa_ERR_WRDMA_CH0_SHFT                                       17
#define HWIO_LPASS_LPAIF_IRQ_STATa_OVR_WRDMA_CH0_BMSK                                  0x10000
#define HWIO_LPASS_LPAIF_IRQ_STATa_OVR_WRDMA_CH0_SHFT                                       16
#define HWIO_LPASS_LPAIF_IRQ_STATa_PER_WRDMA_CH0_BMSK                                   0x8000
#define HWIO_LPASS_LPAIF_IRQ_STATa_PER_WRDMA_CH0_SHFT                                       15
#define HWIO_LPASS_LPAIF_IRQ_STATa_ERR_RDDMA_CH4_BMSK                                   0x4000
#define HWIO_LPASS_LPAIF_IRQ_STATa_ERR_RDDMA_CH4_SHFT                                       14
#define HWIO_LPASS_LPAIF_IRQ_STATa_UNDR_RDDMA_CH4_BMSK                                  0x2000
#define HWIO_LPASS_LPAIF_IRQ_STATa_UNDR_RDDMA_CH4_SHFT                                      13
#define HWIO_LPASS_LPAIF_IRQ_STATa_PER_RDDMA_CH4_BMSK                                   0x1000
#define HWIO_LPASS_LPAIF_IRQ_STATa_PER_RDDMA_CH4_SHFT                                       12
#define HWIO_LPASS_LPAIF_IRQ_STATa_ERR_RDDMA_CH3_BMSK                                    0x800
#define HWIO_LPASS_LPAIF_IRQ_STATa_ERR_RDDMA_CH3_SHFT                                       11
#define HWIO_LPASS_LPAIF_IRQ_STATa_UNDR_RDDMA_CH3_BMSK                                   0x400
#define HWIO_LPASS_LPAIF_IRQ_STATa_UNDR_RDDMA_CH3_SHFT                                      10
#define HWIO_LPASS_LPAIF_IRQ_STATa_PER_RDDMA_CH3_BMSK                                    0x200
#define HWIO_LPASS_LPAIF_IRQ_STATa_PER_RDDMA_CH3_SHFT                                        9
#define HWIO_LPASS_LPAIF_IRQ_STATa_ERR_RDDMA_CH2_BMSK                                    0x100
#define HWIO_LPASS_LPAIF_IRQ_STATa_ERR_RDDMA_CH2_SHFT                                        8
#define HWIO_LPASS_LPAIF_IRQ_STATa_UNDR_RDDMA_CH2_BMSK                                    0x80
#define HWIO_LPASS_LPAIF_IRQ_STATa_UNDR_RDDMA_CH2_SHFT                                       7
#define HWIO_LPASS_LPAIF_IRQ_STATa_PER_RDDMA_CH2_BMSK                                     0x40
#define HWIO_LPASS_LPAIF_IRQ_STATa_PER_RDDMA_CH2_SHFT                                        6
#define HWIO_LPASS_LPAIF_IRQ_STATa_ERR_RDDMA_CH1_BMSK                                     0x20
#define HWIO_LPASS_LPAIF_IRQ_STATa_ERR_RDDMA_CH1_SHFT                                        5
#define HWIO_LPASS_LPAIF_IRQ_STATa_UNDR_RDDMA_CH1_BMSK                                    0x10
#define HWIO_LPASS_LPAIF_IRQ_STATa_UNDR_RDDMA_CH1_SHFT                                       4
#define HWIO_LPASS_LPAIF_IRQ_STATa_PER_RDDMA_CH1_BMSK                                      0x8
#define HWIO_LPASS_LPAIF_IRQ_STATa_PER_RDDMA_CH1_SHFT                                        3
#define HWIO_LPASS_LPAIF_IRQ_STATa_ERR_RDDMA_CH0_BMSK                                      0x4
#define HWIO_LPASS_LPAIF_IRQ_STATa_ERR_RDDMA_CH0_SHFT                                        2
#define HWIO_LPASS_LPAIF_IRQ_STATa_UNDR_RDDMA_CH0_BMSK                                     0x2
#define HWIO_LPASS_LPAIF_IRQ_STATa_UNDR_RDDMA_CH0_SHFT                                       1
#define HWIO_LPASS_LPAIF_IRQ_STATa_PER_RDDMA_CH0_BMSK                                      0x1
#define HWIO_LPASS_LPAIF_IRQ_STATa_PER_RDDMA_CH0_SHFT                                        0

#define HWIO_LPASS_LPAIF_IRQ_RAW_STATa_ADDR(base,a)                                 ((base) + 0X9008 + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_IRQ_RAW_STATa_OFFS(a)                                      (0X9008 + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_IRQ_RAW_STATa_RMSK                                         0xffffffff
#define HWIO_LPASS_LPAIF_IRQ_RAW_STATa_MAXa                                                  2
#define HWIO_LPASS_LPAIF_IRQ_RAW_STATa_INI(base,a)                \
                in_dword_masked(HWIO_LPASS_LPAIF_IRQ_RAW_STATa_ADDR(base,a), HWIO_LPASS_LPAIF_IRQ_RAW_STATa_RMSK)
#define HWIO_LPASS_LPAIF_IRQ_RAW_STATa_INMI(base,a,mask)        \
                in_dword_masked(HWIO_LPASS_LPAIF_IRQ_RAW_STATa_ADDR(base,a), mask)
#define HWIO_LPASS_LPAIF_IRQ_RAW_STATa_SEC_RD_NO_RATE_BMSK                          0x80000000
#define HWIO_LPASS_LPAIF_IRQ_RAW_STATa_SEC_RD_NO_RATE_SHFT                                  31
#define HWIO_LPASS_LPAIF_IRQ_RAW_STATa_SEC_RD_DIFF_RATE_BMSK                        0x40000000
#define HWIO_LPASS_LPAIF_IRQ_RAW_STATa_SEC_RD_DIFF_RATE_SHFT                                30
#define HWIO_LPASS_LPAIF_IRQ_RAW_STATa_PRI_RD_NO_RATE_BMSK                          0x20000000
#define HWIO_LPASS_LPAIF_IRQ_RAW_STATa_PRI_RD_NO_RATE_SHFT                                  29
#define HWIO_LPASS_LPAIF_IRQ_RAW_STATa_PRI_RD_DIFF_RATE_BMSK                        0x10000000
#define HWIO_LPASS_LPAIF_IRQ_RAW_STATa_PRI_RD_DIFF_RATE_SHFT                                28
#define HWIO_LPASS_LPAIF_IRQ_RAW_STATa_FRM_REF_BMSK                                  0x8000000
#define HWIO_LPASS_LPAIF_IRQ_RAW_STATa_FRM_REF_SHFT                                         27
#define HWIO_LPASS_LPAIF_IRQ_RAW_STATa_ERR_WRDMA_CH3_BMSK                            0x4000000
#define HWIO_LPASS_LPAIF_IRQ_RAW_STATa_ERR_WRDMA_CH3_SHFT                                   26
#define HWIO_LPASS_LPAIF_IRQ_RAW_STATa_OVR_WRDMA_CH3_BMSK                            0x2000000
#define HWIO_LPASS_LPAIF_IRQ_RAW_STATa_OVR_WRDMA_CH3_SHFT                                   25
#define HWIO_LPASS_LPAIF_IRQ_RAW_STATa_PER_WRDMA_CH3_BMSK                            0x1000000
#define HWIO_LPASS_LPAIF_IRQ_RAW_STATa_PER_WRDMA_CH3_SHFT                                   24
#define HWIO_LPASS_LPAIF_IRQ_RAW_STATa_ERR_WRDMA_CH2_BMSK                             0x800000
#define HWIO_LPASS_LPAIF_IRQ_RAW_STATa_ERR_WRDMA_CH2_SHFT                                   23
#define HWIO_LPASS_LPAIF_IRQ_RAW_STATa_OVR_WRDMA_CH2_BMSK                             0x400000
#define HWIO_LPASS_LPAIF_IRQ_RAW_STATa_OVR_WRDMA_CH2_SHFT                                   22
#define HWIO_LPASS_LPAIF_IRQ_RAW_STATa_PER_WRDMA_CH2_BMSK                             0x200000
#define HWIO_LPASS_LPAIF_IRQ_RAW_STATa_PER_WRDMA_CH2_SHFT                                   21
#define HWIO_LPASS_LPAIF_IRQ_RAW_STATa_ERR_WRDMA_CH1_BMSK                             0x100000
#define HWIO_LPASS_LPAIF_IRQ_RAW_STATa_ERR_WRDMA_CH1_SHFT                                   20
#define HWIO_LPASS_LPAIF_IRQ_RAW_STATa_OVR_WRDMA_CH1_BMSK                              0x80000
#define HWIO_LPASS_LPAIF_IRQ_RAW_STATa_OVR_WRDMA_CH1_SHFT                                   19
#define HWIO_LPASS_LPAIF_IRQ_RAW_STATa_PER_WRDMA_CH1_BMSK                              0x40000
#define HWIO_LPASS_LPAIF_IRQ_RAW_STATa_PER_WRDMA_CH1_SHFT                                   18
#define HWIO_LPASS_LPAIF_IRQ_RAW_STATa_ERR_WRDMA_CH0_BMSK                              0x20000
#define HWIO_LPASS_LPAIF_IRQ_RAW_STATa_ERR_WRDMA_CH0_SHFT                                   17
#define HWIO_LPASS_LPAIF_IRQ_RAW_STATa_OVR_WRDMA_CH0_BMSK                              0x10000
#define HWIO_LPASS_LPAIF_IRQ_RAW_STATa_OVR_WRDMA_CH0_SHFT                                   16
#define HWIO_LPASS_LPAIF_IRQ_RAW_STATa_PER_WRDMA_CH0_BMSK                               0x8000
#define HWIO_LPASS_LPAIF_IRQ_RAW_STATa_PER_WRDMA_CH0_SHFT                                   15
#define HWIO_LPASS_LPAIF_IRQ_RAW_STATa_ERR_RDDMA_CH4_BMSK                               0x4000
#define HWIO_LPASS_LPAIF_IRQ_RAW_STATa_ERR_RDDMA_CH4_SHFT                                   14
#define HWIO_LPASS_LPAIF_IRQ_RAW_STATa_UNDR_RDDMA_CH4_BMSK                              0x2000
#define HWIO_LPASS_LPAIF_IRQ_RAW_STATa_UNDR_RDDMA_CH4_SHFT                                  13
#define HWIO_LPASS_LPAIF_IRQ_RAW_STATa_PER_RDDMA_CH4_BMSK                               0x1000
#define HWIO_LPASS_LPAIF_IRQ_RAW_STATa_PER_RDDMA_CH4_SHFT                                   12
#define HWIO_LPASS_LPAIF_IRQ_RAW_STATa_ERR_RDDMA_CH3_BMSK                                0x800
#define HWIO_LPASS_LPAIF_IRQ_RAW_STATa_ERR_RDDMA_CH3_SHFT                                   11
#define HWIO_LPASS_LPAIF_IRQ_RAW_STATa_UNDR_RDDMA_CH3_BMSK                               0x400
#define HWIO_LPASS_LPAIF_IRQ_RAW_STATa_UNDR_RDDMA_CH3_SHFT                                  10
#define HWIO_LPASS_LPAIF_IRQ_RAW_STATa_PER_RDDMA_CH3_BMSK                                0x200
#define HWIO_LPASS_LPAIF_IRQ_RAW_STATa_PER_RDDMA_CH3_SHFT                                    9
#define HWIO_LPASS_LPAIF_IRQ_RAW_STATa_ERR_RDDMA_CH2_BMSK                                0x100
#define HWIO_LPASS_LPAIF_IRQ_RAW_STATa_ERR_RDDMA_CH2_SHFT                                    8
#define HWIO_LPASS_LPAIF_IRQ_RAW_STATa_UNDR_RDDMA_CH2_BMSK                                0x80
#define HWIO_LPASS_LPAIF_IRQ_RAW_STATa_UNDR_RDDMA_CH2_SHFT                                   7
#define HWIO_LPASS_LPAIF_IRQ_RAW_STATa_PER_RDDMA_CH2_BMSK                                 0x40
#define HWIO_LPASS_LPAIF_IRQ_RAW_STATa_PER_RDDMA_CH2_SHFT                                    6
#define HWIO_LPASS_LPAIF_IRQ_RAW_STATa_ERR_RDDMA_CH1_BMSK                                 0x20
#define HWIO_LPASS_LPAIF_IRQ_RAW_STATa_ERR_RDDMA_CH1_SHFT                                    5
#define HWIO_LPASS_LPAIF_IRQ_RAW_STATa_UNDR_RDDMA_CH1_BMSK                                0x10
#define HWIO_LPASS_LPAIF_IRQ_RAW_STATa_UNDR_RDDMA_CH1_SHFT                                   4
#define HWIO_LPASS_LPAIF_IRQ_RAW_STATa_PER_RDDMA_CH1_BMSK                                  0x8
#define HWIO_LPASS_LPAIF_IRQ_RAW_STATa_PER_RDDMA_CH1_SHFT                                    3
#define HWIO_LPASS_LPAIF_IRQ_RAW_STATa_ERR_RDDMA_CH0_BMSK                                  0x4
#define HWIO_LPASS_LPAIF_IRQ_RAW_STATa_ERR_RDDMA_CH0_SHFT                                    2
#define HWIO_LPASS_LPAIF_IRQ_RAW_STATa_UNDR_RDDMA_CH0_BMSK                                 0x2
#define HWIO_LPASS_LPAIF_IRQ_RAW_STATa_UNDR_RDDMA_CH0_SHFT                                   1
#define HWIO_LPASS_LPAIF_IRQ_RAW_STATa_PER_RDDMA_CH0_BMSK                                  0x1
#define HWIO_LPASS_LPAIF_IRQ_RAW_STATa_PER_RDDMA_CH0_SHFT                                    0

#define HWIO_LPASS_LPAIF_IRQ_CLEARa_ADDR(base,a)                                    ((base) + 0X900C + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_IRQ_CLEARa_OFFS(a)                                         (0X900C + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_IRQ_CLEARa_RMSK                                            0xffffffff
#define HWIO_LPASS_LPAIF_IRQ_CLEARa_MAXa                                                     2
#define HWIO_LPASS_LPAIF_IRQ_CLEARa_OUTI(base,a,val)        \
                out_dword(HWIO_LPASS_LPAIF_IRQ_CLEARa_ADDR(base,a),val)
#define HWIO_LPASS_LPAIF_IRQ_CLEARa_SEC_RD_NO_RATE_BMSK                             0x80000000
#define HWIO_LPASS_LPAIF_IRQ_CLEARa_SEC_RD_NO_RATE_SHFT                                     31
#define HWIO_LPASS_LPAIF_IRQ_CLEARa_SEC_RD_DIFF_RATE_BMSK                           0x40000000
#define HWIO_LPASS_LPAIF_IRQ_CLEARa_SEC_RD_DIFF_RATE_SHFT                                   30
#define HWIO_LPASS_LPAIF_IRQ_CLEARa_PRI_RD_NO_RATE_BMSK                             0x20000000
#define HWIO_LPASS_LPAIF_IRQ_CLEARa_PRI_RD_NO_RATE_SHFT                                     29
#define HWIO_LPASS_LPAIF_IRQ_CLEARa_PRI_RD_DIFF_RATE_BMSK                           0x10000000
#define HWIO_LPASS_LPAIF_IRQ_CLEARa_PRI_RD_DIFF_RATE_SHFT                                   28
#define HWIO_LPASS_LPAIF_IRQ_CLEARa_FRM_REF_BMSK                                     0x8000000
#define HWIO_LPASS_LPAIF_IRQ_CLEARa_FRM_REF_SHFT                                            27
#define HWIO_LPASS_LPAIF_IRQ_CLEARa_ERR_WRDMA_CH3_BMSK                               0x4000000
#define HWIO_LPASS_LPAIF_IRQ_CLEARa_ERR_WRDMA_CH3_SHFT                                      26
#define HWIO_LPASS_LPAIF_IRQ_CLEARa_OVR_WRDMA_CH3_BMSK                               0x2000000
#define HWIO_LPASS_LPAIF_IRQ_CLEARa_OVR_WRDMA_CH3_SHFT                                      25
#define HWIO_LPASS_LPAIF_IRQ_CLEARa_PER_WRDMA_CH3_BMSK                               0x1000000
#define HWIO_LPASS_LPAIF_IRQ_CLEARa_PER_WRDMA_CH3_SHFT                                      24
#define HWIO_LPASS_LPAIF_IRQ_CLEARa_ERR_WRDMA_CH2_BMSK                                0x800000
#define HWIO_LPASS_LPAIF_IRQ_CLEARa_ERR_WRDMA_CH2_SHFT                                      23
#define HWIO_LPASS_LPAIF_IRQ_CLEARa_OVR_WRDMA_CH2_BMSK                                0x400000
#define HWIO_LPASS_LPAIF_IRQ_CLEARa_OVR_WRDMA_CH2_SHFT                                      22
#define HWIO_LPASS_LPAIF_IRQ_CLEARa_PER_WRDMA_CH2_BMSK                                0x200000
#define HWIO_LPASS_LPAIF_IRQ_CLEARa_PER_WRDMA_CH2_SHFT                                      21
#define HWIO_LPASS_LPAIF_IRQ_CLEARa_ERR_WRDMA_CH1_BMSK                                0x100000
#define HWIO_LPASS_LPAIF_IRQ_CLEARa_ERR_WRDMA_CH1_SHFT                                      20
#define HWIO_LPASS_LPAIF_IRQ_CLEARa_OVR_WRDMA_CH1_BMSK                                 0x80000
#define HWIO_LPASS_LPAIF_IRQ_CLEARa_OVR_WRDMA_CH1_SHFT                                      19
#define HWIO_LPASS_LPAIF_IRQ_CLEARa_PER_WRDMA_CH1_BMSK                                 0x40000
#define HWIO_LPASS_LPAIF_IRQ_CLEARa_PER_WRDMA_CH1_SHFT                                      18
#define HWIO_LPASS_LPAIF_IRQ_CLEARa_ERR_WRDMA_CH0_BMSK                                 0x20000
#define HWIO_LPASS_LPAIF_IRQ_CLEARa_ERR_WRDMA_CH0_SHFT                                      17
#define HWIO_LPASS_LPAIF_IRQ_CLEARa_OVR_WRDMA_CH0_BMSK                                 0x10000
#define HWIO_LPASS_LPAIF_IRQ_CLEARa_OVR_WRDMA_CH0_SHFT                                      16
#define HWIO_LPASS_LPAIF_IRQ_CLEARa_PER_WRDMA_CH0_BMSK                                  0x8000
#define HWIO_LPASS_LPAIF_IRQ_CLEARa_PER_WRDMA_CH0_SHFT                                      15
#define HWIO_LPASS_LPAIF_IRQ_CLEARa_ERR_RDDMA_CH4_BMSK                                  0x4000
#define HWIO_LPASS_LPAIF_IRQ_CLEARa_ERR_RDDMA_CH4_SHFT                                      14
#define HWIO_LPASS_LPAIF_IRQ_CLEARa_UNDR_RDDMA_CH4_BMSK                                 0x2000
#define HWIO_LPASS_LPAIF_IRQ_CLEARa_UNDR_RDDMA_CH4_SHFT                                     13
#define HWIO_LPASS_LPAIF_IRQ_CLEARa_PER_RDDMA_CH4_BMSK                                  0x1000
#define HWIO_LPASS_LPAIF_IRQ_CLEARa_PER_RDDMA_CH4_SHFT                                      12
#define HWIO_LPASS_LPAIF_IRQ_CLEARa_ERR_RDDMA_CH3_BMSK                                   0x800
#define HWIO_LPASS_LPAIF_IRQ_CLEARa_ERR_RDDMA_CH3_SHFT                                      11
#define HWIO_LPASS_LPAIF_IRQ_CLEARa_UNDR_RDDMA_CH3_BMSK                                  0x400
#define HWIO_LPASS_LPAIF_IRQ_CLEARa_UNDR_RDDMA_CH3_SHFT                                     10
#define HWIO_LPASS_LPAIF_IRQ_CLEARa_PER_RDDMA_CH3_BMSK                                   0x200
#define HWIO_LPASS_LPAIF_IRQ_CLEARa_PER_RDDMA_CH3_SHFT                                       9
#define HWIO_LPASS_LPAIF_IRQ_CLEARa_ERR_RDDMA_CH2_BMSK                                   0x100
#define HWIO_LPASS_LPAIF_IRQ_CLEARa_ERR_RDDMA_CH2_SHFT                                       8
#define HWIO_LPASS_LPAIF_IRQ_CLEARa_UNDR_RDDMA_CH2_BMSK                                   0x80
#define HWIO_LPASS_LPAIF_IRQ_CLEARa_UNDR_RDDMA_CH2_SHFT                                      7
#define HWIO_LPASS_LPAIF_IRQ_CLEARa_PER_RDDMA_CH2_BMSK                                    0x40
#define HWIO_LPASS_LPAIF_IRQ_CLEARa_PER_RDDMA_CH2_SHFT                                       6
#define HWIO_LPASS_LPAIF_IRQ_CLEARa_ERR_RDDMA_CH1_BMSK                                    0x20
#define HWIO_LPASS_LPAIF_IRQ_CLEARa_ERR_RDDMA_CH1_SHFT                                       5
#define HWIO_LPASS_LPAIF_IRQ_CLEARa_UNDR_RDDMA_CH1_BMSK                                   0x10
#define HWIO_LPASS_LPAIF_IRQ_CLEARa_UNDR_RDDMA_CH1_SHFT                                      4
#define HWIO_LPASS_LPAIF_IRQ_CLEARa_PER_RDDMA_CH1_BMSK                                     0x8
#define HWIO_LPASS_LPAIF_IRQ_CLEARa_PER_RDDMA_CH1_SHFT                                       3
#define HWIO_LPASS_LPAIF_IRQ_CLEARa_ERR_RDDMA_CH0_BMSK                                     0x4
#define HWIO_LPASS_LPAIF_IRQ_CLEARa_ERR_RDDMA_CH0_SHFT                                       2
#define HWIO_LPASS_LPAIF_IRQ_CLEARa_UNDR_RDDMA_CH0_BMSK                                    0x2
#define HWIO_LPASS_LPAIF_IRQ_CLEARa_UNDR_RDDMA_CH0_SHFT                                      1
#define HWIO_LPASS_LPAIF_IRQ_CLEARa_PER_RDDMA_CH0_BMSK                                     0x1
#define HWIO_LPASS_LPAIF_IRQ_CLEARa_PER_RDDMA_CH0_SHFT                                       0

#define HWIO_LPASS_LPAIF_IRQ_FORCEa_ADDR(base,a)                                    ((base) + 0X9010 + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_IRQ_FORCEa_OFFS(a)                                         (0X9010 + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_IRQ_FORCEa_RMSK                                            0xffffffff
#define HWIO_LPASS_LPAIF_IRQ_FORCEa_MAXa                                                     2
#define HWIO_LPASS_LPAIF_IRQ_FORCEa_OUTI(base,a,val)        \
                out_dword(HWIO_LPASS_LPAIF_IRQ_FORCEa_ADDR(base,a),val)
#define HWIO_LPASS_LPAIF_IRQ_FORCEa_SEC_RD_NO_RATE_BMSK                             0x80000000
#define HWIO_LPASS_LPAIF_IRQ_FORCEa_SEC_RD_NO_RATE_SHFT                                     31
#define HWIO_LPASS_LPAIF_IRQ_FORCEa_SEC_RD_DIFF_RATE_BMSK                           0x40000000
#define HWIO_LPASS_LPAIF_IRQ_FORCEa_SEC_RD_DIFF_RATE_SHFT                                   30
#define HWIO_LPASS_LPAIF_IRQ_FORCEa_PRI_RD_NO_RATE_BMSK                             0x20000000
#define HWIO_LPASS_LPAIF_IRQ_FORCEa_PRI_RD_NO_RATE_SHFT                                     29
#define HWIO_LPASS_LPAIF_IRQ_FORCEa_PRI_RD_DIFF_RATE_BMSK                           0x10000000
#define HWIO_LPASS_LPAIF_IRQ_FORCEa_PRI_RD_DIFF_RATE_SHFT                                   28
#define HWIO_LPASS_LPAIF_IRQ_FORCEa_FRM_REF_BMSK                                     0x8000000
#define HWIO_LPASS_LPAIF_IRQ_FORCEa_FRM_REF_SHFT                                            27
#define HWIO_LPASS_LPAIF_IRQ_FORCEa_ERR_WRDMA_CH3_BMSK                               0x4000000
#define HWIO_LPASS_LPAIF_IRQ_FORCEa_ERR_WRDMA_CH3_SHFT                                      26
#define HWIO_LPASS_LPAIF_IRQ_FORCEa_OVR_WRDMA_CH3_BMSK                               0x2000000
#define HWIO_LPASS_LPAIF_IRQ_FORCEa_OVR_WRDMA_CH3_SHFT                                      25
#define HWIO_LPASS_LPAIF_IRQ_FORCEa_PER_WRDMA_CH3_BMSK                               0x1000000
#define HWIO_LPASS_LPAIF_IRQ_FORCEa_PER_WRDMA_CH3_SHFT                                      24
#define HWIO_LPASS_LPAIF_IRQ_FORCEa_ERR_WRDMA_CH2_BMSK                                0x800000
#define HWIO_LPASS_LPAIF_IRQ_FORCEa_ERR_WRDMA_CH2_SHFT                                      23
#define HWIO_LPASS_LPAIF_IRQ_FORCEa_OVR_WRDMA_CH2_BMSK                                0x400000
#define HWIO_LPASS_LPAIF_IRQ_FORCEa_OVR_WRDMA_CH2_SHFT                                      22
#define HWIO_LPASS_LPAIF_IRQ_FORCEa_PER_WRDMA_CH2_BMSK                                0x200000
#define HWIO_LPASS_LPAIF_IRQ_FORCEa_PER_WRDMA_CH2_SHFT                                      21
#define HWIO_LPASS_LPAIF_IRQ_FORCEa_ERR_WRDMA_CH1_BMSK                                0x100000
#define HWIO_LPASS_LPAIF_IRQ_FORCEa_ERR_WRDMA_CH1_SHFT                                      20
#define HWIO_LPASS_LPAIF_IRQ_FORCEa_OVR_WRDMA_CH1_BMSK                                 0x80000
#define HWIO_LPASS_LPAIF_IRQ_FORCEa_OVR_WRDMA_CH1_SHFT                                      19
#define HWIO_LPASS_LPAIF_IRQ_FORCEa_PER_WRDMA_CH1_BMSK                                 0x40000
#define HWIO_LPASS_LPAIF_IRQ_FORCEa_PER_WRDMA_CH1_SHFT                                      18
#define HWIO_LPASS_LPAIF_IRQ_FORCEa_ERR_WRDMA_CH0_BMSK                                 0x20000
#define HWIO_LPASS_LPAIF_IRQ_FORCEa_ERR_WRDMA_CH0_SHFT                                      17
#define HWIO_LPASS_LPAIF_IRQ_FORCEa_OVR_WRDMA_CH0_BMSK                                 0x10000
#define HWIO_LPASS_LPAIF_IRQ_FORCEa_OVR_WRDMA_CH0_SHFT                                      16
#define HWIO_LPASS_LPAIF_IRQ_FORCEa_PER_WRDMA_CH0_BMSK                                  0x8000
#define HWIO_LPASS_LPAIF_IRQ_FORCEa_PER_WRDMA_CH0_SHFT                                      15
#define HWIO_LPASS_LPAIF_IRQ_FORCEa_ERR_RDDMA_CH4_BMSK                                  0x4000
#define HWIO_LPASS_LPAIF_IRQ_FORCEa_ERR_RDDMA_CH4_SHFT                                      14
#define HWIO_LPASS_LPAIF_IRQ_FORCEa_UNDR_RDDMA_CH4_BMSK                                 0x2000
#define HWIO_LPASS_LPAIF_IRQ_FORCEa_UNDR_RDDMA_CH4_SHFT                                     13
#define HWIO_LPASS_LPAIF_IRQ_FORCEa_PER_RDDMA_CH4_BMSK                                  0x1000
#define HWIO_LPASS_LPAIF_IRQ_FORCEa_PER_RDDMA_CH4_SHFT                                      12
#define HWIO_LPASS_LPAIF_IRQ_FORCEa_ERR_RDDMA_CH3_BMSK                                   0x800
#define HWIO_LPASS_LPAIF_IRQ_FORCEa_ERR_RDDMA_CH3_SHFT                                      11
#define HWIO_LPASS_LPAIF_IRQ_FORCEa_UNDR_RDDMA_CH3_BMSK                                  0x400
#define HWIO_LPASS_LPAIF_IRQ_FORCEa_UNDR_RDDMA_CH3_SHFT                                     10
#define HWIO_LPASS_LPAIF_IRQ_FORCEa_PER_RDDMA_CH3_BMSK                                   0x200
#define HWIO_LPASS_LPAIF_IRQ_FORCEa_PER_RDDMA_CH3_SHFT                                       9
#define HWIO_LPASS_LPAIF_IRQ_FORCEa_ERR_RDDMA_CH2_BMSK                                   0x100
#define HWIO_LPASS_LPAIF_IRQ_FORCEa_ERR_RDDMA_CH2_SHFT                                       8
#define HWIO_LPASS_LPAIF_IRQ_FORCEa_UNDR_RDDMA_CH2_BMSK                                   0x80
#define HWIO_LPASS_LPAIF_IRQ_FORCEa_UNDR_RDDMA_CH2_SHFT                                      7
#define HWIO_LPASS_LPAIF_IRQ_FORCEa_PER_RDDMA_CH2_BMSK                                    0x40
#define HWIO_LPASS_LPAIF_IRQ_FORCEa_PER_RDDMA_CH2_SHFT                                       6
#define HWIO_LPASS_LPAIF_IRQ_FORCEa_ERR_RDDMA_CH1_BMSK                                    0x20
#define HWIO_LPASS_LPAIF_IRQ_FORCEa_ERR_RDDMA_CH1_SHFT                                       5
#define HWIO_LPASS_LPAIF_IRQ_FORCEa_UNDR_RDDMA_CH1_BMSK                                   0x10
#define HWIO_LPASS_LPAIF_IRQ_FORCEa_UNDR_RDDMA_CH1_SHFT                                      4
#define HWIO_LPASS_LPAIF_IRQ_FORCEa_PER_RDDMA_CH1_BMSK                                     0x8
#define HWIO_LPASS_LPAIF_IRQ_FORCEa_PER_RDDMA_CH1_SHFT                                       3
#define HWIO_LPASS_LPAIF_IRQ_FORCEa_ERR_RDDMA_CH0_BMSK                                     0x4
#define HWIO_LPASS_LPAIF_IRQ_FORCEa_ERR_RDDMA_CH0_SHFT                                       2
#define HWIO_LPASS_LPAIF_IRQ_FORCEa_UNDR_RDDMA_CH0_BMSK                                    0x2
#define HWIO_LPASS_LPAIF_IRQ_FORCEa_UNDR_RDDMA_CH0_SHFT                                      1
#define HWIO_LPASS_LPAIF_IRQ_FORCEa_PER_RDDMA_CH0_BMSK                                     0x1
#define HWIO_LPASS_LPAIF_IRQ_FORCEa_PER_RDDMA_CH0_SHFT                                       0

#define HWIO_LPASS_LPAIF_IRQ2_ENa_ADDR(base,a)                                      ((base) + 0X9014 + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_IRQ2_ENa_OFFS(a)                                           (0X9014 + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_IRQ2_ENa_RMSK                                              0x7fffffff
#define HWIO_LPASS_LPAIF_IRQ2_ENa_MAXa                                                       2
#define HWIO_LPASS_LPAIF_IRQ2_ENa_INI(base,a)                \
                in_dword_masked(HWIO_LPASS_LPAIF_IRQ2_ENa_ADDR(base,a), HWIO_LPASS_LPAIF_IRQ2_ENa_RMSK)
#define HWIO_LPASS_LPAIF_IRQ2_ENa_INMI(base,a,mask)        \
                in_dword_masked(HWIO_LPASS_LPAIF_IRQ2_ENa_ADDR(base,a), mask)
#define HWIO_LPASS_LPAIF_IRQ2_ENa_OUTI(base,a,val)        \
                out_dword(HWIO_LPASS_LPAIF_IRQ2_ENa_ADDR(base,a),val)
#define HWIO_LPASS_LPAIF_IRQ2_ENa_OUTMI(base,a,mask,val) \
                out_dword_masked_ns(HWIO_LPASS_LPAIF_IRQ2_ENa_ADDR(base,a),mask,val,HWIO_LPASS_LPAIF_IRQ2_ENa_INI(base,a))
#define HWIO_LPASS_LPAIF_IRQ2_ENa_ERR_WRDMA_CH8_BMSK                                0x40000000
#define HWIO_LPASS_LPAIF_IRQ2_ENa_ERR_WRDMA_CH8_SHFT                                        30
#define HWIO_LPASS_LPAIF_IRQ2_ENa_OVR_WRDMA_CH8_BMSK                                0x20000000
#define HWIO_LPASS_LPAIF_IRQ2_ENa_OVR_WRDMA_CH8_SHFT                                        29
#define HWIO_LPASS_LPAIF_IRQ2_ENa_PER_WRDMA_CH8_BMSK                                0x10000000
#define HWIO_LPASS_LPAIF_IRQ2_ENa_PER_WRDMA_CH8_SHFT                                        28
#define HWIO_LPASS_LPAIF_IRQ2_ENa_ERR_WRDMA_CH7_BMSK                                 0x8000000
#define HWIO_LPASS_LPAIF_IRQ2_ENa_ERR_WRDMA_CH7_SHFT                                        27
#define HWIO_LPASS_LPAIF_IRQ2_ENa_OVR_WRDMA_CH7_BMSK                                 0x4000000
#define HWIO_LPASS_LPAIF_IRQ2_ENa_OVR_WRDMA_CH7_SHFT                                        26
#define HWIO_LPASS_LPAIF_IRQ2_ENa_PER_WRDMA_CH7_BMSK                                 0x2000000
#define HWIO_LPASS_LPAIF_IRQ2_ENa_PER_WRDMA_CH7_SHFT                                        25
#define HWIO_LPASS_LPAIF_IRQ2_ENa_ERR_WRDMA_CH6_BMSK                                 0x1000000
#define HWIO_LPASS_LPAIF_IRQ2_ENa_ERR_WRDMA_CH6_SHFT                                        24
#define HWIO_LPASS_LPAIF_IRQ2_ENa_OVR_WRDMA_CH6_BMSK                                  0x800000
#define HWIO_LPASS_LPAIF_IRQ2_ENa_OVR_WRDMA_CH6_SHFT                                        23
#define HWIO_LPASS_LPAIF_IRQ2_ENa_PER_WRDMA_CH6_BMSK                                  0x400000
#define HWIO_LPASS_LPAIF_IRQ2_ENa_PER_WRDMA_CH6_SHFT                                        22
#define HWIO_LPASS_LPAIF_IRQ2_ENa_ERR_RDDMA_CH9_BMSK                                  0x200000
#define HWIO_LPASS_LPAIF_IRQ2_ENa_ERR_RDDMA_CH9_SHFT                                        21
#define HWIO_LPASS_LPAIF_IRQ2_ENa_UNDR_RDDMA_CH9_BMSK                                 0x100000
#define HWIO_LPASS_LPAIF_IRQ2_ENa_UNDR_RDDMA_CH9_SHFT                                       20
#define HWIO_LPASS_LPAIF_IRQ2_ENa_PER_RDDMA_CH9_BMSK                                   0x80000
#define HWIO_LPASS_LPAIF_IRQ2_ENa_PER_RDDMA_CH9_SHFT                                        19
#define HWIO_LPASS_LPAIF_IRQ2_ENa_ERR_RDDMA_CH8_BMSK                                   0x40000
#define HWIO_LPASS_LPAIF_IRQ2_ENa_ERR_RDDMA_CH8_SHFT                                        18
#define HWIO_LPASS_LPAIF_IRQ2_ENa_UNDR_RDDMA_CH8_BMSK                                  0x20000
#define HWIO_LPASS_LPAIF_IRQ2_ENa_UNDR_RDDMA_CH8_SHFT                                       17
#define HWIO_LPASS_LPAIF_IRQ2_ENa_PER_RDDMA_CH8_BMSK                                   0x10000
#define HWIO_LPASS_LPAIF_IRQ2_ENa_PER_RDDMA_CH8_SHFT                                        16
#define HWIO_LPASS_LPAIF_IRQ2_ENa_ERR_RDDMA_CH7_BMSK                                    0x8000
#define HWIO_LPASS_LPAIF_IRQ2_ENa_ERR_RDDMA_CH7_SHFT                                        15
#define HWIO_LPASS_LPAIF_IRQ2_ENa_UNDR_RDDMA_CH7_BMSK                                   0x4000
#define HWIO_LPASS_LPAIF_IRQ2_ENa_UNDR_RDDMA_CH7_SHFT                                       14
#define HWIO_LPASS_LPAIF_IRQ2_ENa_PER_RDDMA_CH7_BMSK                                    0x2000
#define HWIO_LPASS_LPAIF_IRQ2_ENa_PER_RDDMA_CH7_SHFT                                        13
#define HWIO_LPASS_LPAIF_IRQ2_ENa_ERR_RDDMA_CH6_BMSK                                    0x1000
#define HWIO_LPASS_LPAIF_IRQ2_ENa_ERR_RDDMA_CH6_SHFT                                        12
#define HWIO_LPASS_LPAIF_IRQ2_ENa_UNDR_RDDMA_CH6_BMSK                                    0x800
#define HWIO_LPASS_LPAIF_IRQ2_ENa_UNDR_RDDMA_CH6_SHFT                                       11
#define HWIO_LPASS_LPAIF_IRQ2_ENa_PER_RDDMA_CH6_BMSK                                     0x400
#define HWIO_LPASS_LPAIF_IRQ2_ENa_PER_RDDMA_CH6_SHFT                                        10
#define HWIO_LPASS_LPAIF_IRQ2_ENa_ERR_WRDMA_CH5_BMSK                                     0x200
#define HWIO_LPASS_LPAIF_IRQ2_ENa_ERR_WRDMA_CH5_SHFT                                         9
#define HWIO_LPASS_LPAIF_IRQ2_ENa_OVR_WRDMA_CH5_BMSK                                     0x100
#define HWIO_LPASS_LPAIF_IRQ2_ENa_OVR_WRDMA_CH5_SHFT                                         8
#define HWIO_LPASS_LPAIF_IRQ2_ENa_PER_WRDMA_CH5_BMSK                                      0x80
#define HWIO_LPASS_LPAIF_IRQ2_ENa_PER_WRDMA_CH5_SHFT                                         7
#define HWIO_LPASS_LPAIF_IRQ2_ENa_ERR_WRDMA_CH4_BMSK                                      0x40
#define HWIO_LPASS_LPAIF_IRQ2_ENa_ERR_WRDMA_CH4_SHFT                                         6
#define HWIO_LPASS_LPAIF_IRQ2_ENa_OVR_WRDMA_CH4_BMSK                                      0x20
#define HWIO_LPASS_LPAIF_IRQ2_ENa_OVR_WRDMA_CH4_SHFT                                         5
#define HWIO_LPASS_LPAIF_IRQ2_ENa_PER_WRDMA_CH4_BMSK                                      0x10
#define HWIO_LPASS_LPAIF_IRQ2_ENa_PER_WRDMA_CH4_SHFT                                         4
#define HWIO_LPASS_LPAIF_IRQ2_ENa_ERR_RDDMA_CH5_BMSK                                       0x8
#define HWIO_LPASS_LPAIF_IRQ2_ENa_ERR_RDDMA_CH5_SHFT                                         3
#define HWIO_LPASS_LPAIF_IRQ2_ENa_UNDR_RDDMA_CH5_BMSK                                      0x4
#define HWIO_LPASS_LPAIF_IRQ2_ENa_UNDR_RDDMA_CH5_SHFT                                        2
#define HWIO_LPASS_LPAIF_IRQ2_ENa_PER_RDDMA_CH5_BMSK                                       0x2
#define HWIO_LPASS_LPAIF_IRQ2_ENa_PER_RDDMA_CH5_SHFT                                         1
#define HWIO_LPASS_LPAIF_IRQ2_ENa_UNDR_EXTERNAL_BMSK                                       0x1
#define HWIO_LPASS_LPAIF_IRQ2_ENa_UNDR_EXTERNAL_SHFT                                         0

#define HWIO_LPASS_LPAIF_IRQ2_STATa_ADDR(base,a)                                    ((base) + 0X9018 + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_IRQ2_STATa_OFFS(a)                                         (0X9018 + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_IRQ2_STATa_RMSK                                            0x7fffffff
#define HWIO_LPASS_LPAIF_IRQ2_STATa_MAXa                                                     2
#define HWIO_LPASS_LPAIF_IRQ2_STATa_INI(base,a)                \
                in_dword_masked(HWIO_LPASS_LPAIF_IRQ2_STATa_ADDR(base,a), HWIO_LPASS_LPAIF_IRQ2_STATa_RMSK)
#define HWIO_LPASS_LPAIF_IRQ2_STATa_INMI(base,a,mask)        \
                in_dword_masked(HWIO_LPASS_LPAIF_IRQ2_STATa_ADDR(base,a), mask)
#define HWIO_LPASS_LPAIF_IRQ2_STATa_ERR_WRDMA_CH8_BMSK                              0x40000000
#define HWIO_LPASS_LPAIF_IRQ2_STATa_ERR_WRDMA_CH8_SHFT                                      30
#define HWIO_LPASS_LPAIF_IRQ2_STATa_OVR_WRDMA_CH8_BMSK                              0x20000000
#define HWIO_LPASS_LPAIF_IRQ2_STATa_OVR_WRDMA_CH8_SHFT                                      29
#define HWIO_LPASS_LPAIF_IRQ2_STATa_PER_WRDMA_CH8_BMSK                              0x10000000
#define HWIO_LPASS_LPAIF_IRQ2_STATa_PER_WRDMA_CH8_SHFT                                      28
#define HWIO_LPASS_LPAIF_IRQ2_STATa_ERR_WRDMA_CH7_BMSK                               0x8000000
#define HWIO_LPASS_LPAIF_IRQ2_STATa_ERR_WRDMA_CH7_SHFT                                      27
#define HWIO_LPASS_LPAIF_IRQ2_STATa_OVR_WRDMA_CH7_BMSK                               0x4000000
#define HWIO_LPASS_LPAIF_IRQ2_STATa_OVR_WRDMA_CH7_SHFT                                      26
#define HWIO_LPASS_LPAIF_IRQ2_STATa_PER_WRDMA_CH7_BMSK                               0x2000000
#define HWIO_LPASS_LPAIF_IRQ2_STATa_PER_WRDMA_CH7_SHFT                                      25
#define HWIO_LPASS_LPAIF_IRQ2_STATa_ERR_WRDMA_CH6_BMSK                               0x1000000
#define HWIO_LPASS_LPAIF_IRQ2_STATa_ERR_WRDMA_CH6_SHFT                                      24
#define HWIO_LPASS_LPAIF_IRQ2_STATa_OVR_WRDMA_CH6_BMSK                                0x800000
#define HWIO_LPASS_LPAIF_IRQ2_STATa_OVR_WRDMA_CH6_SHFT                                      23
#define HWIO_LPASS_LPAIF_IRQ2_STATa_PER_WRDMA_CH6_BMSK                                0x400000
#define HWIO_LPASS_LPAIF_IRQ2_STATa_PER_WRDMA_CH6_SHFT                                      22
#define HWIO_LPASS_LPAIF_IRQ2_STATa_ERR_RDDMA_CH9_BMSK                                0x200000
#define HWIO_LPASS_LPAIF_IRQ2_STATa_ERR_RDDMA_CH9_SHFT                                      21
#define HWIO_LPASS_LPAIF_IRQ2_STATa_UNDR_RDDMA_CH9_BMSK                               0x100000
#define HWIO_LPASS_LPAIF_IRQ2_STATa_UNDR_RDDMA_CH9_SHFT                                     20
#define HWIO_LPASS_LPAIF_IRQ2_STATa_PER_RDDMA_CH9_BMSK                                 0x80000
#define HWIO_LPASS_LPAIF_IRQ2_STATa_PER_RDDMA_CH9_SHFT                                      19
#define HWIO_LPASS_LPAIF_IRQ2_STATa_ERR_RDDMA_CH8_BMSK                                 0x40000
#define HWIO_LPASS_LPAIF_IRQ2_STATa_ERR_RDDMA_CH8_SHFT                                      18
#define HWIO_LPASS_LPAIF_IRQ2_STATa_UNDR_RDDMA_CH8_BMSK                                0x20000
#define HWIO_LPASS_LPAIF_IRQ2_STATa_UNDR_RDDMA_CH8_SHFT                                     17
#define HWIO_LPASS_LPAIF_IRQ2_STATa_PER_RDDMA_CH8_BMSK                                 0x10000
#define HWIO_LPASS_LPAIF_IRQ2_STATa_PER_RDDMA_CH8_SHFT                                      16
#define HWIO_LPASS_LPAIF_IRQ2_STATa_ERR_RDDMA_CH7_BMSK                                  0x8000
#define HWIO_LPASS_LPAIF_IRQ2_STATa_ERR_RDDMA_CH7_SHFT                                      15
#define HWIO_LPASS_LPAIF_IRQ2_STATa_UNDR_RDDMA_CH7_BMSK                                 0x4000
#define HWIO_LPASS_LPAIF_IRQ2_STATa_UNDR_RDDMA_CH7_SHFT                                     14
#define HWIO_LPASS_LPAIF_IRQ2_STATa_PER_RDDMA_CH7_BMSK                                  0x2000
#define HWIO_LPASS_LPAIF_IRQ2_STATa_PER_RDDMA_CH7_SHFT                                      13
#define HWIO_LPASS_LPAIF_IRQ2_STATa_ERR_RDDMA_CH6_BMSK                                  0x1000
#define HWIO_LPASS_LPAIF_IRQ2_STATa_ERR_RDDMA_CH6_SHFT                                      12
#define HWIO_LPASS_LPAIF_IRQ2_STATa_UNDR_RDDMA_CH6_BMSK                                  0x800
#define HWIO_LPASS_LPAIF_IRQ2_STATa_UNDR_RDDMA_CH6_SHFT                                     11
#define HWIO_LPASS_LPAIF_IRQ2_STATa_PER_RDDMA_CH6_BMSK                                   0x400
#define HWIO_LPASS_LPAIF_IRQ2_STATa_PER_RDDMA_CH6_SHFT                                      10
#define HWIO_LPASS_LPAIF_IRQ2_STATa_ERR_WRDMA_CH5_BMSK                                   0x200
#define HWIO_LPASS_LPAIF_IRQ2_STATa_ERR_WRDMA_CH5_SHFT                                       9
#define HWIO_LPASS_LPAIF_IRQ2_STATa_OVR_WRDMA_CH5_BMSK                                   0x100
#define HWIO_LPASS_LPAIF_IRQ2_STATa_OVR_WRDMA_CH5_SHFT                                       8
#define HWIO_LPASS_LPAIF_IRQ2_STATa_PER_WRDMA_CH5_BMSK                                    0x80
#define HWIO_LPASS_LPAIF_IRQ2_STATa_PER_WRDMA_CH5_SHFT                                       7
#define HWIO_LPASS_LPAIF_IRQ2_STATa_ERR_WRDMA_CH4_BMSK                                    0x40
#define HWIO_LPASS_LPAIF_IRQ2_STATa_ERR_WRDMA_CH4_SHFT                                       6
#define HWIO_LPASS_LPAIF_IRQ2_STATa_OVR_WRDMA_CH4_BMSK                                    0x20
#define HWIO_LPASS_LPAIF_IRQ2_STATa_OVR_WRDMA_CH4_SHFT                                       5
#define HWIO_LPASS_LPAIF_IRQ2_STATa_PER_WRDMA_CH4_BMSK                                    0x10
#define HWIO_LPASS_LPAIF_IRQ2_STATa_PER_WRDMA_CH4_SHFT                                       4
#define HWIO_LPASS_LPAIF_IRQ2_STATa_ERR_RDDMA_CH5_BMSK                                     0x8
#define HWIO_LPASS_LPAIF_IRQ2_STATa_ERR_RDDMA_CH5_SHFT                                       3
#define HWIO_LPASS_LPAIF_IRQ2_STATa_UNDR_RDDMA_CH5_BMSK                                    0x4
#define HWIO_LPASS_LPAIF_IRQ2_STATa_UNDR_RDDMA_CH5_SHFT                                      2
#define HWIO_LPASS_LPAIF_IRQ2_STATa_PER_RDDMA_CH5_BMSK                                     0x2
#define HWIO_LPASS_LPAIF_IRQ2_STATa_PER_RDDMA_CH5_SHFT                                       1
#define HWIO_LPASS_LPAIF_IRQ2_STATa_UNDR_EXTERNAL_BMSK                                     0x1
#define HWIO_LPASS_LPAIF_IRQ2_STATa_UNDR_EXTERNAL_SHFT                                       0

#define HWIO_LPASS_LPAIF_IRQ2_RAW_STATa_ADDR(base,a)                                ((base) + 0X901C + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_IRQ2_RAW_STATa_OFFS(a)                                     (0X901C + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_IRQ2_RAW_STATa_RMSK                                        0x7fffffff
#define HWIO_LPASS_LPAIF_IRQ2_RAW_STATa_MAXa                                                 2
#define HWIO_LPASS_LPAIF_IRQ2_RAW_STATa_INI(base,a)                \
                in_dword_masked(HWIO_LPASS_LPAIF_IRQ2_RAW_STATa_ADDR(base,a), HWIO_LPASS_LPAIF_IRQ2_RAW_STATa_RMSK)
#define HWIO_LPASS_LPAIF_IRQ2_RAW_STATa_INMI(base,a,mask)        \
                in_dword_masked(HWIO_LPASS_LPAIF_IRQ2_RAW_STATa_ADDR(base,a), mask)
#define HWIO_LPASS_LPAIF_IRQ2_RAW_STATa_ERR_WRDMA_CH8_BMSK                          0x40000000
#define HWIO_LPASS_LPAIF_IRQ2_RAW_STATa_ERR_WRDMA_CH8_SHFT                                  30
#define HWIO_LPASS_LPAIF_IRQ2_RAW_STATa_OVR_WRDMA_CH8_BMSK                          0x20000000
#define HWIO_LPASS_LPAIF_IRQ2_RAW_STATa_OVR_WRDMA_CH8_SHFT                                  29
#define HWIO_LPASS_LPAIF_IRQ2_RAW_STATa_PER_WRDMA_CH8_BMSK                          0x10000000
#define HWIO_LPASS_LPAIF_IRQ2_RAW_STATa_PER_WRDMA_CH8_SHFT                                  28
#define HWIO_LPASS_LPAIF_IRQ2_RAW_STATa_ERR_WRDMA_CH7_BMSK                           0x8000000
#define HWIO_LPASS_LPAIF_IRQ2_RAW_STATa_ERR_WRDMA_CH7_SHFT                                  27
#define HWIO_LPASS_LPAIF_IRQ2_RAW_STATa_OVR_WRDMA_CH7_BMSK                           0x4000000
#define HWIO_LPASS_LPAIF_IRQ2_RAW_STATa_OVR_WRDMA_CH7_SHFT                                  26
#define HWIO_LPASS_LPAIF_IRQ2_RAW_STATa_PER_WRDMA_CH7_BMSK                           0x2000000
#define HWIO_LPASS_LPAIF_IRQ2_RAW_STATa_PER_WRDMA_CH7_SHFT                                  25
#define HWIO_LPASS_LPAIF_IRQ2_RAW_STATa_ERR_WRDMA_CH6_BMSK                           0x1000000
#define HWIO_LPASS_LPAIF_IRQ2_RAW_STATa_ERR_WRDMA_CH6_SHFT                                  24
#define HWIO_LPASS_LPAIF_IRQ2_RAW_STATa_OVR_WRDMA_CH6_BMSK                            0x800000
#define HWIO_LPASS_LPAIF_IRQ2_RAW_STATa_OVR_WRDMA_CH6_SHFT                                  23
#define HWIO_LPASS_LPAIF_IRQ2_RAW_STATa_PER_WRDMA_CH6_BMSK                            0x400000
#define HWIO_LPASS_LPAIF_IRQ2_RAW_STATa_PER_WRDMA_CH6_SHFT                                  22
#define HWIO_LPASS_LPAIF_IRQ2_RAW_STATa_ERR_RDDMA_CH9_BMSK                            0x200000
#define HWIO_LPASS_LPAIF_IRQ2_RAW_STATa_ERR_RDDMA_CH9_SHFT                                  21
#define HWIO_LPASS_LPAIF_IRQ2_RAW_STATa_UNDR_RDDMA_CH9_BMSK                           0x100000
#define HWIO_LPASS_LPAIF_IRQ2_RAW_STATa_UNDR_RDDMA_CH9_SHFT                                 20
#define HWIO_LPASS_LPAIF_IRQ2_RAW_STATa_PER_RDDMA_CH9_BMSK                             0x80000
#define HWIO_LPASS_LPAIF_IRQ2_RAW_STATa_PER_RDDMA_CH9_SHFT                                  19
#define HWIO_LPASS_LPAIF_IRQ2_RAW_STATa_ERR_RDDMA_CH8_BMSK                             0x40000
#define HWIO_LPASS_LPAIF_IRQ2_RAW_STATa_ERR_RDDMA_CH8_SHFT                                  18
#define HWIO_LPASS_LPAIF_IRQ2_RAW_STATa_UNDR_RDDMA_CH8_BMSK                            0x20000
#define HWIO_LPASS_LPAIF_IRQ2_RAW_STATa_UNDR_RDDMA_CH8_SHFT                                 17
#define HWIO_LPASS_LPAIF_IRQ2_RAW_STATa_PER_RDDMA_CH8_BMSK                             0x10000
#define HWIO_LPASS_LPAIF_IRQ2_RAW_STATa_PER_RDDMA_CH8_SHFT                                  16
#define HWIO_LPASS_LPAIF_IRQ2_RAW_STATa_ERR_RDDMA_CH7_BMSK                              0x8000
#define HWIO_LPASS_LPAIF_IRQ2_RAW_STATa_ERR_RDDMA_CH7_SHFT                                  15
#define HWIO_LPASS_LPAIF_IRQ2_RAW_STATa_UNDR_RDDMA_CH7_BMSK                             0x4000
#define HWIO_LPASS_LPAIF_IRQ2_RAW_STATa_UNDR_RDDMA_CH7_SHFT                                 14
#define HWIO_LPASS_LPAIF_IRQ2_RAW_STATa_PER_RDDMA_CH7_BMSK                              0x2000
#define HWIO_LPASS_LPAIF_IRQ2_RAW_STATa_PER_RDDMA_CH7_SHFT                                  13
#define HWIO_LPASS_LPAIF_IRQ2_RAW_STATa_ERR_RDDMA_CH6_BMSK                              0x1000
#define HWIO_LPASS_LPAIF_IRQ2_RAW_STATa_ERR_RDDMA_CH6_SHFT                                  12
#define HWIO_LPASS_LPAIF_IRQ2_RAW_STATa_UNDR_RDDMA_CH6_BMSK                              0x800
#define HWIO_LPASS_LPAIF_IRQ2_RAW_STATa_UNDR_RDDMA_CH6_SHFT                                 11
#define HWIO_LPASS_LPAIF_IRQ2_RAW_STATa_PER_RDDMA_CH6_BMSK                               0x400
#define HWIO_LPASS_LPAIF_IRQ2_RAW_STATa_PER_RDDMA_CH6_SHFT                                  10
#define HWIO_LPASS_LPAIF_IRQ2_RAW_STATa_ERR_WRDMA_CH5_BMSK                               0x200
#define HWIO_LPASS_LPAIF_IRQ2_RAW_STATa_ERR_WRDMA_CH5_SHFT                                   9
#define HWIO_LPASS_LPAIF_IRQ2_RAW_STATa_OVR_WRDMA_CH5_BMSK                               0x100
#define HWIO_LPASS_LPAIF_IRQ2_RAW_STATa_OVR_WRDMA_CH5_SHFT                                   8
#define HWIO_LPASS_LPAIF_IRQ2_RAW_STATa_PER_WRDMA_CH5_BMSK                                0x80
#define HWIO_LPASS_LPAIF_IRQ2_RAW_STATa_PER_WRDMA_CH5_SHFT                                   7
#define HWIO_LPASS_LPAIF_IRQ2_RAW_STATa_ERR_WRDMA_CH4_BMSK                                0x40
#define HWIO_LPASS_LPAIF_IRQ2_RAW_STATa_ERR_WRDMA_CH4_SHFT                                   6
#define HWIO_LPASS_LPAIF_IRQ2_RAW_STATa_OVR_WRDMA_CH4_BMSK                                0x20
#define HWIO_LPASS_LPAIF_IRQ2_RAW_STATa_OVR_WRDMA_CH4_SHFT                                   5
#define HWIO_LPASS_LPAIF_IRQ2_RAW_STATa_PER_WRDMA_CH4_BMSK                                0x10
#define HWIO_LPASS_LPAIF_IRQ2_RAW_STATa_PER_WRDMA_CH4_SHFT                                   4
#define HWIO_LPASS_LPAIF_IRQ2_RAW_STATa_ERR_RDDMA_CH5_BMSK                                 0x8
#define HWIO_LPASS_LPAIF_IRQ2_RAW_STATa_ERR_RDDMA_CH5_SHFT                                   3
#define HWIO_LPASS_LPAIF_IRQ2_RAW_STATa_UNDR_RDDMA_CH5_BMSK                                0x4
#define HWIO_LPASS_LPAIF_IRQ2_RAW_STATa_UNDR_RDDMA_CH5_SHFT                                  2
#define HWIO_LPASS_LPAIF_IRQ2_RAW_STATa_PER_RDDMA_CH5_BMSK                                 0x2
#define HWIO_LPASS_LPAIF_IRQ2_RAW_STATa_PER_RDDMA_CH5_SHFT                                   1
#define HWIO_LPASS_LPAIF_IRQ2_RAW_STATa_UNDR_EXTERNAL_BMSK                                 0x1
#define HWIO_LPASS_LPAIF_IRQ2_RAW_STATa_UNDR_EXTERNAL_SHFT                                   0

#define HWIO_LPASS_LPAIF_IRQ2_CLEARa_ADDR(base,a)                                   ((base) + 0X9020 + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_IRQ2_CLEARa_OFFS(a)                                        (0X9020 + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_IRQ2_CLEARa_RMSK                                           0x7fffffff
#define HWIO_LPASS_LPAIF_IRQ2_CLEARa_MAXa                                                    2
#define HWIO_LPASS_LPAIF_IRQ2_CLEARa_OUTI(base,a,val)        \
                out_dword(HWIO_LPASS_LPAIF_IRQ2_CLEARa_ADDR(base,a),val)
#define HWIO_LPASS_LPAIF_IRQ2_CLEARa_ERR_WRDMA_CH8_BMSK                             0x40000000
#define HWIO_LPASS_LPAIF_IRQ2_CLEARa_ERR_WRDMA_CH8_SHFT                                     30
#define HWIO_LPASS_LPAIF_IRQ2_CLEARa_OVR_WRDMA_CH8_BMSK                             0x20000000
#define HWIO_LPASS_LPAIF_IRQ2_CLEARa_OVR_WRDMA_CH8_SHFT                                     29
#define HWIO_LPASS_LPAIF_IRQ2_CLEARa_PER_WRDMA_CH8_BMSK                             0x10000000
#define HWIO_LPASS_LPAIF_IRQ2_CLEARa_PER_WRDMA_CH8_SHFT                                     28
#define HWIO_LPASS_LPAIF_IRQ2_CLEARa_ERR_WRDMA_CH7_BMSK                              0x8000000
#define HWIO_LPASS_LPAIF_IRQ2_CLEARa_ERR_WRDMA_CH7_SHFT                                     27
#define HWIO_LPASS_LPAIF_IRQ2_CLEARa_OVR_WRDMA_CH7_BMSK                              0x4000000
#define HWIO_LPASS_LPAIF_IRQ2_CLEARa_OVR_WRDMA_CH7_SHFT                                     26
#define HWIO_LPASS_LPAIF_IRQ2_CLEARa_PER_WRDMA_CH7_BMSK                              0x2000000
#define HWIO_LPASS_LPAIF_IRQ2_CLEARa_PER_WRDMA_CH7_SHFT                                     25
#define HWIO_LPASS_LPAIF_IRQ2_CLEARa_ERR_WRDMA_CH6_BMSK                              0x1000000
#define HWIO_LPASS_LPAIF_IRQ2_CLEARa_ERR_WRDMA_CH6_SHFT                                     24
#define HWIO_LPASS_LPAIF_IRQ2_CLEARa_OVR_WRDMA_CH6_BMSK                               0x800000
#define HWIO_LPASS_LPAIF_IRQ2_CLEARa_OVR_WRDMA_CH6_SHFT                                     23
#define HWIO_LPASS_LPAIF_IRQ2_CLEARa_PER_WRDMA_CH6_BMSK                               0x400000
#define HWIO_LPASS_LPAIF_IRQ2_CLEARa_PER_WRDMA_CH6_SHFT                                     22
#define HWIO_LPASS_LPAIF_IRQ2_CLEARa_ERR_RDDMA_CH9_BMSK                               0x200000
#define HWIO_LPASS_LPAIF_IRQ2_CLEARa_ERR_RDDMA_CH9_SHFT                                     21
#define HWIO_LPASS_LPAIF_IRQ2_CLEARa_UNDR_RDDMA_CH9_BMSK                              0x100000
#define HWIO_LPASS_LPAIF_IRQ2_CLEARa_UNDR_RDDMA_CH9_SHFT                                    20
#define HWIO_LPASS_LPAIF_IRQ2_CLEARa_PER_RDDMA_CH9_BMSK                                0x80000
#define HWIO_LPASS_LPAIF_IRQ2_CLEARa_PER_RDDMA_CH9_SHFT                                     19
#define HWIO_LPASS_LPAIF_IRQ2_CLEARa_ERR_RDDMA_CH8_BMSK                                0x40000
#define HWIO_LPASS_LPAIF_IRQ2_CLEARa_ERR_RDDMA_CH8_SHFT                                     18
#define HWIO_LPASS_LPAIF_IRQ2_CLEARa_UNDR_RDDMA_CH8_BMSK                               0x20000
#define HWIO_LPASS_LPAIF_IRQ2_CLEARa_UNDR_RDDMA_CH8_SHFT                                    17
#define HWIO_LPASS_LPAIF_IRQ2_CLEARa_PER_RDDMA_CH8_BMSK                                0x10000
#define HWIO_LPASS_LPAIF_IRQ2_CLEARa_PER_RDDMA_CH8_SHFT                                     16
#define HWIO_LPASS_LPAIF_IRQ2_CLEARa_ERR_RDDMA_CH7_BMSK                                 0x8000
#define HWIO_LPASS_LPAIF_IRQ2_CLEARa_ERR_RDDMA_CH7_SHFT                                     15
#define HWIO_LPASS_LPAIF_IRQ2_CLEARa_UNDR_RDDMA_CH7_BMSK                                0x4000
#define HWIO_LPASS_LPAIF_IRQ2_CLEARa_UNDR_RDDMA_CH7_SHFT                                    14
#define HWIO_LPASS_LPAIF_IRQ2_CLEARa_PER_RDDMA_CH7_BMSK                                 0x2000
#define HWIO_LPASS_LPAIF_IRQ2_CLEARa_PER_RDDMA_CH7_SHFT                                     13
#define HWIO_LPASS_LPAIF_IRQ2_CLEARa_ERR_RDDMA_CH6_BMSK                                 0x1000
#define HWIO_LPASS_LPAIF_IRQ2_CLEARa_ERR_RDDMA_CH6_SHFT                                     12
#define HWIO_LPASS_LPAIF_IRQ2_CLEARa_UNDR_RDDMA_CH6_BMSK                                 0x800
#define HWIO_LPASS_LPAIF_IRQ2_CLEARa_UNDR_RDDMA_CH6_SHFT                                    11
#define HWIO_LPASS_LPAIF_IRQ2_CLEARa_PER_RDDMA_CH6_BMSK                                  0x400
#define HWIO_LPASS_LPAIF_IRQ2_CLEARa_PER_RDDMA_CH6_SHFT                                     10
#define HWIO_LPASS_LPAIF_IRQ2_CLEARa_ERR_WRDMA_CH5_BMSK                                  0x200
#define HWIO_LPASS_LPAIF_IRQ2_CLEARa_ERR_WRDMA_CH5_SHFT                                      9
#define HWIO_LPASS_LPAIF_IRQ2_CLEARa_OVR_WRDMA_CH5_BMSK                                  0x100
#define HWIO_LPASS_LPAIF_IRQ2_CLEARa_OVR_WRDMA_CH5_SHFT                                      8
#define HWIO_LPASS_LPAIF_IRQ2_CLEARa_PER_WRDMA_CH5_BMSK                                   0x80
#define HWIO_LPASS_LPAIF_IRQ2_CLEARa_PER_WRDMA_CH5_SHFT                                      7
#define HWIO_LPASS_LPAIF_IRQ2_CLEARa_ERR_WRDMA_CH4_BMSK                                   0x40
#define HWIO_LPASS_LPAIF_IRQ2_CLEARa_ERR_WRDMA_CH4_SHFT                                      6
#define HWIO_LPASS_LPAIF_IRQ2_CLEARa_OVR_WRDMA_CH4_BMSK                                   0x20
#define HWIO_LPASS_LPAIF_IRQ2_CLEARa_OVR_WRDMA_CH4_SHFT                                      5
#define HWIO_LPASS_LPAIF_IRQ2_CLEARa_PER_WRDMA_CH4_BMSK                                   0x10
#define HWIO_LPASS_LPAIF_IRQ2_CLEARa_PER_WRDMA_CH4_SHFT                                      4
#define HWIO_LPASS_LPAIF_IRQ2_CLEARa_ERR_RDDMA_CH5_BMSK                                    0x8
#define HWIO_LPASS_LPAIF_IRQ2_CLEARa_ERR_RDDMA_CH5_SHFT                                      3
#define HWIO_LPASS_LPAIF_IRQ2_CLEARa_UNDR_RDDMA_CH5_BMSK                                   0x4
#define HWIO_LPASS_LPAIF_IRQ2_CLEARa_UNDR_RDDMA_CH5_SHFT                                     2
#define HWIO_LPASS_LPAIF_IRQ2_CLEARa_PER_RDDMA_CH5_BMSK                                    0x2
#define HWIO_LPASS_LPAIF_IRQ2_CLEARa_PER_RDDMA_CH5_SHFT                                      1
#define HWIO_LPASS_LPAIF_IRQ2_CLEARa_UNDR_EXTERNAL_BMSK                                    0x1
#define HWIO_LPASS_LPAIF_IRQ2_CLEARa_UNDR_EXTERNAL_SHFT                                      0

#define HWIO_LPASS_LPAIF_IRQ2_FORCEa_ADDR(base,a)                                   ((base) + 0X9024 + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_IRQ2_FORCEa_OFFS(a)                                        (0X9024 + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_IRQ2_FORCEa_RMSK                                           0x7fffffff
#define HWIO_LPASS_LPAIF_IRQ2_FORCEa_MAXa                                                    2
#define HWIO_LPASS_LPAIF_IRQ2_FORCEa_OUTI(base,a,val)        \
                out_dword(HWIO_LPASS_LPAIF_IRQ2_FORCEa_ADDR(base,a),val)
#define HWIO_LPASS_LPAIF_IRQ2_FORCEa_ERR_WRDMA_CH8_BMSK                             0x40000000
#define HWIO_LPASS_LPAIF_IRQ2_FORCEa_ERR_WRDMA_CH8_SHFT                                     30
#define HWIO_LPASS_LPAIF_IRQ2_FORCEa_OVR_WRDMA_CH8_BMSK                             0x20000000
#define HWIO_LPASS_LPAIF_IRQ2_FORCEa_OVR_WRDMA_CH8_SHFT                                     29
#define HWIO_LPASS_LPAIF_IRQ2_FORCEa_PER_WRDMA_CH8_BMSK                             0x10000000
#define HWIO_LPASS_LPAIF_IRQ2_FORCEa_PER_WRDMA_CH8_SHFT                                     28
#define HWIO_LPASS_LPAIF_IRQ2_FORCEa_ERR_WRDMA_CH7_BMSK                              0x8000000
#define HWIO_LPASS_LPAIF_IRQ2_FORCEa_ERR_WRDMA_CH7_SHFT                                     27
#define HWIO_LPASS_LPAIF_IRQ2_FORCEa_OVR_WRDMA_CH7_BMSK                              0x4000000
#define HWIO_LPASS_LPAIF_IRQ2_FORCEa_OVR_WRDMA_CH7_SHFT                                     26
#define HWIO_LPASS_LPAIF_IRQ2_FORCEa_PER_WRDMA_CH7_BMSK                              0x2000000
#define HWIO_LPASS_LPAIF_IRQ2_FORCEa_PER_WRDMA_CH7_SHFT                                     25
#define HWIO_LPASS_LPAIF_IRQ2_FORCEa_ERR_WRDMA_CH6_BMSK                              0x1000000
#define HWIO_LPASS_LPAIF_IRQ2_FORCEa_ERR_WRDMA_CH6_SHFT                                     24
#define HWIO_LPASS_LPAIF_IRQ2_FORCEa_OVR_WRDMA_CH6_BMSK                               0x800000
#define HWIO_LPASS_LPAIF_IRQ2_FORCEa_OVR_WRDMA_CH6_SHFT                                     23
#define HWIO_LPASS_LPAIF_IRQ2_FORCEa_PER_WRDMA_CH6_BMSK                               0x400000
#define HWIO_LPASS_LPAIF_IRQ2_FORCEa_PER_WRDMA_CH6_SHFT                                     22
#define HWIO_LPASS_LPAIF_IRQ2_FORCEa_ERR_RDDMA_CH9_BMSK                               0x200000
#define HWIO_LPASS_LPAIF_IRQ2_FORCEa_ERR_RDDMA_CH9_SHFT                                     21
#define HWIO_LPASS_LPAIF_IRQ2_FORCEa_UNDR_RDDMA_CH9_BMSK                              0x100000
#define HWIO_LPASS_LPAIF_IRQ2_FORCEa_UNDR_RDDMA_CH9_SHFT                                    20
#define HWIO_LPASS_LPAIF_IRQ2_FORCEa_PER_RDDMA_CH9_BMSK                                0x80000
#define HWIO_LPASS_LPAIF_IRQ2_FORCEa_PER_RDDMA_CH9_SHFT                                     19
#define HWIO_LPASS_LPAIF_IRQ2_FORCEa_ERR_RDDMA_CH8_BMSK                                0x40000
#define HWIO_LPASS_LPAIF_IRQ2_FORCEa_ERR_RDDMA_CH8_SHFT                                     18
#define HWIO_LPASS_LPAIF_IRQ2_FORCEa_UNDR_RDDMA_CH8_BMSK                               0x20000
#define HWIO_LPASS_LPAIF_IRQ2_FORCEa_UNDR_RDDMA_CH8_SHFT                                    17
#define HWIO_LPASS_LPAIF_IRQ2_FORCEa_PER_RDDMA_CH8_BMSK                                0x10000
#define HWIO_LPASS_LPAIF_IRQ2_FORCEa_PER_RDDMA_CH8_SHFT                                     16
#define HWIO_LPASS_LPAIF_IRQ2_FORCEa_ERR_RDDMA_CH7_BMSK                                 0x8000
#define HWIO_LPASS_LPAIF_IRQ2_FORCEa_ERR_RDDMA_CH7_SHFT                                     15
#define HWIO_LPASS_LPAIF_IRQ2_FORCEa_UNDR_RDDMA_CH7_BMSK                                0x4000
#define HWIO_LPASS_LPAIF_IRQ2_FORCEa_UNDR_RDDMA_CH7_SHFT                                    14
#define HWIO_LPASS_LPAIF_IRQ2_FORCEa_PER_RDDMA_CH7_BMSK                                 0x2000
#define HWIO_LPASS_LPAIF_IRQ2_FORCEa_PER_RDDMA_CH7_SHFT                                     13
#define HWIO_LPASS_LPAIF_IRQ2_FORCEa_ERR_RDDMA_CH6_BMSK                                 0x1000
#define HWIO_LPASS_LPAIF_IRQ2_FORCEa_ERR_RDDMA_CH6_SHFT                                     12
#define HWIO_LPASS_LPAIF_IRQ2_FORCEa_UNDR_RDDMA_CH6_BMSK                                 0x800
#define HWIO_LPASS_LPAIF_IRQ2_FORCEa_UNDR_RDDMA_CH6_SHFT                                    11
#define HWIO_LPASS_LPAIF_IRQ2_FORCEa_PER_RDDMA_CH6_BMSK                                  0x400
#define HWIO_LPASS_LPAIF_IRQ2_FORCEa_PER_RDDMA_CH6_SHFT                                     10
#define HWIO_LPASS_LPAIF_IRQ2_FORCEa_ERR_WRDMA_CH5_BMSK                                  0x200
#define HWIO_LPASS_LPAIF_IRQ2_FORCEa_ERR_WRDMA_CH5_SHFT                                      9
#define HWIO_LPASS_LPAIF_IRQ2_FORCEa_OVR_WRDMA_CH5_BMSK                                  0x100
#define HWIO_LPASS_LPAIF_IRQ2_FORCEa_OVR_WRDMA_CH5_SHFT                                      8
#define HWIO_LPASS_LPAIF_IRQ2_FORCEa_PER_WRDMA_CH5_BMSK                                   0x80
#define HWIO_LPASS_LPAIF_IRQ2_FORCEa_PER_WRDMA_CH5_SHFT                                      7
#define HWIO_LPASS_LPAIF_IRQ2_FORCEa_ERR_WRDMA_CH4_BMSK                                   0x40
#define HWIO_LPASS_LPAIF_IRQ2_FORCEa_ERR_WRDMA_CH4_SHFT                                      6
#define HWIO_LPASS_LPAIF_IRQ2_FORCEa_OVR_WRDMA_CH4_BMSK                                   0x20
#define HWIO_LPASS_LPAIF_IRQ2_FORCEa_OVR_WRDMA_CH4_SHFT                                      5
#define HWIO_LPASS_LPAIF_IRQ2_FORCEa_PER_WRDMA_CH4_BMSK                                   0x10
#define HWIO_LPASS_LPAIF_IRQ2_FORCEa_PER_WRDMA_CH4_SHFT                                      4
#define HWIO_LPASS_LPAIF_IRQ2_FORCEa_ERR_RDDMA_CH5_BMSK                                    0x8
#define HWIO_LPASS_LPAIF_IRQ2_FORCEa_ERR_RDDMA_CH5_SHFT                                      3
#define HWIO_LPASS_LPAIF_IRQ2_FORCEa_UNDR_RDDMA_CH5_BMSK                                   0x4
#define HWIO_LPASS_LPAIF_IRQ2_FORCEa_UNDR_RDDMA_CH5_SHFT                                     2
#define HWIO_LPASS_LPAIF_IRQ2_FORCEa_PER_RDDMA_CH5_BMSK                                    0x2
#define HWIO_LPASS_LPAIF_IRQ2_FORCEa_PER_RDDMA_CH5_SHFT                                      1
#define HWIO_LPASS_LPAIF_IRQ2_FORCEa_UNDR_EXTERNAL_BMSK                                    0x1
#define HWIO_LPASS_LPAIF_IRQ2_FORCEa_UNDR_EXTERNAL_SHFT                                      0

#define HWIO_LPASS_LPAIF_IRQ3_ENa_ADDR(base,a)                                      ((base) + 0X9028 + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_IRQ3_ENa_OFFS(a)                                           (0X9028 + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_IRQ3_ENa_RMSK                                                   0x3ff
#define HWIO_LPASS_LPAIF_IRQ3_ENa_MAXa                                                       2
#define HWIO_LPASS_LPAIF_IRQ3_ENa_INI(base,a)                \
                in_dword_masked(HWIO_LPASS_LPAIF_IRQ3_ENa_ADDR(base,a), HWIO_LPASS_LPAIF_IRQ3_ENa_RMSK)
#define HWIO_LPASS_LPAIF_IRQ3_ENa_INMI(base,a,mask)        \
                in_dword_masked(HWIO_LPASS_LPAIF_IRQ3_ENa_ADDR(base,a), mask)
#define HWIO_LPASS_LPAIF_IRQ3_ENa_OUTI(base,a,val)        \
                out_dword(HWIO_LPASS_LPAIF_IRQ3_ENa_ADDR(base,a),val)
#define HWIO_LPASS_LPAIF_IRQ3_ENa_OUTMI(base,a,mask,val) \
                out_dword_masked_ns(HWIO_LPASS_LPAIF_IRQ3_ENa_ADDR(base,a),mask,val,HWIO_LPASS_LPAIF_IRQ3_ENa_INI(base,a))
#define HWIO_LPASS_LPAIF_IRQ3_ENa_SECURITY_MISMATCH_ERR_BMSK                             0x200
#define HWIO_LPASS_LPAIF_IRQ3_ENa_SECURITY_MISMATCH_ERR_SHFT                                 9
#define HWIO_LPASS_LPAIF_IRQ3_ENa_ERR_RDDMA_CH11_BMSK                                    0x100
#define HWIO_LPASS_LPAIF_IRQ3_ENa_ERR_RDDMA_CH11_SHFT                                        8
#define HWIO_LPASS_LPAIF_IRQ3_ENa_UNDR_RDDMA_CH11_BMSK                                    0x80
#define HWIO_LPASS_LPAIF_IRQ3_ENa_UNDR_RDDMA_CH11_SHFT                                       7
#define HWIO_LPASS_LPAIF_IRQ3_ENa_PER_RDDMA_CH11_BMSK                                     0x40
#define HWIO_LPASS_LPAIF_IRQ3_ENa_PER_RDDMA_CH11_SHFT                                        6
#define HWIO_LPASS_LPAIF_IRQ3_ENa_ERR_RDDMA_CH10_BMSK                                     0x20
#define HWIO_LPASS_LPAIF_IRQ3_ENa_ERR_RDDMA_CH10_SHFT                                        5
#define HWIO_LPASS_LPAIF_IRQ3_ENa_UNDR_RDDMA_CH10_BMSK                                    0x10
#define HWIO_LPASS_LPAIF_IRQ3_ENa_UNDR_RDDMA_CH10_SHFT                                       4
#define HWIO_LPASS_LPAIF_IRQ3_ENa_PER_RDDMA_CH10_BMSK                                      0x8
#define HWIO_LPASS_LPAIF_IRQ3_ENa_PER_RDDMA_CH10_SHFT                                        3
#define HWIO_LPASS_LPAIF_IRQ3_ENa_ERR_WRDMA_CH9_BMSK                                       0x4
#define HWIO_LPASS_LPAIF_IRQ3_ENa_ERR_WRDMA_CH9_SHFT                                         2
#define HWIO_LPASS_LPAIF_IRQ3_ENa_OVR_WRDMA_CH9_BMSK                                       0x2
#define HWIO_LPASS_LPAIF_IRQ3_ENa_OVR_WRDMA_CH9_SHFT                                         1
#define HWIO_LPASS_LPAIF_IRQ3_ENa_PER_WRDMA_CH9_BMSK                                       0x1
#define HWIO_LPASS_LPAIF_IRQ3_ENa_PER_WRDMA_CH9_SHFT                                         0

#define HWIO_LPASS_LPAIF_IRQ3_STATa_ADDR(base,a)                                    ((base) + 0X902C + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_IRQ3_STATa_OFFS(a)                                         (0X902C + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_IRQ3_STATa_RMSK                                                 0x3ff
#define HWIO_LPASS_LPAIF_IRQ3_STATa_MAXa                                                     2
#define HWIO_LPASS_LPAIF_IRQ3_STATa_INI(base,a)                \
                in_dword_masked(HWIO_LPASS_LPAIF_IRQ3_STATa_ADDR(base,a), HWIO_LPASS_LPAIF_IRQ3_STATa_RMSK)
#define HWIO_LPASS_LPAIF_IRQ3_STATa_INMI(base,a,mask)        \
                in_dword_masked(HWIO_LPASS_LPAIF_IRQ3_STATa_ADDR(base,a), mask)
#define HWIO_LPASS_LPAIF_IRQ3_STATa_SECURITY_MISMATCH_ERR_BMSK                           0x200
#define HWIO_LPASS_LPAIF_IRQ3_STATa_SECURITY_MISMATCH_ERR_SHFT                               9
#define HWIO_LPASS_LPAIF_IRQ3_STATa_ERR_RDDMA_CH11_BMSK                                  0x100
#define HWIO_LPASS_LPAIF_IRQ3_STATa_ERR_RDDMA_CH11_SHFT                                      8
#define HWIO_LPASS_LPAIF_IRQ3_STATa_UNDR_RDDMA_CH11_BMSK                                  0x80
#define HWIO_LPASS_LPAIF_IRQ3_STATa_UNDR_RDDMA_CH11_SHFT                                     7
#define HWIO_LPASS_LPAIF_IRQ3_STATa_PER_RDDMA_CH11_BMSK                                   0x40
#define HWIO_LPASS_LPAIF_IRQ3_STATa_PER_RDDMA_CH11_SHFT                                      6
#define HWIO_LPASS_LPAIF_IRQ3_STATa_ERR_RDDMA_CH10_BMSK                                   0x20
#define HWIO_LPASS_LPAIF_IRQ3_STATa_ERR_RDDMA_CH10_SHFT                                      5
#define HWIO_LPASS_LPAIF_IRQ3_STATa_UNDR_RDDMA_CH10_BMSK                                  0x10
#define HWIO_LPASS_LPAIF_IRQ3_STATa_UNDR_RDDMA_CH10_SHFT                                     4
#define HWIO_LPASS_LPAIF_IRQ3_STATa_PER_RDDMA_CH10_BMSK                                    0x8
#define HWIO_LPASS_LPAIF_IRQ3_STATa_PER_RDDMA_CH10_SHFT                                      3
#define HWIO_LPASS_LPAIF_IRQ3_STATa_ERR_WRDMA_CH9_BMSK                                     0x4
#define HWIO_LPASS_LPAIF_IRQ3_STATa_ERR_WRDMA_CH9_SHFT                                       2
#define HWIO_LPASS_LPAIF_IRQ3_STATa_OVR_WRDMA_CH9_BMSK                                     0x2
#define HWIO_LPASS_LPAIF_IRQ3_STATa_OVR_WRDMA_CH9_SHFT                                       1
#define HWIO_LPASS_LPAIF_IRQ3_STATa_PER_WRDMA_CH9_BMSK                                     0x1
#define HWIO_LPASS_LPAIF_IRQ3_STATa_PER_WRDMA_CH9_SHFT                                       0

#define HWIO_LPASS_LPAIF_IRQ3_RAW_STATa_ADDR(base,a)                                ((base) + 0X9030 + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_IRQ3_RAW_STATa_OFFS(a)                                     (0X9030 + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_IRQ3_RAW_STATa_RMSK                                             0x3ff
#define HWIO_LPASS_LPAIF_IRQ3_RAW_STATa_MAXa                                                 2
#define HWIO_LPASS_LPAIF_IRQ3_RAW_STATa_INI(base,a)                \
                in_dword_masked(HWIO_LPASS_LPAIF_IRQ3_RAW_STATa_ADDR(base,a), HWIO_LPASS_LPAIF_IRQ3_RAW_STATa_RMSK)
#define HWIO_LPASS_LPAIF_IRQ3_RAW_STATa_INMI(base,a,mask)        \
                in_dword_masked(HWIO_LPASS_LPAIF_IRQ3_RAW_STATa_ADDR(base,a), mask)
#define HWIO_LPASS_LPAIF_IRQ3_RAW_STATa_SECURITY_MISMATCH_ERR_BMSK                       0x200
#define HWIO_LPASS_LPAIF_IRQ3_RAW_STATa_SECURITY_MISMATCH_ERR_SHFT                           9
#define HWIO_LPASS_LPAIF_IRQ3_RAW_STATa_ERR_RDDMA_CH11_BMSK                              0x100
#define HWIO_LPASS_LPAIF_IRQ3_RAW_STATa_ERR_RDDMA_CH11_SHFT                                  8
#define HWIO_LPASS_LPAIF_IRQ3_RAW_STATa_UNDR_RDDMA_CH11_BMSK                              0x80
#define HWIO_LPASS_LPAIF_IRQ3_RAW_STATa_UNDR_RDDMA_CH11_SHFT                                 7
#define HWIO_LPASS_LPAIF_IRQ3_RAW_STATa_PER_RDDMA_CH11_BMSK                               0x40
#define HWIO_LPASS_LPAIF_IRQ3_RAW_STATa_PER_RDDMA_CH11_SHFT                                  6
#define HWIO_LPASS_LPAIF_IRQ3_RAW_STATa_ERR_RDDMA_CH10_BMSK                               0x20
#define HWIO_LPASS_LPAIF_IRQ3_RAW_STATa_ERR_RDDMA_CH10_SHFT                                  5
#define HWIO_LPASS_LPAIF_IRQ3_RAW_STATa_UNDR_RDDMA_CH10_BMSK                              0x10
#define HWIO_LPASS_LPAIF_IRQ3_RAW_STATa_UNDR_RDDMA_CH10_SHFT                                 4
#define HWIO_LPASS_LPAIF_IRQ3_RAW_STATa_PER_RDDMA_CH10_BMSK                                0x8
#define HWIO_LPASS_LPAIF_IRQ3_RAW_STATa_PER_RDDMA_CH10_SHFT                                  3
#define HWIO_LPASS_LPAIF_IRQ3_RAW_STATa_ERR_WRDMA_CH9_BMSK                                 0x4
#define HWIO_LPASS_LPAIF_IRQ3_RAW_STATa_ERR_WRDMA_CH9_SHFT                                   2
#define HWIO_LPASS_LPAIF_IRQ3_RAW_STATa_OVR_WRDMA_CH9_BMSK                                 0x2
#define HWIO_LPASS_LPAIF_IRQ3_RAW_STATa_OVR_WRDMA_CH9_SHFT                                   1
#define HWIO_LPASS_LPAIF_IRQ3_RAW_STATa_PER_WRDMA_CH9_BMSK                                 0x1
#define HWIO_LPASS_LPAIF_IRQ3_RAW_STATa_PER_WRDMA_CH9_SHFT                                   0

#define HWIO_LPASS_LPAIF_IRQ3_CLEARa_ADDR(base,a)                                   ((base) + 0X9034 + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_IRQ3_CLEARa_OFFS(a)                                        (0X9034 + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_IRQ3_CLEARa_RMSK                                                0x3ff
#define HWIO_LPASS_LPAIF_IRQ3_CLEARa_MAXa                                                    2
#define HWIO_LPASS_LPAIF_IRQ3_CLEARa_OUTI(base,a,val)        \
                out_dword(HWIO_LPASS_LPAIF_IRQ3_CLEARa_ADDR(base,a),val)
#define HWIO_LPASS_LPAIF_IRQ3_CLEARa_SECURITY_MISMATCH_ERR_BMSK                          0x200
#define HWIO_LPASS_LPAIF_IRQ3_CLEARa_SECURITY_MISMATCH_ERR_SHFT                              9
#define HWIO_LPASS_LPAIF_IRQ3_CLEARa_ERR_RDDMA_CH11_BMSK                                 0x100
#define HWIO_LPASS_LPAIF_IRQ3_CLEARa_ERR_RDDMA_CH11_SHFT                                     8
#define HWIO_LPASS_LPAIF_IRQ3_CLEARa_UNDR_RDDMA_CH11_BMSK                                 0x80
#define HWIO_LPASS_LPAIF_IRQ3_CLEARa_UNDR_RDDMA_CH11_SHFT                                    7
#define HWIO_LPASS_LPAIF_IRQ3_CLEARa_PER_RDDMA_CH11_BMSK                                  0x40
#define HWIO_LPASS_LPAIF_IRQ3_CLEARa_PER_RDDMA_CH11_SHFT                                     6
#define HWIO_LPASS_LPAIF_IRQ3_CLEARa_ERR_RDDMA_CH10_BMSK                                  0x20
#define HWIO_LPASS_LPAIF_IRQ3_CLEARa_ERR_RDDMA_CH10_SHFT                                     5
#define HWIO_LPASS_LPAIF_IRQ3_CLEARa_UNDR_RDDMA_CH10_BMSK                                 0x10
#define HWIO_LPASS_LPAIF_IRQ3_CLEARa_UNDR_RDDMA_CH10_SHFT                                    4
#define HWIO_LPASS_LPAIF_IRQ3_CLEARa_PER_RDDMA_CH10_BMSK                                   0x8
#define HWIO_LPASS_LPAIF_IRQ3_CLEARa_PER_RDDMA_CH10_SHFT                                     3
#define HWIO_LPASS_LPAIF_IRQ3_CLEARa_ERR_WRDMA_CH9_BMSK                                    0x4
#define HWIO_LPASS_LPAIF_IRQ3_CLEARa_ERR_WRDMA_CH9_SHFT                                      2
#define HWIO_LPASS_LPAIF_IRQ3_CLEARa_OVR_WRDMA_CH9_BMSK                                    0x2
#define HWIO_LPASS_LPAIF_IRQ3_CLEARa_OVR_WRDMA_CH9_SHFT                                      1
#define HWIO_LPASS_LPAIF_IRQ3_CLEARa_PER_WRDMA_CH9_BMSK                                    0x1
#define HWIO_LPASS_LPAIF_IRQ3_CLEARa_PER_WRDMA_CH9_SHFT                                      0

#define HWIO_LPASS_LPAIF_IRQ3_FORCEa_ADDR(base,a)                                   ((base) + 0X9038 + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_IRQ3_FORCEa_OFFS(a)                                        (0X9038 + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_IRQ3_FORCEa_RMSK                                                0x3ff
#define HWIO_LPASS_LPAIF_IRQ3_FORCEa_MAXa                                                    2
#define HWIO_LPASS_LPAIF_IRQ3_FORCEa_OUTI(base,a,val)        \
                out_dword(HWIO_LPASS_LPAIF_IRQ3_FORCEa_ADDR(base,a),val)
#define HWIO_LPASS_LPAIF_IRQ3_FORCEa_SECURITY_MISMATCH_ERR_BMSK                          0x200
#define HWIO_LPASS_LPAIF_IRQ3_FORCEa_SECURITY_MISMATCH_ERR_SHFT                              9
#define HWIO_LPASS_LPAIF_IRQ3_FORCEa_ERR_RDDMA_CH11_BMSK                                 0x100
#define HWIO_LPASS_LPAIF_IRQ3_FORCEa_ERR_RDDMA_CH11_SHFT                                     8
#define HWIO_LPASS_LPAIF_IRQ3_FORCEa_UNDR_RDDMA_CH11_BMSK                                 0x80
#define HWIO_LPASS_LPAIF_IRQ3_FORCEa_UNDR_RDDMA_CH11_SHFT                                    7
#define HWIO_LPASS_LPAIF_IRQ3_FORCEa_PER_RDDMA_CH11_BMSK                                  0x40
#define HWIO_LPASS_LPAIF_IRQ3_FORCEa_PER_RDDMA_CH11_SHFT                                     6
#define HWIO_LPASS_LPAIF_IRQ3_FORCEa_ERR_RDDMA_CH10_BMSK                                  0x20
#define HWIO_LPASS_LPAIF_IRQ3_FORCEa_ERR_RDDMA_CH10_SHFT                                     5
#define HWIO_LPASS_LPAIF_IRQ3_FORCEa_UNDR_RDDMA_CH10_BMSK                                 0x10
#define HWIO_LPASS_LPAIF_IRQ3_FORCEa_UNDR_RDDMA_CH10_SHFT                                    4
#define HWIO_LPASS_LPAIF_IRQ3_FORCEa_PER_RDDMA_CH10_BMSK                                   0x8
#define HWIO_LPASS_LPAIF_IRQ3_FORCEa_PER_RDDMA_CH10_SHFT                                     3
#define HWIO_LPASS_LPAIF_IRQ3_FORCEa_ERR_WRDMA_CH9_BMSK                                    0x4
#define HWIO_LPASS_LPAIF_IRQ3_FORCEa_ERR_WRDMA_CH9_SHFT                                      2
#define HWIO_LPASS_LPAIF_IRQ3_FORCEa_OVR_WRDMA_CH9_BMSK                                    0x2
#define HWIO_LPASS_LPAIF_IRQ3_FORCEa_OVR_WRDMA_CH9_SHFT                                      1
#define HWIO_LPASS_LPAIF_IRQ3_FORCEa_PER_WRDMA_CH9_BMSK                                    0x1
#define HWIO_LPASS_LPAIF_IRQ3_FORCEa_PER_WRDMA_CH9_SHFT                                      0

#define HWIO_LPASS_LPAIF_SPDIF_IRQ_ENa_ADDR(base,a)                                 ((base) + 0X903C + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_SPDIF_IRQ_ENa_OFFS(a)                                      (0X903C + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_SPDIF_IRQ_ENa_RMSK                                                0x3
#define HWIO_LPASS_LPAIF_SPDIF_IRQ_ENa_MAXa                                                  2
#define HWIO_LPASS_LPAIF_SPDIF_IRQ_ENa_INI(base,a)                \
                in_dword_masked(HWIO_LPASS_LPAIF_SPDIF_IRQ_ENa_ADDR(base,a), HWIO_LPASS_LPAIF_SPDIF_IRQ_ENa_RMSK)
#define HWIO_LPASS_LPAIF_SPDIF_IRQ_ENa_INMI(base,a,mask)        \
                in_dword_masked(HWIO_LPASS_LPAIF_SPDIF_IRQ_ENa_ADDR(base,a), mask)
#define HWIO_LPASS_LPAIF_SPDIF_IRQ_ENa_OUTI(base,a,val)        \
                out_dword(HWIO_LPASS_LPAIF_SPDIF_IRQ_ENa_ADDR(base,a),val)
#define HWIO_LPASS_LPAIF_SPDIF_IRQ_ENa_OUTMI(base,a,mask,val) \
                out_dword_masked_ns(HWIO_LPASS_LPAIF_SPDIF_IRQ_ENa_ADDR(base,a),mask,val,HWIO_LPASS_LPAIF_SPDIF_IRQ_ENa_INI(base,a))
#define HWIO_LPASS_LPAIF_SPDIF_IRQ_ENa_SEC_SPDIFRX_EN_BMSK                                 0x2
#define HWIO_LPASS_LPAIF_SPDIF_IRQ_ENa_SEC_SPDIFRX_EN_SHFT                                   1
#define HWIO_LPASS_LPAIF_SPDIF_IRQ_ENa_PRI_SPDIFRX_EN_BMSK                                 0x1
#define HWIO_LPASS_LPAIF_SPDIF_IRQ_ENa_PRI_SPDIFRX_EN_SHFT                                   0

#define HWIO_LPASS_LPAIF_RDDMA_CTLa_ADDR(base,a)                                    ((base) + 0XC000 + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_RDDMA_CTLa_POR							0x00200000
#define HWIO_LPASS_LPAIF_RDDMA_CTLa_OFFS(a)                                         (0XC000 + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_RDDMA_CTLa_RMSK                                            0xfffff03f
#define HWIO_LPASS_LPAIF_RDDMA_CTLa_MAXa                                                     2
#define HWIO_LPASS_LPAIF_RDDMA_CTLa_INI(base,a)                \
                in_dword_masked(HWIO_LPASS_LPAIF_RDDMA_CTLa_ADDR(base,a), HWIO_LPASS_LPAIF_RDDMA_CTLa_RMSK)
#define HWIO_LPASS_LPAIF_RDDMA_CTLa_INMI(base,a,mask)        \
                in_dword_masked(HWIO_LPASS_LPAIF_RDDMA_CTLa_ADDR(base,a), mask)
#define HWIO_LPASS_LPAIF_RDDMA_CTLa_OUTI(base,a,val)        \
                out_dword(HWIO_LPASS_LPAIF_RDDMA_CTLa_ADDR(base,a),val)
#define HWIO_LPASS_LPAIF_RDDMA_CTLa_OUTMI(base,a,mask,val) \
                out_dword_masked_ns(HWIO_LPASS_LPAIF_RDDMA_CTLa_ADDR(base,a),mask,val,HWIO_LPASS_LPAIF_RDDMA_CTLa_INI(base,a))
#define HWIO_LPASS_LPAIF_RDDMA_CTLa_RESET_BMSK                                      0x80000000
#define HWIO_LPASS_LPAIF_RDDMA_CTLa_RESET_SHFT                                              31
#define HWIO_LPASS_LPAIF_RDDMA_CTLa_RESET_DISABLE_FVAL                                     0x0
#define HWIO_LPASS_LPAIF_RDDMA_CTLa_RESET_ENABLE_FVAL                                      0x1
#define HWIO_LPASS_LPAIF_RDDMA_CTLa_DYNBURST_EN_BMSK                                0x40000000
#define HWIO_LPASS_LPAIF_RDDMA_CTLa_DYNBURST_EN_SHFT                                        30
#define HWIO_LPASS_LPAIF_RDDMA_CTLa_DYNBURST_EN_DISABLE_FVAL                               0x0
#define HWIO_LPASS_LPAIF_RDDMA_CTLa_DYNBURST_EN_ENABLE_FVAL                                0x1
#define HWIO_LPASS_LPAIF_RDDMA_CTLa_BURST16_EN_BMSK                                 0x20000000
#define HWIO_LPASS_LPAIF_RDDMA_CTLa_BURST16_EN_SHFT                                         29
#define HWIO_LPASS_LPAIF_RDDMA_CTLa_BURST16_EN_DISABLE_FVAL                                0x0
#define HWIO_LPASS_LPAIF_RDDMA_CTLa_BURST16_EN_ENABLE_FVAL                                 0x1
#define HWIO_LPASS_LPAIF_RDDMA_CTLa_BURST8_EN_BMSK                                  0x10000000
#define HWIO_LPASS_LPAIF_RDDMA_CTLa_BURST8_EN_SHFT                                          28
#define HWIO_LPASS_LPAIF_RDDMA_CTLa_BURST8_EN_DISABLE_FVAL                                 0x0
#define HWIO_LPASS_LPAIF_RDDMA_CTLa_BURST8_EN_ENABLE_FVAL                                  0x1
#define HWIO_LPASS_LPAIF_RDDMA_CTLa_PADDING_NUM_BMSK                                 0xf800000
#define HWIO_LPASS_LPAIF_RDDMA_CTLa_PADDING_NUM_SHFT                                        23
#define HWIO_LPASS_LPAIF_RDDMA_CTLa_PADDING_EN_BMSK                                   0x400000
#define HWIO_LPASS_LPAIF_RDDMA_CTLa_PADDING_EN_SHFT                                         22
#define HWIO_LPASS_LPAIF_RDDMA_CTLa_PADDING_EN_DISABLE_FVAL                                0x0
#define HWIO_LPASS_LPAIF_RDDMA_CTLa_PADDING_EN_ENABLE_FVAL                                 0x1
#define HWIO_LPASS_LPAIF_RDDMA_CTLa_DYNAMIC_CLOCK_BMSK                                0x200000
#define HWIO_LPASS_LPAIF_RDDMA_CTLa_DYNAMIC_CLOCK_SHFT                                      21
#define HWIO_LPASS_LPAIF_RDDMA_CTLa_DYNAMIC_CLOCK_OFF_FVAL                                 0x0
#define HWIO_LPASS_LPAIF_RDDMA_CTLa_DYNAMIC_CLOCK_ON_FVAL                                  0x1
#define HWIO_LPASS_LPAIF_RDDMA_CTLa_BURST_EN_BMSK                                     0x100000
#define HWIO_LPASS_LPAIF_RDDMA_CTLa_BURST_EN_SHFT                                           20
#define HWIO_LPASS_LPAIF_RDDMA_CTLa_BURST_EN_SINGLE_FVAL                                   0x0
#define HWIO_LPASS_LPAIF_RDDMA_CTLa_BURST_EN_INCR4_FVAL                                    0x1
#define HWIO_LPASS_LPAIF_RDDMA_CTLa_WPSCNT_BMSK                                        0xf0000
#define HWIO_LPASS_LPAIF_RDDMA_CTLa_WPSCNT_SHFT                                             16
#define HWIO_LPASS_LPAIF_RDDMA_CTLa_WPSCNT_ONE_FVAL                                        0x0
#define HWIO_LPASS_LPAIF_RDDMA_CTLa_WPSCNT_TWO_FVAL                                        0x1
#define HWIO_LPASS_LPAIF_RDDMA_CTLa_WPSCNT_THREE_FVAL                                      0x2
#define HWIO_LPASS_LPAIF_RDDMA_CTLa_WPSCNT_FOUR_FVAL                                       0x3
#define HWIO_LPASS_LPAIF_RDDMA_CTLa_WPSCNT_SIX_FVAL                                        0x5
#define HWIO_LPASS_LPAIF_RDDMA_CTLa_WPSCNT_EIGHT_FVAL                                      0x7
#define HWIO_LPASS_LPAIF_RDDMA_CTLa_WPSCNT_TEN_FVAL                                        0x9
#define HWIO_LPASS_LPAIF_RDDMA_CTLa_WPSCNT_TWELVE_FVAL                                     0xb
#define HWIO_LPASS_LPAIF_RDDMA_CTLa_WPSCNT_FOURTEEN_FVAL                                   0xd
#define HWIO_LPASS_LPAIF_RDDMA_CTLa_WPSCNT_SIXTEEN_FVAL                                    0xf
#define HWIO_LPASS_LPAIF_RDDMA_CTLa_AUDIO_INTF_BMSK                                     0xf000
#define HWIO_LPASS_LPAIF_RDDMA_CTLa_AUDIO_INTF_SHFT                                         12
#define HWIO_LPASS_LPAIF_RDDMA_CTLa_AUDIO_INTF_NONE_FVAL                                   0x0
#define HWIO_LPASS_LPAIF_RDDMA_CTLa_AUDIO_INTF_PRI_SRC_FVAL                                0x1
#define HWIO_LPASS_LPAIF_RDDMA_CTLa_AUDIO_INTF_SEC_SRC_FVAL                                0x2
#define HWIO_LPASS_LPAIF_RDDMA_CTLa_AUDIO_INTF_TER_SRC_FVAL                                0x3
#define HWIO_LPASS_LPAIF_RDDMA_CTLa_AUDIO_INTF_QUA_SRC_FVAL                                0x4
#define HWIO_LPASS_LPAIF_RDDMA_CTLa_AUDIO_INTF_SPKR_I2S_FVAL                               0x5
#define HWIO_LPASS_LPAIF_RDDMA_CTLa_AUDIO_INTF_HDMI_FVAL                                   0x6
#define HWIO_LPASS_LPAIF_RDDMA_CTLa_AUDIO_INTF_QUI_SRC_FVAL                                0xe
#define HWIO_LPASS_LPAIF_RDDMA_CTLa_FIFO_WATERMRK_BMSK                                    0x3e
#define HWIO_LPASS_LPAIF_RDDMA_CTLa_FIFO_WATERMRK_SHFT                                       1
#define HWIO_LPASS_LPAIF_RDDMA_CTLa_FIFO_WATERMRK_ENUM_1_FVAL                              0x0
#define HWIO_LPASS_LPAIF_RDDMA_CTLa_FIFO_WATERMRK_ENUM_2_FVAL                              0x1
#define HWIO_LPASS_LPAIF_RDDMA_CTLa_FIFO_WATERMRK_ENUM_3_FVAL                              0x2
#define HWIO_LPASS_LPAIF_RDDMA_CTLa_FIFO_WATERMRK_ENUM_4_FVAL                              0x3
#define HWIO_LPASS_LPAIF_RDDMA_CTLa_FIFO_WATERMRK_ENUM_5_FVAL                              0x4
#define HWIO_LPASS_LPAIF_RDDMA_CTLa_FIFO_WATERMRK_ENUM_6_FVAL                              0x5
#define HWIO_LPASS_LPAIF_RDDMA_CTLa_FIFO_WATERMRK_ENUM_7_FVAL                              0x6
#define HWIO_LPASS_LPAIF_RDDMA_CTLa_FIFO_WATERMRK_ENUM_8_FVAL                              0x7
#define HWIO_LPASS_LPAIF_RDDMA_CTLa_FIFO_WATERMRK_ENUM_9_FVAL                              0x8
#define HWIO_LPASS_LPAIF_RDDMA_CTLa_FIFO_WATERMRK_ENUM_10_FVAL                             0x9
#define HWIO_LPASS_LPAIF_RDDMA_CTLa_FIFO_WATERMRK_ENUM_11_FVAL                             0xa
#define HWIO_LPASS_LPAIF_RDDMA_CTLa_FIFO_WATERMRK_ENUM_12_FVAL                             0xb
#define HWIO_LPASS_LPAIF_RDDMA_CTLa_FIFO_WATERMRK_ENUM_13_FVAL                             0xc
#define HWIO_LPASS_LPAIF_RDDMA_CTLa_FIFO_WATERMRK_ENUM_14_FVAL                             0xd
#define HWIO_LPASS_LPAIF_RDDMA_CTLa_FIFO_WATERMRK_ENUM_15_FVAL                             0xe
#define HWIO_LPASS_LPAIF_RDDMA_CTLa_FIFO_WATERMRK_ENUM_16_FVAL                             0xf
#define HWIO_LPASS_LPAIF_RDDMA_CTLa_FIFO_WATERMRK_ENUM_17_FVAL                            0x10
#define HWIO_LPASS_LPAIF_RDDMA_CTLa_FIFO_WATERMRK_ENUM_18_FVAL                            0x11
#define HWIO_LPASS_LPAIF_RDDMA_CTLa_FIFO_WATERMRK_ENUM_19_FVAL                            0x12
#define HWIO_LPASS_LPAIF_RDDMA_CTLa_FIFO_WATERMRK_ENUM_20_FVAL                            0x13
#define HWIO_LPASS_LPAIF_RDDMA_CTLa_FIFO_WATERMRK_ENUM_21_FVAL                            0x14
#define HWIO_LPASS_LPAIF_RDDMA_CTLa_FIFO_WATERMRK_ENUM_22_FVAL                            0x15
#define HWIO_LPASS_LPAIF_RDDMA_CTLa_FIFO_WATERMRK_ENUM_23_FVAL                            0x16
#define HWIO_LPASS_LPAIF_RDDMA_CTLa_FIFO_WATERMRK_ENUM_24_FVAL                            0x17
#define HWIO_LPASS_LPAIF_RDDMA_CTLa_FIFO_WATERMRK_ENUM_25_FVAL                            0x18
#define HWIO_LPASS_LPAIF_RDDMA_CTLa_FIFO_WATERMRK_ENUM_26_FVAL                            0x19
#define HWIO_LPASS_LPAIF_RDDMA_CTLa_FIFO_WATERMRK_ENUM_27_FVAL                            0x1a
#define HWIO_LPASS_LPAIF_RDDMA_CTLa_FIFO_WATERMRK_ENUM_28_FVAL                            0x1b
#define HWIO_LPASS_LPAIF_RDDMA_CTLa_FIFO_WATERMRK_ENUM_29_FVAL                            0x1c
#define HWIO_LPASS_LPAIF_RDDMA_CTLa_FIFO_WATERMRK_ENUM_30_FVAL                            0x1d
#define HWIO_LPASS_LPAIF_RDDMA_CTLa_FIFO_WATERMRK_ENUM_31_FVAL                            0x1e
#define HWIO_LPASS_LPAIF_RDDMA_CTLa_FIFO_WATERMRK_ENUM_32_FVAL                            0x1f
#define HWIO_LPASS_LPAIF_RDDMA_CTLa_ENABLE_BMSK                                            0x1
#define HWIO_LPASS_LPAIF_RDDMA_CTLa_ENABLE_SHFT                                              0
#define HWIO_LPASS_LPAIF_RDDMA_CTLa_ENABLE_OFF_FVAL                                        0x0
#define HWIO_LPASS_LPAIF_RDDMA_CTLa_ENABLE_ON_FVAL                                         0x1

#define HWIO_LPASS_LPAIF_RDDMA_BASEa_ADDR(base,a)                                   ((base) + 0XC004 + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_RDDMA_BASEa_OFFS(a)                                        (0XC004 + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_RDDMA_BASEa_RMSK                                           0xfffffff0
#define HWIO_LPASS_LPAIF_RDDMA_BASEa_MAXa                                                    2
#define HWIO_LPASS_LPAIF_RDDMA_BASEa_INI(base,a)                \
                in_dword_masked(HWIO_LPASS_LPAIF_RDDMA_BASEa_ADDR(base,a), HWIO_LPASS_LPAIF_RDDMA_BASEa_RMSK)
#define HWIO_LPASS_LPAIF_RDDMA_BASEa_INMI(base,a,mask)        \
                in_dword_masked(HWIO_LPASS_LPAIF_RDDMA_BASEa_ADDR(base,a), mask)
#define HWIO_LPASS_LPAIF_RDDMA_BASEa_OUTI(base,a,val)        \
                out_dword(HWIO_LPASS_LPAIF_RDDMA_BASEa_ADDR(base,a),val)
#define HWIO_LPASS_LPAIF_RDDMA_BASEa_OUTMI(base,a,mask,val) \
                out_dword_masked_ns(HWIO_LPASS_LPAIF_RDDMA_BASEa_ADDR(base,a),mask,val,HWIO_LPASS_LPAIF_RDDMA_BASEa_INI(base,a))
#define HWIO_LPASS_LPAIF_RDDMA_BASEa_BASE_ADDR_BMSK                                 0xfffffff0
#define HWIO_LPASS_LPAIF_RDDMA_BASEa_BASE_ADDR_SHFT                                          4

#define HWIO_LPASS_LPAIF_RDDMA_BUFF_LENa_ADDR(base,a)                               ((base) + 0XC008 + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_RDDMA_BUFF_LENa_OFFS(a)                                    (0XC008 + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_RDDMA_BUFF_LENa_RMSK                                          0xffffc
#define HWIO_LPASS_LPAIF_RDDMA_BUFF_LENa_MAXa                                                2
#define HWIO_LPASS_LPAIF_RDDMA_BUFF_LENa_INI(base,a)                \
                in_dword_masked(HWIO_LPASS_LPAIF_RDDMA_BUFF_LENa_ADDR(base,a), HWIO_LPASS_LPAIF_RDDMA_BUFF_LENa_RMSK)
#define HWIO_LPASS_LPAIF_RDDMA_BUFF_LENa_INMI(base,a,mask)        \
                in_dword_masked(HWIO_LPASS_LPAIF_RDDMA_BUFF_LENa_ADDR(base,a), mask)
#define HWIO_LPASS_LPAIF_RDDMA_BUFF_LENa_OUTI(base,a,val)        \
                out_dword(HWIO_LPASS_LPAIF_RDDMA_BUFF_LENa_ADDR(base,a),val)
#define HWIO_LPASS_LPAIF_RDDMA_BUFF_LENa_OUTMI(base,a,mask,val) \
                out_dword_masked_ns(HWIO_LPASS_LPAIF_RDDMA_BUFF_LENa_ADDR(base,a),mask,val,HWIO_LPASS_LPAIF_RDDMA_BUFF_LENa_INI(base,a))
#define HWIO_LPASS_LPAIF_RDDMA_BUFF_LENa_LENGTH_BMSK                                   0xffffc
#define HWIO_LPASS_LPAIF_RDDMA_BUFF_LENa_LENGTH_SHFT                                         2

#define HWIO_LPASS_LPAIF_RDDMA_CURR_ADDRa_ADDR(base,a)                              ((base) + 0XC00C + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_RDDMA_CURR_ADDRa_OFFS(a)                                   (0XC00C + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_RDDMA_CURR_ADDRa_RMSK                                      0xffffffff
#define HWIO_LPASS_LPAIF_RDDMA_CURR_ADDRa_MAXa                                               2
#define HWIO_LPASS_LPAIF_RDDMA_CURR_ADDRa_INI(base,a)                \
                in_dword_masked(HWIO_LPASS_LPAIF_RDDMA_CURR_ADDRa_ADDR(base,a), HWIO_LPASS_LPAIF_RDDMA_CURR_ADDRa_RMSK)
#define HWIO_LPASS_LPAIF_RDDMA_CURR_ADDRa_INMI(base,a,mask)        \
                in_dword_masked(HWIO_LPASS_LPAIF_RDDMA_CURR_ADDRa_ADDR(base,a), mask)
#define HWIO_LPASS_LPAIF_RDDMA_CURR_ADDRa_ADDR_BMSK                                 0xffffffff
#define HWIO_LPASS_LPAIF_RDDMA_CURR_ADDRa_ADDR_SHFT                                          0

#define HWIO_LPASS_LPAIF_RDDMA_PER_LENa_ADDR(base,a)                                ((base) + 0XC010 + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_RDDMA_PER_LENa_OFFS(a)                                     (0XC010 + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_RDDMA_PER_LENa_RMSK                                           0xfffff
#define HWIO_LPASS_LPAIF_RDDMA_PER_LENa_MAXa                                                 2
#define HWIO_LPASS_LPAIF_RDDMA_PER_LENa_INI(base,a)                \
                in_dword_masked(HWIO_LPASS_LPAIF_RDDMA_PER_LENa_ADDR(base,a), HWIO_LPASS_LPAIF_RDDMA_PER_LENa_RMSK)
#define HWIO_LPASS_LPAIF_RDDMA_PER_LENa_INMI(base,a,mask)        \
                in_dword_masked(HWIO_LPASS_LPAIF_RDDMA_PER_LENa_ADDR(base,a), mask)
#define HWIO_LPASS_LPAIF_RDDMA_PER_LENa_OUTI(base,a,val)        \
                out_dword(HWIO_LPASS_LPAIF_RDDMA_PER_LENa_ADDR(base,a),val)
#define HWIO_LPASS_LPAIF_RDDMA_PER_LENa_OUTMI(base,a,mask,val) \
                out_dword_masked_ns(HWIO_LPASS_LPAIF_RDDMA_PER_LENa_ADDR(base,a),mask,val,HWIO_LPASS_LPAIF_RDDMA_PER_LENa_INI(base,a))
#define HWIO_LPASS_LPAIF_RDDMA_PER_LENa_LENGTH_BMSK                                    0xfffff
#define HWIO_LPASS_LPAIF_RDDMA_PER_LENa_LENGTH_SHFT                                          0

#define HWIO_LPASS_LPAIF_RDDMA_PER_CNTa_ADDR(base,a)                                ((base) + 0XC014 + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_RDDMA_PER_CNTa_OFFS(a)                                     (0XC014 + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_RDDMA_PER_CNTa_RMSK                                        0x43ffffff
#define HWIO_LPASS_LPAIF_RDDMA_PER_CNTa_MAXa                                                 2
#define HWIO_LPASS_LPAIF_RDDMA_PER_CNTa_INI(base,a)                \
                in_dword_masked(HWIO_LPASS_LPAIF_RDDMA_PER_CNTa_ADDR(base,a), HWIO_LPASS_LPAIF_RDDMA_PER_CNTa_RMSK)
#define HWIO_LPASS_LPAIF_RDDMA_PER_CNTa_INMI(base,a,mask)        \
                in_dword_masked(HWIO_LPASS_LPAIF_RDDMA_PER_CNTa_ADDR(base,a), mask)
#define HWIO_LPASS_LPAIF_RDDMA_PER_CNTa_FORMAT_ERR_BMSK                             0x40000000
#define HWIO_LPASS_LPAIF_RDDMA_PER_CNTa_FORMAT_ERR_SHFT                                     30
#define HWIO_LPASS_LPAIF_RDDMA_PER_CNTa_FIFO_WORDCNT_BMSK                            0x3f00000
#define HWIO_LPASS_LPAIF_RDDMA_PER_CNTa_FIFO_WORDCNT_SHFT                                   20
#define HWIO_LPASS_LPAIF_RDDMA_PER_CNTa_FIFO_WORDCNT_ENUM_0_FVAL                           0x0
#define HWIO_LPASS_LPAIF_RDDMA_PER_CNTa_FIFO_WORDCNT_ENUM_1_FVAL                           0x1
#define HWIO_LPASS_LPAIF_RDDMA_PER_CNTa_FIFO_WORDCNT_ENUM_2_FVAL                           0x2
#define HWIO_LPASS_LPAIF_RDDMA_PER_CNTa_FIFO_WORDCNT_ENUM_3_FVAL                           0x3
#define HWIO_LPASS_LPAIF_RDDMA_PER_CNTa_FIFO_WORDCNT_ENUM_4_FVAL                           0x4
#define HWIO_LPASS_LPAIF_RDDMA_PER_CNTa_FIFO_WORDCNT_ENUM_5_FVAL                           0x5
#define HWIO_LPASS_LPAIF_RDDMA_PER_CNTa_FIFO_WORDCNT_ENUM_6_FVAL                           0x6
#define HWIO_LPASS_LPAIF_RDDMA_PER_CNTa_FIFO_WORDCNT_ENUM_7_FVAL                           0x7
#define HWIO_LPASS_LPAIF_RDDMA_PER_CNTa_FIFO_WORDCNT_ENUM_8_FVAL                           0x8
#define HWIO_LPASS_LPAIF_RDDMA_PER_CNTa_FIFO_WORDCNT_ENUM_9_FVAL                           0x9
#define HWIO_LPASS_LPAIF_RDDMA_PER_CNTa_FIFO_WORDCNT_ENUM_10_FVAL                          0xa
#define HWIO_LPASS_LPAIF_RDDMA_PER_CNTa_FIFO_WORDCNT_ENUM_11_FVAL                          0xb
#define HWIO_LPASS_LPAIF_RDDMA_PER_CNTa_FIFO_WORDCNT_ENUM_12_FVAL                          0xc
#define HWIO_LPASS_LPAIF_RDDMA_PER_CNTa_FIFO_WORDCNT_ENUM_13_FVAL                          0xd
#define HWIO_LPASS_LPAIF_RDDMA_PER_CNTa_FIFO_WORDCNT_ENUM_14_FVAL                          0xe
#define HWIO_LPASS_LPAIF_RDDMA_PER_CNTa_FIFO_WORDCNT_ENUM_15_FVAL                          0xf
#define HWIO_LPASS_LPAIF_RDDMA_PER_CNTa_FIFO_WORDCNT_ENUM_16_FVAL                         0x10
#define HWIO_LPASS_LPAIF_RDDMA_PER_CNTa_FIFO_WORDCNT_ENUM_17_FVAL                         0x11
#define HWIO_LPASS_LPAIF_RDDMA_PER_CNTa_FIFO_WORDCNT_ENUM_18_FVAL                         0x12
#define HWIO_LPASS_LPAIF_RDDMA_PER_CNTa_FIFO_WORDCNT_ENUM_19_FVAL                         0x13
#define HWIO_LPASS_LPAIF_RDDMA_PER_CNTa_FIFO_WORDCNT_ENUM_20_FVAL                         0x14
#define HWIO_LPASS_LPAIF_RDDMA_PER_CNTa_FIFO_WORDCNT_ENUM_21_FVAL                         0x15
#define HWIO_LPASS_LPAIF_RDDMA_PER_CNTa_FIFO_WORDCNT_ENUM_22_FVAL                         0x16
#define HWIO_LPASS_LPAIF_RDDMA_PER_CNTa_FIFO_WORDCNT_ENUM_23_FVAL                         0x17
#define HWIO_LPASS_LPAIF_RDDMA_PER_CNTa_FIFO_WORDCNT_ENUM_24_FVAL                         0x18
#define HWIO_LPASS_LPAIF_RDDMA_PER_CNTa_FIFO_WORDCNT_ENUM_25_FVAL                         0x19
#define HWIO_LPASS_LPAIF_RDDMA_PER_CNTa_FIFO_WORDCNT_ENUM_26_FVAL                         0x1a
#define HWIO_LPASS_LPAIF_RDDMA_PER_CNTa_FIFO_WORDCNT_ENUM_27_FVAL                         0x1b
#define HWIO_LPASS_LPAIF_RDDMA_PER_CNTa_FIFO_WORDCNT_ENUM_28_FVAL                         0x1c
#define HWIO_LPASS_LPAIF_RDDMA_PER_CNTa_FIFO_WORDCNT_ENUM_29_FVAL                         0x1d
#define HWIO_LPASS_LPAIF_RDDMA_PER_CNTa_FIFO_WORDCNT_ENUM_30_FVAL                         0x1e
#define HWIO_LPASS_LPAIF_RDDMA_PER_CNTa_FIFO_WORDCNT_ENUM_31_FVAL                         0x1f
#define HWIO_LPASS_LPAIF_RDDMA_PER_CNTa_FIFO_WORDCNT_ENUM_32_FVAL                         0x20
#define HWIO_LPASS_LPAIF_RDDMA_PER_CNTa_PER_CNT_BMSK                                   0xfffff
#define HWIO_LPASS_LPAIF_RDDMA_PER_CNTa_PER_CNT_SHFT                                         0

#define HWIO_LPASS_LPAIF_RDDMA_PER_CNT_EXTa_ADDR(base,a)                            ((base) + 0XC018 + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_RDDMA_PER_CNT_EXTa_OFFS(a)                                 (0XC018 + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_RDDMA_PER_CNT_EXTa_RMSK                                         0xfff
#define HWIO_LPASS_LPAIF_RDDMA_PER_CNT_EXTa_MAXa                                             2
#define HWIO_LPASS_LPAIF_RDDMA_PER_CNT_EXTa_INI(base,a)                \
                in_dword_masked(HWIO_LPASS_LPAIF_RDDMA_PER_CNT_EXTa_ADDR(base,a), HWIO_LPASS_LPAIF_RDDMA_PER_CNT_EXTa_RMSK)
#define HWIO_LPASS_LPAIF_RDDMA_PER_CNT_EXTa_INMI(base,a,mask)        \
                in_dword_masked(HWIO_LPASS_LPAIF_RDDMA_PER_CNT_EXTa_ADDR(base,a), mask)
#define HWIO_LPASS_LPAIF_RDDMA_PER_CNT_EXTa_FIFO_WORDCNT_BMSK                            0xfff
#define HWIO_LPASS_LPAIF_RDDMA_PER_CNT_EXTa_FIFO_WORDCNT_SHFT                                0
#define HWIO_LPASS_LPAIF_RDDMA_PER_CNT_EXTa_FIFO_WORDCNT_ENUM_0_FVAL                       0x0
#define HWIO_LPASS_LPAIF_RDDMA_PER_CNT_EXTa_FIFO_WORDCNT_ENUM_1_FVAL                       0x1
#define HWIO_LPASS_LPAIF_RDDMA_PER_CNT_EXTa_FIFO_WORDCNT_ENUM_2_FVAL                       0x2
#define HWIO_LPASS_LPAIF_RDDMA_PER_CNT_EXTa_FIFO_WORDCNT_ENUM_3_FVAL                       0x3
#define HWIO_LPASS_LPAIF_RDDMA_PER_CNT_EXTa_FIFO_WORDCNT_ENUM_4_FVAL                       0x4
#define HWIO_LPASS_LPAIF_RDDMA_PER_CNT_EXTa_FIFO_WORDCNT_ENUM_5_FVAL                       0x5
#define HWIO_LPASS_LPAIF_RDDMA_PER_CNT_EXTa_FIFO_WORDCNT_ENUM_6_FVAL                       0x6
#define HWIO_LPASS_LPAIF_RDDMA_PER_CNT_EXTa_FIFO_WORDCNT_ENUM_7_FVAL                       0x7
#define HWIO_LPASS_LPAIF_RDDMA_PER_CNT_EXTa_FIFO_WORDCNT_ENUM_8_FVAL                       0x8
#define HWIO_LPASS_LPAIF_RDDMA_PER_CNT_EXTa_FIFO_WORDCNT_ENUM_9_FVAL                       0x9
#define HWIO_LPASS_LPAIF_RDDMA_PER_CNT_EXTa_FIFO_WORDCNT_ENUM_10_FVAL                      0xa
#define HWIO_LPASS_LPAIF_RDDMA_PER_CNT_EXTa_FIFO_WORDCNT_ENUM_11_FVAL                      0xb
#define HWIO_LPASS_LPAIF_RDDMA_PER_CNT_EXTa_FIFO_WORDCNT_ENUM_12_FVAL                      0xc
#define HWIO_LPASS_LPAIF_RDDMA_PER_CNT_EXTa_FIFO_WORDCNT_ENUM_13_FVAL                      0xd
#define HWIO_LPASS_LPAIF_RDDMA_PER_CNT_EXTa_FIFO_WORDCNT_ENUM_14_FVAL                      0xe
#define HWIO_LPASS_LPAIF_RDDMA_PER_CNT_EXTa_FIFO_WORDCNT_ENUM_15_FVAL                      0xf
#define HWIO_LPASS_LPAIF_RDDMA_PER_CNT_EXTa_FIFO_WORDCNT_ENUM_16_FVAL                     0x10
#define HWIO_LPASS_LPAIF_RDDMA_PER_CNT_EXTa_FIFO_WORDCNT_ENUM_17_FVAL                     0x11
#define HWIO_LPASS_LPAIF_RDDMA_PER_CNT_EXTa_FIFO_WORDCNT_ENUM_18_FVAL                     0x12
#define HWIO_LPASS_LPAIF_RDDMA_PER_CNT_EXTa_FIFO_WORDCNT_ENUM_19_FVAL                     0x13
#define HWIO_LPASS_LPAIF_RDDMA_PER_CNT_EXTa_FIFO_WORDCNT_ENUM_20_FVAL                     0x14
#define HWIO_LPASS_LPAIF_RDDMA_PER_CNT_EXTa_FIFO_WORDCNT_ENUM_21_FVAL                     0x15
#define HWIO_LPASS_LPAIF_RDDMA_PER_CNT_EXTa_FIFO_WORDCNT_ENUM_22_FVAL                     0x16
#define HWIO_LPASS_LPAIF_RDDMA_PER_CNT_EXTa_FIFO_WORDCNT_ENUM_23_FVAL                     0x17
#define HWIO_LPASS_LPAIF_RDDMA_PER_CNT_EXTa_FIFO_WORDCNT_ENUM_24_FVAL                     0x18
#define HWIO_LPASS_LPAIF_RDDMA_PER_CNT_EXTa_FIFO_WORDCNT_ENUM_25_FVAL                     0x19
#define HWIO_LPASS_LPAIF_RDDMA_PER_CNT_EXTa_FIFO_WORDCNT_ENUM_26_FVAL                     0x1a
#define HWIO_LPASS_LPAIF_RDDMA_PER_CNT_EXTa_FIFO_WORDCNT_ENUM_27_FVAL                     0x1b
#define HWIO_LPASS_LPAIF_RDDMA_PER_CNT_EXTa_FIFO_WORDCNT_ENUM_28_FVAL                     0x1c
#define HWIO_LPASS_LPAIF_RDDMA_PER_CNT_EXTa_FIFO_WORDCNT_ENUM_29_FVAL                     0x1d
#define HWIO_LPASS_LPAIF_RDDMA_PER_CNT_EXTa_FIFO_WORDCNT_ENUM_30_FVAL                     0x1e
#define HWIO_LPASS_LPAIF_RDDMA_PER_CNT_EXTa_FIFO_WORDCNT_ENUM_31_FVAL                     0x1f
#define HWIO_LPASS_LPAIF_RDDMA_PER_CNT_EXTa_FIFO_WORDCNT_ENUM_32_FVAL                     0x20

#define HWIO_LPASS_LPAIF_RDDMA_FRMa_ADDR(base,a)                                    ((base) + 0XC01C + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_RDDMA_FRMa_OFFS(a)                                         (0XC01C + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_RDDMA_FRMa_RMSK                                            0x7fffffff
#define HWIO_LPASS_LPAIF_RDDMA_FRMa_MAXa                                                     2
#define HWIO_LPASS_LPAIF_RDDMA_FRMa_INI(base,a)                \
                in_dword_masked(HWIO_LPASS_LPAIF_RDDMA_FRMa_ADDR(base,a), HWIO_LPASS_LPAIF_RDDMA_FRMa_RMSK)
#define HWIO_LPASS_LPAIF_RDDMA_FRMa_INMI(base,a,mask)        \
                in_dword_masked(HWIO_LPASS_LPAIF_RDDMA_FRMa_ADDR(base,a), mask)
#define HWIO_LPASS_LPAIF_RDDMA_FRMa_FRAME_EXP_CNT_BMSK                              0x78000000
#define HWIO_LPASS_LPAIF_RDDMA_FRMa_FRAME_EXP_CNT_SHFT                                      27
#define HWIO_LPASS_LPAIF_RDDMA_FRMa_FRAME_UPDATE_BMSK                                0x4000000
#define HWIO_LPASS_LPAIF_RDDMA_FRMa_FRAME_UPDATE_SHFT                                       26
#define HWIO_LPASS_LPAIF_RDDMA_FRMa_FRAME_FIFO_BMSK                                  0x3f00000
#define HWIO_LPASS_LPAIF_RDDMA_FRMa_FRAME_FIFO_SHFT                                         20
#define HWIO_LPASS_LPAIF_RDDMA_FRMa_FRAME_FIFO_ENUM_0_FVAL                                 0x0
#define HWIO_LPASS_LPAIF_RDDMA_FRMa_FRAME_FIFO_ENUM_1_FVAL                                 0x1
#define HWIO_LPASS_LPAIF_RDDMA_FRMa_FRAME_FIFO_ENUM_2_FVAL                                 0x2
#define HWIO_LPASS_LPAIF_RDDMA_FRMa_FRAME_FIFO_ENUM_3_FVAL                                 0x3
#define HWIO_LPASS_LPAIF_RDDMA_FRMa_FRAME_FIFO_ENUM_4_FVAL                                 0x4
#define HWIO_LPASS_LPAIF_RDDMA_FRMa_FRAME_FIFO_ENUM_5_FVAL                                 0x5
#define HWIO_LPASS_LPAIF_RDDMA_FRMa_FRAME_FIFO_ENUM_6_FVAL                                 0x6
#define HWIO_LPASS_LPAIF_RDDMA_FRMa_FRAME_FIFO_ENUM_7_FVAL                                 0x7
#define HWIO_LPASS_LPAIF_RDDMA_FRMa_FRAME_FIFO_ENUM_8_FVAL                                 0x8
#define HWIO_LPASS_LPAIF_RDDMA_FRMa_FRAME_FIFO_ENUM_9_FVAL                                 0x9
#define HWIO_LPASS_LPAIF_RDDMA_FRMa_FRAME_FIFO_ENUM_10_FVAL                                0xa
#define HWIO_LPASS_LPAIF_RDDMA_FRMa_FRAME_FIFO_ENUM_11_FVAL                                0xb
#define HWIO_LPASS_LPAIF_RDDMA_FRMa_FRAME_FIFO_ENUM_12_FVAL                                0xc
#define HWIO_LPASS_LPAIF_RDDMA_FRMa_FRAME_FIFO_ENUM_13_FVAL                                0xd
#define HWIO_LPASS_LPAIF_RDDMA_FRMa_FRAME_FIFO_ENUM_14_FVAL                                0xe
#define HWIO_LPASS_LPAIF_RDDMA_FRMa_FRAME_FIFO_ENUM_15_FVAL                                0xf
#define HWIO_LPASS_LPAIF_RDDMA_FRMa_FRAME_FIFO_ENUM_16_FVAL                               0x10
#define HWIO_LPASS_LPAIF_RDDMA_FRMa_FRAME_FIFO_ENUM_17_FVAL                               0x11
#define HWIO_LPASS_LPAIF_RDDMA_FRMa_FRAME_FIFO_ENUM_18_FVAL                               0x12
#define HWIO_LPASS_LPAIF_RDDMA_FRMa_FRAME_FIFO_ENUM_19_FVAL                               0x13
#define HWIO_LPASS_LPAIF_RDDMA_FRMa_FRAME_FIFO_ENUM_20_FVAL                               0x14
#define HWIO_LPASS_LPAIF_RDDMA_FRMa_FRAME_FIFO_ENUM_21_FVAL                               0x15
#define HWIO_LPASS_LPAIF_RDDMA_FRMa_FRAME_FIFO_ENUM_22_FVAL                               0x16
#define HWIO_LPASS_LPAIF_RDDMA_FRMa_FRAME_FIFO_ENUM_23_FVAL                               0x17
#define HWIO_LPASS_LPAIF_RDDMA_FRMa_FRAME_FIFO_ENUM_24_FVAL                               0x18
#define HWIO_LPASS_LPAIF_RDDMA_FRMa_FRAME_FIFO_ENUM_25_FVAL                               0x19
#define HWIO_LPASS_LPAIF_RDDMA_FRMa_FRAME_FIFO_ENUM_26_FVAL                               0x1a
#define HWIO_LPASS_LPAIF_RDDMA_FRMa_FRAME_FIFO_ENUM_27_FVAL                               0x1b
#define HWIO_LPASS_LPAIF_RDDMA_FRMa_FRAME_FIFO_ENUM_28_FVAL                               0x1c
#define HWIO_LPASS_LPAIF_RDDMA_FRMa_FRAME_FIFO_ENUM_29_FVAL                               0x1d
#define HWIO_LPASS_LPAIF_RDDMA_FRMa_FRAME_FIFO_ENUM_30_FVAL                               0x1e
#define HWIO_LPASS_LPAIF_RDDMA_FRMa_FRAME_FIFO_ENUM_31_FVAL                               0x1f
#define HWIO_LPASS_LPAIF_RDDMA_FRMa_FRAME_FIFO_ENUM_32_FVAL                               0x20
#define HWIO_LPASS_LPAIF_RDDMA_FRMa_FRAME_CNT_BMSK                                     0xfffff
#define HWIO_LPASS_LPAIF_RDDMA_FRMa_FRAME_CNT_SHFT                                           0

#define HWIO_LPASS_LPAIF_RDDMA_FRM_EXTa_ADDR(base,a)                                ((base) + 0XC020 + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_RDDMA_FRM_EXTa_OFFS(a)                                     (0XC020 + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_RDDMA_FRM_EXTa_RMSK                                             0xfff
#define HWIO_LPASS_LPAIF_RDDMA_FRM_EXTa_MAXa                                                 2
#define HWIO_LPASS_LPAIF_RDDMA_FRM_EXTa_INI(base,a)                \
                in_dword_masked(HWIO_LPASS_LPAIF_RDDMA_FRM_EXTa_ADDR(base,a), HWIO_LPASS_LPAIF_RDDMA_FRM_EXTa_RMSK)
#define HWIO_LPASS_LPAIF_RDDMA_FRM_EXTa_INMI(base,a,mask)        \
                in_dword_masked(HWIO_LPASS_LPAIF_RDDMA_FRM_EXTa_ADDR(base,a), mask)
#define HWIO_LPASS_LPAIF_RDDMA_FRM_EXTa_FRAME_FIFO_BMSK                                  0xfff
#define HWIO_LPASS_LPAIF_RDDMA_FRM_EXTa_FRAME_FIFO_SHFT                                      0
#define HWIO_LPASS_LPAIF_RDDMA_FRM_EXTa_FRAME_FIFO_ENUM_0_FVAL                             0x0
#define HWIO_LPASS_LPAIF_RDDMA_FRM_EXTa_FRAME_FIFO_ENUM_1_FVAL                             0x1
#define HWIO_LPASS_LPAIF_RDDMA_FRM_EXTa_FRAME_FIFO_ENUM_2_FVAL                             0x2
#define HWIO_LPASS_LPAIF_RDDMA_FRM_EXTa_FRAME_FIFO_ENUM_3_FVAL                             0x3
#define HWIO_LPASS_LPAIF_RDDMA_FRM_EXTa_FRAME_FIFO_ENUM_4_FVAL                             0x4
#define HWIO_LPASS_LPAIF_RDDMA_FRM_EXTa_FRAME_FIFO_ENUM_5_FVAL                             0x5
#define HWIO_LPASS_LPAIF_RDDMA_FRM_EXTa_FRAME_FIFO_ENUM_6_FVAL                             0x6
#define HWIO_LPASS_LPAIF_RDDMA_FRM_EXTa_FRAME_FIFO_ENUM_7_FVAL                             0x7
#define HWIO_LPASS_LPAIF_RDDMA_FRM_EXTa_FRAME_FIFO_ENUM_8_FVAL                             0x8
#define HWIO_LPASS_LPAIF_RDDMA_FRM_EXTa_FRAME_FIFO_ENUM_9_FVAL                             0x9
#define HWIO_LPASS_LPAIF_RDDMA_FRM_EXTa_FRAME_FIFO_ENUM_10_FVAL                            0xa
#define HWIO_LPASS_LPAIF_RDDMA_FRM_EXTa_FRAME_FIFO_ENUM_11_FVAL                            0xb
#define HWIO_LPASS_LPAIF_RDDMA_FRM_EXTa_FRAME_FIFO_ENUM_12_FVAL                            0xc
#define HWIO_LPASS_LPAIF_RDDMA_FRM_EXTa_FRAME_FIFO_ENUM_13_FVAL                            0xd
#define HWIO_LPASS_LPAIF_RDDMA_FRM_EXTa_FRAME_FIFO_ENUM_14_FVAL                            0xe
#define HWIO_LPASS_LPAIF_RDDMA_FRM_EXTa_FRAME_FIFO_ENUM_15_FVAL                            0xf
#define HWIO_LPASS_LPAIF_RDDMA_FRM_EXTa_FRAME_FIFO_ENUM_16_FVAL                           0x10
#define HWIO_LPASS_LPAIF_RDDMA_FRM_EXTa_FRAME_FIFO_ENUM_17_FVAL                           0x11
#define HWIO_LPASS_LPAIF_RDDMA_FRM_EXTa_FRAME_FIFO_ENUM_18_FVAL                           0x12
#define HWIO_LPASS_LPAIF_RDDMA_FRM_EXTa_FRAME_FIFO_ENUM_19_FVAL                           0x13
#define HWIO_LPASS_LPAIF_RDDMA_FRM_EXTa_FRAME_FIFO_ENUM_20_FVAL                           0x14
#define HWIO_LPASS_LPAIF_RDDMA_FRM_EXTa_FRAME_FIFO_ENUM_21_FVAL                           0x15
#define HWIO_LPASS_LPAIF_RDDMA_FRM_EXTa_FRAME_FIFO_ENUM_22_FVAL                           0x16
#define HWIO_LPASS_LPAIF_RDDMA_FRM_EXTa_FRAME_FIFO_ENUM_23_FVAL                           0x17
#define HWIO_LPASS_LPAIF_RDDMA_FRM_EXTa_FRAME_FIFO_ENUM_24_FVAL                           0x18
#define HWIO_LPASS_LPAIF_RDDMA_FRM_EXTa_FRAME_FIFO_ENUM_25_FVAL                           0x19
#define HWIO_LPASS_LPAIF_RDDMA_FRM_EXTa_FRAME_FIFO_ENUM_26_FVAL                           0x1a
#define HWIO_LPASS_LPAIF_RDDMA_FRM_EXTa_FRAME_FIFO_ENUM_27_FVAL                           0x1b
#define HWIO_LPASS_LPAIF_RDDMA_FRM_EXTa_FRAME_FIFO_ENUM_28_FVAL                           0x1c
#define HWIO_LPASS_LPAIF_RDDMA_FRM_EXTa_FRAME_FIFO_ENUM_29_FVAL                           0x1d
#define HWIO_LPASS_LPAIF_RDDMA_FRM_EXTa_FRAME_FIFO_ENUM_30_FVAL                           0x1e
#define HWIO_LPASS_LPAIF_RDDMA_FRM_EXTa_FRAME_FIFO_ENUM_31_FVAL                           0x1f
#define HWIO_LPASS_LPAIF_RDDMA_FRM_EXTa_FRAME_FIFO_ENUM_32_FVAL                           0x20

#define HWIO_LPASS_LPAIF_RDDMA_FRMCLRa_ADDR(base,a)                                 ((base) + 0XC024 + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_RDDMA_FRMCLRa_OFFS(a)                                      (0XC024 + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_RDDMA_FRMCLRa_RMSK                                                0x1
#define HWIO_LPASS_LPAIF_RDDMA_FRMCLRa_MAXa                                                  2
#define HWIO_LPASS_LPAIF_RDDMA_FRMCLRa_OUTI(base,a,val)        \
                out_dword(HWIO_LPASS_LPAIF_RDDMA_FRMCLRa_ADDR(base,a),val)
#define HWIO_LPASS_LPAIF_RDDMA_FRMCLRa_FRAME_CLR_BMSK                                      0x1
#define HWIO_LPASS_LPAIF_RDDMA_FRMCLRa_FRAME_CLR_SHFT                                        0

#define HWIO_LPASS_LPAIF_RDDMA_SET_BUFF_CNTa_ADDR(base,a)                           ((base) + 0XC028 + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_RDDMA_SET_BUFF_CNTa_OFFS(a)                                (0XC028 + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_RDDMA_SET_BUFF_CNTa_RMSK                                      0xfffff
#define HWIO_LPASS_LPAIF_RDDMA_SET_BUFF_CNTa_MAXa                                            2
#define HWIO_LPASS_LPAIF_RDDMA_SET_BUFF_CNTa_OUTI(base,a,val)        \
                out_dword(HWIO_LPASS_LPAIF_RDDMA_SET_BUFF_CNTa_ADDR(base,a),val)
#define HWIO_LPASS_LPAIF_RDDMA_SET_BUFF_CNTa_CNT_BMSK                                  0xfffff
#define HWIO_LPASS_LPAIF_RDDMA_SET_BUFF_CNTa_CNT_SHFT                                        0

#define HWIO_LPASS_LPAIF_RDDMA_SET_PER_CNTa_ADDR(base,a)                            ((base) + 0XC02C + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_RDDMA_SET_PER_CNTa_OFFS(a)                                 (0XC02C + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_RDDMA_SET_PER_CNTa_RMSK                                       0xfffff
#define HWIO_LPASS_LPAIF_RDDMA_SET_PER_CNTa_MAXa                                             2
#define HWIO_LPASS_LPAIF_RDDMA_SET_PER_CNTa_OUTI(base,a,val)        \
                out_dword(HWIO_LPASS_LPAIF_RDDMA_SET_PER_CNTa_ADDR(base,a),val)
#define HWIO_LPASS_LPAIF_RDDMA_SET_PER_CNTa_CNT_BMSK                                   0xfffff
#define HWIO_LPASS_LPAIF_RDDMA_SET_PER_CNTa_CNT_SHFT                                         0

#define HWIO_LPASS_LPAIF_RDDMA_STC_LSBa_ADDR(base,a)                                ((base) + 0XC030 + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_RDDMA_STC_LSBa_OFFS(a)                                     (0XC030 + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_RDDMA_STC_LSBa_RMSK                                        0xffffffff
#define HWIO_LPASS_LPAIF_RDDMA_STC_LSBa_MAXa                                                 2
#define HWIO_LPASS_LPAIF_RDDMA_STC_LSBa_INI(base,a)                \
                in_dword_masked(HWIO_LPASS_LPAIF_RDDMA_STC_LSBa_ADDR(base,a), HWIO_LPASS_LPAIF_RDDMA_STC_LSBa_RMSK)
#define HWIO_LPASS_LPAIF_RDDMA_STC_LSBa_INMI(base,a,mask)        \
                in_dword_masked(HWIO_LPASS_LPAIF_RDDMA_STC_LSBa_ADDR(base,a), mask)
#define HWIO_LPASS_LPAIF_RDDMA_STC_LSBa_STC_LSB_BMSK                                0xffffffff
#define HWIO_LPASS_LPAIF_RDDMA_STC_LSBa_STC_LSB_SHFT                                         0

#define HWIO_LPASS_LPAIF_RDDMA_STC_MSBa_ADDR(base,a)                                ((base) + 0XC034 + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_RDDMA_STC_MSBa_OFFS(a)                                     (0XC034 + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_RDDMA_STC_MSBa_RMSK                                          0xffffff
#define HWIO_LPASS_LPAIF_RDDMA_STC_MSBa_MAXa                                                 2
#define HWIO_LPASS_LPAIF_RDDMA_STC_MSBa_INI(base,a)                \
                in_dword_masked(HWIO_LPASS_LPAIF_RDDMA_STC_MSBa_ADDR(base,a), HWIO_LPASS_LPAIF_RDDMA_STC_MSBa_RMSK)
#define HWIO_LPASS_LPAIF_RDDMA_STC_MSBa_INMI(base,a,mask)        \
                in_dword_masked(HWIO_LPASS_LPAIF_RDDMA_STC_MSBa_ADDR(base,a), mask)
#define HWIO_LPASS_LPAIF_RDDMA_STC_MSBa_STC_MSB_BMSK                                  0xffffff
#define HWIO_LPASS_LPAIF_RDDMA_STC_MSBa_STC_MSB_SHFT                                         0

#define HWIO_LPASS_LPAIF_RDDMA_PERa_ADDR(base,a)                                    ((base) + 0XC038 + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_RDDMA_PERa_OFFS(a)                                         (0XC038 + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_RDDMA_PERa_RMSK                                               0x1003f
#define HWIO_LPASS_LPAIF_RDDMA_PERa_MAXa                                                     2
#define HWIO_LPASS_LPAIF_RDDMA_PERa_INI(base,a)                \
                in_dword_masked(HWIO_LPASS_LPAIF_RDDMA_PERa_ADDR(base,a), HWIO_LPASS_LPAIF_RDDMA_PERa_RMSK)
#define HWIO_LPASS_LPAIF_RDDMA_PERa_INMI(base,a,mask)        \
                in_dword_masked(HWIO_LPASS_LPAIF_RDDMA_PERa_ADDR(base,a), mask)
#define HWIO_LPASS_LPAIF_RDDMA_PERa_PERIOD_UPDATE_BMSK                                 0x10000
#define HWIO_LPASS_LPAIF_RDDMA_PERa_PERIOD_UPDATE_SHFT                                      16
#define HWIO_LPASS_LPAIF_RDDMA_PERa_PERIOD_FIFO_BMSK                                      0x3f
#define HWIO_LPASS_LPAIF_RDDMA_PERa_PERIOD_FIFO_SHFT                                         0
#define HWIO_LPASS_LPAIF_RDDMA_PERa_PERIOD_FIFO_ENUM_0_FVAL                                0x0
#define HWIO_LPASS_LPAIF_RDDMA_PERa_PERIOD_FIFO_ENUM_1_FVAL                                0x1
#define HWIO_LPASS_LPAIF_RDDMA_PERa_PERIOD_FIFO_ENUM_2_FVAL                                0x2
#define HWIO_LPASS_LPAIF_RDDMA_PERa_PERIOD_FIFO_ENUM_3_FVAL                                0x3
#define HWIO_LPASS_LPAIF_RDDMA_PERa_PERIOD_FIFO_ENUM_4_FVAL                                0x4
#define HWIO_LPASS_LPAIF_RDDMA_PERa_PERIOD_FIFO_ENUM_5_FVAL                                0x5
#define HWIO_LPASS_LPAIF_RDDMA_PERa_PERIOD_FIFO_ENUM_6_FVAL                                0x6
#define HWIO_LPASS_LPAIF_RDDMA_PERa_PERIOD_FIFO_ENUM_7_FVAL                                0x7
#define HWIO_LPASS_LPAIF_RDDMA_PERa_PERIOD_FIFO_ENUM_8_FVAL                                0x8
#define HWIO_LPASS_LPAIF_RDDMA_PERa_PERIOD_FIFO_ENUM_9_FVAL                                0x9
#define HWIO_LPASS_LPAIF_RDDMA_PERa_PERIOD_FIFO_ENUM_10_FVAL                               0xa
#define HWIO_LPASS_LPAIF_RDDMA_PERa_PERIOD_FIFO_ENUM_11_FVAL                               0xb
#define HWIO_LPASS_LPAIF_RDDMA_PERa_PERIOD_FIFO_ENUM_12_FVAL                               0xc
#define HWIO_LPASS_LPAIF_RDDMA_PERa_PERIOD_FIFO_ENUM_13_FVAL                               0xd
#define HWIO_LPASS_LPAIF_RDDMA_PERa_PERIOD_FIFO_ENUM_14_FVAL                               0xe
#define HWIO_LPASS_LPAIF_RDDMA_PERa_PERIOD_FIFO_ENUM_15_FVAL                               0xf
#define HWIO_LPASS_LPAIF_RDDMA_PERa_PERIOD_FIFO_ENUM_16_FVAL                              0x10
#define HWIO_LPASS_LPAIF_RDDMA_PERa_PERIOD_FIFO_ENUM_17_FVAL                              0x11
#define HWIO_LPASS_LPAIF_RDDMA_PERa_PERIOD_FIFO_ENUM_18_FVAL                              0x12
#define HWIO_LPASS_LPAIF_RDDMA_PERa_PERIOD_FIFO_ENUM_19_FVAL                              0x13
#define HWIO_LPASS_LPAIF_RDDMA_PERa_PERIOD_FIFO_ENUM_20_FVAL                              0x14
#define HWIO_LPASS_LPAIF_RDDMA_PERa_PERIOD_FIFO_ENUM_21_FVAL                              0x15
#define HWIO_LPASS_LPAIF_RDDMA_PERa_PERIOD_FIFO_ENUM_22_FVAL                              0x16
#define HWIO_LPASS_LPAIF_RDDMA_PERa_PERIOD_FIFO_ENUM_23_FVAL                              0x17
#define HWIO_LPASS_LPAIF_RDDMA_PERa_PERIOD_FIFO_ENUM_24_FVAL                              0x18
#define HWIO_LPASS_LPAIF_RDDMA_PERa_PERIOD_FIFO_ENUM_25_FVAL                              0x19
#define HWIO_LPASS_LPAIF_RDDMA_PERa_PERIOD_FIFO_ENUM_26_FVAL                              0x1a
#define HWIO_LPASS_LPAIF_RDDMA_PERa_PERIOD_FIFO_ENUM_27_FVAL                              0x1b
#define HWIO_LPASS_LPAIF_RDDMA_PERa_PERIOD_FIFO_ENUM_28_FVAL                              0x1c
#define HWIO_LPASS_LPAIF_RDDMA_PERa_PERIOD_FIFO_ENUM_29_FVAL                              0x1d
#define HWIO_LPASS_LPAIF_RDDMA_PERa_PERIOD_FIFO_ENUM_30_FVAL                              0x1e
#define HWIO_LPASS_LPAIF_RDDMA_PERa_PERIOD_FIFO_ENUM_31_FVAL                              0x1f
#define HWIO_LPASS_LPAIF_RDDMA_PERa_PERIOD_FIFO_ENUM_32_FVAL                              0x20

#define HWIO_LPASS_LPAIF_RDDMA_PERCLRa_ADDR(base,a)                                 ((base) + 0XC03C + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_RDDMA_PERCLRa_OFFS(a)                                      (0XC03C + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_RDDMA_PERCLRa_RMSK                                                0x1
#define HWIO_LPASS_LPAIF_RDDMA_PERCLRa_MAXa                                                  2
#define HWIO_LPASS_LPAIF_RDDMA_PERCLRa_OUTI(base,a,val)        \
                out_dword(HWIO_LPASS_LPAIF_RDDMA_PERCLRa_ADDR(base,a),val)
#define HWIO_LPASS_LPAIF_RDDMA_PERCLRa_PERIOD_CLR_BMSK                                     0x1
#define HWIO_LPASS_LPAIF_RDDMA_PERCLRa_PERIOD_CLR_SHFT                                       0

#define HWIO_LPASS_LPAIF_RDDMA_BASE_EXTa_ADDR(base,a)                               ((base) + 0XC040 + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_RDDMA_BASE_EXTa_OFFS(a)                                    (0XC040 + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_RDDMA_BASE_EXTa_RMSK                                              0xf
#define HWIO_LPASS_LPAIF_RDDMA_BASE_EXTa_MAXa                                                2
#define HWIO_LPASS_LPAIF_RDDMA_BASE_EXTa_INI(base,a)                \
                in_dword_masked(HWIO_LPASS_LPAIF_RDDMA_BASE_EXTa_ADDR(base,a), HWIO_LPASS_LPAIF_RDDMA_BASE_EXTa_RMSK)
#define HWIO_LPASS_LPAIF_RDDMA_BASE_EXTa_INMI(base,a,mask)        \
                in_dword_masked(HWIO_LPASS_LPAIF_RDDMA_BASE_EXTa_ADDR(base,a), mask)
#define HWIO_LPASS_LPAIF_RDDMA_BASE_EXTa_OUTI(base,a,val)        \
                out_dword(HWIO_LPASS_LPAIF_RDDMA_BASE_EXTa_ADDR(base,a),val)
#define HWIO_LPASS_LPAIF_RDDMA_BASE_EXTa_OUTMI(base,a,mask,val) \
                out_dword_masked_ns(HWIO_LPASS_LPAIF_RDDMA_BASE_EXTa_ADDR(base,a),mask,val,HWIO_LPASS_LPAIF_RDDMA_BASE_EXTa_INI(base,a))
#define HWIO_LPASS_LPAIF_RDDMA_BASE_EXTa_BASE_ADDR_BMSK                                    0xf
#define HWIO_LPASS_LPAIF_RDDMA_BASE_EXTa_BASE_ADDR_SHFT                                      0

#define HWIO_LPASS_LPAIF_RDDMA_CURR_ADDR_EXTa_ADDR(base,a)                          ((base) + 0XC044 + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_RDDMA_CURR_ADDR_EXTa_OFFS(a)                               (0XC044 + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_RDDMA_CURR_ADDR_EXTa_RMSK                                         0xf
#define HWIO_LPASS_LPAIF_RDDMA_CURR_ADDR_EXTa_MAXa                                           2
#define HWIO_LPASS_LPAIF_RDDMA_CURR_ADDR_EXTa_INI(base,a)                \
                in_dword_masked(HWIO_LPASS_LPAIF_RDDMA_CURR_ADDR_EXTa_ADDR(base,a), HWIO_LPASS_LPAIF_RDDMA_CURR_ADDR_EXTa_RMSK)
#define HWIO_LPASS_LPAIF_RDDMA_CURR_ADDR_EXTa_INMI(base,a,mask)        \
                in_dword_masked(HWIO_LPASS_LPAIF_RDDMA_CURR_ADDR_EXTa_ADDR(base,a), mask)
#define HWIO_LPASS_LPAIF_RDDMA_CURR_ADDR_EXTa_ADDR_BMSK                                    0xf
#define HWIO_LPASS_LPAIF_RDDMA_CURR_ADDR_EXTa_ADDR_SHFT                                      0

#define HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR0_ADDR(x)                              ((x) + 0xc048)
#define HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR0_OFFS                                 (0xc048)
#define HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR0_RMSK                                      0x7ff
#define HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR0_IN(x)            \
                in_dword(HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR0_ADDR(x))
#define HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR0_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR0_ADDR(x), m)
#define HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR0_OUT(x, v)            \
                out_dword(HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR0_ADDR(x),v)
#define HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR0_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR0_ADDR(x),m,v,HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR0_IN(x))
#define HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR0_START_ADDR_BMSK                           0x7ff
#define HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR0_START_ADDR_SHFT                               0

#define HWIO_LPASS_LPAIF_RDDMA_RAM_LENGTHa_ADDR(base,a)                             ((base) + 0XC04C + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_RDDMA_RAM_LENGTHa_OFFS(a)                                  (0XC04C + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_RDDMA_RAM_LENGTHa_RMSK                                          0x7ff
#define HWIO_LPASS_LPAIF_RDDMA_RAM_LENGTHa_MAXa                                              2
#define HWIO_LPASS_LPAIF_RDDMA_RAM_LENGTHa_INI(base,a)                \
                in_dword_masked(HWIO_LPASS_LPAIF_RDDMA_RAM_LENGTHa_ADDR(base,a), HWIO_LPASS_LPAIF_RDDMA_RAM_LENGTHa_RMSK)
#define HWIO_LPASS_LPAIF_RDDMA_RAM_LENGTHa_INMI(base,a,mask)        \
                in_dword_masked(HWIO_LPASS_LPAIF_RDDMA_RAM_LENGTHa_ADDR(base,a), mask)
#define HWIO_LPASS_LPAIF_RDDMA_RAM_LENGTHa_OUTI(base,a,val)        \
                out_dword(HWIO_LPASS_LPAIF_RDDMA_RAM_LENGTHa_ADDR(base,a),val)
#define HWIO_LPASS_LPAIF_RDDMA_RAM_LENGTHa_OUTMI(base,a,mask,val) \
                out_dword_masked_ns(HWIO_LPASS_LPAIF_RDDMA_RAM_LENGTHa_ADDR(base,a),mask,val,HWIO_LPASS_LPAIF_RDDMA_RAM_LENGTHa_INI(base,a))
#define HWIO_LPASS_LPAIF_RDDMA_RAM_LENGTHa_LENGTH_BMSK                                   0x7ff
#define HWIO_LPASS_LPAIF_RDDMA_RAM_LENGTHa_LENGTH_SHFT                                       0

#define HWIO_LPASS_LPAIF_RDDMA_CODEC_INTFa_ADDR(base,a)                             ((base) + 0XC050 + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_RDDMA_CODEC_INTFa_OFFS(a)                                  (0XC050 + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_RDDMA_CODEC_INTFa_RMSK                                     0xefef00ff
#define HWIO_LPASS_LPAIF_RDDMA_CODEC_INTFa_MAXa                                              2
#define HWIO_LPASS_LPAIF_RDDMA_CODEC_INTFa_INI(base,a)                \
                in_dword_masked(HWIO_LPASS_LPAIF_RDDMA_CODEC_INTFa_ADDR(base,a), HWIO_LPASS_LPAIF_RDDMA_CODEC_INTFa_RMSK)
#define HWIO_LPASS_LPAIF_RDDMA_CODEC_INTFa_INMI(base,a,mask)        \
                in_dword_masked(HWIO_LPASS_LPAIF_RDDMA_CODEC_INTFa_ADDR(base,a), mask)
#define HWIO_LPASS_LPAIF_RDDMA_CODEC_INTFa_OUTI(base,a,val)        \
                out_dword(HWIO_LPASS_LPAIF_RDDMA_CODEC_INTFa_ADDR(base,a),val)
#define HWIO_LPASS_LPAIF_RDDMA_CODEC_INTFa_OUTMI(base,a,mask,val) \
                out_dword_masked_ns(HWIO_LPASS_LPAIF_RDDMA_CODEC_INTFa_ADDR(base,a),mask,val,HWIO_LPASS_LPAIF_RDDMA_CODEC_INTFa_INI(base,a))
#define HWIO_LPASS_LPAIF_RDDMA_CODEC_INTFa_RESET_BMSK                               0x80000000
#define HWIO_LPASS_LPAIF_RDDMA_CODEC_INTFa_RESET_SHFT                                       31
#define HWIO_LPASS_LPAIF_RDDMA_CODEC_INTFa_ENABLE_BMSK                              0x40000000
#define HWIO_LPASS_LPAIF_RDDMA_CODEC_INTFa_ENABLE_SHFT                                      30
#define HWIO_LPASS_LPAIF_RDDMA_CODEC_INTFa_ENABLE_16BIT_PACKING_BMSK                0x20000000
#define HWIO_LPASS_LPAIF_RDDMA_CODEC_INTFa_ENABLE_16BIT_PACKING_SHFT                        29
#define HWIO_LPASS_LPAIF_RDDMA_CODEC_INTFa_ENABLE_16BIT_PACKING_DISABLE_FVAL               0x0
#define HWIO_LPASS_LPAIF_RDDMA_CODEC_INTFa_ENABLE_16BIT_PACKING_ENABLE_FVAL                0x1
#define HWIO_LPASS_LPAIF_RDDMA_CODEC_INTFa_CODEC_FS_SEL_BMSK                         0xe000000
#define HWIO_LPASS_LPAIF_RDDMA_CODEC_INTFa_CODEC_FS_SEL_SHFT                                25
#define HWIO_LPASS_LPAIF_RDDMA_CODEC_INTFa_CODEC_FS_DELAY_BMSK                       0x1e00000
#define HWIO_LPASS_LPAIF_RDDMA_CODEC_INTFa_CODEC_FS_DELAY_SHFT                              21
#define HWIO_LPASS_LPAIF_RDDMA_CODEC_INTFa_CODEC_INTF_BMSK                             0xf0000
#define HWIO_LPASS_LPAIF_RDDMA_CODEC_INTFa_CODEC_INTF_SHFT                                  16
#define HWIO_LPASS_LPAIF_RDDMA_CODEC_INTFa_CODEC_INTF_NONE_FVAL                            0x0
#define HWIO_LPASS_LPAIF_RDDMA_CODEC_INTFa_CODEC_INTF_CODEC_RX0_SRC_FVAL                   0x1
#define HWIO_LPASS_LPAIF_RDDMA_CODEC_INTFa_CODEC_INTF_CODEC_RX1_SRC_FVAL                   0x2
#define HWIO_LPASS_LPAIF_RDDMA_CODEC_INTFa_CODEC_INTF_CODEC_RX2_SRC_FVAL                   0x3
#define HWIO_LPASS_LPAIF_RDDMA_CODEC_INTFa_CODEC_INTF_CODEC_RX3_SRC_FVAL                   0x4
#define HWIO_LPASS_LPAIF_RDDMA_CODEC_INTFa_CODEC_INTF_CODEC_RX4_SRC_FVAL                   0x5
#define HWIO_LPASS_LPAIF_RDDMA_CODEC_INTFa_CODEC_INTF_CODEC_RX5_SRC_FVAL                   0x6
#define HWIO_LPASS_LPAIF_RDDMA_CODEC_INTFa_CODEC_INTF_CODEC_RX6_SRC_FVAL                   0x7
#define HWIO_LPASS_LPAIF_RDDMA_CODEC_INTFa_CODEC_INTF_CODEC_RX7_SRC_FVAL                   0x8
#define HWIO_LPASS_LPAIF_RDDMA_CODEC_INTFa_CODEC_INTF_CODEC_RX8_SRC_FVAL                   0x9
#define HWIO_LPASS_LPAIF_RDDMA_CODEC_INTFa_CODEC_INTF_CODEC_RX9_SRC_FVAL                   0xa
#define HWIO_LPASS_LPAIF_RDDMA_CODEC_INTFa_NUM_ACTIVE_CHANNEL_BMSK                        0xff
#define HWIO_LPASS_LPAIF_RDDMA_CODEC_INTFa_NUM_ACTIVE_CHANNEL_SHFT                           0

#define HWIO_LPASS_LPAIF_RDDMA_AHB_BYPASSa_ADDR(base,a)                             ((base) + 0XC054 + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_RDDMA_AHB_BYPASSa_OFFS(a)                                  (0XC054 + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_RDDMA_AHB_BYPASSa_RMSK                                          0x10f
#define HWIO_LPASS_LPAIF_RDDMA_AHB_BYPASSa_MAXa                                              2
#define HWIO_LPASS_LPAIF_RDDMA_AHB_BYPASSa_INI(base,a)                \
                in_dword_masked(HWIO_LPASS_LPAIF_RDDMA_AHB_BYPASSa_ADDR(base,a), HWIO_LPASS_LPAIF_RDDMA_AHB_BYPASSa_RMSK)
#define HWIO_LPASS_LPAIF_RDDMA_AHB_BYPASSa_INMI(base,a,mask)        \
                in_dword_masked(HWIO_LPASS_LPAIF_RDDMA_AHB_BYPASSa_ADDR(base,a), mask)
#define HWIO_LPASS_LPAIF_RDDMA_AHB_BYPASSa_OUTI(base,a,val)        \
                out_dword(HWIO_LPASS_LPAIF_RDDMA_AHB_BYPASSa_ADDR(base,a),val)
#define HWIO_LPASS_LPAIF_RDDMA_AHB_BYPASSa_OUTMI(base,a,mask,val) \
                out_dword_masked_ns(HWIO_LPASS_LPAIF_RDDMA_AHB_BYPASSa_ADDR(base,a),mask,val,HWIO_LPASS_LPAIF_RDDMA_AHB_BYPASSa_INI(base,a))
#define HWIO_LPASS_LPAIF_RDDMA_AHB_BYPASSa_BYPASS_ENABLE_BMSK                            0x100
#define HWIO_LPASS_LPAIF_RDDMA_AHB_BYPASSa_BYPASS_ENABLE_SHFT                                8
#define HWIO_LPASS_LPAIF_RDDMA_AHB_BYPASSa_BYPASS_SEL_BMSK                                 0xf
#define HWIO_LPASS_LPAIF_RDDMA_AHB_BYPASSa_BYPASS_SEL_SHFT                                   0
#define HWIO_LPASS_LPAIF_RDDMA_AHB_BYPASSa_BYPASS_SEL_NONE_FVAL                            0x0
#define HWIO_LPASS_LPAIF_RDDMA_AHB_BYPASSa_BYPASS_SEL_CODEC_TX0_SRC_FVAL                   0x1
#define HWIO_LPASS_LPAIF_RDDMA_AHB_BYPASSa_BYPASS_SEL_CODEC_TX1_SRC_FVAL                   0x2
#define HWIO_LPASS_LPAIF_RDDMA_AHB_BYPASSa_BYPASS_SEL_CODEC_TX2_SRC_FVAL                   0x3
#define HWIO_LPASS_LPAIF_RDDMA_AHB_BYPASSa_BYPASS_SEL_CODEC_TX3_SRC_FVAL                   0x4
#define HWIO_LPASS_LPAIF_RDDMA_AHB_BYPASSa_BYPASS_SEL_CODEC_TX4_SRC_FVAL                   0x5
#define HWIO_LPASS_LPAIF_RDDMA_AHB_BYPASSa_BYPASS_SEL_CODEC_TX5_SRC_FVAL                   0x6
#define HWIO_LPASS_LPAIF_RDDMA_AHB_BYPASSa_BYPASS_SEL_CODEC_TX6_SRC_FVAL                   0x7
#define HWIO_LPASS_LPAIF_RDDMA_AHB_BYPASSa_BYPASS_SEL_CODEC_TX7_SRC_FVAL                   0x8
#define HWIO_LPASS_LPAIF_RDDMA_AHB_BYPASSa_BYPASS_SEL_CODEC_TX8_SRC_FVAL                   0x9

#define HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR1_ADDR(x)                              ((x) + 0xd048)
#define HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR1_OFFS                                 (0xd048)
#define HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR1_RMSK                                      0x7ff
#define HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR1_IN(x)            \
                in_dword(HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR1_ADDR(x))
#define HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR1_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR1_ADDR(x), m)
#define HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR1_OUT(x, v)            \
                out_dword(HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR1_ADDR(x),v)
#define HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR1_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR1_ADDR(x),m,v,HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR1_IN(x))
#define HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR1_START_ADDR_BMSK                           0x7ff
#define HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR1_START_ADDR_SHFT                               0

#define HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR2_ADDR(x)                              ((x) + 0xe048)
#define HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR2_OFFS                                 (0xe048)
#define HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR2_RMSK                                      0x7ff
#define HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR2_IN(x)            \
                in_dword(HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR2_ADDR(x))
#define HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR2_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR2_ADDR(x), m)
#define HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR2_OUT(x, v)            \
                out_dword(HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR2_ADDR(x),v)
#define HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR2_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR2_ADDR(x),m,v,HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR2_IN(x))
#define HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR2_START_ADDR_BMSK                           0x7ff
#define HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR2_START_ADDR_SHFT                               0

#define HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR3_ADDR(x)                              ((x) + 0xf048)
#define HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR3_OFFS                                 (0xf048)
#define HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR3_RMSK                                      0x7ff
#define HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR3_IN(x)            \
                in_dword(HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR3_ADDR(x))
#define HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR3_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR3_ADDR(x), m)
#define HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR3_OUT(x, v)            \
                out_dword(HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR3_ADDR(x),v)
#define HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR3_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR3_ADDR(x),m,v,HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR3_IN(x))
#define HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR3_START_ADDR_BMSK                           0x7ff
#define HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR3_START_ADDR_SHFT                               0

#define HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR4_ADDR(x)                              ((x) + 0x10048)
#define HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR4_OFFS                                 (0x10048)
#define HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR4_RMSK                                      0x7ff
#define HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR4_IN(x)            \
                in_dword(HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR4_ADDR(x))
#define HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR4_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR4_ADDR(x), m)
#define HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR4_OUT(x, v)            \
                out_dword(HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR4_ADDR(x),v)
#define HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR4_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR4_ADDR(x),m,v,HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR4_IN(x))
#define HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR4_START_ADDR_BMSK                           0x7ff
#define HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR4_START_ADDR_SHFT                               0

#define HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR5_ADDR(x)                              ((x) + 0x11048)
#define HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR5_OFFS                                 (0x11048)
#define HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR5_RMSK                                      0x7ff
#define HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR5_IN(x)            \
                in_dword(HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR5_ADDR(x))
#define HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR5_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR5_ADDR(x), m)
#define HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR5_OUT(x, v)            \
                out_dword(HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR5_ADDR(x),v)
#define HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR5_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR5_ADDR(x),m,v,HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR5_IN(x))
#define HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR5_START_ADDR_BMSK                           0x7ff
#define HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR5_START_ADDR_SHFT                               0

#define HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR6_ADDR(x)                              ((x) + 0x12048)
#define HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR6_OFFS                                 (0x12048)
#define HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR6_RMSK                                      0x7ff
#define HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR6_IN(x)            \
                in_dword(HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR6_ADDR(x))
#define HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR6_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR6_ADDR(x), m)
#define HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR6_OUT(x, v)            \
                out_dword(HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR6_ADDR(x),v)
#define HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR6_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR6_ADDR(x),m,v,HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR6_IN(x))
#define HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR6_START_ADDR_BMSK                           0x7ff
#define HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR6_START_ADDR_SHFT                               0

#define HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR7_ADDR(x)                              ((x) + 0x13048)
#define HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR7_OFFS                                 (0x13048)
#define HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR7_RMSK                                      0x7ff
#define HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR7_IN(x)            \
                in_dword(HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR7_ADDR(x))
#define HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR7_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR7_ADDR(x), m)
#define HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR7_OUT(x, v)            \
                out_dword(HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR7_ADDR(x),v)
#define HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR7_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR7_ADDR(x),m,v,HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR7_IN(x))
#define HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR7_START_ADDR_BMSK                           0x7ff
#define HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR7_START_ADDR_SHFT                               0

#define HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR8_ADDR(x)                              ((x) + 0x14048)
#define HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR8_OFFS                                 (0x14048)
#define HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR8_RMSK                                      0x7ff
#define HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR8_IN(x)            \
                in_dword(HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR8_ADDR(x))
#define HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR8_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR8_ADDR(x), m)
#define HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR8_OUT(x, v)            \
                out_dword(HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR8_ADDR(x),v)
#define HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR8_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR8_ADDR(x),m,v,HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR8_IN(x))
#define HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR8_START_ADDR_BMSK                           0x7ff
#define HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR8_START_ADDR_SHFT                               0

#define HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR9_ADDR(x)                              ((x) + 0x15048)
#define HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR9_OFFS                                 (0x15048)
#define HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR9_RMSK                                      0x7ff
#define HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR9_IN(x)            \
                in_dword(HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR9_ADDR(x))
#define HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR9_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR9_ADDR(x), m)
#define HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR9_OUT(x, v)            \
                out_dword(HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR9_ADDR(x),v)
#define HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR9_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR9_ADDR(x),m,v,HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR9_IN(x))
#define HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR9_START_ADDR_BMSK                           0x7ff
#define HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR9_START_ADDR_SHFT                               0

#define HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR10_ADDR(x)                             ((x) + 0x16048)
#define HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR10_OFFS                                (0x16048)
#define HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR10_RMSK                                     0x7ff
#define HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR10_IN(x)            \
                in_dword(HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR10_ADDR(x))
#define HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR10_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR10_ADDR(x), m)
#define HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR10_OUT(x, v)            \
                out_dword(HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR10_ADDR(x),v)
#define HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR10_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR10_ADDR(x),m,v,HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR10_IN(x))
#define HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR10_START_ADDR_BMSK                          0x7ff
#define HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR10_START_ADDR_SHFT                              0

#define HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR11_ADDR(x)                             ((x) + 0x17048)
#define HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR11_OFFS                                (0x17048)
#define HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR11_RMSK                                     0x7ff
#define HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR11_IN(x)            \
                in_dword(HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR11_ADDR(x))
#define HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR11_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR11_ADDR(x), m)
#define HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR11_OUT(x, v)            \
                out_dword(HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR11_ADDR(x),v)
#define HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR11_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR11_ADDR(x),m,v,HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR11_IN(x))
#define HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR11_START_ADDR_BMSK                          0x7ff
#define HWIO_LPASS_LPAIF_RDDMA_RAM_START_ADDR11_START_ADDR_SHFT                              0

#define HWIO_LPASS_LPAIF_WRDMA_CTLa_ADDR(base,a)                                    ((base) + 0X18000 + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_WRDMA_CTLa_OFFS(a)                                         (0X18000 + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_WRDMA_CTLa_POR							0x00400000
#define HWIO_LPASS_LPAIF_WRDMA_CTLa_RMSK                                            0x81fff03f
#define HWIO_LPASS_LPAIF_WRDMA_CTLa_MAXa                                                     2
#define HWIO_LPASS_LPAIF_WRDMA_CTLa_INI(base,a)                \
                in_dword_masked(HWIO_LPASS_LPAIF_WRDMA_CTLa_ADDR(base,a), HWIO_LPASS_LPAIF_WRDMA_CTLa_RMSK)
#define HWIO_LPASS_LPAIF_WRDMA_CTLa_INMI(base,a,mask)        \
                in_dword_masked(HWIO_LPASS_LPAIF_WRDMA_CTLa_ADDR(base,a), mask)
#define HWIO_LPASS_LPAIF_WRDMA_CTLa_OUTI(base,a,val)        \
                out_dword(HWIO_LPASS_LPAIF_WRDMA_CTLa_ADDR(base,a),val)
#define HWIO_LPASS_LPAIF_WRDMA_CTLa_OUTMI(base,a,mask,val) \
                out_dword_masked_ns(HWIO_LPASS_LPAIF_WRDMA_CTLa_ADDR(base,a),mask,val,HWIO_LPASS_LPAIF_WRDMA_CTLa_INI(base,a))
#define HWIO_LPASS_LPAIF_WRDMA_CTLa_RESET_BMSK                                      0x80000000
#define HWIO_LPASS_LPAIF_WRDMA_CTLa_RESET_SHFT                                              31
#define HWIO_LPASS_LPAIF_WRDMA_CTLa_RESET_DISABLE_FVAL                                     0x0
#define HWIO_LPASS_LPAIF_WRDMA_CTLa_RESET_ENABLE_FVAL                                      0x1
#define HWIO_LPASS_LPAIF_WRDMA_CTLa_BURST16_EN_BMSK                                  0x1000000
#define HWIO_LPASS_LPAIF_WRDMA_CTLa_BURST16_EN_SHFT                                         24
#define HWIO_LPASS_LPAIF_WRDMA_CTLa_BURST16_EN_DISABLE_FVAL                                0x0
#define HWIO_LPASS_LPAIF_WRDMA_CTLa_BURST16_EN_ENABLE_FVAL                                 0x1
#define HWIO_LPASS_LPAIF_WRDMA_CTLa_BURST8_EN_BMSK                                    0x800000
#define HWIO_LPASS_LPAIF_WRDMA_CTLa_BURST8_EN_SHFT                                          23
#define HWIO_LPASS_LPAIF_WRDMA_CTLa_BURST8_EN_DISABLE_FVAL                                 0x0
#define HWIO_LPASS_LPAIF_WRDMA_CTLa_BURST8_EN_ENABLE_FVAL                                  0x1
#define HWIO_LPASS_LPAIF_WRDMA_CTLa_DYNAMIC_CLOCK_BMSK                                0x400000
#define HWIO_LPASS_LPAIF_WRDMA_CTLa_DYNAMIC_CLOCK_SHFT                                      22
#define HWIO_LPASS_LPAIF_WRDMA_CTLa_DYNAMIC_CLOCK_OFF_FVAL                                 0x0
#define HWIO_LPASS_LPAIF_WRDMA_CTLa_DYNAMIC_CLOCK_ON_FVAL                                  0x1
#define HWIO_LPASS_LPAIF_WRDMA_CTLa_BURST_EN_BMSK                                     0x200000
#define HWIO_LPASS_LPAIF_WRDMA_CTLa_BURST_EN_SHFT                                           21
#define HWIO_LPASS_LPAIF_WRDMA_CTLa_BURST_EN_SINGLE_FVAL                                   0x0
#define HWIO_LPASS_LPAIF_WRDMA_CTLa_BURST_EN_INCR4_FVAL                                    0x1
#define HWIO_LPASS_LPAIF_WRDMA_CTLa_WPSCNT_BMSK                                       0x1e0000
#define HWIO_LPASS_LPAIF_WRDMA_CTLa_WPSCNT_SHFT                                             17
#define HWIO_LPASS_LPAIF_WRDMA_CTLa_WPSCNT_ONE_FVAL                                        0x0
#define HWIO_LPASS_LPAIF_WRDMA_CTLa_WPSCNT_TWO_FVAL                                        0x1
#define HWIO_LPASS_LPAIF_WRDMA_CTLa_WPSCNT_THREE_FVAL                                      0x2
#define HWIO_LPASS_LPAIF_WRDMA_CTLa_WPSCNT_FOUR_FVAL                                       0x3
#define HWIO_LPASS_LPAIF_WRDMA_CTLa_WPSCNT_SIX_FVAL                                        0x5
#define HWIO_LPASS_LPAIF_WRDMA_CTLa_WPSCNT_EIGHT_FVAL                                      0x7
#define HWIO_LPASS_LPAIF_WRDMA_CTLa_WPSCNT_TEN_FVAL                                        0x9
#define HWIO_LPASS_LPAIF_WRDMA_CTLa_WPSCNT_TWELVE_FVAL                                     0xb
#define HWIO_LPASS_LPAIF_WRDMA_CTLa_WPSCNT_FOURTEEN_FVAL                                   0xd
#define HWIO_LPASS_LPAIF_WRDMA_CTLa_WPSCNT_SIXTEEN_FVAL                                    0xf
#define HWIO_LPASS_LPAIF_WRDMA_CTLa_AUDIO_INTF_BMSK                                    0x1f000
#define HWIO_LPASS_LPAIF_WRDMA_CTLa_AUDIO_INTF_SHFT                                         12
#define HWIO_LPASS_LPAIF_WRDMA_CTLa_AUDIO_INTF_NONE_FVAL                                   0x0
#define HWIO_LPASS_LPAIF_WRDMA_CTLa_AUDIO_INTF_PRI_SRC_FVAL                                0x1
#define HWIO_LPASS_LPAIF_WRDMA_CTLa_AUDIO_INTF_SEC_SRC_FVAL                                0x2
#define HWIO_LPASS_LPAIF_WRDMA_CTLa_AUDIO_INTF_TER_SRC_FVAL                                0x3
#define HWIO_LPASS_LPAIF_WRDMA_CTLa_AUDIO_INTF_QUA_SRC_FVAL                                0x4
#define HWIO_LPASS_LPAIF_WRDMA_CTLa_AUDIO_INTF_QUI_SRC_FVAL                                0x5
#define HWIO_LPASS_LPAIF_WRDMA_CTLa_AUDIO_INTF_MIXOUT_FVAL                                 0x6
#define HWIO_LPASS_LPAIF_WRDMA_CTLa_AUDIO_INTF_LOOPBACK_CH0_FVAL                           0x9
#define HWIO_LPASS_LPAIF_WRDMA_CTLa_AUDIO_INTF_LOOPBACK_CH1_FVAL                           0xa
#define HWIO_LPASS_LPAIF_WRDMA_CTLa_AUDIO_INTF_LOOPBACK_CH2_FVAL                           0xb
#define HWIO_LPASS_LPAIF_WRDMA_CTLa_AUDIO_INTF_LOOPBACK_CH3_FVAL                           0xc
#define HWIO_LPASS_LPAIF_WRDMA_CTLa_AUDIO_INTF_LOOPBACK_CH4_FVAL                           0xd
#define HWIO_LPASS_LPAIF_WRDMA_CTLa_AUDIO_INTF_LOOPBACK_CH5_FVAL                           0xe
#define HWIO_LPASS_LPAIF_WRDMA_CTLa_AUDIO_INTF_LOOPBACK_CH6_FVAL                          0x10
#define HWIO_LPASS_LPAIF_WRDMA_CTLa_AUDIO_INTF_LOOPBACK_CH7_FVAL                          0x11
#define HWIO_LPASS_LPAIF_WRDMA_CTLa_AUDIO_INTF_LOOPBACK_CH8_FVAL                          0x12
#define HWIO_LPASS_LPAIF_WRDMA_CTLa_AUDIO_INTF_LOOPBACK_CH9_FVAL                          0x13
#define HWIO_LPASS_LPAIF_WRDMA_CTLa_AUDIO_INTF_LOOPBACK_CH10_FVAL                         0x14
#define HWIO_LPASS_LPAIF_WRDMA_CTLa_AUDIO_INTF_LOOPBACK_CH11_FVAL                         0x15
#define HWIO_LPASS_LPAIF_WRDMA_CTLa_FIFO_WATERMRK_BMSK                                    0x3e
#define HWIO_LPASS_LPAIF_WRDMA_CTLa_FIFO_WATERMRK_SHFT                                       1
#define HWIO_LPASS_LPAIF_WRDMA_CTLa_FIFO_WATERMRK_ENUM_1_FVAL                              0x0
#define HWIO_LPASS_LPAIF_WRDMA_CTLa_FIFO_WATERMRK_ENUM_2_FVAL                              0x1
#define HWIO_LPASS_LPAIF_WRDMA_CTLa_FIFO_WATERMRK_ENUM_3_FVAL                              0x2
#define HWIO_LPASS_LPAIF_WRDMA_CTLa_FIFO_WATERMRK_ENUM_4_FVAL                              0x3
#define HWIO_LPASS_LPAIF_WRDMA_CTLa_FIFO_WATERMRK_ENUM_5_FVAL                              0x4
#define HWIO_LPASS_LPAIF_WRDMA_CTLa_FIFO_WATERMRK_ENUM_6_FVAL                              0x5
#define HWIO_LPASS_LPAIF_WRDMA_CTLa_FIFO_WATERMRK_ENUM_7_FVAL                              0x6
#define HWIO_LPASS_LPAIF_WRDMA_CTLa_FIFO_WATERMRK_ENUM_8_FVAL                              0x7
#define HWIO_LPASS_LPAIF_WRDMA_CTLa_ENABLE_BMSK                                            0x1
#define HWIO_LPASS_LPAIF_WRDMA_CTLa_ENABLE_SHFT                                              0
#define HWIO_LPASS_LPAIF_WRDMA_CTLa_ENABLE_OFF_FVAL                                        0x0
#define HWIO_LPASS_LPAIF_WRDMA_CTLa_ENABLE_ON_FVAL                                         0x1

#define HWIO_LPASS_LPAIF_WRDMA_BASEa_ADDR(base,a)                                   ((base) + 0X18004 + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_WRDMA_BASEa_OFFS(a)                                        (0X18004 + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_WRDMA_BASEa_RMSK                                           0xfffffff0
#define HWIO_LPASS_LPAIF_WRDMA_BASEa_MAXa                                                    2
#define HWIO_LPASS_LPAIF_WRDMA_BASEa_INI(base,a)                \
                in_dword_masked(HWIO_LPASS_LPAIF_WRDMA_BASEa_ADDR(base,a), HWIO_LPASS_LPAIF_WRDMA_BASEa_RMSK)
#define HWIO_LPASS_LPAIF_WRDMA_BASEa_INMI(base,a,mask)        \
                in_dword_masked(HWIO_LPASS_LPAIF_WRDMA_BASEa_ADDR(base,a), mask)
#define HWIO_LPASS_LPAIF_WRDMA_BASEa_OUTI(base,a,val)        \
                out_dword(HWIO_LPASS_LPAIF_WRDMA_BASEa_ADDR(base,a),val)
#define HWIO_LPASS_LPAIF_WRDMA_BASEa_OUTMI(base,a,mask,val) \
                out_dword_masked_ns(HWIO_LPASS_LPAIF_WRDMA_BASEa_ADDR(base,a),mask,val,HWIO_LPASS_LPAIF_WRDMA_BASEa_INI(base,a))
#define HWIO_LPASS_LPAIF_WRDMA_BASEa_BASE_ADDR_BMSK                                 0xfffffff0
#define HWIO_LPASS_LPAIF_WRDMA_BASEa_BASE_ADDR_SHFT                                          4

#define HWIO_LPASS_LPAIF_WRDMA_BUFF_LENa_ADDR(base,a)                               ((base) + 0X18008 + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_WRDMA_BUFF_LENa_OFFS(a)                                    (0X18008 + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_WRDMA_BUFF_LENa_RMSK                                          0xffffc
#define HWIO_LPASS_LPAIF_WRDMA_BUFF_LENa_MAXa                                                2
#define HWIO_LPASS_LPAIF_WRDMA_BUFF_LENa_INI(base,a)                \
                in_dword_masked(HWIO_LPASS_LPAIF_WRDMA_BUFF_LENa_ADDR(base,a), HWIO_LPASS_LPAIF_WRDMA_BUFF_LENa_RMSK)
#define HWIO_LPASS_LPAIF_WRDMA_BUFF_LENa_INMI(base,a,mask)        \
                in_dword_masked(HWIO_LPASS_LPAIF_WRDMA_BUFF_LENa_ADDR(base,a), mask)
#define HWIO_LPASS_LPAIF_WRDMA_BUFF_LENa_OUTI(base,a,val)        \
                out_dword(HWIO_LPASS_LPAIF_WRDMA_BUFF_LENa_ADDR(base,a),val)
#define HWIO_LPASS_LPAIF_WRDMA_BUFF_LENa_OUTMI(base,a,mask,val) \
                out_dword_masked_ns(HWIO_LPASS_LPAIF_WRDMA_BUFF_LENa_ADDR(base,a),mask,val,HWIO_LPASS_LPAIF_WRDMA_BUFF_LENa_INI(base,a))
#define HWIO_LPASS_LPAIF_WRDMA_BUFF_LENa_LENGTH_BMSK                                   0xffffc
#define HWIO_LPASS_LPAIF_WRDMA_BUFF_LENa_LENGTH_SHFT                                         2

#define HWIO_LPASS_LPAIF_WRDMA_CURR_ADDRa_ADDR(base,a)                              ((base) + 0X1800C + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_WRDMA_CURR_ADDRa_OFFS(a)                                   (0X1800C + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_WRDMA_CURR_ADDRa_RMSK                                      0xffffffff
#define HWIO_LPASS_LPAIF_WRDMA_CURR_ADDRa_MAXa                                               2
#define HWIO_LPASS_LPAIF_WRDMA_CURR_ADDRa_INI(base,a)                \
                in_dword_masked(HWIO_LPASS_LPAIF_WRDMA_CURR_ADDRa_ADDR(base,a), HWIO_LPASS_LPAIF_WRDMA_CURR_ADDRa_RMSK)
#define HWIO_LPASS_LPAIF_WRDMA_CURR_ADDRa_INMI(base,a,mask)        \
                in_dword_masked(HWIO_LPASS_LPAIF_WRDMA_CURR_ADDRa_ADDR(base,a), mask)
#define HWIO_LPASS_LPAIF_WRDMA_CURR_ADDRa_ADDR_BMSK                                 0xffffffff
#define HWIO_LPASS_LPAIF_WRDMA_CURR_ADDRa_ADDR_SHFT                                          0

#define HWIO_LPASS_LPAIF_WRDMA_PER_LENa_ADDR(base,a)                                ((base) + 0X18010 + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_WRDMA_PER_LENa_OFFS(a)                                     (0X18010 + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_WRDMA_PER_LENa_RMSK                                           0xfffff
#define HWIO_LPASS_LPAIF_WRDMA_PER_LENa_MAXa                                                 2
#define HWIO_LPASS_LPAIF_WRDMA_PER_LENa_INI(base,a)                \
                in_dword_masked(HWIO_LPASS_LPAIF_WRDMA_PER_LENa_ADDR(base,a), HWIO_LPASS_LPAIF_WRDMA_PER_LENa_RMSK)
#define HWIO_LPASS_LPAIF_WRDMA_PER_LENa_INMI(base,a,mask)        \
                in_dword_masked(HWIO_LPASS_LPAIF_WRDMA_PER_LENa_ADDR(base,a), mask)
#define HWIO_LPASS_LPAIF_WRDMA_PER_LENa_OUTI(base,a,val)        \
                out_dword(HWIO_LPASS_LPAIF_WRDMA_PER_LENa_ADDR(base,a),val)
#define HWIO_LPASS_LPAIF_WRDMA_PER_LENa_OUTMI(base,a,mask,val) \
                out_dword_masked_ns(HWIO_LPASS_LPAIF_WRDMA_PER_LENa_ADDR(base,a),mask,val,HWIO_LPASS_LPAIF_WRDMA_PER_LENa_INI(base,a))
#define HWIO_LPASS_LPAIF_WRDMA_PER_LENa_LENGTH_BMSK                                    0xfffff
#define HWIO_LPASS_LPAIF_WRDMA_PER_LENa_LENGTH_SHFT                                          0

#define HWIO_LPASS_LPAIF_WRDMA_PER_CNTa_ADDR(base,a)                                ((base) + 0X18014 + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_WRDMA_PER_CNTa_OFFS(a)                                     (0X18014 + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_WRDMA_PER_CNTa_RMSK                                         0x7ffffff
#define HWIO_LPASS_LPAIF_WRDMA_PER_CNTa_MAXa                                                 2
#define HWIO_LPASS_LPAIF_WRDMA_PER_CNTa_INI(base,a)                \
                in_dword_masked(HWIO_LPASS_LPAIF_WRDMA_PER_CNTa_ADDR(base,a), HWIO_LPASS_LPAIF_WRDMA_PER_CNTa_RMSK)
#define HWIO_LPASS_LPAIF_WRDMA_PER_CNTa_INMI(base,a,mask)        \
                in_dword_masked(HWIO_LPASS_LPAIF_WRDMA_PER_CNTa_ADDR(base,a), mask)
#define HWIO_LPASS_LPAIF_WRDMA_PER_CNTa_FORMAT_ERR_BMSK                              0x4000000
#define HWIO_LPASS_LPAIF_WRDMA_PER_CNTa_FORMAT_ERR_SHFT                                     26
#define HWIO_LPASS_LPAIF_WRDMA_PER_CNTa_FIFO_WORDCNT_BMSK                            0x3f00000
#define HWIO_LPASS_LPAIF_WRDMA_PER_CNTa_FIFO_WORDCNT_SHFT                                   20
#define HWIO_LPASS_LPAIF_WRDMA_PER_CNTa_FIFO_WORDCNT_ENUM_0_FVAL                           0x0
#define HWIO_LPASS_LPAIF_WRDMA_PER_CNTa_FIFO_WORDCNT_ENUM_1_FVAL                           0x1
#define HWIO_LPASS_LPAIF_WRDMA_PER_CNTa_FIFO_WORDCNT_ENUM_2_FVAL                           0x2
#define HWIO_LPASS_LPAIF_WRDMA_PER_CNTa_FIFO_WORDCNT_ENUM_3_FVAL                           0x3
#define HWIO_LPASS_LPAIF_WRDMA_PER_CNTa_FIFO_WORDCNT_ENUM_4_FVAL                           0x4
#define HWIO_LPASS_LPAIF_WRDMA_PER_CNTa_FIFO_WORDCNT_ENUM_5_FVAL                           0x5
#define HWIO_LPASS_LPAIF_WRDMA_PER_CNTa_FIFO_WORDCNT_ENUM_6_FVAL                           0x6
#define HWIO_LPASS_LPAIF_WRDMA_PER_CNTa_FIFO_WORDCNT_ENUM_7_FVAL                           0x7
#define HWIO_LPASS_LPAIF_WRDMA_PER_CNTa_FIFO_WORDCNT_ENUM_8_FVAL                           0x8
#define HWIO_LPASS_LPAIF_WRDMA_PER_CNTa_PER_CNT_BMSK                                   0xfffff
#define HWIO_LPASS_LPAIF_WRDMA_PER_CNTa_PER_CNT_SHFT                                         0

#define HWIO_LPASS_LPAIF_WRDMA_FRMa_ADDR(base,a)                                    ((base) + 0X18018 + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_WRDMA_FRMa_OFFS(a)                                         (0X18018 + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_WRDMA_FRMa_RMSK                                            0x7fffffff
#define HWIO_LPASS_LPAIF_WRDMA_FRMa_MAXa                                                     2
#define HWIO_LPASS_LPAIF_WRDMA_FRMa_INI(base,a)                \
                in_dword_masked(HWIO_LPASS_LPAIF_WRDMA_FRMa_ADDR(base,a), HWIO_LPASS_LPAIF_WRDMA_FRMa_RMSK)
#define HWIO_LPASS_LPAIF_WRDMA_FRMa_INMI(base,a,mask)        \
                in_dword_masked(HWIO_LPASS_LPAIF_WRDMA_FRMa_ADDR(base,a), mask)
#define HWIO_LPASS_LPAIF_WRDMA_FRMa_FRAME_EXP_CNT_BMSK                              0x78000000
#define HWIO_LPASS_LPAIF_WRDMA_FRMa_FRAME_EXP_CNT_SHFT                                      27
#define HWIO_LPASS_LPAIF_WRDMA_FRMa_FRAME_UPDATE_BMSK                                0x4000000
#define HWIO_LPASS_LPAIF_WRDMA_FRMa_FRAME_UPDATE_SHFT                                       26
#define HWIO_LPASS_LPAIF_WRDMA_FRMa_FRAME_FIFO_BMSK                                  0x3f00000
#define HWIO_LPASS_LPAIF_WRDMA_FRMa_FRAME_FIFO_SHFT                                         20
#define HWIO_LPASS_LPAIF_WRDMA_FRMa_FRAME_FIFO_ENUM_0_FVAL                                 0x0
#define HWIO_LPASS_LPAIF_WRDMA_FRMa_FRAME_FIFO_ENUM_1_FVAL                                 0x1
#define HWIO_LPASS_LPAIF_WRDMA_FRMa_FRAME_FIFO_ENUM_2_FVAL                                 0x2
#define HWIO_LPASS_LPAIF_WRDMA_FRMa_FRAME_FIFO_ENUM_3_FVAL                                 0x3
#define HWIO_LPASS_LPAIF_WRDMA_FRMa_FRAME_FIFO_ENUM_4_FVAL                                 0x4
#define HWIO_LPASS_LPAIF_WRDMA_FRMa_FRAME_FIFO_ENUM_5_FVAL                                 0x5
#define HWIO_LPASS_LPAIF_WRDMA_FRMa_FRAME_FIFO_ENUM_6_FVAL                                 0x6
#define HWIO_LPASS_LPAIF_WRDMA_FRMa_FRAME_FIFO_ENUM_7_FVAL                                 0x7
#define HWIO_LPASS_LPAIF_WRDMA_FRMa_FRAME_FIFO_ENUM_8_FVAL                                 0x8
#define HWIO_LPASS_LPAIF_WRDMA_FRMa_FRAME_CNT_BMSK                                     0xfffff
#define HWIO_LPASS_LPAIF_WRDMA_FRMa_FRAME_CNT_SHFT                                           0

#define HWIO_LPASS_LPAIF_WRDMA_FRMCLRa_ADDR(base,a)                                 ((base) + 0X1801C + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_WRDMA_FRMCLRa_OFFS(a)                                      (0X1801C + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_WRDMA_FRMCLRa_RMSK                                                0x1
#define HWIO_LPASS_LPAIF_WRDMA_FRMCLRa_MAXa                                                  2
#define HWIO_LPASS_LPAIF_WRDMA_FRMCLRa_OUTI(base,a,val)        \
                out_dword(HWIO_LPASS_LPAIF_WRDMA_FRMCLRa_ADDR(base,a),val)
#define HWIO_LPASS_LPAIF_WRDMA_FRMCLRa_FRAME_CLR_BMSK                                      0x1
#define HWIO_LPASS_LPAIF_WRDMA_FRMCLRa_FRAME_CLR_SHFT                                        0

#define HWIO_LPASS_LPAIF_WRDMA_SET_BUFF_CNTa_ADDR(base,a)                           ((base) + 0X18020 + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_WRDMA_SET_BUFF_CNTa_OFFS(a)                                (0X18020 + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_WRDMA_SET_BUFF_CNTa_RMSK                                      0xfffff
#define HWIO_LPASS_LPAIF_WRDMA_SET_BUFF_CNTa_MAXa                                            2
#define HWIO_LPASS_LPAIF_WRDMA_SET_BUFF_CNTa_OUTI(base,a,val)        \
                out_dword(HWIO_LPASS_LPAIF_WRDMA_SET_BUFF_CNTa_ADDR(base,a),val)
#define HWIO_LPASS_LPAIF_WRDMA_SET_BUFF_CNTa_CNT_BMSK                                  0xfffff
#define HWIO_LPASS_LPAIF_WRDMA_SET_BUFF_CNTa_CNT_SHFT                                        0

#define HWIO_LPASS_LPAIF_WRDMA_SET_PER_CNTa_ADDR(base,a)                            ((base) + 0X18024 + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_WRDMA_SET_PER_CNTa_OFFS(a)                                 (0X18024 + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_WRDMA_SET_PER_CNTa_RMSK                                       0xfffff
#define HWIO_LPASS_LPAIF_WRDMA_SET_PER_CNTa_MAXa                                             2
#define HWIO_LPASS_LPAIF_WRDMA_SET_PER_CNTa_OUTI(base,a,val)        \
                out_dword(HWIO_LPASS_LPAIF_WRDMA_SET_PER_CNTa_ADDR(base,a),val)
#define HWIO_LPASS_LPAIF_WRDMA_SET_PER_CNTa_CNT_BMSK                                   0xfffff
#define HWIO_LPASS_LPAIF_WRDMA_SET_PER_CNTa_CNT_SHFT                                         0

#define HWIO_LPASS_LPAIF_WRDMA_STC_LSBa_ADDR(base,a)                                ((base) + 0X18028 + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_WRDMA_STC_LSBa_OFFS(a)                                     (0X18028 + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_WRDMA_STC_LSBa_RMSK                                        0xffffffff
#define HWIO_LPASS_LPAIF_WRDMA_STC_LSBa_MAXa                                                 2
#define HWIO_LPASS_LPAIF_WRDMA_STC_LSBa_INI(base,a)                \
                in_dword_masked(HWIO_LPASS_LPAIF_WRDMA_STC_LSBa_ADDR(base,a), HWIO_LPASS_LPAIF_WRDMA_STC_LSBa_RMSK)
#define HWIO_LPASS_LPAIF_WRDMA_STC_LSBa_INMI(base,a,mask)        \
                in_dword_masked(HWIO_LPASS_LPAIF_WRDMA_STC_LSBa_ADDR(base,a), mask)
#define HWIO_LPASS_LPAIF_WRDMA_STC_LSBa_STC_LSB_BMSK                                0xffffffff
#define HWIO_LPASS_LPAIF_WRDMA_STC_LSBa_STC_LSB_SHFT                                         0

#define HWIO_LPASS_LPAIF_WRDMA_STC_MSBa_ADDR(base,a)                                ((base) + 0X1802C + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_WRDMA_STC_MSBa_OFFS(a)                                     (0X1802C + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_WRDMA_STC_MSBa_RMSK                                          0xffffff
#define HWIO_LPASS_LPAIF_WRDMA_STC_MSBa_MAXa                                                 2
#define HWIO_LPASS_LPAIF_WRDMA_STC_MSBa_INI(base,a)                \
                in_dword_masked(HWIO_LPASS_LPAIF_WRDMA_STC_MSBa_ADDR(base,a), HWIO_LPASS_LPAIF_WRDMA_STC_MSBa_RMSK)
#define HWIO_LPASS_LPAIF_WRDMA_STC_MSBa_INMI(base,a,mask)        \
                in_dword_masked(HWIO_LPASS_LPAIF_WRDMA_STC_MSBa_ADDR(base,a), mask)
#define HWIO_LPASS_LPAIF_WRDMA_STC_MSBa_STC_MSB_BMSK                                  0xffffff
#define HWIO_LPASS_LPAIF_WRDMA_STC_MSBa_STC_MSB_SHFT                                         0

#define HWIO_LPASS_LPAIF_WRDMA_PERa_ADDR(base,a)                                    ((base) + 0X18030 + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_WRDMA_PERa_OFFS(a)                                         (0X18030 + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_WRDMA_PERa_RMSK                                                0x100f
#define HWIO_LPASS_LPAIF_WRDMA_PERa_MAXa                                                     2
#define HWIO_LPASS_LPAIF_WRDMA_PERa_INI(base,a)                \
                in_dword_masked(HWIO_LPASS_LPAIF_WRDMA_PERa_ADDR(base,a), HWIO_LPASS_LPAIF_WRDMA_PERa_RMSK)
#define HWIO_LPASS_LPAIF_WRDMA_PERa_INMI(base,a,mask)        \
                in_dword_masked(HWIO_LPASS_LPAIF_WRDMA_PERa_ADDR(base,a), mask)
#define HWIO_LPASS_LPAIF_WRDMA_PERa_PERIOD_UPDATE_BMSK                                  0x1000
#define HWIO_LPASS_LPAIF_WRDMA_PERa_PERIOD_UPDATE_SHFT                                      12
#define HWIO_LPASS_LPAIF_WRDMA_PERa_PERIOD_FIFO_BMSK                                       0xf
#define HWIO_LPASS_LPAIF_WRDMA_PERa_PERIOD_FIFO_SHFT                                         0
#define HWIO_LPASS_LPAIF_WRDMA_PERa_PERIOD_FIFO_ENUM_0_FVAL                                0x0
#define HWIO_LPASS_LPAIF_WRDMA_PERa_PERIOD_FIFO_ENUM_1_FVAL                                0x1
#define HWIO_LPASS_LPAIF_WRDMA_PERa_PERIOD_FIFO_ENUM_2_FVAL                                0x2
#define HWIO_LPASS_LPAIF_WRDMA_PERa_PERIOD_FIFO_ENUM_3_FVAL                                0x3
#define HWIO_LPASS_LPAIF_WRDMA_PERa_PERIOD_FIFO_ENUM_4_FVAL                                0x4
#define HWIO_LPASS_LPAIF_WRDMA_PERa_PERIOD_FIFO_ENUM_5_FVAL                                0x5
#define HWIO_LPASS_LPAIF_WRDMA_PERa_PERIOD_FIFO_ENUM_6_FVAL                                0x6
#define HWIO_LPASS_LPAIF_WRDMA_PERa_PERIOD_FIFO_ENUM_7_FVAL                                0x7
#define HWIO_LPASS_LPAIF_WRDMA_PERa_PERIOD_FIFO_ENUM_8_FVAL                                0x8

#define HWIO_LPASS_LPAIF_WRDMA_PERCLRa_ADDR(base,a)                                 ((base) + 0X18034 + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_WRDMA_PERCLRa_OFFS(a)                                      (0X18034 + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_WRDMA_PERCLRa_RMSK                                                0x1
#define HWIO_LPASS_LPAIF_WRDMA_PERCLRa_MAXa                                                  2
#define HWIO_LPASS_LPAIF_WRDMA_PERCLRa_OUTI(base,a,val)        \
                out_dword(HWIO_LPASS_LPAIF_WRDMA_PERCLRa_ADDR(base,a),val)
#define HWIO_LPASS_LPAIF_WRDMA_PERCLRa_PERIOD_CLR_BMSK                                     0x1
#define HWIO_LPASS_LPAIF_WRDMA_PERCLRa_PERIOD_CLR_SHFT                                       0

#define HWIO_LPASS_LPAIF_WRDMA_BASE_EXTa_ADDR(base,a)                               ((base) + 0X18038 + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_WRDMA_BASE_EXTa_OFFS(a)                                    (0X18038 + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_WRDMA_BASE_EXTa_RMSK                                              0xf
#define HWIO_LPASS_LPAIF_WRDMA_BASE_EXTa_MAXa                                                2
#define HWIO_LPASS_LPAIF_WRDMA_BASE_EXTa_INI(base,a)                \
                in_dword_masked(HWIO_LPASS_LPAIF_WRDMA_BASE_EXTa_ADDR(base,a), HWIO_LPASS_LPAIF_WRDMA_BASE_EXTa_RMSK)
#define HWIO_LPASS_LPAIF_WRDMA_BASE_EXTa_INMI(base,a,mask)        \
                in_dword_masked(HWIO_LPASS_LPAIF_WRDMA_BASE_EXTa_ADDR(base,a), mask)
#define HWIO_LPASS_LPAIF_WRDMA_BASE_EXTa_OUTI(base,a,val)        \
                out_dword(HWIO_LPASS_LPAIF_WRDMA_BASE_EXTa_ADDR(base,a),val)
#define HWIO_LPASS_LPAIF_WRDMA_BASE_EXTa_OUTMI(base,a,mask,val) \
                out_dword_masked_ns(HWIO_LPASS_LPAIF_WRDMA_BASE_EXTa_ADDR(base,a),mask,val,HWIO_LPASS_LPAIF_WRDMA_BASE_EXTa_INI(base,a))
#define HWIO_LPASS_LPAIF_WRDMA_BASE_EXTa_BASE_ADDR_BMSK                                    0xf
#define HWIO_LPASS_LPAIF_WRDMA_BASE_EXTa_BASE_ADDR_SHFT                                      0

#define HWIO_LPASS_LPAIF_WRDMA_CURR_ADDR_EXTa_ADDR(base,a)                          ((base) + 0X1803C + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_WRDMA_CURR_ADDR_EXTa_OFFS(a)                               (0X1803C + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_WRDMA_CURR_ADDR_EXTa_RMSK                                         0xf
#define HWIO_LPASS_LPAIF_WRDMA_CURR_ADDR_EXTa_MAXa                                           2
#define HWIO_LPASS_LPAIF_WRDMA_CURR_ADDR_EXTa_INI(base,a)                \
                in_dword_masked(HWIO_LPASS_LPAIF_WRDMA_CURR_ADDR_EXTa_ADDR(base,a), HWIO_LPASS_LPAIF_WRDMA_CURR_ADDR_EXTa_RMSK)
#define HWIO_LPASS_LPAIF_WRDMA_CURR_ADDR_EXTa_INMI(base,a,mask)        \
                in_dword_masked(HWIO_LPASS_LPAIF_WRDMA_CURR_ADDR_EXTa_ADDR(base,a), mask)
#define HWIO_LPASS_LPAIF_WRDMA_CURR_ADDR_EXTa_ADDR_BMSK                                    0xf
#define HWIO_LPASS_LPAIF_WRDMA_CURR_ADDR_EXTa_ADDR_SHFT                                      0

#define HWIO_LPASS_LPAIF_WRDMA_PER_CNT_EXTa_ADDR(base,a)                            ((base) + 0X18040 + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_WRDMA_PER_CNT_EXTa_OFFS(a)                                 (0X18040 + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_WRDMA_PER_CNT_EXTa_RMSK                                         0xfff
#define HWIO_LPASS_LPAIF_WRDMA_PER_CNT_EXTa_MAXa                                             2
#define HWIO_LPASS_LPAIF_WRDMA_PER_CNT_EXTa_INI(base,a)                \
                in_dword_masked(HWIO_LPASS_LPAIF_WRDMA_PER_CNT_EXTa_ADDR(base,a), HWIO_LPASS_LPAIF_WRDMA_PER_CNT_EXTa_RMSK)
#define HWIO_LPASS_LPAIF_WRDMA_PER_CNT_EXTa_INMI(base,a,mask)        \
                in_dword_masked(HWIO_LPASS_LPAIF_WRDMA_PER_CNT_EXTa_ADDR(base,a), mask)
#define HWIO_LPASS_LPAIF_WRDMA_PER_CNT_EXTa_FIFO_WORDCNT_BMSK                            0xfff
#define HWIO_LPASS_LPAIF_WRDMA_PER_CNT_EXTa_FIFO_WORDCNT_SHFT                                0
#define HWIO_LPASS_LPAIF_WRDMA_PER_CNT_EXTa_FIFO_WORDCNT_ENUM_0_FVAL                       0x0
#define HWIO_LPASS_LPAIF_WRDMA_PER_CNT_EXTa_FIFO_WORDCNT_ENUM_1_FVAL                       0x1
#define HWIO_LPASS_LPAIF_WRDMA_PER_CNT_EXTa_FIFO_WORDCNT_ENUM_2_FVAL                       0x2
#define HWIO_LPASS_LPAIF_WRDMA_PER_CNT_EXTa_FIFO_WORDCNT_ENUM_3_FVAL                       0x3
#define HWIO_LPASS_LPAIF_WRDMA_PER_CNT_EXTa_FIFO_WORDCNT_ENUM_4_FVAL                       0x4
#define HWIO_LPASS_LPAIF_WRDMA_PER_CNT_EXTa_FIFO_WORDCNT_ENUM_5_FVAL                       0x5
#define HWIO_LPASS_LPAIF_WRDMA_PER_CNT_EXTa_FIFO_WORDCNT_ENUM_6_FVAL                       0x6
#define HWIO_LPASS_LPAIF_WRDMA_PER_CNT_EXTa_FIFO_WORDCNT_ENUM_7_FVAL                       0x7
#define HWIO_LPASS_LPAIF_WRDMA_PER_CNT_EXTa_FIFO_WORDCNT_ENUM_8_FVAL                       0x8

#define HWIO_LPASS_LPAIF_WRDMA_FRM_EXTa_ADDR(base,a)                                ((base) + 0X18044 + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_WRDMA_FRM_EXTa_OFFS(a)                                     (0X18044 + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_WRDMA_FRM_EXTa_RMSK                                             0xfff
#define HWIO_LPASS_LPAIF_WRDMA_FRM_EXTa_MAXa                                                 2
#define HWIO_LPASS_LPAIF_WRDMA_FRM_EXTa_INI(base,a)                \
                in_dword_masked(HWIO_LPASS_LPAIF_WRDMA_FRM_EXTa_ADDR(base,a), HWIO_LPASS_LPAIF_WRDMA_FRM_EXTa_RMSK)
#define HWIO_LPASS_LPAIF_WRDMA_FRM_EXTa_INMI(base,a,mask)        \
                in_dword_masked(HWIO_LPASS_LPAIF_WRDMA_FRM_EXTa_ADDR(base,a), mask)
#define HWIO_LPASS_LPAIF_WRDMA_FRM_EXTa_FRAME_FIFO_BMSK                                  0xfff
#define HWIO_LPASS_LPAIF_WRDMA_FRM_EXTa_FRAME_FIFO_SHFT                                      0
#define HWIO_LPASS_LPAIF_WRDMA_FRM_EXTa_FRAME_FIFO_ENUM_0_FVAL                             0x0
#define HWIO_LPASS_LPAIF_WRDMA_FRM_EXTa_FRAME_FIFO_ENUM_1_FVAL                             0x1
#define HWIO_LPASS_LPAIF_WRDMA_FRM_EXTa_FRAME_FIFO_ENUM_2_FVAL                             0x2
#define HWIO_LPASS_LPAIF_WRDMA_FRM_EXTa_FRAME_FIFO_ENUM_3_FVAL                             0x3
#define HWIO_LPASS_LPAIF_WRDMA_FRM_EXTa_FRAME_FIFO_ENUM_4_FVAL                             0x4
#define HWIO_LPASS_LPAIF_WRDMA_FRM_EXTa_FRAME_FIFO_ENUM_5_FVAL                             0x5
#define HWIO_LPASS_LPAIF_WRDMA_FRM_EXTa_FRAME_FIFO_ENUM_6_FVAL                             0x6
#define HWIO_LPASS_LPAIF_WRDMA_FRM_EXTa_FRAME_FIFO_ENUM_7_FVAL                             0x7
#define HWIO_LPASS_LPAIF_WRDMA_FRM_EXTa_FRAME_FIFO_ENUM_8_FVAL                             0x8

#define HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR0_ADDR(x)                              ((x) + 0x18048)
#define HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR0_OFFS                                 (0x18048)
#define HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR0_RMSK                                      0x7ff
#define HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR0_IN(x)            \
                in_dword(HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR0_ADDR(x))
#define HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR0_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR0_ADDR(x), m)
#define HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR0_OUT(x, v)            \
                out_dword(HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR0_ADDR(x),v)
#define HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR0_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR0_ADDR(x),m,v,HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR0_IN(x))
#define HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR0_START_ADDR_BMSK                           0x7ff
#define HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR0_START_ADDR_SHFT                               0

#define HWIO_LPASS_LPAIF_WRDMA_RAM_LENGTHa_ADDR(base,a)                             ((base) + 0X1804C + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_WRDMA_RAM_LENGTHa_OFFS(a)                                  (0X1804C + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_WRDMA_RAM_LENGTHa_RMSK                                          0x7ff
#define HWIO_LPASS_LPAIF_WRDMA_RAM_LENGTHa_MAXa                                              2
#define HWIO_LPASS_LPAIF_WRDMA_RAM_LENGTHa_INI(base,a)                \
                in_dword_masked(HWIO_LPASS_LPAIF_WRDMA_RAM_LENGTHa_ADDR(base,a), HWIO_LPASS_LPAIF_WRDMA_RAM_LENGTHa_RMSK)
#define HWIO_LPASS_LPAIF_WRDMA_RAM_LENGTHa_INMI(base,a,mask)        \
                in_dword_masked(HWIO_LPASS_LPAIF_WRDMA_RAM_LENGTHa_ADDR(base,a), mask)
#define HWIO_LPASS_LPAIF_WRDMA_RAM_LENGTHa_OUTI(base,a,val)        \
                out_dword(HWIO_LPASS_LPAIF_WRDMA_RAM_LENGTHa_ADDR(base,a),val)
#define HWIO_LPASS_LPAIF_WRDMA_RAM_LENGTHa_OUTMI(base,a,mask,val) \
                out_dword_masked_ns(HWIO_LPASS_LPAIF_WRDMA_RAM_LENGTHa_ADDR(base,a),mask,val,HWIO_LPASS_LPAIF_WRDMA_RAM_LENGTHa_INI(base,a))
#define HWIO_LPASS_LPAIF_WRDMA_RAM_LENGTHa_LENGTH_BMSK                                   0x7ff
#define HWIO_LPASS_LPAIF_WRDMA_RAM_LENGTHa_LENGTH_SHFT                                       0

#define HWIO_LPASS_LPAIF_WRDMA_CODEC_INTFa_ADDR(base,a)                             ((base) + 0X18050 + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_WRDMA_CODEC_INTFa_OFFS(a)                                  (0X18050 + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_WRDMA_CODEC_INTFa_RMSK                                     0xefef00ff
#define HWIO_LPASS_LPAIF_WRDMA_CODEC_INTFa_MAXa                                              2
#define HWIO_LPASS_LPAIF_WRDMA_CODEC_INTFa_INI(base,a)                \
                in_dword_masked(HWIO_LPASS_LPAIF_WRDMA_CODEC_INTFa_ADDR(base,a), HWIO_LPASS_LPAIF_WRDMA_CODEC_INTFa_RMSK)
#define HWIO_LPASS_LPAIF_WRDMA_CODEC_INTFa_INMI(base,a,mask)        \
                in_dword_masked(HWIO_LPASS_LPAIF_WRDMA_CODEC_INTFa_ADDR(base,a), mask)
#define HWIO_LPASS_LPAIF_WRDMA_CODEC_INTFa_OUTI(base,a,val)        \
                out_dword(HWIO_LPASS_LPAIF_WRDMA_CODEC_INTFa_ADDR(base,a),val)
#define HWIO_LPASS_LPAIF_WRDMA_CODEC_INTFa_OUTMI(base,a,mask,val) \
                out_dword_masked_ns(HWIO_LPASS_LPAIF_WRDMA_CODEC_INTFa_ADDR(base,a),mask,val,HWIO_LPASS_LPAIF_WRDMA_CODEC_INTFa_INI(base,a))
#define HWIO_LPASS_LPAIF_WRDMA_CODEC_INTFa_RESET_BMSK                               0x80000000
#define HWIO_LPASS_LPAIF_WRDMA_CODEC_INTFa_RESET_SHFT                                       31
#define HWIO_LPASS_LPAIF_WRDMA_CODEC_INTFa_ENABLE_BMSK                              0x40000000
#define HWIO_LPASS_LPAIF_WRDMA_CODEC_INTFa_ENABLE_SHFT                                      30
#define HWIO_LPASS_LPAIF_WRDMA_CODEC_INTFa_ENABLE_16BIT_PACKING_BMSK                0x20000000
#define HWIO_LPASS_LPAIF_WRDMA_CODEC_INTFa_ENABLE_16BIT_PACKING_SHFT                        29
#define HWIO_LPASS_LPAIF_WRDMA_CODEC_INTFa_ENABLE_16BIT_PACKING_DISABLE_FVAL               0x0
#define HWIO_LPASS_LPAIF_WRDMA_CODEC_INTFa_ENABLE_16BIT_PACKING_ENABLE_FVAL                0x1
#define HWIO_LPASS_LPAIF_WRDMA_CODEC_INTFa_CODEC_FS_SEL_BMSK                         0xe000000
#define HWIO_LPASS_LPAIF_WRDMA_CODEC_INTFa_CODEC_FS_SEL_SHFT                                25
#define HWIO_LPASS_LPAIF_WRDMA_CODEC_INTFa_CODEC_FS_DELAY_BMSK                       0x1e00000
#define HWIO_LPASS_LPAIF_WRDMA_CODEC_INTFa_CODEC_FS_DELAY_SHFT                              21
#define HWIO_LPASS_LPAIF_WRDMA_CODEC_INTFa_CODEC_INTF_BMSK                             0xf0000
#define HWIO_LPASS_LPAIF_WRDMA_CODEC_INTFa_CODEC_INTF_SHFT                                  16
#define HWIO_LPASS_LPAIF_WRDMA_CODEC_INTFa_CODEC_INTF_NONE_FVAL                            0x0
#define HWIO_LPASS_LPAIF_WRDMA_CODEC_INTFa_CODEC_INTF_CODEC_TX0_SRC_FVAL                   0x1
#define HWIO_LPASS_LPAIF_WRDMA_CODEC_INTFa_CODEC_INTF_CODEC_TX1_SRC_FVAL                   0x2
#define HWIO_LPASS_LPAIF_WRDMA_CODEC_INTFa_CODEC_INTF_CODEC_TX2_SRC_FVAL                   0x3
#define HWIO_LPASS_LPAIF_WRDMA_CODEC_INTFa_CODEC_INTF_CODEC_TX3_SRC_FVAL                   0x4
#define HWIO_LPASS_LPAIF_WRDMA_CODEC_INTFa_CODEC_INTF_CODEC_TX4_SRC_FVAL                   0x5
#define HWIO_LPASS_LPAIF_WRDMA_CODEC_INTFa_CODEC_INTF_CODEC_TX5_SRC_FVAL                   0x6
#define HWIO_LPASS_LPAIF_WRDMA_CODEC_INTFa_CODEC_INTF_CODEC_TX6_SRC_FVAL                   0x7
#define HWIO_LPASS_LPAIF_WRDMA_CODEC_INTFa_CODEC_INTF_CODEC_TX7_SRC_FVAL                   0x8
#define HWIO_LPASS_LPAIF_WRDMA_CODEC_INTFa_CODEC_INTF_CODEC_TX8_SRC_FVAL                   0x9
#define HWIO_LPASS_LPAIF_WRDMA_CODEC_INTFa_CODEC_INTF_PRI_SPDIFRX_SRC_FVAL                 0xa
#define HWIO_LPASS_LPAIF_WRDMA_CODEC_INTFa_CODEC_INTF_SEC_SPDIFRX_SRC_FVAL                 0xb
#define HWIO_LPASS_LPAIF_WRDMA_CODEC_INTFa_NUM_ACTIVE_CHANNEL_BMSK                        0xff
#define HWIO_LPASS_LPAIF_WRDMA_CODEC_INTFa_NUM_ACTIVE_CHANNEL_SHFT                           0

#define HWIO_LPASS_LPAIF_WRDMA_AHB_BYPASSa_ADDR(base,a)                             ((base) + 0X18054 + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_WRDMA_AHB_BYPASSa_OFFS(a)                                  (0X18054 + (0x1000*(a)))
#define HWIO_LPASS_LPAIF_WRDMA_AHB_BYPASSa_RMSK                                          0x10f
#define HWIO_LPASS_LPAIF_WRDMA_AHB_BYPASSa_MAXa                                              2
#define HWIO_LPASS_LPAIF_WRDMA_AHB_BYPASSa_INI(base,a)                \
                in_dword_masked(HWIO_LPASS_LPAIF_WRDMA_AHB_BYPASSa_ADDR(base,a), HWIO_LPASS_LPAIF_WRDMA_AHB_BYPASSa_RMSK)
#define HWIO_LPASS_LPAIF_WRDMA_AHB_BYPASSa_INMI(base,a,mask)        \
                in_dword_masked(HWIO_LPASS_LPAIF_WRDMA_AHB_BYPASSa_ADDR(base,a), mask)
#define HWIO_LPASS_LPAIF_WRDMA_AHB_BYPASSa_OUTI(base,a,val)        \
                out_dword(HWIO_LPASS_LPAIF_WRDMA_AHB_BYPASSa_ADDR(base,a),val)
#define HWIO_LPASS_LPAIF_WRDMA_AHB_BYPASSa_OUTMI(base,a,mask,val) \
                out_dword_masked_ns(HWIO_LPASS_LPAIF_WRDMA_AHB_BYPASSa_ADDR(base,a),mask,val,HWIO_LPASS_LPAIF_WRDMA_AHB_BYPASSa_INI(base,a))
#define HWIO_LPASS_LPAIF_WRDMA_AHB_BYPASSa_BYPASS_ENABLE_BMSK                            0x100
#define HWIO_LPASS_LPAIF_WRDMA_AHB_BYPASSa_BYPASS_ENABLE_SHFT                                8
#define HWIO_LPASS_LPAIF_WRDMA_AHB_BYPASSa_BYPASS_SEL_BMSK                                 0xf
#define HWIO_LPASS_LPAIF_WRDMA_AHB_BYPASSa_BYPASS_SEL_SHFT                                   0
#define HWIO_LPASS_LPAIF_WRDMA_AHB_BYPASSa_BYPASS_SEL_NONE_FVAL                            0x0
#define HWIO_LPASS_LPAIF_WRDMA_AHB_BYPASSa_BYPASS_SEL_CODEC_RX0_SRC_FVAL                   0x1
#define HWIO_LPASS_LPAIF_WRDMA_AHB_BYPASSa_BYPASS_SEL_CODEC_RX1_SRC_FVAL                   0x2
#define HWIO_LPASS_LPAIF_WRDMA_AHB_BYPASSa_BYPASS_SEL_CODEC_RX2_SRC_FVAL                   0x3
#define HWIO_LPASS_LPAIF_WRDMA_AHB_BYPASSa_BYPASS_SEL_CODEC_RX3_SRC_FVAL                   0x4
#define HWIO_LPASS_LPAIF_WRDMA_AHB_BYPASSa_BYPASS_SEL_CODEC_RX4_SRC_FVAL                   0x5
#define HWIO_LPASS_LPAIF_WRDMA_AHB_BYPASSa_BYPASS_SEL_CODEC_RX5_SRC_FVAL                   0x6
#define HWIO_LPASS_LPAIF_WRDMA_AHB_BYPASSa_BYPASS_SEL_CODEC_RX6_SRC_FVAL                   0x7
#define HWIO_LPASS_LPAIF_WRDMA_AHB_BYPASSa_BYPASS_SEL_CODEC_RX7_SRC_FVAL                   0x8
#define HWIO_LPASS_LPAIF_WRDMA_AHB_BYPASSa_BYPASS_SEL_CODEC_RX8_SRC_FVAL                   0x9
#define HWIO_LPASS_LPAIF_WRDMA_AHB_BYPASSa_BYPASS_SEL_CODEC_RX9_SRC_FVAL                   0xa

#define HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR1_ADDR(x)                              ((x) + 0x19048)
#define HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR1_OFFS                                 (0x19048)
#define HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR1_RMSK                                      0x7ff
#define HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR1_IN(x)            \
                in_dword(HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR1_ADDR(x))
#define HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR1_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR1_ADDR(x), m)
#define HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR1_OUT(x, v)            \
                out_dword(HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR1_ADDR(x),v)
#define HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR1_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR1_ADDR(x),m,v,HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR1_IN(x))
#define HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR1_START_ADDR_BMSK                           0x7ff
#define HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR1_START_ADDR_SHFT                               0

#define HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR2_ADDR(x)                              ((x) + 0x1a048)
#define HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR2_OFFS                                 (0x1a048)
#define HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR2_RMSK                                      0x7ff
#define HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR2_IN(x)            \
                in_dword(HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR2_ADDR(x))
#define HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR2_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR2_ADDR(x), m)
#define HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR2_OUT(x, v)            \
                out_dword(HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR2_ADDR(x),v)
#define HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR2_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR2_ADDR(x),m,v,HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR2_IN(x))
#define HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR2_START_ADDR_BMSK                           0x7ff
#define HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR2_START_ADDR_SHFT                               0

#define HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR3_ADDR(x)                              ((x) + 0x1b048)
#define HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR3_OFFS                                 (0x1b048)
#define HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR3_RMSK                                      0x7ff
#define HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR3_IN(x)            \
                in_dword(HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR3_ADDR(x))
#define HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR3_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR3_ADDR(x), m)
#define HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR3_OUT(x, v)            \
                out_dword(HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR3_ADDR(x),v)
#define HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR3_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR3_ADDR(x),m,v,HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR3_IN(x))
#define HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR3_START_ADDR_BMSK                           0x7ff
#define HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR3_START_ADDR_SHFT                               0

#define HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR4_ADDR(x)                              ((x) + 0x1c048)
#define HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR4_OFFS                                 (0x1c048)
#define HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR4_RMSK                                      0x7ff
#define HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR4_IN(x)            \
                in_dword(HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR4_ADDR(x))
#define HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR4_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR4_ADDR(x), m)
#define HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR4_OUT(x, v)            \
                out_dword(HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR4_ADDR(x),v)
#define HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR4_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR4_ADDR(x),m,v,HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR4_IN(x))
#define HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR4_START_ADDR_BMSK                           0x7ff
#define HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR4_START_ADDR_SHFT                               0

#define HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR5_ADDR(x)                              ((x) + 0x1d048)
#define HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR5_OFFS                                 (0x1d048)
#define HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR5_RMSK                                      0x7ff
#define HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR5_IN(x)            \
                in_dword(HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR5_ADDR(x))
#define HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR5_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR5_ADDR(x), m)
#define HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR5_OUT(x, v)            \
                out_dword(HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR5_ADDR(x),v)
#define HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR5_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR5_ADDR(x),m,v,HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR5_IN(x))
#define HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR5_START_ADDR_BMSK                           0x7ff
#define HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR5_START_ADDR_SHFT                               0

#define HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR6_ADDR(x)                              ((x) + 0x1e048)
#define HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR6_OFFS                                 (0x1e048)
#define HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR6_RMSK                                      0x7ff
#define HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR6_IN(x)            \
                in_dword(HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR6_ADDR(x))
#define HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR6_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR6_ADDR(x), m)
#define HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR6_OUT(x, v)            \
                out_dword(HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR6_ADDR(x),v)
#define HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR6_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR6_ADDR(x),m,v,HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR6_IN(x))
#define HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR6_START_ADDR_BMSK                           0x7ff
#define HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR6_START_ADDR_SHFT                               0

#define HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR7_ADDR(x)                              ((x) + 0x1f048)
#define HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR7_OFFS                                 (0x1f048)
#define HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR7_RMSK                                      0x7ff
#define HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR7_IN(x)            \
                in_dword(HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR7_ADDR(x))
#define HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR7_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR7_ADDR(x), m)
#define HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR7_OUT(x, v)            \
                out_dword(HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR7_ADDR(x),v)
#define HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR7_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR7_ADDR(x),m,v,HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR7_IN(x))
#define HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR7_START_ADDR_BMSK                           0x7ff
#define HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR7_START_ADDR_SHFT                               0

#define HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR8_ADDR(x)                              ((x) + 0x20048)
#define HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR8_OFFS                                 (0x20048)
#define HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR8_RMSK                                      0x7ff
#define HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR8_IN(x)            \
                in_dword(HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR8_ADDR(x))
#define HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR8_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR8_ADDR(x), m)
#define HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR8_OUT(x, v)            \
                out_dword(HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR8_ADDR(x),v)
#define HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR8_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR8_ADDR(x),m,v,HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR8_IN(x))
#define HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR8_START_ADDR_BMSK                           0x7ff
#define HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR8_START_ADDR_SHFT                               0

#define HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR9_ADDR(x)                              ((x) + 0x21048)
#define HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR9_OFFS                                 (0x21048)
#define HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR9_RMSK                                      0x7ff
#define HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR9_IN(x)            \
                in_dword(HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR9_ADDR(x))
#define HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR9_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR9_ADDR(x), m)
#define HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR9_OUT(x, v)            \
                out_dword(HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR9_ADDR(x),v)
#define HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR9_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR9_ADDR(x),m,v,HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR9_IN(x))
#define HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR9_START_ADDR_BMSK                           0x7ff
#define HWIO_LPASS_LPAIF_WRDMA_RAM_START_ADDR9_START_ADDR_SHFT                               0

#define HWIO_LPASS_LPAIF_PRI_RATE_DET_CONFIG_ADDR(x)                                ((x) + 0x22000)
#define HWIO_LPASS_LPAIF_PRI_RATE_DET_CONFIG_OFFS                                   (0x22000)
#define HWIO_LPASS_LPAIF_PRI_RATE_DET_CONFIG_RMSK                                   0xfffffff1
#define HWIO_LPASS_LPAIF_PRI_RATE_DET_CONFIG_IN(x)            \
                in_dword(HWIO_LPASS_LPAIF_PRI_RATE_DET_CONFIG_ADDR(x))
#define HWIO_LPASS_LPAIF_PRI_RATE_DET_CONFIG_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPAIF_PRI_RATE_DET_CONFIG_ADDR(x), m)
#define HWIO_LPASS_LPAIF_PRI_RATE_DET_CONFIG_OUT(x, v)            \
                out_dword(HWIO_LPASS_LPAIF_PRI_RATE_DET_CONFIG_ADDR(x),v)
#define HWIO_LPASS_LPAIF_PRI_RATE_DET_CONFIG_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_LPAIF_PRI_RATE_DET_CONFIG_ADDR(x),m,v,HWIO_LPASS_LPAIF_PRI_RATE_DET_CONFIG_IN(x))
#define HWIO_LPASS_LPAIF_PRI_RATE_DET_CONFIG_RESET_BMSK                             0x80000000
#define HWIO_LPASS_LPAIF_PRI_RATE_DET_CONFIG_RESET_SHFT                                     31
#define HWIO_LPASS_LPAIF_PRI_RATE_DET_CONFIG_RESET_DISABLE_FVAL                            0x0
#define HWIO_LPASS_LPAIF_PRI_RATE_DET_CONFIG_RESET_ENABLE_FVAL                             0x1
#define HWIO_LPASS_LPAIF_PRI_RATE_DET_CONFIG_VAR_192K_1764K_BMSK                    0x7f800000
#define HWIO_LPASS_LPAIF_PRI_RATE_DET_CONFIG_VAR_192K_1764K_SHFT                            23
#define HWIO_LPASS_LPAIF_PRI_RATE_DET_CONFIG_VAR_128K_441K_BMSK                       0x7f8000
#define HWIO_LPASS_LPAIF_PRI_RATE_DET_CONFIG_VAR_128K_441K_SHFT                             15
#define HWIO_LPASS_LPAIF_PRI_RATE_DET_CONFIG_VAR_32K_8K_BMSK                            0x7f80
#define HWIO_LPASS_LPAIF_PRI_RATE_DET_CONFIG_VAR_32K_8K_SHFT                                 7
#define HWIO_LPASS_LPAIF_PRI_RATE_DET_CONFIG_NUM_FS_BMSK                                  0x70
#define HWIO_LPASS_LPAIF_PRI_RATE_DET_CONFIG_NUM_FS_SHFT                                     4
#define HWIO_LPASS_LPAIF_PRI_RATE_DET_CONFIG_NUM_FS_ONE_FS_FVAL                            0x0
#define HWIO_LPASS_LPAIF_PRI_RATE_DET_CONFIG_NUM_FS_TWO_FS_FVAL                            0x1
#define HWIO_LPASS_LPAIF_PRI_RATE_DET_CONFIG_NUM_FS_THREE_FS_FVAL                          0x2
#define HWIO_LPASS_LPAIF_PRI_RATE_DET_CONFIG_NUM_FS_FOUR_FS_FVAL                           0x3
#define HWIO_LPASS_LPAIF_PRI_RATE_DET_CONFIG_NUM_FS_FIVE_FS_FVAL                           0x4
#define HWIO_LPASS_LPAIF_PRI_RATE_DET_CONFIG_NUM_FS_SIX_FS_FVAL                            0x5
#define HWIO_LPASS_LPAIF_PRI_RATE_DET_CONFIG_NUM_FS_SEVEN_FS_FVAL                          0x6
#define HWIO_LPASS_LPAIF_PRI_RATE_DET_CONFIG_NUM_FS_EIGHT_FS_FVAL                          0x7
#define HWIO_LPASS_LPAIF_PRI_RATE_DET_CONFIG_EN_BMSK                                       0x1
#define HWIO_LPASS_LPAIF_PRI_RATE_DET_CONFIG_EN_SHFT                                         0

#define HWIO_LPASS_LPAIF_PRI_RATE_DET_TARGET1_CONFIG_ADDR(x)                        ((x) + 0x22004)
#define HWIO_LPASS_LPAIF_PRI_RATE_DET_TARGET1_CONFIG_OFFS                           (0x22004)
#define HWIO_LPASS_LPAIF_PRI_RATE_DET_TARGET1_CONFIG_RMSK                           0xffffffff
#define HWIO_LPASS_LPAIF_PRI_RATE_DET_TARGET1_CONFIG_IN(x)            \
                in_dword(HWIO_LPASS_LPAIF_PRI_RATE_DET_TARGET1_CONFIG_ADDR(x))
#define HWIO_LPASS_LPAIF_PRI_RATE_DET_TARGET1_CONFIG_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPAIF_PRI_RATE_DET_TARGET1_CONFIG_ADDR(x), m)
#define HWIO_LPASS_LPAIF_PRI_RATE_DET_TARGET1_CONFIG_OUT(x, v)            \
                out_dword(HWIO_LPASS_LPAIF_PRI_RATE_DET_TARGET1_CONFIG_ADDR(x),v)
#define HWIO_LPASS_LPAIF_PRI_RATE_DET_TARGET1_CONFIG_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_LPAIF_PRI_RATE_DET_TARGET1_CONFIG_ADDR(x),m,v,HWIO_LPASS_LPAIF_PRI_RATE_DET_TARGET1_CONFIG_IN(x))
#define HWIO_LPASS_LPAIF_PRI_RATE_DET_TARGET1_CONFIG_TARGET_128KHZ_BMSK             0xffff0000
#define HWIO_LPASS_LPAIF_PRI_RATE_DET_TARGET1_CONFIG_TARGET_128KHZ_SHFT                     16
#define HWIO_LPASS_LPAIF_PRI_RATE_DET_TARGET1_CONFIG_TARGET_176P4KHZ_BMSK               0xffff
#define HWIO_LPASS_LPAIF_PRI_RATE_DET_TARGET1_CONFIG_TARGET_176P4KHZ_SHFT                    0

#define HWIO_LPASS_LPAIF_PRI_RATE_DET_TARGET2_CONFIG_ADDR(x)                        ((x) + 0x22008)
#define HWIO_LPASS_LPAIF_PRI_RATE_DET_TARGET2_CONFIG_OFFS                           (0x22008)
#define HWIO_LPASS_LPAIF_PRI_RATE_DET_TARGET2_CONFIG_RMSK                               0xffff
#define HWIO_LPASS_LPAIF_PRI_RATE_DET_TARGET2_CONFIG_IN(x)            \
                in_dword(HWIO_LPASS_LPAIF_PRI_RATE_DET_TARGET2_CONFIG_ADDR(x))
#define HWIO_LPASS_LPAIF_PRI_RATE_DET_TARGET2_CONFIG_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPAIF_PRI_RATE_DET_TARGET2_CONFIG_ADDR(x), m)
#define HWIO_LPASS_LPAIF_PRI_RATE_DET_TARGET2_CONFIG_OUT(x, v)            \
                out_dword(HWIO_LPASS_LPAIF_PRI_RATE_DET_TARGET2_CONFIG_ADDR(x),v)
#define HWIO_LPASS_LPAIF_PRI_RATE_DET_TARGET2_CONFIG_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_LPAIF_PRI_RATE_DET_TARGET2_CONFIG_ADDR(x),m,v,HWIO_LPASS_LPAIF_PRI_RATE_DET_TARGET2_CONFIG_IN(x))
#define HWIO_LPASS_LPAIF_PRI_RATE_DET_TARGET2_CONFIG_TARGET_192KHZ_BMSK                 0xffff
#define HWIO_LPASS_LPAIF_PRI_RATE_DET_TARGET2_CONFIG_TARGET_192KHZ_SHFT                      0

#define HWIO_LPASS_LPAIF_PRI_RATE_BIN_ADDR(x)                                       ((x) + 0x2200c)
#define HWIO_LPASS_LPAIF_PRI_RATE_BIN_OFFS                                          (0x2200c)
#define HWIO_LPASS_LPAIF_PRI_RATE_BIN_RMSK                                              0x7fff
#define HWIO_LPASS_LPAIF_PRI_RATE_BIN_IN(x)            \
                in_dword(HWIO_LPASS_LPAIF_PRI_RATE_BIN_ADDR(x))
#define HWIO_LPASS_LPAIF_PRI_RATE_BIN_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPAIF_PRI_RATE_BIN_ADDR(x), m)
#define HWIO_LPASS_LPAIF_PRI_RATE_BIN_RATE_BIN_BMSK                                     0x7fff
#define HWIO_LPASS_LPAIF_PRI_RATE_BIN_RATE_BIN_SHFT                                          0

#define HWIO_LPASS_LPAIF_PRI_STC_DIFF_ADDR(x)                                       ((x) + 0x22010)
#define HWIO_LPASS_LPAIF_PRI_STC_DIFF_OFFS                                          (0x22010)
#define HWIO_LPASS_LPAIF_PRI_STC_DIFF_RMSK                                              0xffff
#define HWIO_LPASS_LPAIF_PRI_STC_DIFF_IN(x)            \
                in_dword(HWIO_LPASS_LPAIF_PRI_STC_DIFF_ADDR(x))
#define HWIO_LPASS_LPAIF_PRI_STC_DIFF_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPAIF_PRI_STC_DIFF_ADDR(x), m)
#define HWIO_LPASS_LPAIF_PRI_STC_DIFF_STC_DIFF_BMSK                                     0xffff
#define HWIO_LPASS_LPAIF_PRI_STC_DIFF_STC_DIFF_SHFT                                          0

#define HWIO_LPASS_LPAIF_PRI_RATE_DET_SEL_ADDR(x)                                   ((x) + 0x22014)
#define HWIO_LPASS_LPAIF_PRI_RATE_DET_SEL_OFFS                                      (0x22014)
#define HWIO_LPASS_LPAIF_PRI_RATE_DET_SEL_RMSK                                            0x1f
#define HWIO_LPASS_LPAIF_PRI_RATE_DET_SEL_IN(x)            \
                in_dword(HWIO_LPASS_LPAIF_PRI_RATE_DET_SEL_ADDR(x))
#define HWIO_LPASS_LPAIF_PRI_RATE_DET_SEL_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPAIF_PRI_RATE_DET_SEL_ADDR(x), m)
#define HWIO_LPASS_LPAIF_PRI_RATE_DET_SEL_OUT(x, v)            \
                out_dword(HWIO_LPASS_LPAIF_PRI_RATE_DET_SEL_ADDR(x),v)
#define HWIO_LPASS_LPAIF_PRI_RATE_DET_SEL_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_LPAIF_PRI_RATE_DET_SEL_ADDR(x),m,v,HWIO_LPASS_LPAIF_PRI_RATE_DET_SEL_IN(x))
#define HWIO_LPASS_LPAIF_PRI_RATE_DET_SEL_SYNC_SEL_BMSK                                   0x1f
#define HWIO_LPASS_LPAIF_PRI_RATE_DET_SEL_SYNC_SEL_SHFT                                      0
#define HWIO_LPASS_LPAIF_PRI_RATE_DET_SEL_SYNC_SEL_QUI_SRC_FVAL                            0x0
#define HWIO_LPASS_LPAIF_PRI_RATE_DET_SEL_SYNC_SEL_PRI_SRC_FVAL                            0x1
#define HWIO_LPASS_LPAIF_PRI_RATE_DET_SEL_SYNC_SEL_SEC_SRC_FVAL                            0x2
#define HWIO_LPASS_LPAIF_PRI_RATE_DET_SEL_SYNC_SEL_TER_SRC_FVAL                            0x3
#define HWIO_LPASS_LPAIF_PRI_RATE_DET_SEL_SYNC_SEL_QUA_SRC_FVAL                            0x4
#define HWIO_LPASS_LPAIF_PRI_RATE_DET_SEL_SYNC_SEL_CODEC_TX0_SRC_FVAL                      0x5
#define HWIO_LPASS_LPAIF_PRI_RATE_DET_SEL_SYNC_SEL_CODEC_TX1_SRC_FVAL                      0x6
#define HWIO_LPASS_LPAIF_PRI_RATE_DET_SEL_SYNC_SEL_CODEC_TX2_SRC_FVAL                      0x7
#define HWIO_LPASS_LPAIF_PRI_RATE_DET_SEL_SYNC_SEL_CODEC_TX3_SRC_FVAL                      0x8
#define HWIO_LPASS_LPAIF_PRI_RATE_DET_SEL_SYNC_SEL_CODEC_TX4_SRC_FVAL                      0x9
#define HWIO_LPASS_LPAIF_PRI_RATE_DET_SEL_SYNC_SEL_CODEC_TX5_SRC_FVAL                      0xa
#define HWIO_LPASS_LPAIF_PRI_RATE_DET_SEL_SYNC_SEL_CODEC_TX6_SRC_FVAL                      0xb
#define HWIO_LPASS_LPAIF_PRI_RATE_DET_SEL_SYNC_SEL_CODEC_TX7_SRC_FVAL                      0xc
#define HWIO_LPASS_LPAIF_PRI_RATE_DET_SEL_SYNC_SEL_CODEC_TX8_SRC_FVAL                      0xd
#define HWIO_LPASS_LPAIF_PRI_RATE_DET_SEL_SYNC_SEL_CODEC_TX9_SRC_FVAL                      0xe
#define HWIO_LPASS_LPAIF_PRI_RATE_DET_SEL_SYNC_SEL_CODEC_RX0_SRC_FVAL                      0xf
#define HWIO_LPASS_LPAIF_PRI_RATE_DET_SEL_SYNC_SEL_CODEC_RX1_SRC_FVAL                     0x10
#define HWIO_LPASS_LPAIF_PRI_RATE_DET_SEL_SYNC_SEL_CODEC_RX2_SRC_FVAL                     0x11
#define HWIO_LPASS_LPAIF_PRI_RATE_DET_SEL_SYNC_SEL_CODEC_RX3_SRC_FVAL                     0x12
#define HWIO_LPASS_LPAIF_PRI_RATE_DET_SEL_SYNC_SEL_CODEC_RX4_SRC_FVAL                     0x13
#define HWIO_LPASS_LPAIF_PRI_RATE_DET_SEL_SYNC_SEL_CODEC_RX5_SRC_FVAL                     0x14
#define HWIO_LPASS_LPAIF_PRI_RATE_DET_SEL_SYNC_SEL_CODEC_RX6_SRC_FVAL                     0x15
#define HWIO_LPASS_LPAIF_PRI_RATE_DET_SEL_SYNC_SEL_CODEC_RX7_SRC_FVAL                     0x16
#define HWIO_LPASS_LPAIF_PRI_RATE_DET_SEL_SYNC_SEL_CODEC_RX8_SRC_FVAL                     0x17
#define HWIO_LPASS_LPAIF_PRI_RATE_DET_SEL_SYNC_SEL_PRI_SPDIFRX_SRC_FVAL                   0x18
#define HWIO_LPASS_LPAIF_PRI_RATE_DET_SEL_SYNC_SEL_SEC_SPDIFRX_SRC_FVAL                   0x19

#define HWIO_LPASS_LPAIF_SEC_RATE_DET_CONFIG_ADDR(x)                                ((x) + 0x23000)
#define HWIO_LPASS_LPAIF_SEC_RATE_DET_CONFIG_OFFS                                   (0x23000)
#define HWIO_LPASS_LPAIF_SEC_RATE_DET_CONFIG_RMSK                                   0xfffffff1
#define HWIO_LPASS_LPAIF_SEC_RATE_DET_CONFIG_IN(x)            \
                in_dword(HWIO_LPASS_LPAIF_SEC_RATE_DET_CONFIG_ADDR(x))
#define HWIO_LPASS_LPAIF_SEC_RATE_DET_CONFIG_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPAIF_SEC_RATE_DET_CONFIG_ADDR(x), m)
#define HWIO_LPASS_LPAIF_SEC_RATE_DET_CONFIG_OUT(x, v)            \
                out_dword(HWIO_LPASS_LPAIF_SEC_RATE_DET_CONFIG_ADDR(x),v)
#define HWIO_LPASS_LPAIF_SEC_RATE_DET_CONFIG_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_LPAIF_SEC_RATE_DET_CONFIG_ADDR(x),m,v,HWIO_LPASS_LPAIF_SEC_RATE_DET_CONFIG_IN(x))
#define HWIO_LPASS_LPAIF_SEC_RATE_DET_CONFIG_RESET_BMSK                             0x80000000
#define HWIO_LPASS_LPAIF_SEC_RATE_DET_CONFIG_RESET_SHFT                                     31
#define HWIO_LPASS_LPAIF_SEC_RATE_DET_CONFIG_RESET_DISABLE_FVAL                            0x0
#define HWIO_LPASS_LPAIF_SEC_RATE_DET_CONFIG_RESET_ENABLE_FVAL                             0x1
#define HWIO_LPASS_LPAIF_SEC_RATE_DET_CONFIG_VAR_192K_1764K_BMSK                    0x7f800000
#define HWIO_LPASS_LPAIF_SEC_RATE_DET_CONFIG_VAR_192K_1764K_SHFT                            23
#define HWIO_LPASS_LPAIF_SEC_RATE_DET_CONFIG_VAR_128K_441K_BMSK                       0x7f8000
#define HWIO_LPASS_LPAIF_SEC_RATE_DET_CONFIG_VAR_128K_441K_SHFT                             15
#define HWIO_LPASS_LPAIF_SEC_RATE_DET_CONFIG_VAR_32K_8K_BMSK                            0x7f80
#define HWIO_LPASS_LPAIF_SEC_RATE_DET_CONFIG_VAR_32K_8K_SHFT                                 7
#define HWIO_LPASS_LPAIF_SEC_RATE_DET_CONFIG_NUM_FS_BMSK                                  0x70
#define HWIO_LPASS_LPAIF_SEC_RATE_DET_CONFIG_NUM_FS_SHFT                                     4
#define HWIO_LPASS_LPAIF_SEC_RATE_DET_CONFIG_NUM_FS_ONE_FS_FVAL                            0x0
#define HWIO_LPASS_LPAIF_SEC_RATE_DET_CONFIG_NUM_FS_TWO_FS_FVAL                            0x1
#define HWIO_LPASS_LPAIF_SEC_RATE_DET_CONFIG_NUM_FS_THREE_FS_FVAL                          0x2
#define HWIO_LPASS_LPAIF_SEC_RATE_DET_CONFIG_NUM_FS_FOUR_FS_FVAL                           0x3
#define HWIO_LPASS_LPAIF_SEC_RATE_DET_CONFIG_NUM_FS_FIVE_FS_FVAL                           0x4
#define HWIO_LPASS_LPAIF_SEC_RATE_DET_CONFIG_NUM_FS_SIX_FS_FVAL                            0x5
#define HWIO_LPASS_LPAIF_SEC_RATE_DET_CONFIG_NUM_FS_SEVEN_FS_FVAL                          0x6
#define HWIO_LPASS_LPAIF_SEC_RATE_DET_CONFIG_NUM_FS_EIGHT_FS_FVAL                          0x7
#define HWIO_LPASS_LPAIF_SEC_RATE_DET_CONFIG_EN_BMSK                                       0x1
#define HWIO_LPASS_LPAIF_SEC_RATE_DET_CONFIG_EN_SHFT                                         0

#define HWIO_LPASS_LPAIF_SEC_RATE_DET_TARGET1_CONFIG_ADDR(x)                        ((x) + 0x23004)
#define HWIO_LPASS_LPAIF_SEC_RATE_DET_TARGET1_CONFIG_OFFS                           (0x23004)
#define HWIO_LPASS_LPAIF_SEC_RATE_DET_TARGET1_CONFIG_RMSK                           0xffffffff
#define HWIO_LPASS_LPAIF_SEC_RATE_DET_TARGET1_CONFIG_IN(x)            \
                in_dword(HWIO_LPASS_LPAIF_SEC_RATE_DET_TARGET1_CONFIG_ADDR(x))
#define HWIO_LPASS_LPAIF_SEC_RATE_DET_TARGET1_CONFIG_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPAIF_SEC_RATE_DET_TARGET1_CONFIG_ADDR(x), m)
#define HWIO_LPASS_LPAIF_SEC_RATE_DET_TARGET1_CONFIG_OUT(x, v)            \
                out_dword(HWIO_LPASS_LPAIF_SEC_RATE_DET_TARGET1_CONFIG_ADDR(x),v)
#define HWIO_LPASS_LPAIF_SEC_RATE_DET_TARGET1_CONFIG_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_LPAIF_SEC_RATE_DET_TARGET1_CONFIG_ADDR(x),m,v,HWIO_LPASS_LPAIF_SEC_RATE_DET_TARGET1_CONFIG_IN(x))
#define HWIO_LPASS_LPAIF_SEC_RATE_DET_TARGET1_CONFIG_TARGET_128KHZ_BMSK             0xffff0000
#define HWIO_LPASS_LPAIF_SEC_RATE_DET_TARGET1_CONFIG_TARGET_128KHZ_SHFT                     16
#define HWIO_LPASS_LPAIF_SEC_RATE_DET_TARGET1_CONFIG_TARGET_176P4KHZ_BMSK               0xffff
#define HWIO_LPASS_LPAIF_SEC_RATE_DET_TARGET1_CONFIG_TARGET_176P4KHZ_SHFT                    0

#define HWIO_LPASS_LPAIF_SEC_RATE_DET_TARGET2_CONFIG_ADDR(x)                        ((x) + 0x23008)
#define HWIO_LPASS_LPAIF_SEC_RATE_DET_TARGET2_CONFIG_OFFS                           (0x23008)
#define HWIO_LPASS_LPAIF_SEC_RATE_DET_TARGET2_CONFIG_RMSK                               0xffff
#define HWIO_LPASS_LPAIF_SEC_RATE_DET_TARGET2_CONFIG_IN(x)            \
                in_dword(HWIO_LPASS_LPAIF_SEC_RATE_DET_TARGET2_CONFIG_ADDR(x))
#define HWIO_LPASS_LPAIF_SEC_RATE_DET_TARGET2_CONFIG_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPAIF_SEC_RATE_DET_TARGET2_CONFIG_ADDR(x), m)
#define HWIO_LPASS_LPAIF_SEC_RATE_DET_TARGET2_CONFIG_OUT(x, v)            \
                out_dword(HWIO_LPASS_LPAIF_SEC_RATE_DET_TARGET2_CONFIG_ADDR(x),v)
#define HWIO_LPASS_LPAIF_SEC_RATE_DET_TARGET2_CONFIG_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_LPAIF_SEC_RATE_DET_TARGET2_CONFIG_ADDR(x),m,v,HWIO_LPASS_LPAIF_SEC_RATE_DET_TARGET2_CONFIG_IN(x))
#define HWIO_LPASS_LPAIF_SEC_RATE_DET_TARGET2_CONFIG_TARGET_192KHZ_BMSK                 0xffff
#define HWIO_LPASS_LPAIF_SEC_RATE_DET_TARGET2_CONFIG_TARGET_192KHZ_SHFT                      0

#define HWIO_LPASS_LPAIF_SEC_RATE_BIN_ADDR(x)                                       ((x) + 0x2300c)
#define HWIO_LPASS_LPAIF_SEC_RATE_BIN_OFFS                                          (0x2300c)
#define HWIO_LPASS_LPAIF_SEC_RATE_BIN_RMSK                                              0x7fff
#define HWIO_LPASS_LPAIF_SEC_RATE_BIN_IN(x)            \
                in_dword(HWIO_LPASS_LPAIF_SEC_RATE_BIN_ADDR(x))
#define HWIO_LPASS_LPAIF_SEC_RATE_BIN_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPAIF_SEC_RATE_BIN_ADDR(x), m)
#define HWIO_LPASS_LPAIF_SEC_RATE_BIN_RATE_BIN_BMSK                                     0x7fff
#define HWIO_LPASS_LPAIF_SEC_RATE_BIN_RATE_BIN_SHFT                                          0

#define HWIO_LPASS_LPAIF_SEC_STC_DIFF_ADDR(x)                                       ((x) + 0x23010)
#define HWIO_LPASS_LPAIF_SEC_STC_DIFF_OFFS                                          (0x23010)
#define HWIO_LPASS_LPAIF_SEC_STC_DIFF_RMSK                                              0xffff
#define HWIO_LPASS_LPAIF_SEC_STC_DIFF_IN(x)            \
                in_dword(HWIO_LPASS_LPAIF_SEC_STC_DIFF_ADDR(x))
#define HWIO_LPASS_LPAIF_SEC_STC_DIFF_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPAIF_SEC_STC_DIFF_ADDR(x), m)
#define HWIO_LPASS_LPAIF_SEC_STC_DIFF_STC_DIFF_BMSK                                     0xffff
#define HWIO_LPASS_LPAIF_SEC_STC_DIFF_STC_DIFF_SHFT                                          0

#define HWIO_LPASS_LPAIF_SEC_RATE_DET_SEL_ADDR(x)                                   ((x) + 0x23014)
#define HWIO_LPASS_LPAIF_SEC_RATE_DET_SEL_OFFS                                      (0x23014)
#define HWIO_LPASS_LPAIF_SEC_RATE_DET_SEL_RMSK                                            0x1f
#define HWIO_LPASS_LPAIF_SEC_RATE_DET_SEL_IN(x)            \
                in_dword(HWIO_LPASS_LPAIF_SEC_RATE_DET_SEL_ADDR(x))
#define HWIO_LPASS_LPAIF_SEC_RATE_DET_SEL_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPAIF_SEC_RATE_DET_SEL_ADDR(x), m)
#define HWIO_LPASS_LPAIF_SEC_RATE_DET_SEL_OUT(x, v)            \
                out_dword(HWIO_LPASS_LPAIF_SEC_RATE_DET_SEL_ADDR(x),v)
#define HWIO_LPASS_LPAIF_SEC_RATE_DET_SEL_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_LPAIF_SEC_RATE_DET_SEL_ADDR(x),m,v,HWIO_LPASS_LPAIF_SEC_RATE_DET_SEL_IN(x))
#define HWIO_LPASS_LPAIF_SEC_RATE_DET_SEL_SYNC_SEL_BMSK                                   0x1f
#define HWIO_LPASS_LPAIF_SEC_RATE_DET_SEL_SYNC_SEL_SHFT                                      0
#define HWIO_LPASS_LPAIF_SEC_RATE_DET_SEL_SYNC_SEL_QUI_SRC_FVAL                            0x0
#define HWIO_LPASS_LPAIF_SEC_RATE_DET_SEL_SYNC_SEL_PRI_SRC_FVAL                            0x1
#define HWIO_LPASS_LPAIF_SEC_RATE_DET_SEL_SYNC_SEL_SEC_SRC_FVAL                            0x2
#define HWIO_LPASS_LPAIF_SEC_RATE_DET_SEL_SYNC_SEL_TER_SRC_FVAL                            0x3
#define HWIO_LPASS_LPAIF_SEC_RATE_DET_SEL_SYNC_SEL_QUA_SRC_FVAL                            0x4
#define HWIO_LPASS_LPAIF_SEC_RATE_DET_SEL_SYNC_SEL_CODEC_TX0_SRC_FVAL                      0x5
#define HWIO_LPASS_LPAIF_SEC_RATE_DET_SEL_SYNC_SEL_CODEC_TX1_SRC_FVAL                      0x6
#define HWIO_LPASS_LPAIF_SEC_RATE_DET_SEL_SYNC_SEL_CODEC_TX2_SRC_FVAL                      0x7
#define HWIO_LPASS_LPAIF_SEC_RATE_DET_SEL_SYNC_SEL_CODEC_TX3_SRC_FVAL                      0x8
#define HWIO_LPASS_LPAIF_SEC_RATE_DET_SEL_SYNC_SEL_CODEC_TX4_SRC_FVAL                      0x9
#define HWIO_LPASS_LPAIF_SEC_RATE_DET_SEL_SYNC_SEL_CODEC_TX5_SRC_FVAL                      0xa
#define HWIO_LPASS_LPAIF_SEC_RATE_DET_SEL_SYNC_SEL_CODEC_TX6_SRC_FVAL                      0xb
#define HWIO_LPASS_LPAIF_SEC_RATE_DET_SEL_SYNC_SEL_CODEC_TX7_SRC_FVAL                      0xc
#define HWIO_LPASS_LPAIF_SEC_RATE_DET_SEL_SYNC_SEL_CODEC_TX8_SRC_FVAL                      0xd
#define HWIO_LPASS_LPAIF_SEC_RATE_DET_SEL_SYNC_SEL_CODEC_TX9_SRC_FVAL                      0xe
#define HWIO_LPASS_LPAIF_SEC_RATE_DET_SEL_SYNC_SEL_CODEC_RX0_SRC_FVAL                      0xf
#define HWIO_LPASS_LPAIF_SEC_RATE_DET_SEL_SYNC_SEL_CODEC_RX1_SRC_FVAL                     0x10
#define HWIO_LPASS_LPAIF_SEC_RATE_DET_SEL_SYNC_SEL_CODEC_RX2_SRC_FVAL                     0x11
#define HWIO_LPASS_LPAIF_SEC_RATE_DET_SEL_SYNC_SEL_CODEC_RX3_SRC_FVAL                     0x12
#define HWIO_LPASS_LPAIF_SEC_RATE_DET_SEL_SYNC_SEL_CODEC_RX4_SRC_FVAL                     0x13
#define HWIO_LPASS_LPAIF_SEC_RATE_DET_SEL_SYNC_SEL_CODEC_RX5_SRC_FVAL                     0x14
#define HWIO_LPASS_LPAIF_SEC_RATE_DET_SEL_SYNC_SEL_CODEC_RX6_SRC_FVAL                     0x15
#define HWIO_LPASS_LPAIF_SEC_RATE_DET_SEL_SYNC_SEL_CODEC_RX7_SRC_FVAL                     0x16
#define HWIO_LPASS_LPAIF_SEC_RATE_DET_SEL_SYNC_SEL_CODEC_RX8_SRC_FVAL                     0x17
#define HWIO_LPASS_LPAIF_SEC_RATE_DET_SEL_SYNC_SEL_PRI_SPDIFRX_SRC_FVAL                   0x18
#define HWIO_LPASS_LPAIF_SEC_RATE_DET_SEL_SYNC_SEL_SEC_SPDIFRX_SRC_FVAL                   0x19

#define HWIO_LPASS_LPAIF_EXT_WRDMA_INTF_ADDR(x)                                     ((x) + 0x24000)
#define HWIO_LPASS_LPAIF_EXT_WRDMA_INTF_OFFS                                        (0x24000)
#define HWIO_LPASS_LPAIF_EXT_WRDMA_INTF_RMSK                                              0x1f
#define HWIO_LPASS_LPAIF_EXT_WRDMA_INTF_IN(x)            \
                in_dword(HWIO_LPASS_LPAIF_EXT_WRDMA_INTF_ADDR(x))
#define HWIO_LPASS_LPAIF_EXT_WRDMA_INTF_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPAIF_EXT_WRDMA_INTF_ADDR(x), m)
#define HWIO_LPASS_LPAIF_EXT_WRDMA_INTF_OUT(x, v)            \
                out_dword(HWIO_LPASS_LPAIF_EXT_WRDMA_INTF_ADDR(x),v)
#define HWIO_LPASS_LPAIF_EXT_WRDMA_INTF_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_LPAIF_EXT_WRDMA_INTF_ADDR(x),m,v,HWIO_LPASS_LPAIF_EXT_WRDMA_INTF_IN(x))
#define HWIO_LPASS_LPAIF_EXT_WRDMA_INTF_AUDIO_INTF_BMSK                                   0x1f
#define HWIO_LPASS_LPAIF_EXT_WRDMA_INTF_AUDIO_INTF_SHFT                                      0
#define HWIO_LPASS_LPAIF_EXT_WRDMA_INTF_AUDIO_INTF_NONE_FVAL                               0x0
#define HWIO_LPASS_LPAIF_EXT_WRDMA_INTF_AUDIO_INTF_PRI_SRC_FVAL                            0x1
#define HWIO_LPASS_LPAIF_EXT_WRDMA_INTF_AUDIO_INTF_SEC_SRC_FVAL                            0x2
#define HWIO_LPASS_LPAIF_EXT_WRDMA_INTF_AUDIO_INTF_TER_SRC_FVAL                            0x3
#define HWIO_LPASS_LPAIF_EXT_WRDMA_INTF_AUDIO_INTF_QUA_SRC_FVAL                            0x4
#define HWIO_LPASS_LPAIF_EXT_WRDMA_INTF_AUDIO_INTF_QUI_SRC_FVAL                            0x5
#define HWIO_LPASS_LPAIF_EXT_WRDMA_INTF_AUDIO_INTF_CODEC_TX0_SRC_FVAL                      0x6
#define HWIO_LPASS_LPAIF_EXT_WRDMA_INTF_AUDIO_INTF_CODEC_TX1_SRC_FVAL                      0x7
#define HWIO_LPASS_LPAIF_EXT_WRDMA_INTF_AUDIO_INTF_CODEC_TX2_SRC_FVAL                      0x8
#define HWIO_LPASS_LPAIF_EXT_WRDMA_INTF_AUDIO_INTF_CODEC_TX3_SRC_FVAL                      0x9
#define HWIO_LPASS_LPAIF_EXT_WRDMA_INTF_AUDIO_INTF_CODEC_TX4_SRC_FVAL                      0xa
#define HWIO_LPASS_LPAIF_EXT_WRDMA_INTF_AUDIO_INTF_CODEC_TX5_SRC_FVAL                      0xb
#define HWIO_LPASS_LPAIF_EXT_WRDMA_INTF_AUDIO_INTF_CODEC_TX6_SRC_FVAL                      0xc
#define HWIO_LPASS_LPAIF_EXT_WRDMA_INTF_AUDIO_INTF_CODEC_TX7_SRC_FVAL                      0xd
#define HWIO_LPASS_LPAIF_EXT_WRDMA_INTF_AUDIO_INTF_CODEC_TX8_SRC_FVAL                      0xe
#define HWIO_LPASS_LPAIF_EXT_WRDMA_INTF_AUDIO_INTF_PRI_SPDIFRX_SRC_FVAL                    0xf
#define HWIO_LPASS_LPAIF_EXT_WRDMA_INTF_AUDIO_INTF_SEC_SPDIFRX_SRC_FVAL                   0x10
#define HWIO_LPASS_LPAIF_EXT_WRDMA_INTF_AUDIO_INTF_RDDMA_CH0_FVAL                         0x11
#define HWIO_LPASS_LPAIF_EXT_WRDMA_INTF_AUDIO_INTF_RDDMA_CH1_FVAL                         0x12
#define HWIO_LPASS_LPAIF_EXT_WRDMA_INTF_AUDIO_INTF_RDDMA_CH2_FVAL                         0x13
#define HWIO_LPASS_LPAIF_EXT_WRDMA_INTF_AUDIO_INTF_RDDMA_CH3_FVAL                         0x14
#define HWIO_LPASS_LPAIF_EXT_WRDMA_INTF_AUDIO_INTF_RDDMA_CH4_FVAL                         0x15
#define HWIO_LPASS_LPAIF_EXT_WRDMA_INTF_AUDIO_INTF_RDDMA_CH5_FVAL                         0x16
#define HWIO_LPASS_LPAIF_EXT_WRDMA_INTF_AUDIO_INTF_RDDMA_CH6_FVAL                         0x17
#define HWIO_LPASS_LPAIF_EXT_WRDMA_INTF_AUDIO_INTF_RDDMA_CH7_FVAL                         0x18
#define HWIO_LPASS_LPAIF_EXT_WRDMA_INTF_AUDIO_INTF_RDDMA_CH8_FVAL                         0x19
#define HWIO_LPASS_LPAIF_EXT_WRDMA_INTF_AUDIO_INTF_RDDMA_CH9_FVAL                         0x1a
#define HWIO_LPASS_LPAIF_EXT_WRDMA_INTF_AUDIO_INTF_RDDMA_CH10_FVAL                        0x1b
#define HWIO_LPASS_LPAIF_EXT_WRDMA_INTF_AUDIO_INTF_RDDMA_CH11_FVAL                        0x1c

#define HWIO_LPASS_LPAIF_EXT_WRDMA_CODEC_INTF_ADDR(x)                               ((x) + 0x24004)
#define HWIO_LPASS_LPAIF_EXT_WRDMA_CODEC_INTF_OFFS                                  (0x24004)
#define HWIO_LPASS_LPAIF_EXT_WRDMA_CODEC_INTF_RMSK                                  0xefe000ff
#define HWIO_LPASS_LPAIF_EXT_WRDMA_CODEC_INTF_IN(x)            \
                in_dword(HWIO_LPASS_LPAIF_EXT_WRDMA_CODEC_INTF_ADDR(x))
#define HWIO_LPASS_LPAIF_EXT_WRDMA_CODEC_INTF_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPAIF_EXT_WRDMA_CODEC_INTF_ADDR(x), m)
#define HWIO_LPASS_LPAIF_EXT_WRDMA_CODEC_INTF_OUT(x, v)            \
                out_dword(HWIO_LPASS_LPAIF_EXT_WRDMA_CODEC_INTF_ADDR(x),v)
#define HWIO_LPASS_LPAIF_EXT_WRDMA_CODEC_INTF_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_LPAIF_EXT_WRDMA_CODEC_INTF_ADDR(x),m,v,HWIO_LPASS_LPAIF_EXT_WRDMA_CODEC_INTF_IN(x))
#define HWIO_LPASS_LPAIF_EXT_WRDMA_CODEC_INTF_RESET_BMSK                            0x80000000
#define HWIO_LPASS_LPAIF_EXT_WRDMA_CODEC_INTF_RESET_SHFT                                    31
#define HWIO_LPASS_LPAIF_EXT_WRDMA_CODEC_INTF_ENABLE_BMSK                           0x40000000
#define HWIO_LPASS_LPAIF_EXT_WRDMA_CODEC_INTF_ENABLE_SHFT                                   30
#define HWIO_LPASS_LPAIF_EXT_WRDMA_CODEC_INTF_ENABLE_16BIT_UNPACKING_BMSK           0x20000000
#define HWIO_LPASS_LPAIF_EXT_WRDMA_CODEC_INTF_ENABLE_16BIT_UNPACKING_SHFT                   29
#define HWIO_LPASS_LPAIF_EXT_WRDMA_CODEC_INTF_ENABLE_16BIT_UNPACKING_DISABLE_FVAL          0x0
#define HWIO_LPASS_LPAIF_EXT_WRDMA_CODEC_INTF_ENABLE_16BIT_UNPACKING_ENABLE_FVAL           0x1
#define HWIO_LPASS_LPAIF_EXT_WRDMA_CODEC_INTF_CODEC_FS_SEL_BMSK                      0xe000000
#define HWIO_LPASS_LPAIF_EXT_WRDMA_CODEC_INTF_CODEC_FS_SEL_SHFT                             25
#define HWIO_LPASS_LPAIF_EXT_WRDMA_CODEC_INTF_CODEC_FS_DELAY_BMSK                    0x1e00000
#define HWIO_LPASS_LPAIF_EXT_WRDMA_CODEC_INTF_CODEC_FS_DELAY_SHFT                           21
#define HWIO_LPASS_LPAIF_EXT_WRDMA_CODEC_INTF_NUM_ACTIVE_CHANNEL_BMSK                     0xff
#define HWIO_LPASS_LPAIF_EXT_WRDMA_CODEC_INTF_NUM_ACTIVE_CHANNEL_SHFT                        0

#define HWIO_LPASS_LPAIF_INTF_GRP0_ADDR(x)                                          ((x) + 0x25000)
#define HWIO_LPASS_LPAIF_INTF_GRP0_OFFS                                             (0x25000)
#define HWIO_LPASS_LPAIF_INTF_GRP0_RMSK                                                  0x11f
#define HWIO_LPASS_LPAIF_INTF_GRP0_IN(x)            \
                in_dword(HWIO_LPASS_LPAIF_INTF_GRP0_ADDR(x))
#define HWIO_LPASS_LPAIF_INTF_GRP0_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPAIF_INTF_GRP0_ADDR(x), m)
#define HWIO_LPASS_LPAIF_INTF_GRP0_OUT(x, v)            \
                out_dword(HWIO_LPASS_LPAIF_INTF_GRP0_ADDR(x),v)
#define HWIO_LPASS_LPAIF_INTF_GRP0_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_LPAIF_INTF_GRP0_ADDR(x),m,v,HWIO_LPASS_LPAIF_INTF_GRP0_IN(x))
#define HWIO_LPASS_LPAIF_INTF_GRP0_ENABLE_BMSK                                           0x100
#define HWIO_LPASS_LPAIF_INTF_GRP0_ENABLE_SHFT                                               8
#define HWIO_LPASS_LPAIF_INTF_GRP0_ENABLE_DISABLE_FVAL                                     0x0
#define HWIO_LPASS_LPAIF_INTF_GRP0_ENABLE_ENABLE_FVAL                                      0x1
#define HWIO_LPASS_LPAIF_INTF_GRP0_QUI_SEL_BMSK                                           0x10
#define HWIO_LPASS_LPAIF_INTF_GRP0_QUI_SEL_SHFT                                              4
#define HWIO_LPASS_LPAIF_INTF_GRP0_QUI_SEL_DISABLE_FVAL                                    0x0
#define HWIO_LPASS_LPAIF_INTF_GRP0_QUI_SEL_ENABLE_FVAL                                     0x1
#define HWIO_LPASS_LPAIF_INTF_GRP0_QUA_SEL_BMSK                                            0x8
#define HWIO_LPASS_LPAIF_INTF_GRP0_QUA_SEL_SHFT                                              3
#define HWIO_LPASS_LPAIF_INTF_GRP0_QUA_SEL_DISABLE_FVAL                                    0x0
#define HWIO_LPASS_LPAIF_INTF_GRP0_QUA_SEL_ENABLE_FVAL                                     0x1
#define HWIO_LPASS_LPAIF_INTF_GRP0_TER_SEL_BMSK                                            0x4
#define HWIO_LPASS_LPAIF_INTF_GRP0_TER_SEL_SHFT                                              2
#define HWIO_LPASS_LPAIF_INTF_GRP0_TER_SEL_DISABLE_FVAL                                    0x0
#define HWIO_LPASS_LPAIF_INTF_GRP0_TER_SEL_ENABLE_FVAL                                     0x1
#define HWIO_LPASS_LPAIF_INTF_GRP0_SEC_SEL_BMSK                                            0x2
#define HWIO_LPASS_LPAIF_INTF_GRP0_SEC_SEL_SHFT                                              1
#define HWIO_LPASS_LPAIF_INTF_GRP0_SEC_SEL_DISABLE_FVAL                                    0x0
#define HWIO_LPASS_LPAIF_INTF_GRP0_SEC_SEL_ENABLE_FVAL                                     0x1
#define HWIO_LPASS_LPAIF_INTF_GRP0_PRI_SEL_BMSK                                            0x1
#define HWIO_LPASS_LPAIF_INTF_GRP0_PRI_SEL_SHFT                                              0
#define HWIO_LPASS_LPAIF_INTF_GRP0_PRI_SEL_DISABLE_FVAL                                    0x0
#define HWIO_LPASS_LPAIF_INTF_GRP0_PRI_SEL_ENABLE_FVAL                                     0x1

#define HWIO_LPASS_LPAIF_INTF_GRP1_ADDR(x)                                          ((x) + 0x26000)
#define HWIO_LPASS_LPAIF_INTF_GRP1_OFFS                                             (0x26000)
#define HWIO_LPASS_LPAIF_INTF_GRP1_RMSK                                                  0x11f
#define HWIO_LPASS_LPAIF_INTF_GRP1_IN(x)            \
                in_dword(HWIO_LPASS_LPAIF_INTF_GRP1_ADDR(x))
#define HWIO_LPASS_LPAIF_INTF_GRP1_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPAIF_INTF_GRP1_ADDR(x), m)
#define HWIO_LPASS_LPAIF_INTF_GRP1_OUT(x, v)            \
                out_dword(HWIO_LPASS_LPAIF_INTF_GRP1_ADDR(x),v)
#define HWIO_LPASS_LPAIF_INTF_GRP1_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_LPAIF_INTF_GRP1_ADDR(x),m,v,HWIO_LPASS_LPAIF_INTF_GRP1_IN(x))
#define HWIO_LPASS_LPAIF_INTF_GRP1_ENABLE_BMSK                                           0x100
#define HWIO_LPASS_LPAIF_INTF_GRP1_ENABLE_SHFT                                               8
#define HWIO_LPASS_LPAIF_INTF_GRP1_ENABLE_DISABLE_FVAL                                     0x0
#define HWIO_LPASS_LPAIF_INTF_GRP1_ENABLE_ENABLE_FVAL                                      0x1
#define HWIO_LPASS_LPAIF_INTF_GRP1_QUI_SEL_BMSK                                           0x10
#define HWIO_LPASS_LPAIF_INTF_GRP1_QUI_SEL_SHFT                                              4
#define HWIO_LPASS_LPAIF_INTF_GRP1_QUI_SEL_DISABLE_FVAL                                    0x0
#define HWIO_LPASS_LPAIF_INTF_GRP1_QUI_SEL_ENABLE_FVAL                                     0x1
#define HWIO_LPASS_LPAIF_INTF_GRP1_QUA_SEL_BMSK                                            0x8
#define HWIO_LPASS_LPAIF_INTF_GRP1_QUA_SEL_SHFT                                              3
#define HWIO_LPASS_LPAIF_INTF_GRP1_QUA_SEL_DISABLE_FVAL                                    0x0
#define HWIO_LPASS_LPAIF_INTF_GRP1_QUA_SEL_ENABLE_FVAL                                     0x1
#define HWIO_LPASS_LPAIF_INTF_GRP1_TER_SEL_BMSK                                            0x4
#define HWIO_LPASS_LPAIF_INTF_GRP1_TER_SEL_SHFT                                              2
#define HWIO_LPASS_LPAIF_INTF_GRP1_TER_SEL_DISABLE_FVAL                                    0x0
#define HWIO_LPASS_LPAIF_INTF_GRP1_TER_SEL_ENABLE_FVAL                                     0x1
#define HWIO_LPASS_LPAIF_INTF_GRP1_SEC_SEL_BMSK                                            0x2
#define HWIO_LPASS_LPAIF_INTF_GRP1_SEC_SEL_SHFT                                              1
#define HWIO_LPASS_LPAIF_INTF_GRP1_SEC_SEL_DISABLE_FVAL                                    0x0
#define HWIO_LPASS_LPAIF_INTF_GRP1_SEC_SEL_ENABLE_FVAL                                     0x1
#define HWIO_LPASS_LPAIF_INTF_GRP1_PRI_SEL_BMSK                                            0x1
#define HWIO_LPASS_LPAIF_INTF_GRP1_PRI_SEL_SHFT                                              0
#define HWIO_LPASS_LPAIF_INTF_GRP1_PRI_SEL_DISABLE_FVAL                                    0x0
#define HWIO_LPASS_LPAIF_INTF_GRP1_PRI_SEL_ENABLE_FVAL                                     0x1

#define HWIO_LPASS_LPAIF_AXI_CONFIG_ADDR(x)                                         ((x) + 0x27000)
#define HWIO_LPASS_LPAIF_AXI_CONFIG_OFFS                                            (0x27000)
#define HWIO_LPASS_LPAIF_AXI_CONFIG_RMSK                                                   0x7
#define HWIO_LPASS_LPAIF_AXI_CONFIG_IN(x)            \
                in_dword(HWIO_LPASS_LPAIF_AXI_CONFIG_ADDR(x))
#define HWIO_LPASS_LPAIF_AXI_CONFIG_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPAIF_AXI_CONFIG_ADDR(x), m)
#define HWIO_LPASS_LPAIF_AXI_CONFIG_OUT(x, v)            \
                out_dword(HWIO_LPASS_LPAIF_AXI_CONFIG_ADDR(x),v)
#define HWIO_LPASS_LPAIF_AXI_CONFIG_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_LPAIF_AXI_CONFIG_ADDR(x),m,v,HWIO_LPASS_LPAIF_AXI_CONFIG_IN(x))
#define HWIO_LPASS_LPAIF_AXI_CONFIG_PRIORITYLVL_BMSK                                       0x6
#define HWIO_LPASS_LPAIF_AXI_CONFIG_PRIORITYLVL_SHFT                                         1
#define HWIO_LPASS_LPAIF_AXI_CONFIG_HALT_REQ_BMSK                                          0x1
#define HWIO_LPASS_LPAIF_AXI_CONFIG_HALT_REQ_SHFT                                            0

#define HWIO_LPASS_LPAIF_AXI_STATUS_ADDR(x)                                         ((x) + 0x27004)
#define HWIO_LPASS_LPAIF_AXI_STATUS_OFFS                                            (0x27004)
#define HWIO_LPASS_LPAIF_AXI_STATUS_RMSK                                                   0x1
#define HWIO_LPASS_LPAIF_AXI_STATUS_IN(x)            \
                in_dword(HWIO_LPASS_LPAIF_AXI_STATUS_ADDR(x))
#define HWIO_LPASS_LPAIF_AXI_STATUS_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPAIF_AXI_STATUS_ADDR(x), m)
#define HWIO_LPASS_LPAIF_AXI_STATUS_HALT_ACK_BMSK                                          0x1
#define HWIO_LPASS_LPAIF_AXI_STATUS_HALT_ACK_SHFT                                            0

#define HWIO_LPASS_LPAIF_IO_MUXCTL_ADDR(x)                                          ((x) + 0x28000)
#define HWIO_LPASS_LPAIF_IO_MUXCTL_OFFS                                             (0x28000)
#define HWIO_LPASS_LPAIF_IO_MUXCTL_RMSK                                                0x1003f
#define HWIO_LPASS_LPAIF_IO_MUXCTL_IN(x)            \
                in_dword(HWIO_LPASS_LPAIF_IO_MUXCTL_ADDR(x))
#define HWIO_LPASS_LPAIF_IO_MUXCTL_INM(x, m)            \
                in_dword_masked(HWIO_LPASS_LPAIF_IO_MUXCTL_ADDR(x), m)
#define HWIO_LPASS_LPAIF_IO_MUXCTL_OUT(x, v)            \
                out_dword(HWIO_LPASS_LPAIF_IO_MUXCTL_ADDR(x),v)
#define HWIO_LPASS_LPAIF_IO_MUXCTL_OUTM(x,m,v) \
                out_dword_masked_ns(HWIO_LPASS_LPAIF_IO_MUXCTL_ADDR(x),m,v,HWIO_LPASS_LPAIF_IO_MUXCTL_IN(x))
#define HWIO_LPASS_LPAIF_IO_MUXCTL_TX_SWR_PCM_SEL_BMSK                                 0x10000
#define HWIO_LPASS_LPAIF_IO_MUXCTL_TX_SWR_PCM_SEL_SHFT                                      16
#define HWIO_LPASS_LPAIF_IO_MUXCTL_RX5_SEL_BMSK                                           0x20
#define HWIO_LPASS_LPAIF_IO_MUXCTL_RX5_SEL_SHFT                                              5
#define HWIO_LPASS_LPAIF_IO_MUXCTL_RX4_SEL_BMSK                                           0x10
#define HWIO_LPASS_LPAIF_IO_MUXCTL_RX4_SEL_SHFT                                              4
#define HWIO_LPASS_LPAIF_IO_MUXCTL_RX3_SEL_BMSK                                            0x8
#define HWIO_LPASS_LPAIF_IO_MUXCTL_RX3_SEL_SHFT                                              3
#define HWIO_LPASS_LPAIF_IO_MUXCTL_RX2_SEL_BMSK                                            0x4
#define HWIO_LPASS_LPAIF_IO_MUXCTL_RX2_SEL_SHFT                                              2
#define HWIO_LPASS_LPAIF_IO_MUXCTL_RX1_SEL_BMSK                                            0x2
#define HWIO_LPASS_LPAIF_IO_MUXCTL_RX1_SEL_SHFT                                              1
#define HWIO_LPASS_LPAIF_IO_MUXCTL_RX0_SEL_BMSK                                            0x1
#define HWIO_LPASS_LPAIF_IO_MUXCTL_RX0_SEL_SHFT                                              0

/*----------------------------------------------------------------------------
 * MODULE: LPASS_LPASS_LPM
 *--------------------------------------------------------------------------*/

#define LPASS_LPASS_LPM_REG_BASE(x)		((x) + 0x00250000)
#define LPASS_LPASS_LPM_REG_BASE_SIZE		0x4000
#define LPASS_LPASS_LPM_REG_BASE_USED		0x0
#define LPASS_LPASS_LPM_REG_BASE_OFFS		0x00250000

#define GCC_LPASS_RESTART		0x180F000
#define GCC_SNOC_LPASS_AXIM_CBCR	0x1826074
#define GCC_SNOC_LPASS_SWAY_CBCR	0x1826078
#define GCC_LPASS_BCR			0x182E000
#define GCC_LPASS_AXIM_CMD_RCGR		0x182E028
#define GCC_LPASS_AXIM_CFG_RCGR		0x182E02C
#define GCC_LPASS_SWAY_CMD_RCGR		0x182E040
#define GCC_LPASS_SWAY_CFG_RCGR		0x182E044
#define GCC_LPASS_CORE_AXIM_CBCR	0x182E048
#define GCC_LPASS_SWAY_CBCR		0x182E04C

typedef enum {
	LPASSPLL0,
	LPASSPLL1,
	LPASSPLL2,
	LPASS_ALLPLL
} ipq_lpass_pll_type;

typedef enum {
	USE_TIMER= 0,
	REG_POOL
} ipq_lpass_time;

#define INTERFACE_PRIMARY		1
#define INTERFACE_SECONDARY		2

#define TDM_MODE_SLAVE			0
#define TDM_MODE_MASTER			1

#define LPAIF_MASTER_MODE_MUXSEL	0
#define LPAIF_SLAVE_MODE_MUXSEL		1

enum
{
	TDM_DIR_INVALID = -1,
	TDM_SINK = 0 ,
	TDM_SOURCE,
	LPASS_HW_DMA_SINK = 0,
	LPASS_HW_DMA_SOURCE
};


enum
{
	TDM_SHORT_SYNC_TYPE = 0,
	TDM_LONG_SYNC_TYPE,
	TDM_SLOT_SYNC_TYPE,
};

enum
{
	TDM_LONG_SYNC_NORMAL = 0,
	TDM_LONG_SYNC_INVERT
};
enum
{
	TDM_DATA_DELAY_0_CYCLE = 2,
	TDM_DATA_DELAY_1_CYCLE = 1,
	TDM_DATA_DELAY_2_CYCLE = 0
};

enum
{
	TDM_NO_CTRL_DATA_OE = -1,
	TDM_CTRL_DATA_OE_DISABLE = 0,
	TDM_CTRL_DATA_OE_ENABLE = 1,
};

enum
{
	NO_INVERSION,
	INVERT_INT_CLK,
	INVERT_EXT_CLK,
	INVERT_INT_EXT_CLK,
};

typedef enum {
	SRC_HB_INT_CXO = 0,
	SRC_HB_INT_EMPTY_1,
	SRC_HB_INT_EMPTY_2,
	SRC_HB_INT_EMPTY_3,
	SRC_HB_INT_EMPTY_4,
	SRC_HB_INT_DIGPLL_AUX1,
	SRC_HB_INT_AUDPLL_AUX1,
} ipq_lpass_clk_src;

struct lpass_res{
	struct reset_control *reset;
	struct clk *axi_core_clk;
	struct clk *sway_clk;
	struct clk *snoc_cfg_clk;
	struct clk *pcnoc_clk;
	struct clk *axi_snoc_clk;
	struct clk *sway_snoc_clk;
};

struct ipq_lpass_pll{
	ipq_lpass_pll_type  pll_type;

	uint32_t l;
	uint32_t alpha;
	uint32_t alpha_u;
	uint32_t pre_div;
	uint32_t post_div;
	uint32_t src;

	uint32_t pll_reset_wait;
	uint32_t pll_lock_wait;

	uint32_t app_vote;
	uint32_t q6_vote;
	uint32_t rpm_vote;
	uint32_t pll_lock_count;
	uint32_t pll_bias_count;
	uint32_t pll_vote_fsm_ena;

	uint32_t adelay;
	uint32_t anupdate;

	uint32_t cal_l_value;
	uint32_t user_ctl;
	uint32_t user_ctl_u;
	uint32_t config_ctl;
	uint32_t config_ctl_u;
	uint32_t test_ctl;
	uint32_t test_ctl_u;
	uint32_t mode;
	uint32_t freq_ctl;
	uint32_t opmode;
	uint32_t droop;
	uint32_t frac_val;

	uint32_t bist_ctl;
};

struct ipq_lpass_plllock{
	uint32_t lock_time;
	uint32_t value;
	ipq_lpass_time timer_type;
};

struct lpass_dma_config {
	void __iomem *lpaif_base;
	uint32_t buffer_start_addr;
	uint32_t dma_int_per_cnt;
	uint32_t buffer_len;
	uint8_t burst_size;
	uint8_t wps_count;
	uint8_t watermark;
	uint8_t ifconfig;
	uint8_t idx;
	uint8_t dir;
	uint8_t burst8_en;
	uint8_t burst16_en;
};

struct ipq_lpass_pcm_config
{
	uint32_t invert_sync;
	uint32_t sync_src;
	uint32_t bit_width;
	uint32_t slot_count;
	uint32_t sync_type;
	uint32_t dir;
	uint32_t pcm_index;
	uint32_t sync_delay;
	uint32_t slot_width;
	uint32_t slot_mask;
	uint32_t ctrl_data_oe;
};

void ipq_lpass_pcm_reset(void __iomem *lpaif_base,
					uint32_t pcm_index, uint32_t dir);
void ipq_lpass_pcm_reset_release(void __iomem *lpaif_base, uint32_t pcm_index,
						uint32_t dir);
void ipq_lpass_pcm_config(struct ipq_lpass_pcm_config *configPtr,
				void __iomem *lpaif_base,
				uint32_t pcm_index, uint32_t dir);
void ipq_lpass_pcm_enable(void __iomem *lpaif_base,
				uint32_t pcm_index, uint32_t dir);
void ipq_lpass_pcm_disable(void __iomem *lpaif_base,
					uint32_t pcm_index, uint32_t dir);
void ipq_lpass_enable_dma_channel(void __iomem *lpaif_base,
						uint32_t dma_idx,
						uint32_t dma_dir);
void ipq_lpass_disable_dma_channel(void __iomem *lpaif_base,
						uint32_t dma_idx,
						uint32_t dma_dir);
void ipq_lpass_dma_clear_interrupt(void __iomem *lpaif_base,
						uint32_t dma_intr_idx,
						uint32_t value);
void ipq_lpass_dma_clear_interrupt_config(void __iomem *lpaif_base,
						uint32_t dma_dir,
						uint32_t dma_idx,
						uint32_t dma_intr_idx);
void ipq_lpass_dma_disable_interrupt(void __iomem *lpaif_base,
						uint32_t dma_dir,
						uint32_t dma_idx,
						uint32_t dma_intr_idx);
void ipq_lpass_dma_enable_interrupt(void __iomem *lpaif_base,
						uint32_t dma_dir,
						uint32_t dma_idx,
						uint32_t dma_intr_idx);
void ipq_lpass_dma_get_dma_fifo_count(void __iomem *lpaif_base,
					uint32_t *fifo_cnt_ptr,
					uint32_t dma_dir, uint32_t dma_idx);
void ipq_lpass_config_dma_channel(struct lpass_dma_config *config);
void ipq_lpass_dma_read_interrupt_status(void __iomem *lpaif_base,
						uint32_t dma_intr_idx,
							uint32_t *status);
void ipq_lpass_dma_reset(void __iomem *lpaif_base,
				uint32_t dma_idx, uint32_t dma_dir);
uint32_t ipq_lpass_set_clk_rate(uint32_t intf, uint32_t clk);
void ipq_lpass_lpaif_muxsetup(uint32_t intf, uint32_t mode,
						uint32_t val, uint32_t src);
void ipq_lpass_dma_get_curr_addr(void __iomem *lpaif_base,
					uint32_t dma_idx,uint32_t dma_dir,
					uint32_t *curr_addr);
void __iomem *ipq_lpass_phy_to_virt(uint32_t phy_addr);
void ipq_lpass_pcm_enable_loopback(void __iomem *lpaif_base, uint32_t pcm_index);
