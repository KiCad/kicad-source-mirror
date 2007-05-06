	/***************/
	/* hotkeys.cpp */
	/***************/

#include "fctsys.h"

#include "gr_basic.h"

#include "common.h"
#include "program.h"
#include "libcmp.h"
#include "general.h"

#include "id.h"

#include "protos.h"

/* Routines locales */

/* variables externes */

/***********************************************************/
void WinEDA_SchematicFrame::OnHotKey(wxDC * DC, int hotkey,
					EDA_BaseStruct * DrawStruct)
/***********************************************************/
/* Gestion des commandes rapides (Raccourcis claviers) concernant l'element
sous le courseur souris
 Les majuscules/minuscules sont indifferenciees
	touche DELETE: Effacement (tout element)
	touche R: Rotation (composant ou label)
	touche X: Miroir X (composant)
	touche Y: Miroir Y (composant)
	touche N: Orient 0 (composant)
	touche M: Start Move composant
*/
{
bool PopupOn = m_CurrentScreen->m_CurrentItem  &&
			m_CurrentScreen->m_CurrentItem->m_Flags;

	if ( hotkey == 0 ) return;

	switch (hotkey)
	{
		case WXK_DELETE:
		case WXK_NUMPAD_DELETE:
			if ( PopupOn ) break;
			LocateAndDeleteItem(this, DC);
			m_CurrentScreen->SetModify();
			m_CurrentScreen->m_CurrentItem = NULL;
			TestDanglingEnds(m_CurrentScreen->EEDrawList, DC);
			break;

		case WXK_F5 :
			if ( g_LastSearchIsMarker ) WinEDA_SchematicFrame::FindMarker(1);
			else FindSchematicItem(wxEmptyString, 2);
			break;

		case 'r':	// Rotation
		case 'R':
			if ( DrawStruct == NULL )
			{
				DrawStruct = PickStruct( GetScreen()->m_Curseur,
					GetScreen()->EEDrawList, LIBITEM|TEXTITEM|LABELITEM );
				if ( DrawStruct == NULL ) break;
				if ( DrawStruct->m_StructType == DRAW_LIB_ITEM_STRUCT_TYPE )
						DrawStruct = LocateSmallestComponent( GetScreen() );
				if ( DrawStruct == NULL ) break;
			}
			switch (DrawStruct->m_StructType)
			{
				case DRAW_LIB_ITEM_STRUCT_TYPE:
					CmpRotationMiroir(
						(EDA_SchComponentStruct *) DrawStruct, DC, CMP_ROTATE_COUNTERCLOCKWISE );
					break;

				case DRAW_TEXT_STRUCT_TYPE:
				case DRAW_LABEL_STRUCT_TYPE:
				case DRAW_GLOBAL_LABEL_STRUCT_TYPE:
						ChangeTextOrient( (DrawTextStruct*)DrawStruct, DC);
						break;
			}
			break;

		case 'y':	// Mirror Y (drawlibpart)
		case 'Y':
			if ( DrawStruct == NULL )
				DrawStruct = LocateSmallestComponent( GetScreen() );
			if ( DrawStruct )
			{
				CmpRotationMiroir(
					(EDA_SchComponentStruct *) DrawStruct, DC, CMP_MIROIR_Y );
			}
			break;

		case 'x':	// Mirror X (drawlibpart)
		case 'X':
			if ( DrawStruct == NULL )
				DrawStruct = LocateSmallestComponent( GetScreen() );
			if ( DrawStruct )
			{
				CmpRotationMiroir(
					(EDA_SchComponentStruct *) DrawStruct, DC, CMP_MIROIR_X );
			}
			break;

		case 'n':
		case 'N':	// Orient 0, no mirror (drawlibpart)
			if ( DrawStruct == NULL )
				DrawStruct = LocateSmallestComponent( GetScreen() );
			if ( DrawStruct )
			{
				CmpRotationMiroir(
					(EDA_SchComponentStruct *) DrawStruct, DC, CMP_NORMAL );
				TestDanglingEnds(m_CurrentScreen->EEDrawList, DC);
			}
			break;

		case 'm':
		case 'M':	// Start move drawlibpart
			if ( PopupOn ) break;
			if ( DrawStruct == NULL )
				DrawStruct = LocateSmallestComponent( GetScreen() );
			if ( DrawStruct && (DrawStruct->m_Flags ==0) )
			{
				m_CurrentScreen->m_CurrentItem = DrawStruct;
				Process_Move_Item(m_CurrentScreen->m_CurrentItem, DC);
			}
			break;
	}
}
