AC_PREREQ[(2.69)]
AC_INIT([location-status], [0.110], [parazyd@dyne.org])
AC_CONFIG_SRCDIR([src])
AM_INIT_AUTOMAKE
LT_INIT([shared disable-static])
AC_CONFIG_HEADER([config.h])
AC_CONFIG_MACRO_DIRS([m4])

AC_PROG_CC
AC_PROG_INSTALL
AM_PROG_LIBTOOL

PKG_CHECK_MODULES(LS, hildon-1 libhildondesktop-1 dbus-glib-1 dbus-1 libosso)
AC_SUBST(LS_CFLAGS)
AC_SUBST(LS_LIBS)

PKG_CHECK_MODULES(LL, liblocation)
AC_SUBST(LL_CFLAGS)

hildonhomedesktopdir="`$PKG_CONFIG --variable=hildonstatusmenudesktopentrydir libhildondesktop-1`"
AC_SUBST(hildonhomedesktopdir)

hildondesktoplibdir="`$PKG_CONFIG --variable=hildondesktoplibdir libhildondesktop-1`"
AC_SUBST(hildondesktoplibdir)

AC_OUTPUT([Makefile src/Makefile data/Makefile])
