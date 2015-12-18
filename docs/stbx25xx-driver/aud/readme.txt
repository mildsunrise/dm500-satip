IBM STB audio decoder device driver for Linux(ver 0.9a)

Config and Build:
    Make sure you have the correct microcode.
    Edit drv.make,  change the below two lines according to your microcode file name
        AUDIO_MICRO_BIN = XXXXXXXX.bin
        AUDIO_MICRO_DAT = XXXXXXXX.dat


Main function:
	1. decode stream from demux (play, stop, freeze)
	2. decode clip from memory
	   There are two interfaces for this function, write and mmap. Transferring data
	   from user space to decoder buffer by mmap is more efficient than by write 
	   operation. Please refer to aud_t.c for detailed information.
	3. PES, ES, PCM playback are supported


Installation:

	
	(install aud.o to your module directory)  This step is unused now

	install aud_t to your application directory
	mknod /etc/adec_dev c 203 0
	
File List:
    ATOM layer:
		aud_atom.c
		aud_atom.h
		vid_atom_local.h
	OSI layer:
		aud_osi.c
		aud_osi.h
	OSD layer:
		aud_osd.c
		aud_osd.h
	INF layer
		aud_inf.c
		../include/aud/aud_inf.h
		../include/aud/aud_types.h
	APP layer:
	    aud_t.c			clip playback test
Note:
   major number of video device is 203

___________________________________________________________
