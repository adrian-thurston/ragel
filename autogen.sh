#!/bin/bash
#

libtoolize --copy --force
aclocal
# autoheader 
automake --foreign --add-missing 
autoconf
