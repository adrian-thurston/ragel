#!/bin/bash
#

aclocal
autoheader
automake --foreign --add-missing 
autoconf
