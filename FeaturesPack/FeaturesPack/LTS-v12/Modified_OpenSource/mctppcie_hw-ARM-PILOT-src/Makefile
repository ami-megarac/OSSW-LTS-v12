#----------------------------------------------------------------------
#                         Makefile to build pcie
#                 Copyright (c) 2014 American Megatrends Inc 
#-----------------------------------------------------------------------

DEBUG     := n
TARGET    := mctppcie_hw
OBJS      := mctppciemain_hw.o 

EXTRA_CFLAGS += -I${SPXINC}/linux/linux

EXTRA_CFLAGS += -I${SPXINC}/global 
EXTRA_CFLAGS += -I${SPXINC}/mctppcie
EXTRA_CFLAGS += -I${SPXINC}/reset
EXTRA_CFLAGS += -I${SPXINC}/helper

KBUILD_EXTRA_SYMBOLS +=${SPXINC}/reset/Module.symvers

-include ${TOOLDIR}/rules/Rules.make.drivers

