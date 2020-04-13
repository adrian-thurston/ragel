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
  
AC_INIT(ragel, 7.0)

RAGEL_VERSION="7.0"
RAGEL_PUBDATE="January 2020"

COLM_VERSION="0.14"
COLM_PUBDATE="January 2020"

AM_INIT_AUTOMAKE([foreign])

: ${CFLAGS="-Wall -g"}
: ${CXXFLAGS="-Wall -g"}

AC_CONFIG_MACRO_DIR([m4])

AC_SUBST(PUBDATE)
AC_SUBST(COLM_VERSION)
AC_SUBST(COLM_PUBDATE)
AC_SUBST(RAGEL_VERSION)
AC_SUBST(RAGEL_PUBDATE)
AC_CHECK_SIZEOF([int])
AC_CHECK_SIZEOF([long])
AC_CHECK_SIZEOF([void *])
AC_CHECK_SIZEOF([long])
AC_CHECK_SIZEOF([unsigned long])
AC_CHECK_SIZEOF([unsigned long long])
AC_CHECK_HEADERS([sys/mman.h sys/wait.h unistd.h])

AC_ARG_WITH(colm,
	[AC_HELP_STRING([--with-colm], [location of colm install])],
	[
		COLM="$withval/bin/colm"
		COLM_WRAP="$withval/bin/colm-wrap"
		CPPFLAGS="-I$withval/include ${CPPFLAGS}"
		CPPFLAGS="-I$withval/include/aapl ${CPPFLAGS}"
		LDFLAGS="-L$withval/lib ${LDFLAGS}"
		LIBCOLM_LA="$withval/lib/libcolm.la"
		LIBFSM_LA="$withval/lib/libfsm.la"
		COLM_SHARE="$withval/share"
	],
	[]
)

AC_CHECK_FILES(
	[$COLM],
	[],
	[AC_ERROR([colm is required to build ragel])]
)
AC_SUBST(COLM)
AC_SUBST(COLM_WRAP)
AC_SUBST(COLM_SHARE)
AC_SUBST(LIBCOLM_LA)
AC_SUBST(LIBFSM_LA)

EXPECTED_COLM_VER=0.14
INSTALLED_COLM_VER=`$COLM -v | sed -n -e '1 {' -e 's/^.*version //' -e 's/ .*$//' -e p -e '}'`
if test "x$INSTALLED_COLM_VER" != "x$EXPECTED_COLM_VER"; then
	AC_ERROR( [check colm: expected version $EXPECTED_COLM_VER, but $INSTALLED_COLM_VER is installed] )
fi

dnl Set to true if build system should generate parsers from ragel, kelbt, and
dnl gperf sources. Set to false if generated files are included and not to be
dnl built (production).
dnl AC_SUBST(BUILD_PARSERS,true)

dnl Checks for programs.
AC_PROG_CC
AC_PROG_CXX
AC_CHECK_TOOL(AR, ar)
AC_PROG_RANLIB
AC_PROG_LIBTOOL

SED_SUBST=["\
	-e 's|@CXX@|${CXX}|g' \
	-e 's|@CC@|${CC}|g' \
"]

SED_SUBST="$SED_SUBST -e 's|@COLM_SHARE@|${COLM_SHARE}|g'"

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

dnl Choose a default for the build_manual var. If the dist file is present in
dnl the root then default to no, otherwise go for it.
AC_CHECK_FILES( [$srcdir/DIST], 
	[. $srcdir/DIST;], 
	[build_manual=yes; ] )

dnl
dnl Enable arg to explicitly control the building of the manual
dnl
AC_ARG_ENABLE(manual,
	[  --enable-manual         do we want to build the manual?],
	[
		if test "x$enableval" = "xyes"; then
			build_manual=yes;
		else
			build_manual=no;
		fi
	],
)

dnl Checks to carry out if we are building the manual.
if test "x$build_manual" = "xyes"; then
	AC_CHECK_PROG(FIG2DEV, fig2dev, fig2dev)
	if test -z "$FIG2DEV"; then
		echo
		echo "error: fig2dev is required to build the manual (maybe use --disable-manual)"
		echo
		exit 1
	fi

	AC_CHECK_PROG(PDFLATEX, pdflatex, pdflatex)
	if test -z "$PDFLATEX"; then
		echo
		echo "error: pdflatex is required to build the manual (maybe use --disable-manual)"
		echo
		exit 1
	fi
fi

dnl Set to true if the manual should be built.
AM_CONDITIONAL(BUILD_MANUAL, [test "x$build_manual" = "xyes"])

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

AC_PATH_PROG([D_BIN], [gdc-8])
if test "x$D_BIN" = x; then
	AC_PATH_PROG([D_BIN], [gdc-7])
	if test "x$D_BIN" = x; then
		AC_PATH_PROG([D_BIN], [gdc-6])
		if test "x$D_BIN" = x; then
			AC_PATH_PROG([D_BIN], [gdc-5])
		fi
	fi
fi

AC_PATH_PROG([JAVAC_BIN], [javac])

dnl
dnl Java test cases are now requring a large virtual address space. On systems
dnl where this is limited by ulimit, java will fail to run.
dnl

if test "x$JAVAC_BIN" != x; then
	echo 'class conftest { public static void main( String[] args ) { } }' > conftest.java
	echo -n "checking if javac is able to compile programs ... "
	if sh -c "$JAVAC_BIN conftest.java" >>config.log 2>&1; then
		echo "yes"
	else
		echo "no"
		JAVAC_BIN=""
	fi
	rm -f conftest.java conftest.class
fi

AC_PATH_PROG([RUBY_BIN], [ruby])
AC_PATH_PROG([CSHARP_BIN], [mcs])
AC_PATH_PROG([GO_BIN], [go])
AC_PATH_PROG([OCAML_BIN], [ocaml])
AC_PATH_PROG([RUST_BIN], [rustc])
AC_PATH_PROG([JULIA_BIN], [julia])

dnl
dnl Julia requires a large virtual address space. On systems where this is
dnl limited by ulimit, julia will fail to run.
dnl

if test "x$JULIA_BIN" != x; then
	echo 'println( "can run julia programs" );' > conftest.jl
	echo -n "checking if julia is able to run programs ... "
	if sh -c "$JULIA_BIN conftest.jl" >>config.log 2>&1; then
		echo "yes"
	else
		echo "no"
		JULIA_BIN=""
	fi
	rm -f conftest.jl
fi

dnl We can run julia, now make sure we have 1.0
if test "x$JULIA_BIN" != x; then
	echo -n "checking if julia is version 1.0 or later ... "

	dnl We assume the form "julia version X.X.X"
	dnl   1. everything before the version number.
	dnl   2. Remove trailing version.
	dnl   3. Verify it is plain number greater than 1.
	dnl Any failure comes out as empty string. Note the sed command is wrapped
	dnl in [] so autotools do not interpret anything in it.
	JULIA1="`$JULIA_BIN -v | sed ['s/[A-Za-z ]\+//g; s/\.[0-9\.]*//; /^[1-9][0-9]*/!d;']`"
	if test "x$JULIA1" != x; then
		echo "yes"
	else
		JULIA_BIN=""
		echo "no"
	fi
fi

AC_PATH_PROG([GNUSTEP_CONFIG], [gnustep-config])

dnl
dnl The ASM tests have been based off of sources that are non-relocatable
dnl These won't assemble on some systems (with the right architecture). Check
dnl our specific flavour of ASM now so we can avoid some test failures.
dnl

ASM_BIN=$CC

cat <<\EOF > conftest.s
.section .rodata
.L_works:
.string "works"

.text
.globl	main
main:
	pushq	%rbp
	movq	%rsp, %rbp
	pushq	%rdi
	movq	$.L_works, %rdi
	call	puts
	popq	%rdi
	mov	$0, %rax
	ret
EOF

echo -n "checking if ragel ASM tests will build ... "
if sh -c "$ASM_BIN -o conftest.bin conftest.s" >>config.log 2>&1; then
	echo "yes"
else
	echo "no"
	ASM_BIN=""
fi
rm -f conftest.s

SED_SUBST="$SED_SUBST -e 's|@D_BIN@|${D_BIN}|g'"
SED_SUBST="$SED_SUBST -e 's|@JAVAC_BIN@|${JAVAC_BIN}|g'"
SED_SUBST="$SED_SUBST -e 's|@RUBY_BIN@|${RUBY_BIN}|g'"
SED_SUBST="$SED_SUBST -e 's|@CSHARP_BIN@|${CSHARP_BIN}|g'"
SED_SUBST="$SED_SUBST -e 's|@GO_BIN@|${GO_BIN}|g'"
SED_SUBST="$SED_SUBST -e 's|@OCAML_BIN@|${OCAML_BIN}|g'"
SED_SUBST="$SED_SUBST -e 's|@RUST_BIN@|${RUST_BIN}|g'"
SED_SUBST="$SED_SUBST -e 's|@JULIA_BIN@|${JULIA_BIN}|g'"
SED_SUBST="$SED_SUBST -e 's|@GNUSTEP_CONFIG@|${GNUSTEP_CONFIG}|g'"
SED_SUBST="$SED_SUBST -e 's|@ASM_BIN@|${ASM_BIN}|g'"

AC_ARG_WITH(crack,
	[AC_HELP_STRING([--with-crack], [location of crack install])],
	[CRACK_BIN="$withval/bin/crack"],
	[AC_PATH_PROG([CRACK_BIN], [crack])]
)
SED_SUBST="$SED_SUBST -e 's|@CRACK_BIN@|${CRACK_BIN}|g'"

AC_SUBST(CRACK_BIN)
	
dnl Skip bootstrap process and build using an existing version of colm. Allows us to
dnl break colm without breaking the build of colm.
dnl 
dnl WARNING: DO NOT USE FOR PRODUCTION BUILD
dnl
dnl Only use this when you need to in order to get colm to build. Using an external
dnl colm program (and library) has the consequence that parsing of patterns and
dnl constructors in colm programs will also use this external library, since there
dnl can only be one set of colm functions linked. It's not possible to use the
dnl external parser for parsing the colm program, then use the internal parser for
dnl the patterns.
AC_ARG_WITH(colm,
	[AC_HELP_STRING([--with-colm], [prefix of colm install to build with, instead of bootstrapping])],
	[
		EXTERNAL_COLM="$withval"
		EXTERNAL_INC="-I$withval/include"
		EXTERNAL_LIBS="-L$withval/lib"
		AC_CHECK_FILES(["$EXTERNAL_COLM/bin/colm"], [],
			[AC_ERROR(["could not find $EXTERNAL_COLM/bin/colm"])])
	],
	[]
)

AM_CONDITIONAL([EXTERNAL_COLM], [test "x$EXTERNAL_COLM" != x])
AC_SUBST(EXTERNAL_COLM)
AC_SUBST(EXTERNAL_INC)
AC_SUBST(EXTERNAL_LIBS)

dnl
dnl Wrap up.
dnl

AC_SUBST(SED_SUBST)
AC_CONFIG_HEADERS([src/config.h])
AC_OUTPUT([
	Makefile
	src/Makefile
	src/host-ruby/Makefile
	src/host-asm/Makefile
	src/host-julia/Makefile
	src/host-ocaml/Makefile
	src/host-c/Makefile
	src/host-d/Makefile
	src/host-csharp/Makefile
	src/host-go/Makefile
	src/host-java/Makefile
	src/host-rust/Makefile
	src/host-crack/Makefile
	src/host-js/Makefile
	test/Makefile
	test/ragel.d/Makefile
	doc/Makefile
	doc/ragel/Makefile
])

echo "configuration of colm complete"
