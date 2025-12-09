#!/bin/bash
#
# build-package.sh - Build the MuseDAC+ LED Debian package
#
# This script:
#   - Compiles the device tree overlay (.dts -> .dtbo)
#   - Creates the .deb package with proper ownership
#
# Usage: ./build-package.sh
#

set -e

PACKAGE_NAME="musedacled"
VERSION=$(grep -oP 'Version: \K.*' DEBIAN/control || echo "1.0")
ARCH=$(grep -oP 'Architecture: \K.*' DEBIAN/control || echo "all")
DTS_FILE="usr/share/musedacled/src/spi5-musedacled.dts"
DTBO_FILE="usr/share/musedacled/spi5-musedacled.dtbo"

echo "Building ${PACKAGE_NAME} v${VERSION}..."

# 1) Compile the device tree overlay
echo "Compiling device tree overlay..."
if [ ! -f "$DTS_FILE" ]; then
    echo "ERROR: $DTS_FILE not found!"
    exit 1
fi

# Check if dtc (device tree compiler) is available
if ! command -v dtc &> /dev/null; then
    echo "ERROR: dtc (device tree compiler) not found."
    echo "Install it with: sudo apt-get install device-tree-compiler"
    exit 1
fi

# Compile .dts to .dtbo
dtc -@ -I dts -O dtb -o "$DTBO_FILE" "$DTS_FILE"
echo "Created $DTBO_FILE"

# 2) Build the .deb package
echo "Building package..."
DEB_FILE="${PACKAGE_NAME}_${VERSION}_${ARCH}.deb"

# Use --root-owner-group to ensure proper file ownership
sudo dpkg-deb --build --root-owner-group . "$DEB_FILE"

echo ""
echo "SUCCESS! Package created: $DEB_FILE"
echo ""
echo "To install: sudo dpkg -i $DEB_FILE"
echo "To test: sudo dpkg -i $DEB_FILE && sudo systemctl status musedacled"
