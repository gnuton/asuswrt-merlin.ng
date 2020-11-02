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
 * File Name  : bcmusb.c
 *
 * Description: This file contains the implementation for a USB
 *              network interface driver that executes on the BCM963xx
 *              reference platforms.  It requires one of the following host
 *              drivers.
 *
 *              * Remote NDIS - Used on Windows with a Microsoft RNDIS host
 *                driver.
 *
 *              * USB Class Definitions for Communications Devices 1.1 
 *                (CDC 1.1) specification, Ethernet Control Model - Used
 *                on Windows with a Broadcom supplied host driver, Linux PCs
 *                and Macintosh.
 *
 ***************************************************************************/

/* Defines. */
#define CARDNAME    "BCM63XX_USB"
#define VERSION     "0.4a"
#define VER_STR     "v" VERSION

/* Includes. */
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
#include <bcmtypes.h>
#include <bcm_mm.h>
#include <bcm_map.h>
#include <bcm_intr.h>
#include <board.h>
#include <boardparms.h>
#include <bcmnetlink.h>
#include "bcmrndis.h"
#include "bcmusb.h"
#include <linux/nbuff.h>


#if !defined(LOCAL)
#define LOCAL static
#endif

/* Undefine LOCAL for debugging. */
#if defined(LOCAL)
#undef LOCAL
#define LOCAL
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)
#define NETDEV_PRIV(x)      (PUSBNIC_DEV_CONTEXT)netdev_priv(x)
#else
#define NETDEV_PRIV(x)      (PUSBNIC_DEV_CONTEXT)x->priv
#endif

/* Globals. */
PUSBNIC_DEV_CONTEXT g_pDevCtx = NULL;
UINT8 nullMsg = '\0';

/* Externs. */
extern int kerSysGetMacAddress(unsigned char *pucaMacAddr, unsigned long ulId);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
extern FN_HANDLER_RT RndisCtrlRxIsr(int irq, void *dev_id);
#else
extern void RndisCtrlRxIsr( int irq, void *dev_id, struct pt_regs *regs );
#endif
extern void RndisDoGetEncapsulatedResponse( PUSBNIC_DEV_CONTEXT pDevCtx,
    UINT16 usLength );
extern void RndisProcessMsg( PUSBNIC_DEV_CONTEXT pDevCtx );

void usb_setup_txrx_fifo (int Speed) ;
void  usb_set_csr_endpoints (int cfg, int intf, int altIntf, int speed) ;

extern UsbConfiguration  gUsbConfiguration ;
extern StringTable gUsbStringTable[] ;

/* Prototypes. */
#if defined(MODULE)
LOCAL
#endif
int __init bcmusbnic_probe( void );
LOCAL int UsbNicInit( PUSBNIC_DEV_CONTEXT pDevCtx, int initFlag );
LOCAL void SetMacAddr( char *pStrBuf, UINT8 *pucMacAddr );

// These functions prepare for enumeration by a host by copying the different descriptors
// into the global configuration buffer.
LOCAL void PrepareUsbDescriptors (PUSBNIC_DEV_CONTEXT pDevCtx) ;
LOCAL UINT16 PrepareUsbConfigurationBuffer (PUSBNIC_DEV_CONTEXT pDevCtx, UINT8 * bytePtr, UINT8 numBytes);
LOCAL void ConcatEnetControlModelDescriptors (PUSBNIC_DEV_CONTEXT pDevCtx, UsbConfiguration *pUsbCfg);
LOCAL void ConcatRndisModelDescriptors (PUSBNIC_DEV_CONTEXT pDevCtx, UsbConfiguration *pUsbCfg);

LOCAL void UsbNicEnable( volatile DmaRegs *pDmaRegs,
    volatile DmaChannelCfg *pRxDma, volatile DmaChannelCfg *pTxDma,
    volatile DmaChannelCfg *pRxCtrlDma, 
    volatile DmaChannelCfg *pTxCtrlDma, 
    volatile DmaChannelCfg *pTxIntrDma, unsigned int ulParam) ;
LOCAL void bcmusbnic_cleanup(void);
LOCAL void UsbNicDisable( volatile DmaChannelCfg *pRxDma,
    volatile DmaChannelCfg *pTxDma );
LOCAL int bcmusbnic_open( struct net_device *dev );
LOCAL int bcmusbnic_close(struct net_device *dev);
LOCAL void bcmusbnic_timeout(struct net_device *dev);
LOCAL struct net_device_stats *bcmusbnic_query(struct net_device *dev);
LOCAL int bcmusbnic_ioctl(struct net_device *dev, struct ifreq *rq, int cmd);
LOCAL void usbnic_recycle_skb_or_data(struct sk_buff *skb,
    PUSBNIC_DEV_CONTEXT pDevCtx, UINT32 nFlag);
LOCAL void usbnic_recycle(pNBuff_t pNBuff,
    PUSBNIC_DEV_CONTEXT pDevCtx, UINT32 nFlag);
LOCAL int bcmusbnic_xmit(pNBuff_t pNBuff, struct net_device *dev);
LOCAL void usbnic_timer( PUSBNIC_DEV_CONTEXT pDevCtx );
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
LOCAL FN_HANDLER_RT bcmBulkRxIsr(int irq, void *dev_id);
LOCAL FN_HANDLER_RT bcmUsbIsr(int irq, void *dev_id);
#else
LOCAL void bcmBulkRxIsr(int irq, void * dev_id, struct pt_regs * regs);
LOCAL void bcmUsbIsr(int irq, void *dev_id, struct pt_regs *regs);
#endif
VOID UsbProcessConfigDescRequest (PUSBNIC_DEV_CONTEXT pDevCtx, UINT8 desc,
											 UINT32 length, UINT8 index);
LOCAL void usbnic_bulkrx( PUSBNIC_DEV_CONTEXT pDevCtx );
LOCAL void ConvertAnsiToUnicode( char *pszOut, char *pszIn );
void UsbNicSubmitIntrPacket( PUSBNIC_DEV_CONTEXT pDevCtx, UINT8 *pBuf,
    UINT32 ulLength);
void UsbNicSubmitControlPacket( PUSBNIC_DEV_CONTEXT pDevCtx, UINT8 *pBuf,
    UINT32 ulLength, UINT8 advanceCurr);
/* Ethtool support */
LOCAL int netdev_ethtool_ioctl(struct net_device *dev, void *useraddr);
LOCAL UINT16 configBufferOffset;
LOCAL UINT8 scfg, sintf, saltIntf, sSpeed = 0xFF;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)
static const struct net_device_ops bcmUsb_netdev_ops = {
    .ndo_open               = bcmusbnic_open,
    .ndo_stop               = bcmusbnic_close,
    .ndo_start_xmit         = (HardStartXmitFuncP)bcmusbnic_xmit,
    .ndo_do_ioctl           = bcmusbnic_ioctl,
    .ndo_tx_timeout         = bcmusbnic_timeout,
    .ndo_get_stats          = bcmusbnic_query
};
#endif



/*******************************************************************************
*
* bcmusbnic_probe
*
* Initial driver entry point.
*
* RETURNS: 0 - success, < 0 - error
*/

#if defined(MODULE)
LOCAL
#endif
int __init bcmusbnic_probe( void )
{
    static int nProbed = 0;
    int nStatus = 0;
    unsigned int GPIOOverlays;

    if ( SKB_ALIGNED_SIZE != skb_aligned_size() )
    {
        PRINTK("skb_aligned_size mismatch. Need to recompile usb module\n");
        return -ENOMEM;
    }

    PRINTK("Entering bcmusbnic_probe \n") ;

    if( BpGetGPIOverlays(&GPIOOverlays) == BP_SUCCESS ) {
        if (!(GPIOOverlays & BP_OVERLAY_USB_DEVICE)) {
            printk(KERN_DEBUG CARDNAME" USB Device not present\n");
            return -ENODEV;
        }
    }

    if( nProbed == 0 )
    {
        PUSBNIC_DEV_CONTEXT pDevCtx = NULL;
        struct net_device *dev = NULL;
        UINT16 usChipId;
        UINT16 usChipRev;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)
#else
        UINT8 *p;
#endif

        nProbed = 1;

        usChipId  = (PERF->RevID & CHIP_ID_MASK) >> CHIP_ID_SHIFT;
        usChipRev = (PERF->RevID & REV_ID_MASK);

        /* Allocate device context structure */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)
        /* Initialize this device as a network device. */
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 0, 0)
        if ((dev = alloc_netdev(sizeof(USBNIC_DEV_CONTEXT) + 32, "usb%d", ether_setup)) != NULL)
#else
        if ((dev = alloc_netdev(sizeof(USBNIC_DEV_CONTEXT) + 32, "usb%d", NET_NAME_UNKNOWN, ether_setup)) != NULL)
#endif
	{
            nStatus = 0;
        } else {
            printk(KERN_ERR CARDNAME": alloc_netdev failed\n");
            nStatus = -ENOMEM;
        }

        if( dev == NULL )
            nStatus = -ENOMEM;

        if( nStatus == 0 )
        {
            g_pDevCtx = pDevCtx = (PUSBNIC_DEV_CONTEXT) netdev_priv(dev);
            memset( pDevCtx, 0x00, sizeof(USBNIC_DEV_CONTEXT) );
            pDevCtx->pStrTbl = &gUsbStringTable[0] ;
            pDevCtx->pUsbCfg = &gUsbConfiguration ;
            pDevCtx->ulUsbCfgSize = sizeof(UsbConfiguration);
            pDevCtx->usPacketFilterBitmap = NDIS_PACKET_TYPE_MULTICAST |
                NDIS_PACKET_TYPE_BROADCAST | NDIS_PACKET_TYPE_DIRECTED |
                NDIS_PACKET_TYPE_ALL_MULTICAST | NDIS_PACKET_TYPE_PROMISCUOUS;
            pDevCtx->ulFlags = 0;
            pDevCtx->ulHostDriverType = HOST_DRIVER_CDC; /* default value */
            pDevCtx->waitForRndisMessage = FALSE ;

            spin_lock_init(&pDevCtx->usbnic_lock_tx);
            spin_lock_init(&pDevCtx->usbnic_lock_rx); 
            spin_lock_init(&pDevCtx->usbnic_lock_msg);
            spin_lock_init(&pDevCtx->usbnic_lock_UsbIsr);

            PRINTK ("\n ########### HOST Driver Type = CDC ############ \n") ;

            /* Print the chip id and module version. */
            printk("Broadcom BCM%X%X USB Network Device ", usChipId, usChipRev);
            printk(VER_STR "\n");


            /* Initialize a timer that calls a function to free transmit
             * buffers.
             */
            init_timer(&pDevCtx->ReclaimTimer);
            pDevCtx->ReclaimTimer.data = (unsigned long)pDevCtx;
            pDevCtx->ReclaimTimer.function = (void *) usbnic_timer;

            /* Initialize bottom half for handling data received on the bulk
             * endpoint and RNDIS messages.
             */
#ifdef USE_BH
            tasklet_init(&(pDevCtx->BhBulkRx), (void *) usbnic_bulkrx, (unsigned long) pDevCtx);
            tasklet_init(&(pDevCtx->BhRndisMsg), (void *) RndisProcessMsg, (unsigned long) pDevCtx);
#endif

        }
#else // LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)

		p = (UINT8 *) kmalloc( sizeof(USBNIC_DEV_CONTEXT) + 32, GFP_KERNEL );

        if( p == NULL )
        nStatus = -ENOMEM;

        if( nStatus == 0 )
        {
            g_pDevCtx = pDevCtx = (PUSBNIC_DEV_CONTEXT) p;
            memset( pDevCtx, 0x00, sizeof(USBNIC_DEV_CONTEXT) );
            pDevCtx->pStrTbl = &gUsbStringTable[0] ;
            pDevCtx->pUsbCfg = &gUsbConfiguration ;
            pDevCtx->ulUsbCfgSize = sizeof(UsbConfiguration);
            pDevCtx->usPacketFilterBitmap = NDIS_PACKET_TYPE_MULTICAST |
                NDIS_PACKET_TYPE_BROADCAST | NDIS_PACKET_TYPE_DIRECTED |
                NDIS_PACKET_TYPE_ALL_MULTICAST | NDIS_PACKET_TYPE_PROMISCUOUS;
            pDevCtx->ulFlags = 0;
            pDevCtx->ulHostDriverType = HOST_DRIVER_CDC; /* default value */
            pDevCtx->waitForRndisMessage = FALSE ;

            PRINTK ("\n ########### HOST Driver Type = CDC ############ \n") ;

            /* Print the chip id and module version. */
            printk("Broadcom BCM%X%X USB Network Device ", usChipId, usChipRev);
            printk(VER_STR "\n");

            /* Initialize a timer that calls a function to free transmit
             * buffers.
             */
            init_timer(&pDevCtx->ReclaimTimer);
            pDevCtx->ReclaimTimer.data = (unsigned long)pDevCtx;
            pDevCtx->ReclaimTimer.function = (void *) usbnic_timer;

            /* Initialize bottom half for handling data received on the bulk
             * endpoint and RNDIS messages.
             */
#ifdef USE_BH
            tasklet_init(&(pDevCtx->BhBulkRx), (void *) usbnic_bulkrx, (unsigned long) pDevCtx);
            tasklet_init(&(pDevCtx->BhRndisMsg), (void *) RndisProcessMsg, (unsigned long) pDevCtx);
#endif

            /* Initialize this device as a network device. */
            if ((dev = alloc_netdev(sizeof(*pDevCtx), "usb%d", ether_setup)) != NULL) {
                nStatus = 0;
            } else {
                printk(KERN_ERR CARDNAME": alloc_netdev failed\n");
                nStatus = -ENOMEM;
            }
        }
#endif // else LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)

        if( nStatus == 0 )
        {
            dev_alloc_name(dev, dev->name);
            
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)
#else
            dev->priv = pDevCtx;
#endif
            pDevCtx->pDev = dev;

            /* Read and display the USB network device MAC address. */
            dev->dev_addr[0] = 0xff;
            kerSysGetMacAddress( dev->dev_addr, dev->ifindex + 50 );
            if( (dev->dev_addr[0] & 0x01) == 0x01 )
            {
                printk( KERN_ERR CARDNAME": Unable to read MAC address from "
                    "persistent storage.  Using default address.\n" );
                memcpy( dev->dev_addr, "\x40\x10\x18\x02\x00\x01", 6 );
            }

            printk("%s: MAC Address: %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X\n",
                dev->name, dev->dev_addr[0], dev->dev_addr[1], dev->dev_addr[2],
                dev->dev_addr[3], dev->dev_addr[4], dev->dev_addr[5] );

            kerSysGetMacAddress( pDevCtx->ucPermanentHostMacAddr,
                dev->ifindex + 100 );

            if( (pDevCtx->ucPermanentHostMacAddr[0] & 0x01) == 0x01 )
            {
                printk( KERN_ERR CARDNAME": Unable to read host MAC address "
                    "from persistent storage.  Using default address.\n" );
                memcpy( pDevCtx->ucPermanentHostMacAddr,
                    "\x40\x10\x18\x82\x00\x01", ETH_ALEN );
            }

            memcpy( pDevCtx->ucCurrentHostMacAddr,
                pDevCtx->ucPermanentHostMacAddr, ETH_ALEN );

            printk("%s: Host MAC Address: %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X\n",
                dev->name, pDevCtx->ucPermanentHostMacAddr[0],
                pDevCtx->ucPermanentHostMacAddr[1],
                pDevCtx->ucPermanentHostMacAddr[2],
                pDevCtx->ucPermanentHostMacAddr[3],
                pDevCtx->ucPermanentHostMacAddr[4],
                pDevCtx->ucPermanentHostMacAddr[5] );

            /* Initialize buffer descriptors. */
            nStatus = UsbNicInit( pDevCtx, USBD_INIT );

#if 0 /* DEBUG */
            printk("USBDBG: pDevCtx=0x%8.8lx\n", (UINT32) pDevCtx );
            printk("USBDBG: pRxBdBase=0x%8.8lx\n", (UINT32) pDevCtx->pRxBdBase);
            printk("USBDBG: pTxBdBase=0x%8.8lx\n", (UINT32) pDevCtx->pTxBdBase);
            printk("USBDBG: pRxCtrlBdBase=0x%8.8lx\n", (UINT32) pDevCtx->pRxCtrlBdBase);
            //printk("USBDBG: pCtrlPktsBase=0x%8.8lx\n", (UINT32) pDevCtx->pCtrlPktsBase);
            //printk("USBDBG: &pCtrlPktsBase=0x%8.8lx\n", (UINT32) &pDevCtx->pCtrlPktsBase);
            printk("USBDBG: &pRxDma=0x%8.8lx\n", (UINT32) (UINT32) pDevCtx->pRxDma);
            printk("USBDBG: &pTxDma=0x%8.8lx\n", (UINT32) (UINT32) pDevCtx->pTxDma);
            printk("USBDBG: &pRxCtrlDma=0x%8.8lx\n", (UINT32) (UINT32) pDevCtx->pRxCtrlDma);
            printk("USBDBG: &pTxCtrlDma=0x%8.8lx\n", (UINT32) pDevCtx->pTxCtrlDma);
            printk("USBDBG: &pTxIntrDma=0x%8.8lx\n", (UINT32) pDevCtx->pTxIntrDma) ;
#endif
        }

        if( nStatus == 0 )
        {


            dev->watchdog_timeo     = USB_TIMEOUT;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)


            dev->netdev_ops = &bcmUsb_netdev_ops;
#else
            /* Setup the callback functions. */
            dev->open               = bcmusbnic_open;
            dev->stop               = bcmusbnic_close;
            dev->hard_start_xmit    = (HardStartXmitFuncP) bcmusbnic_xmit;
            dev->tx_timeout         = bcmusbnic_timeout;
            dev->get_stats          = bcmusbnic_query;
            dev->set_multicast_list = NULL;
            dev->do_ioctl           = &bcmusbnic_ioctl;
#endif
            netdev_path_set_hw_port(dev, 0, BLOG_USBPHY);

            /* Don't reset or enable the device yet. "Open" does that. */
            nStatus = register_netdev(dev);
            if (nStatus != 0) 
            {
                printk(KERN_ERR CARDNAME": register_netdev failed\n");
                kfree(pDevCtx);
                free_netdev(dev);
            }
        }
        else
            if( pDevCtx )
                kfree(pDevCtx);
    }
    else
        nStatus = -ENXIO;

	//printk ("Exiting bcmusbnic_probe status = %d \n", nStatus) ;
    return( nStatus );
} /* bcmusbnic_probe */


void usb_setup_txrx_fifo (int Speed)
{
   FIFO_PARAMS txFifo [3], rxFifo [3] ;
   int i ;

      // Initialize the TX/RX FIFO configuration registers
      // Tx/RxFifo0 Start & Stop addresses (Ctrl Endpoint 0) & Sizes.

   if (Speed == 0) { /* Hi-Speed */
      PRINTK ("usb_setup_txrx_fifo(): HS %d \n", Speed) ;
      txFifo [0].ctl    = 0x001F0000 ;
      txFifo [0].epSize = USB_CTL_EP_SIZE ;
      rxFifo [0].ctl    = 0x001F0000 ;
      rxFifo [0].epSize = USB_CTL_EP_SIZE ;

      // Tx/RxFifo1 Start & Stop addresses (Bulk Endpoints 1,2) & Sizes.
      txFifo [1].ctl    = 0x009f0020 ;
      txFifo [1].epSize = USB_HS_MAX_BULK ;
      rxFifo [1].ctl    = 0x009f0020 ;
      rxFifo [1].epSize = USB_HS_MAX_BULK ;

      // Tx/RxFifo2 Start & Stop addresses (Interrupt Endpoints 3,4) & Sizes.
      txFifo [2].ctl    = 0x00BF00A0 ;
      txFifo [2].epSize = USB_HS_MAX_INT ;
      rxFifo [2].ctl    = 0x00BF00A0 ;
      rxFifo [2].epSize = USB_HS_MAX_INT ;
   } 
   else { /* Full Speed */
      PRINTK ("usb_setup_txrx_fifo(): FS %d \n", Speed) ;
      txFifo [0].ctl    = 0x001F0000 ;
      txFifo [0].epSize = USB_CTL_EP_SIZE ;
      rxFifo [0].ctl    = 0x001F0000 ;
      rxFifo [0].epSize = USB_CTL_EP_SIZE ;

      // Tx/RxFifo1 Start & Stop addresses (Bulk Endpoints 1,2) & Sizes.
      txFifo [1].ctl    = 0x009f0020 ;
      txFifo [1].epSize = USB_FS_MAX_BULK ;
      rxFifo [1].ctl    = 0x009f0020 ;
      rxFifo [1].epSize = USB_FS_MAX_BULK ;

      // Tx/RxFifo2 Start & Stop addresses (Interrupt Endpoints 3,4) & Sizes.
      txFifo [2].ctl    = 0x00Bf00A0 ;
      txFifo [2].epSize = USB_FS_MAX_INT ;
      rxFifo [2].ctl    = 0x00Bf00A0 ;
      rxFifo [2].epSize = USB_FS_MAX_INT ;
   }

   /* Setup Fifo Start/Stop/EPSize pointers */
	for (i = 0; i < 3; i++)
	{
	   USB->usbd_control       = USBD_CONTROL_APP_FIFO_INIT_SEL(i) ;
	   USB->usbd_txfifo_config = txFifo[i].ctl ;
	   USB->usbd_rxfifo_config = rxFifo[i].ctl ; 
	   USB->usbd_txfifo_epsize = txFifo[i].epSize ; 
	   USB->usbd_rxfifo_epsize = rxFifo[i].epSize ; 
	}
}

void  usb_set_csr_endpoints (int cfg, int intf, int altIntf, int speed)
{
   int ctlPktsz, bulkPktSz, intrPktSz ;

      PRINTK ("usb_set_csr_endpoints -cfg:intf:altIntf:speed = %d:%d:%d:%d \n", cfg, intf, altIntf, speed) ;

      ctlPktsz  = USB_CTL_EP_SIZE;
      bulkPktSz = USB_FS_MAX_BULK;

      if (speed == 0)
	intrPktSz = USB_HS_MAX_INT;
      else
	intrPktSz = USB_FS_MAX_INT;

   /* Default initializations for CSRs */
   USB->usbd_csr_setupaddr = 0x0000b550 ;    /* Some hardcoded value */

   USB->usbd_csr_ep [0] = USBD_EPNUM(0) | USBD_EPDIR_OUT | USBD_EPTYP_CTRL |
                          USBD_EPCFG(cfg) | USBD_EPINTF (intf) | 
                          USBD_EPAINTF (altIntf) | USBD_EPMAXPKT (ctlPktsz) ;

   USB->usbd_csr_ep [1] = USBD_EPNUM(1) | USBD_EPDIR_IN | USBD_EPTYP_BULK |
                          USBD_EPCFG(cfg) | USBD_EPINTF (intf) | 
                          USBD_EPAINTF (altIntf) | USBD_EPMAXPKT (bulkPktSz) ;

   USB->usbd_csr_ep [2] = USBD_EPNUM(2) | USBD_EPDIR_OUT | USBD_EPTYP_BULK |
                          USBD_EPCFG(cfg) | USBD_EPINTF (intf) |
                          USBD_EPAINTF (altIntf) | USBD_EPMAXPKT (bulkPktSz) ;

   USB->usbd_csr_ep [3] = USBD_EPNUM(3) | USBD_EPDIR_IN | USBD_EPTYP_IRQ |
                          USBD_EPCFG(cfg) | USBD_EPINTF (intf) |
                          USBD_EPAINTF (altIntf) | USBD_EPMAXPKT (intrPktSz) ;

   USB->usbd_csr_ep [4] = USBD_EPNUM(4) | USBD_EPDIR_OUT | USBD_EPTYP_IRQ |
                          USBD_EPCFG(cfg) | USBD_EPINTF (intf) |
                          USBD_EPAINTF (altIntf) | USBD_EPMAXPKT (intrPktSz) ;
   return ;
}


/*******************************************************************************
*
* UsbNicInit
*
* This routine initializes resources used by the USB NIC driver.
*
* RETURNS: 0 - success, < 0 - error.
*/

LOCAL int UsbNicInit( PUSBNIC_DEV_CONTEXT pDevCtx, int initFlag )
{
   int nStatus = 0;
   UINT32 i, ulLen;
   volatile DmaDesc *pBd;
   UINT8 *p, *pData;
   UINT32 epConfig [5] ;
   volatile DmaChannelCfg *pRxDma, *pTxDma;
   PUSB_TX_HDR pTxHdr;
   PCTRL_PKT_BUFS pCtrlPkt;
   UINT8 *pSockBuf;
   UINT16 usVid = (UINT16) USB_VENDOR_ID;
   UINT16 usPid = (UINT16) USB_DSL_PRODUCT_ID;
   unsigned long irqFlags;

   PRINTK ("UsbNicInit init %d \n", initFlag) ;

   spin_lock_irqsave(&pDevCtx->usbnic_lock_UsbIsr, irqFlags);

   /* Make sure USB clock is on. Some of the USBH block pwr,clk also need be turned on for USBD to run here
    * in case USB host is not enabled in the profile. */
   PERF->blkEnables |= USBD_CLK_EN | USBH_CLK_EN ;

#if defined(CONFIG_BCM963268)
   PERF->softResetB |= SOFT_RST_USBD | SOFT_RST_USBH;
   TIMER->ClkRstCtl |= USB_REF_CLKEN;
   MISC->miscIddqCtrl &= ~MISC_IDDQ_CTRL_USBH;
   MISC->miscIddqCtrl &= ~MISC_IDDQ_CTRL_USBD;
   mdelay(100);
   USBH->PllControl1 &= ~(PLLC_PLL_IDDQ_PWRDN | PLLC_PLL_PWRDN_DELAY);
#endif

#if defined(CONFIG_BCM963268)
   USBH->GenericControl |= GC_PLL_SUSPEND_EN;
#endif

   /* Enable USB Device on port 2  - Default is Host */
   USBH->SwapControl |= USB_DEVICE_SEL ;



   // Initialize all the strapping options for the USB Core. The defaults
   // are mostly what we want.
#ifdef HIGH_SPEED
   USB->usbd_straps = (USBD_STRAPS_APP_SELF_PWR | USBD_STRAPS_APP_RAM_IF |
         USBD_STRAPS_APP_CSRPRG_SUP | USBD_STRAPS_APP_PHYIF_8BIT |
         USBD_STRAPS_HIGH_SPEED) | APPUTMIDIR(UNIDIR) ;
#else
   USB->usbd_straps = (USBD_STRAPS_APP_SELF_PWR | USBD_STRAPS_APP_RAM_IF |
         USBD_STRAPS_APP_CSRPRG_SUP | USBD_STRAPS_APP_PHYIF_8BIT |
         USBD_STRAPS_FULL_SPEED | APPUTMIDIR(UNIDIR)) ;
#endif

   epConfig [0] = USBD_EPNUM_EPTYPE(USBD_EPNUM_CTRL) | USBD_EPNUM_EPDMACHMAP(0) ;
   epConfig [1] = USBD_EPNUM_EPTYPE(USBD_EPNUM_BULK) | USBD_EPNUM_EPDMACHMAP(1) ;
   epConfig [2] = USBD_EPNUM_EPTYPE(USBD_EPNUM_BULK) | USBD_EPNUM_EPDMACHMAP(1) ;
   epConfig [3] = USBD_EPNUM_EPTYPE(USBD_EPNUM_IRQ)  | USBD_EPNUM_EPDMACHMAP(2) ;
   epConfig [4] = USBD_EPNUM_EPTYPE(USBD_EPNUM_IRQ)  | USBD_EPNUM_EPDMACHMAP(2) ;

   // default to starting in full speed; the PHY returns a mode that's either
   // HS or FS on FS handshake, so don't set HS 'til the PHY actually has it
   pDevCtx->speed = 3 ;
   usb_setup_txrx_fifo (pDevCtx->speed) ;

   /* Reuse init_sel to setup EPNUM/TYPE mapping */
   for (i = 0; i < 5; i++)
   {
      USB->usbd_control       = USBD_CONTROL_APP_FIFO_INIT_SEL(i) ;
      USB->usbd_epnum_typemap = epConfig [i] ;
   }

   pDevCtx->cfg = 1 ;
   pDevCtx->intf = 0 ;
   pDevCtx->altIntf = 0 ;

   usb_set_csr_endpoints (pDevCtx->cfg, pDevCtx->intf, pDevCtx->altIntf, pDevCtx->speed) ;

   // Initialize each endpoint's Tx/Rx Fifo by resetting them
   for (i = 0; i < 3; i++)
   {
      USB->usbd_control = USBD_CONTROL_APP_FIFO_INIT_SEL(i) ;
      USB->usbd_control |= USBD_CONTROL_APP_AUTO_CSRS ;
      USB->usbd_control |= EN_TXZLENINS | APPSETUPERRLOCK | 0x000010C0 ;  // Reset each Rx/Tx FIFO
   }

   spin_unlock_irqrestore(&pDevCtx->usbnic_lock_UsbIsr, irqFlags);

   ulLen = (USB_NUM_RX_PKTS * RX_USB_ALLOC_TOTAL_SIZE) + 16;

   if (initFlag == USBD_INIT) {
	  if( (pDevCtx->pRxBufs = (char *) kmalloc(ulLen, GFP_KERNEL|GFP_DMA)) == NULL ) {
         printk ("USB : No memory for pDevCtx->pRxBufs, size : %x \n", ulLen) ;
         nStatus = -ENOMEM;
      }
   }

   if (initFlag == USBD_REINIT) {
      /* Initialize RX ring pointer variables. */
      for( i = 0, pBd = pDevCtx->pRxBdBase,
            p = (unsigned char *) USB_NIC_ALIGN(pDevCtx->pRxBufs, 16);
            i < USB_NR_RX_BDS; i++, pBd++, p += RX_USB_ALLOC_TOTAL_SIZE )
      {
         if (pBd->address == 0)  {
            nStatus = -EAGAIN ;
            break ;
         }
      }
   }

   if (nStatus == 0) {

      /* Make BDs paragraph (16 byte) aligned in uncached memory */
      p = (UINT8 *) KSEG1ADDR(&pDevCtx->RxBds[0]);
      pDevCtx->pRxBdBase = (DmaDesc *) USB_NIC_ALIGN(p, 16);
      pDevCtx->pRxBdAssign = pDevCtx->pRxBdRead = pDevCtx->pRxBdBase;

      p = (UINT8 *) KSEG1ADDR(&pDevCtx->TxBds[0]);
      pDevCtx->pTxBdBase = (DmaDesc *) USB_NIC_ALIGN(p, 16) ;
      pDevCtx->pTxBdNext = pDevCtx->pTxBdBase;

      p = (UINT8 *) KSEG1ADDR(&pDevCtx->RxCtrlBds[0]);
      pDevCtx->pRxCtrlBdBase = (DmaDesc *) USB_NIC_ALIGN(p, 16);
      pDevCtx->pRxCtrlBdNext = pDevCtx->pRxCtrlBdBase;

      p = (UINT8 *) &pDevCtx->RxCtrlPkts[0];
      pDevCtx->pRxCtrlPktsBase = (PCTRL_PKT_BUFS) USB_NIC_ALIGN(p, 16);

      p = (UINT8 *) KSEG1ADDR(&pDevCtx->TxCtrlBds[0]);
      pDevCtx->pTxCtrlBdBase = (DmaDesc *) USB_NIC_ALIGN(p, 16);

      spin_lock_irqsave(&pDevCtx->usbnic_lock_UsbIsr, irqFlags);
      pDevCtx->pTxCtrlBdCurr = pDevCtx->pTxCtrlBdNext = pDevCtx->pTxCtrlBdBase;
      spin_unlock_irqrestore(&pDevCtx->usbnic_lock_UsbIsr, irqFlags);

      p = (UINT8 *) &pDevCtx->TxCtrlPkts[0];
      pDevCtx->pTxCtrlPktsBase = (PCTRL_PKT_BUFS) USB_NIC_ALIGN(p, 16);

      p = (UINT8 *) KSEG1ADDR(&pDevCtx->TxIntrBd);
      pDevCtx->pTxIntrBdBase = (DmaDesc *) USB_NIC_ALIGN(p, 16);

      p = (UINT8 *) KSEG1ADDR(&pDevCtx->CtrlPacketAlign);
      pDevCtx->pCtrlPacketBuf = (UINT8 *) USB_NIC_ALIGN(p, 16);

      dma_cache_wback_inv((unsigned long) pDevCtx->pRxBufs, ulLen);

      /* Initialize RX ring pointer variables. */
      for( i = 0, pBd = pDevCtx->pRxBdBase,
            p = (unsigned char *) USB_NIC_ALIGN(pDevCtx->pRxBufs, 16);
            i < USB_NR_RX_BDS; i++, pBd++, p += RX_USB_ALLOC_TOTAL_SIZE )
      {
         fkb_preinit(p, (RecycleFuncP)usbnic_recycle, (UINT32)pDevCtx);
         pBd->status = DMA_OWN;
         pBd->length = RX_USB_NIC_BUF_SIZE;
         pData = PFKBUFF_TO_PDATA(p, RX_USB_SKB_RESERVE);
         pBd->address = (UINT32) VIRT_TO_PHYS(pData);
      }

      pDevCtx->pRxBdBase[USB_NR_RX_BDS - 1].status |= DMA_WRAP;

      /* Initialize TX ring pointer variables. */
      for( i = 0, pBd = pDevCtx->pTxBdBase; i < USB_NR_TX_BDS; i++, pBd++ )
      {
         pBd->status = 0;
         pBd->length = 0;
         pBd->address = 0;
      }

      pDevCtx->pTxBdBase[USB_NR_TX_BDS - 1].status = DMA_WRAP;

      /* Initialize RX control endpoint ring pointer variables. */
      for( i = 0, pBd = pDevCtx->pRxCtrlBdBase,
            pCtrlPkt = pDevCtx->pRxCtrlPktsBase;
            i < NR_CTRL_BDS; i++, pBd++, pCtrlPkt++ )
      {
         pBd->status = DMA_OWN;
         pBd->length = MAX_CTRL_PKT_SIZE;
         pBd->address = (UINT32) VIRT_TO_PHYS(pCtrlPkt);
         pCtrlPkt->pBd = pBd;
      }

      pDevCtx->pRxCtrlBdBase[NR_CTRL_BDS - 1].status |= DMA_WRAP;

      /* Initialize TX control endpoint ring pointer variables. */
      for( i = 0, pBd = pDevCtx->pTxCtrlBdBase,
            pCtrlPkt = pDevCtx->pTxCtrlPktsBase;
            i < NR_CTRL_BDS; i++, pBd++, pCtrlPkt++ )
      {

         pBd->status = 0;
         pBd->length = MAX_CTRL_PKT_SIZE;
         pBd->address = (UINT32) VIRT_TO_PHYS(pCtrlPkt);
         pCtrlPkt->pBd = pBd;
         PRINTK ("BD : %u  %x BD->address : %x \n", i, (unsigned) pBd, (unsigned) pCtrlPkt) ;
      }

      pDevCtx->pTxCtrlBdBase[NR_CTRL_BDS - 1].status |= DMA_WRAP;

      /* Make the transmit header structures long word aligned */
      p = (char *) pDevCtx->TxHdrs;
      pDevCtx->pTxHdrBase = (PUSB_TX_HDR) USB_NIC_ALIGN(p, 16) ;

      /* Chain USB_TX_HDR structures together. */
      for(i = 0, pTxHdr = pDevCtx->pTxHdrBase; i < NR_TX_HDRS-1; i++, pTxHdr++)
      {
         pTxHdr->pNext = pTxHdr + 1;
      }

      /* Chain socket buffers. */
      for( i = 0, pSockBuf = (unsigned char *)
            (((unsigned long) pDevCtx->SockBufs + 0x0f) & ~0x0f);
            i < USB_NUM_RX_PKTS; i++, pSockBuf += SKB_ALIGNED_SIZE )
      {
         ((struct sk_buff *)pSockBuf)->next_free = pDevCtx->pFreeSockBufList;
         pDevCtx->pFreeSockBufList = (struct sk_buff *) pSockBuf;
      }

      pDevCtx->pTxHdrBase[NR_TX_HDRS - 1].pNext = NULL;

      pDevCtx->pTxHdrFreeHead = pDevCtx->pTxHdrBase; /* first tx header */
      pDevCtx->pTxHdrFreeTail = pTxHdr; /* last tx header */

      pDevCtx->pTxHdrReclaimHead = NULL;
      pDevCtx->pTxHdrReclaimTail = NULL;

      /* Disable interrupts. */
      BcmHalInterruptDisable(INTERRUPT_ID_USB_BULK_RX_DMA);
      BcmHalInterruptDisable(INTERRUPT_ID_USB_CNTL_RX_DMA);
      BcmHalInterruptDisable(INTERRUPT_ID_USBS);

      /* Set USB Vendor ID and Product ID. */
      pDevCtx->pUsbCfg->deviceDesc.idVendor_lo = (usVid >> 8) & 0xFF ;
      pDevCtx->pUsbCfg->deviceDesc.idVendor_hi = (usVid & 0xFF) ;
      pDevCtx->pUsbCfg->deviceDesc.idProduct_lo = (usPid >> 8) & 0xFF ;
      pDevCtx->pUsbCfg->deviceDesc.idProduct_hi = (usPid & 0xFF) ;

      SetMacAddr( pDevCtx->pStrTbl[SERIAL_NUM_INDEX].string,
            (char *) pDevCtx->ucPermanentHostMacAddr);

      pDevCtx->usbConfigBufferBase = &pDevCtx->usbConfigBuffer[0] ;

      PrepareUsbDescriptors(pDevCtx) ;

      /*------------------------------------------------------------------
       * initialize the DMA channels
       *------------------------------------------------------------------*/

      /* clear State RAM */
      pDevCtx->pDmaRegs = (DmaRegs *) USB_DMA_BASE;
      memset( (char *) &pDevCtx->pDmaRegs->stram.s[FIRST_USB_DMA_CHANNEL],
            0x00, sizeof(DmaStateRam) * NR_USB_DMA_CHANNELS );

      /* Bulk endpoint DMA initialization. */
      pDevCtx->pRxDma = pRxDma = pDevCtx->pDmaRegs->chcfg + USB_BULK1_RX_CHAN;
      pDevCtx->pTxDma = pTxDma = pDevCtx->pDmaRegs->chcfg + USB_BULK1_TX_CHAN;

      /* bulk endpoint transmit */
      pTxDma->cfg = 0;    /* initialize first (will enable later) */
      pTxDma->maxBurst = USB_DMA_MAX_BURST_LENGTH;
      pTxDma->intStat = DMA_DONE | DMA_NO_DESC | DMA_BUFF_DONE; /* clear */
      pTxDma->intMask = DMA_DONE | DMA_NO_DESC ; /* configure */
      pDevCtx->pDmaRegs->stram.s[USB_BULK1_TX_CHAN].baseDescPtr =
         (UINT32) VIRT_TO_PHYS(pDevCtx->pTxBdBase);

      /* bulk endpoint receive */
      pRxDma->cfg = 0;    /* initialize first (will enable later) */
      pRxDma->maxBurst = BULK_RX_DMA_MAX_BURST_LEN;
      pRxDma->intStat = DMA_DONE | DMA_NO_DESC | DMA_BUFF_DONE; /* clear */
      pRxDma->intMask = DMA_DONE | DMA_NO_DESC; /* configure */
      pDevCtx->pDmaRegs->stram.s[USB_BULK1_RX_CHAN].baseDescPtr =
         (UINT32) VIRT_TO_PHYS(pDevCtx->pRxBdBase);

      /* Control endpoint DMA initialization. */
      pDevCtx->pRxCtrlDma=pRxDma=pDevCtx->pDmaRegs->chcfg + USB_CNTL_RX_CHAN;
      pDevCtx->pTxCtrlDma=pTxDma=pDevCtx->pDmaRegs->chcfg + USB_CNTL_TX_CHAN;

      /* control endpoint transmit */
      pTxDma->cfg = 0;    /* initialize first (will enable later) */
      pTxDma->maxBurst = USB_DMA_MAX_BURST_LENGTH;
      pTxDma->intStat = DMA_DONE;
      pTxDma->intMask = 0;  /* mask all ints */
      pDevCtx->pDmaRegs->stram.s[USB_CNTL_TX_CHAN].baseDescPtr =
         (UINT32) VIRT_TO_PHYS(pDevCtx->pTxCtrlBdBase);

      /* control endpoint receive */
      pRxDma->cfg = 0;    /* initialize first (will enable later) */
      pRxDma->maxBurst = USB_DMA_MAX_BURST_LENGTH;
      pRxDma->intStat = DMA_DONE | DMA_NO_DESC | DMA_BUFF_DONE;
      pRxDma->intMask = 0;  /* mask all ints */
      pDevCtx->pDmaRegs->stram.s[USB_CNTL_RX_CHAN].baseDescPtr =
         (UINT32) VIRT_TO_PHYS(pDevCtx->pRxCtrlBdBase);

      /* Interrupt endpoint DMA initialization. */
      pDevCtx->pTxIntrDma=pTxDma=pDevCtx->pDmaRegs->chcfg + USB_IRQ_TX_CHAN;

      /* Interrupt endpoint transmit */
      pTxDma->cfg = 0;    /* initialize first (will enable later) */
      pTxDma->maxBurst = USB_DMA_MAX_BURST_LENGTH;
      pTxDma->intStat = DMA_DONE;
      pTxDma->intMask = 0;  /* mask all ints */
      pDevCtx->pDmaRegs->stram.s[USB_IRQ_TX_CHAN].baseDescPtr =
         (UINT32) VIRT_TO_PHYS(pDevCtx->pTxIntrBdBase);

      /* Enable USB to send and receive data. */
      UsbNicEnable( pDevCtx->pDmaRegs, pDevCtx->pRxDma, pDevCtx->pTxDma,
            pDevCtx->pRxCtrlDma, pDevCtx->pTxCtrlDma,
            pDevCtx->pTxIntrDma, (unsigned int) pDevCtx) ;

      if (initFlag == USBD_INIT) {
         /* Start a timer that reclaims transmitted packets. */
         pDevCtx->ReclaimTimer.expires = jiffies + USB_TIMEOUT;
         add_timer(&pDevCtx->ReclaimTimer);
      }
   }

   printk ("USBD %s done status %d \n", ((initFlag == USBD_INIT) ? "Initialization" : "ReInitialization"), nStatus) ;

   return( nStatus );

} /* UsbNicInit */


/******************************************************************************
*
* SetMacAddr
*
* This routine copies a MAC address as an ANSI string.
*
* RETURNS: None.
*/

LOCAL void SetMacAddr( char *pStrBuf, UINT8 *pucMacAddr )
{
    char szHexChars[] = "0123456789abcdef";
    UINT8 ch;
    int i;
    for( i = 0; i < MAC_ADDR_LEN; i++ )
    {
        ch = pucMacAddr[i];
        *pStrBuf++ = szHexChars[ch >> 4];
        *pStrBuf++ = szHexChars[ch & 0x0f];
    }
    *pStrBuf = '\0'; /* null terminate string  */
} /* SetMacAddr */


// Organizes the various USB descriptors into a full configuration buffer
// to be written to the chip. The order and number of descriptors used
// depends on the CDC control models supported (Ethernet Netorking Control
// Model and/or RNDIS (Abstract Control Model)).
//
// This is the order that things are done to get a particular configuration
// into the chip:
//
//  InitUsb()
//  |
//  -> PrepareUsbDescriptors()  calls one or both, depending on models enabled
//  |  |
//  |  -> ConcatEnetControlModelDescriptors()  (if standard model enabled)
//  |  |  |   (either one or both of these Concat... funcs is called)
//  |  -> ConcatRndisModelDescriptors()        (if RNDIS model enabled)
//  |     |
//  |     -> PrepareUsbConfigurationBuffer()  (for each descriptor in model)
//  |         This function does the actual concatenation of all the relevant
//  |         descriptors into our local configuration buffer.
//  |
//
// Parameters:  none
//
// Returns: nothing
//
//
LOCAL void PrepareUsbDescriptors ( PUSBNIC_DEV_CONTEXT pDevCtx )
{
	UsbConfiguration  *pUsbCfg = pDevCtx->pUsbCfg ;

	spin_lock_bh(&pDevCtx->usbnic_lock_msg);
	memset (pDevCtx->usbConfigBuffer, 0, MAX_USB_CONFIG_LEN);     // clear out buffer
	configBufferOffset = 0;
	pDevCtx->ulHostDriverType = HOST_DRIVER_CDC; /* default value */
	pDevCtx->waitForRndisMessage = FALSE ;

	// We do this in order to support legacy USB devices in the Windows 98 and before.
	// Win 98 doesnt' support RNDIS.  However, we are required to work on a Win 98 host
	// if our nonvol settings are configured for RNDIS operation.  To support this fairly
	// obtuse requirement, we jump through the following hoops:
	// We store both sets of descriptors (RNDIS & CDC) in the global buffer with
	// RNDIS as config 1 and CDC as config 2.  When we enumerate with a host, we'll
	// first send it 9 bytes of the config descriptor so that it can tell how long
	// the config is and how many configs we support.  On a pre-Win 98 host, our NDIS
	// driver will ask for the second configuration so that we'll automagically switch
	// to CDC mode even though we were configured to work in RNDIS.  If, on the other hand,
	// we enumerate with a host that supports RNDIS, we'll set this flag to true when
	// we see the first RNDIS message on the CONTROL channel.

	PrepareUsbConfigurationBuffer (pDevCtx, (uint8 *) &pUsbCfg->deviceDesc, sizeof (UsbDeviceDesc));

	pUsbCfg->rndis_configDesc.bConfigValue = HOST_DRIVER_RNDIS;  // first config
	pUsbCfg->cdc_configDesc.bConfigValue = HOST_DRIVER_CDC;   // second config

	ConcatRndisModelDescriptors (pDevCtx, pUsbCfg);
	ConcatEnetControlModelDescriptors (pDevCtx, pUsbCfg);
	spin_unlock_bh(&pDevCtx->usbnic_lock_msg);

	PRINTK("Initial USB Config Buffer Start\n\n");
	DUMPADDR(pDevCtx->usbConfigBuffer, MAX_USB_CONFIG_LEN);
	PRINTK("Initial USB Config Buffer End\n");
}

// Copies the descriptor data for the RNDIS Model into the config buffer.
// THESE MUST BE DONE IN A PARTICULAR ORDER. See the specification for the chip.
//
// Parameters:  none
//
// Returns:     nothing
//
LOCAL void ConcatRndisModelDescriptors (PUSBNIC_DEV_CONTEXT pDevCtx,
		                                  UsbConfiguration *pUsbCfg)
{
  pDevCtx->config1CfgDescOffset = configBufferOffset;
  PrepareUsbConfigurationBuffer (pDevCtx, (uint8 *) &pUsbCfg->rndis_configDesc, sizeof (UsbConfigDesc));
  PrepareUsbConfigurationBuffer (pDevCtx, (uint8 *) &pUsbCfg->rndis_interfaceDesc, sizeof (UsbInterfaceDesc));
  PrepareUsbConfigurationBuffer (pDevCtx, (uint8 *) &pUsbCfg->rndis_comm_classDesc, sizeof (RNDIS_classDesc));
  pDevCtx->config1IntEndptOffset = configBufferOffset;
  PrepareUsbConfigurationBuffer (pDevCtx, (uint8 *) &pUsbCfg->rndis_endpoint_notificationDesc, sizeof (UsbEndpointDesc));
  PrepareUsbConfigurationBuffer (pDevCtx, (uint8 *) &pUsbCfg->rndis_dataClassDesc, sizeof (UsbInterfaceDesc));
  pDevCtx->config1BlkEndptOffset = configBufferOffset;
  PrepareUsbConfigurationBuffer (pDevCtx, (uint8 *) &pUsbCfg->rndis_endpoint_bulkDataInDesc, sizeof (UsbEndpointDesc));
  PrepareUsbConfigurationBuffer (pDevCtx, (uint8 *) &pUsbCfg->rndis_endpoint_bulkDataOutDesc, sizeof (UsbEndpointDesc));
}

// Copies the descriptor data for the Enet Control Model into the config buffer.
//
// Parameters:  none
//
// Returns:     nothing
//
// #ifdef 3255A0_CDC
LOCAL void ConcatEnetControlModelDescriptors (PUSBNIC_DEV_CONTEXT pDevCtx,
		                                        UsbConfiguration *pUsbCfg)
{
  pDevCtx->config2CfgDescOffset = configBufferOffset;
  PrepareUsbConfigurationBuffer (pDevCtx, (uint8 *) &pUsbCfg->cdc_configDesc, sizeof (UsbConfigDesc));
  PrepareUsbConfigurationBuffer (pDevCtx, (uint8 *) &pUsbCfg->cdc_interfaceDesc, sizeof (UsbInterfaceDesc));
  PrepareUsbConfigurationBuffer (pDevCtx, (uint8 *) &pUsbCfg->cdc_comm_classDesc, sizeof (COMM_classDesc));
  pDevCtx->config2IntEndptOffset = configBufferOffset;
  PrepareUsbConfigurationBuffer (pDevCtx, (uint8 *) &pUsbCfg->cdc_endpoint_notificationDesc, sizeof (UsbEndpointDesc));
  PrepareUsbConfigurationBuffer (pDevCtx, (uint8 *) &pUsbCfg->cdc_dataClassDesc, sizeof (UsbInterfaceDesc));
  PrepareUsbConfigurationBuffer (pDevCtx, (uint8 *) &pUsbCfg->cdc_dataClassDescAlt, sizeof (UsbInterfaceDesc));
  pDevCtx->config2BlkEndptOffset = configBufferOffset;
  PrepareUsbConfigurationBuffer (pDevCtx, (uint8 *) &pUsbCfg->cdc_endpoint_bulkDataInDesc, sizeof (UsbEndpointDesc));
  PrepareUsbConfigurationBuffer (pDevCtx, (uint8 *) &pUsbCfg->cdc_endpoint_bulkDataOutDesc, sizeof (UsbEndpointDesc));
}

// Copies the passed USB descriptors into the master USB configuration
// buffer (max of 512 bytes). This concatenates the descriptors together
// into a byte array.
//
// Parameters:
//      bytePtr = pointer to USB descriptor to concatenate into buffer
//      numBytes = number of bytes in descriptor
//
// Returns:
//      number of bytes written so far
//
LOCAL UINT16 PrepareUsbConfigurationBuffer (PUSBNIC_DEV_CONTEXT pDevCtx, UINT8 * bytePtr,
		                                      UINT8 numBytes)
{
  if (configBufferOffset >= MAX_USB_CONFIG_LEN)
  {
    printk ("\nError in PrepareUsbConfigurationBuffer() -- Configuration data exceeds allowed length.\n");
    return configBufferOffset;
  }

  if ((configBufferOffset + numBytes) >= MAX_USB_CONFIG_LEN)
  {
    printk ("\nError in PrepareUsbConfigurationBuffer() -- Configuration data will exceed allowed length.\n");
    return configBufferOffset;
  }

  while (numBytes--)
  {
    pDevCtx->usbConfigBuffer[configBufferOffset++] = *bytePtr++;
  }

  return configBufferOffset;
}


/******************************************************************************
*
* UsbNicEnable
*
* This routine enables the USB device.
*
* RETURNS: None.
*/

LOCAL void UsbNicEnable( volatile DmaRegs *pDmaRegs,
    volatile DmaChannelCfg *pRxDma, volatile DmaChannelCfg *pTxDma,
    volatile DmaChannelCfg *pRxCtrlDma, 
    volatile DmaChannelCfg *pTxCtrlDma, 
    volatile DmaChannelCfg *pTxIntrDma, unsigned int ulParam)
{
	unsigned long irqFlags;
	
	spin_lock_irqsave(&g_pDevCtx->usbnic_lock_UsbIsr, irqFlags);
	// Clear the status/event bits that tell us if we're suspended etc.
	USB->usbd_status = 0x00000000;

	USB->usbd_events_irq = 0;  /* clr any pending interrupts then set mask */
	spin_unlock_irqrestore(&g_pDevCtx->usbnic_lock_UsbIsr, irqFlags);

	//printk ("USB : Perf->IrqMask Before = %x \n", PERF->IrqMask) ;

	/* Register the interrupt service handlers and enable interrupts. */
	BcmHalInterruptDisable( INTERRUPT_ID_USB_BULK_RX_DMA );
	BcmHalInterruptDisable( INTERRUPT_ID_USB_CNTL_RX_DMA );
	BcmHalInterruptDisable( INTERRUPT_ID_USBS );

	BcmHalMapInterrupt( bcmBulkRxIsr, (void*)ulParam,
			INTERRUPT_ID_USB_BULK_RX_DMA );
	BcmHalMapInterrupt( RndisCtrlRxIsr, (void*)ulParam,
			INTERRUPT_ID_USB_CNTL_RX_DMA );
	BcmHalMapInterrupt( bcmUsbIsr, (void*)ulParam, INTERRUPT_ID_USBS );

	BcmHalInterruptEnable( INTERRUPT_ID_USB_BULK_RX_DMA );
	BcmHalInterruptEnable( INTERRUPT_ID_USB_CNTL_RX_DMA );
	BcmHalInterruptEnable( INTERRUPT_ID_USBS );

	//printk ("USB : Perf->IrqMask After = %x \n", PERF->IrqMask) ;

	/* Enable DMA on the control enpoint. */
	//USB->endpt_cntl &= ~(USB_RX_CNTL_DMA_EN | USB_TX_CNTL_DMA_EN);

	/* Configure DMA channels and enable Rx. */
	pTxDma->intStat = DMA_DONE | DMA_NO_DESC | DMA_BUFF_DONE;
	pTxDma->intMask = DMA_DONE;
	//pTxDma->cfg = DMA_ENABLE;

	pDmaRegs->controller_cfg |= DMA_MASTER_EN;

	pRxDma->intStat = DMA_DONE | DMA_NO_DESC | DMA_BUFF_DONE;
	pRxDma->intMask = DMA_DONE | DMA_NO_DESC;
	pRxDma->cfg = DMA_ENABLE;

	pRxCtrlDma->intStat = DMA_DONE | DMA_NO_DESC | DMA_BUFF_DONE;
	pRxCtrlDma->intMask = DMA_DONE;
	pRxCtrlDma->cfg = DMA_ENABLE;

	//pTxCtrlDma->intStat = DMA_DONE | DMA_NO_DESC | DMA_BUFF_DONE;
	//pTxCtrlDma->intMask = DMA_DONE;
	//pTxCtrlDma->cfg = DMA_ENABLE;

	//pTxIntrDma->intStat = DMA_DONE | DMA_NO_DESC | DMA_BUFF_DONE;
	//pTxIntrDma->intMask = DMA_DONE;
	//pTxIntrDma->cfg = DMA_ENABLE;

	spin_lock_irqsave(&g_pDevCtx->usbnic_lock_UsbIsr, irqFlags);
	/* Clear any pending interrupts then set mask. */
	USB->usbd_irqcfg_hi |= FALLING(ENUM_ON)  | FALLING(SET_CSRS) ;
	USB->usbd_irqcfg_lo |= RISING(USB_RESET) | RISING(HST_SETCFG) | FALLING(USB_LINK) ;

	USB->usbd_events_irq_mask = USBD_USB_RESET_IRQ | USBD_USB_SUSPEND_IRQ
		                         | USBD_USB_SETUP_IRQ
										 | USBD_USB_ENUM_ON_IRQ
                               | USBD_USB_SETCFG_IRQ
                               | USBD_USB_SET_CSRS_IRQ
                               | USBD_USB_LINK_IRQ ;

	spin_unlock_irqrestore(&g_pDevCtx->usbnic_lock_UsbIsr, irqFlags);
	//printk ("USB : Perf->IrqMask Exit = %x \n", PERF->IrqMask) ;
} /* UsbNicEnable */


/*******************************************************************************
*
* bcmusbnic_cleanup
*
* Called when the driver is no longer used.
*
* RETURNS: None.
*/

LOCAL void bcmusbnic_cleanup(void)
{
    PUSBNIC_DEV_CONTEXT pDevCtx = g_pDevCtx;

    if( pDevCtx && pDevCtx->pDev && pDevCtx->pDev->name[0] != '\0' )
    {
        PUSB_TX_HDR pReclaimHead = pDevCtx->pTxHdrReclaimHead;

        del_timer_sync(&pDevCtx->ReclaimTimer);

        UsbNicDisable( pDevCtx->pRxDma, pDevCtx->pTxDma );

        // release the memory
        if (pDevCtx->pRxBufs)
            kfree(pDevCtx->pRxBufs);
        
        /* No need to check MOD_IN_USE, as sys_delete_module() checks. */
        unregister_netdev( pDevCtx->pDev );

        while( pReclaimHead )
        {
            nbuff_free(pReclaimHead->pNBuff);
            pReclaimHead = pReclaimHead->pNext;
        }

        kfree( pDevCtx);
    }
} /* bcmusbnic_cleanup */


/******************************************************************************
*
* UsbNicDisable
*
* This routine disables the USB device.
*
* RETURNS: None.
*/

LOCAL void UsbNicDisable( volatile DmaChannelCfg *pRxDma,
    volatile DmaChannelCfg *pTxDma )
{
    unsigned long irqFlags;
	
    /* Disable interrupts. */
    spin_lock_irqsave(&g_pDevCtx->usbnic_lock_UsbIsr, irqFlags);
    USB->usbd_events_irq_mask = 0;
    spin_unlock_irqrestore(&g_pDevCtx->usbnic_lock_UsbIsr, irqFlags);

    /* Stop DMA channels. */
    pTxDma->intMask = 0;
    pRxDma->intMask = 0;
} /* UsbNicDisable */


/*******************************************************************************
*
* bcmusbnic_open
*
* Makes the USB device operational.  Called due to the shell command,
* "ifconfig ethX up"
*
* RETURNS: None.
*/

LOCAL int bcmusbnic_open( struct net_device *dev )
{
    PUSBNIC_DEV_CONTEXT pDevCtx = NETDEV_PRIV(dev);
    unsigned long irqFlags;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
#else
    MOD_INC_USE_COUNT;
#endif

    spin_lock_irqsave(&pDevCtx->usbnic_lock_UsbIsr, irqFlags);
    pDevCtx->ulFlags |= USB_FLAGS_OPENED;
    spin_unlock_irqrestore(&pDevCtx->usbnic_lock_UsbIsr, irqFlags);

    /* Start the network engine. */
    netif_start_queue(dev);

    return( 0 );
} /* bcmusbnic_open */



/*******************************************************************************
*
* bcmusbnic_close
*
* Stops the USB device.  Called due to the shell command,
* "ifconfig ethX down"
*
* RETURNS: None.
*/

LOCAL int bcmusbnic_close(struct net_device *dev)
{
    PUSBNIC_DEV_CONTEXT pDevCtx = NETDEV_PRIV(dev);
    unsigned long irqFlags;

    netif_stop_queue(dev);

    spin_lock_irqsave(&pDevCtx->usbnic_lock_UsbIsr, irqFlags);
    pDevCtx->ulFlags &= ~USB_FLAGS_OPENED;
    spin_unlock_irqrestore(&pDevCtx->usbnic_lock_UsbIsr, irqFlags);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
#else
    MOD_DEC_USE_COUNT;
#endif

    return( 0 );
} /* bcmusbnic_close */


/******************************************************************************
*
* bcmusbnic_timeout
*
* This routine processes a transmit timeout. 
*
* RETURNS: None.
*/

LOCAL void bcmusbnic_timeout(struct net_device *dev)
{
    /* Someone thinks we stalled. Now what??? */
    printk(KERN_WARNING CARDNAME ": transmit timed out\n");

    dev->trans_start = jiffies;

    netif_wake_queue(dev);
} /* bcmusbnic_timeout */


/******************************************************************************
*
* bcmusbnic_query
*
* This routine returns the device statistics structure.
*
* RETURNS: device statistics structure
*/

LOCAL struct net_device_stats *bcmusbnic_query(struct net_device *dev)
{
    PUSBNIC_DEV_CONTEXT pDevCtx = NETDEV_PRIV(dev);
    return( &pDevCtx->DevStats );
} /* bcmusbnic_query */


/* Ethtool support */
LOCAL int netdev_ethtool_ioctl(struct net_device *dev, void *useraddr)
{
    /* PUSBNIC_DEV_CONTEXT pDevCtx = dev->priv; */
    u32 ethcmd;

    if (copy_from_user(&ethcmd, useraddr, sizeof(ethcmd)))
       return -EFAULT;

    switch (ethcmd) {
    /* get driver-specific version/etc. info */
    case ETHTOOL_GDRVINFO: {
        struct ethtool_drvinfo info = {ETHTOOL_GDRVINFO};

        strncpy(info.driver, CARDNAME, sizeof(info.driver)-1);
        strncpy(info.version, VERSION, sizeof(info.version)-1);
        if (copy_to_user(useraddr, &info, sizeof(info)))
           return -EFAULT;
        return 0;
        }
    /* get settings */
    case ETHTOOL_GSET: {
        struct ethtool_cmd ecmd = { ETHTOOL_GSET };

        ecmd.speed = SPEED_10;
 
        if (copy_to_user(useraddr, &ecmd, sizeof(ecmd)))
           return -EFAULT;
        return 0;
        }
    }

    return -EOPNOTSUPP;    
}

/******************************************************************************
*
* bcmusbnic_ioctl
*
* This routine return nonzero if link up, otherwise return 0
*
* RETURNS: 0 = link down, !0 = link up
*/
LOCAL int bcmusbnic_ioctl(struct net_device *dev, struct ifreq *rq, int cmd)
{
    PUSBNIC_DEV_CONTEXT pDevCtx = NETDEV_PRIV(dev);
    int *data=(int*)rq->ifr_data;
    int status;

    /* we can add sub-command in ifr_data if we need to in the future */
    switch (cmd)
    {
    case SIOCGLINKSTATE:
        if( (pDevCtx->ulFlags & USB_FLAGS_PLUGGED_IN) != 0 )
            status = LINKSTATE_UP;
        else
            status = LINKSTATE_DOWN;
        if (copy_to_user((void*)data, (void*)&status, sizeof(int)))
            return -EFAULT;
        break;

    case SIOCSCLEARMIBCNTR:
        memset(&pDevCtx->DevStats, 0, sizeof(struct net_device_stats));
        break;

    case SIOCMIBINFO:
        if (copy_to_user((void*)data, (void*)&pDevCtx->MibInfo,
            sizeof(pDevCtx->MibInfo)))
        {
            return -EFAULT;
        }
        break;

    case SIOCETHTOOL:
        return netdev_ethtool_ioctl(dev, (void *) rq->ifr_data);

    }

    return 0;
} /* bcmusbnic_ioctl */


/*******************************************************************************
*
* UsbEthernetPacketFilter - Filter out packets before sending
*
* This routine uses a filter mask passed in the SET_ETHERNET_PACKET_FILTER
* class command to filter out some types of packets.
*
* RETURNS: 1 = filter packet, 0 = do not filter packet
*/

LOCAL inline int UsbNicEthernetPacketFilter( UINT16 usPacketFilterBitmap,
    UINT8 *pDestAddr, UINT8 *pMacAddr, UINT8 *pucMcList, UINT8 ucMcListSize )
{
    int nFilterOn = 1;

    if( ((usPacketFilterBitmap & NDIS_PACKET_TYPE_PROMISCUOUS) != 0) )
    {
        nFilterOn = 0;
    }
    else
    {
        if( ((usPacketFilterBitmap & NDIS_PACKET_TYPE_DIRECTED) != 0 &&
            *(UINT16 *) &pDestAddr[0] == *(UINT16 *) &pMacAddr[0] &&
            *(UINT16 *) &pDestAddr[2] == *(UINT16 *) &pMacAddr[2] &&
            *(UINT16 *) &pDestAddr[4] == *(UINT16 *) &pMacAddr[4]) )
        {
            nFilterOn = 0;
        }
        else
        {
            if( (pDestAddr[0] & 0x01) == 0x01 )
            {
                if( *(UINT16 *) &pDestAddr[0] == 0xffff &&
                    *(UINT16 *) &pDestAddr[2] == 0xffff &&
                    *(UINT16 *) &pDestAddr[4] == 0xffff )
                {
                    if((usPacketFilterBitmap & NDIS_PACKET_TYPE_BROADCAST) != 0)
                        nFilterOn = 0;
                }
                else
                {
                    if((usPacketFilterBitmap&NDIS_PACKET_TYPE_ALL_MULTICAST)!=0)
                        nFilterOn = 0;
                }
            }

            if( nFilterOn == 1 &&
                (usPacketFilterBitmap & NDIS_PACKET_TYPE_MULTICAST) != 0 )
            {
                UINT8 i;
                for( i = 0; i < ucMcListSize; i++, pucMcList += ETH_ALEN )
                {
                    if( *(UINT16 *) &pDestAddr[0] == *(UINT16 *) &pucMcList[0] &&
                        *(UINT16 *) &pDestAddr[2] == *(UINT16 *) &pucMcList[2] &&
                        *(UINT16 *) &pDestAddr[4] == *(UINT16 *) &pucMcList[4] )
                    {
                        nFilterOn = 0;
                        break;
                    }
                }
            }
        }
    }

    return( nFilterOn );
} /* UsbNicEthernetPacketFilter */


/*******************************************************************************
*
* UsbNicSendNow - Prepare to add data to the transmit queue.
*
* This routine fills in a USB_TX_HDR structure for preparing to send data.
*
* RETURNS: 0 - success, 1 - error
*/

LOCAL inline int UsbNicSendNow(pNBuff_t pNBuff, UINT8 * pData, int len,
    PUSBNIC_DEV_CONTEXT pDevCtx)
{
    int nStatus = 0;
    PUSB_TX_HDR pTxHdr = pDevCtx->pTxHdrFreeHead;

    DUMPADDR (pData, len) ;

    if( pTxHdr )
    {
        volatile DmaDesc *pBd = pDevCtx->pTxBdNext;
        volatile DmaDesc *pBdPktHdr = NULL;
        UINT32 ulAlignLen = 0;

        if( pDevCtx->ulHostDriverType == HOST_DRIVER_RNDIS )
        {
            UINT32 ulHdrLen = sizeof(RNDIS_PACKET) + RNDIS_PACKET_OVERHEAD;
            UINT32 ulTotalLen = ulHdrLen + len;
            UINT32 ulOffset = sizeof(RNDIS_PACKET);

            /* The buffer length of this BD must be a mulitple of 4 and the
             * starting buffer address of the next BD must be a multiple of 4.
             */
            ulAlignLen = (UINT32) pData & (sizeof(long) - 1);
            if( ulAlignLen )
            {
                UINT32 ulAlignDifference = sizeof(long) - ulAlignLen;
                memcpy((char *) &pTxHdr->PktHdr + ulHdrLen + ulAlignDifference,
                    pData, ulAlignLen);
                ulHdrLen += sizeof(long);
                ulTotalLen += ulAlignDifference;
                ulOffset += ulAlignDifference;
            }

            spin_lock_bh(&pDevCtx->usbnic_lock_tx);
            pTxHdr->PktHdr.NdisMessageType = BE_SWAP4(REMOTE_NDIS_PACKET_MSG);
            pTxHdr->PktHdr.MessageLength = BE_SWAP4(ulTotalLen);
            pTxHdr->PktHdr.Message.Packet.DataLength = BE_SWAP4(len);
            pTxHdr->PktHdr.Message.Packet.DataOffset = BE_SWAP4(ulOffset);

            pBdPktHdr = pBd;

            dma_cache_wback_inv((unsigned long) &pTxHdr->PktHdr, ulHdrLen );
            pBdPktHdr->address = (UINT32) VIRT_TO_PHYS(&pTxHdr->PktHdr);
            pBdPktHdr->length = ulHdrLen;

            /* Advance BD pointer to next in the chain. */
            if( (pBdPktHdr->status & DMA_WRAP) != 0 )
                pDevCtx->pTxBdNext = pDevCtx->pTxBdBase;
            else
                pDevCtx->pTxBdNext++;

            pBd = pDevCtx->pTxBdNext;
            spin_unlock_bh(&pDevCtx->usbnic_lock_tx);
        }

        spin_lock_bh(&pDevCtx->usbnic_lock_tx);
        pDevCtx->pTxHdrFreeHead = pTxHdr->pNext;
        pTxHdr->pBdAddr = pBd;
        pTxHdr->pNBuff = pNBuff;

        nbuff_flush(pNBuff, pData, len);

        pBd->address = (UINT32) VIRT_TO_PHYS(pData + ulAlignLen);
        pBd->length = len - ulAlignLen;

        /* Advance BD pointer to next in the chain. */
        if( (pBd->status & DMA_WRAP) != 0 )
            pDevCtx->pTxBdNext = pDevCtx->pTxBdBase;
        else
            pDevCtx->pTxBdNext++;

        /* Packet was enqueued correctly.  Advance to the next TXHDR needing to
         * be assigned to a BD.
         */
        if( pDevCtx->pTxHdrReclaimHead == NULL )
            pDevCtx->pTxHdrReclaimHead = pTxHdr;
        else
            pDevCtx->pTxHdrReclaimTail->pNext = pTxHdr;

        pDevCtx->pTxHdrReclaimTail = pTxHdr;
        pTxHdr->pNext = NULL;

        /* Start DMA transmit by setting own bit. */
        if( pBdPktHdr ) /* RNDIS */
        {
            pBd->status = (pBd->status & DMA_WRAP) | DMA_EOP | DMA_OWN;
            pBdPktHdr->status = (pBdPktHdr->status&DMA_WRAP) | DMA_SOP | DMA_OWN;
        }
        else /* CDC */
            pBd->status = (pBd->status & DMA_WRAP) | DMA_SOP | DMA_EOP | DMA_OWN;

        //DUMPADDR (pBd, sizeof (DmaDesc)) ;
        /* Enable DMA for this channel */
        pDevCtx->pTxDma->cfg |= DMA_ENABLE;

        pDevCtx->pDev->trans_start = jiffies;

        /* Update record of total bytes sent. */
        pDevCtx->DevStats.tx_bytes += len;
        if( pBdPktHdr )
            pDevCtx->DevStats.tx_bytes += sizeof(RNDIS_MESSAGE);
        spin_unlock_bh(&pDevCtx->usbnic_lock_tx);

        nStatus = 0;
    }
    else
    {
        /* No more transmit headers. */
        spin_lock_bh(&pDevCtx->usbnic_lock_tx);
        pDevCtx->DevStats.tx_aborted_errors++;
        spin_unlock_bh(&pDevCtx->usbnic_lock_tx);

        nStatus = 1;
    }

    return( nStatus );
} /* UsbNicSendNow */


/******************************************************************************
*
* bcmusbnic_xmit
*
* This routine sends a MAC frame over the USB.
*
* RETURNS: 0 - sucess, < 0 - error
*/

LOCAL int bcmusbnic_xmit(pNBuff_t pNBuff, struct net_device *dev)
{
    PUSBNIC_DEV_CONTEXT pDevCtx = NETDEV_PRIV(dev);
    PUSB_TX_HDR pReclaimHead = pDevCtx->pTxHdrReclaimHead;
    PUSB_TX_HDR pTxHdr;
    PENET_HDR pEh;
    UINT8 * pData;
    int len;

    /* Obtain exclusive access to transmitter.  This is necessary because
     * we might have more than one stack transmitting at once.
     */
    netif_stop_queue(pDevCtx->pDev);

    /* Reclaim transmitted buffers.  Loop here for each Tx'ed packet. */
    if( pReclaimHead && (pReclaimHead->pBdAddr->status & DMA_OWN) == 0 )
    {
        do
        {
            /* Indicate to upper levels that this ethernet packet was sent. */
            nbuff_free(pReclaimHead->pNBuff);

            spin_lock_bh(&pDevCtx->usbnic_lock_tx);
            /* Finally, return the transmit header to the free list. */
            pTxHdr = pReclaimHead;
            pReclaimHead = pReclaimHead->pNext;
            pTxHdr->pNext = pDevCtx->pTxHdrFreeHead;
            pDevCtx->pTxHdrFreeHead = pTxHdr;
            if( pDevCtx->pTxHdrFreeTail == NULL )
                pDevCtx->pTxHdrFreeTail = pTxHdr;
            spin_unlock_bh(&pDevCtx->usbnic_lock_tx);
        } while(pReclaimHead && (pReclaimHead->pBdAddr->status & DMA_OWN) == 0);

        spin_lock_bh(&pDevCtx->usbnic_lock_tx);
        /* Assign current reclaim head and tail pointers. */
        if( pReclaimHead == NULL )
            pDevCtx->pTxHdrReclaimHead = pDevCtx->pTxHdrReclaimTail = NULL;
        else
            pDevCtx->pTxHdrReclaimHead = pReclaimHead;
        spin_unlock_bh(&pDevCtx->usbnic_lock_tx);
    }

    if( nbuff_get_context(pNBuff, &pData, &len) != NULL )
    {
        DUMPADDR (pData, len) ;
        if( (pDevCtx->ulFlags & (USB_FLAGS_CONNECTED | USB_FLAGS_OPENED)) == 
            (USB_FLAGS_CONNECTED | USB_FLAGS_OPENED) )
        {
            spin_lock_bh(&pDevCtx->usbnic_lock_tx);
            pDevCtx->DevStats.tx_packets++;
            spin_unlock_bh(&pDevCtx->usbnic_lock_tx);

            /* Send the packet if it is not filtered */
            pEh = (PENET_HDR) pData;
            if( UsbNicEthernetPacketFilter( pDevCtx->usPacketFilterBitmap,
                pEh->dest, pDevCtx->ucCurrentHostMacAddr,
                pDevCtx->ucRndisMulticastList,
                pDevCtx->ucRndisNumMulticastListEntries ) )
            {
                nbuff_flushfree(pNBuff);
            }
            else
                if( UsbNicSendNow( pNBuff, pData, len, pDevCtx ) != 0 )
                    nbuff_flushfree(pNBuff);
        }
        else
        {
            nbuff_free(pNBuff);
            /* pDevCtx->DevStats.tx_dropped++; */
        }
    }

    netif_wake_queue(dev);

    return( 0 );
} /* bcmusbnic_xmit */


/*******************************************************************************
*
* usbnic_timer - free tranmit buffer complete and process receive buffers.
*
* This routine reclaims transmit frames which have been sent and processes
* receive buffers.  This is not a normal execution path.  Typically, transmit
* buffers a freed when sending the next frame and receive buffers are received
* due to an interrupt.  This function is used for special cases that are not
* caught by those conditions.
*
* RETURNS: None.
*/

LOCAL void usbnic_timer( PUSBNIC_DEV_CONTEXT pDevCtx )
{
    static int flags = 0 ;
    int i ;
    unsigned long irqFlags;

    /* Free transmitted buffers.  */
    if( netif_queue_stopped(pDevCtx->pDev) == 0 )
        bcmusbnic_xmit( PNBUFF_NULL, pDevCtx->pDev );

    /* Check for link up/down. */
    if( (pDevCtx->ulFlags & USB_FLAGS_PLUGGED_IN) == USB_FLAGS_PLUGGED_IN) {

        spin_lock_bh(&pDevCtx->usbnic_lock_tx);

        spin_lock_irqsave(&pDevCtx->usbnic_lock_UsbIsr, irqFlags);

        pDevCtx->ulFlags |= USB_FLAGS_PLUGGED_IN;
        USB->usbd_events_irq = 0;

        spin_unlock_irqrestore(&pDevCtx->usbnic_lock_UsbIsr, irqFlags);

        pDevCtx->MibInfo.ulIfLastChange = (jiffies * 100) / HZ;
        if (((USB->usbd_status & USBD_ENUM_SPEED) >> USBD_ENUM_SPEED_SHIFT) == 0x00)
            pDevCtx->MibInfo.ulIfSpeed = 480000000; /* 480 Mb */
        else
            pDevCtx->MibInfo.ulIfSpeed = 12000000; /* 12 Mb */

        spin_unlock_bh(&pDevCtx->usbnic_lock_tx);

        if ((flags == 0) || (flags == 2)) {
            if (netif_carrier_ok(pDevCtx->pDev) == 0)
                netif_carrier_on(pDevCtx->pDev);
            printk("USB Link UP.\r\n");
            flags = 1 ;

            /* Wake up the user mode application that monitors link status. */
            kerSysSendtoMonitorTask(MSG_NETLINK_BRCM_LINK_STATUS_CHANGED,NULL,0);
        }
    }
    else {

       if ((flags == 0) || (flags == 1) || (flags == 3)) {
          if (netif_carrier_ok(pDevCtx->pDev) != 0)
              netif_carrier_off(pDevCtx->pDev);
          printk("USB Link DOWN.\r\n");

          /* Wake up the user mode application that monitors link status. */
          kerSysSendtoMonitorTask(MSG_NETLINK_BRCM_LINK_STATUS_CHANGED,NULL,0);

          spin_lock_bh(&pDevCtx->usbnic_lock_tx);

          spin_lock_irqsave(&pDevCtx->usbnic_lock_UsbIsr, irqFlags);

          pDevCtx->ulFlags &= ~USB_FLAGS_PLUGGED_IN;

          spin_unlock_irqrestore(&pDevCtx->usbnic_lock_UsbIsr, irqFlags);

          pDevCtx->MibInfo.ulIfLastChange = (jiffies * 100) / HZ;
          pDevCtx->MibInfo.ulIfSpeed = 0;

          spin_unlock_bh(&pDevCtx->usbnic_lock_tx);

          if (flags != 0) {
             /* USB Link Down, we could experiment the USBD reset and then wake it up
              * again. Assert the reset for USB core.
              */
             PERF->softResetB &= ~(SOFT_RST_USBD) ;

             for (i=50 ; i >0 ; i--)
                ;

             /* De-Assert the USBD core */
             PERF->softResetB |= (SOFT_RST_USBD) ;
             if (UsbNicInit (pDevCtx, USBD_REINIT) != 0) {
                flags = 3 ;
                goto _Timer ;
             }
          }

          flags = 2 ;
       }
    }

_Timer :
    /* Restart the timer. */
    pDevCtx->ReclaimTimer.expires = jiffies + USB_TIMEOUT;
    add_timer(&pDevCtx->ReclaimTimer);
} /* usbnic_timer */


/*******************************************************************************
*
* bcmBulkRxIsr - DMA Interrupt Service Routine for the USB bulk rx endpoint
*
* This routine is called when data has been received on the bulk endpoint.
* It schedules a function to run in the bottom half to process the received
* data.
*
* RETURNS: None.
*/
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
LOCAL FN_HANDLER_RT bcmBulkRxIsr(int irq, void *dev_id)
#else
LOCAL void bcmBulkRxIsr(int irq, void *dev_id, struct pt_regs *regs)
#endif
{
    PUSBNIC_DEV_CONTEXT pDevCtx = dev_id;
    volatile DmaChannelCfg *pRxDma = pDevCtx->pRxDma;
    UINT32 ulRxIntStat = pRxDma->intStat;

    PRINTK ("Entry bcmBulkRxIsr \n") ;

    if( (ulRxIntStat & DMA_DONE) != 0 )
        pRxDma->intStat = DMA_DONE | DMA_BUFF_DONE;  /* clear interrupt */

#ifdef USE_BH
    tasklet_schedule(&pDevCtx->BhBulkRx);
#else
    usbnic_bulkrx(pDevCtx);
#endif    
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
    return BCM_IRQ_HANDLED;
#endif 
} /* bcmBulkRxIsr */

/*******************************************************************************
*
* usbnic_assign_buffer - Re-assign the released buffer to the recieve pool.
*
* RETURNS: None.
*/

LOCAL void usbnic_assign_buffer(UINT8 * pBuf, PUSBNIC_DEV_CONTEXT pDevCtx)
{
    volatile DmaDesc *pAssign;
    volatile DmaChannelCfg *pRxDma;

    spin_lock_bh(&pDevCtx->usbnic_lock_rx);

    pAssign = pDevCtx->pRxBdAssign;
    pRxDma = pDevCtx->pRxDma;

    /* Some cache lines might be dirty. If these lines are flushed after the
     * DMA puts data into SDRAM, the data will be corrupted. Invalidating
     * the buffer will clear the dirty bit(s) and prevent this problem.
     */
    dma_cache_wback_inv((unsigned long) pBuf, RX_USB_NIC_BUF_SIZE);

    if( pAssign->address == 0 )
    {
        pAssign->address = VIRT_TO_PHYS((unsigned long) pBuf);
        pAssign->length  = RX_USB_NIC_BUF_SIZE;
        pAssign->status = (pAssign->status & DMA_WRAP) | DMA_OWN;

        if( (pAssign->status & DMA_WRAP) != 0 )
            pDevCtx->pRxBdAssign = pDevCtx->pRxBdBase;
        else
            pDevCtx->pRxBdAssign++;
    }
    else
    {
        /* This should not happen. */
        printk("USBD: Buffer already assigned. Typical for USBD ReInit. \n");
    }

    /* Restart DMA in case the DMA ran out of descriptors, and
     * is currently shut down.
     */
    if( (pRxDma->intStat & DMA_NO_DESC) != 0 )
    {
        pDevCtx->DevStats.rx_errors++;
        pRxDma->intStat = DMA_NO_DESC;
        pRxDma->cfg = DMA_ENABLE;
    }

    spin_unlock_bh(&pDevCtx->usbnic_lock_rx);
} /* usbnic_assign_buffer */

/*******************************************************************************
*
* usbnic_recycle_fkb - recycle transmitted fkbuff
*
* This routine is the callback handler for recyclying a fkbuff
*
* RETURNS: None.
*/

LOCAL void usbnic_recycle_fkb(struct fkbuff *pFkb, PUSBNIC_DEV_CONTEXT pDevCtx)
{
    UINT8 * pBuf = PFKBUFF_TO_PDATA(pFkb, RX_USB_SKB_RESERVE);

    usbnic_assign_buffer(pBuf, pDevCtx); /* No need to cache invalidate */

} /* usbnic_recycle_fkb */

/*******************************************************************************
*
* usbnic_recycle_skb_or_data - recycle transmitted sk_buff or data
*
* This routine is the callback handler for recyclying a sk_buff or data based
* on the free flags. A recycled skb is placed on the free list.
*
* RETURNS: None.
*/

LOCAL void usbnic_recycle_skb_or_data(struct sk_buff *skb,
    PUSBNIC_DEV_CONTEXT pDevCtx, UINT32 nFlag)
{
    if( nFlag & SKB_RECYCLE )
    {
        spin_lock_bh(&pDevCtx->usbnic_lock_rx);

        skb->next_free = pDevCtx->pFreeSockBufList;
        pDevCtx->pFreeSockBufList = skb;

        spin_unlock_bh(&pDevCtx->usbnic_lock_rx);
    }
    else // if ( nFlag & SKB_DATA_RECYCLE )
    {
        UINT8 *pData;
        pData = skb->head + RX_USB_SKB_RESERVE;
        usbnic_assign_buffer(pData, pDevCtx);
    }

} /* usbnic_recycle_skb_or_data */

/*******************************************************************************
*
* usbnic_recycle - recycle transmitted network buffers
*
* This routine is the callback handler for recyclying a pNBuff. It invokes
* the sk_buff or fk_buff version of recycler.
*
* RETURNS: None.
*/

LOCAL void usbnic_recycle(pNBuff_t pNBuff, PUSBNIC_DEV_CONTEXT pDevCtx,
    UINT32 nFlag)
{
    if (IS_FKBUFF_PTR(pNBuff))
        usbnic_recycle_fkb( PNBUFF_2_FKBUFF(pNBuff), pDevCtx );
    else // if (IS_SKBUFF_PTR(pNbuff))
        usbnic_recycle_skb_or_data( PNBUFF_2_SKBUFF(pNBuff), pDevCtx, nFlag);
} /* usbnic_recycle */

/*******************************************************************************
*
* usbnic_bulkrx - Process received packets
*
* This routine runs in the context of a bottom half.  It processes packets
* received on USB bulk out endpoint.  It assumes/expects that one complete
* packet is received in one DMA buffer descriptor.
*
* RETURNS: None.
*/

LOCAL void usbnic_bulkrx( PUSBNIC_DEV_CONTEXT pDevCtx )
{
    FkBuff_t * pFkb;
    struct net_device *dev = pDevCtx->pDev;
    UINT8 *pBuf;
    UINT16 usLength, usMinLen;
    volatile DmaDesc *pRxBd = pDevCtx->pRxBdRead;

    if( pDevCtx->ulHostDriverType == HOST_DRIVER_RNDIS )
        usMinLen = sizeof(RNDIS_PACKET) + RNDIS_PACKET_OVERHEAD;
    else
        usMinLen = 0;

    /* Loop here for each received packet. */
    while( (pRxBd->status & DMA_OWN) == 0 && pRxBd->address != 0 )
    {
        pBuf = (UINT8 *) (phys_to_virt(pRxBd->address));

        usLength = pRxBd->length;

        DUMPADDR (pBuf, usLength) ;

        spin_lock_bh(&pDevCtx->usbnic_lock_rx);
        pRxBd->address = 0;
        pRxBd->status &= DMA_WRAP;

        if( (pRxBd->status & DMA_WRAP) != 0 )
            pRxBd = pDevCtx->pRxBdBase;
        else
            pRxBd++;
        spin_unlock_bh(&pDevCtx->usbnic_lock_rx);

        if( usLength < usMinLen )
        {
            usbnic_assign_buffer(pBuf, pDevCtx);
            continue;
        }

        spin_lock_bh(&pDevCtx->usbnic_lock_rx);
        pDevCtx->DevStats.rx_packets++;
        pDevCtx->DevStats.rx_bytes += usLength;
        spin_unlock_bh(&pDevCtx->usbnic_lock_rx);

        /* Notify kernel if device is opened. */
        if( (pDevCtx->ulFlags & (USB_FLAGS_CONNECTED | USB_FLAGS_OPENED)) !=
             (USB_FLAGS_CONNECTED | USB_FLAGS_OPENED) )
        {
            usbnic_assign_buffer(pBuf, pDevCtx);
            continue;
        }

        pFkb = fkb_init(pBuf, RX_USB_SKB_RESERVE, pBuf, usLength);

        if( pDevCtx->ulHostDriverType == HOST_DRIVER_RNDIS ) {
            fkb_pull(pFkb, sizeof(RNDIS_PACKET) + RNDIS_PACKET_OVERHEAD);
        }
#ifdef CONFIG_BLOG
        if ( blog_finit(pFkb, dev, TYPE_ETH, ~0, BLOG_USBPHY)
                == PKT_DONE )
#else
        if(0)
#endif
        {
            dev->last_rx = jiffies;
        }
        else if( pDevCtx->pFreeSockBufList )
        {
            struct sk_buff *skb;
            spin_lock_bh(&pDevCtx->usbnic_lock_rx);
            skb = pDevCtx->pFreeSockBufList;
            pDevCtx->pFreeSockBufList = pDevCtx->pFreeSockBufList->next_free;
            spin_unlock_bh(&pDevCtx->usbnic_lock_rx);

            skb_headerinit( RX_USB_SKB_RESERVE,
                RX_USB_ALLOC_BUF_SIZE - RX_USB_SKB_RESERVE, skb, pBuf,
                (RecycleFuncP)usbnic_recycle_skb_or_data, (UINT32)pDevCtx,
                (void*)pFkb->blog_p);
            __skb_trim(skb, usLength);

            if( pDevCtx->ulHostDriverType == HOST_DRIVER_RNDIS ) {
                __skb_pull(skb, sizeof(RNDIS_PACKET) + RNDIS_PACKET_OVERHEAD);
            }

            /* Finish setting up the received SKB and send it to the kernel */
            skb->dev = dev;

            /* Set the protocol type */
            skb->protocol = eth_type_trans(skb, dev);

            /* Notify kernel if device is opened. */
            netif_rx(skb);
            dev->last_rx = jiffies;
        }
        else
        {
            spin_lock_bh(&pDevCtx->usbnic_lock_rx);

            pDevCtx->DevStats.rx_dropped++;

            spin_unlock_bh(&pDevCtx->usbnic_lock_rx);

            fkb_release(pFkb);
            usbnic_assign_buffer( pBuf, pDevCtx);
        }
    }

    spin_lock_bh(&pDevCtx->usbnic_lock_rx);

    pDevCtx->pRxBdRead = pRxBd;

    spin_unlock_bh(&pDevCtx->usbnic_lock_rx);

    /* Re-enable interrupts to resume packet reception. */
    BcmHalInterruptEnable(INTERRUPT_ID_USB_BULK_RX_DMA);
} /* usbnic_bulkrx */


/*******************************************************************************
*
* bcmUsbIsr - USB Interrupt Service Routine
*
* This routine schedules a function to run in the tNetTask context to process
* an event for the USB MAC.
*
* RETURNS: ISR handle status.
*/
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
LOCAL FN_HANDLER_RT bcmUsbIsr(int irq, void *dev_id)
#else
LOCAL void bcmUsbIsr(int irq, void *dev_id, struct pt_regs *regs)
#endif
{
    PUSBNIC_DEV_CONTEXT pDevCtx = dev_id;
    UINT32 ulEventsIrqMask = USB->usbd_events_irq_mask;
    UINT32 ulEventsIrq = USB->usbd_events_irq;
    UINT16 Speed;

    //printk ("Entry bcmUsbIsr \n") ;

    /* Mask all. */
    USB->usbd_events_irq_mask = 0;

    if( (ulEventsIrq & USBD_USB_RESET_IRQ) != 0 )
    {
		 //printk ("Reset \n") ;
		  //DUMPADDR (pDevCtx->usbConfigBufferBase, 20) ;
        /* Clear all stalled endpoints. */
        //USB->endpt_stall_reset |= (byte)(USB->endpt_prnt & USB->endpt_status);

        /* Clear interrupt condition. */
        USB->usbd_events_irq = USBD_USB_RESET_IRQ ;

        ulEventsIrqMask |= USBD_USB_RESET_IRQ;

	dma_cache_wback_inv((unsigned long) pDevCtx->pTxCtrlBdBase, NR_CTRL_BDS * sizeof (DmaDesc));
        pDevCtx->pTxCtrlBdCurr = pDevCtx->pTxCtrlBdNext;
    }

    if( (ulEventsIrq & USBD_USB_SETUP_IRQ) != 0 )
    {
       int i ;
        USB->usbd_events_irq = USBD_USB_SETUP_IRQ; /* clear interrupt */

        Speed = ((USB->usbd_status & USBD_ENUM_SPEED) >> USBD_ENUM_SPEED_SHIFT) ;
	PRINTK ("USB: SETUP On; Speed : %d\n", Speed) ;

        if (pDevCtx->speed != Speed)
	{
           pDevCtx->speed = Speed ;

           if (Speed == 0x0) {
              PRINTK ("High Speed 480Mbps\n");
           }
           else if (Speed == 0x2) {
              PRINTK ("Low Speed 1.5Mbps\n") ;
           }
           else if (Speed == 0x3) {
              PRINTK ("Full Speed 12Mbps\n") ;
           }
           else {
             PRINTK ("Handshaking (or Full/Low Speed)\n");
           }
        }

	// try and only set the FIFOs/EPs if there's some sort of change
	if((pDevCtx->cfg != scfg) || (pDevCtx->intf != sintf) || (pDevCtx->altIntf != saltIntf) || (pDevCtx->speed != sSpeed))
	{
	  scfg = pDevCtx->cfg; sintf = pDevCtx->intf; saltIntf = pDevCtx->altIntf; sSpeed = pDevCtx->speed;

	  usb_setup_txrx_fifo (pDevCtx->speed) ;

	  // Initialize each endpoint's Tx/Rx Fifo by resetting them
	  for (i = 0; i < 3; i++)
	  {
	       USB->usbd_control = USBD_CONTROL_APP_FIFO_INIT_SEL(i) ;
	       USB->usbd_control |= USBD_CONTROL_APP_AUTO_CSRS ;
	       USB->usbd_control |= EN_TXZLENINS | APPSETUPERRLOCK | 0x000010C0 ;  // Reset each Rx/Tx FIFO
	  }

	  pDevCtx->cfg = UDC20_CFG (USB->usbd_status) ;
	  pDevCtx->intf = UDC20_INTF (USB->usbd_status) ;
	  pDevCtx->altIntf = UDC20_ALTINTF (USB->usbd_status) ;
	  usb_set_csr_endpoints (pDevCtx->cfg, pDevCtx->intf, pDevCtx->altIntf, pDevCtx->speed) ;
	}

        //UsbNicProcessSetupCommand( pDevCtx );
        pDevCtx->ulFlags |= USB_FLAGS_PLUGGED_IN;
    }

    if( (ulEventsIrq & USBD_USB_SETCFG_IRQ) != 0 )
    {
		 //printk ("USB: SetCfg Irq \n") ;
       USB->usbd_events_irq = USBD_USB_SETCFG_IRQ; /* clear interrupt */
       USB->usbd_control |= USBD_CONTROL_APP_AUTO_CSRS ;
       pDevCtx->cfg = UDC20_CFG (USB->usbd_status) ;

       //UsbNicProcessSetupCommand( pDevCtx );
       pDevCtx->ulFlags |= USB_FLAGS_PLUGGED_IN;
    }

    if( (ulEventsIrq & USBD_USB_SET_CSRS_IRQ) != 0 )
    {
		 //printk ("USB: SetCsrs Irq \n") ;
       USB->usbd_events_irq = USBD_USB_SET_CSRS_IRQ; /* clear interrupt */
       USB->usbd_control |= USBD_CONTROL_APP_DONECSR ;
       pDevCtx->intf = UDC20_INTF (USB->usbd_status) ;
       pDevCtx->altIntf = UDC20_ALTINTF (USB->usbd_status) ;

       //if ((pDevCtx->intf !=0) || (pDevCtx->altIntf !=0))
       usb_set_csr_endpoints (pDevCtx->cfg, pDevCtx->intf, pDevCtx->altIntf, pDevCtx->speed) ;
       //UsbNicProcessSetupCommand( pDevCtx );
       pDevCtx->ulFlags |= USB_FLAGS_PLUGGED_IN;
    }

    if( (ulEventsIrq & USBD_USB_LINK_IRQ) != 0 )
    {
		 PRINTK ("USBD: Link Irq \n") ;
       USB->usbd_events_irq = USBD_USB_LINK_IRQ; /* clear interrupt */
       if (USB->usbd_events & USBD_LINK)
         pDevCtx->ulFlags |= USB_FLAGS_PLUGGED_IN;
       else
         pDevCtx->ulFlags &= ~USB_FLAGS_PLUGGED_IN;
    }

    if( (ulEventsIrq & USBD_USB_ENUM_ON_IRQ) != 0 )
     {
	UINT16 Speed, i ; 
	USB->usbd_events_irq = USBD_USB_ENUM_ON_IRQ; /* clear interrupt */
	Speed = ((USB->usbd_status & USBD_ENUM_SPEED) >> USBD_ENUM_SPEED_SHIFT) ;

	PRINTK ("USB: Enum On  Current Speed : %d\n", Speed) ;

       if (pDevCtx->speed != Speed)
       {
          pDevCtx->speed = Speed ;
          if (Speed == 0x0) {
             PRINTK ("High Speed 480Mbps\n") ;
          }
          else if (Speed == 0x2) {
             PRINTK ("Low Speed 1.5Mbps\n") ;
          }
          else if (Speed == 0x3) {
             PRINTK ("Full Speed 12Mbps\n");
          }
          else {
             PRINTK ("Handshaking (or Full/Low Speed)\n");
             goto _exit2 ;
          }

      }
      usb_setup_txrx_fifo (pDevCtx->speed) ;

      // Initialize each endpoint's Tx/Rx Fifo by resetting them
      for (i = 0; i < 3; i++)
      {
	USB->usbd_control = USBD_CONTROL_APP_FIFO_INIT_SEL(i) ;
	   USB->usbd_control |= USBD_CONTROL_APP_AUTO_CSRS ;
		  USB->usbd_control |= EN_TXZLENINS | APPSETUPERRLOCK | 0x000010C0 ;  // Reset each Rx/Tx FIFO
      }

      pDevCtx->cfg = UDC20_CFG (USB->usbd_status) ;
      pDevCtx->intf = UDC20_INTF (USB->usbd_status) ;
      pDevCtx->altIntf = UDC20_ALTINTF (USB->usbd_status) ;
      usb_set_csr_endpoints (pDevCtx->cfg, pDevCtx->intf, pDevCtx->altIntf, pDevCtx->speed) ;
    }

_exit2  :
    if( (ulEventsIrq & USBD_USB_SUSPEND_IRQ) != 0 )
    {
		  //DUMPADDR (pDevCtx->usbConfigBufferBase, 20) ;
        USB->usbd_events_irq = USBD_USB_SUSPEND_IRQ; /* clear interrupt */
        pDevCtx->ulFlags &= ~USB_FLAGS_CONNECTED;
        //pDevCtx->pTxCtrlBdCurr = pDevCtx->pTxCtrlBdNext;
    }

    USB->usbd_events_irq_mask = ulEventsIrqMask;
	
    BcmHalInterruptEnable(INTERRUPT_ID_USBS);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
    return BCM_IRQ_HANDLED;    
#endif

} /* bcmUsbIsr */

/*******************************************************************************
*
* UsbNicProcessCtrlMsg - Process a received CTRL message.
*
* This routine processes a control pipe request from the USB host.
*
* RETURNS: None.
*/

void UsbNicProcessCtrlMsg(PUSBNIC_DEV_CONTEXT pDevCtx, PCTRL_PKT_BUFS pCtrlPkt,
		                    UINT32 length)
{
	USB_setup *pSetupMsg = (USB_setup *) pCtrlPkt;
	StringDesc *pStrDesc = &pDevCtx->StrDesc;
	UINT32 ulIndex;
	int errFlag = 0;
	unsigned long irqFlags;

	/* Endian conversion. */
	pSetupMsg->wValue = BE_SWAP2(pSetupMsg->wValue);
	pSetupMsg->wIndex = BE_SWAP2(pSetupMsg->wIndex);
	pSetupMsg->wLength = BE_SWAP2(pSetupMsg->wLength);

	
	switch( (pSetupMsg->bmRequestType & RT_TYPE_MASK) )
	{
		case RT_TYPE_STANDARD:
			switch( pSetupMsg->bRequest)
			{
				case GET_DESCRIPTOR:
				switch(pSetupMsg->wValue >> 8)
				{
					case STRING_DESC:
						ulIndex = (pSetupMsg->wValue & 0x00ff) ;
						PRINTK ("[STRING_DESC %u ]\n", ulIndex);

						spin_lock_bh(&pDevCtx->usbnic_lock_msg);

						pStrDesc->bDescriptorType = STRING_DESC;
						if((ulIndex = (pSetupMsg->wValue & 0x00ff)) < STRING_TABLE_SIZE)
						{
							if( ulIndex == 0 )
							{
								pStrDesc->bLength =
									strlen( pDevCtx->pStrTbl[ulIndex].string ) + 2;

								strcpy( pStrDesc->genericString,
										pDevCtx->pStrTbl[ulIndex].string );
							}
							else
							{
								/* Handle specific string descriptor, unicode form */
								pStrDesc->bLength =
									strlen( pDevCtx->pStrTbl[ulIndex].string ) * 2 + 2;

								ConvertAnsiToUnicode( (char *) pStrDesc->genericString,
										pDevCtx->pStrTbl[ulIndex].string );

								/* The USB cable is plugged in.  Data is ready to be
								 * received if the CDC host driver is being used.
								 */
								if( pDevCtx->ulHostDriverType == HOST_DRIVER_CDC )
								{
									spin_lock_irqsave(&pDevCtx->usbnic_lock_UsbIsr, irqFlags);
									pDevCtx->ulFlags |= USB_FLAGS_CONNECTED;
									spin_unlock_irqrestore(&pDevCtx->usbnic_lock_UsbIsr, irqFlags);
								}
							}
						}
						else
						{
							/* String index is greater than string table size.  Signal
							 * error and abort processing.
							 */
							pStrDesc->bLength = 3;
							pStrDesc->genericString[0] = 0;
							errFlag = 1;
							printk("UsbNicProcessCtrlMsg(): String index error\n");
						}

						spin_unlock_bh(&pDevCtx->usbnic_lock_msg);

						/* Send string descriptor. */
						SCTLPKT( pDevCtx, (UINT8 *) pStrDesc,
								(UINT16) 
								(pSetupMsg->wLength > pStrDesc->bLength)
								? pStrDesc->bLength : pSetupMsg->wLength,
								TRUE) ;
						break;

					case DEVICE_DESC:
						PRINTK ("[DEVICE_DESC]\n");

						PrepareUsbDescriptors(pDevCtx) ;
						// If we're connected to the host when we power up, we don't see
						// the usb connected interrupt.  So, this handles that situation
						// since we won't receive a GET_DESCRIPTOR unless we're connected!

						spin_lock_bh(&pDevCtx->usbnic_lock_msg);

						if( (pDevCtx->ulFlags & USB_FLAGS_CONNECTED) 
								!= USB_FLAGS_CONNECTED)
						{
							spin_lock_irqsave(&pDevCtx->usbnic_lock_UsbIsr, irqFlags);
							pDevCtx->ulFlags |= USB_FLAGS_CONNECTED;
							spin_unlock_irqrestore(&pDevCtx->usbnic_lock_UsbIsr, irqFlags);
						}
						if (pSetupMsg->wLength > pDevCtx->pUsbCfg->deviceDesc.bLength)
							pSetupMsg->wLength = pDevCtx->pUsbCfg->deviceDesc.bLength;

						spin_unlock_bh(&pDevCtx->usbnic_lock_msg);

						SCTLPKT( pDevCtx, pDevCtx->usbConfigBufferBase,
								pSetupMsg->wLength,
								TRUE);
						//DUMPADDR (pDevCtx->usbConfigBufferBase, pSetupMsg->wLength) ;
						break;

					case CONFIGURATION_DESC:
						ulIndex = (pSetupMsg->wValue & 0x00ff) ;
						PRINTK ("[CONFIGURATION_DESC] %d \n", pSetupMsg->wIndex) ;
						// Check which config. the host is requesting - we support 2 configurations
						// in RNDIS mode.  In CDC, there's only one config.
						UsbProcessConfigDescRequest (pDevCtx, pSetupMsg->wValue>>8, pSetupMsg->wLength,
								ulIndex) ;
						break;
					case OTHER_SPEED_DESC:
						PRINTK ("[OTHER_SPEED_DESC]\n");
						ulIndex = (pSetupMsg->wValue & 0x00ff) ;
						if (pSetupMsg->wLength > pDevCtx->pUsbCfg->cdc_configDesc.wTotalLength_hi)
							pSetupMsg->wLength = pDevCtx->pUsbCfg->cdc_configDesc.wTotalLength_hi ;
						UsbProcessConfigDescRequest (pDevCtx, pSetupMsg->wValue>>8, pSetupMsg->wLength,
								ulIndex) ;
						break;

					case DEVQUALIFIER_DESC:
						PRINTK ("[DEVICE_QUALIFIER_DESC]\n");
						if (pSetupMsg->wLength > pDevCtx->pUsbCfg->deviceQualDesc.bLength)
							pSetupMsg->wLength = pDevCtx->pUsbCfg->deviceQualDesc.bLength;
						SCTLPKT( pDevCtx, (UINT8 *) &pDevCtx->pUsbCfg->deviceQualDesc,
								pSetupMsg->wLength,
								TRUE);
						break;

					case INTERFACE_DESC:
					case ENDPOINT_DESC:
						break;

					default:
						/* Invalid descriptor type. Signal error and abort processing.*/
						printk ("[Invalid GET_DESCRIPTOR]\n") ;
						//DUMPADDR (pSetupMsg, sizeof (USB_setup)) ;
						errFlag = 1;
						break;
				}
				break;

				case GET_STATUS:
				PRINTK ("[GET_STATUS]\n");
				break;

				case CLEAR_FEATURE:
				PRINTK ("[CLEAR_FEATURE]\n");
				break;

				case SET_FEATURE:
				PRINTK ("[SET_FEATURE]\n");
				break;

				case SET_ADDRESS:
				PRINTK ("[SET_ADDRESS]:%x\n", pSetupMsg->wValue);
				break;

				case SET_DESCRIPTOR:
				PRINTK ("[SET_DESCRIPTOR]\n");
				break;

				case GET_CONFIGURATION:
				PRINTK ("[GET_CONFIGURATION]\n");
				break;

				case SET_CONFIGURATION:
				PRINTK ("[SET_CONFIGURATION]:%d\n", pSetupMsg->wValue);
				break;

				case GET_INTERFACE:
				PRINTK ("[GET_INTERFACE]\n");
				break;

				case SET_INTERFACE:
				PRINTK ("[SET_INTERFACE]\n");
				break;

				case SYNCH_FRAME:
				/* dg-mod, currently unsupported (signal error) */
				PRINTK ("[SYNCH_FRAME]\n");
				errFlag = 1;
				break;

				default:
				PRINTK ("[BAD_REQUEST %d]\n", pSetupMsg->bRequest) ;
				/* Invalid setup request.  Signal error and abort processing. */
				errFlag = 1;
				break;
			} /* switch( pSetupMsg->bRequest) */
			break;

		case RT_TYPE_CLASS:
			switch( pSetupMsg->bRequest & 0xFF)
			{
				case SEND_ENCAPSULATED_COMMAND:
					PRINTK("[SEND_ENCAPSULATED_COMMAND]\n");
					// This is part of the standard SETUP packet of the USB protocol.
					// The SEND_ENCAPSULATED_COMMAND request tells us the the RNDIS message
					// will follow in the next BD.  Let's set a flag to denote that we're 
					// waiting for the RNDIS portion of the SEND_ENCAPSULATED_COMMAND. Depending
					// on the speed of the host, the message may or may not be available already 
					// in the next BD, but we'll play it safe, set the flag, reclaim the BD and do 
					// the usual stuff.  When the next BD's available, the control rx thread will
					// check the flag and call the RndisProcessMessage() function.
               				pDevCtx->ulHostDriverType = HOST_DRIVER_RNDIS ;
               				pDevCtx->waitForRndisMessage = TRUE ;
					PRINTK ("\n ########### HOST Driver Type = RNDIS ############ \n") ;
					//PrepareUsbDescriptors (pDevCtx) ;
					break;

				case GET_ENCAPSULATED_RESPONSE:
					/* Sent by host Remote NDIS driver. */
					PRINTK("[GET_ENCAPSULATED_RESPONSE]\n");
               				RndisDoGetEncapsulatedResponse(pDevCtx, pSetupMsg->wLength );
               				break;

				case SET_ETHERNET_PACKET_FILTER:
               			PRINTK ("[SET_ETHERNET_PACKET_FILTER]\n");
					/* Should be sent by host CDC driver. */

					spin_lock_bh(&pDevCtx->usbnic_lock_msg);

					pDevCtx->usPacketFilterBitmap = 0;

					if( (pSetupMsg->wValue & USB_PACKET_TYPE_MULTICAST) != 0 )
						pDevCtx->usPacketFilterBitmap |= NDIS_PACKET_TYPE_MULTICAST;

					if( (pSetupMsg->wValue & USB_PACKET_TYPE_BROADCAST) != 0 )
						pDevCtx->usPacketFilterBitmap |= NDIS_PACKET_TYPE_BROADCAST;

					if( (pSetupMsg->wValue & USB_PACKET_TYPE_DIRECTED) != 0 )
						pDevCtx->usPacketFilterBitmap |= NDIS_PACKET_TYPE_DIRECTED;

					if( (pSetupMsg->wValue & USB_PACKET_TYPE_ALL_MULTICAST) != 0 )
						pDevCtx->usPacketFilterBitmap |= NDIS_PACKET_TYPE_ALL_MULTICAST;

					if( (pSetupMsg->wValue & USB_PACKET_TYPE_PROMISCUOUS) != 0 )
						pDevCtx->usPacketFilterBitmap |= NDIS_PACKET_TYPE_PROMISCUOUS;

					spin_unlock_bh(&pDevCtx->usbnic_lock_msg);

					break;

				default:
					/* Invalid setup request.  Signal error and abort processing. */
               				printk ("[BAD_REQUEST]:%02x\n", pSetupMsg->bRequest);
					errFlag = 1;
					break;
			}
			break;

		case RT_TYPE_VENDOR:
			PRINTK ("[RT_TYPE_VENDOR]\n");

			switch(pSetupMsg->bRequest & 0xff)
			{
				case SET_HOST_VERSION:

					spin_lock_bh(&pDevCtx->usbnic_lock_msg);

					pDevCtx->ulHostVersion = (pSetupMsg->wValue << 16) | pSetupMsg->wIndex;

					spin_unlock_bh(&pDevCtx->usbnic_lock_msg);

					PRINTK("[SET_HOST_VERSION]:0x%08lx\n", pDevCtx->ulHostVersion);
					break;

				default:
					/* Invalid setup request.  Signal error and abort processing. */
					printk("[BAD_REQUEST]:%02x\n", pSetupMsg->wValue >> 8);
					break;
			}
			break;

		default:
			errFlag = 1;
			break;
	} /* switch( (pSetupMsg->bmRequestType & RT_TYPE_MASK) ) */

	/* Signal end of command processing. */
	if( errFlag == 1 )
	{
		// Issue a STALL to the host.  The STALL will be cleared automatically by the
		// next SETUP command from the host.
		printk ("\nUSBD : STALL\n") ;

		spin_lock_bh(&pDevCtx->usbnic_lock_msg);
		USB->usbd_stall = USB_ENDPOINT_0 ; /* issue stall on endpoint 0 */
		spin_unlock_bh(&pDevCtx->usbnic_lock_msg);
	}

} /* UsbNicProcessCtrlMsg */


// This function processes CONFIGURATION_DESCRIPTOR requests and
// OTHER_SPEED_CONFIGURATION_DESCRIPTOR requests.  The descriptors for both these
// requests are similar, so we use the same buffers to process both.  The approach is
// a little kludgy [ed: no isht - krc], but we would rather save the buffer space for an entire config. that
// had to be duplicated otherwise.

VOID UsbProcessConfigDescRequest (PUSBNIC_DEV_CONTEXT pDevCtx, UINT8 desc, UINT32 length, UINT8 index)
{
	UINT8 usbCurrentMode;
	UsbConfigDesc *configDescPtr;
	UsbEndpointDesc *endptIntDescPtr;
	UsbEndpointDesc *endptDescPtr, *pedp;
	UINT8 i, maxBlkSizeLSB, maxBlkSizeMSB, bIntervalInt, maxIntSize;
	UINT8 bIntervalOut;

	PRINTK("UsbProcessConfigDescRequest(): desc %d len %u index %d\n", desc, length, index);

	usbCurrentMode = ((USB->usbd_status & USBD_ENUM_SPEED) >> USBD_ENUM_SPEED_SHIFT) ;

	if (desc == CONFIGURATION_DESC)
	{
		PRINTK("UsbProcessConfigDescRequest(): CONFIGURATION_DESC\n");
		if (usbCurrentMode == 0x00)
		{
			// USB HS
			maxBlkSizeLSB = (USB_HS_MAX_BULK % 256); // 512 byte max bulk size
			maxBlkSizeMSB = (USB_HS_MAX_BULK / 256);
			maxIntSize    = USB_HS_MAX_INT;		 // 64  byte max int  size
			bIntervalInt  = (index == 1) ? USB_HS_INT_CDC_BINTERVAL : USB_HS_INT_RNDIS_BINTERVAL;
			bIntervalOut  = USB_HS_BOUT_BINTERVAL;
		}
		else
		{
			maxBlkSizeLSB = USB_FS_MAX_BULK; // 64 byte max bulk size
			maxBlkSizeMSB = 0;
			maxIntSize    = USB_FS_MAX_INT;	// 8  byte max int   size
			bIntervalInt  = (index == 1) ? USB_FS_INT_CDC_BINTERVAL : USB_FS_INT_RNDIS_BINTERVAL;
			bIntervalOut  = 0;
		}
	}
	else
	{
		PRINTK("UsbProcessConfigDescRequest(): NOT CONFIGURATION_DESC\n");
		// Check our current operating speed.  If we're in High Speed mode, 
		// set the configuration descr. in the global buffer to FS values.
		if (usbCurrentMode == 0x00)
		{
			// new mode: USB FS
			maxBlkSizeLSB = USB_FS_MAX_BULK;
			maxBlkSizeMSB = 0;
			maxIntSize    = USB_FS_MAX_INT;	// 8  byte max int   size
			bIntervalInt  = (index == 1) ? USB_FS_INT_CDC_BINTERVAL : USB_FS_INT_RNDIS_BINTERVAL;
			bIntervalOut  = 0;
		}
		else
		{
			// new mode: USB HS
			maxBlkSizeLSB = (USB_HS_MAX_BULK % 256);
			maxBlkSizeMSB = (USB_HS_MAX_BULK / 256);
			maxIntSize    = USB_HS_MAX_INT;	// 64  byte max int  size
			bIntervalInt  = (index == 1) ? USB_HS_INT_CDC_BINTERVAL : USB_HS_INT_RNDIS_BINTERVAL;
			bIntervalOut  = USB_HS_BOUT_BINTERVAL;
		}
	}

	spin_lock_bh(&pDevCtx->usbnic_lock_msg);

	if (index == 1)
	{
	  // use the CDC set
	  configDescPtr = (UsbConfigDesc *)(pDevCtx->config2CfgDescOffset + (UINT8 *)pDevCtx->usbConfigBuffer);
	  endptDescPtr = (UsbEndpointDesc *)(pDevCtx->config2BlkEndptOffset + (UINT8 *)pDevCtx->usbConfigBuffer);
	  endptIntDescPtr = (UsbEndpointDesc *)(pDevCtx->config2IntEndptOffset + (UINT8 *)pDevCtx->usbConfigBuffer);
	}
	else
	{
	  // otherwise default to the RNDIS set
	  configDescPtr = (UsbConfigDesc *)(pDevCtx->config1CfgDescOffset + (UINT8 *)pDevCtx->usbConfigBuffer);
	  endptDescPtr = (UsbEndpointDesc *)(pDevCtx->config1BlkEndptOffset + (UINT8 *)pDevCtx->usbConfigBuffer);
	  endptIntDescPtr = (UsbEndpointDesc *)(pDevCtx->config1IntEndptOffset + (UINT8 *)pDevCtx->usbConfigBuffer);
	};

	pedp = endptDescPtr;

	PRINTK("UsbProcessConfigDescRequest(): configDescPtr %d (%d bytes)\n", index + 1, sizeof(UsbConfigDesc));
	DUMPADDR((UINT8 *)configDescPtr, sizeof(UsbConfigDesc));

	configDescPtr->bDescriptorType = desc;

	if (length > configDescPtr->wTotalLength_hi)
	{
		PRINTK("UsbProcessConfigDescRequest(): resetting length from %u to %d\n",
		  length, configDescPtr->wTotalLength_hi);
		length = configDescPtr->wTotalLength_hi;
	}

	PRINTK("Sending out a config descriptor: %02lx %d\n", (UINT32) configDescPtr, length);

	PRINTK ("CF, EP Before (%u)\n", length) ;
	DUMPADDR ((UINT8 *) configDescPtr, length) ;
	DUMPADDR ((UINT8 *) pedp, sizeof(UsbEndpointDesc) * 2) ;

	// fix up the Interrupt EP
	endptIntDescPtr->wMaxPacketSizeLSB = maxIntSize;
	endptIntDescPtr->wMaxPacketSizeMSB = 0;
	endptIntDescPtr->bInterval = bIntervalInt;
	PRINTK("UsbProcessConfigDescRequest(): ep 0x%02x psh %d psl %d bInt %d\n",
	  endptIntDescPtr->bEndpointAddress,
	  endptIntDescPtr->wMaxPacketSizeLSB,
	  endptIntDescPtr->wMaxPacketSizeMSB,
	  endptIntDescPtr->bInterval);
	DUMPADDR ((UINT8 *) endptIntDescPtr, sizeof(UsbEndpointDesc));

	// and now the Bulk EPs
	for (i = 0; i < 2; i++)
	{
		endptDescPtr->wMaxPacketSizeLSB = maxBlkSizeLSB;
		endptDescPtr->wMaxPacketSizeMSB = maxBlkSizeMSB;
		PRINTK("UsbProcessConfigDescRequest(): ep 0x%02X psh %d psl %d bInt %d\n",
		  endptDescPtr->bEndpointAddress,
		  endptDescPtr->wMaxPacketSizeLSB,
		  endptDescPtr->wMaxPacketSizeMSB,
		  endptDescPtr->bInterval);
		endptDescPtr++;
	}

	spin_unlock_bh(&pDevCtx->usbnic_lock_msg);

	PRINTK ("CF, EP After (now sending CF) %u\n", length) ;
	DUMPADDR ((UINT8 *) configDescPtr, length) ;
	DUMPADDR ((UINT8 *) pedp, sizeof(UsbEndpointDesc) * 2) ;

	// Respond with the configuration or other speed descriptors to the host.
	SCTLPKT( pDevCtx, (UINT8 *) configDescPtr, length, TRUE);
}

/*******************************************************************************
*
* ConvertAnsiToUnicode
*
* This routine onverts an ANSI string to a Unicode string by inserting a NULL
* character between each ANSI character.
*
* RETURNS: None.
*/

LOCAL void ConvertAnsiToUnicode( char *pszOut, char *pszIn )
{
    while( *pszIn )
    {
        *pszOut++ = *pszIn++;
        *pszOut++ = 0;
    }
} /* ConvertAnsiToUnicode */


/*******************************************************************************
*
* UsbNicSubmitControlPacket
*
* This routine sends a USB packet on the control pipe.  The packet buffer must
* be long word (32 bit) aligned.
*
* RETURNS: None.
*/

void UsbNicSubmitControlPacket( PUSBNIC_DEV_CONTEXT pDevCtx, UINT8 *pBuf1,
    UINT32 ulLength, UINT8 advanceCurr)
{
	UINT32 ulStatus;
	volatile DmaChannelCfg *pTxDma ;
	volatile DmaDesc *pBd ;
	UINT8 wrapOn = 0 ;
	UINT8 *bBuf, *pBuf;
	UINT32 i = 0 ;
	unsigned long irqFlags;

	pTxDma = pDevCtx->pTxCtrlDma ;
	pBd = pDevCtx->pTxCtrlBdNext;
	pBuf = bBuf = 0;

	if( (pTxDma->cfg & DMA_ENABLE) == 0 )
	{
		dma_cache_wback_inv((unsigned long) pBd, sizeof (DmaDesc));

		// Turns out the chip *can* sometimes handle unaligned accesses, but *sometimes* not.
		// This created a hard-to-diagnose issue when we'd get a descriptor that wasn't aligned
		// (sometimes they'd make it out OK, sometimes they wouldn't)
		// In any case, create a 32-bit aligned bounce buffer and send from there to be sure
      if(!(bBuf = kmalloc(MAX_CTRL_PKT_SIZE + 32, GFP_ATOMIC | GFP_DMA)))
      {
         printk("UsbNicSubmitControlPacket(): can't create bounce buffer\n");
         return;
      };

      pBuf = (UINT8 *)USB_NIC_ALIGN(bBuf, 16);

      if (ulLength != 0)
      {
         memcpy(pBuf, pBuf1, ulLength);
         dma_cache_wback_inv((unsigned long) pBuf, ulLength);
      DUMPADDR(pBuf, ulLength);
      }

		spin_lock_bh(&pDevCtx->usbnic_lock_msg);

		ulStatus = pTxDma->intStat;
		pTxDma->intStat = ulStatus;

		if (pBd->status & DMA_WRAP)
			wrapOn = 1 ;

		// This may be a zero length packet we're sending after recv'ing
		// a CONTROL transfer that had both SETUP and DATA stages.  If we
		// don't send a zero length packet back, the transaction will not
		// be completed.
	   pBd->address = (UINT32) VIRT_TO_PHYS(pBuf);	// just in case alignment matters here, too
      pBd->status =  DMA_SOP | DMA_EOP | DMA_OWN ;

		if (ulLength == 0) {
			PRINTK("UsbNicSubmitControlPacket(): ZLP\n");
			pBd->length = 1 ;
			pBd->status |= USB_ZERO_PKT;
		}
		else {
			pBd->length = ulLength;
		}

		if (wrapOn)
			pBd->status |= DMA_WRAP ;

      PRINTK ("USBD: dma-0x%x, bd-0x%x, buf-0x%x, len-%d, status-0x%x\n",(unsigned int) pTxDma, (unsigned int)pBd, (unsigned int)pBuf, pBd->length,
              (unsigned int)pBd->status) ;
		pTxDma->cfg = DMA_ENABLE;
		while(((volatile UINT8) pTxDma->cfg) & DMA_ENABLE) {
         if ((++i%0xffffffff)==0)
            printk ("USBD:L\n") ;
		  ;	// spin here 'till done to prevent descriptor corruption from next pkt in sequence
      }

		if( (pBd->status & DMA_WRAP) != 0 )
			pDevCtx->pTxCtrlBdNext = pDevCtx->pTxCtrlBdBase;
		else
			pDevCtx->pTxCtrlBdNext++;

		if (advanceCurr == TRUE)
		{
			spin_lock_irqsave(&pDevCtx->usbnic_lock_UsbIsr, irqFlags);
			pDevCtx->pTxCtrlBdCurr = pDevCtx->pTxCtrlBdNext ;
			spin_unlock_irqrestore(&pDevCtx->usbnic_lock_UsbIsr, irqFlags);
		}

		spin_unlock_bh(&pDevCtx->usbnic_lock_msg);

		if(bBuf)
			kfree(bBuf);
	}
	else
	{
		/* This should not happen. */
		printk(" Last control endpoint tramsmit has not completed.\n");
	}
} /* UsbNicSubmitControlPacket */


/*******************************************************************************
*
* UsbNicSubmitIntrPacket
*
* This routine sends a USB packet on the INTR pipe.  The packet buffer must
* be long word (32 bit) aligned.
*
* RETURNS: None.
*/

void UsbNicSubmitIntrPacket( PUSBNIC_DEV_CONTEXT pDevCtx, UINT8 *pBuf,
    UINT32 ulLength)
{
	UINT32 ulStatus;
	volatile DmaChannelCfg *pTxDma ;
	volatile DmaDesc *pBd ;

	pTxDma = pDevCtx->pTxIntrDma ;
	pBd = pDevCtx->pTxIntrBdBase ;

	PRINTK("UsbNicSubmitIntrPacket(): %u bytes\n", ulLength);
	if( (pTxDma->cfg & DMA_ENABLE) == 0 )
	{
		dma_cache_wback_inv((unsigned long) pBd, sizeof (DmaDesc));

		if (ulLength != 0)
		{
      dma_cache_wback_inv((unsigned long) pBuf, ulLength);
      DUMPADDR(pBuf, ulLength);
		};

		ulStatus = pTxDma->intStat;
		pTxDma->intStat = ulStatus;

		// This may be a zero length packet we're sending after recv'ing
		// a CONTROL transfer that had both SETUP and DATA stages.  If we
		// don't send a zero length packet back, the transaction will not
		// be completed.
		if (ulLength == 0) {
			pBd->address = (UINT32) VIRT_TO_PHYS(&nullMsg);
			pBd->length = 1 ;
			pBd->status =  DMA_WRAP | DMA_SOP | DMA_EOP | DMA_OWN | USB_ZERO_PKT;
		}
		else {
      pBd->address = (UINT32) VIRT_TO_PHYS(pBuf);
      pBd->length = ulLength;
      pBd->status =  DMA_WRAP | DMA_SOP | DMA_EOP | DMA_OWN;
		}

      //printk ("Intr Bd \n");
      //DUMPADDR (pBd, sizeof (DmaDesc)) ;

		pTxDma->cfg = DMA_ENABLE;
	}
	else
	{
		/* This should not happen. */
		printk(" Last Intr endpoint tramsmit has not completed.\n");
	}
} /* UsbNicSubmitIntrPacket */


/* --------------------------------------------------------------------------
  MACRO to call driver initialization and cleanup functions.
-------------------------------------------------------------------------- */
module_init(bcmusbnic_probe);
module_exit(bcmusbnic_cleanup);
MODULE_LICENSE("Proprietary");
MODULE_VERSION(VERSION);
