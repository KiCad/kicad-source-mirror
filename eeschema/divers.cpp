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


