dnl 
dnl   Copyright 2001-2018 Adrian Thurston <thurston@colm.net>
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

AC_ARG_WITH(crack,
	[AC_HELP_STRING([--with-crack], [location of crack install])],
	[CRACK_BIN="$withval/bin/crack"],
	[AC_ERROR(["require --with-crack"])]
)

AC_SUBST(CRACK_BIN)

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

			RAGEL_C_BIN="$withval/bin/ragel-c"
			RAGEL_D_BIN="$withval/bin/ragel-d"
			RAGEL_JAVA_BIN="$withval/bin/ragel-java"
			RAGEL_RUBY_BIN="$withval/bin/ragel-ruby"
			RAGEL_CSHARP_BIN="$withval/bin/ragel-csharp"
			RAGEL_GO_BIN="$withval/bin/ragel-go"
			RAGEL_OCAML_BIN="$withval/bin/ragel-ocaml"
			RAGEL_ASM_BIN="$withval/bin/ragel-asm"
			RAGEL_RUST_BIN="$withval/bin/ragel-rust"
			RAGEL_CRACK_BIN="$withval/bin/ragel-crack"
			RAGEL_JULIA_BIN="$withval/bin/ragel-julia"
		else
			RAGEL_BIN="${withval}/src/ragel"
			RAGEL_CPPFLAGS="-I${withval}/src/include -I${withval}/aapl"
			RAGEL_LDFLAGS="-L${withval}/src/.libs -Wl,-rpath,${withval}/src/.libs"
			RAGEL_LM="${withval}/src"

			RAGEL_C_BIN="$withval/src/host-c/ragel-c"
			RAGEL_D_BIN="$withval/src/host-d/ragel-d"
			RAGEL_JAVA_BIN="$withval/src/host-java/ragel-java"
			RAGEL_RUBY_BIN="$withval/src/host-ruby/ragel-ruby"
			RAGEL_CSHARP_BIN="$withval/src/host-csharp/ragel-csharp"
			RAGEL_GO_BIN="$withval/src/host-go/ragel-go"
			RAGEL_OCAML_BIN="$withval/src/host-ocaml/ragel-ocaml"
			RAGEL_ASM_BIN="$withval/src/host-asm/ragel-asm"
			RAGEL_RUST_BIN="$withval/src/host-rust/ragel-rust"
			RAGEL_CRACK_BIN="$withval/src/host-crack/ragel-crack"
			RAGEL_JULIA_BIN="$withval/src/host-julia/ragel-julia"
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
	[AC_ERROR([ragel is required to build this package])]
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
		if test -d "$withval/src"; then
			AAPL_CPPFLAGS="-I${withval}/src -I${withval}/aapl"
		else
			AAPL_CPPFLAGS="-I$withval/include/aapl"
		fi
	],
	[AC_ERROR(["require --with-aapl"])]
)

AC_SUBST(AAPL_CPPFLAGS)
			
SED_SUBST=["\
	-e 's|@CRACK_BIN@|${CRACK_BIN}|g' \
	-e 's|@RAGEL_BIN@|${RAGEL_BIN}|g' \
	-e 's|@RAGEL_C_BIN@|${RAGEL_C_BIN}|g' \
	-e 's|@RAGEL_D_BIN@|${RAGEL_D_BIN}|g' \
	-e 's|@RAGEL_JAVA_BIN@|${RAGEL_JAVA_BIN}|g' \
	-e 's|@RAGEL_RUBY_BIN@|${RAGEL_RUBY_BIN}|g' \
	-e 's|@RAGEL_CSHARP_BIN@|${RAGEL_CSHARP_BIN}|g' \
	-e 's|@RAGEL_GO_BIN@|${RAGEL_GO_BIN}|g' \
	-e 's|@RAGEL_OCAML_BIN@|${RAGEL_OCAML_BIN}|g' \
	-e 's|@RAGEL_ASM_BIN@|${RAGEL_ASM_BIN}|g' \
	-e 's|@RAGEL_RUST_BIN@|${RAGEL_RUST_BIN}|g' \
	-e 's|@RAGEL_CRACK_BIN@|${RAGEL_CRACK_BIN}|g' \
	-e 's|@RAGEL_JULIA_BIN@|${RAGEL_JULIA_BIN}|g' \
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

# END PUBLIC

dnl
dnl Stop before AC_OUTPUT. The slicer will add the correct AC_OUTPUT.
dnl


dnl
dnl Configure the genf tests.
dnl
root=$(pwd)

AC_ARG_WITH(genf,
	[AC_HELP_STRING([--with-genf], [location of genf install])],
	[WITH_GENF="$withval"],
	[AC_ERROR(["require --with-genf"])]
)

cd cases/app1.d
./configure --prefix=$root/cases/inst/app1 --with-aapl=$WITH_AAPL --with-genf=$WITH_GENF
cd ../..

cd cases/kern1.d
./configure --prefix=$root/cases/inst/kern1 --with-aapl=$WITH_AAPL --with-genf=$WITH_GENF
cd ../..

cd cases/process1.d
./configure --prefix=$root/cases/inst/kern1 --with-aapl=$WITH_AAPL --with-genf=$WITH_GENF
cd ../..

dnl
dnl Finish up.
dnl

AC_SUBST(SED_SUBST)

AC_OUTPUT([
		Makefile
		cases/Makefile
		cases/ragel.d/Makefile
		cases/aapl.d/Makefile
		cases/colm.d/Makefile
		cases/yellowbrick.d/Makefile
		cases/yellowbrick.d/src/Makefile
		cases/pcre-ragel.d/Makefile
])

echo "configuration of testing package complete"

