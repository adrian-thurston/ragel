#!/bin/bash
#

set -xe

aclocal
autoheader
automake --foreign --add-missing 
autoconf

