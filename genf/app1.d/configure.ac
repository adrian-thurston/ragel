AC_INIT([app1], [1.0.1])
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

dnl
dnl Which genf are we testing. Default is the one in parent's source dir. If
dnl specified, should be the install prefix.
dnl
AC_ARG_WITH(subject,
	[AC_HELP_STRING([--with-subject], [location of genf to test])],
	[
		SUBJECT="$withval/bin/genf"
		CPPFLAGS="${CPPFLAGS} -I$withval/include"
		LDFLAGS="${LDFLAGS} -L$withval/lib"
	],
	[
		SUBJECT="`pwd`/../src/genf"
		CPPFLAGS="${CPPFLAGS} -I`pwd`/../src/include"
		LDFLAGS="${LDFLAGS} -L`pwd`/../src/.libs"
	]
)

AC_CHECK_GENF_DEPS()

AC_CHECK_FILES([$SUBJECT], [], [AC_ERROR([subject binary $SUBJECT does not exist])])
AC_CHECK_LIB([genf], [genf_thread_start], [], [AC_ERROR([check genf: cannot link with -lgenf])])

AC_SUBST(SUBJECT)
AC_SUBST(top_srcdir)

AC_SUBST(SED_SUBST)

dnl write output files
AC_OUTPUT([
		Makefile
		src/Makefile
])
