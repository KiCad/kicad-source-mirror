/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2013 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 1992-2018 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <dialog_shim.h>
#include <dialogs/panel_common_settings.h>
#include <dialogs/panel_mouse_settings.h>
#include <filehistory.h>
#include <id.h>
#include <kiface_i.h>
#include <menus_helpers.h>
#include <panel_hotkeys_editor.h>
#include <pgm_base.h>
#include <settings/app_settings.h>
#include <settings/common_settings.h>
#include <settings/settings_manager.h>
#include <tool/action_manager.h>
#include <tool/action_menu.h>
#include <tool/actions.h>
#include <tool/common_control.h>
#include <tool/tool_manager.h>
#include <tool/tool_dispatcher.h>
#include <trace_helpers.h>
#include <widgets/paged_dialog.h>
#include <wx/display.h>
#include <wx/stdpaths.h>
#include <wx/string.h>

wxDEFINE_EVENT( UNITS_CHANGED, wxCommandEvent );


// Minimum window size
static const int s_minsize_x = 500;
static const int s_minsize_y = 400;


BEGIN_EVENT_TABLE( EDA_BASE_FRAME, wxFrame )
    EVT_MENU( wxID_ABOUT, EDA_BASE_FRAME::OnKicadAbout )
    EVT_MENU( wxID_PREFERENCES, EDA_BASE_FRAME::OnPreferences )

    EVT_CHAR_HOOK( EDA_BASE_FRAME::OnCharHook )
    EVT_MENU_OPEN( EDA_BASE_FRAME::OnMenuEvent )
    EVT_MENU_CLOSE( EDA_BASE_FRAME::OnMenuEvent )
    EVT_MENU_HIGHLIGHT_ALL( EDA_BASE_FRAME::OnMenuEvent )
    EVT_MOVE( EDA_BASE_FRAME::OnMove )
    EVT_MAXIMIZE( EDA_BASE_FRAME::OnMaximize )
END_EVENT_TABLE()

EDA_BASE_FRAME::EDA_BASE_FRAME( wxWindow* aParent, FRAME_T aFrameType,
                                const wxString& aTitle, const wxPoint& aPos, const wxSize& aSize,
                                long aStyle, const wxString& aFrameName, KIWAY* aKiway ) :
        wxFrame( aParent, wxID_ANY, aTitle, aPos, aSize, aStyle, aFrameName ),
        TOOLS_HOLDER(),
        KIWAY_HOLDER( aKiway, KIWAY_HOLDER::FRAME ),
        m_Ident( aFrameType ),
        m_infoBar( nullptr ),
        m_settingsManager( nullptr ),
        m_fileHistory( nullptr ),
        m_hasAutoSave( false ),
        m_autoSaveState( false ),
        m_autoSaveInterval(-1 ),
        m_UndoRedoCountMax( DEFAULT_MAX_UNDO_ITEMS ),
        m_userUnits( EDA_UNITS::MILLIMETRES )
{
    m_autoSaveTimer = new wxTimer( this, ID_AUTO_SAVE_TIMER );
    m_mruPath       = wxStandardPaths::Get().GetDocumentsDir();
    m_FrameSize     = wxSize( s_minsize_x, s_minsize_y );

    m_settingsManager = &Pgm().GetSettingsManager();

    // Set a reasonable minimal size for the frame
    SetSizeHints( s_minsize_x, s_minsize_y, -1, -1, -1, -1 );

    // Store dimensions of the user area of the main window.
    GetClientSize( &m_FrameSize.x, &m_FrameSize.y );

    Connect( ID_AUTO_SAVE_TIMER, wxEVT_TIMER,
             wxTimerEventHandler( EDA_BASE_FRAME::onAutoSaveTimer ) );

    // hook wxEVT_CLOSE_WINDOW so we can call SaveSettings().  This function seems
    // to be called before any other hook for wxCloseEvent, which is necessary.
    Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( EDA_BASE_FRAME::windowClosing ) );
}


wxWindow* EDA_BASE_FRAME::findQuasiModalDialog()
{
    for( auto& iter : GetChildren() )
    {
        DIALOG_SHIM* dlg = dynamic_cast<DIALOG_SHIM*>( iter );
        if( dlg && dlg->IsQuasiModal() )
            return dlg;
    }

    // FIXME: CvPcb is currently implemented on top of KIWAY_PLAYER rather than DIALOG_SHIM,
    // so we have to look for it separately.
    if( m_Ident == FRAME_SCH )
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

        event.Veto();
        return;
    }

    APP_SETTINGS_BASE* cfg = config();

    if( cfg )
        SaveSettings( cfg );    // virtual, wxFrame specific

    event.Skip();       // we did not "handle" the event, only eavesdropped on it.
}


EDA_BASE_FRAME::~EDA_BASE_FRAME()
{
    delete m_autoSaveTimer;
    delete m_fileHistory;

    ClearUndoRedoList();

    if( SupportsShutdownBlockReason() )
        RemoveShutdownBlockReason();
}


bool EDA_BASE_FRAME::SupportsShutdownBlockReason()
{
#if defined( _WIN32 )
    return true;
#else
    return false;
#endif
}


void EDA_BASE_FRAME::RemoveShutdownBlockReason()
{
#if defined( _WIN32 )
    // Windows: Destroys any block reason that may have existed
    ShutdownBlockReasonDestroy( GetHandle() );
#endif
}


void EDA_BASE_FRAME::SetShutdownBlockReason( const wxString& aReason )
{
#if defined( _WIN32 )
    // Windows: sets up the pretty message on the shutdown page on why it's being "blocked"
    // This is used in conjunction with handling WM_QUERYENDSESSION (wxCloseEvent)
    // ShutdownBlockReasonCreate does not block by itself

    ShutdownBlockReasonDestroy( GetHandle() ); // Destroys any existing or nonexisting reason

    if( !ShutdownBlockReasonCreate( GetHandle(), aReason.wc_str() ) )
    {
        // Nothing bad happens if this fails, at worst it uses a generic application is preventing shutdown message
        wxLogDebug( wxT( "ShutdownBlockReasonCreate failed to set reason: %s" ), aReason );
    }
#endif
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


void EDA_BASE_FRAME::ReCreateMenuBar()
{
}


void EDA_BASE_FRAME::AddStandardHelpMenu( wxMenuBar* aMenuBar )
{
    COMMON_CONTROL* commonControl = m_toolManager->GetTool<COMMON_CONTROL>();
    ACTION_MENU*    helpMenu = new ACTION_MENU( false );

    helpMenu->SetTool( commonControl );

    helpMenu->Add( ACTIONS::help );
    helpMenu->Add( ACTIONS::gettingStarted );
    helpMenu->Add( ACTIONS::listHotKeys );
    helpMenu->Add( ACTIONS::getInvolved );
    helpMenu->Add( ACTIONS::reportBug );

    helpMenu->AppendSeparator();
    helpMenu->Add( _( "&About KiCad" ), "", wxID_ABOUT, about_xpm );

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


void EDA_BASE_FRAME::CommonSettingsChanged( bool aEnvVarsChanged )
{
    TOOLS_HOLDER::CommonSettingsChanged( aEnvVarsChanged );

    COMMON_SETTINGS* settings = Pgm().GetCommonSettings();

    if( m_fileHistory )
    {
        int historySize = settings->m_System.file_history_size;
        m_fileHistory->SetMaxFiles( (unsigned) std::max( 0, historySize ) );
    }

    if( GetMenuBar() )
    {
        // For icons in menus, icon scaling & hotkeys
        ReCreateMenuBar();
        GetMenuBar()->Refresh();
    }
}


void EDA_BASE_FRAME::LoadWindowSettings( WINDOW_SETTINGS* aCfg )
{
    m_FramePos.x  = aCfg->pos_x;
    m_FramePos.y  = aCfg->pos_y;
    m_FrameSize.x = aCfg->size_x;
    m_FrameSize.y = aCfg->size_y;

    wxLogTrace( traceDisplayLocation, "Config position (%d, %d) with size (%d, %d)",
            m_FramePos.x, m_FramePos.y, m_FrameSize.x, m_FrameSize.y );

    // Ensure minimum size is set if the stored config was zero-initialized
    if( m_FrameSize.x < s_minsize_x || m_FrameSize.y < s_minsize_y )
    {
        m_FrameSize.x = s_minsize_x;
        m_FrameSize.y = s_minsize_y;

        wxLogTrace( traceDisplayLocation, "Using minimum size (%d, %d)", m_FrameSize.x, m_FrameSize.y );
    }

    wxPoint upperRight( m_FramePos.x + m_FrameSize.x, m_FramePos.y );
    wxPoint upperLeft( m_FramePos.x, m_FramePos.y );

    // Check to see if the requested display is still attached to the computer
    int leftInd  = wxDisplay::GetFromPoint( upperLeft );
    int rightInd = wxDisplay::GetFromPoint( upperRight );

    wxLogTrace( traceDisplayLocation, "Number of displays: %d", wxDisplay::GetCount() );
    wxLogTrace( traceDisplayLocation, "Previous display indices: %d and %d", leftInd, rightInd );

    if( rightInd == wxNOT_FOUND && leftInd == wxNOT_FOUND )
    {
        wxLogTrace( traceDisplayLocation, "Previous display not found" );

        // If it isn't attached, use the first display
        // Warning wxDisplay has 2 ctor variants. the parameter needs a type:
        const unsigned int index = 0;
        wxDisplay display( index );
        wxRect    clientSize = display.GetClientArea();

        wxLogDebug( "Client size (%d, %d)", clientSize.width, clientSize.height );

        m_FramePos = wxDefaultPosition;

        // Ensure the window fits on the display, since the other one could have been larger
        if( m_FrameSize.x > clientSize.width )
            m_FrameSize.x = clientSize.width;

        if( m_FrameSize.y > clientSize.height )
            m_FrameSize.y = clientSize.height;
    }
    else
    {
        wxRect clientSize;

        if( leftInd == wxNOT_FOUND )
        {
            // If the top-left point is off-screen, use the display for the top-right point
            wxDisplay display( rightInd );
            clientSize = display.GetClientArea();
        }
        else
        {
            wxDisplay display( leftInd );
            clientSize = display.GetClientArea();
        }

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
            m_FramePos = wxDefaultPosition;
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
    if( m_FramePos.y < Ypos_min )
        m_FramePos.y = Ypos_min;

    wxLogTrace( traceDisplayLocation, "Final window position (%d, %d) with size (%d, %d)",
        m_FramePos.x, m_FramePos.y, m_FrameSize.x, m_FrameSize.y );

    SetSize( m_FramePos.x, m_FramePos.y, m_FrameSize.x, m_FrameSize.y );

    // Center the window if we reset to default
    if( m_FramePos.x == -1 )
    {
        wxLogTrace( traceDisplayLocation, "Centering window" );
        Center();
        m_FramePos = GetPosition();
    }

    // Record the frame sizes in an un-maximized state
    m_NormalFrameSize = m_FrameSize;
    m_NormalFramePos  = m_FramePos;

    // Maximize if we were maximized before
    if( aCfg->maximized )
    {
        wxLogTrace( traceDisplayLocation, "Maximizing window" );
        Maximize();
    }

    if( m_hasAutoSave )
        m_autoSaveInterval = Pgm().GetCommonSettings()->m_System.autosave_interval;

    m_perspective = aCfg->perspective;
    m_mruPath = aCfg->mru_path;

    TOOLS_HOLDER::CommonSettingsChanged( false );
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
        m_FramePos  = m_NormalFramePos;
        m_FrameSize = m_NormalFrameSize;
    }
    else
    {
        m_FrameSize = GetWindowSize();
        m_FramePos  = GetPosition();
    }

    aCfg->pos_x     = m_FramePos.x;
    aCfg->pos_y     = m_FramePos.y;
    aCfg->size_x    = m_FrameSize.x;
    aCfg->size_y    = m_FrameSize.y;
    aCfg->maximized = IsMaximized();

    wxLogTrace( traceDisplayLocation, "Saving window maximized: %s", IsMaximized() ? "true" : "false" );
    wxLogTrace( traceDisplayLocation, "Saving config position (%d, %d) with size (%d, %d)",
            m_FramePos.x, m_FramePos.y, m_FrameSize.x, m_FrameSize.y );

    // TODO(JE) should auto-save in common settings be overwritten by every app?
    if( m_hasAutoSave )
        Pgm().GetCommonSettings()->m_System.autosave_interval = m_autoSaveInterval;

    // Once this is fully implemented, wxAuiManager will be used to maintain
    // the persistance of the main frame and all it's managed windows and
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

    // Save the recently used files list
    if( m_fileHistory )
    {
        // Save the currently opened file in the file history
        wxString currentlyOpenedFile = GetCurrentFileName();

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


void EDA_BASE_FRAME::UpdateFileHistory( const wxString& FullFileName, FILE_HISTORY* aFileHistory )
{
    if( !aFileHistory )
        aFileHistory = m_fileHistory;

    wxASSERT( aFileHistory );

    aFileHistory->AddFileToHistory( FullFileName );

    // Update the menubar to update the file history menu
    if( GetMenuBar() )
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
    PAGED_DIALOG dlg( this, _( "Preferences" ) );
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
        dlg.Kiway().CommonSettingsChanged( false );
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
                    GetChars( fn.GetPath() ) );
    }
    else if( !fn.FileExists() && !fn.IsDirWritable() )
    {
        msg.Printf( _( "You do not have write permissions to save file \"%s\" to folder \"%s\"." ),
                    GetChars( fn.GetFullName() ), GetChars( fn.GetPath() ) );
    }
    else if( fn.FileExists() && !fn.IsFileWritable() )
    {
        msg.Printf( _( "You do not have write permissions to save file \"%s\"." ),
                    GetChars( fn.GetFullPath() ) );
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
            GetChars( aFileName.GetFullName() )
        );

    int response = wxMessageBox( msg, Pgm().App().GetAppName(), wxYES_NO | wxICON_QUESTION, this );

    // Make a backup of the current file, delete the file, and rename the auto save file to
    // the file name.
    if( response == wxYES )
    {
        if( !wxRenameFile( autoSaveFileName.GetFullPath(), aFileName.GetFullPath() ) )
        {
            wxMessageBox( _( "The auto save file could not be renamed to the board file name." ),
                          Pgm().App().GetAppName(), wxOK | wxICON_EXCLAMATION, this );
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


void EDA_BASE_FRAME::ClearUndoRedoList()
{
    ClearUndoORRedoList( UNDO_LIST );
    ClearUndoORRedoList( REDO_LIST );
}


void EDA_BASE_FRAME::PushCommandToUndoList( PICKED_ITEMS_LIST* aNewitem )
{
    m_undoList.PushCommand( aNewitem );

    // Delete the extra items, if count max reached
    if( m_UndoRedoCountMax > 0 )
    {
        int extraitems = GetUndoCommandCount() - m_UndoRedoCountMax;

        if( extraitems > 0 )
            ClearUndoORRedoList( UNDO_LIST, extraitems );
    }
}


void EDA_BASE_FRAME::PushCommandToRedoList( PICKED_ITEMS_LIST* aNewitem )
{
    m_redoList.PushCommand( aNewitem );

    // Delete the extra items, if count max reached
    if( m_UndoRedoCountMax > 0 )
    {
        int extraitems = GetRedoCommandCount() - m_UndoRedoCountMax;

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
        m_NormalFrameSize = GetWindowSize();
        m_NormalFramePos  = GetPosition();
        wxLogTrace( traceDisplayLocation, "Maximizing window - Saving position (%d, %d) with size (%d, %d)",
                m_NormalFramePos.x, m_NormalFramePos.y, m_NormalFrameSize.x, m_NormalFrameSize.y );
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
