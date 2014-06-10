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
Marvell Commercial License Option

If you received this File from Marvell and you have entered into a commercial
license agreement (a "Commercial License") with Marvell, the File is licensed
to you under the terms of the applicable Commercial License.

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
********************************************************************************
Marvell BSD License Option

If you received this File from Marvell, you may opt to use, redistribute and/or 
modify this File under the following licensing terms. 
Redistribution and use in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met:

    *   Redistributions of source code must retain the above copyright notice,
        this list of conditions and the following disclaimer. 

    *   Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution. 

    *   Neither the name of Marvell nor the names of its contributors may be 
        used to endorse or promote products derived from this software without 
        specific prior written permission. 
    
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND 
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR 
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON 
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS 
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*******************************************************************************/

/*******************************************************************************
* mvEth.c - Marvell's UniMAC Fast Ethernet controller low level driver
*
* DESCRIPTION:
*       This file introduce OS independent APIs to Marvell's UniMAC Fast Ethernet
*       controller. This UniMAC Fast Ethernet Controller driver API controls
*       1) Operations (i.e. port Init, Finish, Up, Down, PhyReset etc').
*       2) Data flow (i.e. port Send, Receive etc').
*       3) Debug functions (ethPortRegs, ethPortCounters, ethPortQueues, etc.)
*       Each Ethernet port is controlled via ETH_PORT_CTRL struct.
*       This struct includes configuration information as well as driver
*       internal data needed for its operations.
*
*       Supported Features:
*       - OS independent. All required OS services are implemented via external 
*       OS dependent components. 
*       - The user is free from Rx/Tx queue managing.
*       - Simple Ethernet port operation API.
*       - Simple Ethernet port data flow API.
*       - Data flow and operation API support per queue functionality.
*       - Support cached descriptors for better performance.
*       - PHY access and control API.
*       - Port Configuration API.
*
*******************************************************************************/
/* includes */
#include "mvTypes.h"
#include "mv802_3.h"
#include "mvDebug.h"
#include "mvCommon.h"
#include "mvOs.h"
#include "mvCtrlEnvLib.h"
#include "mvEthPhy.h"
#include "mvEth.h"
#include "mvEthUnimac.h"
#include "mvCpu.h"

#ifdef INCLUDE_SYNC_BARR
#include "mvCpuIf.h"
#endif

#ifdef MV_RT_DEBUG
#   define ETH_DEBUG
#endif


/* This array holds the control structure of each port */
ETH_PORT_CTRL* ethPortCtrl[BOARD_ETH_PORT_NUM];

MV_BOOL         ethDescSwCoher = MV_TRUE;


/* Ethernet Port Local routines */
static void    ethInitRxDescRing(ETH_PORT_CTRL* pPortCtrl, int queue);
static void    ethInitTxDescRing(ETH_PORT_CTRL* pPortCtrl, int queue);
static void    ethBCopy(char* srcAddr, char* dstAddr, int byteCount);
static void    ethFreeDescrMemory(ETH_PORT_CTRL* pEthPortCtrl, MV_BUF_INFO* pDescBuf);
static MV_U8*  ethAllocDescrMemory(ETH_PORT_CTRL* pEthPortCtrl, int size, 
                                   MV_ULONG* pPhysAddr);

static INLINE MV_ULONG  ethDescVirtToPhy(ETH_QUEUE_CTRL* pQueueCtrl, MV_U8* pDesc)
{
    return (pQueueCtrl->descBuf.bufPhysAddr + (pDesc - pQueueCtrl->descBuf.bufVirtPtr)); 
}


/******************************************************************************/
/*                      EthDrv Initialization functions                       */
/******************************************************************************/

/*******************************************************************************
* mvEthInit - Initialize the UniMAC Fast Ethernet unit
*
* DESCRIPTION:
*       This function initialize the UniMAC Fast Ethernet unit.
*       1) Configure Address decode windows of the unit
*       2) Set registers to HW default values. 
*       3) Clear and Disable interrupts
*
* INPUT:  NONE
*
* RETURN: NONE
*
* NOTE: this function is called once in the boot process.
*******************************************************************************/
void    mvEthInit(void)
{
    int port;

    /* Init static data structures */
    for(port=0; port<BOARD_ETH_PORT_NUM; port++)
    {
        ethPortCtrl[port] = NULL;
    }

    /* Disable UniMAC Fast Ethernet Unit interrupts */
    MV_REG_WRITE(ETH_INTR_MASK_REG(ETH_PORT), 0);

    /* Using default window configuration */    
    mvEthWinInit();

    /* Clear interrupt cause register */
    MV_REG_WRITE(ETH_INTR_CAUSE_REG(ETH_PORT), 0);
}


/******************************************************************************/
/*                      Port Initialization functions                         */
/******************************************************************************/

/*******************************************************************************
* mvEthPortInit - Initialize the Ethernet port driver
*
* DESCRIPTION:
*       This function initialize the ethernet port.
*       1) Allocate and initialize internal port Control structure.
*       2) Create RX and TX descriptor rings for default RX and TX queues
*       3) Disable RX and TX operations, clear cause registers and 
*          mask all interrupts.
*       4) Set all registers to default values and clean all MAC tables. 
*
* INPUT:
*       int             portNo          - Ethernet port number
*       ETH_PORT_INIT   *pEthPortInit   - Ethernet port init structure
*
* RETURN:
*       void* - ethernet port handler, that should be passed to the most other
*               functions dealing with this port.
*
* NOTE: This function is called once per port when loading the eth module.
*******************************************************************************/
void*   mvEthPortInit(int portNo, MV_ETH_PORT_INIT *pEthPortInit)
{
    int             queue, descSize;
    ETH_PORT_CTRL*  pPortCtrl;
    
    /* Check validity of parameters */
    if( (portNo >= mvCtrlEthMaxPortGet()) || 
        (pEthPortInit->rxDefQ   >= MV_ETH_RX_Q_NUM) )
    {
        mvOsPrintf("EthPort #%d: Bad initialization parameters\n", portNo);
        return NULL;
    }
    if( (pEthPortInit->rxDescrNum[pEthPortInit->rxDefQ]) == 0)
    {
        mvOsPrintf("EthPort #%d: rxDefQ (%d) must be created\n", 
                    portNo, pEthPortInit->rxDefQ);
        return NULL;

    }
        
    pPortCtrl = (ETH_PORT_CTRL*)mvOsMalloc( sizeof(ETH_PORT_CTRL) );
    if(pPortCtrl == NULL)   
    {
       mvOsPrintf("ethDrv: Can't allocate %dB for port #%d control structure!\n",
                   (int)sizeof(ETH_PORT_CTRL), portNo);
       return NULL;
    }

    memset(pPortCtrl, 0, sizeof(ETH_PORT_CTRL) );
    ethPortCtrl[portNo] = pPortCtrl;

    pPortCtrl->portState = MV_UNDEFINED_STATE;

    pPortCtrl->portNo = portNo;

    /* Copy Configuration parameters */
    pPortCtrl->portConfig.rxDefQ = pEthPortInit->rxDefQ;

    for( queue=0; queue<MV_ETH_RX_Q_NUM; queue++ )
    {
        pPortCtrl->rxQueueConfig[queue].descrNum = pEthPortInit->rxDescrNum[queue];
    }
    for( queue=0; queue<MV_ETH_TX_Q_NUM; queue++ )
    {
        pPortCtrl->txQueueConfig[queue].descrNum = pEthPortInit->txDescrNum[queue];
    }

    mvEthPortDisable(pPortCtrl);

    /* Create all requested RX queues */
    for(queue=0; queue<MV_ETH_RX_Q_NUM; queue++)
    {
        if(pPortCtrl->rxQueueConfig[queue].descrNum == 0)
            continue;

        /* Allocate memory for RX descriptors */
        descSize = ((pPortCtrl->rxQueueConfig[queue].descrNum * ETH_RX_DESC_ALIGNED_SIZE) +
                                                        CPU_D_CACHE_LINE_SIZE);
 
        pPortCtrl->rxQueue[queue].descBuf.bufVirtPtr = 
                        ethAllocDescrMemory(pPortCtrl, descSize, 
                                    &pPortCtrl->rxQueue[queue].descBuf.bufPhysAddr);
        pPortCtrl->rxQueue[queue].descBuf.bufSize = descSize;
        if(pPortCtrl->rxQueue[queue].descBuf.bufVirtPtr == NULL)
        {
            mvOsPrintf("EthPort #%d, rxQ=%d: Can't allocate %d bytes in DRAM for %d RX descr\n", 
                        pPortCtrl->portNo, queue, descSize, 
                        pPortCtrl->rxQueueConfig[queue].descrNum);
            return NULL;
        }

        ethInitRxDescRing(pPortCtrl, queue);
    }

    /* Create TX queues */
    for(queue=0; queue<MV_ETH_TX_Q_NUM; queue++)
    {
        if(pPortCtrl->txQueueConfig[queue].descrNum == 0)
            continue;

        /* Allocate memory for TX descriptors */
        descSize = ((pPortCtrl->txQueueConfig[queue].descrNum * ETH_TX_DESC_ALIGNED_SIZE) +
                                                        CPU_D_CACHE_LINE_SIZE);
 
        pPortCtrl->txQueue[queue].descBuf.bufVirtPtr = ethAllocDescrMemory(pPortCtrl, 
                             descSize, &pPortCtrl->txQueue[queue].descBuf.bufPhysAddr);
        pPortCtrl->txQueue[queue].descBuf.bufSize = descSize;

        if(pPortCtrl->txQueue[queue].descBuf.bufVirtPtr == NULL)
        {
            mvOsPrintf("EthPort #%d, txQ=%d: Can't allocate %d bytes in DRAM for %d TX descr\n", 
                        pPortCtrl->portNo, queue, descSize, 
                        pPortCtrl->txQueueConfig[queue].descrNum);
            return NULL;
        }

        ethInitTxDescRing(pPortCtrl, queue);
    }

    if (mvEthDefaultsSet(pPortCtrl) != MV_OK)
	return NULL;

    if (mvEthHashTableInit(pPortCtrl) != MV_OK)
	return NULL;

    pPortCtrl->portState = MV_IDLE;
    return pPortCtrl;
}


/*******************************************************************************
* ethPortFinish - Finish the Ethernet port driver
*
* DESCRIPTION:
*       This function finish the ethernet port.
*       1) Down ethernet port if needed.
*       2) Delete RX and TX descriptor rings for all created RX and TX queues
*       3) Free internal port Control structure.
*
* INPUT:
*       void*   pEthPortHndl  - Ethernet port handler
*
* RETURN:   NONE.
*
*******************************************************************************/
void    mvEthPortFinish(void* pPortHndl)
{
    ETH_PORT_CTRL*  pPortCtrl = (ETH_PORT_CTRL*)pPortHndl;
    int             queue, portNo  = pPortCtrl->portNo;

    if(pPortCtrl->portState == MV_ACTIVE)
    {
        mvOsPrintf("ethPort #%d: Warning !!! Finish port in Active state\n",
                 portNo);
        mvEthPortDisable(pPortHndl);
    }

    /* Free all allocated RX queues */
    for(queue=0; queue<MV_ETH_RX_Q_NUM; queue++)
    {
        ethFreeDescrMemory(pPortCtrl, &pPortCtrl->rxQueue[queue].descBuf);
    }

    /* Free all allocated TX queues */
    for(queue=0; queue<MV_ETH_TX_Q_NUM; queue++)
    {
        ethFreeDescrMemory(pPortCtrl, &pPortCtrl->txQueue[queue].descBuf);
    }

    /* Free port control structure */
    mvOsFree(pPortCtrl);

    ethPortCtrl[portNo] = NULL;
}

/*******************************************************************************
* mvEthHashTableInit - Initialize the UniMAC Hash Table
*
* DESCRIPTION:
*       This function initializes the UniMAC Hash Table
*       
* INPUT:
*       void*   pEthPortHndl  - Ethernet port handler
*
* RETURN:   MV_STATUS  
*               MV_OK - Success, Others - Failure
*
*******************************************************************************/
MV_STATUS   mvEthHashTableInit(void* pPortHndl)
{
    ETH_PORT_CTRL*	pPortCtrl = (ETH_PORT_CTRL*)pPortHndl;
    int			ethPortNo = pPortCtrl->portNo;
    MV_ULONG		hashPtrPhys = 0;

    /* Set ETH_HASH_TABLE_BASE_ADDR_REG (in mv_gateway we work in promiscuous mode and don't use the hash table) */
    pPortCtrl->hashPtr = (MV_U32 *)mvOsIoCachedMalloc(NULL, HASH_TABLE_MEM_SIZE, &hashPtrPhys);
    if(pPortCtrl->hashPtr == NULL)   
    {
       mvOsPrintf("ethDrv: Can't allocate %d bytes for hash table!\n", HASH_TABLE_MEM_SIZE);
       return MV_OUT_OF_CPU_MEM;
    }
    memset(pPortCtrl->hashPtr, 0, HASH_TABLE_MEM_SIZE);
    MV_REG_WRITE(ETH_HASH_TABLE_BASE_ADDR_REG(ethPortNo), hashPtrPhys);

    return MV_OK;
}

/*******************************************************************************
* mvEthDefaultsSet - Set defaults to the ethernet port
*
* DESCRIPTION:
*       This function set default values to the ethernet port.
*       1) Clear Cause registers and Mask all interrupts
*       2) Clear all MAC tables
*       3) Set defaults to all registers
*       4) Reset all created RX and TX descriptors ring
*       5) Reset PHY
*
* INPUT:
*       void*   pEthPortHndl  - Ethernet port handler
*
* RETURN:   MV_STATUS  
*               MV_OK - Success, Others - Failure
* NOTE:
*   This function update all the port configuration except those set
*   Initialy by the OsGlue by MV_ETH_PORT_INIT.
*   This function can be called after portDown to return the port setting 
*   to defaults.
*******************************************************************************/
MV_STATUS   mvEthDefaultsSet(void* pPortHndl)
{
    int			ethPortNo, queue;
    ETH_PORT_CTRL*	pPortCtrl = (ETH_PORT_CTRL*)pPortHndl;
    ETH_QUEUE_CTRL*	pQueueCtrl;
    MV_U32          	txPrio;
    MV_U32		portCfgReg;
    MV_U32      	portCfgExtReg;
    MV_U32      	portSdmaCfgReg = 0;
 	
    ethPortNo = pPortCtrl->portNo;

    /* Clear Cause registers and mask all interrupts */
    MV_REG_WRITE(ETH_INTR_MASK_REG(ethPortNo),0);
    MV_REG_WRITE(ETH_RESET_SELECT_REG(ethPortNo),0);
    MV_REG_WRITE(ETH_INTR_CAUSE_REG(ethPortNo),0);

    portCfgReg		= PORT_CFG_INIT_VALUE; 
    portCfgExtReg	= PORT_CONFIG_EXTEND_VALUE;
  
    /* build PORT_SDMA_CONFIG_REG */
    portSdmaCfgReg = (ETH_BURST_SIZE_MASK(ETH_BURST_SIZE_8_32BIT_VALUE) | 
                      ETH_RX_FRAME_INTERRUPT_MASK                       | 
                      ETH_RX_NO_DATA_SWAP_MASK                          | 
                      ETH_TX_NO_DATA_SWAP_MASK);
    
    /* not used in UniMAC */
    pPortCtrl->portRxQueueCmdReg = 0;
    pPortCtrl->portTxQueueCmdReg = 0;
        
    /* Update value of PortConfig register accordingly with all RxQueue types */
    pPortCtrl->portConfig.rxArpQ = pPortCtrl->portConfig.rxDefQ;
    pPortCtrl->portConfig.rxBpduQ = pPortCtrl->portConfig.rxDefQ; 
    pPortCtrl->portConfig.rxTcpQ = pPortCtrl->portConfig.rxDefQ; 
    pPortCtrl->portConfig.rxUdpQ = pPortCtrl->portConfig.rxDefQ; 
  
    /* Assignment of Tx CTRP of given queue */
    txPrio = 0;
    for(queue=0; queue<MV_ETH_TX_Q_NUM; queue++)
    {
        pQueueCtrl = &pPortCtrl->txQueue[queue];

        if(pQueueCtrl->pFirstDescr != NULL)
        {
            ethResetTxDescRing(pPortCtrl, queue);
        }
    }

    /* Assignment of Rx CRDP of given queue */
    for(queue=0; queue<MV_ETH_RX_Q_NUM; queue++)
    {
        ethResetRxDescRing(pPortCtrl, queue);
    }

    /* Assign port configuration register initial value */
    MV_REG_WRITE(ETH_PORT_CONFIG_REG(ethPortNo), portCfgReg);

    /* Assign port configuration extend register */
    MV_REG_WRITE(ETH_PORT_CONFIG_EXTEND_REG(ethPortNo), portCfgExtReg);

    /* Assign port SDMA configuration */
    MV_REG_WRITE(ETH_SDMA_CONFIG_REG(ethPortNo), portSdmaCfgReg);

    /* Zero VLAN Tag To Rx Priority Queue Regsiter */
    MV_REG_WRITE(ETH_VLAN_TAG_TO_PRIO_REG(ethPortNo), 0);

    return MV_OK;
}

/*******************************************************************************
* ethPortUp - Start the Ethernet port RX and TX activity.
*
* DESCRIPTION:
*       This routine start Rx and Tx activity:
*
*       Note: Each Rx and Tx queue descriptor's list must be initialized prior
*       to calling this function (use etherInitTxDescRing for Tx queues and
*       etherInitRxDescRing for Rx queues).
*
* INPUT:
*       void*   pEthPortHndl  - Ethernet port handler
*
* RETURN:   MV_STATUS
*           MV_OK - Success, Others - Failure.
*
* NOTE : used for port link up.
*******************************************************************************/
MV_STATUS   mvEthPortUp(void* pEthPortHndl)
{
    int             ethPortNo;
    ETH_PORT_CTRL*  pPortCtrl = (ETH_PORT_CTRL*)pEthPortHndl;

    ethPortNo = pPortCtrl->portNo;
    
    if( (pPortCtrl->portState != MV_ACTIVE) && 
        (pPortCtrl->portState != MV_PAUSED) )
    {
        mvOsPrintf("ethDrv port%d: Unexpected port state %d\n", 
                        ethPortNo, pPortCtrl->portState);
        return MV_BAD_STATE;
    }
    
    ethPortNo = pPortCtrl->portNo;

    /* Enable port RX */
    MV_REG_WRITE(ETH_SDMA_COMMAND_REG(ethPortNo), ETH_ENABLE_RX_DMA_MASK);

    pPortCtrl->portState = MV_ACTIVE;

    return MV_OK;
}

/*******************************************************************************
* ethPortDown - Stop the Ethernet port activity.
*
* DESCRIPTION:
*
* INPUT:
*       void*   pEthPortHndl  - Ethernet port handler
*
* RETURN:   MV_STATUS
*               MV_OK - Success, Others - Failure.
*
* NOTE : used for port link down.
*******************************************************************************/
MV_STATUS   mvEthPortDown(void* pEthPortHndl)
{
    ETH_PORT_CTRL*  pPortCtrl = (ETH_PORT_CTRL*)pEthPortHndl;
    int             ethPortNum = pPortCtrl->portNo;
    unsigned int    regData;
    volatile int    uDelay, mDelay;
    
    /* Note: Stopping Rx operation by writing the Abort Receive (AR) bit */
    /* When the AR bit is set, the SDMA aborts its current operation and */
    /* moves to idle. No descriptor is closed !                          */
    /* The AR bit is cleared upon entering idle.                         */
    MV_REG_WRITE(ETH_SDMA_COMMAND_REG(ethPortNum), ETH_ABORT_RECEIVE_MASK);

    mDelay = 0;
    do {
	if(mDelay >= RX_DISABLE_TIMEOUT_MSEC) 
        {
            mvOsPrintf("ethPort_%d: TIMEOUT for RX stopped !!!\n", ethPortNum);
            break;
        }
        mvOsDelay(1);
        mDelay++;
        regData = MV_REG_READ(ETH_SDMA_COMMAND_REG(ethPortNum));
    } while(regData & ETH_ABORT_RECEIVE_MASK); 

    /* Stopping Tx operation by writing to the Stop Tx High and Low               */
    /* (STDH, STDL) bits. This causes the Start Tx High and Low (TxDH, TxDL) bits */
    /* to be set to 0.                                                            */
    MV_REG_WRITE(ETH_SDMA_COMMAND_REG(ethPortNum), (ETH_STOP_TX_HIGH_MASK | ETH_STOP_TX_LOW_MASK));
    
    mDelay = 0;
    do {
        if(mDelay >= TX_DISABLE_TIMEOUT_MSEC)
        {
            mvOsPrintf("ethPort_%d: TIMEOUT for TX stopped !!!\n", ethPortNum);
            break;
        }
	mvOsDelay(1);
        mDelay++;
        regData = MV_REG_READ(ETH_SDMA_COMMAND_REG(ethPortNum));
    } while((regData & (ETH_START_TX_HIGH_MASK | ETH_START_TX_LOW_MASK)));

    /* Poll Port Status Register to make sure Tx operation has stopped */
    mDelay = 0;
    do {
        if(mDelay >= TX_DISABLE_TIMEOUT_MSEC)
        {
            mvOsPrintf("ethPort_%d: TIMEOUT for TX stopped !!!\n", ethPortNum);
            break;
        }
	mvOsDelay(1);
        mDelay++;
	regData = MV_REG_READ(ETH_PORT_STATUS_REG(ethPortNum));
    } while(regData & (ETH_TX_STATUS_LOW_MASK | ETH_TX_STATUS_HIGH_MASK | ETH_TX_IN_PROGRESS_MASK));

    /* Wait about 2500 tclk cycles */
    uDelay = (PORT_DISABLE_WAIT_TCLOCKS/(mvBoardTclkGet()/1000000));
    mvOsUDelay(uDelay);

    pPortCtrl->portState = MV_PAUSED;

    return MV_OK;   
}


/*******************************************************************************
* ethPortEnable - Enable the Ethernet port and Start RX and TX.
*
* DESCRIPTION:
*       This routine enable the Ethernet port and Rx and Tx activity:
*
*       Note: Each Rx and Tx queue descriptor's list must be initialized prior
*       to calling this function (use etherInitTxDescRing for Tx queues and
*       etherInitRxDescRing for Rx queues).
*
* INPUT:
*       void*   pEthPortHndl  - Ethernet port handler
*
* RETURN:   MV_STATUS
*               MV_OK - Success, Others - Failure.
*
* NOTE: main usage is to enable the port after ifconfig up.
*******************************************************************************/
MV_STATUS   mvEthPortEnable(void* pEthPortHndl)
{
    int             ethPortNo;
    ETH_PORT_CTRL*  pPortCtrl = (ETH_PORT_CTRL*)pEthPortHndl;
    MV_U32          regData;

    ethPortNo = pPortCtrl->portNo;

    /* Enable port */
    regData = MV_REG_READ(ETH_PORT_CONFIG_REG(ethPortNo));
    MV_REG_WRITE(ETH_PORT_CONFIG_REG(ethPortNo),
                 (regData | ETH_PORT_ENABLE_MASK));

    pPortCtrl->portState = MV_PAUSED;

    /* Assume Link is UP, Start RX and TX traffic */
    return( mvEthPortUp(pEthPortHndl) );
}


/*******************************************************************************
* mvEthPortDisable - Stop RX and TX activities and Disable the Ethernet port.
*
* DESCRIPTION:
*
* INPUT:
*       void*   pEthPortHndl  - Ethernet port handler
*
* RETURN:   MV_STATUS
*               MV_OK - Success, Others - Failure.
*
* NOTE: main usage is to disable the port after ifconfig down.
*******************************************************************************/
MV_STATUS   mvEthPortDisable(void* pEthPortHndl)
{
    ETH_PORT_CTRL*  pPortCtrl = (ETH_PORT_CTRL*)pEthPortHndl;
    int             ethPortNum = pPortCtrl->portNo;
    unsigned int    regData;
    volatile int    delay;

    if(pPortCtrl->portState == MV_ACTIVE)           
    {    
        /* Stop RX and TX activities */
        mvEthPortDown(pEthPortHndl);
    }

    /* Reset the Enable bit in the Port Configuration Register */
    regData = MV_REG_READ(ETH_PORT_CONFIG_REG(ethPortNum));
    regData &= ~(ETH_PORT_ENABLE_MASK);
    MV_REG_WRITE(ETH_PORT_CONFIG_REG(ethPortNum), regData);

    /* Wait about 2500 tclk cycles */
    delay = (PORT_DISABLE_WAIT_TCLOCKS*(mvCpuPclkGet()/mvBoardTclkGet()));
    for(delay; delay>0; delay--);

    pPortCtrl->portState = MV_IDLE;
    return MV_OK;
}


/******************************************************************************/
/*                          Data Flow functions                               */
/******************************************************************************/

/*******************************************************************************
* mvEthPortTx - Send an Ethernet packet
*
* DESCRIPTION:
*       This routine send a given packet described by pBufInfo parameter. It
*       supports transmitting of a packet spanned over multiple buffers. 
*
* INPUT:
*       void*       pEthPortHndl  - Ethernet Port handler.
*       int         txQueue       - Number of Tx queue.
*       MV_PKT_INFO *pPktInfo     - User packet to send.
*
* RETURN:
*       MV_NO_RESOURCE  - No enough resources to send this packet.
*       MV_ERROR        - Unexpected Fatal error.
*       MV_OK           - Packet send successfully.
*
*******************************************************************************/
MV_STATUS   mvEthPortTx(void* pEthPortHndl, int txQueue, MV_PKT_INFO* pPktInfo)
{

    ETH_TX_DESC*    pTxFirstDesc;
    ETH_TX_DESC*    pTxCurrDesc;
    ETH_PORT_CTRL*  pPortCtrl = (ETH_PORT_CTRL*)pEthPortHndl;
    ETH_QUEUE_CTRL* pQueueCtrl;
    int             portNo, bufCount;
    MV_BUF_INFO*    pBufInfo = pPktInfo->pFrags;
    MV_U8*          pTxBuf;
#ifdef ETH_DEBUG
    if(pPortCtrl->portState != MV_ACTIVE)
        return MV_BAD_STATE;
#endif /* ETH_DEBUG */

    portNo = pPortCtrl->portNo;
    pQueueCtrl = &pPortCtrl->txQueue[txQueue];


    /* Get the Tx Desc ring indexes */
    pTxCurrDesc = pQueueCtrl->pCurrentDescr;

#ifdef ETH_DEBUG
    if(pTxCurrDesc == NULL)
        return MV_ERROR;
#endif /* ETH_DEBUG */

    /* Check if there is enough resources to send the packet */
    if(pQueueCtrl->resource < pPktInfo->numFrags)
        return MV_NO_RESOURCE;

    /* Remember first desc */
    pTxFirstDesc  = pTxCurrDesc;

    bufCount = 0;
    while(MV_TRUE)
    {   
        if(pBufInfo[bufCount].bufSize <= MIN_TX_BUFF_LOAD)
        {
            /* Buffers with a payload smaller than MIN_TX_BUFF_LOAD (4 bytes in UniMAC) must be aligned    */
            /* to 32-bit boundary. Copy the payload to the reserved 4 bytes inside descriptor.   */
            pTxBuf = ((MV_U8*)pTxCurrDesc)+TX_BUF_OFFSET_IN_DESC;
            ethBCopy(pBufInfo[bufCount].bufVirtPtr, pTxBuf, pBufInfo[bufCount].bufSize);
            pTxCurrDesc->bufPtr = mvOsIoVirtToPhy(NULL, pTxBuf);            
        }
        else
        {
            pTxCurrDesc->bufPtr = mvOsIoVirtToPhy(NULL, pBufInfo[bufCount].bufVirtPtr);
            /* Flush Buffer */
            ETH_PACKET_CACHE_FLUSH(pBufInfo[bufCount].bufVirtPtr, pBufInfo[bufCount].bufSize);
        }

        pTxCurrDesc->byteCnt = pBufInfo[bufCount].bufSize;
        bufCount++;

        if(bufCount >= pPktInfo->numFrags)
            break;

        if(bufCount > 1)
        {
            /* There is middle buffer of the packet Not First and Not Last */
            pTxCurrDesc->cmdSts = ETH_BUFFER_OWNED_BY_DMA;
	    ETH_DESCR_FLUSH_INV(pPortCtrl, pTxCurrDesc);
        }
        /* Go to next descriptor and next buffer */
        pTxCurrDesc = TX_NEXT_DESC_PTR(pTxCurrDesc, pQueueCtrl);
    }

    /* Set last desc with DMA ownership and interrupt enable. */
    pTxCurrDesc->returnInfo = pPktInfo->osInfo;
    if(bufCount == 1) 
    {
        /* There is only one buffer in the packet */
        /* The OSG might set some bits for checksum offload, so add them to first descriptor */
        pTxCurrDesc->cmdSts = pPktInfo->status              |
                              ETH_BUFFER_OWNED_BY_DMA       |
                              ETH_TX_GENERATE_CRC_MASK      |
                              ETH_ENABLE_INTERRUPT_MASK     |
                              ETH_TX_ZERO_PADDING_MASK      |
                              ETH_FIRST_MASK        	    |
                              ETH_LAST_MASK;

	ETH_DESCR_FLUSH_INV(pPortCtrl, pTxCurrDesc);
    }
    else
    {
        /* Last but not First */
        pTxCurrDesc->cmdSts = ETH_BUFFER_OWNED_BY_DMA       |
                              ETH_ENABLE_INTERRUPT_MASK     |
			      ETH_TX_GENERATE_CRC_MASK	    |
                              ETH_TX_ZERO_PADDING_MASK      |
                              ETH_LAST_MASK;
	
        ETH_DESCR_FLUSH_INV(pPortCtrl, pTxCurrDesc);

        /* Update First when more than one buffer in the packet */
        /* The OSG might set some bits for checksum offload, so add them to first descriptor */
        pTxFirstDesc->cmdSts = pPktInfo->status             |
                               ETH_BUFFER_OWNED_BY_DMA      |
                               ETH_FIRST_MASK;
	
        ETH_DESCR_FLUSH_INV(pPortCtrl, pTxFirstDesc);

    }
    
    MV_REG_WRITE(ETH_SDMA_COMMAND_REG(portNo), txQueue ? 
		(ETH_START_TX_HIGH_MASK /*| ETH_ENABLE_RX_DMA_MASK*/) : (ETH_START_TX_LOW_MASK /*| ETH_ENABLE_RX_DMA_MASK*/));

    /* Update txQueue state */
    pQueueCtrl->resource -= bufCount;
    pQueueCtrl->pCurrentDescr = TX_NEXT_DESC_PTR(pTxCurrDesc, pQueueCtrl);

    return MV_OK;
}

/*******************************************************************************
* mvEthPortTxDone - Free all used Tx descriptors and mBlks.
*
* DESCRIPTION:
*       This routine returns the transmitted packet information to the caller.
*
* INPUT:
*       void*       pEthPortHndl    - Ethernet Port handler.
*       int         txQueue         - Number of Tx queue.
*
* OUTPUT:
*       MV_PKT_INFO *pPktInfo       - Pointer to packet was sent.
*
* RETURN:
*       MV_NOT_FOUND    - No transmitted packets to return. Transmit in progress.
*       MV_EMPTY        - No transmitted packets to return. TX Queue is empty.
*       MV_ERROR        - Unexpected Fatal error.
*       MV_OK           - There is transmitted packet in the queue, 
*                       'pPktInfo' filled with relevant information.
*
*******************************************************************************/
MV_STATUS   mvEthPortTxDone(void* pEthPortHndl, int txQueue, MV_PKT_INFO *pPktInfo)
{


    ETH_TX_DESC*    pTxCurrDesc;
    ETH_TX_DESC*    pTxUsedDesc;
    ETH_QUEUE_CTRL* pQueueCtrl;
    ETH_PORT_CTRL*  pPortCtrl = (ETH_PORT_CTRL*)pEthPortHndl;
    MV_U32          commandStatus;

    pQueueCtrl = &pPortCtrl->txQueue[txQueue];

    pTxUsedDesc = pQueueCtrl->pUsedDescr;
    pTxCurrDesc = pQueueCtrl->pCurrentDescr;

    /* Sanity check */
#ifdef ETH_DEBUG
    if(pTxUsedDesc == 0)
        return MV_ERROR;
#endif /* ETH_DEBUG */

    while(MV_TRUE)
    {
        /* No more used descriptors */
        commandStatus = pTxUsedDesc->cmdSts;
        if (commandStatus  & (ETH_BUFFER_OWNED_BY_DMA))
        {           
            ETH_DESCR_INV(pPortCtrl, pTxUsedDesc);
            return MV_NOT_FOUND;
        }
        if( (pTxUsedDesc == pTxCurrDesc) &&
            (pQueueCtrl->resource != 0) )
        {
            return MV_EMPTY;
        }
        pQueueCtrl->resource++;
        pQueueCtrl->pUsedDescr = TX_NEXT_DESC_PTR(pTxUsedDesc, pQueueCtrl);

        if(commandStatus & (ETH_LAST_MASK)) 
        {
            pPktInfo->status  = commandStatus;
            pPktInfo->osInfo  = pTxUsedDesc->returnInfo;
            
            return MV_OK;
        }

        pTxUsedDesc = pQueueCtrl->pUsedDescr;
    }
}

/*******************************************************************************
* mvEthPortForceTxDone - Get next buffer from TX queue in spite of buffer ownership.
*
* DESCRIPTION:
*       This routine used to free buffers attached to the Tx ring and should
*       be called only when Giga Ethernet port is Down
*
* INPUT:
*       void*       pEthPortHndl    - Ethernet Port handler.
*       int         txQueue         - Number of TX queue.
*
* OUTPUT:
*       MV_PKT_INFO *pPktInfo       - Pointer to packet was sent.
*
* RETURN:
*       MV_EMPTY    - There is no more buffers in this queue.
*       MV_OK       - Buffer detached from the queue and pPktInfo structure 
*                   filled with relevant information.
*
*******************************************************************************/
MV_STATUS   mvEthPortForceTxDone(void* pEthPortHndl, int txQueue, MV_PKT_INFO *pPktInfo)
{
    ETH_PORT_CTRL*  pPortCtrl = (ETH_PORT_CTRL*)pEthPortHndl;
    ETH_QUEUE_CTRL* pQueueCtrl;
    int             port = pPortCtrl->portNo;
       
    pQueueCtrl = &pPortCtrl->txQueue[txQueue];

    while( (pQueueCtrl->pUsedDescr != pQueueCtrl->pCurrentDescr) ||
           (pQueueCtrl->resource == 0) )
    {
        /* Free next descriptor */
        pQueueCtrl->resource++;
        pPktInfo->status = ((ETH_TX_DESC*)pQueueCtrl->pUsedDescr)->cmdSts;
        pPktInfo->osInfo  = ((ETH_TX_DESC*)pQueueCtrl->pUsedDescr)->returnInfo;
        ((ETH_TX_DESC*)pQueueCtrl->pUsedDescr)->cmdSts = 0x0;
        ((ETH_TX_DESC*)pQueueCtrl->pUsedDescr)->returnInfo = 0x0;
        ETH_DESCR_FLUSH_INV(pPortCtrl, pQueueCtrl->pUsedDescr);

        pQueueCtrl->pUsedDescr = TX_NEXT_DESC_PTR(pQueueCtrl->pUsedDescr, pQueueCtrl);

        if(pPktInfo->status  & ETH_LAST_MASK) 
            return MV_OK;
    }   
    MV_REG_WRITE( CURR_TX_DESC_PTR(port, txQueue), 
                    (MV_U32)ethDescVirtToPhy(pQueueCtrl, pQueueCtrl->pCurrentDescr) );


    return MV_EMPTY;
}

        
/*******************************************************************************
* mvEthPortRx - Get new received packets from Rx queue.
*
* DESCRIPTION:
*       This routine returns the received data to the caller. There is no
*       data copying during routine operation. All information is returned
*       using pointer to packet information struct passed from the caller.
*
* INPUT:
*       void*       pEthPortHndl    - Ethernet Port handler.
*       int         rxQueue         - Number of Rx queue.
*
* OUTPUT:
*       MV_PKT_INFO *pPktInfo       - Pointer to received packet.
*
* RETURN:
*       MV_NO_RESOURCE  - No free resources in RX queue.
*       MV_ERROR        - Unexpected Fatal error.
*       MV_OK           - New packet received and 'pBufInfo' structure filled
*                       with relevant information.
*
*******************************************************************************/
MV_STATUS   mvEthPortRx(void* pEthPortHndl, int rxQueue, MV_PKT_INFO *pPktInfo)
{
    ETH_RX_DESC     *pRxCurrDesc;
    MV_U32          commandStatus;
    ETH_PORT_CTRL*  pPortCtrl = (ETH_PORT_CTRL*)pEthPortHndl;
    ETH_QUEUE_CTRL* pQueueCtrl;

    pQueueCtrl = &(pPortCtrl->rxQueue[rxQueue]);
    /* Check resources */
    if(pQueueCtrl->resource == 0)
        return MV_NO_RESOURCE;
    
    while(MV_TRUE)
    {
        /* Get the Rx Desc ring 'curr and 'used' indexes */
        pRxCurrDesc = pQueueCtrl->pCurrentDescr;
    
        /* Sanity check */
#ifdef ETH_DEBUG
        if (pRxCurrDesc == 0)
            return MV_ERROR;
#endif  /* ETH_DEBUG */

        commandStatus   = pRxCurrDesc->cmdSts;
        if (commandStatus & (ETH_BUFFER_OWNED_BY_DMA))
        {
            /* Nothing to receive... */
            ETH_DESCR_INV(pPortCtrl, pRxCurrDesc);
            return MV_NO_MORE;
        }

        /* Valid RX only if FIRST and LAST bits are set */
        if( (commandStatus & (ETH_LAST_MASK | ETH_FIRST_MASK)) == 
                             (ETH_LAST_MASK | ETH_FIRST_MASK) )
        {
	    pPktInfo->pktSize    = pRxCurrDesc->byteCnt;
            pPktInfo->status     = commandStatus;
            pPktInfo->osInfo     = pRxCurrDesc->returnInfo;
            pQueueCtrl->resource--;
            /* Update 'curr' in data structure */
            pQueueCtrl->pCurrentDescr = RX_NEXT_DESC_PTR(pRxCurrDesc, pQueueCtrl);

            return MV_OK;
        }
        else
        {
            ETH_RX_DESC*    pRxUsedDesc = pQueueCtrl->pUsedDescr;
#ifdef ETH_DEBUG
            mvOsPrintf("ethDrv: Unexpected Jumbo frame: "
                       "status=0x%08x, byteCnt=%d, pData=0x%x\n", 
                 commandStatus, pRxCurrDesc->byteCnt, pRxCurrDesc->bufPtr);
#endif /* ETH_DEBUG */
            /* move buffer from pCurrentDescr position to pUsedDescr position */
            pRxUsedDesc->bufPtr     = pRxCurrDesc->bufPtr;
            pRxUsedDesc->returnInfo = pRxCurrDesc->returnInfo;
            pRxUsedDesc->bufSize    = pRxCurrDesc->bufSize & ETH_RX_BUFFER_MASK;

            /* Return the descriptor to DMA ownership */
            pRxUsedDesc->cmdSts = ETH_BUFFER_OWNED_BY_DMA | 
                                  ETH_ENABLE_INTERRUPT_MASK;

            /* Flush descriptor and CPU pipe */
            ETH_DESCR_FLUSH_INV(pPortCtrl, pRxUsedDesc);

            /* Move the used descriptor pointer to the next descriptor */
            pQueueCtrl->pUsedDescr = RX_NEXT_DESC_PTR(pRxUsedDesc, pQueueCtrl);
            pQueueCtrl->pCurrentDescr = RX_NEXT_DESC_PTR(pRxCurrDesc, pQueueCtrl);            
        }
    }
}

/*******************************************************************************
* mvEthPortRxDone - Returns a Rx buffer back to the Rx ring.
*
* DESCRIPTION:
*       This routine returns a Rx buffer back to the Rx ring. 
*
* INPUT:
*       void*       pEthPortHndl    - Ethernet Port handler.
*       int         rxQueue         - Number of Rx queue.
*       MV_PKT_INFO *pPktInfo       - Pointer to received packet.
*
* RETURN:
*       MV_ERROR        - Unexpected Fatal error.
*       MV_OUT_OF_RANGE - RX queue is already FULL, so this buffer can't be 
*                       returned to this queue.
*       MV_FULL         - Buffer returned successfully and RX queue became full.
*                       More buffers should not be returned at the time.
*       MV_OK           - Buffer returned successfully and there are more free 
*                       places in the queue.
*
*******************************************************************************/
MV_STATUS   mvEthPortRxDone(void* pEthPortHndl, int rxQueue, MV_PKT_INFO *pPktInfo)
{

    ETH_RX_DESC*    pRxUsedDesc;
    ETH_QUEUE_CTRL* pQueueCtrl;
    ETH_PORT_CTRL*  pPortCtrl = (ETH_PORT_CTRL*)pEthPortHndl;
            
    pQueueCtrl = &pPortCtrl->rxQueue[rxQueue];

    /* Get 'used' Rx descriptor */
    pRxUsedDesc = pQueueCtrl->pUsedDescr;

#ifdef ETH_DEBUG
    /* Sanity check */
    if (pRxUsedDesc == NULL)
        return MV_ERROR;
#endif /* ETH_DEBUG */

    /* Check that ring is not FULL */
    if( (pQueueCtrl->pUsedDescr == pQueueCtrl->pCurrentDescr) && 
        (pQueueCtrl->resource != 0) )
    {
        mvOsPrintf("%s %d: out of range Error resource=%d, curr=%p, used=%p\n", 
                    __FUNCTION__, pPortCtrl->portNo, pQueueCtrl->resource, 
                    pQueueCtrl->pCurrentDescr, pQueueCtrl->pUsedDescr);
        return MV_OUT_OF_RANGE;
    }

    pRxUsedDesc->bufPtr     = mvOsIoVirtToPhy(pPortCtrl, pPktInfo->pFrags->bufVirtPtr);
    pRxUsedDesc->returnInfo = pPktInfo->osInfo;
    pRxUsedDesc->bufSize    = pPktInfo->pFrags->bufSize & ETH_RX_BUFFER_MASK;


    /* Invalidate data buffer accordingly with pktSize */
    ETH_PACKET_CACHE_INVALIDATE(pPktInfo->pFrags->bufVirtPtr, pPktInfo->pktSize);
    /* Return the descriptor to DMA ownership */
    pRxUsedDesc->cmdSts = ETH_BUFFER_OWNED_BY_DMA | ETH_ENABLE_INTERRUPT_MASK;
    /* Flush descriptor and CPU pipe */
    ETH_DESCR_FLUSH_INV(pPortCtrl, pRxUsedDesc);

    pQueueCtrl->resource++;

    /* Move the used descriptor pointer to the next descriptor */
    pQueueCtrl->pUsedDescr = RX_NEXT_DESC_PTR(pRxUsedDesc, pQueueCtrl);

    /* If ring became Full return MV_FULL */
    if(pQueueCtrl->pUsedDescr == pQueueCtrl->pCurrentDescr) 
        return MV_FULL;

    return MV_OK;
}

/*******************************************************************************
* mvEthPortForceRx - Get next buffer from RX queue in spite of buffer ownership.
*
* DESCRIPTION:
*       This routine used to free buffers attached to the Rx ring and should
*       be called only when Giga Ethernet port is Down
*
* INPUT:
*       void*       pEthPortHndl    - Ethernet Port handler.
*       int         rxQueue         - Number of Rx queue.
*
* OUTPUT:
*       MV_PKT_INFO *pPktInfo       - Pointer to received packet.
*
* RETURN:
*       MV_EMPTY    - There is no more buffers in this queue.
*       MV_OK       - Buffer detached from the queue and pBufInfo structure 
*                   filled with relevant information.
*
*******************************************************************************/
MV_STATUS   mvEthPortForceRx(void* pEthPortHndl, int rxQueue, MV_PKT_INFO *pPktInfo)
{
    ETH_PORT_CTRL*  pPortCtrl = (ETH_PORT_CTRL*)pEthPortHndl;
    ETH_QUEUE_CTRL* pQueueCtrl;
    int             port = pPortCtrl->portNo;
       
    pQueueCtrl = &pPortCtrl->rxQueue[rxQueue];

    if(pQueueCtrl->resource == 0)
    {
        MV_REG_WRITE( CURR_RX_DESC_PTR(port, rxQueue), 
                    (MV_U32)ethDescVirtToPhy(pQueueCtrl, pQueueCtrl->pCurrentDescr) );

        return MV_EMPTY;
    }
    /* Free next descriptor */
    pQueueCtrl->resource--;
    pPktInfo->status  = ((ETH_RX_DESC*)pQueueCtrl->pCurrentDescr)->cmdSts;
    pPktInfo->osInfo  = ((ETH_RX_DESC*)pQueueCtrl->pCurrentDescr)->returnInfo;
    ((ETH_RX_DESC*)pQueueCtrl->pCurrentDescr)->cmdSts = 0x0;
    ((ETH_RX_DESC*)pQueueCtrl->pCurrentDescr)->returnInfo = 0x0;
    ETH_DESCR_FLUSH_INV(pPortCtrl, pQueueCtrl->pCurrentDescr);

    pQueueCtrl->pCurrentDescr = RX_NEXT_DESC_PTR(pQueueCtrl->pCurrentDescr, pQueueCtrl);

    return MV_OK;    
}

/******************************************************************************/
/*                          Port Status functions                             */
/******************************************************************************/
/* Get number of Free resources in specific TX queue */
int     mvEthTxResourceGet(void* pPortHndl, int txQueue)
{
    ETH_PORT_CTRL*  pPortCtrl = (ETH_PORT_CTRL*)pPortHndl;

    return (pPortCtrl->txQueue[txQueue].resource);      
}

/* Get number of Free resources in specific RX queue */
int     mvEthRxResourceGet(void* pPortHndl, int rxQueue)
{
    ETH_PORT_CTRL*  pPortCtrl = (ETH_PORT_CTRL*)pPortHndl;

    return (pPortCtrl->rxQueue[rxQueue].resource);      
}


/******************************************************************************/
/*                      MAC Filtering functions                               */
/******************************************************************************/

/*
 * An address table entry 64bit representation 
 */
typedef struct addressTableEntryStruct
{
	MV_U32 hi;
	MV_U32 lo;
} addrTblEntry;

#define _8K_TABLE	0
#define _HALFK_TABLE	1
#define SKIP_BIT	1
#define VALID		1
#define HOP_NUMBER	12
#define MAC_ENTRY_SIZE	sizeof(addrTblEntry)


#define NIBBLE_SWAPPING_32_BIT(X) ( (((X) & 0xf0f0f0f0) >> 4) \
                                  | (((X) & 0x0f0f0f0f) << 4) )

#define NIBBLE_SWAPPING_16_BIT(X) ( (((X) & 0x0000f0f0) >> 4) \
                                  | (((X) & 0x00000f0f) << 4) )

#define FLIP_4_BITS(X)  ( (((X) & 0x01) << 3) | (((X) & 0x002) << 1) \
                        | (((X) & 0x04) >> 1) | (((X) & 0x008) >> 3) )

#define FLIP_6_BITS(X)  ( (((X) & 0x01) << 5) | (((X) & 0x020) >> 5) \
                        | (((X) & 0x02) << 3) | (((X) & 0x010) >> 3) \
                        | (((X) & 0x04) << 1) | (((X) & 0x008) >> 1) )

#define FLIP_9_BITS(X)  ( (((X) & 0x01) << 8) | (((X) & 0x100) >> 8) \
                        | (((X) & 0x02) << 6) | (((X) & 0x080) >> 6) \
                        | (((X) & 0x04) << 4) | (((X) & 0x040) >> 4) \
         | ((X) & 0x10) | (((X) & 0x08) << 2) | (((X) & 0x020) >> 2) )

/*
 * ----------------------------------------------------------------------------
 * This function will calculate the hash function of the address.
 * depends on the hash mode and hash size.
 * Inputs
 * macH             - the 2 most significant bytes of the MAC address.
 * macL             - the 4 least significant bytes of the MAC address.
 * hashMode         - hash mode 0 or hash mode 1.
 * hashSize	    - indicates number of hash table entries (0=0x8000,1=0x800)
 * Outputs
 * return the calculated entry.
 */
static MV_U32 hashTableFunction(MV_U32 macH, MV_U32 macL, MV_U32 HashSize, MV_U32 hash_mode)
{
	MV_U32 hashResult;
	MV_U32 addrH;
	MV_U32 addrL;
	MV_U32 addr0;
	MV_U32 addr1;
	MV_U32 addr2;
	MV_U32 addr3;
	MV_U32 addrHSwapped;
	MV_U32 addrLSwapped;

	addrH = NIBBLE_SWAPPING_16_BIT(macH);
	addrL = NIBBLE_SWAPPING_32_BIT(macL);

	addrHSwapped = FLIP_4_BITS(addrH & 0xf)
				   + ((FLIP_4_BITS((addrH >> 4) & 0xf)) << 4)
				   + ((FLIP_4_BITS((addrH >> 8) & 0xf)) << 8)
				   + ((FLIP_4_BITS((addrH >> 12) & 0xf)) << 12);

	addrLSwapped = FLIP_4_BITS(addrL & 0xf)
				   + ((FLIP_4_BITS((addrL >> 4) & 0xf)) << 4)
				   + ((FLIP_4_BITS((addrL >> 8) & 0xf)) << 8)
				   + ((FLIP_4_BITS((addrL >> 12) & 0xf)) << 12)
				   + ((FLIP_4_BITS((addrL >> 16) & 0xf)) << 16)
				   + ((FLIP_4_BITS((addrL >> 20) & 0xf)) << 20)
				   + ((FLIP_4_BITS((addrL >> 24) & 0xf)) << 24)
				   + ((FLIP_4_BITS((addrL >> 28) & 0xf)) << 28);

	addrH = addrHSwapped;
	addrL = addrLSwapped;

	if (hash_mode == 0)
	{
		addr0 = (addrL >> 2) & 0x03f;
		addr1 = (addrL & 0x003) | ((addrL >> 8) & 0x7f) << 2;
		addr2 = (addrL >> 15) & 0x1ff;
		addr3 = ((addrL >> 24) & 0x0ff) | ((addrH & 1) << 8);
	} else
	{
		addr0 = FLIP_6_BITS(addrL & 0x03f);
		addr1 = FLIP_9_BITS(((addrL >> 6) & 0x1ff));
		addr2 = FLIP_9_BITS((addrL >> 15) & 0x1ff);
		addr3 =
		FLIP_9_BITS((((addrL >> 24) & 0x0ff) |
					 ((addrH & 0x1) << 8)));
	}

	hashResult = (addr0 << 9) | (addr1 ^ addr2 ^ addr3);

	if (HashSize == _8K_TABLE)
	{
		hashResult = hashResult & 0xffff;
	} else
	{
		hashResult = hashResult & 0x07ff;
	}

	return(hashResult);
}

/*
 * ----------------------------------------------------------------------------
 * This function will add an entry to the address table.
 * depends on the hash mode and hash size that was initialized.
 * Inputs
 * port - Ethernet port number.
 * macH - the 2 most significant bytes of the MAC address.
 * macL - the 4 least significant bytes of the MAC address.
 * skip - if 1, skip this address.
 * queue - queue to receive this packets with this address. if queue = -1, remove this address entry.
 * Outputs
 * address table entry is added.
 * 1 if success.
 * 0 if table full
 */
static int addAddressTableEntry(void* pPortHndl, MV_U32 macH, MV_U32 macL, MV_U32 skip, int queue)
{
	ETH_PORT_CTRL*  pPortCtrl = (ETH_PORT_CTRL*)pPortHndl;
	MV_U32 entryAddr = 0;
	addrTblEntry *entry;
	MV_U32 newHi;
	MV_U32 newLo;
	MV_U32 i;
	MV_U32 rd = 1; /* rd = 1 for Receive, 0 for Discard. We always use Receive. */

	newHi = (((macH >> 4) & 0xf) << 15)
			| (((macH >> 0) & 0xf) << 11)
			| (((macH >> 12) & 0xf) << 7)
			| (((macH >> 8) & 0xf) << 3)
			| (((macL >> 20) & 0x1) << 31)
			| (((macL >> 16) & 0xf) << 27)
			| (((macL >> 28) & 0xf) << 23)
			| (((macL >> 24) & 0xf) << 19)
			| (skip << SKIP_BIT) | (rd << 2) | VALID;

	newLo = (((macL >> 4) & 0xf) << 15)
			| (((macL >> 0) & 0xf) << 11)
			| (((macL >> 12) & 0xf) << 7)
			| (((macL >> 8) & 0xf) << 3)
			| (((macL >> 21) & 0x7) << 0);

	/*
	 * Pick the appropriate table, start scanning for free/reusable
	 * entries at the index obtained by hashing the specified MAC address
	 */
	entryAddr = (MV_U32)pPortCtrl->hashPtr;
	entryAddr += (hashTableFunction(macH, macL, _HALFK_TABLE, 0) << 3);
	entry = (addrTblEntry *)entryAddr;

	for (i = 0; i < HOP_NUMBER; i++, entry++)
	{
		if (!(entry->lo & VALID) /*|| (entry->lo & SKIP) */ )
		{
			break;
		} else
		{	 /* if same address put in same position */
			if (((entry->lo & 0xfffffff8) == (newLo & 0xfffffff8)) 
				&& (entry->hi == newHi))
			{
				break;
			}
		}
	}

	if (i == HOP_NUMBER)
	{
		printk("add address Table Entry: table section is full\n");
		return(0);
	}

	/*
	 * Update the selected entry
	 */
	if (queue < 0) {
	    entry->hi = 0;
	    entry->lo = 0;
	}
	else {
	    entry->hi = newHi;
	    entry->lo = newLo;
	}

	mvOsCacheFlush(NULL, entry,  MAC_ENTRY_SIZE);
	return(1);
}


/*******************************************************************************
* mvEthRxFilterModeSet - Configure Fitering mode of Ethernet port
*
* DESCRIPTION:
*       
* INPUT:
*       void*       pEthPortHndl    - Ethernet Port handler.
*       MV_BOOL     isPromisc       - Promiscous mode
*                                   MV_TRUE  - set UniMAC to be in Multicast and Unicast promiscuous mode - 
* 					       accept all Multicast and Unicast packets
*                                   MV_FALSE - UniMAC not in Multicast and Unicast promiscuous mode
*
* RETURN:   MV_STATUS   MV_OK - Success, Other - Failure
*
*******************************************************************************/
MV_STATUS   mvEthRxFilterModeSet(void* pEthPortHndl, MV_BOOL isPromisc)
{
    ETH_PORT_CTRL*  pPortCtrl = (ETH_PORT_CTRL*)pEthPortHndl;
    MV_U32      portCfgReg;

    portCfgReg = MV_REG_READ(ETH_PORT_CONFIG_REG(pPortCtrl->portNo));
    /* Set / Clear PM_UC and PM_MC bits in port configuration register */
    if(isPromisc)
    {
        portCfgReg |= (ETH_UNICAST_PROMISCUOUS_MODE_MASK | ETH_MULTICAST_PROMISCUOUS_MODE_MASK);
    }
    else
    {
        portCfgReg &= ~(ETH_UNICAST_PROMISCUOUS_MODE_MASK | ETH_MULTICAST_PROMISCUOUS_MODE_MASK);    
    }
    MV_REG_WRITE(ETH_PORT_CONFIG_REG(pPortCtrl->portNo), portCfgReg);        
            
    return MV_OK;
}

/*******************************************************************************
* mvEthMacAddrSet - This function sets the port Unicast address.
*
* DESCRIPTION:
*       This function sets the port Ethernet MAC address. Packets with this
*       address will be accepted.
*
* INPUT:
*       void*   pEthPortHndl    - Ethernet port handler.
*       char*   pAddr           - Address to be set
*
* RETURN:   MV_STATUS
*               MV_OK - Success,  Other - Faulure
*
*******************************************************************************/
MV_STATUS   mvEthMacAddrSet(void* pPortHndl, unsigned char *pAddr, int queue)
{
    MV_U32 macH;
    MV_U32 macL;

    if(queue >= MV_ETH_RX_Q_NUM)
    {
        mvOsPrintf("ethDrv: RX queue #%d is out of range\n", queue);
        return MV_BAD_PARAM;
    }

    macL = (pAddr[2] << 24) | (pAddr[3] << 16) | (pAddr[4] << 8) | (pAddr[5]);
    macH = (pAddr[0] << 8) | (pAddr[1]);
    
    /* Accept frames of this address */
    if (!addAddressTableEntry(pPortHndl, macH, macL, 0, queue))
	return MV_ERROR;

    return MV_OK;
}


MV_STATUS   mvEthMcastAddrSet(void* pPortHndl, MV_U8 *pAddr, int queue)
{
    return mvEthMacAddrSet(pPortHndl, pAddr, queue);
}
/******************************************************************************/
/*                        RX Dispatching configuration routines               */
/******************************************************************************/

/*******************************************************************************
* mvEthVlanPrioRxQueue - Configure RX queue to capture VLAN tagged packets with 
*                        special priority bits [0-2]
*
* DESCRIPTION:
*
* INPUT:
*       void*   pPortHandle - Pointer to port specific handler;
*       
* RETURN:   MV_STATUS
*       MV_OK       - Success
*       MV_FAIL     - Failed. 
*
*******************************************************************************/
MV_STATUS   mvEthVlanPrioRxQueue(void* pPortHandle, int vlanPrio, int vlanPrioQueue)
{
    ETH_PORT_CTRL*  pPortCtrl = (ETH_PORT_CTRL*)pPortHandle;
    MV_U32          vlanPrioReg;

    if(vlanPrioQueue >= MV_ETH_RX_Q_NUM)
    {
        mvOsPrintf("ethDrv: RX queue #%d is out of range\n", vlanPrioQueue);
        return MV_BAD_PARAM;
    }
    if(vlanPrio >= 8)
    {
        mvOsPrintf("ethDrv: vlanPrio=%d is out of range\n", vlanPrio);
        return MV_BAD_PARAM;
    }
	
    vlanPrioReg = MV_REG_READ(ETH_VLAN_TAG_TO_PRIO_REG(pPortCtrl->portNo));
    /* zero current value for this vlan priority */
    vlanPrioReg &= ~(0x1 << vlanPrio);
    vlanPrioReg &= ~(0x1 << (vlanPrio + 8));

    if (vlanPrioQueue & 0x1) /* first bit of queue */
        vlanPrioReg |= (0x1 << vlanPrio);

    if (vlanPrioQueue & 0x2) /* second bit of queue */
        vlanPrioReg |= (0x1 << (vlanPrio + 8));
 
    MV_REG_WRITE(ETH_VLAN_TAG_TO_PRIO_REG(pPortCtrl->portNo), vlanPrioReg);

    return MV_OK;
}


/*******************************************************************************
* mvEthBpduRxQueue - Configure RX queue to capture BPDU packets.
*
* DESCRIPTION:
*       This function defines processing of BPDU packets. 
*   BPDU packets can be accepted and captured to the high priority RX queue 
*   (queue 3) or can be processing as regular Multicast packets. 
*
* INPUT:
*       void*   pPortHandle - Pointer to port specific handler;
*       int     bpduQueue   - Special queue to capture BPDU packets (DA is equal to 
*                           01-80-C2-00-00-00 through 01-80-C2-00-00-FF, 
*                           except for the Flow-Control Pause packets). 
*                           Negative value (-1) means no special processing for BPDU, 
*                           packets so they will be processed as regular Multicast packets.
*
* RETURN:   MV_STATUS
*       MV_OK       - Success
*       MV_FAIL     - Failed. 
*
*******************************************************************************/
MV_STATUS   mvEthBpduRxQueue(void* pPortHandle, int bpduQueue)
{
    ETH_PORT_CTRL*  pPortCtrl = (ETH_PORT_CTRL*)pPortHandle;
    MV_U32      portCfgExtReg;

    if(bpduQueue >= MV_ETH_RX_Q_NUM)
    {
        mvOsPrintf("ethDrv: RX queue #%d is out of range\n", bpduQueue);
        return MV_BAD_PARAM;
    }
  
    portCfgExtReg = MV_REG_READ(ETH_PORT_CONFIG_EXTEND_REG(pPortCtrl->portNo));

    if(bpduQueue >= 0)
    {
        pPortCtrl->portConfig.rxBpduQ = bpduQueue;
        portCfgExtReg |= ETH_BPDU_CAPTURE_ENABLE_MASK;
    }
    else
    {
        pPortCtrl->portConfig.rxBpduQ = -1;
        /* no special processing for BPDU packets */
        portCfgExtReg &= (~ETH_BPDU_CAPTURE_ENABLE_MASK);
    }

    MV_REG_WRITE(ETH_PORT_CONFIG_EXTEND_REG(pPortCtrl->portNo),  portCfgExtReg);

    return MV_OK;
}


/*******************************************************************************
* mvEthFlowCtrlSet - Set Flow Control of the port.
*
* DESCRIPTION:
*       This function configure the port to work with desirable 
*       flow control mode.
*
* INPUT:
*       void*           pPortHandle - Pointer to port specific handler;
*       MV_ETH_PORT_FC  flowControl - Flow control of the port.
*
* RETURN:   MV_STATUS
*       MV_OK           - Success
*
*******************************************************************************/
MV_STATUS   mvEthFlowCtrlSet(void* pPortHandle, MV_ETH_PORT_FC flowControl)
{
    ETH_PORT_CTRL*  pPortCtrl = (ETH_PORT_CTRL*)pPortHandle;
    int             port = pPortCtrl->portNo;
    MV_U32	    portCfgExtendReg;
    
    if( (port < 0) || (port >= mvCtrlEthMaxPortGet() ) )
        return MV_OUT_OF_RANGE;

    pPortCtrl = ethPortCtrl[port];
    if(pPortCtrl == NULL)
        return MV_NOT_FOUND;

    portCfgExtendReg = MV_REG_READ(ETH_PORT_CONFIG_EXTEND_REG(port));
    switch(flowControl)
    {
        case MV_ETH_FC_AN_ADV_DIS:                     
        case MV_ETH_FC_AN_ADV_SYM:
	    /* do nothing, UniMAC doesn't support this */
	    break;
             
        case MV_ETH_FC_DISABLE:
            portCfgExtendReg |= ETH_FLOW_CTRL_DISABLE_MASK;
            break;

        case MV_ETH_FC_ENABLE:
            portCfgExtendReg &= ~ETH_FLOW_CTRL_DISABLE_MASK;
            break;

        default:
            mvOsPrintf("ethDrv: Unexpected FlowControl value %d\n", flowControl);
            return MV_BAD_VALUE;
    }
    MV_REG_WRITE(ETH_PORT_CONFIG_EXTEND_REG(port), portCfgExtendReg);

    return MV_OK;
}


MV_STATUS mvEthHeaderModeSet(void* pPortHandle, MV_ETH_HEADER_MODE headerMode)
{
    ETH_PORT_CTRL*  pPortCtrl = (ETH_PORT_CTRL*)pPortHandle;
    int             port = pPortCtrl->portNo;
    MV_U32			portCfgExtendReg;
    
    if( (port < 0) || (port >= mvCtrlEthMaxPortGet() ) )
        return MV_OUT_OF_RANGE;

    pPortCtrl = ethPortCtrl[port];
    if(pPortCtrl == NULL)
        return MV_NOT_FOUND;

    portCfgExtendReg = MV_REG_READ(ETH_PORT_CONFIG_EXTEND_REG(port));
	portCfgExtendReg &= ~ETH_HEADER_MODE_MASK;
	portCfgExtendReg |= (headerMode << ETH_HEADER_MODE_OFFSET);

	MV_REG_WRITE(ETH_PORT_CONFIG_EXTEND_REG(port), portCfgExtendReg);

    return MV_OK;
}


/******************************************************************************/
/*                Descriptor handling Functions                               */
/******************************************************************************/

/*******************************************************************************
* etherInitRxDescRing - Curve a Rx chain desc list and buffer in memory.
*
* DESCRIPTION:
*       This function prepares a Rx chained list of descriptors and packet
*       buffers in a form of a ring. The routine must be called after port
*       initialization routine and before port start routine.
*       The Ethernet SDMA engine uses CPU bus addresses to access the various
*       devices in the system (i.e. DRAM). This function uses the ethernet
*       struct 'virtual to physical' routine (set by the user) to set the ring
*       with physical addresses.
*
* INPUT:
*       ETH_QUEUE_CTRL  *pEthPortCtrl   Ethernet Port Control srtuct.
*       int             rxQueue         Number of Rx queue.
*       int             rxDescNum       Number of Rx descriptors
*       MV_U8*          rxDescBaseAddr  Rx descriptors memory area base addr.
*
* OUTPUT:
*       The routine updates the Ethernet port control struct with information
*       regarding the Rx descriptors and buffers.
*
* RETURN: None
*
*******************************************************************************/
static void ethInitRxDescRing(ETH_PORT_CTRL* pPortCtrl, int queue)
{
    ETH_RX_DESC     *pRxDescBase, *pRxDesc, *pRxPrevDesc; 
    int             ix, rxDescNum = pPortCtrl->rxQueueConfig[queue].descrNum;
    ETH_QUEUE_CTRL  *pQueueCtrl = &pPortCtrl->rxQueue[queue];

    /* Make sure descriptor address is cache line size aligned  */        
    pRxDescBase = (ETH_RX_DESC*)MV_ALIGN_UP((MV_ULONG)pQueueCtrl->descBuf.bufVirtPtr,
                                     CPU_D_CACHE_LINE_SIZE);

    pRxDesc      = (ETH_RX_DESC*)pRxDescBase;
    pRxPrevDesc  = pRxDesc;

    /* initialize the Rx descriptors ring */
    for (ix=0; ix<rxDescNum; ix++)
    {
        pRxDesc->bufSize     = 0x0;
        pRxDesc->byteCnt     = 0x0;
        pRxDesc->cmdSts      = ETH_BUFFER_OWNED_BY_HOST;
        pRxDesc->bufPtr      = 0x0;
        pRxDesc->returnInfo  = 0x0;
        pRxPrevDesc = pRxDesc;
        if(ix == (rxDescNum-1))
        {
            /* Closing Rx descriptors ring */
            pRxPrevDesc->nextDescPtr = (MV_U32)ethDescVirtToPhy(pQueueCtrl, (void*)pRxDescBase);
        }
        else
        {
            pRxDesc = (ETH_RX_DESC*)((MV_ULONG)pRxDesc + ETH_RX_DESC_ALIGNED_SIZE);
            pRxPrevDesc->nextDescPtr = (MV_U32)ethDescVirtToPhy(pQueueCtrl, (void*)pRxDesc);
        }
        ETH_DESCR_FLUSH_INV(pPortCtrl, pRxPrevDesc);
    }

    pQueueCtrl->pCurrentDescr = pRxDescBase;
    pQueueCtrl->pUsedDescr = pRxDescBase;
    
    pQueueCtrl->pFirstDescr = pRxDescBase;
    pQueueCtrl->pLastDescr = pRxDesc;
    pQueueCtrl->resource = 0;
}

/******************************************************************************/

void ethResetRxDescRing(void* pPortHndl, int queue)
{
    ETH_PORT_CTRL*  pPortCtrl = (ETH_PORT_CTRL*)pPortHndl;
    ETH_QUEUE_CTRL* pQueueCtrl = &pPortCtrl->rxQueue[queue];
    ETH_RX_DESC*    pRxDesc = (ETH_RX_DESC*)pQueueCtrl->pFirstDescr;
    
    pQueueCtrl->resource = 0;
    if(pQueueCtrl->pFirstDescr != NULL)
    {
	while(MV_TRUE)
        {
            pRxDesc->bufSize     = 0x0;
            pRxDesc->byteCnt     = 0x0;
            pRxDesc->cmdSts      = ETH_BUFFER_OWNED_BY_HOST;
            pRxDesc->bufPtr      = 0x0;
            pRxDesc->returnInfo  = 0x0;
            ETH_DESCR_FLUSH_INV(pPortCtrl, pRxDesc);
            if( (void*)pRxDesc == pQueueCtrl->pLastDescr)
                    break;
            pRxDesc = RX_NEXT_DESC_PTR(pRxDesc, pQueueCtrl);
        }
        pQueueCtrl->pCurrentDescr = pQueueCtrl->pFirstDescr;
        pQueueCtrl->pUsedDescr = pQueueCtrl->pFirstDescr;
        
        /* update HW */
	MV_REG_WRITE( FIRST_RX_DESC_PTR(pPortCtrl->portNo, queue),
                 (MV_U32)ethDescVirtToPhy(pQueueCtrl, pQueueCtrl->pFirstDescr) );

        MV_REG_WRITE( CURR_RX_DESC_PTR(pPortCtrl->portNo, queue),
                 (MV_U32)ethDescVirtToPhy(pQueueCtrl, pQueueCtrl->pCurrentDescr) );
    }
    else
    {
        /* update HW */
        MV_REG_WRITE( CURR_RX_DESC_PTR(pPortCtrl->portNo, queue), 0);
    }
}

/*******************************************************************************
* etherInitTxDescRing - Curve a Tx chain desc list and buffer in memory.
*
* DESCRIPTION:
*       This function prepares a Tx chained list of descriptors and packet
*       buffers in a form of a ring. The routine must be called after port
*       initialization routine and before port start routine.
*       The Ethernet SDMA engine uses CPU bus addresses to access the various
*       devices in the system (i.e. DRAM). This function uses the ethernet
*       struct 'virtual to physical' routine (set by the user) to set the ring
*       with physical addresses.
*
* INPUT:
*       ETH_PORT_CTRL   *pEthPortCtrl   Ethernet Port Control srtuct.
*       int             txQueue         Number of Tx queue.
*       int             txDescNum       Number of Tx descriptors
*       int             txBuffSize      Size of Tx buffer
*       MV_U8*          pTxDescBase     Tx descriptors memory area base addr.
*
* OUTPUT:
*       The routine updates the Ethernet port control struct with information
*       regarding the Tx descriptors and buffers.
*
* RETURN:   None.
*
*******************************************************************************/
static void ethInitTxDescRing(ETH_PORT_CTRL* pPortCtrl, int queue)
{

    ETH_TX_DESC     *pTxDescBase, *pTxDesc, *pTxPrevDesc; 
    int             ix, txDescNum = pPortCtrl->txQueueConfig[queue].descrNum;
    ETH_QUEUE_CTRL  *pQueueCtrl = &pPortCtrl->txQueue[queue];

    /* Make sure descriptor address is cache line size aligned  */
    pTxDescBase = (ETH_TX_DESC*)MV_ALIGN_UP((MV_ULONG)pQueueCtrl->descBuf.bufVirtPtr, 
                                     CPU_D_CACHE_LINE_SIZE);

    pTxDesc      = (ETH_TX_DESC*)pTxDescBase;
    pTxPrevDesc  = pTxDesc;

    /* initialize the Tx descriptors ring */
    for (ix=0; ix<txDescNum; ix++)
    {
        pTxDesc->byteCnt     = 0x0000;
        pTxDesc->reserved    = 0x0000;
        pTxDesc->cmdSts      = ETH_BUFFER_OWNED_BY_HOST;
        pTxDesc->bufPtr      = 0x0;
        pTxDesc->returnInfo  = 0x0;

        pTxPrevDesc = pTxDesc;

        if(ix == (txDescNum-1))
        {
            /* Closing Tx descriptors ring */
            pTxPrevDesc->nextDescPtr = (MV_U32)ethDescVirtToPhy(pQueueCtrl, (void*)pTxDescBase);
        }
        else
        {
            pTxDesc = (ETH_TX_DESC*)((MV_ULONG)pTxDesc + ETH_TX_DESC_ALIGNED_SIZE);
            pTxPrevDesc->nextDescPtr = (MV_U32)ethDescVirtToPhy(pQueueCtrl, (void*)pTxDesc);
        }
        ETH_DESCR_FLUSH_INV(pPortCtrl, pTxPrevDesc);
    }

    pQueueCtrl->pCurrentDescr = pTxDescBase;
    pQueueCtrl->pUsedDescr = pTxDescBase;
    
    pQueueCtrl->pFirstDescr = pTxDescBase;
    pQueueCtrl->pLastDescr = pTxDesc;
    /* Leave one TX descriptor out of use */
    pQueueCtrl->resource = txDescNum - 1;
}

/******************************************************************************/

void ethResetTxDescRing(void* pPortHndl, int queue)
{
    ETH_PORT_CTRL*  pPortCtrl = (ETH_PORT_CTRL*)pPortHndl;
    ETH_QUEUE_CTRL* pQueueCtrl = &pPortCtrl->txQueue[queue];
    ETH_TX_DESC*    pTxDesc = (ETH_TX_DESC*)pQueueCtrl->pFirstDescr;
    pQueueCtrl->resource = 0;
    if(pQueueCtrl->pFirstDescr != NULL)
    {
        while(MV_TRUE)
        {
            pTxDesc->byteCnt     = 0x0000;
            pTxDesc->reserved    = 0x0000;
            pTxDesc->cmdSts      = ETH_BUFFER_OWNED_BY_HOST;
            pTxDesc->bufPtr      = 0x0;
            pTxDesc->returnInfo  = 0x0;
            ETH_DESCR_FLUSH_INV(pPortCtrl, pTxDesc);
            pQueueCtrl->resource++;
            if( (void*)pTxDesc == pQueueCtrl->pLastDescr)
                    break;
            pTxDesc = TX_NEXT_DESC_PTR(pTxDesc, pQueueCtrl);
        }
	/* Leave one TX descriptor out of use */
        pQueueCtrl->resource--;
        pQueueCtrl->pCurrentDescr = pQueueCtrl->pFirstDescr;
        pQueueCtrl->pUsedDescr = pQueueCtrl->pFirstDescr;

        /* update HW */
	MV_REG_WRITE( CURR_TX_DESC_PTR(pPortCtrl->portNo, queue), 
			(MV_U32)ethDescVirtToPhy(pQueueCtrl, pQueueCtrl->pCurrentDescr) );
    }
    else
    {
        /* update HW */
        MV_REG_WRITE( CURR_TX_DESC_PTR(pPortCtrl->portNo, queue), 0 );    
    }
}

/*******************************************************************************
* ethAllocDescrMemory - Allocate memory for RX and TX descriptors.
*
* DESCRIPTION:
*       This function allocates memory for RX and TX descriptors.
*       - If ETH_DESCR_IN_SDRAM defined, allocate memory in SDRAM.
*
* INPUT:
*       int size - size of memory should be allocated.
*
* RETURN: None
*
*******************************************************************************/
MV_U8*  ethAllocDescrMemory(ETH_PORT_CTRL* pPortCtrl, int descSize, 
                            MV_ULONG* pPhysAddr)
{
    MV_U8*  pVirt;

#ifdef ETH_DESCR_UNCACHED
    pVirt = (char*)mvOsIoUncachedMalloc(NULL, descSize, pPhysAddr);
#else
    pVirt = (char*)mvOsIoCachedMalloc(NULL, descSize, pPhysAddr);
#endif
    memset(pVirt, 0, descSize);

    return pVirt;
}

/*******************************************************************************
* ethFreeDescrMemory - Free memory allocated for RX and TX descriptors.
*
* DESCRIPTION:
*       This function frees memory allocated for RX and TX descriptors.
*       - If ETH_DESCR_IN_SDRAM defined, free memory using mvOsFree() function.
*
* INPUT:
*       void* pVirtAddr - virtual pointer to memory allocated for RX and TX
*                       desriptors.        
*
* RETURN: None
*
*******************************************************************************/
void    ethFreeDescrMemory(ETH_PORT_CTRL* pPortCtrl, MV_BUF_INFO* pDescBuf)
{
    if( (pDescBuf == NULL) || (pDescBuf->bufVirtPtr == NULL) )
        return;

#ifdef ETH_DESCR_UNCACHED
    mvOsIoUncachedFree(NULL, pDescBuf->bufSize, pDescBuf->bufPhysAddr, 
                     pDescBuf->bufVirtPtr);
#else
    mvOsIoCachedFree(NULL, pDescBuf->bufSize, pDescBuf->bufPhysAddr, 
                     pDescBuf->bufVirtPtr);
#endif
}
                                                                                                                            
/******************************************************************************/
/*                Other Functions                                             */
/******************************************************************************/


/*******************************************************************************
* ethBCopy - Copy bytes from source to destination
*
* DESCRIPTION:
*       This function supports the eight bytes limitation on Tx buffer size.
*       The routine will zero eight bytes starting from the destination address
*       followed by copying bytes from the source address to the destination.
*
* INPUT:
*       unsigned int srcAddr    32 bit source address.
*       unsigned int dstAddr    32 bit destination address.
*       int        byteCount    Number of bytes to copy.
*
* OUTPUT:
*       See description.
*
* RETURN:
*       None.
*
*******************************************************************************/
static void ethBCopy(char* srcAddr, char* dstAddr, int byteCount)
{
    while(byteCount != 0)
    {
        *dstAddr = *srcAddr;
        dstAddr++;
        srcAddr++;
        byteCount--;
    }
}

/******************************************************************************/

/* Return port handler */
void*   mvEthPortHndlGet(int port)
{
    if( (port < 0) || (port >= BOARD_ETH_PORT_NUM) )
    {
        mvOsPrintf("eth port #%d is not exist\n", port);
        return NULL;
    }
    return  ethPortCtrl[port];
}

/******************************************************************************/
/* Can be used for debug purposes: */
void mvEthPrintAllRegs(int port)
{
    unsigned int    regData = 0;
    MV_U32	    winNum = 0;
    
    if( (port < 0) || (port >= BOARD_ETH_PORT_NUM) )
    {
        mvOsPrintf("eth port #%d is not exist\n", port);
        return;
    }

    printk("************ UniMAC Register Information: ****************\n");

    regData = MV_REG_READ(ETH_SMI_REG);
    printk("ETH_SMI_REG = 0x%X\n", regData);

    regData = MV_REG_READ(ETH_PORT_CONFIG_REG(port));
    printk("ETH_PORT_CONFIG_REG = 0x%X\n", regData);

    regData = MV_REG_READ(ETH_PORT_CONFIG_EXTEND_REG(port));
    printk("ETH_PORT_CONFIG_EXTEND_REG = 0x%X\n", regData);

    regData = MV_REG_READ(ETH_PORT_COMMAND_REG(port));
    printk("ETH_PORT_COMMAND_REG = 0x%X\n", regData);

    regData = MV_REG_READ(ETH_PORT_STATUS_REG(port));
    printk("ETH_PORT_STATUS_REG = 0x%X\n", regData);

    regData = MV_REG_READ(ETH_HASH_TABLE_BASE_ADDR_REG(port));
    printk("ETH_HASH_TABLE_BASE_ADDR_REG = 0x%X\n", regData);

    regData = MV_REG_READ(ETH_SDMA_CONFIG_REG(port));
    printk("ETH_SDMA_CONFIG_REG = 0x%X\n", regData);

    regData = MV_REG_READ(ETH_SDMA_COMMAND_REG(port));
    printk("ETH_SDMA_COMMAND_REG = 0x%X\n", regData);

    regData = MV_REG_READ(ETH_INTR_CAUSE_REG(port));
    printk("ETH_INTR_CAUSE_REG = 0x%X\n", regData);

    regData = MV_REG_READ(ETH_RESET_SELECT_REG(port));
    printk("ETH_RESET_SELECT_REG = 0x%X\n", regData);

    regData = MV_REG_READ(ETH_INTR_MASK_REG(port));
    printk("ETH_INTR_MASK_REG = 0x%X\n", regData);

    regData = MV_REG_READ(FIRST_RX_DESC_PTR0(port));
    printk("FIRST_RX_DESC_PTR0 = 0x%X\n", regData);

    regData = MV_REG_READ(FIRST_RX_DESC_PTR1(port));
    printk("FIRST_RX_DESC_PTR1 = 0x%X\n", regData);

    regData = MV_REG_READ(FIRST_RX_DESC_PTR2(port));
    printk("FIRST_RX_DESC_PTR2 = 0x%X\n", regData);

    regData = MV_REG_READ(FIRST_RX_DESC_PTR3(port));
    printk("FIRST_RX_DESC_PTR3 = 0x%X\n", regData);

    regData = MV_REG_READ(CURR_RX_DESC_PTR0(port));
    printk("CURR_RX_DESC_PTR0 = 0x%X\n", regData);

    regData = MV_REG_READ(CURR_RX_DESC_PTR1(port));
    printk("CURR_RX_DESC_PTR1 = 0x%X\n", regData);

    regData = MV_REG_READ(CURR_RX_DESC_PTR2(port));
    printk("CURR_RX_DESC_PTR2 = 0x%X\n", regData);

    regData = MV_REG_READ(CURR_RX_DESC_PTR3(port));
    printk("CURR_RX_DESC_PTR3 = 0x%X\n", regData);

    regData = MV_REG_READ(CURR_TX_DESC_PTR0(port));
    printk("CURR_TX_DESC_PTR0 = 0x%X\n", regData);

    regData = MV_REG_READ(CURR_TX_DESC_PTR1(port));
    printk("CURR_TX_DESC_PTR1 = 0x%X\n", regData);

    for(winNum=0; winNum<ETH_MAX_DECODE_WIN; winNum++) {
	regData = MV_REG_READ(ETH_WIN_CONTROL_REG(winNum));
	printk("ETH_WIN_CONTROL_REG%d = 0x%X\n", winNum, regData);
    }
    for(winNum=0; winNum<ETH_MAX_DECODE_WIN; winNum++) {
	regData = MV_REG_READ(ETH_WIN_BASE_REG(winNum));
	printk("ETH_WIN_BASE_REG%d = 0x%X\n", winNum, regData);
    }

    printk("**********************************************************\n");


}

/******************************************************************************/











