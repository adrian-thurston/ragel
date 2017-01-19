#!/bin/bash

libtoolize --quiet --copy --force
aclocal
autoheader
automake --foreign --add-missing
autoconf

