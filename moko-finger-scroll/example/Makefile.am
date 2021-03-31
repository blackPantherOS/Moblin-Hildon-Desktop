bin_PROGRAMS = mokoexample  mokoglade
mokoexample_SOURCES = example.c

# set the include path found by configure
INCLUDES= $(all_includes) 

AM_CPPFLAGS = $(GLADE_CFLAGS)   $(GTK_CFLAGS)  -I../src

# the library search path.
mokoexample_LDFLAGS = $(all_libraries)  ../src/libmokofingerscroll.la

mokoglade_SOURCES = glade.c

# the library search path.
mokoglade_LDFLAGS = $(all_libraries)  ../src/libmokofingerscroll.la $(GLADE_LIBS)  -export-dynamic
