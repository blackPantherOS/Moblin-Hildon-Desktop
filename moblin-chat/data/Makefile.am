SUBDIRS = 16x16 22x22 24x24 32x32 48x48 scalable

profiledir = $(datadir)/mission-control/profiles
#profile_DATA = 				\
#	jabber.profile			\
#	gtalk.profile			\
#	salut.profile			\
#	irc.profile			\
#	msn.profile
profile_DATA = gtalk.profile

gtk_update_icon_cache = gtk-update-icon-cache -f -t $(datadir)/icons/hicolor

install-data-hook: update-icon-cache
uninstall-hook: update-icon-cache
update-icon-cache:
	@-if test -z "$(DESTDIR)"; then \
		echo "Updating Gtk icon cache."; \
		$(gtk_update_icon_cache); \
	else \
		echo "*** Icon cache not updated.  After (un)install, run this:"; \
		echo "***   $(gtk_update_icon_cache)"; \
	fi

desktopdir = $(datadir)/applications
desktop_in_files = empathy.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

schemasdir	 = $(GCONF_SCHEMA_FILE_DIR)
schemas_in_files = empathy.schemas.in
schemas_DATA 	 = $(schemas_in_files:.schemas.in=.schemas)
@INTLTOOL_SCHEMAS_RULE@

if GCONF_SCHEMAS_INSTALL
install-data-local:
	if test -z "$(DESTDIR)" ; then \
	  for p in $(schemas_DATA) ; do \
	    GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-install-rule $(srcdir)/$$p ; \
	  done \
	fi
else
install-data-local:
endif

EXTRA_DIST = 				\
	$(image_DATA)			\
	$(profile_DATA)			\
	$(desktop_in_files)		\
	$(desltop_DATA)		\
	$(schemas_in_files)		\
	$(schemas_DATA)

DISTCLEANFILES = 			\
	$(desktop_DATA)		\
	$(schemas_DATA)

