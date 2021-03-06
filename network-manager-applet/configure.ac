AC_PREREQ(2.52)

AC_INIT(nm-applet, 0.6.5, dcbw@redhat.com, nm-applet)
AM_INIT_AUTOMAKE([subdir-objects])
AM_MAINTAINER_MODE

AM_CONFIG_HEADER(config.h)

dnl
dnl Require programs
dnl
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_INSTALL
AC_PROG_LIBTOOL

dnl
dnl This doesn't depend on gnome support, only on gnome-common, autogen already
dnl uses gnome-autogen
dnl
GNOME_COMMON_INIT
GNOME_DEBUG_CHECK
GNOME_COMPILE_WARNINGS([maximum])
GNOME_MAINTAINER_MODE_DEFINES

dnl
dnl Required headers
dnl
AC_HEADER_STDC
AC_CHECK_HEADERS(fcntl.h paths.h sys/ioctl.h sys/time.h syslog.h unistd.h)

dnl
dnl Checks for typedefs, structures, and compiler characteristics.
dnl
AC_TYPE_MODE_T
AC_TYPE_PID_T
AC_HEADER_TIME

dnl
dnl Checks for library functions.
dnl
AC_PROG_GCC_TRADITIONAL
AC_FUNC_MEMCMP
AC_CHECK_FUNCS(select socket uname)

dnl
dnl translation support
dnl
GETTEXT_PACKAGE=nm-applet
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", [Gettext package])
ALL_LINGUAS="ar bg bs ca cs da de dz el en_CA en_GB es fi fr gl gu hr hu it ja ko lt pl mk nb ne nl pa pt pt_BR ru rw sk sl sq sr@Latn sr sv tr uk vi wa zh_CN zh_HK zh_TW"
AC_PROG_INTLTOOL([0.27.2])
AM_GLIB_GNU_GETTEXT

dnl
dnl Make sha1.c happy on big endian systems
dnl
AC_C_BIGENDIAN

AC_CHECK_LIB(nm-util, ieee_802_11_cipher_validate)

PKG_CHECK_MODULES(DBUS, dbus-glib-1 >= 0.60)

##### Find out the version of DBUS we're using
dbus_version=`pkg-config --modversion dbus-1`
DBUS_VERSION_MAJOR=`echo $dbus_version | awk -F. '{print $1}'`
DBUS_VERSION_MINOR=`echo $dbus_version | awk -F. '{print $2}'`
DBUS_VERSION_MICRO=`echo $dbus_version | awk -F. '{print $3}'`
if test "z$DBUS_VERSION_MAJOR" = "z"; then
	DBUS_VERSION_MAJOR="0"
fi
if test "z$DBUS_VERSION_MINOR" = "z"; then
	DBUS_VERSION_MINOR="0"
fi
if test "z$DBUS_VERSION_MICRO" = "z"; then
	DBUS_VERSION_MICRO="0"
fi

if test "z$DBUS_VERSION_MAJOR" = "z0" -a "z$DBUS_VERSION_MINOR" = "z0" -a "z$DBUS_VERSION_MICRO" = "z0"; then
	echo "Error: Couldn't determine the version of your DBUS package."
	echo "  This is probably an error in this script, please report it"
	echo "  along with the following information:"
	echo "      Base DBUS version ='$dbus_version'"
	echo "      DBUS_VERSION_MAJOR='$DBUS_VERSION_MAJOR'"
	echo "      DBUS_VERSION_MINOR='$DBUS_VERSION_MINOR'"
	echo "      DBUS_VERSION_MICRO='$DBUS_VERSION_MICRO'"
	exit 1
else

	echo "Your dbus version is $DBUS_VERSION_MAJOR,$DBUS_VERSION_MINOR,$DBUS_VERSION_MICRO."
	DBUS_CFLAGS="$DBUS_CFLAGS -DDBUS_VERSION_MAJOR=$DBUS_VERSION_MAJOR"
	DBUS_CFLAGS="$DBUS_CFLAGS -DDBUS_VERSION_MINOR=$DBUS_VERSION_MINOR"
	DBUS_CFLAGS="$DBUS_CFLAGS -DDBUS_VERSION_MICRO=$DBUS_VERSION_MICRO"
fi
AC_SUBST(DBUS_CFLAGS)
AC_SUBST(DBUS_LIBS)

PKG_CHECK_MODULES(GTHREAD, gthread-2.0)
AC_SUBST(GTHREAD_CFLAGS)
AC_SUBST(GTHREAD_LIBS)

PKG_CHECK_MODULES(GLIB, glib-2.0 >= 2.10)
AC_SUBST(GLIB_CFLAGS)
AC_SUBST(GLIB_LIBS)

PKG_CHECK_MODULES(GMODULE, gmodule-2.0)
AC_SUBST(GMODULE_CFLAGS)
AC_SUBST(GMODULE_LIBS)

PKG_CHECK_MODULES(GOBJECT, gobject-2.0)
AC_SUBST(GOBJECT_CFLAGS)
AC_SUBST(GOBJECT_LIBS)

PKG_CHECK_MODULES(NM, NetworkManager)
AC_SUBST(NM_CFLAGS)
AC_SUBST(NM_LIBS)

AC_ARG_WITH(notify, AC_HELP_STRING([--with-notify], [Use libnotify]), enable_notify=$withval, enable_notify=auto)

PKG_CHECK_MODULES(GTK, gtk+-2.0)
AC_SUBST(GTK_CFLAGS)
AC_SUBST(GTK_LIBS)

PKG_CHECK_MODULES(GDK_PIXBUF, gdk-pixbuf-2.0)
AC_SUBST(GDK_PIXBUF_CFLAGS)
AC_SUBST(GDK_PIXBUF_LIBS)

PKG_CHECK_MODULES(GLADE, libglade-2.0)
AC_SUBST(GLADE_CFLAGS)
AC_SUBST(GLADE_LIBS)

PKG_CHECK_MODULES(GCONF, gconf-2.0)
AC_SUBST(GCONF_CFLAGS)
AC_SUBST(GCONF_LIBS)

PKG_CHECK_MODULES(GNOME_KEYRING, gnome-keyring-1)
AC_SUBST(GNOME_KEYRING_CFLAGS)
AC_SUBST(GNOME_KEYRING_LIBS)

LIBNOTIFY_REQUIRED=0.3.0
AC_ARG_WITH(notify, AC_HELP_STRING([--with-notify], [Use libnotify]), enable_notify=$withval, enable_notify=auto)

if test x"$enable_notify" != xno; then
	PKG_CHECK_MODULES([NOTIFY], [libnotify >= $LIBNOTIFY_REQUIRED], [enable_notify=yes],
				[enable_notify=no])
	if test "x$enable_notify" != "xno"; then
		AC_DEFINE_UNQUOTED([ENABLE_NOTIFY], [1],
		[Enable notifications with libnotify])
	fi
fi
AM_CONDITIONAL(WITH_NOTIFY, test x"$enable_notify" != xno) # can't do it from inside 'if'

PKG_CHECK_MODULES(LIBGNOMEUI, libgnomeui-2.0)
AC_SUBST(LIBGNOMEUI_CFLAGS)
AC_SUBST(LIBGNOMEUI_LIBS)

PKG_CHECK_EXISTS([libgnome-2.0 >= 2.14.0],[have_libgnome_2_14=yes],[have_libgnome_2_14=no])
if test "$have_libgnome_2_14" = "yes"; then
	AC_DEFINE([HAVE_LIBGNOME_2_14],[1],[Define if we have libgnome 2.14])
fi

AC_MSG_CHECKING([for wireless-tools >= 28pre9])
AC_TRY_COMPILE([#include <iwlib.h>],
               [#ifndef IWEVGENIE
                #error "not found"
                #endif],
               [ac_have_iwevgenie=yes],
               [ac_have_iwevgenie=no])
AC_MSG_RESULT($ac_have_iwevgenie)
if test "$ac_have_iwevgenie" = no; then
	AC_MSG_ERROR(wireless-tools >= 28pre9 not installed or not functional)
fi
IWLIB=-liw
AC_SUBST(IWLIB)

AC_ARG_WITH(dbus-sys, AC_HELP_STRING([--with-dbus-sys=DIR], [where D-BUS system.d directory is]))

if ! test -z "$with_dbus_sys" ; then
    DBUS_SYS_DIR="$with_dbus_sys"
else
    DBUS_SYS_DIR="${sysconfdir}/dbus-1/system.d"
fi
AC_SUBST(DBUS_SYS_DIR)
AC_DEFINE_UNQUOTED(DBUS_SYSTEMD_DIR, "$DBUS_SYS_DIR", [Where system.d dir for DBUS is])

#### find the actual value for $prefix that we'll end up with
##   (I know this is broken and should be done in the Makefile, but
##    that's a major pain and almost nobody actually seems to care)
REAL_PREFIX=
if test "x$prefix" = "xNONE"; then
  REAL_PREFIX=$ac_default_prefix
else
  REAL_PREFIX=$prefix
fi

## temporarily change prefix and exec_prefix
old_prefix=$prefix
prefix=$REAL_PREFIX

if test "x$exec_prefix" = xNONE ; then
   REAL_EXEC_PREFIX=$REAL_PREFIX
else
   REAL_EXEC_PREFIX=$exec_prefix
fi
old_exec_prefix=$exec_prefix
exec_prefix=$REAL_EXEC_PREFIX

BINDIR_TMP="$bindir"
EXPANDED_BINDIR=`eval echo $BINDIR_TMP`
AC_SUBST(EXPANDED_BINDIR)
AC_DEFINE_UNQUOTED(EXPANDED_BINDIR, "$EXPANDED_BINDIR", [Where the executables are installed])

## put prefix and exec_prefix back
prefix=$old_prefix
exec_prefix=$old_exec_prefix

AC_ARG_ENABLE(more-warnings,
AC_HELP_STRING([--enable-more-warnings], [Maximum compiler warnings]),
set_more_warnings="$enableval",[
if test -d "$srcdir/{arch}" || test -d "$srcdir/CVS"; then
	set_more_warnings=yes
else
	set_more_warnings=no
fi
])
AC_MSG_CHECKING(for more warnings, including -Werror)
if test "$GCC" = "yes" -a "$set_more_warnings" != "no"; then
	AC_MSG_RESULT(yes)
	CFLAGS="-Wall -Werror -std=gnu89 $CFLAGS"

	for option in -Wshadow -Wmissing-declarations -Wmissing-prototypes \
		      -Wdeclaration-after-statement -Wstrict-prototypes \
		      -Wfloat-equal -Wno-unused-parameter -Wno-sign-compare; do
		SAVE_CFLAGS="$CFLAGS"
		CFLAGS="$CFLAGS $option"
		AC_MSG_CHECKING([whether gcc understands $option])
		AC_TRY_COMPILE([], [],
			has_option=yes,
			has_option=no,)
		if test $has_option = no; then
			CFLAGS="$SAVE_CFLAGS"
		fi
		AC_MSG_RESULT($has_option)
		unset has_option
		unset SAVE_CFLAGS
	done
	unset option
else
	AC_MSG_RESULT(no)
fi

AC_OUTPUT([
Makefile
src/Makefile
icons/Makefile
po/Makefile.in
])
