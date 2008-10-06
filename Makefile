CCFLAGS=-Wall -Iinclude -std=c99
LDFLAGS=

ifeq ($(DEBUG),1)
  CCFLAGS+=-ggdb -DDEBUG
endif

all: demo

demo: termkey.o demo.c
	gcc $(CCFLAGS) $(LDFLAGS) -o $@ $^

termkey.o: termkey.c
	gcc $(CCFLAGS) -o $@ -c $^

.PHONY: clean
clean:
	rm -f *.o demo
