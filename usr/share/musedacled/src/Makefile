# Kernel module
obj-m              := musedacled.o
musedacled-objs    := musedacled_main.o ledparser.o ledanim.o

# Module build dirs
KDIR               := /lib/modules/$(shell uname -r)/build
PWD                := $(shell pwd)

# Device-tree overlay sources
OVERLAY_SRC        := spi5-musedacled.dts
OVERLAY_DTBO       := spi5-musedacled.dtbo

# Default: just build the kernel module
all: modules

# Build modules
modules:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

# Compile the DT overlay
.PHONY: overlay
overlay: $(OVERLAY_DTBO)

$(OVERLAY_DTBO): $(OVERLAY_SRC)
	dtc -@ -I dts -O dtb -o $@ $<

# Clean both module and overlay
.PHONY: clean
clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -f $(OVERLAY_DTBO)

# Install your module
.PHONY: copy
copy:
	mkdir -p ../../../../lib/modules/$(shell uname -r)/extra
	cp musedacled.ko ../../../../lib/modules/$(shell uname -r)/extra/
	cp musedacled.ko /lib/modules/$(shell uname -r)/extra/
	cp spi5-musedacled.dtbo ..
	sudo cp spi5-musedacled.dtbo /boot/overlays
