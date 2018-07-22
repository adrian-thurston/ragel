#!/bin/bash
#

set -x

aclocal
autoheader
automake --foreign --add-missing 
autoconf
