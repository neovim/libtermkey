CC?=gcc
CFLAGS?=

CFLAGS_DEBUG=

ifeq ($(DEBUG),1)
  CFLAGS_DEBUG=-ggdb -DDEBUG
endif

all: demo

demo: termkey.o driver-csi.o driver-ti.o demo.c
	$(CC) $(CFLAGS) $(CFLAGS_DEBUG) -o $@ $^ -lncurses

%.o: %.c
	$(CC) $(CFLAGS) $(CFLAGS_DEBUG) -Wall -std=c99 -o $@ -c $^

.PHONY: clean
clean:
	rm -f *.o demo
