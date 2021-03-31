#!/bin/sh
#
# This file is part of the Linux Power Policy Manager
#
# Copyright © 2007 Intel Corporation
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to
# deal in the Software without restriction, including without limitation the
# rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
# sell copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
# IN THE SOFTWARE.
#
# Alternatively, the contents of this file may be used under the terms of the
# GNU General Public License version 2 (the "GPL"), in which case the
# provisions of the GPL are applicable instead of the above.  If you wish to
# allow the use of your version of this file only under the terms of the GPL
# and not to allow others to use your version of this file under the MIT
# license, indicate your decision by deleting the provisions above and replace
# them with the notice and other provisions required by the GPL.  If you do
# not delete the provisions above, a recipient may use your version of this
# file under either the GPL or the MIT license.
#
# Authors:
# 	Tariq Shureih  <tariq.shureih@intel.com>
# 	Arjan van de Ven <arjan@linux.intel.com>
# 	Mohamed Abbas <mohamed.abbas@intel.com>
# 	Sarah Sharp <sarah.a.sharp@intel.com>
#
# usage:
#	./test.sh [ppm directory] <mode>...
#	assumes that the mode directory is one level below the layer directory

# Find the top level directory, shift variables if necessary
PPM_DIR="$1"
if ! [ -d "$PPM_DIR/modes/" -a -d "$PPM_DIR/layers/" ]
then
	PPM_DIR=`pwd`
else
	shift
fi

TMPFILE=`mktemp` || exit 1

# Check to make sure they passed in at least one mode
if [ $# -lt 1 ]
then
	echo 'Usage:  ./test.sh [ppm source directory] <mode>...' >&2
	exit
fi

cd "$PPM_DIR/layers"

# Run ppmtest to change modes and record plugin command to stdout
./../daemon/ppmtest $@ | sort -suk 1,2 > $TMPFILE

# Use sort and unique to print out
# the list of commands that should have been sent.
for f in "$@"
do
	echo ../modes/"$f"
done |
xargs sort -ru |
# only grab command lines, ignore flags or event lines
grep '^[0-9]\{2\}_' |
xargs sort -suk 1,2 |
sed 's/[[:space:]]\+/ /g' |
diff - $TMPFILE

rm $TMPFILE
