/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2021 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file sch_edit_frame.cpp
 */

#include <fctsys.h>
#include <kiface_i.h>
#include <pgm_base.h>
#include <gr_basic.h>
#include <sch_draw_panel.h>
#include <gestfich.h>
#include <confirm.h>
#include <base_units.h>
#include <msgpanel.h>
#include <html_messagebox.h>
#include <executable_names.h>
#include <eda_dockart.h>

#include <general.h>
#include <eeschema_id.h>
#include <netlist.h>
#include <lib_pin.h>
#include <class_library.h>
#include <sch_edit_frame.h>
#include <sch_component.h>
#include <symbol_lib_table.h>

#include <dialog_helpers.h>
#include <reporter.h>
#include <lib_edit_frame.h>
#include <viewlib_frame.h>
#include <hotkeys.h>
#include <eeschema_config.h>
#include <sch_sheet.h>
#include <sch_sheet_path.h>
#include "sim/sim_plot_frame.h"

#include <invoke_sch_dialog.h>
#include <dialogs/dialog_schematic_find.h>
#include <dialog_symbol_remap.h>
#include <view/view.h>
#include <tool/tool_manager.h>

#include <wx/display.h>
#include <build_version.h>
#include <wildcards_and_files_ext.h>

#include <netlist_exporter_kicad.h>
#include <kiway.h>
#include <dialogs/dialog_fields_editor_global.h>

#include <sch_view.h>
#include <sch_painter.h>

#include <gal/graphics_abstraction_layer.h>

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

    EVT_MENU_RANGE( ID_FILE1, ID_FILEMAX, SCH_EDIT_FRAME::OnLoadFile )

    EVT_MENU( ID_APPEND_PROJECT, SCH_EDIT_FRAME::OnAppendProject )
    EVT_MENU( ID_IMPORT_NON_KICAD_SCH, SCH_EDIT_FRAME::OnImportProject )

    EVT_TOOL( ID_NEW_PROJECT, SCH_EDIT_FRAME::OnNewProject )
    EVT_TOOL( ID_LOAD_PROJECT, SCH_EDIT_FRAME::OnLoadProject )

    EVT_MENU( ID_SAVE_PROJECT, SCH_EDIT_FRAME::OnSaveProject )
    EVT_MENU( ID_UPDATE_ONE_SHEET, SCH_EDIT_FRAME::Save_File )
    EVT_MENU( ID_SAVE_ONE_SHEET_UNDER_NEW_NAME, SCH_EDIT_FRAME::Save_File )
    EVT_MENU( ID_GEN_PLOT_SCHEMATIC, SCH_EDIT_FRAME::PlotSchematic )
    EVT_MENU( ID_GEN_COPY_SHEET_TO_CLIPBOARD, EDA_DRAW_FRAME::CopyToClipboard )
    EVT_MENU( wxID_EXIT, SCH_EDIT_FRAME::OnExit )
    EVT_MENU( ID_POPUP_SCH_SELECT_ON_PCB, SCH_EDIT_FRAME::SelectAllFromSheet )

    EVT_MENU( ID_POPUP_SCH_DUPLICATE_ITEM, SCH_EDIT_FRAME::OnCopySchematicItemRequest )

    EVT_MENU( ID_CONFIG_SAVE, SCH_EDIT_FRAME::Process_Config )
    EVT_MENU( ID_CONFIG_READ, SCH_EDIT_FRAME::Process_Config )
    EVT_MENU( ID_PREFERENCES_HOTKEY_SHOW_CURRENT_LIST, SCH_EDIT_FRAME::Process_Config )

    EVT_TOOL( wxID_PREFERENCES, SCH_EDIT_FRAME::OnPreferencesOptions )
    EVT_MENU( ID_PREFERENCES_CONFIGURE_PATHS, SCH_EDIT_FRAME::OnConfigurePaths )

    EVT_TOOL( ID_RUN_LIBRARY, SCH_EDIT_FRAME::OnOpenLibraryEditor )
    EVT_TOOL( ID_POPUP_SCH_CALL_LIBEDIT_AND_LOAD_CMP, SCH_EDIT_FRAME::OnOpenLibraryEditor )
    EVT_TOOL( ID_TO_LIBVIEW, SCH_EDIT_FRAME::OnOpenLibraryViewer )
    EVT_TOOL( ID_RESCUE_CACHED, SCH_EDIT_FRAME::OnRescueProject )
    EVT_MENU( ID_REMAP_SYMBOLS, SCH_EDIT_FRAME::OnRemapSymbols )
    EVT_MENU( ID_EDIT_COMPONENTS_TO_SYMBOLS_LIB_ID, SCH_EDIT_FRAME::OnEditComponentSymbolsId )

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
    EVT_TOOL( ID_UPDATE_PCB_FROM_SCH, SCH_EDIT_FRAME::OnUpdatePCB )
    EVT_TOOL( ID_GET_TOOLS, SCH_EDIT_FRAME::OnCreateBillOfMaterials )
    EVT_TOOL( ID_OPEN_CMP_TABLE, SCH_EDIT_FRAME::OnLaunchBomManager )
    EVT_TOOL( ID_FIND_ITEMS, SCH_EDIT_FRAME::OnFindItems )
    EVT_TOOL( wxID_REPLACE, SCH_EDIT_FRAME::OnFindItems )
    EVT_TOOL( ID_BACKANNO_ITEMS, SCH_EDIT_FRAME::OnLoadCmpToFootprintLinkFile )
    EVT_TOOL( ID_UPDATE_FIELDS, SCH_EDIT_FRAME::OnUpdateFields )
    EVT_TOOL( ID_SCH_MOVE_ITEM, SCH_EDIT_FRAME::OnMoveItem )
    EVT_TOOL( ID_AUTOPLACE_FIELDS, SCH_EDIT_FRAME::OnAutoplaceFields )
    EVT_MENU( wxID_HELP, EDA_DRAW_FRAME::GetKicadHelp )
    EVT_MENU( wxID_INDEX, EDA_DRAW_FRAME::GetKicadHelp )
    EVT_MENU( ID_HELP_GET_INVOLVED, EDA_DRAW_FRAME::GetKicadContribute )
    EVT_MENU( wxID_ABOUT, EDA_BASE_FRAME::GetKicadAbout )
    EVT_MENU( ID_GRID_SETTINGS, SCH_BASE_FRAME::OnGridSettings )

    // Tools and buttons for vertical toolbar.
    EVT_TOOL( ID_NO_TOOL_SELECTED, SCH_EDIT_FRAME::OnSelectTool )
    EVT_TOOL( ID_HIGHLIGHT, SCH_EDIT_FRAME::OnSelectTool )
    EVT_MENU( ID_MENU_ZOOM_SELECTION, SCH_EDIT_FRAME::OnSelectTool )
    EVT_TOOL( ID_ZOOM_SELECTION, SCH_EDIT_FRAME::OnSelectTool )
    EVT_TOOL_RANGE( ID_SCHEMATIC_VERTICAL_TOOLBAR_START, ID_SCHEMATIC_VERTICAL_TOOLBAR_END,
                    SCH_EDIT_FRAME::OnSelectTool )
    EVT_TOOL_RANGE( ID_SCHEMATIC_PLACE_MENU_START, ID_SCHEMATIC_PLACE_MENU_END,
                    SCH_EDIT_FRAME::OnSelectTool )

#ifdef KICAD_SPICE
    EVT_TOOL( ID_SIM_SHOW, SCH_EDIT_FRAME::OnSimulate )
    EVT_TOOL( ID_SIM_PROBE, SCH_EDIT_FRAME::OnSelectTool )
    EVT_TOOL( ID_SIM_TUNE, SCH_EDIT_FRAME::OnSelectTool )
#endif /* KICAD_SPICE */

    EVT_MENU( ID_CANCEL_CURRENT_COMMAND, SCH_EDIT_FRAME::OnCancelCurrentCommand )
    EVT_MENU( ID_SCH_DRAG_ITEM, SCH_EDIT_FRAME::OnDragItem )
    EVT_MENU_RANGE( ID_SCH_ROTATE_CLOCKWISE, ID_SCH_ROTATE_COUNTERCLOCKWISE,
                    SCH_EDIT_FRAME::OnRotate )
    EVT_MENU_RANGE( ID_SCH_EDIT_ITEM, ID_SCH_EDIT_COMPONENT_FOOTPRINT,
                    SCH_EDIT_FRAME::OnEditItem )
    EVT_MENU_RANGE( ID_SCH_MIRROR_X, ID_SCH_ORIENT_NORMAL, SCH_EDIT_FRAME::OnOrient )
    EVT_MENU_RANGE( ID_POPUP_START_RANGE, ID_POPUP_END_RANGE,
                    SCH_EDIT_FRAME::Process_Special_Functions )
    EVT_MENU( ID_POPUP_SCH_DISPLAYDOC_CMP, SCH_EDIT_FRAME::OnEditItem )

    EVT_MENU( ID_MENU_CANVAS_CAIRO, SCH_EDIT_FRAME::OnSwitchCanvas )
    EVT_MENU( ID_MENU_CANVAS_OPENGL, SCH_EDIT_FRAME::OnSwitchCanvas )

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
    EVT_UPDATE_UI( wxID_PASTE, SCH_EDIT_FRAME::OnUpdatePaste )
    EVT_UPDATE_UI( ID_TB_OPTIONS_HIDDEN_PINS, SCH_EDIT_FRAME::OnUpdateHiddenPins )
    EVT_UPDATE_UI( ID_TB_OPTIONS_BUS_WIRES_ORIENT, SCH_EDIT_FRAME::OnUpdateBusOrientation )
    EVT_UPDATE_UI( ID_NO_TOOL_SELECTED, SCH_EDIT_FRAME::OnUpdateSelectTool )
    EVT_UPDATE_UI( ID_HIGHLIGHT, SCH_EDIT_FRAME::OnUpdateSelectTool )
    EVT_UPDATE_UI( ID_ZOOM_SELECTION, SCH_EDIT_FRAME::OnUpdateSelectTool )
    EVT_UPDATE_UI_RANGE( ID_SCHEMATIC_VERTICAL_TOOLBAR_START, ID_SCHEMATIC_VERTICAL_TOOLBAR_END,
                         SCH_EDIT_FRAME::OnUpdateSelectTool )
    EVT_UPDATE_UI( ID_SAVE_PROJECT, SCH_EDIT_FRAME::OnUpdateSave )
    EVT_UPDATE_UI( ID_UPDATE_ONE_SHEET, SCH_EDIT_FRAME::OnUpdateSaveSheet )
    EVT_UPDATE_UI( ID_POPUP_SCH_LEAVE_SHEET, SCH_EDIT_FRAME::OnUpdateHierarchySheet )
    EVT_UPDATE_UI( ID_REMAP_SYMBOLS, SCH_EDIT_FRAME::OnUpdateRemapSymbols )
    EVT_UPDATE_UI( ID_MENU_CANVAS_CAIRO, SCH_EDIT_FRAME::OnUpdateSwitchCanvas )
    EVT_UPDATE_UI( ID_MENU_CANVAS_OPENGL, SCH_EDIT_FRAME::OnUpdateSwitchCanvas )

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
    m_printMonochrome = true;
    m_printSheetReference = true;
    SetShowPageLimits( true );
    m_hotkeysDescrList = g_Schematic_Hotkeys_Descr;
    m_dlgFindReplace = NULL;
    m_findReplaceData = new wxFindReplaceData( wxFR_DOWN );
    m_findReplaceStatus = new wxString( wxEmptyString );
    m_undoItem = NULL;
    m_hasAutoSave = true;
    m_showIllegalSymbolLibDialog = true;
    m_showSheetFileNameCaseSensitivityDlg = true;
    m_FrameSize = ConvertDialogToPixels( wxSize( 500, 350 ) );    // default in case of no prefs
    m_AboutTitle = "Eeschema";

    m_toolManager = new TOOL_MANAGER;

    SetForceHVLines( true );
    SetSpiceAjustPassiveValues( false );

    // Give an icon
    wxIcon icon;
    icon.CopyFromBitmap( KiBitmap( icon_eeschema_xpm ) );
    SetIcon( icon );

    // Initialize grid id to the default value (50 mils):
    m_LastGridSizeId = ID_POPUP_GRID_LEVEL_50 - ID_POPUP_GRID_LEVEL_1000;

    LoadSettings( config() );

    CreateScreens();

    SetPresetGrid( m_LastGridSizeId );

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
    m_auimgr.SetArtProvider( new EDA_DOCKART( this ) );

    m_auimgr.AddPane( m_mainToolBar, EDA_PANE().HToolbar().Name( "MainToolbar" ).Top().Layer(6) );
    m_auimgr.AddPane( m_optionsToolBar, EDA_PANE().VToolbar().Name( "OptToolbar" ).Left().Layer(3) );
    m_auimgr.AddPane( m_drawToolBar, EDA_PANE().VToolbar().Name( "ToolsToolbar" ).Right().Layer(1) );
    m_auimgr.AddPane( m_canvas->GetWindow(), EDA_PANE().Canvas().Name( "DrawFrame" ).Center() );
    m_auimgr.AddPane( m_messagePanel, EDA_PANE().Messages().Name( "MsgPanel" ).Bottom().Layer(6) );

    m_auimgr.Update();

    Zoom_Automatique( false );

    if( GetGalCanvas() )
        GetGalCanvas()->GetGAL()->SetGridVisibility( IsGridVisible() );

    // Net list generator
    DefaultExecFlags();

    Bind( wxEVT_COMMAND_MENU_SELECTED, &SCH_EDIT_FRAME::OnEditSymbolLibTable, this,
          ID_EDIT_SYM_LIB_TABLE );

    UpdateTitle();
}


SCH_EDIT_FRAME::~SCH_EDIT_FRAME()
{
    Unbind( wxEVT_COMMAND_MENU_SELECTED, &SCH_EDIT_FRAME::OnEditSymbolLibTable, this,
            ID_EDIT_SYM_LIB_TABLE );

    delete m_item_to_repeat;        // we own the cloned object, see this->SetRepeatItem()

    SetScreen( NULL );

    delete m_CurrentSheet;          // a SCH_SHEET_PATH, on the heap.
    delete m_undoItem;
    delete m_findReplaceData;
    delete m_findReplaceStatus;

    delete g_RootSheet;
    g_RootSheet = NULL;
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
    SCH_SCREEN* screen;
    SCH_SCREENS s_list;

    /* Set the sheet count, and the sheet number (1 for root sheet)
     */
    int            sheet_count = g_RootSheet->CountSheets();
    int            SheetNumber = 1;
    wxString       current_sheetpath = m_CurrentSheet->Path();
    SCH_SHEET_LIST sheetList( g_RootSheet, false );

    // Examine all sheets path to find the current sheets path,
    // and count them from root to the current sheet path:
    for( unsigned i = 0; i < sheetList.size(); i++ )
    {
        wxString sheetpath = sheetList[i].Path();

        if( sheetpath == current_sheetpath )    // Current sheet path found
            break;

        SheetNumber++;                          /* Not found, increment sheet
                                                 * number before this current
                                                 * path */
    }

    m_CurrentSheet->SetPageNumber( SheetNumber );

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

    m_CurrentSheet->clear();
    m_CurrentSheet->push_back( g_RootSheet );

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
    if( aSheet != *m_CurrentSheet )
    {
        *m_CurrentSheet = aSheet;

        static_cast<SCH_DRAW_PANEL*>( m_canvas )->DisplaySheet( m_CurrentSheet->LastScreen() );
    }
}


void SCH_EDIT_FRAME::HardRedraw()
{
    static_cast<SCH_DRAW_PANEL*>( m_canvas )->DisplaySheet( m_CurrentSheet->LastScreen() );
    GetCanvas()->Refresh();
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
        LIB_EDIT_FRAME* libeditFrame = (LIB_EDIT_FRAME*) Kiway().Player( FRAME_SCH_LIB_EDITOR,
                                                                         false );
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

    SCH_SHEET_LIST sheetList( g_RootSheet, false );

    if( sheetList.IsModified() )
    {
        wxString fileName = Prj().AbsolutePath( g_RootSheet->GetScreen()->GetFileName() );
        wxString msg = _( "Save changes to\n\"%s\"\nbefore closing?" );

        if( !HandleUnsavedChanges( this, wxString::Format( msg, fileName ),
                                   [&]()->bool { return SaveProject(); } ) )
        {
            aEvent.Veto();
            return;
        }
    }

    // Close the find dialog and preserve it's setting if it is displayed.
    if( m_dlgFindReplace )
    {
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
    m_CurrentSheet->clear();

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
    wxString sheetFullName =  m_CurrentSheet->PathHumanReadable();

    if( sheetFullName == "<root sheet>" || sheetFullName == "/" )
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

    m_foundItems.SetForceSearch();

    m_canvas->Refresh();
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
    SCH_SCREENS screenList( g_RootSheet );

    aEvent.Enable( screenList.IsModified() );
}


void SCH_EDIT_FRAME::OnUpdateRemapSymbols( wxUpdateUIEvent& aEvent )
{
    SCH_SCREENS schematic;

    // The remapping can only be performed on legacy projects.
    aEvent.Enable( schematic.HasNoFullyDefinedLibIds() );
}


void SCH_EDIT_FRAME::OnUpdateSaveSheet( wxUpdateUIEvent& aEvent )
{
    aEvent.Enable( GetScreen()->IsModify() );

}


void SCH_EDIT_FRAME::OnUpdateHierarchySheet( wxUpdateUIEvent& aEvent )
{
    aEvent.Enable( m_CurrentSheet->Last() != g_RootSheet );
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


void SCH_EDIT_FRAME::CloseErc()
{
    // Find the ERC dialog if it's open and close it
    wxWindow* erc = FindWindowById( ID_DIALOG_ERC, this );

    if( erc )
        erc->Close( false );
}


void SCH_EDIT_FRAME::OnUpdatePCB( wxCommandEvent& event )
{
    doUpdatePcb( "" );
}


void SCH_EDIT_FRAME::doUpdatePcb( const wxString& aUpdateOptions )
{
    wxFileName fn = Prj().AbsolutePath( g_RootSheet->GetScreen()->GetFileName() );

    fn.SetExt( PcbFileExtension );

    if( Kiface().IsSingle() )
    {
        DisplayError( this,  _( "Cannot update the PCB, because the Schematic Editor is"
                                " opened in stand-alone mode. In order to create/update"
                                " PCBs from schematics, you need to launch Kicad shell"
                                " and create a PCB project." ) );
        return;
    }
    else
    {
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
    }

    if( aUpdateOptions.Contains( "quiet-annotate" ) )
    {
        SCH_SCREENS schematic;
        schematic.UpdateSymbolLinks();
        SCH_SHEET_LIST sheets( g_RootSheet );
        sheets.AnnotatePowerSymbols();
        AnnotateComponents( true, UNSORTED, INCREMENTAL_BY_REF, 0, false, false, true,
                            NULL_REPORTER::GetInstance() );
    }

    if( !aUpdateOptions.Contains( "no-annotate" ) )
    {
        // Ensure the schematic is OK for a netlist creation
        // (especially all components are annotated):
        if( !prepareForNetlist() )
            return;
    }

    NETLIST_OBJECT_LIST* net_atoms = BuildNetListBase();
    NETLIST_EXPORTER_KICAD exporter( this, net_atoms );
    STRING_FORMATTER formatter;

    exporter.Format( &formatter, GNL_ALL );

    auto updateOptions = aUpdateOptions.ToStdString();
    auto netlistString = formatter.GetString();
    auto finalNetlist = updateOptions + "\n" + netlistString;

    // Now, send the "kicad" (s-expr) netlist to Pcbnew
    Kiway().ExpressMail( FRAME_PCB, MAIL_SCH_PCB_UPDATE, finalNetlist, this );
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


void SCH_EDIT_FRAME::OnLaunchBomManager( wxCommandEvent& event )
{
    DIALOG_FIELDS_EDITOR_GLOBAL dlg( this );
    dlg.ShowQuasiModal();
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

    int style = 0;

    if( aEvent.GetId() == wxID_REPLACE )
        style = wxFR_REPLACEDIALOG;

    m_dlgFindReplace = new DIALOG_SCH_FIND( this, m_findReplaceData, m_findReplaceStatus,
                                            wxDefaultPosition, wxDefaultSize, style );

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


void SCH_EDIT_FRAME::OnUpdateFields( wxCommandEvent& event )
{
    std::list<SCH_COMPONENT*> components;

    for( SCH_ITEM* item = GetScreen()->GetDrawItems(); item; item = item->Next() )
    {
        if( item->Type() == SCH_COMPONENT_T )
            components.push_back( static_cast<SCH_COMPONENT*>( item ) );
    }

    if( InvokeDialogUpdateFields( this, components, true ) == wxID_OK )
        m_canvas->Refresh();
}


void SCH_EDIT_FRAME::OnNewProject( wxCommandEvent& event )
{
    wxString pro_dir = m_mruPath;

    wxFileDialog dlg( this, _( "New Schematic" ), pro_dir,
                      wxEmptyString, SchematicFileWildcard(),
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


void SCH_EDIT_FRAME::OnLoadProject( wxCommandEvent& event )
{
    wxString pro_dir = m_mruPath;

    wxFileDialog dlg( this, _( "Open Schematic" ), pro_dir,
                      wxEmptyString, SchematicFileWildcard(),
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

    if( !prepareForNetlist() )
        return;

    try
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
    catch( const IO_ERROR& )
    {
        DisplayError( this, _( "Could not open CvPcb" ) );
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
            wxMessageBox( _( "Error: not a symbol or no symbol." ) );
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
        LIB_ID id = component->GetLibId();
        LIB_ALIAS* entry = nullptr;

        try
        {
            entry = Prj().SchSymbolLibTable()->LoadSymbol( id );
        }
        catch( const IO_ERROR& ioe )
        {
            wxString msg;

            msg.Printf( _( "Error occurred loading symbol \"%s\" from library \"%s\"." ),
                        id.GetLibItemName().wx_str(), id.GetLibNickname().wx_str() );
            DisplayErrorMessage( this, msg, ioe.What() );
            return;
        }

        if( !entry )     // Should not occur
            return;

        libeditFrame->LoadComponentAndSelectLib( id, component->GetUnit(), component->GetConvert() );
    }

    SchematicCleanUp();
    m_canvas->Refresh();
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

    m_canvas->Refresh( true );
}


// This method is not the same as OnRemapSymbols.
// It allows renaming the lib id of groups of components when a symbol
// has moved from a library to another library.
// For instance to rename libname1::mysymbol to libname2::mysymbol
// or any other lib id name
void SCH_EDIT_FRAME::OnEditComponentSymbolsId( wxCommandEvent& event )
{
    InvokeDialogEditComponentsLibId( this );
    m_canvas->Refresh( true );
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
        Prj().ConfigSave( Kiface().KifaceSearch(), GROUP_SCH_EDIT, GetProjectFileParameters() );
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
        SCH_SCREENS screenList;

        for( SCH_SCREEN* screen = screenList.GetFirst(); screen; screen = screenList.GetNext() )
        {
            if( screen->IsSave() )
                return true;
        }
    }

    return false;
}


void SCH_EDIT_FRAME::addCurrentItemToScreen()
{
    SCH_SCREEN* screen = GetScreen();
    SCH_ITEM*   item = screen->GetCurItem();

    wxCHECK_RET( item != NULL, wxT( "Cannot add current item to list." ) );

    m_canvas->SetAutoPanRequest( false );

    SCH_SHEET*     parentSheet = nullptr;
    SCH_COMPONENT* parentComponent = nullptr;
    SCH_ITEM*      undoItem = item;

    if( item->Type() == SCH_SHEET_PIN_T )
    {
        parentSheet = (SCH_SHEET*) item->GetParent();

        wxCHECK_RET( parentSheet && parentSheet->Type() == SCH_SHEET_T,
                     wxT( "Cannot place sheet pin in invalid schematic sheet object." ) );

        undoItem = parentSheet;
    }
    else if( item->Type() == SCH_FIELD_T )
    {
        parentComponent = (SCH_COMPONENT*) item->GetParent();

        wxCHECK_RET( parentComponent && parentComponent->Type() == SCH_COMPONENT_T,
                     wxT( "Cannot place field in invalid schematic component object." ) );

        undoItem = parentComponent;
    }

    if( item->IsNew() )
    {
        // When a new sheet is added to the hierarchy, a clear annotation can be needed
        // for all new sheet paths added by the new sheet (if this sheet is loaded from
        // and existing sheet or a existing file, it can also contain subsheets)
        bool doClearAnnotation = false;
        SCH_SHEET_LIST initial_sheetpathList( g_RootSheet, false );

        if( item->Type() == SCH_SHEET_T )
        {
            // Fix the size and position of the new sheet using the last values set by
            // the m_mouseCaptureCallback function.
            m_canvas->SetMouseCapture( NULL, NULL );

            if( !EditSheet( (SCH_SHEET*)item, m_CurrentSheet, &doClearAnnotation ) )
            {
                screen->SetCurItem( NULL );
                delete item;

                return;
            }

            SetSheetNumberAndCount();

            if( !screen->CheckIfOnDrawList( item ) )  // don't want a loop!
                AddToScreen( item );

            SetRepeatItem( item );
            SaveCopyInUndoList( undoItem, UR_NEW );
        }
        else if( item->Type() == SCH_SHEET_PIN_T )
        {
            // Sheet pins are owned by their parent sheet.
            SaveCopyInUndoList( undoItem, UR_CHANGED );     // save the parent sheet

            parentSheet->AddPin( (SCH_SHEET_PIN*) item );
        }
        else if( item->Type() == SCH_FIELD_T )
        {
            // Component fields are also owned by their parent, but new component fields
            // are handled elsewhere.
            wxLogMessage( wxT( "addCurrentItemToScreen: unexpected new SCH_FIELD" ) );
        }
        else
        {
            if( !screen->CheckIfOnDrawList( item ) )  // don't want a loop!
                AddToScreen( item );

            SetRepeatItem( item );
            SaveCopyInUndoList( undoItem, UR_NEW );
        }

        if( doClearAnnotation )
        {
            // Clear annotation of new sheet paths: the new sheet and its sub-sheets
            // If needed the missing alternate references of components will be created
            SCH_SCREENS screensList( g_RootSheet );
            screensList.ClearAnnotationOfNewSheetPaths( initial_sheetpathList );
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

    RefreshItem( item );

    if( item->IsConnectable() )
    {
        std::vector< wxPoint > pts;
        item->GetConnectionPoints( pts );

        for( auto i = pts.begin(); i != pts.end(); i++ )
        {
            for( auto j = i + 1; j != pts.end(); j++ )
                TrimWire( *i, *j, true );

            if( screen->IsJunctionNeeded( *i, true ) )
                AddJunction( *i, true );
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
        title.Printf( _( "Eeschema" ) + wxT( " \u2014 %s" ), GetScreen()->GetFileName() );
    }
    else
    {
        wxString    fileName = Prj().AbsolutePath( GetScreen()->GetFileName() );
        wxFileName  fn = fileName;

        title.Printf( _( "Eeschema" ) + wxT( " \u2014 %s [%s] \u2014 %s" ),
                      fn.GetFullName(), m_CurrentSheet->PathHumanReadable(),
                      fn.GetPath() );

        if( fn.FileExists() )
        {
            if( !fn.IsFileWritable() )
                title += _( " [Read Only]" );
        }
        else
        {
            title += _( " [no file]" );
        }
    }

    SetTitle( title );
}


void SCH_EDIT_FRAME::CommonSettingsChanged()
{
    SCH_BASE_FRAME::CommonSettingsChanged();

    ReCreateHToolbar();
    ReCreateVToolbar();
    ReCreateOptToolbar();
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
    ReCreateHToolbar();
    ReCreateVToolbar();
    ReCreateOptToolbar();

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
    EDA_DRAW_FRAME::SetScreen( aScreen );
    auto c = static_cast<SCH_DRAW_PANEL*>(m_canvas);
    c->DisplaySheet( static_cast<SCH_SCREEN*>( aScreen ) );
}


const BOX2I SCH_EDIT_FRAME::GetDocumentExtents() const
{
    int sizeX = GetScreen()->GetPageSettings().GetWidthIU();
    int sizeY = GetScreen()->GetPageSettings().GetHeightIU();

    return BOX2I( VECTOR2I(0, 0), VECTOR2I( sizeX, sizeY ) );
}
