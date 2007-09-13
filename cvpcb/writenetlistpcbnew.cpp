	/***************************/
	/* writenetlistpcbnew.cpp  */
	/***************************/

/*
Complete la netliste (*.NET) en y placant les ref *.lib FORMAT PCBNEW ou ORCADPCB
*/

#include "fctsys.h"

#include "common.h"
#include "cvpcb.h"

#include "protos.h"

#define MAX_LEN_NETNAME 16

/* Routines locales */
static void TriPinsModule( STORECMP * CurrentCmp );
static int PinCompare(const void *cmp1, const void *cmp2);
static void ChangePinNet(  wxString &  PinNet );
static void WriteFootprintFilterInfos(FILE * dest);

/* Variables Locales */
int NetNumCode;			/* Nombre utilise pour cree des NetNames lors de
						reaffectation de NetNames */


/*************************/
int GenNetlistPcbnew()
/*************************/
{
#define NETLIST_HEAD_STRING "EESchema Netlist Version 1.1"
char Line[1024];
STOREPIN * Pin;
STORECMP * CurrentCmp;
wxString Title = g_Main_Title + wxT(" ") + GetBuildVersion();

	NetNumCode = 1; DateAndTime(Line);
	if ( g_FlagEESchema )
		fprintf( dest, "# %s created  %s\n(\n", NETLIST_HEAD_STRING, Line);
	else fprintf( dest, "( { netlist created  %s }\n", Line );

	  /***********************/
	  /* Lecture de la liste */
	  /***********************/

	CurrentCmp = g_BaseListeCmp;
	for( ; CurrentCmp != NULL; CurrentCmp = CurrentCmp->Pnext)
	{
		fprintf(dest," ( %s ", CONV_TO_UTF8(CurrentCmp->m_TimeStamp));

		if( ! CurrentCmp->m_Module.IsEmpty() )
			fprintf(dest, CONV_TO_UTF8(CurrentCmp->m_Module));

		else fprintf(dest,"$noname$") ;

		fprintf(dest," %s ",CONV_TO_UTF8(CurrentCmp->m_Reference)) ;

		/* placement de la valeur */
		fprintf(dest,"%s\n",CONV_TO_UTF8(CurrentCmp->m_Valeur)) ;

		/* Tri des pins */
		TriPinsModule( CurrentCmp );

		/* Placement de la liste des pins */
		Pin = CurrentCmp->m_Pins;
		for( ; Pin != NULL; Pin = Pin->Pnext )
		{
			if( Pin->m_PinNet.Len() > MAX_LEN_NETNAME)
				ChangePinNet( Pin->m_PinNet );
				
			if( ! Pin->m_PinNet.IsEmpty() )
				fprintf(dest,"  ( %s %s )\n",
					CONV_TO_UTF8(Pin->m_PinNum),
					CONV_TO_UTF8(Pin->m_PinNet));
			else
				fprintf(dest,"  ( %s ? )\n", CONV_TO_UTF8(Pin->m_PinNum));
		}
		fprintf(dest," )\n");
	}
	fprintf(dest,")\n*\n");
	if ( g_FlagEESchema ) WriteFootprintFilterInfos(dest);
	fclose(dest);
	return(0);
}

/******************************************/
void WriteFootprintFilterInfos(FILE * dest)
/******************************************/
/* Write the allowed footprint list for each component */
{
STORECMP * component = g_BaseListeCmp;
bool WriteHeader = FALSE;
	
	for ( ; component != NULL; component = component->Pnext )
	{
		unsigned int FilterCount;
		FilterCount = component->m_FootprintFilter.GetCount();
		if ( FilterCount == 0 ) continue;
		if ( ! WriteHeader ) 
		{
			fprintf( dest, "{ Allowed footprints by component:\n" );
			WriteHeader = TRUE;
		}
		fprintf(dest, "$component %s\n", CONV_TO_UTF8(component->m_Reference));
		/* Write the footprint list */
		for ( unsigned int jj = 0; jj < FilterCount; jj ++ )
		{
			fprintf(dest, " %s\n", CONV_TO_UTF8(component->m_FootprintFilter[jj]));
		}
		fprintf(dest, "$endlist\n");
	}
	if ( WriteHeader ) 
		fprintf(dest, "$endfootprintlist\n}\n");
}

/***********************************************/
static void TriPinsModule( STORECMP * CurrentCmp )
/***********************************************/
/* Tri et controle des pins du module CurrentCmp
*/
{
STOREPIN * Pin, * NextPin, ** BasePin;
int nbpins = 0, ii;

	Pin = CurrentCmp->m_Pins;
	if( Pin == NULL ) return;

	/* comptage des pins */
	for( ; Pin != NULL ; Pin = Pin->Pnext ) nbpins++;

	/* Tri des pins: etablissement de la liste des pointeurs */
	BasePin = (STOREPIN ** )MyZMalloc( nbpins * sizeof(STOREPIN*) );

	Pin = CurrentCmp->m_Pins;
	for( ii = 0 ; ii < nbpins ; ii++, Pin = Pin->Pnext )
	{
		BasePin[ii] = Pin;
	}
	/* Tri des Pins */
	qsort( BasePin, nbpins, sizeof( STORECMP*), PinCompare) ;

	/* Remise a jour des pointeurs chaines */
	for( ii = 0 ; ii < nbpins-1 ; ii++ )
	{
		BasePin[ii]->Pnext = BasePin[ii+1];
	}
	BasePin[ii]->Pnext = NULL;
	CurrentCmp->m_Pins = BasePin[0];

	MyFree(BasePin);

	/* Elimination des redondances */
	Pin = CurrentCmp->m_Pins;
	while( Pin != NULL)
	{
		NextPin = Pin->Pnext;
		if ( NextPin == NULL ) break;
		if( Pin->m_PinNum != NextPin->m_PinNum )
		{
			Pin = Pin->Pnext;  continue;
		}
		/* 2 pins successives ont le meme numero */
		if( Pin->m_PinNet != NextPin->m_PinNet )
		{
			wxString msg;
			msg.Printf( _("%s %s pin %s : Different Nets"),
					CurrentCmp->m_Reference.GetData(),CurrentCmp->m_Valeur.GetData(),
					Pin->m_PinNum.GetData());
			DisplayError(NULL, msg, 60 );
		}
		Pin->Pnext = NextPin->Pnext;
		delete NextPin;
	}
}


/*******************************************************/
static int PinCompare(const void *cmp1,const void *cmp2)
/*******************************************************/
/*
routine PinCompare() pour qsort() pour classement alphabetique
	pour tri de la liste des Pins
*/
{
STOREPIN **pt1 , **pt2 ;
int ii;

	pt1 = (STOREPIN**)cmp1;
	pt2 = (STOREPIN**)cmp2;

	ii = StrLenNumICmp( (*pt1)->m_PinNum.GetData(), (*pt2)->m_PinNum.GetData(), 4);
	return(ii);
}


/*******************************************/
static void ChangePinNet( wxString & PinNet )
/*******************************************/
/* Change le NetName PinNet par un nom compose des 8 derniers codes de PinNet
	suivi de _Xnnnnn ou nnnnn est un nom de 0 a 99999
*/
{
STOREPIN * Pin;
STORECMP * CurrentCmp;
int ii;
wxString OldName;
wxString NewName;

	OldName = PinNet;
	ii = PinNet.Len();
	if( Rjustify )  /* On conserve les 8 dernieres lettres du nom */
	{
		NewName= OldName.Right(8); NewName << NetNumCode;
	}

	else			 /* On conserve les 8 premieres lettres du nom */
	{
		NewName = OldName.Left(8); NewName << NetNumCode;
	}
	NetNumCode ++;

	CurrentCmp = g_BaseListeCmp;
	for( ; CurrentCmp != NULL; CurrentCmp = CurrentCmp->Pnext)
	{
		Pin = CurrentCmp->m_Pins;
		for( ; Pin != NULL; Pin = Pin->Pnext )
		{
			if( Pin->m_PinNet != OldName ) continue;
			Pin->m_PinNet = NewName;
		}
	}
}

