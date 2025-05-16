#!/bin/sh
# Ensure spidev is unbound, then bind spi5.0 to musedacled
if [ -e /sys/bus/spi/devices/spi5.0 ]; then
  echo spi5.0    > /sys/bus/spi/drivers/spidev/unbind  || true
  echo musedacled > /sys/bus/spi/devices/spi5.0/driver_override  || true
  echo spi5.0    > /sys/bus/spi/drivers/musedacled/bind    || true
fi

