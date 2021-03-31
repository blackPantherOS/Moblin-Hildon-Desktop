include Makefile.include

CC=gcc
CFLAGS ?= -O2 -g -D_FORTIFY_SOURCE=2 `pkg-config --cflags glib-2.0` -Wall
LDFLAGS ?= -export-dynamic -ld

SUBDIRS = include daemon plugins ppmtool conf

all:
	@set -e; for i in ${SUBDIRS}; do	\
		${MAKE} -C $$i $@; 		\
	done

install: all
	@set -e; for i in ${SUBDIRS}; do	\
		${MAKE} -C $$i $@;		\
	done
	install -d $(DESTDIR)$(ppmdatadir)
	cp -dr layers modes $(DESTDIR)$(ppmdatadir)

uninstall:
	@set -e; for i in ${SUBDIRS}; do	\
		${MAKE} -C $$i $@;		\
	done
	rm -rf $(DESTDIR)$(ppmdatadir)/modes $(DESTDIR)$(ppmdatadir)/layers
	-rmdir -p $(DESTDIR)$(ppmdatadir)

clean:
	@for i in ${SUBDIRS}; do	\
		${MAKE} -C $$i $@;	\
	done

test:
	@for i in ${SUBDIRS}; do	\
		${MAKE} -C $$i $@;	\
	done

.PHONY: all clean install uninstall test
