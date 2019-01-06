/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2017 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2004-2018 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <base_screen.h>
#include <msgpanel.h>
#include <draw_frame.h>
#include <confirm.h>
#include <kicad_device_context.h>
#include <dialog_helpers.h>
#include <base_units.h>
#include <math/box2.h>
#include <lockfile.h>
#include <trace_helpers.h>
//#include "../eeschema/sch_draw_panel.h"
#include <wx/fontdlg.h>
#include <wx/snglinst.h>
#include <view/view.h>
#include <view/view_controls.h>
#include <gal/graphics_abstraction_layer.h>
#include <tool/tool_manager.h>
#include <tool/tool_dispatcher.h>
#include <tool/actions.h>
#include <wx/clipbrd.h>
#include <fctsys.h>
#include <gr_basic.h>
#include <common.h>
#include <id.h>
#include <base_screen.h>
#include <confirm.h>
#include <draw_frame.h>
#include <menus_helpers.h>
#include <worksheet_shape_builder.h>
#include <page_info.h>
#include <title_block.h>
#include <advanced_config.h>

/**
 * Definition for enabling and disabling scroll bar setting trace output.  See the
 * wxWidgets documentation on useing the WXTRACE environment variable.
 */
static const wxString traceScrollSettings( wxT( "KicadScrollSettings" ) );


///@{
/// \ingroup config

const wxChar EDA_DRAW_FRAME::CANVAS_TYPE_KEY[] = wxT( "canvas_type" );

static const wxString FirstRunShownKeyword( wxT( "FirstRunShown" ) );

///@}

/**
 * Integer to set the maximum number of undo items on the stack. If zero,
 * undo items are unlimited.
 *
 * Present as:
 *
 * - SchematicFrameDevelMaxUndoItems (file: eeschema)
 * - LibeditFrameDevelMaxUndoItems (file: eeschema)
 * - PcbFrameDevelMaxUndoItems (file: pcbnew)
 * - ModEditFrameDevelMaxUndoItems (file: pcbnew)
 *
 * \ingroup develconfig
 */
static const wxString MaxUndoItemsEntry(wxT( "DevelMaxUndoItems" ) );

BEGIN_EVENT_TABLE( EDA_DRAW_FRAME, KIWAY_PLAYER )
    EVT_CHAR_HOOK( EDA_DRAW_FRAME::OnCharHook )

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
    m_useSingleCanvasPane = false;
    m_socketServer        = nullptr;
    m_drawToolBar         = NULL;
    m_optionsToolBar      = NULL;
    m_auxiliaryToolBar    = NULL;
    m_gridSelectBox       = NULL;
    m_zoomSelectBox       = NULL;
    m_hotkeysDescrList    = NULL;

    m_canvas              = NULL;
    m_canvasType          = EDA_DRAW_PANEL_GAL::GAL_TYPE_NONE;
    m_canvasTypeDirty     = false;
    m_galCanvas           = NULL;
    m_galCanvasActive     = false;
    m_actions             = NULL;
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
    m_LastGridSizeId      = 0;
    m_drawGrid            = true;       // hide/Show grid. default = show
    m_gridColor           = COLOR4D( DARKGRAY );   // Default grid color
    m_showPageLimits      = false;
    m_drawBgColor         = COLOR4D( BLACK );   // the background color of the draw canvas:
                                                // BLACK for Pcbnew, BLACK or WHITE for eeschema
    m_snapToGrid          = true;
    m_MsgFrameHeight      = EDA_MSG_PANEL::GetRequiredHeight();
    m_movingCursorWithKeyboard = false;
    m_zoomLevelCoeff      = 1.0;

    m_auimgr.SetFlags(wxAUI_MGR_DEFAULT);

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

    SetStatusWidths( arrayDim( dims ), dims );

    // Create child subwindows.
    GetClientSize( &m_FrameSize.x, &m_FrameSize.y );
    m_FramePos.x   = m_FramePos.y = 0;
    m_FrameSize.y -= m_MsgFrameHeight;

    m_messagePanel  = new EDA_MSG_PANEL( this, -1, wxPoint( 0, m_FrameSize.y ),
                                         wxSize( m_FrameSize.x, m_MsgFrameHeight ) );

    m_messagePanel->SetBackgroundColour( COLOR4D( LIGHTGRAY ).ToColour() );
}


EDA_DRAW_FRAME::~EDA_DRAW_FRAME()
{
    delete m_socketServer;
    for( auto socket : m_sockets )
    {
        socket->Shutdown();
        socket->Destroy();
    }

    if( m_canvasTypeDirty )     // the canvas type has changed: save the new type
        saveCanvasTypeSetting( m_canvasType );

    delete m_actions;
    delete m_toolManager;
    delete m_toolDispatcher;
    delete m_galCanvas;

    delete m_currentScreen;
    m_currentScreen = NULL;

    m_auimgr.UnInit();

    ReleaseFile();
}


void EDA_DRAW_FRAME::OnCharHook( wxKeyEvent& event )
{
    wxLogTrace( kicadTraceKeyEvent, "EDA_DRAW_FRAME::OnCharHook %s", dump( event ) );
    // Key events can be filtered here.
    // Currently no filtering is made.
    event.Skip();
}


void EDA_DRAW_FRAME::ReleaseFile()
{
    m_file_checker = nullptr;
}


bool EDA_DRAW_FRAME::LockFile( const wxString& aFileName )
{
    m_file_checker = ::LockFile( aFileName );

    return bool( m_file_checker );
}


void EDA_DRAW_FRAME::unitsChangeRefresh()
{
    UpdateStatusBar();
    UpdateMsgPanel();
}


void EDA_DRAW_FRAME::CommonSettingsChanged()
{
    EDA_BASE_FRAME::CommonSettingsChanged();

    wxConfigBase* settings = Pgm().CommonSettings();

    int autosaveInterval;
    settings->Read( AUTOSAVE_INTERVAL_KEY, &autosaveInterval );
    SetAutoSaveInterval( autosaveInterval );

    int historySize;
    settings->Read( FILE_HISTORY_SIZE_KEY, &historySize, DEFAULT_FILE_HISTORY_SIZE );
    Kiface().GetFileHistory().SetMaxFiles( (unsigned) std::max( 0, historySize ) );

    bool option;
    settings->Read( ENBL_MOUSEWHEEL_PAN_KEY, &option );
    m_canvas->SetEnableMousewheelPan( option );

    settings->Read( ENBL_ZOOM_NO_CENTER_KEY, &option );
    m_canvas->SetEnableZoomNoCenter( option );

    settings->Read( ENBL_AUTO_PAN_KEY, &option );
    m_canvas->SetEnableAutoPan( option );

    int tmp;
    settings->Read( GAL_ANTIALIASING_MODE_KEY, &tmp, (int) KIGFX::OPENGL_ANTIALIASING_MODE::NONE );
    m_galDisplayOptions.gl_antialiasing_mode = (KIGFX::OPENGL_ANTIALIASING_MODE) tmp;

    settings->Read( CAIRO_ANTIALIASING_MODE_KEY, &tmp, (int) KIGFX::CAIRO_ANTIALIASING_MODE::NONE );
    m_galDisplayOptions.cairo_antialiasing_mode = (KIGFX::CAIRO_ANTIALIASING_MODE) tmp;

    m_galDisplayOptions.NotifyChanged();
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


bool EDA_DRAW_FRAME::GetToolToggled( int aToolId )
{
    // Checks all the toolbars and returns true if the given tool id is toggled.
    return ( ( m_mainToolBar && m_mainToolBar->GetToolToggled( aToolId ) ) ||
             ( m_optionsToolBar && m_optionsToolBar->GetToolToggled( aToolId ) ) ||
             ( m_drawToolBar && m_drawToolBar->GetToolToggled( aToolId ) ) ||
             ( m_auxiliaryToolBar && m_auxiliaryToolBar->GetToolToggled( aToolId ) )
           );
}


wxAuiToolBarItem* EDA_DRAW_FRAME::GetToolbarTool( int aToolId )
{
    // Checks all the toolbars and returns a reference to the given tool id
    // (or the first tool found, but only one or 0 tool is expected, because on
    // Windows, when different tools have the same ID, it creates issues)
    if( m_mainToolBar && m_mainToolBar->FindTool( aToolId ) )
        return m_mainToolBar->FindTool( aToolId );

    if( m_optionsToolBar && m_optionsToolBar->FindTool( aToolId ) )
        return m_optionsToolBar->FindTool( aToolId );

    if( m_drawToolBar && m_drawToolBar->FindTool( aToolId ) )
        return m_drawToolBar->FindTool( aToolId );

    if( m_auxiliaryToolBar && m_auxiliaryToolBar->FindTool( aToolId ) )
        return m_auxiliaryToolBar->FindTool( aToolId );

    return nullptr;
}


void EDA_DRAW_FRAME::OnSelectUnits( wxCommandEvent& aEvent )
{
    if( aEvent.GetId() == ID_TB_OPTIONS_SELECT_UNIT_MM && m_UserUnits != MILLIMETRES )
    {
        m_UserUnits = MILLIMETRES;
        unitsChangeRefresh();
    }
    else if( aEvent.GetId() == ID_TB_OPTIONS_SELECT_UNIT_INCH && m_UserUnits != INCHES )
    {
        m_UserUnits = INCHES;
        unitsChangeRefresh();
    }
}


void EDA_DRAW_FRAME::OnToggleCrossHairStyle( wxCommandEvent& aEvent )
{
    auto& galOpts = GetGalDisplayOptions();
    galOpts.m_fullscreenCursor = !galOpts.m_fullscreenCursor;

    galOpts.NotifyChanged();

    // make sure the cursor is redrawn
    GetCanvas()->Refresh();
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

    enable = ( ((aEvent.GetId() == ID_TB_OPTIONS_SELECT_UNIT_MM) && (m_UserUnits == MILLIMETRES))
            || ((aEvent.GetId() == ID_TB_OPTIONS_SELECT_UNIT_INCH) && (m_UserUnits == INCHES)) );

    aEvent.Check( enable );
    DisplayUnitsMsg();
}


void EDA_DRAW_FRAME::OnUpdateGrid( wxUpdateUIEvent& aEvent )
{
    wxString tool_tip = IsGridVisible() ? _( "Hide grid" ) : _( "Show grid" );

    aEvent.Check( IsGridVisible() );
    m_optionsToolBar->SetToolShortHelp( ID_TB_OPTIONS_SHOW_GRID, tool_tip );
}


void EDA_DRAW_FRAME::OnUpdateSelectGrid( wxUpdateUIEvent& aEvent )
{
    // No need to update the grid select box if it doesn't exist or the grid setting change
    // was made using the select box.
    if( m_gridSelectBox == NULL || m_auxiliaryToolBar == NULL )
        return;

    int select = wxNOT_FOUND;

    for( size_t i = 0; i < GetScreen()->GetGridCount(); i++ )
    {
        if( GetScreen()->GetGridCmdId() == GetScreen()->GetGrid( i ).m_CmdId )
        {
            select = (int) i;
            break;
        }
    }

    if( select != m_gridSelectBox->GetSelection() )
        m_gridSelectBox->SetSelection( select );
}


void EDA_DRAW_FRAME::OnUpdateCrossHairStyle( wxUpdateUIEvent& aEvent )
{
    aEvent.Check( GetGalDisplayOptions().m_fullscreenCursor );
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


int EDA_DRAW_FRAME::WriteHotkeyConfig( struct EDA_HOTKEY_CONFIG* aDescList,
                                       wxString* aFullFileName )
{
    int result = EDA_BASE_FRAME::WriteHotkeyConfig( aDescList, aFullFileName );

    if( GetToolManager() )
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

    if( event.GetEventType() == wxEVT_COMBOBOX )
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

        if( index == int( m_gridSelectBox->GetCount() - 2 ) )
        {
            // this is the separator
            wxUpdateUIEvent dummy;
            OnUpdateSelectGrid( dummy );
            return;
        }
        else if( index == int( m_gridSelectBox->GetCount() - 1 ) )
        {
            wxUpdateUIEvent dummy;
            OnUpdateSelectGrid( dummy );
            wxCommandEvent dummy2;
            OnGridSettings( dummy2 );
            return;
        }

        clientData = (int*) m_gridSelectBox->wxItemContainer::GetClientData( index );

        if( clientData != NULL )
            eventId = *clientData;
    }
    else
    {
        eventId = event.GetId();
    }

    int idx = eventId - ID_POPUP_GRID_LEVEL_1000;

    SetPresetGrid( idx );
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
        double selectedZoom = GetScreen()->m_ZoomList[id-1];

        if( GetScreen()->GetZoom() == selectedZoom )
            return;

        GetScreen()->SetZoom( selectedZoom );
        RedrawScreen( GetScrollCenterPosition(), false );
    }

    // Notify GAL
    TOOL_MANAGER* mgr = GetToolManager();

    if( mgr && IsGalCanvasActive() )
    {
        mgr->RunAction( "common.Control.zoomPreset", true, id );
        UpdateStatusBar();
    }
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

    switch( m_UserUnits )
    {
    case INCHES:      msg = _( "Inches" ); break;
    case MILLIMETRES: msg = _( "mm" );     break;
    default:          msg = _( "Units" );  break;
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

    // Change GAL canvas cursor if requested.
    if( IsGalCanvasActive() && aCursor >= 0 )
        GetGalCanvas()->SetCurrentCursor( aCursor );

    DisplayToolMsg( aToolMsg );

    if( aId < 0 )
        return;

    wxCHECK2_MSG( aId >= ID_NO_TOOL_SELECTED, aId = ID_NO_TOOL_SELECTED,
                  wxString::Format( wxT( "Current tool ID cannot be set to %d." ), aId ) );

    m_toolId = aId;
}


void EDA_DRAW_FRAME::SetNoToolSelected()
{
    // Select the ID_NO_TOOL_SELECTED id tool (Idle tool)

    int defaultCursor = wxCURSOR_DEFAULT;

    // Change GAL canvas cursor if requested.
    if( IsGalCanvasActive() )
        defaultCursor = GetGalCanvas()->GetDefaultCursor();

    SetToolID( ID_NO_TOOL_SELECTED, defaultCursor, wxEmptyString );
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
    BASE_SCREEN* screen = GetScreen();
    KIGFX::VIEW* view = GetGalCanvas()->GetView();

    if( ! screen->GridExists( aIndex + ID_POPUP_GRID_LEVEL_1000 ) )
        aIndex = 0;

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

    screen->SetGrid( aIndex + ID_POPUP_GRID_LEVEL_1000 );
    view->GetGAL()->SetGridSize( VECTOR2D( screen->GetGridSize() ) );
    view->MarkTargetDirty( KIGFX::TARGET_NONCACHED );

    // Put cursor on new grid
    SetCrossHairPosition( RefPos( true ) );
}


int EDA_DRAW_FRAME::BlockCommand( EDA_KEY key )
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
    wxString Line;
    double level = 0.0;

    if( IsGalCanvasActive() )
    {
        level = m_galCanvas->GetGAL()->GetZoomFactor();
    }
    else if( BASE_SCREEN* screen = GetScreen() )
    {
        level = m_zoomLevelCoeff / screen->GetZoom();
    }

    // returns a human readable value which can be displayed as zoom
    // level indicator in dialogs.
    Line.Printf( wxT( "Z %.2f" ), level );

    return Line;
}


void EDA_DRAW_FRAME::LoadSettings( wxConfigBase* aCfg )
{
    EDA_BASE_FRAME::LoadSettings( aCfg );

    wxString baseCfgName = ConfigBaseName();
    wxConfigBase* cmnCfg = Pgm().CommonSettings();

    // Read units used in dialogs and toolbars
    EDA_UNITS_T unitsTmp;

    if( aCfg->Read( baseCfgName + UserUnitsEntryKeyword, (int*) &unitsTmp ) )
        SetUserUnits( unitsTmp );
    else
        SetUserUnits( MILLIMETRES );

    // Read show/hide grid entry
    bool btmp;
    if( aCfg->Read( baseCfgName + ShowGridEntryKeyword, &btmp ) )
        SetGridVisibility( btmp );

    aCfg->Read( baseCfgName + LastGridSizeIdKeyword, &m_LastGridSizeId, m_LastGridSizeId );

    // m_LastGridSizeId is an offset, expected to be >= 0
    if( m_LastGridSizeId < 0 )
        m_LastGridSizeId = 0;

    m_UndoRedoCountMax = aCfg->Read( baseCfgName + MaxUndoItemsEntry,
            long( DEFAULT_MAX_UNDO_ITEMS ) );

    aCfg->Read( baseCfgName + FirstRunShownKeyword, &m_firstRunDialogSetting, 0L );

    m_galDisplayOptions.ReadConfig( aCfg, baseCfgName + GAL_DISPLAY_OPTIONS_KEY );

    int temp;
    cmnCfg->Read( GAL_ANTIALIASING_MODE_KEY, &temp, (int) KIGFX::OPENGL_ANTIALIASING_MODE::NONE );
    m_galDisplayOptions.gl_antialiasing_mode = (KIGFX::OPENGL_ANTIALIASING_MODE) temp;

    cmnCfg->Read( CAIRO_ANTIALIASING_MODE_KEY, &temp, (int) KIGFX::CAIRO_ANTIALIASING_MODE::NONE );
    m_galDisplayOptions.cairo_antialiasing_mode = (KIGFX::CAIRO_ANTIALIASING_MODE) temp;

    m_galDisplayOptions.NotifyChanged();
}


void EDA_DRAW_FRAME::SaveSettings( wxConfigBase* aCfg )
{
    EDA_BASE_FRAME::SaveSettings( aCfg );

    wxString baseCfgName = ConfigBaseName();

    aCfg->Write( baseCfgName + UserUnitsEntryKeyword, (int) m_UserUnits );
    aCfg->Write( baseCfgName + ShowGridEntryKeyword, IsGridVisible() );
    aCfg->Write( baseCfgName + LastGridSizeIdKeyword, ( long ) m_LastGridSizeId );
    aCfg->Write( baseCfgName + FirstRunShownKeyword, m_firstRunDialogSetting );

    if( GetScreen() )
        aCfg->Write( baseCfgName + MaxUndoItemsEntry, long( GetScreen()->GetMaxUndoItems() ) );

    m_galDisplayOptions.WriteConfig( aCfg, baseCfgName + GAL_DISPLAY_OPTIONS_KEY );
}


void EDA_DRAW_FRAME::AppendMsgPanel( const wxString& textUpper,
                                     const wxString& textLower,
                                     COLOR4D color, int pad )
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
    aItem->GetMsgPanelInfo( m_UserUnits, items );
    SetMsgPanel( items );
}


void EDA_DRAW_FRAME::UpdateMsgPanel()
{
    EDA_ITEM* item = GetScreen()->GetCurItem();

    if( item )
        SetMsgPanel( item );
}


// FIXME: There needs to be a better way for child windows to load preferences.
//        This function pushes four preferences from a parent window to a child window
//        i.e. from eeschema to the schematic symbol editor
void EDA_DRAW_FRAME::PushPreferences( const EDA_DRAW_PANEL* aParentCanvas )
{
    m_canvas->SetEnableZoomNoCenter( aParentCanvas->GetEnableZoomNoCenter() );
    m_canvas->SetEnableAutoPan( aParentCanvas->GetEnableAutoPan() );
}


bool EDA_DRAW_FRAME::HandleBlockBegin( wxDC* aDC, EDA_KEY aKey, const wxPoint& aPosition,
       int aExplicitCommand )
{
    BLOCK_SELECTOR* block = &GetScreen()->m_BlockLocate;

    if( ( block->GetCommand() != BLOCK_IDLE ) || ( block->GetState() != STATE_NO_BLOCK ) )
        return false;

    if( aExplicitCommand == 0 )
        block->SetCommand( (BLOCK_COMMAND_T) BlockCommand( aKey ) );
    else
        block->SetCommand( (BLOCK_COMMAND_T) aExplicitCommand );

    if( block->GetCommand() == 0 )
        return false;

    switch( block->GetCommand() )
    {
    case BLOCK_IDLE:
        break;

    case BLOCK_MOVE:                // Move
    case BLOCK_DRAG:                // Drag (block defined)
    case BLOCK_DRAG_ITEM:           // Drag from a drag item command
    case BLOCK_DUPLICATE:           // Duplicate
    case BLOCK_DUPLICATE_AND_INCREMENT: // Duplicate and increment relevant references
    case BLOCK_DELETE:              // Delete
    case BLOCK_COPY:                // Copy
    case BLOCK_FLIP:                // Flip
    case BLOCK_ZOOM:                // Window Zoom
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
            block->SetState( STATE_NO_BLOCK );
            block->SetMessageBlock( this );
            return true;
        }

        if( !m_canvas->IsMouseCaptured() )
        {
            block->ClearItemsList();
            DisplayError( this,
                          wxT( "EDA_DRAW_FRAME::HandleBlockBegin() Err: m_mouseCaptureCallback NULL" ) );
            block->SetState( STATE_NO_BLOCK );
            block->SetMessageBlock( this );
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


void EDA_DRAW_FRAME::AdjustScrollBars( const wxPoint& aCenterPositionIU )
{
}


void EDA_DRAW_FRAME::UseGalCanvas( bool aEnable )
{
    EDA_DRAW_PANEL_GAL* galCanvas = GetGalCanvas();

    // Display the same view after canvas switching
    if( aEnable )
    {
        // Switch to GAL renderer from legacy
        if( !m_galCanvasActive )
        {
            // Set up viewport
            KIGFX::VIEW* view = galCanvas->GetView();
            view->SetScale( GetZoomLevelCoeff() / m_canvas->GetZoom() );
            view->SetCenter( VECTOR2D( m_canvas->GetScreenCenterLogicalPosition() ) );
        }

        // Transfer EDA_DRAW_PANEL settings
        KIGFX::VIEW_CONTROLS* viewControls = galCanvas->GetViewControls();
        viewControls->EnableCursorWarping( !m_canvas->GetEnableZoomNoCenter() );
        viewControls->EnableMousewheelPan( m_canvas->GetEnableMousewheelPan() );
        viewControls->EnableAutoPan( m_canvas->GetEnableAutoPan() );
    }

    galCanvas->SetEvtHandlerEnabled( aEnable );

    // Reset current tool on switch();
    SetNoToolSelected();

    m_galCanvasActive = aEnable;
}


bool EDA_DRAW_FRAME::SwitchCanvas( EDA_DRAW_PANEL_GAL::GAL_TYPE aCanvasType )
{
    auto galCanvas = GetGalCanvas();
    wxCHECK( galCanvas, false );
    bool use_gal = galCanvas->SwitchBackend( aCanvasType );
    use_gal &= aCanvasType != EDA_DRAW_PANEL_GAL::GAL_TYPE_NONE;
    UseGalCanvas( use_gal );
    m_canvasType = use_gal ? aCanvasType : EDA_DRAW_PANEL_GAL::GAL_TYPE_NONE;
    m_canvasTypeDirty = true;

    return use_gal;
}


EDA_DRAW_PANEL_GAL::GAL_TYPE EDA_DRAW_FRAME::LoadCanvasTypeSetting()
{
    EDA_DRAW_PANEL_GAL::GAL_TYPE canvasType = EDA_DRAW_PANEL_GAL::GAL_TYPE_NONE;
    wxConfigBase* cfg = Kiface().KifaceSettings();

    if( cfg )
        canvasType = (EDA_DRAW_PANEL_GAL::GAL_TYPE) cfg->ReadLong( CANVAS_TYPE_KEY,
                                                                   EDA_DRAW_PANEL_GAL::GAL_TYPE_NONE );

    if( canvasType < EDA_DRAW_PANEL_GAL::GAL_TYPE_NONE
            || canvasType >= EDA_DRAW_PANEL_GAL::GAL_TYPE_LAST )
    {
        wxASSERT( false );
        canvasType = EDA_DRAW_PANEL_GAL::GAL_TYPE_NONE;
    }

    // Coerce the value into a GAL type when Legacy is not available
    // Default to Cairo, and on the first, user will be prompted for OpenGL
    if( canvasType == EDA_DRAW_PANEL_GAL::GAL_TYPE_NONE
            && !ADVANCED_CFG::GetCfg().AllowLegacyCanvas() )
    {
        canvasType = EDA_DRAW_PANEL_GAL::GAL_TYPE_CAIRO;
    }

    return canvasType;
}


bool EDA_DRAW_FRAME::saveCanvasTypeSetting( EDA_DRAW_PANEL_GAL::GAL_TYPE aCanvasType )
{
    if( aCanvasType < EDA_DRAW_PANEL_GAL::GAL_TYPE_NONE
            || aCanvasType >= EDA_DRAW_PANEL_GAL::GAL_TYPE_LAST )
    {
        wxASSERT( false );
        return false;
    }

    wxConfigBase* cfg = Kiface().KifaceSettings();

    if( cfg )
        return cfg->Write( CANVAS_TYPE_KEY, (long) aCanvasType );

    return false;
}

//-----< BASE_SCREEN API moved here >--------------------------------------------

wxPoint EDA_DRAW_FRAME::GetCrossHairPosition( bool aInvertY ) const
{
    VECTOR2I cursor = GetGalCanvas()->GetViewControls()->GetCursorPosition();
    return wxPoint( cursor.x, aInvertY ? -cursor.y : cursor.y );
}


void EDA_DRAW_FRAME::SetCrossHairPosition( const wxPoint& aPosition, bool aSnapToGrid )
{
    // While we're now a GAL canvas, we still have a pre-GAL toolset so some code is going
    // to look for the crosshair position in the BASE_SCREEN and some code is going to look
    // for it in the VIEW_CONTROLS.  Better set it in both.

    BASE_SCREEN* screen = GetScreen();  // virtual call
    screen->setCrossHairPosition( aPosition, GetGridOrigin(), aSnapToGrid );

    wxPoint snappedPosition = screen->getCrossHairPosition( false );
    GetGalCanvas()->GetViewControls()->SetCrossHairCursorPosition( snappedPosition, false );
}


wxPoint EDA_DRAW_FRAME::GetCursorPosition( bool aOnGrid, wxRealPoint* aGridSize ) const
{
    BASE_SCREEN* screen = GetScreen();  // virtual call
    return screen->getCursorPosition( aOnGrid, GetGridOrigin(), aGridSize );
}


wxPoint EDA_DRAW_FRAME::GetNearestGridPosition( const wxPoint& aPosition,
                                                wxRealPoint* aGridSize ) const
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
    wxFAIL_MSG( "shouldn't be using RefreshCrossHair() anymore; use CallMouseCapture() directly" );

    // CrossHair is no longer XORed; no need for most of this
#if 0
    wxPoint newpos = GetCrossHairPosition();


    // Redraw the crosshair if it moved
    if( aOldPos != newpos )
    {
        SetCrossHairPosition( aOldPos, false );
        m_canvas->CrossHairOff( aDC );
        SetCrossHairPosition( newpos, false );
        m_canvas->CrossHairOn( aDC );
#endif
        if( m_canvas->IsMouseCaptured() )
        {
            m_canvas->CallMouseCapture( aDC, aEvtPos, true );
        }
#if 0
    }
#endif
}


bool EDA_DRAW_FRAME::GeneralControlKeyMovement( int aHotKey, wxPoint *aPos,
                                                bool aSnapToGrid )
{
    bool key_handled = false;

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
        key_handled = true;
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

    return key_handled;
}


bool EDA_DRAW_FRAME::isBusy() const
{
    const BASE_SCREEN* screen = const_cast< BASE_SCREEN* >( GetScreen() );

    if( !screen )
        return false;

    return ( screen->GetCurItem() && screen->GetCurItem()->GetFlags() )
           || ( screen->m_BlockLocate.GetState() != STATE_NO_BLOCK );
}


const BOX2I EDA_DRAW_FRAME::GetDocumentExtents() const
{
    return BOX2I();
}


void EDA_DRAW_FRAME::RedrawScreen( const wxPoint& aCenterPoint, bool aWarpPointer )
{
}


void EDA_DRAW_FRAME::RedrawScreen2( const wxPoint& posBefore )
{
}


void EDA_DRAW_FRAME::HardRedraw()
{
}


// Factor out the calculation portion of the various BestZoom() implementations.
//
// Note that like it's forerunners this routine has an intentional side-effect: it
// sets the scroll centre position.  While I'm not happy about that, it's probably
// not worth fixing as its days are numbered (GAL canvases use a different method).
double EDA_DRAW_FRAME::bestZoom( double sizeX, double sizeY, double scaleFactor, wxPoint centre )
{
	return 1.0;
}


void EDA_DRAW_FRAME::Zoom_Automatique( bool aWarpPointer )
{
    if ( IsGalCanvasActive() )
    {
        m_toolManager->RunAction( "common.Control.zoomFitScreen", true );
        return;
    }

}


void EDA_DRAW_FRAME::Window_Zoom( EDA_RECT& Rect )
{
}


void EDA_DRAW_FRAME::OnZoom( wxCommandEvent& event )
{
    if( m_canvas == NULL )
        return;

    int          id = event.GetId();
    bool         warp_cursor = false;
    VECTOR2D cpos = GetCrossHairPosition();//GetGalCanvas()->GetViewControls()->GetCursorPosition();
    wxPoint zoom_center( (int)cpos.x, (int)cpos.y );

    if ( id == ID_KEY_ZOOM_IN )
    {
        id = GetCanvas()->GetEnableZoomNoCenter() ?
             ID_OFFCENTER_ZOOM_IN : ID_POPUP_ZOOM_IN;
    }
    else if ( id == ID_KEY_ZOOM_OUT )
    {
        id = GetCanvas()->GetEnableZoomNoCenter() ?
             ID_OFFCENTER_ZOOM_OUT : ID_POPUP_ZOOM_OUT;
    }

    switch( id )
    {
    case ID_OFFCENTER_ZOOM_IN:
        SetPreviousZoomAndRedraw( zoom_center,warp_cursor );
        break;

    case ID_POPUP_ZOOM_IN:
        warp_cursor = true;
        // fall thru
    case ID_VIEWER_ZOOM_IN:
    case ID_ZOOM_IN:
        SetPreviousZoomAndRedraw( zoom_center,warp_cursor );
        break;

    case ID_OFFCENTER_ZOOM_OUT:
        SetNextZoomAndRedraw( zoom_center, warp_cursor );
        break;

    case ID_POPUP_ZOOM_OUT:
        warp_cursor = true;
        // fall thru
    case ID_VIEWER_ZOOM_OUT:
    case ID_ZOOM_OUT:
        SetNextZoomAndRedraw( zoom_center, warp_cursor );
        break;

    case ID_VIEWER_ZOOM_REDRAW:
    case ID_POPUP_ZOOM_REDRAW:
    case ID_ZOOM_REDRAW:
        // This usually means something went wrong.  Do a hard refresh.
        HardRedraw();
        break;

    case ID_POPUP_ZOOM_CENTER:
        GetGalCanvas()->GetView()->SetScale( GetGalCanvas()->GetView()->GetScale(), zoom_center );
        GetGalCanvas()->GetViewControls()->CenterOnCursor();
        break;

    case ID_POPUP_ZOOM_PAGE:
    case ID_VIEWER_ZOOM_PAGE:
    case ID_ZOOM_PAGE:
        Zoom_Automatique( false );
        break;

    case ID_POPUP_ZOOM_SELECT:
        break;

    case ID_POPUP_CANCEL:
        m_canvas->MoveCursorToCrossHair();
        break;

    default:
        SetPresetZoom( id - ID_POPUP_ZOOM_LEVEL_START );
    }

    UpdateStatusBar();
}


void EDA_DRAW_FRAME::SetNextZoom()
{
    GetScreen()->SetNextZoom();
}


void EDA_DRAW_FRAME::SetPrevZoom()
{
    GetScreen()->SetPreviousZoom();
}


void EDA_DRAW_FRAME::SetNextZoomAndRedraw( const wxPoint& aCenterPoint, bool aWarpPointer )
{
    double zoom = GetGalCanvas()->GetLegacyZoom();
    zoom *= 1.3;

    // Now look for the next closest menu step
    std::vector<double>& zoomList = GetScreen()->m_ZoomList;
    int idx;

    for( idx = 0; idx < (int)zoomList.size(); ++idx )
    {
        if( zoomList[idx] > zoom )
            break;
    }

    if( idx >= (int)zoomList.size() )
        return;

    if( m_zoomSelectBox )
        m_zoomSelectBox->SetSelection( idx );

    GetScreen()->SetZoom( GetScreen()->m_ZoomList[idx] );
    RedrawScreen( aCenterPoint, aWarpPointer );
}


void EDA_DRAW_FRAME::SetPreviousZoomAndRedraw( const wxPoint& aCenterPoint, bool aWarpPointer )
{
    double zoom = GetGalCanvas()->GetLegacyZoom();
    zoom /= 1.3;

    // Now look for the next closest menu step
    std::vector<double>& zoomList = GetScreen()->m_ZoomList;
    int idx;

    for( idx = zoomList.size() - 1; idx >= 0; --idx )
    {
        if( zoomList[idx] < zoom )
            break;
    }

    if( idx < 0 )
        return;

    if( m_zoomSelectBox )
        m_zoomSelectBox->SetSelection( idx );

    GetScreen()->SetZoom( GetScreen()->m_ZoomList[idx] );
    RedrawScreen( aCenterPoint, aWarpPointer );
}


void EDA_DRAW_FRAME::SetPresetZoom( int aIndex )
{
    BASE_SCREEN* screen = GetScreen();

    if( aIndex >= (int) screen->m_ZoomList.size() )
    {
        wxLogDebug( wxT( "%s %d: index %d is outside the bounds of the zoom list." ),
                    __TFILE__, __LINE__, aIndex );
        return;
    }

    if( m_zoomSelectBox )
        m_zoomSelectBox->SetSelection( aIndex );

    if( screen->SetZoom( screen->m_ZoomList[aIndex] ) )
        RedrawScreen( GetScrollCenterPosition(), true );

    UpdateStatusBar();
}


void EDA_DRAW_FRAME::AddMenuZoomAndGrid( wxMenu* MasterMenu )
{
    int         maxZoomIds;
    double      zoom;
    wxString    msg;
    BASE_SCREEN* screen = m_canvas->GetScreen();

    msg = AddHotkeyName( _( "Center" ), m_hotkeysDescrList, HK_ZOOM_CENTER );
    AddMenuItem( MasterMenu, ID_POPUP_ZOOM_CENTER, msg, KiBitmap( zoom_center_on_screen_xpm ) );
    msg = AddHotkeyName( _( "Zoom In" ), m_hotkeysDescrList, HK_ZOOM_IN );
    AddMenuItem( MasterMenu, ID_POPUP_ZOOM_IN, msg, KiBitmap( zoom_in_xpm ) );
    msg = AddHotkeyName( _( "Zoom Out" ), m_hotkeysDescrList, HK_ZOOM_OUT );
    AddMenuItem( MasterMenu, ID_POPUP_ZOOM_OUT, msg, KiBitmap( zoom_out_xpm ) );
    msg = AddHotkeyName( _( "Redraw View" ), m_hotkeysDescrList, HK_ZOOM_REDRAW );
    AddMenuItem( MasterMenu, ID_POPUP_ZOOM_REDRAW, msg, KiBitmap( zoom_redraw_xpm ) );
    msg = AddHotkeyName( _( "Zoom to Fit" ), m_hotkeysDescrList, HK_ZOOM_AUTO );
    AddMenuItem( MasterMenu, ID_POPUP_ZOOM_PAGE, msg, KiBitmap( zoom_fit_in_page_xpm ) );


    wxMenu* zoom_choice = new wxMenu;
    AddMenuItem( MasterMenu, zoom_choice,
                 ID_POPUP_ZOOM_SELECT, _( "Zoom" ),
                 KiBitmap( zoom_selection_xpm ) );

    zoom = screen->GetZoom();
    maxZoomIds = ID_POPUP_ZOOM_LEVEL_END - ID_POPUP_ZOOM_LEVEL_START;
    maxZoomIds = ( (size_t) maxZoomIds < screen->m_ZoomList.size() ) ?
                 maxZoomIds : screen->m_ZoomList.size();

    // Populate zoom submenu.
    for( int i = 0; i < maxZoomIds; i++ )
    {
        msg.Printf( wxT( "%.2f" ), m_zoomLevelCoeff / screen->m_ZoomList[i] );

        zoom_choice->Append( ID_POPUP_ZOOM_LEVEL_START + i, _( "Zoom: " ) + msg,
                             wxEmptyString, wxITEM_CHECK );
        if( zoom == screen->m_ZoomList[i] )
            zoom_choice->Check( ID_POPUP_ZOOM_LEVEL_START + i, true );
    }

    // Create grid submenu as required.
    if( screen->GetGridCount() )
    {
        wxMenu* gridMenu = new wxMenu;
        AddMenuItem( MasterMenu, gridMenu, ID_POPUP_GRID_SELECT,
                     _( "Grid" ), KiBitmap( grid_select_xpm ) );

        wxArrayString gridsList;
        int icurr = screen->BuildGridsChoiceList( gridsList, GetUserUnits() != INCHES );

        for( unsigned i = 0; i < gridsList.GetCount(); i++ )
        {
            GRID_TYPE& grid = screen->GetGrid( i );
            gridMenu->Append( grid.m_CmdId, gridsList[i], wxEmptyString, wxITEM_CHECK );

            if( (int)i == icurr )
                gridMenu->Check( grid.m_CmdId, true );
        }
    }

    MasterMenu->AppendSeparator();
    AddMenuItem( MasterMenu, ID_POPUP_CANCEL, _( "Close" ), KiBitmap( cancel_xpm ) );
}


// Find the first child dialog.
wxWindow* findDialog( wxWindowList& aList )
{
    for( wxWindow* window : aList )
    {
        if( dynamic_cast<DIALOG_SHIM*>( window ) )
            return window;
    }
    return NULL;
}


void EDA_DRAW_FRAME::FocusOnLocation( const wxPoint& aPos, bool aWarpCursor, bool aCenterView )
{
    if( aCenterView )
    {
        wxWindow* dialog = findDialog( GetChildren() );

        // If a dialog partly obscures the window, then center on the uncovered area.
        if( dialog )
        {
            wxRect dialogRect( GetGalCanvas()->ScreenToClient( dialog->GetScreenPosition() ),
                               dialog->GetSize() );
            GetGalCanvas()->GetView()->SetCenter( aPos, dialogRect );
        }
        else
            GetGalCanvas()->GetView()->SetCenter( aPos );
    }

    if( aWarpCursor )
        GetGalCanvas()->GetViewControls()->SetCursorPosition( aPos );
    else
        GetGalCanvas()->GetViewControls()->SetCrossHairCursorPosition( aPos );
}


static bool DrawPageOnClipboard( EDA_DRAW_FRAME* aFrame );


void EDA_DRAW_FRAME::CopyToClipboard( wxCommandEvent& event )
{
    DrawPageOnClipboard( this );

    if( event.GetId() == ID_GEN_COPY_BLOCK_TO_CLIPBOARD )
    {
        // fixme-gal
        //if( GetScreen()->IsBlockActive() )
            //m_canvas->SetCursor( wxCursor( (wxStockCursor) m_canvas->GetDefaultCursor() ) );

        m_canvas->EndMouseCapture();
    }
}


/* copy the current page or block to the clipboard ,
 * to export drawings to other applications (word processing ...)
 * This is not suitable for copy command within Eeschema or Pcbnew
 */
bool DrawPageOnClipboard( EDA_DRAW_FRAME* aFrame )
{
    bool    DrawBlock = false;
    wxRect  DrawArea;
    BASE_SCREEN* screen = aFrame->GetCanvas()->GetScreen();

    if( screen->IsBlockActive() )
    {
        DrawBlock = true;
        DrawArea.SetX( screen->m_BlockLocate.GetX() );
        DrawArea.SetY( screen->m_BlockLocate.GetY() );
        DrawArea.SetWidth( screen->m_BlockLocate.GetWidth() );
        DrawArea.SetHeight( screen->m_BlockLocate.GetHeight() );
    }
    else
        DrawArea.SetSize( aFrame->GetPageSizeIU() );

    // Calculate a reasonable dc size, in pixels, and the dc scale to fit
    // the drawings into the dc size
    // scale is the ratio resolution (in PPI) / internal units
    double ppi = 300;   // Use 300 pixels per inch to create bitmap images on start
    double inch2Iu = 1000.0 * (double) screen->MilsToIuScalar();
    double  scale = ppi / inch2Iu;

    wxSize dcsize = DrawArea.GetSize();

    int maxdim = std::max( dcsize.x, dcsize.y );
    // the max size in pixels of the bitmap used to byuild the sheet copy
    const int maxbitmapsize = 3000;

    while( int( maxdim * scale ) > maxbitmapsize )
    {
        ppi = ppi / 1.5;
        scale = ppi / inch2Iu;
    }

    dcsize.x *= scale;
    dcsize.y *= scale;

    // Set draw offset, zoom... to values needed to draw in the memory DC
    // after saving initial values:
    wxPoint tmp_startvisu = screen->m_StartVisu;
    double tmpzoom = screen->GetZoom();
    wxPoint old_org = screen->m_DrawOrg;
    screen->m_DrawOrg.x   = screen->m_DrawOrg.y = 0;
    screen->m_StartVisu.x = screen->m_StartVisu.y = 0;

    screen->SetZoom( 1 );   // we use zoom = 1 in draw functions.

    wxMemoryDC dc;
    wxBitmap image( dcsize );
    dc.SelectObject( image );

    EDA_RECT tmp = *aFrame->GetCanvas()->GetClipBox();
    GRResetPenAndBrush( &dc );
    GRForceBlackPen( false );
    screen->m_IsPrinting = true;
    dc.SetUserScale( scale, scale );

    aFrame->GetCanvas()->SetClipBox( EDA_RECT( wxPoint( 0, 0 ),
                                     wxSize( 0x7FFFFF0, 0x7FFFFF0 ) ) );

    if( DrawBlock )
    {
        dc.SetClippingRegion( DrawArea );
    }

    dc.Clear();
    aFrame->GetCanvas()->EraseScreen( &dc );
    const LSET allLayersMask = LSET().set();
    aFrame->PrintPage( &dc, allLayersMask, false );
    screen->m_IsPrinting = false;
    aFrame->GetCanvas()->SetClipBox( tmp );

    bool    success = true;

    if( wxTheClipboard->Open() )
    {
        // This data objects are held by the clipboard,
        // so do not delete them in the app.
        wxBitmapDataObject* clipbrd_data = new wxBitmapDataObject( image );
        wxTheClipboard->SetData( clipbrd_data );
        wxTheClipboard->Close();
    }
    else
        success = false;

    // Deselect Bitmap from DC in order to delete the MemoryDC safely
    // without deleting the bitmap
    dc.SelectObject( wxNullBitmap );

    GRForceBlackPen( false );

    screen->m_StartVisu = tmp_startvisu;
    screen->m_DrawOrg   = old_org;
    screen->SetZoom( tmpzoom );

    return success;
}

static const wxString productName = wxT( "KiCad E.D.A.  " );

void DrawPageLayout( wxDC* aDC, EDA_RECT* aClipBox,
                     const PAGE_INFO& aPageInfo,
                     const wxString &aFullSheetName,
                     const wxString& aFileName,
                     TITLE_BLOCK& aTitleBlock,
                     int aSheetCount, int aSheetNumber,
                     int aPenWidth, double aScalar,
                     COLOR4D aColor, COLOR4D aAltColor,
                     const wxString& aSheetLayer )
{
    WS_DRAW_ITEM_LIST drawList;

    drawList.SetPenSize( aPenWidth );
    drawList.SetMilsToIUfactor( aScalar );
    drawList.SetSheetNumber( aSheetNumber );
    drawList.SetSheetCount( aSheetCount );
    drawList.SetFileName( aFileName );
    drawList.SetSheetName( aFullSheetName );
    drawList.SetSheetLayer( aSheetLayer );

    drawList.BuildWorkSheetGraphicList( aPageInfo,
                               aTitleBlock, aColor, aAltColor );

    // Draw item list
    drawList.Draw( aClipBox, aDC );
}


void EDA_DRAW_FRAME::DrawWorkSheet( wxDC* aDC, BASE_SCREEN* aScreen, int aLineWidth,
                                     double aScalar, const wxString &aFilename,
                                     const wxString &aSheetLayer )
{
    if( !m_showBorderAndTitleBlock )
        return;

    const PAGE_INFO&  pageInfo = GetPageSettings();
    wxSize  pageSize = pageInfo.GetSizeMils();

    // if not printing, draw the page limits:
    if( !aScreen->m_IsPrinting && m_showPageLimits )
    {
        GRSetDrawMode( aDC, GR_COPY );
        GRRect( m_canvas->GetClipBox(), aDC, 0, 0,
                pageSize.x * aScalar, pageSize.y * aScalar, aLineWidth,
                m_drawBgColor == WHITE ? LIGHTGRAY : DARKDARKGRAY );
    }

    TITLE_BLOCK t_block = GetTitleBlock();
    COLOR4D color = COLOR4D( RED );

    wxPoint origin = aDC->GetDeviceOrigin();

    if( aScreen->m_IsPrinting && origin.y > 0 )
    {
        aDC->SetDeviceOrigin( 0, 0 );
        aDC->SetAxisOrientation( true, false );
    }

    DrawPageLayout( aDC, m_canvas->GetClipBox(), pageInfo,
                    GetScreenDesc(), aFilename, t_block,
                    aScreen->m_NumberOfScreens, aScreen->m_ScreenNumber,
                    aLineWidth, aScalar, color, color, aSheetLayer );

    if( aScreen->m_IsPrinting && origin.y > 0 )
    {
        aDC->SetDeviceOrigin( origin.x, origin.y );
        aDC->SetAxisOrientation( true, true );
    }
}


wxString EDA_DRAW_FRAME::GetScreenDesc() const
{
    // Virtual function. Base class implementation returns an empty string.
    return wxEmptyString;
}

bool EDA_DRAW_FRAME::LibraryFileBrowser( bool doOpen, wxFileName& aFilename,
                                         const wxString& wildcard, const wxString& ext,
                                         bool isDirectory )
{
    wxString prompt = doOpen ? _( "Select Library" ) : _( "New Library" );
    aFilename.SetExt( ext );

    if( isDirectory )
    {
        wxDirDialog dlg( this, prompt, Prj().GetProjectPath(),
                         doOpen ? wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST : wxDD_DEFAULT_STYLE );

        if( dlg.ShowModal() == wxID_CANCEL )
            return false;

        aFilename = dlg.GetPath();
        aFilename.SetExt( ext );
    }
    else
    {
        wxFileDialog dlg( this, prompt, Prj().GetProjectPath(), aFilename.GetFullName() ,
                          wildcard, doOpen ? wxFD_OPEN | wxFD_FILE_MUST_EXIST
                                           : wxFD_SAVE | wxFD_CHANGE_DIR | wxFD_OVERWRITE_PROMPT );

        if( dlg.ShowModal() == wxID_CANCEL )
            return false;

        aFilename = dlg.GetPath();
        aFilename.SetExt( ext );
    }

    return true;
}


bool EDA_DRAW_FRAME::saveCanvasImageToFile( const wxString& aFileName, wxBitmapType aBitmapType )
{
    return SaveCanvasImageToFile( this, aFileName, aBitmapType );
}
