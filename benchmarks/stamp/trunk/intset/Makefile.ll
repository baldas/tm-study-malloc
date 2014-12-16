TMBUILD ?= seq

PROG := intset-ll

SRCS += \
	intset.c

OBJS := ${SRCS:.c=.o}

CFLAGS += -DUSE_LINKEDLIST

include ../common/$(TMBUILD)/Makefile.common


