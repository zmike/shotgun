AM_CFLAGS = \
  @ELM_CFLAGS@ \
  @ECORE_X_CFLAGS@ \
  @edbus_CFLAGS@ \
  @enotify_CFLAGS@ \
 -DPACKAGE_DATA_DIR=\"$(datadir)/$(PACKAGE)\" \
 -DPACKAGE_LIB_DIR=\"$(libdir)\" \
 -I@top_builddir@ \
 -I@top_srcdir@/src/include

AM_CXXFLAGS = $(AM_CFLAGS)

bin_PROGRAMS = shotgun

shotgun_LDADD = \
  @ELM_LIBS@ \
  @ECORE_X_LIBS@ \
  @edbus_LIBS@ \
  @enotify_LIBS@ \
  -lm \
  @top_builddir@/src/lib/libshotgun.la

shotgun_SOURCES = \
chat.c \
chat_image.c \
contact.c \
contact_list.c \
dbus.c \
eet.c \
events.c \
getpass_x.c \
main.c \
sha1.c \
sha1wrap.c \
util.c

MAINTAINERCLEANFILES = Makefile.in
