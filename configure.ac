#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([logwatcher], [0.4.7], [sperner@stud.fh-frankfurt.de])
AC_CONFIG_SRCDIR([source/main.c])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
: ${CFLAGS=""}
AC_PROG_CC

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h limits.h netinet/in.h stdlib.h string.h sys/socket.h sys/time.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.

# we want to use *automake*
AM_INIT_AUTOMAKE

# use source/ as source directory
AC_CONFIG_SRCDIR([source/main.c])

# generate a >Makefile<
AC_CONFIG_FILES([Makefile])

# check if debugging symbols are wanted
# (whether --enable-debug was given to >configure<)
AC_MSG_CHECKING([whether to build with debug information])
AC_ARG_ENABLE([debug],
    [AS_HELP_STRING([--enable-debug],[enable debug data generation (default=no)])],
    [want_debug="$enableval"],
    [want_debug=no])
AC_MSG_RESULT([$want_debug])
if test x"$want_debug" = x"yes"
then
    AC_DEFINE([DEBUG],[],[Debug Mode])
    CFLAGS="$CFLAGS -g -Wall -Werror -Wno-uninitialized -O0"
else
    AC_DEFINE([NDEBUG],[],[Release Mode])
    CFLAGS="$CFLAGS -O2"
fi

# check for pthread library
AC_CHECK_LIB([pthread], [pthread_mutex_init], [], [
    echo "pthread library is missing. pthread is required for this program"
    exit -1])

# check for header files
AC_CHECK_HEADERS([arpa/inet.h limits.h netinet/in.h pthread.h stdlib.h string.h sys/socket.h unistd.h])

# Checks for library functions.
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_CHECK_FUNCS([gettimeofday inet_ntoa socket strstr])

AC_OUTPUT
