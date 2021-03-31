bin_PROGRAMS=desktop-launcher

PKGDATADIR = $(datadir)/desktop-launcher
AM_CFLAGS = \
	$(DEPS_CFLAGS) 				\
	$(GCC_FLAGS) 				\
	-I$(top_builddir)			\
	-I$(top_srcdir)				\
	-D_GNU_SOURCE 				\
	-DPKGDATADIR=\"$(PKGDATADIR)\"

desktop_launcher_LDADD  = \
	$(DEPS_LIBS)

desktop_launcher_SOURCES = \
	launcher-animation.c			\
	launcher-animation.h			\
	launcher-animation-linear.c		\
	launcher-animation-linear.h		\
	launcher-background.c			\
	launcher-background.h			\
	launcher-behave.c			\
	launcher-behave.h			\
	launcher-catmap.c			\
	launcher-catmap.h			\
	launcher-item.c				\
	launcher-item.h				\
	launcher-menu.c				\
	launcher-menu.h				\
	launcher-minimap-manager.c		\
	launcher-minimap-manager.h		\
	launcher-minimap.c			\
	launcher-minimap.h			\
	launcher-spinner.c			\
	launcher-spinner.h			\
	launcher-startup.c			\
	launcher-startup.h			\
	launcher-util.c				\
	launcher-util.h 			\
	main.c
