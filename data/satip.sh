#!/bin/sh
# Called at startup to start serving SAT>IP.

# Load DVB module
modprobe dvb_stbx25xx

# Start SAT>IP server
minisatip -R /usr/share/minisatip/html -x 80 \
  -f > /dev/null &

exit 0
