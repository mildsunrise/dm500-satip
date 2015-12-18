#!/bin/sh

[ -z "$1" ] && echo "Error: should be called from udhcpc" && exit 1

case "$1" in
	renew|bound)
		if [ -n "$hostname" ]; then
			hostname $hostname
		fi
		;;
esac

exit 0
