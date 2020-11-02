/*
<:copyright-BRCM:2015:proprietary:standard

   Copyright (c) 2015 Broadcom 
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
 * File Name  : bcmrndis.c
 *
 * Description: This file contains the implementation for the Remote NDIS
 *              connectionless device using USB.
 *
 * Updates    : 07/19/2002  lat.   Created.
 ***************************************************************************/

/* Defines. */
#define CARDNAME    "BCM63XX_USB"
#define VERSION     "0.4"
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

#include <asm/uaccess.h>

#include <bcmnet.h>
#include <bcmtypes.h>
#include <bcm_mm.h>
#include <bcm_map.h>
#include <bcm_intr.h>
#include <board.h>
#include "bcmrndis.h"
#include "bcmusb.h"

#if !defined(LOCAL)
#define LOCAL static
#endif

/* Externs. */

void UsbNicSubmitControlPacket( PUSBNIC_DEV_CONTEXT pDevCtx, UINT8 *pBuf,
    UINT32 ulLength, UINT8 advanceCurr);
void UsbNicSubmitIntrPacket( PUSBNIC_DEV_CONTEXT pDevCtx, UINT8 *pBuf,
    UINT32 ulLength);

/* Undefine LOCAL for debugging. */
#if defined(LOCAL)
#undef LOCAL
#define LOCAL
#endif

/* Prototypes. */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
irqreturn_t RndisCtrlRxIsr(int irq, void *dev_id);
#else
void RndisCtrlRxIsr( int irq, void *dev_id, struct pt_regs *regs );
#endif
void RndisProcessMsg( PUSBNIC_DEV_CONTEXT pDevCtx );
LOCAL void RndisProcessInitializeMsg( PUSBNIC_DEV_CONTEXT pDevCtx,
    PRNDIS_INITIALIZE_REQUEST pReqMsg, PRNDIS_MESSAGE pRspMsg );
LOCAL void RndisProcessHaltMsg( PUSBNIC_DEV_CONTEXT pDevCtx,
    PRNDIS_HALT_REQUEST pReqMsg, PRNDIS_MESSAGE pRspMsg );
LOCAL void RndisProcessQueryMsg( PUSBNIC_DEV_CONTEXT pDevCtx,
    PRNDIS_QUERY_REQUEST pReqMsg, PRNDIS_MESSAGE pRspMsg );
LOCAL void RndisProcessSetMsg( PUSBNIC_DEV_CONTEXT pDevCtx,
    PRNDIS_SET_REQUEST pReqMsg, PRNDIS_MESSAGE pRspMsg );
LOCAL void RndisProcessResetMsg( PUSBNIC_DEV_CONTEXT pDevCtx,
    PRNDIS_RESET_REQUEST pReqMsg, PRNDIS_MESSAGE pRspMsg );
LOCAL void RndisProcessKeepaliveMsg( PUSBNIC_DEV_CONTEXT pDevCtx,
    PRNDIS_KEEPALIVE_REQUEST pReqMsg, PRNDIS_MESSAGE pRspMsg );
LOCAL void RndisCreateIndicateStatusMsg( RNDIS_STATUS Status, UINT32 ulOffset,
    PRNDIS_MESSAGE pRspMsg );
void RndisDoGetEncapsulatedResponse( PUSBNIC_DEV_CONTEXT pDevCtx,
    UINT16 usLength );
LOCAL void ConvertUnicodeToAnsi( char *pszOut, char *pszIn );



/*******************************************************************************
*
* RndisCtrlRxIsr
*
* This routine is called when data has been received on the control endpoint.
* It schedules a function to run in the bottom half to process the received
* data.
*
* RETURNS: None.
*/
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
irqreturn_t RndisCtrlRxIsr(int irq, void *dev_id)
#else
void RndisCtrlRxIsr( int irq, void *dev_id, struct pt_regs *regs )
#endif
{
    PUSBNIC_DEV_CONTEXT pDevCtx = dev_id;
    volatile DmaChannelCfg *pRxCtrlDma = pDevCtx->pRxCtrlDma;
    UINT32 ulRxIntStat = pRxCtrlDma->intStat;

    //printk ("Entry RndisCtrlRxIsr \n") ;

    pRxCtrlDma->intStat = ulRxIntStat;  /* clear interrupt */

#ifdef USE_BH
    tasklet_schedule(&pDevCtx->BhRndisMsg);
#else
    RndisProcessMsg(pDevCtx);
#endif    
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
    return IRQ_HANDLED;
#endif  
} /* RndisDoSendEncapsultedCommand */


/*******************************************************************************
*
* RndisProcessMsg - Process RNDIS SEND_ENCAPSULATED_COMMAND message.
*
* This routine runs in the context of a bottom half.  It processes RNDIS
* request messages received on the control endpoint.
*
* RETURNS: None.
*/

void RndisProcessMsg( PUSBNIC_DEV_CONTEXT pDevCtx )
{
    PRNDIS_MESSAGE pReqMsg;
    PRNDIS_MESSAGE pRspMsg;
    PCTRL_PKT_BUFS pCtrlPkt;
    PCTRL_PKT_BUFS pTxCtrlPkt;
    UINT32 ulMsgType;
    volatile DmaDesc *pBd = pDevCtx->pRxCtrlBdNext;
    volatile DmaDesc *pTxBd ;
    UINT8 ucRspAvailable[] = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    unsigned long irqFlags;

    while( (pBd->status & DMA_OWN) == 0 )
	 {
		 if (!(pBd->status & DMA_SOP) || !(pBd->status & DMA_EOP)) {
			 printk ("!!!!!USBD: frameneted packets - no support \n") ;
			 goto _nextBd ;
		 }

		 if (pBd->length != 0) {
			 //pCtrlPkt = (PCTRL_PKT_BUFS) KSEG0ADDR(pBd->address);
			 pCtrlPkt = (PCTRL_PKT_BUFS) (phys_to_virt(pBd->address));

			 if( pDevCtx->waitForRndisMessage == TRUE) {

             //printk ("Sending zero pkt on CNTL channel \n") ;
				 UsbNicSubmitControlPacket (pDevCtx, (UINT8 *)0, 0, TRUE);

                spin_lock_bh(&pDevCtx->usbnic_lock_msg);

             pTxBd = pDevCtx->pTxCtrlBdCurr;

             //printk ("BdNext = %x BdCurr = %x \n", pDevCtx->pTxCtrlBdNext, pTxBd) ;

			    pTxCtrlPkt = &pDevCtx->pTxCtrlPktsBase [pDevCtx->txCtrlPktsIndex] ;
             pDevCtx->txCtrlPktsIndex = (pDevCtx->txCtrlPktsIndex + 1) & (NR_CTRL_BDS-1);

            spin_lock_irqsave(&pDevCtx->usbnic_lock_UsbIsr, irqFlags);
             pTxBd->address =  (UINT32) VIRT_TO_PHYS (pTxCtrlPkt);
            spin_unlock_irqrestore(&pDevCtx->usbnic_lock_UsbIsr, irqFlags);

             //printk ("Finished Sending zero pkt on CNTL channel \n") ;
				 pReqMsg = (PRNDIS_MESSAGE) pCtrlPkt->ulBuf;
				 pRspMsg = (PRNDIS_MESSAGE) pTxCtrlPkt->ulBuf;

				 /* Process the SEND_ENCAPSULATED_COMMAND message. */
				 memset( pRspMsg, 0x00, MAX_CTRL_PKT_SIZE );
             //printk ("RspMsg = %x \n", pRspMsg) ;
				 ulMsgType = BE_SWAP4(pReqMsg->NdisMessageType);

			    pDevCtx->waitForRndisMessage = FALSE ;

				 switch( ulMsgType )
				 {
					 case REMOTE_NDIS_INITIALIZE_MSG:
						 //printk ("[REMOTE_NDIS_INITIALIZE_MSG]\n");
						 RndisProcessInitializeMsg( pDevCtx,
								 &pReqMsg->Message.InitializeRequest, pRspMsg );
						 break;

					 case REMOTE_NDIS_HALT_MSG:
						 //printk ("[REMOTE_NDIS_HALT_MSG]\n");
						 RndisProcessHaltMsg( pDevCtx, &pReqMsg->Message.HaltRequest,
								 pRspMsg );
						 break;

					 case REMOTE_NDIS_QUERY_MSG:
						 //printk ("[REMOTE_NDIS_QUERY_MSG]\n");
						 RndisProcessQueryMsg(pDevCtx,&pReqMsg->Message.QueryRequest,
								 pRspMsg );
						 break;

					 case REMOTE_NDIS_SET_MSG:
						 //printk ("[REMOTE_NDIS_SET_MSG]\n");
						 RndisProcessSetMsg(pDevCtx, &pReqMsg->Message.SetRequest, pRspMsg);
						 break;

					 case REMOTE_NDIS_RESET_MSG:
						 //printk ("[REMOTE_NDIS_RESET_MSG]\n");
						 RndisProcessResetMsg( pDevCtx, &pReqMsg->Message.ResetRequest,
								 pRspMsg );
						 break;

					 case REMOTE_NDIS_KEEPALIVE_MSG:
						 //printk ("[REMOTE_NDIS_KEEPALIVE_MSG]\n");
						 RndisProcessKeepaliveMsg( pDevCtx,
								 &pReqMsg->Message.KeepaliveRequest, pRspMsg );
						 break;

						 ///////////////////////////////////////////////////////////////////////////
						 // Completion message types
						 //  - Some RNDIS messages are paired with a completion reply:
					 case REMOTE_NDIS_INITIALIZE_CMPLT_TYPE:
						 //printk ("[REMOTE_NDIS_INITIALIZE_CMPLT_TYPE]\n");
					 case REMOTE_NDIS_KEEPALIVE_CMPLT_TYPE:
						 //printk ("[REMOTE_NDIS_KEEPALIVE_CMPLT_TYPE]\n");
					 case REMOTE_NDIS_RESET_CMPLT_TYPE:
						 //printk ("[REMOTE_NDIS_RESET_CMPLT_TYPE]\n");
						 // Drop through...
					 case REMOTE_NDIS_QUERY_CMPLT_TYPE:
						 //printk ("[REMOTE_NDIS_QUERY_CMPLT_TYPE]\n");
					 case REMOTE_NDIS_SET_CMPLT_TYPE:
						 //printk ("[REMOTE_NDIS_SET_CMPLT_TYPE]\n");
						 // We've already sent the ack back, so we just
						 // proceed to the next BD from here.
						spin_unlock_bh(&pDevCtx->usbnic_lock_msg);
						 goto _nextBd ;
						 break ;

					 default:
						 //printk ("[RNDIS: NO SUPPORT]\n");
						 RndisCreateIndicateStatusMsg( RNDIS_STATUS_NOT_SUPPORTED, 0,
								 pRspMsg );
						 break;
				 }

            spin_lock_irqsave(&pDevCtx->usbnic_lock_UsbIsr, irqFlags);
             if( (pTxBd->status & DMA_WRAP) != 0 )
                pDevCtx->pTxCtrlBdCurr = pDevCtx->pTxCtrlBdBase;
             else
                pDevCtx->pTxCtrlBdCurr++;
            spin_unlock_irqrestore(&pDevCtx->usbnic_lock_UsbIsr, irqFlags);

				 if( ulMsgType != REMOTE_NDIS_HALT_MSG )
				 {
                //printk ("Sending intr packet ..\n") ;

					 /* Send a notification to the host on the interrupt endpoint
					  * so the response can be returned to the host in a
					  * GET_ENCAPSULATED_RESPONSE message.
					  */
					 UsbNicSubmitIntrPacket (pDevCtx, (UINT8 *) ucRspAvailable,
                      sizeof (ucRspAvailable));
				 }

                spin_unlock_bh(&pDevCtx->usbnic_lock_msg);
			 }
			 else {
             //printk ("processing ctrl msg \n") ;
				 UsbNicProcessCtrlMsg (pDevCtx, pCtrlPkt, pBd->length) ;
			 }

			 dma_cache_wback_inv((unsigned long)pCtrlPkt, MAX_CTRL_PKT_SIZE);
		 } /* if (pBd->length != 0) */
       else {
          //printk ("Rx BD length is 0. Discard BD : %x \n", pBd) ;
       }

_nextBd :
		spin_lock_bh(&pDevCtx->usbnic_lock_msg);
		 if( (pBd->status & DMA_WRAP) != 0 )
			 pDevCtx->pRxCtrlBdNext = pDevCtx->pRxCtrlBdBase;
		 else
			 pDevCtx->pRxCtrlBdNext++;

		 pBd->length = MAX_CTRL_PKT_SIZE;
		 pBd->status = (pBd->status & DMA_WRAP) | DMA_OWN;

		 pBd = pDevCtx->pRxCtrlBdNext;
		spin_unlock_bh(&pDevCtx->usbnic_lock_msg);
	 }

    /* Re-enable interrupts to resume packet reception. */
    BcmHalInterruptEnable(INTERRUPT_ID_USB_CNTL_RX_DMA);
} /* RndisProcessMsg */


/*******************************************************************************
*
* RndisProcessInitializeMsg
*
* This routine processes a RNDIS_INITIALIZE_REQUEST message and creates
* the RNDIS_INITALIZATION_COMPLETE response message.
*
* RETURNS: None.
*/
LOCAL void RndisProcessInitializeMsg( PUSBNIC_DEV_CONTEXT pDevCtx,
    PRNDIS_INITIALIZE_REQUEST pReqMsg, PRNDIS_MESSAGE pRspMsg )
{
    PRNDIS_INITIALIZE_COMPLETE pInitCmplt =
        &pRspMsg->Message.InitializeComplete;
    UINT32 ulMsgLen = sizeof(RNDIS_INITIALIZE_COMPLETE) +
        sizeof(pRspMsg->NdisMessageType)+sizeof(pRspMsg->MessageLength);

    pRspMsg->NdisMessageType = BE_SWAP4(REMOTE_NDIS_INITIALIZE_CMPLT);
    pRspMsg->MessageLength = BE_SWAP4(ulMsgLen);
    pInitCmplt->RequestId = pReqMsg->RequestId;
    pInitCmplt->Status = BE_SWAP4(RNDIS_STATUS_SUCCESS);
    pInitCmplt->MajorVersion = BE_SWAP4(RNDIS_MAJOR_VERSION);
    pInitCmplt->MinorVersion = BE_SWAP4(RNDIS_MINOR_VERSION);
    pInitCmplt->DeviceFlags = BE_SWAP4(RNDIS_DF_CONNECTIONLESS);
    pInitCmplt->Medium = BE_SWAP4(RNdisMedium802_3);
    pInitCmplt->MaxPacketsPerMessage = BE_SWAP4(1);
    pInitCmplt->MaxTransferSize = BE_SWAP4(RX_USB_NIC_BUF_SIZE);
    pInitCmplt->PacketAlignmentFactor = BE_SWAP4(3);

    pDevCtx->ulRndisState = RNDIS_STATE_INITIALIZED;

    /* Reset current MAC address. */
    memcpy( pDevCtx->ucCurrentHostMacAddr, pDevCtx->ucPermanentHostMacAddr,
        ETH_ALEN );

    //printk ("Rndis \n") ;
  	 //dumpaddr (pDevCtx->usbConfigBufferBase, 20) ;
} /* RndisProcessInitializeMsg */


/*******************************************************************************
*
* RndisProcessHaltMsg
*
* This routine processes a RNDIS_HALT_REQUEST message.
*
* RETURNS: None.
*/
LOCAL void RndisProcessHaltMsg( PUSBNIC_DEV_CONTEXT pDevCtx,
    PRNDIS_HALT_REQUEST pReqMsg, PRNDIS_MESSAGE pRspMsg )
{
    unsigned long irqFlags;

    if( pDevCtx->ulRndisState == RNDIS_STATE_INITIALIZED ||
        pDevCtx->ulRndisState == RNDIS_STATE_DATA_INITIALIZED )
    {
        /* Set link down and release buffers in the transmit queue. */

        spin_lock_irqsave(&pDevCtx->usbnic_lock_UsbIsr, irqFlags);
        pDevCtx->ulFlags &= ~USB_FLAGS_CONNECTED;
        spin_unlock_irqrestore(&pDevCtx->usbnic_lock_UsbIsr, irqFlags);

        pDevCtx->ulRndisState = RNDIS_STATE_UNINITIALIZED;

        /* Set message type to indicate to RndisDoGetEncapsulatedResponse
         * to not send response.
         */
        pRspMsg->NdisMessageType = REMOTE_NDIS_HALT_MSG;
    }
} /* RndisProcessHaltMsg */


/*******************************************************************************
*
* RndisProcessQueryMsg
*
* This routine processes a RNDIS_QUERY_REQUEST message and creates
* the RNDIS_QUERY_COMPLETE response message.
*
* RETURNS: None.
*/
LOCAL void RndisProcessQueryMsg( PUSBNIC_DEV_CONTEXT pDevCtx,
    PRNDIS_QUERY_REQUEST pReqMsg, PRNDIS_MESSAGE pRspMsg )
{
   UINT32 ulSupportedList[] =
   {
      RNDIS_OID_GEN_SUPPORTED_LIST,
      RNDIS_OID_GEN_HARDWARE_STATUS,
      RNDIS_OID_GEN_MEDIA_SUPPORTED,
      RNDIS_OID_GEN_MEDIA_IN_USE,
      RNDIS_OID_GEN_MAXIMUM_FRAME_SIZE,
      RNDIS_OID_GEN_LINK_SPEED,
      RNDIS_OID_GEN_TRANSMIT_BLOCK_SIZE,
      RNDIS_OID_GEN_RECEIVE_BLOCK_SIZE,
      RNDIS_OID_GEN_VENDOR_ID,
      RNDIS_OID_GEN_VENDOR_DESCRIPTION,
      RNDIS_OID_GEN_CURRENT_PACKET_FILTER,
      RNDIS_OID_GEN_MAXIMUM_TOTAL_SIZE,
      RNDIS_OID_GEN_MAC_OPTIONS,
      RNDIS_OID_GEN_MEDIA_CONNECT_STATUS,
      RNDIS_OID_GEN_PHYSICAL_MEDIUM,
      RNDIS_OID_GEN_RNDIS_CONFIG_PARAMETER,
      RNDIS_OID_GEN_XMIT_OK,
      RNDIS_OID_GEN_RCV_OK,
      RNDIS_OID_GEN_XMIT_ERROR,
      RNDIS_OID_GEN_RCV_ERROR,
      RNDIS_OID_GEN_RCV_NO_BUFFER,
      RNDIS_OID_802_3_PERMANENT_ADDRESS,
      RNDIS_OID_802_3_CURRENT_ADDRESS,
      RNDIS_OID_802_3_MULTICAST_LIST,
      RNDIS_OID_802_3_MAXIMUM_LIST_SIZE,
      RNDIS_OID_802_3_RCV_ERROR_ALIGNMENT,
      RNDIS_OID_802_3_XMIT_ONE_COLLISION,
      RNDIS_OID_802_3_XMIT_MORE_COLLISIONS
   };

   char szVendorDescription[] = "Broadcom USB RNDIS Driver ";
   PRNDIS_QUERY_COMPLETE pQueryCmplt = &pRspMsg->Message.QueryComplete;
   UINT8  *pucData = (UINT8  *) (pQueryCmplt + 1);
   UINT32 *pulData = (UINT32 *) pucData;
   UINT32 ulLen = sizeof(UINT32); /* most data lengths are this size */
   UINT32 i = 0;
   UINT32 ulMsgLen = sizeof(RNDIS_QUERY_COMPLETE) +
      sizeof(pRspMsg->NdisMessageType)+sizeof(pRspMsg->MessageLength);
   unsigned long irqFlags;

   pRspMsg->NdisMessageType = BE_SWAP4(REMOTE_NDIS_QUERY_CMPLT);
   pQueryCmplt->RequestId = pReqMsg->RequestId;
   pQueryCmplt->Status = BE_SWAP4(RNDIS_STATUS_SUCCESS);
   //printk ("QUERY_MSG_LENGTH: %ld\n", BE_SWAP4(pReqMsg->InformationBufferLength));


   switch( BE_SWAP4(pReqMsg->Oid) )
   {
      case RNDIS_OID_GEN_SUPPORTED_LIST:
         //printk ("[RNDIS_OID_GEN_SUPPORTED_LIST]\n");
         for( i = 0; i < sizeof(ulSupportedList) / sizeof(UINT32); i++ )
            pulData[i] = BE_SWAP4(ulSupportedList[i]);
         ulLen = sizeof(ulSupportedList);
         break;

      case RNDIS_OID_GEN_HARDWARE_STATUS:
         //printk ("[RNDIS_OID_GEN_HARDWARE_STATUS]\n");
         *pulData = BE_SWAP4(NdisHardwareStatusReady);
         break;

      case RNDIS_OID_GEN_MEDIA_SUPPORTED:
         //printk ("[RNDIS_OID_GEN_MEDIA_SUPPORTED]\n");
      case RNDIS_OID_GEN_MEDIA_IN_USE:
         //printk ("[RNDIS_OID_GEN_MEDIA_IN_USE]\n");
         *pulData = BE_SWAP4(RNdisMedium802_3);
         break;

      case RNDIS_OID_GEN_MAXIMUM_FRAME_SIZE:
         //printk ("[RNDIS_OID_GEN_MAXIMUM_FRAME_SIZE]\n");
         *pulData = BE_SWAP4(USB_MAX_FRAME_SIZE);
         break;

      case RNDIS_OID_GEN_TRANSMIT_BLOCK_SIZE:
         //printk ("[RNDIS_OID_GEN_TRANSMIT_BLOCK_SIZE]\n");
      case RNDIS_OID_GEN_RECEIVE_BLOCK_SIZE:
         //printk ("[RNDIS_OID_GEN_RECEIVE_BLOCK_SIZE]\n");
      case RNDIS_OID_GEN_MAXIMUM_TOTAL_SIZE:
         //printk ("[RNDIS_OID_GEN_MAXIMUM_TOTAL_SIZE]\n");
         if( pDevCtx->ulRndisState == RNDIS_STATE_INITIALIZED )
         {
            /* If the driver is not in the "data initialized" state, force
             * into it. This code is not compliant with the RNDIS spec but
             * resolves a problem running the WHQL HCT tests.
             */
            pDevCtx->usPacketFilterBitmap = NDIS_PACKET_TYPE_PROMISCUOUS |
               NDIS_PACKET_TYPE_DIRECTED | NDIS_PACKET_TYPE_MULTICAST;
            pDevCtx->ulRndisState = RNDIS_STATE_DATA_INITIALIZED;

            spin_lock_irqsave(&pDevCtx->usbnic_lock_UsbIsr, irqFlags);
            pDevCtx->ulFlags |= USB_FLAGS_CONNECTED;
            spin_unlock_irqrestore(&pDevCtx->usbnic_lock_UsbIsr, irqFlags);
         }

         *pulData = BE_SWAP4(USB_MAX_MTU_SIZE);
         break;

      case RNDIS_OID_GEN_LINK_SPEED:
         //printk ("[RNDIS_OID_GEN_LINK_SPEED]\n");
         /* The link speed is suppose to refer to the speed of the remote link
          * (ADSL or Ethernet) with the assumption that the device is a dongle.
          * However, this device is a router and the USB is viewed as a link
          * (or connection), not a bus.  Therefore, requests for link speed
          * and link up/down status will be for the USB and not ADSL or Ethernet.
          */
         ////printk("[RNDIS_OID_GEN_LINK_SPEED]\n");
         // Check what mode we enumerated in.  If High Speed, we'll
         // report 480 Mbps and if in full speed, we'll report 12 Mbps
         if (((USB->usbd_status & USBD_ENUM_SPEED) >> USBD_ENUM_SPEED_SHIFT) == 0x00)
         {
            *pulData = BE_SWAP4(4800000);
         }
         else
         {
            *pulData = BE_SWAP4(120000); /* 12 Mbps in units of 100 */
         }
         break;

      case RNDIS_OID_GEN_VENDOR_ID:
         //printk ("[RNDIS_OID_GEN_VENDOR_ID]\n");
         pucData[0] = pDevCtx->ucPermanentHostMacAddr[0];
         pucData[1] = pDevCtx->ucPermanentHostMacAddr[1];
         pucData[2] = pDevCtx->ucPermanentHostMacAddr[2];
         pucData[3] = 0x00;
         break;

      case RNDIS_OID_GEN_VENDOR_DESCRIPTION:
         //printk ("[RNDIS_OID_GEN_VENDOR_DESCRIPTION]\n");
         memcpy( pucData, szVendorDescription, sizeof(szVendorDescription) );
         ulLen = sizeof(szVendorDescription);
         break;

      case RNDIS_OID_GEN_CURRENT_PACKET_FILTER:
         //printk ("[RNDIS_OID_GEN_CURRENT_PACKET_FILTER]\n");
         *pulData = BE_SWAP4((UINT32) pDevCtx->usPacketFilterBitmap);
         break;

      case RNDIS_OID_GEN_MAC_OPTIONS:
         //printk ("[RNDIS_OID_GEN_MAC_OPTIONS]\n");
         *pulData = BE_SWAP4((UINT32) NDIS_MAC_OPTION_NO_LOOPBACK);
         break;

      case RNDIS_OID_GEN_MEDIA_CONNECT_STATUS:
         //printk ("[RNDIS_OID_GEN_MEDIA_CONNECT_STATUS]\n");
         *pulData = BE_SWAP4(RNdisMediaStateConnected);
         break;

      case RNDIS_OID_GEN_PHYSICAL_MEDIUM:
         //printk ("[RNDIS_OID_GEN_PHYSICAL_MEDIUM]\n");
         *pulData = BE_SWAP4((UINT32) NdisPhysicalMediumDSL);
         break;

      case RNDIS_OID_GEN_XMIT_OK:
         //printk ("[RNDIS_OID_GEN_XMIT_OK]\n");
         /* Transmit OK corresponds to successfully receiving from the RNDIS
          * driver.
          */
         i = pDevCtx->DevStats.rx_packets - pDevCtx->DevStats.rx_errors;
         *pulData = BE_SWAP4(i);
         break;

      case RNDIS_OID_GEN_RCV_OK:
         //printk ("[RNDIS_OID_GEN_RCV_OK]\n");
         /* Receive OK corresponds to successfully sending to the RNDIS driver.*/
         i = pDevCtx->DevStats.tx_packets -
            (pDevCtx->DevStats.tx_dropped + pDevCtx->DevStats.tx_aborted_errors);
         *pulData = BE_SWAP4(i);
         break;

      case RNDIS_OID_GEN_XMIT_ERROR:
         //printk ("[RNDIS_OID_GEN_XMIT_ERROR]\n");
         /* RNDIS transmit errors mean that this driver cannot receive from the
          * RNDIS driver.
          */
         *pulData = BE_SWAP4(pDevCtx->DevStats.rx_errors);
         break;

      case RNDIS_OID_GEN_RCV_ERROR:
         //printk ("[RNDIS_OID_GEN_RCV_ERROR]\n");
         /* RNDIS receive errors means that this driver cannot send to the
          * RNDIS driver.  The tx_dropped field means that the USB cable was
          * unplugged when a packet was available to be transmitted.
          */
         i = pDevCtx->DevStats.tx_dropped;
         *pulData = BE_SWAP4(i);
         break;

      case RNDIS_OID_GEN_RCV_NO_BUFFER:
         //printk ("[RNDIS_OID_GEN_RCV_NO_BUFFER]\n");
         /* No BD is available on the transmit queue. */
         i = pDevCtx->DevStats.tx_aborted_errors;
         *pulData = BE_SWAP4(i);
         break;

      case RNDIS_OID_802_3_PERMANENT_ADDRESS:
         //printk ("[RNDIS_OID_802_3_PERMANENT_ADDRESS]\n");
         memcpy( pucData, pDevCtx->ucPermanentHostMacAddr, ETH_ALEN );
         ulLen = ETH_ALEN;
         break;

      case RNDIS_OID_802_3_CURRENT_ADDRESS:
         //printk ("[RNDIS_OID_802_3_CURRENT_ADDRESS]\n");
         memcpy( pucData, pDevCtx->ucCurrentHostMacAddr, ETH_ALEN );
         ulLen = ETH_ALEN;
         break;

      case RNDIS_OID_802_3_MULTICAST_LIST:
         //printk ("[RNDIS_OID_802_3_MULTICAST_LIST]\n");
         ulLen = pDevCtx->ucRndisNumMulticastListEntries * ETH_ALEN;
         memcpy( pucData, pDevCtx->ucRndisMulticastList, ulLen );
         break;

      case RNDIS_OID_802_3_MAXIMUM_LIST_SIZE:
         //printk ("[RNDIS_OID_802_3_MAXIMUM_LIST_SIZE]\n");
         *pulData = BE_SWAP4(MAX_MULTICAST_ADDRESSES);
         break;

      case RNDIS_OID_802_3_RCV_ERROR_ALIGNMENT:
         //printk ("[RNDIS_OID_802_3_RCV_ERROR_ALIGNMENT]\n");
      case RNDIS_OID_802_3_XMIT_ONE_COLLISION:
         //printk ("[RNDIS_OID_802_3_XMIT_ONE_COLLISION]\n");
      case RNDIS_OID_802_3_XMIT_MORE_COLLISIONS:
         //printk ("[RNDIS_OID_802_3_XMIT_MORE_COLLISIONS]\n");
         /* The RNDIS spec assumes that this driver is controlling a remote
          * 802.3 connection.  This driver is not.  It treats the USB connection
          * as an 802.3 link.  A USB connection cannot get these errors.
          */
         *pulData = 0;
         break;

      default:
         //printk ("[Rndis default]\n");
         *pulData = 0;
         break;
   }

   ulMsgLen += ulLen;
   pRspMsg->MessageLength = BE_SWAP4(ulMsgLen);

   if( ulLen )
   {
      pQueryCmplt->InformationBufferLength = BE_SWAP4(ulLen);
      pQueryCmplt->InformationBufferOffset =
         BE_SWAP4(sizeof(RNDIS_QUERY_COMPLETE));
   }
} /* RndisProcessQueryMsg */


/*******************************************************************************
*
* RndisProcessSetMsg
*
* This routine processes a RNDIS_SET_REQUEST message and creates
* the RNDIS_SET_COMPLETE response message.
*
* RETURNS: None.
*/
LOCAL void RndisProcessSetMsg( PUSBNIC_DEV_CONTEXT pDevCtx,
    PRNDIS_SET_REQUEST pReqMsg, PRNDIS_MESSAGE pRspMsg )
{
    PRNDIS_SET_COMPLETE pSetCmplt = &pRspMsg->Message.SetComplete;
    UINT8  *pucData =
        ((UINT8  *) pReqMsg + BE_SWAP4(pReqMsg->InformationBufferOffset));
    UINT32 *pulData = (UINT32 *) pucData;
    UINT32 ulReqBufLen = BE_SWAP4(pReqMsg->InformationBufferLength);
    UINT32 ulSts = RNDIS_STATUS_SUCCESS;
    UINT32 ulMsgLen = sizeof(RNDIS_SET_COMPLETE) +
        sizeof(pRspMsg->NdisMessageType)+sizeof(pRspMsg->MessageLength);
    char szParm[64] = "";
    unsigned long irqFlags;

    pRspMsg->NdisMessageType = BE_SWAP4(REMOTE_NDIS_SET_CMPLT);
    pRspMsg->MessageLength = BE_SWAP4(ulMsgLen);
    pSetCmplt->RequestId = pReqMsg->RequestId;

    switch( BE_SWAP4(pReqMsg->Oid) )
    {
    case RNDIS_OID_802_3_MULTICAST_LIST:
        //printk ("[RNDIS_OID_802_3_MULTICAST_LIST] %ld\n", ulReqBufLen);
        if( (ulReqBufLen % ETH_ALEN) != 0 )
            ulSts = RNDIS_STATUS_INVALID_LENGTH;
        else
            if( ulReqBufLen / ETH_ALEN > MAX_MULTICAST_ADDRESSES )
                ulSts = RNDIS_STATUS_MULTICAST_FULL;
            else
            {
                pDevCtx->ucRndisNumMulticastListEntries = ulReqBufLen/ETH_ALEN;
                memcpy( pDevCtx->ucRndisMulticastList, pucData, ulReqBufLen );
            }
        break;

    case RNDIS_OID_GEN_CURRENT_PACKET_FILTER:
        //printk ("[RNDIS_OID_GEN_CURRENT_PACKET_FILTER]\n");
        if( ulReqBufLen == sizeof(UINT32) )
        {
            pDevCtx->usPacketFilterBitmap = (UINT16) BE_SWAP4(*pulData);
            if( pDevCtx->ulRndisState != RNDIS_STATE_UNINITIALIZED )
            {
                if( *pulData )
                {
                    /* Data can be sent and received. */
                    pDevCtx->ulRndisState = RNDIS_STATE_DATA_INITIALIZED;

                    spin_lock_irqsave(&pDevCtx->usbnic_lock_UsbIsr, irqFlags);
                    pDevCtx->ulFlags |= USB_FLAGS_CONNECTED;
                    spin_unlock_irqrestore(&pDevCtx->usbnic_lock_UsbIsr, irqFlags);
                }
                else
                {
                    pDevCtx->ulRndisState = RNDIS_STATE_INITIALIZED;

                    spin_lock_irqsave(&pDevCtx->usbnic_lock_UsbIsr, irqFlags);
                    pDevCtx->ulFlags &= ~USB_FLAGS_CONNECTED;
                    spin_unlock_irqrestore(&pDevCtx->usbnic_lock_UsbIsr, irqFlags);
                }
            }
            else
                ulSts = RNDIS_STATUS_INVALID_DATA;
        }
        else
            ulSts = RNDIS_STATUS_INVALID_LENGTH;
        break;

    case RNDIS_OID_GEN_RNDIS_CONFIG_PARAMETER:
        //printk ("[RNDIS_OID_GEN_RNDIS_CONFIG_PARAMETER]\n");
        if( ulReqBufLen < sizeof(szParm) * 2 )
        {
            char szNetAddr[] = "NetworkAddress";

            szParm[0] = '\0';
            ConvertUnicodeToAnsi(szParm, (char *) pucData + BE_SWAP4(*pulData));
            if( !strncmp( szParm, szNetAddr, sizeof(szNetAddr) - 1 ) )
            {
                char *p = szParm + sizeof(szNetAddr) - 1;
                int i;
                char ch;
                UINT8 hn, ln;

                /* A new host MAC address is being set.  It was passed as a
                 * Unicode character string.  It has been coverted into an
                 * ANSI string and stored in szParm.  Now convert it into a
                 * binary MAC address.
                 */
                for( i = 0; i < ETH_ALEN; i++ )
                {
                    /* high nibble */
                    ch = *p++, hn = 0, ln = 0;
                    if( ch >= '0' && ch <= '9' )
                        hn = ch - '0';
                    else
                        if( ch >= 'A' && ch <= 'F' )
                            hn = ch - 'A';
                        else
                            if( ch >= 'a' && ch <= 'f' )
                                hn = ch - 'a';

                    /* low nibble */
                    ch = *p++, hn = 0, ln = 0;
                    if( ch >= '0' && ch <= '9' )
                        ln = ch - '0';
                    else
                        if( ch >= 'A' && ch <= 'F' )
                            ln = ch - 'A';
                        else
                            if( ch >= 'a' && ch <= 'f' )
                                ln = ch - 'a';

                    pDevCtx->ucCurrentHostMacAddr[i] = (hn << 4) | ln;
                }
            }
        }
        break;
    }

    pSetCmplt->Status = BE_SWAP4(ulSts);
} /* RndisProcessSetMsg */


/*******************************************************************************
*
* RndisProcessResetMsg
*
* This routine processes a RNDIS_RESET_REQUEST message and creates
* the RNDIS_RESET_COMPLETE response message.
*
* RETURNS: None.
*/
LOCAL void RndisProcessResetMsg( PUSBNIC_DEV_CONTEXT pDevCtx,
    PRNDIS_RESET_REQUEST pReqMsg, PRNDIS_MESSAGE pRspMsg )
{
    PRNDIS_RESET_COMPLETE pResetCmplt = &pRspMsg->Message.ResetComplete;
    UINT32 ulMsgLen = sizeof(RNDIS_RESET_COMPLETE) +
        sizeof(pRspMsg->NdisMessageType)+sizeof(pRspMsg->MessageLength);
    unsigned long irqFlags;

    pRspMsg->NdisMessageType = BE_SWAP4(REMOTE_NDIS_RESET_CMPLT);
    pRspMsg->MessageLength = BE_SWAP4(ulMsgLen);
    pResetCmplt->Status = BE_SWAP4(RNDIS_STATUS_SUCCESS);
    pResetCmplt->AddressingReset = BE_SWAP4(0);

    /* Soft reset is processed by timer function, usbnic_timer. */
    spin_lock_irqsave(&pDevCtx->usbnic_lock_UsbIsr, irqFlags);
    pDevCtx->ulFlags |= USB_FLAGS_RNDIS_SOFT_RESET;
    spin_unlock_irqrestore(&pDevCtx->usbnic_lock_UsbIsr, irqFlags);
} /* RndisProcessResetMsg */


/*******************************************************************************
*
* RndisProcessKeepaliveMsg
*
* This routine processes a RNDIS_KEEPALIVE_REQUEST message and creates
* the RNDIS_KEEPALIVE_COMPLETE response message.
*
* RETURNS: None.
*/
LOCAL void RndisProcessKeepaliveMsg( PUSBNIC_DEV_CONTEXT pDevCtx,
    PRNDIS_KEEPALIVE_REQUEST pReqMsg, PRNDIS_MESSAGE pRspMsg )
{
    PRNDIS_KEEPALIVE_COMPLETE pKeepaliveCmplt =
        &pRspMsg->Message.KeepaliveComplete;
    UINT32 ulMsgLen = sizeof(RNDIS_KEEPALIVE_COMPLETE) +
        sizeof(pRspMsg->NdisMessageType)+sizeof(pRspMsg->MessageLength);

    pRspMsg->NdisMessageType = BE_SWAP4(REMOTE_NDIS_KEEPALIVE_CMPLT);
    pRspMsg->MessageLength = BE_SWAP4(ulMsgLen);
    pKeepaliveCmplt->RequestId = pReqMsg->RequestId;
    pKeepaliveCmplt->Status = BE_SWAP4(RNDIS_STATUS_SUCCESS);
} /* RndisProcessKeepaliveMsg */


/*******************************************************************************
*
* RndisCreateIndicateStatusMsg
*
* This creates an RNDIS_INDICATE_STATUS response message.
*
* RETURNS: None.
*/
LOCAL void RndisCreateIndicateStatusMsg( RNDIS_STATUS Status, UINT32 ulOffset,
    PRNDIS_MESSAGE pRspMsg )
{
    PRNDIS_INDICATE_STATUS pIndicateStatus =
        &pRspMsg->Message.IndicateStatus;
    PRNDIS_DIAGNOSTIC_INFO pDiagInfo =
        (PRNDIS_DIAGNOSTIC_INFO) (pIndicateStatus + 1);
    UINT32 ulMsgLen=sizeof(RNDIS_INDICATE_STATUS)+sizeof(RNDIS_DIAGNOSTIC_INFO)+
        sizeof(pRspMsg->NdisMessageType)+sizeof(pRspMsg->MessageLength);

    pRspMsg->NdisMessageType = BE_SWAP4(REMOTE_NDIS_INDICATE_STATUS_MSG);
    pRspMsg->MessageLength = BE_SWAP4(ulMsgLen);
    pIndicateStatus->Status = BE_SWAP4(RNDIS_STATUS_SUCCESS);
    pIndicateStatus->StatusBufferLength =
        BE_SWAP4(sizeof(RNDIS_DIAGNOSTIC_INFO));
    pIndicateStatus->StatusBufferOffset =
        BE_SWAP4(sizeof(RNDIS_INDICATE_STATUS));
    pDiagInfo->DiagStatus = BE_SWAP4(Status);
    pDiagInfo->ErrorOffset = BE_SWAP4(ulOffset);
} /* RndisCreateIndicateStatusMsg */


/*******************************************************************************
*
* RndisDoGetEncapsulatedResponse
*
* This routine sends an RNDIS GET_ENCAPSULATED_RESPONSE message on the control
* pipe.
*
* RETURNS: None.
*/

void RndisDoGetEncapsulatedResponse(PUSBNIC_DEV_CONTEXT pDevCtx, UINT16 usLength)
{
    volatile DmaDesc *pBd = pDevCtx->pTxCtrlBdNext;
    UINT32 ulSendNull = 1;
    PRNDIS_MESSAGE pRspMsg;
    PCTRL_PKT_BUFS pCtrlPkt;
    UINT32 ulLen;
    UINT8 ucRspAvailable[] = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    //printk ("Bd = %x CurrBd = %x \n", pBd, pDevCtx->pTxCtrlBdCurr) ;

    /* Send a response if the queue that contains the response is not empty. */
    while( pBd != pDevCtx->pTxCtrlBdCurr )
    {
		  pCtrlPkt = (PCTRL_PKT_BUFS) (phys_to_virt(pBd->address));
        pRspMsg = (PRNDIS_MESSAGE) pCtrlPkt->ulBuf;
        ulLen = BE_SWAP4(pRspMsg->MessageLength);

        /* If the request was a Halt message, skip to the next BD because there
         * is no response for a Halt message.
         */
        if( pRspMsg->NdisMessageType == REMOTE_NDIS_HALT_MSG )
        {
            spin_lock_bh(&pDevCtx->usbnic_lock_msg);
            if( (pBd->status & DMA_WRAP) != 0 )
                pDevCtx->pTxCtrlBdNext = pDevCtx->pTxCtrlBdBase;
            else
                pDevCtx->pTxCtrlBdNext++;
            pBd = pDevCtx->pTxCtrlBdNext;
            spin_unlock_bh(&pDevCtx->usbnic_lock_msg);

            ulSendNull = 0;
            continue;
        }

        /* Send the response if the length is OK. */
        if( ulLen <= usLength )
        {
            UsbNicSubmitControlPacket( pDevCtx, (UINT8 *) pRspMsg, ulLen,
                                       FALSE);
            ulSendNull = 0;
        }
        else
            ulSendNull = 1;

        spin_lock_bh(&pDevCtx->usbnic_lock_msg);
        pBd = pDevCtx->pTxCtrlBdNext;
        spin_unlock_bh(&pDevCtx->usbnic_lock_msg);
    }

    if( ulSendNull == 1 )
    {
        /* A response message could not be sent. Send a one byte packet of 0. */
        UINT8 *p = (UINT8 *) &ucRspAvailable[0]; /* arbitrary unused field */
        *p = 0;
        UsbNicSubmitControlPacket( pDevCtx, p, 1, TRUE);
    }
} /* RndisDoGetEncapsulatedResponse */


/*******************************************************************************
*
* ConvertUnicodeToAnsi
*
* This routine onverts a Unicdoe string to an ANSI string by removing the
* least significant byte of each short word.
*
* RETURNS: None.
*/

LOCAL void ConvertUnicodeToAnsi( char *pszOut, char *pszIn )
{
    while( *pszIn )
    {
        *pszOut++ = *pszIn++;
        pszIn++;
    }
} /* ConvertUnicodeToAnsi*/

MODULE_LICENSE("GPL");
MODULE_VERSION(VERSION);
