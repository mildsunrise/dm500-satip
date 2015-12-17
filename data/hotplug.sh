#!/bin/sh
# Simple hotplug script to load firmware.

if [ ! -z "$FIRMWARE" ]; then
  BLOB="/lib/firmware/$FIRMWARE"
  if [ -f "$BLOB" ]; then
    echo 1 > /sys/$DEVPATH/loading
    cat $BLOB > /sys/$DEVPATH/data
    echo 0 > /sys/$DEVPATH/loading
  else
    echo -1 > /sys/$DEVPATH/loading
  fi
fi
