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

#include <wx/stdpaths.h>
#include <wx/string.h>
#include <wx/display.h>
#include <dialog_shim.h>
#include <eda_doc.h>
#include <id.h>
#include <kiface_i.h>
#include <pgm_base.h>
#include <trace_helpers.h>
#include <panel_hotkeys_editor.h>
#include <dialogs/panel_common_settings.h>
#include <widgets/paged_dialog.h>
#include <bitmaps.h>
#include <tool/action_menu.h>
#include <tool/common_control.h>
#include <tool/tool_manager.h>
#include <tool/action_manager.h>
#include <menus_helpers.h>
#include <tool/actions.h>


/// The default auto save interval is 10 minutes.
#define DEFAULT_AUTO_SAVE_INTERVAL 600

///@{
/// \ingroup config

/// Configuration file entry name for auto save interval.
static const wxString entryAutoSaveInterval = "AutoSaveInterval";

/// Configuration file entry for wxAuiManger perspective.
static const wxString entryPerspective = "Perspective";

/// Configuration file entry for most recently used path.
static const wxString entryMruPath = "MostRecentlyUsedPath";

static const wxString entryPosY = "Pos_y";   ///< Y position of frame, in pixels (suffix)
static const wxString entryPosX = "Pos_x";   ///< X position of frame, in pixels (suffix)
static const wxString entrySizeY = "Size_y"; ///< Height of frame, in pixels (suffix)
static const wxString entrySizeX = "Size_x"; ///< Width of frame, in pixels (suffix)
static const wxString entryMaximized = "Maximized";  ///< Nonzero iff frame is maximized (suffix)
///@}


BEGIN_EVENT_TABLE( EDA_BASE_FRAME, wxFrame )
    EVT_MENU( wxID_ABOUT, EDA_BASE_FRAME::OnKicadAbout )
    EVT_MENU( wxID_PREFERENCES, EDA_BASE_FRAME::OnPreferences )

    EVT_CHAR_HOOK( EDA_BASE_FRAME::OnCharHook )
    EVT_MENU_OPEN( EDA_BASE_FRAME::OnMenuOpen )
    EVT_MENU_CLOSE( EDA_BASE_FRAME::OnMenuOpen )
    EVT_MENU_HIGHLIGHT_ALL( EDA_BASE_FRAME::OnMenuOpen )
END_EVENT_TABLE()

EDA_BASE_FRAME::EDA_BASE_FRAME( wxWindow* aParent, FRAME_T aFrameType,
                                const wxString& aTitle, const wxPoint& aPos, const wxSize& aSize,
                                long aStyle, const wxString& aFrameName, KIWAY* aKiway ) :
        wxFrame( aParent, wxID_ANY, aTitle, aPos, aSize, aStyle, aFrameName ),
        KIWAY_HOLDER( aKiway, KIWAY_HOLDER::FRAME )
{
    m_Ident = aFrameType;
    m_hasAutoSave = false;
    m_autoSaveState = false;
    m_autoSaveInterval = -1;
    m_autoSaveTimer = new wxTimer( this, ID_AUTO_SAVE_TIMER );
    m_mruPath = wxStandardPaths::Get().GetDocumentsDir();
    m_toolManager = nullptr;

    // Gives a reasonable minimal size to the frame:
    const int minsize_x = 500;
    const int  minsize_y = 400;
    SetSizeHints( minsize_x, minsize_y, -1, -1, -1, -1 );

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

    wxConfigBase* cfg = config();

    if( cfg )
        SaveSettings( cfg );       // virtual, wxFrame specific

    event.Skip();       // we did not "handle" the event, only eavesdropped on it.
}


EDA_BASE_FRAME::~EDA_BASE_FRAME()
{
    delete m_autoSaveTimer;
}


void EDA_BASE_FRAME::PushTool( const std::string& actionName )
{
    m_toolStack.push_back( actionName );

    // Human cognitive stacking is very shallow; deeper tool stacks just get annoying
    if( m_toolStack.size() > 3 )
        m_toolStack.erase( m_toolStack.begin() );

    TOOL_ACTION* action = m_toolManager->GetActionManager()->FindAction( actionName );

    if( action )
        DisplayToolMsg( action->GetLabel() );
    else
        DisplayToolMsg( actionName );
}


void EDA_BASE_FRAME::PopTool( const std::string& actionName )
{
    // Push/pop events can get out of order (such as when they're generated by the Simulator
    // frame but not processed until the mouse is back in the Schematic frame), so make sure
    // we're popping the right stack frame.

    for( int i = m_toolStack.size() - 1; i >= 0; --i )
    {
        if( m_toolStack[ i ] == actionName )
        {
            m_toolStack.erase( m_toolStack.begin() + i );

            // If there's something underneath us, and it's now the top of the stack, then
            // re-activate it
            if( ( --i ) >= 0 && i == (int)m_toolStack.size() - 1 )
            {
                std::string  back = m_toolStack[ i ];
                TOOL_ACTION* action = m_toolManager->GetActionManager()->FindAction( back );

                if( action )
                {
                    // Pop the action as running it will push it back onto the stack
                    m_toolStack.pop_back();

                    TOOL_EVENT evt = action->MakeEvent();
                    evt.SetHasPosition( false );
                    GetToolManager()->PostEvent( evt );
                }
            }
            else
                DisplayToolMsg( ACTIONS::selectionTool.GetLabel() );

            return;
        }
    }
}


std::string EDA_BASE_FRAME::CurrentToolName() const
{
    if( m_toolStack.empty() )
        return ACTIONS::selectionTool.GetName();
    else
        return m_toolStack.back();
}


bool EDA_BASE_FRAME::IsCurrentTool( const TOOL_ACTION& aAction ) const
{
    if( m_toolStack.empty() )
        return &aAction == &ACTIONS::selectionTool;
    else
        return m_toolStack.back() == aAction.GetName();
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


void EDA_BASE_FRAME::OnMenuOpen( wxMenuEvent& event )
{
    //
    // wxWidgets has several issues that we have to work around:
    //
    // 1) wxWidgets 3.0.x Windows has a bug where wxEVT_MENU_OPEN and wxEVT_MENU_HIGHLIGHT
    //    events are not captured by the ACTON_MENU menus.  So we forward them here.
    //    (FWIW, this one is fixed in wxWidgets 3.1.x.)
    //
    // 2) wxWidgets doesn't pass the menu pointer for wxEVT_MENU_HIGHLIGHT events.  So we
    //    store the menu pointer from the wxEVT_MENU_OPEN call.
    //
    // 3) wxWidgets has no way to tell whether a command is from a menu selection or a
    //    hotkey.  So we keep track of menu highlighting so we can differentiate.
    //

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
    else if( event.GetEventType() == wxEVT_MENU_CLOSE )
    {
        if( currentMenu )
            currentMenu->OnMenuEvent( event );

        currentMenu = nullptr;
    }

    event.Skip();
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
    GetToolManager()->GetActionManager()->UpdateHotKeys( false );

    if( GetMenuBar() )
    {
        // For icons in menus, icon scaling & hotkeys
        ReCreateMenuBar();
        GetMenuBar()->Refresh();
    }

    wxConfigBase* settings = Pgm().CommonSettings();

    settings->Read( WARP_MOUSE_ON_MOVE_KEY, &m_moveWarpsCursor );
    settings->Read( PREFER_SELECT_TO_DRAG_KEY, &m_dragSelects );
    settings->Read( IMMEDIATE_ACTIONS_KEY, &m_immediateActions );
}


void EDA_BASE_FRAME::LoadSettings( wxConfigBase* aCfg )
{
    int maximized = 0;

    wxString baseCfgName = ConfigBaseName();

    wxString text = baseCfgName + entryPosX;
    aCfg->Read( text, &m_FramePos.x, m_FramePos.x );

    text = baseCfgName + entryPosY;
    aCfg->Read( text, &m_FramePos.y, m_FramePos.y );

    text = baseCfgName + entrySizeX;
    aCfg->Read( text, &m_FrameSize.x, m_FrameSize.x );

    text = baseCfgName + entrySizeY;
    aCfg->Read( text, &m_FrameSize.y, m_FrameSize.y );

    text = baseCfgName + entryMaximized;
    aCfg->Read( text, &maximized, 0 );

    if( m_hasAutoSave )
    {
        text = baseCfgName + entryAutoSaveInterval;
        aCfg->Read( text, &m_autoSaveInterval, DEFAULT_AUTO_SAVE_INTERVAL );
    }

    // Ensure the window is on a connected display, and is visible.
    // (at least a corner of the frame must be visible on screen)
    // Sometimes, if a window was moved on an auxiliary display, and when this
    // display is no more available, it is not the case.
    wxRect rect( m_FramePos, m_FrameSize );

    if( wxDisplay::GetFromPoint( rect.GetTopLeft() ) == wxNOT_FOUND &&
        wxDisplay::GetFromPoint( rect.GetTopRight() ) == wxNOT_FOUND &&
        wxDisplay::GetFromPoint( rect.GetBottomLeft() ) == wxNOT_FOUND &&
        wxDisplay::GetFromPoint( rect.GetBottomRight() ) == wxNOT_FOUND )
    {
        m_FramePos = wxDefaultPosition;
    }

    // Ensure Window title bar is visible
#if defined( __WXMAC__ )
    // for macOSX, the window must be below system (macOSX) toolbar
    // Ypos_min = GetMBarHeight(); seems no more exist in new API (subject to change)
    int Ypos_min = 20;
#else
    int Ypos_min = 0;
#endif
    if( m_FramePos.y < Ypos_min )
        m_FramePos.y = Ypos_min;

    if( maximized )
        Maximize();

    aCfg->Read( baseCfgName + entryPerspective, &m_perspective );
    aCfg->Read( baseCfgName + entryMruPath, &m_mruPath );

    wxConfigBase* settings = Pgm().CommonSettings();

    if( !settings->Read( WARP_MOUSE_ON_MOVE_KEY, &m_moveWarpsCursor ) )
    {
        // Legacy versions stored the property only for Eeschema, so see if we have it there
        std::unique_ptr<wxConfigBase> pcbSettings = GetNewConfig( wxT( "eeschema" ) );
        pcbSettings->Read( "MoveWarpsCursor", &m_moveWarpsCursor, true );
    }

    if( !settings->Read( PREFER_SELECT_TO_DRAG_KEY, &m_dragSelects ) )
    {
        // Legacy versions stored the property only for PCBNew, so see if we have it there
        std::unique_ptr<wxConfigBase> pcbSettings = GetNewConfig( wxT( "pcbnew" ) );
        pcbSettings->Read( "DragSelects", &m_dragSelects, true );
    }

    settings->Read( IMMEDIATE_ACTIONS_KEY, &m_immediateActions, false );
}


void EDA_BASE_FRAME::SaveSettings( wxConfigBase* aCfg )
{
    wxString        text;

    if( IsIconized() )
        return;

    wxString baseCfgName = ConfigBaseName();

    m_FrameSize = GetSize();
    m_FramePos  = GetPosition();

    text = baseCfgName + wxT( "Pos_x" );
    aCfg->Write( text, (long) m_FramePos.x );

    text = baseCfgName + wxT( "Pos_y" );
    aCfg->Write( text, (long) m_FramePos.y );

    text = baseCfgName + wxT( "Size_x" );
    aCfg->Write( text, (long) m_FrameSize.x );

    text = baseCfgName + wxT( "Size_y" );
    aCfg->Write( text, (long) m_FrameSize.y );

    text = baseCfgName + wxT( "Maximized" );
    aCfg->Write( text, IsMaximized() );

    if( m_hasAutoSave )
    {
        text = baseCfgName + entryAutoSaveInterval;
        aCfg->Write( text, m_autoSaveInterval );
    }

    // Once this is fully implemented, wxAuiManager will be used to maintain
    // the persistance of the main frame and all it's managed windows and
    // all of the legacy frame persistence position code can be removed.
    wxString perspective = m_auimgr.SavePerspective();

    // printf( "perspective(%s): %s\n",
    //    TO_UTF8( m_FrameName + entryPerspective ), TO_UTF8( perspective ) );
    aCfg->Write( baseCfgName + entryPerspective, perspective );
    aCfg->Write( baseCfgName + entryMruPath, m_mruPath );
}


wxConfigBase* EDA_BASE_FRAME::config()
{
    // KICAD_MANAGER_FRAME overrides this
    wxConfigBase* ret = Kiface().KifaceSettings();
    //wxASSERT( ret );
    return ret;
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


void EDA_BASE_FRAME::UpdateFileHistory( const wxString& FullFileName,
                                        wxFileHistory* aFileHistory )
{
    wxFileHistory* fileHistory = aFileHistory;

    if( !fileHistory )
        fileHistory = &Kiface().GetFileHistory();

    fileHistory->AddFileToHistory( FullFileName );
}


wxString EDA_BASE_FRAME::GetFileFromHistory( int cmdId, const wxString& type,
                                             wxFileHistory* aFileHistory )
{
    wxFileHistory* fileHistory = aFileHistory;

    if( !fileHistory )
        fileHistory = &Kiface().GetFileHistory();

    int baseId = fileHistory->GetBaseId();

    wxASSERT( cmdId >= baseId && cmdId < baseId + (int) fileHistory->GetCount() );

    unsigned i = cmdId - baseId;

    if( i < fileHistory->GetCount() )
    {
        wxString fn = fileHistory->GetHistoryFile( i );

        if( wxFileName::FileExists( fn ) )
            return fn;
        else
        {
            wxString msg = wxString::Format( _( "File \"%s\" was not found." ), fn );
            wxMessageBox( msg );

            fileHistory->RemoveFileFromHistory( i );
        }
    }

    return wxEmptyString;
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
        // Get the backup file name.
        wxFileName backupFileName = aFileName;
        backupFileName.SetExt( aFileName.GetExt() + GetBackupSuffix() );

        // If an old backup file exists, delete it.  If an old copy of the file exists, rename
        // it to the backup file name
        if( aFileName.FileExists() )
        {
            // Rename the old file to the backup file name.
            if( !wxRenameFile( aFileName.GetFullPath(), backupFileName.GetFullPath(), true ) )
            {
                msg.Printf( _( "Could not create backup file \"%s\"" ),
                            GetChars( backupFileName.GetFullPath() ) );
                wxMessageBox( msg );
            }
        }

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


