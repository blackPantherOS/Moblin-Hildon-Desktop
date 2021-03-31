#!/bin/sh

set -x
libtoolize --automake
intltoolize -c --force --automake
aclocal 
autoconf
autoheader
automake --add-missing --foreign
