####################################################################################
#  scp makefile
####################################################################################

LOCAL_TARGET	:= scp_drv.o
LOCAL_TEST      := scpt

include ../config.local

OBJS := scp_atom.o scp_osd.o scp_osi.o scp_inf.o xilinx.o

#COMMONHEADERS = $(DRV_INCLUDE_DIR)/os/os-types.h $(DRV_INCLUDE_DIR)/hw/gpt_ports.h $(DRV_INCLUDE_DIR)/os/drv_debug.h


all: $(LOCAL_TARGET) $(LOCAL_TEST)

%.o: %.c
	$(CC) $(LOCAL_DEFINE) $(BASE_MODCFLAGS) -c -o $@ $<

$(LOCAL_TARGET) : $(OBJS)
	$(LD) -m elf32ppclinux -r -o $@ $(OBJS)
#	$(STRIP) $@

.PHONY : clean

test_apps: $(LOCAL_TEST)

$(LOCAL_TEST) : test/mu_scp.c  test/sflash.c test/sflash_test.c test/sem.c
	$(CC) -I$(LSP_INCLUDE_DIR) -I$(DRV_INCLUDE_DIR) -o $@ $^

clean : 
	rm -f $(OBJS) $(LOCAL_TARGET) $(LOCAL_TEST)

install : $(LOCAL_TARGET) $(LOCAL_TEST)
#install : $(LOCAL_TARGET)
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


