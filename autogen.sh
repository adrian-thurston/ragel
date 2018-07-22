#!/bin/bash
#

set -xe

cd public
./autogen.sh
cd ..

cd cases
./autogen.sh
cd ..

cd genf.d/app1
$HOME/pkgs/pkgbuild/bin/autogen.sh
cd ../..

