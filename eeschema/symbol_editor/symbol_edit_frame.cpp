/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008 Wayne Stambaugh <stambaughw@gmail.com>
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

#include <bitmaps.h>
#include <wx/hyperlink.h>
#include <base_screen.h>
#include <symbol_library.h>
#include <confirm.h>
#include <core/kicad_algo.h>
#include <eeschema_id.h>
#include <eeschema_settings.h>
#include <env_paths.h>
#include <gal/graphics_abstraction_layer.h>
#include <kidialog.h>
#include <kiface_base.h>
#include <kiplatform/app.h>
#include <kiway_express.h>
#include <symbol_edit_frame.h>
#include <lib_symbol_library_manager.h>
#include <symbol_editor/symbol_editor_settings.h>
#include <paths.h>
#include <pgm_base.h>
#include <project_sch.h>
#include <sch_painter.h>
#include <sch_view.h>
#include <settings/settings_manager.h>
#include <symbol_lib_table.h>
#include <tool/action_manager.h>
#include <tool/action_toolbar.h>
#include <tool/common_control.h>
#include <tool/common_tools.h>
#include <tool/editor_conditions.h>
#include <tool/embed_tool.h>
#include <tool/library_editor_control.h>
#include <tool/picker_tool.h>
#include <tool/properties_tool.h>
#include <tool/selection.h>
#include <tool/tool_dispatcher.h>
#include <tool/tool_manager.h>
#include <tool/zoom_tool.h>
#include <tools/ee_actions.h>
#include <tools/ee_inspection_tool.h>
#include <tools/ee_point_editor.h>
#include <tools/ee_grid_helper.h>
#include <tools/ee_selection_tool.h>
#include <tools/symbol_editor_control.h>
#include <tools/symbol_editor_drawing_tools.h>
#include <tools/symbol_editor_edit_tool.h>
#include <tools/symbol_editor_move_tool.h>
#include <tools/symbol_editor_pin_tool.h>
#include <view/view_controls.h>
#include <widgets/app_progress_dialog.h>
#include <widgets/wx_infobar.h>
#include <widgets/wx_progress_reporters.h>
#include <widgets/panel_sch_selection_filter.h>
#include <widgets/sch_properties_panel.h>
#include <widgets/symbol_tree_pane.h>
#include <widgets/wx_aui_utils.h>
#include <wildcards_and_files_ext.h>
#include <panel_sym_lib_table.h>
#include <string_utils.h>
#include <wx/msgdlg.h>
#include <wx/log.h>


bool SYMBOL_EDIT_FRAME::m_showDeMorgan = false;


BEGIN_EVENT_TABLE( SYMBOL_EDIT_FRAME, SCH_BASE_FRAME )
    EVT_COMBOBOX( ID_LIBEDIT_SELECT_UNIT_NUMBER, SYMBOL_EDIT_FRAME::OnSelectUnit )

    // menubar commands
    EVT_MENU( wxID_EXIT, SYMBOL_EDIT_FRAME::OnExitKiCad )
    EVT_MENU( wxID_CLOSE, SYMBOL_EDIT_FRAME::CloseWindow )

    // Update user interface elements.
    EVT_UPDATE_UI( ID_LIBEDIT_SELECT_UNIT_NUMBER, SYMBOL_EDIT_FRAME::OnUpdateUnitNumber )

    // Drop files event
    EVT_DROP_FILES( SYMBOL_EDIT_FRAME::OnDropFiles )

END_EVENT_TABLE()


SYMBOL_EDIT_FRAME::SYMBOL_EDIT_FRAME( KIWAY* aKiway, wxWindow* aParent ) :
        SCH_BASE_FRAME( aKiway, aParent, FRAME_SCH_SYMBOL_EDITOR, _( "Library Editor" ),
                        wxDefaultPosition, wxDefaultSize, KICAD_DEFAULT_DRAWFRAME_STYLE,
                        LIB_EDIT_FRAME_NAME ),
        m_unitSelectBox( nullptr ),
        m_isSymbolFromSchematic( false )
{
    SetShowDeMorgan( false );
    m_SyncPinEdit = false;

    m_symbol = nullptr;
    m_treePane = nullptr;
    m_libMgr = nullptr;
    m_unit = 1;
    m_bodyStyle = 1;
    m_aboutTitle = _HKI( "KiCad Symbol Editor" );

    wxIcon icon;
    wxIconBundle icon_bundle;

    icon.CopyFromBitmap( KiBitmap( BITMAPS::icon_libedit, 48 ) );
    icon_bundle.AddIcon( icon );
    icon.CopyFromBitmap( KiBitmap( BITMAPS::icon_libedit, 256 ) );
    icon_bundle.AddIcon( icon );
    icon.CopyFromBitmap( KiBitmap( BITMAPS::icon_libedit, 128 ) );
    icon_bundle.AddIcon( icon );
    icon.CopyFromBitmap( KiBitmap( BITMAPS::icon_libedit_32 ) );
    icon_bundle.AddIcon( icon );
    icon.CopyFromBitmap( KiBitmap( BITMAPS::icon_libedit_16 ) );
    icon_bundle.AddIcon( icon );

    SetIcons( icon_bundle );

    m_settings = Pgm().GetSettingsManager().GetAppSettings<SYMBOL_EDITOR_SETTINGS>( "symbol_editor" );
    LoadSettings( m_settings );

    m_libMgr = new LIB_SYMBOL_LIBRARY_MANAGER( *this );
    bool loadingCancelled = false;

    {
        // Preload libraries before using SyncLibraries the first time, as the preload is
        // multi-threaded
        WX_PROGRESS_REPORTER reporter( this, _( "Loading Symbol Libraries" ),
                                       m_libMgr->GetLibraryCount(), true );
        m_libMgr->Preload( reporter );

        loadingCancelled = reporter.IsCancelled();
        wxSafeYield();
    }

    SyncLibraries( false, loadingCancelled );
    m_treePane = new SYMBOL_TREE_PANE( this, m_libMgr );
    m_treePane->GetLibTree()->SetSortMode( (LIB_TREE_MODEL_ADAPTER::SORT_MODE) m_settings->m_LibrarySortMode );

    resolveCanvasType();
    SwitchCanvas( m_canvasType );

    // Ensure axis are always drawn
    KIGFX::GAL_DISPLAY_OPTIONS& gal_opts = GetGalDisplayOptions();
    gal_opts.m_axesEnabled = true;

    m_dummyScreen = new SCH_SCREEN();
    SetScreen( m_dummyScreen );
    GetScreen()->m_Center = true;

    GetCanvas()->GetViewControls()->SetCrossHairCursorPosition( VECTOR2D( 0, 0 ), false );

    GetRenderSettings()->LoadColors( GetColorSettings() );
    GetRenderSettings()->m_IsSymbolEditor = true;
    GetCanvas()->GetGAL()->SetAxesColor( m_colorSettings->GetColor( LAYER_SCHEMATIC_GRID_AXES ) );

    setupTools();
    setupUIConditions();

    ReCreateMenuBar();
    ReCreateHToolbar();
    ReCreateVToolbar();
    ReCreateOptToolbar();

    UpdateTitle();
    UpdateSymbolMsgPanelInfo();
    RebuildSymbolUnitsList();

    m_propertiesPanel = new SCH_PROPERTIES_PANEL( this, this );
    m_propertiesPanel->SetSplitterProportion( m_settings->m_AuiPanels.properties_splitter );

    m_selectionFilterPanel = new PANEL_SCH_SELECTION_FILTER( this );

    m_auimgr.SetManagedWindow( this );

    CreateInfoBar();

    // Rows; layers 4 - 6
    m_auimgr.AddPane( m_mainToolBar, EDA_PANE().HToolbar().Name( "MainToolbar" )
                      .Top().Layer( 6 ) );

    m_auimgr.AddPane( m_messagePanel, EDA_PANE().Messages().Name( "MsgPanel" )
                      .Bottom().Layer( 6 ) );

    // Columns; layers 1 - 3
    m_auimgr.AddPane( m_treePane, EDA_PANE().Palette().Name( "LibraryTree" )
                      .Left().Layer( 3 )
                      .TopDockable( false ).BottomDockable( false )
                      .Caption( _( "Libraries" ) )
                      .MinSize( FromDIP( 250 ), -1 ).BestSize( FromDIP( 250 ), -1 ) );

    m_auimgr.AddPane( m_propertiesPanel, defaultPropertiesPaneInfo( this ) );
    // Show or hide m_propertiesPanel depending on current settings:
    wxAuiPaneInfo& propertiesPaneInfo = m_auimgr.GetPane( PropertiesPaneName() );

    m_auimgr.AddPane( m_selectionFilterPanel, defaultSchSelectionFilterPaneInfo( this ) );

    wxAuiPaneInfo& selectionFilterPane = m_auimgr.GetPane( wxS( "SelectionFilter" ) );
    // The selection filter doesn't need to grow in the vertical direction when docked
    selectionFilterPane.dock_proportion = 0;

    propertiesPaneInfo.Show( m_settings->m_AuiPanels.show_properties );
    updateSelectionFilterVisbility();

    m_auimgr.AddPane( m_optionsToolBar, EDA_PANE().VToolbar().Name( "OptToolbar" )
                      .Left().Layer( 2 ) );

    m_auimgr.AddPane( m_drawToolBar, EDA_PANE().VToolbar().Name( "ToolsToolbar" )
                      .Right().Layer( 2 ) );

    // Center
    m_auimgr.AddPane( GetCanvas(), wxAuiPaneInfo().Name( "DrawFrame" )
                      .CentrePane() );

    FinishAUIInitialization();

    // Can't put this in LoadSettings, because it has to be called before setupTools :/
    EE_SELECTION_TOOL* selTool = GetToolManager()->GetTool<EE_SELECTION_TOOL>();
    selTool->GetFilter() = GetSettings()->m_SelectionFilter;

    if( m_settings->m_LibWidth > 0 )
        SetAuiPaneSize( m_auimgr, m_auimgr.GetPane( "LibraryTree" ), m_settings->m_LibWidth, -1 );

    Raise();
    Show( true );

    SyncView();
    GetCanvas()->GetView()->UseDrawPriority( true );
    GetCanvas()->GetGAL()->SetAxesEnabled( true );

    setupUnits( m_settings );

    // Set the working/draw area size to display a symbol to a reasonable value:
    // A 600mm x 600mm with a origin at the area center looks like a large working area
    double max_size_x = schIUScale.mmToIU( 600 );
    double max_size_y = schIUScale.mmToIU( 600 );
    BOX2D bbox;
    bbox.SetOrigin( -max_size_x /2, -max_size_y/2 );
    bbox.SetSize( max_size_x, max_size_y );
    GetCanvas()->GetView()->SetBoundary( bbox );

    m_toolManager->RunAction( ACTIONS::zoomFitScreen );

    m_acceptedExts.emplace( FILEEXT::KiCadSymbolLibFileExtension, &ACTIONS::ddAddLibrary );
    DragAcceptFiles( true );

    KIPLATFORM::APP::SetShutdownBlockReason( this, _( "Library changes are unsaved" ) );

    // Catch unhandled accelerator command characters that were no handled by the library tree
    // panel.
    Bind( wxEVT_CHAR, &TOOL_DISPATCHER::DispatchWxEvent, m_toolDispatcher );
    Bind( wxEVT_CHAR_HOOK, &TOOL_DISPATCHER::DispatchWxEvent, m_toolDispatcher );

    // Ensure the window is on top
    Raise();

    if( loadingCancelled )
        ShowInfoBarWarning( _( "Symbol library loading was cancelled by user." ) );
}


SYMBOL_EDIT_FRAME::~SYMBOL_EDIT_FRAME()
{
    // Shutdown all running tools
    if( m_toolManager )
        m_toolManager->ShutdownAllTools();

    setSymWatcher( nullptr );

    if( IsSymbolFromSchematic() )
    {
        delete m_symbol;
        m_symbol = nullptr;

        SCH_SCREEN* screen = GetScreen();
        delete screen;
        m_isSymbolFromSchematic = false;
    }

    // current screen is destroyed in EDA_DRAW_FRAME
    SetScreen( m_dummyScreen );

    SETTINGS_MANAGER&       mgr = Pgm().GetSettingsManager();
    SYMBOL_EDITOR_SETTINGS* cfg = mgr.GetAppSettings<SYMBOL_EDITOR_SETTINGS>( "symbol_editor" );

    if( cfg )
        mgr.Save( cfg );

    delete m_libMgr;
}


void SYMBOL_EDIT_FRAME::LoadSettings( APP_SETTINGS_BASE* aCfg )
{
    wxCHECK_RET( m_settings, "Call to SYMBOL_EDIT_FRAME::LoadSettings with null m_boardAdapter" );

    SCH_BASE_FRAME::LoadSettings( GetSettings() );

    GetRenderSettings()->m_ShowPinsElectricalType = m_settings->m_ShowPinElectricalType;
    GetRenderSettings()->m_ShowHiddenPins = m_settings->m_ShowHiddenPins;
    GetRenderSettings()->m_ShowHiddenFields = m_settings->m_ShowHiddenFields;
    GetRenderSettings()->m_ShowPinAltIcons = m_settings->m_ShowPinAltIcons;
    GetRenderSettings()->SetDefaultFont( wxEmptyString );
}


void SYMBOL_EDIT_FRAME::SaveSettings( APP_SETTINGS_BASE* aCfg )
{
    wxCHECK_RET( m_settings, "Call to SYMBOL_EDIT_FRAME::LoadSettings with null m_boardAdapter" );

    GetGalDisplayOptions().m_axesEnabled = true;

    SCH_BASE_FRAME::SaveSettings( GetSettings() );

    m_settings->m_ShowPinElectricalType  = GetRenderSettings()->m_ShowPinsElectricalType;
    m_settings->m_ShowHiddenPins = GetRenderSettings()->m_ShowHiddenPins;
    m_settings->m_ShowHiddenFields = GetRenderSettings()->m_ShowHiddenFields;
    m_settings->m_ShowPinAltIcons = GetRenderSettings()->m_ShowPinAltIcons;

    m_settings->m_LibWidth = m_treePane->GetSize().x;

    m_settings->m_LibrarySortMode = GetLibTree()->GetSortMode();

    m_settings->m_AuiPanels.properties_splitter = m_propertiesPanel->SplitterProportion();
    bool prop_shown = m_auimgr.GetPane( PropertiesPaneName() ).IsShown();
    m_settings->m_AuiPanels.show_properties = prop_shown;

    EE_SELECTION_TOOL* selTool = GetToolManager()->GetTool<EE_SELECTION_TOOL>();
    m_settings->m_SelectionFilter = selTool->GetFilter();
}


APP_SETTINGS_BASE* SYMBOL_EDIT_FRAME::config() const
{
    return static_cast<APP_SETTINGS_BASE*>( GetSettings() );
}


COLOR_SETTINGS* SYMBOL_EDIT_FRAME::GetColorSettings( bool aForceRefresh ) const
{
    SETTINGS_MANAGER& mgr = Pgm().GetSettingsManager();

    if( GetSettings()->m_UseEeschemaColorSettings )
        return mgr.GetColorSettings( mgr.GetAppSettings<EESCHEMA_SETTINGS>( "eeschema" )->m_ColorTheme );
    else
        return mgr.GetColorSettings( GetSettings()->m_ColorTheme );
}


void SYMBOL_EDIT_FRAME::setupTools()
{
    // Create the manager and dispatcher & route draw panel events to the dispatcher
    m_toolManager = new TOOL_MANAGER;
    m_toolManager->SetEnvironment( GetScreen(), GetCanvas()->GetView(),
                                   GetCanvas()->GetViewControls(), GetSettings(), this );
    m_actions = new EE_ACTIONS();
    m_toolDispatcher = new TOOL_DISPATCHER( m_toolManager );

    // Register tools
    m_toolManager->RegisterTool( new COMMON_CONTROL );
    m_toolManager->RegisterTool( new COMMON_TOOLS );
    m_toolManager->RegisterTool( new ZOOM_TOOL );
    m_toolManager->RegisterTool( new EE_SELECTION_TOOL );
    m_toolManager->RegisterTool( new PICKER_TOOL );
    m_toolManager->RegisterTool( new EE_INSPECTION_TOOL );
    m_toolManager->RegisterTool( new SYMBOL_EDITOR_PIN_TOOL );
    m_toolManager->RegisterTool( new SYMBOL_EDITOR_DRAWING_TOOLS );
    m_toolManager->RegisterTool( new EE_POINT_EDITOR );
    m_toolManager->RegisterTool( new SYMBOL_EDITOR_MOVE_TOOL );
    m_toolManager->RegisterTool( new SYMBOL_EDITOR_EDIT_TOOL );
    m_toolManager->RegisterTool( new LIBRARY_EDITOR_CONTROL );
    m_toolManager->RegisterTool( new SYMBOL_EDITOR_CONTROL );
    m_toolManager->RegisterTool( new PROPERTIES_TOOL );
    m_toolManager->RegisterTool( new EMBED_TOOL );
    m_toolManager->InitTools();

    // Run the selection tool, it is supposed to be always active
    m_toolManager->InvokeTool( "eeschema.InteractiveSelection" );

    GetCanvas()->SetEventDispatcher( m_toolDispatcher );
}


void SYMBOL_EDIT_FRAME::setupUIConditions()
{
    SCH_BASE_FRAME::setupUIConditions();

    ACTION_MANAGER*   mgr = m_toolManager->GetActionManager();
    EDITOR_CONDITIONS cond( this );

    wxASSERT( mgr );

#define ENABLE( x ) ACTION_CONDITIONS().Enable( x )
#define CHECK( x )  ACTION_CONDITIONS().Check( x )

    auto haveSymbolCond =
            [this]( const SELECTION& )
            {
                return m_symbol;
            };

    auto isEditableCond =
            [this]( const SELECTION& )
            {
                // Only root symbols from the new s-expression libraries or the schematic
                // are editable.
                return IsSymbolEditable() && !IsSymbolAlias();
            };

    auto isEditableInAliasCond =
            [this]( const SELECTION& )
            {
                // Less restrictive than isEditableCond
                // Symbols fields (root symbols and aliases) from the new s-expression libraries
                // or in the schematic are editable.
                return IsSymbolEditable();
            };

    auto symbolModifiedCondition =
            [this]( const SELECTION& sel )
            {
                return m_libMgr && m_libMgr->IsSymbolModified( GetTargetLibId().GetLibItemName(),
                                                               GetTargetLibId().GetLibNickname() );
            };

    auto libSelectedCondition =
            [this]( const SELECTION& sel )
            {
                return !GetTargetLibId().GetLibNickname().empty();
            };

    auto canEditProperties =
            [this]( const SELECTION& sel )
            {
                return m_symbol && ( !IsSymbolFromLegacyLibrary() || IsSymbolFromSchematic() );
            };

    auto saveSymbolAsCondition =
            [this]( const SELECTION& aSel )
            {
                return getTargetSymbol() != nullptr;
            };

    const auto isSymbolFromSchematicCond =
            [this]( const SELECTION& )
            {
                return IsSymbolFromSchematic();
            };

    // clang-format off
    mgr->SetConditions( ACTIONS::saveAll,                       ENABLE( SELECTION_CONDITIONS::ShowAlways ) );
    mgr->SetConditions( ACTIONS::save,                          ENABLE( SELECTION_CONDITIONS::ShowAlways ) );
    mgr->SetConditions( EE_ACTIONS::saveLibraryAs,              ENABLE( libSelectedCondition ) );
    mgr->SetConditions( EE_ACTIONS::saveSymbolAs,               ENABLE( saveSymbolAsCondition ) );
    mgr->SetConditions( EE_ACTIONS::saveSymbolCopyAs,           ENABLE( saveSymbolAsCondition ) );
    mgr->SetConditions( EE_ACTIONS::newSymbol,                  ENABLE( SELECTION_CONDITIONS::ShowAlways ) );
    mgr->SetConditions( EE_ACTIONS::importSymbol,               ENABLE( SELECTION_CONDITIONS::ShowAlways ) );
    mgr->SetConditions( EE_ACTIONS::editLibSymbolWithLibEdit,   ENABLE( isSymbolFromSchematicCond ) );

    mgr->SetConditions( ACTIONS::undo,                ENABLE( haveSymbolCond && cond.UndoAvailable() ) );
    mgr->SetConditions( ACTIONS::redo,                ENABLE( haveSymbolCond && cond.RedoAvailable() ) );
    mgr->SetConditions( ACTIONS::revert,              ENABLE( symbolModifiedCondition ) );

    mgr->SetConditions( ACTIONS::toggleGrid,          CHECK( cond.GridVisible() ) );
    mgr->SetConditions( ACTIONS::toggleGridOverrides, CHECK( cond.GridOverrides() ) );
    mgr->SetConditions( ACTIONS::toggleCursorStyle,   CHECK( cond.FullscreenCursor() ) );
    mgr->SetConditions( ACTIONS::millimetersUnits,    CHECK( cond.Units( EDA_UNITS::MM ) ) );
    mgr->SetConditions( ACTIONS::inchesUnits,         CHECK( cond.Units( EDA_UNITS::INCH ) ) );
    mgr->SetConditions( ACTIONS::milsUnits,           CHECK( cond.Units( EDA_UNITS::MILS ) ) );

    mgr->SetConditions( ACTIONS::cut,                 ENABLE( isEditableCond ) );
    mgr->SetConditions( ACTIONS::copy,                ENABLE( haveSymbolCond ) );
    mgr->SetConditions( ACTIONS::copyAsText,          ENABLE( haveSymbolCond ) );
    mgr->SetConditions( ACTIONS::paste,               ENABLE( isEditableCond &&
                                                              SELECTION_CONDITIONS::Idle && cond.NoActiveTool() ) );
    mgr->SetConditions( ACTIONS::doDelete,            ENABLE( isEditableCond ) );
    mgr->SetConditions( ACTIONS::duplicate,           ENABLE( isEditableCond ) );
    mgr->SetConditions( ACTIONS::selectAll,           ENABLE( haveSymbolCond ) );
    mgr->SetConditions( ACTIONS::unselectAll,         ENABLE( haveSymbolCond ) );

    // These actions in symbol editor when editing alias field rotations are allowed.
    mgr->SetConditions( EE_ACTIONS::rotateCW,         ENABLE( isEditableInAliasCond ) );
    mgr->SetConditions( EE_ACTIONS::rotateCCW,        ENABLE( isEditableInAliasCond ) );

    mgr->SetConditions( EE_ACTIONS::mirrorH,          ENABLE( isEditableCond ) );
    mgr->SetConditions( EE_ACTIONS::mirrorV,          ENABLE( isEditableCond ) );

    mgr->SetConditions( ACTIONS::zoomTool,            CHECK( cond.CurrentTool( ACTIONS::zoomTool ) ) );
    mgr->SetConditions( ACTIONS::selectionTool,       CHECK( cond.CurrentTool( ACTIONS::selectionTool ) ) );
    // clang-format on

    auto pinTypeCond =
            [this]( const SELECTION& )
            {
                return libeditconfig()->m_ShowPinElectricalType;
            };

    auto hiddenPinCond =
            [this]( const SELECTION& )
            {
                return libeditconfig()->m_ShowHiddenPins;
            };

    auto hiddenFieldCond =
            [this]( const SELECTION& )
            {
                return libeditconfig()->m_ShowHiddenFields;
            };

    auto showPinAltIconsCond =
            [this]( const SELECTION& )
            {
                return libeditconfig()->m_ShowPinAltIcons;
            };

    auto showLibraryTreeCond =
            [this]( const SELECTION& )
            {
                return IsLibraryTreeShown();
            };

    auto propertiesCond =
            [this] ( const SELECTION& )
            {
                return m_auimgr.GetPane( PropertiesPaneName() ).IsShown();
            };

    mgr->SetConditions( EE_ACTIONS::showElectricalTypes, CHECK( pinTypeCond ) );
    mgr->SetConditions( ACTIONS::toggleBoundingBoxes,    CHECK( cond.BoundingBoxes() ) );
    mgr->SetConditions( ACTIONS::showLibraryTree,        CHECK( showLibraryTreeCond ) );
    mgr->SetConditions( ACTIONS::showProperties,         CHECK( propertiesCond ) );
    mgr->SetConditions( EE_ACTIONS::showHiddenPins,      CHECK( hiddenPinCond ) );
    mgr->SetConditions( EE_ACTIONS::showHiddenFields,    CHECK( hiddenFieldCond ) );
    mgr->SetConditions( EE_ACTIONS::togglePinAltIcons,   CHECK( showPinAltIconsCond ) );

    auto demorganCond =
            [this]( const SELECTION& )
            {
                return GetShowDeMorgan();
            };

    auto demorganStandardCond =
            [this]( const SELECTION& )
            {
                return m_bodyStyle == BODY_STYLE::BASE;
            };

    auto demorganAlternateCond =
            [this]( const SELECTION& )
            {
                return m_bodyStyle == BODY_STYLE::DEMORGAN;
            };

    auto multiUnitModeCond =
            [this]( const SELECTION& )
            {
                return m_symbol && m_symbol->IsMulti() && !m_symbol->UnitsLocked();
            };

    auto hasMultipleUnitsCond =
            [this]( const SELECTION& )
            {
                return m_symbol && m_symbol->IsMulti();
            };

    auto syncedPinsModeCond =
            [this]( const SELECTION& )
            {
                return m_SyncPinEdit;
            };

    auto haveDatasheetCond =
            [this]( const SELECTION& )
            {
                return m_symbol && !m_symbol->GetDatasheetField().GetText().IsEmpty();
            };

    mgr->SetConditions( ACTIONS::showDatasheet,    ENABLE( haveDatasheetCond ) );
    mgr->SetConditions( EE_ACTIONS::symbolProperties, ENABLE( canEditProperties && haveSymbolCond ) );
    mgr->SetConditions( EE_ACTIONS::runERC,           ENABLE( haveSymbolCond ) );
    mgr->SetConditions( EE_ACTIONS::pinTable,         ENABLE( isEditableCond && haveSymbolCond ) );

    mgr->SetConditions( EE_ACTIONS::showDeMorganStandard,
                        ACTION_CONDITIONS().Enable( demorganCond ).Check( demorganStandardCond ) );
    mgr->SetConditions( EE_ACTIONS::showDeMorganAlternate,
                        ACTION_CONDITIONS().Enable( demorganCond ).Check( demorganAlternateCond ) );
    mgr->SetConditions( EE_ACTIONS::toggleSyncedPinsMode,
                        ACTION_CONDITIONS().Enable( multiUnitModeCond ).Check( syncedPinsModeCond ) );
    mgr->SetConditions( EE_ACTIONS::setUnitDisplayName,
                        ACTION_CONDITIONS().Enable( isEditableCond && hasMultipleUnitsCond ) );

// Only enable a tool if the symbol is edtable
#define EDIT_TOOL( tool ) ACTION_CONDITIONS().Enable( isEditableCond ).Check( cond.CurrentTool( tool ) )

    mgr->SetConditions( ACTIONS::deleteTool,             EDIT_TOOL( ACTIONS::deleteTool ) );
    mgr->SetConditions( EE_ACTIONS::placeSymbolPin,      EDIT_TOOL( EE_ACTIONS::placeSymbolPin ) );
    mgr->SetConditions( EE_ACTIONS::placeSymbolText,     EDIT_TOOL( EE_ACTIONS::placeSymbolText ) );
    mgr->SetConditions( EE_ACTIONS::drawSymbolTextBox,   EDIT_TOOL( EE_ACTIONS::drawSymbolTextBox ) );
    mgr->SetConditions( EE_ACTIONS::drawRectangle,       EDIT_TOOL( EE_ACTIONS::drawRectangle ) );
    mgr->SetConditions( EE_ACTIONS::drawCircle,          EDIT_TOOL( EE_ACTIONS::drawCircle ) );
    mgr->SetConditions( EE_ACTIONS::drawArc,             EDIT_TOOL( EE_ACTIONS::drawArc ) );
    mgr->SetConditions( EE_ACTIONS::drawBezier,          EDIT_TOOL( EE_ACTIONS::drawBezier ) );
    mgr->SetConditions( EE_ACTIONS::drawSymbolLines,     EDIT_TOOL( EE_ACTIONS::drawSymbolLines ) );
    mgr->SetConditions( EE_ACTIONS::drawSymbolPolygon,   EDIT_TOOL( EE_ACTIONS::drawSymbolPolygon ) );
    mgr->SetConditions( EE_ACTIONS::placeSymbolAnchor,   EDIT_TOOL( EE_ACTIONS::placeSymbolAnchor ) );
    mgr->SetConditions( EE_ACTIONS::importGraphics,      EDIT_TOOL( EE_ACTIONS::importGraphics ) );

#undef CHECK
#undef ENABLE
#undef EDIT_TOOL
}


bool SYMBOL_EDIT_FRAME::CanCloseSymbolFromSchematic( bool doClose )
{
    if( IsContentModified() )
    {
        SCH_EDIT_FRAME* schframe = (SCH_EDIT_FRAME*) Kiway().Player( FRAME_SCH, false );
        wxString        msg = _( "Save changes to '%s' before closing?" );

        switch( UnsavedChangesDialog( this, wxString::Format( msg, m_reference ), nullptr ) )
        {
        case wxID_YES:
            if( schframe && GetCurSymbol() )  // Should be always the case
                schframe->SaveSymbolToSchematic( *GetCurSymbol(), m_schematicSymbolUUID );

            break;

        case wxID_NO:
            break;

        default:
        case wxID_CANCEL:
            return false;
        }
    }

    if( doClose )
    {
        SetCurSymbol( nullptr, false );
        UpdateTitle();
    }

    return true;
}


bool SYMBOL_EDIT_FRAME::canCloseWindow( wxCloseEvent& aEvent )
{
    // Shutdown blocks must be determined and vetoed as early as possible
    if( KIPLATFORM::APP::SupportsShutdownBlockReason()
            && aEvent.GetId() == wxEVT_QUERY_END_SESSION
            && IsContentModified() )
    {
        return false;
    }

    if( m_isSymbolFromSchematic && !CanCloseSymbolFromSchematic( false ) )
        return false;

    if( !saveAllLibraries( true ) )
        return false;

    // Save symbol tree column widths
    m_libMgr->GetAdapter()->SaveSettings();

    return true;
}


void SYMBOL_EDIT_FRAME::doCloseWindow()
{
    Destroy();
}


void SYMBOL_EDIT_FRAME::RebuildSymbolUnitsList()
{
    if( !m_unitSelectBox )
        return;

    if( m_unitSelectBox->GetCount() != 0 )
        m_unitSelectBox->Clear();

    if( !m_symbol || m_symbol->GetUnitCount() <= 1 )
    {
        m_unit = 1;
        m_unitSelectBox->Append( wxEmptyString );
    }
    else
    {
        for( int i = 0; i < m_symbol->GetUnitCount(); i++ )
        {
            wxString unitDisplayName = m_symbol->GetUnitDisplayName( i + 1 );
            m_unitSelectBox->Append( unitDisplayName );
        }
    }

    // Ensure the selected unit is compatible with the number of units of the current symbol:
    if( m_symbol && m_symbol->GetUnitCount() < m_unit )
        m_unit = 1;

    m_unitSelectBox->SetSelection(( m_unit > 0 ) ? m_unit - 1 : 0 );
}


void SYMBOL_EDIT_FRAME::ToggleLibraryTree()
{
    wxAuiPaneInfo& treePane = m_auimgr.GetPane( m_treePane );
    treePane.Show( !IsLibraryTreeShown() );
    updateSelectionFilterVisbility();
    m_auimgr.Update();
    Refresh();
}


bool SYMBOL_EDIT_FRAME::IsLibraryTreeShown() const
{
    return const_cast<wxAuiManager&>( m_auimgr ).GetPane( m_treePane ).IsShown();
}


void SYMBOL_EDIT_FRAME::FocusLibraryTreeInput()
{
    GetLibTree()->FocusSearchFieldIfExists();
}


void SYMBOL_EDIT_FRAME::FreezeLibraryTree()
{
    m_treePane->Freeze();
    m_libMgr->GetAdapter()->Freeze();
}


void SYMBOL_EDIT_FRAME::ThawLibraryTree()
{
    m_libMgr->GetAdapter()->Thaw();
    m_treePane->Thaw();
}


void SYMBOL_EDIT_FRAME::OnExitKiCad( wxCommandEvent& event )
{
    Kiway().OnKiCadExit();
}


void SYMBOL_EDIT_FRAME::OnUpdateUnitNumber( wxUpdateUIEvent& event )
{
    event.Enable( m_symbol && m_symbol->GetUnitCount() > 1 );
}


void SYMBOL_EDIT_FRAME::OnSelectUnit( wxCommandEvent& event )
{
    int i = event.GetSelection();

    if( i == wxNOT_FOUND )
        return;

    SetUnit( i + 1 );
}


bool SYMBOL_EDIT_FRAME::IsSymbolFromLegacyLibrary() const
{
    if( m_symbol )
    {
        SYMBOL_LIB_TABLE_ROW* row = m_libMgr->GetLibrary( m_symbol->GetLibNickname() );

        if( row && row->GetType() == SCH_IO_MGR::ShowType( SCH_IO_MGR::SCH_LEGACY ) )
            return true;
    }

    return false;
}


wxString SYMBOL_EDIT_FRAME::GetCurLib() const
{
    wxString libNickname = Prj().GetRString( PROJECT::SCH_LIBEDIT_CUR_LIB );

    if( !libNickname.empty() )
    {
        if( !PROJECT_SCH::SchSymbolLibTable( &Prj() )->HasLibrary( libNickname ) )
        {
            Prj().SetRString( PROJECT::SCH_LIBEDIT_CUR_LIB, wxEmptyString );
            libNickname = wxEmptyString;
        }
    }

    return libNickname;
}


wxString SYMBOL_EDIT_FRAME::SetCurLib( const wxString& aLibNickname )
{
    wxString old = GetCurLib();

    if( aLibNickname.empty() || !PROJECT_SCH::SchSymbolLibTable( &Prj() )->HasLibrary( aLibNickname ) )
        Prj().SetRString( PROJECT::SCH_LIBEDIT_CUR_LIB, wxEmptyString );
    else
        Prj().SetRString( PROJECT::SCH_LIBEDIT_CUR_LIB, aLibNickname );

    return old;
}


void SYMBOL_EDIT_FRAME::SetCurSymbol( LIB_SYMBOL* aSymbol, bool aUpdateZoom )
{
    wxCHECK( m_toolManager, /* void */ );

    m_toolManager->RunAction( EE_ACTIONS::clearSelection );
    GetCanvas()->GetView()->Clear();
    delete m_symbol;

    m_symbol = aSymbol;

    // select the current symbol in the tree widget
    if( !IsSymbolFromSchematic() && m_symbol )
        GetLibTree()->SelectLibId( m_symbol->GetLibId() );
    else
        GetLibTree()->Unselect();

    wxString symbolName;
    wxString libName;

    if( m_symbol )
    {
        symbolName = m_symbol->GetName();
        libName = UnescapeString( m_symbol->GetLibId().GetLibNickname() );
    }

    // retain in case this wxFrame is re-opened later on the same PROJECT
    Prj().SetRString( PROJECT::SCH_LIBEDIT_CUR_SYMBOL, symbolName );

    // Ensure synchronized pin edit can be enabled only symbols with interchangeable units
    m_SyncPinEdit = aSymbol && aSymbol->IsRoot() && aSymbol->IsMulti() && !aSymbol->UnitsLocked();

    m_toolManager->ResetTools( TOOL_BASE::MODEL_RELOAD );

    GetRenderSettings()->m_ShowUnit = m_unit;
    GetRenderSettings()->m_ShowBodyStyle = m_bodyStyle;
    GetRenderSettings()->m_ShowDisabled = IsSymbolFromLegacyLibrary() && !IsSymbolFromSchematic();
    GetRenderSettings()->m_ShowGraphicsDisabled = IsSymbolAlias() && !IsSymbolFromSchematic();
    GetCanvas()->DisplaySymbol( m_symbol );
    GetCanvas()->GetView()->HideDrawingSheet();
    GetCanvas()->GetView()->ClearHiddenFlags();

    if( aUpdateZoom )
        m_toolManager->RunAction( ACTIONS::zoomFitScreen );

    GetCanvas()->Refresh();

    WX_INFOBAR& infobar = *GetInfoBar();
    infobar.RemoveAllButtons();

    wxArrayString msgs;
    int           infobarFlags = wxICON_INFORMATION;

    if( IsSymbolFromSchematic() )
    {
        msgs.push_back( wxString::Format( _( "Editing symbol %s from schematic.  Saving will "
                                             "update the schematic only." ),
                                          m_reference ) );

        wxString         link = wxString::Format( _( "Open symbol from library %s" ), libName );
        wxHyperlinkCtrl* button = new wxHyperlinkCtrl( &infobar, wxID_ANY, link, wxEmptyString );

        button->Bind( wxEVT_COMMAND_HYPERLINK, std::function<void( wxHyperlinkEvent& aEvent )>(
                [this, symbolName, libName]( wxHyperlinkEvent& aEvent )
                {
                    GetToolManager()->RunAction( EE_ACTIONS::editLibSymbolWithLibEdit );
                } ) );

        infobar.AddButton( button );
    }
    else if( IsSymbolFromLegacyLibrary() )
    {
        msgs.push_back( _( "Symbols in legacy libraries are not editable.  Use Manage Symbol "
                           "Libraries to migrate to current format." ) );

        wxString         link = _( "Manage symbol libraries" );
        wxHyperlinkCtrl* button = new wxHyperlinkCtrl( &infobar, wxID_ANY, link, wxEmptyString );

        button->Bind( wxEVT_COMMAND_HYPERLINK, std::function<void( wxHyperlinkEvent& aEvent )>(
                [this]( wxHyperlinkEvent& aEvent )
                {
                    InvokeSchEditSymbolLibTable( &Kiway(), this );
                } ) );

        infobar.AddButton( button );
    }
    else if( IsSymbolAlias() )
    {
        msgs.push_back( wxString::Format( _( "Symbol %s is a derived symbol. Symbol graphics will "
                                             "not be editable." ),
                                          UnescapeString( symbolName ) ) );

        // Don't assume the parent symbol shared pointer is still valid.
        if( std::shared_ptr<LIB_SYMBOL> rootSymbol = m_symbol->GetRootSymbol() )
        {
            int      unit = GetUnit();
            int      bodyStyle = GetBodyStyle();
            wxString rootSymbolName = rootSymbol->GetName();
            wxString link = wxString::Format( _( "Open %s" ), UnescapeString( rootSymbolName ) );

            wxHyperlinkCtrl* button = new wxHyperlinkCtrl( &infobar, wxID_ANY, link,
                                                           wxEmptyString );

            button->Bind( wxEVT_COMMAND_HYPERLINK, std::function<void( wxHyperlinkEvent& aEvent )>(
                    [this, rootSymbolName, unit, bodyStyle]( wxHyperlinkEvent& aEvent )
                    {
                        LoadSymbolFromCurrentLib( rootSymbolName, unit, bodyStyle );
                    } ) );

            infobar.AddButton( button );
        }
    }

    if( m_symbol
            && !IsSymbolFromSchematic()
            && m_libMgr->IsLibraryReadOnly( m_symbol->GetLibId().GetFullLibraryName() ) )
    {
        msgs.push_back( _( "Library is read-only.  Changes cannot be saved to this library." ) );

        wxString         link = wxString::Format( _( "Create an editable copy" ) );
        wxHyperlinkCtrl* button = new wxHyperlinkCtrl( &infobar, wxID_ANY, link, wxEmptyString );

        button->Bind( wxEVT_COMMAND_HYPERLINK, std::function<void( wxHyperlinkEvent& aEvent )>(
                [this, symbolName, libName]( wxHyperlinkEvent& aEvent )
                {
                    wxString msg = wxString::Format( _( "Create an editable copy of the symbol or "
                                                        "the entire library (%s)?" ),
                                                     libName );

                    KIDIALOG errorDlg( this, msg, _( "Select type of item to save" ),
                                       wxYES_NO | wxCANCEL | wxICON_QUESTION );
                    // These buttons are in a weird order(?)
                    errorDlg.SetYesNoCancelLabels( _( "Copy symbol" ), _( "Cancel" ),
                                                   _( "Copy library" ) );

                    int choice = errorDlg.ShowModal();

                    switch( choice )
                    {
                    case wxID_YES:
                        SaveSymbolCopyAs( true );
                        break;
                    case wxID_CANCEL:
                        SaveLibraryAs();
                        break;
                    default:
                        // Do nothing
                        break;
                    }
                } ) );

        infobar.AddButton( button );
    }

    if( msgs.empty() )
    {
        infobar.Dismiss();
    }
    else
    {
        wxString msg = wxJoin( msgs, '\n', '\0' );
        infobar.ShowMessage( msg, infobarFlags );
    }
}


LIB_SYMBOL_LIBRARY_MANAGER& SYMBOL_EDIT_FRAME::GetLibManager()
{
    wxASSERT( m_libMgr );
    return *m_libMgr;
}


void SYMBOL_EDIT_FRAME::OnModify()
{
    EDA_BASE_FRAME::OnModify();

    GetScreen()->SetContentModified();
    m_autoSaveRequired = true;

    if( !IsSymbolFromSchematic() )
        storeCurrentSymbol();

    GetLibTree()->RefreshLibTree();

    if( !GetTitle().StartsWith( "*" ) )
        UpdateTitle();
}


void SYMBOL_EDIT_FRAME::SetUnit( int aUnit )
{
    wxCHECK( aUnit > 0 && aUnit <= GetCurSymbol()->GetUnitCount(), /* void*/ );

    if( m_unit == aUnit )
        return;

    m_toolManager->RunAction( ACTIONS::cancelInteractive );
    m_toolManager->RunAction( EE_ACTIONS::clearSelection );

    m_unit = aUnit;

    if( m_unitSelectBox->GetSelection() != ( m_unit - 1 ) )
        m_unitSelectBox->SetSelection( m_unit - 1 );

    m_toolManager->ResetTools( TOOL_BASE::MODEL_RELOAD );
    RebuildView();
    UpdateSymbolMsgPanelInfo();
}


bool SYMBOL_EDIT_FRAME::SynchronizePins()
{
    return m_SyncPinEdit && m_symbol && m_symbol->IsMulti() && !m_symbol->UnitsLocked();
}


wxString SYMBOL_EDIT_FRAME::AddLibraryFile( bool aCreateNew )
{
    // Select the target library table (global/project)
    SYMBOL_LIB_TABLE* libTable = SelectSymLibTable();

    if( !libTable )
        return wxEmptyString;

    wxFileName fn = m_libMgr->GetUniqueLibraryName();

    if( !LibraryFileBrowser( !aCreateNew, fn, FILEEXT::KiCadSymbolLibFileWildcard(),
                             FILEEXT::KiCadSymbolLibFileExtension, false,
                             ( libTable == &SYMBOL_LIB_TABLE::GetGlobalLibTable() ),
                             PATHS::GetDefaultUserSymbolsPath() ) )
    {
        return wxEmptyString;
    }

    wxString libName = fn.GetName();

    if( libName.IsEmpty() )
        return wxEmptyString;

    if( m_libMgr->LibraryExists( libName ) )
    {
        DisplayError( this, wxString::Format( _( "Library '%s' already exists." ), libName ) );
        return wxEmptyString;
    }

    if( aCreateNew )
    {
        if( !m_libMgr->CreateLibrary( fn.GetFullPath(), *libTable ) )
        {
            DisplayError( this, wxString::Format( _( "Could not create the library file '%s'.\n"
                                                     "Make sure you have write permissions and "
                                                     "try again." ),
                                                  fn.GetFullPath() ) );
            return wxEmptyString;
        }
    }
    else
    {
        if( !m_libMgr->AddLibrary( fn.GetFullPath(), *libTable ) )
        {
            DisplayError( this, _( "Could not open the library file." ) );
            return wxEmptyString;
        }
    }

    bool globalTable = ( libTable == &SYMBOL_LIB_TABLE::GetGlobalLibTable() );
    saveSymbolLibTables( globalTable, !globalTable );

    std::string packet = fn.GetFullPath().ToStdString();
    this->Kiway().ExpressMail( FRAME_SCH_SYMBOL_EDITOR, MAIL_LIB_EDIT, packet );

    return fn.GetFullPath();
}


void SYMBOL_EDIT_FRAME::DdAddLibrary( wxString aLibFile )
{
        // Select the target library table (global/project)
    SYMBOL_LIB_TABLE* libTable = SelectSymLibTable();

    if( !libTable )
        return;

    wxFileName fn = wxFileName( aLibFile );

    wxString libName = fn.GetName();

    if( libName.IsEmpty() )
        return;

    if( m_libMgr->LibraryExists( libName ) )
    {
        DisplayError( this, wxString::Format( _( "Library '%s' already exists." ), libName ) );
        return;
    }

    if( !m_libMgr->AddLibrary( fn.GetFullPath(), *libTable ) )
    {
        DisplayError( this, _( "Could not open the library file." ) );
        return;
    }

    bool globalTable = ( libTable == &SYMBOL_LIB_TABLE::GetGlobalLibTable() );
    saveSymbolLibTables( globalTable, !globalTable );

    std::string packet = fn.GetFullPath().ToStdString();
    this->Kiway().ExpressMail( FRAME_SCH_SYMBOL_EDITOR, MAIL_LIB_EDIT, packet );
}


LIB_ID SYMBOL_EDIT_FRAME::GetTreeLIBID( int* aUnit ) const
{
    return GetLibTree()->GetSelectedLibId( aUnit );
}


int SYMBOL_EDIT_FRAME::GetTreeSelectionCount() const
{
    return GetLibTree()->GetSelectionCount();
}

int SYMBOL_EDIT_FRAME::GetTreeLIBIDs( std::vector<LIB_ID>& aSelection ) const
{
    return GetLibTree()->GetSelectedLibIds( aSelection );
}


LIB_SYMBOL* SYMBOL_EDIT_FRAME::getTargetSymbol() const
{
    if( IsLibraryTreeShown() )
    {
        LIB_ID libId = GetTreeLIBID();

        if( libId.IsValid() )
            return m_libMgr->GetAlias( libId.GetLibItemName(), libId.GetLibNickname() );
    }

    return m_symbol;
}


LIB_ID SYMBOL_EDIT_FRAME::GetTargetLibId() const
{
    LIB_ID id;

    if( IsLibraryTreeShown() )
        id = GetTreeLIBID();

    if( id.GetLibNickname().empty() && m_symbol )
        id = m_symbol->GetLibId();

    return id;
}


std::vector<LIB_ID> SYMBOL_EDIT_FRAME::GetSelectedLibIds() const
{
    std::vector<LIB_ID> ids;
    GetTreeLIBIDs( ids );
    return ids;
}


wxString SYMBOL_EDIT_FRAME::getTargetLib() const
{
    return GetTargetLibId().GetLibNickname();
}


void SYMBOL_EDIT_FRAME::SyncLibraries( bool aShowProgress, bool aPreloadCancelled,
                                       const wxString& aForceRefresh )
{
    LIB_ID selected;

    if( m_treePane )
        selected = GetLibTree()->GetSelectedLibId();

    if( aShowProgress )
    {
        APP_PROGRESS_DIALOG progressDlg( _( "Loading Symbol Libraries" ), wxEmptyString,
                                         m_libMgr->GetAdapter()->GetLibrariesCount(), this );

        m_libMgr->Sync( aForceRefresh,
                [&]( int progress, int max, const wxString& libName )
                {
                    progressDlg.Update( progress, wxString::Format( _( "Loading library '%s'..." ),
                                                                    libName ) );
                } );
    }
    else if( !aPreloadCancelled )
    {
        m_libMgr->Sync( aForceRefresh,
                [&]( int progress, int max, const wxString& libName )
                {
                } );
    }

    if( m_treePane )
    {
        wxDataViewItem found;

        if( selected.IsValid() )
        {
            // Check if the previously selected item is still valid,
            // if not - it has to be unselected to prevent crash
            found = m_libMgr->GetAdapter()->FindItem( selected );

            if( !found )
                GetLibTree()->Unselect();
        }

        GetLibTree()->Regenerate( true );

        // Try to select the parent library, in case the symbol is not found
        if( !found && selected.IsValid() )
        {
            selected.SetLibItemName( "" );
            found = m_libMgr->GetAdapter()->FindItem( selected );

            if( found )
                GetLibTree()->SelectLibId( selected );
        }

        // If no selection, see if there's a current symbol to centre
        if( !selected.IsValid() && m_symbol )
        {
            LIB_ID current( GetCurLib(), m_symbol->GetName() );
            GetLibTree()->CenterLibId( current );
        }
    }
}


void SYMBOL_EDIT_FRAME::RefreshLibraryTree()
{
    GetLibTree()->RefreshLibTree();
}


void SYMBOL_EDIT_FRAME::FocusOnLibId( const LIB_ID& aLibID )
{
    GetLibTree()->SelectLibId( aLibID );
}


void SYMBOL_EDIT_FRAME::UpdateLibraryTree( const wxDataViewItem& aTreeItem, LIB_SYMBOL* aSymbol )
{
    if( aTreeItem.IsOk() )   // Can be not found in tree if the current footprint is imported
                             // from file therefore not yet in tree.
    {
        static_cast<LIB_TREE_NODE_ITEM*>( aTreeItem.GetID() )->Update( aSymbol );
        GetLibTree()->RefreshLibTree();
    }
}


bool SYMBOL_EDIT_FRAME::backupFile( const wxFileName& aOriginalFile, const wxString& aBackupExt )
{
    if( aOriginalFile.FileExists() )
    {
        wxFileName backupFileName( aOriginalFile );
        backupFileName.SetExt( aBackupExt );

        if( backupFileName.FileExists() )
            wxRemoveFile( backupFileName.GetFullPath() );

        if( !wxCopyFile( aOriginalFile.GetFullPath(), backupFileName.GetFullPath() ) )
        {
            DisplayError( this, wxString::Format( _( "Failed to save backup to '%s'." ),
                                                  backupFileName.GetFullPath() ) );
            return false;
        }
    }

    return true;
}


void SYMBOL_EDIT_FRAME::storeCurrentSymbol()
{
    if( m_symbol && !GetCurLib().IsEmpty() && GetScreen()->IsContentModified() )
        m_libMgr->UpdateSymbol( m_symbol, GetCurLib() ); // UpdateSymbol() makes a copy
}


bool SYMBOL_EDIT_FRAME::IsCurrentSymbol( const LIB_ID& aLibId ) const
{
    // This will return the root symbol of any alias
    LIB_SYMBOL* symbol = m_libMgr->GetBufferedSymbol( aLibId.GetLibItemName(),
                                                      aLibId.GetLibNickname() );

    // Now we can compare the libId of the current symbol and the root symbol
    return ( symbol && m_symbol && symbol->GetLibId() == m_symbol->GetLibId() );
}


void SYMBOL_EDIT_FRAME::emptyScreen()
{
    GetLibTree()->Unselect();
    SetCurLib( wxEmptyString );
    SetCurSymbol( nullptr, false );
    SetScreen( m_dummyScreen );
    ClearUndoRedoList();
    m_toolManager->RunAction( ACTIONS::zoomFitScreen );
    Refresh();
}


void SYMBOL_EDIT_FRAME::CommonSettingsChanged( int aFlags )
{
    SCH_BASE_FRAME::CommonSettingsChanged( aFlags );

    SETTINGS_MANAGER*       mgr = GetSettingsManager();
    SYMBOL_EDITOR_SETTINGS* cfg = mgr->GetAppSettings<SYMBOL_EDITOR_SETTINGS>( "symbol_editor" );

    GetRenderSettings()->m_ShowPinsElectricalType = cfg->m_ShowPinElectricalType;
    GetRenderSettings()->m_ShowHiddenPins = cfg->m_ShowHiddenPins;
    GetRenderSettings()->m_ShowHiddenFields = cfg->m_ShowHiddenFields;
    GetRenderSettings()->m_ShowPinAltIcons = cfg->m_ShowPinAltIcons;

    GetGalDisplayOptions().ReadWindowSettings( cfg->m_Window );

    if( m_symbol )
        m_symbol->ClearCaches();

    GetCanvas()->ForceRefresh();

    GetCanvas()->GetView()->UpdateAllItems( KIGFX::ALL );
    GetCanvas()->Refresh();

    RecreateToolbars();

    if( aFlags & ENVVARS_CHANGED )
        SyncLibraries( true );

    Layout();
    SendSizeEvent();
}


void SYMBOL_EDIT_FRAME::ShowChangedLanguage()
{
    // call my base class
    SCH_BASE_FRAME::ShowChangedLanguage();

    // tooltips in toolbars
    RecreateToolbars();

    // For some obscure reason, the AUI manager hides the first modified pane.
    // So force show panes
    wxAuiPaneInfo& tree_pane_info = m_auimgr.GetPane( m_treePane );
    bool tree_shown = tree_pane_info.IsShown();
    tree_pane_info.Caption( _( "Libraries" ) );
    tree_pane_info.Show( tree_shown );
    m_auimgr.Update();

    GetLibTree()->ShowChangedLanguage();

    // status bar
    UpdateMsgPanel();

    if( GetRenderSettings()->m_ShowPinsElectricalType )
    {
        GetCanvas()->GetView()->UpdateAllItems( KIGFX::ALL );
        GetCanvas()->Refresh();
    }

    UpdateTitle();
}


void SYMBOL_EDIT_FRAME::SetScreen( BASE_SCREEN* aScreen )
{
    SCH_BASE_FRAME::SetScreen( aScreen );

    // Let tools add things to the view if necessary
    if( m_toolManager )
        m_toolManager->ResetTools( TOOL_BASE::MODEL_RELOAD );
}


void SYMBOL_EDIT_FRAME::RebuildView()
{
    GetRenderSettings()->m_ShowUnit = m_unit;
    GetRenderSettings()->m_ShowBodyStyle = m_bodyStyle;
    GetRenderSettings()->m_ShowDisabled = IsSymbolFromLegacyLibrary() && !IsSymbolFromSchematic();
    GetRenderSettings()->m_ShowGraphicsDisabled = IsSymbolAlias() && !IsSymbolFromSchematic();
    GetCanvas()->DisplaySymbol( m_symbol );
    GetCanvas()->GetView()->HideDrawingSheet();
    GetCanvas()->GetView()->ClearHiddenFlags();

    // Let tools add things to the view if necessary
    if( m_toolManager )
        m_toolManager->ResetTools( TOOL_BASE::MODEL_RELOAD );

    GetCanvas()->GetView()->UpdateAllItems( KIGFX::ALL );
    GetCanvas()->Refresh();
}


void SYMBOL_EDIT_FRAME::HardRedraw()
{
    SyncLibraries( true );

    if( m_symbol )
    {
        EE_SELECTION_TOOL* selectionTool = m_toolManager->GetTool<EE_SELECTION_TOOL>();
        EE_SELECTION&      selection = selectionTool->GetSelection();

        for( SCH_ITEM& item : m_symbol->GetDrawItems() )
        {
            if( !alg::contains( selection, &item ) )
                item.ClearSelected();
            else
                item.SetSelected();
        }

        m_symbol->ClearCaches();
    }

    RebuildView();
}


const BOX2I SYMBOL_EDIT_FRAME::GetDocumentExtents( bool aIncludeAllVisible ) const
{
    if( !m_symbol )
    {
        // Gives a reasonable drawing area size
        int width = schIUScale.mmToIU( 50 );
        int height = schIUScale.mmToIU( 30 );

        return BOX2I( VECTOR2I( -width/2, -height/2 ), VECTOR2I( width, height ) );
    }
    else
    {
        return m_symbol->Flatten()->GetUnitBoundingBox( m_unit, m_bodyStyle );
    }
}


void SYMBOL_EDIT_FRAME::FocusOnItem( SCH_ITEM* aItem )
{
    static KIID lastBrightenedItemID( niluuid );

    SCH_ITEM* lastItem = nullptr;

    if( m_symbol )
    {
        for( SCH_PIN* pin : m_symbol->GetPins() )
        {
            if( pin->m_Uuid == lastBrightenedItemID )
                lastItem = pin;
        }

        std::vector<SCH_FIELD*> fields;
        m_symbol->GetFields( fields );

        for( SCH_FIELD* field : fields )
        {
            if( field->m_Uuid == lastBrightenedItemID )
                lastItem = field;
        }
    }

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

        FocusOnLocation( VECTOR2I( aItem->GetFocusPosition().x, -aItem->GetFocusPosition().y ) );
    }
}


void SYMBOL_EDIT_FRAME::KiwayMailIn( KIWAY_EXPRESS& mail )
{
    const std::string& payload = mail.GetPayload();

    switch( mail.Command() )
    {
    case MAIL_LIB_EDIT:
        if( !payload.empty() )
        {
            wxString libFileName( payload );
            wxString libNickname;
            wxString msg;

            SYMBOL_LIB_TABLE*    libTable = PROJECT_SCH::SchSymbolLibTable( &Prj() );
            const LIB_TABLE_ROW* libTableRow = libTable->FindRowByURI( libFileName );

            if( !libTableRow )
            {
                msg.Printf( _( "The current configuration does not include the symbol library '%s'." ),
                            libFileName );
                msg += wxS( "\n" ) + _( "Use Manage Symbol Libraries to edit the configuration." );
                DisplayErrorMessage( this, _( "Library not found in symbol library table." ), msg );
                break;
            }

            libNickname = libTableRow->GetNickName();

            if( !libTable->HasLibrary( libNickname, true ) )
            {
                msg.Printf( _( "The symbol library '%s' is not enabled in the current configuration." ),
                            UnescapeString( libNickname ) );
                msg += wxS( "\n" ) + _( "Use Manage Symbol Libraries to edit the configuration." );
                DisplayErrorMessage( this, _( "Symbol library not enabled." ), msg );
                break;
            }

            SetCurLib( libNickname );

            if( m_treePane )
            {
                LIB_ID id( libNickname, wxEmptyString );
                GetLibTree()->SelectLibId( id );
                GetLibTree()->ExpandLibId( id );
                GetLibTree()->CenterLibId( id );
            }
        }

        break;

    case MAIL_RELOAD_LIB:
    {
        wxString          currentLib = GetCurLib();
        SYMBOL_LIB_TABLE* libTable = PROJECT_SCH::SchSymbolLibTable( &Prj() );

        FreezeLibraryTree();

        // Check if the currently selected symbol library been removed or disabled.
        if( !currentLib.empty() && libTable && !libTable->HasLibrary( currentLib, true ) )
        {
            SetCurLib( wxEmptyString );
            emptyScreen();
        }

        SyncLibraries( true );
        ThawLibraryTree();
        RefreshLibraryTree();

        break;
    }

    case MAIL_REFRESH_SYMBOL:
    {
        SYMBOL_LIB_TABLE* tbl = PROJECT_SCH::SchSymbolLibTable( &Prj() );
        LIB_SYMBOL* symbol = GetCurSymbol();

        wxLogTrace( "KICAD_LIB_WATCH", "Received refresh symbol request for %s",
                    payload );

        if( !tbl || !symbol )
            break;

        wxString libName = symbol->GetLibId().GetLibNickname();
        const SYMBOL_LIB_TABLE_ROW* row = tbl->FindRow( libName );

        if( !row )
            return;

        wxFileName libfullname( row->GetFullURI( true ) );

        wxFileName changedLib( mail.GetPayload() );
        wxLogTrace( "KICAD_LIB_WATCH",
                    "Received refresh symbol request for %s, current symbols is %s",
                    changedLib.GetFullPath(), libfullname.GetFullPath() );

        if( changedLib == libfullname )
        {
            wxLogTrace( "KICAD_LIB_WATCH", "Refreshing symbol %s", symbol->GetName() );

            SetScreen( m_dummyScreen );  // UpdateLibraryBuffer will destroy the old screen
            m_libMgr->UpdateLibraryBuffer( libName );

            LIB_SYMBOL* lib_symbol = m_libMgr->GetBufferedSymbol( symbol->GetName(), libName );
            wxCHECK2_MSG( lib_symbol, break, wxString::Format( "Symbol %s not found in library %s",
                          symbol->GetName(), libName ) );

            // The buffered screen for the symbol
            SCH_SCREEN* symbol_screen = m_libMgr->GetScreen( lib_symbol->GetName(), libName );

            SetScreen( symbol_screen );
            SetCurSymbol( new LIB_SYMBOL( *lib_symbol ), false );
            RebuildSymbolUnitsList();
            SetShowDeMorgan( GetCurSymbol()->HasAlternateBodyStyle() );

            if( m_toolManager )
                m_toolManager->ResetTools( TOOL_BASE::MODEL_RELOAD );
        }

        break;
    }

    default:
        ;
    }
}


std::unique_ptr<GRID_HELPER> SYMBOL_EDIT_FRAME::MakeGridHelper()
{
    return std::make_unique<EE_GRID_HELPER>( m_toolManager );
}


void SYMBOL_EDIT_FRAME::SwitchCanvas( EDA_DRAW_PANEL_GAL::GAL_TYPE aCanvasType )
{
    // switches currently used canvas ( Cairo / OpenGL):
    SCH_BASE_FRAME::SwitchCanvas( aCanvasType );

    // Set options specific to symbol editor (axies are always enabled):
    GetCanvas()->GetGAL()->SetAxesEnabled( true );
    GetCanvas()->GetGAL()->SetAxesColor( m_colorSettings->GetColor( LAYER_SCHEMATIC_GRID_AXES ) );
}


bool SYMBOL_EDIT_FRAME::HasLibModifications() const
{
    wxCHECK( m_libMgr, false );

    return m_libMgr->HasModifications();
}


bool SYMBOL_EDIT_FRAME::IsContentModified() const
{
    wxCHECK( m_libMgr, false );

    // Test if the currently edited symbol is modified
    if( GetScreen() && GetScreen()->IsContentModified() && GetCurSymbol() )
        return true;

    // Test if any library has been modified
    for( const wxString& libName : m_libMgr->GetLibraryNames() )
    {
        if( m_libMgr->IsLibraryModified( libName ) && !m_libMgr->IsLibraryReadOnly( libName ) )
            return true;
    }

    return false;
}


void SYMBOL_EDIT_FRAME::ClearUndoORRedoList( UNDO_REDO_LIST whichList, int aItemCount )
{
    if( aItemCount == 0 )
        return;

    UNDO_REDO_CONTAINER& list = ( whichList == UNDO_LIST ) ? m_undoList : m_redoList;

    if( aItemCount < 0 )
    {
        list.ClearCommandList();
    }
    else
    {
        for( int ii = 0; ii < aItemCount; ii++ )
        {
            if( list.m_CommandsList.size() == 0 )
                break;

            PICKED_ITEMS_LIST* curr_cmd = list.m_CommandsList[0];
            list.m_CommandsList.erase( list.m_CommandsList.begin() );

            curr_cmd->ClearListAndDeleteItems( []( EDA_ITEM* aItem )
                                               {
                                                   delete aItem;
                                               } );
            delete curr_cmd;    // Delete command
        }
    }
}


SELECTION& SYMBOL_EDIT_FRAME::GetCurrentSelection()
{
    return m_toolManager->GetTool<EE_SELECTION_TOOL>()->GetSelection();
}


void SYMBOL_EDIT_FRAME::LoadSymbolFromSchematic( SCH_SYMBOL* aSymbol )
{
    std::unique_ptr<LIB_SYMBOL> symbol = aSymbol->GetLibSymbolRef()->Flatten();
    wxCHECK( symbol, /* void */ );

    symbol->SetLibId( aSymbol->GetLibId() );

    // Take in account the symbol orientation and mirroring. to calculate the field
    // positions in symbol editor (i.e. no rotation, no mirroring)
    int orientation = aSymbol->GetOrientation() & ~( SYM_MIRROR_X | SYM_MIRROR_Y );
    int mirror = aSymbol->GetOrientation() & ( SYM_MIRROR_X | SYM_MIRROR_Y );

    std::vector<SCH_FIELD> fullSetOfFields;

    for( int i = 0; i < (int) aSymbol->GetFields().size(); ++i )
    {
        const SCH_FIELD& field = aSymbol->GetFields()[i];
        VECTOR2I         pos = field.GetPosition() - aSymbol->GetPosition();
        SCH_FIELD        libField( symbol.get(), field.GetId() );

        libField = field;

        // The inverse transform is mirroring before, rotate after
        switch( mirror )
        {
        case SYM_MIRROR_X: pos.y = -pos.y; break;
        case SYM_MIRROR_Y: pos.x = -pos.x; break;
        default:                           break;
        }

        switch( orientation )
        {
        case SYM_ORIENT_90:
            std::swap( pos.x, pos.y );
            pos.x = - pos.x;
            break;
        case SYM_ORIENT_270:
            std::swap( pos.x, pos.y );
            pos.y = - pos.y;
            break;
        case SYM_ORIENT_180:
            pos.x = - pos.x;
            pos.y = - pos.y;
            break;
        default:
            break;
        }

        libField.SetPosition( pos );

        fullSetOfFields.emplace_back( std::move( libField ) );
    }

    symbol->SetFields( fullSetOfFields );

    if( m_symbol )
        SetCurSymbol( nullptr, false );

    m_isSymbolFromSchematic = true;
    m_schematicSymbolUUID = aSymbol->m_Uuid;
    m_reference = symbol->GetFieldById( REFERENCE_FIELD )->GetText();
    m_unit = std::max( 1, aSymbol->GetUnit() );
    m_bodyStyle = std::max( 1, aSymbol->GetBodyStyle() );

    // Optimize default edit options for this symbol
    // Usually if units are locked, graphic items are specific to each unit
    // and if units are interchangeable, graphic items are common to units
    SYMBOL_EDITOR_DRAWING_TOOLS* tools = GetToolManager()->GetTool<SYMBOL_EDITOR_DRAWING_TOOLS>();
    tools->SetDrawSpecificUnit( symbol->UnitsLocked() );

    // The buffered screen for the symbol
    SCH_SCREEN* tmpScreen = new SCH_SCREEN();

    SetScreen( tmpScreen );
    SetCurSymbol( symbol.release(), true );
    setSymWatcher( nullptr );

    ReCreateMenuBar();
    ReCreateHToolbar();

    if( IsLibraryTreeShown() )
        ToggleLibraryTree();

    UpdateTitle();
    RebuildSymbolUnitsList();
    SetShowDeMorgan( GetCurSymbol()->HasAlternateBodyStyle() );
    UpdateSymbolMsgPanelInfo();

    // Let tools add things to the view if necessary
    if( m_toolManager )
        m_toolManager->ResetTools( TOOL_BASE::MODEL_RELOAD );

    GetCanvas()->GetView()->UpdateAllItems( KIGFX::ALL );
    GetCanvas()->Refresh();
}


bool SYMBOL_EDIT_FRAME::addLibTableEntry( const wxString& aLibFile, TABLE_SCOPE aScope )
{
    wxFileName fn = aLibFile;
    wxFileName libTableFileName( Prj().GetProjectPath(),
                                 SYMBOL_LIB_TABLE::GetSymbolLibTableFileName() );
    wxString libNickname = fn.GetName();
    SYMBOL_LIB_TABLE* libTable = PROJECT_SCH::SchSymbolLibTable( &Prj() );
    const ENV_VAR_MAP& envVars = Pgm().GetLocalEnvVariables();

    if( libTable->HasLibrary( libNickname ) )
    {
        wxString tmp;
        int suffix = 1;

        while( libTable->HasLibrary( libNickname ) )
        {
            tmp.Printf( "%s%d", fn.GetName(), suffix );
            libNickname = tmp;
            suffix += 1;
        }
    }

    SYMBOL_LIB_TABLE_ROW* row = new SYMBOL_LIB_TABLE_ROW();
    row->SetNickName( libNickname );

    wxString normalizedPath = NormalizePath( aLibFile, &envVars, Prj().GetProjectPath() );

    if( aScope == GLOBAL_LIB_TABLE )
    {
        libTable = &SYMBOL_LIB_TABLE::GetGlobalLibTable();
        libTableFileName = SYMBOL_LIB_TABLE::GetGlobalTableFileName();

        // We cannot normalize against the current project path when saving to global table.
        normalizedPath = NormalizePath( aLibFile, &envVars, wxEmptyString );
    }

    row->SetFullURI( normalizedPath );

    wxCHECK( libTable->InsertRow( row ), false );

    try
    {
        libTable->Save( libTableFileName.GetFullPath() );
    }
    catch( const IO_ERROR& ioe )
    {
        wxString msg = aScope == GLOBAL_LIB_TABLE ? _( "Error saving global library table." )
                                                  : _( "Error saving project library table." );

        wxMessageDialog dlg( this, msg, _( "File Save Error" ), wxOK | wxICON_ERROR );
        dlg.SetExtendedMessage( ioe.What() );
        dlg.ShowModal();

        return false;
    }

    return true;
}


bool SYMBOL_EDIT_FRAME::replaceLibTableEntry( const wxString& aLibNickname,
                                              const wxString& aLibFile )
{
    // Check the global library table first because checking the project library table
    // checks the global library table as well due to library chaining.
    bool isGlobalTable = true;
    wxFileName libTableFileName = SYMBOL_LIB_TABLE::GetGlobalTableFileName();;
    const ENV_VAR_MAP& envVars = Pgm().GetLocalEnvVariables();
    SYMBOL_LIB_TABLE* libTable = &SYMBOL_LIB_TABLE::GetGlobalLibTable();
    SYMBOL_LIB_TABLE_ROW* row = libTable->FindRow( aLibNickname );

    if( !row )
    {
        libTableFileName.SetPath( Prj().GetProjectPath() );
        libTableFileName.SetName( SYMBOL_LIB_TABLE::GetSymbolLibTableFileName() );
        libTable = PROJECT_SCH::SchSymbolLibTable( &Prj() );
        isGlobalTable = false;
        row = libTable->FindRow( aLibNickname );
    }

    wxCHECK( row, false );

    wxString projectPath;

    if( !isGlobalTable )
        projectPath = Prj().GetProjectPath();

    wxString normalizedPath = NormalizePath( aLibFile, &envVars, projectPath );

    row->SetFullURI( normalizedPath );
    row->SetType( "KiCad" );

    try
    {
        libTable->Save( libTableFileName.GetFullPath() );
    }
    catch( const IO_ERROR& ioe )
    {
        wxString msg = isGlobalTable ? _( "Error saving global library table." )
                                     : _( "Error saving project library table." );

        wxMessageDialog dlg( this, msg, _( "File Save Error" ), wxOK | wxICON_ERROR );
        dlg.SetExtendedMessage( ioe.What() );
        dlg.ShowModal();

        return false;
    }

    return true;
}


bool SYMBOL_EDIT_FRAME::IsSymbolAlias() const
{
    return m_symbol && !m_symbol->IsRoot();
}


bool SYMBOL_EDIT_FRAME::IsSymbolEditable() const
{
    return m_symbol && ( !IsSymbolFromLegacyLibrary() || IsSymbolFromSchematic() );
}


void SYMBOL_EDIT_FRAME::UpdateItem( EDA_ITEM* aItem, bool isAddOrDelete, bool aUpdateRtree )
{
    SCH_BASE_FRAME::UpdateItem( aItem, isAddOrDelete, aUpdateRtree );

    if( EDA_TEXT* eda_text = dynamic_cast<EDA_TEXT*>( aItem ) )
    {
        eda_text->ClearBoundingBoxCache();
        eda_text->ClearRenderCache();
    }
}


void SYMBOL_EDIT_FRAME::updateSelectionFilterVisbility()
{
    wxAuiPaneInfo& treePane = m_auimgr.GetPane( m_treePane );
    wxAuiPaneInfo& propertiesPane = m_auimgr.GetPane( PropertiesPaneName() );
    wxAuiPaneInfo& selectionFilterPane = m_auimgr.GetPane( wxS( "SelectionFilter" ) );

    // Don't give the selection filter its own visibility controls; instead show it if
    // anything else is visible
    bool showFilter = ( treePane.IsShown() && treePane.IsDocked() )
                      || ( propertiesPane.IsShown() && propertiesPane.IsDocked() );

    selectionFilterPane.Show( showFilter );
}


bool SYMBOL_EDIT_FRAME::GetShowInvisibleFields()
{
    // Returns the current render option for invisible fields
    return libeditconfig()->m_ShowHiddenFields;
}


bool SYMBOL_EDIT_FRAME::GetShowInvisiblePins()
{
    // Returns the current render option for invisible pins
    return libeditconfig()->m_ShowHiddenPins;
}
