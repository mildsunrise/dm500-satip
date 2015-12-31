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

And many more.

## Install

You can download pre-made images from the [Releases] page,
or you can build them yourself, see `BUILDING.md`.

Building them yourself gives you full control over the generated
image: you can add drivers for more devices, add software (such
as an SSH server), change the default root password and more.

Some important things to consider:

 - **The firmware is provided without warranty of any kind.**
   It's been tested on multiple DM500S STBs though, and if it
   doesn't work you should be able to flash another image.
   **It has no time bombs** (see last section).

 - **This image is currently for DM500S only.** If you flash it
   on a DM500T or DM500C it will work the same, you'll see the
   SAT>IP server but you won't find any tuner in it. This is
   because support for the T and C tuners is missing (see next
   section).

 - **You should have an RS-232 adapter at hand.** DreamUp has an
   option to flash over the network, and it'll work just as well,
   but **after this is flashed, the only way to reflash is via
   RS-232**.

If you aren't flashing via DreamUp, make sure you flash to partition
0 (labeled "CramFS + SquashFS" or similar).

## Usage

Once the image is flashed, disconnect power from the DM500
for some seconds, then connect it again. This only needs to
be done the first time.

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

If you found a bug or have a suggestion for the firmware image,
feel free to open an issue on this repo.

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

Hardware that has support but has not been tested, is not enabled
in `config_kernel` or isn't finished:

 - DVB video decoder, audio decoder
 - Audio output
 - Video output
 - Flash memory

Unsupported hardware, or hardware which hasn't been investigated:

 - DVB terrestrial and cable frontends
 - Smartcard reader
 - IR receiver

Because the flash memory isn't supported yet, we're forced to put
the FS into an initrd.

The audio and video decoders need firmware blobs to be supplied, so make
sure to enable `FW_LOADER` and set `COPY_FIRMWARE` to `1` in `build-fs.sh`.
Support for the video output and audio output are also in the works.

[Buildroot] is what builds the toolchain, software, kernel, filesystem,
and calls `build-img.sh` to produce the final firmware image.

[minisatip] is the SAT>IP server implementation used by this image. It's
version 0.4 with the modifications at [the `dm500-satip` branch][minisatip-compare]
in my fork.



[SAT>IP]: https://en.wikipedia.org/wiki/Sat-IP
[oscam]: http://www.streamboard.tv/oscam
[minisatip]: https://github.com/catalinii/minisatip
[tvheadend]: https://tvheadend.org/
[releases]: https://github.com/mildsunrise/dm500-satip/releases
[buildroot]: https://buildroot.org
[stbx25xx-linux]: http://stbx25xx-linux.sf.net
[kernel-fork]: https://github.com/mildsunrise/stbx25xx-linux
[minisatip-compare]: https://github.com/catalinii/minisatip/compare/0.4...mildsunrise:dm500-satip
