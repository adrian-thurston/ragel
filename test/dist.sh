#!/bin/bash
#

# Make the dist, build from the dist it, install, then run the test suite
# against what's been installed.

set -ex

VERSION=`sed -n '/^[ \t]*AC_INIT\>/{ s/.*, *//; s/ *).*//; p }' configure.ac`

rm -Rf colm-suite-${VERSION}.tar.{gz,bz2} colm-suite-${VERSION}

make dist
tar -zxvf colm-suite-${VERSION}.tar.gz

cd colm-suite-${VERSION}

./configure --prefix=/tmp/colm-suite \
	--with-crack=/home/thurston/pkgs/crack --enable-manual --enable-debug

make

cd test

./runtests
