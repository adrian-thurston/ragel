#!/bin/bash
#

set -xe

aclocal
autoheader
automake --foreign --add-missing 
autoconf

# END PUBLIC

# GENF cases must have autogen.sh called.

cd genf/app1.d
$HOME/pkgs/pkgbuild/bin/autogen.sh
cd ../..

cd genf/kern1.d
$HOME/pkgs/pkgbuild/bin/autogen.sh
cd ../..

cd genf/process1.d
$HOME/pkgs/pkgbuild/bin/autogen.sh
cd ../..
