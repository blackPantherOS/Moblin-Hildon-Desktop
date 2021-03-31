## Process this file with automake to produce Makefile.in

dbusdir = $(datadir)/dbus-1/services
dbus_DATA = org.moblin.calculator.service 

desktopdir = $(datadir)/applications
desktop_in_files = calculator.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

CLEANFILES = $(desktop_DATA) $(dbus_DATA)

SUBDIRS = src po ui doc

# things we want to include in the source code distribution

EXTRA_DIST = calculator.spec \
	calculator.spec.in \
	calculator.desktop.in \
	org.moblin.calculator.service.in \
	intltool-merge.in \
	intltool-extract.in \
	intltool-update.in \
	ABOUT-NLS

