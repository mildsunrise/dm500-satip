####################################################################################
#  ircombo makefile
####################################################################################

LOCAL_TARGET	:= tv_t 

LOCAL_DEFINE    :=  

include ../config.local


COMMONHEADERS = $(DRV_INCLUDE_DIR)/os/os-types.h $(DRV_INCLUDE_DIR)/os/drv_debug.h



all: $(LOCAL_TARGET)

$(LOCAL_TARGET): tv_t.c vid_t.c ../demux/tv_function.c
		 $(CC) -I$(LSP_INCLUDE_DIR) -I$(TOP_DIR)/include -o $@ $^	

.PHONY : clean


clean : 
	rm -f $(OBJS) $(LOCAL_TARGET) 

install : $(LOCAL_TARGET)
	@if [ ! -d $(LOCALBIN_DIR) ] ; then \
		echo "Creating target directory : "  $(LOCALBIN_DIR) ; \
		mkdir -p $(LOCALBIN_DIR) ; \
	fi
	install $(LOCAL_TARGET) $(LOCALBIN_DIR)

