dnl 
dnl Copyright 2001-2018 Adrian Thurston <thurston@colm.net>
dnl
dnl Permission is hereby granted, free of charge, to any person obtaining a copy
dnl of this software and associated documentation files (the "Software"), to
dnl deal in the Software without restriction, including without limitation the
dnl rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
dnl sell copies of the Software, and to permit persons to whom the Software is
dnl furnished to do so, subject to the following conditions:
dnl
dnl The above copyright notice and this permission notice shall be included in all
dnl copies or substantial portions of the Software.
dnl
dnl THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
dnl IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
dnl FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
dnl AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
dnl LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
dnl OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
dnl SOFTWARE.
dnl
  
AC_INIT(ragel, 7.0.0.10)
PUBDATE="May 2017"

EXPECTED_COLM=0.13.0.5

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

dnl
dnl Build the program? ( no means just build libfsm )
dnl
AC_ARG_ENABLE(program,
	[AC_HELP_STRING([--enable-program], [build the program? no means libfsm only] )],
	[
		if test "x$enableval" = "xyes"; then
			build_program=yes;
		else
			build_program=no;
		fi
	],
	[
		build_program=yes;
	]
)

dnl Checks for programs.
AC_PROG_CC
AC_PROG_CXX
AC_CHECK_TOOL(AR, ar)
AC_PROG_RANLIB
AC_PROG_LIBTOOL

dnl Set test on c++ compiler.
AC_LANG_CPLUSPLUS

dnl Check for definition of MAKE.
AC_PROG_MAKE_SET

AC_CHECK_SIZEOF([int])
AC_CHECK_SIZEOF([long])

if [test "x$build_program" = "xyes"]; then
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

	AC_SUBST(RAGEL)
	AC_SUBST(KELBT)

	if test x$RAGEL != x -a x$KELBT != x; then
		RAGEL_KELBT=yes
		AC_DEFINE([WITH_RAGEL_KELBT], [1], [ragel AND kelbt are available])
	fi

	dnl
	dnl Check for colm.
	dnl
	AC_ARG_WITH(colm,
		[AC_HELP_STRING([--with-colm], [location of colm install (or built source)])],
		[
			if test -f "$withval/src/fsmgraph.h"; then
				CPPFLAGS="${CPPFLAGS} -I$withval/src/include"
				LDFLAGS="${LDFLAGS} -L$withval/src"
				COLM="$withval/src/colm"
				COLM_LD="$withval/src/.libs/libcolm.a"
				COLM_LIBDEP="$COLM_LD"
				COLM_BINDEP="$COLM"
			else
				CPPFLAGS="${CPPFLAGS} -I$withval/include"
				LDFLAGS="${LDFLAGS} -L$withval/lib"
				COLM="$withval/bin/colm"
				COLM_LD="-lcolm"
				COLM_LIBDEP=""
				COLM_BINDEP="$COLM"
			fi

			AC_CHECK_FILES(
				[$COLM],
				[],
				[AC_ERROR([--with-colm was specified, but binary was not found])]
			)
		],
		[
			AC_PATH_PROG( [COLM], [colm] )
			COLM_LD="-lcolm"
			COLM_LIBDEP=""
			COLM_BINDEP=""
		]
	)

	if test -z "$COLM"; then
		AC_ERROR([colm is required to build ragel])
	fi

	AC_CHECK_FILES(
		[$COLM],
		[],
		[AC_ERROR([could not find $COLM])]
	)

	INSTALLED_VER=`$COLM -v | sed -n -e '1 { s/^.*version //; s/ .*$//; p; }'`
	if test "x$INSTALLED_VER" != "x$EXPECTED_COLM"; then
		AC_ERROR( [check colm: expected version $EXPECTED_COLM, but $INSTALLED_VER is installed] )
	fi

	AC_SUBST(COLM)
	AC_SUBST(COLM_LD)
	AC_SUBST(COLM_LIBDEP)
	AC_SUBST(COLM_BINDEP)
fi

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

AM_CONDITIONAL([BUILD_PROGRAM],    [test "x$build_program" = "xyes"])
AM_CONDITIONAL([BUILD_MANUAL],     [test "x$build_manual" = "xyes"])
AM_CONDITIONAL([WITH_RAGEL_KELBT], [test "x$RAGEL_KELBT" = "xyes"])

dnl write output files
AC_OUTPUT([
		Makefile
		src/Makefile
		src/ragel-ruby/Makefile
		src/ragel-asm/Makefile
		src/ragel-julia/Makefile
		src/ragel-ocaml/Makefile
		src/ragel-c/Makefile
		aapl/Makefile 
		doc/Makefile
		doc/ragel.1
])

echo "configuration of ragel complete"
