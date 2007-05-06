/***************************/
/* readschematicnetlist.cpp*/
/***************************/

/* convertit la netliste ORCADPCB en netliste ORCADPCB (fichier temporaire)
assure la r‚affectation des alimentations selon le format :
( XXXXXX VALEUR|(pin1,pin2,...=newalim) ID VALEUR
*/

#include "fctsys.h"

#include "wxstruct.h"
#include "common.h"
#include "cvpcb.h"

#include "protos.h"

#define SEPARATEUR '|'  /* caractere separateur dans netliste */

/* routines locales : */

static int pin_orcad(STORECMP * CurrentCmp);


/************************************************/
int WinEDA_CvpcbFrame::ReadSchematicNetlist(void)
/************************************************/
{
int i , j , k ,l ;
char * LibName;
char Line[1024];
char label[80] ;		/* buffer des references composants */
char ref_schema[80] ;	/* buffer de la ref schematique */
char val[80] ;		 /* buffer des valeurs/ref.lib */
char postval[80] ;	/* buffer de la valeur de fin de ligne (vraie valeur) */
char *ptchar ;		/* pointeur de service */
STORECMP * Cmp;

	modified = 0;
	Rjustify = 0;
	g_FlagEESchema = FALSE;

	/* Raz buffer et variable de gestion */
	if( g_BaseListeCmp ) FreeMemoryComponants();

	/* Ouverture du fichier source */
	source = wxFopen(FFileName, wxT("rt"));
	if (source == 0)
	{
		wxString msg;
		msg.Printf( _("File <%s> not found"),FFileName.GetData());
		DisplayError(this, msg); return(-1);
	}

	/* Read the file header (must be  "( { OrCAD PCB" or "({ OrCAD PCB" ) */
	/* or "# EESchema Netliste"*/
	fgets(Line,255,source) ;
	/* test for netlist type PCB2 */
	i =  strnicmp(Line,"( {",3) ;
	if( i != 0 )
		i =  strnicmp(Line,"({",2) ;
	if( i != 0 )
	{
		i =  strnicmp(Line,"# EESchema",7) ;	/* net type EESchema */
		if( i == 0 )  g_FlagEESchema = TRUE;
	}

	if ( i != 0 )
	{
		wxString msg, Lineconv = CONV_FROM_UTF8(Line);
		msg.Printf( _("Unknown file format <%s>"), Lineconv.GetData());
		DisplayError(this, msg);
		fclose(source); return(-3) ;
	}

	SetStatusText( _("Netlist Format: EESchema"), 0);


	/* Lecture de la liste */
	for (;;)
	{
		/* recherche du debut de la description d'un composant */

		if( fgets(Line,255,source)  == 0 ) break;

		/* Remove blanks */
		i = 0 ; while (Line[i] == ' ') i++ ;

		/* elimination des lignes vides : */
		if (Line[i] < ' ') continue ;

		if (strnicmp(&Line[i],"{ Allowed footprints",  20 ) == 0 )
		{
			ReadFootprintFilterList(source);
			continue;
		}
			
		if (strnicmp(&Line[i],"( ",2) != 0) continue ;

		/****************************/
		/* debut description trouve */
		/****************************/
		/* memo ident schema */
		while ( Line[i] != ' ') i++ ;
		while ( Line[i] == ' ') i++ ; /* i pointe 1er caractere de l'ident schema */

		j = 0 ; while ( Line[i] != ' ') ref_schema[j++] = Line[i++] ;
		ref_schema[j] = 0 ;

		/* recherche val/ref.lib */
		while ( Line[i] == ' ') i++ ; /* i pointe la valeur du composant */
		LibName = Line + i;

		memset(label, 0, sizeof(label));
		memset(val, 0, sizeof(val) ) ;
		memset(postval, 0, sizeof(postval) ) ;
		memset(alim, 0, sizeof(alim) ) ;

		/* lecture valeur du composant */

		/* recherche fin de valeur (' ') */
		ptchar = strstr(&Line[i]," ") ;
		if (ptchar == 0)
		{
		wxString msg;
		msg.Printf( _("Netlist error: %s"),Line) ;
			DisplayError(NULL, msg);
			k = 0 ;
		}
		else k = ptchar - Line ;

		for (j = 0 ; i < k ;  i++)
		{
			 if ( Line[i] == SEPARATEUR ) break ;
			 if ( j < 8 ) val[j++] = Line[i] ;
		}

		if ( (Line[++i] == '(') && (Line[k-1] == ')' ) )
		{
			i++ ; l = 0 ; while ( k-1 > i ) alim[l++] = Line[i++] ;
		}

		else	i = k ;

		/* recherche reference du composant */
		while(Line[i] != ' ') i++ ; /* elimination fin valeur */
		while(Line[i] == ' ') i++ ; /* recherche debut reference */

		/* debut reference trouv‚ */
		for ( k = 0 ; k < 8 ; i++ , k++ )
		{
			if ( Line[i] <= ' ' ) break ;
			label[k] = Line[i] ;
		}

		/* recherche vraie valeur du composant */
		while(Line[i] != ' ') i++ ; /* elimination fin reference */
		while(Line[i] == ' ') i++ ; /* recherche debut vraie valeur */

		/* debut vraie valeur trouv‚e */
		for ( k = 0 ; k < 16 ; i++ , k++ )
		{
			if ( Line[i] <= ' ' ) break ;
			postval[k] = Line[i] ;
		}


		/* classement du composant ,suivi de sa valeur */
		Cmp = new STORECMP();
		Cmp->Pnext = g_BaseListeCmp;
		g_BaseListeCmp = Cmp;
		Cmp->m_Reference = CONV_FROM_UTF8(label);
		Cmp->m_Valeur = CONV_FROM_UTF8(postval) ;

		if(  g_FlagEESchema )	/* Copie du nom module: */
		{
			if( strnicmp(LibName, "$noname", 7 ) != 0 )
			{
				while( *LibName > ' ' )
				{
					Cmp->m_Module.Append(*LibName);
					LibName++;
				}
			}
		}
		/* classement du TimeStamp */
		Cmp->m_TimeStamp = CONV_FROM_UTF8(ref_schema);

		pin_orcad( Cmp) ;

		nbcomp++ ;
	}
	fclose(source);

	/* reclassement alpabetique : */
	g_BaseListeCmp = TriListeComposantss( g_BaseListeCmp, nbcomp);

	return(0);
}

/********************************************************/
int WinEDA_CvpcbFrame::ReadFootprintFilterList( FILE * f)
/********************************************************/
{
char Line[1024];
wxString CmpRef;
STORECMP * Cmp = NULL;
	
	for (;;)
	{
		if( fgets(Line,255,source)  == 0 ) break;
		if (strnicmp(Line,"$endlist", 8 ) == 0 )
		{
			Cmp = NULL;
			continue;
		}
		if (strnicmp(Line,"$endfootprintlist", 4 ) == 0 ) return 0;
		
		if (strnicmp(Line,"$component", 10 ) == 0 ) // New component ref found
		{
			CmpRef = CONV_FROM_UTF8(Line+11);
			CmpRef.Trim(TRUE); CmpRef.Trim(FALSE);
			/* Search the new component in list */
			for ( Cmp = g_BaseListeCmp; Cmp != NULL; Cmp = Cmp->Pnext )
			{
				if ( Cmp->m_Reference == CmpRef) break;
			}
		}
		
		else if ( Cmp )
		{
			wxString fp = CONV_FROM_UTF8(Line+1);
			fp.Trim(FALSE); fp.Trim(TRUE);
			Cmp->m_FootprintFilter.Add(fp);
		}
	}
	
	return 1;
}


/***********************************/
int pin_orcad(STORECMP * Cmp)
/***********************************/
{
int i , jj;
char numpin[9] , net[1024] ;
char Line[1024];
STOREPIN * Pin = NULL;
STOREPIN ** LastPin = & Cmp->m_Pins;

for ( ;; )
	{
	/* debut description trouv‚ */
	for ( ;; )
		{
		if ( fgets(Line,80,source) == 0 ) return(-1) ;

		/* suppression des blancs en d‚but de ligne */
		i = 0 ; while (Line[i] == ' ') i++ ;
		while (Line[i] == '(') i++ ;
		while (Line[i] == ' ') i++ ;

		/* elimination des lignes vides : */
		if (Line[i] < ' ') continue ;

		/* fin de description ? */
		if (Line[i] == ')' ) return(0) ;

		memset( net, 0, sizeof(net) );
		memset( numpin, 0, sizeof(numpin) );

		/* lecture name pin , 4 lettres */
		for (jj = 0 ; jj < 4 ; jj++ , i++)
			{
			if ( Line[i] == ' ' ) break ;
			numpin[jj] = Line[i] ;
			}

		/* recherche affectation forc‚e de net  */
		if ( reaffect(numpin,net) != 0)
		{
			Pin = new STOREPIN();
			*LastPin = Pin; LastPin = &Pin->Pnext;
			Pin->m_PinNum = CONV_FROM_UTF8(numpin);
			Pin->m_PinNet = CONV_FROM_UTF8(net);
			continue ;
		}

		/* recherche netname */
		while(Line[i] == ' ') i++ ; /* recherche debut reference */

		/* debut netname trouv‚ */
		for ( jj = 0 ; jj < (int)sizeof(net)-1 ; i++ , jj++ )
			{
			if ( Line[i] <= ' ' ) break ;
			net[jj] = Line[i] ;
			}

		Pin =  new STOREPIN();
		*LastPin = Pin; LastPin = &Pin->Pnext;
		Pin->m_PinNum = CONV_FROM_UTF8(numpin);
		Pin->m_PinNet = CONV_FROM_UTF8(net);
		}
	}
}


/****************************************************************/
STORECMP * TriListeComposantss(STORECMP * BaseListe, int nbitems)
/****************************************************************/
/* Tri la liste des composants par ordre alphabetique et me a jour
le nouveau chainage avant/arriere
	retourne un pointeur sur le 1er element de la liste
*/
{
STORECMP ** bufferptr, * Item;
int ii;

	if (nbitems <= 0 ) return(NULL);
	bufferptr = (STORECMP**)MyZMalloc( (nbitems+2) * sizeof(STORECMP*) );

	for( ii= 1, Item = BaseListe; Item != NULL; Item = Item->Pnext, ii++)
	{
		bufferptr[ii] = Item;
	}

	/* ici bufferptr[0] = NULL et bufferptr[nbitem+1] = NULL et ces 2 valeurs
	representent le chainage arriere du 1er element, et le chainage avant
	du dernier element */

	qsort(bufferptr+1,nbitems,sizeof(STORECMP*),
							(int(*)(const void*,const void*))CmpCompare) ;
	/* Mise a jour du chainage */
	for( ii = 1; ii <= nbitems; ii++ )
	{
		Item = bufferptr[ii];
		Item->m_Num = ii;
		Item->Pnext = bufferptr[ii+1];
		Item->Pback = bufferptr[ii-1];
	}
	return(bufferptr[1]);
}


/****************************************/
int CmpCompare(void * mod1, void * mod2)
/****************************************/
/*
routine compare() pour qsort() en classement alphabetique des composants
*/
{
int ii;
STORECMP *pt1 , *pt2 ;

	pt1 = * ((STORECMP**)mod1);
	pt2 = * ((STORECMP**)mod2);

	//FIXME:
	ii = StrNumICmp( (const wxChar*) pt1->m_Reference, (const wxChar*) pt2->m_Reference );
	return(ii);
}



