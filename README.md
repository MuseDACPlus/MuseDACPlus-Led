MuseDAC+ LED controller

musedacled is a custom Linux kernel module and Raspberry Pi HAT driver for controlling SK9822 (APA102-compatible) addressable RGB LEDs via the SPI5 interface. It provides a simple character-device interface for setting static colors and running animations directly from userspace.

FEATURES
--------
- SPI5-based control of SK9822 LEDs on Raspberry Pi (uses /dev/spidev5.0)
- Character device: /dev/musedacled
- Static color support:
* Named colors: red, green, blue, yellow, cyan, magenta, white, black, etc.
* Hex codes: #RRGGBB or 0xRRGGBB
* Optional brightness 0-31 (append ":<brightness>")
- Animations:
* blink:<ms> - toggle on/off every ms milliseconds
* fade:<ms> - smooth fade in/out over ms milliseconds
* pulse:<ms> - linear ramp up/down over ms milliseconds
- Runtime color change: changing color while animating updates animation base
- Stop command: "stop" halts any running animation
- Modular code structure:
* musedacled_main.c - command parsing, SPI I/O, character device
* ledparser.c - parsing color strings into SPI frames
* ledanim.c - animation state machine and timer
- Debian packaging and udev rules for automatic setup

COMMAND REFERENCE
-----------------
Write one of these to /dev/musedacled:

color <spec>
Set static colors. <spec> is one or more tokens separated by spaces:
- Named: red, green, blue, etc.
- Hex: #RRGGBB or 0xRRGGBB
- Brightness: append :0-31 (e.g. red:10)
Example:
echo -n "color red green:15 #00FF00:5" > /dev/musedacled

anim blink:<ms>
Toggle LEDs on/off every <ms> milliseconds
Example:
echo -n "anim blink:300" > /dev/musedacled

anim fade:<ms>
Smooth fade in/out cycle over <ms> milliseconds
Example:
echo -n "anim fade:1000" > /dev/musedacled

anim pulse:<ms>
Linear pulse up/down cycle over <ms> milliseconds
Example:
echo -n "anim pulse:500" > /dev/musedacled

stop
Stop any running animation (leaves LEDs at last displayed frame)
Example:
echo -n "stop" > /dev/musedacled

BUILD & INSTALLATION
--------------------
Prerequisites:
- Raspberry Pi with SPI5 enabled (GPIO 14/15)
- Kernel headers installed (linux-headers-$(uname -r))

Manual build:
cd src
make clean && make
sudo cp musedacled.ko /lib/modules/$(uname -r)/extra/
sudo depmod
sudo modprobe musedacled

Debian package:
sudo dpkg -i musedacled_1.0_all.deb

TROUBLESHOOTING
---------------
- Permission denied: check udev rule or use sudo
- No LEDs light: verify SPI5 overlay is active and /dev/spidev5.0 exists
- Parsing errors: use echo -n or printf to avoid trailing newline
- Stuck animation: echo -n "stop" > /dev/musedacled; reload module

LICENSE
-------
GPLv2 - see LICENSE file for details
