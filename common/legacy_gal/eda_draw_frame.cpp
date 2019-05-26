/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2017 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2004-2019 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <lockfile.h>
#include <trace_helpers.h>
#include <wx/snglinst.h>
#include <view/view.h>
#include <tool/tool_manager.h>
#include <tool/tool_dispatcher.h>
#include <tool/action_menu.h>
#include <tool/actions.h>
#include <wx/clipbrd.h>
#include <ws_draw_item.h>
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
    EVT_MOUSEWHEEL( EDA_DRAW_FRAME::OnMouseEvent )
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
    m_UserUnits           = MILLIMETRES;
    m_PolarCoords         = false;

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

    m_galDisplayOptions.ReadCommonConfig( *settings, this );
}


void EDA_DRAW_FRAME::EraseMsgBox()
{
    if( m_messagePanel )
        m_messagePanel->EraseMsgBox();
}


void EDA_DRAW_FRAME::OnActivate( wxActivateEvent& event )
{
    // TODO Obsolete!
    event.Skip();   // required under wxMAC
}


void EDA_DRAW_FRAME::OnMenuOpen( wxMenuEvent& event )
{
    // On wxWidgets 3.0.x Windows, EVT_MENU_OPEN and EVT_MENU_HIGHLIGHT events are not
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
    else // if( event.GetEventType() == wxEVT_MENU_CLOSE )
    {
        currentMenu = nullptr;
    }
//#endif

    event.Skip();
}


void EDA_DRAW_FRAME::SkipNextLeftButtonReleaseEvent()
{
   m_canvas->SetIgnoreLeftButtonReleaseEvent( true );
}


void EDA_DRAW_FRAME::OnToggleGridState( wxCommandEvent& aEvent )
{
    wxFAIL_MSG( "Obsolete!  Should go through EDITOR_CONTROL." );
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


void EDA_DRAW_FRAME::OnToggleCrossHairStyle( wxCommandEvent& aEvent )
{
    wxFAIL_MSG( "Obsolete!  Should go through EDITOR_CONTROL." );
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
}


void EDA_DRAW_FRAME::OnSelectGrid( wxCommandEvent& event )
{
    wxFAIL_MSG( "Obsolete!  Should go through ToolManager." );
}


void EDA_DRAW_FRAME::OnSelectZoom( wxCommandEvent& event )
{
    wxFAIL_MSG( "Obsolete!  Should go through ToolManager." );
}


double EDA_DRAW_FRAME::GetZoom()
{
    return GetScreen()->GetZoom();
}


void EDA_DRAW_FRAME::OnMouseEvent( wxMouseEvent& event )
{
    event.Skip();
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
    wxFAIL_MSG( "Obsolete!  Should go through COMMON_TOOLS." );
    return aPosition;
}


void EDA_DRAW_FRAME::SetNextGrid()
{
    wxFAIL_MSG( "Obsolete!  Should go through COMMON_TOOLS." );
}


void EDA_DRAW_FRAME::SetPrevGrid()
{
    wxFAIL_MSG( "Obsolete!  Should go through COMMON_TOOLS." );
}


void EDA_DRAW_FRAME::SetPresetGrid( int aIndex )
{
    wxFAIL_MSG( "Obsolete!  Should go through COMMON_TOOLS." );
}


int EDA_DRAW_FRAME::BlockCommand( EDA_KEY key )
{
    wxFAIL_MSG( "Obsolete; how'd we get here?" );
    return 0;
}


void EDA_DRAW_FRAME::InitBlockPasteInfos()
{
    wxFAIL_MSG( "Obsolete; how'd we get here?" );
}


bool EDA_DRAW_FRAME::HandleBlockBegin( wxDC* , EDA_KEY , const wxPoint& , int  )
{
    wxFAIL_MSG( "Obsolete; how'd we get here?" );
    return false;
}


void EDA_DRAW_FRAME::HandleBlockPlace( wxDC* DC )
{
    wxFAIL_MSG( "Obsolete; how'd we get here?" );
}


bool EDA_DRAW_FRAME::HandleBlockEnd( wxDC* DC )
{
    wxFAIL_MSG( "Obsolete; how'd we get here?" );
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
    // returns a human readable value which can be displayed as zoom
    // level indicator in dialogs.
    return wxString::Format( wxT( "Z %.2f" ), m_galCanvas->GetGAL()->GetZoomFactor() );
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

    m_galDisplayOptions.ReadConfig( *cmnCfg, *aCfg, baseCfgName, this );
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

    m_messagePanel->EraseMsgBox();

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
    galCanvas->StartDrawing();

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

    return use_gal;
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
        wxASSERT( false );
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
        FRAME_SCH, FRAME_PCB, FRAME_PCB_MODULE_EDITOR
    };

    bool allow_save = false;

    for( int ii = 0; ii < 3; ii++ )
    {
        if( m_Ident == allowed_frames[ii] )
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
    VECTOR2I cursor = GetGalCanvas()->GetViewControls()->GetCursorPosition();
    return wxPoint( cursor.x, aInvertY ? -cursor.y : cursor.y );
}


void EDA_DRAW_FRAME::SetCrossHairPosition( const wxPoint& aPosition, bool aSnapToGrid )
{
    GetGalCanvas()->GetViewControls()->SetCrossHairCursorPosition( aPosition, false );
}


wxPoint EDA_DRAW_FRAME::GetCursorPosition( bool , wxRealPoint*  ) const
{
    wxFAIL_MSG( "Obsolete; use VIEW_CONTROLS instead" );
    return wxPoint();
}


wxPoint EDA_DRAW_FRAME::GetNearestGridPosition( const wxPoint& aPosition,
                                                wxRealPoint* aGridSize ) const
{
    BASE_SCREEN* screen = GetScreen();  // virtual call
    return screen->getNearestGridPosition( aPosition, GetGridOrigin(), aGridSize );
}


wxPoint EDA_DRAW_FRAME::GetCrossHairScreenPosition() const
{
    wxFAIL_MSG( "Obsolete; use VIEW_CONTROLS instead" );
    return wxPoint();
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

void EDA_DRAW_FRAME::RefreshCrossHair( const wxPoint &aOldPos, const wxPoint &aEvtPos, wxDC* aDC )
{
    wxFAIL_MSG( "Obsolete; use CallMouseCapture() directly" );
}


bool EDA_DRAW_FRAME::GeneralControlKeyMovement( int aHotKey, wxPoint *aPos, bool aSnapToGrid )
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
    wxFAIL_MSG( "Obsolete!  Should go through COMMON_TOOLS." );
}


void EDA_DRAW_FRAME::OnZoom( wxCommandEvent& event )
{
    wxFAIL_MSG( "Obsolete!  Should go through COMMON_TOOLS." );
}


void EDA_DRAW_FRAME::AddMenuZoomAndGrid( wxMenu* MasterMenu )
{
    wxFAIL_MSG( "Obsolete!  Should go through COMMON_TOOLS." );
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

    aFrame->GetCanvas()->SetClipBox( EDA_RECT( wxPoint( 0, 0 ), wxSize( 0x7FFFFF0, 0x7FFFFF0 ) ) );

    if( DrawBlock )
        dc.SetClippingRegion( DrawArea );

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
        GRRect( m_canvas->GetClipBox(), aDC, 0, 0, pageSize.x * aScalar, pageSize.y * aScalar,
                aLineWidth, m_drawBgColor == WHITE ? LIGHTGRAY : DARKDARKGRAY );
    }

    TITLE_BLOCK t_block = GetTitleBlock();
    COLOR4D color = ( aColor != COLOR4D::UNSPECIFIED ) ? aColor : COLOR4D( RED );

    wxPoint origin = aDC->GetDeviceOrigin();

    if( aScreen->m_IsPrinting && origin.y > 0 )
    {
        aDC->SetDeviceOrigin( 0, 0 );
        aDC->SetAxisOrientation( true, false );
    }

    DrawPageLayout( aDC, m_canvas->GetClipBox(), pageInfo, GetScreenDesc(), aFilename, t_block,
                    aScreen->m_NumberOfScreens, aScreen->m_ScreenNumber, aLineWidth, aScalar,
                    color, aSheetLayer );

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
