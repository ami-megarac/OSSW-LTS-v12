DEBUG  := n
TARGET := postcode
OBJS   := postcodemain.o

EXTRA_CFLAGS += -I${SPXINC}/global
EXTRA_CFLAGS += -I${SPXINC}/helper
EXTRA_CFLAGS += -I${SPXINC}/reset

KBUILD_EXTRA_SYMBOLS += ${SPXINC}/reset/Module.symvers

-include ${TOOLDIR}/rules/Rules.make.drivers

