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
#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>


#include "board.h"
#include "wlssk.h"
#include "service.h"
#include "wlsyscall.h"
#include "debug.h"

#include "bcmnvram.h"
#include <shutils.h>


#define DEFAULT_CRASH_LOG_DIR  "/mnt/crash_logs"

extern int act_wl_cnt;

extern void clientMode_BSS_downup();

extern int start_hapd_wpasupp();
extern void stop_hapd_wpasupp();

static bool detectApp(char *app)
{
    FILE *fp;
    char *bin_folder[] = {"/bin", "/usr/sbin", NULL};
    char temp[128];
    int index = -1;

    while(bin_folder[++index])
    {
        snprintf(temp, 128, "%s/%s", bin_folder[index], app);
        fp = fopen(temp, "r");
        if ( fp != NULL ) {
            fclose(fp);
            return TRUE;
        }
    }
    return FALSE;
}

static void start_ceventd(void)
{
    char *ceventdEnable = NULL;

    if (!detectApp("ceventd"))
        return;

    ceventdEnable = nvram_unf_get("ceventd_enable");
    if (ceventdEnable && strcmp(ceventdEnable, "1") == 0)
    {
        bcmSystem("ceventd");
        free(ceventdEnable);
    }
}


static void start_debug_monitor(int start)
{
    char monitor[16] = "debug_monitor";
    char cmd[CMD_BUF_SIZE];
    char *dir;
    char *nv_debug_monitor;

    crash_log_backup_reinit();

    /* Kill previous instance of monitor before invoking new one */
    snprintf(cmd, sizeof(cmd), "killall -q -9 %s 2>/dev/null", monitor);
    bcmSystem(cmd);

    bcmSystem("mkdir -p /tmp/dm");
    nv_debug_monitor = nvram_unf_get("debug_monitor_enable");
    if (nv_debug_monitor)
    {
        if ((nv_debug_monitor[0]=='0') && (nv_debug_monitor[1]=='\0'))
        {
            bcmSystem("rm -rf /tmp/dm");
            start = 0;
        }
        free(nv_debug_monitor);
    }

    /* all interface is disabled or debug_monitor is disabled */
    if (!start) return;

    dir = nvram_unf_get("crash_log_backup_dir");
    if (dir)
    {
        snprintf(cmd, sizeof(cmd), "mkdir -p %s", dir);
        bcmSystem(cmd);
        snprintf(cmd, sizeof(cmd), "%s %s", monitor, dir);
        free(dir);
    }
    else
    {
        /* assign default path for storing crash logs */
        nvram_set("crash_log_backup_dir", DEFAULT_CRASH_LOG_DIR);
        bcmSystem("mkdir -p "DEFAULT_CRASH_LOG_DIR);
        snprintf(cmd, sizeof(cmd), "%s %s", monitor, DEFAULT_CRASH_LOG_DIR);
    }

    bcmSystem(cmd);
} 

int stop_debug_monitor(void)
{
    /* remove debug_monitor directory */
    system("rm -rf /tmp/dm/*");
    /* Don't kill debug_monitor here */
    return 0;
}


#if defined(SUPPORT_WSC)
void start_Wsc(void)
{
    char *unit = NULL;
    char ifname[IFNAME_LENGTH] = {0};
    char nvname[NVNAME_SIZE] = {0};
    char ifnames[128];
    char *buf = NULL;
    char *wps_version2 = NULL;
    int br;

    unit = nvram_unf_get("wl_unit");

    if (unit)
    {
        if (strlen(unit) != 0)
            snprintf(ifname, IFNAME_LENGTH, "wl%s", unit);
        else
            snprintf(ifname, IFNAME_LENGTH, "wl0");
        free(unit);
    }
    else
        snprintf(ifname, IFNAME_LENGTH, "wl0");

    NVRAM_UNIT_COPY("wps_mode", ifname);
    NVRAM_UNIT_COPY("wps_config_state", ifname); // 1/0

    nvram_set("wl_wps_reg", "enabled");

    /* Since 5.22.76 release, WPS IR is changed to per Bridge. Previous IR enabled/disabled is
    Per Wlan Intf */
    for ( br=0; br<MAX_BR_NUM; br++ ) {
        if ( br == 0 )
            snprintf(nvname, sizeof(nvname), "lan_ifnames");
        else
            snprintf(nvname, sizeof(nvname), "lan%d_ifnames", br);

        buf = nvram_unf_get(nvname);
        if(buf)
        {
            strncpy(ifnames, buf, sizeof(ifnames));
            free(buf);
        }
        else
            continue;

        if (ifnames[0] =='\0')
            continue;
        if ( br == 0 )
            snprintf(nvname, sizeof(nvname), "lan_wps_reg");
        else
            snprintf(nvname, sizeof(nvname), "lan%d_wps_reg", br);

        nvram_set(nvname, "enabled");
    }

    wps_version2 = nvram_unf_get("wps_version2");
    if (wps_version2 == NULL)
        nvram_set("wps_version2", "enabled");
    else
        free(wps_version2);

    if (strlen(nvram_safe_get("wps_device_pin")) != 8)
        genWscPin();

    if (!nvram_match("wps_restart", "1"))
        nvram_set("wps_proc_status", "0");

    nvram_set("wps_restart", "0");
#if defined(CONFIG_HOSTAPD) && defined(BCA_CPEROUTER)
     if (nvram_match("hapd_enable", "0"))
#endif
    bcmSystem("wps_monitor&");
}

#endif //end of SUPPORT_WSC

static int acs_getScanPeriod(int index)
{
    char nvname[NVNAME_SIZE]= {0};
    char *scanPeriod;
    int ret = 0;

    snprintf(nvname, NVNAME_SIZE, "wl%d_acs_cs_scan_timer", index);
    scanPeriod = nvram_unf_get(nvname);
    if (scanPeriod)
    {
        ret = atoi(scanPeriod);
        free(scanPeriod);
    }
    return ret;
}

static void start_Acsd(void)
{
    int timeout = 0;
    char ifname[IFNAME_LENGTH]= {0};
    char cmd[CMD_BUF_SIZE];
    char *nvAcsVersion;
    int acsVersion = 2;
    int i=0,enabled=0;

    // Default acs_version is 2
    nvAcsVersion = nvram_unf_get("acs_version");
    if (nvAcsVersion == NULL)
    {
        // assign default value into nvram
        nvram_set("acs_version", "2");
        acsVersion = 2;
    }
    else
    {
        /** Only nvram value is set to "1".
         * Switch the acsd version to 1.
         */ 
        if (strcmp(nvAcsVersion, "1") == 0)
            acsVersion = 1;

        free(nvAcsVersion);
    }

    for (i = 0 ; i < act_wl_cnt ; i++ )
    {
        snprintf(ifname, IFNAME_LENGTH, "wl%d", i);
        enabled = isAdapterEnabled(ifname) && isInterfaceEnabled(ifname);
        if (enabled)
            break;
    }

    if(!enabled)
        return;

    // if escand exists, run it rather than acsd or acsd2
    if (detectApp("escand"))
    {
        bcmSystem("escand");
        return;
    }

    // Check acsd2 first, if no escand
    if (detectApp("acsd2") && acsVersion == 2)
    {
        bcmSystem("acsd2");
        return;
    }
    
    if (detectApp("acsd"))
    {
        bcmSystem("acsd");
        for (i = 0 ; i < act_wl_cnt ; i++ )
        {
            timeout = acs_getScanPeriod(i) * 60; // minutes to seconds
            if(timeout)
            {
                snprintf(cmd, sizeof(cmd), "acs_cli -i wl%d acs_cs_scan_timer %d", i, timeout);
                bcmSystem(cmd);
            }
        }
    }
}

static void start_BSD(void)
{
    /* First to kill all bsd process if already start*/
    //bcmSystem("killall -q -15 bsd 2>/dev/null");
    /* workaround for impl51 bsd primary mode issue */
    bcmSystem("killall -q -9 bsd 2>/dev/null");
    bcmSystem("bsd&");
}

static void start_SSD(void)
{
    char nvname[NVNAME_SIZE];
    char *ssd_enable;

    /* First to kill all ssd process if already start*/
    bcmSystem("killall -q -15 ssd 2>/dev/null");

    sprintf(nvname, "ssd_enable");
    ssd_enable = nvram_unf_get(nvname);
    if (ssd_enable)
    {
        if(!strcmp(ssd_enable, "1"))
        {
            bcmSystem("ssd&");
        }
        free(ssd_enable);
    }
}

#ifdef  __CONFIG_RPCAPD__
static void start_rpcapd(void)
{
    int i=0,rpcapd_enabled=0;
    char nvname[NVNAME_SIZE] = {0};
    char cmd[CMD_BUF_SIZE] = {0};
    char *nv_value = NULL;
    char to_restart=0;

    for ( i = 0 ; i < act_wl_cnt ; i++)
    {
        sprintf(nvname, "wl%d_mode", i);
        nv_value = nvram_unf_get(nvname);
        if (nv_value && strcmp(nv_value, "monitor") == 0)
        {
            free(nv_value);
            rpcapd_enabled=1;
            sprintf(nvname, "dhd%d_rnr_rxoffl",i);
            nv_value = nvram_unf_get(nvname);
            if(!nv_value || !strncmp(nv_value,"1",1)) {
                /*need to use system shell to set nvram in order to
                 *available to kernel,need to adjust  */
                snprintf(cmd, CMD_BUF_SIZE, "nvram kset dhd%d_rnr_rxoffl=0",i);
                bcmSystem(cmd);
                // save into nvram
                nvram_set(nvname, "0");
                /*mark it changed and restore back if change back
                 *to other mode */
                snprintf(cmd, CMD_BUF_SIZE, "nvram kset dhd%d_rnr_rxoffl_changed=1",i);
                bcmSystem(cmd);
                sprintf(nvname, "dhd%d_rnr_rxoffl_changed",i);
                // save into nvram
                nvram_set(nvname, "1");
                to_restart=1;
            }

            if (nv_value != NULL)
                free(nv_value);
        }
        else
        {
            if (nv_value != NULL) 
                free(nv_value);

            sprintf(nvname, "dhd%d_rnr_rxoffl_changed",i);
            /*restore it back to rxoffl enable mode*/
            nv_value = nvram_unf_get(nvname);
            if (nv_value != NULL)
            {
                snprintf(cmd, CMD_BUF_SIZE, "nvram unset dhd%d_rnr_rxoffl",i);
                bcmSystem(cmd);
                snprintf(cmd, CMD_BUF_SIZE, "nvram unset dhd%d_rnr_changed",i);
                bcmSystem(cmd);
                to_restart=1;

                free(nv_value);
            }
        }
    }

    if(to_restart)
    {
        bcmSystem("nvram commit");
        fprintf(stderr, "Board reboots to enable runner offload...\n");
        bcmSystem("reboot");
    } 
    else if(rpcapd_enabled) 
    {
        bcmSystem("rpcapd -d -n > /dev/console &");
    }
}

#endif /* __CONFIG_RPCAPD__ */

#ifdef WL_AIR_IQ
static void startAirIQ(void) {
    int index=0;
    char temp[128];
    char *nv_value = NULL;

    if(detectApp("airiq_service") && (nv_value = nvram_unf_get("airiq_service_enable")))
    {
        if(!strcmp(nv_value,"1") || !strcmp(nv_value,"y"))
        {
            char *_nv_value = NULL;

            bcmSystem("airiq_service -c /usr/sbin/airiq_service.cfg -pfs /usr/sbin/flash_policy.xml &");
            sleep(2);
            for (index = 0; index < act_wl_cnt; ++index)
            {
                snprintf(temp, 128, "wl%d_airiq_enable", index);
                if((_nv_value = nvram_unf_get(temp)) != NULL)
                {
                    if(!strcmp(_nv_value,"1") || !strcmp(_nv_value,"y"))
                    {
                        snprintf(temp, 128, "airiq_app -i wl%d >/dev/null &", index);
                        bcmSystem(temp);
                    }
                    free(_nv_value);
                }
            }
        }
        free(nv_value);
    }
}

static void stopAirIQ(void) {
    char *nv_value = NULL;

    if(detectApp("airiq_service") && (nv_value = nvram_unf_get("airiq_service_enable")))
    {
        if(!strcmp(nv_value,"1") || !strcmp(nv_value,"y"))
        {
            bcmSystem("killall -q -9 airiq_service");
            bcmSystem("killall -q airiq_app");
        }
        free(nv_value);
    }
}
#endif

#ifdef __CONFIG_WBD__
static void start_wbd(void)
{
	bcmSystem("wbd_master");
	bcmSystem("wbd_slave");
}

static void stop_wbd(void)
{
    bcmSystem("killall wbd_master 2>/dev/null");
    bcmSystem("killall wbd_slave 2>/dev/null");
}
#endif /* __CONFIG_WBD__ */

#ifdef __CONFIG_BCM_APPEVENTD__ 
static void start_appeventd(void)
{
    char *enable = NULL;

    enable = nvram_unf_get("appeventd_enable");
    
    if (enable)
    {
        if (strncmp(enable, "1", 1) == 0)
            bcmSystem("appeventd&");

        free(enable);
    }
}

static void stop_appeventd(void)
{
    bcmSystem("killall -q -9 appeventd 2>/dev/null");
}
#endif /* __CONFIG_BCM_APPEVENTD__ */


#if defined(WL_BSTREAM_IQOS)
static int
start_broadstream_iqos(void)
{
    if (!nvram_match("broadstream_iqos_enable", "1"))
        return 0;
    bcmSystem("bcmiqosd start");

    return 0;
}

static int
stop_broadstream_iqos(void)
{
    bcmSystem("bcmiqosd stop");

    return 0;
}
#endif /* WL_BSTREAM_IQOS */

void wlssk_start_services(unsigned int services, int intfEnabled)
{
    int i;
    char cmd[CMD_BUF_SIZE];
    char nvname[NVNAME_SIZE];
    char *nv_value;

    bcmSystem("eapd");
    start_ceventd();

#if defined(CONFIG_HOSTAPD) && defined(BCA_CPEROUTER)
    nv_value = nvram_unf_get("hapd_enable");
    if (nv_value)
    {
        if (strcmp(nv_value, "0") == 0) // hapd is disabled
        {
            bcmSystem("nas");
            clientMode_BSS_downup(); // Make nas client perform authorize correctly
        }
        free(nv_value);
    }
    else // hapd_enable is not defined
    {
        nvram_set("hapd_enable", "1");
    }
#else
    bcmSystem("nas");
    clientMode_BSS_downup(); // Make nas  client perform authorize correctly
#endif
    start_debug_monitor(1);

#ifdef SUPPORT_WSC
    start_Wsc();
#endif

    start_BSD();
    start_SSD();

#if defined(HSPOT_SUPPORT)
    bcmSystem("hspotap&");
#endif
#ifdef __CONFIG_TOAD__
    bcmSystem("toad");
#endif

    // ACSD
    start_Acsd();

#ifdef  __CONFIG_RPCAPD__
    start_rpcapd();
#endif

#ifdef __CONFIG_VISUALIZATION__
    bcmSystem("vis-dcon");
    bcmSystem("vis-datacollector");
#endif

#ifdef __CONFIG_BCM_APPEVENTD__
    start_appeventd();
#endif

#ifdef WL_AIR_IQ
    startAirIQ();
#endif

#ifdef __CONFIG_EXTACS__
    for ( i = 0 ; i < act_wl_cnt ; i++)
    {
        snprintf(nvname, sizeof(nvname), "wl%d_acs_cs_scan_timer", i);
        nv_value = nvram_unf_get(nvname);
        if (nv_value != NULL)
        {
            snprintf(cmd, sizeof(cmd), "acs_cli -i wl%d acs_cs_scan_timer %s", i, nv_value);
            free(nv_value);
        }
        else
            snprintf(cmd, sizeof(cmd), "acs_cli -i wl%d acs_cs_scan_timer %d", i, 600);

        bcmSystem(cmd);
    }
#endif

#ifdef CONFIG_HOSTAPD
    {
        char *hapd = NULL;
        hapd = nvram_unf_get("hapd_enable"); 
        if (hapd != NULL)
        {
            if (strcmp(hapd, "0") != 0)
            {
                start_hapd_wpasupp();
#if defined(SUPPORT_WSC) && defined(BCA_CPEROUTER)
		bcmSystem("wps_pbcd");
#endif
            }
            free(hapd);
        }
    }
#endif  /* CONFIG_HOSTAPD */

#ifdef __CONFIG_WBD__
   start_wbd(); 
#endif

#if defined(WL_BSTREAM_IQOS)
    start_broadstream_iqos();
#endif
}


void wlssk_stop_services(unsigned int services)
{
    stop_debug_monitor();
#ifdef SUPPORT_WSC
    bcmSystem("killall -q -9 wps_ap 2>/dev/null");
    bcmSystem("killall -q -9 wps_enr 2>/dev/null");
    bcmSystem("killall -q wps_monitor 2>/dev/null");
    clearSesLed();
#endif
#ifdef CONFIG_HOSTAPD
    bcmSystem("killall wps_pbcd");
    stop_hapd_wpasupp();
#endif  /* CONFIG_HOSTAPD */
    bcmSystem("killall -q -9 nas 2>/dev/null");
    bcmSystem("killall -q -9 acsd 2>/dev/null");
    bcmSystem("killall -q -9 acsd2 2>/dev/null");

#ifdef BCMWAPI_WAI
    bcmSystem("killall -q -15 wapid");
#endif

#if defined(HSPOT_SUPPORT)
    bcmSystem("killall -q -15 hspotap");
#endif

    /* workaround for impl51 bsd primary mode issue */
    bcmSystem("killall -q -9 bsd");
    //bcmSystem("killall -q -15 bsd");
    bcmSystem("killall -q -15 ssd");

#ifdef __CONFIG_TOAD__
    bcmSystem("killall -q -15 toad");
#endif

#ifdef __CONFIG_VISUALIZATION__
    bcmSystem("killall -q -9 vis-datacollector");
    bcmSystem("killall -q -9 vis-dcon");
#endif

#ifdef __CONFIG_WBD__
    stop_wbd();
#endif

#ifdef __CONFIG_BCM_APPEVENTD__
    stop_appeventd();
#endif

#ifdef  __CONFIG_RPCAPD__
    bcmSystem("killall -q -9 rpcapd 2>/dev/null");
#endif /* __CONFIG_RPCAPD__ */

    bcmSystem("killall -q -9 dhd_monitor 2>/dev/null");
#ifdef WL_AIR_IQ
    stopAirIQ();
#endif

    bcmSystem("killall ceventd  2>/dev/null");
    bcmSystem("killall eapd 2>/dev/null");

#if defined(WL_BSTREAM_IQOS)
    stop_broadstream_iqos();
#endif
}
