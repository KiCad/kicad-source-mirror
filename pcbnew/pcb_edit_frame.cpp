/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2013 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2013 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
#include <connectivity/connectivity_data.h>
#include <kiface_base.h>
#include <kiway.h>
#include <board_design_settings.h>
#include <settings/color_settings.h>
#include <pgm_base.h>
#include <pcb_edit_frame.h>
#include <3d_viewer/eda_3d_viewer_frame.h>
#include <api/api_plugin_manager.h>
#include <geometry/geometry_utils.h>
#include <bitmaps.h>
#include <confirm.h>
#include <lset.h>
#include <trace_helpers.h>
#include <algorithm>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <pcbnew_id.h>
#include <pcbnew_settings.h>
#include <pcb_layer_box_selector.h>
#include <footprint_edit_frame.h>
#include <dialog_find.h>
#include <dialog_footprint_properties.h>
#include <dialogs/dialog_exchange_footprints.h>
#include <dialog_board_setup.h>
#include <dialogs/dialog_dimension_properties.h>
#include <dialogs/dialog_table_properties.h>
#include <gal/graphics_abstraction_layer.h>
#include <pcb_target.h>
#include <pcb_point.h>
#include <layer_pairs.h>
#include <drawing_sheet/ds_proxy_view_item.h>
#include <wildcards_and_files_ext.h>
#include <wx/filename.h>
#include <functional>
#include <pcb_barcode.h>
#include <pcb_painter.h>
#include <project/project_file.h>
#include <project/project_local_settings.h>
#include <python_scripting.h>
#include <settings/common_settings.h>
#include <settings/settings_manager.h>
#include <local_history.h>
#include <tool/tool_manager.h>
#include <tool/tool_dispatcher.h>
#include <tool/action_toolbar.h>
#include <tool/common_control.h>
#include <tool/common_tools.h>
#include <tool/embed_tool.h>
#include <tool/properties_tool.h>
#include <tool/selection.h>
#include <tool/zoom_tool.h>
#include <tools/array_tool.h>
#include <tools/pcb_grid_helper.h>
#include <tools/pcb_selection_tool.h>
#include <tools/pcb_picker_tool.h>
#include <tools/pcb_point_editor.h>
#include <tools/edit_tool.h>
#include <tools/pcb_edit_table_tool.h>
#include <tools/pcb_group_tool.h>
#include <tools/generator_tool.h>
#include <tools/drc_tool.h>
#include <tools/drc_rule_editor_tool.h>
#include <tools/global_edit_tool.h>
#include <tools/convert_tool.h>
#include <tools/drawing_tool.h>
#include <tools/pcb_control.h>
#include <tools/pcb_design_block_control.h>
#include <tools/board_editor_control.h>
#include <tools/board_inspection_tool.h>
#include <tools/pcb_editor_conditions.h>
#include <tools/pcb_viewer_tools.h>
#include <tools/board_reannotate_tool.h>
#include <tools/align_distribute_tool.h>
#include <tools/pad_tool.h>
#include <microwave/microwave_tool.h>
#include <properties/property.h>
#include <properties/property_mgr.h>
#include <tools/position_relative_tool.h>
#include <tools/zone_filler_tool.h>
#include <tools/multichannel_tool.h>
#include <router/router_tool.h>
#include <autorouter/autoplace_tool.h>
#include <python/scripting/pcb_scripting_tool.h>
#include <netlist_reader/netlist_reader.h>
#include <wx/socket.h>
#include <wx/wupdlock.h>
#include <dialog_drc.h>     // for DIALOG_DRC_WINDOW_NAME definition
#include <ratsnest/ratsnest_view_item.h>
#include <widgets/appearance_controls.h>
#include <widgets/pcb_design_block_pane.h>
#include <widgets/pcb_search_pane.h>
#include <widgets/wx_infobar.h>
#include <widgets/panel_selection_filter.h>
#include <widgets/pcb_properties_panel.h>
#include <widgets/pcb_net_inspector_panel.h>
#include <widgets/wx_aui_utils.h>
#include <kiplatform/app.h>
#include <kiplatform/ui.h>
#include <core/profile.h>
#include <math/box2_minmax.h>
#include <view/wx_view_controls.h>
#include <footprint_viewer_frame.h>
#include <footprint_chooser_frame.h>
#include <toolbars_pcb_editor.h>
#include <wx/log.h>
#include <drc/rule_editor/dialog_drc_rule_editor.h>

#ifdef KICAD_IPC_API
#include <api/api_server.h>
#include <api/api_handler_pcb.h>
#include <api/api_handler_common.h>
#include <api/api_utils.h>
#endif

#include <action_plugin.h>
#include <pcbnew_scripting_helpers.h>
#include <richio.h>

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

    // Menu Files:
    EVT_MENU_RANGE( ID_FILE1, ID_FILEMAX, PCB_EDIT_FRAME::OnFileHistory )
    EVT_MENU( ID_FILE_LIST_CLEAR, PCB_EDIT_FRAME::OnClearFileHistory )

    EVT_MENU( wxID_EXIT, PCB_EDIT_FRAME::OnQuit )
    EVT_MENU( wxID_CLOSE, PCB_EDIT_FRAME::OnQuit )

    // Horizontal toolbar
    EVT_CHOICE( ID_AUX_TOOLBAR_PCB_TRACK_WIDTH, PCB_EDIT_FRAME::Tracks_and_Vias_Size_Event )
    EVT_CHOICE( ID_AUX_TOOLBAR_PCB_VIA_SIZE, PCB_EDIT_FRAME::Tracks_and_Vias_Size_Event )
    EVT_CHOICE( ID_AUX_TOOLBAR_PCB_VARIANT_SELECT, PCB_EDIT_FRAME::onVariantSelected )

    // Tracks and vias sizes general options
    EVT_MENU_RANGE( ID_POPUP_PCB_SELECT_WIDTH_START_RANGE, ID_POPUP_PCB_SELECT_WIDTH_END_RANGE,
                    PCB_EDIT_FRAME::Tracks_and_Vias_Size_Event )

    // User interface update event handlers.
    EVT_UPDATE_UI( ID_AUX_TOOLBAR_PCB_TRACK_WIDTH, PCB_EDIT_FRAME::OnUpdateSelectTrackWidth )
    EVT_UPDATE_UI( ID_AUX_TOOLBAR_PCB_VIA_SIZE, PCB_EDIT_FRAME::OnUpdateSelectViaSize )
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
    m_boardSetupDlg( nullptr ),
    m_designBlocksPane( nullptr ),
    m_importProperties( nullptr ),
    m_eventCounterTimer( nullptr )
{
    m_maximizeByDefault = true;
    m_showBorderAndTitleBlock = true;   // true to display sheet references
    m_SelTrackWidthBox = nullptr;
    m_SelViaSizeBox = nullptr;
    m_currentVariantCtrl = nullptr;
    m_show_layer_manager_tools = true;
    m_supportsAutoSave = true;
    m_probingSchToPcb = false;
    m_show_search = false;
    m_show_net_inspector = false;
    // Ensure timer has an owner before binding so it generates events.
    m_crossProbeFlashTimer.SetOwner( this );
    Bind( wxEVT_TIMER, &PCB_EDIT_FRAME::OnCrossProbeFlashTimer, this, m_crossProbeFlashTimer.GetId() );

    // We don't know what state board was in when it was last saved, so we have to
    // assume dirty
    m_ZoneFillsDirty = true;

    m_aboutTitle = _HKI( "KiCad PCB Editor" );

    // Must be created before the menus are created.
    if( ADVANCED_CFG::GetCfg().m_ShowPcbnewExportNetlist )
    {
        m_exportNetlistAction = new TOOL_ACTION( "pcbnew.EditorControl.exportNetlist",
                                                 AS_GLOBAL, 0, "", _( "Netlist..." ),
                                                 _( "Export netlist used to update schematics" ) );
    }

    // Create GAL canvas
    auto canvas = new PCB_DRAW_PANEL_GAL( this, -1, wxPoint( 0, 0 ), m_frameSize,
                                          GetGalDisplayOptions(),
                                          EDA_DRAW_PANEL_GAL::GAL_FALLBACK );

    SetCanvas( canvas );
    SetBoard( new BOARD() );

    wxIcon icon;
    wxIconBundle icon_bundle;

    icon.CopyFromBitmap( KiBitmap( BITMAPS::icon_pcbnew, 48 ) );
    icon_bundle.AddIcon( icon );
    icon.CopyFromBitmap( KiBitmap( BITMAPS::icon_pcbnew, 128 ) );
    icon_bundle.AddIcon( icon );
    icon.CopyFromBitmap( KiBitmap( BITMAPS::icon_pcbnew, 256 ) );
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

    m_toolbarSettings = GetToolbarSettings<PCB_EDIT_TOOLBAR_SETTINGS>( "pcbnew-toolbars" );
    configureToolbars();
    RecreateToolbars();
    PrepareLayerIndicator( true );

    ReCreateMenuBar();

#ifdef KICAD_IPC_API
    wxTheApp->Bind( EDA_EVT_PLUGIN_AVAILABILITY_CHANGED,
                    &PCB_EDIT_FRAME::onPluginAvailabilityChanged, this );
#endif

    // Fetch a COPY of the config as a lot of these initializations are going to overwrite our
    // data.
    PCBNEW_SETTINGS::AUI_PANELS aui_cfg = GetPcbNewSettings()->m_AuiPanels;

    m_propertiesPanel = new PCB_PROPERTIES_PANEL( this, this );
    m_propertiesPanel->SetSplitterProportion( aui_cfg.properties_splitter );

    m_selectionFilterPanel = new PANEL_SELECTION_FILTER( this );

    m_appearancePanel = new APPEARANCE_CONTROLS( this, GetCanvas() );
    m_searchPane = new PCB_SEARCH_PANE( this );
    m_netInspectorPanel = new PCB_NET_INSPECTOR_PANEL( this, this );
    m_designBlocksPane = new PCB_DESIGN_BLOCK_PANE( this, nullptr, m_designBlockHistoryList );

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
    m_auimgr.AddPane( m_tbTopMain, EDA_PANE().HToolbar().Name( wxS( "TopMainToolbar" ) )
                      .Top().Layer( 6 ) );
    m_auimgr.AddPane( m_tbTopAux, EDA_PANE().HToolbar().Name( wxS( "TopAuxToolbar" ) )
                      .Top().Layer( 5 ) );
    m_auimgr.AddPane( m_messagePanel, EDA_PANE().Messages().Name( wxS( "MsgPanel" ) )
                      .Bottom().Layer( 6 ) );

    // Columns; layers 1 - 3
    m_auimgr.AddPane( m_tbLeft, EDA_PANE().VToolbar().Name( wxS( "LeftToolbar" ) )
                      .Left().Layer( 3 ) );

    m_auimgr.AddPane( m_tbRight, EDA_PANE().VToolbar().Name( wxS( "RightToolbar" ) )
                      .Right().Layer( 3 ) );

    m_auimgr.AddPane( m_appearancePanel, EDA_PANE().Name( wxS( "LayersManager" ) )
                      .Right().Layer( 4 )
                      .Caption( _( "Appearance" ) ).PaneBorder( false )
                      // Don't use -1 for don't-change-height on a growable panel; it has side-effects.
                      .MinSize( m_appearancePanel->GetMinSize().x, FromDIP( 60 ) )
#ifdef __WXMAC__
                      // Best size for this pane is calculated larger than necessary on wxMac
                      .BestSize( m_appearancePanel->GetMinSize().x, -1 )
#else
                      .BestSize( m_appearancePanel->GetBestSize().x, -1 )
#endif
                      .FloatingSize( m_appearancePanel->GetBestSize() )
                      .CloseButton( false ) );

    m_auimgr.AddPane( m_selectionFilterPanel, EDA_PANE().Name( wxS( "SelectionFilter" ) )
                      .Right().Layer( 4 ).Position( 2 )
                      .Caption( _( "Selection Filter" ) ).PaneBorder( false )
                      // Fixed-size pane; -1 for MinSize height is required
                      .MinSize( m_selectionFilterPanel->GetMinSize().x, -1 )
                      .BestSize( m_selectionFilterPanel->GetBestSize().x, -1 )
                      .FloatingSize( m_selectionFilterPanel->GetBestSize() )
                      .CloseButton( false ) );

    m_auimgr.AddPane( m_designBlocksPane, EDA_PANE().Name( DesignBlocksPaneName() )
                      .Right().Layer( 5 )
                      .Caption( _( "Design Blocks" ) )
                      .CaptionVisible( true )
                      .PaneBorder( true )
                      .TopDockable( false )
                      .BottomDockable( false )
                      .CloseButton( true )
                      .MinSize( FromDIP( wxSize( 240, 60 ) ) )
                      .BestSize( FromDIP( wxSize( 300, 200 ) ) )
                      .FloatingSize( FromDIP( wxSize( 800, 600 ) ) )
                      .FloatingPosition( FromDIP( wxPoint( 50, 200 ) ) )
                      .Show( true ) );

    m_auimgr.AddPane( m_propertiesPanel, EDA_PANE().Name( PropertiesPaneName() )
                      .Left().Layer( 5 )
                      .Caption( _( "Properties" ) ).PaneBorder( false )
                      .MinSize( FromDIP( wxSize( 240, 60 ) ) )
                      .BestSize( FromDIP( wxSize( 300, 200 ) ) )
                      .FloatingSize( wxSize( 300, 200 ) )
                      .CloseButton( true ) );

    // Center
    m_auimgr.AddPane( GetCanvas(), EDA_PANE().Canvas().Name( wxS( "DrawFrame" ) )
                      .Center() );

    m_auimgr.AddPane( m_netInspectorPanel, EDA_PANE().Name( NetInspectorPanelName() )
                      .Bottom()
                      .Caption( _( "Net Inspector" ) )
                      .PaneBorder( false )
                      .MinSize( FromDIP( wxSize( 240, 60 ) ) )
                      .BestSize( FromDIP( wxSize( 300, 200 ) ) )
                      .FloatingSize( wxSize( 300, 200 ) )
                      .CloseButton( true ) );

    m_auimgr.AddPane( m_searchPane, EDA_PANE().Name( SearchPaneName() )
                      .Bottom()
                      .Caption( _( "Search" ) ).PaneBorder( false )
                      .MinSize( FromDIP( wxSize ( 180, 60 ) ) )
                      .BestSize( FromDIP( wxSize ( 180, 100 ) ) )
                      .FloatingSize( FromDIP( wxSize( 480, 200 ) ) )
                      .DestroyOnClose( false )
                      .CloseButton( true ) );

    RestoreAuiLayout();

    m_auimgr.GetPane( "LayersManager" ).Show( m_show_layer_manager_tools );
    m_auimgr.GetPane( "SelectionFilter" ).Show( m_show_layer_manager_tools );
    m_auimgr.GetPane( PropertiesPaneName() ).Show( GetPcbNewSettings()->m_AuiPanels.show_properties );
    m_auimgr.GetPane( NetInspectorPanelName() ).Show( m_show_net_inspector );
    m_auimgr.GetPane( SearchPaneName() ).Show( m_show_search );
    m_auimgr.GetPane( DesignBlocksPaneName() ).Show( GetPcbNewSettings()->m_AuiPanels.design_blocks_show );

    // The selection filter doesn't need to grow in the vertical direction when docked
    m_auimgr.GetPane( "SelectionFilter" ).dock_proportion = 0;
    FinishAUIInitialization();

    if( aui_cfg.right_panel_width > 0 )
    {
        wxAuiPaneInfo& layersManager = m_auimgr.GetPane( wxS( "LayersManager" ) );
        SetAuiPaneSize( m_auimgr, layersManager, aui_cfg.right_panel_width, -1 );

        wxAuiPaneInfo& designBlocksPane = m_auimgr.GetPane( DesignBlocksPaneName() );
        SetAuiPaneSize( m_auimgr, designBlocksPane, aui_cfg.design_blocks_panel_docked_width, -1 );
    }

    if( aui_cfg.properties_panel_width > 0 && m_propertiesPanel )
    {
        wxAuiPaneInfo& propertiesPanel = m_auimgr.GetPane( PropertiesPaneName() );
        SetAuiPaneSize( m_auimgr, propertiesPanel, aui_cfg.properties_panel_width, -1 );
    }

    if( aui_cfg.search_panel_height > 0
        && ( aui_cfg.search_panel_dock_direction == wxAUI_DOCK_TOP
            || aui_cfg.search_panel_dock_direction == wxAUI_DOCK_BOTTOM ) )
    {
        wxAuiPaneInfo& searchPane = m_auimgr.GetPane( SearchPaneName() );
        searchPane.Direction( aui_cfg.search_panel_dock_direction );
        SetAuiPaneSize( m_auimgr, searchPane, -1, aui_cfg.search_panel_height );
    }
    else if( aui_cfg.search_panel_width > 0
            && ( aui_cfg.search_panel_dock_direction == wxAUI_DOCK_LEFT
                || aui_cfg.search_panel_dock_direction == wxAUI_DOCK_RIGHT ) )
    {
        wxAuiPaneInfo& searchPane = m_auimgr.GetPane( SearchPaneName() );
        searchPane.Direction( aui_cfg.search_panel_dock_direction );
        SetAuiPaneSize( m_auimgr, searchPane, aui_cfg.search_panel_width, -1 );
    }

    m_appearancePanel->SetTabIndex( aui_cfg.appearance_panel_tab );

    {
        m_layerPairSettings = std::make_unique<LAYER_PAIR_SETTINGS>();

        m_layerPairSettings->Bind( PCB_LAYER_PAIR_PRESETS_CHANGED, [&]( wxCommandEvent& aEvt )
        {
            // Update the project file list
            std::span<const LAYER_PAIR_INFO> newPairInfos = m_layerPairSettings->GetLayerPairs();
            Prj().GetProjectFile().m_LayerPairInfos =
                    std::vector<LAYER_PAIR_INFO>{ newPairInfos.begin(), newPairInfos.end() };
        });

        m_layerPairSettings->Bind( PCB_CURRENT_LAYER_PAIR_CHANGED, [&]( wxCommandEvent& aEvt )
        {
            const LAYER_PAIR& layerPair = m_layerPairSettings->GetCurrentLayerPair();
            PCB_SCREEN& screen = *GetScreen();

            screen.m_Route_Layer_TOP = layerPair.GetLayerA();
            screen.m_Route_Layer_BOTTOM = layerPair.GetLayerB();

            // Update the toolbar icon
            PrepareLayerIndicator();
        });
    }

    GetToolManager()->PostAction( ACTIONS::zoomFitScreen );

    // This is used temporarily to fix a client size issue on GTK that causes zoom to fit
    // to calculate the wrong zoom size.  See PCB_EDIT_FRAME::onSize().
    Bind( wxEVT_SIZE, &PCB_EDIT_FRAME::onSize, this );

    Bind( wxEVT_IDLE,
          [this]( wxIdleEvent& aEvent )
          {
              BOX2D viewport = GetCanvas()->GetView()->GetViewport();

              if( viewport != m_lastNetnamesViewport )
              {
                  redrawNetnames();
                  m_lastNetnamesViewport = viewport;
              }

              // Do not forget to pass the Idle event to other clients:
              aEvent.Skip();
          } );

    resolveCanvasType();

    setupUnits( config() );

    // Ensure the DRC engine is initialized so that constraints can be resolved even before a
    // board is loaded or saved
    try
    {
        m_toolManager->GetTool<DRC_TOOL>()->GetDRCEngine()->InitEngine( wxFileName() );
    }
    catch( PARSE_ERROR& )
    {
    }

    // Ensure the Python interpreter is up to date with its environment variables
    PythonSyncEnvironmentVariables();
    PythonSyncProjectName();

    // Sync action plugins in case they changed since the last time the frame opened
    GetToolManager()->RunAction( ACTIONS::pluginsReload );

#ifdef KICAD_IPC_API
    m_apiHandler = std::make_unique<API_HANDLER_PCB>( this );
    Pgm().GetApiServer().RegisterHandler( m_apiHandler.get() );

    if( Kiface().IsSingle() )
    {
        m_apiHandlerCommon = std::make_unique<API_HANDLER_COMMON>();
        Pgm().GetApiServer().RegisterHandler( m_apiHandlerCommon.get() );
    }
#endif

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
        CallAfter( [this]()
                   {
                       m_appearancePanel->OnBoardChanged();
                   } );
    }
#endif

    // Register a call to update the toolbar sizes. It can't be done immediately because
    // it seems to require some sizes calculated that aren't yet (at least on GTK).
    CallAfter( [this]()
               {
                   // Ensure the controls on the toolbars all are correctly sized
                    UpdateToolbarControlSizes();

                    // Update the angle snap mode toolbar button to reflect the current preference
                    GetToolManager()->RunAction( PCB_ACTIONS::angleSnapModeChanged );
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

    m_acceptedExts.emplace( FILEEXT::KiCadPcbFileExtension, &PCB_ACTIONS::ddAppendBoard );
    m_acceptedExts.emplace( FILEEXT::LegacyPcbFileExtension, &PCB_ACTIONS::ddAppendBoard );
    m_acceptedExts.emplace( wxS( "dxf" ), &PCB_ACTIONS::ddImportGraphics );
    m_acceptedExts.emplace( FILEEXT::SVGFileExtension, &PCB_ACTIONS::ddImportGraphics );
    DragAcceptFiles( true );

    Bind( EDA_EVT_CLOSE_DIALOG_BOOK_REPORTER, &PCB_EDIT_FRAME::onCloseModelessBookReporterDialogs, this );
}


void PCB_EDIT_FRAME::StartCrossProbeFlash( const std::vector<BOARD_ITEM*>& aItems )
{
    if( !GetPcbNewSettings()->m_CrossProbing.flash_selection )
    {
        wxLogTrace( traceCrossProbeFlash, "StartCrossProbeFlash(PCB): aborted (setting disabled) items=%zu",
                    aItems.size() );
        return;
    }

    if( aItems.empty() )
    {
        wxLogTrace( traceCrossProbeFlash, "StartCrossProbeFlash(PCB): aborted (no items)" );
        return;
    }

    // Don't start flashing if any of the items are being moved. The flash timer toggles
    // selection hide/show which corrupts the VIEW overlay state during an active move.
    for( const BOARD_ITEM* item : aItems )
    {
        if( item->IsMoving() )
        {
            wxLogTrace( traceCrossProbeFlash,
                        "StartCrossProbeFlash(PCB): aborted (items are moving)" );
            return;
        }
    }

    if( m_crossProbeFlashing )
    {
        wxLogTrace( traceCrossProbeFlash, "StartCrossProbeFlash(PCB): restarting existing flash (phase=%d)",
                    m_crossProbeFlashPhase );
        m_crossProbeFlashTimer.Stop();
    }

    wxLogTrace( traceCrossProbeFlash, "StartCrossProbeFlash(PCB): starting with %zu items", aItems.size() );

    // Store uuids
    m_crossProbeFlashItems.clear();
    for( BOARD_ITEM* it : aItems )
        m_crossProbeFlashItems.push_back( it->m_Uuid );

    m_crossProbeFlashPhase = 0;
    m_crossProbeFlashing = true;

    if( !m_crossProbeFlashTimer.GetOwner() )
        m_crossProbeFlashTimer.SetOwner( this );

    bool started = m_crossProbeFlashTimer.Start( 500, wxTIMER_CONTINUOUS ); // 0.5s intervals -> 3s total for 6 phases
    wxLogTrace( traceCrossProbeFlash, "StartCrossProbeFlash(PCB): timer start=%d id=%d",
                (int) started, m_crossProbeFlashTimer.GetId() );
}


void PCB_EDIT_FRAME::OnCrossProbeFlashTimer( wxTimerEvent& aEvent )
{
    wxLogTrace( traceCrossProbeFlash, "Timer(PCB) fired: phase=%d running=%d items=%zu",
                m_crossProbeFlashPhase, (int) m_crossProbeFlashing, m_crossProbeFlashItems.size() );

    if( !m_crossProbeFlashing )
    {
        wxLogTrace( traceCrossProbeFlash, "Timer(PCB) fired but not flashing (ignored)" );
        return;
    }

    PCB_SELECTION_TOOL* selTool = GetToolManager()->GetTool<PCB_SELECTION_TOOL>();

    if( !selTool )
        return;

    // Don't manipulate the selection while items are being moved. The move tool holds a
    // live reference to the selection and toggling hide/show on selected items corrupts
    // the VIEW overlay state, causing crashes.
    for( const KIID& id : m_crossProbeFlashItems )
    {
        if( EDA_ITEM* item = GetBoard()->ResolveItem( id, true ) )
        {
            if( item->IsMoving() )
            {
                wxLogTrace( traceCrossProbeFlash,
                            "Timer(PCB) phase=%d: items are moving, stopping flash",
                            m_crossProbeFlashPhase );
                m_crossProbeFlashing = false;
                m_crossProbeFlashTimer.Stop();
                return;
            }
        }
    }

    // Prevent recursion / IPC during flashing
    bool prevGuard = m_probingSchToPcb;
    m_probingSchToPcb = true;

    if( m_crossProbeFlashPhase % 2 == 0 )
    {
        // Hide selection
        selTool->ClearSelection( true );
        wxLogTrace( traceCrossProbeFlash, "Phase %d (PCB): cleared selection", m_crossProbeFlashPhase );
    }
    else
    {
        // Restore selection
        for( const KIID& id : m_crossProbeFlashItems )
        {
            if( EDA_ITEM* item = GetBoard()->ResolveItem( id, true ) )
                selTool->AddItemToSel( item, true );
        }

        wxLogTrace( traceCrossProbeFlash, "Phase %d (PCB): restored %zu items",
                    m_crossProbeFlashPhase, m_crossProbeFlashItems.size() );
    }

    // Force a redraw even if the canvas / frame does not currently have focus (mouse elsewhere)
    if( GetCanvas() )
    {
        GetCanvas()->ForceRefresh();
        wxLogTrace( traceCrossProbeFlash, "Phase %d (PCB): forced canvas refresh",
                    m_crossProbeFlashPhase );
    }

    m_probingSchToPcb = prevGuard;

    m_crossProbeFlashPhase++;

    if( m_crossProbeFlashPhase > 6 )
    {
        // Ensure final state (selected)
        for( const KIID& id : m_crossProbeFlashItems )
        {
            if( EDA_ITEM* item = GetBoard()->ResolveItem( id, true ) )
                selTool->AddItemToSel( item, true );
        }

        m_crossProbeFlashing = false;
        m_crossProbeFlashTimer.Stop();

        wxLogTrace( traceCrossProbeFlash, "Flashing complete (PCB). Final selection size=%zu",
                    m_crossProbeFlashItems.size() );
    }
}


PCB_EDIT_FRAME::~PCB_EDIT_FRAME()
{
    ScriptingOnDestructPcbEditFrame( this );

    if( ADVANCED_CFG::GetCfg().m_ShowEventCounters )
    {
        // Stop the timer during destruction early to avoid potential event race conditions (that
        // do happen on windows)
        m_eventCounterTimer->Stop();
        delete m_eventCounterTimer;
    }

    // Close modeless dialogs
    wxWindow* drcDlg = wxWindow::FindWindowByName( DIALOG_DRC_WINDOW_NAME );                                              
                  
    if( drcDlg )                                                                                                          
        drcDlg->Close( true );

    wxWindow* ruleEditorDlg = wxWindow::FindWindowByName( DIALOG_DRC_RULE_EDITOR_WINDOW_NAME );

    if( ruleEditorDlg )
        ruleEditorDlg->Close( true );

    // Shutdown all running tools
    if( m_toolManager )
        m_toolManager->ShutdownAllTools();

    if( GetBoard() )
        GetBoard()->RemoveAllListeners();

    // We passed ownership of these to wxAuiManager.
    // delete m_selectionFilterPanel;
    // delete m_appearancePanel;
    // delete m_propertiesPanel;
    // delete m_netInspectorPanel;

    delete m_exportNetlistAction;
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

    UpdateVariantSelectionCtrl();
}


BOARD_ITEM_CONTAINER* PCB_EDIT_FRAME::GetModel() const
{
    return m_pcb;
}


std::unique_ptr<GRID_HELPER> PCB_EDIT_FRAME::MakeGridHelper()
{
    return std::make_unique<PCB_GRID_HELPER>( m_toolManager, GetMagneticItemsSettings() );
}


void PCB_EDIT_FRAME::redrawNetnames()
{
    /*
     * While new items being scrolled into the view will get painted, they will only get
     * annotated with netname instances currently within the view.  Subsequent panning will not
     * draw newly-visible netname instances because the item has already been drawn.
     *
     * This routine, fired on idle if the viewport has changed, looks for visible items that
     * might have multiple netname instances and redraws them.  (It does not need to handle pads
     * and vias because they only ever have a single netname instance drawn on them.)
     */
    PCBNEW_SETTINGS* cfg = dynamic_cast<PCBNEW_SETTINGS*>( Kiface().KifaceSettings() );

    if( !cfg || cfg->m_Display.m_NetNames < 2 )
        return;

    KIGFX::VIEW* view = GetCanvas()->GetView();
    BOX2D        viewport = view->GetViewport();

    // Inflate to catch most of the track width
    BOX2I_MINMAX clipbox( BOX2ISafe( viewport.Inflate( pcbIUScale.mmToIU( 2.0 ) ) ) );

    for( PCB_TRACK* track : GetBoard()->Tracks() )
    {
        // Don't need to update vias
        if( track->Type() == PCB_VIA_T )
            continue;

        // Don't update invisible tracks
        if( !clipbox.Intersects( BOX2I_MINMAX( track->GetStart(), track->GetEnd() ) ) )
            continue;

        if( track->ViewGetLOD( GetNetnameLayer( track->GetLayer() ), view ) < view->GetScale() )
            view->Update( track, KIGFX::REPAINT );
    }
}


void PCB_EDIT_FRAME::SetPageSettings( const PAGE_INFO& aPageSettings )
{
    PCB_BASE_FRAME::SetPageSettings( aPageSettings );

    // Prepare drawing-sheet template
    DS_PROXY_VIEW_ITEM* drawingSheet = new DS_PROXY_VIEW_ITEM( pcbIUScale,
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
    m_toolManager->RegisterTool( new EDIT_TOOL );
    m_toolManager->RegisterTool( new PCB_EDIT_TABLE_TOOL );
    m_toolManager->RegisterTool( new GLOBAL_EDIT_TOOL );
    m_toolManager->RegisterTool( new PAD_TOOL );
    m_toolManager->RegisterTool( new DRAWING_TOOL );
    m_toolManager->RegisterTool( new PCB_POINT_EDITOR );
    m_toolManager->RegisterTool( new PCB_CONTROL );
    m_toolManager->RegisterTool( new PCB_DESIGN_BLOCK_CONTROL );
    m_toolManager->RegisterTool( new BOARD_EDITOR_CONTROL );
    m_toolManager->RegisterTool( new BOARD_INSPECTION_TOOL );
    m_toolManager->RegisterTool( new BOARD_REANNOTATE_TOOL );
    m_toolManager->RegisterTool( new ALIGN_DISTRIBUTE_TOOL );
    m_toolManager->RegisterTool( new MICROWAVE_TOOL );
    m_toolManager->RegisterTool( new POSITION_RELATIVE_TOOL );
    m_toolManager->RegisterTool( new ARRAY_TOOL );
    m_toolManager->RegisterTool( new ZONE_FILLER_TOOL );
    m_toolManager->RegisterTool( new AUTOPLACE_TOOL );
    m_toolManager->RegisterTool( new DRC_TOOL );
    m_toolManager->RegisterTool( new PCB_VIEWER_TOOLS );
    m_toolManager->RegisterTool( new CONVERT_TOOL );
    m_toolManager->RegisterTool( new PCB_GROUP_TOOL );
    m_toolManager->RegisterTool( new GENERATOR_TOOL );
    m_toolManager->RegisterTool( new SCRIPTING_TOOL );
    m_toolManager->RegisterTool( new PROPERTIES_TOOL );
    m_toolManager->RegisterTool( new MULTICHANNEL_TOOL );
    m_toolManager->RegisterTool( new EMBED_TOOL );
    m_toolManager->RegisterTool( new DRC_RULE_EDITOR_TOOL );
    m_toolManager->InitTools();

    for( TOOL_BASE* tool : m_toolManager->Tools() )
    {
        if( PCB_TOOL_BASE* pcbTool = dynamic_cast<PCB_TOOL_BASE*>( tool ) )
            pcbTool->SetIsBoardEditor( true );
    }

    // Run the selection tool, it is supposed to be always active
    m_toolManager->InvokeTool( "common.InteractiveSelection" );
}


void PCB_EDIT_FRAME::setupUIConditions()
{
    PCB_BASE_EDIT_FRAME::setupUIConditions();

    ACTION_MANAGER*       mgr = m_toolManager->GetActionManager();
    PCB_EDITOR_CONDITIONS cond( this );

    auto undoCond =
            [ this ] (const SELECTION& aSel )
            {
                DRAWING_TOOL* drawingTool = m_toolManager->GetTool<DRAWING_TOOL>();

                if( drawingTool && drawingTool->GetDrawingMode() != DRAWING_TOOL::MODE::NONE )
                    return true;

                ROUTER_TOOL* routerTool = m_toolManager->GetTool<ROUTER_TOOL>();

                if( routerTool && routerTool->RoutingInProgress() )
                    return true;

                return GetUndoCommandCount() > 0;
            };

    auto groupWithDesignBlockLink =
            [] ( const SELECTION& aSel )
            {
                if( aSel.Size() != 1 )
                    return false;

                if( aSel[0]->Type() != PCB_GROUP_T )
                    return false;

                PCB_GROUP* group = static_cast<PCB_GROUP*>( aSel.GetItem( 0 ) );

                return group->HasDesignBlockLink();
            };

    wxASSERT( mgr );

#define ENABLE( x ) ACTION_CONDITIONS().Enable( x )
#define CHECK( x )  ACTION_CONDITIONS().Check( x )
// clang-format off

    mgr->SetConditions( ACTIONS::save,         ENABLE( SELECTION_CONDITIONS::ShowAlways ) );
    mgr->SetConditions( ACTIONS::undo,         ENABLE( undoCond ) );
    mgr->SetConditions( ACTIONS::redo,         ENABLE( cond.RedoAvailable() ) );

    mgr->SetConditions( ACTIONS::toggleGrid,          CHECK( cond.GridVisible() ) );
    mgr->SetConditions( ACTIONS::toggleGridOverrides, CHECK( cond.GridOverrides() ) );
    mgr->SetConditions( ACTIONS::togglePolarCoords,   CHECK( cond.PolarCoordinates() ) );

    mgr->SetConditions( ACTIONS::cut,          ENABLE( cond.HasItems() ) );
    mgr->SetConditions( ACTIONS::copy,         ENABLE( cond.HasItems() ) );
    mgr->SetConditions( ACTIONS::paste,        ENABLE( SELECTION_CONDITIONS::Idle && cond.NoActiveTool() ) );
    mgr->SetConditions( ACTIONS::pasteSpecial, ENABLE( SELECTION_CONDITIONS::Idle && cond.NoActiveTool() ) );
    mgr->SetConditions( ACTIONS::selectAll,    ENABLE( cond.HasItems() ) );
    mgr->SetConditions( ACTIONS::unselectAll,  ENABLE( cond.HasItems() ) );
    mgr->SetConditions( ACTIONS::doDelete,     ENABLE( cond.HasItems() ) );
    mgr->SetConditions( ACTIONS::duplicate,    ENABLE( cond.HasItems() ) );

    static const std::vector<KICAD_T> groupTypes = { PCB_GROUP_T, PCB_GENERATOR_T };

    mgr->SetConditions( ACTIONS::group,        ENABLE( SELECTION_CONDITIONS::MoreThan( 1 ) ) );
    mgr->SetConditions( ACTIONS::ungroup,      ENABLE( SELECTION_CONDITIONS::HasTypes( groupTypes ) ) );
    mgr->SetConditions( PCB_ACTIONS::lock,     ENABLE( PCB_SELECTION_CONDITIONS::HasUnlockedItems ) );
    mgr->SetConditions( PCB_ACTIONS::unlock,   ENABLE( PCB_SELECTION_CONDITIONS::HasLockedItems ) );

    mgr->SetConditions( PCB_ACTIONS::placeLinkedDesignBlock, ENABLE( groupWithDesignBlockLink) );
    mgr->SetConditions( PCB_ACTIONS::saveToLinkedDesignBlock, ENABLE( groupWithDesignBlockLink) );

    mgr->SetConditions( PCB_ACTIONS::padDisplayMode,   CHECK( !cond.PadFillDisplay() ) );
    mgr->SetConditions( PCB_ACTIONS::viaDisplayMode,   CHECK( !cond.ViaFillDisplay() ) );
    mgr->SetConditions( PCB_ACTIONS::trackDisplayMode, CHECK( !cond.TrackFillDisplay() ) );
    mgr->SetConditions( PCB_ACTIONS::graphicsOutlines, CHECK( !cond.GraphicsFillDisplay() ) );
    mgr->SetConditions( PCB_ACTIONS::textOutlines,     CHECK( !cond.TextFillDisplay() ) );

    if( SCRIPTING::IsWxAvailable() )
        mgr->SetConditions( PCB_ACTIONS::showPythonConsole, CHECK( cond.ScriptingConsoleVisible() ) );

    auto enableZoneControlCondition =
            [this] ( const SELECTION& )
            {
                return GetBoard() && GetBoard()->GetVisibleElements().Contains( LAYER_ZONES )
                        && GetDisplayOptions().m_ZoneOpacity > 0.0;
            };

    mgr->SetConditions( PCB_ACTIONS::zoneDisplayFilled,
                        ENABLE( enableZoneControlCondition )
                        .Check( cond.ZoneDisplayMode( ZONE_DISPLAY_MODE::SHOW_FILLED ) ) );
    mgr->SetConditions( PCB_ACTIONS::zoneDisplayOutline,
                        ENABLE( enableZoneControlCondition )
                        .Check( cond.ZoneDisplayMode( ZONE_DISPLAY_MODE::SHOW_ZONE_OUTLINE ) ) );
    mgr->SetConditions( PCB_ACTIONS::zoneDisplayFractured,
                        ENABLE( enableZoneControlCondition )
                        .Check( cond.ZoneDisplayMode( ZONE_DISPLAY_MODE::SHOW_FRACTURE_BORDERS ) ) );
    mgr->SetConditions( PCB_ACTIONS::zoneDisplayTriangulated,
                        ENABLE( enableZoneControlCondition )
                        .Check( cond.ZoneDisplayMode( ZONE_DISPLAY_MODE::SHOW_TRIANGULATION ) ) );

    mgr->SetConditions( ACTIONS::toggleBoundingBoxes, CHECK( cond.BoundingBoxes() ) );

    auto hasElements =
            [ this ] ( const SELECTION& aSel )
            {
                return GetBoard() &&
                        ( !GetBoard()->IsEmpty() || !SELECTION_CONDITIONS::Idle( aSel ) );
            };

    auto boardFlippedCond =
            [this]( const SELECTION& )
            {
                return GetCanvas() && GetCanvas()->GetView()->IsMirroredX();
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

    auto netInspectorCond =
            [this] ( const SELECTION& )
            {
                return NetInspectorShown();
            };

    auto searchPaneCond =
            [this] ( const SELECTION& )
            {
                return m_auimgr.GetPane( SearchPaneName() ).IsShown();
            };

    auto designBlockCond =
            [ this ] (const SELECTION& aSel )
            {
                return m_auimgr.GetPane( DesignBlocksPaneName() ).IsShown();
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
                return tool && tool->IsNetHighlightSet();
            };

    mgr->SetConditions( ACTIONS::highContrastMode,         CHECK( highContrastCond ) );
    mgr->SetConditions( PCB_ACTIONS::flipBoard,            CHECK( boardFlippedCond ) );
    mgr->SetConditions( PCB_ACTIONS::showLayersManager,    CHECK( layerManagerCond ) );
    mgr->SetConditions( PCB_ACTIONS::showRatsnest,         CHECK( globalRatsnestCond ) );
    mgr->SetConditions( PCB_ACTIONS::ratsnestLineMode,     CHECK( curvedRatsnestCond ) );
    mgr->SetConditions( PCB_ACTIONS::toggleNetHighlight,   CHECK( netHighlightCond )
                                                           .Enable( enableNetHighlightCond ) );
    mgr->SetConditions( ACTIONS::showProperties,           CHECK( propertiesCond ) );
    mgr->SetConditions( PCB_ACTIONS::showNetInspector,     CHECK( netInspectorCond ) );
    mgr->SetConditions( PCB_ACTIONS::showSearch,           CHECK( searchPaneCond ) );
    mgr->SetConditions( PCB_ACTIONS::showDesignBlockPanel, CHECK( designBlockCond ) );

    mgr->SetConditions( PCB_ACTIONS::saveBoardAsDesignBlock,     ENABLE( hasElements ) );
    mgr->SetConditions( PCB_ACTIONS::saveSelectionAsDesignBlock, ENABLE( SELECTION_CONDITIONS::NotEmpty ) );

    const auto isArcKeepCenterMode =
            [this]( const SELECTION& )
            {
                return GetPcbNewSettings()->m_ArcEditMode == ARC_EDIT_MODE::KEEP_CENTER_ADJUST_ANGLE_RADIUS;
            };

    const auto isArcKeepEndpointMode =
            [this]( const SELECTION& )
            {
                return GetPcbNewSettings()->m_ArcEditMode == ARC_EDIT_MODE::KEEP_ENDPOINTS_OR_START_DIRECTION;
            };

    const auto isArcKeepRadiusMode =
            [this]( const SELECTION& )
            {
                return GetPcbNewSettings()->m_ArcEditMode == ARC_EDIT_MODE::KEEP_CENTER_ENDS_ADJUST_ANGLE;
            };

    mgr->SetConditions( ACTIONS::pointEditorArcKeepCenter,   CHECK( isArcKeepCenterMode ) );
    mgr->SetConditions( ACTIONS::pointEditorArcKeepEndpoint, CHECK( isArcKeepEndpointMode ) );
    mgr->SetConditions( ACTIONS::pointEditorArcKeepRadius,   CHECK( isArcKeepRadiusMode ) );

    auto isHighlightMode =
            [this]( const SELECTION& )
            {
                ROUTER_TOOL* tool = m_toolManager->GetTool<ROUTER_TOOL>();
                return tool && tool->GetRouterMode() == PNS::RM_MarkObstacles;
            };

    auto isShoveMode =
            [this]( const SELECTION& )
            {
                ROUTER_TOOL* tool = m_toolManager->GetTool<ROUTER_TOOL>();
                return tool && tool->GetRouterMode() == PNS::RM_Shove;
            };

    auto isWalkaroundMode =
            [this]( const SELECTION& )
            {
                ROUTER_TOOL* tool = m_toolManager->GetTool<ROUTER_TOOL>();
                return tool && tool->GetRouterMode() == PNS::RM_Walkaround;
            };

    mgr->SetConditions( PCB_ACTIONS::routerHighlightMode,  CHECK( isHighlightMode ) );
    mgr->SetConditions( PCB_ACTIONS::routerShoveMode,      CHECK( isShoveMode ) );
    mgr->SetConditions( PCB_ACTIONS::routerWalkaroundMode, CHECK( isWalkaroundMode ) );

    auto isAutoTrackWidth =
            [this]( const SELECTION& )
            {
                return GetDesignSettings().m_UseConnectedTrackWidth;
            };

    mgr->SetConditions( PCB_ACTIONS::autoTrackWidth, CHECK( isAutoTrackWidth ) );

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

    static const std::vector<KICAD_T> trackTypes =      { PCB_TRACE_T, PCB_ARC_T, PCB_VIA_T };
    static const std::vector<KICAD_T> padOwnerTypes =   { PCB_FOOTPRINT_T, PCB_PAD_T };
    static const std::vector<KICAD_T> footprintTypes =  { PCB_FOOTPRINT_T };
    static const std::vector<KICAD_T> crossProbeTypes = { PCB_PAD_T, PCB_FOOTPRINT_T, PCB_GROUP_T };
    static const std::vector<KICAD_T> zoneTypes =       { PCB_ZONE_T };

    mgr->SetConditions( PCB_ACTIONS::selectNet,         ENABLE( SELECTION_CONDITIONS::OnlyTypes( trackTypes ) ) );
    mgr->SetConditions( PCB_ACTIONS::deselectNet,       ENABLE( SELECTION_CONDITIONS::OnlyTypes( trackTypes ) ) );
    mgr->SetConditions( PCB_ACTIONS::selectUnconnected, ENABLE( SELECTION_CONDITIONS::OnlyTypes( padOwnerTypes ) ) );
    mgr->SetConditions( PCB_ACTIONS::selectSameSheet,   ENABLE( SELECTION_CONDITIONS::OnlyTypes( footprintTypes ) ) );
    mgr->SetConditions( PCB_ACTIONS::selectOnSchematic, ENABLE( SELECTION_CONDITIONS::HasTypes( crossProbeTypes ) ) );


    SELECTION_CONDITION singleZoneCond = SELECTION_CONDITIONS::Count( 1 )
                                    && SELECTION_CONDITIONS::OnlyTypes( zoneTypes );

    SELECTION_CONDITION zoneMergeCond = SELECTION_CONDITIONS::MoreThan( 1 )
                                    && SELECTION_CONDITIONS::OnlyTypes( zoneTypes );

    mgr->SetConditions( PCB_ACTIONS::zoneDuplicate,   ENABLE( singleZoneCond ) );
    mgr->SetConditions( PCB_ACTIONS::drawZoneCutout,  ENABLE( singleZoneCond ) );
    mgr->SetConditions( PCB_ACTIONS::drawSimilarZone, ENABLE( singleZoneCond ) );
    mgr->SetConditions( PCB_ACTIONS::zoneMerge,       ENABLE( zoneMergeCond ) );

#define CURRENT_TOOL( action ) mgr->SetConditions( action, CHECK( cond.CurrentTool( action ) ) )

    // These tools can be used at any time to inspect the board
    CURRENT_TOOL( ACTIONS::zoomTool );
    CURRENT_TOOL( ACTIONS::measureTool );
    CURRENT_TOOL( ACTIONS::selectionTool );
    CURRENT_TOOL( PCB_ACTIONS::localRatsnestTool );

    auto isDRCIdle =
            [this] ( const SELECTION& )
            {
                DRC_TOOL* tool = m_toolManager->GetTool<DRC_TOOL>();
                return !( tool && tool->IsDRCRunning() );
            };

#define CURRENT_EDIT_TOOL( action )                                                             \
            mgr->SetConditions( action, ACTION_CONDITIONS().Check( cond.CurrentTool( action ) ) \
                                                           .Enable( isDRCIdle ) )

    // These tools edit the board, so they must be disabled during some operations
    CURRENT_EDIT_TOOL( ACTIONS::embeddedFiles );
    CURRENT_EDIT_TOOL( ACTIONS::deleteTool );
    CURRENT_EDIT_TOOL( PCB_ACTIONS::placeFootprint );
    CURRENT_EDIT_TOOL( PCB_ACTIONS::placeDesignBlock );
    CURRENT_EDIT_TOOL( PCB_ACTIONS::routeSingleTrack);
    CURRENT_EDIT_TOOL( PCB_ACTIONS::routeDiffPair );
    CURRENT_EDIT_TOOL( PCB_ACTIONS::tuneSingleTrack);
    CURRENT_EDIT_TOOL( PCB_ACTIONS::tuneDiffPair );
    CURRENT_EDIT_TOOL( PCB_ACTIONS::tuneSkew );
    CURRENT_EDIT_TOOL( PCB_ACTIONS::drawVia );
    CURRENT_EDIT_TOOL( PCB_ACTIONS::drawZone );
    CURRENT_EDIT_TOOL( PCB_ACTIONS::drawRuleArea );
    CURRENT_EDIT_TOOL( PCB_ACTIONS::drawLine );
    CURRENT_EDIT_TOOL( PCB_ACTIONS::drawRectangle );
    CURRENT_EDIT_TOOL( PCB_ACTIONS::drawCircle );
    CURRENT_EDIT_TOOL( PCB_ACTIONS::drawArc );
    CURRENT_EDIT_TOOL( PCB_ACTIONS::drawPolygon );
    CURRENT_EDIT_TOOL( PCB_ACTIONS::drawBezier );
    CURRENT_EDIT_TOOL( PCB_ACTIONS::placePoint );
    CURRENT_EDIT_TOOL( PCB_ACTIONS::placeReferenceImage );
    CURRENT_EDIT_TOOL( PCB_ACTIONS::placeText );
    CURRENT_EDIT_TOOL( PCB_ACTIONS::drawTextBox );
    CURRENT_EDIT_TOOL( PCB_ACTIONS::drawTable );
    CURRENT_EDIT_TOOL( PCB_ACTIONS::drawAlignedDimension );
    CURRENT_EDIT_TOOL( PCB_ACTIONS::drawOrthogonalDimension );
    CURRENT_EDIT_TOOL( PCB_ACTIONS::drawCenterDimension );
    CURRENT_EDIT_TOOL( PCB_ACTIONS::drawRadialDimension );
    CURRENT_EDIT_TOOL( PCB_ACTIONS::drawLeader );
    CURRENT_EDIT_TOOL( PCB_ACTIONS::drillOrigin );
    CURRENT_EDIT_TOOL( ACTIONS::gridSetOrigin );
    CURRENT_EDIT_TOOL( PCB_ACTIONS::createArray );

    CURRENT_EDIT_TOOL( PCB_ACTIONS::microwaveCreateLine );
    CURRENT_EDIT_TOOL( PCB_ACTIONS::microwaveCreateGap );
    CURRENT_EDIT_TOOL( PCB_ACTIONS::microwaveCreateStub );
    CURRENT_EDIT_TOOL( PCB_ACTIONS::microwaveCreateStubArc );
    CURRENT_EDIT_TOOL( PCB_ACTIONS::microwaveCreateFunctionShape );

#undef CURRENT_TOOL
#undef CURRENT_EDIT_TOOL
#undef ENABLE
#undef CHECK
// clang-format on
}


void PCB_EDIT_FRAME::OnQuit( wxCommandEvent& event )
{
    if( event.GetId() == wxID_EXIT )
        Kiway().OnKiCadExit();

    if( event.GetId() == wxID_CLOSE || Kiface().IsSingle() )
        Close( false );
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

    // Don't allow closing while the modal footprint chooser is open
    auto* chooser = (FOOTPRINT_CHOOSER_FRAME*) Kiway().Player( FRAME_FOOTPRINT_CHOOSER, false );

    if( chooser && chooser->IsModal() ) // Can close footprint chooser?
        return false;

    if( Kiface().IsSingle() )
    {
        auto* fpEditor = (FOOTPRINT_EDIT_FRAME*) Kiway().Player( FRAME_FOOTPRINT_EDITOR, false );

        if( fpEditor && !fpEditor->Close() )   // Can close footprint editor?
            return false;

        auto* fpViewer = (FOOTPRINT_VIEWER_FRAME*) Kiway().Player( FRAME_FOOTPRINT_VIEWER, false );

        if( fpViewer && !fpViewer->Close() )   // Can close footprint viewer?
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
                                       return SaveBoard();
                                   } ) )
        {
            return false;
        }

        // If user discarded changes, create a duplicate commit of last saved PCB state and
        // advance Last_Save_pcb tag for explicit history event.
        if( GetLastUnsavedChangesResponse() == wxID_NO )
        {
            wxString projPath = Prj().GetProjectPath();

            if( !projPath.IsEmpty() && Kiway().LocalHistory().HistoryExists( projPath ) )
            {
                Kiway().LocalHistory().CommitDuplicateOfLastSave( projPath, wxS("pcb"),
                        wxS("Discard unsaved pcb changes") );
            }
        }
    }

    return PCB_BASE_EDIT_FRAME::canCloseWindow( aEvent );
}


void PCB_EDIT_FRAME::doCloseWindow()
{
    // Unregister the autosave saver before any cleanup that might invalidate the board
    if( GetBoard() )
        Kiway().LocalHistory().UnregisterSaver( GetBoard() );

    // On Windows 7 / 32 bits, on OpenGL mode only, Pcbnew crashes
    // when closing this frame if a footprint was selected, and the footprint editor called
    // to edit this footprint, and when closing pcbnew if this footprint is still selected
    // See https://bugs.launchpad.net/kicad/+bug/1655858
    // I think this is certainly a OpenGL event fired after frame deletion, so this workaround
    // avoid the crash (JPC)
    GetCanvas()->SetEvtHandlerEnabled( false );

    GetCanvas()->StopDrawing();

#ifdef KICAD_IPC_API
    Pgm().GetApiServer().DeregisterHandler( m_apiHandler.get() );
    wxTheApp->Unbind( EDA_EVT_PLUGIN_AVAILABILITY_CHANGED,
                      &PCB_EDIT_FRAME::onPluginAvailabilityChanged, this );
#endif

    // Clean up mode-less dialogs.
    Unbind( EDA_EVT_CLOSE_DIALOG_BOOK_REPORTER, &PCB_EDIT_FRAME::onCloseModelessBookReporterDialogs,
            this );

    wxWindow* drcDlg = wxWindow::FindWindowByName( DIALOG_DRC_WINDOW_NAME );                                              
                  
    if( drcDlg )                                                                                                          
        drcDlg->Close( true );

    wxWindow* ruleEditorDlg = wxWindow::FindWindowByName( DIALOG_DRC_RULE_EDITOR_WINDOW_NAME );

    if( ruleEditorDlg )
        ruleEditorDlg->Close( true );


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

    // Delete the auto save file if it exists.
    wxFileName fn = GetBoard()->GetFileName();

    // Make sure local settings are persisted
    if( Prj().GetLocalSettings().ShouldAutoSave() )
    {
        m_netInspectorPanel->SaveSettings();
        SaveProjectLocalSettings();
    }
    else
    {
        wxLogTrace( traceAutoSave, wxT( "Skipping auto-save of migrated local settings" ) );
    }

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


void PCB_EDIT_FRAME::ShowBoardSetupDialog( const wxString& aInitialPage, wxWindow* aParent )
{
    static std::mutex dialogMutex; // Local static mutex

    std::unique_lock<std::mutex> dialogLock( dialogMutex, std::try_to_lock );

    // One dialog at a time.
    if( !dialogLock.owns_lock() )
    {
        if( m_boardSetupDlg && m_boardSetupDlg->IsShown() )
        {
            m_boardSetupDlg->Raise(); // Brings the existing dialog to the front
        }

        return;
    }

    // Make sure everything's up-to-date
    GetBoard()->BuildListOfNets();

    DIALOG_BOARD_SETUP dlg( this, aParent );

    if( !aInitialPage.IsEmpty() )
        dlg.SetInitialPage( aInitialPage, wxEmptyString );

    // Assign dlg to the m_boardSetupDlg pointer to track its status.
    m_boardSetupDlg = &dlg;

    // QuasiModal required for Scintilla auto-complete
    if( dlg.ShowQuasiModal() == wxID_OK )
    {
        // Note: We must synchronise time domain properties before nets and classes, otherwise the updates
        // called by the board listener events are using stale data
        GetBoard()->SynchronizeTuningProfileProperties();
        GetBoard()->SynchronizeNetsAndNetClasses( true );

        if( !GetBoard()->SynchronizeComponentClasses( std::unordered_set<wxString>() ) )
        {
            m_infoBar->RemoveAllButtons();
            m_infoBar->AddCloseButton();
            m_infoBar->ShowMessage( _( "Could not load component class assignment rules" ),
                                    wxICON_WARNING, WX_INFOBAR::MESSAGE_TYPE::GENERIC );
        }

        // We don't know if anything was modified, so err on the side of requiring a save
        OnModify();

        Kiway().CommonSettingsChanged( TEXTVARS_CHANGED );

        Prj().IncrementTextVarsTicker();
        Prj().IncrementNetclassesTicker();

        PCBNEW_SETTINGS* settings = GetPcbNewSettings();
        static LSET      maskAndPasteLayers = LSET( { F_Mask, F_Paste, B_Mask, B_Paste } );

        GetCanvas()->GetView()->UpdateAllItemsConditionally(
                [&]( KIGFX::VIEW_ITEM* aItem ) -> int
                {
                    int flags = 0;

                    if( !aItem->IsBOARD_ITEM() )
                        return flags;

                    BOARD_ITEM* item = static_cast<BOARD_ITEM*>( aItem );

                    if( item->Type() == PCB_VIA_T || item->Type() == PCB_PAD_T )
                    {
                        // Note: KIGFX::REPAINT isn't enough for things that go from invisible
                        // to visible as they won't be found in the view layer's itemset for
                        // re-painting.
                        if( ( GetBoard()->GetVisibleLayers() & maskAndPasteLayers ).any() )
                            flags |= KIGFX::ALL;
                    }

                    if( item->Type() == PCB_TRACE_T || item->Type() == PCB_ARC_T || item->Type() == PCB_VIA_T )
                    {
                        if( settings->m_Display.m_TrackClearance == SHOW_WITH_VIA_ALWAYS )
                            flags |= KIGFX::REPAINT;
                    }

                    if( item->Type() == PCB_PAD_T )
                    {
                        if( settings->m_Display.m_PadClearance )
                            flags |= KIGFX::REPAINT;
                    }

                    if( EDA_TEXT* text = dynamic_cast<EDA_TEXT*>( aItem ) )
                    {
                        if( text->HasTextVars() )
                        {
                            text->ClearRenderCache();
                            text->ClearBoundingBoxCache();
                            flags |= KIGFX::GEOMETRY | KIGFX::REPAINT;
                        }
                    }

                    return flags;
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

    // Reset m_boardSetupDlg after the dialog is closed
    m_boardSetupDlg = nullptr;
}


void PCB_EDIT_FRAME::FocusSearch()
{
    m_searchPane->FocusSearch();
}


void PCB_EDIT_FRAME::LoadSettings( APP_SETTINGS_BASE* aCfg )
{
    PCB_BASE_FRAME::LoadSettings( aCfg );

    PCBNEW_SETTINGS* cfg = dynamic_cast<PCBNEW_SETTINGS*>( aCfg );
    wxASSERT( cfg );

    if( cfg )
    {
        m_show_layer_manager_tools = cfg->m_AuiPanels.show_layer_manager;
        m_show_search              = cfg->m_AuiPanels.show_search;
        m_show_net_inspector       = cfg->m_AuiPanels.show_net_inspector;
    }
}


void PCB_EDIT_FRAME::SaveSettings( APP_SETTINGS_BASE* aCfg )
{
    PCB_BASE_FRAME::SaveSettings( aCfg );

    PCBNEW_SETTINGS* cfg = dynamic_cast<PCBNEW_SETTINGS*>( aCfg );
    wxASSERT( cfg );

    if( cfg )
    {
        wxAuiPaneInfo& apperancePane = m_auimgr.GetPane( AppearancePanelName() );
        cfg->m_AuiPanels.show_layer_manager = apperancePane.IsShown();

        if( m_propertiesPanel )
        {
            cfg->m_AuiPanels.show_properties        = m_propertiesPanel->IsShownOnScreen();
            cfg->m_AuiPanels.properties_panel_width = m_propertiesPanel->GetSize().x;
            cfg->m_AuiPanels.properties_splitter    = m_propertiesPanel->SplitterProportion();
        }

        // ensure m_show_search is up to date (the pane can be closed)
        wxAuiPaneInfo& searchPaneInfo = m_auimgr.GetPane( SearchPaneName() );
        m_show_search = searchPaneInfo.IsShown();
        cfg->m_AuiPanels.show_search = m_show_search;
        cfg->m_AuiPanels.search_panel_height = m_searchPane->GetSize().y;
        cfg->m_AuiPanels.search_panel_width = m_searchPane->GetSize().x;
        cfg->m_AuiPanels.search_panel_dock_direction = searchPaneInfo.dock_direction;

        if( m_netInspectorPanel )
        {
            wxAuiPaneInfo& netInspectorhPaneInfo = m_auimgr.GetPane( NetInspectorPanelName() );
            m_show_net_inspector = netInspectorhPaneInfo.IsShown();
            cfg->m_AuiPanels.show_net_inspector = m_show_net_inspector;
        }

        if( m_appearancePanel )
        {
            cfg->m_AuiPanels.right_panel_width               = m_appearancePanel->GetSize().x;
            cfg->m_AuiPanels.appearance_panel_tab            = m_appearancePanel->GetTabIndex();
            cfg->m_AuiPanels.appearance_expand_layer_display = m_appearancePanel->IsLayerOptionsExpanded();
            cfg->m_AuiPanels.appearance_expand_net_display   = m_appearancePanel->IsNetOptionsExpanded();
        }

        wxAuiPaneInfo& designBlocksPane = m_auimgr.GetPane( DesignBlocksPaneName() );
        cfg->m_AuiPanels.design_blocks_show = designBlocksPane.IsShown();

        if( designBlocksPane.IsDocked() )
            cfg->m_AuiPanels.design_blocks_panel_docked_width = m_designBlocksPane->GetSize().x;
        else
        {
            cfg->m_AuiPanels.design_blocks_panel_float_height = designBlocksPane.floating_size.y;
            cfg->m_AuiPanels.design_blocks_panel_float_width = designBlocksPane.floating_size.x;
        }

        m_designBlocksPane->SaveSettings();
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


void PCB_EDIT_FRAME::SetActiveLayer( PCB_LAYER_ID aLayer, bool aForceRedraw )
{
    const PCB_LAYER_ID oldLayer = GetActiveLayer();

    if( oldLayer == aLayer && !aForceRedraw )
        return;

    PCB_BASE_FRAME::SetActiveLayer( aLayer );

    m_appearancePanel->OnLayerChanged();

    m_toolManager->PostAction( PCB_ACTIONS::layerChanged );  // notify other tools
    GetCanvas()->SetFocus();                                // allow capture of hotkeys
    GetCanvas()->SetHighContrastLayer( aLayer );

    /*
    * Only show pad, via and track clearances when a copper layer is active
    * and then only show the clearance layer for that copper layer. For
    * front/back non-copper layers, show the clearance layer for the outer
    * layer on that side.
    *
    * For pads/vias, this is to avoid clutter when there are pad/via layers
    * that vary in flash (i.e. clearance from the hole or pad edge), padstack
    * shape on each layer or clearances on each layer.
    *
    * For tracks, this follows the same logic as pads/vias, but in theory could
    * have their own set of independent clearance layers to allow track clearance
    * to be shown for more layers.
    */
    const auto getClearanceLayerForActive = []( PCB_LAYER_ID aActiveLayer ) -> std::optional<int>
    {
        if( IsCopperLayer( aActiveLayer ) )
            return CLEARANCE_LAYER_FOR( aActiveLayer );

        return std::nullopt;
    };

    if( std::optional<int> oldClearanceLayer = getClearanceLayerForActive( oldLayer ) )
        GetCanvas()->GetView()->SetLayerVisible( *oldClearanceLayer, false );

    if( std::optional<int> newClearanceLayer = getClearanceLayerForActive( aLayer ) )
        GetCanvas()->GetView()->SetLayerVisible( *newClearanceLayer, true );

    GetCanvas()->GetView()->UpdateAllItemsConditionally(
            [&]( KIGFX::VIEW_ITEM* aItem ) -> int
            {
                if( !aItem->IsBOARD_ITEM() )
                    return 0;

                BOARD_ITEM* item = static_cast<BOARD_ITEM*>( aItem );

                // Note: KIGFX::REPAINT isn't enough for things that go from invisible to visible
                // as they won't be found in the view layer's itemset for re-painting.
                if( GetDisplayOptions().m_ContrastModeDisplay == HIGH_CONTRAST_MODE::HIDDEN )
                {
                    if( item->IsOnLayer( oldLayer ) || item->IsOnLayer( aLayer ) )
                        return KIGFX::ALL;
                }

                if( item->Type() == PCB_VIA_T )
                {
                    PCB_VIA* via = static_cast<PCB_VIA*>( item );

                    // Vias on a restricted layer set must be redrawn when the active layer
                    // is changed
                    if( via->GetViaType() == VIATYPE::BLIND
                            || via->GetViaType() == VIATYPE::BURIED
                            || via->GetViaType() == VIATYPE::MICROVIA )
                    {
                        return KIGFX::REPAINT;
                    }

                    if( via->GetRemoveUnconnected() )
                        return KIGFX::ALL;
                }
                else if( item->Type() == PCB_PAD_T )
                {
                    PAD* pad = static_cast<PAD*>( item );

                    if( pad->GetRemoveUnconnected() )
                        return KIGFX::ALL;
                }

                return 0;
            } );

    GetCanvas()->Refresh();
}


void PCB_EDIT_FRAME::OnBoardLoaded()
{
    wxFileName fn( GetBoard()->GetFileName() );
    Kiway().LocalHistory().Init( fn.GetPath() );
    ENUM_MAP<PCB_LAYER_ID>& layerEnum = ENUM_MAP<PCB_LAYER_ID>::Instance();

    layerEnum.Choices().Clear();
    layerEnum.Undefined( UNDEFINED_LAYER );

    for( PCB_LAYER_ID layer : LSET::AllLayersMask() )
    {
        // Canonical name
        layerEnum.Map( layer, LSET::Name( layer ) );

        // User name
        layerEnum.Map( layer, GetBoard()->GetLayerName( layer ) );
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

    GetBoard()->InitializeClearanceCache();

    UpdateTitle();

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
        SetActiveLayer( localSettings.m_ActiveLayer, true );
    else
        SetActiveLayer( GetActiveLayer(), true );   // Make sure to repaint even if not switching

    PROJECT_FILE& projectFile = Prj().GetProjectFile();

    m_layerPairSettings->SetLayerPairs( projectFile.m_LayerPairInfos );
    m_layerPairSettings->SetCurrentLayerPair( LAYER_PAIR{ F_Cu, B_Cu } );

    // Updates any auto dimensions and the auxiliary toolbar tracks/via sizes
    unitsChangeRefresh();

    // Sync the net inspector now we have connectivity calculated
    if( m_netInspectorPanel )
        m_netInspectorPanel->OnBoardChanged();

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
    m_auimgr.GetPane( m_netInspectorPanel ).Caption( _( "Net Inspector" ) );
    m_auimgr.Update();

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
    Kiway().LocalHistory().NoteFileChange( GetBoard()->GetFileName() );
    m_ZoneFillsDirty = true;

    if( m_isClosing )
        return;

    Update3DView( true, GetPcbNewSettings()->m_Display.m_Live3DRefresh );

    if( !GetTitle().StartsWith( wxT( "*" ) ) )
        UpdateTitle();

}


void PCB_EDIT_FRAME::HardRedraw()
{
    Update3DView( true, true );

    std::shared_ptr<CONNECTIVITY_DATA> connectivity = GetBoard()->GetConnectivity();
    connectivity->RecalculateRatsnest( nullptr );
    GetCanvas()->RedrawRatsnest();

    std::vector<MSG_PANEL_ITEM> msg_list;
    GetBoard()->GetMsgPanelInfo( this, msg_list );
    SetMsgPanel( msg_list );
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

    for( PCB_LAYER_ID layer : LSET::AllLayersMask() )
    {
        // Canonical name
        layerEnum.Map( layer, LSET::Name( layer ) );

        // User name
        layerEnum.Map( layer, GetBoard()->GetLayerName( layer ) );
    }

    GetToolManager()->RunAction( PCB_ACTIONS::angleSnapModeChanged );

    // Sync visibility with canvas
    for( PCB_LAYER_ID layer : LSET::AllLayersMask() )
        GetCanvas()->GetView()->SetLayerVisible( layer, GetBoard()->IsLayerVisible( layer ) );

    // Stackup and/or color theme may have changed
    m_appearancePanel->OnBoardChanged();
    m_netInspectorPanel->OnParentSetupChanged();
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

        case PCB_FIELD_T:
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

    if( !frame->IsShownOnScreen() )
    {
        wxEventBlocker blocker( this );
        wxFileName fn( Prj().GetProjectPath(), Prj().GetProjectName(),
                       FILEEXT::KiCadSchematicFileExtension );

        // Maybe the file hasn't been converted to the new s-expression file format so
        // see if the legacy schematic file is still in play.
        if( !fn.FileExists() )
        {
            fn.SetExt( FILEEXT::LegacySchematicFileExtension );

            if( !fn.FileExists() )
            {
                DisplayErrorMessage( this, _( "The schematic for this board cannot be found." ) );
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
    int standalone = TestStandalone();

    if( standalone == 0 )
    {
        DisplayErrorMessage( this, _( "Cannot update the PCB because PCB editor is opened in "
                                      "stand-alone mode. In order to create or update PCBs from "
                                      "schematics, you must launch the KiCad project manager and "
                                      "create a project." ) );
        return false;       // Not in standalone mode
    }

    if( standalone < 0 )      // Problem with Eeschema or the schematic
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

        dlg.ShowQuasiModal();
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
        if( KIWAY_PLAYER* frame = Kiway().Player( FRAME_FOOTPRINT_EDITOR, true ) )
        {
            FOOTPRINT_EDIT_FRAME* fp_editor = static_cast<FOOTPRINT_EDIT_FRAME*>( frame );

            fp_editor->LoadFootprintFromBoard( aFootprint );
            fp_editor->Show( true );
            fp_editor->Raise();        // Iconize( false );
        }
    }
    else if( retvalue == DIALOG_FOOTPRINT_PROPERTIES::FP_PROPS_EDIT_LIBRARY_FP )
    {
        if( KIWAY_PLAYER* frame = Kiway().Player( FRAME_FOOTPRINT_EDITOR, true ) )
        {
            FOOTPRINT_EDIT_FRAME* fp_editor = static_cast<FOOTPRINT_EDIT_FRAME*>( frame );

            fp_editor->LoadFootprintFromLibrary( aFootprint->GetFPID() );
            fp_editor->Show( true );
            fp_editor->Raise();        // Iconize( false );
        }
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


/**
 * copy text settings from aSrc to aDest
 * @param aSrc is the PCB_TEXT source
 * @param aDest is the PCB_TEXT target
 * @param aResetText is true to keep the default target text (false to use the aSrc text)
 * @param aResetTextLayers is true to keep the default target layers setting
 * (false to use the aSrc setting)
 * @param aResetTextEffects is true to keep the default target text effects
 * (false to use the aSrc effect)
 * @param aUpdated is a refrence to a bool to keep trace of changes
 */
static void processTextItem( const PCB_TEXT& aSrc, PCB_TEXT& aDest,
                             bool aResetText, bool aResetTextLayers, bool aResetTextEffects,
                             bool aResetTextPositions, bool* aUpdated )
{
    if( aResetText )
        *aUpdated |= aSrc.GetText() != aDest.GetText();
    else
        aDest.SetText( aSrc.GetText() );

    if( aResetTextLayers )
    {
        *aUpdated |= aSrc.GetLayer() != aDest.GetLayer();
        *aUpdated |= aSrc.IsVisible() != aDest.IsVisible();
    }
    else
    {
        aDest.SetLayer( aSrc.GetLayer() );
        aDest.SetVisible( aSrc.IsVisible() );
    }

    VECTOR2I origPos = aDest.GetFPRelativePosition();

    if( aResetTextEffects )
    {
        *aUpdated |= aSrc.GetHorizJustify() != aDest.GetHorizJustify();
        *aUpdated |= aSrc.GetVertJustify() != aDest.GetVertJustify();
        *aUpdated |= aSrc.GetTextSize() != aDest.GetTextSize();
        *aUpdated |= aSrc.GetTextThickness() != aDest.GetTextThickness();
        *aUpdated |= aSrc.GetTextAngle() != aDest.GetTextAngle();
    }
    else
    {
        aDest.SetAttributes( aSrc );
    }

    if( aResetTextPositions )
    {
        *aUpdated |= aSrc.GetFPRelativePosition() != origPos;
        aDest.SetFPRelativePosition( origPos );
    }
    else
    {
        aDest.SetFPRelativePosition( aSrc.GetFPRelativePosition() );
    }

    aDest.SetLocked( aSrc.IsLocked() );
    const_cast<KIID&>( aDest.m_Uuid ) = aSrc.m_Uuid;
}


template<typename T>
static std::vector<std::pair<T*, T*>> matchItemsBySimilarity( const std::vector<T*>& aExisting,
                                                              const std::vector<T*>& aNew )
{
    struct MATCH_CANDIDATE
    {
        T*      existing;
        T*      updated;
        double  score;
    };

    std::vector<MATCH_CANDIDATE> candidates;

    for( T* existing : aExisting )
    {
        for( T* updated : aNew )
        {
            if( existing->Type() != updated->Type() )
                continue;

            double similarity = existing->Similarity( *updated );

            if constexpr( std::is_same_v<T, PAD> )
            {
                if( existing->GetNumber() == updated->GetNumber() )
                    similarity += 2.0;
            }

            if( similarity <= 0.0 )
                continue;

            candidates.push_back( { existing, updated, similarity } );
        }
    }

    std::sort( candidates.begin(), candidates.end(),
               []( const MATCH_CANDIDATE& a, const MATCH_CANDIDATE& b )
               {
                   if( a.score != b.score )
                       return a.score > b.score;

                   if( a.existing != b.existing )
                       return a.existing < b.existing;

                   return a.updated < b.updated;
               } );

    std::vector<std::pair<T*, T*>> matches;
    matches.reserve( candidates.size() );

    std::unordered_set<T*> matchedExisting;
    std::unordered_set<T*> matchedNew;

    for( const MATCH_CANDIDATE& candidate : candidates )
    {
        if( matchedExisting.find( candidate.existing ) != matchedExisting.end() )
            continue;

        if( matchedNew.find( candidate.updated ) != matchedNew.end() )
            continue;

        matchedExisting.insert( candidate.existing );
        matchedNew.insert( candidate.updated );
        matches.emplace_back( candidate.existing, candidate.updated );
    }

    return matches;
}


void PCB_EDIT_FRAME::ExchangeFootprint( FOOTPRINT* aExisting, FOOTPRINT* aNew,
                                        BOARD_COMMIT& aCommit,
                                        bool deleteExtraTexts,
                                        bool resetTextLayers,
                                        bool resetTextEffects,
                                        bool resetTextPositions,
                                        bool resetTextContent,
                                        bool resetFabricationAttrs,
                                        bool resetClearanceOverrides,
                                        bool reset3DModels,
                                        bool* aUpdated )
{
    EDA_GROUP* parentGroup = aExisting->GetParentGroup();
    bool       dummyBool   = false;

    if( !aUpdated )
        aUpdated = &dummyBool;

    if( parentGroup )
    {
        aCommit.Modify( parentGroup->AsEdaItem(), nullptr, RECURSE_MODE::NO_RECURSE );
        parentGroup->RemoveItem( aExisting );
        parentGroup->AddItem( aNew );
    }

    aNew->SetParent( GetBoard() );

    PlaceFootprint( aNew, false, aExisting->GetPosition() );

    if( aNew->GetLayer() != aExisting->GetLayer() )
        aNew->Flip( aNew->GetPosition(), GetPcbNewSettings()->m_FlipDirection );

    if( aNew->GetOrientation() != aExisting->GetOrientation() )
        aNew->SetOrientation( aExisting->GetOrientation() );

    aNew->SetLocked( aExisting->IsLocked() );

    const_cast<KIID&>( aNew->m_Uuid ) = aExisting->m_Uuid;
    const_cast<KIID&>( aNew->Reference().m_Uuid ) = aExisting->Reference().m_Uuid;
    const_cast<KIID&>( aNew->Value().m_Uuid ) = aExisting->Value().m_Uuid;

    std::vector<PAD*> oldPads;
    oldPads.reserve( aExisting->Pads().size() );

    for( PAD* pad : aExisting->Pads() )
        oldPads.push_back( pad );

    std::vector<PAD*> newPads;
    newPads.reserve( aNew->Pads().size() );

    for( PAD* pad : aNew->Pads() )
        newPads.push_back( pad );

    auto padMatches = matchItemsBySimilarity<PAD>( oldPads, newPads );
    std::unordered_set<PAD*> matchedNewPads;

    for( const auto& match : padMatches )
    {
        PAD* oldPad = match.first;
        PAD* newPad = match.second;

        matchedNewPads.insert( newPad );
        const_cast<KIID&>( newPad->m_Uuid ) = oldPad->m_Uuid;
        newPad->SetLocalRatsnestVisible( oldPad->GetLocalRatsnestVisible() );
        newPad->SetPinFunction( oldPad->GetPinFunction() );
        newPad->SetPinType( oldPad->GetPinType() );

        if( newPad->IsOnCopperLayer() )
            newPad->SetNetCode( oldPad->GetNetCode() );
        else
            newPad->SetNetCode( NETINFO_LIST::UNCONNECTED );
    }

    for( PAD* newPad : aNew->Pads() )
    {
        if( matchedNewPads.find( newPad ) != matchedNewPads.end() )
            continue;

        const_cast<KIID&>( newPad->m_Uuid ) = KIID();
        newPad->SetNetCode( NETINFO_LIST::UNCONNECTED );
    }

    std::vector<BOARD_ITEM*> oldDrawings;
    oldDrawings.reserve( aExisting->GraphicalItems().size() );

    for( BOARD_ITEM* item : aExisting->GraphicalItems() )
        oldDrawings.push_back( item );

    std::vector<BOARD_ITEM*> newDrawings;
    newDrawings.reserve( aNew->GraphicalItems().size() );

    for( BOARD_ITEM* item : aNew->GraphicalItems() )
        newDrawings.push_back( item );

    auto drawingMatches = matchItemsBySimilarity<BOARD_ITEM>( oldDrawings, newDrawings );
    std::unordered_map<BOARD_ITEM*, BOARD_ITEM*> oldToNewDrawings;
    std::unordered_set<BOARD_ITEM*> matchedNewDrawings;

    for( const auto& match : drawingMatches )
    {
        BOARD_ITEM* oldItem = match.first;
        BOARD_ITEM* newItem = match.second;

        oldToNewDrawings[ oldItem ] = newItem;
        matchedNewDrawings.insert( newItem );
        const_cast<KIID&>( newItem->m_Uuid ) = oldItem->m_Uuid;
    }

    for( BOARD_ITEM* newItem : newDrawings )
    {
        if( matchedNewDrawings.find( newItem ) == matchedNewDrawings.end() )
            const_cast<KIID&>( newItem->m_Uuid ) = KIID();
    }

    std::vector<ZONE*> oldZones;
    oldZones.reserve( aExisting->Zones().size() );

    for( ZONE* zone : aExisting->Zones() )
        oldZones.push_back( zone );

    std::vector<ZONE*> newZones;
    newZones.reserve( aNew->Zones().size() );

    for( ZONE* zone : aNew->Zones() )
        newZones.push_back( zone );

    auto zoneMatches = matchItemsBySimilarity<ZONE>( oldZones, newZones );
    std::unordered_set<ZONE*> matchedNewZones;

    for( const auto& match : zoneMatches )
    {
        ZONE* oldZone = match.first;
        ZONE* newZone = match.second;

        matchedNewZones.insert( newZone );
        const_cast<KIID&>( newZone->m_Uuid ) = oldZone->m_Uuid;
    }

    for( ZONE* newZone : newZones )
    {
        if( matchedNewZones.find( newZone ) == matchedNewZones.end() )
            const_cast<KIID&>( newZone->m_Uuid ) = KIID();
    }

    std::vector<PCB_POINT*> oldPoints;
    oldPoints.reserve( aExisting->Points().size() );

    for( PCB_POINT* point : aExisting->Points() )
        oldPoints.push_back( point );

    std::vector<PCB_POINT*> newPoints;
    newPoints.reserve( aNew->Points().size() );

    for( PCB_POINT* point : aNew->Points() )
        newPoints.push_back( point );

    auto pointMatches = matchItemsBySimilarity<PCB_POINT>( oldPoints, newPoints );
    std::unordered_set<PCB_POINT*> matchedNewPoints;

    for( const auto& match : pointMatches )
    {
        PCB_POINT* oldPoint = match.first;
        PCB_POINT* newPoint = match.second;

        matchedNewPoints.insert( newPoint );
        const_cast<KIID&>( newPoint->m_Uuid ) = oldPoint->m_Uuid;
    }

    for( PCB_POINT* newPoint : newPoints )
    {
        if( matchedNewPoints.find( newPoint ) == matchedNewPoints.end() )
            const_cast<KIID&>( newPoint->m_Uuid ) = KIID();
    }

    std::vector<PCB_GROUP*> oldGroups;
    oldGroups.reserve( aExisting->Groups().size() );

    for( PCB_GROUP* group : aExisting->Groups() )
        oldGroups.push_back( group );

    std::vector<PCB_GROUP*> newGroups;
    newGroups.reserve( aNew->Groups().size() );

    for( PCB_GROUP* group : aNew->Groups() )
        newGroups.push_back( group );

    auto groupMatches = matchItemsBySimilarity<PCB_GROUP>( oldGroups, newGroups );
    std::unordered_set<PCB_GROUP*> matchedNewGroups;

    for( const auto& match : groupMatches )
    {
        PCB_GROUP* oldGroup = match.first;
        PCB_GROUP* newGroup = match.second;

        matchedNewGroups.insert( newGroup );
        const_cast<KIID&>( newGroup->m_Uuid ) = oldGroup->m_Uuid;
    }

    for( PCB_GROUP* newGroup : newGroups )
    {
        if( matchedNewGroups.find( newGroup ) == matchedNewGroups.end() )
            const_cast<KIID&>( newGroup->m_Uuid ) = KIID();
    }

    std::vector<PCB_FIELD*> oldFieldsVec;
    std::vector<PCB_FIELD*> newFieldsVec;

    oldFieldsVec.reserve( aExisting->GetFields().size() );

    for( PCB_FIELD* field : aExisting->GetFields() )
    {
        wxCHECK2( field, continue );

        if( field->IsReference() || field->IsValue() )
            continue;

        oldFieldsVec.push_back( field );
    }

    newFieldsVec.reserve( aNew->GetFields().size() );

    for( PCB_FIELD* field : aNew->GetFields() )
    {
        wxCHECK2( field, continue );

        if( field->IsReference() || field->IsValue() )
            continue;

        newFieldsVec.push_back( field );
    }

    auto fieldMatches = matchItemsBySimilarity<PCB_FIELD>( oldFieldsVec, newFieldsVec );
    std::unordered_map<PCB_FIELD*, PCB_FIELD*> oldToNewFields;
    std::unordered_set<PCB_FIELD*> matchedNewFields;

    for( const auto& match : fieldMatches )
    {
        PCB_FIELD* oldField = match.first;
        PCB_FIELD* newField = match.second;

        oldToNewFields[ oldField ] = newField;
        matchedNewFields.insert( newField );
        const_cast<KIID&>( newField->m_Uuid ) = oldField->m_Uuid;
    }

    for( PCB_FIELD* newField : newFieldsVec )
    {
        if( matchedNewFields.find( newField ) == matchedNewFields.end() )
            const_cast<KIID&>( newField->m_Uuid ) = KIID();
    }

    std::unordered_map<PCB_TEXT*, PCB_TEXT*> oldToNewTexts;

    for( const auto& match : drawingMatches )
    {
        PCB_TEXT* oldText = dynamic_cast<PCB_TEXT*>( match.first );
        PCB_TEXT* newText = dynamic_cast<PCB_TEXT*>( match.second );

        if( oldText && newText )
            oldToNewTexts[ oldText ] = newText;
    }

    std::set<PCB_TEXT*> handledTextItems;

    for( BOARD_ITEM* oldItem : aExisting->GraphicalItems() )
    {
        PCB_TEXT* oldTextItem = dynamic_cast<PCB_TEXT*>( oldItem );

        if( oldTextItem )
        {
            // Dimensions have PCB_TEXT base but are not treated like texts in the updater
            if( dynamic_cast<PCB_DIMENSION_BASE*>( oldTextItem ) )
                continue;

            PCB_TEXT* newTextItem = nullptr;

            auto textMatchIt = oldToNewTexts.find( oldTextItem );

            if( textMatchIt != oldToNewTexts.end() )
                newTextItem = textMatchIt->second;

            if( newTextItem )
            {
                handledTextItems.insert( newTextItem );
                processTextItem( *oldTextItem, *newTextItem, resetTextContent, resetTextLayers,
                                 resetTextEffects, resetTextPositions, aUpdated );
            }
            else if( deleteExtraTexts )
            {
                *aUpdated = true;
            }
            else
            {
                newTextItem = static_cast<PCB_TEXT*>( oldTextItem->Clone() );
                handledTextItems.insert( newTextItem );
                aNew->Add( newTextItem );
            }
        }
    }

    // Check for any newly-added text items and set the update flag as appropriate
    for( BOARD_ITEM* newItem : aNew->GraphicalItems() )
    {
        PCB_TEXT* newTextItem = dynamic_cast<PCB_TEXT*>( newItem );

        if( newTextItem )
        {
            // Dimensions have PCB_TEXT base but are not treated like texts in the updater
            if( dynamic_cast<PCB_DIMENSION_BASE*>( newTextItem ) )
                continue;

            if( !handledTextItems.contains( newTextItem ) )
            {
                *aUpdated = true;
                break;
            }
        }
    }

    // Copy reference. The initial text is always used, never resetted
    processTextItem( aExisting->Reference(), aNew->Reference(), false, resetTextLayers,
                     resetTextEffects, resetTextPositions, aUpdated );

    // Copy value
    processTextItem( aExisting->Value(), aNew->Value(),
                     // reset value text only when it is a proxy for the footprint ID
                     // (cf replacing value "MountingHole-2.5mm" with "MountingHole-4.0mm")
                     aExisting->GetValue() == aExisting->GetFPID().GetLibItemName().wx_str(),
                     resetTextLayers, resetTextEffects, resetTextPositions, aUpdated );

    std::set<PCB_FIELD*> handledFields;

    // Copy fields in accordance with the reset* flags
    for( PCB_FIELD* oldField : aExisting->GetFields() )
    {
        wxCHECK2( oldField, continue );

        // Reference and value are already handled
        if( oldField->IsReference() || oldField->IsValue() )
            continue;

        PCB_FIELD* newField = nullptr;

        auto fieldMatchIt = oldToNewFields.find( oldField );

        if( fieldMatchIt != oldToNewFields.end() )
            newField = fieldMatchIt->second;

        if( newField )
        {
            handledFields.insert( newField );
            processTextItem( *oldField, *newField, resetTextContent, resetTextLayers,
                             resetTextEffects, resetTextPositions, aUpdated );
        }
        else if( deleteExtraTexts )
        {
            *aUpdated = true;
        }
        else
        {
            newField = new PCB_FIELD( *oldField );
            handledFields.insert( newField );
            aNew->Add( newField );
        }
    }

    // Check for any newly-added fields and set the update flag as appropriate
    for( PCB_FIELD* newField : aNew->GetFields() )
    {
        wxCHECK2( newField, continue );

        // Reference and value are already handled
        if( newField->IsReference() || newField->IsValue() )
            continue;

        if( !handledFields.contains( newField ) )
        {
            *aUpdated = true;
            break;
        }
    }

    if( resetFabricationAttrs )
    {
        // We've replaced the existing footprint with the library one, so the fabrication attrs
        // are already reset.  Just set the aUpdated flag if appropriate.
        if( aNew->GetAttributes() != aExisting->GetAttributes() )
            *aUpdated = true;
    }
    else
    {
        aNew->SetAttributes( aExisting->GetAttributes() );
    }

    if( resetClearanceOverrides )
    {
        if( aExisting->AllowSolderMaskBridges() != aNew->AllowSolderMaskBridges() )
            *aUpdated = true;

        if( ( aExisting->GetLocalClearance() != aNew->GetLocalClearance() )
                || ( aExisting->GetLocalSolderMaskMargin() != aNew->GetLocalSolderMaskMargin() )
                || ( aExisting->GetLocalSolderPasteMargin() != aNew->GetLocalSolderPasteMargin() )
                || ( aExisting->GetLocalSolderPasteMarginRatio() != aNew->GetLocalSolderPasteMarginRatio() )
                || ( aExisting->GetLocalZoneConnection() != aNew->GetLocalZoneConnection() ) )
        {
            *aUpdated = true;
        }
    }
    else
    {
        aNew->SetLocalClearance( aExisting->GetLocalClearance() );
        aNew->SetLocalSolderMaskMargin( aExisting->GetLocalSolderMaskMargin() );
        aNew->SetLocalSolderPasteMargin( aExisting->GetLocalSolderPasteMargin() );
        aNew->SetLocalSolderPasteMarginRatio( aExisting->GetLocalSolderPasteMarginRatio() );
        aNew->SetLocalZoneConnection( aExisting->GetLocalZoneConnection() );
        aNew->SetAllowSolderMaskBridges( aExisting->AllowSolderMaskBridges() );
    }

    if( reset3DModels )
    {
        // We've replaced the existing footprint with the library one, so the 3D models are
        // already reset.  Just set the aUpdated flag if appropriate.
        if( aNew->Models().size() != aExisting->Models().size() )
        {
            *aUpdated = true;
        }
        else
        {
            for( size_t ii = 0; ii < aNew->Models().size(); ++ii )
            {
                if( aNew->Models()[ii] != aExisting->Models()[ii] )
                {
                    *aUpdated = true;
                    break;
                }
            }
        }
    }
    else
    {
        aNew->Models() = aExisting->Models();  // Linked list of 3D models.
    }

    // Updating other parameters
    aNew->SetPath( aExisting->GetPath() );
    aNew->SetSheetfile( aExisting->GetSheetfile() );
    aNew->SetSheetname( aExisting->GetSheetname() );
    aNew->SetFilters( aExisting->GetFilters() );
    aNew->SetStaticComponentClass( aExisting->GetComponentClass() );

    if( *aUpdated == false )
    {
        // Check pad shapes, graphics, zones, etc. for changes
        if( aNew->FootprintNeedsUpdate( aExisting, BOARD_ITEM::COMPARE_FLAGS::INSTANCE_TO_INSTANCE ) )
            *aUpdated = true;
    }

    aCommit.Remove( aExisting );
    aCommit.Add( aNew );

    aNew->ClearFlags();
}


void PCB_EDIT_FRAME::CommonSettingsChanged( int aFlags )
{
    PCB_BASE_EDIT_FRAME::CommonSettingsChanged( aFlags );

    PrepareLayerIndicator();

    GetAppearancePanel()->OnColorThemeChanged();

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
    if( aFlags & ENVVARS_CHANGED )
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

    // Register autosave history saver for the board.
    // Saver exports the in-memory BOARD into the history mirror preserving the original
    // relative path and file name (reparented under .history) without touching dirty flags.
    if( GetBoard() )
    {
        Kiway().LocalHistory().RegisterSaver( GetBoard(),
                [this]( const wxString& aProjectPath, std::vector<wxString>& aFiles )
                {
                    GetBoard()->SaveToHistory( aProjectPath, aFiles );
                } );
    }
}


bool PCB_EDIT_FRAME::CanAcceptApiCommands()
{
    TOOL_BASE* currentTool = GetToolManager()->GetCurrentTool();

    // When a single item that can be point-edited is selected, the point editor
    // tool will be active instead of the selection tool.  It blocks undo/redo
    // while the user is actually dragging points around, though, so we can use
    // this as an initial check to prevent API actions when points are being edited.
    if( UndoRedoBlocked() )
        return false;

    // Don't allow any API use while the user is using a tool that could
    // modify the model in the middle of the message stream
    if( currentTool != GetToolManager()->GetTool<PCB_SELECTION_TOOL>() &&
        currentTool != GetToolManager()->GetTool<PCB_POINT_EDITOR>() )
    {
        return false;
    }

    ZONE_FILLER_TOOL* zoneFillerTool = m_toolManager->GetTool<ZONE_FILLER_TOOL>();

    if( zoneFillerTool->IsBusy() )
        return false;

    ROUTER_TOOL* routerTool = m_toolManager->GetTool<ROUTER_TOOL>();

    if( routerTool && routerTool->RoutingInProgress() )
        return false;

    return EDA_BASE_FRAME::CanAcceptApiCommands();
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
    return m_auimgr.GetPane( PropertiesPaneName() ).IsShown();
}


bool PCB_EDIT_FRAME::NetInspectorShown()
{
    return m_auimgr.GetPane( NetInspectorPanelName() ).IsShown();
}


void PCB_EDIT_FRAME::onSize( wxSizeEvent& aEvent )
{
    if( IsShownOnScreen() )
    {
        // We only need this until the frame is done resizing and the final client size is
        // established.
        Unbind( wxEVT_SIZE, &PCB_EDIT_FRAME::onSize, this );
        GetToolManager()->RunAction( ACTIONS::zoomFitScreen );
    }

    // Skip() is called in the base class.
    EDA_DRAW_FRAME::OnSize( aEvent );
}


DIALOG_BOOK_REPORTER* PCB_EDIT_FRAME::GetInspectDrcErrorDialog()
{
    if( !m_inspectDrcErrorDlg )
    {
        m_inspectDrcErrorDlg = new DIALOG_BOOK_REPORTER( this, INSPECT_DRC_ERROR_DIALOG_NAME,
                                                         _( "Violation Report" ) );
    }

    return m_inspectDrcErrorDlg;
}


DIALOG_BOOK_REPORTER* PCB_EDIT_FRAME::GetInspectClearanceDialog()
{
    if( !m_inspectClearanceDlg )
    {
        m_inspectClearanceDlg = new DIALOG_BOOK_REPORTER( this, INSPECT_CLEARANCE_DIALOG_NAME,
                                                          _( "Clearance Report" ) );
    }

    return m_inspectClearanceDlg;
}


DIALOG_BOOK_REPORTER* PCB_EDIT_FRAME::GetInspectConstraintsDialog()
{
    if( !m_inspectConstraintsDlg )
    {
        m_inspectConstraintsDlg = new DIALOG_BOOK_REPORTER( this, INSPECT_CONSTRAINTS_DIALOG_NAME,
                                                            _( "Constraints Report" ) );
    }

    return m_inspectConstraintsDlg;
}


DIALOG_BOOK_REPORTER* PCB_EDIT_FRAME::GetFootprintDiffDialog()
{
    if( !m_footprintDiffDlg )
    {
        m_footprintDiffDlg = new DIALOG_BOOK_REPORTER( this, FOOTPRINT_DIFF_DIALOG_NAME,
                                                       _( "Compare Footprint with Library" ) );

        m_footprintDiffDlg->m_sdbSizerApply->SetLabel( _( "Update Footprint from Library..." ) );
        m_footprintDiffDlg->m_sdbSizerApply->PostSizeEventToParent();
        m_footprintDiffDlg->m_sdbSizerApply->Show();
    }

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
    else if( m_footprintDiffDlg && aEvent.GetString() == FOOTPRINT_DIFF_DIALOG_NAME )
    {
        if( aEvent.GetId() == wxID_APPLY )
        {
            KIID fpUUID = m_footprintDiffDlg->GetUserItemID();

            CallAfter(
                    [this, fpUUID]()
                    {
                        BOARD_ITEM* item = m_pcb->ResolveItem( fpUUID );

                        if( FOOTPRINT* footprint = dynamic_cast<FOOTPRINT*>( item ) )
                        {
                            m_toolManager->RunAction<EDA_ITEM*>( ACTIONS::selectItem, footprint );

                            DIALOG_EXCHANGE_FOOTPRINTS dialog( this, footprint, true, true );
                            dialog.ShowQuasiModal();
                        }
                    } );
        }

        m_footprintDiffDlg->Destroy();
        m_footprintDiffDlg = nullptr;
    }
}


#ifdef KICAD_IPC_API
void PCB_EDIT_FRAME::onPluginAvailabilityChanged( wxCommandEvent& aEvt )
{
    wxLogTrace( traceApi, "PCB frame: EDA_EVT_PLUGIN_AVAILABILITY_CHANGED" );
    ReCreateHToolbar();
    aEvt.Skip();
}
#endif


void PCB_EDIT_FRAME::SwitchLayer( PCB_LAYER_ID layer )
{
    PCB_LAYER_ID curLayer = GetActiveLayer();
    const PCB_DISPLAY_OPTIONS& displ_opts = GetDisplayOptions();

    // Check if the specified layer matches the present layer
    if( layer == curLayer )
        return;

    // Copper layers cannot be selected unconditionally; how many of those layers are currently
    // enabled needs to be checked.
    if( IsCopperLayer( layer ) )
    {
        if( layer > GetBoard()->GetCopperLayerStackMaxId() )
            return;
    }

    // Is yet more checking required? E.g. when the layer to be selected is a non-copper layer,
    // or when switching between a copper layer and a non-copper layer, or vice-versa?

    SetActiveLayer( layer );

    if( displ_opts.m_ContrastModeDisplay != HIGH_CONTRAST_MODE::NORMAL )
        GetCanvas()->Refresh();
}


void PCB_EDIT_FRAME::OnEditItemRequest( BOARD_ITEM* aItem )
{
    switch( aItem->Type() )
    {
    case PCB_REFERENCE_IMAGE_T:
        ShowReferenceImagePropertiesDialog( aItem );
        break;

    case PCB_BARCODE_T:
        ShowBarcodePropertiesDialog( static_cast<PCB_BARCODE*>( aItem ) );
        break;

    case PCB_FIELD_T:
    case PCB_TEXT_T:
        ShowTextPropertiesDialog( static_cast<PCB_TEXT*>( aItem ) );
        break;

    case PCB_TEXTBOX_T:
        ShowTextBoxPropertiesDialog( static_cast<PCB_TEXTBOX*>( aItem ) );
        break;

    case PCB_TABLE_T:
    {
        DIALOG_TABLE_PROPERTIES dlg( this, static_cast<PCB_TABLE*>( aItem ) );

        //QuasiModal required for Scintilla auto-complete
        dlg.ShowQuasiModal();
        break;
    }

    case PCB_PAD_T:
        ShowPadPropertiesDialog( static_cast<PAD*>( aItem ) );
        break;

    case PCB_FOOTPRINT_T:
        ShowFootprintPropertiesDialog( static_cast<FOOTPRINT*>( aItem ) );
        break;

    case PCB_TARGET_T:
        ShowTargetOptionsDialog( static_cast<PCB_TARGET*>( aItem ) );
        break;

    case PCB_DIM_ALIGNED_T:
    case PCB_DIM_CENTER_T:
    case PCB_DIM_RADIAL_T:
    case PCB_DIM_ORTHOGONAL_T:
    case PCB_DIM_LEADER_T:
    {
        DIALOG_DIMENSION_PROPERTIES dlg( this, static_cast<PCB_DIMENSION_BASE*>( aItem ) );

        dlg.ShowModal();
        break;
    }

    case PCB_SHAPE_T:
        ShowGraphicItemPropertiesDialog( static_cast<PCB_SHAPE*>( aItem ) );
        break;

    case PCB_ZONE_T:
        Edit_Zone_Params( static_cast<ZONE*>( aItem ) );
        break;

    case PCB_GROUP_T:
        m_toolManager->RunAction( ACTIONS::groupProperties,
                                  static_cast<EDA_GROUP*>( static_cast<PCB_GROUP*>( aItem ) ) );
        break;

    case PCB_GENERATOR_T:
        static_cast<PCB_GENERATOR*>( aItem )->ShowPropertiesDialog( this );
        break;

    case PCB_MARKER_T:
        m_toolManager->GetTool<DRC_TOOL>()->CrossProbe( static_cast<PCB_MARKER*>( aItem ) );
        break;

    case PCB_POINT_T:
        break;

    default:
        break;
    }
}


bool PCB_EDIT_FRAME::DoAutoSave()
{
    // For now we just delegate to the base implementation which commits any pending
    // local history snapshots.  If PCB-specific preconditions are later needed (e.g.
    // flushing zone fills or router state) they can be added here before calling the
    // base class method.
    return EDA_BASE_FRAME::doAutoSave();
}
