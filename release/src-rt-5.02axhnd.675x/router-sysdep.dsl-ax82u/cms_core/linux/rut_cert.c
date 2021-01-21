/***********************************************************************
 *
 *  Copyright (c) 2007  Broadcom Corporation
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

#ifdef SUPPORT_CERT

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>

#include "cms.h"
#include "cms_log.h"
#include "cms_mem.h"
#include "cms_util.h"
#include "cms_obj.h"
#include "cms_dal.h"
#include "mdm.h"
#include "rut_util.h"
#ifdef DMP_DEVICE2_CERTIFICATES_1
#include "device2/rut2_cert.h"
#endif

#define CERT_CONF_NAME       "openssl.cnf"
#define CERT_TEMP_NAME        "__temp__"
#define CERT_LOG_NAME         "log"

#define BEGIN_CERT_REQUEST         "BEGIN CERTIFICATE REQUEST-----"



static CmsRet makePathToSslApp(char *buf, UINT32 bufLen);
static void makePathToCert(char *buf, UINT32 bufLen, const char *certFile);


// Set symbolic links for CA certificates (needed by ssl)
void rutCert_setCACertLinks(void)
{
   char cmd[BUFLEN_1024], certPath[BUFLEN_264];
   char sslApp[BUFLEN_40];
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   CertificateCfgObject *certCfg = NULL;
   CmsRet ret = CMSRET_SUCCESS;

   makePathToCert(certPath, BUFLEN_264, "");
   makePathToSslApp(sslApp, BUFLEN_40);

   while ((ret = cmsObj_getNext
         (MDMOID_CERTIFICATE_CFG, &iidStack, (void **) &certCfg)) == CMSRET_SUCCESS)
   {
      if (cmsUtl_strcmp(certCfg->type, CERT_TYPE_CA) == 0)
      {
         sprintf(cmd, "ln -s '%s%s.cacert' %s`%s x509 -noout -hash < '%s%s.cacert'`.0",
                 certPath, certCfg->name, certPath, sslApp, certPath, certCfg->name);
         rut_doSystemAction(__FUNCTION__, cmd);
      }
   }
}

// create Openssl configuration file.
static void createCertConf(const char *confFileName, PCERT_ADD_INFO pAddInfo) 
{
   char line[CERT_BUFF_MAX_LEN+6];


   FILE *fs = fopen(confFileName, "w");
   if ( fs == NULL )
   {
      cmsLog_error("Could not open %s", confFileName);
      return;
   }
   
   // req configuration 
   sprintf(line, "[ req ]\n");
   fputs(line, fs);
   // key size in bits
   sprintf(line, "default_bits = %d\n", CERT_KEY_SIZE);
   fputs(line, fs);
   fputs("distinguished_name = req_distinguished_name\n", fs);
   fputs("prompt = no\n", fs);
   fputs("[ req_distinguished_name ]\n", fs);
   // common name
   sprintf(line, "CN = %s\n", pAddInfo->commonName);
   fputs(line, fs);
   // organization
   sprintf(line, "O = %s\n", pAddInfo->organization);
   fputs(line, fs);
   // state/province
   sprintf(line, "ST = %s\n", pAddInfo->state);
   fputs(line, fs);
   // country
   sprintf(line, "C = %s\n", pAddInfo->country);
   fputs(line, fs);

   fclose(fs);
}


static void deleteCertConf(const char *confFileName) 
{
   char cmd[CERT_BUFF_MAX_LEN];

   sprintf(cmd, "rm -f %s 2> /dev/null", confFileName);
   rut_doSystemAction(__FUNCTION__, cmd); 
}


// create certificate request
CmsRet rutCert_createCertReq(PCERT_ADD_INFO pAddInfo, CertificateCfgObject *certCfg) 
{
   CmsRet ret = CMSRET_SUCCESS;
   char cmd[CERT_BUFF_MAX_LEN*3], fname[CERT_BUFF_MAX_LEN], data[CERT_BUFF_MAX_LEN];
   char certCfgName[BUFLEN_264], certTmpName[BUFLEN_264], certLogName[BUFLEN_264];
   char sslApp[BUFLEN_40];
   FILE* errFs;

   makePathToCert(certCfgName, BUFLEN_264, CERT_CONF_NAME);
   makePathToCert(certTmpName, BUFLEN_264, CERT_TEMP_NAME);
   makePathToCert(certLogName, BUFLEN_264, CERT_LOG_NAME);
   makePathToSslApp(sslApp, BUFLEN_40);

   /* create openssl configuration file */
   createCertConf(certCfgName, pAddInfo);

   /* generate private key */
   sprintf(cmd, "%s genrsa -out %s.key %d 2> %s", sslApp, certTmpName, CERT_KEY_SIZE, certLogName);
   rut_doSystemAction(__FUNCTION__, cmd); 
   data[0] = '\0';
   errFs = fopen(certLogName, "r");
   if (errFs != NULL) 
   {
      // read last line of key gen result for verification 
      while (fgets(data, BUFLEN_264, errFs)) 
      {
         cmsLog_debug("certLog=%s", data);
      }
      fclose(errFs);

      sprintf(cmd, "rm %s 2> /dev/null", certLogName);
      rut_doSystemAction(__FUNCTION__, cmd); 

      if (cmsUtl_strstr(data, "(0x10001)")) 
      {
         SINT32 keyFileSize;

         /* key generated successful */
         sprintf(fname, "%s.key", certTmpName);
         
         if ((keyFileSize = cmsFil_getSize(fname)) > 0)
         {
            cmsLog_debug("keyFileSize=%d", keyFileSize);

            /* add one to ensure there is a trailing NULL */
            keyFileSize++;

            cmsMem_free(certCfg->privKey);
            certCfg->privKey = cmsMem_alloc(keyFileSize, mdmLibCtx.allocFlags);
            if (certCfg->privKey != NULL)
            {
               ret = cmsFil_copyToBuffer(fname, (UINT8 *) certCfg->privKey, (UINT32 *) &keyFileSize);
               cmsLog_debug("ret=%d bytesRead=%d certPrivKey=%s", ret, keyFileSize, certCfg->privKey);
            }
            else
            {
               /* buffer allocation failure */
               cmsLog_error("could not allocate buf of %d bytes for priv key", keyFileSize);
               ret = CMSRET_RESOURCE_EXCEEDED;
            }
         }
         else 
         {
            /* key file open error */
            ret = CMSRET_OPEN_FILE_ERROR;
            cmsLog_error("Could not open key file %s", fname);
         }
      }
      else 
      {
         /* key generation error */
         ret = CMSRET_KEY_GENERATION_ERROR;
         cmsLog_error("key file generation error");
      }
   }
   else 
   {
      /* key generation result file open error */
      ret = CMSRET_OPEN_FILE_ERROR;
      printf("could not open log file %s", certLogName);
   }

   // openssl req -new -key ca.key -out csr1.pem -batch -config openssl2.cnf
   /* generate certificate request */
   if (ret == CMSRET_SUCCESS) 
   {
      SINT32 reqFileSize;

      sprintf(cmd, "%s req -new -key %s.key -out %s.csr -batch -config %s", 
         sslApp, certTmpName, certTmpName,  certCfgName);
      rut_doSystemAction(__FUNCTION__, cmd); 

      /* certificate request created */
      sprintf(fname, "%s.csr", certTmpName);

      if ((reqFileSize = cmsFil_getSize(fname)) > 0)
      {
         /* add one to ensure there is a trailing NULL */
         reqFileSize++;

         cmsMem_free(certCfg->reqPub);
         certCfg->reqPub = cmsMem_alloc(reqFileSize, mdmLibCtx.allocFlags);
         if (certCfg->reqPub != NULL)
         {
            ret = cmsFil_copyToBuffer(fname, (UINT8 *) certCfg->reqPub, (UINT32 *) &reqFileSize);
            cmsLog_debug("ret=%d bytesRead=%d reqPub=%s", ret, reqFileSize, certCfg->reqPub);

            if (ret == CMSRET_SUCCESS)
            {
               /* check for some known string in the reqPub */
               if (cmsUtl_strstr(certCfg->reqPub, BEGIN_CERT_REQUEST) == NULL) 
               {
                  ret = CMSRET_OPEN_FILE_ERROR;
               }
            }
         }
         else
         {
            /* buffer allocation failure */
            cmsLog_error("could not allocate %d bytes for cert req", reqFileSize);
            ret = CMSRET_RESOURCE_EXCEEDED;
         }
      } 
      else 
      {
         /* certificate request open error */
         ret = CMSRET_OPEN_FILE_ERROR;
         cmsLog_error("could not open csr %s", certTmpName);
      }
   }

   /* always delete the config file, we don't need it anymore */
   deleteCertConf(certCfgName);

   if (ret != CMSRET_SUCCESS)
   {
      /*
       * if there is an error, clean up the certCfg object.
       * Also delete the .key and .csr file.  I guess that means
       * if there is no error, we intentionally leave the .key and
       * .csr file in the file system.
       */
      CMSMEM_FREE_BUF_AND_NULL_PTR(certCfg->reqPub);
      CMSMEM_FREE_BUF_AND_NULL_PTR(certCfg->privKey);

      sprintf(cmd, "rm -f %s.key 2> /dev/null", certTmpName);
      rut_doSystemAction(__FUNCTION__, cmd); 

      sprintf(cmd, "rm -f %s.csr 2> /dev/null", certTmpName);
      rut_doSystemAction(__FUNCTION__, cmd); 
   }

   return ret;
}

// create signed certificate or CA certificate
CmsRet rutCert_createCertFile(const CertificateCfgObject *certCfg) 
{
   char fn[BUFLEN_512], certPath[BUFLEN_264];
   FILE *fp = NULL;

   if (certCfg == NULL)
   {
      return CMSRET_SUCCESS;
   }
   if (certCfg->type == NULL || certCfg->content == NULL || certCfg->name == NULL)
   {
      return CMSRET_SUCCESS;
   }

   makePathToCert(certPath, BUFLEN_264, "");
   if (cmsUtl_strcmp(certCfg->type, CERT_TYPE_CA) == 0)
   {
      if (certCfg->content != NULL)
      {
         sprintf(fn, "%s%s.cacert", certPath, certCfg->name);
         fp = fopen(fn, "w");
         if (fp == NULL) 
         {
             return CMSRET_OPEN_FILE_ERROR;
         }
         fprintf(fp, "%s", certCfg->content);
         fclose(fp);
      }
      // reset CA certificate links
      rutCert_setCACertLinks();
   }
   else
   {
      if (certCfg->content != NULL)
      {
         sprintf(fn, "%s%s.cert", certPath, certCfg->name);
         fp = fopen(fn, "w");
         if (fp == NULL) 
         {
             return CMSRET_OPEN_FILE_ERROR;
         }
         fprintf(fp, "%s", certCfg->content);
         fclose(fp);
      }
      if (certCfg->privKey != NULL)
      {
         sprintf(fn, "%s%s.priv", certPath, certCfg->name);
         fp = fopen(fn, "w");
         if (fp == NULL) 
         {
             return CMSRET_OPEN_FILE_ERROR;
         }
         fprintf(fp, "%s", certCfg->privKey);
         fclose(fp);
      }
      if (certCfg->reqPub != NULL)
      {
         sprintf(fn, "%s%s.req", certPath, certCfg->name);
         fp = fopen(fn, "w");
         if (fp == NULL) 
         {
             return CMSRET_OPEN_FILE_ERROR;
         }
         fprintf(fp, "%s", certCfg->reqPub);
         fclose(fp);
      }
   }

#ifdef DMP_DEVICE2_CERTIFICATES_1
   rutCert_addCertificate_dev2(certCfg);
#endif

   return (CMSRET_SUCCESS);
}

// remove signed certificate or CA certificate
CmsRet rutCert_removeCertFile(const CertificateCfgObject *certCfg) 
{
   char fn[BUFLEN_512], certPath[BUFLEN_264], cmd[BUFLEN_512+BUFLEN_32];

   if (certCfg == NULL)
   {
      return CMSRET_SUCCESS;
   }
   if (certCfg->name == NULL || certCfg->type == NULL)
   {
      return CMSRET_SUCCESS;
   }

   makePathToCert(certPath, BUFLEN_264, "");
   if (cmsUtl_strcmp(certCfg->type, CERT_TYPE_CA) == 0)
   {
      sprintf(fn, "%s%s.cacert", certPath, certCfg->name);
      sprintf(cmd, "rm %s 2> /dev/null", fn);
      rut_doSystemAction(__FUNCTION__, cmd); 
      // reset CA certificate links
      rutCert_setCACertLinks();
   }
   else
   {
      sprintf(fn, "%s%s.cert", certPath, certCfg->name);
      sprintf(cmd, "rm %s 2> /dev/null", fn);
      rut_doSystemAction(__FUNCTION__, cmd); 

      sprintf(fn, "%s%s.req", certPath, certCfg->name);
      sprintf(cmd, "rm %s 2> /dev/null", fn);
      rut_doSystemAction(__FUNCTION__, cmd); 

      sprintf(fn, "%s%s.priv", certPath, certCfg->name);
      sprintf(cmd, "rm %s 2> /dev/null", fn);
      rut_doSystemAction(__FUNCTION__, cmd); 
   }

#ifdef DMP_DEVICE2_CERTIFICATES_1
   rutCert_deleteCertificate_dev2(certCfg);
#endif

   return CMSRET_SUCCESS;
}

//  compare two files
UBOOL8 rutCert_compareFile(char *fn1, char *fn2)
{
   char buf1[BUFLEN_264];
   char buf2[BUFLEN_264];
   UBOOL8 ret = TRUE;
    
   FILE *fp1 = fopen(fn1, "r");
   if (fp1 == NULL) 
   {
      printf("no file 1 = %s\n", buf1);
      ret = FALSE;
      return (ret);
   }
   FILE *fp2 = fopen(fn2, "r");
   if (fp2 == NULL) 
   {
      fclose(fp1);
      printf("no file 2 = %s\n", buf1);
      ret = FALSE;
      return (ret);
   }

   for(;!feof(fp1) || !feof(fp2);) 
   {
      int rc1 = fread(buf1, 1, sizeof(buf1), fp1);
      int rc2 = fread(buf2, 1, sizeof(buf2), fp2);
       
      if (rc1 != rc2) 
      {
         ret = FALSE;
         break;
      }
      if (strncmp(buf1, buf2, rc1) != 0) 
      {
         ret = FALSE;
         break;
      }
   }

   fclose(fp1);
   fclose(fp2);
    
   return (ret);
}

// Verify issued certificate against request
CmsRet rutCert_verifyCertReq(CertificateCfgObject *certCfg) 
{
   CmsRet ret = CMSRET_SUCCESS;
   char certTmpName[BUFLEN_256], certn[BUFLEN_264+BUFLEN_32], reqn[BUFLEN_256+BUFLEN_32];
   char cmd[BUFLEN_1024], outn1[BUFLEN_256+BUFLEN_32], outn2[BUFLEN_256+BUFLEN_32];
   char sslApp[BUFLEN_40];

   makePathToCert(certTmpName, BUFLEN_264, CERT_TEMP_NAME);
   makePathToSslApp(sslApp, BUFLEN_40);

   sprintf(certn, "%s.cert", certTmpName);
   FILE *fcert = fopen(certn, "w");
   fprintf(fcert, "%s", certCfg->content);
   fclose(fcert);

   sprintf(reqn, "%s.req", certTmpName);
   FILE *freq = fopen(reqn, "w");
   fprintf(freq, "%s", certCfg->reqPub);
   fclose(freq);

   sprintf(outn1, "%s.out1", certTmpName);
   sprintf(outn2, "%s.out2", certTmpName);
   
   sprintf(cmd, "%s x509 -pubkey -in %s -noout > %s", sslApp, certn, outn1);
   rut_doSystemAction(__FUNCTION__, cmd);
   sprintf(cmd, "%s req -pubkey -in %s  -noout > %s", sslApp, reqn, outn2);
   rut_doSystemAction(__FUNCTION__, cmd);

   if (rutCert_compareFile(outn1, outn2) == FALSE) 
   {
      ret = CMSRET_INVALID_CERT_REQ; 
   }
   
   sprintf(cmd, "rm %s 2> /dev/null", certn);
   rut_doSystemAction(__FUNCTION__, cmd);
   sprintf(cmd, "rm %s 2> /dev/null", reqn);
   rut_doSystemAction(__FUNCTION__, cmd);
   sprintf(cmd, "rm %s 2> /dev/null", outn1);
   rut_doSystemAction(__FUNCTION__, cmd);
   sprintf(cmd, "rm %s 2> /dev/null", outn2);
   rut_doSystemAction(__FUNCTION__, cmd);
     
   return (ret);
}

// Get subject of certificate
CmsRet rutCert_retrieveSubject(CertificateCfgObject *certCfg) 
{
   CmsRet ret = CMSRET_SUCCESS;
   char certTmpName[BUFLEN_264], certn[BUFLEN_264+BUFLEN_32];
   char cmd[BUFLEN_1024], subject[BUFLEN_264], outn[BUFLEN_264+BUFLEN_32];
   char sslApp[BUFLEN_40], subhead[] = "subject=";
   FILE *fcert = NULL;

   makePathToCert(certTmpName, BUFLEN_264, CERT_TEMP_NAME);
   makePathToSslApp(sslApp, BUFLEN_40);

   sprintf(outn, "%s.out", certTmpName);
   sprintf(certn, "%s.cert", certTmpName);
   if (cmsUtl_strcmp(certCfg->type, CERT_TYPE_SIGNING_REQ) == 0) 
   {
       fcert = fopen(certn, "w");       
       fprintf(fcert, "%s", certCfg->reqPub);
       fclose(fcert);
       sprintf(cmd, "%s req -subject -in %s -noout > %s 2>/dev/null", sslApp, certn, outn);
       rut_doSystemAction(__FUNCTION__, cmd);
   }
   else 
   {
       fcert = fopen(certn, "w");       
       fprintf(fcert, "%s", certCfg->content);
       fclose(fcert);
       sprintf(cmd, "%s x509 -subject -in %s -noout > %s 2>/dev/null", sslApp, certn, outn);
       rut_doSystemAction(__FUNCTION__, cmd);
   }
   
   FILE *fp = fopen(outn, "r");
   if (fp == NULL) 
   {
       CMSMEM_REPLACE_STRING_FLAGS(certCfg->subject, "", mdmLibCtx.allocFlags);
       ret = CMSRET_OPEN_FILE_ERROR;
   }
   else 
   {
      int rc = fread(subject, 1, BUFLEN_264, fp);
      subject[rc] = 0;
      fclose(fp);
      char *sp = cmsUtl_strstr(subject, subhead);
      if (sp != NULL) 
      {
         CMSMEM_REPLACE_STRING_FLAGS(certCfg->subject, sp+strlen(subhead), mdmLibCtx.allocFlags);
      }
      else 
      {
         CMSMEM_REPLACE_STRING_FLAGS(certCfg->subject, "", mdmLibCtx.allocFlags);
         ret = CMSRET_INVALID_CERT_SUBJECT;
      }
   }

   sprintf(cmd, "rm %s 2> /dev/null", certn);
   rut_doSystemAction(__FUNCTION__, cmd);
   sprintf(cmd, "rm %s 2> /dev/null", outn);
   rut_doSystemAction(__FUNCTION__, cmd);

   return (ret);
}

// Get serial number, signature algorithm,
// issuer, not before, not after, and
// subject of certificate
CmsRet rutCert_retrieveInfo
   (const CertificateCfgObject *certCfg,
    UINT32 numberOfArgs,
    char *serialNumber,
    char *signature,
    char *issuer,
    char *notBefore,
    char *notAfter,
    char *subject) 
{
   CmsRet ret = CMSRET_SUCCESS;
   UBOOL8 serialNumberLine = FALSE;
   UINT32 count = 0;
   char certTmpName[BUFLEN_264], certn[BUFLEN_264+BUFLEN_32];
   char cmd[BUFLEN_1024], outn[BUFLEN_264+BUFLEN_32];
   char sslApp[BUFLEN_40], line[BUFLEN_264];
   char *cp = NULL, *cp2 = NULL;
   FILE *fcert = NULL;

   if (certCfg == NULL)
   {
      return ret;
   }

   if (certCfg->type == NULL || certCfg->content == NULL || certCfg->name == NULL)
   {
      return ret;
   }

   // cannot retrieve all info for signed request certificate
   if (cmsUtl_strcmp(certCfg->type, CERT_TYPE_SIGNING_REQ) == 0)
   {
      cmsLog_error("Signed Request certificate does not have issuer information");
      return CMSRET_INVALID_ARGUMENTS;
   }

   makePathToCert(certTmpName, BUFLEN_264, CERT_TEMP_NAME);
   makePathToSslApp(sslApp, BUFLEN_40);

   sprintf(outn, "%s.out", certTmpName);
   sprintf(certn, "%s.cert", certTmpName);

   fcert = fopen(certn, "w");       
   fprintf(fcert, "%s", certCfg->content);
   fclose(fcert);
   sprintf(cmd, "%s x509 -text -in %s -noout > %s 2>/dev/null", sslApp, certn, outn);
   rut_doSystemAction(__FUNCTION__, cmd);
   
   FILE *fp = fopen(outn, "r");
   if (fp == NULL) 
   {
      ret = CMSRET_OPEN_FILE_ERROR;
   }
   else 
   {
      while ( count < numberOfArgs &&
              fgets(line, sizeof(line), fp) )
      {
         // line should has at least newline
         // at the end of line --> remove it
         line[strlen(line) - 1] = '\0';

         if (serialNumberLine == TRUE)
         {
            serialNumberLine = FALSE;
            // left trim
            for (cp = line; isspace(*cp); cp++)
               ;
            // advance until space characters
            for (cp2 = cp; !isspace(*cp2); cp2++)
               ;
            cmsUtl_strncpy(serialNumber, cp, cp2-cp+1);
            count++;
         }
         else if ((cp = cmsUtl_strstr(line, "Serial Number:")) != NULL)
         {
            cp += strlen("Serial Number:");
            if (*cp == '\0')
            {
               // setup flag to read serial number on next line
               serialNumberLine = TRUE;
            }
            else
            {
               // +1 to get serial number after space character
               cmsUtl_strcpy(serialNumber, cp+1);
               count++;
            }
         }
         else if ((cp = cmsUtl_strstr(line, "Signature Algorithm: ")) != NULL)
         {
            cmsUtl_strcpy(signature, cp+strlen("Signature Algorithm: "));
            count++;
         }
         else if ((cp = cmsUtl_strstr(line, "Issuer: ")) != NULL)
         {
            cmsUtl_strcpy(issuer, cp+strlen("Issuer: "));
            count++;
         }
         else if ((cp = cmsUtl_strstr(line, "Not Before: ")) != NULL)
         {
            cmsUtl_strcpy(notBefore, cp+strlen("Not Before: "));
            count++;
         }
         else if ((cp = cmsUtl_strstr(line, "Not After: ")) != NULL)
         {
            cmsUtl_strcpy(notAfter, cp+strlen("Not After: "));
            count++;
         }
         else if ((cp = cmsUtl_strstr(line, "Subject: ")) != NULL)
         {
            cmsUtl_strcpy(subject, cp+strlen("Subject: "));
            count++;
         }
      }

      fclose(fp);
   }

   sprintf(cmd, "rm %s 2> /dev/null", certn);
   rut_doSystemAction(__FUNCTION__, cmd);
   sprintf(cmd, "rm %s 2> /dev/null", outn);
   rut_doSystemAction(__FUNCTION__, cmd);

   return (ret);
}

// Process and verify imported certificate and private key
CmsRet rutCert_processImportedCert(CertificateCfgObject *certCfg) 
{
   CmsRet ret = CMSRET_SUCCESS;

   if (cmsUtl_strcmp(certCfg->type, CERT_TYPE_CA) !=0) 
   {
      char certTmpName[BUFLEN_264], privkeyn[BUFLEN_264+BUFLEN_32];
      char cmd[BUFLEN_1024], outn[BUFLEN_264+BUFLEN_32], buf[BUFLEN_40];
      char sslApp[BUFLEN_40];
      FILE *fprivkey = NULL, *fp = NULL;

      makePathToCert(certTmpName, BUFLEN_264, CERT_TEMP_NAME);
      makePathToSslApp(sslApp, BUFLEN_40);

      sprintf(privkeyn, "%s.cert", certTmpName);
      fprivkey = fopen(privkeyn, "w");
      fprintf(fprivkey, "%s", certCfg->privKey);
      fclose(fprivkey);

      sprintf(outn, "%s.out", certTmpName);
      sprintf(cmd, "%s rsa -check -in %s -noout -out %s", sslApp, privkeyn, outn);
      rut_doSystemAction(__FUNCTION__, cmd);

      fp = fopen(outn, "r");
      if (fp == NULL) 
      {
         ret = CMSRET_OPEN_FILE_ERROR;
      }
      else 
      {
         int rc = fread(buf, 1, sizeof(buf) -1, fp);
         buf[rc] = 0;
         if (!cmsUtl_strstr(buf, "key ok")) 
         {
            ret = CMSRET_INTERNAL_ERROR;
         }
         fclose(fp);
      }

      sprintf(cmd, "rm %s 2> /dev/null", outn);
      rut_doSystemAction(__FUNCTION__, cmd);
      sprintf(cmd, "rm %s 2> /dev/null", privkeyn);
      rut_doSystemAction(__FUNCTION__, cmd);
   }   

   ret = rutCert_retrieveSubject(certCfg);
     
   return (ret);
}


/** Figure out the path to create certificate files
 *
 * @param buf         (OUT) the file name includes the cert path and cert file
 * @param bufLen    (IN) size of buf
 * @param buf         (IN) the name of cert file
 *
 */
void makePathToCert(char *buf, UINT32 bufLen, const char *certFile)
{
   CmsRet ret = CMSRET_SUCCESS;

   if ((ret = cmsUtl_getRunTimePath("/var/cert/", buf, bufLen)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not get root dir, ret=%d", ret);
      return;
   }

   // if /var/cert/ directory does not exist then create it
   if (access(buf,  F_OK) !=  0)
   {
      char cmd[CMS_MAX_FULLPATH_LENGTH];
      SINT32 rc;
      rc = snprintf(cmd, sizeof(cmd), "mkdir -p %s 2> /dev/null", buf);
      if (rc >= (SINT32) sizeof(cmd))
      {
         cmsLog_error("cmd buf is too small for mkdir cmd (%s)", cmd);
         return;
      }
      rut_doSystemAction(__FUNCTION__, cmd);
   }

   ret = cmsUtl_strncat(buf, bufLen, certFile);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("buf too small (bufLen=%d) for certFile %s",
                   bufLen, certFile);
      return;
   }

   cmsLog_debug("buf=%s", buf);
}



/** Figure out the path to openssl application
 *
 * @param buf         (OUT) the openssl file name includes the path
 * @param bufLen    (IN) size of buf
 *
 */
CmsRet makePathToSslApp(char *buf, UINT32 bufLen)
{
   CmsRet ret = CMSRET_INTERNAL_ERROR;

   if (buf == NULL) return ret;

#ifdef DESKTOP_LINUX
   if (bufLen > strlen("openssl"))
   {
      strcpy(buf, "openssl");
      ret = CMSRET_SUCCESS;
   }
   else
   {
      buf[0] = '\0';
   }
#else
   if (bufLen > strlen("/bin/openssl"))
   {
      strcpy(buf, "/bin/openssl");
      ret = CMSRET_SUCCESS;
   }
   else
   {
      buf[0] = '\0';
   }
#endif

   return ret;
}


#endif /* SUPPORT_CERT */

