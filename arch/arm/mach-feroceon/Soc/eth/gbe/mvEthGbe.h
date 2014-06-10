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
* mvEth.h - Header File for : Marvell Gigabit Ethernet Controller
*
* DESCRIPTION:
*       This header file contains macros typedefs and function declaration specific to 
*       the Marvell Gigabit Ethernet Controller.
*
* DEPENDENCIES:
*       None.
*
*******************************************************************************/

#ifndef __mvEthGbe_h__
#define __mvEthGbe_h__


static INLINE void      mvEthPortTxEnable(void* pPortHndl)
{
    ETH_PORT_CTRL*      pPortCtrl = (ETH_PORT_CTRL*)pPortHndl;

    MV_REG_VALUE(ETH_TX_QUEUE_COMMAND_REG(pPortCtrl->portNo)) = pPortCtrl->portTxQueueCmdReg;
}

static INLINE MV_ULONG  ethDescVirtToPhy(ETH_QUEUE_CTRL* pQueueCtrl, MV_U8* pDesc)
{
#if defined (ETH_DESCR_IN_SRAM)
    if( ethDescInSram )
        return mvSramVirtToPhy(pDesc);
    else
#endif /* ETH_DESCR_IN_SRAM */
        return (pQueueCtrl->descBuf.bufPhysAddr + (pDesc - pQueueCtrl->descBuf.bufVirtPtr));
}

/* defines  */
#define ETH_CSUM_MIN_BYTE_COUNT     72
#if (MV_ETH_VERSION <= 1)
#define ETH_CSUM_MAX_BYTE_COUNT     9*1024
#else
#define ETH_CSUM_MAX_BYTE_COUNT     2*1024
#endif /* MV_ETH_VERSION */

/* An offest in Tx descriptors to store data for buffers less than 8 Bytes */
#define MIN_TX_BUFF_LOAD            8
#define TX_BUF_OFFSET_IN_DESC       (ETH_TX_DESC_ALIGNED_SIZE - MIN_TX_BUFF_LOAD)

/* Default port configuration value */
#define PORT_CONFIG_VALUE                       \
             ETH_DEF_RX_QUEUE_MASK(0)       |   \
             ETH_DEF_RX_ARP_QUEUE_MASK(0)   |   \
             ETH_DEF_RX_TCP_QUEUE_MASK(0)   |   \
             ETH_DEF_RX_UDP_QUEUE_MASK(0)   |   \
             ETH_DEF_RX_BPDU_QUEUE_MASK(0)  |   \
             ETH_RX_CHECKSUM_WITH_PSEUDO_HDR

/* Default port extend configuration value */
#define PORT_CONFIG_EXTEND_VALUE            0

#define PORT_SERIAL_CONTROL_VALUE                           \
            ETH_DISABLE_FC_AUTO_NEG_MASK                |   \
            BIT9                                        |   \
            ETH_DO_NOT_FORCE_LINK_FAIL_MASK             |   \
            ETH_MAX_RX_PACKET_1552BYTE                  |   \
            ETH_SET_FULL_DUPLEX_MASK

#define PORT_SERIAL_CONTROL_100MB_FORCE_VALUE               \
            ETH_FORCE_LINK_PASS_MASK                    |   \
            ETH_DISABLE_DUPLEX_AUTO_NEG_MASK            |   \
            ETH_DISABLE_FC_AUTO_NEG_MASK                |   \
            BIT9                                        |   \
            ETH_DO_NOT_FORCE_LINK_FAIL_MASK             |   \
            ETH_DISABLE_SPEED_AUTO_NEG_MASK             |   \
            ETH_SET_FULL_DUPLEX_MASK                    |   \
            ETH_SET_MII_SPEED_100_MASK                  |   \
            ETH_MAX_RX_PACKET_1552BYTE


#define PORT_SERIAL_CONTROL_1000MB_FORCE_VALUE              \
            ETH_FORCE_LINK_PASS_MASK                    |   \
            ETH_DISABLE_DUPLEX_AUTO_NEG_MASK            |   \
            ETH_DISABLE_FC_AUTO_NEG_MASK                |   \
            BIT9                                        |   \
            ETH_DO_NOT_FORCE_LINK_FAIL_MASK             |   \
            ETH_DISABLE_SPEED_AUTO_NEG_MASK             |   \
            ETH_SET_FULL_DUPLEX_MASK                    |   \
            ETH_SET_GMII_SPEED_1000_MASK                |   \
            ETH_MAX_RX_PACKET_1552BYTE

#define PORT_SERIAL_CONTROL_SGMII_IBAN_VALUE                \
            ETH_DISABLE_FC_AUTO_NEG_MASK                |   \
            BIT9                                        |   \
            ETH_IN_BAND_AN_EN_MASK                      |   \
            ETH_DO_NOT_FORCE_LINK_FAIL_MASK             |   \
            ETH_MAX_RX_PACKET_1552BYTE                  

/* Function headers: */
MV_VOID     mvEthSetSpecialMcastTable(int portNo, int queue);
MV_STATUS   mvEthArpRxQueue(void* pPortHandle, int arpQueue);
MV_STATUS   mvEthUdpRxQueue(void* pPortHandle, int udpQueue);
MV_STATUS   mvEthTcpRxQueue(void* pPortHandle, int tcpQueue);
MV_STATUS   mvEthMacAddrGet(int portNo, unsigned char *pAddr);
MV_VOID     mvEthSetOtherMcastTable(int portNo, int queue);
/* Interrupt Coalesting functions */
MV_U32      mvEthRxCoalSet(void* pPortHndl, MV_U32 uSec);
MV_U32      mvEthTxCoalSet(void* pPortHndl, MV_U32 uSec);
MV_STATUS   mvEthCoalGet(void* pPortHndl, MV_U32* pRxCoal, MV_U32* pTxCoal);



#endif /* __mvEthGbe_h__ */


