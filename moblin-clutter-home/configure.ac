AC_PREREQ(2.53)
AC_INIT(desktop-launcher, 0.1, [])
AM_INIT_AUTOMAKE()
AC_CONFIG_SRCDIR(src/main.c)
AM_CONFIG_HEADER(config.h)
AM_MAINTAINER_MODE

AC_ISC_POSIX
AC_PROG_CC
AC_STDC_HEADERS
AC_PROG_LIBTOOL

if test "x$GCC" = "xyes"; then
        GCC_FLAGS="-g -Wall -Werror"
fi

AC_SUBST(GCC_FLAGS)


PKG_CHECK_MODULES(DEPS, 
                  clutter-0.4  
                  gtk+-2.0 
                  gconf-2.0 
                  libgnome-menu
                  libstartup-notification-1.0)
AC_SUBST(DEPS_CFLAGS)
AC_SUBST(DEPS_LIBS)

AC_PATH_PROG(GCONFTOOL, gconftool-2)
AM_GCONF_SOURCE_2

AC_OUTPUT([
Makefile
data/Makefile
src/Makefile
])
