/************************************************************/
/* moduleframe.cpp - Footprint (module) editor main window. */
/************************************************************/

#ifdef __GNUG__
#pragma implementation
#endif

#include "fctsys.h"
#include "appl_wxstruct.h"
#include "common.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "pcbnew.h"
#include "wxPcbStruct.h"
#include "bitmaps.h"
#include "protos.h"
#include "pcbnew_id.h"

#include "3d_viewer.h"


static PCB_SCREEN* s_screenModule = NULL;   // the PCB_SCREEN used by the
                                            // footprint editor


/********************************/
/* class WinEDA_ModuleEditFrame */
/********************************/
BEGIN_EVENT_TABLE( WinEDA_ModuleEditFrame, WinEDA_BasePcbFrame )
    EVT_MENU_RANGE( ID_POPUP_PCB_ITEM_SELECTION_START,
                    ID_POPUP_PCB_ITEM_SELECTION_END,
                    WinEDA_BasePcbFrame::ProcessItemSelection )
    EVT_CLOSE( WinEDA_ModuleEditFrame::OnCloseWindow )
    EVT_SIZE( WinEDA_ModuleEditFrame::OnSize )

    EVT_KICAD_CHOICEBOX( ID_ON_ZOOM_SELECT,
                         WinEDA_ModuleEditFrame::OnSelectZoom )
    EVT_KICAD_CHOICEBOX( ID_ON_GRID_SELECT,
                         WinEDA_ModuleEditFrame::OnSelectGrid )

    EVT_TOOL_RANGE( ID_ZOOM_IN, ID_ZOOM_PAGE, WinEDA_ModuleEditFrame::OnZoom )

    EVT_TOOL( ID_MODEDIT_SELECT_CURRENT_LIB,
              WinEDA_ModuleEditFrame::Process_Special_Functions )
    EVT_TOOL( ID_MODEDIT_SAVE_LIBMODULE,
              WinEDA_ModuleEditFrame::Process_Special_Functions )
    EVT_TOOL( ID_MODEDIT_DELETE_PART,
              WinEDA_ModuleEditFrame::Process_Special_Functions )
    EVT_TOOL( ID_MODEDIT_NEW_MODULE,
              WinEDA_ModuleEditFrame::Process_Special_Functions )
    EVT_TOOL( ID_MODEDIT_LOAD_MODULE,
              WinEDA_ModuleEditFrame::Process_Special_Functions )
    EVT_TOOL( ID_MODEDIT_IMPORT_PART,
              WinEDA_ModuleEditFrame::Process_Special_Functions )
    EVT_TOOL( ID_MODEDIT_EXPORT_PART,
              WinEDA_ModuleEditFrame::Process_Special_Functions )
    EVT_TOOL( ID_MODEDIT_CREATE_NEW_LIB_AND_SAVE_CURRENT_PART,
              WinEDA_ModuleEditFrame::Process_Special_Functions )
    EVT_TOOL( ID_MODEDIT_SHEET_SET,
              WinEDA_ModuleEditFrame::Process_Special_Functions )
    EVT_TOOL( ID_GEN_PRINT, WinEDA_ModuleEditFrame::ToPrinter )
    EVT_TOOL( ID_MODEDIT_LOAD_MODULE,
              WinEDA_ModuleEditFrame::Process_Special_Functions )
    EVT_TOOL( ID_MODEDIT_CHECK,
              WinEDA_ModuleEditFrame::Process_Special_Functions )
    EVT_TOOL( ID_MODEDIT_PAD_SETTINGS,
              WinEDA_ModuleEditFrame::Process_Special_Functions )
    EVT_TOOL( ID_MODEDIT_LOAD_MODULE_FROM_BOARD,
              WinEDA_ModuleEditFrame::LoadModuleFromBoard )
    EVT_TOOL( ID_MODEDIT_INSERT_MODULE_IN_BOARD,
              WinEDA_ModuleEditFrame::Process_Special_Functions )
    EVT_TOOL( ID_MODEDIT_UPDATE_MODULE_IN_BOARD,
              WinEDA_ModuleEditFrame::Process_Special_Functions )
    EVT_TOOL( ID_MODEDIT_EDIT_MODULE_PROPERTIES,
              WinEDA_ModuleEditFrame::Process_Special_Functions )
    EVT_TOOL( ID_MODEDIT_UNDO,
              WinEDA_ModuleEditFrame::GetComponentFromUndoList )
    EVT_TOOL( ID_MODEDIT_REDO,
              WinEDA_ModuleEditFrame::GetComponentFromRedoList )

    // Vertical toolbar (left click):
    EVT_TOOL( ID_NO_SELECT_BUTT,
              WinEDA_ModuleEditFrame::Process_Special_Functions )
    EVT_TOOL( ID_MODEDIT_ADD_PAD,
              WinEDA_ModuleEditFrame::Process_Special_Functions )
    EVT_TOOL( ID_PCB_ARC_BUTT,
              WinEDA_ModuleEditFrame::Process_Special_Functions )
    EVT_TOOL( ID_PCB_CIRCLE_BUTT,
              WinEDA_ModuleEditFrame::Process_Special_Functions )
    EVT_TOOL( ID_PCB_ADD_TEXT_BUTT,
              WinEDA_ModuleEditFrame::Process_Special_Functions )
    EVT_TOOL( ID_PCB_ADD_LINE_BUTT,
              WinEDA_ModuleEditFrame::Process_Special_Functions )
    EVT_TOOL( ID_MODEDIT_DELETE_ITEM_BUTT,
              WinEDA_ModuleEditFrame::Process_Special_Functions )
    EVT_TOOL( ID_MODEDIT_PLACE_ANCHOR,
              WinEDA_ModuleEditFrame::Process_Special_Functions )

    // Vertical toolbar (right click):
    EVT_TOOL_RCLICKED( ID_MODEDIT_ADD_PAD,
                       WinEDA_ModuleEditFrame::ToolOnRightClick )
    EVT_TOOL_RCLICKED( ID_TRACK_BUTT,
                       WinEDA_ModuleEditFrame::ToolOnRightClick )
    EVT_TOOL_RCLICKED( ID_PCB_CIRCLE_BUTT,
                       WinEDA_ModuleEditFrame::ToolOnRightClick )
    EVT_TOOL_RCLICKED( ID_PCB_ARC_BUTT,
                       WinEDA_ModuleEditFrame::ToolOnRightClick )
    EVT_TOOL_RCLICKED( ID_PCB_ADD_TEXT_BUTT,
                       WinEDA_ModuleEditFrame::ToolOnRightClick )
    EVT_TOOL_RCLICKED( ID_PCB_ADD_LINE_BUTT,
                       WinEDA_ModuleEditFrame::ToolOnRightClick )
    EVT_TOOL_RCLICKED( ID_PCB_COTATION_BUTT,
                       WinEDA_ModuleEditFrame::ToolOnRightClick )

    // Options Toolbar
    EVT_TOOL_RANGE( ID_TB_OPTIONS_START, ID_TB_OPTIONS_END,
                    WinEDA_ModuleEditFrame::OnSelectOptionToolbar )

    // popup commands
    EVT_MENU_RANGE( ID_POPUP_PCB_START_RANGE, ID_POPUP_PCB_END_RANGE,
                    WinEDA_ModuleEditFrame::Process_Special_Functions )

    EVT_MENU_RANGE( ID_POPUP_GENERAL_START_RANGE, ID_POPUP_GENERAL_END_RANGE,
                    WinEDA_ModuleEditFrame::Process_Special_Functions )

    // Module transformations
    EVT_MENU( ID_MODEDIT_MODULE_ROTATE,
              WinEDA_ModuleEditFrame::Process_Special_Functions )
    EVT_MENU( ID_MODEDIT_MODULE_MIRROR,
              WinEDA_ModuleEditFrame::Process_Special_Functions )

    EVT_MENU( ID_PCB_DRAWINGS_WIDTHS_SETUP,
              WinEDA_ModuleEditFrame::Process_Special_Functions )
    EVT_MENU( ID_PCB_PAD_SETUP,
              WinEDA_ModuleEditFrame::Process_Special_Functions )
    EVT_MENU( ID_PCB_USER_GRID_SETUP,
              WinEDA_PcbFrame::Process_Special_Functions )

    // Menu 3D Frame
    EVT_MENU( ID_MENU_PCB_SHOW_3D_FRAME, WinEDA_ModuleEditFrame::Show3D_Frame )

END_EVENT_TABLE()


WinEDA_ModuleEditFrame::WinEDA_ModuleEditFrame( wxWindow*       father,
                                                const wxString& title,
                                                const wxPoint&  pos,
                                                const wxSize&   size,
                                                long            style ) :
    WinEDA_BasePcbFrame( father, MODULE_EDITOR_FRAME,
                         wxEmptyString, pos, size, style )
{
    m_FrameName = wxT( "ModEditFrame" );
    m_Draw_Sheet_Ref = false;   // true to show the frame references
    m_Draw_Axis = true;         // true to show X and Y axis on screen

    // Give an icon
    SetIcon( wxICON( icon_modedit ) );

    SetTitle( wxT( "Module Editor (lib: " ) + m_CurrentLib + wxT( ")" ) );

    if( g_ModuleEditor_Pcb == NULL )
        g_ModuleEditor_Pcb = new BOARD( NULL, this );

    SetBoard( g_ModuleEditor_Pcb );
    GetBoard()->m_PcbFrame = this;

    if( s_screenModule == NULL )
        s_screenModule = new PCB_SCREEN();
    SetBaseScreen( s_screenModule );
    ActiveScreen = GetScreen();
    GetScreen()->SetCurItem( NULL );
    LoadSettings();

    GetScreen()->AddGrid( m_UserGridSize, m_UserGridUnits, ID_POPUP_GRID_USER );
    GetScreen()->SetGrid( ID_POPUP_GRID_LEVEL_1000 + m_LastGridSizeId  );

    SetSize( m_FramePos.x, m_FramePos.y, m_FrameSize.x, m_FrameSize.y );
    ReCreateMenuBar();
    ReCreateHToolbar();
    ReCreateAuxiliaryToolbar();
    ReCreateVToolbar();
    ReCreateOptToolbar();

    if( DrawPanel )
        DrawPanel->m_Block_Enable = TRUE;
#if defined(KICAD_AUIMANAGER)
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
                      wxAuiPaneInfo( horiz ).Name( wxT( "m_HToolBar" ) ).Top().
                      Row( 0 ) );

    m_auimgr.AddPane( m_AuxiliaryToolBar,
                     wxAuiPaneInfo( horiz ).Name( wxT( "m_AuxiliaryToolBar" ) ).
                     Top().Row( 1 ) );

    m_auimgr.AddPane( m_VToolBar,
                     wxAuiPaneInfo( vert ).Name( wxT( "m_VToolBar" ) ).Right() );

    m_auimgr.AddPane( m_OptionsToolBar,
                     wxAuiPaneInfo( vert ).Name( wxT( "m_OptionsToolBar" ) ).
                     Left() );

    m_auimgr.AddPane( DrawPanel,
                      wxAuiPaneInfo().Name( wxT( "DrawFrame" ) ).CentrePane() );

    m_auimgr.AddPane( MsgPanel,
                      wxAuiPaneInfo( horiz ).Name( wxT( "MsgPanel" ) ).Bottom() );

    m_auimgr.Update();
#endif
}


WinEDA_ModuleEditFrame::~WinEDA_ModuleEditFrame()
{
    /* g_ModuleEditor_Pcb and its corresponding PCB_SCREEN are not deleted
     * here, because if we reopen the Footprint editor, we expect to find
     * the last edited item
     */
    SetBaseScreen( NULL );  /* Do not delete (by the destructor of
                             * WinEDA_DrawFrame) the PCB_SCREEN handling
                             * g_ModuleEditor_Pcb
                             */

    WinEDA_BasePcbFrame* frame = (WinEDA_BasePcbFrame*) GetParent();
    frame->m_ModuleEditFrame = NULL;
    ActiveScreen = frame->GetScreen();
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


void WinEDA_ModuleEditFrame::SetToolbars()
{
    bool             active, islib = TRUE;
    WinEDA_PcbFrame* frame = (WinEDA_PcbFrame*) wxGetApp().GetTopWindow();

    if( m_HToolBar == NULL )
        return;

    if( m_CurrentLib == wxEmptyString )
        islib = false;

    m_HToolBar->EnableTool( ID_MODEDIT_SAVE_LIBMODULE, islib );
    m_HToolBar->EnableTool( ID_MODEDIT_DELETE_PART, islib );

    if( GetBoard()->m_Modules == NULL )
        active = false;
    else
        active = TRUE;

    m_HToolBar->EnableTool( ID_MODEDIT_EXPORT_PART, active );
    m_HToolBar->EnableTool( ID_MODEDIT_CREATE_NEW_LIB_AND_SAVE_CURRENT_PART,
                            active );
    m_HToolBar->EnableTool( ID_MODEDIT_SAVE_LIBMODULE, active && islib );
    MODULE* module_in_edit = GetBoard()->m_Modules;
    if( module_in_edit && module_in_edit->m_Link ) // this is not a new module
                                                   // ...
    {
        BOARD*  mainpcb = frame->GetBoard();
        MODULE* source_module = mainpcb->m_Modules;

        // search if the source module was not deleted:
        for( ; source_module != NULL; source_module = source_module->Next() )
        {
            if( module_in_edit->m_Link == source_module->m_TimeStamp )
                break;
        }

        if( source_module )
        {
            m_HToolBar->EnableTool( ID_MODEDIT_INSERT_MODULE_IN_BOARD, false );
            m_HToolBar->EnableTool( ID_MODEDIT_UPDATE_MODULE_IN_BOARD, true );
        }
        else    // The source was deleted, therefore we can insert but not
                // update the module
        {
            m_HToolBar->EnableTool( ID_MODEDIT_INSERT_MODULE_IN_BOARD, true );
            m_HToolBar->EnableTool( ID_MODEDIT_UPDATE_MODULE_IN_BOARD, false );
        }
    }
    else
    {
        m_HToolBar->EnableTool( ID_MODEDIT_INSERT_MODULE_IN_BOARD, active );
        m_HToolBar->EnableTool( ID_MODEDIT_UPDATE_MODULE_IN_BOARD, false );
    }

    if( GetScreen() )
    {
        m_HToolBar->EnableTool( ID_MODEDIT_UNDO,
                                GetScreen()->GetUndoCommandCount()>0 && active );
        m_HToolBar->EnableTool( ID_MODEDIT_REDO,
                                GetScreen()->GetRedoCommandCount()>0 && active );
    }

    if( frame->GetBoard()->m_Modules )
    {
        m_HToolBar->EnableTool( ID_MODEDIT_LOAD_MODULE_FROM_BOARD, TRUE );
    }
    else
    {
        m_HToolBar->EnableTool( ID_MODEDIT_LOAD_MODULE_FROM_BOARD, false );
    }


    if( m_VToolBar )
    {
        m_VToolBar->EnableTool( ID_MODEDIT_ADD_PAD, active );
        m_VToolBar->EnableTool( ID_PCB_ADD_LINE_BUTT, active );
        m_VToolBar->EnableTool( ID_PCB_CIRCLE_BUTT, active );
        m_VToolBar->EnableTool( ID_PCB_ARC_BUTT, active );
        m_VToolBar->EnableTool( ID_PCB_ADD_TEXT_BUTT, active );
        m_VToolBar->EnableTool( ID_MODEDIT_PLACE_ANCHOR, active );
        m_VToolBar->EnableTool( ID_PCB_DELETE_ITEM_BUTT, active );
    }

    if( m_OptionsToolBar )
    {
        m_OptionsToolBar->ToggleTool(
            ID_TB_OPTIONS_SELECT_UNIT_MM,
            g_UnitMetric ==
            MILLIMETRE ? TRUE : false );
        m_OptionsToolBar->ToggleTool( ID_TB_OPTIONS_SELECT_UNIT_INCH,
                                      g_UnitMetric == INCHES ? TRUE : false );

        m_OptionsToolBar->ToggleTool( ID_TB_OPTIONS_SHOW_POLAR_COORD,
                                      DisplayOpt.DisplayPolarCood );
        m_OptionsToolBar->SetToolShortHelp( ID_TB_OPTIONS_SHOW_POLAR_COORD,
                                            DisplayOpt.DisplayPolarCood ?
                                            _( "Display rectangular coordinates" ) :
                                            _( "Display polar coordinates" ) );

        m_OptionsToolBar->ToggleTool( ID_TB_OPTIONS_SHOW_GRID,
                                      m_Draw_Grid );
        m_OptionsToolBar->SetToolShortHelp( ID_TB_OPTIONS_SHOW_GRID,
                                            m_Draw_Grid ?
                                            _( "Hide grid" ) :
                                            _( "Show grid" ) );


        m_OptionsToolBar->ToggleTool( ID_TB_OPTIONS_SELECT_CURSOR,
                                      m_CursorShape );


        m_OptionsToolBar->ToggleTool( ID_TB_OPTIONS_SHOW_PADS_SKETCH,
                                      !m_DisplayPadFill );

        m_OptionsToolBar->SetToolShortHelp( ID_TB_OPTIONS_SHOW_PADS_SKETCH,
                                            m_DisplayPadFill ?
                                            _( "Show pads in sketch mode" ) :
                                            _( "Show pads in filled mode" ) );
    }

    if( m_AuxiliaryToolBar )
    {
        unsigned jj;
        if( m_SelZoomBox )
        {
            bool not_found = true;
            for( jj = 0; jj < GetScreen()->m_ZoomList.GetCount(); jj++ )
            {
                if( GetScreen()->GetZoom() == GetScreen()->m_ZoomList[jj] )
                {
                    m_SelZoomBox->SetSelection( jj + 1 );
                    not_found = false;
                    break;
                }
            }

            if( not_found )
                m_SelZoomBox->SetSelection( -1 );
        }

        if( m_SelGridBox )
            m_SelGridBox->SetSelection( m_LastGridSizeId );
    }

    DisplayUnitsMsg();
#if defined(KICAD_AUIMANAGER)
    if( m_auimgr.GetManagedWindow() )
        m_auimgr.Update();
#endif
}


/**
 * Display 3D frame of footprint (module) being edited.
 */
void WinEDA_ModuleEditFrame::Show3D_Frame( wxCommandEvent& event )
{
    if( m_Draw3DFrame )
    {
        DisplayInfoMessage( this, _( "3D Frame already opened" ) );
        return;
    }

    m_Draw3DFrame = new WinEDA3D_DrawFrame( this, _( "3D Viewer" ) );
    m_Draw3DFrame->Show( TRUE );
}


void WinEDA_ModuleEditFrame::GeneralControle( wxDC* DC, wxPoint Mouse )
{
    wxRealPoint delta;
    wxPoint     curpos, oldpos;
    int         hotkey = 0;

    if( GetScreen()->IsRefreshReq() )
    {
        DrawPanel->Refresh( );

        // We must return here, instead of proceeding.
        // If we let the cursor move during a refresh request,
        // the cursor be displayed in the wrong place
        // during delayed repaint events that occur when
        // you move the mouse when a message dialog is on
        // the screen, and then you dismiss the dialog by
        // typing the Enter key.
        return;
    }

    curpos = DrawPanel->CursorRealPosition( Mouse );
    oldpos = GetScreen()->m_Curseur;

    delta = GetScreen()->GetGridSize();
    GetScreen()->Scale( delta );

    if( delta.x == 0 )
        delta.x = 1;
    if( delta.y == 0 )
        delta.y = 1;

    switch( g_KeyPressed )
    {
    case WXK_NUMPAD8:
    case WXK_UP:
        Mouse.y -= wxRound( delta.y );
        DrawPanel->MouseTo( Mouse );
        break;

    case WXK_NUMPAD2:
    case WXK_DOWN:
        Mouse.y += wxRound( delta.y );
        DrawPanel->MouseTo( Mouse );
        break;

    case WXK_NUMPAD4:
    case WXK_LEFT:
        Mouse.x -= wxRound( delta.x );
        DrawPanel->MouseTo( Mouse );
        break;

    case WXK_NUMPAD6:
    case WXK_RIGHT:
        Mouse.x += wxRound( delta.x );
        DrawPanel->MouseTo( Mouse );
        break;

    default:
        hotkey = g_KeyPressed;
        break;
    }

    GetScreen()->m_Curseur = curpos;
    PutOnGrid( &GetScreen()->m_Curseur );

    if( oldpos != GetScreen()->m_Curseur )
    {
        curpos = GetScreen()->m_Curseur;
        GetScreen()->m_Curseur = oldpos;
        DrawPanel->CursorOff( DC );

        GetScreen()->m_Curseur = curpos;
        DrawPanel->CursorOn( DC );

        if( DrawPanel->ManageCurseur )
        {
            DrawPanel->ManageCurseur( DrawPanel, DC, TRUE );
        }
    }

    if( hotkey )
    {
        OnHotKey( DC, hotkey, NULL );
    }

    if( GetScreen()->IsRefreshReq() )
    {
        DrawPanel->Refresh( );
        wxSafeYield();
    }
    SetToolbars();
    UpdateStatusBar();
}
