AC_INIT(example-plugins, 0.0.1)

AM_INIT_AUTOMAKE

AC_CONFIG_SRCDIR([src/close-app-plugin.h])

AM_CONFIG_HEADER(config.h)

AC_CANONICAL_HOST

AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_RANLIB
AC_PROG_LIBTOOL

AC_C_CONST
AC_HEADER_STDC

AC_HEADER_STDBOOL

#++++++++++++++++++
# Misc build flags
#++++++++++++++++++

CFLAGS="$CFLAGS -Wall -Werror -ansi -Wmissing-prototypes -Wmissing-declarations -std=c99 -rdynamic"

AC_SUBST(CFLAGS)

#+++++++++++++++++++++
# Dependencies checks
#+++++++++++++++++++++

PKG_CHECK_MODULES(GTK, gtk+-2.0 >= 2.6.0)
AC_SUBST(GTK_LIBS)
AC_SUBST(GTK_CFLAGS)

PKG_CHECK_MODULES(GLIB, glib-2.0 >= 2.6.0)
AC_SUBST(GLIB_LIBS)
AC_SUBST(GLIB_CFLAGS)

PKG_CHECK_MODULES(LIBHILDONDESKTOP, libhildondesktop)
AC_SUBST(LIBHILDONDESKTOP_LIBS)
AC_SUBST(LIBHILDONDESKTOP_CFLAGS)

PKG_CHECK_MODULES(LIBHILDONWM, libhildonwm)
AC_SUBST(LIBHILDONWM_LIBS)
AC_SUBST(LIBHILDONWM_CFLAGS)

#+++++++++++++++++++
# Directories setup
#+++++++++++++++++++

marqueeentrydir=${datadir}/applications/hildon-marquee
AC_SUBST(marqueeentrydir)

hildonstatusbarentrydir=${datadir}/applications/hildon-status-bar
AC_SUBST(hildonstatusbarentrydir)

marqueepluginsdir=${libdir}/hildon-desktop
AC_SUBST(marqueepluginsdir)

AC_CONFIG_FILES([
Makefile
src/Makefile
src/drop-down-menu.desktop
src/clock-plugin.desktop
src/close-app.desktop
src/showhome-plugin.desktop
])

AC_OUTPUT
