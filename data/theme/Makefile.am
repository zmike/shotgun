MAINTAINERCLEANFILES = Makefile.in

EDJE_CC = @edje_cc@
EDJE_FLAGS = -v -id $(top_srcdir)/data/theme

filesdir = $(datadir)/$(PACKAGE)
files_DATA = default.edj

images = \
arrows_both.png \
arrows_pending_left.png \
arrows_pending_right.png \
arrows_rejected.png \
dialog_ok.png \
useradd.png \
userdel.png \
x.png

EXTRA_DIST = default.edc $(images)

default.edj: Makefile $(images) default.edc
        $(EDJE_CC) $(EDJE_FLAGS) \
        $(top_srcdir)/data/themes/default.edc \
        $(top_builddir)/data/themes/default.edj

clean-local:
        rm -f default.edj

