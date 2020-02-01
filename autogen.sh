#!/bin/bash

set -x

case `uname` in
	Darwin)
		glibtoolize --quiet --copy --force
		;;
	*)
		libtoolize --quiet --copy --force
	;;
esac

aclocal
autoheader
automake --foreign --add-missing
autoconf

