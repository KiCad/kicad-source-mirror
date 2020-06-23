/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2017 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2004-2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <base_screen.h>
#include <bitmaps.h>
#include <confirm.h>
#include <dialog_shim.h>
#include <eda_draw_frame.h>
#include <fctsys.h>
#include <filehistory.h>
#include <id.h>
#include <kiface_i.h>
#include <lockfile.h>
#include <macros.h>
#include <msgpanel.h>
#include <page_info.h>
#include <pgm_base.h>
#include <settings/app_settings.h>
#include <settings/color_settings.h>
#include <settings/common_settings.h>
#include <settings/settings_manager.h>
#include <title_block.h>
#include <tool/action_manager.h>
#include <tool/actions.h>
#include <tool/common_tools.h>
#include <tool/grid_menu.h>
#include <tool/selection_conditions.h>
#include <tool/tool_dispatcher.h>
#include <tool/tool_manager.h>
#include <tool/tool_menu.h>
#include <tool/zoom_menu.h>
#include <trace_helpers.h>
#include <view/view.h>
#include <ws_draw_item.h>
#include <wx/snglinst.h>
#include <dialogs/dialog_grid_settings.h>

#define FR_HISTORY_LIST_CNT     10   ///< Maximum size of the find/replace history stacks.


BEGIN_EVENT_TABLE( EDA_DRAW_FRAME, KIWAY_PLAYER )
    EVT_UPDATE_UI( ID_ON_GRID_SELECT, EDA_DRAW_FRAME::OnUpdateSelectGrid )
    EVT_UPDATE_UI( ID_ON_ZOOM_SELECT, EDA_DRAW_FRAME::OnUpdateSelectZoom )
END_EVENT_TABLE()


EDA_DRAW_FRAME::EDA_DRAW_FRAME( KIWAY* aKiway, wxWindow* aParent, FRAME_T aFrameType,
                                const wxString& aTitle, const wxPoint& aPos, const wxSize& aSize,
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
    m_firstRunDialogSetting = 0;
    m_UndoRedoCountMax    = DEFAULT_MAX_UNDO_ITEMS;

    m_canvasType          = EDA_DRAW_PANEL_GAL::GAL_TYPE_NONE;
    m_canvas              = NULL;
    m_toolDispatcher      = NULL;
    m_messagePanel        = NULL;
    m_currentScreen       = NULL;
    m_showBorderAndTitleBlock = false;  // true to display reference sheet.
    m_gridColor           = COLOR4D( DARKGRAY );   // Default grid color
    m_showPageLimits      = false;
    m_drawBgColor         = COLOR4D( BLACK );   // the background color of the draw canvas:
                                                // BLACK for Pcbnew, BLACK or WHITE for eeschema
    m_colorSettings       = nullptr;
    m_MsgFrameHeight      = EDA_MSG_PANEL::GetRequiredHeight();
    m_userUnits           = EDA_UNITS::MILLIMETRES;
    m_PolarCoords         = false;
    m_findReplaceData     = new wxFindReplaceData( wxFR_DOWN );

    m_auimgr.SetFlags(wxAUI_MGR_DEFAULT);

    CreateStatusBar( 7 );

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

        // grid size
        GetTextSize( wxT( "grid X 0234.567890  Y 0234.567890" ), stsbar ).x + 10,

        // units display, Inches is bigger than mm
        GetTextSize( _( "Inches" ), stsbar ).x + 10,

            // Size for the "Current Tool" panel; longest string from SetTool()
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
    delete m_canvas;

    delete m_currentScreen;
    m_currentScreen = NULL;

    delete m_findReplaceData;

    m_auimgr.UnInit();

    ReleaseFile();
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


void EDA_DRAW_FRAME::ToggleUserUnits()
{
    SetUserUnits( m_userUnits == EDA_UNITS::INCHES ? EDA_UNITS::MILLIMETRES : EDA_UNITS::INCHES );
    unitsChangeRefresh();

    wxCommandEvent e( UNITS_CHANGED );
    ProcessEventLocally( e );
}


void EDA_DRAW_FRAME::CommonSettingsChanged( bool aEnvVarsChanged )
{
    EDA_BASE_FRAME::CommonSettingsChanged( aEnvVarsChanged );

    COMMON_SETTINGS*      settings = Pgm().GetCommonSettings();
    KIGFX::VIEW_CONTROLS* viewControls = GetCanvas()->GetViewControls();

    SetAutoSaveInterval( settings->m_System.autosave_interval );

    viewControls->LoadSettings();

    m_galDisplayOptions.ReadCommonConfig( *settings, this );
}


void EDA_DRAW_FRAME::EraseMsgBox()
{
    if( m_messagePanel )
        m_messagePanel->EraseMsgBox();
}


void EDA_DRAW_FRAME::UpdateGridSelectBox()
{
    UpdateStatusBar();
    DisplayUnitsMsg();

    if( m_gridSelectBox == NULL )
        return;

    // Update grid values with the current units setting.
    m_gridSelectBox->Clear();
    wxArrayString gridsList;

    GRID_MENU::BuildChoiceList( &gridsList, config(), GetUserUnits() != EDA_UNITS::INCHES );

    for( const wxString& grid : gridsList )
        m_gridSelectBox->Append( grid );

    m_gridSelectBox->Append( wxT( "---" ) );
    m_gridSelectBox->Append( _( "Edit User Grid..." ) );

    m_gridSelectBox->SetSelection( config()->m_Window.grid.last_size_idx );
}


void EDA_DRAW_FRAME::OnUpdateSelectGrid( wxUpdateUIEvent& aEvent )
{
    // No need to update the grid select box if it doesn't exist or the grid setting change
    // was made using the select box.
    if( m_gridSelectBox == NULL )
        return;

    int idx = config()->m_Window.grid.last_size_idx;
    idx = std::max( 0, std::min( idx, (int) m_gridSelectBox->GetCount() - 1 ) );

    if( idx != m_gridSelectBox->GetSelection() )
        m_gridSelectBox->SetSelection( idx );
}


void EDA_DRAW_FRAME::PrintPage( RENDER_SETTINGS* aSettings )
{
    wxMessageBox( wxT("EDA_DRAW_FRAME::PrintPage() error") );
}


/*
 * Respond to selections in the toolbar grid popup
 */
void EDA_DRAW_FRAME::OnSelectGrid( wxCommandEvent& event )
{
    wxCHECK_RET( m_gridSelectBox, "m_gridSelectBox uninitialized" );

    int idx = m_gridSelectBox->GetCurrentSelection();

    if( idx == int( m_gridSelectBox->GetCount() ) - 2 )
    {
        // wxWidgets will check the separator, which we don't want.
        // Re-check the current grid.
        wxUpdateUIEvent dummy;
        OnUpdateSelectGrid( dummy );
    }
    else if( idx == int( m_gridSelectBox->GetCount() ) - 1 )
    {
        // wxWidgets will check the Grid Settings... entry, which we don't want.
        // Re-check the current grid.
        wxUpdateUIEvent dummy;
        OnUpdateSelectGrid( dummy );
        // Now run the Grid Settings... dialog
        wxCommandEvent dummy2;
        OnGridSettings( dummy2 );
    }
    else
    {
        m_toolManager->RunAction( ACTIONS::gridPreset, true, idx );
    }

    UpdateStatusBar();
    m_canvas->Refresh();
}


void EDA_DRAW_FRAME::OnGridSettings( wxCommandEvent& aEvent )
{
    DIALOG_GRID_SETTINGS dlg( this );

    if( dlg.ShowModal() == wxID_OK )
    {
        UpdateStatusBar();
        GetCanvas()->Refresh();
    }
}


bool EDA_DRAW_FRAME::IsGridVisible() const
{
    return config()->m_Window.grid.show;
}


void EDA_DRAW_FRAME::SetGridVisibility( bool aVisible )
{
    config()->m_Window.grid.show = aVisible;

    // Update the display with the new grid
    if( GetCanvas() )
    {
        // Check to ensure these exist, since this function could be called before
        // the GAL and View have been created
        if( GetCanvas()->GetGAL() )
            GetCanvas()->GetGAL()->SetGridVisibility( aVisible );

        if( GetCanvas()->GetView() )
            GetCanvas()->GetView()->MarkTargetDirty( KIGFX::TARGET_NONCACHED );

        GetCanvas()->Refresh();
    }
}


void EDA_DRAW_FRAME::UpdateZoomSelectBox()
{
    if( m_zoomSelectBox == NULL )
        return;

    double zoom = m_canvas->GetGAL()->GetZoomFactor() / ZOOM_COEFF;

    m_zoomSelectBox->Clear();
    m_zoomSelectBox->Append( _( "Zoom Auto" ) );
    m_zoomSelectBox->SetSelection( 0 );

    for( unsigned i = 0;  i < config()->m_Window.zoom_factors.size();  ++i )
    {
        double current = config()->m_Window.zoom_factors[i];

        m_zoomSelectBox->Append( wxString::Format( _( "Zoom %.2f" ), current ) );

        if( zoom == current )
            m_zoomSelectBox->SetSelection( i + 1 );
    }
}


void EDA_DRAW_FRAME::OnUpdateSelectZoom( wxUpdateUIEvent& aEvent )
{
    if( m_zoomSelectBox == NULL || m_zoomSelectBox->GetParent() == NULL )
        return;

    int current = 0;    // display Auto if no match found

    // check for a match within 1%
    double zoom = GetCanvas()->GetGAL()->GetZoomFactor() / ZOOM_COEFF;

    for( unsigned i = 0; i < config()->m_Window.zoom_factors.size(); i++ )
    {
        if( std::fabs( zoom - config()->m_Window.zoom_factors[i] ) < ( zoom / 100.0 ) )
        {
            current = i + 1;
            break;
        }
    }

    if( current != m_zoomSelectBox->GetSelection() )
        m_zoomSelectBox->SetSelection( current );
}


/*
 * Respond to selections in the toolbar zoom popup
 */
void EDA_DRAW_FRAME::OnSelectZoom( wxCommandEvent& event )
{
    wxCHECK_RET( m_zoomSelectBox, "m_zoomSelectBox uninitialized" );

    int id = m_zoomSelectBox->GetCurrentSelection();

    if( id < 0 || !( id < (int)m_zoomSelectBox->GetCount() ) )
        return;

    m_toolManager->RunAction( "common.Control.zoomPreset", true, id );
    UpdateStatusBar();
    m_canvas->Refresh();
}


void EDA_DRAW_FRAME::OnMove( wxMoveEvent& aEvent )
{
    // If the window is moved to a different display, the scaling factor may change
    double oldFactor = m_galDisplayOptions.m_scaleFactor;
    m_galDisplayOptions.UpdateScaleFactor();

    if( oldFactor != m_galDisplayOptions.m_scaleFactor )
    {
        wxSize clientSize = GetClientSize();
        GetCanvas()->GetGAL()->ResizeScreen( clientSize.x, clientSize.y );
        GetCanvas()->GetView()->MarkDirty();
    }

    aEvent.Skip();
}


void EDA_DRAW_FRAME::AddStandardSubMenus( TOOL_MENU& aToolMenu )
{
    COMMON_TOOLS*     commonTools = m_toolManager->GetTool<COMMON_TOOLS>();
    CONDITIONAL_MENU& aMenu = aToolMenu.GetMenu();

    aMenu.AddSeparator( 1000 );
    aMenu.AddItem( ACTIONS::zoomCenter,    SELECTION_CONDITIONS::ShowAlways, 1000 );
    aMenu.AddItem( ACTIONS::zoomIn,        SELECTION_CONDITIONS::ShowAlways, 1000 );
    aMenu.AddItem( ACTIONS::zoomOut,       SELECTION_CONDITIONS::ShowAlways, 1000 );
    aMenu.AddItem( ACTIONS::zoomFitScreen, SELECTION_CONDITIONS::ShowAlways, 1000 );

    aMenu.AddSeparator( 1000 );

    auto zoomMenu = std::make_shared<ZOOM_MENU>( this );
    zoomMenu->SetTool( commonTools );
    aToolMenu.AddSubMenu( zoomMenu );

    auto gridMenu = std::make_shared<GRID_MENU>( this );
    gridMenu->SetTool( commonTools );
    aToolMenu.AddSubMenu( gridMenu );

    aMenu.AddMenu( zoomMenu.get(),   SELECTION_CONDITIONS::ShowAlways, 1000 );
    aMenu.AddMenu( gridMenu.get(), SELECTION_CONDITIONS::ShowAlways, 1000 );
}


void EDA_DRAW_FRAME::InitExitKey()
{
    wxAcceleratorEntry entries[1];
    entries[0].Set( wxACCEL_CTRL, int( 'Q' ), wxID_EXIT );
    wxAcceleratorTable accel( 1, entries );
    SetAcceleratorTable( accel );
}


void EDA_DRAW_FRAME::DisplayToolMsg( const wxString& msg )
{
    SetStatusText( msg, 6 );
}


/*
 * Display the grid status.
 */
void EDA_DRAW_FRAME::DisplayGridMsg()
{
    wxString line;
    wxString gridformatter;

    switch( m_userUnits )
    {
    case EDA_UNITS::INCHES:      gridformatter = "grid %.3f"; break;
    case EDA_UNITS::MILLIMETRES: gridformatter = "grid %.4f"; break;
    default:                     gridformatter = "grid %f";   break;
    }

    double grid = To_User_Unit( m_userUnits, GetCanvas()->GetGAL()->GetGridSize().x );
    line.Printf( gridformatter, grid );

    SetStatusText( line, 4 );
}


void EDA_DRAW_FRAME::DisplayUnitsMsg()
{
    wxString msg;

    switch( m_userUnits )
    {
    case EDA_UNITS::INCHES:      msg = _( "Inches" ); break;
    case EDA_UNITS::MILLIMETRES: msg = _( "mm" );     break;
    default:                     msg = _( "Units" );  break;
    }

    SetStatusText( msg, 5 );
}


void EDA_DRAW_FRAME::OnSize( wxSizeEvent& SizeEv )
{
    m_FrameSize = GetClientSize( );

    SizeEv.Skip();
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
    double zoom = m_canvas->GetGAL()->GetZoomFactor() / ZOOM_COEFF;
    return wxString::Format( wxT( "Z %.2f" ), zoom );
}


void EDA_DRAW_FRAME::LoadSettings( APP_SETTINGS_BASE* aCfg )
{
    EDA_BASE_FRAME::LoadSettings( aCfg );

    COMMON_SETTINGS* cmnCfg = Pgm().GetCommonSettings();
    WINDOW_SETTINGS* window = GetWindowSettings( aCfg );

    // Read units used in dialogs and toolbars
    SetUserUnits( static_cast<EDA_UNITS>( aCfg->m_System.units ) );

    m_UndoRedoCountMax = aCfg->m_System.max_undo_items;
    m_firstRunDialogSetting = aCfg->m_System.first_run_shown;

    m_galDisplayOptions.ReadConfig( *cmnCfg, *window, this );

    m_findReplaceData->SetFlags( aCfg->m_FindReplace.flags );
    m_findReplaceData->SetFindString( aCfg->m_FindReplace.find_string );
    m_findReplaceData->SetReplaceString( aCfg->m_FindReplace.replace_string );

    for( auto& s : aCfg->m_FindReplace.find_history )
        m_findStringHistoryList.Add( s );

    for( auto& s : aCfg->m_FindReplace.replace_history )
        m_replaceStringHistoryList.Add( s );
}


void EDA_DRAW_FRAME::SaveSettings( APP_SETTINGS_BASE* aCfg )
{
    EDA_BASE_FRAME::SaveSettings( aCfg );

    WINDOW_SETTINGS* window = GetWindowSettings( aCfg );

    aCfg->m_System.units = static_cast<int>( m_userUnits );
    aCfg->m_System.first_run_shown = m_firstRunDialogSetting;

    if( GetScreen() )
        aCfg->m_System.max_undo_items = GetScreen()->GetMaxUndoItems();

    m_galDisplayOptions.WriteConfig( *window );

    aCfg->m_FindReplace.flags = m_findReplaceData->GetFlags();
    aCfg->m_FindReplace.find_string = m_findReplaceData->GetFindString();
    aCfg->m_FindReplace.replace_string = m_findReplaceData->GetReplaceString();

    aCfg->m_FindReplace.find_history.clear();
    aCfg->m_FindReplace.replace_history.clear();

    for( size_t i = 0; i < m_findStringHistoryList.GetCount() && i < FR_HISTORY_LIST_CNT; i++ )
        aCfg->m_FindReplace.find_history.push_back( m_findStringHistoryList[ i ].ToStdString() );

    for( size_t i = 0; i < m_replaceStringHistoryList.GetCount() && i < FR_HISTORY_LIST_CNT; i++ )
        aCfg->m_FindReplace.replace_history.push_back(
                m_replaceStringHistoryList[ i ].ToStdString() );
}


void EDA_DRAW_FRAME::AppendMsgPanel( const wxString& textUpper, const wxString& textLower,
                                     COLOR4D color, int pad )
{
    if( m_messagePanel )
        m_messagePanel->AppendMessage( textUpper, textLower, color, pad );
}


void EDA_DRAW_FRAME::ClearMsgPanel()
{
    if( m_messagePanel )
        m_messagePanel->EraseMsgBox();
}


void EDA_DRAW_FRAME::SetMsgPanel( const MSG_PANEL_ITEMS& aList )
{
    if( m_messagePanel )
    {
        m_messagePanel->EraseMsgBox();

        for( const MSG_PANEL_ITEM& item : aList )
            m_messagePanel->AppendMessage( item );
    }
}


void EDA_DRAW_FRAME::SetMsgPanel( EDA_ITEM* aItem )
{
    wxCHECK_RET( aItem, wxT( "Invalid EDA_ITEM pointer.  Bad programmer." ) );

    MSG_PANEL_ITEMS items;
    aItem->GetMsgPanelInfo( this, items );
    SetMsgPanel( items );
}


void EDA_DRAW_FRAME::UpdateMsgPanel()
{
    GetToolManager()->PostEvent( EVENTS::SelectedItemsModified );
}


void EDA_DRAW_FRAME::ActivateGalCanvas()
{
    GetCanvas()->SetEvtHandlerEnabled( true );
    GetCanvas()->StartDrawing();
}


void EDA_DRAW_FRAME::SwitchCanvas( EDA_DRAW_PANEL_GAL::GAL_TYPE aCanvasType )
{
    GetCanvas()->SwitchBackend( aCanvasType );
    m_canvasType = GetCanvas()->GetBackend();

    ActivateGalCanvas();
}


EDA_DRAW_PANEL_GAL::GAL_TYPE EDA_DRAW_FRAME::LoadCanvasTypeSetting()
{
    EDA_DRAW_PANEL_GAL::GAL_TYPE canvasType = EDA_DRAW_PANEL_GAL::GAL_TYPE_NONE;
    APP_SETTINGS_BASE* cfg = Kiface().KifaceSettings();

    if( cfg )
        canvasType = static_cast<EDA_DRAW_PANEL_GAL::GAL_TYPE>( cfg->m_Graphics.canvas_type );

    if( canvasType < EDA_DRAW_PANEL_GAL::GAL_TYPE_NONE
            || canvasType >= EDA_DRAW_PANEL_GAL::GAL_TYPE_LAST )
    {
        wxASSERT( false );
        canvasType = EDA_DRAW_PANEL_GAL::GAL_TYPE_NONE;
    }

    // Legacy canvas no longer supported.  Switch to Cairo, and on the first instantiation
    // the user will be prompted to switch to OpenGL
    if( canvasType == EDA_DRAW_PANEL_GAL::GAL_TYPE_NONE )
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
        FRAME_SCH,
        FRAME_PCB_EDITOR, FRAME_FOOTPRINT_EDITOR,
        FRAME_GERBER
    };

    bool allow_save = false;

    for( unsigned ii = 0; ii < arrayDim( allowed_frames ); ii++ )
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

    APP_SETTINGS_BASE* cfg = Kiface().KifaceSettings();

    if( cfg )
        cfg->m_Graphics.canvas_type = static_cast<int>( aCanvasType );

    return false;
}


wxPoint EDA_DRAW_FRAME::GetNearestGridPosition( const wxPoint& aPosition ) const
{
    const wxPoint& gridOrigin = GetGridOrigin();
    VECTOR2D       gridSize = GetCanvas()->GetGAL()->GetGridSize();

    double xOffset = fmod( gridOrigin.x, gridSize.x );
    int    x = KiROUND( (aPosition.x - xOffset) / gridSize.x );
    double yOffset = fmod( gridOrigin.y, gridSize.y );
    int    y = KiROUND( (aPosition.y - yOffset) / gridSize.y );

    return wxPoint( KiROUND( x * gridSize.x + xOffset ), KiROUND( y * gridSize.y + yOffset ) );
}


const BOX2I EDA_DRAW_FRAME::GetDocumentExtents() const
{
    return BOX2I();
}


void EDA_DRAW_FRAME::HardRedraw()
{
    // To be implemented by subclasses.
}


void EDA_DRAW_FRAME::Zoom_Automatique( bool aWarpPointer )
{
    m_toolManager->RunAction( ACTIONS::zoomFitScreen, true );
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


void EDA_DRAW_FRAME::FocusOnLocation( const wxPoint& aPos )
{
    bool  centerView = false;
    BOX2D r = GetCanvas()->GetView()->GetViewport();

    // Center if we're off the current view, or within 10% of its edge
    r.Inflate( - (int) r.GetWidth() / 10 );

    if( !r.Contains( aPos ) )
        centerView = true;

    // Center if we're behind an obscuring dialog, or within 10% of its edge
    wxWindow* dialog = findDialog( GetChildren() );

    if( dialog )
    {
        wxRect dialogRect( GetCanvas()->ScreenToClient( dialog->GetScreenPosition() ),
                           dialog->GetSize() );
        dialogRect.Inflate( dialogRect.GetWidth() / 10 );

        if( dialogRect.Contains( (wxPoint) GetCanvas()->GetView()->ToScreen( aPos ) ) )
            centerView = true;
    }

    if( centerView )
    {
        // If a dialog partly obscures the window, then center on the uncovered area.
        if( dialog )
        {
            wxRect dialogRect( GetCanvas()->ScreenToClient( dialog->GetScreenPosition() ),
                               dialog->GetSize() );
            GetCanvas()->GetView()->SetCenter( aPos, dialogRect );
        }
        else
            GetCanvas()->GetView()->SetCenter( aPos );
    }

    GetCanvas()->GetViewControls()->SetCrossHairCursorPosition( aPos );
}


static const wxString productName = wxT( "KiCad E.D.A.  " );

void PrintPageLayout( RENDER_SETTINGS* aSettings, const PAGE_INFO& aPageInfo,
                      const wxString& aFullSheetName, const wxString& aFileName,
                      const TITLE_BLOCK& aTitleBlock, int aSheetCount, int aSheetNumber,
                      double aScalar, const PROJECT* aProject, const wxString& aSheetLayer )
{
    WS_DRAW_ITEM_LIST drawList;

    drawList.SetDefaultPenSize( aSettings->GetDefaultPenWidth() );
    drawList.SetMilsToIUfactor( aScalar );
    drawList.SetSheetNumber( aSheetNumber );
    drawList.SetSheetCount( aSheetCount );
    drawList.SetFileName( aFileName );
    drawList.SetSheetName( aFullSheetName );
    drawList.SetSheetLayer( aSheetLayer );
    drawList.SetProject( aProject );

    drawList.BuildWorkSheetGraphicList( aPageInfo, aTitleBlock );

    // Draw item list
    drawList.Print( aSettings );
}


void EDA_DRAW_FRAME::PrintWorkSheet( RENDER_SETTINGS* aSettings, BASE_SCREEN* aScreen,
                                     double aScalar, const wxString &aFilename,
                                     const wxString &aSheetLayer )
{
    if( !m_showBorderAndTitleBlock )
        return;

    wxDC*   DC = aSettings->GetPrintDC();
    wxPoint origin = DC->GetDeviceOrigin();

    if( origin.y > 0 )
    {
        DC->SetDeviceOrigin( 0, 0 );
        DC->SetAxisOrientation( true, false );
    }

    PrintPageLayout( aSettings, GetPageSettings(), GetScreenDesc(), aFilename, GetTitleBlock(),
                     aScreen->m_NumberOfScreens, aScreen->m_ScreenNumber, aScalar, &Prj(),
                     aSheetLayer );

    if( origin.y > 0 )
    {
        DC->SetDeviceOrigin( origin.x, origin.y );
        DC->SetAxisOrientation( true, true );
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


void EDA_DRAW_FRAME::RecreateToolbars()
{
    // Rebuild all toolbars, and update the checked state of check tools
    if( m_mainToolBar )
        ReCreateHToolbar();

    if( m_drawToolBar )         // Drawing tools (typically on right edge of window)
        ReCreateVToolbar();

    if( m_optionsToolBar )      // Options (typically on left edge of window)
        ReCreateOptToolbar();

    if( m_auxiliaryToolBar )    // Additional tools under main toolbar
       ReCreateAuxiliaryToolbar();

    // Update the checked state of tools
    SyncToolbars();
}


COLOR_SETTINGS* EDA_DRAW_FRAME::GetColorSettings()
{
    if( !m_colorSettings )
        m_colorSettings = Pgm().GetSettingsManager().GetColorSettings();

    return m_colorSettings;
}
