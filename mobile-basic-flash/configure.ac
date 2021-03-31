AC_INIT(mobile-basic-flash, 0.0.1)

AM_INIT_AUTOMAKE

AC_CONFIG_SRCDIR([src/mobile-basic-home-plugin.h])

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

CFLAGS="$CFLAGS -Wall -ansi -Wmissing-prototypes -Wmissing-declarations -Werror -std=c99 -rdynamic"

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

PKG_CHECK_MODULES(MOZEMBED, xulrunner-gtkmozembed >= 1.8.1)
AC_SUBST(MOZEMBED_LIBS)
AC_SUBST(MOZEMBED_CFLAGS)

PKG_CHECK_MODULES(LIBHILDONDESKTOP, libhildondesktop)
AC_SUBST(LIBHILDONDESKTOP_LIBS)
AC_SUBST(LIBHILDONDESKTOP_CFLAGS)

PKG_CHECK_MODULES(LIBHILDONWM, libhildonwm)
AC_SUBST(LIBHILDONWM_LIBS)
AC_SUBST(LIBHILDONWM_CFLAGS)

PKG_CHECK_MODULES(LIBOSSO, libosso)
AC_SUBST(LIBOSSO_LIBS)
AC_SUBST(LIBOSSO_CFLAGS)

PKG_CHECK_MODULES(GNOME_MENU, libgnome-menu)
AC_SUBST(GNOME_MENU_LIBS)
AC_SUBST(GNOME_MENU_CFLAGS)

PKG_CHECK_MODULES(LIBWNCK, libwnck-1.0, libstartup-notification-1.0)
AC_SUBST(LIBWNCK_LIBS)
AC_SUBST(LIBWNCK_CFLAGS)

hildonhomeentrydir=${datadir}/applications/hildon-home
AC_SUBST(hildonhomeentrydir)

mobilebasichomedir=${libdir}/hildon-desktop
AC_SUBST(mobilebasichomedir)

AC_PATH_PROG(GCONFTOOL, gconftool-2)
AM_GCONF_SOURCE_2

AC_CONFIG_FILES([
Makefile
src/Makefile
src/mobile-basic-home.desktop
content/Makefile
])

AC_OUTPUT
