CC = gcc
CFLAGS = -fpic -g -c -W -Wall -I atomic_ops
LDFLAGS = -shared
LIBS = -ldl
OBJS = malloc_profile.o memprofile.o 

TARGET = libmallocprof.so

#all: test

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) -o $@ $(LIBS)

clean:
	rm -f *.so *.o
