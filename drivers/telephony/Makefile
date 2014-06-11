#
# Makefile for drivers/telephony
#

obj-$(CONFIG_PHONE) += phonedev.o
obj-$(CONFIG_PHONE_IXJ) += ixj.o
obj-$(CONFIG_PHONE_IXJ_PCMCIA) += ixj_pcmcia.o
ifeq ($(CONFIG_ARCH_ARMADA370), y)
obj-$(CONFIG_MV_PHONE) += ../../arch/arm/plat-armada370/mv_drivers_lsp/mv_phone/
endif
ifeq ($(CONFIG_ARCH_ARMADA_XP), y)
obj-$(CONFIG_MV_PHONE) += ../../arch/arm/plat-armadaxp/mv_drivers_lsp/mv_phone/
endif
