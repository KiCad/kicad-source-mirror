	/******************************************************************/
	/* gerberframe.cpp - fonctions des classes du type WinEDA_GerberFrame */
	/******************************************************************/

#ifdef __GNUG__
#pragma implementation
#endif

#include "fctsys.h"

#include "common.h"
#include "gerbview.h"
#include "pcbplot.h"

#include "bitmaps.h"
#include "protos.h"
#include "id.h"


	/****************************************/
	/* class WinEDA_GerberFrame for GerbView*/
	/****************************************/

BEGIN_EVENT_TABLE(WinEDA_GerberFrame, wxFrame)
	COMMON_EVENTS_DRAWFRAME

	EVT_CLOSE(WinEDA_GerberFrame::OnCloseWindow)
	EVT_SIZE(WinEDA_GerberFrame::OnSize)

	EVT_TOOL_RANGE(ID_ZOOM_PLUS_BUTT, ID_ZOOM_PAGE_BUTT,
			WinEDA_GerberFrame::Process_Zoom)

	EVT_TOOL(ID_LOAD_FILE, WinEDA_GerberFrame::Files_io)
	EVT_TOOL(ID_APPEND_FILE, WinEDA_GerberFrame::Files_io)
	EVT_TOOL(ID_INC_LAYER_AND_APPEND_FILE, WinEDA_GerberFrame::Files_io)
	EVT_TOOL(ID_GERBVIEW_LOAD_DRILL_FILE, WinEDA_GerberFrame::Files_io)
	EVT_TOOL(ID_GERBVIEW_LOAD_DCODE_FILE, WinEDA_GerberFrame::Files_io)
	EVT_TOOL(ID_NEW_BOARD, WinEDA_GerberFrame::Files_io)
	EVT_TOOL(ID_SAVE_BOARD, WinEDA_GerberFrame::Files_io)

	EVT_MENU_RANGE(ID_PREFERENCES_FONT_INFOSCREEN, ID_PREFERENCES_FONT_END,
		WinEDA_DrawFrame::ProcessFontPreferences)

	// Menu Files:
	EVT_MENU(ID_MENU_LOAD_FILE, WinEDA_GerberFrame::Files_io)
	EVT_MENU(ID_MENU_APPEND_FILE, WinEDA_GerberFrame::Files_io)
	EVT_MENU(ID_MENU_INC_LAYER_AND_APPEND_FILE, WinEDA_GerberFrame::Files_io)
	EVT_MENU(ID_MENU_NEW_BOARD, WinEDA_GerberFrame::Files_io)
	EVT_MENU(ID_MENU_SAVE_BOARD, WinEDA_GerberFrame::Files_io)
	EVT_MENU(ID_MENU_SAVE_BOARD_AS, WinEDA_GerberFrame::Files_io)
	EVT_MENU(ID_GEN_PLOT, WinEDA_GerberFrame::ToPlotter)
	EVT_MENU(ID_GERBVIEW_EXPORT_TO_PCBNEW, WinEDA_GerberFrame::ExportDataInPcbnewFormat)

	EVT_MENU_RANGE(ID_LOAD_FILE_1,ID_LOAD_FILE_10,
		WinEDA_GerberFrame::Files_io)

	EVT_MENU(ID_EXIT, WinEDA_GerberFrame::Process_Special_Functions)

	// menu Config
	EVT_MENU(ID_CONFIG_REQ, WinEDA_GerberFrame::Process_Config)
	EVT_MENU(ID_COLORS_SETUP, WinEDA_GerberFrame::Process_Config)
	EVT_MENU(ID_OPTIONS_SETUP, WinEDA_GerberFrame::Process_Config)
	EVT_MENU(ID_PCB_LOOK_SETUP, WinEDA_GerberFrame::Process_Config)
	EVT_MENU(ID_CONFIG_SAVE, WinEDA_GerberFrame::Process_Config)
	EVT_MENU(ID_PREFERENCES_CREATE_CONFIG_HOTKEYS, WinEDA_GerberFrame::Process_Config)
	EVT_MENU(ID_PREFERENCES_READ_CONFIG_HOTKEYS, WinEDA_GerberFrame::Process_Config)
	EVT_MENU_RANGE(ID_LANGUAGE_CHOICE, ID_LANGUAGE_CHOICE_END,
		WinEDA_DrawFrame::SetLanguage)

	// menu Postprocess
	EVT_MENU(ID_GERBVIEW_SHOW_LIST_DCODES, WinEDA_GerberFrame::Process_Special_Functions)
	EVT_MENU(ID_GERBVIEW_POPUP_DELETE_DCODE_ITEMS, WinEDA_GerberFrame::Process_Special_Functions)
	EVT_MENU(ID_GERBVIEW_SHOW_SOURCE,
			WinEDA_GerberFrame::Process_Special_Functions )


	// menu Miscellaneous
	EVT_MENU(ID_PCB_GLOBAL_DELETE, WinEDA_GerberFrame::Process_Special_Functions)
	// Menu Help
	EVT_MENU(ID_GENERAL_HELP, WinEDA_DrawFrame::GetKicadHelp)
	EVT_MENU(ID_KICAD_ABOUT, WinEDA_DrawFrame::GetKicadAbout)

	EVT_TOOL(ID_SHEET_SET, WinEDA_DrawFrame::Process_PageSettings)
	EVT_TOOL(wxID_CUT, WinEDA_GerberFrame::Process_Special_Functions)
	EVT_TOOL(wxID_COPY, WinEDA_GerberFrame::Process_Special_Functions)
	EVT_TOOL(wxID_PASTE, WinEDA_GerberFrame::Process_Special_Functions)
	EVT_TOOL(ID_UNDO_BUTT, WinEDA_GerberFrame::Process_Special_Functions)
	EVT_TOOL(ID_GEN_PRINT, WinEDA_GerberFrame::ToPrinter)
	EVT_TOOL(ID_FIND_ITEMS, WinEDA_GerberFrame::Process_Special_Functions)
	EVT_TOOL(ID_DRC_CONTROL, WinEDA_GerberFrame::Process_Special_Functions)
	EVT_KICAD_CHOICEBOX(ID_TOOLBARH_PCB_SELECT_LAYER,
					WinEDA_GerberFrame::Process_Special_Functions)

	EVT_KICAD_CHOICEBOX(ID_TOOLBARH_GERBER_SELECT_TOOL,
					WinEDA_GerberFrame::Process_Special_Functions)


	// Vertical toolbar:
	EVT_TOOL(ID_NO_SELECT_BUTT, WinEDA_GerberFrame::Process_Special_Functions)
	EVT_TOOL(ID_TRACK_BUTT, WinEDA_GerberFrame::Process_Special_Functions)
	EVT_TOOL(ID_PCB_ZONES_BUTT, WinEDA_GerberFrame::Process_Special_Functions)
	EVT_TOOL(ID_PCB_DELETE_ITEM_BUTT, WinEDA_GerberFrame::Process_Special_Functions)

	// Annulation de commande en cours
	EVT_MENU_RANGE(ID_POPUP_GENERAL_START_RANGE, ID_POPUP_GENERAL_END_RANGE,
			WinEDA_PcbFrame::Process_Special_Functions )

	// Pop up menu
	EVT_MENU(ID_GERBVIEW_POPUP_DELETE_DCODE_ITEMS,
			WinEDA_GerberFrame::Process_Special_Functions )

	// Option toolbar
	EVT_TOOL_RANGE(ID_TB_OPTIONS_START,ID_TB_OPTIONS_END,
				WinEDA_GerberFrame::OnSelectOptionToolbar)


	// PopUp Menu traités dans drawpanel.cpp

END_EVENT_TABLE()


	/****************/
	/* Constructeur */
	/****************/

WinEDA_GerberFrame::WinEDA_GerberFrame(wxWindow * father, WinEDA_App *parent,
					const wxString & title, const wxPoint& pos, const wxSize& size) :
					WinEDA_BasePcbFrame(father, parent, GERBER_FRAME, title, pos, size)
{
	m_FrameName = wxT("GerberFrame");
	m_Draw_Axis = TRUE;			// TRUE pour avoir les axes dessines
	m_Draw_Grid = TRUE;			// TRUE pour avoir la axes dessinee
	m_Draw_Sheet_Ref = FALSE;	// TRUE pour avoir le cartouche dessiné
	m_Ident = GERBER_FRAME;
	m_ZoomMaxValue = 1024;
	if ( DrawPanel ) DrawPanel->m_Block_Enable = TRUE;
	// Give an icon
	#ifdef __WINDOWS__
	SetIcon( wxICON(a_icon_gerbview));
	#else
	SetIcon( wxICON(icon_gerbview));
	#endif

	m_CurrentScreen = ActiveScreen = ScreenPcb;

	GetSettings();
	SetSize(m_FramePos.x, m_FramePos.y, m_FrameSize.x, m_FrameSize.y);
	ReCreateMenuBar();
	ReCreateHToolbar();
	ReCreateVToolbar();
	ReCreateOptToolbar();
}


WinEDA_GerberFrame::~WinEDA_GerberFrame()
{
	m_Parent->m_GerberFrame = NULL;
	m_CurrentScreen = ScreenPcb;
}


/***********************************************************/
void WinEDA_GerberFrame::OnCloseWindow(wxCloseEvent & Event)
/***********************************************************/
{
PCB_SCREEN * screen;

	screen = ScreenPcb ;
	while( screen )
		{
		if(screen->IsModify()) break;
		screen = screen->Next();
		}

	if ( screen )
		{
		if( ! IsOK(this, _("Layer modified,  Continue ?")) )
			{
			Event.Veto();
			return;
			}
		}

	while( screen )	// suppression flag modify pour eviter d'autres message
		{
		screen->ClrModify();
		screen = screen->Next();
		}

	/* Reselection de l'ecran de base,
		pour les evenements de refresh générés par wxWindows */
	m_CurrentScreen = ActiveScreen = ScreenPcb;

	SaveSettings();
	Destroy();
}

/*******************************************/
void WinEDA_GerberFrame::SetToolbars()
/*******************************************/
/* Active ou desactive les tools du toolbar horizontal, en fonction des commandes
en cours
*/
{
int layer = GetScreen()->m_Active_Layer;
GERBER_Descr * Gerber_layer_descr	= g_GERBER_Descr_List[layer];

	if( m_HToolBar == NULL ) return;

	if ( GetScreen()->BlockLocate.m_Command == BLOCK_MOVE )
	{
		m_HToolBar->EnableTool(wxID_CUT,TRUE);
		m_HToolBar->EnableTool(wxID_COPY,TRUE);
	}
	else
	{
		m_HToolBar->EnableTool(wxID_CUT,FALSE);
		m_HToolBar->EnableTool(wxID_COPY,FALSE);
	}

	if ( g_UnDeleteStackPtr )
	{
		m_HToolBar->EnableTool(wxID_PASTE,TRUE);
		m_HToolBar->EnableTool(ID_UNDO_BUTT,TRUE);
	}
	else
	{
		m_HToolBar->EnableTool(wxID_PASTE,FALSE);
		m_HToolBar->EnableTool(ID_UNDO_BUTT,FALSE);
	}


	if ( m_SelLayerBox->GetSelection() != GetScreen()->m_Active_Layer )
	{
		m_SelLayerBox->SetSelection( GetScreen()->m_Active_Layer );
	}

	if ( Gerber_layer_descr )
	{
		int sel_index;
		m_SelLayerTool->Enable(TRUE);
		if ( Gerber_layer_descr->m_Selected_Tool < FIRST_DCODE ) // No tool selected
			sel_index = 0;
		else
			sel_index = Gerber_layer_descr->m_Selected_Tool - FIRST_DCODE + 1;

		if ( sel_index != m_SelLayerTool->GetSelection() )
		{
			m_SelLayerTool->SetSelection(sel_index);
		}
	}
	else
	{
		m_SelLayerTool->SetSelection(0);
		m_SelLayerTool->Enable(FALSE);
	}

	if ( m_OptionsToolBar )
	{
		m_OptionsToolBar->ToggleTool(ID_TB_OPTIONS_SELECT_UNIT_MM,
			g_UnitMetric == MILLIMETRE ? TRUE : FALSE);
		m_OptionsToolBar->ToggleTool(ID_TB_OPTIONS_SELECT_UNIT_INCH,
			g_UnitMetric == INCHES ? TRUE : FALSE);

		m_OptionsToolBar->ToggleTool(ID_TB_OPTIONS_SHOW_POLAR_COORD,
			DisplayOpt.DisplayPolarCood);

		m_OptionsToolBar->ToggleTool(ID_TB_OPTIONS_SHOW_GRID,
			m_Draw_Grid);

		m_OptionsToolBar->ToggleTool(ID_TB_OPTIONS_SELECT_CURSOR,
			g_CursorShape);

		m_OptionsToolBar->ToggleTool(ID_TB_OPTIONS_SHOW_PADS_SKETCH,
			! m_DisplayPadFill);

		m_OptionsToolBar->ToggleTool(ID_TB_OPTIONS_SHOW_TRACKS_SKETCH,
			 ! m_DisplayPcbTrackFill);

		m_OptionsToolBar->ToggleTool(ID_TB_OPTIONS_SHOW_DCODES,
			 DisplayOpt.DisplayPadNum);
	}

	DisplayUnitsMsg();
}

/*************************************/
int WinEDA_GerberFrame::BestZoom()
/*************************************/
{
int ii,jj ;
int bestzoom;
wxSize size;

	/* calcul du zoom montrant tout le dessim */
	m_Pcb->ComputeBoundaryBox();
	size =  DrawPanel->GetClientSize();
	ii = m_Pcb->m_BoundaryBox.GetWidth() / size.x;
	jj = m_Pcb->m_BoundaryBox.GetHeight() / size.y;
	bestzoom = MAX(ii, jj) + 1;

	/* determination du zoom existant le plus proche */
	for (ii = 1 ; ii < 2048 ; ii <<= 1 )
		{
		if(ii >= bestzoom) break;
		}
	bestzoom = ii;

	GetScreen()->m_Curseur = m_Pcb->m_BoundaryBox.Centre();

	return(bestzoom);
}

