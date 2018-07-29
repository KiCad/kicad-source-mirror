/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2015 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2015-2016 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 1992-2018 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file moduleframe.cpp
 * @brief Footprint (module) editor main window.
 */

#include <fctsys.h>
#include <kiface_i.h>
#include <kiway.h>
#include <project.h>
#include <kicad_plugin.h>
#include <class_drawpanel.h>
#include <pcb_draw_panel_gal.h>
#include <confirm.h>
#include <pcb_edit_frame.h>
#include <dialog_helpers.h>
#include <3d_viewer/eda_3d_viewer.h>
#include <msgpanel.h>
#include <fp_lib_table.h>
#include <bitmaps.h>
#include <gal/graphics_abstraction_layer.h>

#include <class_board.h>
#include <class_module.h>

#include <pcbnew.h>
#include <pcbnew_id.h>
#include <hotkeys.h>
#include <footprint_edit_frame.h>
#include <footprint_viewer_frame.h>
#include <wildcards_and_files_ext.h>
#include <pcb_layer_widget.h>
#include <invoke_pcb_dialog.h>

#include <tool/tool_manager.h>
#include <tool/common_tools.h>
#include <tool/tool_dispatcher.h>
#include <tool/zoom_tool.h>

#include <footprint_tree_pane.h>
#include <widgets/lib_tree.h>
#include <fp_lib_table.h>
#include <footprint_info_impl.h>

#include <widgets/paged_dialog.h>
#include <dialogs/panel_modedit_settings.h>
#include <dialogs/panel_modedit_defaults.h>
#include <dialogs/panel_modedit_display_options.h>
#include <dialog_configure_paths.h>

#include <tools/position_relative_tool.h>
#include <widgets/progress_reporter.h>
#include "tools/selection_tool.h"
#include "tools/edit_tool.h"
#include "tools/drawing_tool.h"
#include "tools/point_editor.h"
#include "tools/pcbnew_control.h"
#include "tools/footprint_editor_tools.h"
#include "tools/placement_tool.h"
#include "tools/picker_tool.h"
#include "tools/pad_tool.h"
#include "tools/pcb_actions.h"


BEGIN_EVENT_TABLE( FOOTPRINT_EDIT_FRAME, PCB_BASE_FRAME )
    EVT_MENU_RANGE( ID_POPUP_PCB_ITEM_SELECTION_START, ID_POPUP_PCB_ITEM_SELECTION_END,
                    PCB_BASE_FRAME::ProcessItemSelection )
    EVT_CLOSE( FOOTPRINT_EDIT_FRAME::OnCloseWindow )
    EVT_MENU( wxID_EXIT, FOOTPRINT_EDIT_FRAME::CloseModuleEditor )

    EVT_SIZE( FOOTPRINT_EDIT_FRAME::OnSize )

    EVT_CHOICE( ID_ON_ZOOM_SELECT, FOOTPRINT_EDIT_FRAME::OnSelectZoom )
    EVT_CHOICE( ID_ON_GRID_SELECT, FOOTPRINT_EDIT_FRAME::OnSelectGrid )

    EVT_TOOL( ID_MODEDIT_SAVE, FOOTPRINT_EDIT_FRAME::Process_Special_Functions )
    EVT_TOOL( ID_MODEDIT_SAVE_AS, FOOTPRINT_EDIT_FRAME::Process_Special_Functions )
    EVT_TOOL( ID_OPEN_MODULE_VIEWER, FOOTPRINT_EDIT_FRAME::Process_Special_Functions )

    EVT_TOOL( ID_MODEDIT_DELETE_PART, FOOTPRINT_EDIT_FRAME::Process_Special_Functions )
    EVT_TOOL( ID_MODEDIT_NEW_MODULE, FOOTPRINT_EDIT_FRAME::Process_Special_Functions )
    EVT_TOOL( ID_MODEDIT_NEW_MODULE_FROM_WIZARD, FOOTPRINT_EDIT_FRAME::Process_Special_Functions )
    EVT_TOOL( ID_MODEDIT_IMPORT_PART, FOOTPRINT_EDIT_FRAME::Process_Special_Functions )
    EVT_TOOL( ID_MODEDIT_EXPORT_PART, FOOTPRINT_EDIT_FRAME::Process_Special_Functions )
    EVT_TOOL( ID_MODEDIT_CREATE_NEW_LIB, FOOTPRINT_EDIT_FRAME::Process_Special_Functions )
    EVT_TOOL( ID_MODEDIT_SHEET_SET, FOOTPRINT_EDIT_FRAME::Process_Special_Functions )
    EVT_TOOL( ID_GEN_IMPORT_DXF_FILE, FOOTPRINT_EDIT_FRAME::Process_Special_Functions )
    EVT_TOOL( wxID_PRINT, FOOTPRINT_EDIT_FRAME::ToPrinter )
    EVT_TOOL( ID_MODEDIT_EDIT_MODULE, FOOTPRINT_EDIT_FRAME::Process_Special_Functions )
    EVT_TOOL( ID_MODEDIT_CHECK, FOOTPRINT_EDIT_FRAME::Process_Special_Functions )
    EVT_TOOL( ID_MODEDIT_PAD_SETTINGS, FOOTPRINT_EDIT_FRAME::Process_Special_Functions )
    EVT_TOOL( ID_MODEDIT_LOAD_MODULE_FROM_BOARD, FOOTPRINT_EDIT_FRAME::LoadModuleFromBoard )
    EVT_TOOL( ID_MODEDIT_INSERT_MODULE_IN_BOARD, FOOTPRINT_EDIT_FRAME::Process_Special_Functions )
    EVT_TOOL( ID_MODEDIT_UPDATE_MODULE_IN_BOARD, FOOTPRINT_EDIT_FRAME::Process_Special_Functions )
    EVT_TOOL( ID_MODEDIT_EDIT_MODULE_PROPERTIES, FOOTPRINT_EDIT_FRAME::Process_Special_Functions )
    EVT_TOOL( wxID_UNDO, FOOTPRINT_EDIT_FRAME::RestoreCopyFromUndoList )
    EVT_TOOL( wxID_REDO, FOOTPRINT_EDIT_FRAME::RestoreCopyFromRedoList )

    // Vertical tool bar button click event handler.
    EVT_TOOL( ID_NO_TOOL_SELECTED, FOOTPRINT_EDIT_FRAME::OnVerticalToolbar )
    EVT_TOOL( ID_ZOOM_SELECTION, FOOTPRINT_EDIT_FRAME::OnVerticalToolbar )
    EVT_TOOL_RANGE( ID_MODEDIT_PAD_TOOL, ID_MODEDIT_MEASUREMENT_TOOL,
                    FOOTPRINT_EDIT_FRAME::OnVerticalToolbar )

    // Options Toolbar
    // ID_TB_OPTIONS_SHOW_PADS_SKETCH id is managed in PCB_BASE_FRAME
    // ID_TB_OPTIONS_SHOW_MODULE_TEXT_SKETCH id is managed in PCB_BASE_FRAME
    // ID_TB_OPTIONS_SHOW_MODULE_EDGE_SKETCH id is managed in PCB_BASE_FRAME
    EVT_TOOL( ID_TB_OPTIONS_SHOW_HIGH_CONTRAST_MODE, FOOTPRINT_EDIT_FRAME::OnSelectOptionToolbar )

    // Preferences and option menus
    EVT_MENU( ID_PREFERENCES_HOTKEY_SHOW_CURRENT_LIST,
              FOOTPRINT_EDIT_FRAME::ProcessPreferences )
    EVT_MENU( ID_PCB_LIB_TABLE_EDIT,
              FOOTPRINT_EDIT_FRAME::ProcessPreferences )
    EVT_MENU( wxID_PREFERENCES,
              FOOTPRINT_EDIT_FRAME::ProcessPreferences )
    EVT_MENU( ID_PREFERENCES_CONFIGURE_PATHS, FOOTPRINT_EDIT_FRAME::OnConfigurePaths )

    // popup commands
    EVT_MENU_RANGE( ID_POPUP_PCB_START_RANGE, ID_POPUP_PCB_END_RANGE,
                    FOOTPRINT_EDIT_FRAME::Process_Special_Functions )

    EVT_MENU_RANGE( ID_POPUP_GENERAL_START_RANGE, ID_POPUP_GENERAL_END_RANGE,
                    FOOTPRINT_EDIT_FRAME::Process_Special_Functions )

    EVT_MENU( ID_POPUP_MODEDIT_EDIT_BODY_ITEM,
              FOOTPRINT_EDIT_FRAME::Process_Special_Functions )
    EVT_MENU( ID_POPUP_MODEDIT_EDIT_WIDTH_ALL_EDGE,
              FOOTPRINT_EDIT_FRAME::Process_Special_Functions )
    EVT_MENU( ID_POPUP_MODEDIT_EDIT_LAYER_ALL_EDGE,
              FOOTPRINT_EDIT_FRAME::Process_Special_Functions )

    // Module transformations
    EVT_MENU( ID_MODEDIT_MODULE_ROTATE, FOOTPRINT_EDIT_FRAME::Process_Special_Functions )
    EVT_MENU( ID_MODEDIT_MODULE_MIRROR, FOOTPRINT_EDIT_FRAME::Process_Special_Functions )
    EVT_MENU( ID_MODEDIT_MODULE_MOVE_EXACT, FOOTPRINT_EDIT_FRAME::Process_Special_Functions )

    EVT_MENU( ID_PCB_USER_GRID_SETUP, FOOTPRINT_EDIT_FRAME::Process_Special_Functions )

    // Menu Help
    EVT_MENU( wxID_HELP, EDA_DRAW_FRAME::GetKicadHelp )
    EVT_MENU( wxID_INDEX, EDA_DRAW_FRAME::GetKicadHelp )
    EVT_MENU( ID_HELP_GET_INVOLVED, EDA_DRAW_FRAME::GetKicadContribute )
    EVT_MENU( wxID_ABOUT, EDA_BASE_FRAME::GetKicadAbout )

    // Menu 3D Frame
    EVT_MENU( ID_MENU_PCB_SHOW_3D_FRAME, FOOTPRINT_EDIT_FRAME::Show3D_Frame )

    // Switching canvases
    EVT_MENU( ID_MENU_CANVAS_LEGACY, PCB_BASE_FRAME::OnSwitchCanvas )
    EVT_MENU( ID_MENU_CANVAS_CAIRO, PCB_BASE_FRAME::OnSwitchCanvas )
    EVT_MENU( ID_MENU_CANVAS_OPENGL, PCB_BASE_FRAME::OnSwitchCanvas )

    // UI update events.
    EVT_UPDATE_UI( ID_MODEDIT_EXPORT_PART, FOOTPRINT_EDIT_FRAME::OnUpdateModuleTargeted )
    EVT_UPDATE_UI( ID_MODEDIT_SAVE, FOOTPRINT_EDIT_FRAME::OnUpdateSave )
    EVT_UPDATE_UI( ID_MODEDIT_SAVE_AS, FOOTPRINT_EDIT_FRAME::OnUpdateSaveAs )
    EVT_UPDATE_UI( ID_MODEDIT_DELETE_PART, FOOTPRINT_EDIT_FRAME::OnUpdateModuleTargeted )
    EVT_UPDATE_UI( ID_MODEDIT_LOAD_MODULE_FROM_BOARD,
                   FOOTPRINT_EDIT_FRAME::OnUpdateLoadModuleFromBoard )
    EVT_UPDATE_UI( ID_MODEDIT_INSERT_MODULE_IN_BOARD,
                   FOOTPRINT_EDIT_FRAME::OnUpdateInsertModuleInBoard )
    EVT_UPDATE_UI( ID_MODEDIT_UPDATE_MODULE_IN_BOARD,
                   FOOTPRINT_EDIT_FRAME::OnUpdateReplaceModuleInBoard )
    EVT_UPDATE_UI( ID_NO_TOOL_SELECTED, FOOTPRINT_EDIT_FRAME::OnUpdateSelectTool )
    EVT_UPDATE_UI( ID_ZOOM_SELECTION, FOOTPRINT_EDIT_FRAME::OnUpdateSelectTool )

    EVT_UPDATE_UI_RANGE( ID_MODEDIT_PAD_TOOL, ID_MODEDIT_MEASUREMENT_TOOL,
                         FOOTPRINT_EDIT_FRAME::OnUpdateVerticalToolbar )

    EVT_UPDATE_UI( ID_MODEDIT_EDIT_MODULE_PROPERTIES, FOOTPRINT_EDIT_FRAME::OnUpdateModuleSelected )
    EVT_UPDATE_UI( ID_MODEDIT_PAD_SETTINGS, FOOTPRINT_EDIT_FRAME::OnUpdateModuleSelected )

    // Option toolbar:
    EVT_UPDATE_UI( ID_TB_OPTIONS_SHOW_HIGH_CONTRAST_MODE,
                   FOOTPRINT_EDIT_FRAME::OnUpdateOptionsToolbar )

    EVT_UPDATE_UI( ID_GEN_IMPORT_DXF_FILE, FOOTPRINT_EDIT_FRAME::OnUpdateModuleSelected )

END_EVENT_TABLE()


FOOTPRINT_EDIT_FRAME::FOOTPRINT_EDIT_FRAME( KIWAY* aKiway, wxWindow* aParent ) :
    PCB_BASE_EDIT_FRAME( aKiway, aParent, FRAME_PCB_MODULE_EDITOR, wxEmptyString,
                         wxDefaultPosition, wxDefaultSize,
                         KICAD_DEFAULT_DRAWFRAME_STYLE, GetFootprintEditorFrameName() )
{
    m_showBorderAndTitleBlock = false;   // true to show the frame references
    m_showAxis = true;                   // true to show X and Y axis on screen
    m_showGridAxis = true;               // show the grid origin axis
    m_hotkeysDescrList = g_Module_Editor_Hotkeys_Descr;

    // Give an icon
    wxIcon icon;
    icon.CopyFromBitmap( KiBitmap( icon_modedit_xpm ) );
    SetIcon( icon );

    // Create GAL canvas
    bool boardEditorWasRunning = Kiway().Player( FRAME_PCB, false ) != nullptr;
    PCB_BASE_FRAME* pcbFrame = static_cast<PCB_BASE_FRAME*>( Kiway().Player( FRAME_PCB, true ) );
    PCB_DRAW_PANEL_GAL* drawPanel = new PCB_DRAW_PANEL_GAL( this, -1, wxPoint( 0, 0 ), m_FrameSize,
                                                            GetGalDisplayOptions(),
                                                            pcbFrame->GetGalCanvas()->GetBackend() );
    SetGalCanvas( drawPanel );

    SetBoard( new BOARD() );
    // In modedit, the default net clearance is not known.
    // (it depends on the actual board)
    // So we do not show the default clearance, by setting it to 0
    // The footprint or pad specific clearance will be shown
    GetBoard()->GetDesignSettings().GetDefault()->SetClearance( 0 );

    // Don't show the default board solder mask clearance in the footprint editor.  Only the
    // footprint or pad clearance setting should be shown if it is not 0.
    GetBoard()->GetDesignSettings().m_SolderMaskMargin = 0;

    // restore the last footprint from the project, if any
    restoreLastFootprint();

    // Ensure all layers and items are visible:
    // In footprint editor, some layers have no meaning or
    // cannot be used, but we show all of them, at least to be able
    // to edit a bad layer
    GetBoard()->SetVisibleAlls();

    // However the "no net" mark on pads is useless, because there is
    // no net in footprint editor: make it non visible
    GetBoard()->SetElementVisibility( LAYER_NO_CONNECTS, false );

    m_Layers = new PCB_LAYER_WIDGET( this, GetCanvas(), true );

    // LoadSettings() *after* creating m_LayersManager, because LoadSettings()
    // initialize parameters in m_LayersManager
    LoadSettings( config() );
    GetGalDisplayOptions().m_axesEnabled = true;

    SetScreen( new PCB_SCREEN( GetPageSettings().GetSizeIU() ) );
    GetScreen()->SetMaxUndoItems( m_UndoRedoCountMax );
    GetScreen()->SetCurItem( NULL );

    GetScreen()->AddGrid( m_UserGridSize, EDA_UNITS_T::UNSCALED_UNITS, ID_POPUP_GRID_USER );
    GetScreen()->SetGrid( ID_POPUP_GRID_LEVEL_1000 + m_LastGridSizeId );

    // In modedit, set the default paper size to A4:
    // this should be OK for all footprint to plot/print
    SetPageSettings( PAGE_INFO( PAGE_INFO::A4 ) );

    SetSize( m_FramePos.x, m_FramePos.y, m_FrameSize.x, m_FrameSize.y );

    initLibraryTree();
    m_treePane = new FOOTPRINT_TREE_PANE( this );

    ReCreateMenuBar();
    ReCreateHToolbar();
    ReCreateAuxiliaryToolbar();
    ReCreateVToolbar();
    ReCreateOptToolbar();

    if( m_canvas )
        m_canvas->SetEnableBlockCommands( true );

    m_auimgr.SetManagedWindow( this );

    EDA_PANEINFO horiz;
    horiz.HorizontalToolbarPane();

    EDA_PANEINFO vert;
    vert.VerticalToolbarPane();

    EDA_PANEINFO mesg_pane;
    mesg_pane.MessageToolbarPane();

    // Create a wxAuiPaneInfo for the Layers Manager, not derived from the template.
    // LAYER_WIDGET is floatable, but initially docked at far right
    EDA_PANEINFO   lyrs;
    lyrs.LayersToolbarPane();
    lyrs.MinSize( m_Layers->GetBestSize() );    // updated in ReFillLayerWidget
    lyrs.BestSize( m_Layers->GetBestSize() );
    lyrs.Caption( _( "Visibles" ) );

    m_auimgr.AddPane( m_mainToolBar,
                      wxAuiPaneInfo( horiz ).Name( "m_mainToolBar" ).Top(). Row( 0 ) );

    m_auimgr.AddPane( m_auxiliaryToolBar,
                      wxAuiPaneInfo( horiz ).Name( "m_auxiliaryToolBar" ).Top().Row( 1 ) );

    // The main right vertical toolbar
    m_auimgr.AddPane( m_drawToolBar,
                      wxAuiPaneInfo( vert ).Name( "m_VToolBar" ).Right().Layer(1) );

    // Add the layer manager ( most right side of pcbframe )
    m_auimgr.AddPane( m_Layers, lyrs.Name( "m_LayersManagerToolBar" ).Right().Layer( 2 ) );
    // Layers manager is visible
    m_auimgr.GetPane( "m_LayersManagerToolBar" ).Show( true );

    // The left vertical toolbar (fast acces to display options)
    m_auimgr.AddPane( m_optionsToolBar,
                      wxAuiPaneInfo( vert ).Name( "m_optionsToolBar" ). Left().Layer(1) );

    m_auimgr.AddPane( m_canvas,
                      wxAuiPaneInfo().Name( "DrawFrame" ).CentrePane() );
    m_auimgr.AddPane( (wxWindow*) GetGalCanvas(),
                      wxAuiPaneInfo().Name( "DrawFrameGal" ).CentrePane().Hide() );

    m_auimgr.AddPane( m_messagePanel,
                      wxAuiPaneInfo( mesg_pane ).Name( "MsgPanel" ).Bottom().Layer(10) );

    m_auimgr.AddPane( m_treePane, wxAuiPaneInfo().Name( "FootprintTree" ).Left().Row( 1 )
            .Resizable().MinSize( 250, 400 ).Dock().CloseButton( false ) );

    // Create the manager and dispatcher & route draw panel events to the dispatcher
    setupTools();
    GetGalCanvas()->GetGAL()->SetAxesEnabled( true );
    UseGalCanvas( pcbFrame->IsGalCanvasActive() );

    if( m_auimgr.GetPane( "m_LayersManagerToolBar" ).IsShown() )
    {
        m_Layers->ReFill();
        m_Layers->ReFillRender();

        GetScreen()->m_Active_Layer = F_SilkS;
        m_Layers->SelectLayer( F_SilkS );
        m_Layers->OnLayerSelected();
    }

    if( !boardEditorWasRunning )
        pcbFrame->Destroy();

    m_auimgr.Update();
    updateTitle();

    Raise();            // On some window managers, this is needed
    Show( true );

    Zoom_Automatique( false );
}


FOOTPRINT_EDIT_FRAME::~FOOTPRINT_EDIT_FRAME()
{
    // save the footprint in the PROJECT
    retainLastFootprint();

    delete m_Layers;
}


BOARD_ITEM_CONTAINER* FOOTPRINT_EDIT_FRAME::GetModel() const
{
    return GetBoard()->m_Modules;
}


LIB_ID FOOTPRINT_EDIT_FRAME::getTargetLibId() const
{
    LIB_ID   id = m_treePane->GetLibTree()->GetSelectedLibId();
    wxString nickname = id.GetLibNickname();

    if( nickname.IsEmpty() )
        return GetCurrentLibId();

    return id;
}


LIB_ID FOOTPRINT_EDIT_FRAME::GetCurrentLibId() const
{
    MODULE* module = GetBoard()->m_Modules;

    if( module )
        return module->GetFPID();
    else
        return LIB_ID();
}


void FOOTPRINT_EDIT_FRAME::retainLastFootprint()
{
    LIB_ID id = GetCurrentLibId();

    if( id.IsValid() )
    {
        Prj().SetRString( PROJECT::PCB_FOOTPRINT_EDITOR_NICKNAME, id.GetLibNickname() );
        Prj().SetRString( PROJECT::PCB_FOOTPRINT_EDITOR_FPNAME, id.GetLibItemName() );
    }
}


void FOOTPRINT_EDIT_FRAME::restoreLastFootprint()
{
    const wxString& curFootprintName = Prj().GetRString( PROJECT::PCB_FOOTPRINT_EDITOR_FPNAME );
    const wxString& curNickname =  Prj().GetRString( PROJECT::PCB_FOOTPRINT_EDITOR_NICKNAME );

    if( curNickname.Length() && curFootprintName.Length() )
    {
        LIB_ID id;
        id.SetLibNickname( curNickname );
        id.SetLibItemName( curFootprintName );

        MODULE* module = loadFootprint( id );

        if( module )
            GetBoard()->Add( module );
    }
}


const wxChar* FOOTPRINT_EDIT_FRAME::GetFootprintEditorFrameName()
{
    return FOOTPRINT_EDIT_FRAME_NAME;
}


BOARD_DESIGN_SETTINGS& FOOTPRINT_EDIT_FRAME::GetDesignSettings() const
{
    return GetBoard()->GetDesignSettings();
}


void FOOTPRINT_EDIT_FRAME::SetDesignSettings( const BOARD_DESIGN_SETTINGS& aSettings )
{
    GetBoard()->SetDesignSettings( aSettings );
}


const PCB_PLOT_PARAMS& FOOTPRINT_EDIT_FRAME::GetPlotSettings() const
{
    // get the settings from the parent editor, not our BOARD.

    // @todo(DICK) change the routing to some default or the board directly, parent may not exist
    PCB_BASE_FRAME* parentFrame = (PCB_BASE_FRAME*) Kiway().Player( FRAME_PCB, true );
    wxASSERT( parentFrame );

    return parentFrame->GetPlotSettings();
}


void FOOTPRINT_EDIT_FRAME::SetPlotSettings( const PCB_PLOT_PARAMS& aSettings )
{
    // set the settings into parent editor, not our BOARD.

    // @todo(DICK) change the routing to some default or the board directly, parent may not exist
    PCB_BASE_FRAME* parentFrame = (PCB_BASE_FRAME*) Kiway().Player( FRAME_PCB, true );
    wxASSERT( parentFrame );

    parentFrame->SetPlotSettings( aSettings );
}


void FOOTPRINT_EDIT_FRAME::LoadSettings( wxConfigBase* aCfg )
{
    PCB_BASE_FRAME::LoadSettings( aCfg );
    wxConfigLoadSetups( aCfg, GetConfigurationSettings() );

    m_configSettings.Load( aCfg );  // mainly, load the color config

    // Ensure some params are valid
    BOARD_DESIGN_SETTINGS& settings = GetDesignSettings();

    // Usually, graphic items are drawn on F_SilkS or F_Fab layer
    // Force these layers if not default
    if( ( settings.m_RefDefaultlayer != F_SilkS ) && ( settings.m_RefDefaultlayer != F_Fab ) )
        settings.m_RefDefaultlayer = F_SilkS;

    if( ( settings.m_ValueDefaultlayer != F_SilkS ) && ( settings.m_ValueDefaultlayer != F_Fab ) )
        settings.m_ValueDefaultlayer = F_Fab;
}


void FOOTPRINT_EDIT_FRAME::SaveSettings( wxConfigBase* aCfg )
{
    m_configSettings.Save( aCfg );

    PCB_BASE_FRAME::SaveSettings( aCfg );
    wxConfigSaveSetups( aCfg, GetConfigurationSettings() );
}


double FOOTPRINT_EDIT_FRAME::BestZoom()
{
    EDA_RECT    ibbbox  = GetBoardBoundingBox();

    double sizeX = (double) ibbbox.GetWidth();
    double sizeY = (double) ibbbox.GetHeight();

    wxPoint centre = ibbbox.Centre();

    // Reserve a 20% margin around "board" bounding box.
    double margin_scale_factor = 1.2;
    return bestZoom( sizeX, sizeY, margin_scale_factor, centre );
}


void FOOTPRINT_EDIT_FRAME::OnCloseWindow( wxCloseEvent& Event )
{
    if( GetScreen()->IsModify() && GetBoard()->m_Modules )
    {
        int ii = DisplayExitDialog( this, _( "Save changes to footprint before closing?" ) );

        switch( ii )
        {
        default:
        case wxID_NO:
            break;

        case wxID_YES:
            if( !SaveFootprint( GetBoard()->m_Modules ) )
            {
                Event.Veto();
                return;
            }
            break;

        case wxID_CANCEL:
            Event.Veto();
            return;
        }
    }

    if( IsGalCanvasActive() )
        GetGalCanvas()->StopDrawing();

    //close the editor
    Destroy();
}


void FOOTPRINT_EDIT_FRAME::CloseModuleEditor( wxCommandEvent& Event )
{
    Close();
}


void FOOTPRINT_EDIT_FRAME::OnUpdateSelectTool( wxUpdateUIEvent& aEvent )
{
    if( aEvent.GetEventObject() == m_drawToolBar || aEvent.GetEventObject() == m_mainToolBar )
        aEvent.Check( GetToolId() == aEvent.GetId() );
}


void FOOTPRINT_EDIT_FRAME::OnUpdateVerticalToolbar( wxUpdateUIEvent& aEvent )
{
    aEvent.Enable( GetBoard()->m_Modules != NULL );

    if( aEvent.GetEventObject() == m_drawToolBar )
        aEvent.Check( GetToolId() == aEvent.GetId() );
}


void FOOTPRINT_EDIT_FRAME::OnUpdateOptionsToolbar( wxUpdateUIEvent& aEvent )
{
    int        id = aEvent.GetId();
    auto displ_opts = (PCB_DISPLAY_OPTIONS*)GetDisplayOptions();

    bool state = false;

    switch( id )
    {
    case ID_TB_OPTIONS_SHOW_HIGH_CONTRAST_MODE:
        state = displ_opts->m_ContrastModeDisplay;
        break;

    default:
        wxMessageBox( "FOOTPRINT_EDIT_FRAME::OnUpdateOptionsToolbar error" );
        break;
    }

    aEvent.Check( state );
}


void FOOTPRINT_EDIT_FRAME::OnUpdateModuleSelected( wxUpdateUIEvent& aEvent )
{
    aEvent.Enable( GetBoard()->m_Modules != NULL );
}


void FOOTPRINT_EDIT_FRAME::OnUpdateModuleTargeted( wxUpdateUIEvent& aEvent )
{
    aEvent.Enable( getTargetLibId().IsValid() );
}


void FOOTPRINT_EDIT_FRAME::OnUpdateSave( wxUpdateUIEvent& aEvent )
{
    aEvent.Enable( GetBoard()->m_Modules && GetScreen()->IsModify() );
}


void FOOTPRINT_EDIT_FRAME::OnUpdateSaveAs( wxUpdateUIEvent& aEvent )
{
    LIB_ID libId = getTargetLibId();
    const wxString& libName = libId.GetLibNickname();
    const wxString& partName = libId.GetLibItemName();

    aEvent.Enable( !libName.IsEmpty() || !partName.IsEmpty() );
}


void FOOTPRINT_EDIT_FRAME::OnUpdateLoadModuleFromBoard( wxUpdateUIEvent& aEvent )
{
    PCB_EDIT_FRAME* frame = (PCB_EDIT_FRAME*) Kiway().Player( FRAME_PCB, false );

    aEvent.Enable( frame && frame->GetBoard()->m_Modules != NULL );
}


void FOOTPRINT_EDIT_FRAME::OnUpdateInsertModuleInBoard( wxUpdateUIEvent& aEvent )
{
    PCB_EDIT_FRAME* frame = (PCB_EDIT_FRAME*) Kiway().Player( FRAME_PCB, false );

    MODULE* module_in_edit = GetBoard()->m_Modules;
    bool canInsert = frame && module_in_edit && !module_in_edit->GetLink();

    // If the source was deleted, the module can inserted but not updated in the board.
    if( frame && module_in_edit && module_in_edit->GetLink() ) // this is not a new module
    {
        BOARD*  mainpcb = frame->GetBoard();
        MODULE* source_module = mainpcb->m_Modules;

        // search if the source module was not deleted:
        for( ; source_module != NULL; source_module = source_module->Next() )
        {
            if( module_in_edit->GetLink() == source_module->GetTimeStamp() )
                break;
        }

        canInsert = ( source_module == NULL );
    }

    aEvent.Enable( canInsert );
}


void FOOTPRINT_EDIT_FRAME::OnUpdateReplaceModuleInBoard( wxUpdateUIEvent& aEvent )
{
    PCB_EDIT_FRAME* frame = (PCB_EDIT_FRAME*) Kiway().Player( FRAME_PCB, false );

    MODULE* module_in_edit = GetBoard()->m_Modules;
    bool canReplace = frame && module_in_edit && module_in_edit->GetLink();

    if( canReplace ) // this is not a new module, but verify if the source is still on board
    {
        BOARD*  mainpcb = frame->GetBoard();
        MODULE* source_module = mainpcb->m_Modules;

        // search if the source module was not deleted:
        for( ; source_module != NULL; source_module = source_module->Next() )
        {
            if( module_in_edit->GetLink() == source_module->GetTimeStamp() )
                break;
        }

        canReplace = ( source_module != NULL );
    }

    aEvent.Enable( canReplace );
}


void FOOTPRINT_EDIT_FRAME::Show3D_Frame( wxCommandEvent& event )
{
    bool forceRecreateIfNotOwner = true;
    CreateAndShow3D_Frame( forceRecreateIfNotOwner );
}


bool FOOTPRINT_EDIT_FRAME::GeneralControl( wxDC* aDC, const wxPoint& aPosition, EDA_KEY aHotKey )
{
    // Filter out the 'fake' mouse motion after a keyboard movement
    if( !aHotKey && m_movingCursorWithKeyboard )
    {
        m_movingCursorWithKeyboard = false;
        return false;
    }

    // when moving mouse, use the "magnetic" grid, unless the shift+ctrl keys is pressed
    // for next cursor position
    // ( shift or ctrl key down are PAN command with mouse wheel)
    bool snapToGrid = true;

    if( !aHotKey && wxGetKeyState( WXK_SHIFT ) && wxGetKeyState( WXK_CONTROL ) )
        snapToGrid = false;

    wxPoint oldpos = GetCrossHairPosition();
    wxPoint pos = aPosition;
    bool keyHandled = GeneralControlKeyMovement( aHotKey, &pos, snapToGrid );

    SetCrossHairPosition( pos, snapToGrid );
    RefreshCrossHair( oldpos, aPosition, aDC );

    if( aHotKey && OnHotKey( aDC, aHotKey, aPosition ) )
    {
        keyHandled = true;
    }

    UpdateStatusBar();

    return keyHandled;
}


void FOOTPRINT_EDIT_FRAME::OnModify()
{
    PCB_BASE_FRAME::OnModify();
    Update3DView();
    m_treePane->GetLibTree()->Refresh();
}


void FOOTPRINT_EDIT_FRAME::updateTitle()
{
    wxString title = _( "Footprint Editor" );
    LIB_ID   fpid = GetCurrentLibId();
    bool     writable = true;

    if( fpid.IsValid() )
    {
        try
        {
            writable = Prj().PcbFootprintLibs()->IsFootprintLibWritable( fpid.GetLibNickname() );
        }
        catch( const IO_ERROR& )
        {
            // best efforts...
        }

        title += wxString::Format( wxT( " \u2014 %s %s" ),
                                   FROM_UTF8( fpid.Format().c_str() ),
                                   writable ? wxString( wxEmptyString ) : _( "[Read Only]" ) );
    }
    else if( !fpid.GetLibItemName().empty() )
    {
        title += wxString::Format( wxT( " \u2014 %s %s" ),
                                   FROM_UTF8( fpid.GetLibItemName().c_str() ),
                                   _( "[Unsaved]" ) );
    }

    SetTitle( title );
}


void FOOTPRINT_EDIT_FRAME::updateView()
{
    auto dp = static_cast<PCB_DRAW_PANEL_GAL*>( GetGalCanvas() );
    dp->UseColorScheme( &Settings().Colors() );
    dp->DisplayBoard( GetBoard() );
    m_toolManager->ResetTools( TOOL_BASE::MODEL_RELOAD );
    m_toolManager->RunAction( ACTIONS::zoomFitScreen, true );
    updateTitle();
}


void FOOTPRINT_EDIT_FRAME::initLibraryTree()
{
    FP_LIB_TABLE*   fpTable = Prj().PcbFootprintLibs();

    WX_PROGRESS_REPORTER progressReporter( this, _( "Loading Footprint Libraries" ), 2 );
    GFootprintList.ReadFootprintFiles( fpTable, NULL, &progressReporter );
    progressReporter.Show( false );

    if( GFootprintList.GetErrorCount() )
        GFootprintList.DisplayErrors( this );

    m_adapter = FP_TREE_SYNCHRONIZING_ADAPTER::Create( this, fpTable );
    auto adapter = static_cast<FP_TREE_SYNCHRONIZING_ADAPTER*>( m_adapter.get() );

    adapter->AddLibraries();
}


void FOOTPRINT_EDIT_FRAME::syncLibraryTree( bool aProgress )
{
    FP_LIB_TABLE* fpTable = Prj().PcbFootprintLibs();
    auto          adapter = static_cast<FP_TREE_SYNCHRONIZING_ADAPTER*>( m_adapter.get() );
    LIB_ID        target = getTargetLibId();
    bool          targetSelected = ( target == m_treePane->GetLibTree()->GetSelectedLibId() );

    // Sync FOOTPRINT_INFO list to the libraries on disk
    if( aProgress )
    {
        WX_PROGRESS_REPORTER progressReporter( this, _( "Updating Footprint Libraries" ), 2 );
        GFootprintList.ReadFootprintFiles( fpTable, NULL, &progressReporter );
        progressReporter.Show( false );
    }
    else
    {
        GFootprintList.ReadFootprintFiles( fpTable, NULL, NULL );
    }

    // Sync the LIB_TREE to the FOOTPRINT_INFO list
    adapter->Sync();

    m_treePane->GetLibTree()->Unselect();
    m_treePane->Regenerate();

    if( target.IsValid() )
    {
        if( adapter->FindItem( target ) )
        {
            if( targetSelected )
                m_treePane->GetLibTree()->SelectLibId( target );
            else
                m_treePane->GetLibTree()->CenterLibId( target );
        }
        else
        {
            // Try to focus on parent
            target.SetLibItemName( wxEmptyString );
            m_treePane->GetLibTree()->CenterLibId( target );
        }
    }
}


bool FOOTPRINT_EDIT_FRAME::IsGridVisible() const
{
    return IsElementVisible( LAYER_GRID );
}


void FOOTPRINT_EDIT_FRAME::SetGridVisibility(bool aVisible)
{
    SetElementVisibility( LAYER_GRID, aVisible );
}


bool FOOTPRINT_EDIT_FRAME::IsElementVisible( GAL_LAYER_ID aElement ) const
{
    return GetBoard()->IsElementVisible( aElement );
}


void FOOTPRINT_EDIT_FRAME::SetElementVisibility( GAL_LAYER_ID aElement, bool aNewState )
{
    GetGalCanvas()->GetView()->SetLayerVisible( aElement , aNewState );
    GetBoard()->SetElementVisibility( aElement, aNewState );
    m_Layers->SetRenderState( aElement, aNewState );
}


void FOOTPRINT_EDIT_FRAME::OnUpdateLayerAlpha( wxUpdateUIEvent & )
{
    m_Layers->SyncLayerAlphaIndicators();
}


void FOOTPRINT_EDIT_FRAME::ProcessPreferences( wxCommandEvent& event )
{
    int id = event.GetId();

    switch( id )
    {
    // Hotkey IDs
    case ID_PREFERENCES_HOTKEY_SHOW_CURRENT_LIST:
        // Display current hotkey list for the footprint editor.
        DisplayHotkeyList( this, g_Module_Editor_Hotkeys_Descr );
        break;

    case ID_PCB_LIB_TABLE_EDIT:
        {
            bool tableChanged = false;
            int r = InvokePcbLibTableEditor( this, &GFootprintTable, Prj().PcbFootprintLibs() );

            if( r & 1 )
            {
                try
                {
                    FILE_OUTPUTFORMATTER sf( FP_LIB_TABLE::GetGlobalTableFileName() );

                    GFootprintTable.Format( &sf, 0 );
                    tableChanged = true;
                }
                catch( const IO_ERROR& ioe )
                {
                    wxString msg;
                    msg.Printf( _( "Error saving the global footprint library table:\n\n%s" ),
                                ioe.What().GetData() );
                    wxMessageBox( msg, _( "File Save Error" ), wxOK | wxICON_ERROR );
                }
            }

            if( r & 2 )
            {
                wxString tblName = Prj().FootprintLibTblName();

                try
                {
                    Prj().PcbFootprintLibs()->Save( tblName );
                    tableChanged = true;
                }
                catch( const IO_ERROR& ioe )
                {
                    wxString msg;
                    msg.Printf( _( "Error saving project specific footprint library table:\n\n%s" ),
                                ioe.What() );
                    wxMessageBox( msg, _( "File Save Error" ), wxOK | wxICON_ERROR );
                }
            }

            FOOTPRINT_VIEWER_FRAME* viewer;
            viewer = (FOOTPRINT_VIEWER_FRAME*)Kiway().Player( FRAME_PCB_MODULE_VIEWER, false );

            if( tableChanged )
            {
                if( viewer != NULL )
                    viewer->ReCreateLibraryList();

                syncLibraryTree( true );
            }
        }
        break;

    case wxID_PREFERENCES:
        ShowPreferences( g_Pcbnew_Editor_Hotkeys_Descr, g_Module_Editor_Hotkeys_Descr, wxT( "pcbnew" ) );
        break;

    default:
        DisplayError( this, "FOOTPRINT_EDIT_FRAME::ProcessPreferences error" );
    }
}


void FOOTPRINT_EDIT_FRAME::InstallPreferences( PAGED_DIALOG* aParent )
{
    wxTreebook* book = aParent->GetTreebook();

    book->AddPage( new PANEL_MODEDIT_SETTINGS( this, aParent ), _( "Footprint Editor" ) );
    book->AddSubPage( new PANEL_MODEDIT_DISPLAY_OPTIONS( this, aParent ), _( "Display Options" ) );
    book->AddSubPage( new PANEL_MODEDIT_DEFAULTS( this, aParent ), _( "Default Values" ) );
}


void FOOTPRINT_EDIT_FRAME::OnConfigurePaths( wxCommandEvent& aEvent )
{
    DIALOG_CONFIGURE_PATHS dlg( this, Prj().Get3DCacheManager()->GetResolver() );
    dlg.ShowModal();
}


void FOOTPRINT_EDIT_FRAME::setupTools()
{
    PCB_DRAW_PANEL_GAL* drawPanel = static_cast<PCB_DRAW_PANEL_GAL*>( GetGalCanvas() );

    // Create the manager and dispatcher & route draw panel events to the dispatcher
    m_toolManager = new TOOL_MANAGER;
    m_toolManager->SetEnvironment( GetBoard(), drawPanel->GetView(),
                                   drawPanel->GetViewControls(), this );
    m_actions = new PCB_ACTIONS();
    m_toolDispatcher = new TOOL_DISPATCHER( m_toolManager, m_actions );

    drawPanel->SetEventDispatcher( m_toolDispatcher );

    m_toolManager->RegisterTool( new COMMON_TOOLS );
    m_toolManager->RegisterTool( new SELECTION_TOOL );
    m_toolManager->RegisterTool( new ZOOM_TOOL );
    m_toolManager->RegisterTool( new EDIT_TOOL );
    m_toolManager->RegisterTool( new PAD_TOOL );
    m_toolManager->RegisterTool( new DRAWING_TOOL );
    m_toolManager->RegisterTool( new POINT_EDITOR );
    m_toolManager->RegisterTool( new PCBNEW_CONTROL );
    m_toolManager->RegisterTool( new MODULE_EDITOR_TOOLS );
    m_toolManager->RegisterTool( new ALIGN_DISTRIBUTE_TOOL );
    m_toolManager->RegisterTool( new PICKER_TOOL );
    m_toolManager->RegisterTool( new POSITION_RELATIVE_TOOL );

    m_toolManager->GetTool<PAD_TOOL>()->SetEditModules( true );
    m_toolManager->GetTool<SELECTION_TOOL>()->SetEditModules( true );
    m_toolManager->GetTool<EDIT_TOOL>()->SetEditModules( true );
    m_toolManager->GetTool<DRAWING_TOOL>()->SetEditModules( true );

    m_toolManager->InitTools();

    m_toolManager->InvokeTool( "pcbnew.InteractiveSelection" );
}


void FOOTPRINT_EDIT_FRAME::UseGalCanvas( bool aEnable )
{
    PCB_BASE_EDIT_FRAME::UseGalCanvas( aEnable );

    if( aEnable )
    {
        // Be sure the axis are enabled:
        GetGalCanvas()->GetGAL()->SetAxesEnabled( true );
        updateView();
    }

    ReCreateMenuBar();
}


void FOOTPRINT_EDIT_FRAME::CommonSettingsChanged()
{
    PCB_BASE_EDIT_FRAME::CommonSettingsChanged();

    ReCreateHToolbar();
    ReCreateAuxiliaryToolbar();
    ReCreateVToolbar();
    ReCreateOptToolbar();
    Layout();
    SendSizeEvent();
}


void FOOTPRINT_EDIT_FRAME::UpdateMsgPanel()
{
    // If a item is currently selected, displays the item info.
    // If nothing selected, display the current footprint info
    BOARD_ITEM* item = GetScreen()->GetCurItem();

    if( !item )
        item = GetBoard()->m_Modules;

    MSG_PANEL_ITEMS items;

    if( item )
    {
        item->GetMsgPanelInfo( m_UserUnits, items );
        SetMsgPanel( items );
    }
    else
        ClearMsgPanel();
}

