/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2019 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <confirm.h>
#include <eda_dockart.h>
#include <executable_names.h>
#include <fctsys.h>
#include <gestfich.h>
#include <gr_basic.h>
#include <html_messagebox.h>
#include <kiface_i.h>
#include <kiway.h>
#include <pgm_base.h>
#include <profile.h>
#include <advanced_config.h>
#include <general.h>
#include <eeschema_id.h>
#include <class_library.h>
#include <sch_edit_frame.h>
#include <symbol_lib_table.h>
#include <reporter.h>
#include <lib_edit_frame.h>
#include <viewlib_frame.h>
#include <eeschema_config.h>
#include <sch_sheet.h>
#include <sim/sim_plot_frame.h>
#include <invoke_sch_dialog.h>
#include <dialogs/dialog_schematic_find.h>
#include <dialog_symbol_remap.h>
#include <tool/tool_manager.h>
#include <tool/tool_dispatcher.h>
#include <tool/action_toolbar.h>
#include <tool/common_control.h>
#include <tool/common_tools.h>
#include <tool/picker_tool.h>
#include <tool/zoom_tool.h>
#include <tools/ee_actions.h>
#include <tools/ee_selection_tool.h>
#include <tools/ee_point_editor.h>
#include <tools/sch_drawing_tools.h>
#include <tools/sch_line_wire_bus_tool.h>
#include <tools/sch_move_tool.h>
#include <tools/sch_edit_tool.h>
#include <tools/ee_inspection_tool.h>
#include <tools/sch_editor_control.h>
#include <wildcards_and_files_ext.h>
#include <connection_graph.h>
#include <sch_painter.h>

#include <gal/graphics_abstraction_layer.h>

SCH_SHEET_PATH* g_CurrentSheet = nullptr; // declared in general.h
CONNECTION_GRAPH* g_ConnectionGraph = nullptr;

// non-member so it can be moved easily, and kept REALLY private.
// Do NOT Clear() in here.
static void add_search_paths( SEARCH_STACK* aDst, const SEARCH_STACK& aSrc, int aIndex )
{
    for( unsigned i=0; i<aSrc.GetCount();  ++i )
        aDst->AddPaths( aSrc[i], aIndex );
}


// non-member so it can be moved easily, and kept REALLY private.
// Do NOT Clear() in here.
static void add_search_paths( SEARCH_STACK* aDst, wxConfigBase* aCfg, int aIndex )
{
    for( int i=1;  true;  ++i )
    {
        wxString key   = wxString::Format( wxT( "LibraryPath%d" ), i );
        wxString upath = aCfg->Read( key, wxEmptyString );

        if( !upath )
            break;

        aDst->AddPaths( upath, aIndex );
    }
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
        catch( const IO_ERROR& DBG( ioe ) )
        {
            DBG(printf( "%s: %s\n", __func__, TO_UTF8( ioe.What() ) );)
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

        // addLibrarySearchPaths( SEARCH_STACK* aSP, wxConfigBase* aCfg )
        // This is undocumented, but somebody wanted to store !schematic!
        // library search paths in the .kicad_common file?
        add_search_paths( ss, Pgm().CommonSettings(), -1 );
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

    EVT_CLOSE( SCH_EDIT_FRAME::OnCloseWindow )
    EVT_SIZE( SCH_EDIT_FRAME::OnSize )

    EVT_MENU_RANGE( ID_FILE1, ID_FILEMAX, SCH_EDIT_FRAME::OnLoadFile )

    EVT_MENU( ID_APPEND_PROJECT, SCH_EDIT_FRAME::OnAppendProject )
    EVT_MENU( ID_IMPORT_NON_KICAD_SCH, SCH_EDIT_FRAME::OnImportProject )

    EVT_MENU( wxID_EXIT, SCH_EDIT_FRAME::OnExit )
    EVT_MENU( wxID_CLOSE, SCH_EDIT_FRAME::OnExit )

    EVT_TOOL( ID_RESCUE_CACHED, SCH_EDIT_FRAME::OnRescueProject )
    EVT_MENU( ID_REMAP_SYMBOLS, SCH_EDIT_FRAME::OnRemapSymbols )

    EVT_MENU( ID_GRID_SETTINGS, SCH_BASE_FRAME::OnGridSettings )
END_EVENT_TABLE()


SCH_EDIT_FRAME::SCH_EDIT_FRAME( KIWAY* aKiway, wxWindow* aParent ):
    SCH_BASE_FRAME( aKiway, aParent, FRAME_SCH, wxT( "Eeschema" ),
        wxDefaultPosition, wxDefaultSize, KICAD_DEFAULT_DRAWFRAME_STYLE, SCH_EDIT_FRAME_NAME ),
    m_item_to_repeat( nullptr )
{
    g_CurrentSheet = new SCH_SHEET_PATH();
    g_ConnectionGraph = new CONNECTION_GRAPH( this );

    m_showBorderAndTitleBlock = true;   // true to show sheet references
    m_DefaultSchematicFileName = NAMELESS_PROJECT;
    m_DefaultSchematicFileName += wxT( ".sch" );
    m_showAllPins = false;
    m_printMonochrome = true;
    m_printSheetReference = true;
    SetShowPageLimits( true );
    m_undoItem = NULL;
    m_hasAutoSave = true;
    m_showIllegalSymbolLibDialog = true;
    m_FrameSize = ConvertDialogToPixels( wxSize( 500, 350 ) );    // default in case of no prefs
    m_AboutTitle = "Eeschema";

    m_findReplaceDialog = nullptr;
    m_findReplaceStatusPopup = nullptr;

    SetForceHVLines( true );
    SetSpiceAdjustPassiveValues( false );

    // Give an icon
    wxIcon icon;
    icon.CopyFromBitmap( KiBitmap( icon_eeschema_xpm ) );
    SetIcon( icon );

    // Initialize grid id to the default value (50 mils):
    m_LastGridSizeId = ID_POPUP_GRID_LEVEL_50 - ID_POPUP_GRID_LEVEL_1000;

    LoadSettings( config() );

    CreateScreens();

    SetSize( m_FramePos.x, m_FramePos.y, m_FrameSize.x, m_FrameSize.y );

    setupTools();
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
    m_auimgr.SetArtProvider( new EDA_DOCKART() );

    m_auimgr.AddPane( m_mainToolBar, EDA_PANE().HToolbar().Name( "MainToolbar" ).Top().Layer(6) );
    m_auimgr.AddPane( m_optionsToolBar, EDA_PANE().VToolbar().Name( "OptToolbar" ).Left().Layer(3) );
    m_auimgr.AddPane( m_drawToolBar, EDA_PANE().VToolbar().Name( "ToolsToolbar" ).Right().Layer(1) );
    m_auimgr.AddPane( GetCanvas(), EDA_PANE().Canvas().Name( "DrawFrame" ).Center() );
    m_auimgr.AddPane( m_messagePanel, EDA_PANE().Messages().Name( "MsgPanel" ).Bottom().Layer(6) );

    m_auimgr.Update();

    GetToolManager()->RunAction( ACTIONS::gridPreset, true, m_LastGridSizeId );
    GetToolManager()->RunAction( ACTIONS::zoomFitScreen, true );

    if( GetCanvas() )
        GetCanvas()->GetGAL()->SetGridVisibility( IsGridVisible() );

    InitExitKey();

    // Net list generator
    DefaultExecFlags();

    UpdateTitle();
}


SCH_EDIT_FRAME::~SCH_EDIT_FRAME()
{
    delete m_item_to_repeat;        // we own the cloned object, see this->SaveCopyForRepeatItem()

    SetScreen( NULL );

    delete g_CurrentSheet;          // a SCH_SHEET_PATH, on the heap.
    delete g_ConnectionGraph;
    delete m_undoItem;
    delete g_RootSheet;

    g_CurrentSheet = nullptr;
    g_ConnectionGraph = nullptr;
    g_RootSheet = NULL;
}


void SCH_EDIT_FRAME::setupTools()
{
    // Create the manager and dispatcher & route draw panel events to the dispatcher
    m_toolManager = new TOOL_MANAGER;
    m_toolManager->SetEnvironment( GetScreen(), GetCanvas()->GetView(),
                                   GetCanvas()->GetViewControls(), this );
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
    m_toolManager->InitTools();

    // Run the selection tool, it is supposed to be always active
    m_toolManager->RunAction( EE_ACTIONS::selectionActivate );

    GetCanvas()->SetEventDispatcher( m_toolDispatcher );
}


void SCH_EDIT_FRAME::SaveCopyForRepeatItem( SCH_ITEM* aItem )
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


void SCH_EDIT_FRAME::SetSheetNumberAndCount()
{
    SCH_SCREEN* screen;
    SCH_SCREENS s_list;

    /* Set the sheet count, and the sheet number (1 for root sheet)
     */
    int            sheet_count = g_RootSheet->CountSheets();
    int            SheetNumber = 1;
    wxString       current_sheetpath = g_CurrentSheet->Path();
    SCH_SHEET_LIST sheetList( g_RootSheet );

    // Examine all sheets path to find the current sheets path,
    // and count them from root to the current sheet path:
    for( const SCH_SHEET_PATH& sheet : sheetList )
    {
        wxString sheetpath = sheet.Path();

        if( sheetpath == current_sheetpath )   // Current sheet path found
            break;

        SheetNumber++;                         // Not found, increment before this current path
    }

    g_CurrentSheet->SetPageNumber( SheetNumber );

    for( screen = s_list.GetFirst(); screen != NULL; screen = s_list.GetNext() )
        screen->m_NumberOfScreens = sheet_count;

    GetScreen()->m_ScreenNumber = SheetNumber;
}


SCH_SCREEN* SCH_EDIT_FRAME::GetScreen() const
{
    if( !g_CurrentSheet )
        return nullptr;

    return g_CurrentSheet->LastScreen();
}


wxString SCH_EDIT_FRAME::GetScreenDesc() const
{
    if(! g_CurrentSheet )
        return wxT("<unknown>");

    wxString s = g_CurrentSheet->PathHumanReadable();

    return s;
}


void SCH_EDIT_FRAME::CreateScreens()
{
    if( g_RootSheet == NULL )
        g_RootSheet = new SCH_SHEET();

    if( g_RootSheet->GetScreen() == NULL )
    {
        SCH_SCREEN* screen = new SCH_SCREEN( &Kiway() );
        screen->SetMaxUndoItems( m_UndoRedoCountMax );
        g_RootSheet->SetScreen( screen );
        SetScreen( g_RootSheet->GetScreen() );
    }

    g_RootSheet->GetScreen()->SetFileName( m_DefaultSchematicFileName );

    g_CurrentSheet->clear();
    g_CurrentSheet->push_back( g_RootSheet );
    g_ConnectionGraph->Reset();

    if( GetScreen() == NULL )
    {
        SCH_SCREEN* screen = new SCH_SCREEN( &Kiway() );
        screen->SetMaxUndoItems( m_UndoRedoCountMax );
        SetScreen( screen );
    }

    GetScreen()->SetZoom( 32.0 );
}


SCH_SHEET_PATH& SCH_EDIT_FRAME::GetCurrentSheet()
{
    wxASSERT_MSG( g_CurrentSheet != NULL, wxT( "SCH_EDIT_FRAME g_CurrentSheet member is NULL." ) );

    return *g_CurrentSheet;
}


void SCH_EDIT_FRAME::SetCurrentSheet( const SCH_SHEET_PATH& aSheet )
{
    if( aSheet != *g_CurrentSheet )
    {
        *g_CurrentSheet = aSheet;
        GetCanvas()->DisplaySheet( g_CurrentSheet->LastScreen() );
    }
}


void SCH_EDIT_FRAME::HardRedraw()
{
    GetCanvas()->DisplaySheet( g_CurrentSheet->LastScreen() );
    GetCanvas()->ForceRefresh();
}


void SCH_EDIT_FRAME::SaveUndoItemInUndoList( SCH_ITEM* aItem, bool aAppend )
{
    wxCHECK_RET( aItem != NULL, wxT( "Cannot swap undo item structures." ) );
    wxCHECK_RET( m_undoItem != NULL, wxT( "Cannot swap undo item structures." ) );
    wxCHECK_RET( aItem->Type() == m_undoItem->Type(), wxT( "Cannot swap undo item structures." ) );

    aItem->SwapData( m_undoItem );
    SaveCopyInUndoList( aItem, UR_CHANGED, aAppend );
    aItem->SwapData( m_undoItem );
}


void SCH_EDIT_FRAME::OnCloseWindow( wxCloseEvent& aEvent )
{
    if( Kiface().IsSingle() )
    {
        LIB_EDIT_FRAME* libeditFrame = (LIB_EDIT_FRAME*) Kiway().Player( FRAME_SCH_LIB_EDITOR, false );
        if( libeditFrame && !libeditFrame->Close() )   // Can close component editor?
            return;

        LIB_VIEW_FRAME* viewlibFrame = (LIB_VIEW_FRAME*) Kiway().Player( FRAME_SCH_VIEWER, false );
        if( viewlibFrame && !viewlibFrame->Close() )   // Can close component viewer?
            return;

        viewlibFrame = (LIB_VIEW_FRAME*) Kiway().Player( FRAME_SCH_VIEWER_MODAL, false );

        if( viewlibFrame && !viewlibFrame->Close() )   // Can close modal component viewer?
            return;
    }

    SIM_PLOT_FRAME* simFrame = (SIM_PLOT_FRAME*) Kiway().Player( FRAME_SIMULATOR, false );

    if( simFrame && !simFrame->Close() )   // Can close the simulator?
        return;

    SCH_SHEET_LIST sheetList( g_RootSheet );

    if( sheetList.IsModified() )
    {
        wxFileName fileName = g_RootSheet->GetScreen()->GetFileName();
        wxString msg = _( "Save changes to \"%s\" before closing?" );

        if( !HandleUnsavedChanges( this, wxString::Format( msg, fileName.GetFullName() ),
                                   [&]()->bool { return SaveProject(); } ) )
        {
            aEvent.Veto();
            return;
        }
    }

    // Close the find dialog and preserve it's setting if it is displayed.
    if( m_findReplaceDialog )
    {
        m_findStringHistoryList = m_findReplaceDialog->GetFindEntries();
        m_replaceStringHistoryList = m_findReplaceDialog->GetReplaceEntries();

        m_findReplaceStatusPopup->Destroy();
        m_findReplaceStatusPopup = nullptr;

        // Must destroy statusPopup first as it holds a pointer to the dialog

        m_findReplaceDialog->Destroy();
        m_findReplaceDialog = nullptr;
    }

    SCH_SCREENS screens;
    wxFileName fn;

    for( SCH_SCREEN* screen = screens.GetFirst(); screen != NULL; screen = screens.GetNext() )
    {
        fn = Prj().AbsolutePath( screen->GetFileName() );

        // Auto save file name is the normal file name prepended with GetAutoSaveFilePrefix().
        fn.SetName( GetAutoSaveFilePrefix() + fn.GetName() );

        if( fn.FileExists() && fn.IsFileWritable() )
            wxRemoveFile( fn.GetFullPath() );
    }

    sheetList.ClearModifyStatus();

    wxString fileName = Prj().AbsolutePath( g_RootSheet->GetScreen()->GetFileName() );

    if( !g_RootSheet->GetScreen()->GetFileName().IsEmpty() &&
        g_RootSheet->GetScreen()->GetDrawItems() != NULL )
    {
        UpdateFileHistory( fileName );
    }

    g_RootSheet->GetScreen()->Clear();

    // all sub sheets are deleted, only the main sheet is usable
    g_CurrentSheet->clear();

    Destroy();
}


double SCH_EDIT_FRAME::BestZoom()
{
    double  sizeX = (double) GetScreen()->GetPageSettings().GetWidthIU();
    double  sizeY = (double) GetScreen()->GetPageSettings().GetHeightIU();
    wxPoint centre( wxPoint( sizeX / 2, sizeY / 2 ) );

    // The sheet boundary already affords us some margin, so add only an
    // additional 5%.
    double margin_scale_factor = 1.05;

    return bestZoom( sizeX, sizeY, margin_scale_factor, centre );
}


wxString SCH_EDIT_FRAME::GetUniqueFilenameForCurrentSheet()
{
    wxFileName fn = GetScreen()->GetFileName();

    // Name is <root sheet filename>-<sheet path> and has no extension.
    // However if filename is too long name is <sheet filename>-<sheet number>

    #define FN_LEN_MAX 80   // A reasonable value for the short filename len

    wxString filename = fn.GetName();
    wxString sheetFullName =  g_CurrentSheet->PathHumanReadable();

    if( sheetFullName == SCH_SHEET_PATH::GetRootPathName( false ) ||
        sheetFullName == SCH_SHEET_PATH::GetRootPathName( true ) )
    {
        // For the root sheet, use root schematic file name.
        sheetFullName.clear();
    }
    else
    {
        if( filename.Last() != '-' || filename.Last() != '_' )
            filename += '-';

        // Remove the first and last '/' of the path human readable
        sheetFullName.RemoveLast();
        sheetFullName.Remove( 0, 1 );
        sheetFullName.Trim( true );
        sheetFullName.Trim( false );

        // Convert path human readable separator to '-'
        sheetFullName.Replace( "/", "-" );
    }

    if( ( filename.Len() + sheetFullName.Len() ) < FN_LEN_MAX )
        filename += sheetFullName;
    else
        filename << wxT( "-" ) << GetScreen()->m_ScreenNumber;

    return filename;
}


void SCH_EDIT_FRAME::OnModify()
{
    GetScreen()->SetModify();
    GetScreen()->SetSave();

    if( ADVANCED_CFG::GetCfg().m_realTimeConnectivity && CONNECTION_GRAPH::m_allowRealTime )
        RecalculateConnections( false );

    GetCanvas()->Refresh();
}


void SCH_EDIT_FRAME::OnUpdatePCB( wxCommandEvent& event )
{
    wxFileName fn = Prj().AbsolutePath( g_RootSheet->GetScreen()->GetFileName() );

    fn.SetExt( PcbFileExtension );

    if( Kiface().IsSingle() )
    {
        DisplayError( this,  _( "Cannot update the PCB, because the Schematic Editor is opened"
                                " in stand-alone mode. In order to create/update PCBs from"
                                " schematics, launch the Kicad shell and create a project." ) );
        return;
    }

    KIWAY_PLAYER* frame = Kiway().Player( FRAME_PCB, true );

    // a pcb frame can be already existing, but not yet used.
    // this is the case when running the footprint editor, or the footprint viewer first
    // if the frame is not visible, the board is not yet loaded
    if( !frame->IsVisible() )
    {
        frame->OpenProjectFiles( std::vector<wxString>( 1, fn.GetFullPath() ) );
        frame->Show( true );
    }

    // On Windows, Raise() does not bring the window on screen, when iconized
    if( frame->IsIconized() )
        frame->Iconize( false );

    frame->Raise();

    std::string payload;
    Kiway().ExpressMail( FRAME_PCB, MAIL_PCB_UPDATE, payload, this );
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


void SCH_EDIT_FRAME::ShowFindReplaceDialog( bool aReplace )
{
    if( m_findReplaceStatusPopup )
        m_findReplaceStatusPopup->Destroy();

    // Must destroy statusPopup first as it holds a pointer to the dialog

    if( m_findReplaceDialog )
        m_findReplaceDialog->Destroy();

    m_findReplaceDialog= new DIALOG_SCH_FIND( this, m_findReplaceData, wxDefaultPosition,
                                              wxDefaultSize, aReplace ? wxFR_REPLACEDIALOG : 0 );

    m_findReplaceDialog->SetFindEntries( m_findStringHistoryList );
    m_findReplaceDialog->SetReplaceEntries( m_replaceStringHistoryList );
    m_findReplaceDialog->Show( true );

    m_findReplaceStatusPopup = new STATUS_TEXT_POPUP( m_findReplaceDialog );
    m_findReplaceStatusPopup->SetTextColor( wxColour( 255, 0, 0 ) );
}


void SCH_EDIT_FRAME::ShowFindReplaceStatus( const wxString& aMsg )
{
    wxPoint pos = wxGetMousePosition() - m_findReplaceStatusPopup->GetSize() - wxPoint( 10, 10 );

    m_findReplaceStatusPopup->SetText( aMsg );
    m_findReplaceStatusPopup->Move( pos );
    m_findReplaceStatusPopup->PopupFor( 3000 );
}


void SCH_EDIT_FRAME::ClearFindReplaceStatus()
{
    m_findReplaceStatusPopup->Hide();
}


void SCH_EDIT_FRAME::OnFindDialogClose()
{
    m_findStringHistoryList = m_findReplaceDialog->GetFindEntries();
    m_replaceStringHistoryList = m_findReplaceDialog->GetReplaceEntries();

    m_findReplaceStatusPopup->Destroy();
    m_findReplaceStatusPopup = nullptr;

    // Must destroy statusPopup first as it holds a pointer to the dialog

    m_findReplaceDialog->Destroy();
    m_findReplaceDialog = nullptr;
}


void SCH_EDIT_FRAME::OnLoadFile( wxCommandEvent& event )
{
    wxString fn = GetFileFromHistory( event.GetId(), _( "Schematic" ) );

    if( fn.size() )
        OpenProjectFiles( std::vector<wxString>( 1, fn ) );
}


void SCH_EDIT_FRAME::NewProject()
{
    wxString pro_dir = m_mruPath;

    wxFileDialog dlg( this, _( "New Schematic" ), pro_dir, wxEmptyString, SchematicFileWildcard(),
                      wxFD_SAVE );

    if( dlg.ShowModal() != wxID_CANCEL )
    {
        // Enforce the extension, wxFileDialog is inept.
        wxFileName create_me = dlg.GetPath();
        create_me.SetExt( SchematicFileExtension );

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

    wxFileDialog dlg( this, _( "Open Schematic" ), pro_dir, wxEmptyString, SchematicFileWildcard(),
                      wxFD_OPEN | wxFD_FILE_MUST_EXIST );

    if( dlg.ShowModal() != wxID_CANCEL )
    {
        OpenProjectFiles( std::vector<wxString>( 1, dlg.GetPath() ) );
        m_mruPath = Prj().GetProjectPath();
    }
}


void SCH_EDIT_FRAME::OnOpenPcbnew( wxCommandEvent& event )
{
    wxFileName kicad_board = Prj().AbsolutePath( g_RootSheet->GetScreen()->GetFileName() );

    if( kicad_board.IsOk() )
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
            KIWAY_PLAYER* frame = Kiway().Player( FRAME_PCB, true );

            // a pcb frame can be already existing, but not yet used.
            // this is the case when running the footprint editor, or the footprint viewer first
            // if the frame is not visible, the board is not yet loaded
             if( !frame->IsVisible() )
            {
                frame->OpenProjectFiles( std::vector<wxString>( 1, boardfn.GetFullPath() ) );
                frame->Show( true );
            }

            // On Windows, Raise() does not bring the window on screen, when iconized
            if( frame->IsIconized() )
                frame->Iconize( false );

            frame->Raise();
        }
    }
    else
    {
        ExecuteFile( this, PCBNEW_EXE );
    }
}


void SCH_EDIT_FRAME::OnOpenCvpcb( wxCommandEvent& event )
{
    wxFileName fn = Prj().AbsolutePath( g_RootSheet->GetScreen()->GetFileName() );
    fn.SetExt( NetlistFileExtension );

    if( !prepareForNetlist() )
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


void SCH_EDIT_FRAME::OnRescueProject( wxCommandEvent& event )
{
    SCH_SCREENS schematic;

    if( schematic.HasNoFullyDefinedLibIds() )
        RescueLegacyProject( true );
    else
        RescueSymbolLibTableProject( true );
}


void SCH_EDIT_FRAME::OnRemapSymbols( wxCommandEvent& event )
{
    DIALOG_SYMBOL_REMAP dlgRemap( this );

    dlgRemap.ShowQuasiModal();

    GetCanvas()->Refresh( true );
}


void SCH_EDIT_FRAME::OnExit( wxCommandEvent& event )
{
    if( event.GetId() == wxID_EXIT )
        Kiway().OnKiCadExit();

    if( event.GetId() == wxID_CLOSE || Kiface().IsSingle() )
        Close( false );
}


void SCH_EDIT_FRAME::Print()
{
    InvokeDialogPrintUsingPrinter( this );

    wxFileName fn = Prj().AbsolutePath( g_RootSheet->GetScreen()->GetFileName() );

    if( fn.GetName() != NAMELESS_PROJECT )
        Prj().ConfigSave( Kiface().KifaceSearch(), GROUP_SCH_EDIT, GetProjectFileParameters() );
}


void SCH_EDIT_FRAME::PrintPage( wxDC* aDC )
{
    wxString fileName = Prj().AbsolutePath( GetScreen()->GetFileName() );

    aDC->SetLogicalFunction( wxCOPY );
    GetScreen()->Print( aDC );
    PrintWorkSheet( aDC, GetScreen(), GetDefaultLineThickness(), IU_PER_MILS, fileName );
}


bool SCH_EDIT_FRAME::isAutoSaveRequired() const
{
    // In case this event happens before g_RootSheet is initialized which does happen
    // on mingw64 builds.

    if( g_RootSheet != NULL )
    {
        SCH_SCREENS screenList;

        for( SCH_SCREEN* screen = screenList.GetFirst(); screen; screen = screenList.GetNext() )
        {
            if( screen->IsSave() )
                return true;
        }
    }

    return false;
}


void SCH_EDIT_FRAME::AddItemToScreenAndUndoList( SCH_ITEM* aItem, bool aUndoAppend )
{
    SCH_SCREEN* screen = GetScreen();

    wxCHECK_RET( aItem != NULL, wxT( "Cannot add null item to list." ) );

    SCH_SHEET*     parentSheet = nullptr;
    SCH_COMPONENT* parentComponent = nullptr;
    SCH_ITEM*      undoItem = aItem;

    if( aItem->Type() == SCH_SHEET_PIN_T )
    {
        parentSheet = (SCH_SHEET*) aItem->GetParent();

        wxCHECK_RET( parentSheet && parentSheet->Type() == SCH_SHEET_T,
                     wxT( "Cannot place sheet pin in invalid schematic sheet object." ) );

        undoItem = parentSheet;
    }

    else if( aItem->Type() == SCH_FIELD_T )
    {
        parentComponent = (SCH_COMPONENT*) aItem->GetParent();

        wxCHECK_RET( parentComponent && parentComponent->Type() == SCH_COMPONENT_T,
                     wxT( "Cannot place field in invalid schematic component object." ) );

        undoItem = parentComponent;
    }

    if( aItem->IsNew() )
    {
        if( aItem->Type() == SCH_SHEET_PIN_T )
        {
            // Sheet pins are owned by their parent sheet.
            SaveCopyInUndoList( undoItem, UR_CHANGED, aUndoAppend );     // save the parent sheet

            parentSheet->AddPin( (SCH_SHEET_PIN*) aItem );
        }
        else if( aItem->Type() == SCH_FIELD_T )
        {
            // Component fields are also owned by their parent, but new component fields
            // are handled elsewhere.
            wxLogMessage( wxT( "addCurrentItemToScreen: unexpected new SCH_FIELD" ) );
        }
        else
        {
            if( !screen->CheckIfOnDrawList( aItem ) )  // don't want a loop!
                AddToScreen( aItem );

            SaveCopyForRepeatItem( aItem );
            SaveCopyInUndoList( undoItem, UR_NEW, aUndoAppend );
        }

        // Update connectivity info for new item
        if( !aItem->IsMoving() )
            RecalculateConnections();
    }
    else
    {
        SaveUndoItemInUndoList( undoItem, aUndoAppend );
    }

    aItem->ClearEditFlags();

    screen->SetModify();
    RefreshItem( aItem );

    if( !aItem->IsMoving() && aItem->IsConnectable() )
    {
        std::vector< wxPoint > pts;
        aItem->GetConnectionPoints( pts );

        for( auto i = pts.begin(); i != pts.end(); i++ )
        {
            for( auto j = i + 1; j != pts.end(); j++ )
                TrimWire( *i, *j );

            if( screen->IsJunctionNeeded( *i, true ) )
                AddJunction( *i, true, false );
        }

        TestDanglingEnds();
    }

    GetCanvas()->Refresh();
}


void SCH_EDIT_FRAME::UpdateTitle()
{
    wxString title;

    if( GetScreen()->GetFileName() == m_DefaultSchematicFileName )
    {
        title.Printf( _( "Eeschema" ) + wxT( " \u2014 %s" ),
                      GetScreen()->GetFileName() );
    }
    else
    {
        wxString    fileName = Prj().AbsolutePath( GetScreen()->GetFileName() );
        wxFileName  fn = fileName;

        title.Printf( _( "Eeschema" ) + wxT( " \u2014 %s [%s] \u2014 %s" ),
                      fn.GetFullName(),
                      g_CurrentSheet->PathHumanReadable(),
                      fn.GetPath() );

        if( fn.FileExists() )
        {
            if( !fn.IsFileWritable() )
                title += _( " [Read Only]" );
        }
        else
            title += _( " [no file]" );
    }

    SetTitle( title );
}


void SCH_EDIT_FRAME::RecalculateConnections( bool aDoCleanup )
{
    SCH_SHEET_LIST list( g_RootSheet );

    PROF_COUNTER timer;

    // Ensure schematic graph is accurate
    if( aDoCleanup )
    {
        for( const auto& sheet : list )
            SchematicCleanUp( sheet.LastScreen() );
    }

    timer.Stop();
    wxLogTrace( "CONN_PROFILE", "SchematicCleanUp() %0.4f ms", timer.msecs() );

    g_ConnectionGraph->Recalculate( list, true );
}


void SCH_EDIT_FRAME::CommonSettingsChanged( bool aEnvVarsChanged )
{
    SCH_BASE_FRAME::CommonSettingsChanged( aEnvVarsChanged );

    RecreateToolbars();
    Layout();
    SendSizeEvent();
}


void SCH_EDIT_FRAME::OnPageSettingsChange()
{
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


void SCH_EDIT_FRAME::SetScreen( BASE_SCREEN* aScreen )
{
    SCH_BASE_FRAME::SetScreen( aScreen );
    GetCanvas()->DisplaySheet( static_cast<SCH_SCREEN*>( aScreen ) );
}


const BOX2I SCH_EDIT_FRAME::GetDocumentExtents() const
{
    int sizeX = GetScreen()->GetPageSettings().GetWidthIU();
    int sizeY = GetScreen()->GetPageSettings().GetHeightIU();

    return BOX2I( VECTOR2I(0, 0), VECTOR2I( sizeX, sizeY ) );
}

void SCH_EDIT_FRAME::FixupJunctions()
{
    // Save the current sheet, to retrieve it later
    auto currSheet = GetCurrentSheet();

    SCH_SHEET_LIST sheetList;
    sheetList.BuildSheetList( g_RootSheet );

    for( const SCH_SHEET_PATH& sheet : sheetList )
    {
        std::vector<wxPoint> anchors;

        SetCurrentSheet( sheet );
        GetCurrentSheet().UpdateAllScreenReferences();

        auto screen = GetCurrentSheet().LastScreen();

        for( SCH_ITEM* item = screen->GetDrawItems(); item; item = item->Next() )
        {
            if( item->Type() == SCH_COMPONENT_T )
            {
                auto cmp = static_cast<SCH_COMPONENT*>( item );
                auto xform = cmp->GetTransform();

                for( const SCH_PIN& pin : cmp->GetPins() )
                {
                    auto pos = cmp->GetPosition() + xform.TransformCoordinate( pin.GetPosition() );

                    // Test if a _new_ junction is needed, and add it if missing
                    if ( screen->IsJunctionNeeded( pos, true ) )
                        AddJunction( pos );
                }
            }
        }
    }

    // Reselect the initial sheet:
    SetCurrentSheet( currSheet );
    GetCurrentSheet().UpdateAllScreenReferences();
    SetScreen( GetCurrentSheet().LastScreen() );
}
