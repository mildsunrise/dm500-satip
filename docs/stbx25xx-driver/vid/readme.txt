IBM STB video decoder device driver for Linux(ver 0.9a)

Config and build:

    Make sure your video microcode file is correct, we assume it is named as "sXvstdXX.bin"
    Edit atom.make,  change the line "VIDEO_MICRO_CODE = xxx"  according to your micro code
    

Main function:
	1. decode stream from demux (play, stop, freeze)
	2. decode clip from memory
	   There are two interfaces for this function, write and mmap. Transferring data
	   from user space to decoder buffer by mmap is more efficient than by write 
	   operation. Please refer to vclip_t.c for detailed information.
	3. Both PAL and NTSC stream are supported

Installation:

	(install vidatom.o to your module directory)   This step is not used now
	(install vidosi.o to your module directory)    This step is not used now

	install vclip_t to your application directory
	install tv_t to your application directory
	mknod /etc/vdec_dev c 202 0
	
File List:
    ATOM layer:
		vid_atom.c
		vid_atom_clip.c
		vid_atom_int.c
		vid_atom_others.c
		vid_atom_sync.c
		vid_atom.h
		vid_atom_local.h
	OSI layer:
		vid_osi.c
		vid_osi.h
	OSD layer:
		vid_osd.c
		vid_osd.h
	INF layer
		vid_inf.c
		../include/vid/vid_inf.h
		../include/vid/vid_types.h
	APP layer:
	    vclip_t.c		clip playback test
		tv_t.c			tv function test
		vid_t.c			video function subroutine (scale function can not work yet)
Note:
   major number of video device is 202


