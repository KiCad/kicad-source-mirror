/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2013 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file schframe.cpp
 */

#include <fctsys.h>
#include <kiface_i.h>
#include <pgm_base.h>
#include <gr_basic.h>
#include <class_drawpanel.h>
#include <gestfich.h>
#include <confirm.h>
#include <base_units.h>
#include <msgpanel.h>
#include <html_messagebox.h>

#include <general.h>
#include <eeschema_id.h>
#include <netlist.h>
#include <lib_pin.h>
#include <class_library.h>
#include <schframe.h>
#include <sch_component.h>

#include <dialog_helpers.h>
#include <libeditframe.h>
#include <viewlib_frame.h>
#include <hotkeys.h>
#include <eeschema_config.h>
#include <sch_sheet.h>
#include <sch_sheet_path.h>

#include <invoke_sch_dialog.h>
#include <dialogs/dialog_schematic_find.h>

#include <wx/display.h>
#include <build_version.h>
#include <wildcards_and_files_ext.h>


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
        catch( const IO_ERROR& ioe )
        {
            DBG(printf( "%s: %s\n", __func__, TO_UTF8( ioe.errorText ) );)
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
    PART_LIBS* libs = (PART_LIBS*)  GetElem( PROJECT::ELEM_SCH_PART_LIBS );

    wxASSERT( !libs || dynamic_cast<PART_LIBS*>( libs ) );

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
            wxWindow*   parent = 0; // Pgm().App().GetTopWindow();

            // parent of this dialog cannot be NULL since that breaks the Kiway() chain.
            HTML_MESSAGE_BOX dlg( parent, _( "Not Found" ) );

            dlg.MessageSet( _( "The following libraries were not found:" ) );

            dlg.ListSet( lib_list );

            dlg.Layout();

            dlg.ShowModal();
        }
        catch( const IO_ERROR& ioe )
        {
            DisplayError( NULL, ioe.errorText );
        }
    }

    return libs;
}

/*
NETLIST_OBJECT_LIST* PROJECT::Netlist()
{
    NETLIST_OBJECT_LIST* netlist = (NETLIST_OBJECT_LIST*)  GetElem( PROJECT::ELEM_SCH_NETLIST );

    wxASSERT( !libs || dynamic_cast<NETLIST_OBJECT_LIST*>( netlist ) );

    if( !netlist )
    {
        netlist = new NETLIST_OBJECT_LIST();

        // Make PROJECT the new NETLIST_OBJECT_LIST owner.
        SetElem( PROJECT::ELEM_SCH_NETLIST, netlist );
    }
}
*/

//-----</SCH "data on demand" functions>------------------------------------------


BEGIN_EVENT_TABLE( SCH_EDIT_FRAME, EDA_DRAW_FRAME )
    EVT_SOCKET( ID_EDA_SOCKET_EVENT_SERV, EDA_DRAW_FRAME::OnSockRequestServer )
    EVT_SOCKET( ID_EDA_SOCKET_EVENT, EDA_DRAW_FRAME::OnSockRequest )

    EVT_CLOSE( SCH_EDIT_FRAME::OnCloseWindow )
    EVT_SIZE( SCH_EDIT_FRAME::OnSize )

    EVT_MENU( ID_NEW_PROJECT, SCH_EDIT_FRAME::OnNewProject )
    EVT_MENU( ID_LOAD_PROJECT, SCH_EDIT_FRAME::OnLoadProject )

    EVT_MENU_RANGE( wxID_FILE1, wxID_FILE9, SCH_EDIT_FRAME::OnLoadFile )

    EVT_MENU( ID_APPEND_PROJECT, SCH_EDIT_FRAME::OnAppendProject )

    EVT_TOOL( ID_NEW_PROJECT, SCH_EDIT_FRAME::OnNewProject )
    EVT_TOOL( ID_LOAD_PROJECT, SCH_EDIT_FRAME::OnLoadProject )

    EVT_MENU( ID_SAVE_PROJECT, SCH_EDIT_FRAME::OnSaveProject )
    EVT_MENU( ID_UPDATE_ONE_SHEET, SCH_EDIT_FRAME::Save_File )
    EVT_MENU( ID_SAVE_ONE_SHEET_UNDER_NEW_NAME, SCH_EDIT_FRAME::Save_File )
    EVT_MENU( ID_GEN_PLOT_SCHEMATIC, SCH_EDIT_FRAME::PlotSchematic )
    EVT_MENU( ID_GEN_COPY_SHEET_TO_CLIPBOARD, EDA_DRAW_FRAME::CopyToClipboard )
    EVT_MENU( wxID_EXIT, SCH_EDIT_FRAME::OnExit )

    EVT_MENU( ID_POPUP_SCH_COPY_ITEM, SCH_EDIT_FRAME::OnCopySchematicItemRequest )

    EVT_MENU( ID_CONFIG_REQ, SCH_EDIT_FRAME::InstallConfigFrame )
    EVT_MENU( ID_CONFIG_SAVE, SCH_EDIT_FRAME::Process_Config )
    EVT_MENU( ID_CONFIG_READ, SCH_EDIT_FRAME::Process_Config )
    EVT_MENU_RANGE( ID_PREFERENCES_HOTKEY_START, ID_PREFERENCES_HOTKEY_END,
                    SCH_EDIT_FRAME::Process_Config )

    EVT_MENU( ID_COLORS_SETUP, SCH_EDIT_FRAME::OnColorConfig )
    EVT_TOOL( wxID_PREFERENCES, SCH_EDIT_FRAME::OnPreferencesOptions )

    EVT_TOOL( ID_RUN_LIBRARY, SCH_EDIT_FRAME::OnOpenLibraryEditor )
    EVT_TOOL( ID_POPUP_SCH_CALL_LIBEDIT_AND_LOAD_CMP, SCH_EDIT_FRAME::OnOpenLibraryEditor )
    EVT_TOOL( ID_TO_LIBVIEW, SCH_EDIT_FRAME::OnOpenLibraryViewer )
    EVT_TOOL( ID_RESCUE_CACHED, SCH_EDIT_FRAME::OnRescueProject )

    EVT_TOOL( ID_RUN_PCB, SCH_EDIT_FRAME::OnOpenPcbnew )
    EVT_TOOL( ID_RUN_PCB_MODULE_EDITOR, SCH_EDIT_FRAME::OnOpenPcbModuleEditor )

    EVT_TOOL( ID_RUN_CVPCB, SCH_EDIT_FRAME::OnOpenCvpcb )

    EVT_TOOL( ID_SHEET_SET, EDA_DRAW_FRAME::Process_PageSettings )
    EVT_TOOL( ID_HIERARCHY, SCH_EDIT_FRAME::Process_Special_Functions )
    EVT_TOOL( wxID_CUT, SCH_EDIT_FRAME::Process_Special_Functions )
    EVT_TOOL( wxID_COPY, SCH_EDIT_FRAME::Process_Special_Functions )
    EVT_TOOL( wxID_PASTE, SCH_EDIT_FRAME::Process_Special_Functions )
    EVT_TOOL( wxID_UNDO, SCH_EDIT_FRAME::GetSchematicFromUndoList )
    EVT_TOOL( wxID_REDO, SCH_EDIT_FRAME::GetSchematicFromRedoList )
    EVT_TOOL( ID_GET_ANNOTATE, SCH_EDIT_FRAME::OnAnnotate )
    EVT_TOOL( wxID_PRINT, SCH_EDIT_FRAME::OnPrint )
    EVT_TOOL( ID_GET_ERC, SCH_EDIT_FRAME::OnErc )
    EVT_TOOL( ID_GET_NETLIST, SCH_EDIT_FRAME::OnCreateNetlist )
    EVT_TOOL( ID_GET_TOOLS, SCH_EDIT_FRAME::OnCreateBillOfMaterials )
    EVT_TOOL( ID_FIND_ITEMS, SCH_EDIT_FRAME::OnFindItems )
    EVT_TOOL( wxID_REPLACE, SCH_EDIT_FRAME::OnFindItems )
    EVT_TOOL( ID_BACKANNO_ITEMS, SCH_EDIT_FRAME::OnLoadCmpToFootprintLinkFile )
    EVT_TOOL( ID_SCH_MOVE_ITEM, SCH_EDIT_FRAME::OnMoveItem )
    EVT_MENU( wxID_HELP, EDA_DRAW_FRAME::GetKicadHelp )
    EVT_MENU( wxID_INDEX, EDA_DRAW_FRAME::GetKicadHelp )
    EVT_MENU( wxID_ABOUT, EDA_BASE_FRAME::GetKicadAbout )

    // Tools and buttons for vertical toolbar.
    EVT_TOOL( ID_NO_TOOL_SELECTED, SCH_EDIT_FRAME::OnSelectTool )
    EVT_TOOL_RANGE( ID_SCHEMATIC_VERTICAL_TOOLBAR_START, ID_SCHEMATIC_VERTICAL_TOOLBAR_END,
                    SCH_EDIT_FRAME::OnSelectTool )

    EVT_MENU( ID_CANCEL_CURRENT_COMMAND, SCH_EDIT_FRAME::OnCancelCurrentCommand )
    EVT_MENU( ID_SCH_DRAG_ITEM, SCH_EDIT_FRAME::OnDragItem )
    EVT_MENU_RANGE( ID_SCH_ROTATE_CLOCKWISE, ID_SCH_ROTATE_COUNTERCLOCKWISE,
                    SCH_EDIT_FRAME::OnRotate )
    EVT_MENU_RANGE( ID_SCH_EDIT_ITEM, ID_SCH_EDIT_COMPONENT_FOOTPRINT,
                    SCH_EDIT_FRAME::OnEditItem )
    EVT_MENU_RANGE( ID_SCH_MIRROR_X, ID_SCH_ORIENT_NORMAL, SCH_EDIT_FRAME::OnOrient )
    EVT_MENU_RANGE( ID_POPUP_START_RANGE, ID_POPUP_END_RANGE,
                    SCH_EDIT_FRAME::Process_Special_Functions )

    // Tools and buttons options toolbar
    EVT_TOOL( ID_TB_OPTIONS_HIDDEN_PINS, SCH_EDIT_FRAME::OnSelectOptionToolbar )
    EVT_TOOL( ID_TB_OPTIONS_BUS_WIRES_ORIENT, SCH_EDIT_FRAME::OnSelectOptionToolbar )

    EVT_MENU_RANGE( ID_POPUP_GENERAL_START_RANGE, ID_POPUP_GENERAL_END_RANGE,
                    SCH_EDIT_FRAME::Process_Special_Functions )
    EVT_MENU_RANGE( ID_POPUP_SCH_SELECT_UNIT1, ID_POPUP_SCH_SELECT_UNIT_CMP_MAX,
                    SCH_EDIT_FRAME::OnSelectUnit )
    EVT_MENU_RANGE( ID_POPUP_SCH_CHANGE_TYPE_TEXT, ID_POPUP_SCH_CHANGE_TYPE_TEXT_TO_COMMENT,
                    SCH_EDIT_FRAME::OnConvertTextType )

    // Multple item selection context menu commands.
    EVT_MENU_RANGE( ID_SELECT_ITEM_START, ID_SELECT_ITEM_END, SCH_EDIT_FRAME::OnSelectItem )

    /* Handle user interface update events. */
    EVT_UPDATE_UI( wxID_CUT, SCH_EDIT_FRAME::OnUpdateBlockSelected )
    EVT_UPDATE_UI( wxID_COPY, SCH_EDIT_FRAME::OnUpdateBlockSelected )
    EVT_UPDATE_UI( wxID_PASTE, SCH_EDIT_FRAME::OnUpdatePaste )
    EVT_UPDATE_UI( ID_TB_OPTIONS_HIDDEN_PINS, SCH_EDIT_FRAME::OnUpdateHiddenPins )
    EVT_UPDATE_UI( ID_TB_OPTIONS_BUS_WIRES_ORIENT, SCH_EDIT_FRAME::OnUpdateBusOrientation )
    EVT_UPDATE_UI( ID_NO_TOOL_SELECTED, SCH_EDIT_FRAME::OnUpdateSelectTool )
    EVT_UPDATE_UI_RANGE( ID_SCHEMATIC_VERTICAL_TOOLBAR_START, ID_SCHEMATIC_VERTICAL_TOOLBAR_END,
                         SCH_EDIT_FRAME::OnUpdateSelectTool )
    EVT_UPDATE_UI( ID_SAVE_PROJECT, SCH_EDIT_FRAME::OnUpdateSave )
    EVT_UPDATE_UI( ID_UPDATE_ONE_SHEET, SCH_EDIT_FRAME::OnUpdateSaveSheet )
    EVT_UPDATE_UI( ID_POPUP_SCH_LEAVE_SHEET, SCH_EDIT_FRAME::OnUpdateHierarchySheet )

    /* Search dialog events. */
    EVT_FIND_CLOSE( wxID_ANY, SCH_EDIT_FRAME::OnFindDialogClose )
    EVT_FIND_DRC_MARKER( wxID_ANY, SCH_EDIT_FRAME::OnFindDrcMarker )
    EVT_FIND( wxID_ANY, SCH_EDIT_FRAME::OnFindSchematicItem )
    EVT_FIND_REPLACE( wxID_ANY, SCH_EDIT_FRAME::OnFindReplace )
    EVT_FIND_REPLACE_ALL( wxID_ANY, SCH_EDIT_FRAME::OnFindReplace )

END_EVENT_TABLE()


SCH_EDIT_FRAME::SCH_EDIT_FRAME( KIWAY* aKiway, wxWindow* aParent ):
    SCH_BASE_FRAME( aKiway, aParent, FRAME_SCH, wxT( "Eeschema" ),
        wxDefaultPosition, wxDefaultSize, KICAD_DEFAULT_DRAWFRAME_STYLE, SCH_EDIT_FRAME_NAME ),
    m_item_to_repeat( 0 )
{
    m_showAxis = false;                 // true to show axis
    m_showBorderAndTitleBlock = true;   // true to show sheet references
    m_CurrentSheet = new SCH_SHEET_PATH;
    m_DefaultSchematicFileName = NAMELESS_PROJECT;
    m_DefaultSchematicFileName += wxT( ".sch" );
    m_showAllPins = false;
    m_previewPosition = wxDefaultPosition;
    m_previewSize = wxDefaultSize;
    m_printMonochrome = true;
    m_printSheetReference = true;
    SetShowPageLimits( true );
    m_hotkeysDescrList = g_Schematic_Hokeys_Descr;
    m_dlgFindReplace = NULL;
    m_findReplaceData = new wxFindReplaceData( wxFR_DOWN );
    m_undoItem = NULL;
    m_hasAutoSave = true;

    SetForceHVLines( true );
    SetSpiceAddReferencePrefix( false );
    SetSpiceUseNetcodeAsNetname( false );

    // Give an icon
    wxIcon icon;
    icon.CopyFromBitmap( KiBitmap( icon_eeschema_xpm ) );
    SetIcon( icon );

    // Initialize grid id to the default value (50 mils):
    const int default_grid = ID_POPUP_GRID_LEVEL_50 - ID_POPUP_GRID_LEVEL_1000;
    m_LastGridSizeId = default_grid;

    LoadSettings( config() );

    CreateScreens();

    // Ensure m_LastGridSizeId is an offset inside the allowed schematic grid range
    if( !GetScreen()->GridExists( m_LastGridSizeId + ID_POPUP_GRID_LEVEL_1000 ) )
        m_LastGridSizeId = default_grid;

    SetSize( m_FramePos.x, m_FramePos.y, m_FrameSize.x, m_FrameSize.y );

    if( m_canvas )
        m_canvas->SetEnableBlockCommands( true );

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

    EDA_PANEINFO horiz;
    horiz.HorizontalToolbarPane();

    EDA_PANEINFO vert;
    vert.VerticalToolbarPane();

    EDA_PANEINFO mesg;
    mesg.MessageToolbarPane();

    if( m_mainToolBar )
        m_auimgr.AddPane( m_mainToolBar,
                          wxAuiPaneInfo( horiz ).Name( wxT( "m_mainToolBar" ) ).Top().Row( 0 ) );

    if( m_drawToolBar )
        m_auimgr.AddPane( m_drawToolBar, wxAuiPaneInfo( vert ).Name( wxT( "m_drawToolBar" ) ).Right() );

    if( m_optionsToolBar )
        m_auimgr.AddPane( m_optionsToolBar,
                          wxAuiPaneInfo( vert ).Name( wxT( "m_optionsToolBar" ) ).Left() );

    if( m_canvas )
        m_auimgr.AddPane( m_canvas, wxAuiPaneInfo().Name( wxT( "DrawFrame" ) ).CentrePane() );

    if( m_messagePanel )
        m_auimgr.AddPane( m_messagePanel, wxAuiPaneInfo( mesg ).Name( wxT( "MsgPanel" ) ).Bottom().
                          Layer(10) );

    m_auimgr.Update();

    // Now Drawpanel is sized, we can use BestZoom to show the component (if any)
    GetScreen()->SetZoom( BestZoom() );

    Zoom_Automatique( false );
}


SCH_EDIT_FRAME::~SCH_EDIT_FRAME()
{
    delete m_item_to_repeat;        // we own the cloned object, see this->SetRepeatItem()

    SetScreen( NULL );

    delete m_CurrentSheet;          // a SCH_SHEET_PATH, on the heap.
    delete m_undoItem;
    delete g_RootSheet;
    delete m_findReplaceData;

    m_CurrentSheet = NULL;
    m_undoItem = NULL;
    g_RootSheet = NULL;
    m_findReplaceData = NULL;
}


void SCH_EDIT_FRAME::SetRepeatItem( SCH_ITEM* aItem )
{
    // we cannot store a pointer to an item in the display list here since
    // that item may be deleted, such as part of a line concatonation or other.
    // So simply always keep a copy of the object which is to be repeated.

    SCH_ITEM*   old = m_item_to_repeat;
    SCH_ITEM*   cur = aItem;

    if( cur != old )
    {
        if( cur )
        {
            aItem = (SCH_ITEM*) cur->Clone();

            // Clone() preserves the flags, we want 'em cleared.
            aItem->ClearFlags();
        }

        m_item_to_repeat = aItem;

        delete old;
    }
}


void SCH_EDIT_FRAME::SetSheetNumberAndCount()
{
    SCH_SCREEN* screen = GetScreen();
    SCH_SCREENS s_list;

    /* Set the sheet count, and the sheet number (1 for root sheet)
     */
    int            sheet_count = g_RootSheet->CountSheets();
    int            SheetNumber = 1;
    wxString       current_sheetpath = m_CurrentSheet->Path();
    SCH_SHEET_LIST sheetList;

    // Examine all sheets path to find the current sheets path,
    // and count them from root to the current sheet path:
    SCH_SHEET_PATH* sheet;

    for( sheet = sheetList.GetFirst(); sheet != NULL; sheet = sheetList.GetNext() )
    {
        wxString sheetpath = sheet->Path();

        if( sheetpath == current_sheetpath )    // Current sheet path found
            break;

        SheetNumber++;                          /* Not found, increment sheet
                                                 * number before this current
                                                 * path */
    }

    for( screen = s_list.GetFirst(); screen != NULL; screen = s_list.GetNext() )
    {
        screen->m_NumberOfScreens = sheet_count;
    }

    GetScreen()->m_ScreenNumber = SheetNumber;
}


SCH_SCREEN* SCH_EDIT_FRAME::GetScreen() const
{
    return m_CurrentSheet->LastScreen();
}


wxString SCH_EDIT_FRAME::GetScreenDesc() const
{
    wxString s = m_CurrentSheet->PathHumanReadable();

    return s;
}


void SCH_EDIT_FRAME::CreateScreens()
{
    if( g_RootSheet == NULL )
    {
        g_RootSheet = new SCH_SHEET();
    }

    if( g_RootSheet->GetScreen() == NULL )
    {
        SCH_SCREEN* screen = new SCH_SCREEN( &Kiway() );
        screen->SetMaxUndoItems( m_UndoRedoCountMax );
        g_RootSheet->SetScreen( screen );
        SetScreen( g_RootSheet->GetScreen() );
    }

    g_RootSheet->GetScreen()->SetFileName( m_DefaultSchematicFileName );

    m_CurrentSheet->Clear();
    m_CurrentSheet->Push( g_RootSheet );

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
    wxASSERT_MSG( m_CurrentSheet != NULL, wxT( "SCH_EDIT_FRAME m_CurrentSheet member is NULL." ) );

    return *m_CurrentSheet;
}


void SCH_EDIT_FRAME::SetCurrentSheet( const SCH_SHEET_PATH& aSheet )
{
    *m_CurrentSheet = aSheet;
}


void SCH_EDIT_FRAME::SetUndoItem( const SCH_ITEM* aItem )
{
    // if aItem != NULL, delete a previous m_undoItem, if exists
    // if aItme = NULL, just clear m_undoItem,
    // because when calling SetUndoItem( NULL ), we only clear m_undoItem,
    // because the owner of m_undoItem is no more me.
    if( aItem && m_undoItem )
    {
        delete m_undoItem;
    }

    m_undoItem = NULL;

    if( aItem )
        m_undoItem = (SCH_ITEM*) aItem->Clone();

}


void SCH_EDIT_FRAME::SaveUndoItemInUndoList( SCH_ITEM* aItem )
{
    wxCHECK_RET( aItem != NULL,
                 wxT( "Cannot swap undo item structures.  Bad programmer!." ) );
    wxCHECK_RET( m_undoItem != NULL,
                 wxT( "Cannot swap undo item structures.  Bad programmer!." ) );
    wxCHECK_RET( aItem->Type() == m_undoItem->Type(),
                 wxT( "Cannot swap undo item structures.  Bad programmer!." ) );

    aItem->SwapData( m_undoItem );
    SaveCopyInUndoList( aItem, UR_CHANGED );
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

    SCH_SHEET_LIST sheetList;

    if( sheetList.IsModified() )
    {
        wxString fileName = Prj().AbsolutePath( g_RootSheet->GetScreen()->GetFileName() );
        wxString msg = wxString::Format( _(
                "Save the changes in\n'%s'\nbefore closing?"),
                GetChars( fileName )
                );

        int ii = DisplayExitDialog( this, msg );

        switch( ii )
        {
        case wxID_CANCEL:
            aEvent.Veto();
            return;

        case wxID_NO:
            break;

        case wxID_YES:
            wxCommandEvent tmp( ID_SAVE_PROJECT );
            OnSaveProject( tmp );
            break;
        }
    }

    // Close the find dialog and preserve it's setting if it is displayed.
    if( m_dlgFindReplace )
    {
        m_findDialogPosition = m_dlgFindReplace->GetPosition();
        m_findDialogSize = m_dlgFindReplace->GetSize();
        m_findStringHistoryList = m_dlgFindReplace->GetFindEntries();
        m_replaceStringHistoryList = m_dlgFindReplace->GetReplaceEntries();
        m_dlgFindReplace->Destroy();
        m_dlgFindReplace = NULL;
    }

    SCH_SCREENS screens;
    wxFileName fn;

    for( SCH_SCREEN* screen = screens.GetFirst(); screen != NULL; screen = screens.GetNext() )
    {
        fn = Prj().AbsolutePath( screen->GetFileName() );

        // Auto save file name is the normal file name prepended with AUTOSAVE_PREFIX_FILENAME.
        fn.SetName( AUTOSAVE_PREFIX_FILENAME + fn.GetName() );

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
    m_CurrentSheet->Clear();

    Destroy();
}


double SCH_EDIT_FRAME::BestZoom()
{
    int    dx, dy;
    wxSize size;

    dx = GetScreen()->GetPageSettings().GetWidthIU();
    dy = GetScreen()->GetPageSettings().GetHeightIU();

    size = m_canvas->GetClientSize();

    // Reserve no margin because best zoom shows the full page
    // and margins are already included in function that draws the sheet refernces
    double margin_scale_factor = 1.0;
    double zx =(double) dx / ( margin_scale_factor * (double)size.x );
    double zy = (double) dy / ( margin_scale_factor * (double)size.y );

    double bestzoom = std::max( zx, zy );

    SetScrollCenterPosition( wxPoint( dx / 2, dy / 2 ) );

    return bestzoom;
}


wxString SCH_EDIT_FRAME::GetUniqueFilenameForCurrentSheet()
{
    wxFileName fn = GetScreen()->GetFileName();

    // Name is <root sheet filename>-<sheet path> and has no extension.
    // However if filename is too long name is <sheet filename>-<sheet number>

    #define FN_LEN_MAX 80   // A reasonable value for the short filename len

    wxString filename = fn.GetName();
    wxString sheetFullName =  m_CurrentSheet->PathHumanReadable();

    // Remove the last '/' of the path human readable
    // (and for the root sheet, make sheetFullName empty):
    sheetFullName.RemoveLast();

    sheetFullName.Trim( true );
    sheetFullName.Trim( false );

    // Convert path human readable separator to '-'
    sheetFullName.Replace( wxT( "/" ), wxT( "-" ) );

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

    m_foundItems.SetForceSearch();
}


void SCH_EDIT_FRAME::OnUpdateBlockSelected( wxUpdateUIEvent& event )
{
    bool enable = ( GetScreen() && GetScreen()->m_BlockLocate.GetCommand() == BLOCK_MOVE );

    event.Enable( enable );
}


void SCH_EDIT_FRAME::OnUpdatePaste( wxUpdateUIEvent& event )
{
    event.Enable( m_blockItems.GetCount() > 0 );
}


void SCH_EDIT_FRAME::OnUpdateBusOrientation( wxUpdateUIEvent& aEvent )
{
    wxString tool_tip = GetForceHVLines() ?
                        _( "Draw wires and buses in any direction" ) :
                        _( "Draw horizontal and vertical wires and buses only" );

    aEvent.Check( GetForceHVLines() );
    m_optionsToolBar->SetToolShortHelp( ID_TB_OPTIONS_BUS_WIRES_ORIENT, tool_tip );
}


void SCH_EDIT_FRAME::OnUpdateHiddenPins( wxUpdateUIEvent& aEvent )
{
    wxString tool_tip = m_showAllPins ? _( "Do not show hidden pins" ) :
                        _( "Show hidden pins" );

    aEvent.Check( m_showAllPins );
    m_optionsToolBar->SetToolShortHelp( ID_TB_OPTIONS_HIDDEN_PINS, tool_tip );
}


void SCH_EDIT_FRAME::OnUpdateSave( wxUpdateUIEvent& aEvent )
{
    SCH_SHEET_LIST sheetList;

    aEvent.Enable( sheetList.IsModified() );
}


void SCH_EDIT_FRAME::OnUpdateSaveSheet( wxUpdateUIEvent& aEvent )
{
    aEvent.Enable( GetScreen()->IsModify() );

}


void SCH_EDIT_FRAME::OnUpdateHierarchySheet( wxUpdateUIEvent& aEvent )
{
    aEvent.Enable( m_CurrentSheet->Last() != g_RootSheet );
}


void SCH_EDIT_FRAME::OnAnnotate( wxCommandEvent& event )
{
    InvokeDialogAnnotate( this );
}


void SCH_EDIT_FRAME::OnErc( wxCommandEvent& event )
{
    // See if it's already open...
    wxWindow* erc = FindWindowById( ID_DIALOG_ERC, this );

    if( erc )
        // Bring it to the top if already open.  Dual monitor users need this.
        erc->Raise();
    else
        InvokeDialogERC( this );
}


void SCH_EDIT_FRAME::OnCreateNetlist( wxCommandEvent& event )
{
    int result;

    do
    {
        result = InvokeDialogNetList( this );

        // If a plugin is removed or added, rebuild and reopen the new dialog

    } while( result == NET_PLUGIN_CHANGE );
}


void SCH_EDIT_FRAME::OnCreateBillOfMaterials( wxCommandEvent& )
{
    InvokeDialogCreateBOM( this );
}


void SCH_EDIT_FRAME::OnFindItems( wxCommandEvent& aEvent )
{
    wxCHECK_RET( m_findReplaceData != NULL,
                 wxT( "Forgot to create find/replace data.  Bad Programmer!" ) );

    if( m_dlgFindReplace )
    {
        delete m_dlgFindReplace;
        m_dlgFindReplace = NULL;
    }

    // Verify the find dialog is not drawn off the visible display area in case the
    // display configuration has changed since the last time the dialog position was
    // saved.
    wxRect displayRect = wxDisplay().GetGeometry();
    wxRect dialogRect = wxRect( m_findDialogPosition, m_findDialogSize );

    wxPoint position = m_findDialogPosition;

    if( !displayRect.Contains( dialogRect ) )
    {
        position = wxDefaultPosition;
    }

    int style = 0;

    if( aEvent.GetId() == wxID_REPLACE )
        style = wxFR_REPLACEDIALOG;

    m_dlgFindReplace = new DIALOG_SCH_FIND( this, m_findReplaceData, position, m_findDialogSize,
                                            style );

    m_dlgFindReplace->SetFindEntries( m_findStringHistoryList );
    m_dlgFindReplace->SetReplaceEntries( m_replaceStringHistoryList );
    m_dlgFindReplace->Show( true );
}


void SCH_EDIT_FRAME::OnFindDialogClose( wxFindDialogEvent& event )
{
    // If the user dismissed the dialog with the mouse, this will send the cursor back
    // to the last item found.
    OnFindSchematicItem( event );

    if( m_dlgFindReplace )
    {
        m_findDialogPosition = m_dlgFindReplace->GetPosition();
        m_findDialogSize = m_dlgFindReplace->GetSize();
        m_findStringHistoryList = m_dlgFindReplace->GetFindEntries();
        m_replaceStringHistoryList = m_dlgFindReplace->GetReplaceEntries();
        m_dlgFindReplace->Destroy();
        m_dlgFindReplace = NULL;
    }
}


void SCH_EDIT_FRAME::OnLoadFile( wxCommandEvent& event )
{
    wxString fn = GetFileFromHistory( event.GetId(), _( "Schematic" ) );

    if( fn.size() )
        OpenProjectFiles( std::vector<wxString>( 1, fn ) );
}


void SCH_EDIT_FRAME::OnLoadCmpToFootprintLinkFile( wxCommandEvent& event )
{
    LoadCmpToFootprintLinkFile();
    m_canvas->Refresh();
}


void SCH_EDIT_FRAME::OnNewProject( wxCommandEvent& event )
{
//  wxString pro_dir = wxPathOnly( Prj().GetProjectFullName() );
    wxString pro_dir = m_mruPath;

    wxFileDialog dlg( this, _( "New Schematic" ), pro_dir,
                      wxEmptyString, SchematicFileWildcard,
                      wxFD_SAVE );

    if( dlg.ShowModal() != wxID_CANCEL )
    {
        // Enforce the extension, wxFileDialog is inept.
        wxFileName create_me = dlg.GetPath();
        create_me.SetExt( SchematicFileExtension );

        if( create_me.FileExists() )
        {
            wxString msg = wxString::Format( _(
                    "Schematic file '%s' already exists, use Open instead" ),
                    GetChars( create_me.GetFullName() )
                    );
            DisplayError( this, msg );
            return ;
        }

        // OpenProjectFiles() requires absolute
        wxASSERT_MSG( create_me.IsAbsolute(), wxT( "wxFileDialog returned non-absolute" ) );

        OpenProjectFiles( std::vector<wxString>( 1, create_me.GetFullPath() ), KICTL_CREATE );
        m_mruPath = create_me.GetPath();
    }
}


void SCH_EDIT_FRAME::OnLoadProject( wxCommandEvent& event )
{
//  wxString pro_dir = wxPathOnly( Prj().GetProjectFullName() );
    wxString pro_dir = m_mruPath;

    wxFileDialog dlg( this, _( "Open Schematic" ), pro_dir,
                      wxEmptyString, SchematicFileWildcard,
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
        wxFileName& boardfn = ( !legacy_board.FileExists() || kicad_board.FileExists() ) ?
                                    kicad_board : legacy_board;

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


void SCH_EDIT_FRAME::OnOpenPcbModuleEditor( wxCommandEvent& event )
{
    wxFileName fn = Prj().AbsolutePath( g_RootSheet->GetScreen()->GetFileName() );

    if( fn.IsOk() )
    {
        KIWAY_PLAYER* fp_editor = Kiway().Player( FRAME_PCB_MODULE_EDITOR );

        // On Windows, Raise() does not bring the window on screen, when iconized
        if( fp_editor->IsIconized() )
            fp_editor->Iconize( false );

        fp_editor->Show( true );
        fp_editor->Raise();
    }
}


void SCH_EDIT_FRAME::OnOpenCvpcb( wxCommandEvent& event )
{
    wxFileName fn = Prj().AbsolutePath( g_RootSheet->GetScreen()->GetFileName() );

    fn.SetExt( NetlistFileExtension );

    if( prepareForNetlist() )
    {
        KIWAY_PLAYER* player = Kiway().Player( FRAME_CVPCB, false );  // test open already.

        if( !player )
        {
            player = Kiway().Player( FRAME_CVPCB, true );
            player->Show( true );
            // player->OpenProjectFiles( std::vector<wxString>( 1, fn.GetFullPath() ) );
        }

        sendNetlist();

        player->Raise();
    }
}


void SCH_EDIT_FRAME::OnOpenLibraryEditor( wxCommandEvent& event )
{
    SCH_COMPONENT* component = NULL;

    if( event.GetId() == ID_POPUP_SCH_CALL_LIBEDIT_AND_LOAD_CMP )
    {
        // We want to edit a component with Libedit.
        // we are here by a hot key, or by a popup menu
        SCH_ITEM* item = GetScreen()->GetCurItem();

        if( !item )
        {
            // If we didn't get here by a hot key, then something has gone wrong.
            if( event.GetInt() == 0 )
                return;

            EDA_HOTKEY_CLIENT_DATA* data = (EDA_HOTKEY_CLIENT_DATA*) event.GetClientObject();

            wxCHECK_RET( data != NULL, wxT( "Invalid hot key client object." ) );

            // Set the locat filter, according to the edit command
            const KICAD_T* filterList = SCH_COLLECTOR::ComponentsOnly;
            item = LocateAndShowItem( data->GetPosition(), filterList, event.GetInt() );

            // Exit if no item found at the current location or the item is already being edited.
            if( (item == NULL) || (item->GetFlags() != 0) )
                return;
        }


        if( !item || (item->GetFlags() != 0) || ( item->Type() != SCH_COMPONENT_T ) )
        {
            wxMessageBox( _( "Error: not a component or no component" ) );
            return;
        }

        component = (SCH_COMPONENT*) item;
    }

    LIB_EDIT_FRAME* libeditFrame = (LIB_EDIT_FRAME*) Kiway().Player( FRAME_SCH_LIB_EDITOR, false );

    if( !libeditFrame )
    {
        libeditFrame = (LIB_EDIT_FRAME*) Kiway().Player( FRAME_SCH_LIB_EDITOR, true );
        libeditFrame->Show( true );
    }

    libeditFrame->PushPreferences( m_canvas );

    // On Windows, Raise() does not bring the window on screen, when iconized
    if( libeditFrame->IsIconized() )
        libeditFrame->Iconize( false );

    libeditFrame->Raise();

    if( component )
    {
        if( PART_LIBS* libs = Prj().SchLibs() )
        {
            LIB_ALIAS* entry = libs->FindLibraryEntry( component->GetPartName() );

            if( !entry )     // Should not occur
                return;

            PART_LIB* library = entry->GetLib();

            libeditFrame->LoadComponentAndSelectLib( entry, library );
        }
    }
}


void SCH_EDIT_FRAME::OnRescueProject( wxCommandEvent& event )
{
    RescueProject( true );
}


void SCH_EDIT_FRAME::OnExit( wxCommandEvent& event )
{
    Close( false );
}


void SCH_EDIT_FRAME::OnPrint( wxCommandEvent& event )
{
    InvokeDialogPrintUsingPrinter( this );

    wxFileName fn = Prj().AbsolutePath( g_RootSheet->GetScreen()->GetFileName() );

    if( fn.GetName() != NAMELESS_PROJECT )
    {
        // was: wxGetApp().WriteProjectConfig( fn.GetFullPath(), GROUP, GetProjectFileParametersList() );
        Prj().ConfigSave( Kiface().KifaceSearch(), GROUP_SCH_EDITOR,
                          GetProjectFileParametersList() );
    }
}


void SCH_EDIT_FRAME::PrintPage( wxDC* aDC, LSET aPrintMask, bool aPrintMirrorMode,
                                void* aData )
{
    wxString fileName = Prj().AbsolutePath( GetScreen()->GetFileName() );

    GetScreen()->Draw( m_canvas, aDC, GR_DEFAULT_DRAWMODE );
    DrawWorkSheet( aDC, GetScreen(), GetDefaultLineThickness(), IU_PER_MILS, fileName );
}


void SCH_EDIT_FRAME::OnSelectItem( wxCommandEvent& aEvent )
{
    int id = aEvent.GetId();
    int index = id - ID_SELECT_ITEM_START;

    if( (id >= ID_SELECT_ITEM_START && id <= ID_SELECT_ITEM_END)
        && (index >= 0 && index < m_collectedItems.GetCount()) )
    {
        SCH_ITEM* item = m_collectedItems[index];
        m_canvas->SetAbortRequest( false );
        GetScreen()->SetCurItem( item );
    }
}


bool SCH_EDIT_FRAME::isAutoSaveRequired() const
{
    // In case this event happens before g_RootSheet is initialized which does happen
    // on mingw64 builds.

    if( g_RootSheet != NULL )
    {
        SCH_SHEET_LIST sheetList;

        return sheetList.IsAutoSaveRequired();
    }

    return false;
}


void SCH_EDIT_FRAME::addCurrentItemToList( bool aRedraw )
{
    SCH_SCREEN* screen = GetScreen();
    SCH_ITEM*   item = screen->GetCurItem();

    wxCHECK_RET( item != NULL, wxT( "Cannot add current item to list." ) );

    m_canvas->SetAutoPanRequest( false );

    SCH_ITEM* undoItem = item;

    if( item->Type() == SCH_SHEET_PIN_T )
    {
        SCH_SHEET* sheet = (SCH_SHEET*) item->GetParent();

        wxCHECK_RET( (sheet != NULL) && (sheet->Type() == SCH_SHEET_T),
                     wxT( "Cannot place sheet pin in invalid schematic sheet object." ) );

        undoItem = sheet;
    }

    else if( item->Type() == SCH_FIELD_T )
    {
        SCH_COMPONENT* cmp = (SCH_COMPONENT*) item->GetParent();

        wxCHECK_RET( (cmp != NULL) && (cmp->Type() == SCH_COMPONENT_T),
                     wxT( "Cannot place field in invalid schematic component object." ) );

        undoItem = cmp;
    }

    if( item->IsNew() )
    {
        if( item->Type() == SCH_SHEET_T )
        {
            // Fix the size and position of the new sheet using the last values set by
            // the m_mouseCaptureCallback function.
            m_canvas->SetMouseCapture( NULL, NULL );

            if( !EditSheet( (SCH_SHEET*)item, m_CurrentSheet ) )
            {
                screen->SetCurItem( NULL );
                delete item;

                if( aRedraw )
                    GetCanvas()->Refresh();

                return;
            }

            SetSheetNumberAndCount();
        }

        if( undoItem == item )
        {
            if( !screen->CheckIfOnDrawList( item ) )  // don't want a loop!
                screen->Append( item );

            SetRepeatItem( item );

            SaveCopyInUndoList( undoItem, UR_NEW );
        }
        else
        {
            // Here, item is not a basic schematic item, but an item inside
            // a parent basic schematic item,
            // currently: sheet pin or component field.
            // currently, only a sheet pin can be found as new item,
            // because new component fields have a specific handling, and do not appears here
            SaveCopyInUndoList( undoItem, UR_CHANGED );

            if( item->Type() == SCH_SHEET_PIN_T )
                ( (SCH_SHEET*)undoItem )->AddPin( (SCH_SHEET_PIN*) item );
            else
                wxLogMessage( wxT( "addCurrentItemToList: expected type = SCH_SHEET_PIN_T, actual type = %d" ),
                              item->Type() );
        }
    }
    else
    {
        SaveUndoItemInUndoList( undoItem );
    }

    item->ClearFlags();
    screen->SetModify();
    screen->SetCurItem( NULL );
    m_canvas->SetMouseCapture( NULL, NULL );
    m_canvas->EndMouseCapture();

    if( item->IsConnectable() )
        screen->TestDanglingEnds();

    if( aRedraw )
        GetCanvas()->Refresh();
}


void SCH_EDIT_FRAME::UpdateTitle()
{
    wxString title;

    if( GetScreen()->GetFileName() == m_DefaultSchematicFileName )
    {
        title.Printf( wxT( "Eeschema %s [%s]" ), GetChars( GetBuildVersion() ),
                            GetChars( GetScreen()->GetFileName() ) );
    }
    else
    {
        wxString    fileName = Prj().AbsolutePath( GetScreen()->GetFileName() );
        wxFileName  fn = fileName;

        title.Printf( wxT( "[ %s %s] (%s)" ),
                      GetChars( fn.GetName() ),
                      GetChars( m_CurrentSheet->PathHumanReadable() ),
                      GetChars( fn.GetPath() ) );

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

