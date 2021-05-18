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

#ifndef __RUT_SYSTEM_H__
#define __RUT_SYSTEM_H__


/*!\file rut_system.h
 * \brief System level interface functions for various system functionality.
 *
 * The functions in this file should only be called by
 * RCL, STL, and other RUT functions.
 */

#include "cms.h"
#include "cms_core.h"

#define MAX_DEVICE_CONFIG_CONFIGFILE_SIZE      32768  /* (32K) */
#define MAX_DEVICE_CONFIG_PERSISTENT_DATA_SIZE 256
#define DEFAULT_CONFIG_FILE_NAME               "ConfigFile"
#define DEVICE_PERSISTENT_DATA_TOKEN           "deviceData"
#define BBF_DATA_MODEL_FILE                    "/webs/data-model/bbf-data-model-2.xml"

/** Write out the /etc/passwd file.
 * 
 * Don't know if any management entities actually use it to authenticate
 * login.  It would be nice if we didn't even have to write out a file
 * (saves memory).  Instead, management entities can just send an
 * "AUTHENTICATE" message to the smd and let smd look up the password
 * in the MDM.  Oh, well, a small optimization for later.
 * 
 * @param cp_admin   (IN) clear text password for admin account.
 * @param cp_support (IN) clear text password for support account.
 * @param cp_user    (IN) clear text password for user account.
 * @return CmsRet enum.
 */
CmsRet rut_createLoginCfg(const char *cp_admin,
                          const char *cp_support,
                          const char *cp_user);


/** Return true if localloglevel, remoteLogLevel, serverIpAddress, or serverPortNumber
 *  has changed.
 *
 * @param newObj   (IN) new syslogCfgObject.
 * @param currObj  (IN) current syslogCfgObject.
 *
 * @return True if any of localloglevel, remoteLogLevel, serverIpAddress,
 *         or serverPortNumber has changed.
 */
UBOOL8 rut_isSyslogCfgChanged(const _SyslogCfgObject *newObj,
                            const _SyslogCfgObject *currObj);


/** Stop syslogd and klogd and restart them again with the new parameters.
 *
 * @param syslogCfg  (IN) the current syslogCfgObject.
 *
 * @return CmsRet enum.
 */
CmsRet rut_restartsysklogd(const _SyslogCfgObject *syslogCfg);


/** Get the syslog log file.
 *
 * @param logLen (OUT) Number of bytes in log.
 *
 * @return pointer to buffer containing the log.  The caller is responsible
 *         for freeing the buffer.
 */
char *rutSys_getDeviceLog(UINT16 *logLen);


/** Update the log level for the specified application.
 *
 */
void rut_updateLogLevel(CmsEntityId destEid, const char *logLevel);


/** Update the log destination for the specified application.
 *
 */
void rut_updateLogDestination(CmsEntityId destEid, const char *logDest);


/** Update the log SOAP with source ConfigId for the specified application (mainly for tr69c).
 *
 */
void rut_updateLogSOAP(CmsEntityId destEid, const char *ConfigId);

/** Get status and MAC address of an interface.
 *
 */
/* notUsed void rut_getIntfStatus(char *devName, char *statusStr, char *hwAddr);
*/

/* get interface statistics: Ethernet, USB, WLAN based on device name *devName. */
void rut_getIntfStats(const char *devName, 
                      UINT32 *byteRx, UINT32 *packetRx, 
                      UINT32 *byteMultiRx, UINT32 *packetMulitRx, UINT32 *packetUniRx, UINT32 *packetBcastRx,
                      UINT32 *errRx, UINT32 *dropRx,
                      UINT32 *byteTx, UINT32 *packetTx, 
                      UINT32 *byteMultiTx, UINT32 *packetMulitTx, UINT32 *packetUniTx, UINT32 *packetBcastTx,
                      UINT32 *errTx, UINT32 *dropTx);

void rut_getIntfStats_uint64(const char *devName, 
                             UINT64 *byteRx, UINT64 *packetRx, 
                             UINT64 *byteMultiRx, UINT64 *packetMultiRx,
                             UINT64 *packetUniRx, UINT64 *packetBcastRx,
                             UINT64 *errRx, UINT64 *dropRx,
                             UINT64 *byteTx, UINT64 *packetTx, 
                             UINT64 *byteMultiTx, UINT64 *packetMultiTx,
                             UINT64 *packetUniTx, UINT64 *packetBcastTx,
                             UINT64 *errTx, UINT64 *dropTx);

void rut_clearIntfStats(const char *devname);


void rut_clearWanIntfStats(char *devname);


/** Create the /var/hosts file.
 *
 * Writes out the loopback 127.0.0.1 entry and all hosts seen on the LAN side.
 * If dnsproxy is enabled, also writes out the hostname.domain name entry.
 * Currently, this function only gets called if dnsproxy is enabled.
 */
void rutSys_createHostsFile(void);

char* rutSys_getDevicePersistentData(void);
char* rutSys_getRunningConfigFile(void);
void rutSys_setDevicePersistentData(char *data, int dataLen);
CmsRet rutSys_setRunningConfigFile(char *configBuf, int bufLen);

/** Return the number of CPU threads on system.
 *
 * @returns At least 1.
 */
UINT32 rutSys_getNumCpuThreads(void);

/** Get frequency and architecture information of the given processor ID.
 *
 * @param id           (IN)  The processor ID.
 * @param frequence    (OUT) The processor frequency.
 * @param architecture (OUT) The processor architecture as bufer with length is BUFLEN_32.
 * 
 * Return CmsRet
 */
CmsRet rutSys_getCpuInfo(UINT32 id, UINT32 *frequency, char *architecture);

/** Get value of uuid attribute in BBF data model XML file.
 *  BBF_DATA_MODEL_FILE is at /webs/data-model/bbf-data-model-2.xml
 *
 * @param len    (IN)  Length of uuid.
 * @param uuid   (OUT) Value of uuid.
 * 
 * Return CmsRet
 */
CmsRet rutSys_getUuidFromBbfDataModel(UINT32 len, char *uuid);

/** Get CPE features from DMP_xyz that are defined in make.common.
 *
 * @param len        (IN)  Length of features.
 * @param features   (OUT) Value of features.
 * 
 * Return CmsRet
 */
CmsRet rutSys_getFeaturesFromDataModel(UINT32 len, char *features);

#endif /* __RUT_SYSTEM_H__  */
