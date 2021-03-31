SUBDIRS = pixmaps

NULL =

bin_PROGRAMS = moblinsays 

moblinsays_SOURCES = \
	main.c \
	$(NULL)

moblinsaysmaindir = $(pkgdatadir)/moblinsays
moblinsaysmain_DATA = customgtkrc

moblinsays_CPPFLAGS = \
	-I$(top_srcdir)				\
	-DDATADIR="\"$(datadir)\""		\
	-DICONDIR="\"$(datadir)/icons/hicolor\""\
	-DPKGDATADIR="\"$(pkgdatadir)\""	\
	-DPROGDIR="\"$(pkgdatadir)/moblinsays\""      	\
	-DPIXMAPDIR="\"$(pkgdatadir)/moblinsays/pixmaps\""	\
	$(AM_CPPFLAGS)

moblinsays_CFLAGS = \
	$(GTK_CFLAGS)	\
	$(HILDON_CFLAGS) \
	$(AM_CFLAGS)

moblinsays_LDFLAGS = \
	$(AM_LDFLAGS)

moblinsays_LDADD = \
	$(HILDON_LIBS) \
	$(GTK_LIBS)	

desktop_in_files = \
	moblinsays.desktop \
	moblinsaysmobile.desktop 

if HAVE_MAEMO
desktopdir = $(shell $(PKG_CONFIG) osso-af-settings --variable=desktopentrydir)
else
desktopdir = $(datadir)/applications
if HAVE_HILDON
mobiledesktopdir = /usr/share/mobile-basic-flash/applications
mobiledesktop_DATA = moblinsaysmobile.desktop
endif
endif

desktop_DATA = moblinsays.desktop
ossoservice_in_files = org.moblin.moblinsays.service.in

if HAVE_HILDON
servicedir = $(datadir)/dbus-1/services
service_DATA = $(ossoservice_in_files:.service.in=.service)
endif

if HAVE_MAEMO
servicedir = $(shell $(PKG_CONFIG) osso-af-settings --variable=dbusservicedir)
service_DATA = $(ossoservice_in_files:.service.in=.service)
endif

EXTRA_DIST = \
	AUTHORS			\
	TODO			\
	customgtkrc             \
	$(icon_DATA)		\
	$(allicons)		\
	org.moblin.moblinsays.service \
	$(ossoservice_in_files)	\
	$(desktop_in_files)    

CLEANFILES = \
	$(desktop_DATA)	\
	$(service_DATA)

DISTCLEANFILES = \
	$(desktop_DATA)	\
	$(service_DATA)

