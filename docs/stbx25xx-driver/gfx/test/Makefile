####################################################################################
#  gfxtest makefile
####################################################################################

##  Uncomment this to enable support of freetype 2 font rendering  in gfxfont.c
#FREETYPE2_SUPPORT := freetype2

## Uncomment this to enable libjpeg support in gfxbmp.c
#JPEG_LIB_READY := -D__JPEG_LIB_READY__


ifdef FREETYPE2_SUPPORT
FREETYPE2_LIB    := $(FREETYPE2_SUPPORT)/lib/libfreetype.a
FREETYPE2_INCLUDE := $(FREETYPE2_SUPPORT)/include
else
FREETYPE2_LIB    := 
FREETYPE2_INCLUDE := 
endif

ifdef JPEG_LIB_READY

JPEG_LIB_LINK := -ljpeg

endif



LOCAL_TARGET	:= gfxlib.a

LOCAL_TEST	:= gfxtester


include ../../config.local

ifdef FREETYPE2_SUPPORT

OBJS := gfxlib.o gfxbmp.o gfxdraw.o gfxfont.o

gfxtest : gfxtest.c $(LOCAL_TARGET)
	$(CC) -I$(DRV_INCLUDE_DIR) -DFREETYPE2_SUPPORT -lm $(JPEG_LIB_LINK) -o $@ $^ $(FREETYPE2_LIB)

af_test : af_test.c $(LOCAL_TARGET)
	$(CC) -I$(DRV_INCLUDE_DIR) -DFREETYPE2_SUPPORT -o $@ $^ $(FREETYPE2_LIB)

else

OBJS := gfxlib.o gfxbmp.o gfxdraw.o
gfxtest : gfxtest.c $(LOCAL_TARGET)
	$(CC) -I$(DRV_INCLUDE_DIR) -lm $(JPEG_LIB_LINK) -o $@ $^

af_test : af_test.c $(LOCAL_TARGET)
	$(CC) -I$(DRV_INCLUDE_DIR) -o $@ $^ 

endif

all: $(LOCAL_TARGET) $(LOCAL_TEST) 

.PHONY : clean

test_apps: $(LOCAL_TEST) 

%.o : %.c
	$(CC) -c -I$(DRV_INCLUDE_DIR) $(JPEG_LIB_READY) -o $@ $<
$(LOCAL_TARGET) : $(OBJS)
	$(AR) rc $@ $^

gfxlib.o : gfxlib.c gfxlib.h

gfxbmp.o : gfxbmp.c gfxlib.h

gfxdraw.o : gfxdraw.c gfxlib.h

gfxfont.o : gfxfont.c gfxlib.h
	$(CC) -c -I$(FREETYPE2_INCLUDE) -I$(DRV_INCLUDE_DIR) -o $@ $<


$(LOCAL_TEST) : gfxtest fbtest af_test


fbtest : fbtest.c
	$(CC) -I$(DRV_INCLUDE_DIR) -o $@ $^ 


clean : 
	rm -f $(OBJS)  $(LOCAL_TARGET) fbtest gfxtest af_test *.o *.a

install : $(LOCAL_TEST)
	@if [ ! -d $(LOCALBIN_DIR) ] ; then \
		echo "Creating target directory : "  $(LOCALBIN_DIR) ; \
		mkdir -p $(LOCALBIN_DIR) ; \
	fi
	install gfxtest $(LOCALBIN_DIR)
	install fbtest $(LOCALBIN_DIR)
	install af_test $(LOCALBIN_DIR)
