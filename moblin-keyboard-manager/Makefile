all:	libkeyboard.so

libkeyboard.so:	libkeyboard.c
	gcc -fPIC -shared `pkg-config --cflags --libs gtk+-2.0 libhildondesktop libosso gconf-2.0` \
	libkeyboard.c -o libkeyboard.so

install:	all
	echo "nothing to do"	
clean:
	rm -rf *.o *.so *~
