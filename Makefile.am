AUTOMAKE_OPTIONS = foreign
ACLOCAL_AMFLAGS  = -I m4

SUBDIRS = data src

desktoplibdir = $(hildondesktoplibdir)

install-exec-hook:
	find $(DESTDIR)$(desktoplibdir) -type f -name \*.la -delete
