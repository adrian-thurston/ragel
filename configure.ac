dnl 
dnl   Copyright 2006-2016 Adrian Thurston <thurston@complang.org>
dnl

dnl   This file is part of Colm.
dnl  
dnl   Colm is free software; you can redistribute it and/or modify
dnl   it under the terms of the GNU General Public License as published by
dnl   the Free Software Foundation; either version 2 of the License, or
dnl   (at your option) any later version.
dnl   
dnl   Colm is distributed in the hope that it will be useful,
dnl   but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl   GNU General Public License for more details.
dnl   
dnl   You should have received a copy of the GNU General Public License
dnl   along with Colm; if not, write to the Free Software
dnl   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
  
AC_INIT(colm, 0.13.0.4)
PUBDATE="July 2016"

AM_INIT_AUTOMAKE([foreign])

: ${CFLAGS="-Wall -g"}
: ${CXXFLAGS="-Wall -g"}

AC_CONFIG_MACRO_DIR([m4])

AC_SUBST(PUBDATE)
AC_CONFIG_HEADER([src/config.h src/defs.h])

dnl Choose a default for the build_manual var. If the dist file is present in
dnl the root then default to no, otherwise go for it.
AC_CHECK_FILES( [$srcdir/DIST], 
	[. $srcdir/DIST;], 
	[build_manual=yes; ] )

dnl Set to true if the manual should be built.
AM_CONDITIONAL(BUILD_MANUAL, [test "x$build_manual" = "xyes"])

AC_CHECK_SIZEOF([long])

dnl Generic dependency specification.
AC_ARG_WITH(deps,
	[AC_HELP_STRING([--with-deps], [generic dependency location])],
	[DEPS="$withval"],
	[DEPS="/opt/colm"]
)

dnl Set to true if build system should generate parsers from ragel, kelbt, and
dnl gperf sources. Set to false if generated files are included and not to be
dnl built (production).
dnl AC_SUBST(BUILD_PARSERS,true)

dnl Checks for programs.
AC_PROG_CC

dnl Checks for programs.
AC_PROG_CXX

AC_PROG_LIBTOOL

dnl Set test on c++ compiler.
AC_LANG_CPLUSPLUS

dnl Check for definition of MAKE.
AC_PROG_MAKE_SET

AC_ARG_ENABLE(pool-malloc, 
		AC_HELP_STRING([--enable-pool-malloc], [allocate pool objects with malloc]), 
		AC_DEFINE([POOL_MALLOC], [1], [allocate pool objects with malloc]))

AC_ARG_ENABLE(debug,
		AC_HELP_STRING([--enable-debug], [enable debug statements]), 
		AC_DEFINE([DEBUG], [1], [enable debug statements]))

dnl write output files
AC_OUTPUT([
	Makefile
	src/Makefile
	aapl/Makefile
])

echo "configuration of colm complete"
