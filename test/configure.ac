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
	],
	[
		SUBJECT_BIN="../src/colm"
		SUBJECT_CPPFLAGS="-I../src/include"
		SUBJECT_LDFLAGS="-L../src"
	]
)

AC_SUBST(SUBJECT_BIN)
AC_SUBST(SUBJECT_CPPFLAGS)
AC_SUBST(SUBJECT_LDFLAGS)

SED_SUBST=["\
	-e 's|@SUBJECT_BIN@|${SUBJECT_BIN}|g' \
	-e 's|@SUBJECT_CPPFLAGS@|${SUBJECT_CPPFLAGS}|g' \
	-e 's|@SUBJECT_LDFLAGS@|${SUBJECT_LDFLAGS}|g' \
"]
AC_SUBST(SED_SUBST)

dnl write output files
AC_OUTPUT([
		Makefile
])
