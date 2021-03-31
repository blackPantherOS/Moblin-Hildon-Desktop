lib_LTLIBRARIES  = libmokofingerscroll.la

libmokofingerscroll_la_SOURCES =moko-finger-scroll.c

mokoincludedir = $(includedir)/moko
mokoinclude_HEADERS = moko-finger-scroll.h moko-type.h

# set the include path found by configure
INCLUDES= $(all_includes)

AM_CPPFLAGS = $(GTK_CFLAGS) 

# the library search path.
libmokofingerscroll_la_LDFLAGS = $(all_libraries)  $(GTK_LIBS)
