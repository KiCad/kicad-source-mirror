	/********************************************************/
	/* Traitement des netlistes VIEWLOGIC , Format WIRELIST */
	/********************************************************/

/* Traite la netliste VIEWLOGIC au format WIRELIST
*/

#include "fctsys.h"

#include "common.h"
#include "cvpcb.h"

#include "protos.h"

/* routines locales : */
static int ReadVLDescrCmp( char * Line, STORECMP * Cmp, int Type);
static int ReadReelNumPin( char * Line, STORECMP * Cmp, int Type);
static int RegroupeUnitsComposant( STORECMP * BaseCmp );
static STORECMP *TraitePseudoCmp(char * Text, STORECMP *PseudoCmp, int Header);
static void MergePseudoCmp(STORECMP * BaseCmp, STORECMP * BasePseudoCmp);

/* Variables Locales */
STORECMP * BasePseudoCmp;	/* Stockage des descriptions generales */

/* Identificateurs de debut de ligne */
#define API ( (('A'&255) << 16) | (('P'&255) << 8 ) | ('I'&255) )
#define AP_ ( (('A'&255) << 8) | ('P'&255) )
#define AS_ ( (('A'&255) << 8) | ('S'&255) )
#define A__ ('A' & 255)
#define W__ ('W' & 255)
#define M__ ('M' & 255)
#define I__ ('I' & 255)


/************************************************/
int WinEDA_CvpcbFrame::ReadViewlogicWirList(void)
/************************************************/
{
int ii, Type = 0, error, Header;
char RefDes[40], val[40], LocalRef[40], Generic[40] ;
char Line[1024], *Text;
STORECMP * Cmp, *NextCmp;
STORECMP * PseudoCmp = NULL;
wxString msg;
	
	modified = 0;
	Rjustify = 1;

	/* Raz buffer et variable de gestion */
	if( g_BaseListeCmp ) FreeMemoryComponants();

	Cmp = NULL;

	/* Ouverture du fichier source */
	msg = _("Netlist file ") + FFileName;
	SetStatusText(msg,0);

	source = wxFopen(FFileName, wxT("rt"));
	if (source == 0)
	{
		msg = _("File not found ") + FFileName;
		DisplayError(this, msg); return(-1);
	}

	/* Lecture entete qui doit etre "| Wirelist ..*/
	fgets(Line,1024,source) ;
	ii = strncmp(Line,"| Wirelist",3) ;	 /* net type Viewlogic */

	if ( ii != 0 )
	{
		wxString Lineconv = CONV_FROM_UTF8(Line);
		msg.Printf( _("Unknown file format <%s>"), Lineconv.GetData());
		fclose(source); return(-3) ;
	}

	SetStatusText( wxT("Format Netliste ViewLogic wirelist"), 0);

	/* Lecture de la liste */
	for ( ;; )
		{
		if ( fgets(Line,1024,source) == 0 )  break ;
		Text = StrPurge(Line);

		Header = *Text & 255;	/* Header est la copie du 1er mot de la ligne */
		for( ii = 1; ii < 3 ; ii++ )
			{
			if( Text[ii] <= ' ' ) break;
			Header <<= 8; Header |= Text[ii] & 255;
			}

		switch( Header )
			{
			case AP_ :
			case AS_ :
				PseudoCmp = TraitePseudoCmp(Text, PseudoCmp, Header);
				Type = 0;
				break;

			case API :
				if( (nbcomp <= 0 ) || (Cmp == NULL) )
					{
					DisplayError(NULL, wxT("Description API inattendue"), 20);
					break;
					}
				error = ReadReelNumPin( Text, Cmp, Type );
				if( error < 0 )
					{
					msg.Printf( wxT("Erreur %d ligne API"), -error );
					DisplayError(NULL, msg,10 );
					}
				break;

			case I__ :	/* Lecture descr 1 composant */
				if( nbcomp <= 0 )
				{
					DisplayError(NULL, wxT("Description Composant inattendue"), 20);
					break;
				}
				*RefDes = 0; *val = 0;
				ReadVLDescrCmp( Text, Cmp, Type);
				Type = 1;	/* pour lecture num pins */
				break;

			case W__ :
			case M__ :
				Cmp = new STORECMP();
				Cmp->Pnext = g_BaseListeCmp;
				g_BaseListeCmp = Cmp;
				nbcomp++ ;
				Type = Header;
				Text = strtok(NULL, " \t\n\r");
				if( Text == NULL ) break;
				strncpy(Generic, Text, 40);
				Text = strtok(NULL, " \n\r");
				if( Text == NULL ) break;
				strncpy(LocalRef, Text, 40);
				break;

			default:
				Type = 0;
				break;
			}
		}

	fclose(source);

	/* reclassement alpab‚tique : */
	g_BaseListeCmp = TriListeComposantss( g_BaseListeCmp, nbcomp);
	nbcomp -= RegroupeUnitsComposant( g_BaseListeCmp );

	/* Addition des renseignements issus des pseudo composants */
	MergePseudoCmp( g_BaseListeCmp, BasePseudoCmp);

	/* Renumerotation des composants */
	Cmp = g_BaseListeCmp;
	for( ii = 1; Cmp != NULL; ii++, Cmp = Cmp->Pnext)
	{
		Cmp->m_Num = ii;
	}

	/* Liberation memoire */
	Cmp = BasePseudoCmp;
	for( ; Cmp != NULL; Cmp = NextCmp )
	{
		NextCmp = Cmp->Pnext; delete Cmp;
	}
	BasePseudoCmp = NULL;

	return(0);
}


/****************************************************************/
static int ReadVLDescrCmp( char * Line, STORECMP * Cmp, int Type)
/****************************************************************/
/* Lecture de la description d'un composant
	(ligne commencant par I ...)
*/
{
char * Text, *Ident;
int nbpins = 0;
char numpin[9];
STOREPIN * Pin = NULL;
STOREPIN ** LastPin = & Cmp->m_Pins;

	Text = strtok(Line, " \n\t\r`");

	Text = strtok(NULL, " \n\t\r`");	/* Text pointe 1er mot utile */

	Ident = strtok(NULL, " \n\t\r`");	/* Ident pointe identificateur */
	Cmp->m_Repere = CONV_FROM_UTF8(Ident);

	while ( Text )
		{
		Text = strtok(NULL, " \t\n\r`");
		if( Text == NULL ) break;

		if( strncmp(Text, "VALUE=" ,6) == 0 )
		{
			Cmp->m_Valeur = CONV_FROM_UTF8(Text+6);
			continue;
		}

		if( strncmp(Text, "REFDES=",7 ) == 0 )
		{
			Cmp->m_Reference = CONV_FROM_UTF8(Text+7);
			if( !isdigit(Cmp->m_Reference.Last() ) )
				Cmp->m_Reference.RemoveLast();
			continue;
		}

		/* Lecture d'un net pin */
		nbpins++;
		Pin = (STOREPIN *)MyZMalloc( sizeof(STOREPIN) );
		*LastPin = Pin; LastPin = &Pin->Pnext;
		sprintf(numpin,"%d", nbpins);
		Pin->m_Index = nbpins;
		Pin->m_PinNum = CONV_FROM_UTF8(numpin);
		Pin->m_PinNet = CONV_FROM_UTF8(Text);
		}

	if( Cmp->m_Valeur.IsEmpty())
	{
		Cmp->m_Valeur = CONV_FROM_UTF8(Ident);
	}


	/* Mise en place du TimeStamp init a 0 */
	Cmp->m_TimeStamp = wxT("00000000");

	/* Analyse du type */
	switch( Type )
	{
		case M__: return(0);
		case W__: return(0);
		default: break;
	}
	return(-1);
}

	/*********************************************************************/
	/* int ReadReelNumPin( char * Line, STORECMP * Cmp, int Type) */
	/*********************************************************************/

/* Lit les lignes commencant par "API" pour recuperer le vrai numero de pin
*/
static int ReadReelNumPin( char * Line, STORECMP * Cmp, int Type)
{
char * Text, *Ident;
int numpin;
STOREPIN * Pin;


	if( Type != 1 ) return (-1);

	Text = strtok(Line, " \n\t\r");

	Ident = strtok(NULL, " \n\t\r");	/* Ident pointe identificateur */

	Text = strtok(NULL, " \n\t\r");		/* Text pointe type composant */

	Text = strtok(NULL, " \n\t\r");		/* pointe numero d'ordre */
	if( Text == NULL ) return(-2);;

	numpin = atoi(Text);

	Text = strtok(NULL, " \n\t\r");		/* pointe Reel NumPin */
	if( Text == NULL ) return(-3);

	if( strncmp(Text, "#=" ,2) ) return (-4);

	for(Pin = Cmp->m_Pins ; Pin != NULL; Pin = Pin->Pnext )
		{
		if( Pin->m_Type != STRUCT_PIN ) return(-5);
		if( Pin->m_Index != numpin ) continue;
		Pin->m_PinNum = CONV_FROM_UTF8(Text+2);
		return(0);
		}
	return(-6 );
}


	/***********************************************/
	/* int RegroupeComposant( STORECMP * BaseCmp ) */
	/***********************************************/

/* Regroupe les pins des differentes Unites d'un meme composant
	et modifie le chainage
*/
static int RegroupeUnitsComposant( STORECMP * BaseCmp )
{
STORECMP * Cmp = BaseCmp;
STORECMP * NextCmp = Cmp->Pnext;
STOREPIN * Pin;
int Deleted = 0;


	if( NextCmp == NULL ) return(0);

	for( ; NextCmp != NULL; Cmp = NextCmp, NextCmp = NextCmp->Pnext )
		{
		if( Cmp->m_Reference != NextCmp->m_Reference ) continue;
		/* 2 composants identiques : Pins a regrouper */
		Deleted++;
		Pin = Cmp->m_Pins;
		if( Pin == NULL ) Cmp->m_Pins = NextCmp->m_Pins;
		else
		{
			while(Pin->Pnext) Pin = Pin->Pnext;
			Pin->Pnext = NextCmp->m_Pins;
		}
		NextCmp->m_Pins = NULL;
		Cmp->Pnext = NextCmp->Pnext;
		(Cmp->Pnext)->Pback = Cmp;
		delete NextCmp;
		NextCmp = Cmp;
		}
	return(Deleted);
}



/****************************************************************************/
static STORECMP *TraitePseudoCmp(char * Line, STORECMP *PseudoCmp, int Header)
/****************************************************************************/
{
STORECMP * Cmp = PseudoCmp;
STOREPIN * Pin;
char  *Text;
wxString Name;

	Text = strtok(Line," \t\n\r");
	Text = strtok(NULL," \t\n\r");  /* Pointe Name */
	Name = CONV_FROM_UTF8(Text);
	Text = strtok(NULL," \t\n\r");  /* Pointe partie utile */

	if( Cmp == NULL )
	{
		Cmp = BasePseudoCmp = new STORECMP();
		Cmp->m_Repere = Name;
		Cmp->m_Valeur = Name;
	}

	else if( Name != Cmp->m_Valeur )	/* Nouveau pseudo composant */
	{
		Cmp = new STORECMP();
		PseudoCmp->Pnext = Cmp;
		Cmp->m_Valeur = Name;
		Cmp->m_Repere = Name;
	}

	switch ( Header )
		{
		case AS_ :
			if( strnicmp(Text,"PKG_TYPE=",9) == 0 )
				{
				Cmp->m_Module = CONV_FROM_UTF8(Text+9);
				break;
				}
			if( strnicmp(Text,"PARTS =",7) == 0 )
				{
				Cmp->m_Multi = atoi(Text+7);
				break;
				}
			if( strnicmp(Text,"REFDES=",7) == 0 )
				{
				Cmp->m_Reference = CONV_FROM_UTF8(Text+7);
				break;
				}
			if( strnicmp(Text,"SIGNAL=",7) == 0 )
				{
				Text = strtok(Text," ;=\t\n\r");
				Text = strtok(NULL," ;=\t\n\r");  /* Pointe partie utile */
				Pin = new STOREPIN();
				Pin->Pnext = Cmp->m_Pins;
				Cmp->m_Pins = Pin;
				Pin->m_PinNet = CONV_FROM_UTF8(Text);
				Text = strtok(NULL," ;=\t\n\r");  /* Pointe partie utile */
				Pin->m_PinNum = CONV_FROM_UTF8(Text);
				break;
				}
			break;

		case AP_ :
			break;

		}	

	return(Cmp);
}


/*********************************************************************/
static void MergePseudoCmp(STORECMP * BaseCmp, STORECMP * BasePseudoCmp)
/*********************************************************************/
/* Additionne aux composants standards les renseignements contenus
dans les descriptions generales
*/
{
STORECMP * Cmp, * PseudoCmp;
STOREPIN * Pin, * PseudoPin;

	Cmp = BaseCmp;
	for( ; Cmp != NULL; Cmp = Cmp->Pnext)
	{
		PseudoCmp = BasePseudoCmp;
		for( ; PseudoCmp != NULL; PseudoCmp = PseudoCmp->Pnext)
		{
			if( Cmp->m_Repere != PseudoCmp->m_Repere ) continue;
			/* Description trouvee, transfert des infos */
			Cmp->m_Multi = PseudoCmp->m_Multi;
			PseudoPin = PseudoCmp->m_Pins;
			for ( ; PseudoPin != NULL; PseudoPin = PseudoPin->Pnext)
			{
				Pin = new STOREPIN(*PseudoPin);
				Pin->Pnext = Cmp->m_Pins; Cmp->m_Pins = Pin;
			}
			break;
		}
	}
}

