/*
<:copyright-BRCM:2013:proprietary:standard  

   Copyright (c) 2013 Broadcom 
   All Rights Reserved

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
*/
/**************************************************************************
 * File Name  : bcm63xx_usb.h
 *
 * Description: This file contains constant definitions, structure definitions
 *              and function prototypes for a USB network interface driver
 *              that executes on the BCM963xx reference platforms.
 *
 * Updates    : 02/19/2002  lat.   Created for Linux.
 ***************************************************************************/

#if !defined(_BCM63XX_USB_H)
#define _BCM63XX_USB_H

#if defined __cplusplus
extern "C" {
#endif

#ifdef  DUMP_USB_INFO
#define SCTLPKT(pDC, pB, uL, adv)       {printk("UsbNicSubmitControlPacket(): %lu bytes @ %d%s\n", uL, __LINE__ , adv == TRUE ? " (A)" : "" ); UsbNicSubmitControlPacket(pDC, pB, uL, adv);}
#define DUMPADDR(addr, len)             dumpaddr(addr, len)
#define PRINTK(format, arg...)     	printk(KERN_INFO format , ## arg)
#else
#define SCTLPKT(pDC, pB, uL, adv)       UsbNicSubmitControlPacket(pDC, pB, uL, adv)
#define DUMPADDR(addr, len)             do { } while (0)
#define PRINTK(format, arg...)          do { } while (0)
#endif

/* Standard Request Codes */
#define GET_STATUS                      0
#define CLEAR_FEATURE                   1
#define SET_FEATURE                     3
#define SET_ADDRESS                     5
#define GET_DESCRIPTOR                  6
#define SET_DESCRIPTOR                  7
#define GET_CONFIGURATION               8
#define SET_CONFIGURATION               9
#define GET_INTERFACE                   10
#define SET_INTERFACE                   11
#define SYNCH_FRAME                     12

#define USBD_INIT                       0x0
#define USBD_REINIT                     0x1

	// Optional
#define     SET_HOST_VERSION            0x45

/* supported class specific Request Codes */
#define SEND_ENCAPSULATED_COMMAND       0x00
#define GET_ENCAPSULATED_RESPONSE       0x01
#define SET_ETHERNET_PACKET_FILTER      0x43

/* bmRequestType Types */
#define RT_TYPE_MASK                    0x60
#define RT_TYPE_STANDARD                0x00
#define RT_TYPE_CLASS                   0x20
#define RT_TYPE_VENDOR                  0x40

/* These packet filter definitions are defined in the Microsoft DDK ndis.h. */
#define NDIS_PACKET_TYPE_DIRECTED       0x00000001
#define NDIS_PACKET_TYPE_MULTICAST      0x00000002
#define NDIS_PACKET_TYPE_ALL_MULTICAST  0x00000004
#define NDIS_PACKET_TYPE_BROADCAST      0x00000008
#define NDIS_PACKET_TYPE_SOURCE_ROUTING 0x00000010
#define NDIS_PACKET_TYPE_PROMISCUOUS    0x00000020
#define NDIS_PACKET_TYPE_SMT            0x00000040
#define NDIS_PACKET_TYPE_ALL_LOCAL      0x00000080
#define NDIS_PACKET_TYPE_GROUP          0x00001000
#define NDIS_PACKET_TYPE_ALL_FUNCTIONAL 0x00002000
#define NDIS_PACKET_TYPE_FUNCTIONAL     0x00004000
#define NDIS_PACKET_TYPE_MAC_FRAME      0x00008000

/* These packet filter definitions are defined in the USB CDC specification. */
#define USB_PACKET_TYPE_MULTICAST       0x0010  /* disable is optional */
#define USB_PACKET_TYPE_BROADCAST       0x0008  /* disable is optional */
#define USB_PACKET_TYPE_DIRECTED        0x0004  /* disable is optional */
#define USB_PACKET_TYPE_ALL_MULTICAST   0x0002  /* disable is MUST */
#define USB_PACKET_TYPE_PROMISCUOUS     0x0001  /* disable is MUST */

/* Descriptor Types */
#define DEVICE_DESC                     1
#define CONFIGURATION_DESC              2
#define STRING_DESC                     3
#define INTERFACE_DESC                  4
#define ENDPOINT_DESC                   5
#define DEVQUALIFIER_DESC					 6
#define OTHER_SPEED_DESC					 7

#define CS_INTERFACE                    0x24
#define CS_ENDPOINT                     0x25

typedef enum                    // Device Class Codes  (values for bDeviceClass) -- we use only one value
{                               // From Table 14 in CDC 1.1 spec
  kCommDevice = 2
} UsbDeviceClassCode;


typedef enum                    // Interface Class Codes (values for bInterfaceClass)
{                               // From Table 15 in CDC 1.1 spec
  kCommInterface = 0x02,
  kDataInterface = 0x0a
} UsbInterfaceClassCode;


typedef enum                    // Communication Interface Class SubClass Codes (values for bInterfaceSubClass)
{                               // From Table 16 in CDC 1.1 spec  (defining only the ones we use)
  kAbstractControl = 2,
  kEthernetControl = 6
} UsbCommInterfaceSubClassCode;


typedef enum                    // SubTypes for Functional Descriptors (values for bDescriptorTypeSubtype)
                // From Table 25 in CDC 1.1 spec -- defining only what we need
{
  kHeaderFunc = 0x00,
  kCallManagementFunc = 0x01,
  kAbstractControlFunc = 0x02,
  kUnionFunc = 0x06,
  kEthernetNetFunc = 0x0f
} UsbDescriptorSubType;


typedef enum                    // Standard Feature Selectors -- when UsbRequestCode is kSetFeature,
{
  kEndpointHalt = 0,
  kDeviceRemoteWakeup = 1
} UsbStandardFeature;


typedef enum                    // Transfer Types for the Endpoint (part of bmAttributes)
{
  kControl = 0,
  kIsoChronous = 1,
  kBulk = 2,
  kInterrupt = 3
} UsbEpTransferType;

typedef enum                    // These states are defined in the USB spec
{
  kUsbDevUnattached = 1,
  kUsbDevAttached = 2,
  kUsbDevPowered = 3,
  kUsbDevDefault = 4,
  kUsbDevAddress = 5,
  kUsbDevConfigured = 6,
  kUsbDevSuspended = 7
} UsbDeviceStatus;


/* Miscellaneous definitions */
#define USB_DMA_MAX_BURST_LENGTH        8       /* in 32 bit words */
#define USB_VENDOR_ID                   0x0a5c
#define USB_DSL_PRODUCT_ID              0x6300
#define STRING_TABLE_SIZE               7
#define MAX_USB_FIFO_WAIT_CNT           2000000
#define BCM_USB_NIC_DBG                 0x00

/* NDIS and RNDIS definitions that are not in rndis.h but are documented on
 * MSDN.
 */
#define RNDIS_OID_GEN_PHYSICAL_MEDIUM           0x00010202
#define RNDIS_OID_GEN_RNDIS_CONFIG_PARAMETER    0x0001021b
#define NdisPhysicalMediumDSL                   5
#define NdisHardwareStatusReady                 0
#define NDIS_MAC_OPTION_NO_LOOPBACK             8

/*----------------------------------------------------------------------
 * endianess conversion MACROS
 *---------------------------------------------------------------------*/

/* Endian conversions. */
#if defined(CONFIG_CPU_LITTLE_ENDIAN) /*processor is configured as little endian*/

#define BE_SWAP2(x) (x)
#define BE_SWAP4(x) (x)
#define LE_SWAP2(x) (((unsigned short) (x) << 8) | ((unsigned short) (x) >> 8))
#define LE_SWAP4(x) (((unsigned long) (x) << 24) | \
                      (((unsigned long) (x) << 8) & 0x00ff0000) | \
                      (((unsigned long) (x) >> 8) & 0x0000ff00) | \
                      ((unsigned long) (x) >> 24))

#else /* processor is configured as big endian */

#define BE_SWAP2(x) (((unsigned short) (x) << 8) | ((unsigned short) (x) >> 8))
#define BE_SWAP4(x) (((unsigned long) (x) << 24) | \
                      (((unsigned long) (x) << 8) & 0x00ff0000) | \
                      (((unsigned long) (x) >> 8) & 0x0000ff00) | \
                      ((unsigned long) (x) >> 24))
#define LE_SWAP2(x) (x)
#define LE_SWAP4(x) (x)

#endif

                              
/*----------------------------------------------------------------------
 * USB protocol structures
 *---------------------------------------------------------------------*/

/* Enable structure packing. */
#pragma pack(1)

#if defined(__GNUC__)
#define __packed__
#define PACKED  __attribute__((packed))
#else
#define PACKED
#endif

typedef enum
{
    USB_NT_LINKSTATUS,
    USB_NT_SPEEDCHANGE,
    USB_NT_NUMITEMS
} USB_NOTIFY_TYPE;    

/* USB setup command */
typedef struct
{
    UINT8  bmRequestType;
    UINT8  bRequest;      
    UINT16 wValue;
    UINT16 wIndex;
    UINT16 wLength;   
} PACKED USB_setup;

/* Device descriptor */
typedef struct
{
	UINT8 bLength;                // descriptor size in bytes
	UINT8 bDescriptorType;        // descriptor type is 1 (DEVICE)
	UINT8 bcdUSB_hi;              // USB spec release number in BCD
	UINT8 bcdUSB_lo;
	UINT8 bDeviceClass;           // class code
	UINT8 bDeviceSubClass;        // subclass code
	UINT8 bDeviceProtocol;        // protocol code
	UINT8 bMaxPacketSize0;        // max packet size for Endpoint 0
	UINT8 idVendor_hi;            // vendor ID
	UINT8 idVendor_lo;
	UINT8 idProduct_hi;           // product ID
	UINT8 idProduct_lo;
	UINT8 bcdDevice_hi;           // device release number (BCD)
	UINT8 bcdDevice_lo;
	UINT8 iManufacturer;          // index into string table for manufacturer name
	UINT8 iProduct;               // index into string table for product name
	UINT8 iSerialNumber;          // index into string table for serial number
	UINT8 bNumConfigurations;     // number of configurations
} PACKED UsbDeviceDesc;

typedef struct
{
	UINT8 bLength;                // descriptor size in bytes
	UINT8 bDescriptorType;        // descriptor type is 1 (DEVICE)
	UINT8 bcdUSB_hi;              // USB spec release number in BCD
	UINT8 bcdUSB_lo;
	UINT8 bDeviceClass;           // class code
	UINT8 bDeviceSubClass;        // subclass code
	UINT8 bDeviceProtocol;        // protocol code
	UINT8 bMaxPacketSize0;        // max packet size for Endpoint 0
	UINT8 bNumConfigurations;     // number of configurations
	UINT8 reserved;
} PACKED UsbDevQualifierDescriptor;

/* Configuration descriptor */
typedef struct
{
	UINT8 bLength;                // descriptor size in bytes
	UINT8 bDescriptorType;        // descriptor type is 2 (CONFIGURATION)
	UINT8 wTotalLength_hi;        // size of all data returned for this config in bytes
	UINT8 wTotalLength_lo;
	UINT8 bNumInterfaces;         // number of interfaces the configuration supports
	UINT8 bConfigValue;           // the id of this configuration
	UINT8 iConfiguration;         // index into string table for configuration name 
	UINT8 bmAttributes;           // bitmap; bit 7 always 1; bit 6 = self powered; bit 5 = supports remote wakeup
#define USB_RESERVED_ALWAYS_SET     0x80
#define USB_CONFIG_SELF_POWERED     0x40
#define USB_CONFIG_REMOTE_WAKEUP    0x20
	UINT8 MaxPower;               // bus power required (units of 2ma)
} PACKED UsbConfigDesc;

/* Communication Class descriptor */
typedef struct
{
	UINT8 bLength;                // descriptor size in bytes
	UINT8 bDescriptorType;        // descriptor type is 4 (INTERFACE)
	UINT8 bInterfaceNumber;       // number identifying this interface
	UINT8 bAlternateSetting;      // value used to select an alternate setting
	UINT8 bNumEndpoints;          // number of endpoints supported, except Endpoint 0
	UINT8 bInterfaceClass;        // class code
	UINT8 bInterfaceSubClass;     // subclass code
	UINT8 bInterfaceProtocol;     // protocol code
	UINT8 iInterface;             // index into string table for interface name
} PACKED UsbInterfaceDesc;

//
// Interface Class Descriptors -- for CDC Ethernet Control Model and RNDIS Model
//
// The Header Functional Descriptor is used for both models.
typedef struct
{
	UINT8 bLength;                // descriptor size in bytes (5)
	UINT8 bDescriptorType;        // descriptor type is 0x24 (CS_INTERFACE)
	UINT8 bDescriptorSubType;     // sub-type is 0 (Header)
	UINT8 bcdCDC_hi;              // revision of USB CDC spec
	UINT8 bcdCDC_lo;
} PACKED UsbHeaderDescriptor;


// The Union Functional Descriptor is used for both models, but the 
// values for the master and slave interfaces may be different.
typedef struct
{
	UINT8 bLength;                // descriptor size in bytes (5)
	UINT8 bDescriptorType;        // descriptor type is 0x24 (CS_INTERFACE)
	UINT8 bDescriptorSubType;     // sub-type is 6 (Union)
	UINT8 bMasterInterface;       // set to bInterfaceNumber of the Comm Class Interface
	UINT8 bSlaveInterface;        // set to bInterfaceNumber of the Data Class Interface
} PACKED UsbUnionDescriptor;


// The Ethernet Networking Functional Descriptor is used only
// in the Ethernet Control Model
typedef struct
{
	UINT8 bLength;                // descriptor size in bytes (13)
	UINT8 bDescriptorType;        // descriptor type is 0x24 (CS_INTERFACE)
	UINT8 bDescriptorSubType;     // sub-type is 0xf (Ethernet Networking)
	UINT8 iMacAddress;            // index into string table for mac address
	UINT8 bmEthernetStatisticsLowest;        // bitmap -- 0 means keep count of Ethernet statistic
	UINT8 bmEthernetStatisticsLow;
	UINT8 bmEthernetStatisticsHigh;
	UINT8 bmEthernetStatisticsHighest;
	UINT8 wMaxSegmentSize_hi;     // max segment size Ethernet device can support (typically 1514 bytes)
	UINT8 wMaxSegmentSize_lo;
	UINT8 wNumberMCFilters_hi;    // number of multicast filters that can be configured
	UINT8 wNumberMCFilters_lo;
	UINT8 bNumberPowerFilters;    // number of power filters that can cause wake-up of host
} PACKED UsbEnetNetworkingDescriptor;


typedef struct
{
	UsbHeaderDescriptor				hdrDesc ;
	UsbUnionDescriptor 				unionDesc ;
	UsbEnetNetworkingDescriptor	enerNetDesc ;
} PACKED COMM_classDesc;


// The Call Magagement Func Descriptor is used only for RNDIS.
typedef struct
{
	UINT8 bLength;                // descriptor size in bytes (5)
	UINT8 bDescriptorType;        // descriptor type is 0x24 (CS_INTERFACE)
	UINT8 bDescriptorSubType;     // sub-type is 1 (Call Management)
	UINT8 bmCapabilities;         // bitmap of capabilities -- set to 0
	UINT8 bDataInterface;         // data interface (ignored if bmCapabilities is 0)
} PACKED UsbCallManagementDescriptor;


// The Abstract Control Magagement Func Descriptor is used only for RNDIS.
typedef struct
{
	UINT8 bLength;                // descriptor size in bytes (4)
	UINT8 bDescriptorType;        // descriptor type is 0x24 (CS_INTERFACE)
	UINT8 bDescriptorSubType;     // sub-type is 2 (Abstract Control)
	UINT8 bmCapabilities;         // bitmap of capabilities -- set to 0
} PACKED UsbAbstractControlDescriptor;


typedef struct
{
	UsbHeaderDescriptor				hdrDesc ;
	UsbCallManagementDescriptor   calMgmtDesc ;
	UsbAbstractControlDescriptor  abstractDesc ;
	UsbUnionDescriptor 				unionDesc ;
} PACKED RNDIS_classDesc;

/* Endpoint Descriptor */
typedef struct
{
	UINT8 bLength;                // descriptor size in bytes
	UINT8 bDescriptorType;        // descriptor type is 5 (ENDPOINT)
	UINT8 bEndpointAddress;       // endpoint number and direction
	UINT8 bmAttribute;            // transfer type supported
	UINT8 wMaxPacketSizeLSB;      // max packet size supported
	UINT8 wMaxPacketSizeMSB;
	UINT8 bInterval;              // max latency/polling interval/NAK rate
} PACKED UsbEndpointDesc;

#define USB_HS_INT_CDC_BINTERVAL	10	// (2 ^ (10 - 1)) * 125E-6 = 64msec
#define USB_FS_INT_CDC_BINTERVAL	64	// 64 * 1msec frames
#define USB_HS_INT_RNDIS_BINTERVAL	11	// (2 ^ (11 - 1)) * 125E-6 = 128msec
#define USB_FS_INT_RNDIS_BINTERVAL	100	// 100 * 1msec frames
#define	USB_HS_BOUT_BINTERVAL		10	// 10 * 125E-6 = 12.5usec // FIXME?!
#define	USB_FS_BOUT_BINTERVAL		0

typedef struct
{
	UsbDeviceDesc 					deviceDesc;
	UsbDevQualifierDescriptor			deviceQualDesc;

	/* RNDIS */
	UsbConfigDesc 					rndis_configDesc;
	UsbInterfaceDesc 				rndis_interfaceDesc;
	RNDIS_classDesc 				rndis_comm_classDesc;
	UsbEndpointDesc 				rndis_endpoint_notificationDesc;
	UsbInterfaceDesc 				rndis_dataClassDesc;
	UsbEndpointDesc 				rndis_endpoint_bulkDataInDesc;
	UsbEndpointDesc 				rndis_endpoint_bulkDataOutDesc;

	/* CDC */
	UsbConfigDesc 					cdc_configDesc;
	UsbInterfaceDesc 				cdc_interfaceDesc;
	COMM_classDesc 					cdc_comm_classDesc;
	UsbEndpointDesc 				cdc_endpoint_notificationDesc;
	UsbInterfaceDesc 				cdc_dataClassDesc;
	UsbInterfaceDesc 				cdc_dataClassDescAlt;
	UsbEndpointDesc 				cdc_endpoint_bulkDataInDesc;
	UsbEndpointDesc 				cdc_endpoint_bulkDataOutDesc;
} PACKED UsbConfiguration;


/* String Descriptor structure (Generic) */
#define GENERIC_STRING_LEN  128
typedef struct
{
    UINT8 bLength;
    UINT8 bDescriptorType;
    UINT8 genericString[GENERIC_STRING_LEN];      
} PACKED StringDesc;          


typedef struct
{
    char string[GENERIC_STRING_LEN];
} PACKED StringTable;


typedef enum                    // indexes into USB string table
{
  kLangIdString = 0,
  kVendorString,
  kProductString,
  kMacAddressString,
  kConfigString,
  kInterface1String,
  kInterface2String
} UsbStringIndex;


#undef PACKED
#pragma pack()

#define  USB_NIC_ALIGN(addr, bound) (((UINT32) addr + bound - 1) & ~(bound - 1))

/* Configuration items */
#define USB_NUM_RX_PKTS             (1 << 6)
#define USB_RX_BD_RING_SIZE         (1 << 6)
#define USB_NR_RX_BDS               USB_RX_BD_RING_SIZE
#define USB_NR_TX_BDS               100
#define NR_TX_HDRS                  (USB_NR_TX_BDS / 2)
#define NR_CTRL_BDS              	4 /* Needs to be power of 2 */
#define NR_INTR_BDS              	4
#define USB_MAX_FRAME_SIZE          1500
#define USB_MAX_MTU_SIZE            (USB_MAX_FRAME_SIZE + sizeof(ENET_HDR))
#define RX_USB_BUF_OVERHEAD         22
#define RX_USB_FKB_INPLACE          ((sizeof(FkBuff_t) + 0x0f) & ~0x0f)
#define BULK_RX_DMA_MAX_BURST_LEN   4
#define RX_USB_SKB_RESERVE          ((208 + 0x0f) & ~0x0f)
#define RX_USB_ALIGN                ((BULK_RX_DMA_MAX_BURST_LEN * 8) - 1)
#define RX_USB_NIC_BUF_SIZE         ((USB_MAX_MTU_SIZE + RX_USB_BUF_OVERHEAD + \
                                     sizeof(RNDIS_MESSAGE) + RX_USB_ALIGN) \
                                     & ~RX_USB_ALIGN)
#define RX_USB_ALLOC_BUF_SIZE       ((RX_USB_FKB_INPLACE +  \
                                      RX_USB_NIC_BUF_SIZE + \
                                      RX_USB_SKB_RESERVE +  \
                                      0x0f) & ~0x0f)
#define RX_USB_ALLOC_TOTAL_SIZE     (RX_USB_ALLOC_BUF_SIZE + \
                                     ((sizeof(struct skb_shared_info) + 0x0f) & \
                                      ~0x0f))
#define SKB_ALIGNED_SIZE            ((sizeof(struct sk_buff) + 0x0f) & ~0x0f)
#define MAC_ADDR_LEN                6
#define SERIAL_NUM_INDEX            3
#define EH_SIZE                     14
#define USB_TIMEOUT                 (HZ/20)
#define MAX_MULTICAST_ADDRESSES     32
#define MAX_CTRL_PKT_SIZE           224
#define RNDIS_PACKET_OVERHEAD       8

#define USB_FLAGS_PLUGGED_IN        0x01 /* USB cable is physically plugged in*/
#define USB_FLAGS_CONNECTED         0x02 /* data can now be sent and received */
#define USB_FLAGS_OPENED            0x04 /* OS has opened this network device */
#define USB_FLAGS_RNDIS_SOFT_RESET  0x08 /* RNDIS_RESET_REQUEST */

#define HOST_DRIVER_RNDIS           0x01
#define HOST_DRIVER_CDC             0x02

#define RNDIS_STATE_UNINITIALIZED       0
#define RNDIS_STATE_INITIALIZED         1
#define RNDIS_STATE_DATA_INITIALIZED    2

#define REMOTE_NDIS_INVALID_MSG     0x7fffffff

#define NR_USB_DMA_CHANNELS         6
#define FIRST_USB_DMA_CHANNEL       0
#define USB_CNTL_RX_CHAN            0
#define USB_CNTL_TX_CHAN            1
#define USB_BULK1_RX_CHAN           2
#define USB_BULK1_TX_CHAN           3
#define USB_IRQ_RX_CHAN           	4
#define USB_IRQ_TX_CHAN           	5

// Got really tired of seeing these consts all over the place
#define	USB_CTL_EP_SIZE			64

#define	USB_HS_MAX_BULK			512
#define	USB_HS_MAX_INT			64

#define USB_FS_MAX_BULK			64
#define	USB_FS_MAX_INT			8

typedef struct EnetHdr
{
    UINT8 dest[ETH_ALEN];
    UINT8 src[ETH_ALEN];
    UINT16 type;
} ENET_HDR, *PENET_HDR;

typedef struct UsbTxHdr
{
    struct UsbTxHdr *pNext;      /* ptr to next header in free list */
    volatile DmaDesc *pBdAddr;
    void * pNBuff;
    RNDIS_MESSAGE PktHdr;
} USB_TX_HDR, *PUSB_TX_HDR;

typedef struct CtrlPktBufs
{
    UINT8 ulBuf[MAX_CTRL_PKT_SIZE];
    volatile DmaDesc *pBd;
    UINT32 ulReserved[7];
} CTRL_PKT_BUFS, *PCTRL_PKT_BUFS;

typedef struct FifoParams {
    UINT32  ctl ;
    UINT32  epSize ;
} FIFO_PARAMS ;

/* The definition of the driver control structure */
#define MAX_USB_CONFIG_LEN   512
typedef struct usbnic_dev_context
{
	/* Linux driver fields. */
	struct	net_device *pDev;        
	struct 	net_device_stats DevStats;
#ifdef USE_BH
	struct 	tasklet_struct BhBulkRx;
	struct 	tasklet_struct BhRndisMsg;
#endif
	struct 	timer_list ReclaimTimer;
	IOCTL_MIB_INFO MibInfo;

	/* USB fields. */
	UINT16 	usPacketFilterBitmap;
	StringTable *pStrTbl;
	StringDesc StrDesc;
	UINT32 	ulHostDriverType;
	UINT32 	waitForRndisMessage;
	UINT32   ulHostVersion;
	UsbConfiguration *pUsbCfg;
	UINT32 	ulUsbCfgSize;
	UINT32 	ulFlags;
	UINT8 	ucPermanentHostMacAddr[ETH_ALEN];
	UINT8 	ucCurrentHostMacAddr[ETH_ALEN];
	// Static buffer for holding all the USB descriptors.  Depending on what mode we're
	// in, we'll copy either CDC or RNDIS descriptors into this buffer space at run-time.
	UINT8  	usbConfigBuffer[MAX_USB_CONFIG_LEN];

	// Points to start of address-adjusted descriptor buffer
	UINT8 	*usbConfigBufferBase;

	// These variables are pointers into the global configuration buffer that stores descriptors.
	// These are set at run-time, based on what mode we're operating in (RNDIS/CDC).
	UINT32	config1CfgDescOffset;
	UINT32 	config1IntEndptOffset;
	UINT32 	config1BlkEndptOffset;
	UINT32	config2CfgDescOffset;
	UINT32 	config2IntEndptOffset;
	UINT32 	config2BlkEndptOffset;
	//spin locks for SMP
	spinlock_t usbnic_lock_tx;
	spinlock_t usbnic_lock_rx;
	spinlock_t usbnic_lock_msg;
	spinlock_t usbnic_lock_UsbIsr;

	//SetCfg/Interface Values.
	UINT8    cfg ;
	UINT8    intf ;
	UINT8    altIntf ;
	UINT8    speed ;

	/* DMA fields. */
	volatile DmaRegs *pDmaRegs;
	volatile DmaChannelCfg *pRxDma;
	volatile DmaChannelCfg *pTxDma;
	volatile DmaChannelCfg *pRxCtrlDma;
	volatile DmaChannelCfg *pTxCtrlDma;
	volatile DmaChannelCfg *pTxIntrDma;
	char 		RxBds[(USB_NR_RX_BDS * sizeof(DmaDesc)) + 16];
	char 		TxBds[(USB_NR_TX_BDS * sizeof(DmaDesc)) + 16] ;
	char 		RxCtrlBds[(NR_CTRL_BDS * sizeof(DmaDesc)) + 16];
	char 		TxCtrlBds[(NR_CTRL_BDS * sizeof(DmaDesc)) + 16];
	char 		TxIntrBd[sizeof(DmaDesc) + 16];
	char 		RxCtrlPkts[(NR_CTRL_BDS * sizeof(CTRL_PKT_BUFS)) + 16];
	char 		TxCtrlPkts[(NR_CTRL_BDS * sizeof(CTRL_PKT_BUFS)) + 16];
	PCTRL_PKT_BUFS pRxCtrlPktsBase;
	PCTRL_PKT_BUFS pTxCtrlPktsBase;
	UINT32   txCtrlPktsIndex ;
	volatile DmaDesc *pRxCtrlBdBase;
	volatile DmaDesc *pRxCtrlBdNext;
	volatile DmaDesc *pTxCtrlBdBase;
	volatile DmaDesc *pTxCtrlBdCurr;
	volatile DmaDesc *pTxCtrlBdNext;
	volatile DmaDesc *pTxIntrBdBase;
	volatile DmaDesc *pRxBdBase;
	volatile DmaDesc *pRxBdAssign;
	volatile DmaDesc *pRxBdRead;
	volatile DmaDesc *pTxBdBase;
	volatile DmaDesc *pTxBdNext;
	char 		TxHdrs[(NR_TX_HDRS * sizeof(USB_TX_HDR)) + 16] ;
	PUSB_TX_HDR pTxHdrBase;
	PUSB_TX_HDR pTxHdrFreeHead;
	PUSB_TX_HDR pTxHdrFreeTail;
	PUSB_TX_HDR pTxHdrReclaimHead;
	PUSB_TX_HDR pTxHdrReclaimTail;
	struct 	sk_buff *pFreeSockBufList;
	UINT8 	SockBufs[(USB_NUM_RX_PKTS * SKB_ALIGNED_SIZE) + 0x10];
	char		*pRxBufs;

	/* Remote NDIS fields. */
	UINT32 	ulRndisState;
	UINT8 	ucRndisMulticastList[MAX_MULTICAST_ADDRESSES * ETH_ALEN];
	UINT8 	ucRndisNumMulticastListEntries;
	char	CtrlPacketAlign[MAX_CTRL_PKT_SIZE + 32];
	UINT8	*pCtrlPacketBuf;
} USBNIC_DEV_CONTEXT, *PUSBNIC_DEV_CONTEXT;

void UsbNicProcessCtrlMsg(PUSBNIC_DEV_CONTEXT, PCTRL_PKT_BUFS, UINT32) ;

#if defined __cplusplus
}
#endif

#endif
