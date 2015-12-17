#!/bin/sh
# Script called by Buildroot after all images have been generated.
# This builds the final, flashable image, by wrapping zImage into
# a cramfs, for the bootloader to read.

ZIMAGE=$1/zImage
TMPDIR=$1/cramfs.tmp
CRAMFS=$1/cramfs
OUTFILE=$1/flash.img

set -e

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

# Make image
cat $CRAMFS > $OUTFILE

# Remove temporary folder
rm -rf $TMPDIR

# FIXME: fail if image is too big
