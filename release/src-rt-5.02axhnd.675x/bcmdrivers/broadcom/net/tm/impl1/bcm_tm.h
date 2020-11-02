/*
  <:copyright-BRCM:2015:proprietary:standard
  
     Copyright (c) 2015 Broadcom 
     All Rights Reserved
  
   This program is the proprietary software of Broadcom and/or its
   licensors, and may only be used, duplicated, modified or distributed pursuant
   to the terms and conditions of a separate, written license agreement executed
   between you and Broadcom (an "Authorized License").  Except as set forth in
   an Authorized License, Broadcom grants no license (express or implied), right
   to use, or waiver of any kind with respect to the Software, and Broadcom
   expressly reserves all rights in and to the Software and all intellectual
   property rights therein.  IF YOU HAVE NO AUTHORIZED LICENSE, THEN YOU HAVE
   NO RIGHT TO USE THIS SOFTWARE IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY
   BROADCOM AND DISCONTINUE ALL USE OF THE SOFTWARE.
  
   Except as expressly set forth in the Authorized License,
  
   1. This program, including its structure, sequence and organization,
      constitutes the valuable trade secrets of Broadcom, and you shall use
      all reasonable efforts to protect the confidentiality thereof, and to
      use this information only in connection with your use of Broadcom
      integrated circuit products.
  
   2. TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED "AS IS"
      AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES, REPRESENTATIONS OR
      WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH
      RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY DISCLAIMS ANY AND
      ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY, NONINFRINGEMENT,
      FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES, ACCURACY OR
      COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR CORRESPONDENCE
      TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING OUT OF USE OR
      PERFORMANCE OF THE SOFTWARE.
  
   3. TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
      ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL, SPECIAL,
      INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY
      WAY RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN
      IF BROADCOM HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES;
      OR (ii) ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE
      SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE LIMITATIONS
      SHALL APPLY NOTWITHSTANDING ANY FAILURE OF ESSENTIAL PURPOSE OF ANY
      LIMITED REMEDY.
  :> 
*/

#ifndef __BCM_TM_H_INCLUDED__
#define __BCM_TM_H_INCLUDED__

/*
*******************************************************************************
*
* File Name  : bcm_tm.h
*
* Description: This file contains the Broadcom Traffic Manager global definitions.
*
*******************************************************************************
*/

//#define CC_BCM_TM_DATAPATH_ERROR_CHECK

//#define CC_BCM_TM_DEBUG_ENABLE

#define isLogDebug bcmLog_logIsEnabled(BCM_LOG_ID_TM, BCM_LOG_LEVEL_DEBUG)
#define __logDebug(fmt, arg...)   BCM_LOG_DEBUG(BCM_LOG_ID_TM, fmt, ##arg)
#define __logInfo(fmt, arg...)    BCM_LOG_INFO(BCM_LOG_ID_TM, fmt, ##arg)
#define __logNotice(fmt, arg...)  BCM_LOG_NOTICE(BCM_LOG_ID_TM, fmt, ##arg)
#define __logError(fmt, arg...)   BCM_LOG_ERROR(BCM_LOG_ID_TM, fmt, ##arg)

#define __print(fmt, arg...) printk(fmt, ##arg)

#if defined(CC_BCM_TM_DEBUG_ENABLE)
#define __debug(fmt, arg...)                    \
    BCM_LOGCODE(                                \
        if(isLogDebug)                          \
            __print(fmt, ##arg); )
#define __debugAssert(cond)            BCM_ASSERT(cond)
#else
#define __debug(fmt, arg...)
#define __debugAssert(cond)
#endif

#define BCM_TM_ETH_IFG            20 /* bytes */
#define BCM_TM_ETH_CRC_LEN        4
#define BCM_TM_ETH_OVERHEAD       (BCM_TM_ETH_CRC_LEN + BCM_TM_ETH_IFG) 

/* Ethernet packet + 2 VLAN Tags + PPPoE + Overhead */
#define BCM_TM_BUCKET_SIZE_MIN    (1514 + 8 + 8 + BCM_TM_ETH_OVERHEAD)

#define BCM_TM_QUEUE_WEIGHT_MAX   65535 /* MUST be kept in sync with bcmTm_queue_t */

#define BCM_TM_QUEUE_MAX          32 /* MUST be kept in sync with bcmTm_arbiter_t */

#define BCM_TM_QUEUE_DEPTH_MAX    65535 /* MUST be kept in sync bcmTm_queueCtrl_t */

#define BCM_TM_SCHEDULER_MAX      16    /* MUST be kept in sync bcmTm_schedulerCtrl_t */

/* we support less number of queue profiles to support each queue to have 2 unique
 * queue profile, because we assume those queue profile should be shared */
#define BCM_TM_QUEUE_PROFILE_MAX  8
#define BCM_TM_QUEUE_DROP_ALG_MAX ((BCM_TM_LAN_SCHEDULER_MAX * BCM_TM_LAN_QUEUE_MAX) + BCM_TM_WAN_QUEUE_MAX)

#define BCM_TM_SCHEDULER_HANDLE_INVALID   ( (bcmTm_schedulerHandle_t)(-1) )

typedef uint32_t bcmTm_schedulerHandle_t;

typedef enum {
    BCM_TM_DROPALG_TYPE_DT=0,
    BCM_TM_DROPALG_TYPE_RED,
    BCM_TM_DROPALG_TYPE_WRED,
    BCM_TM_DROPALG_TYPE_MAX
} fap4kTm_dropAlgType_t;

typedef struct bcmTm_queueProfile_s {
    uint32_t dropProb     : 4,
        minThreshold : 14,
        maxThreshold : 14;
} bcmTm_queueProfile_t;

typedef struct bcmTm_queueDropAlg_s {
    uint32_t dropAlg          : 4,
        queueProfileIdLo : 14,
        queueProfileIdHi : 14;
} bcmTm_queueDropAlg_t;

typedef int (* bcmTm_txCallback_t)(uint16_t length, void *param_p);
typedef int (* bcmTm_freeCallback_t)(uint16_t length, void *param_p);

/*
 * Rate Stats
 */

typedef enum {
    BCM_TM_SHAPER_TYPE_MIN=0,
    BCM_TM_SHAPER_TYPE_MAX,
    BCM_TM_SHAPER_TYPE_TOTAL
} bcmTm_shaperType_t;

typedef enum {
    BCM_TM_ARBITER_TYPE_SP=0,
    BCM_TM_ARBITER_TYPE_WRR,
    BCM_TM_ARBITER_TYPE_SP_WRR,
    BCM_TM_ARBITER_TYPE_WFQ,
    BCM_TM_ARBITER_TYPE_TOTAL
} bcmTm_arbiterType_t;

typedef struct {
    uint32_t packets;
    uint32_t bytes;
    uint32_t droppedPackets;
    uint32_t droppedBytes;
    uint32_t schedPackets;
    uint32_t bps;
} bcmTm_queueStats_t;

/*
 * Functions
 */

static inline char *arbiterTypeName(bcmTm_arbiterType_t arbiterType)
{
    switch(arbiterType)
    {
        case BCM_TM_ARBITER_TYPE_SP:
            return "SP";

        case BCM_TM_ARBITER_TYPE_WRR:
            return "WRR";

        case BCM_TM_ARBITER_TYPE_SP_WRR:
            return "SP+WRR";

        case BCM_TM_ARBITER_TYPE_WFQ:
            return "WFQ";

        default:
            return "ERROR";
    }
}

uint32_t bcmTm_kbpsToTokens(int kbps);
uint32_t bcmTm_mbsToBucketSize(uint32_t mbs);

int bcmTm_schedulerCreate(uint32_t nbrOfQueues, uint32_t nbrOfEntries, uint32_t paramSize,
                          bcmTm_txCallback_t txCallbackFunc, bcmTm_freeCallback_t freeCallbackFunc,
                          bcmTm_schedulerHandle_t *schedulerHandle_p);
int bcmTm_schedulerDelete(bcmTm_schedulerHandle_t schedulerHandle);
int bcmTm_getSchedulerConfig(bcmTm_schedulerHandle_t schedulerHandle, uint32_t *nbrOfQueues_p,
                             uint32_t *nbrOfEntries_p, uint32_t *paramSize_p);
int bcmTm_queueConfig(bcmTm_schedulerHandle_t schedulerHandle, uint8_t queue, uint8_t shaperType,
                      uint16_t tokens, uint16_t bucketSize, uint8_t weight, uint16_t nbrOfEntriesCap);
void bcmTm_enable(uint8_t masterEnable);
int bcmTm_getQueueStats(bcmTm_schedulerHandle_t schedulerHandle, uint32_t queueIndex,
                        bcmTm_queueStats_t *queueStats_p);
int bcmTm_schedulerConfig(bcmTm_schedulerHandle_t schedulerHandle, uint8_t enable,
                          uint16_t tokens, uint16_t bucketSize);
int bcmTm_arbiterConfig(bcmTm_schedulerHandle_t schedulerHandle, uint8_t arbiterType,
                        uint8_t arbiterArg, uint8_t qShaping);
int bcmTm_dumpStats(bcmTm_schedulerHandle_t schedulerHandle);
int bcmTm_construct(void);
void bcmTm_destruct(void);

#endif  /* defined(__BCM_TM_H_INCLUDED__) */
