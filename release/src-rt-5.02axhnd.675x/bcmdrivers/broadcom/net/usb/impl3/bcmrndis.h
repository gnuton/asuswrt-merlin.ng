//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
// Use of this source code is subject to the terms of the Microsoft end-user
// license agreement (EULA) under which you licensed this SOFTWARE PRODUCT.
// If you did not accept the terms of the EULA, you are not authorized to use
// this source code. For a copy of the EULA, please see the LICENSE.RTF on your
// install media.
//
/***************************************************************************


Module Name:

    BCMRNDIS.H

Abstract:

    This module defines the Remote NDIS message structures.

Environment:

    kernel mode only

Notes:

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.



Revision History:

    2/8/99 : created

Authors:

    Arvind Murching and Tom Green

    
****************************************************************************/


#ifndef _RNDIS_H_
#define _RNDIS_H_

//
//  Basic types
//
typedef UINT32                                  RNDIS_REQUEST_ID;
typedef UINT32                                  RNDIS_HANDLE;
typedef UINT32                                  RNDIS_STATUS;
typedef UINT32                                  RNDIS_REQUEST_TYPE;
typedef UINT32                                  RNDIS_OID;
typedef UINT32                                  RNDIS_CLASS_ID;
typedef UINT32                                  RNDIS_MEDIUM;
typedef UINT32                                  *PRNDIS_REQUEST_ID;
typedef UINT32                                  *PRNDIS_HANDLE;
typedef UINT32                                  *PRNDIS_STATUS;
typedef UINT32                                  *PRNDIS_REQUEST_TYPE;
typedef UINT32                                  *PRNDIS_OID;
typedef UINT32                                  *PRNDIS_CLASS_ID;
typedef UINT32                                  *PRNDIS_MEDIUM;
typedef UINT32                                  RNDIS_AF;

//
//  Status codes
//

#ifndef STATUS_SUCCESS
#define STATUS_SUCCESS                          (0x00000000L)
#endif

#ifndef STATUS_UNSUCCESSFUL
#define STATUS_UNSUCCESSFUL                     (0xC0000001L)
#endif

#ifndef STATUS_PENDING
#define STATUS_PENDING                          (0x00000103L)
#endif

#ifndef STATUS_INSUFFICIENT_RESOURCES
#define STATUS_INSUFFICIENT_RESOURCES           (0xC000009AL)
#endif

#ifndef STATUS_BUFFER_OVERFLOW
#define STATUS_BUFFER_OVERFLOW                  (0x80000005L)
#endif

#ifndef STATUS_NOT_SUPPORTED
#define STATUS_NOT_SUPPORTED                    (0xC00000BBL)
#endif

#define RNDIS_STATUS_SUCCESS                    ((RNDIS_STATUS)STATUS_SUCCESS)
#define RNDIS_STATUS_PENDING                    ((RNDIS_STATUS)STATUS_PENDING)
#define RNDIS_STATUS_NOT_RECOGNIZED             ((RNDIS_STATUS)0x00010001L)
#define RNDIS_STATUS_NOT_COPIED                 ((RNDIS_STATUS)0x00010002L)
#define RNDIS_STATUS_NOT_ACCEPTED               ((RNDIS_STATUS)0x00010003L)
#define RNDIS_STATUS_CALL_ACTIVE                ((RNDIS_STATUS)0x00010007L)

#define RNDIS_STATUS_ONLINE                     ((RNDIS_STATUS)0x40010003L)
#define RNDIS_STATUS_RESET_START                ((RNDIS_STATUS)0x40010004L)
#define RNDIS_STATUS_RESET_END                  ((RNDIS_STATUS)0x40010005L)
#define RNDIS_STATUS_RING_STATUS                ((RNDIS_STATUS)0x40010006L)
#define RNDIS_STATUS_CLOSED                     ((RNDIS_STATUS)0x40010007L)
#define RNDIS_STATUS_WAN_LINE_UP                ((RNDIS_STATUS)0x40010008L)
#define RNDIS_STATUS_WAN_LINE_DOWN              ((RNDIS_STATUS)0x40010009L)
#define RNDIS_STATUS_WAN_FRAGMENT               ((RNDIS_STATUS)0x4001000AL)
#define RNDIS_STATUS_MEDIA_CONNECT              ((RNDIS_STATUS)0x4001000BL)
#define RNDIS_STATUS_MEDIA_DISCONNECT           ((RNDIS_STATUS)0x4001000CL)
#define RNDIS_STATUS_HARDWARE_LINE_UP           ((RNDIS_STATUS)0x4001000DL)
#define RNDIS_STATUS_HARDWARE_LINE_DOWN         ((RNDIS_STATUS)0x4001000EL)
#define RNDIS_STATUS_INTERFACE_UP               ((RNDIS_STATUS)0x4001000FL)
#define RNDIS_STATUS_INTERFACE_DOWN             ((RNDIS_STATUS)0x40010010L)
#define RNDIS_STATUS_MEDIA_BUSY                 ((RNDIS_STATUS)0x40010011L)
#define RNDIS_STATUS_MEDIA_SPECIFIC_INDICATION  ((RNDIS_STATUS)0x40010012L)
#define RNDIS_STATUS_WW_INDICATION              RNDIS_STATUS_MEDIA_SPECIFIC_INDICATION
#define RNDIS_STATUS_LINK_SPEED_CHANGE          ((RNDIS_STATUS)0x40010013L)

#define RNDIS_STATUS_NOT_RESETTABLE             ((RNDIS_STATUS)0x80010001L)
#define RNDIS_STATUS_SOFT_ERRORS                ((RNDIS_STATUS)0x80010003L)
#define RNDIS_STATUS_HARD_ERRORS                ((RNDIS_STATUS)0x80010004L)
#define RNDIS_STATUS_BUFFER_OVERFLOW            ((RNDIS_STATUS)STATUS_BUFFER_OVERFLOW)

#define RNDIS_STATUS_FAILURE                    ((RNDIS_STATUS)STATUS_UNSUCCESSFUL)
#define RNDIS_STATUS_RESOURCES                  ((RNDIS_STATUS)STATUS_INSUFFICIENT_RESOURCES)
#define RNDIS_STATUS_CLOSING                    ((RNDIS_STATUS)0xC0010002L)
#define RNDIS_STATUS_BAD_VERSION                ((RNDIS_STATUS)0xC0010004L)
#define RNDIS_STATUS_BAD_CHARACTERISTICS        ((RNDIS_STATUS)0xC0010005L)
#define RNDIS_STATUS_ADAPTER_NOT_FOUND          ((RNDIS_STATUS)0xC0010006L)
#define RNDIS_STATUS_OPEN_FAILED                ((RNDIS_STATUS)0xC0010007L)
#define RNDIS_STATUS_DEVICE_FAILED              ((RNDIS_STATUS)0xC0010008L)
#define RNDIS_STATUS_MULTICAST_FULL             ((RNDIS_STATUS)0xC0010009L)
#define RNDIS_STATUS_MULTICAST_EXISTS           ((RNDIS_STATUS)0xC001000AL)
#define RNDIS_STATUS_MULTICAST_NOT_FOUND        ((RNDIS_STATUS)0xC001000BL)
#define RNDIS_STATUS_REQUEST_ABORTED            ((RNDIS_STATUS)0xC001000CL)
#define RNDIS_STATUS_RESET_IN_PROGRESS          ((RNDIS_STATUS)0xC001000DL)
#define RNDIS_STATUS_CLOSING_INDICATING         ((RNDIS_STATUS)0xC001000EL)
#define RNDIS_STATUS_NOT_SUPPORTED              ((RNDIS_STATUS)STATUS_NOT_SUPPORTED)
#define RNDIS_STATUS_INVALID_PACKET             ((RNDIS_STATUS)0xC001000FL)
#define RNDIS_STATUS_OPEN_LIST_FULL             ((RNDIS_STATUS)0xC0010010L)
#define RNDIS_STATUS_ADAPTER_NOT_READY          ((RNDIS_STATUS)0xC0010011L)
#define RNDIS_STATUS_ADAPTER_NOT_OPEN           ((RNDIS_STATUS)0xC0010012L)
#define RNDIS_STATUS_NOT_INDICATING             ((RNDIS_STATUS)0xC0010013L)
#define RNDIS_STATUS_INVALID_LENGTH             ((RNDIS_STATUS)0xC0010014L)
#define RNDIS_STATUS_INVALID_DATA               ((RNDIS_STATUS)0xC0010015L)
#define RNDIS_STATUS_BUFFER_TOO_SHORT           ((RNDIS_STATUS)0xC0010016L)
#define RNDIS_STATUS_INVALID_OID                ((RNDIS_STATUS)0xC0010017L)
#define RNDIS_STATUS_ADAPTER_REMOVED            ((RNDIS_STATUS)0xC0010018L)
#define RNDIS_STATUS_UNSUPPORTED_MEDIA          ((RNDIS_STATUS)0xC0010019L)
#define RNDIS_STATUS_GROUP_ADDRESS_IN_USE       ((RNDIS_STATUS)0xC001001AL)
#define RNDIS_STATUS_FILE_NOT_FOUND             ((RNDIS_STATUS)0xC001001BL)
#define RNDIS_STATUS_ERROR_READING_FILE         ((RNDIS_STATUS)0xC001001CL)
#define RNDIS_STATUS_ALREADY_MAPPED             ((RNDIS_STATUS)0xC001001DL)
#define RNDIS_STATUS_RESOURCE_CONFLICT          ((RNDIS_STATUS)0xC001001EL)
#define RNDIS_STATUS_NO_CABLE                   ((RNDIS_STATUS)0xC001001FL)

#define RNDIS_STATUS_INVALID_SAP                ((RNDIS_STATUS)0xC0010020L)
#define RNDIS_STATUS_SAP_IN_USE                 ((RNDIS_STATUS)0xC0010021L)
#define RNDIS_STATUS_INVALID_ADDRESS            ((RNDIS_STATUS)0xC0010022L)
#define RNDIS_STATUS_VC_NOT_ACTIVATED           ((RNDIS_STATUS)0xC0010023L)
#define RNDIS_STATUS_DEST_OUT_OF_ORDER          ((RNDIS_STATUS)0xC0010024L)
#define RNDIS_STATUS_VC_NOT_AVAILABLE           ((RNDIS_STATUS)0xC0010025L)
#define RNDIS_STATUS_CELLRATE_NOT_AVAILABLE     ((RNDIS_STATUS)0xC0010026L)
#define RNDIS_STATUS_INCOMPATABLE_QOS           ((RNDIS_STATUS)0xC0010027L)
#define RNDIS_STATUS_AAL_PARAMS_UNSUPPORTED     ((RNDIS_STATUS)0xC0010028L)
#define RNDIS_STATUS_NO_ROUTE_TO_DESTINATION    ((RNDIS_STATUS)0xC0010029L)

#define RNDIS_STATUS_TOKEN_RING_OPEN_ERROR      ((RNDIS_STATUS)0xC0011000L)


//
// Object Identifiers used by NdisRequest Query/Set Information
//

//
// General Objects
//

#define RNDIS_OID_GEN_SUPPORTED_LIST                    0x00010101
#define RNDIS_OID_GEN_HARDWARE_STATUS                   0x00010102
#define RNDIS_OID_GEN_MEDIA_SUPPORTED                   0x00010103
#define RNDIS_OID_GEN_MEDIA_IN_USE                      0x00010104
#define RNDIS_OID_GEN_MAXIMUM_LOOKAHEAD                 0x00010105
#define RNDIS_OID_GEN_MAXIMUM_FRAME_SIZE                0x00010106
#define RNDIS_OID_GEN_LINK_SPEED                        0x00010107
#define RNDIS_OID_GEN_TRANSMIT_BUFFER_SPACE             0x00010108
#define RNDIS_OID_GEN_RECEIVE_BUFFER_SPACE              0x00010109
#define RNDIS_OID_GEN_TRANSMIT_BLOCK_SIZE               0x0001010A
#define RNDIS_OID_GEN_RECEIVE_BLOCK_SIZE                0x0001010B
#define RNDIS_OID_GEN_VENDOR_ID                         0x0001010C
#define RNDIS_OID_GEN_VENDOR_DESCRIPTION                0x0001010D
#define RNDIS_OID_GEN_CURRENT_PACKET_FILTER             0x0001010E
#define RNDIS_OID_GEN_CURRENT_LOOKAHEAD                 0x0001010F
#define RNDIS_OID_GEN_DRIVER_VERSION                    0x00010110
#define RNDIS_OID_GEN_MAXIMUM_TOTAL_SIZE                0x00010111
#define RNDIS_OID_GEN_PROTOCOL_OPTIONS                  0x00010112
#define RNDIS_OID_GEN_MAC_OPTIONS                       0x00010113
#define RNDIS_OID_GEN_MEDIA_CONNECT_STATUS              0x00010114
#define RNDIS_OID_GEN_MAXIMUM_SEND_PACKETS              0x00010115
#define RNDIS_OID_GEN_VENDOR_DRIVER_VERSION             0x00010116

#define RNDIS_OID_GEN_XMIT_OK                           0x00020101
#define RNDIS_OID_GEN_RCV_OK                            0x00020102
#define RNDIS_OID_GEN_XMIT_ERROR                        0x00020103
#define RNDIS_OID_GEN_RCV_ERROR                         0x00020104
#define RNDIS_OID_GEN_RCV_NO_BUFFER                     0x00020105

#define RNDIS_OID_GEN_DIRECTED_BYTES_XMIT               0x00020201
#define RNDIS_OID_GEN_DIRECTED_FRAMES_XMIT              0x00020202
#define RNDIS_OID_GEN_MULTICAST_BYTES_XMIT              0x00020203
#define RNDIS_OID_GEN_MULTICAST_FRAMES_XMIT             0x00020204
#define RNDIS_OID_GEN_BROADCAST_BYTES_XMIT              0x00020205
#define RNDIS_OID_GEN_BROADCAST_FRAMES_XMIT             0x00020206
#define RNDIS_OID_GEN_DIRECTED_BYTES_RCV                0x00020207
#define RNDIS_OID_GEN_DIRECTED_FRAMES_RCV               0x00020208
#define RNDIS_OID_GEN_MULTICAST_BYTES_RCV               0x00020209
#define RNDIS_OID_GEN_MULTICAST_FRAMES_RCV              0x0002020A
#define RNDIS_OID_GEN_BROADCAST_BYTES_RCV               0x0002020B
#define RNDIS_OID_GEN_BROADCAST_FRAMES_RCV              0x0002020C

#define RNDIS_OID_GEN_RCV_CRC_ERROR                     0x0002020D
#define RNDIS_OID_GEN_TRANSMIT_QUEUE_LENGTH             0x0002020E

#define RNDIS_OID_GEN_GET_TIME_CAPS                     0x0002020F
#define RNDIS_OID_GEN_GET_NETCARD_TIME                  0x00020210

//
// These are connection-oriented general OIDs.
// These replace the above OIDs for connection-oriented media.
//
#define RNDIS_OID_GEN_CO_SUPPORTED_LIST                 0x00010101
#define RNDIS_OID_GEN_CO_HARDWARE_STATUS                0x00010102
#define RNDIS_OID_GEN_CO_MEDIA_SUPPORTED                0x00010103
#define RNDIS_OID_GEN_CO_MEDIA_IN_USE                   0x00010104
#define RNDIS_OID_GEN_CO_LINK_SPEED                     0x00010105
#define RNDIS_OID_GEN_CO_VENDOR_ID                      0x00010106
#define RNDIS_OID_GEN_CO_VENDOR_DESCRIPTION             0x00010107
#define RNDIS_OID_GEN_CO_DRIVER_VERSION                 0x00010108
#define RNDIS_OID_GEN_CO_PROTOCOL_OPTIONS               0x00010109
#define RNDIS_OID_GEN_CO_MAC_OPTIONS                    0x0001010A
#define RNDIS_OID_GEN_CO_MEDIA_CONNECT_STATUS           0x0001010B
#define RNDIS_OID_GEN_CO_VENDOR_DRIVER_VERSION          0x0001010C
#define RNDIS_OID_GEN_CO_MINIMUM_LINK_SPEED             0x0001010D

#define RNDIS_OID_GEN_CO_GET_TIME_CAPS                  0x00010201
#define RNDIS_OID_GEN_CO_GET_NETCARD_TIME               0x00010202

//
// These are connection-oriented statistics OIDs.
//
#define RNDIS_OID_GEN_CO_XMIT_PDUS_OK                   0x00020101
#define RNDIS_OID_GEN_CO_RCV_PDUS_OK                    0x00020102
#define RNDIS_OID_GEN_CO_XMIT_PDUS_ERROR                0x00020103
#define RNDIS_OID_GEN_CO_RCV_PDUS_ERROR                 0x00020104
#define RNDIS_OID_GEN_CO_RCV_PDUS_NO_BUFFER             0x00020105


#define RNDIS_OID_GEN_CO_RCV_CRC_ERROR                  0x00020201
#define RNDIS_OID_GEN_CO_TRANSMIT_QUEUE_LENGTH          0x00020202
#define RNDIS_OID_GEN_CO_BYTES_XMIT                     0x00020203
#define RNDIS_OID_GEN_CO_BYTES_RCV                      0x00020204
#define RNDIS_OID_GEN_CO_BYTES_XMIT_OUTSTANDING         0x00020205
#define RNDIS_OID_GEN_CO_NETCARD_LOAD                   0x00020206

//
// These are objects for Connection-oriented media call-managers.
//
#define RNDIS_OID_CO_ADD_PVC                            0xFF000001
#define RNDIS_OID_CO_DELETE_PVC                         0xFF000002
#define RNDIS_OID_CO_GET_CALL_INFORMATION               0xFF000003
#define RNDIS_OID_CO_ADD_ADDRESS                        0xFF000004
#define RNDIS_OID_CO_DELETE_ADDRESS                     0xFF000005
#define RNDIS_OID_CO_GET_ADDRESSES                      0xFF000006
#define RNDIS_OID_CO_ADDRESS_CHANGE                     0xFF000007
#define RNDIS_OID_CO_SIGNALING_ENABLED                  0xFF000008
#define RNDIS_OID_CO_SIGNALING_DISABLED                 0xFF000009


//
// 802.3 Objects (Ethernet)
//

#define RNDIS_OID_802_3_PERMANENT_ADDRESS               0x01010101
#define RNDIS_OID_802_3_CURRENT_ADDRESS                 0x01010102
#define RNDIS_OID_802_3_MULTICAST_LIST                  0x01010103
#define RNDIS_OID_802_3_MAXIMUM_LIST_SIZE               0x01010104
#define RNDIS_OID_802_3_MAC_OPTIONS                     0x01010105

//
//
#define NDIS_802_3_MAC_OPTION_PRIORITY                  0x00000001

#define RNDIS_OID_802_3_RCV_ERROR_ALIGNMENT             0x01020101
#define RNDIS_OID_802_3_XMIT_ONE_COLLISION              0x01020102
#define RNDIS_OID_802_3_XMIT_MORE_COLLISIONS            0x01020103

#define RNDIS_OID_802_3_XMIT_DEFERRED                   0x01020201
#define RNDIS_OID_802_3_XMIT_MAX_COLLISIONS             0x01020202
#define RNDIS_OID_802_3_RCV_OVERRUN                     0x01020203
#define RNDIS_OID_802_3_XMIT_UNDERRUN                   0x01020204
#define RNDIS_OID_802_3_XMIT_HEARTBEAT_FAILURE          0x01020205
#define RNDIS_OID_802_3_XMIT_TIMES_CRS_LOST             0x01020206
#define RNDIS_OID_802_3_XMIT_LATE_COLLISIONS            0x01020207


//
// 802.5 Objects (Token-Ring)
//

#define RNDIS_OID_802_5_PERMANENT_ADDRESS               0x02010101
#define RNDIS_OID_802_5_CURRENT_ADDRESS                 0x02010102
#define RNDIS_OID_802_5_CURRENT_FUNCTIONAL              0x02010103
#define RNDIS_OID_802_5_CURRENT_GROUP                   0x02010104
#define RNDIS_OID_802_5_LAST_OPEN_STATUS                0x02010105
#define RNDIS_OID_802_5_CURRENT_RING_STATUS             0x02010106
#define RNDIS_OID_802_5_CURRENT_RING_STATE              0x02010107

#define RNDIS_OID_802_5_LINE_ERRORS                     0x02020101
#define RNDIS_OID_802_5_LOST_FRAMES                     0x02020102

#define RNDIS_OID_802_5_BURST_ERRORS                    0x02020201
#define RNDIS_OID_802_5_AC_ERRORS                       0x02020202
#define RNDIS_OID_802_5_ABORT_DELIMETERS                0x02020203
#define RNDIS_OID_802_5_FRAME_COPIED_ERRORS             0x02020204
#define RNDIS_OID_802_5_FREQUENCY_ERRORS                0x02020205
#define RNDIS_OID_802_5_TOKEN_ERRORS                    0x02020206
#define RNDIS_OID_802_5_INTERNAL_ERRORS                 0x02020207


//
// FDDI Objects
//

#define RNDIS_OID_FDDI_LONG_PERMANENT_ADDR              0x03010101
#define RNDIS_OID_FDDI_LONG_CURRENT_ADDR                0x03010102
#define RNDIS_OID_FDDI_LONG_MULTICAST_LIST              0x03010103
#define RNDIS_OID_FDDI_LONG_MAX_LIST_SIZE               0x03010104
#define RNDIS_OID_FDDI_SHORT_PERMANENT_ADDR             0x03010105
#define RNDIS_OID_FDDI_SHORT_CURRENT_ADDR               0x03010106
#define RNDIS_OID_FDDI_SHORT_MULTICAST_LIST             0x03010107
#define RNDIS_OID_FDDI_SHORT_MAX_LIST_SIZE              0x03010108

#define RNDIS_OID_FDDI_ATTACHMENT_TYPE                  0x03020101
#define RNDIS_OID_FDDI_UPSTREAM_NODE_LONG               0x03020102
#define RNDIS_OID_FDDI_DOWNSTREAM_NODE_LONG             0x03020103
#define RNDIS_OID_FDDI_FRAME_ERRORS                     0x03020104
#define RNDIS_OID_FDDI_FRAMES_LOST                      0x03020105
#define RNDIS_OID_FDDI_RING_MGT_STATE                   0x03020106
#define RNDIS_OID_FDDI_LCT_FAILURES                     0x03020107
#define RNDIS_OID_FDDI_LEM_REJECTS                      0x03020108
#define RNDIS_OID_FDDI_LCONNECTION_STATE                0x03020109

#define RNDIS_OID_FDDI_SMT_STATION_ID                   0x03030201
#define RNDIS_OID_FDDI_SMT_OP_VERSION_ID                0x03030202
#define RNDIS_OID_FDDI_SMT_HI_VERSION_ID                0x03030203
#define RNDIS_OID_FDDI_SMT_LO_VERSION_ID                0x03030204
#define RNDIS_OID_FDDI_SMT_MANUFACTURER_DATA            0x03030205
#define RNDIS_OID_FDDI_SMT_USER_DATA                    0x03030206
#define RNDIS_OID_FDDI_SMT_MIB_VERSION_ID               0x03030207
#define RNDIS_OID_FDDI_SMT_MAC_CT                       0x03030208
#define RNDIS_OID_FDDI_SMT_NON_MASTER_CT                0x03030209
#define RNDIS_OID_FDDI_SMT_MASTER_CT                    0x0303020A
#define RNDIS_OID_FDDI_SMT_AVAILABLE_PATHS              0x0303020B
#define RNDIS_OID_FDDI_SMT_CONFIG_CAPABILITIES          0x0303020C
#define RNDIS_OID_FDDI_SMT_CONFIG_POLICY                0x0303020D
#define RNDIS_OID_FDDI_SMT_CONNECTION_POLICY            0x0303020E
#define RNDIS_OID_FDDI_SMT_T_NOTIFY                     0x0303020F
#define RNDIS_OID_FDDI_SMT_STAT_RPT_POLICY              0x03030210
#define RNDIS_OID_FDDI_SMT_TRACE_MAX_EXPIRATION         0x03030211
#define RNDIS_OID_FDDI_SMT_PORT_INDEXES                 0x03030212
#define RNDIS_OID_FDDI_SMT_MAC_INDEXES                  0x03030213
#define RNDIS_OID_FDDI_SMT_BYPASS_PRESENT               0x03030214
#define RNDIS_OID_FDDI_SMT_ECM_STATE                    0x03030215
#define RNDIS_OID_FDDI_SMT_CF_STATE                     0x03030216
#define RNDIS_OID_FDDI_SMT_HOLD_STATE                   0x03030217
#define RNDIS_OID_FDDI_SMT_REMOTE_DISCONNECT_FLAG       0x03030218
#define RNDIS_OID_FDDI_SMT_STATION_STATUS               0x03030219
#define RNDIS_OID_FDDI_SMT_PEER_WRAP_FLAG               0x0303021A
#define RNDIS_OID_FDDI_SMT_MSG_TIME_STAMP               0x0303021B
#define RNDIS_OID_FDDI_SMT_TRANSITION_TIME_STAMP        0x0303021C
#define RNDIS_OID_FDDI_SMT_SET_COUNT                    0x0303021D
#define RNDIS_OID_FDDI_SMT_LAST_SET_STATION_ID          0x0303021E
#define RNDIS_OID_FDDI_MAC_FRAME_STATUS_FUNCTIONS       0x0303021F
#define RNDIS_OID_FDDI_MAC_BRIDGE_FUNCTIONS             0x03030220
#define RNDIS_OID_FDDI_MAC_T_MAX_CAPABILITY             0x03030221
#define RNDIS_OID_FDDI_MAC_TVX_CAPABILITY               0x03030222
#define RNDIS_OID_FDDI_MAC_AVAILABLE_PATHS              0x03030223
#define RNDIS_OID_FDDI_MAC_CURRENT_PATH                 0x03030224
#define RNDIS_OID_FDDI_MAC_UPSTREAM_NBR                 0x03030225
#define RNDIS_OID_FDDI_MAC_DOWNSTREAM_NBR               0x03030226
#define RNDIS_OID_FDDI_MAC_OLD_UPSTREAM_NBR             0x03030227
#define RNDIS_OID_FDDI_MAC_OLD_DOWNSTREAM_NBR           0x03030228
#define RNDIS_OID_FDDI_MAC_DUP_ADDRESS_TEST             0x03030229
#define RNDIS_OID_FDDI_MAC_REQUESTED_PATHS              0x0303022A
#define RNDIS_OID_FDDI_MAC_DOWNSTREAM_PORT_TYPE         0x0303022B
#define RNDIS_OID_FDDI_MAC_INDEX                        0x0303022C
#define RNDIS_OID_FDDI_MAC_SMT_ADDRESS                  0x0303022D
#define RNDIS_OID_FDDI_MAC_LONG_GRP_ADDRESS             0x0303022E
#define RNDIS_OID_FDDI_MAC_SHORT_GRP_ADDRESS            0x0303022F
#define RNDIS_OID_FDDI_MAC_T_REQ                        0x03030230
#define RNDIS_OID_FDDI_MAC_T_NEG                        0x03030231
#define RNDIS_OID_FDDI_MAC_T_MAX                        0x03030232
#define RNDIS_OID_FDDI_MAC_TVX_VALUE                    0x03030233
#define RNDIS_OID_FDDI_MAC_T_PRI0                       0x03030234
#define RNDIS_OID_FDDI_MAC_T_PRI1                       0x03030235
#define RNDIS_OID_FDDI_MAC_T_PRI2                       0x03030236
#define RNDIS_OID_FDDI_MAC_T_PRI3                       0x03030237
#define RNDIS_OID_FDDI_MAC_T_PRI4                       0x03030238
#define RNDIS_OID_FDDI_MAC_T_PRI5                       0x03030239
#define RNDIS_OID_FDDI_MAC_T_PRI6                       0x0303023A
#define RNDIS_OID_FDDI_MAC_FRAME_CT                     0x0303023B
#define RNDIS_OID_FDDI_MAC_COPIED_CT                    0x0303023C
#define RNDIS_OID_FDDI_MAC_TRANSMIT_CT                  0x0303023D
#define RNDIS_OID_FDDI_MAC_TOKEN_CT                     0x0303023E
#define RNDIS_OID_FDDI_MAC_ERROR_CT                     0x0303023F
#define RNDIS_OID_FDDI_MAC_LOST_CT                      0x03030240
#define RNDIS_OID_FDDI_MAC_TVX_EXPIRED_CT               0x03030241
#define RNDIS_OID_FDDI_MAC_NOT_COPIED_CT                0x03030242
#define RNDIS_OID_FDDI_MAC_LATE_CT                      0x03030243
#define RNDIS_OID_FDDI_MAC_RING_OP_CT                   0x03030244
#define RNDIS_OID_FDDI_MAC_FRAME_ERROR_THRESHOLD        0x03030245
#define RNDIS_OID_FDDI_MAC_FRAME_ERROR_RATIO            0x03030246
#define RNDIS_OID_FDDI_MAC_NOT_COPIED_THRESHOLD         0x03030247
#define RNDIS_OID_FDDI_MAC_NOT_COPIED_RATIO             0x03030248
#define RNDIS_OID_FDDI_MAC_RMT_STATE                    0x03030249
#define RNDIS_OID_FDDI_MAC_DA_FLAG                      0x0303024A
#define RNDIS_OID_FDDI_MAC_UNDA_FLAG                    0x0303024B
#define RNDIS_OID_FDDI_MAC_FRAME_ERROR_FLAG             0x0303024C
#define RNDIS_OID_FDDI_MAC_NOT_COPIED_FLAG              0x0303024D
#define RNDIS_OID_FDDI_MAC_MA_UNITDATA_AVAILABLE        0x0303024E
#define RNDIS_OID_FDDI_MAC_HARDWARE_PRESENT             0x0303024F
#define RNDIS_OID_FDDI_MAC_MA_UNITDATA_ENABLE           0x03030250
#define RNDIS_OID_FDDI_PATH_INDEX                       0x03030251
#define RNDIS_OID_FDDI_PATH_RING_LATENCY                0x03030252
#define RNDIS_OID_FDDI_PATH_TRACE_STATUS                0x03030253
#define RNDIS_OID_FDDI_PATH_SBA_PAYLOAD                 0x03030254
#define RNDIS_OID_FDDI_PATH_SBA_OVERHEAD                0x03030255
#define RNDIS_OID_FDDI_PATH_CONFIGURATION               0x03030256
#define RNDIS_OID_FDDI_PATH_T_R_MODE                    0x03030257
#define RNDIS_OID_FDDI_PATH_SBA_AVAILABLE               0x03030258
#define RNDIS_OID_FDDI_PATH_TVX_LOWER_BOUND             0x03030259
#define RNDIS_OID_FDDI_PATH_T_MAX_LOWER_BOUND           0x0303025A
#define RNDIS_OID_FDDI_PATH_MAX_T_REQ                   0x0303025B
#define RNDIS_OID_FDDI_PORT_MY_TYPE                     0x0303025C
#define RNDIS_OID_FDDI_PORT_NEIGHBOR_TYPE               0x0303025D
#define RNDIS_OID_FDDI_PORT_CONNECTION_POLICIES         0x0303025E
#define RNDIS_OID_FDDI_PORT_MAC_INDICATED               0x0303025F
#define RNDIS_OID_FDDI_PORT_CURRENT_PATH                0x03030260
#define RNDIS_OID_FDDI_PORT_REQUESTED_PATHS             0x03030261
#define RNDIS_OID_FDDI_PORT_MAC_PLACEMENT               0x03030262
#define RNDIS_OID_FDDI_PORT_AVAILABLE_PATHS             0x03030263
#define RNDIS_OID_FDDI_PORT_MAC_LOOP_TIME               0x03030264
#define RNDIS_OID_FDDI_PORT_PMD_CLASS                   0x03030265
#define RNDIS_OID_FDDI_PORT_CONNECTION_CAPABILITIES     0x03030266
#define RNDIS_OID_FDDI_PORT_INDEX                       0x03030267
#define RNDIS_OID_FDDI_PORT_MAINT_LS                    0x03030268
#define RNDIS_OID_FDDI_PORT_BS_FLAG                     0x03030269
#define RNDIS_OID_FDDI_PORT_PC_LS                       0x0303026A
#define RNDIS_OID_FDDI_PORT_EB_ERROR_CT                 0x0303026B
#define RNDIS_OID_FDDI_PORT_LCT_FAIL_CT                 0x0303026C
#define RNDIS_OID_FDDI_PORT_LER_ESTIMATE                0x0303026D
#define RNDIS_OID_FDDI_PORT_LEM_REJECT_CT               0x0303026E
#define RNDIS_OID_FDDI_PORT_LEM_CT                      0x0303026F
#define RNDIS_OID_FDDI_PORT_LER_CUTOFF                  0x03030270
#define RNDIS_OID_FDDI_PORT_LER_ALARM                   0x03030271
#define RNDIS_OID_FDDI_PORT_CONNNECT_STATE              0x03030272
#define RNDIS_OID_FDDI_PORT_PCM_STATE                   0x03030273
#define RNDIS_OID_FDDI_PORT_PC_WITHHOLD                 0x03030274
#define RNDIS_OID_FDDI_PORT_LER_FLAG                    0x03030275
#define RNDIS_OID_FDDI_PORT_HARDWARE_PRESENT            0x03030276
#define RNDIS_OID_FDDI_SMT_STATION_ACTION               0x03030277
#define RNDIS_OID_FDDI_PORT_ACTION                      0x03030278
#define RNDIS_OID_FDDI_IF_DESCR                         0x03030279
#define RNDIS_OID_FDDI_IF_TYPE                          0x0303027A
#define RNDIS_OID_FDDI_IF_MTU                           0x0303027B
#define RNDIS_OID_FDDI_IF_SPEED                         0x0303027C
#define RNDIS_OID_FDDI_IF_PHYS_ADDRESS                  0x0303027D
#define RNDIS_OID_FDDI_IF_ADMIN_STATUS                  0x0303027E
#define RNDIS_OID_FDDI_IF_OPER_STATUS                   0x0303027F
#define RNDIS_OID_FDDI_IF_LAST_CHANGE                   0x03030280
#define RNDIS_OID_FDDI_IF_IN_OCTETS                     0x03030281
#define RNDIS_OID_FDDI_IF_IN_UCAST_PKTS                 0x03030282
#define RNDIS_OID_FDDI_IF_IN_NUCAST_PKTS                0x03030283
#define RNDIS_OID_FDDI_IF_IN_DISCARDS                   0x03030284
#define RNDIS_OID_FDDI_IF_IN_ERRORS                     0x03030285
#define RNDIS_OID_FDDI_IF_IN_UNKNOWN_PROTOS             0x03030286
#define RNDIS_OID_FDDI_IF_OUT_OCTETS                    0x03030287
#define RNDIS_OID_FDDI_IF_OUT_UCAST_PKTS                0x03030288
#define RNDIS_OID_FDDI_IF_OUT_NUCAST_PKTS               0x03030289
#define RNDIS_OID_FDDI_IF_OUT_DISCARDS                  0x0303028A
#define RNDIS_OID_FDDI_IF_OUT_ERRORS                    0x0303028B
#define RNDIS_OID_FDDI_IF_OUT_QLEN                      0x0303028C
#define RNDIS_OID_FDDI_IF_SPECIFIC                      0x0303028D


//
// WAN objects
//

#define RNDIS_OID_WAN_PERMANENT_ADDRESS                 0x04010101
#define RNDIS_OID_WAN_CURRENT_ADDRESS                   0x04010102
#define RNDIS_OID_WAN_QUALITY_OF_SERVICE                0x04010103
#define RNDIS_OID_WAN_PROTOCOL_TYPE                     0x04010104
#define RNDIS_OID_WAN_MEDIUM_SUBTYPE                    0x04010105
#define RNDIS_OID_WAN_HEADER_FORMAT                     0x04010106

#define RNDIS_OID_WAN_GET_INFO                          0x04010107
#define RNDIS_OID_WAN_SET_LINK_INFO                     0x04010108
#define RNDIS_OID_WAN_GET_LINK_INFO                     0x04010109

#define RNDIS_OID_WAN_LINE_COUNT                        0x0401010A

#define RNDIS_OID_WAN_GET_BRIDGE_INFO                   0x0401020A
#define RNDIS_OID_WAN_SET_BRIDGE_INFO                   0x0401020B
#define RNDIS_OID_WAN_GET_COMP_INFO                     0x0401020C
#define RNDIS_OID_WAN_SET_COMP_INFO                     0x0401020D
#define RNDIS_OID_WAN_GET_STATS_INFO                    0x0401020E


//
// LocalTalk objects
//

#define RNDIS_OID_LTALK_CURRENT_NODE_ID                 0x05010102

#define RNDIS_OID_LTALK_IN_BROADCASTS                   0x05020101
#define RNDIS_OID_LTALK_IN_LENGTH_ERRORS                0x05020102

#define RNDIS_OID_LTALK_OUT_NO_HANDLERS                 0x05020201
#define RNDIS_OID_LTALK_COLLISIONS                      0x05020202
#define RNDIS_OID_LTALK_DEFERS                          0x05020203
#define RNDIS_OID_LTALK_NO_DATA_ERRORS                  0x05020204
#define RNDIS_OID_LTALK_RANDOM_CTS_ERRORS               0x05020205
#define RNDIS_OID_LTALK_FCS_ERRORS                      0x05020206


//
// Arcnet objects
//

#define RNDIS_OID_ARCNET_PERMANENT_ADDRESS              0x06010101
#define RNDIS_OID_ARCNET_CURRENT_ADDRESS                0x06010102

#define RNDIS_OID_ARCNET_RECONFIGURATIONS               0x06020201


//
// TAPI objects
//
#define RNDIS_OID_TAPI_ACCEPT                           0x07030101
#define RNDIS_OID_TAPI_ANSWER                           0x07030102
#define RNDIS_OID_TAPI_CLOSE                            0x07030103
#define RNDIS_OID_TAPI_CLOSE_CALL                       0x07030104
#define RNDIS_OID_TAPI_CONDITIONAL_MEDIA_DETECTION      0x07030105
#define RNDIS_OID_TAPI_CONFIG_DIALOG                    0x07030106
#define RNDIS_OID_TAPI_DEV_SPECIFIC                     0x07030107
#define RNDIS_OID_TAPI_DIAL                             0x07030108
#define RNDIS_OID_TAPI_DROP                             0x07030109
#define RNDIS_OID_TAPI_GET_ADDRESS_CAPS                 0x0703010A
#define RNDIS_OID_TAPI_GET_ADDRESS_ID                   0x0703010B
#define RNDIS_OID_TAPI_GET_ADDRESS_STATUS               0x0703010C
#define RNDIS_OID_TAPI_GET_CALL_ADDRESS_ID              0x0703010D
#define RNDIS_OID_TAPI_GET_CALL_INFO                    0x0703010E
#define RNDIS_OID_TAPI_GET_CALL_STATUS                  0x0703010F
#define RNDIS_OID_TAPI_GET_DEV_CAPS                     0x07030110
#define RNDIS_OID_TAPI_GET_DEV_CONFIG                   0x07030111
#define RNDIS_OID_TAPI_GET_EXTENSION_ID                 0x07030112
#define RNDIS_OID_TAPI_GET_ID                           0x07030113
#define RNDIS_OID_TAPI_GET_LINE_DEV_STATUS              0x07030114
#define RNDIS_OID_TAPI_MAKE_CALL                        0x07030115
#define RNDIS_OID_TAPI_NEGOTIATE_EXT_VERSION            0x07030116
#define RNDIS_OID_TAPI_OPEN                             0x07030117
#define RNDIS_OID_TAPI_PROVIDER_INITIALIZE              0x07030118
#define RNDIS_OID_TAPI_PROVIDER_SHUTDOWN                0x07030119
#define RNDIS_OID_TAPI_SECURE_CALL                      0x0703011A
#define RNDIS_OID_TAPI_SELECT_EXT_VERSION               0x0703011B
#define RNDIS_OID_TAPI_SEND_USER_USER_INFO              0x0703011C
#define RNDIS_OID_TAPI_SET_APP_SPECIFIC                 0x0703011D
#define RNDIS_OID_TAPI_SET_CALL_PARAMS                  0x0703011E
#define RNDIS_OID_TAPI_SET_DEFAULT_MEDIA_DETECTION      0x0703011F
#define RNDIS_OID_TAPI_SET_DEV_CONFIG                   0x07030120
#define RNDIS_OID_TAPI_SET_MEDIA_MODE                   0x07030121
#define RNDIS_OID_TAPI_SET_STATUS_MESSAGES              0x07030122


//
// ATM Connection Oriented Ndis
//
#define RNDIS_OID_ATM_SUPPORTED_VC_RATES                0x08010101
#define RNDIS_OID_ATM_SUPPORTED_SERVICE_CATEGORY        0x08010102
#define RNDIS_OID_ATM_SUPPORTED_AAL_TYPES               0x08010103
#define RNDIS_OID_ATM_HW_CURRENT_ADDRESS                0x08010104
#define RNDIS_OID_ATM_MAX_ACTIVE_VCS                    0x08010105
#define RNDIS_OID_ATM_MAX_ACTIVE_VCI_BITS               0x08010106
#define RNDIS_OID_ATM_MAX_ACTIVE_VPI_BITS               0x08010107
#define RNDIS_OID_ATM_ALIGNMENT_REQUIRED                0x08010108
#define RNDIS_OID_ATM_MAX_AAL0_PACKET_SIZE              0x08010109
#define RNDIS_OID_ATM_MAX_AAL1_PACKET_SIZE              0x0801010A
#define RNDIS_OID_ATM_MAX_AAL34_PACKET_SIZE             0x0801010B
#define RNDIS_OID_ATM_MAX_AAL5_PACKET_SIZE              0x0801010C

#define RNDIS_OID_ATM_SIGNALING_VPIVCI                  0x08010201
#define RNDIS_OID_ATM_ASSIGNED_VPI                      0x08010202
#define RNDIS_OID_ATM_ACQUIRE_ACCESS_NET_RESOURCES      0x08010203
#define RNDIS_OID_ATM_RELEASE_ACCESS_NET_RESOURCES      0x08010204
#define RNDIS_OID_ATM_ILMI_VPIVCI                       0x08010205
#define RNDIS_OID_ATM_DIGITAL_BROADCAST_VPIVCI          0x08010206
#define RNDIS_OID_ATM_GET_NEAREST_FLOW                  0x08010207

//
// ATM specific statistics OIDs.
//
#define RNDIS_OID_ATM_RCV_CELLS_OK                      0x08020101
#define RNDIS_OID_ATM_XMIT_CELLS_OK                     0x08020102
#define RNDIS_OID_ATM_RCV_CELLS_DROPPED                 0x08020103

#define RNDIS_OID_ATM_RCV_INVALID_VPI_VCI               0x08020201
#define RNDIS_OID_ATM_CELLS_HEC_ERROR                   0x08020202
#define RNDIS_OID_ATM_RCV_REASSEMBLY_ERROR              0x08020203

//
// PCCA (Wireless) object
//

//
// All WirelessWAN devices must support the following OIDs
//

#define RNDIS_OID_WW_GEN_NETWORK_TYPES_SUPPORTED        0x09010101
#define RNDIS_OID_WW_GEN_NETWORK_TYPE_IN_USE            0x09010102
#define RNDIS_OID_WW_GEN_HEADER_FORMATS_SUPPORTED       0x09010103
#define RNDIS_OID_WW_GEN_HEADER_FORMAT_IN_USE           0x09010104
#define RNDIS_OID_WW_GEN_INDICATION_REQUEST             0x09010105
#define RNDIS_OID_WW_GEN_DEVICE_INFO                    0x09010106
#define RNDIS_OID_WW_GEN_OPERATION_MODE                 0x09010107
#define RNDIS_OID_WW_GEN_LOCK_STATUS                    0x09010108
#define RNDIS_OID_WW_GEN_DISABLE_TRANSMITTER            0x09010109
#define RNDIS_OID_WW_GEN_NETWORK_ID                     0x0901010A
#define RNDIS_OID_WW_GEN_PERMANENT_ADDRESS              0x0901010B
#define RNDIS_OID_WW_GEN_CURRENT_ADDRESS                0x0901010C
#define RNDIS_OID_WW_GEN_SUSPEND_DRIVER                 0x0901010D
#define RNDIS_OID_WW_GEN_BASESTATION_ID                 0x0901010E
#define RNDIS_OID_WW_GEN_CHANNEL_ID                     0x0901010F
#define RNDIS_OID_WW_GEN_ENCRYPTION_SUPPORTED           0x09010110
#define RNDIS_OID_WW_GEN_ENCRYPTION_IN_USE              0x09010111
#define RNDIS_OID_WW_GEN_ENCRYPTION_STATE               0x09010112
#define RNDIS_OID_WW_GEN_CHANNEL_QUALITY                0x09010113
#define RNDIS_OID_WW_GEN_REGISTRATION_STATUS            0x09010114
#define RNDIS_OID_WW_GEN_RADIO_LINK_SPEED               0x09010115
#define RNDIS_OID_WW_GEN_LATENCY                        0x09010116
#define RNDIS_OID_WW_GEN_BATTERY_LEVEL                  0x09010117
#define RNDIS_OID_WW_GEN_EXTERNAL_POWER                 0x09010118

//
// Network Dependent OIDs - Mobitex:
//

#define RNDIS_OID_WW_MBX_SUBADDR                        0x09050101
// OID 0x09050102 is reserved and may not be used
#define RNDIS_OID_WW_MBX_FLEXLIST                       0x09050103
#define RNDIS_OID_WW_MBX_GROUPLIST                      0x09050104
#define RNDIS_OID_WW_MBX_TRAFFIC_AREA                   0x09050105
#define RNDIS_OID_WW_MBX_LIVE_DIE                       0x09050106
#define RNDIS_OID_WW_MBX_TEMP_DEFAULTLIST               0x09050107

//
// Network Dependent OIDs - Pinpoint:
//

#define RNDIS_OID_WW_PIN_LOC_AUTHORIZE                  0x09090101
#define RNDIS_OID_WW_PIN_LAST_LOCATION                  0x09090102
#define RNDIS_OID_WW_PIN_LOC_FIX                        0x09090103

//
// Network Dependent - CDPD:
//

#define RNDIS_OID_WW_CDPD_SPNI                          0x090D0101
#define RNDIS_OID_WW_CDPD_WASI                          0x090D0102
#define RNDIS_OID_WW_CDPD_AREA_COLOR                    0x090D0103
#define RNDIS_OID_WW_CDPD_TX_POWER_LEVEL                0x090D0104
#define RNDIS_OID_WW_CDPD_EID                           0x090D0105
#define RNDIS_OID_WW_CDPD_HEADER_COMPRESSION            0x090D0106
#define RNDIS_OID_WW_CDPD_DATA_COMPRESSION              0x090D0107
#define RNDIS_OID_WW_CDPD_CHANNEL_SELECT                0x090D0108
#define RNDIS_OID_WW_CDPD_CHANNEL_STATE                 0x090D0109
#define RNDIS_OID_WW_CDPD_NEI                           0x090D010A
#define RNDIS_OID_WW_CDPD_NEI_STATE                     0x090D010B
#define RNDIS_OID_WW_CDPD_SERVICE_PROVIDER_IDENTIFIER   0x090D010C
#define RNDIS_OID_WW_CDPD_SLEEP_MODE                    0x090D010D
#define RNDIS_OID_WW_CDPD_CIRCUIT_SWITCHED              0x090D010E
#define RNDIS_OID_WW_CDPD_TEI                           0x090D010F
#define RNDIS_OID_WW_CDPD_RSSI                          0x090D0110

//
// Network Dependent - Ardis:
//

#define RNDIS_OID_WW_ARD_SNDCP                          0x09110101
#define RNDIS_OID_WW_ARD_TMLY_MSG                       0x09110102
#define RNDIS_OID_WW_ARD_DATAGRAM                       0x09110103

//
// Network Dependent - DataTac:
//

#define RNDIS_OID_WW_TAC_COMPRESSION                    0x09150101
#define RNDIS_OID_WW_TAC_SET_CONFIG                     0x09150102
#define RNDIS_OID_WW_TAC_GET_STATUS                     0x09150103
#define RNDIS_OID_WW_TAC_USER_HEADER                    0x09150104

//
// Network Dependent - Metricom:
//

#define RNDIS_OID_WW_MET_FUNCTION                       0x09190101

//
// IRDA objects
//
#define RNDIS_OID_IRDA_RECEIVING                        0x0A010100
#define RNDIS_OID_IRDA_TURNAROUND_TIME                  0x0A010101
#define RNDIS_OID_IRDA_SUPPORTED_SPEEDS                 0x0A010102
#define RNDIS_OID_IRDA_LINK_SPEED                       0x0A010103
#define RNDIS_OID_IRDA_MEDIA_BUSY                       0x0A010104

#define RNDIS_OID_IRDA_EXTRA_RCV_BOFS                   0x0A010200
#define RNDIS_OID_IRDA_RATE_SNIFF                       0x0A010201
#define RNDIS_OID_IRDA_UNICAST_LIST                     0x0A010202
#define RNDIS_OID_IRDA_MAX_UNICAST_LIST_SIZE            0x0A010203



//
// Remote NDIS message types
//
#define REMOTE_NDIS_PACKET_MSG                  0x00000001
#define REMOTE_NDIS_INITIALIZE_MSG              0x00000002
#define REMOTE_NDIS_INITIALIZE_CMPLT_TYPE       0x80000002
#define REMOTE_NDIS_HALT_MSG                    0x00000003
#define REMOTE_NDIS_QUERY_MSG                   0x00000004
#define REMOTE_NDIS_QUERY_CMPLT_TYPE            0x80000004   
#define REMOTE_NDIS_SET_MSG                     0x00000005
#define REMOTE_NDIS_SET_CMPLT_TYPE              0x80000005
#define REMOTE_NDIS_RESET_MSG                   0x00000006
#define REMOTE_NDIS_RESET_CMPLT_TYPE            0x80000006
#define REMOTE_NDIS_INDICATE_STATUS_MSG         0x00000007
#define REMOTE_NDIS_KEEPALIVE_MSG               0x00000008
#define REMOTE_NDIS_KEEPALIVE_CMPLT_TYPE        0x80000008


#define REMOTE_CONDIS_MP_CREATE_VC_MSG          0x00008001
#define REMOTE_CONDIS_MP_DELETE_VC_MSG          0x00008002
#define REMOTE_CONDIS_MP_ACTIVATE_VC_MSG        0x00008005
#define REMOTE_CONDIS_MP_DEACTIVATE_VC_MSG      0x00008006
#define REMOTE_CONDIS_INDICATE_STATUS_MSG       0x00008007


// Remote NDIS message completion types
#define REMOTE_NDIS_INITIALIZE_CMPLT            0x80000002
#define REMOTE_NDIS_QUERY_CMPLT                 0x80000004
#define REMOTE_NDIS_SET_CMPLT                   0x80000005
#define REMOTE_NDIS_RESET_CMPLT                 0x80000006
#define REMOTE_NDIS_KEEPALIVE_CMPLT             0x80000008

#define REMOTE_CONDIS_MP_CREATE_VC_CMPLT        0x80008001
#define REMOTE_CONDIS_MP_DELETE_VC_CMPLT        0x80008002
#define REMOTE_CONDIS_MP_ACTIVATE_VC_CMPLT      0x80008005
#define REMOTE_CONDIS_MP_DEACTIVATE_VC_CMPLT    0x80008006

//
// Reserved message types for private communication between lower-layer
// host driver and remote device, if necessary.
//
#define REMOTE_NDIS_PRIVATE_MSG1                0xff000001
#define REMOTE_NDIS_PRIVATE_MSG2                0xff000002



//
//  Defines for DeviceFlags in RNDIS_INITIALIZE_COMPLETE
//
#define RNDIS_DF_CONNECTIONLESS             0x00000001
#define RNDIS_DF_CONNECTION_ORIENTED        0x00000002

//
//  Remote NDIS medium types.
//
#define RNdisMedium802_3                    0x00000000
#define RNdisMedium802_5                    0x00000001
#define RNdisMediumFddi                     0x00000002
#define RNdisMediumWan                      0x00000003
#define RNdisMediumLocalTalk                0x00000004
#define RNdisMediumArcnetRaw                0x00000006
#define RNdisMediumArcnet878_2              0x00000007
#define RNdisMediumAtm                      0x00000008
#define RNdisMediumWirelessWan              0x00000009
#define RNdisMediumIrda                     0x0000000a
#define RNdisMediumCoWan                    0x0000000b
#define RNdisMediumMax                      0x0000000d     // Not a real medium, defined as an upper-bound

//
// Remote NDIS medium connection states.
//
#define RNdisMediaStateConnected            0x00000000
#define RNdisMediaStateDisconnected         0x00000001

//
//  Remote NDIS version numbers
//
#define RNDIS_MAJOR_VERSION					0x00000001
#define RNDIS_MINOR_VERSION					0x00000000

//
//  NdisInitialize message
//
typedef struct _RNDIS_INITIALIZE_REQUEST 
{
    RNDIS_REQUEST_ID                        RequestId;
    UINT32                                  MajorVersion;
    UINT32                                  MinorVersion;
    UINT32                                  MaxTransferSize;
} RNDIS_INITIALIZE_REQUEST, *PRNDIS_INITIALIZE_REQUEST;


//
//  Response to NdisInitialize
//
typedef struct _RNDIS_INITIALIZE_COMPLETE 
{
    RNDIS_REQUEST_ID                        RequestId;
    RNDIS_STATUS                            Status;
    UINT32                                  MajorVersion;
    UINT32                                  MinorVersion;
    UINT32                                  DeviceFlags;
    RNDIS_MEDIUM                            Medium;
    UINT32                                  MaxPacketsPerMessage;
    UINT32                                  MaxTransferSize;
    UINT32                                  PacketAlignmentFactor;
    UINT32                                  AFListOffset;
    UINT32                                  AFListSize;
} RNDIS_INITIALIZE_COMPLETE, *PRNDIS_INITIALIZE_COMPLETE;


//
//  Call manager devices only: Information about an address family
//  supported by the device is appended to the response to NdisInitialize.
//
typedef struct _RNDIS_CO_ADDRESS_FAMILY
{
    RNDIS_AF                                AddressFamily;
    UINT32                                  MajorVersion;
    UINT32                                  MinorVersion;
} RNDIS_CO_ADDRESS_FAMILY, *PRNDIS_CO_ADDRESS_FAMILY;


//
//  NdisHalt message
//
typedef struct _RNDIS_HALT_REQUEST 
{
    RNDIS_REQUEST_ID                        RequestId;
} RNDIS_HALT_REQUEST, *PRNDIS_HALT_REQUEST;


//
// NdisQueryRequest message
//
typedef struct _RNDIS_QUERY_REQUEST 
{
    RNDIS_REQUEST_ID                        RequestId;
    RNDIS_OID                               Oid;
    UINT32                                  InformationBufferLength;
    UINT32                                  InformationBufferOffset;
    RNDIS_HANDLE                            DeviceVcHandle;
} RNDIS_QUERY_REQUEST, *PRNDIS_QUERY_REQUEST;


//
//  Response to NdisQueryRequest
//
typedef struct _RNDIS_QUERY_COMPLETE
{
    RNDIS_REQUEST_ID                        RequestId;
    RNDIS_STATUS                            Status;
    UINT32                                  InformationBufferLength;
    UINT32                                  InformationBufferOffset;
} RNDIS_QUERY_COMPLETE, *PRNDIS_QUERY_COMPLETE;


//
//  NdisSetRequest message
//
typedef struct _RNDIS_SET_REQUEST 
{
    RNDIS_REQUEST_ID                        RequestId;
    RNDIS_OID                               Oid;
    UINT32                                  InformationBufferLength;
    UINT32                                  InformationBufferOffset;
    RNDIS_HANDLE                            DeviceVcHandle;
} RNDIS_SET_REQUEST, *PRNDIS_SET_REQUEST;


//
//  Response to NdisSetRequest
//
typedef struct _RNDIS_SET_COMPLETE
{
    RNDIS_REQUEST_ID                        RequestId;
    RNDIS_STATUS                            Status;
} RNDIS_SET_COMPLETE, *PRNDIS_SET_COMPLETE;


//
//  NdisReset message
//
typedef struct _RNDIS_RESET_REQUEST
{
    UINT32                                  Reserved;
} RNDIS_RESET_REQUEST, *PRNDIS_RESET_REQUEST;

//
//  Response to NdisReset
//
typedef struct _RNDIS_RESET_COMPLETE
{
    RNDIS_STATUS                            Status;
    UINT32                                  AddressingReset;
} RNDIS_RESET_COMPLETE, *PRNDIS_RESET_COMPLETE;


//
//  NdisMIndicateStatus message
//
typedef struct _RNDIS_INDICATE_STATUS
{
    RNDIS_STATUS                            Status;
    UINT32                                  StatusBufferLength;
    UINT32                                  StatusBufferOffset;
} RNDIS_INDICATE_STATUS, *PRNDIS_INDICATE_STATUS;


//
//  Diagnostic information passed as the status buffer in
//  RNDIS_INDICATE_STATUS messages signifying error conditions.
//
typedef struct _RNDIS_DIAGNOSTIC_INFO
{
    RNDIS_STATUS                            DiagStatus;
    UINT32                                  ErrorOffset;
} RNDIS_DIAGNOSTIC_INFO, *PRNDIS_DIAGNOSTIC_INFO;



//
//  NdisKeepAlive message
//
typedef struct _RNDIS_KEEPALIVE_REQUEST
{
    RNDIS_REQUEST_ID                        RequestId;
} RNDIS_KEEPALIVE_REQUEST, *PRNDIS_KEEPALIVE_REQUEST;


//
// Response to NdisKeepAlive
//  
typedef struct _RNDIS_KEEPALIVE_COMPLETE
{
    RNDIS_REQUEST_ID                        RequestId;
    RNDIS_STATUS                            Status;
} RNDIS_KEEPALIVE_COMPLETE, *PRNDIS_KEEPALIVE_COMPLETE;


//
//  Data message. All Offset fields contain byte offsets from the beginning
//  of the RNDIS_PACKET structure. All Length fields are in bytes.
//  VcHandle is set to 0 for connectionless data, otherwise it
//  contains the VC handle.
//
typedef struct _RNDIS_PACKET
{
    UINT32                                  DataOffset;
    UINT32                                  DataLength;
    UINT32                                  OOBDataOffset;
    UINT32                                  OOBDataLength;
    UINT32                                  NumOOBDataElements;
    UINT32                                  PerPacketInfoOffset;
    UINT32                                  PerPacketInfoLength;
    RNDIS_HANDLE                            VcHandle;
    UINT32                                  Reserved;
} RNDIS_PACKET, *PRNDIS_PACKET;

//
//  Optional Out of Band data associated with a Data message.
//
typedef struct _RNDIS_OOBD
{
    UINT32                                  Size;
    RNDIS_CLASS_ID                          Type;
    UINT32                                  ClassInformationOffset;
} RNDIS_OOBD, *PRNDIS_OOBD;

//
//  Packet extension field contents associated with a Data message.
//
typedef struct _RNDIS_PER_PACKET_INFO
{
    UINT32                                  Size;
    UINT32                                  Type;
    UINT32                                  PerPacketInformationOffset;
} RNDIS_PER_PACKET_INFO, *PRNDIS_PER_PACKET_INFO;


//
//  CONDIS Miniport messages for connection oriented devices
//  that do not implement a call manager.
//

//
//  CoNdisMiniportCreateVc message
//
typedef struct _RCONDIS_MP_CREATE_VC 
{
    RNDIS_REQUEST_ID                        RequestId;
    RNDIS_HANDLE                            NdisVcHandle;
} RCONDIS_MP_CREATE_VC, *PRCONDIS_MP_CREATE_VC;

//
//  Response to CoNdisMiniportCreateVc
//
typedef struct _RCONDIS_MP_CREATE_VC_COMPLETE 
{
    RNDIS_REQUEST_ID                        RequestId;
    RNDIS_HANDLE                            DeviceVcHandle;
    RNDIS_STATUS                            Status;
} RCONDIS_MP_CREATE_VC_COMPLETE, *PRCONDIS_MP_CREATE_VC_COMPLETE;


//
//  CoNdisMiniportDeleteVc message
//
typedef struct _RCONDIS_MP_DELETE_VC 
{
    RNDIS_REQUEST_ID                        RequestId;
    RNDIS_HANDLE                            DeviceVcHandle;
} RCONDIS_MP_DELETE_VC, *PRCONDIS_MP_DELETE_VC;

//
//  Response to CoNdisMiniportDeleteVc
//
typedef struct _RCONDIS_MP_DELETE_VC_COMPLETE 
{
    RNDIS_REQUEST_ID                        RequestId;
    RNDIS_STATUS                            Status;
} RCONDIS_MP_DELETE_VC_COMPLETE, *PRCONDIS_MP_DELETE_VC_COMPLETE;


//
//  CoNdisMiniportQueryRequest message
//
typedef struct _RCONDIS_MP_QUERY_REQUEST 
{
    RNDIS_REQUEST_ID                        RequestId;
    RNDIS_REQUEST_TYPE                      RequestType;
    RNDIS_OID                               Oid;
    RNDIS_HANDLE                            DeviceVcHandle;
    UINT32                                  InformationBufferLength;
    UINT32                                  InformationBufferOffset;
} RCONDIS_MP_QUERY_REQUEST, *PRCONDIS_MP_QUERY_REQUEST;


//
//  CoNdisMiniportSetRequest message
//
typedef struct _RCONDIS_MP_SET_REQUEST 
{
    RNDIS_REQUEST_ID                        RequestId;
    RNDIS_REQUEST_TYPE                      RequestType;
    RNDIS_OID                               Oid;
    RNDIS_HANDLE                            DeviceVcHandle;
    UINT32                                  InformationBufferLength;
    UINT32                                  InformationBufferOffset;
} RCONDIS_MP_SET_REQUEST, *PRCONDIS_MP_SET_REQUEST;


//
//  CoNdisIndicateStatus message
//
typedef struct _RCONDIS_INDICATE_STATUS
{
    RNDIS_HANDLE                            NdisVcHandle;
    RNDIS_STATUS                            Status;
    UINT32                                  StatusBufferLength;
    UINT32                                  StatusBufferOffset;
} RCONDIS_INDICATE_STATUS, *PRCONDIS_INDICATE_STATUS;


//
//  CONDIS Call/VC parameters
//

typedef struct _RCONDIS_SPECIFIC_PARAMETERS
{
    UINT32                                  ParameterType;
    UINT32                                  ParameterLength;
    UINT32                                  ParameterOffset;
} RCONDIS_SPECIFIC_PARAMETERS, *PRCONDIS_SPECIFIC_PARAMETERS;

typedef struct _RCONDIS_MEDIA_PARAMETERS
{
    UINT32                                  Flags;
    UINT32                                  Reserved1;
    UINT32                                  Reserved2;
    RCONDIS_SPECIFIC_PARAMETERS             MediaSpecific;
} RCONDIS_MEDIA_PARAMETERS, *PRCONDIS_MEDIA_PARAMETERS;


typedef struct _RNDIS_FLOWSPEC
{
    UINT32                                  TokenRate;
    UINT32                                  TokenBucketSize;
    UINT32                                  PeakBandwidth;
    UINT32                                  Latency;
    UINT32                                  DelayVariation;
    UINT32                                  ServiceType;
    UINT32                                  MaxSduSize;
    UINT32                                  MinimumPolicedSize;
} RNDIS_FLOWSPEC, *PRNDIS_FLOWSPEC;

typedef struct _RCONDIS_CALL_MANAGER_PARAMETERS
{
    RNDIS_FLOWSPEC                          Transmit;
    RNDIS_FLOWSPEC                          Receive;
    RCONDIS_SPECIFIC_PARAMETERS             CallMgrSpecific;
} RCONDIS_CALL_MANAGER_PARAMETERS, *PRCONDIS_CALL_MANAGER_PARAMETERS;

//
//  CoNdisMiniportActivateVc message
//
typedef struct _RCONDIS_MP_ACTIVATE_VC_REQUEST
{
    RNDIS_REQUEST_ID                        RequestId;
    UINT32                                  Flags;
    RNDIS_HANDLE                            DeviceVcHandle;
    UINT32                                  MediaParamsOffset;
    UINT32                                  MediaParamsLength;
    UINT32                                  CallMgrParamsOffset;
    UINT32                                  CallMgrParamsLength;
} RCONDIS_MP_ACTIVATE_VC_REQUEST, *PRCONDIS_MP_ACTIVATE_VC_REQUEST;

//
//  Response to CoNdisMiniportActivateVc
//
typedef struct _RCONDIS_MP_ACTIVATE_VC_COMPLETE
{
    RNDIS_REQUEST_ID                        RequestId;
    RNDIS_STATUS                            Status;
} RCONDIS_MP_ACTIVATE_VC_COMPLETE, *PRCONDIS_MP_ACTIVATE_VC_COMPLETE;


//
//  CoNdisMiniportDeactivateVc message
//
typedef struct _RCONDIS_MP_DEACTIVATE_VC_REQUEST
{
    RNDIS_REQUEST_ID                        RequestId;
    UINT32                                  Flags;
    RNDIS_HANDLE                            DeviceVcHandle;
} RCONDIS_MP_DEACTIVATE_VC_REQUEST, *PRCONDIS_MP_DEACTIVATE_VC_REQUEST;

//
//  Response to CoNdisMiniportDeactivateVc
//
typedef struct _RCONDIS_MP_DEACTIVATE_VC_COMPLETE
{
    RNDIS_REQUEST_ID                        RequestId;
    RNDIS_STATUS                            Status;
} RCONDIS_MP_DEACTIVATE_VC_COMPLETE, *PRCONDIS_MP_DEACTIVATE_VC_COMPLETE;


//
// union with all of the RNDIS messages
//
typedef union _RNDIS_MESSAGE_CONTAINER
{
    RNDIS_PACKET                        Packet;
    RNDIS_INITIALIZE_REQUEST            InitializeRequest;
    RNDIS_HALT_REQUEST                  HaltRequest;
    RNDIS_QUERY_REQUEST                 QueryRequest;
    RNDIS_SET_REQUEST                   SetRequest;
    RNDIS_RESET_REQUEST                 ResetRequest;
    RNDIS_KEEPALIVE_REQUEST             KeepaliveRequest;
    RNDIS_INDICATE_STATUS               IndicateStatus;
    RNDIS_INITIALIZE_COMPLETE           InitializeComplete;
    RNDIS_QUERY_COMPLETE                QueryComplete;
    RNDIS_SET_COMPLETE                  SetComplete;
    RNDIS_RESET_COMPLETE                ResetComplete;
    RNDIS_KEEPALIVE_COMPLETE            KeepaliveComplete;
    RCONDIS_MP_CREATE_VC                CoMiniportCreateVc;
    RCONDIS_MP_DELETE_VC                CoMiniportDeleteVc;
    RCONDIS_INDICATE_STATUS             CoIndicateStatus;
    RCONDIS_MP_ACTIVATE_VC_REQUEST      CoMiniportActivateVc;
    RCONDIS_MP_DEACTIVATE_VC_REQUEST    CoMiniportDeactivateVc;
    RCONDIS_MP_CREATE_VC_COMPLETE       CoMiniportCreateVcComplete;
    RCONDIS_MP_DELETE_VC_COMPLETE       CoMiniportDeleteVcComplete;
    RCONDIS_MP_ACTIVATE_VC_COMPLETE     CoMiniportActivateVcComplete;
    RCONDIS_MP_DEACTIVATE_VC_COMPLETE   CoMiniportDeactivateVcComplete;


} RNDIS_MESSAGE_CONTAINER, *PRNDIS_MESSAGE_CONTAINER;

//
// Remote NDIS message format
//
typedef struct _RNDIS_MESSAGE 
{
    UINT32                                  NdisMessageType;

    //
    // Total length of this message, from the beginning
    // of the RNDIS_MESSAGE struct, in bytes.
    //
    UINT32                                  MessageLength;

    // Actual message
    RNDIS_MESSAGE_CONTAINER                 Message;

} RNDIS_MESSAGE, *PRNDIS_MESSAGE;



//
// Handy macros

// get the size of an RNDIS message. Pass in the message type, 
// RNDIS_SET_REQUEST, RNDIS_PACKET for example
#define RNDIS_MESSAGE_SIZE(Message)                             \
    (sizeof(Message) + (sizeof(RNDIS_MESSAGE) - sizeof(RNDIS_MESSAGE_CONTAINER)))

// get pointer to info buffer with message pointer
#define MESSAGE_TO_INFO_BUFFER(Message)                         \
    (((PUCHAR)(Message)) + Message->InformationBufferOffset)

// get pointer to status buffer with message pointer
#define MESSAGE_TO_STATUS_BUFFER(Message)                       \
    (((PUCHAR)(Message)) + Message->StatusBufferOffset)

// get pointer to OOBD buffer with message pointer
#define MESSAGE_TO_OOBD_BUFFER(Message)                         \
    (((PUCHAR)(Message)) + Message->OOBDataOffset)

// get pointer to data buffer with message pointer
#define MESSAGE_TO_DATA_BUFFER(Message)                         \
    (((PUCHAR)(Message)) + Message->PerPacketInfoOffset)

// get pointer to contained message from NDIS_MESSAGE pointer
#define RNDIS_MESSAGE_PTR_TO_MESSAGE_PTR(RndisMessage)          \
    ((PVOID) &RndisMessage->Message)


#endif // _RNDIS_H_
