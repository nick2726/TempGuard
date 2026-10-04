# Kbuild Makefile for LTempGuard Character Device Driver
obj-m += temp_driver.o

KDIR ?= $(shell if [ -d /lib/modules/$$(uname -r)/build ]; then echo /lib/modules/$$(uname -r)/build; elif [ -d /usr/src/WSL2-Linux-Kernel-linux-msft-wsl-6.6.87.2 ]; then echo /usr/src/WSL2-Linux-Kernel-linux-msft-wsl-6.6.87.2; else echo /lib/modules/$$(uname -r)/build; fi)
PWD  := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -rf *.o *.ko *.mod *.mod.c *.mod.o .*.cmd Module.symvers modules.order .tmp_versions

.PHONY: all clean
