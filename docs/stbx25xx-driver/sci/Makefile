####################################################################################
#  sci makefile
####################################################################################

LOCAL_TARGET	:= sci_drv.o
LOCAL_TEST	:= scit

include ../config.local

OBJS := sci_inf.o sci_osd.o sci_osi.o sci_atom.o

COMMONHEADERS = $(DRV_INCLUDE_DIR)/os/os-types.h $(DRV_INCLUDE_DIR)/os/drv_debug.h

all: $(LOCAL_TARGET) $(LOCAL_TEST)

%.o: %.c
	$(CC) $(BASE_MODCFLAGS) -D__DRV_DEBUG -c -o $@ $<


%.o: %.c
	$(CC) $(BASE_MODCFLAGS) -c -o $@ $<

$(LOCAL_TARGET) : $(OBJS)
	$(LD) -m elf32ppclinux -r -o $@ $(OBJS)
#	$(STRIP) $@

sci_inf.o : sci_inf.c sci_osi.h \
			$(COMMONHEADERS) \
		    $(DRV_INCLUDE_DIR)/sci/sci_inf.h \
		    $(DRV_INCLUDE_DIR)/sci/sci_global.h

sci_osd.o : sci_osd.c sci_osd.h sci_osi.h sci_atom.h \
			$(COMMONHEADERS) \
		    $(DRV_INCLUDE_DIR)/os/os-interrupt.h \
		    $(DRV_INCLUDE_DIR)/os/os-sync.h \
		    $(DRV_INCLUDE_DIR)/sci/sci_global.h

sci_osi.o : sci_osi.c sci_osi.h sci_atom.h \
			$(COMMONHEADERS) \
			$(DRV_INCLUDE_DIR)/os/os-interrupt.h \
			$(DRV_INCLUDE_DIR)/os/os-sync.h \
		    $(DRV_INCLUDE_DIR)/sci/sci_global.h

sci_atom.o : sci_atom.c sci_atom.h \
			$(COMMONHEADERS) \
		    $(DRV_INCLUDE_DIR)/sci/sci_global.h


.PHONY : clean

test_apps: $(LOCAL_TEST)

$(LOCAL_TEST) : test/mu_sci.c test/sci_prot.c 
#        	$(DRV_INCLUDE_DIR)/sci/sci_inf.h \
#        	$(DRV_INCLUDE_DIR)/sci/sci_global.h
	$(CC) -I$(LSP_INCLUDE_DIR) -I$(DRV_INCLUDE_DIR) -o $@ $^


clean : 
	rm -f $(OBJS) $(LOCAL_TARGET) $(LOCAL_TEST) *.o

install : $(LOCAL_TARGET) $(LOCAL_TEST)
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
