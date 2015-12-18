####################################################################################
# audio makefile
# ---------------------------------------------------------------------------------
# Changelog:
#
# 24Jun03  JFD  Added targets to make restricted microcode (AC3 decode, DTS/LPCM)
# 22Jul03  JFD  Updated coreload names to latest release level
#
####################################################################################

####################################################################################
#  Make sure you are using the correct micro code
####################################################################################
AUDIO_MICRO_BIN = s5amp318.bin
AUDIO_MICRO_DAT = s5amp318.dat
#AUDIO_AC3_BIN = s5advdr15.bin
#AUDIO_AC3_DAT = s5advdr15.dat
#AUDIO_DTS_BIN = s5adts17.bin
#AUDIO_DTS_DAT = s5adts17.dat

AUDIO_MICROCODE := aud_uc.h astb_d.h
ifdef AUDIO_AC3_BIN
AUDIO_MICROCODE += aud_dvd.h advd_d.h
AUD_MODCFLAGS += -DAC3_ENABLE
endif
ifdef AUDIO_DTS_BIN
AUDIO_MICROCODE += aud_dts.h adts_d.h
AUD_MODCFLAGS += -DDTS_ENABLE
endif
 
LOCAL_TARGET	:= aud.o

LOCAL_DEF	:= 

include ../config.local


OBJS := aud_atom.o aud_atom_sync.o aud_osi.o aud_osd.o aud_inf.o aud_atom_mixer.o aud_osi_clip.o \
        aud_osi_mixer.o ../clip/clip.o

COMMONHEADERS = $(DRV_INCLUDE_DIR)/os/os-types.h $(DRV_INCLUDE_DIR)/hw/gpt_ports.h $(DRV_INCLUDE_DIR)/os/drv_debug.h



all: $(LOCAL_TARGET)

aud_atom.o:  aud_atom.c $(AUDIO_MICROCODE)
	$(CC) $(LOCAL_DEF) $(AUD_MODCFLAGS) $(BASE_MODCFLAGS) -c -o $@ $<

aud_uc.h:  $(AUDIO_MICRO_BIN)
	 ../tools/img2header $< $@ -x -d aud_ucode_stb -n aud_ucode_stb_len -s

astb_d.h: $(AUDIO_MICRO_DAT)
	../tools/img2header  $< $@ -x -d astb_d -n astb_d_len -s

aud_dvd.h:  $(AUDIO_AC3_BIN)
	 ../tools/img2header $< $@ -x -d aud_ucode_dvd -n aud_ucode_dvd_len -s

advd_d.h: $(AUDIO_AC3_DAT)
	../tools/img2header  $< $@ -x -d advd_d -n advd_d_len -s

aud_dts.h:  $(AUDIO_DTS_BIN)
	 ../tools/img2header $< $@ -x -d aud_ucode_dts -n aud_ucode_dts_len -s

adts_d.h: $(AUDIO_DTS_DAT)
	../tools/img2header  $< $@ -x -d adts_d -n adts_d_len -s

%.o: %.c
	$(CC) $(LOCAL_DEF) $(BASE_MODCFLAGS) -c -o $@ $<

$(LOCAL_TARGET) : $(OBJS)
	$(LD) -m elf32ppclinux -r -o $@ $(OBJS)
#	$(STRIP) $@

.PHONY : clean

test_apps:

clean : 
	rm -f $(OBJS) $(LOCAL_TARGET) $(AUDIO_MICROCODE)

install : $(LOCAL_TARGET)

