#
# Makefile for the Marvell Gigabit/UniMAC Ethernet driver
#

include $(TOPDIR)/arch/arm/mach-feroceon-orion/config/mvRules.mk

obj-$(CONFIG_ARCH_FEROCEON) += mv_ethernet.o

MV_ETHERNET_OBJS = mv_ethernet_main.o

ifeq ($(CONFIG_EGIGA_PROC),y)
MV_ETHERNET_OBJS += mv_e_proc.o
endif
 
mv_ethernet-objs := $(MV_ETHERNET_OBJS)

