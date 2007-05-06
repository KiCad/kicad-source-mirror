WXDIR = $(WXWIN)

CC=gcc
LD=gcc
CFLAGS=-O2 -Wall -I$(WXDIR)/src/zlib
EXTRALIBS=  $(WXDIR)/lib/libwxzlib.a

ZIP_OBJS = minizip.o zip.o ioapi.o

.c.o:
	$(CC) -c $(CFLAGS) $*.c

minizip:  $(ZIP_OBJS)
	$(LD) $(LFLAGS) -o $@ $(ZIP_OBJS) $(EXTRALIBS)
