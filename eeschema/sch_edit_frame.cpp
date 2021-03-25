/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2020 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <class_library.h>
#include <confirm.h>
#include <connection_graph.h>
#include <dialogs/dialog_schematic_find.h>
#include <eeschema_id.h>
#include <executable_names.h>
#include <gestfich.h>
#include <hierarch.h>
#include <dialogs/html_messagebox.h>
#include <invoke_sch_dialog.h>
#include <kicad_string.h>
#include <kiface_i.h>
#include <kiplatform/app.h>
#include <kiway.h>
#include <symbol_edit_frame.h>
#include <symbol_viewer_frame.h>
#include <pgm_base.h>
#include <profile.h>
#include <project.h>
#include <project/project_file.h>
#include <project/net_settings.h>
#include <sch_edit_frame.h>
#include <sch_painter.h>
#include <sch_sheet.h>
#include <sch_marker.h>
#include <schematic.h>
#include <settings/settings_manager.h>
#include <advanced_config.h>
#include <sim/sim_plot_frame.h>
#include <tool/action_manager.h>
#include <tool/action_toolbar.h>
#include <tool/common_control.h>
#include <tool/common_tools.h>
#include <tool/editor_conditions.h>
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
#include <tools/sch_editor_control.h>
#include <tools/sch_line_wire_bus_tool.h>
#include <tools/sch_move_tool.h>
#include <tools/sch_navigate_tool.h>
#include <view/view_controls.h>
#include <widgets/infobar.h>
#include <wildcards_and_files_ext.h>
#include <wx/cmdline.h>
#include <gal/graphics_abstraction_layer.h>
#include <drawing_sheet/ds_proxy_view_item.h>

// non-member so it can be moved easily, and kept REALLY private.
// Do NOT Clear() in here.
static void add_search_paths( SEARCH_STACK* aDst, const SEARCH_STACK& aSrc, int aIndex )
{
    for( unsigned i=0; i<aSrc.GetCount();  ++i )
        aDst->AddPaths( aSrc[i], aIndex );
}


//-----<SCH "data on demand" functions>-------------------------------------------

SEARCH_STACK* PROJECT::SchSearchS()
{
    SEARCH_STACK* ss = (SEARCH_STACK*) GetElem( PROJECT::ELEM_SCH_SEARCH_STACK );

    wxASSERT( !ss || dynamic_cast<SEARCH_STACK*>( GetElem( PROJECT::ELEM_SCH_SEARCH_STACK ) ) );

    if( !ss )
    {
        ss = new SEARCH_STACK();

        // Make PROJECT the new SEARCH_STACK owner.
        SetElem( PROJECT::ELEM_SCH_SEARCH_STACK, ss );

        // to the empty SEARCH_STACK for SchSearchS(), add project dir as first
        ss->AddPaths( m_project_name.GetPath() );

        // next add the paths found in *.pro, variable "LibDir"
        wxString        libDir;

        try
        {
            PART_LIBS::LibNamesAndPaths( this, false, &libDir );
        }
        catch( const IO_ERROR& )
        {
        }

        if( !!libDir )
        {
            wxArrayString   paths;

            SEARCH_STACK::Split( &paths, libDir );

            for( unsigned i =0; i<paths.GetCount();  ++i )
            {
                wxString path = AbsolutePath( paths[i] );

                ss->AddPaths( path );     // at the end
            }
        }

        // append all paths from aSList
        add_search_paths( ss, Kiface().KifaceSearch(), -1 );
    }

    return ss;
}


PART_LIBS* PROJECT::SchLibs()
{
    PART_LIBS* libs = (PART_LIBS*) GetElem( PROJECT::ELEM_SCH_PART_LIBS );

    wxASSERT( !libs || libs->Type() == PART_LIBS_T );

    if( !libs )
    {
        libs = new PART_LIBS();

        // Make PROJECT the new PART_LIBS owner.
        SetElem( PROJECT::ELEM_SCH_PART_LIBS, libs );

        try
        {
            libs->LoadAllLibraries( this );
        }
        catch( const PARSE_ERROR& pe )
        {
            wxString    lib_list = UTF8( pe.inputLine );
            wxWindow*   parent = Pgm().App().GetTopWindow();

            // parent of this dialog cannot be NULL since that breaks the Kiway() chain.
            HTML_MESSAGE_BOX dlg( parent, _( "Not Found" ) );

            dlg.MessageSet( _( "The following libraries were not found:" ) );

            dlg.ListSet( lib_list );

            dlg.Layout();

            dlg.ShowModal();
        }
        catch( const IO_ERROR& ioe )
        {
            wxWindow* parent = Pgm().App().GetTopWindow();

            DisplayError( parent, ioe.What() );
        }
    }

    return libs;
}

//-----</SCH "data on demand" functions>------------------------------------------


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
    m_hasAutoSave = true;
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

    // Also links the schematic to the loaded project
    CreateScreens();

    // After schematic has been linked to project, SCHEMATIC_SETTINGS works
    m_defaults = &m_schematic->Settings();
    LoadProjectSettings();

    setupTools();
    setupUIConditions();
    ReCreateMenuBar();
    ReCreateHToolbar();
    ReCreateVToolbar();
    ReCreateOptToolbar();

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
                      .Left().Layer( 3 ) );
    m_auimgr.AddPane( m_drawToolBar, EDA_PANE().VToolbar().Name( "ToolsToolbar" )
                      .Right().Layer( 2 ) );
    m_auimgr.AddPane( GetCanvas(), EDA_PANE().Canvas().Name( "DrawFrame" )
                      .Center() );
    m_auimgr.AddPane( m_messagePanel, EDA_PANE().Messages().Name( "MsgPanel" )
                      .Bottom().Layer( 6 ) );

    FinishAUIInitialization();

    resolveCanvasType();
    SwitchCanvas( m_canvasType );

    initScreenZoom();

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

    // Default shutdown reason until a file is loaded
    KIPLATFORM::APP::SetShutdownBlockReason( this, _( "New schematic file is unsaved" ) );

    // Ensure the window is on top
    Raise();
}


SCH_EDIT_FRAME::~SCH_EDIT_FRAME()
{
    // Shutdown all running tools
    if( m_toolManager )
    {
        m_toolManager->ShutdownAllTools();
        delete m_toolManager;
        m_toolManager = nullptr;
    }

    delete m_item_to_repeat;        // we own the cloned object, see this->SaveCopyForRepeatItem()

    SetScreen( NULL );

    delete m_schematic;
    m_schematic = nullptr;

    // Close the project if we are standalone, so it gets cleaned up properly
    if( Kiface().IsSingle() )
        GetSettingsManager()->UnloadProject( &Prj(), false );
}


void SCH_EDIT_FRAME::setupTools()
{
    // Create the manager and dispatcher & route draw panel events to the dispatcher
    m_toolManager = new TOOL_MANAGER;
    m_toolManager->SetEnvironment( &Schematic(), GetCanvas()->GetView(),
                                   GetCanvas()->GetViewControls(), config(), this );
    m_actions = new EE_ACTIONS();
    m_toolDispatcher = new TOOL_DISPATCHER( m_toolManager, m_actions );

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
    EDITOR_CONDITIONS cond( this );

    wxASSERT( mgr );

    auto hasElements =
            [ this ] ( const SELECTION& aSel )
            {
                return !GetScreen()->Items().empty() || !SELECTION_CONDITIONS::Idle( aSel );
            };

#define ENABLE( x ) ACTION_CONDITIONS().Enable( x )
#define CHECK( x )  ACTION_CONDITIONS().Check( x )

    mgr->SetConditions( ACTIONS::save,                ENABLE( SELECTION_CONDITIONS::ShowAlways ) );
    mgr->SetConditions( ACTIONS::undo,                ENABLE( cond.UndoAvailable() ) );
    mgr->SetConditions( ACTIONS::redo,                ENABLE( cond.RedoAvailable() ) );

    mgr->SetConditions( ACTIONS::toggleGrid,          CHECK( cond.GridVisible() ) );
    mgr->SetConditions( ACTIONS::toggleCursorStyle,   CHECK( cond.FullscreenCursor() ) );
    mgr->SetConditions( ACTIONS::millimetersUnits,    CHECK( cond.Units( EDA_UNITS::MILLIMETRES ) ) );
    mgr->SetConditions( ACTIONS::inchesUnits,         CHECK( cond.Units( EDA_UNITS::INCHES ) ) );
    mgr->SetConditions( ACTIONS::milsUnits,           CHECK( cond.Units( EDA_UNITS::MILS ) ) );

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

    mgr->SetConditions( ACTIONS::zoomTool,            CHECK( cond.CurrentTool( ACTIONS::zoomTool ) ) );
    mgr->SetConditions( ACTIONS::selectionTool,       CHECK( cond.CurrentTool( ACTIONS::selectionTool ) ) );

    auto showHiddenPinsCond =
        [this] ( const SELECTION& )
        {
            return GetShowAllPins();
        };

    auto forceHVCond =
        [this] ( const SELECTION& )
        {
            return eeconfig()->m_Drawing.hv_lines_only;
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
            return GetCurrentSheet().Last() != &Schematic().Root();
        };

    mgr->SetConditions( EE_ACTIONS::leaveSheet,         ENABLE( belowRootSheetCondition ) );
    mgr->SetConditions( EE_ACTIONS::remapSymbols,       ENABLE( remapSymbolsCondition ) );
    mgr->SetConditions( EE_ACTIONS::toggleHiddenPins,   CHECK( showHiddenPinsCond ) );
    mgr->SetConditions( EE_ACTIONS::toggleForceHV,      CHECK( forceHVCond ) );


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
    CURRENT_TOOL( EE_ACTIONS::placeGlobalLabel );
    CURRENT_TOOL( EE_ACTIONS::placeHierLabel );
    CURRENT_TOOL( EE_ACTIONS::drawSheet );
    CURRENT_TOOL( EE_ACTIONS::importSheetPin );
    CURRENT_TOOL( EE_ACTIONS::drawLines );
    CURRENT_TOOL( EE_ACTIONS::placeSchematicText );
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

    // @todo Remove all psuedo page number system is left over from prior to real page number
    //       implementation.
    for( const SCH_SHEET_PATH& sheet : Schematic().GetSheets() )
    {
        if( sheet.Path() == current_sheetpath )  // Current sheet path found
            break;

        sheet_number++;                          // Not found, increment before this current path
    }

    for( screen = s_list.GetFirst(); screen != NULL; screen = s_list.GetNext() )
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

    m_defaults = &m_schematic->Settings();

    SCH_SCREEN* rootScreen = new SCH_SCREEN( m_schematic );
    m_schematic->Root().SetScreen( rootScreen );
    SetScreen( Schematic().RootScreen() );

    m_schematic->RootScreen()->SetFileName( wxEmptyString );

    // Don't leave root page number empty
    SCH_SHEET_PATH rootSheetPath;
    rootSheetPath.push_back( &m_schematic->Root() );
    m_schematic->RootScreen()->SetPageNumber( wxT( "1" ) );
    m_schematic->Root().AddInstance( rootSheetPath.Path() );
    m_schematic->Root().SetPageNumber( rootSheetPath, wxT( "1" ) );

    if( GetScreen() == NULL )
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
        wxString msg = _( "Save changes to \"%s\" before closing?" );

        if( !HandleUnsavedChanges( this, wxString::Format( msg, fileName.GetFullName() ),
                                   [&]()->bool { return SaveProject(); } ) )
        {
            return false;
        }
    }

    return true;
}


void SCH_EDIT_FRAME::doCloseWindow()
{
    SCH_SHEET_LIST sheetlist = Schematic().GetSheets();

    // Shutdown all running tools
    if( m_toolManager )
        m_toolManager->ShutdownAllTools();

    RecordERCExclusions();

    // Close the find dialog and preserve it's setting if it is displayed.
    if( m_findReplaceDialog )
    {
        m_findStringHistoryList = m_findReplaceDialog->GetFindEntries();
        m_replaceStringHistoryList = m_findReplaceDialog->GetReplaceEntries();

        m_findReplaceDialog->Destroy();
        m_findReplaceDialog = nullptr;
    }

    if( FindHierarchyNavigator() )
        FindHierarchyNavigator()->Close( true );

    SCH_SCREENS screens( Schematic().Root() );
    wxFileName fn;

    for( SCH_SCREEN* screen = screens.GetFirst(); screen != NULL; screen = screens.GetNext() )
    {
        fn = Prj().AbsolutePath( screen->GetFileName() );

        // Auto save file name is the normal file name prepended with GetAutoSaveFilePrefix().
        fn.SetName( GetAutoSaveFilePrefix() + fn.GetName() );

        if( fn.FileExists() && fn.IsFileWritable() )
            wxRemoveFile( fn.GetFullPath() );
    }

    sheetlist.ClearModifyStatus();

    wxString fileName = Prj().AbsolutePath( Schematic().RootScreen()->GetFileName() );

    if( !Schematic().GetFileName().IsEmpty() && !Schematic().RootScreen()->IsEmpty() )
        UpdateFileHistory( fileName );

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
        (void) sheetList.GetItem( marker->GetRCItem()->GetMainItemID(), &errorPath );

        if( errorPath.LastScreen() )
            errorPath.LastScreen()->Append( marker );
        else
            Schematic().RootScreen()->Append( marker );
    }
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

    GetScreen()->SetModify();
    GetScreen()->SetSave();

    if( ADVANCED_CFG::GetCfg().m_RealTimeConnectivity && CONNECTION_GRAPH::m_allowRealTime )
        RecalculateConnections( NO_CLEANUP );

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

                return false;
            } );

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


wxFindReplaceData* SCH_EDIT_FRAME::GetFindReplaceData()
{
    if( m_findReplaceDialog && m_findReplaceDialog->IsVisible()
            && !m_findReplaceData->GetFindString().IsEmpty() )
    {
        return m_findReplaceData;
    }

    return nullptr;
}


HIERARCHY_NAVIG_DLG* SCH_EDIT_FRAME::FindHierarchyNavigator()
{
    wxWindow* navigator = wxWindow::FindWindowByName( HIERARCHY_NAVIG_DLG_WNAME );

    return static_cast< HIERARCHY_NAVIG_DLG* >( navigator );
}


void SCH_EDIT_FRAME::UpdateHierarchyNavigator( bool aForceUpdate )
{
    if( aForceUpdate )
    {
        if( FindHierarchyNavigator() )
            FindHierarchyNavigator()->Close();

        HIERARCHY_NAVIG_DLG* hierarchyDialog = new HIERARCHY_NAVIG_DLG( this );

        hierarchyDialog->Show( true );
    }
    else
    {
        if( FindHierarchyNavigator() )
            FindHierarchyNavigator()->UpdateHierarchyTree();
    }
}


void SCH_EDIT_FRAME::ShowFindReplaceDialog( bool aReplace )
{
    if( m_findReplaceDialog )
        m_findReplaceDialog->Destroy();

    m_findReplaceDialog= new DIALOG_SCH_FIND( this, m_findReplaceData, wxDefaultPosition,
                                              wxDefaultSize, aReplace ? wxFR_REPLACEDIALOG : 0 );

    m_findReplaceDialog->SetFindEntries( m_findStringHistoryList );
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
            msg.Printf( _( "Schematic file \"%s\" already exists." ), create_me.GetFullName() );
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
    wxString wildcards = KiCadSchematicFileWildcard();

    wildcards += "|" + LegacySchematicFileWildcard();

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
            wxString filename = QuoteFullPath( boardfn );
            ExecuteFile( this, PCBNEW_EXE, filename );
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
        ExecuteFile( this, PCBNEW_EXE );
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
        SCH_SCREENS screenList( Schematic().Root() );

        for( SCH_SCREEN* screen = screenList.GetFirst(); screen; screen = screenList.GetNext() )
        {
            if( screen->IsSave() )
                return true;
        }
    }

    return false;
}


static void inheritNetclass( const SCH_SHEET_PATH& aSheetPath, SCH_TEXT* aItem )
{
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
    wxCHECK_RET( aItem != NULL, wxT( "Cannot add null item to list." ) );

    SCH_SHEET*     parentSheet = nullptr;
    SCH_COMPONENT* parentSymbol = nullptr;
    SCH_ITEM*      undoItem = aItem;

    if( aItem->Type() == SCH_SHEET_PIN_T )
    {
        parentSheet = (SCH_SHEET*) aItem->GetParent();

        wxCHECK_RET( parentSheet && parentSheet->Type() == SCH_SHEET_T,
                     wxT( "Cannot place sheet pin in invalid schematic sheet." ) );

        undoItem = parentSheet;
    }

    else if( aItem->Type() == SCH_FIELD_T )
    {
        parentSymbol = (SCH_COMPONENT*) aItem->GetParent();

        wxCHECK_RET( parentSymbol && parentSymbol->Type() == SCH_COMPONENT_T,
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

            if( SCH_TEXT* textItem = dynamic_cast<SCH_TEXT*>( aItem ) )
                inheritNetclass( GetCurrentSheet(), textItem );
        }
    }

    aItem->ClearFlags( IS_NEW );

    aScreen->SetModify();
    UpdateItem( aItem );

    if( !aItem->IsMoving() && aItem->IsConnectable() )
    {
        std::vector< wxPoint > pts = aItem->GetConnectionPoints();

        for( auto i = pts.begin(); i != pts.end(); i++ )
        {
            for( auto j = i + 1; j != pts.end(); j++ )
                TrimWire( *i, *j );

            if( aScreen->IsJunctionNeeded( *i, true ) )
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
    wxString title;

    if( GetScreen()->GetFileName().IsEmpty() )
    {
        title = _( "[no file]" ) + wxT( " \u2014 " ) + _( "Schematic Editor" ),
    }
    else
    {
        wxFileName fn( Prj().AbsolutePath( GetScreen()->GetFileName() ) );
        bool       readOnly = false;
        bool       unsaved = false;

        if( fn.IsOk() && fn.FileExists() )
            readOnly = !fn.IsFileWritable();
        else
            unsaved = true;

        title.Printf( wxT( "%s%s [%s] %s%s\u2014 " ) + _( "Schematic Editor" ),
                      IsContentModified() ? "*" : "",
                      fn.GetName(),
                      GetCurrentSheet().PathHumanReadable( false ),
                      readOnly ? _( "[Read Only]" ) + wxS( " " ) : "",
                      unsaved ? _( "[Unsaved]" ) + wxS( " " ) : "" );
    }

    SetTitle( title );
}


void SCH_EDIT_FRAME::initScreenZoom()
{
    m_toolManager->RunAction( ACTIONS::zoomFitScreen, true );
    GetScreen()->m_zoomInitialized = true;
}


void SCH_EDIT_FRAME::RecalculateConnections( SCH_CLEANUP_FLAGS aCleanupFlags )
{
    SCHEMATIC_SETTINGS& settings = Schematic().Settings();
    SCH_SHEET_LIST list = Schematic().GetSheets();
    PROF_COUNTER   timer;

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

    timer.Stop();
    wxLogTrace( "CONN_PROFILE", "SchematicCleanUp() %0.4f ms", timer.msecs() );

    if( settings.m_IntersheetRefsShow )
        RecomputeIntersheetRefs();

    std::function<void( SCH_ITEM* )> changeHandler =
            [&]( SCH_ITEM* aChangedItem ) -> void
            {
                GetCanvas()->GetView()->Update( aChangedItem, KIGFX::REPAINT );
            };

    Schematic().ConnectionGraph()->Recalculate( list, true, &changeHandler );
}


void SCH_EDIT_FRAME::RecomputeIntersheetRefs()
{
    std::map<wxString, std::set<wxString>>& pageRefsMap = Schematic().GetPageRefsMap();

    pageRefsMap.clear();

    SCH_SCREENS           screens( Schematic().Root() );
    std::vector<wxString> pageNumbers;

    /* Iterate over screens */
    for( SCH_SCREEN* screen = screens.GetFirst(); screen != NULL; screen = screens.GetNext() )
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

    /* Refresh all global labels */
    for( EDA_ITEM* item : GetScreen()->Items().OfType( SCH_GLOBAL_LABEL_T ) )
    {
        SCH_GLOBALLABEL* global = static_cast<SCH_GLOBALLABEL*>( item );

        global->GetIntersheetRefs()->SetVisible( show );

        if( show )
            GetCanvas()->GetView()->Update( global );
    }
}


void SCH_EDIT_FRAME::ShowAllIntersheetRefs( bool aShow )
{
    if( aShow )
        RecomputeIntersheetRefs();

    SCH_SCREENS screens( Schematic().Root() );

    for( SCH_SCREEN* screen = screens.GetFirst(); screen; screen = screens.GetNext() )
    {
        for( SCH_ITEM* item : screen->Items().OfType( SCH_GLOBAL_LABEL_T ) )
        {
            SCH_GLOBALLABEL* gLabel = (SCH_GLOBALLABEL*)( item );
            SCH_FIELD*       intersheetRef = gLabel->GetIntersheetRefs();

            intersheetRef->SetVisible( aShow );
            UpdateItem( intersheetRef, true );
        }
    }
}


void SCH_EDIT_FRAME::CommonSettingsChanged( bool aEnvVarsChanged, bool aTextVarsChanged )
{
    SCHEMATIC_SETTINGS& settings = Schematic().Settings();
    SCH_BASE_FRAME::CommonSettingsChanged( aEnvVarsChanged, aTextVarsChanged );

    ShowAllIntersheetRefs( settings.m_IntersheetRefsShow );

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
            {
                if( item->Type() == SCH_COMPONENT_T )
                {
                    // For symbols we need to get the bounding box without invisible text
                    SCH_COMPONENT* symbol = static_cast<SCH_COMPONENT*>( item );
                    bBoxItems.Merge( symbol->GetBoundingBox( false ) );
                }
                else
                    bBoxItems.Merge( item->GetBoundingBox() );
            }
            bBoxDoc = bBoxItems;
        }
    }
    return bBoxDoc;
}


void SCH_EDIT_FRAME::FixupJunctions()
{
    // Save the current sheet, to retrieve it later
    SCH_SHEET_PATH oldsheetpath = GetCurrentSheet();

    bool modified = false;

    SCH_SHEET_LIST sheetList = Schematic().GetSheets();

    for( const SCH_SHEET_PATH& sheet : sheetList )
    {
        // We require a set here to avoid adding multiple junctions to the same spot
        std::set<wxPoint> junctions;

        SetCurrentSheet( sheet );
        GetCurrentSheet().UpdateAllScreenReferences();

        SCH_SCREEN* screen = GetCurrentSheet().LastScreen();

        for( auto aItem : screen->Items().OfType( SCH_COMPONENT_T ) )
        {
            auto cmp = static_cast<SCH_COMPONENT*>( aItem );

            for( const SCH_PIN* pin : cmp->GetPins( &sheet ) )
            {
                auto pos = pin->GetPosition();

                // Test if a _new_ junction is needed, and add it if missing
                if( screen->IsJunctionNeeded( pos, true ) )
                    junctions.insert( pos );
            }
        }

        for( const wxPoint& pos : junctions )
            AddJunction( screen, pos, false, false );

        if( junctions.size() )
            modified = true;
    }

    if( modified )
        OnModify();

    // Reselect the initial sheet:
    SetCurrentSheet( oldsheetpath );
    GetCurrentSheet().UpdateAllScreenReferences();
    SetScreen( GetCurrentSheet().LastScreen() );
}


bool SCH_EDIT_FRAME::IsContentModified()
{
    return Schematic().GetSheets().IsModified();
}


bool SCH_EDIT_FRAME::GetShowAllPins() const
{
    EESCHEMA_SETTINGS* cfg = eeconfig();
    return cfg->m_Appearance.show_hidden_pins;
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


void SCH_EDIT_FRAME::UpdateSymbolFromEditor( const LIB_PART& aSymbol )
{
    wxString msg;
    bool appendToUndo = false;

    wxCHECK( m_toolManager, /* void */ );

    EE_SELECTION_TOOL* selectionTool = m_toolManager->GetTool<EE_SELECTION_TOOL>();

    wxCHECK( selectionTool, /* void */ );

    EE_SELECTION& selection = selectionTool->RequestSelection( EE_COLLECTOR::ComponentsOnly );

    if( selection.Empty() )
        return;

    SCH_SCREEN* currentScreen = GetScreen();

    wxCHECK( currentScreen, /* void */ );

    // This should work for multiple selections of the same symbol even though the editor
    // only works for a single symbol selection.
    for( EDA_ITEM* item : selection )
    {
        SCH_COMPONENT* symbol = dynamic_cast<SCH_COMPONENT*>( item );

        wxCHECK( symbol, /* void */ );

        // This needs to be done before the LIB_PART is changed to prevent stale library symbols in
        // the schematic file.
        currentScreen->Remove( symbol );

        if( !symbol->IsNew() )
        {
            SaveCopyInUndoList( currentScreen, symbol, UNDO_REDO::CHANGED, appendToUndo );
            appendToUndo = true;
        }

        symbol->SetLibSymbol( aSymbol.Flatten().release() );
        symbol->UpdateFields( true, true );

        currentScreen->Append( symbol );
        selectionTool->SelectHighlightItem( symbol );
        GetCanvas()->GetView()->Update( symbol );
    }

    if( selection.IsHover() )
        m_toolManager->RunAction( EE_ACTIONS::clearSelection, true );

    GetCanvas()->Refresh();
    OnModify();
}
