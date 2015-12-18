####################################################################################
#  ircombo makefile
####################################################################################

LOCAL_TARGET	:= ee_t  

LOCAL_DEFINE    := -DPALLAS

include ../config.local


COMMONHEADERS = $(DRV_INCLUDE_DIR)/os/os-types.h $(DRV_INCLUDE_DIR)/hw/gpt_ports.h $(DRV_INCLUDE_DIR)/os/drv_debug.h



all: $(LOCAL_TARGET)

$(LOCAL_TARGET): eeprom.c ee_t.c 
		 $(CC) -I$(DRV_INCLUDE_DIR) $(LOCAL_DEFINE) -I$(LSP_INCLUDE_DIR) -o $@ $^	

.PHONY : clean

test_apps:


clean : 
	rm -f $(OBJS) $(LOCAL_TARGET) 

install : $(LOCAL_TARGET)
	@if [ ! -d $(LOCALBIN_DIR) ] ; then \
		echo "Creating target directory : "  $(LOCALBIN_DIR) ; \
		mkdir -p $(LOCALBIN_DIR) ; \
	fi
	install $(LOCAL_TARGET) $(LOCALBIN_DIR)

