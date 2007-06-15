		/********************************************************/
		/* Routines generales de gestion des commandes usuelles */
		/********************************************************/

	/* fichier controle.cpp */
/*
 Routines d'affichage grille, Boite de coordonnees, Curseurs, marqueurs ...
*/

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "pcbnew.h"

#include "id.h"
#include "protos.h"

/* Routines Locales : */

/* Variables Locales */

/**********************************/
void RemoteCommand(char * cmdline)
/**********************************/
/* Read a remote command send by eeschema via a socket,
	port KICAD_PCB_PORT_SERVICE_NUMBER (currently 4242)
*/
{
char Line[1024];
wxString msg;
char *idcmd, * text;
WinEDA_PcbFrame * frame = EDA_Appl->m_PcbFrame;

	strncpy(Line, cmdline, sizeof(Line) -1 );
	msg = CONV_FROM_UTF8(Line);

	idcmd = strtok(Line," \n\r");
	text = strtok(NULL," \n\r");
	if ( (idcmd == NULL) || (text == NULL) ) return;

	if( strcmp(idcmd,"$PART:") == 0 )
	{
		MODULE * Module;
		msg = CONV_FROM_UTF8(text);
		Module = ReturnModule(frame->m_Pcb, msg);
		msg.Printf(_("Locate module %s %s"),msg.GetData(), Module ? wxT("Ok") : wxT("not found"));
		frame->Affiche_Message(msg);
		if ( Module )
		{
	wxClientDC dc(frame->DrawPanel);
			frame->DrawPanel->PrepareGraphicContext(&dc);
			frame->DrawPanel->CursorOff(&dc);
			frame->GetScreen()->m_Curseur = Module->m_Pos;
			frame->DrawPanel->CursorOn(&dc);
		}
	}

	if(idcmd && strcmp(idcmd,"$PIN:") == 0)
	{
		wxString PinName, ModName;
		MODULE * Module;
		D_PAD * Pad = NULL;
		int netcode = -1;
		PinName = CONV_FROM_UTF8(text);
		text = strtok(NULL," \n\r");
		if(text && strcmp(text, "$PART:") == 0 ) text = strtok(NULL,"\n\r");

wxClientDC dc(frame->DrawPanel);
	frame->DrawPanel->PrepareGraphicContext(&dc);

		ModName = CONV_FROM_UTF8(text);
		Module = ReturnModule(frame->m_Pcb, ModName);
		if( Module ) Pad = ReturnPad(Module, PinName);
		if( Pad ) netcode = Pad->m_NetCode;
		if ( netcode > 0 )
			{
			/* effacement surbrillance ancienne */
			if(g_HightLigt_Status) frame->Hight_Light(&dc);
			g_HightLigth_NetCode = netcode;
			frame->Hight_Light(&dc);
			frame->DrawPanel->CursorOff(&dc);
			frame->GetScreen()->m_Curseur = Pad->m_Pos;
			frame->DrawPanel->CursorOn( &dc);
			}

		if ( Module == NULL )
			msg.Printf( _("module %s not found"), text);
		else if ( Pad == NULL )
			msg.Printf( _("Pin %s (module %s) not found"), PinName.GetData(), ModName.GetData());
		else
			msg.Printf( _("Locate Pin %s (module %s)"), PinName.GetData(),ModName.GetData());
		frame->Affiche_Message(msg);
	}
}


/***********************************************************************/
EDA_BaseStruct * WinEDA_BasePcbFrame::PcbGeneralLocateAndDisplay(void)
/***********************************************************************/
/* Search an item under the mouse cursor.
	items are searched first on the current working layer.
	if nothing found, an item will be searched without layer restriction
*/
{
EDA_BaseStruct * item;
	item = Locate(CURSEUR_OFF_GRILLE, GetScreen()->m_Active_Layer);
	if ( item == NULL )
		item = Locate(CURSEUR_OFF_GRILLE, -1);
	return item;
}


/****************************************************************/
void WinEDA_BasePcbFrame::GeneralControle(wxDC *DC, wxPoint Mouse)
/*****************************************************************/
{
int ll;
wxSize delta;
int zoom = GetScreen()->GetZoom();
wxPoint curpos, oldpos;
int hotkey = 0;

	ActiveScreen = GetScreen();

	// Save the board after the time out :
int CurrentTime = time(NULL);
	if( !GetScreen()->IsModify() || GetScreen()->IsSave() )
	{		/* If no change, reset the time out */
		g_SaveTime = CurrentTime;
	}

	if ( (CurrentTime - g_SaveTime) > g_TimeOut)
	{
		wxString tmpFileName = GetScreen()->m_FileName;
		wxString filename = g_SaveFileName + PcbExtBuffer;
		bool flgmodify = GetScreen()->IsModify();
		((WinEDA_PcbFrame*)this)->SavePcbFile(filename);
		if( flgmodify )	// Set the flags m_Modify cleared by SavePcbFile()
		{
			GetScreen()->SetModify();
			GetScreen()->SetSave();// Set the flags m_FlagSave cleared by SetModify()
		}
		GetScreen()->m_FileName = tmpFileName;
		SetTitle(GetScreen()->m_FileName);
	}

	curpos = DrawPanel->CursorRealPosition(Mouse);
	oldpos = GetScreen()->m_Curseur;

	delta.x = (int) round((double)GetScreen()->GetGrid().x / zoom);
	delta.y = (int) round((double)GetScreen()->GetGrid().y / zoom);
	if( delta.x <= 0 ) delta.x = 1;
	if( delta.y <= 0 ) delta.y = 1;

	switch(g_KeyPressed)
	{
		case WXK_NUMPAD_SUBTRACT :
		case WXK_SUBTRACT :
		case '-' :
			ll = GetScreen()->m_Active_Layer;
			if(ll > CMP_N) break;
			if(ll <= CUIVRE_N) break;
			if ( m_Pcb->m_BoardSettings->m_CopperLayerCount <= 1)	// Single layer
				ll = CUIVRE_N;
			if ( ll == CMP_N )
				ll = MAX(CUIVRE_N, m_Pcb->m_BoardSettings->m_CopperLayerCount-2);
			else if ( ll > CUIVRE_N) ll--;
			GetScreen()->m_Active_Layer = ll;
			if ( DisplayOpt.ContrastModeDisplay ) DrawPanel->Refresh(TRUE);
			break ;

		case WXK_NUMPAD_ADD :
		case WXK_ADD :
		case '+' :
			ll = GetScreen()->m_Active_Layer;
			if(ll >= CMP_N) break;
			ll++;
			if ( ll >= m_Pcb->m_BoardSettings->m_CopperLayerCount-1 )
				ll = CMP_N;
			if ( m_Pcb->m_BoardSettings->m_CopperLayerCount <= 1)	// Single layer
				ll = CUIVRE_N;
			GetScreen()->m_Active_Layer = ll;
			if ( DisplayOpt.ContrastModeDisplay ) DrawPanel->Refresh(TRUE);
			break ;
		case WXK_NUMPAD0 :
		case WXK_PAGEUP :
			SwitchLayer(DC, CMP_N); 
			break ;

		case WXK_NUMPAD9 :
		case WXK_PAGEDOWN :
			SwitchLayer(DC, CUIVRE_N); 
			break ;
			
		case 'F' | GR_KB_CTRL :
		case 'f' | GR_KB_CTRL:
			DisplayOpt.DisplayPcbTrackFill ^= 1; DisplayOpt.DisplayPcbTrackFill &= 1 ;
			GetScreen()->SetRefreshReq();
			break ;

		case ' ' : /* Mise a jour de l'origine des coord relatives */
			GetScreen()->m_O_Curseur = GetScreen()->m_Curseur;
			break ;


		case 'U' | GR_KB_CTRL :
		case 'u' | GR_KB_CTRL :
			g_UnitMetric = (g_UnitMetric == INCHES ) ? MILLIMETRE : INCHES;
			break ;

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
			oldpos = curpos = GetScreen()->m_Curseur;
			break;

		case WXK_F2 :
			OnZoom(ID_ZOOM_MOINS_KEY);
			oldpos = curpos = GetScreen()->m_Curseur;
			break;

		case WXK_F3 :
			OnZoom(ID_ZOOM_REDRAW_KEY);
			break;

		case WXK_F4 :
			OnZoom(ID_ZOOM_CENTER_KEY);
			oldpos = curpos = GetScreen()->m_Curseur;
			break;

		case WXK_F5 :
			SwitchLayer(DC, LAYER_N_2); 
			break;
		
		case WXK_F6 :
			SwitchLayer(DC, LAYER_N_3); 
			break;
			
		case WXK_F7 :
			SwitchLayer(DC, LAYER_N_4); 
			break;
			
		case WXK_F8 :
			SwitchLayer(DC, LAYER_N_5); 
			break;
			
		case WXK_F9 :
			SwitchLayer(DC, LAYER_N_6); 
			break;
			
		case WXK_F10 :
			SwitchLayer(DC, LAYER_N_7); 
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
			break ;

		default: hotkey = g_KeyPressed;
			break;
	}

	/* Put cursor in new position, according to the zoom keys (if any) */
	GetScreen()->m_Curseur = curpos;
	
	/* Put cursor on grid or a pad centre if requested
		But if the tool DELETE is active the cursor is left off grid
		this is better to reach items to delete off grid
	*/
	D_PAD * pad;
	bool keep_on_grid = TRUE;
	if (m_ID_current_state == ID_PCB_DELETE_ITEM_BUTT) keep_on_grid = FALSE;
	/* Cursor is left off grid if no block in progress and no moving object */
	if ( GetScreen()->BlockLocate.m_State != STATE_NO_BLOCK)
		keep_on_grid = TRUE;
	EDA_BaseStruct *DrawStruct = GetScreen()->m_CurrentItem;
	if ( DrawStruct && DrawStruct->m_Flags ) keep_on_grid = TRUE;

	switch ( g_MagneticPadOption )
	{
		case capture_cursor_in_track_tool:
		case capture_always:
			pad = Locate_Any_Pad(m_Pcb,CURSEUR_OFF_GRILLE, TRUE);
			if ( (m_ID_current_state != ID_TRACK_BUTT ) &&
				 (g_MagneticPadOption == capture_cursor_in_track_tool) )
				pad = NULL;
			if (keep_on_grid)
			{
				if (pad)	// Put cursor on the pad
					GetScreen()->m_Curseur = curpos = pad->m_Pos;
				else		// Put cursor on grid
					PutOnGrid( & GetScreen()->m_Curseur);
			}
			break;

		case no_effect:
		default:
			// If we are not in delete function, put cursor on grid
			if (keep_on_grid)
				PutOnGrid( & GetScreen()->m_Curseur);
			break;
	}

	if ( oldpos != GetScreen()->m_Curseur )
	{
		curpos = GetScreen()->m_Curseur;
		GetScreen()->m_Curseur = oldpos;
		DrawPanel->CursorOff(DC);

		GetScreen()->m_Curseur = curpos;
		DrawPanel->CursorOn(DC);

		if(DrawPanel->ManageCurseur)
		{
			DrawPanel->ManageCurseur(DrawPanel, DC, TRUE);
		}
	}

	if( GetScreen()->IsRefreshReq() )
	{
		RedrawActiveWindow(DC, TRUE);
	}

	SetToolbars();
	Affiche_Status_Box();	 /* Affichage des coord curseur */

	if ( hotkey )
	{
		OnHotKey(DC, hotkey, NULL);
	}
}
