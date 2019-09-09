#!/bin/bash

set -x

libtoolize --quiet --copy --force
aclocal
autoheader
automake --foreign --add-missing
autoconf

