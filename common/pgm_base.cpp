/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
#include <wx/msgdlg.h>
#include <wx/propgrid/propgrid.h>
#include <wx/snglinst.h>
#include <wx/stdpaths.h>
#include <wx/sysopt.h>
#include <wx/filedlg.h>
#include <wx/ffile.h>
#include <wx/tooltip.h>

#include <advanced_config.h>
#include <app_monitor.h>
#include <background_jobs_monitor.h>
#include <bitmaps.h>
#include <build_version.h>
#include <common.h>
#include <confirm.h>
#include <core/arraydim.h>
#include <id.h>
#include <kicad_curl/kicad_curl.h>
#include <kiplatform/policy.h>
#include <macros.h>
#include <notifications_manager.h>
#include <paths.h>
#include <pgm_base.h>
#include <policy_keys.h>
#include <python_scripting.h>
#include <settings/common_settings.h>
#include <settings/settings_manager.h>
#include <string_utils.h>
#include <systemdirsappend.h>
#include <thread_pool.h>
#include <trace_helpers.h>

#include <widgets/wx_splash.h>

#ifdef KICAD_IPC_API
#include <api/api_plugin_manager.h>
#include <api/api_server.h>
#include <python_manager.h>
#endif

#ifdef _MSC_VER
#include <winrt/base.h>
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
    { wxLANGUAGE_ARABIC,     ID_LANGUAGE_ARABIC,     wxT( "العربية" ), true },
    { wxLANGUAGE_INDONESIAN, ID_LANGUAGE_INDONESIAN, wxT( "Bahasa Indonesia" ), true },
    { wxLANGUAGE_BULGARIAN,  ID_LANGUAGE_BULGARIAN,  wxT( "Български" ), true },
    { wxLANGUAGE_CATALAN,    ID_LANGUAGE_CATALAN,    wxT( "Català" ), true },
    { wxLANGUAGE_CZECH,      ID_LANGUAGE_CZECH,      wxT( "Čeština" ),  true },
    { wxLANGUAGE_DANISH,     ID_LANGUAGE_DANISH,     wxT( "Dansk" ),    true },
    { wxLANGUAGE_GERMAN,     ID_LANGUAGE_GERMAN,     wxT( "Deutsch" ),  true },
    { wxLANGUAGE_GREEK,      ID_LANGUAGE_GREEK,      wxT( "Ελληνικά" ), true },
    { wxLANGUAGE_ENGLISH,    ID_LANGUAGE_ENGLISH,    wxT( "English" ),  true },
    { wxLANGUAGE_SPANISH,    ID_LANGUAGE_SPANISH,    wxT( "Español" ),  true },
    { wxLANGUAGE_SPANISH_MEXICAN, ID_LANGUAGE_SPANISH_MEXICAN,
      wxT( "Español (Latinoamericano)" ),  true },
    { wxLANGUAGE_FRENCH,     ID_LANGUAGE_FRENCH,     wxT( "Français" ), true },
    { wxLANGUAGE_HEBREW,     ID_LANGUAGE_HEBREW,     wxT( "עברית" ), true },
    { wxLANGUAGE_KOREAN,     ID_LANGUAGE_KOREAN,     wxT( "한국어"),       true },
    { wxLANGUAGE_ITALIAN,    ID_LANGUAGE_ITALIAN,    wxT( "Italiano" ), true },
    { wxLANGUAGE_LATVIAN,    ID_LANGUAGE_LATVIAN,    wxT( "Latviešu" ), true },
    { wxLANGUAGE_LITHUANIAN, ID_LANGUAGE_LITHUANIAN, wxT( "Lietuvių" ), true },
    { wxLANGUAGE_HUNGARIAN,  ID_LANGUAGE_HUNGARIAN,  wxT( "Magyar" ),   true },
    { wxLANGUAGE_DUTCH,      ID_LANGUAGE_DUTCH,      wxT( "Nederlands" ), true },
    { wxLANGUAGE_NORWEGIAN_BOKMAL, ID_LANGUAGE_NORWEGIAN_BOKMAL, wxT( "Norsk Bokmål" ), true },
    { wxLANGUAGE_JAPANESE,   ID_LANGUAGE_JAPANESE,   wxT( "日本語" ),    true },
    { wxLANGUAGE_THAI,       ID_LANGUAGE_THAI,       wxT( "ภาษาไทย" ),    true },
    { wxLANGUAGE_POLISH,     ID_LANGUAGE_POLISH,     wxT( "Polski" ),   true },
    { wxLANGUAGE_PORTUGUESE, ID_LANGUAGE_PORTUGUESE, wxT( "Português" ),true },
    { wxLANGUAGE_PORTUGUESE_BRAZILIAN, ID_LANGUAGE_PORTUGUESE_BRAZILIAN,
      wxT( "Português (Brasil)" ), true },
    { wxLANGUAGE_ROMANIAN,   ID_LANGUAGE_ROMANIAN,   wxT( "Română" ), true },
    { wxLANGUAGE_RUSSIAN,    ID_LANGUAGE_RUSSIAN,    wxT( "Русский" ),  true },
    { wxLANGUAGE_SERBIAN,    ID_LANGUAGE_SERBIAN,    wxT( "Српски" ),   true },
    { wxLANGUAGE_SLOVAK,     ID_LANGUAGE_SLOVAK,     wxT( "Slovenčina" ), true },
    { wxLANGUAGE_SLOVENIAN,  ID_LANGUAGE_SLOVENIAN,  wxT( "Slovenščina" ), true },
    { wxLANGUAGE_FINNISH,    ID_LANGUAGE_FINNISH,    wxT( "Suomi" ),    true },
    { wxLANGUAGE_SWEDISH,    ID_LANGUAGE_SWEDISH,    wxT( "Svenska" ),  true },
    { wxLANGUAGE_VIETNAMESE, ID_LANGUAGE_VIETNAMESE, wxT( "Tiếng Việt" ), true },
    { wxLANGUAGE_TAMIL,      ID_LANGUAGE_TAMIL,      wxT( "தமிழ்" ), true },
    { wxLANGUAGE_TURKISH,    ID_LANGUAGE_TURKISH,    wxT( "Türkçe" ),   true },
    { wxLANGUAGE_UKRAINIAN,  ID_LANGUAGE_UKRAINIAN,   wxT( "Українська" ),   true },
    { wxLANGUAGE_CHINESE_SIMPLIFIED, ID_LANGUAGE_CHINESE_SIMPLIFIED,
            wxT( "简体中文" ), true },
    { wxLANGUAGE_CHINESE_TRADITIONAL, ID_LANGUAGE_CHINESE_TRADITIONAL,
            wxT( "繁體中文" ), true },
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
    m_PropertyGridInitialized = false;

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

    delete m_locale;
    m_locale = nullptr;
}


void PGM_BASE::Destroy()
{
    KICAD_CURL::Cleanup();

    APP_MONITOR::SENTRY::Instance()->Cleanup();

    m_pgm_checker.reset();

#ifdef _MSC_VER
    winrt::uninit_apartment();
#endif
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
void PGM_BASE::sentryPrompt()
{
    if( !IsGUI() )
        return;

    KIPLATFORM::POLICY::PBOOL policyState = KIPLATFORM::POLICY::GetPolicyBool( POLICY_KEY_DATACOLLECTION );

    if( policyState == KIPLATFORM::POLICY::PBOOL::NOT_CONFIGURED
            && !m_settings_manager->GetCommonSettings()->m_DoNotShowAgain.data_collection_prompt )
    {
        wxMessageDialog optIn = wxMessageDialog(
                nullptr,
                _( "KiCad can anonymously report crashes and special event data to developers in order to "
                   "aid identifying critical bugs and help profile functionality to guide improvements. \n"
                   "If you choose to voluntarily participate, KiCad will automatically send said reports "
                   "when crashes or events occur. \n"
                   "Your design files such as schematic and PCB are not shared in this process." ),
                _( "Data Collection Opt In" ), wxYES_NO | wxCENTRE );

        optIn.SetYesNoLabels( _( "Opt In" ), _( "Decline" ) );
        int result = optIn.ShowModal();

        if( result == wxID_YES )
        {
            APP_MONITOR::SENTRY::Instance()->SetSentryOptIn( true );
        }
        else
        {
            APP_MONITOR::SENTRY::Instance()->SetSentryOptIn( false );
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
    // Disabling until we change to load each DSO at startup rather than lazy-load when needed.
    // Note that once the splash screen is re-enabled, there are some remaining bugs to fix:
    // Any wxWidgets error dialogs that appear during startup are hidden by the splash screen,
    // so we either need to prevent these from happening (probably not feasible) or else change
    // the error-handling path to make sure errors go on top of the splash.
#if 0
    if( m_splash )
        return;

    m_splash = new WX_SPLASH( KiBitmap( BITMAPS::splash ), wxSPLASH_CENTRE_ON_SCREEN, 0,
                              NULL, -1, wxDefaultPosition, wxDefaultSize,
                              wxBORDER_NONE | wxSTAY_ON_TOP );
    wxYield();
#endif
}


void PGM_BASE::HideSplash()
{
    if( !m_splash )
        return;

    m_splash->Close( true );
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

    APP_MONITOR::SENTRY::Instance()->Init();

    // Initialize the singleton instance
    m_singleton.Init();

    wxString pgm_name;

    /// Should never happen but boost unit_test isn't playing nicely in some cases
    if( App().argc == 0 )
        pgm_name = wxT( "kicad" );
    else
        pgm_name = wxFileName( App().argv[0] ).GetName().Lower();

    APP_MONITOR::SENTRY::Instance()->AddTag( "kicad.app", pgm_name );

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

    // Ensure the instance checker directory exists
    // It should be globally writable because it is shared between all users on Linux, and so on a
    // multi-user machine, other need to be able to access it to check for the lock files or make
    // their own lock files.
    wxString instanceCheckerDir = PATHS::GetInstanceCheckerPath();
    PATHS::EnsurePathExists( instanceCheckerDir );
    wxChmod( instanceCheckerDir,
             wxPOSIX_USER_READ | wxPOSIX_USER_WRITE | wxPOSIX_USER_EXECUTE |
             wxPOSIX_GROUP_READ | wxPOSIX_GROUP_WRITE | wxPOSIX_GROUP_EXECUTE |
             wxPOSIX_OTHERS_READ | wxPOSIX_OTHERS_WRITE | wxPOSIX_OTHERS_EXECUTE );

    wxString instanceCheckerName = wxString::Format( wxS( "%s-%s" ), pgm_name,
                                                     GetMajorMinorVersion() );

    m_pgm_checker = std::make_unique<wxSingleInstanceChecker>();
    m_pgm_checker->Create( instanceCheckerName, instanceCheckerDir );

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
    SetLanguagePath();
    SetDefaultLanguage( tmp );

#ifdef _MSC_VER
    if( !wxGetEnv( "FONTCONFIG_PATH", NULL ) )
    {
        // We need to set this because the internal fontconfig logic
        // seems to search relative to the dll rather the other logic it
        // has to look for the /etc folder above the dll
        // Also don't set it because we need it in QA cli tests to be set by ctest
        wxSetEnv( "FONTCONFIG_PATH", PATHS::GetWindowsFontConfigDir() );
    }
#endif

#ifdef _MSC_VER
    winrt::init_apartment(winrt::apartment_type::single_threaded);
#endif

    m_settings_manager = std::make_unique<SETTINGS_MANAGER>( aHeadless );
    m_background_jobs_monitor = std::make_unique<BACKGROUND_JOBS_MONITOR>();
    m_notifications_manager = std::make_unique<NOTIFICATIONS_MANAGER>();

#ifdef KICAD_IPC_API
    m_plugin_manager = std::make_unique<API_PLUGIN_MANAGER>( &App() );
#endif

    // Our unit test mocks break if we continue
    // A bug caused InitPgm to terminate early in unit tests and the mocks are...simplistic
    // TODO fix the unit tests so this can be removed
    if( aIsUnitTest )
        return false;

    // Something got in the way of settings load: can't continue
    if( !m_settings_manager->IsOK() )
        return false;

    // Set up built-in environment variables (and override them from the system environment if set)
    COMMON_SETTINGS* commonSettings = GetCommonSettings();
    commonSettings->InitializeEnvironment();

    // Load color settings after env is initialized
    m_settings_manager->ReloadColorSettings();

    // Load common settings from disk after setting up env vars
    GetSettingsManager().Load( commonSettings );

#ifdef KICAD_IPC_API
    // If user doesn't have a saved Python interpreter, try (potentially again) to find one
    if( commonSettings->m_Api.python_interpreter.IsEmpty() )
        commonSettings->m_Api.python_interpreter = PYTHON_MANAGER::FindPythonInterpreter();
#endif

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

    GetNotificationsManager().Load();

    // Create the python scripting stuff
    // Skip it for applications that do not use it
    if( !aSkipPyInit )
        m_python_scripting = std::make_unique<SCRIPTING>();

    // TODO(JE): Remove this if apps are refactored to not assume Prj() always works
    // Need to create a project early for now (it can have an empty path for the moment)
    GetSettingsManager().LoadProject( "" );

#ifdef KICAD_IPC_API
    if( commonSettings->m_Api.enable_server )
        m_plugin_manager->ReloadPlugins();
#endif

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
    {
        return "";
    }
    else
    {
        wxString str = langInfo->GetCanonicalWithRegion();
        str.Replace( "_", "-" );

        return str;
    }
}


void PGM_BASE::SetLanguagePath()
{
#ifdef _MSC_VER
    wxLocale::AddCatalogLookupPathPrefix( PATHS::GetWindowsBaseSharePath() + wxT( "locale" ) );
#endif
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
                    wxT( "PGM_BASE::SetLocalEnvVariable: Attempt to set empty variable to "
                         "value %s" ),
                    aValue );
        return false;
    }

    // Check to see if the environment variable is already set.
    if( wxGetEnv( aName, &env ) )
    {
        wxLogTrace( traceEnvVars,
                    wxT( "PGM_BASE::SetLocalEnvVariable: Environment variable %s already set "
                         "to %s" ),
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
                    wxT( "PGM_BASE::SetLocalEnvVariables: Setting local environment variable %s "
                         "to %s" ),
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

    return wxTheApp->IsGUI();
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
        APP_MONITOR::SENTRY::Instance()->LogException( e.what() );

        wxLogError( wxT( "Unhandled exception class: %s  what: %s" ),
                    From_UTF8( typeid( e ).name() ), From_UTF8( e.what() ) );
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
    APP_MONITOR::ASSERT_CACHE_KEY key = { aFile, aLine, aFunc, aCond };
    APP_MONITOR::SENTRY::Instance()->LogAssert( key, assertStr );
#endif
}


const wxString& PGM_BASE::GetExecutablePath() const
{
    return PATHS::GetExecutablePath();
}


void PGM_BASE::ReadPdfBrowserInfos()
{
    SetPdfBrowserName( GetCommonSettings()->m_System.pdf_viewer_name );
    m_use_system_pdf_browser = GetCommonSettings()->m_System.use_system_pdf_viewer;
}


void PGM_BASE::WritePdfBrowserInfos()
{
    GetCommonSettings()->m_System.pdf_viewer_name = GetPdfBrowserName();
    GetCommonSettings()->m_System.use_system_pdf_viewer = m_use_system_pdf_browser;
}


static PGM_BASE* process;


PGM_BASE& Pgm()
{
    wxASSERT( process ); // KIFACE_GETTER has already been called.
    return *process;
}


// Similar to PGM_BASE& Pgm(), but return nullptr when a *.ki_face is run from a python script.
PGM_BASE* PgmOrNull()
{
    return process;
}


void SetPgm( PGM_BASE* pgm )
{
    process = pgm;
}
