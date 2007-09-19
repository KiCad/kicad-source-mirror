	/******************************************************************/
	/* schframe.cpp  - fonctions des classes du type WinEDA_DrawFrame */
	/******************************************************************/

#ifdef __GNUG__
#pragma implementation
#endif

#include "fctsys.h"
#include "common.h"

#include "program.h"
#include "libcmp.h"
#include "general.h"

#include "bitmaps.h"
#include "protos.h"
#include "id.h"



	/*******************************/
	/* class WinEDA_SchematicFrame */
	/*******************************/

BEGIN_EVENT_TABLE(WinEDA_SchematicFrame, wxFrame)
	COMMON_EVENTS_DRAWFRAME

	EVT_SOCKET(ID_EDA_SOCKET_EVENT_SERV, WinEDA_DrawFrame::OnSockRequestServer)
	EVT_SOCKET(ID_EDA_SOCKET_EVENT, WinEDA_DrawFrame::OnSockRequest)

	EVT_CLOSE(WinEDA_SchematicFrame::OnCloseWindow)
	EVT_SIZE(WinEDA_SchematicFrame::OnSize)

	EVT_MENU_RANGE(ID_LOAD_PROJECT,ID_LOAD_FILE_10,
		WinEDA_SchematicFrame::Process_Special_Functions)

	EVT_TOOL(ID_NEW_PROJECT, WinEDA_SchematicFrame::Process_Special_Functions)
	EVT_TOOL(ID_LOAD_PROJECT, WinEDA_SchematicFrame::Process_Special_Functions)
	EVT_TOOL_RANGE(ID_SCHEMATIC_MAIN_TOOLBAR_START, ID_SCHEMATIC_MAIN_TOOLBAR_END,
		WinEDA_SchematicFrame::Process_Special_Functions)


	EVT_MENU_RANGE(ID_PREFERENCES_FONT_INFOSCREEN, ID_PREFERENCES_FONT_END,
		WinEDA_DrawFrame::ProcessFontPreferences)

	EVT_MENU(ID_SAVE_PROJECT, WinEDA_SchematicFrame::Save_File)
	EVT_MENU(ID_SAVE_ONE_SHEET, WinEDA_SchematicFrame::Save_File)
	EVT_MENU(ID_SAVE_ONE_SHEET_AS, WinEDA_SchematicFrame::Save_File)
	EVT_TOOL(ID_SAVE_PROJECT, WinEDA_SchematicFrame::Save_File)
	EVT_MENU(ID_GEN_PRINT, WinEDA_SchematicFrame::ToPrinter)
	EVT_MENU(ID_GEN_PLOT_PS, WinEDA_SchematicFrame::ToPlot_PS)
	EVT_MENU(ID_GEN_PLOT_HPGL, WinEDA_SchematicFrame::ToPlot_HPGL)
	EVT_MENU(ID_GEN_PLOT_SVG, WinEDA_DrawFrame::SVG_Print)
	EVT_MENU(ID_GEN_COPY_SHEET_TO_CLIPBOARD, WinEDA_DrawFrame::CopyToClipboard)
	EVT_MENU(ID_GEN_COPY_BLOCK_TO_CLIPBOARD, WinEDA_DrawFrame::CopyToClipboard)
	EVT_MENU(ID_EXIT, WinEDA_SchematicFrame::Process_Special_Functions)

	EVT_MENU_RANGE(ID_CONFIG_AND_PREFERENCES_START, ID_CONFIG_AND_PREFERENCES_END, WinEDA_SchematicFrame::Process_Config)

	EVT_MENU_RANGE(ID_LANGUAGE_CHOICE, ID_LANGUAGE_CHOICE_END,
		WinEDA_DrawFrame::SetLanguage)

	EVT_TOOL_RANGE(ID_ZOOM_PLUS_BUTT, ID_ZOOM_PAGE_BUTT,
			WinEDA_SchematicFrame::Process_Zoom)

	EVT_TOOL(ID_NEW_PROJECT, WinEDA_SchematicFrame::Process_Special_Functions)
	EVT_TOOL(ID_LOAD_PROJECT, WinEDA_SchematicFrame::Process_Special_Functions)

	EVT_TOOL(ID_TO_LIBRARY, WinEDA_SchematicFrame::Process_Special_Functions)
	EVT_TOOL(ID_TO_LIBVIEW, WinEDA_SchematicFrame::Process_Special_Functions)

	EVT_TOOL(ID_TO_PCB, WinEDA_SchematicFrame::Process_Special_Functions)
	EVT_TOOL(ID_TO_CVPCB, WinEDA_SchematicFrame::Process_Special_Functions)

	EVT_TOOL(ID_SHEET_SET, WinEDA_DrawFrame::Process_PageSettings)
	EVT_TOOL(ID_HIERARCHY, WinEDA_SchematicFrame::Process_Special_Functions)
	EVT_TOOL(wxID_CUT, WinEDA_SchematicFrame::Process_Special_Functions)
	EVT_TOOL(wxID_COPY, WinEDA_SchematicFrame::Process_Special_Functions)
	EVT_TOOL(wxID_PASTE, WinEDA_SchematicFrame::Process_Special_Functions)
	EVT_TOOL(ID_UNDO_BUTT, WinEDA_SchematicFrame::Process_Special_Functions)
	EVT_TOOL(ID_GEN_PRINT, WinEDA_SchematicFrame::ToPrinter)
	EVT_TOOL_RANGE(ID_GET_ANNOTATE,ID_FIND_ITEMS,
			WinEDA_SchematicFrame::Process_Special_Functions)

	EVT_MENU(ID_GENERAL_HELP, WinEDA_DrawFrame::GetKicadHelp)
	EVT_MENU(ID_KICAD_ABOUT, WinEDA_DrawFrame::GetKicadAbout)

	// Tools et boutons de Schematique, Vertical toolbar:
	EVT_TOOL_RANGE(ID_SCHEMATIC_VERTICAL_TOOLBAR_START,
		ID_SCHEMATIC_VERTICAL_TOOLBAR_END,
		WinEDA_SchematicFrame::Process_Special_Functions)

	EVT_TOOL_RCLICKED(ID_LABEL_BUTT, WinEDA_SchematicFrame::ToolOnRightClick)
	EVT_TOOL_RCLICKED(ID_GLABEL_BUTT, WinEDA_SchematicFrame::ToolOnRightClick)

	EVT_MENU_RANGE(ID_POPUP_START_RANGE, ID_POPUP_END_RANGE,
			WinEDA_SchematicFrame::Process_Special_Functions )

	// Tools et boutons de Schematique, Options toolbar:
	EVT_TOOL_RANGE(ID_TB_OPTIONS_START,ID_TB_OPTIONS_END,
				WinEDA_SchematicFrame::OnSelectOptionToolbar)

	EVT_MENU_RANGE(ID_POPUP_GENERAL_START_RANGE, ID_POPUP_GENERAL_END_RANGE,
			WinEDA_SchematicFrame::Process_Special_Functions )

END_EVENT_TABLE()




	/****************/
	/* Constructeur */
	/****************/

WinEDA_SchematicFrame::	WinEDA_SchematicFrame(wxWindow * father, WinEDA_App *parent,
					const wxString & title, const wxPoint& pos, const wxSize& size) :
					WinEDA_DrawFrame(father, SCHEMATIC_FRAME, parent, title, pos, size)
{
	m_FrameName = wxT("SchematicFrame");
	m_Draw_Axis = FALSE;			// TRUE pour avoir les axes dessines
	m_Draw_Grid = g_ShowGrid;			// TRUE pour avoir la grille dessinee
	m_Draw_Sheet_Ref = TRUE;		// TRUE pour avoir le cartouche dessiné

	// Give an icon
	#ifdef __WINDOWS__
	SetIcon(wxICON(a_icon_eeschema));
	#else
	SetIcon(wxICON(icon_eeschema));
	#endif

	m_CurrentScreen = ScreenSch;
	g_ItemToRepeat = NULL;
	/* Get config */
	GetSettings();
	g_DrawMinimunLineWidth = m_Parent->m_EDA_Config->Read(MINI_DRAW_LINE_WIDTH_KEY, (long)0);
	g_PlotPSMinimunLineWidth = m_Parent->m_EDA_Config->Read(MINI_PLOTPS_LINE_WIDTH_KEY, (long) 4);

	/****/
	SetSize(m_FramePos.x, m_FramePos.y, m_FrameSize.x, m_FrameSize.y);
	if ( DrawPanel ) DrawPanel->m_Block_Enable = TRUE;
	ReCreateMenuBar();
	ReCreateHToolbar();
	ReCreateVToolbar();
	ReCreateOptToolbar();
}


	/***************/
	/* Destructeur */
	/***************/

WinEDA_SchematicFrame::~WinEDA_SchematicFrame()
{
	m_Parent->SchematicFrame = NULL;
	m_CurrentScreen = ScreenSch;
}

/**************************************************************/
void WinEDA_SchematicFrame::OnCloseWindow(wxCloseEvent & Event)
/**************************************************************/
{
SCH_SCREEN * screen;

	if ( m_Parent->LibeditFrame )	// Can close component editor ?
	{
		if ( ! m_Parent->LibeditFrame->Close() ) return;
	}

	screen = ScreenSch ;
	while( screen )
	{
		if(screen->IsModify()) break;
		screen = (SCH_SCREEN*)screen->Pnext;
	}

	if ( screen )
	{
	unsigned ii;
		wxMessageDialog dialog(this, _("Schematic modified, Save before exit ?"),
			_("Confirmation"), wxYES_NO | wxCANCEL | wxICON_EXCLAMATION | wxYES_DEFAULT);
		ii = dialog.ShowModal();
		switch ( ii )
		{
			case wxID_CANCEL:
			Event.Veto();
			return;

			case wxID_NO:
				break;

			case wxID_OK:
			case wxID_YES:
				SaveProject(this);
				break;
		}
	}

	screen = ScreenSch ;
	while( screen )	// suppression flag modify pour eviter d'autres message
	{
		screen->ClrModify();
		screen = (SCH_SCREEN*)screen->Pnext;
	}

	if ( ! ScreenSch->m_FileName.IsEmpty() && (ScreenSch->EEDrawList != NULL) )
			SetLastProject(ScreenSch->m_FileName);

	ClearProjectDrawList(ScreenSch, TRUE);

	/* Tous les autres SCREEN sont effaces, aussi reselection de
	 l'ecran de base, pour les evenements de refresh générés par wxWindows */
	m_CurrentScreen = ActiveScreen = ScreenSch;

	SaveSettings();

	m_Parent->m_EDA_Config->Write(MINI_DRAW_LINE_WIDTH_KEY, (long)g_DrawMinimunLineWidth);
	m_Parent->m_EDA_Config->Write(MINI_PLOTPS_LINE_WIDTH_KEY, (long) g_PlotPSMinimunLineWidth);

	Destroy();
}


/********************************************/
void WinEDA_SchematicFrame::SetToolbars()
/********************************************/
/* Active ou desactive les tools du toolbar horizontal, en fonction des commandes
en cours
*/
{
	if( m_HToolBar )
	{
		if ( m_CurrentScreen->BlockLocate.m_Command == BLOCK_MOVE )
			{
			m_HToolBar->EnableTool(wxID_CUT,TRUE);
			m_HToolBar->EnableTool(wxID_COPY,TRUE);
			}
		else
			{
			m_HToolBar->EnableTool(wxID_CUT,FALSE);
			m_HToolBar->EnableTool(wxID_COPY,FALSE);
			}

		if ( g_BlockSaveDataList ) m_HToolBar->EnableTool(wxID_PASTE,TRUE);
		else m_HToolBar->EnableTool(wxID_PASTE,FALSE);

		if ( GetScreen()->m_RedoList )
			m_HToolBar->EnableTool(ID_SCHEMATIC_REDO,TRUE);
		else m_HToolBar->EnableTool(ID_SCHEMATIC_REDO,FALSE);
		if ( GetScreen()->m_UndoList )
			m_HToolBar->EnableTool(ID_SCHEMATIC_UNDO,TRUE);
		else m_HToolBar->EnableTool(ID_SCHEMATIC_UNDO,FALSE);
	}

	if ( m_OptionsToolBar )
		{
		m_OptionsToolBar->ToggleTool(ID_TB_OPTIONS_SHOW_GRID,m_Draw_Grid);
		m_OptionsToolBar->SetToolShortHelp(ID_TB_OPTIONS_SHOW_GRID,
			m_Draw_Grid ? _("Grid not show") : _("Show Grid"));

		m_OptionsToolBar->ToggleTool(ID_TB_OPTIONS_SELECT_UNIT_MM,
			g_UnitMetric == MILLIMETRE ? TRUE : FALSE);
		m_OptionsToolBar->ToggleTool(ID_TB_OPTIONS_SELECT_UNIT_INCH,
			g_UnitMetric == INCHES ? TRUE : FALSE);

		m_OptionsToolBar->ToggleTool(ID_TB_OPTIONS_SELECT_CURSOR,g_CursorShape);
		m_OptionsToolBar->ToggleTool(ID_TB_OPTIONS_HIDDEN_PINS,g_ShowAllPins);
		m_OptionsToolBar->SetToolShortHelp(ID_TB_OPTIONS_HIDDEN_PINS,
			g_ShowAllPins ? _("No show Hidden Pins") : _("Show Hidden Pins") );
		m_OptionsToolBar->ToggleTool(ID_TB_OPTIONS_BUS_WIRES_ORIENT, g_HVLines);
		m_OptionsToolBar->SetToolShortHelp(ID_TB_OPTIONS_BUS_WIRES_ORIENT,
			g_HVLines ? _("Draw lines at any direction") :
			_("Draw lines H, V or 45 deg only") );
		}

	DisplayUnitsMsg();
}

/******************************************/
int WinEDA_SchematicFrame::BestZoom()
/******************************************/
{
int dx, dy, ii,jj ;
int bestzoom;
wxSize size;

	dx = m_CurrentScreen->m_CurrentSheet->m_Size.x;
	dy = m_CurrentScreen->m_CurrentSheet->m_Size.y;

	size =  DrawPanel->GetClientSize();
	ii = dx / size.x;
	jj = dy / size.y;
	bestzoom = MAX(ii, jj) + 1;

	m_CurrentScreen->SetZoom(ii);
	m_CurrentScreen->m_Curseur.x =  dx / 2;
	m_CurrentScreen->m_Curseur.y =  dy / 2;

	return(bestzoom);
}

