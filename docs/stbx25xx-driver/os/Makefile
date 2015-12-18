####################################################################################
#  os_core makefile
####################################################################################

include ../config.local


OBJS := os_inf.o helper-queue.o helper-pool.o os-interrupt.o  os-sync.o  pm-alloc.o os_alloc.o ../gpt/gpt_atom.o

COMMONHEADERS = $(DRV_INCLUDE_DIR)/os/os-types.h $(DRV_INCLUDE_DIR)/os/drv_debug.h

LOCAL_TARGET	:= os_core.o


all: $(LOCAL_TARGET)

%.o: %.c
	$(CC) $(BASE_MODCFLAGS) -c -o $@ $<

$(LOCAL_TARGET) : $(OBJS)
	$(LD) -m elf32ppclinux -r -o $@ $(OBJS)
#	$(STRIP) $@

helper-queue.o : helper-queue.c helper-queue-local.h \
		$(COMMONHEADERS) \
        	$(DRV_INCLUDE_DIR)/os/helper-queue.h \
        	$(DRV_INCLUDE_DIR)/os/os-generic.h 

helper-pool.o : helper-pool.c helper-pool-local.h \
		$(COMMONHEADERS) \
        	$(DRV_INCLUDE_DIR)/os/helper-pool.h \
        	$(DRV_INCLUDE_DIR)/os/os-generic.h 


os-interrupt.o : os-interrupt.c os-interrupt-local.h \
		$(COMMONHEADERS) \
		$(DRV_INCLUDE_DIR)/os/os-interrupt.h 

os-sync.o : os-sync.c \
		$(COMMONHEADERS) \
		$(DRV_INCLUDE_DIR)/os/os-sync.h 

pm-alloc.o	: pm-alloc.c pm-alloc-local.h \
                $(COMMONHEADERS) \
                $(DRV_INCLUDE_DIR)/os/pm-alloc.h \
		$(DRV_INCLUDE_DIR)/hw/physical-mem.h

os_alloc.o	: os_alloc.c \
                $(COMMONHEADERS) \
                $(DRV_INCLUDE_DIR)/os/pm-alloc.h \
		$(DRV_INCLUDE_DIR)/hw/physical-mem.h

gpt_atom.o : ../gpt/gpt_atom.c \
		$(COMMONHEADERS) \
		$(DRV_INCLUDE_DIR)/os/os-io.h \
		$(DRV_INCLUDE_DIR)/gpt/gpt_ports.h

.PHONY : clean

clean : 
	rm -f $(OBJS) $(LOCAL_TARGET) 

test_apps:

install : $(LOCAL_TARGET)
	@if [ ! -d $(INSTALL_ROOT)/lib/modules/$(VER)/misc ] ; then \
		echo "Creating target directory : "  $(INSTALL_ROOT)/lib/modules/$(VER)/misc ; \
		mkdir -p $(INSTALL_ROOT)/lib/modules/$(VER)/misc ; \
	fi
	install $(LOCAL_TARGET) $(INSTALL_ROOT)/lib/modules/$(VER)/misc
