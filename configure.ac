dnl 
dnl   Copyright 2001-2014 Adrian Thurston <thurston@complang.org>
dnl
  
AC_INIT(ragel, 7.0.0.1)
PUBDATE="December 2014"

AC_CONFIG_MACRO_DIR([m4])
m4_include([m4/ragel.m4])

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

dnl Check for colm.
AC_CHECK_COLM([0.13.0.2])

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
