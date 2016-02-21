#!/bin/sh
# Script executed by Buildroot to finish the filesystem
# before packing it into images (and linking into the kernel).

FS=$1
DATA=$(dirname $0)/data

# Whether the firmware blobs will be copied to the FS.
# Only needed if you are going to use the STBx25xx A/V decoders.
COPY_FIRMWARE=0

### START ###
set -e

# Setup firmware
if [ $COPY_FIRMWARE -ne 0 ]; then
  install $DATA/hotplug.sh $FS/sbin/hotplug
  install -d $FS/lib/firmware

  install -m 644 $DATA/firmware/dvb-stbx25xx-vid.fw $FS/lib/firmware
  install -m 644 $DATA/firmware/dvb-stbx25xx-aud-c.fw $FS/lib/firmware
  install -m 644 $DATA/firmware/dvb-stbx25xx-aud-d.fw $FS/lib/firmware
fi

# Install init script
install $DATA/satip.sh $FS/etc/init.d/S70satip

# Install minisatip data
# FIXME: remove this, replace by package
install $DATA/minisatip $FS/bin
install -d $FS/usr/share/minisatip/html
install -m 644 $DATA/html/sm.png $FS/usr/share/minisatip/html
install -m 644 $DATA/html/sm.jpg $FS/usr/share/minisatip/html
install -m 644 $DATA/html/status.html $FS/usr/share/minisatip/html

# Install other files
install $DATA/misc/udhcpc-hostname.sh $FS/usr/share/udhcpc/default.script.d/set-hostname
