/***********************************************************************
 *
 *  Copyright (c) 2006 - 2009  Broadcom Corporation
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
#ifdef BRCM_VOICE_SUPPORT
#ifdef DMP_VOICE_SERVICE_2

#include "cms.h"
#include "cms_util.h"
#include "cms_core.h"
#include "cms_mem.h"
#include "cms_msg.h"
#include "mdm.h"
#include "mdm_private.h"
#include "linux/rut2_voice.h"

/* Validation function for build specific objects */
static CmsRet validateBuildObjects(void);

/*****************************************************************************
**  FUNCTION:       setBoundIfName
**
**  PURPOSE:        Sets the default boundIfName
**
**  INPUT PARMS:    None.
**
**  OUTPUT PARMS:   None.
**
**  RETURNS:        None
**
*****************************************************************************/
static void setBoundIfName(void)
{
#if defined(EPON_SFU) && defined(BRCM_PKTCBL_SUPPORT)
   VoiceObject *voiceObj = NULL;
   CmsRet ret = CMSRET_SUCCESS;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;

   cmsLog_debug("Setting VoiceService.1.X_BROADCOM_COM_BoundIfName as %s\n",
                EPON_VOICE_WAN_IF_NAME);
   ret = mdm_getNextObject(MDMOID_VOICE, &iidStack, (void **)&voiceObj);
   if( ret == CMSRET_SUCCESS )
   {
       REPLACE_STRING_IF_NOT_EQUAL_FLAGS(voiceObj->X_BROADCOM_COM_BoundIfName,
                                         EPON_VOICE_WAN_IF_NAME,
                                         mdmLibCtx.allocFlags);
       mdm_setObject((void **)&voiceObj, &iidStack, FALSE );
   }
   mdm_freeObject((void **)&voiceObj);
#endif /* defined(EPON_SFU) && defined(BRCM_PKTCBL_SUPPORT) */
}

/*****************************************************************************
**  FUNCTION:       mdm_adjustForVoiceHardware
**
**  PURPOSE:        Adjusts MDM	settings depending on the hardware configuration
**
**  INPUT PARMS:    None.
**
**  OUTPUT PARMS:   None.
**
**  RETURNS:        CMSRET_SUCCESS if valid, error code otherwise
**
*****************************************************************************/
CmsRet mdm_adjustForVoiceHardware(void)
{
   CmsRet ret = CMSRET_SUCCESS;
#if defined( DMP_BASELINE_1 ) || defined( DMP_DEVICE2_BASELINE_1 )
   int i;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   void *obj=NULL;
   MdmPathDescriptor pathDesc;
   UINT32    iid;
   int maxCodecs,maxVoipEndpt,maxPhysEndpt,numFxoEndpt, numFxsEndpt;

   /* Get system wide settings from RUT layer */
   rutVoice_getMaxCodecs( &maxCodecs );
   rutVoice_getMaxVoipEndpt( &maxVoipEndpt );
   rutVoice_getMaxPhysEndpt( &maxPhysEndpt );
   rutVoice_getNumFxoEndpt( &numFxoEndpt );
   rutVoice_getNumFxsEndpt( &numFxsEndpt );

   cmsLog_debug("mdm_adjustForVoiceHardware 1: maxPhysEndpt %d, maxVoipEndpt %d, numFxoEndpt %d, numFxsEndpt %d, maxCodecs %d\n",
         maxPhysEndpt, maxVoipEndpt, numFxoEndpt, numFxsEndpt, maxCodecs );

   /* look for InternetGatewayDevice.Services.VoiceService.1 */
   ret = mdm_getNextObject(MDMOID_VOICE, &iidStack, &obj);
   if (CMSRET_SUCCESS == ret )
   {
      /*
       * We found a InternetGatewayDevice.Services.VoiceService.1 object,
       * so we don't have to create it. Check build specific objects which
       * can be compiled in/out. If they are missing, add them then return.
       */
      mdm_freeObject(&obj);
      ret = validateBuildObjects();
      setBoundIfName();
      return ret;
   }

   /* VoiceService.{i}. */
   cmsLog_debug("Creating VoiceService.1 sub-tree\n");
   INIT_PATH_DESCRIPTOR(&pathDesc);
   pathDesc.oid = MDMOID_VOICE;

   if ((ret = mdm_addObjectInstance(&pathDesc, NULL, NULL)) != CMSRET_SUCCESS)
   {
      cmsLog_error("%s: Failed MDMOID_VOICE\n",__FUNCTION__);
      return ret;
   }

   /* Set the boundIfName if required */
   setBoundIfName();

   /* VoiceService.{i}.Capabilities.MaxCallLogCount
    * This is set in the XML file.  It also is prevented from being saved to flash
    * for some reason.  Further investigation is required why that is the case. */

   /* VoiceService.{i}.Capabilities.Codecs.{i}. */
   cmsLog_debug("Creating VoiceService.1.Capabilities.Codecs.{i} sub-tree\n");
   for ( i = 0; i < maxCodecs; i++)
   {
      INIT_PATH_DESCRIPTOR(&pathDesc);
      pathDesc.oid = MDMOID_VOICE_CAP_CODECS;
      PUSH_INSTANCE_ID(&pathDesc.iidStack, 1);

      if ((ret = mdm_addObjectInstance(&pathDesc, NULL, NULL)) != CMSRET_SUCCESS)
      {
         cmsLog_error("mdm_adjustForVoiceHardware: Failed VSCAPCOD\n");
         return ret;
      }
      else
      {
         VoiceCapCodecsObject *pCodec = NULL;
         char pTime[30], codecName[64];
         int  bitRate, silSup = 0;

         ret = mdm_getObject( pathDesc.oid, &pathDesc.iidStack, (void **)&pCodec);
         iid = PEEK_INSTANCE_ID( &pathDesc.iidStack ); /* get current instance */
         rutVoice_getCodecPtime( iid, pTime, sizeof(pTime));
         rutVoice_getCodecName(  iid, codecName, sizeof(codecName));
         rutVoice_getCodecSilSup(  iid, &silSup );
         rutVoice_getCodecBitRate(  iid, &bitRate );
         if(ret == CMSRET_SUCCESS)
         {
            cmsLog_debug("%s Creating codec (%s) ptime (%s) bitrate (%d) silSup (%d)\n", __FUNCTION__, codecName, pTime, bitRate, silSup);
            CMSMEM_REPLACE_STRING_FLAGS(pCodec->codec, codecName, mdmLibCtx.allocFlags);
            CMSMEM_REPLACE_STRING_FLAGS(pCodec->packetizationPeriod, pTime, mdmLibCtx.allocFlags);
            pCodec->bitRate = bitRate;
            pCodec->silenceSuppression = silSup?TRUE:FALSE;
            if( mdm_setObject((void **)&pCodec, &pathDesc.iidStack, FALSE) != CMSRET_SUCCESS){
                cmsLog_error("%s: Failed set codec object\n", __FUNCTION__);
            }
            mdm_freeObject((void **)&pCodec);
         }
      }
   }


#ifdef DMP_SIPCLIENT_1
#endif /* DMP_SIPCLIENT_1 */

#ifdef DMP_POTS_1
#ifdef DMP_POTSFXS_1
   /* VoiceService.{i}.POTS.FXS.{i} */
   cmsLog_debug("Creating VoiceService.1.POTS.FXS.{i} sub-tree\n");
   for ( i = 0; i < numFxsEndpt; i++)
   {
      INIT_PATH_DESCRIPTOR(&pathDesc);
      pathDesc.oid = MDMOID_POTS_FXS;
      PUSH_INSTANCE_ID(&pathDesc.iidStack, 1);

      if ((ret = mdm_addObjectInstance(&pathDesc, NULL, NULL)) != CMSRET_SUCCESS)
      {
         cmsLog_error("mdm_adjustForVoiceHardware: Failed POTS.FXS.{i}\n");
         return ret;
      }
   }

#endif /* DMP_POTSFXS_1 */

#ifdef DMP_POTSFXO_1
   /* VoiceService.{i}.POTS.FXO.{i} */
   cmsLog_debug("Creating VoiceService.1.POTS.FXO.{i} sub-tree, numFXO %d\n", numFxoEndpt);
   for ( i = 0; i < numFxoEndpt; i++)
   {
      INIT_PATH_DESCRIPTOR(&pathDesc);
      pathDesc.oid = MDMOID_POTS_FXO;
      PUSH_INSTANCE_ID(&pathDesc.iidStack, 1);

      if ((ret = mdm_addObjectInstance(&pathDesc, NULL, NULL)) != CMSRET_SUCCESS)
      {
         cmsLog_error("mdm_adjustForVoiceHardware: Failed POTS.FXO.{i}\n");
         return ret;
      }
   }
#endif /* DMP_POTSFXO_1 */
#endif /* DMP_POTS_1 */

#ifdef DMP_CALLCONTROL_1
#endif /* DMP_CALLCONTROL_1 */

#ifdef DMP_DMP_CALLINGFEATURES_1
#endif /* DMP_DMP_CALLINGFEATURES_1 */

#ifdef DMP_VOIPPROFILE_1
   /* no object is needed now */
#endif /* DMP_VOIPPROFILE_1 */

   /* Send voice default message */
   {
      CmsMsgHeader reqMsg = EMPTY_MSG_HEADER;
      reqMsg.type = CMS_MSG_DEFAULT_VOICE;
      reqMsg.src = EID_SSK;
      reqMsg.dst = EID_SSK;
      reqMsg.flags_request = 1;
      if ((ret = cmsMsg_send(mdmLibCtx.msgHandle, &reqMsg)) != CMSRET_SUCCESS)
      {
         cmsLog_error("CMS_MSG_DEFAULT_VOICE event msg failed. ret=%d", ret);
      }
   }

#endif /* defined( DMP_BASELINE_1 ) || defined( DMP_DEVICE2_BASELINE_1 ) */
   return ret;
}

/* Here we check if all build specific objects are present. Currently the old config file in flash
 * is validated before it is loaded. If parameters or objects are changed in the new image, then a
 * new config file will be generated. Ex: if we had a Sip image and are now loading an Mgcp image,
 * the Sip objects in the old config file will be found invalid.
 *
 * The problem is when there are no invalid objects/parameters in the old config file. It will then
 * be loaded and used, but we will not know if we have all the required objects for the new image
 * in the old config file. Ex: Non Fxo image was old image and we now load a Fxo image. When the old
 * config file is validated, it will not see a pstn object, and will never know to check for it in new
 * image.
 *
 * If any other objects are added to data model which can be compiled in/out for the same type of image,
 * they should be checked for here using the specific build defines for the objects.
 */

CmsRet validateBuildObjects(void)
{

   return CMSRET_SUCCESS;
}

#endif /* DMP_VOICE_SERVICE_2 */
#endif /* BRCM_VOICE_SUPPORT */


