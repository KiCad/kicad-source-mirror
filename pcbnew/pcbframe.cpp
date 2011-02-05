/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2004-2010 Jean-Pierre Charras, jean-pierre.charras@gpisa-lab.inpg.fr
 * Copyright (C) 2010 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2010 Kicad Developers, see change_log.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */


/******************************************/
/* pcbframe.cpp - PCB editor main window. */
/******************************************/

#include "fctsys.h"
#include "appl_wxstruct.h"
#include "common.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "pcbnew.h"
#include "wxPcbStruct.h"
#include "pcbstruct.h"      // enum PCB_VISIBLE
#include "collectors.h"
#include "bitmaps.h"
#include "protos.h"
#include "pcbnew_id.h"
#include "drc_stuff.h"
#include "3d_viewer.h"
#include "kbool/include/kbool/booleng.h"
#include "layer_widget.h"
#include "dialog_design_rules.h"
#include "class_pcb_layer_widget.h"
#include "hotkeys.h"
#include "pcbnew_config.h"
#include "module_editor_frame.h"
#include "dialog_SVG_print.h"
#include "dialog_helpers.h"


extern int g_DrawDefaultLineThickness;

// Keys used in read/write config
#define OPTKEY_DEFAULT_LINEWIDTH_VALUE  wxT( "PlotLineWidth" )
#define PCB_SHOW_FULL_RATSNET_OPT   wxT( "PcbFulRatsnest" )
#define PCB_MAGNETIC_PADS_OPT   wxT( "PcbMagPadOpt" )
#define PCB_MAGNETIC_TRACKS_OPT wxT( "PcbMagTrackOpt" )
#define SHOW_MICROWAVE_TOOLS    wxT( "ShowMicrowaveTools" )
#define SHOW_LAYER_MANAGER_TOOLS    wxT( "ShowLayerManagerTools" )


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
    EVT_MENU( ID_GEN_EXPORT_FILE_MODULE_REPORT, WinEDA_PcbFrame::GenModuleReport )
    EVT_MENU( ID_GEN_EXPORT_FILE_VRML, WinEDA_PcbFrame::OnExportVRML )

    EVT_MENU( ID_GEN_IMPORT_SPECCTRA_SESSION,WinEDA_PcbFrame::ImportSpecctraSession )
    EVT_MENU( ID_GEN_IMPORT_SPECCTRA_DESIGN, WinEDA_PcbFrame::ImportSpecctraDesign )

    EVT_MENU( ID_MENU_ARCHIVE_NEW_MODULES, WinEDA_PcbFrame::Process_Special_Functions )
    EVT_MENU( ID_MENU_ARCHIVE_ALL_MODULES, WinEDA_PcbFrame::Process_Special_Functions )

    EVT_MENU( wxID_EXIT, WinEDA_PcbFrame::OnQuit )

    // menu Config
    EVT_MENU( ID_PCB_DRAWINGS_WIDTHS_SETUP, WinEDA_PcbFrame::OnConfigurePcbOptions )
    EVT_MENU( ID_CONFIG_REQ, WinEDA_PcbFrame::Process_Config )
    EVT_MENU( ID_CONFIG_SAVE, WinEDA_PcbFrame::Process_Config )
    EVT_MENU( ID_CONFIG_READ, WinEDA_PcbFrame::Process_Config )
    EVT_MENU_RANGE( ID_PREFERENCES_HOTKEY_START,
                    ID_PREFERENCES_HOTKEY_END,
                    WinEDA_PcbFrame::Process_Config )
    EVT_MENU( ID_MENU_PCB_SHOW_HIDE_LAYERS_MANAGER_DIALOG, WinEDA_PcbFrame::Process_Config )
    EVT_MENU( ID_OPTIONS_SETUP, WinEDA_PcbFrame::Process_Config )
    EVT_MENU( ID_PCB_LAYERS_SETUP, WinEDA_PcbFrame::Process_Config )
    EVT_MENU( ID_PCB_MASK_CLEARANCE, WinEDA_PcbFrame::Process_Config )
    EVT_MENU( ID_PCB_PAD_SETUP, WinEDA_PcbFrame::Process_Config )
    EVT_MENU( ID_CONFIG_SAVE, WinEDA_PcbFrame::Process_Config )
    EVT_MENU( ID_CONFIG_READ, WinEDA_PcbFrame::Process_Config )
    EVT_MENU( ID_PCB_DISPLAY_OPTIONS_SETUP, WinEDA_PcbFrame::InstallDisplayOptionsDialog )
    EVT_MENU( ID_PCB_USER_GRID_SETUP, WinEDA_PcbFrame::Process_Special_Functions )

    EVT_MENU_RANGE( ID_LANGUAGE_CHOICE, ID_LANGUAGE_CHOICE_END, WinEDA_PcbFrame::SetLanguage )

    // menu Postprocess
    EVT_MENU( ID_PCB_GEN_POS_MODULES_FILE, WinEDA_PcbFrame::GenModulesPosition )
    EVT_MENU( ID_PCB_GEN_DRILL_FILE, WinEDA_PcbFrame::InstallDrillFrame )
    EVT_MENU( ID_PCB_GEN_CMP_FILE, WinEDA_PcbFrame::RecreateCmpFileFromBoard )
    EVT_MENU( ID_PCB_GEN_BOM_FILE_FROM_BOARD, WinEDA_PcbFrame::RecreateBOMFileFromBoard )

    // menu Miscellaneous
    EVT_MENU( ID_MENU_LIST_NETS, WinEDA_PcbFrame::ListNetsAndSelect )
    EVT_MENU( ID_PCB_GLOBAL_DELETE, WinEDA_PcbFrame::Process_Special_Functions )
    EVT_MENU( ID_MENU_PCB_CLEAN, WinEDA_PcbFrame::Process_Special_Functions )
    EVT_MENU( ID_MENU_PCB_SWAP_LAYERS, WinEDA_PcbFrame::Process_Special_Functions )
    EVT_MENU( ID_MENU_PCB_RESET_TEXTMODULE_REFERENCE_SIZES,
              WinEDA_PcbFrame::Process_Special_Functions )
    EVT_MENU( ID_MENU_PCB_RESET_TEXTMODULE_VALUE_SIZES,
              WinEDA_PcbFrame::Process_Special_Functions )

    // Menu Help
    EVT_MENU( ID_GENERAL_HELP, EDA_DRAW_FRAME::GetKicadHelp )
    EVT_MENU( ID_KICAD_ABOUT, EDA_BASE_FRAME::GetKicadAbout )

    // Menu 3D Frame
    EVT_MENU( ID_MENU_PCB_SHOW_3D_FRAME, WinEDA_PcbFrame::Show3D_Frame )

    // Menu Get Design Rules Editor
    EVT_MENU( ID_MENU_PCB_SHOW_DESIGN_RULES_DIALOG, WinEDA_PcbFrame::ShowDesignRulesEditor )

    // Horizontal toolbar
    EVT_TOOL( ID_TO_LIBRARY, WinEDA_PcbFrame::Process_Special_Functions )
    EVT_TOOL( ID_SHEET_SET, EDA_DRAW_FRAME::Process_PageSettings )
    EVT_TOOL( wxID_CUT, WinEDA_PcbFrame::Process_Special_Functions )
    EVT_TOOL( wxID_COPY, WinEDA_PcbFrame::Process_Special_Functions )
    EVT_TOOL( wxID_PASTE, WinEDA_PcbFrame::Process_Special_Functions )
    EVT_TOOL( wxID_UNDO, WinEDA_PcbFrame::GetBoardFromUndoList )
    EVT_TOOL( wxID_REDO, WinEDA_PcbFrame::GetBoardFromRedoList )
    EVT_TOOL( wxID_PRINT, WinEDA_PcbFrame::ToPrinter )
    EVT_TOOL( ID_GEN_PLOT_SVG, WinEDA_PcbFrame::SVG_Print )
    EVT_TOOL( ID_GEN_PLOT, WinEDA_PcbFrame::Process_Special_Functions )
    EVT_TOOL( ID_FIND_ITEMS, WinEDA_PcbFrame::Process_Special_Functions )
    EVT_TOOL( ID_GET_NETLIST, WinEDA_PcbFrame::Process_Special_Functions )
    EVT_TOOL( ID_DRC_CONTROL, WinEDA_PcbFrame::Process_Special_Functions )
    EVT_TOOL( ID_AUX_TOOLBAR_PCB_SELECT_LAYER_PAIR, WinEDA_PcbFrame::Process_Special_Functions )
    EVT_TOOL( ID_AUX_TOOLBAR_PCB_SELECT_AUTO_WIDTH, WinEDA_PcbFrame::Tracks_and_Vias_Size_Event )
    EVT_KICAD_CHOICEBOX( ID_TOOLBARH_PCB_SELECT_LAYER, WinEDA_PcbFrame::Process_Special_Functions )
    EVT_KICAD_CHOICEBOX( ID_AUX_TOOLBAR_PCB_TRACK_WIDTH,
                         WinEDA_PcbFrame::Tracks_and_Vias_Size_Event )
    EVT_KICAD_CHOICEBOX( ID_AUX_TOOLBAR_PCB_VIA_SIZE, WinEDA_PcbFrame::Tracks_and_Vias_Size_Event )
    EVT_TOOL( ID_TOOLBARH_PCB_MODE_MODULE, WinEDA_PcbFrame::AutoPlace )
    EVT_TOOL( ID_TOOLBARH_PCB_MODE_TRACKS, WinEDA_PcbFrame::AutoPlace )
    EVT_TOOL( ID_TOOLBARH_PCB_FREEROUTE_ACCESS, WinEDA_PcbFrame::Access_to_External_Tool )

    // Option toolbar
    EVT_TOOL_RANGE( ID_TB_OPTIONS_START, ID_TB_OPTIONS_END,
                    WinEDA_PcbFrame::OnSelectOptionToolbar )
    EVT_TOOL( ID_TB_OPTIONS_SHOW_MANAGE_LAYERS_VERTICAL_TOOLBAR,
                    WinEDA_PcbFrame::OnSelectOptionToolbar)

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
    EVT_TOOL( ID_PCB_DIMENSION_BUTT, WinEDA_PcbFrame::Process_Special_Functions )
    EVT_TOOL( ID_PCB_DELETE_ITEM_BUTT, WinEDA_PcbFrame::Process_Special_Functions )
    EVT_TOOL( ID_PCB_SHOW_1_RATSNEST_BUTT, WinEDA_PcbFrame::Process_Special_Functions )
    EVT_TOOL( ID_PCB_PLACE_OFFSET_COORD_BUTT, WinEDA_PcbFrame::Process_Special_Functions )

    EVT_TOOL_RANGE( ID_PCB_MUWAVE_START_CMD, ID_PCB_MUWAVE_END_CMD,
                    WinEDA_PcbFrame::ProcessMuWaveFunctions )
    EVT_TOOL( ID_PCB_PLACE_GRID_COORD_BUTT, WinEDA_PcbFrame::Process_Special_Functions )

    EVT_TOOL_RCLICKED( ID_TRACK_BUTT, WinEDA_PcbFrame::ToolOnRightClick )
    EVT_TOOL_RCLICKED( ID_PCB_CIRCLE_BUTT, WinEDA_PcbFrame::ToolOnRightClick )
    EVT_TOOL_RCLICKED( ID_PCB_ARC_BUTT, WinEDA_PcbFrame::ToolOnRightClick )
    EVT_TOOL_RCLICKED( ID_PCB_ADD_TEXT_BUTT, WinEDA_PcbFrame::ToolOnRightClick )
    EVT_TOOL_RCLICKED( ID_PCB_ADD_LINE_BUTT, WinEDA_PcbFrame::ToolOnRightClick )
    EVT_TOOL_RCLICKED( ID_PCB_DIMENSION_BUTT, WinEDA_PcbFrame::ToolOnRightClick )
    EVT_TOOL_RCLICKED( ID_PCB_PLACE_GRID_COORD_BUTT, WinEDA_PcbFrame::ToolOnRightClick )

    EVT_MENU_RANGE( ID_POPUP_PCB_AUTOPLACE_START_RANGE,
                    ID_POPUP_PCB_AUTOPLACE_END_RANGE,
                    WinEDA_PcbFrame::AutoPlace )

    EVT_MENU( ID_POPUP_PCB_REORIENT_ALL_MODULES, WinEDA_PcbFrame::OnOrientFootprints )

    EVT_MENU_RANGE( ID_POPUP_PCB_START_RANGE, ID_POPUP_PCB_END_RANGE,
                    WinEDA_PcbFrame::Process_Special_Functions )

    // Tracks and vias sizes general options
    EVT_MENU_RANGE( ID_POPUP_PCB_SELECT_WIDTH_START_RANGE,
                    ID_POPUP_PCB_SELECT_WIDTH_END_RANGE,
                    WinEDA_PcbFrame::Tracks_and_Vias_Size_Event )

    // popup menus
    EVT_MENU( ID_POPUP_PCB_DELETE_TRACKSEG, WinEDA_PcbFrame::Process_Special_Functions )
    EVT_MENU_RANGE( ID_POPUP_GENERAL_START_RANGE, ID_POPUP_GENERAL_END_RANGE,
                    WinEDA_PcbFrame::Process_Special_Functions )

END_EVENT_TABLE()


///////****************************///////////:


WinEDA_PcbFrame::WinEDA_PcbFrame( wxWindow* parent,
                                  const wxString& title,
                                  const wxPoint& pos, const wxSize& size,
                                  long style ) :
    WinEDA_BasePcbFrame( parent, PCB_FRAME, title, pos, size, style )
{
    m_FrameName = wxT( "PcbFrame" );
    m_Draw_Sheet_Ref = true;            // true to display sheet references
    m_Draw_Axis = false;                 // true to display X and Y axis
    m_Draw_Auxiliary_Axis = true;
    m_Draw_Grid_Axis = true;
    m_SelTrackWidthBox    = NULL;
    m_SelViaSizeBox = NULL;
    m_SelLayerBox   = NULL;
    m_TrackAndViasSizesList_Changed = false;
    m_show_microwave_tools = false;
    m_show_layer_manager_tools = true;
    m_HotkeysZoomAndGridList = g_Board_Editor_Hokeys_Descr;

    SetBoard( new BOARD( NULL, this ) );

    // Create the PCB_LAYER_WIDGET *after* SetBoard():

    wxFont font = wxSystemSettings::GetFont( wxSYS_DEFAULT_GUI_FONT );
    int pointSize = font.GetPointSize();
    int screenHeight = wxSystemSettings::GetMetric( wxSYS_SCREEN_Y );

    // printf( "pointSize:%d  80%%:%d\n", pointSize, (pointSize*8)/10 );

    if( screenHeight <= 900 )
        pointSize = (pointSize * 8) / 10;

    m_Layers = new PCB_LAYER_WIDGET( this, DrawPanel, pointSize );

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
    SetScreen( ScreenPcb );

    // LoadSettings() *after* creating m_LayersManager, because LoadSettings()
    // initialize parameters in m_LayersManager
    LoadSettings();
    SetSize( m_FramePos.x, m_FramePos.y, m_FrameSize.x, m_FrameSize.y );

    GetScreen()->AddGrid( m_UserGridSize, m_UserGridUnit, ID_POPUP_GRID_USER );
    GetScreen()->SetGrid( ID_POPUP_GRID_LEVEL_1000 + m_LastGridSizeId  );

    if( DrawPanel )
        DrawPanel->m_Block_Enable = true;

    ReCreateMenuBar();
    ReCreateHToolbar();
    ReCreateAuxiliaryToolbar();
    ReCreateVToolbar();
    ReCreateOptToolbar();

    ReCreateMicrowaveVToolbar();

    m_auimgr.SetManagedWindow( this );

    // Create a wxAuiPaneInfo template for other wxAuiPaneInfo items
    // Actual wxAuiPaneInfo items will be built from this item.
    wxAuiPaneInfo horiz;
    horiz.Gripper( false );
    horiz.DockFixed( true );
    horiz.Movable( false );
    horiz.Floatable( false );
    horiz.CloseButton( false );
    horiz.CaptionVisible( false );

    // Create a second template from the first:
    wxAuiPaneInfo vert( horiz );

    // Set specific options for horizontal and vertical toolbars, using horiz and vert
    // wxAuiPaneInfo items to manage them.
    vert.TopDockable( false ).BottomDockable( false );
    horiz.LeftDockable( false ).RightDockable( false );

    // Create a template from the horiz wxAuiPaneInfo, specific to horizontal toolbars:
    wxAuiPaneInfo horiz_tb( horiz );
    horiz_tb.ToolbarPane().Gripper( false );

    // Create a wxAuiPaneInfo for the Layers Manager, not derived from the template.
    // LAYER_WIDGET is floatable, but initially docked at far right
    wxAuiPaneInfo   lyrs;
    lyrs.MinSize( m_Layers->GetBestSize() );    // updated in ReFillLayerWidget
    lyrs.BestSize( m_Layers->GetBestSize() );
    lyrs.CloseButton( false );
    lyrs.Caption( _( "Visibles" ) );
    lyrs.IsFloatable();


    if( m_HToolBar )
    {
        m_auimgr.AddPane( m_HToolBar,
                          wxAuiPaneInfo( horiz_tb ).Name( wxT( "m_HToolBar" ) ).Top().Row( 0 ) );
    }

    if( m_AuxiliaryToolBar )
    {
        m_auimgr.AddPane( m_AuxiliaryToolBar,
                          wxAuiPaneInfo( horiz_tb ).Name( wxT( "m_AuxiliaryToolBar" ) ).Top().Row( 1 ) );
    }

    if( m_AuxVToolBar )
        m_auimgr.AddPane( m_AuxVToolBar,
                          wxAuiPaneInfo( vert ).Name( wxT( "m_AuxVToolBar" ) ).Right().Row( 2 ).Hide() );

    if( m_VToolBar )
        m_auimgr.AddPane( m_VToolBar,
                          wxAuiPaneInfo( vert ).Name( wxT( "m_VToolBar" ) ).Right().Row( 1 ) );

    m_auimgr.AddPane( m_Layers, lyrs.Name( wxT( "m_LayersManagerToolBar" ) ).Right().Row( 0 ) );

    if( m_OptionsToolBar )
    {
        m_auimgr.AddPane( m_OptionsToolBar,
                          wxAuiPaneInfo( vert ).Name( wxT( "m_OptionsToolBar" ) ).Left()
                          .ToolbarPane().Gripper( false ) );

        m_OptionsToolBar->ToggleTool( ID_TB_OPTIONS_SHOW_MANAGE_LAYERS_VERTICAL_TOOLBAR,
                                      m_show_layer_manager_tools );
        m_auimgr.GetPane( wxT( "m_LayersManagerToolBar" ) ).Show( m_show_layer_manager_tools );
        m_OptionsToolBar->ToggleTool( ID_TB_OPTIONS_SHOW_EXTRA_VERTICAL_TOOLBAR1,
                                       m_show_microwave_tools );
        m_auimgr.GetPane( wxT( "m_AuxVToolBar" ) ).Show( m_show_microwave_tools );
    }

    if( DrawPanel )
        m_auimgr.AddPane( DrawPanel,
                          wxAuiPaneInfo().Name( wxT( "DrawFrame" ) ).CentrePane() );

    if( MsgPanel )
        m_auimgr.AddPane( MsgPanel,
                          wxAuiPaneInfo( horiz ).Name( wxT( "MsgPanel" ) ).Bottom() );

    SetToolbars();
    ReFillLayerWidget();    // this is near end because contents establish size
    syncLayerWidget();
    m_auimgr.Update();

}


WinEDA_PcbFrame::~WinEDA_PcbFrame()
{
    delete m_drc;
}

void WinEDA_PcbFrame::ReFillLayerWidget()
{
    m_Layers->ReFill();

    wxAuiPaneInfo& lyrs = m_auimgr.GetPane( m_Layers );

    wxSize bestz = m_Layers->GetBestSize();

    lyrs.MinSize( bestz );
    lyrs.BestSize( bestz );
    lyrs.FloatingSize( bestz );

    if( lyrs.IsDocked() )
        m_auimgr.Update();
    else
        m_Layers->SetSize( bestz );
}


void WinEDA_PcbFrame::OnQuit( wxCommandEvent& event )
{
    Close( true );
}

void WinEDA_PcbFrame::OnCloseWindow( wxCloseEvent& Event )
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
            SavePcbFile( GetScreen()->GetFileName() );
            break;
        }
    }

    if( !GetScreen()->GetFileName().IsEmpty() )
    {
        wxFileName fn = GetScreen()->GetFileName();
        fn.SetExt( ProjectFileExtension );
        wxGetApp().WriteProjectConfig( fn.GetFullPath(), GROUP, GetProjectFileParameters() );
    }

    SaveSettings();

    // do not show the window because ScreenPcb will be deleted and we do not
    // want any paint event
    Show( false );
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
        OnModify();
    }
}


void WinEDA_PcbFrame::LoadSettings()
{
    wxConfig* config = wxGetApp().m_EDA_Config;

    if( config == NULL )
        return;

    /* The configuration setting that used to be mixed in with the project
     * file settings.
     */
    wxGetApp().ReadCurrentSetupValues( GetConfigurationSettings() );

    WinEDA_BasePcbFrame::LoadSettings();

    long tmp;
    config->Read( OPTKEY_DEFAULT_LINEWIDTH_VALUE, &g_DrawDefaultLineThickness );
    config->Read( PCB_SHOW_FULL_RATSNET_OPT, &tmp );
    GetBoard()->SetElementVisibility(RATSNEST_VISIBLE, tmp);
    config->Read( PCB_MAGNETIC_PADS_OPT, &g_MagneticPadOption );
    config->Read( PCB_MAGNETIC_TRACKS_OPT, &g_MagneticTrackOption );
    config->Read( SHOW_MICROWAVE_TOOLS, &m_show_microwave_tools );
    config->Read( SHOW_LAYER_MANAGER_TOOLS, &m_show_layer_manager_tools );
}


void WinEDA_PcbFrame::SaveSettings()
{
    wxConfig* config = wxGetApp().m_EDA_Config;

    if( config == NULL )
        return;

    /* The configuration setting that used to be mixed in with the project
     * file settings.
     */
    wxGetApp().SaveCurrentSetupValues( GetConfigurationSettings() );

    WinEDA_BasePcbFrame::SaveSettings();

    wxRealPoint GridSize = GetScreen()->GetGridSize();

    config->Write( OPTKEY_DEFAULT_LINEWIDTH_VALUE, g_DrawDefaultLineThickness );
    long tmp = GetBoard()->IsElementVisible(RATSNEST_VISIBLE);
    config->Write( PCB_SHOW_FULL_RATSNET_OPT, tmp );
    config->Write( PCB_MAGNETIC_PADS_OPT, (long) g_MagneticPadOption );
    config->Write( PCB_MAGNETIC_TRACKS_OPT, (long) g_MagneticTrackOption );
    config->Write( SHOW_MICROWAVE_TOOLS, (long) m_show_microwave_tools );
    config->Write( SHOW_LAYER_MANAGER_TOOLS, (long)m_show_layer_manager_tools );

}

/**
 * Function IsGridVisible() , virtual
 * @return true if the grid must be shown
 */
bool WinEDA_PcbFrame::IsGridVisible()
{
    return IsElementVisible(GRID_VISIBLE);
}

/**
 * Function SetGridVisibility() , virtual
 * It may be overloaded by derived classes
 * if you want to store/retrieve the grid visiblity in configuration.
 * @param aVisible = true if the grid must be shown
 */
void WinEDA_PcbFrame::SetGridVisibility(bool aVisible)
{
    SetElementVisibility(GRID_VISIBLE, aVisible);
}

/**
 * Function GetGridColor() , virtual
 * @return the color of the grid
 */
int WinEDA_PcbFrame::GetGridColor()
{
    return GetBoard()->GetVisibleElementColor( GRID_VISIBLE );
}

/**
 * Function SetGridColor() , virtual
 * @param aColor = the new color of the grid
 */
void WinEDA_PcbFrame::SetGridColor(int aColor)
{
    GetBoard()->SetVisibleElementColor( GRID_VISIBLE, aColor );
}

/* Return true if a microvia can be put on board
 * A microvia ia a small via restricted to 2 near neighbour layers
 * because its is hole is made by laser which can penetrate only one layer
 * It is mainly used to connect BGA to the first inner layer
 * And it is allowed from an external layer to the first inner layer
 */
bool WinEDA_PcbFrame::IsMicroViaAcceptable( void )
{
    int copperlayercnt = GetBoard()->GetCopperLayerCount( );
    int currLayer = getActiveLayer();

    if( !GetBoard()->GetBoardDesignSettings()->m_MicroViasAllowed )
        return false;   // Obvious..

    if( copperlayercnt < 4 )
        return false;   // Only on multilayer boards..

    if( ( currLayer == LAYER_N_BACK )
       || ( currLayer == LAYER_N_FRONT )
       || ( currLayer == copperlayercnt - 2 )
       || ( currLayer == LAYER_N_2 ) )
        return true;

    return false;
}


void WinEDA_PcbFrame::syncLayerWidget( )
{
    m_Layers->SelectLayer( getActiveLayer() );
}

/**
 * Function SetElementVisibility
 * changes the visibility of an element category
 * @param aPCB_VISIBLE is from the enum by the same name
 * @param aNewState = The new visibility state of the element category
 * @see enum PCB_VISIBLE
 */
void WinEDA_PcbFrame::SetElementVisibility( int aPCB_VISIBLE, bool aNewState )
{
    GetBoard()->SetElementVisibility( aPCB_VISIBLE, aNewState );
    m_Layers->SetRenderState( aPCB_VISIBLE, aNewState );
}

/**
 * Function SetVisibleAlls
 * Set the status of all visible element categories and layers to VISIBLE
 */
void WinEDA_PcbFrame::SetVisibleAlls( )
{
    GetBoard()->SetVisibleAlls(  );
    for( int ii = 0; ii < PCB_VISIBLE(END_PCB_VISIBLE_LIST); ii++ )
        m_Layers->SetRenderState( ii, true );
}

/**
 * Function SetLanguage
 * called on a language menu selection
 */
void WinEDA_PcbFrame::SetLanguage( wxCommandEvent& event )
{
    EDA_DRAW_FRAME::SetLanguage( event );
    m_Layers->SetLayersManagerTabsText( );
    wxAuiPaneInfo& pane_info = m_auimgr.GetPane(m_Layers);
    pane_info.Caption( _( "Visibles" ) );
    m_auimgr.Update();
    ReFillLayerWidget();

    if( m_ModuleEditFrame )
        m_ModuleEditFrame->EDA_DRAW_FRAME::SetLanguage( event );
}


wxString WinEDA_PcbFrame::GetLastNetListRead()
{
    wxFileName absoluteFileName = m_lastNetListRead;
    wxFileName pcbFileName = GetScreen()->GetFileName();

    if( !absoluteFileName.MakeAbsolute( pcbFileName.GetPath() )
        || !absoluteFileName.FileExists() )
    {
        absoluteFileName.Clear();
        m_lastNetListRead = wxEmptyString;
    }

    return absoluteFileName.GetFullPath();
}


void WinEDA_PcbFrame::SetLastNetListRead( const wxString& aLastNetListRead )
{
    wxFileName relativeFileName = aLastNetListRead;
    wxFileName pcbFileName = GetScreen()->GetFileName();

    if( relativeFileName.MakeRelativeTo( pcbFileName.GetPath() )
        && relativeFileName.GetFullPath() != aLastNetListRead )
    {
        m_lastNetListRead = relativeFileName.GetFullPath();
    }
}

/**
 * Function OnModify() (virtual)
 * Must be called after a change
 * in order to set the "modify" flag of the current screen
 * and prepare, if needed the refresh of the 3D frame showing the footprint
 * do not forget to call the basic OnModify function to update auxiliary info
 */
void WinEDA_PcbFrame::OnModify( )
{
    WinEDA_BasePcbFrame::OnModify( );
    if( m_Draw3DFrame )
        m_Draw3DFrame->ReloadRequest( );
}


/* Prepare the data structures of print management
 * And displays the dialog window management of printing sheets
 */
void WinEDA_PcbFrame::SVG_Print( wxCommandEvent& event )
{
    DIALOG_SVG_PRINT frame( this );

    frame.ShowModal();
}
