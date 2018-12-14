#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT(tcp-samples, 1.0, jiaru@ict.ac.cn)
AC_CONFIG_SRCDIR([src/echo-server.c])
AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE

AC_SUBST([TCPS_CFLAGS])
AC_SUBST([TCPS_LDFLAGS])

# Checks for programs.
AC_PROG_CC
AC_PROG_MKDIR_P
AC_PROG_RANLIB

# Checks for libraries.
AC_SEARCH_LIBS([pthread_create], [pthread])
AC_SEARCH_LIBS([dlopen], [dl])
AC_SEARCH_LIBS([pow], [m])


# Checks for header files.
AC_CHECK_HEADERS([stdint.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.

# Set default prefix to $(curdir)/build
TCPS_SET_DEFAULT_PREFIX

TCPS_CHECK_DEBUG

AC_CONFIG_FILES([Makefile])

AC_OUTPUT
