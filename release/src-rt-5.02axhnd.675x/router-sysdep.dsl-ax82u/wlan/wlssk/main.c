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

#include <string.h>
#include <stdlib.h>
#include <signal.h>

#include "bcmnvram.h"
#include "wlssk.h"
#include "debug.h"
#include "event.h"
#include "wlsysutil.h"

/* for the Broadcom board driver ioctl codes, in bcmdrivers/opensource/include/bcm963xx */
#include <sys/stat.h>
#include "board.h"
#include "bcm_boarddriverctl.h"

#ifdef BUILD_BRCM_CMS
extern int  cms_init(void);
extern void cms_cleanup();
#endif

int act_wl_cnt = 0;
int terminalSignalReceived = 0;

int restartRequired        = 0;
unsigned long restartTargetMask      = 0x00000;

char *dbgMonitorMtd = NULL;
char *dbgMonitorDir = NULL;

static int initialized  = 1;

static void singal_handler(int sigNum)
{
    if (sigNum == SIGUSR2)
    {
        restartRequired = 1;
        restartTargetMask |= RESTART_ALL; 
    }
    else if (sigNum == SIGTERM)
    {
        terminalSignalReceived = 1;
    }
}
void run_rc_local(void)
{
    int rc;
    char cmd[1024]={0};
    struct stat tmp_stat;
    rc = devCtl_boardIoctl(BOARD_IOCTL_FLASH_READ, SCRATCH_PAD, "rclocal", 0, sizeof(cmd), cmd);
    if (rc >= 0 && stat(cmd, &tmp_stat) == 0 )
    {
        bcmSystem(cmd);
    }
}

int main(int argc, char** argv)
{
    int ret;
    unsigned long timeout = WLSSK_DELAY_TIMEOUT;
    char varValue[8]= {0};

    run_rc_local();

    act_wl_cnt = wlgetintfNo();
    if (!act_wl_cnt )
    {
        fprintf(stdout, "\nNO_WL_ADAPTER !!!!!\n");
        exit(-1);
    }
    /* set signal masks */
    signal(SIGPIPE, SIG_IGN); /* Ignore SIGPIPE signals */

    log_init();

#ifdef BUILD_BRCM_CMS
    if ((ret = cms_init()) != 0)
    {
        goto exit_log;
    }
#endif


    if ((ret = wlevent_init()) != 0)
    {
        initialized = FALSE;
    }

    sprintf(varValue, "%d", act_wl_cnt);
    // for wifi Web-CGI usage.
    // we shall review these in a mean time if web design changed.
    nvram_set("wlInterface", varValue);

    wlssk_init();


    signal(SIGUSR2, singal_handler);
    signal(SIGTERM, singal_handler);

    if (!initialized)
       goto exit; 

    while (!terminalSignalReceived && timeout >= 0)
    {
        timeout = wlssk_run_once(timeout);

        if (restartRequired)
        {
            DEBUG("process wlssk restart\n");
            restartRequired = FALSE;
            wlssk_restart(restartTargetMask);
            DEBUG("done !!!\n");
            restartTargetMask = 0;
        }
    }

exit:
    wlevent_cleanup();
    wlssk_cleanup();

#ifdef BUILD_BRCM_CMS
    cms_cleanup();
#endif

exit_log:
    log_cleanup();

    return ret;
} /* main */
