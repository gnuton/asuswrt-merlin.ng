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

/*
*******************************************************************************
* File Name  : bcm_tm_drv.c
*
* Description: This file contains the Broadcom Traffic Manager Driver.
*
* The driver maintains two sets of TM configuration parameters: One for the
* AUTO mode, and another for the MANUAL mode. The AUTO mode settings are
* managed by the Ethernet driver based on the auto-negotiated PHY rates.
* The MANUAL settings should be used by the user to configure the BCM TM.
*
* The mode can be set dynamically. Changing the mode does not apply the
* corresponding settings immediately, it simply selects the current mode.
* The settings corresponding to the current mode will take effect only when
* explicitly applied. This allows the caller to have a complete configuration
* before activating it.
*
*******************************************************************************
*/

#include <linux/module.h>
#include <linux/jiffies.h>
#include <linux/delay.h>
#include <linux/bcm_log.h>
#include <linux/skbuff.h>
#include <linux/nbuff.h>
#include "bcmPktDma_defines.h"
#include "bcm_tm.h"
#include "bcm_tm_drv.h"

typedef struct {
    int valid;
    int kbps;
    int mbs;
    uint32_t tokens;
    uint32_t bucketSize;
} bcmTmDrv_shaper_t;

typedef struct {
    int dropProb;
    int minThreshold;
    int maxThreshold;
} bcmTmDrv_queueProfile_t;

typedef struct {
    uint32_t alg;
    int queueProfileIdLo;
    int queueProfileIdHi;
    uint32_t priorityMask[2];
} bcmTmDrv_queueDropAlg_t;

#define BCM_TM_DRV_QUEUE_MAX BCM_TM_QUEUE_MAX

#define BCM_TM_DRV_PORT_IS_ENABLED(_port_p)                             \
    ( (_port_p)->enable[(_port_p)->mode] &&                             \
      (_port_p)->schedulerHandle != BCM_TM_SCHEDULER_HANDLE_INVALID )

#define BCM_TM_DRV_QUEUE_PROFILE_MASK_NUM ((BCM_TM_QUEUE_PROFILE_MAX & 0x1f) ? \
                                           ((BCM_TM_QUEUE_PROFILE_MAX >> 5) + 1) : (BCM_TM_QUEUE_PROFILE_MAX >> 5))

typedef struct {
    int weight;
    int configured;
    int nbrOfEntriesCap;
    bcmTmDrv_shaper_t shaper[BCM_TM_SHAPER_TYPE_TOTAL];
} bcmTmDrv_queue_t;

typedef struct {
    bcmTmDrv_shaper_t shaper;
    bcmTmDrv_shapingType_t shapingType;
    bcmTmDrv_queue_t queue[BCM_TM_DRV_QUEUE_MAX];
    bcmTm_arbiterType_t arbiterType;
    int arbiterArg;
} bcmTmDrv_scheduler_t;

typedef struct {
    int enable[BCM_TM_DRV_MODE_MAX];
    bcmTmDrv_mode_t mode;
    int pauseEnable;
    uint32_t nbrOfQueues;
    uint32_t nbrOfEntries;
    uint32_t paramSize;
    bcmTm_schedulerHandle_t schedulerHandle;
    bcmTmDrv_scheduler_t scheduler[BCM_TM_DRV_MODE_MAX];
    bcmTmDrv_queueDropAlg_t queueDropAlg[BCM_TM_DRV_QUEUE_MAX];
    bcmTm_txCallback_t txCallbackFunc;
    bcmTm_freeCallback_t freeCallbackFunc;
} bcmTmDrv_port_t;

typedef struct {
    bcmTmDrv_port_t port[BCM_TM_DRV_PORT_MAX];
} bcmTmDrv_phy_t;

typedef struct {
    int masterEnable;
    bcmTmDrv_phy_t phy[BCM_TM_DRV_PHY_TYPE_MAX];
    bcmTmDrv_queueProfile_t queueProfile[BCM_TM_QUEUE_PROFILE_MAX];
    uint32_t queueProfileMask[BCM_TM_DRV_QUEUE_PROFILE_MASK_NUM];
    bcmTmDrv_queueDropAlg_t xtmChnlDropAlg[XTM_TX_CHANNELS_MAX];
} bcmTmDrv_ctrl_t;

static bcmTmDrv_ctrl_t bcmTmDrv_g;

/*******************************************************************************
 *
 * Private Functions
 *
 *******************************************************************************/

static inline char *__modeToString(bcmTmDrv_mode_t mode)
{
    return (mode == BCM_TM_DRV_MODE_AUTO) ? "AUTO" : "MANUAL";
}

static int __applyQueueRatios(bcmTmDrv_phyType_t phy, uint8_t port)
{
    bcmTmDrv_port_t *port_p = &bcmTmDrv_g.phy[phy].port[port];
    bcmTmDrv_scheduler_t *scheduler_p = &port_p->scheduler[port_p->mode];
    int queueIndex;
    int totalWeight = 0;

    for(queueIndex=0; queueIndex < port_p->nbrOfQueues; ++queueIndex)
    {
        bcmTmDrv_queue_t *queue_p = &scheduler_p->queue[queueIndex];

        totalWeight += queue_p->weight;
    }

    if(totalWeight == 0)
    {
        __logError("Port[%u] ratios have not been configured", port);

        return -EINVAL;
    }

    for(queueIndex=0; queueIndex < port_p->nbrOfQueues; ++queueIndex)
    {
        bcmTmDrv_queue_t *queue_p = &scheduler_p->queue[queueIndex];
        bcmTmDrv_shaper_t *shaper_p = &queue_p->shaper[BCM_TM_SHAPER_TYPE_MIN];
        int shaperType;

        /* Min Shaper */

        shaper_p->valid = 1;
        shaper_p->kbps = (scheduler_p->shaper.kbps * queue_p->weight) / totalWeight;
        shaper_p->mbs = scheduler_p->shaper.mbs;
        shaper_p->tokens = bcmTm_kbpsToTokens(shaper_p->kbps);
        shaper_p->bucketSize = bcmTm_mbsToBucketSize(shaper_p->tokens + shaper_p->mbs);

        if(shaper_p->bucketSize > BCM_TM_DRV_BUCKET_SIZE_MAX)
        {
            shaper_p->bucketSize = BCM_TM_DRV_BUCKET_SIZE_MAX;
            shaper_p->mbs = BCM_TM_DRV_BUCKET_SIZE_MAX - shaper_p->tokens;
        }

        /* Max Shaper */

        shaper_p = &queue_p->shaper[BCM_TM_SHAPER_TYPE_MAX];

        /* Use the port's parameters for the max shaper */

        memcpy(shaper_p, &scheduler_p->shaper, sizeof(bcmTmDrv_shaper_t));

        /* Apply settings */

        for(shaperType=0; shaperType<BCM_TM_SHAPER_TYPE_TOTAL; ++shaperType)
        {
            bcmTmDrv_shaper_t *shaper_p = &queue_p->shaper[shaperType];
            int ret;

            ret = bcmTm_queueConfig(port_p->schedulerHandle, queueIndex, shaperType,
                                    shaper_p->tokens, shaper_p->bucketSize, queue_p->weight,
                                    queue_p->nbrOfEntriesCap);
            if(ret)
            {
                return ret;
            }
        }
    }

    return 0;
}

static int __applyQueueRates(bcmTmDrv_phyType_t phy, uint8_t port)
{
    bcmTmDrv_port_t *port_p = &bcmTmDrv_g.phy[phy].port[port];
    bcmTmDrv_scheduler_t *scheduler_p = &port_p->scheduler[port_p->mode];
    int queueIndex;

    for(queueIndex=0; queueIndex < port_p->nbrOfQueues; ++queueIndex)
    {
        bcmTmDrv_queue_t *queue_p = &scheduler_p->queue[queueIndex];
        bcmTmDrv_shaper_t *minShaper_p = &queue_p->shaper[BCM_TM_SHAPER_TYPE_MIN];
        bcmTmDrv_shaper_t *maxShaper_p = &queue_p->shaper[BCM_TM_SHAPER_TYPE_MAX];
        int shaperType;

        if(scheduler_p->arbiterType == BCM_TM_ARBITER_TYPE_WRR &&
           queue_p->weight == 0)
        {
            /* Weight must be bigger than zero */

            queue_p->weight = 1;
        }

        if(scheduler_p->arbiterType == BCM_TM_ARBITER_TYPE_SP_WRR &&
           queueIndex < scheduler_p->arbiterArg)
        {
            /* This is a WRR queue in the SP+WRR combo, so disable the min shaper,
               and use the port's parameters for the max shaper */

            memset(minShaper_p, 0, sizeof(bcmTmDrv_shaper_t));

            minShaper_p->valid = 1;

            memcpy(maxShaper_p, &scheduler_p->shaper, sizeof(bcmTmDrv_shaper_t));

            /* Weight must be bigger than zero */

            if(queue_p->weight == 0)
            {
                queue_p->weight = 1;
            }
        }
        else
        {
            uint32_t maxShaperTokens;

            if(!minShaper_p->valid || !maxShaper_p->valid)
            {
                __logError("Port[%u].Queue[%u] shapers have not been configured",
                           port, queueIndex);
                continue;
            }

            /* Adjust Max Rate Shaper */

            maxShaperTokens = bcmTm_kbpsToTokens(maxShaper_p->kbps);

            if(maxShaperTokens > minShaper_p->tokens)
            {
                maxShaper_p->tokens = maxShaperTokens - minShaper_p->tokens;
            }
            else
            {
                maxShaper_p->tokens = 0;
            }

            if(maxShaper_p->tokens)
            {
                maxShaper_p->bucketSize = bcmTm_mbsToBucketSize(maxShaper_p->tokens + maxShaper_p->mbs);
            }
            else
            {
                maxShaper_p->bucketSize = 0;
            }
        }

        /* Apply settings */

        for(shaperType=0; shaperType<BCM_TM_SHAPER_TYPE_TOTAL; ++shaperType)
        {
            bcmTmDrv_shaper_t *shaper_p = &queue_p->shaper[shaperType];
            int ret;

            ret = bcmTm_queueConfig(port_p->schedulerHandle, queueIndex, shaperType,
                                    shaper_p->tokens, shaper_p->bucketSize, queue_p->weight,
                                    queue_p->nbrOfEntriesCap);
            if(ret)
            {
                return ret;
            }
        }
    }

    return 0;
}

static char *shapingTypeStr(bcmTmDrv_shapingType_t shapingType)
{
    switch(shapingType)
    {
        case BCM_TM_DRV_SHAPING_TYPE_DISABLED:
            return "DISABLED";

        case BCM_TM_DRV_SHAPING_TYPE_RATE:
            return "RATE";

        case BCM_TM_DRV_SHAPING_TYPE_RATIO:
            return "RATIO";

        default:
            return "ERROR";
    }
}

#if 0
static int __applyQueueProfile(int queueProfileId)
{
    xmit2FapMsg_t fapMsg;
    uint32_t fapIdx;

    memset(&fapMsg, 0, sizeof(xmit2FapMsg_t));

    fapMsg.tm.queueProfileId = queueProfileId;
    fapMsg.tm.dropProb = (uint8_t)bcmTmDrv_g.queueProfile[queueProfileId].dropProb;
    fapMsg.tm.minThreshold = (uint16_t)bcmTmDrv_g.queueProfile[queueProfileId].minThreshold;
    fapMsg.tm.maxThreshold = (uint16_t)bcmTmDrv_g.queueProfile[queueProfileId].maxThreshold;

    for(fapIdx = 0; fapIdx < NUM_FAPS; ++fapIdx)
    {
        fapDrv_Xmit2Fap(fapIdx, FAP_MSG_TM_QUEUE_PROFILE_CONFIG, &fapMsg);
    }

    return 0;
}

static int __applyQueueDropAlg(bcmTmDrv_phyType_t phy, uint8_t port)
{
    bcmTmDrv_port_t *port_p = &bcmTmDrv_g.phy[phy].port[port];
    int queueIndex;

    for(queueIndex=0; queueIndex < port_p->nbrOfQueues; ++queueIndex)
    {
        bcmTmDrv_queueDropAlg_t *queue_p = &port_p->queueDropAlg[queueIndex];
        xmit2FapMsg_t fapMsg;
        uint32_t fapIdx;

        memset(&fapMsg, 0, sizeof(xmit2FapMsg_t));

        fapMsg.tm.port = port;
        fapMsg.tm.queue = queueIndex;
        fapMsg.tm.dropAlg = (uint8_t)queue_p->alg;
        fapMsg.tm.queueProfileIdLo = (uint16_t)queue_p->queueProfileIdLo;
        fapMsg.tm.queueProfileIdHi = (uint16_t)queue_p->queueProfileIdHi;

        for(fapIdx=0; fapIdx<NUM_FAPS; ++fapIdx)
        {
            fapDrv_Xmit2Fap(fapIdx, FAP_MSG_TM_QUEUE_DROPALG_CONFIG, &fapMsg);
        }
    }

    return 0;
}

static char *dropAlgTypeStr(bcmTmDrv_dropAlg_t dropAlgType)
{
    switch(dropAlgType)
    {
        case BCM_TM_DRV_DROP_ALG_DT:
            return "DROPTAIL";

        case BCM_TM_DRV_DROP_ALG_RED:
            return "RED";

        case BCM_TM_DRV_DROP_ALG_WRED:
            return "WRED";

        default:
            return "ERROR";
    }
}
#endif

static int bcmTmDrvHook_register(void *arg_p)
{
    bcmTmDrv_arg_t *tm_p = (bcmTmDrv_arg_t *)arg_p;

    return bcmTmDrv_register(tm_p->phy, tm_p->port, tm_p->nbrOfQueues,
                             tm_p->nbrOfEntries, tm_p->paramSize,
                             (bcmTm_txCallback_t)tm_p->txCallbackFunc,
                             (bcmTm_freeCallback_t)tm_p->freeCallbackFunc);
}

static int bcmTmDrvHook_portConfig(void *arg_p)
{
    bcmTmDrv_arg_t *tm_p = (bcmTmDrv_arg_t *)arg_p;

    return bcmTmDrv_portConfig(tm_p->phy, tm_p->port, tm_p->mode,
                               tm_p->kbps, tm_p->mbs, tm_p->shapingType);
}

static int bcmTmDrvHook_portEnable(void *arg_p)
{
    bcmTmDrv_arg_t *tm_p = (bcmTmDrv_arg_t *)arg_p;

    return bcmTmDrv_portEnable(tm_p->phy, tm_p->port, tm_p->mode, tm_p->enable);
}

static int bcmTmDrvHook_arbiterConfig(void *arg_p)
{
    bcmTmDrv_arg_t *tm_p = (bcmTmDrv_arg_t *)arg_p;

    return bcmTmDrv_arbiterConfig(tm_p->phy, tm_p->port, tm_p->mode,
                                  tm_p->arbiterType, tm_p->arbiterArg);
}

static int bcmTmDrvHook_queueConfig(void *arg_p)
{
    bcmTmDrv_arg_t *tm_p = (bcmTmDrv_arg_t *)arg_p;

    return bcmTmDrv_queueConfig(tm_p->phy, tm_p->port, tm_p->mode, tm_p->queue,
                                tm_p->shaperType, tm_p->kbps, tm_p->mbs);
}

static int bcmTmDrvHook_apply(void *arg_p)
{
    bcmTmDrv_arg_t *tm_p = (bcmTmDrv_arg_t *)arg_p;

    return bcmTmDrv_apply(tm_p->phy, tm_p->port);
}

static int bcmTmDrvHook_enqueue(void *arg_p)
{
    bcmTmDrv_enqueue_t *tmEnqueue_p = (bcmTmDrv_enqueue_t *)arg_p;
    bcmTmDrv_port_t *port_p = &bcmTmDrv_g.phy[tmEnqueue_p->phy].port[tmEnqueue_p->port];

    __debugAssert(tmEnqueue_p->phy < BCM_TM_DRV_PHY_TYPE_MAX &&
                  tmEnqueue_p->port < BCM_TM_DRV_PORT_MAX &&
                  port_p->schedulerHandle != BCM_TM_SCHEDULER_HANDLE_INVALID);

    return bcmTmDrv_enqueue(port_p->schedulerHandle, tmEnqueue_p->queue,
                            tmEnqueue_p->length, tmEnqueue_p->param_p);
}

/*******************************************************************************
 *
 * User API
 *
 *******************************************************************************/

/*******************************************************************************
 *
 * Function: bcmTmDrv_register
 *
 * Allows a data path driver to register its Tx callback function and argument.
 *
 *******************************************************************************/
int bcmTmDrv_register(bcmTmDrv_phyType_t phy, uint8_t port,
                      uint32_t nbrOfQueues, uint32_t nbrOfEntries, uint32_t paramSize,
                      bcmTm_txCallback_t txCallbackFunc, bcmTm_freeCallback_t freeCallbackFunc)
{
    bcmTmDrv_port_t *port_p;

    __logNotice("phy %u, port %u, nbrOfQueues %u, nbrOfEntries %u, paramSize %u, "
                "txCallbackFunc %p, freeCallbackFunc %p",
                phy, port, nbrOfQueues, nbrOfEntries, paramSize, txCallbackFunc, freeCallbackFunc);

    if(phy >= BCM_TM_DRV_PHY_TYPE_MAX || port >= BCM_TM_DRV_PORT_MAX)
    {
        __logError("Invalid phy %u, port %u", phy, port);

        return -EINVAL;
    }

    port_p = &bcmTmDrv_g.phy[phy].port[port];

    port_p->nbrOfQueues = nbrOfQueues;
    port_p->nbrOfEntries = nbrOfEntries;
    port_p->paramSize = paramSize;
    port_p->txCallbackFunc = txCallbackFunc;
    port_p->freeCallbackFunc = freeCallbackFunc;

    return 0;
}

/*******************************************************************************
 *
 * Function: bcmTmDrv_masterConfig
 *
 * Globally enables or disables the BCM TM function. Disabling the BCM TM does
 * not cause the existing settings to be lost.
 *
 *******************************************************************************/
void bcmTmDrv_masterConfig(int enable)
{
    __logDebug("enable %d", enable);

    bcmTm_enable(enable);

    bcmTmDrv_g.masterEnable = enable;
}

/*******************************************************************************
 *
 * Function: bcmTmDrv_portConfig
 *
 * Configures the shaper bit rate (kbps) and Maximum Burst Size (mbs) of the
 * specified port/mode. The mbs setting specifies the amount of bytes that are
 * allowed to be transmitted above the shaper bit rate when the input bit rate
 * exceeds the shaper bit rate. Once the mbs is achieved, no bursts will be
 * allowed until the input bit rate becomes lower than the shaper bit rate.
 *
 * This API does not apply the specified settings. This can only be done via
 * the bcmTmDrv_apply API.
 *
 *******************************************************************************/
int bcmTmDrv_portConfig(bcmTmDrv_phyType_t phy, uint8_t port, bcmTmDrv_mode_t mode,
                        int kbps, int mbs, bcmTmDrv_shapingType_t shapingType)
{
    __logNotice("phy %u, port %u, mode %u, kbps %d, mbs %d, shapingType %d",
                phy, port, mode, kbps, mbs, shapingType);

    if(phy >= BCM_TM_DRV_PHY_TYPE_MAX || port >= BCM_TM_DRV_PORT_MAX)
    {
        __logError("Invalid phy %u, port %u", phy, port);

        return -EINVAL;
    }

    if(mode >= BCM_TM_DRV_MODE_MAX)
    {
        __logError("Invalid TM Mode <%d>", mode);

        return -EINVAL;
    }

    if(kbps > BCM_TM_DRV_KBPS_MAX)
    {
        __logError("Invalid Kbps (req %d, max %d)", kbps, BCM_TM_DRV_KBPS_MAX);

        return -EINVAL;
    }

    if(shapingType >= BCM_TM_DRV_SHAPING_TYPE_MAX)
    {
        __logError("Invalid shapingType (%d)", shapingType);

        return -EINVAL;
    }

    {
        bcmTmDrv_port_t *port_p = &bcmTmDrv_g.phy[phy].port[port];
        bcmTmDrv_scheduler_t *scheduler_p = &port_p->scheduler[mode];
        bcmTmDrv_shaper_t *shaper_p;
        int queueIndex;

        scheduler_p->shapingType = shapingType;

        /* Configure port shaper */

        shaper_p = &scheduler_p->shaper;

        shaper_p->valid = 1;
        shaper_p->kbps = kbps;
        shaper_p->mbs = mbs;
        shaper_p->tokens = bcmTm_kbpsToTokens(kbps);
        shaper_p->bucketSize = bcmTm_mbsToBucketSize(shaper_p->tokens + mbs);

        if(shaper_p->bucketSize > BCM_TM_DRV_BUCKET_SIZE_MAX)
        {
            shaper_p->bucketSize = BCM_TM_DRV_BUCKET_SIZE_MAX;
            shaper_p->mbs = BCM_TM_DRV_BUCKET_SIZE_MAX - shaper_p->tokens;

            printk("Max MBS for %uKbps is %u (requested %u)\n", kbps, shaper_p->mbs, mbs);
        }

        /* Configure queue shapers to the default values, if not configured yet */

        {
            for(queueIndex=0; queueIndex<BCM_TM_DRV_QUEUE_MAX; ++queueIndex)
            {
                shaper_p = &scheduler_p->queue[queueIndex].shaper[BCM_TM_SHAPER_TYPE_MAX];

                if(!shaper_p->valid || shapingType == BCM_TM_DRV_SHAPING_TYPE_DISABLED)
                {
                    shaper_p->valid = 1;
                    shaper_p->kbps = scheduler_p->shaper.kbps;
                    shaper_p->mbs = scheduler_p->shaper.mbs;
                    shaper_p->tokens = scheduler_p->shaper.tokens;
                    shaper_p->bucketSize = scheduler_p->shaper.bucketSize;
                }

                shaper_p = &scheduler_p->queue[queueIndex].shaper[BCM_TM_SHAPER_TYPE_MIN];

                if(!shaper_p->valid || shapingType == BCM_TM_DRV_SHAPING_TYPE_DISABLED)
                {
                    shaper_p->valid = 1;
                    shaper_p->kbps = 0;
                    shaper_p->mbs = 0;
                    shaper_p->tokens = 0;
                    shaper_p->bucketSize = 0;
                }

                if(!scheduler_p->queue[queueIndex].nbrOfEntriesCap)
                {
                    /* Initialize nbrOfEntriesCap to match the configured nbrOfEntries */

                    scheduler_p->queue[queueIndex].nbrOfEntriesCap = port_p->nbrOfEntries;
                }
            }
        }
    }

    return 0;
}

/*******************************************************************************
 *
 * Function: bcmTmDrv_getPortConfig
 *
 * Returns the shaper bit rate (kbps), Maximum Burst Size (mbs), and Shaping
 * Typeof the specified port/mode.
 *
 *******************************************************************************/
int bcmTmDrv_getPortConfig(bcmTmDrv_phyType_t phy, uint8_t port, bcmTmDrv_mode_t mode, int *kbps_p, int *mbs_p,
                           bcmTmDrv_shapingType_t *shapingType_p, uint32_t *nbrOfQueues_p, uint32_t *nbrOfEntries_p,
                           uint32_t *paramSize_p)
{
    __logNotice("phy %u, port %u, mode %u", phy, port, mode);

    if(phy >= BCM_TM_DRV_PHY_TYPE_MAX || port >= BCM_TM_DRV_PORT_MAX)
    {
        __logError("Invalid phy %u, port %u", phy, port);

        return -EINVAL;
    }

    if(mode >= BCM_TM_DRV_MODE_MAX)
    {
        __logError("Invalid TM Mode <%d>", mode);

        return -EINVAL;
    }

    {
        bcmTmDrv_port_t *port_p = &bcmTmDrv_g.phy[phy].port[port];
        bcmTmDrv_scheduler_t *scheduler_p = &port_p->scheduler[mode];

        *nbrOfQueues_p = port_p->nbrOfQueues;
        *nbrOfEntries_p = port_p->nbrOfEntries;
        *paramSize_p = port_p->paramSize;

        *kbps_p = scheduler_p->shaper.kbps;
        *mbs_p = scheduler_p->shaper.mbs;
        *shapingType_p = scheduler_p->shapingType;
    }

    return 0;
}

/*******************************************************************************
 *
 * Function: bcmTmDrv_getPortCapability
 *
 * Returns the scheduling type, max queues, max strict priority queues, 
 * port shaper, and queue shaper support of the specified port.
 *
 *******************************************************************************/
int bcmTmDrv_getPortCapability(bcmTmDrv_phyType_t phy, uint8_t port, uint32_t *schedType_p, int *maxQueues_p,
                               int *maxSpQueues_p, uint8_t *portShaper_p, uint8_t *queueShaper_p)                        
{
    bcmTmDrv_port_t *port_p = NULL;
    
    __logNotice("phy %u, port %u", phy, port);

    if(phy >= BCM_TM_DRV_PHY_TYPE_MAX || port >= BCM_TM_DRV_PORT_MAX)
    {
        __logError("Invalid phy %u, port %u", phy, port);
        return -EINVAL;
    }

    port_p = &bcmTmDrv_g.phy[phy].port[port];

    *schedType_p = 0;
    *schedType_p |= BCM_TM_SP_CAPABLE;
    *schedType_p |= BCM_TM_WRR_CAPABLE;
    *schedType_p |= BCM_TM_WFQ_CAPABLE;
    *schedType_p |= BCM_TM_SP_WRR_CAPABLE;

    *maxQueues_p = port_p->nbrOfQueues;
    *maxSpQueues_p = port_p->nbrOfQueues;
    *portShaper_p = 1;
    *queueShaper_p = 1;

    return 0;
}

/*******************************************************************************
 *
 * Function: bcmTmDrv_queueConfig
 *
 * Configures the shaper bit rate (kbps) and Maximum Burst Size (mbs) of the
 * specified port/queue/mode/shaperType. The shaper type can be either the
 * minimum rate or maximum rate, as defined in bcmTm_shaperType_t.
 *
 * Each queue supports a minimum rate shaper and a maximum rate shaper.
 * The minimum queue rates have precedence over the maximum queue rates among
 * queues of a given port. The minimum queue rates are guaranteed as long as the
 * sum of the minimum queue rates is smaller than or equal to the port's rate.
 * When the sum of the minimum queue rates is smaller than the port's rate, the
 * remaining bandwith (after satisfying the minimum rate of each queue) is
 * distributed amongst the queues based on the arbitration scheme in use (SP,
 * WRR, etc), and is capped at the maximum rate of each queue.
 *
 * The mbs setting specifies the amount of bytes that are allowed to be
 * transmitted above the shaper bit rate when the input bit rate
 * exceeds the shaper bit rate. Once the mbs is achieved, no bursts will be
 * allowed until the input bit rate becomes lower than the shaper bit rate.
 *
 * This API does not apply the specified settings. This can only be done via
 * the bcmTmDrv_apply API.
 *
 *******************************************************************************/
int bcmTmDrv_queueConfig(bcmTmDrv_phyType_t phy, uint8_t port, bcmTmDrv_mode_t mode, uint8_t queue,
                         uint8_t shaperType, int kbps, int mbs)
{
    __logNotice("phy %u, port %u, mode %u, queue %u, shaperType %u, kbps %d, mbs %d",
                phy, port, mode, queue, shaperType, kbps, mbs);

    if(phy >= BCM_TM_DRV_PHY_TYPE_MAX || port >= BCM_TM_DRV_PORT_MAX)
    {
        __logError("Invalid phy %u, port %u", phy, port);

        return -EINVAL;
    }

    if(mode >= BCM_TM_DRV_MODE_MAX)
    {
        __logError("Invalid TM Mode <%d>", mode);

        return -EINVAL;
    }

    if(queue >= BCM_TM_DRV_QUEUE_MAX)
    {
        __logError("Invalid TM Queue <%d>", queue);

        return -EINVAL;
    }

    if(shaperType >= BCM_TM_SHAPER_TYPE_TOTAL)
    {
        __logError("Invalid TM shaperType <%d>", shaperType);

        return -EINVAL;
    }

    if(kbps > BCM_TM_DRV_KBPS_MAX)
    {
        __logError("Invalid Kbps (req %d, max %d)", kbps, BCM_TM_DRV_KBPS_MAX);

        return -EINVAL;
    }

    {
        bcmTmDrv_scheduler_t *scheduler_p = &bcmTmDrv_g.phy[phy].port[port].scheduler[mode];

        if(scheduler_p->shapingType == BCM_TM_DRV_SHAPING_TYPE_RATE)
        {
            bcmTmDrv_shaper_t *shaper_p = &scheduler_p->queue[queue].shaper[shaperType];

            shaper_p->valid = 1;
            shaper_p->kbps = kbps;
            shaper_p->mbs = mbs;
            shaper_p->tokens = bcmTm_kbpsToTokens(kbps);
            shaper_p->bucketSize = bcmTm_mbsToBucketSize(shaper_p->tokens + mbs);

            if(shaper_p->bucketSize > BCM_TM_DRV_BUCKET_SIZE_MAX)
            {
                shaper_p->bucketSize = BCM_TM_DRV_BUCKET_SIZE_MAX;
                shaper_p->mbs = BCM_TM_DRV_BUCKET_SIZE_MAX - shaper_p->tokens;

                printk("Max MBS for %uKbps is %u (requested %u)\n", kbps, shaper_p->mbs, mbs);
            }
        }
        else
        {
            __logError("Queue Shaping is disabled: port %u, mode %u", port, mode);

            return -EINVAL;
        }

        /* Indicate this queue is configured, clear when unconfigured. */
        scheduler_p->queue[queue].configured = 1;
    }

    return 0;
}

/*******************************************************************************
 *
 * Function: bcmTmDrv_queueUnconfig
 *
 * Clear the configured bit of the specified queue.
 * This API does not need to apply. The configured bit is used to detect if a 
 * queue is configured by BCM TM API or not.
 *
 *******************************************************************************/
int bcmTmDrv_queueUnconfig(bcmTmDrv_phyType_t phy, uint8_t port, bcmTmDrv_mode_t mode, uint8_t queue)
{
    __logNotice("phy %u, port %u, mode %u, queue %u", phy, port, mode, queue);

    if(phy >= BCM_TM_DRV_PHY_TYPE_MAX || port >= BCM_TM_DRV_PORT_MAX)
    {
        __logError("Invalid phy %u, port %u", phy, port);

        return -EINVAL;
    }

    if(mode >= BCM_TM_DRV_MODE_MAX)
    {
        __logError("Invalid TM Mode <%d>", mode);

        return -EINVAL;
    }

    if(queue >= BCM_TM_DRV_QUEUE_MAX)
    {
        __logError("Invalid TM Queue <%d>", queue);

        return -EINVAL;
    }

    {
        bcmTmDrv_scheduler_t *scheduler_p = &bcmTmDrv_g.phy[phy].port[port].scheduler[mode];

        /* Indicate this queue is un-configured. */
        scheduler_p->queue[queue].configured = 0;
    }

    return 0;
}

/*******************************************************************************
 *
 * Function: bcmTmDrv_getQueueConfig
 *
 * Returns the max/min shaping bit rate (kbps), Maximum Burst Size (mbs),
 * weight, and queue size of the specified port, mode, and queue. 
 *
 *******************************************************************************/
int bcmTmDrv_getQueueConfig(bcmTmDrv_phyType_t phy, uint8_t port, bcmTmDrv_mode_t mode, uint8_t queue,
                            int *kbps_p, int *minKbps_p, int *mbs_p, int *weight_p, int *qsize_p)
{
    __logNotice("phy %u, port %u, mode %u, queue %u", phy, port, mode, queue);

    if(phy >= BCM_TM_DRV_PHY_TYPE_MAX || port >= BCM_TM_DRV_PORT_MAX)
    {
        __logError("Invalid phy %u, port %u", phy, port);

        return -EINVAL;
    }

    if(mode >= BCM_TM_DRV_MODE_MAX)
    {
        __logError("Invalid TM Mode <%d>", mode);

        return -EINVAL;
    }

    if(queue >= BCM_TM_DRV_QUEUE_MAX)
    {
        __logError("Invalid TM Queue <%d>", queue);

        return -EINVAL;
    }

    {
        bcmTmDrv_scheduler_t *scheduler_p = &bcmTmDrv_g.phy[phy].port[port].scheduler[mode];
        bcmTmDrv_queue_t *queue_p = &scheduler_p->queue[queue];
        bcmTmDrv_shaper_t *minShaper_p = &queue_p->shaper[BCM_TM_SHAPER_TYPE_MIN];
        bcmTmDrv_shaper_t *maxShaper_p = &queue_p->shaper[BCM_TM_SHAPER_TYPE_MAX];

        *weight_p = queue_p->weight;
        *kbps_p = maxShaper_p->kbps;
        *mbs_p = maxShaper_p->mbs;
        *minKbps_p = minShaper_p->kbps;

        if (queue_p->configured)
        {
            /* queue is configured, report real queue size */
            /* TODO: Should qsize be retrieved from bpm thresh? */
            *qsize_p = 512;
        }
        else
        {
            /* queue is not configured, report qsize is zero */
            *qsize_p = 0;
        }
    }

    return 0;
}

/*******************************************************************************
 *
 * Function: bcmTmDrv_setQueueCap
 *
 * Configures the queue level cap, which must be a value greater than zero
 * and smaller than or equal to the number of entries assigned to the queue.
 *
 * This API does not apply the specified settings. This can only be done via
 * the bcmTmDrv_apply API.
 *
 *******************************************************************************/
int bcmTmDrv_setQueueCap(bcmTmDrv_phyType_t phy, uint8_t port, bcmTmDrv_mode_t mode, uint8_t queue,
                         int nbrOfEntriesCap)
{
    __logNotice("phy %u, port %u, mode %u, queue %u, nbrOfEntriesCap %d",
                phy, port, mode, queue, nbrOfEntriesCap);

    if(phy >= BCM_TM_DRV_PHY_TYPE_MAX || port >= BCM_TM_DRV_PORT_MAX)
    {
        __logError("Invalid phy %u, port %u", phy, port);

        return -EINVAL;
    }

    if(mode >= BCM_TM_DRV_MODE_MAX)
    {
        __logError("Invalid TM Mode <%d>", mode);

        return -EINVAL;
    }

    if(queue >= BCM_TM_DRV_QUEUE_MAX)
    {
        __logError("Invalid TM Queue <%d>", queue);

        return -EINVAL;
    }

    {
        bcmTmDrv_port_t *port_p = &bcmTmDrv_g.phy[phy].port[port];
        bcmTmDrv_scheduler_t *scheduler_p = &port_p->scheduler[mode];
        bcmTmDrv_queue_t *queue_p = &scheduler_p->queue[queue];

        if(!nbrOfEntriesCap || (nbrOfEntriesCap > port_p->nbrOfEntries))
        {
            __logError("Invalid Queue Cap <%d>", nbrOfEntriesCap);

            return -EINVAL;
        }

        queue_p->nbrOfEntriesCap = nbrOfEntriesCap;
    }

    return 0;
}

#if 0
/*******************************************************************************
 *
 * Function: bcmTmDrv_allocQueueProfileId
 *
 * Obtain a free queue profile index.  We try to reserve Queue Profile Id#0
 *
 *******************************************************************************/
int bcmTmDrv_allocQueueProfileId(int *queueProfileId_p)
{
    int i;

    for (i = 1; i < BCM_TM_QUEUE_PROFILE_MAX; i++) {
        if ((bcmTmDrv_g.queueProfileMask[i >> 5] & (0x1 << (i & 0x1f))) == 0) {
            *queueProfileId_p = i;
            bcmTmDrv_g.queueProfileMask[i >> 5] |= 0x1 << (i & 0x1f);
            return 0;
        }
    }
    __logError("Run out of Queue Profile");
    return -EINVAL;
}

/*******************************************************************************
 *
 * Function: bcmTmDrv_freeQueueProfileId
 *
 * free a queue profile index
 *
 *******************************************************************************/
int bcmTmDrv_freeQueueProfileId(int queueProfileId)
{
    if((queueProfileId >= BCM_TM_QUEUE_PROFILE_MAX) || (queueProfileId == 0))
    {
        __logError("Invalid Queue Profile ID %u", queueProfileId);

        return -EINVAL;
    }

    bcmTmDrv_g.queueProfileMask[queueProfileId >> 5] &=
        ~(0x1 << (queueProfileId & 0x1f));
    return 0;
}

/*******************************************************************************
 *
 * Function: bcmTmDrv_queueProfileConfig
 *
 * Apply queue profile attribute to the given queue profile index.
 * If queueProfileId given is 0, it will automatically allocate a free Queue
 * Profile Index. (Also, queueProfileId#0 is reserved for default queue profile)
 *
 *******************************************************************************/
int bcmTmDrv_queueProfileConfig(int queueProfileId, int dropProbability, int minThreshold,
                                int maxThreshold)
{
    int ret;

    __logDebug("queueProfileId %d, dropProbability %d, minThreshold %d, "
               "maxThreshold %d\n", queueProfileId, dropProbability,
               minThreshold, maxThreshold);

    if(queueProfileId >= BCM_TM_QUEUE_PROFILE_MAX)
    {
        __logError("Invalid Queue Profile ID %u", queueProfileId);

        return -EINVAL;
    }

    if(queueProfileId == 0) {
        ret = bcmTmDrv_allocQueueProfileId(&queueProfileId);
        if (ret == -EINVAL)
            return ret;
    }

    if((dropProbability < 0) || (dropProbability > 100) || (minThreshold < 0) ||
       (maxThreshold < 0) || (maxThreshold <= minThreshold))
    {
        __logError("Invalid TM Queue Profile Parameter");
        return -EINVAL;
    }

    bcmTmDrv_g.queueProfile[queueProfileId].dropProb = dropProbability;
    bcmTmDrv_g.queueProfile[queueProfileId].minThreshold = minThreshold;
    bcmTmDrv_g.queueProfile[queueProfileId].maxThreshold = maxThreshold;
    bcmTmDrv_g.queueProfileMask[queueProfileId >> 5] |= 0x1 << (queueProfileId & 0x1f);

    return __applyQueueProfile(queueProfileId);
}

/*******************************************************************************
 *
 * Function: bcmTmDrv_getQueueProfileConfig
 *
 * Returns the queue profile attribute to the given queue profile index.
 * Assume those pointers are not NULL.
 *
 *******************************************************************************/
int bcmTmDrv_getQueueProfileConfig(int queueProfileId, int *dropProbability_p,
                                   int *minThreshold_p, int *maxThreshold_p)
{
    __logDebug("queueProfileId %d\n", queueProfileId);

    if(queueProfileId >= BCM_TM_QUEUE_PROFILE_MAX)
    {
        __logError("Invalid Queue Profile ID %u", queueProfileId);

        return -EINVAL;
    }

    *dropProbability_p = bcmTmDrv_g.queueProfile[queueProfileId].dropProb;
    *minThreshold_p = bcmTmDrv_g.queueProfile[queueProfileId].minThreshold;
    *maxThreshold_p = bcmTmDrv_g.queueProfile[queueProfileId].maxThreshold;

    return 0;
}

/*******************************************************************************
 *
 * Function: bcmTmDrv_queueDropAlgConfig
 *
 * Configure the queue dropping algorithm to the given queue.
 * When dropAlgorithm is BCM_TM_DRV_DROP_ALG_DT, it ignores the values from both
 *     queue profile indices
 * When dropAlgorithm is BCM_TM_DRV_DROP_ALG_RED, it will apply the queueProfileIdLo
 * When dropAlgorithm is BCM_TM_DRV_DROP_ALG_WRED, it will use both queue profile
 *     indices
 *
 *******************************************************************************/
int bcmTmDrv_queueDropAlgConfig(bcmTmDrv_phyType_t phy, uint8_t port, uint8_t queue, bcmTmDrv_dropAlg_t dropAlgorithm,
                                int queueProfileIdLo, int queueProfileIdHi,
                                uint32_t priorityMask0, uint32_t priorityMask1)
{
    __logNotice("phy %u, port %u, queue %u, dropAlgorithm %u, queueProfileIdLo %d, "
                "queueProfileIdHi %d, priorityMask0 0x%x, priorityMask1 0x%x\n",
                phy, port, queue, dropAlgorithm, queueProfileIdLo, queueProfileIdHi,
                priorityMask0, priorityMask1);

    if(phy >= BCM_TM_DRV_PHY_TYPE_MAX || port >= BCM_TM_DRV_PORT_MAX)
    {
        __logError("Invalid phy %u, port %u", phy, port);

        return -EINVAL;
    }

    if(queue >= BCM_TM_DRV_QUEUE_MAX)
    {
        __logError("Invalid TM Queue <%d>", queue);

        return -EINVAL;
    }

    if(dropAlgorithm >= BCM_TM_DRV_DROP_ALG_MAX)
    {
        __logError("Invalid TM Drop Algorithm <%u>", dropAlgorithm);

        return -EINVAL;
    }

    if(((dropAlgorithm == BCM_TM_DRV_DROP_ALG_RED) || (dropAlgorithm == BCM_TM_DRV_DROP_ALG_WRED)) &&
       (queueProfileIdLo >= BCM_TM_QUEUE_PROFILE_MAX))
    {
        __logError("Invalid TM Queue Profile ID <%u>",
                   queueProfileIdLo);

        return -EINVAL;
    }

    if((dropAlgorithm == BCM_TM_DRV_DROP_ALG_WRED) &&
       (queueProfileIdHi >= BCM_TM_QUEUE_PROFILE_MAX))
    {
        __logError("Invalid TM Queue Profile ID <%u>",
                   queueProfileIdHi);

        return -EINVAL;
    }

    {
        bcmTmDrv_queueDropAlg_t *queueDropAlg_p = &bcmTmDrv_g.phy[phy].port[port].queueDropAlg[queue];

        queueDropAlg_p->alg = dropAlgorithm;
        queueDropAlg_p->queueProfileIdLo = queueProfileIdLo;
        queueDropAlg_p->queueProfileIdHi = queueProfileIdHi;
        queueDropAlg_p->priorityMask[0] = priorityMask0;
        queueDropAlg_p->priorityMask[1] = priorityMask1;
    }

    return 0;
}

/*******************************************************************************
 *
 * Function: bcmTmDrv_getQueueDropAlgConfig
 *
 * Return the queue drop algorithm applied to the given queue.
 * When dropAlgorithm is BCM_TM_DRV_DROP_ALG_DT, it ignores the values from both
 *     queue profile indices
 * When dropAlgorithm is BCM_TM_DRV_DROP_ALG_RED, it will apply the queueProfileIdLo
 * When dropAlgorithm is BCM_TM_DRV_DROP_ALG_WRED, it will use both queue profile
 *     indices
 *
 *******************************************************************************/
int bcmTmDrv_getQueueDropAlgConfig(bcmTmDrv_phyType_t phy, uint8_t port, uint8_t queue, bcmTmDrv_dropAlg_t *dropAlgorithm_p,
                                   int *queueProfileIdLo_p, int *queueProfileIdHi_p,
                                   uint32_t *priorityMask0_p, uint32_t *priorityMask1_p)
{
    __logNotice("phy %u, port %u, queue %u", phy, port, queue);

    if(phy >= BCM_TM_DRV_PHY_TYPE_MAX || port >= BCM_TM_DRV_PORT_MAX)
    {
        __logError("Invalid phy %u, port %u", phy, port);

        return -EINVAL;
    }

    if(queue >= BCM_TM_DRV_QUEUE_MAX)
    {
        __logError("Invalid TM Queue <%d>", queue);

        return -EINVAL;
    }

    {
        bcmTmDrv_queueDropAlg_t *queueDropAlg_p = &bcmTmDrv_g.phy[phy].port[port].queueDropAlg[queue];

        *dropAlgorithm_p = queueDropAlg_p->alg;
        *queueProfileIdLo_p = queueDropAlg_p->queueProfileIdLo;
        *queueProfileIdHi_p = queueDropAlg_p->queueProfileIdHi;
        *priorityMask0_p = queueDropAlg_p->priorityMask[0];
        *priorityMask1_p = queueDropAlg_p->priorityMask[1];
    }

    return 0;
}

#if defined(CONFIG_BCM_XTMCFG) || defined(CONFIG_BCM_XTMCFG_MODULE)
static int __applyXtmQueueDropAlgConfig(int chnl)
{
    xmit2FapMsg_t fapMsg;
    uint32_t fapIdx;
    bcmTmDrv_queueDropAlg_t *xtmChnlDropAlg_p = &bcmTmDrv_g.xtmChnlDropAlg[chnl];

    memset(&fapMsg, 0, sizeof(xmit2FapMsg_t));

    fapMsg.xtmQueueDropAlg.channel = chnl;
    fapMsg.xtmQueueDropAlg.dropAlg = xtmChnlDropAlg_p->alg;
    fapMsg.xtmQueueDropAlg.queueProfileIdLo = xtmChnlDropAlg_p->queueProfileIdLo;
    fapMsg.xtmQueueDropAlg.queueProfileIdHi = xtmChnlDropAlg_p->queueProfileIdHi;

    for(fapIdx = 0; fapIdx < NUM_FAPS; ++fapIdx)
    {
        fapDrv_Xmit2Fap(fapIdx, FAP_MSG_XTM_QUEUE_DROPALG_CONFIG, &fapMsg);
    }

    return 0;
}

/*******************************************************************************
 *
 * Function: bcmTmDrv_XtmQueueDropAlgConfig
 *
 * Configure the queue dropping algorithm to the given XTM channel.
 * When dropAlgorithm is BCM_TM_DRV_DROP_ALG_DT, it ignores the values from both
 *     queue profile indices
 * When dropAlgorithm is BCM_TM_DRV_DROP_ALG_RED, it will apply the queueProfileIdLo
 * When dropAlgorithm is BCM_TM_DRV_DROP_ALG_WRED, it will use both queue profile
 *     indices
 *
 *******************************************************************************/
int bcmTmDrv_XtmQueueDropAlgConfig(uint8_t chnl, bcmTmDrv_dropAlg_t dropAlgorithm,
                                   int queueProfileIdLo, int queueProfileIdHi,
                                   uint32_t priorityMask0, uint32_t priorityMask1)
{
    __logDebug("channel %u, dropAlgorithm %u, queueProfileIdLo %d, "
               "queueProfileIdHi %d, priorityMask0 0x%x, priorityMask1 0x%x\n",
               channel, dropAlgorithm, queueProfileIdLo, queueProfileIdHi,
               priorityMask0, priorityMask1);

    if(chnl >= XTM_TX_CHANNELS_MAX)
    {
        __logError("Invalid channel %u", chnl);

        return -EINVAL;
    }

    if(dropAlgorithm >= BCM_TM_DRV_DROP_ALG_MAX)
    {
        __logError("Invalid TM Drop Algorithm <%u>", dropAlgorithm);

        return -EINVAL;
    }

    if(((dropAlgorithm == BCM_TM_DRV_DROP_ALG_RED) || (dropAlgorithm == BCM_TM_DRV_DROP_ALG_WRED)) &&
       (queueProfileIdLo >= BCM_TM_QUEUE_PROFILE_MAX))
    {
        __logError("Invalid TM Queue Profile ID <%u>",
                   queueProfileIdLo);

        return -EINVAL;
    }

    if((dropAlgorithm == BCM_TM_DRV_DROP_ALG_WRED) &&
       (queueProfileIdHi >= BCM_TM_QUEUE_PROFILE_MAX))
    {
        __logError("Invalid TM Queue Profile ID <%u>",
                   queueProfileIdHi);

        return -EINVAL;
    }

    {
        bcmTmDrv_queueDropAlg_t *xtmChnlDropAlg_p = &bcmTmDrv_g.xtmChnlDropAlg[chnl];

        xtmChnlDropAlg_p->alg = dropAlgorithm;
        xtmChnlDropAlg_p->queueProfileIdLo = queueProfileIdLo;
        xtmChnlDropAlg_p->queueProfileIdHi = queueProfileIdHi;
        xtmChnlDropAlg_p->priorityMask[0] = priorityMask0;
        xtmChnlDropAlg_p->priorityMask[1] = priorityMask1;
    }

    return __applyXtmQueueDropAlgConfig(chnl);
}

/*******************************************************************************
 *
 * Function: bcmTmDrv_getXtmQueueDropAlgConfig
 *
 * Return the queue drop algorithm applied to the given XTM channel.
 * When dropAlgorithm is BCM_TM_DRV_DROP_ALG_DT, it ignores the values from both
 *     queue profile indices
 * When dropAlgorithm is BCM_TM_DRV_DROP_ALG_RED, it will apply the queueProfileIdLo
 * When dropAlgorithm is BCM_TM_DRV_DROP_ALG_WRED, it will use both queue profile
 *     indices
 *
 *******************************************************************************/
int bcmTmDrv_getXtmQueueDropAlgConfig(uint8_t chnl, bcmTmDrv_dropAlg_t *dropAlgorithm_p,
                                      int *queueProfileIdLo_p, int *queueProfileIdHi_p,
                                      uint32_t *priorityMask0_p, uint32_t *priorityMask1_p)
{
    __logDebug("channel %u", chnl);

    if(chnl >= XTM_TX_CHANNELS_MAX)
    {
        __logError("Invalid channel %u", chnl);

        return -EINVAL;
    }

    {
        bcmTmDrv_queueDropAlg_t *xtmChnlDropAlg_p = &bcmTmDrv_g.xtmChnlDropAlg[chnl];

        *dropAlgorithm_p = xtmChnlDropAlg_p->alg;
        *queueProfileIdLo_p = xtmChnlDropAlg_p->queueProfileIdLo;
        *queueProfileIdHi_p = xtmChnlDropAlg_p->queueProfileIdHi;
        *priorityMask0_p = xtmChnlDropAlg_p->priorityMask[0];
        *priorityMask1_p = xtmChnlDropAlg_p->priorityMask[1];
    }

    return 0;
}
#endif /* defined(CONFIG_BCM_XTMCFG) || defined(CONFIG_BCM_XTMCFG_MODULE) */

/*******************************************************************************
 *
 * Function: bcmTmDrv_checkHighPrio
 *
 * Check the given TC value (0 to 63) is set to use high priority RED profile in
 * WRED scheme, if so, return 1.  If not, return 0
 *
 *******************************************************************************/
int bcmTmDrv_checkHighPrio(bcmTmDrv_phyType_t phy, uint8_t port, uint8_t queue, uint8_t chnl, uint32_t tc)
{
    bcmTmDrv_queueDropAlg_t *queueDropAlg_p;

    __logNotice("phy %u, port %u, queue %u, chnl %u", phy, port, queue, chnl);

    // TODO! are we going to support FAP4KE_PKT_PHY_WLAN?
    if((phy != FAP4KE_PKT_PHY_XTM) && (phy != FAP4KE_PKT_PHY_ENET) &&
       (phy != FAP4KE_PKT_PHY_ENET_EXT))
    {
        /* don't print the error log, it is confusing.  */
        //__logError("Invalid phy %u, support only XTM, ENET, ENET_EXT", phy);

        return -EINVAL;
    }

    if((phy == FAP4KE_PKT_PHY_ENET) || (phy == FAP4KE_PKT_PHY_ENET_EXT))
    {
        if(phy >= BCM_TM_DRV_PHY_TYPE_MAX || port >= BCM_TM_DRV_PORT_MAX)
        {
            __logError("Invalid phy %u, port %u", phy, port);

            return -EINVAL;
        }

        if(queue >= BCM_TM_DRV_QUEUE_MAX)
        {
            __logError("Invalid TM Queue <%d>", queue);

            return -EINVAL;
        }
        queueDropAlg_p = &bcmTmDrv_g.phy[phy].port[port].queueDropAlg[queue];
    }
    else /* if (phy == FAP4KE_PKT_PHY_XTM) */
    {
        if(chnl >= XTM_TX_CHANNELS_MAX)
        {
            __logError("Invalid channel %u", chnl);

            return -EINVAL;
	}
        queueDropAlg_p = &bcmTmDrv_g.xtmChnlDropAlg[chnl];
    }

    if(tc >= BCM_TM_TC_MAX)
    {
        return -EINVAL;
    }

    if(tc < 32)
    {
        if(queueDropAlg_p->priorityMask[0] & (0x1 << tc))
            return 1;
    }
    else /* if(tc < 63) */
    {
        if(queueDropAlg_p->priorityMask[1] & (0x1 << (tc - 32)))
            return 1;
    }

    return 0;
}

/*******************************************************************************
 *
 * Function: bcmTmDrv_checkSetHighPrio
 *
 * Check the given TC value (0 to 63) is set to use high priority RED profile in
 * WRED scheme, if so, reflect the high priority info in destQueue_p.
 *
 *******************************************************************************/
int bcmTmDrv_checkSetHighPrio(bcmTmDrv_phyType_t phy, uint8_t port, uint8_t queue, uint32_t tc, uint32_t *destQueue_p)
{
    __logNotice("phy %u, port %u, queue %u", phy, port, queue);

    if(phy >= BCM_TM_DRV_PHY_TYPE_MAX || port >= BCM_TM_DRV_PORT_MAX)
    {
        __logError("Invalid phy %u, port %u", phy, port);

        return -EINVAL;
    }

    if(queue >= BCM_TM_DRV_QUEUE_MAX)
    {
        __logError("Invalid TM Queue <%d>", queue);

        return -EINVAL;
    }

    if(unlikely(destQueue_p == NULL))
    {
        return -EINVAL;
    }

    if(tc >= BCM_TM_TC_MAX)
    {
        return -EINVAL;
    }

    {
        bcmTmDrv_queueDropAlg_t *queueDropAlg_p = &bcmTmDrv_g.phy[phy].port[port].queueDropAlg[queue];
        if(tc < 32)
        {
            if(queueDropAlg_p->priorityMask[0] & (0x1 << tc))
                *destQueue_p |= BCM_TM_HIGH_PRIO_MASK;
        }
        else /* if(tc < 63) */
        {
            if(queueDropAlg_p->priorityMask[1] & (0x1 << (tc - 32)))
                *destQueue_p |= BCM_TM_HIGH_PRIO_MASK;
        }
    }

    return 0;
}

/*******************************************************************************
 *
 * Function: bcmTmDrv_xtmCheckHighPrio
 *
 * Check the given TC value (0 to 63) is set to use high priority RED profile in
 * WRED scheme, if so, return 1.  If not, return 0
 *
 *******************************************************************************/
int bcmTmDrv_xtmCheckHighPrio(uint8_t chnl, uint32_t tc)
{
    return bcmTmDrv_checkHighPrio(FAP4KE_PKT_PHY_XTM, 0, 0, chnl, tc);
}
#endif

/*******************************************************************************
 *
 * Function: bcmTmDrv_getQueueStats
 *
 * Returns the transmitted packets, transmitted bytes, dropped packets,
 * and dropped bytes of the specified port, mode, and queue.
 *
 *******************************************************************************/
int bcmTmDrv_getQueueStats(bcmTmDrv_phyType_t phy, uint8_t port, bcmTmDrv_mode_t mode, uint8_t queue,
                           uint32_t *txPackets_p, uint32_t *txBytes_p,
                           uint32_t *droppedPackets_p, uint32_t *droppedBytes_p,
                           uint32_t *bps_p)
{
    bcmTmDrv_port_t *port_p = &bcmTmDrv_g.phy[phy].port[port];
    bcmTm_queueStats_t queueStats;
    int ret;

    __logNotice("phy %u, port %u, mode %u, queue %u", phy, port, mode, queue);

    if(phy >= BCM_TM_DRV_PHY_TYPE_MAX || port >= BCM_TM_DRV_PORT_MAX)
    {
        __logError("Invalid phy %u, port %u", phy, port);

        return -EINVAL;
    }

    if(mode >= BCM_TM_DRV_MODE_MAX)
    {
        __logError("Invalid TM Mode <%d>", mode);

        return -EINVAL;
    }

    if(queue >= BCM_TM_DRV_QUEUE_MAX)
    {
        __logError("Invalid TM Queue <%d>", queue);

        return -EINVAL;
    }

    ret = bcmTm_getQueueStats(port_p->schedulerHandle, queue, &queueStats);
    if(ret)
    {
        return ret;
    }

    *txPackets_p = queueStats.packets;
    *txBytes_p = queueStats.bytes;
    *droppedPackets_p = queueStats.droppedPackets;
    *droppedBytes_p = queueStats.droppedBytes;
    *bps_p = queueStats.bps;

    return 0;
}

/*******************************************************************************
 *
 * Function: bcmTmDrv_setQueueWeight
 *
 * Configures the given Queue's weight.
 *
 * This API does not apply the specified settings. This can only be done via
 * the bcmTmDrv_apply API.
 *
 *******************************************************************************/
int bcmTmDrv_setQueueWeight(bcmTmDrv_phyType_t phy, uint8_t port, bcmTmDrv_mode_t mode, uint8_t queue, uint8_t weight)
{
    __logNotice("phy %u, port %u, mode %u, queue %u, weight %d",
                phy, port, mode, queue, weight);

    if(phy >= BCM_TM_DRV_PHY_TYPE_MAX || port >= BCM_TM_DRV_PORT_MAX)
    {
        __logError("Invalid phy %u, port %u", phy, port);

        return -EINVAL;
    }

    if(mode >= BCM_TM_DRV_MODE_MAX)
    {
        __logError("Invalid TM Mode <%d>", mode);

        return -EINVAL;
    }

    if(queue >= BCM_TM_DRV_QUEUE_MAX)
    {
        __logError("Invalid TM Queue <%d>", queue);

        return -EINVAL;
    }

    if(weight == 0 || weight > BCM_TM_QUEUE_WEIGHT_MAX)
    {
        __logError("Invalid Weight %d", weight);

        return -EINVAL;
    }

    {
        bcmTmDrv_scheduler_t *scheduler_p = &bcmTmDrv_g.phy[phy].port[port].scheduler[mode];
        bcmTmDrv_queue_t *queue_p = &scheduler_p->queue[queue];

        queue_p->weight = weight;
    }

    return 0;
}

/*******************************************************************************
 *
 * Function: bcmTmDrv_arbiterConfig
 *
 * Configures the Arbiter type of the given port to SP, WRR, SP+WRR, or WFQ.
 * arbiterArg is a generic argument that is passed to arbiters. Currently it is
 * only used in SP+WRR, where it indicates the lowest priority queue in the
 * SP Tier.
 *
 * This API does not apply the corresponding mode settings. This can only be
 * done via the bcmTmDrv_apply API.
 *
 *******************************************************************************/
int bcmTmDrv_arbiterConfig(bcmTmDrv_phyType_t phy, uint8_t port, bcmTmDrv_mode_t mode, uint8_t arbiterType, uint8_t arbiterArg)
{
    __logNotice("phy %u, port %u, arbiterType %u, arbiterArg %u",
                phy, port, arbiterType, arbiterArg);

    if(phy >= BCM_TM_DRV_PHY_TYPE_MAX || port >= BCM_TM_DRV_PORT_MAX)
    {
        __logError("Invalid phy %u, port %u", phy, port);

        return -EINVAL;
    }

    if(mode >= BCM_TM_DRV_MODE_MAX)
    {
        __logError("Invalid TM Mode <%d>", mode);

        return -EINVAL;
    }

    if(arbiterType >= BCM_TM_ARBITER_TYPE_TOTAL)
    {
        __logError("Invalid arbiterType %u", arbiterType);

        return -EINVAL;
    }

    {
        bcmTmDrv_scheduler_t *scheduler_p = &bcmTmDrv_g.phy[phy].port[port].scheduler[mode];

        if(arbiterType == BCM_TM_ARBITER_TYPE_WRR &&
           scheduler_p->shapingType != BCM_TM_DRV_SHAPING_TYPE_DISABLED)
        {
            __logError("Port %d: In WRR, queue shaping must be disabled", port);

            return -EINVAL;
        }

        scheduler_p->arbiterType = arbiterType;
        scheduler_p->arbiterArg = arbiterArg;
    }

    return 0;
}

/*******************************************************************************
 *
 * Function: bcmTmDrv_getArbiterConfig
 *
 * Returns the Arbiter type of the given port and mode.
 * arbiterArg is a generic argument that is passed to arbiters.
 * Currently it is only used in SP+WRR, where it indicates the lowest 
 * priority queue in the SP Tier.
 *
 *******************************************************************************/
int bcmTmDrv_getArbiterConfig(bcmTmDrv_phyType_t phy, uint8_t port, bcmTmDrv_mode_t mode,
                              bcmTm_arbiterType_t *arbiterType_p, int *arbiterArg_p)
{
    __logNotice("phy %u, port %u, mode %u", phy, port, mode);

    if(phy >= BCM_TM_DRV_PHY_TYPE_MAX || port >= BCM_TM_DRV_PORT_MAX)
    {
        __logError("Invalid phy %u, port %u", phy, port);

        return -EINVAL;
    }

    if(mode >= BCM_TM_DRV_MODE_MAX)
    {
        __logError("Invalid TM Mode <%d>", mode);

        return -EINVAL;
    }

    {
        bcmTmDrv_scheduler_t *scheduler_p = &bcmTmDrv_g.phy[phy].port[port].scheduler[mode];
        *arbiterType_p = scheduler_p->arbiterType;
        *arbiterArg_p = scheduler_p->arbiterArg;
    }

    return 0;
}

/*******************************************************************************
 *
 * Function: bcmTmDrv_setPortMode
 *
 * Sets the mode of the given port to AUTO or MANUAL.
 *
 * This API does not apply the corresponding mode settings. This can only be
 * done via the bcmTmDrv_apply API.
 *
 *******************************************************************************/
int bcmTmDrv_setPortMode(bcmTmDrv_phyType_t phy, uint8_t port, bcmTmDrv_mode_t mode)
{
    bcmTmDrv_port_t *port_p;

    __logNotice("phy %u, port %u, mode %u", phy, port, mode);

    if(phy >= BCM_TM_DRV_PHY_TYPE_MAX || port >= BCM_TM_DRV_PORT_MAX)
    {
        __logError("Invalid phy %u, port %u", phy, port);

        return -EINVAL;
    }

    if(mode >= BCM_TM_DRV_MODE_MAX)
    {
        __logError("Invalid TM Mode <%d>", mode);

        return -EINVAL;
    }

    port_p = &bcmTmDrv_g.phy[phy].port[port];

    port_p->mode = mode;

    return 0;
}

/*******************************************************************************
 *
 * Function: bcmTmDrv_getPortMode
 *
 * Returns the mode of the given port.
 *
 *******************************************************************************/
bcmTmDrv_mode_t bcmTmDrv_getPortMode(bcmTmDrv_phyType_t phy, uint8_t port)
{
    bcmTmDrv_port_t *port_p;

    if(phy >= BCM_TM_DRV_PHY_TYPE_MAX || port >= BCM_TM_DRV_PORT_MAX)
    {
        __logError("Invalid phy %u, port %u", phy, port);

        return -EINVAL;
    }

    port_p = &bcmTmDrv_g.phy[phy].port[port];

    return port_p->mode;
}

/*******************************************************************************
 *
 * Function: bcmTmDrv_modeReset
 *
 * Resets the current configuration of the given port and mode.
 *
 * This API does not apply the corresponding mode settings. This can only be
 * done via the bcmTmDrv_apply API.
 *
 *******************************************************************************/
int bcmTmDrv_modeReset(bcmTmDrv_phyType_t phy, uint8_t port, bcmTmDrv_mode_t mode)
{
    bcmTmDrv_port_t *port_p;
    bcmTmDrv_scheduler_t *scheduler_p;

    __logNotice("phy %u, port %u, mode %u", phy, port, mode);

    if(phy >= BCM_TM_DRV_PHY_TYPE_MAX || port >= BCM_TM_DRV_PORT_MAX)
    {
        __logError("Invalid phy %u, port %u", phy, port);

        return -EINVAL;
    }

    if(mode >= BCM_TM_DRV_MODE_MAX)
    {
        __logError("Invalid TM Mode <%d>", mode);

        return -EINVAL;
    }

    port_p = &bcmTmDrv_g.phy[phy].port[port];

    scheduler_p = &port_p->scheduler[mode];

    memset(scheduler_p, 0, sizeof(bcmTmDrv_scheduler_t));

    return 0;
}

/*******************************************************************************
 *
 * Function: bcmTmDrv_portEnable
 *
 * Enables or Disables the given port and mode.
 *
 * This API does not apply the corresponding mode settings. This can only be
 * done via the bcmTmDrv_apply API.
 *
 *******************************************************************************/
int bcmTmDrv_portEnable(bcmTmDrv_phyType_t phy, uint8_t port, bcmTmDrv_mode_t mode, int enable)
{
    bcmTmDrv_port_t *port_p;

    __logNotice("phy %u, port %u, mode %u, enable %u", phy, port, mode, enable);

    if(phy >= BCM_TM_DRV_PHY_TYPE_MAX || port >= BCM_TM_DRV_PORT_MAX)
    {
        __logError("Invalid phy %u, port %u", phy, port);

        return -EINVAL;
    }

    if(mode >= BCM_TM_DRV_MODE_MAX)
    {
        __logError("Invalid TM Mode <%d>", mode);

        return -EINVAL;
    }

    port_p = &bcmTmDrv_g.phy[phy].port[port];

    port_p->enable[mode] = enable;

    return 0;
}

/*******************************************************************************
 *
 * Function: bcmTmDrv_pauseEnable
 *
 * Enables or Disables pause on a given port.
 *
 * This API does not apply the corresponding mode settings. This can only be
 * done via the bcmTmDrv_apply API.
 *
 *******************************************************************************/
int bcmTmDrv_pauseEnable(bcmTmDrv_phyType_t phy, uint8_t port, int enable)
{
    bcmTmDrv_port_t *port_p;

    __logNotice("phy %u, port %u, enable %d", phy, port, enable);

    if(phy >= BCM_TM_DRV_PHY_TYPE_MAX || port >= BCM_TM_DRV_PORT_MAX)
    {
        __logError("Invalid phy %u, port %u", phy, port);
        return -EINVAL;
    }

    port_p = &bcmTmDrv_g.phy[phy].port[port];
    port_p->pauseEnable = enable;

    return 0;
}

/*******************************************************************************
 *
 * Function: bcmTmDrv_apply
 *
 * Applies the settings corresponding to the current mode of the specified port.
 *
 * This API also allows enabling or disabling the specified port.
 *
 *******************************************************************************/
int bcmTmDrv_apply(bcmTmDrv_phyType_t phy, uint8_t port)
{
    bcmTmDrv_port_t *port_p;
    int enable;
    int ret;

    __logNotice("phy %u, port %u", phy, port);

    if(phy >= BCM_TM_DRV_PHY_TYPE_MAX || port >= BCM_TM_DRV_PORT_MAX)
    {
        __logError("Invalid phy %u, port %u", phy, port);

        return -EINVAL;
    }

    port_p = &bcmTmDrv_g.phy[phy].port[port];

    if(port_p->txCallbackFunc == NULL)
    {
        __logError("PHY %u was not registered", phy);

        return -EINVAL;
    }

    enable = port_p->enable[port_p->mode];

    if(enable)
    {
        bcmTmDrv_scheduler_t *scheduler_p = &port_p->scheduler[port_p->mode];
        bcmTmDrv_shaper_t *shaper_p = &scheduler_p->shaper;
        int qShaping = (scheduler_p->shapingType == BCM_TM_DRV_SHAPING_TYPE_DISABLED) ? 0 : 1;

        if(!shaper_p->valid)
        {
            __logError("Cannot enable port %u, %s mode is not configured",
                       port, __modeToString(port_p->mode));

            return -EINVAL;
        }

        if(port_p->schedulerHandle == BCM_TM_SCHEDULER_HANDLE_INVALID)
        {
            /* Allocate a new scheduler */

            ret = bcmTm_schedulerCreate(port_p->nbrOfQueues, port_p->nbrOfEntries,
                                        port_p->paramSize, port_p->txCallbackFunc,
                                        port_p->freeCallbackFunc, &port_p->schedulerHandle);
            if(ret)
            {
                return ret;
            }
        }
        else
        {
            /* Scheduler already allocated, overwrite the mode parameters to match
               the existing scheduler */

            ret = bcmTm_getSchedulerConfig(port_p->schedulerHandle, &port_p->nbrOfQueues,
                                           &port_p->nbrOfEntries, &port_p->paramSize);
            if(ret)
            {
                return ret;
            }
        }

        ret = bcmTm_schedulerConfig(port_p->schedulerHandle, enable,
                                    shaper_p->tokens, shaper_p->bucketSize);
        if(ret)
        {
            return ret;
        }

        if(scheduler_p->shapingType == BCM_TM_DRV_SHAPING_TYPE_RATIO)
        {
            ret = __applyQueueRatios(phy, port);
            if(ret)
            {
                return ret;
            }
        }
        else /* Rate or Disabled */
        {
            ret = __applyQueueRates(phy, port);
            if(ret)
            {
                return ret;
            }
        }

#if 0
        if(__applyQueueDropAlg(port) == -EINVAL)
        {
            return -EINVAL;
        }
#endif

        ret = bcmTm_arbiterConfig(port_p->schedulerHandle, scheduler_p->arbiterType,
                                  scheduler_p->arbiterArg, qShaping);
        if(ret)
        {
            return ret;
        }
    }
    else /* disable */
    {
        if(port_p->schedulerHandle != BCM_TM_SCHEDULER_HANDLE_INVALID)
        {
            /* Scheduler has been allocated */

            bcmTm_schedulerDelete(port_p->schedulerHandle);

            port_p->schedulerHandle = BCM_TM_SCHEDULER_HANDLE_INVALID;
        }
    }

#if 0
    fapMsg.tm.port = port;
    fapMsg.tm.enable = port_p->pauseEnable;
    for(fapIdx=0; fapIdx<NUM_FAPS; ++fapIdx)
    {
        fapDrv_Xmit2Fap(fapIdx, FAP_MSG_TM_PAUSE_EN, &fapMsg);
    }
#endif

    return 0;
}

/*******************************************************************************
 *
 * Function: bcmTmDrv_status
 *
 * Prints the status of all ENABLED ports.
 *
 *******************************************************************************/
void bcmTmDrv_status(void)
{
    int phy;
    int port;

    printk("\tBCM TM Status: %s\n\n", bcmTmDrv_g.masterEnable ? "ON" : "OFF");

    for(phy=0; phy<BCM_TM_DRV_PHY_TYPE_MAX; ++phy)
    {
        for(port=0; port<BCM_TM_DRV_PORT_MAX; ++port)
        {
            bcmTmDrv_port_t *port_p = &bcmTmDrv_g.phy[phy].port[port];

            if(port_p->schedulerHandle != BCM_TM_SCHEDULER_HANDLE_INVALID)
            {
                bcmTmDrv_mode_t mode;
                int queueIndex;

                printk("\tPort[%02u]: %s, nbrOfQueues %d, nbrOfEntries %d, handle %d\n",
                       port, __modeToString(port_p->mode),
                       port_p->nbrOfQueues, port_p->nbrOfEntries,
                       port_p->schedulerHandle);

                for(mode=0; mode<BCM_TM_DRV_MODE_MAX; ++mode)
                {
                    bcmTmDrv_scheduler_t *scheduler_p = &port_p->scheduler[mode];
                    bcmTmDrv_shaper_t *shaper_p = &scheduler_p->shaper;

                    printk("\t\t  %6s: %s, %s (%d), Kbps %u (tokens %u), MBS %u (bucketSize %u), "
                           "qShaping %s\n",
                           __modeToString(mode), port_p->enable[mode] ? "ENABLED" : "DISABLED",
                           arbiterTypeName(scheduler_p->arbiterType), scheduler_p->arbiterArg,
                           shaper_p->kbps, shaper_p->tokens, shaper_p->mbs, shaper_p->bucketSize,
                           shapingTypeStr(scheduler_p->shapingType));

                    for(queueIndex=0; queueIndex < port_p->nbrOfQueues; ++queueIndex)
                    {
                        bcmTmDrv_queue_t *queue_p = &scheduler_p->queue[queueIndex];

                        printk("\t\t          Queue[%u]: MIN: Kbps %u (tokens %u), MBS %u (size %u), Weight: %u, Cap %u\n"
                               "\t\t                    MAX: Kbps %u (tokens %u), MBS %u (size %u)\n",
                               queueIndex,
                               queue_p->shaper[BCM_TM_SHAPER_TYPE_MIN].kbps,
                               queue_p->shaper[BCM_TM_SHAPER_TYPE_MIN].tokens,
                               queue_p->shaper[BCM_TM_SHAPER_TYPE_MIN].mbs,
                               queue_p->shaper[BCM_TM_SHAPER_TYPE_MIN].bucketSize,
                               queue_p->weight,
                               queue_p->nbrOfEntriesCap,
                               queue_p->shaper[BCM_TM_SHAPER_TYPE_MAX].kbps,
                               queue_p->shaper[BCM_TM_SHAPER_TYPE_MAX].tokens,
                               queue_p->shaper[BCM_TM_SHAPER_TYPE_MAX].mbs,
                               queue_p->shaper[BCM_TM_SHAPER_TYPE_MAX].bucketSize);
                    }
                }

#if 0
                printk("\t\tQueue Drop Algorithm:\n");
                for(queueIndex=0; queueIndex < port_p->nbrOfQueues; ++queueIndex)
                {
                    bcmTmDrv_queueDropAlg_t *queueDropAlg_p = &port_p->queueDropAlg[queueIndex];

                    printk("\t\t          Queue[%u]: DropAlg", queueIndex);

                    if(queueDropAlg_p->alg == BCM_TM_DRV_DROP_ALG_DT)
                    {
                        printk("(%s)\n", dropAlgTypeStr(queueDropAlg_p->alg));
                    }
                    else if(queueDropAlg_p->alg == BCM_TM_DRV_DROP_ALG_RED)
                    {
                        printk("(%s): queueProfileId %d\n",
                               dropAlgTypeStr(queueDropAlg_p->alg),
                               queueDropAlg_p->queueProfileIdLo);
                    }
                    else if(queueDropAlg_p->alg == BCM_TM_DRV_DROP_ALG_WRED)
                    {
                        printk("(%s): queueProfileIdLo %d, queueProfileIdHi %d, "
                               "priorityMask0 0x%x, priorityMask1 0x%x\n",
                               dropAlgTypeStr(queueDropAlg_p->alg),
                               queueDropAlg_p->queueProfileIdLo,
                               queueDropAlg_p->queueProfileIdHi,
                               queueDropAlg_p->priorityMask[0],
                               queueDropAlg_p->priorityMask[1]);
                    }
                    else
                    {
                        printk(" Unknown\n");
                    }
                }
#endif
                printk("\n");
            }
        }
    }

#if 0
#if defined(CONFIG_BCM_XTMCFG) || defined(CONFIG_BCM_XTMCFG_MODULE)
    {
        int chnl;

        printk("\tXTM Queue Drop Algorithm Info:\n");
        for(chnl = 0; chnl < XTM_TX_CHANNELS_MAX; chnl++)
        {
            bcmTmDrv_queueDropAlg_t *queueDropAlg_p = &bcmTmDrv_g.xtmChnlDropAlg[chnl];

            printk("\t\tchannel[%u]: DropAlg", chnl);
            if(queueDropAlg_p->alg == BCM_TM_DRV_DROP_ALG_DT)
            {
                printk("(%s)\n", dropAlgTypeStr(queueDropAlg_p->alg));
            }
            else if(queueDropAlg_p->alg == BCM_TM_DRV_DROP_ALG_RED)
            {
                printk("(%s): queueProfileId %d\n",
                       dropAlgTypeStr(queueDropAlg_p->alg),
                       queueDropAlg_p->queueProfileIdLo);
            }
            else if(queueDropAlg_p->alg == BCM_TM_DRV_DROP_ALG_WRED)
            {
                printk("(%s): queueProfileIdLo %d, queueProfileIdHi %d, "
                       "priorityMask0 0x%x, priorityMask1 0x%x\n",
                       dropAlgTypeStr(queueDropAlg_p->alg),
                       queueDropAlg_p->queueProfileIdLo,
                       queueDropAlg_p->queueProfileIdHi,
                       queueDropAlg_p->priorityMask[0],
                       queueDropAlg_p->priorityMask[1]);
            }
            else
            {
                printk(" Unknown\n");
            }
        }
        printk("\n");
    }
#endif

    {
        int queueProfileId;

        printk("\tQueue Profile Info:\n");
        for(queueProfileId = 0; queueProfileId < BCM_TM_QUEUE_PROFILE_MAX; queueProfileId++)
        {
            if(bcmTmDrv_g.queueProfileMask[queueProfileId >> 5] & (0x1 << (queueProfileId & 0x1f)))
            {
                printk("\t\tQueueProfileID#%d is enabled:\n\t\t\tdropProb %d, minThreshold %d, "
                       "maxThreshold %d\n", queueProfileId,
                       bcmTmDrv_g.queueProfile[queueProfileId].dropProb,
                       bcmTmDrv_g.queueProfile[queueProfileId].minThreshold,
                       bcmTmDrv_g.queueProfile[queueProfileId].maxThreshold);
            }
        }
    }
#endif
}

/*******************************************************************************
 *
 * Function: bcmTmDrv_stats
 *
 * Prints the statistics counters of all ENABLED ports.
 *
 *******************************************************************************/
int bcmTmDrv_stats(bcmTmDrv_phyType_t phy, uint8_t port)
{
    bcmTm_schedulerHandle_t schedulerHandle;

    printk("phy %u, port %u\n", phy, port);

    if(BCM_TM_IS_DONT_CARE(port))
    {
        schedulerHandle = BCM_TM_SCHEDULER_HANDLE_INVALID;
    }
    else
    {
        bcmTmDrv_port_t *port_p = &bcmTmDrv_g.phy[phy].port[port];

        schedulerHandle = port_p->schedulerHandle;
    }

    return bcmTm_dumpStats(schedulerHandle);
}

/*******************************************************************************
 *
 * Function: bcmTmDrv_ioctl
 *
 * IOCTL interface to the BCM TM API.
 *
 *******************************************************************************/
static long bcmTmDrv_ioctl(struct file *filep, unsigned int command, unsigned long arg)
{
    bcmTmDrv_arg_t *userTm_p = (bcmTmDrv_arg_t *)arg;
    bcmTmDrv_arg_t tm;
    bcmTmIoctl_cmd_t cmd;
    int ret = 0;

    if (command >= BCM_TM_IOCTL_MAX)
    {
        cmd = BCM_TM_IOCTL_MAX;
    }
    else
    {
        cmd = (bcmTmIoctl_cmd_t)command;
    }

    __logInfo("cmd<%d>, arg<0x%08lX>", command, arg);

    copy_from_user(&tm, userTm_p, sizeof(bcmTmDrv_arg_t));

    switch(cmd)
    {
        case BCM_TM_IOCTL_MASTER_CONFIG:
            __logNotice("MASTER_CONFIG: enable <%u>", tm.enable);
            bcmTmDrv_masterConfig(tm.enable);
            break;

        case BCM_TM_IOCTL_PORT_CONFIG:
            __logNotice("PORT_CONFIG: phy <%u>, port <%u>, mode <%u>, kbps <%u>, mbs <%u>, shapingType <%u>",
                        tm.phy, tm.port, tm.mode, tm.kbps, tm.mbs, tm.shapingType);
            ret = bcmTmDrv_portConfig(tm.phy, tm.port, tm.mode, tm.kbps, tm.mbs, tm.shapingType);
            break;

        case BCM_TM_IOCTL_GET_PORT_CONFIG:
            ret = bcmTmDrv_getPortConfig(tm.phy, tm.port, tm.mode, &tm.kbps, &tm.mbs,
                                         (bcmTmDrv_shapingType_t *)&tm.shapingType,
                                         &tm.nbrOfQueues, &tm.nbrOfEntries, &tm.paramSize);
            __logNotice("GET_PORT_CONFIG: phy <%u>, port <%u>, mode <%u>, kbps <%u>, mbs <%u>, shapingType <%u>"
                        "nbrOfQueues <%u>, nbrOfEntries <%u>, paramSize <%u>",
                        tm.phy, tm.port, tm.mode, tm.kbps, tm.mbs, tm.shapingType, tm.nbrOfQueues,
                        tm.nbrOfEntries, tm.paramSize);
            copy_to_user(userTm_p, &tm, sizeof(bcmTmDrv_arg_t));
            break;

        case BCM_TM_IOCTL_GET_PORT_CAPABILITY:
            ret = bcmTmDrv_getPortCapability(tm.phy, tm.port,
                                             &tm.portCapability.schedType,
                                             &tm.portCapability.maxQueues, 
                                             &tm.portCapability.maxSpQueues,
                                             &tm.portCapability.portShaper,
                                             &tm.portCapability.queueShaper);           
            __logNotice("GET_PORT_CONFIG: phy <%u>, port <%u>, schedType <%u>, maxQueues <%u>, "
                        "maxSpQueues <%u>, portShaper <%u>, queueShaper <%u>",
                        tm.phy, tm.port,
                        tm.portCapability.schedType,
                        tm.portCapability.maxQueues, 
                        tm.portCapability.maxSpQueues,
                        tm.portCapability.portShaper,
                        tm.portCapability.queueShaper); 
            copy_to_user(userTm_p, &tm, sizeof(bcmTmDrv_arg_t));
            break;

        case BCM_TM_IOCTL_PORT_MODE:
            __logNotice("PORT_MODE: phy <%u>, port <%u>, mode <%u>", tm.phy, tm.port, tm.mode);
            ret = bcmTmDrv_setPortMode(tm.phy, tm.port, tm.mode);
            break;

        case BCM_TM_IOCTL_MODE_RESET:
            __logNotice("MODE_RESET: phy <%u>, port <%u>, mode <%u>", tm.phy, tm.port, tm.mode);
            ret = bcmTmDrv_modeReset(tm.phy, tm.port, tm.mode);
            break;

        case BCM_TM_IOCTL_PORT_ENABLE:
            __logNotice("PORT_ENABLE: phy <%u>, port <%u>, mode <%u>, enable <%u>", tm.phy, tm.port, tm.mode, tm.enable);
            ret = bcmTmDrv_portEnable(tm.phy, tm.port, tm.mode, tm.enable);
            break;

        case BCM_TM_IOCTL_PORT_APPLY:
            __logNotice("PORT_APPLY: phy <%u>, port <%u>", tm.phy, tm.port);
            ret = bcmTmDrv_apply(tm.phy, tm.port);
            break;

        case BCM_TM_IOCTL_QUEUE_CONFIG:
            __logNotice("QUEUE_CONFIG: phy <%u>, port <%u>, mode <%u>, queue <%u>, shaperType <%u>, kbps <%u>, mbs <%u>",
                        tm.phy, tm.port, tm.mode, tm.queue, tm.shaperType, tm.kbps, tm.mbs);
            ret = bcmTmDrv_queueConfig(tm.phy, tm.port, tm.mode, tm.queue, tm.shaperType, tm.kbps, tm.mbs);
            break;

        case BCM_TM_IOCTL_QUEUE_UNCONFIG:
            __logNotice("QUEUE_UNCONFIG: phy <%u>, port <%u>, mode <%u>, queue <%u>",
                        tm.phy, tm.port, tm.mode, tm.queue);
            ret = bcmTmDrv_queueUnconfig(tm.phy, tm.port, tm.mode, tm.queue);
            break;

        case BCM_TM_IOCTL_GET_QUEUE_CONFIG:
            ret = bcmTmDrv_getQueueConfig(tm.phy, tm.port, tm.mode, tm.queue,
                                          &tm.kbps, &tm.minKbps, &tm.mbs, &tm.weight, &tm.qsize);
            __logNotice("GET_QUEUE_CONFIG: phy <%u>, port <%u>, mode <%u>, queue <%u> "
                        "kbps <%u>, minKbps <%u>, mbs <%u>, weight <%u>, qsize <%u>",
                        tm.phy, tm.port, tm.mode, tm.queue,
                        tm.kbps, tm.minKbps, tm.mbs, tm.weight, tm.qsize);
            copy_to_user(userTm_p, &tm, sizeof(bcmTmDrv_arg_t));
            break;

        case BCM_TM_IOCTL_SET_QUEUE_CAP:
            __logNotice("SET_QUEUE_CAP: phy <%u>, port <%u>, mode <%u>, queue <%u>, nbrOfEntriesCap <%u>",
                        tm.phy, tm.port, tm.mode, tm.queue, tm.nbrOfEntriesCap);
            ret = bcmTmDrv_setQueueCap(tm.phy, tm.port, tm.mode, tm.queue, tm.nbrOfEntriesCap);
            break;

#if 0
        case BCM_TM_IOCTL_ALLOC_QUEUE_PROFILE_ID:
            ret = bcmTmDrv_allocQueueProfileId(&tm.queueProfileId);

            __logNotice("GET_PROFILE_ID: queueProfileId <%u>", tm.queueProfileId);
            copy_to_user(userTm_p, &tm, sizeof(bcmTmDrv_arg_t));
            break;

        case BCM_TM_IOCTL_FREE_QUEUE_PROFILE_ID:
            __logNotice("FREE_PROFILE_ID: queueProfileId <%u>", tm.queueProfileId);
            ret = bcmTmDrv_freeQueueProfileId(tm.queueProfileId);

            break;

        case BCM_TM_IOCTL_QUEUE_PROFILE_CONFIG:
            __logNotice("QUEUE_PROFILE_CONFIG: queueProfileId <%u>, dropProbability <%d>, "
                        "minThreshold <%d>, maxThreshold <%d>", tm.queueProfileId,
                        tm.dropProbability, tm.minThreshold, tm.maxThreshold);
            ret = bcmTmDrv_queueProfileConfig(tm.queueProfileId, tm.dropProbability,
                                              tm.minThreshold, tm.maxThreshold);
            break;

        case BCM_TM_IOCTL_GET_QUEUE_PROFILE_CONFIG:
            ret = bcmTmDrv_getQueueProfileConfig(tm.queueProfileId, &tm.dropProbability,
                                                 &tm.minThreshold, &tm.maxThreshold);

            __logNotice("GET_QUEUE_PROFILE_CONFIG: queueProfileId <%u>, dropProbability <%d>, "
                        "minThreshold <%d>, maxThreshold <%d>", tm.queueProfileId,
                        tm.dropProbability, tm.minThreshold, tm.maxThreshold);
            copy_to_user(userTm_p, &tm, sizeof(bcmTmDrv_arg_t));
            break;

        case BCM_TM_IOCTL_QUEUE_DROP_ALG_CONFIG:
            __logNotice("QUEUE_DROP_ALG_CONFIG: phy <%u>, port <%u>, queue <%u>, dropAlgorithm <%d>, "
                        "queueProfileId <%d>, queueProfileIdHi <%d>, priorityMask0 <0x%x>, "
                        "priorityMask1 <0x%x>",
                        tm.phy, tm.port, tm.queue, tm.dropAlgorithm, tm.queueProfileId,
                        tm.queueProfileIdHi, tm.priorityMask0, tm.priorityMask1);
            ret = bcmTmDrv_queueDropAlgConfig(tm.phy, tm.port, tm.queue, tm.dropAlgorithm,
                                              tm.queueProfileId,
                                              tm.queueProfileIdHi,
                                              tm.priorityMask0, tm.priorityMask1);
            break;

        case BCM_TM_IOCTL_GET_QUEUE_DROP_ALG_CONFIG:
            ret = bcmTmDrv_getQueueDropAlgConfig(tm.phy, tm.port, tm.queue,
                                                 (bcmTmDrv_dropAlg_t *)&tm.dropAlgorithm,
                                                 &tm.queueProfileId,
                                                 &tm.queueProfileIdHi,
                                                 &tm.priorityMask0,
                                                 &tm.priorityMask1);
            __logNotice("GET_QUEUE_DROP_ALG_CONFIG: phy <%u>, port <%u>, queue <%u>, dropAlgorithm <%d>, "
                        "queueProfileId <%d>, queueProfileIdHi <%d>, priorityMask0 <0x%x>, "
                        "priorityMask1 <0x%x>",
                        tm.phy, tm.port, tm.queue, tm.dropAlgorithm, tm.queueProfileId,
                        tm.queueProfileIdHi, tm.priorityMask0, tm.priorityMask1);
            copy_to_user(userTm_p, &tm, sizeof(bcmTmDrv_arg_t));
            break;

#if defined(CONFIG_BCM_XTMCFG) || defined(CONFIG_BCM_XTMCFG_MODULE)
        case BCM_TM_IOCTL_XTM_QUEUE_DROP_ALG_CONFIG:
            __logNotice("XTM_QUEUE_DROP_ALG_CONFIG: channel <%u>, dropAlgorithm <%d>, "
                        "queueProfileId <%d>, queueProfileIdHi <%d>, priorityMask0 <0x%x>, "
                        "priorityMask1 <0x%x>",
                        tm.channel, tm.dropAlgorithm, tm.queueProfileId,
                        tm.queueProfileIdHi, tm.priorityMask0, tm.priorityMask1);
            ret = bcmTmDrv_XtmQueueDropAlgConfig(tm.channel, tm.dropAlgorithm,
                                                 tm.queueProfileId,
                                                 tm.queueProfileIdHi,
                                                 tm.priorityMask0, tm.priorityMask1);
            break;

        case BCM_TM_IOCTL_GET_XTM_QUEUE_DROP_ALG_CONFIG:
            ret = bcmTmDrv_getXtmQueueDropAlgConfig(tm.channel,
                                                    (bcmTmDrv_dropAlg_t *)&tm.dropAlgorithm,
                                                    &tm.queueProfileId,
                                                    &tm.queueProfileIdHi,
                                                    &tm.priorityMask0,
                                                    &tm.priorityMask1);
            __logNotice("GET_XTM_QUEUE_DROP_ALG_CONFIG: channel <%u>, dropAlgorithm <%d>, "
                        "queueProfileId <%d>, queueProfileIdHi <%d>, priorityMask0 <0x%x>, "
                        "priorityMask1 <0x%x>",
                        tm.channel, tm.dropAlgorithm, tm.queueProfileId,
                        tm.queueProfileIdHi, tm.priorityMask0, tm.priorityMask1);
            copy_to_user(userTm_p, &tm, sizeof(bcmTmDrv_arg_t));
            break;
#else
        case BCM_TM_IOCTL_XTM_QUEUE_DROP_ALG_CONFIG:
            __logNotice("GET_XTM_QUEUE_DROP_ALG_CONFIG: FAIL! XTM is not enabled!");
            break;

        case BCM_TM_IOCTL_GET_XTM_QUEUE_DROP_ALG_CONFIG:
            __logNotice("GET_XTM_QUEUE_DROP_ALG_CONFIG: FAIL! XTM is not enabled!");
	    break;
#endif
#endif

        case BCM_TM_IOCTL_GET_QUEUE_STATS:
            ret = bcmTmDrv_getQueueStats(tm.phy, tm.port, tm.mode, tm.queue,
                                         &tm.queueStats.txPackets,
                                         &tm.queueStats.txBytes,
                                         &tm.queueStats.droppedPackets,
                                         &tm.queueStats.droppedBytes,
                                         &tm.queueStats.bps);
            __logNotice("GET_QUEUE_STATS: phy <%u>, port <%u>, mode <%u>, queue <%u> "
                        "txPackets <%u>, txBytes <%u>, droppedPackets <%u>, "
                        "droppedBytes <%u>, bps <%u>",
                        tm.phy, tm.port, tm.mode, tm.queue,
                        tm.queueStats.txPackets,
                        tm.queueStats.txBytes,
                        tm.queueStats.droppedPackets,
                        tm.queueStats.droppedBytes,
                        tm.queueStats.bps);
            copy_to_user(userTm_p, &tm, sizeof(bcmTmDrv_arg_t));
            break;
            
        case BCM_TM_IOCTL_QUEUE_WEIGHT:
            __logNotice("QUEUE_WEIGHT: phy <%u>, port <%u>, mode <%u>, queue <%u>, weight <%u>",
                        tm.phy, tm.port, tm.mode, tm.queue, tm.weight);
            ret = bcmTmDrv_setQueueWeight(tm.phy, tm.port, tm.mode, tm.queue, tm.weight);
            break;

        case BCM_TM_IOCTL_ARBITER_CONFIG:
            __logNotice("ARBITER_CONFIG: phy <%u>, port <%u>, mode <%u>, arbiterType <%u>, arbiterArg <%u>",
                        tm.phy, tm.port, tm.mode, tm.arbiterType, tm.arbiterArg);
            ret = bcmTmDrv_arbiterConfig(tm.phy, tm.port, tm.mode, tm.arbiterType, tm.arbiterArg);
            break;

        case BCM_TM_IOCTL_GET_ARBITER_CONFIG:
            ret = bcmTmDrv_getArbiterConfig(tm.phy, tm.port, tm.mode, (bcmTm_arbiterType_t *)&tm.arbiterType, &tm.arbiterArg);
            __logNotice("GET_ARBITER_CONFIG: phy <%u>, port <%u>, mode <%u>, arbiterType <%u>, arbiterArg <%u>",
                        tm.phy, tm.port, tm.mode, tm.arbiterType, tm.arbiterArg);
            copy_to_user(userTm_p, &tm, sizeof(bcmTmDrv_arg_t));
            break;

        case BCM_TM_IOCTL_STATUS:
            bcmTmDrv_status();
            break;

        case BCM_TM_IOCTL_STATS:
            bcmTmDrv_stats(tm.phy, tm.port);
            break;

        default:
            __logError("Invalid IOCTL cmd %d", cmd);
            ret = -EINVAL;
    }

    return ret;
}

/*******************************************************************************
 *
 * Function: bcmTmDrv_init
 *
 * Initializes the FAP TM API.
 *
 *******************************************************************************/
static void bcmTmDrv_init(void)
{
    int phy;
    int port;

    memset(&bcmTmDrv_g, 0, sizeof(bcmTmDrv_ctrl_t));

    for(phy=0; phy<BCM_TM_DRV_PHY_TYPE_MAX; ++phy)
    {
        for(port=0; port<BCM_TM_DRV_PORT_MAX; ++port)
        {
            bcmTmDrv_port_t *port_p = &bcmTmDrv_g.phy[phy].port[port];

            port_p->schedulerHandle = BCM_TM_SCHEDULER_HANDLE_INVALID;
        }
    }

    bcmTmDrv_masterConfig(1);

    /* reserve by marking the queue profile index#0 as used */
    bcmTmDrv_g.queueProfileMask[0] = 0x1;
}

/*
 *------------------------------------------------------------------------------
 * Function Name: bcmTmDrv_open
 * Description  : Function called when the module is opened.
 * Returns      : 0 - success
 *------------------------------------------------------------------------------
 */
static int bcmTmDrv_open(struct inode *inode, struct file *filp)
{
    __logNotice("BCM TM Char Device Opened");

    return 0;
}

/* Global file ops */
static struct file_operations bcmTmDrv_fops_g =
{
    .unlocked_ioctl = bcmTmDrv_ioctl,
#if defined(CONFIG_COMPAT)
    .compat_ioctl = bcmTmDrv_ioctl,
#endif
    .open = bcmTmDrv_open,
};

/*
 *------------------------------------------------------------------------------
 * Function Name: bcmTmDrv_construct
 * Description  : Function called when the module is loaded.
 * Returns      : None.
 *------------------------------------------------------------------------------
 */
static int __init bcmTmDrv_construct(void)
{
    int ret;

    ret = register_chrdev(BCM_TM_DRV_MAJOR, BCM_TM_DRV_NAME, &bcmTmDrv_fops_g);
    if(ret)
    {
        __logError("Cannot register_chrdev <%d>", BCM_TM_DRV_MAJOR);

        return ret;
    }

    ret = bcmTm_construct();
    if(ret)
    {
        unregister_chrdev(BCM_TM_DRV_MAJOR, BCM_TM_DRV_NAME);

        return ret;
    }

    bcmTmDrv_init();

    /* Register hooks */
    bcmFun_reg(BCM_FUN_ID_TM_REGISTER, bcmTmDrvHook_register);
    bcmFun_reg(BCM_FUN_ID_TM_PORT_CONFIG, bcmTmDrvHook_portConfig);
    bcmFun_reg(BCM_FUN_ID_TM_PORT_ENABLE, bcmTmDrvHook_portEnable);
    bcmFun_reg(BCM_FUN_ID_TM_ARBITER_CONFIG, bcmTmDrvHook_arbiterConfig);
    bcmFun_reg(BCM_FUN_ID_TM_QUEUE_CONFIG, bcmTmDrvHook_queueConfig);
    bcmFun_reg(BCM_FUN_ID_TM_APPLY, bcmTmDrvHook_apply);
    bcmFun_reg(BCM_FUN_ID_TM_ENQUEUE, bcmTmDrvHook_enqueue);

    __print(BCM_TM_MODNAME " Char Driver " BCM_TM_VER_STR " Registered <%d>\n", BCM_TM_DRV_MAJOR);

    /* debugging only */
    bcmLog_setLogLevel(BCM_LOG_ID_TM, BCM_LOG_LEVEL_ERROR);

    return 0;
}

/*
 *------------------------------------------------------------------------------
 * Function Name: bcmTmDrv_destruct
 * Description  : Function called when the module is unloaded.
 * Returns      : None.
 *------------------------------------------------------------------------------
 */
static void __exit bcmTmDrv_destruct(void)
{
    bcmTm_destruct();

    unregister_chrdev(BCM_TM_DRV_MAJOR, BCM_TM_DRV_NAME);

    /* Unregister hooks */
    bcmFun_dereg(BCM_FUN_ID_TM_REGISTER);
    bcmFun_dereg(BCM_FUN_ID_TM_PORT_CONFIG);
    bcmFun_dereg(BCM_FUN_ID_TM_PORT_ENABLE);
    bcmFun_dereg(BCM_FUN_ID_TM_ARBITER_CONFIG);
    bcmFun_dereg(BCM_FUN_ID_TM_QUEUE_CONFIG);
    bcmFun_dereg(BCM_FUN_ID_TM_APPLY);
    bcmFun_dereg(BCM_FUN_ID_TM_ENQUEUE);

    __logNotice(BCM_TM_MODNAME " Char Driver " BCM_TM_VER_STR " Unregistered <%d>", BCM_TM_DRV_MAJOR);
}

module_init(bcmTmDrv_construct);
module_exit(bcmTmDrv_destruct);

EXPORT_SYMBOL(bcmTmDrv_masterConfig);
EXPORT_SYMBOL(bcmTmDrv_portConfig);
EXPORT_SYMBOL(bcmTmDrv_getPortConfig);
EXPORT_SYMBOL(bcmTmDrv_getPortCapability);
EXPORT_SYMBOL(bcmTmDrv_queueConfig);
EXPORT_SYMBOL(bcmTmDrv_queueUnconfig);
EXPORT_SYMBOL(bcmTmDrv_getQueueConfig);
#if 0
EXPORT_SYMBOL(bcmTmDrv_allocQueueProfileId);
EXPORT_SYMBOL(bcmTmDrv_freeQueueProfileId);
EXPORT_SYMBOL(bcmTmDrv_queueProfileConfig);
EXPORT_SYMBOL(bcmTmDrv_getQueueProfileConfig);
EXPORT_SYMBOL(bcmTmDrv_queueDropAlgConfig);
EXPORT_SYMBOL(bcmTmDrv_getQueueDropAlgConfig);
EXPORT_SYMBOL(bcmTmDrv_checkSetHighPrio);
#endif
EXPORT_SYMBOL(bcmTmDrv_getQueueStats);
EXPORT_SYMBOL(bcmTmDrv_setQueueWeight);
EXPORT_SYMBOL(bcmTmDrv_arbiterConfig);
EXPORT_SYMBOL(bcmTmDrv_getArbiterConfig);
EXPORT_SYMBOL(bcmTmDrv_setPortMode);
EXPORT_SYMBOL(bcmTmDrv_getPortMode);
EXPORT_SYMBOL(bcmTmDrv_modeReset);
EXPORT_SYMBOL(bcmTmDrv_portEnable);
EXPORT_SYMBOL(bcmTmDrv_apply);

MODULE_DESCRIPTION(BCM_TM_MODNAME);
MODULE_VERSION(BCM_TM_VERSION);
MODULE_LICENSE("Proprietary");
