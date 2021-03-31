all: cei_server.h cei_client.h

cei_server.h:
	dbus-binding-tool --mode=glib-server --prefix=cei cei.xml > cei_server.h

cei_client.h:
	dbus-binding-tool --mode=glib-client --prefix=cei cei.xml > cei_client.h

install:

clean:
	@rm -f cei_server.h cei_client.h

uninstall:

test:

.PHONY: all clean install uninstall test
