#----------------------------------------------------------------------
#                         Makefile to build libipmipdk
#                 Copyright (c) 2020 American Megatrends Inc 
#-----------------------------------------------------------------------

#------- Comment/uncomment the following line to enable/disable debugging------
DEBUG = n
#------------------------------------------------------------------------------

#---------------------- Change according to your files ------------------------
LIBRARY_NAME = libipmipdk
SRC = PDKAlert.c PDKEEPROM.c PDKFRU.c PDKSensor.c PDKHooks.c PDKHW.c PDKLED.c PDKSDR.c PDKSEL.c PDKInt.c
SRC += OEMPLD.c OEMDBG.c OEMLED.c OEMFRU.c OEMFAN.c OEMSysInfo.c

CFLAGS += -I${SPXINC}/global
CFLAGS += -I${SPXINC}/ipmi
CFLAGS += -I${SPXINC}/gpio
CFLAGS += -I${SPXINC}/mmap
CFLAGS += -I${SPXINC}/smtp
CFLAGS += -I${SPXINC}/kcs
CFLAGS += -I${SPXINC}/encryption
CFLAGS += -I${SPXINC}/unix
CFLAGS += -I${SPXINC}/pdk
CFLAGS += -I${SPXINC}/oempdk
CFLAGS += -I${SPXINC}/dbgout
CFLAGS += -I${SPXINC}/ipmihalapi
CFLAGS += -I${SPXINC}/featuredefine
CFLAGS += -I${SPXINC}/network
CFLAGS += -I${SPXINC}/hostname
CFLAGS += -I${SPXINC}/flash
CFLAGS += -I${SPXINC}/dynamicdrv
CFLAGS += -I${SPXINC}/UUID_gen
CFLAGS += -I${TARGETDIR}/usr/include 
CFLAGS += -I${SPXINC}/tlv

LIBS   += -L$(SPXLIB)/gpio -lgpio
LIBS   += -L${SPXLIB}/tlv -ltlv

include ${TOOLDIR}/rules/Rules.make.libs
#------------------------------------------------------------------------------
