ACLOCAL_AMFLAGS = -I m4

SUBDIRS= \
	po \
	dependencies \
	icons \
	libgames-support \
	tests \
	$(gamelist)

if HAVE_GSTREAMER
SUBDIRS += sounds
endif

DIST_SUBDIRS = po dependencies icons sounds libgames-support tests $(moblingames)

EXTRA_DIST = \
	autogen.sh \
	gnome-games.spec.in \
	gnome-games.spec \
	COPYING-DOCS \
	xmldocs.make \
	omf.make \
	MAINTAINERS	\
	HACKING		 \
	intltool-extract.in \
	intltool-merge.in    \
	intltool-update.in \
	gnome-doc-utils.make

DISTCLEANFILES = \
	intltool-extract \
	intltool-merge \
	intltool-update \
	gnome-doc-utils.make

# Ignore score files, these *should* be left behind.
distuninstallcheck_listfiles = find . -type f -print | grep -v '.scores' | grep-v scrollkeeper

DISTCHECK_CONFIGURE_FLAGS = \
	--with-platform=gnome \
	--disable-schemas-install \
	--disable-scrollkeeper
