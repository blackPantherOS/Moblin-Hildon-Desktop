AM_CPPFLAGS =						\
	-I$(top_srcdir)                          	\
	-DPREFIX="\"$(prefix)"\"			\
	-DSYSCONFDIR=\""$(sysconfdir)"\"		\
	-DDATADIR=\""$(datadir)"\"              	\
	-DLIBDIR=\""$(libdir)"\"			\
	-DLOCALEDIR=\""$(datadir)/locale"\"		\
	$(EMPATHY_CFLAGS)				\
	$(WARN_CFLAGS)

if USE_HILDON
AM_CPPFLAGS += $(HILDON_CFLAGS) $(LIBOSSO_CFLAGS)
endif

LDADD =								\
	$(top_builddir)/libempathy-gtk/libempathy-gtk.la	\
	$(top_builddir)/libempathy/libempathy.la		\
	$(EMPATHY_LIBS)

if USE_HILDON
LDADD += $(HILDON_LIBS) $(LIBOSSO_LIBS)
endif

bin_PROGRAMS =			\
	empathy			\
	empathy-accounts

empathy_SOURCES = empathy.c
empathy_accounts_SOURCES = empathy-accounts.c

# Dbus service file
servicedir = $(datadir)/dbus-1/services
if USE_HILDON
service_in_files =                              \
	org.moblin.chat.service.in           \
	org.gnome.Empathy.Chat.service.in
else
service_in_files = org.gnome.Empathy.Chat.service.in
endif
service_DATA = $(service_in_files:.service.in=.service)

# Rule to make the service file with bindir expanded
%.service: %.service.in Makefile
	@sed -e "s|\@bindir\@|$(bindir)|" $< > $@

chandlerdir = $(datadir)/telepathy/managers
chandler_DATA = empathy-chat.chandler

BUILT_SOURCES =							\
	org.gnome.Empathy.Chat.service				\
	org.moblin.chat.service

EXTRA_DIST =							\
	org.gnome.Empathy.Chat.service.in			\
	org.moblin.chat.service.in				\
	$(autostart_DATA)					\
	$(chandler_DATA)

CLEANFILES = $(BUILT_SOURCES)

