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

dnl TCPS_CHECK_DPDK
AC_DEFUN([TCPS_CHECK_DPDK],[
	AC_ARG_WITH([dpdk],
				[AC_HELP_STRING([--with-dpdk=/path/to/dpdk],
								[Specify the DPDK build directroy])],
				[dpdk_spec=true])

	DPDK_AUTOLD=false
	AC_MSG_CHECKING([Whether dpdk datapath is specified])
	if test "$dpdk_spec" != true || test "$with_dpdk" = no; then
		AC_MSG_RESULT([no])
		if test -n "$RTE_SDK" && test -n "$RTE_TARGET"; then
			DPDK_PATH="$RTE_SDK/$RTE_TARGET"
			AC_MSG_NOTICE([RTE_SDK and RTE_TARGET exist, set dpdkpath to $DPDK_PATH])
			DPDK_INCLUDE="$DPDK_PATH/include"
			DPDK_LIB="$DPDK_PATH/lib"
		else
			AC_MSG_NOTICE([Set dpdk path to /usr/local/])
			DPDK_INCLUDE=/usr/local/include/dpdk
			DPDK_LIB=/usr/local/lib
			DPDK_AUTOLD=true
		fi
	else
		AC_MSG_RESULT([yes])
		DPDK_PATH="$with_dpdk"
		DPDK_INCLUDE="$DPDK_PATH/include"
		DPDK_LIB="$DPDK_PATH/lib"
	fi

	dpdk_set=true
	AC_MSG_CHECKING([Check whether dpdk headers are installed])
	if test -f "$DPDK_INCLUDE/rte_config.h"; then
		AC_MSG_RESULT([yes])
	else
		if test -f "$DPDK_INCLUDE/dpdk/rte_config.h"; then
			AC_MSG_RESULT([yes])
			DPDK_INCLIDE="$DPDK_INCLUDE/dpdk"
		else
			AC_MSG_RESULT([no])
			AC_MSG_NOTICE([No dpdk headers found, DPDK-based applications will not be compilied])
			dpdk_set=false
		fi
	fi

	if test "$dpdk_set" = "true"; then
		AC_MSG_CHECKING([Check whether dpdk libraries are installed])
		if test -f "$DPDK_LIB/libdpdk.a"; then
			AC_MSG_RESULT([yes])
		else
			AC_MSG_RESULT([no])
			AC_MSG_NOTICE([No dpdk libraries found, DPDK-based applications will not be compilied])
			dpdk_set=false
		fi
	fi

	if test "$dpdk_set" = "true"; then
		TCPS_CFLAGS="$TCPS_CFLAGS -I$DPDK_INCLUDE -mssse3"
		if test "$DPDK_AUTOLD" = "false"; then
			TCPS_LDFLAGS="$TCPS_LDFLAGS -L${DPDK_LIB}"
		fi
		DPDK_LDFLAGS="-Wl,--whole-archive,-ldpdk,--no-whole-archive"
		AC_SUBST([DPDK_LDFLAGS])
	fi
	AM_CONDITIONAL([TCPS_DPDK], [test x$dpdk_set = xtrue])
])

dnl TCPS_CHECK_MTCP
AC_DEFUN([TCPS_CHECK_MTCP],[
	AC_ARG_WITH([mtcp],
				[AC_HELP_STRING([--with-mtcp=/path/to/mtcp],
								[Specify the MTCP build directroy])],
				[mtcp_spec=true])

	MTCP_AUTOLD=false
	AC_MSG_CHECKING([Whether mtcp datapath is specified])
	if test "$mtcp_spec" != true || test "$with_mtcp" = no; then
		AC_MSG_RESULT([no])
		AC_MSG_NOTICE([Set mtcp path to /usr/local/])
		MTCP_INCLUDE=/usr/local/include/mtcp
		MTCP_LIB=/usr/local/lib
		MTCP_AUTOLD=true
	else
		AC_MSG_RESULT([yes])
		MTCP_PATH="$with_mtcp"
		MTCP_INCLUDE="$MTCP_PATH/include"
		MTCP_LIB="$MTCP_PATH/lib"
	fi

	mtcp_set=true
	AC_MSG_CHECKING([Check whether mtcp headers are installed])
	if test -f "$MTCP_INCLUDE/mtcp_api.h" && test -f "$MTCP_INCLUDE/mtcp_epoll.h"; then
		AC_MSG_RESULT([yes])
	else
		AC_MSG_RESULT([no])
		AC_MSG_NOTICE([No mtcp headers found, MTCP-based applications will not be compilied])
		mtcp_set=false
	fi

	if test "$mtcp_set" = "true"; then
		AC_MSG_CHECKING([Check whether mtcp libraries are installed])
		if test -f "$MTCP_LIB/libmtcp.a"; then
			AC_MSG_RESULT([yes])
		else
			AC_MSG_RESULT([no])
			AC_MSG_NOTICE([No mtcp libraries found, MTCP-based applications will not be compiled])
			mtcp_set=false
		fi
	fi

	if test "$mtcp_set" = "true"; then
		TCPS_CFLAGS="$TCPS_CFLAGS -I$MTCP_INCLUDE"
		if test "$MTCP_AUTOLD" = "false"; then
			TCPS_LDFLAGS="$TCPS_LDFLAGS -L${MTCP_LIB}"
		fi
		MTCP_LDFLAGS="-Wl,--whole-archive,-lmtcp,--no-whole-archive -lgmp -lnuma"
		AC_SUBST([MTCP_LDFLAGS])
	fi
	AM_CONDITIONAL([TCPS_MTCP], [test x$mtcp_set = xtrue])
])
