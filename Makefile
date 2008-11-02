CC?=gcc
CFLAGS?=

CFLAGS_DEBUG=

SONAME=libtermkey.so.0

ifeq ($(DEBUG),1)
  CFLAGS_DEBUG=-ggdb -DDEBUG
endif

all: demo

demo: libtermkey.so demo.c
	$(CC) $(CFLAGS) $(CFLAGS_DEBUG) -o $@ $^

libtermkey.so: termkey.o driver-csi.o driver-ti.o
	$(LD) -shared -soname=$(SONAME) -o $@ $^ -lncurses

%.o: %.c
	$(CC) $(CFLAGS) $(CFLAGS_DEBUG) -Wall -std=c99 -o $@ -c $^

.PHONY: clean
clean:
	rm -f *.o demo
