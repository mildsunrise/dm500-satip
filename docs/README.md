This folder is a general storage of valuable info I have been
able to rescue, that is useful when developing for the DM500.

It includes:

 - `205-00453-0-STBx25.pdf`: PDF with the overall architecture of
   the STBx25xx.

 - `ibmstbx25.{c,h}`: Platform code for the STBx25xx, extracted from
   the Linux repository just before the files were removed.

 - `linuxppc-2.6.9-r1.diff`, `linuxppc-2.6.9-dream-s7.diff`: these
   two patches must be applied, in order, to Linux 2.6.9 in order to
   get the working kernel that is used in the official firmware images.

   They do *not* add support for DVB, framebuffer, smartcard, audio
   and GPIO: those are provided via a `head.ko` module, for which
   no source is available. It can be found at any DM500 image (make
   sure it doesn't come with a timebomb if you have a clone).

 - `stbx25xx-driver`: folder containing what appears to be the source
   code for IBM STBx25xx drivers. The firmware blobs were extracted
   from this archive.
