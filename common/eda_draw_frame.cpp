/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2017 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2004-2023 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <core/arraydim.h>
#include <core/kicad_algo.h>
#include <dialog_shim.h>
#include <dialogs/hotkey_cycle_popup.h>
#include <eda_draw_frame.h>
#include <file_history.h>
#include <id.h>
#include <kiface_base.h>
#include <lockfile.h>
#include <macros.h>
#include <math/vector2wx.h>
#include <page_info.h>
#include <paths.h>
#include <pgm_base.h>
#include <render_settings.h>
#include <settings/app_settings.h>
#include <settings/color_settings.h>
#include <settings/common_settings.h>
#include <settings/settings_manager.h>
#include <title_block.h>
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
#include <drawing_sheet/ds_draw_item.h>
#include <widgets/msgpanel.h>
#include <widgets/properties_panel.h>
#include <wx/event.h>
#include <wx/snglinst.h>
#include <dialogs/dialog_grid_settings.h>
#include <widgets/ui_common.h>
#include <widgets/search_pane.h>
#include <wx/dirdlg.h>
#include <wx/filedlg.h>
#include <wx/msgdlg.h>
#include <wx/socket.h>

#include <wx/snglinst.h>
#include <wx/fdrepdlg.h>

#define FR_HISTORY_LIST_CNT     10   ///< Maximum size of the find/replace history stacks.


BEGIN_EVENT_TABLE( EDA_DRAW_FRAME, KIWAY_PLAYER )
    EVT_UPDATE_UI( ID_ON_GRID_SELECT, EDA_DRAW_FRAME::OnUpdateSelectGrid )
    EVT_UPDATE_UI( ID_ON_ZOOM_SELECT, EDA_DRAW_FRAME::OnUpdateSelectZoom )

    EVT_ACTIVATE( EDA_DRAW_FRAME::onActivate )
END_EVENT_TABLE()


bool EDA_DRAW_FRAME::m_openGLFailureOccured = false;


EDA_DRAW_FRAME::EDA_DRAW_FRAME( KIWAY* aKiway, wxWindow* aParent, FRAME_T aFrameType,
                                const wxString& aTitle, const wxPoint& aPos, const wxSize& aSize,
                                long aStyle, const wxString& aFrameName,
                                const EDA_IU_SCALE& aIuScale ) :
    KIWAY_PLAYER( aKiway, aParent, aFrameType, aTitle, aPos, aSize, aStyle, aFrameName, aIuScale ),
    m_socketServer( nullptr )
{
    m_mainToolBar         = nullptr;
    m_drawToolBar         = nullptr;
    m_optionsToolBar      = nullptr;
    m_auxiliaryToolBar    = nullptr;
    m_gridSelectBox       = nullptr;
    m_zoomSelectBox       = nullptr;
    m_searchPane          = nullptr;
    m_undoRedoCountMax    = DEFAULT_MAX_UNDO_ITEMS;

    m_canvasType          = EDA_DRAW_PANEL_GAL::GAL_TYPE_NONE;
    m_canvas              = nullptr;
    m_toolDispatcher      = nullptr;
    m_messagePanel        = nullptr;
    m_currentScreen       = nullptr;
    m_showBorderAndTitleBlock = false;  // true to display reference sheet.
    m_gridColor           = COLOR4D( DARKGRAY );   // Default grid color
    m_drawBgColor         = COLOR4D( BLACK );   // the background color of the draw canvas:
                                                // BLACK for Pcbnew, BLACK or WHITE for Eeschema
    m_colorSettings       = nullptr;
    m_msgFrameHeight      = EDA_MSG_PANEL::GetRequiredHeight( this );
    m_polarCoords         = false;
    m_findReplaceData     = std::make_unique<EDA_SEARCH_DATA>();
    m_hotkeyPopup         = nullptr;
    m_propertiesPanel     = nullptr;

    SetUserUnits( EDA_UNITS::MILLIMETRES );

    m_auimgr.SetFlags( wxAUI_MGR_DEFAULT );

    CreateStatusBar( 8 )->SetDoubleBuffered( true );

    // set the size of the status bar subwindows:

    wxWindow* stsbar = GetStatusBar();
    int       spacer = KIUI::GetTextSize( wxT( "M" ), stsbar ).x * 2;

    int dims[] = {

        // remainder of status bar on far left is set to a default or whatever is left over.
        -1,

        // When using GetTextSize() remember the width of character '1' is not the same
        // as the width of '0' unless the font is fixed width, and it usually won't be.

        // zoom:
        KIUI::GetTextSize( wxT( "Z 762000" ), stsbar ).x + spacer,

        // cursor coords
        KIUI::GetTextSize( wxT( "X 1234.1234  Y 1234.1234" ), stsbar ).x + spacer,

        // delta distances
        KIUI::GetTextSize( wxT( "dx 1234.1234  dy 1234.1234  dist 1234.1234" ), stsbar ).x + spacer,

        // grid size
        KIUI::GetTextSize( wxT( "grid X 1234.1234  Y 1234.1234" ), stsbar ).x + spacer,

        // units display, Inches is bigger than mm
        KIUI::GetTextSize( _( "Inches" ), stsbar ).x + spacer,

        // Size for the "Current Tool" panel; longest string from SetTool()
        KIUI::GetTextSize( wxT( "Add layer alignment target" ), stsbar ).x + spacer,

        // constraint mode
        KIUI::GetTextSize( _( "Constrain to H, V, 45" ), stsbar ).x + spacer
    };

    SetStatusWidths( arrayDim( dims ), dims );
    stsbar->SetFont( KIUI::GetStatusFont( this ) );

    // Create child subwindows.
    GetClientSize( &m_frameSize.x, &m_frameSize.y );
    m_framePos.x   = m_framePos.y = 0;
    m_frameSize.y -= m_msgFrameHeight;

    m_messagePanel  = new EDA_MSG_PANEL( this, -1, wxPoint( 0, m_frameSize.y ),
                                         wxSize( m_frameSize.x, m_msgFrameHeight ) );

    m_messagePanel->SetBackgroundColour( COLOR4D( LIGHTGRAY ).ToColour() );

#if wxCHECK_VERSION( 3, 1, 3 )
    Bind( wxEVT_DPI_CHANGED,
          [&]( wxDPIChangedEvent& )
          {
              wxMoveEvent dummy;
              OnMove( dummy );

              // we need to kludge the msg panel to the correct size again
              // especially important even for first launches as the constructor of the window
              // here usually doesn't have the correct dpi awareness yet
              m_frameSize.y += m_msgFrameHeight;
              m_msgFrameHeight = EDA_MSG_PANEL::GetRequiredHeight( this );
              m_frameSize.y -= m_msgFrameHeight;

              m_messagePanel->SetPosition( wxPoint( 0, m_frameSize.y ) );
              m_messagePanel->SetSize( m_frameSize.x, m_msgFrameHeight );
          } );
#endif
}


EDA_DRAW_FRAME::~EDA_DRAW_FRAME()
{
    saveCanvasTypeSetting( m_canvasType );

    delete m_actions;
    delete m_toolManager;
    delete m_toolDispatcher;
    delete m_canvas;

    delete m_currentScreen;
    m_currentScreen = nullptr;

    m_auimgr.UnInit();

    ReleaseFile();
}


void EDA_DRAW_FRAME::ReleaseFile()
{
    if( m_file_checker.get() != nullptr )
        m_file_checker->UnlockFile();
}


bool EDA_DRAW_FRAME::LockFile( const wxString& aFileName )
{
    // We need to explicitly reset here to get the deletion before
    // we create a new unique_ptr that may be for the same file
    m_file_checker.reset();

    m_file_checker = std::make_unique<LOCKFILE>( aFileName );

    if( !m_file_checker->Valid() && m_file_checker->IsLockedByMe() )
    {
        // If we cannot acquire the lock but we appear to be the one who
        // locked it, check to see if there is another KiCad instance running.
        // If there is not, then we can override the lock.  This could happen if
        // KiCad crashed or was interrupted
        if( !Pgm().SingleInstance()->IsAnotherRunning() )
            m_file_checker->OverrideLock();
    }
    // If the file is valid, return true.  This could mean that the file is
    // locked or it could mean that the file is read-only
    return m_file_checker->Valid();
}


void EDA_DRAW_FRAME::ScriptingConsoleEnableDisable()
{
    KIWAY_PLAYER* frame = Kiway().Player( FRAME_PYTHON, false );

    wxRect  rect = GetScreenRect();
    wxPoint center = rect.GetPosition() + rect.GetSize() / 2;

    if( !frame )
    {
        frame = Kiway().Player( FRAME_PYTHON, true, Kiway().GetTop() );

        // If we received an error in the CTOR due to Python-ness, don't crash
        if( !frame )
            return;

        if( !frame->IsVisible() )
            frame->Show( true );

        // On Windows, Raise() does not bring the window on screen, when iconized
        if( frame->IsIconized() )
            frame->Iconize( false );

        frame->Raise();
        frame->SetPosition( center - frame->GetSize() / 2 );

        return;
    }

    frame->Show( !frame->IsVisible() );
    frame->SetPosition( center - frame->GetSize() / 2 );
}


bool EDA_DRAW_FRAME::IsScriptingConsoleVisible()
{
    KIWAY_PLAYER* frame = Kiway().Player( FRAME_PYTHON, false );
    return frame && frame->IsVisible();
}


void EDA_DRAW_FRAME::unitsChangeRefresh()
{
    // Notify all tools the units have changed
    if( m_toolManager )
        m_toolManager->RunAction( ACTIONS::updateUnits );

    UpdateStatusBar();
    UpdateMsgPanel();
    UpdateProperties();
}


void EDA_DRAW_FRAME::ToggleUserUnits()
{
    if( m_toolManager->GetTool<COMMON_TOOLS>() )
    {
        TOOL_EVENT dummy;
        m_toolManager->GetTool<COMMON_TOOLS>()->ToggleUnits( dummy );
    }
    else
    {
        SetUserUnits( GetUserUnits() == EDA_UNITS::INCHES ? EDA_UNITS::MILLIMETRES
                                                          : EDA_UNITS::INCHES );
        unitsChangeRefresh();

        wxCommandEvent e( EDA_EVT_UNITS_CHANGED );
        ProcessEventLocally( e );
    }
}


void EDA_DRAW_FRAME::CommonSettingsChanged( bool aEnvVarsChanged, bool aTextVarsChanged )
{
    EDA_BASE_FRAME::CommonSettingsChanged( aEnvVarsChanged, aTextVarsChanged );

    COMMON_SETTINGS*      settings = Pgm().GetCommonSettings();
    KIGFX::VIEW_CONTROLS* viewControls = GetCanvas()->GetViewControls();

    if( m_supportsAutoSave && m_autoSaveTimer->IsRunning() )
    {
        if( GetAutoSaveInterval() > 0 )
        {
            m_autoSaveTimer->Start( GetAutoSaveInterval() * 1000, wxTIMER_ONE_SHOT );
        }
        else
        {
            m_autoSaveTimer->Stop();
            m_autoSavePending = false;
        }
    }

    viewControls->LoadSettings();

    m_galDisplayOptions.ReadCommonConfig( *settings, this );

#ifndef __WXMAC__
    resolveCanvasType();

    if( m_canvasType != GetCanvas()->GetBackend() )
    {
        // Try to switch (will automatically fallback if necessary)
        SwitchCanvas( m_canvasType );
        EDA_DRAW_PANEL_GAL::GAL_TYPE newGAL = GetCanvas()->GetBackend();
        bool                         success = newGAL == m_canvasType;

        if( !success )
        {
            m_canvasType = newGAL;
            m_openGLFailureOccured = true; // Store failure for other EDA_DRAW_FRAMEs
        }
    }
#endif

    // Notify all tools the preferences have changed
    if( m_toolManager )
        m_toolManager->RunAction( ACTIONS::updatePreferences );
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

    if( m_gridSelectBox == nullptr )
        return;

    // Update grid values with the current units setting.
    m_gridSelectBox->Clear();
    wxArrayString gridsList;

    wxCHECK( config(), /* void */ );

    GRID_MENU::BuildChoiceList( &gridsList, config(), this );

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
    if( m_gridSelectBox == nullptr )
        return;

    wxCHECK( config(), /* void */ );

    int idx = config()->m_Window.grid.last_size_idx;
    idx = alg::clamp( 0, idx, (int) m_gridSelectBox->GetCount() - 1 );

    if( idx != m_gridSelectBox->GetSelection() )
        m_gridSelectBox->SetSelection( idx );
}



void EDA_DRAW_FRAME::OnUpdateSelectZoom( wxUpdateUIEvent& aEvent )
{
    // No need to update the grid select box if it doesn't exist or the grid setting change
    // was made using the select box.
    if( m_zoomSelectBox == nullptr )
        return;

    double zoom = GetCanvas()->GetGAL()->GetZoomFactor();

    wxCHECK( config(), /* void */ );

    const std::vector<double>& zoomList = config()->m_Window.zoom_factors;
    int curr_selection = m_zoomSelectBox->GetSelection();
    int new_selection = 0;      // select zoom auto
    double last_approx = 1e9;   // large value to start calculation

    // Search for the nearest available value to the current zoom setting, and select it
    for( size_t jj = 0; jj < zoomList.size(); ++jj )
    {
        double rel_error = std::fabs( zoomList[jj] - zoom ) / zoom;

        if( rel_error < last_approx )
        {
            last_approx = rel_error;
            // zoom IDs in m_zoomSelectBox start with 1 (leaving 0 for auto-zoom choice)
            new_selection = jj+1;
        }
    }

    if( curr_selection != new_selection )
        m_zoomSelectBox->SetSelection( new_selection );
}

void EDA_DRAW_FRAME::PrintPage( const RENDER_SETTINGS* aSettings )
{
    DisplayErrorMessage( this, wxT("EDA_DRAW_FRAME::PrintPage() error") );
}


void EDA_DRAW_FRAME::OnSelectGrid( wxCommandEvent& event )
{
    wxCHECK_RET( m_gridSelectBox, wxS( "m_gridSelectBox uninitialized" ) );

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

        // Give a time-slice to close the menu before opening the dialog.
        // (Only matters on some versions of GTK.)
        wxSafeYield();

        m_toolManager->RunAction( ACTIONS::gridProperties );
    }
    else
    {
        m_toolManager->RunAction( ACTIONS::gridPreset, idx );
    }

    UpdateStatusBar();
    m_canvas->Refresh();
    // Needed on Windows because clicking on m_gridSelectBox remove the focus from m_canvas
    // (Windows specific
    m_canvas->SetFocus();
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
    wxCHECK( config(), true );

    return config()->m_Window.grid.show;
}


void EDA_DRAW_FRAME::SetGridVisibility( bool aVisible )
{
    wxCHECK( config(), /* void */ );

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


bool EDA_DRAW_FRAME::IsGridOverridden() const
{
    wxCHECK( config(), false );

    return config()->m_Window.grid.overrides_enabled;
}


void EDA_DRAW_FRAME::SetGridOverrides( bool aOverride )
{
    wxCHECK( config(), /* void */ );

    config()->m_Window.grid.overrides_enabled = aOverride;
}


void EDA_DRAW_FRAME::UpdateZoomSelectBox()
{
    if( m_zoomSelectBox == nullptr )
        return;

    double zoom = m_canvas->GetGAL()->GetZoomFactor();

    m_zoomSelectBox->Clear();
    m_zoomSelectBox->Append( _( "Zoom Auto" ) );
    m_zoomSelectBox->SetSelection( 0 );

    wxCHECK( config(), /* void */ );

    for( unsigned i = 0;  i < config()->m_Window.zoom_factors.size();  ++i )
    {
        double current = config()->m_Window.zoom_factors[i];

        m_zoomSelectBox->Append( wxString::Format( _( "Zoom %.2f" ), current ) );

        if( zoom == current )
            m_zoomSelectBox->SetSelection( i + 1 );
    }
}


void EDA_DRAW_FRAME::OnSelectZoom( wxCommandEvent& event )
{
    wxCHECK_RET( m_zoomSelectBox, wxS( "m_zoomSelectBox uninitialized" ) );

    int id = m_zoomSelectBox->GetCurrentSelection();

    if( id < 0 || !( id < (int)m_zoomSelectBox->GetCount() ) )
        return;

    m_toolManager->RunAction( ACTIONS::zoomPreset, id );
    UpdateStatusBar();
    m_canvas->Refresh();
    // Needed on Windows because clicking on m_zoomSelectBox remove the focus from m_canvas
    // (Windows specific
    m_canvas->SetFocus();
}


void EDA_DRAW_FRAME::OnMove( wxMoveEvent& aEvent )
{
    // If the window is moved to a different display, the scaling factor may change
    double oldFactor = m_galDisplayOptions.m_scaleFactor;
    m_galDisplayOptions.UpdateScaleFactor();

    if( oldFactor != m_galDisplayOptions.m_scaleFactor && m_canvas )
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

    std::shared_ptr<ZOOM_MENU> zoomMenu = std::make_shared<ZOOM_MENU>( this );
    zoomMenu->SetTool( commonTools );
    aToolMenu.RegisterSubMenu( zoomMenu );

    std::shared_ptr<GRID_MENU> gridMenu = std::make_shared<GRID_MENU>( this );
    gridMenu->SetTool( commonTools );
    aToolMenu.RegisterSubMenu( gridMenu );

    aMenu.AddMenu( zoomMenu.get(), SELECTION_CONDITIONS::ShowAlways, 1000 );
    aMenu.AddMenu( gridMenu.get(), SELECTION_CONDITIONS::ShowAlways, 1000 );
}


void EDA_DRAW_FRAME::DisplayToolMsg( const wxString& msg )
{
    SetStatusText( msg, 6 );
}


void EDA_DRAW_FRAME::DisplayConstraintsMsg( const wxString& msg )
{
    SetStatusText( msg, 7 );
}


void EDA_DRAW_FRAME::DisplayGridMsg()
{
    wxString msg;

    msg.Printf( _( "grid %s" ), MessageTextFromValue( GetCanvas()->GetGAL()->GetGridSize().x,
                                                      false ) );

    SetStatusText( msg, 4 );
}


void EDA_DRAW_FRAME::DisplayUnitsMsg()
{
    wxString msg;

    switch( GetUserUnits() )
    {
    case EDA_UNITS::INCHES:      msg = _( "inches" ); break;
    case EDA_UNITS::MILS:        msg = _( "mils" );   break;
    case EDA_UNITS::MILLIMETRES: msg = _( "mm" );     break;
    default:                     msg = _( "Units" );  break;
    }

    SetStatusText( msg, 5 );
}


void EDA_DRAW_FRAME::OnSize( wxSizeEvent& SizeEv )
{
    EDA_BASE_FRAME::OnSize( SizeEv );

    m_frameSize = GetClientSize( );

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
    double zoom = m_canvas->GetGAL()->GetZoomFactor();
    return wxString::Format( wxT( "Z %.2f" ), zoom );
}


void EDA_DRAW_FRAME::LoadSettings( APP_SETTINGS_BASE* aCfg )
{
    EDA_BASE_FRAME::LoadSettings( aCfg );

    COMMON_SETTINGS* cmnCfg = Pgm().GetCommonSettings();
    WINDOW_SETTINGS* window = GetWindowSettings( aCfg );

    // Read units used in dialogs and toolbars
    SetUserUnits( static_cast<EDA_UNITS>( aCfg->m_System.units ) );

    m_undoRedoCountMax = aCfg->m_System.max_undo_items;

    m_galDisplayOptions.ReadConfig( *cmnCfg, *window, this );

    m_findReplaceData->findString = aCfg->m_FindReplace.find_string;
    m_findReplaceData->replaceString = aCfg->m_FindReplace.replace_string;
    m_findReplaceData->matchMode =
            static_cast<EDA_SEARCH_MATCH_MODE>( aCfg->m_FindReplace.match_mode );
    m_findReplaceData->matchCase = aCfg->m_FindReplace.match_case;
    m_findReplaceData->searchAndReplace = aCfg->m_FindReplace.search_and_replace;

    for( const wxString& s : aCfg->m_FindReplace.find_history )
        m_findStringHistoryList.Add( s );

    for( const wxString& s : aCfg->m_FindReplace.replace_history )
        m_replaceStringHistoryList.Add( s );
}


void EDA_DRAW_FRAME::SaveSettings( APP_SETTINGS_BASE* aCfg )
{
    EDA_BASE_FRAME::SaveSettings( aCfg );

    WINDOW_SETTINGS* window = GetWindowSettings( aCfg );

    aCfg->m_System.units = static_cast<int>( GetUserUnits() );
    aCfg->m_System.max_undo_items = GetMaxUndoItems();

    m_galDisplayOptions.WriteConfig( *window );

    aCfg->m_FindReplace.search_and_replace = m_findReplaceData->searchAndReplace;

    aCfg->m_FindReplace.find_string = m_findReplaceData->findString;
    aCfg->m_FindReplace.replace_string = m_findReplaceData->replaceString;

    aCfg->m_FindReplace.find_history.clear();
    aCfg->m_FindReplace.replace_history.clear();

    for( size_t i = 0; i < m_findStringHistoryList.GetCount() && i < FR_HISTORY_LIST_CNT; i++ )
    {
        aCfg->m_FindReplace.find_history.push_back( m_findStringHistoryList[ i ].ToStdString() );
    }

    for( size_t i = 0; i < m_replaceStringHistoryList.GetCount() && i < FR_HISTORY_LIST_CNT; i++ )
    {
        aCfg->m_FindReplace.replace_history.push_back(
                m_replaceStringHistoryList[ i ].ToStdString() );
    }

    // Save the units used in this frame
    if( m_toolManager )
    {
        if( COMMON_TOOLS* cmnTool = m_toolManager->GetTool<COMMON_TOOLS>() )
        {
            aCfg->m_System.last_imperial_units = static_cast<int>( cmnTool->GetLastImperialUnits() );
            aCfg->m_System.last_metric_units   = static_cast<int>( cmnTool->GetLastMetricUnits() );
        }
    }
}


void EDA_DRAW_FRAME::AppendMsgPanel( const wxString& aTextUpper, const wxString& aTextLower,
                                     int aPadding )
{
    if( m_messagePanel && !m_isClosing )
        m_messagePanel->AppendMessage( aTextUpper, aTextLower, aPadding );
}


void EDA_DRAW_FRAME::ClearMsgPanel()
{
    if( m_messagePanel && !m_isClosing )
        m_messagePanel->EraseMsgBox();
}


void EDA_DRAW_FRAME::SetMsgPanel( const std::vector<MSG_PANEL_ITEM>& aList )
{
    if( m_messagePanel && !m_isClosing )
    {
        m_messagePanel->EraseMsgBox();

        for( const MSG_PANEL_ITEM& item : aList )
            m_messagePanel->AppendMessage( item );
    }
}


void EDA_DRAW_FRAME::SetMsgPanel( const wxString& aTextUpper, const wxString& aTextLower,
                                  int aPadding )
{
    if( m_messagePanel && !m_isClosing )
    {
        m_messagePanel->EraseMsgBox();
        m_messagePanel->AppendMessage( aTextUpper, aTextLower, aPadding );
    }
}


void EDA_DRAW_FRAME::SetMsgPanel( EDA_ITEM* aItem )
{
    wxCHECK_RET( aItem, wxT( "Invalid EDA_ITEM pointer.  Bad programmer." ) );

    std::vector<MSG_PANEL_ITEM> items;
    aItem->GetMsgPanelInfo( this, items );
    SetMsgPanel( items );
}


void EDA_DRAW_FRAME::UpdateMsgPanel()
{
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


EDA_DRAW_PANEL_GAL::GAL_TYPE EDA_DRAW_FRAME::loadCanvasTypeSetting()
{
#ifdef __WXMAC__
    // Cairo renderer doesn't handle Retina displays so there's really only one game
    // in town for Mac
    return EDA_DRAW_PANEL_GAL::GAL_TYPE_OPENGL;
#endif

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

    // Legacy canvas no longer supported.  Switch to OpenGL, falls back to Cairo on failure
    if( canvasType == EDA_DRAW_PANEL_GAL::GAL_TYPE_NONE )
        canvasType = EDA_DRAW_PANEL_GAL::GAL_TYPE_OPENGL;

    return canvasType;
}


bool EDA_DRAW_FRAME::saveCanvasTypeSetting( EDA_DRAW_PANEL_GAL::GAL_TYPE aCanvasType )
{
    // Not all classes derived from EDA_DRAW_FRAME can save the canvas type, because some
    // have a fixed type, or do not have a option to set the canvas type (they inherit from
    // a parent frame)
    static std::vector<FRAME_T> s_allowedFrames =
            {
                FRAME_SCH, FRAME_SCH_SYMBOL_EDITOR,
                FRAME_PCB_EDITOR, FRAME_FOOTPRINT_EDITOR,
                FRAME_GERBER,
                FRAME_PL_EDITOR
            };

    if( !alg::contains( s_allowedFrames, m_ident ) )
        return false;

    if( aCanvasType < EDA_DRAW_PANEL_GAL::GAL_TYPE_NONE
            || aCanvasType >= EDA_DRAW_PANEL_GAL::GAL_TYPE_LAST )
    {
        wxASSERT( false );
        return false;
    }

    if( APP_SETTINGS_BASE* cfg = Kiface().KifaceSettings() )
        cfg->m_Graphics.canvas_type = static_cast<int>( aCanvasType );

    return false;
}


VECTOR2I EDA_DRAW_FRAME::GetNearestGridPosition( const VECTOR2I& aPosition ) const
{
    const VECTOR2I& gridOrigin = GetGridOrigin();
    VECTOR2D        gridSize = GetCanvas()->GetGAL()->GetGridSize();

    double xOffset = fmod( gridOrigin.x, gridSize.x );
    int    x = KiROUND( (aPosition.x - xOffset) / gridSize.x );
    double yOffset = fmod( gridOrigin.y, gridSize.y );
    int    y = KiROUND( (aPosition.y - yOffset) / gridSize.y );

    return VECTOR2I( KiROUND( x * gridSize.x + xOffset ), KiROUND( y * gridSize.y + yOffset ) );
}


VECTOR2I EDA_DRAW_FRAME::GetNearestHalfGridPosition( const VECTOR2I& aPosition ) const
{
    const VECTOR2I& gridOrigin = GetGridOrigin();
    VECTOR2D        gridSize = GetCanvas()->GetGAL()->GetGridSize() / 2.0;

    double xOffset = fmod( gridOrigin.x, gridSize.x );
    int    x = KiROUND( (aPosition.x - xOffset) / gridSize.x );
    double yOffset = fmod( gridOrigin.y, gridSize.y );
    int    y = KiROUND( (aPosition.y - yOffset) / gridSize.y );

    return VECTOR2I( KiROUND( x * gridSize.x + xOffset ), KiROUND( y * gridSize.y + yOffset ) );
}


const BOX2I EDA_DRAW_FRAME::GetDocumentExtents( bool aIncludeAllVisible ) const
{
    return BOX2I();
}


void EDA_DRAW_FRAME::HardRedraw()
{
    // To be implemented by subclasses.
}


void EDA_DRAW_FRAME::Zoom_Automatique( bool aWarpPointer )
{
    m_toolManager->RunAction( ACTIONS::zoomFitScreen );
}


// Find the first child dialog.
std::vector<wxWindow*> EDA_DRAW_FRAME::findDialogs()
{
    std::vector<wxWindow*> dialogs;

    for( wxWindow* window : GetChildren() )
    {
        if( dynamic_cast<DIALOG_SHIM*>( window ) )
            dialogs.push_back( window );
    }

    return dialogs;
}


void EDA_DRAW_FRAME::FocusOnLocation( const VECTOR2I& aPos )
{
    bool  centerView = false;
    BOX2D r = GetCanvas()->GetView()->GetViewport();

    // Center if we're off the current view, or within 10% of its edge
    r.Inflate( - (int) r.GetWidth() / 10 );

    if( !r.Contains( aPos ) )
        centerView = true;

    std::vector<BOX2D> dialogScreenRects;

    for( wxWindow* dialog : findDialogs() )
    {
        dialogScreenRects.emplace_back( ToVECTOR2D( GetCanvas()->ScreenToClient( dialog->GetScreenPosition() ) ),
                                        ToVECTOR2D( dialog->GetSize() ) );
    }

    // Center if we're behind an obscuring dialog, or within 10% of its edge
    for( BOX2D rect : dialogScreenRects )
    {
        rect.Inflate( rect.GetWidth() / 10 );

        if( rect.Contains( GetCanvas()->GetView()->ToScreen( aPos ) ) )
            centerView = true;
    }

    if( centerView )
    {
        try
        {
            GetCanvas()->GetView()->SetCenter( aPos, dialogScreenRects );
        }
        catch( const ClipperLib::clipperException& exc )
        {
            wxLogError( wxT( "Clipper library error '%s' occurred centering object." ),
                        exc.what() );
        }
    }

    GetCanvas()->GetViewControls()->SetCrossHairCursorPosition( aPos );
}


static const wxString productName = wxT( "KiCad E.D.A.  " );


void PrintDrawingSheet( const RENDER_SETTINGS* aSettings, const PAGE_INFO& aPageInfo,
                        const wxString& aSheetName, const wxString& aSheetPath,
                        const wxString& aFileName, const TITLE_BLOCK& aTitleBlock,
                        const std::map<wxString, wxString>* aProperties, int aSheetCount,
                        const wxString& aPageNumber, double aMils2Iu, const PROJECT* aProject,
                        const wxString& aSheetLayer, bool aIsFirstPage )
{
    DS_DRAW_ITEM_LIST drawList( unityScale );

    drawList.SetDefaultPenSize( aSettings->GetDefaultPenWidth() );
    drawList.SetPlotterMilsToIUfactor( aMils2Iu );
    drawList.SetPageNumber( aPageNumber );
    drawList.SetSheetCount( aSheetCount );
    drawList.SetFileName( aFileName );
    drawList.SetSheetName( aSheetName );
    drawList.SetSheetPath( aSheetPath );
    drawList.SetSheetLayer( aSheetLayer );
    drawList.SetProject( aProject );
    drawList.SetIsFirstPage( aIsFirstPage );
    drawList.SetProperties( aProperties );

    drawList.BuildDrawItemsList( aPageInfo, aTitleBlock );

    // Draw item list
    drawList.Print( aSettings );
}


void EDA_DRAW_FRAME::PrintDrawingSheet( const RENDER_SETTINGS* aSettings, BASE_SCREEN* aScreen,
                                        const std::map<wxString, wxString>* aProperties,
                                        double aMils2Iu, const wxString &aFilename,
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

    ::PrintDrawingSheet( aSettings, GetPageSettings(), GetScreenDesc(), GetFullScreenDesc(),
                         aFilename, GetTitleBlock(), aProperties, aScreen->GetPageCount(),
                         aScreen->GetPageNumber(), aMils2Iu, &Prj(), aSheetLayer,
                         aScreen->GetVirtualPageNumber() == 1 );

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


wxString EDA_DRAW_FRAME::GetFullScreenDesc() const
{
    // Virtual function. Base class implementation returns an empty string.
    return wxEmptyString;
}


bool EDA_DRAW_FRAME::LibraryFileBrowser( bool doOpen, wxFileName& aFilename,
                                         const wxString& wildcard, const wxString& ext,
                                         bool isDirectory, bool aIsGlobal,
                                         const wxString& aGlobalPath )
{
    wxString prompt = doOpen ? _( "Select Library" ) : _( "New Library" );
    aFilename.SetExt( ext );

    wxString projectDir = Prj().IsNullProject() ? aFilename.GetPath() : Prj().GetProjectPath();
    wxString defaultDir;

    if( aIsGlobal )
    {
        if( !GetMruPath().IsEmpty() && !GetMruPath().StartsWith( projectDir ) )
            defaultDir = GetMruPath();
        else
            defaultDir = aGlobalPath;
    }
    else
    {
        if( !GetMruPath().IsEmpty() && GetMruPath().StartsWith( projectDir ) )
            defaultDir = GetMruPath();
        else
            defaultDir = projectDir;
    }

    if( isDirectory && doOpen )
    {
        wxDirDialog dlg( this, prompt, defaultDir, wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST );

        if( dlg.ShowModal() == wxID_CANCEL )
            return false;

        aFilename = dlg.GetPath();
        aFilename.SetExt( ext );
    }
    else
    {
        // Ensure the file has a dummy name, otherwise GTK will display the regex from the filter
        if( aFilename.GetName().empty() )
            aFilename.SetName( wxS( "Library" ) );

        wxFileDialog dlg( this, prompt, defaultDir, aFilename.GetFullName(),
                          wildcard, doOpen ? wxFD_OPEN | wxFD_FILE_MUST_EXIST
                                           : wxFD_SAVE | wxFD_CHANGE_DIR | wxFD_OVERWRITE_PROMPT );

        if( dlg.ShowModal() == wxID_CANCEL )
            return false;

        aFilename = dlg.GetPath();
        aFilename.SetExt( ext );
    }

    SetMruPath( aFilename.GetPath() );

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
}


void EDA_DRAW_FRAME::ShowChangedLanguage()
{
    EDA_BASE_FRAME::ShowChangedLanguage();

    if( m_searchPane )
    {
        m_searchPane->OnLanguageChange();
    }

    if( m_propertiesPanel )
        m_propertiesPanel->OnLanguageChanged();
}


void EDA_DRAW_FRAME::UpdateProperties()
{
    if( !m_propertiesPanel || !m_propertiesPanel->IsShownOnScreen() )
        return;

    m_propertiesPanel->UpdateData();
}


void EDA_DRAW_FRAME::CreateHotkeyPopup()
{
    m_hotkeyPopup = new HOTKEY_CYCLE_POPUP( this );
}


COLOR_SETTINGS* EDA_DRAW_FRAME::GetColorSettings( bool aForceRefresh ) const
{
    if( !m_colorSettings || aForceRefresh )
    {
        COLOR_SETTINGS* colorSettings = Pgm().GetSettingsManager().GetColorSettings();

        const_cast<EDA_DRAW_FRAME*>( this )->m_colorSettings = colorSettings;
    }

    return m_colorSettings;
}


void EDA_DRAW_FRAME::setupUnits( APP_SETTINGS_BASE* aCfg )
{
    COMMON_TOOLS* cmnTool = m_toolManager->GetTool<COMMON_TOOLS>();

    if( cmnTool )
    {
        // Tell the tool what the units used last session
        cmnTool->SetLastUnits( static_cast<EDA_UNITS>( aCfg->m_System.last_imperial_units ) );
        cmnTool->SetLastUnits( static_cast<EDA_UNITS>( aCfg->m_System.last_metric_units ) );
    }

    // Tell the tool what units the frame is currently using
    switch( static_cast<EDA_UNITS>( aCfg->m_System.units ) )
    {
    default:
    case EDA_UNITS::MILLIMETRES: m_toolManager->RunAction( ACTIONS::millimetersUnits ); break;
    case EDA_UNITS::INCHES:      m_toolManager->RunAction( ACTIONS::inchesUnits );      break;
    case EDA_UNITS::MILS:        m_toolManager->RunAction( ACTIONS::milsUnits );        break;
    }
}


void EDA_DRAW_FRAME::GetUnitPair( EDA_UNITS& aPrimaryUnit, EDA_UNITS& aSecondaryUnits )
{
    COMMON_TOOLS* cmnTool = m_toolManager->GetTool<COMMON_TOOLS>();

    aPrimaryUnit    = GetUserUnits();
    aSecondaryUnits = EDA_UNITS::MILS;

    if( EDA_UNIT_UTILS::IsImperialUnit( aPrimaryUnit ) )
    {
        if( cmnTool )
            aSecondaryUnits = cmnTool->GetLastMetricUnits();
        else
            aSecondaryUnits = EDA_UNITS::MILLIMETRES;
    }
    else
    {
        if( cmnTool )
            aSecondaryUnits = cmnTool->GetLastImperialUnits();
        else
            aSecondaryUnits = EDA_UNITS::MILS;
    }
}


void EDA_DRAW_FRAME::resolveCanvasType()
{
    m_canvasType = loadCanvasTypeSetting();

    // If we had an OpenGL failure this session, use the fallback GAL but don't update the
    // user preference silently:

    if( m_openGLFailureOccured && m_canvasType == EDA_DRAW_PANEL_GAL::GAL_TYPE_OPENGL )
        m_canvasType = EDA_DRAW_PANEL_GAL::GAL_FALLBACK;
}


void EDA_DRAW_FRAME::handleActivateEvent( wxActivateEvent& aEvent )
{
    // Force a refresh of the message panel to ensure that the text is the right color
    // when the window activates
    if( !IsIconized() )
        m_messagePanel->Refresh();
}


void EDA_DRAW_FRAME::onActivate( wxActivateEvent& aEvent )
{
    handleActivateEvent( aEvent );

    aEvent.Skip();
}
