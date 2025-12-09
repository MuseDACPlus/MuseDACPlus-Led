# MuseDAC+ LED Controller

A Linux kernel module and Raspberry Pi HAT driver for controlling SK9822 (APA102-compatible) addressable RGB LEDs via the SPI5 interface. Provides a simple character device interface (`/dev/musedacled`) for setting colors and running animations from userspace.

## Quick Start

After installation and reboot:

```bash
# Check driver status and see all available commands
cat /dev/musedacled

# Turn all LEDs red
echo -n "color red" > /dev/musedacled

# Set multiple LEDs with different colors and brightness
echo -n "color red:20 green:15 blue:10" > /dev/musedacled

# Start a blinking animation (500ms interval)
echo -n "anim blink:500" > /dev/musedacled

# Smooth fade animation (1 second cycle)
echo -n "anim fade:1000" > /dev/musedacled

# Stop animation
echo -n "stop" > /dev/musedacled

# Turn off all LEDs
echo -n "color black" > /dev/musedacled
```

## Features

- **SPI5-based control** - Uses `/dev/spidev5.0` on Raspberry Pi GPIO 14/15
- **Character device interface** - Simple writes to `/dev/musedacled`
- **Static colors:**
  - Named colors: `red`, `green`, `blue`, `yellow`, `cyan`, `magenta`, `white`, `black`, etc.
  - Hex codes: `#RRGGBB` or `0xRRGGBB`
  - Brightness control: `0-31` (append `:brightness`, e.g., `red:10`)
- **Animations:**
  - `blink:<ms>` - Toggle on/off every ms milliseconds
  - `fade:<ms>` - Smooth fade in/out over ms milliseconds
  - `pulse:<ms>` - Linear ramp up/down over ms milliseconds
- **Runtime updates** - Change colors while animations are running
- **Automatic setup** - systemd service and udev rules included

## Command Reference

All commands are written to `/dev/musedacled`. Use `echo -n` (no newline) or `printf` to avoid parsing errors.

### Get Status and Help

```bash
cat /dev/musedacled
```

Returns the current driver state and lists all available commands with examples.

### Set Static Colors

```bash
echo -n "color <spec>" > /dev/musedacled
```

**Color specifications** (space-separated):
- **Named colors**: `red`, `green`, `blue`, `yellow`, `cyan`, `magenta`, `white`, `black`, `orange`, `purple`, `pink`
- **Hex codes**: `#RRGGBB` or `0xRRGGBB` (e.g., `#FF5500`, `0x00FFAA`)
- **With brightness**: Append `:0-31` (e.g., `red:10`, `#FF0000:20`)

**Examples:**
```bash
# Single color for all LEDs
echo -n "color red" > /dev/musedacled

# Multiple LEDs with different colors
echo -n "color red green blue" > /dev/musedacled

# Mix named colors, hex, and brightness
echo -n "color red:20 green:15 #0000FF:10" > /dev/musedacled
```

### Animations

#### Blink Animation
```bash
echo -n "anim blink:<ms>" > /dev/musedacled
```
Toggles LEDs on/off every `<ms>` milliseconds.

**Example:**
```bash
# Blink every 300ms
echo -n "anim blink:300" > /dev/musedacled
```

#### Fade Animation
```bash
echo -n "anim fade:<ms>" > /dev/musedacled
```
Smooth fade in/out cycle over `<ms>` milliseconds (sine wave).

**Example:**
```bash
# 1-second fade cycle
echo -n "anim fade:1000" > /dev/musedacled
```

#### Pulse Animation
```bash
echo -n "anim pulse:<ms>" > /dev/musedacled
```
Linear brightness ramp up/down over `<ms>` milliseconds.

**Example:**
```bash
# Half-second pulse
echo -n "anim pulse:500" > /dev/musedacled
```

### Stop Animation

```bash
echo -n "stop" > /dev/musedacled
```
Stops any running animation and leaves LEDs at their current state.

### Change Color During Animation

You can update colors while an animation is running:
```bash
# Start fade animation
echo -n "anim fade:1000" > /dev/musedacled

# Change to green (fade continues with new color)
echo -n "color green" > /dev/musedacled
```

## Installation

### Prerequisites
- Raspberry Pi 4 Model B (other models may work but are untested)
- Raspberry Pi OS (Debian-based)
- SK9822 or APA102-compatible LEDs connected to SPI5 (GPIO 14/15)
- **Kernel headers** (required for module compilation):
  ```bash
  sudo apt-get update
  sudo apt-get install raspberrypi-kernel-headers
  ```

### From Pre-built Package

**⚠️ Important:** Install kernel headers BEFORE installing the package, as the kernel module is compiled during installation.

```bash
# 1. Install kernel headers
sudo apt-get update
sudo apt-get install raspberrypi-kernel-headers

# 2. Download and install the package
sudo dpkg -i musedacled_1.0_all.deb

# 3. Reboot
sudo reboot
```

**After reboot, verify:**
```bash
# Check device exists
ls -l /dev/musedacled
# Should show: crw-rw-rw- 1 root root ...

# Check module is loaded
lsmod | grep musedacled

# Test it
echo -n "color red" > /dev/musedacled
```

**If `/dev/musedacled` doesn't exist**, see the [Troubleshooting](#devmusedacled-doesnt-exist-after-installation-and-reboot) section.

### Building from Source

**Prerequisites:**
```bash
sudo apt-get update
## Hardware Setup

Connect your SK9822/APA102 LED strip to the Raspberry Pi:
- **Clock (CI/CK)** → GPIO 14 (SPI5 SCLK)
- **Data (DI/DA)** → GPIO 15 (SPI5 MOSI)  
- **Ground** → GND
- **+5V** → External 5V power supply (LEDs can draw significant current)

⚠️ **Important:** Always power LEDs from an external supply, not the Pi's 5V pin.

## Troubleshooting

### Quick Diagnostics

Run this command to check the status:
```bash
echo "=== Module Status ===" && \
lsmod | grep musedacled && \
echo "=== Device File ===" && \
ls -l /dev/musedacled 2>/dev/null || echo "NOT FOUND" && \
echo "=== SPI5 Device ===" && \
ls -l /dev/spidev5.0 2>/dev/null || echo "NOT FOUND" && \
echo "=== Driver Binding ===" && \
cat /sys/bus/spi/devices/spi5.0/driver/module/name 2>/dev/null || echo "NOT BOUND"
```

### /dev/musedacled doesn't exist after installation and reboot

This is the most common issue. Check each step:

**2. Verify kernel module is loaded:**
```bash
lsmod | grep musedacled
```

If not loaded, try loading manually:
If not loaded, check config.txt:
```bash
grep -E "spi5" /boot/firmware/config.txt
# Should show both:
# dtoverlay=spi5-1cs
# dtoverlay=spi5-musedacled
```

If missing, add them manually:
```bash
echo "dtoverlay=spi5-1cs" | sudo tee -a /boot/firmware/config.txt
echo "dtoverlay=spi5-musedacled" | sudo tee -a /boot/firmware/config.txt
sudo reboot
```

**2. Verify kernel module is loaded:**
```bash
lsmod | grep musedacled
```

If not loaded, try loading manually:
```bash
sudo modprobe musedacled
```

If that fails with "module not found", the module wasn't built during installation:
```bash
# Build it manually
cd /usr/share/musedacled/src
make clean && make
sudo cp musedacled.ko /lib/modules/$(uname -r)/extra/
sudo depmod -a
sudo modprobe musedacled
```

**3. Check if kernel headers were installed:**
```bash
ls /lib/modules/$(uname -r)/build
```

If not found, install them:
```bash
sudo apt-get install raspberrypi-kernel-headers
```

Then rebuild and reinstall the package.

**4. Verify SPI5 device exists:**
```bash
ls -l /dev/spidev5.0
dmesg | grep spi5
```

If `/dev/spidev5.0` doesn't exist, the device tree overlay didn't load:
```bash
# Check config.txt
grep spi5-musedacled /boot/firmware/config.txt

# Verify overlay file exists
ls -l /boot/firmware/overlays/spi5-musedacled.dtbo
```

**5. Check driver binding:**
```bash
# See what driver spi5.0 is using
cat /sys/bus/spi/devices/spi5.0/driver/module/name 2>/dev/null || echo "No driver bound"

# Manually bind to musedacled
echo spi5.0 > /sys/bus/spi/drivers/spidev/unbind 2>/dev/null || true
echo musedacled > /sys/bus/spi/devices/spi5.0/driver_override
echo spi5.0 > /sys/bus/spi/drivers/musedacled/bind
```

**6. Check systemd service:**
```bash
sudo systemctl status musedacled-bind.service
sudo systemctl enable musedacled-bind.service
sudo systemctl start musedacled-bind.service
```

### Permission denied when writing to /dev/musedacled
The udev rule should make the device world-writable. Check:
```bash
ls -l /dev/musedacled
# Should show: crw-rw-rw-
```

If not, reload udev rules:
```bash
sudo udevadm control --reload-rules
sudo udevadm trigger
```

### LEDs don't light up
1. Verify SPI5 is active:
   ```bash
   ls -l /dev/spidev5.0
   # Should exist
   
   dmesg | grep -i spi5
   # Should show SPI5 initialization
   ```

2. Check the device tree overlay:
   ```bash
   grep spi5-musedacled /boot/firmware/config.txt
   # Should show: dtoverlay=spi5-musedacled
   ```

3. Verify module is loaded:
   ```bash
   lsmod | grep musedacled
   dmesg | grep musedacled
   ```

### Parsing errors
Always use `echo -n` (no trailing newline) or `printf`:
```bash
# Good
echo -n "color red" > /dev/musedacled
printf "color red" > /dev/musedacled

# Bad (trailing newline may cause parsing issues)
echo "color red" > /dev/musedacled
```

### Animation stuck or unresponsive
Stop the animation and reload the module:
```bash
echo -n "stop" > /dev/musedacled
sudo rmmod musedacled
sudo modprobe musedacled
```

### Module won't load after kernel update
Rebuild the module for the new kernel:
```bash
cd /usr/share/musedacled/src
make clean && make
sudo cp musedacled.ko /lib/modules/$(uname -r)/extra/
sudo depmod -a
sudo modprobe musedacled
```

## Project Structure

- `DEBIAN/` - Package control files and scripts
- `etc/` - System configuration (udev rules, systemd services)
- `usr/bin/` - Installation and binding scripts
- `usr/share/musedacled/src/` - Kernel module source code
  - `musedacled_main.c` - Character device, SPI I/O, command parsing
  - `ledparser.c` - Color string parsing
  - `ledanim.c` - Animation engine
  - `spi5-musedacled.dts` - Device tree overlay source

## License

GPLv2 - see [LICENSE](LICENSE) file for details

## Contributing

Issues and pull requests welcome at [github.com/MuseDACPlus/MuseDACPlus-Led](https://github.com/MuseDACPlus/MuseDACPlus-Led)

This will:
- Compile the device tree overlay (`spi5-musedacled.dtbo`)
- Build `musedacled_1.0_all.deb` with proper ownership

**Install:**
```bash
sudo dpkg -i musedacled_1.0_all.deb
sudo reboot
```

### Manual Build (Without Package)

If you want to build and load the module manually:

```bash
cd usr/share/musedacled/src

# Compile device tree overlay
dtc -@ -I dts -O dtb -o spi5-musedacled.dtbo spi5-musedacled.dts
sudo cp spi5-musedacled.dtbo /boot/firmware/overlays/

# Add overlay to config
echo "dtoverlay=spi5-musedacled" | sudo tee -a /boot/firmware/config.txt

# Build kernel module
make clean && make
sudo cp musedacled.ko /lib/modules/$(uname -r)/extra/
sudo depmod -a
sudo modprobe musedacled
```

TROUBLESHOOTING
---------------
- Permission denied: check udev rule or use sudo
- No LEDs light: verify SPI5 overlay is active and /dev/spidev5.0 exists
- Parsing errors: use echo -n or printf to avoid trailing newline
- Stuck animation: echo -n "stop" > /dev/musedacled; reload module

LICENSE
-------
GPLv2 - see LICENSE file for details
