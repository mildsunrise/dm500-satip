# Building dm500-satip

**Important:** Building no longer possible with Ubuntu 16.04 due to upgrade from
Make 3.82 to Make 4.1, and also Perl. I was able to build by downgrading these two
things, but I strongly recommend using Ubuntu 14.04 (or similar).

Start by installing the necessary tools:

    sudo apt-get install build-essential cramfsprogs cramfsswap file coreutils curl

Then download and extract Buildroot at this repository, so that
you end with a `buildroot-XXXX.XX.X` directory next to the `config_*`
files. I used version 2017.05-rc3 but other versions may work as well:

    curl https://buildroot.org/downloads/buildroot-2017.05-rc3.tar.gz | tar xz

For convenience, rename `buildroot-XXXX.XX.X` to just `buildroot`:

    mv buildroot-2017.05-rc3 buildroot

Then supply Buildroot with one of the configuration files, depending on which version
you want to build:

    cp config_buildroot buildroot/.config         # build regular version
    cp config_buildroot_static buildroot/.config  # build static version

At this point, you can `cd buildroot` and optionally tweak the configuration
by doing `make menuconfig`, `make xconfig` or whatever method you like.

## Compiling

When ready for compiling, you can start building the image by doing (from
inside Buildroot's directory):

    LANG=C LANGUAGE=C make

The `LANG=C LANGUAGE=C` is not needed in theory, but we are building old code
which may assume that the output of some tools is in English.

And again, because we are building very old code, we will inevitably run
into some build errors. The first one I found:

~~~
  HOSTCC  scripts/unifdef
scripts/unifdef.c:209:25: error: conflicting types for ‘getline’
 static Linetype         getline(void);
                         ^
In file included from scripts/unifdef.c:70:0:
/usr/include/stdio.h:678:20: note: previous declaration of ‘getline’ was here
 extern _IO_ssize_t getline (char **__restrict __lineptr,
                    ^
make[3]: *** [scripts/unifdef] Error 1
make[2]: *** [__headers] Error 2
~~~

This is fixed by editing `output/build/linux-headers-2.6.28/scripts/unifdef.c`
and replacing all three instances of `getline` with another name, say `get_line`.
After that, issue the `LANG=C LANGUAGE=C make` command and the build should
proceed.

Another error mentions the `include/linux/socket.h` and some redefinitions
of `struct iovec`. To fix it, edit
`output/build/linux-headers-2.6.28/usr/include/linux/socket.h` and replace the line:

~~~
#if defined(__KERNEL__) || !defined(__GLIBC__) || (__GLIBC__ < 2)
~~~

with just

~~~
#if defined(__KERNEL__)
~~~

Try again and the build should proceed. If you find more errors, please open
an issue on this repo.

When the build has finished, you should have your image at
`output/images/flash.img`, ready to flash with i.e. DreamUp.
