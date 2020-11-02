/*
<:copyright-BRCM:2003:proprietary:standard

   Copyright (c) 2003 Broadcom 
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
 * File Name  : bcmusbdesc.c
 *
 * Description: This defines all the USB Descriptor structures. This includes the
 *              descriptors for the CDC Ethernet Networking Control model and
 *              the RNDIS model.
 *
 *              * Remote NDIS - Used on Windows with a Microsoft RNDIS host
 *                driver.
 *
 *              * USB Class Definitions for Communications Devices 1.1 
 *                (CDC 1.1) specification, Ethernet Control Model - Used
 *                on Windows with a Broadcom supplied host driver, Linux PCs
 *                and Macintosh.
 *                Only for 9636x platforms.
 *
 * Updates    : 06/15/2007  Srini.   Created for Linux.
 ***************************************************************************/


//********************** Include Files ***************************************

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/version.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <asm/io.h>
#include <linux/delay.h>

#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <linux/rtnetlink.h>
#include <linux/ethtool.h>

#include <asm/uaccess.h>

#include <bcmnet.h>
// For the portable types (uint32, etc).
#include <bcmtypes.h>
#include <bcm_map.h>
#include <bcm_intr.h>
#include <board.h>
#include "bcmrndis.h"
#include "bcmusb.h"


//********************** Local Types *****************************************

#define	BCD(val)	((val) % 256), ((val) / 256)

UsbConfiguration gUsbConfiguration = {
//********************** Local Constants *************************************

// USB Device Descriptor
// There is only one of these -- it's used for EnetControl Model or RNDIS Model
// or if both models are included.
//
// The number of configuration is subject to change at run-time, depending
// on if we are configuring both Enet Control and RNDIS models.

	/************* UsbDeviceDescriptor ****************/
	{
		0x12,                    // descriptor size in bytes
		DEVICE_DESC,             // descriptor type is 1 (DEVICE)
		BCD(0x0200),             // USB spec release number in BCD (byte swapped)
		kCommDevice,             // class code (2 = Communications Device)
		0x00,                    // subclass code
		0x00,                    // protocol code
		USB_CTL_EP_SIZE,	 // max size of Control Endpoint (size FIXED by USBD core!)
		BCD(0x0A5C),             // vendor ID (byte swapped) -- WILL BE REPLACED BY NON-VOL VALUE
		BCD(0x6300),             // product ID (byte swapped)  -- WILL BE REPLACED BY NON-VOL VALUE
		BCD(0x0101),             // device release number (BCD) (byte swapped)
		kVendorString,           // index into string table for manufacturer name
		kProductString,          // index into string table for product name
		kMacAddressString,       // index into string table for serial number
		0x02                     // number of configurations
	},

// USB Device Qualifier Descriptor
// Devices capable of more than one speed must support this descriptor
// Things that don't change between speeds of operation, ie, the
// vendor id, product id etc don't appear in this descriptor.

	/**************** UsbDevQualifierDescriptor ****************/
	{
		0x0A,                    // descriptor size in bytes
		DEVQUALIFIER_DESC,       // descriptor type is 1 (DEVICE)
		BCD(0x0200),             // USB spec release number in BCD (byte swapped)
		kCommDevice,             // class code (2 = Communications Device)
		0x00,                    // subclass code
		0x00,                    // protocol code
		USB_CTL_EP_SIZE,         // Max packet size of Control Endpoint (sized FIXED by USBD core!)
		0x02,                    // number of configurations
		0x00                     // Reserved
	},

// *********************************************************************
// RNDIS (Abstract Control Model) Descriptor Definitions
// This model is defined in the Microsoft RNDIS 1.1 spec.
// The descriptors are from the "Appendix B USB 802.3 Device Sample"
//
// Some of the interface numbers may be subject to change at run-time
// depending on if this is the only model in use, or if both this model
// and the Ethernet Networking Control model are in use.
// *********************************************************************

	/*************** Configuration Descriptor for RNDIS ***************/
	/*************** UsbConfigDesc ****************/
	{
		0x09,                    // descriptor size in bytes
		CONFIGURATION_DESC,      // descriptor type is 2 (CONFIGURATION)
		BCD(67),                 // size of all data returned for this config in bytes (byte swapped)
		0x02,                    // number of interfaces the configuration supports
		HOST_DRIVER_RNDIS,       // the id of this configuration (SUBJECT TO RUN-TIME CHANGE)
		0x04,                    // index into string table for configuration name (unused)
		0xc0,                    // bitmap; bit 7 always 1; bit 6 = self powered; bit 5 = supports remote wakeup
		0x00                     // bus power required (units of 2ma) (0x32 = 50 = 100mA)
	},

// Communication Class Interface descriptor for RNDIS model
	/*************** UsbInterfaceDesc ***************/
	{
		0x09,                         // descriptor size in bytes
		INTERFACE_DESC,               // descriptor type is 4 (INTERFACE)
		0x00,                         // number identifying this interface
		0x00,                         // value used to select an alternate setting
		0x01,                         // number of endpoints supported, except Endpoint 0
		kCommInterface,               // interface class code
		kAbstractControl,             // subclass code
		0xff,                         // protocol code (0xff = vendor specific)
		0x00,                         // index into string table for interface name (unused)
	},


//
// Class-Specific Interface Functional Descriptors for RNDIS (Abstract Control) Model
//
// Header Functional Descriptor for RNDIS
	/* UsbHeaderDesc */
	{
		{
			0x05,                         // descriptor size in bytes (5)
			CS_INTERFACE,                 // descriptor type is 0x24 (CS_INTERFACE)
			kHeaderFunc,                  // sub-type
			BCD(0x0110),                  // revision of USB CDC spec (byte swapped)
		},

		// The Call Management Func Descriptor for RNDIS.
		/* UsbCallManagementDesc */
		{
			0x05,                         // descriptor size in bytes (5)
			CS_INTERFACE,                 // descriptor type is 0x24 (CS_INTERFACE)
			kCallManagementFunc,          // sub-type
			0x00,                         // bitmap of capabilities -- set to 0
			0x00                          // data interface (ignored if bmCapabilities is 0)
		},

		// The Abstract Control Magagement Func Descriptor for RNDIS.
		/* UsbAbstractControlDesc */
		{
			0x04,                         // descriptor size in bytes (4)
			CS_INTERFACE,                 // descriptor type is 0x24 (CS_INTERFACE)
			kAbstractControlFunc,         // sub-type is 2 (Abstract Control)
			0x00                          // bitmap of capabilities -- set to 0
		},

		// Union Functional Descriptor for CDC Ethernet Control Model
		/* UsbUnionDesc */
		{
			0x05,                         // descriptor size in bytes (5)
			CS_INTERFACE,                 // descriptor type is 0x24 (CS_INTERFACE)
			kUnionFunc,                   // sub-type
			0x00,                         // set to bInterfaceNumber of the Comm Class Interface
			0x01                          // set to bInterfaceNumber of the Data Class Interface
		}
	},


// Endpoint Descriptor for RNDIS Comm Class Interface
	/*  Notification Endpoint */
	/* UsbEndPointDesc */
	{
		0x07,                         // descriptor size in bytes
		ENDPOINT_DESC,                // descriptor type is 5 (ENDPOINT)
		0x83,                         // endpoint number/direction (IN) (3 is chip default for interrupt EP)
		kInterrupt,                   // transfer type supported
		BCD(USB_HS_MAX_INT), 	      // max packet size supported (byte swapped)
		USB_HS_INT_RNDIS_BINTERVAL    // max latency/polling interval/NAK rate
	},

// Data Class Interface descriptor for RNDIS
	/* UsbInterfaceDesc */
	{
		0x09,                         // descriptor size in bytes
		INTERFACE_DESC,               // descriptor type is 4 (INTERFACE)
		0x01,                         // number identifying this interface
		0x00,                         // value used to select an alternate setting
		0x02,                         // number of endpoints supported, except Endpoint 0
		kDataInterface,               // interface class code
		0x00,                         // subclass code
		0x00,                         // protocol code (0 = no class specific protocol required)
		0x06,                         // index into string table for interface name (unused)
	},

// Endpoint Descriptor for RNDIS
// IN -- Bulk Data from Modem
	/* UsbEndPointDescriptor */
	{
		0x07,                         // descriptor size in bytes
		ENDPOINT_DESC,                // descriptor type is 5 (ENDPOINT)
		0x81,                         // endpoint number/direction (IN) (1 is chip default for BULK IN EP)
		kBulk,                        // transfer type supported
		BCD(USB_HS_MAX_BULK),	      // max packet size
		0x00                          // bInterval ignored for bulk IN endpoints
	},

// Endpoint Descriptor for RNDIS
// OUT -- Bulk Data to Modem
	/* UsbEndPointDescriptor */
	{
		0x07,                         // descriptor size in bytes
		ENDPOINT_DESC,                // descriptor type is 5 (ENDPOINT)
		0x02,                         // endpoint number/direction (OUT) (2 is chip default for BULK OUT EP)
		kBulk,                        // transfer type supported
		BCD(USB_HS_MAX_BULK),
		USB_HS_BOUT_BINTERVAL         // give a NAK poll period of 12.5usec for HS
	},

// *********************************************************************
// Ethernet Networking Control Model Descriptor Definitions
// This model is defined in the CDC 1.1 spec.
//
// Some of the interface numbers may be subject to change at run-time
// depending on if this is the only model in use, or if both this model
// and the RNDIS model are in use.
// *********************************************************************

// Configuration Descriptor for Ethernet Networking Control Model
	/* UsbConfigurationDesc */
	{
		0x09,                         // descriptor size in bytes
		CONFIGURATION_DESC,           // descriptor type is 2 (CONFIGURATION)
		BCD(80),                      // size of all data returned for this config in bytes (byte swapped)
		0x02,                         // number of interfaces the configuration supports
		HOST_DRIVER_CDC,              // the id of this configuration (SUBJECT TO RUN-TIME CHANGE)
		kConfigString,                // index into string table for configuration name
		0xc0,                         // bitmap; bit 7 always 1; bit 6 = self powered; bit 5 = supports remote wakeup
		0x00                          // bus power required (units of 2ma) (0x32 = 50 = 100mA)
	},

// Communication Class Interface descriptor, Intf 0, Alt 0 -- defined by CDC 1.10
// Used for Ethernet Networking Control Model
	/* UsbInterfaceDesc */
	{
		0x09,                         // descriptor size in bytes
		INTERFACE_DESC,               // descriptor type is 4 (INTERFACE)
		0x00,                         // number identifying this interface
		0x00,                         // value used to select an alternate setting
		0x01,                         // number of endpoints supported, except Endpoint 0
		kCommInterface,               // interface class code
		kEthernetControl,             // subclass code
		0x00,                         // protocol code (0 = no class specific protocol required)
		kInterface1String,            // index into string table for interface name
	},

//
// Class-Specific Interface Functional Descriptors for CDC Ethernet Control Model
	{
		//
		// Header Functional Descriptor for CDC Ethernet Control Model
		/* UsbHeaderDesc */
		{
			0x05,                         // descriptor size in bytes (5)
			CS_INTERFACE,                 // descriptor type is 0x24 (CS_INTERFACE)
			kHeaderFunc,                  // sub-type
			BCD(0x0110),                  // revision of USB CDC spec (byte swapped)
		},

		// Union Functional Descriptor for CDC Ethernet Control Model
		/* UsbUnionDesc */
		{
			0x05,                         // descriptor size in bytes (5)
			CS_INTERFACE,                 // descriptor type is 0x24 (CS_INTERFACE)
			kUnionFunc,                   // sub-type
			0x00,                         // set to bInterfaceNumber of the Comm Class Interface
			0x01                          // set to bInterfaceNumber of the Data Class Interface
		},

		// Ethernet Networking Functional Descriptor
		/* UsbEnetNetworkingDesc */
		{
			0x0d,                         // descriptor size in bytes (13)
			CS_INTERFACE,                 // descriptor type is 0x24 (CS_INTERFACE)
			kEthernetNetFunc,             // sub-type
			kMacAddressString,            // index into string table for mac address
			0x00, 0x00, 0x00, 0x00,       // bitmap -- 0 means keep count of Ethernet statistic
			BCD(1514),                    // max segment size Ethernet device can support (byte swapped)
			BCD(0),                       // number of multicast filters that can be configured (byte swapped)
			0x00                          // number of power filters that can cause wake-up of host
		}
	},

// Endpoint Descriptor for Ethernet Networking Model Comm Class Interface
// Notification Endpoint
	/* UsbEndPointDesc */
	{
		0x07,                         // descriptor size in bytes
		ENDPOINT_DESC,                // descriptor type is 5 (ENDPOINT)
		0x83,                         // endpoint number/direction (IN) (7 is chip default for interrupt EP)
		kInterrupt,                   // transfer type supported
		BCD(USB_HS_MAX_INT),
		USB_HS_INT_CDC_BINTERVAL      // max latency/polling interval/NAK rate	(64msec)
	},


// Data Class Interface descriptor, Intf 1, Alt 0 -- defined by CDC 1.10
// There are no extra endpoints associated with this interface
// Used for Ethernet Networking Control Model
	/* UsbInterfaceDesc */
	{
		0x09,                         // descriptor size in bytes
		INTERFACE_DESC,               // descriptor type is 4 (INTERFACE)
		0x01,                         // number identifying this interface
		0x00,                         // value used to select an alternate setting
		0x00,                         // number of endpoints supported, except Endpoint 0
		kDataInterface,               // interface class code
		0x00,                         // subclass code
		0x00,                         // protocol code (0 = no class specific protocol required)
		kInterface2String,            // index into string table for interface name
	},

// Data Class Interface descriptor, Intf 1, Alt 1 -- defined by CDC 1.10
// Used for Ethernet Networking Control Model
	/* UsbInterfaceDesc */
	{
		0x09,                         // descriptor size in bytes
		INTERFACE_DESC,               // descriptor type is 4 (INTERFACE)
		0x01,                         // number identifying this interface
		0x01,                         // value used to select an alternate setting
		0x02,                         // number of endpoints supported, except Endpoint 0
		kDataInterface,               // interface class code
		0x00,                         // subclass code
		0x00,                         // protocol code (0 = no class specific protocol required)
		kInterface2String,            // index into string table for interface name
	},

// Endpoint Descriptor for Ethernet Networking Model Comm Class Interface
// IN -- Bulk Data from Modem
	/* UsbEndPointDesc */
	{
		0x07,                         // descriptor size in bytes
		ENDPOINT_DESC,                // descriptor type is 5 (ENDPOINT)
		0x81,                         // endpoint number/direction (IN) (1 is chip default for BULK IN EP)
		kBulk,                        // transfer type supported
		BCD(USB_HS_MAX_BULK),
		0x00                          // bInterval ignored for bulk IN endpoints
	},

// Endpoint Descriptor for Ethernet Networking Model Comm Class Interface
// OUT -- Bulk Data to Modem
	/* UsbEndPointDesc */
	{
		0x07,                         // descriptor size in bytes
		ENDPOINT_DESC,                // descriptor type is 5 (ENDPOINT)
		0x02,                         // endpoint number/direction (OUT) (2 is chip default for BULK OUT EP)
		kBulk,                        // transfer type supported
		BCD(USB_HS_MAX_BULK),
		USB_HS_BOUT_BINTERVAL         // bInterval for HS bulk OUT endpoint 12.5usec
	}
};

// *********************************************************************
// String Definitions
// *********************************************************************

// string descriptors (by index), index 0 = LANGID
// THESE MUST MATCH THE ORDER in the enum UsbStringIndex table.
StringTable gUsbStringTable[STRING_TABLE_SIZE] = {
	// Language ID is 16 bits -- top 6 bits (bits 15..10) are the Sublanguage ID
	// The bottom 10 bits (bits 9..0) are the Primary Language ID.
	// Primary ID for English is 9 ( b:0000001001)
	// Sub ID for US English is 1 ( b:000001)
	// Combined, it makes 0x0409 -- byte swap it here for little endian
	/* */
	{{BCD(0x0409), 0}},     /* LANGID - English = 0x0409 (or 0x0109 (old)) */
	{"Broadcom Corporation"},
	{"USB Network Interface"},
	{"001018000000"},  /* serial number */
	{"USB Ethernet Configuration"},
	{"Communication Interface Class"},
	{"Data Interface Class"}
};
