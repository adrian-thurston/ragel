AC_INIT([colm-suite], [1.0.1])
AM_INIT_AUTOMAKE

dnl Generic dependency specification.
AC_ARG_WITH(deps,
	[AC_HELP_STRING([--with-deps], [generic dependency location])],
	[DEPS="$withval"],
	[DEPS="/opt/colm"]
)

dnl
dnl Which colm are we testing. Default is the one in parent's source dir.
dnl
AC_ARG_WITH(subject,
	[AC_HELP_STRING([--with-subject], [location of colm to test])],
	[
		SUBJECT_BIN="$withval/bin/colm"
		SUBJECT_CPPFLAGS="-I$withval/include"
		SUBJECT_LDFLAGS="-L$withval/lib"
		SUBJECT_LD_LIBRARY_PATH="$withval/lib"
	],
	[
		SUBJECT_BIN="\${abs_top_builddir}/../src/colm"
		SUBJECT_CPPFLAGS="-I\${abs_top_builddir}/../src/include"
		SUBJECT_LDFLAGS="-L\${abs_top_builddir}/../src/.libs"
		SUBJECT_LD_LIBRARY_PATH="\${abs_top_builddir}/../src/.libs"
	]
)

dnl Can't do this due to lack of recursive substitution
dnl -- AC_CHECK_FILE($SUBJECT_BIN, [], [AC_ERROR([subject not found: $SUBJECT_BIN])])

AC_SUBST(SUBJECT_BIN)
AC_SUBST(SUBJECT_CPPFLAGS)
AC_SUBST(SUBJECT_LDFLAGS)
AC_SUBST(SUBJECT_LD_LIBRARY_PATH)

SED_SUBST=["\
	-e 's|@SUBJECT_BIN@|${SUBJECT_BIN}|g' \
	-e 's|@SUBJECT_CPPFLAGS@|${SUBJECT_CPPFLAGS}|g' \
	-e 's|@SUBJECT_LDFLAGS@|${SUBJECT_LDFLAGS}|g' \
	-e 's|@SUBJECT_LD_LIBRARY_PATH@|${SUBJECT_LD_LIBRARY_PATH}|g' \
"]
AC_SUBST(SED_SUBST)

dnl write output files
AC_OUTPUT([
		Makefile
		test/Makefile
])
