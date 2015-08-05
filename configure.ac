dnl 
dnl   Copyright 2001-2014 Adrian Thurston <thurston@complang.org>
dnl

dnl   This file is part of Ragel.
dnl  
dnl   Ragel is free software; you can redistribute it and/or modify
dnl   it under the terms of the GNU General Public License as published by
dnl   the Free Software Foundation; either version 2 of the License, or
dnl   (at your option) any later version.
dnl   
dnl   Ragel is distributed in the hope that it will be useful,
dnl   but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl   GNU General Public License for more details.
dnl   
dnl   You should have received a copy of the GNU General Public License
dnl   along with Ragel; if not, write to the Free Software
dnl   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
  
AC_INIT(ragel, 7.0.0.5)
PUBDATE="December 2014"

EXPECTED_COLM=0.13.0.2

AM_INIT_AUTOMAKE([foreign])
AC_SUBST(PUBDATE)
AC_CONFIG_HEADER(src/config.h)

: ${CFLAGS="-Wall -g"}
: ${CXXFLAGS="-Wall -g"}

dnl
dnl Enable arg to explicitly control the building of the manual
dnl
AC_ARG_ENABLE(manual,
	[  --enable-manual         do we want to build the manual?],
	[
		if test "x$enableval" = "xyes"; then
			build_manual=yes;
		else
			build_manual=no;
		fi
	],
)

dnl Set to true if the manual should be built.
AM_CONDITIONAL(BUILD_MANUAL, [test "x$build_manual" = "xyes"])

dnl Checks for programs.
AC_PROG_CC
AC_PROG_CXX
AC_CHECK_TOOL(AR, ar)
AC_PROG_RANLIB

dnl Set test on c++ compiler.
AC_LANG_CPLUSPLUS

dnl Check for definition of MAKE.
AC_PROG_MAKE_SET

dnl
dnl Check for ragel.
dnl
AC_ARG_WITH(ragel,
	[AC_HELP_STRING([--with-ragel], [location of ragel install])],
	[
		RAGEL="$withval/bin/ragel"
		AC_CHECK_FILES( [$RAGEL], [], [RAGEL=""] )
	],
	[
		AC_PATH_PROG( [RAGEL], [ragel] )
	]
)

dnl
dnl Check for kelbt.
dnl
AC_ARG_WITH(kelbt,
	[AC_HELP_STRING([--with-kelbt], [location of kelbt install])],
	[
		KELBT="$withval/bin/kelbt"
		AC_CHECK_FILES( [$KELBT], [], [KELBT=""] )
	],
	[
		AC_PATH_PROG( [KELBT], [kelbt] )
	]
)

if test -z "$RAGEL"; then
	AC_ERROR([ragel is required to build this package])
fi
if test -z "$KELBT"; then
	AC_ERROR([kelbt is required to build this package])
fi

AC_SUBST(RAGEL)
AC_SUBST(KELBT)


dnl
dnl Check for colm.
dnl
AC_ARG_WITH(colm,
	[AC_HELP_STRING([--with-colm], [location of colm install])],
	[
		CPPFLAGS="${CPPFLAGS} -I$withval/include"
		LDFLAGS="${LDFLAGS} -L$withval/lib"
		COLM="$withval/bin/colm"

		AC_CHECK_FILES(
			[$COLM],
			[],
			[AC_ERROR([--with-colm was specified, but binary was not found])]
		)

		INSTALLED_VER=`$COLM -v | sed -n -e '1 { s/^.*version //; s/ .*$//; p; }'`
		if test "x$INSTALLED_VER" != "x$EXPECTED_COLM"; then
			AC_ERROR( [check colm: expected version $EXPECTED_COLM,]
					[ but $INSTALLED_VER is installed] )
		fi

		AC_DEFINE([WITH_COLM], [1], [colm is available])
	],
	[]
)

AC_SUBST(COLM)
AM_CONDITIONAL([WITH_COLM], [test x$COLM != x])

dnl Checks to carry out if we are building the manual.
if test "x$build_manual" = "xyes"; then

AC_CHECK_PROG(FIG2DEV, fig2dev, fig2dev)
if test -z "$FIG2DEV"; then
	echo
	echo "error: fig2dev is required to build the manual (maybe use --disable-manual)"
	echo
	exit 1
fi

AC_CHECK_PROG(PDFLATEX, pdflatex, pdflatex)
if test -z "$PDFLATEX"; then
	echo
	echo "error: pdflatex is required to build the manual (maybe use --disable-manual)"
	echo
	exit 1
fi

fi

dnl write output files
AC_OUTPUT([
		Makefile
		src/Makefile
		aapl/Makefile 
		doc/Makefile
		doc/ragel.1
])

echo "configuration of ragel complete"
