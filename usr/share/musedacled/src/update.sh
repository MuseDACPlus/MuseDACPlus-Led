#!/bin/bash
set -e

echo "[update] Cleaning and rebuilding module..."
make clean && make && make copy

echo "[update] Reloading musedacled kernel module..."
if lsmod | grep -q "^musedacled"; then
    sudo rmmod musedacled
fi

sudo modprobe musedacled

echo "[update] Done."
