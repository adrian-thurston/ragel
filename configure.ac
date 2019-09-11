dnl 
dnl Copyright 2006-2018 Adrian Thurston <thurston@colm.net>
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
  
AC_INIT(colm, 0.13.0.7)
PUBDATE="May 2019"

AM_INIT_AUTOMAKE([foreign])

: ${CFLAGS="-Wall -g"}
: ${CXXFLAGS="-Wall -g"}

AC_CONFIG_MACRO_DIR([m4])

AC_SUBST(PUBDATE)
AC_CHECK_SIZEOF(void *)
AC_CHECK_SIZEOF(long)
AC_CHECK_SIZEOF(unsigned long)
AC_CHECK_SIZEOF(unsigned long long)
AC_CHECK_HEADERS([sys/mman.h sys/wait.h unistd.h])

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

SED_SUBST=["\
	-e 's|@CXX@|${CXX}|g' \
	-e 's|@CC@|${CC}|g' \
"]

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
AM_CONDITIONAL([BUILD_MANUAL], [test "x$ASCIIDOC" != x && test "x$PYGMENTIZE" != x])


dnl
dnl Build the program? ( no means just build libfsm )
dnl
AC_ARG_ENABLE(program,
	[AC_HELP_STRING([--enable-program], [build the program? no means libfsm only] )],
	[
		if test "x$enableval" = "xyes"; then
			build_program=yes;
		else
			build_program=no;
		fi
	],
	[
		build_program=yes;
	]
)

AM_CONDITIONAL([BUILD_PROGRAM],    [test "x$build_program" = "xyes"])
AM_CONDITIONAL([WITH_RAGEL_KELBT], [test "x$RAGEL_KELBT" = "xyes"])

dnl This is from ragel, but already have one above from colm. Need to merge.
dnl AM_CONDITIONAL([BUILD_MANUAL],     [test "x$build_manual" = "xyes"])

AC_CANONICAL_HOST()
AM_CONDITIONAL([LINKER_NO_UNDEFINED], [test "x$host_os" = "xlinux-gnu"])

#
# For building within the tree.
#
COLM_BIN='$(top_srcdir)/colm/colm'
COLM_LA='$(top_srcdir)/colm/libcolm.la'
COLM_LIBDEP="${COLM_LA}"
COLM_BINDEP="${COLM}"

SED_SUBST="$SED_SUBST -e 's|@COLM_BIN@|${COLM_BIN}|g'"
SED_SUBST="$SED_SUBST -e 's|@COLM_LA@|${COLM_LA}|g'"
SED_SUBST="$SED_SUBST -e 's|@COLM_LIBDEP@|${COLM_LIBDEP}|g'"
SED_SUBST="$SED_SUBST -e 's|@COLM_BINDEP@|${COLM_BINDEP}|g'"

AC_SUBST(COLM_BIN)
AC_SUBST(COLM_LA)
AC_SUBST(COLM_LIBDEP)
AC_SUBST(COLM_BINDEP)

dnl
dnl Testing
dnl
dnl Which installed target to test. Without this option supplied testing is
dnl done against what is in the source tree.
dnl
AC_ARG_WITH(subject,
	[AC_HELP_STRING([--with-subject], [location of install tree test (defaults source tree)])],
	[
		SUBJ_AAPL_CPPFLAGS="-I${withval}/include/aapl"

		SUBJ_COLM_BIN="${withval}/bin/colm"
		SUBJ_COLM_CPPFLAGS="-I${withval}/include"
		SUBJ_COLM_LDFLAGS="-L${withval}/lib -Wl,-rpath,${withval}/lib"

		SUBJ_RAGEL_BIN="$withval/bin/ragel"
		SUBJ_RAGEL_CPPFLAGS="-I$withval/include"
		SUBJ_RAGEL_LDFLAGS="-L$withval/lib -Wl,-rpath,${withval}/lib"
		SUBJ_RAGEL_LM="${withval}/share"

		SUBJ_RAGEL_C_BIN="$withval/bin/ragel-c"
		SUBJ_RAGEL_D_BIN="$withval/bin/ragel-d"
		SUBJ_RAGEL_JAVA_BIN="$withval/bin/ragel-java"
		SUBJ_RAGEL_RUBY_BIN="$withval/bin/ragel-ruby"
		SUBJ_RAGEL_CSHARP_BIN="$withval/bin/ragel-csharp"
		SUBJ_RAGEL_GO_BIN="$withval/bin/ragel-go"
		SUBJ_RAGEL_OCAML_BIN="$withval/bin/ragel-ocaml"
		SUBJ_RAGEL_ASM_BIN="$withval/bin/ragel-asm"
		SUBJ_RAGEL_RUST_BIN="$withval/bin/ragel-rust"
		SUBJ_RAGEL_CRACK_BIN="$withval/bin/ragel-crack"
		SUBJ_RAGEL_JULIA_BIN="$withval/bin/ragel-julia"
	],
	[
		SUBJ_AAPL_CPPFLAGS='-I$(abs_top_builddir)/aapl'

		SUBJ_COLM_BIN='$(abs_top_builddir)/colm/colm'
		SUBJ_COLM_CPPFLAGS='-I$(abs_top_builddir)/colm/include'
		SUBJ_COLM_LDFLAGS='-L$(abs_top_builddir)/colm/.libs -Wl,-rpath,${abs_top_builddir}/colm/.libs'

		SUBJ_RAGEL_BIN='$(abs_top_builddir)/ragel/ragel'
		SUBJ_RAGEL_CPPFLAGS='-I$(abs_top_builddir)/ragel/include -I$(abs_top_builddir)/aapl'
		SUBJ_RAGEL_LDFLAGS='-L$(abs_top_builddir)/ragel/.libs -Wl,-rpath,$(abs_top_builddir)/ragel/.libs'
		SUBJ_RAGEL_LM='$(abs_top_builddir)/ragel'

		SUBJ_RAGEL_C_BIN='$(abs_top_builddir)/ragel/host-c/ragel-c'
		SUBJ_RAGEL_D_BIN='$(abs_top_builddir)/ragel/host-d/ragel-d'
		SUBJ_RAGEL_JAVA_BIN='$(abs_top_builddir)/ragel/host-java/ragel-java'
		SUBJ_RAGEL_RUBY_BIN='$(abs_top_builddir)/ragel/host-ruby/ragel-ruby'
		SUBJ_RAGEL_CSHARP_BIN='$(abs_top_builddir)/ragel/host-csharp/ragel-csharp'
		SUBJ_RAGEL_GO_BIN='$(abs_top_builddir)/ragel/host-go/ragel-go'
		SUBJ_RAGEL_OCAML_BIN='$(abs_top_builddir)/ragel/host-ocaml/ragel-ocaml'
		SUBJ_RAGEL_ASM_BIN='$(abs_top_builddir)/ragel/host-asm/ragel-asm'
		SUBJ_RAGEL_RUST_BIN='$(abs_top_builddir)/ragel/host-rust/ragel-rust'
		SUBJ_RAGEL_CRACK_BIN='$(abs_top_builddir)/ragel/host-crack/ragel-crack'
		SUBJ_RAGEL_JULIA_BIN='$(abs_top_builddir)/ragel/host-julia/ragel-julia'
	]
)

SED_SUBST="$SED_SUBST -e 's|@SUBJ_COLM_BIN@|${SUBJ_COLM_BIN}|g'"
SED_SUBST="$SED_SUBST -e 's|@SUBJ_COLM_CPPFLAGS@|${SUBJ_COLM_CPPFLAGS}|g'"
SED_SUBST="$SED_SUBST -e 's|@SUBJ_COLM_LDFLAGS@|${SUBJ_COLM_LDFLAGS}|g'"
SED_SUBST="$SED_SUBST -e 's|@SUBJ_RAGEL_BIN@|${SUBJ_RAGEL_BIN}|g'"
SED_SUBST="$SED_SUBST -e 's|@SUBJ_RAGEL_CPPFLAGS@|${SUBJ_RAGEL_CPPFLAGS}|g'"
SED_SUBST="$SED_SUBST -e 's|@SUBJ_RAGEL_LDFLAGS@|${SUBJ_RAGEL_LDFLAGS}|g'"
SED_SUBST="$SED_SUBST -e 's|@SUBJ_RAGEL_LM@|${SUBJ_RAGEL_LM}|g'"
SED_SUBST="$SED_SUBST -e 's|@SUBJ_RAGEL_C_BIN@|${SUBJ_RAGEL_C_BIN}|g'"
SED_SUBST="$SED_SUBST -e 's|@SUBJ_RAGEL_D_BIN@|${SUBJ_RAGEL_D_BIN}|g'"
SED_SUBST="$SED_SUBST -e 's|@SUBJ_RAGEL_JAVA_BIN@|${SUBJ_RAGEL_JAVA_BIN}|g'"
SED_SUBST="$SED_SUBST -e 's|@SUBJ_RAGEL_RUBY_BIN@|${SUBJ_RAGEL_RUBY_BIN}|g'"
SED_SUBST="$SED_SUBST -e 's|@SUBJ_RAGEL_CSHARP_BIN@|${SUBJ_RAGEL_CSHARP_BIN}|g'"
SED_SUBST="$SED_SUBST -e 's|@SUBJ_RAGEL_GO_BIN@|${SUBJ_RAGEL_GO_BIN}|g'"
SED_SUBST="$SED_SUBST -e 's|@SUBJ_RAGEL_OCAML_BIN@|${SUBJ_RAGEL_OCAML_BIN}|g'"
SED_SUBST="$SED_SUBST -e 's|@SUBJ_RAGEL_ASM_BIN@|${SUBJ_RAGEL_ASM_BIN}|g'"
SED_SUBST="$SED_SUBST -e 's|@SUBJ_RAGEL_RUST_BIN@|${SUBJ_RAGEL_RUST_BIN}|g'"
SED_SUBST="$SED_SUBST -e 's|@SUBJ_RAGEL_CRACK_BIN@|${SUBJ_RAGEL_CRACK_BIN}|g'"
SED_SUBST="$SED_SUBST -e 's|@SUBJ_RAGEL_JULIA_BIN@|${SUBJ_RAGEL_JULIA_BIN}|g'"

AC_SUBST(SUBJ_AAPL_CPPFLAGS)

AC_SUBST(SUBJ_COLM_BIN)
AC_SUBST(SUBJ_COLM_CPPFLAGS)
AC_SUBST(SUBJ_COLM_LDFLAGS)

AC_SUBST(SUBJ_RAGEL_BIN)
AC_SUBST(SUBJ_RAGEL_CPPFLAGS)
AC_SUBST(SUBJ_RAGEL_LDFLAGS)
AC_SUBST(SUBJ_RAGEL_LM)

dnl
dnl Installed programs to test in 
dnl

AC_PATH_PROG([D_BIN], [gdc-5])
AC_PATH_PROG([JAVAC_BIN], [javac])
AC_PATH_PROG([RUBY_BIN], [ruby])
AC_PATH_PROG([CSHARP_BIN], [mcs])
AC_PATH_PROG([GO_BIN], [go])
AC_PATH_PROG([OCAML_BIN], [ocaml])
AC_PATH_PROG([RUST_BIN], [rustc])
AC_PATH_PROG([JULIA_BIN], [julia])

SED_SUBST="$SED_SUBST -e 's|@D_BIN@|${D_BIN}|g'"
SED_SUBST="$SED_SUBST -e 's|@JAVAC_BIN@|${JAVAC_BIN}|g'"
SED_SUBST="$SED_SUBST -e 's|@RUBY_BIN@|${RUBY_BIN}|g'"
SED_SUBST="$SED_SUBST -e 's|@CSHARP_BIN@|${CSHARP_BIN}|g'"
SED_SUBST="$SED_SUBST -e 's|@GO_BIN@|${GO_BIN}|g'"
SED_SUBST="$SED_SUBST -e 's|@OCAML_BIN@|${OCAML_BIN}|g'"
SED_SUBST="$SED_SUBST -e 's|@RUST_BIN@|${RUST_BIN}|g'"
SED_SUBST="$SED_SUBST -e 's|@JULIA_BIN@|${JULIA_BIN}|g'"

AC_ARG_WITH(crack,
	[AC_HELP_STRING([--with-crack], [location of crack install])],
	[CRACK_BIN="$withval/bin/crack"],
	[AC_PATH_PROG([CRACK_BIN], [crack])]
)
SED_SUBST="$SED_SUBST -e 's|@CRACK_BIN@|${CRACK_BIN}|g'"

AC_SUBST(CRACK_BIN)
	

AC_SUBST(SED_SUBST)

AC_CONFIG_HEADERS([src/config.h colm/defs.h])

dnl write output files
AC_OUTPUT([
	Makefile
	aapl/Makefile
	colm/Makefile
	ragel/Makefile
	ragel/host-ruby/Makefile
	ragel/host-asm/Makefile
	ragel/host-julia/Makefile
	ragel/host-ocaml/Makefile
	ragel/host-c/Makefile
	ragel/host-d/Makefile
	ragel/host-csharp/Makefile
	ragel/host-go/Makefile
	ragel/host-java/Makefile
	ragel/host-rust/Makefile
	ragel/host-crack/Makefile
	ragel/host-js/Makefile
	test/Makefile
	test/aapl.d/Makefile
	test/colm.d/Makefile
	test/ragel.d/Makefile
	doc/Makefile
])

echo "configuration of colm complete"
