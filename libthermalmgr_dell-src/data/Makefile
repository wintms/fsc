#----------------------------------------------------------------------
#                         Makefile to build libclspdkfsc
#                 Copyright (c) 2020 American Megatrends Inc 
#-----------------------------------------------------------------------

#------- Comment/uncomment the following line to enable/disable debugging------
DEBUG = n
#------------------------------------------------------------------------------

#---------------------- Change according to your files ------------------------
LIBRARY_NAME = libthermalmgr_dell
SRC = fsc_loop.c fsc_parser.c fsc_core.c

CFLAGS += -I${SPXINC}/global
CFLAGS += -I${SPXINC}/unix
CFLAGS += -I${SPXINC}/ipmi
CFLAGS += -I${SPXINC}/network
CFLAGS += -I${SPXINC}/hostname
CFLAGS += -I${SPXINC}/featuredefine
CFLAGS += -I${SPXINC}/pdk
CFLAGS += -I${SPXINC}/oempdk
CFLAGS += -I${SPXINC}/dbgout
CFLAGS += -I${SPXINC}/ipmihalapi
CFLAGS += -I${SPXINC}/libipmi
CFLAGS += -I${SPXINC}/safesystem
CFLAGS += -I${SPXINC}/pmbus
CFLAGS += -I${SPXINC}/cJSON
CFLAGS += -I${TARGETDIR}/usr/include 

LIBS   += -L${SPXLIB}/safesystem -lsafesystem
LIBS += -L${SPXLIB}/cJSON -lcJSON
include ${TOOLDIR}/rules/Rules.make.libs
#------------------------------------------------------------------------------
