####################################################################################
#  ircombo makefile
####################################################################################

LOCAL_TEST	:= test_rec_bkt_nb test_rec_ts_nb test_rec_pes_nb test_pvr_nb

include ../../config.local


OBJS := test_rec_bkt_nb.o test_rec_ts_nb.o test_rec_pes_nb.o test_pvr_nb.o

COMMONHEADERS = $(DRV_INCLUDE_DIR)/os/os-types.h $(DRV_INCLUDE_DIR)/os/drv_debug.h


all: $(LOCAL_TEST)


test_rec_bkt_nb : test_rec_bkt_nb.c
	$(CC) -I$(LSP_INCLUDE_DIR) -I$(TOP_DIR)/include -o $@ $^

test_rec_pes_nb : test_rec_pes_nb.c
	$(CC) -I$(LSP_INCLUDE_DIR) -I$(TOP_DIR)/include -o $@ $^ -lpthread

test_rec_ts_nb : test_rec_ts_nb.c
	$(CC) -I$(LSP_INCLUDE_DIR) -I$(TOP_DIR)/include -o $@ $^

test_pvr_nb : test_pvr_nb.c
	$(CC) -I$(LSP_INCLUDE_DIR) -I$(TOP_DIR)/include -o $@ $^ -lpthread

.PHONY : clean

clean :
	rm -f $(OBJS) $(LOCAL_TEST)


install : $(LOCAL_TEST)
	@if [ ! -d $(LOCALBIN_DIR) ] ; then \
		echo "Creating target directory : "  $(LOCALBIN_DIR) ; \
		mkdir -p $(LOCALBIN_DIR) ; \
	fi
	install $(LOCAL_TEST) $(LOCALBIN_DIR)
