##################################################################################
# config.make
#   Configuration file for building IBM STB Linux drivers
##################################################################################

#########################################################################################
# Basic Configs
#########################################################################################

#INSTALL_ROOT    = $(TOP_DIR)/../target

# if you have your own target location, please modify the above
# else if you are using a default CDK installation, you may prefer this
INSTALL_ROOT    = /opt/hardhat/devkit/ppc/405/target


CROSS_COMPILE   = /opt/hardhat/devkit/ppc/405/bin/ppc_405-

LSP_DIR         = /opt/hardhat/devkit/lsp/ibm-redwood6-ppc_405/linux-2.4.18_mvl30
#LSP_DIR         = /opt/hardhat/devkit/lsp/ibm-redwood6-ppc_405/linux-2.4.17_mvl21
#LSP_DIR         = /opt/hardhat/devkit/lsp/ibm-redwood6/linux-2.4.2_hhl20

BASE_MODCFLAGS  = -O2 -Wall -I. -I$(LSP_INCLUDE_DIR) -I$(DRV_INCLUDE_DIR) -DMODULE -D__KERNEL__ -DLINUX \
		-I$(LSP_DIR)/arch/ppc \
		$(EXTRA_CFLAGS)



##################################################################################
# Directories
##################################################################################

TOP_DIR  	= $(shell pwd)

LOCALBIN_DIR	= $(INSTALL_ROOT)/usr/local/bin

LSP_INCLUDE_DIR = $(LSP_DIR)/include

DRV_INCLUDE_DIR	= $(TOP_DIR)/include

EXPORT_DIR	= $(TOP_DIR)/export


##################################################################################
# CDK Configs
##################################################################################


AS      =$(CROSS_COMPILE)as
LD      =$(CROSS_COMPILE)ld
CC      =$(CROSS_COMPILE)gcc
CPP     =$(CC) -E
AR      =$(CROSS_COMPILE)ar
NM      =$(CROSS_COMPILE)nm
STRIP   =$(CROSS_COMPILE)strip
OBJDUMP =$(CROSS_COMPILE)objdump


VER = $(shell awk  -F\" '/REL/  {print $$2}' $(LSP_INCLUDE_DIR)/linux/version.h)


#########################################################################################
# Config Exports
#########################################################################################

CONFIG_DEFS = INSTALL_ROOT CROSS_COMPILE LSP_DIR \
	TOP_DIR EXPORT_DIR LOCALBIN_DIR LSP_INCLUDE_DIR DRV_INCLUDE_DIR \
	AS LD CC CPP AR NM STRIP OBJDUMP \
	BASE_MODCFLAGS VER

#########################################################################################
# Config target
#########################################################################################

cfgs = $(foreach abc, $(CONFIG_DEFS),"\\n"$(abc) = $($(abc))"\\n")
all : 
	@echo -n "Creating config.local ..."
	@echo "##############################################################################" > config.local
	@echo "# Automatically generated configuration file, please don't edit" >> config.local
	@echo "##############################################################################" >> config.local
	@echo -e $(cfgs) >> config.local    # creat the file contents
	@echo " Done" 


