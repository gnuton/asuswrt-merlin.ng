/***********************************************************************
 *
 *  Copyright (c) 2006-2018  Broadcom Ltd.
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

#ifdef DMP_DEVICE2_BASELINE_1

#include "cms_core.h"
#include "cms_util.h"
#include "rcl.h"
#include "rut2_ethlag.h"
#include "rut_util.h"
#include "cms.h"
#include "cms_mdm.h"
#include "cms_core.h"
#include "cms_util.h"
#include "stl.h"
#include "rut_util.h"
#include "rut_qos.h"
#include "rut2_bridging.h"
#include "cms_qdm.h"

/*!\file rut2_ethlag.c
 * \brief This file contains Ethernet LAG objects utility functions.
 *
 */
#ifdef DMP_DEVICE2_ETHLAG_1

void convertXmitHashPolicyString(const char *inWithPlus, char *outStr, SINT32 outStrLen)
{
   
   if (!cmsUtl_strcmp(inWithPlus, "layer2"))
   {
      cmsUtl_strncpy(outStr, inWithPlus, outStrLen);
   }
   else if (!cmsUtl_strcmp(inWithPlus, "layer2Plus3"))
   {
      cmsUtl_strncpy(outStr, "layer2+3", outStrLen);
   }
   else if (!cmsUtl_strcmp(inWithPlus, "layer3Plus4"))
   {
      cmsUtl_strncpy(outStr, "layer3+4", outStrLen);
   }
   else if (!cmsUtl_strcmp(inWithPlus, "encap2Plus3"))
   {
      cmsUtl_strncpy(outStr, "encap2+3", outStrLen);
   }
   else if (!cmsUtl_strcmp(inWithPlus, "encap3Plus4"))
   {
      cmsUtl_strncpy(outStr, "encap3+4", outStrLen);
   }   
   else
   {
      cmsUtl_strncpy(outStr, "layer2", outStrLen);  /* if no matching, just default */
   }
   
}

void rutEthLag_configEthIntfOnBridge(const char *brName, const char *ethIfName, UBOOL8 add)
{
   char cmd[BUFLEN_256] = {0};
   char ifNameBuf[CMS_IFNAME_LENGTH]={0};
   
   cmsUtl_strncpy(ifNameBuf, ethIfName, sizeof(ifNameBuf)-1);

   if (qdmIntf_isLayer2IntfNameUpstreamLocked(ifNameBuf))
   {
      cmsLog_debug(" WAN eth.No brctl addif/delif action, return...");
      return;
   }

#ifdef SUPPORT_LANVLAN
   snprintf(ifNameBuf, sizeof(ifNameBuf), "%s.0", ethIfName);
#endif 

   if (add)
   {  
      snprintf(cmd, sizeof(cmd)-1, "brctl addif %s %s", brName, ifNameBuf);
      rut_doSystemAction(__FUNCTION__, cmd); 
      snprintf(cmd, sizeof(cmd)-1, "ifconfig %s up", ifNameBuf);
      rut_doSystemAction(__FUNCTION__, cmd);       
   }
   else
   {
      snprintf(cmd, sizeof(cmd)-1, "ifconfig %s down", ifNameBuf);
      rut_doSystemAction(__FUNCTION__, cmd); 
      snprintf(cmd, sizeof(cmd)-1, "brctl delif %s %s", brName, ifNameBuf);
      rut_doSystemAction(__FUNCTION__, cmd); 
   }
   
}


void rutEthLag_configEthIntfOnLagIntf(const char *ethLagName, const char *ethIfName, UBOOL8 add)
{
   char cmd[BUFLEN_256] = {0};
   char ifNameBuf[CMS_IFNAME_LENGTH]={0};

   cmsUtl_strncpy(ifNameBuf, ethIfName, sizeof(ifNameBuf)-1);
   if (qdmIntf_isLayer2IntfNameUpstreamLocked(ifNameBuf))
   {
      cmsLog_debug("WAN eth. no ethx.0 stuff for %s", ifNameBuf);
   }   
   else
   {
#ifdef SUPPORT_LANVLAN
     /* for LAN eth port only */
      snprintf(ifNameBuf, sizeof(ifNameBuf), "%s.0", ethIfName);
#endif  
   }      

   /* always bring down eth port before add/del to EthLag */
   snprintf(cmd, sizeof(cmd)-1, "ifconfig %s down", ifNameBuf);
   rut_doSystemAction(__FUNCTION__, cmd); 

   if (add)
   {
      if (qdmIntf_isLayer2IntfNameUpstreamLocked(ethIfName))
      {
         cmsLog_debug("WAN ethr %s -not in the bridge so skip", ethIfName);
      }  
      else
      {
         /* remove eth interface from bridge port */
         rutBridge_deleteIntfNameFromBridge_dev2(ifNameBuf);
      }
      
      /* add this eth interface to the ethlag */
      snprintf(cmd, sizeof(cmd)-1, "echo +%s > sys/class/net/%s/bonding/slaves",  ifNameBuf, ethLagName);
      rut_doSystemAction(__FUNCTION__, cmd);  
   }
   else
   {
      snprintf(cmd, sizeof(cmd)-1, "echo -%s > sys/class/net/%s/bonding/slaves",  ifNameBuf, ethLagName);
      rut_doSystemAction(__FUNCTION__, cmd); 
   }   

   /* Bring up eth port after the add/del to/from ethLag operation */
   snprintf(cmd, sizeof(cmd)-1, "ifconfig %s up", ifNameBuf);
   rut_doSystemAction(__FUNCTION__, cmd);  
   
}


void rutEthLag_configInterface(const _Dev2EthLAGObject *obj,  UBOOL8 add)
{
   char cmd[BUFLEN_256] = {0};
   char xmitStr[BUFLEN_32]={0};

   if (add)
   {
      /* disable the ethLag interface first */
      rutEthLag_disableInterface(obj->name);   

      /* set up xmit Hash Policy */
      convertXmitHashPolicyString(obj->X_BROADCOM_COM_XmitHashPolicy, xmitStr, sizeof(xmitStr)-1);
      snprintf(cmd, sizeof(cmd), "echo %s >/sys/class/net/%s/bonding/xmit_hash_policy",
         xmitStr, obj->name);
      rut_doSystemAction(__FUNCTION__, cmd); 
 

      /* set up bonding interface mode */
      snprintf(cmd, sizeof(cmd), "echo %s > /sys/class/net/%s/bonding/mode",  
         obj->X_BROADCOM_COM_Mode, obj->name);
      rut_doSystemAction(__FUNCTION__, cmd); 


      /* Perform non-configurable mii monitor setup here */
      snprintf(cmd, sizeof(cmd), "echo %d > /sys/class/net/%s/bonding/miimon",
         obj->X_BROADCOM_COM_Miimon, obj->name);
      rut_doSystemAction(__FUNCTION__, cmd); 

      /* Perform non-configurable mii monitor setup here */
      snprintf(cmd, sizeof(cmd), "echo %s > /sys/class/net/%s/bonding/ad_select",
         obj->X_BROADCOM_COM_SelectionLogic, obj->name);
      rut_doSystemAction(__FUNCTION__, cmd); 


      /* Perform LACP Rate setup here */
      snprintf(cmd, sizeof(cmd), "echo %s > /sys/class/net/%s/bonding/lacp_rate",
         obj->X_BROADCOM_COM_LacpRate, obj->name);
      rut_doSystemAction(__FUNCTION__, cmd); 
   }
   else
   {
      /* remove the bonding interface */
      /* No need to remove the old info ? */
   }

   /* For add, move the eth ports from bridge to ethLag.
   * for delete (!add), move the eth port back from ethLag to bridge 
   * 
   */
   if (add)
   {
      /* Remove from bridge first and then add to ethLag */
      rutEthLag_configEthIntfOnBridge("br0", obj->X_BROADCOM_COM_EthIfName1, !add);
      rutEthLag_configEthIntfOnBridge("br0", obj->X_BROADCOM_COM_EthIfName2, !add);   
      rutEthLag_configEthIntfOnLagIntf(obj->name, obj->X_BROADCOM_COM_EthIfName1, add);
      rutEthLag_configEthIntfOnLagIntf(obj->name, obj->X_BROADCOM_COM_EthIfName2, add);
   }
   else
   {
      /* Remove from ethLag  first and then add back to bridge  */
      rutEthLag_configEthIntfOnLagIntf(obj->name, obj->X_BROADCOM_COM_EthIfName1, add);
      rutEthLag_configEthIntfOnLagIntf(obj->name, obj->X_BROADCOM_COM_EthIfName2, add);   
      rutEthLag_configEthIntfOnBridge("br0", obj->X_BROADCOM_COM_EthIfName1, !add);
      rutEthLag_configEthIntfOnBridge("br0", obj->X_BROADCOM_COM_EthIfName2, !add);   
   }

   if (add)
   {
      /* enable ethLag object  */
      rutEthLag_enableInterface(obj->name);     
   }
   else
   {
      /* disable the ethLag interface first */
      rutEthLag_disableInterface(obj->name); 
   }
   
   return;
}


void rutEthLag_enableInterface(const char *ifName)
{
   char cmd[BUFLEN_256] = {0};

   /* set eth bond interface up */
   snprintf(cmd, sizeof(cmd), "ifconfig %s up", ifName);
   rut_doSystemAction(__FUNCTION__, cmd); 
}

void rutEthLag_disableInterface(const char *ifName)
{
   char cmd[BUFLEN_256] = {0};

   /* set  eth bond interface down */
   snprintf(cmd, sizeof(cmd), "ifconfig %s down", ifName);
   rut_doSystemAction(__FUNCTION__,  cmd); 
}


static UBOOL8 systemEthLagIntfs(char *bond0Intf, SINT32 bond0IntfStrLen, char *bond1Intf, SINT32 bond1IntfStrLen)
{

#ifdef DESKTOP_LINUX
   cmsUtl_strncpy(bond0Intf, DEFAULT_ETHLAG_0, bond0IntfStrLen-1);
   cmsUtl_strncpy(bond1Intf, DEFAULT_ETHLAG_1, bond1IntfStrLen-1);   
   return TRUE;
#endif

   char line[BUFLEN_256];
   FILE* fs;  
   char b0[BUFLEN_64];
   char b1[BUFLEN_64];
   UBOOL8 found = FALSE;
      
   rut_doSystemAction("rut", "cat /sys/class/net/bonding_masters > /var/bondintfs");

   fs = fopen("/var/bondintfs", "r");
      
   if ( fs != NULL ) 
   {
      while (!found && fgets(line, sizeof(line), fs) )
      {
         sscanf(line, "%s %s", b0, b1);
         if ((cmsUtl_strcmp(b0, DEFAULT_ETHLAG_0) == 0) && (cmsUtl_strcmp(b1,DEFAULT_ETHLAG_1) == 0)) 
         {
            found = TRUE;
            if (bond0Intf != NULL)
            {
               cmsUtl_strncpy(bond0Intf, b0, bond0IntfStrLen-1);
            }
            if (bond1Intf != NULL)
            {
               cmsUtl_strncpy(bond1Intf, b1, bond1IntfStrLen-1);
            }            
         }
      }
      fclose(fs);
      unlink("/var/bondintfs");
   }

   cmsLog_debug("found bond0Intf %s bond1Intf %s", bond0Intf, bond1Intf);
   return found;
   
}   



CmsRet rutEthLag_getAvailableIntf(char *ethLagIntfs, SINT32 ethLagIntfsLen)
{
 	Dev2EthernetInterfaceObject *ethLagObj=NULL;
	InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
	char b0[CMS_IFNAME_LENGTH]={0};
 	char b1[CMS_IFNAME_LENGTH]={0};
	CmsRet ret = CMSRET_SUCCESS;
	
	if (ethLagIntfs == NULL)
	{
		cmsLog_error("ethLagIntfs is NULL!");
		return CMSRET_INVALID_ARGUMENTS;
	}

	/* First get the system available EthLag interfaces */
	if (!systemEthLagIntfs(b0, sizeof(b0), b1, sizeof(b1)))
	{
		cmsLog_error("No ethernet LAG interfaces found - Ethernet LAG driver is not installed ?");			
		return CMSRET_INTERNAL_ERROR;
	}

	/* Look thru configured EthLag object */
	while (cmsObj_getNextFlags(MDMOID_LA_G,
                              &iidStack,
                              OGF_NO_VALUE_UPDATE,
                              (void **) &ethLagObj) == CMSRET_SUCCESS)
	{

		if (!cmsUtl_strcmp(ethLagObj->name, b0))
		{	
			b0[0] = '\0';
		}
		else if (!cmsUtl_strcmp(ethLagObj->name, b1))
		{
			b1[0] = '\0';
		}
		cmsObj_free((void **)&ethLagObj);
	}
	
	if (b0[0] != '\0')
	{
		cmsUtl_strncat(ethLagIntfs, ethLagIntfsLen, b0);
	}
	else if (b1[0] != '\0')
	{
		cmsUtl_strncat(ethLagIntfs, ethLagIntfsLen, b1);
	}
	else 
	{
	   *ethLagIntfs = '\0';
	   ret = CMSRET_INTERNAL_ERROR;   
	}
	
   cmsLog_debug("ethLagIntfs=%s, ret %d", ethLagIntfs, ret);

   return ret;
}


#endif /* DMP_DEVICE2_ETHLAG_1  */
#endif /* DMP_DEVICE2_BASELINE_1 */
