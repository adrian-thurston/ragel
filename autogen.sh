#!/bin/bash
#

set -x

libtoolize --copy --force
aclocal
autoheader
automake --foreign --add-missing 
autoconf
