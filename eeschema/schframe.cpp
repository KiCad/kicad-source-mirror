/******************************************************************/
/* schframe.cpp  - fonctions des classes du type WinEDA_DrawFrame */
/******************************************************************/

#ifdef __GNUG__
#pragma implementation
#endif

#include "fctsys.h"
#include "common.h"

#include "program.h"
#include "libcmp.h"
#include "general.h"

#include "bitmaps.h"
#include "protos.h"
#include "id.h"
#include "netlist.h"

#include "annotate_dialog.h"
#include "dialog_build_BOM.h"
#include "dialog_erc.h"
#include "dialog_find.h"
#include "netlist_control.h"


/*******************************/
/* class WinEDA_SchematicFrame */
/*******************************/

BEGIN_EVENT_TABLE( WinEDA_SchematicFrame, wxFrame )
COMMON_EVENTS_DRAWFRAME EVT_SOCKET(
    ID_EDA_SOCKET_EVENT_SERV,
    WinEDA_DrawFrame::
    OnSockRequestServer )
EVT_SOCKET( ID_EDA_SOCKET_EVENT, WinEDA_DrawFrame::OnSockRequest )

EVT_CLOSE( WinEDA_SchematicFrame::OnCloseWindow )
EVT_SIZE( WinEDA_SchematicFrame::OnSize )

EVT_MENU( ID_NEW_PROJECT, WinEDA_SchematicFrame::OnNewProject )
EVT_MENU( ID_LOAD_PROJECT, WinEDA_SchematicFrame::OnLoadProject )

EVT_MENU_RANGE( ID_LOAD_FILE_1, ID_LOAD_FILE_10,
                WinEDA_SchematicFrame::OnLoadFile )

EVT_TOOL( ID_NEW_PROJECT, WinEDA_SchematicFrame::OnNewProject )
EVT_TOOL( ID_LOAD_PROJECT, WinEDA_SchematicFrame::OnLoadProject )

EVT_TOOL_RANGE( ID_SCHEMATIC_MAIN_TOOLBAR_START,
                ID_SCHEMATIC_MAIN_TOOLBAR_END,
                WinEDA_SchematicFrame::Process_Special_Functions )

EVT_MENU_RANGE( ID_PREFERENCES_FONT_INFOSCREEN, ID_PREFERENCES_FONT_END,
                WinEDA_DrawFrame::ProcessFontPreferences )

EVT_MENU( ID_SAVE_PROJECT, WinEDA_SchematicFrame::Save_File )
EVT_MENU( ID_SAVE_ONE_SHEET, WinEDA_SchematicFrame::Save_File )
EVT_MENU( ID_SAVE_ONE_SHEET_AS, WinEDA_SchematicFrame::Save_File )
EVT_TOOL( ID_SAVE_PROJECT, WinEDA_SchematicFrame::Save_File )
EVT_MENU( ID_GEN_PRINT, WinEDA_SchematicFrame::ToPrinter )
EVT_MENU( ID_GEN_PLOT_PS, WinEDA_SchematicFrame::ToPlot_PS )
EVT_MENU( ID_GEN_PLOT_HPGL, WinEDA_SchematicFrame::ToPlot_HPGL )
EVT_MENU( ID_GEN_PLOT_SVG, WinEDA_DrawFrame::SVG_Print )
EVT_MENU( ID_GEN_COPY_SHEET_TO_CLIPBOARD, WinEDA_DrawFrame::CopyToClipboard )
EVT_MENU( ID_GEN_COPY_BLOCK_TO_CLIPBOARD, WinEDA_DrawFrame::CopyToClipboard )
EVT_MENU( ID_EXIT, WinEDA_SchematicFrame::OnExit )

EVT_MENU_RANGE( ID_CONFIG_AND_PREFERENCES_START,
                ID_CONFIG_AND_PREFERENCES_END,
                WinEDA_SchematicFrame::Process_Config )
EVT_TOOL( ID_COLORS_SETUP, WinEDA_SchematicFrame::Process_Config )
EVT_TOOL( ID_OPTIONS_SETUP, WinEDA_SchematicFrame::Process_Config )

EVT_MENU_RANGE( ID_LANGUAGE_CHOICE, ID_LANGUAGE_CHOICE_END,
                WinEDA_DrawFrame::SetLanguage )

EVT_TOOL_RANGE( ID_ZOOM_IN_BUTT, ID_ZOOM_PAGE_BUTT,
                WinEDA_SchematicFrame::Process_Zoom )

EVT_TOOL( ID_TO_LIBRARY, WinEDA_SchematicFrame::OnOpenLibraryEditor )
EVT_TOOL( ID_TO_LIBVIEW, WinEDA_SchematicFrame::OnOpenLibraryViewer )

EVT_TOOL( ID_TO_PCB, WinEDA_SchematicFrame::OnOpenPcbnew )
EVT_TOOL( ID_TO_CVPCB, WinEDA_SchematicFrame::OnOpenCvpcb )

EVT_TOOL( ID_SHEET_SET, WinEDA_DrawFrame::Process_PageSettings )
EVT_TOOL( ID_HIERARCHY, WinEDA_SchematicFrame::Process_Special_Functions )
EVT_TOOL( wxID_CUT, WinEDA_SchematicFrame::Process_Special_Functions )
EVT_TOOL( wxID_COPY, WinEDA_SchematicFrame::Process_Special_Functions )
EVT_TOOL( wxID_PASTE, WinEDA_SchematicFrame::Process_Special_Functions )
EVT_TOOL( ID_UNDO_BUTT, WinEDA_SchematicFrame::Process_Special_Functions )
EVT_TOOL( ID_GET_ANNOTATE, WinEDA_SchematicFrame::OnAnnotate )
EVT_TOOL( ID_GEN_PRINT, WinEDA_SchematicFrame::ToPrinter )
EVT_TOOL( ID_GET_ERC, WinEDA_SchematicFrame::OnErc )
EVT_TOOL( ID_GET_NETLIST, WinEDA_SchematicFrame::OnCreateNetlist )
EVT_TOOL( ID_GET_TOOLS, WinEDA_SchematicFrame::OnCreateBillOfMaterials )
EVT_TOOL( ID_FIND_ITEMS, WinEDA_SchematicFrame::OnFindItems )
EVT_TOOL( ID_BACKANNO_ITEMS, WinEDA_SchematicFrame::OnLoadStuffFile )

EVT_MENU( ID_GENERAL_HELP, WinEDA_DrawFrame::GetKicadHelp )
EVT_MENU( ID_KICAD_ABOUT, WinEDA_DrawFrame::GetKicadAbout )

// Tools et boutons de Schematique, Vertical toolbar:
EVT_TOOL_RANGE( ID_SCHEMATIC_VERTICAL_TOOLBAR_START,
                ID_SCHEMATIC_VERTICAL_TOOLBAR_END,
                WinEDA_SchematicFrame::Process_Special_Functions )

EVT_TOOL_RCLICKED( ID_LABEL_BUTT, WinEDA_SchematicFrame::ToolOnRightClick )
EVT_TOOL_RCLICKED( ID_GLABEL_BUTT, WinEDA_SchematicFrame::ToolOnRightClick )
EVT_TOOL_RCLICKED( ID_HIERLABEL_BUTT,
                   WinEDA_SchematicFrame::ToolOnRightClick )

EVT_MENU_RANGE( ID_POPUP_START_RANGE, ID_POPUP_END_RANGE,
                WinEDA_SchematicFrame::Process_Special_Functions )

// Tools et boutons de Schematique, Options toolbar:
EVT_TOOL_RANGE( ID_TB_OPTIONS_START, ID_TB_OPTIONS_END,
                WinEDA_SchematicFrame::OnSelectOptionToolbar )

EVT_MENU_RANGE( ID_POPUP_GENERAL_START_RANGE, ID_POPUP_GENERAL_END_RANGE,
                WinEDA_SchematicFrame::Process_Special_Functions )

END_EVENT_TABLE()


/****************/
/* Constructor */
/****************/

WinEDA_SchematicFrame::WinEDA_SchematicFrame( wxWindow*       father,
                                              const wxString& title,
                                              const wxPoint&  pos,
                                              const wxSize&   size,
                                              long            style ) :
    WinEDA_DrawFrame( father, SCHEMATIC_FRAME, title, pos, size, style )
{
    wxConfig* config = wxGetApp().m_EDA_Config;

    m_FrameName = wxT( "SchematicFrame" );
    m_Draw_Axis = FALSE;                // TRUE to show axis
    m_Draw_Grid = g_ShowGrid;           // TRUE to show a grid
    m_Draw_Sheet_Ref = TRUE;            // TRUE to show sheet references
    m_CurrentSheet   = new DrawSheetPath();
    m_CurrentField   = NULL;
    m_Multiflag     = 0;
    m_TextFieldSize = DEFAULT_SIZE_TEXT;
    m_LibeditFrame  = NULL;         // Component editor frame.
    m_ViewlibFrame  = NULL;         // Frame for browsing component libraries

    CreateScreens();

    // Give an icon
#ifdef __WINDOWS__
    SetIcon( wxICON( a_icon_eeschema ) );
#else
    SetIcon( wxICON( icon_eeschema ) );
#endif

    g_ItemToRepeat = NULL;

    /* Get config */
    GetSettings();

    if( config )
    {
        g_DrawMinimunLineWidth = config->Read( MINI_DRAW_LINE_WIDTH_KEY,
                                               (long) 0 );
        g_PlotPSMinimunLineWidth = config->Read( MINI_PLOTPS_LINE_WIDTH_KEY,
                                                 (long) 4 );
    }

    SetSize( m_FramePos.x, m_FramePos.y, m_FrameSize.x, m_FrameSize.y );

    if( DrawPanel )
        DrawPanel->m_Block_Enable = TRUE;

    ReCreateMenuBar();
    ReCreateHToolbar();
    ReCreateVToolbar();
    ReCreateOptToolbar();
}


/***************/
/* Destructor */
/***************/

WinEDA_SchematicFrame::~WinEDA_SchematicFrame()
{
    SAFE_DELETE( g_RootSheet );
    SAFE_DELETE( m_CurrentSheet ); //a DrawSheetPath, on the heap.
    m_CurrentSheet = NULL;
}


BASE_SCREEN* WinEDA_SchematicFrame::GetBaseScreen() const
{
    return GetScreen();
}


/***************/
/* utility functions */
/***************/
DrawSheetPath* WinEDA_SchematicFrame::GetSheet()
{
    return m_CurrentSheet;
}


/****************************************************/
void WinEDA_SchematicFrame::SetSheetNumberAndCount()
/****************************************************/

/** Function SetSheetNumberAndCount
 * Set the m_ScreenNumber and m_NumberOfScreen members for screens
 * must be called after a delete or add sheet command, and when entering a sheet
 */
{
    SCH_SCREEN*    screen = GetScreen();
    EDA_ScreenList s_list;

    /* Set the sheet count, and the sheet number (1 for root sheet)
     */
    int            sheet_count = g_RootSheet->CountSheets();
    int            SheetNumber = 1;
    wxString       current_sheetpath = m_CurrentSheet->Path();
    EDA_SheetList  SheetList;

    // Examine all sheets path to find the current sheets path,
    // and count them from root to the current scheet path:
    DrawSheetPath* sheet;

    for( sheet = SheetList.GetFirst(); sheet != NULL; sheet = SheetList.GetNext() )
    {
        wxString sheetpath = sheet->Path();
        if( sheetpath == current_sheetpath )    // Current sheet path found
            break;
        SheetNumber++;                          // not found, increment sheet number before this current path
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


void WinEDA_SchematicFrame::SetScreen( SCH_SCREEN* screen )
{
    //find it in the hierarchy, and set it.
    //there is ambiguity in this function (there may be several
    //instances of a given sheet, but irregardless it is useful
    //for printing etc.
    DrawSheetPath sheetlist;

    if( g_RootSheet->LocatePathOfScreen( screen, &sheetlist ) )
    {
        *m_CurrentSheet = sheetlist;
        m_CurrentSheet->UpdateAllScreenReferences();
    }
}


wxString WinEDA_SchematicFrame::GetScreenDesc()
{
    wxString s = m_CurrentSheet->PathHumanReadable();

    return s;
}


void WinEDA_SchematicFrame::CreateScreens()
{
    /* creation des ecrans Sch , Lib  */
    if( g_RootSheet == NULL )
    {
        g_RootSheet = new DrawSheetStruct();
    }
    if( g_RootSheet->m_AssociatedScreen == NULL )
    {
        g_RootSheet->m_AssociatedScreen = new SCH_SCREEN();
        g_RootSheet->m_AssociatedScreen->m_RefCount++;
    }
    g_RootSheet->m_AssociatedScreen->m_FileName = g_DefaultSchematicFileName;
    g_RootSheet->m_AssociatedScreen->m_Date     = GenDate();
    m_CurrentSheet->Clear();
    m_CurrentSheet->Push( g_RootSheet );

    if( g_ScreenLib == NULL )
        g_ScreenLib = new SCH_SCREEN();
    g_ScreenLib->SetZoom( 4 );
    g_ScreenLib->m_UndoRedoCountMax = 10;
}


/*****************************************************************/
void WinEDA_SchematicFrame::OnCloseWindow( wxCloseEvent& Event )
/*****************************************************************/
{
    DrawSheetPath* sheet;
    wxConfig*      config = wxGetApp().m_EDA_Config;

    if( m_LibeditFrame ) // Can close component editor ?
    {
        if( !m_LibeditFrame->Close() )
            return;
    }

    EDA_SheetList SheetList;

    for( sheet = SheetList.GetFirst(); sheet != NULL; sheet = SheetList.GetNext() )
    {
        if( sheet->LastScreen() && sheet->LastScreen()->IsModify() )
            break;
    }

    if( sheet )
    {
        unsigned        ii;
        wxMessageDialog dialog( this,
                                _( "Schematic modified, Save before exit ?" ),
                                _( "Confirmation" ), wxYES_NO | wxCANCEL |
                                wxICON_EXCLAMATION | wxYES_DEFAULT );
        ii = dialog.ShowModal();

        switch( ii )
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

    for( sheet = SheetList.GetFirst(); sheet != NULL; sheet = SheetList.GetNext() )
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

    /* allof sub sheets are deleted, only the main sheet is useable */
    m_CurrentSheet->Clear();

    SaveSettings();

    if( config )
    {
        config->Write( MINI_DRAW_LINE_WIDTH_KEY, (long) g_DrawMinimunLineWidth );
        config->Write( MINI_PLOTPS_LINE_WIDTH_KEY,
                       (long) g_PlotPSMinimunLineWidth );
    }

    Destroy();
}


/*****************************************************************************
* Enable or disable some tools according to current conditions
*****************************************************************************/
void WinEDA_SchematicFrame::SetToolbars()
{
    if( m_HToolBar )
    {
        if( GetScreen() && GetScreen()->BlockLocate.m_Command == BLOCK_MOVE )
        {
            m_HToolBar->EnableTool( wxID_CUT, TRUE );
            m_HToolBar->EnableTool( wxID_COPY, TRUE );
        }
        else
        {
            m_HToolBar->EnableTool( wxID_CUT, FALSE );
            m_HToolBar->EnableTool( wxID_COPY, FALSE );
        }

        if( g_BlockSaveDataList )
            m_HToolBar->EnableTool( wxID_PASTE, TRUE );
        else
            m_HToolBar->EnableTool( wxID_PASTE, FALSE );

        wxMenuBar* menuBar = GetMenuBar();
        if( GetScreen() && GetScreen()->m_RedoList )
        {
            m_HToolBar->EnableTool( ID_SCHEMATIC_REDO, TRUE );
            menuBar->Enable( ID_SCHEMATIC_REDO, TRUE );
        }
        else
        {
            m_HToolBar->EnableTool( ID_SCHEMATIC_REDO, FALSE );
            menuBar->Enable( ID_SCHEMATIC_REDO, FALSE );
        }
        if( GetScreen() && GetScreen()->m_UndoList )
        {
            m_HToolBar->EnableTool( ID_SCHEMATIC_UNDO, TRUE );
            menuBar->Enable( ID_SCHEMATIC_UNDO, TRUE );
        }
        else
        {
            m_HToolBar->EnableTool( ID_SCHEMATIC_UNDO, FALSE );
            menuBar->Enable( ID_SCHEMATIC_UNDO, FALSE );
        }
    }

    if( m_OptionsToolBar )
    {
        m_OptionsToolBar->ToggleTool( ID_TB_OPTIONS_SHOW_GRID, m_Draw_Grid );
        m_OptionsToolBar->SetToolShortHelp( ID_TB_OPTIONS_SHOW_GRID,
                                           m_Draw_Grid ? _( "Grid not show" ) : _( "Show Grid" ) );

        m_OptionsToolBar->ToggleTool( ID_TB_OPTIONS_SELECT_UNIT_MM,
                                      g_UnitMetric == MILLIMETRE ? TRUE : FALSE );
        m_OptionsToolBar->ToggleTool( ID_TB_OPTIONS_SELECT_UNIT_INCH,
                                      g_UnitMetric == INCHES ? TRUE : FALSE );

        m_OptionsToolBar->ToggleTool( ID_TB_OPTIONS_SELECT_CURSOR,
                                      g_CursorShape );
        m_OptionsToolBar->ToggleTool( ID_TB_OPTIONS_HIDDEN_PINS, g_ShowAllPins );
        m_OptionsToolBar->SetToolShortHelp( ID_TB_OPTIONS_HIDDEN_PINS,
                                           g_ShowAllPins ? _( "No show Hidden Pins" ) : _(
                                               "Show Hidden Pins" ) );
        m_OptionsToolBar->ToggleTool( ID_TB_OPTIONS_BUS_WIRES_ORIENT,
                                      g_HVLines );
        m_OptionsToolBar->SetToolShortHelp( ID_TB_OPTIONS_BUS_WIRES_ORIENT,
                                           g_HVLines ? _(
                                               "Allows any direction for wires and busses" ) :
                                           _(
                                               "Allows horizontal and vertical wires and busses only" ) );
    }

    DisplayUnitsMsg();
}


int WinEDA_SchematicFrame::BestZoom()
{
    int    dx, dy, ii, jj;
    int    bestzoom;
    wxSize size;

    dx = GetScreen()->m_CurrentSheetDesc->m_Size.x;
    dy = GetScreen()->m_CurrentSheetDesc->m_Size.y;

    size     = DrawPanel->GetClientSize();
    ii       = dx / size.x;
    jj       = dy / size.y;
    bestzoom = MAX( ii, jj ) + 1;

    GetScreen()->SetZoom( ii );
    GetScreen()->m_Curseur.x = dx / 2;
    GetScreen()->m_Curseur.y = dy / 2;

    return bestzoom;
}


/**************************************************************/
void WinEDA_SchematicFrame::OnAnnotate( wxCommandEvent& event )
/**************************************************************/
{
    WinEDA_AnnotateFrame* dlg = new WinEDA_AnnotateFrame( this );

    dlg->ShowModal();
    dlg->Destroy();
}


/*********************************************************/
void WinEDA_SchematicFrame::OnErc( wxCommandEvent& event )
/*********************************************************/
{
    WinEDA_ErcFrame* dlg = new WinEDA_ErcFrame( this );

    dlg->ShowModal();
    dlg->Destroy();
}


/*******************************************************************/
void WinEDA_SchematicFrame::OnCreateNetlist( wxCommandEvent& event )
/*******************************************************************/
{
    int i;

    if( g_NetFormat <  NET_TYPE_PCBNEW )
        g_NetFormat = NET_TYPE_PCBNEW;

    do
    {
        WinEDA_NetlistFrame* dlg = new WinEDA_NetlistFrame( this );
        i = dlg->ShowModal();
        dlg->Destroy();
    } while( i == NET_PLUGIN_CHANGE );

    // If a plugin is removed or added, rebuild and reopen the new dialog
}


/**********************************************************************/
void WinEDA_SchematicFrame::OnCreateBillOfMaterials( wxCommandEvent& )
/**********************************************************************/
{
    WinEDA_Build_BOM_Frame* dlg = new WinEDA_Build_BOM_Frame( this );

    dlg->ShowModal();
    dlg->Destroy();
}


/*******************************************************************/
void WinEDA_SchematicFrame::OnFindItems( wxCommandEvent& event )
/*******************************************************************/
{
    this->DrawPanel->m_IgnoreMouseEvents = TRUE;
    WinEDA_FindFrame* dlg = new WinEDA_FindFrame( this );
    dlg->ShowModal();
    dlg->Destroy();
    this->DrawPanel->m_IgnoreMouseEvents = FALSE;
}


/***************************************************************/
void WinEDA_SchematicFrame::OnLoadFile( wxCommandEvent& event )
/***************************************************************/
{
    int i = event.GetId() - ID_LOAD_FILE_1;

    LoadOneEEProject( GetLastProject( i ).GetData(), false );
    SetToolbars();
}


/*******************************************************************/
void WinEDA_SchematicFrame::OnLoadStuffFile( wxCommandEvent& event )
/*******************************************************************/
{
    ReadInputStuffFile();
    DrawPanel->Refresh();
}


/****************************************************************/
void WinEDA_SchematicFrame::OnNewProject( wxCommandEvent& event )
/****************************************************************/
{
    LoadOneEEProject( wxEmptyString, true );
}


/*****************************************************************/
void WinEDA_SchematicFrame::OnLoadProject( wxCommandEvent& event )
/*****************************************************************/
{
    LoadOneEEProject( wxEmptyString, false );
}


/****************************************************************/
void WinEDA_SchematicFrame::OnOpenPcbnew( wxCommandEvent& event )
/****************************************************************/
{
    wxString Line = g_RootSheet->m_AssociatedScreen->m_FileName;

    if( Line != wxEmptyString )
    {
        AddDelimiterString( Line );
        ChangeFileNameExt( Line, wxEmptyString );
        ExecuteFile( this, PCBNEW_EXE, Line );
    }
    else
        ExecuteFile( this, PCBNEW_EXE );
}


/***************************************************************/
void WinEDA_SchematicFrame::OnOpenCvpcb( wxCommandEvent& event )
/***************************************************************/
{
    wxString Line = g_RootSheet->m_AssociatedScreen->m_FileName;

    if( Line != wxEmptyString )
    {
        AddDelimiterString( Line );
        ChangeFileNameExt( Line, wxEmptyString );
        ExecuteFile( this, CVPCB_EXE, Line );
    }
    else
        ExecuteFile( this, CVPCB_EXE );
}


/*************************************************************************/
void WinEDA_SchematicFrame::OnOpenLibraryViewer( wxCommandEvent& event )
/*************************************************************************/
{
    if( m_ViewlibFrame )
    {
        m_ViewlibFrame->Show( TRUE );
    }
    else
    {
        m_ViewlibFrame = new WinEDA_ViewlibFrame( this );
        m_ViewlibFrame->AdjustScrollBars();
    }
}


/*************************************************************************/
void WinEDA_SchematicFrame::OnOpenLibraryEditor( wxCommandEvent& event )
/*************************************************************************/
{
    if( m_LibeditFrame )
    {
        m_LibeditFrame->Show( TRUE );
    }
    else
    {
        m_LibeditFrame = new WinEDA_LibeditFrame( this,
                                                 wxT( "Library Editor" ),
                                                 wxPoint( -1, -1 ),
                                                 wxSize( 600, 400 ) );
        ActiveScreen = g_ScreenLib;
        m_LibeditFrame->AdjustScrollBars();
    }
}


void WinEDA_SchematicFrame::OnExit( wxCommandEvent& event )
{
    Close( true );
}
