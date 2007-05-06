/************************************************/
/*				Routines diverses				*/
/************************************************/

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"

#include "program.h"
#include "libcmp.h"
#include "general.h"

#include "protos.h"


/**********************************************************/
SCH_SCREEN * CreateNewScreen(WinEDA_DrawFrame * frame_source,
		SCH_SCREEN * OldScreen, int TimeStamp)
/**********************************************************/
/* Routine de creation ( par allocation memoire ) d'un nouvel ecran
	cet ecran est en chainage arriere avec OldScreen
	la valeur TimeStamp est attribuee au parametre NewScreen->TimeStamp
*/
{
SCH_SCREEN * NewScreen;

	NewScreen = new SCH_SCREEN(NULL, frame_source, SCHEMATIC_FRAME);

	NewScreen->SetRefreshReq();
	if(OldScreen) NewScreen->m_Company = OldScreen->m_Company;
	NewScreen->m_TimeStamp = TimeStamp;

	NewScreen->Pback = OldScreen;

	return(NewScreen);
}

/**************************************/
void SetFlagModify(BASE_SCREEN * Window)
/**************************************/
/* Mise a 1 du flag modified de l'ecran Window, et de la date de la feuille
*/
{
	if( Window == NULL ) return;
	Window->SetModify();

	/* Mise a jour des dates */
	Window->m_Date = GenDate();
}


