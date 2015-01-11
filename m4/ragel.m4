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

			AC_CHECK_FILES(
				[$COLM],
				[],
				[AC_ERROR([--with-colm was specified, but binary was not found])]
			)

			INSTALLED_VER=`$COLM -v | sed -n -e '1 { s/^.*version //; s/ .*$//; p; }'`
			if test "x$INSTALLED_VER" != "x$EXPECTED_VER"; then
				AC_ERROR( [check colm: expected version $EXPECTED_VER,]
						[ but $INSTALLED_VER is installed] )
			fi

			AC_DEFINE([WITH_COLM], [1], [colm is available])
		],
		[]
	)

	AC_SUBST(COLM)
	AM_CONDITIONAL([WITH_COLM], [test x$COLM != x])
])

