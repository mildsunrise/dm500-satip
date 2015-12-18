####################################################################################
#  TTX makefile
####################################################################################

LOCAL_TARGET	:= ttx.o
LOCAL_TEST	:= test_ttx

include ../config.local


OBJS := ttx_osd_inf.o ttx_drv.o ttx_vbi.o ttx_mbuf.o powerpc.o


COMMONHEADERS = $(DRV_INCLUDE_DIR)/os/os-types.h $(DRV_INCLUDE_DIR)/os/drv_debug.h


all: $(LOCAL_TARGET)

%.o: %.c
	$(CC) $(BASE_MODCFLAGS) -c -o $@ $<

ttx_osd_inf.o : ttx_osd_inf.c    \
		$(COMMONHEADERS) \

ttx_drv.o :     ttx_drv.c        \
		$(COMMONHEADERS) \

ttx_vbi.o :     ttx_vbi.c        \
		$(COMMONHEADERS) \

ttx_mbuf.o :    ttx_mbuf.c       \
		$(COMMONHEADERS) \

powerpc.o :     powerpc.c        \
		$(COMMONHEADERS) \


test_apps :     $(LOCAL_TEST)

$(LOCAL_TARGET) : $(OBJS)
	$(LD) -m elf32ppclinux -r -o $@ $(OBJS)
#	$(STRIP) $@


$(LOCAL_TEST) : test_ttx.c
	$(CC) -I$(LSP_INCLUDE_DIR) -I$(TOP_DIR)/include -lpthread -o $@ $^


.PHONY : clean

clean :
	rm -f $(OBJS) $(LOCAL_TARGET) $(LOCAL_TEST)


install : $(LOCAL_TARGET)  $(LOCAL_TEST)
	@if [ ! -d $(INSTALL_ROOT)/lib/modules/$(VER)/misc ] ; then \
		echo "Creating target directory : "  $(INSTALL_ROOT)/lib/modules/$(VER)/misc ; \
		mkdir -p $(INSTALL_ROOT)/lib/modules/$(VER)/misc ; \
	fi
	install $(LOCAL_TARGET) $(INSTALL_ROOT)/lib/modules/$(VER)/misc
	@if [ ! -d $(LOCALBIN_DIR) ] ; then \
		echo "Creating target directory : "  $(LOCALBIN_DIR) ; \
		mkdir -p $(LOCALBIN_DIR) ; \
	fi
	install $(LOCAL_TEST) $(LOCALBIN_DIR)
