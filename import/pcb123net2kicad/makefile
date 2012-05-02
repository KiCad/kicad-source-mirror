CFLAGS = -w
CC = gcc

# Expression evaluator program: pcb2kicad
pcb2kicad: netY.o netL.o
	$(CC) $(CFLAGS) -o pcb2kicad netY.o netL.o -lm -lfl

netY.o:netY.y netL.c netY.h
	bison -t -v -d netY.y 
	mv netY.tab.c netY.c
	$(CC) $(CFLAGS) -DYYDEBUG -c netY.c 

netL.c:netL.l netY.h
	flex -t netL.l > netL.c

netY.h:netY.y
	bison -d netY.y
	mv netY.tab.h netY.h

clean:
	rm *.o netL.c netY.c netY.h netY.tab.h *.output pcb2kicad
