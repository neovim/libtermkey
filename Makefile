CC?=gcc
CFLAGS?=

CFLAGS_DEBUG=

SONAME=libtermkey.so.0

PREFIX=/usr/local
LIBDIR=$(PREFIX)/lib
INCDIR=$(PREFIX)/include

ifeq ($(DEBUG),1)
  CFLAGS_DEBUG=-ggdb -DDEBUG
endif

all: demo

demo: libtermkey.so demo.c
	$(CC) $(CFLAGS) $(CFLAGS_DEBUG) -o $@ $^

libtermkey.so: termkey.o driver-csi.o driver-ti.o
	$(LD) -shared -soname=$(SONAME) -o $@ $^ -lncurses

%.o: %.c termkey.h termkey-internal.h
	$(CC) $(CFLAGS) $(CFLAGS_DEBUG) -Wall -std=c99 -fPIC -o $@ -c $<

.PHONY: clean
clean:
	rm -f *.o demo

.PHONY: install
install:
	install -d $(DESTDIR)$(INCDIR)
	install -m644 termkey.h $(DESTDIR)$(INCDIR)
	install -d $(DESTDIR)$(LIBDIR)
	install libtermkey.so $(DESTDIR)$(LIBDIR)/$(SONAME)
	ln -sf $(SONAME) $(DESTDIR)$(LIBDIR)/libtermkey.so
	install -d $(DESTDIR)$(LIBDIR)/pkgconfig
	sed "s,@PREFIX@,$(PREFIX)," <termkey.pc.in >$(DESTDIR)$(LIBDIR)/pkgconfig/termkey.pc
