/**
 * @file schframe.cpp
 */

#ifdef __GNUG__
#pragma implementation
#endif

#include "fctsys.h"
#include "appl_wxstruct.h"
#include "gr_basic.h"
#include "class_drawpanel.h"
#include "gestfich.h"

#include "general.h"
#include "protos.h"
#include "eeschema_id.h"
#include "netlist.h"
#include "lib_pin.h"
#include "class_library.h"
#include "wxEeschemaStruct.h"
#include "sch_component.h"

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

    EVT_MENU( ID_SAVE_PROJECT, SCH_EDIT_FRAME::OnSaveProject )
    EVT_MENU( ID_SAVE_ONE_SHEET, SCH_EDIT_FRAME::Save_File )
    EVT_MENU( ID_SAVE_ONE_SHEET_AS, SCH_EDIT_FRAME::Save_File )
    EVT_MENU( ID_GEN_PLOT_PS, SCH_EDIT_FRAME::ToPlot_PS )
    EVT_MENU( ID_GEN_PLOT_HPGL, SCH_EDIT_FRAME::ToPlot_HPGL )
    EVT_MENU( ID_GEN_PLOT_SVG, SCH_EDIT_FRAME::SVG_Print )
    EVT_MENU( ID_GEN_PLOT_DXF, SCH_EDIT_FRAME::ToPlot_DXF )
    EVT_MENU( ID_GEN_COPY_SHEET_TO_CLIPBOARD, EDA_DRAW_FRAME::CopyToClipboard )
    EVT_MENU( wxID_EXIT, SCH_EDIT_FRAME::OnExit )

    EVT_MENU( ID_POPUP_SCH_COPY_ITEM, SCH_EDIT_FRAME::OnCopySchematicItemRequest )

    EVT_MENU( ID_CONFIG_REQ, SCH_EDIT_FRAME::InstallConfigFrame )
    EVT_MENU( ID_CONFIG_SAVE, SCH_EDIT_FRAME::Process_Config )
    EVT_MENU( ID_CONFIG_READ, SCH_EDIT_FRAME::Process_Config )
    EVT_MENU_RANGE( ID_PREFERENCES_HOTKEY_START, ID_PREFERENCES_HOTKEY_END,
                    SCH_EDIT_FRAME::Process_Config )

    EVT_MENU( ID_COLORS_SETUP, SCH_EDIT_FRAME::OnColorConfig )
    EVT_TOOL( wxID_PREFERENCES, SCH_EDIT_FRAME::OnSetOptions )

    EVT_MENU_RANGE( ID_LANGUAGE_CHOICE, ID_LANGUAGE_CHOICE_END, SCH_EDIT_FRAME::SetLanguage )

    EVT_TOOL( ID_TO_LIBRARY, SCH_EDIT_FRAME::OnOpenLibraryEditor )
    EVT_TOOL( ID_POPUP_SCH_CALL_LIBEDIT_AND_LOAD_CMP, SCH_EDIT_FRAME::OnOpenLibraryEditor )
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
    EVT_TOOL( ID_POPUP_SCH_MOVE_ITEM, SCH_EDIT_FRAME::OnMoveItem )

    EVT_MENU( wxID_HELP, EDA_DRAW_FRAME::GetKicadHelp )
    EVT_MENU( wxID_INDEX, EDA_DRAW_FRAME::GetKicadHelp )
    EVT_MENU( wxID_ABOUT, EDA_BASE_FRAME::GetKicadAbout )

    // Tools and buttons for vertical toolbar.
    EVT_TOOL( ID_CANCEL_CURRENT_COMMAND, SCH_EDIT_FRAME::OnCancelCurrentCommand )
    EVT_TOOL( ID_NO_TOOL_SELECTED, SCH_EDIT_FRAME::OnSelectTool )
    EVT_TOOL_RANGE( ID_SCHEMATIC_VERTICAL_TOOLBAR_START, ID_SCHEMATIC_VERTICAL_TOOLBAR_END,
                    SCH_EDIT_FRAME::OnSelectTool )

    EVT_MENU( ID_CANCEL_CURRENT_COMMAND, SCH_EDIT_FRAME::OnCancelCurrentCommand )
    EVT_MENU_RANGE( ID_POPUP_START_RANGE, ID_POPUP_END_RANGE,
                    SCH_EDIT_FRAME::Process_Special_Functions )

    // Tools and buttons options toolbar
    EVT_TOOL( ID_TB_OPTIONS_HIDDEN_PINS,
                    SCH_EDIT_FRAME::OnSelectOptionToolbar )
    EVT_TOOL( ID_TB_OPTIONS_BUS_WIRES_ORIENT,
                    SCH_EDIT_FRAME::OnSelectOptionToolbar )

    EVT_MENU_RANGE( ID_POPUP_GENERAL_START_RANGE, ID_POPUP_GENERAL_END_RANGE,
                    SCH_EDIT_FRAME::Process_Special_Functions )
    EVT_MENU_RANGE( ID_POPUP_SCH_SELECT_UNIT1, ID_POPUP_SCH_SELECT_UNIT26,
                    SCH_EDIT_FRAME::OnSelectUnit )
    EVT_MENU_RANGE( ID_POPUP_SCH_CHANGE_TYPE_TEXT, ID_POPUP_SCH_CHANGE_TYPE_TEXT_TO_COMMENT,
                    SCH_EDIT_FRAME::OnConvertTextType )
    EVT_MENU_RANGE( ID_POPUP_SCH_MIRROR_X_CMP, ID_POPUP_SCH_ORIENT_NORMAL_CMP,
                    SCH_EDIT_FRAME::OnChangeComponentOrientation )

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
    m_Draw_Axis = FALSE;                // true to show axis
    m_Draw_Sheet_Ref = true;            // true to show sheet references
    m_CurrentSheet   = new SCH_SHEET_PATH();
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
    m_undoItem = NULL;

    CreateScreens();

    // Give an icon
    wxIcon icon;
    icon.CopyFromBitmap( KiBitmap( icon_eeschema_xpm ) );
    SetIcon( icon );

    m_itemToRepeat = NULL;

    /* Get config */
    LoadSettings();

    // Initialize grid id to a default value if not found in config or bad:
    if( (m_LastGridSizeId <= 0)
       || ( m_LastGridSizeId < (ID_POPUP_GRID_USER - ID_POPUP_GRID_LEVEL_1000) ) )
        m_LastGridSizeId = ID_POPUP_GRID_LEVEL_50 - ID_POPUP_GRID_LEVEL_1000;

    SetSize( m_FramePos.x, m_FramePos.y, m_FrameSize.x, m_FrameSize.y );

    if( DrawPanel )
        DrawPanel->m_Block_Enable = true;

    ReCreateMenuBar();
    ReCreateHToolbar();
    ReCreateVToolbar();
    ReCreateOptToolbar();

    /* Initialize print and page setup dialog settings. */
    m_pageSetupData.GetPrintData().SetQuality( wxPRINT_QUALITY_HIGH );
    m_pageSetupData.GetPrintData().SetOrientation( wxLANDSCAPE );

    m_auimgr.SetManagedWindow( this );

    EDA_PANEINFO horiz;
    horiz.HorizontalToolbarPane();

    EDA_PANEINFO vert;
    vert.VerticalToolbarPane();

    EDA_PANEINFO mesg;
    mesg.MessageToolbarPane();



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
        m_auimgr.AddPane( MsgPanel, wxAuiPaneInfo( mesg ).Name( wxT( "MsgPanel" ) ).Bottom().
            Layer(10) );

    m_auimgr.Update();

    // Now Drawpanel is sized, we can use BestZoom to show the component (if any)
    GetScreen()->SetZoom( BestZoom() );
}


SCH_EDIT_FRAME::~SCH_EDIT_FRAME()
{
    SetScreen( NULL );
    SAFE_DELETE( m_CurrentSheet );     // a SCH_SHEET_PATH, on the heap.
    SAFE_DELETE( m_undoItem );
    SAFE_DELETE( g_RootSheet );
    SAFE_DELETE( m_findReplaceData );
    CMP_LIBRARY::RemoveAllLibraries();
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
        SetScreen( g_RootSheet->GetScreen() );
    }

    g_RootSheet->GetScreen()->SetFileName( m_DefaultSchematicFileName );
    g_RootSheet->GetScreen()->m_Date     = GenDate();
    m_CurrentSheet->Clear();
    m_CurrentSheet->Push( g_RootSheet );

    if( GetScreen() == NULL )
        SetScreen( new SCH_SCREEN() );

    GetScreen()->SetZoom( 32.0 );
    GetScreen()->m_UndoRedoCountMax = 10;
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
        m_undoItem = aItem->Clone();

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
    if( m_LibeditFrame && !m_LibeditFrame->Close() )   // Can close component editor?
        return;

    if( m_ViewlibFrame && !m_ViewlibFrame->Close() )   // Can close component viewer?
        return;

    SCH_SHEET_LIST SheetList;

    if( SheetList.IsModified() )
    {
        wxMessageDialog dialog( this,
                                _( "Schematic modified, Save before exit?" ),
                                _( "Confirmation" ), wxYES_NO | wxCANCEL |
                                wxICON_EXCLAMATION | wxYES_DEFAULT );

        switch( dialog.ShowModal() )
        {
        case wxID_CANCEL:
            aEvent.Veto();
            return;

        case wxID_NO:
            break;

        case wxID_OK:
        case wxID_YES:
            wxCommandEvent tmp( ID_SAVE_PROJECT );
            OnSaveProject( tmp );
            break;
        }
    }

    SheetList.ClearModifyStatus();

    if( !g_RootSheet->GetScreen()->GetFileName().IsEmpty()
       && (g_RootSheet->GetScreen()->GetDrawItems() != NULL) )
        UpdateFileHistory( g_RootSheet->GetScreen()->GetFileName() );

    g_RootSheet->GetScreen()->Clear();

    /* all sub sheets are deleted, only the main sheet is usable */
    m_CurrentSheet->Clear();
    SaveSettings();
    Destroy();
}


double SCH_EDIT_FRAME::BestZoom()
{
    int    dx, dy;
    wxSize size;

    dx = GetScreen()->m_CurrentSheetDesc->m_Size.x;
    dy = GetScreen()->m_CurrentSheetDesc->m_Size.y;

    size = DrawPanel->GetClientSize();

    // Reserve no margin because best zoom shows the full page
    // and margins are already included in function that draws the sheet refernces
    double margin_scale_factor = 1.0;
    double zx =(double) dx / ( margin_scale_factor * (double)size.x );
    double zy = (double) dy / ( margin_scale_factor * (double)size.y );

    double bestzoom = MAX( zx, zy );

    GetScreen()->SetScrollCenterPosition( wxPoint( dx / 2, dy / 2 ) );

    return bestzoom;
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
#ifndef __WINDOWS__
        filename.Remove( 0, 1 );
#endif
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
}


void SCH_EDIT_FRAME::OnUpdatePaste( wxUpdateUIEvent& event )
{
    event.Enable( m_blockItems.GetCount() > 0 );
}


void SCH_EDIT_FRAME::OnUpdateBusOrientation( wxUpdateUIEvent& aEvent )
{
    wxString tool_tip = g_HVLines ?
                        _( "Draw wires and buses in any direction" ) :
                        _( "Draw horizontal and vertical wires and buses only" );

    aEvent.Check( g_HVLines );
    m_OptionsToolBar->SetToolShortHelp( ID_TB_OPTIONS_BUS_WIRES_ORIENT, tool_tip );
}


void SCH_EDIT_FRAME::OnUpdateHiddenPins( wxUpdateUIEvent& aEvent )
{
    wxString tool_tip = m_ShowAllPins ? _( "Do not show hidden pins" ) :
                        _( "Show hidden pins" );

    aEvent.Check( m_ShowAllPins );
    m_OptionsToolBar->SetToolShortHelp( ID_TB_OPTIONS_HIDDEN_PINS, tool_tip );
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
        NETLIST_DIALOG* dlg = new NETLIST_DIALOG( this );
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

    this->DrawPanel->m_IgnoreMouseEvents = true;

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
        LoadOneEEProject( fn, false );
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
        m_ViewlibFrame->Show( true );
    }
    else
    {
        m_ViewlibFrame = new LIB_VIEW_FRAME( this );
    }
}


void SCH_EDIT_FRAME::OnOpenLibraryEditor( wxCommandEvent& event )
{
    SCH_COMPONENT* component = NULL;
    if( event.GetId() == ID_POPUP_SCH_CALL_LIBEDIT_AND_LOAD_CMP )
    {
        SCH_ITEM* item = GetScreen()->GetCurItem();
        if( (item == NULL) || (item->GetFlags() != 0) ||
            ( item->Type() != SCH_COMPONENT_T ) )
        {
            wxMessageBox( _("Error: not a component or no component" ) );
            return;
        }

        component = (SCH_COMPONENT*) item;
    }

    if( m_LibeditFrame )
    {
        if( m_LibeditFrame->IsIconized() )
             m_LibeditFrame->Iconize( false );
        m_LibeditFrame->Raise();
    }
    else
        m_LibeditFrame = new LIB_EDIT_FRAME( this,
                                             wxT( "Library Editor" ),
                                             wxPoint( -1, -1 ),
                                             wxSize( 600, 400 ) );
    if( component )
    {
        LIB_ALIAS* entry = CMP_LIBRARY::FindLibraryEntry( component->GetLibName() );
        if( entry == NULL )     // Should not occur
            return;
        CMP_LIBRARY* library = entry->GetLibrary();
        m_LibeditFrame->LoadComponentAndSelectLib( entry, library );
    }
}


void SCH_EDIT_FRAME::OnExit( wxCommandEvent& event )
{
    Close( true );
}


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


void SCH_EDIT_FRAME::SVG_Print( wxCommandEvent& event )
{
    DIALOG_SVG_PRINT frame( this );

    frame.ShowModal();
}

/*
 * Function PrintPage (virtual)
 * Previously used to print a page,
 * but now only used to plot/print the current sheet to the clipboard
 * @param aDC = wxDC given by the calling print function
 * @param aPrintMask = not used here
 * @param aPrintMirrorMode = not used here (Set when printing in mirror mode)
 * @param aData = a pointer on an auxiliary data (not used here)
 */
void SCH_EDIT_FRAME::PrintPage( wxDC* aDC, int aPrintMask, bool aPrintMirrorMode, void* aData )
{
    GetScreen()->Draw( DrawPanel, aDC, GR_DEFAULT_DRAWMODE );
    TraceWorkSheet( aDC, GetScreen(), g_DrawDefaultLineThickness );
}


void SCH_EDIT_FRAME::OnSelectItem( wxCommandEvent& aEvent )
{
    int id = aEvent.GetId();
    int index = id - ID_SELECT_ITEM_START;

    if( (id >= ID_SELECT_ITEM_START && id <= ID_SELECT_ITEM_END)
        && (index >= 0 && index < m_collectedItems.GetCount()) )
    {
        SCH_ITEM* item = m_collectedItems[index];
        DrawPanel->m_AbortRequest = false;
        GetScreen()->SetCurItem( item );
    }
}
