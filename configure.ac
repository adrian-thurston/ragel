dnl 
dnl Copyright 2006-2016 Adrian Thurston <thurston@colm.net>
dnl

dnl Permission is hereby granted, free of charge, to any person obtaining a copy
dnl of this software and associated documentation files (the "Software"), to
dnl deal in the Software without restriction, including without limitation the
dnl rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
dnl sell copies of the Software, and to permit persons to whom the Software is
dnl furnished to do so, subject to the following conditions:
dnl 
dnl The above copyright notice and this permission notice shall be included in all
dnl copies or substantial portions of the Software.
dnl 
dnl THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
dnl IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
dnl FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
dnl AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
dnl LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
dnl OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
dnl SOFTWARE.
  
AC_INIT(colm, 0.13.0.5)
PUBDATE="May 2017"

AM_INIT_AUTOMAKE([foreign])

: ${CFLAGS="-Wall -g"}
: ${CXXFLAGS="-Wall -g"}

AC_CONFIG_MACRO_DIR([m4])

AC_SUBST(PUBDATE)
AC_CONFIG_HEADER([src/config.h src/defs.h])

dnl Choose a default for the build_manual var. If the dist file is present in
dnl the root then default to no, otherwise go for it.
AC_CHECK_FILES( [$srcdir/DIST], 
	[. $srcdir/DIST;], 
	[build_manual=yes; ] )

dnl Set to true if the manual should be built.
AM_CONDITIONAL(BUILD_MANUAL, [test "x$build_manual" = "xyes"])

AC_CHECK_SIZEOF([long])

dnl Generic dependency specification.
AC_ARG_WITH(deps,
	[AC_HELP_STRING([--with-deps], [generic dependency location])],
	[DEPS="$withval"],
	[DEPS="/opt/colm"]
)

dnl Set to true if build system should generate parsers from ragel, kelbt, and
dnl gperf sources. Set to false if generated files are included and not to be
dnl built (production).
dnl AC_SUBST(BUILD_PARSERS,true)

dnl Checks for programs.
AC_PROG_CC
AC_PROG_CXX
AC_PROG_LIBTOOL

dnl Set test on c++ compiler.
AC_LANG_CPLUSPLUS

dnl Check for definition of MAKE.
AC_PROG_MAKE_SET

AC_ARG_ENABLE(pool-malloc, 
		AC_HELP_STRING([--enable-pool-malloc], [allocate pool objects with malloc]), 
		AC_DEFINE([POOL_MALLOC], [1], [allocate pool objects with malloc]))

AC_ARG_ENABLE(debug,
		AC_HELP_STRING([--enable-debug], [enable debug statements]), 
		AC_DEFINE([DEBUG], [1], [enable debug statements]))

AC_CHECK_PROG([ASCIIDOC], [asciidoc], [asciidoc])
AC_CHECK_PROG([PYGMENTIZE], [pygmentize], [pygmentize])

dnl write output files
AC_OUTPUT([
	Makefile
	src/Makefile
	aapl/Makefile
	doc/Makefile
])

echo "configuration of colm complete"
