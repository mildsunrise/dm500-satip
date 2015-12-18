####################################################################################
#  avcore makefile
####################################################################################

LOCAL_TARGET	:= av_core.o

LOCAL_TEST	:= 

LOCAL_CFLAGS := 

include ../config.local


OBJS := av_inf.o ../aud/aud.o ../vid/vidatom.o ../vid/vidosi.o ../demux/demux.o

COMMONHEADERS = 


all: $(LOCAL_TARGET)

%.o: %.c
	$(CC) $(BASE_MODCFLAGS) $(LOCAL_CFLAGS) -c -o $@ $<

$(LOCAL_TARGET) : $(OBJS)
	$(LD) -m elf32ppclinux -r -o $@ $(OBJS)
#	$(STRIP) $@

av_inf.o : av_inf.c 

.PHONY : clean

test_apps: 
	
clean : 
	rm -f $(OBJS) $(LOCAL_TARGET) $(LOCAL_TEST)

install : $(LOCAL_TARGET)
	@if [ ! -d $(INSTALL_ROOT)/lib/modules/$(VER)/misc ] ; then \
		echo "Creating target directory : "  $(INSTALL_ROOT)/lib/modules/$(VER)/misc ; \
		mkdir -p $(INSTALL_ROOT)/lib/modules/$(VER)/misc ; \
	fi
	install $(LOCAL_TARGET) $(INSTALL_ROOT)/lib/modules/$(VER)/misc
