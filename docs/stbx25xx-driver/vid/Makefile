########################################################################################
#  main make file for vid
########################################################################################

all : vid_atom vid_osi clip tv

.BOSS : configs

vid_atom : 
	make -f atom.make

vid_osi : 
	make -f osi.make

clip:
	make -f clip.make

tv:
	make -f tv.make

test_apps:

install :
	make -f atom.make install
	make -f osi.make install
	make -f clip.make install
	make -f tv.make install

clean :	
	make -f atom.make clean
	make -f osi.make clean
	make -f clip.make clean
	make -f tv.make clean
