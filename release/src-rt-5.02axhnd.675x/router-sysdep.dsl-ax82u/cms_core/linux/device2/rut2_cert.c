/***********************************************************************
 *
 *  Copyright (c) 2009-2015  Broadcom Corporation
 *  All Rights Reserved
 *
<:label-BRCM:2015:proprietary:standard

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


#ifdef DMP_DEVICE2_CERTIFICATES_1


#include "cms_core.h"
#include "cms_util.h"
#include "cms_dal.h"
#include "rcl.h"


static UBOOL8 isCertificateExisted(const char *certName)
{
    UBOOL8 found = FALSE;
    Dev2SecurityCertificateObject *certObj = NULL;
    InstanceIdStack certIidStack = EMPTY_INSTANCE_ID_STACK;
    CmsRet ret = CMSRET_SUCCESS;

    if (certName == NULL)
    {
        cmsLog_error("Cannot find Dev2SecurityCertificateObject that has its name as NULL");
        return found;
    }
    
    /* search instance that has name matched with the given name */
    while ((!found) &&
           (ret = cmsObj_getNextFlags(MDMOID_DEV2_SECURITY_CERTIFICATE, 
                                      &certIidStack,
                                      OGF_NO_VALUE_UPDATE,
                                      (void **) &certObj) == CMSRET_SUCCESS))
    {
        if (certObj->X_BROADCOM_COM_Name != NULL &&
            cmsUtl_strcmp(certObj->X_BROADCOM_COM_Name, certName) == 0)
        {
            found = TRUE;
        }

        cmsObj_free((void **) &certObj);
    }

    return found;
}


CmsRet rutCert_addCertificate_dev2(const CertificateCfgObject *certCfgObj)
{
    UINT32 numberOfArgs = 6;
    char serialNumber[BUFLEN_264], signature[BUFLEN_264];
    char notBefore[BUFLEN_264], notAfter[BUFLEN_264];
    char issuer[BUFLEN_264], subject[BUFLEN_264];
    char dateTimeBuf[BUFLEN_64];
    Dev2SecurityCertificateObject *certObj = NULL;
    InstanceIdStack certIidStack = EMPTY_INSTANCE_ID_STACK;
    CmsRet ret = CMSRET_SUCCESS;

    if (certCfgObj == NULL)
    {
        return ret;
    }

    if (certCfgObj->type == NULL ||
        certCfgObj->content == NULL ||
        certCfgObj->name == NULL)
    {
        return ret;
    }

    // do nothing if certificate object is already existed
    if (isCertificateExisted(certCfgObj->name) == TRUE)
    {
        cmsLog_debug("Dev2SecurityCertificateObject with name as '%s' already existed", certCfgObj->name);
        return ret;
    }

    memset(serialNumber, 0, sizeof(serialNumber));
    memset(signature, 0, sizeof(signature));
    memset(notBefore, 0, sizeof(notBefore));
    memset(notAfter, 0, sizeof(notAfter));
    memset(issuer, 0, sizeof(issuer));
    memset(subject, 0, sizeof(subject));

    // retrieve information of certificate configuration object
    ret = rutCert_retrieveInfo(certCfgObj, numberOfArgs,
                               serialNumber, signature,
                               issuer, notBefore,
                               notAfter, subject);

    if (ret != CMSRET_SUCCESS)
    {
        cmsLog_error("Failed to retrieve certificate information. ret=%d", ret);
        return ret;
    }

    // add certificate object
    if ((ret = cmsObj_addInstance(MDMOID_DEV2_SECURITY_CERTIFICATE,
                                  &certIidStack)) != CMSRET_SUCCESS)
    {
        cmsLog_error("cmsObj_addInstance for MDMOID_DEV2_SECURITY_CERTIFICATE failed, ret=%d", ret);
        return ret;
    }

    // get certificate object
    if ((ret = cmsObj_get(MDMOID_DEV2_SECURITY_CERTIFICATE,
                          &certIidStack,
                          OGF_DEFAULT_VALUES,
                          (void **) &certObj)) != CMSRET_SUCCESS)
    {
        cmsLog_error("failed to get Dev2SecurityCertificateObject object, ret=%d", ret);
        cmsObj_deleteInstance(MDMOID_DEV2_SECURITY_CERTIFICATE, &certIidStack);
        return ret;
    }

    certObj->enable = TRUE;
    // use current boot time as last modified time
    memset(dateTimeBuf, 0, sizeof(dateTimeBuf));
    cmsTms_getXSIDateTime(0, dateTimeBuf, sizeof(dateTimeBuf));
    CMSMEM_REPLACE_STRING_FLAGS(certObj->lastModif, dateTimeBuf, mdmLibCtx.allocFlags);
    CMSMEM_REPLACE_STRING_FLAGS(certObj->serialNumber, serialNumber, mdmLibCtx.allocFlags);
    CMSMEM_REPLACE_STRING_FLAGS(certObj->issuer, issuer, mdmLibCtx.allocFlags);
    // need to format correctly notBefore and notAfter
    CMSMEM_REPLACE_STRING_FLAGS(certObj->notBefore, notBefore, mdmLibCtx.allocFlags);
    CMSMEM_REPLACE_STRING_FLAGS(certObj->notAfter, notAfter, mdmLibCtx.allocFlags);
    CMSMEM_REPLACE_STRING_FLAGS(certObj->subject, subject, mdmLibCtx.allocFlags);
    // alternative subject is not supported yet
    //CMSMEM_REPLACE_STRING_FLAGS(certObj->subjectAlt, "", mdmLibCtx.allocFlags);
    CMSMEM_REPLACE_STRING_FLAGS(certObj->signatureAlgorithm, signature, mdmLibCtx.allocFlags);
    CMSMEM_REPLACE_STRING_FLAGS(certObj->X_BROADCOM_COM_Name, certCfgObj->name, mdmLibCtx.allocFlags);

    cmsLog_debug("Before set Dev2SecurityCertificateObject, name='%s', serialNumber='%s', signature='%s', issuer='%s', notBefore='%s', notAfter='%s', subject='%s'",
                 certObj->X_BROADCOM_COM_Name, serialNumber,
                 signature, issuer, notBefore, notAfter, subject);

    // set certificate object
    ret = cmsObj_set((void *)certObj, &certIidStack);

    if (ret != CMSRET_SUCCESS)
    {
        cmsLog_error("Failed to set Dev2SecurityCertificateObject. ret=%d", ret);
        cmsObj_deleteInstance(MDMOID_DEV2_SECURITY_CERTIFICATE, &certIidStack);
    }

    // free certificate object
    cmsObj_free((void **) &certObj);

    return ret;
}


CmsRet rutCert_deleteCertificate_dev2(const CertificateCfgObject *certCfgObj)
{
    UBOOL8 found = FALSE;
    Dev2SecurityCertificateObject *certObj = NULL;
    InstanceIdStack certIidStack = EMPTY_INSTANCE_ID_STACK;
    CmsRet ret = CMSRET_SUCCESS;

    if (certCfgObj->name == NULL)
    {
        cmsLog_error("Cannot delete Dev2SecurityCertificateObject that has its name as NULL");
        return ret;
    }
    
    /* search instance that has name matched with the given name */
    while ((!found) &&
           (ret = cmsObj_getNextFlags(MDMOID_DEV2_SECURITY_CERTIFICATE, 
                                      &certIidStack,
                                      OGF_NO_VALUE_UPDATE,
                                      (void **) &certObj) == CMSRET_SUCCESS))
    {
        if (certObj->X_BROADCOM_COM_Name != NULL &&
            cmsUtl_strcmp(certObj->X_BROADCOM_COM_Name, certCfgObj->name) == 0)
        {
            found = TRUE;
        }

        cmsObj_free((void **) &certObj);
    }

    if (found == TRUE)
    {
        cmsObj_deleteInstance(MDMOID_DEV2_SECURITY_CERTIFICATE, &certIidStack);
    }

    return ret;
}


#endif /* DMP_DEVICE2_CERTIFICATES_1 */
