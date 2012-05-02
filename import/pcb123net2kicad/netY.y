%{
// #define YYDEBUG 1
#include <stdio.h>

extern int yydebug;             // export yydebug so lexer can manipulate it
int nbug=0;

struct lst {
  struct con  *net;
  struct lst *nxt;
} *clst=NULL, *lptr;

struct con  {
  char 		*ref;
  char 		*pin;
  char 		*nnam;
  struct con	*nxt;
} *cons=NULL, *cptr;

struct net {
  char 		*nnam;
  struct net 	*nxt;
  struct con 	*nlst;
} *nets=NULL, *nptr;

%}

%union {char *str; int num;}
%start file
%token <str> NAME
%token 	     EOL
%%

file	: net
	| file net
	;

net	: NAME '-''-''-''-' con  EOL
	{ 
	    if(nbug)printf("%s \n", $1);
  	    // add netname to nets
	    nptr = (struct net *) malloc (sizeof (struct net)); 
	    nptr->nnam = $1;
	    nptr->nxt = nets;
	    nptr->nlst = cons;
	    nets = nptr;

	    while( cons != NULL ){
	    	lptr = (struct lst *) malloc (sizeof (struct lst)); 
	    	lptr->net = cons;
	    	lptr->nxt = clst;
	    	clst = lptr;
		cons->nnam = $1;
		cons = cons->nxt;
	    }
	}
	;

con 	: NAME '-' NAME
	{ 
	    if(nbug)printf(" %s-%s ", $1, $3);
	    cptr = (struct con *) malloc (sizeof (struct con)); 
	    cptr->ref = $1;
	    cptr->pin = $3;
	    cptr->nxt = cons;
	    cons = cptr;
      	}
	| con  ',' NAME '-' NAME
	{ 
	    if(nbug)printf(" %s-%s ", $3, $5);
	    cptr = (struct con *) malloc (sizeof (struct con)); 
	    cptr->ref = $3;
	    cptr->pin = $5;
	    cptr->nxt = cons;
	    cons = cptr;
	}
	;
%%

int main (void) {
  int ch, i, first=1;
  char line[80], s1[40], s2[40], *s;
  extern lineno;

  for( i=0; i<5; ){
    if( (ch=getchar()) == '\n'){
	i++;
	lineno++;
    }
    if(nbug) putchar(ch);
  }

  yyparse ();

  // bubble sort clist by ref 
  struct lst *a, *b, *c, *e = NULL, *tmp; 

  while(e != clst->nxt) {
    c = a = clst; b = a->nxt;
    while(a != e) {
      sprintf(s1, "%s%5s", a->net->ref, a->net->pin);
      sprintf(s2, "%s%5s", b->net->ref, b->net->pin);
      if( strcmp( s1, s2 ) >0 ) {
        if(a == clst) {
          tmp = b->nxt; b->nxt = a; a->nxt = tmp;
          clst = b; c = b;
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

#ifdef NOT
  // dump connections by component
  strcpy(s1,  "" );
  while (clst != NULL){
      if(strcmp(s1, clst->net->ref) == 0)
        printf("%4s %3s %s\n", clst->net->ref, clst->net->pin, clst->net->nnam);
      else
        printf("\n%4s %3s %s\n", clst->net->ref, clst->net->pin, clst->net->nnam);
      strcpy(s1,  clst->net->ref);
      clst = clst->nxt;
  }

  // dump by netnames
  while (nets != NULL){
    printf("\n%s", nets->nnam);
    cptr = nets->nlst;
    while( cptr != NULL ){
	printf(" %s.%s", cptr->ref, cptr->pin);
	cptr = cptr->nxt;
    }	
    nets = nets->nxt;
  }
#endif

  // output kicad netlist
  printf("( { netlist created  13/9/2007-18:11:44 }\n");
  // by component
  strcpy(s1,  "" );
  while (clst != NULL){
    if(strcmp(s1, clst->net->ref) != 0) {
      if(!first) printf(" )\n");
      printf(" ( 84DFBB8F SM0805 %s CAP  {Lib=C}\n", clst->net->ref);
      first=0;
    }
    printf("  (%5s %s )\n",clst->net->pin, clst->net->nnam);
    strcpy(s1,  clst->net->ref);
    clst = clst->nxt;
  }
  printf(" )\n)\n");
#ifdef NOTNEEDED
  // by net
  printf("{ Pin List by Nets\n");
  for( i=2 ; nets != NULL ; i++, nets = nets->nxt ){
    printf("Net %d \"%s\"\n", i, nets->nnam);
    for( cptr = nets->nlst; cptr != NULL ; cptr = cptr->nxt)
      printf(" %s %s\n", cptr->ref, cptr->pin);
  }
  printf("}\n#End");
#endif
}

void yyerror (char *s) {
  extern lineno;
  extern char * yytext;

  fprintf (stderr, "line %d %s\n'%s'\n", lineno, s, yytext);
}

