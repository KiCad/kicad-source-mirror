/******************/
/*  schframe.cpp  */
/******************/

#ifdef __GNUG__
#pragma implementation
#endif

#include "fctsys.h"
#include "appl_wxstruct.h"
#include "common.h"
#include "class_drawpanel.h"
#include "gestfich.h"
#include "bitmaps.h"

#include "general.h"
#include "protos.h"
#include "eeschema_id.h"
#include "netlist.h"
#include "lib_pin.h"
#include "class_library.h"
#include "wxEeschemaStruct.h"
#include "class_sch_screen.h"

#include "dialog_helpers.h"
#include "netlist_control.h"
#include "libeditframe.h"
#include "viewlib_frame.h"
#include "hotkeys.h"
#include "eeschema_config.h"
#include "sch_sheet.h"

#include "dialogs/annotate_dialog.h"
#include "dialogs/dialog_build_BOM.h"
#include "dialogs/dialog_erc.h"
#include "dialogs/dialog_print_using_printer.h"
#include "dialogs/dialog_schematic_find.h"
#include "dialogs/dialog_SVG_print.h"


BEGIN_EVENT_TABLE( SCH_EDIT_FRAME, EDA_DRAW_FRAME )
    EVT_SOCKET( ID_EDA_SOCKET_EVENT_SERV, EDA_DRAW_FRAME::OnSockRequestServer )
    EVT_SOCKET( ID_EDA_SOCKET_EVENT, EDA_DRAW_FRAME::OnSockRequest )

    EVT_CLOSE( SCH_EDIT_FRAME::OnCloseWindow )
    EVT_SIZE( SCH_EDIT_FRAME::OnSize )

    EVT_MENU( ID_NEW_PROJECT, SCH_EDIT_FRAME::OnNewProject )
    EVT_MENU( ID_LOAD_PROJECT, SCH_EDIT_FRAME::OnLoadProject )

    EVT_MENU_RANGE( wxID_FILE1, wxID_FILE9, SCH_EDIT_FRAME::OnLoadFile )

    EVT_TOOL( ID_NEW_PROJECT, SCH_EDIT_FRAME::OnNewProject )
    EVT_TOOL( ID_LOAD_PROJECT, SCH_EDIT_FRAME::OnLoadProject )

    EVT_MENU( ID_SAVE_PROJECT, SCH_EDIT_FRAME::Save_File )
    EVT_MENU( ID_SAVE_ONE_SHEET, SCH_EDIT_FRAME::Save_File )
    EVT_MENU( ID_SAVE_ONE_SHEET_AS, SCH_EDIT_FRAME::Save_File )
    EVT_TOOL( ID_SAVE_PROJECT, SCH_EDIT_FRAME::Save_File )
    EVT_MENU( wxID_PRINT, SCH_EDIT_FRAME::OnPrint )
    EVT_MENU( ID_GEN_PLOT_PS, SCH_EDIT_FRAME::ToPlot_PS )
    EVT_MENU( ID_GEN_PLOT_HPGL, SCH_EDIT_FRAME::ToPlot_HPGL )
    EVT_MENU( ID_GEN_PLOT_SVG, SCH_EDIT_FRAME::SVG_Print )
    EVT_MENU( ID_GEN_PLOT_DXF, SCH_EDIT_FRAME::ToPlot_DXF )
    EVT_MENU( ID_GEN_COPY_SHEET_TO_CLIPBOARD, EDA_DRAW_FRAME::CopyToClipboard )
    EVT_MENU( ID_GEN_COPY_BLOCK_TO_CLIPBOARD, EDA_DRAW_FRAME::CopyToClipboard )
    EVT_MENU( wxID_EXIT, SCH_EDIT_FRAME::OnExit )

    EVT_MENU( ID_POPUP_SCH_COPY_ITEM, SCH_EDIT_FRAME::OnCopySchematicItemRequest )

    EVT_MENU( ID_CONFIG_REQ, SCH_EDIT_FRAME::InstallConfigFrame )
    EVT_MENU( ID_CONFIG_SAVE, SCH_EDIT_FRAME::Process_Config )
    EVT_MENU( ID_CONFIG_READ, SCH_EDIT_FRAME::Process_Config )
    EVT_MENU_RANGE( ID_PREFERENCES_HOTKEY_START,
                    ID_PREFERENCES_HOTKEY_END,
                    SCH_EDIT_FRAME::Process_Config )

    EVT_MENU( ID_COLORS_SETUP, SCH_EDIT_FRAME::OnColorConfig )
    EVT_TOOL( ID_OPTIONS_SETUP, SCH_EDIT_FRAME::OnSetOptions )

    EVT_MENU_RANGE( ID_LANGUAGE_CHOICE, ID_LANGUAGE_CHOICE_END, SCH_EDIT_FRAME::SetLanguage )

    EVT_TOOL_RANGE( ID_ZOOM_IN, ID_ZOOM_PAGE, SCH_EDIT_FRAME::OnZoom )

    EVT_TOOL( ID_TO_LIBRARY, SCH_EDIT_FRAME::OnOpenLibraryEditor )
    EVT_TOOL( ID_TO_LIBVIEW, SCH_EDIT_FRAME::OnOpenLibraryViewer )

    EVT_TOOL( ID_TO_PCB, SCH_EDIT_FRAME::OnOpenPcbnew )
    EVT_TOOL( ID_TO_CVPCB, SCH_EDIT_FRAME::OnOpenCvpcb )

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
    EVT_TOOL( ID_BACKANNO_ITEMS, SCH_EDIT_FRAME::OnLoadStuffFile )
    EVT_TOOL( ID_COMPONENT_BUTT, SCH_EDIT_FRAME::Process_Special_Functions )

    EVT_MENU( ID_GENERAL_HELP, EDA_DRAW_FRAME::GetKicadHelp )
    EVT_MENU( ID_KICAD_ABOUT, EDA_DRAW_FRAME::GetKicadAbout )

    // Tools and buttons for vertical toolbar.
    EVT_TOOL( ID_NO_SELECT_BUTT, SCH_EDIT_FRAME::Process_Special_Functions )
    EVT_TOOL_RANGE( ID_SCHEMATIC_VERTICAL_TOOLBAR_START,
                    ID_SCHEMATIC_VERTICAL_TOOLBAR_END,
                    SCH_EDIT_FRAME::Process_Special_Functions )

    EVT_MENU_RANGE( ID_POPUP_START_RANGE, ID_POPUP_END_RANGE,
                    SCH_EDIT_FRAME::Process_Special_Functions )

    // Tools and buttons options toolbar
    EVT_TOOL_RANGE( ID_TB_OPTIONS_START, ID_TB_OPTIONS_END,
                    SCH_EDIT_FRAME::OnSelectOptionToolbar )

    EVT_MENU_RANGE( ID_POPUP_GENERAL_START_RANGE, ID_POPUP_GENERAL_END_RANGE,
                    SCH_EDIT_FRAME::Process_Special_Functions )

    /* Handle user interface update events. */
    EVT_UPDATE_UI( wxID_CUT, SCH_EDIT_FRAME::OnUpdateBlockSelected )
    EVT_UPDATE_UI( wxID_COPY, SCH_EDIT_FRAME::OnUpdateBlockSelected )
    EVT_UPDATE_UI( wxID_PASTE, SCH_EDIT_FRAME::OnUpdatePaste )
    EVT_UPDATE_UI( wxID_UNDO, SCH_EDIT_FRAME::OnUpdateSchematicUndo )
    EVT_UPDATE_UI( wxID_REDO, SCH_EDIT_FRAME::OnUpdateSchematicRedo )
    EVT_UPDATE_UI( ID_TB_OPTIONS_SHOW_GRID, SCH_EDIT_FRAME::OnUpdateGrid )
    EVT_UPDATE_UI( ID_TB_OPTIONS_SELECT_CURSOR, SCH_EDIT_FRAME::OnUpdateSelectCursor )
    EVT_UPDATE_UI( ID_TB_OPTIONS_HIDDEN_PINS, SCH_EDIT_FRAME::OnUpdateHiddenPins )
    EVT_UPDATE_UI( ID_TB_OPTIONS_BUS_WIRES_ORIENT, SCH_EDIT_FRAME::OnUpdateBusOrientation )
    EVT_UPDATE_UI_RANGE( ID_TB_OPTIONS_SELECT_UNIT_MM,
                         ID_TB_OPTIONS_SELECT_UNIT_INCH,
                         SCH_EDIT_FRAME::OnUpdateUnits )

    /* Search dialog events. */
    EVT_FIND_CLOSE( wxID_ANY, SCH_EDIT_FRAME::OnFindDialogClose )
    EVT_FIND_DRC_MARKER( wxID_ANY, SCH_EDIT_FRAME::OnFindDrcMarker )
    EVT_FIND( wxID_ANY, SCH_EDIT_FRAME::OnFindSchematicItem )

END_EVENT_TABLE()


SCH_EDIT_FRAME::SCH_EDIT_FRAME( wxWindow*       father,
                                const wxString& title,
                                const wxPoint&  pos,
                                const wxSize&   size,
                                long            style ) :
    EDA_DRAW_FRAME( father, SCHEMATIC_FRAME, title, pos, size, style )
{
    m_FrameName = wxT( "SchematicFrame" );
    m_Draw_Axis = FALSE;                // TRUE to show axis
    m_Draw_Sheet_Ref = TRUE;            // TRUE to show sheet references
    m_CurrentSheet   = new SCH_SHEET_PATH();
    m_CurrentField   = NULL;
    m_Multiflag     = 0;
    m_TextFieldSize = DEFAULT_SIZE_TEXT;
    m_LibeditFrame  = NULL;         // Component editor frame.
    m_ViewlibFrame  = NULL;         // Frame for browsing component libraries
    m_DefaultSchematicFileName = NAMELESS_PROJECT;
    m_DefaultSchematicFileName += wxT( ".sch" );
    m_ShowAllPins = false;
    m_previewPosition = wxDefaultPosition;
    m_previewSize = wxDefaultSize;
    m_printMonochrome = true;
    m_printSheetReference = true;
    m_HotkeysZoomAndGridList = s_Schematic_Hokeys_Descr;
    m_dlgFindReplace = NULL;
    m_findReplaceData = new wxFindReplaceData( wxFR_DOWN );

    CreateScreens();

    // Give an icon
#ifdef __WINDOWS__
    SetIcon( wxICON( a_icon_eeschema ) );
#else
    SetIcon( wxICON( icon_eeschema ) );
#endif

    m_itemToRepeat = NULL;

    /* Get config */
    LoadSettings();

    // Internalize grid id to a default value if not found in config or bad:
    if( (m_LastGridSizeId <= 0)
       || ( m_LastGridSizeId < (ID_POPUP_GRID_USER - ID_POPUP_GRID_LEVEL_1000) ) )
        m_LastGridSizeId = ID_POPUP_GRID_LEVEL_50 - ID_POPUP_GRID_LEVEL_1000;

    SetSize( m_FramePos.x, m_FramePos.y, m_FrameSize.x, m_FrameSize.y );

    if( DrawPanel )
        DrawPanel->m_Block_Enable = TRUE;

    ReCreateMenuBar();
    ReCreateHToolbar();
    ReCreateVToolbar();
    ReCreateOptToolbar();

    /* Initialize print and page setup dialog settings. */
    m_pageSetupData.GetPrintData().SetQuality( wxPRINT_QUALITY_HIGH );
    m_pageSetupData.GetPrintData().SetOrientation( wxLANDSCAPE );

    m_auimgr.SetManagedWindow( this );

    wxAuiPaneInfo horiz;
    horiz.Gripper( false );
    horiz.DockFixed( true );
    horiz.Movable( false );
    horiz.Floatable( false );
    horiz.CloseButton( false );
    horiz.CaptionVisible( false );

    wxAuiPaneInfo vert( horiz );

    vert.TopDockable( false ).BottomDockable( false );
    horiz.LeftDockable( false ).RightDockable( false );

    if( m_HToolBar )
        m_auimgr.AddPane( m_HToolBar,
                          wxAuiPaneInfo( horiz ).Name( wxT( "m_HToolBar" ) ).Top().Row( 0 ) );

    if( m_VToolBar )
        m_auimgr.AddPane( m_VToolBar, wxAuiPaneInfo( vert ).Name( wxT( "m_VToolBar" ) ).Right() );

    if( m_OptionsToolBar )
        m_auimgr.AddPane( m_OptionsToolBar,
                          wxAuiPaneInfo( vert ).Name( wxT( "m_OptionsToolBar" ) ).Left() );

    if( DrawPanel )
        m_auimgr.AddPane( DrawPanel, wxAuiPaneInfo().Name( wxT( "DrawFrame" ) ).CentrePane() );

    if( MsgPanel )
        m_auimgr.AddPane( MsgPanel, wxAuiPaneInfo( horiz ).Name( wxT( "MsgPanel" ) ).Bottom() );

    m_auimgr.Update();
}


SCH_EDIT_FRAME::~SCH_EDIT_FRAME()
{
    SAFE_DELETE( g_RootSheet );
    SAFE_DELETE( m_CurrentSheet );     // a SCH_SHEET_PATH, on the heap.
    SAFE_DELETE( m_findReplaceData );
    CMP_LIBRARY::RemoveAllLibraries();
}


BASE_SCREEN* SCH_EDIT_FRAME::GetBaseScreen() const
{
    return GetScreen();
}


SCH_SHEET_PATH* SCH_EDIT_FRAME::GetSheet()
{
    return m_CurrentSheet;
}


/**
 * Function SetSheetNumberAndCount
 * Set the m_ScreenNumber and m_NumberOfScreen members for screens
 * must be called after a delete or add sheet command, and when entering a
 * sheet
 */
void SCH_EDIT_FRAME::SetSheetNumberAndCount()
{
    SCH_SCREEN* screen = GetScreen();
    SCH_SCREENS s_list;

    /* Set the sheet count, and the sheet number (1 for root sheet)
     */
    int            sheet_count = g_RootSheet->CountSheets();
    int            SheetNumber = 1;
    wxString       current_sheetpath = m_CurrentSheet->Path();
    SCH_SHEET_LIST SheetList;

    // Examine all sheets path to find the current sheets path,
    // and count them from root to the current sheet path:
    SCH_SHEET_PATH* sheet;

    for( sheet = SheetList.GetFirst(); sheet != NULL; sheet = SheetList.GetNext() )
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
        screen->m_NumberOfScreen = sheet_count;
    }

    GetScreen()->m_ScreenNumber = SheetNumber;
}


SCH_SCREEN* SCH_EDIT_FRAME::GetScreen() const
{
    return m_CurrentSheet->LastScreen();
}


wxString SCH_EDIT_FRAME::GetScreenDesc()
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
        g_RootSheet->SetScreen( new SCH_SCREEN() );
    }

    g_RootSheet->GetScreen()->SetFileName( m_DefaultSchematicFileName );
    g_RootSheet->GetScreen()->m_Date     = GenDate();
    m_CurrentSheet->Clear();
    m_CurrentSheet->Push( g_RootSheet );

    if( GetBaseScreen() == NULL )
        SetBaseScreen( new SCH_SCREEN() );

    GetBaseScreen()->SetZoom( 4 * GetBaseScreen()->m_ZoomScalar );
    GetBaseScreen()->m_UndoRedoCountMax = 10;
}


void SCH_EDIT_FRAME::OnCloseWindow( wxCloseEvent& Event )
{
    if( m_LibeditFrame && !m_LibeditFrame->Close() )   // Can close component editor?
        return;

    SCH_SHEET_LIST SheetList;

    if( SheetList.IsModified() )
    {
        wxMessageDialog dialog( this,
                                _( "Schematic modified, Save before exit ?" ),
                                _( "Confirmation" ), wxYES_NO | wxCANCEL |
                                wxICON_EXCLAMATION | wxYES_DEFAULT );

        switch( dialog.ShowModal() )
        {
        case wxID_CANCEL:
            Event.Veto();
            return;

        case wxID_NO:
            break;

        case wxID_OK:
        case wxID_YES:
            SaveProject();
            break;
        }
    }

    SheetList.ClearModifyStatus();

    if( !g_RootSheet->GetScreen()->GetFileName().IsEmpty()
       && (g_RootSheet->GetScreen()->GetDrawItems() != NULL) )
        SetLastProject( g_RootSheet->GetScreen()->GetFileName() );

    ClearProjectDrawList( g_RootSheet->GetScreen(), TRUE );

    /* all sub sheets are deleted, only the main sheet is usable */
    m_CurrentSheet->Clear();
    SaveSettings();
    Destroy();
}


int SCH_EDIT_FRAME::BestZoom()
{
    int    dx, dy;
    wxSize size;
    double zoom;

    dx = GetScreen()->m_CurrentSheetDesc->m_Size.x;
    dy = GetScreen()->m_CurrentSheetDesc->m_Size.y;

    size = DrawPanel->GetClientSize();
    zoom = MAX( (double) dx / (double) size.x, (double) dy / (double) size.y );

    GetScreen()->m_Curseur.x = dx / 2;
    GetScreen()->m_Curseur.y = dy / 2;

    return wxRound( zoom * (double) GetScreen()->m_ZoomScalar );
}


wxString SCH_EDIT_FRAME::GetUniqueFilenameForCurrentSheet()
{
    wxFileName fn = g_RootSheet->GetFileName();

#ifndef KICAD_GOST
    wxString filename = fn.GetName();
    if( ( filename.Len() + m_CurrentSheet->PathHumanReadable().Len() ) < 50 )
#else
    fn.ClearExt();
    wxString filename = fn.GetFullPath();
    if( ( filename.Len() + m_CurrentSheet->PathHumanReadable().Len() ) < 80 )
#endif

    {
        filename += m_CurrentSheet->PathHumanReadable();
        filename.Replace( wxT( "/" ), wxT( "-" ) );
        filename.RemoveLast();
#if defined(KICAD_GOST)
        filename.Remove( 0, 1 );
#endif
    }
    else
    {
        filename << wxT( "-" ) << GetScreen()->m_ScreenNumber;
    }

    return filename;
}

/**
 * Function OnModify
 * Must be called after a schematic change
 * in order to set the "modify" flag of the current screen
 * and update the date in frame reference
 */
void SCH_EDIT_FRAME::OnModify( )
{
    GetScreen()->SetModify();

    wxString    date = GenDate();
    SCH_SCREENS s_list;

    // Set the date for each sheet
    // There are 2 possibilities:
    // >> change only the current sheet
    // >> change all sheets.
    // I believe all sheets in a project must have the same date
    SCH_SCREEN* screen = s_list.GetFirst();
    for( ; screen != NULL; screen = s_list.GetNext() )
        screen->m_Date = date;
}

/*****************************************************************************
* Enable or disable menu entry and toolbar buttons according to current
* conditions.
*****************************************************************************/

void SCH_EDIT_FRAME::OnUpdateBlockSelected( wxUpdateUIEvent& event )
{
    bool enable = ( GetScreen() && GetScreen()->m_BlockLocate.m_Command == BLOCK_MOVE );

    event.Enable( enable );
    m_HToolBar->EnableTool( wxID_CUT, enable );
    m_HToolBar->EnableTool( wxID_COPY, enable );
}


void SCH_EDIT_FRAME::OnUpdatePaste( wxUpdateUIEvent& event )
{
    event.Enable( m_blockItems.GetCount() > 0 );
    m_HToolBar->EnableTool( wxID_PASTE, m_blockItems.GetCount() > 0 );
}


void SCH_EDIT_FRAME::OnUpdateSchematicUndo( wxUpdateUIEvent& event )
{
    if( GetScreen() )
        event.Enable( GetScreen()->GetUndoCommandCount() > 0 );
}


void SCH_EDIT_FRAME::OnUpdateSchematicRedo( wxUpdateUIEvent& event )
{
    if( GetScreen() )
        event.Enable( GetScreen()->GetRedoCommandCount() > 0 );
}


void SCH_EDIT_FRAME::OnUpdateBusOrientation( wxUpdateUIEvent& event )
{
    wxString tool_tip = g_HVLines ?
                        _( "Draw wires and buses in any direction" ) :
                        _( "Draw horizontal and vertical wires and buses only" );

    m_OptionsToolBar->ToggleTool( ID_TB_OPTIONS_BUS_WIRES_ORIENT, g_HVLines );
    m_OptionsToolBar->SetToolShortHelp( ID_TB_OPTIONS_BUS_WIRES_ORIENT, tool_tip );
}


void SCH_EDIT_FRAME::OnUpdateHiddenPins( wxUpdateUIEvent& event )
{
    wxString tool_tip = m_ShowAllPins ? _( "Do not show hidden pins" ) :
                        _( "Show hidden pins" );

    m_OptionsToolBar->ToggleTool( ID_TB_OPTIONS_HIDDEN_PINS, m_ShowAllPins );
    m_OptionsToolBar->SetToolShortHelp( ID_TB_OPTIONS_HIDDEN_PINS, tool_tip );
}


void SCH_EDIT_FRAME::OnUpdateSelectCursor( wxUpdateUIEvent& event )
{
    m_OptionsToolBar->ToggleTool( ID_TB_OPTIONS_SELECT_CURSOR, m_CursorShape );
}


void SCH_EDIT_FRAME::OnUpdateUnits( wxUpdateUIEvent& event )
{
    m_OptionsToolBar->ToggleTool( ID_TB_OPTIONS_SELECT_UNIT_MM, g_UserUnit == MILLIMETRES );
    m_OptionsToolBar->ToggleTool( ID_TB_OPTIONS_SELECT_UNIT_INCH, g_UserUnit == INCHES );
    DisplayUnitsMsg();
}


void SCH_EDIT_FRAME::OnUpdateGrid( wxUpdateUIEvent& event )
{
    wxString tool_tip = IsGridVisible() ? _( "Hide grid" ) : _( "Show grid" );

    m_OptionsToolBar->ToggleTool( ID_TB_OPTIONS_SHOW_GRID, IsGridVisible() );
    m_OptionsToolBar->SetToolShortHelp( ID_TB_OPTIONS_SHOW_GRID, tool_tip );
}


void SCH_EDIT_FRAME::OnAnnotate( wxCommandEvent& event )
{
    DIALOG_ANNOTATE* dlg = new DIALOG_ANNOTATE( this );

    dlg->ShowModal();
    dlg->Destroy();
}


void SCH_EDIT_FRAME::OnErc( wxCommandEvent& event )
{
    DIALOG_ERC* dlg = new DIALOG_ERC( this );

    dlg->ShowModal();
    dlg->Destroy();
}


void SCH_EDIT_FRAME::OnCreateNetlist( wxCommandEvent& event )
{
    int i;

    if( m_NetlistFormat <  NET_TYPE_PCBNEW )
        m_NetlistFormat = NET_TYPE_PCBNEW;

    do
    {
        WinEDA_NetlistFrame* dlg = new WinEDA_NetlistFrame( this );
        i = dlg->ShowModal();
        dlg->Destroy();
    } while( i == NET_PLUGIN_CHANGE );

    // If a plugin is removed or added, rebuild and reopen the new dialog
}


void SCH_EDIT_FRAME::OnCreateBillOfMaterials( wxCommandEvent& )
{
    DIALOG_BUILD_BOM* dlg = new DIALOG_BUILD_BOM( this );

    dlg->ShowModal();
    dlg->Destroy();
}


void SCH_EDIT_FRAME::OnFindItems( wxCommandEvent& event )
{
    wxASSERT_MSG( m_findReplaceData != NULL,
                  wxT( "Forgot to create find/replace data.  Bad Programmer!" ) );

    this->DrawPanel->m_IgnoreMouseEvents = TRUE;

    if( m_dlgFindReplace )
    {
        delete m_dlgFindReplace;
        m_dlgFindReplace = NULL;
    }

    m_dlgFindReplace = new DIALOG_SCH_FIND( this, m_findReplaceData, m_findDialogPosition,
                                            m_findDialogSize );
    m_dlgFindReplace->SetFindEntries( m_findStringHistoryList );
    m_dlgFindReplace->SetReplaceEntries( m_replaceStringHistoryList );
    m_dlgFindReplace->SetMinSize( m_dlgFindReplace->GetBestSize() );
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

    DrawPanel->m_IgnoreMouseEvents = false;
}


void SCH_EDIT_FRAME::OnLoadFile( wxCommandEvent& event )
{
    wxString fn;

    fn = GetFileFromHistory( event.GetId(), _( "Schematic" ) );

    if( fn != wxEmptyString )
    {
        LoadOneEEProject( fn, false );
        SetToolbars();
    }
}


void SCH_EDIT_FRAME::OnLoadStuffFile( wxCommandEvent& event )
{
    ReadInputStuffFile();
    DrawPanel->Refresh();
}


void SCH_EDIT_FRAME::OnNewProject( wxCommandEvent& event )
{
    LoadOneEEProject( wxEmptyString, true );
}


void SCH_EDIT_FRAME::OnLoadProject( wxCommandEvent& event )
{
    LoadOneEEProject( wxEmptyString, false );
}


void SCH_EDIT_FRAME::OnOpenPcbnew( wxCommandEvent& event )
{
    wxFileName fn = g_RootSheet->GetScreen()->GetFileName();

    if( fn.IsOk() )
    {
        fn.SetExt( PcbFileExtension );

        wxString filename = QuoteFullPath( fn );

        ExecuteFile( this, PCBNEW_EXE, filename );
    }
    else
        ExecuteFile( this, PCBNEW_EXE );
}


void SCH_EDIT_FRAME::OnOpenCvpcb( wxCommandEvent& event )
{
    wxFileName fn = g_RootSheet->GetScreen()->GetFileName();

    fn.SetExt( NetlistFileExtension );

    if( fn.IsOk() && fn.FileExists() )
    {
        ExecuteFile( this, CVPCB_EXE, QuoteFullPath( fn ) );
    }
    else
        ExecuteFile( this, CVPCB_EXE );
}


void SCH_EDIT_FRAME::OnOpenLibraryViewer( wxCommandEvent& event )
{
    if( m_ViewlibFrame )
    {
        m_ViewlibFrame->Show( TRUE );
    }
    else
    {
        m_ViewlibFrame = new LIB_VIEW_FRAME( this );
        m_ViewlibFrame->AdjustScrollBars();
    }
}


void SCH_EDIT_FRAME::OnOpenLibraryEditor( wxCommandEvent& event )
{
    if( m_LibeditFrame )
    {
        m_LibeditFrame->Show( TRUE );
    }
    else
    {
        m_LibeditFrame = new LIB_EDIT_FRAME( this,
                                             wxT( "Library Editor" ),
                                             wxPoint( -1, -1 ),
                                             wxSize( 600, 400 ) );
        m_LibeditFrame->AdjustScrollBars();
    }
}


void SCH_EDIT_FRAME::OnExit( wxCommandEvent& event )
{
    Close( true );
}

/**
 * Function SetLanguage
 * called on a language menu selection
 */
void SCH_EDIT_FRAME::SetLanguage( wxCommandEvent& event )
{
    EDA_BASE_FRAME::SetLanguage( event );

    if( m_LibeditFrame )
        m_LibeditFrame->EDA_BASE_FRAME::SetLanguage( event );
}


void SCH_EDIT_FRAME::OnPrint( wxCommandEvent& event )
{
    wxFileName fn;
    DIALOG_PRINT_USING_PRINTER dlg( this );

    dlg.ShowModal();

    fn = g_RootSheet->GetScreen()->GetFileName();

    wxString default_name = NAMELESS_PROJECT;
    default_name += wxT( ".sch" );

    if( fn.GetFullName() != default_name )
    {
        fn.SetExt( ProjectFileExtension );
        wxGetApp().WriteProjectConfig( fn.GetFullPath(), GROUP, GetProjectFileParameters() );
    }
}


/* Creates the SVG print file for the current edited component.
 */
void SCH_EDIT_FRAME::SVG_Print( wxCommandEvent& event )
{
    DIALOG_SVG_PRINT frame( this );

    frame.ShowModal();
}
