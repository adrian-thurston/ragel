dnl 
dnl   Copyright 2001-2013 Adrian Thurston <thurston@complang.org>
dnl
  
AC_INIT(ragel-suite, 1.0.1)
PUBDATE="November 2014"

EXPECTED_VER=0.13.0.4

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
		[SUBJECT="\${abs_top_builddir}/../ragel/src/ragel"] )

AC_SUBST(SUBJECT)

dnl Check for colm.
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
		LD_LIBRARY_PATH="$withval/lib"
		COLM="$withval/bin/colm"
	],
	[
		CPPFLAGS="${CPPFLAGS} -I$DEPS/include"
		LDFLAGS="${LDFLAGS} -L$DEPS/lib"
		LD_LIBRARY_PATH="$DEPS/lib"
		COLM="$DEPS/bin/colm"
	]
)

AC_CHECK_FILES(
	[$COLM],
	[],
	[AC_ERROR([colm is required to build this package])]
)
AC_SUBST(COLM)
AC_SUBST(LD_LIBRARY_PATH)

INSTALLED_VER=`$COLM -v | sed -n -e '1 { s/^.*version //; s/ .*$//; p; }'`
if test "x$INSTALLED_VER" != "x$EXPECTED_VER"; then
	AC_ERROR( [check colm: expected version $EXPECTED_VER, but $INSTALLED_VER is installed] )
fi

dnl Check for the D compiler. Disabled for now, need to update D syntax.
dnl AC_CHECK_PROG(GDC, gdc, gdc)

dnl
dnl Check for the Objective-C compiler and for required dependencies.
dnl 
AC_MSG_CHECKING([for the Objective-C compiler])
cat > conftest.m <<EOF
#import <Foundation/Foundation.h>

@interface ObjCTest : NSObject
{
@public 
	int i;
};

@end

int main() { return 0; }
EOF
GOBJC=""
if gcc -x objective-c -I/usr/include/GNUstep conftest.m -o conftest.bin -lobjc -lgnustep-base 2>/dev/null; then
	GOBJC="gcc -x objective-c"
	AC_MSG_RESULT([yes])
else
	AC_MSG_RESULT([no])
fi
AC_SUBST(GOBJC)

AC_CHECK_PROG(JAVAC, javac, javac)
AC_CHECK_PROG(RUBY, ruby, ruby)
AC_CHECK_PROGS(GMCS, [gmcs mcs])
AC_CHECK_PROG(GOBIN, go, go)
AC_CHECK_PROG(OCAML, ocaml, ocaml)
AC_CHECK_PROG(RUST, rustc, rustc)
AC_CHECK_PROG(JULIA, julia, julia)

AC_ARG_WITH(crack,
	[AC_HELP_STRING([--with-crack], [location of crack install])],
	[
		CRACK="$withval/bin/crack"
		AC_CHECK_FILES(
			[$CRACK], [],
			[AC_ERROR([could not find crack binary using --with-crack arg])]
		)
	],
	[
		AC_PATH_PROG( [CRACK], [crack] )
	]
)

SED_SUBST=["\
	-e 's|@SUBJECT@|${SUBJECT}|g' \
	-e 's|@LD_LIBRARY_PATH@|${LD_LIBRARY_PATH}|g' \
	-e 's|@CXX@|${CXX}|g' \
	-e 's|@CC@|${CC}|g' \
	-e 's|@GOBJC@|${GOBJC}|g' \
	-e 's|@GDC@|${GDC}|g' \
	-e 's|@JAVAC@|${JAVAC}|g' \
	-e 's|@RUBY@|${RUBY}|g' \
	-e 's|@GMCS@|${GMCS}|g' \
	-e 's|@GOBIN@|${GOBIN}|g' \
	-e 's|@OCAML@|${OCAML}|g' \
	-e 's|@RUST@|${RUST}|g' \
	-e 's|@CRACK@|${CRACK}|g' \
	-e 's|@JULIA@|${JULIA}|g' \
"]

AC_SUBST(SED_SUBST)

dnl write output files
AC_OUTPUT([
		Makefile
])

echo "configuration of ragel complete"
