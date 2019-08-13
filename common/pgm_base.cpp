/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008 Wayne Stambaugh <stambaughw@gmail.com>
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
 * @file pgm_base.cpp
 *
 * @brief For the main application: init functions, and language selection
 *        (locale handling)
 */

#include <fctsys.h>
#include <wx/html/htmlwin.h>
#include <wx/fs_zip.h>
#include <wx/dir.h>
#include <wx/filename.h>
#include <wx/snglinst.h>
#include <wx/stdpaths.h>
#include <wx/sysopt.h>
#include <wx/richmsgdlg.h>

#include <pgm_base.h>
#include <eda_draw_frame.h>
#include <eda_base_frame.h>
#include <macros.h>
#include <config_params.h>
#include <id.h>
#include <build_version.h>
#include <hotkeys_basic.h>
#include <gestfich.h>
#include <menus_helpers.h>
#include <confirm.h>
#include <dialog_configure_paths.h>
#include <lockfile.h>
#include <systemdirsappend.h>
#include <trace_helpers.h>
#include <gal/gal_display_options.h>

#define KICAD_COMMON                     wxT( "kicad_common" )

// some key strings used to store parameters in KICAD_COMMON

const wxChar PGM_BASE::workingDirKey[] = wxT( "WorkingDir" );     // public

static const wxChar languageCfgKey[]   = wxT( "LanguageID" );
static const wxChar pathEnvVariables[] = wxT( "EnvironmentVariables" );
static const wxChar showEnvVarWarningDialog[] = wxT( "ShowEnvVarWarningDialog" );
static const wxChar traceEnvVars[]     = wxT( "KIENVVARS" );


/**
 * LanguagesList
 * Note: because this list is not created on the fly, wxTranslation
 * must be called when a language name must be displayed after translation.
 * Do not change this behavior, because m_Lang_Label is also used as key in config
 */
#undef _
#define _(s) s
LANGUAGE_DESCR LanguagesList[] =
{
    { wxLANGUAGE_DEFAULT,    ID_LANGUAGE_DEFAULT,    lang_def_xpm,  _( "Default" ) },
    { wxLANGUAGE_ENGLISH,    ID_LANGUAGE_ENGLISH,    lang_en_xpm, wxT( "English" ), true },
    { wxLANGUAGE_FRENCH,     ID_LANGUAGE_FRENCH,     lang_fr_xpm,   _( "French" ) },
    { wxLANGUAGE_FINNISH,    ID_LANGUAGE_FINNISH,    lang_fi_xpm,   _( "Finnish" ) },
    { wxLANGUAGE_SPANISH,    ID_LANGUAGE_SPANISH,    lang_es_xpm,   _( "Spanish" ) },
    { wxLANGUAGE_PORTUGUESE, ID_LANGUAGE_PORTUGUESE, lang_pt_xpm,   _( "Portuguese" ) },
    { wxLANGUAGE_ITALIAN,    ID_LANGUAGE_ITALIAN,    lang_it_xpm,   _( "Italian" ) },
    { wxLANGUAGE_GERMAN,     ID_LANGUAGE_GERMAN,     lang_de_xpm,   _( "German" ) },
    { wxLANGUAGE_GREEK,      ID_LANGUAGE_GREEK,      lang_gr_xpm,   _( "Greek" ) },
    { wxLANGUAGE_SLOVENIAN,  ID_LANGUAGE_SLOVENIAN,  lang_sl_xpm,   _( "Slovenian" ) },
    { wxLANGUAGE_SLOVAK,     ID_LANGUAGE_SLOVAK,     lang_sk_xpm,   _( "Slovak" ) },
    { wxLANGUAGE_HUNGARIAN,  ID_LANGUAGE_HUNGARIAN,  lang_hu_xpm,   _( "Hungarian" ) },
    { wxLANGUAGE_POLISH,     ID_LANGUAGE_POLISH,     lang_pl_xpm,   _( "Polish" ) },
    { wxLANGUAGE_CZECH,      ID_LANGUAGE_CZECH,      lang_cs_xpm,   _( "Czech" ) },
    { wxLANGUAGE_RUSSIAN,    ID_LANGUAGE_RUSSIAN,    lang_ru_xpm,   _( "Russian" ) },
    { wxLANGUAGE_KOREAN,     ID_LANGUAGE_KOREAN,     lang_ko_xpm,   _( "Korean" ) },
    { wxLANGUAGE_CHINESE_SIMPLIFIED, ID_LANGUAGE_CHINESE_SIMPLIFIED, lang_zh_xpm,
                                                            _( "Chinese simplified" ) },
    { wxLANGUAGE_CHINESE_TRADITIONAL, ID_LANGUAGE_CHINESE_TRADITIONAL, lang_zh_xpm,
                                                            _( "Chinese traditional" ) },
    { wxLANGUAGE_CATALAN,    ID_LANGUAGE_CATALAN,    lang_ca_xpm,   _( "Catalan" ) },
    { wxLANGUAGE_DUTCH,      ID_LANGUAGE_DUTCH,      lang_nl_xpm,   _( "Dutch" ) },
    { wxLANGUAGE_JAPANESE,   ID_LANGUAGE_JAPANESE,   lang_jp_xpm,   _( "Japanese" ) },
    { wxLANGUAGE_BULGARIAN,  ID_LANGUAGE_BULGARIAN,  lang_bg_xpm,   _( "Bulgarian" ) },
    { wxLANGUAGE_LITHUANIAN, ID_LANGUAGE_LITHUANIAN, lang_lt_xpm,   _( "Lithuanian" ) },
    { 0, 0, lang_def_xpm,   "" }         // Sentinel
};
#undef _
#define _(s) wxGetTranslation((s))


FILE_HISTORY::FILE_HISTORY( size_t aMaxFiles, int aBaseFileId ) :
        wxFileHistory( std::min( aMaxFiles, (size_t) MAX_FILE_HISTORY_SIZE ) )
{
    SetBaseId( aBaseFileId );
}


void FILE_HISTORY::SetMaxFiles( size_t aMaxFiles )
{
    m_fileMaxFiles = std::min( aMaxFiles, (size_t) MAX_FILE_HISTORY_SIZE );

    size_t numFiles = m_fileHistory.size();

    while( numFiles > m_fileMaxFiles )
        RemoveFileFromHistory( --numFiles );
}


PGM_BASE::PGM_BASE()
{
    m_pgm_checker = NULL;
    m_locale = NULL;
    m_Printing = false;

    m_show_env_var_dialog = true;

    setLanguageId( wxLANGUAGE_DEFAULT );

    ForceSystemPdfBrowser( false );
}


PGM_BASE::~PGM_BASE()
{
    Destroy();
}


void PGM_BASE::Destroy()
{
    // unlike a normal destructor, this is designed to be called more than once safely:
    m_common_settings.reset();

    delete m_pgm_checker;
    m_pgm_checker = 0;

    delete m_locale;
    m_locale = 0;
}


wxApp& PGM_BASE::App()
{
    wxASSERT( wxTheApp );
    return *wxTheApp;
}


void PGM_BASE::SetEditorName( const wxString& aFileName )
{
    m_editor_name = aFileName;
    wxASSERT( m_common_settings );
    m_common_settings->Write( "Editor", aFileName );
}


const wxString& PGM_BASE::GetEditorName( bool aCanShowFileChooser )
{
    wxString editorname = m_editor_name;

    if( !editorname )
    {
        if( !wxGetEnv( "EDITOR", &editorname ) )
        {
            // If there is no EDITOR variable set, try the desktop default
#ifdef __WXMAC__
            editorname = "/usr/bin/open";
#elif __WXX11__
            editorname = "/usr/bin/xdg-open";
#endif
        }
    }

    // If we still don't have an editor name show a dialog asking the user to select one
    if( !editorname && aCanShowFileChooser )
    {
        DisplayInfoMessage( NULL,
                            _( "No default editor found, you must choose it" ) );

        editorname = AskUserForPreferredEditor();
    }

    // If we finally have a new editor name request it to be copied to m_editor_name and
    // saved to the preferences file.
    if( !editorname.IsEmpty() )
        SetEditorName( editorname );

    // m_editor_name already has the same value that editorname, or empty if no editor was
    // found/chosen.
    return m_editor_name;
}


const wxString PGM_BASE::AskUserForPreferredEditor( const wxString& aDefaultEditor )
{
    // Create a mask representing the executable files in the current platform
#ifdef __WINDOWS__
    wxString mask( _( "Executable file (*.exe)|*.exe" ) );
#else
    wxString mask( _( "Executable file (*)|*" ) );
#endif

    // Extract the path, name and extension from the default editor (even if the editor's
    // name was empty, this method will succeed and return empty strings).
    wxString path, name, ext;
    wxFileName::SplitPath( aDefaultEditor, &path, &name, &ext );

    // Show the modal editor and return the file chosen (may be empty if the user cancels
    // the dialog).
    return EDA_FILE_SELECTOR( _( "Select Preferred Editor" ), path,
                              name, ext, mask,
                              NULL, wxFD_OPEN | wxFD_FILE_MUST_EXIST,
                              true );
}


bool PGM_BASE::InitPgm()
{
    wxFileName pgm_name( App().argv[0] );

    wxConfigBase::DontCreateOnDemand();

    wxInitAllImageHandlers();

    m_pgm_checker = new wxSingleInstanceChecker( pgm_name.GetName().Lower() + wxT( "-" ) +
                                                 wxGetUserId(), GetKicadLockFilePath() );

    if( m_pgm_checker->IsAnotherRunning() )
    {
        wxString quiz = wxString::Format(
            _( "%s is already running. Continue?" ),
            GetChars( pgm_name.GetName() )
            );

        if( !IsOK( NULL, quiz ) )
            return false;
    }

    // Init KiCad environment
    // the environment variable KICAD (if exists) gives the kicad path:
    // something like set KICAD=d:\kicad
    bool isDefined = wxGetEnv( "KICAD", &m_kicad_env );

    if( isDefined )    // ensure m_kicad_env ends by "/"
    {
        m_kicad_env.Replace( WIN_STRING_DIR_SEP, UNIX_STRING_DIR_SEP );

        if( !m_kicad_env.IsEmpty() && m_kicad_env.Last() != '/' )
            m_kicad_env += UNIX_STRING_DIR_SEP;
    }

    // Init parameters for configuration
    App().SetVendorName( "KiCad" );
    App().SetAppName( pgm_name.GetName().Lower() );

    // Install some image handlers, mainly for help
    if( wxImage::FindHandler( wxBITMAP_TYPE_PNG ) == NULL )
        wxImage::AddHandler( new wxPNGHandler );

    if( wxImage::FindHandler( wxBITMAP_TYPE_GIF ) == NULL )
        wxImage::AddHandler( new wxGIFHandler );

    if( wxImage::FindHandler( wxBITMAP_TYPE_JPEG ) == NULL )
        wxImage::AddHandler( new wxJPEGHandler );

    wxFileSystem::AddHandler( new wxZipFSHandler );

    // Analyze the command line & initialize the binary path
    setExecutablePath();

    SetLanguagePath();

    // OS specific instantiation of wxConfigBase derivative:
    m_common_settings = GetNewConfig( KICAD_COMMON );

    wxString envVarName = wxT( "KIGITHUB" );
    ENV_VAR_ITEM envVarItem;
    wxString envValue;
    wxFileName tmpFileName;

    if( wxGetEnv( envVarName, &envValue ) == true && !envValue.IsEmpty() )
    {
        tmpFileName.AssignDir( envValue );
        envVarItem.SetDefinedExternally( true );
    }
    else
    {
        envVarItem.SetValue( wxString( wxT( "https://github.com/KiCad" ) ) );
        envVarItem.SetDefinedExternally( false );
    }

    m_local_env_vars[ envVarName ] = envVarItem;

    wxFileName baseSharePath;
#if defined( __WXMSW__ )
    // Make the paths relative to the executable dir as KiCad might be installed anywhere
    // It follows the Windows installer paths scheme, where binaries are installed in
    // PATH/bin and extra files in PATH/share/kicad
    baseSharePath.AssignDir( m_bin_dir + "\\.." );
    baseSharePath.Normalize();
#else
    baseSharePath.AssignDir( wxString( wxT( DEFAULT_INSTALL_PATH ) ) );
#endif

#if !defined( __WXMAC__ )
    baseSharePath.AppendDir( "share" );
    baseSharePath.AppendDir( "kicad" );
#endif

    // KISYSMOD
    envVarName = wxT( "KISYSMOD" );

    if( wxGetEnv( envVarName, &envValue ) == true && !envValue.IsEmpty() )
    {
        tmpFileName.AssignDir( envValue );
        envVarItem.SetDefinedExternally( true );
    }
    else
    {
        tmpFileName = baseSharePath;
        tmpFileName.AppendDir( "modules" );
        envVarItem.SetDefinedExternally( false );
    }

    envVarItem.SetValue( tmpFileName.GetPath() );
    m_local_env_vars[ envVarName ] = envVarItem;

    // KISYS3DMOD
    envVarName = wxT( "KISYS3DMOD" );

    if( wxGetEnv( envVarName, &envValue ) == true && !envValue.IsEmpty() )
    {
        tmpFileName.AssignDir( envValue );
        envVarItem.SetDefinedExternally( true );
    }
    else
    {
        tmpFileName.AppendDir( "packages3d" );
        envVarItem.SetDefinedExternally( false );
    }

    envVarItem.SetValue( tmpFileName.GetFullPath() );
    m_local_env_vars[ envVarName ] = envVarItem;

    // KICAD_TEMPLATE_DIR
    envVarName = "KICAD_TEMPLATE_DIR";

    if( wxGetEnv( envVarName, &envValue ) == true && !envValue.IsEmpty() )
    {
        tmpFileName.AssignDir( envValue );
        envVarItem.SetDefinedExternally( true );
    }
    else
    {
        // Attempt to find the best default template path.
        SEARCH_STACK bases;
        SEARCH_STACK templatePaths;

        SystemDirsAppend( &bases );

        for( unsigned i = 0; i < bases.GetCount(); ++i )
        {
            wxFileName fn( bases[i], wxEmptyString );

            // Add KiCad template file path to search path list.
            fn.AppendDir( "template" );

            // Only add path if exists and can be read by the user.
            if( fn.DirExists() && fn.IsDirReadable() )
            {
                wxLogTrace( tracePathsAndFiles, "Checking template path '%s' exists",
                            fn.GetPath() );
                templatePaths.AddPaths( fn.GetPath() );
            }
        }

        if( templatePaths.IsEmpty() )
        {
            tmpFileName = baseSharePath;
            tmpFileName.AppendDir( "template" );
        }
        else
        {
            // Take the first one.  There may be more but this will likely be the best option.
            tmpFileName.AssignDir( templatePaths[0] );
        }

        envVarItem.SetDefinedExternally( false );
    }

    envVarItem.SetValue( tmpFileName.GetPath() );
    m_local_env_vars[ envVarName ] = envVarItem;

    // KICAD_USER_TEMPLATE_DIR
    envVarName = "KICAD_USER_TEMPLATE_DIR";

    if( wxGetEnv( envVarName, &envValue ) == true && !envValue.IsEmpty() )
    {
        tmpFileName.AssignDir( envValue );
        envVarItem.SetDefinedExternally( true );
    }
    else
    {
        // Default user template path.
        tmpFileName.AssignDir( wxStandardPaths::Get().GetDocumentsDir() );
        tmpFileName.AppendDir( "kicad" );
        tmpFileName.AppendDir( "template" );
        envVarItem.SetDefinedExternally( false );
    }

    envVarItem.SetValue( tmpFileName.GetPath() );
    m_local_env_vars[ envVarName ] = envVarItem;

    // KICAD_SYMBOLS
    envVarName = wxT( "KICAD_SYMBOL_DIR" );

    if( wxGetEnv( envVarName, &envValue ) == true && !envValue.IsEmpty() )
    {
        tmpFileName.AssignDir( envValue );
        envVarItem.SetDefinedExternally( true );
    }
    else
    {
        tmpFileName = baseSharePath;
        tmpFileName.AppendDir( "library" );
        envVarItem.SetDefinedExternally( false );
    }

    envVarItem.SetValue( tmpFileName.GetPath() );
    m_local_env_vars[ envVarName ] = envVarItem;

    ReadPdfBrowserInfos();      // needs m_common_settings

    // Init user language *before* calling loadCommonSettings, because
    // env vars could be incorrectly initialized on Linux
    // (if the value contains some non ASCII7 chars, the env var is not initialized)
    SetLanguage( true );

    loadCommonSettings();

#ifdef __WXMAC__
    // Always show filters on Open dialog to be able to choose plugin
    wxSystemOptions::SetOption( wxOSX_FILEDIALOG_ALWAYS_SHOW_TYPES, 1 );
#endif

    return true;
}


bool PGM_BASE::setExecutablePath()
{
    m_bin_dir = wxStandardPaths::Get().GetExecutablePath();

#ifdef __WXMAC__
    // On OSX Pgm().GetExecutablePath() will always point to main
    // bundle directory, e.g., /Applications/kicad.app/

    wxFileName fn( m_bin_dir );

    if( fn.GetName() == wxT( "kicad" ) )
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
    wxASSERT( m_common_settings );

    m_help_size.x = 500;
    m_help_size.y = 400;

    // This only effect the first time KiCad is run.  The user's setting will be used for all
    // subsequent runs.  Menu icons are off by default on OSX and on for all other platforms.
#if defined( __WXMAC__ )
    bool defaultUseIconsInMenus = false;
#else
    bool defaultUseIconsInMenus = true;
#endif

    m_common_settings->Read( showEnvVarWarningDialog, &m_show_env_var_dialog );

    if( !m_common_settings->HasEntry( USE_ICONS_IN_MENUS_KEY ) )
        m_common_settings->Write( USE_ICONS_IN_MENUS_KEY, defaultUseIconsInMenus );

    if( !m_common_settings->HasEntry( ICON_SCALE_KEY )
        || !m_common_settings->HasEntry( GAL_ANTIALIASING_MODE_KEY )
        || !m_common_settings->HasEntry( CAIRO_ANTIALIASING_MODE_KEY )  )
    {
        // 5.0 and earlier saved common settings in each app, and saved hardware antialiasing
        // options only in pcbnew (which was the only canvas to support them).  Since there's
        // no single right answer to where to pull the common settings from, we might as well
        // get them along with the hardware antialiasing option from pcbnew.
        auto pcbnewConfig = GetNewConfig( wxString::FromUTF8( "pcbnew" ) );
        wxString pcbFrameKey( PCB_EDIT_FRAME_NAME );

        if( !m_common_settings->HasEntry( ICON_SCALE_KEY ) )
        {
            int temp;
            wxString msg;
            bool option;

            pcbnewConfig->Read( "PcbIconScale", &temp, 0 );
            m_common_settings->Write( ICON_SCALE_KEY, temp );

            pcbnewConfig->Read( ENBL_MOUSEWHEEL_PAN_KEY, &option, false );
            m_common_settings->Write( ENBL_MOUSEWHEEL_PAN_KEY, option );

            pcbnewConfig->Read( ENBL_ZOOM_NO_CENTER_KEY, &option, false );
            m_common_settings->Write( ENBL_ZOOM_NO_CENTER_KEY, option );

            pcbnewConfig->Read( ENBL_AUTO_PAN_KEY, &option, true );
            m_common_settings->Write( ENBL_AUTO_PAN_KEY, option );
        }

        if( !m_common_settings->HasEntry( GAL_ANTIALIASING_MODE_KEY ) )
        {
            int temp;
            pcbnewConfig->Read( pcbFrameKey + GAL_DISPLAY_OPTIONS_KEY + GAL_ANTIALIASING_MODE_KEY,
                                &temp, (int) KIGFX::OPENGL_ANTIALIASING_MODE::NONE );
            m_common_settings->Write( GAL_ANTIALIASING_MODE_KEY, temp );
        }

        if( !m_common_settings->HasEntry( CAIRO_ANTIALIASING_MODE_KEY ) )
        {
            int temp;
            pcbnewConfig->Read( pcbFrameKey + GAL_DISPLAY_OPTIONS_KEY + CAIRO_ANTIALIASING_MODE_KEY,
                                &temp, (int) KIGFX::CAIRO_ANTIALIASING_MODE::NONE );
            m_common_settings->Write( CAIRO_ANTIALIASING_MODE_KEY, temp );
        }
    }

    m_editor_name = m_common_settings->Read( "Editor" );

    wxString entry, oldPath;
    wxArrayString entries;
    long index = 0L;

    oldPath = m_common_settings->GetPath();
    m_common_settings->SetPath( pathEnvVariables );

    while( m_common_settings->GetNextEntry( entry, index ) )
    {
        wxLogTrace( traceEnvVars,
                    "Enumerating over entry %s, %ld.", GetChars( entry ), index );

        // Do not store the env var PROJECT_VAR_NAME ("KIPRJMOD") definition if for some reason
        // it is found in config. (It is reserved and defined as project path)
        if( entry == PROJECT_VAR_NAME )
            continue;

        entries.Add( entry );
    }

    for( unsigned i = 0;  i < entries.GetCount();  i++ )
    {
        wxString val = m_common_settings->Read( entries[i], wxEmptyString );

        if( m_local_env_vars[ entries[i] ].GetDefinedExternally() )
            continue;

        m_local_env_vars[ entries[i]  ] = ENV_VAR_ITEM( val, wxGetEnv( entries[i], NULL ) );
    }

    for( ENV_VAR_MAP_ITER it = m_local_env_vars.begin(); it != m_local_env_vars.end(); ++it )
    {
        SetLocalEnvVariable( it->first, it->second.GetValue() );
    }

    m_common_settings->SetPath( oldPath );
}


void PGM_BASE::SaveCommonSettings()
{
    // m_common_settings is not initialized until fairly late in the
    // process startup: InitPgm(), so test before using:
    if( m_common_settings )
    {
        wxString cur_dir = wxGetCwd();

        m_common_settings->Write( workingDirKey, cur_dir );
        m_common_settings->Write( showEnvVarWarningDialog, m_show_env_var_dialog );

        // Save the local environment variables.
        m_common_settings->SetPath( pathEnvVariables );

        for( ENV_VAR_MAP_ITER it = m_local_env_vars.begin(); it != m_local_env_vars.end(); ++it )
        {
            if( it->second.GetDefinedExternally() )
                continue;

            wxLogTrace( traceEnvVars, "Saving environment variable config entry %s as %s",
                        GetChars( it->first ),  GetChars( it->second.GetValue() ) );
            m_common_settings->Write( it->first, it->second.GetValue() );
        }

        m_common_settings->SetPath( ".." );
    }
}


bool PGM_BASE::SetLanguage( bool first_time )
{
    bool     retv = true;

    if( first_time )
    {
        setLanguageId( wxLANGUAGE_DEFAULT );
        // First time SetLanguage is called, the user selected language id is set
        // from commun user config settings
        wxString languageSel;

        m_common_settings->Read( languageCfgKey, &languageSel );

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
    wxString dictionaryName( "kicad" );

    delete m_locale;
    m_locale = new wxLocale;

    if( !m_locale->Init( m_language_id ) )
    {
        wxLogTrace( traceLocale, "This language is not supported by the system." );

        setLanguageId( wxLANGUAGE_DEFAULT );
        delete m_locale;

        m_locale = new wxLocale;
        m_locale->Init();
        retv = false;
    }
    else if( !first_time )
    {
        wxLogTrace( traceLocale, "Search for dictionary %s.mo in %s",
                    GetChars( dictionaryName ), GetChars( m_locale->GetName() ) );
    }

    if( !first_time )
    {
        // If we are here, the user has selected another language.
        // Therefore the new prefered language name is stored in common config.
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

        m_common_settings->Write( languageCfgKey, languageSel );
    }

    // Test if floating point notation is working (bug encountered in cross compilation)
    // Make a conversion double <=> string
    double dtst = 0.5;
    wxString msg;

    msg << dtst;
    double result;
    msg.ToDouble( &result );

    if( result != dtst )
        // string to double encode/decode does not work! Bug detected:
        // Disable floating point localization:
        setlocale( LC_NUMERIC, "C" );

    if( !m_locale->IsLoaded( dictionaryName ) )
        m_locale->AddCatalog( dictionaryName );

    if( !retv )
        return retv;

    return m_locale->IsOk();
}


void PGM_BASE::SetLanguageIdentifier( int menu_id )
{
    wxLogTrace( traceLocale, "Select language ID %d from %d possible languages.",
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


void PGM_BASE::SetLanguagePath()
{
    SEARCH_STACK    guesses;

    SystemDirsAppend( &guesses );

    // Add our internat dir to the wxLocale catalog of paths
    for( unsigned i = 0; i < guesses.GetCount(); i++ )
    {
        wxFileName fn( guesses[i], wxEmptyString );

        // Append path for Windows and unix KiCad package install
        fn.AppendDir( "share" );
        fn.AppendDir( "internat" );

        if( fn.IsDirReadable() )
        {
            wxLogTrace( traceLocale, "Adding locale lookup path: " + fn.GetPath() );
            wxLocale::AddCatalogLookupPathPrefix( fn.GetPath() );
        }

        // Append path for unix standard install
        fn.RemoveLastDir();
        fn.AppendDir( "kicad" );
        fn.AppendDir( "internat" );

        if( fn.IsDirReadable() )
        {
            wxLogTrace( traceLocale, "Adding locale lookup path: " + fn.GetPath() );
            wxLocale::AddCatalogLookupPathPrefix( fn.GetPath() );
        }
    }
}


bool PGM_BASE::SetLocalEnvVariable( const wxString& aName, const wxString& aValue )
{
    wxString env;

    // Check to see if the environment variable is already set.
    if( wxGetEnv( aName, &env ) )
    {
        wxLogTrace( traceEnvVars,  "Environment variable %s already set to %s.",
                    GetChars( aName ), GetChars( env ) );
        return env == aValue;
    }

    wxLogTrace( traceEnvVars, "Setting local environment variable %s to %s.",
                GetChars( aName ), GetChars( aValue ) );

    return wxSetEnv( aName, aValue );
}


void PGM_BASE::SetLocalEnvVariables( const ENV_VAR_MAP& aEnvVarMap )
{
    m_local_env_vars.clear();
    m_local_env_vars = aEnvVarMap;

    if( m_common_settings )
        m_common_settings->DeleteGroup( pathEnvVariables );

    SaveCommonSettings();

    // Overwrites externally defined environment variable until the next time the application
    // is run.
    for( ENV_VAR_MAP_ITER it = m_local_env_vars.begin(); it != m_local_env_vars.end(); ++it )
    {
        wxLogTrace( traceEnvVars, "Setting local environment variable %s to %s.",
                    GetChars( it->first ), GetChars( it->second.GetValue() ) );
        wxSetEnv( it->first, it->second.GetValue() );
    }
}
