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


#include "mvTypes.h"
#include "mv802_3.h"
#include "mvCommon.h"
#include "mvOs.h"
#include "mvCtrlEnvLib.h"
#include "mvCpu.h"
#include "mvCpuIf.h"
#include "mvEthPhy.h"
#include "mvEth.h"



typedef struct _mvEthDecWin
{
    MV_TARGET     target;
    MV_ADDR_WIN   addrWin;  /* An address window*/
    MV_BOOL       enable;   /* Address decode window is enabled/disabled */
  
}MV_ETH_DEC_WIN;

MV_TARGET ethAddrDecPrioTap[] =
{
#if defined(MV_INCLUDE_SDRAM_CS0)
	SDRAM_CS0,
#endif
#if defined(MV_INCLUDE_SDRAM_CS1)
	SDRAM_CS1,
#endif
#if defined(MV_INCLUDE_SDRAM_CS2)
	SDRAM_CS2,
#endif
#if defined(MV_INCLUDE_SDRAM_CS3)
	SDRAM_CS3,
#endif
#if defined(MV_INCLUDE_DEVICE_CS0)
        DEVICE_CS0,
#endif
#if defined(MV_INCLUDE_DEVICE_CS1)
   	DEVICE_CS1,
#endif
#if defined(MV_INCLUDE_DEVICE_CS2)
   	DEVICE_CS2,
#endif
   	TBL_TERM
};

static MV_STATUS   ethWinOverlapDetect(MV_U32 winNum, MV_ADDR_WIN *pAddrWin);
static MV_STATUS   mvEthWinSet(MV_U32 winNum, MV_ETH_DEC_WIN *pAddrDecWin);
static MV_STATUS   mvEthWinGet(MV_U32 winNum, MV_ETH_DEC_WIN *pAddrDecWin);

                                                                                                                             
/*******************************************************************************
* mvEthWinInit - Initialize ETH address decode windows 
*
* DESCRIPTION:
*               This function initialize ETH window decode unit. It set the 
*               default address decode
*               windows of the unit.
*
* INPUT:
*       None.
*
* OUTPUT:
*       None.
*
* RETURN:
*       MV_ERROR if setting fail.
*******************************************************************************/
/* Configure EthDrv memory map registes. */
MV_STATUS 	mvEthWinInit(MV_VOID)
{
    MV_U32          winNum, status, regVal = 0;
    MV_ETH_DEC_WIN  ethWin;
    MV_CPU_DEC_WIN  cpuAddrDecWin;
    MV_U32          winPrioIndex = 0;

    /* Initiate Ethernet address decode */

    /* First disable all address decode windows */	
    for(winNum=0; winNum<ETH_MAX_DECODE_WIN; winNum++)
    {
		regVal = MV_REG_READ(ETH_WIN_CONTROL_REG(winNum));
		regVal &= ~ETH_WIN_EN_MASK;
		MV_REG_WRITE(ETH_WIN_CONTROL_REG(winNum), regVal);	
    }
    
    /* Go through all windows in user table until table terminator      */
    for (winNum=0; ((ethAddrDecPrioTap[winPrioIndex] != TBL_TERM) && 
                    (winNum < ETH_MAX_DECODE_WIN)); )
    {
        /* first get attributes from CPU If */
        status = mvCpuIfTargetWinGet(ethAddrDecPrioTap[winPrioIndex], 
                                     &cpuAddrDecWin);

        if(MV_NO_SUCH == status)
        {
            winPrioIndex++;
            continue;
        }
		if (MV_OK != status)
		{
			mvOsPrintf("mvEthWinInit: ERR. mvCpuIfTargetWinGet failed\n");
			return MV_ERROR;
		}

        if (cpuAddrDecWin.enable == MV_TRUE)
        {
            ethWin.addrWin.baseHigh = cpuAddrDecWin.addrWin.baseHigh;
            ethWin.addrWin.baseLow = cpuAddrDecWin.addrWin.baseLow;
            ethWin.addrWin.size = cpuAddrDecWin.addrWin.size;
            ethWin.enable = MV_TRUE;
            ethWin.target = ethAddrDecPrioTap[winPrioIndex];

            if(MV_OK != mvEthWinSet(winNum, &ethWin))
            {
                mvOsPrintf("mvEthWinInit: ERR. mvEthWinSet failed winNum=%d\n",
                           winNum);
                return MV_ERROR;
            }
            winNum++;
        }
        winPrioIndex ++;
    }

    /* set full access to all windows: UniMAC HW does not have access protection registers */
    
    return MV_OK;
}

/*******************************************************************************
* mvEthWinSet - Set ETH target address window
*
* DESCRIPTION:
*       This function sets a peripheral target (e.g. SDRAM bank0, PCI_MEM0)
*       address window, also known as address decode window.
*       After setting this target window, the ETH will be able to access the
*       target within the address window.
*
* INPUT:
*       winNum      - ETH to target address decode window number.
*       pAddrDecWin - ETH target window data structure.
*
* OUTPUT:
*       None.
*
* RETURN:
*       MV_ERROR if address window overlapps with other address decode windows.
*       MV_BAD_PARAM if base address is invalid parameter or target is
*       unknown.
*
*******************************************************************************/
MV_STATUS mvEthWinSet(MV_U32 winNum, MV_ETH_DEC_WIN *pAddrDecWin)
{
    MV_TARGET_ATTRIB    targetAttribs;
    MV_DEC_REGS         decRegs;
    
    /* Parameter checking   */
    if (winNum >= ETH_MAX_DECODE_WIN)
    {
        mvOsPrintf("mvEthWinSet: ERR. Invalid win num %d\n",winNum);
        return MV_BAD_PARAM;
    }    
    
    /* Check if the requested window overlapps with current windows     */
    if (MV_TRUE == ethWinOverlapDetect(winNum, &pAddrDecWin->addrWin))
    {
        mvOsPrintf("mvEthWinSet: ERR. Window %d overlap\n", winNum);
        return MV_ERROR;
    }

	/* check if address is aligned to the size */
	if(MV_IS_NOT_ALIGN(pAddrDecWin->addrWin.baseLow, pAddrDecWin->addrWin.size))
	{
		mvOsPrintf("mvEthWinSet: Error setting Ethernet window %d to "\
				   "target %s.\nAddress 0x%08x is unaligned to size 0x%x.\n",
				   winNum,
				   mvCtrlTargetNameGet(pAddrDecWin->target), 
				   pAddrDecWin->addrWin.baseLow,
				   pAddrDecWin->addrWin.size);
		return MV_ERROR;
	}

    
    decRegs.baseReg = MV_REG_READ(ETH_WIN_BASE_REG(winNum));
    decRegs.sizeReg = MV_REG_READ(ETH_WIN_CONTROL_REG(winNum));
    
    if (MV_OK != mvCtrlAddrDecToReg(&(pAddrDecWin->addrWin),&decRegs))
    {
        mvOsPrintf("mvEthWinSet:mvCtrlAddrDecToReg Failed\n");
        return MV_ERROR;
    }
    
    mvCtrlAttribGet(pAddrDecWin->target,&targetAttribs);
    
    /* set attributes */
    decRegs.sizeReg &= ~ETH_WIN_ATTR_MASK;
    decRegs.sizeReg |= targetAttribs.attrib << ETH_WIN_ATTR_OFFSET;
    /* set target ID */
    decRegs.sizeReg &= ~ETH_WIN_TARGET_MASK;
    decRegs.sizeReg |= targetAttribs.targetId << ETH_WIN_TARGET_OFFSET;
    
    /* for the safe side we disable the window before writing the new values */
    mvEthWinEnable(winNum,MV_FALSE);
    MV_REG_WRITE(ETH_WIN_BASE_REG(winNum), decRegs.baseReg);
    
    /* Write to address decode Size Register                            */
    MV_REG_WRITE(ETH_WIN_CONTROL_REG(winNum), decRegs.sizeReg);
    
    /* Enable address decode target window                              */
    if (pAddrDecWin->enable == MV_TRUE)
    {
            mvEthWinEnable(winNum, MV_TRUE);
    }
    
    return MV_OK;
}

/*******************************************************************************
* mvETHWinGet - Get dma peripheral target address window.
*
* DESCRIPTION:
*               Get ETH peripheral target address window.
*
* INPUT:
*       winNum - ETH to target address decode window number.
*
* OUTPUT:
*       pAddrDecWin - ETH target window data structure.
*
* RETURN:
*       MV_ERROR if register parameters are invalid.
*
*******************************************************************************/
MV_STATUS mvEthWinGet(MV_U32 winNum, MV_ETH_DEC_WIN *pAddrDecWin)
{
    MV_DEC_REGS decRegs;
    MV_TARGET_ATTRIB targetAttrib;
    
    /* Parameter checking   */
    if (winNum >= ETH_MAX_DECODE_WIN)
    {
        mvOsPrintf("mvEthWinGet: ERR. Invalid winNum %d\n", winNum);
        return MV_NOT_SUPPORTED;
    }
    
    decRegs.baseReg =  MV_REG_READ(ETH_WIN_BASE_REG(winNum));
    decRegs.sizeReg = MV_REG_READ(ETH_WIN_CONTROL_REG(winNum));
    
    if (MV_OK != mvCtrlRegToAddrDec(&decRegs,&(pAddrDecWin->addrWin)))
    {
        mvOsPrintf("mvAhbToMbusWinGet: mvCtrlRegToAddrDec Failed \n");
        return MV_ERROR;
    }
    
    /* attrib and targetId */
    targetAttrib.attrib = 
     (decRegs.sizeReg & ETH_WIN_ATTR_MASK) >> ETH_WIN_ATTR_OFFSET;
    targetAttrib.targetId = 
     (decRegs.sizeReg & ETH_WIN_TARGET_MASK) >> ETH_WIN_TARGET_OFFSET;
    
    pAddrDecWin->target = mvCtrlTargetGet(&targetAttrib);
    
    /* Check if window is enabled   */
	
    if (MV_REG_READ(ETH_WIN_CONTROL_REG(winNum)) & ETH_WIN_EN_MASK )
    {
        pAddrDecWin->enable = MV_TRUE;
    }
    else
    {
        pAddrDecWin->enable = MV_FALSE;
    }
    
    return MV_OK;
}

/*******************************************************************************
* mvEthWinEnable - Enable/disable a ETH to target address window
*
* DESCRIPTION:
*       This function enable/disable a ETH to target address window.
*       According to parameter 'enable' the routine will enable the
*       window, thus enabling ETH accesses (before enabling the window it is
*       tested for overlapping). Otherwise, the window will be disabled.
*
* INPUT:
*       winNum - ETH to target address decode window number.
*       enable - Enable/disable parameter.
*
* OUTPUT:
*       N/A
*
* RETURN:
*       MV_ERROR if decode window number was wrong or enabled window overlapps.
*
*******************************************************************************/
MV_STATUS mvEthWinEnable(MV_U32 winNum,MV_BOOL enable)
{
    MV_ETH_DEC_WIN addrDecWin;

    /* Parameter checking   */
    if (winNum >= ETH_MAX_DECODE_WIN)
    {
        mvOsPrintf("mvEthTargetWinEnable:ERR. Invalid winNum%d\n",winNum);
        return MV_ERROR;
    }

    if (enable == MV_TRUE)
    {   /* First check for overlap with other enabled windows               */
        /* Get current window */
        if (MV_OK != mvEthWinGet(winNum, &addrDecWin))
        {
            mvOsPrintf("mvEthTargetWinEnable:ERR. targetWinGet fail\n");
            return MV_ERROR;
        }
        /* Check for overlapping */
        if (MV_FALSE == ethWinOverlapDetect(winNum, &(addrDecWin.addrWin)))
        {
            /* No Overlap. Enable address decode target window              */
            MV_REG_BIT_SET(ETH_WIN_CONTROL_REG(winNum), ETH_WIN_EN_MASK);
        }
        else
        {   /* Overlap detected */
            mvOsPrintf("mvEthTargetWinEnable:ERR. Overlap detected\n");
            return MV_ERROR;
        }
    }
    else
    {   /* Disable address decode target window                             */
        MV_REG_BIT_RESET(ETH_WIN_CONTROL_REG(winNum), ETH_WIN_EN_MASK);
    }
    return MV_OK;
}
 
/*******************************************************************************
* mvEthWinTargetGet - Get Window number associated with target
*
* DESCRIPTION:
*
* INPUT:
*
* OUTPUT:
*
* RETURN:
*       window number
*
*******************************************************************************/
MV_U32  mvEthWinTargetGet(MV_TARGET target)
{
    MV_ETH_DEC_WIN decWin;
    MV_U32 winNum;

    /* Check parameters */
    if (target >= MAX_TARGETS)
    {
        mvOsPrintf("mvAhbToMbusWinTargetGet: target %d is Illigal\n", target);
        return 0xffffffff;
    }

    for (winNum=0; winNum<ETH_MAX_DECODE_WIN; winNum++)
    {
        if (mvEthWinGet(winNum,&decWin) != MV_OK)
        {
            mvOsPrintf("mvAhbToMbusWinTargetGet: window returned error\n");
            return 0xffffffff;
        }

        if (decWin.enable == MV_TRUE)
        {
            if (decWin.target == target)
            {
                return winNum;
            }  
        }
    }
    return 0xFFFFFFFF;
}
 
/*******************************************************************************
* ethWinOverlapDetect - Detect ETH address windows overlapping
*
* DESCRIPTION:
*       An unpredicted behaviur is expected in case ETH address decode
*       windows overlapps.
*       This function detects ETH address decode windows overlapping of a
*       specified window. The function does not check the window itself for
*       overlapping. The function also skipps disabled address decode windows.
*
* INPUT:
*       winNum      - address decode window number.
*       pAddrDecWin - An address decode window struct.
*
* OUTPUT:
*       None.
*
* RETURN:
*       MV_TRUE if the given address window overlap current address
*       decode map, MV_FALSE otherwise, MV_ERROR if reading invalid data
*       from registers.
*
*******************************************************************************/
static MV_STATUS ethWinOverlapDetect(MV_U32 winNum, MV_ADDR_WIN *pAddrWin)
{    
    MV_U32              winNumIndex;
    MV_ETH_DEC_WIN      addrDecWin;
                                                                                                                             
    for (winNumIndex=0; winNumIndex<ETH_MAX_DECODE_WIN; winNumIndex++)
    {
        /* Do not check window itself           */
        if (winNumIndex == winNum)
        {
            continue;
        }
    
        /* Do not check disabled windows        */
		if (!(MV_REG_READ(ETH_WIN_CONTROL_REG(winNum) & ETH_WIN_EN_MASK)))		
        {
            continue;
        }
    
        /* Get window parameters        */
        if (MV_OK != mvEthWinGet(winNumIndex, &addrDecWin))
        {
            mvOsPrintf("ethWinOverlapDetect: ERR. TargetWinGet failed\n");
            return MV_ERROR;
        }
/*    
        mvOsPrintf("ethWinOverlapDetect:\n
            winNumIndex =%d baseHigh =0x%x baseLow=0x%x size=0x%x enable=0x%x\n",
            winNumIndex,
            addrDecWin.addrWin.baseHigh,
            addrDecWin.addrWin.baseLow,
            addrDecWin.addrWin.size,
            addrDecWin.enable);
*/
        if (MV_TRUE == ctrlWinOverlapTest(pAddrWin, &(addrDecWin.addrWin)))
        {
            return MV_TRUE;
        }
    }
    return MV_FALSE;
}

/*******************************************************************************
* mvEthAddrDecShow - Print the Etherent address decode map.
*
* DESCRIPTION:
*       This function print the Etherent address decode map.
*
* INPUT:
*       None.
*
* OUTPUT:
*       None.
*
* RETURN:
*       None.
*
*******************************************************************************/
void    mvEthAddrDecShow(void)
{
    MV_ETH_DEC_WIN  win;
    int             i;

    mvOsOutput( "\n" );
    mvOsOutput( "ETH:\n" );
    mvOsOutput( "----\n" );

    for( i = 0; i < ETH_MAX_DECODE_WIN; i++ )
    {
        memset( &win, 0, sizeof(ETH_MAX_DECODE_WIN) );

        mvOsOutput( "win%d - ", i );

        if( mvEthWinGet( i, &win ) == MV_OK )
        {
            if( win.enable )
            {
                mvOsOutput( "%s base %08x, ",
                mvCtrlTargetNameGet(win.target), win.addrWin.baseLow );
                mvOsOutput( "...." );
                mvSizePrint( win.addrWin.size );

                mvOsOutput( "\n" );
            }
            else
                mvOsOutput( "disable\n" );
        }
    }
    return;
}
