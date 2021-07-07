DEBUG := n
TARGET := postcode_hw
OBJS := ast_postcode.o

EXTRA_CFLAGS += -I${SPXINC}/global
EXTRA_CFLAGS += -I${SPXINC}/helper
EXTRA_CFLAGS += -I${SPXINC}/postcode

KBUILD_EXTRA_SYMBOLS += ${SPXINC}/postcode/Module.symvers

-include ${TOOLDIR}/rules/Rules.make.drivers
