# This flags will be used only by the Marvell arch files compilation.

# General definitions
CPU_ARCH    = ARM
CHIP        = 88FXX81
#CHIP_DEFINE = 88F5181
VENDOR      = Marvell
ifeq ($(CONFIG_CPU_BIG_ENDIAN),y)
ENDIAN      = BE
else
ENDIAN      = LE
endif

# Main directory structure
SRC_PATH           = .
BOARD_DIR          = $(SRC_PATH)/Board
COMMON_DIR         = $(SRC_PATH)/Common
SOC_DIR            = $(SRC_PATH)/Soc
OSSERVICES_DIR     = $(SRC_PATH)/osServices
LSP_DIR            = $(SRC_PATH)/LSP
LSP_CESA_DIR	   = $(LSP_DIR)/cesa

PLAT_DIR           = $(SRC_PATH)/config

# Board components
BOARD_ENV_DIR      = $(BOARD_DIR)/boardEnv
#BOARD_ENV_PLAT_DIR = $(BOARD_DIR)/boardEnv/DB_$(CHIP)
BOARD_DRAM_DIR     = $(BOARD_DIR)/dram
BOARD_ETHPHY_DIR   = $(BOARD_DIR)/ethPhy
BOARD_FLASH_DIR    = $(BOARD_DIR)/flash
BOARD_PCI_DIR      = $(BOARD_DIR)/pci
BOARD_RTC_DIR      = $(BOARD_DIR)/rtc
BOARD_TDM_DIR      = $(BOARD_DIR)/tdm-fpga
BOARD_SLIC_DIR     = $(BOARD_DIR)/slic
BOARD_DAA_DIR      = $(BOARD_DIR)/daa
SATA_CORE_DIR      = $(BOARD_DIR)/SATA/CoreDriver/
QD_DIR             = $(BOARD_DIR)/QD-DSDT_2.5b
BOARD_SFLASH_DIR   = $(BOARD_DIR)/sflash



# Controller components
SOC_CPU_DIR      = $(SOC_DIR)/cpu
#SOC_CPU_PLAT_DIR = $(SOC_DIR)/cpu/MV_$(CHIP)
SOC_AHB_TO_MBUS_DIR = $(SOC_DIR)/ahbtombus
SOC_CNTMR_DIR     = $(SOC_DIR)/cntmr
SOC_CPUIF_DIR     = $(SOC_DIR)/cpuIf
SOC_ENV_DIR       = $(SOC_DIR)/ctrlEnv
#SOC_ENV_CHIP_DIR  = $(SOC_ENV_DIR)/MV_$(CHIP)
SOC_DEVICE_DIR    = $(SOC_DIR)/device
SOC_DRAM_DIR      = $(SOC_DIR)/dram
SOC_DRAM_ARCH_DIR = $(SOC_DRAM_DIR)/Arch$(CPU_ARCH)
SOC_GPP_DIR       = $(SOC_DIR)/gpp
ifeq ($(CONFIG_MV_INCLUDE_IDMA),y)
SOC_IDMA_DIR      = $(SOC_DIR)/idma
endif
ifeq ($(CONFIG_MV_INCLUDE_PCI),y)
SOC_PCI_DIR       = $(SOC_DIR)/pci
endif
ifeq ($(CONFIG_MV_INCLUDE_PEX),y)
SOC_PEX_DIR       = $(SOC_DIR)/pex
endif
SOC_TWSI_DIR      = $(SOC_DIR)/twsi
SOC_TWSI_ARCH_DIR = $(SOC_TWSI_DIR)/Arch$(CPU_ARCH)
ifeq ($(CONFIG_MV_INCLUDE_TDM),y)
SOC_TDM_DIR       = $(SOC_DIR)/tdm
endif
ifeq ($(CONFIG_MV_INCLUDE_UNM_ETH),y)
SOC_ETH_DIR       = $(SOC_DIR)/eth/unimac
else
SOC_ETH_DIR       = $(SOC_DIR)/eth/gbe
endif

SOC_UART_DIR      = $(SOC_DIR)/uart
ifeq ($(CONFIG_MV_INCLUDE_USB),y)
SOC_USB_DIR       = $(SOC_DIR)/usb
endif
SOC_PCIIF_DIR     = $(SOC_DIR)/pciIf
ifeq ($(CONFIG_MV_INCLUDE_CESA),y)
SOC_CESA_DIR	  = $(SOC_DIR)/cesa
SOC_CESA_AES_DIR  = $(SOC_DIR)/cesa/AES
endif
ifeq ($(CONFIG_MV_INCLUDE_XOR),y)
SOC_XOR_DIR       = $(SOC_DIR)/xor
endif
ifeq ($(CONFIG_MV_INCLUDE_INTEG_SATA),y)
SOC_SATA_DIR       = $(SOC_DIR)/sata
endif
ifeq ($(CONFIG_MV_INCLUDE_INTEG_MFLASH),y)
SOC_MFLASH_DIR       = $(SOC_DIR)/mflash
endif
ifeq ($(CONFIG_MV_INCLUDE_SPI),y)
SOC_SPI_DIR       = $(SOC_DIR)/spi
endif


# OS services
OSSERV_LINUX       = $(OSSERVICES_DIR)/linux
OSSERV_ARCH_DIR    = $(OSSERVICES_DIR)/linux/Arch$(CPU_ARCH)

# Internal definitions
MV_DEFINE = -DMV_LINUX -DMV_CPU_$(ENDIAN) -DMV_$(CPU_ARCH) 

# Internal include path
SRC_PATH_I      = $(TOPDIR)/arch/arm/mach-feroceon

BOARD_PATH      = -I$(SRC_PATH_I)/$(BOARD_DIR) -I$(SRC_PATH_I)/$(BOARD_ENV_DIR) \
                  -I$(SRC_PATH_I)/$(BOARD_DRAM_DIR) -I$(SRC_PATH_I)/$(BOARD_ETHPHY_DIR) -I$(SRC_PATH_I)/$(BOARD_FLASH_DIR) \
                  -I$(SRC_PATH_I)/$(BOARD_PCI_DIR) -I$(SRC_PATH_I)/$(BOARD_RTC_DIR)	\
        		  -I$(SRC_PATH_I)/$(SATA_CORE_DIR) -I$(SRC_PATH_I)/$(BOARD_SLIC_DIR) -I$(SRC_PATH_I)/$(BOARD_TDM_DIR) \
        		  -I$(SRC_PATH_I)/$(BOARD_SFLASH_DIR) -I$(SRC_PATH_I)/$(BOARD_DAA_DIR)
        		  				  
QD_PATH         = -I$(SRC_PATH_I)/$(QD_DIR)/Include  -I$(SRC_PATH_I)/$(QD_DIR)/Include/h/msApi \
                    -I$(SRC_PATH_I)/$(QD_DIR)/Include/h/driver -I$(SRC_PATH_I)/$(QD_DIR)/Include/h/platform
                     
COMMON_PATH   	= -I$(SRC_PATH_I)/$(COMMON_DIR)
 
SOC_PATH        = -I$(SRC_PATH_I)/$(SOC_DIR) -I$(SRC_PATH_I)/$(SOC_UART_DIR) -I$(SRC_PATH_I)/$(SOC_CNTMR_DIR)          \
                  -I$(SRC_PATH_I)/$(SOC_CPUIF_DIR) -I$(SRC_PATH_I)/$(SOC_ENV_DIR) -I$(SRC_PATH_I)/$(SOC_DEVICE_DIR)    \
                  -I$(SRC_PATH_I)/$(SOC_DRAM_DIR) -I$(SRC_PATH_I)/$(SOC_DRAM_ARCH_DIR) -I$(SRC_PATH_I)/$(SOC_GPP_DIR)  \
                  -I$(SRC_PATH_I)/$(SOC_TWSI_DIR) -I$(SRC_PATH_I)/$(SOC_AHB_TO_MBUS_DIR)\
                  -I$(SRC_PATH_I)/$(SOC_CPU_DIR)	-I$(SRC_PATH_I)/$(SOC_SATA_DIR)	\
                  -I$(SRC_PATH_I)/$(SOC_TWSI_ARCH_DIR)  -I$(SRC_PATH_I)/$(SOC_PCIIF_DIR)
                   
ifeq ($(CONFIG_MV_INCLUDE_IDMA),y)
SOC_PATH	+= -I$(SRC_PATH_I)/$(SOC_IDMA_DIR)
endif
ifeq ($(CONFIG_MV_INCLUDE_PCI),y)
SOC_PATH	+= -I$(SRC_PATH_I)/$(SOC_PCI_DIR)
endif
ifeq ($(CONFIG_MV_INCLUDE_PEX),y)
SOC_PATH	+= -I$(SRC_PATH_I)/$(SOC_PEX_DIR)
endif
ifeq ($(CONFIG_MV_INCLUDE_TDM),y)
SOC_PATH	+= -I$(SRC_PATH_I)/$(SOC_TDM_DIR)
endif
ifeq ($(CONFIG_MV_INCLUDE_GIG_ETH),y)
SOC_PATH	+= -I$(SRC_PATH_I)/$(SOC_DIR)/eth/ -I$(SRC_PATH_I)/$(SOC_ETH_DIR) 
endif
ifeq ($(CONFIG_MV_INCLUDE_UNM_ETH),y)
SOC_PATH	+= -I$(SRC_PATH_I)/$(SOC_DIR)/eth/ -I$(SRC_PATH_I)/$(SOC_ETH_DIR) 
endif
ifeq ($(CONFIG_MV_INCLUDE_USB),y)
SOC_PATH	+= -I$(SRC_PATH_I)/$(SOC_USB_DIR)
endif
ifeq ($(CONFIG_MV_INCLUDE_CESA),y)
SOC_PATH	+= -I$(SRC_PATH_I)/$(SOC_CESA_DIR)
SOC_PATH	+= -I$(SRC_PATH_I)/$(SOC_CESA_AES_DIR)
endif
ifeq ($(CONFIG_MV_INCLUDE_XOR),y)
SOC_PATH	+= -I$(SRC_PATH_I)/$(SOC_XOR_DIR)
endif
ifeq ($(CONFIG_MV_INCLUDE_INTEG_MFLASH),y)
SOC_PATH	+= -I$(SRC_PATH_I)/$(SOC_MFLASH_DIR)
endif
ifeq ($(CONFIG_MV_INCLUDE_SPI),y)
SOC_PATH	+= -I$(SRC_PATH_I)/$(SOC_SPI_DIR)
endif

OSSERVICES_PATH = -I$(SRC_PATH_I)/$(OSSERVICES_DIR) -I$(SRC_PATH_I)/$(OSSERV_LINUX) -I$(SRC_PATH_I)/$(OSSERV_ARCH_DIR) 
LSP_PATH        = -I$(SRC_PATH_I)/$(LSP_DIR) -I$(SRC_PATH_I)/$(LSP_CESA_DIR)
PLAT_PATH       = -I$(SRC_PATH_I)/$(PLAT_DIR)


EXTRA_INCLUDE  	= $(MV_DEFINE) $(OSSERVICES_PATH) $(BOARD_PATH) $(COMMON_PATH) $(SOC_PATH)  \
                 $(LSP_PATH) $(PLAT_PATH)

ifeq ($(CONFIG_MV_GATEWAY),y)
EXTRA_INCLUDE   += $(QD_PATH)
EXTRA_CFLAGS    += -DLINUX  
endif

ifeq ($(CONFIG_MV_UNIMAC),y)
EXTRA_INCLUDE   += $(QD_PATH)
EXTRA_CFLAGS    += -DLINUX  
endif

ifeq ($(CONFIG_MV_CESA_TEST),y)
EXTRA_CFLAGS += -DCONFIG_MV_CESA_TEST
endif

ifeq ($(CONFIG_MV_CESA_OCF),y)
EXTRA_CFLAGS    += -I$(TOPDIR)/crypto/ocf
endif

EXTRA_CFLAGS 	+= $(EXTRA_INCLUDE) $(MV_DEFINE)

ifeq ($(CONFIG_SATA_DEBUG_ON_ERROR),y)
EXTRA_CFLAGS    += -DMV_LOG_ERROR
endif

ifeq ($(CONFIG_SATA_FULL_DEBUG),y)
EXTRA_CFLAGS    += -DMV_LOG_DEBUG
endif

ifeq ($(CONFIG_MV_SATA_SUPPORT_ATAPI),y)
EXTRA_CFLAGS    += -DMV_SUPPORT_ATAPI
endif

ifeq ($(CONFIG_MV_SATA_ENABLE_1MB_IOS),y)
EXTRA_CFLAGS    += -DMV_SUPPORT_1MBYTE_IOS
endif

ifeq ($(CONFIG_MV88F5182),y)
EXTRA_CFLAGS    += -DMV_88F5182 -DUSB_UNDERRUN_WA
endif

ifeq ($(CONFIG_MV88F5082),y)
EXTRA_CFLAGS    += -DMV_88F5082 -DUSB_UNDERRUN_WA
endif

ifeq ($(CONFIG_MV88F5181),y)
EXTRA_CFLAGS    += -DMV_88F5181
endif

ifeq ($(CONFIG_MV88F5180N),y)
EXTRA_CFLAGS    += -DMV_88F5180N
endif

ifeq ($(CONFIG_MV88F1181),y)
EXTRA_CFLAGS    += -DMV_88F1181
endif

ifeq ($(CONFIG_MV88F1281),y)
EXTRA_CFLAGS    += -DMV_88F1281
endif


ifeq ($(CONFIG_MV88F6082),y)
EXTRA_CFLAGS    += -DMV_88F6082
endif


ifeq ($(CONFIG_MV88F5181L),y)
EXTRA_CFLAGS    += -DMV_88F5181L -DUSB_UNDERRUN_WA
endif

ifeq ($(CONFIG_MV88W8660),y)
EXTRA_CFLAGS    += -DMV_88W8660
endif


