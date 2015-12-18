########################################################################################
#  main make file at the top level
########################################################################################

MODULESDIR = tools os gfx demux aud vid ttx avcore ir sci eeprom rtc_fpc fe_sonysat scp # add your here


all : configs modules test_apps


.BOSS : configs

configs : config.local

config.local : config.make
	@echo "Making local config file ..."
	@make -f config.make

modules :
	@echo "Making modules ..."
	@for i in $(MODULESDIR); do \
	   if ! make -C $$i  ; then\
		exit -1;\
	   fi\
	done

test_apps:
	@echo "Making test applications ..."
	@for i in $(MODULESDIR); do \
	   if  ! make -C $$i test_apps  ; then\
		exit -1;\
	   fi\
	done


install: configs modules test_apps
	@if [ $(shell whoami) != "root" ]; then\
		echo "**************************************************************"; \
		echo "*  Warning: Root is required to install modules properly !   *"; \
		echo "**************************************************************"; \
	fi
	@for i in $(MODULESDIR); do \
	   if !  make -C $$i install ; then\
		exit -1;\
	   fi\
	done

clean :
	@echo "Cleaning targets ..."
	@for i in $(MODULESDIR); do \
	   if ! make -C $$i clean ; then\
		exit -1;\
	   fi\
	done

