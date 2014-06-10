#
# linux/drivers/chips/Makefile
#

obj-$(CONFIG_MV_INCLUDE_SFLASH_MTD) += sflash.o
obj-$(CONFIG_MV_INCLUDE_MFLASH_MTD) += mflash.o
obj-$(CONFIG_MTD)		+= chipreg.o
obj-$(CONFIG_MTD_CFI)		+= cfi_probe.o
obj-$(CONFIG_MTD_CFI_UTIL)	+= cfi_util.o
obj-$(CONFIG_MTD_CFI_STAA)	+= cfi_cmdset_0020.o
obj-$(CONFIG_MTD_CFI_AMDSTD)	+= cfi_cmdset_0002.o
obj-$(CONFIG_MTD_CFI_INTELEXT)	+= cfi_cmdset_0001.o
obj-$(CONFIG_MTD_GEN_PROBE)	+= gen_probe.o
obj-$(CONFIG_MTD_JEDECPROBE)	+= jedec_probe.o
obj-$(CONFIG_MTD_RAM)		+= map_ram.o
obj-$(CONFIG_MTD_ROM)		+= map_rom.o
obj-$(CONFIG_MTD_ABSENT)	+= map_absent.o

include $(TOPDIR)/arch/arm/mach-feroceon/config/mvRules.mk
sflash-objs = ../../../arch/arm/mach-feroceon/LSP/mtd/sflash.o
mflash-objs = ../../../arch/arm/mach-feroceon/LSP/mtd/mflash.o
