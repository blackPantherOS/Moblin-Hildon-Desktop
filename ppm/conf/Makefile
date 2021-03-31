include ../Makefile.include

all:

ppmd.service: Makefile ../Makefile.include
	echo '[D-BUS Service]' > ppmd.service
	echo 'Name=com.intel.ppmd' >> ppmd.service
	echo 'Exec=$(DESTDIR)$(sbindir)/ppmd' >> ppmd.service

install: ppmd.service
	install -m644 -D ppmd.service $(DESTDIR)$(dbusdatadir)/services/ppmd.service
	install -m644 -D ppmd.conf $(DESTDIR)$(dbusconfdir)/system.d/ppmd.conf

uninstall:
	rm -f $(DESTDIR)$(dbusdatadir)/services/ppmd.service
	rm -f $(DESTDIR)$(dbusconfdir)/system.d/ppmd.conf

clean:
	rm -f ppmd.service

test:

.PHONY: all clean install uninstall test
