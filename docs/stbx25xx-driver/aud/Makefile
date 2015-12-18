########################################################################################
#  main make file for aud
########################################################################################

all : aud_drv #oss

aud_drv : 
	make -f drv.make

#oss:
#	make -f oss.make

clip_apps:
	make -f clip.make

test_apps: clip_apps

install :
	make -f drv.make install
	make -f clip.make install
#	make -f oss.make install

clean :	
	make -f drv.make clean
	make -f clip.make clean
#	make -f oss.make clean


