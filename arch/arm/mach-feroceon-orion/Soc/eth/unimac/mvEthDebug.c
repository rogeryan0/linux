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
* mvEthDebug.c - Source file for user friendly debug functions
*
* DESCRIPTION:
*
* DEPENDENCIES:
*       None.
*
*******************************************************************************/

#include "mvOs.h"
#include "mvEth.h"
#include "mvEthUnimac.h"
#include "mvEthPolicy.h"
#include "mvCommon.h"
#include "mvTypes.h"
#include "mv802_3.h"
#include "mvDebug.h"
#include "mvCtrlEnvLib.h"
#include "mvEthPhy.h"

extern MV_BOOL         ethDescInSram;
extern MV_BOOL         ethDescSwCoher;


void    mvEthPortShow(void* pHndl);
void    mvEthQueuesShow(void* pHndl, int rxQueue, int txQueue, int mode);
/******************************************************************************/
/*                          Debug functions                                   */
/******************************************************************************/
void    ethRxCoal(int port, int usec)
{
    mvOsPrintf("\n\t No support Rx interrupt coalescing in UniMAC\n\n");   
}

void    ethTxCoal(int port, int usec)
{
    mvOsPrintf("\n\t No support Tx interrupt coalescing in UniMAC\n\n");     
}

void    ethBpduRxQ(int port, int bpduQueue)
{
    void*   pHndl;

    pHndl = mvEthPortHndlGet(port);
    if(pHndl != NULL)
    {
        mvEthBpduRxQueue(pHndl, bpduQueue);
    }
}

void    ethArpRxQ(int port, int arpQueue)
{
    void*   pHndl;

    pHndl = mvEthPortHndlGet(port);
    if(pHndl != NULL)
    {
        mvEthArpRxQueue(pHndl, arpQueue);
    }
}

void    ethTcpRxQ(int port, int tcpQueue)
{
    void*   pHndl;

    pHndl = mvEthPortHndlGet(port);
    if(pHndl != NULL)
    {
        mvEthTcpRxQueue(pHndl, tcpQueue);
    }
}

void    ethUdpRxQ(int port, int udpQueue)
{
    void*   pHndl;

    pHndl = mvEthPortHndlGet(port);
    if(pHndl != NULL)
    {
        mvEthUdpRxQueue(pHndl, udpQueue);
    }
}

#ifdef INCLUDE_MULTI_QUEUE
void    ethRxPolMode(int port, MV_ETH_PRIO_MODE prioMode)
{
    void*   pHndl;

    pHndl = mvEthRxPolicyHndlGet(port);
    if(pHndl != NULL)
    {
        mvEthRxPolicyModeSet(pHndl, prioMode);
    }
}

void    ethRxPolQ(int port, int queue, int quota)
{
    void*   pHndl;

    pHndl = mvEthRxPolicyHndlGet(port);
    if(pHndl != NULL)
    {
        mvEthRxPolicyQueueCfg(pHndl, queue, quota);
    }
}

void    ethRxPolicy(int port)
{
    int             queue;
    ETH_RX_POLICY*  pRxPolicy = (ETH_RX_POLICY*)mvEthRxPolicyHndlGet(port);
    ETH_PORT_CTRL*  pPortCtrl = (ETH_PORT_CTRL*)mvEthPortHndlGet(port);

    if( (pPortCtrl == NULL) || (pRxPolicy == NULL) )
    {
        return;
    }
 
    mvOsPrintf("rxDefQ=%d, arpQ=%d, bpduQ=%d, tcpQ=%d, udpQ=%d\n\n",
                pPortCtrl->portConfig.rxDefQ, pPortCtrl->portConfig.rxArpQ, 
                pPortCtrl->portConfig.rxBpduQ, 
                pPortCtrl->portConfig.rxTcpQ, pPortCtrl->portConfig.rxUdpQ); 

    mvOsPrintf("ethRxPolicy #%d: hndl=%p, mode=%s, curQ=%d, curQuota=%d\n",
                pRxPolicy->port, pRxPolicy, 
                (pRxPolicy->rxPrioMode == MV_ETH_PRIO_FIXED) ? "FIXED" : "WRR",
                pRxPolicy->rxCurQ, pRxPolicy->rxCurQuota);

    for(queue=0; queue<MV_ETH_RX_Q_NUM; queue++)
    {
        mvOsPrintf("\t rxQ #%d: rxQuota=%d\n", queue, pRxPolicy->rxQuota[queue]);
    }
}


void    ethTxPolDef(int port, int txQ, char* headerHexStr)
{
    void*                   pHndl;
    MV_ETH_TX_POLICY_ENTRY  polEntry;

    pHndl = mvEthTxPolicyHndlGet(port);
    if(pHndl != NULL)
    {
        polEntry.txQ = txQ;
        polEntry.pHeader = headerHexStr;
        if(headerHexStr != NULL)
            polEntry.headerSize = strlen(headerHexStr);
        else
            polEntry.headerSize = 0;
        mvEthTxPolicyDef(pHndl, &polEntry);
    }
}


#define MV_MAX_HEADER_LEN    64
void    ethTxPolDA(int port, char* macStr, int txQ, char* headerHexStr)
{
    void*                   pHndl;
    MV_ETH_TX_POLICY_MACDA  daPolicy;
    MV_U8           header[MV_MAX_HEADER_LEN];

    pHndl = mvEthTxPolicyHndlGet(port);
    if(pHndl == NULL)
        return;

    mvMacStrToHex(macStr, daPolicy.macDa);
    if(txQ > 0)
    {
        daPolicy.policy.txQ = txQ;
        if(headerHexStr != NULL)
        {
            /* each two char are one byte '55'-> 0x55 */
            daPolicy.policy.headerSize = strlen(headerHexStr)/2; 
            if(daPolicy.policy.headerSize > MV_MAX_HEADER_LEN) 
                return;
            
            mvAsciiToHex(headerHexStr, header);
            daPolicy.policy.pHeader = header;
        }
        else
            daPolicy.policy.headerSize = 0;

        mvEthTxPolicyAdd(pHndl, &daPolicy);
    }
    else
    {
        mvEthTxPolicyDel(pHndl, daPolicy.macDa);
    }
}

void    ethTxPolicy(int port)
{
    ETH_TX_POLICY*  pTxPolicy = (ETH_TX_POLICY*) mvEthTxPolicyHndlGet(port);
    int             idx,i,queue;
    char            macStr[MV_MAC_STR_SIZE];

    ETH_PORT_CTRL*  pPortCtrl = (ETH_PORT_CTRL*)mvEthPortHndlGet(port);

    if((pPortCtrl == NULL) || (pTxPolicy == NULL))
    {
        return;
    }

    for(queue=0; queue<MV_ETH_TX_Q_NUM; queue++)
    {
        mvOsPrintf("TX Queue #%d: mode=%s, quota=%d\n", 
                queue,   (pPortCtrl->txQueueConfig[queue].prioMode == MV_ETH_PRIO_FIXED) ? "FIXED" : "WRR",
                pPortCtrl->txQueueConfig[queue].quota );
    }

    mvOsPrintf("\nethDefTxPolicy : hndl=%p, defQ=%d, defHeader=%p, defSize=%d, maxDa=%d\n\n",
                pTxPolicy, pTxPolicy->txPolDef.txQ,
                pTxPolicy->txPolDef.pHeader, pTxPolicy->txPolDef.headerSize,
                pTxPolicy->txPolMaxDa);
    if(pTxPolicy->txPolDef.headerSize != 0) 
    {
    for(i = 0; i < pTxPolicy->txPolDef.headerSize; i++) {
        mvOsPrintf(" %02x",pTxPolicy->txPolDef.pHeader[i]);
        if( (i & 0xf)  == 0xf ) 
            mvOsPrintf("\n");
    }
    mvOsPrintf("\n");
    }

    for(idx=0; idx<pTxPolicy->txPolMaxDa; idx++)
    {
        if(pTxPolicy->txPolDa[idx].policy.txQ == MV_INVALID)
            continue;

        mvMacHexToStr(pTxPolicy->txPolDa[idx].macDa, macStr);
        mvOsPrintf("%d. MAC = %s, txQ=%d, pHeader=%p, headerSize=%d\n",
                    idx, macStr, pTxPolicy->txPolDa[idx].policy.txQ,
                    pTxPolicy->txPolDa[idx].policy.pHeader, 
                    pTxPolicy->txPolDa[idx].policy.headerSize);
        if(pTxPolicy->txPolDa[idx].policy.headerSize != 0) 
        {
            for(i = 0; i < pTxPolicy->txPolDa[idx].policy.headerSize; i++)  
            {
                mvOsPrintf(" %02x",pTxPolicy->txPolDa[idx].policy.pHeader[i]);
                if( (i & 0xf)  == 0xf ) 
                    mvOsPrintf("\n");
            }
            mvOsPrintf("\n");
        }
    }
}
#endif /* INCLUDE_MULTI_QUEUE */

/* Print important registers of Ethernet port */
void    ethPortRegs(int port)
{
	int i = 0;

    mvOsPrintf("\t eth #%d Port Registers:\n", port);

    mvOsPrintf("ETH_PORT_STATUS_REG                 : 0x%X = 0x%08x\n", 
                ETH_PORT_STATUS_REG(port), 
                MV_REG_READ( ETH_PORT_STATUS_REG(port) ) );

    mvOsPrintf("ETH_PORT_CONFIG_REG                 : 0x%X = 0x%08x\n", 
                ETH_PORT_CONFIG_REG(port), 
                MV_REG_READ( ETH_PORT_CONFIG_REG(port) ) );    

    mvOsPrintf("ETH_PORT_CONFIG_EXTEND_REG          : 0x%X = 0x%08x\n", 
                ETH_PORT_CONFIG_EXTEND_REG(port), 
                MV_REG_READ( ETH_PORT_CONFIG_EXTEND_REG(port) ) );    

    mvOsPrintf("ETH_SDMA_CONFIG_REG                 : 0x%X = 0x%08x\n", 
                ETH_SDMA_CONFIG_REG(port), 
                MV_REG_READ( ETH_SDMA_CONFIG_REG(port) ) );    

    mvOsPrintf("ETH_INTR_CAUSE_REG                  : 0x%X = 0x%08x\n", 
                ETH_INTR_CAUSE_REG(port), 
                MV_REG_READ( ETH_INTR_CAUSE_REG(port) ) );    

    mvOsPrintf("ETH_INTR_MASK_REG                   : 0x%X = 0x%08x\n", 
                ETH_INTR_MASK_REG(port), 
                MV_REG_READ( ETH_INTR_MASK_REG(port) ) );    

	for (i = 0; i < MV_ETH_RX_Q_NUM; i++) 
	{
		mvOsPrintf("FIRST_RX_DESC_PTR             : 0x%X = 0x%08x\n", 
                FIRST_RX_DESC_PTR(port, i), 
                MV_REG_READ( FIRST_RX_DESC_PTR(port, i) ) );
	}

	for (i = 0; i < MV_ETH_RX_Q_NUM; i++) 
	{
		mvOsPrintf("CURR_RX_DESC_PTR             : 0x%X = 0x%08x\n", 
                CURR_RX_DESC_PTR(port, i), 
                MV_REG_READ( CURR_RX_DESC_PTR(port, i) ) );
	}

	for (i = 0; i < MV_ETH_TX_Q_NUM; i++) 
	{
		mvOsPrintf("CURR_TX_DESC_PTR             : 0x%X = 0x%08x\n", 
                CURR_TX_DESC_PTR(port, i), 
                MV_REG_READ( CURR_TX_DESC_PTR(port, i) ) );
	}
    
}


/* Print UniMAC registers */
void    ethRegs(void)
{
    int win;

    mvOsPrintf("ETH_SMI_REG                    : 0x%X = 0x%08x\n", 
                ETH_SMI_REG, 
                MV_REG_READ(ETH_SMI_REG) );    

    for(win=0; win<ETH_MAX_DECODE_WIN; win++)
    {
        mvOsPrintf("\nAddrDecWin #%d\n", win);
        mvOsPrintf("\tETH_WIN_BASE_REG(win)       : 0x%X = 0x%08x\n", 
                ETH_WIN_BASE_REG(win), 
                MV_REG_READ( ETH_WIN_BASE_REG(win)) );    
        mvOsPrintf("\ETH_WIN_CONTROL_REG(win)       : 0x%X = 0x%08x\n", 
                ETH_WIN_CONTROL_REG(win), 
                MV_REG_READ( ETH_WIN_CONTROL_REG(win)) );    
    }
}

/******************************************************************************/
/*                      MIB Counters functions                                */
/******************************************************************************/

/*******************************************************************************
* ethClearMibCounters - Clear all MIB counters
*
* DESCRIPTION:
*       This function clears all MIB counters of a specific ethernet port.
*       A read from the MIB counter will reset the counter.
*
* INPUT:
*       int    port -  Ethernet Port number.
*
* RETURN: None
*
*******************************************************************************/
void ethClearCounters(int port)
{
    mvOsPrintf("\n\t No support for MIB counters in UniMAC\n\n");   
}


/* Print counters of the Ethernet port */
void    ethPortCounters(int port)
{  
    void*   pHndl;

    pHndl = mvEthPortHndlGet(port);
    if(pHndl == NULL)
        return;

    mvOsPrintf("\n\t No support for MIB counters in UniMAC\n\n");   
}

/* Print RMON counters of the Ethernet port */
void    ethPortRmonCounters(int port)
{
    void*   pHndl;

    pHndl = mvEthPortHndlGet(port);
    if(pHndl == NULL)
        return;

    mvOsPrintf("\n\t No support for RMON MIB counters in UniMAC\n\n");
}

/* Print port information */
void    ethPortStatus(int port)
{
    void*   pHndl;

    pHndl = mvEthPortHndlGet(port);
    if(pHndl != NULL)
    {
        mvEthPortShow(pHndl);
    }
}

/* Print port queues information */
void    ethPortQueues(int port, int rxQueue, int txQueue, int mode)  
{
    void*   pHndl;

    pHndl = mvEthPortHndlGet(port);
    if(pHndl != NULL)
    {
        mvEthQueuesShow(pHndl, rxQueue, txQueue, mode);
    }
}

void    ethUcastSet(int port, char* macStr, int queue)
{
    void*   pHndl;
    MV_U8   macAddr[MV_MAC_ADDR_SIZE];

    pHndl = mvEthPortHndlGet(port);
    if(pHndl != NULL)
    {
        mvMacStrToHex(macStr, macAddr);
        mvEthMacAddrSet(pHndl, macAddr, queue);
    }
}

void    ethMcastAdd(int port, char* macStr, int queue)
{
    void*   pHndl;
    MV_U8   macAddr[MV_MAC_ADDR_SIZE];

    pHndl = mvEthPortHndlGet(port);
    if(pHndl != NULL)
    {
        mvMacStrToHex(macStr, macAddr);
        mvEthMcastAddrSet(pHndl, macAddr, queue);
    }
}

void    ethPortMcast(int port)
{
    int     tblIdx, regIdx;
    MV_U32  smcTableReg;

    mvOsPrintf("\n\t Port #%d Special (IP) Multicast table: 01:00:5E:00:00:XX\n\n", 
                port);
#if 0
    for(tblIdx=0; tblIdx < 256/4; tblIdx++)
    {
        smcTableReg = MV_REG_READ((ETH_DA_FILTER_SPEC_MCAST_BASE(port) + tblIdx*4));
        for(regIdx=0; regIdx < 4; regIdx++)
        {
            if((smcTableReg & (0x01 << (regIdx*8))) != 0)
            {
                mvOsPrintf("%02X: Accepted, rxQ = %d\n", 
                    tblIdx*4+regIdx, (smcTableReg & (0xFF<<(regIdx *8))) >> 1);
            }
        }
    }
#endif
/*    mvOsPrintf("\n\t Port #%d Other Multicast table\n\n", port); */
}


/* Print status of Ethernet port */
void    mvEthPortShow(void* pHndl)
{
    MV_U32              regValue;
    int                 queue, port;
    ETH_PORT_CTRL*      pPortCtrl = (ETH_PORT_CTRL*)pHndl;
    unsigned char       macAddress[MV_MAC_ADDR_SIZE];

    port = pPortCtrl->portNo;

    regValue = MV_REG_READ( ETH_PORT_STATUS_REG(port) );

    mvOsPrintf("\t eth #%d port Status: 0x%04x = 0x%08x\n\n", 
                port, ETH_PORT_STATUS_REG(port), regValue);
    
    mvDebugPrintMacAddr(macAddress);
    mvOsPrintf("\n");
    /* Print all RX and TX queues */
    for(queue=0; queue<MV_ETH_RX_Q_NUM; queue++)
    {
        mvOsPrintf("RX Queue #%d: base=0x%lx, free=%d\n", 
                    queue, (MV_ULONG)pPortCtrl->rxQueue[queue].pFirstDescr,
                    mvEthRxResourceGet(pPortCtrl, queue) );
    }
    mvOsPrintf("\n");
    for(queue=0; queue<MV_ETH_TX_Q_NUM; queue++)
    {
        mvOsPrintf("TX Queue #%d: base=0x%lx, free=%d\n", 
                queue, (MV_ULONG)pPortCtrl->txQueue[queue].pFirstDescr,
                mvEthTxResourceGet(pPortCtrl, queue) );
    }
}

/* Print RX and TX queue of the Ethernet port */
void    mvEthQueuesShow(void* pHndl, int rxQueue, int txQueue, int mode)  
{
    ETH_PORT_CTRL   *pPortCtrl = (ETH_PORT_CTRL*)pHndl;
    ETH_QUEUE_CTRL  *pQueueCtrl;
    MV_U32          regValue;
    ETH_RX_DESC     *pRxDescr;
    ETH_TX_DESC     *pTxDescr;
    int             i, port = pPortCtrl->portNo;

    if( (rxQueue >=0) && (rxQueue < MV_ETH_RX_Q_NUM) )
    {
        pQueueCtrl = &(pPortCtrl->rxQueue[rxQueue]);
        mvOsPrintf("Port #%d, RX Queue #%d\n\n", port, rxQueue);

        mvOsPrintf("CURR_RX_DESC_PTR        : 0x%X = 0x%08x\n", 
            CURR_RX_DESC_PTR(port, rxQueue), 
            MV_REG_READ( CURR_RX_DESC_PTR(port, rxQueue)));


        if(pQueueCtrl->pFirstDescr != NULL)
        {
            mvOsPrintf("pFirstDescr=0x%lx, pLastDescr=0x%lx, numOfResources=%d\n",
                (MV_ULONG)pQueueCtrl->pFirstDescr, (MV_ULONG)pQueueCtrl->pLastDescr, 
                pQueueCtrl->resource);
            mvOsPrintf("pCurrDescr: 0x%lx, pUsedDescr: 0x%lx\n",
                (MV_ULONG)pQueueCtrl->pCurrentDescr, 
                (MV_ULONG)pQueueCtrl->pUsedDescr);

            if(mode == 1)
            {
                pRxDescr = (ETH_RX_DESC*)pQueueCtrl->pFirstDescr;
                i = 0; 
                do 
                {
                    mvOsPrintf("%3d. pDescr=0x%lx, cmdSts=0x%08x, byteCnt=%4d, bufSize=%4d, pBuf=0x%08x, rInfo=0x%lx\n", 
                                i, (MV_ULONG)pRxDescr, pRxDescr->cmdSts, pRxDescr->byteCnt, 
                                (MV_U32)pRxDescr->bufSize, (unsigned int)pRxDescr->bufPtr, (MV_ULONG)pRxDescr->returnInfo);

                    ETH_DESCR_INV(pPortCtrl, pRxDescr);
                    pRxDescr = RX_NEXT_DESC_PTR(pRxDescr, pQueueCtrl);
                    i++;
                } while (pRxDescr != pQueueCtrl->pFirstDescr);
            }
        }
        else
            mvOsPrintf("RX Queue #%d is NOT CREATED\n", rxQueue);
    }

    if( (txQueue >=0) && (txQueue < MV_ETH_TX_Q_NUM) )
    {
        pQueueCtrl = &(pPortCtrl->txQueue[txQueue]);
        mvOsPrintf("Port #%d, TX Queue #%d\n\n", port, txQueue);

        regValue = MV_REG_READ( CURR_TX_DESC_PTR(port, txQueue));
        mvOsPrintf("CURR_TX_DESC_PTR        : 0x%X = 0x%08x\n", 
                    CURR_TX_DESC_PTR(port, txQueue), regValue);

        if(pQueueCtrl->pFirstDescr != NULL)
        {
            mvOsPrintf("pFirstDescr=0x%lx, pLastDescr=0x%lx, numOfResources=%d\n",
                       (MV_ULONG)pQueueCtrl->pFirstDescr, 
                       (MV_ULONG)pQueueCtrl->pLastDescr, 
                        pQueueCtrl->resource);
            mvOsPrintf("pCurrDescr: 0x%lx, pUsedDescr: 0x%lx\n",
                       (MV_ULONG)pQueueCtrl->pCurrentDescr, 
                       (MV_ULONG)pQueueCtrl->pUsedDescr);

            if(mode == 1)
            {
                pTxDescr = (ETH_TX_DESC*)pQueueCtrl->pFirstDescr;
                i = 0; 
                do 
                {
                    mvOsPrintf("%3d. pDescr=0x%lx, cmdSts=0x%08x, byteCnt=%4d, pBuf=0x%08x, rInfo=0x%lx, pAlign=0x%lx\n", 
                                i, (MV_ULONG)pTxDescr, pTxDescr->cmdSts, pTxDescr->byteCnt, 
                                (MV_U32)pTxDescr->bufPtr, (MV_ULONG)pTxDescr->returnInfo, (MV_ULONG)pTxDescr->alignBufPtr);

                    ETH_DESCR_INV(pPortCtrl, pTxDescr);
                    pTxDescr = TX_NEXT_DESC_PTR(pTxDescr, pQueueCtrl);
                    i++;
                } while (pTxDescr != pQueueCtrl->pFirstDescr);
            }
        }
        else
            mvOsPrintf("TX Queue #%d is NOT CREATED\n", txQueue);
    }
}

