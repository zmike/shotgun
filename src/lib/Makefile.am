lib_LTLIBRARIES = \
  libshotgun.la

AM_CFLAGS = \
  $(ELM_CFLAGS) \
  @ECORE_X_CFLAGS@ \
 -I@top_builddir@ \
 -I@top_srcdir@/src/include
AM_CXXFLAGS = $(AM_CFLAGS)

libshotgun_la_LIBADD = \
  @ELM_LIBS@ \
  @ECORE_X_LIBS@ \
  -lm

libshotgun_la_LDFLAGS = -version-info @version_info@ @release_info@ -no-undefined

libshotgun_la_SOURCES = \
cdecode.c \
cencode.c \
iq.c \
login.c \
messaging.c \
presence.c \
pugixml.cpp \
shotgun.c \
shotgun_utils.c \
xml.cpp

MAINTAINERCLEANFILES = Makefile.in
