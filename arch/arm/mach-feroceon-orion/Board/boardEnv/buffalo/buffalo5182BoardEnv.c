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

#include <asm/io.h>
#include <asm/byteorder.h>
#include "buffalo5182BoardEnv.h"

//#define DEBUG_ENV
#ifdef DEBUG_ENV
#define DEBUGK(fmt,args...) printk(fmt ,##args)
#else
#define DEBUGK(fmt,args...)
#endif

#if defined(CONFIG_BUFFALO_PLATFORM)

// start information of Buffalo 5182 Board
MV_BOARD_MPP_INFO BoardMppConfigValue[DB_BUF_BOARD_MAX_MPP_CONFIG_NUM] =
  /* {{MV_U32 mpp0_7, MV_U32 mpp8_15, MV_U32 mpp16_23, MV_U32 mppDev}} */
  {{{BUFFALO_BOARD_DEFAULT_MPP0_7,		/* mpp0_7 */
     BUFFALO_BOARD_DEFAULT_MPP8_15,		/* mpp8_15 */
     BUFFALO_BOARD_DEFAULT_MPP16_23,		/* mpp16_23 */
     BUFFALO_BOARD_DEFAULT_MPP_DEV}}};		/* mppDev */
EXPORT_SYMBOL(BoardMppConfigValue);

MV_DEV_CS_INFO DevCsInfo[DB_BUF_BOARD_MAX_DEVICE_CONFIG_NUM] = {
  /*{deviceCS, params, devType, devWidth}*/
  { 1, 0x8fcfffff, BOARD_DEV_NOR_FLASH, 8},	/* devCs1 */
  { 3, 0x8fcfffff, BOARD_DEV_NOR_FLASH, 8},	/* bootCs */
  {N_A, N_A, N_A, N_A},
  {N_A, N_A, N_A, N_A},
};
EXPORT_SYMBOL(DevCsInfo);

MV_BOARD_PCI_IF BoardPciIf[DB_BUF_BOARD_MAX_PCI_IF_NUM] = {
  {N_A, {N_A, N_A, N_A, N_A}},
  {N_A, {N_A, N_A, N_A, N_A}},
  {N_A, {N_A, N_A, N_A, N_A}},
  {N_A, {N_A, N_A, N_A, N_A}},
};

MV_U32 intsGppMask;

MV_BOARD_TWSI_INFO BoardTwsiDevInfo[DB_BUF_BOARD_MAX_TWSI_DEF_NUM] = {
  /* {{MV_BOARD_DEV_CLASS devClass, MV_U8 twsiDevAddr, MV_U8 twsiDevAddrType}} */
  {BOARD_DEV_RTC, 0x32, ADDR7_BIT},
  {N_A, N_A, N_A},
  {N_A, N_A, N_A},
  {N_A, N_A, N_A},
};
EXPORT_SYMBOL(BoardTwsiDevInfo);

MV_BOARD_MAC_INFO BoardMacInfo[DB_BUF_BOARD_MAX_MAC_INFO_NUM] = {
  /* {{MV_BOARD_MAC_SPEED boardMacSpeed, MV_U8 boardEthSmiAddr}} */
  {BOARD_MAC_SPEED_AUTO, 0x8},
  {N_A, N_A},
  {N_A, N_A},
  {N_A, N_A},
};
EXPORT_SYMBOL(BoardMacInfo);

MV_BOARD_GPP_INFO BoardGppInfo[DB_BUF_BOARD_MAX_GPP_INFO_NUM] = {
  /* {{MV_BOARD_DEV_CLASS, VM_U8 gppPinNum}} */
  {BOARD_DEV_RTC, 3},
  {N_A, N_A},
  {N_A, N_A},
  {N_A, N_A},
};
EXPORT_SYMBOL(BoardGppInfo);

MV_BOARD_INFO buffalo_board_info = {
	"BUFFALO_BOARD",			/* boardName[MAX_BOARD_NAME_LEN] */
	DB_BUF_BOARD_DEFAULT_MPP_CONFIG_NUM,	/* numBoardMppConfig */
	BoardMppConfigValue,			/* pBoardMppConfigValue */
	((1 << 2)),				/* intsGppMask */
	DB_BUF_BOARD_DEFAULT_DEVICE_CONFIG_NUM,	/* numBoardDevIf */
	DevCsInfo,				/* pDevCsInfo */
	DB_BUF_BOARD_DEFAULT_PCI_IF_NUM,	/* numBoardPciIf */
	NULL,					/* pBoardPciIf */
	DB_BUF_BOARD_DEFAULT_TWSI_DEF_NUM,	/* numBoardTwsiDev */
	BoardTwsiDevInfo,			/* pBoardTwsiDev */
	DB_BUF_BOARD_DEFAULT_MAC_INFO_NUM,	/* numBoardMacInfo */
	BoardMacInfo,				/* pBoardMacInfo */
	DB_BUF_BOARD_DEFAULT_GPP_INFO_NUM,	/* numBoardGppInfo */
	BoardGppInfo,				/* pBoardGppInfo */
	DB_BUF_BOARD_DEFAULT_DEBUG_LED_NUM,	/* activeLedsNumber */
	NULL,					/* pLedGppPin */
	0,					/* ledsPolarity */
	BUFFALO_BOARD_DEFAULT_GPP_OE,		/* gppOutEnVal */
	BUFFALO_BOARD_DEFAULT_GPP_VAL,		/* gppOutVal */
	BUFFALO_BOARD_DEFAULT_GPP_POLVAL,	/* gppPolarityVal */
};
EXPORT_SYMBOL(buffalo_board_info);

u32 env_format_version;
EXPORT_SYMBOL(env_format_version);
u32 product_id;
EXPORT_SYMBOL(product_id);
char series_name[BUF_NV_SIZE_SERIES_NAME];
EXPORT_SYMBOL(series_name);
char product_name[BUF_NV_SIZE_PRODUCT_NAME];
EXPORT_SYMBOL(product_name);
s8 bitHddPower[BUF_NV_SIZE_BIT_HDD_POWER];
EXPORT_SYMBOL(bitHddPower);
s8 bitUsbPower[BUF_NV_SIZE_BIT_USB_POWER];
EXPORT_SYMBOL(bitUsbPower);
s8 bitInput[BUF_NV_SIZE_BIT_INPUT];
EXPORT_SYMBOL(bitInput);
s8 bitOutput[BUF_NV_SIZE_BIT_OUTPUT];
EXPORT_SYMBOL(bitOutput);
s8 bitLed[BUF_NV_SIZE_BIT_LED];
EXPORT_SYMBOL(bitLed);
u8 use_slide_power;
EXPORT_SYMBOL(use_slide_power);

int buffaloBoardInfoInit(void)
{
	int i;
	char *p;

	DEBUGK("***** enter %s\n", __FUNCTION__);

	p = ioremap(0xffffffff - CFG_ENV_B_SIZE + 1, CFG_ENV_B_SIZE);
	if (!p)
		return (-1);

	for (i=0; i<CFG_ENV_B_SIZE; i++) {
		if ((i & 0x0000000F) == 0x0)
			DEBUGK("\n%04X: ", i);
		DEBUGK("%02X ",ioread8(p + i) );
	}
	DEBUGK("\n");

	env_format_version = __be32_to_cpu(ioread32(p + BUF_NV_ADDR_VERSION));
	product_id = __be32_to_cpu(ioread32(p + BUF_NV_ADDR_PID));
	// 検査対応
	if (product_id == 0x00000000 || product_id == 0xFFFFFFFF)
		product_id = 0x00000009;

	printk("env_format_version = 0x%08X\n", env_format_version);
	printk("product_id = 0x%08X\n", product_id);
	if (env_format_version == 0 ||
	    env_format_version == 0xFFFFFFFF ||
	    product_id == 0 ||
	    product_id == 0xFFFFFFFF)
		return (-1);

	memset(bitHddPower, -1, BUF_NV_SIZE_BIT_HDD_POWER);
	memset(bitUsbPower, -1, BUF_NV_SIZE_BIT_USB_POWER);
	memset(bitInput, -1, BUF_NV_SIZE_BIT_INPUT);
	memset(bitOutput, -1, BUF_NV_SIZE_BIT_OUTPUT);
	memset(bitLed, -1, BUF_NV_SIZE_BIT_LED);
	
	buffalo_board_info.intsGppMask = __be32_to_cpu(ioread32(p + BUF_NV_ADDR_INTS_GPP_MASK));

	if (ioread8(p + BUF_NV_ADDR_NUM_BOARD_MPP) <= DB_BUF_BOARD_MAX_MPP_CONFIG_NUM) {
		buffalo_board_info.numBoardMppConfigValue = ioread8(p + BUF_NV_ADDR_NUM_BOARD_MPP);
		for (i=0; i<buffalo_board_info.numBoardMppConfigValue; i++) {
			buffalo_board_info.pBoardMppConfigValue->mppGroup[i] =
			  __be32_to_cpu(ioread32(p + BUF_NV_ADDR_BOARD_MPP_INFO + i*4));
		}
	}

	if (ioread8(p + BUF_NV_ADDR_NUM_BOARD_DEVICE_IF) <= DB_BUF_BOARD_MAX_DEVICE_CONFIG_NUM) {
		buffalo_board_info.numBoardDeviceIf = ioread8(p + BUF_NV_ADDR_NUM_BOARD_DEVICE_IF);
		for (i=0; i<buffalo_board_info.numBoardDeviceIf; i++) {
			DevCsInfo[i].deviceCS = ioread8(p + BUF_NV_ADDR_DEV_CS_INFO + i*0x08);
			DevCsInfo[i].devClass = ioread8(p + BUF_NV_ADDR_DEV_CS_INFO + i*0x08 + 0x1);
			DevCsInfo[i].devWidth = ioread8(p + BUF_NV_ADDR_DEV_CS_INFO + i*0x08 + 0x2);
			DevCsInfo[i].params   = __be32_to_cpu(
					       ioread32(p + BUF_NV_ADDR_DEV_CS_INFO + i*0x08 + 0x4));
			DEBUGK("DevCsInfo[%d].deviceCS = %d\n", i, DevCsInfo[i].deviceCS);
			DEBUGK("DevCsInfo[%d].params = 0x%08X\n", i, DevCsInfo[i].params);
			DEBUGK("DevCsInfo[%d].devClass = %d\n", i, DevCsInfo[i].devClass);
			DEBUGK("DevCsInfo[%d].devWidth = %d\n", i, DevCsInfo[i].devWidth);
		}
	}

	if (ioread8(p + BUF_NV_ADDR_NUM_BOARD_PCI_IF) <= DB_BUF_BOARD_MAX_PCI_IF_NUM) {
		buffalo_board_info.numBoardPciIf = ioread8(p + BUF_NV_ADDR_NUM_BOARD_PCI_IF);
		for (i=0; i<buffalo_board_info.numBoardPciIf; i++) {
			BoardPciIf[i].pciDevNum = ioread8(p + BUF_NV_ADDR_BOARD_PCI_INFO + i*0x10);
			BoardPciIf[i].pciGppIntMap[0] = ioread8(p + BUF_NV_ADDR_BOARD_PCI_INFO + i*0x10 + 1);
			BoardPciIf[i].pciGppIntMap[1] = ioread8(p + BUF_NV_ADDR_BOARD_PCI_INFO + i*0x10 + 2);
			BoardPciIf[i].pciGppIntMap[2] = ioread8(p + BUF_NV_ADDR_BOARD_PCI_INFO + i*0x10 + 3);
			BoardPciIf[i].pciGppIntMap[3] = ioread8(p + BUF_NV_ADDR_BOARD_PCI_INFO + i*0x10 + 4);
		}
	}

	if (ioread8(p + BUF_NV_ADDR_NUM_BOARD_TWSI_DEV) <= DB_BUF_BOARD_MAX_TWSI_DEF_NUM) {
		buffalo_board_info.numBoardTwsiDev = ioread8(p + BUF_NV_ADDR_NUM_BOARD_TWSI_DEV);
		for (i=0; i<buffalo_board_info.numBoardTwsiDev; i++) {
			BoardTwsiDevInfo[i].devClass = ioread8(p + BUF_NV_ADDR_BOARD_TWSI_INFO + i*3);
			BoardTwsiDevInfo[i].twsiDevAddr = ioread8(p + BUF_NV_ADDR_BOARD_TWSI_INFO + i*3 + 1);
			BoardTwsiDevInfo[i].twsiDevAddrType = ioread8(p + BUF_NV_ADDR_BOARD_TWSI_INFO + i*3 + 2);
			DEBUGK("BoardTwsiDevInfo[%d].devClass = %d\n", i, BoardTwsiDevInfo[i].devClass);
			DEBUGK("BoardTwsiDevInfo[%d].twsiDevAddr = %d\n", i, BoardTwsiDevInfo[i].twsiDevAddr);
			DEBUGK("BoardTwsiDevInfo[%d].twsiDevAddrType = %d\n", i, BoardTwsiDevInfo[i].twsiDevAddrType);
		}
	}

	if (ioread8(p + BUF_NV_ADDR_NUM_BOARD_MAC_INFO) <= DB_BUF_BOARD_MAX_MAC_INFO_NUM) {
		buffalo_board_info.numBoardMacInfo = ioread8(p + BUF_NV_ADDR_NUM_BOARD_MAC_INFO);
		for (i=0; i<buffalo_board_info.numBoardMacInfo; i++) {
			BoardMacInfo[i].boardMacSpeed = ioread8(p + BUF_NV_ADDR_BOARD_MAC_INFO + i*2);
			BoardMacInfo[i].boardEthSmiAddr = ioread8(p + BUF_NV_ADDR_BOARD_MAC_INFO + i*2 + 1);
		}
	}

	if (ioread8(p + BUF_NV_ADDR_NUM_BOARD_GPP_INFO) <= DB_BUF_BOARD_MAX_GPP_INFO_NUM) {
		buffalo_board_info.numBoardGppInfo = ioread8(p + BUF_NV_ADDR_NUM_BOARD_GPP_INFO);
		for (i=0; i<buffalo_board_info.numBoardGppInfo; i++) {
			BoardGppInfo[i].devClass = ioread8(p + BUF_NV_ADDR_BOARD_GPP_INFO + i*2);
			BoardGppInfo[i].gppPinNum = ioread8(p + BUF_NV_ADDR_BOARD_GPP_INFO + i*2 + 1);
		}
	}

//	buffalo_board_info.activeLedsNumber = ioread8(p + BUF_NV_ADDR_ACTIVE_LEDS_NUMBER);
//	buffalo_board_info.ledsPolarity = ioread8(p + BUF_NV_ADDR_LEDS_POLARITY);
	buffalo_board_info.gppOutEnVal = __be32_to_cpu(ioread32(p + BUF_NV_ADDR_GPP_OUT_EN_VAL));
//	buffalo_board_info.gppOutVal = __be32_to_cpu(ioread32(p + BUF_NV_ADDR_GPP_OUT_VAL));
	buffalo_board_info.gppPolarityVal = __be32_to_cpu(ioread32(p + BUF_NV_ADDR_GPP_POLARITY_VAL));

	volatile unsigned *gpio = (unsigned *)(0xF1000000+0x10100);
	buffalo_board_info.gppOutVal = gpio[0];
//	buffalo_board_info.gppOutEnVal = gpio[1];
//	buffalo_board_info.gppPolarityVal = gpio[3];

	memcpy_fromio(series_name, p + BUF_NV_ADDR_SERIES_NAME, BUF_NV_SIZE_SERIES_NAME);
	series_name[BUF_NV_SIZE_SERIES_NAME - 1] = '\0';
	memcpy_fromio(product_name, p + BUF_NV_ADDR_PRODUCT_NAME, BUF_NV_SIZE_PRODUCT_NAME);
	product_name[BUF_NV_SIZE_PRODUCT_NAME - 1] = '\0';

	memcpy_fromio(bitHddPower, p + BUF_NV_ADDR_BIT_HDD_POWER, BUF_NV_SIZE_BIT_HDD_POWER);
	memcpy_fromio(bitUsbPower, p + BUF_NV_ADDR_BIT_USB_POWER, BUF_NV_SIZE_BIT_USB_POWER);
	memcpy_fromio(bitInput, p + BUF_NV_ADDR_BIT_INPUT, BUF_NV_SIZE_BIT_INPUT);
	memcpy_fromio(bitOutput, p + BUF_NV_ADDR_BIT_OUTPUT, BUF_NV_SIZE_BIT_OUTPUT);
	memcpy_fromio(bitLed, p + BUF_NV_ADDR_BIT_LED, BUF_NV_SIZE_BIT_LED);

	use_slide_power = ioread8(p + BUF_NV_ADDR_USE_SWITCH_SLIDE_POWER);

	DEBUGK("numBoardMppConfigValue: %d\n", buffalo_board_info.numBoardMppConfigValue);
	DEBUGK("intsGppMask: 0x%08X\n", buffalo_board_info.intsGppMask);
	DEBUGK("numBoardDeviceIf: %d\n", buffalo_board_info.numBoardDeviceIf);
	DEBUGK("numBoardPciIf: %d\n", buffalo_board_info.numBoardPciIf);
	DEBUGK("numBoardTwsiDev: %d\n", buffalo_board_info.numBoardTwsiDev);
	DEBUGK("numBoardMacInfo: %d\n", buffalo_board_info.numBoardMacInfo);
	DEBUGK("numBoardGppInfo: %d\n", buffalo_board_info.numBoardGppInfo);
	DEBUGK("activeLedsNumber: %d\n", buffalo_board_info.activeLedsNumber);
	DEBUGK("ledsPolarity: %d\n", buffalo_board_info.ledsPolarity);
	DEBUGK("gppOutEnVal: 0x%08X\n", buffalo_board_info.gppOutEnVal);
	DEBUGK("gppOutVal: 0x%08X\n", buffalo_board_info.gppOutVal);
	DEBUGK("gppPolarityVal: 0x%08X\n", buffalo_board_info.gppPolarityVal);
	DEBUGK("series_name: %s\n", series_name);
	DEBUGK("product_name: %s\n", product_name);

	for (i=0; i<BUF_NV_SIZE_BIT_HDD_POWER; i++) {
		DEBUGK("bitHddPower[%d] = %d\n", i, bitHddPower[i]);
	}
	for (i=0; i<BUF_NV_SIZE_BIT_USB_POWER; i++) {
		DEBUGK("bitUsbPower[%d] = %d\n", i, bitUsbPower[i]);
	}
	DEBUGK("bitInput[BUF_NV_OFFS_BIT_MICON] = %d\n", bitInput[BUF_NV_OFFS_BIT_MICON]);
	DEBUGK("bitInput[BUF_NV_OFFS_BIT_RTC] = %d\n", bitInput[BUF_NV_OFFS_BIT_RTC]);
	DEBUGK("bitInput[BUF_NV_OFFS_BIT_UPS] = %d\n", bitInput[BUF_NV_OFFS_BIT_UPS]);
	DEBUGK("bitInput[BUF_NV_OFFS_BIT_OMR_BL] = %d\n", bitInput[BUF_NV_OFFS_BIT_OMR_BL]);
	DEBUGK("bitInput[BUF_NV_OFFS_BIT_FAN_LOCK] = %d\n", bitInput[BUF_NV_OFFS_BIT_FAN_LOCK]);
	DEBUGK("bitInput[BUF_NV_OFFS_SW_POWER] = %d\n", bitInput[BUF_NV_OFFS_SW_POWER]);
	DEBUGK("bitInput[BUF_NV_OFFS_SW_INIT] = %d\n", bitInput[BUF_NV_OFFS_SW_INIT]);
	DEBUGK("bitInput[BUF_NV_OFFS_SW_RAID] = %d\n", bitInput[BUF_NV_OFFS_SW_RAID]);
	DEBUGK("bitInput[BUF_NV_OFFS_SW_AUTO_POWER] = %d\n", bitInput[BUF_NV_OFFS_SW_AUTO_POWER]);
	DEBUGK("bitInput[BUF_NV_OFFS_SW_FUNC] = %d\n", bitInput[BUF_NV_OFFS_SW_FUNC]);
	DEBUGK("bitOutput[BUF_NV_OFFS_BIT_FAN_LOW] = %d\n", bitOutput[BUF_NV_OFFS_BIT_FAN_LOW]);
	DEBUGK("bitOutput[BUF_NV_OFFS_BIT_FAN_HIGH] = %d\n", bitOutput[BUF_NV_OFFS_BIT_FAN_HIGH]);
	DEBUGK("bitLed[BUF_NV_OFFS_BIT_LED_POWER] = %d\n", bitLed[BUF_NV_OFFS_BIT_LED_POWER]);
	DEBUGK("bitLed[BUF_NV_OFFS_BIT_LED_INFO] = %d\n", bitLed[BUF_NV_OFFS_BIT_LED_INFO]);
	DEBUGK("bitLed[BUF_NV_OFFS_BIT_LED_ALARM] = %d\n", bitLed[BUF_NV_OFFS_BIT_LED_ALARM]);
	DEBUGK("bitLed[BUF_NV_OFFS_BIT_LED_FUNC] = %d\n", bitLed[BUF_NV_OFFS_BIT_LED_FUNC]);
	DEBUGK("bitLed[BUF_NV_OFFS_BIT_LED_ETH] = %d\n", bitLed[BUF_NV_OFFS_BIT_LED_ETH]);

	iounmap(p);
	return 0;
}
EXPORT_SYMBOL(buffaloBoardInfoInit);



// end of Buffalo 5182 Board


MV_BOARD_INFO*	boardInfoTbl[]	=	{&buffalo_board_info};

#endif // CONFIG_BUFFALO_PLATFORM
