#
# Makefile for the Marvell Gigabit Ethernet driver
#
ifeq ($(CONFIG_ARCH_FEROCEON_ORION),y)
obj-$(CONFIG_MV_ETHERNET) += ../../../../mach-feroceon-orion/LSP/mv_ethernet/
obj-$(CONFIG_MV_GATEWAY) += ../../../../mach-feroceon-orion/LSP/mv_gateway/
endif
 
ifeq ($(CONFIG_ARCH_FEROCEON_KW),y)
	include $(TOPDIR)/arch/arm/mach-feroceon-kw/config/mvRules.mk
endif

ifeq ($(CONFIG_ARCH_FEROCEON_MV78XX0),y)
	include $(TOPDIR)/arch/arm/mach-feroceon-mv78xx0/config/mvRules.mk
endif

ifneq ($(CONFIG_ARCH_FEROCEON_ORION),y)
obj-$(CONFIG_MV_ETHERNET) += mv_netdev.o mv_ethernet.o
obj-$(CONFIG_MV_ETH_PROC) += mv_eth_proc.o
obj-$(CONFIG_MV_GATEWAY) +=  mv_gateway.o
obj-$(CONFIG_MV_GTW_IGMP) += mv_gtw_igmp.o
endif
