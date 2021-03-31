INCLUDES = $(GTK_CFLAGS)              \
           $(GLIB_CFLAGS)             \
           $(LIBHILDONDESKTOP_CFLAGS) \
           $(LIBHILDONWM_CFLAGS)      

LIBS = $(GTK_LIBS)                \
       $(GLIB_LIBS)               \
       $(LIBHILDONDESKTOP_LIBS)   \
       $(LIBHILDONWM_LIBS)

LDFLAGS = -module -avoid-version
COMMON_CONFIG_SOURCE = common-config.c

marqueeplugins_LTLIBRARIES = libcloseapp.la libdropdownmenu.la libclockplugin.la libshowhomeplugin.la 

marqueeentry_DATA = drop-down-menu.desktop close-app.desktop clock-plugin.desktop showhome-plugin.desktop

libdropdownmenu_la_LIBADD = $(LIBS)
libdropdownmenu_la_SOURCES = $(COMMON_CONFIG_SOURCE) \
                             drop-down-menu-plugin.c

libclockplugin_la_LIBADD = $(LIBS) 
libclockplugin_la_SOURCES = $(COMMON_CONFIG_SOURCE) \
                            clock-plugin.c

libcloseapp_la_LIBADD = $(LIBS)
libcloseapp_la_SOURCES = $(COMMON_CONFIG_SOURCE) \
                         close-app-plugin.c

libshowhomeplugin_la_LIBADD = $(LIBS)
libshowhomeplugin_la_SOURCES = $(COMMON_CONFIG_SOURCE) \
                               showhome-plugin.c

# Sample plugin.  To build, add libdummypanel.la to marqueeplugins_LTLIBRARIES above
# and uncomment the following two lines
#libdummypanel_la_LIBADD = $(GTK_LIBS) $(GLIB_LIBS) $(LIBHILDONDESKTOP_LIBS) $(LIBHILDONWM_LIBS) 
#libdummypanel_la_SOURCES = dummy-panel-plugin.c
#hildonstatusbarentry_DATA = dummy-panel.desktop

CLEANFILES = *~ *.la *.desktop
