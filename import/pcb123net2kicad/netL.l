%{
#include "netY.h"
#include <string.h>

void yyerror(char*);
int cont=0;
int lineno=0;

%}

%%

<<EOF>>		{return 0;}
[+A-Za-z_$0-9]+ {yylval.str = strdup(yytext); return NAME;}
[&]		{cont=1;}
[ \t]+    	;
[\n]     	{lineno++;
		 if( cont )
		   cont = 0;
                 else
		   return EOL;
		}
[-,]     	{return yytext[0];}
.              	{ECHO; yyerror ("unexpected character");}

%%

int yywrap (void) {return 1;}
