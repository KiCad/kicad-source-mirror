#Depending on your system and the libraries and features you have
#and want, you WILL need to modify this Makefile!

#If you do not have gcc, change the setting for COMPILER, but you must
#use an ANSI standard C compiler (NOT the old SunOS 4.1.3 cc
#compiler; get gcc if you are still using that). 
COMPILER=gcc

#If the ar command fails on your system, consult the ar manpage
#for your system. 
AR=ar

#Typical configuration: support for PNG images, JPEG images, and FreeType text.
#Remove -DHAVE_LIBFREETYPE if you can live without FreeType text.
#Add -DHAVE_XPM if you have X and xpm installed and you want that feature.

CFLAGS=-g -DHAVE_LIBPNG -DHAVE_LIBJPEG -DHAVE_LIBFREETYPE

#PLEASE NOTE: YOU MAY HAVE TO JUGGLE THE ORDER OF THE LIBRARIES.
#Some systems are very picky about link order. They don't all agree
#on the right order, either. 
#
#Best for most users. If you don't have FreeType, remove -lfreetype.
#Add -lxpm if you need XPM support.

LIBS=-lgd -lpng -lz -ljpeg -lfreetype -lm

#Typical install locations for freetype 2.0, zlib, xpm, libjpeg 
#and libpng header files. If yours are somewhere else, change this. 
#-I. is important to ensure that the version of gd you are installing 
#is used, and not an older release in your directory tree somewhere.

INCLUDEDIRS=-I. -I/usr/include/freetype2 -I/usr/include/X11 -I/usr/X11R6/include/X11 -I/usr/local/include 

#Typical install locations for freetype, zlib, xpm and libpng libraries.
#If yours are somewhere else, other than a standard location
#such as /lib or /usr/lib, then change this. This line shouldn't hurt 
#if you don't actually have some of the optional libraries and directories.
LIBDIRS=-L/usr/local/lib -L/usr/lib/X11 -L/usr/X11R6/lib

#Location where libgd.so should be installed by "make install".
#THIS MUST BE ONE OF YOUR STANDARD SHARED LIBRARY LOCATIONS, unless
#you add a new directory to your LD_LIBRARY_PATH environment setting.
#Otherwise applications will NOT find libgd.so and will produce an 
#error.  
INSTALL_LIB=/usr/lib

#Location where .h files should be installed by "make install".
INSTALL_INCLUDE=/usr/include

#Location where useful non-test programs should be installed by "make install".
INSTALL_BIN=/usr/local/bin

#
#
# Changes should not be required below here.
#
#

# Update these with each release!

MAJOR_VERSION=2
VERSION=2.0.0

CC=$(COMPILER) $(INCLUDEDIRS)
LINK=$(CC) $(LIBDIRS) $(LIBS)

PROGRAMS=$(BIN_PROGRAMS) $(TEST_PROGRAMS)

BIN_PROGRAMS=pngtogd pngtogd2 gdtopng gd2topng gd2copypal gdparttopng webpng
TEST_PROGRAMS=gdtest gddemo gd2time gdtestft testac

default: instructions

instructions:
	@echo First, edit this Makefile. Read the comments at
	@echo the beginning thoroughly.
	@echo
	@echo Second, type \'make install\' as root.
	@echo
	@echo This installs the GD ${VERSION} shared library,
	@echo which is required in order to use the included
	@echo utility programs, such as webpng, pngtogd, etc.
	@echo 
	@echo OPTIONAL third step: type \'make test\' to build 
	@echo the optional test programs. Type \'make install\' FIRST.

test: $(TEST_PROGRAMS)

install: libgd.so.${VERSION} $(BIN_PROGRAMS)
	sh ./install-item 755 pngtogd $(INSTALL_BIN)/pngtogd
	sh ./install-item 755 pngtogd2 $(INSTALL_BIN)/pngtogd2
	sh ./install-item 755 gdtopng $(INSTALL_BIN)/gdtopng
	sh ./install-item 755 gd2topng $(INSTALL_BIN)/gd2topng
	sh ./install-item 755 gd2copypal $(INSTALL_BIN)/gd2copypal
	sh ./install-item 755 gdparttopng $(INSTALL_BIN)/gdparttopng
	sh ./install-item 755 webpng $(INSTALL_BIN)/webpng
	sh ./install-item 755 bdftogd $(INSTALL_BIN)/bdftogd
	sh ./install-item 644 gd.h $(INSTALL_INCLUDE)/gd.h
	sh ./install-item 644 gdcache.h $(INSTALL_INCLUDE)/gdcache.h
	sh ./install-item 644 gd_io.h $(INSTALL_INCLUDE)/gd_io.h
	sh ./install-item 644 gdfontg.h $(INSTALL_INCLUDE)/gdfontg.h
	sh ./install-item 644 gdfontl.h $(INSTALL_INCLUDE)/gdfontl.h
	sh ./install-item 644 gdfontmb.h $(INSTALL_INCLUDE)/gdfontmb.h
	sh ./install-item 644 gdfonts.h $(INSTALL_INCLUDE)/gdfonts.h
	sh ./install-item 644 gdfontt.h $(INSTALL_INCLUDE)/gdfontt.h

gddemo: gddemo.o
	$(CC) gddemo.o -o gddemo	$(LIBDIRS) $(LIBS)

testac: testac.o
	$(CC) testac.o -o testac	$(LIBDIRS) $(LIBS)

pngtogd: pngtogd.o
	$(CC) pngtogd.o -o pngtogd	$(LIBDIRS) $(LIBS) 

webpng: webpng.o
	$(CC) webpng.o -o webpng	$(LIBDIRS) $(LIBS)

pngtogd2: pngtogd2.o
	$(CC) pngtogd2.o -o pngtogd2	$(LIBDIRS) $(LIBS)

gdtopng: gdtopng.o
	$(CC) gdtopng.o -o gdtopng	$(LIBDIRS) $(LIBS)

gd2topng: gd2topng.o
	$(CC) gd2topng.o -o gd2topng	$(LIBDIRS) $(LIBS)

gd2copypal: gd2copypal.o
	$(CC) gd2copypal.o -o gd2copypal	$(LIBDIRS) $(LIBS)

gdparttopng: gdparttopng.o
	$(CC) gdparttopng.o -o gdparttopng	$(LIBDIRS) $(LIBS)

gdtest: gdtest.o
	$(CC) gdtest.o -o gdtest	$(LIBDIRS) $(LIBS)

gd2time: gd2time.o
	$(CC) gd2time.o -o gd2time	$(LIBDIRS) $(LIBS)

gdtestft: gdtestft.o
	$(CC) --verbose gdtestft.o -o gdtestft $(LIBDIRS) $(LIBS)

LIBOBJS=gd.o gd_gd.o gd_gd2.o gd_io.o gd_io_dp.o \
		gd_io_file.o gd_ss.o gd_io_ss.o gd_png.o gd_jpeg.o gdxpm.o \
		gdfontt.o gdfonts.o gdfontmb.o gdfontl.o gdfontg.o \
		gdtables.o gdft.o gdcache.o gdkanji.o wbmp.o \
		gd_wbmp.o gdhelpers.o

#Shared library. This should work fine on any ELF platform (Linux, etc.) with
#GNU ld or something similarly intelligent. To avoid the chicken-and-egg
#problem, this target also installs the library so that applications can
#actually find it.

libgd.so.${VERSION}: ${LIBOBJS}
	-rm -f libgd.so.${VERSION} 2>/dev/null
	ld -shared -o libgd.so.${VERSION} ${LIBOBJS}
	sh ./install-item 644 libgd.so.${VERSION} \
		$(INSTALL_LIB)/libgd.so.${VERSION}
	-rm $(INSTALL_LIB)/libgd.so.${MAJOR_VERSION} 2>/dev/null
	ln -s $(INSTALL_LIB)/libgd.so.${VERSION} \
		$(INSTALL_LIB)/libgd.so.${MAJOR_VERSION}	
	-rm $(INSTALL_LIB)/libgd.so 2>/dev/null
	ln -s $(INSTALL_LIB)/libgd.so.${VERSION} \
		$(INSTALL_LIB)/libgd.so	

#Static library, if you really need one for some reason.
libgd.a: ${LIBOBJS}
	rm -f libgd.a
	$(AR) rc libgd.a ${LIBOBJS}
	-ranlib libgd.a

clean:
	rm -f *.o *.a *.so ${PROGRAMS} test/gdtest.jpg test/gdtest.wbmp

