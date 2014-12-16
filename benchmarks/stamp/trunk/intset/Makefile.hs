TMBUILD ?= seq

PROG := intset-hs

SRCS += \
	intset.c

OBJS := ${SRCS:.c=.o}

CFLAGS += -DUSE_HASHSET

include ../common/$(TMBUILD)/Makefile.common


