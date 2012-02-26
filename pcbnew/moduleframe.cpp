/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2008-2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2011 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file moduleframe.cpp
 * @brief Footprint (module) editor main window.
 */

#include <fctsys.h>
#include <appl_wxstruct.h>
#include <class_drawpanel.h>
#include <confirm.h>
#include <wxPcbStruct.h>
#include <dialog_helpers.h>
#include <3d_viewer.h>
#include <pcbcommon.h>

#include <class_board.h>
#include <class_module.h>

#include <pcbnew.h>
#include <protos.h>
#include <pcbnew_id.h>
#include <hotkeys.h>
#include <module_editor_frame.h>


static PCB_SCREEN* s_screenModule;      // the PCB_SCREEN used by the footprint editor

wxString FOOTPRINT_EDIT_FRAME::m_CurrentLib = wxEmptyString;

BOARD* FOOTPRINT_EDIT_FRAME::s_Pcb;

BEGIN_EVENT_TABLE( FOOTPRINT_EDIT_FRAME, PCB_BASE_FRAME )
    EVT_MENU_RANGE( ID_POPUP_PCB_ITEM_SELECTION_START, ID_POPUP_PCB_ITEM_SELECTION_END,
                    PCB_BASE_FRAME::ProcessItemSelection )
    EVT_CLOSE( FOOTPRINT_EDIT_FRAME::OnCloseWindow )
    EVT_MENU( wxID_EXIT, FOOTPRINT_EDIT_FRAME::CloseModuleEditor )

    EVT_SIZE( FOOTPRINT_EDIT_FRAME::OnSize )

    EVT_COMBOBOX( ID_ON_ZOOM_SELECT, FOOTPRINT_EDIT_FRAME::OnSelectZoom )
    EVT_COMBOBOX( ID_ON_GRID_SELECT, FOOTPRINT_EDIT_FRAME::OnSelectGrid )

    EVT_TOOL( ID_MODEDIT_SELECT_CURRENT_LIB, FOOTPRINT_EDIT_FRAME::Process_Special_Functions )
    EVT_TOOL( ID_MODEDIT_SAVE_LIBMODULE, FOOTPRINT_EDIT_FRAME::Process_Special_Functions )
    EVT_TOOL( ID_OPEN_MODULE_VIEWER, FOOTPRINT_EDIT_FRAME::Process_Special_Functions )

    EVT_TOOL( ID_MODEDIT_DELETE_PART, FOOTPRINT_EDIT_FRAME::Process_Special_Functions )
    EVT_TOOL( ID_MODEDIT_NEW_MODULE, FOOTPRINT_EDIT_FRAME::Process_Special_Functions )
    EVT_TOOL( ID_MODEDIT_LOAD_MODULE, FOOTPRINT_EDIT_FRAME::Process_Special_Functions )
    EVT_TOOL( ID_MODEDIT_IMPORT_PART, FOOTPRINT_EDIT_FRAME::Process_Special_Functions )
    EVT_TOOL( ID_MODEDIT_EXPORT_PART, FOOTPRINT_EDIT_FRAME::Process_Special_Functions )
    EVT_TOOL( ID_MODEDIT_CREATE_NEW_LIB_AND_SAVE_CURRENT_PART,
              FOOTPRINT_EDIT_FRAME::Process_Special_Functions )
    EVT_TOOL( ID_MODEDIT_SHEET_SET, FOOTPRINT_EDIT_FRAME::Process_Special_Functions )
    EVT_TOOL( wxID_PRINT, FOOTPRINT_EDIT_FRAME::ToPrinter )
    EVT_TOOL( ID_MODEDIT_LOAD_MODULE, FOOTPRINT_EDIT_FRAME::Process_Special_Functions )
    EVT_TOOL( ID_MODEDIT_CHECK, FOOTPRINT_EDIT_FRAME::Process_Special_Functions )
    EVT_TOOL( ID_MODEDIT_PAD_SETTINGS, FOOTPRINT_EDIT_FRAME::Process_Special_Functions )
    EVT_TOOL( ID_MODEDIT_LOAD_MODULE_FROM_BOARD, FOOTPRINT_EDIT_FRAME::LoadModuleFromBoard )
    EVT_TOOL( ID_MODEDIT_INSERT_MODULE_IN_BOARD, FOOTPRINT_EDIT_FRAME::Process_Special_Functions )
    EVT_TOOL( ID_MODEDIT_UPDATE_MODULE_IN_BOARD, FOOTPRINT_EDIT_FRAME::Process_Special_Functions )
    EVT_TOOL( ID_MODEDIT_EDIT_MODULE_PROPERTIES, FOOTPRINT_EDIT_FRAME::Process_Special_Functions )
    EVT_TOOL( wxID_UNDO, FOOTPRINT_EDIT_FRAME::GetComponentFromUndoList )
    EVT_TOOL( wxID_REDO, FOOTPRINT_EDIT_FRAME::GetComponentFromRedoList )

    // Vertical tool bar button click event handler.
    EVT_TOOL( ID_NO_TOOL_SELECTED, FOOTPRINT_EDIT_FRAME::OnVerticalToolbar )
    EVT_TOOL_RANGE( ID_MODEDIT_PAD_TOOL, ID_MODEDIT_PLACE_GRID_COORD,
                    FOOTPRINT_EDIT_FRAME::OnVerticalToolbar )

    // Options Toolbar
    EVT_TOOL( ID_TB_OPTIONS_SHOW_PADS_SKETCH, FOOTPRINT_EDIT_FRAME::OnSelectOptionToolbar )
    EVT_TOOL( ID_TB_OPTIONS_SHOW_VIAS_SKETCH, FOOTPRINT_EDIT_FRAME::OnSelectOptionToolbar )
    EVT_TOOL( ID_TB_OPTIONS_SHOW_MODULE_TEXT_SKETCH, FOOTPRINT_EDIT_FRAME::OnSelectOptionToolbar )
    EVT_TOOL( ID_TB_OPTIONS_SHOW_MODULE_EDGE_SKETCH, FOOTPRINT_EDIT_FRAME::OnSelectOptionToolbar )

    // popup commands
    EVT_MENU_RANGE( ID_POPUP_PCB_START_RANGE, ID_POPUP_PCB_END_RANGE,
                    FOOTPRINT_EDIT_FRAME::Process_Special_Functions )

    EVT_MENU_RANGE( ID_POPUP_GENERAL_START_RANGE, ID_POPUP_GENERAL_END_RANGE,
                    FOOTPRINT_EDIT_FRAME::Process_Special_Functions )

    EVT_MENU( ID_POPUP_MODEDIT_EDIT_BODY_ITEM, FOOTPRINT_EDIT_FRAME::Process_Special_Functions )
    EVT_MENU( ID_POPUP_MODEDIT_EDIT_WIDTH_ALL_EDGE, FOOTPRINT_EDIT_FRAME::Process_Special_Functions )
    EVT_MENU( ID_POPUP_MODEDIT_EDIT_LAYER_CURRENT_EDGE, FOOTPRINT_EDIT_FRAME::Process_Special_Functions )
    EVT_MENU( ID_POPUP_MODEDIT_EDIT_LAYER_ALL_EDGE, FOOTPRINT_EDIT_FRAME::Process_Special_Functions )
    EVT_MENU( ID_POPUP_MODEDIT_ENTER_EDGE_WIDTH, FOOTPRINT_EDIT_FRAME::Process_Special_Functions )

    // Module transformations
    EVT_MENU( ID_MODEDIT_MODULE_ROTATE, FOOTPRINT_EDIT_FRAME::Process_Special_Functions )
    EVT_MENU( ID_MODEDIT_MODULE_MIRROR, FOOTPRINT_EDIT_FRAME::Process_Special_Functions )

    EVT_MENU( ID_PCB_DRAWINGS_WIDTHS_SETUP, FOOTPRINT_EDIT_FRAME::Process_Special_Functions )
    EVT_MENU( ID_PCB_PAD_SETUP, FOOTPRINT_EDIT_FRAME::Process_Special_Functions )
    EVT_MENU( ID_PCB_USER_GRID_SETUP, FOOTPRINT_EDIT_FRAME::Process_Special_Functions )

    // Menu 3D Frame
    EVT_MENU( ID_MENU_PCB_SHOW_3D_FRAME, FOOTPRINT_EDIT_FRAME::Show3D_Frame )

    EVT_UPDATE_UI( ID_MODEDIT_DELETE_PART, FOOTPRINT_EDIT_FRAME::OnUpdateLibSelected )
    EVT_UPDATE_UI( ID_MODEDIT_EXPORT_PART, FOOTPRINT_EDIT_FRAME::OnUpdateModuleSelected )
    EVT_UPDATE_UI( ID_MODEDIT_CREATE_NEW_LIB_AND_SAVE_CURRENT_PART,
                   FOOTPRINT_EDIT_FRAME::OnUpdateModuleSelected )
    EVT_UPDATE_UI( ID_MODEDIT_SAVE_LIBMODULE, FOOTPRINT_EDIT_FRAME::OnUpdateLibAndModuleSelected )
    EVT_UPDATE_UI( ID_MODEDIT_LOAD_MODULE_FROM_BOARD,
                   FOOTPRINT_EDIT_FRAME::OnUpdateLoadModuleFromBoard )
    EVT_UPDATE_UI( ID_MODEDIT_INSERT_MODULE_IN_BOARD,
                   FOOTPRINT_EDIT_FRAME::OnUpdateInsertModuleInBoard )
    EVT_UPDATE_UI( ID_MODEDIT_UPDATE_MODULE_IN_BOARD,
                   FOOTPRINT_EDIT_FRAME::OnUpdateReplaceModuleInBoard )
    EVT_UPDATE_UI( ID_NO_TOOL_SELECTED, FOOTPRINT_EDIT_FRAME::OnUpdateVerticalToolbar )
    EVT_UPDATE_UI_RANGE( ID_MODEDIT_PAD_TOOL, ID_MODEDIT_PLACE_GRID_COORD,
                         FOOTPRINT_EDIT_FRAME::OnUpdateVerticalToolbar )

END_EVENT_TABLE()


FOOTPRINT_EDIT_FRAME::FOOTPRINT_EDIT_FRAME( PCB_EDIT_FRAME* aParent,
                                            const wxString& title,
                                            const wxPoint&  pos,
                                            const wxSize&   size,
                                            long            style ) :
    PCB_BASE_FRAME( aParent, MODULE_EDITOR_FRAME, wxEmptyString, pos, size, style )
{
    m_FrameName = wxT( "ModEditFrame" );
    m_showBorderAndTitleBlock = false;   // true to show the frame references
    m_showAxis = true;                   // true to show X and Y axis on screen
    m_showGridAxis = true;               // show the grid origin axis
    m_HotkeysZoomAndGridList = g_Module_Editor_Hokeys_Descr;

    // Give an icon
    wxIcon icon;
    icon.CopyFromBitmap( KiBitmap( icon_modedit_xpm ) );
    SetIcon( icon );

    UpdateTitle();

    if( !s_Pcb )
    {
        s_Pcb = new BOARD();
        // Ensure all layers and items are visible:
        s_Pcb->SetVisibleAlls();
    }

    SetBoard( s_Pcb );

    if( !s_screenModule )
        s_screenModule = new PCB_SCREEN( GetPageSettings().GetSizeIU() );

    SetScreen( s_screenModule );

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

    if( m_canvas )
        m_canvas->SetEnableBlockCommands( true );

    m_auimgr.SetManagedWindow( this );

    EDA_PANEINFO horiz;
    horiz.HorizontalToolbarPane();

    EDA_PANEINFO vert;
    vert.VerticalToolbarPane();

    EDA_PANEINFO mesg_pane;
    mesg_pane.MessageToolbarPane();

    m_auimgr.AddPane( m_mainToolBar,
                      wxAuiPaneInfo( horiz ).Name( wxT( "m_mainToolBar" ) ).Top(). Row( 0 ) );

    m_auimgr.AddPane( m_auxiliaryToolBar,
                      wxAuiPaneInfo( horiz ).Name( wxT( "m_auxiliaryToolBar" ) ).Top().Row( 1 ) );

    m_auimgr.AddPane( m_drawToolBar,
                      wxAuiPaneInfo( vert ).Name( wxT( "m_VToolBar" ) ).Right() );

    m_auimgr.AddPane( m_optionsToolBar,
                      wxAuiPaneInfo( vert ).Name( wxT( "m_optionsToolBar" ) ). Left() );

    m_auimgr.AddPane( m_canvas,
                      wxAuiPaneInfo().Name( wxT( "DrawFrame" ) ).CentrePane() );

    m_auimgr.AddPane( m_messagePanel,
                      wxAuiPaneInfo( mesg_pane ).Name( wxT( "MsgPanel" ) ).Bottom().Layer(10) );

    m_auimgr.Update();
}


FOOTPRINT_EDIT_FRAME::~FOOTPRINT_EDIT_FRAME()
{
    // When user reopens the Footprint editor, user would like to find the last edited item.
    // Do not delete PCB_SCREEN (by the destructor of EDA_DRAW_FRAME)
    SetScreen( NULL );

    PCB_BASE_FRAME* frame = (PCB_BASE_FRAME*) GetParent();
    frame->m_ModuleEditFrame = NULL;

    // Do not allow PCB_BASE_FRAME::~PCB_BASE_FRAME()
    // to delete our precious BOARD, which is also in static FOOTPRINT_EDIT_FRAME::s_Pcb.
    // That function, PCB_BASE_FRAME::~PCB_BASE_FRAME(), runs immediately next
    // as we return from here.
    m_Pcb = 0;
}


BOARD_DESIGN_SETTINGS& FOOTPRINT_EDIT_FRAME::GetDesignSettings() const
{
    // get the BOARD_DESIGN_SETTINGS from the parent editor, not our BOARD.

    PCB_BASE_FRAME* parentFrame = (PCB_BASE_FRAME*) GetParent();

    wxASSERT( parentFrame );

    return parentFrame->GetDesignSettings();
}


void FOOTPRINT_EDIT_FRAME::SetDesignSettings( const BOARD_DESIGN_SETTINGS& aSettings )
{
    // set the BOARD_DESIGN_SETTINGS int parent editor, not our BOARD.

    PCB_BASE_FRAME* parentFrame = (PCB_BASE_FRAME*) GetParent();

    wxASSERT( parentFrame );

    parentFrame->SetDesignSettings( aSettings );
}


void FOOTPRINT_EDIT_FRAME::OnCloseWindow( wxCloseEvent& Event )
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


void FOOTPRINT_EDIT_FRAME::CloseModuleEditor( wxCommandEvent& Event )
{
    Close();
}


void FOOTPRINT_EDIT_FRAME::OnUpdateVerticalToolbar( wxUpdateUIEvent& aEvent )
{
    aEvent.Enable( GetBoard()->m_Modules != NULL );

    if( aEvent.GetEventObject() == m_drawToolBar )
        aEvent.Check( GetToolId() == aEvent.GetId() );
}


void FOOTPRINT_EDIT_FRAME::OnUpdateLibSelected( wxUpdateUIEvent& aEvent )
{
    aEvent.Enable( m_CurrentLib != wxEmptyString );
}


void FOOTPRINT_EDIT_FRAME::OnUpdateModuleSelected( wxUpdateUIEvent& aEvent )
{
    aEvent.Enable( GetBoard()->m_Modules != NULL );
}


void FOOTPRINT_EDIT_FRAME::OnUpdateLibAndModuleSelected( wxUpdateUIEvent& aEvent )
{
    aEvent.Enable( ( m_CurrentLib != wxEmptyString ) && ( GetBoard()->m_Modules != NULL ) );
}


void FOOTPRINT_EDIT_FRAME::OnUpdateLoadModuleFromBoard( wxUpdateUIEvent& aEvent )
{
    PCB_BASE_FRAME* frame = (PCB_BASE_FRAME*) GetParent();

    aEvent.Enable( frame->GetBoard()->m_Modules != NULL );
}


void FOOTPRINT_EDIT_FRAME::OnUpdateInsertModuleInBoard( wxUpdateUIEvent& aEvent )
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
            if( module_in_edit->m_Link == source_module->GetTimeStamp() )
                break;
        }

        canInsert = ( source_module == NULL );
    }

    aEvent.Enable( canInsert );
}


void FOOTPRINT_EDIT_FRAME::OnUpdateReplaceModuleInBoard( wxUpdateUIEvent& aEvent )
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
            if( module_in_edit->m_Link == source_module->GetTimeStamp() )
                break;
        }

        canReplace = ( source_module != NULL );
    }

    aEvent.Enable( canReplace );
}


void FOOTPRINT_EDIT_FRAME::Show3D_Frame( wxCommandEvent& event )
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


void FOOTPRINT_EDIT_FRAME::GeneralControl( wxDC* aDC, const wxPoint& aPosition, int aHotKey )
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
        m_canvas->MoveCursor( pos );
        break;

    case WXK_NUMPAD2:
    case WXK_DOWN:
        pos.y += wxRound( gridSize.y );
        m_canvas->MoveCursor( pos );
        break;

    case WXK_NUMPAD4:
    case WXK_LEFT:
        pos.x -= wxRound( gridSize.x );
        m_canvas->MoveCursor( pos );
        break;

    case WXK_NUMPAD6:
    case WXK_RIGHT:
        pos.x += wxRound( gridSize.x );
        m_canvas->MoveCursor( pos );
        break;

    default:
        break;
    }

    GetScreen()->SetCrossHairPosition( pos );

    if( oldpos != GetScreen()->GetCrossHairPosition() )
    {
        pos = GetScreen()->GetCrossHairPosition();
        GetScreen()->SetCrossHairPosition( oldpos );
        m_canvas->CrossHairOff( aDC );
        GetScreen()->SetCrossHairPosition( pos );
        m_canvas->CrossHairOn( aDC );

        if( m_canvas->IsMouseCaptured() )
        {
#ifdef USE_WX_OVERLAY
            wxDCOverlay oDC( m_overlay, (wxWindowDC*)aDC );
            oDC.Clear();
            m_canvas->CallMouseCapture( aDC, aPosition, false );
#else
            m_canvas->CallMouseCapture( aDC, aPosition, true );
#endif
        }
#ifdef USE_WX_OVERLAY
        else
        {
            m_overlay.Reset();
        }
#endif
    }

    if( aHotKey )
    {
        OnHotKey( aDC, aHotKey, aPosition );
    }

    UpdateStatusBar();
}


void FOOTPRINT_EDIT_FRAME::OnModify()
{
    PCB_BASE_FRAME::OnModify();

    if( m_Draw3DFrame )
        m_Draw3DFrame->ReloadRequest();
}


void FOOTPRINT_EDIT_FRAME::UpdateTitle()
{
    wxString title = _( "Module Editor " );

    if( m_CurrentLib.IsEmpty() )
    {
        title += _( "(no active library)" );
    }
    else
    {
        wxFileName fileName = wxFileName( wxEmptyString, m_CurrentLib, ModuleFileExtension );
        fileName = wxGetApp().FindLibraryPath( fileName );

        if( !fileName.IsOk() || !fileName.FileExists() )
        {
            title += _( "(no active library)" );
        }
        else
        {
            title = _( "Module Editor (active library: " ) + fileName.GetFullPath() + wxT( ")" );

            if( !fileName.IsFileWritable() )
                title += _( " [Read Only]" );
        }
    }

    SetTitle( title );
}
