# kernel sources
TOPDIR  := $(shell /bin/pwd)

ifndef KSRC
KSRC  := $(TOPDIR)/../..
endif

CC := arm-none-linux-gnueabi-gcc
LD := arm-none-linux-gnueabi-ld

CFLAGS  := $(EXTRA_CFLAGS)
CFLAGS  += -I$(KSRC)/arch/arm/mach-feroceon/LSP/


all: btns_user mv_reg_bit_set

btns_user: btns_user.c
	$(CC) $(CFLAGS) btns_user.c -o btns_user

mv_reg_bit_set: mv_reg_bit_set.c
	$(CC) $(CFLAGS) mv_reg_bit_set.c -o mv_reg_bit_set

clean:
	rm -f  btns_user mv_reg_bit_set *.o
