
# kernel sources
ifndef KSRC
KSRC  := .
endif

TOPDIR := $(KSRC)

ifeq ($(CONFIG_ARM),y)
MV_DIR = $(TOPDIR)/arch/arm/mach-feroceon
USB_API_DIR := $(MV_DIR)/Soc/usb
USB_PRV_DIR := $(MV_DIR)/Soc/usb_restricted
include $(MV_DIR)/config/mvRules.mk
endif 

ifeq ($(CONFIG_PPC),y)
MV_DIR = $(TOPDIR)/arch/ppc/platforms/Marvell
USB_API_DIR := $(MV_DIR)/Controller/usb
USB_PRV_DIR := $(MV_DIR)/Controller/usb_restricted
include $(MV_DIR)/db645xx/mvRules.make
endif 


USB_DEV_DIR := $(USB_PRV_DIR)/device

CFLAGS  += $(EXTRA_CFLAGS) -I$(USB_API_DIR)/api -I$(USB_PRV_DIR)/common -I$(USB_DEV_DIR)

OBJS        := mv_gadget.o ../$(USB_DEV_DIR)/mvUsbDevCh9.o ../$(USB_DEV_DIR)/mvUsbDevMain.o 	\
               ../$(USB_DEV_DIR)/mvUsbDevRecv.o ../$(USB_DEV_DIR)/mvUsbDevSend.o ../$(USB_DEV_DIR)/mvUsbDevUtl.o 	\
	       ../$(USB_DEV_DIR)/mvUsbHsDevUtl.o  ../$(USB_DEV_DIR)/mvUsbHsDevMain.o ../$(USB_DEV_DIR)/mvUsbHsDevCncl.o

mv_usb_gadget-objs := $(OBJS)
obj-m := mv_usb_gadget.o

