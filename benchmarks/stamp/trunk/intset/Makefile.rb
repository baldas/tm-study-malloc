TMBUILD ?= seq

PROG := intset-rb

SRCS += \
	intset.c

OBJS := ${SRCS:.c=.o}

CFLAGS += -DUSE_RBTREE

include ../common/$(TMBUILD)/Makefile.common


