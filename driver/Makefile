# Kbuild Makefile for LTempGuard Character Device Driver
obj-m += temp_driver.o

KDIR ?= /lib/modules/$(shell uname -r)/build
PWD  := $(shell pwd)

# Target rules
all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -rf *.o *.ko *.mod *.mod.c *.mod.o .*.cmd Module.symvers modules.order .tmp_versions

.PHONY: all clean
