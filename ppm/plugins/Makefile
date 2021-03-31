include ../Makefile.include

PLUGINS = cpu.so wireless.so display.so sata.so wireless-mac80211.so
PLUGINS_USER = display-user.so
CFLAGS = -MD -I../daemon -I../include -O2 -g -fPIC -Wall -D_FORTIFY_SOURCE=2 `pkg-config --cflags glib-2.0`

all: $(PLUGINS) $(PLUGINS_USER)

-include *.d

display.so: CFLAGS += `pkg-config --cflags dbus-1 dbus-glib-1 hal`
display.so: LDFLAGS += `pkg-config dbus-1 dbus-glib-1 hal`

display-user.so: CFLAGS += -I/usr/include/X11/extensions
display-user.so: LDFLAGS += `pkg-config --libs x11 xext`

%.so: %.o
	gcc -shared $(LDFLAGS) -o $@ $^

clean:
	rm -f *.o *.so *.d

install:
	install -d $(DESTDIR)$(plugindir)
	install $(PLUGINS) $(DESTDIR)$(plugindir)
	install -d $(DESTDIR)$(plugindiruser)
	install $(PLUGINS_USER) $(DESTDIR)$(plugindiruser)

uninstall:
	-cd $(DESTDIR)$(plugindir) && rm -f $(PLUGINS)
	-rmdir -p $(DESTDIR)$(plugindir)
	-cd $(DESTDIR)$(plugindiruser) && rm -f $(PLUGINS_USER)
	-rmdir -p $(DESTDIR)$(plugindiruser)

test:

.PHONY: all clean install uninstall test
