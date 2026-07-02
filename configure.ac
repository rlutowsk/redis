AC_INIT([redis], [0.1], [rafal.lutowski@ug.edu.pl])
AC_CONFIG_SRCDIR([src/redis.c])
AC_CONFIG_MACRO_DIR([m4])
m4_include([m4/find_gap.m4])

AC_PROG_CC

FIND_GAP

# Checking the hiredis library
AC_CHECK_LIB([hiredis], [redisConnect],
             [AC_MSG_NOTICE([Found the hiredis library.])
              have_hiredis=yes],
             [AC_MSG_ERROR([The hiredis library was not found.])
              have_hiredis=no])

LDFLAGS="-lhiredis $LDFLAGS"
AC_SUBST([LDFLAGS])

# Variable storing the current folder
CURRENT_DIR="$PWD"
AC_SUBST([CURRENT_DIR])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
