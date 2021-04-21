/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2013 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 1992-2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <bitmaps.h>
#include <bitmap_store.h>
#include <dialog_shim.h>
#include <dialogs/panel_common_settings.h>
#include <dialogs/panel_mouse_settings.h>
#include <eda_dde.h>
#include <filehistory.h>
#include <id.h>
#include <kiface_i.h>
#include <menus_helpers.h>
#include <panel_hotkeys_editor.h>
#include <paths.h>
#include <pgm_base.h>
#include <settings/app_settings.h>
#include <settings/common_settings.h>
#include <settings/settings_manager.h>
#include <pgm_base.h>
#include <project/project_local_settings.h>
#include <tool/action_manager.h>
#include <tool/action_menu.h>
#include <tool/action_toolbar.h>
#include <tool/actions.h>
#include <tool/common_control.h>
#include <tool/tool_manager.h>
#include <tool/tool_dispatcher.h>
#include <trace_helpers.h>
#include <widgets/paged_dialog.h>
#include <widgets/infobar.h>
#include <widgets/wx_aui_art_providers.h>
#include <wx/display.h>
#include <wx/stdpaths.h>
#include <wx/string.h>
#include <kiplatform/app.h>
#include <kiplatform/ui.h>

#include <functional>

wxDEFINE_EVENT( UNITS_CHANGED, wxCommandEvent );


// Minimum window size
static const wxSize minSize( FRAME_T aFrameType )
{
    switch( aFrameType )
    {
    case KICAD_MAIN_FRAME_T:
        return wxSize( 406, 354 );

    default:
        return wxSize( 500, 400 );
    }
}

static const wxSize defaultSize( FRAME_T aFrameType )
{
    switch( aFrameType )
    {
    case KICAD_MAIN_FRAME_T:
        return wxSize( 850, 540 );

    default:
        return wxSize( 1280, 720 );
    }
}


BEGIN_EVENT_TABLE( EDA_BASE_FRAME, wxFrame )
    EVT_MENU( wxID_ABOUT, EDA_BASE_FRAME::OnKicadAbout )
    EVT_MENU( wxID_PREFERENCES, EDA_BASE_FRAME::OnPreferences )

    EVT_CHAR_HOOK( EDA_BASE_FRAME::OnCharHook )
    EVT_MENU_OPEN( EDA_BASE_FRAME::OnMenuEvent )
    EVT_MENU_CLOSE( EDA_BASE_FRAME::OnMenuEvent )
    EVT_MENU_HIGHLIGHT_ALL( EDA_BASE_FRAME::OnMenuEvent )
    EVT_MOVE( EDA_BASE_FRAME::OnMove )
    EVT_MAXIMIZE( EDA_BASE_FRAME::OnMaximize )

    EVT_SYS_COLOUR_CHANGED( EDA_BASE_FRAME::onSystemColorChange )
END_EVENT_TABLE()

EDA_BASE_FRAME::EDA_BASE_FRAME( wxWindow* aParent, FRAME_T aFrameType,
                                const wxString& aTitle, const wxPoint& aPos, const wxSize& aSize,
                                long aStyle, const wxString& aFrameName, KIWAY* aKiway ) :
        wxFrame( aParent, wxID_ANY, aTitle, aPos, aSize, aStyle, aFrameName ),
        TOOLS_HOLDER(),
        KIWAY_HOLDER( aKiway, KIWAY_HOLDER::FRAME ),
        m_ident( aFrameType ),
        m_maximizeByDefault( false ),
        m_infoBar( nullptr ),
        m_settingsManager( nullptr ),
        m_fileHistory( nullptr ),
        m_hasAutoSave( false ),
        m_autoSaveState( false ),
        m_autoSaveInterval(-1 ),
        m_undoRedoCountMax( DEFAULT_MAX_UNDO_ITEMS ),
        m_userUnits( EDA_UNITS::MILLIMETRES ),
        m_isClosing( false ),
        m_isNonUserClose( false )
{
    m_autoSaveTimer = new wxTimer( this, ID_AUTO_SAVE_TIMER );
    m_mruPath       = PATHS::GetDefaultUserProjectsPath();
    m_frameSize     = defaultSize( aFrameType );

    m_auimgr.SetArtProvider( new WX_AUI_DOCK_ART() );

    m_settingsManager = &Pgm().GetSettingsManager();

    // Set a reasonable minimal size for the frame
    SetSizeHints( minSize( aFrameType ).x, minSize( aFrameType ).y, -1, -1, -1, -1 );

    // Store dimensions of the user area of the main window.
    GetClientSize( &m_frameSize.x, &m_frameSize.y );

    Connect( ID_AUTO_SAVE_TIMER, wxEVT_TIMER,
             wxTimerEventHandler( EDA_BASE_FRAME::onAutoSaveTimer ) );

    // hook wxEVT_CLOSE_WINDOW so we can call SaveSettings().  This function seems
    // to be called before any other hook for wxCloseEvent, which is necessary.
    Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( EDA_BASE_FRAME::windowClosing ) );

    initExitKey();
}


wxWindow* EDA_BASE_FRAME::findQuasiModalDialog()
{
    for( wxWindow* iter : GetChildren() )
    {
        DIALOG_SHIM* dlg = dynamic_cast<DIALOG_SHIM*>( iter );

        if( dlg && dlg->IsQuasiModal() )
            return dlg;
    }

    // FIXME: CvPcb is currently implemented on top of KIWAY_PLAYER rather than DIALOG_SHIM,
    // so we have to look for it separately.
    if( m_ident == FRAME_SCH )
    {
        wxWindow* cvpcb = wxWindow::FindWindowByName( "CvpcbFrame" );

        if( cvpcb )
            return cvpcb;
    }

    return nullptr;
}


void EDA_BASE_FRAME::windowClosing( wxCloseEvent& event )
{
    // Don't allow closing when a quasi-modal is open.
    wxWindow* quasiModal = findQuasiModalDialog();

    if( quasiModal )
    {
        // Raise and notify; don't give the user a warning regarding "quasi-modal dialogs"
        // when they have no idea what those are.
        quasiModal->Raise();
        wxBell();

        if( event.CanVeto() )
            event.Veto();

        return;
    }


    if( event.GetId() == wxEVT_QUERY_END_SESSION
        || event.GetId() == wxEVT_END_SESSION )
    {
        // End session means the OS is going to terminate us
        m_isNonUserClose = true;
    }

    if( canCloseWindow( event ) )
    {
        m_isClosing = true;
        APP_SETTINGS_BASE* cfg = config();

        if( cfg )
            SaveSettings( cfg );    // virtual, wxFrame specific

        doCloseWindow();

        // Destroy (safe delete frame) this frame only in non modal mode.
        // In modal mode, the caller will call Destroy().
        if( !IsModal() )
            Destroy();
    }
    else
    {
        if( event.CanVeto() )
            event.Veto();
    }
}


EDA_BASE_FRAME::~EDA_BASE_FRAME()
{
    delete m_autoSaveTimer;
    delete m_fileHistory;

    ClearUndoRedoList();

    SocketCleanup();

    KIPLATFORM::APP::RemoveShutdownBlockReason( this );
}


bool EDA_BASE_FRAME::ProcessEvent( wxEvent& aEvent )
{
#ifdef  __WXMAC__
    // Apple in its infinite wisdom will raise a disabled window before even passing
    // us the event, so we have no way to stop it.  Instead, we have to catch an
    // improperly ordered disabled window and quasi-modal dialog here and reorder
    // them.
    if( !IsEnabled() && IsActive() )
    {
        wxWindow* dlg = findQuasiModalDialog();
        if( dlg )
            dlg->Raise();
    }
#endif

    if( !wxFrame::ProcessEvent( aEvent ) )
        return false;

    if( IsShown() && m_hasAutoSave && IsActive() &&
        (m_autoSaveState != isAutoSaveRequired()) && (m_autoSaveInterval > 0) )
    {
        if( !m_autoSaveState )
        {
            wxLogTrace( traceAutoSave, wxT( "Starting auto save timer." ) );
            m_autoSaveTimer->Start( m_autoSaveInterval * 1000, wxTIMER_ONE_SHOT );
            m_autoSaveState = true;
        }
        else if( m_autoSaveTimer->IsRunning() )
        {
            wxLogTrace( traceAutoSave, wxT( "Stopping auto save timer." ) );
            m_autoSaveTimer->Stop();
            m_autoSaveState = false;
        }
    }

    return true;
}


void EDA_BASE_FRAME::SetAutoSaveInterval( int aInterval )
{
    m_autoSaveInterval = aInterval;

    if( m_autoSaveTimer->IsRunning() )
    {
        if( m_autoSaveInterval > 0 )
        {
            m_autoSaveTimer->Start( m_autoSaveInterval * 1000, wxTIMER_ONE_SHOT );
        }
        else
        {
            m_autoSaveTimer->Stop();
            m_autoSaveState = false;
        }
    }
}


void EDA_BASE_FRAME::onAutoSaveTimer( wxTimerEvent& aEvent )
{
    if( !doAutoSave() )
        m_autoSaveTimer->Start( m_autoSaveInterval * 1000, wxTIMER_ONE_SHOT );
}


bool EDA_BASE_FRAME::doAutoSave()
{
    wxCHECK_MSG( false, true, wxT( "Auto save timer function not overridden.  Bad programmer!" ) );
}


void EDA_BASE_FRAME::OnCharHook( wxKeyEvent& event )
{
    wxLogTrace( kicadTraceKeyEvent, "EDA_BASE_FRAME::OnCharHook %s", dump( event ) );
    // Key events can be filtered here.
    // Currently no filtering is made.
    event.Skip();
}


void EDA_BASE_FRAME::OnMenuEvent( wxMenuEvent& aEvent )
{
    if( !m_toolDispatcher )
        aEvent.Skip();
    else
        m_toolDispatcher->DispatchWxEvent( aEvent );
}


void EDA_BASE_FRAME::RegisterUIUpdateHandler( int aID, const ACTION_CONDITIONS& aConditions )
{
    UIUpdateHandler evtFunc = std::bind( &EDA_BASE_FRAME::HandleUpdateUIEvent,
                                         std::placeholders::_1,
                                         this,
                                         aConditions );

    m_uiUpdateMap[aID] = evtFunc;

    Bind( wxEVT_UPDATE_UI, evtFunc, aID );
}


void EDA_BASE_FRAME::UnregisterUIUpdateHandler( int aID )
{
    const auto it = m_uiUpdateMap.find( aID );

    if( it == m_uiUpdateMap.end() )
        return;

    Unbind( wxEVT_UPDATE_UI, it->second, aID );
}


void EDA_BASE_FRAME::HandleUpdateUIEvent( wxUpdateUIEvent& aEvent, EDA_BASE_FRAME* aFrame,
                                          ACTION_CONDITIONS aCond )
{
    bool       checkRes  = false;
    bool       enableRes = true;
    bool       showRes   = true;
    SELECTION& selection = aFrame->GetCurrentSelection();

    try
    {
        checkRes  = aCond.checkCondition( selection );
        enableRes = aCond.enableCondition( selection );
        showRes   = aCond.showCondition( selection );
    }
    catch( std::exception& )
    {
        // Something broke with the conditions, just skip the event.
        aEvent.Skip();
        return;
    }

    aEvent.Enable( enableRes );
    aEvent.Show( showRes );

    // wxWidgets 3.1.5+ includes a field in the event that says if the event supports being
    // checked, since wxMenuItems don't want to be checked unless they actually are checkable
#if wxCHECK_VERSION( 3, 1, 5 )
    if( aEvent.IsCheckable() )
        aEvent.Check( checkRes );
#else
    bool canCheck = true;

    // wxMenuItems don't want to be checked unless they actually are checkable, so we have to check to
    // see if they can be and can't just universally apply a check in this event.
    if( auto menu = dynamic_cast<wxMenu*>( aEvent.GetEventObject() ) )
        canCheck = menu->FindItem( aEvent.GetId() )->IsCheckable();

    if( canCheck )
        aEvent.Check( checkRes );
#endif
}


void EDA_BASE_FRAME::setupUIConditions()
{
    // Setup the conditions to check a language menu item
    auto isCurrentLang =
        [] ( const SELECTION& aSel, int aLangIdentifier )
        {
            return Pgm().GetSelectedLanguageIdentifier() == aLangIdentifier;
        };

    for( unsigned ii = 0;  LanguagesList[ii].m_KI_Lang_Identifier != 0; ii++ )
    {
        ACTION_CONDITIONS cond;
        cond.Check( std::bind( isCurrentLang, std::placeholders::_1,
                               LanguagesList[ii].m_WX_Lang_Identifier ) );

        RegisterUIUpdateHandler( LanguagesList[ii].m_KI_Lang_Identifier, cond );
    }
}


void EDA_BASE_FRAME::ReCreateMenuBar()
{
}


void EDA_BASE_FRAME::AddStandardHelpMenu( wxMenuBar* aMenuBar )
{
    COMMON_CONTROL* commonControl = m_toolManager->GetTool<COMMON_CONTROL>();
    ACTION_MENU*    helpMenu = new ACTION_MENU( false, commonControl );

    helpMenu->Add( ACTIONS::help );
    helpMenu->Add( ACTIONS::gettingStarted );
    helpMenu->Add( ACTIONS::listHotKeys );
    helpMenu->Add( ACTIONS::getInvolved );
    helpMenu->Add( ACTIONS::donate );
    helpMenu->Add( ACTIONS::reportBug );

    helpMenu->AppendSeparator();
    helpMenu->Add( _( "&About KiCad" ), "", wxID_ABOUT, BITMAPS::about );

    aMenuBar->Append( helpMenu, _( "&Help" ) );
}


void EDA_BASE_FRAME::ShowChangedLanguage()
{
    if( GetMenuBar() )
    {
        ReCreateMenuBar();
        GetMenuBar()->Refresh();
    }
}


void EDA_BASE_FRAME::CommonSettingsChanged( bool aEnvVarsChanged, bool aTextVarsChanged )
{
    TOOLS_HOLDER::CommonSettingsChanged( aEnvVarsChanged, aTextVarsChanged );

    COMMON_SETTINGS* settings = Pgm().GetCommonSettings();

    if( m_fileHistory )
    {
        int historySize = settings->m_System.file_history_size;
        m_fileHistory->SetMaxFiles( (unsigned) std::max( 0, historySize ) );
    }

    if( GetBitmapStore()->ThemeChanged() )
    {
        ThemeChanged();
    }

    if( GetMenuBar() )
    {
        // For icons in menus, icon scaling & hotkeys
        ReCreateMenuBar();
        GetMenuBar()->Refresh();
    }
}


void EDA_BASE_FRAME::ThemeChanged()
{
    ClearScaledBitmapCache();

    // Update all the toolbars to have new icons
    wxAuiPaneInfoArray panes = m_auimgr.GetAllPanes();

    for( size_t i = 0; i < panes.GetCount(); ++i )
    {
        if( ACTION_TOOLBAR* toolbar = dynamic_cast<ACTION_TOOLBAR*>( panes[i].window ) )
            toolbar->RefreshBitmaps();
    }
}


void EDA_BASE_FRAME::LoadWindowState( const wxString& aFileName )
{
    if( !Pgm().GetCommonSettings()->m_Session.remember_open_files )
        return;

    const PROJECT_FILE_STATE* state = Prj().GetLocalSettings().GetFileState( aFileName );

    if( state != nullptr )
    {
        LoadWindowState( state->window );
    }
}


void EDA_BASE_FRAME::LoadWindowState( const WINDOW_STATE& aState )
{
    bool wasDefault = false;

    m_framePos.x  = aState.pos_x;
    m_framePos.y  = aState.pos_y;
    m_frameSize.x = aState.size_x;
    m_frameSize.y = aState.size_y;

    wxLogTrace( traceDisplayLocation, "Config position (%d, %d) with size (%d, %d)",
                m_framePos.x, m_framePos.y, m_frameSize.x, m_frameSize.y );

    // Ensure minimum size is set if the stored config was zero-initialized
    if( m_frameSize.x < minSize( m_ident ).x || m_frameSize.y < minSize( m_ident ).y )
    {
        m_frameSize = defaultSize( m_ident );
        wasDefault  = true;

        wxLogTrace( traceDisplayLocation, "Using minimum size (%d, %d)", m_frameSize.x, m_frameSize.y );
    }

    wxLogTrace( traceDisplayLocation, "Number of displays: %d", wxDisplay::GetCount() );

    if( aState.display >= wxDisplay::GetCount() )
    {
        wxLogTrace( traceDisplayLocation, "Previous display not found" );

        // If it isn't attached, use the first display
        // Warning wxDisplay has 2 ctor variants. the parameter needs a type:
        const unsigned int index = 0;
        wxDisplay display( index );
        wxRect    clientSize = display.GetGeometry();

        m_framePos = wxDefaultPosition;

        // Ensure the window fits on the display, since the other one could have been larger
        if( m_frameSize.x > clientSize.width )
            m_frameSize.x = clientSize.width;

        if( m_frameSize.y > clientSize.height )
            m_frameSize.y = clientSize.height;
    }
    else
    {
        wxPoint upperRight( m_framePos.x + m_frameSize.x, m_framePos.y );
        wxPoint upperLeft( m_framePos.x, m_framePos.y );

        wxDisplay display( aState.display );
        wxRect clientSize = display.GetClientArea();

        // The percentage size (represented in decimal) of the region around the screen's border where
        // an upper corner is not allowed
#define SCREEN_BORDER_REGION 0.10

        int yLim      = clientSize.y + ( clientSize.height * ( 1.0 - SCREEN_BORDER_REGION ) );
        int xLimLeft  = clientSize.x + ( clientSize.width  * SCREEN_BORDER_REGION );
        int xLimRight = clientSize.x + ( clientSize.width  * ( 1.0 - SCREEN_BORDER_REGION ) );

        if( upperLeft.x  > xLimRight ||  // Upper left corner too close to right edge of screen
            upperRight.x < xLimLeft  ||  // Upper right corner too close to left edge of screen
            upperRight.y > yLim )        // Upper corner too close to the bottom of the screen
        {
            m_framePos = wxDefaultPosition;
            wxLogTrace( traceDisplayLocation, "Resetting to default position" );
        }
    }

    // Ensure Window title bar is visible
#if defined( __WXOSX__ )
    // for macOSX, the window must be below system (macOSX) toolbar
    int Ypos_min = 20;
#else
    int Ypos_min = 0;
#endif
    if( m_framePos.y < Ypos_min )
        m_framePos.y = Ypos_min;

    wxLogTrace( traceDisplayLocation, "Final window position (%d, %d) with size (%d, %d)",
                m_framePos.x, m_framePos.y, m_frameSize.x, m_frameSize.y );

    SetSize( m_framePos.x, m_framePos.y, m_frameSize.x, m_frameSize.y );

    // Center the window if we reset to default
    if( m_framePos.x == -1 )
    {
        wxLogTrace( traceDisplayLocation, "Centering window" );
        Center();
        m_framePos = GetPosition();
    }

    // Record the frame sizes in an un-maximized state
    m_normalFrameSize = m_frameSize;
    m_normalFramePos  = m_framePos;

    // Maximize if we were maximized before
    if( aState.maximized || ( wasDefault && m_maximizeByDefault ) )
    {
        wxLogTrace( traceDisplayLocation, "Maximizing window" );
        Maximize();
    }
}


void EDA_BASE_FRAME::LoadWindowSettings( const WINDOW_SETTINGS* aCfg )
{
    LoadWindowState( aCfg->state );

    if( m_hasAutoSave )
        m_autoSaveInterval = Pgm().GetCommonSettings()->m_System.autosave_interval;

    m_perspective = aCfg->perspective;
    m_mruPath = aCfg->mru_path;

    TOOLS_HOLDER::CommonSettingsChanged( false, false );
}


void EDA_BASE_FRAME::SaveWindowSettings( WINDOW_SETTINGS* aCfg )
{
    wxString        text;

    if( IsIconized() )
        return;

    wxString baseCfgName = ConfigBaseName();

    // If the window is maximized, we use the saved window size from before it was maximized
    if( IsMaximized() )
    {
        m_framePos  = m_normalFramePos;
        m_frameSize = m_normalFrameSize;
    }
    else
    {
        m_frameSize = GetWindowSize();
        m_framePos  = GetPosition();
    }

    aCfg->state.pos_x     = m_framePos.x;
    aCfg->state.pos_y     = m_framePos.y;
    aCfg->state.size_x    = m_frameSize.x;
    aCfg->state.size_y    = m_frameSize.y;
    aCfg->state.maximized = IsMaximized();
    aCfg->state.display   = wxDisplay::GetFromWindow( this );

    wxLogTrace( traceDisplayLocation, "Saving window maximized: %s", IsMaximized() ? "true" : "false" );
    wxLogTrace( traceDisplayLocation, "Saving config position (%d, %d) with size (%d, %d)",
                m_framePos.x, m_framePos.y, m_frameSize.x, m_frameSize.y );

    // TODO(JE) should auto-save in common settings be overwritten by every app?
    if( m_hasAutoSave )
        Pgm().GetCommonSettings()->m_System.autosave_interval = m_autoSaveInterval;

    // Once this is fully implemented, wxAuiManager will be used to maintain
    // the persistence of the main frame and all it's managed windows and
    // all of the legacy frame persistence position code can be removed.
    aCfg->perspective = m_auimgr.SavePerspective().ToStdString();

    aCfg->mru_path = m_mruPath;
}


void EDA_BASE_FRAME::LoadSettings( APP_SETTINGS_BASE* aCfg )
{
    LoadWindowSettings( GetWindowSettings( aCfg ) );

    // Get file history size from common settings
    int fileHistorySize = Pgm().GetCommonSettings()->m_System.file_history_size;

    // Load the recently used files into the history menu
    m_fileHistory = new FILE_HISTORY( (unsigned) std::max( 0, fileHistorySize ),
                                      ID_FILE1, ID_FILE_LIST_CLEAR );
    m_fileHistory->Load( *aCfg );
}


void EDA_BASE_FRAME::SaveSettings( APP_SETTINGS_BASE* aCfg )
{
    SaveWindowSettings( GetWindowSettings( aCfg ) );

    bool fileOpen = m_isClosing && m_isNonUserClose;

    wxString currentlyOpenedFile = GetCurrentFileName();

    if( Pgm().GetCommonSettings()->m_Session.remember_open_files && !currentlyOpenedFile.IsEmpty() )
    {
        wxFileName rfn( currentlyOpenedFile );
        rfn.MakeRelativeTo( Prj().GetProjectPath() );
        Prj().GetLocalSettings().SaveFileState( rfn.GetFullPath(), &aCfg->m_Window, fileOpen );
    }

    // Save the recently used files list
    if( m_fileHistory )
    {
        // Save the currently opened file in the file history
        if( !currentlyOpenedFile.IsEmpty() )
            UpdateFileHistory( currentlyOpenedFile );

        m_fileHistory->Save( *aCfg );
    }
}


WINDOW_SETTINGS* EDA_BASE_FRAME::GetWindowSettings( APP_SETTINGS_BASE* aCfg )
{
    return &aCfg->m_Window;
}


APP_SETTINGS_BASE* EDA_BASE_FRAME::config() const
{
    // KICAD_MANAGER_FRAME overrides this
    return Kiface().KifaceSettings();
}


const SEARCH_STACK& EDA_BASE_FRAME::sys_search()
{
    return Kiface().KifaceSearch();
}


wxString EDA_BASE_FRAME::help_name()
{
    return Kiface().GetHelpFileName();
}


void EDA_BASE_FRAME::PrintMsg( const wxString& text )
{
    SetStatusText( text );
}


void EDA_BASE_FRAME::CreateInfoBar()
{
#if defined( __WXOSX_MAC__ )
    m_infoBar = new WX_INFOBAR( GetToolCanvas() );
#else
    m_infoBar = new WX_INFOBAR( this, &m_auimgr );

    m_auimgr.AddPane( m_infoBar, EDA_PANE().InfoBar().Name( "InfoBar" ).Top().Layer(1) );
#endif
}


void EDA_BASE_FRAME::FinishAUIInitialization()
{
#if defined( __WXOSX_MAC__ )
    m_auimgr.Update();
#else
    // Call Update() to fix all pane default sizes, especially the "InfoBar" pane before
    // hiding it.
    m_auimgr.Update();

    // We don't want the infobar displayed right away
    m_auimgr.GetPane( "InfoBar" ).Hide();
    m_auimgr.Update();
#endif
}


void EDA_BASE_FRAME::ShowInfoBarError( const wxString& aErrorMsg, bool aShowCloseButton )
{
    m_infoBar->RemoveAllButtons();

    if( aShowCloseButton )
        m_infoBar->AddCloseButton();

    GetInfoBar()->ShowMessageFor( aErrorMsg, 8000, wxICON_ERROR );
}


void EDA_BASE_FRAME::ShowInfoBarError( const wxString& aErrorMsg, bool aShowCloseButton,
                                       std::function<void(void)> aCallback )
{
    m_infoBar->RemoveAllButtons();

    if( aShowCloseButton )
        m_infoBar->AddCloseButton();

    if( aCallback )
        m_infoBar->SetCallback( aCallback );

    GetInfoBar()->ShowMessageFor( aErrorMsg, 6000, wxICON_ERROR );
}


void EDA_BASE_FRAME::ShowInfoBarWarning( const wxString& aWarningMsg, bool aShowCloseButton )
{
    m_infoBar->RemoveAllButtons();

    if( aShowCloseButton )
        m_infoBar->AddCloseButton();

    GetInfoBar()->ShowMessageFor( aWarningMsg, 6000, wxICON_WARNING );
}


void EDA_BASE_FRAME::ShowInfoBarMsg( const wxString& aMsg, bool aShowCloseButton )
{
    m_infoBar->RemoveAllButtons();

    if( aShowCloseButton )
        m_infoBar->AddCloseButton();

    GetInfoBar()->ShowMessageFor( aMsg, 8000, wxICON_INFORMATION );
}


void EDA_BASE_FRAME::UpdateFileHistory( const wxString& FullFileName, FILE_HISTORY* aFileHistory )
{
    if( !aFileHistory )
        aFileHistory = m_fileHistory;

    wxASSERT( aFileHistory );

    aFileHistory->AddFileToHistory( FullFileName );

    // Update the menubar to update the file history menu
    if( !m_isClosing && GetMenuBar() )
    {
        ReCreateMenuBar();
        GetMenuBar()->Refresh();
    }
}


wxString EDA_BASE_FRAME::GetFileFromHistory( int cmdId, const wxString& type,
                                             FILE_HISTORY* aFileHistory )
{
    if( !aFileHistory )
        aFileHistory = m_fileHistory;

    wxASSERT( aFileHistory );

    int baseId = aFileHistory->GetBaseId();

    wxASSERT( cmdId >= baseId && cmdId < baseId + (int) aFileHistory->GetCount() );

    unsigned i = cmdId - baseId;

    if( i < aFileHistory->GetCount() )
    {
        wxString fn = aFileHistory->GetHistoryFile( i );

        if( wxFileName::FileExists( fn ) )
            return fn;
        else
        {
            wxString msg = wxString::Format( _( "File \"%s\" was not found." ), fn );
            wxMessageBox( msg );

            aFileHistory->RemoveFileFromHistory( i );
        }
    }

    // Update the menubar to update the file history menu
    if( GetMenuBar() )
    {
        ReCreateMenuBar();
        GetMenuBar()->Refresh();
    }

    return wxEmptyString;
}


void EDA_BASE_FRAME::ClearFileHistory( FILE_HISTORY* aFileHistory )
{
    if( !aFileHistory )
        aFileHistory = m_fileHistory;

    wxASSERT( aFileHistory );

    aFileHistory->ClearFileHistory();

    // Update the menubar to update the file history menu
    if( GetMenuBar() )
    {
        ReCreateMenuBar();
        GetMenuBar()->Refresh();
    }
}


void EDA_BASE_FRAME::OnKicadAbout( wxCommandEvent& event )
{
    void ShowAboutDialog(EDA_BASE_FRAME * aParent); // See AboutDialog_main.cpp
    ShowAboutDialog( this );
}


void EDA_BASE_FRAME::OnPreferences( wxCommandEvent& event )
{
    PAGED_DIALOG dlg( this, _( "Preferences" ), true );
    wxTreebook* book = dlg.GetTreebook();

    book->AddPage( new PANEL_COMMON_SETTINGS( &dlg, book ), _( "Common" ) );

    book->AddPage( new PANEL_MOUSE_SETTINGS( &dlg, book ), _( "Mouse and Touchpad" ) );

    PANEL_HOTKEYS_EDITOR* hotkeysPanel = new PANEL_HOTKEYS_EDITOR( this, book, false );
    book->AddPage( hotkeysPanel, _( "Hotkeys" ) );

    for( unsigned i = 0; i < KIWAY_PLAYER_COUNT;  ++i )
    {
        KIWAY_PLAYER* frame = dlg.Kiway().Player( (FRAME_T) i, false );

        if( frame )
            frame->InstallPreferences( &dlg, hotkeysPanel );
    }

    // The Kicad manager frame is not a player so we have to add it by hand
    wxWindow* manager = wxFindWindowByName( KICAD_MANAGER_FRAME_NAME );

    if( manager )
        static_cast<EDA_BASE_FRAME*>( manager )->InstallPreferences( &dlg, hotkeysPanel );

    for( size_t i = 0; i < book->GetPageCount(); ++i )
        book->GetPage( i )->Layout();

    if( dlg.ShowModal() == wxID_OK )
        dlg.Kiway().CommonSettingsChanged( false, false );
}


bool EDA_BASE_FRAME::IsWritable( const wxFileName& aFileName )
{
    wxString msg;
    wxFileName fn = aFileName;

    // Check for absence of a file path with a file name.  Unfortunately KiCad
    // uses paths relative to the current project path without the ./ part which
    // confuses wxFileName. Making the file name path absolute may be less than
    // elegant but it solves the problem.
    if( fn.GetPath().IsEmpty() && fn.HasName() )
        fn.MakeAbsolute();

    wxCHECK_MSG( fn.IsOk(), false,
                 wxT( "File name object is invalid.  Bad programmer!" ) );
    wxCHECK_MSG( !fn.GetPath().IsEmpty(), false,
                 wxT( "File name object path <" ) + fn.GetFullPath() +
                 wxT( "> is not set.  Bad programmer!" ) );

    if( fn.IsDir() && !fn.IsDirWritable() )
    {
        msg.Printf( _( "You do not have write permissions to folder \"%s\"." ),
                    fn.GetPath() );
    }
    else if( !fn.FileExists() && !fn.IsDirWritable() )
    {
        msg.Printf( _( "You do not have write permissions to save file \"%s\" to folder \"%s\"." ),
                    fn.GetFullName(), fn.GetPath() );
    }
    else if( fn.FileExists() && !fn.IsFileWritable() )
    {
        msg.Printf( _( "You do not have write permissions to save file \"%s\"." ),
                    fn.GetFullPath() );
    }

    if( !msg.IsEmpty() )
    {
        wxMessageBox( msg );
        return false;
    }

    return true;
}


void EDA_BASE_FRAME::CheckForAutoSaveFile( const wxFileName& aFileName )
{
    wxCHECK_RET( aFileName.IsOk(), wxT( "Invalid file name!" ) );

    wxFileName autoSaveFileName = aFileName;

    // Check for auto save file.
    autoSaveFileName.SetName( GetAutoSaveFilePrefix() + aFileName.GetName() );

    wxLogTrace( traceAutoSave,
                wxT( "Checking for auto save file " ) + autoSaveFileName.GetFullPath() );

    if( !autoSaveFileName.FileExists() )
        return;

    wxString msg = wxString::Format( _(
            "Well this is potentially embarrassing!\n"
            "It appears that the last time you were editing the file\n"
            "\"%s\"\n"
            "it was not saved properly.  Do you wish to restore the last saved edits you made?" ),
            aFileName.GetFullName()
        );

    int response = wxMessageBox( msg, Pgm().App().GetAppDisplayName(), wxYES_NO | wxICON_QUESTION,
                                 this );

    // Make a backup of the current file, delete the file, and rename the auto save file to
    // the file name.
    if( response == wxYES )
    {
        if( !wxRenameFile( autoSaveFileName.GetFullPath(), aFileName.GetFullPath() ) )
        {
            wxMessageBox( _( "The auto save file could not be renamed to the board file name." ),
                          Pgm().App().GetAppDisplayName(), wxOK | wxICON_EXCLAMATION, this );
        }
    }
    else
    {
        wxLogTrace( traceAutoSave,
                    wxT( "Removing auto save file " ) + autoSaveFileName.GetFullPath() );

        // Remove the auto save file when using the previous file as is.
        wxRemoveFile( autoSaveFileName.GetFullPath() );
    }
}


bool EDA_BASE_FRAME::IsContentModified()
{
    // This function should be overridden in child classes
    return false;
}


void EDA_BASE_FRAME::initExitKey()
{
    wxAcceleratorEntry entries[1];
    entries[0].Set( wxACCEL_CTRL, int( 'Q' ), wxID_EXIT );
    wxAcceleratorTable accel( 1, entries );
    SetAcceleratorTable( accel );
}


void EDA_BASE_FRAME::ClearUndoRedoList()
{
    ClearUndoORRedoList( UNDO_LIST );
    ClearUndoORRedoList( REDO_LIST );
}


void EDA_BASE_FRAME::PushCommandToUndoList( PICKED_ITEMS_LIST* aNewitem )
{
    m_undoList.PushCommand( aNewitem );

    // Delete the extra items, if count max reached
    if( m_undoRedoCountMax > 0 )
    {
        int extraitems = GetUndoCommandCount() - m_undoRedoCountMax;

        if( extraitems > 0 )
            ClearUndoORRedoList( UNDO_LIST, extraitems );
    }
}


void EDA_BASE_FRAME::PushCommandToRedoList( PICKED_ITEMS_LIST* aNewitem )
{
    m_redoList.PushCommand( aNewitem );

    // Delete the extra items, if count max reached
    if( m_undoRedoCountMax > 0 )
    {
        int extraitems = GetRedoCommandCount() - m_undoRedoCountMax;

        if( extraitems > 0 )
            ClearUndoORRedoList( REDO_LIST, extraitems );
    }
}


PICKED_ITEMS_LIST* EDA_BASE_FRAME::PopCommandFromUndoList( )
{
    return m_undoList.PopCommand();
}


PICKED_ITEMS_LIST* EDA_BASE_FRAME::PopCommandFromRedoList( )
{
    return m_redoList.PopCommand();
}


void EDA_BASE_FRAME::ChangeUserUnits( EDA_UNITS aUnits )
{
    SetUserUnits( aUnits );
    unitsChangeRefresh();

    wxCommandEvent e( UNITS_CHANGED );
    ProcessEventLocally( e );
}


void EDA_BASE_FRAME::OnMaximize( wxMaximizeEvent& aEvent )
{
    // When we maximize the window, we want to save the old information
    // so that we can add it to the settings on next window load.
    // Contrary to the documentation, this event seems to be generated
    // when the window is also being unmaximized on OSX, so we only
    // capture the size information when we maximize the window when on OSX.
#ifdef __WXOSX__
    if( !IsMaximized() )
#endif
    {
        m_normalFrameSize = GetWindowSize();
        m_normalFramePos  = GetPosition();
        wxLogTrace( traceDisplayLocation, "Maximizing window - Saving position (%d, %d) with size (%d, %d)",
                    m_normalFramePos.x, m_normalFramePos.y, m_normalFrameSize.x, m_normalFrameSize.y );
    }

    // Skip event to actually maximize the window
    aEvent.Skip();
}


wxSize EDA_BASE_FRAME::GetWindowSize()
{
#ifdef __WXGTK__
    // GTK includes the window decorations in the normal GetSize call,
    // so we have to use a GTK-specific sizing call that returns the
    // non-decorated window size.
    int width  = 0;
    int height = 0;
    GTKDoGetSize( &width, &height );

    wxSize winSize( width, height );
#else
    wxSize winSize = GetSize();
#endif

    return winSize;
}


void EDA_BASE_FRAME::HandleSystemColorChange()
{
    // Update the icon theme when the system theme changes and update the toolbars
    if( GetBitmapStore()->ThemeChanged() )
        ThemeChanged();

    // This isn't handled by ThemeChanged()
    if( GetMenuBar() )
    {
        // For icons in menus, icon scaling & hotkeys
        ReCreateMenuBar();
        GetMenuBar()->Refresh();
    }
}


void EDA_BASE_FRAME::onSystemColorChange( wxSysColourChangedEvent& aEvent )
{
    // Call the handler to update the colors used in the frame
    HandleSystemColorChange();

    // Skip the change event to ensure the rest of the window controls get it
    aEvent.Skip();
}
