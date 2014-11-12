dnl 
dnl   Copyright 2001-2013 Adrian Thurston <thurston@complang.org>
dnl
  
AC_INIT(ragel, 7.0.0.1)
PUBDATE="October 2014"

AM_INIT_AUTOMAKE([foreign])
AC_SUBST(PUBDATE)
AC_CONFIG_HEADER(src/config.h)

: ${CFLAGS="-Wall -g"}
: ${CXXFLAGS="-Wall -g"}

dnl Choose defaults for the build_manual var. If the dist file is present in the
dnl root then default to no, otherwise go for it.
AS_IF([test -r $srcdir/DIST], [. $srcdir/DIST], [build_manual=yes])

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
dnl COLM
dnl

COLM_EXPECTED=0.13.0.2

AC_ARG_WITH(colm,
	[AC_HELP_STRING([--with-colm], [location of colm install])],
		[COLM="$withval/bin/colm"],
		[COLM=`which colm`] )

if test -z "$COLM"; then
	AC_ERROR([colm is required to build ragel])
fi

dnl extract the first two precision points from the version
COLM_INSTALLED=`$COLM -v | sed -n '1{s/^.*version //; s/ .*$//; p}'`
if test "$COLM_INSTALLED" != $COLM_EXPECTED; then
	AC_ERROR([expected colm version $COLM_EXPECTED, but $COLM_INSTALLED is installed])
fi

COLM_FULL=[$COLM]
COLM_INCLUDE=[`dirname $COLM_FULL`/../include]
COLM_LIB=[`dirname $COLM_FULL`/../lib]

CPPFLAGS="$CPPFLAGS -I$COLM_INCLUDE"
LDFLAGS="$LDFLAGS -L$COLM_LIB"

AC_CHECK_HEADER([colm/colm.h], [], [AC_ERROR([colm.h not found])])
AC_CHECK_LIB([colmd], [colm_new_program])
AC_SUBST(COLM_FULL)

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

dnl Check for the D compiler
AC_CHECK_PROG(GDC, gdc, gdc)

dnl Check for the Objective-C compiler
AC_MSG_CHECKING([for the Objective-C compiler])
cat > conftest.m <<EOF
int main() { return 0; }
EOF
GOBJC=""
if gcc -x objective-c conftest.m -o conftest.bin 2>/dev/null; then
	GOBJC="gcc -x objective-c"
	AC_MSG_RESULT([yes])
else
	AC_MSG_RESULT([no])
fi
AC_SUBST(GOBJC)

dnl Check for the Java compiler.
AC_CHECK_PROG(JAVAC, javac, javac)

dnl Check for Ruby.
AC_CHECK_PROG(RUBY, ruby, ruby)

dnl Check for the C# compiler.
AC_CHECK_PROG(GMCS, gmcs, gmcs)

dnl Check for the Go compiler.
AC_CHECK_PROG(GOBIN, go, go)

dnl Check for the ocaml compiler.
AC_CHECK_PROG(OCAML, ocaml, ocaml)

SED_SUBST=["\
	-e 's|@CXX@|${CXX}|g' \
	-e 's|@CC@|${CC}|g' \
	-e 's|@GOBJC@|${GOBJC}|g' \
	-e 's|@GDC@|${GDC}|g' \
	-e 's|@JAVAC@|${JAVAC}|g' \
	-e 's|@RUBY@|${RUBY}|g' \
	-e 's|@GMCS@|${GMCS}|g' \
	-e 's|@GOBIN@|${GOBIN}|g' \
	-e 's|@OCAML@|${OCAML}|g' \
"]

AC_SUBST(SED_SUBST)

dnl write output files
AC_OUTPUT([
		Makefile
		src/Makefile
		aapl/Makefile 
		doc/Makefile
		doc/ragel.1
		contrib/Makefile
		test/Makefile
		examples/Makefile
])

echo "configuration of ragel complete"
