desktoplib_LTLIBRARIES = liblocation.la
desktoplibdir = $(hildondesktoplibdir)

liblocation_la_SOURCES = location-status.c

liblocation_la_CFLAGS  = $(LS_CFLAGS) $(LL_CFLAGS)
liblocation_la_LIBADD  = $(LS_LIBS)
liblocation_la_LDFLAGS = -module -avoid-version -Wl,--as-needed
