/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2013 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2013 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2013-2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <advanced_config.h>
#include <kiface_base.h>
#include <kiway.h>
#include <pgm_base.h>
#include <pcb_edit_frame.h>
#include <3d_viewer/eda_3d_viewer_frame.h>
#include <fp_lib_table.h>
#include <bitmaps.h>
#include <confirm.h>
#include <trace_helpers.h>
#include <pcbnew_id.h>
#include <pcbnew_settings.h>
#include <pcb_layer_box_selector.h>
#include <footprint_edit_frame.h>
#include <dialog_plot.h>
#include <dialog_find.h>
#include <dialog_footprint_properties.h>
#include <dialogs/dialog_exchange_footprints.h>
#include <dialogs/dialog_net_inspector.h>
#include <dialog_board_setup.h>
#include <invoke_pcb_dialog.h>
#include <board.h>
#include <pcb_group.h>
#include <board_design_settings.h>
#include <footprint.h>
#include <drawing_sheet/ds_proxy_view_item.h>
#include <connectivity/connectivity_data.h>
#include <wildcards_and_files_ext.h>
#include <pcb_draw_panel_gal.h>
#include <functional>
#include <pcb_painter.h>
#include <project/project_file.h>
#include <project/project_local_settings.h>
#include <python_scripting.h>
#include <settings/common_settings.h>
#include <settings/settings_manager.h>
#include <tool/tool_manager.h>
#include <tool/tool_dispatcher.h>
#include <tool/action_toolbar.h>
#include <tool/common_control.h>
#include <tool/common_tools.h>
#include <tool/selection.h>
#include <tool/zoom_tool.h>
#include <tools/pcb_selection_tool.h>
#include <tools/pcb_picker_tool.h>
#include <tools/pcb_point_editor.h>
#include <tools/edit_tool.h>
#include <tools/group_tool.h>
#include <tools/drc_tool.h>
#include <tools/global_edit_tool.h>
#include <tools/convert_tool.h>
#include <tools/drawing_tool.h>
#include <tools/pcb_control.h>
#include <tools/board_editor_control.h>
#include <tools/board_inspection_tool.h>
#include <tools/pcb_editor_conditions.h>
#include <tools/pcb_viewer_tools.h>
#include <tools/board_reannotate_tool.h>
#include <tools/placement_tool.h>
#include <tools/pad_tool.h>
#include <microwave/microwave_tool.h>
#include <tools/position_relative_tool.h>
#include <tools/properties_tool.h>
#include <tools/zone_filler_tool.h>
#include <tools/pcb_actions.h>
#include <router/router_tool.h>
#include <router/length_tuner_tool.h>
#include <autorouter/autoplace_tool.h>
#include <python/scripting/pcb_scripting_tool.h>
#include <gestfich.h>
#include <executable_names.h>
#include <netlist_reader/netlist_reader.h>
#include <wx/socket.h>
#include <wx/wupdlock.h>
#include <dialog_drc.h>     // for DIALOG_DRC_WINDOW_NAME definition
#include <ratsnest/ratsnest_view_item.h>
#include <widgets/appearance_controls.h>
#include <widgets/pcb_search_pane.h>
#include <widgets/wx_infobar.h>
#include <widgets/panel_selection_filter.h>
#include <widgets/pcb_properties_panel.h>
#include <widgets/wx_aui_utils.h>
#include <kiplatform/app.h>
#include <profile.h>
#include <view/wx_view_controls.h>
#include <footprint_viewer_frame.h>

#include <action_plugin.h>
#include "../scripting/python_scripting.h"

#include <wx/filedlg.h>

using namespace std::placeholders;


#define INSPECT_DRC_ERROR_DIALOG_NAME   wxT( "InspectDrcErrorDialog" )
#define INSPECT_CLEARANCE_DIALOG_NAME   wxT( "InspectClearanceDialog" )
#define INSPECT_CONSTRAINTS_DIALOG_NAME wxT( "InspectConstraintsDialog" )
#define FOOTPRINT_DIFF_DIALOG_NAME      wxT( "FootprintDiffDialog" )


BEGIN_EVENT_TABLE( PCB_EDIT_FRAME, PCB_BASE_FRAME )
    EVT_SOCKET( ID_EDA_SOCKET_EVENT_SERV, PCB_EDIT_FRAME::OnSockRequestServer )
    EVT_SOCKET( ID_EDA_SOCKET_EVENT, PCB_EDIT_FRAME::OnSockRequest )


    EVT_CHOICE( ID_ON_ZOOM_SELECT, PCB_EDIT_FRAME::OnSelectZoom )
    EVT_CHOICE( ID_ON_GRID_SELECT, PCB_EDIT_FRAME::OnSelectGrid )

    EVT_SIZE( PCB_EDIT_FRAME::OnSize )

    EVT_TOOL( ID_MENU_RECOVER_BOARD_AUTOSAVE, PCB_EDIT_FRAME::Files_io )

    // Menu Files:
    EVT_MENU( ID_MAIN_MENUBAR, PCB_EDIT_FRAME::Process_Special_Functions )

    EVT_MENU( ID_IMPORT_NON_KICAD_BOARD, PCB_EDIT_FRAME::Files_io )
    EVT_MENU_RANGE( ID_FILE1, ID_FILEMAX, PCB_EDIT_FRAME::OnFileHistory )
    EVT_MENU( ID_FILE_LIST_CLEAR, PCB_EDIT_FRAME::OnClearFileHistory )

    EVT_MENU( ID_GEN_EXPORT_FILE_GENCADFORMAT, PCB_EDIT_FRAME::ExportToGenCAD )
    EVT_MENU( ID_GEN_EXPORT_FILE_VRML, PCB_EDIT_FRAME::OnExportVRML )
    EVT_MENU( ID_GEN_EXPORT_FILE_IDF3, PCB_EDIT_FRAME::OnExportIDF3 )
    EVT_MENU( ID_GEN_EXPORT_FILE_STEP, PCB_EDIT_FRAME::OnExportSTEP )
    EVT_MENU( ID_GEN_EXPORT_FILE_HYPERLYNX, PCB_EDIT_FRAME::OnExportHyperlynx )

    EVT_MENU( ID_RUN_TEARDROP_TOOL, PCB_EDIT_FRAME::OnRunTeardropTool )
    EVT_MENU( ID_REMOVE_TEARDROP_TOOL, PCB_EDIT_FRAME::OnRemoveTeardropTool )

    EVT_MENU( ID_MENU_EXPORT_FOOTPRINTS_TO_LIBRARY, PCB_EDIT_FRAME::Process_Special_Functions )
    EVT_MENU( ID_MENU_EXPORT_FOOTPRINTS_TO_NEW_LIBRARY, PCB_EDIT_FRAME::Process_Special_Functions )

    EVT_MENU( wxID_EXIT, PCB_EDIT_FRAME::OnQuit )
    EVT_MENU( wxID_CLOSE, PCB_EDIT_FRAME::OnQuit )

    // menu Config
    EVT_MENU( ID_GRID_SETTINGS, PCB_EDIT_FRAME::OnGridSettings )

    // menu Postprocess
    EVT_MENU( ID_PCB_GEN_CMP_FILE, PCB_EDIT_FRAME::RecreateCmpFileFromBoard )

    // Horizontal toolbar
    EVT_TOOL( ID_GEN_PLOT_SVG, PCB_EDIT_FRAME::ExportSVG )
    EVT_TOOL( ID_AUX_TOOLBAR_PCB_SELECT_AUTO_WIDTH, PCB_EDIT_FRAME::Tracks_and_Vias_Size_Event )
    EVT_COMBOBOX( ID_TOOLBARH_PCB_SELECT_LAYER, PCB_EDIT_FRAME::Process_Special_Functions )
    EVT_CHOICE( ID_AUX_TOOLBAR_PCB_TRACK_WIDTH, PCB_EDIT_FRAME::Tracks_and_Vias_Size_Event )
    EVT_CHOICE( ID_AUX_TOOLBAR_PCB_VIA_SIZE, PCB_EDIT_FRAME::Tracks_and_Vias_Size_Event )

    // Tracks and vias sizes general options
    EVT_MENU_RANGE( ID_POPUP_PCB_SELECT_WIDTH_START_RANGE, ID_POPUP_PCB_SELECT_WIDTH_END_RANGE,
                    PCB_EDIT_FRAME::Tracks_and_Vias_Size_Event )

    // User interface update event handlers.
    EVT_UPDATE_UI( ID_TOOLBARH_PCB_SELECT_LAYER, PCB_EDIT_FRAME::OnUpdateLayerSelectBox )
    EVT_UPDATE_UI( ID_AUX_TOOLBAR_PCB_TRACK_WIDTH, PCB_EDIT_FRAME::OnUpdateSelectTrackWidth )
    EVT_UPDATE_UI( ID_AUX_TOOLBAR_PCB_VIA_SIZE, PCB_EDIT_FRAME::OnUpdateSelectViaSize )
    EVT_UPDATE_UI( ID_AUX_TOOLBAR_PCB_SELECT_AUTO_WIDTH, PCB_EDIT_FRAME::OnUpdateSelectAutoWidth )
    EVT_UPDATE_UI_RANGE( ID_POPUP_PCB_SELECT_WIDTH1, ID_POPUP_PCB_SELECT_WIDTH8,
                         PCB_EDIT_FRAME::OnUpdateSelectTrackWidth )
    EVT_UPDATE_UI_RANGE( ID_POPUP_PCB_SELECT_VIASIZE1, ID_POPUP_PCB_SELECT_VIASIZE8,
                         PCB_EDIT_FRAME::OnUpdateSelectViaSize )
    // Drop files event
    EVT_DROP_FILES( PCB_EDIT_FRAME::OnDropFiles )
END_EVENT_TABLE()


PCB_EDIT_FRAME::PCB_EDIT_FRAME( KIWAY* aKiway, wxWindow* aParent ) :
        PCB_BASE_EDIT_FRAME( aKiway, aParent, FRAME_PCB_EDITOR, _( "PCB Editor" ),
                             wxDefaultPosition, wxDefaultSize, KICAD_DEFAULT_DRAWFRAME_STYLE,
                             PCB_EDIT_FRAME_NAME ),
    m_exportNetlistAction( nullptr ),
    m_findDialog( nullptr ),
    m_inspectDrcErrorDlg( nullptr ),
    m_inspectClearanceDlg( nullptr ),
    m_inspectConstraintsDlg( nullptr ),
    m_footprintDiffDlg( nullptr ),
    m_netInspectorDlg( nullptr )
{
    m_maximizeByDefault = true;
    m_showBorderAndTitleBlock = true;   // true to display sheet references
    m_SelTrackWidthBox = nullptr;
    m_SelViaSizeBox = nullptr;
    m_SelLayerBox = nullptr;
    m_show_layer_manager_tools = true;
    m_supportsAutoSave = true;
    m_probingSchToPcb = false;
    m_show_properties = true;
    m_show_search = false;

    // We don't know what state board was in when it was last saved, so we have to
    // assume dirty
    m_ZoneFillsDirty = true;

    m_aboutTitle = _HKI( "KiCad PCB Editor" );

    // Must be created before the menus are created.
    if( ADVANCED_CFG::GetCfg().m_ShowPcbnewExportNetlist )
        m_exportNetlistAction = new TOOL_ACTION( "pcbnew.EditorControl.exportNetlist",
                                                 AS_GLOBAL, 0, "", _( "Netlist..." ),
                                                 _( "Export netlist used to update schematics" ) );

    // Create GAL canvas
    auto canvas = new PCB_DRAW_PANEL_GAL( this, -1, wxPoint( 0, 0 ), m_frameSize,
                                          GetGalDisplayOptions(),
                                          EDA_DRAW_PANEL_GAL::GAL_FALLBACK );

    SetCanvas( canvas );
    SetBoard( new BOARD() );

    wxIcon icon;
    wxIconBundle icon_bundle;

    icon.CopyFromBitmap( KiBitmap( BITMAPS::icon_pcbnew ) );
    icon_bundle.AddIcon( icon );
    icon.CopyFromBitmap( KiBitmap( BITMAPS::icon_pcbnew_32 ) );
    icon_bundle.AddIcon( icon );
    icon.CopyFromBitmap( KiBitmap( BITMAPS::icon_pcbnew_16 ) );
    icon_bundle.AddIcon( icon );

    SetIcons( icon_bundle );

    // LoadSettings() *after* creating m_LayersManager, because LoadSettings()
    // initialize parameters in m_LayersManager
    LoadSettings( config() );

    SetScreen( new PCB_SCREEN( GetPageSettings().GetSizeIU( pcbIUScale.IU_PER_MILS ) ) );

    // PCB drawings start in the upper left corner.
    GetScreen()->m_Center = false;

    setupTools();
    setupUIConditions();

    ReCreateMenuBar();
    ReCreateHToolbar();
    ReCreateAuxiliaryToolbar();
    ReCreateVToolbar();
    ReCreateOptToolbar();


    m_propertiesPanel = new PCB_PROPERTIES_PANEL( this, this );

    float proportion = GetPcbNewSettings()->m_AuiPanels.properties_splitter_proportion;
    m_propertiesPanel->SetSplitterProportion( proportion );

    m_selectionFilterPanel = new PANEL_SELECTION_FILTER( this );

    m_appearancePanel = new APPEARANCE_CONTROLS( this, GetCanvas() );
    m_searchPane = new PCB_SEARCH_PANE( this );

    m_auimgr.SetManagedWindow( this );

    CreateInfoBar();

    unsigned int auiFlags = wxAUI_MGR_DEFAULT;
#if !defined( _WIN32 )
    // Windows cannot redraw the UI fast enough during a live resize and may lead to all kinds
    // of graphical glitches.
    auiFlags |= wxAUI_MGR_LIVE_RESIZE;
#endif
    m_auimgr.SetFlags( auiFlags );

    // Rows; layers 4 - 6
    m_auimgr.AddPane( m_mainToolBar, EDA_PANE().HToolbar().Name( wxS( "MainToolbar" ) )
                      .Top().Layer( 6 ) );
    m_auimgr.AddPane( m_auxiliaryToolBar, EDA_PANE().HToolbar().Name( wxS( "AuxToolbar" ) )
                      .Top().Layer( 5 ) );
    m_auimgr.AddPane( m_messagePanel, EDA_PANE().Messages().Name( wxS( "MsgPanel" ) )
                      .Bottom().Layer( 6 ) );

    // Columns; layers 1 - 3
    m_auimgr.AddPane( m_optionsToolBar, EDA_PANE().VToolbar().Name( wxS( "OptToolbar" ) )
                      .Left().Layer( 3 ) );

    m_auimgr.AddPane( m_drawToolBar, EDA_PANE().VToolbar().Name( wxS( "ToolsToolbar" ) )
                      .Right().Layer( 3 ) );

    m_auimgr.AddPane( m_appearancePanel, EDA_PANE().Name( wxS( "LayersManager" ) )
                      .Right().Layer( 4 )
                      .Caption( _( "Appearance" ) ).PaneBorder( false )
                      .MinSize( 180, -1 ).BestSize( 180, -1 ) );

    m_auimgr.AddPane( m_selectionFilterPanel, EDA_PANE().Name( wxS( "SelectionFilter" ) )
                      .Right().Layer( 4 ).Position( 2 )
                      .Caption( _( "Selection Filter" ) ).PaneBorder( false )
                      .MinSize( 180, -1 ).BestSize( 180, -1 ) );

    m_auimgr.AddPane( m_propertiesPanel, EDA_PANE().Name( wxS( "PropertiesManager" ) )
                      .Left().Layer( 5 ).Caption( _( "Properties" ) )
                      .PaneBorder( false ).MinSize( 240, -1 ).BestSize( 300, -1 ) );

    // Center
    m_auimgr.AddPane( GetCanvas(), EDA_PANE().Canvas().Name( wxS( "DrawFrame" ) )
                      .Center() );


    m_auimgr.AddPane( m_searchPane, EDA_PANE()
                                                 .Name( SearchPaneName() )
                                                 .Bottom()
                                                 .Caption( _( "Search" ) )
                                                 .PaneBorder( false )
                                                 .MinSize( 180, -1 )
                                                 .BestSize( 180, -1 )
                                                 .FloatingSize( 480, 200 )
                                                 .CloseButton( true )
                                                 .DestroyOnClose( false ) );


    m_auimgr.GetPane( "LayersManager" ).Show( m_show_layer_manager_tools );
    m_auimgr.GetPane( "SelectionFilter" ).Show( m_show_layer_manager_tools );
    m_auimgr.GetPane( "PropertiesManager" ).Show( m_show_properties );

    m_auimgr.GetPane( SearchPaneName() ).Show( m_show_search );

    // The selection filter doesn't need to grow in the vertical direction when docked
    m_auimgr.GetPane( "SelectionFilter" ).dock_proportion = 0;

    FinishAUIInitialization();

    if( PCBNEW_SETTINGS* settings = dynamic_cast<PCBNEW_SETTINGS*>( config() ) )
    {
        if( settings->m_AuiPanels.right_panel_width > 0 )
        {
            wxAuiPaneInfo& layersManager = m_auimgr.GetPane( wxS( "LayersManager" ) );
            SetAuiPaneSize( m_auimgr, layersManager, settings->m_AuiPanels.right_panel_width, -1 );
        }

        if( settings->m_AuiPanels.properties_panel_width > 0 && m_propertiesPanel )
        {
            wxAuiPaneInfo& propertiesPanel = m_auimgr.GetPane( wxS( "PropertiesManager" ) );
            SetAuiPaneSize( m_auimgr, propertiesPanel,
                            settings->m_AuiPanels.properties_panel_width, -1 );
        }

        if( settings->m_AuiPanels.search_panel_height > 0 )
        {
            wxAuiPaneInfo& searchPane = m_auimgr.GetPane( SearchPaneName() );
            SetAuiPaneSize( m_auimgr, searchPane, -1, settings->m_AuiPanels.search_panel_height );
        }

        m_appearancePanel->SetTabIndex( settings->m_AuiPanels.appearance_panel_tab );
    }

    GetToolManager()->RunAction( ACTIONS::zoomFitScreen, false );

    // This is used temporarily to fix a client size issue on GTK that causes zoom to fit
    // to calculate the wrong zoom size.  See PCB_EDIT_FRAME::onSize().
    Bind( wxEVT_SIZE, &PCB_EDIT_FRAME::onSize, this );

    // Redraw netnames (so that they fall within the current viewport) after the viewport
    // has stopped changing.  Redrawing them without the timer moves them smoothly with scrolling,
    // making it look like the tracks are being dragged -- which we don't want.
    m_redrawNetnamesTimer.SetOwner( this );
    Connect( wxEVT_TIMER, wxTimerEventHandler( PCB_EDIT_FRAME::redrawNetnames ), nullptr, this );

    Bind( wxEVT_IDLE,
            [this]( wxIdleEvent& aEvent )
            {
                if( GetCanvas()->GetView()->GetViewport() != m_lastViewport )
                {
                    m_lastViewport = GetCanvas()->GetView()->GetViewport();
                    m_redrawNetnamesTimer.StartOnce( 500 );
                }

                // Do not forget to pass the Idle event to other clients:
                aEvent.Skip();
            } );

    resolveCanvasType();

    setupUnits( config() );

    // Ensure the Python interpreter is up to date with its environment variables
    PythonSyncEnvironmentVariables();
    PythonSyncProjectName();

    // Sync action plugins in case they changed since the last time the frame opened
    GetToolManager()->RunAction( PCB_ACTIONS::pluginsReload, true );

    GetCanvas()->SwitchBackend( m_canvasType );
    ActivateGalCanvas();

    // Default shutdown reason until a file is loaded
    KIPLATFORM::APP::SetShutdownBlockReason( this, _( "New PCB file is unsaved" ) );

    // disable Export STEP item if kicad2step does not exist
    wxString strK2S = Pgm().GetExecutablePath();

#ifdef __WXMAC__
    if( strK2S.Find( wxT( "pcbnew.app" ) ) != wxNOT_FOUND )
    {
        // On macOS, we have standalone applications inside the main bundle, so we handle that here:
        strK2S += wxT( "../../" );
    }

    strK2S += wxT( "Contents/MacOS/" );
#endif

    wxFileName appK2S( strK2S, wxT( "kicad2step" ) );

#ifdef _WIN32
    appK2S.SetExt( wxT( "exe" ) );
#endif

    // Ensure the window is on top
    Raise();

//    if( !appK2S.FileExists() )
 //       GetMenuBar()->FindItem( ID_GEN_EXPORT_FILE_STEP )->Enable( false );

    // AUI doesn't refresh properly on wxMac after changes in eb7dc6dd, so force it to
#ifdef __WXMAC__
    if( Kiface().IsSingle() )
    {
        CallAfter( [&]()
                   {
                       m_appearancePanel->OnBoardChanged();
                   } );
    }
#endif

    // Register a call to update the toolbar sizes. It can't be done immediately because
    // it seems to require some sizes calculated that aren't yet (at least on GTK).
    CallAfter( [&]()
               {
                   // Ensure the controls on the toolbars all are correctly sized
                    UpdateToolbarControlSizes();
               } );

    if( ADVANCED_CFG::GetCfg().m_ShowEventCounters )
    {
        m_eventCounterTimer = new wxTimer( this );

        Bind( wxEVT_TIMER,
                [&]( wxTimerEvent& aEvent )
                {
                    GetCanvas()->m_PaintEventCounter->Show();
                    GetCanvas()->m_PaintEventCounter->Reset();

                    KIGFX::WX_VIEW_CONTROLS* vc =
                            static_cast<KIGFX::WX_VIEW_CONTROLS*>( GetCanvas()->GetViewControls() );
                    vc->m_MotionEventCounter->Show();
                    vc->m_MotionEventCounter->Reset();

                },
                m_eventCounterTimer->GetId() );

        m_eventCounterTimer->Start( 1000 );
    }

    Bind( EDA_EVT_CLOSE_DIALOG_BOOK_REPORTER, &PCB_EDIT_FRAME::onCloseModelessBookReporterDialogs,
          this );
    Bind( EDA_EVT_CLOSE_NET_INSPECTOR_DIALOG, &PCB_EDIT_FRAME::onCloseNetInspectorDialog, this );
    Bind( EDA_EVT_UNITS_CHANGED, &PCB_EDIT_FRAME::onUnitsChanged, this );

    m_acceptedExts.emplace( KiCadPcbFileExtension, &PCB_ACTIONS::ddAppendBoard );
    m_acceptedExts.emplace( LegacyPcbFileExtension, &PCB_ACTIONS::ddAppendBoard );
    DragAcceptFiles( true );
}


PCB_EDIT_FRAME::~PCB_EDIT_FRAME()
{
    if( ADVANCED_CFG::GetCfg().m_ShowEventCounters )
    {
        // Stop the timer during destruction early to avoid potential event race conditions (that
        // do happen on windows)
        m_eventCounterTimer->Stop();
        delete m_eventCounterTimer;
    }

    // Shutdown all running tools
    if( m_toolManager )
        m_toolManager->ShutdownAllTools();

    if( GetBoard() )
        GetBoard()->RemoveAllListeners();

    delete m_selectionFilterPanel;
    delete m_appearancePanel;
    delete m_exportNetlistAction;
    delete m_propertiesPanel;
}


void PCB_EDIT_FRAME::SetBoard( BOARD* aBoard, bool aBuildConnectivity,
                               PROGRESS_REPORTER* aReporter )
{
    if( m_pcb )
        m_pcb->ClearProject();

    PCB_BASE_EDIT_FRAME::SetBoard( aBoard, aReporter );

    aBoard->SetProject( &Prj() );

    if( aBuildConnectivity )
        aBoard->BuildConnectivity();

    // reload the drawing-sheet
    SetPageSettings( aBoard->GetPageSettings() );
}


BOARD_ITEM_CONTAINER* PCB_EDIT_FRAME::GetModel() const
{
    return m_pcb;
}


void PCB_EDIT_FRAME::redrawNetnames( wxTimerEvent& aEvent )
{
    bool needs_refresh = false;

    // Don't stomp on the auto-save timer event.
    if( aEvent.GetId() == ID_AUTO_SAVE_TIMER )
    {
        aEvent.Skip();
        return;
    }

    PCBNEW_SETTINGS* cfg = dynamic_cast<PCBNEW_SETTINGS*>( Kiface().KifaceSettings() );

    if( !cfg || cfg->m_Display.m_NetNames < 2 )
        return;

    KIGFX::VIEW* view = GetCanvas()->GetView();
    double scale = view->GetScale();

    for( PCB_TRACK* track : GetBoard()->Tracks() )
    {
        double lod = track->ViewGetLOD( GetNetnameLayer( track->GetLayer() ), view );

        if( lod < scale )
            continue;

        if( lod != track->GetCachedLOD() || scale != track->GetCachedScale() )
        {
            view->Update( track, KIGFX::REPAINT );
            needs_refresh = true;
            track->SetCachedLOD( lod );
            track->SetCachedScale( scale );
        }
    }

    if( needs_refresh )
        GetCanvas()->Refresh();
}


void PCB_EDIT_FRAME::SetPageSettings( const PAGE_INFO& aPageSettings )
{
    PCB_BASE_FRAME::SetPageSettings( aPageSettings );

    // Prepare drawing-sheet template
    DS_PROXY_VIEW_ITEM* drawingSheet = new DS_PROXY_VIEW_ITEM( pcbIUScale.IU_PER_MILS,
                                                               &m_pcb->GetPageSettings(),
                                                               m_pcb->GetProject(),
                                                               &m_pcb->GetTitleBlock(),
                                                               &m_pcb->GetProperties() );

    drawingSheet->SetSheetName( std::string( GetScreenDesc().mb_str() ) );
    drawingSheet->SetSheetPath( std::string( GetFullScreenDesc().mb_str() ) );

    // A board is not like a schematic having a main page and sub sheets.
    // So for the drawing sheet, use only the first page option to display items
    drawingSheet->SetIsFirstPage( true );

    BASE_SCREEN* screen = GetScreen();

    if( screen != nullptr )
    {
        drawingSheet->SetPageNumber(TO_UTF8( screen->GetPageNumber() ) );
        drawingSheet->SetSheetCount( screen->GetPageCount() );
    }

    if( BOARD* board = GetBoard() )
        drawingSheet->SetFileName( TO_UTF8( board->GetFileName() ) );

    // PCB_DRAW_PANEL_GAL takes ownership of the drawing-sheet
    GetCanvas()->SetDrawingSheet( drawingSheet );
}


bool PCB_EDIT_FRAME::IsContentModified() const
{
    return GetScreen() && GetScreen()->IsContentModified();
}


SELECTION& PCB_EDIT_FRAME::GetCurrentSelection()
{
    return m_toolManager->GetTool<PCB_SELECTION_TOOL>()->GetSelection();
}


void PCB_EDIT_FRAME::setupTools()
{
    // Create the manager and dispatcher & route draw panel events to the dispatcher
    m_toolManager = new TOOL_MANAGER;
    m_toolManager->SetEnvironment( m_pcb, GetCanvas()->GetView(),
                                   GetCanvas()->GetViewControls(), config(), this );
    m_actions = new PCB_ACTIONS();
    m_toolDispatcher = new TOOL_DISPATCHER( m_toolManager );

    // Register tools
    m_toolManager->RegisterTool( new COMMON_CONTROL );
    m_toolManager->RegisterTool( new COMMON_TOOLS );
    m_toolManager->RegisterTool( new PCB_SELECTION_TOOL );
    m_toolManager->RegisterTool( new ZOOM_TOOL );
    m_toolManager->RegisterTool( new PCB_PICKER_TOOL );
    m_toolManager->RegisterTool( new ROUTER_TOOL );
    m_toolManager->RegisterTool( new LENGTH_TUNER_TOOL );
    m_toolManager->RegisterTool( new EDIT_TOOL );
    m_toolManager->RegisterTool( new GLOBAL_EDIT_TOOL );
    m_toolManager->RegisterTool( new PAD_TOOL );
    m_toolManager->RegisterTool( new DRAWING_TOOL );
    m_toolManager->RegisterTool( new PCB_POINT_EDITOR );
    m_toolManager->RegisterTool( new PCB_CONTROL );
    m_toolManager->RegisterTool( new BOARD_EDITOR_CONTROL );
    m_toolManager->RegisterTool( new BOARD_INSPECTION_TOOL );
    m_toolManager->RegisterTool( new BOARD_REANNOTATE_TOOL );
    m_toolManager->RegisterTool( new ALIGN_DISTRIBUTE_TOOL );
    m_toolManager->RegisterTool( new MICROWAVE_TOOL );
    m_toolManager->RegisterTool( new POSITION_RELATIVE_TOOL );
    m_toolManager->RegisterTool( new ZONE_FILLER_TOOL );
    m_toolManager->RegisterTool( new AUTOPLACE_TOOL );
    m_toolManager->RegisterTool( new DRC_TOOL );
    m_toolManager->RegisterTool( new PCB_VIEWER_TOOLS );
    m_toolManager->RegisterTool( new CONVERT_TOOL );
    m_toolManager->RegisterTool( new GROUP_TOOL );
    m_toolManager->RegisterTool( new SCRIPTING_TOOL );
    m_toolManager->RegisterTool( new PROPERTIES_TOOL );
    m_toolManager->InitTools();

    for( TOOL_BASE* tool : m_toolManager->Tools() )
    {
        if( PCB_TOOL_BASE* pcbTool = dynamic_cast<PCB_TOOL_BASE*>( tool ) )
            pcbTool->SetIsBoardEditor( true );
    }

    // Run the selection tool, it is supposed to be always active
    m_toolManager->InvokeTool( "pcbnew.InteractiveSelection" );
}


void PCB_EDIT_FRAME::setupUIConditions()
{
    PCB_BASE_EDIT_FRAME::setupUIConditions();

    ACTION_MANAGER*       mgr = m_toolManager->GetActionManager();
    PCB_EDITOR_CONDITIONS cond( this );

    wxASSERT( mgr );

#define ENABLE( x ) ACTION_CONDITIONS().Enable( x )
#define CHECK( x )  ACTION_CONDITIONS().Check( x )

    mgr->SetConditions( ACTIONS::save, ENABLE( SELECTION_CONDITIONS::ShowAlways ) );
    mgr->SetConditions( ACTIONS::undo, ENABLE( cond.UndoAvailable() ) );
    mgr->SetConditions( ACTIONS::redo, ENABLE( cond.RedoAvailable() ) );

    mgr->SetConditions( ACTIONS::toggleGrid, CHECK( cond.GridVisible() ) );
    mgr->SetConditions( ACTIONS::toggleCursorStyle, CHECK( cond.FullscreenCursor() ) );
    mgr->SetConditions( ACTIONS::togglePolarCoords, CHECK( cond.PolarCoordinates() ) );
    mgr->SetConditions( ACTIONS::millimetersUnits, CHECK( cond.Units( EDA_UNITS::MILLIMETRES ) ) );
    mgr->SetConditions( ACTIONS::inchesUnits, CHECK( cond.Units( EDA_UNITS::INCHES ) ) );
    mgr->SetConditions( ACTIONS::milsUnits, CHECK( cond.Units( EDA_UNITS::MILS ) ) );

    mgr->SetConditions( ACTIONS::cut, ENABLE( cond.HasItems() ) );
    mgr->SetConditions( ACTIONS::copy, ENABLE( cond.HasItems() ) );
    mgr->SetConditions( ACTIONS::paste,
                        ENABLE( SELECTION_CONDITIONS::Idle && cond.NoActiveTool() ) );
    mgr->SetConditions( ACTIONS::pasteSpecial,
                        ENABLE( SELECTION_CONDITIONS::Idle && cond.NoActiveTool() ) );
    mgr->SetConditions( ACTIONS::selectAll, ENABLE( cond.HasItems() ) );
    mgr->SetConditions( ACTIONS::doDelete, ENABLE( cond.HasItems() ) );
    mgr->SetConditions( ACTIONS::duplicate, ENABLE( cond.HasItems() ) );

    mgr->SetConditions( PCB_ACTIONS::group, ENABLE( SELECTION_CONDITIONS::MoreThan( 1 ) ) );
    mgr->SetConditions( PCB_ACTIONS::ungroup, ENABLE( SELECTION_CONDITIONS::HasType( PCB_GROUP_T ) ) );
    mgr->SetConditions( PCB_ACTIONS::lock, ENABLE( PCB_SELECTION_CONDITIONS::HasUnlockedItems ) );
    mgr->SetConditions( PCB_ACTIONS::unlock, ENABLE( PCB_SELECTION_CONDITIONS::HasLockedItems ) );

    mgr->SetConditions( PCB_ACTIONS::padDisplayMode, CHECK( !cond.PadFillDisplay() ) );
    mgr->SetConditions( PCB_ACTIONS::viaDisplayMode, CHECK( !cond.ViaFillDisplay() ) );
    mgr->SetConditions( PCB_ACTIONS::trackDisplayMode, CHECK( !cond.TrackFillDisplay() ) );
    mgr->SetConditions( PCB_ACTIONS::graphicsOutlines, CHECK( !cond.GraphicsFillDisplay() ) );
    mgr->SetConditions( PCB_ACTIONS::textOutlines, CHECK( !cond.TextFillDisplay() ) );

    if( SCRIPTING::IsWxAvailable() )
        mgr->SetConditions( PCB_ACTIONS::showPythonConsole, CHECK( cond.ScriptingConsoleVisible() ) );

    auto enableZoneControlConition =
        [this] ( const SELECTION& )
        {
            return GetBoard()->GetVisibleElements().Contains( LAYER_ZONES )
                    && GetDisplayOptions().m_ZoneOpacity > 0.0;
        };

    mgr->SetConditions( PCB_ACTIONS::zoneDisplayFilled,
                        ENABLE( enableZoneControlConition )
                        .Check( cond.ZoneDisplayMode( ZONE_DISPLAY_MODE::SHOW_FILLED ) ) );
    mgr->SetConditions( PCB_ACTIONS::zoneDisplayOutline,
                        ENABLE( enableZoneControlConition )
                        .Check( cond.ZoneDisplayMode( ZONE_DISPLAY_MODE::SHOW_ZONE_OUTLINE ) ) );
    mgr->SetConditions( PCB_ACTIONS::zoneDisplayFractured,
                        ENABLE( enableZoneControlConition )
                        .Check( cond.ZoneDisplayMode( ZONE_DISPLAY_MODE::SHOW_FRACTURE_BORDERS ) ) );
    mgr->SetConditions( PCB_ACTIONS::zoneDisplayTriangulated,
                        ENABLE( enableZoneControlConition )
                        .Check( cond.ZoneDisplayMode( ZONE_DISPLAY_MODE::SHOW_TRIANGULATION ) ) );

    mgr->SetConditions( ACTIONS::toggleBoundingBoxes, CHECK( cond.BoundingBoxes() ) );

    auto constrainedDrawingModeCond =
            [this]( const SELECTION& )
            {
                return GetPcbNewSettings()->m_Use45DegreeLimit;
            };

    auto enableBoardSetupCondition =
            [this] ( const SELECTION& )
            {
                if( DRC_TOOL* tool = m_toolManager->GetTool<DRC_TOOL>() )
                    return !tool->IsDRCDialogShown();

                return true;
            };

    auto boardFlippedCond =
            [this]( const SELECTION& )
            {
                return GetCanvas()->GetView()->IsMirroredX();
            };

    auto layerManagerCond =
            [this] ( const SELECTION& )
            {
                return LayerManagerShown();
            };

    auto propertiesCond =
        [this] ( const SELECTION& )
        {
            return PropertiesShown();
        };

    auto searchPaneCond =
        [this] ( const SELECTION& )
        {
            return m_auimgr.GetPane( SearchPaneName() ).IsShown();
        };

    auto highContrastCond =
            [this] ( const SELECTION& )
            {
                return GetDisplayOptions().m_ContrastModeDisplay != HIGH_CONTRAST_MODE::NORMAL;
            };

    auto globalRatsnestCond =
            [this] (const SELECTION& )
            {
                return GetPcbNewSettings()->m_Display.m_ShowGlobalRatsnest;
            };

    auto curvedRatsnestCond =
            [this] (const SELECTION& )
            {
                return GetPcbNewSettings()->m_Display.m_DisplayRatsnestLinesCurved;
            };

    auto netHighlightCond =
            [this]( const SELECTION& )
            {
                KIGFX::RENDER_SETTINGS* settings = GetCanvas()->GetView()->GetPainter()->GetSettings();
                return !settings->GetHighlightNetCodes().empty();
            };

    auto enableNetHighlightCond =
            [this]( const SELECTION& )
            {
                BOARD_INSPECTION_TOOL* tool = m_toolManager->GetTool<BOARD_INSPECTION_TOOL>();
                return tool->IsNetHighlightSet();
            };

    mgr->SetConditions( PCB_ACTIONS::toggleHV45Mode,       CHECK( constrainedDrawingModeCond ) );
    mgr->SetConditions( ACTIONS::highContrastMode,         CHECK( highContrastCond ) );
    mgr->SetConditions( PCB_ACTIONS::flipBoard,            CHECK( boardFlippedCond ) );
    mgr->SetConditions( PCB_ACTIONS::showLayersManager,    CHECK( layerManagerCond ) );
    mgr->SetConditions( PCB_ACTIONS::showRatsnest,         CHECK( globalRatsnestCond ) );
    mgr->SetConditions( PCB_ACTIONS::ratsnestLineMode,     CHECK( curvedRatsnestCond ) );
    mgr->SetConditions( PCB_ACTIONS::toggleNetHighlight,   CHECK( netHighlightCond )
                                                           .Enable( enableNetHighlightCond ) );
    mgr->SetConditions( PCB_ACTIONS::boardSetup,           ENABLE( enableBoardSetupCondition ) );
    mgr->SetConditions( PCB_ACTIONS::showProperties,       CHECK( propertiesCond ) );
    mgr->SetConditions( PCB_ACTIONS::showSearch,           CHECK( searchPaneCond ) );

    auto isArcKeepCenterMode = [this]( const SELECTION& )
    {
        return GetPcbNewSettings()->m_ArcEditMode == ARC_EDIT_MODE::KEEP_CENTER_ADJUST_ANGLE_RADIUS;
    };

    auto isArcKeepEndpointMode = [this]( const SELECTION& )
    {
        return GetPcbNewSettings()->m_ArcEditMode
               == ARC_EDIT_MODE::KEEP_ENDPOINTS_OR_START_DIRECTION;
    };

    mgr->SetConditions( PCB_ACTIONS::pointEditorArcKeepCenter,   CHECK( isArcKeepCenterMode ) );
    mgr->SetConditions( PCB_ACTIONS::pointEditorArcKeepEndpoint, CHECK( isArcKeepEndpointMode ) );

    auto isHighlightMode =
            [this]( const SELECTION& )
            {
                ROUTER_TOOL* tool = m_toolManager->GetTool<ROUTER_TOOL>();
                return tool->GetRouterMode() == PNS::RM_MarkObstacles;
            };

    auto isShoveMode =
            [this]( const SELECTION& )
            {
                ROUTER_TOOL* tool = m_toolManager->GetTool<ROUTER_TOOL>();
                return tool->GetRouterMode() == PNS::RM_Shove;
            };

    auto isWalkaroundMode =
            [this]( const SELECTION& )
            {
                ROUTER_TOOL* tool = m_toolManager->GetTool<ROUTER_TOOL>();
                return tool->GetRouterMode() == PNS::RM_Walkaround;
            };

    mgr->SetConditions( PCB_ACTIONS::routerHighlightMode,  CHECK( isHighlightMode ) );
    mgr->SetConditions( PCB_ACTIONS::routerShoveMode,      CHECK( isShoveMode ) );
    mgr->SetConditions( PCB_ACTIONS::routerWalkaroundMode, CHECK( isWalkaroundMode ) );

    auto haveNetCond =
            [] ( const SELECTION& aSel )
            {
                for( EDA_ITEM* item : aSel )
                {
                    if( BOARD_CONNECTED_ITEM* bci = dynamic_cast<BOARD_CONNECTED_ITEM*>( item ) )
                    {
                        if( bci->GetNetCode() > 0 )
                            return true;
                    }
                }

                return false;
            };

    mgr->SetConditions( PCB_ACTIONS::showNetInRatsnest,     ENABLE( haveNetCond ) );
    mgr->SetConditions( PCB_ACTIONS::hideNetInRatsnest,     ENABLE( haveNetCond ) );
    mgr->SetConditions( PCB_ACTIONS::highlightNet,          ENABLE( SELECTION_CONDITIONS::ShowAlways ) );
    mgr->SetConditions( PCB_ACTIONS::highlightNetSelection, ENABLE( SELECTION_CONDITIONS::ShowAlways ) );

    mgr->SetConditions( PCB_ACTIONS::selectNet,
                        ENABLE( SELECTION_CONDITIONS::OnlyTypes( { PCB_TRACE_T, PCB_ARC_T, PCB_VIA_T } ) ) );
    mgr->SetConditions( PCB_ACTIONS::deselectNet,
                        ENABLE( SELECTION_CONDITIONS::OnlyTypes( { PCB_TRACE_T, PCB_ARC_T, PCB_VIA_T } ) ) );
    mgr->SetConditions( PCB_ACTIONS::selectUnconnected,
                        ENABLE( SELECTION_CONDITIONS::OnlyTypes( { PCB_FOOTPRINT_T, PCB_PAD_T, PCB_TRACE_T, PCB_ARC_T, PCB_VIA_T } ) ) );
    mgr->SetConditions( PCB_ACTIONS::selectSameSheet,
                        ENABLE( SELECTION_CONDITIONS::OnlyTypes( { PCB_FOOTPRINT_T } ) ) );
    mgr->SetConditions( PCB_ACTIONS::selectOnSchematic,
                        ENABLE( SELECTION_CONDITIONS::HasTypes( { PCB_PAD_T, PCB_FOOTPRINT_T, PCB_GROUP_T } ) ) );


    SELECTION_CONDITION singleZoneCond = SELECTION_CONDITIONS::Count( 1 )
                                    && SELECTION_CONDITIONS::OnlyTypes( { PCB_ZONE_T } );

    SELECTION_CONDITION zoneMergeCond = SELECTION_CONDITIONS::MoreThan( 1 )
                                    && SELECTION_CONDITIONS::OnlyTypes( { PCB_ZONE_T } );

    mgr->SetConditions( PCB_ACTIONS::zoneDuplicate,   ENABLE( singleZoneCond ) );
    mgr->SetConditions( PCB_ACTIONS::drawZoneCutout,  ENABLE( singleZoneCond ) );
    mgr->SetConditions( PCB_ACTIONS::drawSimilarZone, ENABLE( singleZoneCond ) );
    mgr->SetConditions( PCB_ACTIONS::zoneMerge,       ENABLE( zoneMergeCond ) );
    mgr->SetConditions( PCB_ACTIONS::zoneFill,        ENABLE( SELECTION_CONDITIONS::MoreThan( 0 ) ) );
    mgr->SetConditions( PCB_ACTIONS::zoneUnfill,      ENABLE( SELECTION_CONDITIONS::MoreThan( 0 ) ) );

    mgr->SetConditions( PCB_ACTIONS::toggleHV45Mode,  CHECK( cond.Get45degMode() ) );

#define CURRENT_TOOL( action ) mgr->SetConditions( action, CHECK( cond.CurrentTool( action ) ) )

    // These tools can be used at any time to inspect the board
    CURRENT_TOOL( ACTIONS::zoomTool );
    CURRENT_TOOL( ACTIONS::measureTool );
    CURRENT_TOOL( ACTIONS::selectionTool );
    CURRENT_TOOL( PCB_ACTIONS::localRatsnestTool );


    auto isDrcRunning =
            [this] ( const SELECTION& )
            {
                DRC_TOOL* tool = m_toolManager->GetTool<DRC_TOOL>();
                return !tool->IsDRCRunning();
            };

#define CURRENT_EDIT_TOOL( action ) mgr->SetConditions( action, ACTION_CONDITIONS().Check( cond.CurrentTool( action ) ).Enable( isDrcRunning ) )

    // These tools edit the board, so they must be disabled during some operations
    CURRENT_EDIT_TOOL( ACTIONS::deleteTool );
    CURRENT_EDIT_TOOL( PCB_ACTIONS::placeFootprint );
    CURRENT_EDIT_TOOL( PCB_ACTIONS::routeSingleTrack);
    CURRENT_EDIT_TOOL( PCB_ACTIONS::routeDiffPair );
    CURRENT_EDIT_TOOL( PCB_ACTIONS::routerTuneDiffPair );
    CURRENT_EDIT_TOOL( PCB_ACTIONS::routerTuneDiffPairSkew );
    CURRENT_EDIT_TOOL( PCB_ACTIONS::routerTuneSingleTrace );
    CURRENT_EDIT_TOOL( PCB_ACTIONS::drawVia );
    CURRENT_EDIT_TOOL( PCB_ACTIONS::drawZone );
    CURRENT_EDIT_TOOL( PCB_ACTIONS::drawRuleArea );
    CURRENT_EDIT_TOOL( PCB_ACTIONS::drawLine );
    CURRENT_EDIT_TOOL( PCB_ACTIONS::drawRectangle );
    CURRENT_EDIT_TOOL( PCB_ACTIONS::drawCircle );
    CURRENT_EDIT_TOOL( PCB_ACTIONS::drawArc );
    CURRENT_EDIT_TOOL( PCB_ACTIONS::drawPolygon );
    CURRENT_EDIT_TOOL( PCB_ACTIONS::placeImage );
    CURRENT_EDIT_TOOL( PCB_ACTIONS::placeText );
    CURRENT_EDIT_TOOL( PCB_ACTIONS::drawTextBox );
    CURRENT_EDIT_TOOL( PCB_ACTIONS::drawAlignedDimension );
    CURRENT_EDIT_TOOL( PCB_ACTIONS::drawOrthogonalDimension );
    CURRENT_EDIT_TOOL( PCB_ACTIONS::drawCenterDimension );
    CURRENT_EDIT_TOOL( PCB_ACTIONS::drawRadialDimension );
    CURRENT_EDIT_TOOL( PCB_ACTIONS::drawLeader );
    CURRENT_EDIT_TOOL( PCB_ACTIONS::drillOrigin );
    CURRENT_EDIT_TOOL( PCB_ACTIONS::gridSetOrigin );

    CURRENT_EDIT_TOOL( PCB_ACTIONS::microwaveCreateLine );
    CURRENT_EDIT_TOOL( PCB_ACTIONS::microwaveCreateGap );
    CURRENT_EDIT_TOOL( PCB_ACTIONS::microwaveCreateStub );
    CURRENT_EDIT_TOOL( PCB_ACTIONS::microwaveCreateStubArc );
    CURRENT_EDIT_TOOL( PCB_ACTIONS::microwaveCreateFunctionShape );

#undef CURRENT_TOOL
#undef CURRENT_EDIT_TOOL
#undef ENABLE
#undef CHECK
}


void PCB_EDIT_FRAME::OnQuit( wxCommandEvent& event )
{
    if( event.GetId() == wxID_EXIT )
        Kiway().OnKiCadExit();

    if( event.GetId() == wxID_CLOSE || Kiface().IsSingle() )
        Close( false );
}


void PCB_EDIT_FRAME::RecordDRCExclusions()
{
    BOARD_DESIGN_SETTINGS& bds = GetBoard()->GetDesignSettings();
    bds.m_DrcExclusions.clear();

    for( PCB_MARKER* marker : GetBoard()->Markers() )
    {
        if( marker->IsExcluded() )
            bds.m_DrcExclusions.insert( marker->Serialize() );
    }
}


void PCB_EDIT_FRAME::ResolveDRCExclusions( bool aCreateMarkers )
{
    BOARD_COMMIT commit( this );

    for( PCB_MARKER* marker : GetBoard()->ResolveDRCExclusions( aCreateMarkers ) )
    {
        if( marker->GetMarkerType() == MARKER_BASE::MARKER_DRAWING_SHEET )
            marker->GetRCItem()->SetItems( GetCanvas()->GetDrawingSheet() );

        commit.Add( marker );
    }

    commit.Push( wxEmptyString, SKIP_UNDO | SKIP_SET_DIRTY );

    for( PCB_MARKER* marker : GetBoard()->Markers() )
    {
        if( marker->GetSeverity() == RPT_SEVERITY_EXCLUSION )
            GetCanvas()->GetView()->Update( marker );
    }

    GetBoard()->UpdateRatsnestExclusions();
}


bool PCB_EDIT_FRAME::canCloseWindow( wxCloseEvent& aEvent )
{
    // Shutdown blocks must be determined and vetoed as early as possible
    if( KIPLATFORM::APP::SupportsShutdownBlockReason() && aEvent.GetId() == wxEVT_QUERY_END_SESSION
            && IsContentModified() )
    {
        return false;
    }

    ZONE_FILLER_TOOL* zoneFillerTool = m_toolManager->GetTool<ZONE_FILLER_TOOL>();

    if( zoneFillerTool->IsBusy() )
    {
        wxBell();

        if( wxWindow* reporter = dynamic_cast<wxWindow*>( zoneFillerTool->GetProgressReporter() ) )
            reporter->ShowWithEffect( wxSHOW_EFFECT_EXPAND );

        return false;
    }

    if( Kiface().IsSingle() )
    {
        auto* fpEditor = (FOOTPRINT_EDIT_FRAME*) Kiway().Player( FRAME_FOOTPRINT_EDITOR, false );

        if( fpEditor && !fpEditor->Close() )   // Can close footprint editor?
            return false;

        auto* fpViewer = (FOOTPRINT_VIEWER_FRAME*) Kiway().Player( FRAME_FOOTPRINT_VIEWER, false );

        if( fpViewer && !fpViewer->Close() )   // Can close footprint viewer?
            return false;

        fpViewer = (FOOTPRINT_VIEWER_FRAME*) Kiway().Player( FRAME_FOOTPRINT_VIEWER_MODAL, false );

        if( fpViewer && !fpViewer->Close() )   // Can close modal footprint viewer?
            return false;
    }
    else
    {
        auto* fpEditor = (FOOTPRINT_EDIT_FRAME*) Kiway().Player( FRAME_FOOTPRINT_EDITOR, false );

        if( fpEditor && fpEditor->IsCurrentFPFromBoard() )
        {
            if( !fpEditor->CanCloseFPFromBoard( true ) )
                return false;
        }
    }

    if( IsContentModified() )
    {
        wxFileName fileName = GetBoard()->GetFileName();
        wxString msg = _( "Save changes to '%s' before closing?" );

        if( !HandleUnsavedChanges( this, wxString::Format( msg, fileName.GetFullName() ),
                                   [&]() -> bool
                                   {
                                       return Files_io_from_id( ID_SAVE_BOARD );
                                   } ) )
        {
            return false;
        }
    }

    return PCB_BASE_EDIT_FRAME::canCloseWindow( aEvent );
}


void PCB_EDIT_FRAME::doCloseWindow()
{
    // On Windows 7 / 32 bits, on OpenGL mode only, Pcbnew crashes
    // when closing this frame if a footprint was selected, and the footprint editor called
    // to edit this footprint, and when closing pcbnew if this footprint is still selected
    // See https://bugs.launchpad.net/kicad/+bug/1655858
    // I think this is certainly a OpenGL event fired after frame deletion, so this workaround
    // avoid the crash (JPC)
    GetCanvas()->SetEvtHandlerEnabled( false );

    GetCanvas()->StopDrawing();

    // Clean up mode-less dialogs.
    Unbind( EDA_EVT_CLOSE_DIALOG_BOOK_REPORTER,
            &PCB_EDIT_FRAME::onCloseModelessBookReporterDialogs, this );
    Unbind( EDA_EVT_CLOSE_NET_INSPECTOR_DIALOG, &PCB_EDIT_FRAME::onCloseNetInspectorDialog, this );
    Unbind( EDA_EVT_UNITS_CHANGED, &PCB_EDIT_FRAME::onUnitsChanged, this );

    wxWindow* open_dlg = wxWindow::FindWindowByName( DIALOG_DRC_WINDOW_NAME );

    if( open_dlg )
        open_dlg->Close( true );

    if( m_findDialog )
    {
        m_findDialog->Destroy();
        m_findDialog = nullptr;
    }

    if( m_inspectDrcErrorDlg )
    {
        m_inspectDrcErrorDlg->Destroy();
        m_inspectDrcErrorDlg = nullptr;
    }

    if( m_inspectClearanceDlg )
    {
        m_inspectClearanceDlg->Destroy();
        m_inspectClearanceDlg = nullptr;
    }

    if( m_inspectConstraintsDlg )
    {
        m_inspectConstraintsDlg->Destroy();
        m_inspectConstraintsDlg = nullptr;
    }

    if( m_footprintDiffDlg )
    {
        m_footprintDiffDlg->Destroy();
        m_footprintDiffDlg = nullptr;
    }

    if( m_netInspectorDlg )
    {
        RemoveBoardChangeListener( m_netInspectorDlg );
        m_netInspectorDlg->Destroy();
        m_netInspectorDlg = nullptr;
    }

    // Delete the auto save file if it exists.
    wxFileName fn = GetBoard()->GetFileName();

    // Auto save file name is the normal file name prefixed with 'GetAutoSaveFilePrefix()'.
    fn.SetName( GetAutoSaveFilePrefix() + fn.GetName() );

    // When the auto save feature does not have write access to the board file path, it falls
    // back to a platform specific user temporary file path.
    if( !fn.IsOk() || !fn.IsDirWritable() )
        fn.SetPath( wxFileName::GetTempDir() );

    wxLogTrace( traceAutoSave, wxT( "Deleting auto save file <" ) + fn.GetFullPath() + wxT( ">" ) );

    // Remove the auto save file on a normal close of Pcbnew.
    if( fn.FileExists() && !wxRemoveFile( fn.GetFullPath() ) )
    {
        wxLogTrace( traceAutoSave, wxT( "The auto save file could not be removed!" ) );
    }

    // Make sure local settings are persisted
    SaveProjectLocalSettings();

    // Do not show the layer manager during closing to avoid flicker
    // on some platforms (Windows) that generate useless redraw of items in
    // the Layer Manager
    if( m_show_layer_manager_tools )
    {
        m_auimgr.GetPane( wxS( "LayersManager" ) ).Show( false );
        m_auimgr.GetPane( wxS( "TabbedPanel" ) ).Show( false );
    }

    // Unlink the old project if needed
    GetBoard()->ClearProject();

    // Delete board structs and undo/redo lists, to avoid crash on exit
    // when deleting some structs (mainly in undo/redo lists) too late
    Clear_Pcb( false, true );

    // do not show the window because ScreenPcb will be deleted and we do not
    // want any paint event
    Show( false );

    PCB_BASE_EDIT_FRAME::doCloseWindow();
}


void PCB_EDIT_FRAME::ActivateGalCanvas()
{
    PCB_BASE_EDIT_FRAME::ActivateGalCanvas();
    GetCanvas()->UpdateColors();
    GetCanvas()->Refresh();
}


void PCB_EDIT_FRAME::ShowBoardSetupDialog( const wxString& aInitialPage )
{
    std::unique_lock<std::mutex> dialogLock( DIALOG_BOARD_SETUP::g_Mutex, std::try_to_lock );

    // One DIALOG_BOARD_SETUP dialog at a time.
    if( !dialogLock.owns_lock() )
        return;

    // Make sure everything's up-to-date
    GetBoard()->BuildListOfNets();

    DIALOG_BOARD_SETUP dlg( this );

    if( !aInitialPage.IsEmpty() )
        dlg.SetInitialPage( aInitialPage, wxEmptyString );

    if( dlg.ShowQuasiModal() == wxID_OK )
    {
        GetBoard()->SynchronizeNetsAndNetClasses( true );
        // We don't know if anything was modified, so err on the side of requiring a save
        OnModify();

        Kiway().CommonSettingsChanged( false, true );

        PCBNEW_SETTINGS* settings = GetPcbNewSettings();
        static LSET      maskAndPasteLayers = LSET( 4, F_Mask, F_Paste, B_Mask, B_Paste );

        GetCanvas()->GetView()->UpdateAllItemsConditionally(
                [&]( KIGFX::VIEW_ITEM* aItem ) -> int
                {
                    if( dynamic_cast<PCB_TRACK*>( aItem ) )
                    {
                        if( settings->m_Display.m_TrackClearance == SHOW_WITH_VIA_ALWAYS )
                            return KIGFX::REPAINT;
                    }
                    else if( dynamic_cast<PAD*>( aItem ) )
                    {
                        if( settings->m_Display.m_PadClearance )
                            return KIGFX::REPAINT;

                        // Note: KIGFX::REPAINT isn't enough for things that go from invisible
                        // to visible as they won't be found in the view layer's itemset for
                        // re-painting.
                        if( ( GetBoard()->GetVisibleLayers() & maskAndPasteLayers ).any() )
                            return KIGFX::ALL;
                    }

                    EDA_TEXT* text = dynamic_cast<EDA_TEXT*>( aItem );

                    if( text && text->HasTextVars() )
                    {
                        text->ClearRenderCache();
                        text->ClearBoundingBoxCache();
                        return KIGFX::GEOMETRY | KIGFX::REPAINT;
                    }

                    return 0;
                } );

        GetCanvas()->Refresh();

        UpdateUserInterface();
        ReCreateAuxiliaryToolbar();
        m_toolManager->ResetTools( TOOL_BASE::MODEL_RELOAD );

        //this event causes the routing tool to reload its design rules information
        TOOL_EVENT toolEvent( TC_COMMAND, TA_MODEL_CHANGE, AS_ACTIVE );
        toolEvent.SetHasPosition( false );
        m_toolManager->ProcessEvent( toolEvent );
    }

    GetCanvas()->SetFocus();
}


void PCB_EDIT_FRAME::LoadSettings( APP_SETTINGS_BASE* aCfg )
{
    PCB_BASE_FRAME::LoadSettings( aCfg );

    PCBNEW_SETTINGS* cfg = dynamic_cast<PCBNEW_SETTINGS*>( aCfg );
    wxASSERT( cfg );

    if( cfg )
    {
        m_show_layer_manager_tools = cfg->m_AuiPanels.show_layer_manager;
        m_show_properties          = cfg->m_AuiPanels.show_properties;
        m_show_search              = cfg->m_AuiPanels.show_search;
    }
}


void PCB_EDIT_FRAME::SaveSettings( APP_SETTINGS_BASE* aCfg )
{
    PCB_BASE_FRAME::SaveSettings( aCfg );

    PCBNEW_SETTINGS* cfg = dynamic_cast<PCBNEW_SETTINGS*>( aCfg );
    wxASSERT( cfg );

    if( cfg )
    {
        cfg->m_AuiPanels.show_layer_manager   = m_show_layer_manager_tools;
        cfg->m_AuiPanels.right_panel_width    = m_appearancePanel->GetSize().x;
        cfg->m_AuiPanels.appearance_panel_tab = m_appearancePanel->GetTabIndex();

        if( m_propertiesPanel )
        {
            cfg->m_AuiPanels.show_properties        = m_show_properties;
            cfg->m_AuiPanels.properties_panel_width = m_propertiesPanel->GetSize().x;

            cfg->m_AuiPanels.properties_splitter_proportion =
                    m_propertiesPanel->SplitterProportion();
        }

        // ensure m_show_search is up to date (the pane can be closed)
        m_show_search = m_auimgr.GetPane( SearchPaneName() ).IsShown();
        cfg->m_AuiPanels.show_search = m_show_search;
        cfg->m_AuiPanels.search_panel_height = m_searchPane->GetSize().y;
    }
}


EDA_ANGLE PCB_EDIT_FRAME::GetRotationAngle() const
{
    PCBNEW_SETTINGS* cfg = dynamic_cast<PCBNEW_SETTINGS*>( config() );

    return cfg ? cfg->m_RotationAngle : ANGLE_90;
}


COLOR4D PCB_EDIT_FRAME::GetGridColor()
{
    return GetColorSettings()->GetColor( LAYER_GRID );
}


void PCB_EDIT_FRAME::SetGridColor( const COLOR4D& aColor )
{

    GetColorSettings()->SetColor( LAYER_GRID, aColor );
    GetCanvas()->GetGAL()->SetGridColor( aColor );
}


void PCB_EDIT_FRAME::SetActiveLayer( PCB_LAYER_ID aLayer )
{
    PCB_LAYER_ID oldLayer = GetActiveLayer();

    if( oldLayer == aLayer )
        return;

    PCB_BASE_FRAME::SetActiveLayer( aLayer );

    m_appearancePanel->OnLayerChanged();

    m_toolManager->RunAction( PCB_ACTIONS::layerChanged );  // notify other tools
    GetCanvas()->SetFocus();                                // allow capture of hotkeys
    GetCanvas()->SetHighContrastLayer( aLayer );

    GetCanvas()->GetView()->UpdateAllItemsConditionally(
            [&]( KIGFX::VIEW_ITEM* aItem ) -> int
            {
                BOARD_ITEM* item = dynamic_cast<BOARD_ITEM*>( aItem );

                if( !item )
                    return 0;

                // Note: KIGFX::REPAINT isn't enough for things that go from invisible to visible
                // as they won't be found in the view layer's itemset for re-painting.
                if( GetDisplayOptions().m_ContrastModeDisplay == HIGH_CONTRAST_MODE::HIDDEN )
                {
                    if( item->IsOnLayer( oldLayer, true ) || item->IsOnLayer( aLayer, true ) )
                        return KIGFX::ALL;
                }

                if( item->Type() == PCB_VIA_T )
                {
                    PCB_VIA* via = static_cast<PCB_VIA*>( item );

                    // Vias on a restricted layer set must be redrawn when the active layer
                    // is changed
                    if( via->GetViaType() == VIATYPE::BLIND_BURIED
                            || via->GetViaType() == VIATYPE::MICROVIA )
                    {
                        return KIGFX::REPAINT;
                    }
                }
                else if( item->Type() == PCB_PAD_T )
                {
                    PAD* pad = static_cast<PAD*>( item );

                    // Clearances could be layer-dependent so redraw them when the active layer
                    // is changed
                    if( GetPcbNewSettings()->m_Display.m_PadClearance )
                    {
                        // Round-corner rects are expensive to draw, but are mostly found on
                        // SMD pads which only need redrawing on an active-to-not-active
                        // switch.
                        if( pad->GetAttribute() == PAD_ATTRIB::SMD )
                        {
                            if( ( oldLayer == F_Cu || aLayer == F_Cu ) && pad->IsOnLayer( F_Cu ) )
                                return KIGFX::REPAINT;

                            if( ( oldLayer == B_Cu || aLayer == B_Cu ) && pad->IsOnLayer( B_Cu ) )
                                return KIGFX::REPAINT;
                        }
                        else if( pad->IsOnLayer( oldLayer ) || pad->IsOnLayer( aLayer ) )
                        {
                            return KIGFX::REPAINT;
                        }
                    }
                }
                else if( item->Type() == PCB_TRACE_T || item->Type() == PCB_ARC_T )
                {
                    PCB_TRACK* track = static_cast<PCB_TRACK*>( item );

                    // Clearances could be layer-dependent so redraw them when the active layer
                    // is changed
                    if( GetPcbNewSettings()->m_Display.m_TrackClearance )
                    {
                        // Tracks aren't particularly expensive to draw, but it's an easy check.
                        if( track->IsOnLayer( oldLayer ) || track->IsOnLayer( aLayer ) )
                            return KIGFX::REPAINT;
                    }
                }

                return 0;
            } );

    GetCanvas()->Refresh();
}


void PCB_EDIT_FRAME::onBoardLoaded()
{
    // JEY TODO: move this global to the board
    ENUM_MAP<PCB_LAYER_ID>& layerEnum = ENUM_MAP<PCB_LAYER_ID>::Instance();

    layerEnum.Choices().Clear();
    layerEnum.Undefined( UNDEFINED_LAYER );

    for( LSEQ seq = LSET::AllLayersMask().Seq(); seq; ++seq )
    {
        // Canonical name
        layerEnum.Map( *seq, LSET::Name( *seq ) );

        // User name
        layerEnum.Map( *seq, GetBoard()->GetLayerName( *seq ) );
    }

    DRC_TOOL* drcTool = m_toolManager->GetTool<DRC_TOOL>();

    try
    {
        drcTool->GetDRCEngine()->InitEngine( GetDesignRulesPath() );
    }
    catch( PARSE_ERROR& )
    {
        // Not sure this is the best place to tell the user their rules are buggy, so
        // we'll stay quiet for now.  Feel free to revisit this decision....
    }

    UpdateTitle();

    wxFileName fn = GetBoard()->GetFileName();

    // Display a warning that the file is read only
    if( fn.FileExists() && !fn.IsFileWritable() )
    {
        m_infoBar->RemoveAllButtons();
        m_infoBar->AddCloseButton();
        m_infoBar->ShowMessage( _( "Board file is read only." ),
                                wxICON_WARNING, WX_INFOBAR::MESSAGE_TYPE::OUTDATED_SAVE );
    }

    ReCreateLayerBox();

    // Sync layer and item visibility
    GetCanvas()->SyncLayersVisibility( m_pcb );

    SetElementVisibility( LAYER_RATSNEST, GetPcbNewSettings()->m_Display.m_ShowGlobalRatsnest );

    m_appearancePanel->OnBoardChanged();

    // Apply saved display state to the appearance panel after it has been set up
    PROJECT_LOCAL_SETTINGS& localSettings = Prj().GetLocalSettings();

    m_appearancePanel->ApplyLayerPreset( localSettings.m_ActiveLayerPreset );

    if( GetBoard()->GetDesignSettings().IsLayerEnabled( localSettings.m_ActiveLayer ) )
        SetActiveLayer( localSettings.m_ActiveLayer );

    // Updates any auto dimensions and the auxiliary toolbar tracks/via sizes
    unitsChangeRefresh();

    // Display the loaded board:
    Zoom_Automatique( false );

    // Invalidate painting as loading the DRC engine will cause clearances to become valid
    GetCanvas()->GetView()->UpdateAllItems( KIGFX::ALL );

    Refresh();

    SetMsgPanel( GetBoard() );
    SetStatusText( wxEmptyString );

    KIPLATFORM::APP::SetShutdownBlockReason( this, _( "PCB file changes are unsaved" ) );
}


void PCB_EDIT_FRAME::OnDisplayOptionsChanged()
{
    m_appearancePanel->UpdateDisplayOptions();
}


bool PCB_EDIT_FRAME::IsElementVisible( GAL_LAYER_ID aElement ) const
{
    return GetBoard()->IsElementVisible( aElement );
}


void PCB_EDIT_FRAME::SetElementVisibility( GAL_LAYER_ID aElement, bool aNewState )
{
    // Force the RATSNEST visible
    if( aElement == LAYER_RATSNEST )
        GetCanvas()->GetView()->SetLayerVisible( aElement, true );
    else
        GetCanvas()->GetView()->SetLayerVisible( aElement , aNewState );

    GetBoard()->SetElementVisibility( aElement, aNewState );
}


void PCB_EDIT_FRAME::ShowChangedLanguage()
{
    // call my base class
    PCB_BASE_EDIT_FRAME::ShowChangedLanguage();

    m_auimgr.GetPane( m_appearancePanel ).Caption( _( "Appearance" ) );
    m_auimgr.GetPane( m_selectionFilterPanel ).Caption( _( "Selection Filter" ) );
    m_auimgr.GetPane( m_propertiesPanel ).Caption( _( "Properties" ) );
    m_auimgr.Update();

    m_appearancePanel->OnLanguageChanged();
    m_selectionFilterPanel->OnLanguageChanged();
    m_propertiesPanel->OnLanguageChanged();

    UpdateTitle();
}


wxString PCB_EDIT_FRAME::GetLastPath( LAST_PATH_TYPE aType )
{
    PROJECT_FILE& project = Prj().GetProjectFile();

    if( project.m_PcbLastPath[ aType ].IsEmpty() )
        return wxEmptyString;

    wxFileName absoluteFileName = project.m_PcbLastPath[ aType ];
    wxFileName pcbFileName = GetBoard()->GetFileName();

    absoluteFileName.MakeAbsolute( pcbFileName.GetPath() );
    return absoluteFileName.GetFullPath();
}


void PCB_EDIT_FRAME::SetLastPath( LAST_PATH_TYPE aType, const wxString& aLastPath )
{
    PROJECT_FILE& project = Prj().GetProjectFile();

    wxFileName relativeFileName = aLastPath;
    wxFileName pcbFileName = GetBoard()->GetFileName();

    relativeFileName.MakeRelativeTo( pcbFileName.GetPath() );

    if( relativeFileName.GetFullPath() != project.m_PcbLastPath[ aType ] )
    {
        project.m_PcbLastPath[ aType ] = relativeFileName.GetFullPath();
        OnModify();
    }
}


void PCB_EDIT_FRAME::OnModify()
{
    PCB_BASE_FRAME::OnModify();

    Update3DView( true, GetPcbNewSettings()->m_Display.m_Live3DRefresh );

    if( !GetTitle().StartsWith( wxT( "*" ) ) )
        UpdateTitle();

    m_ZoneFillsDirty = true;
}


void PCB_EDIT_FRAME::HardRedraw()
{
    Update3DView( true, true );
}


void PCB_EDIT_FRAME::ExportSVG( wxCommandEvent& event )
{
    InvokeExportSVG( this, GetBoard() );
}


void PCB_EDIT_FRAME::UpdateTitle()
{
    wxFileName fn = GetBoard()->GetFileName();
    bool       readOnly = false;
    bool       unsaved = false;

    if( fn.IsOk() && fn.FileExists() )
        readOnly = !fn.IsFileWritable();
    else
        unsaved = true;

    wxString title;

    if( IsContentModified() )
        title = wxT( "*" );

    title += fn.GetName();

    if( readOnly )
        title += wxS( " " ) + _( "[Read Only]" );

    if( unsaved )
        title += wxS( " " ) + _( "[Unsaved]" );

    title += wxT( " \u2014 " ) + _( "PCB Editor" );

    SetTitle( title );
}


void PCB_EDIT_FRAME::UpdateUserInterface()
{
    // Update the layer manager and other widgets from the board setup
    // (layer and items visibility, colors ...)

    // Rebuild list of nets (full ratsnest rebuild)
    GetBoard()->BuildConnectivity();

    // Update info shown by the horizontal toolbars
    ReCreateLayerBox();

    LSET activeLayers = GetBoard()->GetEnabledLayers();

    if( !activeLayers.test( GetActiveLayer() ) )
        SetActiveLayer( activeLayers.Seq().front() );

    m_SelLayerBox->SetLayerSelection( GetActiveLayer() );

    ENUM_MAP<PCB_LAYER_ID>& layerEnum = ENUM_MAP<PCB_LAYER_ID>::Instance();

    layerEnum.Choices().Clear();
    layerEnum.Undefined( UNDEFINED_LAYER );

    for( LSEQ seq = LSET::AllLayersMask().Seq(); seq; ++seq )
    {
        // Canonical name
        layerEnum.Map( *seq, LSET::Name( *seq ) );

        // User name
        layerEnum.Map( *seq, GetBoard()->GetLayerName( *seq ) );
    }

    // Sync visibility with canvas
    for( PCB_LAYER_ID layer : LSET::AllLayersMask().Seq() )
        GetCanvas()->GetView()->SetLayerVisible( layer, GetBoard()->IsLayerVisible( layer ) );

    // Stackup and/or color theme may have changed
    m_appearancePanel->OnBoardChanged();
}


void PCB_EDIT_FRAME::SwitchCanvas( EDA_DRAW_PANEL_GAL::GAL_TYPE aCanvasType )
{
    // switches currently used canvas (Cairo / OpenGL).
    PCB_BASE_FRAME::SwitchCanvas( aCanvasType );
}


void PCB_EDIT_FRAME::ShowFindDialog()
{
    if( !m_findDialog )
    {
        m_findDialog = new DIALOG_FIND( this );
        m_findDialog->SetCallback( std::bind( &PCB_SELECTION_TOOL::FindItem,
                                              m_toolManager->GetTool<PCB_SELECTION_TOOL>(), _1 ) );
    }

    wxString findString;

    PCB_SELECTION& selection = m_toolManager->GetTool<PCB_SELECTION_TOOL>()->GetSelection();

    if( selection.Size() == 1 )
    {
        EDA_ITEM* front = selection.Front();

        switch( front->Type() )
        {
        case PCB_FOOTPRINT_T:
            findString = UnescapeString( static_cast<FOOTPRINT*>( front )->GetValue() );
            break;

        case PCB_TEXT_T:
            findString = UnescapeString( static_cast<PCB_TEXT*>( front )->GetText() );

            if( findString.Contains( wxT( "\n" ) ) )
                findString = findString.Before( '\n' );

            break;

        default:
            break;
        }
    }

    m_findDialog->Preload( findString );

    m_findDialog->Show( true );
}


void PCB_EDIT_FRAME::FindNext( bool reverse )
{
    if( !m_findDialog )
        ShowFindDialog();

    m_findDialog->FindNext( reverse );
}


void PCB_EDIT_FRAME::ToPlotter( int aID )
{
    PCB_PLOT_PARAMS plotSettings = GetPlotSettings();

    switch( aID )
    {
    case ID_GEN_PLOT_GERBER:
        plotSettings.SetFormat( PLOT_FORMAT::GERBER );
        break;
    case ID_GEN_PLOT_DXF:
        plotSettings.SetFormat( PLOT_FORMAT::DXF );
        break;
    case ID_GEN_PLOT_HPGL:
        plotSettings.SetFormat( PLOT_FORMAT::HPGL );
        break;
    case ID_GEN_PLOT_PDF:
        plotSettings.SetFormat( PLOT_FORMAT::PDF );
        break;
    case ID_GEN_PLOT_PS:
        plotSettings.SetFormat( PLOT_FORMAT::POST );
        break;
    case ID_GEN_PLOT:
        /* keep the previous setup */
        break;
    default:
        wxFAIL_MSG( wxT( "ToPlotter(): unexpected plot type" ) ); break;
        break;
    }

    SetPlotSettings( plotSettings );

    DIALOG_PLOT dlg( this );
    dlg.ShowQuasiModal(  );
}


int PCB_EDIT_FRAME::TestStandalone()
{
    if( Kiface().IsSingle() )
        return 0;

    // Update PCB requires a netlist. Therefore the schematic editor must be running
    // If this is not the case, open the schematic editor
    KIWAY_PLAYER* frame = Kiway().Player( FRAME_SCH, true );

    // If Kiway() cannot create the eeschema frame, it shows a error message, and
    // frame is null
    if( !frame )
        return -1;

    if( !frame->IsShown() )
    {
        wxFileName fn( Prj().GetProjectPath(), Prj().GetProjectName(),
                       KiCadSchematicFileExtension );

        // Maybe the file hasn't been converted to the new s-expression file format so
        // see if the legacy schematic file is still in play.
        if( !fn.FileExists() )
        {
            fn.SetExt( LegacySchematicFileExtension );

            if( !fn.FileExists() )
            {
                DisplayError( this, _( "The schematic for this board cannot be found." ) );
                return -2;
            }
        }

        frame->OpenProjectFiles( std::vector<wxString>( 1, fn.GetFullPath() ) );

        // we show the schematic editor frame, because do not show is seen as
        // a not yet opened schematic by Kicad manager, which is not the case
        frame->Show( true );

        // bring ourselves back to the front
        Raise();
    }

    return 1;            //Success!
}


bool PCB_EDIT_FRAME::FetchNetlistFromSchematic( NETLIST& aNetlist,
                                                const wxString& aAnnotateMessage )
{
    if( TestStandalone() == 0 )
    {
        DisplayErrorMessage( this, _( "Cannot update the PCB because PCB editor is opened in "
                                      "stand-alone mode. In order to create or update PCBs from "
                                      "schematics, you must launch the KiCad project manager and "
                                      "create a project." ) );
        return false;       // Not in standalone mode
    }

    if( TestStandalone() < 0 )      // Problem with Eeschema or the schematic
        return false;

    Raise();                // Show

    std::string payload( aAnnotateMessage );

    Kiway().ExpressMail( FRAME_SCH, MAIL_SCH_GET_NETLIST, payload, this );

    if( payload == aAnnotateMessage )
    {
        Raise();
        DisplayErrorMessage( this, aAnnotateMessage );
        return false;
    }

    try
    {
        auto lineReader = new STRING_LINE_READER( payload, _( "Eeschema netlist" ) );
        KICAD_NETLIST_READER netlistReader( lineReader, &aNetlist );
        netlistReader.LoadNetlist();
    }
    catch( const IO_ERROR& e )
    {
        Raise();

        // Do not translate extra_info strings.  These are for developers
        wxString extra_info = e.Problem() + wxT( " : " ) + e.What() + wxT( " at " ) + e.Where();

        DisplayErrorMessage( this, _( "Received an error while reading netlist.  Please "
                                      "report this issue to the KiCad team using the menu "
                                      "Help->Report Bug."), extra_info );
        return false;
    }

    return true;
}


void PCB_EDIT_FRAME::RunEeschema()
{
    wxString   msg;
    wxFileName schematic( Prj().GetProjectPath(), Prj().GetProjectName(),
                          KiCadSchematicFileExtension );

    if( !schematic.FileExists() )
    {
        wxFileName legacySchematic( Prj().GetProjectPath(), Prj().GetProjectName(),
                                    LegacySchematicFileExtension );

        if( legacySchematic.FileExists() )
        {
            schematic = legacySchematic;
        }
        else
        {
            msg.Printf( _( "Schematic file '%s' not found." ), schematic.GetFullPath() );
            DisplayErrorMessage( this, msg );
            return;
        }
    }

    if( Kiface().IsSingle() )
    {
        ExecuteFile( EESCHEMA_EXE, schematic.GetFullPath() );
    }
    else
    {
        KIWAY_PLAYER* frame = Kiway().Player( FRAME_SCH, false );

        // Please: note: DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB::initBuffers() calls
        // Kiway.Player( FRAME_SCH, true )
        // therefore, the schematic editor is sometimes running, but the schematic project
        // is not loaded, if the library editor was called, and the dialog field editor was used.
        // On Linux, it happens the first time the schematic editor is launched, if
        // library editor was running, and the dialog field editor was open
        // On Windows, it happens always after the library editor was called,
        // and the dialog field editor was used
        if( !frame )
        {
            try
            {
                frame = Kiway().Player( FRAME_SCH, true );
            }
            catch( const IO_ERROR& err )
            {
                DisplayErrorMessage( this, _( "Eeschema failed to load." ) + wxS( "\n" ) + err.What() );
                return;
            }
        }

        // If Kiway() cannot create the eeschema frame, it shows a error message, and
        // frame is null
        if( !frame )
            return;

        if( !frame->IsShown() ) // the frame exists, (created by the dialog field editor)
                                // but no project loaded.
        {
            frame->OpenProjectFiles( std::vector<wxString>( 1, schematic.GetFullPath() ) );
            frame->Show( true );
        }

        // On Windows, Raise() does not bring the window on screen, when iconized or not shown
        // On Linux, Raise() brings the window on screen, but this code works fine
        if( frame->IsIconized() )
        {
            frame->Iconize( false );

            // If an iconized frame was created by Pcbnew, Iconize( false ) is not enough
            // to show the frame at its normal size: Maximize should be called.
            frame->Maximize( false );
        }

        frame->Raise();
    }
}


void PCB_EDIT_FRAME::PythonSyncEnvironmentVariables()
{
    const ENV_VAR_MAP& vars = Pgm().GetLocalEnvVariables();

    // Set the environment variables for python scripts
    // note: the string will be encoded UTF8 for python env
    for( const std::pair<const wxString, ENV_VAR_ITEM>& var : vars )
        UpdatePythonEnvVar( var.first, var.second.GetValue() );

    // Because the env vars can be modified by the python scripts (rewritten in UTF8),
    // regenerate them (in Unicode) for our normal environment
    for( const std::pair<const wxString, ENV_VAR_ITEM>& var : vars )
        wxSetEnv( var.first, var.second.GetValue() );
}


void PCB_EDIT_FRAME::PythonSyncProjectName()
{
    wxString evValue;
    wxGetEnv( PROJECT_VAR_NAME, &evValue );
    UpdatePythonEnvVar( wxString( PROJECT_VAR_NAME ).ToStdString(), evValue );

    // Because PROJECT_VAR_NAME can be modified by the python scripts (rewritten in UTF8),
    // regenerate it (in Unicode) for our normal environment
    wxSetEnv( PROJECT_VAR_NAME, evValue );
}


void PCB_EDIT_FRAME::ShowFootprintPropertiesDialog( FOOTPRINT* aFootprint )
{
    if( aFootprint == nullptr )
        return;

    DIALOG_FOOTPRINT_PROPERTIES::FP_PROPS_RETVALUE retvalue;

    /*
     * Make sure dlg is destroyed before GetCanvas->Refresh is called
     * later or the refresh will try to modify its properties since
     * they share a GL context.
     */
    {
        DIALOG_FOOTPRINT_PROPERTIES dlg( this, aFootprint );

        dlg.ShowModal();
        retvalue = dlg.GetReturnValue();
    }

    /*
     * retvalue =
     *   FP_PROPS_UPDATE_FP to show Update Footprints dialog
     *   FP_PROPS_CHANGE_FP to show Change Footprints dialog
     *   FP_PROPS_OK for normal edit
     *   FP_PROPS_CANCEL if aborted
     *   FP_PROPS_EDIT_BOARD_FP to load board footprint into Footprint Editor
     *   FP_PROPS_EDIT_LIBRARY_FP to load library footprint into Footprint Editor
     */

    if( retvalue == DIALOG_FOOTPRINT_PROPERTIES::FP_PROPS_OK )
    {
        // If something edited, push a refresh request
        GetCanvas()->Refresh();
    }
    else if( retvalue == DIALOG_FOOTPRINT_PROPERTIES::FP_PROPS_EDIT_BOARD_FP )
    {
        auto editor = (FOOTPRINT_EDIT_FRAME*) Kiway().Player( FRAME_FOOTPRINT_EDITOR, true );

        editor->LoadFootprintFromBoard( aFootprint );

        editor->Show( true );
        editor->Raise();        // Iconize( false );
    }
    else if( retvalue == DIALOG_FOOTPRINT_PROPERTIES::FP_PROPS_EDIT_LIBRARY_FP )
    {
        auto editor = (FOOTPRINT_EDIT_FRAME*) Kiway().Player( FRAME_FOOTPRINT_EDITOR, true );

        editor->LoadFootprintFromLibrary( aFootprint->GetFPID() );

        editor->Show( true );
        editor->Raise();        // Iconize( false );
    }
    else if( retvalue == DIALOG_FOOTPRINT_PROPERTIES::FP_PROPS_UPDATE_FP )
    {
        ShowExchangeFootprintsDialog( aFootprint, true, true );
    }
    else if( retvalue == DIALOG_FOOTPRINT_PROPERTIES::FP_PROPS_CHANGE_FP )
    {
        ShowExchangeFootprintsDialog( aFootprint, false, true );
    }
}


int PCB_EDIT_FRAME::ShowExchangeFootprintsDialog( FOOTPRINT* aFootprint, bool aUpdateMode,
                                                  bool aSelectedMode )
{
    DIALOG_EXCHANGE_FOOTPRINTS dialog( this, aFootprint, aUpdateMode, aSelectedMode );

    return dialog.ShowQuasiModal();
}


namespace {


void processTextItem( const PCB_TEXT& aSrc, PCB_TEXT& aDest,
                      bool resetText, bool resetTextLayers, bool resetTextEffects,
                      bool* aUpdated )
{
    if( resetText )
        *aUpdated |= aSrc.GetText() != aDest.GetText();
    else
        aDest.SetText( aSrc.GetText() );

    if( resetTextLayers )
    {
        *aUpdated |= aSrc.GetLayer() != aDest.GetLayer();
        *aUpdated |= aSrc.IsVisible() != aDest.IsVisible();
    }
    else
    {
        aDest.SetLayer( aSrc.GetLayer() );
        aDest.SetVisible( aSrc.IsVisible() );
    }

    if( resetTextEffects )
    {
        *aUpdated |= aSrc.GetHorizJustify() != aDest.GetHorizJustify();
        *aUpdated |= aSrc.GetVertJustify() != aDest.GetVertJustify();
        *aUpdated |= aSrc.GetTextSize() != aDest.GetTextSize();
        *aUpdated |= aSrc.GetTextThickness() != aDest.GetTextThickness();
        *aUpdated |= aSrc.GetTextAngle() != aDest.GetTextAngle();
        *aUpdated |= aSrc.GetFPRelativePosition() != aDest.GetFPRelativePosition();
    }
    else
    {
        // Careful: the visible bit and position are also set by SetAttributes()
        bool visible = aDest.IsVisible();
        aDest.SetAttributes( aSrc );
        aDest.SetVisible( visible );
        aDest.SetFPRelativePosition( aSrc.GetFPRelativePosition() );
    }

    aDest.SetLocked( aSrc.IsLocked() );
}


PCB_TEXT* getMatchingTextItem( PCB_TEXT* aRefItem, FOOTPRINT* aFootprint )
{
    std::vector<PCB_TEXT*> candidates;

    for( BOARD_ITEM* item : aFootprint->GraphicalItems() )
    {
        PCB_TEXT* candidate = dyn_cast<PCB_TEXT*>( item );

        if( candidate && candidate->GetText() == aRefItem->GetText() )
            candidates.push_back( candidate );
    }

    if( candidates.size() == 0 )
        return nullptr;

    if( candidates.size() == 1 )
        return candidates[0];

    // Try refining the match by layer
    std::vector<PCB_TEXT*> candidatesOnSameLayer;

    for( PCB_TEXT* candidate : candidates )
    {
        if( candidate->GetLayer() == aRefItem->GetLayer() )
            candidatesOnSameLayer.push_back( candidate );
    }

    if( candidatesOnSameLayer.size() == 1 )
        return candidatesOnSameLayer[0];

    // Last ditch effort: refine by position
    std::vector<PCB_TEXT*> candidatesAtSamePos;

    for( PCB_TEXT* candidate : candidatesOnSameLayer.size() ? candidatesOnSameLayer : candidates )
    {
        if( candidate->GetFPRelativePosition() == aRefItem->GetFPRelativePosition() )
            candidatesAtSamePos.push_back( candidate );
    }

    if( candidatesAtSamePos.size() > 0 )
        return candidatesAtSamePos[0];
    else if( candidatesOnSameLayer.size() > 0 )
        return candidatesOnSameLayer[0];
    else
        return candidates[0];
}


}


void PCB_EDIT_FRAME::ExchangeFootprint( FOOTPRINT* aExisting, FOOTPRINT* aNew,
                                        BOARD_COMMIT& aCommit, bool deleteExtraTexts,
                                        bool resetTextLayers, bool resetTextEffects,
                                        bool resetFabricationAttrs, bool reset3DModels,
                                        bool* aUpdated )
{
    PCB_GROUP* parentGroup = aExisting->GetParentGroup();
    bool       dummyBool   = false;

    if( !aUpdated )
        aUpdated = &dummyBool;

    if( parentGroup )
    {
        parentGroup->RemoveItem( aExisting );
        parentGroup->AddItem( aNew );
    }

    aNew->SetParent( GetBoard() );

    PlaceFootprint( aNew, false );

    // PlaceFootprint will move the footprint to the cursor position, which we don't want.  Copy
    // the original position across.
    aNew->SetPosition( aExisting->GetPosition() );

    if( aNew->GetLayer() != aExisting->GetLayer() )
        aNew->Flip( aNew->GetPosition(), GetPcbNewSettings()->m_FlipLeftRight );

    if( aNew->GetOrientation() != aExisting->GetOrientation() )
        aNew->SetOrientation( aExisting->GetOrientation() );

    aNew->SetLocked( aExisting->IsLocked() );

    // Now transfer the net info from "old" pads to the new footprint
    for( PAD* pad : aNew->Pads() )
    {
        PAD* pad_model = nullptr;

        // Pads with no copper are never connected to a net
        if( !pad->IsOnCopperLayer() )
        {
            pad->SetNetCode( NETINFO_LIST::UNCONNECTED );
            continue;
        }

        // Pads with no numbers are never connected to a net
        if( pad->GetNumber().IsEmpty() )
        {
            pad->SetNetCode( NETINFO_LIST::UNCONNECTED );
            continue;
        }

        // Search for a similar pad on a copper layer, to reuse net info
        PAD* last_pad = nullptr;

        while( true )
        {
            pad_model = aExisting->FindPadByNumber( pad->GetNumber(), last_pad );

            if( !pad_model )
                break;

            if( pad_model->IsOnCopperLayer() )     // a candidate is found
                break;

            last_pad = pad_model;
        }

        if( pad_model )
        {
            pad->SetLocalRatsnestVisible( pad_model->GetLocalRatsnestVisible() );
            pad->SetPinFunction( pad_model->GetPinFunction() );
            pad->SetPinType( pad_model->GetPinType() );
        }

        pad->SetNetCode( pad_model ? pad_model->GetNetCode() : NETINFO_LIST::UNCONNECTED );
    }

    // Copy reference
    processTextItem( aExisting->Reference(), aNew->Reference(),
                     // never reset reference text
                     false,
                     resetTextLayers, resetTextEffects, aUpdated );

    // Copy value
    processTextItem( aExisting->Value(), aNew->Value(),
                     // reset value text only when it is a proxy for the footprint ID
                     // (cf replacing value "MountingHole-2.5mm" with "MountingHole-4.0mm")
                     aExisting->GetValue() == aExisting->GetFPID().GetLibItemName(),
                     resetTextLayers, resetTextEffects, aUpdated );

    // Copy fields in accordance with the reset* flags

    for( BOARD_ITEM* item : aExisting->GraphicalItems() )
    {
        PCB_TEXT* srcItem = dyn_cast<PCB_TEXT*>( item );

        if( srcItem )
        {
            PCB_TEXT* destItem = getMatchingTextItem( srcItem, aNew );

            if( destItem )
            {
                processTextItem( *srcItem, *destItem, false, resetTextLayers, resetTextEffects,
                                 aUpdated );
            }
            else if( !deleteExtraTexts )
            {
                aNew->Add( new PCB_TEXT( *srcItem ) );
            }
        }
    }

    if( resetFabricationAttrs )
    {
        // We've replaced the existing footprint with the library one, so the fabrication attrs
        // are already reset.
        //
        // We only have to do anything if resetFabricationAttrs is *not* set....
    }
    else
    {
        // Careful; allow-soldermask-bridges is in the m_attributes field but is not presented
        // as a fabrication attribute in the GUI....
        int libraryFlagsToKeep = aNew->GetAttributes() & FP_ALLOW_SOLDERMASK_BRIDGES;
        int existingFlagsToKeep = aExisting->GetAttributes() & ~FP_ALLOW_SOLDERMASK_BRIDGES;
        aNew->SetAttributes( existingFlagsToKeep | libraryFlagsToKeep );
    }

    if( reset3DModels )
    {
        // We've replaced the existing footprint with the library one, so the 3D models are
        // already reset.
        //
        // We only have to do anything if reset3DModels is *not* set....
    }
    else
    {
        aNew->Models() = aExisting->Models();  // Linked list of 3D models.
    }

    // Updating other parameters
    const_cast<KIID&>( aNew->m_Uuid ) = aExisting->m_Uuid;
    aNew->SetProperties( aExisting->GetProperties() );
    aNew->SetPath( aExisting->GetPath() );

    aCommit.Remove( aExisting );
    aCommit.Add( aNew );

    aNew->ClearFlags();
}


void PCB_EDIT_FRAME::CommonSettingsChanged( bool aEnvVarsChanged, bool aTextVarsChanged )
{
    PCB_BASE_EDIT_FRAME::CommonSettingsChanged( aEnvVarsChanged, aTextVarsChanged );

    GetAppearancePanel()->OnColorThemeChanged();

    KIGFX::PCB_VIEW*            view = GetCanvas()->GetView();
    KIGFX::PCB_PAINTER*         painter = static_cast<KIGFX::PCB_PAINTER*>( view->GetPainter() );
    KIGFX::PCB_RENDER_SETTINGS* renderSettings = painter->GetSettings();

    renderSettings->LoadDisplayOptions( GetDisplayOptions() );

    SetElementVisibility( LAYER_RATSNEST, GetPcbNewSettings()->m_Display.m_ShowGlobalRatsnest );

    GetGalDisplayOptions().ReadWindowSettings( GetPcbNewSettings()->m_Window );

    // Netclass definitions could have changed, either by us or by Eeschema, so we need to
    // recompile the implicit rules
    DRC_TOOL*   drcTool = m_toolManager->GetTool<DRC_TOOL>();
    WX_INFOBAR* infobar = GetInfoBar();

    try
    {
        drcTool->GetDRCEngine()->InitEngine( GetDesignRulesPath() );

        if( infobar->GetMessageType() == WX_INFOBAR::MESSAGE_TYPE::DRC_RULES_ERROR )
            infobar->Dismiss();
    }
    catch( PARSE_ERROR& )
    {
        wxHyperlinkCtrl* button = new wxHyperlinkCtrl( infobar, wxID_ANY, _( "Edit design rules" ),
                                                       wxEmptyString );

        button->Bind( wxEVT_COMMAND_HYPERLINK, std::function<void( wxHyperlinkEvent& aEvent )>(
                [&]( wxHyperlinkEvent& aEvent )
                {
                    ShowBoardSetupDialog( _( "Custom Rules" ) );
                } ) );

        infobar->RemoveAllButtons();
        infobar->AddButton( button );
        infobar->AddCloseButton();
        infobar->ShowMessage( _( "Could not compile custom design rules." ), wxICON_ERROR,
                              WX_INFOBAR::MESSAGE_TYPE::DRC_RULES_ERROR );
    }

    GetCanvas()->GetView()->MarkTargetDirty( KIGFX::TARGET_NONCACHED );
    GetCanvas()->ForceRefresh();

    // Update the environment variables in the Python interpreter
    if( aEnvVarsChanged )
        PythonSyncEnvironmentVariables();

    Layout();
    SendSizeEvent();
}


void PCB_EDIT_FRAME::ThemeChanged()
{
    PCB_BASE_EDIT_FRAME::ThemeChanged();
}


void PCB_EDIT_FRAME::ProjectChanged()
{
    PythonSyncProjectName();
}


bool ExportBoardToHyperlynx( BOARD* aBoard, const wxFileName& aPath );


void PCB_EDIT_FRAME::OnExportHyperlynx( wxCommandEvent& event )
{
    wxString    wildcard =  wxT( "*.hyp" );
    wxFileName  fn = GetBoard()->GetFileName();

    fn.SetExt( wxT("hyp") );

    wxFileDialog dlg( this, _( "Export Hyperlynx Layout" ), fn.GetPath(), fn.GetFullName(),
                      wildcard, wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    if( dlg.ShowModal() != wxID_OK )
        return;

    fn = dlg.GetPath();

    // always enforce filename extension, user may not have entered it.
    fn.SetExt( wxT( "hyp" ) );

    ExportBoardToHyperlynx( GetBoard(), fn );
}


wxString PCB_EDIT_FRAME::GetCurrentFileName() const
{
    return GetBoard()->GetFileName();
}


bool PCB_EDIT_FRAME::LayerManagerShown()
{
    return m_auimgr.GetPane( wxS( "LayersManager" ) ).IsShown();
}


bool PCB_EDIT_FRAME::PropertiesShown()
{
    return m_auimgr.GetPane( wxS( "PropertiesManager" ) ).IsShown();
}


void PCB_EDIT_FRAME::onSize( wxSizeEvent& aEvent )
{
    if( IsShown() )
    {
        // We only need this until the frame is done resizing and the final client size is
        // established.
        Unbind( wxEVT_SIZE, &PCB_EDIT_FRAME::onSize, this );
        GetToolManager()->RunAction( ACTIONS::zoomFitScreen, true );
    }

    // Skip() is called in the base class.
    EDA_DRAW_FRAME::OnSize( aEvent );
}


DIALOG_BOOK_REPORTER* PCB_EDIT_FRAME::GetInspectDrcErrorDialog()
{
    if( !m_inspectDrcErrorDlg )
        m_inspectDrcErrorDlg = new DIALOG_BOOK_REPORTER( this, INSPECT_DRC_ERROR_DIALOG_NAME,
                                                         _( "Violation Report" ) );

    return m_inspectDrcErrorDlg;
}


DIALOG_BOOK_REPORTER* PCB_EDIT_FRAME::GetInspectClearanceDialog()
{
    if( !m_inspectClearanceDlg )
        m_inspectClearanceDlg = new DIALOG_BOOK_REPORTER( this, INSPECT_CLEARANCE_DIALOG_NAME,
                                                          _( "Clearance Report" ) );

    return m_inspectClearanceDlg;
}


DIALOG_BOOK_REPORTER* PCB_EDIT_FRAME::GetInspectConstraintsDialog()
{
    if( !m_inspectConstraintsDlg )
        m_inspectConstraintsDlg = new DIALOG_BOOK_REPORTER( this, INSPECT_CONSTRAINTS_DIALOG_NAME,
                                                            _( "Constraints Report" ) );

    return m_inspectConstraintsDlg;
}


DIALOG_BOOK_REPORTER* PCB_EDIT_FRAME::GetFootprintDiffDialog()
{
    if( !m_footprintDiffDlg )
        m_footprintDiffDlg = new DIALOG_BOOK_REPORTER( this, FOOTPRINT_DIFF_DIALOG_NAME,
                                                       _( "Diff Footprint with Library" ) );

    return m_footprintDiffDlg;
}


void PCB_EDIT_FRAME::onCloseModelessBookReporterDialogs( wxCommandEvent& aEvent )
{
    if( m_inspectDrcErrorDlg && aEvent.GetString() == INSPECT_DRC_ERROR_DIALOG_NAME )
    {
        m_inspectDrcErrorDlg->Destroy();
        m_inspectDrcErrorDlg = nullptr;
    }
    else if( m_inspectClearanceDlg && aEvent.GetString() == INSPECT_CLEARANCE_DIALOG_NAME )
    {
        m_inspectClearanceDlg->Destroy();
        m_inspectClearanceDlg = nullptr;
    }
    else if( m_inspectConstraintsDlg && aEvent.GetString() == INSPECT_CONSTRAINTS_DIALOG_NAME )
    {
        m_inspectConstraintsDlg->Destroy();
        m_inspectConstraintsDlg = nullptr;
    }
    else if( m_footprintDiffDlg && aEvent.GetString() == INSPECT_CONSTRAINTS_DIALOG_NAME )
    {
        m_footprintDiffDlg->Destroy();
        m_footprintDiffDlg = nullptr;
    }
}


DIALOG_NET_INSPECTOR* PCB_EDIT_FRAME::GetNetInspectorDialog()
{
    if( !m_netInspectorDlg )
    {
        m_netInspectorDlg = new DIALOG_NET_INSPECTOR( this );
        AddBoardChangeListener( m_netInspectorDlg );
    }

    return m_netInspectorDlg;
}


void PCB_EDIT_FRAME::onCloseNetInspectorDialog( wxCommandEvent& aEvent )
{
    if( m_netInspectorDlg )
    {
        RemoveBoardChangeListener( m_netInspectorDlg );
        m_netInspectorDlg->Destroy();
        m_netInspectorDlg = nullptr;
    }
}


void PCB_EDIT_FRAME::onUnitsChanged( wxCommandEvent& aEvent )
{
    wxCommandEvent evt( EDA_EVT_UNITS_CHANGED );

    if( m_netInspectorDlg )
        m_netInspectorDlg->HandleWindowEvent( evt );
}
