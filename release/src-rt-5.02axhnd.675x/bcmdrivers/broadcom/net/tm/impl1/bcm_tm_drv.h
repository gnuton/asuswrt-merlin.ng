#ifndef __BCM_TM_DRV_H_INCLUDED__
#define __BCM_TM_DRV_H_INCLUDED__

/*
  <:copyright-BRCM:2007:DUAL/GPL:standard
  
     Copyright (c) 2007 Broadcom 
     All Rights Reserved
  
  Unless you and Broadcom execute a separate written software license
  agreement governing use of this software, this software is licensed
  to you under the terms of the GNU General Public License version 2
  (the "GPL"), available at http://www.broadcom.com/licenses/GPLv2.php,
  with the following added to such license:
  
     As a special exception, the copyright holders of this software give
     you permission to link this software with independent modules, and
     to copy and distribute the resulting executable under terms of your
     choice, provided that you also meet, for each linked independent
     module, the terms and conditions of the license of that module.
     An independent module is a module which is not derived from this
     software.  The special exception does not apply to any modifications
     of the software.
  
  Not withstanding the above, under no circumstances may you combine
  this software in any way with any other Broadcom software provided
  under a license other than the GPL, without Broadcom's express prior
  written consent.
  
  :>
*/

/*
*******************************************************************************
* File Name  : bcm_tm_drv.h
*
* Description: This file contains the Broadcom Traffic Manager Driver API.
*
*******************************************************************************
*/

#include "bcm_tm_defs.h"

#define BCM_TM_DRV_BUCKET_SIZE_MAX   65535
#define BCM_TM_DRV_KBPS_MAX          1000000

/* Management API */
void bcmTmDrv_masterConfig(int enable);
int bcmTmDrv_register(bcmTmDrv_phyType_t phy, uint8_t port,
                      uint32_t nbrOfQueues, uint32_t nbrOfEntries, uint32_t paramSize,
                      bcmTm_txCallback_t txCallbackFunc, bcmTm_freeCallback_t freeCallbackFunc);
int bcmTmDrv_portConfig(bcmTmDrv_phyType_t phy, uint8_t port, bcmTmDrv_mode_t mode,
                        int kbps, int mbs, bcmTmDrv_shapingType_t shapingType);
int bcmTmDrv_getPortConfig(bcmTmDrv_phyType_t phy, uint8_t port, bcmTmDrv_mode_t mode, int *kbps_p, int *mbs_p,
                           bcmTmDrv_shapingType_t *shapingType_p, uint32_t *nbrOfQueues_p, uint32_t *nbrOfEntries_p,
                           uint32_t *paramSize_p);
int bcmTmDrv_getPortCapability(bcmTmDrv_phyType_t phy, uint8_t port, uint32_t *schedType_p, int *maxQueues_p,
                               int *maxSpQueues_p, uint8_t *portShaper_p, uint8_t *queueShaper_p);                       
int bcmTmDrv_setPortMode(bcmTmDrv_phyType_t phy, uint8_t port, bcmTmDrv_mode_t mode);
bcmTmDrv_mode_t bcmTmDrv_getPortMode(bcmTmDrv_phyType_t phy, uint8_t port);
int bcmTmDrv_modeReset(bcmTmDrv_phyType_t phy, uint8_t port, bcmTmDrv_mode_t mode);
int bcmTmDrv_portEnable(bcmTmDrv_phyType_t phy, uint8_t port, bcmTmDrv_mode_t mode, int enable);
int bcmTmDrv_pauseEnable(bcmTmDrv_phyType_t phy, uint8_t port, int enable);
int bcmTmDrv_apply(bcmTmDrv_phyType_t phy, uint8_t port);
int bcmTmDrv_queueConfig(bcmTmDrv_phyType_t phy, uint8_t port, bcmTmDrv_mode_t mode, uint8_t queue,
                         uint8_t shaperType, int kbps, int mbs);
int bcmTmDrv_queueUnconfig(bcmTmDrv_phyType_t phy, uint8_t port, bcmTmDrv_mode_t mode, uint8_t queue);
int bcmTmDrv_getQueueConfig(bcmTmDrv_phyType_t phy, uint8_t port, bcmTmDrv_mode_t mode, uint8_t queue,
                            int *kbps_p, int *minKbps_p, int *mbs_p, int *weight_p, int *qsize_p);
int bcmTmDrv_allocQueueProfileId(int *queueProfileId_p);
int bcmTmDrv_freeQueueProfileId(int queueProfileId);
int bcmTmDrv_queueProfileConfig(int queueProfileId, int dropProbability, int minThreshold,
                                int maxThreshold);
int bcmTmDrv_getQueueProfileConfig(int queueProfileId, int *dropProbability_p,
                                   int *minThreshold_p, int *maxThreshold_p);
int bcmTmDrv_queueDropAlgConfig(bcmTmDrv_phyType_t phy, uint8_t port, uint8_t queue, bcmTmDrv_dropAlg_t dropAlgorithm,
                                int queueProfileIdLo, int queueProfileIdHi,
                                uint32_t priorityMask0, uint32_t priorityMask1);
int bcmTmDrv_getQueueDropAlgConfig(bcmTmDrv_phyType_t phy, uint8_t port, uint8_t queue, bcmTmDrv_dropAlg_t *dropAlgorithm_p,
                                   int *queueProfileIdLo_p, int *queueProfileIdHi_p,
                                   uint32_t *priorityMask0_p, uint32_t *priorityMask1_p);
int bcmTmDrv_checkHighPrio(bcmTmDrv_phyType_t phy, uint8_t port, uint8_t queue, uint8_t chnl, uint32_t tc);
int bcmTmDrv_checkSetHighPrio(bcmTmDrv_phyType_t phy, uint8_t port, uint8_t queue, uint32_t tc, uint32_t *destQueue_p);
int bcmTmDrv_xtmCheckHighPrio(uint8_t chnl, uint32_t tc);
#if defined(CONFIG_BCM_XTMCFG) || defined(CONFIG_BCM_XTMCFG_MODULE)
int bcmTmDrv_XtmQueueDropAlgConfig(uint8_t chnl, bcmTmDrv_dropAlg_t dropAlgorithm,
                                   int queueProfileIdLo, int queueProfileIdHi,
                                   uint32_t priorityMask0, uint32_t priorityMask1);
int bcmTmDrv_getXtmQueueDropAlgConfig(uint8_t chnl, bcmTmDrv_dropAlg_t *dropAlgorithm_p,
                                      int *queueProfileIdLo_p, int *queueProfileIdHi_p,
                                      uint32_t *priorityMask0_p, uint32_t *priorityMask1_p);
#endif
int bcmTmDrv_getQueueStats(bcmTmDrv_phyType_t phy, uint8_t port, bcmTmDrv_mode_t mode, uint8_t queue,
                           uint32_t *txPackets_p, uint32_t *txBytes_p,
                           uint32_t *droppedPackets_p, uint32_t *droppedBytes_p,
                           uint32_t *bps_p);
int bcmTmDrv_arbiterConfig(bcmTmDrv_phyType_t phy, uint8_t port, bcmTmDrv_mode_t mode, uint8_t arbiterType, uint8_t arbiterArg);
int bcmTmDrv_getArbiterConfig(bcmTmDrv_phyType_t phy, uint8_t port, bcmTmDrv_mode_t mode, bcmTm_arbiterType_t *arbiterType_p, int *arbiterArg_p);
int bcmTmDrv_setQueueWeight(bcmTmDrv_phyType_t phy, uint8_t port, bcmTmDrv_mode_t mode, uint8_t queue, uint8_t weight);
int bcmTmDrv_getQueueRate(bcmTmDrv_phyType_t phy, uint8_t port, uint8_t queue);

int bcmTmDrv_enqueue(bcmTm_schedulerHandle_t schedulerHandle, uint32_t queue, uint16_t length, void *param_p);

/* Stats/Debuggging */
void bcmTmDrv_status(void);
int bcmTmDrv_stats(bcmTmDrv_phyType_t phy, uint8_t port);

#endif /* __BCM_TM_DRV_H_INCLUDED__ */
