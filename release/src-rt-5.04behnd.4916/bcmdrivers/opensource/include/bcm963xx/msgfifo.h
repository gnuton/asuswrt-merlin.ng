/****************************************************************************
 *
 * Broadcom Proprietary and Confidential.
 * (c) 2016 Broadcom. All rights reserved.
 * The term "Broadcom" refers to Broadcom Limited and/or its subsidiaries.
 *
 * Unless you and Broadcom execute a separate written software license
 * agreement governing use of this software, this software is licensed to
 * you under the terms of the GNU General Public License version 2 (the
 * "GPL"), available at [http://www.broadcom.com/licenses/GPLv2.php], with
 * the following added to such license:
 *
 * As a special exception, the copyright holders of this software give you
 * permission to link this software with independent modules, and to copy
 * and distribute the resulting executable under terms of your choice,
 * provided that you also meet, for each linked independent module, the
 * terms and conditions of the license of that module. An independent
 * module is a module which is not derived from this software. The special
 * exception does not apply to any modifications of the software.
 *
 * Notwithstanding the above, under no circumstances may you combine this
 * software in any way with any other Broadcom software provided under a
 * license other than the GPL, without Broadcom's express prior written
 * consent.
 *
 ****************************************************************************/

#ifndef __MSGFIFO_H__
#define __MSGFIFO_H__

#if defined __cplusplus
extern "C" {
#endif
/*
 * Visual SlickEdit gets confused by the open { above, and forces the code that
 * follows to be indented.  This close } satisfies the editor and makes life
 * easier.  Because it's inside the #if 0, the compiler will ignore it.
 */
#if 0
}
#endif
/*
 * Maximum message offset length.
 * This length needs to be agreed upon by all parties so there is
 * no misunderstanding on what the offset size to be used is on all cores.
 * This size needs to cover max wifi header + VLAN headers + GRE tunnel headers, etc.
 * 16B Cache alligned
 * NOTE: Since wifi grabs 1952B packets for headers we are using the remainder
 * to stay within 2048 which currently is our max size.
 */
#define MAX_MSG_OFFSET          240

/* Token Defines */
#define TKN_VALID_TKN_BIT       0x80000000
#define TKN_VALID_TKN_SHIFT     31
#define TKN_RSVD_BIT            0x40000000
#define TKN_RSVD_SHIFT          30
#define TKN_BUF_INDX_MASK       0x3FFFF000
#define TKN_BUF_INDX_SHIFT      12
#define TKN_BUF_POOL_NUM_MASK   0x30000000
#define TKN_BUF_POOL_NUM_SHIFT  28
#define TKN_BUF_POOL_INDX_MASK  0x0FFFF000
#define TKN_BUF_POOL_INDX_SHIFT 12
#define TKN_SIZE_MASK           0x00000FFF
#define TKN_SIZE_SHIFT          0

/* FPM buffer size bitfield definitions that everyone needs to */
/* know what the minimum FPM buffer size being used is */
enum fpmbufsizevalue
{
    FpmBufSizeVal512 = 0,  /* NOTE: This is for 3380 legacy support 512-2304 */
    FpmBufSizeVal768,
    FpmBufSizeVal1024,
    FpmBufSizeVal1280,
    FpmBufSizeVal1536,
    FpmBufSizeVal1792,
    FpmBufSizeVal2048,
    FpmBufSizeVal256
};


/* Data Message Defines. */
enum msgids
{
    /* Start of Incoming/Outgoing MsgIds */
    LANRxMsg = 0,
    LANTxMsg = 1,
    TXStatusMsg = 2,
    DSPktMsg = 3,
    DSBpiSeqErrMsg = 4,
    DSPhsDiscardMsg = 5,
    USPPRequestMsg = 6,
    USPPResponseMsg = 7,
    DSPacketRxMsg = 8,
    LANPrePendMsg = 9,
    PacketOffloadSupportMsg = 10,
    USPacketTxMsg = 11,
    USReleaseTokenMsg = 12,
    USPPTxStatusMsg = 13,
    ItcRpcServiceMsg = 14,
    USLegacyMsg,
    LANRxMsgSkb = 30,
    LANTxMsgSkb = 31,
    /* Start of DQM MsgIds */
    CREATEFlowCmdMsg = 32,
    REMOVEFlowCmdMsg,
    CREATEFlowRspMsg,
    REMOVEFlowRspMsg,
    USPacketMsg,
    SHOWMibsCmdMsg,
    CLEARMibsCmdMsg,
    FLUSHFlowCmdMsg,
    fTESTModeCmdMsg,
    /* Start of test MsgIds */
    INSERTTokenMsg = 58,
    FLOWNotInserviceMsg = 59,
    TESTStart = 60,
    TESTStop,
    TESTEndPass,
    TESTEndFail
};
/*
 * Data Types.
 * General structure of the message sent to the fifo.
 * The data portion will hold the actual message body.
 */
struct msgstruct
{
    enum msgids msgid;       /* Will be or'ed in to first word. 6 MSB of first word. */
    uint32_t      msglen;      /* In 32 bit words. */
    uint32_t      destaddr;    /* UBUS destination address if use for outgoing msg fifo */
    /*UBUS source address if use for incoming msg fifo */
    /*DQM queue number(0-31) if use for DQM */
    uint32_t      msgdata[16]; /*Provides space for 16 words including headers. */
};

struct msgstats_s
{
    uint32_t srctkncnt;
    uint32_t dsttkncnt;
    uint32_t srcrlscnt;
    uint32_t dstrlscnt;
};

struct msg_s
{
    enum msgids msgid;      /*Will be or'ed in to first word. 6 MSB of first word. */
    uint16_t      dqmqueue;   /*DQM queue number(0-31), for FPGA only(0-5)*/
    uint32_t      msglen;     /*In 32 bit words.*/
    uint32_t      ubusaddr;   /*UBUS destination address if use for outgoing msg fifo*/
    /*UBUS source address if use for incoming msg fifo*/
    uint32_t     *msgdata;    /*pointer to data buffer*/
    struct msgstats_s *msgstats;
};

/*Message ID, READ and WRITE macro.*/
#define ID_MASK              0xfc000000
#define ID_SHIFT             26
#define MAC_ID_MASK          0x03c00000
#define MAC_ID_SHIFT         22
#define SUB_ID_MASK          0x0000f000
#define SUB_ID_SHIFT         12
#define WRITE_HDR_FIELD(msg,val,mask,shift)  (msg = ((val<<shift) & mask) | (msg & ~mask))
#define GET_MSG_ID(msg)                      (((msg&ID_MASK) >> ID_SHIFT))
#define READ_HDR_FIELD(msg,val,mask,shift)   (val = ((msg & mask)>>shift))
#define COMPARE_FIELD(msg, val, mask, shift) (((val << shift) & mask) != (msg & mask))

#define MAC_ID_UNIMAC      0  // DFAP Unimac
#define MAC_ID_ETHSW_0     1  // Switch Port 0 // RDD_LAN0_VPORT=RDD_VPORT_ID_1
#define MAC_ID_ETHSW_1     2  // Switch Port 1 // RDD_LAN1_VPORT=RDD_VPORT_ID_2
#define MAC_ID_ETHSW_2     3  // Switch Port 2 // RDD_LAN2_VPORT=RDD_VPORT_ID_3
#define MAC_ID_ETHSW_3     4  // Switch Port 3 // RDD_LAN3_VPORT=RDD_VPORT_ID_4
#define MAC_ID_ETHSW_4     5  // Switch Port 4 // RDD_LAN4_VPORT=RDD_VPORT_ID_5
#define MAC_ID_MOCA        5  // Moca          // RDD_LAN4_VPORT=RDD_VPORT_ID_5
#define MAC_ID_STB         6  //
#define MAC_ID_ETHSW_7     6  // ETH 2.5G Port // RDD_LAN5_VPORT=RDD_VPORT_ID_6
#define MAC_ID_WIFI0       8  // Wifi 0        // RDD_WIFI_VPORT=RDD_VPORT_ID_8  SUB_ID is (SSID)
#define MAC_ID_WIFI1       9  // Wifi 1
#define MAC_ID_WIFI2       10 // Wifi 2
#define MAC_ID_LINUXHOST   12 // Linux Host
#define MAC_ID_EMTA        13 // EMTA / Voice
#define MAC_ID_CMHOST      14 // Cablemodem Host / IPStacks
#define MAC_ID_CM          15 // DS/US
#define MAC_ID_EROUTER     MAC_ID_LINUXHOST
#define MAC_ID_ETHWAN      MAC_ID_UNIMAC

#define MAC_ID_ETHSW       0  // Switch Connection. (RouterUnimac 0,1). Note: SubId could be used for switch port.
#define MAC_ID_ETH         1  // Ethernet Connection Alternate  (StbUnimac)
#define MAC_ID_STB_B       13 // Set Top Bridged Connection.
#define MAC_ID_STB_R       14 // Set Top Routed  Connection.

/* Generic Token Msg*/
/* This is used to overlay various Message Types with common fields used by many of them*/
struct generictknmsgt
{
  union {
      struct {
          uint32_t msgid                    :6;   /* Message ID*/
          uint32_t macid                    :4;   /* MAC ID number*/
          uint32_t reserved0                :2;   /* Reserved*/
          uint32_t qos                      :4;   /* Tx QoS Tag*/
          uint32_t reserved1                :4;   /* Reserved*/
          uint32_t endofpacket              :1;   /* End of Packet  - last buffer for this packet (chaining)*/
          uint32_t offset                   :11;  /* Message offset*/
      } bits;
      uint32_t reg32;
  };
	uint32_t token;                           /* Token is always the second word*/
};

/*LAN RX Message*/
/*Producer  LAN MAC DMA*/
/*Consumer  RX Message Processor (4ke)*/
struct lanrxmsg
{
    uint32_t msghdr;  /* Uses GenericTknMsgT to overlay*/
        #define LANRX_MAC_ID_MASK     0x03C00000
        #define LANRX_MAC_ID_SHIFT    22
        #define LANRX_QOS_MASK        0x000f0000
        #define LANRX_QOS_SHIFT       16
    uint32_t token;
};


/*LAN RX Offset Message or LAN RX SKB Message*/
struct lanrxoffsetmsg
{
    uint32_t msghdr;  /* Uses GenericTknMsgT to overlay*/
        #define LANRXO_MSG_ID_MASK     0xfc000000
        #define LANRXO_MSG_ID_SHIFT    26

    /* Define the MSG ID*/
#define LANRXO_MSG_ID           0
#define LANRXSKB_MSG_ID         30

#define LANRXO_MAC_ID_MASK     0x03c00000
#define LANRXO_MAC_ID_SHIFT    22

    /* Define the MAC IDs*/
#define LANRXO_MAC_ID_ETH0        MAC_ID_ETH0
#define LANRXO_MAC_ID_ETH1        MAC_ID_ETH1
#define LANRXO_MAC_ID_WIFI0       MAC_ID_WIFI0
#define LANRXO_MAC_ID_WIFI1       MAC_ID_WIFI1
#define LANRXO_MAC_ID_USB         MAC_ID_USB
#define LANRXO_MAC_ID_MOCA        MAC_ID_MOCA
#define LANRXO_MAC_ID_WIFI0_GST   MAC_ID_WIFI0_GST
#define LANRXO_MAC_ID_WIFI1_GST   MAC_ID_WIFI1_GST
#define LANRXO_MAC_ID_ITC         MAC_ID_APPS_ROUTED
#define LANRXO_MAC_ID_APPS        MAC_ID_APPS_ROUTED
#define LANRXO_MAC_ID_EROUTER     MAC_ID_EROUTER

#define LANRXO_QOS_MASK        0x000f0000
#define LANRXO_QOS_SHIFT       16
#define LANRXO_SUB_ID_MASK     0x0000f000
#define LANRXO_SUB_ID_SHIFT    12
#define LANRXO_EOP_MASK        0x00000800
#define LANRXO_EOP_SHIFT       11
#define LANRXO_OFFSET_MASK     0x000007ff
#define LANRXO_OFFSET_SHIFT    0
#define LANRXSKB_LEN_MASK      LANRXO_OFFSET_MASK
#define LANRXSKB_LEN_SHIFT     LANRXO_OFFSET_SHIFT
    union {
        uint32_t token;     /* FPM */
        uint32_t context;   /* SKB */
    };
    uint32_t addr;          /* SKB */
};


/*LAN TX Message*/
/*Producer  TX Message Processor*/
/*Consumer  LAN MAC TX DMA*/
struct lantxmsg
{
    uint32_t msghdr;  /* Uses GenericTknMsgT to overlay*/
        #define LANTX_MAC_ID_MASK     0x03c00000
        #define LANTX_MAC_ID_SHIFT    22
        #define LANTX_REL_TKN_MASK    0x00200000
        #define LANTX_REL_TKN_SHIFT   21
        #define LANTX_STS_REQ_BIT     0x00100000
        #define LANTX_STS_REQ_SHIFT   20
        #define LANTX_QOS_MASK        0x000f0000
        #define LANTX_QOS_SHIFT       16
        #define LANTX_EOP_MASK        0x00008000
        #define LANTX_EOP_SHIFT       15
    uint32_t token;
};


/*LAN TX Offset Message or LAN TX SKB Message */
struct lantxoffsetmsg
{
    uint32_t msghdr;  /* Uses GenericTknMsgT to overlay*/
        #define LANTXO_MSG_ID_MASK     0xfc000000
        #define LANTXO_MSG_ID_SHIFT    26

    /* Define the MSG ID*/
#define LANTXO_MSG_ID           1
#define LANTXSKB_MSG_ID         31

#define LANTXO_MAC_ID_MASK     0x03c00000
#define LANTXO_MAC_ID_SHIFT    22

    /* Define the MAC IDs*/
#define LANTXO_MAC_ID_ETH0        MAC_ID_ETH0
#define LANTXO_MAC_ID_ETH1        MAC_ID_ETH1
#define LANTXO_MAC_ID_WIFI0       MAC_ID_WIFI0
#define LANTXO_MAC_ID_WIFI1       MAC_ID_WIFI1
#define LANTXO_MAC_ID_USB         MAC_ID_USB
#define LANTXO_MAC_ID_MOCA        MAC_ID_MOCA
#define LANTXO_MAC_ID_WIFI0_GST   MAC_ID_WIFI0_GST
#define LANTXO_MAC_ID_WIFI1_GST   MAC_ID_WIFI1_GST
#define LANTXO_MAC_ID_ITC         MAC_ID_APPS_ROUTED
#define LANTXO_MAC_ID_APPS        MAC_ID_APPS_ROUTED
#define LANTXO_MAC_ID_EROUTER     MAC_ID_EROUTER

#define LANTXO_REL_TKN_MASK    0x00200000
#define LANTXO_REL_TKN_SHIFT   21
#define LANTXO_STS_REQ_BIT     0x00100000
#define LANTXO_STS_REQ_SHIFT   20
#define LANTXO_QOS_MASK        0x000f0000
#define LANTXO_QOS_SHIFT       16
#define LANTXO_SUB_ID_MASK     0x0000f000
#define LANTXO_SUB_ID_SHIFT    12
#define LANTXO_EOP_MASK        0x00000800
#define LANTXO_EOP_SHIFT       11
#define LANTXO_OFFSET_MASK     0x000007ff
#define LANTXO_OFFSET_SHIFT    0
#define LANTXSKB_LEN_MASK      LANTXO_OFFSET_MASK
#define LANTXSKB_LEN_SHIFT     LANTXO_OFFSET_SHIFT
    union {
        uint32_t token;     /* FPM */
        uint32_t context;   /* SKB */
    };
    uint32_t addr;          /* SKB */
};

/*US Packet TX Offset Message*/
/*This is used by 3383/3384 and newer architecture*/
/*Producer  TX Message Processor*/
/*Consumer  LAN MAC TX DMA*/
struct ustxmsg
{
    uint32_t msghdr;  /* Uses GenericTknMsgT to overlay*/
        #define USTX_MSG_ID_MASK     0xfc000000
        #define USTX_MSG_ID_SHIFT    26

    /* Define the MSG ID*/
#define USTX_MSG_ID          11

#define USTX_MAC_ID_MASK     0x03c00000
#define USTX_MAC_ID_SHIFT    22

    /* Define the MAC IDs*/
#define USTX_MAC_ID_CM       MAC_ID_CM

#define USTX_REL_TKN_MASK    0x00200000
#define USTX_REL_TKN_SHIFT   21
#define USTX_STS_REQ_BIT     0x00100000
#define USTX_STS_REQ_SHIFT   20
#define USTX_QOS_MASK        0x000f0000
#define USTX_QOS_SHIFT       16
#define USTX_RSVD_MASK       0x0000f000
#define USTX_RSVD_SHIFT      12
#define USTX_EOP_MASK        0x00000800
#define USTX_EOP_SHIFT       11
#define USTX_OFFSET_MASK     0x000007ff
#define USTX_OFFSET_SHIFT    0
    uint32_t token;
    uint32_t ustxinfo;
#define USTX_PRIORITY_MASK   0x02000000
#define USTX_PRIORITY_SHIFT  25
#define USTX_FLOWID_MASK     0x01f00000
#define USTX_FLOWID_SHIFT    20
#define USTX_PHSI_MASK       0x000000ff
#define USTX_PHSI_SHIFT      0
    uint32_t ackcelid;
};


/*TX STATUS Message*/
/*Producer  LAN MAC TX DMA*/
/*Consumer  TX Message Processor*/
struct txstatusmsg
{
    uint32_t msghdr;
        #define TXSTS_MAC_ID_MASK     0x03c00000
        #define TXSTS_MAC_ID_SHIFT    22
        #define TXSTS_TX_STS_MASK     0x003f0000
        #define TXSTS_QOS_SHIFT       16
        #define TXSTS_TX_QD_MASK      0x0000003f
        #define TXSTS_TX_QD_SHIFT     0
    uint32_t token;
};


/*DTP Bonded Packet RX Message*/
/*Producer  DTP*/
/*Consumer  DFAP/FAP/Host*/
/*NOTE: When using MAC MGT messages we overload this structure a bit.*/
/*We are using the channel id 6 bits to overwrite the QoS and RSVD bits in the*/
/*msgHdr.  We are doing this because the DTP does not want to send 3 words to us*/
/*for MAC MGMT messages currently.*/
struct dspktrxmsg
{
    uint32_t msghdr;
        #define DSPKTRX_MSG_ID_MASK     0xfc000000
        #define DSPKTRX_MSG_ID_SHIFT    26
        #define DSPKTRX_MAC_ID_MASK     0x03c00000
        #define DSPKTRX_MAC_ID_SHIFT    22
        #define DSPKTRX_QOS_MASK        0x000f0000
        #define DSPKTRX_QOS_SHIFT       16
        #define DSPKTRX_RSVD_MASK       0x0000f000
        #define DSPKTRX_RSVD_SHIFT      12
        #define DSPKTRX_DS_CHAN_MASK    0x03f00000  /* Only used for MAC MGMT msgs*/
        #define DSPKTRX_DS_CHAN_SHIFT   20          /* Only used for MAC MGMT msgs*/
        #define DSPKTRX_EOP_MASK        0x00000800
        #define DSPKTRX_EOP_SHIFT       11
        #define DSPKTRX_OFFSET_MASK     0x000007ff
        #define DSPKTRX_OFFSET_SHIFT     0
    uint32_t token;
    uint32_t dsid;
};


/*DS Packet Message*/
/*Producer  DS MAC DMA*/
/*Consumer  DS Token Processor*/
struct dspktmsg
{
    uint32_t msghdr;
#define DSPKT_DS_CHAN_SHIFT     20
#define DSPKT_DS_CHAN_MASK      (0x3f << DSPKT_DS_CHAN_SHIFT)
#define DSPKT_QOS_TAG_SHIFT     16
#define DSPKT_QOS_TAG_MASK      (0x0f << DSPKT_QOS_TAG_SHIFT)
#define DSPKT_PKT_SEQ_SHIFT     0
#define DSPKT_PKT_SEQ_MASK      (0xffff << DSPKT_PKT_SEQ_SHIFT)
    uint32_t token;
    uint32_t dsid;
#define DSPKT_DSID_RSVD_SHIFT   31
#define DSPKT_DSID_RSVD_MASK    (0x01 << DSPKT_DSID_RSVD_SHIFT)
#define DSPKT_MCAST_MSG_SHIFT   30
#define DSPKT_MCAST_MSG_MASK    (0x01 << DSPKT_MCAST_MSG_SHIFT)
#define DSPKT_BCAST_MSG_SHIFT   29
#define DSPKT_BCAST_MSG_MASK    (0x01 << DSPKT_BCAST_MSG_SHIFT)
#define DSPKT_MAC_MSG_SHIFT     28
#define DSPKT_MAC_MSG_MASK      (0x01 << DSPKT_MAC_MSG_SHIFT)
#define DSPKT_PHSR_INVLD_SHIFT  27
#define DSPKT_PHSR_INVLD_MASK   (0x01 << DSPKT_PHSR_INVLD_SHIFT)
#define DSPKT_DSID_VLD_SHIFT    26
#define DSPKT_DSID_VLD_MASK     (0x01 << DSPKT_DSID_VLD_SHIFT)
#define DSPKT_PSN_VLD_SHIFT     25
#define DSPKT_PSN_VLD_MASK      (0x01 << DSPKT_PSN_VLD_SHIFT)
#define DSPKT_PRI_VLD_SHIFT     24
#define DSPKT_PRI_VLD_MASK      (0x01 << DSPKT_PRI_VLD_SHIFT)
#define DSPKT_TRF_PRI_SHIFT     21
#define DSPKT_TRF_PRI_MASK      (0x07 << DSPKT_TRF_PRI_SHIFT)
#define DSPKT_SEQ_CHG_SHIFT     20
#define DSPKT_SEQ_CHG_MASK      (0x01 << DSPKT_SEQ_CHG_SHIFT)
#define DSPKT_DSID_SHIFT        0
#define DSPKT_DSID_MASK         (0xfffff << DSPKT_DSID_SHIFT)
};

/*DS BPI Sequence Error Message*/
/*Producer  DS MAC DMA*/
/*Consumer  DS Token Processor*/
struct dsbpiseqerrmsg
{
    uint32_t msghdr;
#define DSBPI_DS_CHAN_SHIFT     20
#define DSBPI_DS_CHAN_MASK      (0x3f << DSBPI_DS_CHAN_SHIFT)
#define DSBPI_BPI_KSQ_SHIFT     16
#define DSBPI_BPI_KSQ_MASK      (0x0f << DSBPI_BPI_KSQ_SHIFT)
#define DSBPI_BPI_EN_SHIFT      15
#define DSBPI_BPI_EN_MASK       (0x01 << DSBPI_BPI_EN_SHIFT)
#define DSBPI_BPI_O_E_SHIFT     14
#define DSBPI_BPI_O_E_MASK      (0x01 << DSBPI_BPI_O_E_SHIFT)
#define DSBPI_SAID_SHIFT        0
#define DSBPI_SAID_MASK         (0x3fff << DSBPI_SAID_SHIFT)
};

/*DS PHS Discard Message*/
/*Producer  DS MAC DMA*/
/*Consumer  DS Token Processor*/
struct dsphsdiscardmsg
{
    uint32_t msghdr;
#define DSPHS_DS_CHAN_SHIFT     20
#define DSPHS_DS_CHAN_MASK      (0x3f << DSPHS_DS_CHAN_SHIFT)
#define DSPHS_DSID_MC_SHIFT     14
#define DSPHS_DSID_MC_MASK      (0x3f << DSPHS_DSID_MC_SHIFT)
#define DSPHS_RSVD_SHIFT        8
#define DSPHS_RSVD_MASK         (0x3f << DSPHS_RSVD_SHIFT)
#define DSPHS_PHS_IDX_SHIFT     0
#define DSPHS_PHS_IDX_MASK      (0xff << DSPHS_PHS_IDX_SHIFT)
};

struct uspprequestmsg
{
    uint32_t lantxhdr;
#define USPP_REQ_MSG_ID_MASK        0xFC000000
#define USPP_REQ_MSG_ID_SHIFT       26
#define USPP_REQ_REL_TKN_MASK       0x00200000
#define USPP_REQ_REL_TKN_SHIFT      21
#define USPP_REQ_STS_REQ_MASK       0x00100000
#define USPP_REQ_STS_REQ_SHIFT      20
#define USPP_REQ_EOP_MASK           0x00000800
#define USPP_REQ_EOP_SHIFT          11
#define USPP_REQ_OFFSET_MASK        0x000007FF
#define USPP_REQ_OFFSET_SHIFT       0
    uint32_t srctoken;
    uint32_t ackcelid;
    uint32_t msghdr;
#define USPP_REQ_FLOW_PRI_BIT       0x02000000
#define USPP_REQ_FLOW_PRI_SHIFT     25
#define USPP_REQ_FLOW_ID_MASK       0x01f00000
#define USPP_REQ_FLOW_ID_SHIFT      20
#define USPP_REQ_TOGGLE_QI_BIT      0x00000400
#define USPP_REQ_TOGGLE_QI_SHIFT    10
#define USPP_REQ_ENCRYPT_MASK       0x00000200
#define USPP_REQ_ENCRYPT_SHIFT      9
#define USPP_REQ_MAC_MSG_MASK       0x00000100
#define USPP_REQ_MAC_MSG_SHIFT      8
#define USPP_REQ_PHS_INDX_MASK      0x000000ff
#define USPP_REQ_PHS_INDX_SHIFT     0
};

struct usppresponsemsg
{
    uint32_t msghdr;
#define USPP_RSPN_FLOW_MASK         0x03f00000
#define USPP_RSPN_FLOW_SHIFT        20
#define USPP_RSPN_FLOW_PRI_BIT      0x02000000
#define USPP_RSPN_FLOW_PRI_SHIFT    25
#define USPP_RSPN_FLOW_ID_MASK      0x01f00000
#define USPP_RSPN_FLOW_ID_SHIFT     20
#define USPP_RSPN_BPI_KSQ_MASK      0x00080000
#define USPP_RSPN_BPI_KSQ_SHIFT     19
#define USPP_RSPN_REQ_18_12_MASK    0x0007f000
#define USPP_RSPN_REQ_18_12_SHIFT   12
#define USPP_RSPN_PHS_INV_MASK      0x00000800
#define USPP_RSPN_PHS_INV_SHIFT     11
#define USPP_RSPN_SUPPRESSED_MASK   0x000007ff   /* Src + CRC(4) - SuppressedBytes*/
#define USPP_RSPN_SUPPRESSED_SHIFT  0
    uint32_t dsttoken;
    uint32_t ackcelid;
};

struct uspptxstatusmsg
{
    uint32_t msghdr;
#define USPP_TXSTS_FLOW_PRI_BIT     0x02000000
#define USPP_TXSTS_FLOW_PRI_SHIFT   25
#define USPP_TXSTS_FLOW_ID_MASK     0x01f00000
#define USPP_TXSTS_FLOW_ID_SHIFT    20
#define USPP_TXSTS_STATUS_MASK      0x0000007E
#define USPP_TXSTS_STATUS_SHIFT     1
#define USPP_TXSTS_TKN_REL_MASK     0x00000001
#define USPP_TXSTS_TKN_REL_SHIFT    0
    uint32_t token;
};

/*US Token Free Message*/
/*Producer  US DMA Processor*/
/*Consumer  US Token Processor*/
struct usreleasetokenmsg
{
    uint32_t msghdr;
#define USRELEASE_FLOW_PRI_BIT      0x02000000
#define USRELEASE_FLOW_PRI_SHIFT    25
#define USRELEASE_FLOW_ID_MASK      0x01f00000
#define USRELEASE_FLOW_ID_SHIFT     20
#define USRELEASE_FLUSH_DONE_BIT    0x00000001
#define USRELEASE_FLUSH_DONE_SHIFT  0
    uint32_t tokenid;
#define USRELEASE_TKN_ID_MASK       0xffff0000
#define USRELEASE_TKN_ID_SHIFT      16
#define USRELEASE_NUM_TKN_MASK      0x0000ffff
#define USRELEASE_NUM_TKN_SHIFT     0
};


/*DQM US data message define.*/
/*Producer  Host Processor*/
/*Consumer  US Token Processor*/
struct uspacketmsg
{
    /* see USPPRequestMsg for bit definition*/
    uint32_t msghdr;
    uint32_t srctoken;
        #define US_TOKEN_SEND_COMPLETE                           0x40000000
    uint32_t akclid;
    uint32_t sidhdrlen;
};

union  utplantxmsgstruct {
  struct {
    uint32_t fmsgid            :6;
    uint32_t fmacid            :4;
    uint32_t freltkn           :1;
    uint32_t fstsreq           :1;
    uint32_t frsvd1            :8;
    uint32_t feop              :1;
    uint32_t foffset           :11;
  } bits;
  uint32_t reg32;
};

union  utpdocsismsgstruct {
  struct {
    uint32_t frsvd1            :6;
    uint32_t fpriority         :1;
    uint32_t fqueue            :5;
    uint32_t frsvd2            :6;
    uint32_t flogging          :1;
    uint32_t fsendcomplete     :1;
    uint32_t fugsflag          :1;
    uint32_t fqi               :1;
    uint32_t fencrypt          :1;
    uint32_t fmacmsg           :1;
    uint32_t fphsindex         :8;
        #define UTP_DOC_PHSI_MASK               0x000000FF
        #define UTP_DOC_PHSI_SHIFT              0
  } bits;
  uint32_t reg32;
};

union hdrsizeoffsetstruct {
  struct {
    uint32_t rsvd          :9;
    uint32_t fsize         :12;
        #define UTP_DOC_HDR_SIZE_MASK           0x007FF800
        #define UTP_DOC_HDR_SIZE_SHIFT          11
    uint32_t foffset       :11;
        #define UTP_DOC_HDR_OFFSET_MASK         0x000007FF
        #define UTP_DOC_HDR_OFFSET_SHIFT        0
  } bits;
  uint32_t reg32;
};

struct utplegpacketmsgstruct {
    union utplantxmsgstruct   flantxmsg;
    uint32_t              ftoken;
    union utpdocsismsgstruct  fdocmsg;
    union hdrsizeoffsetstruct fdochdr;
};

struct utppacketmsgstruct {
    union utplantxmsgstruct   flantxmsg;
    uint32_t              ftoken;
    union utpdocsismsgstruct  fdocmsg;
    uint32_t              fackcelid;
};

struct utphostlegmsgstruct {
    union utplantxmsgstruct  flantxmsg;
    uint32_t             ftoken;
    union utpdocsismsgstruct fdocmsg;
};

union uschainedlantxmsghdr {
  struct {
    uint32_t rsvd2            :6;
    uint32_t priority         :1;
    uint32_t queue            :5;
    uint32_t rsvd1            :8;
    uint32_t eop              :1;
    uint32_t offset           :11;
  } bits;
  uint32_t reg32;
};

union uschainedmsghdr {
  struct {
    uint32_t rsvd2            :6;
    uint32_t priority         :1;
    uint32_t queue            :5;
    uint32_t rsvd1            :9;
    uint32_t qi               :1;
    uint32_t encrypt          :1;
    uint32_t macmsg           :1;
    uint32_t phsindex         :8;
  } bits;
  uint32_t reg32;
};

struct uschainedpktmsg {
    union uschainedlantxmsghdr lantxmsghdr;
    uint32_t                    srctoken;
    union uschainedmsghdr      msghdr;
    uint32_t                    ackcelid;
};

/*
 * Prototype for the main init function for the generis message
 * fifo register set. This function clears the fifos and sets
 * up the message lengths for all defined MSG fifo message types.
 */
int initmsgfifo(void);

/*Parses received messages.*/
int getinoutmsgtypelengthandhandler(unsigned int msgword, unsigned int *msgtype, unsigned int *msglen);

/* pull message out of incoming message fifo*/
uint32_t rxincmsgfifomsg(struct msgstruct *prxmsg);

/*Load the outgoing message fifo with the data and send.*/
uint32_t sendoutmsgfifomsg(struct msgstruct *pfifomsg);

/** InOutMsgFifo regression tests
 *
 * Regression tests for this module.  These are all of the tests that were used
 * for validation when developing the module, examples of typical usage
 * scenarios, and whatever additional tests that were added when bugs were found
 * and fixed.
 *
 * The body of this method is normally compiled out; edit the .cpp file and
 * enable the REGRESSION_TEST define to build it in to your image.
 *
 * This code also serves as a pretty comprehensive set of examples showing how
 * this module can/should be used.  Any usage scenario not shown here is not
 * supported, and may not work if this module changes.
 *
 * \note If you don't see your usage scenario in the regression suite, go ahead
 * and add it!  This serves a number of purposes:
 *      - It shows the original designer/implementer how the module is
 *        being used, which can lead to design improvements (new helper
 *        methods that make things easier, etc).
 *      - It provides more examples to other programmers that can be
 *        copied and reused, rather than re-inventing the wheel.
 *      - It ensures that code changes to this module won't break your
 *        usage scenario.  Code changes are valid only if all
 *        regression tests pass; if your usage scenario isn't
 *        represented in the tests, then we have no way of knowing
 *        whether or not the change will break your use.  This will
 *        reduce the amount of debugging that you have to do, which is
 *        always a good thing.
 *
 *  \return
 *      bool - true if all tests passed, false if one or more tests failed.
 */
bool inoutmsgfiforegress(void);

#if defined __cplusplus
}  // extern "C"
#endif

#endif
