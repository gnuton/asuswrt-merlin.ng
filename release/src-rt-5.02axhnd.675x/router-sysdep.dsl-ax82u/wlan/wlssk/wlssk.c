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
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "wlssk.h"
#include "service.h"
#include "debug.h"
#include "bcmnvram.h"
#include "wlsyscall.h"
#include "event.h"

#include "wlsysutil.h"
#include "nvram_api.h"
#include <wlif_utils.h>
#include <dpsta_linux.h>


extern int wl_wlif_wds_ap_ifname(char *inteface,char* name);
extern bool wl_wlif_is_psta(char *ifname);

#ifdef WL_BSTREAM_IQOS
static void broadstream_iqos_init(void);
#endif

extern int act_wl_cnt;
unsigned long gpioOverlays;

// for receiving external event
typedef struct Listener {
    int fd;
    EventListenerFunc func;
    void *data;
    struct Listener *next;
}Listener;

// for internal timeout event
typedef struct Timer {
    TimerFunc func;
    void *data;
    struct Timer *next; 
}Timer;

static Listener *listeners = NULL;
static Timer    *timers    = NULL;

void wlssk_init(void)
{
    int i;
    char nvname[NVNAME_SIZE];
    char *restore = NULL;

    if ((restore = nvram_unf_get("_default_restored_")) == NULL)
    {
        for (i = 0 ; i < act_wl_cnt ; i++)
        {
            /* set bw_cap default value */
            snprintf(nvname, NVNAME_SIZE, "wl%d_%s", i, "bw");
            nvram_set(nvname, "1");
            snprintf(nvname, NVNAME_SIZE, "wl%d_%s", i, "bw_cap");
            nvram_set(nvname, "1");
        }
    }
    else
    {
        free(restore);
    }

    /* For bcm47622, we don't want the cpu pinning in eid.c, call wlaffinity here again to ensure the setting is accurate. */
    bcmSystem("wlaffinity auto");

    gpioOverlays = bcmGetGPIOOverlays();

#ifdef __CONFIG_VISUALIZATION__
    nvram_set("wlVisualization", "1");
#else
    nvram_unset("wlVisualization");
#endif
#if defined(__CONFIG_HSPOT__)
    nvram_set("wlPassPoint", "1");
#endif
    nvram_set("hide_hnd_header", "y");
#ifdef WL_BSTREAM_IQOS
    nvram_set("wlBIQOS", "1");
    broadstream_iqos_init();
#else
    nvram_unset("wlBIQOS");
#endif

    setup_mbss_Mac_addr();

    crash_log_backup_init();

    wlssk_restart(RESTART_ALL);
}

void wlssk_cleanup(void)
{
    wlssk_stop_services(0);
}


static void insert_into_ifnames(char *ifname, unsigned int index)
{ 
    char *current_ifnames;
    char nvname[NVNAME_SIZE];
    char ifnames[MAX_BUF_SIZE] = {0};

    if (index == 0)
        snprintf(nvname, NVNAME_SIZE, "lan_ifnames");
    else
        snprintf(nvname, NVNAME_SIZE, "lan%d_ifnames", index);

    current_ifnames = nvram_unf_get(nvname);
    if (current_ifnames)
    {
        snprintf(ifnames, MAX_BUF_SIZE, "%s %s ", current_ifnames, ifname);
        free(current_ifnames);
    }
    else
        snprintf(ifnames, MAX_BUF_SIZE, "%s ", ifname);

    nvram_set(nvname, ifnames);
}

static void intf_up(const char* ifname)
{
    char cmd[CMD_BUF_SIZE];
    char nvname[NVNAME_SIZE]; 
    char *radio = NULL;
    
    if (!ifname)
        return;

    /* give wlx.y_radio=1 for JIRA-32693 tr69 configure wifi issue 
     * EAPD and WBD would care about this nvram */
    snprintf(nvname, NVNAME_SIZE, "%s_radio", ifname);
    if ((radio = nvram_unf_get(nvname)) == NULL)
        nvram_set(nvname, "1");
    else
        free(radio);

    snprintf(cmd, sizeof(cmd), "ifconfig %s up 2>/dev/null", ifname);
    bcmSystem(cmd);
}

static void intf_down(const char* ifname)
{
    char cmd[CMD_BUF_SIZE];
    char nvname[NVNAME_SIZE];
    char *brName;

    if (!ifname)
        return;

    snprintf(cmd, sizeof(cmd), "ifconfig %s down 2>/dev/null", ifname);
    bcmSystem(cmd);

    if ((brName = nvram_unf_get("lan_ifname")) != NULL)
    {
        snprintf(cmd, sizeof(cmd), "brctl delif %s %s 2>/dev/null", brName, ifname);
        bcmSystem(cmd);
        free(brName);
    }
    if ((brName = nvram_unf_get("lan1_ifname")) != NULL)
    {
        snprintf(cmd, sizeof(cmd), "brctl delif %s %s 2>/dev/null", brName, ifname);
        bcmSystem(cmd);
        free(brName);
    }

    snprintf(nvname, sizeof(nvname), "%s_ifname", ifname);

    if (strstr(ifname, ".")) //for virtual interfaces
    {
        /* if wlx.y is enabled, set wlx.y_ifname */
        if (isInterfaceEnabled(ifname))
        {
            nvram_set(nvname, ifname);
        }
        else /* if wlx.y is not enabled, unset wlx.y_ifname */
        {
            nvram_unset(nvname);
        }
    }
    else // always set for primary interface
    {
        nvram_set(nvname, ifname);
    }
}

static int adapter_up(const unsigned int idx)
{
    char ifname[IFNAME_LENGTH] = {0};
    int NumOfVifs = wlgetVirtIntfNo(idx);
    int i, ret = FALSE;

    snprintf(ifname, sizeof(ifname), "wl%d", idx);
    if (!isAdapterEnabled(ifname)) // check radio is enabled
        return ret;

    for (i = 1 ; i < NumOfVifs ; i++)
    {
        snprintf(ifname, sizeof(ifname), "wl%d.%d", idx, i);
        if (isInterfaceEnabled(ifname))
        {
            ret = TRUE;
            intf_up(ifname);
        }
    }

    snprintf(ifname, sizeof(ifname), "wl%d", idx);
    if (ret || isInterfaceEnabled(ifname)) // check ssid is enabled
    {
        ret = TRUE;
        intf_up(ifname);
    }


#if 0  //WDS
    for_each(mac,WL_RADIO_WLWDS(idx),next) {
        snprintf(cmd, sizeof(cmd), "ifconfig wds%d.%d up 2>/dev/null", idx, j);
        bcmSystem(cmd);
        j++;
    }
#endif
    return ret;
}

static void adapter_down(const unsigned int idx)
{
    char ifname[IFNAME_LENGTH] = {0};
    char cmd[CMD_BUF_SIZE] = {0};
    int NumOfVifs = wlgetVirtIntfNo(idx);
    int i;
    
    /* reset dongle */
    snprintf(cmd, CMD_BUF_SIZE, "wl -i wl%d reboot 2>/dev/null", idx);
    bcmSystem(cmd);

    for ( i = 0 ; i < NumOfVifs ; i++)
    {
        if (i)
            snprintf(ifname, sizeof(ifname), "wl%d.%d", idx, i);
        else
            snprintf(ifname, sizeof(ifname), "wl%d", idx);

        intf_down(ifname);
    }
}

static void adapter_setup(const unsigned int idx)
{
    char cmd[CMD_BUF_SIZE] = {0};
    char ifname[IFNAME_LENGTH] = {0};
    char nvname[NVNAME_SIZE] = {0}; 
    char vifs[CMD_BUF_SIZE] = {0};
    char *p = vifs;
    int  i, NumOfVifs = wlgetVirtIntfNo(idx);
    char *mac = NULL;

    // unset wl(i)_vifs
    snprintf(nvname, sizeof(nvname), "wl%d_vifs", idx);
    nvram_unset(nvname);

    for (i = 0 ; i < NumOfVifs ; i++)
    {
        if (i)
            snprintf(ifname, IFNAME_LENGTH, "wl%d.%d", idx, i);
        else
            snprintf(ifname, IFNAME_LENGTH, "wl%d", idx);

        sprintf(nvname, "%s_hwaddr", ifname);
        mac = nvram_unf_get(nvname);
        if (mac)
        {
            snprintf(cmd, sizeof(cmd), "ifconfig %s hw ether %s 2>/dev/null", ifname, mac);
            free(mac);
        }

        bcmSystem(cmd);
        mac = NULL;
        
        // check vitual interface
        if (i && isInterfaceEnabled(ifname)) 
        {
            if (p == vifs)  // first added item
                p += sprintf(p, "%s", ifname);
            else
                p += sprintf(p, " %s", ifname);
        }
    }

    // set wl(idx)_vifs
    snprintf(nvname, sizeof(nvname), "wl%d_vifs", idx);
    nvram_set(nvname, vifs);
}

static void wlconf_setup(const unsigned int idx)
{
    char cmd[CMD_BUF_SIZE];

    snprintf(cmd, sizeof(cmd), "wlconf wl%d down", idx);
    DEBUG("[WLCONF]%s\n", cmd);
    bcmSystem(cmd);
    
    snprintf(cmd, sizeof(cmd), "wlconf wl%d up", idx);
    DEBUG("[WLCONF]%s\n", cmd);
    bcmSystem(cmd);

#if 0 //def SUPPORT_MIMO
    if (MIMO_PHY) {

        /* Set Rxchain power save sta assoc check default to 1 */
        snprintf(cmd, sizeof(cmd), "wl -i wl%d rxchain_pwrsave_stas_assoc_check %d", idx, ON);
        BCMWL_WLCTL_CMD(cmd);

        /* Set Radio power save sta assoc check default to 1 */
        snprintf(cmd, sizeof(cmd), "wl -i wl%d radio_pwrsave_stas_assoc_check %d", idx, ON);
        BCMWL_WLCTL_CMD(cmd);
    }
#endif
}

static void wlconf_start(const unsigned int idx)
{
    char cmd[CMD_BUF_SIZE];
    snprintf(cmd, sizeof(cmd), "wlconf wl%d start", idx);
    DEBUG("[WLCONF]%s\n", cmd);
    bcmSystem(cmd);
}

#define DPSTA_IFNAME "dpsta"
static void bridge_setup()
{
    char ifname[IFNAME_LENGTH] = {0};
    char *temp = NULL, *dpsta_ifnames = NULL, *lan_hwaddr = NULL;
    char cmd[CMD_BUF_SIZE] = {0};
    int idx, subIdx;
	dpsta_enable_info_t info = { 0 };

	if ((dpsta_ifnames = nvram_unf_get("dpsta_ifnames")) != NULL)
    {
		char *policy = NULL, *lan_uif = NULL;

        DEBUG("%s: DPSTA mode is enabled ... bring up dpsta interface\n", __FUNCTION__);
		if ((lan_hwaddr = nvram_unf_get("lan_hwaddr")) != NULL)
    	{
			snprintf(cmd, sizeof(cmd), "ifconfig %s hw ether %s 2> /dev/null", DPSTA_IFNAME, lan_hwaddr);
			bcmSystem(cmd);
			free(lan_hwaddr);
		}
		snprintf(cmd, sizeof(cmd), "ifconfig %s up 2> /dev/null", DPSTA_IFNAME);
		bcmSystem(cmd);

		policy = nvram_unf_get("dpsta_policy");
		lan_uif = nvram_unf_get("dpsta_lan_uif");
		info.enable = 1;
		info.policy = policy ? atoi(policy) : 1;
		info.lan_uif = lan_uif ? atoi(lan_uif) : 1;
		if (policy)
			free(policy);
		if (lan_uif)
			free(lan_uif);
	}

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
                char *bridgeName = NULL;

                get_bridge_by_ifname(ifname, &bridgeName);
                if (!bridgeName)  // assign to default bridge
                {
                    bridgeName = "br0";
                    insert_into_ifnames(ifname, 0);
                }
				if (dpsta_ifnames && strstr(dpsta_ifnames, ifname)) {
					snprintf(cmd, sizeof(cmd), "brctl delif %s %s 2> /dev/null", bridgeName, ifname);
					DEBUG("%s: %s (idx %d) was found dpsta_ifnames,"
						"removing it from %s\n", __FUNCTION__, ifname, idx, bridgeName);
                	bcmSystem(cmd);
					strcpy((char *)info.upstream_if[idx], ifname);	
					dpsta_ioctl("dpsta", &info, sizeof(dpsta_enable_info_t));
					snprintf(ifname, sizeof(ifname), "%s", DPSTA_IFNAME);
				}
                snprintf(cmd, sizeof(cmd), "brctl addif %s %s 2> /dev/null", bridgeName, ifname);
                bcmSystem(cmd);
            }
        }
    }

	if (dpsta_ifnames)
		free(dpsta_ifnames);

    if ((temp = nvram_unf_get("lan_ifname")) == NULL)
    {
        /* default to br0 */
        nvram_set("lan_ifname", "br0");
    }
    else
        free(temp);

    // set br_ifnames and lan_hwaddr
    for (idx = 0 ; idx < MAX_BR_NUM ; idx++)
    {
        char lanifname[NVNAME_SIZE] = {0};
        char lanifnames[NVNAME_SIZE] = {0};
        char lanhwaddr[NVNAME_SIZE] = {0};
        char brifnames[NVNAME_SIZE] = {0};
        char hwaddr[NVNAME_SIZE]= {0};
        char *brName = NULL;
        char *ifnames = NULL;

        if (idx == 0)
        {
            snprintf(lanifname, NVNAME_SIZE, "lan_ifname");
            snprintf(lanifnames, NVNAME_SIZE, "lan_ifnames");
            snprintf(lanhwaddr, NVNAME_SIZE, "lan_hwaddr");
        }
        else
        {
            snprintf(lanifname, NVNAME_SIZE, "lan%d_ifname", idx);
            snprintf(lanifnames, NVNAME_SIZE, "lan%d_ifnames", idx);
            snprintf(lanhwaddr, NVNAME_SIZE, "lan%d_hwaddr", idx);
        }

        if ((brName = nvram_unf_get(lanifname)) != NULL)
        {
            if ((ifnames = nvram_unf_get(lanifnames)) != NULL)
            {
                snprintf(brifnames, NVNAME_SIZE, "%s_ifnames", brName);
                nvram_set(brifnames, ifnames);
                free(ifnames);
            }

            if (getHwAddress(brName, hwaddr) == 0)
                nvram_set(lanhwaddr, hwaddr);

            free(brName);
        }
        else
            break;
    }
}

void wlssk_hotplug_handler(char *ifname, char *action)
{
    char cmd[CMD_BUF_SIZE] = {0};
    char wdsap_ifname[IFNAME_LENGTH]= {0};
    char *br_ifname = NULL;
    bool psta_if, monitor_if, wds_if, dyn_if, add_event, remove_event;
    psta_if = wl_wlif_is_psta(ifname);
    monitor_if = !strncmp(ifname, "radiotap", 3);
    wds_if = !strncmp(ifname, "wds", 3);
    dyn_if = wds_if || psta_if || monitor_if;
    add_event = !strcasecmp(action, "add");
    remove_event = !strcasecmp(action, "remove");

    if (!dyn_if)
       return;

    /** Notice.
     *  since get_bridge_by_ifname will calling nvram_get,
     *  we should not use nvram_unf_get here
     */ 
    if(wds_if) {
        if (!wl_wlif_wds_ap_ifname(ifname, wdsap_ifname)) {
            get_bridge_by_ifname(wdsap_ifname, &br_ifname);
        } else {
            /* Since wl_wlif_wds_ap_ifname() hasn't returned the 
             * parent interface this might be vlan interface crated on a
             * dwds interface, don't add it to default bridge br0.
             */
            return;
        }
    }

    if(!br_ifname && !(br_ifname=nvram_get("lan_ifname")))
        br_ifname="br0";
    
    // it should never be true
    if (br_ifname == NULL)  
        return;

    if (add_event)
    {
       snprintf(cmd, CMD_BUF_SIZE, "ifconfig %s up", ifname);
       bcmSystem(cmd);
       if (wds_if)
       {
          snprintf(cmd, CMD_BUF_SIZE, "brctl addif %s %s",br_ifname, ifname);
          bcmSystem(cmd);
       }
       if (psta_if)
       {
          wlevent_send_dif_event(ifname, 0);
       }
    }

    if (remove_event)
    {
       snprintf(cmd, CMD_BUF_SIZE, "brctl delif %s %s", br_ifname, ifname);
       bcmSystem(cmd);
       snprintf(cmd, CMD_BUF_SIZE, "ifconfig %s down", ifname);
       bcmSystem(cmd);
       if (psta_if)
       {
          wlevent_send_dif_event(ifname, 1);
       }
    }
}

void wlssk_restart(unsigned long targets)
{
    int i;
    int anyIntfEnabled = FALSE;
    nvram_set("_wlrestart_", "1");
    wlssk_stop_services(0);

    // pre-setup
    for (i = 0 ; i < act_wl_cnt ; i++ )
    {
        adapter_down(i);
        wlconf_setup(i);
        adapter_setup(i);
        wlconf_start(i);
        anyIntfEnabled |= adapter_up(i);
#ifdef IDLE_PWRSAVE
        /* enabled ASPM */
        setASPM(i, 1);
#endif
    }

    bridge_setup();

    wlssk_start_services(0, anyIntfEnabled);

    nvram_unset("map_reonboard");
    nvram_unset("_wlrestart_");
    nvram_set("_default_restored_", "1");
    nvram_commit();
    fprintf(stdout, "--WLSSK RESTART DONE--\n");
}

int wlssk_register_listener(int fd, EventListenerFunc func, void *data)
{
    Listener *listener = listeners;

    // Check if this file descriptor is already registered
    while (listener)
    {
        if (listener->fd == fd)
        {
            if ((func != listener->func) || (data != listener->data))
            {
                ERROR("register fd:%d conflict to old one\n");
                return -1;
            }
            return 0;
        }
        listener = listener->next;
    }

    listener = (Listener *)malloc(sizeof(Listener));

    if (!listener)
        return -1;

    listener->fd = fd;
    listener->func = func;
    listener->data = data;
    listener->next = listeners;

    listeners = listener;
    return 0;
} 

void wlssk_deregister_listener(int fd)
{
    Listener *previous = NULL;
    Listener *tmp = NULL;

    // no registered listener
    if (listeners) 
        return;
    
    // check the head of list, if match remove it.
    if (listeners->fd == fd)
    {
        tmp = listeners;
        listeners = listeners->next;

        free(tmp);
        return;
    }

    previous = listeners;
    tmp = listeners->next;

    // find matched listener and remove it.
    while (tmp)
    {
        if (tmp->fd == fd)
        {
            previous->next = tmp->next;

            free(tmp);
            return; 
        }

        previous = tmp;
        tmp = tmp->next;
    }
}

int wlssk_register_timer(TimerFunc func, void *data)
{
    Timer *timer = timers;

    // Check if this TimerFunc is already registered
    while (timer)
    {
        if (timer->func == func)
        {
            return 0;
        }
        timer = timer->next;
    }

    timer = (Timer *)malloc(sizeof(Timer));

    if (!timer)
        return -1;

    timer->func = func;
    timer->data = data;
    timer->next = timers;

    timers = timer;
    return 0;
}

unsigned long wlssk_run_once(unsigned long timeout)
{
    int res, maxFd = 0;
    unsigned long nextTimeout = WLSSK_DELAY_TIMEOUT;
    Listener *l = listeners;
    Timer    *t = timers;
    struct timeval selectTime;
    fd_set fds;

    FD_ZERO(&fds);

    // collect fd from listeners list.
    for (l=listeners ; l ; l = l->next)
    {
        FD_SET(l->fd, &fds);

        if (maxFd <= l->fd)
            maxFd = l->fd + 1;
    }

    selectTime.tv_sec = timeout / 1000;
    selectTime.tv_usec = (timeout % 1000) * 1000;

    // select fds with timerout for external events
    res = select(maxFd, &fds, NULL, NULL, &selectTime);    

    if (res < 0 && errno != EINTR)
    {
        ERROR("error in select (%d=%s)", errno, strerror(errno));
        return nextTimeout;
    }
    else if (res < 0 && errno == EINTR)
    {
        return nextTimeout;
    }
    
    for (l = listeners ; l ; l = l->next)
    {
        if (FD_ISSET(l->fd, &fds))
        {
            /* clear fd in the fdset */
            FD_CLR(l->fd, &fds);

            l->func(l->data);
        }
    }

    // check each timer and calculate the next timeout
    for (t = timers ; t ; t = t->next)
    {
        long n = t->func(t->data);
        nextTimeout = n < nextTimeout ? n : nextTimeout;
    }

    return nextTimeout;
}

#ifdef WL_BSTREAM_IQOS
static void broadstream_iqos_init(void)
{
    struct stat file_stat;

    if (stat("/usr/sbin/qos.conf", &file_stat) == 0) {
        if (mkdir("/tmp/trend", 0777) < 0 && errno != EEXIST)
            perror("/tmp/trend not created");
        else {
            bcmSystem("cp /usr/sbin/tdts_rule_agent /tmp/trend");
            bcmSystem("cp /usr/sbin/setup.sh /tmp/trend");
            bcmSystem("cp /usr/sbin/rule.trf /tmp/trend");
            bcmSystem("cp /usr/sbin/sample.bin /tmp/trend");
            bcmSystem("cp /usr/sbin/moni.sh /tmp/trend");
            bcmSystem("cp /usr/sbin/iqos-setup.sh /tmp/trend");
            bcmSystem("cp /usr/sbin/clean-cache.s /tmp/trend");
            bcmSystem("cp /usr/sbin/shn_ctrl /tmp/trend");
            bcmSystem("cp /usr/sbin/tcd /tmp/trend");
        }
    }
}
#endif  /* WL_BSTREAM_IQOS */
