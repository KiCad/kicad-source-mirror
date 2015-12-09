/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2015 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2008-2015 Wayne Stambaugh <stambaughw@verizon.net>
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
#include <wxstruct.h>
#include <macros.h>
#include <config_params.h>
#include <id.h>
#include <build_version.h>
#include <hotkeys_basic.h>
#include <online_help.h>
#include <gestfich.h>
#include <menus_helpers.h>
#include <confirm.h>
#include <dialog_env_var_config.h>


#define KICAD_COMMON                     wxT( "kicad_common" )

// some key strings used to store parameters in KICAD_COMMON

const wxChar PGM_BASE::workingDirKey[] = wxT( "WorkingDir" );     // public

static const wxChar languageCfgKey[]   = wxT( "LanguageID" );
static const wxChar pathEnvVariables[] = wxT( "EnvironmentVariables" );
static const wxChar showEnvVarWarningDialog[] = wxT( "ShowEnvVarWarningDialog" );
static const wxChar traceEnvVars[]     = wxT( "KIENVVARS" );


/**
 *   A small class to handle the list of existing translations.
 *   The locale translation is automatic.
 *   The selection of languages is mainly for maintainer's convenience
 *   To add a support to a new translation:
 *   create a new icon (flag of the country) (see Lang_Fr.xpm as an example)
 *   add a new item to s_Languages[].
 */
struct LANGUAGE_DESCR
{
    /// wxWidgets locale identifier (See wxWidgets doc)
    int         m_WX_Lang_Identifier;

    /// KiCad identifier used in menu selection (See id.h)
    int         m_KI_Lang_Identifier;

    /// The menu language icons
    BITMAP_DEF  m_Lang_Icon;

    /// Labels used in menus
    wxString    m_Lang_Label;

    /// Set to true if the m_Lang_Label must not be translated
    bool        m_DoNotTranslate;
};


/**
 * Variable s_Languages
 * Note: because this list is not created on the fly, wxTranslation
 * must be called when a language name must be displayed after translation.
 * Do not change this behavior, because m_Lang_Label is also used as key in config
 */
static LANGUAGE_DESCR s_Languages[] =
{
    // Default language
    {
        wxLANGUAGE_DEFAULT,
        ID_LANGUAGE_DEFAULT,
        lang_def_xpm,
        _( "Default" )
    },

    // English language
    {
        wxLANGUAGE_ENGLISH,
        ID_LANGUAGE_ENGLISH,
        lang_en_xpm,
        wxT( "English" ),
        true
    },

    // French language
    {
        wxLANGUAGE_FRENCH,
        ID_LANGUAGE_FRENCH,
        lang_fr_xpm,
        _( "French" )
    },

    // Finnish language
    {
        wxLANGUAGE_FINNISH,
        ID_LANGUAGE_FINNISH,
        lang_fi_xpm,
        _( "Finnish" )
    },

    // Spanish language
    {
        wxLANGUAGE_SPANISH,
        ID_LANGUAGE_SPANISH,
        lang_es_xpm,
        _( "Spanish" )
    },

    // Portuguese language
    {
        wxLANGUAGE_PORTUGUESE,
        ID_LANGUAGE_PORTUGUESE,
        lang_pt_xpm,
        _( "Portuguese" )
    },

    // Italian language
    {
        wxLANGUAGE_ITALIAN,
        ID_LANGUAGE_ITALIAN,
        lang_it_xpm,
        _( "Italian" )
    },

    // German language
    {
        wxLANGUAGE_GERMAN,
        ID_LANGUAGE_GERMAN,
        lang_de_xpm,
        _( "German" )
    },

    // Greek language
    {
        wxLANGUAGE_GREEK,
        ID_LANGUAGE_GREEK,
        lang_gr_xpm,
        _( "Greek" )
    },

    // Slovenian language
    {
        wxLANGUAGE_SLOVENIAN,
        ID_LANGUAGE_SLOVENIAN,
        lang_sl_xpm,
        _( "Slovenian" )
    },

    // Hungarian language
    {
        wxLANGUAGE_HUNGARIAN,
        ID_LANGUAGE_HUNGARIAN,
        lang_hu_xpm,
        _( "Hungarian" )
    },

    // Polish language
    {
        wxLANGUAGE_POLISH,
        ID_LANGUAGE_POLISH,
        lang_pl_xpm,
        _( "Polish" )
    },

    // Czech language
    {
        wxLANGUAGE_CZECH,
        ID_LANGUAGE_CZECH,
        lang_cs_xpm,
        _( "Czech" )
    },

    // Russian language
    {
        wxLANGUAGE_RUSSIAN,
        ID_LANGUAGE_RUSSIAN,
        lang_ru_xpm,
        _( "Russian" )
    },

    // Korean language
    {
        wxLANGUAGE_KOREAN,
        ID_LANGUAGE_KOREAN,
        lang_ko_xpm,
        _( "Korean" )
    },

    // Chinese simplified
    {
        wxLANGUAGE_CHINESE_SIMPLIFIED,
        ID_LANGUAGE_CHINESE_SIMPLIFIED,
        lang_chinese_xpm,
        _( "Chinese simplified" )
    },

    // Catalan language
    {
        wxLANGUAGE_CATALAN,
        ID_LANGUAGE_CATALAN,
        lang_catalan_xpm,
        _( "Catalan" )
    },

    // Dutch language
    {
        wxLANGUAGE_DUTCH,
        ID_LANGUAGE_DUTCH,
        lang_nl_xpm,
        _( "Dutch" )
    },

    // Japanese language
    {
        wxLANGUAGE_JAPANESE,
        ID_LANGUAGE_JAPANESE,
        lang_jp_xpm,
        _( "Japanese" )
    },

    // Bulgarian language
    {
        wxLANGUAGE_BULGARIAN,
        ID_LANGUAGE_BULGARIAN,
        lang_bg_xpm,
        _( "Bulgarian" )
    }
};


PGM_BASE::PGM_BASE()
{
    m_pgm_checker = NULL;
    m_locale = NULL;
    m_common_settings = NULL;

    m_wx_app = NULL;
    m_show_env_var_dialog = true;

    setLanguageId( wxLANGUAGE_DEFAULT );

    ForceSystemPdfBrowser( false );
}


PGM_BASE::~PGM_BASE()
{
    destroy();
}


void PGM_BASE::destroy()
{
    // unlike a normal destructor, this is designed to be called more than once safely:

    delete m_common_settings;
    m_common_settings = 0;

    delete m_pgm_checker;
    m_pgm_checker = 0;

    delete m_locale;
    m_locale = 0;
}


void PGM_BASE::SetEditorName( const wxString& aFileName )
{
    m_editor_name = aFileName;
    wxASSERT( m_common_settings );
    m_common_settings->Write( wxT( "Editor" ), aFileName );
}


const wxString& PGM_BASE::GetEditorName()
{
    wxString editorname = m_editor_name;

    if( !editorname )
    {
        // Get the preferred editor name from environment variable first.
        if(!wxGetEnv( wxT( "EDITOR" ), &editorname ))
        {
            // If there is no EDITOR variable set, try the desktop default
#ifdef __WXMAC__
            editorname = "/usr/bin/open";
#elif __WXX11__
            editorname = "/usr/bin/xdg-open";
#endif
        }
    }

    if( !editorname )       // We must get a preferred editor name
    {
        DisplayInfoMessage( NULL,
                            _( "No default editor found, you must choose it" ) );

        wxString mask( wxT( "*" ) );

#ifdef __WINDOWS__
        mask += wxT( ".exe" );
#endif
        editorname = EDA_FILE_SELECTOR( _( "Preferred Editor:" ), wxEmptyString,
                                        wxEmptyString, wxEmptyString, mask,
                                        NULL, wxFD_OPEN, true );
    }

    if( !editorname.IsEmpty() )
    {
        m_editor_name = editorname;
        m_common_settings->Write( wxT( "Editor" ), m_editor_name );
    }

    return m_editor_name;
}


bool PGM_BASE::initPgm()
{
    wxFileName pgm_name( App().argv[0] );

    wxConfigBase::DontCreateOnDemand();

    wxInitAllImageHandlers();

    m_pgm_checker = new wxSingleInstanceChecker( pgm_name.GetName().Lower() + wxT( "-" ) +
                                                 wxGetUserId(), GetKicadLockFilePath() );

    if( m_pgm_checker->IsAnotherRunning() )
    {
        wxString quiz = wxString::Format(
            _( "%s is already running, Continue?" ),
            GetChars( pgm_name.GetName() )
            );

        if( !IsOK( NULL, quiz ) )
            return false;
    }

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
    App().SetVendorName( wxT( "KiCad" ) );
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

    // Only define the default environment variable if they haven't been set in the
    // .kicad_common configuration file.
    if( m_common_settings && !m_common_settings->HasGroup( pathEnvVariables ) )
    {
        wxString envVarName = wxT( "KIGITHUB" );
        ENV_VAR_ITEM envVarItem;
        wxString envValue;
        wxFileName tmpFileName;

        envVarItem.SetValue( wxString( wxT( "https://github.com/KiCad" ) ) );
        envVarItem.SetDefinedExternally( wxGetEnv( envVarName, NULL ) );
        m_local_env_vars[ envVarName ] = envVarItem;

        wxFileName baseSharePath;
        baseSharePath.AssignDir( wxString( wxT( DEFAULT_INSTALL_PATH ) ) );

#if !defined( __WXMAC__ )
        baseSharePath.AppendDir( wxT( "share" ) );
        baseSharePath.AppendDir( wxT( "kicad" ) );
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
            tmpFileName.AppendDir( wxT( "modules" ) );
            envVarItem.SetDefinedExternally( false );
        }
        envVarItem.SetValue( tmpFileName.GetFullPath() );
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
            tmpFileName.AppendDir( wxT( "packages3d" ) );
            envVarItem.SetDefinedExternally( false );
        }
        envVarItem.SetValue( tmpFileName.GetFullPath() );
        m_local_env_vars[ envVarName ] = envVarItem;

        // KICAD_PTEMPLATES
        envVarName = wxT( "KICAD_PTEMPLATES" );
        if( wxGetEnv( envVarName, &envValue ) == true && !envValue.IsEmpty() )
        {
            tmpFileName.AssignDir( envValue );
            envVarItem.SetDefinedExternally( true );
        }
        else
        {
            tmpFileName = baseSharePath;
            tmpFileName.AppendDir( wxT( "template" ) );
            envVarItem.SetDefinedExternally( false );
        }
        envVarItem.SetValue( tmpFileName.GetFullPath() );
        m_local_env_vars[ envVarName ] = envVarItem;
    }

    ReadPdfBrowserInfos();      // needs m_common_settings

    loadCommonSettings();

    SetLanguage( true );

    // Set locale option for separator used in float numbers
    SetLocaleTo_Default();

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

    wxString languageSel;

    m_common_settings->Read( languageCfgKey, &languageSel );
    setLanguageId( wxLANGUAGE_DEFAULT );

    m_common_settings->Read( showEnvVarWarningDialog, &m_show_env_var_dialog );

    // Search for the current selection
    for( unsigned ii = 0; ii < DIM( s_Languages ); ii++ )
    {
        if( s_Languages[ii].m_Lang_Label == languageSel )
        {
            setLanguageId( s_Languages[ii].m_WX_Lang_Identifier );
            break;
        }
    }

    m_editor_name = m_common_settings->Read( wxT( "Editor" ) );

    wxString entry, oldPath;
    wxArrayString entries;
    long index = 0L;

    oldPath = m_common_settings->GetPath();
    m_common_settings->SetPath( pathEnvVariables );

    while( m_common_settings->GetNextEntry( entry, index ) )
    {
        wxLogTrace( traceEnvVars,
                    wxT( "Enumerating over entry %s, %ld." ), GetChars( entry ), index );
        entries.Add( entry );
    }

    for( unsigned i = 0;  i < entries.GetCount();  i++ )
    {
        wxString val = m_common_settings->Read( entries[i], wxEmptyString );
        m_local_env_vars[ entries[i]  ] = ENV_VAR_ITEM( val, wxGetEnv( entries[i], NULL ) );
    }

    for( ENV_VAR_MAP_ITER it = m_local_env_vars.begin(); it != m_local_env_vars.end(); ++it )
        SetLocalEnvVariable( it->first, it->second.GetValue() );

    m_common_settings->SetPath( oldPath );
}


void PGM_BASE::saveCommonSettings()
{
    // m_common_settings is not initialized until fairly late in the
    // process startup: initPgm(), so test before using:
    if( m_common_settings )
    {
        wxString cur_dir = wxGetCwd();

        m_common_settings->Write( workingDirKey, cur_dir );
        m_common_settings->Write( showEnvVarWarningDialog, m_show_env_var_dialog );

        // Save the local environment variables.
        m_common_settings->SetPath( pathEnvVariables );

        for( ENV_VAR_MAP_ITER it = m_local_env_vars.begin(); it != m_local_env_vars.end(); ++it )
        {
            wxLogTrace( traceEnvVars, wxT( "Saving environment varaiable config entry %s as %s" ),
                        GetChars( it->first ),  GetChars( it->second.GetValue() ) );
            m_common_settings->Write( it->first, it->second.GetValue() );
        }

        m_common_settings->SetPath( wxT( ".." ) );
    }
}


bool PGM_BASE::SetLanguage( bool first_time )
{
    bool     retv = true;

    // dictionary file name without extend (full name is kicad.mo)
    wxString dictionaryName( wxT( "kicad" ) );

    delete m_locale;
    m_locale = new wxLocale;

    if( !m_locale->Init( m_language_id ) )
    {
        wxLogDebug( wxT( "This language is not supported by the system." ) );

        setLanguageId( wxLANGUAGE_DEFAULT );
        delete m_locale;

        m_locale = new wxLocale;
        m_locale->Init();
        retv = false;
    }
    else if( !first_time )
    {
        wxLogDebug( wxT( "Search for dictionary %s.mo in %s" ),
                    GetChars( dictionaryName ), GetChars( m_locale->GetName() ) );
    }

    // how about a meaningful comment here.
    if( !first_time )
    {
        wxString languageSel;

        // Search for the current selection
        for( unsigned ii = 0; ii < DIM( s_Languages ); ii++ )
        {
            if( s_Languages[ii].m_WX_Lang_Identifier == m_language_id )
            {
                languageSel = s_Languages[ii].m_Lang_Label;
                break;
            }
        }

        m_common_settings->Write( languageCfgKey, languageSel );
    }

    // Test if floating point notation is working (bug in cross compilation, using wine)
    // Make a conversion double <=> string
    double dtst = 0.5;
    wxString msg;

    extern bool g_DisableFloatingPointLocalNotation;    // See common.cpp

    g_DisableFloatingPointLocalNotation = false;

    msg << dtst;
    double result;
    msg.ToDouble( &result );

    if( result != dtst )  // string to double encode/decode does not work! Bug detected
    {
        // Disable floating point localization:
        g_DisableFloatingPointLocalNotation = true;
        SetLocaleTo_C_standard( );
    }

    if( !m_locale->IsLoaded( dictionaryName ) )
        m_locale->AddCatalog( dictionaryName );

    if( !retv )
        return retv;

    return m_locale->IsOk();
}


void PGM_BASE::SetLanguageIdentifier( int menu_id )
{
    wxLogDebug( wxT( "Select language ID %d from %d possible languages." ),
                menu_id, DIM( s_Languages ) );

    for( unsigned ii = 0; ii < DIM( s_Languages ); ii++ )
    {
        if( menu_id == s_Languages[ii].m_KI_Lang_Identifier )
        {
            setLanguageId( s_Languages[ii].m_WX_Lang_Identifier );
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
        fn.AppendDir( wxT( "share" ) );
        fn.AppendDir( wxT( "internat" ) );

        if( fn.IsDirReadable() )
        {
            wxLogDebug( wxT( "Adding locale lookup path: " ) + fn.GetPath() );
            wxLocale::AddCatalogLookupPathPrefix( fn.GetPath() );
        }

        // Append path for unix standard install
        fn.RemoveLastDir();
        fn.AppendDir( wxT( "kicad" ) );
        fn.AppendDir( wxT( "internat" ) );

        if( fn.IsDirReadable() )
        {
            wxLogDebug( wxT( "Adding locale lookup path: " ) + fn.GetPath() );
            wxLocale::AddCatalogLookupPathPrefix( fn.GetPath() );
        }
    }
}


void PGM_BASE::AddMenuLanguageList( wxMenu* MasterMenu )
{
    wxMenu*      menu = NULL;
    wxMenuItem*  item = MasterMenu->FindItem( ID_LANGUAGE_CHOICE );

    if( item )     // This menu exists, do nothing
        return;

    menu = new wxMenu;

    for( unsigned ii = 0; ii < DIM( s_Languages ); ii++ )
    {
        wxString label;

        if( s_Languages[ii].m_DoNotTranslate )
            label = s_Languages[ii].m_Lang_Label;
        else
            label = wxGetTranslation( s_Languages[ii].m_Lang_Label );

        AddMenuItem( menu, s_Languages[ii].m_KI_Lang_Identifier,
                     label, KiBitmap(s_Languages[ii].m_Lang_Icon ),
                     wxITEM_CHECK );
    }

    AddMenuItem( MasterMenu, menu,
                 ID_LANGUAGE_CHOICE,
                 _( "Language" ),
                 _( "Select application language (only for testing!)" ),
                 KiBitmap( language_xpm ) );

    // Set Check mark on current selected language
    for( unsigned ii = 0;  ii < DIM( s_Languages );  ii++ )
    {
        if( m_language_id == s_Languages[ii].m_WX_Lang_Identifier )
            menu->Check( s_Languages[ii].m_KI_Lang_Identifier, true );
        else
            menu->Check( s_Languages[ii].m_KI_Lang_Identifier, false );
    }
}


bool PGM_BASE::SetLocalEnvVariable( const wxString& aName, const wxString& aValue )
{
    wxString env;

    // Check to see if the environment variable is already set.
    if( wxGetEnv( aName, &env ) )
    {
        wxLogTrace( traceEnvVars, wxT( "Environment variable %s already set to %s." ),
                    GetChars( aName ), GetChars( env ) );
        return env == aValue;
    }

    wxLogTrace( traceEnvVars, wxT( "Setting local environment variable %s to %s." ),
                GetChars( aName ), GetChars( aValue ) );

    return wxSetEnv( aName, aValue );
}


void PGM_BASE::SetLocalEnvVariables( const ENV_VAR_MAP& aEnvVarMap )
{
    m_local_env_vars.clear();
    m_local_env_vars = aEnvVarMap;

    if( m_common_settings )
        m_common_settings->DeleteGroup( pathEnvVariables );

    saveCommonSettings();

    // Overwrites externally defined environment variable until the next time the application
    // is run.
    for( ENV_VAR_MAP_ITER it = m_local_env_vars.begin(); it != m_local_env_vars.end(); ++it )
    {
        wxLogTrace( traceEnvVars, wxT( "Setting local environment variable %s to %s." ),
                    GetChars( it->first ), GetChars( it->second.GetValue() ) );
        wxSetEnv( it->first, it->second.GetValue() );
    }
}


void PGM_BASE::ConfigurePaths( wxWindow* aParent )
{
    DIALOG_ENV_VAR_CONFIG dlg( aParent, GetLocalEnvVariables() );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    ENV_VAR_MAP envVarMap = dlg.GetEnvVarMap();

    for( ENV_VAR_MAP_ITER it = envVarMap.begin(); it != envVarMap.end(); ++it )
    {
        wxLogTrace( traceEnvVars, wxT( "Environment variable %s=%s defined externally = %d" ),
                    GetChars( it->first ), GetChars( it->second.GetValue() ),
                    it->second.GetDefinedExternally() );
    }

    // If any of the environment variables are defined externally, warn the user that the
    // next time kicad is run that the externally defined variables will be used instead of
    // the user's settings.  This is by design.
    if( dlg.ExternalDefsChanged() && m_show_env_var_dialog )
    {
        wxString msg1 = _( "Warning!  Some of paths you have configured have been defined \n"
                           "externally to the running process and will be temporarily overwritten." );
        wxString msg2 = _( "The next time KiCad is launched, any paths that have already\n"
                           "been defined are honored and any settings defined in the path\n"
                           "configuration dialog are ignored.  If you did not intend for this\n"
                           "behavior, either rename any conflicting entries or remove the\n"
                           "external environment variable definition(s) from your system." );
        wxRichMessageDialog dlg( aParent, msg1, _( "Warning" ), wxOK | wxCENTRE );
        dlg.ShowDetailedText( msg2 );
        dlg.ShowCheckBox( _( "Do not show this message again." ) );
        dlg.ShowModal();
        m_show_env_var_dialog = !dlg.IsCheckBoxChecked();
    }

    SetLocalEnvVariables( dlg.GetEnvVarMap() );
}
