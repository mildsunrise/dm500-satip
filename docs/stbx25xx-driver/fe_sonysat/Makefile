####################################################################################
#  tuner driver makefile
####################################################################################

LOCAL_TARGET	:= fe_sonysat
LOCAL_TEST	:= tuner_test

LOCAL_CPP_OPT	:= 
#		-D__GFX_INF_HELPER_DEBUG -D__GFX_INF_DEBUG -D__GFX_ATOM_DEBUG \
#		-D__GFX_OSI_DEBUG -D__GFX_SURFACE_DEBUG -D__OSD_ATOM_DEBUG -D__OSD_FB_DEBUG \
#		-D__OSD_OSI_DEBUG


include ../config.local


OBJS := tuner.o cstuner.o


all: $(LOCAL_TARGET) $(LOCAL_TEST)

%.o: %.c
	$(CC) $(BASE_MODCFLAGS) $(LOCAL_CPP_OPT) -c -o $@ $<

$(LOCAL_TARGET) : $(OBJS)
	$(LD) -m elf32ppclinux -r -o $@ $^

tuner.o : tuner.c tuner.h

cstuner.o : cstuner.c cstuner.h tuner.h

$(LOCAL_TEST) : tuner_test.c tuner.h
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

 
