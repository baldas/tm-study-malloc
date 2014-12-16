
CC  := gcc

CCFLAGS  := -O3 -DNDEBUG 
CCFLAGS   += -fno-builtin-malloc -fno-builtin-calloc -fno-builtin-realloc -fno-builtin-free

TARGET = threadtest

$(TARGET):
	$(CC) $(CCFLAGS) threadtest.c -o $(TARGET) -lpthread

clean:
	rm -f $(TARGET)
