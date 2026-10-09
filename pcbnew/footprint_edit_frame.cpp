/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2015 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2015-2016 Wayne Stambaugh <stambaughw@gmail.com>
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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "tool/embed_tool.h"
#include <diff_merge/fp_digest.h>
#include <libraries/library_manager.h>
#include <footprint_library_adapter.h>
#include "tools/convert_tool.h"
#include "tools/constraint_edit_tool.h"
#include "tools/drawing_tool.h"
#include "tools/edit_tool.h"
#include "tools/pcb_edit_table_tool.h"
#include "tools/footprint_editor_control.h"
#include "tools/pad_tool.h"
#include "tools/pcb_actions.h"
#include "tools/pcb_control.h"
#include "tools/pcb_picker_tool.h"
#include "tools/match_properties_tool.h"
#include "tools/graphic_edit_tool.h"
#include <geometry/geometry_utils.h>
#include "tools/align_distribute_tool.h"
#include "tools/pcb_point_editor.h"
#include "tools/pcb_selection_tool.h"
#include <bitmaps.h>
#include <api/api_handler_common.h>
#include <api/api_handler_footprint.h>
#include <api/api_handler_libraries.h>
#include <api/api_plugin_manager.h>
#include <api/api_server.h>
#include <api/api_utils.h>
#include <board.h>
#include <dialogs/dialog_library_source_setup.h>
#include <dialogs/dialog_mdw_library_checkin.h>
#include <project/net_settings.h>
#include <widgets/wx_infobar.h>
#include <footprint.h>
#include <confirm.h>
#include <drc/drc_item.h>
#include <kidialog.h>
#include <footprint_edit_frame.h>
#include <fp_tree_synchronizing_adapter.h>
#include <footprint_editor_settings.h>
#include <footprint_editor_tab_context.h>
#include <footprint_library_adapter.h>
#include <gal/graphics_abstraction_layer.h>
#include <pcb_painter.h>
#include <render_settings.h>
#include <scoped_set_reset.h>
#include <view/view.h>
#include <kiface_base.h>
#include <kiplatform/app.h>
#include <kiplatform/ui.h>
#include <kiway.h>
#include <macros.h>
#include <pcbnew_id.h>
#include <pgm_base.h>
#include <project.h>
#include <project_pcb.h>
#include <string_utils.h>
#include <settings/settings_manager.h>
#include <tool/action_toolbar.h>
#include <tool/common_control.h>
#include <tool/common_tools.h>
#include <tool/properties_tool.h>
#include <tool/selection.h>
#include <tool/library_editor_control.h>
#include <tool/tool_dispatcher.h>
#include <tool/tool_manager.h>
#include <tool/zoom_tool.h>
#include <tools/array_tool.h>
#include <tools/pcb_grid_helper.h>
#include <tools/pcb_editor_conditions.h>
#include <tools/pcb_viewer_tools.h>
#include <tools/pcb_group_tool.h>
#include <tools/position_relative_tool.h>
#include <widgets/appearance_controls.h>
#include <widgets/editor_tabs_panel.h>
#include <widgets/lib_tree.h>
#include <widgets/mdw_part_actions.h>
#include <widgets/panel_selection_filter.h>
#include <widgets/pcb_properties_panel.h>
#include <wildcards_and_files_ext.h>
#include <widgets/wx_aui_utils.h>
#include <toolbars_footprint_editor.h>
#include <wx/app.h>

#include <algorithm>
#include <set>

#include <wx/filedlg.h>
#include <wx/hyperlink.h>
#include <wx/log.h>


namespace
{
    // Long enough to cover a burst of edits, short enough that the release status follows them
    constexpr int TEAM_DIGEST_DELAY_MS = 300;
} // namespace


BEGIN_EVENT_TABLE( FOOTPRINT_EDIT_FRAME, PCB_BASE_FRAME )
    EVT_MENU( wxID_CLOSE, FOOTPRINT_EDIT_FRAME::CloseFootprintEditor )
    EVT_MENU( wxID_EXIT, FOOTPRINT_EDIT_FRAME::OnExitKiCad )

    EVT_SIZE( FOOTPRINT_EDIT_FRAME::OnSize )

    EVT_CHOICE( ID_ON_ZOOM_SELECT, FOOTPRINT_EDIT_FRAME::OnSelectZoom )
    EVT_CHOICE( ID_ON_GRID_SELECT, FOOTPRINT_EDIT_FRAME::OnSelectGrid )

    EVT_TOOL( ID_FPEDIT_SAVE_PNG, FOOTPRINT_EDIT_FRAME::OnSaveFootprintAsPng )

    // Drop files event
    EVT_DROP_FILES( FOOTPRINT_EDIT_FRAME::OnDropFiles )

END_EVENT_TABLE()


FOOTPRINT_EDIT_FRAME::FOOTPRINT_EDIT_FRAME( KIWAY* aKiway, wxWindow* aParent ) :
        PCB_BASE_EDIT_FRAME( aKiway, aParent, FRAME_FOOTPRINT_EDITOR, _( "Footprint Editor" ), wxDefaultPosition,
                             wxDefaultSize, KICAD_DEFAULT_DRAWFRAME_STYLE, GetFootprintEditorFrameName() ),
        m_show_layer_manager_tools( true ),
        m_teamPart( this, *this,
                    [this]()
                    {
                        updateInfoBar();
                    } ),
        m_tabsPanel( nullptr ),
        m_activeTab( nullptr ),
        m_suppressTabActivation( false ),
        m_protectedBorrowedBoard( nullptr ),
        m_silentFootprintTabClose( false )
{
    m_showBorderAndTitleBlock = false;   // true to show the frame references
    m_aboutTitle = _HKI( "KiCad Footprint Editor" );

    m_teamDigest.Bind( wxEVT_TIMER,
                       [this]( wxTimerEvent& )
                       {
                           if( m_isClosing )
                               return;

                           wxString teamMessage = m_teamPart.Message();
                           refreshTeamPart( false );

                           if( m_teamPart.Message() != teamMessage )
                               updateInfoBar();
                       } );

    m_staleTabsTimer.Bind( wxEVT_TIMER,
                           [this]( wxTimerEvent& )
                           {
                               if( !m_isClosing )
                                   refreshStaleFootprintTabs();
                           } );

    m_editorSettings = nullptr;

    // Give an icon
    wxIcon icon;
    wxIconBundle icon_bundle;

    icon.CopyFromBitmap( KiBitmap( BITMAPS::icon_modedit, 48 ) );
    icon_bundle.AddIcon( icon );
    icon.CopyFromBitmap( KiBitmap( BITMAPS::icon_modedit, 128 ) );
    icon_bundle.AddIcon( icon );
    icon.CopyFromBitmap( KiBitmap( BITMAPS::icon_modedit, 256 ) );
    icon_bundle.AddIcon( icon );
    icon.CopyFromBitmap( KiBitmap( BITMAPS::icon_modedit_32 ) );
    icon_bundle.AddIcon( icon );
    icon.CopyFromBitmap( KiBitmap( BITMAPS::icon_modedit_16 ) );
    icon_bundle.AddIcon( icon );

    SetIcons( icon_bundle );

    // Create GAL canvas
    m_canvasType = loadCanvasTypeSetting();

    PCB_DRAW_PANEL_GAL* drawPanel = new PCB_DRAW_PANEL_GAL( this, -1, wxPoint( 0, 0 ), m_frameSize,
                                                            GetGalDisplayOptions(), m_canvasType );
    SetCanvas( drawPanel );

    SetBoard( new BOARD() );

    // This board will only be used to hold a footprint for editing
    GetBoard()->SetBoardUse( BOARD_USE::FPHOLDER );

    // In Footprint Editor, the default net clearance is not known (it depends on the actual
    // board).  So we do not show the default clearance, by setting it to 0.  The footprint or
    // pad specific clearance will be shown.
    GetBoard()->GetDesignSettings().m_NetSettings->GetDefaultNetclass()->SetClearance( 0 );

    // Don't show the default board solder mask expansion in the footprint editor.  Only the
    // footprint or pad mask expansions settings should be shown.
    GetBoard()->GetDesignSettings().m_SolderMaskExpansion = 0;

    // Ensure all layers and items are visible:
    // In footprint editor, some layers have no meaning or cannot be used, but we show all of
    // them, at least to be able to edit a bad layer
    GetBoard()->SetVisibleAlls();

    GetGalDisplayOptions().m_axesEnabled = true;

    // In Footprint Editor, set the default paper size to A4 for plot/print
    SetPageSettings( PAGE_INFO( PAGE_SIZE_TYPE::A4 ) );
    SetScreen( new PCB_SCREEN( GetPageSettings().GetSizeIU( pcbIUScale.IU_PER_MILS ) ) );

    // Create the manager and dispatcher & route draw panel events to the dispatcher
    setupTools();
    setupUIConditions();

    initLibraryTree();
    m_treePane = new FOOTPRINT_TREE_PANE( this );

    m_toolbarSettings = GetToolbarSettings<FOOTPRINT_EDIT_TOOLBAR_SETTINGS>( "fpedit-toolbars" );
    configureToolbars();
    RecreateToolbars();
    ReCreateLayerBox( false );

    ReCreateMenuBar();

    wxTheApp->Bind( EDA_EVT_PLUGIN_AVAILABILITY_CHANGED,
                    &FOOTPRINT_EDIT_FRAME::onPluginAvailabilityChanged, this );

    m_selectionFilterPanel = new PANEL_SELECTION_FILTER( this );
    m_appearancePanel = new APPEARANCE_CONTROLS( this, GetCanvas(), true );
    m_propertiesPanel = new PCB_PROPERTIES_PANEL( this, this );

    // LoadSettings() *after* creating m_LayersManager, because LoadSettings() initialize
    // parameters in m_LayersManager
    // NOTE: KifaceSettings() will return PCBNEW_SETTINGS if we started from pcbnew
    LoadSettings( GetSettings() );

    updateEnabledLayers();

    float proportion = GetFootprintEditorSettings()->m_AuiPanels.properties_splitter;
    m_propertiesPanel->SetSplitterProportion( proportion );

    SetActiveLayer( F_SilkS );

    // Fetch a COPY of the config as a lot of these initializations are going to overwrite our data.
    FOOTPRINT_EDITOR_SETTINGS::AUI_PANELS aui_cfg;
    int                                   libWidth = 0;

    if( FOOTPRINT_EDITOR_SETTINGS* cfg = GetSettings() )
    {
        libWidth = cfg->m_LibWidth;
        aui_cfg = cfg->m_AuiPanels;
    }
    else
    {
        // keep gcc quiet about uninitalized vars:
        aui_cfg.appearance_panel_tab = 0;
        aui_cfg.right_panel_width = -1;
    }

    m_auimgr.SetManagedWindow( this );

    // Create the infobar pane only after the AUI manager owns this frame. Registering it before
    // SetManagedWindow() leaves the Top/Layer-1 pane mispositioned once the tabbed center pane
    // mounts and the perspective restores, so ShowMessage() never reveals it.
    CreateInfoBar();

    unsigned int auiFlags = wxAUI_MGR_DEFAULT;
#if !defined( _WIN32 )
    // Windows cannot redraw the UI fast enough during a live resize and may lead to all kinds
    // of graphical glitches
    auiFlags |= wxAUI_MGR_LIVE_RESIZE;
#endif
    m_auimgr.SetFlags( auiFlags );

    // Rows; layers 4 - 6
    m_auimgr.AddPane( m_tbTopMain, EDA_PANE().HToolbar().Name( "TopMainToolbar" )
                      .Top().Layer( 6 ) );

    m_auimgr.AddPane( m_messagePanel, EDA_PANE().Messages().Name( "MsgPanel" )
                      .Bottom().Layer( 6 ) );

    // Columns; layers 1 - 3
    m_auimgr.AddPane( m_treePane, EDA_PANE().Palette().Name( "Footprints" )
                      .Left().Layer( 4 )
                      .Caption( _( "Libraries" ) )
                      // Don't use -1 for don't-change-height on a growable panel; it has side-effects.
                      .MinSize( FromDIP( 250 ), FromDIP( 80 ) )
                      .BestSize( FromDIP( 250 ), -1 ) );
    m_auimgr.AddPane( m_propertiesPanel, EDA_PANE().Name( PropertiesPaneName() )
                      .Left().Layer( 3 )
                      .Caption( _( "Properties" ) ).PaneBorder( false )
                      .MinSize( FromDIP( wxSize( 240, 60 ) ) ).BestSize( FromDIP( wxSize( 300, 200 ) ) ) );
    m_auimgr.AddPane( m_tbLeft, EDA_PANE().VToolbar().Name( "LeftToolbar" )
                      .Left().Layer( 2 ) );

    m_auimgr.AddPane( m_tbRight, EDA_PANE().VToolbar().Name( "RightToolbar" )
                      .Right().Layer(2) );
    m_auimgr.AddPane( m_appearancePanel, EDA_PANE().Name( "LayersManager" )
                      .Right().Layer( 3 )
                      .Caption( _( "Appearance" ) ).PaneBorder( false )
                      // Don't use -1 for don't-change-height on a growable panel; it has side-effects.
                      .MinSize( FromDIP( 180 ), FromDIP( 80 ) )
                      .BestSize( FromDIP( 180 ), -1 ) );
    m_auimgr.AddPane( m_selectionFilterPanel, EDA_PANE().Palette().Name( "SelectionFilter" )
                      .Right().Layer( 3 ).Position( 2 )
                      .Caption( _( "Selection Filter" ) ).PaneBorder( false )
                      // Fixed-size pane; -1 for MinSize height is required
                      .MinSize( FromDIP( 180 ), -1 )
                      .BestSize( FromDIP( 180 ), -1 ) );

    // Center: the tabbed-document strip owns the tab bar above a single, never-reparented canvas.
    // The panel borrows the canvas; the frame keeps ownership through GetCanvas().
    m_tabsPanel = new EDITOR_TABS_PANEL( this, GetCanvas() );

    m_tabsPanel->onActivateTab =
            [this]( int aIdx )
            {
                // Ignore activation during a full clear, when each tab close re-selects a fallback
                // that would re-borrow a tab-owned board into m_pcb just before it is freed.
                if( m_suppressTabActivation )
                    return;

                if( aIdx >= 0 && aIdx < static_cast<int>( m_tabContexts.size() ) )
                    activateFootprintTab( m_tabContexts[aIdx].get() );
            };

    m_tabsPanel->onCloseTabRequested =
            [this]( int aIdx ) -> bool
            {
                return promptAndCloseFootprintTab( aIdx );
            };

    // onCloseTabRequested installs the successor board atomically, so the panel must not re-activate
    // the fallback after a close and re-borrow a just-installed or just-freed index.
    m_tabsPanel->SetSuppressActivateOnClose( true );

    m_tabsPanel->onQueryVisualState =
            [this]( int aIdx ) -> TAB_VISUAL_STATE
            {
                // The panel model owns preview, but the live dirty flag lives on the shared frame
                // screen folded into the context, so read modified from the context.
                const std::vector<EDITOR_TABS_MODEL::ENTRY>& entries = m_tabsPanel->Model().Entries();

                if( aIdx < 0 || aIdx >= static_cast<int>( entries.size() ) )
                    return TAB_VISUAL_STATE{};

                bool modified = entries[aIdx].modified;

                if( aIdx < (int) m_tabContexts.size() && m_tabContexts[aIdx].get() )
                    modified = m_tabContexts[aIdx]->IsModified();

                return ResolveTabVisualState( entries[aIdx].preview, modified );
            };

    m_auimgr.AddPane( m_tabsPanel, EDA_PANE().Canvas().Name( "DrawFrame" )
                      .Center() );

    RestoreAuiLayout();

    m_auimgr.GetPane( "LayersManager" ).Show( m_show_layer_manager_tools );
    m_auimgr.GetPane( "SelectionFilter" ).Show( m_show_layer_manager_tools );
    m_auimgr.GetPane( PropertiesPaneName() ).Show( GetSettings()->m_AuiPanels.show_properties );

    wxAuiPaneInfo& selectionFilterPane = m_auimgr.GetPane( wxS( "SelectionFilter" ) );

    // The selection filter doesn't need to grow in the vertical direction when docked
    selectionFilterPane.dock_proportion = 0;

    // wx 3.3.3 turns a -1 min height into 1, and a legacy perspective restores one
    selectionFilterPane.min_size.y = m_selectionFilterPanel->GetBestSize().y;

    m_acceptedExts.emplace( FILEEXT::KiCadFootprintLibPathExtension, &ACTIONS::ddAddLibrary );
    m_acceptedExts.emplace( FILEEXT::KiCadFootprintFileExtension, &PCB_ACTIONS::ddImportFootprint );
    m_acceptedExts.emplace( FILEEXT::PngFileExtension, &PCB_ACTIONS::ddPlaceReferenceImage );
    m_acceptedExts.emplace( FILEEXT::JpegFileExtension, &PCB_ACTIONS::ddPlaceReferenceImage );
    m_acceptedExts.emplace( wxS( "jpeg" ), &PCB_ACTIONS::ddPlaceReferenceImage );
    DragAcceptFiles( true );

    FinishAUIInitialization();

    // Apply saved visibility stuff at the end
    wxAuiPaneInfo& treePane = m_auimgr.GetPane( "Footprints" );
    wxAuiPaneInfo& layersManager = m_auimgr.GetPane( "LayersManager" );

    if( libWidth > 0 )
        SetAuiPaneSize( m_auimgr, treePane, libWidth, -1 );

    if( aui_cfg.right_panel_width > 0 )
        SetAuiPaneSize( m_auimgr, layersManager, aui_cfg.right_panel_width, -1 );

    m_appearancePanel->SetTabIndex( aui_cfg.appearance_panel_tab );

    if( FOOTPRINT_EDITOR_SETTINGS* cfg = dynamic_cast<FOOTPRINT_EDITOR_SETTINGS*>( GetSettings() ) )
    {
        m_appearancePanel->SetUserLayerPresets( cfg->m_LayerPresets );
        m_appearancePanel->ApplyLayerPreset( cfg->m_ActiveLayerPreset );
    }

    // restore the last footprint from the project, if any, after the library has been init'ed
    // N.B. This needs to happen after the AUI manager has been initialized so that we can
    // properly call the WX_INFOBAR without crashing on some systems.
    restoreLastFootprint();

    m_apiHandler = std::make_unique<API_HANDLER_FOOTPRINT>( this );
    Pgm().GetApiServer().RegisterHandler( m_apiHandler.get() );

    if( Kiface().IsSingle() )
    {
        m_apiHandlerCommon = std::make_unique<API_HANDLER_COMMON>();
        Pgm().GetApiServer().RegisterHandler( m_apiHandlerCommon.get() );
        m_apiHandlerFpLibs = std::make_unique<API_HANDLER_LIBRARIES>(
                LIBRARY_TABLE_TYPE::DESIGN_BLOCK );
        Pgm().GetApiServer().RegisterHandler( m_apiHandlerFpLibs.get() );
    }

    GetToolManager()->PostAction( ACTIONS::zoomFitScreen );
    setupUnits( GetSettings() );

    resolveCanvasType();

    // This displays the last footprint loaded, if any, so it must be done after restoreLastFootprint()
    ActivateGalCanvas();

    // Default shutdown reason until a file is loaded
    KIPLATFORM::APP::SetShutdownBlockReason( this, _( "Footprint changes are unsaved" ) );

    // Catch unhandled accelerator command characters that were no handled by the library tree
    // panel.
    Bind( wxEVT_CHAR, &TOOL_DISPATCHER::DispatchWxEvent, m_toolDispatcher );
    Bind( wxEVT_CHAR_HOOK, &TOOL_DISPATCHER::DispatchWxEvent, m_toolDispatcher );

    // GTK rejects WXK_TAB as a menu accelerator, so handle Ctrl+Tab cycling from the CHAR_HOOK
    // stream. Binding after the dispatcher runs this handler first so it can consume the event.
    Bind( wxEVT_CHAR_HOOK, &FOOTPRINT_EDIT_FRAME::OnTabCharHook, this );

    // Ensure the window is on top
    Raise();
    Show( true );

    // Register a call to update the toolbar sizes. It can't be done immediately because
    // it seems to require some sizes calculated that aren't yet (at least on GTK).
    CallAfter(
            [this]()
            {
                // Ensure the controls on the toolbars all are correctly sized
                UpdateToolbarControlSizes();
                m_treePane->FocusSearchFieldIfExists();

                // Update the angle snap mode toolbar button to reflect the current preference
                GetToolManager()->RunAction( PCB_ACTIONS::angleSnapModeChanged );
            } );
}


FOOTPRINT_EDIT_FRAME::~FOOTPRINT_EDIT_FRAME()
{
    m_teamDigest.Stop();
    m_staleTabsTimer.Stop();

    Pgm().GetApiServer().DeregisterHandler( m_apiHandler.get() );

    if( Kiface().IsSingle() )
    {
        Pgm().GetApiServer().DeregisterHandler( m_apiHandlerCommon.get() );
        Pgm().GetApiServer().DeregisterHandler( m_apiHandlerFpLibs.get() );
    }

    // Hand the borrowed canvas back before the base destructor deletes it, or the later-destroyed
    // panel would reparent already-freed memory. The canvas then frees exactly once.
    m_tabsPanel->ReleaseSharedCanvas();

    // Shutdown all running tools
    if( m_toolManager )
        m_toolManager->ShutdownAllTools();

    // save the footprint in the PROJECT
    retainLastFootprint();

    // Clear the watched file
    setFPWatcher( nullptr );

    // When a tab context owns the active board, null the frame pointer so the base destructor does
    // not double-free it. A frame-owned bootstrap board is left for the base class to delete.
    if( activeBoardOwnedByTab() )
        m_pcb = nullptr;

    // We passed ownership of these to wxAuiManager.
    // delete m_selectionFilterPanel;
    // delete m_appearancePanel;
    // delete m_treePane;
}


void FOOTPRINT_EDIT_FRAME::UpdateMsgPanel()
{
    EDA_DRAW_FRAME::UpdateMsgPanel();

    FOOTPRINT* fp = static_cast<FOOTPRINT*>( GetModel() );

    if( fp )
    {
        std::vector<MSG_PANEL_ITEM> msgItems;
        fp->GetMsgPanelInfo( this, msgItems );
        SetMsgPanel( msgItems );
    }
}


bool FOOTPRINT_EDIT_FRAME::IsContentModified() const
{
    return GetScreen()
                && GetScreen()->IsContentModified()
                && GetBoard() && GetBoard()->GetFirstFootprint();
}


SELECTION& FOOTPRINT_EDIT_FRAME::GetCurrentSelection()
{
    return m_toolManager->GetTool<PCB_SELECTION_TOOL>()->GetSelection();
}


void FOOTPRINT_EDIT_FRAME::SwitchCanvas( EDA_DRAW_PANEL_GAL::GAL_TYPE aCanvasType )
{
    // switches currently used canvas (Cairo / OpenGL).
    PCB_BASE_FRAME::SwitchCanvas( aCanvasType );

    GetCanvas()->GetGAL()->SetAxesEnabled( true );

    // The base class method *does not reinit* the layers manager. We must update the layer
    // widget to match board visibility states, both layers and render columns, and and some
    // settings dependent on the canvas.
    UpdateUserInterface();
}


void FOOTPRINT_EDIT_FRAME::HardRedraw()
{
    SyncLibraryTree( true );
    GetCanvas()->ForceRefresh();
}


void FOOTPRINT_EDIT_FRAME::ToggleLibraryTree()
{
    wxAuiPaneInfo& treePane = m_auimgr.GetPane( m_treePane );
    treePane.Show( !IsLibraryTreeShown() );

    if( IsLibraryTreeShown() )
    {
        // SetAuiPaneSize also updates m_auimgr
        SetAuiPaneSize( m_auimgr, treePane, m_editorSettings->m_LibWidth, -1 );
    }
    else
    {
        m_editorSettings->m_LibWidth = m_treePane->GetSize().x;
        m_auimgr.Update();
    }
}


void FOOTPRINT_EDIT_FRAME::FocusLibraryTreeInput()
{
    m_treePane->FocusSearchFieldIfExists();
}


void FOOTPRINT_EDIT_FRAME::ToggleLayersManager()
{
    FOOTPRINT_EDITOR_SETTINGS* settings = GetSettings();
    wxAuiPaneInfo&             layersManager = m_auimgr.GetPane( "LayersManager" );
    wxAuiPaneInfo&             selectionFilter = m_auimgr.GetPane( "SelectionFilter" );

    // show auxiliary Vertical layers and visibility manager toolbar
    m_show_layer_manager_tools = !m_show_layer_manager_tools;
    layersManager.Show( m_show_layer_manager_tools );
    selectionFilter.Show( m_show_layer_manager_tools );

    if( m_show_layer_manager_tools )
    {
        SetAuiPaneSize( m_auimgr, layersManager, settings->m_AuiPanels.right_panel_width, -1 );
    }
    else
    {
        settings->m_AuiPanels.right_panel_width = m_appearancePanel->GetSize().x;
        m_auimgr.Update();
    }
}


bool FOOTPRINT_EDIT_FRAME::IsLibraryTreeShown() const
{
    return const_cast<wxAuiManager&>( m_auimgr ).GetPane( m_treePane ).IsShown();
}


BOARD_ITEM_CONTAINER* FOOTPRINT_EDIT_FRAME::GetModel() const
{
    // A tab close transiently leaves the frame with no board while UI repaints can still run.
    if( !GetBoard() )
        return nullptr;

    return GetBoard()->GetFirstFootprint();
}


LIB_ID FOOTPRINT_EDIT_FRAME::GetTargetFPID() const
{
    LIB_ID id;

    if( IsLibraryTreeShown() )
        id = GetLibTree()->GetSelectedLibId();

    if( id.GetLibNickname().empty() )
        id = GetLoadedFPID();

    return id;
}


LIB_ID FOOTPRINT_EDIT_FRAME::GetLoadedFPID() const
{
    // The board pointer is transiently null during a tab close while the library tree adapter can
    // still repaint, so tolerate having no board.
    if( !GetBoard() )
        return LIB_ID();

    FOOTPRINT* footprint = GetBoard()->GetFirstFootprint();

    if( footprint )
        return LIB_ID( footprint->GetFPID().GetLibNickname(), m_footprintNameWhenLoaded );
    else
        return LIB_ID();
}


void FOOTPRINT_EDIT_FRAME::ClearModify()
{
    if( GetBoard() && GetBoard()->GetFirstFootprint() )
    {
        m_footprintNameWhenLoaded =
                GetBoard()->GetFirstFootprint()->GetFPID().GetUniStringLibItemName();
    }

    GetScreen()->SetContentModified( false );

    // OnModify() mirrors the dirty flag onto the active tab context and the panel model, so clear both
    // here too. Tab rendering and close prompting read the context's flag, so a save that only cleared
    // the shared screen would leave the tab starred and re-prompting to save.
    if( m_activeTab )
    {
        m_activeTab->SetModified( false );

        if( int idx = m_tabsPanel->FindTab( m_activeTab->GetTabKey() ); idx >= 0 )
            m_tabsPanel->MarkModified( idx, false );
    }

    // A save ends the unsaved work the infobar's outcome may describe, once its caller is done
    if( !m_isClosing )
    {
        CallAfter(
                [this]()
                {
                    RefreshPart();
                } );
    }
}


bool FOOTPRINT_EDIT_FRAME::IsCurrentFPFromBoard() const
{
    // If we've already vetted closing this window, then we have no FP anymore
    if( m_isClosing || !GetBoard() )
        return false;

    FOOTPRINT* footprint = GetBoard()->GetFirstFootprint();

    return ( footprint && footprint->GetLink() != niluuid );
}


void FOOTPRINT_EDIT_FRAME::retainLastFootprint()
{
    LIB_ID id = GetLoadedFPID();

    if( id.IsValid() )
    {
        Prj().SetRString( PROJECT::PCB_FOOTPRINT_EDITOR_LIB_NICKNAME, id.GetLibNickname() );
        Prj().SetRString( PROJECT::PCB_FOOTPRINT_EDITOR_FP_NAME, id.GetLibItemName() );
    }
}


void FOOTPRINT_EDIT_FRAME::restoreLastFootprint()
{
    // Restore the persisted tab set first. The active tab is loaded last so it ends up focused, and
    // tabs that no longer resolve are silently dropped.
    if( FOOTPRINT_EDITOR_SETTINGS* cfg = GetSettings(); cfg && !cfg->m_OpenTabs.empty() )
    {
        const wxString activeKey = cfg->m_ActiveTab;

        const auto loadTab =
                [this]( const FOOTPRINT_EDITOR_SETTINGS::OPEN_TAB& aTab ) -> bool
                {
                    LIB_ID id;
                    id.SetLibNickname( aTab.m_lib );
                    id.SetLibItemName( aTab.m_fpName );

                    if( FOOTPRINT* footprint = LoadFootprint( id ) )
                    {
                        AddFootprintToBoard( footprint );
                        findOrCreateFootprintTab( id, aTab.m_preview );

                        return true;
                    }

                    // LoadFootprint swallows the IO_ERROR when the library no longer resolves, so
                    // log the dropped tab to keep a failed restore diagnosable.
                    wxLogTrace( wxT( "KICAD_FP_TABS" ),
                                wxT( "Dropping persisted footprint tab '%s:%s' (failed to load)" ),
                                aTab.m_lib, aTab.m_fpName );

                    return false;
                };

        const FOOTPRINT_EDITOR_SETTINGS::OPEN_TAB* activeTab = nullptr;

        for( const FOOTPRINT_EDITOR_SETTINGS::OPEN_TAB& tab : cfg->m_OpenTabs )
        {
            const wxString key = tab.m_lib + wxT( ":" ) + tab.m_fpName;

            if( key == activeKey )
                activeTab = &tab;
            else
                loadTab( tab );
        }

        if( activeTab )
            loadTab( *activeTab );

        if( !m_tabContexts.empty() )
            return;
    }

    const wxString& footprintName = Prj().GetRString( PROJECT::PCB_FOOTPRINT_EDITOR_FP_NAME );
    const wxString& libNickname =  Prj().GetRString( PROJECT::PCB_FOOTPRINT_EDITOR_LIB_NICKNAME );

    if( libNickname.Length() && footprintName.Length() )
    {
        LIB_ID id;
        id.SetLibNickname( libNickname );
        id.SetLibItemName( footprintName );

        FOOTPRINT* footprint = LoadFootprint( id );

        if( footprint )
        {
            AddFootprintToBoard( footprint );

            // The last-session footprint is a real document, so promote its tab to permanent.
            findOrCreateFootprintTab( id, false );
        }
    }
}


void FOOTPRINT_EDIT_FRAME::updateEnabledLayers()
{
    FOOTPRINT* footprint = static_cast<FOOTPRINT*>( GetModel() );
    BOARD& board = *GetBoard();

    // All FPs have these layers enabled
    LSET enabledLayers = LSET::AllTechMask() | LSET::UserMask();

    const auto configureStackup =
            [&]( FOOTPRINT_STACKUP aMode, const LSET& aLayerSet )
            {
                const LSET cuLayers = aLayerSet & LSET::AllCuMask();
                board.SetCopperLayerCount( cuLayers.count() );

                switch( aMode )
                {
                case FOOTPRINT_STACKUP::EXPAND_INNER_LAYERS:
                    enabledLayers |= LSET{ F_Cu, In1_Cu, B_Cu };
                    board.SetLayerName( In1_Cu, _( "Inner layers" ) );
                    break;

                case FOOTPRINT_STACKUP::CUSTOM_LAYERS:
                    // Nothing extra to add

                    // Clear layer name defaults
                    board.SetLayerName( In1_Cu, wxEmptyString );
                    break;
                }

                enabledLayers |= aLayerSet;
            };

    if( footprint )
    {
        configureStackup( footprint->GetStackupMode(), footprint->GetStackupLayers() );
    }
    else
    {
        // If no footprint is loaded, we assume the default stackup mode
        configureStackup( FOOTPRINT_STACKUP::EXPAND_INNER_LAYERS, LSET{} );
    }

    if( m_originalFootprintCopy )
    {
        m_originalFootprintCopy->RunOnChildren(
                [&]( BOARD_ITEM* child )
                {
                    LSET childLayers = child->GetLayerSet() & LSET::UserDefinedLayersMask();

                    for( PCB_LAYER_ID layer : childLayers )
                        enabledLayers.set( layer );
                },
                RECURSE_MODE::RECURSE );
    }

    // Enable the user-configured number of user layers, plus any specifically named layers
    if( FOOTPRINT_EDITOR_SETTINGS* cfg = GetAppSettings<FOOTPRINT_EDITOR_SETTINGS>( "fpedit" ) )
    {
        int userLayerCount = cfg->m_DesignSettings.GetUserDefinedLayerCount();
        enabledLayers |= LSET::UserDefinedLayersMask( userLayerCount );

        for( const PCB_LAYER_ID& user : LSET::UserDefinedLayersMask() )
        {
            if( cfg->m_DesignSettings.m_UserLayerNames.contains( LSET::Name( user ).ToStdString() ) )
                enabledLayers.set( user );
        }
    }

    board.SetEnabledLayers( enabledLayers );

    // Footprint Editor layer visibility is kept in the view, not the board (because the board
    // just delegates to the project file, which we don't have).
    for( const PCB_LAYER_ID& layer : GetBoard()->GetEnabledLayers() )
        GetCanvas()->GetView()->SetLayerVisible( layer, true );
}


void FOOTPRINT_EDIT_FRAME::updateInfoBar()
{
    // Use CallAfter so that we update the canvas before waiting for the infobar animation
    CallAfter(
            [this]()
            {
                WX_INFOBAR& infobar = *GetInfoBar();
                infobar.RemoveAllButtons();

                wxArrayString msgs;
                int           infobarFlags = wxICON_INFORMATION;
                FOOTPRINT*    footprint = GetBoard() ? GetBoard()->GetFirstFootprint() : nullptr;
                wxString      lib = footprint ? footprint->GetFPID().GetLibNickname() : UTF8();

                if( IsCurrentFPFromBoard() )
                {
                    msgs.push_back( wxString::Format( _( "Editing %s from board.  Saving will update the board only." ),
                                                      footprint->GetReference() ) );

                    infobar.AddLink( wxString::Format( _( "Open footprint from library %s" ), UnescapeString( lib ) ),
                            [this]( wxHyperlinkEvent& aEvent )
                            {
                                GetToolManager()->RunAction( PCB_ACTIONS::editLibFpInFpEditor );
                            } );
                }

                if( footprint && !IsCurrentFPFromBoard() )
                {
                    MdwAddPartLinks( infobar, msgs, *this,
                                     [this, id = footprint->GetFPID()]( const MDW_PART_ACTION& aAction )
                                     {
                                         FOOTPRINT* open = GetBoard() ? GetBoard()->GetFirstFootprint() : nullptr;

                                         // A link clicked after its part was closed or replaced acts on nothing
                                         if( open && !IsCurrentFPFromBoard() && open->GetFPID() == id )
                                             MdwRunPartAction( *this, aAction.id, { id }, aAction.target );
                                     } );
                }

                if( footprint
                        && !IsCurrentFPFromBoard()
                        && !PROJECT_PCB::FootprintLibAdapter( &Prj() )->IsFootprintLibWritable( lib ) )
                {
                    msgs.push_back( _( "Library is read-only.  Changes cannot be saved to this library." ) );

                    infobar.AddLink( _( "Save as editable copy" ),
                            [this]( wxHyperlinkEvent& aEvent )
                            {
                                SaveFootprintAs( GetBoard()->GetFirstFootprint() );
                                GetCanvas()->GetView()->Update( GetBoard()->GetFirstFootprint() );
                                ClearModify();

                                // Get rid of the save-will-update-board-only (or any other dismissable warning)
                                WX_INFOBAR* local_infobar = GetInfoBar();

                                if( local_infobar->IsShownOnScreen() && local_infobar->HasCloseButton() )
                                    local_infobar->Dismiss();

                                GetCanvas()->ForceRefresh();
                                SyncLibraryTree( true );
                            } );
                }

                if( msgs.empty() )
                {
                    infobar.Dismiss();
                }
                else
                {
                    wxString msg = wxJoin( msgs, '\n', '\0' );
                    infobar.AddCloseButton();
                    infobar.ShowMessage( msg, infobarFlags );
                }
            } );
}


void FOOTPRINT_EDIT_FRAME::setLoadBaseline( const FOOTPRINT& aFootprint )
{
    m_originalFootprintCopy.reset( static_cast<FOOTPRINT*>( aFootprint.Clone() ) );
    m_originalFootprintCopy->SetParent( nullptr );

    m_footprintNameWhenLoaded = aFootprint.GetFPID().GetUniStringLibItemName();

    // Mirror the load baseline into the active tab context so it survives a tab switch (the frame
    // members are only a borrowed view of the active context's baseline).
    if( m_activeTab )
    {
        m_activeTab->SetOriginalFootprintCopy(
                std::unique_ptr<FOOTPRINT>( static_cast<FOOTPRINT*>( m_originalFootprintCopy->Clone() ) ) );
        m_activeTab->SetFootprintNameWhenLoaded( m_footprintNameWhenLoaded );
        m_activeTab->SetLibNickname( aFootprint.GetFPID().GetLibNickname() );
        m_activeTab->SetFootprintName( aFootprint.GetFPID().GetLibItemName() );
    }
}


void FOOTPRINT_EDIT_FRAME::DiscardLoadBaseline()
{
    m_originalFootprintCopy.reset();

    if( m_activeTab )
        m_activeTab->SetOriginalFootprintCopy( nullptr );
}


void FOOTPRINT_EDIT_FRAME::ReloadFootprint( FOOTPRINT* aFootprint )
{
    // Cancel a mid-draw tool before the footprint it points into is freed (#24975).
    if( GetToolManager() )
        GetToolManager()->ResetTools( TOOL_BASE::MODEL_RELOAD );

    // The history holds pointers into the footprints deleted here, and undoing would bring back stale content
    freeUndoRedoCommandsWithItems( m_undoList, m_redoList );
    GetBoard()->DeleteAllFootprints();

    setLoadBaseline( *aFootprint );

    PCB_BASE_EDIT_FRAME::AddFootprintToBoard( aFootprint );
    // Ensure item UUIDs are valid
    // ("old" footprints can have null uuids that create issues in fp editor)
    aFootprint->FixUuids();

    refreshTeamPart( true );

    updateEnabledLayers();
    updateInfoBar();
    UpdateMsgPanel();
    UpdateUserInterface();
}


bool FOOTPRINT_EDIT_FRAME::IsContentReadOnly() const
{
    FOOTPRINT* footprint = GetBoard() ? GetBoard()->GetFirstFootprint() : nullptr;

    // The state follows the footprint it was read for, never one opened since
    return footprint && !IsCurrentFPFromBoard() && m_teamPart.IsReadOnly()
           && m_teamPart.Key() == footprint->GetFPID().Format().wx_str();
}


void FOOTPRINT_EDIT_FRAME::ShowReadOnlyRefusal()
{
    m_teamPart.RefuseEdit( this,
                           [this]()
                           {
                               if( !GetBoard() || !GetBoard()->GetFirstFootprint() )
                                   return false;

                               // The message panel still describes the value that was just refused
                               UpdateMsgPanel();
                               return true;
                           } );
}


std::optional<MDW_PART_TARGET> FOOTPRINT_EDIT_FRAME::resolveTeamPart( PROJECT& aProject, const LIB_ID& aId )
{
    FOOTPRINT_LIBRARY_ADAPTER*        adapter = PROJECT_PCB::FootprintLibAdapter( &aProject );
    std::optional<LIBRARY_TABLE_ROW*> row = adapter->GetRow( aId.GetLibNickname() );

    if( !row )
        return std::nullopt;

    const wxString name = aId.GetUniStringLibItemName();
    const wxString file =
            MdwTeamPartFile( MDW_LIBRARY_TYPE::FOOTPRINT, LIBRARY_MANAGER::GetFullURI( *row, true ), name, nullptr );

    return MdwPartTarget( MDW_LIBRARY_TYPE::FOOTPRINT, **row, name, file );
}


bool FOOTPRINT_EDIT_FRAME::RefuseReleasedChange( const LIB_ID& aId )
{
    FOOTPRINT*                    open = GetBoard() ? GetBoard()->GetFirstFootprint() : nullptr;
    std::optional<MDW_PART_STATE> released;

    // The open part's own state is newer than any listing. Save As renames the open footprint before asking,
    // so the controller must still describe aId
    if( open && !IsCurrentFPFromBoard() && m_teamPart.IsTeamPart() && open->GetFPID() == aId
        && m_teamPart.Key() == aId.Format().wx_str() )
    {
        released = m_teamPart.ReadOnlyState();
    }
    else
    {
        released = MDW_PART_CONTROLLER::ReleasedState( resolveTeamPart( Prj(), aId ) );
    }

    return MDW_PART_CONTROLLER::RefuseReleased( this, released, aId.GetUniStringLibItemName() );
}


bool FOOTPRINT_EDIT_FRAME::IsReleasedTeamPart( PROJECT& aProject, const LIB_ID& aId )
{
    return MDW_PART_CONTROLLER::IsReleased( resolveTeamPart( aProject, aId ) );
}


void FOOTPRINT_EDIT_FRAME::listenForTeamListings()
{
    m_teamPart.ListenForListings(
            [this]()
            {
                refreshTeamPart( true, true );
                updateInfoBar();
            },
            [this]( const MDW_MEMBER_CHANGES& aChanges )
            {
                if( m_adapter )
                    m_adapter->RefreshLifecycle( aChanges );

                if( m_treePane )
                    GetLibTree()->RefreshFiltered();
            } );
}


void FOOTPRINT_EDIT_FRAME::refreshTeamPart( bool aReload, bool aKeepUnresolved, bool aDigest )
{
    FOOTPRINT* footprint = GetBoard() ? GetBoard()->GetFirstFootprint() : nullptr;
    wxString   key = footprint && !IsCurrentFPFromBoard() ? footprint->GetFPID().Format().wx_str() : wxString();

    listenForTeamListings();

    if( aReload || key != m_teamPart.Key() )
    {
        std::optional<MDW_PART_TARGET> target;

        if( footprint && !IsCurrentFPFromBoard() )
            target = resolveTeamPart( Prj(), footprint->GetFPID() );

        // A source still coming up after a reconnect resolves nothing yet, and the part stays protected meanwhile
        if( target || !aKeepUnresolved || key != m_teamPart.Key() )
        {
            wxString name = footprint ? UnescapeString( footprint->GetFPID().GetUniStringLibItemName() ) : wxString();
            m_teamPart.SetPart( std::move( target ), name, key );
        }
    }

    if( aDigest && m_teamPart.IsTeamPart() && footprint )
    {
        m_teamDigest.Stop();
        m_teamPart.SetDigest( KICAD_DIFF::FootprintDigest( *footprint ),
                              KICAD_DIFF::FootprintDigest( *footprint, KICAD_DIFF::FOOTPRINT_DIGEST::MEMBER ) );
    }
}


std::optional<std::string> FOOTPRINT_EDIT_FRAME::CurrentFormat( const std::string&              aDestination,
                                                                const std::vector<std::string>& aFolder,
                                                                const std::string& aPartFile, const wxString& aName )
{
    // A footprint file is the whole part and names its own version, so it reads the same in any later format
    return aPartFile;
}


void FOOTPRINT_EDIT_FRAME::PopTool( const TOOL_EVENT& aEvent )
{
    PCB_BASE_EDIT_FRAME::PopTool( aEvent );

    if( ToolStackIsEmpty() )
        m_teamPart.RecheckOutcome();
}


void FOOTPRINT_EDIT_FRAME::GetLatest()
{
    if( FOOTPRINT* footprint = GetBoard() ? GetBoard()->GetFirstFootprint() : nullptr )
    {
        MdwUpdatePartLibrary( *this, footprint->GetFPID().GetLibNickname() );
    }
}


bool FOOTPRINT_EDIT_FRAME::CanReloadPart( bool aDiscardEdits )
{
    FOOTPRINT* footprint = GetBoard() ? GetBoard()->GetFirstFootprint() : nullptr;

    // Replacing the footprint under a dialog or an interactive tool would leave them dangling
    return footprint && !IsCurrentFPFromBoard() && ( aDiscardEdits || !IsContentModified() ) && IsEnabled()
           && !Kiway().HasBlockingDialog() && ToolStackIsEmpty();
}


bool FOOTPRINT_EDIT_FRAME::ReloadPart( bool aDiscardEdits )
{
    if( !CanReloadPart( aDiscardEdits ) )
        return false;

    FOOTPRINT*                 footprint = GetBoard()->GetFirstFootprint();
    FOOTPRINT_LIBRARY_ADAPTER* adapter = PROJECT_PCB::FootprintLibAdapter( &Prj() );
    const wxString             nickname = footprint->GetFPID().GetLibNickname();

    // A same-second rewrite keeps the cached library's timestamp, so its plugin is made afresh
    // Reload only the row in effect, since a shadowed global library shares its preload cache
    if( std::optional<LIBRARY_TABLE_ROW*> row = adapter->GetRow( nickname ) )
        adapter->ReloadLibraryEntry( nickname, ( *row )->Scope() );

    std::optional<LIB_STATUS> status = adapter->LoadOne( nickname );

    if( !status || status->load_status != LOAD_STATUS::LOADED )
        return false;

    std::unique_ptr<FOOTPRINT> reloaded( LoadFootprint( footprint->GetFPID() ) );

    if( !reloaded )
        return false;

    // The tab keeps its own unsaved edits over the file, so edits the user gave up must stop counting first
    if( aDiscardEdits )
        ClearModify();

    return RefreshLibraryFootprintTab( *reloaded );
}


std::optional<MDW_PART_TARGET> FOOTPRINT_EDIT_FRAME::ResolvePart( const LIB_ID& aId )
{
    return resolveTeamPart( Prj(), aId );
}


bool FOOTPRINT_EDIT_FRAME::OpenPart( const LIB_ID& aId )
{
    return MdwOpenPartOnce(
            [&]()
            {
                FOOTPRINT* footprint = GetBoard() ? GetBoard()->GetFirstFootprint() : nullptr;
                return footprint && !IsCurrentFPFromBoard() && footprint->GetFPID() == aId;
            },
            [&]()
            {
                LoadFootprintFromLibrary( aId );
            } );
}


void FOOTPRINT_EDIT_FRAME::RefreshPart()
{
    refreshTeamPart( false );
    updateInfoBar();
}


bool FOOTPRINT_EDIT_FRAME::DescribeForRelease( MDW_RELEASE_ITEM& aItem, const std::vector<LIB_ID>& aReleasing )
{
    aItem.modified = IsFootprintModified( aItem.id );
    return true;
}


void FOOTPRINT_EDIT_FRAME::CheckInLibrary( const wxString& aNickname )
{
    // Board instances are never offered, since saving them writes the board rather than the library
    MdwCheckInPartLibrary( *this, aNickname, HasModifiedFootprintTabs( true, aNickname ),
                           [&]()
                           {
                               if( !SaveModifiedLibraryTabs( aNickname ) )
                                   return false;

                               GetCanvas()->ForceRefresh();
                               RefreshLibraryTree();
                               return true;
                           } );
}


bool FOOTPRINT_EDIT_FRAME::IsFootprintModified( const LIB_ID& aId ) const
{
    FOOTPRINT* open = GetBoard() ? GetBoard()->GetFirstFootprint() : nullptr;

    if( open && !IsCurrentFPFromBoard() && open->GetFPID() == aId && IsContentModified() )
        return true;

    return hasModifiedTab( aId );
}


bool FOOTPRINT_EDIT_FRAME::hasModifiedTab( const LIB_ID& aId ) const
{
    if( !m_tabsPanel )
        return false;

    const wxString key = aId.GetLibNickname().wx_str() + wxT( ':' ) + aId.GetLibItemName().wx_str();
    const int      idx = m_tabsPanel->FindTab( key );

    if( idx < 0 || idx >= static_cast<int>( m_tabContexts.size() ) )
        return false;

    return m_tabContexts[idx].get() != m_activeTab && m_tabContexts[idx]->IsModified();
}


bool FOOTPRINT_EDIT_FRAME::SaveForRelease( std::vector<MDW_RELEASE_ITEM>& aItems )
{
    std::vector<LIB_ID> released;

    for( const MDW_RELEASE_ITEM& item : aItems )
        released.push_back( item.id );

    // The source releases what is on disk, so the tabs the prompt names are saved first and no others
    if( !SaveModifiedLibraryTabs( wxEmptyString, released ) )
        return false;

    GetCanvas()->ForceRefresh();
    RefreshLibraryTree();

    for( MDW_RELEASE_ITEM& item : aItems )
    {
        // A listing still being read again falls back to the one shown, which the backend refuses if it is stale
        if( std::optional<MDW_PART_TARGET> target = resolveTeamPart( Prj(), item.id ) )
            item.target = std::move( *target );

        MDW_OVERWRITE_ANSWER answer = SettleOverwrittenPart( this, Kiway(), &Prj(), item.target );

        if( answer == MDW_OVERWRITE_ANSWER::CANCELLED )
        {
            item.held = _( "Not released" );
            continue;
        }

        // Settling brings the copy up to date, which may have reloaded the board
        FOOTPRINT* open = GetBoard() ? GetBoard()->GetFirstFootprint() : nullptr;
        const bool isOpen = open && !IsCurrentFPFromBoard() && open->GetFPID() == item.id;

        if( answer == MDW_OVERWRITE_ANSWER::REWRITTEN )
        {
            if( isOpen )
                m_teamPart.Settle();

            item.held = _( "Rewritten with the version you chose; review it and release again" );
            continue;
        }

        std::unique_ptr<FOOTPRINT> saved( isOpen ? nullptr : LoadFootprint( item.id ) );
        const FOOTPRINT*           footprint = isOpen ? open : saved.get();

        if( !footprint )
        {
            item.held = _( "No longer in its library" );
            continue;
        }

        item.digest = KICAD_DIFF::FootprintDigest( *footprint );
        item.memberDigest = KICAD_DIFF::FootprintDigest( *footprint, KICAD_DIFF::FOOTPRINT_DIGEST::MEMBER );
    }

    updateInfoBar();
    return true;
}


bool FOOTPRINT_EDIT_FRAME::prepareFootprintTabHandoff( FOOTPRINT* aFootprint, bool aHasTabs )
{
    const bool fromBoard = aFootprint && aFootprint->GetLink() != niluuid;

    if( !aHasTabs || !aFootprint || fromBoard || !aFootprint->GetFPID().IsValid() )
        return false;

    // A new footprint is parented to the board the tab switch is about to free, so drop the parent
    // before the switch; ReloadFootprint reparents it to the incoming board
    aFootprint->SetParent( nullptr );

    return true;
}


void FOOTPRINT_EDIT_FRAME::installFootprintOnActiveBoard( FOOTPRINT* aFootprint )
{
    ReloadFootprint( aFootprint );

    if( IsCurrentFPFromBoard() )
        setFPWatcher( nullptr );
    else
        setFPWatcher( aFootprint );
}


void FOOTPRINT_EDIT_FRAME::AddFootprintToBoard( FOOTPRINT* aFootprint )
{
    // Opening from the library is a preview that the next library-open reuses; editing promotes it
    // to a permanent tab.
    if( prepareFootprintTabHandoff( aFootprint, m_tabsPanel != nullptr ) )
        findOrCreateFootprintTab( aFootprint->GetFPID(), true );

    installFootprintOnActiveBoard( aFootprint );
}


bool FOOTPRINT_EDIT_FRAME::activeBoardOwnedByTab() const
{
    if( m_protectedBorrowedBoard && m_protectedBorrowedBoard == m_pcb )
        return true;

    return std::any_of( m_tabContexts.begin(), m_tabContexts.end(),
                        [this]( const std::unique_ptr<FOOTPRINT_EDITOR_TAB_CONTEXT>& aCtx )
                        {
                            return aCtx->GetBoard() == m_pcb;
                        } );
}


void FOOTPRINT_EDIT_FRAME::borrowBoardNonDestructive( BOARD* aBoard )
{
    // Swap the board pointer without the SetBoard delete, since on a tab switch the outgoing board
    // is owned by its tab context. Reproduce only the side effects SetBoard applies to the incoming
    // board.
    if( m_pcb == aBoard )
        return;

    // The only frame-owned board is the constructor's bootstrap board. Delete it when we first
    // switch away so it does not leak; tab-owned boards are left to their contexts. The view must
    // release the bootstrap board's items before their owner is deleted.
    if( m_pcb && !activeBoardOwnedByTab() )
    {
        if( GetCanvas() )
            GetCanvas()->GetView()->Clear();

        delete m_pcb;
    }

    m_pcb = aBoard;

    if( GetBoard() )
        GetBoard()->SetUserUnits( GetUserUnits() );

    if( GetBoard() && GetCanvas() )
    {
        if( KIGFX::RENDER_SETTINGS* rs = GetCanvas()->GetView()->GetPainter()->GetSettings() )
        {
            rs->SetDashLengthRatio( GetBoard()->GetPlotOptions().GetDashedLineDashRatio() );
            rs->SetGapLengthRatio( GetBoard()->GetPlotOptions().GetDashedLineGapRatio() );
        }

        GetCanvas()->GetView()->GetGAL()->SetGridOrigin( GetBoard()->GetDesignSettings().GetGridOrigin() );
    }

    wxCommandEvent e( EDA_EVT_BOARD_CHANGED );
    ProcessEventLocally( e );
}


void FOOTPRINT_EDIT_FRAME::detachActiveFootprintTab()
{
    if( !m_activeTab )
        return;

    KIGFX::VIEW* view = GetCanvas()->GetView();

    EDITOR_TAB_CONTEXT::VIEW_SNAPSHOT& snap = m_activeTab->ViewSnapshot();
    snap.scale = view->GetScale();
    snap.center = view->GetCenter();
    snap.valid = true;

    std::vector<KIID>& sel = m_activeTab->SavedSelection();
    sel.clear();

    for( EDA_ITEM* item : GetCurrentSelection() )
        sel.push_back( item->m_Uuid );

    // Dirty state lives on the shared frame screen, so fold it into the context to survive the switch.
    if( GetScreen() )
        m_activeTab->SetModified( GetScreen()->IsContentModified() );

    // Swap the raw vectors since UNDO_REDO_CONTAINER is non-copyable; ownership transfers intact.
    m_activeTab->UndoList().m_CommandsList.swap( m_undoList.m_CommandsList );
    m_activeTab->RedoList().m_CommandsList.swap( m_redoList.m_CommandsList );

    m_activeTab = nullptr;
}


void FOOTPRINT_EDIT_FRAME::activateFootprintTab( FOOTPRINT_EDITOR_TAB_CONTEXT* aCtx )
{
    if( !aCtx || aCtx == m_activeTab )
        return;

    installFootprintTabBoard( aCtx, aCtx->GetBoard() );
}


void FOOTPRINT_EDIT_FRAME::recombineEditedPad()
{
    PAD_TOOL* padTool = m_toolManager ? m_toolManager->GetTool<PAD_TOOL>() : nullptr;

    if( padTool && padTool->InPadEditMode() )
        m_toolManager->RunAction( PCB_ACTIONS::recombinePad );
}


void FOOTPRINT_EDIT_FRAME::installFootprintTabBoard( FOOTPRINT_EDITOR_TAB_CONTEXT* aCtx, BOARD* aBoard )
{
    // Install the successor board and tool environment before the caller frees the outgoing context
    // so no tool Reset() or repaint ever sees a freed or null m_pcb. Never calls the base SetBoard,
    // which would delete m_pcb and reset tools while the model still aliases it.

    recombineEditedPad();
    detachActiveFootprintTab();

    // Drop selection and unwind a mid-draw tool while the outgoing board is still valid (#24975).
    if( m_toolManager )
    {
        m_toolManager->RunAction( ACTIONS::selectionClear );
        m_toolManager->DeactivateTool();
    }

    m_activeTab = aCtx;

    borrowBoardNonDestructive( aBoard );

    if( aCtx )
    {
        m_undoList.m_CommandsList.swap( aCtx->UndoList().m_CommandsList );
        m_redoList.m_CommandsList.swap( aCtx->RedoList().m_CommandsList );
    }
    else
    {
        // Discard the swapped-in lists with their transient items so nothing references a freed board.
        freeUndoRedoCommandsWithItems( m_undoList, m_redoList );
    }

    m_originalFootprintCopy.reset( aCtx && aCtx->GetOriginalFootprintCopy()
                                               ? static_cast<FOOTPRINT*>( aCtx->GetOriginalFootprintCopy()->Clone() )
                                               : nullptr );
    m_footprintNameWhenLoaded = aCtx ? aCtx->GetFootprintNameWhenLoaded() : wxString();

    // Mirror the incoming tab's board-uuid remap onto the frame so SaveFootprintToBoard saves an
    // instance edit back to the originating board items. A library tab has an empty map.
    if( aCtx )
        m_boardFootprintUuids = aCtx->BoardFootprintUuids();
    else
        m_boardFootprintUuids.clear();

    if( GetScreen() )
        GetScreen()->SetContentModified( aCtx ? aCtx->IsModified() : false );

    // Point the tool environment at the incoming board before ResetTools so tools never dereference
    // a freed model.
    m_toolManager->SetEnvironment( GetBoard(), GetCanvas()->GetView(), GetCanvas()->GetViewControls(), config(),
                                   this );

    GetCanvas()->DisplayBoard( GetBoard() );
    GetCanvas()->UpdateColors();

    m_toolManager->ResetTools( TOOL_BASE::MODEL_RELOAD );

    FOOTPRINT* activeFootprint = GetBoard()->GetFirstFootprint();

    if( IsCurrentFPFromBoard() )
        setFPWatcher( nullptr );
    else
        setFPWatcher( activeFootprint );

    // The release lock follows the tab, before any tool can edit what it now shows
    refreshTeamPart( false );

    if( !aCtx )
    {
        updateEnabledLayers();
        updateInfoBar();
        UpdateMsgPanel();
        UpdateUserInterface();
        GetCanvas()->ForceRefresh();
        return;
    }

    const EDITOR_TAB_CONTEXT::VIEW_SNAPSHOT& snap = aCtx->ViewSnapshot();

    if( snap.valid )
    {
        GetCanvas()->GetView()->SetScale( snap.scale );
        GetCanvas()->GetView()->SetCenter( snap.center );
    }
    else
    {
        // First time this tab is shown there is no saved view
        m_toolManager->RunAction( ACTIONS::zoomFitScreen );
    }

    if( !aCtx->SavedSelection().empty() )
    {
        PCB_SELECTION_TOOL* selTool = m_toolManager->GetTool<PCB_SELECTION_TOOL>();

        if( selTool )
        {
            for( const KIID& id : aCtx->SavedSelection() )
            {
                if( BOARD_ITEM* item = GetBoard()->ResolveItem( id, true ) )
                    selTool->select( item );
            }
        }
    }

    if( m_appearancePanel )
        m_appearancePanel->OnBoardChanged();

    if( m_propertiesPanel )
        m_propertiesPanel->UpdateData();

    updateEnabledLayers();
    updateInfoBar();
    UpdateMsgPanel();
    UpdateUserInterface();
    GetCanvas()->ForceRefresh();
}


static std::unique_ptr<BOARD> makeFpHolderBoard( FOOTPRINT_EDITOR_SETTINGS* aCfg )
{
    auto board = std::make_unique<BOARD>();
    board->SetBoardUse( BOARD_USE::FPHOLDER );
    board->GetDesignSettings().m_NetSettings->GetDefaultNetclass()->SetClearance( 0 );
    board->GetDesignSettings().m_SolderMaskExpansion = 0;
    board->SetVisibleAlls();

    // Get our own settings; aCfg will be the PCBNEW_SETTINGS because we're part of the pcbnew
    // compile unit
    if( aCfg )
    {
        board->GetDesignSettings() = aCfg->m_DesignSettings;

        for( auto& [source_name, dest_name] : aCfg->m_DesignSettings.m_UserLayerNames )
        {
            wxString wx_source_name = source_name;
            PCB_LAYER_ID layer = static_cast<PCB_LAYER_ID>( LSET::NameToLayer( wx_source_name ) );

            if( IsUserLayer( layer ) )
                board->SetLayerName( layer, dest_name );
        }
    }

    return board;
}


FOOTPRINT_EDITOR_TAB_CONTEXT* FOOTPRINT_EDIT_FRAME::findOrCreateFootprintTab( const LIB_ID& aLibId, bool aAsPreview )
{
    const wxString lib = aLibId.GetLibNickname();
    const wxString name = aLibId.GetLibItemName();
    const wxString key = lib + wxT( ":" ) + name;

    if( int existing = m_tabsPanel->FindTab( key ); existing >= 0 )
    {
        m_tabsPanel->ActivateOrCreateTab( key, name, aAsPreview );
        return m_tabContexts[existing].get();
    }

    auto ctx = std::make_unique<FOOTPRINT_EDITOR_TAB_CONTEXT>( lib, name, makeFpHolderBoard( GetSettings() ) );
    ctx->SetPreview( aAsPreview );

    FOOTPRINT_EDITOR_TAB_CONTEXT* raw = ctx.get();

    // Keep m_tabContexts index-aligned with the panel model. A previewed open reuses the preview
    // slot in place; otherwise the context is appended. It must be at its final index before AddTab
    // fires onActivateTab.
    const int reuseSlot = aAsPreview ? m_tabsPanel->Model().PreviewIndex() : -1;

    if( reuseSlot >= 0 && reuseSlot < static_cast<int>( m_tabContexts.size() ) )
    {
        // Tear the active document down while the outgoing board is still alive, before it is
        // displaced and freed.
        if( m_tabContexts[reuseSlot].get() == m_activeTab )
        {
            if( m_toolManager )
                m_toolManager->RunAction( ACTIONS::selectionClear );

            freeUndoRedoCommandsWithItems( m_undoList, m_redoList );
            m_activeTab = nullptr;
        }

        return placeReusedTabContext( m_tabContexts, reuseSlot, std::move( ctx ),
                [&]( const FOOTPRINT_EDITOR_TAB_CONTEXT& aDisplaced )
                {
                    SCOPED_SET_RESET<BOARD*> protectedBoard( m_protectedBorrowedBoard, aDisplaced.GetBoard() );
                    m_tabsPanel->ActivateOrCreateTab( key, name, aAsPreview );
                } );
    }

    m_tabContexts.push_back( std::move( ctx ) );

    m_tabsPanel->ActivateOrCreateTab( key, name, aAsPreview );

    return raw;
}


FOOTPRINT_EDITOR_TAB_CONTEXT* FOOTPRINT_EDIT_FRAME::placeReusedTabContext(
        std::vector<std::unique_ptr<FOOTPRINT_EDITOR_TAB_CONTEXT>>& aContexts, int aSlot,
        std::unique_ptr<FOOTPRINT_EDITOR_TAB_CONTEXT> aNew,
        const std::function<void( const FOOTPRINT_EDITOR_TAB_CONTEXT& )>& aInstallSuccessor )
{
    // Hold the displaced context, and its still-displayed board, alive across aInstallSuccessor, which
    // repoints the canvas VIEW at the incoming board. Freeing the old board first would leave
    // VIEW::Clear() dereferencing items from a destroyed board.
    std::unique_ptr<FOOTPRINT_EDITOR_TAB_CONTEXT> displaced = std::move( aContexts[aSlot] );
    aContexts[aSlot] = std::move( aNew );

    FOOTPRINT_EDITOR_TAB_CONTEXT* raw = aContexts[aSlot].get();

    aInstallSuccessor( *displaced );

    return raw;
}


FOOTPRINT_EDITOR_TAB_CONTEXT* FOOTPRINT_EDIT_FRAME::findOrCreateFootprintInstanceTab( FOOTPRINT* aBoardFootprint )
{
    if( !aBoardFootprint )
        return nullptr;

    const KIID      sourceUuid = aBoardFootprint->m_Uuid;
    const wxString  key = FOOTPRINT_EDITOR_TAB_CONTEXT::MakeInstanceTabKey( sourceUuid );
    const wxString& reference = aBoardFootprint->GetReference();

    // Re-editing the same placed footprint focuses the live tab rather than duplicating it.
    if( int existing = m_tabsPanel->FindTab( key ); existing >= 0 )
    {
        m_tabsPanel->ActivateOrCreateTab( key, m_tabContexts[existing]->GetDisplayName(), false );
        return m_tabContexts[existing].get();
    }

    std::unique_ptr<BOARD> board = makeFpHolderBoard( GetSettings() );
    board->GetDesignSettings().m_DRCSeverities[DRCE_MISSING_COURTYARD] = RPT_SEVERITY_WARNING;

    auto ctx = std::make_unique<FOOTPRINT_EDITOR_TAB_CONTEXT>( sourceUuid, reference, std::move( board ) );
    BOARD*                        ctxBoard = ctx->GetBoard();
    std::map<KIID, KIID>&         uuidMap = ctx->BoardFootprintUuids();
    FOOTPRINT_EDITOR_TAB_CONTEXT* raw = ctx.get();

    const auto recordAndUpdateUuid =
            [&]( BOARD_ITEM* aItem )
            {
                KIID newId;
                uuidMap[newId] = aItem->m_Uuid;
                aItem->SetUuid( newId );
            };

    // Clone keeps the existing uuids, which the remap then swaps for fresh editor-local ones so
    // SaveFootprintToBoard can map them back to the original board items.
    FOOTPRINT* clone = static_cast<FOOTPRINT*>( aBoardFootprint->Clone() );
    clone->SetParent( ctxBoard );
    clone->SetParentGroup( nullptr );
    clone->SetLink( aBoardFootprint->m_Uuid );
    clone->ClearFlags();
    clone->SetLocked( false );

    recordAndUpdateUuid( clone );
    clone->RunOnChildren(
            [&]( BOARD_ITEM* aItem )
            {
                if( aItem->Type() == PCB_PAD_T )
                    aItem->SetLocked( false );

                aItem->ClearFlags();
                recordAndUpdateUuid( aItem );
            },
            RECURSE_MODE::RECURSE );

    ctxBoard->Add( clone );

    // The editor knows nothing of the board's nets, so force the orphaned dummy net on every pad to
    // avoid saving a reference to an unknown net into the library cache.
    clone->ClearAllNets();

    // Normalize placement to the footprint-editor defaults so the editor view matches a library load.
    clone->SetPosition( VECTOR2I( 0, 0 ) );

    if( clone->GetLayer() != F_Cu )
        clone->Flip( clone->GetPosition(), GetPcbNewSettings()->m_FlipDirection );

    clone->SetOrientation( ANGLE_0 );
    clone->RunOnChildren(
            [&]( BOARD_ITEM* aItem )
            {
                if( aItem->Type() == PCB_TEXT_T )
                    static_cast<PCB_TEXT*>( aItem )->KeepUpright();
            },
            RECURSE_MODE::RECURSE );

    m_tabContexts.push_back( std::move( ctx ) );

    // Index-aligned with the panel model; the context is at its final index before AddTab fires
    // onActivateTab.
    m_tabsPanel->ActivateOrCreateTab( key, m_tabContexts.back()->GetDisplayName(), false );

    return raw;
}


FOOTPRINT_EDITOR_TAB_CONTEXT* FOOTPRINT_EDIT_FRAME::CreateUnsavedFootprintTab()
{
    std::unique_ptr<BOARD>                        board = makeFpHolderBoard( GetSettings() );
    std::unique_ptr<FOOTPRINT_EDITOR_TAB_CONTEXT> ctx = FOOTPRINT_EDITOR_TAB_CONTEXT::MakeUnsaved( std::move( board ) );

    const wxString                key = ctx->GetTabKey();
    FOOTPRINT_EDITOR_TAB_CONTEXT* raw = ctx.get();

    m_tabContexts.push_back( std::move( ctx ) );

    // At its final index before AddTab fires onActivateTab, which makes the new tab active
    m_tabsPanel->ActivateOrCreateTab( key, m_tabContexts.back()->GetDisplayName(), false );

    return raw;
}


void FOOTPRINT_EDIT_FRAME::freeUndoRedoCommandsWithItems( UNDO_REDO_CONTAINER& aUndo, UNDO_REDO_CONTAINER& aRedo )
{
    // Free the UR_TRANSIENT board items each command owns and the command wrappers. The frame's own
    // ClearUndoRedoList() and the bare container destructor delete only the wrappers and leak the
    // transient items, so a discarded tab history must come through here.
    for( UNDO_REDO_CONTAINER* list : { &aUndo, &aRedo } )
    {
        for( PICKED_ITEMS_LIST* cmd : list->m_CommandsList )
        {
            ClearListAndDeleteItems( cmd );
            delete cmd;
        }

        list->m_CommandsList.clear();
    }
}


void FOOTPRINT_EDIT_FRAME::freeFootprintTabUndoRedo( FOOTPRINT_EDITOR_TAB_CONTEXT& aCtx )
{
    // The context is detached here, so the transient items in its lists are never the live objects.
    freeUndoRedoCommandsWithItems( aCtx.UndoList(), aCtx.RedoList() );
}


bool FOOTPRINT_EDIT_FRAME::promptAndCloseFootprintTab( int aIdx )
{
    if( aIdx < 0 || aIdx >= static_cast<int>( m_tabContexts.size() ) )
        return false;

    FOOTPRINT_EDITOR_TAB_CONTEXT* ctx = m_tabContexts[aIdx].get();

    // The active tab's live dirty state lives on the shared frame screen, so ctx->IsModified() is
    // stale for it. Fold the screen state back in so an unsaved active tab still prompts.
    if( ctx == m_activeTab && GetScreen() )
        ctx->SetModified( GetScreen()->IsContentModified() );

    if( ctx->IsModified() && !m_silentFootprintTabClose )
    {
        // Prompt while the closing tab is still fully live so a save reads its real board.
        wxString msg = wxString::Format( _( "Save changes to '%s' before closing?" ), ctx->GetDisplayName( true ) );

        KIDIALOG dlg( this, msg, _( "Confirmation" ), wxYES_NO | wxCANCEL | wxICON_WARNING );
        dlg.SetYesNoCancelLabels( _( "Save" ), _( "Discard Changes" ), _( "Cancel" ) );

        switch( dlg.ShowModal() )
        {
        case wxID_YES:
        {
            // SaveFootprint reads the active tab's load baseline for rename detection, so an inactive
            // tab must be made active first or it saves against the wrong baseline. Restore the prior
            // active tab afterwards so closing an inactive tab stays an inactive close, keeping the
            // panel's suppressed fallback selection aligned with the live board. Veto the close if the
            // save did not complete so the edits and history are not silently dropped.
            FOOTPRINT_EDITOR_TAB_CONTEXT* activeBeforeSave = m_activeTab;
            const bool                    activatedForSave = ( ctx != m_activeTab );

            if( activatedForSave )
                activateFootprintTab( ctx );

            const bool saved = SaveFootprint( ctx->GetBoard()->GetFirstFootprint() );

            if( activatedForSave && activeBeforeSave )
                activateFootprintTab( activeBeforeSave );

            if( !saved )
                return false;

            break;
        }

        case wxID_NO:
            break;

        default:
        case wxID_CANCEL:
            return false;
        }
    }

    const bool closingActive = ( ctx == m_activeTab );

    if( !closingActive )
    {
        // m_pcb aliases a different live tab's board, so freeing this context dangles nothing.
        freeFootprintTabUndoRedo( *ctx );
        m_tabContexts.erase( m_tabContexts.begin() + aIdx );
        return true;
    }

    // The closing tab's lists are currently the frame's live lists. Free them now with their
    // transient items, so the install's detach swaps empty lists into the closing context.
    freeUndoRedoCommandsWithItems( m_undoList, m_redoList );

    const int newCount = static_cast<int>( m_tabContexts.size() ) - 1;
    const int successorIdx = newCount > 0 ? std::min( aIdx, newCount - 1 ) : -1;

    if( successorIdx >= 0 )
    {
        // Map the successor's post-erase position back to a pre-erase index. Indices above the closed
        // one shift down by one after the erase.
        const int preEraseSuccessor = successorIdx < aIdx ? successorIdx : successorIdx + 1;

        FOOTPRINT_EDITOR_TAB_CONTEXT* successor = m_tabContexts[preEraseSuccessor].get();

        installFootprintTabBoard( successor, successor->GetBoard() );
    }
    else
    {
        // Last tab: the fresh empty board is frame-owned like the bootstrap board.
        installFootprintTabBoard( nullptr, makeFpHolderBoard( GetSettings() ).release() );
    }

    // The install swapped m_pcb off the closing board, so erasing the context frees it safely.
    m_tabContexts.erase( m_tabContexts.begin() + aIdx );

    return true;
}


void FOOTPRINT_EDIT_FRAME::CloseFootprintTab( const LIB_ID& aFPID )
{
    const wxString lib = aFPID.GetLibNickname();
    const wxString name = aFPID.GetLibItemName();
    const int      idx = m_tabsPanel->FindTab( lib + wxT( ":" ) + name );

    if( idx < 0 )
        return;

    // The caller already confirmed the deletion, so close without re-prompting. The panel selects a
    // successor when the closed tab was active.
    m_silentFootprintTabClose = true;
    m_tabsPanel->CloseTab( idx );
    m_silentFootprintTabClose = false;
}


void FOOTPRINT_EDIT_FRAME::RenameFootprintTab( const LIB_ID& aOldId, const LIB_ID& aNewId )
{
    const wxString newLib = aNewId.GetLibNickname();
    const wxString newName = aNewId.GetLibItemName();

    if( aOldId.GetLibNickname().empty() )
    {
        // This test precludes a tree-driven rename, so we can assume that the active tab is being
        // renamed.

        // An unsaved import is keyed by a session id, not by aOldId, so the save-as that named it
        // promotes the active tab.
        if( m_activeTab && m_activeTab->IsUnsaved() && !newLib.IsEmpty() )
        {
            const wxString unsavedKey = m_activeTab->GetTabKey();

            m_activeTab->PromoteToLibrary( newLib, newName );
            m_tabsPanel->RenameTab( unsavedKey, m_activeTab->GetTabKey(), m_activeTab->GetDisplayName() );

            return;
        }

        // A board footprint's tab is named from the board instance's reference, not any properties of
        // the edited copy.
        if( m_activeTab && m_activeTab->IsFromBoard() && aOldId.GetLibNickname().empty() )
            return;
    }

    const wxString oldLib = aOldId.GetLibNickname();
    const wxString oldName = aOldId.GetLibItemName();
    const wxString oldKey = oldLib + wxT( ":" ) + oldName;
    const int      idx = m_tabsPanel->FindTab( oldKey );

    if( idx < 0 )
        return;

    if( idx < static_cast<int>( m_tabContexts.size() ) )
    {
        // A tab switch restores the context's baseline onto the frame, so it must track the rename too
        // or the next save would delete the pre-renamed library entry
        m_tabContexts[idx]->SetLibNickname( newLib );
        m_tabContexts[idx]->SetFootprintName( newName );
        m_tabContexts[idx]->SetFootprintNameWhenLoaded( newName );
    }

    m_tabsPanel->RenameTab( oldKey, newLib + wxT( ":" ) + newName, newName );
}


static bool outsideLibrary( const FOOTPRINT_EDITOR_TAB_CONTEXT& aCtx, const wxString& aNickname )
{
    return !aNickname.IsEmpty() && ( aCtx.IsTransient() || aCtx.GetLibNickname() != aNickname );
}


bool FOOTPRINT_EDIT_FRAME::HasModifiedFootprintTabs( bool aLibraryOnly, const wxString& aNickname ) const
{
    for( const std::unique_ptr<FOOTPRINT_EDITOR_TAB_CONTEXT>& ctx : m_tabContexts )
    {
        if( ( aLibraryOnly && ctx->IsFromBoard() ) || outsideLibrary( *ctx, aNickname ) )
            continue;

        if( ctx.get() == m_activeTab ? IsContentModified() : ctx->IsModified() )
            return true;
    }

    return false;
}


bool FOOTPRINT_EDIT_FRAME::RefreshLibraryFootprintTab( const FOOTPRINT& aFootprint )
{
    if( !m_tabsPanel )
        return false;

    const LIB_ID&  footprintId = aFootprint.GetFPID();
    const wxString key = footprintId.GetLibNickname().wx_str() + wxT( ':' ) + footprintId.GetLibItemName().wx_str();
    const int      idx = m_tabsPanel->FindTab( key );

    if( idx < 0 || idx >= static_cast<int>( m_tabContexts.size() ) )
        return false;

    FOOTPRINT_EDITOR_TAB_CONTEXT* ctx = m_tabContexts[idx].get();
    FOOTPRINT_EDITOR_TAB_CONTEXT* originalActive = m_activeTab;

    // If we have our own unsaved changes, then leave it be.
    if( ctx->IsModified() )
        return false;

    // We have to activate this footprint to work on it.  We'll switch back to the user-selected
    // tab afterwards.
    activateFootprintTab( ctx );
    ctx->SavedSelection().clear();

    FOOTPRINT* replacement = static_cast<FOOTPRINT*>( aFootprint.Clone() );
    replacement->SetParent( nullptr );
    ReloadFootprint( replacement );
    replacement->ClearFlags();
    ClearModify();

    // Switch back to the user-selected tab.
    if( originalActive && originalActive != ctx )
        activateFootprintTab( originalActive );

    UpdateView();
    GetCanvas()->ForceRefresh();
    Update3DView( true, true );

    return true;
}


bool FOOTPRINT_EDIT_FRAME::footprintChangedOnDisk( const FOOTPRINT& aLoaded, const FOOTPRINT* aOnDisk )
{
    using KICAD_DIFF::FOOTPRINT_DIGEST;

    return !aOnDisk
           || KICAD_DIFF::FootprintDigest( aLoaded, FOOTPRINT_DIGEST::MEMBER )
                      != KICAD_DIFF::FootprintDigest( *aOnDisk, FOOTPRINT_DIGEST::MEMBER );
}


void FOOTPRINT_EDIT_FRAME::refreshStaleFootprintTabs()
{
    FOOTPRINT_LIBRARY_ADAPTER* adapter = PROJECT_PCB::FootprintLibAdapter( &Prj() );

    if( !adapter )
        return;

    std::optional<float> progress = adapter->AsyncLoadProgress();

    // A tool or dialog holds the open footprint, and a library still loading cannot be read from yet
    if( !IsEnabled() || Kiway().HasBlockingDialog() || !ToolStackIsEmpty() || ( progress && *progress < 1 ) )
    {
        m_staleTabsTimer.StartOnce( 1000 );
        return;
    }

    std::vector<std::unique_ptr<FOOTPRINT>> replacements;
    bool                                    activeReplaced = false;

    for( const std::unique_ptr<FOOTPRINT_EDITOR_TAB_CONTEXT>& ctx : m_tabContexts )
    {
        const FOOTPRINT* loaded = ctx->GetOriginalFootprintCopy();
        bool             modified = ctx.get() == m_activeTab ? IsContentModified() : ctx->IsModified();

        if( ctx->IsTransient() || modified || !loaded )
            continue;

        try
        {
            std::unique_ptr<FOOTPRINT> onDisk =
                    adapter->FootprintOnDisk( ctx->GetLibNickname(), ctx->GetFootprintNameWhenLoaded() );

            // A footprint the pull deleted stays open, and saving it asks first
            if( onDisk && footprintChangedOnDisk( *loaded, onDisk.get() ) )
            {
                activeReplaced |= ctx.get() == m_activeTab;
                replacements.push_back( std::move( onDisk ) );
            }
        }
        catch( const IO_ERROR& )
        {
            // An unreadable library is left as it is, and saving into it reports why
        }
    }

    for( const std::unique_ptr<FOOTPRINT>& replacement : replacements )
        RefreshLibraryFootprintTab( *replacement );

    // The watcher would otherwise reload the same change again
    if( activeReplaced )
        setFPWatcher( GetBoard()->GetFirstFootprint() );
}


bool FOOTPRINT_EDIT_FRAME::SaveModifiedLibraryTabs( const wxString& aNickname, const std::vector<LIB_ID>& aOnly )
{
    FOOTPRINT_EDITOR_TAB_CONTEXT* original = m_activeTab;
    bool                          saved = true;
    std::set<wxString>            only;

    for( const LIB_ID& id : aOnly )
        only.insert( id.GetLibNickname().wx_str() + wxT( ':' ) + id.GetLibItemName().wx_str() );

    for( const std::unique_ptr<FOOTPRINT_EDITOR_TAB_CONTEXT>& ctx : m_tabContexts )
    {
        if( ctx->IsFromBoard() || outsideLibrary( *ctx, aNickname )
            || ( !only.empty() && !only.contains( ctx->GetTabKey() ) )
            || !( ctx.get() == m_activeTab ? IsContentModified() : ctx->IsModified() ) )
        {
            continue;
        }

        if( ctx.get() != m_activeTab )
            activateFootprintTab( ctx.get() );

        FOOTPRINT* footprint = GetBoard() ? GetBoard()->GetFirstFootprint() : nullptr;

        if( !footprint || !SaveFootprint( footprint ) )
        {
            saved = false;
            break;
        }

        ClearModify();
    }

    if( original && m_activeTab != original )
        activateFootprintTab( original );

    return saved;
}


bool FOOTPRINT_EDIT_FRAME::hasDirtyInactiveTabs() const
{
    for( const std::unique_ptr<FOOTPRINT_EDITOR_TAB_CONTEXT>& ctx : m_tabContexts )
    {
        if( ctx.get() != m_activeTab && ctx->IsModified() )
            return true;
    }

    return false;
}


bool FOOTPRINT_EDIT_FRAME::HandleUnsavedChanges( bool aFromBoardOnly )
{
    // Active tab always goes first
    for( int ii = 0; ii < (int) m_tabContexts.size(); ++ii )
    {
        if( aFromBoardOnly && !m_tabContexts[ii]->IsFromBoard() )
            continue;

        if( m_tabContexts[ii].get() == m_activeTab )
        {
            wxSafeYield( this, true );      // Allow frame to come to front before showing "Save Changes?"

            if( !m_tabsPanel->CloseTab( ii ) )
                return false;
        }
    }

    // Go from back so we don't have to worry about deletions
    for( int ii = (int) m_tabContexts.size() - 1; ii >= 0; --ii )
    {
        if( aFromBoardOnly && !m_tabContexts[ii]->IsFromBoard() )
            continue;

        activateFootprintTab( m_tabContexts[ii].get() );
        wxSafeYield( this, true );      // Allow tab to come to front before showing "Save Changes?"

        if( !m_tabsPanel->CloseTab( ii ) )
            return false;
    }

    return true;
}


void FOOTPRINT_EDIT_FRAME::refreshFootprintTabState()
{
    m_tabsPanel->RefreshTabLabels();
}


void FOOTPRINT_EDIT_FRAME::detachTabsForFullClear( BOARD* aReplacement )
{
    wxCHECK( aReplacement, /* void */ );

    // Install the fresh frame-owned board and reset the tools against it before any tab board is
    // freed, so no ResetTools or repaint sees a freed or null m_pcb.
    installFootprintTabBoard( nullptr, aReplacement );

    // Tear the strip down without re-prompting to save, since unsaved changes were already handled.
    // Suppress the host close callback and tab activation so CloseAll only does panel bookkeeping.
    std::function<bool( int )> savedCb = std::move( m_tabsPanel->onCloseTabRequested );
    m_tabsPanel->onCloseTabRequested =
            []( int )
            {
                return true;
            };

    m_suppressTabActivation = true;
    m_tabsPanel->CloseAll();
    m_suppressTabActivation = false;

    m_tabsPanel->onCloseTabRequested = std::move( savedCb );

    wxASSERT_MSG( m_pcb == aReplacement,
                  wxT( "m_pcb must alias the frame-owned replacement before contexts are freed" ) );

    for( const std::unique_ptr<FOOTPRINT_EDITOR_TAB_CONTEXT>& ctx : m_tabContexts )
        freeFootprintTabUndoRedo( *ctx );

    m_tabContexts.clear();
}


void FOOTPRINT_EDIT_FRAME::AdvanceFootprintTab( bool aForward )
{
    m_tabsPanel->AdvanceTab( aForward );
}


void FOOTPRINT_EDIT_FRAME::CloseActiveFootprintTab()
{
    m_tabsPanel->CloseTab( m_tabsPanel->GetActiveTab() );
}


const wxChar* FOOTPRINT_EDIT_FRAME::GetFootprintEditorFrameName()
{
    return FOOTPRINT_EDIT_FRAME_NAME;
}


BOARD_DESIGN_SETTINGS& FOOTPRINT_EDIT_FRAME::GetDesignSettings() const
{
    return GetBoard()->GetDesignSettings();
}


const PCB_PLOT_PARAMS& FOOTPRINT_EDIT_FRAME::GetPlotSettings() const
{
    wxFAIL_MSG( wxT( "Plotting not supported in Footprint Editor" ) );

    return PCB_BASE_FRAME::GetPlotSettings();
}


void FOOTPRINT_EDIT_FRAME::SetPlotSettings( const PCB_PLOT_PARAMS& aSettings )
{
    wxFAIL_MSG( wxT( "Plotting not supported in Footprint Editor" ) );
}


FOOTPRINT_EDITOR_SETTINGS* FOOTPRINT_EDIT_FRAME::GetSettings()
{
    if( !m_editorSettings )
        m_editorSettings = GetAppSettings<FOOTPRINT_EDITOR_SETTINGS>( "fpedit" );

    return m_editorSettings;
}


APP_SETTINGS_BASE* FOOTPRINT_EDIT_FRAME::config() const
{
    if( m_editorSettings )
        return m_editorSettings;

    return Pgm().GetSettingsManager().GetAppSettings<FOOTPRINT_EDITOR_SETTINGS>( "fpedit" );
}


void FOOTPRINT_EDIT_FRAME::LoadSettings( APP_SETTINGS_BASE* aCfg )
{
    // Get our own settings; aCfg will be the PCBNEW_SETTINGS because we're part of the pcbnew
    // compile unit
    FOOTPRINT_EDITOR_SETTINGS* cfg = GetSettings();

    if( cfg )
    {
        PCB_BASE_FRAME::LoadSettings( cfg );

        GetDesignSettings() = cfg->m_DesignSettings;

        m_displayOptions = cfg->m_Display;
        m_show_layer_manager_tools = cfg->m_AuiPanels.show_layer_manager;

        GetToolManager()->GetTool<PCB_SELECTION_TOOL>()->GetFilter() = cfg->m_SelectionFilter;
        m_selectionFilterPanel->SetCheckboxesFromFilter( cfg->m_SelectionFilter );

        GetLibTree()->SetSortMode( (LIB_TREE_MODEL_ADAPTER::SORT_MODE) cfg->m_LibrarySortMode );

        for( auto& [source_name, dest_name] : cfg->m_DesignSettings.m_UserLayerNames )
        {
            wxString wx_source_name = source_name;
            PCB_LAYER_ID layer = static_cast<PCB_LAYER_ID>( LSET::NameToLayer( wx_source_name ) );

            if( IsUserLayer( layer ) )
                GetBoard()->SetLayerName( layer, dest_name );
        }

    }
}


void FOOTPRINT_EDIT_FRAME::SaveSettings( APP_SETTINGS_BASE* aCfg )
{
    GetGalDisplayOptions().m_axesEnabled = true;

    // Get our own settings; aCfg will be the PCBNEW_SETTINGS because we're part of the pcbnew
    // compile unit
    FOOTPRINT_EDITOR_SETTINGS* cfg = GetSettings();

    if( cfg )
    {
        PCB_BASE_FRAME::SaveSettings( cfg );

        cfg->m_DesignSettings  = GetDesignSettings();
        cfg->m_Display         = m_displayOptions;
        cfg->m_LibWidth        = m_treePane->GetSize().x;

        if( TOOL_MANAGER* toolMgr = GetToolManager() )
        {
            if( PCB_SELECTION_TOOL* selTool = toolMgr->GetTool<PCB_SELECTION_TOOL>() )
                cfg->m_SelectionFilter = selTool->GetFilter();
        }

        cfg->m_AuiPanels.show_layer_manager = m_show_layer_manager_tools;

        if( m_propertiesPanel )
        {
            cfg->m_AuiPanels.show_properties        = m_propertiesPanel->IsShownOnScreen();
            cfg->m_AuiPanels.properties_panel_width = m_propertiesPanel->GetSize().x;
            cfg->m_AuiPanels.properties_splitter    = m_propertiesPanel->SplitterProportion();
        }

        cfg->m_LibrarySortMode = GetLibTree()->GetSortMode();

        if( m_appearancePanel )
        {
            cfg->m_AuiPanels.right_panel_width    = m_appearancePanel->GetSize().x;
            cfg->m_AuiPanels.appearance_panel_tab = m_appearancePanel->GetTabIndex();
            cfg->m_LayerPresets                   = m_appearancePanel->GetUserLayerPresets();
            cfg->m_ActiveLayerPreset              = m_appearancePanel->GetActiveLayerPreset();
        }

        cfg->m_OpenTabs.clear();
        cfg->m_ActiveTab.Clear();

        const std::vector<EDITOR_TABS_MODEL::ENTRY>& entries = m_tabsPanel->Model().Entries();

        for( size_t i = 0; i < m_tabContexts.size(); ++i )
        {
            const FOOTPRINT_EDITOR_TAB_CONTEXT* ctx = m_tabContexts[i].get();

            // Instance and unsaved-import tabs are session-only and never persisted
            if( ctx->IsTransient() )
                continue;

            const bool preview = i < entries.size() && entries[i].preview;

            cfg->m_OpenTabs.push_back( { ctx->GetLibNickname(), ctx->GetFootprintName(), preview } );

            if( ctx == m_activeTab )
                cfg->m_ActiveTab = ctx->GetTabKey();
        }
    }
}



EDA_ANGLE FOOTPRINT_EDIT_FRAME::GetRotationAngle() const
{
    FOOTPRINT_EDITOR_SETTINGS* cfg = const_cast<FOOTPRINT_EDIT_FRAME*>( this )->GetSettings();

    return cfg ? cfg->m_RotationAngle : ANGLE_90;
}



COLOR_SETTINGS* FOOTPRINT_EDIT_FRAME::GetColorSettings( bool aForceRefresh ) const
{
    FOOTPRINT_EDITOR_SETTINGS* cfg = GetFootprintEditorSettings();
    return ::GetColorSettings( cfg ? cfg->m_ColorTheme : DEFAULT_THEME );
}


MAGNETIC_SETTINGS* FOOTPRINT_EDIT_FRAME::GetMagneticItemsSettings()
{
    static MAGNETIC_SETTINGS fallback;

    if( FOOTPRINT_EDITOR_SETTINGS* cfg = GetSettings() )
        return &cfg->m_MagneticItems;

    return &fallback;
}


const BOX2I FOOTPRINT_EDIT_FRAME::GetDocumentExtents( bool aIncludeAllVisible ) const
{
    FOOTPRINT* footprint = GetBoard()->GetFirstFootprint();

    if( footprint )
    {
        bool hasGraphicalItem = footprint->Pads().size() || footprint->Zones().size();

        if( !hasGraphicalItem )
        {
            for( const BOARD_ITEM* item : footprint->GraphicalItems() )
            {
                if( item->Type() == PCB_TEXT_T || item->Type() == PCB_TEXTBOX_T )
                    continue;

                hasGraphicalItem = true;
                break;
            }
        }

        if( hasGraphicalItem )
        {
            return footprint->GetBoundingBox( false );
        }
        else
        {
            BOX2I newFootprintBB( { 0, 0 }, { 0, 0 } );
            newFootprintBB.Inflate( pcbIUScale.mmToIU( 12 ) );
            return newFootprintBB;
        }
    }

    return GetBoardBoundingBox( false );
}


bool FOOTPRINT_EDIT_FRAME::canCloseWindow( wxCloseEvent& aEvent )
{
    // Shutdown blocks must be determined and vetoed as early as possible, before any modal prompt.
    // IsContentModified only sees the active tab, so also account for dirty inactive tabs.
    if( isBlockableSessionEnd() && ( IsContentModified() || hasDirtyInactiveTabs() ) )
    {
        aEvent.Veto();
        return false;
    }

    if( !HandleUnsavedChanges( false ) )
    {
        aEvent.Veto();
        return false;
    }

    PAD_TOOL* padTool = m_toolManager->GetTool<PAD_TOOL>();

    if( padTool->InPadEditMode() )
        padTool->ExitPadEditMode();

    // Save footprint tree column widths
    m_adapter->SaveSettings();

    return PCB_BASE_EDIT_FRAME::canCloseWindow( aEvent );
}


void FOOTPRINT_EDIT_FRAME::doCloseWindow()
{
    // No more vetos
    GetCanvas()->SetEventDispatcher( nullptr );
    GetCanvas()->StopDrawing();

    Pgm().GetApiServer().DeregisterHandler( m_apiHandler.get() );
    wxTheApp->Unbind( EDA_EVT_PLUGIN_AVAILABILITY_CHANGED,
                      &FOOTPRINT_EDIT_FRAME::onPluginAvailabilityChanged, this );

    if( GetLibTree() )
        GetLibTree()->ShutdownPreviews();

    // Do not show the layer manager during closing to avoid flicker
    // on some platforms (Windows) that generate useless redraw of items in
    // the Layer Manager
    m_auimgr.GetPane( wxT( "LayersManager" ) ).Show( false );
    m_auimgr.GetPane( wxT( "SelectionFilter" ) ).Show( false );

    Clear_Pcb();
}


void FOOTPRINT_EDIT_FRAME::OnExitKiCad( wxCommandEvent& event )
{
    Kiway().OnKiCadExit();
}


void FOOTPRINT_EDIT_FRAME::CloseFootprintEditor( wxCommandEvent& Event )
{
    Close();
}


void FOOTPRINT_EDIT_FRAME::ShowChangedLanguage()
{
    // call my base class
    PCB_BASE_EDIT_FRAME::ShowChangedLanguage();

    // We have 2 panes to update.
    // For some obscure reason, the AUI manager hides the first modified pane.
    // So force show panes
    wxAuiPaneInfo& tree_pane_info = m_auimgr.GetPane( m_treePane );
    bool tree_shown = tree_pane_info.IsShown();
    tree_pane_info.Caption( _( "Libraries" ) );

    wxAuiPaneInfo& lm_pane_info = m_auimgr.GetPane( m_appearancePanel );
    bool lm_shown = lm_pane_info.IsShown();
    lm_pane_info.Caption( _( "Appearance" ) );
    wxAuiPaneInfo& sf_pane_info = m_auimgr.GetPane( m_selectionFilterPanel );
    sf_pane_info.Caption( _( "Selection Filter" ) );

    // update the layer manager
    UpdateUserInterface();

    // Now restore the visibility:
    lm_pane_info.Show( lm_shown );
    tree_pane_info.Show( tree_shown );
    m_auimgr.Update();

    GetLibTree()->ShowChangedLanguage();

    UpdateTitle();
}


void FOOTPRINT_EDIT_FRAME::OnModify()
{
    PCB_BASE_FRAME::OnModify();

    if( m_isClosing )
        return;

    wxString teamMessage = m_teamPart.Message();
    refreshTeamPart( false, false, false );

    if( m_teamPart.Message() != teamMessage )
        updateInfoBar();

    // A digest reads the whole footprint and any fonts it embeds, so a burst of edits is digested once
    if( m_teamPart.IsTeamPart() )
        m_teamDigest.StartOnce( TEAM_DIGEST_DELAY_MS );

    // An edit promotes the active tab from preview to permanent and flags it dirty. Reflect the
    // shared screen's dirty state onto the context and panel model so the tab shows bold + "*" and a
    // later library-open opens its own tab instead of replacing this one.
    if( m_activeTab )
    {
        m_activeTab->SetPreview( false );
        m_activeTab->SetModified( true );

        if( int idx = m_tabsPanel->FindTab( m_activeTab->GetTabKey() ); idx >= 0 )
            m_tabsPanel->MarkModified( idx, true );
    }

    Update3DView( true, true );
    GetLibTree()->RefreshLibTree();
}


void FOOTPRINT_EDIT_FRAME::UpdateTitle()
{
    SetTitle( _( "Footprint Editor" ) );
}


void FOOTPRINT_EDIT_FRAME::UpdateUserInterface()
{
    m_appearancePanel->OnBoardChanged();
}


void FOOTPRINT_EDIT_FRAME::UpdateView()
{
    GetCanvas()->UpdateColors();
    GetCanvas()->DisplayBoard( GetBoard() );
    m_toolManager->ResetTools( TOOL_BASE::MODEL_RELOAD );
    m_propertiesPanel->UpdateData();
}


void FOOTPRINT_EDIT_FRAME::initLibraryTree()
{
    FOOTPRINT_LIBRARY_ADAPTER* footprints = PROJECT_PCB::FootprintLibAdapter( &Prj() );

    m_adapter = FP_TREE_SYNCHRONIZING_ADAPTER::Create( this, footprints );
    auto adapter = static_cast<FP_TREE_SYNCHRONIZING_ADAPTER*>( m_adapter.get() );

    adapter->AddLibraries( this );

    // The tree's first lifecycle states come from a listing that lands later, or from the index already
    listenForTeamListings();
    m_adapter->RefreshLifecycle();
}


void FOOTPRINT_EDIT_FRAME::SyncLibraryTree( [[maybe_unused]] bool aProgress )
{
    wxLogTrace( wxT( "KICAD_TABS_DBG" ), wxT( "FOOTPRINT_EDIT_FRAME::SyncLibraryTree enter" ) );

    FOOTPRINT_LIBRARY_ADAPTER*     footprints = PROJECT_PCB::FootprintLibAdapter( &Prj() );
    FP_TREE_SYNCHRONIZING_ADAPTER* adapter = static_cast<FP_TREE_SYNCHRONIZING_ADAPTER*>( m_adapter.get() );
    LIB_ID                         target = GetTargetFPID();
    bool                           targetSelected = !target.GetLibNickname().empty()
                                                    && target == GetLibTree()->GetSelectedLibId();
    std::vector<LIB_ID>            expanded = GetLibTree()->GetExpandedLibraries();

    auto loadSettled =
            [&]( const LIB_ID& aId )
            {
                std::optional<LIB_STATUS> status = footprints->GetLibraryStatus( aId.GetLibNickname() );

                return status
                       && ( status->load_status == LOAD_STATUS::LOADED
                            || status->load_status == LOAD_STATUS::LOAD_ERROR );
            };

    // A different canvas footprint means the user moved on from the pending target
    if( GetLoadedFPID() != m_pendingTreeCanvas )
        m_pendingTreeTarget = LIB_ID();

    // Until the library loaded the tree selected a fallback row, not a choice
    if( m_pendingTreeTarget.IsValid() && loadSettled( m_pendingTreeTarget ) )
    {
        target = m_pendingTreeTarget;
        targetSelected = true;
        m_pendingTreeTarget = LIB_ID();
    }

    // Unselect before syncing to avoid null reference in the adapter
    // if a selected item is removed during the sync
    GetLibTree()->Unselect();

    // Sync the LIB_TREE to the FOOTPRINT_INFO list
    adapter->Sync( footprints );

    wxLogTrace( wxT( "KICAD_TABS_DBG" ), wxT( "FOOTPRINT_EDIT_FRAME::SyncLibraryTree Regenerate" ) );
    GetLibTree()->Regenerate( true );

    // Sync() collapsed the tree, so re-expand the libraries that were open before it.
    for( const LIB_ID& libId : expanded )
        GetLibTree()->ExpandLibId( libId );

    // A filter selects its best match, which tree actions would take for the user's choice
    if( !targetSelected )
        GetLibTree()->Unselect();

    if( target.IsValid() )
    {
        if( adapter->FindItem( target ) )
        {
            if( targetSelected )
                GetLibTree()->SelectLibId( target );
            else
                GetLibTree()->CenterLibId( target );
        }
        else
        {
            if( !loadSettled( target ) )
            {
                m_pendingTreeTarget = target;
                m_pendingTreeCanvas = GetLoadedFPID();
            }

            // Try to focus on parent
            target.SetLibItemName( wxEmptyString );
            GetLibTree()->CenterLibId( target );
        }
    }

    wxLogTrace( wxT( "KICAD_TABS_DBG" ), wxT( "FOOTPRINT_EDIT_FRAME::SyncLibraryTree exit" ) );
}


void FOOTPRINT_EDIT_FRAME::RefreshLibraryTree()
{
    GetLibTree()->RefreshLibTree();
}


void FOOTPRINT_EDIT_FRAME::FocusOnLibID( const LIB_ID& aLibID )
{
    GetLibTree()->SelectLibId( aLibID );
}


void FOOTPRINT_EDIT_FRAME::OnDisplayOptionsChanged()
{
    m_appearancePanel->UpdateDisplayOptions();
}


void FOOTPRINT_EDIT_FRAME::setupTools()
{
    // Create the manager and dispatcher & route draw panel events to the dispatcher
    m_toolManager = new TOOL_MANAGER;
    m_toolManager->SetEnvironment( GetBoard(), GetCanvas()->GetView(), GetCanvas()->GetViewControls(), config(), this );
    m_actions = new PCB_ACTIONS();
    m_toolDispatcher = new TOOL_DISPATCHER( m_toolManager );

    GetCanvas()->SetEventDispatcher( m_toolDispatcher );

    m_toolManager->RegisterTool( new COMMON_CONTROL );
    m_toolManager->RegisterTool( new COMMON_TOOLS );
    m_toolManager->RegisterTool( new PCB_SELECTION_TOOL );
    m_toolManager->RegisterTool( new ZOOM_TOOL );
    m_toolManager->RegisterTool( new EDIT_TOOL );
    m_toolManager->RegisterTool( new PCB_EDIT_TABLE_TOOL );
    m_toolManager->RegisterTool( new PAD_TOOL );
    m_toolManager->RegisterTool( new DRAWING_TOOL );
    m_toolManager->RegisterTool( new PCB_POINT_EDITOR );
    m_toolManager->RegisterTool( new PCB_CONTROL );            // copy/paste
    m_toolManager->RegisterTool( new LIBRARY_EDITOR_CONTROL );
    m_toolManager->RegisterTool( new FOOTPRINT_EDITOR_CONTROL );
    m_toolManager->RegisterTool( new ALIGN_DISTRIBUTE_TOOL );
    m_toolManager->RegisterTool( new PCB_PICKER_TOOL );
    m_toolManager->RegisterTool( new POSITION_RELATIVE_TOOL );
    m_toolManager->RegisterTool( new ARRAY_TOOL );
    m_toolManager->RegisterTool( new PCB_VIEWER_TOOLS );
    m_toolManager->RegisterTool( new PCB_GROUP_TOOL );
    m_toolManager->RegisterTool( new CONSTRAINT_EDIT_TOOL );
    m_toolManager->RegisterTool( new CONVERT_TOOL );
    m_toolManager->RegisterTool( new PROPERTIES_TOOL );
    m_toolManager->RegisterTool( new MATCH_PROPERTIES_TOOL );
    m_toolManager->RegisterTool( new GRAPHIC_EDIT_TOOL );
    m_toolManager->RegisterTool( new EMBED_TOOL );

    for( TOOL_BASE* tool : m_toolManager->Tools() )
    {
        if( PCB_TOOL_BASE* pcbTool = dynamic_cast<PCB_TOOL_BASE*>( tool ) )
            pcbTool->SetIsFootprintEditor( true );
    }

    m_toolManager->GetTool<PCB_VIEWER_TOOLS>()->SetFootprintFrame( true );
    m_toolManager->InitTools();

    m_toolManager->InvokeTool( "common.InteractiveSelection" );

    // Load or reload wizard plugins in case they changed since the last time the frame opened
    // Because the board editor has also a plugin python menu,
    // call the PCB_EDIT_FRAME RunAction() if the board editor is running
    // Otherwise run the current RunAction().
    PCB_EDIT_FRAME* pcbframe = static_cast<PCB_EDIT_FRAME*>( Kiway().Player( FRAME_PCB_EDITOR, false ) );

    if( pcbframe )
        pcbframe->GetToolManager()->RunAction( ACTIONS::pluginsReload );
    else
        m_toolManager->RunAction( ACTIONS::pluginsReload );
}


void FOOTPRINT_EDIT_FRAME::setupUIConditions()
{
    PCB_BASE_EDIT_FRAME::setupUIConditions();

    ACTION_MANAGER*       mgr = m_toolManager->GetActionManager();
    PCB_EDITOR_CONDITIONS cond( this );

    wxCHECK2( mgr, return );

#define ENABLE( x ) ACTION_CONDITIONS().Enable( x )
#define CHECK( x )  ACTION_CONDITIONS().Check( x )

    auto haveFootprintCond =
            [this]( const SELECTION& )
            {
                return GetBoard() && GetBoard()->GetFirstFootprint() != nullptr;
            };

    auto editableCond =
            [this]( const SELECTION& )
            {
                return !IsContentReadOnly();
            };

    auto footprintTargettedCond =
            [this]( const SELECTION& )
            {
                return !GetTargetFPID().GetLibItemName().empty();
            };

    auto footprintSelectedInTreeCond =
            [this]( const SELECTION& )
            {
                LIB_ID sel = GetLibTree()->GetSelectedLibId();
                return !sel.GetLibNickname().empty() && !sel.GetLibItemName().empty();
            };

    // Pinned copies keep the entry enabled since GTK shows no tooltip on a disabled item, so the click explains
    auto canCheckInCond =
            [this]( const SELECTION& )
            {
                wxString libName = GetLibTree()->GetSelectedLibId().GetLibNickname();
                return MdwCanCheckInLibrary( LIBRARY_TABLE_TYPE::FOOTPRINT, libName );
            };

    const auto footprintFromBoardCond =
            [this]( const SELECTION& )
            {
                return IsCurrentFPFromBoard();
            };

    auto pcbFrameExistsCond =
            [this]( const SELECTION& )
            {
                PCB_EDIT_FRAME* frame = dynamic_cast<PCB_EDIT_FRAME*>( Kiway().Player( FRAME_PCB_EDITOR, false ) );

                return ( frame != nullptr );
            };

    auto boardFootprintExistsCond =
            [this]( const SELECTION& )
            {
                PCB_EDIT_FRAME* frame = dynamic_cast<PCB_EDIT_FRAME*>( Kiway().Player( FRAME_PCB_EDITOR, false ) );

                FOOTPRINT* editorFootprint = GetBoard()->GetFirstFootprint();
                bool       canInsert = frame && editorFootprint && editorFootprint->GetLink() == niluuid;

                // If the source was deleted, the footprint can inserted but not updated in the board.
                if( frame && editorFootprint && editorFootprint->GetLink() != niluuid )
                {
                    BOARD*  mainpcb = frame->GetBoard();
                    canInsert = true;

                    // search if the source footprint was not deleted:
                    for( FOOTPRINT* candidate : mainpcb->Footprints() )
                    {
                        if( editorFootprint->GetLink() == candidate->m_Uuid )
                        {
                            canInsert = false;
                            break;
                        }
                    }
                }

                return canInsert;
            };

    // clang-format off
    mgr->SetConditions( ACTIONS::saveAs,                 ENABLE( footprintTargettedCond ) );
    mgr->SetConditions( ACTIONS::exportForOlderVersion, ENABLE( footprintTargettedCond ) );
    mgr->SetConditions( ACTIONS::revert,                 ENABLE( cond.ContentModified() ) );
    mgr->SetConditions( ACTIONS::checkInTeamLibrary,     ENABLE( canCheckInCond ) );
    mgr->SetConditions( ACTIONS::save,                   ENABLE( SELECTION_CONDITIONS::ShowAlways ) );
    mgr->SetConditions( PCB_ACTIONS::editLibFpInFpEditor,ENABLE( footprintFromBoardCond ) );

    mgr->SetConditions( PCB_ACTIONS::saveFpToBoard,      ENABLE( boardFootprintExistsCond ) );
    mgr->SetConditions( PCB_ACTIONS::loadFpFromBoard,    ENABLE( pcbFrameExistsCond ) );

    SELECTION_CONDITION canPasteCond = editableCond && SELECTION_CONDITIONS::Idle && cond.NoActiveTool();

    mgr->SetConditions( ACTIONS::undo,                   ENABLE( editableCond && cond.UndoAvailable() ) );
    mgr->SetConditions( ACTIONS::redo,                   ENABLE( editableCond && cond.RedoAvailable() ) );

    mgr->SetConditions( ACTIONS::toggleGrid,             CHECK( cond.GridVisible() ) );
    mgr->SetConditions( ACTIONS::toggleGridOverrides,    CHECK( cond.GridOverrides() ) );

    mgr->SetConditions( ACTIONS::cut,                    ENABLE( editableCond && cond.HasItems() ) );
    mgr->SetConditions( ACTIONS::copy,                   ENABLE( cond.HasItems() ) );
    mgr->SetConditions( ACTIONS::paste,                  ENABLE( canPasteCond ) );
    mgr->SetConditions( ACTIONS::pasteSpecial,           ENABLE( canPasteCond ) );
    mgr->SetConditions( ACTIONS::doDelete,               ENABLE( editableCond && cond.HasItems() ) );
    mgr->SetConditions( ACTIONS::duplicate,              ENABLE( editableCond && cond.HasItems() ) );
    mgr->SetConditions( ACTIONS::selectAll,              ENABLE( cond.HasItems() ) );
    mgr->SetConditions( ACTIONS::unselectAll,            ENABLE( cond.HasItems() ) );

    mgr->SetConditions( PCB_ACTIONS::rotateCw,           ENABLE( editableCond && SELECTION_CONDITIONS::NotEmpty )
                                                             .HotkeyEnable( editableCond && cond.HasItems() ) );
    mgr->SetConditions( PCB_ACTIONS::rotateCcw,          ENABLE( editableCond && SELECTION_CONDITIONS::NotEmpty )
                                                             .HotkeyEnable( editableCond && cond.HasItems() ) );
    mgr->SetConditions( PCB_ACTIONS::mirrorH,            ENABLE( editableCond && SELECTION_CONDITIONS::NotEmpty )
                                                             .HotkeyEnable( editableCond && cond.HasItems() ) );
    mgr->SetConditions( PCB_ACTIONS::mirrorV,            ENABLE( editableCond && SELECTION_CONDITIONS::NotEmpty )
                                                             .HotkeyEnable( editableCond && cond.HasItems() ) );
    mgr->SetConditions( PCB_ACTIONS::mirrorInteractive,  ENABLE( editableCond && SELECTION_CONDITIONS::NotEmpty )
                                                             .HotkeyEnable( editableCond && cond.HasItems() ) );
    mgr->SetConditions( ACTIONS::group,                  ENABLE( SELECTION_CONDITIONS::MoreThan( 1 ) ) );
    mgr->SetConditions( ACTIONS::ungroup,                ENABLE( SELECTION_CONDITIONS::HasType( PCB_GROUP_T ) ) );

    mgr->SetConditions( PCB_ACTIONS::padDisplayMode,     CHECK( !cond.PadFillDisplay() ) );
    mgr->SetConditions( PCB_ACTIONS::textOutlines,       CHECK( !cond.TextFillDisplay() ) );
    mgr->SetConditions( PCB_ACTIONS::graphicsOutlines,   CHECK( !cond.GraphicsFillDisplay() ) );

    mgr->SetConditions( ACTIONS::zoomTool,               CHECK( cond.CurrentTool( ACTIONS::zoomTool ) ) );
    mgr->SetConditions( ACTIONS::selectionTool,          CHECK( cond.CurrentTool( ACTIONS::selectionTool ) ) );
    mgr->SetConditions( ACTIONS::selectSetRect,          CHECK( cond.CurrentTool( ACTIONS::selectionTool ) ) );
    mgr->SetConditions( ACTIONS::selectSetLasso,         CHECK( cond.CurrentTool( ACTIONS::selectionTool ) ) );
    // clang-format on

    auto highContrastCond =
            [this]( const SELECTION& )
            {
                return GetDisplayOptions().m_ContrastModeDisplay != HIGH_CONTRAST_MODE::NORMAL;
            };

    auto autoConstraintsCond = [this]( const SELECTION& )
    {
        FOOTPRINT_EDITOR_SETTINGS* cfg = GetSettings();
        return cfg && cfg->m_AutoConstraints;
    };

    mgr->SetConditions( PCB_ACTIONS::toggleAutoConstraints, CHECK( autoConstraintsCond ) );

    auto boardFlippedCond = [this]( const SELECTION& )
    {
        return GetDisplayOptions().m_FlipBoardView;
    };

    auto libraryTreeCond =
            [this](const SELECTION& )
            {
                return IsLibraryTreeShown();
            };

    auto layerManagerCond =
            [this]( const SELECTION& )
            {
                return m_auimgr.GetPane( "LayersManager" ).IsShown();
            };

    auto propertiesCond =
            [this] ( const SELECTION& )
            {
                return m_auimgr.GetPane( PropertiesPaneName() ).IsShown();
            };

    mgr->SetConditions( ACTIONS::highContrastMode,          CHECK( highContrastCond ) );
    mgr->SetConditions( PCB_ACTIONS::flipBoard,             CHECK( boardFlippedCond ) );
    mgr->SetConditions( ACTIONS::toggleBoundingBoxes,       CHECK( cond.BoundingBoxes() ) );

    mgr->SetConditions( ACTIONS::showLibraryTree,           CHECK( libraryTreeCond ) );
    mgr->SetConditions( PCB_ACTIONS::showLayersManager,     CHECK( layerManagerCond ) );
    mgr->SetConditions( ACTIONS::showProperties,            CHECK( propertiesCond ) );

    mgr->SetConditions( ACTIONS::print,                     ENABLE( haveFootprintCond ) );
    mgr->SetConditions( PCB_ACTIONS::exportFootprint,       ENABLE( haveFootprintCond ) );
    mgr->SetConditions( PCB_ACTIONS::placeImportedGraphics, ENABLE( haveFootprintCond ) );

    mgr->SetConditions( PCB_ACTIONS::footprintProperties,   ENABLE( footprintSelectedInTreeCond || haveFootprintCond ) );
    mgr->SetConditions( PCB_ACTIONS::padTable,              ENABLE( haveFootprintCond ) );
    mgr->SetConditions( PCB_ACTIONS::editTextAndGraphics,   ENABLE( haveFootprintCond ) );
    mgr->SetConditions( PCB_ACTIONS::checkFootprint,        ENABLE( haveFootprintCond ) );
    mgr->SetConditions( PCB_ACTIONS::repairFootprint,       ENABLE( haveFootprintCond ) );
    mgr->SetConditions( PCB_ACTIONS::cleanupGraphics,       ENABLE( haveFootprintCond ) );
    mgr->SetConditions( ACTIONS::showDatasheet,             ENABLE( haveFootprintCond ) );

    const auto isArcKeepCenterMode =
            [this]( const SELECTION& )
            {
                return GetSettings()->m_ArcEditMode == ARC_EDIT_MODE::KEEP_CENTER_ADJUST_ANGLE_RADIUS;
            };

    const auto isArcKeepEndpointMode =
            [this]( const SELECTION& )
            {
                return GetSettings()->m_ArcEditMode == ARC_EDIT_MODE::KEEP_ENDPOINTS_OR_START_DIRECTION;
            };

    const auto isArcKeepRadiusMode =
            [this]( const SELECTION& )
            {
                return GetSettings()->m_ArcEditMode == ARC_EDIT_MODE::KEEP_CENTER_ENDS_ADJUST_ANGLE;
            };

    // clang-format off
    mgr->SetConditions( ACTIONS::pointEditorArcKeepCenter,   CHECK( isArcKeepCenterMode ) );
    mgr->SetConditions( ACTIONS::pointEditorArcKeepEndpoint, CHECK( isArcKeepEndpointMode ) );
    mgr->SetConditions( ACTIONS::pointEditorArcKeepRadius,   CHECK( isArcKeepRadiusMode ) );
    // clang-format on

    mgr->SetConditions( ACTIONS::measureTool, ACTION_CONDITIONS().Enable( haveFootprintCond )
                                                             .Check( cond.CurrentTool( ACTIONS::measureTool ) ) );

// Only enable a tool if the part is editable
#define CURRENT_EDIT_TOOL( action ) mgr->SetConditions( action, ACTION_CONDITIONS()                  \
                                                .Enable( haveFootprintCond && editableCond )         \
                                                .Check( cond.CurrentTool( action ) ) )

    CURRENT_EDIT_TOOL( ACTIONS::deleteTool );
    CURRENT_EDIT_TOOL( ACTIONS::embeddedFiles );
    CURRENT_EDIT_TOOL( PCB_ACTIONS::placePad );
    CURRENT_EDIT_TOOL( PCB_ACTIONS::placeThtPad );
    CURRENT_EDIT_TOOL( PCB_ACTIONS::placeSmdPad );
    CURRENT_EDIT_TOOL( PCB_ACTIONS::placeNpthPad );
    CURRENT_EDIT_TOOL( PCB_ACTIONS::placePasteAperturePad );
    CURRENT_EDIT_TOOL( PCB_ACTIONS::drawLine );
    CURRENT_EDIT_TOOL( PCB_ACTIONS::drawRectangle );
    CURRENT_EDIT_TOOL( PCB_ACTIONS::drawCircle );
    CURRENT_EDIT_TOOL( PCB_ACTIONS::drawEllipse );
    CURRENT_EDIT_TOOL( PCB_ACTIONS::drawEllipseArc );
    CURRENT_EDIT_TOOL( PCB_ACTIONS::drawArc );
    setArcModeConditions( PCB_ACTIONS::drawArc, haveFootprintCond && editableCond );
    CURRENT_EDIT_TOOL( PCB_ACTIONS::drawPolygon );
    CURRENT_EDIT_TOOL( PCB_ACTIONS::drawBezier );
    CURRENT_EDIT_TOOL( PCB_ACTIONS::drawRuleArea );
    CURRENT_EDIT_TOOL( PCB_ACTIONS::placePoint );
    CURRENT_EDIT_TOOL( PCB_ACTIONS::placeReferenceImage );
    CURRENT_EDIT_TOOL( PCB_ACTIONS::placeBarcode );
    CURRENT_EDIT_TOOL( PCB_ACTIONS::placeText );
    CURRENT_EDIT_TOOL( PCB_ACTIONS::drawTextBox );
    CURRENT_EDIT_TOOL( PCB_ACTIONS::drawTable );
    CURRENT_EDIT_TOOL( PCB_ACTIONS::drawAlignedDimension );
    CURRENT_EDIT_TOOL( PCB_ACTIONS::drawOrthogonalDimension );
    CURRENT_EDIT_TOOL( PCB_ACTIONS::drawCenterDimension );
    CURRENT_EDIT_TOOL( PCB_ACTIONS::drawRadialDimension );
    CURRENT_EDIT_TOOL( PCB_ACTIONS::drawLeader );
    CURRENT_EDIT_TOOL( PCB_ACTIONS::setAnchor );
    CURRENT_EDIT_TOOL( ACTIONS::gridSetOrigin );

    CURRENT_EDIT_TOOL( PCB_ACTIONS::addConstraintCoincident );
    CURRENT_EDIT_TOOL( PCB_ACTIONS::addConstraintPointOnLine );
    CURRENT_EDIT_TOOL( PCB_ACTIONS::addConstraintMidpoint );
    CURRENT_EDIT_TOOL( PCB_ACTIONS::addConstraintSymmetric );
    CURRENT_EDIT_TOOL( PCB_ACTIONS::addConstraintFixedPosition );
    CURRENT_EDIT_TOOL( PCB_ACTIONS::addConstraintParallel );
    CURRENT_EDIT_TOOL( PCB_ACTIONS::addConstraintPerpendicular );
    CURRENT_EDIT_TOOL( PCB_ACTIONS::addConstraintCollinear );
    CURRENT_EDIT_TOOL( PCB_ACTIONS::addConstraintHorizontal );
    CURRENT_EDIT_TOOL( PCB_ACTIONS::addConstraintVertical );
    CURRENT_EDIT_TOOL( PCB_ACTIONS::addConstraintTangent );
    CURRENT_EDIT_TOOL( PCB_ACTIONS::addConstraintEqualLength );
    CURRENT_EDIT_TOOL( PCB_ACTIONS::addConstraintEqualRadius );
    CURRENT_EDIT_TOOL( PCB_ACTIONS::addConstraintConcentric );
    CURRENT_EDIT_TOOL( PCB_ACTIONS::addConstraintFixedLength );
    CURRENT_EDIT_TOOL( PCB_ACTIONS::addConstraintFixedRadius );
    CURRENT_EDIT_TOOL( PCB_ACTIONS::addConstraintArcAngle );
    CURRENT_EDIT_TOOL( PCB_ACTIONS::addConstraintAngular );

#undef CURRENT_EDIT_TOOL
#undef ENABLE
#undef CHECK
}


void FOOTPRINT_EDIT_FRAME::ActivateGalCanvas()
{
    PCB_BASE_EDIT_FRAME::ActivateGalCanvas();

    // Be sure the axis are enabled
    GetCanvas()->GetGAL()->SetAxesEnabled( true );

    UpdateView();

    // Ensure the m_Layers settings are using the canvas type:
    UpdateUserInterface();
}


void FOOTPRINT_EDIT_FRAME::CommonSettingsChanged( int aFlags )
{
    PCB_BASE_EDIT_FRAME::CommonSettingsChanged( aFlags );
    m_appearancePanel->CommonSettingsChanged( aFlags );

    if( FOOTPRINT_EDITOR_SETTINGS* cfg = GetAppSettings<FOOTPRINT_EDITOR_SETTINGS>( "fpedit" ) )
    {
        // This editor always shows the axes, whatever a settings file not yet saved by it says
        cfg->m_Window.grid.axes_enabled = true;
        GetGalDisplayOptions().ReadWindowSettings( cfg->m_Window );

        GetBoard()->GetDesignSettings() = cfg->m_DesignSettings;
        updateEnabledLayers();
    }

    GetCanvas()->GetView()->UpdateAllLayersColor();
    GetCanvas()->GetView()->MarkTargetDirty( KIGFX::TARGET_NONCACHED );
    GetCanvas()->ForceRefresh();

    UpdateUserInterface();

    if( aFlags & ENVVARS_CHANGED )
        SyncLibraryTree( true );

    // The workspace preference decides whether the open footprint is a team part at all, and which libraries
    // show a lifecycle, and so what a filter matches
    if( m_teamPart.IntegrationToggled() )
    {
        refreshTeamPart( true );
        updateInfoBar();

        if( m_adapter )
            m_adapter->RefreshLifecycle();

        if( m_treePane )
            GetLibTree()->Regenerate( true );
    }

    Layout();
    SendSizeEvent();
}


std::unique_ptr<GRID_HELPER> FOOTPRINT_EDIT_FRAME::MakeGridHelper()
{
    return std::make_unique<PCB_GRID_HELPER>( m_toolManager, GetMagneticItemsSettings() );
}


void FOOTPRINT_EDIT_FRAME::OnSaveFootprintAsPng( wxCommandEvent& event )
{
    LIB_ID id = GetLoadedFPID();

    if( id.empty() )
    {
        DisplayErrorMessage( this, _( "No footprint selected." ) );
        return;
    }

    wxFileName fn( id.GetLibItemName() );
    fn.SetExt( wxT( "png" ) );

    wxString projectPath = wxPathOnly( Prj().GetProjectFullName() );

    wxFileDialog dlg( this, _( "Export View as PNG" ), projectPath, fn.GetFullName(), FILEEXT::PngFileWildcard(),
                      wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    KIPLATFORM::UI::AllowNetworkFileSystems( &dlg );

    if( dlg.ShowModal() == wxID_CANCEL || dlg.GetPath().IsEmpty() )
        return;

    // calling wxYield is mandatory under Linux, after closing the file selector dialog
    // to refresh the screen before creating the PNG or JPEG image from screen
    wxYield();
    this->SaveCanvasImageToFile( dlg.GetPath(), BITMAP_TYPE::PNG );
}
