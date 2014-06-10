#*******************************************************************************
# Copyright (C) Marvell International Ltd. and its affiliates
#
# This software file (the "File") is owned and distributed by Marvell 
# International Ltd. and/or its affiliates ("Marvell") under the following
# alternative licensing terms.  Once you have made an election to distribute the
# File under one of the following license alternatives, please (i) delete this
# introductory statement regarding license alternatives, (ii) delete the two
# license alternatives that you have not elected to use and (iii) preserve the
# Marvell copyright notice above.
#
#
#*******************************************************************************
# Marvell GPL License Option
#
# If you received this File from Marvell, you may opt to use, redistribute and/or 
# modify this File in accordance with the terms and conditions of the General 
# Public License Version 2, June 1991 (the "GPL License"), a copy of which is 
# available along with the File in the license.txt file or by writing to the Free 
# Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 or 
# on the worldwide web at http://www.gnu.org/licenses/gpl.txt. 
#
# THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE IMPLIED 
# WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE ARE EXPRESSLY 
# DISCLAIMED.  The GPL License provides additional details about this warranty 
# disclaimer.
#*******************************************************************************/
include $(TOPDIR)/arch/arm/mach-feroceon/config/mvRules.mk

# Objects list

COMMON_OBJS        = $(COMMON_DIR)/mvDebug.o $(COMMON_DIR)/mvCommon.o

OSSERVICES_OBJS    = $(OSSERV_LINUX)/mvOsLinux.o $(OSSERV_ARCH_DIR)/mvLinuxArm.o

BOARD_OBJS      = $(BOARD_RTC_DIR)/mvDS1339.o $(BOARD_ENV_DIR)/mvBoardEnvLib.o  \
                  $(BOARD_ENV_DIR)/mvBoardEnvSpec.o $(BOARD_DRAM_DIR)/mvDram.o
                  
QD_OBJS         = $(QD_DIR)/src/driver/gtDrvConfig.o $(QD_DIR)/src/driver/gtDrvEvents.o \
                    $(QD_DIR)/src/driver/gtHwCntl.o $(QD_DIR)/src/platform/gtMiiSmiIf.o\
                    $(QD_DIR)/src/platform/platformDeps.o $(QD_DIR)/src/platform/gtSem.o\
                    $(QD_DIR)/src/platform/gtDebug.o $(QD_DIR)/src/msapi/gtBrgFdb.o \
                    $(QD_DIR)/src/msapi/gtBrgStp.o $(QD_DIR)/src/msapi/gtBrgVlan.o \
                    $(QD_DIR)/src/msapi/gtEvents.o $(QD_DIR)/src/msapi/gtPortCtrl.o \
                    $(QD_DIR)/src/msapi/gtPortStat.o $(QD_DIR)/src/msapi/gtPortStatus.o \
                    $(QD_DIR)/src/msapi/gtQosMap.o $(QD_DIR)/src/msapi/gtPIRL.o  \
                    $(QD_DIR)/src/msapi/gtPhyCtrl.o $(QD_DIR)/src/msapi/gtPhyInt.o \
                    $(QD_DIR)/src/msapi/gtSysConfig.o $(QD_DIR)/src/msapi/gtSysCtrl.o\
                    $(QD_DIR)/src/msapi/gtVersion.o $(QD_DIR)/src/msapi/gtUtils.o \
                    $(QD_DIR)/src/msapi/gtBrgVtu.o $(QD_DIR)/src/msapi/gtPortRmon.o \
                    $(QD_DIR)/src/msapi/gtSysStatus.o $(QD_DIR)/src/msapi/gtPortRateCtrl.o \
                    $(QD_DIR)/src/msapi/gtPortPav.o $(QD_DIR)/src/msapi/gtVct.o 


CONTROLLER_OBJS = $(SOC_CPUIF_DIR)/mvCpuIf.o $(SOC_ENV_DIR)/mvCtrlEnvLib.o      \
                  $(SOC_ENV_DIR)/mvCtrlEnvAddrDec.o $(SOC_CNTMR_DIR)/mvCntmr.o  \
                  $(SOC_DEVICE_DIR)/mvDevice.o $(SOC_TWSI_DIR)/mvTwsi.o         \
                  $(SOC_UART_DIR)/mvUart.o $(SOC_GPP_DIR)/mvGpp.o               \
                  $(SOC_AHB_TO_MBUS_DIR)/mvAhbToMbus.o \
                  $(SOC_DRAM_DIR)/mvDramIf.o $(SOC_PCIIF_DIR)/mvPciIf.o 	\
                  $(SOC_CPU_DIR)/mvCpuArm.o
              
 
#ifdef CONFIG_BUFFALO_LINKSTATION_LSGL
ifneq (${CONFIG_BUFFALO_PLATFORM},)
BOARD_OBJS += $(BOARD_DIR)/buffalo/BuffaloUart.o
BOARD_OBJS += $(BOARD_DIR)/buffalo/BuffaloGpio.o
endif
LSP_OBJS        = $(LSP_DIR)/core.o $(LSP_DIR)/irq.o $(LSP_DIR)/time.o  \
                  $(LSP_DIR)/leds.o $(LSP_DIR)/sysmap.o  $(LSP_DIR)/rtc.o \
		  $(LSP_DIR)/export.o

obj-y   :=  feroceon.o
feroceon-objs  :=$(LSP_OBJS) $(COMMON_OBJS) $(OSSERVICES_OBJS) $(BOARD_OBJS) $(CONTROLLER_OBJS)
feroceon-$(CONFIG_MV_UNIMAC)     += $(BOARD_ETHPHY_DIR)/mvEthPhy.o
feroceon-$(CONFIG_MV_INCLUDE_IDMA) += $(SOC_IDMA_DIR)/mvIdma.o
feroceon-$(CONFIG_MV_INCLUDE_XOR) +=  $(SOC_XOR_DIR)/mvXor.o
feroceon-$(CONFIG_MV_INCLUDE_PCI) += $(SOC_PCI_DIR)/mvPci.o
feroceon-$(CONFIG_MV_INCLUDE_PEX) += $(SOC_PEX_DIR)/mvPex.o
feroceon-$(CONFIG_MV_INCLUDE_USB) += $(SOC_USB_DIR)/mvUsb.o
feroceon-$(CONFIG_MV_CESA) += $(SOC_CESA_DIR)/mvCesa.o $(SOC_CESA_DIR)/mvCesaDebug.o \
                        $(SOC_CESA_DIR)/mvMD5.o $(SOC_CESA_DIR)/mvSHA1.o \
                        $(SOC_CESA_AES_DIR)/mvAesAlg.o $(SOC_CESA_AES_DIR)/mvAesApi.o
feroceon-$(CONFIG_MV_CESA_TEST) += $(SOC_CESA_DIR)/mvCesaTest.o
feroceon-$(CONFIG_MV_INCLUDE_INTEG_SATA) += $(SOC_SATA_DIR)/mvSataAddrDec.o
feroceon-$(CONFIG_MV_INCLUDE_GIG_ETH) += $(BOARD_ETHPHY_DIR)/mvEthPhy.o \
                $(SOC_ETH_DIR)/mvEth.o $(SOC_ETH_DIR)/mvEthAddrDec.o 
feroceon-$(CONFIG_MV_INCLUDE_UNM_ETH) += $(BOARD_ETHPHY_DIR)/mvEthPhy.o \
                $(SOC_ETH_DIR)/mvEth.o $(SOC_ETH_DIR)/mvEthAddrDec.o 
feroceon-$(CONFIG_ETH_MULTI_Q) += $(SOC_ETH_DIR)/../mvEthPolicy.o
feroceon-$(CONFIG_EGIGA_PROC) += $(SOC_ETH_DIR)/mvEthDebug.o
feroceon-$(CONFIG_MV_GTW_PROC) += $(SOC_ETH_DIR)/mvEthDebug.o

# LSP part
feroceon-$(CONFIG_MV_CESA_TOOL)  += $(LSP_CESA_DIR)/cesa_dev.o
feroceon-$(CONFIG_MV_CESA_OCF) += $(LSP_CESA_DIR)/cesa_ocf_drv.o
feroceon-$(CONFIG_MV_USE_XOR_ENGINE) +=  $(LSP_DIR)/xor.o
feroceon-$(CONFIG_MV_USE_IDMA_ENGINE) +=  $(LSP_DIR)/dma.o
feroceon-$(CONFIG_MV_INCLUDE_USB)    += $(LSP_DIR)/usb.o
feroceon-$(CONFIG_PCI)  += $(LSP_DIR)/pci.o
feroceon-$(CONFIG_FEROCEON_PROC) += $(LSP_DIR)/proc.o
feroceon-$(CONFIG_MV_GATEWAY) += $(QD_OBJS)
feroceon-$(CONFIG_MV_UNIMAC) += $(QD_OBJS) $(UNIMAC_OBJS)
feroceon-$(CONFIG_MV_DBG_TRACE) += $(LSP_DIR)/dbg-trace.o
feroceon-$(CONFIG_MV_INCLUDE_INTEG_MFLASH) += $(SOC_MFLASH_DIR)/mvMFlash.o $(SOC_MFLASH_DIR)/mvPMFlash.o \
                         $(SOC_MFLASH_DIR)/mvSMFlash.o
feroceon-$(CONFIG_MV_INCLUDE_SPI) += $(SOC_SPI_DIR)/mvSpi.o $(SOC_SPI_DIR)/mvSpiCmnd.o \
                         $(BOARD_SFLASH_DIR)/mvSFlash.o

