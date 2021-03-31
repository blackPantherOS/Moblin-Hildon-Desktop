#!/bin/sh
# Script for running GNU toolchain for configuration

set -x
libtoolize --automake --copy
aclocal
autoconf
autoheader
automake --add-missing --foreign --force --copy
