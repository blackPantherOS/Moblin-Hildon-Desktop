BINDIR = $(DESTDIR)/usr/bin
SERVICEDIR = $(DESTDIR)/usr/share/dbus-1/services
CONFIGDIR = $(DESTDIR)/usr/share/media-services

all:

install:
	install -m755 -D gst-media-service $(BINDIR)/gst-media-service
	install -m644 -D org.moblin.MobileMediaService.service $(SERVICEDIR)/org.gnome.MobileMediaService.service
	install -m644 -D gst.conf $(CONFIGDIR)/gst.conf
clean:
	rm -f *~
