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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <signal.h>
#include <netinet/if_ether.h>
#include <ctype.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>


#include "board.h"
#include "debug.h"

#include <bcmnvram.h>

#include "wlsyscall.h"
#include "wlssk.h"

#include "wlsysutil.h"
#include "nvram_api.h"

#include <sys/socket.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/if_packet.h>

#include <pwd.h>
#include <sys/utsname.h>
#include <dirent.h>
#include <os_defs.h>

extern int act_wl_cnt;
extern unsigned long gpioOverlays;
extern int dhd_probe(char *name);
extern char *dbgMonitorMtd;
extern char *dbgMonitorDir;

void clearSesLed(void)
{
    int f = open( "/dev/brcmboard", O_RDWR );
    /* set led off */
    if( f > 0 ) {
        int  led = 0;
        BOARD_IOCTL_PARMS IoctlParms;
        memset( &IoctlParms, 0x00, sizeof(IoctlParms) );
        IoctlParms.result = -1;
        IoctlParms.string = (char *)&led;
        IoctlParms.strLen = sizeof(led);
        ioctl(f, BOARD_IOCTL_SET_SES_LED, &IoctlParms);
        close(f);
    }
}

static int getRandomBytes(unsigned char* bytes, int len)
{
    int dev_random_fd;

    dev_random_fd = open("/dev/urandom", O_RDONLY|O_NONBLOCK);
    if (dev_random_fd < 0)
    {
        ERROR("Could not open /dev/urandom\n");
        return -1;
    }

    read(dev_random_fd, bytes, len);
    close(dev_random_fd);
    return 0;
}

void genWscPin(void)
{
    unsigned long PIN = 0;
    unsigned long accum = 0;
    int digit;
    char devPwd[BUF_SIZE_32] = {0};
    unsigned long randNum = 0;

    srand(time((time_t *)NULL));
    if(!getRandomBytes((unsigned char*)&randNum, sizeof(unsigned long))) 
    {
        snprintf( devPwd, sizeof(devPwd), "%08lu", randNum);
        devPwd[7] = '\0';
        PIN = strtoul( devPwd, NULL, 10 );
        PIN *= 10;
        accum += 3 * ((PIN / 10000000) % 10);
        accum += 1 * ((PIN / 1000000) % 10);
        accum += 3 * ((PIN / 100000) % 10);
        accum += 1 * ((PIN / 10000) % 10);
        accum += 3 * ((PIN / 1000) % 10);
        accum += 1 * ((PIN / 100) % 10);
        accum += 3 * ((PIN / 10) % 10);
        digit = (accum % 10);
        accum = (10 - digit) % 10;
        PIN += accum;
        snprintf( devPwd, sizeof(devPwd), "%08lu", PIN );
        devPwd[8] = '\0';
    }

    printf("WPS Device PIN = %s\n", devPwd);
    if(strlen(devPwd)>1)
        nvram_set("wps_device_pin", devPwd);
    else
        nvram_set("wps_device_pin", "12345670");
}

unsigned long bcmGetGPIOOverlays(void)
{
    int f = open( "/dev/brcmboard", O_RDWR );
    unsigned long  GPIOOverlays = 0;
    if( f > 0 ) {
        BOARD_IOCTL_PARMS IoctlParms;
        memset( &IoctlParms, 0x00, sizeof(IoctlParms) );
        IoctlParms.result = -1;
        IoctlParms.string = (char *)&GPIOOverlays;
        IoctlParms.strLen = sizeof(GPIOOverlays);
        ioctl(f, BOARD_IOCTL_GET_GPIOVERLAYS, &IoctlParms);
        //printf("GPIOOverlay---: 0x%lu \r\n",GPIOOverlays );
        close(f);
    }
    return GPIOOverlays;
}

int getHwAddress(const char* ifname, char* hwaddr)
{
    struct ifreq s;
    int ret = 1;
    int fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);

    if (fd < 0)
        return ret;

    strcpy(s.ifr_name, ifname);
    if (0 == ioctl(fd, SIOCGIFHWADDR, &s)) {
        sprintf(hwaddr, "%02x:%02x:%02x:%02x:%02x:%02x", (unsigned char) s.ifr_addr.sa_data[0],
                                                         (unsigned char) s.ifr_addr.sa_data[1],
                                                         (unsigned char) s.ifr_addr.sa_data[2],
                                                         (unsigned char) s.ifr_addr.sa_data[3],
                                                         (unsigned char) s.ifr_addr.sa_data[4],
                                                         (unsigned char) s.ifr_addr.sa_data[5]);
        ret = 0;
    }
    
    close(fd);
    return ret;
}

int getMACBuf(const char* ifname, unsigned char* buf)
{
    struct ifreq s;
    int i, ret = 1;
    int fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);

    if (fd < 0)
        return ret;

    strcpy(s.ifr_name, ifname);
    if (0 == ioctl(fd, SIOCGIFHWADDR, &s)) {
        for (i = 0 ; i < 6 ; i++)
            buf[i] = (unsigned char) s.ifr_addr.sa_data[i];
        ret = 0;
    }
    
    close(fd);
    return ret;
}

#ifdef IDLE_PWRSAVE
static char *bcmGetWlName(int idx, int ssid_idx, char *ifcName)
{
   if ( !ifcName ) 
   	return NULL;

   if (ssid_idx == 0)
         sprintf(ifcName, "wl%d", idx);
   else
         sprintf(ifcName, "wl%d.%d",idx, ssid_idx);

   return ifcName;
}

void setASPM(const unsigned int idx, const int enabled)
{
    char ifname[IFNAME_LENGTH];
    char cmd[CMD_BUF_SIZE];
    char wlcmd[4];
    int cfg_value = 0;

    snprintf(ifname, sizeof(ifname), "wl%d", idx);
    /* dhd_probe() returns 0 if using dhd driver */
    if(!dhd_probe(ifname))
        strcpy(wlcmd, "dhd");
    else
        strcpy(wlcmd, "wl");

    if(enabled)
    {
        /* Only enable L1 mode, because L0s creates EVM issues. The power savings are the same */
        if (gpioOverlays & BP_OVERLAY_PCIE_CLKREQ) {
            /* aspm=0x102 is to enable CLKFREQ and ASPM L1 */
            cfg_value = 0x102;
        } else {
            /* aspm=0x2 is to enable ASPM L1 */
            cfg_value = 0x2;
        }
    }
    snprintf(cmd, sizeof(cmd), "%s -i %s aspm 0x%x", wlcmd, ifname, cfg_value);
    bcmSystem(cmd);
}

void bcmTogglePowerSave(void)
{
    char nvname[NVNAME_SIZE]; 
    char ifcName[IFNAME_LENGTH]={0};
    char cmd[CMD_BUF_SIZE];
    char *nv_value = NULL;
    int idx, subIdx;
    int isAssociated = 0, pwrsave_enable = 0;

    for (idx = 0 ; idx < act_wl_cnt ; idx++)
    {
        int NumOfVifs = wlgetVirtIntfNo(idx);
        isAssociated = 0;
        pwrsave_enable = 0;
        for (subIdx = 1 ; subIdx < NumOfVifs ; subIdx++)
        {
            bcmGetWlName(idx, subIdx, ifcName);
            snprintf(cmd, sizeof(cmd), "wl -i %s assoclist | grep :", ifcName);
            isAssociated |= !bcmSystem(cmd); //if any associated station, return 0
        }

        if (!isAssociated) /* turn off pwrsave when any client is associated */
        {
            snprintf(nvname, sizeof(nvname), "wl%d_rxchain_pwrsave_enable", idx);
            nv_value = nvram_unf_get(nvname);
            if (nv_value)
            {
                if (*nv_value == '1')
                    pwrsave_enable = 1;
                free(nv_value);
            }
        }

        snprintf(cmd, sizeof(cmd), "wl -i wl%d rxchain_pwrsave_enable %d", idx, pwrsave_enable);
        bcmSystem(cmd);

        snprintf(nvname, sizeof(nvname), "wl%d_phytype", idx);
        nv_value = nvram_unf_get(nvname);
        if (nv_value)
        {
            if (*nv_value == 'v') // only ac mode should adjust ASPM
                setASPM(idx, !isAssociated);
            free(nv_value);
        }
    }

}
#endif

int isAdapterEnabled(const char* ifname)
{
    char nvname[NVNAME_SIZE] = {0};
    char *radio = NULL;
    int ret = 0;

    if (!ifname)
        return ret;

    snprintf(nvname, sizeof(nvname), "%s_radio", ifname);
    radio = nvram_unf_get(nvname);
    if (radio)
    {
        if (radio[0] == '1')
            ret = 1;
        free(radio);
    }

    return ret;
}

int isInterfaceEnabled(const char* ifname)
{
    char nvname[NVNAME_SIZE] = {0};
    char *bssEnabled;
    int ret = 0;

    if (!ifname)
        return ret;

    snprintf(nvname, sizeof(nvname), "%s_bss_enabled", ifname);
    bssEnabled = nvram_unf_get(nvname);
    if (bssEnabled)
    {
        if (bssEnabled[0] == '1')
            ret = 1;
        free(bssEnabled);
    }

    return ret;
}

void clientMode_BSS_downup(void)
{
    char ifname[IFNAME_LENGTH] = {0};
    char cmd[CMD_BUF_SIZE] = {0};
    char nvname[NVNAME_SIZE] = {0};
    int idx, subIdx;
    char *nv_value;
    
    for (idx = 0 ; idx < act_wl_cnt ; idx++ )
    {
        int NumOfVifs = wlgetVirtIntfNo(idx);
        for (subIdx = 0 ; subIdx < NumOfVifs ; subIdx++)
        {
            if (subIdx == 0)
                snprintf(ifname, sizeof(ifname), "wl%d", idx);
            else
                snprintf(ifname, sizeof(ifname), "wl%d.%d", idx, subIdx);

            if (isInterfaceEnabled(ifname))
            {
                snprintf(nvname, sizeof(nvname), "%s_mode", ifname);
                nv_value = nvram_unf_get(nvname);

                if (nv_value)
                {
                    if (strcmp(nv_value, "ap") != 0) // non-AP mode (Implicit sta mode)
                    {
                        snprintf(cmd, sizeof(cmd), "wl -i %s bss down ; wl -i %s bss up 2> /dev/null", ifname,ifname);
                        bcmSystem(cmd);
                    }
                    free(nv_value);                    
                }
            }
        }
    }
}

void crash_log_backup_init(void)
{
    char cmd[CMD_BUF_SIZE];
    dbgMonitorMtd = nvram_unf_get("crash_log_backup_mtd");
    dbgMonitorDir = nvram_unf_get("crash_log_backup_dir");

    if (dbgMonitorDir == NULL)
    {
        nvram_set("crash_log_backup_dir", "/mnt/crash_logs");
        dbgMonitorDir = nvram_unf_get("crash_log_backup_dir");
    }
    snprintf(cmd, CMD_BUF_SIZE-1, "mkdir -p %s", dbgMonitorDir);
    bcmSystem(cmd);

    if (dbgMonitorMtd != NULL)
    {
        snprintf(cmd, CMD_BUF_SIZE-1, "hnddm.sh %s %s 2> /dev/null", dbgMonitorMtd, dbgMonitorDir);
        bcmSystem(cmd);
    }
}

void crash_log_backup_reinit(void)
{
    int needUnmount = FALSE, needMount = FALSE;
    char cmd[CMD_BUF_SIZE];
    char *currentMtd = dbgMonitorMtd;
    char *currentDir = dbgMonitorDir;
    
    dbgMonitorMtd = nvram_unf_get("crash_log_backup_mtd");
    dbgMonitorDir = nvram_unf_get("crash_log_backup_dir");

    DEBUG("curMtd:%s curDir:%s\n", currentMtd, currentDir);
    DEBUG("dbgMonitorMtd:%s dbgMonitorDir:%s\n", dbgMonitorMtd, dbgMonitorDir);

    if ((currentMtd && currentDir) && !(dbgMonitorMtd && dbgMonitorDir)) //unset mtd or dir
    {
        needUnmount = TRUE;
    }

    if (dbgMonitorMtd && dbgMonitorDir)
    {
        needMount = TRUE;
        if (currentMtd && currentDir)
        {
            if (strcmp(currentMtd, dbgMonitorMtd) || strcmp(currentDir, dbgMonitorDir))
               needUnmount = TRUE; 

            if (strcmp(currentMtd, dbgMonitorMtd)==0 && strcmp(currentDir, dbgMonitorDir)==0) // all the same
               needMount=FALSE;
        }
    }
    DEBUG("[%s] mount:%d unmount:%d \n", needMount, needUnmount);

    if (needMount)
    {
        snprintf(cmd, CMD_BUF_SIZE-1, "mkdir -p %s", dbgMonitorDir);
        bcmSystem(cmd);
    }

    if (needUnmount && needMount)
    {
        snprintf(cmd, CMD_BUF_SIZE, "hnddm.sh %s %s %s %s 2> /dev/null", dbgMonitorMtd, dbgMonitorDir, currentMtd, currentDir);
        bcmSystem(cmd);
    }
    else if (needUnmount) // only unmount
    {
        snprintf(cmd, CMD_BUF_SIZE, "hnddm.sh \"\" \"\" %s %s 2> /dev/null", currentMtd, currentDir);
        bcmSystem(cmd);
    }
    else if (needMount) // only mount
    {
        snprintf(cmd, CMD_BUF_SIZE-1, "hnddm.sh %s %s 2> /dev/null", dbgMonitorMtd, dbgMonitorDir);
        bcmSystem(cmd);
    }

    if (currentMtd)
        free(currentMtd);

    if (currentDir)
        free(currentDir);
} 

void setup_mbss_Mac_addr(void)
{
    char ifname[IFNAME_LENGTH];
    char macAddr[BUF_SIZE_32];
    unsigned char macBuf[6];
    unsigned char macs[BUF_SIZE_32];
    char nvname[NVNAME_SIZE] = {0};
    int idx, i, j, collision;

    for (idx = 0 ; idx < act_wl_cnt ; idx++ )
    {
        int NumOfVifs = wlgetVirtIntfNo(idx);

        snprintf(ifname, sizeof(ifname), "wl%d", idx);
        getHwAddress(ifname, macAddr);
        snprintf(nvname, sizeof(nvname), "wl%d_hwaddr", idx);
        nvram_set(nvname, macAddr);
        
        // make local address
        getMACBuf(ifname, macBuf);
        macBuf[0] = 96 + (macBuf[5] % (NumOfVifs -1) * 8);
        macBuf[0] |= 0x02;

        for (i = 1 ; i < NumOfVifs ; i++)
        {
            // construct virtual hw_addr
            macBuf[5] = (macBuf[5]&~(NumOfVifs-1))|((NumOfVifs-1)&(macBuf[5]+1));
            do {
                collision = FALSE;
                for(j=0; j<(i-1); j++) {
                    if(macs[j]== macBuf[5]) {
                        collision=TRUE;
                        macBuf[5]++;
                        break;
                    }
                }
            } while(collision);
            macs[i-1]=macBuf[5];
            snprintf(macAddr, sizeof(macAddr), "%02x:%02x:%02x:%02x:%02x:%02x", macBuf[0], macBuf[1], macBuf[2], macBuf[3], macBuf[4], macBuf[5]);
            snprintf(nvname, sizeof(nvname), "wl%d.%d_hwaddr", idx, i);
            nvram_set(nvname, macAddr);
        }
    
    }
}

int
dpsta_ioctl(char *name, void *buf, int len)
{
	struct ifreq ifr;
	int ret = 0;
	int s;

	/* open socket to kernel */
	if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("socket");
		return errno;
	}

	strncpy(ifr.ifr_name, name, IFNAMSIZ);
	ifr.ifr_name[sizeof(ifr.ifr_name) - 1] = '\0';
	ifr.ifr_data = (caddr_t)buf;
	if ((ret = ioctl(s, SIOCDEVPRIVATE, &ifr)) < 0)
		perror(ifr.ifr_name);

	/* cleanup */
	close(s);
	return ret;
}
//endof wsyscall.c

