dnl 
dnl   Copyright 2001-2013 Adrian Thurston <thurston@complang.org>
dnl
  
AC_INIT(test-suite, 1.0.1)

EXPECTED_COLM_VER=0.13.0.5

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
dnl Which ragel are we testing. Default is the one in parent's source dir.
dnl
AC_ARG_WITH(ragel,
	[AC_HELP_STRING([--with-ragel], [location of ragel to test])],
	[
		if test -d "$withval/bin"; then
			RAGEL_BIN="$withval/bin/ragel"
			RAGEL_CPPFLAGS="-I$withval/include"
			RAGEL_LDFLAGS="-L$withval/lib"
			RAGEL_LM="${withval}/share/ragel.lm"
			LD_LIBRARY_PATH="$withval/lib"
		else
			RAGEL_BIN="${withval}/src/ragel"
			RAGEL_CPPFLAGS="-I${withval}/src -I${withval}/aapl"
			RAGEL_LDFLAGS="-L${withval}/src/.libs"
			RAGEL_LM="${withval}/src/ragel.lm"
			LD_LIBRARY_PATH="${withval}/src/.libs"
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

AC_ARG_WITH(deps,
	[AC_HELP_STRING([--with-deps], [generic dependency location])],
	[DEPS="$withval"],
	[DEPS="/opt/colm"]
)

AC_ARG_WITH(colm,
	[AC_HELP_STRING([--with-colm], [location of colm install])],
	[
		if test -d "${withval}/bin"; then
			COLM_BIN="${withval}/bin/colm"
			COLM_CPPFLAGS="-I${withval}/include"
			COLM_LDFLAGS="${LDFLAGS} -L${withval}/lib"
			LD_LIBRARY_PATH="${LD_LIBRARY_PATH}:${withval}/lib"
		else
			COLM_BIN="${withval}/src/colm"
			COLM_CPPFLAGS="-I${withval}/src/include"
			COLM_LDFLAGS="-L${withval}/src/.libs"
			LD_LIBRARY_PATH="${LD_LIBRARY_PATH}:${withval}/src/.libs"
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

AC_SUBST(LD_LIBRARY_PATH)

INSTALLED_VER=`$COLM_BIN -v | sed -n -e '1 { s/^.*version //; s/ .*$//; p; }'`
if test "x$INSTALLED_VER" != "x$EXPECTED_COLM_VER"; then
	AC_ERROR( [check colm: expected version $EXPECTED_COLM_VER, but $INSTALLED_VER is installed] )
fi

SED_SUBST=["\
	-e 's|@RAGEL_BIN@|${RAGEL_BIN}|g' \
	-e 's|@RAGEL_CPPFLAGS@|${RAGEL_CPPFLAGS}|g' \
	-e 's|@RAGEL_LDFLAGS@|${RAGEL_LDFLAGS}|g' \
	-e 's|@RAGEL_LM@|${RAGEL_LM}|g' \
	-e 's|@COLM_BIN@|${COLM_BIN}|g' \
	-e 's|@COLM_CPPFLAGS@|${COLM_CPPFLAGS}|g' \
	-e 's|@COLM_LDFLAGS@|${COLM_LDFLAGS}|g' \
	-e 's|@LD_LIBRARY_PATH@|${LD_LIBRARY_PATH}|g' \
	-e 's|@CXX@|${CXX}|g' \
	-e 's|@CC@|${CC}|g' \
"]

AC_SUBST(SED_SUBST)

dnl write output files
AC_OUTPUT([
		ragel/Makefile
		colm/Makefile
])

echo "configuration of ragel test package complete"
