TMBUILD ?= seq

PROG := intset-sl

SRCS += \
	intset.c

OBJS := ${SRCS:.c=.o}

CFLAGS += -DUSE_SKIPLIST

include ../common/$(TMBUILD)/Makefile.common


