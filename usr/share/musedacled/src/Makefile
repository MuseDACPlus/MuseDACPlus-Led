obj-m := musedacled.o
musedacled-objs := musedacled_main.o ledparser.o ledanim.o

# Kernel build directory and local path
KDIR := /lib/modules/$(shell uname -r)/build
PWD  := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

copy:
	mkdir -p ./lib/modules/$(shell uname -r)/extra
	cp musedacled.ko ~/MuseDACPlus-Led/lib/modules/$(shell uname -r)/extra/
	cp musedacled.ko /lib/modules/$(shell uname -r)/extra/
