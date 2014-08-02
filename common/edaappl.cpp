/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2008-2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2011 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file edaappl.cpp
 *
 * @brief For the main application: init functions, and language selection
 *        (locale handling)
 */

#include <fctsys.h>
#include <gr_basic.h>
#include <wx/html/htmlwin.h>
#include <wx/fs_zip.h>
#include <wx/dir.h>
#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <wx/apptrait.h>
#include <wx/snglinst.h>
#include <wx/tokenzr.h>

#include <appl_wxstruct.h>
#include <wxstruct.h>
#include <macros.h>
#include <param_config.h>
#include <id.h>
#include <build_version.h>
#include <hotkeys_basic.h>
#include <online_help.h>
#include <gestfich.h>
#include <menus_helpers.h>
#include <fp_lib_table.h>


static const wxChar* CommonConfigPath = wxT( "kicad_common" );


// some key strings used to store parameters in config
static const wxChar backgroundColorKey[] = wxT( "BackgroundColor" );
static const wxChar showPageLimitsKey[]  = wxT( "ShowPageLimits" );
static const wxChar workingDirKey[]  = wxT( "WorkingDir" );
static const wxChar languageCfgKey[] = wxT( "LanguageID" );
static const wxChar kicadFpLibPath[] = wxT( "KicadFootprintLibraryPath" );


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


EDA_APP::EDA_APP()
{
    m_Checker = NULL;
    m_oneInstancePerFileChecker = NULL;
    m_HtmlCtrl = NULL;
    m_settings = NULL;
    setLanguageId( wxLANGUAGE_DEFAULT );
    m_Locale = NULL;
    m_projectSettings = NULL;
    m_commonSettings = NULL;
    ForceSystemPdfBrowser( false );
}


EDA_APP::~EDA_APP()
{
    SaveSettings();

    // delete user datas
    delete m_projectSettings;
    delete m_commonSettings;
    delete m_settings;
    delete m_Checker;
    delete m_oneInstancePerFileChecker;
    delete m_Locale;
}


void EDA_APP::InitEDA_Appl( const wxString& aName, EDA_APP_T aId )
{
    m_Id = aId;
    m_Checker = new wxSingleInstanceChecker( aName.Lower() + wxT( "-" ) + wxGetUserId() );

    // Init KiCad environment
    // the environment variable KICAD (if exists) gives the kicad path:
    // something like set KICAD=d:\kicad
    bool isDefined = wxGetEnv( wxT( "KICAD" ), &m_KicadEnv );

    if( isDefined )    // ensure m_KicadEnv ends by "/"
    {
        m_KicadEnv.Replace( WIN_STRING_DIR_SEP, UNIX_STRING_DIR_SEP );

        if( !m_KicadEnv.IsEmpty() && m_KicadEnv.Last() != '/' )
            m_KicadEnv += UNIX_STRING_DIR_SEP;
    }

    // Prepare On Line Help. Use only lower case for help file names, in order to
    // avoid problems with upper/lower case file names under windows and unix.
#if defined ONLINE_HELP_FILES_FORMAT_IS_HTML
    m_HelpFileName = aName.Lower() + wxT( ".html" );
#elif defined ONLINE_HELP_FILES_FORMAT_IS_PDF
    m_HelpFileName = aName.Lower() + wxT( ".pdf" );
#else
    #error Help files format not defined
#endif

    // Init parameters for configuration
    SetVendorName( wxT( "KiCad" ) );
    SetAppName( aName.Lower() );
    SetTitle( aName );

    m_settings = new wxConfig();

    wxASSERT( m_settings != NULL );

    m_commonSettings = new wxConfig( CommonConfigPath );
    wxASSERT( m_commonSettings != NULL );

    // Install some image handlers, mainly for help
    wxImage::AddHandler( new wxPNGHandler );
    wxImage::AddHandler( new wxGIFHandler );
    wxImage::AddHandler( new wxJPEGHandler );
    wxFileSystem::AddHandler( new wxZipFSHandler );

    // Analyze the command line & init binary path
    SetBinDir();
    SetDefaultSearchPaths();
    SetLanguagePath();
    ReadPdfBrowserInfos();

    // Internationalization: loading the kicad suitable Dictionary
    wxString languageSel;
    m_commonSettings->Read( languageCfgKey, &languageSel);

    setLanguageId( wxLANGUAGE_DEFAULT );

    // Search for the current selection
    for( unsigned ii = 0; ii < DIM( s_Languages ); ii++ )
    {
        if( s_Languages[ii].m_Lang_Label == languageSel )
        {
            setLanguageId( s_Languages[ii].m_WX_Lang_Identifier );
            break;
        }
    }

    bool succes = SetLanguage( true );

    if( !succes )
    {
    }

    // Set locale option for separator used in float numbers
    SetLocaleTo_Default();
}


void EDA_APP::SetHtmlHelpController( wxHtmlHelpController* aController )
{
    delete m_HtmlCtrl;

    m_HtmlCtrl = aController;
}


void EDA_APP::InitOnLineHelp()
{
    wxString fullfilename = FindKicadHelpPath();

#if defined ONLINE_HELP_FILES_FORMAT_IS_HTML
    m_HelpFileName = fullfilename + wxT( ".html" );
    fullfilename  += wxT( "kicad.hhp" );

    if( wxFileExists( fullfilename ) )
    {
        m_HtmlCtrl = new wxHtmlHelpController( wxHF_TOOLBAR | wxHF_CONTENTS |
                                               wxHF_PRINT | wxHF_OPEN_FILES
                                               /*| wxHF_SEARCH */ );
        m_HtmlCtrl->UseConfig( m_commonSettings );
        m_HtmlCtrl->SetTitleFormat( wxT( "KiCad Help" ) );
        m_HtmlCtrl->AddBook( fullfilename );
    }

#elif defined ONLINE_HELP_FILES_FORMAT_IS_PDF
    m_HtmlCtrl = NULL;

#else
    #error Help files format not defined
#endif
}


bool EDA_APP::SetBinDir()
{
// Apple MacOSx
#ifdef __APPLE__

    // Derive path from location of the app bundle
    CFBundleRef mainBundle = CFBundleGetMainBundle();

    if( mainBundle == NULL )
        return false;

    CFURLRef urlref = CFBundleCopyBundleURL( mainBundle );

    if( urlref == NULL )
        return false;

    CFStringRef str = CFURLCopyFileSystemPath( urlref, kCFURLPOSIXPathStyle );

    if( str == NULL )
        return false;

    char* native_str = NULL;
    int   len = CFStringGetMaximumSizeForEncoding( CFStringGetLength( str ),
                                                   kCFStringEncodingUTF8 ) + 1;
    native_str = new char[len];

    CFStringGetCString( str, native_str, len, kCFStringEncodingUTF8 );
    m_BinDir = FROM_UTF8( native_str );
    delete[] native_str;

#elif defined(__UNIX__)     // Linux and non-Apple Unix
    m_BinDir = wxStandardPaths::Get().GetExecutablePath();

#else
    m_BinDir = argv[0];
#endif

    // Use unix notation for paths. I am not sure this is a good idea,
    // but it simplifies compatibility between Windows and Unices.
    // However it is a potential problem in path handling under Windows.
    m_BinDir.Replace( WIN_STRING_DIR_SEP, UNIX_STRING_DIR_SEP );

    // Remove file name form command line:
    while( m_BinDir.Last() != '/' && !m_BinDir.IsEmpty() )
        m_BinDir.RemoveLast();

    return true;
}


void EDA_APP::SetDefaultSearchPaths()
{
    wxString   path = m_BinDir;
    wxPathList tmp;

    m_searchPaths.Clear();

#ifdef __WINDOWS__

    /* m_BinDir path is in unix notation.
     * But wxFileName expect (to work fine) native notation
     * specifically when using a path including a server, like
     * \\myserver\local_path .
     */
    path.Replace( UNIX_STRING_DIR_SEP, WIN_STRING_DIR_SEP );

#endif
    wxFileName fn( path, wxEmptyString );

    /* User environment variable path is the first search path.  Chances are
     * if the user is savvy enough to set an environment variable they know
     * what they are doing. */
    if( ::wxGetEnv( wxT( "KICAD" ), NULL ) )
        tmp.AddEnvList( wxT( "KICAD" ) );

    // Add the user's home path.
    tmp.Add( GetTraits()->GetStandardPaths().GetUserDataDir() );

    // Standard application data path if it is different from the binary path.
    if( fn.GetPath() != GetTraits()->GetStandardPaths().GetDataDir() )
    {
        tmp.Add( GetTraits()->GetStandardPaths().GetDataDir() );
    }

    // Up one level relative to binary path with "share" appended for Windows.
    fn.RemoveLastDir();
    tmp.Add( fn.GetPath() );

    /* The normal OS program file install paths allow for binary to be
     * installed in a different path from the library files.  This is
     * useful for development purposes so the library and documentation
     * files do not need to be installed separately.  If someone can
     * figure out a way to implement this without #ifdef, please do.
     */
#ifdef __WXMSW__
    tmp.AddEnvList( wxT( "PROGRAMFILES" ) );
#elif __WXMAC__
    tmp.Add( wxString( wxGetenv( wxT( "HOME" ) ) ) + wxT( "/Library/Application Support/kicad/" ) );
    tmp.Add( wxT( "/Library/Application Support/kicad/" ) );
#else
    tmp.AddEnvList( wxT( "PATH" ) );
#endif

    // This is the equivalent of CMAKE_INSTALL_PREFIX.  Useful when installed by `make install`.
    tmp.Add( wxT( DEFAULT_INSTALL_PATH ) );

    // Add kicad, kicad/share, share, and share/kicad to each possible base path.
    for( unsigned i = 0; i < tmp.GetCount(); i++ )
    {
        fn = wxFileName( tmp[i], wxEmptyString );

        if( fn.GetPath().AfterLast( fn.GetPathSeparator() ) == wxT( "bin" ) )
            fn.RemoveLastDir();

        m_searchPaths.Add( fn.GetPath() );
        fn.AppendDir( wxT( "kicad" ) );
        m_searchPaths.Add( fn.GetPath() );
        fn.AppendDir( wxT( "share" ) );
        m_searchPaths.Add( fn.GetPath() );
        fn.RemoveLastDir();
        fn.RemoveLastDir();
        fn.AppendDir( wxT( "share" ) );
        m_searchPaths.Add( fn.GetPath() );
        fn.AppendDir( wxT( "kicad" ) );
        m_searchPaths.Add( fn.GetPath() );
    }

    // Remove all non-existent paths from the list.
    for( unsigned i = 0; i < m_searchPaths.GetCount(); i++ )
    {
        if( !wxFileName::IsDirReadable( m_searchPaths[i] ) )
        {
            m_searchPaths.RemoveAt( i );
            i -= 1;
        }
        else
        {
            fn.Clear();
            fn.SetPath( m_searchPaths[i] );

            /* Add schematic library file path to search path list.
             * we must add <kicad path>/library and <kicad path>/library/doc
             */
            if( m_Id == APP_EESCHEMA_T )
            {
                fn.AppendDir( wxT( "library" ) );

                if( fn.IsDirReadable() )
                {
                     m_libSearchPaths.Add( fn.GetPath() );
                }

                // Add schematic doc file path (library/doc)to search path list.
                fn.AppendDir( wxT( "doc" ) );

                if( fn.IsDirReadable() )
                {
                    m_libSearchPaths.Add( fn.GetPath() );
                }

                fn.RemoveLastDir();
                fn.RemoveLastDir(); // point to <kicad path>
            }

            // Add PCB library file path to search path list.
            if( m_Id == APP_PCBNEW_T  ||  m_Id == APP_CVPCB_T )
            {
                fn.AppendDir( wxT( "modules" ) );

                if( fn.IsDirReadable() )
                {
                     m_libSearchPaths.Add( fn.GetPath() );
                }

                // Add 3D module library file path to search path list.
                fn.AppendDir( wxT( "packages3d" ) );

                if( fn.IsDirReadable() )
                {
                    m_libSearchPaths.Add( fn.GetPath() );
                }

                fn.RemoveLastDir();
                fn.RemoveLastDir(); // point to <kicad path>
            }

            // Add KiCad template file path to search path list.
            fn.AppendDir( wxT( "template" ) );

            if( fn.IsDirReadable() )
            {
                 m_libSearchPaths.Add( fn.GetPath() );
            }

            fn.RemoveLastDir();
        }
    }

#if 0 && defined( DEBUG )
    wxLogDebug( wxT( "Library search paths:" ) );

    for( unsigned i = 0;  i < m_libSearchPaths.GetCount();  i++ )
        wxLogDebug( wxT( "    %s" ), GetChars( m_libSearchPaths[i] ) );
#endif
}


void EDA_APP::GetSettings( bool aReopenLastUsedDirectory )
{
    wxASSERT( m_settings != NULL && m_commonSettings != NULL );

    m_HelpSize.x = 500;
    m_HelpSize.y = 400;

    wxString languageSel;

    m_commonSettings->Read( languageCfgKey, &languageSel );
    setLanguageId( wxLANGUAGE_DEFAULT );

    // Search for the current selection
    for( unsigned ii = 0; ii < DIM( s_Languages ); ii++ )
    {
        if( s_Languages[ii].m_Lang_Label == languageSel )
        {
            setLanguageId( s_Languages[ii].m_WX_Lang_Identifier );
            break;
        }
    }

    m_EditorName = m_commonSettings->Read( wxT( "Editor" ) );

    m_fileHistory.Load( *m_settings );

    m_settings->Read( showPageLimitsKey, &g_ShowPageLimits );

    if( aReopenLastUsedDirectory )
    {
        wxString dir;

        if( m_settings->Read( workingDirKey, &dir ) && wxDirExists( dir ) )
        {
            wxSetWorkingDirectory( dir );
        }
    }

    // FIXME OSX Mountain Lion (10.8)
    // Seems that Read doesn't found anything and ColorFromInt Asserts - I'm unable to reproduce
    // on 10.7
    // In general terms I think is better have a failsafe default than an uninit variable
    int draw_bg_color = (int)BLACK;      // Default for all apps but Eeschema

    if( m_Id == APP_EESCHEMA_T )
        draw_bg_color = (int)WHITE;      // Default for Eeschema

    m_settings->Read( backgroundColorKey, &draw_bg_color );
    g_DrawBgColor = ColorFromInt( draw_bg_color );

    // Load per-user search paths from settings file

    wxString   upath;
    int i = 1;

    while( 1 )
    {
        upath = m_commonSettings->Read(
                    wxString::Format( wxT( "LibraryPath%d" ), i ), wxT( "" ) );

        if( upath.IsSameAs( wxT( "" ) ) )
            break;

        m_libSearchPaths.Add( upath );
        i++;
    }
}


void EDA_APP::SaveSettings()
{
    wxASSERT( m_settings != NULL );

    m_settings->Write( showPageLimitsKey, g_ShowPageLimits );
    m_settings->Write( workingDirKey, wxGetCwd() );
    m_settings->Write( backgroundColorKey, (long) g_DrawBgColor );

    // Save the file history list
    m_fileHistory.Save( *m_settings );
}


bool EDA_APP::SetLanguage( bool first_time )
{
    bool     retv = true;

    // dictionary file name without extend (full name is kicad.mo)
    wxString DictionaryName( wxT( "kicad" ) );

    delete m_Locale;
    m_Locale = new wxLocale;

#if wxCHECK_VERSION( 2, 9, 0 )
    if( !m_Locale->Init( m_LanguageId ) )
#else
    if( !m_Locale->Init( m_LanguageId, wxLOCALE_CONV_ENCODING ) )
#endif
    {
        wxLogDebug( wxT( "This language is not supported by the system." ) );

        setLanguageId( wxLANGUAGE_DEFAULT );
        delete m_Locale;

        m_Locale = new wxLocale;
        m_Locale->Init();
        retv = false;
    }
    else if( !first_time )
    {
        wxLogDebug( wxT( "Search for dictionary %s.mo in %s" ),
                    GetChars( DictionaryName ), GetChars( m_Locale->GetName() ) );
    }

    if( !first_time )
    {
        wxString languageSel;

        // Search for the current selection
        for( unsigned ii = 0; ii < DIM( s_Languages ); ii++ )
        {
            if( s_Languages[ii].m_WX_Lang_Identifier == m_LanguageId )
            {
                languageSel = s_Languages[ii].m_Lang_Label;
                break;
            }
        }

        m_commonSettings->Write( languageCfgKey, languageSel );
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

    if( !m_Locale->IsLoaded( DictionaryName ) )
        m_Locale->AddCatalog( DictionaryName );

    if( !retv )
        return retv;

    return m_Locale->IsOk();
}


void EDA_APP::SetLanguageIdentifier( int menu_id )
{
    wxLogDebug( wxT( "Select language ID %d from %zd possible languages." ),
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


void EDA_APP::SetLanguagePath()
{
    // Add defined search paths to locale paths
    if( !m_searchPaths.IsEmpty() )
    {
        for( unsigned i = 0; i < m_searchPaths.GetCount(); i++ )
        {
            wxFileName fn( m_searchPaths[i], wxEmptyString );

            // Append path for Windows and unix KiCad pack install
            fn.AppendDir( wxT( "share" ) );
            fn.AppendDir( wxT( "internat" ) );

            if( fn.DirExists() )
            {
                wxLogDebug( wxT( "Adding locale lookup path: " ) + fn.GetPath() );
                wxLocale::AddCatalogLookupPathPrefix( fn.GetPath() );
            }

            // Append path for unix standard install
            fn.RemoveLastDir();

            // Append path for unix standard install
            fn.AppendDir( wxT( "kicad" ) );
            fn.AppendDir( wxT( "internat" ) );

            if( fn.DirExists() )
            {
                wxLogDebug( wxT( "Adding locale lookup path: " ) + fn.GetPath() );
                wxLocale::AddCatalogLookupPathPrefix( fn.GetPath() );
            }
        }
    }
}


void EDA_APP::AddMenuLanguageList( wxMenu* MasterMenu )
{
    wxMenu*      menu = NULL;
    wxMenuItem*  item;

    item = MasterMenu->FindItem( ID_LANGUAGE_CHOICE );

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
    for( unsigned ii = 0; ii < DIM( s_Languages ); ii++ )
    {
        if( m_LanguageId == s_Languages[ii].m_WX_Lang_Identifier )
            menu->Check( s_Languages[ii].m_KI_Lang_Identifier, true );
        else
            menu->Check( s_Languages[ii].m_KI_Lang_Identifier, false );
    }
}


wxString EDA_APP::FindFileInSearchPaths(
        const wxString& filename, const wxArrayString* subdirs )
{
    size_t     i, j;
    wxFileName fn;
    wxPathList paths;

    for( i = 0; i < m_searchPaths.GetCount(); i++ )
    {
        fn = wxFileName( m_searchPaths[i], wxEmptyString );
        if( subdirs )
        {
            for( j = 0; j < subdirs->GetCount(); j++ )
                fn.AppendDir( subdirs->Item( j ) );
        }

        if( fn.DirExists() )
        {
            paths.Add( fn.GetPath() );
        }
    }

    return paths.FindValidPath( filename );
}


wxString EDA_APP::GetHelpFile()
{
    wxString      fn;
    wxArrayString subdirs, altsubdirs;

    /* FIXME: This is not the ideal way to handle this.  Unfortunately, the
     *        CMake install paths seem to be a moving target so this crude
     *        hack solve the problem of install path differences between
     *        Windows and non-Windows platforms. */

    // Partially fixed, but must be enhanced

    // Create subdir tree for "standard" linux distributions, when KiCad comes
    // from a distribution files are in /usr/share/doc/kicad/help and binaries
    // in /usr/bin or /usr/local/bin
    subdirs.Add( wxT( "share" ) );
    subdirs.Add( _T( "doc" ) );
    subdirs.Add( wxT( "kicad" ) );
    subdirs.Add( _T( "help" ) );

    // Create subdir tree for linux and Windows KiCad pack.
    // Note  the pack form under linux is also useful if a user wants to
    // install KiCad to a server because there is only one path to mount
    // or export (something like /usr/local/kicad).
    // files are in <install dir>/kicad/doc/help
    // (often /usr/local/kicad/kicad/doc/help)
    // <install dir>/kicad/ is retrieved from m_BinDir
    altsubdirs.Add( _T( "doc" ) );
    altsubdirs.Add( _T( "help" ) );

    /* Search for a help file.
     *  we *must* find a help file.
     *  so help is searched in directories in this order:
     *  help/<canonical name> like help/en_GB
     *  help/<short name> like help/en
     *  help/en
     */

    // Step 1 : Try to find help file in help/<canonical name>
    subdirs.Add( m_Locale->GetCanonicalName() );
    altsubdirs.Add( m_Locale->GetCanonicalName() );

    fn = FindFileInSearchPaths( m_HelpFileName, &altsubdirs );

    if( !fn  )
        fn = FindFileInSearchPaths( m_HelpFileName, &subdirs );

    // Step 2 : if not found Try to find help file in help/<short name>
    if( !fn  )
    {
        subdirs.RemoveAt( subdirs.GetCount() - 1 );
        altsubdirs.RemoveAt( altsubdirs.GetCount() - 1 );

        // wxLocale::GetName() does not return always the short name
        subdirs.Add( m_Locale->GetName().BeforeLast( '_' ) );
        altsubdirs.Add( m_Locale->GetName().BeforeLast( '_' ) );

        fn = FindFileInSearchPaths( m_HelpFileName, &altsubdirs );

        if( !fn )
            fn = FindFileInSearchPaths( m_HelpFileName, &subdirs );
    }

    // Step 3 : if not found Try to find help file in help/en
    if( !fn )
    {
        subdirs.RemoveAt( subdirs.GetCount() - 1 );
        altsubdirs.RemoveAt( altsubdirs.GetCount() - 1 );
        subdirs.Add( _T( "en" ) );
        altsubdirs.Add( _T( "en" ) );

        fn = FindFileInSearchPaths( m_HelpFileName, &altsubdirs );

        if( !fn )
            fn = FindFileInSearchPaths( m_HelpFileName, &subdirs );
    }

    return fn;
}


wxString EDA_APP::ReturnLastVisitedLibraryPath( const wxString& aSubPathToSearch )
{
    if( !m_LastVisitedLibPath.IsEmpty() )
        return m_LastVisitedLibPath;

    wxString path;

    /* Initialize default path to the main default lib path
     * this is the second path in list (the first is the project path)
     */
    unsigned pcount = m_libSearchPaths.GetCount();

    if( pcount )
    {
        unsigned ipath = 0;

        if( m_libSearchPaths[0] == wxGetCwd() )
            ipath = 1;

        // First choice of path:
        if( ipath < pcount )
            path = m_libSearchPaths[ipath];

        // Search a sub path matching aSubPathToSearch
        if( !aSubPathToSearch.IsEmpty() )
        {
            for( ; ipath < pcount; ipath++ )
            {
                if( m_libSearchPaths[ipath].Contains( aSubPathToSearch ) )
                {
                    path = m_libSearchPaths[ipath];
                    break;
                }
            }
        }
    }

    if( path.IsEmpty() )
        path = wxGetCwd();

    return path;
}


void EDA_APP::SaveLastVisitedLibraryPath( const wxString& aPath )
{
    m_LastVisitedLibPath = aPath;
}


wxString EDA_APP::ReturnFilenameWithRelativePathInLibPath( const wxString& aFullFilename )
{
    /* If the library path is already in the library search paths
     * list, just add the library name to the list.  Otherwise, add
     * the library name with the full or relative path.
     * the relative path, when possible is preferable,
     * because it preserve use of default libraries paths, when the path is a sub path of
     * these default paths
     * Note we accept only sub paths,
     * not relative paths starting by ../ that are not subpaths and are outside kicad libs paths
     */
    wxFileName fn = aFullFilename;
    wxString   filename = aFullFilename;
    unsigned   pathlen  = fn.GetPath().Len();   /* path len, used to find the better (shortest)
                                                 * subpath within defaults paths */

    for( unsigned kk = 0; kk < m_libSearchPaths.GetCount(); kk++ )
    {
        fn = aFullFilename;

        // Search for the shortest subpath within m_libSearchPaths:
        if( fn.MakeRelativeTo( m_libSearchPaths[kk] ) )
        {
            if( fn.GetPathWithSep().StartsWith( wxT("..") ) )  // Path outside kicad libs paths
                continue;

            if( pathlen > fn.GetPath().Len() )    // A better (shortest) subpath is found
            {
                filename = fn.GetPathWithSep() + fn.GetFullName();
                pathlen  = fn.GetPath().Len();
            }
        }
    }

    return filename;
}


wxString EDA_APP::FindLibraryPath( const wxString& aFileName )
{
    if( wxFileName::FileExists( aFileName ) )
        return aFileName;
    else
        return m_libSearchPaths.FindValidPath( aFileName );
}


void EDA_APP::RemoveLibraryPath( const wxString& aPaths )
{
    wxStringTokenizer Token( aPaths, wxT( ";\n\r" ) );

    while( Token.HasMoreTokens() )
    {
        wxString path = Token.GetNextToken();

        if( m_libSearchPaths.Index( path, wxFileName::IsCaseSensitive() ) != wxNOT_FOUND )
        {
            m_libSearchPaths.Remove( path );
        }
    }
}


void EDA_APP::InsertLibraryPath( const wxString& aPaths, size_t aIndex )
{
    wxStringTokenizer Token( aPaths, wxT( ";\n\r" ) );

    while( Token.HasMoreTokens() )
    {
        wxString path = Token.GetNextToken();

        if( wxFileName::DirExists( path )
            && m_libSearchPaths.Index( path, wxFileName::IsCaseSensitive() ) == wxNOT_FOUND )
        {
            if( aIndex >= m_libSearchPaths.GetCount() )
            {
                m_libSearchPaths.Add( path );
            }
            else
            {
                m_libSearchPaths.Insert( path, aIndex );
            }

            aIndex++;
        }
    }
}


bool EDA_APP::LockFile( const wxString& fileName )
{
    // first make absolute and normalize, to avoid that different lock files
    // for the same file can be created
    wxFileName fn = fileName;

    fn.MakeAbsolute();

    // semaphore to protect the edition of the file by more than one instance
    if( m_oneInstancePerFileChecker != NULL )
    {
        // it means that we had an open file and we are opening a different one
        delete m_oneInstancePerFileChecker;
    }

    wxString lockFileName = fn.GetFullPath() + wxT( ".lock" );

    lockFileName.Replace( wxT( "/" ), wxT( "_" ) );

    // We can have filenames coming from Windows, so also convert Windows separator
    lockFileName.Replace( wxT( "\\" ), wxT( "_" ) );

    m_oneInstancePerFileChecker = new wxSingleInstanceChecker( lockFileName );

    if( m_oneInstancePerFileChecker &&
        m_oneInstancePerFileChecker->IsAnotherRunning() )
    {
        return false;
    }

    return true;
}


bool EDA_APP::SetFootprintLibTablePath()
{
    wxString    path;
    wxString    kisysmod( wxT( KISYSMOD ) );

    // Set the KISYSMOD environment variable for the current process if it is not already
    // defined in the user's environment.  This is required to expand the global footprint
    // library table paths.
    if( wxGetEnv( kisysmod, &path ) && wxFileName::DirExists( path ) )
        return true;

    // Set the KISYSMOD environment variable to the path defined in the user's configuration
    // if it is defined and the path exists.
    if( m_commonSettings->Read( kicadFpLibPath, &path ) && wxFileName::DirExists( path ) )
    {
        wxSetEnv( kisysmod, path );
        return true;
    }

    // Attempt to determine where the footprint libraries were installed using the legacy
    // library search paths.
    if( !GetLibraryPathList().IsEmpty() )
    {
        unsigned      modFileCount = 0;
        wxString      bestPath;
        wxArrayString tmp;

        for( unsigned i = 0;  i < GetLibraryPathList().GetCount();  i++ )
        {
            unsigned cnt = wxDir::GetAllFiles( GetLibraryPathList()[i], &tmp, wxT( "*.mod" ),
                                               wxDIR_FILES );

            if( cnt > modFileCount )
            {
                modFileCount = cnt;
                bestPath = GetLibraryPathList()[i];
            }
        }

        if( modFileCount != 0 )
        {
            wxSetEnv( kisysmod, bestPath );
            return true;
        }
    }

    return false;
}

/**
 * Function Set3DShapesPath
 * attempts set the environment variable given by aKiSys3Dmod to a valid path.
 * (typically "KISYS3DMOD" )
 * If the environment variable is already set,
 * then it left as is to respect the wishes of the user.
 *
 * The path is determined by attempting to find the path modules/packages3d
 * files in kicad tree.
 * This may or may not be the best path but it provides the best solution for
 * backwards compatibility with the previous 3D shapes search path implementation.
 *
 * @note This must be called after #SetBinDir() is called at least on Windows.
 * Otherwise, the kicad path is not known (Windows specific)
 *
 * @param aKiSys3Dmod = the value of environment variable, typically "KISYS3DMOD"
 * @return false if the aKiSys3Dmod path is not valid.
 */
bool EDA_APP::Set3DShapesPath( const wxString& aKiSys3Dmod )
{
    wxString    path;

    // Set the KISYS3DMOD environment variable for the current process,
    // if it is not already defined in the user's environment and valid.
    if( wxGetEnv( aKiSys3Dmod, &path ) && wxFileName::DirExists( path ) )
        return true;

    // Attempt to determine where the 3D shape libraries were installed using the
    // legacy path:
    // on Unix: /usr/local/kicad/share/modules/packages3d
    // or  /usr/share/kicad/modules/packages3d
    // On Windows: bin../share/modules/packages3d
    wxString relpath( wxT( "modules/packages3d" ) );

// Apple MacOSx
#ifdef __WXMAC__
    path = wxT("/Library/Application Support/kicad/modules/packages3d/");

    if( wxFileName::DirExists( path ) )
    {
        wxSetEnv( aKiSys3Dmod, path );
        return true;
    }

    path = wxString( wxGetenv( wxT( "HOME" ) ) ) + wxT("/Library/Application Support/kicad/modules/packages3d/");

    if( wxFileName::DirExists( path ) )
    {
        wxSetEnv( aKiSys3Dmod, path );
        return true;
    }

#elif defined(__UNIX__)     // Linux and non-Apple Unix
    // Try the home directory:
    path.Empty();
    wxGetEnv( wxT("HOME"), &path );
    path += wxT("/kicad/share/") + relpath;

    if( wxFileName::DirExists( path ) )
    {
        wxSetEnv( aKiSys3Dmod, path );
        return true;
    }

    // Try the standard install path:
    path = wxT("/usr/local/kicad/share/") + relpath;

    if( wxFileName::DirExists( path ) )
    {
        wxSetEnv( aKiSys3Dmod, path );
        return true;
    }

    // Try the official distrib standard install path:
    path = wxT("/usr/share/kicad/") + relpath;

    if( wxFileName::DirExists( path ) )
    {
        wxSetEnv( aKiSys3Dmod, path );
        return true;
    }

#else   // Windows
    // On Windows, the install path is given by the path of executables
    wxFileName fn;
    fn.AssignDir( m_BinDir );
    fn.RemoveLastDir();
    path = fn.GetPathWithSep() + wxT("share/") + relpath;

    if( wxFileName::DirExists( path ) )
    {
        wxSetEnv( aKiSys3Dmod, path );
        return true;
    }
#endif

    return false;
}

