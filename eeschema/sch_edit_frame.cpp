/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2022 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <base_units.h>
#include <bitmaps.h>
#include <symbol_library.h>
#include <confirm.h>
#include <connection_graph.h>
#include <dialogs/dialog_schematic_find.h>
#include <eeschema_id.h>
#include <executable_names.h>
#include <gestfich.h>
#include <hierarch.h>
#include <dialogs/html_message_box.h>
#include <ignore.h>
#include <invoke_sch_dialog.h>
#include <string_utils.h>
#include <kiface_base.h>
#include <kiplatform/app.h>
#include <kiway.h>
#include <symbol_edit_frame.h>
#include <symbol_viewer_frame.h>
#include <pgm_base.h>
#include <profile.h>
#include <project.h>
#include <project/project_file.h>
#include <project/net_settings.h>
#include <dialog_erc.h>
#include <python_scripting.h>
#include <sch_edit_frame.h>
#include <sch_painter.h>
#include <sch_sheet.h>
#include <sch_marker.h>
#include <schematic.h>
#include <settings/settings_manager.h>
#include <advanced_config.h>
#include <sim/sim_plot_frame.h>
#include <sim/spice_settings.h>
#include <tool/action_manager.h>
#include <tool/action_toolbar.h>
#include <tool/common_control.h>
#include <tool/common_tools.h>
#include <tool/picker_tool.h>
#include <tool/selection.h>
#include <tool/tool_dispatcher.h>
#include <tool/tool_manager.h>
#include <tool/zoom_tool.h>
#include <tools/ee_actions.h>
#include <tools/ee_inspection_tool.h>
#include <tools/ee_point_editor.h>
#include <tools/ee_selection_tool.h>
#include <tools/sch_drawing_tools.h>
#include <tools/sch_edit_tool.h>
#include <tools/sch_editor_conditions.h>
#include <tools/sch_editor_control.h>
#include <tools/sch_line_wire_bus_tool.h>
#include <tools/sch_move_tool.h>
#include <tools/sch_navigate_tool.h>
#include <view/view_controls.h>
#include <widgets/infobar.h>
#include <wildcards_and_files_ext.h>
#include <wx/cmdline.h>
#include <wx/app.h>
#include <wx/filedlg.h>
#include <wx/socket.h>
#include <widgets/wx_aui_utils.h>

#include <gal/graphics_abstraction_layer.h>
#include <drawing_sheet/ds_proxy_view_item.h>


BEGIN_EVENT_TABLE( SCH_EDIT_FRAME, EDA_DRAW_FRAME )
    EVT_SOCKET( ID_EDA_SOCKET_EVENT_SERV, EDA_DRAW_FRAME::OnSockRequestServer )
    EVT_SOCKET( ID_EDA_SOCKET_EVENT, EDA_DRAW_FRAME::OnSockRequest )

    EVT_SIZE( SCH_EDIT_FRAME::OnSize )

    EVT_MENU_RANGE( ID_FILE1, ID_FILEMAX, SCH_EDIT_FRAME::OnLoadFile )
    EVT_MENU( ID_FILE_LIST_CLEAR, SCH_EDIT_FRAME::OnClearFileHistory )

    EVT_MENU( ID_APPEND_PROJECT, SCH_EDIT_FRAME::OnAppendProject )
    EVT_MENU( ID_IMPORT_NON_KICAD_SCH, SCH_EDIT_FRAME::OnImportProject )

    EVT_MENU( wxID_EXIT, SCH_EDIT_FRAME::OnExit )
    EVT_MENU( wxID_CLOSE, SCH_EDIT_FRAME::OnExit )

    EVT_MENU( ID_GRID_SETTINGS, SCH_BASE_FRAME::OnGridSettings )
END_EVENT_TABLE()


SCH_EDIT_FRAME::SCH_EDIT_FRAME( KIWAY* aKiway, wxWindow* aParent ) :
        SCH_BASE_FRAME( aKiway, aParent, FRAME_SCH, wxT( "Eeschema" ), wxDefaultPosition,
                        wxDefaultSize, KICAD_DEFAULT_DRAWFRAME_STYLE, SCH_EDIT_FRAME_NAME ),
        m_highlightedConn( nullptr ),
        m_item_to_repeat( nullptr )
{
    m_maximizeByDefault = true;
    m_schematic = new SCHEMATIC( nullptr );

    m_showBorderAndTitleBlock = true;   // true to show sheet references
    m_showHierarchy = true;
    m_supportsAutoSave = true;
    m_aboutTitle = _( "KiCad Schematic Editor" );

    m_findReplaceDialog = nullptr;

    // Give an icon
    wxIcon icon;
    wxIconBundle icon_bundle;

    icon.CopyFromBitmap( KiBitmap( BITMAPS::icon_eeschema ) );
    icon_bundle.AddIcon( icon );
    icon.CopyFromBitmap( KiBitmap( BITMAPS::icon_eeschema_32 ) );
    icon_bundle.AddIcon( icon );
    icon.CopyFromBitmap( KiBitmap( BITMAPS::icon_eeschema_16 ) );
    icon_bundle.AddIcon( icon );

    SetIcons( icon_bundle );

    LoadSettings( eeconfig() );

    // NB: also links the schematic to the loaded project
    CreateScreens();

    setupTools();
    setupUIConditions();
    ReCreateMenuBar();
    ReCreateHToolbar();
    ReCreateVToolbar();
    ReCreateOptToolbar();

    m_hierarchy = new HIERARCHY_NAVIG_PANEL( this );

    // Initialize common print setup dialog settings.
    m_pageSetupData.GetPrintData().SetPrintMode( wxPRINT_MODE_PRINTER );
    m_pageSetupData.GetPrintData().SetQuality( wxPRINT_QUALITY_MEDIUM );
    m_pageSetupData.GetPrintData().SetBin( wxPRINTBIN_AUTO );
    m_pageSetupData.GetPrintData().SetNoCopies( 1 );

    m_auimgr.SetManagedWindow( this );

    CreateInfoBar();
    m_auimgr.AddPane( m_mainToolBar, EDA_PANE().HToolbar().Name( "MainToolbar" )
                      .Top().Layer( 6 ) );
    m_auimgr.AddPane( m_optionsToolBar, EDA_PANE().VToolbar().Name( "OptToolbar" )
                      .Left().Layer( 4 ) );
    m_auimgr.AddPane( m_hierarchy, EDA_PANE().Palette().Name( SchematicHierarchyPaneName() )
                      .Caption( _("Schematic Hierarchy") )
                      .Left().Layer( 3 )
                      .TopDockable( false )
                      .BottomDockable( false )
                      .CloseButton( true )
                      .MinSize(120, -1)
                      .BestSize(200, -1)
                      .FloatingSize( 200, 80 )
                      .Show( false )
                      );
    m_auimgr.AddPane( m_drawToolBar, EDA_PANE().VToolbar().Name( "ToolsToolbar" )
                      .Right().Layer( 2 ) );
    m_auimgr.AddPane( GetCanvas(), EDA_PANE().Canvas().Name( "DrawFrame" )
                      .Center() );
    m_auimgr.AddPane( m_messagePanel, EDA_PANE().Messages().Name( "MsgPanel" )
                      .Bottom().Layer( 6 ) );

    wxAuiPaneInfo& hierarchy_pane = m_auimgr.GetPane( SchematicHierarchyPaneName() );

    if( eeconfig() )
    {
        if( eeconfig()->m_AuiPanels.hierarchy_panel_float_width > 0
            && eeconfig()->m_AuiPanels.hierarchy_panel_float_height > 0 )
        {
            hierarchy_pane.FloatingSize( eeconfig()->m_AuiPanels.hierarchy_panel_float_width,
                                         eeconfig()->m_AuiPanels.hierarchy_panel_float_height );
        }

        if( eeconfig()->m_AuiPanels.hierarchy_panel_docked_width > 0 )
        {
            hierarchy_pane.BestSize( eeconfig()->m_AuiPanels.hierarchy_panel_docked_width, -1);
            SetAuiPaneSize( m_auimgr, hierarchy_pane,
                            eeconfig()->m_AuiPanels.hierarchy_panel_docked_width, -1 );
        }
    }

    FinishAUIInitialization();

    resolveCanvasType();
    SwitchCanvas( m_canvasType );

    LoadProjectSettings();

    initScreenZoom();

    m_hierarchy->Connect( wxEVT_SIZE,
                          wxSizeEventHandler( SCH_EDIT_FRAME::OnResizeHierarchyNavigator ),
                          NULL, this );


    // This is used temporarily to fix a client size issue on GTK that causes zoom to fit
    // to calculate the wrong zoom size.  See SCH_EDIT_FRAME::onSize().
    Bind( wxEVT_SIZE, &SCH_EDIT_FRAME::onSize, this );

    if( GetCanvas() )
    {
        GetCanvas()->GetGAL()->SetAxesEnabled( false );

        if( auto p = dynamic_cast<KIGFX::SCH_PAINTER*>( GetCanvas()->GetView()->GetPainter() ) )
            p->SetSchematic( m_schematic );
    }

    setupUnits( eeconfig() );

    // Net list generator
    DefaultExecFlags();

    UpdateTitle();
    m_toolManager->GetTool<SCH_NAVIGATE_TOOL>()->ResetHistory();

    // Default shutdown reason until a file is loaded
    KIPLATFORM::APP::SetShutdownBlockReason( this, _( "New schematic file is unsaved" ) );

    // Ensure the window is on top
    Raise();

    // Now every sizes are fixed, set the initial hierarchy_pane floating position
    // to the left top corner of the canvas
    wxPoint canvas_pos = GetCanvas()->GetScreenPosition();
    hierarchy_pane.FloatingPosition( canvas_pos.x + 10, canvas_pos.y + 10 );

    if( eeconfig() && eeconfig()->m_AuiPanels.schematic_hierarchy_float )
        hierarchy_pane.Float();

    hierarchy_pane.Show( m_showHierarchy );
}


SCH_EDIT_FRAME::~SCH_EDIT_FRAME()
{

    m_hierarchy->Disconnect( wxEVT_SIZE,
                          wxSizeEventHandler( SCH_EDIT_FRAME::OnResizeHierarchyNavigator ),
                          NULL, this );
    // Ensure m_canvasType is up to date, to save it in config
    m_canvasType = GetCanvas()->GetBackend();

    // Close modeless dialogs
    wxWindow* open_dlg = wxWindow::FindWindowByName( DIALOG_ERC_WINDOW_NAME );

    if( open_dlg )
        open_dlg->Close( true );

    // Shutdown all running tools
    if( m_toolManager )
    {
        m_toolManager->ShutdownAllTools();
        delete m_toolManager;
        m_toolManager = nullptr;
    }

    delete m_item_to_repeat;        // we own the cloned object, see this->SaveCopyForRepeatItem()

    SetScreen( nullptr );

    delete m_schematic;
    m_schematic = nullptr;

    // Close the project if we are standalone, so it gets cleaned up properly
    if( Kiface().IsSingle() )
    {
        try
        {
            GetSettingsManager()->UnloadProject( &Prj(), false );
        }
        catch( const nlohmann::detail::type_error& exc )
        {
            // This may be overkill and could be an assertion but we are more likely to
            // find any settings manager errors this way.
            wxLogError( wxT( "Settings exception '%s' occurred." ), exc.what() );
        }
    }

    delete m_hierarchy;
}


void SCH_EDIT_FRAME::OnResizeHierarchyNavigator( wxSizeEvent& aEvent )
{
    aEvent.Skip();

    // Called when resizing the Hierarchy Navigator panel
    // Store the current pane size
    // It allows to retrieve the last defined pane size when switching between
    // docked and floating pane state
    // Note: *DO NOT* call m_auimgr.Update() here: it crashes Kicad at leat on Windows

    EESCHEMA_SETTINGS* cfg = dynamic_cast<EESCHEMA_SETTINGS*>( Kiface().KifaceSettings() );
    wxAuiPaneInfo&     hierarchy_pane = m_auimgr.GetPane( SchematicHierarchyPaneName() );

    if( cfg && m_hierarchy->IsShown() )
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
    m_actions = new EE_ACTIONS();
    m_toolDispatcher = new TOOL_DISPATCHER( m_toolManager );

    // Register tools
    m_toolManager->RegisterTool( new COMMON_CONTROL );
    m_toolManager->RegisterTool( new COMMON_TOOLS );
    m_toolManager->RegisterTool( new ZOOM_TOOL );
    m_toolManager->RegisterTool( new EE_SELECTION_TOOL );
    m_toolManager->RegisterTool( new PICKER_TOOL );
    m_toolManager->RegisterTool( new SCH_DRAWING_TOOLS );
    m_toolManager->RegisterTool( new SCH_LINE_WIRE_BUS_TOOL );
    m_toolManager->RegisterTool( new SCH_MOVE_TOOL );
    m_toolManager->RegisterTool( new SCH_EDIT_TOOL );
    m_toolManager->RegisterTool( new EE_INSPECTION_TOOL );
    m_toolManager->RegisterTool( new SCH_EDITOR_CONTROL );
    m_toolManager->RegisterTool( new EE_POINT_EDITOR );
    m_toolManager->RegisterTool( new SCH_NAVIGATE_TOOL );
    m_toolManager->InitTools();

    // Run the selection tool, it is supposed to be always active
    m_toolManager->RunAction( EE_ACTIONS::selectionActivate );

    GetCanvas()->SetEventDispatcher( m_toolDispatcher );
}


void SCH_EDIT_FRAME::setupUIConditions()
{
    SCH_BASE_FRAME::setupUIConditions();

    ACTION_MANAGER*   mgr = m_toolManager->GetActionManager();
    SCH_EDITOR_CONDITIONS cond( this );

    wxASSERT( mgr );

    auto hasElements =
            [ this ] ( const SELECTION& aSel )
            {
                return GetScreen() &&
                        ( !GetScreen()->Items().empty() || !SELECTION_CONDITIONS::Idle( aSel ) );
            };

    auto hierarchyNavigatorCond =
            [ this ] ( const SELECTION& aSel )
            {
                return m_auimgr.GetPane( SchematicHierarchyPaneName() ).IsShown();
            };


#define ENABLE( x ) ACTION_CONDITIONS().Enable( x )
#define CHECK( x )  ACTION_CONDITIONS().Check( x )

    mgr->SetConditions( ACTIONS::save,                ENABLE( SELECTION_CONDITIONS::ShowAlways ) );
    mgr->SetConditions( ACTIONS::undo,                ENABLE( cond.UndoAvailable() ) );
    mgr->SetConditions( ACTIONS::redo,                ENABLE( cond.RedoAvailable() ) );

    mgr->SetConditions( EE_ACTIONS::showHierarchy,    CHECK( hierarchyNavigatorCond ) );

    mgr->SetConditions( ACTIONS::toggleGrid,          CHECK( cond.GridVisible() ) );
    mgr->SetConditions( ACTIONS::toggleCursorStyle,   CHECK( cond.FullscreenCursor() ) );
    mgr->SetConditions( ACTIONS::millimetersUnits,
                        CHECK( cond.Units( EDA_UNITS::MILLIMETRES ) ) );
    mgr->SetConditions( ACTIONS::inchesUnits,         CHECK( cond.Units( EDA_UNITS::INCHES ) ) );
    mgr->SetConditions( ACTIONS::milsUnits,           CHECK( cond.Units( EDA_UNITS::MILS ) ) );

    mgr->SetConditions( EE_ACTIONS::lineModeFree,     CHECK( cond.LineMode( LINE_MODE::LINE_MODE_FREE ) ) );
    mgr->SetConditions( EE_ACTIONS::lineMode90,       CHECK( cond.LineMode( LINE_MODE::LINE_MODE_90 ) ) );
    mgr->SetConditions( EE_ACTIONS::lineMode45,       CHECK( cond.LineMode( LINE_MODE::LINE_MODE_45 ) ) );

    mgr->SetConditions( ACTIONS::cut,                 ENABLE( hasElements ) );
    mgr->SetConditions( ACTIONS::copy,                ENABLE( hasElements ) );
    mgr->SetConditions( ACTIONS::paste,               ENABLE( SELECTION_CONDITIONS::Idle ) );
    mgr->SetConditions( ACTIONS::pasteSpecial,        ENABLE( SELECTION_CONDITIONS::Idle ) );
    mgr->SetConditions( ACTIONS::doDelete,            ENABLE( hasElements ) );
    mgr->SetConditions( ACTIONS::duplicate,           ENABLE( hasElements ) );
    mgr->SetConditions( ACTIONS::selectAll,           ENABLE( hasElements ) );

    mgr->SetConditions( EE_ACTIONS::rotateCW,         ENABLE( hasElements ) );
    mgr->SetConditions( EE_ACTIONS::rotateCCW,        ENABLE( hasElements ) );
    mgr->SetConditions( EE_ACTIONS::mirrorH,          ENABLE( hasElements ) );
    mgr->SetConditions( EE_ACTIONS::mirrorV,          ENABLE( hasElements ) );

    mgr->SetConditions( ACTIONS::zoomTool,
                        CHECK( cond.CurrentTool( ACTIONS::zoomTool ) ) );
    mgr->SetConditions( ACTIONS::selectionTool,
                        CHECK( cond.CurrentTool( ACTIONS::selectionTool ) ) );

    if( SCRIPTING::IsWxAvailable() )
    {
        mgr->SetConditions( EE_ACTIONS::showPythonConsole,
                            CHECK( cond.ScriptingConsoleVisible() ) );
    }

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

    auto showAnnotateAutomaticallyCond =
            [this]( const SELECTION& )
            {
                EESCHEMA_SETTINGS* cfg = eeconfig();
                return cfg && cfg->m_AnnotatePanel.automatic;
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
                return m_toolManager->GetTool<SCH_NAVIGATE_TOOL>()->CanGoUp();
            };

    auto navHistoryHasForward =
            [this]( const SELECTION& aSel )
            {
                return m_toolManager->GetTool<SCH_NAVIGATE_TOOL>()->CanGoForward();
            };

    auto navHistoryHasBackward =
            [this]( const SELECTION& aSel )
            {
                return m_toolManager->GetTool<SCH_NAVIGATE_TOOL>()->CanGoBack();
            };

    auto navSchematicHasPreviousSheet =
            [this]( const SELECTION& aSel )
            {
                return m_toolManager->GetTool<SCH_NAVIGATE_TOOL>()->CanGoPrevious();
            };

    auto navSchematicHasNextSheet =
            [this]( const SELECTION& aSel )
            {
                return m_toolManager->GetTool<SCH_NAVIGATE_TOOL>()->CanGoNext();
            };

    mgr->SetConditions( EE_ACTIONS::leaveSheet,          ENABLE( belowRootSheetCondition ) );
    mgr->SetConditions( EE_ACTIONS::navigateUp,          ENABLE( belowRootSheetCondition ) );
    mgr->SetConditions( EE_ACTIONS::navigateForward,     ENABLE( navHistoryHasForward ) );
    mgr->SetConditions( EE_ACTIONS::navigateBack,        ENABLE( navHistoryHasBackward ) );
    mgr->SetConditions( EE_ACTIONS::navigatePrevious,    ENABLE( navSchematicHasPreviousSheet ) );
    mgr->SetConditions( EE_ACTIONS::navigateNext,        ENABLE( navSchematicHasNextSheet ) );
    mgr->SetConditions( EE_ACTIONS::remapSymbols,        ENABLE( remapSymbolsCondition ) );
    mgr->SetConditions( EE_ACTIONS::toggleHiddenPins,    CHECK( showHiddenPinsCond ) );
    mgr->SetConditions( EE_ACTIONS::toggleHiddenFields,  CHECK( showHiddenFieldsCond ) );
    mgr->SetConditions( EE_ACTIONS::toggleERCErrors,     CHECK( showERCErrorsCond ) );
    mgr->SetConditions( EE_ACTIONS::toggleERCWarnings,   CHECK( showERCWarningsCond ) );
    mgr->SetConditions( EE_ACTIONS::toggleERCExclusions, CHECK( showERCExclusionsCond ) );
    mgr->SetConditions( EE_ACTIONS::toggleAnnotateAuto,  CHECK( showAnnotateAutomaticallyCond ) );
    mgr->SetConditions( ACTIONS::toggleBoundingBoxes,    CHECK( cond.BoundingBoxes() ) );


#define CURRENT_TOOL( action ) mgr->SetConditions( action, CHECK( cond.CurrentTool( action ) ) )

    CURRENT_TOOL( ACTIONS::deleteTool );
    CURRENT_TOOL( EE_ACTIONS::highlightNetTool );
    CURRENT_TOOL( EE_ACTIONS::placeSymbol );
    CURRENT_TOOL( EE_ACTIONS::placePower );
    CURRENT_TOOL( EE_ACTIONS::drawWire );
    CURRENT_TOOL( EE_ACTIONS::drawBus );
    CURRENT_TOOL( EE_ACTIONS::placeBusWireEntry );
    CURRENT_TOOL( EE_ACTIONS::placeNoConnect );
    CURRENT_TOOL( EE_ACTIONS::placeJunction );
    CURRENT_TOOL( EE_ACTIONS::placeLabel );
    CURRENT_TOOL( EE_ACTIONS::placeClassLabel );
    CURRENT_TOOL( EE_ACTIONS::placeGlobalLabel );
    CURRENT_TOOL( EE_ACTIONS::placeHierLabel );
    CURRENT_TOOL( EE_ACTIONS::drawSheet );
    CURRENT_TOOL( EE_ACTIONS::importSheetPin );
    CURRENT_TOOL( EE_ACTIONS::drawRectangle );
    CURRENT_TOOL( EE_ACTIONS::drawCircle );
    CURRENT_TOOL( EE_ACTIONS::drawArc );
    CURRENT_TOOL( EE_ACTIONS::drawLines );
    CURRENT_TOOL( EE_ACTIONS::placeSchematicText );
    CURRENT_TOOL( EE_ACTIONS::drawTextBox );
    CURRENT_TOOL( EE_ACTIONS::placeImage );

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
        delete m_item_to_repeat;

        m_item_to_repeat = (SCH_ITEM*) aItem->Clone();

        // Clone() preserves the flags, we want 'em cleared.
        m_item_to_repeat->ClearFlags();
    }
}


EDA_ITEM* SCH_EDIT_FRAME::GetItem( const KIID& aId ) const
{
    return Schematic().GetSheets().GetItem( aId );
}


void SCH_EDIT_FRAME::SetSheetNumberAndCount()
{
    SCH_SCREEN* screen;
    SCH_SCREENS s_list( Schematic().Root() );

    // Set the sheet count, and the sheet number (1 for root sheet)
    int              sheet_count       = Schematic().Root().CountSheets();
    int              sheet_number      = 1;
    const KIID_PATH& current_sheetpath = GetCurrentSheet().Path();

    // @todo Remove all pseudo page number system is left over from prior to real page number
    //       implementation.
    for( const SCH_SHEET_PATH& sheet : Schematic().GetSheets() )
    {
        if( sheet.Path() == current_sheetpath )  // Current sheet path found
            break;

        sheet_number++;                          // Not found, increment before this current path
    }

    for( screen = s_list.GetFirst(); screen != nullptr; screen = s_list.GetNext() )
        screen->SetPageCount( sheet_count );

    GetCurrentSheet().SetVirtualPageNumber( sheet_number );
    GetScreen()->SetVirtualPageNumber( sheet_number );
    GetScreen()->SetPageNumber( GetCurrentSheet().GetPageNumber() );
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
    wxString s = GetCurrentSheet().PathHumanReadable();

    return s;
}


void SCH_EDIT_FRAME::CreateScreens()
{
    m_schematic->Reset();
    m_schematic->SetProject( &Prj() );

    m_schematic->SetRoot( new SCH_SHEET( m_schematic ) );

    SCH_SCREEN* rootScreen = new SCH_SCREEN( m_schematic );
    m_schematic->Root().SetScreen( rootScreen );
    SetScreen( Schematic().RootScreen() );

    m_schematic->RootScreen()->SetFileName( wxEmptyString );

    // Don't leave root page number empty
    SCH_SHEET_PATH rootSheetPath;
    rootSheetPath.push_back( &m_schematic->Root() );
    m_schematic->RootScreen()->SetPageNumber( wxT( "1" ) );
    m_schematic->Root().AddInstance( rootSheetPath );
    m_schematic->Root().SetPageNumber( rootSheetPath, wxT( "1" ) );

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
        FocusOnItem( nullptr );

        Schematic().SetCurrentSheet( aSheet );
        GetCanvas()->DisplaySheet( aSheet.LastScreen() );
    }
}


void SCH_EDIT_FRAME::HardRedraw()
{
    SCH_SCREEN* screen = GetCurrentSheet().LastScreen();

    for( SCH_ITEM* item : screen->Items() )
        item->ClearCaches();

    for( std::pair<const wxString, LIB_SYMBOL*>& libSymbol : screen->GetLibSymbols() )
        libSymbol.second->ClearCaches();

    RecalculateConnections( LOCAL_CLEANUP );

    FocusOnItem( nullptr );

    GetCanvas()->DisplaySheet( GetCurrentSheet().LastScreen() );
    GetCanvas()->ForceRefresh();
}


bool SCH_EDIT_FRAME::canCloseWindow( wxCloseEvent& aEvent )
{
    // Exit interactive editing
    // Note this this will commit *some* pending changes.  For instance, the EE_POINT_EDITOR
    // will cancel any drag currently in progress, but commit all changes from previous drags.
    if( m_toolManager )
        m_toolManager->RunAction( ACTIONS::cancelInteractive, true );

    // Shutdown blocks must be determined and vetoed as early as possible
    if( KIPLATFORM::APP::SupportsShutdownBlockReason() && aEvent.GetId() == wxEVT_QUERY_END_SESSION
            && Schematic().GetSheets().IsModified() )
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

        symbolViewer = (SYMBOL_VIEWER_FRAME*) Kiway().Player( FRAME_SCH_VIEWER_MODAL, false );

        if( symbolViewer && !symbolViewer->Close() )   // Can close modal symbol viewer?
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

    SIM_PLOT_FRAME* simFrame = (SIM_PLOT_FRAME*) Kiway().Player( FRAME_SIMULATOR, false );

    if( simFrame && !simFrame->Close() )   // Can close the simulator?
        return false;

    // We may have gotten multiple events; don't clean up twice
    if( !Schematic().IsValid() )
        return false;

    SCH_SHEET_LIST sheetlist = Schematic().GetSheets();

    if( sheetlist.IsModified() )
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
    }

    // Close modeless dialogs.  They're trouble when they get destroyed after the frame and/or
    // board.
    wxWindow* open_dlg = wxWindow::FindWindowByName( DIALOG_ERC_WINDOW_NAME );

    if( open_dlg )
        open_dlg->Close( true );

    return true;
}


void SCH_EDIT_FRAME::doCloseWindow()
{
    SCH_SHEET_LIST sheetlist = Schematic().GetSheets();

    // Shutdown all running tools
    if( m_toolManager )
        m_toolManager->ShutdownAllTools();

    RecordERCExclusions();

    // Close the find dialog and preserve its setting if it is displayed.
    if( m_findReplaceDialog )
    {
        m_findStringHistoryList = m_findReplaceDialog->GetFindEntries();
        m_replaceStringHistoryList = m_findReplaceDialog->GetReplaceEntries();

        m_findReplaceDialog->Destroy();
        m_findReplaceDialog = nullptr;
    }

    if( Kiway().Player( FRAME_SIMULATOR, false ) )
        Prj().GetProjectFile().m_SchematicSettings->m_NgspiceSimulatorSettings->SaveToFile();

    SCH_SCREENS screens( Schematic().Root() );
    wxFileName fn;

    for( SCH_SCREEN* screen = screens.GetFirst(); screen != nullptr; screen = screens.GetNext() )
    {
        fn = Prj().AbsolutePath( screen->GetFileName() );

        // Auto save file name is the normal file name prepended with GetAutoSaveFilePrefix().
        fn.SetName( GetAutoSaveFilePrefix() + fn.GetName() );

        if( fn.IsFileWritable() )
            wxRemoveFile( fn.GetFullPath() );
    }

    wxFileName tmpFn = Prj().GetProjectFullName();
    wxFileName autoSaveFileName( tmpFn.GetPath(), getAutoSaveFileName() );

    if( autoSaveFileName.IsFileWritable() )
        wxRemoveFile( autoSaveFileName.GetFullPath() );

    sheetlist.ClearModifyStatus();

    wxString fileName = Prj().AbsolutePath( Schematic().RootScreen()->GetFileName() );

    if( !Schematic().GetFileName().IsEmpty() && !Schematic().RootScreen()->IsEmpty() )
        UpdateFileHistory( fileName );

    // Make sure local settings are persisted
    SaveProjectSettings();

    Schematic().RootScreen()->Clear();

    // all sub sheets are deleted, only the main sheet is usable
    GetCurrentSheet().clear();

    // Clear view before destroying schematic as repaints depend on schematic being valid
    SetScreen( nullptr );

    Schematic().Reset();

    Destroy();
}


void SCH_EDIT_FRAME::RecordERCExclusions()
{
    SCH_SHEET_LIST sheetList = Schematic().GetSheets();
    ERC_SETTINGS&  ercSettings = Schematic().ErcSettings();

    ercSettings.m_ErcExclusions.clear();

    for( unsigned i = 0; i < sheetList.size(); i++ )
    {
        for( SCH_ITEM* item : sheetList[i].LastScreen()->Items().OfType( SCH_MARKER_T ) )
        {
            SCH_MARKER* marker = static_cast<SCH_MARKER*>( item );

            if( marker->IsExcluded() )
                ercSettings.m_ErcExclusions.insert( marker->Serialize() );
        }
    }
}


void SCH_EDIT_FRAME::ResolveERCExclusions()
{
    SCH_SHEET_LIST sheetList = Schematic().GetSheets();

    for( SCH_MARKER* marker : Schematic().ResolveERCExclusions() )
    {
        SCH_SHEET_PATH errorPath;
        ignore_unused( sheetList.GetItem( marker->GetRCItem()->GetMainItemID(), &errorPath ) );

        if( errorPath.LastScreen() )
            errorPath.LastScreen()->Append( marker );
        else
            Schematic().RootScreen()->Append( marker );
    }
}


SEVERITY SCH_EDIT_FRAME::GetSeverity( int aErrorCode ) const
{
    return Schematic().ErcSettings().GetSeverity( aErrorCode );
}


wxString SCH_EDIT_FRAME::GetUniqueFilenameForCurrentSheet()
{
    // Filename is rootSheetName-sheetName-...-sheetName
    // Note that we need to fetch the rootSheetName out of its filename, as the root SCH_SHEET's
    // name is just a timestamp.

    wxFileName rootFn( GetCurrentSheet().at( 0 )->GetFileName() );
    wxString   filename = rootFn.GetName();

    for( unsigned i = 1; i < GetCurrentSheet().size(); i++ )
        filename += wxT( "-" ) + GetCurrentSheet().at( i )->GetName();

    return filename;
}


void SCH_EDIT_FRAME::OnModify()
{
    wxASSERT( GetScreen() );

    if( !GetScreen() )
        return;

    GetScreen()->SetContentModified();

    if( ADVANCED_CFG::GetCfg().m_RealTimeConnectivity && CONNECTION_GRAPH::m_allowRealTime )
        RecalculateConnections( NO_CLEANUP );
    else
        GetScreen()->SetConnectivityDirty();

    GetCanvas()->Refresh();
    UpdateHierarchyNavigator();

    if( !GetTitle().StartsWith( "*" ) )
        UpdateTitle();
}


void SCH_EDIT_FRAME::OnUpdatePCB( wxCommandEvent& event )
{
    if( Kiface().IsSingle() )
    {
        DisplayError( this,  _( "Cannot update the PCB, because the Schematic Editor is opened"
                                " in stand-alone mode. In order to create/update PCBs from"
                                " schematics, launch the KiCad shell and create a project." ) );
        return;
    }

    KIWAY_PLAYER* frame = Kiway().Player( FRAME_PCB_EDITOR, false );

    if( !frame )
    {
        wxFileName fn = Prj().GetProjectFullName();
        fn.SetExt( PcbFileExtension );

        frame = Kiway().Player( FRAME_PCB_EDITOR, true );
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


void SCH_EDIT_FRAME::UpdateHierarchyNavigator()
{
    m_toolManager->GetTool<SCH_NAVIGATE_TOOL>()->CleanHistory();
    m_hierarchy->UpdateHierarchyTree();
}


void SCH_EDIT_FRAME::ShowFindReplaceDialog( bool aReplace )
{
    wxString findString;

    EE_SELECTION& selection = m_toolManager->GetTool<EE_SELECTION_TOOL>()->GetSelection();

    if( selection.Size() == 1 )
    {
        EDA_ITEM* front = selection.Front();

        switch( front->Type() )
        {
        case SCH_SYMBOL_T:
            findString = static_cast<SCH_SYMBOL*>( front )->GetValue( &GetCurrentSheet(), true );
            break;

        case SCH_FIELD_T:
            findString = static_cast<SCH_FIELD*>( front )->GetShownText();
            break;

        case SCH_LABEL_T:
        case SCH_GLOBAL_LABEL_T:
        case SCH_HIER_LABEL_T:
        case SCH_SHEET_PIN_T:
            findString = static_cast<SCH_LABEL_BASE*>( front )->GetShownText();
            break;

        case SCH_TEXT_T:
            findString = static_cast<SCH_TEXT*>( front )->GetShownText();

            if( findString.Contains( wxT( "\n" ) ) )
                findString = findString.Before( '\n' );

            break;

        default:
            break;
        }
    }

    if( m_findReplaceDialog )
        m_findReplaceDialog->Destroy();

    m_findReplaceDialog= new DIALOG_SCH_FIND( this, m_findReplaceData, wxDefaultPosition,
                                              wxDefaultSize, aReplace ? wxFR_REPLACEDIALOG : 0 );

    m_findReplaceDialog->SetFindEntries( m_findStringHistoryList, findString );
    m_findReplaceDialog->SetReplaceEntries( m_replaceStringHistoryList );
    m_findReplaceDialog->Show( true );
}


void SCH_EDIT_FRAME::ShowFindReplaceStatus( const wxString& aMsg, int aStatusTime )
{
    // Prepare the infobar, since we don't know its state
    m_infoBar->RemoveAllButtons();
    m_infoBar->AddCloseButton();

    m_infoBar->ShowMessageFor( aMsg, aStatusTime, wxICON_INFORMATION );
}


void SCH_EDIT_FRAME::ClearFindReplaceStatus()
{
    m_infoBar->Dismiss();
}


void SCH_EDIT_FRAME::OnFindDialogClose()
{
    m_findStringHistoryList = m_findReplaceDialog->GetFindEntries();
    m_replaceStringHistoryList = m_findReplaceDialog->GetReplaceEntries();

    m_findReplaceDialog->Destroy();
    m_findReplaceDialog = nullptr;

    m_toolManager->RunAction( ACTIONS::updateFind, true );
}


void SCH_EDIT_FRAME::OnLoadFile( wxCommandEvent& event )
{
    wxString fn = GetFileFromHistory( event.GetId(), _( "Schematic" ) );

    if( fn.size() )
        OpenProjectFiles( std::vector<wxString>( 1, fn ) );
}


void SCH_EDIT_FRAME::OnClearFileHistory( wxCommandEvent& aEvent )
{
    ClearFileHistory();
}


void SCH_EDIT_FRAME::NewProject()
{
    wxString pro_dir = m_mruPath;

    wxFileDialog dlg( this, _( "New Schematic" ), pro_dir, wxEmptyString,
                      KiCadSchematicFileWildcard(), wxFD_SAVE );

    if( dlg.ShowModal() != wxID_CANCEL )
    {
        // Enforce the extension, wxFileDialog is inept.
        wxFileName create_me = dlg.GetPath();
        create_me.SetExt( KiCadSchematicFileExtension );

        if( create_me.FileExists() )
        {
            wxString msg;
            msg.Printf( _( "Schematic file '%s' already exists." ), create_me.GetFullName() );
            DisplayError( this, msg );
            return ;
        }

        // OpenProjectFiles() requires absolute
        wxASSERT_MSG( create_me.IsAbsolute(), "wxFileDialog returned non-absolute path" );

        OpenProjectFiles( std::vector<wxString>( 1, create_me.GetFullPath() ), KICTL_CREATE );
        m_mruPath = create_me.GetPath();
    }
}


void SCH_EDIT_FRAME::LoadProject()
{
    wxString pro_dir = m_mruPath;
    wxString wildcards = AllSchematicFilesWildcard()
                            + "|" + KiCadSchematicFileWildcard()
                            + "|" + LegacySchematicFileWildcard();

    wxFileDialog dlg( this, _( "Open Schematic" ), pro_dir, wxEmptyString,
                      wildcards, wxFD_OPEN | wxFD_FILE_MUST_EXIST );

    if( dlg.ShowModal() != wxID_CANCEL )
    {
        OpenProjectFiles( std::vector<wxString>( 1, dlg.GetPath() ) );
        m_mruPath = Prj().GetProjectPath();
    }
}


void SCH_EDIT_FRAME::OnOpenPcbnew( wxCommandEvent& event )
{
    wxFileName kicad_board = Prj().AbsolutePath( Schematic().GetFileName() );

    if( kicad_board.IsOk() && !Schematic().GetFileName().IsEmpty() )
    {
        kicad_board.SetExt( PcbFileExtension );
        wxFileName legacy_board( kicad_board );
        legacy_board.SetExt( LegacyPcbFileExtension );
        wxFileName& boardfn = legacy_board;

        if( !legacy_board.FileExists() || kicad_board.FileExists() )
            boardfn = kicad_board;

        if( Kiface().IsSingle() )
        {
            ExecuteFile( PCBNEW_EXE, boardfn.GetFullPath() );
        }
        else
        {
            KIWAY_PLAYER* frame = Kiway().Player( FRAME_PCB_EDITOR, false );

            if( !frame )
            {
                frame = Kiway().Player( FRAME_PCB_EDITOR, true );
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
        // If we are running inside a project, it should be impossible for this case to happen
        wxASSERT( Kiface().IsSingle() );
        ExecuteFile( PCBNEW_EXE );
    }
}


void SCH_EDIT_FRAME::OnOpenCvpcb( wxCommandEvent& event )
{
    wxFileName fn = Prj().AbsolutePath( Schematic().GetFileName() );
    fn.SetExt( NetlistFileExtension );

    if( !ReadyToNetlist( _( "Assigning footprints requires a fully annotated schematic." ) ) )
        return;

    try
    {
        KIWAY_PLAYER* player = Kiway().Player( FRAME_CVPCB, false );  // test open already.

        if( !player )
        {
            player = Kiway().Player( FRAME_CVPCB, true );
            player->Show( true );
        }

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


void SCH_EDIT_FRAME::PrintPage( const RENDER_SETTINGS* aSettings )
{
    wxString fileName = Prj().AbsolutePath( GetScreen()->GetFileName() );

    const wxBrush& brush =
            wxBrush( GetColorSettings()->GetColor( LAYER_SCHEMATIC_BACKGROUND ).ToColour() );
    aSettings->GetPrintDC()->SetBackground( brush );
    aSettings->GetPrintDC()->Clear();

    aSettings->GetPrintDC()->SetLogicalFunction( wxCOPY );
    GetScreen()->Print( aSettings );
    PrintDrawingSheet( aSettings, GetScreen(), IU_PER_MILS, fileName );
}


bool SCH_EDIT_FRAME::isAutoSaveRequired() const
{
    // In case this event happens before g_RootSheet is initialized which does happen
    // on mingw64 builds.

    if( Schematic().IsValid() )
    {
        return IsContentModified();
    }

    return false;
}


static void inheritNetclass( const SCH_SHEET_PATH& aSheetPath, SCH_LABEL_BASE* aItem )
{
    if( CONNECTION_SUBGRAPH::GetDriverPriority( aItem ) == CONNECTION_SUBGRAPH::PRIORITY::NONE )
        return;

    // Netclasses are assigned to subgraphs by association with their netname.  However, when
    // a new label is attached to an existing subgraph (with an existing netclass association),
    // the association will be lost as the label will drive its name on to the graph.
    //
    // Here we find the previous driver of the subgraph and if it had a netclass we associate
    // the new netname with that netclass as well.
    //
    SCHEMATIC*           schematic = aItem->Schematic();
    CONNECTION_SUBGRAPH* subgraph = schematic->ConnectionGraph()->GetSubgraphForItem( aItem );

    std::map<wxString, wxString>& netclassAssignments =
                            schematic->Prj().GetProjectFile().NetSettings().m_NetClassAssignments;

    if( subgraph )
    {
        SCH_ITEM* previousDriver = nullptr;
        CONNECTION_SUBGRAPH::PRIORITY priority = CONNECTION_SUBGRAPH::PRIORITY::INVALID;

        for( SCH_ITEM* item : subgraph->m_drivers )
        {
            if( item == aItem )
                continue;

            CONNECTION_SUBGRAPH::PRIORITY p = CONNECTION_SUBGRAPH::GetDriverPriority( item );

            if( p > priority )
            {
                priority = p;
                previousDriver = item;
            }
        }

        if( previousDriver )
        {
            wxString path = aSheetPath.PathHumanReadable();
            wxString oldDrivenName = path + subgraph->GetNameForDriver( previousDriver );
            wxString drivenName = path + subgraph->GetNameForDriver( aItem );

            if( netclassAssignments.count( oldDrivenName ) )
                netclassAssignments[ drivenName ] = netclassAssignments[ oldDrivenName ];
        }
    }
}


void SCH_EDIT_FRAME::AddItemToScreenAndUndoList( SCH_SCREEN* aScreen, SCH_ITEM* aItem,
                                                 bool aUndoAppend )
{
    wxCHECK_RET( aItem != nullptr, wxT( "Cannot add null item to list." ) );

    SCH_SHEET*  parentSheet = nullptr;
    SCH_SYMBOL* parentSymbol = nullptr;
    SCH_ITEM*   undoItem = aItem;

    if( aItem->Type() == SCH_SHEET_PIN_T )
    {
        parentSheet = (SCH_SHEET*) aItem->GetParent();

        wxCHECK_RET( parentSheet && parentSheet->Type() == SCH_SHEET_T,
                     wxT( "Cannot place sheet pin in invalid schematic sheet." ) );

        undoItem = parentSheet;
    }

    else if( aItem->Type() == SCH_FIELD_T )
    {
        parentSymbol = (SCH_SYMBOL*) aItem->GetParent();

        wxCHECK_RET( parentSymbol && parentSymbol->Type() == SCH_SYMBOL_T,
                     wxT( "Cannot place field in invalid schematic symbol." ) );

        undoItem = parentSymbol;
    }

    if( aItem->IsNew() )
    {
        if( aItem->Type() == SCH_SHEET_PIN_T )
        {
            // Sheet pins are owned by their parent sheet.
            SaveCopyInUndoList( aScreen, undoItem, UNDO_REDO::CHANGED, aUndoAppend );

            parentSheet->AddPin( (SCH_SHEET_PIN*) aItem );
        }
        else if( aItem->Type() == SCH_FIELD_T )
        {
            // Symbol fields are also owned by their parent, but new symbol fields are
            // handled elsewhere.
            wxLogMessage( wxT( "addCurrentItemToScreen: unexpected new SCH_FIELD" ) );
        }
        else
        {
            if( !aScreen->CheckIfOnDrawList( aItem ) )  // don't want a loop!
                AddToScreen( aItem, aScreen );

            SaveCopyForRepeatItem( aItem );
            SaveCopyInUndoList( aScreen, undoItem, UNDO_REDO::NEWITEM, aUndoAppend );
        }

        // Update connectivity info for new item
        if( !aItem->IsMoving() )
        {
            RecalculateConnections( LOCAL_CLEANUP );

            if( SCH_LABEL_BASE* label = dynamic_cast<SCH_LABEL_BASE*>( aItem ) )
                inheritNetclass( GetCurrentSheet(), label );
        }
    }

    aItem->ClearFlags( IS_NEW );

    aScreen->SetContentModified();
    UpdateItem( aItem );

    if( !aItem->IsMoving() && aItem->IsConnectable() )
    {
        std::vector<VECTOR2I> pts = aItem->GetConnectionPoints();

        for( auto i = pts.begin(); i != pts.end(); i++ )
        {
            for( auto j = i + 1; j != pts.end(); j++ )
                TrimWire( *i, *j );

            if( aScreen->IsExplicitJunctionNeeded( *i ) )
                AddJunction( aScreen, *i, true, false );
        }

        TestDanglingEnds();

        for( SCH_ITEM* item : aItem->ConnectedItems( GetCurrentSheet() ) )
            UpdateItem( item );
    }

    aItem->ClearEditFlags();
    GetCanvas()->Refresh();
}


void SCH_EDIT_FRAME::UpdateTitle()
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
        title += wxString::Format( wxT( " [%s]" ), GetCurrentSheet().PathHumanReadable( false ) );

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
    m_toolManager->RunAction( ACTIONS::zoomFitScreen, true );
    GetScreen()->m_zoomInitialized = true;
}


void SCH_EDIT_FRAME::RecalculateConnections( SCH_CLEANUP_FLAGS aCleanupFlags )
{
    const SCH_CONNECTION* highlight       = GetHighlightedConnection();
    SCH_ITEM*             highlightedItem = highlight ? highlight->Parent() : nullptr;
    SCH_SHEET_PATH highlightPath;

    if( highlight )
        highlightPath = highlight->LocalSheet();

    SCHEMATIC_SETTINGS& settings = Schematic().Settings();
    SCH_SHEET_LIST      list = Schematic().GetSheets();
#ifdef PROFILE
    PROF_COUNTER   timer;
#endif

    // Ensure schematic graph is accurate
    if( aCleanupFlags == LOCAL_CLEANUP )
    {
        SchematicCleanUp( GetScreen() );
    }
    else if( aCleanupFlags == GLOBAL_CLEANUP )
    {
        for( const SCH_SHEET_PATH& sheet : list )
            SchematicCleanUp( sheet.LastScreen() );
    }

#ifdef PROFILE
    timer.Stop();
    wxLogTrace( "CONN_PROFILE", "SchematicCleanUp() %0.4f ms", timer.msecs() );
#endif

    if( settings.m_IntersheetRefsShow )
        RecomputeIntersheetRefs();

    std::function<void( SCH_ITEM* )> changeHandler =
            [&]( SCH_ITEM* aChangedItem ) -> void
            {
                GetCanvas()->GetView()->Update( aChangedItem, KIGFX::REPAINT );
            };

    Schematic().ConnectionGraph()->Recalculate( list, true, &changeHandler );

    GetCanvas()->GetView()->UpdateAllItemsConditionally( KIGFX::REPAINT,
            []( KIGFX::VIEW_ITEM* aItem )
            {
                SCH_ITEM* item = dynamic_cast<SCH_ITEM*>( aItem );
                SCH_CONNECTION* connection = item ? item->Connection() : nullptr;

                if( connection && connection->HasDriverChanged() )
                {
                    connection->ClearDriverChanged();
                    return true;
                }

                EDA_TEXT* text = dynamic_cast<EDA_TEXT*>( aItem );

                if( text && text->HasTextVars() )
                    return true;

                return false;
            } );

    if( highlightedItem )
        SetHighlightedConnection( highlightedItem->Connection( &highlightPath ) );
}


void SCH_EDIT_FRAME::RecomputeIntersheetRefs()
{
    std::map<wxString, std::set<wxString>>& pageRefsMap = Schematic().GetPageRefsMap();

    pageRefsMap.clear();

    SCH_SCREENS           screens( Schematic().Root() );
    std::vector<wxString> pageNumbers;

    /* Iterate over screens */
    for( SCH_SCREEN* screen = screens.GetFirst(); screen != nullptr; screen = screens.GetNext() )
    {
        pageNumbers.clear();

        /* Find in which sheets this screen is used */
        for( const SCH_SHEET_PATH& sheet : Schematic().GetSheets() )
        {
            if( sheet.LastScreen() == screen )
                pageNumbers.push_back( sheet.GetPageNumber() );
        }

        for( SCH_ITEM* item : screen->Items() )
        {
            if( item->Type() == SCH_GLOBAL_LABEL_T )
            {
                SCH_GLOBALLABEL*    globalLabel = static_cast<SCH_GLOBALLABEL*>( item );
                std::set<wxString>& pageList = pageRefsMap[ globalLabel->GetText() ];

                for( const wxString& pageNo : pageNumbers )
                    pageList.insert( pageNo );
            }
        }
    }

    bool show = Schematic().Settings().m_IntersheetRefsShow;

    // Refresh all global labels.  Note that we have to collect them first as the
    // SCH_SCREEN::Update() call is going to invalidate the RTree iterator.

    std::vector<SCH_GLOBALLABEL*> globalLabels;

    for( EDA_ITEM* item : GetScreen()->Items().OfType( SCH_GLOBAL_LABEL_T ) )
        globalLabels.push_back( static_cast<SCH_GLOBALLABEL*>( item ) );

    for( SCH_GLOBALLABEL* globalLabel : globalLabels )
    {
        globalLabel->GetFields()[0].SetVisible( show );

        if( show )
        {
            GetScreen()->Update( globalLabel );
            GetCanvas()->GetView()->Update( globalLabel );
        }
    }
}


void SCH_EDIT_FRAME::ShowAllIntersheetRefs( bool aShow )
{
    if( aShow )
        RecomputeIntersheetRefs();

    GetCanvas()->GetView()->SetLayerVisible( LAYER_INTERSHEET_REFS, aShow );
}


void SCH_EDIT_FRAME::CommonSettingsChanged( bool aEnvVarsChanged, bool aTextVarsChanged )
{
    SCHEMATIC_SETTINGS& settings = Schematic().Settings();

    SCH_BASE_FRAME::CommonSettingsChanged( aEnvVarsChanged, aTextVarsChanged );
    settings.m_JunctionSize = GetSchematicJunctionSize();

    ShowAllIntersheetRefs( settings.m_IntersheetRefsShow );

    EESCHEMA_SETTINGS* cfg = Pgm().GetSettingsManager().GetAppSettings<EESCHEMA_SETTINGS>();
    GetGalDisplayOptions().ReadWindowSettings( cfg->m_Window );

    KIGFX::VIEW* view = GetCanvas()->GetView();
    view->SetLayerVisible( LAYER_ERC_ERR, cfg->m_Appearance.show_erc_errors );
    view->SetLayerVisible( LAYER_ERC_WARN, cfg->m_Appearance.show_erc_warnings );
    view->SetLayerVisible( LAYER_ERC_EXCLUSION, cfg->m_Appearance.show_erc_exclusions );

    SCH_SCREEN* screen = GetCurrentSheet().LastScreen();

    for( SCH_ITEM* item : screen->Items() )
        item->ClearCaches();

    for( std::pair<const wxString, LIB_SYMBOL*>& libSymbol : screen->GetLibSymbols() )
        libSymbol.second->ClearCaches();

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

    // status bar
    UpdateMsgPanel();

    UpdateTitle();

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
    if( const SCH_CONNECTION* conn = GetHighlightedConnection() )
    {
        SetStatusText( wxString::Format( _( "Highlighted net: %s" ),
                                         UnescapeString( conn->Name() ) ) );
    }
    else
    {
        SetStatusText( wxT( "" ) );
    }
}


void SCH_EDIT_FRAME::SetScreen( BASE_SCREEN* aScreen )
{
    if( m_toolManager )
        m_toolManager->RunAction( EE_ACTIONS::clearSelection, true );

    SCH_BASE_FRAME::SetScreen( aScreen );
    GetCanvas()->DisplaySheet( static_cast<SCH_SCREEN*>( aScreen ) );
}


const BOX2I SCH_EDIT_FRAME::GetDocumentExtents( bool aIncludeAllVisible ) const
{
    BOX2I bBoxDoc;

    if( aIncludeAllVisible )
    {
        // Get the whole page size and return that
        int sizeX = GetScreen()->GetPageSettings().GetWidthIU();
        int sizeY = GetScreen()->GetPageSettings().GetHeightIU();
        bBoxDoc   = BOX2I( VECTOR2I( 0, 0 ), VECTOR2I( sizeX, sizeY ) );
    }
    else
    {
        // Get current drawing-sheet in a form we can compare to an EDA_ITEM
        DS_PROXY_VIEW_ITEM* ds = SCH_BASE_FRAME::GetCanvas()->GetView()->GetDrawingSheet();
        EDA_ITEM*           dsAsItem = static_cast<EDA_ITEM*>( ds );

        // Need an EDA_RECT so the first ".Merge" sees it's uninitialized
        EDA_RECT bBoxItems;

        // Calc the bounding box of all items on screen except the page border
        for( EDA_ITEM* item : GetScreen()->Items() )
        {
            if( item != dsAsItem ) // Ignore the drawing-sheet itself
                bBoxItems.Merge( item->GetBoundingBox() );

            bBoxDoc = bBoxItems;
        }
    }
    return bBoxDoc;
}


void SCH_EDIT_FRAME::FixupJunctions()
{
    // Save the current sheet, to retrieve it later
    SCH_SHEET_PATH oldsheetpath = GetCurrentSheet();

    SCH_SHEET_LIST sheetList = Schematic().GetSheets();

    for( const SCH_SHEET_PATH& sheet : sheetList )
    {
        size_t num_undos = m_undoList.m_CommandsList.size();

        // We require a set here to avoid adding multiple junctions to the same spot
        std::set<wxPoint> junctions;

        SetCurrentSheet( sheet );
        GetCurrentSheet().UpdateAllScreenReferences();

        SCH_SCREEN* screen = GetCurrentSheet().LastScreen();

        EE_SELECTION allItems;

        for( auto item : screen->Items() )
            allItems.Add( item );

        m_toolManager->RunAction( EE_ACTIONS::addNeededJunctions, true, &allItems );

        // Check if we modified anything during this routine
        // Needs to happen for every sheet to set the proper modified flag
        if( m_undoList.m_CommandsList.size() > num_undos )
            OnModify();
    }

    // Reselect the initial sheet:
    SetCurrentSheet( oldsheetpath );
    GetCurrentSheet().UpdateAllScreenReferences();
    SetScreen( GetCurrentSheet().LastScreen() );
}


bool SCH_EDIT_FRAME::IsContentModified() const
{
    return Schematic().GetSheets().IsModified();
}


bool SCH_EDIT_FRAME::GetShowAllPins() const
{
    EESCHEMA_SETTINGS* cfg = eeconfig();
    return cfg && cfg->m_Appearance.show_hidden_pins;
}


void SCH_EDIT_FRAME::FocusOnItem( SCH_ITEM* aItem )
{
    static KIID lastBrightenedItemID( niluuid );

    SCH_SHEET_LIST sheetList = Schematic().GetSheets();
    SCH_SHEET_PATH dummy;
    SCH_ITEM*      lastItem = sheetList.GetItem( lastBrightenedItemID, &dummy );

    if( lastItem && lastItem != aItem )
    {
        lastItem->ClearBrightened();

        UpdateItem( lastItem );
        lastBrightenedItemID = niluuid;
    }

    if( aItem )
    {
        aItem->SetBrightened();

        UpdateItem( aItem );
        lastBrightenedItemID = aItem->m_Uuid;

        FocusOnLocation( aItem->GetFocusPosition() );
    }
}


wxString SCH_EDIT_FRAME::GetCurrentFileName() const
{
    return Schematic().GetFileName();
}


SELECTION& SCH_EDIT_FRAME::GetCurrentSelection()
{
    return m_toolManager->GetTool<EE_SELECTION_TOOL>()->GetSelection();
}


void SCH_EDIT_FRAME::onSize( wxSizeEvent& aEvent )
{
    if( IsShown() )
    {
        // We only need this until the frame is done resizing and the final client size is
        // established.
        Unbind( wxEVT_SIZE, &SCH_EDIT_FRAME::onSize, this );
        GetToolManager()->RunAction( ACTIONS::zoomFitScreen, true );
    }

    // Skip() is called in the base class.
    EDA_DRAW_FRAME::OnSize( aEvent );
}


void SCH_EDIT_FRAME::SaveSymbolToSchematic( const LIB_SYMBOL& aSymbol,
                                            const KIID& aSchematicSymbolUUID )
{
    wxString msg;
    bool appendToUndo = false;

    SCH_SHEET_PATH sheetPath;
    SCH_ITEM*      item = Schematic().GetSheets().GetItem( aSchematicSymbolUUID, &sheetPath );

    if( item )
    {
        SCH_SYMBOL* symbol = dynamic_cast<SCH_SYMBOL*>( item );

        wxCHECK( symbol, /* void */ );

        // This needs to be done before the LIB_SYMBOL is changed to prevent stale library
        // symbols in the schematic file.
        sheetPath.LastScreen()->Remove( symbol );

        if( !symbol->IsNew() )
        {
            SaveCopyInUndoList( sheetPath.LastScreen(), symbol, UNDO_REDO::CHANGED, appendToUndo );
            appendToUndo = true;
        }

        symbol->SetLibSymbol( aSymbol.Flatten().release() );
        symbol->UpdateFields( &GetCurrentSheet(),
                              true, /* update style */
                              true, /* update ref */
                              true, /* update other fields */
                              false, /* reset ref */
                              false /* reset other fields */ );

        sheetPath.LastScreen()->Append( symbol );
        GetCanvas()->GetView()->Update( symbol );
    }

    GetCanvas()->Refresh();
    OnModify();
}


void SCH_EDIT_FRAME::UpdateItem( EDA_ITEM* aItem, bool isAddOrDelete, bool aUpdateRtree )
{
    SCH_BASE_FRAME::UpdateItem( aItem, isAddOrDelete, aUpdateRtree );

    if( SCH_ITEM* sch_item = dynamic_cast<SCH_ITEM*>( aItem ) )
        sch_item->ClearCaches();
}


void SCH_EDIT_FRAME::DisplayCurrentSheet()
{
    m_toolManager->RunAction( ACTIONS::cancelInteractive, true );
    m_toolManager->RunAction( EE_ACTIONS::clearSelection, true );
    SCH_SCREEN* screen = GetCurrentSheet().LastScreen();

    wxASSERT( screen );

    SetScreen( screen );

    // update the References
    GetCurrentSheet().UpdateAllScreenReferences();
    SetSheetNumberAndCount();

    if( !screen->m_zoomInitialized )
    {
        initScreenZoom();
    }
    else
    {
        // Set zoom to last used in this screen
        GetCanvas()->GetView()->SetScale( GetScreen()->m_LastZoomLevel );
        RedrawScreen( (wxPoint) GetScreen()->m_ScrollCenter, false );
    }

    UpdateTitle();

    HardRedraw();   // Ensure all items are redrawn (especially the drawing-sheet items)

    SCH_EDITOR_CONTROL* editTool = m_toolManager->GetTool<SCH_EDITOR_CONTROL>();
    TOOL_EVENT dummy;
    editTool->UpdateNetHighlighting( dummy );

    m_hierarchy->UpdateHierarchySelection();
}
