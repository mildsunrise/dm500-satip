####################################################################################
#  ircombo makefile
####################################################################################

LOCAL_TARGET	:= vidosi.o

LOCAL_DEFINE    := -DEXPORT_SYMTAB 

include ../config.local


OBJS := vid_osi.o  vid_osd.o vid_inf.o vid_osi_scr.o

COMMONHEADERS = $(DRV_INCLUDE_DIR)/os/os-types.h $(DRV_INCLUDE_DIR)/hw/gpt_ports.h $(DRV_INCLUDE_DIR)/os/drv_debug.h



all: $(LOCAL_TARGET)

%.o: %.c
	$(CC) $(LOCAL_DEFINE) $(BASE_MODCFLAGS) -c -o $@ $<

$(LOCAL_TARGET) : $(OBJS)
	$(LD) -m elf32ppclinux -r -o $@ $(OBJS)
#	$(STRIP) $@

.PHONY : clean

test_apps:

clean : 
	rm -f $(OBJS) $(LOCAL_TARGET) 

install : $(LOCAL_TARGET)

