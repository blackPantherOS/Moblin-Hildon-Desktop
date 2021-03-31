# $Id: Makefile.am 23667 2006-11-04 16:16:51Z benny $

INCLUDES =								\
	-DG_LOG_DOMAIN=\"moblin-notes\"					\
	-DLOCALEDIR=\""$(datadir)/locale"\"				\
	-DICONDIR=\""$(datadir)/pixmaps"\"				\
	$(PLATFORM_CPPFLAGS)

bin_PROGRAMS = 								\
	notes

notes_SOURCES = 							\
	mousepad.h							\
	main.c								\
	window.h							\
	window.c							\
	menu.h								\
	menu.c								\
	callback.h							\
	callback.c							\
	keyevent.h							\
	keyevent.c							\
	dnd.h								\
	dnd.c								\
	dialog.h							\
	dialog.c							\
	selector.h							\
	selector.c							\
	file.h								\
	file.c								\
	encoding.h							\
	encoding.c							\
	search.h							\
	search.c							\
	font.h								\
	font.c								\
	utils.h								\
	utils.c								\
	undo.h								\
	undo.c								\
	indent.h							\
	indent.c							\
	linenum.h							\
	linenum.c							\
	gtksourceiter.h							\
	gtksourceiter.c 

notes_CFLAGS =								\
	$(GTK_CFLAGS)							\
	$(HILDON_CFLAGS)						\
	$(HILDON_FM_CFLAGS)						\
	$(LIBOSSO_CFLAGS)						\
	$(MOKO_CFLAGS)							\
	$(LIBXFCEGUI4_CFLAGS)						\
	$(PLATFORM_CFLAGS)

notes_LDFLAGS =							        \
	$(PLATFORM_LDFLAGS)

notes_LDADD =								\
	$(GTK_LIBS)							\
	$(HILDON_LIBS)							\
	$(HILDON_FM_LIBS)						\
	$(LIBOSSO_LIBS)							\
	$(MOKO_LIBS)							\
	$(LIBXFCEGUI4_LIBS)

# vi:set ts=8 sw=8 noet ai nocindent:
