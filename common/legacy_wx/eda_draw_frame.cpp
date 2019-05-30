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
#include <wx/clipbrd.h>
#include <fctsys.h>
#include <gr_basic.h>
#include <common.h>
#include <id.h>
#include <base_screen.h>
#include <confirm.h>
#include <draw_frame.h>
#include <wx/fontdlg.h>
#include <wx/snglinst.h>
#include <view/view.h>
#include <view/view_controls.h>
#include <gal/graphics_abstraction_layer.h>
#include <tool/tool_manager.h>
#include <tool/tool_dispatcher.h>
#include <tool/actions.h>
#include <tool/action_menu.h>
#include <advanced_config.h>
#include <menus_helpers.h>
#include <page_info.h>
#include <title_block.h>
#include <ws_draw_item.h>

/**
 * Definition for enabling and disabling scroll bar setting trace output.  See the
 * wxWidgets documentation on useing the WXTRACE environment variable.
 */
static const wxString traceScrollSettings( wxT( "KicadScrollSettings" ) );


///@{
/// \ingroup config
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

    EVT_MENU_OPEN( EDA_DRAW_FRAME::OnMenuOpen )
    EVT_MENU_CLOSE( EDA_DRAW_FRAME::OnMenuOpen )
    EVT_MENU_HIGHLIGHT_ALL( EDA_DRAW_FRAME::OnMenuOpen )

    EVT_MENU_RANGE( ID_POPUP_GRID_LEVEL_1000, ID_POPUP_GRID_USER,
                    EDA_DRAW_FRAME::OnSelectGrid )
END_EVENT_TABLE()


EDA_DRAW_FRAME::EDA_DRAW_FRAME( KIWAY* aKiway, wxWindow* aParent,
                                FRAME_T aFrameType,
                                const wxString& aTitle,
                                const wxPoint& aPos, const wxSize& aSize,
                                long aStyle, const wxString & aFrameName ) :
    KIWAY_PLAYER( aKiway, aParent, aFrameType, aTitle, aPos, aSize, aStyle, aFrameName )
{
    m_socketServer        = nullptr;
    m_mainToolBar         = NULL;
    m_drawToolBar         = NULL;
    m_optionsToolBar      = NULL;
    m_auxiliaryToolBar    = NULL;
    m_gridSelectBox       = NULL;
    m_zoomSelectBox       = NULL;
    m_hotkeysDescrList    = NULL;

    m_canvas              = NULL;
    m_canvasType          = EDA_DRAW_PANEL_GAL::GAL_TYPE_NONE;
    m_galCanvas           = NULL;
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

    m_canvas = new EDA_DRAW_PANEL( this, -1, wxPoint( 0, 0 ), m_FrameSize );
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

    wxConfigBase*         settings = Pgm().CommonSettings();
    KIGFX::VIEW_CONTROLS* viewControls = GetGalCanvas()->GetViewControls();

    int autosaveInterval;
    settings->Read( AUTOSAVE_INTERVAL_KEY, &autosaveInterval );
    SetAutoSaveInterval( autosaveInterval );

    int historySize;
    settings->Read( FILE_HISTORY_SIZE_KEY, &historySize, DEFAULT_FILE_HISTORY_SIZE );
    Kiface().GetFileHistory().SetMaxFiles( (unsigned) std::max( 0, historySize ) );

    bool option;
    settings->Read( ENBL_MOUSEWHEEL_PAN_KEY, &option );
    viewControls->EnableMousewheelPan( option );

    settings->Read( ENBL_ZOOM_NO_CENTER_KEY, &option );
    viewControls->EnableCursorWarping( !option );

    settings->Read( ENBL_AUTO_PAN_KEY, &option );
    viewControls->EnableAutoPan( option );

    m_galDisplayOptions.ReadCommonConfig( *settings, this );
}


void EDA_DRAW_FRAME::EraseMsgBox()
{
    if( m_messagePanel )
        m_messagePanel->EraseMsgBox();
}


void EDA_DRAW_FRAME::OnMenuOpen( wxMenuEvent& event )
{
    // On wxWidgets 3.0.x Windows, EVT_MENU_OPEN ( and other EVT_MENU_xx) events are not
    // captured by the ACTON_MENU menus.  While it is fixed in wxWidgets 3.1.x, we still
    // need a solution for the earlier verions.
    //
    // This could be made conditional, but for now I'm going to use the same strategy
    // everywhere so it gets wider testing.
    // Note that if the conditional compilation is reactivated, the Connect() lines in
    // ACTION_MENU::setupEvents() will need to be re-enabled.
//#if defined( __WINDOWS__ ) && wxCHECK_VERSION( 3, 0, 0 ) && !wxCHECK_VERSION( 3, 1, 0 )

    // As if things weren't bad enough, wxWidgets doesn't pass the menu pointer when the
    // event is a wxEVT_MENU_HIGHLIGHT, so we store the menu from the EVT_MENU_OPEN call.
    static ACTION_MENU* currentMenu;

    if( event.GetEventType() == wxEVT_MENU_OPEN )
    {
        currentMenu = dynamic_cast<ACTION_MENU*>( event.GetMenu() );

        if( currentMenu )
            currentMenu->OnMenuEvent( event );
    }
    else if( event.GetEventType() == wxEVT_MENU_HIGHLIGHT )
    {
        if( currentMenu )
            currentMenu->OnMenuEvent( event );
    }
    else    // if( event.GetEventType() == wxEVT_MENU_CLOSE )
    {
        currentMenu = nullptr;
    }
//#endif

    event.Skip();
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


void EDA_DRAW_FRAME::ReCreateAuxiliaryToolbar()
{
}


void EDA_DRAW_FRAME::ReCreateMenuBar()
{
}


int EDA_DRAW_FRAME::WriteHotkeyConfig( struct EDA_HOTKEY_CONFIG* aDescList,
                                       wxString* aFullFileName )
{
    int result = EDA_BASE_FRAME::WriteHotkeyConfig( aDescList, aFullFileName );

    GetToolManager()->UpdateHotKeys();

    return result;
}

void EDA_DRAW_FRAME::PrintPage( wxDC* aDC, LSET aPrintMask, bool aPrintMirrorMode, void* aData )
{
    wxMessageBox( wxT("EDA_DRAW_FRAME::PrintPage() error") );
}


void EDA_DRAW_FRAME::OnSelectGrid( wxCommandEvent& event )
{
    wxCHECK_RET( m_gridSelectBox, "m_gridSelectBox uninitialized" );

    int id = m_gridSelectBox->GetCurrentSelection() + ID_POPUP_GRID_FIRST;

    if( id == ID_POPUP_GRID_SEPARATOR )
    {
        // wxWidgets will check the separator, which we don't want.
        // Re-check the current grid.
        wxUpdateUIEvent dummy;
        OnUpdateSelectGrid( dummy );
    }
    else if( id == ID_POPUP_GRID_SETTINGS )
    {
        // wxWidgets will check the Grid Settings... entry, which we don't want.
        // R-check the current grid.
        wxUpdateUIEvent dummy;
        OnUpdateSelectGrid( dummy );
        // Now run the Grid Settings... dialog
        wxCommandEvent dummy2;
        OnGridSettings( dummy2 );
    }
    else if( id >= ID_POPUP_GRID_FIRST && id < ID_POPUP_GRID_SEPARATOR  )
    {
        m_toolManager->RunAction( ACTIONS::gridPreset, true, id );
    }

    UpdateStatusBar();
    m_galCanvas->Refresh();
}


void EDA_DRAW_FRAME::OnSelectZoom( wxCommandEvent& event )
{
    wxCHECK_RET( m_zoomSelectBox, "m_zoomSelectBox uninitialized" );

    int id = m_zoomSelectBox->GetCurrentSelection();

    if( id < 0 || !( id < (int)m_zoomSelectBox->GetCount() ) )
        return;

    m_toolManager->RunAction( "common.Control.zoomPreset", true, id );
    UpdateStatusBar();
    m_galCanvas->Refresh();
}


double EDA_DRAW_FRAME::GetZoom()
{
    return GetScreen()->GetZoom();
}


void EDA_DRAW_FRAME::DisplayToolMsg( const wxString& msg )
{
    m_toolMsg = msg;
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

    // Change m_canvas cursor if requested.
    if( m_canvas && aCursor >= 0 )
        m_canvas->SetCurrentCursor( aCursor );

    // Change GAL canvas cursor if requested.
    if( aCursor >= 0 )
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
    SetToolID( ID_NO_TOOL_SELECTED, GetGalCanvas()->GetDefaultCursor(), wxEmptyString );
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
    double level = m_galCanvas->GetGAL()->GetZoomFactor();

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

    // Read grid color:
    COLOR4D wtmp = COLOR4D::UNSPECIFIED;

    if( wtmp.SetFromWxString( aCfg->Read( baseCfgName + GridColorEntryKeyword, wxT( "NONE" ) ) ) )
        SetGridColor( wtmp );

    aCfg->Read( baseCfgName + LastGridSizeIdKeyword, &m_LastGridSizeId, 0L );

    // m_LastGridSizeId is an offset, expected to be >= 0
    if( m_LastGridSizeId < 0 )
        m_LastGridSizeId = 0;

    m_UndoRedoCountMax = aCfg->Read( baseCfgName + MaxUndoItemsEntry, long( DEFAULT_MAX_UNDO_ITEMS ) );

    aCfg->Read( baseCfgName + FirstRunShownKeyword, &m_firstRunDialogSetting, 0L );

    m_galDisplayOptions.ReadConfig( *cmnCfg, *aCfg, baseCfgName, this );
}


void EDA_DRAW_FRAME::SaveSettings( wxConfigBase* aCfg )
{
    EDA_BASE_FRAME::SaveSettings( aCfg );

    wxString baseCfgName = ConfigBaseName();

    aCfg->Write( baseCfgName + UserUnitsEntryKeyword, (int) m_UserUnits );
    aCfg->Write( baseCfgName + ShowGridEntryKeyword, IsGridVisible() );
    aCfg->Write( baseCfgName + GridColorEntryKeyword,
                 GetGridColor().ToColour().GetAsString( wxC2S_CSS_SYNTAX ) );
    aCfg->Write( baseCfgName + LastGridSizeIdKeyword, ( long ) m_LastGridSizeId );
    aCfg->Write( baseCfgName + FirstRunShownKeyword, m_firstRunDialogSetting );

    if( GetScreen() )
        aCfg->Write( baseCfgName + MaxUndoItemsEntry, long( GetScreen()->GetMaxUndoItems() ) );

    m_galDisplayOptions.WriteConfig( *aCfg, baseCfgName );
}


void EDA_DRAW_FRAME::AppendMsgPanel( const wxString& textUpper, const wxString& textLower,
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

    for( MSG_PANEL_ITEM item : aList )
        m_messagePanel->AppendMessage( item );
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
    GetToolManager()->PostEvent( EVENTS::SelectedItemsModified );
}


void EDA_DRAW_FRAME::ActivateGalCanvas()
{
    KIGFX::GAL* gal = GetGalCanvas()->GetGAL();

    // Set up grid settings
    gal->SetGridVisibility( IsGridVisible() );
    gal->SetGridSize( VECTOR2D( GetScreen()->GetGridSize() ) );
    gal->SetGridOrigin( VECTOR2D( GetGridOrigin() ) );

    m_canvas->SetEvtHandlerEnabled( false );
    GetGalCanvas()->SetEvtHandlerEnabled( true );

    GetGalCanvas()->StartDrawing();

    // Reset current tool on switch();
    SetNoToolSelected();
}


void EDA_DRAW_FRAME::SwitchCanvas( EDA_DRAW_PANEL_GAL::GAL_TYPE aCanvasType )
{
    GetGalCanvas()->SwitchBackend( aCanvasType );
    m_canvasType = GetGalCanvas()->GetBackend();

    ActivateGalCanvas();
}


EDA_DRAW_PANEL_GAL::GAL_TYPE EDA_DRAW_FRAME::LoadCanvasTypeSetting()
{
    EDA_DRAW_PANEL_GAL::GAL_TYPE canvasType = EDA_DRAW_PANEL_GAL::GAL_TYPE_NONE;
    wxConfigBase* cfg = Kiface().KifaceSettings();

    if( cfg )
    {
        canvasType = (EDA_DRAW_PANEL_GAL::GAL_TYPE)
                        cfg->ReadLong( GetCanvasTypeKey(), EDA_DRAW_PANEL_GAL::GAL_TYPE_NONE );
    }

    if( canvasType < EDA_DRAW_PANEL_GAL::GAL_TYPE_NONE
            || canvasType >= EDA_DRAW_PANEL_GAL::GAL_TYPE_LAST )
    {
        assert( false );
        canvasType = EDA_DRAW_PANEL_GAL::GAL_TYPE_NONE;
    }

    // Coerce the value into a GAL type when Legacy is not available
    // Default to Cairo, and on the first, user will be prompted for OpenGL
    if( canvasType == EDA_DRAW_PANEL_GAL::GAL_TYPE_NONE
            && !ADVANCED_CFG::GetCfg().AllowLegacyCanvas() )
    {
#ifdef __WXMAC__
        // Cairo renderer doesn't handle Retina displays
        canvasType = EDA_DRAW_PANEL_GAL::GAL_TYPE_OPENGL;
#else
        canvasType = EDA_DRAW_PANEL_GAL::GAL_TYPE_CAIRO;
#endif
    }

    return canvasType;
}


bool EDA_DRAW_FRAME::saveCanvasTypeSetting( EDA_DRAW_PANEL_GAL::GAL_TYPE aCanvasType )
{
    // Not all classes derived from EDA_DRAW_FRAME can save the canvas type, because some
    // have a fixed type, or do not have a option to set the canvas type (they inherit from
    // a parent frame)
    FRAME_T allowed_frames[] =
    {
        FRAME_SCH, FRAME_PCB, FRAME_PCB_MODULE_EDITOR, FRAME_GERBER
    };

    bool allow_save = false;

    for( FRAME_T frame : allowed_frames )
    {
        if( m_Ident == frame )
        {
            allow_save = true;
            break;
        }
    }

    if( !allow_save )
        return false;

    if( aCanvasType < EDA_DRAW_PANEL_GAL::GAL_TYPE_NONE
            || aCanvasType >= EDA_DRAW_PANEL_GAL::GAL_TYPE_LAST )
    {
        wxASSERT( false );
        return false;
    }

    wxConfigBase* cfg = Kiface().KifaceSettings();

    if( cfg )
        return cfg->Write( GetCanvasTypeKey(), (long) aCanvasType );

    return false;
}

//-----< BASE_SCREEN API moved here >--------------------------------------------

wxPoint EDA_DRAW_FRAME::GetCrossHairPosition( bool aInvertY ) const
{
    // subject to change, borrow from old BASE_SCREEN for now.
    VECTOR2I cursor = GetGalCanvas()->GetViewControls()->GetCursorPosition();

    return wxPoint( cursor.x, cursor.y );
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

bool EDA_DRAW_FRAME::LibraryFileBrowser( bool doOpen, wxFileName& aFilename,
                                         const wxString& wildcard, const wxString& ext,
                                         bool isDirectory )
{
    wxString prompt = doOpen ? _( "Select Library" ) : _( "New Library" );
    aFilename.SetExt( ext );

#ifndef __WXMAC__
    if( isDirectory && doOpen )
    {
        wxDirDialog dlg( this, prompt, Prj().GetProjectPath(),
                wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST );

        if( dlg.ShowModal() == wxID_CANCEL )
            return false;

        aFilename = dlg.GetPath();
        aFilename.SetExt( ext );
    }
    else
#endif
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


void EDA_DRAW_FRAME::HardRedraw()
{
    GetGalCanvas()->Refresh();
    GetGalCanvas()->Update();
}


// Factor out the calculation portion of the various BestZoom() implementations.
//
// Note that like it's forerunners this routine has an intentional side-effect: it
// sets the scroll centre position.  While I'm not happy about that, it's probably
// not worth fixing as its days are numbered (GAL canvases use a different method).
double EDA_DRAW_FRAME::bestZoom( double sizeX, double sizeY, double scaleFactor, wxPoint centre )
{
    double bestzoom = std::max( sizeX * scaleFactor / (double) m_canvas->GetClientSize().x,
                                sizeY * scaleFactor / (double) m_canvas->GetClientSize().y );

    // Take scrollbars into account
    DSIZE scrollbarSize = m_canvas->GetSize() - m_canvas->GetClientSize();
    centre.x -= int( bestzoom * scrollbarSize.x / 2.0 );
    centre.y -= int( bestzoom * scrollbarSize.y / 2.0 );

    SetScrollCenterPosition( centre );

    return bestzoom;
}


// JEY TODO: Obsolete; replace with ACTIONS::zoomFitScreen
// JEY TODO: This is the last caller of BestZoom() too....
void EDA_DRAW_FRAME::Zoom_Automatique( bool aWarpPointer )
{
    BASE_SCREEN* screen = GetScreen();

    // Set the best zoom and get center point.

    // BestZoom() can compute an illegal zoom if the client window size
    // is small, say because frame is not maximized.  So use the clamping form
    // of SetZoom():
    double bestzoom = BestZoom();
    screen->SetScalingFactor( bestzoom );

    if( !screen->m_Initialized )
        SetCrossHairPosition( GetScrollCenterPosition() );

    m_toolManager->RunAction( "common.Control.zoomFitScreen", true );
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


void DrawPageLayout( wxDC*            aDC,
                     EDA_RECT*        aClipBox,
                     const PAGE_INFO& aPageInfo,
                     const wxString&  aFullSheetName,
                     const wxString&  aFileName,
                     TITLE_BLOCK&     aTitleBlock,
                     int              aSheetCount,
                     int              aSheetNumber,
                     int              aPenWidth,
                     double           aScalar,
                     COLOR4D          aColor,
                     const wxString&  aSheetLayer )
{
    WS_DRAW_ITEM_LIST drawList;

    drawList.SetDefaultPenSize( aPenWidth );
    drawList.SetMilsToIUfactor( aScalar );
    drawList.SetSheetNumber( aSheetNumber );
    drawList.SetSheetCount( aSheetCount );
    drawList.SetFileName( aFileName );
    drawList.SetSheetName( aFullSheetName );
    drawList.SetSheetLayer( aSheetLayer );

    drawList.BuildWorkSheetGraphicList( aPageInfo, aTitleBlock );

    // Draw item list
    drawList.Draw( aClipBox, aDC, aColor );
}


void EDA_DRAW_FRAME::DrawWorkSheet( wxDC* aDC, BASE_SCREEN* aScreen, int aLineWidth,
                                    double aScalar, const wxString &aFilename,
                                    const wxString &aSheetLayer, COLOR4D aColor )
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
    COLOR4D color = ( aColor != COLOR4D::UNSPECIFIED ) ? aColor : COLOR4D( RED );

    wxPoint origin = aDC->GetDeviceOrigin();

    if( aScreen->m_IsPrinting && origin.y > 0 )
    {
        aDC->SetDeviceOrigin( 0, 0 );
        aDC->SetAxisOrientation( true, false );
    }

    DrawPageLayout( aDC, m_canvas->GetClipBox(), pageInfo,
                    GetScreenDesc(), aFilename, t_block,
                    aScreen->m_NumberOfScreens, aScreen->m_ScreenNumber,
                    aLineWidth, aScalar, color, aSheetLayer );

    if( aScreen->m_IsPrinting && origin.y > 0 )
    {
        aDC->SetDeviceOrigin( origin.x, origin.y );
        aDC->SetAxisOrientation( true, true );
    }
}


wxString EDA_DRAW_FRAME::GetScreenDesc() const
{
    // Virtual function. In basic class, returns
    // an empty string.
    return wxEmptyString;
}


const BOX2I EDA_DRAW_FRAME::GetDocumentExtents() const
{
    BOX2I rv;
    rv.SetMaximum();
    return rv;
}


bool EDA_DRAW_FRAME::saveCanvasImageToFile( const wxString& aFileName, wxBitmapType aBitmapType )
{
    return SaveCanvasImageToFile( this, aFileName, aBitmapType );
}
