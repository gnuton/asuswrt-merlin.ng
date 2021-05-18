/***********************************************************************
 *
 *  Copyright (c) 2006-2008  Broadcom Corporation
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

#ifdef DMP_X_BROADCOM_COM_EPON_1 

#include "cms.h"
#include "cms_core.h"
#include "cms_util.h"
#include "mdm.h"
#include "mdm_private.h"
#include "cms_boardcmds.h"
#include "cms_boardioctl.h"

#define SW_IMAGE_0_VERSION "534F465457415245494D41474530" /*  SOFTWAREIMAGE0*/
#define SW_IMAGE_1_VERSION "534F465457415245494D41474531" /*  SOFTWAREIMAGE1*/

#define SWIMAGE_VER_STR_LEN 14
#define SWIMAGE_VER_HEX_LEN (SWIMAGE_VER_STR_LEN * 2)

static CmsRet addDefaultEponSoftwareImageObject(void)
{
    MdmPathDescriptor pathDesc;
    InstanceIdStack iidStack;
    CmsRet ret;
    UINT32 objInst;
    _EponSoftwareImageObject *mdmObj = NULL;
    
    int AsciiStrLen, Lcv;
    char AsciiVerStr[SWIMAGE_VER_STR_LEN + 1] = "";
    char hexVerStr[SWIMAGE_VER_HEX_LEN + 1] = "";
    int BootPartition = devCtl_getBootedImagePartition();
    int BootState = devCtl_getImageState();

    struct
    {
        int IsActive;
        int IsCommitted ;
        int IsValid;
    } SwImageAttributes[2] = {{FALSE,FALSE,FALSE},{FALSE,FALSE,FALSE}};

    char DEFAULT_VERSION_STRING[2][SWIMAGE_VER_HEX_LEN + 1] =  {SW_IMAGE_0_VERSION, SW_IMAGE_1_VERSION};

    //////////////////////////////////////////////////////////////////////////////
    // use boot state and booted partition values to preset attribute values for both MEs.
    switch (BootPartition)
    {
        case BOOTED_PART1_IMAGE:
        {        
            SwImageAttributes[0].IsActive  = TRUE;              // must be true because we are booted to this image

            if (BOOT_SET_PART1_IMAGE == BootState)
            {
                // boot image == boot state
                SwImageAttributes[0].IsCommitted    = TRUE;     // must be true since bootstate points to this image for the next boot
                SwImageAttributes[0].IsValid        = TRUE;     // must be true since we are booting this image at all
            }
            else
            {
                SwImageAttributes[0].IsValid        = TRUE;     // must be true since we are booted to this image
                SwImageAttributes[1].IsCommitted    = TRUE;     // must be true because boot state points to it for the next boot
                SwImageAttributes[1].IsValid        = TRUE;     // must be true because boot state points to it for the next boot
            }
        
            break;
        }
        case BOOTED_PART2_IMAGE:
        {
            SwImageAttributes[1].IsActive  = TRUE;              // must be true because we are booted to this image

            if (BOOT_SET_PART2_IMAGE == BootState)
            {
                // boot image == boot state
                SwImageAttributes[1].IsCommitted    = TRUE;     // must be true since bootstate points to this image for the next boot
                SwImageAttributes[1].IsValid        = TRUE;     // must be true since we are booting this image at all
            }
            else
            {
                SwImageAttributes[1].IsValid        = TRUE;     // must be true since we are booted to this image
                SwImageAttributes[0].IsCommitted    = TRUE;     // must be true because boot state points to it for the next boot
                SwImageAttributes[0].IsValid        = TRUE;     // must be true because boot state points to it for the next boot
            }
            break;
        }
        default:
        {
            cmsLog_error("Unknown booted partition value %x, failed to set default attribute for sw image MEs \n", BootPartition);
        }
    }
    
    //printf("Active partition = %d, Bootstate = %d\n", BootPartition, BootState);

    //////////////////////////////////////////////////////////////////////////////
    // Create new software image MEs if needed
    INIT_INSTANCE_ID_STACK(&iidStack);
    ret = mdm_getNextObject(MDMOID_EPON_SOFTWARE_IMAGE, &iidStack, (void **)&mdmObj);
    if (ret != CMSRET_SUCCESS)
    {

        // first object doesn't exist so recreate both...
        for(objInst=0; objInst<=1; objInst++)
        {
            cmsLog_debug("Adding %s instance %d", "EponSoftwareImageObject", objInst);
            mdm_initPathDescriptor(&pathDesc);
            pathDesc.oid = MDMOID_EPON_SOFTWARE_IMAGE;
            PUSH_INSTANCE_ID(&(pathDesc.iidStack), objInst + 1);

            if ((ret = mdm_addObjectInstance(&pathDesc, NULL, NULL)) != CMSRET_SUCCESS)
            {
                cmsLog_error("Failed to add %s, ret=%d", "EponSoftwareImageObject", ret);
                return ret;
            }
            /* now get the object we have just created */
            iidStack = pathDesc.iidStack;
            if ((ret = mdm_getObject(MDMOID_EPON_SOFTWARE_IMAGE, &iidStack, (void **)&mdmObj)) != CMSRET_SUCCESS)
            {
                cmsLog_error("Failed to get %s, ret=%d", "SoftwareImageObject", ret);
                return ret;
            }
            
            //now set default values
            mdmObj->isCommitted  = 0;
            mdmObj->isActive     = 0;
            mdmObj->isValid      = 0;            

            /* set the object */
            ret = mdm_setObject((void **)&mdmObj, &iidStack, FALSE);
            if (ret != CMSRET_SUCCESS)
            {
                cmsLog_error("Failed to set %s, ret = %d", "SoftwareImageObject", ret);
                return ret;
            }            
        }
    }

    //////////////////////////////////////////////////////////////////////////////
    // Get each of the software image MEs and set the attribute and version string values

    INIT_INSTANCE_ID_STACK(&iidStack);
    for(objInst=0; objInst<=1; objInst++)
    {    
        if ((ret = mdm_getNextObject(MDMOID_EPON_SOFTWARE_IMAGE, &iidStack, (void **)&mdmObj)) == CMSRET_SUCCESS)
        {
            // set all attribute values for instance 0 on partition 1
            mdmObj->isActive        = SwImageAttributes[objInst].IsActive;
            mdmObj->isCommitted     = SwImageAttributes[objInst].IsCommitted;
            mdmObj->isValid         = mdmObj->isValid || SwImageAttributes[objInst].IsValid;
            mdmObj->managedEntityId = objInst;
            
            memset(&AsciiVerStr, 0x0, sizeof(AsciiVerStr));
            memset(&hexVerStr, 0x0, sizeof(hexVerStr));
            if (0 != (AsciiStrLen = devCtl_getImageVersion(objInst+1, AsciiVerStr, SWIMAGE_VER_STR_LEN)))
            {
                for (Lcv=0; Lcv < AsciiStrLen; Lcv++)
                {
                    sprintf(hexVerStr+2*Lcv, "%.2X",AsciiVerStr[Lcv]);
                }
                //printf("ASCII version string %s extracted from software image %d\n"
                //      "Hexadecimal version string %s added to software image ME %d\n",
                //      AsciiVerStr, objInst, hexVerStr, objInst);
            }
            else
            {
                //printf("ASCII version string NOT extracted from software image %d\n"
                //       "Default hexadecimal version string %s added to software image ME %d\n",
                //       objInst, DEFAULT_VERSION_STRING[objInst], objInst);
                memcpy(hexVerStr, DEFAULT_VERSION_STRING[objInst], SWIMAGE_VER_HEX_LEN);                    
            }

            CMSMEM_REPLACE_STRING_FLAGS(mdmObj->version, hexVerStr, mdmLibCtx.allocFlags);
            
            mdm_setObject((void **)&mdmObj, &iidStack, FALSE);
            mdm_freeObject((void **)&mdmObj);
        }
        else
        {
            cmsLog_error("Failed to get %s %d, ret = %d", "SoftwareImageObject", objInst, ret);
            return ret;
        }
    }
    
    // save Software Image to flash
    cmsMgm_saveConfigToFlash();

    return CMSRET_SUCCESS;
}

CmsRet addDefaultEponObjects(void)
{
    CmsRet ret = CMSRET_SUCCESS;

    if ((ret = cmsLck_acquireLockWithTimeout(6*MSECS_IN_SEC)) != CMSRET_SUCCESS)
    {
        cmsLog_error("failed to get lock, ret=%d", ret);
        return ret;
    }

#if 0
#if defined(DMP_X_ITU_ORG_GPON_1) && defined(DMP_X_ITU_ORG_VOICE_SIP_1)
    // Attempt to halt voice stack with invalid BoundIpAddress.
    if ((ret = haltVoiceStack()) != CMSRET_SUCCESS)
    {
        goto out;
    }

#endif // #if defined(DMP_X_ITU_ORG_GPON_1) && defined(DMP_X_ITU_ORG_VOICE_SIP_1)
#endif

    // addDefaultEponSoftwareImageObject saves Software Image objects
    // to flash if they don't exist yet
    if ((ret = addDefaultEponSoftwareImageObject()) != CMSRET_SUCCESS)
    {
        printf("Error adding DefaultEponSoftwareImageObject \n");
    }

    cmsLck_releaseLock();

    return ret;
}

#endif /* DMP_X_BROADCOM_COM_EPON_1 */

