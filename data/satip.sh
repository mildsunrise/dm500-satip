#!/bin/sh
# Called at startup to start serving SAT>IP.

# Load DVB module
modprobe dvb_stbx25xx

# Create devices
mkdir -p /dev/dvb/adapter0 && cd /dev/dvb/adapter0
[ -e /sys/class/dvb/dvb0.audio0 ] && mknod audio0 u 212 1
[ -e /sys/class/dvb/dvb0.demux0 ] && mknod demux0 u 212 4
[ -e /sys/class/dvb/dvb0.dvr0 ] && mknod dvr0 u 212 5
[ -e /sys/class/dvb/dvb0.frontend0 ] && mknod frontend0 u 212 3
[ -e /sys/class/dvb/dvb0.net0 ] && mknod net0 u 212 7
[ -e /sys/class/dvb/dvb0.video0 ] && mknod video0 u 212 0

# Start SAT>IP server and inetd
minisatip -R /usr/share/minisatip/html -x 80
inetd

exit 0
