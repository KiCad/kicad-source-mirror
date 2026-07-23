AM_CPPFLAGS += -I$(srcdir)

DEFS = @DEFS@ @GD_DEFS@

GDHDRS = gd.h gdcache.h gd_clip.h gd_io.h gdfontg.h gdfontl.h gdfontmb.h gdfonts.h gdfontt.h

if LIBWMF_OPT_SYS_GD
LIBGD =
HDRGDINST =
else
LIBGD = libgd.la
HDRGDINST = $(GDHDRS)
endif

noinst_LTLIBRARIES = $(LIBGD)

libgd_la_SOURCES = gd.c gd_gd.c gd_gd2.c gd_io.c gd_io_dp.c \
		gd_io_file.c gd_ss.c gd_io_ss.c gd_png.c gd_jpeg.c gdxpm.c \
		gdfontt.c gdfonts.c gdfontmb.c gdfontl.c gdfontg.c \
		gdtables.c gdft.c gdcache.c gdkanji.c wbmp.c \
		gd_wbmp.c gdhelpers.c gd_clip.c

gddir = $(includedir)/libwmf/gd

gd_HEADERS = $(HDRGDINST)

GDOTHER = \
	Makefile.gd \
	Makefile.nt \
	README-JPEG.TXT \
	Readme.wmf \
	alphachanneltest.html \
	arc \
	bdftogd \
	bresenham_ellipse \
	errs \
	gd2copypal.c \
	gdhelpers.h \
	gd2time.c \
	gd2topng.c \
	gd_arc_f_buggy.c \
	gddemo.c \
	gdparttopng.c \
	gdtest.c \
	gdtestft.c \
	gdtopng.c \
	index.html \
	install-item \
	jisx0208.h \
	mathmake.c \
	pngtogd.c \
	pngtogd2.c \
	readme.txt \
	tc \
	testac.c \
	wbmp.h \
	webpng.c

EXTRA_DIST = Makefile.gd Makefile.nt Readme.wmf $(GDHDRS) $(GDOTHER)
