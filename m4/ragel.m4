dnl Generic dependency specification.
AC_ARG_WITH(deps,
	[AC_HELP_STRING([--with-deps], [generic dependency location])],
	[DEPS="$withval"],
	[DEPS="/opt/colm"]
)

AC_DEFUN([AC_CHECK_COLM], [
	EXPECTED_VER=$1
	if test -z "$EXPECTED_VER"; then
		AC_ERROR( [check colm: expected version not passed in] )
	fi

	AC_ARG_WITH(colm,
		[AC_HELP_STRING([--with-colm], [location of colm install])],
		[
			CPPFLAGS="${CPPFLAGS} -I$withval/include"
			LDFLAGS="${LDFLAGS} -L$withval/lib"
			COLM="$withval/bin/colm"
		],
		[
			CPPFLAGS="${CPPFLAGS} -I$DEPS/include"
			LDFLAGS="${LDFLAGS} -L$DEPS/lib"
			COLM="$DEPS/bin/colm"
		]
	)

	AC_CHECK_FILES(
		[$COLM],
		[],
		[AC_ERROR([colm is required to build this package])]
	)
	AC_SUBST(COLM)

	INSTALLED_VER=`$COLM -v | sed -n -e '1 { s/^.*version //; s/ .*$//; p; }'`
	if test "x$INSTALLED_VER" != "x$EXPECTED_VER"; then
		AC_ERROR( [check colm: expected version $EXPECTED_VER, but $INSTALLED_VER is installed] )
	fi
])

