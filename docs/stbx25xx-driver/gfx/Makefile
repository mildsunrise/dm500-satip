####################################################################################
#  gfx makefile
####################################################################################

LOCAL_TARGET	:= gfx.o

LOCAL_TEST	:= osdtst.o

LOCAL_CPP_OPT	:= 
#		-D__GFX_INF_HELPER_DEBUG -D__GFX_INF_DEBUG -D__GFX_ATOM_DEBUG \
#		-D__GFX_OSI_DEBUG -D__GFX_SURFACE_DEBUG -D__OSD_ATOM_DEBUG -D__OSD_FB_DEBUG \
#		-D__OSD_OSI_DEBUG


include ../config.local


OBJS := osd_osi.o osd_atom.o gfx_palette.o gfx_osi_surface.o gfx_osi.o gfx_osi_engine.o gfx_inf_helper.o gfx_inf.o gfx_atom.o

COMMONHEADERS = $(DRV_INCLUDE_DIR)/os/os-types.h $(DRV_INCLUDE_DIR)/os/drv_debug.h $(DRV_INCLUDE_DIR)/gfx/gfx_common.h



all: $(LOCAL_TARGET) osdfb.o gfxtest

%.o: %.c
	$(CC) $(BASE_MODCFLAGS) $(LOCAL_CPP_OPT) -c -o $@ $<

$(LOCAL_TARGET) : $(OBJS)
	$(LD) -m elf32ppclinux -r -o $@ $^

osdfb.o : osd_inf_fb.o 
	cp osd_inf_fb.o osdfb.o

osd_inf_fb.o : osd_inf_fb.c \
		$(COMMONHEADERS) \
        	osd_osi.h\
        	gfx_surface.h

osd_osi.o : osd_osi.c gfx_osi_local.h \
        	osd_atom.h\
        	osd_osi.h\
        	gfx_surface.h

osd_atom.o : osd_atom.c osd_dcr.h\
		osd_atom.h

gfx_palette.o	: gfx_palette.c \
                $(COMMONHEADERS)


gfx_osi_surface.o : gfx_osi_surface.c \
		$(COMMONHEADERS) \
		$(DRV_INCLUDE_DIR)/os/pm-alloc.h \
		gfx_surface.h

gfx_osi.o : gfx_osi.c gfx_osi_local.h \
		$(COMMONHEADERS) \
		$(DRV_INCLUDE_DIR)/os/os-sync.h \
		gfx_surface.h \
		gfx_osi.h   \
		gfx_osi_engine.h

gfx_osi_engine.o : gfx_osi_engine.c \
		$(COMMONHEADERS) \
		$(DRV_INCLUDE_DIR)/os/os-sync.h \
		gfx_surface.h \
		gfx_osi.h   \
		gfx_osi_engine.h

gfx_inf_helper.o : gfx_inf_helper.c gfx_inf_helper.h \
		$(COMMONHEADERS) \
		$(DRV_INCLUDE_DIR)/os/os-generic.h \
		gfx_surface.h \
		gfx_osi.h \
		osd_osi.h \
		$(DRV_INCLUDE_DIR)/gfx/gfx_inf_struct.h

gfx_inf.o : gfx_inf.c gfx_inf_helper.h \
		$(COMMONHEADERS) \
		$(DRV_INCLUDE_DIR)/gfx/gfx_inf_struct.h \
		$(DRV_INCLUDE_DIR)/gfx/gfx_inf.h

gfx_atom.o : gfx_atom.c gfx_atom.h


osd_inf_test.o :  osd_inf_test.c

.PHONY : clean

test_apps: $(LOCAL_TEST)
    

$(LOCAL_TEST) : osd_inf_test.o
	cp osd_inf_test.o $(LOCAL_TEST)


gfxtest :
	make -C test

clean : 
	rm -f $(OBJS) $(LOCAL_TARGET) osdfb.o osd_inf_fb.o osd_inf_test.o $(LOCAL_TEST)
	make -C test clean


install : $(LOCAL_TARGET)  $(LOCAL_TEST) osdfb.o  gfxtest
	@if [ ! -d $(INSTALL_ROOT)/lib/modules/$(VER)/misc ] ; then \
		echo "Creating target directory : "  $(INSTALL_ROOT)/lib/modules/$(VER)/misc ; \
		mkdir -p $(INSTALL_ROOT)/lib/modules/$(VER)/misc ; \
	fi
	install $(LOCAL_TARGET) $(INSTALL_ROOT)/lib/modules/$(VER)/misc
	install osdfb.o $(INSTALL_ROOT)/lib/modules/$(VER)/misc
	install $(LOCAL_TEST) $(INSTALL_ROOT)/lib/modules/$(VER)/misc
	make -C test install 

