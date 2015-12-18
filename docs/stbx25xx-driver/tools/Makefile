####################################################################################
#  gfx makefile
####################################################################################

LOCAL_TARGET	:= img2header

LOCAL_TEST	:= 

LOCAL_CPP_OPT	:= 


include ../config.local


OBJS := 

COMMONHEADERS = 

all: $(LOCAL_TARGET)

$(LOCAL_TARGET) : img2header.c
	gcc  -o $@ $<


.PHONY : clean

test_apps:

clean : 
	rm -f $(LOCAL_TARGET)


install : 