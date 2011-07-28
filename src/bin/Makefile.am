AM_CFLAGS = \
  @ELM_CFLAGS@ \
  @ECORE_X_CFLAGS@ \
 -I@top_builddir@ \
 -I@top_srcdir@/src/include
AM_CXXFLAGS = $(AM_CFLAGS)

bin_PROGRAMS = shotgun

shotgun_LDADD = \
  @ELM_LIBS@ \
  @ECORE_X_LIBS@ \
  -lm \
  @top_builddir@/src/lib/libshotgun.la

shotgun_SOURCES = \
chat.c \
chat_image.c \
contact.c \
contact_list.c \
events.c \
getpass_x.c \
main.c

MAINTAINERCLEANFILES = Makefile.in
