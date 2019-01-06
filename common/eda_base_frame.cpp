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

/**
 * @file eda_base_frame.cpp
 * @brief EDA_BASE_FRAME class implementation.
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


/// The default auto save interval is 10 minutes.
#define DEFAULT_AUTO_SAVE_INTERVAL 600

#define URL_GET_INVOLVED "http://kicad-pcb.org/contribute/"

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

EDA_BASE_FRAME::EDA_BASE_FRAME( wxWindow* aParent, FRAME_T aFrameType,
        const wxString& aTitle, const wxPoint& aPos, const wxSize& aSize,
        long aStyle, const wxString& aFrameName ) :
    wxFrame( aParent, wxID_ANY, aTitle, aPos, aSize, aStyle, aFrameName )
{
    m_Ident = aFrameType;
    m_mainToolBar = NULL;
    m_hasAutoSave = false;
    m_autoSaveState = false;
    m_autoSaveInterval = -1;
    m_autoSaveTimer = new wxTimer( this, ID_AUTO_SAVE_TIMER );
    m_mruPath = wxStandardPaths::Get().GetDocumentsDir();

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

    // This is needed for OSX: avoids further OnDraw processing after this
    // destructor and before the native window is destroyed
    this->Freeze();
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


void EDA_BASE_FRAME::ReCreateMenuBar()
{
}


void EDA_BASE_FRAME::ShowChangedLanguage()
{
    if( GetMenuBar() )
    {
        ReCreateMenuBar();
        GetMenuBar()->Refresh();
    }
}


void EDA_BASE_FRAME::CommonSettingsChanged()
{
    if( GetMenuBar() )
    {
        // For icons in menus, icon scaling & hotkeys
        ReCreateMenuBar();
        GetMenuBar()->Refresh();
    }
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


void EDA_BASE_FRAME::GetKicadHelp( wxCommandEvent& event )
{
    const SEARCH_STACK& search = sys_search();

    /* We have to get document for beginners,
     * or the full specific doc
     * if event id is wxID_INDEX, we want the document for beginners.
     * else the specific doc file (its name is in Kiface().GetHelpFileName())
     * The document for beginners is the same for all KiCad utilities
     */
    if( event.GetId() == wxID_INDEX )
    {
        // List of possible names for Getting Started in KiCad
        const wxChar* names[2] = {
            wxT( "getting_started_in_kicad" ),
            wxT( "Getting_Started_in_KiCad" )
            };

        wxString helpFile;
        // Search for "getting_started_in_kicad.html" or "getting_started_in_kicad.pdf"
        // or "Getting_Started_in_KiCad.html" or "Getting_Started_in_KiCad.pdf"
        for( unsigned ii = 0; ii < arrayDim( names ); ii++ )
        {
            helpFile = SearchHelpFileFullPath( search, names[ii] );

            if( !helpFile.IsEmpty() )
               break;
        }

        if( !helpFile )
        {
            wxString msg = wxString::Format( _(
                "Html or pdf help file \n\"%s\"\n or\n\"%s\" could not be found." ), names[0], names[1] );
            wxMessageBox( msg );
        }
        else
        {
            GetAssociatedDocument( this, helpFile );
        }

        return;
    }

    wxString base_name = help_name();
    wxString helpFile = SearchHelpFileFullPath( search, base_name );

    if( !helpFile )
    {
        wxString msg = wxString::Format( _(
            "Help file \"%s\" could not be found." ),
            GetChars( base_name )
            );
        wxMessageBox( msg );
    }
    else
    {
        GetAssociatedDocument( this, helpFile );
    }
}


void EDA_BASE_FRAME::GetKicadContribute( wxCommandEvent& event )
{
    if( !wxLaunchDefaultBrowser( URL_GET_INVOLVED ) )
    {
        wxString msg;
        msg.Printf( _( "Could not launch the default browser.\n"
                       "For information on how to help the KiCad project, visit %s" ),
                    URL_GET_INVOLVED );
        wxMessageBox( msg, _( "Get involved with KiCad" ), wxOK, this );
    }
}


void EDA_BASE_FRAME::GetKicadAbout( wxCommandEvent& event )
{
    void ShowAboutDialog(EDA_BASE_FRAME * aParent); // See AboutDialog_main.cpp
    ShowAboutDialog( this );
}


bool EDA_BASE_FRAME::ShowPreferences( EDA_HOTKEY_CONFIG* aHotkeys, EDA_HOTKEY_CONFIG* aShowHotkeys,
                                      const wxString& aHotkeysNickname )
{
    PAGED_DIALOG dlg( this, _( "Preferences" ) );
    wxTreebook* book = dlg.GetTreebook();

    book->AddPage( new PANEL_COMMON_SETTINGS( &dlg, book ), _( "Common" ) );
    book->AddPage( new PANEL_HOTKEYS_EDITOR( this, book, false,
        aHotkeys, aShowHotkeys, aHotkeysNickname ), _( "Hotkeys" ) );

    for( unsigned i = 0; i < KIWAY_PLAYER_COUNT;  ++i )
    {
        KIWAY_PLAYER* frame = dlg.Kiway().Player( (FRAME_T) i, false );

        if( frame )
            frame->InstallPreferences( &dlg );
    }

    if( dlg.ShowModal() == wxID_OK )
    {
        dlg.Kiway().CommonSettingsChanged();
        return true;
    }

    return false;
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


void EDA_BASE_FRAME::CheckForAutoSaveFile( const wxFileName& aFileName,
                                           const wxString&   aBackupFileExtension )
{
    wxCHECK_RET( aFileName.IsOk(), wxT( "Invalid file name!" ) );
    wxCHECK_RET( !aBackupFileExtension.IsEmpty(), wxT( "Invalid backup file extension!" ) );

    wxFileName autoSaveFileName = aFileName;

    // Check for auto save file.
    autoSaveFileName.SetName( AUTOSAVE_PREFIX_FILENAME + aFileName.GetName() );

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
        backupFileName.SetExt( aBackupFileExtension );

        // If an old backup file exists, delete it.  If an old copy of the file exists, rename
        // it to the backup file name
        if( aFileName.FileExists() )
        {
            // Remove the old file backup file.
            if( backupFileName.FileExists() )
                wxRemoveFile( backupFileName.GetFullPath() );

            // Rename the old file to the backup file name.
            if( !wxRenameFile( aFileName.GetFullPath(), backupFileName.GetFullPath() ) )
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


bool EDA_BASE_FRAME::PostCommandMenuEvent( int evt_type )
{
    if( evt_type != 0 )
    {
        wxCommandEvent evt( wxEVT_COMMAND_MENU_SELECTED );
        evt.SetEventObject( this );
        evt.SetId( evt_type );
        wxPostEvent( this, evt );
        return true;
    }

    return false;
}

