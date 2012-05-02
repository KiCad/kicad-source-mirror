/*
 * e2net - EDIF to KiCad netlist
 */
#define global

#include <stdio.h>
#include <string.h>
#include "ed.h"
#include "eelibsl.h"

int yydebug=0;
int bug=0;  		// debug level: >2 netlist, >5 schematic, >8 all

char *InFile = "-";

char FileNameNet[64], FileNameSdtLib[64], FileNameEESchema[64], FileNameKiPro[64];
FILE * FileEdf, * FileNet, * FileEESchema, * FileSdtLib=NULL, * FileKiPro=NULL;

global char                      *cur_nnam=NULL;
global struct inst               *insts=NULL, *iptr=NULL;
global struct con                *cons=NULL,  *cptr=NULL;
global float scale;

main(int argc, char *argv[])
{
  char * version      = "0.91";
  char * progname;

  progname = strrchr(argv[0],'/');
  if (progname)
    progname++;
  else
    progname = argv[0];

  fprintf(stderr, "*** %s Version %s ***\n", progname, version);

  // if( argc != 2 ) {
  //    fprintf(stderr,  " usage: %s EDIDsrc \n") ; return(1);
  // }

  if( argc != 2 ){
     FileEdf = stdin;
     FileNet = stdout;
  }else{
     InFile= argv[1];
     sprintf(FileNameNet,"%s.net",argv[1]);
     fprintf(stderr, "Parsing %s\n", InFile);
     if( (FileEdf = fopen( InFile, "rt" )) == NULL ) {
          fprintf(stderr, " %s non trouve\n", InFile);
          return(-1);
     }

     if( (FileNet = fopen( FileNameNet, "wt" )) == NULL ) {
          fprintf(stderr, " %s impossible a creer\n", FileNameNet);
          return(-1);
     }
  }

  Libs=NULL;
  FileEESchema = NULL;
  ParseEDIF(FileEdf, stderr);
  fprintf(stderr,"Parse Complete\n");

  // bubble sort cons by ref
  struct con *start, *a, *b, *c, *e = NULL, *tmp;
  char line[80], s1[40], s2[40], *s;

//  for (start=cons ; start != NULL ; start = start->nxt ){
//      fprintf(stderr,"%s %25s %s\n", start->ref, start->pin, start->nnam);
//  }

  if(cons != NULL)
  while (e != cons->nxt ) {
    c = a = cons; b = a->nxt;
    while(a != e) {
      sprintf(s1, "%s%25s", a->ref, a->pin);
      sprintf(s2, "%s%25s", b->ref, b->pin);
      if( strcmp( s1, s2 ) >0 ) {
        if(a == cons) {
          tmp = b->nxt; b->nxt = a; a->nxt = tmp;
          cons = b; c = b;
        } else {
          tmp = b->nxt; b->nxt = a; a->nxt = tmp;
          c->nxt = b; c = b;
        }
      } else {
        c = a; a = a->nxt;
      }
      b = a->nxt;
      if(b == e)
        e = a;
    }
  }

  // dump connections by component
  strcpy(s1,  "" );
  for (start=cons ; start != NULL ; start = start->nxt ){
      if(strcmp(s1, start->ref) != 0)
	printf("\n");
      printf("%4s %3s %s\n", start->ref, start->pin, start->nnam);
      strcpy(s1,  start->ref);
  }

#ifdef NOT
  while(insts != NULL){
    printf("%5s %s\n", insts->ins, insts->sym);
    insts = insts->nxt;
  }
#endif

  // output kicad netlist
  int  first=1;

  fprintf(FileNet,"( { netlist created  13/9/2007-18:11:44 }\n");
  // by component
  strcpy(s1,  "" );
  while (cons != NULL){
    if(strcmp(s1, cons->ref) != 0) {
      if(!first) fprintf(FileNet," )\n");
      for( s=NULL, iptr=insts ; iptr != NULL && iptr->ins != NULL ; iptr = iptr->nxt ){
	if( !strcmp(cons->ref, iptr->ins)){
	   s = iptr->sym;
	   break;
	}
      }	
      fprintf(FileNet," ( 84DFBB8F $noname %s %s  {Lib=%s}\n", cons->ref, s, s);
      first=0;
    }
    fprintf(FileNet,"  (%5s %s )\n",cons->pin, cons->nnam);
    strcpy(s1,  cons->ref);
    cons = cons->nxt;
  }
  fprintf(FileNet," )\n)\n");

  fclose(FileEdf);
  fclose(FileNet);
  
  if( FileNet != stdout )
    fprintf(stderr,"  output is %s \n", FileNameNet);

  fprintf(stderr, " BonJour\n");
  return(0);
}
