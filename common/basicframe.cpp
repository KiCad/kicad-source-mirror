/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2013-2015 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2015 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file basicframe.cpp
 * @brief EDA_BASE_FRAME class implementation.
 */

#include <wx/aboutdlg.h>
#include <wx/fontdlg.h>
#include <wx/clipbrd.h>
#include <wx/statline.h>
#include <wx/platinfo.h>

#include <build_version.h>
#include <fctsys.h>
#include <pgm_base.h>
#include <kiface_i.h>
#include <online_help.h>
#include <id.h>
#include <eda_doc.h>
#include <wxstruct.h>
#include <macros.h>
#include <menus_helpers.h>
#include <dialog_shim.h>

#include <boost/version.hpp>
#include <typeinfo>

/// The default auto save interval is 10 minutes.
#define DEFAULT_AUTO_SAVE_INTERVAL 600


const wxChar traceAutoSave[] = wxT( "KicadAutoSave" );

/// Configuration file entry name for auto save interval.
static const wxChar entryAutoSaveInterval[] = wxT( "AutoSaveInterval" );

/// Configuration file entry for wxAuiManger perspective.
static const wxChar entryPerspective[] = wxT( "Perspective" );



EDA_BASE_FRAME::EDA_BASE_FRAME( wxWindow* aParent, FRAME_T aFrameType,
        const wxString& aTitle, const wxPoint& aPos, const wxSize& aSize,
        long aStyle, const wxString& aFrameName ) :
    wxFrame( aParent, wxID_ANY, aTitle, aPos, aSize, aStyle, aFrameName )
{
    wxSize minsize;

    m_Ident = aFrameType;
    m_mainToolBar = NULL;
    m_hasAutoSave = false;
    m_autoSaveState = false;
    m_autoSaveInterval = -1;
    m_autoSaveTimer = new wxTimer( this, ID_AUTO_SAVE_TIMER );

    minsize.x = 470;
    minsize.y = 350;

    SetSizeHints( minsize.x, minsize.y, -1, -1, -1, -1 );

    if( ( aSize.x < minsize.x ) || ( aSize.y < minsize.y ) )
        SetSize( 0, 0, minsize.x, minsize.y );

    // Create child subwindows.

    // Dimensions of the user area of the main window.
    GetClientSize( &m_FrameSize.x, &m_FrameSize.y );

    m_FramePos.x = m_FramePos.y = 0;

    Connect( ID_HELP_COPY_VERSION_STRING,
             wxEVT_COMMAND_MENU_SELECTED,
             wxCommandEventHandler( EDA_BASE_FRAME::CopyVersionInfoToClipboard ) );

    Connect( ID_AUTO_SAVE_TIMER, wxEVT_TIMER,
             wxTimerEventHandler( EDA_BASE_FRAME::onAutoSaveTimer ) );

    // hook wxEVT_CLOSE_WINDOW so we can call SaveSettings().  This function seems
    // to be called before any other hook for wxCloseEvent, which is necessary.
    Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( EDA_BASE_FRAME::windowClosing ) );
}


void EDA_BASE_FRAME::windowClosing( wxCloseEvent& event )
{
    DIALOG_SHIM* dlg  = NULL;
    wxWindowList list = GetChildren();

    // Quasi modal dialogs create issues (crashes) when closing Kicad.
    // I am guessing they are delete too late, when deleting main frames.
    // AFAIK, only these DIALOG_SHIM dialogs create such issues.
    // The policy is do not allow closing Kicad if a Quasi modal dialog is open.
    // (Anyway, closing without prompting the user is certainly bad,
    // because an edit is in preogress)
    // Therefore, iterate through the child list to find at least
    // a DIALOG_SHIM opened in quasi modal mode
    for( wxWindowList::iterator iter = list.begin(); iter != list.end(); ++iter )
    {
        if( (dlg = dynamic_cast<DIALOG_SHIM*> (*iter) ) != NULL )
        {
            if( dlg->IsQuasiModal() )
                break;
            else
                dlg = NULL;
        }
    }

    if( dlg )
    {
        // Happens when a quasi modal dialog is currently open.
        // For example: if the Kicad manager try to close Kicad.
        wxMessageBox( _(
                "The program cannot be closed\n"
                "A quasi-modal dialog window is currently open, please close it first." ) );
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
    if( !wxFrame::ProcessEvent( aEvent ) )
        return false;

    if( m_hasAutoSave && (m_autoSaveState != isAutoSaveRequired()) && (m_autoSaveInterval > 0) )
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


bool EDA_BASE_FRAME::Enable( bool enable )
{
    // so we can do logging of this state change:

#if defined(DEBUG)
    const char* type_id = typeid( *this ).name();
    printf( "wxFrame %-28s: %s\n", type_id, enable ? "enabled" : "disabled" );
#endif

    return wxFrame::Enable( enable );
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
    ReCreateMenuBar();
    GetMenuBar()->Refresh();
}


void EDA_BASE_FRAME::LoadSettings( wxConfigBase* aCfg )
{
    int maximized = 0;

    wxString baseCfgName = GetName();

    wxString text = baseCfgName + wxT( "Pos_x" );
    aCfg->Read( text, &m_FramePos.x );

    text = baseCfgName + wxT( "Pos_y" );
    aCfg->Read( text, &m_FramePos.y );

    text = baseCfgName + wxT( "Size_x" );
    aCfg->Read( text, &m_FrameSize.x, 600 );

    text = baseCfgName + wxT( "Size_y" );
    aCfg->Read( text, &m_FrameSize.y, 400 );

    text = baseCfgName + wxT( "Maximized" );
    aCfg->Read( text, &maximized, 0 );

    if( m_hasAutoSave )
    {
        text = baseCfgName + entryAutoSaveInterval;
        aCfg->Read( text, &m_autoSaveInterval, DEFAULT_AUTO_SAVE_INTERVAL );
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
}


void EDA_BASE_FRAME::SaveSettings( wxConfigBase* aCfg )
{
    wxString        text;

    if( IsIconized() )
        return;

    wxString baseCfgName = GetName();

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
            wxString msg = wxString::Format(
                        wxT( "file '%s' was not found." ),
                        GetChars( fn ) );

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
        for( unsigned ii = 0; ii < DIM( names ); ii++ )
        {
            helpFile = SearchHelpFileFullPath( search, names[ii] );

            if( !helpFile.IsEmpty() )
               break;
        }

        if( !helpFile )
        {
            wxString msg = wxString::Format( _(
                "Html or pdf help file \n'%s'\n or\n'%s' could not be found." ), names[0], names[1] );
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
            "Help file '%s' could not be found." ),
            GetChars( base_name )
            );
        wxMessageBox( msg );
    }
    else
    {
        GetAssociatedDocument( this, helpFile );
    }
}


void EDA_BASE_FRAME::OnSelectPreferredEditor( wxCommandEvent& event )
{
    wxFileName  fn = Pgm().GetEditorName();
    wxString    wildcard( wxT( "*" ) );

#ifdef __WINDOWS__
    wildcard += wxT( ".exe" );
#endif

    wildcard.Printf( _( "Executable file (%s)|%s" ),
                     GetChars( wildcard ), GetChars( wildcard ) );

    wxFileDialog dlg( this, _( "Select Preferred Editor" ), fn.GetPath(),
                      fn.GetFullName(), wildcard,
                      wxFD_OPEN | wxFD_FILE_MUST_EXIST );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    wxString editor = dlg.GetPath();

    Pgm().SetEditorName( editor );
}


void EDA_BASE_FRAME::GetKicadAbout( wxCommandEvent& event )
{
    bool ShowAboutDialog(wxWindow * parent);
    ShowAboutDialog( this );
}


void EDA_BASE_FRAME::AddHelpVersionInfoMenuEntry( wxMenu* aMenu )
{
    wxASSERT( aMenu != NULL );

    // Copy version string to clipboard for bug report purposes.
    AddMenuItem( aMenu, ID_HELP_COPY_VERSION_STRING,
                 _( "Copy &Version Information" ),
                 _( "Copy the version string to clipboard to send with bug reports" ),
                 KiBitmap( copy_button_xpm ) );
}


// This is an enhanced version of the compiler build macro provided by wxWidgets
// in <wx/build.h>. Please do not make any of these strings translatable.  They
// are used for conveying troubleshooting information to developers.

#if defined(__GXX_ABI_VERSION)
    #define __ABI_VERSION  ",compiler with C++ ABI " __WX_BO_STRINGIZE(__GXX_ABI_VERSION)
#else
    #define __ABI_VERSION  ",compiler without C++ ABI "
#endif

#if defined(__INTEL_COMPILER)
    #define __BO_COMPILER ",Intel C++"
#elif defined(__GNUG__)
    #define __BO_COMPILER ",GCC " \
            __WX_BO_STRINGIZE(__GNUC__) "." \
            __WX_BO_STRINGIZE(__GNUC_MINOR__) "." \
            __WX_BO_STRINGIZE(__GNUC_PATCHLEVEL__)
#elif defined(__VISUALC__)
    #define __BO_COMPILER ",Visual C++"
#elif defined(__BORLANDC__)
    #define __BO_COMPILER ",Borland C++"
#elif defined(__DIGITALMARS__)
    #define __BO_COMPILER ",DigitalMars"
#elif defined(__WATCOMC__)
    #define __BO_COMPILER ",Watcom C++"
#else
    #define __BO_COMPILER ",unknown"
#endif

#if wxCHECK_VERSION( 2, 9, 0 )

static inline const char* KICAD_BUILD_OPTIONS_SIGNATURE()
{
    return
#ifdef __WXDEBUG__
    " (debug,"
#else
    " (release,"
#endif
    __WX_BO_UNICODE __ABI_VERSION __BO_COMPILER __WX_BO_STL

#if !wxCHECK_VERSION( 3, 0, 0 )
    __WX_BO_WXWIN_COMPAT_2_6
#endif

    __WX_BO_WXWIN_COMPAT_2_8 ")"
    ;
}

#else

static inline const char* KICAD_BUILD_OPTIONS_SIGNATURE()
{
    return
#ifdef __WXDEBUG__
    " (debug,"
#else
    " (release,"
#endif
    __WX_BO_UNICODE __ABI_VERSION __BO_COMPILER __WX_BO_STL
    __WX_BO_WXWIN_COMPAT_2_4 __WX_BO_WXWIN_COMPAT_2_6 ")"
    ;
}

#endif

void EDA_BASE_FRAME::CopyVersionInfoToClipboard( wxCommandEvent&  event )
{
    if( !wxTheClipboard->Open() )
    {
        wxMessageBox( _( "Could not open clipboard to write version information." ),
                      _( "Clipboard Error" ), wxOK | wxICON_EXCLAMATION, this );
        return;
    }

    wxString tmp;
    wxPlatformInfo info;

    tmp = wxT( "Application: " ) + Pgm().App().GetAppName() + wxT( "\n" );
    tmp << wxT( "Version: " ) << GetBuildVersion()
#ifdef DEBUG
        << wxT( " Debug" )
#else
        << wxT( " Release" )
#endif
        << wxT( " build\n" );
    tmp << wxT( "wxWidgets: Version " ) << FROM_UTF8( wxVERSION_NUM_DOT_STRING )
        << FROM_UTF8( KICAD_BUILD_OPTIONS_SIGNATURE() ) << wxT( "\n" )
        << wxT( "Platform: " ) << wxGetOsDescription() << wxT( ", " )
        << info.GetArchName() << wxT( ", " ) << info.GetEndiannessName() << wxT( ", " )
        << info.GetPortIdName() << wxT( "\n" );

    // Just in case someone builds KiCad with the platform native of Boost instead of
    // the version included with the KiCad source.
    tmp << wxT( "Boost version: " ) << ( BOOST_VERSION / 100000 ) << wxT( "." )
        << ( BOOST_VERSION / 100 % 1000 ) << wxT( "." ) << ( BOOST_VERSION % 100 ) << wxT( "\n" );

    tmp << wxT( "         USE_WX_GRAPHICS_CONTEXT=" );
#ifdef USE_WX_GRAPHICS_CONTEXT
    tmp << wxT( "ON\n" );
#else
    tmp << wxT( "OFF\n" );
#endif

    tmp << wxT( "         USE_WX_OVERLAY=" );
#ifdef USE_WX_OVERLAY
    tmp << wxT( "ON\n" );
#else
    tmp << wxT( "OFF\n" );
#endif

    tmp << wxT( "         KICAD_SCRIPTING=" );
#ifdef KICAD_SCRIPTING
    tmp << wxT( "ON\n" );
#else
    tmp << wxT( "OFF\n" );
#endif

    tmp << wxT( "         KICAD_SCRIPTING_MODULES=" );
#ifdef KICAD_SCRIPTING_MODULES
    tmp << wxT( "ON\n" );
#else
    tmp << wxT( "OFF\n" );
#endif

    tmp << wxT( "         KICAD_SCRIPTING_WXPYTHON=" );
#ifdef KICAD_SCRIPTING_WXPYTHON
    tmp << wxT( "ON\n" );
#else
    tmp << wxT( "OFF\n" );
#endif

    tmp << wxT( "         USE_FP_LIB_TABLE=HARD_CODED_ON\n" );

    tmp << wxT( "         BUILD_GITHUB_PLUGIN=" );
#ifdef BUILD_GITHUB_PLUGIN
    tmp << wxT( "ON\n" );
#else
    tmp << wxT( "OFF\n" );
#endif

    wxMessageBox( tmp, _("Version Information (copied to the clipboard)") );

    wxTheClipboard->SetData( new wxTextDataObject( tmp ) );
    wxTheClipboard->Close();
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
        msg.Printf( _( "You do not have write permissions to folder <%s>." ),
                    GetChars( fn.GetPath() ) );
    }
    else if( !fn.FileExists() && !fn.IsDirWritable() )
    {
        msg.Printf( _( "You do not have write permissions to save file <%s> to folder <%s>." ),
                    GetChars( fn.GetFullName() ), GetChars( fn.GetPath() ) );
    }
    else if( fn.FileExists() && !fn.IsFileWritable() )
    {
        msg.Printf( _( "You do not have write permissions to save file <%s>." ),
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
            "'%s'\n"
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
                msg.Printf( _( "Could not create backup file <%s>" ),
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

