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

#ifdef BUILD_BRCM_CMS

#include "wlssk.h"
#include "event.h"

#include "cms.h"
#include "cms_msg.h"
#include "cms_mdm.h"
#include "cms_util.h"

#include "debug.h"
#include "nvram_api.h"

extern int terminalSignalReceived;
extern int restartRequired;
extern int restartTargetMask;

void *msgHandle = NULL;
void *tmrHandle = NULL;


#define COLLECT_RESTART_MSG_PERIOD 500 //ms

static void mark_restart(void *data)
{
    restartRequired = 1;
}


static void handleWlanChangedEvent(unsigned int data, char *cmd)
{
    if (cmsUtl_strcmp(cmd, "Restart") == 0)
    {
        restartTargetMask |= data;
        cmsTmr_replaceIfSooner(tmrHandle, mark_restart, 0, COLLECT_RESTART_MSG_PERIOD, "WLANChangeRestart");
        return;
    }
    
    if (strstr(cmd, "Hotplug") != NULL)
    {
       char ifname[BUF_SIZE_32], action[BUF_SIZE_32];
       sscanf(cmd, "Hotplug %32s %32s", ifname, action);
       DEBUG("ifname:%s action:%s\n", ifname, action);
       wlssk_hotplug_handler(ifname, action);
    }
}

static void cmsMessageListener()
{
    CmsMsgHeader* msg;
    char*         cmd = NULL;
    char          buf[BUF_SIZE_8] = {0};
    CmsRet        ret;

    // receive one message in queue
    if((ret = cmsMsg_receiveWithTimeout(msgHandle, &msg, COLLECT_RESTART_MSG_PERIOD)) == CMSRET_SUCCESS)
    {
        switch((UINT32) msg->type)
        {
            case CMS_MSG_SYSTEM_BOOT:
                break;
            case CMS_MSG_WLAN_CHANGED:
                cmd = (char*)(msg + 1);
                DEBUG("[%s] receive CMS_MSG_WLAN_CHANGED (data=%d)(cmd=%s)\n", __func__, (int)msg->wordData, cmd);
                handleWlanChangedEvent(msg->wordData, cmd);
                break;
            case CMS_MSG_SET_LOG_LEVEL:
                cmsLog_setLevel(msg->wordData);
                if ((ret = cmsMsg_sendReply(msgHandle, msg, CMSRET_SUCCESS)) != CMSRET_SUCCESS)
                {
                    cmsLog_error("send response for msg 0x%x failed, ret=%d", msg->type, ret);
                    break;
                }

                snprintf(buf, 8, "%d", msg->wordData);
                nvram_set("loglevel", buf);
                nvram_commit();
                break;

            case CMS_MSG_SET_LOG_DESTINATION:
                cmsLog_setDestination(msg->wordData);
                if ((ret = cmsMsg_sendReply(msgHandle, msg, CMSRET_SUCCESS)) != CMSRET_SUCCESS)
                {
                    cmsLog_error("send response for msg 0x%x failed, ret=%d", msg->type, ret);
                    break;
                }

                snprintf(buf, 8, "%d", msg->wordData);
                nvram_set("logmode", buf);
                nvram_commit();
                break;

            default:
                ERROR("unknown message type (%d)\n", msg->type);
        }

        CMSMEM_FREE_BUF_AND_NULL_PTR(msg);
    }

    // if SMD is disconnected, whole system would not work properly
    // simplly terminate itself
    if (ret == CMSRET_DISCONNECTED)
    {
        terminalSignalReceived = 1;
    }

}

static unsigned long timerChecker(void *data __attribute__((unused)))
{
    UINT32  nextEventMs;       

    /*
     * service all timer events that are due 
     * (there may be no events due if we woke up from externl events).
     */
    cmsTmr_executeExpiredEvents(tmrHandle);

    cmsTmr_getTimeToNextEvent(tmrHandle, &nextEventMs);

    return nextEventMs;
}

int cms_init(void)
{
    CmsRet ret = CMSRET_SUCCESS;
    SINT32 fd;
    SINT32 shmId=0;
    CmsMsgHeader* msg = NULL;

    cmsLog_notice("initializing timers");
    if ((ret = cmsTmr_init(&tmrHandle)) != CMSRET_SUCCESS)
    {
        cmsLog_error("cmsTmr_init failed, ret=%d", ret);
        return -1;
    }

    cmsLog_notice("calling cmsMsg_init");
    if ((ret = cmsMsg_init(EID_WLSSK, &msgHandle)) != CMSRET_SUCCESS)
    {
        cmsLog_error("cmsMsg_init failed, ret=%d", ret);
        return ret;
    }

    cmsLog_notice("calling cmsMdm_init with shmId=%d", shmId);
    if ((ret = cmsMdm_init(EID_WLSSK, msgHandle, &shmId)) != CMSRET_SUCCESS)
    {
        cmsMsg_cleanup(&msgHandle);
        cmsLog_error("cmsMdm_init error ret=%d", ret);
        return ret;
    }

    while((ret = cmsMsg_receiveWithTimeout(msgHandle, &msg, COLLECT_RESTART_MSG_PERIOD)) == CMSRET_SUCCESS)
    {
        cmsLog_debug("ignore message before init\n");
        CMSMEM_FREE_BUF_AND_NULL_PTR(msg);
    }


    ret = cmsMsg_getEventHandle(msgHandle, &fd);
    wlssk_register_listener(fd, cmsMessageListener, NULL);
    wlssk_register_timer(timerChecker, NULL);

    return ret;
}

void cms_cleanup()
{
    int fd;
    cmsMsg_getEventHandle(msgHandle, &fd);
    wlssk_deregister_listener(fd);

    cmsMdm_cleanup();
    cmsMsg_cleanup(&msgHandle);
}

void cms_update_associated_device(STAEVT_INFO event)
{
    CmsMsgHeader *msg = NULL;
    STAEVT_INFO  *payload;
    msg = (CmsMsgHeader *) cmsMem_alloc((sizeof(CmsMsgHeader)+ sizeof(STAEVT_INFO)), ALLOC_ZEROIZE);

    if (!msg)
    {
        cmsLog_error("ERROR: Fail to allocate message buffer");
        return;
    }

    payload = (STAEVT_INFO *) (msg+1);
    memcpy(payload, &event, sizeof(STAEVT_INFO));
    
    msg->src = EID_WLSSK;
    msg->dst = EID_SSK;
    msg->type = CMS_MSG_WIFI_UPDATE_ASSOCIATEDDEVICE;
    msg->flags_request = 1;
    msg->dataLength = sizeof(STAEVT_INFO);
    cmsMsg_send(msgHandle, msg);
    CMSMEM_FREE_BUF_AND_NULL_PTR(msg);
}

#endif /* BUILD_BRCM_CMS */
