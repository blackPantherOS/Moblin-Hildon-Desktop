SUBDIRS =  hooks power.d

extradir = $(sysconfdir)/pm

extra_SCRIPTS =			\
	functions		\
	functions-ati		\
	functions-intel		\
	functions-nvidia	\
	config

EXTRA_DIST =			\
	$(extra_SCRIPTS)

install-exec-local:
	-mkdir $(DESTDIR)$(sysconfdir)/pm/config.d

clean-local :
	rm -f *~
