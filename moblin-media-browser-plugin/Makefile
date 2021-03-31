# Where is your Xwindows located
X11=/usr/X11R6

#
# For building rpms
root=${DESTDIR}

#
# For installing
prefix=/usr

#
#
RPMDIR=/usr/src/RPM

# Choose compiler
CC=gcc
LD=gcc
BINFORMAT=bin

#Debug or not debug build
BUILD=NODEBUG
#BUILD=DEBUG

any:
	@echo 'Choose target:'
	@echo '   ${MAKE} linux        - for Linux'
	@echo '   ${MAKE} solaris-gcc  - for Solaris with gcc'
	@echo '   ${MAKE} solaris-cc   - for Solaris with cc'
	@echo '   ${MAKE} du           - for Digital/Compac Unix or OSF1'
	@echo '   ${MAKE} freebsd-aout - for FreeBSD with a.out netscape'
	@echo '   ${MAKE} hp-gcc       - for HP-UX with gcc'
	@echo '   ${MAKE} hp-cc        - for HP-UX with cc'
	@echo '   ${MAKE} irix         - for Irix'
	@echo '   ${MAKE} aix          - for AIX >= 4.2'
	@echo '   ${MAKE} freebsd      - for FreeBSD 4.x'
	@echo '   ${MAKE} openbsd      - for OpenBSD'
	@echo '   ${MAKE} xx BUILD=DEBUG  - for xx with debug'
	@echo 'Always run $(MAKE) clean first when switching target type'

irix:
	${MAKE} all CC=gcc XCFLAGS=-fPIC LD=ld XLDFLAGS='-shared -n32'

aix:
	${MAKE} all CC=gcc XCFLAGS=-fPIC LD=ld XLDFLAGS='-G -bexpall'

hp-gcc:
	${MAKE} all CC=gcc XCFLAGS=-fPIC LD=ld XLDFLAGS=-b X11=/usr/include/X11R6

hp-cc:
	${MAKE} all CC=cc XCFLAGS='+DAportable -Ae +z' LD=ld XLDFLAGS='-b +e NP_GetValue +e NP_Initialize +e NP_Shutdown +e NP_GetMIMEDescription' RPM_OPT_FLAGS=+O2  X11=/usr/include/X11R6

linux:
	${MAKE} all CC=gcc XCFLAGS='-fPIC -Wall' LD=gcc XLDFLAGS=-shared

solaris-gcc:
	${MAKE} all CC=gcc XCFLAGS=-fPIC LD=ld XLDFLAGS=-G XLIBS='-lX11 -lsocket -lnsl' X11=/usr/openwin/

solaris-cc:
	${MAKE} all CC=cc XCFLAGS=-Kpic RPM_OPT_FLAGS=-O LD=ld XLDFLAGS=-G  XLIBS='-lX11 -lsocket -lnsl' X11=/usr/openwin/

du:
	${MAKE} all CC=cc LD=ld XLDFLAGS='-expect_unresolved "*" -shared -msym -O3'

freebsd-aout:
	${MAKE} all CC=gcc XCFLAGS='-fPIC -aout' LD=gcc XLDFLAGS='-shared -aout'

freebsd:
	${MAKE} all CC=gcc XCFLAGS=-fPIC LD=gcc XLDFLAGS=-shared X11=/usr/X11R6/

openbsd:
	${MAKE} all CC=gcc XCFLAGS=-fPIC LD=gcc XLDFLAGS=-shared X11=/usr/X11R6/

auto:
	@case "x`uname`" in \
	  xLinux) ${MAKE} linux ;; \
	  xSunOS) ${MAKE} solaris-gcc ;; \
	  xHP-UX) ${MAKE} hp-gcc ;; \
	  xOSF1) ${MAKE} du ;; \
	  xIRIX) ${MAKE} irix ;; \
	  xAIX) ${MAKE} aix ;; \
	  xFreeBSD) ${MAKE} freebsd ;; \
	  xOpenBSD) ${MAKE} openbsd ;; \
	esac

# For other systems, look at the example Makefiles in the
# PluginSDK30b5/examples/UnixTemplate/Unix directory

#
# No user servicable parts beyond this point
# 

VERSION=1.10.1

BASE_FILES=Makefile \
	   mozplugger.7 \
	   README \
	   COPYING \
	   mid-mozpluggerrc

SOURCE_FILES=mozplugger.c \
	     mozplugger-common.c \
	     mozplugger-helper.c \
	     mozplugger.spec \
	     mozplugger-common.c \
	     mozplugger.h 

BIN_FILES=mozplugger.so \
	  mozplugger-helper \
	  mozplugger-controller

DEFINES= -DXP_UNIX -DVERSION=\"$(VERSION)\" -D$(BUILD) #-D__amd64__
INCLUDES= -Inpapi/include -I$(X11)/include
RPM_OPT_FLAGS=-O2
COMMON_CFLAGS=$(RPM_OPT_FLAGS) $(INCLUDES) $(DEFINES)
COMMON_LDFLAGS=

NORM_LDFLAGS=
XCFLAGS=-shared
XLDFLAGS=-shared
XLIBDIR = $(X11)/lib
XLIBS=-lX11 -L$(XLIBDIR)

LDSHARED=$(LD) $(XLDFLAGS)
LDFLAGS=$(COMMON_LDFLAGS) $(NORM_LDFLAGS)
CFLAGS=$(COMMON_CFLAGS) $(XCFLAGS)

all: mozplugger.so mozplugger-helper mozplugger-controller

mozplugger-helper: mozplugger-helper.o mozplugger-common.o
	$(CC) $(LDFLAGS) -o mozplugger-helper mozplugger-helper.o mozplugger-common.o $(XLIBS)

mozplugger-helper.o: mozplugger-helper.c mozplugger.h Makefile
	$(CC) -c $(CFLAGS) -o mozplugger-helper.o mozplugger-helper.c

mozplugger.so: mozplugger.o stub.o mozplugger-common.o Makefile
	$(LDSHARED) $(LDFLAGS) -o mozplugger.so mozplugger.o mozplugger-common.o stub.o

mozplugger-common.o: mozplugger-common.c mozplugger.h Makefile
	$(CC) -c $(CFLAGS) -o mozplugger-common.o mozplugger-common.c

mozplugger.o: mozplugger.c mozplugger.h Makefile
	$(CC) -c $(CFLAGS) -o mozplugger.o mozplugger.c

stub.o: npapi/common/npunix.c
	$(CC) -c $(CFLAGS) -o stub.o npapi/common/npunix.c

mozplugger-controller: mozplugger-controller.o mozplugger-common.o mozplugger-common.c
	$(CC) -o mozplugger-controller mozplugger-controller.o mozplugger-common.o $(LDFLAGS) $(XLIBS)

clean:
	-rm -f *.o npapi/common/*.o
	-rm -f mozplugger-helper mozplugger-controller mozplugger.so

spotless: clean
	-rm *.so *~ core mozplugger
	-rm -rf rpmtmp

localinstall: mozplugger.so mozplugger-helper mozplugger-controller
	-@mkdir -p $$HOME/$(BROWSERDIR)/plugins
	cp mozplugger.so $$HOME/$(BROWSERDIR)/plugins/
	cp mozplugger-helper $$HOME/$(BROWSERDIR)/
	cp mozplugger-controller $$HOME/$(BROWSERDIR)/
	if [ -f $$HOME/$(BROWSERDIR)/mid-mozpluggerrc ]; then mv $$HOME/$(BROWSERDIR)/mid-mozpluggerrc $$HOME/$(BROWSERDIR)/mid-mozpluggerrc.old; fi
	cp mid-mozpluggerrc $$HOME/$(BROWSERDIR)/

localinstall_mozilla:
	make localinstall BROWSERDIR=.mozilla

localinstall_netscape:
	make localinstall BROWSERDIR=.netscape

localinstall_opera:
	make localinstall BROWSERDIR=.opera

install:
	-@mkdir -p $(root)$(prefix)/bin
	-@mkdir -p $(root)$(prefix)/lib/midbrowser/plugins
	-@mkdir -p $(root)$(prefix)/share/man/man7
	-@mkdir -p $(root)$(prefix)/share/applications
	-@mkdir -p $(root)/etc
	cp mozplugger-helper $(root)$(prefix)/bin/
	cp mozplugger-controller $(root)$(prefix)/bin/
	cp mozplugger.so $(root)$(prefix)/lib/midbrowser/plugins/
	cp mid-mozpluggerrc $(root)/etc/
	cp mozplugger.7 $(root)$(prefix)/share/man/man7/
	cp defaults.list $(root)$(prefix)/share/applications/
	cp moblin-media.desktop $(root)$(prefix)/share/applications/
	cp moblinplayer-client $(root)$(prefix)/bin

mozplugger.tar.gz: $(BASE_FILES) $(SOURCE_FILES)
	@( DIR=`pwd`;\
          BASE=`basename $$DIR`;\
	  cd .. ; \
	  if [ "$$BASE" != "mozplugger" ]; then \
	    ln -s "$$BASE" mozplugger ; \
          fi ;\
	  tar cf - `for a in $(BASE_FILES) $(SOURCE_FILES); do echo mozplugger/$$a ; done` | gzip -9 >mozplugger/mozplugger.tar.gz  ;\
          if [ "$$BASE" != "mozplugger" ]; then \
	    rm mozplugger ; \
          fi ;\
	)

export: mozplugger.tar.gz

bin_export: mozplugger-$(BINFORMAT).tar.gz

echo_version:
	@echo $(VERSION)

rpm: $(RPMDIR)/SOURCES/mozplugger.tar.gz ./mozplugger.spec
	rm -rf rpmtmp ||:
	mkdir rpmtmp
	mkdir rpmtmp/usr
	mkdir rpmtmp/etc
	mkdir rpmtmp/opt
	mkdir rpmtmp/usr/local
	rpm -ba --buildroot `pwd`/rpmtmp mozplugger.spec
	cp $(RPMDIR)/SRPMS/mozplugger-1.src.rpm .
	cp $(RPMDIR)/RPMS/*/mozplugger-1.*.rpm .
	rm -rf rpmtmp

$(RPMDIR)/SOURCES/mozplugger.tar.gz: mozplugger.tar.gz
	cp mozplugger.tar.gz $(RPMDIR)/SOURCES/mozplugger.tar.gz

mozplugger-$(BINFORMAT).tar.gz: $(BASE_FILES) $(SOURCE_FILES) $(BIN_FILES)
	@( DIR=`pwd`;\
          BASE=`basename $$DIR`;\
	  cd .. ; \
	  if [ "$$BASE" != "mozplugger" ]; then \
	    ln -s "$$BASE" mozplugger ; \
          fi ;\
	  tar cf - `for a in $(BASE_FILES) $(BIN_FILES); do echo mozplugger/$$a ; done` | gzip -9 >mozplugger/mozplugger-$(BINFORMAT).tar.gz  ;\
          if [ "$$BASE" != "mozplugger" ]; then \
	    rm mozplugger ; \
          fi ;\
	)
