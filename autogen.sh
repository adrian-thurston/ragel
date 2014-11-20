#!/bin/bash
#

set -x

aclocal
automake --foreign --add-missing 
autoconf
