dnl 
dnl   Copyright 2001-2013 Adrian Thurston <thurston@complang.org>
dnl
  
AC_INIT(ragel-suite, 1.0.1)
PUBDATE="November 2014"

AM_INIT_AUTOMAKE([foreign])
AC_SUBST(PUBDATE)
AC_CONFIG_HEADER(config.h)

: ${CFLAGS="-Wall -g"}
: ${CXXFLAGS="-Wall -g"}

dnl Checks for programs.
AC_PROG_CC
AC_PROG_CXX
AC_PROG_RANLIB

dnl Set test on c++ compiler.
AC_LANG_CPLUSPLUS

dnl Check for definition of MAKE.
AC_PROG_MAKE_SET

dnl
dnl Which ragel are we testing. Default is the one in parent's source dir.
dnl
AC_ARG_WITH(subject,
	[AC_HELP_STRING([--with-subject], [location of ragel to test])],
		[SUBJECT="$withval/bin/ragel"],
		[SUBJECT="../src/ragel"] )

AC_SUBST(SUBJECT)

dnl Check for colm.
EXPECTED_VER=0.13.0.1
if test -z "$EXPECTED_VER"; then
	AC_ERROR( [check colm: expected version not passed in] )
fi

AC_ARG_WITH(deps,
	[AC_HELP_STRING([--with-deps], [generic dependency location])],
	[DEPS="$withval"],
	[DEPS="/opt/colm"]
)

AC_ARG_WITH(colm,
	[AC_HELP_STRING([--with-colm], [location of colm install])],
	[
		CPPFLAGS="${CPPFLAGS} -I$withval/include"
		LDFLAGS="${LDFLAGS} -L$withval/lib"
		COLM="$withval/bin/colm"
	],
	[
		CPPFLAGS="${CPPFLAGS} -I$DEPS/include"
		LDFLAGS="${LDFLAGS} -L$DEPS/lib"
		COLM="$DEPS/bin/colm"
	]
)

AC_CHECK_FILES(
	[$COLM],
	[],
	[AC_ERROR([colm is required to build this package])]
)
AC_SUBST(COLM)

INSTALLED_VER=`$COLM -v | sed -n -e '1 { s/^.*version //; s/ .*$//; p; }'`
if test "x$INSTALLED_VER" != "x$EXPECTED_VER"; then
	AC_ERROR( [check colm: expected version $EXPECTED_VER, but $INSTALLED_VER is installed] )
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
	-e 's|@SUBJECT@|${SUBJECT}|g' \
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
])

echo "configuration of ragel complete"
