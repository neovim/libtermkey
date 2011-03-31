LIBTOOL=libtool

CFLAGS?=

CFLAGS_DEBUG=

VERSION_MAJOR=0
VERSION_MINOR=6

VERSION_CURRENT=2
VERSION_REVISION=0
VERSION_AGE=1

PREFIX=/usr/local
LIBDIR=$(PREFIX)/lib
INCDIR=$(PREFIX)/include
MANDIR=$(PREFIX)/share/man
MAN3DIR=$(MANDIR)/man3

ifeq ($(DEBUG),1)
  CFLAGS_DEBUG=-ggdb -DDEBUG
endif

all: termkey.h demo demo-async

OBJECTS=termkey.lo driver-csi.lo driver-ti.lo
LIBRARY=libtermkey.la

%.lo: %.c termkey.h termkey-internal.h
	$(LIBTOOL) --mode=compile --tag=CC gcc $(CFLAGS) $(CFLAGS_DEBUG) -Wall -std=c99 -o $@ -c $<

$(LIBRARY): $(OBJECTS)
	$(LIBTOOL) --mode=link --tag=CC gcc -rpath $(LIBDIR) -version-info $(VERSION_CURRENT):$(VERSION_REVISION):$(VERSION_AGE) -lncurses -o $@ $^

demo: $(LIBRARY) demo.lo
	$(LIBTOOL) --mode=link --tag=CC gcc -o $@ $^

demo-async: $(LIBRARY) demo-async.lo
	$(LIBTOOL) --mode=link --tag=CC gcc -o $@ $^

.PHONY: clean
clean:
	$(LIBTOOL) --mode=clean rm -f $(OBJECTS) demo.lo demo-async.lo
	$(LIBTOOL) --mode=clean rm -f $(LIBRARY)
	$(LIBTOOL) --mode=clean rm -rf demo demo-async

.PHONY: install
install: install-inc install-lib install-man
	$(LIBTOOL) --mode=finish $(DESTDIR)$(LIBDIR)

install-inc:
	install -d $(DESTDIR)$(INCDIR)
	install -m644 termkey.h $(DESTDIR)$(INCDIR)
	install -d $(DESTDIR)$(LIBDIR)/pkgconfig
	sed "s,@PREFIX@,$(PREFIX)," <termkey.pc.in >$(DESTDIR)$(LIBDIR)/pkgconfig/termkey.pc

install-lib:
	install -d $(DESTDIR)$(LIBDIR)
	$(LIBTOOL) --mode=install cp libtermkey.la $(DESTDIR)$(LIBDIR)/libtermkey.la

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

VERSION=$(VERSION_MAJOR).$(VERSION_MINOR)

all: doc

doc: $(BUILTMAN)

%.3: %.3.sh
	sh $< >$@

TESTSOURCES=$(wildcard t/[0-9]*.c)
TESTFILES=$(TESTSOURCES:.c=.t)

t/%.t: t/%.c $(LIBRARY) t/taplib.lo
	$(LIBTOOL) --mode=link --tag=CC gcc -o $@ $^

t/taplib.lo: t/taplib.c
	$(LIBTOOL) --mode=compile --tag=CC gcc $(CFLAGS) -o $@ -c $^

test: $(TESTFILES)
	prove -e ""

clean: clean-built

clean-built:
	rm -f $(BUILTMAN) termkey.h

termkey.h: termkey.h.in Makefile
	sed -e 's/@@VERSION_MAJOR@@/$(VERSION_MAJOR)/g' \
	    -e 's/@@VERSION_MINOR@@/$(VERSION_MINOR)/g' \
	    $< >$@

DISTDIR=libtermkey-$(VERSION_MAJOR).$(VERSION_MINOR)

distdir: all
	mkdir __distdir
	cp *.c *.h *.3 __distdir
	sed "s,@VERSION@,$(VERSION)," <termkey.pc.in >__distdir/termkey.pc.in
	sed "/^# DIST CUT/Q" <Makefile >__distdir/Makefile
	mv __distdir $(DISTDIR)

TARBALL=$(DISTDIR).tar.gz

dist: distdir
	tar -czf $(TARBALL) $(DISTDIR)
	rm -rf $(DISTDIR)
