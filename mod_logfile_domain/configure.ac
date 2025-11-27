AC_INIT([mod_logfile_domain], [1.0])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_CONFIG_MACRO_DIRS([m4])

AC_PROG_CC
AM_PROG_AR
LT_INIT

# Check for FreeSWITCH
AC_ARG_WITH([freeswitch],
  [AS_HELP_STRING([--with-freeswitch=PATH], [FreeSWITCH installation path (default: /usr)])],
  [FREESWITCH_PREFIX="$withval"],
  [FREESWITCH_PREFIX="/usr"]
)

# Try pkg-config first, fallback to direct paths
PKG_CHECK_MODULES([FREESWITCH], [freeswitch >= 1.8], [], [
  # Manual fallback if pkg-config doesn't work
  FREESWITCH_CFLAGS="-I${FREESWITCH_PREFIX}/include/freeswitch"
  FREESWITCH_LIBS="-L${FREESWITCH_PREFIX}/lib -lfreeswitch"
])

AC_SUBST([FREESWITCH_CFLAGS])
AC_SUBST([FREESWITCH_LIBS])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
