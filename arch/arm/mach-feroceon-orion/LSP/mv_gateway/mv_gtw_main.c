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
#ifndef EXPORT_SYMTAB
#define EXPORT_SYMTAB
#endif

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <linux/pci.h>
#include <linux/ip.h>
#include <linux/in.h>
#include <linux/tcp.h>

#include "mvOs.h"
#include "mvDebug.h"
#include "mvSysHwConfig.h"
#include "mvEth.h"
#include "mvEthPhy.h"
#include "msApi.h"
#include "dbg-trace.h"
#ifdef INCLUDE_MULTI_QUEUE
#include "mvEthPolicy.h"
#endif

#if defined(CONFIG_MV_INCLUDE_UNM_ETH) /* UNIMAC */
#undef TX_CSUM_OFFLOAD
#undef RX_CSUM_OFFLOAD
#endif

/* RD2-5181L-FE and RD-88W8660 board/switch ports mapping */
#define FE_QD_PORT_CPU  5
#define FE_QD_PORT_0    0
#define FE_QD_PORT_1    1
#define FE_QD_PORT_2    2
#define FE_QD_PORT_3    3
#define FE_QD_PORT_4    4

/* RD2-5181L-GE board/switch ports mapping */
#define GE_QD_PORT_CPU  3
#define GE_QD_PORT_0    2
#define GE_QD_PORT_1    1
#define GE_QD_PORT_2    0
#define GE_QD_PORT_3    7
#define GE_QD_PORT_4    5

/* run time detection GE/FE */
int QD_PORT_CPU;
int QD_PORT_0;
int QD_PORT_1;
int QD_PORT_2;
int QD_PORT_3;
int QD_PORT_4;

/* use this MACRO to find if a certain port (0-7) is actually connected */
#define QD_IS_PORT_CONNECTED(p)	( ((p) == QD_PORT_CPU) || ((p) == QD_PORT_0) || \
				  ((p) == QD_PORT_1)   || ((p) == QD_PORT_2) || \
				  ((p) == QD_PORT_3)   || ((p) == QD_PORT_4)  ) ? (1) : (0)


/* helpers for VLAN tag handling */
#define MV_GTW_PORT_VLAN_ID(grp,port)  ((grp)+(port)+1)
#define MV_GTW_GROUP_VLAN_ID(grp)      (((grp)+1)<<8)
#define MV_GTW_VLANID_TO_GROUP(vlanid) ((((vlanid) & 0xf00) >> 8)-1)
#define MV_GTW_VLANID_TO_PORT(vlanid)  (((vlanid) & 0xf)-1)

/* Switch related */
#define QD_MIN_ETH_PACKET_LEN 60
#define QD_VLANTAG_SIZE 4
#define QD_HEADER_SIZE  2

/* eth related */
#define MV_MTU MV_ALIGN_UP(1500 + 2 + 4 + ETH_HLEN + 4, 32)  /* 2(HW hdr) 4(QD extra) 14(MAC hdr) 4(CRC) */
#define ETH_DEF_PORT	    0

#ifdef INCLUDE_MULTI_QUEUE
 /* RX priority (lower number = lower priority) */
 #ifdef CONFIG_MV_GTW_QOS_ROUTING
  #define MV_ROUTING_PRIO	2
 #endif
 #ifdef CONFIG_MV_GTW_QOS_VOIP
  #define MV_VOIP_PRIO		3
 #endif
 #if defined(CONFIG_MV_INCLUDE_UNM_ETH)
 /* UniMAC Interrupt Cause Register (ICR) */
#ifdef ETH_TX_DONE_ISR
 #define ETH_TXQ_MASK		(BIT3|BIT2)
#else
 #define ETH_TXQ_MASK		0
#endif /* ETH_TX_DONE_ISR */
 #define ETH_RXQ_MASK		(BIT19|BIT18|BIT17|BIT16)
 #define ETH_RXQ_RES_MASK	(BIT23|BIT22|BIT21|BIT20)
 #else
 /* Gigabit Ethernet Port Interrupt Cause and Port Interrupt Cause Extend Registers (PICR and PICER) */
#ifdef ETH_TX_DONE_ISR
 #define ETH_TXQ_MASK		(BIT0)
#else
 #define ETH_TXQ_MASK		0
#endif /* ETH_TX_DONE_ISR */
 #define ETH_RXQ_MASK		(BIT9|BIT8|BIT7|BIT6|BIT5|BIT4|BIT3|BIT2)
 #define ETH_RXQ_RES_MASK	(BIT18|BIT17|BIT16|BIT15|BIT14|BIT13|BIT12|BIT11)
 #endif
#else /* not INCLUDE_MULTI_QUEUE */
 #if defined(CONFIG_MV_INCLUDE_UNM_ETH)
 /* UniMAC Interrupt Cause Register (ICR) */
 #define ETH_TXQ_MASK		(BIT3)
 #define ETH_RXQ_MASK		(BIT16)
 #define ETH_RXQ_RES_MASK	(BIT20)
 #else
 /* Gigabit Ethernet Port Interrupt Cause and Port Interrupt Cause Extend Registers (PICR and PICER) */
 #define ETH_TXQ_MASK		(BIT0)
 #define ETH_RXQ_MASK		(BIT2)
 #define ETH_RXQ_RES_MASK	(BIT11)
 #endif
#endif /* INCLUDE_MULTI_QUEUE */

#if defined(CONFIG_MV_INCLUDE_UNM_ETH)
 /* UniMAC Interrupt Mask Register (IMR) */
 #define ETH_PICR_MASK      (ETH_RXQ_MASK|ETH_RXQ_RES_MASK)
#else
 /* Gigabit Ethernet Port Interrupt Mask and Port Interrupt Mask Extend Registers (PIMR and PIMER) */
 #define ETH_PICR_MASK      (BIT1|ETH_RXQ_MASK|ETH_RXQ_RES_MASK)
#endif

/* debug control */
#define GTW_DEBUG
#undef GTW_DEBUG
#define GTW_DBG_OFF     0x0000
#define GTW_DBG_RX      0x0001
#define GTW_DBG_TX      0x0002
#define GTW_DBG_RX_FILL 0x0004
#define GTW_DBG_TX_DONE 0x0008
#define GTW_DBG_LOAD    0x0010
#define GTW_DBG_IOCTL   0x0020
#define GTW_DBG_INT     0x0040
#define GTW_DBG_STATS   0x0080
#define GTW_DBG_MCAST   0x0100
#define GTW_DBG_VLAN    0x0200
#define GTW_DBG_IGMP    0x0400
#define GTW_DBG_MACADDR 0x0800
#define GTW_DBG_ALL     0xffff
#ifdef GTW_DEBUG
#define GTW_DBG(FLG, X) if( (mv_gtw_dbg & (FLG)) == (FLG) ) printk X
#else
#define GTW_DBG(FLG, X)
#endif
unsigned int mv_gtw_dbg = GTW_DBG_ALL;

/* gigabit statistics */
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
#define EGIGA_STAT_POLL    0x0080
#define EGIGA_STAT_TSO	   0x0100
#define EGIGA_STAT_ALL     0xffff
#ifdef EGIGA_STATISTICS
# define EGIGA_STAT(FLG, CODE) if( (mv_gtw_stat & (FLG)) == (FLG) ) CODE;
#else
# define EGIGA_STAT(FLG, CODE)
#endif
unsigned int mv_gtw_stat =  EGIGA_STAT_ALL;

/* prototypes */
struct mv_vlan_cfg {
    char name[IFNAMSIZ];
    unsigned int ports_mask;
    unsigned short vlan_grp_id;
    MV_8 *macaddr; /* e.g. "00:11:22:33:44:55" */
};

typedef struct _mv_priv {
    struct net_device *net_dev;			/* back reference to the net_device */
    struct mv_vlan_cfg *vlan_cfg;		/* reference to entry in net config table */
    struct net_device_stats net_dev_stat;	/* statistic counters */
}mv_priv;

typedef struct _eth_statistics
{
    unsigned int irq_total, irq_none;
    unsigned int poll_events, poll_complete;
    unsigned int rx_events, rx_hal_ok[MV_ETH_RX_Q_NUM],
    	rx_hal_no_resource[MV_ETH_RX_Q_NUM],rx_hal_no_more[MV_ETH_RX_Q_NUM],
    	rx_hal_error[MV_ETH_RX_Q_NUM],rx_hal_invalid_skb[MV_ETH_RX_Q_NUM],
    	rx_hal_bad_stat[MV_ETH_RX_Q_NUM],rx_netif_drop[MV_ETH_RX_Q_NUM];
    unsigned int tx_done_events,tx_done_hal_ok[MV_ETH_TX_Q_NUM],
    	tx_done_hal_invalid_skb[MV_ETH_TX_Q_NUM],tx_done_hal_bad_stat[MV_ETH_TX_Q_NUM],
    	tx_done_hal_still_tx[MV_ETH_TX_Q_NUM],tx_done_hal_no_more[MV_ETH_TX_Q_NUM],
    	tx_done_hal_unrecognize[MV_ETH_TX_Q_NUM],tx_done_max[MV_ETH_TX_Q_NUM],
    	tx_done_min[MV_ETH_TX_Q_NUM],tx_done_netif_wake[MV_ETH_TX_Q_NUM];
    unsigned int rx_fill_events[MV_ETH_RX_Q_NUM],rx_fill_alloc_skb_fail[MV_ETH_RX_Q_NUM],
    	rx_fill_hal_ok[MV_ETH_RX_Q_NUM],rx_fill_hal_full[MV_ETH_RX_Q_NUM],
    	rx_fill_hal_error[MV_ETH_RX_Q_NUM],rx_fill_timeout_events;
    unsigned int tx_events,tx_hal_ok[MV_ETH_TX_Q_NUM],tx_hal_no_resource[MV_ETH_TX_Q_NUM],
    	tx_hal_error[MV_ETH_TX_Q_NUM],tx_hal_unrecognize[MV_ETH_TX_Q_NUM],tx_netif_stop[MV_ETH_TX_Q_NUM],
    	tx_timeout;
    unsigned int tso_stats[64];
} eth_statistics;

typedef struct _eth_priv
{
    void* hal_priv;
    void* rx_policy_priv;
    unsigned int rxq_count[MV_ETH_RX_Q_NUM], txq_count[MV_ETH_TX_Q_NUM];
    MV_BUF_INFO tx_buf_info_arr[MAX_SKB_FRAGS+3];
    MV_PKT_INFO tx_pkt_info;
#ifdef EGIGA_STATISTICS
    eth_statistics eth_stat;
#endif
#if (ETH_TX_TIMER_PERIOD > 0)
    struct timer_list tx_timer;
    unsigned int tx_timer_flag;
#endif
    unsigned int rx_coal, tx_coal, rxcause, txcause;
    spinlock_t lock;
    spinlock_t mii_lock;
    unsigned int netif_stopped;
    struct timer_list rx_fill_timer;
    unsigned rx_fill_flag;
    unsigned int picr;	/* Port Interrupt Cause Register */
    unsigned int picer;	/* Port Interrupt Cause Extend Register, available in Gigabit HW only and not in UniMAC HW */
#ifdef ETH_INCLUDE_TSO
    char*   tx_extra_bufs[ETH_NUM_OF_TX_DESCR]; 
    int     tx_extra_buf_idx;
#endif /* ETH_INCLUDE_TSO */
} eth_priv; 

/* Network interfaces configuration */
#define MAX_NUM_OF_IFS 5
#define DEF_NUM_OF_IFS 2

#define MV_IF0_NAME          "eth0" /* WAN */
#define MV_IF0_VLAN_PORTS_GE ((1<<GE_QD_PORT_0))
#define MV_IF0_VLAN_PORTS_FE ((1<<FE_QD_PORT_0))

#define MV_IF1_NAME          "eth1" /* LAN */
#define MV_IF1_VLAN_PORTS_GE ((1<<GE_QD_PORT_1)|(1<<GE_QD_PORT_2)|(1<<GE_QD_PORT_3)|(1<<GE_QD_PORT_4))
#define MV_IF1_VLAN_PORTS_FE ((1<<FE_QD_PORT_1)|(1<<FE_QD_PORT_2)|(1<<FE_QD_PORT_3)|(1<<FE_QD_PORT_4))


/* globals variables */
static mv_priv mv_privs[MAX_NUM_OF_IFS];
int num_of_ifs = DEF_NUM_OF_IFS;
eth_priv eth_dev;
GT_QD_DEV qddev, *qd_dev = NULL;
static GT_SYS_CONFIG qd_cfg;
static struct net_device *main_net_dev = NULL;
struct mv_vlan_cfg net_cfg[MAX_NUM_OF_IFS];
static unsigned char zero_pad[QD_MIN_ETH_PACKET_LEN] = {0};
extern unsigned int overEthAddr;
#if defined(CONFIG_MV_INCLUDE_UNM_ETH)
extern unsigned char mvMacAddr[6];
#endif
static char boot_mac_addr[MAX_NUM_OF_IFS][32];
int eth_desc_num_per_rxq[MV_ETH_RX_Q_NUM];
int eth_desc_num_per_txq[MV_ETH_TX_Q_NUM];
unsigned int enabled_ports;
#ifdef CONFIG_MV_GTW_LINK_STATUS
static int switch_irq = -1;
struct timer_list link_timer;
#endif
int     eth_tx_done_quota = 16;


/* functions prototype */
static int __init mv_gtw_init_module(void);
static void __exit mv_gtw_exit_module(void);
static int init_switch(void);
static int init_eth_mac(void);
static int mv_gtw_start(struct net_device *dev);
static int mv_gtw_stop(struct net_device *dev);
static int mv_gtw_tx(struct sk_buff *skb, struct net_device *dev);
static unsigned int mv_gtw_tx_done(void);
static unsigned int mv_gtw_rx(unsigned int work_to_do);
void mv_gtw_rx_fill(void);
static void mv_gtw_tx_timeout(struct net_device *dev);
void mv_gtw_rx_fill_on_timeout(unsigned long data);
static inline void mv_gtw_print_rx_errors(unsigned int err, MV_U32 pkt_info_status);
#if (ETH_TX_TIMER_PERIOD > 0)
static void eth_tx_timer_callback(  unsigned long data );
#endif
static int mv_gtw_poll(struct net_device *dev, int *budget);
static irqreturn_t mv_gtw_interrupt_handler(int rq , void *dev_id , struct pt_regs *regs);
#ifdef CONFIG_MV_GTW_LINK_STATUS
static void mv_gtw_link_timer_function(unsigned long data);
static irqreturn_t mv_gtw_link_interrupt_handler(int rq, void *dev_id, struct pt_regs *regs);
#endif
GT_BOOL ReadMiiWrap(GT_QD_DEV* dev, unsigned int portNumber, unsigned int MIIReg, unsigned int* value);
GT_BOOL WriteMiiWrap(GT_QD_DEV* dev, unsigned int portNumber, unsigned int MIIReg, unsigned int data);
static int mv_gtw_set_port_based_vlan(unsigned int ports_mask);
static int mv_gtw_set_vlan_in_vtu(unsigned short vlan_id,unsigned int ports_mask);
#ifdef CONFIG_MV_GTW_QOS
static int mv_gtw_set_qos_in_switch(void);
#endif
static int mv_gtw_set_mac_addr( struct net_device *dev, void *addr );
#if (defined(CONFIG_MV_GTW_QOS) && (!defined(CONFIG_MV_INCLUDE_UNM_ETH)))
static void mv_gtw_set_mac_addr_to_eth(struct net_device *dev, int queue);
#endif
int mv_gtw_set_mac_addr_to_switch(unsigned char *mac_addr, unsigned char db, unsigned int ports_mask, unsigned char op);
static void mv_gtw_set_multicast_list(struct net_device *dev);
static struct net_device_stats* mv_gtw_get_stats(struct net_device *dev);
static struct net_device* mv_gtw_main_net_dev_get(void);
void print_mv_gtw_stat(void);
static int init_config(void);
void print_qd_port_counters(unsigned int port);
void print_qd_atu(int db);
static void mv_gtw_convert_str_to_mac(char *source , char *dest);
static unsigned int mv_gtw_str_to_hex(char ch);
static void mv_gtw_eth_mac_addr_get(int port, char *addr);
static inline void mv_gtw_align_ip_header(struct sk_buff *skb);
static inline MV_STATUS mv_gtw_update_rx_skb(MV_PKT_INFO *pkt_info, struct sk_buff *skb, unsigned int *if_num);
static inline void mv_gtw_update_tx_skb(unsigned int *switch_info, struct mv_vlan_cfg *vlan_cfg);
static inline void mv_gtw_read_interrupt_cause_regs(void);
static inline void mv_gtw_save_clear_rx_tx_cause(void);
static inline void mv_gtw_read_save_clear_tx_cause(void);
static inline void mv_gtw_read_save_clear_rx_cause(void);
static inline void mv_gtw_print_int_while_polling(void);
static inline void mv_gtw_eth_mask_interrupts(int port);
static inline void mv_gtw_unmask_interrupts(int port, MV_U32 rx_interrupts, MV_U32 tx_interrupts);
static inline void mv_gtw_clear_interrupts(int port);
void print_skb(struct sk_buff* skb);
void print_iph(struct iphdr* iph);


#ifdef CONFIG_MV_GTW_QOS_VOIP
int mv_gtw_qos_tos_quota = -1;
extern int MAX_SOFTIRQ_RESTART;
int mv_gtw_qos_tos_enable(void);
int mv_gtw_qos_tos_disable(void);
EXPORT_SYMBOL(mv_gtw_qos_tos_enable);
EXPORT_SYMBOL(mv_gtw_qos_tos_disable);
#endif

#ifdef CONFIG_MV_GTW_IGMP
extern int mv_gtw_igmp_snoop_init(void);
extern int mv_gtw_igmp_snoop_exit(void);
extern int mv_gtw_igmp_snoop_process(struct sk_buff* skb, unsigned char port, unsigned char vlan_dbnum);
#endif

module_init(mv_gtw_init_module);
module_exit(mv_gtw_exit_module);
MODULE_DESCRIPTION("Marvell Gateway Driver - www.marvell.com");
MODULE_AUTHOR("Tzachi Perelstein <tzachi@marvell.com>");
MODULE_LICENSE("GPL");

extern GT_STATUS hwReadPortReg(IN GT_QD_DEV *dev,IN GT_U8 portNum, IN GT_U8 regAddr, OUT GT_U16 *data);
static unsigned short mv_gtw_read_switch_reg(unsigned char port, unsigned char reg)
{
    unsigned short data=0;
    hwReadPortReg(qd_dev,port,reg,&data);
    return data;
}

static int mv_gtw_port2lport(int port)
{
    if(port==QD_PORT_0) return 0;
    if(port==QD_PORT_1) return 1;
    if(port==QD_PORT_2) return 2;
    if(port==QD_PORT_3) return 3;
    if(port==QD_PORT_4) return 4;
    return -1;
}

/*********************************************************************************************************/

/* Example: "mv_net_config=(eth0,00:99:88:88:99:77,0)(eth1,00:55:44:55:66:77,1:2:3:4)" */
static char *cmdline;


int mv_gtw_cmdline_config(char *s)
{
    cmdline = s;
    return 1;
}


__setup("mv_net_config=", mv_gtw_cmdline_config);


static int mv_gtw_check_open_bracket(char **p_net_config)
{
    if (**p_net_config == '(') {
        (*p_net_config)++;
	return 0;
    } 
    printk("Syntax error: could not find opening bracket\n");
    return -EINVAL;
}


static int mv_gtw_get_interface_name(char **p_net_config, int interface)
{
    int name_len = 0;
    char *name = *p_net_config;

    for (name_len = 0 ; 
	(**p_net_config != '\0') && (**p_net_config != ',') && (name_len < IFNAMSIZ-1); 
	(*p_net_config)++, name_len++);

    if (name_len > 0) {
	memcpy(net_cfg[interface].name, name, name_len);
	net_cfg[interface].name[name_len] = '\0';
	return 0;
    }
    printk("Syntax error while parsing interface name from command line\n");
    return -EINVAL;
}


static int mv_gtw_check_comma(char **p_net_config)
{
    if (**p_net_config == ',') {
        (*p_net_config)++;
	return 0;
    } 
    printk("Syntax error: could not find comma\n");
    return -EINVAL;
}


static int mv_gtw_is_digit(char ch)
{
    if( ((ch >= '0') && (ch <= '9')) ||
	((ch >= 'a') && (ch <= 'f')) ||
	((ch >= 'A') && (ch <= 'F')) )
	return 0;

    return -1;
}


static int mv_gtw_get_cmdline_mac_addr(char **p_net_config, int interface)
{
    /* the MAC address should look like: 00:99:88:88:99:77 */
    /* that is, 6 two-digit numbers, separated by :        */

    const int exact_len = 17; /* 6 times two-digits, plus 5 colons, total: 17 characters */
    int i = 0;
    int syntax_err = 0;
    char *p_mac_addr = *p_net_config;

    /* check first 15 characters in groups of 3 characters at a time */
    for (i = 0; i < exact_len-2; i+=3) {
	if ( (mv_gtw_is_digit(**p_net_config) == 0) && 
	     (mv_gtw_is_digit(*(*p_net_config+1)) == 0) &&
	     ((*(*p_net_config+2)) == ':') )
	{
	    (*p_net_config) += 3;
	}
	else {
	    syntax_err = 1;
	    break;
	}
    }

    /* two characters remaining, must be two digits */
    if ( (mv_gtw_is_digit(**p_net_config) == 0) && 
         (mv_gtw_is_digit(*(*p_net_config+1)) == 0) )
    {
	(*p_net_config) += 2;
    }
    else
	syntax_err = 1;

    if (syntax_err == 0) {
	memcpy(boot_mac_addr[interface], p_mac_addr, exact_len);
        net_cfg[interface].macaddr = boot_mac_addr[interface];
        return 0;
    }
    printk("Syntax error while parsing MAC address from command line\n");
    return -EINVAL;
}


static void mv_gtw_update_curr_port_mask(char digit, unsigned int *curr_port_mask)
{
    if (digit == '0')
	*curr_port_mask |= (1<<QD_PORT_0);
    if (digit == '1')
	*curr_port_mask |= (1<<QD_PORT_1);
    if (digit == '2')
	*curr_port_mask |= (1<<QD_PORT_2);
    if (digit == '3')
	*curr_port_mask |= (1<<QD_PORT_3);
    if (digit == '4')
	*curr_port_mask |= (1<<QD_PORT_4);
}


static int mv_gtw_get_port_mask(char **p_net_config, int interface)
{
    /* the port mask should look like this: */
    /* example 1: 0 */
    /* example 2: 1:2:3:4 */
    /* that is, one or more one-digit numbers, separated with : */
    /* we have up to MAX_NUM_OF_IFS interfaces */

    unsigned int curr_port_mask = 0, i = 0;
    int syntax_err = 0;

    for (i = 0; i < MAX_NUM_OF_IFS; i++) {
	if (mv_gtw_is_digit(**p_net_config) == 0) {
	    if (*(*p_net_config+1) == ':') {
		mv_gtw_update_curr_port_mask(**p_net_config, &curr_port_mask);	
		(*p_net_config) += 2;
	    }
	    else if (*(*p_net_config+1) == ')') { 
		mv_gtw_update_curr_port_mask(**p_net_config, &curr_port_mask);	
		(*p_net_config)++;
		break;
	    }
	    else {
		syntax_err = 1;
		break;
	    }
	}
	else {
	    syntax_err = 1;
	    break;
	}
    }

    if (syntax_err == 0) {	
	net_cfg[interface].ports_mask = curr_port_mask;
	return 0;
    }
    printk("Syntax error while parsing port mask from command line\n");
    return -EINVAL;
}


static int mv_gtw_check_closing_bracket(char **p_net_config)
{
    if (**p_net_config == ')') {
        (*p_net_config)++;
	return 0;
    } 
    printk("Syntax error: could not find closing bracket\n");
    return -EINVAL;
}


static int mv_gtw_parse_net_config(int use_cmdline)
{
    char *p_net_config = NULL;
    int i = 0;
    int status = 0;

    if (use_cmdline)
	p_net_config = cmdline;
    else
	p_net_config = CONFIG_MV_NET_CONFIG;

    if (p_net_config == NULL)
	return -EINVAL;

    num_of_ifs = 0;

    for (i = 0; (i < MAX_NUM_OF_IFS) && (*p_net_config != '\0'); i++) {
        status = mv_gtw_check_open_bracket(&p_net_config);
	if (status != 0) 
	    break;
	status = mv_gtw_get_interface_name(&p_net_config, i);
	if (status != 0) 
	    break;
	status = mv_gtw_check_comma(&p_net_config);
	if (status != 0) 
	    break;
	status = mv_gtw_get_cmdline_mac_addr(&p_net_config, i);
	if (status != 0) 
	    break;
	status = mv_gtw_check_comma(&p_net_config);
	if (status != 0) 
	    break;
	status = mv_gtw_get_port_mask(&p_net_config, i);
	if (status != 0) 
	    break;
	status = mv_gtw_check_closing_bracket(&p_net_config);
	if (status != 0) 
	    break;

	num_of_ifs++;
    }

    /* at this point, we have parsed up to MAX_NUM_OF_IFS */
    /* if the net_config string is not finished yet, then its format is invalid */
    if (*p_net_config != '\0')
	status = -EINVAL;

    return ( (status == 0) ? 0 : -1);
}


static int mv_gtw_boot_net_config(MV_U32 boardId)
{
    struct mv_vlan_cfg *nc;
    int i;
    char tmp_addr[DEF_NUM_OF_IFS][6];
    
    /* Interface eth0 gets the MAC address defined in uBoot.                                    */
    /* Interface eth1 gets the same MAC address, except for the least significant bit of the    */
    /* address, which is the opposite of the least significant bit of the given address.        */

    for(i=0, nc=net_cfg; i<DEF_NUM_OF_IFS; i++,nc++) {

	mv_gtw_eth_mac_addr_get(ETH_DEF_PORT, tmp_addr[i]);

	if(i == 0) {
	    sprintf(nc->name,MV_IF0_NAME);
            sprintf(boot_mac_addr[i], "%02x:%02x:%02x:%02x:%02x:%02x",
            	    tmp_addr[i][0],tmp_addr[i][1],tmp_addr[i][2],tmp_addr[i][3],tmp_addr[i][4],tmp_addr[i][5]);
    	    nc->macaddr = boot_mac_addr[i];
	    nc->ports_mask = (boardId == RD_88F5181L_VOIP_GE     || 
			      boardId == RD_88F5181L_VOIP_FXO_GE || 
			      boardId == RD_88F5181_GTW_GE) ? MV_IF0_VLAN_PORTS_GE : MV_IF0_VLAN_PORTS_FE;	    
	}
	else if(i == 1) {
	    sprintf(nc->name,MV_IF1_NAME);
	    tmp_addr[i][5] = (tmp_addr[i][5] & 0x1) ? (tmp_addr[i][5] & ~0x1) : (tmp_addr[i][5] | 0x1);
            sprintf(boot_mac_addr[i], "%02x:%02x:%02x:%02x:%02x:%02x",
        	    tmp_addr[i][0],tmp_addr[i][1],tmp_addr[i][2],tmp_addr[i][3],tmp_addr[i][4],tmp_addr[i][5]);
	    nc->macaddr = boot_mac_addr[i];
	    nc->ports_mask = (boardId == RD_88F5181L_VOIP_GE     || 
			      boardId == RD_88F5181L_VOIP_FXO_GE || 
			      boardId == RD_88F5181_GTW_GE) ? MV_IF1_VLAN_PORTS_GE : MV_IF1_VLAN_PORTS_FE;
	}
    }
    return 0;
}


static int mv_gtw_net_setup(MV_U32 boardId)
{
    struct mv_vlan_cfg *nc;
    int i = 0;
    int use_cmdline = 0;

    /* build the net config table */
    memset(net_cfg,0xff,sizeof(net_cfg));

    if (cmdline != NULL) {
	printk("Using command line network interface configuration\n");
	use_cmdline = 1;
        if (mv_gtw_parse_net_config(use_cmdline) != 0) {
	    printk("Error parsing mv_net_config\n");
	    return -EINVAL;
        }
    }
    else {
	if (overEthAddr) {
	    /* the user did not pass any string as the mv_net_config */
	    /* using mv_net_config from the Linux configuration file */
	    printk("Using default network interface configuration, overriding boot MAC address\n");
	    use_cmdline = 0;
	    if (mv_gtw_parse_net_config(use_cmdline) != 0) {
	        printk("Error parsing mv_net_config\n");
	        return -EINVAL;
            }
	}
	else {
	    /* the user did not pass any string as the mv_net_config */
	    /* overEthAddr is not set - using boot MAC address      */
	    printk("Using boot network interface configuration\n");
	    num_of_ifs = DEF_NUM_OF_IFS;
	    if (mv_gtw_boot_net_config(boardId) != 0)
	        return -EINVAL;
	}
    }


    enabled_ports = (1<<QD_PORT_CPU); /* CPU port should always be enabled */

    for(i=0, nc=net_cfg; i<num_of_ifs; i++,nc++) {
	/* VLAN ID */
	nc->vlan_grp_id = MV_GTW_GROUP_VLAN_ID(i);
	/* print info */
	printk("%s: mac_addr %s, VID 0x%03x, port list: ", nc->name,nc->macaddr,nc->vlan_grp_id);
	if(nc->ports_mask & (1<<QD_PORT_CPU)) printk("port-CPU ");
	if(nc->ports_mask & (1<<QD_PORT_0)) printk("port-0 ");
	if(nc->ports_mask & (1<<QD_PORT_1)) printk("port-1 ");
	if(nc->ports_mask & (1<<QD_PORT_2)) printk("port-2 ");
	if(nc->ports_mask & (1<<QD_PORT_3)) printk("port-3 ");
	if(nc->ports_mask & (1<<QD_PORT_4)) printk("port-4 ");
	printk("\n");
	/* collect per-interface port_mask into a global port_mask, used for enabling the Switch ports */
	enabled_ports |= nc->ports_mask;
    }

    return 0;
}

/*********************************************************************************************************/

static int __init mv_gtw_init_module( void ) 
{
    unsigned int i;
    struct net_device *dev = NULL;
    struct mv_vlan_cfg *nc;
    MV_U32 board_id = mvBoardIdGet();

    printk("Marvell Gateway Driver:\n");

    TRC_INIT(0,0,0,0);

    if(init_config()) {
	printk("failed to init config\n");
	return -ENODEV;
    }

    if(init_switch()) {
	printk("failed to init switch\n");
	return -ENODEV;
    }

    if(init_eth_mac()) {
	printk("failed to init ethernet device\n");
	return -ENODEV;
    }

    /* load net_devices */
    printk("loading network interfaces: ");
    for (i = 0, nc = net_cfg; i < num_of_ifs; i++, nc++) {
        dev = alloc_netdev(0, nc->name, ether_setup);
	mv_privs[i].net_dev = dev;	/* back reference to the net_device */
	mv_privs[i].vlan_cfg = nc;	/* reference to entry in net config table */
	memset(&(mv_privs[i].net_dev_stat), 0, sizeof(struct net_device_stats));/* statistic counters */
	dev->priv = &mv_privs[i];
        dev->irq = ETH_PORT0_IRQ_NUM;
	mv_gtw_convert_str_to_mac(nc->macaddr,dev->dev_addr);
	dev->open = mv_gtw_start;
        dev->stop = mv_gtw_stop;
	dev->hard_start_xmit = mv_gtw_tx;
        dev->tx_timeout = mv_gtw_tx_timeout;
	dev->watchdog_timeo = 5*HZ;
        dev->tx_queue_len = eth_desc_num_per_txq[ETH_DEF_TXQ];
	dev->set_mac_address = mv_gtw_set_mac_addr;
	dev->set_multicast_list = mv_gtw_set_multicast_list;
	dev->poll = &mv_gtw_poll;
        dev->weight = 64;
	dev->get_stats = mv_gtw_get_stats;
#ifdef TX_CSUM_OFFLOAD
        dev->features = NETIF_F_SG | NETIF_F_IP_CSUM;
#endif

#if (ETH_TX_TIMER_PERIOD > 0)
    	eth_dev.tx_timer.data = (unsigned long)dev;
#endif 

#ifdef ETH_INCLUDE_TSO
        dev->features |= NETIF_F_TSO;
#endif /* ETH_INCLUDE_TSO */

	if(register_netdev(dev)) {
	    printk(KERN_ERR "failed to register %s\n",dev->name);
	}
	else {
	    printk("%s ",dev->name);
    	}
    }
    main_net_dev = NULL;
    printk("\n");

    /* connect to MAC port interrupt line */
    if( request_irq(dev->irq, mv_gtw_interrupt_handler, (SA_INTERRUPT | SA_SAMPLE_RANDOM), "mv_gateway", NULL ) ) {
        printk(KERN_ERR "failed to assign irq%d\n", dev->irq);
        dev->irq = 0;
    }

#ifdef CONFIG_MV_GTW_LINK_STATUS
    if ( (board_id != RD_88F5181L_VOIP_FE) && 
	 (board_id != RD_88F5181_GTW_FE)   && 
	 (board_id != RD_88F5181L_VOIP_FXO_GE) ) {
        if(switch_irq != -1) {
            if(request_irq(switch_irq, mv_gtw_link_interrupt_handler, (SA_INTERRUPT|SA_SAMPLE_RANDOM), "link status", NULL)) {
	        printk(KERN_ERR "failed to assign irq%d\n", switch_irq);
	    }
        }
    }
    else {
        memset( &link_timer, 0, sizeof(struct timer_list) );
	init_timer(&link_timer);
        link_timer.function = mv_gtw_link_timer_function;
        link_timer.data = -1;
   	link_timer.expires = jiffies + (HZ); /* 1 second */ 
    	add_timer(&link_timer);
    }
#endif

    /* unmask interrupts */
    mv_gtw_unmask_interrupts(ETH_DEF_PORT, ETH_PICR_MASK, ETH_TXQ_MASK);

#ifdef CONFIG_MV_GTW_IGMP
    /* Initialize the IGMP snooping handler */
    if(mv_gtw_igmp_snoop_init()) {
        printk("failed to init IGMP snooping handler\n");
    }
#endif

    return 0;
}


static void __exit mv_gtw_exit_module(void) 
{
    printk("Removing Marvell Gateway Driver: not implemented\n");
    /* 2do: currently this module is statically linked */

#ifdef CONFIG_MV_GTW_IGMP
    /* Release the IGMP snooping handler */
    if (mv_gtw_igmp_snoop_exit())
	printk("failed to exit IGMP snooping handler\n");
#endif      
}


static struct net_device* mv_gtw_main_net_dev_get(void)
{
    int i;

    for(i=0; i<num_of_ifs; i++) {
        if(netif_running(mv_privs[i].net_dev)) {
            return mv_privs[i].net_dev;
	}
    }
    return NULL;
}


static int mv_gtw_start( struct net_device *dev ) 
{
    mv_priv *mvp = (mv_priv *)dev->priv;
    struct mv_vlan_cfg *vlan_cfg = mvp->vlan_cfg;
    unsigned char broadcast[6] = {0xff,0xff,0xff,0xff,0xff,0xff};

    printk("mv_gateway: starting %s\n",dev->name);

    /* start upper layer */
    netif_carrier_on(dev);
    netif_wake_queue(dev);
    netif_poll_enable(dev);

    main_net_dev = mv_gtw_main_net_dev_get();

    /* Add our MAC addr to the VLAN DB at switch level to forward packets with this DA */
    /* to CPU port by using the tunneling feature. The device is always in promisc mode.      */
    mv_gtw_set_mac_addr_to_switch(dev->dev_addr,MV_GTW_VLANID_TO_GROUP(vlan_cfg->vlan_grp_id),(1<<QD_PORT_CPU),1);

    /* We also need to allow L2 broadcasts comming up for this interface */
    mv_gtw_set_mac_addr_to_switch(broadcast,MV_GTW_VLANID_TO_GROUP(vlan_cfg->vlan_grp_id),vlan_cfg->ports_mask|(1<<QD_PORT_CPU),1);

#if (defined(CONFIG_MV_GTW_QOS) && (!defined(CONFIG_MV_INCLUDE_UNM_ETH)))
    mv_gtw_set_mac_addr_to_eth(dev, ETH_DEF_RXQ);
#endif

#if (ETH_TX_TIMER_PERIOD > 0)
    if(eth_dev.tx_timer_flag == 0)
    {
        eth_dev.tx_timer.expires = jiffies + ((HZ*ETH_TX_TIMER_PERIOD)/1000); /*ms*/
        add_timer( &eth_dev.tx_timer );
        eth_dev.tx_timer_flag = 1;
    }
#endif /* ETH_TX_TIMER_PERIOD > 0 */

    return 0;
}


static int mv_gtw_stop( struct net_device *dev )
{
    mv_priv *mvp = (mv_priv *)dev->priv;
    struct mv_vlan_cfg *vlan_cfg = mvp->vlan_cfg;
    unsigned char broadcast[6] = {0xff,0xff,0xff,0xff,0xff,0xff};

    printk("mv_gateway: stopping %s\n",dev->name);

#if (ETH_TX_TIMER_PERIOD > 0)
    eth_dev.tx_timer_flag = 0;
    del_timer(&eth_dev.tx_timer);
#endif
    /* stop upper layer */
    netif_poll_disable(dev);
    netif_carrier_off(dev);
    netif_stop_queue(dev);

    /* stop switch from forwarding packets from this VLAN toward CPU port */
    if( gfdbFlushInDB(qd_dev, GT_FLUSH_ALL, MV_GTW_VLANID_TO_GROUP(vlan_cfg->vlan_grp_id)) != GT_OK) {
        printk("gfdbFlushInDB failed\n");
    }

#if (defined(CONFIG_MV_GTW_QOS) && (!defined(CONFIG_MV_INCLUDE_UNM_ETH)))
    mv_gtw_set_mac_addr_to_eth(dev, -1);
#endif
    main_net_dev = mv_gtw_main_net_dev_get();

    return 0;
}


static void mv_gtw_stop_netif(void)
{
    int i;
#ifdef EGIGA_STATISTICS
    int queue = ETH_DEF_TXQ;
#endif
    unsigned long flags;
    local_irq_save(flags);
    GTW_DBG( GTW_DBG_TX | GTW_DBG_TX_DONE, ("mv_gateway: stopping netif transmission\n"));
    EGIGA_STAT( EGIGA_STAT_TX, (eth_dev.eth_stat.tx_netif_stop[queue]++) );
    for(i=0; i<num_of_ifs; i++)
	netif_stop_queue(mv_privs[i].net_dev);
    eth_dev.netif_stopped = 1;
    local_irq_restore(flags);
}


static void mv_gtw_wakeup_netif(void)
{
    int i;
#ifdef EGIGA_STATISTICS
    int queue = ETH_DEF_TXQ;
#endif
    unsigned long flags;
    local_irq_save(flags);
    GTW_DBG( GTW_DBG_TX | GTW_DBG_TX_DONE, ("mv_gateway: waking up netif transmission\n"));
    EGIGA_STAT( EGIGA_STAT_TX_DONE, (eth_dev.eth_stat.tx_done_netif_wake[queue]++) );
    for(i=0; i<num_of_ifs; i++)
	if((netif_queue_stopped(mv_privs[i].net_dev)) && (mv_privs[i].net_dev->flags & IFF_UP))
	    netif_wake_queue(mv_privs[i].net_dev);
    eth_dev.netif_stopped = 0;
    local_irq_restore(flags);
}


static int init_config(void)
{
    int i;
    MV_U32 board_id = mvBoardIdGet();
    
    /* switch ports on-board mapping */
    switch(board_id) {
	case RD_88F5181L_VOIP_FXO_GE:
	case RD_88F5181L_VOIP_GE:
	case RD_88F5181_GTW_GE:
	    QD_PORT_CPU = GE_QD_PORT_CPU;
	    QD_PORT_0   = GE_QD_PORT_0;
	    QD_PORT_1   = GE_QD_PORT_1;
	    QD_PORT_2   = GE_QD_PORT_2;
	    QD_PORT_3   = GE_QD_PORT_3;
	    QD_PORT_4   = GE_QD_PORT_4;
	    break;
	case RD_88F5181L_VOIP_FE:
	case RD_88F5181_GTW_FE:
	case RD_88W8660_DDR1:
	case RD_88W8660_AP82S_DDR1:
	    QD_PORT_CPU = FE_QD_PORT_CPU;
	    QD_PORT_0   = FE_QD_PORT_0;
	    QD_PORT_1   = FE_QD_PORT_1;
	    QD_PORT_2   = FE_QD_PORT_2;
	    QD_PORT_3   = FE_QD_PORT_3;
	    QD_PORT_4   = FE_QD_PORT_4;
	    break;
	default:
	    printk("Unsupported platform.\n");
	    return -1;
    }

    /* print board detection: */
    switch(board_id) {
	case RD_88F5181L_VOIP_GE:
	    printk("Detected RD_88F5181L_VOIP_GE\n");
	    break;
	case RD_88F5181_GTW_GE:
	    printk("Detected RD_88F5181_GTW_GE\n");
	    break;
	case RD_88F5181L_VOIP_FE:
	    printk("Detected RD_88F5181L_VOIP_FE\n");
	    break;
	case RD_88F5181_GTW_FE:
	    printk("Detected RD_88F5181_GTW_FE\n");
	    break;
	case RD_88W8660_DDR1:
	    printk("Detected RD_88W8660_DDR1\n");
	    break;
	case RD_88W8660_AP82S_DDR1:
	    printk("Detected RD_88W8660_AP82S_DDR1\n");
	    break;
	case RD_88F5181L_VOIP_FXO_GE:
	    printk("Detected RD_88F5181L_VOIP_FXO_GE\n");
	    break;

	default:
	    printk("Unsupported platform.\n");
	    return -1;
    }
	
#if defined(CONFIG_MV_GTW_HEADER_MODE)
	printk("Using Marvell Header Mode\n");
#else
	printk("Using VLAN-tag Mode\n");
#endif

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
    printk("TCP segmentation offload enabled\n");
#endif /* ETH_INCLUDE_TSO */

#if defined(TX_CSUM_OFFLOAD) && defined(RX_CSUM_OFFLOAD)
    printk("L3/L4 Checksum offload enabled\n");
#else
#if defined(RX_CSUM_OFFLOAD)
    printk("L3/L4 Receive checksum offload enabled\n");
#endif
#if defined(TX_CSUM_OFFLOAD)
    printk("L3/L4 Transmit checksum offload enabled\n");
#endif
#endif

#ifdef CONFIG_MV_GTW_LINK_STATUS
    switch(board_id) {
	case RD_88F5181L_VOIP_GE:
	case RD_88F5181_GTW_GE:
	    switch_irq = 32+8; /* MPP8 */
	    break;
	case RD_88F5181L_VOIP_FE:
	case RD_88F5181_GTW_FE:
	case RD_88W8660_DDR1:
	case RD_88W8660_AP82S_DDR1:
	case RD_88F5181L_VOIP_FXO_GE: 
	    switch_irq = 32+9; /* MPP9 */
	    break;
	default:
	    switch_irq = -1;
    }
    if(switch_irq != -1)
	printk("Link status indication mode is on (switch irq=%d)\n",switch_irq);
#endif

    for(i=0;i<MV_ETH_RX_Q_NUM;i++) {
	if (i == ETH_DEF_RXQ)
            eth_desc_num_per_rxq[i] = (2*ETH_NUM_OF_RX_DESCR);
	else
	    eth_desc_num_per_rxq[i] = ETH_NUM_OF_RX_DESCR;
    }
    for(i=0;i<MV_ETH_TX_Q_NUM;i++) {
        eth_desc_num_per_txq[i] = ETH_NUM_OF_TX_DESCR; /* should be large enough to enhance ipsec support */
    }

#ifdef INCLUDE_MULTI_QUEUE
    printk("Multi queue support ( ");
    for(i=0;i<MV_ETH_RX_Q_NUM;i++)
	printk("rxq%d=%d ",i,eth_desc_num_per_rxq[i]);
    for(i=0;i<MV_ETH_TX_Q_NUM;i++)
	printk("txq%d=%d ",i,eth_desc_num_per_txq[i]);
    printk(")\n");
#endif

#ifdef CONFIG_MV_GTW_QOS_VOIP
    printk("VoIP QoS support (ToS %s)\n", CONFIG_MV_GTW_QOS_VOIP_TOS);
#endif

#ifdef CONFIG_MV_GTW_QOS_ROUTING
    printk("Routing QoS support (ToS %s)\n", CONFIG_MV_GTW_QOS_ROUTING_TOS);
#endif

#ifdef CONFIG_MV_GTW_IGMP
    printk("L2 IGMP support\n");
#endif

#ifdef EGIGA_STATISTICS
    printk("Statistics enabled\n");
#endif

#ifdef MV_GTW_DEBUG
    printk("Debug messages enabled\n");
#endif

#ifdef CONFIG_MV_GTW_PROC
    printk("Proc tool API enabled\n");
#endif

    /* build the net config table */
    if (mv_gtw_net_setup(board_id) != 0)
	return -1;
      
    return 0;
}


static int init_switch(void)
{
    unsigned int i, p;
    unsigned char cnt;
    GT_LPORT port_list[MAX_SWITCH_PORTS];
    struct mv_vlan_cfg *nc;
    MV_U32 board_id = mvBoardIdGet();

    printk("init switch layer... ");

    memset((char*)&qd_cfg,0,sizeof(GT_SYS_CONFIG));

    /* init config structure for qd package */
    qd_cfg.BSPFunctions.readMii   = ReadMiiWrap;
    qd_cfg.BSPFunctions.writeMii  = WriteMiiWrap;
    qd_cfg.BSPFunctions.semCreate = NULL;
    qd_cfg.BSPFunctions.semDelete = NULL;
    qd_cfg.BSPFunctions.semTake   = NULL;
    qd_cfg.BSPFunctions.semGive   = NULL;
    qd_cfg.initPorts = GT_TRUE;
    qd_cfg.cpuPortNum = QD_PORT_CPU;
    if(board_id == RD_88F5181L_VOIP_GE || board_id == RD_88F5181L_VOIP_FXO_GE || board_id == RD_88F5181_GTW_GE) {
        qd_cfg.mode.scanMode = SMI_MANUAL_MODE;
    }

    /* load switch sw package */
    if( qdLoadDriver(&qd_cfg, &qddev) != GT_OK) {
	printk("qdLoadDriver failed\n");
        return -1;
    }
    qd_dev = &qddev;

    GTW_DBG( GTW_DBG_LOAD, ("qd_dev->numOfPorts=%d\n",qd_dev->numOfPorts));

    /* clear the zero pad buffer */
    memset(zero_pad,0,QD_MIN_ETH_PACKET_LEN);

    /* disable all ports */
    for(p=0; p<qd_dev->numOfPorts; p++) {
	gstpSetPortState(qd_dev, p, GT_PORT_DISABLE);
    }

    /* special board settings */
    switch(board_id) {

	case RD_88F5181L_VOIP_GE:
	case RD_88F5181_GTW_GE:
	case RD_88F5181L_VOIP_FXO_GE:
	    /* check switch id first */
	    if((qd_dev->deviceId != GT_88E6131) && (qd_dev->deviceId != GT_88E6108))
		printk("Unsupported switch id 0x%x. Trying anyway...\n",qd_dev->deviceId);
	    /* enable external ports */
	    GTW_DBG( GTW_DBG_LOAD, ("enable phy polling for external ports\n"));
	    if(gsysSetPPUEn(qd_dev, GT_TRUE) != GT_OK) {
		printk("gsysSetPPUEn failed\n");
		return -1;
	    }
		
	    /* Note: GbE does not support Marvell Header Mode, always using VLAN tags */
	    
	    /* set cpu-port with ingress double-tag mode */
	    GTW_DBG( GTW_DBG_LOAD, ("cpu port ingress double-tag mode\n"));
	    if(gprtSetDoubleTag(qd_dev, QD_PORT_CPU, GT_TRUE) != GT_OK) {
	        printk("gprtSetDoubleTag failed\n");
	        return -1;
	    }
	    /* set cpu-port with egrees add-tag mode */
	    GTW_DBG( GTW_DBG_LOAD, ("cpu port egrees add-tag mode\n"));
	    if(gprtSetEgressMode(qd_dev, QD_PORT_CPU, GT_ADD_TAG) != GT_OK) {
		printk("gprtSetEgressMode failed\n");
		return -1;
	    }
	    /* config the switch to use the double tag data (relevant to cpu-port only) */
	    GTW_DBG( GTW_DBG_LOAD, ("use double-tag and remove\n"));
	    if(gsysSetUseDoubleTagData(qd_dev,GT_TRUE) != GT_OK) {
	        printk("gsysSetUseDoubleTagData failed\n");
	        return -1;
	    }
	    /* set cpu-port with 802.1q secured mode */
	    GTW_DBG( GTW_DBG_LOAD, ("cpu port-based 802.1q secure mode\n"));
	    if(gvlnSetPortVlanDot1qMode(qd_dev,QD_PORT_CPU,GT_SECURE) != GT_OK) {
		printk("gvlnSetPortVlanDot1qMode failed\n"); 
	        return -1;
	    }
	    break;

	case RD_88F5181L_VOIP_FE:
	case RD_88F5181_GTW_FE:
	case RD_88W8660_DDR1:
	case RD_88W8660_AP82S_DDR1: 
	    /* check switch id first */
	    if((qd_dev->deviceId != GT_88E6065) && (qd_dev->deviceId != GT_88E6061))
		printk("Unsupported switch id 0x%x. Trying anyway...\n",qd_dev->deviceId);
	    /* set CPU port number */
            if(gsysSetCPUPort(qd_dev, QD_PORT_CPU) != GT_OK) {
	        printk("gsysSetCPUPort failed\n");
	        return -1;
	    }

	    if(qd_dev->deviceId == GT_88E6065) {
#ifndef CONFIG_MV_GTW_HEADER_MODE
		/* Set Provider tag id = 0x8100 to support ingress double tagging */
		if(gprtSetProviderTag(qd_dev, QD_PORT_CPU, 0x8100) != GT_OK) {
		    printk("gprtSetProviderTag failed\n");
		    return -1;
		}
		/* set cpu-port with egrees add-tag mode */
		GTW_DBG( GTW_DBG_LOAD, ("cpu port egrees add-tag mode\n"));
		if(gprtSetEgressMode(qd_dev, QD_PORT_CPU, GT_ADD_TAG) != GT_OK) {
		    printk("gprtSetEgressMode add-tag failed\n");
		    return -1;
		}
	        /* set cpu-port with 802.1q secured mode */
	        GTW_DBG( GTW_DBG_LOAD, ("cpu port-based 802.1q secure mode\n"));
	        if(gvlnSetPortVlanDot1qMode(qd_dev,QD_PORT_CPU,GT_SECURE) != GT_OK) {
	            printk("gvlnSetPortVlanDot1qMode failed\n"); 
	            return -1;
	        }
#endif
		if(gstatsFlushAll(qd_dev) != GT_OK)
		    printk("gstatsFlushAll failed\n");
	    }
	    else if(qd_dev->deviceId == GT_88E6061) {
#ifndef CONFIG_MV_GTW_HEADER_MODE
		/* set cpu-port with egrees single tag mode (no support for double tagging) */
		GTW_DBG( GTW_DBG_LOAD, ("cpu port egrees single tag mode\n"));
		if(gprtSetEgressMode(qd_dev, QD_PORT_CPU, GT_TAGGED_EGRESS) != GT_OK) {
		    printk("gprtSetEgressMode single-tag failed\n");
		    return -1;
		}
		/* set all other ports to remove the vlan tag on egress */
		for(i=0; i<qd_dev->numOfPorts; i++) {
		    if(i != QD_PORT_CPU) {
			GTW_DBG( GTW_DBG_LOAD, ("port %d remove tag on egrees\n",i));
			if(gprtSetEgressMode(qd_dev, i, GT_UNTAGGED_EGRESS) != GT_OK) {
			    printk("gprtSetEgressMode remove-tag failed\n");
			    return -1;
			}
		    }
		}
	        /* set cpu-port with 802.1q secured mode */
	        GTW_DBG( GTW_DBG_LOAD, ("cpu port-based 802.1q secure mode\n"));
	        if(gvlnSetPortVlanDot1qMode(qd_dev,QD_PORT_CPU,GT_SECURE) != GT_OK) {
	            printk("gvlnSetPortVlanDot1qMode secure mode failed\n"); 
	            return -1;
	        }
		/* set all other ports with 802.1q fall back mode */
		for(i=0; i<qd_dev->numOfPorts; i++) {
		    if(i != QD_PORT_CPU) {
			if(gvlnSetPortVlanDot1qMode(qd_dev,i,GT_FALLBACK) != GT_OK) {
			    printk("gvlnSetPortVlanDot1qMode fall back failed\n"); 
			    return -1;
			}
		    }
		}
#endif
	    }
#ifdef CONFIG_MV_GTW_HEADER_MODE
	    /* set all ports not to unmodify the vlan tag on egress */
	    for(i=0; i<qd_dev->numOfPorts; i++) {
		if(gprtSetEgressMode(qd_dev, i, GT_UNMODIFY_EGRESS) != GT_OK) {
		    printk("gprtSetEgressMode GT_UNMODIFY_EGRESS failed\n"); 
		    return -1;
		}
	    }
	    if(gprtSetHeaderMode(qd_dev,QD_PORT_CPU,GT_TRUE) != GT_OK) {
		printk("gprtSetHeaderMode GT_TRUE failed\n");
		return -1;
	    }
#endif
	    /* init counters */
	    if(gprtClearAllCtr(qd_dev) != GT_OK)
	        printk("gprtClearAllCtr failed\n"); 
	    if(gprtSetCtrMode(qd_dev, GT_CTR_ALL) != GT_OK)
	        printk("gprtSetCtrMode failed\n"); 
	    break;

	default:
	    printk("Error: unsupported board id 0x%x\n",board_id);
	    return -1;
    }
     
    /* set priorities rules */
    for(i=0; i<qd_dev->numOfPorts; i++) {
        /* default port priority to queue zero */
	if(gcosSetPortDefaultTc(qd_dev, i, 0) != GT_OK)
	    printk("gcosSetPortDefaultTc failed (port %d)\n", i);
        /* enable IP TOS Prio */
	if(gqosIpPrioMapEn(qd_dev, i, GT_TRUE) != GT_OK)
	    printk("gqosIpPrioMapEn failed (port %d)\n",i); 
	/* set IP QoS */
	if(gqosSetPrioMapRule(qd_dev, i, GT_FALSE) != GT_OK)
	    printk("gqosSetPrioMapRule failed (port %d)\n",i); 
        /* disable Vlan QoS Prio */
	if(gqosUserPrioMapEn(qd_dev, i, GT_FALSE) != GT_OK)
	    printk("gqosUserPrioMapEn failed (port %d)\n",i); 
        /* Set force flow control to FALSE for all ports */
	if(gprtSetForceFc(qd_dev, i, GT_FALSE) != GT_OK)
	    printk("gprtSetForceFc failed (port %d)\n",i); 
    }
  
    /* The switch CPU port is not part of the VLAN, but rather connected by tunneling to each */
    /* of the VLAN's ports. Our MAC addr will be added during start operation to the VLAN DB  */
    /* at switch level to forward packets with this DA to CPU port.                           */
    GTW_DBG( GTW_DBG_LOAD, ("Enabling Tunneling on ports: "));
    for(i=0; i<qd_dev->numOfPorts; i++) {
	if(i != QD_PORT_CPU) {
	    if(gprtSetVlanTunnel(qd_dev, i, GT_TRUE) != GT_OK) {
		printk("gprtSetVlanTunnel failed (port %d)\n",i); 
		return -1;
	    }
	    else {
		GTW_DBG( GTW_DBG_LOAD, ("%d ",i));
	    }
	}
    }
    GTW_DBG( GTW_DBG_LOAD, ("\n"));

    /* configure ports (excluding CPU port) for each network interface (VLAN): */
    for(i=0, nc=net_cfg; i<num_of_ifs; i++,nc++) {
	GTW_DBG( GTW_DBG_LOAD, ("%s configuration\n", nc->name));

	/* set port's defaul private vlan id and database number (DB per group): */
	for(p=0; p<qd_dev->numOfPorts; p++) {
	    if( MV_BIT_CHECK(nc->ports_mask, p) && (p != QD_PORT_CPU) ) {
		GTW_DBG( GTW_DBG_LOAD, ("port %d default private vlan id: 0x%x\n", p, MV_GTW_PORT_VLAN_ID(nc->vlan_grp_id,p)));
		if( gvlnSetPortVid(qd_dev, p, MV_GTW_PORT_VLAN_ID(nc->vlan_grp_id,p)) != GT_OK ) {
			printk("gvlnSetPortVid failed"); 
			return -1;
		}
		if( gvlnSetPortVlanDBNum(qd_dev, p, MV_GTW_VLANID_TO_GROUP(nc->vlan_grp_id)) != GT_OK) {
		    printk("gvlnSetPortVlanDBNum failed\n");
		    return -1;
		}
	    }
	}

	/* set port's port-based vlan (CPU port is not part of VLAN) */
        if(mv_gtw_set_port_based_vlan(nc->ports_mask & ~(1<<QD_PORT_CPU)) != 0) {
	    printk("mv_gtw_set_port_based_vlan failed\n");
	}

        /* set vtu with group vlan id (used in tx) */
        if(mv_gtw_set_vlan_in_vtu(nc->vlan_grp_id,nc->ports_mask|(1<<QD_PORT_CPU)) != 0) {
	    printk("mv_gtw_set_vlan_in_vtu failed\n");
	}

        /* set vtu with each port private vlan id (used in rx) */
 	for(p=0; p<qd_dev->numOfPorts; p++) {
	    if(MV_BIT_CHECK(nc->ports_mask, p) && (p!=QD_PORT_CPU)) {
		if(mv_gtw_set_vlan_in_vtu(MV_GTW_PORT_VLAN_ID(nc->vlan_grp_id,p),nc->ports_mask & ~(1<<QD_PORT_CPU)) != 0) {
		    printk("mv_gtw_set_vlan_in_vtu failed\n");
		}
	    }
	}
    }

    /* set cpu-port with port-based vlan to all other ports */
    GTW_DBG( GTW_DBG_LOAD, ("cpu port-based vlan:"));
    for(p=0,cnt=0; p<qd_dev->numOfPorts; p++) {
        if(p != QD_PORT_CPU) {
	    GTW_DBG( GTW_DBG_LOAD, ("%d ",p));
            port_list[cnt] = p;
            cnt++;
        }
    }
    GTW_DBG( GTW_DBG_LOAD, ("\n"));
    if( gvlnSetPortVlanPorts(qd_dev, QD_PORT_CPU, port_list, cnt) != GT_OK) {
        printk("gvlnSetPortVlanPorts failed\n"); 
        return -1;
    }

#ifdef CONFIG_ETH_FLOW_CONTROL
    /* Force flow control on CPU-port */
    if(gprtSetForceFc(qd_dev, QD_PORT_CPU, GT_TRUE) != GT_OK)
        printk("gprtSetForceFc failed (port %d)\n", QD_PORT_CPU); 
#endif

#ifdef CONFIG_MV_GTW_QOS
    mv_gtw_set_qos_in_switch();
#endif

    if(gfdbFlush(qd_dev,GT_FLUSH_ALL) != GT_OK) {
	printk("gfdbFlush failed\n");
    }

#if 0
    /* rate limit simulation - 32M egress on WAN port */
    if(grcSetEgressRate(qd_dev,QD_PORT_0,GT_32M) != GT_OK)
	printk("mv_gateway failed to simulate 32M upstream on WAN port\n");
    /* Example - Force flow control on WAN-port */
    if(gprtSetForceFc(qd_dev, QD_PORT_0, GT_TRUE) != GT_OK)
        printk("gprtSetForceFc failed (port %d)\n", QD_PORT_0); 
#endif

    /* done! enable all Switch ports according to the net config table */
    GTW_DBG( GTW_DBG_LOAD, ("enabling: ports "));
    for(p=0; p<qd_dev->numOfPorts; p++) {
	if (MV_BIT_CHECK(enabled_ports, p)) {
	    GTW_DBG( GTW_DBG_LOAD, ("%d ",p));
	    if(gstpSetPortState(qd_dev, p, GT_PORT_FORWARDING) != GT_OK) {
	        printk("gstpSetPortState failed\n");
	    }
	}
    }

    GTW_DBG( GTW_DBG_LOAD, ("\n"));

#ifdef CONFIG_MV_GTW_LINK_STATUS
    /* Enable Phy Link Status Changed interrupt at Phy level for the all enabled ports */
    for(p=0; p<qd_dev->numOfPorts; p++) {
	if(MV_BIT_CHECK(enabled_ports, p) && (p != QD_PORT_CPU)) {
	    if(gprtPhyIntEnable(qd_dev, p, (GT_LINK_STATUS_CHANGED)) != GT_OK) {
		printk("gprtPhyIntEnable failed port %d\n", p);
	    }
	}
    }
    if ( (board_id != RD_88F5181L_VOIP_FE) && (board_id != RD_88F5181_GTW_FE) && 
	 (board_id != RD_88F5181L_VOIP_FXO_GE) ) {
        if(eventSetActive(qd_dev, GT_PHY_INTERRUPT) != GT_OK) {
	    printk("eventSetActive failed\n");
        }
    }
 #endif

    if ( (board_id == RD_88F5181L_VOIP_GE) || (board_id == RD_88F5181_GTW_GE) || board_id == RD_88F5181L_VOIP_FXO_GE) { 
        /* config LEDs: Bi-Color Mode-4:  */
        /* 1000 Mbps Link - Solid Green; 1000 Mbps Activity - Blinking Green */
        /* 100 Mbps Link - Solid Red; 100 Mbps Activity - Blinking Red */
        for(p=0; p<qd_dev->numOfPorts; p++) {
	    if( (p != QD_PORT_CPU) && (QD_IS_PORT_CONNECTED(p)) ) { 
	        if(gprtSetPagedPhyReg(qd_dev,p,16,3,0x888F)) { /* Configure Register 16 page 3 to 0x888F for mode 4 */
		    printk("gprtSetPagedPhyReg failed (port=%d)\n", p);
	        }
		if(gprtSetPagedPhyReg(qd_dev,p,17,3,0x4400)) { /* Configure Register 17 page 3 to 0x4400 50% mixed LEDs */
		    printk("gprtSetPagedPhyReg failed (port=%d)\n", p);
	        }

	    }
        }
    }
    else {
	for(p=0; p<qd_dev->numOfPorts; p++) {
	    if( (p != QD_PORT_CPU) && (QD_IS_PORT_CONNECTED(p)) ) {
	        if(gprtSetPhyReg(qd_dev,p,22,0x1FFA)) { /* Configure Register 22 LED0 to 0xA for Link/Act */
	    	    printk("gprtSetPhyReg failed (port=%d)\n", p);
		}
	    }
	}
    }
    
#if 0
for(i=0; i<qd_dev->numOfPorts; i++) {
    printk("port %d reg 0x4: 0x%x\n",i,mv_gtw_read_switch_reg(i, 0x4));
    printk("port %d reg 0x6: 0x%x\n",i,mv_gtw_read_switch_reg(i, 0x6));
    printk("port %d reg 0x8: 0x%x\n",i,mv_gtw_read_switch_reg(i, 0x8));
}
#endif

    printk("done\n");

    return 0;
}

#ifdef CONFIG_MV_GTW_QOS
static int mv_gtw_set_qos_in_switch(void)
{
    /* The ToS value must be represented as follow: "0xVV;0xYY..."        */
    unsigned char tos = 0;
    char *str;
    int i;

    /* Clear all */
    for(i=0; i<=0x40; i++) {
        if(gcosSetDscp2Tc(qd_dev, i, 0) != GT_OK) {
	    printk("gcosSetDscp2Tc failed\n");
	}
    }

#ifdef CONFIG_MV_GTW_QOS_VOIP
    /* VoIP support only one ToS value */
    str = CONFIG_MV_GTW_QOS_VOIP_TOS;
    tos = (mv_gtw_str_to_hex(str[2])<<4) + (mv_gtw_str_to_hex(str[3]));
    if(gcosSetDscp2Tc(qd_dev, tos>>2, MV_VOIP_PRIO) != GT_OK) {
	printk("gcosSetDscp2Tc failed\n");
    }
    GTW_DBG( GTW_DBG_LOAD, ("VoIP ToS 0x%x goes to queue %d\n",tos,MV_VOIP_PRIO));
#endif
#ifdef CONFIG_MV_GTW_QOS_ROUTING
    /* Routing supports multiple ToS values */
    for(i=0;;i+=5) {
	str = CONFIG_MV_GTW_QOS_ROUTING_TOS + i;
	tos = (mv_gtw_str_to_hex(str[2])<<4) + (mv_gtw_str_to_hex(str[3]));
	if(gcosSetDscp2Tc(qd_dev, tos>>2, MV_ROUTING_PRIO) != GT_OK) {
	   printk("gcosSetDscp2Tc failed\n");
	}
	GTW_DBG( GTW_DBG_LOAD, ("Routing ToS 0x%x goes to queue %d\n",tos,MV_ROUTING_PRIO));
	if(str[4] != ';')
	    break;
    }
#endif
}
#endif /* QOS */

#ifdef ETH_INCLUDE_TSO
/*********************************************************** 
 * eth_tso_tx --                                             *
 *   send a packet.                                        *
 ***********************************************************/
static int eth_tso_tx( struct sk_buff *skb , struct net_device *dev )
{
    mv_priv *mvp = dev->priv;
    struct net_device_stats *stats = &(mvp->net_dev_stat);
    MV_STATUS       status;
    int             pkt, frag, buf;
    int             total_len, hdr_len, mac_hdr_len, size, frag_size, data_left;
    char            *frag_ptr, *extra_ptr;
    MV_U16          ip_id;
    MV_U32          tcp_seq;
    struct iphdr    *iph;
    struct tcphdr   *tcph;
    skb_frag_t      *skb_frag_ptr;
    int             queue;
    unsigned int switch_info;

    pkt = 0;        
    frag = 0;
    total_len = skb->len;
    hdr_len = ((skb->h.raw - skb->data) + (skb->h.th->doff << 2));
    mac_hdr_len = skb->nh.raw - skb->data;

    eth_dev.tx_pkt_info.pFrags = &eth_dev.tx_buf_info_arr[1];
    total_len -= hdr_len;

    if(skb_shinfo(skb)->tso_segs == 1)
    {
        printk("Only one TSO segs\n");
        print_skb(skb);
    }

    if(total_len <= skb_shinfo(skb)->tso_size)
    {
        printk("***** total_len less than tso_size\n");
        print_skb(skb);
    }
    if( (htons(ETH_P_IP) != skb->protocol) || 
        (skb->nh.iph->protocol != IPPROTO_TCP) )
    {
        printk("***** ERROR: Unexpected protocol\n");
        print_skb(skb);
    }

    ip_id = ntohs(skb->nh.iph->id);
    tcp_seq = ntohl(skb->h.th->seq);

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
        extra_ptr = eth_dev.tx_extra_bufs[eth_dev.tx_extra_buf_idx++];
        if(eth_dev.tx_extra_buf_idx == ETH_NUM_OF_TX_DESCR)
            eth_dev.tx_extra_buf_idx = 0;

        extra_ptr += 2;
        memcpy(extra_ptr, skb->data, hdr_len);

        eth_dev.tx_pkt_info.pFrags[0].bufVirtPtr = extra_ptr;
        eth_dev.tx_pkt_info.pFrags[0].bufSize = hdr_len;

        data_left = MV_MIN(skb_shinfo(skb)->tso_size, total_len);
        eth_dev.tx_pkt_info.pktSize = data_left + hdr_len;
        total_len -= data_left;

        /* Update fields */
        iph = (struct iphdr*)(extra_ptr + mac_hdr_len);
        iph->tot_len = htons(data_left + hdr_len - mac_hdr_len);
        iph->id = htons(ip_id);

        tcph = (struct tcphdr*)(extra_ptr + (skb->h.raw - skb->data));
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
            eth_dev.tx_pkt_info.osInfo = (MV_ULONG)skb;
        }
        else
        {
            /* Clear all special flags for not last packet */
            tcph->psh = 0;
            tcph->fin = 0;
            tcph->rst = 0;
            eth_dev.tx_pkt_info.osInfo = (MV_ULONG)0;
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
            eth_dev.tx_pkt_info.pFrags[buf].bufVirtPtr = frag_ptr;
            eth_dev.tx_pkt_info.pFrags[buf].bufSize = size;
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
        eth_dev.tx_pkt_info.numFrags = buf;
        eth_dev.tx_pkt_info.status =  
                (ETH_TX_IP_NO_FRAG | ETH_TX_VLAN_TAGGED_FRAME_MASK | ETH_TX_L4_TCP_TYPE |
                 ETH_TX_GENERATE_L4_CHKSUM_MASK | ETH_TX_GENERATE_IP_CHKSUM_MASK |
                 ((skb->nh.iph->ihl) << ETH_TX_IP_HEADER_LEN_OFFSET) );

        queue = ETH_DEF_TXQ;
#if 0
	/* is this necessary ? */
    	/* add zero pad for small packets */
    	if(unlikely(eth_dev.tx_pkt_info.pktSize < QD_MIN_ETH_PACKET_LEN)) {
            /* add zero pad in last cell of packet in buf_info_arr */
            GTW_DBG(GTW_DBG_TX,("add zero pad\n"));
            eth_dev.tx_buf_info_arr[eth_dev.tx_pkt_info.numFrags].bufVirtPtr = zero_pad;
            eth_dev.tx_buf_info_arr[eth_dev.tx_pkt_info.numFrags].bufSize = 
			QD_MIN_ETH_PACKET_LEN - eth_dev.tx_pkt_info.pktSize;
            eth_dev.tx_pkt_info.pktSize += QD_MIN_ETH_PACKET_LEN - eth_dev.tx_pkt_info.pktSize;
            eth_dev.tx_pkt_info.numFrags++;
    	}
#endif
	mv_gtw_update_tx_skb(&switch_info, mvp->vlan_cfg);
	
        status = mvEthPortTx( eth_dev.hal_priv, queue, &eth_dev.tx_pkt_info );
        if( status == MV_OK ) {
            stats->tx_packets ++;
            stats->tx_bytes += eth_dev.tx_pkt_info.pktSize;
            dev->trans_start = jiffies;
            eth_dev.txq_count[queue]++;
            EGIGA_STAT( EGIGA_STAT_TX, (eth_dev.eth_stat.tx_hal_ok[queue]++) );
        }
        else
        {
            /* tx failed. higher layers will free the skb */
            stats->tx_dropped++;
            if( status == MV_NO_RESOURCE ) {
                /* it must not happen because we call to netif_stop_queue in advance. */
                GTW_DBG( GTW_DBG_TX, ("%s: queue is full, stop transmit\n", dev->name) );
                netif_stop_queue( dev );
                EGIGA_STAT( EGIGA_STAT_TX, (eth_dev.eth_stat.tx_hal_no_resource[queue]++) );
                EGIGA_STAT( EGIGA_STAT_TX, (eth_dev.eth_stat.tx_netif_stop[queue]++) );
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


static int mv_gtw_tx( struct sk_buff *skb , struct net_device *dev )
{
    mv_priv *mvp = dev->priv;
    struct net_device_stats *stats = &(mvp->net_dev_stat);
    unsigned long flags;
    MV_STATUS status;
    int ret = 0, i, queue = ETH_DEF_TXQ;
    unsigned int switch_info;
   
    if(netif_queue_stopped(dev)) {
        printk( KERN_ERR "%s: transmitting while stopped\n", dev->name );
        return 1;
    }

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,9)
    local_irq_save(flags);
    if (unlikely(!spin_trylock(&eth_dev.lock))) {
    	/* Collision - tell upper layer to requeue */
        local_irq_restore(flags);
        return NETDEV_TX_LOCKED;
    }
#else
    spin_lock_irqsave( &eth_dev.lock, flags );
#endif

    GTW_DBG( GTW_DBG_TX, ("%s: tx, #%d frag(s), csum by %s\n",
             dev->name, skb_shinfo(skb)->nr_frags+1, (skb->ip_summed==CHECKSUM_HW)?"HW":"CPU") );
    EGIGA_STAT( EGIGA_STAT_TX, (eth_dev.eth_stat.tx_events++) );
#ifdef ETH_INCLUDE_TSO
    EGIGA_STAT( EGIGA_STAT_TSO, (eth_dev.eth_stat.tso_stats[skb->len >> 10]++) ); 
    if(skb_shinfo(skb)->frag_list != NULL)
    {
        printk("eth Warning: skb->frag_list != NULL\n");
        print_skb(skb);
    }
    if(skb_shinfo(skb)->tso_size) {
        ret = eth_tso_tx(skb, dev);
        goto tx_end;
    }
#endif /* ETH_INCLUDE_TSO */

    /* basic init of pkt_info */
    eth_dev.tx_pkt_info.osInfo = (MV_ULONG)skb;
    eth_dev.tx_pkt_info.pktSize = skb->len;
    eth_dev.tx_pkt_info.pFrags = &eth_dev.tx_buf_info_arr[0];
    eth_dev.tx_pkt_info.numFrags = 0;
    eth_dev.tx_pkt_info.status = 0;
    
    /* see if this is a single/multiple buffered skb */
    if( skb_shinfo(skb)->nr_frags == 0 ) {
        eth_dev.tx_pkt_info.pFrags->bufVirtPtr = skb->data;
        eth_dev.tx_pkt_info.pFrags->bufSize = skb->len;
        eth_dev.tx_pkt_info.numFrags = 1;
    }
    else {
        MV_BUF_INFO *p_buf_info = eth_dev.tx_pkt_info.pFrags;

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

        eth_dev.tx_pkt_info.numFrags = skb_shinfo(skb)->nr_frags + 1;
    }

#ifdef TX_CSUM_OFFLOAD
    /* if HW is suppose to offload layer4 checksum, set some bits in the first buf_info command */
    if(skb->ip_summed == CHECKSUM_HW) {
        GTW_DBG( GTW_DBG_TX, ("%s: tx csum offload\n", dev->name) );
        /*EGIGA_STAT( EGIGA_STAT_TX, Add counter here );*/
        eth_dev.tx_pkt_info.status =
        ETH_TX_IP_NO_FRAG |           /* we do not handle fragmented IP packets. add check inside iph!! */
	ETH_TX_VLAN_TAGGED_FRAME_MASK |                             /* mark the VLAN tag that we insert */
        ((skb->nh.iph->ihl) << ETH_TX_IP_HEADER_LEN_OFFSET) |                            /* 32bit units */
        ((skb->nh.iph->protocol == IPPROTO_TCP) ? ETH_TX_L4_TCP_TYPE : ETH_TX_L4_UDP_TYPE) | /* TCP/UDP */
        ETH_TX_GENERATE_L4_CHKSUM_MASK |                                /* generate layer4 csum command */
        ETH_TX_GENERATE_IP_CHKSUM_MASK;                              /* generate IP csum (already done?) */
    }
    else {
        GTW_DBG( GTW_DBG_TX, ("%s: no tx csum offload\n", dev->name) );
        /*EGIGA_STAT( EGIGA_STAT_TX, Add counter here );*/
        eth_dev.tx_pkt_info.status = 0x5 << ETH_TX_IP_HEADER_LEN_OFFSET; /* Errata BTS #50 */
    }
#endif

    /* add zero pad for small packets */
    if(unlikely(eth_dev.tx_pkt_info.pktSize < QD_MIN_ETH_PACKET_LEN)) {
        /* add zero pad in last cell of packet in buf_info_arr */
        GTW_DBG(GTW_DBG_TX,("add zero pad\n"));
        eth_dev.tx_buf_info_arr[eth_dev.tx_pkt_info.numFrags].bufVirtPtr = zero_pad;
        eth_dev.tx_buf_info_arr[eth_dev.tx_pkt_info.numFrags].bufSize = QD_MIN_ETH_PACKET_LEN - eth_dev.tx_pkt_info.pktSize;
        eth_dev.tx_pkt_info.pktSize += QD_MIN_ETH_PACKET_LEN - eth_dev.tx_pkt_info.pktSize;
        eth_dev.tx_pkt_info.numFrags++;
    }

    mv_gtw_update_tx_skb(&switch_info, mvp->vlan_cfg);

    /* now send the packet */
    status = mvEthPortTx( eth_dev.hal_priv, queue, &eth_dev.tx_pkt_info );

    /* check status */
    if(likely(status == MV_OK)) {
        stats->tx_bytes += skb->len;
        stats->tx_packets++;
        dev->trans_start = jiffies;
        eth_dev.txq_count[queue]++;
        GTW_DBG( GTW_DBG_TX, ("ok (%d); ", eth_dev.txq_count[queue]) );
        EGIGA_STAT( EGIGA_STAT_TX, (eth_dev.eth_stat.tx_hal_ok[queue]++) );
    }
    else {
        /* tx failed. higher layers will free the skb */
        ret = 1;
        stats->tx_dropped++;

        if(status == MV_NO_RESOURCE) {
            /* it must not happen because we call to netif_stop_queue in advance. */
            GTW_DBG( GTW_DBG_TX, ("%s: queue is full, stop transmit (should never happen here)\n", dev->name) );
	    mv_gtw_stop_netif();
            EGIGA_STAT( EGIGA_STAT_TX, (eth_dev.eth_stat.tx_hal_no_resource[queue]++) );
            EGIGA_STAT( EGIGA_STAT_TX, (eth_dev.eth_stat.tx_netif_stop[queue]++) );
        }
        else if(status == MV_ERROR) {
            printk( KERN_ERR "%s: error on transmit\n", dev->name );
            EGIGA_STAT( EGIGA_STAT_TX, (eth_dev.eth_stat.tx_hal_error[queue]++) );
        }
        else {
            printk( KERN_ERR "%s: unrecognized status on transmit\n", dev->name );
            EGIGA_STAT( EGIGA_STAT_TX, (eth_dev.eth_stat.tx_hal_unrecognize[queue]++) );
        }
    }

#ifdef ETH_INCLUDE_TSO
tx_end:
#endif

#ifdef ETH_TX_DONE_ISR
#else
    if(eth_dev.txq_count[ETH_DEF_TXQ] >= eth_tx_done_quota)
    {
        mv_gtw_tx_done();
    }
#endif /* ETH_TX_DONE_ISR */ 

    /* if number of available descriptors left is less than MAX_SKB_FRAGS stop the stack. */
    if(unlikely(mvEthTxResourceGet(eth_dev.hal_priv, queue) <= MAX_SKB_FRAGS)) {
        GTW_DBG( GTW_DBG_TX, ("%s: stopping network tx interface\n", dev->name) );
	mv_gtw_stop_netif();
    }

    spin_unlock_irqrestore( &eth_dev.lock, flags );

    return ret;
}


static unsigned int mv_gtw_tx_done(void)
{
    MV_PKT_INFO pkt_info;
    unsigned int count = 0;
    MV_STATUS status;
    unsigned int queue = 0;

    EGIGA_STAT( EGIGA_STAT_TX_DONE, (eth_dev.eth_stat.tx_done_events++) );

    /* read cause once more and clear tx bits */
    mv_gtw_read_save_clear_tx_cause();	

    /* release the transmitted packets */
    while( 1 ) {
#if defined(CONFIG_MV_INCLUDE_UNM_ETH) /* UNIMAC */
	/* Note: in UniMAC we do not break if !eth_dev.txcause - we check the Tx ring anyway */
	/* That is because the Interrupt Cause Register is not updated with new events, if they are masked */
	/* in the Interrupt Mask Register. */
	/* Do not uncomment */
        /* if(unlikely(!eth_dev.txcause)) */
	/*     break; */
#else /* not UNIMAC */
	if(unlikely(!eth_dev.txcause)) 
		break;
#endif /* UNIMAC */

        queue = ETH_DEF_TXQ;

        /* get a packet */  
        status = mvEthPortTxDone( eth_dev.hal_priv, queue, &pkt_info );

	if(likely(status == MV_OK)) {
	    eth_dev.txq_count[queue]--;

	    /* handle tx error */
	    if(unlikely(pkt_info.status & (ETH_ERROR_SUMMARY_BIT))) {
	        GTW_DBG( GTW_DBG_TX_DONE, ("bad tx-done status\n") );
		EGIGA_STAT( EGIGA_STAT_TX_DONE, (eth_dev.eth_stat.tx_done_hal_bad_stat[queue]++) );
	    }

	    count++;
	    EGIGA_STAT( EGIGA_STAT_TX_DONE, (eth_dev.eth_stat.tx_done_hal_ok[queue]++) );
	    EGIGA_STAT( EGIGA_STAT_TX_DONE, if(eth_dev.eth_stat.tx_done_max[queue] < count) eth_dev.eth_stat.tx_done_max[queue] = count );
	    EGIGA_STAT( EGIGA_STAT_TX_DONE, if(eth_dev.eth_stat.tx_done_min[queue] > count) eth_dev.eth_stat.tx_done_min[queue] = count );

	    
	    /* validate skb */
	    if(unlikely(!(pkt_info.osInfo))) {
		EGIGA_STAT( EGIGA_STAT_TX_DONE, (eth_dev.eth_stat.tx_done_hal_invalid_skb[queue]++) );
		continue;
	    }

	    /* release the skb */
	    dev_kfree_skb_any( (struct sk_buff *)pkt_info.osInfo );
	}
	else {
	    if( status == MV_EMPTY ) {
	    	/* no more work */
	    	GTW_DBG( GTW_DBG_TX_DONE, ("no more work ") );
	    	EGIGA_STAT( EGIGA_STAT_TX_DONE, (eth_dev.eth_stat.tx_done_hal_no_more[queue]++) );
	    }
	    else if( status == MV_NOT_FOUND ) {
	    	/* hw still in tx */
	    	GTW_DBG( GTW_DBG_TX_DONE, ("hw still in tx ") );
	    	EGIGA_STAT( EGIGA_STAT_TX_DONE, (eth_dev.eth_stat.tx_done_hal_still_tx[queue]++) );
	    }
	    else {
	    	printk( KERN_ERR "unrecognized status on tx done\n");
	    	EGIGA_STAT( EGIGA_STAT_TX_DONE, (eth_dev.eth_stat.tx_done_hal_unrecognize[queue]++) );
	    }

	    eth_dev.txcause &= ~ETH_CAUSE_TX_BUF_MASK(queue);
	    break;
    	}
    }
    /* it transmission was previously stopped, now it can be restarted. */
    if( count &&  eth_dev.netif_stopped) {
	mv_gtw_wakeup_netif();
    }

    GTW_DBG( GTW_DBG_TX_DONE, ("tx-done %d (%d)\n", count, eth_dev.txq_count[queue]) );
		
    return count;
}


static void mv_gtw_tx_timeout( struct net_device *dev ) 
{
    EGIGA_STAT( EGIGA_STAT_TX, (eth_dev.eth_stat.tx_timeout++) );
    printk( KERN_INFO "%s: tx timeout\n", dev->name );
}


#ifdef RX_CSUM_OFFLOAD
static MV_STATUS mv_gtw_rx_csum_offload(MV_PKT_INFO *pkt_info)
{
    if( (pkt_info->pktSize > ETH_CSUM_MIN_BYTE_COUNT)   && /* Minimum        */
        (pkt_info->status & ETH_RX_IP_FRAME_TYPE_MASK) && /* IPv4 packet    */
        (pkt_info->status & ETH_RX_IP_HEADER_OK_MASK)  && /* IP header OK   */
        (!(pkt_info->fragIP))                          && /* non frag IP    */
        (!(pkt_info->status & ETH_RX_L4_OTHER_TYPE))   && /* L4 is TCP/UDP  */
        (pkt_info->status & ETH_RX_L4_CHECKSUM_OK_MASK) ) /* L4 checksum OK */
            return MV_OK;

    if(!(pkt_info->pktSize > ETH_CSUM_MIN_BYTE_COUNT))
        GTW_DBG( GTW_DBG_RX, ("Byte count smaller than %d\n", ETH_CSUM_MIN_BYTE_COUNT) );
    if(!(pkt_info->status & ETH_RX_IP_FRAME_TYPE_MASK))
        GTW_DBG( GTW_DBG_RX, ("Unknown L3 protocol\n") );
    if(!(pkt_info->status & ETH_RX_IP_HEADER_OK_MASK))
        GTW_DBG( GTW_DBG_RX, ("Bad IP csum\n") );
    if(pkt_info->fragIP)
        GTW_DBG( GTW_DBG_RX, ("Fragmented IP\n") );
    if(pkt_info->status & ETH_RX_L4_OTHER_TYPE)
        GTW_DBG( GTW_DBG_RX, ("Unknown L4 protocol\n") );
    if(!(pkt_info->status & ETH_RX_L4_CHECKSUM_OK_MASK))
        GTW_DBG( GTW_DBG_RX, ("Bad L4 csum\n") );

    return MV_FAIL;
}
#endif


static unsigned int mv_gtw_rx(unsigned int work_to_do)
{
    unsigned int if_num;
    struct sk_buff *skb;
    MV_PKT_INFO pkt_info;
    int work_done = 0;
    MV_STATUS status;
    unsigned int queue = 0;

#ifdef INCLUDE_MULTI_QUEUE
    mv_gtw_read_save_clear_rx_cause();

    GTW_DBG( GTW_DBG_RX,("cause = 0x%08x\n", eth_dev.rxcause) );
#endif /* INCLUDE_MULTI_QUEUE */

    EGIGA_STAT( EGIGA_STAT_RX, (eth_dev.eth_stat.rx_events++) );
    
#ifdef CONFIG_MV_GTW_QOS_VOIP
    if(mv_gtw_qos_tos_quota != -1) {
        work_to_do = (work_to_do<mv_gtw_qos_tos_quota) ? work_to_do : mv_gtw_qos_tos_quota;
    }
#endif

    GTW_DBG( GTW_DBG_RX, ("^rx work_to_do %d\n",work_to_do) );

    while( work_done < work_to_do ) {
#ifdef INCLUDE_MULTI_QUEUE
        if(!eth_dev.rxcause)
	    break;
	if(eth_dev.rx_policy_priv)
	    queue = mvEthRxPolicyGet(eth_dev.rx_policy_priv, eth_dev.rxcause); 
        else
	    queue = ETH_DEF_RXQ;
#else
        queue = ETH_DEF_RXQ;
#endif /* INCLUDE_MULTI_QUEUE */

        /* get rx packet */ 
	status = mvEthPortRx( eth_dev.hal_priv, queue, &pkt_info );
        /* check status */
	if(likely(status == MV_OK)) {
	    work_done++;
	    eth_dev.rxq_count[queue]--;
	    EGIGA_STAT( EGIGA_STAT_RX, (eth_dev.eth_stat.rx_hal_ok[queue]++) );
	} else { 
	    if( status == MV_NO_RESOURCE ) {
	    	/* no buffers for rx */
	    	GTW_DBG( GTW_DBG_RX, ("rx no resource ") );
	    	EGIGA_STAT( EGIGA_STAT_RX, (eth_dev.eth_stat.rx_hal_no_resource[queue]++) );
		TRC_REC("rx no resource\n");
	    } 
	    else if( status == MV_NO_MORE ) {
	    	/* no more rx packets ready */
	    	GTW_DBG( GTW_DBG_RX, ("rx no more ") );
	    	EGIGA_STAT( EGIGA_STAT_RX, (eth_dev.eth_stat.rx_hal_no_more[queue]++) );
		TRC_REC("rx no more\n");
	    } 
	    else {		
	    	printk( KERN_ERR "undefined status on rx poll\n");
	    	EGIGA_STAT( EGIGA_STAT_RX, (eth_dev.eth_stat.rx_hal_error[queue]++) );
		TRC_REC("rx undefined status\n");
	    }

#ifdef INCLUDE_MULTI_QUEUE
	    eth_dev.rxcause &= ~ETH_CAUSE_RX_READY_MASK(queue);
	    continue;
#else
	    break;
#endif
	}

	/* validate skb */ 
	if(unlikely(!(pkt_info.osInfo))) {
	    printk( KERN_ERR "error in rx\n");
	    EGIGA_STAT( EGIGA_STAT_RX, (eth_dev.eth_stat.rx_hal_invalid_skb[queue]++) );
	    continue;
	}

	skb = (struct sk_buff *)( pkt_info.osInfo );

	/* handle rx error */
	if(unlikely(pkt_info.status & (ETH_ERROR_SUMMARY_MASK))) {
            unsigned int err = pkt_info.status & ETH_RX_ERROR_CODE_MASK;
	    TRC_REC("rx general error\n");
            
	    mv_gtw_print_rx_errors(err, pkt_info.status);
	    
	    dev_kfree_skb(skb);
	    EGIGA_STAT( EGIGA_STAT_RX, (eth_dev.eth_stat.rx_hal_bad_stat[queue]++) );
	    continue;
	}

	/* good rx */
        GTW_DBG( GTW_DBG_RX, ("\ngood rx Q%d\n",queue));

	if(mv_gtw_update_rx_skb(&pkt_info, skb, &if_num) != MV_OK) {
		dev_kfree_skb(skb);
		continue;
	}

#ifdef RX_CSUM_OFFLOAD
        /* checksum offload */
        if( mv_gtw_rx_csum_offload( &pkt_info ) == MV_OK ) {

            GTW_DBG( GTW_DBG_RX, ("rx csum offload ok\n") );
            /* EGIGA_STAT( EGIGA_STAT_RX, Add counter here) */

            skb->ip_summed = CHECKSUM_UNNECESSARY;

            /* Is this necessary? */
            skb->csum = htons((pkt_info.status & ETH_RX_L4_CHECKSUM_MASK) >> ETH_RX_L4_CHECKSUM_OFFSET);
        }
        else {
            GTW_DBG( GTW_DBG_RX, ("rx csum offload failed\n") );
            /* EGIGA_STAT( EGIGA_STAT_RX, Add counter here) */
            skb->ip_summed = CHECKSUM_NONE;
        }
#else
        skb->ip_summed = CHECKSUM_NONE;
#endif

	skb->protocol = eth_type_trans(skb,skb->dev); 

#ifdef CONFIG_MV_GTW_IGMP
	if( (if_num != 0) && 
	    (ntohs(skb->protocol) == ETH_P_IP) && 
	    (((struct iphdr*)(skb->data))->protocol == IPPROTO_IGMP)) {		
		GTW_DBG( GTW_DBG_IGMP, ("invoke IGMP snooping handler for IGMP message"));
		mv_gtw_igmp_snoop_process(skb, (unsigned char)if_num, (unsigned char)if_num);		
	}
#endif

	status = netif_receive_skb( skb );
        EGIGA_STAT( EGIGA_STAT_RX, if(status) (eth_dev.eth_stat.rx_netif_drop[queue]++) );
    }

    GTW_DBG( GTW_DBG_RX, ("\nwork_done %d (%d)\n", work_done, eth_dev.rxq_count[queue]) );

    if(work_done)
        /* refill rx ring with new buffers */
        mv_gtw_rx_fill();

    return(work_done);
}


void mv_gtw_rx_fill()
{
    MV_PKT_INFO pkt_info;
    MV_BUF_INFO bufInfo;
    struct sk_buff *skb;
    unsigned int count, buf_size;
    MV_STATUS status;
    int alloc_skb_failed = 0;
    unsigned int queue = 0;
    int total = 0;

    for (queue = 0; queue < MV_ETH_RX_Q_NUM; queue++) {

	/* Don't try to fill the ring for this queue if it is already full */
	if (mvEthRxResourceGet(eth_dev.hal_priv, queue) >= eth_desc_num_per_rxq[queue])
	    continue;

    	total = eth_desc_num_per_rxq[queue];
	count = 0;
    	EGIGA_STAT( EGIGA_STAT_RX_FILL, (eth_dev.eth_stat.rx_fill_events[queue]++) );
    
    	while( total-- ) {

            /* allocate a buffer */
	    buf_size = MV_MTU + 32 /* 32(extra for cache prefetch) */ + 8 /* +8 to align on 8B */;

            skb = dev_alloc_skb( buf_size ); 
	    if(unlikely(!skb)) {
	    	GTW_DBG( GTW_DBG_RX_FILL, ("rx_fill failed alloc skb\n") );
	    	EGIGA_STAT( EGIGA_STAT_RX_FILL, (eth_dev.eth_stat.rx_fill_alloc_skb_fail[queue]++) );
	    	alloc_skb_failed = 1;
	    	break;
	    }

	    /* align the buffer on 8B */
	    if( (unsigned long)(skb->data) & 0x7 ) {
	    	skb_reserve( skb, 8 - ((unsigned long)(skb->data) & 0x7) );
	    }

            bufInfo.bufVirtPtr = skb->data;
            bufInfo.bufSize = MV_MTU;
            pkt_info.osInfo = (MV_ULONG)skb;
            pkt_info.pFrags = &bufInfo;
	    pkt_info.pktSize = MV_MTU; /* how much to invalidate */

	    /* give the buffer to hal */
	    status = mvEthPortRxDone( eth_dev.hal_priv, queue, &pkt_info );
	
	    if(likely(status == MV_OK)) {
	    	count++;
	    	eth_dev.rxq_count[queue]++;
	    	EGIGA_STAT( EGIGA_STAT_RX_FILL, (eth_dev.eth_stat.rx_fill_hal_ok[queue]++) );	    
	    }
	    else if( status == MV_FULL ) {
	    	/* the ring is full */
	    	count++;
	    	eth_dev.rxq_count[queue]++;
	    	GTW_DBG( GTW_DBG_RX_FILL, ("rxq full\n") );
	    	EGIGA_STAT( EGIGA_STAT_RX_FILL, (eth_dev.eth_stat.rx_fill_hal_full[queue]++) );
	    	if( eth_dev.rxq_count[queue] != eth_desc_num_per_rxq[queue])
	            printk( KERN_ERR "mv_gateway: Q %d: error in status fill (%d != %d)\n", queue, eth_dev.rxq_count[queue], eth_desc_num_per_rxq[queue]);	    	
	    	break;
	    } 
	    else {
	    	printk( KERN_ERR "mv_gateway: Q %d: error in rx-fill\n", queue );
	    	EGIGA_STAT( EGIGA_STAT_RX_FILL, (eth_dev.eth_stat.rx_fill_hal_error[queue]++) );
	    	dev_kfree_skb(skb);
	    	break;
	    }
    	}

    	/* if allocation failed and the number of rx buffers in the ring is less than */
    	/* half of the ring size, then set a timer to try again later.                */
    	if(unlikely(alloc_skb_failed)) {
            if( eth_dev.rx_fill_flag == 0 ) {
	    	printk( KERN_INFO "mv_gateway: setting rx timeout to allocate skb\n");
	    	eth_dev.rx_fill_timer.expires = jiffies + (HZ/10); /*100ms*/
	    	add_timer( &eth_dev.rx_fill_timer );
	    	eth_dev.rx_fill_flag = 1;
	    }
	    break;
    	}

        GTW_DBG( GTW_DBG_RX_FILL, ("queue %d: rx fill %d (total %d)", queue, count, eth_dev.rxq_count[queue]) );
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

    GTW_DBG( GTW_DBG_TX, ("%s: tx_timer_callback", dev->name) );
    EGIGA_STAT( EGIGA_STAT_RX_FILL, (eth_dev.eth_stat.tx_timer_events++) );
    if( !netif_queue_stopped( dev ) )
    {
        /* TX enable */
        mvEthPortTxEnable(eth_dev.hal_priv);
    }
    /* Call TX done */
#ifdef ETH_TX_DONE_ISR
#else
    if(eth_dev.txq_count[ETH_DEF_TXQ] > 0)
    {
        unsigned long flags;

        spin_lock_irqsave( &(eth_dev.lock), flags );

        mv_gtw_tx_done();

        spin_unlock_irqrestore( &(eth_dev.lock), flags);
    }
#endif /* ETH_TX_DONE_ISR */ 

    if(eth_dev.tx_timer_flag)
    {
        eth_dev.tx_timer.expires = jiffies + ((HZ*ETH_TX_TIMER_PERIOD)/1000); /*ms*/
        add_timer( &eth_dev.tx_timer );
    }
}
#endif /* ETH_TX_TIMER_PERIOD */

void mv_gtw_rx_fill_on_timeout( unsigned long data ) 
{
    unsigned long flags;

    spin_lock_irqsave( &eth_dev.lock, flags );
    
    GTW_DBG( GTW_DBG_RX_FILL, ("rx_fill_on_timeout") );
    EGIGA_STAT( EGIGA_STAT_RX_FILL, (eth_dev.eth_stat.rx_fill_timeout_events++) );

    eth_dev.rx_fill_flag = 0;
    mv_gtw_rx_fill();

    spin_unlock_irqrestore( &eth_dev.lock, flags );
}


static int mv_gtw_poll( struct net_device *dev, int *budget )
{
    int rx_work_done;
    int tx_work_done = 0;
    unsigned long flags;

    EGIGA_STAT( EGIGA_STAT_INT, (eth_dev.eth_stat.poll_events++) );
#ifdef ETH_TX_DONE_ISR
    tx_work_done = mv_gtw_tx_done();
#endif
    rx_work_done = mv_gtw_rx( min(*budget,dev->quota) );

    *budget -= rx_work_done;
    dev->quota -= rx_work_done;

    GTW_DBG( GTW_DBG_RX, ("poll work done: tx-%d rx-%d\n",tx_work_done,rx_work_done) );
    TRC_REC("^mv_gtw_poll work done: tx-%d rx-%d\n",tx_work_done,rx_work_done);

    if( ((tx_work_done==0) && (rx_work_done==0)) || (!netif_running(dev)) ) {
	    local_irq_save(flags);
	    TRC_REC("^mv_gtw_poll remove from napi\n");
            netif_rx_complete(dev);
	    EGIGA_STAT( EGIGA_STAT_INT, (eth_dev.eth_stat.poll_complete++) );
	    /* unmask interrupts */
	    mv_gtw_unmask_interrupts(ETH_DEF_PORT, ETH_PICR_MASK, ETH_TXQ_MASK);
	    GTW_DBG( GTW_DBG_RX, ("unmask\n") );
	    local_irq_restore(flags);
            return 0;
    }

    return 1;
}


static irqreturn_t mv_gtw_interrupt_handler( int irq , void *dev_id , struct pt_regs *regs )
{
    spin_lock( &eth_dev.lock );
    EGIGA_STAT( EGIGA_STAT_INT, (eth_dev.eth_stat.irq_total++) );

    mv_gtw_read_interrupt_cause_regs();
 
    GTW_DBG( GTW_DBG_INT, ("%s [picr %08x]",__FUNCTION__,eth_dev.picr) );
    TRC_REC("^mv_gtw_interrupt_handler\n");

    if(unlikely(!(eth_dev.picr))) {
        EGIGA_STAT( EGIGA_STAT_INT, (eth_dev.eth_stat.irq_none++) );
	spin_unlock( &eth_dev.lock );
        return IRQ_NONE;
    }

    if(unlikely(main_net_dev == NULL)) {
	printk("Interrupt while no interface is up\n");
	spin_unlock( &eth_dev.lock );
	return IRQ_HANDLED;
    }

    mv_gtw_save_clear_rx_tx_cause();

    /* schedule the first net_device to do the work out of interrupt context (NAPI) */
    if(netif_rx_schedule_prep(main_net_dev)) {
        TRC_REC("^sched napi\n");

	mv_gtw_eth_mask_interrupts(ETH_DEF_PORT);

	mv_gtw_clear_interrupts(ETH_DEF_PORT);

        /* schedule the work (rx+txdone) out of interrupt contxet */
        __netif_rx_schedule(main_net_dev);
    }
    else {
        if(unlikely(netif_running(main_net_dev))) {
	    mv_gtw_print_int_while_polling();
	}
    }

    spin_unlock( &eth_dev.lock );

    return IRQ_HANDLED;
}


#ifdef CONFIG_MV_GTW_LINK_STATUS

static void mv_gtw_link_timer_function(unsigned long data)
{
    mv_gtw_link_interrupt_handler(switch_irq, NULL, NULL);
}

static irqreturn_t mv_gtw_link_interrupt_handler(int irq , void *dev_id, struct pt_regs *regs)
{
    unsigned short switch_cause, phy_cause, phys_port, p;
    MV_U32 board_id = mvBoardIdGet();

    if ( (board_id != RD_88F5181L_VOIP_FE) && (board_id != RD_88F5181_GTW_FE) &&
	 (board_id != RD_88F5181L_VOIP_FXO_GE) ) { 
	if(eventGetIntStatus(qd_dev,&switch_cause) != GT_TRUE) /* bug in QD -> need to be GT_OK */
	    switch_cause = 0;
    }
    else {
	switch_cause = GT_PHY_INTERRUPT;
    }
    if(switch_cause & GT_PHY_INTERRUPT) {
        gprtGetPhyIntPortSummary(qd_dev,&phys_port);
        for(p=0; p<qd_dev->numOfPorts; p++) {
            if (MV_BIT_CHECK(phys_port, p)) {
		if(gprtGetPhyIntStatus(qd_dev,p,&phy_cause) == GT_OK) {
		    if(phy_cause & GT_LINK_STATUS_CHANGED) { 
			char *link=NULL, *duplex=NULL, *speed=NULL;
			GT_BOOL flag;
			GT_PORT_SPEED_MODE speed_mode;
			if(gprtGetLinkState(qd_dev,p,&flag) != GT_OK) {
			    printk("gprtGetLinkState failed (port %d)\n",p);
			    link = "ERR";
			}
			else link = (flag)?"up":"down";
			if(flag) {
			    if(gprtGetDuplex(qd_dev,p,&flag) != GT_OK) {
			        printk("gprtGetDuplex failed (port %d)\n",p);
			        duplex = "ERR";
			    }
			    else duplex = (flag)?"Full":"Half";
			    if(gprtGetSpeedMode(qd_dev,p,&speed_mode) != GT_OK) {
			        printk("gprtGetSpeedMode failed (port %d)\n",p);
			        speed = "ERR";
			    }
			    else {
			        if (speed_mode == PORT_SPEED_1000_MBPS)
			            speed = "1000Mbps";
			        else if (speed_mode == PORT_SPEED_100_MBPS)
			            speed = "100Mbps";
			        else
			            speed = "10Mbps";
			    }
			    printk("Port %d: Link-%s, %s-duplex, Speed-%s.\n",mv_gtw_port2lport(p),link,duplex,speed);
			}
			else {
			    printk("Port %d: Link-down\n",mv_gtw_port2lport(p));
			}
		    }
		}
	    }
	}
    }
    
    if ( (board_id == RD_88F5181L_VOIP_FE) || 
	 (board_id == RD_88F5181_GTW_FE)   || 
	 (board_id == RD_88F5181L_VOIP_FXO_GE) ) {
    	link_timer.expires = jiffies + (HZ); /* 1 second */ 
    	add_timer(&link_timer);
    }

    return IRQ_HANDLED;
}
#endif


static struct net_device_stats* mv_gtw_get_stats( struct net_device *dev )
{
    return &(((mv_priv *)dev->priv)->net_dev_stat);
}


static int mv_gtw_set_mac_addr( struct net_device *dev, void *p )
{
    mv_priv *mvp = (mv_priv *)dev->priv;
    struct mv_vlan_cfg *vlan_cfg = mvp->vlan_cfg;

    struct sockaddr *addr = p;
    if(!is_valid_ether_addr(addr->sa_data))
	return -EADDRNOTAVAIL;

    /* remove old mac addr from VLAN DB */
    mv_gtw_set_mac_addr_to_switch(dev->dev_addr,MV_GTW_VLANID_TO_GROUP(vlan_cfg->vlan_grp_id),(1<<QD_PORT_CPU),0);

    memcpy(dev->dev_addr, addr->sa_data, 6);

    /* add new mac addr to VLAN DB */
    mv_gtw_set_mac_addr_to_switch(dev->dev_addr,MV_GTW_VLANID_TO_GROUP(vlan_cfg->vlan_grp_id),(1<<QD_PORT_CPU),1);

    printk("mv_gateway: %s change mac address to %02x:%02x:%02x:%02x:%02x:%02x\n", dev->name,
       *(dev->dev_addr), *(dev->dev_addr+1), *(dev->dev_addr+2), *(dev->dev_addr+3), *(dev->dev_addr+4), *(dev->dev_addr+5));

#if (defined(CONFIG_MV_GTW_QOS) && (!defined(CONFIG_MV_INCLUDE_UNM_ETH)))
    mv_gtw_set_mac_addr_to_eth(dev, ETH_DEF_RXQ);
#endif
    return 0;
}

#if (defined(CONFIG_MV_GTW_QOS) && (!defined(CONFIG_MV_INCLUDE_UNM_ETH)))
static void mv_gtw_set_mac_addr_to_eth(struct net_device *dev, int queue)
{
    char curr_addr[6];
    int print_flag = 0;
    
    /* In order to coexist with QoS filtering at gigabit level, we must have a match between  */
    /* incomming packet DA and the mac addr (bits 47:4) set in gigabit level. Since different */
    /* interfaces has different MAC addresses, only the WAN interface benefits from QoS.      */
    if(strcmp(dev->name,CONFIG_MV_GTW_QOS_NET_IF)==0) {
        print_flag = 1;
	mvEthMacAddrSet(eth_dev.hal_priv, dev->dev_addr, queue);	
    }
    else { 
	mvEthMacAddrGet(ETH_DEF_PORT, curr_addr);

	if ( (dev->dev_addr[0] == curr_addr[0]) && 
	     (dev->dev_addr[1] == curr_addr[1]) &&
	     (dev->dev_addr[2] == curr_addr[2]) &&
	     (dev->dev_addr[3] == curr_addr[3]) &&
	     (dev->dev_addr[4] == curr_addr[4]) ) {
		/* have to set the entry in the unicast table to accept */
		print_flag = 1;
		mvEthMacAddrSet(eth_dev.hal_priv, dev->dev_addr, queue);
    	}
    }

    if (print_flag == 1) {
	if (queue == -1) {
	    GTW_DBG(GTW_DBG_MACADDR, ("mv_gateway: %s delete %02x %02x %02x %02x %02x %02x also from gigabit level\n", dev->name, *(dev->dev_addr), *(dev->dev_addr+1), *(dev->dev_addr+2), *(dev->dev_addr+3), *(dev->dev_addr+4), *(dev->dev_addr+5)));
	}
	else {
	    GTW_DBG(GTW_DBG_MACADDR, ("mv_gateway: %s set %02x %02x %02x %02x %02x %02x also in gigabit level\n", dev->name, *(dev->dev_addr), *(dev->dev_addr+1), *(dev->dev_addr+2), *(dev->dev_addr+3), *(dev->dev_addr+4), *(dev->dev_addr+5)));
	}
    }
}
#endif


int mv_gtw_set_mac_addr_to_switch(unsigned char *mac_addr, unsigned char db, unsigned int ports_mask, unsigned char op)
{
    GT_ATU_ENTRY mac_entry;
    struct mv_vlan_cfg *nc;

    /* validate db with VLAN id */
    nc = &net_cfg[db];
    if(MV_GTW_VLANID_TO_GROUP(nc->vlan_grp_id) != db) {
        printk("mv_gtw_set_mac_addr_to_switch (invalid db)\n");
	return -1;
    }

    memset(&mac_entry,0,sizeof(GT_ATU_ENTRY));

    mac_entry.trunkMember = GT_FALSE;
    mac_entry.prio = 0;
    mac_entry.exPrio.useMacFPri = GT_FALSE;
    mac_entry.exPrio.macFPri = 0;
    mac_entry.exPrio.macQPri = 0;
    mac_entry.DBNum = db;
    mac_entry.portVec = ports_mask;
    memcpy(mac_entry.macAddr.arEther,mac_addr,6);

    if(is_multicast_ether_addr(mac_addr))
	mac_entry.entryState.mcEntryState = GT_MC_STATIC;
    else
	mac_entry.entryState.ucEntryState = GT_UC_NO_PRI_STATIC;

    printk("mv_gateway: %s db%d port-mask=0x%x, %02x:%02x:%02x:%02x:%02x:%02x ", 
	nc->name, db, (unsigned int)mac_entry.portVec,
	mac_entry.macAddr.arEther[0],mac_entry.macAddr.arEther[1],mac_entry.macAddr.arEther[2],
	mac_entry.macAddr.arEther[3],mac_entry.macAddr.arEther[4],mac_entry.macAddr.arEther[5]);

    if((op == 0) || (mac_entry.portVec == 0)) { 
        if(gfdbDelAtuEntry(qd_dev, &mac_entry) != GT_OK) {
	    printk("gfdbDelAtuEntry failed\n");
	    return -1;
        }
	printk("deleted\n");
    }
    else {
        if(gfdbAddMacEntry(qd_dev, &mac_entry) != GT_OK) {
	    printk("gfdbAddMacEntry failed\n");
	    return -1;
        }
	printk("added\n");
    }

    return 0;
}


static void mv_gtw_set_multicast_list(struct net_device *dev) 
{
    struct dev_mc_list *curr_addr = dev->mc_list;
    mv_priv *mvp = (mv_priv *)dev->priv;
    struct mv_vlan_cfg *vlan_cfg = mvp->vlan_cfg;
    int i;
    GT_ATU_ENTRY mac_entry;
    GT_BOOL found = GT_FALSE;
    GT_STATUS status;

    disable_irq(ETH_PORT0_IRQ_NUM);

    if((dev->flags & IFF_PROMISC) || (dev->flags & IFF_ALLMULTI)) { 
	/* promiscuous mode - connect the CPU port to the VLAN (port based + 802.1q) */
	/*
	if(dev->flags & IFF_PROMISC)
	    printk("mv_gateway: setting promiscuous mode\n");
	if(dev->flags & IFF_ALLMULTI)
	    printk("mv_gateway: setting multicast promiscuous mode\n");
	*/
	mv_gtw_set_port_based_vlan(vlan_cfg->ports_mask|(1<<QD_PORT_CPU));
	for(i=0; i<qd_dev->numOfPorts; i++) {
	    if(MV_BIT_CHECK(vlan_cfg->ports_mask, i) && (i!=QD_PORT_CPU)) {
		if(mv_gtw_set_vlan_in_vtu(MV_GTW_PORT_VLAN_ID(vlan_cfg->vlan_grp_id,i),vlan_cfg->ports_mask|(1<<QD_PORT_CPU)) != 0) {
		    printk("mv_gtw_set_vlan_in_vtu failed\n");
		}
	   }
	}
    }
    else {
	/* not in promiscuous or allmulti mode - disconnect the CPU port to the VLAN (port based + 802.1q) */
	mv_gtw_set_port_based_vlan(vlan_cfg->ports_mask&(~(1<<QD_PORT_CPU)));
	for(i=0; i<qd_dev->numOfPorts; i++) {
	    if(MV_BIT_CHECK(vlan_cfg->ports_mask, i) && (i!=QD_PORT_CPU)) {
		if(mv_gtw_set_vlan_in_vtu(MV_GTW_PORT_VLAN_ID(vlan_cfg->vlan_grp_id,i),vlan_cfg->ports_mask&(~(1<<QD_PORT_CPU))) != 0) {
		    printk("mv_gtw_set_vlan_in_vtu failed\n");
		}
	   }
	}
	if(dev->mc_count) { 
	    /* accept specific multicasts */
	    for(i=0; i<dev->mc_count; i++, curr_addr = curr_addr->next) {
	        if (!curr_addr)
		    break;
		/* The Switch may already have information about this multicast address in      */
		/* its ATU. If this address is already in the ATU, use the existing port vector */
		/* ORed with the CPU port. Otherwise, just use the CPU port.                    */
		memset(&mac_entry,0,sizeof(GT_ATU_ENTRY));
		mac_entry.DBNum = MV_GTW_VLANID_TO_GROUP(vlan_cfg->vlan_grp_id);
		memcpy(mac_entry.macAddr.arEther, curr_addr->dmi_addr, 6);
		status = gfdbFindAtuMacEntry(qd_dev, &mac_entry, &found);
		if ( (status != GT_OK) || (found != GT_TRUE) ) {
		    mv_gtw_set_mac_addr_to_switch(curr_addr->dmi_addr, 
		    				  MV_GTW_VLANID_TO_GROUP(vlan_cfg->vlan_grp_id), 
						  (1<<QD_PORT_CPU)|(vlan_cfg->ports_mask), 1);
		}
		else {
		    mv_gtw_set_mac_addr_to_switch(curr_addr->dmi_addr, 
						  MV_GTW_VLANID_TO_GROUP(vlan_cfg->vlan_grp_id), 
						  (mac_entry.portVec | (1<<QD_PORT_CPU)), 1);
		}
	    }
	}
    }
    enable_irq(ETH_PORT0_IRQ_NUM);
}


GT_BOOL ReadMiiWrap(GT_QD_DEV* dev, unsigned int portNumber, unsigned int MIIReg, unsigned int* value)
{
    unsigned long flags;
    MV_STATUS status;

    spin_lock_irqsave(&eth_dev.mii_lock, flags);

    status = mvEthPhyRegRead(portNumber, MIIReg , (unsigned short *)value);
    spin_unlock_irqrestore(&eth_dev.mii_lock, flags);

    if (status == MV_OK)
        return GT_TRUE;

    return GT_FALSE;
}


GT_BOOL WriteMiiWrap(GT_QD_DEV* dev, unsigned int portNumber, unsigned int MIIReg, unsigned int data)
{
    unsigned long flags;
    MV_STATUS status;

    spin_lock_irqsave(&eth_dev.mii_lock, flags);

    status = mvEthPhyRegWrite(portNumber, MIIReg ,data);

    spin_unlock_irqrestore(&eth_dev.mii_lock, flags);

    if (status == MV_OK)
        return GT_TRUE;
    return GT_FALSE;
}


static int mv_gtw_set_port_based_vlan(unsigned int ports_mask)
{
    unsigned int p, pl;
    unsigned char cnt;
    GT_LPORT port_list[MAX_SWITCH_PORTS];

    for(p=0; p<qd_dev->numOfPorts; p++) {
	if( MV_BIT_CHECK(ports_mask, p) && (p != QD_PORT_CPU) ) {
	    GTW_DBG( GTW_DBG_LOAD|GTW_DBG_MCAST|GTW_DBG_VLAN, ("port based vlan, port %d: ",p));
	    for(pl=0,cnt=0; pl<qd_dev->numOfPorts; pl++) {
		if( MV_BIT_CHECK(ports_mask, pl) && (pl != p) ) {
		    GTW_DBG( GTW_DBG_LOAD|GTW_DBG_MCAST|GTW_DBG_VLAN, ("%d ",pl));
		    port_list[cnt] = pl;
                    cnt++;
                }
	    }
            if( gvlnSetPortVlanPorts(qd_dev, p, port_list, cnt) != GT_OK) {
	        printk("gvlnSetPortVlanPorts failed\n");
                return -1;
            }
	    GTW_DBG( GTW_DBG_LOAD|GTW_DBG_MCAST|GTW_DBG_VLAN, ("\n"));
        }
    }
    return 0;
}


static int mv_gtw_set_vlan_in_vtu(unsigned short vlan_id,unsigned int ports_mask)
{
    GT_VTU_ENTRY vtu_entry;
    unsigned int p;

    vtu_entry.vid = vlan_id;
    vtu_entry.DBNum = MV_GTW_VLANID_TO_GROUP(vlan_id);
    vtu_entry.vidPriOverride = GT_FALSE;
    vtu_entry.vidPriority = 0;
    vtu_entry.vidExInfo.useVIDFPri = GT_FALSE;
    vtu_entry.vidExInfo.vidFPri = 0;
    vtu_entry.vidExInfo.useVIDQPri = GT_FALSE;
    vtu_entry.vidExInfo.vidQPri = 0;
    vtu_entry.vidExInfo.vidNRateLimit = GT_FALSE;
    GTW_DBG( GTW_DBG_LOAD|GTW_DBG_MCAST|GTW_DBG_VLAN, ("vtu entry: vid=0x%x, port ", vtu_entry.vid));
    for(p=0; p<qd_dev->numOfPorts; p++) {
        if(MV_BIT_CHECK(ports_mask, p)) {
	    GTW_DBG( GTW_DBG_LOAD|GTW_DBG_MCAST|GTW_DBG_VLAN, ("%d ", p));
	    if(qd_dev->deviceId == GT_88E6061) {
                /* for 6061 device, no double/provider tag controlling on ingress. */
                /* therefore, we need to strip the tag on egress on all ports except cpu port */
                /* anyway, if we're using header mode no vlan-tag need to be added here */
#ifdef CONFIG_MV_GTW_HEADER_MODE
		vtu_entry.vtuData.memberTagP[p] = MEMBER_EGRESS_UNMODIFIED;
#else
		if(p == QD_PORT_CPU)
		    vtu_entry.vtuData.memberTagP[p] = MEMBER_EGRESS_TAGGED; /* in addition to egress tag mode, this is required for promisc mode */
		else
		    vtu_entry.vtuData.memberTagP[p] = MEMBER_EGRESS_UNTAGGED;
#endif
	    }
	    else {
		vtu_entry.vtuData.memberTagP[p] = MEMBER_EGRESS_UNMODIFIED;
	    }
	}
	else {
	    vtu_entry.vtuData.memberTagP[p] = NOT_A_MEMBER;
	}
	vtu_entry.vtuData.portStateP[p] = 0;
    }
    if(gvtuAddEntry(qd_dev, &vtu_entry) != GT_OK) {
        printk("gvtuAddEntry failed\n");
        return -1;
    }

    GTW_DBG( GTW_DBG_LOAD|GTW_DBG_MCAST|GTW_DBG_VLAN, ("\n"));
    return 0;
}


#ifdef CONFIG_MV_GTW_QOS_VOIP
int mv_gtw_qos_tos_enable(void)
{
    printk("mv_gateway: VoIP QoS ON\n");
    netdev_max_backlog = 15;
    MAX_SOFTIRQ_RESTART = 1;
    mv_gtw_qos_tos_quota = 5;
    TRC_REC("^QoS ON sched-iter=%d quota=%d netdev_max_backlog=%d\n",MAX_SOFTIRQ_RESTART,mv_gtw_qos_tos_quota,netdev_max_backlog);
    return 0;
}


int mv_gtw_qos_tos_disable(void)
{
    printk("mv_gateway VoIP QoS OFF\n");
    netdev_max_backlog = 300;
    MAX_SOFTIRQ_RESTART = 10;
    mv_gtw_qos_tos_quota = -1;
    TRC_REC("^QoS OFF sched-iter=%d quota=%d\n",MAX_SOFTIRQ_RESTART,mv_gtw_qos_tos_quota);
    return 0;
}
#endif /*CONFIG_MV_GTW_QOS*/


#ifdef CONFIG_MV_GTW_IGMP
int mv_gtw_enable_igmp(void)
{
    unsigned char p;

    GTW_DBG( GTW_DBG_IGMP, ("enabling L2 IGMP snooping\n"));

    /* enable IGMP snoop on all ports (except cpu port) */
    for(p=0; p<qd_dev->numOfPorts; p++) {
	if(p != QD_PORT_CPU) {
	    if(gprtSetIGMPSnoop(qd_dev, p, GT_TRUE) != GT_OK) {
		printk("gprtSetIGMPSnoop failed\n");
		return -1;
	    }
	}
    }
    return -1;
}
#endif


static void mv_gtw_convert_str_to_mac( char *source , char *dest ) 
{
    dest[0] = (mv_gtw_str_to_hex( source[0] ) << 4) + mv_gtw_str_to_hex( source[1] );
    dest[1] = (mv_gtw_str_to_hex( source[3] ) << 4) + mv_gtw_str_to_hex( source[4] );
    dest[2] = (mv_gtw_str_to_hex( source[6] ) << 4) + mv_gtw_str_to_hex( source[7] );
    dest[3] = (mv_gtw_str_to_hex( source[9] ) << 4) + mv_gtw_str_to_hex( source[10] );
    dest[4] = (mv_gtw_str_to_hex( source[12] ) << 4) + mv_gtw_str_to_hex( source[13] );
    dest[5] = (mv_gtw_str_to_hex( source[15] ) << 4) + mv_gtw_str_to_hex( source[16] );
}


static unsigned int mv_gtw_str_to_hex( char ch ) 
{
    if( (ch >= '0') && (ch <= '9') )
        return( ch - '0' );

    if( (ch >= 'a') && (ch <= 'f') )
	return( ch - 'a' + 10 );

    if( (ch >= 'A') && (ch <= 'F') )
	return( ch - 'A' + 10 );

    return 0;
}


#define STAT_PER_Q(qnum,x)  for(queue = 0; queue < qnum; queue++) \
				printk("%10u ",x[queue]); \
			    printk("\n");

void print_mv_gtw_stat(void)
{
#ifndef EGIGA_STATISTICS
  printk(" Error: driver is compiled without statistics support!! \n");
  return;
#else
    eth_statistics *stat = &eth_dev.eth_stat;
    unsigned int queue;

    printk("QUEUS:.........................");
    for(queue = 0; queue < MV_ETH_RX_Q_NUM; queue++) 
	printk( "%10d ",queue);
    printk("\n");

  if( mv_gtw_stat & EGIGA_STAT_INT ) {
      printk( "\n====================================================\n" );
      printk( "interrupt statistics" );
      printk( "\n-------------------------------\n" );
      printk( "irq_total.....................%10u\n", stat->irq_total );
      printk( "irq_none......................%10u\n", stat->irq_none );
  }
  if( mv_gtw_stat & EGIGA_STAT_POLL ) {
      printk( "\n====================================================\n" );
      printk( "polling statistics" );
      printk( "\n-------------------------------\n" );
      printk( "poll_events...................%10u\n", stat->poll_events );
      printk( "poll_complete.................%10u\n", stat->poll_complete );
  }
  if( mv_gtw_stat & EGIGA_STAT_RX ) {
      printk( "\n====================================================\n" );
      printk( "rx statistics" );
      printk( "\n-------------------------------\n" );
      printk( "rx_events................%10u\n", stat->rx_events );
      printk( "rx_hal_ok................"); STAT_PER_Q(MV_ETH_RX_Q_NUM, stat->rx_hal_ok);
      printk( "rx_hal_no_resource......."); STAT_PER_Q(MV_ETH_RX_Q_NUM, stat->rx_hal_no_resource );
      printk( "rx_hal_no_more..........."); STAT_PER_Q(MV_ETH_RX_Q_NUM, stat->rx_hal_no_more );
      printk( "rx_hal_error............."); STAT_PER_Q(MV_ETH_RX_Q_NUM, stat->rx_hal_error );
      printk( "rx_hal_invalid_skb......."); STAT_PER_Q(MV_ETH_RX_Q_NUM, stat->rx_hal_invalid_skb );
      printk( "rx_hal_bad_stat.........."); STAT_PER_Q(MV_ETH_RX_Q_NUM, stat->rx_hal_bad_stat );
      printk( "rx_netif_drop............"); STAT_PER_Q(MV_ETH_RX_Q_NUM, stat->rx_netif_drop );
  }
  if( mv_gtw_stat & EGIGA_STAT_RX_FILL ) {
      printk( "\n====================================================\n" );
      printk( "rx fill statistics" );
      printk( "\n-------------------------------\n" );
      printk( "rx_fill_events................"); STAT_PER_Q(MV_ETH_RX_Q_NUM, stat->rx_fill_events );
      printk( "rx_fill_alloc_skb_fail........"); STAT_PER_Q(MV_ETH_RX_Q_NUM, stat->rx_fill_alloc_skb_fail );
      printk( "rx_fill_hal_ok................"); STAT_PER_Q(MV_ETH_RX_Q_NUM,stat->rx_fill_hal_ok);
      printk( "rx_fill_hal_full.............."); STAT_PER_Q(MV_ETH_RX_Q_NUM, stat->rx_fill_hal_full );
      printk( "rx_fill_hal_error............."); STAT_PER_Q(MV_ETH_RX_Q_NUM, stat->rx_fill_hal_error );
      printk( "rx_fill_timeout_events........%10u\n", stat->rx_fill_timeout_events );
      printk( "rx buffer size................%10u\n", MV_MTU);
  }
  if( mv_gtw_stat & EGIGA_STAT_TX ) {
      printk( "\n====================================================\n" );
      printk( "tx statistics" );
      printk( "\n-------------------------------\n" );
      printk( "tx_events.....................%10u\n", stat->tx_events );
      printk( "tx_hal_ok.....................");STAT_PER_Q(MV_ETH_TX_Q_NUM, stat->tx_hal_ok);
      printk( "tx_hal_no_resource............");STAT_PER_Q(MV_ETH_TX_Q_NUM, stat->tx_hal_no_resource );
      printk( "tx_hal_no_error...............");STAT_PER_Q(MV_ETH_TX_Q_NUM, stat->tx_hal_error );
      printk( "tx_hal_unrecognize............");STAT_PER_Q(MV_ETH_TX_Q_NUM, stat->tx_hal_unrecognize );
      printk( "tx_netif_stop.................");STAT_PER_Q(MV_ETH_TX_Q_NUM, stat->tx_netif_stop );
      printk( "tx_timeout....................%10u\n", stat->tx_timeout );
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
  if( mv_gtw_stat & EGIGA_STAT_TX_DONE ) {
      printk( "\n====================================================\n" );
      printk( "tx-done statistics" );
      printk( "\n-------------------------------\n" );
      printk( "tx_done_events................%10u\n", stat->tx_done_events );
      printk( "tx_done_hal_ok................");STAT_PER_Q(MV_ETH_TX_Q_NUM, stat->tx_done_hal_ok);
      printk( "tx_done_hal_invalid_skb.......");STAT_PER_Q(MV_ETH_TX_Q_NUM, stat->tx_done_hal_invalid_skb );
      printk( "tx_done_hal_bad_stat..........");STAT_PER_Q(MV_ETH_TX_Q_NUM, stat->tx_done_hal_bad_stat );
      printk( "tx_done_hal_still_tx..........");STAT_PER_Q(MV_ETH_TX_Q_NUM, stat->tx_done_hal_still_tx );
      printk( "tx_done_hal_no_more...........");STAT_PER_Q(MV_ETH_TX_Q_NUM, stat->tx_done_hal_no_more );
      printk( "tx_done_hal_unrecognize.......");STAT_PER_Q(MV_ETH_TX_Q_NUM, stat->tx_done_hal_unrecognize );
      printk( "tx_done_max...................");STAT_PER_Q(MV_ETH_TX_Q_NUM, stat->tx_done_max );
      printk( "tx_done_min...................");STAT_PER_Q(MV_ETH_TX_Q_NUM, stat->tx_done_min );
      printk( "tx_done_netif_wake............");STAT_PER_Q(MV_ETH_TX_Q_NUM, stat->tx_done_netif_wake );
  }

  memset( stat, 0, sizeof(eth_statistics) );

  printk("QD CPU_PORT statistics: \n");
  print_qd_port_counters(QD_PORT_CPU);
#endif /*EGIGA_STATISTICS*/
}

/**************************************************************************************************************/

void print_qd_atu(int db)
{
    GT_ATU_ENTRY atu_entry;
    memset(&atu_entry,0,sizeof(GT_ATU_ENTRY));
    atu_entry.DBNum = db;
    printk("ATU DB%d display:\n",db);
    while(gfdbGetAtuEntryNext(qd_dev,&atu_entry) == GT_OK) {
	printk("%02x:%02x:%02x:%02x:%02x:%02x, PortVec %#lx, Pri %#x, State %#x\n",
	atu_entry.macAddr.arEther[0],atu_entry.macAddr.arEther[1],
	atu_entry.macAddr.arEther[2],atu_entry.macAddr.arEther[3],
	atu_entry.macAddr.arEther[4],atu_entry.macAddr.arEther[5],
	atu_entry.portVec,atu_entry.prio,atu_entry.entryState.mcEntryState);
    }
}

/**************************************************************************************************************/

static int init_eth_mac(void)
{
    MV_ETH_PORT_INIT hal_init_struct;
    MV_STATUS status;

    printk("init MAC layer... ");

    mvEthInit();
    hal_init_struct.maxRxPktSize = MV_MTU;
    hal_init_struct.rxDefQ = ETH_DEF_RXQ; 
    memcpy(hal_init_struct.rxDescrNum,  eth_desc_num_per_rxq, sizeof(eth_desc_num_per_rxq));
    memcpy(hal_init_struct.txDescrNum,  eth_desc_num_per_txq, sizeof(eth_desc_num_per_txq));
    eth_dev.hal_priv = mvEthPortInit(ETH_DEF_PORT, &hal_init_struct);
    if(!eth_dev.hal_priv) {
        printk(KERN_ERR "failed to init MAC hal\n");
	return -1;
    }
    spin_lock_init( &eth_dev.lock );
    spin_lock_init( &eth_dev.mii_lock );
    eth_dev.netif_stopped = 0;
    memset( &eth_dev.rx_fill_timer, 0, sizeof(struct timer_list) );
    init_timer(&eth_dev.rx_fill_timer);
    eth_dev.rx_fill_timer.function = mv_gtw_rx_fill_on_timeout;
    eth_dev.rx_fill_timer.data = -1;
    eth_dev.rx_fill_flag = 0;

#ifdef INCLUDE_MULTI_QUEUE
    /* init rx policy */
    eth_dev.rx_policy_priv = mvEthRxPolicyInit(ETH_DEF_PORT, ETH_RX_QUEUE_QUOTA, MV_ETH_PRIO_FIXED);
    if(!eth_dev.rx_policy_priv) {
        printk(KERN_ERR "failed to init rx policy\n");
    }

#ifdef CONFIG_MV_GTW_QOS_VOIP  
    /* set VoIP packets (marked with vlan-prio 3) to MAC queue 3 */
    if(mvEthVlanPrioRxQueue(eth_dev.hal_priv,(MV_VOIP_PRIO<<1),MV_VOIP_PRIO) != MV_OK) {
        printk(KERN_ERR "failed to prioritize VoIP VlanPrio=%d (queue %d)\n",(MV_VOIP_PRIO << 1), MV_VOIP_PRIO);
    }
#endif
#ifdef CONFIG_MV_GTW_QOS_ROUTING
    /* set Routing packets (marked with vlan-prio 2) to MAC queue 2 */
    if(mvEthVlanPrioRxQueue(eth_dev.hal_priv,(MV_ROUTING_PRIO<<1),MV_ROUTING_PRIO) != MV_OK) {
        printk(KERN_ERR "failed to prioritize Routing VlanPrio=%d (queue %d)\n",(MV_ROUTING_PRIO<<1),MV_ROUTING_PRIO);
    }
#endif
#if 0
    /* set ARP packets (marked with vlan-prio 1) to MAC queue 1 */
    if(mvEthVlanPrioRxQueue(eth_dev.hal_priv,(MV_ARP_PRIO<<1),MV_ARP_PRIO) != MV_OK) {
        printk(KERN_ERR "failed to prioritize VlanPrio=%d (queue %d)\n",(MV_ARP_PRIO << 1), MV_ARP_PRIO);
    }
#endif

#endif /* INCLUDE_MULTI_QUEUE */

#ifdef CONFIG_MV_GTW_HEADER_MODE
	mvEthHeaderModeSet(eth_dev.hal_priv, MV_ETH_ENABLE_HEADER_MODE_PRI_2_1);
#endif

#ifdef CONFIG_ETH_FLOW_CONTROL
    /* enable flow Control in MAC level */
    mvEthFlowCtrlSet(eth_dev.hal_priv, MV_ETH_FC_ENABLE);
#endif

    /* set unicast and multicast promiscuous mode in MAC level (unknowns are dropped in switch CPU port level) */
    GTW_DBG( GTW_DBG_LOAD, ("MAC in unicast promiscuos mode\n"));
    mvEthRxFilterModeSet(eth_dev.hal_priv, MV_TRUE);
             
    /* fill rx ring with buffers */
    mv_gtw_rx_fill();
    
    /* start the hal - rx/tx activity */
    status = mvEthPortEnable( eth_dev.hal_priv );
    if( (status != MV_OK) && (status != MV_NOT_READY)){
         printk(KERN_ERR "ethPortEnable failed\n");
	 return -1;
    }
#if defined(CONFIG_MV_INCLUDE_UNM_ETH) /* UNIMAC */
    eth_dev.rx_coal = 0; /* not used in UniMAC */
    eth_dev.tx_coal = 0; /* not used in UniMAC */
#else /* not UNIMAC */
    /* set tx/rx coalescing mechanism */
    eth_dev.rx_coal = mvEthRxCoalSet( eth_dev.hal_priv, 0);
    eth_dev.tx_coal = mvEthTxCoalSet( eth_dev.hal_priv, 0);
#endif /* UNIMAC */

#if (ETH_TX_TIMER_PERIOD > 0)
    memset( &eth_dev.tx_timer, 0, sizeof(struct timer_list) );
    eth_dev.tx_timer.function = eth_tx_timer_callback;
    init_timer(&eth_dev.tx_timer);
    eth_dev.tx_timer_flag = 0;
#endif 

#ifdef ETH_INCLUDE_TSO
    {
        int i;
	
	eth_dev.tx_extra_buf_idx = 0;

        for(i = 0; i < ETH_NUM_OF_TX_DESCR; i++)
        {
            eth_dev.tx_extra_bufs[i] = mvOsMalloc(128);
            if(eth_dev.tx_extra_bufs[i] == NULL)
            {
                printk("eth_TSO: Can't alloc extra TX buffer (128 bytes) for %d descr\n", i);
                return -ENOMEM;
            }
        }
    }
#endif /* ETH_INCLUDE_TSO */


    /* clear and mask interrupts */
    mv_gtw_clear_interrupts(ETH_DEF_PORT);
    eth_dev.rxcause = 0;
    eth_dev.txcause = 0;
    mv_gtw_eth_mask_interrupts(ETH_DEF_PORT);

    printk("done\n");
    return 0;
}

/**************************************************************************************************************/

static inline void mv_gtw_print_rx_errors(unsigned int err, MV_U32 pkt_info_status)
{
#if defined(CONFIG_MV_INCLUDE_UNM_ETH) /* UNIMAC */
    if (err & ETH_OVERRUN_ERROR_BIT) {
	GTW_DBG( GTW_DBG_RX, ("bad rx status %08x, (overrun error)\n",(unsigned int)pkt_info_status));
    }
    if (err & ETH_CRC_ERROR_BIT) {
	printk( KERN_INFO "bad rx status %08x, (crc error)\n",(unsigned int)pkt_info_status );
    }
#else /* not UNIMAC */
    /* RX resource error is likely to happen when receiving packets, which are     */
    /* longer then the Rx buffer size, and they are spreading on multiple buffers. */
    /* Rx resource error - No descriptor in the middle of a frame.                 */
    if( err == ETH_RX_RESOURCE_ERROR ) {
        GTW_DBG( GTW_DBG_RX, ("bad rx status %08x, (resource error)",(unsigned int)pkt_info_status));
    }
    else if( err == ETH_RX_OVERRUN_ERROR ) {
	GTW_DBG( GTW_DBG_RX, ("bad rx status %08x, (overrun error)",(unsigned int)pkt_info_status));
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

static void mv_gtw_eth_mac_addr_get(int port, char *addr)
{
#if defined(CONFIG_MV_INCLUDE_UNM_ETH)
    /* get it from bootloader info structure */
    memcpy(addr, mvMacAddr, 6);
#else
    /* get it from HW */
    mvEthMacAddrGet(port, addr);
#endif
}

/**************************************************************************************************************/

static inline void mv_gtw_align_ip_header(struct sk_buff *skb)
{
    /* When using Gigabit Ethernet HW or when working with Marvell Header in UniMAC HW,    */
	/* 2 extra bytes are shifted by the HW to align the IP header. Without Marvell Header, */
    /* UniMAC HW does not do this so the IP header is not aligned. This function shifts the 2 extra bytes */
#if defined(CONFIG_MV_INCLUDE_UNM_ETH) && !defined(CONFIG_MV_GTW_HEADER_MODE) /* UNIMAC */
    memmove( skb->data + 2, skb->data, skb->len);
    skb->data += 2;
    skb->tail += 2;
#endif /* UNIMAC */
}

/**************************************************************************************************************/

static inline MV_STATUS mv_gtw_update_rx_skb(MV_PKT_INFO *pkt_info, struct sk_buff *skb, unsigned int *if_num)
{
	mv_priv *mvp;
	struct net_device_stats *stats;
	unsigned char if_index;

	skb->len=0;
	skb_put(skb, pkt_info->pktSize - 4);

#ifdef CONFIG_MV_GTW_HEADER_MODE

#ifdef CONFIG_ETH_FLOW_CONTROL
	/* Received a flow control pause frame from the switch (without a header) */
	if ((skb->data[12] == 0x88) && (skb->data[13] == 0x08)) {
	    return MV_ERROR;
	}
#endif /* CONFIG_ETH_FLOW_CONTROL */

	/* use hedaer to find interface */
	if_index = ( (skb->data[0] & 0xF0) >> 4); /* get DBNum[3:0] from high nibble of first byte of the header */

#if 0 
printk("RX: [HDR = %02x %02x] [DA = %02x %02x %02x %02x %02x %02x] [SA = %02x %02x %02x %02x %02x %02x] [TYPE = %02x %02x %02x %02x]\n",
*(skb->data), *(skb->data+1), *(skb->data+2), *(skb->data+3), *(skb->data+4), *(skb->data+5),
*(skb->data+6), *(skb->data+7), *(skb->data+8), *(skb->data+9), *(skb->data+10), *(skb->data+11),
*(skb->data+12), *(skb->data+13), *(skb->data+14), *(skb->data+15),
*(skb->data+16), *(skb->data+17));

printk("mv_gtw_update_rx_skb: if_index = %d, pkt_info->pktSize = %d, skb->len = %d\n",
            if_index, pkt_info->pktSize, skb->len);
#endif

    /* skip on 2B Marvell-header */
    skb_pull(skb, QD_HEADER_SIZE);

#else /* not CONFIG_MV_GTW_HEADER_MODE, meaning we expect the packet to have a VLAN tag added by the switch */

#ifndef CONFIG_MV_INCLUDE_UNM_ETH
	/* skip on 2B GbE IP-alignment */
	skb_pull(skb, 2);
#endif

	/* disregard any packets that do not have a 802.1Q VLAN tag - make */
	/* sure that 0x8100 appears right after the source MAC address     */
	if (unlikely((*(skb->data+12) != 0x81) || (*(skb->data+13) != 0x00))) {
	    GTW_DBG( GTW_DBG_VLAN, ("Packet is not 802.1Q VLAN-tagged\n") );
	    return MV_ERROR;
	}

	/* find our net_device by the vid group id from vlan-tag */
	if_index = MV_GTW_VLANID_TO_GROUP((skb->data[14]<<8) | skb->data[15]);

#if 0
printk("RX: [DA = %02x %02x %02x %02x %02x %02x] [SA = %02x %02x %02x %02x %02x %02x] [VLAN = %02x %02x %02x %02x] [TYPE = %02x %02x %02x %02x]\n",
*(skb->data), *(skb->data+1), *(skb->data+2), *(skb->data+3), *(skb->data+4), *(skb->data+5),
*(skb->data+6), *(skb->data+7), *(skb->data+8), *(skb->data+9), *(skb->data+10), *(skb->data+11),
*(skb->data+12), *(skb->data+13), *(skb->data+14), *(skb->data+15),
*(skb->data+16), *(skb->data+17), *(skb->data+18), *(skb->data+19));
#endif

	/* remove the vlan tag */
	memmove(skb->data+QD_VLANTAG_SIZE,skb->data,12);
	skb_pull(skb, QD_VLANTAG_SIZE);	

	mv_gtw_align_ip_header(skb);
#endif

	if(unlikely(if_index >= num_of_ifs)) {
		return MV_ERROR;
	}

	*if_num = if_index;

	mvp = &mv_privs[if_index];
	stats = &mvp->net_dev_stat;
	stats->rx_packets++;
	stats->rx_bytes += pkt_info->pktSize;
	skb->dev = mvp->net_dev;

	return MV_OK;
}

/**************************************************************************************************************/

static inline void mv_gtw_update_tx_skb(unsigned int *switch_info, struct mv_vlan_cfg *vlan_cfg)
{
	MV_BUF_INFO *p_buf_info_first = eth_dev.tx_pkt_info.pFrags;
	MV_BUF_INFO *p_buf_info_last = eth_dev.tx_pkt_info.pFrags + eth_dev.tx_pkt_info.numFrags - 1;

#ifdef CONFIG_MV_GTW_HEADER_MODE
	struct sk_buff *skb = (struct sk_buff *)eth_dev.tx_pkt_info.osInfo;
	unsigned short header;

	/* build the header */
	header = cpu_to_be16((MV_GTW_VLANID_TO_GROUP(vlan_cfg->vlan_grp_id) << 12) /*| (1<<8)*/ | vlan_cfg->ports_mask);

	/* check if we have place inside skb for the header */
	if(skb_headroom(skb) >= QD_HEADER_SIZE) {
		*(unsigned short *)((skb->data)-QD_HEADER_SIZE) = header;
		p_buf_info_first->bufVirtPtr -= QD_HEADER_SIZE;
		p_buf_info_first->bufSize += QD_HEADER_SIZE;
#if 0
{
        unsigned char *data = eth_dev.tx_pkt_info.pFrags->bufVirtPtr;
        printk("TX: [HDR = %02x %02x] [DA = %02x %02x %02x %02x %02x %02x] [SA = %02x %02x %02x %02x %02x %02x]\n",
        *(data), *(data+1), *(data+2), *(data+3), *(data+4), *(data+5),
        *(data+6), *(data+7), *(data+8), *(data+9), *(data+10), *(data+11), *(data+12), *(data+13));
}
#endif
	}
	else {
	 	/* make room for one cell (safe because the array is big enough) */
		do {
	            *(p_buf_info_last+1) = *p_buf_info_last;
        	    p_buf_info_last--;
		} while(p_buf_info_last >= p_buf_info_first);

		/* the header (safe on stack) */
		*switch_info = header;
		p_buf_info_first->bufVirtPtr = (unsigned char *)switch_info;
		p_buf_info_first->bufSize = QD_HEADER_SIZE;
		//printk("switch_info=0x%x first->virt=0x%x\n",*switch_info,p_buf_info_first->bufVirtPtr);

		/* count the new frags */
		eth_dev.tx_pkt_info.numFrags += 1;
		eth_dev.tx_pkt_info.pktSize += QD_HEADER_SIZE;
#if 0
{
        unsigned char *data = eth_dev.tx_pkt_info.pFrags->bufVirtPtr;
        printk("TX: [HDR = %02x %02x] ", *(data), *(data+1));
        data = (eth_dev.tx_pkt_info.pFrags+1)->bufVirtPtr;
        printk("[DA = %02x %02x %02x %02x %02x %02x] [SA = %02x %02x %02x %02x %02x %02x]\n",
        *(data), *(data+1), *(data+2), *(data+3), *(data+4), *(data+5),
        *(data+6), *(data+7), *(data+8), *(data+9), *(data+10), *(data+11));
}
#endif
	}

#else /* not CONFIG_MV_GTW_HEADER_MODE, meaning we have to build a VLAN tag and add it to the packet */

	/* add vlan tag by breaking first frag to three parts: */
    /* (1) DA+SA in skb (2) the new tag (3) rest of skb    */
    if(unlikely(eth_dev.tx_pkt_info.pFrags->bufSize < 12)) {
        /* never happens, but for completeness */
        printk(KERN_ERR "mv_gtw cannot transmit first frag when len < 12\n");
    }
    else {
 	/* make room for two cells (safe because the array is big enough) */
        while(p_buf_info_last != p_buf_info_first) {
	    *(p_buf_info_last+2) = *p_buf_info_last;
	    p_buf_info_last--;
	}

	/* the second half of the original first frag */
	(p_buf_info_first+2)->bufSize = p_buf_info_first->bufSize - 12;
	(p_buf_info_first+2)->bufVirtPtr = p_buf_info_first->bufVirtPtr + 12;

	/* the tag (safe on stack) */
	*switch_info = cpu_to_be32((0x8100 << 16) | vlan_cfg->vlan_grp_id);
	(p_buf_info_first+1)->bufVirtPtr = (unsigned char *)switch_info;
	(p_buf_info_first+1)->bufSize = QD_VLANTAG_SIZE;

	/* the first half of the original first frag */
	p_buf_info_first->bufSize = 12;

	/* count the new frags */
	eth_dev.tx_pkt_info.numFrags += 2;
	eth_dev.tx_pkt_info.pktSize += QD_VLANTAG_SIZE;
    	GTW_DBG(GTW_DBG_VLAN, ("%s TX: q=%d, vid=0x%x\n",dev->name,queue,vlan_cfg->vlan_grp_id));
    }

#if 0
{
	unsigned char *data = eth_dev.tx_pkt_info.pFrags->bufVirtPtr;
	printk("TX: [DA = %02x %02x %02x %02x %02x %02x] [SA = %02x %02x %02x %02x %02x %02x] ", 
	*(data), *(data+1), *(data+2), *(data+3), *(data+4), *(data+5),
	*(data+6), *(data+7), *(data+8), *(data+9), *(data+10), *(data+11));
	data = (eth_dev.tx_pkt_info.pFrags+1)->bufVirtPtr;
	printk("[VLAN = %02x %02x %02x %02x] ", *(data), *(data+1), *(data+2), *(data+3));
	data = (eth_dev.tx_pkt_info.pFrags+2)->bufVirtPtr;
	printk("[TYPE = %02x %02x %02x %02x]\n", *(data), *(data+1), *(data+2), *(data+3));
}
#endif

#endif
}

/**************************************************************************************************************/

static inline void mv_gtw_read_interrupt_cause_regs(void)
{
    eth_dev.picr = (MV_REG_READ(ETH_INTR_CAUSE_REG(ETH_DEF_PORT)) & MV_REG_READ(ETH_INTR_MASK_REG(ETH_DEF_PORT)));
#if defined(CONFIG_MV_INCLUDE_UNM_ETH) /* UNIMAC */
    eth_dev.picer = 0; /* Not available in UniMAC HW */
#else /* not UNIMAC */
    if(eth_dev.picr & BIT1) 
	eth_dev.picer = MV_REG_READ(ETH_INTR_CAUSE_EXT_REG(ETH_DEF_PORT)) & MV_REG_READ(ETH_INTR_MASK_EXT_REG(ETH_DEF_PORT));
    else
        eth_dev.picer = 0;
#endif /* UNIMAC */
}

/**************************************************************************************************************/

static inline void mv_gtw_save_clear_rx_tx_cause(void)
{ 
    /* save rx cause */
    eth_dev.rxcause |= (eth_dev.picr & ETH_RXQ_MASK) | 
			((eth_dev.picr & ETH_RXQ_RES_MASK) >> (ETH_CAUSE_RX_ERROR_OFFSET - ETH_CAUSE_RX_READY_OFFSET));
#if defined(CONFIG_MV_INCLUDE_UNM_ETH) /* UNIMAC */
    /* save tx cause */
    eth_dev.txcause |= (eth_dev.picr & ETH_TXQ_MASK);

    /* Clear the specific interrupts according to the bits currently set in the cause_regs */
    MV_REG_WRITE(ETH_INTR_CAUSE_REG(ETH_DEF_PORT), ~(eth_dev.picr) );
#else /* not UNIMAC */
    /* save tx cause */
    eth_dev.txcause |= (eth_dev.picer & ETH_TXQ_MASK);

    /* Clear the specific interrupts according to the bits currently set in the cause_regs */
    MV_REG_WRITE(ETH_INTR_CAUSE_REG(ETH_DEF_PORT), ~(eth_dev.picr));
    if(eth_dev.picr & BIT1) {
	if(eth_dev.picer)
	    MV_REG_WRITE(ETH_INTR_CAUSE_EXT_REG(ETH_DEF_PORT), ~(eth_dev.picer));
    }
#endif /* UNIMAC */
}

/**************************************************************************************************************/

static inline void mv_gtw_print_int_while_polling(void)
{
    printk("Interrupt while in polling list\n");
    printk("Interrupt Cause Register = 0x%08x\n", eth_dev.picr);
    printk("Interrupt Mask Register = 0x%08x\n", MV_REG_READ(ETH_INTR_MASK_REG(ETH_DEF_PORT)));
#ifndef CONFIG_MV_INCLUDE_UNM_ETH
    printk("Interrupt Cause Extend Register = 0x%08x\n", eth_dev.picer);
    printk("Interrupt Mask Extend Register = 0x%08x\n", MV_REG_READ(ETH_INTR_MASK_EXT_REG(ETH_DEF_PORT)));
#endif
}

/**************************************************************************************************************/

static inline void mv_gtw_read_save_clear_tx_cause(void)
{
#if defined(CONFIG_MV_INCLUDE_UNM_ETH) /* UNIMAC */
    eth_dev.txcause |= (MV_REG_READ(ETH_INTR_CAUSE_REG(ETH_DEF_PORT)) & ETH_TXQ_MASK);
    MV_REG_WRITE(ETH_INTR_CAUSE_REG(ETH_DEF_PORT), ~(eth_dev.txcause & ETH_TXQ_MASK));
#else /* not UNIMAC */
    eth_dev.txcause |= MV_REG_READ(ETH_INTR_CAUSE_EXT_REG(ETH_DEF_PORT));
    MV_REG_WRITE(ETH_INTR_CAUSE_EXT_REG(ETH_DEF_PORT), eth_dev.txcause & (~ETH_TXQ_MASK) );
#endif /* UNIMAC */
}

/**************************************************************************************************************/

static inline void mv_gtw_read_save_clear_rx_cause(void)
{
#if defined(CONFIG_MV_INCLUDE_UNM_ETH) /* UNIMAC */
    /* The Interrupt Cause Register is not updated with new events, if they are masked  */
    /* in the Interrupt Mask Register. That is why we set eth_dev.rxcause to the entire */
    /* ETH_RXQ_MASK so that we go over all the Rx queues.                               */
    eth_dev.rxcause = ETH_RXQ_MASK;
    MV_REG_WRITE(ETH_INTR_CAUSE_REG(ETH_DEF_PORT), 
		~(eth_dev.rxcause | (eth_dev.rxcause << (ETH_CAUSE_RX_ERROR_OFFSET - ETH_CAUSE_RX_READY_OFFSET)) ) );
#else /* not UNIMAC */
    unsigned int temp = MV_REG_READ(ETH_INTR_CAUSE_REG(ETH_DEF_PORT));
    eth_dev.rxcause |= temp & ETH_RXQ_MASK;
    eth_dev.rxcause |= (temp & ETH_RXQ_RES_MASK) >> (ETH_CAUSE_RX_ERROR_OFFSET - ETH_CAUSE_RX_READY_OFFSET);
    MV_REG_WRITE(ETH_INTR_CAUSE_REG(ETH_DEF_PORT), 
		~(eth_dev.rxcause | (eth_dev.rxcause << (ETH_CAUSE_RX_ERROR_OFFSET - ETH_CAUSE_RX_READY_OFFSET)) ) );
#endif /* UNIMAC */
}

/**************************************************************************************************************/

static inline void mv_gtw_eth_mask_interrupts(int port)
{
    MV_REG_WRITE( ETH_INTR_MASK_REG(port), 0 );
#ifndef CONFIG_MV_INCLUDE_UNM_ETH
    MV_REG_WRITE( ETH_INTR_MASK_EXT_REG(port), 0 );
#endif
}

/**************************************************************************************************************/

static inline void mv_gtw_unmask_interrupts(int port, MV_U32 rx_interrupts, MV_U32 tx_interrupts)
{
#if defined(CONFIG_MV_INCLUDE_UNM_ETH) /* UNIMAC */
    /* unmask UniMAC Rx and Tx interrupts */
    MV_REG_WRITE( ETH_INTR_MASK_REG(port), (rx_interrupts|tx_interrupts) );
#else /* not UNIMAC */
    /* unmask GbE Rx and Tx interrupts */
    MV_REG_WRITE( ETH_INTR_MASK_REG(port), rx_interrupts);
    MV_REG_WRITE( ETH_INTR_MASK_EXT_REG(port), tx_interrupts);    
#endif /* UNIMAC */
}

/**************************************************************************************************************/

static inline void mv_gtw_clear_interrupts(int port)
{
    /* clear interrupts */
    MV_REG_WRITE( ETH_INTR_CAUSE_REG(port), 0 );
#ifndef CONFIG_MV_INCLUDE_UNM_ETH
    /* clear Tx interrupts */
    MV_REG_WRITE( ETH_INTR_CAUSE_EXT_REG(port), 0 );
#endif
}

/**************************************************************************************************************/

void print_qd_port_counters(unsigned int port)
{
#if defined(CONFIG_MV_INCLUDE_UNM_ETH) /* UNIMAC */
    GT_PORT_STAT counters;

    printk("Getting QD counters for port %d.\n", port);

    if(gprtGetPortCtr(qd_dev,port,&counters) != GT_OK) {
    	printk("gprtGetPortCtr failed");
	return;
    }

    printk("rxCtr    %u    ", counters.rxCtr);
    printk("txCtr    %u    ", counters.txCtr);
    printk("dropped  %u    \n", counters.dropped);
    
    return;
#else /* not UNIMAC */
    GT_STATS_COUNTER_SET3 counters;

    printk("Getting QD counters for port %d.\n", port);

    if(gstatsGetPortAllCounters3(qd_dev,port,&counters) != GT_OK) {
    	printk("gstatsGetPortAllCounters3 failed");
	return;
    }

    printk("InGoodOctetsLo  %lu    ", counters.InGoodOctetsLo);
    printk("InGoodOctetsHi  %lu   \n", counters.InGoodOctetsHi);
    printk("InBadOctets     %lu    ", counters.InBadOctets);
    printk("OutFCSErr       %lu   \n", counters.OutFCSErr);
    printk("Deferred        %lu   \n", counters.Deferred);
    printk("InBroadcasts    %lu    ", counters.InBroadcasts);
    printk("InMulticasts    %lu   \n", counters.InMulticasts);
    printk("Octets64        %lu    ", counters.Octets64);
    printk("Octets127       %lu   \n", counters.Octets127);
    printk("Octets255       %lu    ", counters.Octets255);
    printk("Octets511       %lu   \n", counters.Octets511);
    printk("Octets1023      %lu    ", counters.Octets1023);
    printk("OctetsMax       %lu   \n", counters.OctetsMax);
    printk("OutOctetsLo     %lu    ", counters.OutOctetsLo);
    printk("OutOctetsHi     %lu   \n", counters.OutOctetsHi);
    printk("OutUnicasts     %lu    ", counters.OutOctetsHi);
    printk("Excessive       %lu   \n", counters.Excessive);
    printk("OutMulticasts   %lu    ", counters.OutMulticasts);
    printk("OutBroadcasts   %lu   \n", counters.OutBroadcasts);
    printk("Single          %lu    ", counters.OutBroadcasts);
    printk("OutPause        %lu   \n", counters.OutPause);
    printk("InPause         %lu    ", counters.InPause);
    printk("Multiple        %lu   \n", counters.InPause);
    printk("Undersize       %lu    ", counters.Undersize);
    printk("Fragments       %lu   \n", counters.Fragments);
    printk("Oversize        %lu    ", counters.Oversize);
    printk("Jabber          %lu   \n", counters.Jabber);
    printk("InMACRcvErr     %lu    ", counters.InMACRcvErr);
    printk("InFCSErr        %lu   \n", counters.InFCSErr);
    printk("Collisions      %lu    ", counters.Collisions);
    printk("Late            %lu   \n", counters.Late);

    return;
#endif /* UNIMAC */
}

/**************************************************************************************************************/

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
    printk("\t mac=%p, nh=%p, h=%p\n", skb->mac.raw, skb->nh.iph, skb->h.th);
    printk("\t dataref=0x%x, nr_frags=%d, tso_size=%d, tso_segs=%d, frag_list=%p\n",
            atomic_read(&skb_shinfo(skb)->dataref), skb_shinfo(skb)->nr_frags, skb_shinfo(skb)->tso_size,
            skb_shinfo(skb)->tso_segs, skb_shinfo(skb)->frag_list);
    for(i=0; i<skb_shinfo(skb)->nr_frags; i++)
    {
        printk("\t frag_%d. page=%p, page_offset=0x%x, size=%d\n",
            i, page_address(skb_shinfo(skb)->frags[i].page), 
            skb_shinfo(skb)->frags[i].page_offset & 0xFFFF, 
            skb_shinfo(skb)->frags[i].size & 0xFFFF);
    }
    if( (skb->protocol == ntohs(ETH_P_IP)) && (skb->nh.iph != NULL) )
    {
        print_iph(skb->nh.iph);
        if(skb->nh.iph->protocol == IPPROTO_TCP)
            print_tcph(skb->h.th);
    }
    printk("\n");
}

/**************************************************************************************************************/


