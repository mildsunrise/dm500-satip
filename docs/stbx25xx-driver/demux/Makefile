####################################################################################
#  demux makefile
####################################################################################

LOCAL_TARGET	:= demux.o
LOCAL_TEST	:= test_av test_rec_ts test_pvr test_rec_pes 

include ../config.local


OBJS := xp_osd_inf.o xp_osd_drv.o xp_osi.o xp_osi_chann.o xp_osi_clk.o xp_atom_dcr.o \
	xp_osi_filte.o xp_osi_inter.o xp_osi_key.o xp_osi_queue.o xp_osi_pcr.o xp_osi_cchan.o \
	pvr_osi.o pvr_osd.o 

COMMONHEADERS = $(DRV_INCLUDE_DIR)/os/os-types.h $(DRV_INCLUDE_DIR)/os/drv_debug.h


all: $(LOCAL_TARGET)

%.o: %.c
	$(CC) $(BASE_MODCFLAGS) -c -o $@ $<

$(LOCAL_TARGET) : $(OBJS)
	$(LD) -m elf32ppclinux -r -o $@ $(OBJS)
#	$(STRIP) $@

xp_osd_inf.o : xp_osd_inf.c \
		$(COMMONHEADERS) \

xp_osd_drv.o : xp_osd_drv.c \
		$(COMMONHEADERS) \

xp_osi.o     : xp_osi.c \
		$(COMMONHEADERS) \

xp_osi_chann.o : xp_osi_chann.c \
		$(COMMONHEADERS) \

xp_osi_clk.o : xp_osi_clk.c \
		$(COMMONHEADERS) \

xp_atom_dcr.o : xp_atom_dcr.c \
		$(COMMONHEADERS) \

xp_osi_filte.o : xp_osi_filte.c \
		$(COMMONHEADERS) \

xp_osi_inter.o : xp_osi_inter.c \
		$(COMMONHEADERS) \
		
xp_osi_key.o : xp_osi_key.c \
		$(COMMONHEADERS) \

xp_osi_queue.o : xp_osi_queue.c \
		$(COMMONHEADERS) \

xp_osi_pcr.o : xp_osi_pcr.c \
		$(COMMONHEADERS) \

pvr_osi.o : pvr_osi.c \
		$(COMMONHEADERS) \

pvr_osd.o : pvr_osd.c \
		$(COMMONHEADERS) \

pvrtester:
	make -C pvrtest

test_apps : $(LOCAL_TEST) pvrtester

test_av : test_av.c tv_function.c
	$(CC) -I$(LSP_INCLUDE_DIR) -I$(TOP_DIR)/include -o $@ $^

test_rec_ts : test_rec_ts.c
	$(CC) -I$(LSP_INCLUDE_DIR) -I$(TOP_DIR)/include -o $@ $<

test_pvr : test_pvr.c
	$(CC) -I$(LSP_INCLUDE_DIR) -I$(TOP_DIR)/include -o $@ $<

test_rec_pes : test_rec_pes.c tv_function.c
	$(CC) -I$(LSP_INCLUDE_DIR) -I$(TOP_DIR)/include -o $@ $^

.PHONY : clean

clean : 
	rm -f $(OBJS) $(LOCAL_TARGET) $(LOCAL_TEST)
	make -C pvrtest clean


install : test_apps
	@if [ ! -d $(LOCALBIN_DIR) ] ; then \
		echo "Creating target directory : "  $(LOCALBIN_DIR) ; \
		mkdir -p $(LOCALBIN_DIR) ; \
	fi
	install $(LOCAL_TEST) $(LOCALBIN_DIR)
	make -C pvrtest install
