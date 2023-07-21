/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 1992-2021 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file pgm_base.cpp
 *
 * @brief For the main application: init functions, and language selection
 *        (locale handling)
 */

#include <wx/html/htmlwin.h>
#include <wx/fs_zip.h>
#include <wx/dir.h>
#include <wx/filename.h>
#include <wx/propgrid/propgrid.h>
#include <wx/snglinst.h>
#include <wx/stdpaths.h>
#include <wx/sysopt.h>
#include <wx/filedlg.h>
#include <wx/ffile.h>
#include <wx/tooltip.h>

#include <advanced_config.h>
#include <bitmaps.h>
#include <cli/cli_names.h> // Needed for the pre wx 3.2 cli workaround
#include <common.h>
#include <config_params.h>
#include <confirm.h>
#include <core/arraydim.h>
#include <eda_base_frame.h>
#include <eda_draw_frame.h>
#include <gestfich.h>
#include <id.h>
#include <kicad_curl/kicad_curl.h>
#include <kiplatform/policy.h>
#include <macros.h>
#include <menus_helpers.h>
#include <paths.h>
#include <pgm_base.h>
#include <policy_keys.h>
#include <python_scripting.h>
#include <settings/common_settings.h>
#include <settings/settings_manager.h>
#include <systemdirsappend.h>
#include <thread_pool.h>
#include <trace_helpers.h>

#include <wx/splash.h>

#ifdef KICAD_USE_SENTRY
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <sentry.h>
#include <build_version.h>
#endif

/**
 * Current list of languages supported by KiCad.
 *
 * @note Because this list is not created on the fly, wxTranslation
 * must be called when a language name must be displayed after translation.
 * Do not change this behavior, because m_Lang_Label is also used as key in config
 * N.B. Languages that are commented out have some translation existing but are
 * not sufficiently translated to list as valid translations in KiCad for release
 */
#undef _
#define _(s) s
LANGUAGE_DESCR LanguagesList[] =
{
    { wxLANGUAGE_DEFAULT,    ID_LANGUAGE_DEFAULT,    _( "Default" ),    false },
//    { wxLANGUAGE_INDONESIAN, ID_LANGUAGE_INDONESIAN, wxT( "Bahasa Indonesia" ), true },
    { wxLANGUAGE_CZECH,      ID_LANGUAGE_CZECH,      wxT( "Čeština" ),  true },
    { wxLANGUAGE_DANISH,     ID_LANGUAGE_DANISH,     wxT( "Dansk" ),    true },
    { wxLANGUAGE_GERMAN,     ID_LANGUAGE_GERMAN,     wxT( "Deutsch" ),  true },
    { wxLANGUAGE_GREEK,      ID_LANGUAGE_GREEK,      wxT( "Ελληνικά" ), true },
    { wxLANGUAGE_ENGLISH,    ID_LANGUAGE_ENGLISH,    wxT( "English" ),  true },
    { wxLANGUAGE_SPANISH,    ID_LANGUAGE_SPANISH,    wxT( "Español" ),  true },
    { wxLANGUAGE_SPANISH_MEXICAN, ID_LANGUAGE_SPANISH_MEXICAN, wxT( "Español (Latinoamericano)" ),  true },
    { wxLANGUAGE_FRENCH,     ID_LANGUAGE_FRENCH,     wxT( "Français" ), true },
    { wxLANGUAGE_KOREAN,     ID_LANGUAGE_KOREAN,     wxT( "한국어"),       true },
    { wxLANGUAGE_ITALIAN,    ID_LANGUAGE_ITALIAN,    wxT( "Italiano" ), true },
    { wxLANGUAGE_LITHUANIAN, ID_LANGUAGE_LITHUANIAN, wxT( "Lietuvių" ), true },
//    { wxLANGUAGE_HUNGARIAN,  ID_LANGUAGE_HUNGARIAN,  wxT( "Magyar" ),   true },
    { wxLANGUAGE_DUTCH,      ID_LANGUAGE_DUTCH,      wxT( "Nederlandse" ), true },
    { wxLANGUAGE_JAPANESE,   ID_LANGUAGE_JAPANESE,   wxT( "日本語" ),    true },
    { wxLANGUAGE_THAI,       ID_LANGUAGE_THAI,       wxT( "ภาษาไทย" ),    true },
    { wxLANGUAGE_POLISH,     ID_LANGUAGE_POLISH,     wxT( "Polski" ),   true },
    { wxLANGUAGE_PORTUGUESE, ID_LANGUAGE_PORTUGUESE, wxT( "Português" ),true },
    { wxLANGUAGE_PORTUGUESE_BRAZILIAN, ID_LANGUAGE_PORTUGUESE_BRAZILIAN, wxT( "Português (Brasil)" ), true },
    { wxLANGUAGE_RUSSIAN,    ID_LANGUAGE_RUSSIAN,    wxT( "Русский" ),  true },
//    { wxLANGUAGE_SERBIAN,    ID_LANGUAGE_SERBIAN,    wxT( "Српски" ),   true },
    { wxLANGUAGE_FINNISH,    ID_LANGUAGE_FINNISH,    wxT( "Suomi" ),    true },
    { wxLANGUAGE_SWEDISH,    ID_LANGUAGE_SWEDISH,    wxT( "Svenska" ),  true },
//    { wxLANGUAGE_VIETNAMESE, ID_LANGUAGE_VIETNAMESE, wxT( "Tiếng Việt" ), true },
//    { wxLANGUAGE_TURKISH,    ID_LANGUAGE_TURKISH,    wxT( "Türkçe" ),   true },
    { wxLANGUAGE_UKRAINIAN,  ID_LANGUAGE_UKRANIAN,   wxT( "Українська" ),   true },
    { wxLANGUAGE_CHINESE_SIMPLIFIED, ID_LANGUAGE_CHINESE_SIMPLIFIED,
            wxT( "简体中文" ), true },
    { wxLANGUAGE_CHINESE_TRADITIONAL, ID_LANGUAGE_CHINESE_TRADITIONAL,
            wxT( "繁體中文" ), false },
    { 0, 0, "", false }         // Sentinel
};
#undef _
#define _(s) wxGetTranslation((s))


PGM_BASE::PGM_BASE()
{
    m_locale = nullptr;
    m_Printing = false;
    m_Quitting = false;
    m_argcUtf8 = 0;
    m_argvUtf8 = nullptr;
    m_splash = nullptr;

    setLanguageId( wxLANGUAGE_DEFAULT );

    ForceSystemPdfBrowser( false );
}


PGM_BASE::~PGM_BASE()
{
    HideSplash();
    Destroy();

    for( int n = 0; n < m_argcUtf8; n++ )
    {
        free( m_argvUtf8[n] );
    }

    delete[] m_argvUtf8;
}


void PGM_BASE::Destroy()
{
    KICAD_CURL::Cleanup();

#ifdef KICAD_USE_SENTRY
    sentry_close();
#endif

    // unlike a normal destructor, this is designed to be called more than once safely:
    delete m_locale;
    m_locale = nullptr;

    m_pgm_checker.reset();
}


wxApp& PGM_BASE::App()
{
    wxASSERT( wxTheApp );
    return *wxTheApp;
}


void PGM_BASE::SetTextEditor( const wxString& aFileName )
{
    m_text_editor = aFileName;
    GetCommonSettings()->m_System.text_editor = aFileName;
}


const wxString& PGM_BASE::GetTextEditor( bool aCanShowFileChooser )
{
    wxString editorname = m_text_editor;

    if( !editorname )
    {
        if( !wxGetEnv(  wxT( "EDITOR" ), &editorname ) )
        {
            // If there is no EDITOR variable set, try the desktop default
#ifdef __WXMAC__
            editorname = wxT( "/usr/bin/open -e" );
#elif __WXX11__
            editorname =  wxT( "/usr/bin/xdg-open" );
#endif
        }
    }

    // If we still don't have an editor name show a dialog asking the user to select one
    if( !editorname && aCanShowFileChooser )
    {
        DisplayInfoMessage( nullptr, _( "No default editor found, you must choose one." ) );

        editorname = AskUserForPreferredEditor();
    }

    // If we finally have a new editor name request it to be copied to m_text_editor and
    // saved to the preferences file.
    if( !editorname.IsEmpty() )
        SetTextEditor( editorname );

    // m_text_editor already has the same value that editorname, or empty if no editor was
    // found/chosen.
    return m_text_editor;
}


const wxString PGM_BASE::AskUserForPreferredEditor( const wxString& aDefaultEditor )
{
    // Create a mask representing the executable files in the current platform
#ifdef __WINDOWS__
    wxString mask( _( "Executable file" ) + wxT( " (*.exe)|*.exe" ) );
#else
    wxString mask( _( "Executable file" ) + wxT( " (*)|*" ) );
#endif

    // Extract the path, name and extension from the default editor (even if the editor's
    // name was empty, this method will succeed and return empty strings).
    wxString path, name, ext;
    wxFileName::SplitPath( aDefaultEditor, &path, &name, &ext );

    // Show the modal editor and return the file chosen (may be empty if the user cancels
    // the dialog).
    return wxFileSelector( _( "Select Preferred Editor" ), path, name, wxT( "." ) + ext,
                           mask, wxFD_OPEN | wxFD_FILE_MUST_EXIST, nullptr );
}


#ifdef KICAD_USE_SENTRY
bool PGM_BASE::IsSentryOptedIn()
{
    KIPLATFORM::POLICY::PBOOL policyState =
            KIPLATFORM::POLICY::GetPolicyBool( POLICY_KEY_DATACOLLECTION );
    if( policyState != KIPLATFORM::POLICY::PBOOL::NOT_CONFIGURED )
    {
        return policyState == KIPLATFORM::POLICY::PBOOL::ENABLED;
    }

    return m_sentry_optin_fn.Exists();
}


void PGM_BASE::SetSentryOptIn( bool aOptIn )
{
    if( aOptIn )
    {
        if( !m_sentry_uid_fn.Exists() )
        {
            sentryCreateUid();
        }

        if( !m_sentry_optin_fn.Exists() )
        {
            wxFFile sentryInitFile( m_sentry_optin_fn.GetFullPath(), "w" );
            sentryInitFile.Write( "" );
            sentryInitFile.Close();
        }
    }
    else
    {
        if( m_sentry_optin_fn.Exists() )
        {
            wxRemoveFile( m_sentry_optin_fn.GetFullPath() );
            sentry_close();
        }
    }
}


wxString PGM_BASE::sentryCreateUid()
{
    boost::uuids::uuid uuid = boost::uuids::random_generator()();
    wxString           userGuid = boost::uuids::to_string( uuid );

    wxFFile sentryInitFile( m_sentry_uid_fn.GetFullPath(), "w" );
    sentryInitFile.Write( userGuid );
    sentryInitFile.Close();

    return userGuid;
}


void PGM_BASE::ResetSentryId()
{
    m_sentryUid = sentryCreateUid();
}


const wxString& PGM_BASE::GetSentryId()
{
    return m_sentryUid;
}


void PGM_BASE::sentryInit()
{
    m_sentry_optin_fn = wxFileName( PATHS::GetUserCachePath(), "sentry-opt-in" );
    m_sentry_uid_fn = wxFileName( PATHS::GetUserCachePath(), "sentry-uid" );

    if( IsSentryOptedIn() )
    {
        wxFFile sentryInitFile( m_sentry_uid_fn.GetFullPath() );
        sentryInitFile.ReadAll( &m_sentryUid );
        sentryInitFile.Close();

        if( m_sentryUid.IsEmpty() || m_sentryUid.length() != 36 )
        {
            m_sentryUid = sentryCreateUid();
        }

        sentry_options_t* options = sentry_options_new();

        sentry_options_set_dsn(
                options,
                "https://463925e689c34632b5172436ffb76de5@sentry-relay.kicad.org/6266565" );

        wxFileName tmp;
        tmp.AssignDir( PATHS::GetUserCachePath() );
        tmp.AppendDir( "sentry" );

        sentry_options_set_database_pathw( options, tmp.GetPathWithSep().wc_str() );
        sentry_options_set_symbolize_stacktraces( options, true );
        sentry_options_set_auto_session_tracking( options, false );

        sentry_options_set_release( options, GetCommitHash().ToStdString().c_str() );

        sentry_init( options );

        sentry_value_t user = sentry_value_new_object();
        sentry_value_set_by_key( user, "id", sentry_value_new_string( m_sentryUid.c_str() ) );
        sentry_set_user( user );

        sentry_set_tag( "kicad.version", GetBuildVersion().ToStdString().c_str() );
    }
}


void PGM_BASE::sentryPrompt()
{
    if( !IsGUI() )
        return;

    KIPLATFORM::POLICY::PBOOL policyState =
            KIPLATFORM::POLICY::GetPolicyBool( POLICY_KEY_DATACOLLECTION );

    if( policyState == KIPLATFORM::POLICY::PBOOL::NOT_CONFIGURED
        && !m_settings_manager->GetCommonSettings()->m_DoNotShowAgain.data_collection_prompt )
    {
        wxMessageDialog optIn = wxMessageDialog(
                nullptr,
                _( "KiCad can anonymously report crashes and special event "
                   "data to developers in order to aid identifying critical bugs "
                   "across the user base more effectively and help profile "
                   "functionality to guide improvements. \n"
                   "If you choose to voluntarily participate, KiCad will automatically "
                   "handle sending said reports when crashes or events occur. \n"
                   "Your design files such as schematic or PCB are not shared in this process." ),
                _( "Data collection opt in request" ), wxYES_NO | wxCENTRE );

        int result = optIn.ShowModal();

        if( result == wxID_YES )
        {
            SetSentryOptIn( true );
            sentryInit();
        }
        else
        {
            SetSentryOptIn( false );
        }

        m_settings_manager->GetCommonSettings()->m_DoNotShowAgain.data_collection_prompt = true;
    }
}
#endif


void PGM_BASE::BuildArgvUtf8()
{
    const wxArrayString& argArray = App().argv.GetArguments();
    m_argcUtf8 = argArray.size();

    m_argvUtf8 = new char*[m_argcUtf8 + 1];
    for( int n = 0; n < m_argcUtf8; n++ )
    {
        m_argvUtf8[n] = wxStrdup( argArray[n].ToUTF8() );
    }

    m_argvUtf8[m_argcUtf8] = NULL;  // null terminator at end of argv
}


void PGM_BASE::ShowSplash()
{
    if( m_splash )
        return;

    m_splash = new wxSplashScreen( KiBitmap( BITMAPS::splash ), wxSPLASH_CENTRE_ON_SCREEN, 0,
                                    NULL, -1, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE | wxSTAY_ON_TOP );
    wxYield();
}


void PGM_BASE::HideSplash()
{
    if( !m_splash )
        return;

    m_splash->Close();
    m_splash->Destroy();
    m_splash = nullptr;
}


bool PGM_BASE::InitPgm( bool aHeadless, bool aSkipPyInit, bool aIsUnitTest )
{
#if defined( __WXMAC__ )
    // Set the application locale to the system default
    wxLogNull noLog;
    wxLocale loc;
    loc.Init();
#endif

    // Just make sure we init precreate any folders early for later code
    // In particular, the user cache path is the most likely to be hit by startup code
    PATHS::EnsureUserPathsExist();

    KICAD_CURL::Init();

#ifdef KICAD_USE_SENTRY
    sentryInit();
#endif
    wxString pgm_name;

    /// Should never happen but boost unit_test isn't playing nicely in some cases
    if( App().argc == 0 )
        pgm_name = wxT( "kicad" );
    else
        pgm_name = wxFileName( App().argv[0] ).GetName().Lower();

#ifdef KICAD_USE_SENTRY
    sentry_set_tag( "kicad.app", pgm_name.c_str() );
#endif

    wxInitAllImageHandlers();

    // Without this the wxPropertyGridManager segfaults on Windows.
    if( !wxPGGlobalVars )
        wxPGInitResourceModule();

#ifndef __WINDOWS__
    if( wxString( wxGetenv( "HOME" ) ).IsEmpty() )
    {
        DisplayErrorMessage( nullptr, _( "Environment variable HOME is empty.  "
                                         "Unable to continue." ) );
        return false;
    }
#endif
    m_pgm_checker = std::make_unique<wxSingleInstanceChecker>();
    m_pgm_checker->Create( pgm_name, wxStandardPaths::Get().GetTempDir() );

    // Init KiCad environment
    // the environment variable KICAD (if exists) gives the kicad path:
    // something like set KICAD=d:\kicad
    bool isDefined = wxGetEnv( wxT( "KICAD" ), &m_kicad_env );

    if( isDefined )    // ensure m_kicad_env ends by "/"
    {
        m_kicad_env.Replace( WIN_STRING_DIR_SEP, UNIX_STRING_DIR_SEP );

        if( !m_kicad_env.IsEmpty() && m_kicad_env.Last() != '/' )
            m_kicad_env += UNIX_STRING_DIR_SEP;
    }

    // Init parameters for configuration
    App().SetVendorName(  wxT( "KiCad" ) );
    App().SetAppName( pgm_name );

    // Install some image handlers, mainly for help
    if( wxImage::FindHandler( wxBITMAP_TYPE_PNG ) == nullptr )
        wxImage::AddHandler( new wxPNGHandler );

    if( wxImage::FindHandler( wxBITMAP_TYPE_GIF ) == nullptr )
        wxImage::AddHandler( new wxGIFHandler );

    if( wxImage::FindHandler( wxBITMAP_TYPE_JPEG ) == nullptr )
        wxImage::AddHandler( new wxJPEGHandler );

    wxFileSystem::AddHandler( new wxZipFSHandler );

    // Analyze the command line & initialize the binary path
    wxString tmp;
    setExecutablePath();
    SetLanguagePath();
    SetDefaultLanguage( tmp );

    m_settings_manager = std::make_unique<SETTINGS_MANAGER>( aHeadless );

    // Our unit test mocks break if we continue
    // A bug caused InitPgm to terminate early in unit tests and the mocks are...simplistic
    // TODO fix the unit tests so this can be removed
    if( aIsUnitTest )
        return false;

    // Something got in the way of settings load: can't continue
    if( !m_settings_manager->IsOK() )
        return false;

    // Set up built-in environment variables (and override them from the system environment if set)
    GetCommonSettings()->InitializeEnvironment();

    // Load color settings after env is initialized
    m_settings_manager->ReloadColorSettings();

    // Load common settings from disk after setting up env vars
    GetSettingsManager().Load( GetCommonSettings() );

    // Init user language *before* calling loadSettings, because
    // env vars could be incorrectly initialized on Linux
    // (if the value contains some non ASCII7 chars, the env var is not initialized)
    SetLanguage( tmp, true );

    // Now that translations are available, inform the user if the OS is unsupported
    WarnUserIfOperatingSystemUnsupported();

    loadCommonSettings();

#ifdef KICAD_USE_SENTRY
    sentryPrompt();
#endif

    ReadPdfBrowserInfos();      // needs GetCommonSettings()

    // Create the python scripting stuff
    // Skip it fot applications that do not use it
    if( !aSkipPyInit )
        m_python_scripting = std::make_unique<SCRIPTING>();

    // TODO(JE): Remove this if apps are refactored to not assume Prj() always works
    // Need to create a project early for now (it can have an empty path for the moment)
    GetSettingsManager().LoadProject( "" );

    // This sets the maximum tooltip display duration to 10s (up from 5) but only affects
    // Windows as other platforms display tooltips while the mouse is not moving
    if( !aHeadless )
    {
        wxToolTip::Enable( true );
        wxToolTip::SetAutoPop( 10000 );
    }

    if( ADVANCED_CFG::GetCfg().m_UpdateUIEventInterval != 0 )
        wxUpdateUIEvent::SetUpdateInterval( ADVANCED_CFG::GetCfg().m_UpdateUIEventInterval );

    // Now the application can safely start, show the splash screen
    if( !aHeadless )
        ShowSplash();

    return true;
}


bool PGM_BASE::setExecutablePath()
{
    m_bin_dir = wxStandardPaths::Get().GetExecutablePath();

#ifdef __WXMAC__
    // On OSX Pgm().GetExecutablePath() will always point to main
    // bundle directory, e.g., /Applications/kicad.app/

    wxFileName fn( m_bin_dir );

    if( fn.GetName() == wxT( "kicad" ) || fn.GetName() == wxT( "kicad-cli" ) )
    {
        // kicad launcher, so just remove the Contents/MacOS part
        fn.RemoveLastDir();
        fn.RemoveLastDir();
    }
    else
    {
        // standalone binaries live in Contents/Applications/<standalone>.app/Contents/MacOS
        fn.RemoveLastDir();
        fn.RemoveLastDir();
        fn.RemoveLastDir();
        fn.RemoveLastDir();
        fn.RemoveLastDir();
    }

    m_bin_dir = fn.GetPath() + wxT( "/" );
#else
    // Use unix notation for paths. I am not sure this is a good idea,
    // but it simplifies compatibility between Windows and Unices.
    // However it is a potential problem in path handling under Windows.
    m_bin_dir.Replace( WIN_STRING_DIR_SEP, UNIX_STRING_DIR_SEP );

    // Remove file name form command line:
    while( m_bin_dir.Last() != '/' && !m_bin_dir.IsEmpty() )
        m_bin_dir.RemoveLast();
#endif

    return true;
}


void PGM_BASE::loadCommonSettings()
{
    m_text_editor = GetCommonSettings()->m_System.text_editor;

    for( const std::pair<wxString, ENV_VAR_ITEM> it : GetCommonSettings()->m_Env.vars )
    {
        wxLogTrace( traceEnvVars, wxT( "PGM_BASE::loadSettings: Found entry %s = %s" ),
                    it.first, it.second.GetValue() );

        // Do not store the env var PROJECT_VAR_NAME ("KIPRJMOD") definition if for some reason
        // it is found in config. (It is reserved and defined as project path)
        if( it.first == PROJECT_VAR_NAME )
            continue;

        // Don't set bogus empty entries in the environment
        if( it.first.IsEmpty() )
            continue;

        // Do not overwrite vars set by the system environment with values from the settings file
        if( it.second.GetDefinedExternally() )
            continue;

        SetLocalEnvVariable( it.first, it.second.GetValue() );
    }
}


void PGM_BASE::SaveCommonSettings()
{
    // GetCommonSettings() is not initialized until fairly late in the
    // process startup: InitPgm(), so test before using:
    if( GetCommonSettings() )
        GetCommonSettings()->m_System.working_dir = wxGetCwd();
}


COMMON_SETTINGS* PGM_BASE::GetCommonSettings() const
{
    return m_settings_manager ? m_settings_manager->GetCommonSettings() : nullptr;
}


bool PGM_BASE::SetLanguage( wxString& aErrMsg, bool first_time )
{
    // Suppress wxWidgets error popups if locale is not found
    wxLogNull doNotLog;

    if( first_time )
    {
        setLanguageId( wxLANGUAGE_DEFAULT );
        // First time SetLanguage is called, the user selected language id is set
        // from common user config settings
        wxString languageSel = GetCommonSettings()->m_System.language;

        // Search for the current selection
        for( unsigned ii = 0; LanguagesList[ii].m_KI_Lang_Identifier != 0; ii++ )
        {
            if( LanguagesList[ii].m_Lang_Label == languageSel )
            {
                setLanguageId( LanguagesList[ii].m_WX_Lang_Identifier );
                break;
            }
        }
    }

    // dictionary file name without extend (full name is kicad.mo)
    wxString dictionaryName( wxT( "kicad" ) );

    delete m_locale;
    m_locale = new wxLocale;

    // don't use wxLOCALE_LOAD_DEFAULT flag so that Init() doesn't return
    // false just because it failed to load wxstd catalog
    if( !m_locale->Init( m_language_id ) )
    {
        wxLogTrace( traceLocale, wxT( "This language is not supported by the system." ) );

        setLanguageId( wxLANGUAGE_DEFAULT );
        delete m_locale;

        m_locale = new wxLocale;
        m_locale->Init( wxLANGUAGE_DEFAULT );

        aErrMsg = _( "This language is not supported by the operating system." );
        return false;
    }
    else if( !first_time )
    {
        wxLogTrace( traceLocale, wxT( "Search for dictionary %s.mo in %s" ) ,
                    dictionaryName, m_locale->GetName() );
    }

    if( !first_time )
    {
        // If we are here, the user has selected another language.
        // Therefore the new preferred language name is stored in common config.
        // Do NOT store the wxWidgets language Id, it can change between wxWidgets
        // versions, for a given language
        wxString languageSel;

        // Search for the current selection language name
        for( unsigned ii = 0;  LanguagesList[ii].m_KI_Lang_Identifier != 0; ii++ )
        {
            if( LanguagesList[ii].m_WX_Lang_Identifier == m_language_id )
            {
                languageSel = LanguagesList[ii].m_Lang_Label;
                break;
            }
        }

        COMMON_SETTINGS* cfg = GetCommonSettings();
        cfg->m_System.language = languageSel;
        cfg->SaveToFile( GetSettingsManager().GetPathForSettingsFile( cfg ) );
    }

    // Try adding the dictionary if it is not currently loaded
    if( !m_locale->IsLoaded( dictionaryName ) )
        m_locale->AddCatalog( dictionaryName );

    // Verify the Kicad dictionary was loaded properly
    // However, for the English language, the dictionary is not mandatory, as
    // all messages are already in English, just restricted to ASCII7 chars,
    // the verification is skipped.
    if( !m_locale->IsLoaded( dictionaryName ) && m_language_id != wxLANGUAGE_ENGLISH )
    {
        wxLogTrace( traceLocale, wxT( "Unable to load dictionary %s.mo in %s" ),
                    dictionaryName, m_locale->GetName() );

        setLanguageId( wxLANGUAGE_DEFAULT );
        delete m_locale;

        m_locale = new wxLocale;
        m_locale->Init( wxLANGUAGE_DEFAULT );

        aErrMsg = _( "The KiCad language file for this language is not installed." );
        return false;
    }

    return true;
}


bool PGM_BASE::SetDefaultLanguage( wxString& aErrMsg )
{
    // Suppress error popups from wxLocale
    wxLogNull doNotLog;

    setLanguageId( wxLANGUAGE_DEFAULT );

    // dictionary file name without extend (full name is kicad.mo)
    wxString dictionaryName( wxT( "kicad" ) );

    delete m_locale;
    m_locale = new wxLocale;
    m_locale->Init();

    // Try adding the dictionary if it is not currently loaded
    if( !m_locale->IsLoaded( dictionaryName ) )
        m_locale->AddCatalog( dictionaryName );

    // Verify the Kicad dictionary was loaded properly
    // However, for the English language, the dictionary is not mandatory, as
    // all messages are already in English, just restricted to ASCII7 chars,
    // the verification is skipped.
    if( !m_locale->IsLoaded( dictionaryName ) && m_language_id != wxLANGUAGE_ENGLISH )
    {
        wxLogTrace( traceLocale, wxT( "Unable to load dictionary %s.mo in %s" ),
                    dictionaryName, m_locale->GetName() );

        setLanguageId( wxLANGUAGE_DEFAULT );
        delete m_locale;

        m_locale = new wxLocale;
        m_locale->Init();

        aErrMsg = _( "The KiCad language file for this language is not installed." );
        return false;
    }

    return true;
}


void PGM_BASE::SetLanguageIdentifier( int menu_id )
{
    wxLogTrace( traceLocale, wxT( "Select language ID %d from %d possible languages." ),
                menu_id, (int)arrayDim( LanguagesList )-1 );

    for( unsigned ii = 0;  LanguagesList[ii].m_KI_Lang_Identifier != 0; ii++ )
    {
        if( menu_id == LanguagesList[ii].m_KI_Lang_Identifier )
        {
            setLanguageId( LanguagesList[ii].m_WX_Lang_Identifier );
            break;
        }
    }
}


wxString PGM_BASE::GetLanguageTag()
{
    const wxLanguageInfo* langInfo = wxLocale::GetLanguageInfo( m_language_id );

    if( !langInfo )
        return "";
    else
    {
        #if wxCHECK_VERSION( 3, 1, 6 )
        wxString str = langInfo->GetCanonicalWithRegion();
        #else
        wxString str = langInfo->CanonicalName;
        #endif
        str.Replace( "_", "-" );

        return str;
    }
}


void PGM_BASE::SetLanguagePath()
{
    wxLocale::AddCatalogLookupPathPrefix( PATHS::GetLocaleDataPath() );

    if( wxGetEnv( wxT( "KICAD_RUN_FROM_BUILD_DIR" ), nullptr ) )
    {
        wxFileName fn( Pgm().GetExecutablePath() );
        fn.RemoveLastDir();
        fn.AppendDir( wxT( "translation" ) );
        wxLocale::AddCatalogLookupPathPrefix( fn.GetPath() );
    }
}


bool PGM_BASE::SetLocalEnvVariable( const wxString& aName, const wxString& aValue )
{
    wxString env;

    if( aName.IsEmpty() )
    {
        wxLogTrace( traceEnvVars,
                    wxT( "PGM_BASE::SetLocalEnvVariable: Attempt to set empty variable to value %s" ),
                    aValue );
        return false;
    }

    // Check to see if the environment variable is already set.
    if( wxGetEnv( aName, &env ) )
    {
        wxLogTrace( traceEnvVars,
                    wxT( "PGM_BASE::SetLocalEnvVariable: Environment variable %s already set to %s" ),
                    aName, env );
        return env == aValue;
    }

    wxLogTrace( traceEnvVars,
                wxT( "PGM_BASE::SetLocalEnvVariable: Setting local environment variable %s to %s" ),
                aName, aValue );

    return wxSetEnv( aName, aValue );
}


void PGM_BASE::SetLocalEnvVariables()
{
    // Overwrites externally defined environment variable until the next time the application
    // is run.
    for( const std::pair<wxString, ENV_VAR_ITEM> m_local_env_var : GetCommonSettings()->m_Env.vars )
    {
        wxLogTrace( traceEnvVars,
                    wxT( "PGM_BASE::SetLocalEnvVariables: Setting local environment variable %s to %s" ),
                    m_local_env_var.first,
                    m_local_env_var.second.GetValue() );
        wxSetEnv( m_local_env_var.first, m_local_env_var.second.GetValue() );
    }
}


ENV_VAR_MAP& PGM_BASE::GetLocalEnvVariables() const
{
    return GetCommonSettings()->m_Env.vars;
}


bool PGM_BASE::IsGUI()
{
    if( !wxTheApp )
        return false;

#if wxCHECK_VERSION( 3, 1, 6 )
    return wxTheApp->IsGUI();
#else
    // wxWidgets older than version 3.1.6 do not have a way to know if the app
    // has a GUI or is a console application.
    // So the trick is to set the App class name when starting kicad-cli, and when
    // the app class name is the kicad-cli class name the app is a console app
    bool run_gui = wxTheApp->GetClassName() != KICAD_CLI_APP_NAME;
    return run_gui;
#endif
}


void PGM_BASE::HandleException( std::exception_ptr aPtr )
{
    try
    {
        if( aPtr )
            std::rethrow_exception( aPtr );
    }
    catch( const IO_ERROR& ioe )
    {
        wxLogError( ioe.What() );
    }
    catch( const std::exception& e )
    {
#ifdef KICAD_USE_SENTRY
        if( IsSentryOptedIn() )
        {
            sentry_value_t exc = sentry_value_new_exception( "exception", e.what() );
            sentry_value_set_stacktrace( exc, NULL, 0 );

            sentry_value_t sentryEvent = sentry_value_new_event();
            sentry_event_add_exception( sentryEvent, exc );
            sentry_capture_event( sentryEvent );
        }
#endif

        wxLogError( wxT( "Unhandled exception class: %s  what: %s" ),
                    FROM_UTF8( typeid( e ).name() ), FROM_UTF8( e.what() ) );
    }
    catch( ... )
    {
        wxLogError( wxT( "Unhandled exception of unknown type" ) );
    }
}


void PGM_BASE::HandleAssert( const wxString& aFile, int aLine, const wxString& aFunc,
                             const wxString& aCond, const wxString& aMsg )
{
    wxString assertStr;
    // Log the assertion details to standard log
    if( !aMsg.empty() )
    {
        assertStr = wxString::Format( "Assertion failed at %s:%d in %s: %s - %s", aFile, aLine,
                                      aFunc, aCond, aMsg );
    }
    else
    {
        assertStr = wxString::Format( "Assertion failed at %s:%d in %s: %s", aFile, aLine, aFunc,
                                      aCond );
    }

#ifndef NDEBUG
    wxLogError( assertStr );
#endif

#ifdef KICAD_USE_SENTRY
    if( IsSentryOptedIn() )
    {
        sentry_value_t exc = sentry_value_new_exception( "assert", assertStr );
        sentry_value_set_stacktrace( exc, NULL, 0 );

        sentry_value_t sentryEvent = sentry_value_new_event();
        sentry_event_add_exception( sentryEvent, exc );
        sentry_capture_event( sentryEvent );
    }
#endif
}
