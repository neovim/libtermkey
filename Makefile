CC?=gcc
CFLAGS?=

CFLAGS_DEBUG=

VERSION_MAJOR=0
VERSION_MINOR=0

SONAME=libtermkey.so.$(VERSION_MAJOR)

PREFIX=/usr/local
LIBDIR=$(PREFIX)/lib
INCDIR=$(PREFIX)/include
MANDIR=$(PREFIX)/share/man
MAN3DIR=$(MANDIR)/man3

MANSOURCE=$(wildcard *.3.sh)
BUILTMAN=$(MANSOURCE:.3.sh=.3)

ifeq ($(DEBUG),1)
  CFLAGS_DEBUG=-ggdb -DDEBUG
endif

all: termkey.h demo demo-async doc

termkey.h: termkey.h.in Makefile
	sed -e 's/@@VERSION_MAJOR@@/$(VERSION_MAJOR)/g' \
	    -e 's/@@VERSION_MINOR@@/$(VERSION_MINOR)/g' \
	    $< >$@

demo: libtermkey.so demo.c
	$(CC) $(CFLAGS) $(CFLAGS_DEBUG) -o $@ $^

demo-async: libtermkey.so demo-async.c
	$(CC) $(CFLAGS) $(CFLAGS_DEBUG) -o $@ $^

libtermkey.so: termkey.o driver-csi.o driver-ti.o
	$(LD) -shared -soname=$(SONAME) -o $@ $^ -lncurses

%.o: %.c termkey.h termkey-internal.h
	$(CC) $(CFLAGS) $(CFLAGS_DEBUG) -Wall -std=c99 -fPIC -o $@ -c $<

doc: $(BUILTMAN)

%.3: %.3.sh
	sh $< >$@

.PHONY: clean
clean:
	rm -f *.o demo $(BUILTMAN) termkey.h

.PHONY: install
install: install-inc install-lib install-man

install-inc:
	install -d $(DESTDIR)$(INCDIR)
	install -m644 termkey.h $(DESTDIR)$(INCDIR)
	install -d $(DESTDIR)$(LIBDIR)/pkgconfig
	sed "s,@PREFIX@,$(PREFIX)," <termkey.pc.in >$(DESTDIR)$(LIBDIR)/pkgconfig/termkey.pc

install-lib:
	install -d $(DESTDIR)$(LIBDIR)
	install libtermkey.so $(DESTDIR)$(LIBDIR)/$(SONAME)
	ln -sf $(SONAME) $(DESTDIR)$(LIBDIR)/libtermkey.so

install-man:
	install -d $(DESTDIR)$(MAN3DIR)
	for F in *.3; do \
	  gzip <$$F >$(DESTDIR)$(MAN3DIR)/$$F.gz; \
	done
	ln -sf termkey_new.3.gz $(DESTDIR)$(MAN3DIR)/termkey_destroy.3.gz
	ln -sf termkey_getkey.3.gz $(DESTDIR)$(MAN3DIR)/termkey_getkey_force.3.gz
	ln -sf termkey_set_waittime.3.gz $(DESTDIR)$(MAN3DIR)/termkey_get_waittime.3.gz
