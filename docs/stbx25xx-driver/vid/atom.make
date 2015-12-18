####################################################################################
#  video atom makefile
####################################################################################

####################################################################################
#   Make sure you have the correct micro code file listed here
####################################################################################

VIDEO_MICRO_CODE =   s025v306.bin


LOCAL_TARGET	:= vidatom.o

LOCAL_DEFINE    := -DEXPORT_SYMTAB 

include ../config.local


OBJS := vid_atom.o vid_atom_clip.o vid_atom_int.o vid_atom_others.o vid_atom_sync.o vid_atom_denc.o

COMMONHEADERS = $(DRV_INCLUDE_DIR)/os/os-types.h $(DRV_INCLUDE_DIR)/hw/gpt_ports.h $(DRV_INCLUDE_DIR)/os/drv_debug.h



all: vid_uc.h $(LOCAL_TARGET)

vid_uc.h:  $(VIDEO_MICRO_CODE)
	../tools/img2header  $< $@ -x -d vid_ucode -n vid_ucode_len -s

%.o: %.c
	$(CC) $(LOCAL_DEFINE) $(BASE_MODCFLAGS) -c -o $@ $<

$(LOCAL_TARGET) : $(OBJS)
	$(LD) -m elf32ppclinux -r -o $@ $(OBJS)
#	$(STRIP) $@

.PHONY : clean

test_apps:

clean : 
	rm -f $(OBJS) $(LOCAL_TARGET)  vid_uc.h

install : $(LOCAL_TARGET)

