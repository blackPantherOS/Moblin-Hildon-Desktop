# $Id: Makefile.am 23667 2006-11-04 16:16:51Z benny $

SUBDIRS =								\
	po								\
	src

pixmapsdir = $(datadir)/pixmaps
pixmaps_DATA =								\
	moblin-notes.png

desktopdir = $(datadir)/applications
desktop_in_files = notes.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

AUTOMAKE_OPTIONS =							\
	1.8								\
	dist-bzip2

EXTRA_DIST =								\
	intltool-extract.in						\
	intltool-merge.in						\
	intltool-update.in						\
	$(desktop_in_files)						\
	$(pixmaps_DATA)

DISTCLEANFILES =							\
	intltool-extract						\
	intltool-merge							\
	intltool-update							\
	$(desktop_DATA)

# vi:set ts=8 sw=8 noet ai nocindent syntax=automake:
