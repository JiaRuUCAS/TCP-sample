# -*- autoconf -*-

dnl TCPS_SET_DEFAULT_PREFIX
AC_DEFUN([TCPS_SET_DEFAULT_PREFIX],[
	prefix=`pwd`/build
	if test -d "$prefix"; then
		AC_MSG_NOTICE([Set default prefix to $prefix])
	else
		mkdir -p "$prefix"
		AC_MSG_NOTICE([Set default prefix to $prefix])
	fi])

dnl Checks for --enable-debug and defines TCPS_DEBUG if it is specified.
AC_DEFUN([TCPS_CHECK_DEBUG],
	[AC_ARG_ENABLE(
		[debug],
		[AC_HELP_STRING([--enable-debug],
			[Enable debugging features])],
		[case "${enableval}" in
			(yes) debug=true ;;
			(no)  debug=false ;;
			(*) AC_MSG_ERROR([bad value ${enableval} for --enable-debug]) ;;
		esac],
		[debug=false])
	AM_CONDITIONAL([TCPS_DEBUG], [test x$debug = xtrue])
])

