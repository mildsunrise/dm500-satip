# dm500-satip

This is an effort to build a firmware image for the Dreambox
DM500 set-top-box, that effectively turns it into a
fully-operational [SAT>IP] server, with the only job of
streaming the transport stream straight from the DVB tuner.

The SAT>IP server can be accessed directly through various client
apps, or can be fed as input to [Tvheadend]. This last option
allows you to:

 - Join multiple SAT>IP servers (and USB tuners) and use them
   as a pool: feed multiple clients, or watch TV on one while
   the other records another programme, for instance.

 - Apply DVBCSA descrambling connecting to an [oscam] server.

 - Schedule programmes for recording.

 - Filter, demux and reencode streams.
 
 - The Tvheadend server may need to be started with an argument
   `--bindaddr $IPADDRESS` to detect SAT>IP tuners. See [issue.](https://github.com/openwrt/packages/issues/16500)

And many more.

## Install

You can download pre-made images from the [Releases] page,
or you can build them yourself, see `BUILDING.md`.

Building them yourself gives you full control over the generated
image: you can add drivers for more devices, add software (such
as an SSH server), change the default root password and more.

Some important things to consider:

 - **The firmware is provided without warranty of any kind.**
   It's been tested on multiple DM500S including clones, and if it
   doesn't work you should be able to flash another image.
   **It has no time bombs** (see last section).

 - **This image is currently for DM500S only.** Flashing it on
   a DM500T or DM500C will get you a nice SAT>IP server with 0
   tuners, because support for the T/C frontends is missing.

   I've been unable to find a DM500T or DM500C at a decent price.
   If you have one to spare, you can donate it (contact me) and
   I'll happily implement the support, it shouldn't take much (not
   guaranteeing anything though).

If you aren't flashing via DreamUp, make sure you flash to partition
0 (labeled "CramFS + SquashFS" or similar) using instructions below.

Logon to dreambox via 'telnet' and check if hardware info matches:
```
uname -a
#Linux dreambox 2.6.9 #3 Sun Apr 27 20:01:34 WEST 2008 ppc unknown

cat /proc/mtd
#dev:    size   erasesize  name
#mtd0: 00600000 00020000 "DreamBOX cramfs+squashfs"
#mtd1: 001c0000 00020000 "DreamBOX jffs2"
#mtd2: 00040000 00020000 "DreamBOX OpenBIOS"
#mtd3: 007c0000 00020000 "DreamBOX (w/o bootloader)"
#mtd4: 00800000 00020000 "DreamBOX (w/ bootloader)"
#mtd5: 004e0000 00020000 "DreamBOX SquashedFS"
#mtd6: 00120000 00020000 "DreamBOX Cramfs"

cat /proc/cpuinfo 
#processor	: 0
#cpu		: STBx25xx
#clock		: 252MHz
#revision	: 9.80 (pvr 5151 0950)
#bogomips	: 250.88
#machine		: Dream Multimedia TV Dreambox
#plb bus clock	: 63MHz

df -h
#Filesystem                Size      Used Available Use% Mounted on
#/dev/root                 3.9M      3.9M         0 100% /
#/dev/mtdblock/1           1.8M    448.0k      1.3M  25% /var

free
#              total         used         free       shared      buffers
#  Mem:        30184        23096         7088            0         2348
# Swap:            0            0            0
#Total:        30184        23096         7088
```
Before flashing its good practice to backup mtd0-6. With 'telnet' copy 
each mtd to '/tmp/' and transfer it via FileZilla:
```
cat /dev/mtd/0 > /tmp/mtd0 #creates backup of first partition takes time!
#Back it up with FileZilla and remove when done via FileZilla or telnet
rm /tmp/mtd0

#repeat previous steps for mtd1 mtd2 mtd3 mtd4 mtd5 mtd6
#remove has to be done 1 at the time otherwise space will run out!
```
If you backed up all mtd0-6 and transfered the replacement firmware image
'dm500-satip-2.3.img' back via FileZilla to '/tmp/' flash with following command:
```
eraseall /dev/mtd/0 && cp /tmp/dm500-satip-2.3.img /dev/mtd/0;sync;#reboot
```

### The static version

Starting at version 2.3, a 'static' image is distributed along with
the regular one.

If the regular one won't boot on your box (probably because of
corrupted flash memory) try to flash the static one. It's pretty
much the same, but uses a **read-only FS** so it'll probably work
correctly.

Because of the read-only FS, all changes you make through SSH
(including changing the root password, firewall, static IP, init
script) will go away on the next boot. If you need those changes
permanently, build your own image.

Also, a different SSH host key is generated on every boot, so you'll
need to revoke the key each time SSH gets mad.

## Usage

When powered, DM500 will obtain an IP by DHCP, and the
SAT>IP server will start. To verify that it's working, browse to
`http://<ip of DM500>/`, you should see a table listing one
tuner.

You then use it like any other SAT>IP server. For instance, to
use it with a Tvheadend server, put it on the same network and
you should see the DM500 appear in the inputs tab.

**Important:** The SAT>IP server does *not* support full TS
streaming (aka `pids=all`)! For Tvheadend users, this means you
should untick the "Full Mux Rx mode supported" checkbox if it's
ticked, in the SAT>IP server entry.

**Important:** The SAT>IP server does *not* support more than
29 concurrent PIDs! For Tvheadend users, this means you should
set "Maximum PIDs" to 29, in the SAT>IP server entry.

**Important:** The SAT>IP server does *not* support DVB-S2 (aka
HD channels)! An option [has been added in TVHeadend](http://tvheadend.org/issues/4466)
to restrict the delivery systems for a DVB card, set it.

If you found a bug or have a suggestion for the firmware image,
feel free to open an issue on this repo.

The DM500 also has an SSH server running, you can login with
`root` and password `dreambox`. Thus, **make sure the DM500 is
behind a firewall**.

## Design

**This image is not based on the official firmware.** Instead,
it's an effort to build a firmware from scratch, basing on the
efforts of the (currently abandoned) [stbx25xx-linux] project,
which ported Linux 2.6.28 to the IBM STBx25xx (the SoC in DM500).

I [forked][kernel-fork] the stbx25xx-linux project and added support
for the DM500 and a few of its hardware (reverse engineered some of
the closed-source drivers). Currently, the following hardware from
the DM500 is usable and has been tested:

 - DVB satellite frontend (STV0299), PLL tuner
 - DVB demuxer
 - GPIO (two LEDs, LNB voltage, etc.)
 - I2C bus
 - RS-232 UART
 - NE2000-based network
 - NOR flash memory

Hardware that has support but has not been tested, is not enabled
in `config_kernel` or isn't finished:

 - DVB video decoder, audio decoder
 - Audio output
 - Video output

Unsupported hardware, or hardware which hasn't been investigated:

 - DVB terrestrial and cable frontends
 - Smartcard reader
 - IR receiver

The files are put into a JFFS2 filesystem, which ends up in the
partition next to the CramFS (where a SquashFS would go, in traditional
firmwares).

The audio and video decoders need firmware blobs to be supplied, so make
sure to enable `FW_LOADER` and set `COPY_FIRMWARE` to `1` in `build-fs.sh`.
Support for the video output and audio output are also in the works.

[Buildroot] is what builds the toolchain, software, kernel, filesystem,
and calls `build-img.sh` to produce the final firmware image.

[minisatip] is the SAT>IP server implementation used by this image. It's
version 0.4 with the modifications at [the `dm500-satip-2.3` branch][minisatip-compare]
in my fork.



[SAT>IP]: https://en.wikipedia.org/wiki/Sat-IP
[oscam]: http://www.streamboard.tv/oscam
[minisatip]: https://github.com/catalinii/minisatip
[tvheadend]: https://tvheadend.org/
[releases]: https://github.com/mildsunrise/dm500-satip/releases
[buildroot]: https://buildroot.org
[stbx25xx-linux]: http://stbx25xx-linux.sf.net
[kernel-fork]: https://github.com/mildsunrise/stbx25xx-linux
[minisatip-compare]: https://github.com/catalinii/minisatip/compare/0.4...mildsunrise:dm500-satip-2.3
