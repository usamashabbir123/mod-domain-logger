AC_INIT([mod_logfile_domain], [1.0], [support@example.com])
AM_INIT_AUTOMAKE([-Wall -Werror foreign subdir-objects])

AC_PROG_CC
AC_PROG_LIBTOOL

# Check for FreeSWITCH
PKG_CHECK_MODULES([FREESWITCH], [freeswitch], [], [
  # Fallback if pkg-config doesn't find it
  AC_ARG_WITH([freeswitch],
    [AS_HELP_STRING([--with-freeswitch=PATH], [FreeSWITCH installation path])],
    [
      FREESWITCH_CFLAGS="-I$withval/include/freeswitch"
      FREESWITCH_LIBS="-L$withval/lib -lfreeswitch"
    ],
    [
      # Default FreeSWITCH paths
      FREESWITCH_CFLAGS="-I/usr/include/freeswitch"
      FREESWITCH_LIBS="-L/usr/lib -lfreeswitch"
    ]
  )
])

AC_SUBST([FREESWITCH_CFLAGS])
AC_SUBST([FREESWITCH_LIBS])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile])

AC_OUTPUT
