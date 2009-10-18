/****************************************************************/
/* pcbframe.cpp - fonctions des classes du type WinEDA_PcbFrame */
/****************************************************************/

#include "fctsys.h"
#include "appl_wxstruct.h"
#include "common.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "pcbnew.h"
#include "wxPcbStruct.h"
#include "collectors.h"
#include "bitmaps.h"
#include "protos.h"
#include "pcbnew_id.h"
#include "drc_stuff.h"
#include "3d_viewer.h"
#include "kbool/include/kbool/booleng.h"

#include "dialog_design_rules.h"

// Keys used in read/write config
#define PCB_MAGNETIC_PADS_OPT   wxT( "PcbMagPadOpt" )
#define PCB_MAGNETIC_TRACKS_OPT wxT( "PcbMagTrackOpt" )
#define SHOW_MICROWAVE_TOOLS    wxT( "ShowMicrowaveTools" )


/*******************************/
/* class WinEDA_PcbFrame */
/*******************************/

BEGIN_EVENT_TABLE( WinEDA_PcbFrame, WinEDA_BasePcbFrame )
EVT_SOCKET( ID_EDA_SOCKET_EVENT_SERV, WinEDA_PcbFrame::OnSockRequestServer )
EVT_SOCKET( ID_EDA_SOCKET_EVENT, WinEDA_PcbFrame::OnSockRequest )

EVT_KICAD_CHOICEBOX( ID_ON_ZOOM_SELECT, WinEDA_PcbFrame::OnSelectZoom )
EVT_KICAD_CHOICEBOX( ID_ON_GRID_SELECT, WinEDA_PcbFrame::OnSelectGrid )

EVT_CLOSE( WinEDA_PcbFrame::OnCloseWindow )
EVT_SIZE( WinEDA_PcbFrame::OnSize )

EVT_TOOL_RANGE( ID_ZOOM_IN, ID_ZOOM_PAGE, WinEDA_PcbFrame::OnZoom )

EVT_TOOL( ID_LOAD_FILE, WinEDA_PcbFrame::Files_io )
EVT_TOOL( ID_MENU_READ_LAST_SAVED_VERSION_BOARD, WinEDA_PcbFrame::Files_io )
EVT_TOOL( ID_MENU_RECOVER_BOARD, WinEDA_PcbFrame::Files_io )
EVT_TOOL( ID_NEW_BOARD, WinEDA_PcbFrame::Files_io )
EVT_TOOL( ID_SAVE_BOARD, WinEDA_PcbFrame::Files_io )
EVT_TOOL( ID_OPEN_MODULE_EDITOR, WinEDA_PcbFrame::Process_Special_Functions )

// Menu Files:
EVT_MENU( ID_MAIN_MENUBAR, WinEDA_PcbFrame::Process_Special_Functions )

EVT_MENU( ID_LOAD_FILE, WinEDA_PcbFrame::Files_io )
EVT_MENU( ID_NEW_BOARD, WinEDA_PcbFrame::Files_io )
EVT_MENU( ID_SAVE_BOARD, WinEDA_PcbFrame::Files_io )
EVT_MENU( ID_APPEND_FILE, WinEDA_PcbFrame::Files_io )
EVT_MENU( ID_SAVE_BOARD_AS, WinEDA_PcbFrame::Files_io )
EVT_MENU_RANGE( wxID_FILE1, wxID_FILE9, WinEDA_PcbFrame::OnFileHistory )

EVT_MENU( ID_GEN_PLOT, WinEDA_PcbFrame::ToPlotter )

EVT_MENU( ID_GEN_EXPORT_SPECCTRA, WinEDA_PcbFrame::ExportToSpecctra )
EVT_MENU( ID_GEN_EXPORT_FILE_GENCADFORMAT, WinEDA_PcbFrame::ExportToGenCAD )
EVT_MENU( ID_GEN_EXPORT_FILE_MODULE_REPORT,
          WinEDA_PcbFrame::GenModuleReport )

EVT_MENU( ID_GEN_IMPORT_SPECCTRA_SESSION,
          WinEDA_PcbFrame::ImportSpecctraSession )
EVT_MENU( ID_GEN_IMPORT_SPECCTRA_DESIGN,
          WinEDA_PcbFrame::ImportSpecctraDesign )

EVT_MENU( ID_MENU_ARCHIVE_NEW_MODULES,
          WinEDA_PcbFrame::Process_Special_Functions )
EVT_MENU( ID_MENU_ARCHIVE_ALL_MODULES,
          WinEDA_PcbFrame::Process_Special_Functions )

EVT_MENU( ID_EXIT, WinEDA_PcbFrame::Process_Special_Functions )

// menu Config
EVT_MENU_RANGE( ID_CONFIG_AND_PREFERENCES_START,
                ID_CONFIG_AND_PREFERENCES_END,
                WinEDA_PcbFrame::Process_Config )

EVT_MENU( ID_COLORS_SETUP, WinEDA_PcbFrame::Process_Config )
EVT_MENU( ID_OPTIONS_SETUP, WinEDA_PcbFrame::Process_Config )
//EVT_MENU( ID_PCB_COPPER_LAYERS_SETUP, WinEDA_PcbFrame::Process_Config )
EVT_MENU( ID_PCB_LAYERS_SETUP, WinEDA_PcbFrame::Process_Config )
EVT_MENU( ID_PCB_TRACK_SIZE_SETUP, WinEDA_PcbFrame::Process_Config )
EVT_MENU( ID_PCB_DRAWINGS_WIDTHS_SETUP, WinEDA_PcbFrame::Process_Config )
EVT_MENU( ID_PCB_PAD_SETUP, WinEDA_PcbFrame::Process_Config )
EVT_MENU( ID_CONFIG_SAVE, WinEDA_PcbFrame::Process_Config )
EVT_MENU( ID_CONFIG_READ, WinEDA_PcbFrame::Process_Config )
EVT_MENU( ID_PCB_DISPLAY_OPTIONS_SETUP, WinEDA_PcbFrame::InstallDisplayOptionsDialog )


EVT_MENU( ID_PCB_USER_GRID_SETUP,
          WinEDA_PcbFrame::Process_Special_Functions )

EVT_MENU_RANGE( ID_LANGUAGE_CHOICE, ID_LANGUAGE_CHOICE_END,
                WinEDA_DrawFrame::SetLanguage )

// menu Postprocess
EVT_MENU( ID_PCB_GEN_POS_MODULES_FILE, WinEDA_PcbFrame::GenModulesPosition )
EVT_MENU( ID_PCB_GEN_DRILL_FILE, WinEDA_PcbFrame::InstallDrillFrame )
EVT_MENU( ID_PCB_GEN_CMP_FILE, WinEDA_PcbFrame::RecreateCmpFileFromBoard )
EVT_MENU( ID_PCB_GEN_BOM_FILE_FROM_BOARD, WinEDA_PcbFrame::RecreateBOMFileFromBoard )

// menu Miscellaneous
EVT_MENU( ID_MENU_LIST_NETS, WinEDA_PcbFrame::ListNetsAndSelect )
EVT_MENU( ID_PCB_GLOBAL_DELETE, WinEDA_PcbFrame::Process_Special_Functions )
EVT_MENU( ID_MENU_PCB_CLEAN, WinEDA_PcbFrame::Process_Special_Functions )
EVT_MENU( ID_MENU_PCB_SWAP_LAYERS,
          WinEDA_PcbFrame::Process_Special_Functions )

// Menu Help
EVT_MENU( ID_GENERAL_HELP, WinEDA_DrawFrame::GetKicadHelp )
EVT_MENU( ID_KICAD_ABOUT, WinEDA_BasicFrame::GetKicadAbout )

// Menu 3D Frame
EVT_MENU( ID_MENU_PCB_SHOW_3D_FRAME, WinEDA_PcbFrame::Show3D_Frame )

// Menu Get Design Rules Editor
EVT_MENU( ID_MENU_PCB_SHOW_DESIGN_RULES_DIALOG, WinEDA_PcbFrame::ShowDesignRulesEditor )

// Horizontal toolbar
EVT_TOOL( ID_TO_LIBRARY, WinEDA_PcbFrame::Process_Special_Functions )
EVT_TOOL( ID_SHEET_SET, WinEDA_DrawFrame::Process_PageSettings )
EVT_TOOL( wxID_CUT, WinEDA_PcbFrame::Process_Special_Functions )
EVT_TOOL( wxID_COPY, WinEDA_PcbFrame::Process_Special_Functions )
EVT_TOOL( wxID_PASTE, WinEDA_PcbFrame::Process_Special_Functions )
EVT_TOOL( ID_UNDO_BUTT, WinEDA_PcbFrame::GetBoardFromUndoList )
EVT_TOOL( ID_REDO_BUTT, WinEDA_PcbFrame::GetBoardFromRedoList )
EVT_TOOL( ID_GEN_PRINT, WinEDA_DrawFrame::ToPrinter )
EVT_TOOL( ID_GEN_PLOT_SVG, WinEDA_DrawFrame::SVG_Print )
EVT_TOOL( ID_GEN_PLOT, WinEDA_PcbFrame::Process_Special_Functions )
EVT_TOOL( ID_FIND_ITEMS, WinEDA_PcbFrame::Process_Special_Functions )
EVT_TOOL( ID_GET_NETLIST, WinEDA_PcbFrame::Process_Special_Functions )
EVT_TOOL( ID_DRC_CONTROL, WinEDA_PcbFrame::Process_Special_Functions )
EVT_TOOL( ID_AUX_TOOLBAR_PCB_SELECT_LAYER_PAIR,
          WinEDA_PcbFrame::Process_Special_Functions )
EVT_TOOL( ID_AUX_TOOLBAR_PCB_SELECT_AUTO_WIDTH,
          WinEDA_PcbFrame::Tracks_and_Vias_Size_Event )
EVT_KICAD_CHOICEBOX( ID_TOOLBARH_PCB_SELECT_LAYER,
                     WinEDA_PcbFrame::Process_Special_Functions )
EVT_KICAD_CHOICEBOX( ID_AUX_TOOLBAR_PCB_TRACK_WIDTH,
                     WinEDA_PcbFrame::Tracks_and_Vias_Size_Event )
EVT_KICAD_CHOICEBOX( ID_AUX_TOOLBAR_PCB_VIA_SIZE,
                     WinEDA_PcbFrame::Tracks_and_Vias_Size_Event )
EVT_TOOL( ID_TOOLBARH_PCB_AUTOPLACE, WinEDA_PcbFrame::AutoPlace )
EVT_TOOL( ID_TOOLBARH_PCB_AUTOROUTE, WinEDA_PcbFrame::AutoPlace )
EVT_TOOL( ID_TOOLBARH_PCB_FREEROUTE_ACCESS,
          WinEDA_PcbFrame::Access_to_External_Tool )

// Option toolbar
EVT_TOOL_RANGE( ID_TB_OPTIONS_START, ID_TB_OPTIONS_END,
                WinEDA_PcbFrame::OnSelectOptionToolbar )

// Vertical toolbar:
EVT_TOOL( ID_NO_SELECT_BUTT, WinEDA_PcbFrame::Process_Special_Functions )
EVT_TOOL( ID_PCB_HIGHLIGHT_BUTT, WinEDA_PcbFrame::Process_Special_Functions )
EVT_TOOL( ID_COMPONENT_BUTT, WinEDA_PcbFrame::Process_Special_Functions )
EVT_TOOL( ID_TRACK_BUTT, WinEDA_PcbFrame::Process_Special_Functions )
EVT_TOOL( ID_PCB_ZONES_BUTT, WinEDA_PcbFrame::Process_Special_Functions )
EVT_TOOL( ID_PCB_MIRE_BUTT, WinEDA_PcbFrame::Process_Special_Functions )
EVT_TOOL( ID_PCB_ARC_BUTT, WinEDA_PcbFrame::Process_Special_Functions )
EVT_TOOL( ID_PCB_CIRCLE_BUTT, WinEDA_PcbFrame::Process_Special_Functions )
EVT_TOOL( ID_PCB_ADD_TEXT_BUTT, WinEDA_PcbFrame::Process_Special_Functions )
EVT_TOOL( ID_PCB_ADD_LINE_BUTT, WinEDA_PcbFrame::Process_Special_Functions )
EVT_TOOL( ID_PCB_COTATION_BUTT, WinEDA_PcbFrame::Process_Special_Functions )
EVT_TOOL( ID_PCB_DELETE_ITEM_BUTT,
          WinEDA_PcbFrame::Process_Special_Functions )
EVT_TOOL( ID_PCB_SHOW_1_RATSNEST_BUTT,
          WinEDA_PcbFrame::Process_Special_Functions )
EVT_TOOL( ID_PCB_PLACE_OFFSET_COORD_BUTT,
          WinEDA_PcbFrame::Process_Special_Functions )

EVT_TOOL_RANGE( ID_PCB_MUWAVE_START_CMD, ID_PCB_MUWAVE_END_CMD,
                WinEDA_PcbFrame::ProcessMuWaveFunctions )

EVT_TOOL_RCLICKED( ID_TRACK_BUTT, WinEDA_PcbFrame::ToolOnRightClick )
EVT_TOOL_RCLICKED( ID_PCB_CIRCLE_BUTT, WinEDA_PcbFrame::ToolOnRightClick )
EVT_TOOL_RCLICKED( ID_PCB_ARC_BUTT, WinEDA_PcbFrame::ToolOnRightClick )
EVT_TOOL_RCLICKED( ID_PCB_ADD_TEXT_BUTT, WinEDA_PcbFrame::ToolOnRightClick )
EVT_TOOL_RCLICKED( ID_PCB_ADD_LINE_BUTT, WinEDA_PcbFrame::ToolOnRightClick )
EVT_TOOL_RCLICKED( ID_PCB_COTATION_BUTT, WinEDA_PcbFrame::ToolOnRightClick )

EVT_MENU_RANGE( ID_POPUP_PCB_AUTOPLACE_START_RANGE,
                ID_POPUP_PCB_AUTOPLACE_END_RANGE,
                WinEDA_PcbFrame::AutoPlace )

EVT_MENU_RANGE( ID_POPUP_PCB_START_RANGE, ID_POPUP_PCB_END_RANGE,
                WinEDA_PcbFrame::Process_Special_Functions )

// Tracks and vias sizes general options
EVT_MENU_RANGE( ID_POPUP_PCB_SELECT_WIDTH_START_RANGE, ID_POPUP_PCB_SELECT_WIDTH_END_RANGE,
                WinEDA_PcbFrame::Tracks_and_Vias_Size_Event )

// popup menus
EVT_MENU( ID_POPUP_PCB_DELETE_TRACKSEG, WinEDA_PcbFrame::Process_Special_Functions )
EVT_MENU_RANGE( ID_POPUP_GENERAL_START_RANGE, ID_POPUP_GENERAL_END_RANGE,
                WinEDA_PcbFrame::Process_Special_Functions )

EVT_MENU_RANGE( ID_POPUP_VIA_EDIT_START_RANGE, ID_POPUP_VIA_EDIT_END_RANGE,
                WinEDA_PcbFrame::Via_Edit_Control )

// PopUp Menus pour Zooms traites dans drawpanel.cpp
END_EVENT_TABLE()


///////****************************///////////:

/****************/
/* Constructeur */
/****************/

WinEDA_PcbFrame::WinEDA_PcbFrame( wxWindow* father,
                                  const wxString& title,
                                  const wxPoint& pos, const wxSize& size,
                                  long style ) :
    WinEDA_BasePcbFrame( father, PCB_FRAME, title, pos, size, style )
{
    m_FrameName = wxT( "PcbFrame" );
    m_Draw_Sheet_Ref = true;            // true to display sheet references
    m_Draw_Auxiliary_Axis = true;
    m_SelTrackWidthBox    = NULL;
    m_SelViaSizeBox = NULL;
    m_SelLayerBox   = NULL;
    m_TrackAndViasSizesList_Changed = false;
    m_show_microwave_tools = false;

    SetBoard( new BOARD( NULL, this ) );
    m_TrackAndViasSizesList_Changed = true;

    m_drc = new DRC( this );        // these 2 objects point to each other

    m_DisplayPcbTrackFill = DisplayOpt.DisplayPcbTrackFill;
    m_DisplayPadFill = DisplayOpt.DisplayPadFill;
    m_DisplayViaFill = DisplayOpt.DisplayViaFill;
    m_DisplayPadNum  = DisplayOpt.DisplayPadNum;

    m_DisplayModEdge = DisplayOpt.DisplayModEdge;
    m_DisplayModText = DisplayOpt.DisplayModText;

    // Give an icon
    SetIcon( wxICON( a_icon_pcbnew ) );

    m_InternalUnits = PCB_INTERNAL_UNIT;    // Unites internes = 1/10000 inch
    SetBaseScreen( ScreenPcb );
    LoadSettings();
    // Initilialize grid id to a default value if not found in config or bad:
    if( (m_LastGridSizeId <= 0) ||
        (m_LastGridSizeId < (ID_POPUP_GRID_USER - ID_POPUP_GRID_LEVEL_1000)) )
        m_LastGridSizeId = ID_POPUP_GRID_LEVEL_500 - ID_POPUP_GRID_LEVEL_1000;
    SetSize( m_FramePos.x, m_FramePos.y, m_FrameSize.x, m_FrameSize.y );

    GetScreen()->AddGrid( m_UserGridSize, m_UserGridUnits, ID_POPUP_GRID_USER );
    GetScreen()->SetGrid( ID_POPUP_GRID_LEVEL_1000 + m_LastGridSizeId  );

    if( DrawPanel )
        DrawPanel->m_Block_Enable = true;
    ReCreateMenuBar();
    ReCreateHToolbar();
    ReCreateAuxiliaryToolbar();
    ReCreateVToolbar();
    ReCreateOptToolbar();

    if( m_show_microwave_tools )
        ReCreateAuxVToolbar();
}


/************************************/
WinEDA_PcbFrame::~WinEDA_PcbFrame()
/************************************/
{
    extern PARAM_CFG_BASE* ParamCfgList[];

    wxGetApp().SaveCurrentSetupValues( ParamCfgList );
    delete m_drc;
}


/********************************************************/
void WinEDA_PcbFrame::OnCloseWindow( wxCloseEvent& Event )
/********************************************************/
{
    DrawPanel->m_AbortRequest = true;

    if( ScreenPcb->IsModify() )
    {
        unsigned        ii;
        wxMessageDialog dialog( this, _( "Board modified, Save before exit ?" ),
                                _( "Confirmation" ),
                                wxYES_NO | wxCANCEL | wxICON_EXCLAMATION |
                                wxYES_DEFAULT );

        ii = dialog.ShowModal();

        switch( ii )
        {
        case wxID_CANCEL:
            Event.Veto();
            return;

        case wxID_NO:
            break;

        case wxID_OK:
        case wxID_YES:
            SavePcbFile( GetScreen()->m_FileName );
            break;
        }
    }

    SaveSettings();

    // do not show the window because ScreenPcb will be deleted and we do not want any paint event
    Show( false );
    ActiveScreen = ScreenPcb;
    Destroy();
}


/**
 * Display 3D frame of current printed circuit board.
 */
void WinEDA_PcbFrame::Show3D_Frame( wxCommandEvent& event )
{
    if( m_Draw3DFrame )
    {
        DisplayInfoMessage( this, _( "3D Frame already opened" ) );
        return;
    }

    m_Draw3DFrame = new WinEDA3D_DrawFrame( this, _( "3D Viewer" ) );
    m_Draw3DFrame->Show( true );
}


/**
 * Display the Design Rules Editor.
 */
void WinEDA_PcbFrame::ShowDesignRulesEditor( wxCommandEvent& event )
{
    DIALOG_DESIGN_RULES dR_editor( this );
    int returncode = dR_editor.ShowModal();

    if( returncode == wxID_OK )     // New rules, or others changes.
    {
        ReCreateLayerBox( NULL );
        GetScreen()->SetModify();
    }
}


void WinEDA_PcbFrame::LoadSettings()
{
    wxConfig* config = wxGetApp().m_EDA_Config;

    if( config == NULL )
        return;

    WinEDA_BasePcbFrame::LoadSettings();

    config->Read( PCB_MAGNETIC_PADS_OPT, &g_MagneticPadOption );
    config->Read( PCB_MAGNETIC_TRACKS_OPT, &g_MagneticTrackOption );
    config->Read( SHOW_MICROWAVE_TOOLS, &m_show_microwave_tools );
}


void WinEDA_PcbFrame::SaveSettings()
{
    wxConfig* config = wxGetApp().m_EDA_Config;

    if( config == NULL )
        return;

    WinEDA_BasePcbFrame::SaveSettings();

    wxRealPoint GridSize = GetScreen()->GetGridSize();

    config->Write( PCB_MAGNETIC_PADS_OPT, (long) g_MagneticPadOption );
    config->Write( PCB_MAGNETIC_TRACKS_OPT, (long) g_MagneticTrackOption );
    config->Write( SHOW_MICROWAVE_TOOLS, ( m_AuxVToolBar ) ? true : false );
}
