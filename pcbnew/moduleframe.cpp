/************************************************************/
/* moduleframe.cpp - Footprint (module) editor main window. */
/************************************************************/

#include "fctsys.h"
#include "appl_wxstruct.h"
#include "common.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "pcbnew.h"
#include "wxPcbStruct.h"
#include "module_editor_frame.h"
#include "bitmaps.h"
#include "protos.h"
#include "pcbnew_id.h"
#include "hotkeys.h"
#include "dialog_helpers.h"

#include "3d_viewer.h"


static PCB_SCREEN* s_screenModule = NULL;   // the PCB_SCREEN used by the
                                            // footprint editor

// Design setting for the module editor:
static BOARD_DESIGN_SETTINGS s_ModuleEditorDesignSetting;

wxString WinEDA_ModuleEditFrame::m_CurrentLib = wxEmptyString;

/********************************/
/* class WinEDA_ModuleEditFrame */
/********************************/
BEGIN_EVENT_TABLE( WinEDA_ModuleEditFrame, PCB_BASE_FRAME )
    EVT_MENU_RANGE( ID_POPUP_PCB_ITEM_SELECTION_START, ID_POPUP_PCB_ITEM_SELECTION_END,
                    PCB_BASE_FRAME::ProcessItemSelection )
    EVT_CLOSE( WinEDA_ModuleEditFrame::OnCloseWindow )
    EVT_MENU( wxID_EXIT, WinEDA_ModuleEditFrame::CloseModuleEditor )

    EVT_SIZE( WinEDA_ModuleEditFrame::OnSize )

    EVT_COMBOBOX( ID_ON_ZOOM_SELECT, WinEDA_ModuleEditFrame::OnSelectZoom )
    EVT_COMBOBOX( ID_ON_GRID_SELECT, WinEDA_ModuleEditFrame::OnSelectGrid )

    EVT_TOOL( ID_MODEDIT_SELECT_CURRENT_LIB, WinEDA_ModuleEditFrame::Process_Special_Functions )
    EVT_TOOL( ID_MODEDIT_SAVE_LIBMODULE, WinEDA_ModuleEditFrame::Process_Special_Functions )
    EVT_TOOL( ID_MODEDIT_DELETE_PART, WinEDA_ModuleEditFrame::Process_Special_Functions )
    EVT_TOOL( ID_MODEDIT_NEW_MODULE, WinEDA_ModuleEditFrame::Process_Special_Functions )
    EVT_TOOL( ID_MODEDIT_LOAD_MODULE, WinEDA_ModuleEditFrame::Process_Special_Functions )
    EVT_TOOL( ID_MODEDIT_IMPORT_PART, WinEDA_ModuleEditFrame::Process_Special_Functions )
    EVT_TOOL( ID_MODEDIT_EXPORT_PART, WinEDA_ModuleEditFrame::Process_Special_Functions )
    EVT_TOOL( ID_MODEDIT_CREATE_NEW_LIB_AND_SAVE_CURRENT_PART,
              WinEDA_ModuleEditFrame::Process_Special_Functions )
    EVT_TOOL( ID_MODEDIT_SHEET_SET, WinEDA_ModuleEditFrame::Process_Special_Functions )
    EVT_TOOL( wxID_PRINT, WinEDA_ModuleEditFrame::ToPrinter )
    EVT_TOOL( ID_MODEDIT_LOAD_MODULE, WinEDA_ModuleEditFrame::Process_Special_Functions )
    EVT_TOOL( ID_MODEDIT_CHECK, WinEDA_ModuleEditFrame::Process_Special_Functions )
    EVT_TOOL( ID_MODEDIT_PAD_SETTINGS, WinEDA_ModuleEditFrame::Process_Special_Functions )
    EVT_TOOL( ID_MODEDIT_LOAD_MODULE_FROM_BOARD, WinEDA_ModuleEditFrame::LoadModuleFromBoard )
    EVT_TOOL( ID_MODEDIT_INSERT_MODULE_IN_BOARD, WinEDA_ModuleEditFrame::Process_Special_Functions )
    EVT_TOOL( ID_MODEDIT_UPDATE_MODULE_IN_BOARD, WinEDA_ModuleEditFrame::Process_Special_Functions )
    EVT_TOOL( ID_MODEDIT_EDIT_MODULE_PROPERTIES, WinEDA_ModuleEditFrame::Process_Special_Functions )
    EVT_TOOL( wxID_UNDO, WinEDA_ModuleEditFrame::GetComponentFromUndoList )
    EVT_TOOL( wxID_REDO, WinEDA_ModuleEditFrame::GetComponentFromRedoList )

    // Vertical tool bar button click event handler.
    EVT_TOOL( ID_NO_TOOL_SELECTED, WinEDA_ModuleEditFrame::OnVerticalToolbar )
    EVT_TOOL_RANGE( ID_MODEDIT_PAD_TOOL, ID_MODEDIT_PLACE_GRID_COORD,
                    WinEDA_ModuleEditFrame::OnVerticalToolbar )

    // Options Toolbar
    EVT_TOOL( ID_TB_OPTIONS_SHOW_PADS_SKETCH,
                    WinEDA_ModuleEditFrame::OnSelectOptionToolbar )
    EVT_TOOL( ID_TB_OPTIONS_SHOW_VIAS_SKETCH,
                    WinEDA_ModuleEditFrame::OnSelectOptionToolbar )
    EVT_TOOL( ID_TB_OPTIONS_SHOW_MODULE_TEXT_SKETCH,
                    WinEDA_ModuleEditFrame::OnSelectOptionToolbar )
    EVT_TOOL( ID_TB_OPTIONS_SHOW_MODULE_EDGE_SKETCH,
                    WinEDA_ModuleEditFrame::OnSelectOptionToolbar )

    // popup commands
    EVT_MENU_RANGE( ID_POPUP_PCB_START_RANGE, ID_POPUP_PCB_END_RANGE,
                    WinEDA_ModuleEditFrame::Process_Special_Functions )

    EVT_MENU_RANGE( ID_POPUP_GENERAL_START_RANGE, ID_POPUP_GENERAL_END_RANGE,
                    WinEDA_ModuleEditFrame::Process_Special_Functions )

    // Module transformations
    EVT_MENU( ID_MODEDIT_MODULE_ROTATE, WinEDA_ModuleEditFrame::Process_Special_Functions )
    EVT_MENU( ID_MODEDIT_MODULE_MIRROR, WinEDA_ModuleEditFrame::Process_Special_Functions )

    EVT_MENU( ID_PCB_DRAWINGS_WIDTHS_SETUP, WinEDA_ModuleEditFrame::Process_Special_Functions )
    EVT_MENU( ID_PCB_PAD_SETUP, WinEDA_ModuleEditFrame::Process_Special_Functions )
    EVT_MENU( ID_PCB_USER_GRID_SETUP, PCB_EDIT_FRAME::Process_Special_Functions )

    // Menu 3D Frame
    EVT_MENU( ID_MENU_PCB_SHOW_3D_FRAME, WinEDA_ModuleEditFrame::Show3D_Frame )

    EVT_UPDATE_UI( ID_MODEDIT_DELETE_PART, WinEDA_ModuleEditFrame::OnUpdateLibSelected )
    EVT_UPDATE_UI( ID_MODEDIT_EXPORT_PART, WinEDA_ModuleEditFrame::OnUpdateModuleSelected )
    EVT_UPDATE_UI( ID_MODEDIT_CREATE_NEW_LIB_AND_SAVE_CURRENT_PART,
                   WinEDA_ModuleEditFrame::OnUpdateModuleSelected )
    EVT_UPDATE_UI( ID_MODEDIT_SAVE_LIBMODULE,
                   WinEDA_ModuleEditFrame::OnUpdateLibAndModuleSelected )
    EVT_UPDATE_UI( ID_MODEDIT_LOAD_MODULE_FROM_BOARD,
                   WinEDA_ModuleEditFrame::OnUpdateLoadModuleFromBoard )
    EVT_UPDATE_UI( ID_MODEDIT_INSERT_MODULE_IN_BOARD,
                   WinEDA_ModuleEditFrame::OnUpdateInsertModuleInBoard )
    EVT_UPDATE_UI( ID_MODEDIT_UPDATE_MODULE_IN_BOARD,
                   WinEDA_ModuleEditFrame::OnUpdateReplaceModuleInBoard )
    EVT_UPDATE_UI( ID_NO_TOOL_SELECTED, WinEDA_ModuleEditFrame::OnUpdateVerticalToolbar )
    EVT_UPDATE_UI_RANGE( ID_MODEDIT_PAD_TOOL, ID_MODEDIT_PLACE_GRID_COORD,
                         WinEDA_ModuleEditFrame::OnUpdateVerticalToolbar )

END_EVENT_TABLE()


WinEDA_ModuleEditFrame::WinEDA_ModuleEditFrame( wxWindow*       father,
                                                const wxString& title,
                                                const wxPoint&  pos,
                                                const wxSize&   size,
                                                long            style ) :
    PCB_BASE_FRAME( father, MODULE_EDITOR_FRAME, wxEmptyString, pos, size, style )
{
    m_FrameName = wxT( "ModEditFrame" );
    m_Draw_Sheet_Ref = false;   // true to show the frame references
    m_Draw_Axis = true;         // true to show X and Y axis on screen
    m_Draw_Grid_Axis = true;    // show the grid origin axis
    m_HotkeysZoomAndGridList = g_Module_Editor_Hokeys_Descr;

    // Give an icon
    SetIcon( wxICON( icon_modedit ) );

    UpdateTitle();

    if( g_ModuleEditor_Pcb == NULL )
        g_ModuleEditor_Pcb = new BOARD( NULL, this );

    SetBoard( g_ModuleEditor_Pcb );
    GetBoard()->m_PcbFrame = this;

    if( s_screenModule == NULL )
        s_screenModule = new PCB_SCREEN();

    SetScreen( s_screenModule );
    GetBoard()->SetBoardDesignSettings( &s_ModuleEditorDesignSetting );
    GetScreen()->SetCurItem( NULL );
    LoadSettings();

    GetScreen()->AddGrid( m_UserGridSize, m_UserGridUnit, ID_POPUP_GRID_USER );
    GetScreen()->SetGrid( ID_POPUP_GRID_LEVEL_1000 + m_LastGridSizeId  );

    SetSize( m_FramePos.x, m_FramePos.y, m_FrameSize.x, m_FrameSize.y );
    ReCreateMenuBar();
    ReCreateHToolbar();
    ReCreateAuxiliaryToolbar();
    ReCreateVToolbar();
    ReCreateOptToolbar();

    if( DrawPanel )
        DrawPanel->m_Block_Enable = true;

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

    m_auimgr.AddPane( m_HToolBar,
                      wxAuiPaneInfo( horiz ).Name( wxT( "m_HToolBar" ) ).Top(). Row( 0 ) );

    m_auimgr.AddPane( m_AuxiliaryToolBar,
                      wxAuiPaneInfo( horiz ).Name( wxT( "m_AuxiliaryToolBar" ) ).Top().Row( 1 ) );

    m_auimgr.AddPane( m_VToolBar,
                      wxAuiPaneInfo( vert ).Name( wxT( "m_VToolBar" ) ).Right() );

    m_auimgr.AddPane( m_OptionsToolBar,
                      wxAuiPaneInfo( vert ).Name( wxT( "m_OptionsToolBar" ) ). Left() );

    m_auimgr.AddPane( DrawPanel,
                      wxAuiPaneInfo().Name( wxT( "DrawFrame" ) ).CentrePane() );

    m_auimgr.AddPane( MsgPanel,
                      wxAuiPaneInfo( horiz ).Name( wxT( "MsgPanel" ) ).Bottom() );

    m_auimgr.Update();
}


WinEDA_ModuleEditFrame::~WinEDA_ModuleEditFrame()
{
    /* g_ModuleEditor_Pcb and its corresponding PCB_SCREEN are not deleted
     * here, because if we reopen the Footprint editor, we expect to find
     * the last edited item
     */
    SetScreen( NULL );  /* Do not delete (by the destructor of EDA_DRAW_FRAME) the
                         * PCB_SCREEN handling g_ModuleEditor_Pcb
                         */

    PCB_BASE_FRAME* frame = (PCB_BASE_FRAME*) GetParent();
    frame->m_ModuleEditFrame = NULL;
}


void WinEDA_ModuleEditFrame::OnCloseWindow( wxCloseEvent& Event )
{
    if( GetScreen()->IsModify() )
    {
        if( !IsOK( this, _( "Module Editor: Module modified! Continue?" ) ) )
        {
            Event.Veto(); return;
        }
    }

    SaveSettings();
    Destroy();
}


void WinEDA_ModuleEditFrame::CloseModuleEditor( wxCommandEvent& Event )
{
    Close();
}


void WinEDA_ModuleEditFrame::OnUpdateVerticalToolbar( wxUpdateUIEvent& aEvent )
{
    aEvent.Enable( GetBoard()->m_Modules != NULL );

    if( aEvent.GetEventObject() == m_VToolBar )
        aEvent.Check( GetToolId() == aEvent.GetId() );
}


void WinEDA_ModuleEditFrame::OnUpdateLibSelected( wxUpdateUIEvent& aEvent )
{
    aEvent.Enable( m_CurrentLib != wxEmptyString );
}


void WinEDA_ModuleEditFrame::OnUpdateModuleSelected( wxUpdateUIEvent& aEvent )
{
    aEvent.Enable( GetBoard()->m_Modules != NULL );
}


void WinEDA_ModuleEditFrame::OnUpdateLibAndModuleSelected( wxUpdateUIEvent& aEvent )
{
    aEvent.Enable( ( m_CurrentLib != wxEmptyString ) && ( GetBoard()->m_Modules != NULL ) );
}


void WinEDA_ModuleEditFrame::OnUpdateLoadModuleFromBoard( wxUpdateUIEvent& aEvent )
{
    PCB_BASE_FRAME* frame = (PCB_BASE_FRAME*) GetParent();

    aEvent.Enable( frame->GetBoard()->m_Modules != NULL );
}


void WinEDA_ModuleEditFrame::OnUpdateInsertModuleInBoard( wxUpdateUIEvent& aEvent )
{
    PCB_BASE_FRAME* frame = (PCB_BASE_FRAME*) GetParent();

    MODULE* module_in_edit = GetBoard()->m_Modules;
    bool canInsert = ( module_in_edit && !module_in_edit->m_Link );

    // If the source was deleted, the module can inserted but not updated in the board.
    if( module_in_edit && module_in_edit->m_Link ) // this is not a new module
    {
        BOARD*  mainpcb = frame->GetBoard();
        MODULE* source_module = mainpcb->m_Modules;

        // search if the source module was not deleted:
        for( ; source_module != NULL; source_module = source_module->Next() )
        {
            if( module_in_edit->m_Link == source_module->m_TimeStamp )
                break;
        }

        canInsert = ( source_module == NULL );
    }

    aEvent.Enable( canInsert );
}


void WinEDA_ModuleEditFrame::OnUpdateReplaceModuleInBoard( wxUpdateUIEvent& aEvent )
{
    PCB_BASE_FRAME* frame = (PCB_BASE_FRAME*) GetParent();

    MODULE* module_in_edit = GetBoard()->m_Modules;
    bool canReplace = ( module_in_edit && module_in_edit->m_Link );

    if( module_in_edit && module_in_edit->m_Link ) // this is not a new module
    {
        BOARD*  mainpcb = frame->GetBoard();
        MODULE* source_module = mainpcb->m_Modules;

        // search if the source module was not deleted:
        for( ; source_module != NULL; source_module = source_module->Next() )
        {
            if( module_in_edit->m_Link == source_module->m_TimeStamp )
                break;
        }

        canReplace = ( source_module != NULL );
    }

    aEvent.Enable( canReplace );
}


/**
 * Display 3D frame of footprint (module) being edited.
 */
void WinEDA_ModuleEditFrame::Show3D_Frame( wxCommandEvent& event )
{
    if( m_Draw3DFrame )
    {
        // Raising the window does not show the window on Windows if iconized.
        // This should work on any platform.
        if( m_Draw3DFrame->IsIconized() )
             m_Draw3DFrame->Iconize( false );
        m_Draw3DFrame->Raise();

        // Raising the window does not set the focus on Linux.  This should work on any platform.
        if( wxWindow::FindFocus() != m_Draw3DFrame )
            m_Draw3DFrame->SetFocus();

        return;
    }

    m_Draw3DFrame = new EDA_3D_FRAME( this, _( "3D Viewer" ) );
    m_Draw3DFrame->Show( true );
}


void WinEDA_ModuleEditFrame::GeneralControl( wxDC* aDC, const wxPoint& aPosition, int aHotKey )
{
    wxRealPoint gridSize;
    wxPoint     oldpos;
    wxPoint     pos = aPosition;

    pos = GetScreen()->GetNearestGridPosition( aPosition );
    oldpos = GetScreen()->GetCrossHairPosition();
    gridSize = GetScreen()->GetGridSize();

    switch( aHotKey )
    {
    case WXK_NUMPAD8:
    case WXK_UP:
        pos.y -= wxRound( gridSize.y );
        DrawPanel->MoveCursor( pos );
        break;

    case WXK_NUMPAD2:
    case WXK_DOWN:
        pos.y += wxRound( gridSize.y );
        DrawPanel->MoveCursor( pos );
        break;

    case WXK_NUMPAD4:
    case WXK_LEFT:
        pos.x -= wxRound( gridSize.x );
        DrawPanel->MoveCursor( pos );
        break;

    case WXK_NUMPAD6:
    case WXK_RIGHT:
        pos.x += wxRound( gridSize.x );
        DrawPanel->MoveCursor( pos );
        break;

    default:
        break;
    }

    GetScreen()->SetCrossHairPosition( pos );

    if( oldpos != GetScreen()->GetCrossHairPosition() )
    {
        pos = GetScreen()->GetCrossHairPosition();
        GetScreen()->SetCrossHairPosition( oldpos );
        DrawPanel->CrossHairOff( aDC );
        GetScreen()->SetCrossHairPosition( pos );
        DrawPanel->CrossHairOn( aDC );

        if( DrawPanel->IsMouseCaptured() )
        {
#ifdef USE_WX_OVERLAY
            wxDCOverlay oDC( DrawPanel->m_overlay, (wxWindowDC*)aDC );
            oDC.Clear();
            DrawPanel->m_mouseCaptureCallback( DrawPanel, aDC, aPosition, false );
#else
            DrawPanel->m_mouseCaptureCallback( DrawPanel, aDC, aPosition, true );
#endif
        }
#ifdef USE_WX_OVERLAY
        else
            DrawPanel->m_overlay.Reset();
#endif
    }

    if( aHotKey )
    {
        OnHotKey( aDC, aHotKey, aPosition );
    }

    UpdateStatusBar();
}


/**
 * Function OnModify() (virtual)
 * Must be called after a change
 * in order to set the "modify" flag of the current screen
 * and prepare, if needed the refresh of the 3D frame showing the footprint
 * do not forget to call the basic OnModify function to update auxiliary info
 */
void WinEDA_ModuleEditFrame::OnModify()
{
    PCB_BASE_FRAME::OnModify();

    if( m_Draw3DFrame )
        m_Draw3DFrame->ReloadRequest();
}


void WinEDA_ModuleEditFrame::UpdateTitle()
{
    if( m_CurrentLib.IsEmpty() )
        SetTitle( _( "Module Editor (no active library)" ) );
    else
        SetTitle( _( "Module Editor (active library: " ) + m_CurrentLib + wxT( ")" ) );
}
