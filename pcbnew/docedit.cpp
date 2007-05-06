	/****************************/
	/*	PcbNew - docedit.cppp	*/
	/****************************/

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "pcbnew.h"

#include "protos.h"


/*******************************************************************/
int KeyWordOk(const wxString & KeyList, const wxString & Database )
/*******************************************************************/
/* Recherche si dans le texte Database on retrouve tous les mots
	cles donnes dans KeyList ( KeyList = suite de mots cles
	separes par des espaces
*/
{
char * Keys, *Token, *Data, * TLoc = NULL;

	if(*KeyList < ' ' ) return(0);

	Keys = strdup(KeyList.GetData()); strupper(Keys);
	Data = strdup(Database.GetData()); strupper(Data);

	Token = strtok(Keys," \n\r");
	while (Token)
		{
		TLoc = strstr( Data, Token);
		if( TLoc == NULL) break;
		/* Verification que la chaine trouvee ne soit pas un morceau de mot */
		if( TLoc > Data )
			{
			if ( *(TLoc-1) != ' ' )
				{
				TLoc = NULL; break;
				}
			}
		TLoc += strlen(Token);
		if( *TLoc > ' ' ) { TLoc = NULL; break; }
		Token = strtok(NULL," \n\r");
		}

	free( Keys );
	free( Data );

	if ( TLoc ) return (1);
	return(0);
}


