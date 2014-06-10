/*******************************************************************************
*                Copyright 2003, MARVELL SEMICONDUCTOR, LTD.                   *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL.                      *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
*                                                                              *
* MARVELL COMPRISES MARVELL TECHNOLOGY GROUP LTD. (MTGL) AND ITS SUBSIDIARIES, *
* MARVELL INTERNATIONAL LTD. (MIL), MARVELL TECHNOLOGY, INC. (MTI), MARVELL    *
* SEMICONDUCTOR, INC. (MSI), MARVELL ASIA PTE LTD. (MAPL), MARVELL JAPAN K.K.  *
* (MJKK), MARVELL ISRAEL LTD. (MSIL).  										   *
********************************************************************************
* mvEthRegs.h - Header File for : Marvell UniMAC Fast Ethernet Controller registers 
*
* DESCRIPTION:
*       This header file contains macros typedefs and function declaration for
*       the Marvell UniMAC Fast Ethernet Controller registers. 
*
* DEPENDENCIES:
*       None.
*
*******************************************************************************/

#ifndef __INCethRegsh
#define __INCethRegsh

#define ETH_PORTS_GAP					0
#define ETH_PORTS_BASE_ADDR				0x78000
#define ETH_PORTS_WINDOWS_BASE_ADDR			0x74000

/* UniMAC Fast Ethernet Controller registers */
#define ETH_SMI_REG             			(ETH_PORTS_BASE_ADDR + 0x010)
#define ETH_PORT_CONFIG_REG(ethPortNum)         	(ETH_PORTS_BASE_ADDR + (ETH_PORTS_GAP*(ethPortNum) + 0x400))
#define ETH_PORT_CONFIG_EXTEND_REG(ethPortNum)		(ETH_PORTS_BASE_ADDR + (ETH_PORTS_GAP*(ethPortNum) + 0x408))
#define ETH_PORT_COMMAND_REG(ethPortNum)   		(ETH_PORTS_BASE_ADDR + (ETH_PORTS_GAP*(ethPortNum) + 0x410))
#define ETH_PORT_STATUS_REG(ethPortNum)    		(ETH_PORTS_BASE_ADDR + (ETH_PORTS_GAP*(ethPortNum) + 0x418))
#define ETH_HASH_TABLE_BASE_ADDR_REG(ethPortNum)    	(ETH_PORTS_BASE_ADDR + (ETH_PORTS_GAP*(ethPortNum) + 0x428))
#define ETH_SDMA_CONFIG_REG(ethPortNum)    		(ETH_PORTS_BASE_ADDR + (ETH_PORTS_GAP*(ethPortNum) + 0x440))
#define ETH_SDMA_COMMAND_REG(ethPortNum)   		(ETH_PORTS_BASE_ADDR + (ETH_PORTS_GAP*(ethPortNum) + 0x448))
#define ETH_INTR_CAUSE_REG(ethPortNum)           	(ETH_PORTS_BASE_ADDR + (ETH_PORTS_GAP*(ethPortNum) + 0x450))
#define ETH_RESET_SELECT_REG(ethPortNum)    		(ETH_PORTS_BASE_ADDR + (ETH_PORTS_GAP*(ethPortNum) + 0x454))
#define ETH_INTR_MASK_REG(ethPortNum)            	(ETH_PORTS_BASE_ADDR + (ETH_PORTS_GAP*(ethPortNum) + 0x458))
#define ETH_VLAN_TAG_TO_PRIO_REG(ethPortNum)		(ETH_PORTS_BASE_ADDR + (ETH_PORTS_GAP*(ethPortNum) + 0x470))

#define FIRST_RX_DESC_PTR0(ethPortNum)      		(ETH_PORTS_BASE_ADDR + (ETH_PORTS_GAP*(ethPortNum) + 0x480))
#define FIRST_RX_DESC_PTR1(ethPortNum)      		(ETH_PORTS_BASE_ADDR + (ETH_PORTS_GAP*(ethPortNum) + 0x484))
#define FIRST_RX_DESC_PTR2(ethPortNum)      		(ETH_PORTS_BASE_ADDR + (ETH_PORTS_GAP*(ethPortNum) + 0x488))
#define FIRST_RX_DESC_PTR3(ethPortNum)      		(ETH_PORTS_BASE_ADDR + (ETH_PORTS_GAP*(ethPortNum) + 0x48c))

#define CURR_RX_DESC_PTR0(ethPortNum)       		(ETH_PORTS_BASE_ADDR + (ETH_PORTS_GAP*(ethPortNum) + 0x4a0))
#define CURR_RX_DESC_PTR1(ethPortNum)       		(ETH_PORTS_BASE_ADDR + (ETH_PORTS_GAP*(ethPortNum) + 0x4a4))
#define CURR_RX_DESC_PTR2(ethPortNum)       		(ETH_PORTS_BASE_ADDR + (ETH_PORTS_GAP*(ethPortNum) + 0x4a8))
#define CURR_RX_DESC_PTR3(ethPortNum)       		(ETH_PORTS_BASE_ADDR + (ETH_PORTS_GAP*(ethPortNum) + 0x4ac))

#define CURR_TX_DESC_PTR0(ethPortNum)       		(ETH_PORTS_BASE_ADDR + (ETH_PORTS_GAP*(ethPortNum) + 0x4e0))
#define CURR_TX_DESC_PTR1(ethPortNum)       		(ETH_PORTS_BASE_ADDR + (ETH_PORTS_GAP*(ethPortNum) + 0x4e4))

#define FIRST_RX_DESC_PTR(ethPort, rxQueue)             \
            (ETH_PORTS_BASE_ADDR + (ETH_PORTS_GAP*(ethPort) + 4*(rxQueue) + 0x480))
#define CURR_RX_DESC_PTR(ethPort, rxQueue)              \
            (ETH_PORTS_BASE_ADDR + (ETH_PORTS_GAP*(ethPort) + 4*(rxQueue) + 0x4a0))
#define CURR_TX_DESC_PTR(ethPort, txQueue)              \
            (ETH_PORTS_BASE_ADDR + (ETH_PORTS_GAP*(ethPort) + 4*(txQueue) + 0x4e0))

#define ETH_WIN_CONTROL_REG(win)				\
			(ETH_PORTS_WINDOWS_BASE_ADDR + 0x10*(win) + 0x30)
#define ETH_WIN_BASE_REG(win)					\
			(ETH_PORTS_WINDOWS_BASE_ADDR + 0x10*(win) + 0x34)


/*******************************************************************************/

/* Ethernet SMI Register (SMIR) */
#define ETH_SMI_DATA_OFFSET			0
#define ETH_SMI_DATA_MASK			(0xFF<<ETH_SMI_DATA_OFFSET)
#define ETH_SMI_PHY_AD_OFFSET			16
#define ETH_SMI_PHY_AD_MASK			(0x1F<<ETH_SMI_PHY_AD_OFFSET)
#define ETH_SMI_REG_AD_OFFSET			21
#define ETH_SMI_REG_AD_MASK			(0x1F<<ETH_SMI_REG_AD_OFFSET)
#define ETH_SMI_OPCODE_OFFSET			26
#define ETH_SMI_OPCODE_MASK			(1<<ETH_SMI_OPCODE_OFFSET)
#define ETH_SMI_READ_VALID_OFFSET		27
#define ETH_SMI_READ_VALID_MASK			(1<<ETH_SMI_READ_VALID_OFFSET)
#define ETH_SMI_BUSY_OFFSET			28
#define ETH_SMI_BUSY_MASK			(1<<ETH_SMI_BUSY_OFFSET)


/* Ethernet Port Status Register (PSR) */
#define ETH_PAUSE_BIT				4
#define ETH_PAUSE_MASK				(1<<ETH_PAUSE_BIT)
#define ETH_TX_STATUS_LOW_BIT			5
#define ETH_TX_STATUS_LOW_MASK			(1<<ETH_TX_STATUS_LOW_BIT)
#define ETH_TX_STATUS_HIGH_BIT			6
#define ETH_TX_STATUS_HIGH_MASK			(1<<ETH_TX_STATUS_HIGH_BIT)
#define ETH_TX_IN_PROGRESS_BIT			7
#define ETH_TX_IN_PROGRESS_MASK			(1<<ETH_TX_IN_PROGRESS_BIT)


/* SDMA Configuration Register (SDCR) */
#define ETH_RX_NO_DATA_SWAP_BIT			6
#define ETH_RX_NO_DATA_SWAP_MASK		(1<<ETH_RX_NO_DATA_SWAP_BIT)
#define ETH_RX_DATA_SWAP_MASK			(0<<ETH_RX_NO_DATA_SWAP_BIT)
#define ETH_TX_NO_DATA_SWAP_BIT			7
#define ETH_TX_NO_DATA_SWAP_MASK		(1<<ETH_TX_NO_DATA_SWAP_BIT)
#define ETH_TX_DATA_SWAP_MASK			(0<<ETH_TX_NO_DATA_SWAP_BIT)
#define ETH_RX_FRAME_INTERRUPT_BIT		9
#define ETH_RX_FRAME_INTERRUPT_MASK		(1<<ETH_RX_FRAME_INTERRUPT_BIT)
#define ETH_BURST_SIZE_OFFSET           	12
#define ETH_BURST_SIZE_ALL_MASK         	(3<<ETH_BURST_SIZE_OFFSET)
#define ETH_BURST_SIZE_MASK(burst)      	((burst)<<ETH_BURST_SIZE_OFFSET)

#define ETH_BURST_SIZE_8_32BIT_VALUE		3

/* Ethernet Port Configuration Register (PCR) */
#define ETH_UNICAST_PROMISCUOUS_MODE_BIT    	0 /* Unicast Promiscuous Mode */
#define ETH_UNICAST_PROMISCUOUS_MODE_MASK   	(1<<ETH_UNICAST_PROMISCUOUS_MODE_BIT)
#define ETH_REJECT_BROADCAST_BIT		1 /* Reject Broadcast Mode */
#define ETH_REJECT_BROADCAST_MASK		(1<<ETH_REJECT_BROADCAST_BIT)
#define ETH_MULTICAST_PROMISCUOUS_MODE_BIT	3 /* Multicast Promiscuous Mode */
#define ETH_MULTICAST_PROMISCUOUS_MODE_MASK	(1<<ETH_MULTICAST_PROMISCUOUS_MODE_BIT)
#define ETH_PORT_ENABLE_BIT			7 /* Port Enable */
#define ETH_PORT_ENABLE_MASK			(1<<ETH_PORT_ENABLE_BIT)
#define ETH_HASH_SIZE_500_BIT			12 /* Hash table size 0 - 8K, 1 - 0.5k */
#define ETH_HASH_SIZE_500_MASK			(1<<ETH_HASH_SIZE_500_BIT)
#define ETH_HASH_FUNCTION_1_BIT			13 /* Hash function 1 or function 0 */
#define ETH_HASH_FUNCTION_1_MASK		(1<<ETH_HASH_FUNCTION_1_BIT)
#define ETH_HASH_PASS_ADDRS_NOT_FOUND_BIT	14 /* Hash function operation mode */
#define ETH_HASH_PASS_ADDRS_NOT_FOUND_MASK 	(1<<ETH_HASH_PASS_ADDRS_NOT_FOUND_BIT)


/* Ethernet Port Configuration Extend Register (PCXR) */
#define ETH_IGMP_CAPTURE_ENABLE_BIT		0 /* IGMP Packets Capture Enable */
#define ETH_IGMP_CAPTURE_ENABLE_MASK		(1<<ETH_IGMP_CAPTURE_ENABLE_BIT)
#define ETH_BPDU_CAPTURE_ENABLE_BIT		1 /* Spanning Tree Packets Capture Enable */
#define ETH_BPDU_CAPTURE_ENABLE_MASK		(1<<ETH_BPDU_CAPTURE_ENABLE_BIT)
#define ETH_PRIO_TX_OFFSET			3
#define ETH_PRIO_TX_MASK			(7<<ETH_PRIO_TX_OFFSET)
#define ETH_DEF_PRIO_RX_OFFSET			6
#define ETH_DEF_PRIO_RX_MASK			(3<<ETH_DEF_PRIO_RX_OFFSET)
#define ETH_RX_PRIO_OVERRIDE_BIT		8 /* Override Priority Rx on this Port */
#define ETH_RX_PRIO_OVERRIDE_MASK		(1<<ETH_RX_PRIO_OVERRIDE_BIT)
#define ETH_FLOW_CTRL_DISABLE_BIT		12 /* Enable Flow-Control Mode */
#define ETH_FLOW_CTRL_DISABLE_MASK		(1<<ETH_FLOW_CTRL_DISABLE_BIT)
#define ETH_DSCP_ENABLE_BIT			21 /* DSCP enable */
#define ETH_DSCP_ENABLE_MASK			(1<<ETH_DSCP_ENABLE_BIT)			
#define ETH_HEADER_MODE_OFFSET			22 /* Marvell header mode setting */
#define ETH_HEADER_MODE_MASK			(3<<ETH_HEADER_MODE_OFFSET)
#define ETH_SPID_BIT				23
#define ETH_SPID_MASK				(1<<ETH_SPID_BIT)

/* initial value for ETH_PORT_CONFIG_REG with reserved bits set according to spec */
#define PORT_CFG_INIT_VALUE			0x09209000 

/* defualt value, with Marvell Header disabled , with DSCP enabled */
#define PORT_CONFIG_EXTEND_VALUE		0x000C4600

#define HASH_TABLE_MEM_SIZE			16384 /* 16 KByte */

/* Ethernet SDMA Command Register */
#define ETH_ENABLE_RX_DMA_BIT			7
#define ETH_ENABLE_RX_DMA_MASK			(1<<ETH_ENABLE_RX_DMA_BIT)
#define ETH_ABORT_RECEIVE_BIT			15
#define ETH_ABORT_RECEIVE_MASK			(1<<ETH_ABORT_RECEIVE_BIT)
#define ETH_STOP_TX_HIGH_BIT    		16
#define ETH_STOP_TX_HIGH_MASK			(1<<ETH_STOP_TX_HIGH_BIT)
#define ETH_STOP_TX_LOW_BIT     		17
#define ETH_STOP_TX_LOW_MASK			(1<<ETH_STOP_TX_LOW_BIT)
#define ETH_START_TX_HIGH_BIT   		23
#define ETH_START_TX_HIGH_MASK			(1<<ETH_START_TX_HIGH_BIT)
#define ETH_START_TX_LOW_BIT    		24
#define ETH_START_TX_LOW_MASK			(1<<ETH_START_TX_LOW_BIT)


/* Tx & Rx descriptor bits */
#define ETH_BUFFER_OWNER_BIT                	31             
#define ETH_BUFFER_OWNED_BY_DMA             	(1<<ETH_BUFFER_OWNER_BIT)
#define ETH_BUFFER_OWNED_BY_HOST            	(0<<ETH_BUFFER_OWNER_BIT)
#define ETH_AUTO_MODE_BIT			30
#define ETH_AUTO_MODE_MASK			(1<<ETH_AUTO_MODE_BIT)
#define ETH_ENABLE_INTERRUPT_BIT		23
#define ETH_ENABLE_INTERRUPT_MASK		(1<<ETH_ENABLE_INTERRUPT_BIT)
#define ETH_FIRST_BIT				17
#define ETH_FIRST_MASK				(1<<ETH_FIRST_BIT)
#define ETH_LAST_BIT				16
#define ETH_LAST_MASK				(1<<ETH_LAST_BIT)
#define ETH_ERROR_SUMMARY_BIT               	15
#define ETH_ERROR_SUMMARY_MASK              	(1<<ETH_ERROR_SUMMARY_BIT)


/* Tx descriptor bits */
#define ETH_TX_GENERATE_CRC_BIT             	22
#define ETH_TX_GENERATE_CRC_MASK            	(1<<ETH_TX_GENERATE_CRC_BIT)
#define ETH_TX_ZERO_PADDING_BIT             	18
#define ETH_TX_ZERO_PADDING_MASK            	(1<<ETH_TX_ZERO_PADDING_BIT)
#define ETH_TX_UNDERRUN_ERROR_BIT           	6
#define ETH_TX_UNDERRUN_ERROR_MASK		(11<ETH_TX_UNDERRUN_ERROR_BIT)


/* Rx descriptor bits */	
#define ETH_HASH_TABLE_EXPIRED_BIT		13
#define ETH_HASH_TABLE_EXPIRED_MASK		(1<<ETH_HASH_TABLE_EXPIRED_BIT)
#define ETH_MISSED_FRAME_BIT			12
#define ETH_MISSED_FRAME_MASK			(1<<ETH_MISSED_FRAME_BIT)
#define ETH_OVERRUN_ERROR_BIT			6
#define ETH_OVERRUN_ERROR_MASK			(1<<ETH_OVERRUN_ERROR_BIT)
#define ETH_CRC_ERROR_BIT			0
#define ETH_CRC_ERROR_MASK			(1<<ETH_CRC_ERROR_BIT)
#define ETH_RX_ERROR_CODE_MASK			(ETH_OVERRUN_ERROR_BIT | ETH_CRC_ERROR_BIT)

#define ETH_RX_BUFFER_MASK          		0xFFF8


/* Ethernet Interrupt Cause Register (ICR) */
#define ETH_CAUSE_RX_READY_SUM_BIT          	0
#define ETH_CAUSE_RX_READY_SUM_MASK		(1<<ETH_CAUSE_RX_READY_SUM_BIT)
#define ETH_CAUSE_TX_BUF_OFFSET             	3
#define ETH_CAUSE_TX_BUF_BIT(queue)         	(ETH_CAUSE_TX_BUF_OFFSET - (queue))
#define ETH_CAUSE_TX_BUF_MASK(queue)        	(1 << (ETH_CAUSE_TX_BUF_BIT(queue)))
#define ETH_CAUSE_TX_ERROR_OFFSET           	11
#define ETH_CAUSE_TX_ERROR_BIT(queue)       	(ETH_CAUSE_TX_ERROR_OFFSET - (queue))
#define ETH_CAUSE_TX_ERROR_MASK(queue)      	(1 << (ETH_CAUSE_TX_ERROR_BIT(queue)))
#define ETH_CAUSE_RX_OVERRUN_BIT		12
#define ETH_CAUSE_RX_OVERRUN_MASK		(1<<ETH_CAUSE_RX_OVERRUN_BIT)
#define ETH_CAUSE_TX_UNDERRUN_BIT		13
#define ETH_CAUSE_TX_UNDERRUN_MASK		(1<<ETH_CAUSE_TX_UNDERRUN_BIT)
#define ETH_CAUSE_TX_END_BIT(queue)         	(7-(queue))
#define ETH_CAUSE_RX_ERROR_SUM_BIT          	8
#define ETH_CAUSE_RX_ERROR_SUM_MASK		(1<<ETH_CAUSE_RX_ERROR_SUM_BIT)
#define ETH_CAUSE_RX_READY_OFFSET           	16
#define ETH_CAUSE_RX_READY_BIT(queue)       	(ETH_CAUSE_RX_READY_OFFSET + (queue))
#define ETH_CAUSE_RX_READY_MASK(queue)      	(1 << (ETH_CAUSE_RX_READY_BIT(queue))) 
#define ETH_CAUSE_RX_ERROR_OFFSET           	20
#define ETH_CAUSE_RX_ERROR_BIT(queue)       	(ETH_CAUSE_RX_ERROR_OFFSET + (queue))
#define ETH_CAUSE_RX_ERROR_MASK(queue)      	(1 << (ETH_CAUSE_RX_ERROR_BIT(queue))) 
#define ETH_CAUSE_SMI_DONE_BIT			29
#define ETH_CAUSE_SMI_DONE_MASK			(1<<ETH_CAUSE_SMI_DONE_BIT)


/* Ethernet Port Command Register (PCMR) */
#define ETH_FLOW_CONTROL_BIT			15
#define ETH_FLOW_CONTROL_MASK			(1<<ETH_FLOW_CONTROL_BIT)


/* Address Decode Parameters */
#define ETH_MAX_DECODE_WIN			4

/* Ethernet Window Control Registers */
#define ETH_WIN_EN_BIT				0
#define ETH_WIN_EN_MASK				(1<<ETH_WIN_EN_BIT)
#define ETH_WIN_TARGET_OFFSET			4
#define ETH_WIN_TARGET_MASK			(0xF<<ETH_WIN_TARGET_OFFSET)
#define ETH_WIN_ATTR_OFFSET			8
#define ETH_WIN_ATTR_MASK			(0xFF<<ETH_WIN_ATTR_OFFSET)
#define ETH_WIN_SIZE_OFFSET			16
#define ETH_WIN_SIZE_MASK			(0xFFFF<<ETH_WIN_SIZE_OFFSET)

/* Ethernet Window Base Registers */
#define ETH_WIN_BASE_ADDR_OFFSET		16
#define ETH_WIN_BASE_ADDR_MASK			(0xFFFF<<ETH_WIN_BASE_ADDR_OFFSET)

/* BITs of SDMA Descriptor Command/Status field */

typedef struct _ethRxDesc
{
    MV_U32      cmdSts     ;    /* Descriptor command status        */
    MV_U16      byteCnt    ;    /* Descriptor buffer byte count     */
    MV_U16      bufSize    ;    /* Buffer size                      */
    MV_U32      bufPtr     ;    /* Descriptor buffer pointer        */
    MV_U32      nextDescPtr;    /* Next descriptor pointer          */
    MV_ULONG    returnInfo ;    /* User resource return information */
} ETH_RX_DESC;

typedef struct _ethTxDesc
{
    MV_U32      cmdSts     ;    /* Descriptor command status        */
    MV_U16	reserved   ;    /* Reserved			    */
    MV_U16      byteCnt    ;    /* Descriptor buffer byte count     */
    MV_U32      bufPtr     ;    /* Descriptor buffer pointer        */
    MV_U32      nextDescPtr;    /* Next descriptor pointer          */
    MV_ULONG    returnInfo ;    /* User resource return information */
    MV_U8*      alignBufPtr;    /* Pointer to 32 byte aligned buffer */
} ETH_TX_DESC;

#endif /* __INCethRegsh */

