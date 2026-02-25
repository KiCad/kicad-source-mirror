/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <algorithm>
#include <api/api_handler_sch.h>
#include <api/api_server.h>
#include <base_units.h>
#include <bitmaps.h>
#include <confirm.h>
#include <connection_graph.h>
#include <dialogs/dialog_erc.h>
#include <dialogs/dialog_book_reporter.h>
#include <dialogs/dialog_symbol_fields_table.h>
#include <widgets/sch_design_block_pane.h>
#include <widgets/panel_remote_symbol.h>
#include <wx/srchctrl.h>
#include <mail_type.h>
#include <wx/clntdata.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/menu.h>
#include <local_history.h>
#include <eeschema_id.h>
#include <executable_names.h>
#include <gal/graphics_abstraction_layer.h>
#include <geometry/shape_segment.h>
#include <gestfich.h>
#include <dialogs/html_message_box.h>
#include <string_utils.h>
#include <kiface_base.h>
#include <kiplatform/app.h>
#include <kiplatform/ui.h>
#include <kiway.h>
#include <symbol_edit_frame.h>
#include <symbol_viewer_frame.h>
#include <pgm_base.h>
#include <core/profile.h>
#include <project/project_file.h>
#include <project/net_settings.h>
#include <python_scripting.h>
#include <sch_edit_frame.h>
#include <symbol_chooser_frame.h>
#include <sch_painter.h>
#include <sch_marker.h>
#include <sch_sheet_pin.h>
#include <sch_commit.h>
#include <sch_rule_area.h>
#include <settings/settings_manager.h>
#include <advanced_config.h>
#include <sim/simulator_frame.h>
#include <tool/action_manager.h>
#include <tool/action_toolbar.h>
#include <tool/common_control.h>
#include <tool/common_tools.h>
#include <tool/embed_tool.h>
#include <tool/picker_tool.h>
#include <tool/properties_tool.h>
#include <tool/selection.h>
#include <tool/tool_dispatcher.h>
#include <tool/tool_manager.h>
#include <tool/zoom_tool.h>
#include <tools/sch_actions.h>
#include <tools/sch_align_tool.h>
#include <tools/ee_grid_helper.h>
#include <tools/sch_inspection_tool.h>
#include <tools/sch_point_editor.h>
#include <tools/sch_design_block_control.h>
#include <sch_io/sch_io_mgr.h>
#include <sch_io/sch_io.h>
#include <tools/sch_drawing_tools.h>
#include <tools/sch_edit_tool.h>
#include <tools/sch_edit_table_tool.h>
#include <tools/sch_editor_conditions.h>
#include <tools/sch_editor_control.h>
#include <tools/sch_group_tool.h>
#include <tools/sch_line_wire_bus_tool.h>
#include <tools/sch_move_tool.h>
#include <tools/sch_navigate_tool.h>
#include <tools/sch_find_replace_tool.h>
#include <trace_helpers.h>
#include <unordered_set>
#include <view/view_controls.h>
#include <widgets/wx_infobar.h>
#include <widgets/hierarchy_pane.h>
#include <widgets/bitmap_button.h>
#include <widgets/sch_properties_panel.h>
#include <widgets/sch_search_pane.h>
#include <wildcards_and_files_ext.h>
#include <wx/cmdline.h>
#include <wx/app.h>
#include <wx/filedlg.h>
#include <wx/socket.h>
#include <wx/debug.h>
#include <widgets/panel_sch_selection_filter.h>
#include <widgets/wx_aui_utils.h>
#include <drawing_sheet/ds_proxy_view_item.h>
#include <project/project_local_settings.h>
#include <toolbars_sch_editor.h>
#include <wx/log.h>
#include <wx/choicdlg.h>
#include <wx/textdlg.h>
#include <wx/generic/treectlg.h>


#ifdef KICAD_IPC_API
#include <api/api_plugin_manager.h>
#include <api/api_utils.h>
#endif

#include <dialog_change_symbols.h>


#define DIFF_SYMBOLS_DIALOG_NAME wxT( "DiffSymbolsDialog" )


BEGIN_EVENT_TABLE( SCH_EDIT_FRAME, SCH_BASE_FRAME )
    EVT_SOCKET( ID_EDA_SOCKET_EVENT_SERV, EDA_DRAW_FRAME::OnSockRequestServer )
    EVT_SOCKET( ID_EDA_SOCKET_EVENT, EDA_DRAW_FRAME::OnSockRequest )

    EVT_SIZE( SCH_EDIT_FRAME::OnSize )

    EVT_MENU_RANGE( ID_FILE1, ID_FILEMAX, SCH_EDIT_FRAME::OnLoadFile )
    EVT_MENU( ID_FILE_LIST_CLEAR, SCH_EDIT_FRAME::OnClearFileHistory )

    EVT_MENU( ID_IMPORT_NON_KICAD_SCH, SCH_EDIT_FRAME::OnImportProject )

    EVT_MENU( wxID_EXIT, SCH_EDIT_FRAME::OnExit )
    EVT_MENU( wxID_CLOSE, SCH_EDIT_FRAME::OnExit )

    EVT_CHOICE( ID_ON_ZOOM_SELECT, SCH_EDIT_FRAME::OnSelectZoom )
    EVT_CHOICE( ID_ON_GRID_SELECT, SCH_EDIT_FRAME::OnSelectGrid )

    // Drop files event
    EVT_DROP_FILES( SCH_EDIT_FRAME::OnDropFiles )
END_EVENT_TABLE()


wxDEFINE_EVENT( EDA_EVT_SCHEMATIC_CHANGING, wxCommandEvent );
wxDEFINE_EVENT( EDA_EVT_SCHEMATIC_CHANGED, wxCommandEvent );


SCH_EDIT_FRAME::SCH_EDIT_FRAME( KIWAY* aKiway, wxWindow* aParent ) :
        SCH_BASE_FRAME( aKiway, aParent, FRAME_SCH, wxT( "Eeschema" ), wxDefaultPosition,
                        wxDefaultSize, KICAD_DEFAULT_DRAWFRAME_STYLE, SCH_EDIT_FRAME_NAME ),
        m_ercDialog( nullptr ),
        m_diffSymbolDialog( nullptr ),
        m_symbolFieldsTableDialog( nullptr ),
        m_netNavigator( nullptr ),
        m_netNavigatorFilter( nullptr ),
        m_netNavigatorFilterValue(),
        m_netNavigatorMenuNetName(),
        m_highlightedConnChanged( false ),
        m_designBlocksPane( nullptr ),
        m_remoteSymbolPane( nullptr ),
        m_currentVariantCtrl( nullptr )
{
    m_maximizeByDefault = true;
    m_schematic = new SCHEMATIC( &Prj() );
    m_schematic->SetSchematicHolder( this );
    CreateDefaultScreens();

    m_showBorderAndTitleBlock = true;   // true to show sheet references
    m_supportsAutoSave = true;
    m_syncingPcbToSchSelection = false;
    m_aboutTitle = _HKI( "KiCad Schematic Editor" );
    m_show_search = false;
    // Ensure timer has an owner before binding so it generates events.
    m_crossProbeFlashTimer.SetOwner( this );
    Bind( wxEVT_TIMER, &SCH_EDIT_FRAME::OnCrossProbeFlashTimer, this, m_crossProbeFlashTimer.GetId() );

    // Give an icon
    wxIcon icon;
    wxIconBundle icon_bundle;

    icon.CopyFromBitmap( KiBitmap( BITMAPS::icon_eeschema, 48 ) );
    icon_bundle.AddIcon( icon );
    icon.CopyFromBitmap( KiBitmap( BITMAPS::icon_eeschema, 128 ) );
    icon_bundle.AddIcon( icon );
    icon.CopyFromBitmap( KiBitmap( BITMAPS::icon_eeschema, 256 ) );
    icon_bundle.AddIcon( icon );
    icon.CopyFromBitmap( KiBitmap( BITMAPS::icon_eeschema_32 ) );
    icon_bundle.AddIcon( icon );
    icon.CopyFromBitmap( KiBitmap( BITMAPS::icon_eeschema_16 ) );
    icon_bundle.AddIcon( icon );

    SetIcons( icon_bundle );

    LoadSettings( eeconfig() );

    setupTools();
    setupUIConditions();
    ReCreateMenuBar();

    m_toolbarSettings = GetToolbarSettings<SCH_EDIT_TOOLBAR_SETTINGS>( "eeschema-toolbars" );
    configureToolbars();
    RecreateToolbars();

    // Ensure the "Line modes" toolbar group shows the current angle mode on startup
    if( GetToolManager() )
        GetToolManager()->RunAction( SCH_ACTIONS::angleSnapModeChanged );

#ifdef KICAD_IPC_API
    wxTheApp->Bind( EDA_EVT_PLUGIN_AVAILABILITY_CHANGED, &SCH_EDIT_FRAME::onPluginAvailabilityChanged, this );
#endif

    m_hierarchy = new HIERARCHY_PANE( this );

    // Initialize common print setup dialog settings.
    m_pageSetupData.GetPrintData().SetPrintMode( wxPRINT_MODE_PRINTER );
    m_pageSetupData.GetPrintData().SetQuality( wxPRINT_QUALITY_MEDIUM );
    m_pageSetupData.GetPrintData().SetBin( wxPRINTBIN_AUTO );
    m_pageSetupData.GetPrintData().SetNoCopies( 1 );

    m_searchPane = new SCH_SEARCH_PANE( this );
    m_propertiesPanel = new SCH_PROPERTIES_PANEL( this, this );
    m_remoteSymbolPane = new PANEL_REMOTE_SYMBOL( this );

    m_propertiesPanel->SetSplitterProportion( eeconfig()->m_AuiPanels.properties_splitter );

    m_selectionFilterPanel = new PANEL_SCH_SELECTION_FILTER( this );
    m_designBlocksPane = new SCH_DESIGN_BLOCK_PANE( this, nullptr, m_designBlockHistoryList );

    m_auimgr.SetManagedWindow( this );

    CreateInfoBar();

    // Fetch a COPY of the config as a lot of these initializations are going to overwrite our
    // data.
    EESCHEMA_SETTINGS::AUI_PANELS aui_cfg = eeconfig()->m_AuiPanels;
    EESCHEMA_SETTINGS::APPEARANCE appearance_cfg = eeconfig()->m_Appearance;

    // Rows; layers 4 - 6
    m_auimgr.AddPane( m_tbTopMain, EDA_PANE().HToolbar().Name( wxS( "TopMainToolbar" ) )
                      .Top().Layer( 6 ) );

    m_auimgr.AddPane( m_messagePanel, EDA_PANE().Messages().Name( wxS( "MsgPanel" ) )
                      .Bottom().Layer( 6 ) );

    // Columns; layers 1 - 3
    m_auimgr.AddPane( m_hierarchy, EDA_PANE().Palette().Name( SchematicHierarchyPaneName() )
                      .Caption( _( "Schematic Hierarchy" ) )
                      .Left().Layer( 3 ).Position( 1 )
                      .TopDockable( false )
                      .BottomDockable( false )
                      .CloseButton( true )
                      .MinSize( FromDIP( wxSize( 120, 60 ) ) )
                      .BestSize( FromDIP( wxSize( 200, 200 ) ) )
                      .FloatingSize( FromDIP( wxSize( 200, 200 ) ) )
                      .FloatingPosition( FromDIP( wxPoint( 50, 50 ) ) )
                      .Show( false ) );

    m_auimgr.AddPane( m_propertiesPanel, defaultPropertiesPaneInfo( this ) );
    m_auimgr.AddPane( m_selectionFilterPanel, defaultSchSelectionFilterPaneInfo( this ) );

    m_auimgr.AddPane( m_designBlocksPane, defaultDesignBlocksPaneInfo( this ) );
    m_auimgr.AddPane( m_remoteSymbolPane, defaultRemoteSymbolPaneInfo( this ) );

    m_auimgr.AddPane( createHighlightedNetNavigator(), defaultNetNavigatorPaneInfo() );

    m_auimgr.AddPane( m_tbLeft, EDA_PANE().VToolbar().Name( wxS( "LeftToolbar" ) )
                      .Left().Layer( 2 ) );

    m_auimgr.AddPane( m_tbRight, EDA_PANE().VToolbar().Name( wxS( "RightToolbar" ) )
                      .Right().Layer( 2 ) );

    // Center
    m_auimgr.AddPane( GetCanvas(), EDA_PANE().Canvas().Name( wxS( "DrawFrame" ) )
                      .Center() );

    m_auimgr.AddPane( m_searchPane, EDA_PANE()
                      .Name( SearchPaneName() )
                      .Bottom()
                      .Caption( _( "Search" ) )
                      .PaneBorder( false )
                      .MinSize( FromDIP( wxSize( 180, 60 ) ) )
                      .BestSize( FromDIP( wxSize( 180, 100 ) ) )
                      .FloatingSize( FromDIP( wxSize( 480, 200 ) ) )
                      .CloseButton( true )
                      .DestroyOnClose( false )
                      .Show( m_show_search ) );

    RestoreAuiLayout();
    FinishAUIInitialization();

    wxAuiPaneInfo& hierarchy_pane = m_auimgr.GetPane( SchematicHierarchyPaneName() );
    wxAuiPaneInfo& netNavigatorPane = m_auimgr.GetPane( NetNavigatorPaneName() );
    wxAuiPaneInfo& propertiesPane = m_auimgr.GetPane( PropertiesPaneName() );
    wxAuiPaneInfo& selectionFilterPane = m_auimgr.GetPane( wxS( "SelectionFilter" ) );
    wxAuiPaneInfo& designBlocksPane = m_auimgr.GetPane( DesignBlocksPaneName() );
    wxAuiPaneInfo& remoteSymbolPane = m_auimgr.GetPane( RemoteSymbolPaneName() );

    hierarchy_pane.Show( aui_cfg.show_schematic_hierarchy );
    netNavigatorPane.Show( aui_cfg.show_net_nav_panel );
    propertiesPane.Show( aui_cfg.show_properties );
    designBlocksPane.Show( aui_cfg.design_blocks_show );

    if( m_remoteSymbolPane && !m_remoteSymbolPane->HasDataSources() )
        remoteSymbolPane.Show( false );
    else
        remoteSymbolPane.Show( aui_cfg.remote_symbol_show );

    updateSelectionFilterVisbility();

    // The selection filter doesn't need to grow in the vertical direction when docked
    selectionFilterPane.dock_proportion = 0;

    if( aui_cfg.hierarchy_panel_float_width > 0 && aui_cfg.hierarchy_panel_float_height > 0 )
    {
        // Show at end, after positioning
        hierarchy_pane.FloatingSize( aui_cfg.hierarchy_panel_float_width,
                                     aui_cfg.hierarchy_panel_float_height );
    }

    if( aui_cfg.net_nav_panel_float_size.GetWidth() > 0
      && aui_cfg.net_nav_panel_float_size.GetHeight() > 0 )
    {
        netNavigatorPane.FloatingSize( aui_cfg.net_nav_panel_float_size );
        netNavigatorPane.FloatingPosition( aui_cfg.net_nav_panel_float_pos );
    }

    if( aui_cfg.properties_panel_width > 0 )
        SetAuiPaneSize( m_auimgr, propertiesPane, aui_cfg.properties_panel_width, -1 );

    if( aui_cfg.schematic_hierarchy_float )
        hierarchy_pane.Float();

    if( aui_cfg.search_panel_height > 0 && (   aui_cfg.search_panel_dock_direction == wxAUI_DOCK_TOP
                                            || aui_cfg.search_panel_dock_direction == wxAUI_DOCK_BOTTOM ) )
    {
        wxAuiPaneInfo& searchPane = m_auimgr.GetPane( SearchPaneName() );
        searchPane.Direction( aui_cfg.search_panel_dock_direction );
        SetAuiPaneSize( m_auimgr, searchPane, -1, aui_cfg.search_panel_height );
    }

    else if( aui_cfg.search_panel_width > 0 && (   aui_cfg.search_panel_dock_direction == wxAUI_DOCK_LEFT
                                                || aui_cfg.search_panel_dock_direction == wxAUI_DOCK_RIGHT ) )
    {
        wxAuiPaneInfo& searchPane = m_auimgr.GetPane( SearchPaneName() );
        searchPane.Direction( aui_cfg.search_panel_dock_direction );
        SetAuiPaneSize( m_auimgr, searchPane, aui_cfg.search_panel_width, -1 );
    }

    if( aui_cfg.float_net_nav_panel )
        netNavigatorPane.Float();

    if( aui_cfg.design_blocks_show )
        SetAuiPaneSize( m_auimgr, designBlocksPane, aui_cfg.design_blocks_panel_docked_width, -1 );

    if( aui_cfg.remote_symbol_show )
        SetAuiPaneSize( m_auimgr, remoteSymbolPane, aui_cfg.remote_symbol_panel_docked_width, -1 );

    if( aui_cfg.hierarchy_panel_docked_width > 0 )
    {
        // If the net navigator is not show, let the hierarchy navigator take all of the vertical
        // space.
        if( !aui_cfg.show_net_nav_panel )
        {
            SetAuiPaneSize( m_auimgr, hierarchy_pane, aui_cfg.hierarchy_panel_docked_width, -1 );
        }
        else
        {
            SetAuiPaneSize( m_auimgr, hierarchy_pane,
                            aui_cfg.hierarchy_panel_docked_width,
                            aui_cfg.hierarchy_panel_docked_height );

            SetAuiPaneSize( m_auimgr, netNavigatorPane,
                            aui_cfg.net_nav_panel_docked_size.GetWidth(),
                            aui_cfg.net_nav_panel_docked_size.GetHeight() );
        }

        // wxAUI hack: force width by setting MinSize() and then Fixed()
        // thanks to ZenJu https://github.com/wxWidgets/wxWidgets/issues/13180
        hierarchy_pane.MinSize( aui_cfg.hierarchy_panel_docked_width, 60 );
        hierarchy_pane.Fixed();
        netNavigatorPane.MinSize( aui_cfg.net_nav_panel_docked_size.GetWidth(), 60 );
        netNavigatorPane.Fixed();
        m_auimgr.Update();

        // now make it resizable again
        hierarchy_pane.Resizable();
        netNavigatorPane.Resizable();
        m_auimgr.Update();

        // Note: DO NOT call m_auimgr.Update() anywhere after this; it will nuke the size
        // back to minimum.
        hierarchy_pane.MinSize( FromDIP( wxSize( 120, 60 ) ) );
        netNavigatorPane.MinSize( FromDIP( wxSize( 120, 60 ) ) );
    }
    else
    {
        m_auimgr.Update();
    }

    resolveCanvasType();
    SwitchCanvas( m_canvasType );

    GetCanvas()->GetGAL()->SetAxesEnabled( false );

    KIGFX::SCH_VIEW* view = GetCanvas()->GetView();
    static_cast<KIGFX::SCH_PAINTER*>( view->GetPainter() )->SetSchematic( m_schematic );

    LoadProjectSettings();
    LoadDrawingSheet();

    view->SetLayerVisible( LAYER_ERC_ERR, appearance_cfg.show_erc_errors );
    view->SetLayerVisible( LAYER_ERC_WARN, appearance_cfg.show_erc_warnings );
    view->SetLayerVisible( LAYER_ERC_EXCLUSION, appearance_cfg.show_erc_exclusions );
    view->SetLayerVisible( LAYER_OP_VOLTAGES, appearance_cfg.show_op_voltages );
    view->SetLayerVisible( LAYER_OP_CURRENTS, appearance_cfg.show_op_currents );

    initScreenZoom();

    m_hierarchy->Bind( wxEVT_SIZE, &SCH_EDIT_FRAME::OnResizeHierarchyNavigator, this );
    m_netNavigator->Bind( wxEVT_TREE_SEL_CHANGING, &SCH_EDIT_FRAME::onNetNavigatorSelChanging, this );
    m_netNavigator->Bind( wxEVT_TREE_SEL_CHANGED, &SCH_EDIT_FRAME::onNetNavigatorSelection, this );
    m_netNavigator->Bind( wxEVT_SIZE, &SCH_EDIT_FRAME::onResizeNetNavigator, this );

    // This is used temporarily to fix a client size issue on GTK that causes zoom to fit
    // to calculate the wrong zoom size.  See SCH_EDIT_FRAME::onSize().
    Bind( wxEVT_SIZE, &SCH_EDIT_FRAME::onSize, this );

    setupUnits( eeconfig() );

    // Net list generator
    DefaultExecFlags();

    updateTitle();
    m_toolManager->GetTool<SCH_NAVIGATE_TOOL>()->ResetHistory();

#ifdef KICAD_IPC_API
    m_apiHandler = std::make_unique<API_HANDLER_SCH>( this );
    Pgm().GetApiServer().RegisterHandler( m_apiHandler.get() );
#endif

    // Default shutdown reason until a file is loaded
    KIPLATFORM::APP::SetShutdownBlockReason( this, _( "New schematic file is unsaved" ) );

    // Init for dropping files
    m_acceptedExts.emplace( FILEEXT::KiCadSchematicFileExtension, &SCH_ACTIONS::ddAppendFile );
    m_acceptedExts.emplace( FILEEXT::PngFileExtension, &SCH_ACTIONS::ddAddImage );
    m_acceptedExts.emplace( FILEEXT::JpegFileExtension, &SCH_ACTIONS::ddAddImage );
    m_acceptedExts.emplace( wxS( "jpeg" ), &SCH_ACTIONS::ddAddImage );
    m_acceptedExts.emplace( wxS( "dxf" ), &SCH_ACTIONS::ddImportGraphics );
    m_acceptedExts.emplace( FILEEXT::SVGFileExtension, &SCH_ACTIONS::ddImportGraphics );
    DragAcceptFiles( true );

    // Ensure the window is on top
    Raise();

    // Now that all sizes are fixed, set the initial hierarchy_pane floating position to the
    // top-left corner of the canvas
    wxPoint canvas_pos = GetCanvas()->GetScreenPosition();
    hierarchy_pane.FloatingPosition( canvas_pos.x + 10, canvas_pos.y + 10 );

    Bind( wxEVT_CHOICE, &SCH_EDIT_FRAME::onVariantSelected, this );
    Bind( EDA_EVT_CLOSE_DIALOG_BOOK_REPORTER, &SCH_EDIT_FRAME::onCloseSymbolDiffDialog, this );
    Bind( EDA_EVT_CLOSE_ERC_DIALOG, &SCH_EDIT_FRAME::onCloseErcDialog, this );
    Bind( EDA_EVT_CLOSE_DIALOG_SYMBOL_FIELDS_TABLE, &SCH_EDIT_FRAME::onCloseSymbolFieldsTableDialog, this );
}

void SCH_EDIT_FRAME::StartCrossProbeFlash( const std::vector<SCH_ITEM*>& aItems )
{
    if( !eeconfig()->m_CrossProbing.flash_selection )
    {
        wxLogTrace( traceCrossProbeFlash, "StartCrossProbeFlash: aborted (setting disabled) items=%zu", aItems.size() );
        return;
    }

    if( aItems.empty() )
    {
        wxLogTrace( traceCrossProbeFlash, "StartCrossProbeFlash: aborted (no items)" );
        return;
    }

    if( m_crossProbeFlashing )
    {
        wxLogTrace( traceCrossProbeFlash, "StartCrossProbeFlash: restarting existing flash (phase=%d)",
                    m_crossProbeFlashPhase );
        m_crossProbeFlashTimer.Stop();
    }

    wxLogTrace( traceCrossProbeFlash, "StartCrossProbeFlash: starting with %zu items", aItems.size() );
    m_crossProbeFlashItems.clear();

    for( SCH_ITEM* it : aItems )
        m_crossProbeFlashItems.push_back( it->m_Uuid );

    m_crossProbeFlashPhase = 0;
    m_crossProbeFlashing = true;

    if( !m_crossProbeFlashTimer.GetOwner() )
        m_crossProbeFlashTimer.SetOwner( this );

    bool started = m_crossProbeFlashTimer.Start( 500, wxTIMER_CONTINUOUS );
    wxLogTrace( traceCrossProbeFlash, "StartCrossProbeFlash: timer start=%d id=%d", (int) started,
                m_crossProbeFlashTimer.GetId() );
}


void SCH_EDIT_FRAME::OnCrossProbeFlashTimer( wxTimerEvent& aEvent )
{
    wxLogTrace( traceCrossProbeFlash, "Timer(SCH) fired: phase=%d running=%d items=%zu", m_crossProbeFlashPhase,
                (int) m_crossProbeFlashing, m_crossProbeFlashItems.size() );

    if( !m_crossProbeFlashing )
    {
        wxLogTrace( traceCrossProbeFlash, "Timer fired but not flashing (ignored)" );
        return;
    }

    SCH_SELECTION_TOOL* selTool = GetToolManager()->GetTool<SCH_SELECTION_TOOL>();

    if( !selTool )
        return;

    bool prevGuard = m_syncingPcbToSchSelection;
    m_syncingPcbToSchSelection = true;

    if( m_crossProbeFlashPhase % 2 == 0 )
    {
        selTool->ClearSelection( true );
        wxLogTrace( traceCrossProbeFlash, "Phase %d: cleared selection", m_crossProbeFlashPhase );
    }
    else
    {
        for( const KIID& id : m_crossProbeFlashItems )
        {
            if( SCH_ITEM* item = Schematic().ResolveItem( id, nullptr, true ) )
                selTool->AddItemToSel( item, true );
        }

        wxLogTrace( traceCrossProbeFlash, "Phase %d: restored %zu items", m_crossProbeFlashPhase,
                    m_crossProbeFlashItems.size() );
    }

    if( GetCanvas() )
    {
        GetCanvas()->ForceRefresh();
        wxLogTrace( traceCrossProbeFlash, "Phase %d: forced canvas refresh", m_crossProbeFlashPhase );
    }

    m_syncingPcbToSchSelection = prevGuard;
    m_crossProbeFlashPhase++;

    if( m_crossProbeFlashPhase > 6 )
    {
        for( const KIID& id : m_crossProbeFlashItems )
        {
            if( SCH_ITEM* item = Schematic().ResolveItem( id, nullptr, true ) )
                selTool->AddItemToSel( item, true );
        }

        m_crossProbeFlashing = false;
        m_crossProbeFlashTimer.Stop();
        wxLogTrace( traceCrossProbeFlash, "Flashing complete. Final selection size=%zu",
                    m_crossProbeFlashItems.size() );
    }
}


SCH_EDIT_FRAME::~SCH_EDIT_FRAME()
{
    m_hierarchy->Unbind( wxEVT_SIZE, &SCH_EDIT_FRAME::OnResizeHierarchyNavigator, this );

    // Ensure m_canvasType is up to date, to save it in config
    m_canvasType = GetCanvas()->GetBackend();

    SetScreen( nullptr );

    if( m_schematic )
        m_schematic->RemoveAllListeners();

    // Delete all items not in draw list before deleting schematic
    // to avoid dangling pointers stored in these items
    ClearUndoRedoList();
    ClearRepeatItemsList();

    delete m_schematic;
    m_schematic = nullptr;

    // Close the project if we are standalone, so it gets cleaned up properly
    if( Kiface().IsSingle() )
    {
        try
        {
            GetSettingsManager()->UnloadProject( &Prj(), false );
        }
        catch( const std::runtime_error& e )
        {
            wxFAIL_MSG( wxString::Format( wxT( "Settings exception occurred: %s" ), e.what() ) );
        }
    }

    // We passed ownership of these to wxAuiManager.
    // delete m_hierarchy;
    // delete m_selectionFilterPanel;
}


void SCH_EDIT_FRAME::OnResizeHierarchyNavigator( wxSizeEvent& aEvent )
{
    aEvent.Skip();

     // 1st Call: Handle the size update during the first resize event.
    CaptureHierarchyPaneSize();

    // Defer the second size capture
    CallAfter( [this]()
               {
                   CaptureHierarchyPaneSize();
               } );
}


void SCH_EDIT_FRAME::CaptureHierarchyPaneSize()
{
    // Called when resizing the Hierarchy Navigator panel
    // Store the current pane size
    // It allows to retrieve the last defined pane size when switching between
    // docked and floating pane state
    // Note: *DO NOT* call m_auimgr.Update() here: it crashes KiCad at least on Windows

    EESCHEMA_SETTINGS* cfg = dynamic_cast<EESCHEMA_SETTINGS*>( Kiface().KifaceSettings() );
    wxAuiPaneInfo&     hierarchy_pane = m_auimgr.GetPane( SchematicHierarchyPaneName() );

    if( cfg && m_hierarchy->IsShownOnScreen() )
    {
        cfg->m_AuiPanels.hierarchy_panel_float_width  = hierarchy_pane.floating_size.x;
        cfg->m_AuiPanels.hierarchy_panel_float_height = hierarchy_pane.floating_size.y;

        // initialize hierarchy_panel_docked_width and best size only if the hierarchy_pane
        // width is > 0 (i.e. if its size is already set and has meaning)
        // if it is floating, its size is not initialized (only floating_size is initialized)
        // initializing hierarchy_pane.best_size is useful when switching to float pane and
        // after switching to the docked pane, to retrieve the last docked pane width
        if( hierarchy_pane.rect.width > 50 )    // 50 is a good margin
        {
            cfg->m_AuiPanels.hierarchy_panel_docked_width = hierarchy_pane.rect.width;
            hierarchy_pane.best_size.x = hierarchy_pane.rect.width;
        }
    }
}


void SCH_EDIT_FRAME::setupTools()
{
    // Create the manager and dispatcher & route draw panel events to the dispatcher
    m_toolManager = new TOOL_MANAGER;
    m_toolManager->SetEnvironment( &Schematic(), GetCanvas()->GetView(),
                                   GetCanvas()->GetViewControls(), config(), this );
    m_actions = new SCH_ACTIONS();
    m_toolDispatcher = new TOOL_DISPATCHER( m_toolManager );

    // Register tools
    m_toolManager->RegisterTool( new COMMON_CONTROL );
    m_toolManager->RegisterTool( new COMMON_TOOLS );
    m_toolManager->RegisterTool( new ZOOM_TOOL );
    m_toolManager->RegisterTool( new SCH_SELECTION_TOOL );
    m_toolManager->RegisterTool( new PICKER_TOOL );
    m_toolManager->RegisterTool( new SCH_DRAWING_TOOLS );
    m_toolManager->RegisterTool( new SCH_LINE_WIRE_BUS_TOOL );
    m_toolManager->RegisterTool( new SCH_MOVE_TOOL );
    m_toolManager->RegisterTool( new SCH_ALIGN_TOOL );
    m_toolManager->RegisterTool( new SCH_EDIT_TOOL );
    m_toolManager->RegisterTool( new SCH_EDIT_TABLE_TOOL );
    m_toolManager->RegisterTool( new SCH_GROUP_TOOL );
    m_toolManager->RegisterTool( new SCH_INSPECTION_TOOL );
    m_toolManager->RegisterTool( new SCH_DESIGN_BLOCK_CONTROL );
    m_toolManager->RegisterTool( new SCH_EDITOR_CONTROL );
    m_toolManager->RegisterTool( new SCH_FIND_REPLACE_TOOL );
    m_toolManager->RegisterTool( new SCH_POINT_EDITOR );
    m_toolManager->RegisterTool( new SCH_NAVIGATE_TOOL );
    m_toolManager->RegisterTool( new PROPERTIES_TOOL );
    m_toolManager->RegisterTool( new EMBED_TOOL );
    m_toolManager->InitTools();

    // Run the selection tool, it is supposed to be always active
    m_toolManager->PostAction( ACTIONS::selectionActivate );

    GetCanvas()->SetEventDispatcher( m_toolDispatcher );
}


void SCH_EDIT_FRAME::setupUIConditions()
{
    SCH_BASE_FRAME::setupUIConditions();

    ACTION_MANAGER*       mgr = m_toolManager->GetActionManager();
    SCH_EDITOR_CONDITIONS cond( this );

    wxASSERT( mgr );

    auto hasElements =
            [ this ] ( const SELECTION& aSel )
            {
                return GetScreen() &&
                        ( !GetScreen()->Items().empty() || !SELECTION_CONDITIONS::Idle( aSel ) );
            };

    auto searchPaneCond =
            [this] ( const SELECTION& )
            {
                return m_auimgr.GetPane( SearchPaneName() ).IsShown();
            };

    auto propertiesCond =
            [this] ( const SELECTION& )
            {
                return m_auimgr.GetPane( PropertiesPaneName() ).IsShown();
            };

    auto hierarchyNavigatorCond =
            [ this ] ( const SELECTION& aSel )
            {
                return m_auimgr.GetPane( SchematicHierarchyPaneName() ).IsShown();
            };

    auto netNavigatorCond =
            [ this ] (const SELECTION& aSel )
            {
                return m_auimgr.GetPane( NetNavigatorPaneName() ).IsShown();
            };

    auto designBlockCond =
            [ this ] (const SELECTION& aSel )
            {
                return m_auimgr.GetPane( DesignBlocksPaneName() ).IsShown();
            };

    auto remoteSymbolCond =
            [ this ] (const SELECTION& aSel )
            {
                return m_auimgr.GetPane( RemoteSymbolPaneName() ).IsShown();
            };

    auto undoCond =
            [ this ] (const SELECTION& aSel )
            {
                if( SCH_LINE_WIRE_BUS_TOOL::IsDrawingLineWireOrBus( aSel ) )
                    return true;

                return GetUndoCommandCount() > 0;
            };

    auto groupWithDesignBlockLink =
            [] ( const SELECTION& aSel )
            {
                if( aSel.Size() != 1 )
                    return false;

                if( aSel[0]->Type() != SCH_GROUP_T )
                    return false;

                SCH_GROUP* group = static_cast<SCH_GROUP*>( aSel.GetItem( 0 ) );

                return group->HasDesignBlockLink();
            };

#define ENABLE( x ) ACTION_CONDITIONS().Enable( x )
#define CHECK( x )  ACTION_CONDITIONS().Check( x )

    mgr->SetConditions( ACTIONS::save,                ENABLE( SELECTION_CONDITIONS::ShowAlways ) );
    mgr->SetConditions( ACTIONS::undo,                ENABLE( undoCond ) );
    mgr->SetConditions( ACTIONS::redo,                ENABLE( cond.RedoAvailable() ) );

    mgr->SetConditions( SCH_ACTIONS::showSearch,           CHECK( searchPaneCond ) );
    mgr->SetConditions( SCH_ACTIONS::showHierarchy,        CHECK( hierarchyNavigatorCond ) );
    mgr->SetConditions( SCH_ACTIONS::showNetNavigator,     CHECK( netNavigatorCond ) );
    mgr->SetConditions( ACTIONS::showProperties,           CHECK( propertiesCond ) );
    mgr->SetConditions( SCH_ACTIONS::showDesignBlockPanel, CHECK( designBlockCond ) );
    mgr->SetConditions( SCH_ACTIONS::showRemoteSymbolPanel, CHECK( remoteSymbolCond ) );
    mgr->SetConditions( ACTIONS::toggleGrid,               CHECK( cond.GridVisible() ) );
    mgr->SetConditions( ACTIONS::toggleGridOverrides,      CHECK( cond.GridOverrides() ) );

    mgr->SetConditions( ACTIONS::cut,                 ENABLE( hasElements ) );
    mgr->SetConditions( ACTIONS::copy,                ENABLE( hasElements ) );
    mgr->SetConditions( ACTIONS::copyAsText,          ENABLE( hasElements ) );
    mgr->SetConditions( ACTIONS::paste,               ENABLE( SELECTION_CONDITIONS::Idle ) );
    mgr->SetConditions( ACTIONS::pasteSpecial,        ENABLE( SELECTION_CONDITIONS::Idle ) );
    mgr->SetConditions( ACTIONS::doDelete,            ENABLE( hasElements ) );
    mgr->SetConditions( ACTIONS::duplicate,           ENABLE( hasElements ) );
    mgr->SetConditions( SCH_ACTIONS::alignLeft,       ENABLE( SELECTION_CONDITIONS::MoreThan( 1 ) ) );
    mgr->SetConditions( SCH_ACTIONS::alignCenterX,    ENABLE( SELECTION_CONDITIONS::MoreThan( 1 ) ) );
    mgr->SetConditions( SCH_ACTIONS::alignRight,      ENABLE( SELECTION_CONDITIONS::MoreThan( 1 ) ) );
    mgr->SetConditions( SCH_ACTIONS::alignTop,        ENABLE( SELECTION_CONDITIONS::MoreThan( 1 ) ) );
    mgr->SetConditions( SCH_ACTIONS::alignCenterY,    ENABLE( SELECTION_CONDITIONS::MoreThan( 1 ) ) );
    mgr->SetConditions( SCH_ACTIONS::alignBottom,     ENABLE( SELECTION_CONDITIONS::MoreThan( 1 ) ) );
    mgr->SetConditions( ACTIONS::selectAll,           ENABLE( hasElements ) );
    mgr->SetConditions( ACTIONS::unselectAll,         ENABLE( hasElements ) );

    mgr->SetConditions( SCH_ACTIONS::rotateCW,        ENABLE( hasElements ) );
    mgr->SetConditions( SCH_ACTIONS::rotateCCW,       ENABLE( hasElements ) );
    mgr->SetConditions( SCH_ACTIONS::mirrorH,         ENABLE( hasElements ) );
    mgr->SetConditions( SCH_ACTIONS::mirrorV,         ENABLE( hasElements ) );
    mgr->SetConditions( ACTIONS::group,               ENABLE( SELECTION_CONDITIONS::MoreThan( 1 ) ) );
    mgr->SetConditions( ACTIONS::ungroup,             ENABLE( SELECTION_CONDITIONS::HasType( SCH_GROUP_T ) ) );

    mgr->SetConditions( SCH_ACTIONS::placeLinkedDesignBlock, ENABLE( groupWithDesignBlockLink ) );
    mgr->SetConditions( SCH_ACTIONS::saveToLinkedDesignBlock, ENABLE( groupWithDesignBlockLink ) );

    mgr->SetConditions( ACTIONS::zoomTool,            CHECK( cond.CurrentTool( ACTIONS::zoomTool ) ) );
    mgr->SetConditions( ACTIONS::selectionTool,       CHECK( cond.CurrentTool( ACTIONS::selectionTool ) ) );

    auto showHiddenPinsCond =
            [this]( const SELECTION& )
            {
                return GetShowAllPins();
            };

    auto showHiddenFieldsCond =
            [this]( const SELECTION& )
            {
                EESCHEMA_SETTINGS* cfg = eeconfig();
                return cfg && cfg->m_Appearance.show_hidden_fields;
            };

    auto showDirectiveLabelsCond =
            [this]( const SELECTION& )
            {
                EESCHEMA_SETTINGS* cfg = eeconfig();
                return cfg && cfg->m_Appearance.show_directive_labels;
            };

    auto showERCErrorsCond =
            [this]( const SELECTION& )
            {
                EESCHEMA_SETTINGS* cfg = eeconfig();
                return cfg && cfg->m_Appearance.show_erc_errors;
            };

    auto showERCWarningsCond =
            [this]( const SELECTION& )
            {
                EESCHEMA_SETTINGS* cfg = eeconfig();
                return cfg && cfg->m_Appearance.show_erc_warnings;
            };

    auto showERCExclusionsCond =
            [this]( const SELECTION& )
            {
                EESCHEMA_SETTINGS* cfg = eeconfig();
                return cfg && cfg->m_Appearance.show_erc_exclusions;
            };

    auto markSimExclusionsCond =
            [this]( const SELECTION& )
            {
                EESCHEMA_SETTINGS* cfg = eeconfig();
                return cfg && cfg->m_Appearance.mark_sim_exclusions;
            };

    auto showOPVoltagesCond =
            [this]( const SELECTION& )
            {
                EESCHEMA_SETTINGS* cfg = eeconfig();
                return cfg && cfg->m_Appearance.show_op_voltages;
            };

    auto showOPCurrentsCond =
            [this]( const SELECTION& )
            {
                EESCHEMA_SETTINGS* cfg = eeconfig();
                return cfg && cfg->m_Appearance.show_op_currents;
            };

    auto showPinAltModeIconsCond =
            [this]( const SELECTION& )
            {
                EESCHEMA_SETTINGS* cfg = eeconfig();
                return cfg && cfg->m_Appearance.show_pin_alt_icons;
            };

    auto showAnnotateAutomaticallyCond =
            [this]( const SELECTION& )
            {
                return eeconfig()->m_AnnotatePanel.automatic;
            };

    auto remapSymbolsCondition =
            [&]( const SELECTION& aSel )
            {
                SCH_SCREENS schematic( Schematic().Root() );

                // The remapping can only be performed on legacy projects.
                return schematic.HasNoFullyDefinedLibIds();
            };

    auto belowRootSheetCondition =
            [this]( const SELECTION& aSel )
            {
                SCH_NAVIGATE_TOOL* navigateTool = m_toolManager->GetTool<SCH_NAVIGATE_TOOL>();
                return navigateTool && navigateTool->CanGoUp();
            };

    mgr->SetConditions( SCH_ACTIONS::leaveSheet,            ENABLE( belowRootSheetCondition ) );

    /* Some of these are bound by default to arrow keys which will get a different action if we
     * disable the buttons.  So always leave them enabled so the action is consistent.
     * https://gitlab.com/kicad/code/kicad/-/issues/14783
    mgr->SetConditions( SCH_ACTIONS::navigateUp,            ENABLE( belowRootSheetCondition ) );
    mgr->SetConditions( SCH_ACTIONS::navigateForward,       ENABLE( navHistoryHasForward ) );
    mgr->SetConditions( SCH_ACTIONS::navigateBack,          ENABLE( navHistoryHsBackward ) );
     */

    mgr->SetConditions( SCH_ACTIONS::remapSymbols,          ENABLE( remapSymbolsCondition ) );
    mgr->SetConditions( SCH_ACTIONS::toggleHiddenPins,      CHECK( showHiddenPinsCond ) );
    mgr->SetConditions( SCH_ACTIONS::toggleHiddenFields,    CHECK( showHiddenFieldsCond ) );
    mgr->SetConditions( SCH_ACTIONS::toggleDirectiveLabels, CHECK( showDirectiveLabelsCond ) );
    mgr->SetConditions( SCH_ACTIONS::toggleERCErrors,       CHECK( showERCErrorsCond ) );
    mgr->SetConditions( SCH_ACTIONS::toggleERCWarnings,     CHECK( showERCWarningsCond ) );
    mgr->SetConditions( SCH_ACTIONS::toggleERCExclusions,   CHECK( showERCExclusionsCond ) );
    mgr->SetConditions( SCH_ACTIONS::markSimExclusions,     CHECK( markSimExclusionsCond ) );
    mgr->SetConditions( SCH_ACTIONS::toggleOPVoltages,      CHECK( showOPVoltagesCond ) );
    mgr->SetConditions( SCH_ACTIONS::toggleOPCurrents,      CHECK( showOPCurrentsCond ) );
    mgr->SetConditions( SCH_ACTIONS::togglePinAltIcons,     CHECK( showPinAltModeIconsCond ) );
    mgr->SetConditions( SCH_ACTIONS::toggleAnnotateAuto,    CHECK( showAnnotateAutomaticallyCond ) );
    mgr->SetConditions( ACTIONS::toggleBoundingBoxes,       CHECK( cond.BoundingBoxes() ) );

    mgr->SetConditions( SCH_ACTIONS::saveSheetAsDesignBlock,     ENABLE( hasElements ) );
    mgr->SetConditions( SCH_ACTIONS::saveSelectionAsDesignBlock, ENABLE( SELECTION_CONDITIONS::NotEmpty ) );

#define CURRENT_TOOL( action ) mgr->SetConditions( action, CHECK( cond.CurrentTool( action ) ) )

    CURRENT_TOOL( ACTIONS::deleteTool );
    CURRENT_TOOL( SCH_ACTIONS::highlightNetTool );
    CURRENT_TOOL( SCH_ACTIONS::placeSymbol );
    CURRENT_TOOL( SCH_ACTIONS::placePower );
    CURRENT_TOOL( SCH_ACTIONS::placeDesignBlock );
    CURRENT_TOOL( SCH_ACTIONS::drawWire );
    CURRENT_TOOL( SCH_ACTIONS::drawBus );
    CURRENT_TOOL( SCH_ACTIONS::placeBusWireEntry );
    CURRENT_TOOL( SCH_ACTIONS::placeNoConnect );
    CURRENT_TOOL( SCH_ACTIONS::placeJunction );
    CURRENT_TOOL( SCH_ACTIONS::placeLabel );
    CURRENT_TOOL( SCH_ACTIONS::placeClassLabel );
    CURRENT_TOOL( SCH_ACTIONS::placeGlobalLabel );
    CURRENT_TOOL( SCH_ACTIONS::placeHierLabel );
    CURRENT_TOOL( SCH_ACTIONS::drawRuleArea );
    CURRENT_TOOL( SCH_ACTIONS::drawSheet );
    CURRENT_TOOL( SCH_ACTIONS::placeSheetPin );
    CURRENT_TOOL( SCH_ACTIONS::syncSheetPins );
    CURRENT_TOOL( SCH_ACTIONS::drawSheetFromFile );
    CURRENT_TOOL( SCH_ACTIONS::drawSheetFromDesignBlock );
    CURRENT_TOOL( SCH_ACTIONS::drawRectangle );
    CURRENT_TOOL( SCH_ACTIONS::drawCircle );
    CURRENT_TOOL( SCH_ACTIONS::drawArc );
    CURRENT_TOOL( SCH_ACTIONS::drawBezier );
    CURRENT_TOOL( SCH_ACTIONS::drawLines );
    CURRENT_TOOL( SCH_ACTIONS::placeSchematicText );
    CURRENT_TOOL( SCH_ACTIONS::drawTextBox );
    CURRENT_TOOL( SCH_ACTIONS::drawTable );
    CURRENT_TOOL( SCH_ACTIONS::placeImage );

#undef CURRENT_TOOL
#undef CHECK
#undef ENABLE
}


void SCH_EDIT_FRAME::SaveCopyForRepeatItem( const SCH_ITEM* aItem )
{
    // we cannot store a pointer to an item in the display list here since
    // that item may be deleted, such as part of a line concatenation or other.
    // So simply always keep a copy of the object which is to be repeated.

    if( aItem )
    {
        m_items_to_repeat.clear();

        AddCopyForRepeatItem( aItem );
    }
}


void SCH_EDIT_FRAME::AddCopyForRepeatItem( const SCH_ITEM* aItem )
{
    // we cannot store a pointer to an item in the display list here since
    // that item may be deleted, such as part of a line concatenation or other.
    // So simply always keep a copy of the object which is to be repeated.

    if( aItem )
    {
        std::unique_ptr<SCH_ITEM> repeatItem( static_cast<SCH_ITEM*>( aItem->Duplicate( IGNORE_PARENT_GROUP ) ) );

        // Clone() preserves the flags & parent, we want 'em cleared.
        repeatItem->ClearFlags();
        repeatItem->SetParent( nullptr );

        m_items_to_repeat.emplace_back( std::move( repeatItem ) );
    }
}


EDA_ITEM* SCH_EDIT_FRAME::ResolveItem( const KIID& aId, bool aAllowNullptrReturn ) const
{
    return Schematic().ResolveItem( aId, nullptr, aAllowNullptrReturn );
}


void SCH_EDIT_FRAME::SetSheetNumberAndCount()
{
    Schematic().SetSheetNumberAndCount();
}


SCH_SCREEN* SCH_EDIT_FRAME::GetScreen() const
{
    return GetCurrentSheet().LastScreen();
}


SCHEMATIC& SCH_EDIT_FRAME::Schematic() const
{
    return *m_schematic;
}


wxString SCH_EDIT_FRAME::GetScreenDesc() const
{
    return GetCurrentSheet().Last()->GetName();
}


wxString SCH_EDIT_FRAME::GetFullScreenDesc() const
{
    return GetCurrentSheet().PathHumanReadable();
}


void SCH_EDIT_FRAME::CreateDefaultScreens()
{
    m_schematic->CreateDefaultScreens();
    SetScreen( Schematic().RootScreen() );

    if( GetScreen() == nullptr )
    {
        SCH_SCREEN* screen = new SCH_SCREEN( m_schematic );
        SetScreen( screen );
    }
}


SCH_SHEET_PATH& SCH_EDIT_FRAME::GetCurrentSheet() const
{
    return m_schematic->CurrentSheet();
}


void SCH_EDIT_FRAME::SetCurrentSheet( const SCH_SHEET_PATH& aSheet )
{
    if( aSheet != GetCurrentSheet() )
    {
        ClearFocus();

        wxLogTrace( traceSchCurrentSheet,
                   "SCH_EDIT_FRAME::SetCurrentSheet: Changing from path='%s' (size=%zu) to path='%s' (size=%zu)",
                   GetCurrentSheet().Path().AsString(),
                   GetCurrentSheet().size(),
                   aSheet.Path().AsString(),
                   aSheet.size() );

        Schematic().SetCurrentSheet( aSheet );
        GetCanvas()->DisplaySheet( aSheet.LastScreen() );
    }
}


void SCH_EDIT_FRAME::HardRedraw()
{
    SCH_SCREEN* screen = GetCurrentSheet().LastScreen();

    for( SCH_ITEM* item : screen->Items() )
        item->ClearCaches();

    for( const std::pair<const wxString, LIB_SYMBOL*>& libSymbol : screen->GetLibSymbols() )
    {
        wxCHECK2( libSymbol.second, continue );
        libSymbol.second->ClearCaches();
    }

    if( Schematic().Settings().m_IntersheetRefsShow )
        RecomputeIntersheetRefs();

    ClearFocus();

    GetCanvas()->DisplaySheet( GetCurrentSheet().LastScreen() );

    if( SCH_SELECTION_TOOL* selectionTool = m_toolManager->GetTool<SCH_SELECTION_TOOL>() )
        selectionTool->Reset( TOOL_BASE::REDRAW );

    GetCanvas()->ForceRefresh();
}


bool SCH_EDIT_FRAME::canCloseWindow( wxCloseEvent& aEvent )
{
    // Exit interactive editing
    // Note this this will commit *some* pending changes.  For instance, the SCH_POINT_EDITOR
    // will cancel any drag currently in progress, but commit all changes from previous drags.
    if( m_toolManager )
        m_toolManager->RunAction( ACTIONS::cancelInteractive );

    // Shutdown blocks must be determined and vetoed as early as possible
    if( KIPLATFORM::APP::SupportsShutdownBlockReason() && aEvent.GetId() == wxEVT_QUERY_END_SESSION
            && IsContentModified() )
    {
        return false;
    }

    if( Kiface().IsSingle() )
    {
        auto* symbolEditor = (SYMBOL_EDIT_FRAME*) Kiway().Player( FRAME_SCH_SYMBOL_EDITOR, false );

        if( symbolEditor && !symbolEditor->Close() )   // Can close symbol editor?
            return false;

        auto* symbolViewer = (SYMBOL_VIEWER_FRAME*) Kiway().Player( FRAME_SCH_VIEWER, false );

        if( symbolViewer && !symbolViewer->Close() )   // Can close symbol viewer?
            return false;

        // SYMBOL_CHOOSER_FRAME is always modal so this shouldn't come up, but better safe than
        // sorry.
        auto* chooser = (SYMBOL_CHOOSER_FRAME*) Kiway().Player( FRAME_SYMBOL_CHOOSER, false );

        if( chooser && !chooser->Close() )   // Can close symbol chooser?
            return false;
    }
    else
    {
        auto* symbolEditor = (SYMBOL_EDIT_FRAME*) Kiway().Player( FRAME_SCH_SYMBOL_EDITOR, false );

        if( symbolEditor && symbolEditor->IsSymbolFromSchematic() )
        {
            if( !symbolEditor->CanCloseSymbolFromSchematic( true ) )
                return false;
        }
    }

    if( !Kiway().PlayerClose( FRAME_SIMULATOR, false ) )   // Can close the simulator?
        return false;

    if( m_symbolFieldsTableDialog
        && !m_symbolFieldsTableDialog->Close( false ) ) // Can close the symbol fields table?
    {
        return false;
    }

    // We may have gotten multiple events; don't clean up twice
    if( !Schematic().IsValid() )
        return false;

    if( IsContentModified() )
    {
        wxFileName fileName = Schematic().RootScreen()->GetFileName();
        wxString msg = _( "Save changes to '%s' before closing?" );

        if( !HandleUnsavedChanges( this, wxString::Format( msg, fileName.GetFullName() ),
                                   [&]() -> bool
                                   {
                                       return SaveProject();
                                   } ) )
        {
            return false;
        }

        // If user selected 'No' (discard), create duplicate commit of last saved state and
        // move Last_Save tag forward so history shows an explicit discard event.
        if( GetLastUnsavedChangesResponse() == wxID_NO )
        {
            wxString projPath = Prj().GetProjectPath();

            if( !projPath.IsEmpty() && Kiway().LocalHistory().HistoryExists( projPath ) )
            {
                Kiway().LocalHistory().CommitDuplicateOfLastSave( projPath, wxS( "Schematic" ),
                                                                  wxS( "Discard unsaved schematic changes" ) );
            }
        }
    }

    return true;
}


void SCH_EDIT_FRAME::doCloseWindow()
{
    // Unregister the autosave saver before any cleanup that might invalidate m_schematic
    if( m_schematic )
        Kiway().LocalHistory().UnregisterSaver( m_schematic );

    SCH_BASE_FRAME::doCloseWindow();

    SCH_SHEET_LIST sheetlist = Schematic().Hierarchy();

#ifdef KICAD_IPC_API
    Pgm().GetApiServer().DeregisterHandler( m_apiHandler.get() );
    wxTheApp->Unbind( EDA_EVT_PLUGIN_AVAILABILITY_CHANGED, &SCH_EDIT_FRAME::onPluginAvailabilityChanged, this );
#endif

    // Close modeless dialogs.  They're trouble when they get destroyed after the frame.
    Unbind( EDA_EVT_CLOSE_DIALOG_BOOK_REPORTER, &SCH_EDIT_FRAME::onCloseSymbolDiffDialog, this );
    Unbind( EDA_EVT_CLOSE_ERC_DIALOG, &SCH_EDIT_FRAME::onCloseErcDialog, this );
    Unbind( EDA_EVT_CLOSE_DIALOG_SYMBOL_FIELDS_TABLE, &SCH_EDIT_FRAME::onCloseSymbolFieldsTableDialog, this );
    m_netNavigator->Unbind( wxEVT_TREE_SEL_CHANGING, &SCH_EDIT_FRAME::onNetNavigatorSelChanging, this );
    m_netNavigator->Unbind( wxEVT_TREE_SEL_CHANGED, &SCH_EDIT_FRAME::onNetNavigatorSelection, this );

    if( m_diffSymbolDialog )
    {
        m_diffSymbolDialog->Destroy();
        m_diffSymbolDialog = nullptr;
    }

    if( m_ercDialog )
    {
        m_ercDialog->Destroy();
        m_ercDialog = nullptr;
    }

    if( m_symbolFieldsTableDialog )
    {
        m_symbolFieldsTableDialog->Destroy();
        m_symbolFieldsTableDialog = nullptr;
    }

    // Make sure local settings are persisted
    if( Prj().GetLocalSettings().ShouldAutoSave() )
        SaveProjectLocalSettings();

    delete m_toolManager;
    m_toolManager = nullptr;

    wxAuiPaneInfo& hierarchy_pane = m_auimgr.GetPane( SchematicHierarchyPaneName() );

    if( hierarchy_pane.IsShown() && hierarchy_pane.IsFloating() )
    {
        hierarchy_pane.Show( false );
        m_auimgr.Update();
    }

    sheetlist.ClearModifyStatus();

    wxString fileName = Prj().AbsolutePath( Schematic().RootScreen()->GetFileName() );

    if( !Schematic().GetFileName().IsEmpty() && !Schematic().RootScreen()->IsEmpty() )
        UpdateFileHistory( fileName );

    Schematic().RootScreen()->Clear( true );

    // all sub sheets are deleted, only the main sheet is usable
    GetCurrentSheet().clear();

    // Clear view before destroying schematic as repaints depend on schematic being valid
    SetScreen( nullptr );

    Schematic().Reset();

    // Prevents any rogue events from continuing (i.e. search panel tries to redraw)
    Show( false );

    Destroy();
}


void SCH_EDIT_FRAME::FocusSearch()
{
    m_searchPane->FocusSearch();
}


SEVERITY SCH_EDIT_FRAME::GetSeverity( int aErrorCode ) const
{
    return Schematic().ErcSettings().GetSeverity( aErrorCode );
}


void SCH_EDIT_FRAME::OnModify()
{
    EDA_BASE_FRAME::OnModify();

    if( GetScreen() )
    {
        GetScreen()->SetContentModified();
        Kiway().LocalHistory().NoteFileChange( GetScreen()->GetFileName() );
    }

    if( m_isClosing )
        return;

    if( GetCanvas() )
        GetCanvas()->Refresh();

    if( GetScreen() && !GetTitle().StartsWith( wxS( "*" ) ) )
        updateTitle();
}


void SCH_EDIT_FRAME::OnUpdatePCB()
{
    if( Kiface().IsSingle() )
    {
        DisplayError( this,  _( "Cannot update the PCB because the Schematic Editor is opened in stand-alone "
                                "mode. In order to create/update PCBs from schematics, launch the main KiCad "
                                "application and create a project." ) );
        return;
    }

    KIWAY_PLAYER* frame = Kiway().Player( FRAME_PCB_EDITOR, false );
    wxEventBlocker blocker( this );

    if( !frame )
    {
        wxFileName fn = Prj().GetProjectFullName();
        fn.SetExt( FILEEXT::PcbFileExtension );

        frame = Kiway().Player( FRAME_PCB_EDITOR, true );

        // If Kiway() cannot create the Pcbnew frame, it shows a error message, and
        // frame is null
        if( !frame )
            return;

        frame->OpenProjectFiles( std::vector<wxString>( 1, fn.GetFullPath() ) );
    }

    if( !frame->IsVisible() )
        frame->Show( true );

    // On Windows, Raise() does not bring the window on screen, when iconized
    if( frame->IsIconized() )
        frame->Iconize( false );

    frame->Raise();

    std::string payload;
    Kiway().ExpressMail( FRAME_PCB_EDITOR, MAIL_PCB_UPDATE, payload, this );
}


void SCH_EDIT_FRAME::UpdateHierarchyNavigator( bool aRefreshNetNavigator, bool aClear )
{
    m_toolManager->GetTool<SCH_NAVIGATE_TOOL>()->CleanHistory();
    m_hierarchy->UpdateHierarchyTree( aClear );

    if( aRefreshNetNavigator )
        RefreshNetNavigator();
}


void SCH_EDIT_FRAME::UpdateLabelsHierarchyNavigator()
{
    // Update only the hierarchy navigation tree labels.
    // The tree list is expected to be up to date
    m_hierarchy->UpdateLabelsHierarchyTree();
}


void SCH_EDIT_FRAME::UpdateHierarchySelection()
{
    m_hierarchy->UpdateHierarchySelection();
}


void SCH_EDIT_FRAME::OnLoadFile( wxCommandEvent& event )
{
    wxString filename = GetFileFromHistory( event.GetId(), _( "Schematic" ) );

    if( !filename.IsEmpty() )
        OpenProjectFiles( std::vector<wxString>( 1, filename ) );
}


void SCH_EDIT_FRAME::OnClearFileHistory( wxCommandEvent& aEvent )
{
    ClearFileHistory();
}


void SCH_EDIT_FRAME::NewProject()
{
    // Only standalone mode can directly load a new document
    if( !Kiface().IsSingle() )
        return;

    wxString pro_dir = m_mruPath;

    wxFileDialog dlg( this, _( "New Schematic" ), pro_dir, wxEmptyString, FILEEXT::KiCadSchematicFileWildcard(),
                      wxFD_SAVE );

    KIPLATFORM::UI::AllowNetworkFileSystems( &dlg );

    if( dlg.ShowModal() != wxID_CANCEL )
    {
        // Enforce the extension, wxFileDialog is inept.
        wxFileName create_me = EnsureFileExtension( dlg.GetPath(), FILEEXT::KiCadSchematicFileExtension );

        if( create_me.FileExists() )
        {
            wxString msg;
            msg.Printf( _( "Schematic file '%s' already exists." ), create_me.GetFullName() );
            DisplayError( this, msg );
            return ;
        }

        // OpenProjectFiles() requires absolute
        wxASSERT_MSG( create_me.IsAbsolute(), wxS( "wxFileDialog returned non-absolute path" ) );

        OpenProjectFiles( std::vector<wxString>( 1, create_me.GetFullPath() ), KICTL_CREATE );
        m_mruPath = create_me.GetPath();
    }
}


void SCH_EDIT_FRAME::LoadProject()
{
    // Only standalone mode can directly load a new document
    if( !Kiface().IsSingle() )
        return;

    wxString pro_dir = m_mruPath;
    wxString wildcards = FILEEXT::AllSchematicFilesWildcard()
                            + wxS( "|" ) + FILEEXT::KiCadSchematicFileWildcard()
                            + wxS( "|" ) + FILEEXT::LegacySchematicFileWildcard();

    wxFileDialog dlg( this, _( "Open Schematic" ), pro_dir, wxEmptyString, wildcards,
                      wxFD_OPEN | wxFD_FILE_MUST_EXIST );

    KIPLATFORM::UI::AllowNetworkFileSystems( &dlg );

    if( dlg.ShowModal() != wxID_CANCEL )
    {
        OpenProjectFiles( std::vector<wxString>( 1, dlg.GetPath() ) );
        m_mruPath = Prj().GetProjectPath();
    }

    // Since we know we're single-top here: trigger library reload
    CallAfter(
            [&]()
            {
                KIFACE *schface = Kiway().KiFACE( KIWAY::FACE_SCH );
                schface->PreloadLibraries( &Kiway() );

                Pgm().PreloadDesignBlockLibraries( &Kiway() );
            } );
}


void SCH_EDIT_FRAME::ProjectChanged()
{
    SCH_BASE_FRAME::ProjectChanged();

    // Register schematic saver for autosave history
    Kiway().LocalHistory().RegisterSaver( m_schematic,
            [this]( const wxString& aProjectPath, std::vector<wxString>& aFiles )
            {
                m_schematic->SaveToHistory( aProjectPath, aFiles );
            } );

    m_designBlocksPane->ProjectChanged();
}


void SCH_EDIT_FRAME::OnOpenPcbnew()
{
    wxFileName kicad_board = Prj().AbsolutePath( Schematic().GetFileName() );

    if( kicad_board.IsOk() && !Schematic().GetFileName().IsEmpty() )
    {
        kicad_board.SetExt( FILEEXT::PcbFileExtension );
        wxFileName legacy_board( kicad_board );
        legacy_board.SetExt( FILEEXT::LegacyPcbFileExtension );
        wxFileName& boardfn = legacy_board;

        if( !legacy_board.FileExists() || kicad_board.FileExists() )
            boardfn = kicad_board;

        if( Kiface().IsSingle() )
        {
            ExecuteFile( PCBNEW_EXE, boardfn.GetFullPath() );
        }
        else
        {
            wxEventBlocker blocker(this);
            KIWAY_PLAYER* frame = Kiway().Player( FRAME_PCB_EDITOR, false );

            if( !frame )
            {
                frame = Kiway().Player( FRAME_PCB_EDITOR, true );

                // frame can be null if Cvpcb cannot be run. No need to show a warning
                // Kiway() generates the error messages
                if( !frame )
                    return;

                frame->OpenProjectFiles( std::vector<wxString>( 1, boardfn.GetFullPath() ) );
            }

            if( !frame->IsVisible() )
                frame->Show( true );

            // On Windows, Raise() does not bring the window on screen, when iconized
            if( frame->IsIconized() )
                frame->Iconize( false );

            frame->Raise();
        }
    }
    else
    {
        ExecuteFile( PCBNEW_EXE );
    }
}


void SCH_EDIT_FRAME::OnOpenCvpcb()
{
    wxFileName fn = Prj().AbsolutePath( Schematic().GetFileName() );
    fn.SetExt( FILEEXT::NetlistFileExtension );

    if( !ReadyToNetlist( _( "Assigning footprints requires a fully annotated schematic." ) ) )
        return;

    try
    {
        KIWAY_PLAYER* player = Kiway().Player( FRAME_CVPCB, false );  // test open already.

        if( !player )
        {
            player = Kiway().Player( FRAME_CVPCB, true );

            // player can be null if Cvpcb cannot be run. No need to show a warning
            // Kiway() generates the error messages
            if( !player )
                return;

            player->Show( true );
        }

        // Ensure the netlist (mainly info about symbols) is up to date
        RecalculateConnections( nullptr, GLOBAL_CLEANUP );
        sendNetlistToCvpcb();

        player->Raise();
    }
    catch( const IO_ERROR& )
    {
        DisplayError( this, _( "Could not open CvPcb" ) );
    }
}


void SCH_EDIT_FRAME::OnExit( wxCommandEvent& event )
{
    if( event.GetId() == wxID_EXIT )
        Kiway().OnKiCadExit();

    if( event.GetId() == wxID_CLOSE || Kiface().IsSingle() )
        Close( false );
}


void SCH_EDIT_FRAME::RefreshOperatingPointDisplay()
{
    SCHEMATIC_SETTINGS& settings = m_schematic->Settings();
    SIM_LIB_MGR         simLibMgr( &Prj() );
    NULL_REPORTER       devnull;
    SCH_SHEET_PATH&     sheetPath = GetCurrentSheet();
    wxString            variant = m_schematic->GetCurrentVariant();

    // Patch for bug early in V7.99 dev
    if( settings.m_OPO_VRange.EndsWith( 'A' ) )
        settings.m_OPO_VRange[ settings.m_OPO_VRange.Length() - 1 ] = 'V';

    // Update items which may have ${OP} text variables
    //
    GetCanvas()->GetView()->UpdateAllItemsConditionally(
            [&]( KIGFX::VIEW_ITEM* aItem ) -> int
            {
                int flags = 0;

                auto invalidateTextVars =
                        [&flags]( EDA_TEXT* text )
                        {
                            if( text->HasTextVars() )
                            {
                                text->ClearRenderCache();
                                text->ClearBoundingBoxCache();
                                flags |= KIGFX::GEOMETRY | KIGFX::REPAINT;
                            }
                        };

                if( SCH_ITEM* item = dynamic_cast<SCH_ITEM*>( aItem ) )
                {
                    item->RunOnChildren(
                            [&invalidateTextVars]( SCH_ITEM* aChild )
                            {
                                if( EDA_TEXT* text = dynamic_cast<EDA_TEXT*>( aChild ) )
                                    invalidateTextVars( text );
                            },
                            RECURSE_MODE::NO_RECURSE );
                }

                if( EDA_TEXT* text = dynamic_cast<EDA_TEXT*>( aItem ) )
                    invalidateTextVars( text );

                return flags;
            } );

    // Update OP overlay items
    //
    for( SCH_ITEM* item : GetScreen()->Items() )
    {
        if( sheetPath.GetExcludedFromSim( variant ) )
            continue;

        if( item->Type() == SCH_LINE_T )
        {
            SCH_LINE* line = static_cast<SCH_LINE*>( item );

            if( !line->GetOperatingPoint().IsEmpty() )
                GetCanvas()->GetView()->Update( line );

            line->SetOperatingPoint( wxEmptyString );

            // update value from netlist, below
        }
        else if( item->Type() == SCH_SYMBOL_T )
        {
            SCH_SYMBOL*           symbol = static_cast<SCH_SYMBOL*>( item );
            wxString              ref = symbol->GetRef( &sheetPath );
            std::vector<SCH_PIN*> pins = symbol->GetPins( &sheetPath );

            // Power symbols and other symbols which have the reference starting with "#" are
            // not included in simulation
            if( ref.StartsWith( '#' ) || symbol->ResolveExcludedFromSim( &sheetPath, variant ) )
                continue;

            for( SCH_PIN* pin : pins )
            {
                if( !pin->GetOperatingPoint().IsEmpty() )
                    GetCanvas()->GetView()->Update( pin );

                pin->SetOperatingPoint( wxEmptyString );
            }

            if( pins.size() == 2 )
            {
                wxString op = m_schematic->GetOperatingPoint( ref, settings.m_OPO_IPrecision, settings.m_OPO_IRange );

                if( !op.IsEmpty() && op != wxS( "--" ) && op != wxS( "?" ) )
                {
                    pins[0]->SetOperatingPoint( op );
                    GetCanvas()->GetView()->Update( symbol );
                }
            }
            else
            {
                std::vector<EMBEDDED_FILES*> embeddedFilesStack;
                embeddedFilesStack.push_back( m_schematic->GetEmbeddedFiles() );

                if( EMBEDDED_FILES* symbolEmbeddedFiles = symbol->GetEmbeddedFiles() )
                {
                    embeddedFilesStack.push_back( symbolEmbeddedFiles );
                    symbol->GetLibSymbolRef()->AppendParentEmbeddedFiles( embeddedFilesStack );
                }

                simLibMgr.SetFilesStack( std::move( embeddedFilesStack ) );

                SIM_MODEL& model = simLibMgr.CreateModel( &sheetPath, *symbol, true, 0, variant, devnull ).model;

                SPICE_ITEM spiceItem;
                spiceItem.refName = ref;
                ref = model.SpiceGenerator().ItemName( spiceItem );

                for( const auto& modelPin : model.GetPins() )
                {
                    SCH_PIN* symbolPin = symbol->GetPin( modelPin.get().symbolPinNumber );
                    wxString signalName = ref + wxS( ":" ) + modelPin.get().modelPinName;
                    wxString op = m_schematic->GetOperatingPoint( signalName, settings.m_OPO_IPrecision,
                                                                  settings.m_OPO_IRange );

                    if( symbolPin && !op.IsEmpty() && op != wxS( "--" ) && op != wxS( "?" ) )
                    {
                        symbolPin->SetOperatingPoint( op );
                        GetCanvas()->GetView()->Update( symbol );
                    }
                }
            }
        }
    }

    for( const auto& [ key, subgraphList ] : m_schematic->m_connectionGraph->GetNetMap() )
    {
        wxString op = m_schematic->GetOperatingPoint( key.Name, settings.m_OPO_VPrecision, settings.m_OPO_VRange );

        if( !op.IsEmpty() && op != wxS( "--" ) && op != wxS( "?" ) )
        {
            for( CONNECTION_SUBGRAPH* subgraph : subgraphList )
            {
                SCH_LINE* longestWire = nullptr;
                double    length = 0.0;

                if( subgraph->GetSheet().GetExcludedFromSim( variant ) )
                    continue;

                for( SCH_ITEM* item : subgraph->GetItems() )
                {
                    if( item->Type() == SCH_LINE_T )
                    {
                        SCH_LINE* line = static_cast<SCH_LINE*>( item );

                        if( line->IsWire() && line->GetLength() > length )
                        {
                            longestWire = line;
                            length = line->GetLength();
                        }
                    }
                }

                if( longestWire )
                {
                    longestWire->SetOperatingPoint( op );
                    GetCanvas()->GetView()->Update( longestWire );
                }
            }
        }
    }
}


void SCH_EDIT_FRAME::AutoRotateItem( SCH_SCREEN* aScreen, SCH_ITEM* aItem )
{
    if( aItem->Type() == SCH_GLOBAL_LABEL_T || aItem->Type() == SCH_HIER_LABEL_T )
    {
        SCH_LABEL_BASE* label = static_cast<SCH_LABEL_BASE*>( aItem );

        if( label->AutoRotateOnPlacement() )
        {
            SPIN_STYLE spin = aScreen->GetLabelOrientationForPoint( label->GetPosition(), label->GetSpinStyle(),
                                                                    &GetCurrentSheet() );

            if( spin != label->GetSpinStyle() )
            {
                label->SetSpinStyle( spin );

                for( SCH_ITEM* item : aScreen->Items().OfType( SCH_GLOBAL_LABEL_T ) )
                {
                    SCH_LABEL_BASE* otherLabel = static_cast<SCH_LABEL_BASE*>( item );

                    if( otherLabel != label && otherLabel->GetText() == label->GetText() )
                        otherLabel->AutoplaceFields( aScreen, AUTOPLACE_AUTO );
                }
            }
        }
    }
}


void SCH_EDIT_FRAME::updateTitle()
{
    SCH_SCREEN* screen = GetScreen();

    wxCHECK( screen, /* void */ );

    wxString title;

    if( !screen->GetFileName().IsEmpty() )
    {
        wxFileName fn( Prj().AbsolutePath( screen->GetFileName() ) );
        bool       readOnly = false;
        bool       unsaved = false;

        if( fn.IsOk() && screen->FileExists() )
            readOnly = screen->IsReadOnly();
        else
            unsaved = true;

        if( IsContentModified() )
            title = wxT( "*" );

        title += fn.GetName();

        wxString sheetPath = GetCurrentSheet().PathHumanReadable( false, true );

        if( sheetPath != fn.GetName() )
            title += wxString::Format( wxT( " [%s]" ), sheetPath );

        if( readOnly )
            title += wxS( " " ) + _( "[Read Only]" );

        if( unsaved )
            title += wxS( " " ) +  _( "[Unsaved]" );
    }
    else
    {
        title = _( "[no schematic loaded]" );
    }

    title += wxT( " \u2014 " ) + _( "Schematic Editor" );

    SetTitle( title );
}


void SCH_EDIT_FRAME::initScreenZoom()
{
    m_toolManager->RunAction( ACTIONS::zoomFitScreen );

    if( GetScreen() )
        GetScreen()->m_zoomInitialized = true;
}


void SCH_EDIT_FRAME::RecalculateConnections( SCH_COMMIT* aCommit, SCH_CLEANUP_FLAGS aCleanupFlags,
                                             PROGRESS_REPORTER* aProgressReporter )
{
    wxString highlightedConn = GetHighlightedConnection();
    bool     hasHighlightedConn = !highlightedConn.IsEmpty();

    std::function<void( SCH_ITEM* )> changeHandler =
            [&]( SCH_ITEM* aChangedItem ) -> void
            {
                GetCanvas()->GetView()->Update( aChangedItem, KIGFX::REPAINT );

                SCH_CONNECTION* connection = aChangedItem->Connection();

                if( m_highlightedConnChanged )
                    return;

                if( !hasHighlightedConn )
                {
                    // No highlighted connection, but connectivity has changed, so refresh
                    // the list of all nets
                    m_highlightedConnChanged = true;
                }
                else if( connection && (   connection->Name() == highlightedConn
                                        || connection->HasDriverChanged() ) )
                {
                    m_highlightedConnChanged = true;
                }
            };

    Schematic().RecalculateConnections( aCommit, aCleanupFlags,
                                        m_toolManager,
                                        aProgressReporter,
                                        GetCanvas()->GetView(),
                                        &changeHandler,
                                        m_undoList.m_CommandsList.empty() ? nullptr
                                                                          : m_undoList.m_CommandsList.back() );

    GetCanvas()->GetView()->UpdateAllItemsConditionally(
            [&]( KIGFX::VIEW_ITEM* aItem ) -> int
            {
                int             flags = 0;
                SCH_ITEM*       item = dynamic_cast<SCH_ITEM*>( aItem );
                SCH_CONNECTION* connection = item ? item->Connection() : nullptr;

                auto invalidateTextVars =
                        [&flags]( EDA_TEXT* text )
                        {
                            if( text->HasTextVars() )
                            {
                                text->ClearRenderCache();
                                text->ClearBoundingBoxCache();
                                flags |= KIGFX::GEOMETRY | KIGFX::REPAINT;
                            }
                        };

                if( connection && connection->HasDriverChanged() )
                {
                    connection->ClearDriverChanged();
                    flags |= KIGFX::REPAINT;
                }

                if( item )
                {
                    item->RunOnChildren(
                            [&invalidateTextVars]( SCH_ITEM* aChild )
                            {
                                if( EDA_TEXT* text = dynamic_cast<EDA_TEXT*>( aChild ) )
                                    invalidateTextVars( text );
                            },
                            RECURSE_MODE::NO_RECURSE );

                    if( flags & KIGFX::GEOMETRY )
                        GetScreen()->Update( item, false );     // Refresh RTree
                }

                if( EDA_TEXT* text = dynamic_cast<EDA_TEXT*>( aItem ) )
                    invalidateTextVars( text );

                return flags;
            } );

    if( m_highlightedConnChanged
        || !Schematic().ConnectionGraph()->FindFirstSubgraphByName( highlightedConn ) )
    {
        GetToolManager()->RunAction( SCH_ACTIONS::updateNetHighlighting );
        RefreshNetNavigator();
        m_highlightedConnChanged = false;
    }
}


void SCH_EDIT_FRAME::RecomputeIntersheetRefs()
{
    Schematic().RecomputeIntersheetRefs();
}


void SCH_EDIT_FRAME::IntersheetRefUpdate( SCH_GLOBALLABEL* aItem )
{
    GetCanvas()->GetView()->Update( aItem );
}


void SCH_EDIT_FRAME::ShowAllIntersheetRefs( bool aShow )
{
    RecomputeIntersheetRefs();

    GetCanvas()->GetView()->SetLayerVisible( LAYER_INTERSHEET_REFS, aShow );
}


std::unique_ptr<GRID_HELPER> SCH_EDIT_FRAME::MakeGridHelper()
{
    return std::make_unique<EE_GRID_HELPER>( m_toolManager );
}


void SCH_EDIT_FRAME::CommonSettingsChanged( int aFlags )
{
    SCH_BASE_FRAME::CommonSettingsChanged( aFlags );

    SCHEMATIC_SETTINGS& settings = Schematic().Settings();

    settings.m_JunctionSize = GetSchematicJunctionSize();
    settings.m_HopOverScale = GetSchematicHopOverScale();

    ShowAllIntersheetRefs( settings.m_IntersheetRefsShow );

    if( EESCHEMA_SETTINGS* cfg = GetAppSettings<EESCHEMA_SETTINGS>( "eeschema" ) )
    {
        GetGalDisplayOptions().ReadWindowSettings( cfg->m_Window );
        GetRenderSettings()->SetDefaultFont( cfg->m_Appearance.default_font );

        KIGFX::VIEW* view = GetCanvas()->GetView();
        view->SetLayerVisible( LAYER_ERC_ERR, cfg->m_Appearance.show_erc_errors );
        view->SetLayerVisible( LAYER_ERC_WARN, cfg->m_Appearance.show_erc_warnings );
        view->SetLayerVisible( LAYER_ERC_EXCLUSION, cfg->m_Appearance.show_erc_exclusions );
        view->SetLayerVisible( LAYER_OP_VOLTAGES, cfg->m_Appearance.show_op_voltages );
        view->SetLayerVisible( LAYER_OP_CURRENTS, cfg->m_Appearance.show_op_currents );

        GetRenderSettings()->m_ShowPinAltIcons = cfg->m_Appearance.show_pin_alt_icons;

        RefreshOperatingPointDisplay();

        settings.m_TemplateFieldNames.DeleteAllFieldNameTemplates( true /* global */ );

        if( !cfg->m_Drawing.field_names.IsEmpty() )
            settings.m_TemplateFieldNames.AddTemplateFieldNames( cfg->m_Drawing.field_names );
    }

    SCH_SCREEN* screen = GetCurrentSheet().LastScreen();

    for( SCH_ITEM* item : screen->Items() )
    {
        item->ClearCaches();

        if( item->Type() == SCH_LINE_T )
        {
            SCH_LINE* line = static_cast<SCH_LINE*>( item );

            if( line->IsWire() )
                UpdateHopOveredWires( line );
        }
    }

    for( const auto& [ libItemName, libSymbol ] : screen->GetLibSymbols() )
        libSymbol->ClearCaches();

    GetCanvas()->ForceRefresh();

    RecreateToolbars();
    Layout();
    SendSizeEvent();
}


void SCH_EDIT_FRAME::OnPageSettingsChange()
{
    // Store the current zoom level into the current screen before calling
    // DisplayCurrentSheet() that set the zoom to GetScreen()->m_LastZoomLevel
    GetScreen()->m_LastZoomLevel = GetCanvas()->GetView()->GetScale();

    // Rebuild the sheet view (draw area and any other items):
    DisplayCurrentSheet();
}


void SCH_EDIT_FRAME::ShowChangedLanguage()
{
    // call my base class
    SCH_BASE_FRAME::ShowChangedLanguage();

    // tooltips in toolbars
    RecreateToolbars();

    // For some obscure reason, the AUI manager hides the first modified pane.
    // So force show panes
    wxAuiPaneInfo& design_blocks_pane_info = m_auimgr.GetPane( m_designBlocksPane );
    bool panel_shown = design_blocks_pane_info.IsShown();
    design_blocks_pane_info.Caption( _( "Design Blocks" ) );
    design_blocks_pane_info.Show( panel_shown );

    m_auimgr.GetPane( m_hierarchy ).Caption( _( "Schematic Hierarchy" ) );
    m_auimgr.GetPane( m_selectionFilterPanel ).Caption( _( "Selection Filter" ) );
    m_auimgr.GetPane( m_propertiesPanel ).Caption( _( "Properties" ) );
    m_auimgr.GetPane( m_designBlocksPane ).Caption( _( "Design Blocks" ) );
    m_auimgr.GetPane( RemoteSymbolPaneName() ).Caption( _( "Remote Symbols" ) );
    m_auimgr.Update();
    m_hierarchy->UpdateHierarchyTree();

    // status bar
    UpdateMsgPanel();

    updateTitle();

    // This ugly hack is to fix an option(left) toolbar update bug that seems to only affect
    // windows.  See https://bugs.launchpad.net/kicad/+bug/1816492.  For some reason, calling
    // wxWindow::Refresh() does not resolve the issue.  Only a resize event seems to force the
    // toolbar to update correctly.
#if defined( __WXMSW__ )
    PostSizeEvent();
#endif
}


void SCH_EDIT_FRAME::UpdateNetHighlightStatus()
{
    if( !GetHighlightedConnection().IsEmpty() )
    {
        SetStatusText( wxString::Format( _( "Highlighted net: %s" ),
                                         UnescapeString( GetHighlightedConnection() ) ) );
    }
    else
    {
        SetStatusText( wxT( "" ) );
    }
}


void SCH_EDIT_FRAME::SetScreen( BASE_SCREEN* aScreen )
{
    if( m_toolManager )
        m_toolManager->RunAction( ACTIONS::selectionClear );

    SCH_BASE_FRAME::SetScreen( aScreen );
    GetCanvas()->DisplaySheet( static_cast<SCH_SCREEN*>( aScreen ) );

    if( m_toolManager )
        m_toolManager->ResetTools( TOOL_BASE::MODEL_RELOAD );
}


const BOX2I SCH_EDIT_FRAME::GetDocumentExtents( bool aIncludeAllVisible ) const
{
    BOX2I bBoxDoc;

    if( !GetScreen() )
        return bBoxDoc;

    if( aIncludeAllVisible )
    {
        // Get the whole page size and return that
        int sizeX = GetScreen()->GetPageSettings().GetWidthIU( schIUScale.IU_PER_MILS );
        int sizeY = GetScreen()->GetPageSettings().GetHeightIU( schIUScale.IU_PER_MILS );
        bBoxDoc   = BOX2I( VECTOR2I( 0, 0 ), VECTOR2I( sizeX, sizeY ) );
    }
    else
    {
        // Get current drawing-sheet in a form we can compare to an EDA_ITEM
        DS_PROXY_VIEW_ITEM* ds = SCH_BASE_FRAME::GetCanvas()->GetView()->GetDrawingSheet();
        EDA_ITEM*           dsAsItem = static_cast<EDA_ITEM*>( ds );

        // Calc the bounding box of all items on screen except the page border
        for( EDA_ITEM* item : GetScreen()->Items() )
        {
            if( item != dsAsItem ) // Ignore the drawing-sheet itself
                bBoxDoc.Merge( item->GetBoundingBox() );
        }
    }

    return bBoxDoc;
}


bool SCH_EDIT_FRAME::IsContentModified() const
{
    if( !Schematic().HasHierarchy() )
        return false;

    return Schematic().Hierarchy().IsModified();
}


bool SCH_EDIT_FRAME::GetShowAllPins() const
{
    EESCHEMA_SETTINGS* cfg = eeconfig();
    return cfg && cfg->m_Appearance.show_hidden_pins;
}


void SCH_EDIT_FRAME::FocusOnItem( EDA_ITEM* aItem, bool aAllowScroll )
{
    // nullptr will clear the current focus
    if( aItem != nullptr && !aItem->IsSCH_ITEM() )
        return;

    static KIID lastBrightenedItemID( niluuid );

    SCH_ITEM* lastItem = Schematic().ResolveItem( lastBrightenedItemID, nullptr, true );

    if( lastItem && lastItem != aItem )
    {
        lastItem->ClearBrightened();

        UpdateItem( lastItem );
        lastBrightenedItemID = niluuid;
    }

    if( aItem )
    {
        if( !aItem->IsBrightened() )
        {
            aItem->SetBrightened();

            UpdateItem( aItem );
            lastBrightenedItemID = aItem->m_Uuid;
        }

        FocusOnLocation( aItem->GetFocusPosition(), aAllowScroll );
    }
}


wxString SCH_EDIT_FRAME::GetCurrentFileName() const
{
    return Schematic().GetFileName();
}


SELECTION& SCH_EDIT_FRAME::GetCurrentSelection()
{
    return m_toolManager->GetTool<SCH_SELECTION_TOOL>()->GetSelection();
}

void SCH_EDIT_FRAME::onSize( wxSizeEvent& aEvent )
{
    if( IsShown() )
    {
        // We only need this until the frame is done resizing and the final client size is
        // established.
        Unbind( wxEVT_SIZE, &SCH_EDIT_FRAME::onSize, this );
        GetToolManager()->RunAction( ACTIONS::zoomFitScreen );
    }

    // Skip() is called in the base class.
    EDA_DRAW_FRAME::OnSize( aEvent );
}


void SCH_EDIT_FRAME::SaveSymbolToSchematic( const LIB_SYMBOL& aSymbol,
                                            const KIID& aSchematicSymbolUUID )
{
    SCH_SHEET_PATH principalPath;
    SCH_SHEET_LIST sheets = Schematic().Hierarchy();
    SCH_ITEM*      item = sheets.ResolveItem( aSchematicSymbolUUID, &principalPath, true );
    SCH_SYMBOL*    principalSymbol = dynamic_cast<SCH_SYMBOL*>( item );
    SCH_COMMIT     commit( m_toolManager );

    if( !principalSymbol )
        return;

    wxString principalRef;

    if( principalSymbol->IsAnnotated( &principalPath ) )
        principalRef = principalSymbol->GetRef( &principalPath, false );

    std::vector< std::pair<SCH_SYMBOL*, SCH_SHEET_PATH> > allUnits;

    for( const SCH_SHEET_PATH& path : sheets )
    {
        for( SCH_ITEM* candidate : path.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* candidateSymbol = static_cast<SCH_SYMBOL*>( candidate );

            if( candidateSymbol == principalSymbol
                || ( candidateSymbol->IsAnnotated( &path )
                     && candidateSymbol->GetRef( &path, false ) == principalRef ) )
            {
                allUnits.emplace_back( candidateSymbol, path );
            }
        }
    }

    for( auto& [ unit, path ] : allUnits )
    {
        // This needs to be done before the LIB_SYMBOL is changed to prevent stale
        // library symbols in the schematic file.
        path.LastScreen()->Remove( unit );

        if( !unit->IsNew() )
            commit.Modify( unit, path.LastScreen() );

        unit->SetLibSymbol( aSymbol.Flatten().release() );
        unit->UpdateFields( &GetCurrentSheet(),
                            true, /* update style */
                            true, /* update ref */
                            true, /* update other fields */
                            false, /* reset ref */
                            false /* reset other fields */ );

        path.LastScreen()->Append( unit );
        GetCanvas()->GetView()->Update( unit );
    }

    // Clear any orphaned alternate pins.
    for( SCH_PIN* pin : principalSymbol->GetPins() )
    {
        wxString altName = pin->GetAlt();

        if( altName.IsEmpty() )
            continue;

        if( pin->GetAlternates().count( altName ) == 0 )
            pin->SetAlt( wxEmptyString );
    }

    if( !commit.Empty() )
        commit.Push( _( "Save Symbol to Schematic" ) );
}


void SCH_EDIT_FRAME::UpdateItem( EDA_ITEM* aItem, bool isAddOrDelete, bool aUpdateRtree )
{
    SCH_BASE_FRAME::UpdateItem( aItem, isAddOrDelete, aUpdateRtree );

    if( SCH_ITEM* sch_item = dynamic_cast<SCH_ITEM*>( aItem ) )
        sch_item->ClearCaches();
}


void SCH_EDIT_FRAME::DisplayCurrentSheet()
{
    wxCHECK( m_toolManager, /* void */ );

    m_toolManager->RunAction( ACTIONS::cancelInteractive );
    m_toolManager->RunAction( ACTIONS::selectionClear );
    SCH_SCREEN* screen = GetCurrentSheet().LastScreen();

    wxCHECK( screen, /* void */ );

    m_toolManager->RunAction( ACTIONS::selectionClear );

    SCH_BASE_FRAME::SetScreen( screen );

    SetSheetNumberAndCount();   // will also update CurrentScreen()'s sheet number info

    m_toolManager->ResetTools( TOOL_BASE::MODEL_RELOAD );

    // update the references, units, and intersheet-refs
    GetCurrentSheet().UpdateAllScreenReferences();

    // dangling state can also have changed if different units with different pin locations are
    // used
    GetCurrentSheet().LastScreen()->TestDanglingEnds();
    RefreshOperatingPointDisplay();

    SCH_SELECTION_TOOL* selectionTool = m_toolManager->GetTool<SCH_SELECTION_TOOL>();

    wxCHECK( selectionTool, /* void */ );

    auto visit =
            [&]( EDA_ITEM* item )
            {
                if( m_findReplaceDialog
                        && !m_findReplaceData->findString.IsEmpty()
                        && item->Matches( *m_findReplaceData, &GetCurrentSheet() ) )
                {
                    item->SetForceVisible( true );
                    selectionTool->BrightenItem( item );
                }
                else if( item->IsBrightened() )
                {
                    item->SetForceVisible( false );
                    selectionTool->UnbrightenItem( item );
                }
            };

    for( SCH_ITEM* item : screen->Items() )
    {
        visit( item );

        item->RunOnChildren(
                [&]( SCH_ITEM* aChild )
                {
                    visit( aChild );
                },
                RECURSE_MODE::NO_RECURSE );
    }

    if( !screen->m_zoomInitialized )
    {
        initScreenZoom();
    }
    else
    {
        // Set zoom to last used in this screen
        GetCanvas()->GetView()->SetScale( GetScreen()->m_LastZoomLevel );
        GetCanvas()->GetView()->SetCenter( GetScreen()->m_ScrollCenter );
    }

    updateTitle();

    HardRedraw();   // Ensure all items are redrawn (especially the drawing-sheet items)

    // Allow tools to re-add their VIEW_ITEMs after the last call to Clear in HardRedraw
    m_toolManager->ResetTools( TOOL_BASE::MODEL_RELOAD );

    SCH_EDITOR_CONTROL* editTool = m_toolManager->GetTool<SCH_EDITOR_CONTROL>();

    wxCHECK( editTool, /* void */ );

    TOOL_EVENT dummy;
    editTool->UpdateNetHighlighting( dummy );

    m_hierarchy->UpdateHierarchySelection();

    m_schematic->OnSchSheetChanged();
}


DIALOG_BOOK_REPORTER* SCH_EDIT_FRAME::GetSymbolDiffDialog()
{
    if( !m_diffSymbolDialog )
    {
        m_diffSymbolDialog = new DIALOG_BOOK_REPORTER( this, DIFF_SYMBOLS_DIALOG_NAME,
                                                       _( "Compare Symbol with Library" ) );

        m_diffSymbolDialog->m_sdbSizerApply->SetLabel( _( "Update Symbol from Library..." ) );
        m_diffSymbolDialog->m_sdbSizerApply->PostSizeEventToParent();
        m_diffSymbolDialog->m_sdbSizerApply->Show();
    }

    return m_diffSymbolDialog;
}


void SCH_EDIT_FRAME::onCloseSymbolDiffDialog( wxCommandEvent& aEvent )
{
    if( m_diffSymbolDialog && aEvent.GetString() == DIFF_SYMBOLS_DIALOG_NAME )
    {
        if( aEvent.GetId() == wxID_APPLY )
        {
            KIID symbolUUID = m_diffSymbolDialog->GetUserItemID();

            CallAfter(
                    [this, symbolUUID]()
                    {
                        EDA_ITEM* item = ResolveItem( symbolUUID );

                        if( SCH_SYMBOL* symbol = dynamic_cast<SCH_SYMBOL*>( item ) )
                        {
                            m_toolManager->RunAction<EDA_ITEM*>( ACTIONS::selectItem, symbol );

                            DIALOG_CHANGE_SYMBOLS dlg( this, symbol, DIALOG_CHANGE_SYMBOLS::MODE::UPDATE );
                            dlg.ShowQuasiModal();
                        }
                    } );
        }

        m_diffSymbolDialog->Destroy();
        m_diffSymbolDialog = nullptr;
    }
}


DIALOG_ERC* SCH_EDIT_FRAME::GetErcDialog()
{
    if( !m_ercDialog )
        m_ercDialog = new DIALOG_ERC( this );

    return m_ercDialog;
}


void SCH_EDIT_FRAME::onCloseErcDialog( wxCommandEvent& aEvent )
{
    if( m_ercDialog )
    {
        m_ercDialog->Destroy();
        m_ercDialog = nullptr;
    }
}


DIALOG_SYMBOL_FIELDS_TABLE* SCH_EDIT_FRAME::GetSymbolFieldsTableDialog()
{
    if( !m_symbolFieldsTableDialog )
        m_symbolFieldsTableDialog = new DIALOG_SYMBOL_FIELDS_TABLE( this );

    return m_symbolFieldsTableDialog;
}


wxGenericTreeCtrl* SCH_EDIT_FRAME::GetNetNavigator()
{
    return m_netNavigator;
}


void SCH_EDIT_FRAME::onCloseSymbolFieldsTableDialog( wxCommandEvent& aEvent )
{
    if( m_symbolFieldsTableDialog )
    {
        m_symbolFieldsTableDialog->Destroy();
        m_symbolFieldsTableDialog = nullptr;
    }
}


void SCH_EDIT_FRAME::AddSchematicChangeListener( wxEvtHandler* aListener )
{
    auto it = std::find( m_schematicChangeListeners.begin(), m_schematicChangeListeners.end(), aListener );

    // Don't add duplicate listeners.
    if( it == m_schematicChangeListeners.end() )
        m_schematicChangeListeners.push_back( aListener );
}


void SCH_EDIT_FRAME::RemoveSchematicChangeListener( wxEvtHandler* aListener )
{
    auto it = std::find( m_schematicChangeListeners.begin(), m_schematicChangeListeners.end(), aListener );

    // Don't add duplicate listeners.
    if( it != m_schematicChangeListeners.end() )
        m_schematicChangeListeners.erase( it );
}


wxWindow* SCH_EDIT_FRAME::createHighlightedNetNavigator()
{
    wxPanel* panel = new wxPanel( this );

    wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );

    // Create horizontal sizer for search control and gear button
    wxBoxSizer* searchSizer = new wxBoxSizer( wxHORIZONTAL );

    m_netNavigatorFilter = new wxSearchCtrl( panel, wxID_ANY );
    m_netNavigatorFilter->SetDescriptiveText( _( "Filter nets" ) );
    m_netNavigatorFilter->ShowCancelButton( false );
    searchSizer->Add( m_netNavigatorFilter, 1, wxEXPAND | wxRIGHT, FromDIP( 2 ) );

    m_netNavigatorMenuButton = new BITMAP_BUTTON( panel, wxID_ANY );
    m_netNavigatorMenuButton->SetBitmap( KiBitmapBundle( BITMAPS::config ) );
    m_netNavigatorMenuButton->SetPadding( 2 );
    searchSizer->Add( m_netNavigatorMenuButton, 0, wxALIGN_CENTER_VERTICAL );

    sizer->Add( searchSizer, 0, wxEXPAND | wxALL, FromDIP( 2 ) );

    m_netNavigator = new wxGenericTreeCtrl( panel, wxID_ANY, wxPoint( 0, 0 ), FromDIP( wxSize( 160, 250 ) ),
                                            wxTR_DEFAULT_STYLE | wxNO_BORDER );
    sizer->Add( m_netNavigator, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, FromDIP( 2 ) );

    panel->SetSizer( sizer );

    m_netNavigatorFilter->Bind( wxEVT_COMMAND_TEXT_UPDATED, &SCH_EDIT_FRAME::onNetNavigatorFilterChanged, this );
    m_netNavigatorFilter->Bind( wxEVT_KEY_DOWN, &SCH_EDIT_FRAME::onNetNavigatorKey, this );
    m_netNavigator->Bind( wxEVT_KEY_DOWN, &SCH_EDIT_FRAME::onNetNavigatorKey, this );
    m_netNavigator->Bind( wxEVT_TREE_ITEM_MENU, &SCH_EDIT_FRAME::onNetNavigatorItemMenu, this );
    m_netNavigator->Bind( wxEVT_CONTEXT_MENU, &SCH_EDIT_FRAME::onNetNavigatorContextMenu, this );

    m_netNavigatorMenuButton->Bind( wxEVT_LEFT_DOWN,
            [this]( wxMouseEvent& event )
            {
                wxMenu menu;
                wxMenuItem* wildcardItem = menu.AppendRadioItem( ID_NET_NAVIGATOR_SEARCH_WILDCARD,
                                                                  _( "Wildcard Search" ) );
                wxMenuItem* regexItem = menu.AppendRadioItem( ID_NET_NAVIGATOR_SEARCH_REGEX,
                                                              _( "Regex Search" ) );

                EESCHEMA_SETTINGS* cfg = eeconfig();

                if( cfg && cfg->m_AuiPanels.net_nav_search_mode_wildcard )
                    wildcardItem->Check();
                else
                    regexItem->Check();

                PopupMenu( &menu );
            } );

    Bind( wxEVT_MENU, &SCH_EDIT_FRAME::onNetNavigatorMenuCommand, this, ID_NET_NAVIGATOR_EXPAND_ALL );
    Bind( wxEVT_MENU, &SCH_EDIT_FRAME::onNetNavigatorMenuCommand, this, ID_NET_NAVIGATOR_COLLAPSE_ALL );
    Bind( wxEVT_MENU, &SCH_EDIT_FRAME::onNetNavigatorMenuCommand, this, ID_NET_NAVIGATOR_FIND_IN_INSPECTOR );
    Bind( wxEVT_MENU, &SCH_EDIT_FRAME::onNetNavigatorMenuCommand, this, ID_NET_NAVIGATOR_SEARCH_WILDCARD );
    Bind( wxEVT_MENU, &SCH_EDIT_FRAME::onNetNavigatorMenuCommand, this, ID_NET_NAVIGATOR_SEARCH_REGEX );

    return panel;
}


void SCH_EDIT_FRAME::SetHighlightedConnection( const wxString& aConnection,
                                               const NET_NAVIGATOR_ITEM_DATA* aSelection )
{
    bool refreshNetNavigator = aConnection != m_highlightedConn;

    m_highlightedConn = aConnection;

    if( refreshNetNavigator )
        RefreshNetNavigator( aSelection );
}


void SCH_EDIT_FRAME::unitsChangeRefresh()
{
    EDA_DRAW_FRAME::unitsChangeRefresh();

    if( m_netNavigator )
    {
        NET_NAVIGATOR_ITEM_DATA itemData;
        wxTreeItemId selection = m_netNavigator->GetSelection();
        bool refreshSelection = selection.IsOk() && selection != m_netNavigator->GetRootItem();

        if( refreshSelection )
        {
            NET_NAVIGATOR_ITEM_DATA* tmp =
                    dynamic_cast<NET_NAVIGATOR_ITEM_DATA*>( m_netNavigator->GetItemData( selection ) );

            wxCHECK( tmp, /* void */ );
            itemData = *tmp;
        }

        m_netNavigator->DeleteAllItems();
        RefreshNetNavigator( refreshSelection ? &itemData : nullptr );
    }
}


void SCH_EDIT_FRAME::onNetNavigatorFilterChanged( wxCommandEvent& aEvent )
{
    if( !m_netNavigator )
        return;

    wxString newFilter = m_netNavigatorFilter ? m_netNavigatorFilter->GetValue() : wxString();

    if( newFilter == m_netNavigatorFilterValue )
        return;

    m_netNavigatorFilterValue = newFilter;

    NET_NAVIGATOR_ITEM_DATA selectionData;
    NET_NAVIGATOR_ITEM_DATA* selectionPtr = nullptr;

    wxTreeItemId selection = m_netNavigator->GetSelection();

    if( selection.IsOk() )
    {
        if( NET_NAVIGATOR_ITEM_DATA* tmp =
                    dynamic_cast<NET_NAVIGATOR_ITEM_DATA*>( m_netNavigator->GetItemData( selection ) ) )
        {
            selectionData = *tmp;
            selectionPtr = &selectionData;
        }
    }

    RefreshNetNavigator( selectionPtr );

    aEvent.Skip();
}


void SCH_EDIT_FRAME::onNetNavigatorKey( wxKeyEvent& aEvent )
{
    if( aEvent.GetKeyCode() == WXK_ESCAPE )
    {
        // Clear the search string and refresh
        if( m_netNavigatorFilter )
            m_netNavigatorFilter->SetValue( wxEmptyString );

        m_netNavigatorFilterValue = wxEmptyString;

        RefreshNetNavigator();

        // Don't skip the event - we handled it
        return;
    }

    aEvent.Skip();
}



void SCH_EDIT_FRAME::onNetNavigatorItemMenu( wxTreeEvent& aEvent )
{
    showNetNavigatorMenu( aEvent.GetItem() );
}


void SCH_EDIT_FRAME::onNetNavigatorContextMenu( wxContextMenuEvent& aEvent )
{
    if( !m_netNavigator )
        return;

    wxPoint screenPos = aEvent.GetPosition();

    if( screenPos == wxDefaultPosition )
        screenPos = wxGetMousePosition();

    wxPoint      clientPos = m_netNavigator->ScreenToClient( screenPos );
    int          flags = 0;
    wxTreeItemId item = m_netNavigator->HitTest( clientPos, flags );

    showNetNavigatorMenu( item );
}


void SCH_EDIT_FRAME::showNetNavigatorMenu( const wxTreeItemId& aItem )
{
    if( !m_netNavigator )
        return;

    wxMenu menu;

    menu.Append( ID_NET_NAVIGATOR_EXPAND_ALL, _( "Expand All" ) );
    menu.Append( ID_NET_NAVIGATOR_COLLAPSE_ALL, _( "Collapse All" ) );

    wxMenuItem* findInInspector = new wxMenuItem( &menu, ID_NET_NAVIGATOR_FIND_IN_INSPECTOR,
                                                  _( "Find in Net Inspector" ) );
    menu.Append( findInInspector );

    wxString netName;

    if( aItem.IsOk() )
    {
        wxTreeItemId netItem = aItem;

        if( m_netNavigator->GetItemParent( netItem ) != m_netNavigator->GetRootItem() )
        {
            wxTreeItemId parent = m_netNavigator->GetItemParent( netItem );

            while( parent.IsOk() && parent != m_netNavigator->GetRootItem() )
            {
                netItem = parent;
                parent = m_netNavigator->GetItemParent( netItem );
            }

            if( parent == m_netNavigator->GetRootItem() )
            {
                if( wxStringClientData* data =
                            dynamic_cast<wxStringClientData*>( m_netNavigator->GetItemData( netItem ) ) )
                {
                    netName = data->GetData();
                }
            }
        }
        else if( m_netNavigator->GetItemParent( netItem ) == m_netNavigator->GetRootItem() )
        {
            if( wxStringClientData* data =
                        dynamic_cast<wxStringClientData*>( m_netNavigator->GetItemData( netItem ) ) )
            {
                netName = data->GetData();
            }
        }
        else if( !m_highlightedConn.IsEmpty() && netItem == m_netNavigator->GetRootItem() )
        {
            netName = m_highlightedConn;
        }
    }
    else if( !m_highlightedConn.IsEmpty() && m_netNavigator->GetRootItem().IsOk() )
    {
        netName = m_highlightedConn;
    }

    if( netName.IsEmpty() )
    {
        findInInspector->Enable( false );
        m_netNavigatorMenuNetName.clear();
    }
    else
    {
        m_netNavigatorMenuNetName = netName;
    }

    PopupMenu( &menu );
}


void SCH_EDIT_FRAME::onNetNavigatorMenuCommand( wxCommandEvent& aEvent )
{
    if( !m_netNavigator )
        return;

    switch( aEvent.GetId() )
    {
    case ID_NET_NAVIGATOR_EXPAND_ALL:
        m_netNavigator->ExpandAll();
        break;

    case ID_NET_NAVIGATOR_COLLAPSE_ALL:
        m_netNavigator->CollapseAll();

        if( m_netNavigator->GetRootItem().IsOk() )
            m_netNavigator->Expand( m_netNavigator->GetRootItem() );
        break;

    case ID_NET_NAVIGATOR_FIND_IN_INSPECTOR:
        if( !m_netNavigatorMenuNetName.IsEmpty() )
            FindNetInInspector( m_netNavigatorMenuNetName );
        break;

    case ID_NET_NAVIGATOR_SEARCH_WILDCARD:
    case ID_NET_NAVIGATOR_SEARCH_REGEX:
        if( EESCHEMA_SETTINGS* cfg = eeconfig() )
        {
            cfg->m_AuiPanels.net_nav_search_mode_wildcard = ( aEvent.GetId() == ID_NET_NAVIGATOR_SEARCH_WILDCARD );

            // Refresh the navigator with current filter
            RefreshNetNavigator();
        }

        break;

    default:
        aEvent.Skip();
        return;
    }

    m_netNavigatorMenuNetName.clear();

    aEvent.Skip( false );
}


void SCH_EDIT_FRAME::updateSelectionFilterVisbility()
{
    wxAuiPaneInfo& hierarchyPane = m_auimgr.GetPane( SchematicHierarchyPaneName() );
    wxAuiPaneInfo& netNavigatorPane = m_auimgr.GetPane( NetNavigatorPaneName() );
    wxAuiPaneInfo& propertiesPane = m_auimgr.GetPane( PropertiesPaneName() );
    wxAuiPaneInfo& selectionFilterPane = m_auimgr.GetPane( wxS( "SelectionFilter" ) );

    // Don't give the selection filter its own visibility controls; instead show it if
    // anything else is visible
    bool showFilter = ( hierarchyPane.IsShown() && hierarchyPane.IsDocked() )
                      || ( netNavigatorPane.IsShown() && netNavigatorPane.IsDocked() )
                      || ( propertiesPane.IsShown() && propertiesPane.IsDocked() );

    selectionFilterPane.Show( showFilter );
}


#ifdef KICAD_IPC_API
void SCH_EDIT_FRAME::onPluginAvailabilityChanged( wxCommandEvent& aEvt )
{
    wxLogTrace( traceApi, "SCH frame: EDA_EVT_PLUGIN_AVAILABILITY_CHANGED" );
    RecreateToolbars();
    aEvt.Skip();
}
#endif


void SCH_EDIT_FRAME::ToggleSearch()
{
    EESCHEMA_SETTINGS* cfg = eeconfig();

    // Ensure m_show_search is up to date (the pane can be closed outside the menu)
    m_show_search = m_auimgr.GetPane( SearchPaneName() ).IsShown();

    m_show_search = !m_show_search;

    wxAuiPaneInfo& searchPaneInfo = m_auimgr.GetPane( SearchPaneName() );
    searchPaneInfo.Show( m_show_search );

    if( m_show_search )
    {
        searchPaneInfo.Direction( cfg->m_AuiPanels.search_panel_dock_direction );

        if( cfg->m_AuiPanels.search_panel_dock_direction == wxAUI_DOCK_TOP
            || cfg->m_AuiPanels.search_panel_dock_direction == wxAUI_DOCK_BOTTOM )
        {
            SetAuiPaneSize( m_auimgr, searchPaneInfo, -1, cfg->m_AuiPanels.search_panel_height );
        }
        else if( cfg->m_AuiPanels.search_panel_dock_direction == wxAUI_DOCK_LEFT
                 || cfg->m_AuiPanels.search_panel_dock_direction == wxAUI_DOCK_RIGHT )
        {
            SetAuiPaneSize( m_auimgr, searchPaneInfo, cfg->m_AuiPanels.search_panel_width, -1 );
        }

        m_searchPane->FocusSearch();
        m_searchPane->RefreshSearch();
    }
    else
    {
        cfg->m_AuiPanels.search_panel_height = m_searchPane->GetSize().y;
        cfg->m_AuiPanels.search_panel_width = m_searchPane->GetSize().x;
        cfg->m_AuiPanels.search_panel_dock_direction = searchPaneInfo.dock_direction;
        m_auimgr.Update();
    }
}


void SCH_EDIT_FRAME::ToggleProperties()
{
    if( !m_propertiesPanel )
        return;

    bool show = !m_propertiesPanel->IsShownOnScreen();

    wxAuiPaneInfo& propertiesPaneInfo = m_auimgr.GetPane( PropertiesPaneName() );
    propertiesPaneInfo.Show( show );

    updateSelectionFilterVisbility();

    EESCHEMA_SETTINGS* settings = eeconfig();

    if( show )
    {
        SetAuiPaneSize( m_auimgr, propertiesPaneInfo, settings->m_AuiPanels.properties_panel_width, -1 );
    }
    else
    {
        settings->m_AuiPanels.properties_panel_width = m_propertiesPanel->GetSize().x;
        m_auimgr.Update();
    }
}


void SCH_EDIT_FRAME::ToggleSchematicHierarchy()
{
    EESCHEMA_SETTINGS* cfg = eeconfig();

    wxCHECK( cfg, /* void */ );

    wxAuiPaneInfo&     hierarchy_pane = m_auimgr.GetPane( SchematicHierarchyPaneName() );

    hierarchy_pane.Show( !hierarchy_pane.IsShown() );

    updateSelectionFilterVisbility();

    if( hierarchy_pane.IsShown() )
    {
        if( hierarchy_pane.IsFloating() )
        {
            hierarchy_pane.FloatingSize( cfg->m_AuiPanels.hierarchy_panel_float_width,
                                         cfg->m_AuiPanels.hierarchy_panel_float_height );
            m_auimgr.Update();
        }
        else if( cfg->m_AuiPanels.hierarchy_panel_docked_width > 0 )
        {
            // SetAuiPaneSize also updates m_auimgr
            SetAuiPaneSize( m_auimgr, hierarchy_pane, cfg->m_AuiPanels.hierarchy_panel_docked_width, -1 );
        }
    }
    else
    {
        if( hierarchy_pane.IsFloating() )
        {
            cfg->m_AuiPanels.hierarchy_panel_float_width  = hierarchy_pane.floating_size.x;
            cfg->m_AuiPanels.hierarchy_panel_float_height = hierarchy_pane.floating_size.y;
        }
        else
        {
            cfg->m_AuiPanels.hierarchy_panel_docked_width = m_hierarchy->GetSize().x;
        }

        m_auimgr.Update();
    }
}


void SCH_EDIT_FRAME::ToggleLibraryTree()
{
    EESCHEMA_SETTINGS* cfg = eeconfig();

    wxCHECK( cfg, /* void */ );

    wxAuiPaneInfo& db_library_pane = m_auimgr.GetPane( DesignBlocksPaneName() );

    db_library_pane.Show( !db_library_pane.IsShown() );

    if( db_library_pane.IsShown() )
    {
        if( db_library_pane.IsFloating() )
        {
            db_library_pane.FloatingSize( cfg->m_AuiPanels.design_blocks_panel_float_width,
                                          cfg->m_AuiPanels.design_blocks_panel_float_height );
            m_auimgr.Update();
        }
        else if( cfg->m_AuiPanels.design_blocks_panel_docked_width > 0 )
        {
            // SetAuiPaneSize also updates m_auimgr
            SetAuiPaneSize( m_auimgr, db_library_pane,
                            cfg->m_AuiPanels.design_blocks_panel_docked_width, -1 );
        }
    }
    else
    {
        if( db_library_pane.IsFloating() )
        {
            cfg->m_AuiPanels.design_blocks_panel_float_width  = db_library_pane.floating_size.x;
            cfg->m_AuiPanels.design_blocks_panel_float_height = db_library_pane.floating_size.y;
        }
        else
        {
            cfg->m_AuiPanels.design_blocks_panel_docked_width = m_designBlocksPane->GetSize().x;
        }

        m_auimgr.Update();
    }
}


void SCH_EDIT_FRAME::ToggleRemoteSymbolPanel()
{
    EESCHEMA_SETTINGS* cfg = eeconfig();

    wxCHECK( cfg, /* void */ );

    wxAuiPaneInfo& remotePane = m_auimgr.GetPane( RemoteSymbolPaneName() );

    remotePane.Show( !remotePane.IsShown() );

    if( remotePane.IsShown() )
    {
        if( remotePane.IsFloating() )
        {
            remotePane.FloatingSize( cfg->m_AuiPanels.remote_symbol_panel_float_width,
                                     cfg->m_AuiPanels.remote_symbol_panel_float_height );
            m_auimgr.Update();
        }
        else if( cfg->m_AuiPanels.remote_symbol_panel_docked_width > 0 )
        {
            SetAuiPaneSize( m_auimgr, remotePane,
                            cfg->m_AuiPanels.remote_symbol_panel_docked_width, -1 );
        }
    }
    else
    {
        if( remotePane.IsFloating() )
        {
            cfg->m_AuiPanels.remote_symbol_panel_float_width  = remotePane.floating_size.x;
            cfg->m_AuiPanels.remote_symbol_panel_float_height = remotePane.floating_size.y;
        }
        else if( m_remoteSymbolPane )
        {
            cfg->m_AuiPanels.remote_symbol_panel_docked_width = m_remoteSymbolPane->GetSize().x;
        }

        m_auimgr.Update();
    }
}


void SCH_EDIT_FRAME::SetSchematic( SCHEMATIC* aSchematic )
{
    wxCHECK( aSchematic, /* void */ );

    if( m_schematic )
        m_schematic->SetProject( nullptr );

    aSchematic->SetProject( &Prj() );
    delete m_schematic;

    m_schematic = aSchematic;
    m_schematic->SetSchematicHolder( this );

    KIGFX::SCH_VIEW* view = GetCanvas()->GetView();
    static_cast<KIGFX::SCH_PAINTER*>( view->GetPainter() )->SetSchematic( m_schematic );
    m_toolManager->SetEnvironment( m_schematic, GetCanvas()->GetView(), GetCanvas()->GetViewControls(), config(),
                                   this );
}


void SCH_EDIT_FRAME::AddVariant()
{
    if( !m_currentVariantCtrl )
        return;

    m_currentVariantCtrl->SetSelection( m_currentVariantCtrl->GetCount() - 1 );

    wxCommandEvent dummy( wxEVT_CHOICE, ID_TOOLBAR_SCH_SELECT_VARAIANT );
    onVariantSelected( dummy );
}


void SCH_EDIT_FRAME::RemoveVariant()
{
    if( !m_currentVariantCtrl )
        return;

    wxArrayString choices = Schematic().GetVariantNamesForUI();

    // Default variant cannot be removed.
    choices.RemoveAt( 0 );

    // wxSingleChoiceDialog will ellipsize the title bar if the contents aren't wide enough.  The set
    // of spaces in the control label are to prevent this.
    wxSingleChoiceDialog dlg( this, _( "Select variant name to remove:" ) + wxS( "                " ),
                              _( "Remove Design Variant" ), choices );
    dlg.Layout();

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    wxString variantName = dlg.GetStringSelection();

    if( variantName.IsEmpty() )
        return;

    SCH_COMMIT commit( this );
    Schematic().DeleteVariant( variantName, &commit );

    if( !commit.Empty() )
    {
        commit.Push( wxString::Format( wxS( "Delete variant '%s'" ), variantName ) );
        OnModify();
    }

    if( Schematic().GetCurrentVariant() == variantName )
        SetCurrentVariant( wxEmptyString );

    UpdateVariantSelectionCtrl( Schematic().GetVariantNamesForUI() );

    GetCanvas()->Refresh();
}


bool SCH_EDIT_FRAME::doAutoSave()
{
    // Delegate to base auto-save behavior (commits pending local history) for now.
    return EDA_BASE_FRAME::doAutoSave();
}
