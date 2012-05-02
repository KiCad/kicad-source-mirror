#
#	Makefile for EDIF parser.
#

# CFLAGS = -DDEBUG
# CFLAGS = -O
CC	   = gcc
CFLAGS = -g -static

SOURCES = edif.y

all  :	e2net ppedif e2sch

ppedif : ppedif.o
	gcc $(CFLAGS) ppedif.c -o ppedif

e2net :	ed.h eelibsl.h e2net.o edif.o savelib.o
	gcc $(CFLAGS) e2net.o edif.o savelib.o -o e2net -lm

e2sch :	ed.h eelibsl.h e2sch.o edif.o savelib.o
	gcc $(CFLAGS) e2sch.o edif.o savelib.o -o e2sch -lm

savelib : fctsys.h eelibsl.h savelib.o 
	gcc $(CFLAGS) -c savelib.c

edif :	ed.h eelibsl.h edif.o 
	gcc $(CFLAGS) -c edif.c

// main.o : main.c
edif.o : edif.c

edif.c : edif.y
	bison -t -v -d edif.y 
	cp edif.tab.c edif.c

#	mv y.tab.c edif.c

# edif.y : edif.y.1 edif.y.2
# 	cat edif.y.1 edif.y.2 > edif.y

clean :
	rm *.o edif.c edif.output edif.tab.c edif.tab.h e2sch e2net ppedif 
	rm e2net.exe  e2sch.exe  ppedif.exe
