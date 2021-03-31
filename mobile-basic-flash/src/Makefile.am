INCLUDES = $(GTK_CFLAGS)              \
           $(GLIB_CFLAGS)             \
           $(LIBOSSO_CFLAGS)          \
           $(LIBHILDONDESKTOP_CFLAGS) \
           $(LIBHILDONWM_CFLAGS)      \
           $(LIBWNCK_CFLAGS)          \
           $(GNOME_MENU_CFLAGS)       \
	   $(MOZEMBED_CFLAGS)

LDFLAGS = -module -avoid-version

mobilebasichome_LTLIBRARIES = libmobilebasichome.la

hildonhomeentry_DATA = mobile-basic-home.desktop

libmobilebasichome_la_LIBADD = $(GTK_LIBS) $(GLIB_LIBS) $(LIBOSSO_LIBS) $(LIBHILDONDESKTOP_LIBS) $(LIBHILDONWM_LIBS) $(MOZEMBED_LIBS) $(GNOME_MENU_LIBS) $(LIBWNCK_LIBS)
 
libmobilebasichome_la_SOURCES = mobile-basic-home-plugin.c windowhelper.c

CLEANFILES = *~ *.la *.desktop *.o *.so
