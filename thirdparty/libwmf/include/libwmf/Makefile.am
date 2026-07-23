HDRHEAVY = foreign.h eps.h fig.h svg.h gd.h x.h

if LIBWMF_BUILD_ALL
HDRBUILD = $(HDRHEAVY)
else
HDRBUILD = 
endif

wmfhdrdir = $(includedir)/libwmf

wmfhdr_HEADERS = api.h color.h defs.h fund.h ipa.h types.h macro.h font.h canvas.h $(HDRBUILD)

EXTRA_DIST = template.h plot.h $(HDRHEAVY)
