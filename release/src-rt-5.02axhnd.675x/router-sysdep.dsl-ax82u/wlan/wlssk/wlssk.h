/***********************************************************************
 *
 *  Copyright (c) 2018  Broadcom
 *  All Rights Reserved
 *
<:label-BRCM:2018:proprietary:standard

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
 *
 ************************************************************************/

#ifndef __WLSSK_H__
#define __WLSSK_H__

#include "bcmconfig.h"
#include "wlsyscall.h"

typedef void (*EventListenerFunc)();
typedef unsigned long (*TimerFunc)(void *data);

#define WLSSK_DELAY_TIMEOUT 3000  // MS
#define SOCK_RCV_BUF_LEN (4096)

#define RESTART_MASK_1  0X1
#define RESTART_MASK_2  0X2
#define RESTART_MASK_3  0X4
#define RESTART_ALL     0XF

#define STA_DISCONNECTED (0)
#define STA_CONNECTED (1)
#define UPDATE_STA_ALL (2)
#define UPDATE_STA_REMOVE_ALL (3)

#define bcmSystem(cmd)		system(cmd)


#define BUF_SIZE_8     8
#define BUF_SIZE_32    32
#define BUF_SIZE_256   256
#define MAX_BUF_SIZE   1024
#define CMD_BUF_SIZE   MAX_BUF_SIZE
#define NVNAME_SIZE    64           //keep consistent with nvc.h
#define IFNAME_LENGTH  16
#define MAX_BR_NUM     8

/**
 *
 *
 */
void wlssk_init(void);

void wlssk_cleanup(void);

void wlssk_restart(unsigned long targets);

/** Execute once for wlssk.
 *  IN:
 *     timeout for this execution. 
 *  OUT:
 *     if success, return positive integer as the next timeout
 *     return -1 when fail.
 */
unsigned long wlssk_run_once(unsigned long timeout);


/** Register fd listener for receiving external events.
 *
 *  @param  fd   file descriptor or socket
 *  @param  func callback function
 *  @param  data the user data would pass into callback function
 *  
 *  @return 0 if success and -1  as fail.
 */
int wlssk_register_listener(int fd, EventListenerFunc func, void *data);

/** Deregister listener by fd.
 *  IN:
 *     fd   file descriptor or socket
 */
void wlssk_deregister_listener(int fd);

/** Register a timer and its handle callback function.
 *  
 *  @param  func callback function
 *  @param  data the user data would pass into callback function
 *  
 *  @return 0 if success.
 */
int wlssk_register_timer(TimerFunc func, void *data);

void wlssk_hotplug_handler(char *ifname, char *action);
#endif //__WLSSK_H__

