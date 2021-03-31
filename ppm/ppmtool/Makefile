include ../Makefile.include

CFLAGS ?= -MD -O2 -g -D_FORTIFY_SOURCE=2 -DVERSION=0.1 `pkg-config --cflags glib-2.0 dbus-glib-1 gdk-x11-2.0 atk` -Wall -I../include 
LDFLAGS ?= -export-dynamic -ldl $(shell pkg-config --libs gtk+-2.0 glib-2.0 gdk-x11-2.0 atk)
LIBS = dbus-bind.o ppmtool.o ppmtool-interface.o

all: ppmtool

-include *.d

ppmtool: $(LIBS)
	gcc -o ppmtool $(LIBS) `pkg-config --libs glib-2.0 dbus-glib-1` $(LDFLAGS)

install: ppmtool
	install -m755 -D ppmtool $(DESTDIR)$(bindir)/ppmtool

uninstall:
	rm -f $(DESTDIR)$(bindir)/ppmtool
	-rmdir -p $(DESTDIR)$(bindir)

clean:
	rm -f *.o ppmtool *.d

test:

.PHONY: all clean install uninstall test
