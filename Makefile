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

ifeq ($(DEBUG),1)
  CFLAGS_DEBUG=-ggdb -DDEBUG
endif

all: termkey.h demo demo-async

demo: libtermkey.so demo.c
	$(CC) $(CFLAGS) $(CFLAGS_DEBUG) -o $@ $^

demo-async: libtermkey.so demo-async.c
	$(CC) $(CFLAGS) $(CFLAGS_DEBUG) -o $@ $^

libtermkey.so: termkey.o driver-csi.o driver-ti.o
	$(LD) -shared -soname=$(SONAME) -o $@ $^ -lncurses

%.o: %.c termkey.h termkey-internal.h
	$(CC) $(CFLAGS) $(CFLAGS_DEBUG) -Wall -std=c99 -fPIC -o $@ -c $<

.PHONY: clean
clean:
	rm -f *.o libtermkey.so demo demo-async

.PHONY: install
install: install-inc install-lib install-man

install-inc:
	install -d $(DESTDIR)$(INCDIR)
	install -m644 termkey.h $(DESTDIR)$(INCDIR)
	install -d $(DESTDIR)$(LIBDIR)/pkgconfig
	sed "s,@PREFIX@,$(PREFIX)," <termkey.pc.in >$(DESTDIR)$(LIBDIR)/pkgconfig/termkey.pc

install-lib:
	install -d $(DESTDIR)$(LIBDIR)
	install libtermkey.so $(DESTDIR)$(LIBDIR)/$(SONAME).$(VERSION_MINOR)
	ln -sf $(SONAME).$(VERSION_MINOR) $(DESTDIR)$(LIBDIR)/$(SONAME)
	ln -sf $(SONAME) $(DESTDIR)$(LIBDIR)/libtermkey.so

install-man:
	install -d $(DESTDIR)$(MAN3DIR)
	for F in *.3; do \
	  gzip <$$F >$(DESTDIR)$(MAN3DIR)/$$F.gz; \
	done
	ln -sf termkey_new.3.gz $(DESTDIR)$(MAN3DIR)/termkey_destroy.3.gz
	ln -sf termkey_getkey.3.gz $(DESTDIR)$(MAN3DIR)/termkey_getkey_force.3.gz
	ln -sf termkey_set_waittime.3.gz $(DESTDIR)$(MAN3DIR)/termkey_get_waittime.3.gz

# DIST CUT

MANSOURCE=$(wildcard *.3.sh)
BUILTMAN=$(MANSOURCE:.3.sh=.3)

all: doc

doc: $(BUILTMAN)

%.3: %.3.sh
	sh $< >$@

clean: clean-built

clean-built:
	rm -f $(BUILTMAN) termkey.h

termkey.h: termkey.h.in Makefile
	sed -e 's/@@VERSION_MAJOR@@/$(VERSION_MAJOR)/g' \
	    -e 's/@@VERSION_MINOR@@/$(VERSION_MINOR)/g' \
	    $< >$@

DISTDIR=libtermkey-$(VERSION_MAJOR).$(VERSION_MINOR)

distdir: all
	mkdir $(DISTDIR)
	cp *.c *.h *.3 $(DISTDIR)
	cp *.pc.in $(DISTDIR)
	sed "/^# DIST CUT/Q" <Makefile >$(DISTDIR)/Makefile

TARBALL=$(DISTDIR).tar.bz2

dist: distdir
	tar -cjf $(TARBALL) $(DISTDIR)
	rm -rf $(DISTDIR)
