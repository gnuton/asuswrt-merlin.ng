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
#include "wlssk.h"
#include "event.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <signal.h>
#include <ctype.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <linux/if_packet.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <bcmnvram.h>
#include "wlsyscall.h"
#include <wlif_utils.h>
#include <security_ipc.h>

#include <typedefs.h>
#include <wlioctl.h>

#include "debug.h"


static void wlevent_handler();
static int sock = -1;

int wlevent_init()
{

    int reuse = 1;
    int err = 0;

    struct sockaddr_in sockaddr;

    /* open loopback socket to communicate with EAPD */
    memset(&sockaddr, 0, sizeof(sockaddr));
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    sockaddr.sin_port = htons(EAPD_WKSP_MEVENT_UDP_SPORT);

    if (( sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        ERROR("%s@%d Unable to create socket\n", __FUNCTION__, __LINE__ );
        err = -1;
    } else if ( (err = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char*)&reuse, sizeof(reuse))) < 0) {
        ERROR("%s@%d: Unable to setsockopt to loopback socket %d.\n", __FUNCTION__, __LINE__, sock);
    } else if ( (err = bind(sock, (struct sockaddr *)&sockaddr, sizeof(sockaddr))) < 0) {
        ERROR("%s@%d Unable to bind to loopback socket %d\n", __FUNCTION__, __LINE__, sock);
    }

    if ( err < 0  && sock >= 0 )
    {
        ERROR("%s@%d: failure. Close socket\n", __FUNCTION__, __LINE__ );
        close(sock);
        return err;
    }
    else 
    {
        DEBUG("%s@%d: opened loopback socket %d\n", __FUNCTION__, __LINE__, sock);
        wlssk_register_listener(sock, wlevent_handler, NULL); 
    }

    return err;
}

void wlevent_cleanup()
{
    if (sock > 0)
    {
        wlssk_deregister_listener(sock);
        close(sock);
    }
}
#if defined(BUILD_BRCM_CMS) && !defined(BUILD_HND_EAP)
extern void cms_update_associated_device(STAEVT_INFO);
#endif

char const _hex_chars[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
static char * BINMAC_STR_CONVERT(char *data) {
    int i=0,pos=0;
    static char strMAC[MACSTR_LEN];
    memset(strMAC, 0, MACSTR_LEN);
    for(i = 0; i < 6; i++ )
    {
        char const byte = data[i];
        strMAC[pos++]= _hex_chars[ ( byte & 0xF0 ) >> 4 ];
        strMAC[pos++]= _hex_chars[ ( byte & 0x0F ) >> 0 ];
        if(i<5) strMAC[pos++]=':';
        else strMAC[pos++]='\0';
    }
    return strMAC;
}

void wlevent_handler(void* data __attribute__((unused)))
{
    static char pkt[SOCK_RCV_BUF_LEN] = {0};
    struct sockaddr_in from;
    int sock_len = sizeof(struct sockaddr_in);
    bcm_event_t *dpkt;
    int sta_type = -1 , event_type;

    if (recvfrom(sock, pkt, SOCK_RCV_BUF_LEN, 0, (struct sockaddr *)&from, (socklen_t *)&sock_len)>0) {
        dpkt = (bcm_event_t *)(pkt+IFNAME_LEN);
        event_type = ntohl(dpkt->event.event_type);
        DEBUG("event_type:%lu\n", event_type);

        /* when STA leaves without notice bys sending disassociate, dongle will detec tand send WLC_E_DEAUTH */
        if (event_type == WLC_E_AUTH_IND || event_type == WLC_E_ASSOC_IND || event_type == WLC_E_REASSOC_IND)
            sta_type = STA_CONNECTED;
        else if( event_type == WLC_E_DEAUTH_IND || event_type == WLC_E_DEAUTH || event_type == WLC_E_DISASSOC ||
                event_type == WLC_E_DISASSOC_IND)
            sta_type=STA_DISCONNECTED;

        if(sta_type>=0) 
        {
            STAEVT_INFO event = {0};
            event.type = sta_type;
            memcpy(event.mac, BINMAC_STR_CONVERT((char *)(&(dpkt->event.addr))),MACSTR_LEN);
            strncpy(event.ifname, dpkt->event.ifname, IFNAME_LEN-1);

            DEBUG("STA_TYPE[%d][%s]MAC_ADDR:%s\n", event.type, event.ifname, event.mac);

/* 
 * For EAP build, we won't send assoc/disassoc notifications to ssk for updating AssociatedDevice MDM
 * in order to avoid psmx watchdog triggering when running the associate/disassociate test 
 * with max number of clients per radio, e.g., 128 clients.
 */
#if defined(BUILD_BRCM_CMS) && !defined(BUILD_HND_EAP)
            // send cms message
            cms_update_associated_device(event);
#endif
#ifdef IDLE_PWRSAVE
            bcmTogglePowerSave();
#endif
        }

    }
}

int wlevent_send_dif_event(char *ifname, unsigned int event)
{
   static int s = -1;
   int len, n;
   struct sockaddr_in to;
   char data[IFNAME_LEN + sizeof(uint32)];

   /* create a socket to receive dynamic i/f events */
   if (s < 0) {
      s = socket(AF_INET, SOCK_DGRAM, 0);
      if (s < 0) {
         perror("socket");
         return -1;
      }
   }

   /* Init the message contents to send to eapd. Specify the interface
    * and the event that occured on the interface.
    */
   strncpy(data, ifname, IFNAME_LEN);
   *(uint32 *)(data + IFNAME_LEN) = event;
   len = IFNAME_LEN + sizeof(uint32);

   /* send to eapd */
   to.sin_addr.s_addr = inet_addr(EAPD_WKSP_UDP_ADDR);
   to.sin_family = AF_INET;
   to.sin_port = htons(EAPD_WKSP_DIF_UDP_PORT);

   n = sendto(s, data, len, 0, (struct sockaddr *)&to,
         sizeof(struct sockaddr_in));

   if (n != len) {
      perror("udp send failed\n");
      return -1;
   }

   DEBUG("hotplug_net(): sent event %d\n", event);

   return n;
}
