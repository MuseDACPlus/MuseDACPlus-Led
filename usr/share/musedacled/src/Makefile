obj-m += musedacled.o ledparser.o

KDIR := /lib/modules/$(shell uname -r)/build
PWD  := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

install:
	$(MAKE) -C $(KDIR) M=$(PWD) modules_install
	depmod

load:
	sudo modprobe musedacled

unload:
	sudo rmmod musedacled

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
