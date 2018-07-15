dnl 
dnl   Copyright 2001-2013 Adrian Thurston <thurston@complang.org>
dnl
  
AC_INIT(test-suite, 1.0.1)

EXPECTED_COLM_VER=0.13.0.6

AM_INIT_AUTOMAKE([foreign])
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
dnl Which ragel are we testing. Can use a src or install directory.
dnl
AC_ARG_WITH(ragel,
	[AC_HELP_STRING([--with-ragel], [location of ragel to test (source or installed)])],
	[
		if test -d "$withval/bin"; then
			RAGEL_BIN="$withval/bin/ragel"
			RAGEL_CPPFLAGS="-I$withval/include"
			RAGEL_LDFLAGS="-L$withval/lib -Wl,-rpath,${withval}/lib"
			RAGEL_LM="${withval}/share"
		else
			RAGEL_BIN="${withval}/src/ragel"
			RAGEL_CPPFLAGS="-I${withval}/src -I${withval}/aapl"
			RAGEL_LDFLAGS="-L${withval}/src/.libs -Wl,-rpath,${withval}/src/.libs"
			RAGEL_LM="${withval}/src"
		fi
	],
	[AC_ERROR(["require --with-ragel"])]
)

AC_SUBST(RAGEL_BIN)
AC_SUBST(RAGEL_CPPFLAGS)
AC_SUBST(RAGEL_LDFLAGS)
AC_SUBST(RAGEL_LM)

AC_ARG_WITH(ragel-src,
	[AC_HELP_STRING([--with-ragel-src], [location of ragel src code])],
		[RAGEL_SRC="$withval"],
		[RAGEL_SRC="$HOME/devel/ragel/src"] )

AC_CHECK_FILES(
	[$RAGEL_BIN],
	[],
	[AC_ERROR([colm is required to build this package])]
)

AC_SUBST(RAGEL_SRC)

dnl Check for colm.
if test -z "$EXPECTED_COLM_VER"; then
	AC_ERROR( [check colm: expected version not passed in] )
fi

AC_ARG_WITH(colm,
	[AC_HELP_STRING([--with-colm], [location of colm to test (source or installed)])],
	[
		if test -d "${withval}/bin"; then
			COLM_BIN="${withval}/bin/colm"
			COLM_CPPFLAGS="-I${withval}/include"
			COLM_LDFLAGS="${LDFLAGS} -L${withval}/lib -Wl,-rpath,${withval}/lib"
		else
			COLM_BIN="${withval}/src/colm"
			COLM_CPPFLAGS="-I${withval}/src/include"
			COLM_LDFLAGS="-L${withval}/src/.libs -Wl,-rpath,${withval}/src/.libs"
		fi
	],
	[AC_ERROR(["require --with-colm"])]
)

AC_CHECK_FILES(
	[$COLM_BIN],
	[],
	[AC_ERROR([colm is required to build this package])]
)

AC_SUBST(COLM_BIN)
AC_SUBST(COLM_CPPFLAGS)
AC_SUBST(COLM_LDFLAGS)

INSTALLED_VER=`$COLM_BIN -v | sed -n -e '1 { s/^.*version //; s/ .*$//; p; }'`
if test "x$INSTALLED_VER" != "x$EXPECTED_COLM_VER"; then
	AC_ERROR( [check colm: expected version $EXPECTED_COLM_VER, but $INSTALLED_VER is installed] )
fi

dnl
dnl Which aapl
dnl
AC_ARG_WITH(aapl,
	[AC_HELP_STRING([--with-aapl], [location of aapl to test (source or installed)])],
	[
		if test -d "$withval/bin"; then
			AAPL_CPPFLAGS="-I$withval/include"
		else
			AAPL_CPPFLAGS="-I${withval}/src -I${withval}/aapl"
		fi
	],
	[AC_ERROR(["require --with-aapl"])]
)

AC_SUBST(AAPL_CPPFLAGS)

SED_SUBST=["\
	-e 's|@RAGEL_BIN@|${RAGEL_BIN}|g' \
	-e 's|@RAGEL_CPPFLAGS@|${RAGEL_CPPFLAGS}|g' \
	-e 's|@RAGEL_LDFLAGS@|${RAGEL_LDFLAGS}|g' \
	-e 's|@RAGEL_LM@|${RAGEL_LM}|g' \
	-e 's|@COLM_BIN@|${COLM_BIN}|g' \
	-e 's|@COLM_CPPFLAGS@|${COLM_CPPFLAGS}|g' \
	-e 's|@COLM_LDFLAGS@|${COLM_LDFLAGS}|g' \
	-e 's|@AAPL_CPPFLAGS@|${AAPL_CPPFLAGS}|g' \
	-e 's|@CXX@|${CXX}|g' \
	-e 's|@CC@|${CC}|g' \
"]

AC_SUBST(SED_SUBST)

dnl START PRIVATE CASES
dnl
dnl NEED some solution stripping out private cases from AC_OUTPUT. Can't put
dnl comments in AC_OUTPUT.
dnl
dnl END PRIVATE CASES

dnl write output files
AC_OUTPUT([
		Makefile
		cases/Makefile
		cases/ragel.d/Makefile
		cases/aapl.d/Makefile
		cases/colm.d/Makefile
		cases/dummy.d/Makefile
])

echo "configuration of testing package complete"
