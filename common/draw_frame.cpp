/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2015 Jean-Pierre Charras, jean-pierre.charras@gipsa-lab.inpg.fr
 * Copyright (C) 2008-2015 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 2004-2015 KiCad Developers, see change_log.txt for contributors.
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
 * @file draw_frame.cpp
 */

#include <fctsys.h>
#include <pgm_base.h>
#include <kiface_i.h>
#include <gr_basic.h>
#include <common.h>
#include <bitmaps.h>
#include <macros.h>
#include <id.h>
#include <class_drawpanel.h>
#include <class_draw_panel_gal.h>
#include <class_base_screen.h>
#include <msgpanel.h>
#include <draw_frame.h>
#include <confirm.h>
#include <kicad_device_context.h>
#include <dialog_helpers.h>
#include <base_units.h>
#include <math/box2.h>

#include <wx/fontdlg.h>
#include <wx/snglinst.h>
#include <view/view.h>
#include <view/view_controls.h>
#include <gal/graphics_abstraction_layer.h>
#include <tool/tool_manager.h>
#include <tool/tool_dispatcher.h>

/**
 * Definition for enabling and disabling scroll bar setting trace output.  See the
 * wxWidgets documentation on useing the WXTRACE environment variable.
 */
static const wxString traceScrollSettings( wxT( "KicadScrollSettings" ) );


// Configuration entry names.
static const wxString CursorShapeEntryKeyword( wxT( "CursorShape" ) );
static const wxString ShowGridEntryKeyword( wxT( "ShowGrid" ) );
static const wxString GridColorEntryKeyword( wxT( "GridColor" ) );
static const wxString LastGridSizeIdKeyword( wxT( "_LastGridSize" ) );


BEGIN_EVENT_TABLE( EDA_DRAW_FRAME, KIWAY_PLAYER )
    EVT_MOUSEWHEEL( EDA_DRAW_FRAME::OnMouseEvent )
    EVT_MENU_OPEN( EDA_DRAW_FRAME::OnMenuOpen )
    EVT_ACTIVATE( EDA_DRAW_FRAME::OnActivate )
    EVT_MENU_RANGE( ID_ZOOM_BEGIN, ID_ZOOM_END, EDA_DRAW_FRAME::OnZoom )

    EVT_MENU_RANGE( ID_POPUP_ZOOM_START_RANGE, ID_POPUP_ZOOM_END_RANGE,
                    EDA_DRAW_FRAME::OnZoom )

    EVT_MENU_RANGE( ID_POPUP_GRID_LEVEL_1000, ID_POPUP_GRID_USER,
                    EDA_DRAW_FRAME::OnSelectGrid )

    EVT_TOOL( ID_TB_OPTIONS_SHOW_GRID, EDA_DRAW_FRAME::OnToggleGridState )
    EVT_TOOL_RANGE( ID_TB_OPTIONS_SELECT_UNIT_MM, ID_TB_OPTIONS_SELECT_UNIT_INCH,
                    EDA_DRAW_FRAME::OnSelectUnits )
    EVT_TOOL( ID_TB_OPTIONS_SELECT_CURSOR, EDA_DRAW_FRAME::OnToggleCrossHairStyle )

    EVT_UPDATE_UI( wxID_UNDO, EDA_DRAW_FRAME::OnUpdateUndo )
    EVT_UPDATE_UI( wxID_REDO, EDA_DRAW_FRAME::OnUpdateRedo )
    EVT_UPDATE_UI( ID_TB_OPTIONS_SHOW_GRID, EDA_DRAW_FRAME::OnUpdateGrid )
    EVT_UPDATE_UI( ID_TB_OPTIONS_SELECT_CURSOR, EDA_DRAW_FRAME::OnUpdateCrossHairStyle )
    EVT_UPDATE_UI_RANGE( ID_TB_OPTIONS_SELECT_UNIT_MM, ID_TB_OPTIONS_SELECT_UNIT_INCH,
                         EDA_DRAW_FRAME::OnUpdateUnits )
END_EVENT_TABLE()


EDA_DRAW_FRAME::EDA_DRAW_FRAME( KIWAY* aKiway, wxWindow* aParent,
                                FRAME_T aFrameType,
                                const wxString& aTitle,
                                const wxPoint& aPos, const wxSize& aSize,
                                long aStyle, const wxString & aFrameName ) :
    KIWAY_PLAYER( aKiway, aParent, aFrameType, aTitle, aPos, aSize, aStyle, aFrameName )
{
    m_file_checker        = NULL;

    m_drawToolBar         = NULL;
    m_optionsToolBar      = NULL;
    m_gridSelectBox       = NULL;
    m_zoomSelectBox       = NULL;
    m_hotkeysDescrList    = NULL;

    m_canvas              = NULL;
    m_galCanvas           = NULL;
    m_galCanvasActive     = false;
    m_toolManager         = NULL;
    m_toolDispatcher      = NULL;
    m_messagePanel        = NULL;
    m_currentScreen       = NULL;
    m_toolId              = ID_NO_TOOL_SELECTED;
    m_lastDrawToolId      = ID_NO_TOOL_SELECTED;
    m_showAxis            = false;      // true to draw axis.
    m_showBorderAndTitleBlock = false;  // true to display reference sheet.
    m_showGridAxis        = false;      // true to draw the grid axis
    m_showOriginAxis      = false;      // true to draw the grid origin
    m_cursorShape         = 0;
    m_LastGridSizeId      = 0;
    m_drawGrid            = true;       // hide/Show grid. default = show
    m_gridColor           = DARKGRAY;   // Default grid color
    m_showPageLimits      = false;
    m_drawBgColor         = BLACK;      // the background color of the draw canvas:
                                        // BLACK for Pcbnew, BLACK or WHITE for eeschema
    m_snapToGrid          = true;
    m_MsgFrameHeight      = EDA_MSG_PANEL::GetRequiredHeight();
    m_movingCursorWithKeyboard = false;
    m_zoomLevelCoeff      = 1.0;

    m_auimgr.SetFlags(wxAUI_MGR_DEFAULT|wxAUI_MGR_LIVE_RESIZE);

    CreateStatusBar( 6 );

    // set the size of the status bar subwindows:

    wxWindow* stsbar = GetStatusBar();

    int dims[] = {

        // remainder of status bar on far left is set to a default or whatever is left over.
        -1,

        // When using GetTextSize() remember the width of character '1' is not the same
        // as the width of '0' unless the font is fixed width, and it usually won't be.

        // zoom:
        GetTextSize( wxT( "Z 762000" ), stsbar ).x + 10,

        // cursor coords
        GetTextSize( wxT( "X 0234.567890  Y 0234.567890" ), stsbar ).x + 10,

        // delta distances
        GetTextSize( wxT( "dx 0234.567890  dx 0234.567890  d 0234.567890" ), stsbar ).x + 10,

        // units display, Inches is bigger than mm
        GetTextSize( _( "Inches" ), stsbar ).x + 10,

        // Size for the panel used as "Current tool in play": will take longest string from
        // void PCB_EDIT_FRAME::OnSelectTool( wxCommandEvent& aEvent ) in pcbnew/edit.cpp
        GetTextSize( wxT( "Add layer alignment target" ), stsbar ).x + 10,
    };

    SetStatusWidths( DIM( dims ), dims );

    // Create child subwindows.
    GetClientSize( &m_FrameSize.x, &m_FrameSize.y );
    m_FramePos.x   = m_FramePos.y = 0;
    m_FrameSize.y -= m_MsgFrameHeight;

    m_canvas = new EDA_DRAW_PANEL( this, -1, wxPoint( 0, 0 ), m_FrameSize );
    m_messagePanel  = new EDA_MSG_PANEL( this, -1, wxPoint( 0, m_FrameSize.y ),
                                         wxSize( m_FrameSize.x, m_MsgFrameHeight ) );

    m_messagePanel->SetBackgroundColour( MakeColour( LIGHTGRAY ) );
}


EDA_DRAW_FRAME::~EDA_DRAW_FRAME()
{
    delete m_toolManager;
    delete m_toolDispatcher;
    delete m_galCanvas;

    delete m_currentScreen;
    m_currentScreen = NULL;

    m_auimgr.UnInit();

    ReleaseFile();
}


void EDA_DRAW_FRAME::ReleaseFile()
{
    delete m_file_checker;
    m_file_checker = 0;
}


bool EDA_DRAW_FRAME::LockFile( const wxString& aFileName )
{
    delete m_file_checker;

    m_file_checker = ::LockFile( aFileName );

    return bool( m_file_checker );
}


void EDA_DRAW_FRAME::unitsChangeRefresh()
{
    UpdateStatusBar();

    EDA_ITEM* item = GetScreen()->GetCurItem();

    if( item )
        SetMsgPanel( item );
}


void EDA_DRAW_FRAME::EraseMsgBox()
{
    if( m_messagePanel )
        m_messagePanel->EraseMsgBox();
}


void EDA_DRAW_FRAME::OnActivate( wxActivateEvent& event )
{
    if( m_canvas )
        m_canvas->SetCanStartBlock( -1 );

    event.Skip();   // required under wxMAC
}


void EDA_DRAW_FRAME::OnMenuOpen( wxMenuEvent& event )
{
    if( m_canvas )
        m_canvas->SetCanStartBlock( -1 );

    event.Skip();
}


void EDA_DRAW_FRAME::SkipNextLeftButtonReleaseEvent()
{
   m_canvas->SetIgnoreLeftButtonReleaseEvent( true );
}


void EDA_DRAW_FRAME::OnToggleGridState( wxCommandEvent& aEvent )
{
    SetGridVisibility( !IsGridVisible() );

    if( IsGalCanvasActive() )
    {
        GetGalCanvas()->GetGAL()->SetGridVisibility( IsGridVisible() );
        GetGalCanvas()->GetView()->MarkTargetDirty( KIGFX::TARGET_NONCACHED );
    }

    m_canvas->Refresh();
}


void EDA_DRAW_FRAME::OnSelectUnits( wxCommandEvent& aEvent )
{
    if( aEvent.GetId() == ID_TB_OPTIONS_SELECT_UNIT_MM && g_UserUnit != MILLIMETRES )
    {
        g_UserUnit = MILLIMETRES;
        unitsChangeRefresh();
    }
    else if( aEvent.GetId() == ID_TB_OPTIONS_SELECT_UNIT_INCH && g_UserUnit != INCHES )
    {
        g_UserUnit = INCHES;
        unitsChangeRefresh();
    }
}


void EDA_DRAW_FRAME::OnToggleCrossHairStyle( wxCommandEvent& aEvent )
{
    INSTALL_UNBUFFERED_DC( dc, m_canvas );
    m_canvas->CrossHairOff( &dc );
    m_cursorShape = !m_cursorShape;
    m_canvas->CrossHairOn( &dc );
}


void EDA_DRAW_FRAME::OnUpdateUndo( wxUpdateUIEvent& aEvent )
{
    if( GetScreen() )
        aEvent.Enable( GetScreen()->GetUndoCommandCount() > 0 );
}


void EDA_DRAW_FRAME::OnUpdateRedo( wxUpdateUIEvent& aEvent )
{
    if( GetScreen() )
        aEvent.Enable( GetScreen()->GetRedoCommandCount() > 0 );
}


void EDA_DRAW_FRAME::OnUpdateUnits( wxUpdateUIEvent& aEvent )
{
    bool enable;

    enable = ( ((aEvent.GetId() == ID_TB_OPTIONS_SELECT_UNIT_MM) && (g_UserUnit == MILLIMETRES))
            || ((aEvent.GetId() == ID_TB_OPTIONS_SELECT_UNIT_INCH) && (g_UserUnit == INCHES)) );

    aEvent.Check( enable );
    DisplayUnitsMsg();
}


void EDA_DRAW_FRAME::OnUpdateGrid( wxUpdateUIEvent& aEvent )
{
    wxString tool_tip = IsGridVisible() ? _( "Hide grid" ) : _( "Show grid" );

    aEvent.Check( IsGridVisible() );
    m_optionsToolBar->SetToolShortHelp( ID_TB_OPTIONS_SHOW_GRID, tool_tip );
}


void EDA_DRAW_FRAME::OnUpdateCrossHairStyle( wxUpdateUIEvent& aEvent )
{
    if( aEvent.IsChecked() != m_cursorShape && IsGalCanvasActive() )
        GetToolManager()->RunAction( "pcbnew.Control.switchCursor" );

    aEvent.Check( m_cursorShape );
}


void EDA_DRAW_FRAME::ReCreateAuxiliaryToolbar()
{
}


void EDA_DRAW_FRAME::ReCreateMenuBar()
{
}


bool EDA_DRAW_FRAME::OnHotKey( wxDC* aDC, int aHotKey, const wxPoint& aPosition, EDA_ITEM* aItem )
{
    return false;
}

int EDA_DRAW_FRAME::WriteHotkeyConfig( struct EDA_HOTKEY_CONFIG* aDescList, wxString* aFullFileName )
{
    int result = EDA_BASE_FRAME::WriteHotkeyConfig( aDescList, aFullFileName );

    if( IsGalCanvasActive() )
        GetToolManager()->UpdateHotKeys();

    return result;
}

void EDA_DRAW_FRAME::ToolOnRightClick( wxCommandEvent& event )
{
}


void EDA_DRAW_FRAME::PrintPage( wxDC* aDC, LSET aPrintMask, bool aPrintMirrorMode, void* aData )
{
    wxMessageBox( wxT("EDA_DRAW_FRAME::PrintPage() error") );
}


void EDA_DRAW_FRAME::OnSelectGrid( wxCommandEvent& event )
{
    int* clientData;
    int  eventId = ID_POPUP_GRID_LEVEL_100;

    if( event.GetEventType() == wxEVT_CHOICE )
    {
        if( m_gridSelectBox == NULL )   // Should not happen
            return;

        /*
         * Don't use wxCommandEvent::GetClientData() here.  It always
         * returns NULL in GTK.  This solution is not as elegant but
         * it works.
         */
        int index = m_gridSelectBox->GetSelection();
        wxASSERT( index != wxNOT_FOUND );
        clientData = (int*) m_gridSelectBox->wxItemContainer::GetClientData( index );

        if( clientData != NULL )
            eventId = *clientData;
    }
    else
    {
        eventId = event.GetId();
    }

    int idx = eventId - ID_POPUP_GRID_LEVEL_1000;

    // Notify GAL
    TOOL_MANAGER* mgr = GetToolManager();

    if( mgr && IsGalCanvasActive() )
    {
        mgr->RunAction( "common.Control.gridPreset", true, idx );
    }
    else
        SetPresetGrid( idx );

    m_canvas->Refresh();
}


void EDA_DRAW_FRAME::OnSelectZoom( wxCommandEvent& event )
{
    if( m_zoomSelectBox == NULL )
        return;                        // Should not happen!

    int id = m_zoomSelectBox->GetCurrentSelection();

    if( id < 0 || !( id < (int)m_zoomSelectBox->GetCount() ) )
        return;

    if( id == 0 )                      // Auto zoom (Fit in Page)
    {
        Zoom_Automatique( true );
    }
    else
    {
        id--;
        double selectedZoom = GetScreen()->m_ZoomList[id];

        if( GetScreen()->GetZoom() == selectedZoom )
            return;

        GetScreen()->SetZoom( selectedZoom );
        RedrawScreen( GetScrollCenterPosition(), false );
    }

    // Notify GAL
    TOOL_MANAGER* mgr = GetToolManager();

    if( mgr && IsGalCanvasActive() )
        mgr->RunAction( "common.Control.zoomPreset", true, id );
}


double EDA_DRAW_FRAME::GetZoom()
{
    return GetScreen()->GetZoom();
}


void EDA_DRAW_FRAME::OnMouseEvent( wxMouseEvent& event )
{
    event.Skip();
}


void EDA_DRAW_FRAME::OnLeftDClick( wxDC* DC, const wxPoint& MousePos )
{
}


void EDA_DRAW_FRAME::DisplayToolMsg( const wxString& msg )
{
    SetStatusText( msg, 5 );
}


void EDA_DRAW_FRAME::DisplayUnitsMsg()
{
    wxString msg;

    switch( g_UserUnit )
    {
    case INCHES:
        msg = _( "Inches" );
        break;

    case MILLIMETRES:
        msg = _( "mm" );
        break;

    default:
        msg = _( "Units" );
        break;
    }

    SetStatusText( msg, 4 );
}



void EDA_DRAW_FRAME::OnSize( wxSizeEvent& SizeEv )
{
    m_FrameSize = GetClientSize( );

    SizeEv.Skip();
}


void EDA_DRAW_FRAME::SetToolID( int aId, int aCursor, const wxString& aToolMsg )
{
    // Keep default cursor in toolbars
    SetCursor( wxNullCursor );

    // Change m_canvas cursor if requested.
    if( m_canvas && aCursor >= 0 )
        m_canvas->SetCurrentCursor( aCursor );

    DisplayToolMsg( aToolMsg );

    if( aId < 0 )
        return;

    wxCHECK2_MSG( aId >= ID_NO_TOOL_SELECTED, aId = ID_NO_TOOL_SELECTED,
                  wxString::Format( wxT( "Current tool ID cannot be set to %d." ), aId ) );

    m_toolId = aId;
}


wxPoint EDA_DRAW_FRAME::GetGridPosition( const wxPoint& aPosition ) const
{
    wxPoint pos = aPosition;

    if( m_currentScreen != NULL && m_snapToGrid )
        pos = GetNearestGridPosition( aPosition );

    return pos;
}


void EDA_DRAW_FRAME::SetNextGrid()
{
    BASE_SCREEN * screen = GetScreen();

    int new_grid_cmd = screen->GetGridCmdId();

    // if the grid id is the not the last, increment it
    if( screen->GridExists( new_grid_cmd + 1 ) )
        new_grid_cmd += 1;

   SetPresetGrid( new_grid_cmd - ID_POPUP_GRID_LEVEL_1000 );
}


void EDA_DRAW_FRAME::SetPrevGrid()
{
    BASE_SCREEN * screen = GetScreen();

    int new_grid_cmd = screen->GetGridCmdId();

    // if the grid id is the not the first, increment it
    if( screen->GridExists( new_grid_cmd - 1 ) )
        new_grid_cmd -= 1;

    SetPresetGrid( new_grid_cmd - ID_POPUP_GRID_LEVEL_1000 );
}


void EDA_DRAW_FRAME::SetPresetGrid( int aIndex )
{
    BASE_SCREEN * screen = GetScreen();

    if( ! screen->GridExists( aIndex + ID_POPUP_GRID_LEVEL_1000 ) )
        aIndex = screen->GetGrids()[0].m_CmdId;

    // aIndex is a Command Id relative to ID_POPUP_GRID_LEVEL_1000 comand id code.
    // we need an index in grid list (the cmd id in list is is screen->GetGrids()[0].m_CmdId):
    int glistIdx = aIndex + ID_POPUP_GRID_LEVEL_1000 - screen->GetGrids()[0].m_CmdId;

    if( m_gridSelectBox )
    {
        if( glistIdx < 0 || glistIdx >= (int) m_gridSelectBox->GetCount() )
        {
            wxASSERT_MSG( false, "Invalid grid index" );
            return;
        }

        m_gridSelectBox->SetSelection( glistIdx );
    }

    // Be sure m_LastGridSizeId is up to date.
    m_LastGridSizeId = aIndex;
    GetScreen()->SetGrid( aIndex + ID_POPUP_GRID_LEVEL_1000 );

    // Put cursor on new grid
    SetCrossHairPosition( RefPos( true ) );
}


int EDA_DRAW_FRAME::BlockCommand( int key )
{
    return 0;
}


void EDA_DRAW_FRAME::InitBlockPasteInfos()
{
    GetScreen()->m_BlockLocate.ClearItemsList();
    m_canvas->SetMouseCaptureCallback( NULL );
}


void EDA_DRAW_FRAME::HandleBlockPlace( wxDC* DC )
{
}


bool EDA_DRAW_FRAME::HandleBlockEnd( wxDC* DC )
{
    return false;
}


void EDA_DRAW_FRAME::UpdateStatusBar()
{
    SetStatusText( GetZoomLevelIndicator(), 1 );

    // Absolute and relative cursor positions are handled by overloading this function and
    // handling the internal to user units conversion at the appropriate level.

    // refresh units display
    DisplayUnitsMsg();
}

const wxString EDA_DRAW_FRAME::GetZoomLevelIndicator() const
{
    BASE_SCREEN*    screen = GetScreen();
    wxString Line;

    if( screen )
    {
        // returns a human readable value which can be displayed as zoom
        // level indicator in dialogs.
        double level =  m_zoomLevelCoeff / (double)screen->GetZoom();
        Line.Printf( wxT( "Z %.2f" ), level );
    }

    return Line;
}


void EDA_DRAW_FRAME::LoadSettings( wxConfigBase* aCfg )
{
    EDA_BASE_FRAME::LoadSettings( aCfg );

    wxString baseCfgName = GetName();

    aCfg->Read( baseCfgName + CursorShapeEntryKeyword, &m_cursorShape, ( long )0 );

    bool btmp;
    if( aCfg->Read( baseCfgName + ShowGridEntryKeyword, &btmp ) )
        SetGridVisibility( btmp );

    int itmp;
    if( aCfg->Read( baseCfgName + GridColorEntryKeyword, &itmp ) )
        SetGridColor( ColorFromInt( itmp ) );

    aCfg->Read( baseCfgName + LastGridSizeIdKeyword, &m_LastGridSizeId, 0L );

    // m_LastGridSizeId is an offset, expected to be >= 0
    if( m_LastGridSizeId < 0 )
        m_LastGridSizeId = 0;
}


void EDA_DRAW_FRAME::SaveSettings( wxConfigBase* aCfg )
{
    EDA_BASE_FRAME::SaveSettings( aCfg );

    wxString baseCfgName = GetName();

    aCfg->Write( baseCfgName + CursorShapeEntryKeyword, m_cursorShape );
    aCfg->Write( baseCfgName + ShowGridEntryKeyword, IsGridVisible() );
    aCfg->Write( baseCfgName + GridColorEntryKeyword, ( long ) GetGridColor() );
    aCfg->Write( baseCfgName + LastGridSizeIdKeyword, ( long ) m_LastGridSizeId );
}


void EDA_DRAW_FRAME::AppendMsgPanel( const wxString& textUpper,
                                     const wxString& textLower,
                                     EDA_COLOR_T color, int pad )
{
    if( m_messagePanel == NULL )
        return;

    m_messagePanel->AppendMessage( textUpper, textLower, color, pad );
}


void EDA_DRAW_FRAME::ClearMsgPanel()
{
    if( m_messagePanel == NULL )
        return;

    m_messagePanel->EraseMsgBox();
}


void EDA_DRAW_FRAME::SetMsgPanel( const MSG_PANEL_ITEMS& aList )
{
    if( m_messagePanel == NULL )
        return;

    ClearMsgPanel();

    for( unsigned i = 0;  i < aList.size();  i++ )
        m_messagePanel->AppendMessage( aList[i] );
}


void EDA_DRAW_FRAME::SetMsgPanel( EDA_ITEM* aItem )
{
    wxCHECK_RET( aItem != NULL, wxT( "Invalid EDA_ITEM pointer.  Bad programmer." ) );

    MSG_PANEL_ITEMS items;
    aItem->GetMsgPanelInfo( items );
    SetMsgPanel( items );
}


wxString EDA_DRAW_FRAME::CoordinateToString( int aValue, bool aConvertToMils ) const
{
    return ::CoordinateToString( aValue, aConvertToMils );
}

wxString EDA_DRAW_FRAME::LengthDoubleToString( double aValue, bool aConvertToMils ) const
{
    return ::LengthDoubleToString( aValue, aConvertToMils );
}


bool EDA_DRAW_FRAME::HandleBlockBegin( wxDC* aDC, int aKey, const wxPoint& aPosition )
{
    BLOCK_SELECTOR* block = &GetScreen()->m_BlockLocate;

    if( ( block->GetCommand() != BLOCK_IDLE ) || ( block->GetState() != STATE_NO_BLOCK ) )
        return false;

    block->SetCommand( (BLOCK_COMMAND_T) BlockCommand( aKey ) );

    if( block->GetCommand() == 0 )
        return false;

    switch( block->GetCommand() )
    {
    case BLOCK_IDLE:
        break;

    case BLOCK_MOVE:                // Move
    case BLOCK_DRAG:                // Drag (block defined)
    case BLOCK_DRAG_ITEM:           // Drag from a drag item command
    case BLOCK_COPY:                // Copy
    case BLOCK_COPY_AND_INCREMENT:  // Copy and increment relevant references
    case BLOCK_DELETE:              // Delete
    case BLOCK_SAVE:                // Save
    case BLOCK_ROTATE:              // Rotate 90 deg
    case BLOCK_FLIP:                // Flip
    case BLOCK_ZOOM:                // Window Zoom
    case BLOCK_MIRROR_X:
    case BLOCK_MIRROR_Y:            // mirror
    case BLOCK_PRESELECT_MOVE:      // Move with preselection list
        block->InitData( m_canvas, aPosition );
        break;

    case BLOCK_PASTE:
        block->InitData( m_canvas, aPosition );
        block->SetLastCursorPosition( wxPoint( 0, 0 ) );
        InitBlockPasteInfos();

        if( block->GetCount() == 0 )      // No data to paste
        {
            DisplayError( this, wxT( "No block to paste" ), 20 );
            GetScreen()->m_BlockLocate.SetCommand( BLOCK_IDLE );
            m_canvas->SetMouseCaptureCallback( NULL );
            return true;
        }

        if( !m_canvas->IsMouseCaptured() )
        {
            block->ClearItemsList();
            DisplayError( this,
                          wxT( "EDA_DRAW_FRAME::HandleBlockBegin() Err: m_mouseCaptureCallback NULL" ) );
            return true;
        }

        block->SetState( STATE_BLOCK_MOVE );
        m_canvas->CallMouseCapture( aDC, aPosition, false );
        break;

    default:
        {
            wxString msg;
            msg << wxT( "EDA_DRAW_FRAME::HandleBlockBegin() error: Unknown command " ) <<
            block->GetCommand();
            DisplayError( this, msg );
        }
        break;
    }

    block->SetMessageBlock( this );
    return true;
}


// I am not seeing a problem with this size yet:
static const double MAX_AXIS = INT_MAX - 100;

#define VIRT_MIN    (-MAX_AXIS/2.0)     ///< min X or Y coordinate in virtual space
#define VIRT_MAX    (MAX_AXIS/2.0)      ///< max X or Y coordinate in virtual space


void EDA_DRAW_FRAME::AdjustScrollBars( const wxPoint& aCenterPositionIU )
{
    BASE_SCREEN* screen = GetScreen();

    if( !screen || !m_canvas )
        return;

    double scale = screen->GetScalingFactor();

    wxLogTrace( traceScrollSettings, wxT( "Center Position = ( %d, %d ), scale = %.10g" ),
                aCenterPositionIU.x, aCenterPositionIU.y, scale );

    // Calculate the portion of the drawing that can be displayed in the
    // client area at the current zoom level.

    // visible viewport in device units ~ pixels
    wxSize  clientSizeDU = m_canvas->GetClientSize();

    // Size of the client window in IU
    DSIZE   clientSizeIU( clientSizeDU.x / scale, clientSizeDU.y / scale );

    // Full drawing or "page" rectangle in internal units
    DBOX    pageRectIU( wxPoint( 0, 0 ), wxSize( GetPageSizeIU().x, GetPageSizeIU().y ) );

    // The upper left corner of the client rectangle in internal units.
    double xIU = aCenterPositionIU.x - clientSizeIU.x / 2.0;
    double yIU = aCenterPositionIU.y - clientSizeIU.y / 2.0;

    // If drawn around the center, adjust the client rectangle accordingly.
    if( screen->m_Center )
    {
        // half page offset.
        xIU += pageRectIU.GetWidth()  / 2.0;
        yIU += pageRectIU.GetHeight() / 2.0;
    }

    DBOX    clientRectIU( wxPoint( xIU, yIU ), wxSize( clientSizeIU.x, clientSizeIU.y ) );
    wxPoint centerPositionIU;

    // put "int" limits on the clientRect
    if( clientRectIU.GetLeft() < VIRT_MIN )
        clientRectIU.MoveLeftTo( VIRT_MIN );
    if( clientRectIU.GetTop() < VIRT_MIN )
        clientRectIU.MoveTopTo( VIRT_MIN );
    if( clientRectIU.GetRight() > VIRT_MAX )
        clientRectIU.MoveRightTo( VIRT_MAX );
    if( clientRectIU.GetBottom() > VIRT_MAX )
        clientRectIU.MoveBottomTo( VIRT_MAX );

    centerPositionIU.x = KiROUND( clientRectIU.GetX() + clientRectIU.GetWidth() / 2 );
    centerPositionIU.y = KiROUND( clientRectIU.GetY() + clientRectIU.GetHeight() / 2 );

    if( screen->m_Center )
    {
        centerPositionIU.x -= KiROUND( pageRectIU.GetWidth() / 2.0 );
        centerPositionIU.y -= KiROUND( pageRectIU.GetHeight() / 2.0 );
    }

    DSIZE   virtualSizeIU;

    if( pageRectIU.GetLeft() < clientRectIU.GetLeft() && pageRectIU.GetRight() > clientRectIU.GetRight() )
    {
        virtualSizeIU.x = pageRectIU.GetSize().x;
    }
    else
    {
        double pageCenterX    = pageRectIU.GetX()   + ( pageRectIU.GetWidth() / 2 );
        double clientCenterX  = clientRectIU.GetX() + ( clientRectIU.GetWidth() / 2 );

        if( clientRectIU.GetWidth() > pageRectIU.GetWidth() )
        {
            if( pageCenterX > clientCenterX )
                virtualSizeIU.x = ( pageCenterX - clientRectIU.GetLeft() ) * 2;
            else if( pageCenterX < clientCenterX )
                virtualSizeIU.x = ( clientRectIU.GetRight() - pageCenterX ) * 2;
            else
                virtualSizeIU.x = clientRectIU.GetWidth();
        }
        else
        {
            if( pageCenterX > clientCenterX )
                virtualSizeIU.x = pageRectIU.GetWidth() + ( (pageRectIU.GetLeft() - clientRectIU.GetLeft() ) * 2 );
            else if( pageCenterX < clientCenterX )
                virtualSizeIU.x = pageRectIU.GetWidth() + ( (clientRectIU.GetRight() - pageRectIU.GetRight() ) * 2 );
            else
                virtualSizeIU.x = pageRectIU.GetWidth();
        }
    }

    if( pageRectIU.GetTop() < clientRectIU.GetTop() && pageRectIU.GetBottom() > clientRectIU.GetBottom() )
    {
        virtualSizeIU.y = pageRectIU.GetSize().y;
    }
    else
    {
        double pageCenterY   = pageRectIU.GetY()   + ( pageRectIU.GetHeight() / 2 );
        double clientCenterY = clientRectIU.GetY() + ( clientRectIU.GetHeight() / 2 );

        if( clientRectIU.GetHeight() > pageRectIU.GetHeight() )
        {
            if( pageCenterY > clientCenterY )
                virtualSizeIU.y = ( pageCenterY - clientRectIU.GetTop() ) * 2;
            else if( pageCenterY < clientCenterY )
                virtualSizeIU.y = ( clientRectIU.GetBottom() - pageCenterY ) * 2;
            else
                virtualSizeIU.y = clientRectIU.GetHeight();
        }
        else
        {
            if( pageCenterY > clientCenterY )
                virtualSizeIU.y = pageRectIU.GetHeight() +
                                ( ( pageRectIU.GetTop() - clientRectIU.GetTop() ) * 2 );
            else if( pageCenterY < clientCenterY )
                virtualSizeIU.y = pageRectIU.GetHeight() +
                                ( ( clientRectIU.GetBottom() - pageRectIU.GetBottom() ) * 2 );
            else
                virtualSizeIU.y = pageRectIU.GetHeight();
        }
    }

    // put "int" limits on the virtualSizeIU
    virtualSizeIU.x = std::min( virtualSizeIU.x, MAX_AXIS );
    virtualSizeIU.y = std::min( virtualSizeIU.y, MAX_AXIS );

    if( screen->m_Center )
    {
        screen->m_DrawOrg.x = -KiROUND( virtualSizeIU.x / 2.0 );
        screen->m_DrawOrg.y = -KiROUND( virtualSizeIU.y / 2.0 );
    }
    else
    {
        screen->m_DrawOrg.x = -KiROUND( ( virtualSizeIU.x - pageRectIU.GetWidth() )  / 2.0 );
        screen->m_DrawOrg.y = -KiROUND( ( virtualSizeIU.y - pageRectIU.GetHeight() ) / 2.0 );
    }

    /* Always set scrollbar pixels per unit to 1 unless you want the zoom
     * around cursor to jump around.  This reported problem occurs when the
     * zoom point is not on a pixel per unit increment.  If you set the
     * pixels per unit to 10, you have potential for the zoom point to
     * jump around +/-5 pixels from the nearest grid point.
     */
    screen->m_ScrollPixelsPerUnitX = screen->m_ScrollPixelsPerUnitY = 1;

    // Number of scroll bar units for the given zoom level in device units.
    double unitsX = virtualSizeIU.x * scale;
    double unitsY = virtualSizeIU.y * scale;

    // Calculate the scroll bar position in internal units to place the
    // center position at the center of client rectangle.
    SetScrollCenterPosition( centerPositionIU );

    double posX = centerPositionIU.x - clientRectIU.GetWidth()  / 2.0 - screen->m_DrawOrg.x;
    double posY = centerPositionIU.y - clientRectIU.GetHeight() / 2.0 - screen->m_DrawOrg.y;

    // Convert scroll bar position to device units.
    posX = KiROUND( posX * scale );
    posY = KiROUND( posY * scale );

    if( posX < 0 )
    {
        wxLogTrace( traceScrollSettings, wxT( "Required scroll bar X position %.10g" ), posX );
        posX = 0;
    }

    if( posX > unitsX )
    {
        wxLogTrace( traceScrollSettings, wxT( "Required scroll bar X position %.10g" ), posX );
        posX = unitsX;
    }

    if( posY < 0 )
    {
        wxLogTrace( traceScrollSettings, wxT( "Required scroll bar Y position %.10g" ), posY );
        posY = 0;
    }

    if( posY > unitsY )
    {
        wxLogTrace( traceScrollSettings, wxT( "Required scroll bar Y position %.10g" ), posY );
        posY = unitsY;
    }

    screen->m_ScrollbarPos    = wxPoint( KiROUND( posX ),  KiROUND( posY ) );
    screen->m_ScrollbarNumber = wxSize( KiROUND( unitsX ), KiROUND( unitsY ) );

    wxLogTrace( traceScrollSettings,
                wxT( "Drawing = (%.10g, %.10g), Client = (%.10g, %.10g), Offset = (%d, %d), SetScrollbars(%d, %d, %d, %d, %d, %d)" ),
                virtualSizeIU.x, virtualSizeIU.y, clientSizeIU.x, clientSizeIU.y,
                screen->m_DrawOrg.x, screen->m_DrawOrg.y,
                screen->m_ScrollPixelsPerUnitX, screen->m_ScrollPixelsPerUnitY,
                screen->m_ScrollbarNumber.x, screen->m_ScrollbarNumber.y,
                screen->m_ScrollbarPos.x, screen->m_ScrollbarPos.y );

    bool            noRefresh = true;

    m_canvas->SetScrollbars( screen->m_ScrollPixelsPerUnitX,
                             screen->m_ScrollPixelsPerUnitY,
                             screen->m_ScrollbarNumber.x,
                             screen->m_ScrollbarNumber.y,
                             screen->m_ScrollbarPos.x,
                             screen->m_ScrollbarPos.y, noRefresh );
}


void EDA_DRAW_FRAME::UseGalCanvas( bool aEnable )
{
    KIGFX::VIEW* view = GetGalCanvas()->GetView();
    KIGFX::GAL* gal = GetGalCanvas()->GetGAL();

    double zoomFactor = gal->GetWorldScale() / gal->GetZoomFactor();
    BASE_SCREEN* screen = GetScreen();

    // Display the same view after canvas switching
    if( aEnable )
    {
        // Switch to GAL rendering
        if( !m_galCanvasActive )
        {
            // Set up viewport
            double zoom = 1.0 / ( zoomFactor * m_canvas->GetZoom() );
            view->SetScale( zoom );
            view->SetCenter( VECTOR2D( m_canvas->GetScreenCenterLogicalPosition() ) );
        }

        // Set up grid settings
        gal->SetGridVisibility( IsGridVisible() );
        gal->SetGridSize( VECTOR2D( screen->GetGridSize() ) );
        gal->SetGridOrigin( VECTOR2D( GetGridOrigin() ) );

        GetToolManager()->RunAction( "pcbnew.Control.switchCursor" );
    }
    else if( m_galCanvasActive )
    {
        // Switch to standard rendering
        double zoom = 1.0 / ( zoomFactor * view->GetScale() );
        m_canvas->SetZoom( zoom );

        VECTOR2D center = view->GetCenter();
        AdjustScrollBars( wxPoint( center.x, center.y ) );
    }

    m_canvas->SetEvtHandlerEnabled( !aEnable );
    GetGalCanvas()->SetEvtHandlerEnabled( aEnable );

    // Switch panes
    m_auimgr.GetPane( wxT( "DrawFrame" ) ).Show( !aEnable );
    m_auimgr.GetPane( wxT( "DrawFrameGal" ) ).Show( aEnable );
    m_auimgr.Update();

    // Reset current tool on switch();
    SetToolID( ID_NO_TOOL_SELECTED, wxCURSOR_DEFAULT, wxEmptyString );

    m_galCanvasActive = aEnable;
}

//-----< BASE_SCREEN API moved here >--------------------------------------------

wxPoint EDA_DRAW_FRAME::GetCrossHairPosition( bool aInvertY ) const
{
    // subject to change, borrow from old BASE_SCREEN for now.
    if( IsGalCanvasActive() )
    {
        VECTOR2I cursor = GetGalCanvas()->GetViewControls()->GetCursorPosition();

        return wxPoint( cursor.x, cursor.y );
    }
    else
    {
        BASE_SCREEN* screen = GetScreen();  // virtual call
        return screen->getCrossHairPosition( aInvertY );
    }
}


void EDA_DRAW_FRAME::SetCrossHairPosition( const wxPoint& aPosition, bool aSnapToGrid )
{
    BASE_SCREEN* screen = GetScreen();  // virtual call
    screen->setCrossHairPosition( aPosition, GetGridOrigin(), aSnapToGrid );
}


wxPoint EDA_DRAW_FRAME::GetCursorPosition( bool aOnGrid, wxRealPoint* aGridSize ) const
{
    BASE_SCREEN* screen = GetScreen();  // virtual call
    return screen->getCursorPosition( aOnGrid, GetGridOrigin(), aGridSize );
}


wxPoint EDA_DRAW_FRAME::GetNearestGridPosition( const wxPoint& aPosition, wxRealPoint* aGridSize ) const
{
    BASE_SCREEN* screen = GetScreen();  // virtual call
    return screen->getNearestGridPosition( aPosition, GetGridOrigin(), aGridSize );
}


wxPoint EDA_DRAW_FRAME::GetCrossHairScreenPosition() const
{
    BASE_SCREEN* screen = GetScreen();  // virtual call
    return screen->getCrossHairScreenPosition();
}


void EDA_DRAW_FRAME::SetMousePosition( const wxPoint& aPosition )
{
    BASE_SCREEN* screen = GetScreen();  // virtual call
    screen->setMousePosition( aPosition );
}


wxPoint EDA_DRAW_FRAME::RefPos( bool useMouse ) const
{
    BASE_SCREEN* screen = GetScreen();  // virtual call
    return screen->refPos( useMouse );
}


const wxPoint& EDA_DRAW_FRAME::GetScrollCenterPosition() const
{
    BASE_SCREEN* screen = GetScreen();  // virtual call
    return screen->getScrollCenterPosition();
}


void EDA_DRAW_FRAME::SetScrollCenterPosition( const wxPoint& aPoint )
{
    BASE_SCREEN* screen = GetScreen();  // virtual call
    screen->setScrollCenterPosition( aPoint );
}

//-----</BASE_SCREEN API moved here >--------------------------------------------

void EDA_DRAW_FRAME::RefreshCrossHair( const wxPoint &aOldPos,
                                       const wxPoint &aEvtPos,
                                       wxDC* aDC )
{
    wxPoint newpos = GetCrossHairPosition();

    // Redraw the crosshair if it moved
    if( aOldPos != newpos )
    {
        SetCrossHairPosition( aOldPos, false );
        m_canvas->CrossHairOff( aDC );
        SetCrossHairPosition( newpos, false );
        m_canvas->CrossHairOn( aDC );

        if( m_canvas->IsMouseCaptured() )
        {
#ifdef USE_WX_OVERLAY
            wxDCOverlay oDC( m_overlay, (wxWindowDC*)aDC );
            oDC.Clear();
            m_canvas->CallMouseCapture( aDC, aEvtPos, false );
#else
            m_canvas->CallMouseCapture( aDC, aEvtPos, true );
#endif
        }
#ifdef USE_WX_OVERLAY
        else
        {
            m_overlay.Reset();
        }
#endif
    }
}

void EDA_DRAW_FRAME::GeneralControlKeyMovement( int aHotKey, wxPoint *aPos,
                                                bool aSnapToGrid )
{

    // If requested snap the current position to the grid
    if( aSnapToGrid )
        *aPos = GetNearestGridPosition( *aPos );

    switch( aHotKey )
    {
    // All these keys have almost the same treatment
    case GR_KB_CTRL | WXK_NUMPAD8:
    case GR_KB_CTRL | WXK_UP:
    case GR_KB_CTRL | WXK_NUMPAD2:
    case GR_KB_CTRL | WXK_DOWN:
    case GR_KB_CTRL | WXK_NUMPAD4:
    case GR_KB_CTRL | WXK_LEFT:
    case GR_KB_CTRL | WXK_NUMPAD6:
    case GR_KB_CTRL | WXK_RIGHT:
    case WXK_NUMPAD8:
    case WXK_UP:
    case WXK_NUMPAD2:
    case WXK_DOWN:
    case WXK_NUMPAD4:
    case WXK_LEFT:
    case WXK_NUMPAD6:
    case WXK_RIGHT:
        {
            /* Here's a tricky part: when doing cursor key movement, the
             * 'previous' point should be taken from memory, *not* from the
             * freshly computed position in the event. Otherwise you can't do
             * sub-pixel movement. The m_movingCursorWithKeyboard oneshot 'eats'
             * the automatic motion event generated by cursor warping */
            wxRealPoint gridSize = GetScreen()->GetGridSize();
            *aPos = GetCrossHairPosition();

            // Bonus: ^key moves faster (x10)
            switch( aHotKey )
            {
            case GR_KB_CTRL|WXK_NUMPAD8:
            case GR_KB_CTRL|WXK_UP:
                aPos->y -= KiROUND( 10 * gridSize.y );
                break;

            case GR_KB_CTRL|WXK_NUMPAD2:
            case GR_KB_CTRL|WXK_DOWN:
                aPos->y += KiROUND( 10 * gridSize.y );
                break;

            case GR_KB_CTRL|WXK_NUMPAD4:
            case GR_KB_CTRL|WXK_LEFT:
                aPos->x -= KiROUND( 10 * gridSize.x );
                break;

            case GR_KB_CTRL|WXK_NUMPAD6:
            case GR_KB_CTRL|WXK_RIGHT:
                aPos->x += KiROUND( 10 * gridSize.x );
                break;

            case WXK_NUMPAD8:
            case WXK_UP:
                aPos->y -= KiROUND( gridSize.y );
                break;

            case WXK_NUMPAD2:
            case WXK_DOWN:
                aPos->y += KiROUND( gridSize.y );
                break;

            case WXK_NUMPAD4:
            case WXK_LEFT:
                aPos->x -= KiROUND( gridSize.x );
                break;

            case WXK_NUMPAD6:
            case WXK_RIGHT:
                aPos->x += KiROUND( gridSize.x );
                break;

            default: /* Can't happen since we entered the statement */
                break;
            }
            m_canvas->MoveCursor( *aPos );
            m_movingCursorWithKeyboard = true;
        }
        break;

    default:
        break;
    }
}

