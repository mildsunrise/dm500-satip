# dm500-satip

This is an effort to build a firmware image for the Dreambox
DM500 set-top-box, that effectively turns it into a
fully-operational [SAT>IP] server, with the only job of
streaming the TS stream straight from the DVB tuner.

The SAT>IP server can be accessed directly through various client
apps, or can be feed as input to [Tvheadend]. This last option
allows you to:

 - Join multiple SAT>IP servers (and USB tuners) and use them
   as a pool: feed multiple clients, or watch TV on one while
   the other records another programme.

 - Apply DVBCSA descrambling connecting an [oscam] server.

 - Schedule programmes for recording.

 - Filter, demux and reencode streams.

And many more. The SAT>IP server itself is [minisatip].

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
   **It has no time bombs** (see next section).

 - **This image is currently for DM500S only.** If you flash it
   on a DM500T or DM500C it will work the same, you'll see the
   SAT>IP server but you won't find any tuner in it. This is
   because support for the T and C tuners is missing (see next
   section).

 - **You should have an RS-232 adapter at hand.** DreamUp has an
   option to flash over the network, and it'll work just as well,
   but **after this is flashed, the only way to reflash is via
   RS-232**.

If you aren't via DreamUp, make sure you flash to partition 0 (labeled
"CramFS + SquashFS" or similar).

## Usage

Once the image is flashed, disconnect power from the DM500
for some seconds, then connect it again. This only needs to
be done the first time.

When powered, DM500 will obtain an IP by DHCP, and the
SAT>IP server will start. To test it's working, browse to
`http://<ip of DM500>:8080/desc.xml` and verify that you see
`DVBS2-1` at the end, instead of `DVBS2-0`.

You use it like any other SAT>IP server. If it's on the same
network as your Tvheadend server, restart it and you should
see the DM500 appear in the inputs tab.

If you found a bug or have a suggestion for the firmware image,
feel free to open an issue on this repo.

## Design

**This image is not based on the official firmware.** Instead,
it's an effort to build a firmware from scratch, basing on the
efforts of the [stbx25xx-linux] project, which ported Linux
2.6.28 to the IBM STBx25xx (the SoC in DM500).

I [forked][kernel-fork] the stbx25xx-linux project and added support
for the DM500 and a few of its hardware. Currently, the kernel supports:

 - DVB frontend (STV0299), PLL tuner
 - DVB demuxer, video decoder, audio decoder
 - GPIO (two LEDs, LNB voltage, etc.)
 - I2C bus
 - The RS-232 UART
 - NE2000-based network

Currently unsupported hardware (or not finished):

 - Flash memory
 - Audio output
 - Video output
 - Smartcard reader
 - IR receiver

Because the flash memory isn't supported yet, we're forced to put
the FS into an initrd.

The audio and video decoders need firmware blobs to be supplied, so
make sure to set `DVB_STBx25xx` to `m` instead of `y`, enable `FW_LOADER`
and set `COPY_FIRMWARE` to `1` in `build-fs.sh`. Support for the video
output and audio output are also in the works.

[Buildroot] is what builds the toolchain, software, kernel, filesystem,
and calls `build-img.sh` to produce the final firmware image.
