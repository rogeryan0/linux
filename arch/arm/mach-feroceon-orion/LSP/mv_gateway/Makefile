#
# Makefile for the Marvell gateway ethernet driver
#

include $(TOPDIR)/arch/arm/mach-feroceon-orion/config/mvRules.mk

obj-$(CONFIG_ARCH_FEROCEON) += mv_gateway.o

MV_GTW_OBJS = mv_gtw_main.o

ifeq ($(CONFIG_MV_GTW_PROC),y)
MV_GTW_OBJS += mv_gtw_proc.o
endif

ifeq ($(CONFIG_MV_GTW_IGMP),y)
MV_GTW_OBJS += mv_gtw_igmp_snoop.o
endif
 
mv_gateway-objs := $(MV_GTW_OBJS)
