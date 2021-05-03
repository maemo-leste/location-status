desktoplib_LTLIBRARIES = location-status.la
desktoplibdir = $(hildondesktoplibdir)

location_status_la_SOURCES = location-status.c

location_status_la_CFLAGS  = $(LS_CFLAGS) $(LL_CFLAGS)
location_status_la_LIBADD  = $(LS_LIBS)
location_status_la_LDFLAGS = -module -avoid-version -Wl,--as-needed

desktopdir = $(hildonhomedesktopdir)
desktop_DATA = location-status.desktop
