/***********************************************************************
 *
 *  Copyright (c) 2010  Broadcom Corporation
 *  All Rights Reserved
 *
<:label-BRCM:2012:proprietary:standard

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

#ifdef SUPPORT_QOS

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <ctype.h>
#include <linux/if_ether.h>
#include <net/if.h>
#include <net/route.h>
#include <fcntl.h>
#include <unistd.h>

#include "bcmnet.h"
#include "cms_core.h"
#include "cms_util.h"
#include "cms_eid.h"
#include "cms_qos.h"
#include "cms_dal.h"
#include "cms_boardcmds.h"
#include "rcl.h"
#include "odl.h"
#include "rut_util.h"
#include "rut_qos.h"
#include "rut_atm.h"
#include "rut_dsl.h"
#include "rut_wan.h"
#include "rut_wanlayer2.h"
#include "rut_xtmlinkcfg.h"
#include "devctl_xtm.h"
#include "skb_defines.h"

#ifdef SUPPORT_RATE_LIMIT

/** Local functions **/


/***************************************************************************
// Function Name: rutQos_rateLimit
// Description  : execute tc commands to add or remove rate limit for a traffic class.
// Parameters   : clsObj - QoS class information.
//                qObj - egress queue information.
//                cmdType - command type either config or unconfig.
// Returns      : CmsRet.
****************************************************************************/
CmsRet rutQos_rateLimit(QosCommandType cmdType,
                        const char *queueIntfName,
                        UINT32 clsKey,
                        SINT32 classRate)
{
   char cmd[BUFLEN_1024]={0};
   UINT32 clsKeyMark = 0;
   CmsRet ret = CMSRET_SUCCESS;

   cmsLog_debug("Entered: cmdType=%d queueIntfName=%s clsKey=%d classRate=%d",
                cmdType, queueIntfName, clsKey, classRate);

   if (classRate == QOS_RESULT_NO_CHANGE)
   {
      return ret;
   }

   clsKeyMark = SKBMARK_SET_FLOW_ID(clsKeyMark, clsKey); 


   if (cmdType == QOS_COMMAND_CONFIG)
   {
      /* Add the root qdisc, I guess it does not hurt to add it multiple times. */
      sprintf(cmd, "tc qdisc add dev %s root handle 100: htb 2>/dev/null", queueIntfName);
      rut_doSystemAction("rutQos_rateLimit", cmd);

      /* add the individual rate limiter */
      sprintf(cmd, "tc class add dev %s parent 100: classid 100:%d htb rate %dkbit ceil %dkbit 2>/dev/null",
              queueIntfName, clsKey, classRate, classRate);
      rut_doSystemAction("rutQos_rateLimit", cmd);
      sprintf(cmd, "tc qdisc add dev %s parent 100:%d handle %d: pfifo limit 10 2>/dev/null",
              queueIntfName, clsKey, 100+clsKey);
      rut_doSystemAction("rutQos_rateLimit", cmd);
      sprintf(cmd, "tc filter add dev %s parent 100: protocol 0x%x prio 1 handle 0x%x/0x%x fw classid 100:%d 2>/dev/null",
              queueIntfName, ETH_P_ALL, clsKeyMark, SKBMARK_FLOW_ID_M, clsKey);
      rut_doSystemAction("rutQos_rateLimit", cmd);
   }
   else
   {
      /* delete the individual rate limiter */
      sprintf(cmd, "tc filter del dev %s parent 100: protocol 0x%x prio 1 handle 0x%x/0x%x fw classid 100:%d 2>/dev/null",
              queueIntfName, ETH_P_ALL, clsKeyMark, SKBMARK_FLOW_ID_M, clsKey);
      rut_doSystemAction("rutQos_rateLimit", cmd);
      sprintf(cmd, "tc qdisc del dev %s parent 100:%d handle %d: pfifo limit 10 2>/dev/null",
              queueIntfName, clsKey, 100+clsKey);
      rut_doSystemAction("rutQos_rateLimit", cmd);
      sprintf(cmd, "tc class del dev %s parent 100: classid 100:%d htb rate %dkbit ceil %dkbit 2>/dev/null",
              queueIntfName, clsKey, classRate, classRate);
      rut_doSystemAction("rutQos_rateLimit", cmd);
      

      /* if this was the last one, delete the whole thing */
      if (rutQos_isAnotherClassRateLimitExist(clsKey, queueIntfName) == FALSE)
      {
         sprintf(cmd, "tc qdisc del dev %s root handle 100: htb 2>/dev/null", queueIntfName);
         rut_doSystemAction("rutQos_rateLimit", cmd);
      }
   }

   return ret;

}  /* End of rutQos_rateLimit() */


#endif  /* SUPPORT_RATE_LIMIT */


#endif  /* SUPPORT_QOS */


