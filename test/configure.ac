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
	[SUBJECT="$withval/bin/colm"],
	[SUBJECT="../src/colm"]
)

AC_SUBST(SUBJECT)

SED_SUBST=["\
	-e 's|@SUBJECT@|${SUBJECT}|g' \
"]
AC_SUBST(SED_SUBST)

dnl write output files
AC_OUTPUT([
		Makefile
])
