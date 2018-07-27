#!/bin/bash
#

set -xe

cd public
./autogen.sh
cd ..

# END PUBLIC

cd cases
./autogen.sh
cd ..

cd genf/app1.d
$HOME/pkgs/pkgbuild/bin/autogen.sh
cd ../..

cd genf/kern1.d
$HOME/pkgs/pkgbuild/bin/autogen.sh
cd ../..

# BEGIN PUBLIC

rm -f configure
cp configure.in configure
chmod +x-w configure

