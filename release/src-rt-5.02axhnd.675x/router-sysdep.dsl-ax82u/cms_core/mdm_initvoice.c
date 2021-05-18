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

#include "cms.h"
#include "cms_util.h"
#include "cms_core.h"
#include "cms_mem.h"
#include "mdm.h"
#include "mdm_private.h"

#ifdef BRCM_VOICE_SUPPORT
#include "linux/rut_voice.h"

/* Validation function for build specific objects */
static CmsRet validateBuildObjects(void);

CmsRet mdm_adjustForVoiceHardware(void)
{
   CmsRet ret;
   int i,j;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   void *obj=NULL;
   MdmPathDescriptor pathDesc;
   int maxCodecs,maxVoipEndpt,maxPhysEndpt,numFxoEndpt, numFxsEndpt;

   /* Get system wide settings from RUT layer */
   rutVoice_getMaxCodecs( &maxCodecs );
   rutVoice_getMaxVoipEndpt( &maxVoipEndpt );
   rutVoice_getMaxPhysEndpt( &maxPhysEndpt );
   rutVoice_getNumFxoEndpt( &numFxoEndpt );
   rutVoice_getNumFxsEndpt( &numFxsEndpt );

   cmsLog_notice("mdm_adjustForVoiceHardware: maxPhysEndpt %d, maxVoipEndpt %d, numFxoEndpt %d, maxCodecs %d\n",
         maxPhysEndpt, maxVoipEndpt, numFxoEndpt, maxCodecs );

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
      return ret;
   }

   /* VoiceService.{i}. */
   cmsLog_notice("Creating VoiceService.1 sub-tree");
   INIT_PATH_DESCRIPTOR(&pathDesc);
   pathDesc.oid = MDMOID_VOICE;

   if ((ret = mdm_addObjectInstance(&pathDesc, NULL, NULL)) != CMSRET_SUCCESS)
   {
      cmsLog_error("mdm_adjustForVoiceHardware: Failed VS\n");
      return ret;
   }

   /* VoiceService.{i}.Capabilities.Codecs.{i}. */
   for ( i = 0; i < maxCodecs; i++)
   {
      cmsLog_notice("Creating VoiceService.1.Capabilities.Codecs.%d sub-tree", i+1);
      INIT_PATH_DESCRIPTOR(&pathDesc);
      pathDesc.oid = MDMOID_VOICE_CAP_CODECS;
      PUSH_INSTANCE_ID(&pathDesc.iidStack, 1);

      if ((ret = mdm_addObjectInstance(&pathDesc, NULL, NULL)) != CMSRET_SUCCESS)
      {
         cmsLog_error("mdm_adjustForVoiceHardware: Failed VSCAPCOD\n");
         return ret;
      }
   }

   /* VoiceService.{i}.PhyInterface.{i}. */
   for ( i = 1; i <= maxPhysEndpt; i++)
   {
      cmsLog_notice("Creating VoiceService.1.PhyInterface.%d sub-tree", i);

      INIT_PATH_DESCRIPTOR(&pathDesc);
      pathDesc.oid = MDMOID_VOICE_PHY_INTF;
      PUSH_INSTANCE_ID(&pathDesc.iidStack, 1);
      PUSH_INSTANCE_ID(&pathDesc.iidStack, i);

      if ((ret = mdm_addObjectInstance(&pathDesc, NULL, NULL)) != CMSRET_SUCCESS)
      {
         cmsLog_error("mdm_adjustForVoiceHardware: Failed VSPHY %d\n", ret);
         return ret;
      }
   }

#if DMP_X_BROADCOM_COM_PSTNENDPOINT_1
   /* VoiceService.{i}.PSTN.{i}. */
   for ( i = 0; i < numFxoEndpt; i++)
   {
      cmsLog_notice("Creating VoiceService.1.PSTN.%d sub-tree", i+1);

      INIT_PATH_DESCRIPTOR(&pathDesc);
      pathDesc.oid = MDMOID_VOICE_PSTN;
      PUSH_INSTANCE_ID(&pathDesc.iidStack, 1);

      if ((ret = mdm_addObjectInstance(&pathDesc, NULL, NULL)) != CMSRET_SUCCESS)
      {
         cmsLog_error("mdm_adjustForVoiceHardware: Failed VSPSTN %d\n", ret);
         return ret;
      }
   }
#endif

#ifdef DMP_X_BROADCOM_COM_NTR_1
   /* VoiceService.{i}.X_BROADCOM_COM_Ntr.History.{i} */
   for( i = 0 ; i < 10 ; i++ )
   {
      cmsLog_notice("Creating VoiceService.1.X_BROADCOM_COM_Ntr.History.%d sub-tree", i+1);

      INIT_PATH_DESCRIPTOR(&pathDesc);
      pathDesc.oid = MDMOID_VOICE_NTR_HISTORY;
      PUSH_INSTANCE_ID(&pathDesc.iidStack, 1);

      if ((ret = mdm_addObjectInstance(&pathDesc, NULL, NULL)) != CMSRET_SUCCESS)
      {
         cmsLog_error("mdm_adjustForVoiceHardware: Failed VSNTR %d\n", ret);
         return ret;
      }
   }
#endif /* DMP_X_BROADCOM_COM_NTR_1 */

   /* VoiceService.{i}.VoiceProfile.{i}. */
   cmsLog_notice("Creating VoiceService.1.VoiceProfile.1 sub-tree");
   INIT_PATH_DESCRIPTOR(&pathDesc);
   pathDesc.oid = MDMOID_VOICE_PROF;
   PUSH_INSTANCE_ID(&pathDesc.iidStack, 1);

   if ((ret = mdm_addObjectInstance(&pathDesc, NULL, NULL)) != CMSRET_SUCCESS)
   {
      cmsLog_error("mdm_adjustForVoiceHardware: Failed VSVP\n");
      return ret;
   }

   /* VoiceService.{i}.VoiceProfile.{i}.Line.{i} */
#ifdef SIPLOAD
   for ( i = 0; i < maxPhysEndpt ; i++)
#else
   for ( i = 0; i < maxPhysEndpt - numFxoEndpt ; i++)
#endif
   {
      cmsLog_notice("Creating VoiceService.1.VoiceProfile.1.Line.%d sub-tree", i+1);

      INIT_PATH_DESCRIPTOR(&pathDesc);
      pathDesc.oid = MDMOID_VOICE_LINE;
      PUSH_INSTANCE_ID(&pathDesc.iidStack, 1);
      PUSH_INSTANCE_ID(&pathDesc.iidStack, 1);

      if ((ret = mdm_addObjectInstance(&pathDesc, NULL, NULL)) != CMSRET_SUCCESS)
      {
         cmsLog_error("mdm_adjustForVoiceHardware: Failed VSVPL\n");
         return ret;
      }
   }

   /* VoiceService.{i}.VoiceProfile.{i}.Line.{i}.Codec.List.{i}. */
   for ( i = 0; i < maxPhysEndpt - numFxoEndpt; i++)
   {
      for ( j = 0; j < maxCodecs; j++)
      {
         cmsLog_notice("Creating VoiceService.1.VoiceProfile.1.Line.%d.Codec.List.%d sub-tree",i+1,j+1);
         INIT_PATH_DESCRIPTOR(&pathDesc);
         pathDesc.oid = MDMOID_VOICE_LINE_CODEC_LIST;
         PUSH_INSTANCE_ID(&pathDesc.iidStack, 1);
         PUSH_INSTANCE_ID(&pathDesc.iidStack, 1);
         PUSH_INSTANCE_ID(&pathDesc.iidStack, i+1);

         if ((ret = mdm_addObjectInstance(&pathDesc, NULL, NULL)) != CMSRET_SUCCESS)
         {
            cmsLog_error("mdm_adjustForVoiceHardware: Failed VSVPLCODLST\n");
            return ret;
         }
      }
   }

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
#if defined(DMP_X_BROADCOM_COM_NTR_1) || defined(DMP_X_BROADCOM_COM_PSTNENDPOINT_1)
   CmsRet ret = CMSRET_SUCCESS;
   void *obj=NULL;
   int i;
   MdmPathDescriptor pathDesc;
   InstanceIdStack iidStack;
#endif

/* PSTN related objects check */
#ifdef DMP_X_BROADCOM_COM_PSTNENDPOINT_1
   int numFxoEndpt;

   rutVoice_getNumFxoEndpt( &numFxoEndpt );
   if( numFxoEndpt == 0 )
   {
      /* No Fxo endpts, no need for this check */
   }
   else
   {
      /* look for InternetGatewayDevice.Services.VoiceService.1.X_BROADCOM_COM_PSTN.{i}. */
      INIT_INSTANCE_ID_STACK(&iidStack);
      for ( i = 0; i < numFxoEndpt; i++)
      {
         ret = mdm_getNextObject(MDMOID_VOICE_PSTN, &iidStack, &obj);
         if( ret == CMSRET_SUCCESS )
         {
            /* The pstn object exists, do nothing */
            cmsLog_debug("VoiceService.1.X_BROADCOM_COM_PSTN.%d object existance validated\n", i+1);
            mdm_freeObject(&obj);
         }
         else
         {
            /* We need to create a new pstn object */
            cmsLog_debug("Creating VoiceService.1.X_BROADCOM_COM_PSTN.%d sub-tree", i+1);

            INIT_PATH_DESCRIPTOR(&pathDesc);
            pathDesc.oid = MDMOID_VOICE_PSTN;
            PUSH_INSTANCE_ID(&pathDesc.iidStack, 1);

            if ((ret = mdm_addObjectInstance(&pathDesc, NULL, NULL)) != CMSRET_SUCCESS)
            {
               cmsLog_error("validateBuildObjects: Failed VSPSTN %d\n", ret);
               return ret;
            }
            else
            {
               iidStack = pathDesc.iidStack;
            }
         }
      }
   }
#endif /* DMP_X_BROADCOM_COM_PSTNENDPOINT_1 */

   /* NTR related objects check */
#ifdef DMP_X_BROADCOM_COM_NTR_1

   INIT_INSTANCE_ID_STACK(&iidStack);
   /* VoiceService.{i}.X_BROADCOM_COM_Ntr.History.{i} */
   for( i = 0 ; i < 10 ; i++ )
   {
      ret = mdm_getNextObject(MDMOID_VOICE_NTR_HISTORY, &iidStack, &obj);
      if( ret == CMSRET_SUCCESS )
      {
         /* The NTR History object exists, do nothing */
         cmsLog_debug("VoiceService.1.X_BROADCOM_COM_Ntr.History.%d. already exists \n", i+1);
         mdm_freeObject(&obj);
      }
      else
      {
         /* We need to create a new NTR History object */
         cmsLog_debug("Creating VoiceService.1.X_BROADCOM_COM_Ntr.History.%d sub-tree", i+1);

         INIT_PATH_DESCRIPTOR(&pathDesc);
         pathDesc.oid = MDMOID_VOICE_NTR_HISTORY;
         PUSH_INSTANCE_ID(&pathDesc.iidStack, 1);

         if ((ret = mdm_addObjectInstance(&pathDesc, NULL, NULL)) != CMSRET_SUCCESS)
         {
            cmsLog_error("validateBuildObjects: Failed adding NTR History instance %d\n", ret);
            return ret;
         }
         else
         {
            iidStack = pathDesc.iidStack;
         }
      }
   }
#endif /* DMP_X_BROADCOM_COM_NTR_1 */

   return CMSRET_SUCCESS;
}

#endif /* BRCM_VOICE_SUPPORT */


