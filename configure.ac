  
AC_INIT(aapl, 1.0.1)
PUBDATE="Aug 2016"

AM_INIT_AUTOMAKE([foreign])
AC_SUBST(PUBDATE)
AC_CONFIG_HEADER(src/config.h)

AC_CONFIG_MACRO_DIR([m4])
m4_include([m4/common.m4])

dnl write output files
AC_OUTPUT([
		Makefile
		src/Makefile
])

echo "configuration of aal complete"
