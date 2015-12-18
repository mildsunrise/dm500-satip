####################################################################################
#  ircombo makefile
####################################################################################


LOCAL_TARGET	:= ircombo.o

LOCAL_TEST	:= tstrawir

#  CONFIG_LAMAR   Define this to enable support of the IR for Lamar platform
#                        Default is for Redwood 5 platform.
#   _IR_MOUSE_SUPPORT   Define this to enable Acer keyboard mouse pad support.
#   _IR_RAWMODE_        Define this to enable capturing of all remote controll 
#                       codes without filtering and translating 
#                       **** This is not compatible with old applications ****

LOCAL_CFLAGS	= -I$(LSP_DIR)/drivers/char -D_IR_MOUSE_SUPPORT -DCONFIG_VULCAN  # -D_IR_RAWMODE_


include ../config.local


OBJS := ir_inf_combo.o ir_osi_acer_kbms.o ir_osi_hitachi.o ir_osi_device.o ir_atom.o

COMMONHEADERS = $(DRV_INCLUDE_DIR)/os/os-types.h $(DRV_INCLUDE_DIR)/gpt/gpt_ports.h $(DRV_INCLUDE_DIR)/os/drv_debug.h



all: $(LOCAL_TARGET)

%.o: %.c
	$(CC) $(BASE_MODCFLAGS) $(LOCAL_CFLAGS) -c -o $@ $<

$(LOCAL_TARGET) : $(OBJS)
	$(LD) -m elf32ppclinux -r -o $@ $(OBJS)
#	$(STRIP) $@

ir_inf_combo.o : ir_inf_combo.c \
		$(COMMONHEADERS) \
        	$(DRV_INCLUDE_DIR)/os/helper-queue.h \
        	$(DRV_INCLUDE_DIR)/os/os-sync.h \
        	$(DRV_INCLUDE_DIR)/os/os-generic.h \
        	$(DRV_INCLUDE_DIR)/ir/ir_osi_device.h \
        	$(DRV_INCLUDE_DIR)/ir/ir_osi_acer_kbms.h \
        	$(DRV_INCLUDE_DIR)/ir/ir_osi_hitachi.h


ir_osi_acer_kbms.o : ir_osi_acer_kbms.c ir_osi_acer-local.h \
		$(DRV_INCLUDE_DIR)/ir/ir_osi_acer_kbms.h \
		$(DRV_INCLUDE_DIR)/ir/ir_osi_device.h

ir_osi_hitachi.o : ir_osi_hitachi.c ir_osi_hitachi-local.h \
		$(DRV_INCLUDE_DIR)/ir/ir_osi_hitachi.h \
		$(DRV_INCLUDE_DIR)/ir/ir_osi_device.h

ir_osi_device.o	: ir_osi_device.c  ir_atom.h\
                $(COMMONHEADERS) \
                $(DRV_INCLUDE_DIR)/ir/ir_osi_device.h

ir_atom.o	: ir_atom.c  ir_atom.h\
                $(COMMONHEADERS) 


.PHONY : clean

test_apps: $(LOCAL_TEST)
	
$(LOCAL_TEST) : test/tstrawir.c
	$(CC) -I$(LSP_INCLUDE_DIR) -o $@ $<


clean : 
	rm -f $(OBJS) $(LOCAL_TARGET) $(LOCAL_TEST)

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
