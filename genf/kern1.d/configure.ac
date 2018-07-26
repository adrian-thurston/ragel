AC_INIT([kern1], [1.0.1])
AM_INIT_AUTOMAKE

AC_CONFIG_MACRO_DIR([m4])
m4_include([m4/common.m4])

: ${CFLAGS="-Wall -g"}
: ${CXXFLAGS="-Wall -g"}

dnl Checks for programs.
AC_PROG_CC
AC_PROG_CXX
AC_PROG_LIBTOOL
AC_LANG_CPLUSPLUS

dnl genf
AC_CHECK_GENF([0.1.0])

AC_SUBST(SED_SUBST)

dnl write output files
AC_OUTPUT([
		Makefile
		src/Makefile
])
