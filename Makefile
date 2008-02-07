CCFLAGS=-Wall -Iinclude -std=c99
LDFLAGS=

CCFLAGS+=$(shell pkg-config --cflags glib-2.0)
LDFLAGS+=$(shell pkg-config --libs   glib-2.0)

all: demo

demo: termkey.o demo.c
	gcc $(CCFLAGS) $(LDFLAGS) -o $@ $^

termkey.o: termkey.c
	gcc $(CCFLAGS) -o $@ -c $^

.PHONY: clean
clean:
	rm -f *.o demo
