#!/bin/bash
#

# Make the dist, build from the dist it, install, then run the test suite
# against what's been installed.

set -ex

make dist

rm -Rf colm-suite-1.0.1

tar -zxvf colm-suite-1.0.1.tar.gz

cd colm-suite-1.0.1

./configure --prefix=/tmp/colm-suite \
	--with-crack=/home/thurston/pkgs/crack --enable-manual --enable-debug

make

cd test

./runtests
