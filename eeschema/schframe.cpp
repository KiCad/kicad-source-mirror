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


BEGIN_EVENT_TABLE( WinEDA_SchematicFrame, WinEDA_DrawFrame )
    EVT_SOCKET( ID_EDA_SOCKET_EVENT_SERV, WinEDA_DrawFrame::OnSockRequestServer )
    EVT_SOCKET( ID_EDA_SOCKET_EVENT, WinEDA_DrawFrame::OnSockRequest )

    EVT_CLOSE( WinEDA_SchematicFrame::OnCloseWindow )
    EVT_SIZE( WinEDA_SchematicFrame::OnSize )

    EVT_MENU( ID_NEW_PROJECT, WinEDA_SchematicFrame::OnNewProject )
    EVT_MENU( ID_LOAD_PROJECT, WinEDA_SchematicFrame::OnLoadProject )

    EVT_MENU_RANGE( wxID_FILE1, wxID_FILE9, WinEDA_SchematicFrame::OnLoadFile )

    EVT_TOOL( ID_NEW_PROJECT, WinEDA_SchematicFrame::OnNewProject )
    EVT_TOOL( ID_LOAD_PROJECT, WinEDA_SchematicFrame::OnLoadProject )

    EVT_MENU( ID_SAVE_PROJECT, WinEDA_SchematicFrame::Save_File )
    EVT_MENU( ID_SAVE_ONE_SHEET, WinEDA_SchematicFrame::Save_File )
    EVT_MENU( ID_SAVE_ONE_SHEET_AS, WinEDA_SchematicFrame::Save_File )
    EVT_TOOL( ID_SAVE_PROJECT, WinEDA_SchematicFrame::Save_File )
    EVT_MENU( wxID_PRINT, WinEDA_SchematicFrame::OnPrint )
    EVT_MENU( ID_GEN_PLOT_PS, WinEDA_SchematicFrame::ToPlot_PS )
    EVT_MENU( ID_GEN_PLOT_HPGL, WinEDA_SchematicFrame::ToPlot_HPGL )
    EVT_MENU( ID_GEN_PLOT_SVG, WinEDA_SchematicFrame::SVG_Print )
    EVT_MENU( ID_GEN_PLOT_DXF, WinEDA_SchematicFrame::ToPlot_DXF )
    EVT_MENU( ID_GEN_COPY_SHEET_TO_CLIPBOARD, WinEDA_DrawFrame::CopyToClipboard )
    EVT_MENU( ID_GEN_COPY_BLOCK_TO_CLIPBOARD, WinEDA_DrawFrame::CopyToClipboard )
    EVT_MENU( wxID_EXIT, WinEDA_SchematicFrame::OnExit )

    EVT_MENU( ID_POPUP_SCH_COPY_ITEM, WinEDA_SchematicFrame::OnCopySchematicItemRequest )

    EVT_MENU( ID_CONFIG_REQ, WinEDA_SchematicFrame::InstallConfigFrame )
    EVT_MENU( ID_CONFIG_SAVE, WinEDA_SchematicFrame::Process_Config )
    EVT_MENU( ID_CONFIG_READ, WinEDA_SchematicFrame::Process_Config )
    EVT_MENU_RANGE( ID_PREFERENCES_HOTKEY_START,
                    ID_PREFERENCES_HOTKEY_END,
                    WinEDA_SchematicFrame::Process_Config )

    EVT_MENU( ID_COLORS_SETUP, WinEDA_SchematicFrame::OnColorConfig )
    EVT_TOOL( ID_OPTIONS_SETUP, WinEDA_SchematicFrame::OnSetOptions )

    EVT_MENU_RANGE( ID_LANGUAGE_CHOICE, ID_LANGUAGE_CHOICE_END,
                    WinEDA_SchematicFrame::SetLanguage )

    EVT_TOOL_RANGE( ID_ZOOM_IN, ID_ZOOM_PAGE, WinEDA_SchematicFrame::OnZoom )

    EVT_TOOL( ID_TO_LIBRARY, WinEDA_SchematicFrame::OnOpenLibraryEditor )
    EVT_TOOL( ID_TO_LIBVIEW, WinEDA_SchematicFrame::OnOpenLibraryViewer )

    EVT_TOOL( ID_TO_PCB, WinEDA_SchematicFrame::OnOpenPcbnew )
    EVT_TOOL( ID_TO_CVPCB, WinEDA_SchematicFrame::OnOpenCvpcb )

    EVT_TOOL( ID_SHEET_SET, WinEDA_DrawFrame::Process_PageSettings )
    EVT_TOOL( ID_HIERARCHY, WinEDA_SchematicFrame::Process_Special_Functions )
    EVT_TOOL( wxID_CUT, WinEDA_SchematicFrame::Process_Special_Functions )
    EVT_TOOL( wxID_COPY, WinEDA_SchematicFrame::Process_Special_Functions )
    EVT_TOOL( wxID_PASTE, WinEDA_SchematicFrame::Process_Special_Functions )
    EVT_TOOL( wxID_UNDO, WinEDA_SchematicFrame::GetSchematicFromUndoList )
    EVT_TOOL( wxID_REDO, WinEDA_SchematicFrame::GetSchematicFromRedoList )
    EVT_TOOL( ID_GET_ANNOTATE, WinEDA_SchematicFrame::OnAnnotate )
    EVT_TOOL( wxID_PRINT, WinEDA_SchematicFrame::OnPrint )
    EVT_TOOL( ID_GET_ERC, WinEDA_SchematicFrame::OnErc )
    EVT_TOOL( ID_GET_NETLIST, WinEDA_SchematicFrame::OnCreateNetlist )
    EVT_TOOL( ID_GET_TOOLS, WinEDA_SchematicFrame::OnCreateBillOfMaterials )
    EVT_TOOL( ID_FIND_ITEMS, WinEDA_SchematicFrame::OnFindItems )
    EVT_TOOL( ID_BACKANNO_ITEMS, WinEDA_SchematicFrame::OnLoadStuffFile )
    EVT_TOOL( ID_COMPONENT_BUTT, WinEDA_SchematicFrame::Process_Special_Functions )

    EVT_MENU( ID_GENERAL_HELP, WinEDA_DrawFrame::GetKicadHelp )
    EVT_MENU( ID_KICAD_ABOUT, WinEDA_DrawFrame::GetKicadAbout )

    // Tools and buttons for vertical toolbar.
    EVT_TOOL( ID_NO_SELECT_BUTT, WinEDA_SchematicFrame::Process_Special_Functions )
    EVT_TOOL_RANGE( ID_SCHEMATIC_VERTICAL_TOOLBAR_START,
                    ID_SCHEMATIC_VERTICAL_TOOLBAR_END,
                    WinEDA_SchematicFrame::Process_Special_Functions )

    EVT_MENU_RANGE( ID_POPUP_START_RANGE, ID_POPUP_END_RANGE,
                    WinEDA_SchematicFrame::Process_Special_Functions )

    // Tools and buttons options toolbar
    EVT_TOOL_RANGE( ID_TB_OPTIONS_START, ID_TB_OPTIONS_END,
                    WinEDA_SchematicFrame::OnSelectOptionToolbar )

    EVT_MENU_RANGE( ID_POPUP_GENERAL_START_RANGE, ID_POPUP_GENERAL_END_RANGE,
                    WinEDA_SchematicFrame::Process_Special_Functions )

    /* Handle user interface update events. */
    EVT_UPDATE_UI( wxID_CUT, WinEDA_SchematicFrame::OnUpdateBlockSelected )
    EVT_UPDATE_UI( wxID_COPY, WinEDA_SchematicFrame::OnUpdateBlockSelected )
    EVT_UPDATE_UI( wxID_PASTE, WinEDA_SchematicFrame::OnUpdatePaste )
    EVT_UPDATE_UI( wxID_UNDO, WinEDA_SchematicFrame::OnUpdateSchematicUndo )
    EVT_UPDATE_UI( wxID_REDO, WinEDA_SchematicFrame::OnUpdateSchematicRedo )
    EVT_UPDATE_UI( ID_TB_OPTIONS_SHOW_GRID, WinEDA_SchematicFrame::OnUpdateGrid )
    EVT_UPDATE_UI( ID_TB_OPTIONS_SELECT_CURSOR, WinEDA_SchematicFrame::OnUpdateSelectCursor )
    EVT_UPDATE_UI( ID_TB_OPTIONS_HIDDEN_PINS, WinEDA_SchematicFrame::OnUpdateHiddenPins )
    EVT_UPDATE_UI( ID_TB_OPTIONS_BUS_WIRES_ORIENT, WinEDA_SchematicFrame::OnUpdateBusOrientation )
    EVT_UPDATE_UI_RANGE( ID_TB_OPTIONS_SELECT_UNIT_MM,
                         ID_TB_OPTIONS_SELECT_UNIT_INCH,
                         WinEDA_SchematicFrame::OnUpdateUnits )

    /* Search dialog events. */
    EVT_FIND_CLOSE( wxID_ANY, WinEDA_SchematicFrame::OnFindDialogClose )
    EVT_FIND_DRC_MARKER( wxID_ANY, WinEDA_SchematicFrame::OnFindDrcMarker )
    EVT_FIND( wxID_ANY, WinEDA_SchematicFrame::OnFindSchematicItem )

END_EVENT_TABLE()


WinEDA_SchematicFrame::WinEDA_SchematicFrame( wxWindow*       father,
                                              const wxString& title,
                                              const wxPoint&  pos,
                                              const wxSize&   size,
                                              long            style ) :
    WinEDA_DrawFrame( father, SCHEMATIC_FRAME, title, pos, size, style )
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

    g_ItemToRepeat = NULL;

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


WinEDA_SchematicFrame::~WinEDA_SchematicFrame()
{
    SAFE_DELETE( g_RootSheet );
    SAFE_DELETE( m_CurrentSheet );     // a SCH_SHEET_PATH, on the heap.
    SAFE_DELETE( m_findReplaceData );
    CMP_LIBRARY::RemoveAllLibraries();
}


BASE_SCREEN* WinEDA_SchematicFrame::GetBaseScreen() const
{
    return GetScreen();
}


SCH_SHEET_PATH* WinEDA_SchematicFrame::GetSheet()
{
    return m_CurrentSheet;
}


/**
 * Function SetSheetNumberAndCount
 * Set the m_ScreenNumber and m_NumberOfScreen members for screens
 * must be called after a delete or add sheet command, and when entering a
 * sheet
 */
void WinEDA_SchematicFrame::SetSheetNumberAndCount()
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


SCH_SCREEN* WinEDA_SchematicFrame::GetScreen() const
{
    return m_CurrentSheet->LastScreen();
}


wxString WinEDA_SchematicFrame::GetScreenDesc()
{
    wxString s = m_CurrentSheet->PathHumanReadable();

    return s;
}


void WinEDA_SchematicFrame::CreateScreens()
{
    if( g_RootSheet == NULL )
    {
        g_RootSheet = new SCH_SHEET();
    }
    if( g_RootSheet->m_AssociatedScreen == NULL )
    {
        g_RootSheet->m_AssociatedScreen = new SCH_SCREEN();
        g_RootSheet->m_AssociatedScreen->m_RefCount++;
    }
    g_RootSheet->m_AssociatedScreen->m_FileName = m_DefaultSchematicFileName;
    g_RootSheet->m_AssociatedScreen->m_Date     = GenDate();
    m_CurrentSheet->Clear();
    m_CurrentSheet->Push( g_RootSheet );

    if( GetBaseScreen() == NULL )
        SetBaseScreen( new SCH_SCREEN() );
    GetBaseScreen()->SetZoom( 4 * GetBaseScreen()->m_ZoomScalar );
    GetBaseScreen()->m_UndoRedoCountMax = 10;
}


void WinEDA_SchematicFrame::OnCloseWindow( wxCloseEvent& Event )
{
    SCH_SHEET_PATH* sheet;

    if( m_LibeditFrame ) // Can close component editor ?
    {
        if( !m_LibeditFrame->Close() )
            return;
    }

    SCH_SHEET_LIST SheetList;

    for( sheet = SheetList.GetFirst(); sheet != NULL; sheet = SheetList.GetNext() )
    {
        if( sheet->LastScreen() && sheet->LastScreen()->IsModify() )
            break;
    }

    if( sheet )
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

    for( sheet = SheetList.GetFirst();
        sheet != NULL;
        sheet = SheetList.GetNext() )
    {
        if( sheet->LastScreen() )
        {
            sheet->LastScreen()->ClrModify();
        }
    }

    if( !g_RootSheet->m_AssociatedScreen->m_FileName.IsEmpty()
       && (g_RootSheet->m_AssociatedScreen->EEDrawList != NULL) )
        SetLastProject( g_RootSheet->m_AssociatedScreen->m_FileName );

    ClearProjectDrawList( g_RootSheet->m_AssociatedScreen, TRUE );

    /* all sub sheets are deleted, only the main sheet is usable */
    m_CurrentSheet->Clear();
    SaveSettings();
    Destroy();
}


int WinEDA_SchematicFrame::BestZoom()
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


/**
 * Function GetUniqueFilenameForCurrentSheet
 * @return a filename that can be used in plot and print functions
 * for the current screen and sheet path.
 * This filename is unique and must be used insteed of the screen filename
 * (or sheet filename) when one must creates file for each sheet in the
 * heierarchy.
 * because in complex hierarchies a sheet and a SCH_SCREEN is used more than
 * once
 * Name is <root sheet filename>-<sheet path>
 * and has no extension.
 * However if filename is too long name is <sheet filename>-<sheet number>
 */
wxString WinEDA_SchematicFrame::GetUniqueFilenameForCurrentSheet()
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
void WinEDA_SchematicFrame::OnModify( )
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

void WinEDA_SchematicFrame::OnUpdateBlockSelected( wxUpdateUIEvent& event )
{
    bool enable = ( GetScreen() && GetScreen()->m_BlockLocate.m_Command == BLOCK_MOVE );

    event.Enable( enable );
    m_HToolBar->EnableTool( wxID_CUT, enable );
    m_HToolBar->EnableTool( wxID_COPY, enable );
}


void WinEDA_SchematicFrame::OnUpdatePaste( wxUpdateUIEvent& event )
{
    event.Enable( g_BlockSaveDataList.GetCount() > 0 );
    m_HToolBar->EnableTool( wxID_PASTE, g_BlockSaveDataList.GetCount() > 0 );
}


void WinEDA_SchematicFrame::OnUpdateSchematicUndo( wxUpdateUIEvent& event )
{
    if( GetScreen() )
        event.Enable( GetScreen()->GetUndoCommandCount() > 0 );
}


void WinEDA_SchematicFrame::OnUpdateSchematicRedo( wxUpdateUIEvent& event )
{
    if( GetScreen() )
        event.Enable( GetScreen()->GetRedoCommandCount() > 0 );
}


void WinEDA_SchematicFrame::OnUpdateBusOrientation( wxUpdateUIEvent& event )
{
    wxString tool_tip = g_HVLines ?
                        _( "Draw wires and buses in any direction" ) :
                        _( "Draw horizontal and vertical wires and buses only" );

    m_OptionsToolBar->ToggleTool( ID_TB_OPTIONS_BUS_WIRES_ORIENT, g_HVLines );
    m_OptionsToolBar->SetToolShortHelp( ID_TB_OPTIONS_BUS_WIRES_ORIENT, tool_tip );
}


void WinEDA_SchematicFrame::OnUpdateHiddenPins( wxUpdateUIEvent& event )
{
    wxString tool_tip = m_ShowAllPins ? _( "Do not show hidden pins" ) :
                        _( "Show hidden pins" );

    m_OptionsToolBar->ToggleTool( ID_TB_OPTIONS_HIDDEN_PINS, m_ShowAllPins );
    m_OptionsToolBar->SetToolShortHelp( ID_TB_OPTIONS_HIDDEN_PINS, tool_tip );
}


void WinEDA_SchematicFrame::OnUpdateSelectCursor( wxUpdateUIEvent& event )
{
    m_OptionsToolBar->ToggleTool( ID_TB_OPTIONS_SELECT_CURSOR, m_CursorShape );
}


void WinEDA_SchematicFrame::OnUpdateUnits( wxUpdateUIEvent& event )
{
    m_OptionsToolBar->ToggleTool( ID_TB_OPTIONS_SELECT_UNIT_MM, g_UserUnit == MILLIMETRES );
    m_OptionsToolBar->ToggleTool( ID_TB_OPTIONS_SELECT_UNIT_INCH, g_UserUnit == INCHES );
    DisplayUnitsMsg();
}


void WinEDA_SchematicFrame::OnUpdateGrid( wxUpdateUIEvent& event )
{
    wxString tool_tip = IsGridVisible() ? _( "Hide grid" ) : _( "Show grid" );

    m_OptionsToolBar->ToggleTool( ID_TB_OPTIONS_SHOW_GRID, IsGridVisible() );
    m_OptionsToolBar->SetToolShortHelp( ID_TB_OPTIONS_SHOW_GRID, tool_tip );
}


void WinEDA_SchematicFrame::OnAnnotate( wxCommandEvent& event )
{
    DIALOG_ANNOTATE* dlg = new DIALOG_ANNOTATE( this );

    dlg->ShowModal();
    dlg->Destroy();
}


void WinEDA_SchematicFrame::OnErc( wxCommandEvent& event )
{
    DIALOG_ERC* dlg = new DIALOG_ERC( this );

    dlg->ShowModal();
    dlg->Destroy();
}


void WinEDA_SchematicFrame::OnCreateNetlist( wxCommandEvent& event )
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


void WinEDA_SchematicFrame::OnCreateBillOfMaterials( wxCommandEvent& )
{
    DIALOG_BUILD_BOM* dlg = new DIALOG_BUILD_BOM( this );

    dlg->ShowModal();
    dlg->Destroy();
}


void WinEDA_SchematicFrame::OnFindItems( wxCommandEvent& event )
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


void WinEDA_SchematicFrame::OnFindDialogClose( wxFindDialogEvent& event )
{
    if( m_dlgFindReplace )
    {
        m_findDialogPosition = m_dlgFindReplace->GetPosition();
        m_findDialogSize = m_dlgFindReplace->GetSize();
        m_findStringHistoryList = m_dlgFindReplace->GetFindEntries();
        m_replaceStringHistoryList = m_dlgFindReplace->GetReplaceEntries();
        m_dlgFindReplace->Destroy();
        m_dlgFindReplace = NULL;
    }

    this->DrawPanel->m_IgnoreMouseEvents = FALSE;
}


void WinEDA_SchematicFrame::OnLoadFile( wxCommandEvent& event )
{
    wxString fn;

    fn = GetFileFromHistory( event.GetId(), _( "Schematic" ) );

    if( fn != wxEmptyString )
    {
        LoadOneEEProject( fn, false );
        SetToolbars();
    }
}


void WinEDA_SchematicFrame::OnLoadStuffFile( wxCommandEvent& event )
{
    ReadInputStuffFile();
    DrawPanel->Refresh();
}


void WinEDA_SchematicFrame::OnNewProject( wxCommandEvent& event )
{
    LoadOneEEProject( wxEmptyString, true );
}


void WinEDA_SchematicFrame::OnLoadProject( wxCommandEvent& event )
{
    LoadOneEEProject( wxEmptyString, false );
}


void WinEDA_SchematicFrame::OnOpenPcbnew( wxCommandEvent& event )
{
    wxFileName fn = g_RootSheet->m_AssociatedScreen->m_FileName;

    if( fn.IsOk() )
    {
        fn.SetExt( PcbFileExtension );

        wxString filename = QuoteFullPath( fn );

        ExecuteFile( this, PCBNEW_EXE, filename );
    }
    else
        ExecuteFile( this, PCBNEW_EXE );
}


void WinEDA_SchematicFrame::OnOpenCvpcb( wxCommandEvent& event )
{
    wxFileName fn = g_RootSheet->m_AssociatedScreen->m_FileName;

    fn.SetExt( NetlistFileExtension );

    if( fn.IsOk() && fn.FileExists() )
    {
        ExecuteFile( this, CVPCB_EXE, QuoteFullPath( fn ) );
    }
    else
        ExecuteFile( this, CVPCB_EXE );
}


void WinEDA_SchematicFrame::OnOpenLibraryViewer( wxCommandEvent& event )
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


void WinEDA_SchematicFrame::OnOpenLibraryEditor( wxCommandEvent& event )
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
        ActiveScreen = GetBaseScreen();
        m_LibeditFrame->AdjustScrollBars();
    }
}


void WinEDA_SchematicFrame::OnExit( wxCommandEvent& event )
{
    Close( true );
}

/**
 * Function SetLanguage
 * called on a language menu selection
 */
void WinEDA_SchematicFrame::SetLanguage( wxCommandEvent& event )
{
    WinEDA_BasicFrame::SetLanguage( event );

    if( m_LibeditFrame )
        m_LibeditFrame->WinEDA_BasicFrame::SetLanguage( event );
}


void WinEDA_SchematicFrame::OnPrint( wxCommandEvent& event )
{
    wxFileName fn;
    DIALOG_PRINT_USING_PRINTER dlg( this );

    dlg.ShowModal();

    fn = g_RootSheet->m_AssociatedScreen->m_FileName;

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
void WinEDA_SchematicFrame::SVG_Print( wxCommandEvent& event )
{
    DIALOG_SVG_PRINT frame( this );

    frame.ShowModal();
}
