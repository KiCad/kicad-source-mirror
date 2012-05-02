#include <stdio.h>

id(int j)
{
printf("\n");
for( ; j>0 ; j--)
   printf(" ");
}

main()
{   
    int ch, i=0, j=0, wh=0, last=0, instr=0 ;

    while((ch = getchar()) != EOF){
	switch (ch) {
	case '(':
	    if( last == ')')
	        id(i);
	    i++; j=i+1;
	    wh=0;
	    break;

	case ')':
	    if(--j == i){
	        printf(")");
		if(i>0)i--;
		last=ch;
	        continue;
	    }
	    wh=1;
	    break;

	case '"':
	    instr ^=1;
	    break;

	case '\r':
	case '\n':
		if(instr)
	    	continue;
	    ch=' ';
		break;
	case ' ':
	case '\t':
	    if(wh || last=='(')
	        continue;
	    break;

        default:
 	    wh=0;
	    break;
	}
	last = ch;
    	printf("%c", ch);
    }
}
