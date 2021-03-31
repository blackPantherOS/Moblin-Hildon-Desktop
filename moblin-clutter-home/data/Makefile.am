default_DATA =  \
	default.svg				\
	hilight.svg				\
	hilight-long.svg			\
	spinner.svg				\
	catmap-hover.svg			

defaultdir = $(pkgdatadir)

schemadir = $(sysconfdir)/gconf/schemas
schema_DATA = desktop-launcher.schemas

if GCONF_SCHEMAS_INSTALL
install-data-local:
	-GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) \
	$(GCONFTOOL) --makefile-install-rule $(schema_DATA)
endif

if GCONF_SCHEMAS_INSTALL
uninstall-local:
	-GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) \
	$(GCONFTOOL) --makefile-uninstall-rule $(schema_DATA)
endif


EXTRA_DIST = $(default_DATA) $(schema_DATA)	
