#!/bin/bash
# Script called by Buildroot after all images have been generated.
# This builds the final, flashable image, by wrapping zImage into
# a cramfs, for the bootloader to read.

set -e

ZIMAGE=$1/zImage
TMPDIR=$1/cramfs.tmp
CRAMFS=$1/cramfs

# Remove a previous temporary dir
if [ -e $TMPDIR ]; then
  rm -rf $TMPDIR
fi
mkdir $TMPDIR

# Prepare contents
install -d $TMPDIR/root/platform/kernel
install $ZIMAGE $TMPDIR/root/platform/kernel/os

# Make CramFS
mkcramfs -n 'Compressed' $TMPDIR $CRAMFS
if [ $(printf '\1' | od -dAn) -eq 1 ]; then
  cramfsswap $CRAMFS $CRAMFS.be
  mv $CRAMFS.be $CRAMFS
fi

# Remove temporary folder
rm -rf $TMPDIR


################
# CREATE IMAGE #
################

JFFS2="$1/rootfs.jffs2"
OUTFILE="$1/flash.img"

append_part() {
  first_block="$1"; # offset of image, in blocks
  max_size="$2";    # maximum size, in bytes
  file="$3";        # file to read from
  size=$(wc -c < "$file")
  if [ $size -gt $max_size ]; then
    echo "Part $file too big ($size found, $max_size max)" >&2
    exit 1
  fi
  dd bs=128k of="$OUTFILE" if="$file" seek="$first_block" conv=notrunc
}

# Create empty image for cramfs space
dd bs=128k of="$OUTFILE" if=/dev/null count=9

# Write each part
append_part 0 1179648 "$CRAMFS"
append_part 9 5111808 "$JFFS2"

echo -e "\nFirmware image generated: $OUTFILE"
