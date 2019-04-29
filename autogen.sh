#!/bin/bash
#

set -xe

aclocal
autoheader
automake --foreign --add-missing 
autoconf

# END PUBLIC

# GENF cases must have autogen.sh called.

cd cases/app1.d
$HOME/pkgs/pkgbuild/bin/autogen.sh
cd ../..

cd cases/kern1.d
$HOME/pkgs/pkgbuild/bin/autogen.sh
cd ../..

cd cases/process1.d
$HOME/pkgs/pkgbuild/bin/autogen.sh
cd ../..
