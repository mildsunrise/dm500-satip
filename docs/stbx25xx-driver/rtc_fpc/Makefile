####################################################################################
#  rtc_fpc makefile
####################################################################################

LOCAL_TARGET	:= stbrtc.o
LOCAL_TEST	:= rtc_t

LOCAL_CPP_OPT	:= 
#		-D__GFX_INF_HELPER_DEBUG -D__GFX_INF_DEBUG -D__GFX_ATOM_DEBUG \
#		-D__GFX_OSI_DEBUG -D__GFX_SURFACE_DEBUG -D__OSD_ATOM_DEBUG -D__OSD_FB_DEBUG \
#		-D__OSD_OSI_DEBUG


include ../config.local


OBJS := rtc_fpc.o


all: $(LOCAL_TARGET) $(LOCAL_TEST)

%.o: %.c
	$(CC) $(BASE_MODCFLAGS) $(LOCAL_CPP_OPT) -c -o $@ $<

$(LOCAL_TARGET) : $(OBJS)
	$(LD) -m elf32ppclinux -r -o $@ $^

rtc_fpc.o : rtc_fpc.c $(DRV_INCLUDE_DIR)/rtc_fpc/rtc_fpc.h rtc_fpc_local.h

$(LOCAL_TEST) : rtc_t.c $(DRV_INCLUDE_DIR)/rtc_fpc/rtc_fpc.h
	$(CC) -I$(DRV_INCLUDE_DIR) -o $@ $<

test_apps: $(LOCAL_TEST)

.PHONY : clean

clean : 
	rm -f *~ $(OBJS) $(LOCAL_TARGET) $(LOCAL_TEST)

install : $(LOCAL_TARGET) $(LOCAL_TEST)
	@if [ ! -d $(INSTALL_ROOT)/lib/modules/$(VER)/misc ] ; then \
		echo "Creating target directory : "  $(INSTALL_ROOT)/lib/modules/$(VER)/misc ; \
		mkdir -p $(INSTALL_ROOT)/lib/modules/$(VER)/misc ; \
	fi
	install $(LOCAL_TARGET) $(INSTALL_ROOT)/lib/modules/$(VER)/misc
	install $(LOCAL_TEST) $(LOCALBIN_DIR)

 
