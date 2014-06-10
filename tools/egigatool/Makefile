# kernel sources 
TOPDIR  := $(shell /bin/pwd)

ifndef KSRC
KSRC  := $(TOPDIR)/../..
endif

include $(KSRC)/.config

#CC := codesourcery/arm-none-linux-gnueabi/bin/arm-none-linux-gnueabi-gcc
#LD := /codesourcery/arm-none-linux-gnueabi/bin/arm-none-linux-gnueabi-ld
#CC := /opt/crosstool/powerpc-7450-linux-gnu/gcc-3.4.3-glibc-2.3.3/bin/powerpc-7450-linux-gnu-gcc
#LD := /opt/crosstool/powerpc-7450-linux-gnu/gcc-3.4.3-glibc-2.3.3/bin/powerpc-7450-linux-gnu-ld

CFLAGS  := $(EXTRA_CFLAGS)
CFLAGS  += -I$(KSRC)/arch/arm/mach-feroceon/LSP/mv_ethernet/
		 

SRCS := egigatool.c
OBJS := egigatool.o

ifeq ($(CONFIG_MV_ETH_HEADER),y)
SRCS += shim_egigatool.c
OBJS += shim_egigatool.o
CFLAGS += -DCONFIG_MV_ETH_HEADER
endif

TARGET := mvEgigaTool

default :
	$(CC) $(CFLAGS) $(SRCS) -o $(TARGET)

clean:
	rm -f $(TARGET) $(OBJS)


all : clean default

