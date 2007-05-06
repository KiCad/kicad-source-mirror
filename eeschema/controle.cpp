	/****************/
	/* controle.cpp */
	/****************/

#include "fctsys.h"

#include "gr_basic.h"

#include "common.h"
#include "program.h"
#include "libcmp.h"
#include "general.h"

#include "eda_dde.h"

#include "id.h"

#include "protos.h"

/* Routines locales */

/* variables externes */

#define MSG_TO_PCB KICAD_PCB_PORT_SERVICE_NUMBER


/**************************************************************/
EDA_BaseStruct * WinEDA_SchematicFrame::
		SchematicGeneralLocateAndDisplay(bool IncludePin)
/**************************************************************/

/* Routine de localisation et d'affichage des caract (si utile )
	de l'element pointe par la souris
	- marqueur
	- noconnect
	- jonction
	- wire/bus/entry
	- label
	- composant
	- pin
	retourne
		un pointeur sur le composant
		Null sinon
*/
{
EDA_BaseStruct *DrawStruct;
LibDrawPin * Pin;
EDA_SchComponentStruct * LibItem;
wxString Text;
char Line[1024];
wxString msg;
int ii;

	DrawStruct = PickStruct(GetScreen()->m_Curseur, GetScreen()->EEDrawList, MARKERITEM);
	if( DrawStruct )
		{
		DrawMarkerStruct * Marker = (DrawMarkerStruct *) DrawStruct;
		ii = Marker->m_Type;
		Text = Marker->GetComment();
		if(Text.IsEmpty() ) Text = wxT("NoComment");
		msg = NameMarqueurType[ii]; msg << wxT(" << ") << Text;
		Affiche_Message(msg);
		return(DrawStruct);
		}

	DrawStruct = PickStruct(GetScreen()->m_Curseur, GetScreen()->EEDrawList,
			NOCONNECTITEM);
	if( DrawStruct )
		{
		Affiche_Message(wxEmptyString);
		return(DrawStruct);
		}

	DrawStruct = PickStruct(GetScreen()->m_Curseur, GetScreen()->EEDrawList,
			JUNCTIONITEM);
	if( DrawStruct )
		{
		Affiche_Message(wxEmptyString);
		return(DrawStruct);
		}

	DrawStruct = PickStruct(GetScreen()->m_Curseur, GetScreen()->EEDrawList,
			WIREITEM|BUSITEM|RACCORDITEM);
	if( DrawStruct )	// Recherche d'une pin eventelle
	{
		Pin = LocateAnyPin(m_CurrentScreen->EEDrawList,m_CurrentScreen->m_Curseur, &LibItem);
		if( Pin )
		{
			Pin->Display_Infos(this);
			if ( LibItem )
				Affiche_1_Parametre( this, 1,
						LibItem->m_Field[REFERENCE].m_Text,
						LibItem->m_Field[VALUE].m_Text,
						CYAN);
	
				/* envoi id pin a pcbnew */
			if(Pin->m_PinNum)
			{
				char pinnum[20];
				pinnum[0] = Pin->m_PinNum & 255;
				pinnum[1] = (Pin->m_PinNum >> 8 ) & 255;
				pinnum[2] = (Pin->m_PinNum >> 16 ) & 255;
				pinnum[3] = (Pin->m_PinNum >> 24 ) & 255;
				pinnum[4] = 0;
				sprintf(Line,"$PIN: %s $PART: %s", pinnum,
							CONV_TO_UTF8(LibItem->m_Field[REFERENCE].m_Text));
				SendCommand(MSG_TO_PCB, Line);
			}
		}
		else  Affiche_Message(wxEmptyString);
		return(DrawStruct);
	}

	// Cross probing: Send a command to pcbnew via a socket link, service 4242
	// Cross probing:1- look for a component, and send a locate footprint command to pcbnew
	DrawStruct = PickStruct(GetScreen()->m_Curseur, GetScreen()->EEDrawList,
			FIELDCMPITEM);
	if( DrawStruct )
		{
		PartTextStruct * Field = (PartTextStruct *) DrawStruct;
		LibItem = (EDA_SchComponentStruct * )Field->m_Parent;
		LibItem->Display_Infos(this);

		sprintf(Line,"$PART: %s", CONV_TO_UTF8(LibItem->m_Field[REFERENCE].m_Text) );
		SendCommand(MSG_TO_PCB, Line);

		return(DrawStruct);
		}

	/* search for a pin */
	Pin = LocateAnyPin(m_CurrentScreen->EEDrawList,m_CurrentScreen->m_Curseur, &LibItem);
	if( Pin )
	{
		Pin->Display_Infos(this);
		if ( LibItem )
			Affiche_1_Parametre( this, 1,
					LibItem->m_Field[REFERENCE].m_Text,
					LibItem->m_Field[VALUE].m_Text,
					CYAN);

		// Cross probing:2 - pin found, and send a locate pin command to pcbnew (hightlight net)
		if(Pin->m_PinNum)
		{
			wxString pinnum;
			Pin->ReturnPinStringNum(pinnum);
			sprintf(Line,"$PIN: %s $PART: %s", CONV_TO_UTF8(pinnum),
						CONV_TO_UTF8(LibItem->m_Field[REFERENCE].m_Text));
			SendCommand(MSG_TO_PCB, Line);
		}

		if ( IncludePin == TRUE ) return(LibItem);
	}

	DrawStruct = PickStruct(GetScreen()->m_Curseur, GetScreen()->EEDrawList,
			LIBITEM);
	if( DrawStruct )
	{
		DrawStruct = LocateSmallestComponent( GetScreen() );
		LibItem = (EDA_SchComponentStruct *) DrawStruct;
		LibItem->Display_Infos(this);

		sprintf(Line,"$PART: %s",
			CONV_TO_UTF8(LibItem->m_Field[REFERENCE].m_Text));
		SendCommand(MSG_TO_PCB, Line);

		return(DrawStruct);
	}

	DrawStruct = PickStruct(GetScreen()->m_Curseur, GetScreen()->EEDrawList,
			SHEETITEM);
	if( DrawStruct )
	{
		((DrawSheetStruct*) DrawStruct)->Display_Infos(this);
		return(DrawStruct);
	}

	// Recherche des autres elements
	DrawStruct = PickStruct(GetScreen()->m_Curseur, GetScreen()->EEDrawList,
			SEARCHALL);
	if( DrawStruct )
	{
		return(DrawStruct);
	}

	MsgPanel->EraseMsgBox();
	return(NULL);
}



/**************************************************************/
void WinEDA_DrawFrame::GeneralControle(wxDC *DC, wxPoint Mouse)
/**************************************************************/
{
wxSize delta;
int zoom = m_CurrentScreen->GetZoom();
wxPoint curpos, oldpos;
int hotkey = 0;
	
	ActiveScreen = (SCH_SCREEN *) m_CurrentScreen;
	
	curpos = DrawPanel->CursorRealPosition(Mouse);
	oldpos = m_CurrentScreen->m_Curseur;

	delta.x = m_CurrentScreen->GetGrid().x / zoom;
	delta.y = m_CurrentScreen->GetGrid().y / zoom;

	if( delta.x <= 0 ) delta.x = 1;
	if( delta.y <= 0 ) delta.y = 1;
	
	switch( g_KeyPressed )
		{
		case EDA_PANNING_UP_KEY :
			OnZoom(ID_ZOOM_PANNING_UP);
			curpos = m_CurrentScreen->m_Curseur;
			break;
		case EDA_PANNING_DOWN_KEY :
			OnZoom(ID_ZOOM_PANNING_DOWN);
			curpos = m_CurrentScreen->m_Curseur;
			break;
		case EDA_PANNING_LEFT_KEY :
			OnZoom(ID_ZOOM_PANNING_LEFT);
			curpos = m_CurrentScreen->m_Curseur;
			break;
		case EDA_PANNING_RIGHT_KEY :
			OnZoom(ID_ZOOM_PANNING_RIGHT);
			curpos = m_CurrentScreen->m_Curseur;
			break;
		case WXK_F1 :
			OnZoom(ID_ZOOM_PLUS_KEY);
			curpos = m_CurrentScreen->m_Curseur;
			break;
		case WXK_F2 :
			OnZoom(ID_ZOOM_MOINS_KEY);
			curpos = m_CurrentScreen->m_Curseur;
			break;
		case WXK_F3 :
			OnZoom(ID_ZOOM_REDRAW_KEY);
			break;
		case WXK_F4 :
			OnZoom(ID_ZOOM_CENTER_KEY);
			curpos = m_CurrentScreen->m_Curseur;
			break;

		case ' ':	// Remise a zero coord relatives
			m_CurrentScreen->m_O_Curseur = m_CurrentScreen->m_Curseur;
			break;

		case '\t':    // Switch to drag mode, when block moving
			((WinEDA_SchematicFrame*)this)->HandleBlockEndByPopUp(BLOCK_DRAG, DC);
			break;

		case WXK_NUMPAD8  :	/* Deplacement curseur vers le haut */
		case WXK_UP	:
			Mouse.y -= delta.y;
			DrawPanel->MouseTo(Mouse);
			break ;

		case WXK_NUMPAD2:	/* Deplacement curseur vers le bas */
		case WXK_DOWN:
			Mouse.y += delta.y;
			DrawPanel->MouseTo(Mouse);
			break ;

		case WXK_NUMPAD4:	/* Deplacement curseur vers la gauche */
		case WXK_LEFT :
			Mouse.x -= delta.x;
			DrawPanel->MouseTo(Mouse);
			break ;

		case WXK_NUMPAD6:  /* Deplacement curseur vers la droite */
		case WXK_RIGHT:
			Mouse.x += delta.x;
			DrawPanel->MouseTo(Mouse);
			break;

		case WXK_INSERT:
		case WXK_NUMPAD0:
			if ( m_Ident == SCHEMATIC_FRAME )
			{
				if ( g_ItemToRepeat && (g_ItemToRepeat->m_Flags == 0)  )
				{
					((WinEDA_SchematicFrame*)this)->RepeatDrawItem(DC);
				}
				else wxBell();
				break;
			}
			if ( m_Ident == LIBEDITOR_FRAME )
			{
				if ( LibItemToRepeat && (LibItemToRepeat->m_Flags == 0) &&
					 (LibItemToRepeat->m_StructType == COMPONENT_PIN_DRAW_TYPE) )
				{
					((WinEDA_LibeditFrame*)this)->RepeatPinItem(DC,
								(LibDrawPin*) LibItemToRepeat);
				}
				else wxBell();
				break;
			}

		case 0:
		case WXK_DECIMAL:
			break;
		
		default: hotkey = g_KeyPressed;
			break;
			
		}

	/* Recalcul de la position du curseur schema */
	m_CurrentScreen->m_Curseur = curpos;
	/* Placement sur la grille generale */
	PutOnGrid( & m_CurrentScreen->m_Curseur);

	if( m_CurrentScreen->IsRefreshReq() )
	{
		RedrawActiveWindow(DC, TRUE);
	}

	if ( (oldpos.x != m_CurrentScreen->m_Curseur.x) ||
		 (oldpos.y != m_CurrentScreen->m_Curseur.y) )
	{
		curpos = m_CurrentScreen->m_Curseur;
		m_CurrentScreen->m_Curseur = oldpos;
		DrawPanel->CursorOff(DC);
		m_CurrentScreen->m_Curseur = curpos;
		DrawPanel->CursorOn(DC);

		if(DrawPanel->ManageCurseur)
		{
			DrawPanel->ManageCurseur(DrawPanel, DC, TRUE);
		}
	}

	Affiche_Status_Box();	 /* Affichage des coord curseur */
	
	if ( hotkey )
	{
		if( m_CurrentScreen->m_CurrentItem  &&
			m_CurrentScreen->m_CurrentItem->m_Flags )
			OnHotKey(DC, hotkey, m_CurrentScreen->m_CurrentItem);
		else OnHotKey(DC, hotkey, NULL);
	}

}


