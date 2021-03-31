include ../Makefile.include

VERSION = 0.1
CFLAGS ?= -MD -O2 -D_FORTIFY_SOURCE=2 $(shell pkg-config --cflags glib-2.0 dbus-1 dbus-glib-1 hal) -I. -I../include -Wall -DVERSION
LDFLAGS ?= -export-dynamic -ldl -lpthread $(shell pkg-config --libs glib-2.0 dbus-1 dbus-glib-1 hal)
LIBS = hal.o dbus-bind.o plugins.o policy.o ppmd.o
TEST_LIBS = plugins.o policy.o ppmtest.o
USER_LIBS = plugins.o ppmd-user.o

all: ppmd ppmd-user

-include *.d

ppmd: config.h $(LIBS)
	gcc -o ppmd $(LIBS) $(CFLAGS) $(LDFLAGS)

ppmd-user: config.h $(USER_LIBS)
	gcc -o ppmd-user $(USER_LIBS) $(CFLAGS) $(LDFLAGS)

ppmtest: config.h $(TEST_LIBS)
	gcc -g -o ppmtest $(TEST_LIBS) $(CFLAGS) $(LDFLAGS)

config.h: Makefile ../Makefile.include
	echo '#define POLICY_PATH "$(ppmdatadir)/layers"' > config.h
	echo '#define MODES_PATH "$(ppmdatadir)/modes"' >> config.h
	echo '#define PLUGIN_PATH "$(plugindir)"' >> config.h
	echo '#define PLUGIN_PATH_USER "$(plugindiruser)"' >> config.h
	echo '#define PPMD_LOGFILE "$(logdir)/ppmd"' >> config.h
	echo '#define PPMD_LOGFILE_USER "$(logdir)/ppmd-user"' >> config.h
	echo '#define TEST_POLICY_PATH "$(shell pwd)/../layers"' >> config.h
	echo '#define TEST_MODES_PATH "$(shell pwd)/../modes"' >> config.h

install: ppmd
	install -m755 -D ppmd $(DESTDIR)$(sbindir)/ppmd
	install -m755 -D ppmd-user $(DESTDIR)$(sbindir)/ppmd-user

test: ppmtest

uninstall:
	rm -f $(DESTDIR)$(sbindir)/ppmd
	rm -f $(DESTDIR)$(sbindir)/ppmd-user
	-rmdir -p $(DESTDIR)$(sbindir)

clean:
	rm -f *.o ppmd ppmd-user ppmtest config.h *.d

.PHONY: all clean install uninstall test
