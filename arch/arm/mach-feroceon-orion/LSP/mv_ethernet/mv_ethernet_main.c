/*******************************************************************************
Copyright (C) Marvell International Ltd. and its affiliates

This software file (the "File") is owned and distributed by Marvell 
International Ltd. and/or its affiliates ("Marvell") under the following
alternative licensing terms.  Once you have made an election to distribute the
File under one of the following license alternatives, please (i) delete this
introductory statement regarding license alternatives, (ii) delete the two
license alternatives that you have not elected to use and (iii) preserve the
Marvell copyright notice above.


********************************************************************************
Marvell GPL License Option

If you received this File from Marvell, you may opt to use, redistribute and/or 
modify this File in accordance with the terms and conditions of the General 
Public License Version 2, June 1991 (the "GPL License"), a copy of which is 
available along with the File in the license.txt file or by writing to the Free 
Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 or 
on the worldwide web at http://www.gnu.org/licenses/gpl.txt. 

THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE IMPLIED 
WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE ARE EXPRESSLY 
DISCLAIMED.  The GPL License provides additional details about this warranty 
disclaimer.
*******************************************************************************/

#include "mvCommon.h"  /* Should be included before mvSysHwConfig */
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <linux/pci.h>
#include <linux/ip.h>
#include <linux/in.h>
#include <linux/tcp.h>

#include "mvOs.h"
#include "mvSysHwConfig.h"
#include "mvEth.h"
#include "mvEthPhy.h"
#ifdef INCLUDE_MULTI_QUEUE
#include "mvEthPolicy.h"
#endif

#if defined(CONFIG_BUFFALO_PLATFORM)
 #define CONFIG_BUFFALO_ETH_FORCE_MASTER	1
 #define BUFFALO_ETH_FORCE_MASTER		1
 #define BUFFALO_ETH_FORCE_SLAVE		2
 #define BUFFALO_ETH_FORCE_AUTO			0
 static MV_U8	buffalo_eth_mode=BUFFALO_ETH_FORCE_AUTO;
#endif

/****************************************************** 
 * driver internal definitions --                     *
 ******************************************************/ 

#if defined(CONFIG_MV_INCLUDE_UNM_ETH) /* UNIMAC */
/* No support for Checksum Offload in UniMAC */
#undef TX_CSUM_OFFLOAD
#undef RX_CSUM_OFFLOAD
#endif

/* Interrupt Cause Masks */
#ifdef ETH_TX_DONE_ISR
#define ETH_TXQ_MASK 		(((1 << MV_ETH_TX_Q_NUM) - 1) << ETH_CAUSE_TX_BUF_OFFSET) 
#else
#define ETH_TXQ_MASK      0
#endif /* ETH_TX_DONE_ISR */

#define ETH_RXQ_MASK      (((1 << MV_ETH_RX_Q_NUM) - 1) << ETH_CAUSE_RX_READY_OFFSET)
#define ETH_RXQ_RES_MASK  (((1 << MV_ETH_RX_Q_NUM) - 1) << ETH_CAUSE_RX_ERROR_OFFSET)


#if defined(CONFIG_MV_INCLUDE_UNM_ETH)
 /* UniMAC Interrupt Mask Register (IMR) */
 #define ETH_PICR_MASK      (ETH_RXQ_MASK|ETH_RXQ_RES_MASK)
#else
 /* Gigabit Ethernet Port Interrupt Mask and Port Interrupt Mask Extend Registers (PIMR and PIMER) */
 #define ETH_PICR_MASK      (BIT1|ETH_RXQ_MASK|ETH_RXQ_RES_MASK)
 #define ETH_PICER_MASK     (BIT20|BIT16|ETH_TXQ_MASK) /* phy/link-status-change, tx-done-q0 - q7 */
#endif

/* rx buffer size */ 
#define WRAP          (2 + ETH_HLEN + 4)  /* 2(HW hdr) 14(MAC hdr) 4(CRC) */

#define RX_BUFFER_SIZE(MTU, PRIV) (MTU + WRAP)

int ethDescRxQ[MV_ETH_RX_Q_NUM];
int ethDescTxQ[MV_ETH_TX_Q_NUM];

int     eth_loopback = 0;
int     eth_tx_done_quota = 16;
/****************************************************** 
 * driver debug control --                            *
 ******************************************************/
/* debug main on/off switch (more in debug control below ) */
#define ETH_DEBUG
#undef ETH_DEBUG

#define ETH_DBG_OFF     0x0000
#define ETH_DBG_RX      0x0001
#define ETH_DBG_TX      0x0002
#define ETH_DBG_RX_FILL 0x0004
#define ETH_DBG_TX_DONE 0x0008
#define ETH_DBG_LOAD    0x0010
#define ETH_DBG_IOCTL   0x0020
#define ETH_DBG_INT     0x0040
#define ETH_DBG_STATS   0x0080
#define ETH_DBG_ALL     0xffff

#ifdef ETH_DEBUG
# define ETH_DBG(FLG, X) if( (eth_dbg & (FLG)) == (FLG) ) printk X
#else
# define ETH_DBG(FLG, X)
#endif

u32 eth_dbg = ETH_DBG_ALL;

/****************************************************** 
 * driver statistics control --                       *
 ******************************************************/
/* statistics main on/off switch (more in statistics control below ) */
#ifdef CONFIG_EGIGA_STATIS
#define EGIGA_STATISTICS
#else
#undef EGIGA_STATISTICS
#endif

#define EGIGA_STAT_OFF     0x0000
#define EGIGA_STAT_RX      0x0001
#define EGIGA_STAT_TX      0x0002
#define EGIGA_STAT_RX_FILL 0x0004
#define EGIGA_STAT_TX_DONE 0x0008
#define EGIGA_STAT_LOAD    0x0010
#define EGIGA_STAT_IOCTL   0x0020
#define EGIGA_STAT_INT     0x0040
#define EGIGA_STAT_TSO     0x0080
#define EGIGA_STAT_ALL     0xffff

#ifdef EGIGA_STATISTICS
# define EGIGA_STAT(FLG, CODE) if( (eth_stat & (FLG)) == (FLG) ) CODE;
#else
# define EGIGA_STAT(FLG, CODE)
#endif

u32 eth_stat = EGIGA_STAT_ALL;

spinlock_t mii_lock = SPIN_LOCK_UNLOCKED;

extern u32 overEthAddr;
#ifdef CONFIG_BUFFALO_PLATFORM
// only 1 device
static struct {
	unsigned char mac[6];
	int linkspeed;
	int duplex_f;
	int mtu;
} buffalo_struc;
#endif
extern unsigned char mvMacAddr[6];
#if defined(CONFIG_MV_INCLUDE_UNM_ETH)
struct timer_list phy_link_timer;
unsigned int phy_link_timer_flag;
MV_U32 phy_link_up;
#endif


/****************************************************** 
 * device private information --                      *
 ******************************************************/
typedef struct _eth_statistics
{
    /* interrupt stats */
    u32 int_total, int_rx_events, int_tx_done_events;
    u32 int_phy_events, int_none_events;

    /* rx stats */
    u32 rx_poll_events, rx_poll_hal_ok[MV_ETH_RX_Q_NUM], rx_poll_hal_no_resource[MV_ETH_RX_Q_NUM];
    u32 rx_poll_hal_no_more[MV_ETH_RX_Q_NUM], rx_poll_hal_error[MV_ETH_RX_Q_NUM], rx_poll_hal_invalid_skb[MV_ETH_RX_Q_NUM];
    u32 rx_poll_hal_bad_stat[MV_ETH_RX_Q_NUM], rx_poll_netif_drop[MV_ETH_RX_Q_NUM], rx_poll_netif_complete;
    u32 rx_poll_distribution[ETH_NUM_OF_RX_DESCR+1];

    /* rx-fill stats */
    u32 rx_fill_events[MV_ETH_RX_Q_NUM], rx_fill_alloc_skb_fail[MV_ETH_RX_Q_NUM], rx_fill_hal_ok[MV_ETH_RX_Q_NUM];
    u32 rx_fill_hal_full[MV_ETH_RX_Q_NUM], rx_fill_hal_error[MV_ETH_RX_Q_NUM], rx_fill_timeout_events;

    /* tx stats */
    u32 tx_events, tx_hal_ok[MV_ETH_TX_Q_NUM], tx_hal_no_resource[MV_ETH_TX_Q_NUM], tx_hal_error[MV_ETH_TX_Q_NUM];
    u32 tx_hal_unrecognize[MV_ETH_TX_Q_NUM], tx_netif_stop[MV_ETH_TX_Q_NUM];
    u32 tx_timeout, tx_timer_events, tx_csum_offload;
    u32 tso_stats[64];

    /* tx-done stats */
    u32 tx_done_events, tx_done_hal_invalid_skb[MV_ETH_TX_Q_NUM], tx_done_hal_bad_stat[MV_ETH_TX_Q_NUM];
    u32 tx_done_hal_still_tx[MV_ETH_TX_Q_NUM], tx_done_hal_ok[MV_ETH_TX_Q_NUM], tx_done_hal_no_more[MV_ETH_TX_Q_NUM];
    u32 tx_done_hal_unrecognize[MV_ETH_TX_Q_NUM], tx_done_max, tx_done_netif_wake;
    u32 tx_done_distribution[ETH_NUM_OF_TX_DESCR+1];

} eth_statistics;


typedef struct _eth_priv
{
    int port;
    int vid;       /* the VLAN ID (VID) */
    void* hal_priv;
#ifdef INCLUDE_MULTI_QUEUE
    void* pRxPolicyHndl;
    void* pTxPolicyHndl;
#endif /* INCLUDE_MULTI_QUEUE */
    u32 rxq_count[MV_ETH_RX_Q_NUM];
    u32 txq_count[MV_ETH_TX_Q_NUM];
    spinlock_t lock;
    struct net_device_stats stats;
    MV_BUF_INFO tx_buf_info_arr[MAX_SKB_FRAGS+3];
    MV_PKT_INFO tx_pkt_info;

#ifdef EGIGA_STATISTICS
    eth_statistics eth_stat;
#endif
#if (ETH_TX_TIMER_PERIOD > 0)
    struct timer_list tx_timer;
    unsigned int tx_timer_flag;
#endif 
    struct timer_list rx_fill_timer;
    unsigned int rx_fill_flag;
    u32 picr;
    u32 picer;
    u32 rxcause;
    u32 txcause;
    u32 txmask;
    u32 rxmask;
#ifdef ETH_INCLUDE_TSO
    char*   tx_extra_bufs[ETH_NUM_OF_TX_DESCR]; 
    int     tx_extra_buf_idx;
#endif /* ETH_INCLUDE_TSO */
} eth_priv; 

/****************************************************** 
 * functions prototype --                             *
 ******************************************************/
static int __init eth_init_module( void );
static void __exit eth_exit_module( void );
module_init( eth_init_module );
module_exit( eth_exit_module);
static int eth_load( int port, char *name, char *mac_addr, int mtu, int irq );
static int eth_unload( int port, char *name );
static int eth_open( struct net_device *dev );
static int eth_start( struct net_device *dev );
static int eth_start_internals( struct net_device *dev );
static int eth_stop( struct net_device *dev );
static int eth_close( struct net_device *dev );
static int eth_stop_internals( struct net_device *dev );
static int eth_down_internals( struct net_device *dev );
static int eth_tx( struct sk_buff *skb, struct net_device *dev );
static u32 eth_tx_done( struct net_device *dev );
static void eth_tx_timeout( struct net_device *dev );
static int  eth_rx( struct net_device *dev,unsigned int work_to_do );
static void eth_rx_fill( struct net_device *dev );
static void eth_rx_fill_on_timeout( unsigned long data );
static inline void eth_print_rx_errors(unsigned int err, MV_U32 pkt_info_status);
static void eth_tx_timer_callback(  unsigned long data );
static int eth_poll( struct net_device *dev, int *budget );
static irqreturn_t eth_interrupt_handler( int rq , void *dev_id );
static struct net_device_stats* eth_get_stats( struct net_device *dev );
static void eth_set_multicast_list(struct net_device *dev);
static int eth_set_mac_addr( struct net_device *dev, void *addr );
#ifndef CONFIG_MV_INCLUDE_UNM_ETH
static int eth_change_mtu_internals( struct net_device *dev, int mtu );
static int eth_change_mtu( struct net_device *dev, int mtu );
#endif
static void eth_check_phy_link_status(struct net_device *dev, int print_flag);
static void eth_print_phy_status( struct net_device *dev );
static void eth_convert_str_to_mac( char *source , char *dest );
static unsigned int eth_str_to_hex( char ch );
void print_eth_stat( unsigned int port );
static int restart_autoneg( int port );
static void eth_mac_addr_get(int port, char *addr);
static inline int eth_get_hw_header_size(void);
static inline void eth_align_ip_header(struct sk_buff *skb);
static inline void eth_read_interrupt_cause_regs(struct net_device *dev);
static inline void eth_save_clear_rx_tx_cause(struct net_device *dev);
static inline void eth_read_save_clear_tx_cause(struct net_device *dev);
static inline void eth_read_save_clear_rx_cause(struct net_device *dev);
static inline void eth_print_int_while_polling(struct net_device *dev);
static inline void eth_mask_interrupts(struct net_device *dev);
static inline void eth_unmask_interrupts(struct net_device *dev);
static inline void eth_clear_interrupts(int port);

#if (defined(EGIGA_STATISTICS) || defined(CONFIG_MV_INCLUDE_UNM_ETH))
static struct net_device* get_net_device_by_port_num(unsigned int port);
#endif
int ReadMiiWrap(unsigned int portNumber, unsigned int MIIReg, unsigned int* value);
int WriteMiiWrap(unsigned int portNumber, unsigned int MIIReg, unsigned int data);
void print_skb(struct sk_buff* skb);
void print_iph(struct iphdr* iph);

int ReadMiiWrap(unsigned int portNumber, unsigned int MIIReg, unsigned int* value)
{
    unsigned long flags;
    MV_STATUS status;

    spin_lock_irqsave(&mii_lock, flags);
    status = mvEthPhyRegRead(portNumber, MIIReg , (unsigned short *)value);
    spin_unlock_irqrestore(&mii_lock, flags);

    if (status == MV_OK)
        return 0;

    return -1;
}


int WriteMiiWrap(unsigned int portNumber, unsigned int MIIReg, unsigned int data)
{
    unsigned long flags;
    MV_STATUS status;

    spin_lock_irqsave(&mii_lock, flags);
    status = mvEthPhyRegWrite(portNumber, MIIReg ,data);
    spin_unlock_irqrestore(&mii_lock, flags);

    if (status == MV_OK)
        return 0;

    return -1;
}


#if defined(CONFIG_MV_INCLUDE_UNM_ETH)
static void eth_phy_link_timer_function(unsigned long port)
{
    struct net_device *dev;
    
    if (phy_link_timer_flag) {
        dev = get_net_device_by_port_num(port);
        eth_check_phy_link_status(dev, 0);
	phy_link_timer.expires = jiffies + (HZ); /* 1 second */ 
        add_timer(&phy_link_timer);
    }
}
#endif


/*********************************************************** 
 * eth_init_module --                                    *
 *   main driver initialization. loading the interfaces.   *
 ***********************************************************/
static int __init eth_init_module( void ) 
{
    u32 i, err;
    printk( "Marvell Ethernet Driver 'mv_ethernet':\n" );

    printk( "  o %s\n", ETH_DESCR_CONFIG_STR );

#if defined(ETH_DESCR_IN_SRAM)
    printk( "  o %s\n", INTEG_SRAM_CONFIG_STR );
#endif

    printk( "  o %s\n", ETH_SDRAM_CONFIG_STR );
#if (MV_ETH_TX_Q_NUM > 1) && defined(ETH_INCLUDE_TSO)
#error "TSO is not supported when more than one TX queue is used"
#endif

#if (MV_ETH_TX_Q_NUM > 1)  && !defined(ETH_TX_DONE_ISR)
#error "When more than one TX queue is used, ETH_TX_DONE_ISR should be defined"
#endif 

#if defined(ETH_INCLUDE_TSO) && !defined(TX_CSUM_OFFLOAD)
#error "If TSO enabled - TX checksum offload must be enabled too"
#endif

#if defined(ETH_INCLUDE_TSO)
    printk( "  o TCP segmentation offload enabled\n");
#endif /* ETH_INCLUDE_TSO */

#if defined(TX_CSUM_OFFLOAD) && defined(RX_CSUM_OFFLOAD)
    printk( "  o Checksum offload enabled\n");
#else
#if defined(RX_CSUM_OFFLOAD)
    printk( "  o Receive checksum offload enabled\n");
#endif
#if defined(TX_CSUM_OFFLOAD)
    printk( "  o Transmit checksum offload enabled\n");
#endif
#endif

#ifdef INCLUDE_MULTI_QUEUE
    printk( "  o Multi Queue enabled\n");
#endif

#ifdef EGIGA_STATISTICS
    printk( "  o Driver statistics enabled\n");
#endif

#ifdef ETH_DEBUG
    printk( "  o Driver debug messages enabled\n");
#endif

#ifdef CONFIG_EGIGA_PROC
    printk("  o Marvell ethtool proc enabled\n");
#endif

    /* init MAC Unit */
    mvEthInit();

    printk("  o Rx desc:");
    /* Initialize ethDescRxQ array */
    for(i=0; i<MV_ETH_RX_Q_NUM; i++) {
        if (i == ETH_DEF_RXQ)
            ethDescRxQ[i] = (ETH_NUM_OF_RX_DESCR);
        else
            ethDescRxQ[i] = ETH_NUM_OF_RX_DESCR/2;
	printk(" %d",ethDescRxQ[i]);
    }
    printk("\n");
    
    printk("  o Tx desc:");
    /* Initialize ethDescTxQ  */
    for(i=0; i<MV_ETH_TX_Q_NUM; i++) {
        if (i == ETH_DEF_TXQ)
            ethDescTxQ[i] = ETH_NUM_OF_TX_DESCR;
        else
            ethDescTxQ[i] = ETH_NUM_OF_TX_DESCR/2;
	printk(" %d",ethDescTxQ[i]);
    }
    printk("\n");

    printk( "  o Loading network interface " );
    /* load interfaces */
    for(i=0; i<mvCtrlEthMaxPortGet(); i++ ) {
        err = 0;
#ifdef CONFIG_BUFFALO_PLATFORM
	printk("** %s (%d)\n",__FUNCTION__,i);
#endif

#ifndef CONFIG_MV_INCLUDE_UNM_ETH 
	if (MV_FALSE == mvCtrlPwrClckGet(ETH_GIG_UNIT_ID, i)) 
	{
		printk("\nWarning Giga %d is Powered Off\n",i);
		continue;
	}
#endif

        switch(i) {
            case 0:
                printk( "'%s0' ",CONFIG_MV_ETH_NAME );
                if( eth_load( 0, CONFIG_MV_ETH_NAME"0", CONFIG_ETH_0_MACADDR, CONFIG_ETH_0_MTU, ETH_PORT_IRQ_NUM(i) ) )
                    err = 1;
                break;
            case 1:
                printk( "'%s1' ",CONFIG_MV_ETH_NAME );
                if( eth_load( 1, CONFIG_MV_ETH_NAME"1", CONFIG_ETH_1_MACADDR, CONFIG_ETH_1_MTU, ETH_PORT_IRQ_NUM(i) ) )
                    err = 1;
                break;
            case 2:
                printk( "'%s2' ",CONFIG_MV_ETH_NAME );
                if( eth_load( 2, CONFIG_MV_ETH_NAME"2", CONFIG_ETH_2_MACADDR, CONFIG_ETH_2_MTU, ETH_PORT_IRQ_NUM(i) ) )
                    err = 1;
                break;
            default:
                err = 1;
                break;  
        }
        if( err ) printk( KERN_ERR "Error loading ethernet port %d\n", i );
    }
    printk( "\n" );

    return 0;
}



/*********************************************************** 
 * eth_exit_module --                                    *
 *   main driver termination. unloading the interfaces.    *
 ***********************************************************/
static void __exit eth_exit_module(void) 
{
    u32 i, err;

    for( i=0; i<mvCtrlEthMaxPortGet(); i++ ) {
        err = 0;
        switch(i) {
            case 0:
                if( eth_unload( 0, CONFIG_MV_ETH_NAME"0" ) )
                    err = 1;
                break;
            case 1:
                if( eth_unload( 1, CONFIG_MV_ETH_NAME"1" ) )
                    err = 1;
                break;
            case 2:
                if( eth_unload( 2, CONFIG_MV_ETH_NAME"2" ) )
                    err = 1;
                break;
            default:
                err = 1;
                break; 
        }
        if( err ) printk( KERN_ERR "Error unloading ethernet port %d\n", i);
    }
}

/*********************************************************** 
 * eth_load --                                           *
 *   load a network interface instance into linux core.    *
 *   initialize sw structures e.g. private, rings, etc.    *
 ***********************************************************/
static int eth_load( int port, char *name, char *mac_addr, int mtu, int irq ) 
{
    struct net_device *dev = NULL;
    eth_priv *priv = NULL;
    MV_ETH_PORT_INIT hal_init_struct;
    int ret = 0;
#ifdef INCLUDE_MULTI_QUEUE
    MV_ETH_TX_POLICY_ENTRY  ethTxDefPolicy =
        {
         NULL,                   /* pHeader */
         0,                      /* headerSize */
         ETH_DEF_TXQ    /* txQ */
        };
#endif

    if( strlen(name) > IFNAMSIZ ) { /* defined in netdevice.h */
        printk( KERN_ERR "%s must be less than %d chars\n", name, IFNAMSIZ );
    ret = -1;
    goto error;
    }

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
    dev = alloc_etherdev(sizeof(eth_priv));
#else
    dev = init_etherdev( dev, sizeof(eth_priv) );
#endif

    if( !dev ) {
        ret = -ENOMEM;
    goto error;
    }

    priv = (eth_priv *)dev->priv;
    if( !priv ) { 
        ret = -ENOMEM;
    goto error;
    }

    memset( priv , 0, sizeof(eth_priv) );

    /* init device mac addr */
    if(!overEthAddr)
#ifdef CONFIG_BUFFALO_PLATFORM
	mvEthMacAddrGet(port, dev->dev_addr);
#else /* CONFIG_BUFFALO_PLATFORM */
		eth_mac_addr_get(port, dev->dev_addr);
#endif /* CONFIG_BUFFALO_PLATFORM */
    else
        eth_convert_str_to_mac( mac_addr, dev->dev_addr );

    /* init device methods */
    strcpy( dev->name, name );
    dev->base_addr = 0;
    dev->irq = irq;
    dev->open = eth_open;
    dev->stop = eth_close;
    dev->hard_start_xmit = eth_tx;
    dev->tx_timeout = eth_tx_timeout;
    dev->watchdog_timeo = 5*HZ;
    dev->tx_queue_len = ethDescTxQ[ETH_DEF_TXQ];
    dev->poll = &eth_poll;

    if(ethDescRxQ[ETH_DEF_RXQ] <= 64)
	dev->weight = 32;
    else
        dev->weight = 64;

    dev->mtu = mtu;
    dev->get_stats = eth_get_stats;
    dev->set_mac_address = eth_set_mac_addr;
#ifndef CONFIG_MV_INCLUDE_UNM_ETH
    dev->change_mtu = &eth_change_mtu;
    dev->features = NETIF_F_SG;
#endif
    dev->set_multicast_list = eth_set_multicast_list;

#ifdef ETH_INCLUDE_TSO
    {
        int i;

        dev->features |= NETIF_F_TSO;
        for(i=0; i<(ETH_NUM_OF_TX_DESCR); i++)
        {
            priv->tx_extra_bufs[i] = mvOsMalloc(128);
            if(priv->tx_extra_bufs[i] == NULL)
            {
                printk("eth_TSO: Can't alloc extra TX buffer (128 bytes) for %d descr\n", i);
                return -ENOMEM;
            }
            priv->tx_extra_buf_idx = 0;
        }
    }
#endif /* ETH_INCLUDE_TSO */

    /* init eth_priv */
    priv->port = port;
    spin_lock_init( &priv->lock );
    spin_lock_init( &mii_lock );
    memset( &priv->rx_fill_timer, 0, sizeof(struct timer_list) );
    priv->rx_fill_timer.function = eth_rx_fill_on_timeout;
    priv->rx_fill_timer.data = (unsigned long)dev;
    init_timer(&priv->rx_fill_timer);
    priv->rx_fill_flag = 0;

#if (ETH_TX_TIMER_PERIOD > 0)
    memset( &priv->tx_timer, 0, sizeof(struct timer_list) );
    priv->tx_timer.function = eth_tx_timer_callback;
    priv->tx_timer.data = (unsigned long)dev;
    init_timer(&priv->tx_timer);
    priv->tx_timer_flag = 0;
#endif 
    /* init the hal */
    hal_init_struct.maxRxPktSize = RX_BUFFER_SIZE( dev->mtu, priv);
    hal_init_struct.rxDefQ = ETH_DEF_RXQ;
    memcpy(hal_init_struct.rxDescrNum,  ethDescRxQ, sizeof(int)*MV_ETH_RX_Q_NUM);
    memcpy(hal_init_struct.txDescrNum,  ethDescTxQ, sizeof(int)*MV_ETH_TX_Q_NUM);

#ifdef INCLUDE_MULTI_QUEUE
    /* Initialize RX policy */
    priv->pRxPolicyHndl = mvEthRxPolicyInit(port, ETH_RX_QUEUE_QUOTA, MV_ETH_PRIO_FIXED);
    if(priv->pRxPolicyHndl == NULL)
    {
        mvOsPrintf("mv_ethernet: Can't init RX Policy for Eth port #%d\n", port);
    kfree( priv );
    kfree( dev );
    return -ENODEV;
    }

    /* Initialize TX policy */
    priv->pTxPolicyHndl = mvEthTxPolicyInit(port, &ethTxDefPolicy);
    if(priv->pTxPolicyHndl == NULL)
    {
        mvOsPrintf("mv_ethernet: Can't init TX Policy for Eth port #%d\n", port);
    kfree( priv );
    kfree( dev );
    return -ENODEV;
    }
#endif /* INCLUDE_MULTI_QUEUE */

    /* create internal port control structure and descriptor rings.               */
    /* open address decode windows, disable rx and tx operations. mask interrupts */
    priv->hal_priv = mvEthPortInit( port, &hal_init_struct );

    if( !priv->hal_priv ) {
        printk( KERN_ERR "%s: load failed\n", dev->name );
    kfree( priv );
    kfree( dev );
    return -ENODEV;
    }

    /* set new addr in hw */
    if( mvEthMacAddrSet( priv->hal_priv, dev->dev_addr, ETH_DEF_RXQ) != MV_OK ) {
        printk( KERN_ERR "%s: ethSetMacAddr failed\n", dev->name );
    return -1;
    }

#ifndef CONFIG_MV_INCLUDE_UNM_ETH
    eth_change_mtu_internals(dev, mtu);
#endif

#ifdef CONFIG_BUFFALO_PLATFORM
	memcpy(&buffalo_struc.mac, dev->dev_addr, 6);
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
   /* register the device */
   if(register_netdev(dev)) {
        printk( KERN_ERR "%s: register failed\n", dev->name );
        kfree( priv );
        kfree( dev );
   }
#endif

    return 0;

 error:
    if( priv )
        kfree( dev->priv );

    if( dev )
        kfree( dev );

    return ret;
}



/*********************************************************** 
 * eth_unload --                                         *
 *   this is not a loadable module. nothig to be done here *
 ***********************************************************/
static int eth_unload( int port, char *name )
{
    /* shut down ethernet port if needed. free descriptor rings. */
    /* free internal port control structure.                     */
    /* ethPortFinish( port );                                    */

    return 0;
}

/************************************************************ 
 * eth_open -- Restore MAC address and call to   *
 *                eth_start                               *
 ************************************************************/
static int eth_open( struct net_device *dev )
{
    eth_priv *priv = dev->priv;
    int         queue = ETH_DEF_RXQ;

    if( mvEthMacAddrSet( priv->hal_priv, dev->dev_addr, queue) != MV_OK ) {
        printk( KERN_ERR "%s: ethSetMacAddr failed\n", dev->name );
        return -1;
    }

    if(eth_start( dev )){
        printk( KERN_ERR "%s: start interface failed\n", dev->name );
        return -1;
    } 
    return 0;
}


/*********************************************************** 
 * eth_start --                                          *
 *   start a network device. connect and enable interrupts *
 *   set hw defaults. fill rx buffers. restart phy link    *
 *   auto neg. set device link flags. report status.       *
 ***********************************************************/
static int eth_start( struct net_device *dev ) 
{
    unsigned long flags;
    eth_priv *priv = dev->priv;

    ETH_DBG( ETH_DBG_LOAD, ("%s: starting... ", dev->name ) );
    spin_lock_irqsave( &(priv->lock), flags);

    /* connect to port interrupt line */
   if( request_irq( dev->irq, eth_interrupt_handler,
        (IRQF_DISABLED | IRQF_SAMPLE_RANDOM) , dev->name, dev ) ) {
        printk( KERN_ERR "cannot assign irq%d to %s port%d\n", dev->irq, dev->name, priv->port );
        dev->irq = 0;
    goto error;
    }

    /* in default link is down */
    netif_carrier_off( dev );

    /* Stop the TX queue - it will be enabled upon PHY status change after link-up interrupt/timer */
    netif_stop_queue( dev );

    /* enable polling on the port, must be used after netif_poll_disable */
    netif_poll_enable(dev);

    /* fill rx buffers, start rx/tx activity, set coalescing */
    if( eth_start_internals( dev ) != 0 ) {
        printk( KERN_ERR "%s: start internals failed\n", dev->name );
    goto error;
    }
    
    restart_autoneg( priv->port );

    /* GbE hardware relies on the Phy interrupt to be handled in the interrupt handler and    */
    /* invoke netif_wake_queue. UniMAC does not provide a Phy interrupt indication, therefore */
    /* we use polling, invoking a timer every one second.                                     */
#ifdef CONFIG_MV_INCLUDE_UNM_ETH
    if(!phy_link_timer_flag) {
	phy_link_up = 0;
    	memset( &phy_link_timer, 0, sizeof(struct timer_list) );
    	init_timer(&phy_link_timer);
    	phy_link_timer.function = eth_phy_link_timer_function;
    	phy_link_timer.data = (unsigned long)priv->port;
    	phy_link_timer.expires = jiffies + (HZ); /* 1 second */ 
    	phy_link_timer_flag = 1;
    	add_timer(&phy_link_timer);
    }
#endif

#if (ETH_TX_TIMER_PERIOD > 0)
    if(priv->tx_timer_flag == 0)
    {
        priv->tx_timer.expires = jiffies + ((HZ*ETH_TX_TIMER_PERIOD)/1000); /*ms*/
        add_timer( &priv->tx_timer );
        priv->tx_timer_flag = 1;
    }
#endif /* ETH_TX_TIMER_PERIOD > 0 */
    ETH_DBG( ETH_DBG_LOAD, ("%s: start ok\n", dev->name) );

    spin_unlock_irqrestore( &(priv->lock), flags);

    return 0;

 error:
    spin_unlock_irqrestore( &(priv->lock), flags);

    if( dev->irq != 0 )
    {
        free_irq( dev->irq, dev );
    }

    printk( KERN_ERR "%s: start failed\n", dev->name );
    return -1;
}



/*********************************************************** 
 * eth_start_internals --                                *
 *   fill rx buffers. start rx/tx activity. set coalesing. *
 *   clear and unmask interrupt bits                       *
 ***********************************************************/
static int eth_start_internals( struct net_device *dev )
{
    unsigned int status;

    eth_priv *priv = dev->priv;
 
    /* fill rx ring with buffers */
    eth_rx_fill(dev);

    eth_clear_interrupts(priv->port);

    /* start the hal - rx/tx activity */
    status = mvEthPortEnable( priv->hal_priv );
    if( (status != MV_OK) && (status != MV_NOT_READY)) {
        printk( KERN_ERR "%s: ethPortEnable failed\n", dev->name );
     return -1;
    }

#ifndef CONFIG_MV_INCLUDE_UNM_ETH
    /* set tx/rx coalescing mechanism */
    mvEthTxCoalSet( priv->hal_priv, ETH_TX_COAL );
    mvEthRxCoalSet( priv->hal_priv, ETH_RX_COAL );
#endif

    eth_unmask_interrupts(dev);

    return 0;
}


/*********************************************************** 
 * eth_close --                                      *
 *   stop interface with linux core. stop port activity.   *
 *   free skb's from rings. set defaults to hw. disconnect *
 *   interrupt line.                                       *
 ***********************************************************/
static int eth_close( struct net_device *dev )
{
    unsigned long flags;
    eth_priv *priv = dev->priv;
    spin_lock_irqsave( &(priv->lock), flags);

#if (ETH_TX_TIMER_PERIOD > 0)
    priv->tx_timer_flag = 0;
    del_timer(&priv->tx_timer);
#endif
    /* stop upper layer */
    netif_carrier_off( dev );
    netif_stop_queue( dev );

    /* stop tx/rx activity, mask all interrupts, relese skb in rings */
    eth_stop_internals( dev );

    /* clear cause registers. mask interrupts. clear MAC tables. */
    /* set defaults. reset descriptors ring. reset PHY.          */
    if( mvEthDefaultsSet( priv->hal_priv ) != MV_OK ) {
        printk( KERN_ERR "%s: error set default on stop\n", dev->name );
    goto error;
    }
#if defined(CONFIG_MV_INCLUDE_UNM_ETH)    
    phy_link_timer_flag = 0;
#endif
    spin_unlock_irqrestore( &priv->lock, flags);

    if( dev->irq != 0 )
    {
        free_irq( dev->irq, dev );
    }
    
    return 0;

 error:
    printk( KERN_ERR "%s: stop failed\n", dev->name );
    spin_unlock_irqrestore( &priv->lock, flags);
    return -1;
    
}

/*********************************************************** 
 * eth_stop --                                       *
 *   stop interface with linux core. stop port activity.   *
 *   free skb's from rings.                                *
 ***********************************************************/
static int eth_stop( struct net_device *dev )
{
    unsigned long flags;
    eth_priv *priv = dev->priv;

    /* first make sure that the port finished its Rx polling - see tg3 */
    /* otherwise it may cause issue in SMP, one CPU is here and the other is doing the polling
    and both of it are messing with the descriptors rings!! */
    netif_poll_disable( dev );

    spin_lock_irqsave( &(priv->lock), flags);

    /* stop upper layer */
    netif_carrier_off( dev );
    netif_stop_queue( dev );

    /* stop tx/rx activity, mask all interrupts, relese skb in rings,*/
    eth_stop_internals( dev );
    
    spin_unlock_irqrestore( &priv->lock, flags);

    if( dev->irq != 0 )
    {
        free_irq( dev->irq, dev );
    }

    return 0;
}


/***********************************************************
 * eth_down_internals --                                 *
 *   down port rx/tx activity. free skb's from rx/tx rings.*
 ***********************************************************/
static int eth_down_internals( struct net_device *dev )
{
    eth_priv *priv = dev->priv;
    MV_PKT_INFO pkt_info;
    unsigned int queue;

    /* stop the port activity, mask all interrupts */
    if( mvEthPortDown( priv->hal_priv ) != MV_OK ) {
        printk( KERN_ERR "%s: ethPortDown failed\n", dev->name );
        goto error;
    }

    /* free the skb's in the hal tx ring */
    for(queue = 0; queue < MV_ETH_TX_Q_NUM; queue++) {
        while( mvEthPortForceTxDone( priv->hal_priv, queue, &pkt_info ) == MV_OK ) {
            priv->txq_count[queue]--;
            if( pkt_info.osInfo )
                    dev_kfree_skb_any( (struct sk_buff *)pkt_info.osInfo );
            else {
                    printk( KERN_ERR "%s: error in ethGetNextRxBuf\n", dev->name );
                    goto error;
            }
        }
    }

    return 0;

 error:
    printk( KERN_ERR "%s: stop internals failed\n", dev->name );
    return -1;
}


/*********************************************************** 
 * eth_stop_internals --                                 *
 *   stop port rx/tx activity. free skb's from rx/tx rings.*
 ***********************************************************/
static int eth_stop_internals( struct net_device *dev )
{
    eth_priv *priv = dev->priv;
    MV_PKT_INFO pkt_info;
    unsigned int queue;

    /* stop the port activity, mask all interrupts */
    if( mvEthPortDisable( priv->hal_priv ) != MV_OK ) {
        printk( KERN_ERR "%s: ethPortDisable failed\n", dev->name );
        goto error;
    }
    
    /* clear all ethernet port interrupts */
    eth_clear_interrupts(priv->port);    

    eth_mask_interrupts(dev);

    /* free the skb's in the hal tx ring */
    for(queue = 0; queue < MV_ETH_TX_Q_NUM; queue++)
    {
        while( mvEthPortForceTxDone( priv->hal_priv, queue, &pkt_info ) == MV_OK ) {
            priv->txq_count[queue]--;
        if( pkt_info.osInfo )
                dev_kfree_skb_any( (struct sk_buff *)pkt_info.osInfo );
        else {
                printk( KERN_ERR "%s: error in ethGetNextRxBuf\n", dev->name );
                goto error;
        }
        }
    }
    /* free the skb's in the hal rx ring */
    for(queue=0; queue<MV_ETH_RX_Q_NUM; queue++)
    {    
        while( mvEthPortForceRx( priv->hal_priv, queue, &pkt_info) == MV_OK ) {
            priv->rxq_count[queue]--;
        if( pkt_info.osInfo )
                dev_kfree_skb_any( (struct sk_buff *)pkt_info.osInfo );
        else {
                printk( KERN_ERR "%s: error in ethGetNextRxBuf\n", dev->name );
                goto error;
        }
        }
    }

    /* Reset Rx descriptors ring */
    for(queue=0; queue<MV_ETH_RX_Q_NUM; queue++)
    {
    ethResetRxDescRing(priv->hal_priv, queue);
    }
    /* Reset Tx descriptors ring */
    for(queue=0; queue<MV_ETH_TX_Q_NUM; queue++)
    {
    ethResetTxDescRing(priv->hal_priv, queue);
    }

    return 0;

 error:
    printk( KERN_ERR "%s: stop internals failed\n", dev->name );
    return -1;
}

static INLINE int     eth_tx_policy(eth_priv *priv)
{
    int     queue;

#ifdef INCLUDE_MULTI_QUEUE
    /* Second case: tx queue number (no prepended header) */
    queue = mvEthTxPolicyGet(priv->pTxPolicyHndl, &priv->tx_pkt_info, NULL);
#else
    /* no multiqueue. all packets go to one default queue. */
    queue = ETH_DEF_TXQ;
#endif /* INCLUDE_MULTI_QUEUE */

    return queue;
}


#ifdef ETH_INCLUDE_TSO
/*********************************************************** 
 * eth_tso_tx --                                             *
 *   send a packet.                                        *
 ***********************************************************/
static int eth_tso_tx( struct sk_buff *skb , struct net_device *dev )
{
    MV_STATUS       status;
    int             pkt, frag, buf;
    int             total_len, hdr_len, mac_hdr_len, size, frag_size, data_left;
    char            *frag_ptr, *extra_ptr;
    MV_U16          ip_id;
    MV_U32          tcp_seq;
    struct iphdr    *iph;
    struct tcphdr   *tcph;
    skb_frag_t      *skb_frag_ptr;
    eth_priv        *priv = dev->priv;
    int             queue;
    const struct tcphdr *th = tcp_hdr(skb);

    pkt = 0;        
    frag = 0;
    total_len = skb->len;
    hdr_len = (skb_transport_offset(skb) + tcp_hdrlen(skb));
    mac_hdr_len = skb_network_offset(skb);

    priv->tx_pkt_info.pFrags = &priv->tx_buf_info_arr[1];
    total_len -= hdr_len;

    if(skb_shinfo(skb)->gso_segs == 1)
    {
        printk("Only one TSO segs\n");
        print_skb(skb);
    }

    if(total_len <= skb_shinfo(skb)->gso_size)
    {
        printk("***** total_len less than gso_size\n");
        print_skb(skb);
    }
    if( (htons(ETH_P_IP) != skb->protocol) || 
        ( ip_hdr(skb)->protocol != IPPROTO_TCP) )
    {
        printk("***** ERROR: Unexpected protocol\n");
        print_skb(skb);
    }

    ip_id = ntohs(ip_hdr(skb)->id);
    tcp_seq = ntohl(th->seq);

    frag_size = skb_headlen(skb);
    frag_ptr = skb->data;

    if(frag_size < hdr_len){
        printk("***** ERROR: frag_size=%d, hdr_len=%d\n", frag_size, hdr_len);
        print_skb(skb);
    }

    frag_size -= hdr_len;
    frag_ptr += hdr_len;
    if(frag_size == 0)
    {
        skb_frag_ptr = &skb_shinfo(skb)->frags[frag];

        /* Move to next segment */
        frag_size = skb_frag_ptr->size;
        frag_ptr = page_address(skb_frag_ptr->page) + skb_frag_ptr->page_offset;
        frag++;
    }
    
    while(total_len > 0)
    {            
        extra_ptr = priv->tx_extra_bufs[priv->tx_extra_buf_idx++];
        if(priv->tx_extra_buf_idx == ETH_NUM_OF_TX_DESCR)
            priv->tx_extra_buf_idx = 0;

        extra_ptr += 2;
        memcpy(extra_ptr, skb->data, hdr_len);

        priv->tx_pkt_info.pFrags[0].bufVirtPtr = extra_ptr;
        priv->tx_pkt_info.pFrags[0].bufSize = hdr_len;

        data_left = MV_MIN(skb_shinfo(skb)->gso_size, total_len);
        priv->tx_pkt_info.pktSize = data_left + hdr_len;
        total_len -= data_left;

        /* Update fields */
        iph = (struct iphdr*)(extra_ptr + mac_hdr_len);
        iph->tot_len = htons(data_left + hdr_len - mac_hdr_len);
        iph->id = htons(ip_id);

        tcph = (struct tcphdr*)(extra_ptr + skb_transport_offset(skb));
        tcph->seq = htonl(tcp_seq);
/*
        printk("pkt=%d, extra=%p, left=%d, total=%d, iph=%p, tcph=%p, id=%d, seq=0x%x\n",
                pkt, extra_ptr, data_left, total_len, iph, tcph, ip_id, tcp_seq);
*/
        tcp_seq += data_left;
        ip_id++;
        if(total_len == 0)
        {
            /* Only for last packet */
            priv->tx_pkt_info.osInfo = (MV_ULONG)skb;
        }
        else
        {
            /* Clear all special flags for not last packet */
            tcph->psh = 0;
            tcph->fin = 0;
            tcph->rst = 0;
            priv->tx_pkt_info.osInfo = (MV_ULONG)0;
        }
        buf = 1;
        while(data_left > 0)
        {
            size = MV_MIN(frag_size, data_left);
            if(size == 0)
            {
                printk("***** ERROR: data_left=%d, frag_size=%d\n", data_left, frag_size);
                print_skb(skb);
            }
            data_left -= size;
            frag_size -= size;
            priv->tx_pkt_info.pFrags[buf].bufVirtPtr = frag_ptr;
            priv->tx_pkt_info.pFrags[buf].bufSize = size;
            frag_ptr += size;
            buf++;
            if( (frag < skb_shinfo(skb)->nr_frags) && (frag_size == 0) )
            {                 
                skb_frag_ptr = &skb_shinfo(skb)->frags[frag];

                /* Move to next segment */
                frag_size = skb_frag_ptr->size;
                frag_ptr = page_address(skb_frag_ptr->page) + skb_frag_ptr->page_offset;
                frag++;
            }
        }
        /* packet is full */
        priv->tx_pkt_info.numFrags = buf;
        priv->tx_pkt_info.status =  
                (ETH_TX_IP_NO_FRAG | ETH_TX_L4_TCP_TYPE |
                 ETH_TX_GENERATE_L4_CHKSUM_MASK | ETH_TX_GENERATE_IP_CHKSUM_MASK |
                 (( ip_hdr(skb)->ihl) << ETH_TX_IP_HEADER_LEN_OFFSET) );

        /* At this point we need to decide to which tx queue this packet goes, */
        /* and whether we need to prepend a proprietary header.                */
        queue = eth_tx_policy(priv);

        status = mvEthPortTx( priv->hal_priv, queue, &priv->tx_pkt_info );
        if( status == MV_OK ) {
            priv->stats.tx_packets ++;
            priv->stats.tx_bytes += priv->tx_pkt_info.pktSize;
            dev->trans_start = jiffies;
            priv->txq_count[queue]++;
            EGIGA_STAT( EGIGA_STAT_TX, (priv->eth_stat.tx_hal_ok[queue]++) );
        }
        else
        {
            /* tx failed. higher layers will free the skb */
            priv->stats.tx_dropped++;
            if( status == MV_NO_RESOURCE ) {
                /* it must not happen because we call to netif_stop_queue in advance. */
                ETH_DBG( ETH_DBG_TX, ("%s: queue is full, stop transmit\n", dev->name) );
                netif_stop_queue( dev );
                EGIGA_STAT( EGIGA_STAT_TX, (priv->eth_stat.tx_hal_no_resource[queue]++) );
                EGIGA_STAT( EGIGA_STAT_TX, (priv->eth_stat.tx_netif_stop[queue]++) );
            }
            else {
                printk( KERN_ERR "%s: error (%d) on transmit\n", dev->name, status);
            }
            return 1;
        }   
        pkt++;
    }    
    return 0;
}
#endif /* ETH_INCLUDE_TSO */

/*********************************************************** 
 * eth_tx --                                             *
 *   send a packet.                                        *
 ***********************************************************/
static int eth_tx( struct sk_buff *skb , struct net_device *dev )
{
    eth_priv *priv = dev->priv;
    struct net_device_stats *stats = &priv->stats;
    unsigned long flags;
    MV_STATUS status;
    int ret = 0, i, queue;

    if( netif_queue_stopped( dev ) ) {
        printk( KERN_ERR "%s: transmitting while stopped\n", dev->name );
        return 1;
    }

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,9)
    local_irq_save(flags);
    if (!spin_trylock(&priv->lock)) {
        /* Collision - tell upper layer to requeue */
        local_irq_restore(flags);
        return NETDEV_TX_LOCKED;
    }
#else
    spin_lock_irqsave( &(priv->lock), flags );
#endif

    ETH_DBG( ETH_DBG_TX, ("%s: tx, #%d frag(s), csum by %s\n",
             dev->name, skb_shinfo(skb)->nr_frags+1, (skb->ip_summed==CHECKSUM_PARTIAL)?"HW":"CPU") );
    EGIGA_STAT( EGIGA_STAT_TX, (priv->eth_stat.tx_events++) );
#ifdef ETH_INCLUDE_TSO
    EGIGA_STAT( EGIGA_STAT_TSO, (priv->eth_stat.tso_stats[skb->len >> 10]++) ); 
    if(skb_shinfo(skb)->frag_list != NULL)
    {
        printk("eth Warning: skb->frag_list != NULL\n");
        print_skb(skb);
    }
    if(skb_shinfo(skb)->gso_size) {
        ret = eth_tso_tx(skb, dev);
        goto tx_end;
    }
#endif /* ETH_INCLUDE_TSO */

    /* basic init of pkt_info. first cell in buf_info_arr is left for header prepending if necessary */
    priv->tx_pkt_info.osInfo = (MV_ULONG)skb;
    priv->tx_pkt_info.pktSize = skb->len;
    priv->tx_pkt_info.pFrags = &priv->tx_buf_info_arr[1];
    priv->tx_pkt_info.status = 0;
    
    /* see if this is a single/multiple buffered skb */
    if( skb_shinfo(skb)->nr_frags == 0 ) {
        priv->tx_pkt_info.pFrags->bufVirtPtr = skb->data;
        priv->tx_pkt_info.pFrags->bufSize = skb->len;
        priv->tx_pkt_info.numFrags = 1;
    }
    else {

        MV_BUF_INFO *p_buf_info = priv->tx_pkt_info.pFrags;

        /* first skb fragment */
        p_buf_info->bufSize = skb_headlen(skb);
        p_buf_info->bufVirtPtr = skb->data;
        p_buf_info++;

        /* now handle all other skb fragments */
        for ( i = 0; i < skb_shinfo(skb)->nr_frags; i++ ) {

            skb_frag_t *frag = &skb_shinfo(skb)->frags[i];

            p_buf_info->bufSize = frag->size;
            p_buf_info->bufVirtPtr = page_address(frag->page) + frag->page_offset;
            p_buf_info++;
        }

        priv->tx_pkt_info.numFrags = skb_shinfo(skb)->nr_frags + 1;
    }

#ifdef TX_CSUM_OFFLOAD
    /* if HW is suppose to offload layer4 checksum, set some bits in the first buf_info command */
    if(skb->ip_summed == CHECKSUM_PARTIAL) {
        ETH_DBG( ETH_DBG_TX, ("%s: tx csum offload\n", dev->name) );
        /*EGIGA_STAT( EGIGA_STAT_TX, Add counter here );*/
        priv->tx_pkt_info.status =
        ETH_TX_IP_NO_FRAG |           /* we do not handle fragmented IP packets. add check inside iph!! */
        ((ip_hdr(skb)->ihl) << ETH_TX_IP_HEADER_LEN_OFFSET) |                            /* 32bit units */
        ((ip_hdr(skb)->protocol == IPPROTO_TCP) ? ETH_TX_L4_TCP_TYPE : ETH_TX_L4_UDP_TYPE) | /* TCP/UDP */
        ETH_TX_GENERATE_L4_CHKSUM_MASK |                                /* generate layer4 csum command */
        ETH_TX_GENERATE_IP_CHKSUM_MASK;                              /* generate IP csum (already done?) */
    }
    else {
        ETH_DBG( ETH_DBG_TX, ("%s: no tx csum offload\n", dev->name) );
        /*EGIGA_STAT( EGIGA_STAT_TX, Add counter here );*/
        priv->tx_pkt_info.status = 0x5 << ETH_TX_IP_HEADER_LEN_OFFSET; /* Errata BTS #50 */
    }
#endif

    /* At this point we need to decide to which tx queue this packet goes, */
    /* and whether we need to prepend a proprietary header.                */
    queue = eth_tx_policy(priv);

    /* now send the packet */
    status = mvEthPortTx( priv->hal_priv, queue, &priv->tx_pkt_info );
    /* check status */
    if( status == MV_OK ) {
        stats->tx_bytes += skb->len;
        stats->tx_packets ++;
        dev->trans_start = jiffies;
        priv->txq_count[queue]++;
        ETH_DBG( ETH_DBG_TX, ("ok (%d); ", priv->txq_count[queue]) );
        EGIGA_STAT( EGIGA_STAT_TX, (priv->eth_stat.tx_hal_ok[queue]++) );
    }
    else {
        /* tx failed. higher layers will free the skb */
        ret = 1;
        stats->tx_dropped++;

        if( status == MV_NO_RESOURCE ) {
            /* it must not happen because we call to netif_stop_queue in advance. */
            ETH_DBG( ETH_DBG_TX, ("%s: queue is full, stop transmit\n", dev->name) );
            netif_stop_queue( dev );
            EGIGA_STAT( EGIGA_STAT_TX, (priv->eth_stat.tx_hal_no_resource[queue]++) );
            EGIGA_STAT( EGIGA_STAT_TX, (priv->eth_stat.tx_netif_stop[queue]++) );
        }
        else if( status == MV_ERROR ) {
            printk( KERN_ERR "%s: error on transmit\n", dev->name );
            EGIGA_STAT( EGIGA_STAT_TX, (priv->eth_stat.tx_hal_error[queue]++) );
        }
        else {
            printk( KERN_ERR "%s: unrecognized status on transmit\n", dev->name );
            EGIGA_STAT( EGIGA_STAT_TX, (priv->eth_stat.tx_hal_unrecognize[queue]++) );
        }
    }

#ifdef ETH_INCLUDE_TSO
tx_end:
#endif

#ifdef ETH_TX_DONE_ISR
#else
    if(priv->txq_count[ETH_DEF_TXQ] >= eth_tx_done_quota)
    {
        eth_tx_done(dev);
    }
#endif /* ETH_TX_DONE_ISR */ 
#ifndef INCLUDE_MULTI_QUEUE
    /* if number of available descriptors left is less than  */
    /* MAX_SKB_FRAGS stop the stack. if multi queue is used, */
    /* don't stop the stack just because one queue is full.  */
    if( mvEthTxResourceGet(priv->hal_priv, ETH_DEF_TXQ) <= MAX_SKB_FRAGS ) {
        ETH_DBG( ETH_DBG_TX, ("%s: stopping network tx interface\n", dev->name) );
        netif_stop_queue( dev );
        EGIGA_STAT( EGIGA_STAT_TX, (priv->eth_stat.tx_netif_stop[ETH_DEF_TXQ]++) );
    }
#endif
    spin_unlock_irqrestore( &(priv->lock), flags );

    return ret;
}

/*********************************************************** 
 * eth_tx_done --                                             *
 *   release transmitted packets. interrupt context.       *
 ***********************************************************/
static u32 eth_tx_done( struct net_device *dev )
{
    eth_priv *priv = dev->priv;
    struct net_device_stats *stats = &priv->stats;
    MV_PKT_INFO pkt_info;
    u32 count = 0;
    MV_STATUS status;
    unsigned int queue = 0;

    ETH_DBG( ETH_DBG_TX_DONE, ("%s: tx-done ", dev->name) );
    EGIGA_STAT( EGIGA_STAT_TX_DONE, (priv->eth_stat.tx_done_events++) );

    eth_read_save_clear_tx_cause(dev); 

    /* release the transmitted packets */
    while( 1 ) {

#ifdef INCLUDE_MULTI_QUEUE
 #if defined(CONFIG_MV_INCLUDE_UNM_ETH) /* UNIMAC */
	/* Note: in UniMAC we do not break if priv->txcause == 0 - we check the Tx ring anyway */
	/* That is because the Interrupt Cause Register is not updated with new events, if they are masked */
	/* in the Interrupt Mask Register. */
	/* Do not uncomment */
        /* if(priv->txcause == 0) */
	/*     break; */
 #else /* not UNIMAC */
        if(priv->txcause == 0)
            break;
        while( (priv->txcause & ETH_CAUSE_TX_BUF_MASK(queue)) == 0) 
        {
            queue++; /* Can't pass MAX Q */
        }

 #endif /* UNIMAC */
#else
        queue = ETH_DEF_TXQ;
#endif /* INCLUDE_MULTI_QUEUE */

        /* get a packet */  
        status = mvEthPortTxDone( priv->hal_priv, queue, &pkt_info );
    if( status == MV_OK ) {

        priv->txq_count[queue]--;

        /* handle tx error */
        if( pkt_info.status & (ETH_ERROR_SUMMARY_BIT) ) {
                ETH_DBG( ETH_DBG_TX_DONE, ("%s: bad tx-done status\n",dev->name) );
                EGIGA_STAT( EGIGA_STAT_TX_DONE, (priv->eth_stat.tx_done_hal_bad_stat[queue]++) );
            stats->tx_errors++;
        }

        count++;
        EGIGA_STAT( EGIGA_STAT_TX_DONE, (priv->eth_stat.tx_done_hal_ok[queue]++) );

        /* validate skb */
        if( !(pkt_info.osInfo) ) {
            EGIGA_STAT( EGIGA_STAT_TX_DONE, (priv->eth_stat.tx_done_hal_invalid_skb[queue]++) );
            continue;
        }

        /* release the skb */
        dev_kfree_skb_any( (struct sk_buff *)pkt_info.osInfo );
    }
    else {
        if( status == MV_EMPTY ) {
                /* no more work */
                ETH_DBG( ETH_DBG_TX_DONE, ("no more work ") );
                EGIGA_STAT( EGIGA_STAT_TX_DONE, (priv->eth_stat.tx_done_hal_no_more[queue]++) );
        }
        else if( status == MV_NOT_FOUND ) {
                /* hw still in tx */
                ETH_DBG( ETH_DBG_TX_DONE, ("hw still in tx ") );
                EGIGA_STAT( EGIGA_STAT_TX_DONE, (priv->eth_stat.tx_done_hal_still_tx[queue]++) );
        }
        else {
                printk( KERN_ERR "%s: unrecognize status on tx done\n", dev->name );
                EGIGA_STAT( EGIGA_STAT_TX_DONE, (priv->eth_stat.tx_done_hal_unrecognize[queue]++) );
                stats->tx_errors++;
        }
#if defined(CONFIG_MV_INCLUDE_UNM_ETH)
	    priv->txcause &= ~ETH_CAUSE_TX_BUF_MASK(queue);
	    break;
#else
#ifdef INCLUDE_MULTI_QUEUE
            priv->txcause &= ~ETH_CAUSE_TX_BUF_MASK(queue);
#else
        break;
#endif
#endif
        }
    }

    /* it transmission was previously stopped, now it can be restarted. */
    if( netif_queue_stopped( dev ) && (dev->flags & IFF_UP) && count > 0) {
        ETH_DBG( ETH_DBG_TX_DONE, ("%s: restart transmit\n", dev->name) );
        EGIGA_STAT( EGIGA_STAT_TX_DONE, (priv->eth_stat.tx_done_netif_wake++) );
        netif_wake_queue( dev );    
    }
    EGIGA_STAT( EGIGA_STAT_TX_DONE, if(priv->eth_stat.tx_done_max < count) priv->eth_stat.tx_done_max = count );
    EGIGA_STAT( EGIGA_STAT_TX_DONE, priv->eth_stat.tx_done_distribution[count]++);
    ETH_DBG( ETH_DBG_TX_DONE, ("%s: tx-done %d\n", dev->name, count) );
    return count;
}



/*********************************************************** 
 * eth_tx_timeout --                                       *
 *   nothing to be done (?)                                *
 ***********************************************************/
static void eth_tx_timeout( struct net_device *dev ) 
{
    EGIGA_STAT( EGIGA_STAT_TX, ( ((eth_priv*)&(dev->priv))->eth_stat.tx_timeout++) );
    printk( KERN_INFO "%s: tx timeout\n", dev->name );
}

#ifdef RX_CSUM_OFFLOAD
static MV_STATUS eth_rx_csum_offload(MV_PKT_INFO *pkt_info)
{
    if( (pkt_info->pktSize > ETH_CSUM_MIN_BYTE_COUNT)   && /* Minimum        */
        (pkt_info->status & ETH_RX_IP_FRAME_TYPE_MASK) && /* IPv4 packet    */
        (pkt_info->status & ETH_RX_IP_HEADER_OK_MASK)  && /* IP header OK   */
        (!(pkt_info->fragIP))                          && /* non frag IP    */
        (!(pkt_info->status & ETH_RX_L4_OTHER_TYPE))   && /* L4 is TCP/UDP  */
        (pkt_info->status & ETH_RX_L4_CHECKSUM_OK_MASK) ) /* L4 checksum OK */
            return MV_OK;

    if(!(pkt_info->pktSize > ETH_CSUM_MIN_BYTE_COUNT))
        ETH_DBG( ETH_DBG_RX, ("Byte count smaller than %d\n", ETH_CSUM_MIN_BYTE_COUNT) );
    if(!(pkt_info->status & ETH_RX_IP_FRAME_TYPE_MASK))
        ETH_DBG( ETH_DBG_RX, ("Unknown L3 protocol\n") );
    if(!(pkt_info->status & ETH_RX_IP_HEADER_OK_MASK))
        ETH_DBG( ETH_DBG_RX, ("Bad IP csum\n") );
    if(pkt_info->fragIP)
        ETH_DBG( ETH_DBG_RX, ("Fragmented IP\n") );
    if(pkt_info->status & ETH_RX_L4_OTHER_TYPE)
        ETH_DBG( ETH_DBG_RX, ("Unknown L4 protocol\n") );
    if(!(pkt_info->status & ETH_RX_L4_CHECKSUM_OK_MASK))
        ETH_DBG( ETH_DBG_RX, ("Bad L4 csum\n") );

    return MV_FAIL;
}
#endif /* RX_CSUM_OFFLOAD */


static int eth_poll( struct net_device *dev, int *budget )
{
    int             rx_work_done, rx_todo;
    unsigned long   flags;
#ifdef EGIGA_STATISTICS
    eth_priv        *priv = dev->priv;
#endif

    EGIGA_STAT( EGIGA_STAT_INT, (priv->eth_stat.rx_poll_events++) );

#ifdef ETH_TX_DONE_ISR
    eth_tx_done(dev);
#endif
    rx_todo = min(*budget,dev->quota);
    rx_work_done = eth_rx( dev, rx_todo);

    *budget -= rx_work_done;
    dev->quota -= rx_work_done;

    ETH_DBG( ETH_DBG_INT, ("poll work done: tx-%d rx-%d\n",tx_work_done,rx_work_done) );

    if( (rx_work_done != rx_todo) || (!netif_running(dev)) ) { 
        local_irq_save(flags);
        netif_rx_complete(dev);
        EGIGA_STAT( EGIGA_STAT_INT, (((eth_priv*)&(dev->priv))->eth_stat.rx_poll_netif_complete++) );
        eth_unmask_interrupts(dev);
	    ETH_DBG( ETH_DBG_RX, ("unmask\n") );
        local_irq_restore(flags);
        return 0;
    }

    return 1;
}

/*********************************************************** 
 * eth_rx_poll --                                        *
 *   NAPI rx polling method. deliver rx packets to linux   *
 *   core. refill new rx buffers. unmaks rx interrupt only *
 *   if all packets were delivered.                        *
 ***********************************************************/
static int eth_rx( struct net_device *dev,unsigned int work_to_do )
{
    eth_priv *priv = dev->priv;
    struct net_device_stats *stats = &(priv->stats);
    struct sk_buff *skb;
    MV_PKT_INFO pkt_info;
    int work_done = 0;
    MV_STATUS status;
    unsigned int queue = 0; 

#ifdef INCLUDE_MULTI_QUEUE
    eth_read_save_clear_rx_cause(dev);
 
    ETH_DBG( ETH_DBG_RX,("%s: cause = 0x%08x\n\n", dev->name, priv->rxcause) );
#endif /* INCLUDE_MULTI_QUEUE */

    ETH_DBG( ETH_DBG_RX, ("%s: rx_poll work_to_do %d\n", dev->name, work_to_do) );

    EGIGA_STAT( EGIGA_STAT_RX, (priv->eth_stat.rx_poll_events++) );

    /* fairness NAPI loop */
    while( work_done < work_to_do ) {

#ifdef INCLUDE_MULTI_QUEUE
        if(priv->rxcause == 0)
            break;
        queue = mvEthRxPolicyGet(priv->pRxPolicyHndl, priv->rxcause);
#else
        queue = ETH_DEF_RXQ;
#endif /* INCLUDE_MULTI_QUEUE */

        /* get rx packet */ 
        status = mvEthPortRx( priv->hal_priv, queue, &pkt_info );

        /* check status */
        if( status == MV_OK ) {
            work_done++;
            priv->rxq_count[queue]--;
            EGIGA_STAT( EGIGA_STAT_RX, (priv->eth_stat.rx_poll_hal_ok[queue]++) );
        } else { 
            if( status == MV_NO_RESOURCE ) {
                /* no buffers for rx */
                ETH_DBG( ETH_DBG_RX, ("%s: rx_poll no resource ", dev->name) );
                stats->rx_errors++;
                EGIGA_STAT( EGIGA_STAT_RX, (priv->eth_stat.rx_poll_hal_no_resource[queue]++) );

            } else if( status == MV_NO_MORE ) {
                /* no more rx packets ready */
                ETH_DBG( ETH_DBG_RX, ("%s: rx_poll no more ", dev->name) );
                EGIGA_STAT( EGIGA_STAT_RX, (priv->eth_stat.rx_poll_hal_no_more[queue]++) );

            } else {
                printk( KERN_ERR "%s: unrecognized status on rx poll\n", dev->name );
                stats->rx_errors++;
                EGIGA_STAT( EGIGA_STAT_RX, (priv->eth_stat.rx_poll_hal_error[queue]++) );
            }

#ifdef INCLUDE_MULTI_QUEUE
            priv->rxcause &= ~ETH_CAUSE_RX_READY_MASK(queue);
            continue;
#else
            break;
#endif
        }

        /* validate skb */ 
        if( !(pkt_info.osInfo) ) {
            printk( KERN_ERR "%s: error in rx\n",dev->name );
            stats->rx_errors++;
            EGIGA_STAT( EGIGA_STAT_RX, (priv->eth_stat.rx_poll_hal_invalid_skb[queue]++) );
            continue;
        }

        skb = (struct sk_buff *)( pkt_info.osInfo );

        /* handle rx error */
        if( pkt_info.status & (ETH_ERROR_SUMMARY_MASK) ) {
            u32 err = pkt_info.status & ETH_RX_ERROR_CODE_MASK;

            eth_print_rx_errors(err, pkt_info.status);

	        dev_kfree_skb(skb);
            stats->rx_errors++;
            EGIGA_STAT( EGIGA_STAT_RX, (priv->eth_stat.rx_poll_hal_bad_stat[queue]++) );
            continue;
        }

        /* good rx */
        ETH_DBG( ETH_DBG_RX, ("good rx. skb=%p, skb->data=%p\n", skb, skb->data) );
        stats->rx_packets++;
        stats->rx_bytes += pkt_info.pktSize; /* include 4B crc */


        /* reduce 4B crc, 2B added by GbE HW */
        skb_put( skb, (pkt_info.pktSize - 4 - eth_get_hw_header_size()) );
        skb->dev = dev;

	    eth_align_ip_header(skb);   /* align IP header by shifting 2 bytes, if not done by HW */

        if(eth_loopback == 1)
        {
            status = eth_tx(skb, dev);
        }
        else if(eth_loopback == 2)
        {
            /* RX only */
            dev_kfree_skb_any(skb);           
        }
        else
        {
            prefetch( (void *)(skb->data) );

#ifdef RX_CSUM_OFFLOAD
            /* checksum offload */
            if( eth_rx_csum_offload( &pkt_info ) == MV_OK ) {

                ETH_DBG( ETH_DBG_RX, ("%s: rx csum offload ok\n", dev->name) );
                /* EGIGA_STAT( EGIGA_STAT_RX, Add counter here) */

                skb->ip_summed = CHECKSUM_UNNECESSARY;

                /* Is this necessary? */
                skb->csum = htons((pkt_info.status & ETH_RX_L4_CHECKSUM_MASK) >> ETH_RX_L4_CHECKSUM_OFFSET);
            }
            else {
                ETH_DBG( ETH_DBG_RX, ("%s: rx csum offload failed\n", dev->name) );
                /* EGIGA_STAT( EGIGA_STAT_RX, Add counter here) */
                skb->ip_summed = CHECKSUM_NONE;
            }
#else
            skb->ip_summed = CHECKSUM_NONE;
#endif
            skb->protocol = eth_type_trans(skb, dev); 
            status = netif_receive_skb( skb );
            EGIGA_STAT( EGIGA_STAT_RX, if(status) (priv->eth_stat.rx_poll_netif_drop[queue]++) );
        }
    }

    EGIGA_STAT( EGIGA_STAT_RX, priv->eth_stat.rx_poll_distribution[work_done]++); 
    ETH_DBG( ETH_DBG_RX, ("\nwork_done %d (%d)", work_done, priv->rxq_count[queue]) );

    /* refill rx ring with new buffers */
    if(work_done)
        eth_rx_fill(dev);

    /* notify upper layer about more work to do */
    return( work_done );
}


/*********************************************************** 
 * eth_rx_fill --                                        *
 *   fill new rx buffers to ring.                          *
 ***********************************************************/
static void eth_rx_fill(struct net_device *dev)
{
    eth_priv *priv = dev->priv;
    MV_PKT_INFO pkt_info;
    MV_BUF_INFO bufInfo;
    struct sk_buff *skb;
    u32 count = 0, buf_size;
    MV_STATUS status;
    int alloc_skb_failed = 0;
    unsigned int queue = 0;
    int total = 0;

    for (queue = 0; queue < MV_ETH_RX_Q_NUM; queue++) {

	    /* Don't try to fill the ring for this queue if it is already full */
	    if (mvEthRxResourceGet(priv->hal_priv, queue) >= ethDescRxQ[queue])
	        continue;

    	total = ethDescRxQ[queue];
	    count = 0;
    	ETH_DBG( ETH_DBG_RX_FILL, ("%s: rx fill queue %d", dev->name, queue) );
    	EGIGA_STAT( EGIGA_STAT_RX_FILL, (priv->eth_stat.rx_fill_events[queue]++) );

        while( total-- ) {

        /* allocate a buffer */
        buf_size = RX_BUFFER_SIZE( dev->mtu, priv) + 32 /* 32(extra for cache prefetch) */ + 8 /* +8 to align on 8B */;

        skb = dev_alloc_skb( buf_size ); 
        if( !skb ) {
            ETH_DBG( ETH_DBG_RX_FILL, ("%s: rx_fill cannot allocate skb\n", dev->name) );
            EGIGA_STAT( EGIGA_STAT_RX_FILL, (priv->eth_stat.rx_fill_alloc_skb_fail[queue]++) );
            alloc_skb_failed = 1;
            break;
        }

        /* align the buffer on 8B */
        if( (unsigned long)(skb->data) & 0x7 ) {
            skb_reserve( skb, 8 - ((unsigned long)(skb->data) & 0x7) );
        }

        bufInfo.bufVirtPtr = skb->data;
        bufInfo.bufSize = RX_BUFFER_SIZE( dev->mtu, priv);
        pkt_info.osInfo = (MV_ULONG)skb;
        pkt_info.pFrags = &bufInfo;
        pkt_info.pktSize = RX_BUFFER_SIZE( dev->mtu, priv); /* how much to invalidate */

    	/* skip on first 2B (GbE HW header) */
    	skb_reserve( skb, eth_get_hw_header_size() ); 

        /* give the buffer to hal */
        status = mvEthPortRxDone( priv->hal_priv, queue, &pkt_info );
    
        if( status == MV_OK ) {
            count++;
            priv->rxq_count[queue]++;
            	EGIGA_STAT( EGIGA_STAT_RX_FILL, (priv->eth_stat.rx_fill_hal_ok[queue]++) );       
        }
        else if( status == MV_FULL ) {
            /* the ring is full */
            count++;
            priv->rxq_count[queue]++;
            ETH_DBG( ETH_DBG_RX_FILL, ("%s: rxq full\n", dev->name) );
            EGIGA_STAT( EGIGA_STAT_RX_FILL, (priv->eth_stat.rx_fill_hal_full[queue]++) );
            if( priv->rxq_count[queue] != ethDescRxQ[queue])
                printk( KERN_ERR "%s Q %d: error in status fill (%d != %d)\n", dev->name, queue, priv->rxq_count[queue], 
                                                ethDescRxQ[queue]);
            break;
        } 
        else {
            printk( KERN_ERR "%s Q %d: error in rx-fill\n", dev->name, queue );
            	EGIGA_STAT( EGIGA_STAT_RX_FILL, (priv->eth_stat.rx_fill_hal_error[queue]++) );
	    	dev_kfree_skb(skb);
            break;
        }
    }

    /* if allocation failed and the number of rx buffers in the ring is less than */
    /* half of the ring size, then set a timer to try again later.                */
        if( alloc_skb_failed && (priv->rxq_count[queue] < (ethDescRxQ[queue]/2)) ) {
            if( priv->rx_fill_flag == 0 ) {
            	printk( KERN_INFO "%s: setting rx timeout to allocate skb\n", dev->name);
                priv->rx_fill_timer.expires = jiffies + (HZ/10); /*100ms*/
                add_timer( &priv->rx_fill_timer );
                priv->rx_fill_flag = 1;
            }
	        break;
        }

        ETH_DBG( ETH_DBG_RX_FILL, ("queue %d: rx fill %d (total %d)", queue, count, priv->rxq_count[queue]) );
    }
}

#if (ETH_TX_TIMER_PERIOD > 0)

/*********************************************************** 
 * eth_tx_timer_callback --                              *
 *   100 msec periodic callback to prevent TX stack.       *
 ***********************************************************/
static void eth_tx_timer_callback(  unsigned long data )
{
    struct net_device   *dev = (struct net_device *)data;
    eth_priv            *priv = dev->priv;

    ETH_DBG( ETH_DBG_TX, ("%s: tx_timer_callback", dev->name) );
    EGIGA_STAT( EGIGA_STAT_RX_FILL, (priv->eth_stat.tx_timer_events++) );
    if( !netif_queue_stopped( dev ) )
    {
        /* TX enable */
        mvEthPortTxEnable(priv->hal_priv);
    }
    /* Call TX done */
#ifdef ETH_TX_DONE_ISR
#else
    if(priv->txq_count[ETH_DEF_TXQ] > 0)
    {
        unsigned long flags;

        spin_lock_irqsave( &(priv->lock), flags );

        eth_tx_done(dev);

        spin_unlock_irqrestore( &(priv->lock), flags);
    }
#endif /* ETH_TX_DONE_ISR */ 

    if(priv->tx_timer_flag)
    {
        priv->tx_timer.expires = jiffies + ((HZ*ETH_TX_TIMER_PERIOD)/1000); /*ms*/
        add_timer( &priv->tx_timer );
    }
}
#endif /* ETH_TX_TIMER_PERIOD */
/*********************************************************** 
 * eth_rx_fill_on_timeout --                             *
 *   previous rx fill failed allocate skb. try now again.  *
 ***********************************************************/
static void eth_rx_fill_on_timeout( unsigned long data ) 
{
    struct net_device *dev = (struct net_device *)data;
    eth_priv *priv = dev->priv;
    unsigned long flags;

    spin_lock_irqsave( &(priv->lock), flags );

    ETH_DBG( ETH_DBG_RX_FILL, ("%s: rx_fill_on_timeout", dev->name) );
    EGIGA_STAT( EGIGA_STAT_RX_FILL, (priv->eth_stat.rx_fill_timeout_events++) );
   
    priv->rx_fill_flag = 0;
    eth_rx_fill(dev);

    spin_unlock_irqrestore( &(priv->lock), flags );
}


static void eth_check_phy_link_status(struct net_device *dev, int print_flag)
{
    eth_priv *priv = dev->priv;
    MV_U32 phy_reg_data = 0;

    if (dev == NULL)
	    return;

#ifndef CONFIG_MV_INCLUDE_UNM_ETH
    if( priv->picer & (BIT16 | BIT20) ) 
#endif
    {
        EGIGA_STAT( EGIGA_STAT_INT, (priv->eth_stat.int_phy_events++) );

    	/* Check Link status on ethernet port */
	ReadMiiWrap(mvBoardPhyAddrGet(priv->port), ETH_PHY_STATUS_REG, &phy_reg_data);
#ifdef CONFIG_MV_INCLUDE_UNM_ETH
	/* If link status changed, print new link status */
	if (phy_link_up != (phy_reg_data & ETH_PHY_STATUS_AN_DONE_MASK)) {
	    print_flag = 1;
	    phy_link_up = (phy_reg_data & ETH_PHY_STATUS_AN_DONE_MASK);
	} 
#endif
    	if( !(phy_reg_data & ETH_PHY_STATUS_AN_DONE_MASK) ) { 
            netif_carrier_off( dev );
            netif_stop_queue( dev );
            eth_down_internals( dev );
        }
    	else
        {
            mvEthPortUp( priv->hal_priv );
            netif_carrier_on( dev );
            netif_wake_queue( dev );            
    	}
	if(print_flag)
    	    eth_print_phy_status( dev );
    } 
}

/*********************************************************** 
 * eth_interrupt_handler --                              *
 *   serve rx-q0, tx-done-q0, phy/link state change.       *
 *   phy is served in interrupt context.           *
 *   tx and rx are scheduled out of interrupt context (NAPI poll)  *
 ***********************************************************/
static irqreturn_t eth_interrupt_handler( int irq , void *dev_id)
{
    struct net_device *dev = (struct net_device *)dev_id;
    eth_priv *priv = dev->priv;
    int port = priv->port;
 
    spin_lock( &(priv->lock) );
    
    ETH_DBG( ETH_DBG_INT, ("\n%s: isr ", dev->name) );
    EGIGA_STAT( EGIGA_STAT_INT, (priv->eth_stat.int_total++) );

    /* read port interrupt cause register */
    eth_read_interrupt_cause_regs(dev);

    ETH_DBG( ETH_DBG_INT, ("[picr %08x]", priv->picr) );
    if( !priv->picr ) {
        EGIGA_STAT( EGIGA_STAT_INT, (priv->eth_stat.int_none_events++) );
    	spin_unlock( &(priv->lock) );
	/* when using coalsing, we might get once in a ... a bogus int ?!?! -> ignore it. */
        return IRQ_HANDLED;
    }

    eth_save_clear_rx_tx_cause(dev);

#ifndef CONFIG_MV_INCLUDE_UNM_ETH
    eth_check_phy_link_status(dev, 1);
#else
    /* Phy link status checked using a timer every 1 second */
#endif

    /* Verify that the device not already on the polling list */
	if (netif_rx_schedule_prep(dev)) {

	eth_mask_interrupts(dev);
	eth_clear_interrupts(port);

	    /* schedule the work (rx+txdone) out of interrupt contxet */
	    __netif_rx_schedule(dev);
	}
	else {
	    if(netif_running(dev)) {
            eth_print_int_while_polling(dev);
	}
    }

    spin_unlock( &(priv->lock) );

    return IRQ_HANDLED;
}



/*********************************************************** 
 * eth_get_stats --                                      *
 *   return the device statistics.                         *
 *   print private statistics if compile flag set.         *
 ***********************************************************/
static struct net_device_stats* eth_get_stats( struct net_device *dev )
{
    return &(((eth_priv *)dev->priv)->stats);
}

/***********************************************************
 * eth_set_multicast_list --                             *
 *   Add multicast addresses or set promiscuous mode.      *
 *   This function should have been but was not included   *
 *   by Marvell. -bbozarth                                 *
 ***********************************************************/
static void eth_set_multicast_list(struct net_device *dev) {

     eth_priv *priv = dev->priv;
     int queue = ETH_DEF_RXQ;
     struct dev_mc_list *curr_addr = dev->mc_list;
     int i;

     if (dev->flags & IFF_PROMISC)
     {
        mvEthRxFilterModeSet(priv->hal_priv, 1);
     }
     else if (dev->flags & IFF_ALLMULTI)
     {
#if defined(CONFIG_MV_INCLUDE_UNM_ETH) /* UNIMAC */
	 /* BTS #63: Despite the existence of PM_MC (promisc. multicast) bit in UniMAC, */
	 /* It is actually only possible to be in both unicast and multicast promiscuous mode together */
	 mvEthRxFilterModeSet(priv->hal_priv, 1);
#else
        mvEthRxFilterModeSet(priv->hal_priv, 0);
        mvEthMacAddrSet(priv->hal_priv, dev->dev_addr, queue);
        mvEthSetSpecialMcastTable(priv->port, queue);
        mvEthSetOtherMcastTable(priv->port, queue);
#endif
     }
     else if (dev->mc_count)
     {
        mvEthRxFilterModeSet(priv->hal_priv, 0);
        mvEthMacAddrSet(priv->hal_priv, dev->dev_addr, queue);
        for (i=0; i<dev->mc_count; i++, curr_addr = curr_addr->next)
        {
            if (!curr_addr)
                break;
            mvEthMcastAddrSet(priv->hal_priv, curr_addr->dmi_addr, queue);
        }
     }
     else /* No Mcast addrs, not promisc or all multi - clear tables */
     {
        mvEthRxFilterModeSet(priv->hal_priv, 0);
        mvEthMacAddrSet(priv->hal_priv, dev->dev_addr, queue);
     }
}


/*********************************************************** 
 * eth_set_mac_addr --                                   *
 *   stop port activity. set new addr in device and hw.    *
 *   restart port activity.                                *
 ***********************************************************/
static int eth_set_mac_addr_internals(struct net_device *dev, void *addr )
{
    eth_priv *priv = dev->priv;
    u8* mac = &(((u8*)addr)[2]);  /* skip on first 2B (ether HW addr type) */
    int i;

    /* remove previous address table entry */
    if( mvEthMacAddrSet( priv->hal_priv, dev->dev_addr, -1) != MV_OK ) {
        printk( KERN_ERR "%s: ethSetMacAddr failed\n", dev->name );
        return -1;
    }

    /* set new addr in hw */
    if( mvEthMacAddrSet( priv->hal_priv, mac, ETH_DEF_RXQ) != MV_OK ) {
        printk( KERN_ERR "%s: ethSetMacAddr failed\n", dev->name );
    return -1;
    }

    /* set addr in the device */ 
    for( i = 0; i < 6; i++ )
        dev->dev_addr[i] = mac[i];

    printk( KERN_NOTICE "%s: mac address changed\n", dev->name );
#ifdef CONFIG_BUFFALO_PLATFORM
	memcpy(&buffalo_struc.mac, mac, 6);
#endif

    return 0;
}

static int eth_set_mac_addr( struct net_device *dev, void *addr )
{
    if(!netif_running(dev)) {
        if(eth_set_mac_addr_internals(dev, addr) == -1)
            goto error;
        return 0;
    }

    if( eth_stop( dev )) {
        printk( KERN_ERR "%s: stop interface failed\n", dev->name );
        goto error;
    }

    if(eth_set_mac_addr_internals(dev, addr) == -1)
        goto error;

    if(eth_start( dev )) {
        printk( KERN_ERR "%s: start interface failed\n", dev->name );
    goto error;
    } 

    return 0;

 error:
    printk( "%s: set mac addr failed\n", dev->name );
    return -1;
}

#ifndef CONFIG_MV_INCLUDE_UNM_ETH
/*********************************************************** 
 * eth_change_mtu --                                     *
 *   stop port activity. release skb from rings. set new   *
 *   mtu in device and hw. restart port activity and       *
 *   and fill rx-buiffers with size according to new mtu.  *
 ***********************************************************/
static int eth_change_mtu_internals( struct net_device *dev, int mtu )
{
    eth_priv *priv = dev->priv;

    if(mtu < 1498 /* 1518 - 20 */) {
        printk( "%s: Ilegal MTU value %d, ", dev->name, mtu);
        mtu = 1500;
        printk(" rounding MTU to: %d \n",mtu);
    }
    else if(mtu > 9676 /* 9700 - 20 and rounding to 8 */) {
        printk( "%s: Ilegal MTU value %d, ", dev->name, mtu);
        mtu = 9676;
        printk(" rounding MTU to: %d \n",mtu);  
    }
      
    if(RX_BUFFER_SIZE( mtu, priv) & ~ETH_RX_BUFFER_MASK) {
        printk( "%s: Ilegal MTU value %d, ", dev->name, mtu);
        mtu = 8 - (RX_BUFFER_SIZE( mtu, priv) & ~ETH_RX_BUFFER_MASK) + mtu;
        printk(" rounding MTU to: %d \n",mtu);
    }

    /* set mtu in device and in hal sw structures */
    if( mvEthMaxRxSizeSet( priv->hal_priv, RX_BUFFER_SIZE( mtu, priv)) ) {
        printk( KERN_ERR "%s: ethPortSetMaxBufSize failed\n", dev->name );
        return -1;
    }
    
#if defined(ETH_INCLUDE_TSO)
    dev->features |= NETIF_F_TSO;
#endif /* ETH_INCLUDE_TSO */
#ifdef TX_CSUM_OFFLOAD
    if(mtu <= ETH_CSUM_MAX_BYTE_COUNT) {
        dev->features |= NETIF_F_IP_CSUM;
    }
    else {
        /* Without CSUM offload - NO TSO support */
        dev->features &= ~(NETIF_F_IP_CSUM | NETIF_F_TSO);
    }
#endif /* TX_CSUM_OFFLOAD */

    dev->mtu = mtu;

#ifdef CONFIG_BUFFALO_PLATFORM
	buffalo_struc.mtu = mtu;
#endif
    return 0;
}

static int eth_change_mtu( struct net_device *dev, int mtu )
{
    int old_mtu = dev->mtu;

    if(!netif_running(dev)) {
    	if(eth_change_mtu_internals(dev, mtu) == -1) {
        goto error;
    }
        printk( KERN_NOTICE "%s: change mtu %d (buffer-size %d) to %d (buffer-size %d)\n",
        dev->name, old_mtu, RX_BUFFER_SIZE( old_mtu, dev->priv), dev->mtu, RX_BUFFER_SIZE( dev->mtu, dev->priv) );
        return 0;
    }

    if( eth_stop( dev )) {
        printk( KERN_ERR "%s: stop interface failed\n", dev->name );
        goto error;
    }

    if(eth_change_mtu_internals(dev, mtu) == -1) {
        goto error;
    }

    if(eth_start( dev )) {
        printk( KERN_ERR "%s: start interface failed\n", dev->name );
        goto error;
    } 
    printk( KERN_NOTICE "%s: change mtu %d (buffer-size %d) to %d (buffer-size %d)\n",
                dev->name, old_mtu, RX_BUFFER_SIZE( old_mtu, priv), dev->mtu, 
                RX_BUFFER_SIZE( dev->mtu, priv));
 
    return 0;

 error:
    printk( "%s: change mtu failed\n", dev->name );
    return -1;
}
#endif
/**************************************************************************************************************/

static inline void eth_print_rx_errors(unsigned int err, MV_U32 pkt_info_status)
{
#if defined(CONFIG_MV_INCLUDE_UNM_ETH) /* UNIMAC */
    if (err & ETH_OVERRUN_ERROR_BIT) {
	ETH_DBG( ETH_DBG_RX, ("bad rx status %08x, (overrun error)\n",(unsigned int)pkt_info_status));
    }
    if (err & ETH_CRC_ERROR_BIT) {
	printk( KERN_INFO "bad rx status %08x, (crc error)\n",(unsigned int)pkt_info_status );
    }
#else /* not UNIMAC */
    /* RX resource error is likely to happen when receiving packets, which are     */
    /* longer then the Rx buffer size, and they are spreading on multiple buffers. */
    /* Rx resource error - No descriptor in the middle of a frame.                 */
    if( err == ETH_RX_RESOURCE_ERROR ) {
        ETH_DBG( ETH_DBG_RX, ("bad rx status %08x, (resource error)",(unsigned int)pkt_info_status));
    }
    else if( err == ETH_RX_OVERRUN_ERROR ) {
	ETH_DBG( ETH_DBG_RX, ("bad rx status %08x, (overrun error)",(unsigned int)pkt_info_status));
    }
    else {
	printk( KERN_INFO "bad rx status %08x, ",(unsigned int)pkt_info_status );
    	if( err == ETH_RX_MAX_FRAME_LEN_ERROR )
        	printk("(max frame length error)");
    	else if( err == ETH_RX_CRC_ERROR )
        	printk("(crc error)");
    	else
        	printk("(unknown error)");
    	printk("\n");
    }
#endif /* UNIMAC */
}

/**************************************************************************************************************/

static void eth_mac_addr_get(int port, char *addr)
{
#if defined(CONFIG_MV_INCLUDE_UNM_ETH)
    /* get it from bootloader info structure */
    memcpy(addr, mvMacAddr, 6);
    /* Note: we do not support multiple interfaces */
#else
    /* get it from HW */
   
    int i;
	for (i=0 ; i<6 ; i++)
	{
		if (mvMacAddr[i] != 0) break;
	}
	/* if mvMacAddr is Zero then read from the Port itself */
	if (i==6) 
	{
		mvEthMacAddrGet(port, addr);
	}
	else
	{
		memcpy(addr, mvMacAddr, 6);
		*(addr+5) = (char)(*(addr+5) + port);
	}
#endif
}

/**************************************************************************************************************/

static inline int eth_get_hw_header_size(void)
{
#if defined(CONFIG_MV_INCLUDE_UNM_ETH) /* UNIMAC */
    return 0;
#else
    return 2;
#endif
}

/**************************************************************************************************************/

static inline void eth_align_ip_header(struct sk_buff *skb)
{
    /* When using Gigabit Ethernet HW, 2 extra bytes are shifted by the HW to align the IP header */
    /* UniMAC HW does not do this so the IP header is not aligned. This function shifts the 2 extra bytes */
#if defined(CONFIG_MV_INCLUDE_UNM_ETH) /* UNIMAC */
    memmove( skb->data + 2, skb->data, skb->len);
    skb->data += 2;
    skb->tail += 2;
#endif /* UNIMAC */
}

/**************************************************************************************************************/

static inline void eth_read_interrupt_cause_regs(struct net_device *dev)
{
    eth_priv *priv = dev->priv;

    priv->picr = (MV_REG_READ(ETH_INTR_CAUSE_REG(priv->port)) & MV_REG_READ(ETH_INTR_MASK_REG(priv->port)));
#if defined(CONFIG_MV_INCLUDE_UNM_ETH) /* UNIMAC */
    priv->picer = 0; /* Not available in UniMAC HW */
#else /* not UNIMAC */
    if(priv->picr & BIT1) 
	priv->picer = MV_REG_READ(ETH_INTR_CAUSE_EXT_REG(priv->port)) & MV_REG_READ(ETH_INTR_MASK_EXT_REG(priv->port));
    else
        priv->picer = 0;
#endif /* UNIMAC */
}

/**************************************************************************************************************/

static inline void eth_save_clear_rx_tx_cause(struct net_device *dev)
{ 
    eth_priv *priv = dev->priv;

    /* save rx cause */
    priv->rxcause |= (priv->picr & ETH_RXQ_MASK) | 
			((priv->picr & ETH_RXQ_RES_MASK) >> (ETH_CAUSE_RX_ERROR_OFFSET - ETH_CAUSE_RX_READY_OFFSET));
#if defined(CONFIG_MV_INCLUDE_UNM_ETH) /* UNIMAC */
    /* save tx cause */
    priv->txcause |= (priv->picr & ETH_TXQ_MASK);

    /* Clear the specific interrupts according to the bits currently set in the cause_regs */
    MV_REG_WRITE(ETH_INTR_CAUSE_REG(priv->port), ~(priv->picr) );
#else /* not UNIMAC */
    /* save tx cause */
    priv->txcause |= (priv->picer & ETH_TXQ_MASK);

    /* Clear the specific interrupts according to the bits currently set in the cause_regs */
    MV_REG_WRITE(ETH_INTR_CAUSE_REG(priv->port), ~(priv->picr));
    if(priv->picr & BIT1) {
	if(priv->picer)
	    MV_REG_WRITE(ETH_INTR_CAUSE_EXT_REG(priv->port), ~(priv->picer));
    }
#endif /* UNIMAC */
}

/**************************************************************************************************************/

static inline void eth_print_int_while_polling(struct net_device *dev)
{
    eth_priv *priv = dev->priv;

    printk("Interrupt while in polling list\n");
    printk("Interrupt Cause Register = 0x%08x\n", priv->picr);
    printk("Interrupt Mask Register = 0x%08x\n", MV_REG_READ(ETH_INTR_MASK_REG(priv->port)));
#ifndef CONFIG_MV_INCLUDE_UNM_ETH
    printk("Interrupt Cause Extend Register = 0x%08x\n", priv->picer);
    printk("Interrupt Mask Extend Register = 0x%08x\n", MV_REG_READ(ETH_INTR_MASK_EXT_REG(priv->port)));
#endif
}

/**************************************************************************************************************/

static inline void eth_read_save_clear_tx_cause(struct net_device *dev)
{
    eth_priv *priv = dev->priv;

#if defined(CONFIG_MV_INCLUDE_UNM_ETH) /* UNIMAC */
    priv->txcause |= (MV_REG_READ(ETH_INTR_CAUSE_REG(priv->port)) & ETH_TXQ_MASK);
    MV_REG_WRITE(ETH_INTR_CAUSE_REG(priv->port), ~(priv->txcause & ETH_TXQ_MASK));
#else /* not UNIMAC */
    priv->txcause |= MV_REG_READ(ETH_INTR_CAUSE_EXT_REG(priv->port));
    MV_REG_WRITE(ETH_INTR_CAUSE_EXT_REG(priv->port), priv->txcause & (~ETH_TXQ_MASK) );
#endif /* UNIMAC */
}

/**************************************************************************************************************/

static inline void eth_read_save_clear_rx_cause(struct net_device *dev)
{
    eth_priv *priv = dev->priv;

#if defined(CONFIG_MV_INCLUDE_UNM_ETH) /* UNIMAC */
    /* The Interrupt Cause Register is not updated with new events, if they are masked  */
    /* in the Interrupt Mask Register. That is why we set eth_dev.rxcause to the entire */
    /* ETH_RXQ_MASK so that we go over all the Rx queues.                               */
    priv->rxcause = ETH_RXQ_MASK;
    MV_REG_WRITE(ETH_INTR_CAUSE_REG(priv->port), 
		~(priv->rxcause | (priv->rxcause << (ETH_CAUSE_RX_ERROR_OFFSET - ETH_CAUSE_RX_READY_OFFSET)) ) );
#else /* not UNIMAC */
    unsigned int temp = MV_REG_READ(ETH_INTR_CAUSE_REG(priv->port));
    priv->rxcause |= temp & ETH_RXQ_MASK;
    priv->rxcause |= (temp & ETH_RXQ_RES_MASK) >> (ETH_CAUSE_RX_ERROR_OFFSET - ETH_CAUSE_RX_READY_OFFSET);
    MV_REG_WRITE(ETH_INTR_CAUSE_REG(priv->port), 
		~(priv->rxcause | (priv->rxcause << (ETH_CAUSE_RX_ERROR_OFFSET - ETH_CAUSE_RX_READY_OFFSET)) ) );
#endif /* UNIMAC */
}

/**************************************************************************************************************/

static inline void eth_mask_interrupts(struct net_device *dev)
{
    eth_priv *priv = dev->priv;

    MV_REG_WRITE( ETH_INTR_MASK_REG(priv->port), 0 );
#ifndef CONFIG_MV_INCLUDE_UNM_ETH
    MV_REG_WRITE( ETH_INTR_MASK_EXT_REG(priv->port), 0 );
#endif
    priv->rxmask = 0;
    priv->txmask = 0;
}

/**************************************************************************************************************/

static inline void eth_unmask_interrupts(struct net_device *dev)
{
    eth_priv *priv = dev->priv;

#if defined(CONFIG_MV_INCLUDE_UNM_ETH) /* UNIMAC */
    /* unmask UniMAC Rx and Tx interrupts */
    MV_REG_WRITE( ETH_INTR_MASK_REG(priv->port), (ETH_TXQ_MASK | ETH_PICR_MASK) );
    priv->rxmask = ETH_PICR_MASK;
    priv->txmask = ETH_TXQ_MASK;
#else /* not UNIMAC */
    /* unmask GbE Rx and Tx interrupts */
    MV_REG_WRITE( ETH_INTR_MASK_REG(priv->port), ETH_PICR_MASK);
    MV_REG_WRITE( ETH_INTR_MASK_EXT_REG(priv->port), ETH_PICER_MASK); 
    priv->rxmask = ETH_PICR_MASK; 
    priv->txmask = ETH_PICER_MASK;
#endif /* UNIMAC */
}

/**************************************************************************************************************/

static inline void eth_clear_interrupts(int port)
{
    /* clear interrupts */
    MV_REG_WRITE( ETH_INTR_CAUSE_REG(port), 0 );
#ifndef CONFIG_MV_INCLUDE_UNM_ETH
    /* clear Tx interrupts */
    MV_REG_WRITE( ETH_INTR_CAUSE_EXT_REG(port), 0 );
#endif
}

/**************************************************************************************************************/

/***********************************************************************************
 ***  get device by port number 
 ***********************************************************************************/
#if (defined(EGIGA_STATISTICS) || defined(CONFIG_MV_INCLUDE_UNM_ETH))
static struct net_device* get_net_device_by_port_num(unsigned int port) {
    
    struct net_device *dev = NULL;
    switch(port) {
    case 0:
        dev = __dev_get_by_name(CONFIG_MV_ETH_NAME"0");
        break;
    case 1:
        dev = __dev_get_by_name(CONFIG_MV_ETH_NAME"1");
        break;
    case 2:
        dev = __dev_get_by_name(CONFIG_MV_ETH_NAME"2");
        break;
    default:
        printk("get_net_device_by_port_num: unknown port number.\n");   
    }
    return dev;
}
#endif

/*********************************************************** 
 * string helpers for mac address setting                  *
 ***********************************************************/
static void eth_convert_str_to_mac( char *source , char *dest ) 
{
    dest[0] = (eth_str_to_hex( source[0] ) << 4) + eth_str_to_hex( source[1] );
    dest[1] = (eth_str_to_hex( source[2] ) << 4) + eth_str_to_hex( source[3] );
    dest[2] = (eth_str_to_hex( source[4] ) << 4) + eth_str_to_hex( source[5] );
    dest[3] = (eth_str_to_hex( source[6] ) << 4) + eth_str_to_hex( source[7] );
    dest[4] = (eth_str_to_hex( source[8] ) << 4) + eth_str_to_hex( source[9] );
    dest[5] = (eth_str_to_hex( source[10] ) << 4) + eth_str_to_hex( source[11] );
}
static unsigned int eth_str_to_hex( char ch ) 
{
    if( (ch >= '0') && (ch <= '9') )
        return( ch - '0' );

    if( (ch >= 'a') && (ch <= 'f') )
    return( ch - 'a' + 10 );

    if( (ch >= 'A') && (ch <= 'F') )
    return( ch - 'A' + 10 );

    return 0;
}


/***********************************************************************************
 ***  print port statistics
 ***********************************************************************************/
#define   STAT_PER_Q(qnum,x) for(queue = 0; queue < qnum; queue++) \
                printk("%10u ",x[queue]); \
                    printk("\n");

void print_eth_stat( unsigned int port )
{
#ifndef EGIGA_STATISTICS
  printk(" Error: eth is compiled without statistics support!! \n");
  return;
#else
  struct net_device *dev = get_net_device_by_port_num(port);
  eth_priv *priv = dev? (eth_priv *)(dev->priv) : NULL;
  eth_statistics *stat = priv? &(priv->eth_stat) : NULL;
  unsigned int queue, i;

  BUG_ON(dev == NULL);
  BUG_ON(priv == NULL);
  BUG_ON(stat == NULL);

      printk("QUEUS:.........................");
  for(queue = 0; queue < MV_ETH_RX_Q_NUM; queue++) 
      printk( "%10d ",queue);
  printk("\n");

  if( eth_stat & EGIGA_STAT_INT ) {
      printk( "\n====================================================\n" );
      printk( "%s: interrupt statistics", dev->name );
      printk( "\n-------------------------------\n" );
      printk( "int_total.....................%10u\n", stat->int_total );
      printk( "int_rx_events.................%10u\n", stat->int_rx_events );
      printk( "int_tx_done_events............%10u\n", stat->int_tx_done_events );
      printk( "int_phy_events................%10u\n", stat->int_phy_events );
      printk( "int_none_events...............%10u\n", stat->int_none_events );
  }
  if( eth_stat & EGIGA_STAT_RX ) {
      printk( "\n====================================================\n" );
      printk( "%s: rx statistics", dev->name );
      printk( "\n-------------------------------\n" );
      printk( "rx_poll_events................%10u\n", stat->rx_poll_events );
      printk( "rx_poll_hal_ok................"); STAT_PER_Q(MV_ETH_RX_Q_NUM, stat->rx_poll_hal_ok);
      printk( "rx_poll_hal_no_resource......."); STAT_PER_Q(MV_ETH_RX_Q_NUM, stat->rx_poll_hal_no_resource );
      printk( "rx_poll_hal_no_more..........."); STAT_PER_Q(MV_ETH_RX_Q_NUM, stat->rx_poll_hal_no_more );
      printk( "rx_poll_hal_error............."); STAT_PER_Q(MV_ETH_RX_Q_NUM, stat->rx_poll_hal_error );
      printk( "rx_poll_hal_invalid_skb......."); STAT_PER_Q(MV_ETH_RX_Q_NUM, stat->rx_poll_hal_invalid_skb );
      printk( "rx_poll_hal_bad_stat.........."); STAT_PER_Q(MV_ETH_RX_Q_NUM, stat->rx_poll_hal_bad_stat );
      printk( "rx_poll_netif_drop............"); STAT_PER_Q(MV_ETH_RX_Q_NUM, stat->rx_poll_netif_drop );
      printk( "rx_poll_netif_complete........%10u\n",stat->rx_poll_netif_complete );
      printk( "Current Rx Cause is...........%10x\n",priv->rxcause);
      for(i=0; i<sizeof(stat->rx_poll_distribution)/sizeof(u32); i++)
      {
          if(stat->rx_poll_distribution[i] != 0)
            printk("%d RxPkts - %d times\n", i, stat->rx_poll_distribution[i]);
      } 
  }
  if( eth_stat & EGIGA_STAT_RX_FILL ) {
      printk( "\n====================================================\n" );
      printk( "%s: rx fill statistics", dev->name );
      printk( "\n-------------------------------\n" );
      printk( "rx_fill_events................"); STAT_PER_Q(MV_ETH_RX_Q_NUM, stat->rx_fill_events );
      printk( "rx_fill_alloc_skb_fail........"); STAT_PER_Q(MV_ETH_RX_Q_NUM, stat->rx_fill_alloc_skb_fail );
      printk( "rx_fill_hal_ok................"); STAT_PER_Q(MV_ETH_RX_Q_NUM,stat->rx_fill_hal_ok);
      printk( "rx_fill_hal_full.............."); STAT_PER_Q(MV_ETH_RX_Q_NUM, stat->rx_fill_hal_full );
      printk( "rx_fill_hal_error............."); STAT_PER_Q(MV_ETH_RX_Q_NUM, stat->rx_fill_hal_error );
      printk( "rx_fill_timeout_events........%10u\n", stat->rx_fill_timeout_events );
      printk( "rx buffer size................%10u\n",RX_BUFFER_SIZE(dev->mtu, priv));
  }
  if( eth_stat & EGIGA_STAT_TX ) {
      printk( "\n====================================================\n" );
      printk( "%s: tx statistics", dev->name );
      printk( "\n-------------------------------\n" );
      printk( "tx_events.....................%10u\n", stat->tx_events );
      printk( "tx_hal_ok.....................");STAT_PER_Q(MV_ETH_TX_Q_NUM, stat->tx_hal_ok);
      printk( "tx_hal_no_resource............");STAT_PER_Q(MV_ETH_TX_Q_NUM, stat->tx_hal_no_resource );
      printk( "tx_hal_no_error...............");STAT_PER_Q(MV_ETH_TX_Q_NUM, stat->tx_hal_error );
      printk( "tx_hal_unrecognize............");STAT_PER_Q(MV_ETH_TX_Q_NUM, stat->tx_hal_unrecognize );
      printk( "tx_netif_stop.................");STAT_PER_Q(MV_ETH_TX_Q_NUM, stat->tx_netif_stop );
      printk( "tx_timeout....................%10u\n", stat->tx_timeout );
      printk( "tx_csum_offload...............%10u\n", stat->tx_csum_offload);
      printk( "tx_timer_events...............%10u\n", stat->tx_timer_events);
#ifdef INCLUDE_MULTI_QUEUE
      printk( "Current Tx Cause is...........%10x\n",priv->txcause);
#endif
  }
  printk("\n      TSO stats\n");
  for(i=0; i<64; i++)
  {
      if(stat->tso_stats[i] != 0)
      {
          printk("\t %d KBytes - %d times\n", i, stat->tso_stats[i]);
          stat->tso_stats[i] = 0;
      }
  } 
  if( eth_stat & EGIGA_STAT_TX_DONE ) {
      printk( "\n====================================================\n" );
      printk( "%s: tx-done statistics", dev->name );
      printk( "\n-------------------------------\n" );
      printk( "tx_done_events................%10u\n", stat->tx_done_events );
      printk( "tx_done_hal_ok................");STAT_PER_Q(MV_ETH_TX_Q_NUM, stat->tx_done_hal_ok);
      printk( "tx_done_hal_invalid_skb.......");STAT_PER_Q(MV_ETH_TX_Q_NUM, stat->tx_done_hal_invalid_skb );
      printk( "tx_done_hal_bad_stat..........");STAT_PER_Q(MV_ETH_TX_Q_NUM, stat->tx_done_hal_bad_stat );
      printk( "tx_done_hal_still_tx..........");STAT_PER_Q(MV_ETH_TX_Q_NUM, stat->tx_done_hal_still_tx );
      printk( "tx_done_hal_no_more...........");STAT_PER_Q(MV_ETH_TX_Q_NUM, stat->tx_done_hal_no_more );
      printk( "tx_done_hal_unrecognize.......");STAT_PER_Q(MV_ETH_TX_Q_NUM, stat->tx_done_hal_unrecognize );
      printk( "tx_done_max...................%10u\n", stat->tx_done_max );
      printk( "tx_done_netif_wake............%10u\n", stat->tx_done_netif_wake );
      for(i=0; i<sizeof(stat->tx_done_distribution)/sizeof(u32); i++)
      {
        if(stat->tx_done_distribution[i] != 0)
            printk("%d TxDonePkts - %d times\n", i, stat->tx_done_distribution[i]);
      } 
  }

  memset( stat, 0, sizeof(eth_statistics) );
#endif /*EGIGA_STATISTICS*/
}


/***********************************************************************************
 *** IN THE NEW ARCHITECTURE THE FOLLOWING FUNCTIONS SHOULD BE MOVED TO HAL/MCSP ***
 ***   phy_status, rx\tx_coal, get_dram\sram_base, adrress_decode, phy_addr      ***
 ***********************************************************************************/
static void eth_print_phy_status( struct net_device *dev ) 
{
    eth_priv *priv = dev->priv;
    int port = priv->port;
    u32 port_status;
    u32 phy_reg_val;
    
    /* check link status on phy */
    ReadMiiWrap(mvBoardPhyAddrGet(port), ETH_PHY_STATUS_REG , &phy_reg_val);
    
    if( !(phy_reg_val & ETH_PHY_STATUS_AN_DONE_MASK) )
    {
    	printk( KERN_NOTICE "%s: link down\n", dev->name );
#ifdef CONFIG_BUFFALO_PLATFORM
        buffalo_struc.linkspeed=0;
#endif
    } else {
    printk( KERN_NOTICE "%s: link up", dev->name );

#if defined(CONFIG_MV_INCLUDE_UNM_ETH) /* UNIMAC */
	/* Read speed and duplex directly from the Phy, page 0 register 17 */
	ReadMiiWrap(mvBoardPhyAddrGet(port), ETH_PHY_SPEC_STATUS_REG, &phy_reg_val);

	if (phy_reg_val & ETH_PHY_SPEC_STATUS_DUPLEX_MASK)
	    printk(", full duplex");
	else
	    printk(", half duplex");

	if ( (phy_reg_val & ETH_PHY_SPEC_STATUS_SPEED_MASK) == ETH_PHY_SPEC_STATUS_SPEED_1000MBPS)
	    printk(", speed 1 Gbps" );
	else if ( (phy_reg_val & ETH_PHY_SPEC_STATUS_SPEED_MASK) == ETH_PHY_SPEC_STATUS_SPEED_100MBPS)
	    printk(", speed 100 Mbps" );
	else
	    printk(", speed 10 Mbps" );

#else
        /* check port status register */
    port_status = MV_REG_READ( ETH_PORT_STATUS_REG( port ) );
        printk(", %s",(port_status & BIT2) ? "full duplex" : "half duplex" );
#ifdef CONFIG_BUFFALO_PLATFORM
        buffalo_struc.duplex_f = (port_status & BIT2);
#endif

#if defined(CONFIG_BUFFALO_PLATFORM)
	if( port_status & BIT4 )
	{
		printk(", speed 1 Gbps" );
		buffalo_struc.linkspeed=1000;
	}
        else
	{
		printk(", %s",(port_status & BIT5) ? "speed 100 Mbps" : "speed 10 Mbps" );
		buffalo_struc.linkspeed=((port_status & BIT5) ? 100 : 10 );
	}
#else
	if( port_status & BIT4 )
		printk(", speed 1 Gbps" );
        else
		printk(", %s",(port_status & BIT5) ? "speed 100 Mbps" : "speed 10 Mbps" );

#endif
#endif    	
	printk("\n" );
    }
#ifdef CONFIG_BUFFALO_PLATFORM
	kernevnt_LanAct(buffalo_struc.linkspeed,buffalo_struc.duplex_f);
#endif
}


static int restart_autoneg( int port )
{
    u32 phy_reg_val = 0;

#if defined CONFIG_BUFFALO_PLATFORM
	mvEthPhyRegRead(mvBoardPhyAddrGet(port), 9, &phy_reg_val);
	switch(buffalo_eth_mode){
	case BUFFALO_ETH_FORCE_MASTER:
		phy_reg_val &= ~(BIT11 | BIT12);
		phy_reg_val |= (BIT11 | BIT12);
		break;
	case BUFFALO_ETH_FORCE_SLAVE:
		phy_reg_val &= ~(BIT11 | BIT12);
		phy_reg_val |= (BIT12);
		break;
	case BUFFALO_ETH_FORCE_AUTO:
	default:
		phy_reg_val &= ~(BIT11 | BIT12);
		break;
	}
	mvEthPhyRegWrite(mvBoardPhyAddrGet(port), 9, phy_reg_val);
	printk("eth reg(9) val=0x%04x\n", phy_reg_val);
#endif

#if defined(CONFIG_MV_INCLUDE_UNM_ETH) /* UNIMAC */
    /* UniMAC doesn't support Gigabit, so we tell the Phy specifically */
    /* not to advertise 1000 Base-T Operation                          */
    ReadMiiWrap(mvBoardPhyAddrGet(port), ETH_PHY_1000BASE_T_CTRL_REG, &phy_reg_val);
    phy_reg_val &= ~(ETH_PHY_1000BASE_ADVERTISE_MASK);
    WriteMiiWrap(mvBoardPhyAddrGet(port), ETH_PHY_1000BASE_T_CTRL_REG, phy_reg_val);
#endif

    /* enable auto-negotiation */
    ReadMiiWrap(mvBoardPhyAddrGet(port), ETH_PHY_CTRL_REG, &phy_reg_val);
    phy_reg_val |= BIT12;
    WriteMiiWrap(mvBoardPhyAddrGet(port), ETH_PHY_CTRL_REG, phy_reg_val);

    mdelay(10);

    /* restart auto-negotiation */
    phy_reg_val |= BIT9;
    WriteMiiWrap(mvBoardPhyAddrGet(port), ETH_PHY_CTRL_REG, phy_reg_val);

    mdelay(10);

    return 0;
}

void print_iph(struct iphdr* iph)
{
    printk("**************** IP Header: ver=%d, ihl=%d ******************\n", 
            iph->version, iph->ihl);
    printk("tot_len=%d, id=0x%x, proto=%d, csum=0x%x, sip=0x%x, dip=0x%x\n",
            ntohs(iph->tot_len & 0xFFFF), ntohs(iph->id & 0xFFFF), iph->protocol & 0xFF, 
            ntohs(iph->check & 0xFFFF), ntohl(iph->saddr), ntohl(iph->daddr));
}

void print_tcph(struct tcphdr* hdr)
{
    printk("################## TCP Header: doff=%d ####################\n", hdr->doff); 
    printk("sPort=%d, dPort=%d, seqId=0x%x, ackId=0x%x, win=0x%x, csum=0x%x\n", 
            ntohs(hdr->source), ntohs(hdr->dest), ntohl(hdr->seq), ntohl(hdr->ack_seq),
            ntohs(hdr->window), ntohs(hdr->check) );
    printk("Flags: fin=%d, syn=%d, rst=%d, psh=%d, ack=%d, urg=%d, ece=%d, cwr=%d\n", 
            hdr->fin, hdr->syn, hdr->rst, hdr->psh, hdr->ack, hdr->urg, hdr->ece, hdr->cwr);
}

void print_skb(struct sk_buff* skb)
{
    int i;

    printk("\nskb=%p: head=%p, data=%p, tail=%p, end=%p\n", 
                skb, skb->head, skb->data, skb->tail, skb->end);
    printk("\t users=%d, truesize=%d, len=%d, data_len=%d, mac_len=%d\n", 
            atomic_read(&skb->users), skb->truesize, skb->len, skb->data_len, skb->mac_len);
    printk("\t next=%p, prev=%p, csum=0x%x, ip_summed=%d, pkt_type=%d, proto=0x%x, cloned=%d\n",
            skb->next, skb->prev, skb->csum, skb->ip_summed, skb->pkt_type, 
            ntohs(skb->protocol & 0xFFFF), skb->cloned);
    printk("\t mac=%p, nh=%p, h=%p\n", skb_mac_header(skb),  ip_hdr(skb), tcp_hdr(skb));
    printk("\t dataref=0x%x, nr_frags=%d, gso_size=%d, gso_segs=%d, frag_list=%p\n",
            atomic_read(&skb_shinfo(skb)->dataref), skb_shinfo(skb)->nr_frags, skb_shinfo(skb)->gso_size,
            skb_shinfo(skb)->gso_segs, skb_shinfo(skb)->frag_list);
    for(i=0; i<skb_shinfo(skb)->nr_frags; i++)
    {
        printk("\t frag_%d. page=%p, page_offset=0x%x, size=%d\n",
            i, page_address(skb_shinfo(skb)->frags[i].page), 
            skb_shinfo(skb)->frags[i].page_offset & 0xFFFF, 
            skb_shinfo(skb)->frags[i].size & 0xFFFF);
    }
    if( (skb->protocol == ntohs(ETH_P_IP)) && (ip_hdr(skb) != NULL) )
    {
        print_iph(ip_hdr(skb));
        if( ip_hdr(skb)->protocol == IPPROTO_TCP)
            print_tcph(tcp_hdr(skb));
    }
    printk("\n");
}

#ifdef CONFIG_BUFFALO_PLATFORM
void
egiga_buffalo_change_configuration(MV_U8 mode)
{
	printk("buffalo_eth_mode changed to %d\n", mode);
	buffalo_eth_mode = mode;
}

// called from buffalo/drivers/buffalocore.c
int egiga_buffalo_proc_string(char *buf)
{
	int len=0;
	MV_U16 reg;
	
	len += sprintf(buf+len,"MAC=%02X:%02X:%02X:%02X:%02X:%02X\n",
		buffalo_struc.mac[0],buffalo_struc.mac[1],buffalo_struc.mac[2],
		buffalo_struc.mac[3],buffalo_struc.mac[4],buffalo_struc.mac[5]
		);
	len += sprintf(buf+len,"link=%dMbps\n",buffalo_struc.linkspeed);
	len += sprintf(buf+len,"duplex=%s\n",buffalo_struc.duplex_f? "full":"half");
	len += sprintf(buf+len,"jumboframe=enable\n");
	len += sprintf(buf+len,"mtu=%d\n",buffalo_struc.mtu);

	mvEthPhyRegRead(mvBoardPhyAddrGet(0), 9, &reg);
#if 1
	len += sprintf(buf+len, "register9 value=0x%04x\n", reg);
#endif
	len += sprintf(buf+len, "mode.config=%s:%s\n",
		(reg & BIT12)? "manual":"auto",
		(reg & BIT11)? "master":"slave");
	mvEthPhyRegRead(mvBoardPhyAddrGet(0), 10, &reg);
	len += sprintf(buf+len, "model.actual=%s\n",
		(reg & BIT14)? "master":"slave");

	return len;
}
#endif
