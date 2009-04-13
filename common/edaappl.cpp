/***************/
/* edaappl.cpp */
/***************/

/***
 * @file edaapl.cpp
 *
 * @brief  For the main application: init functions, and language selection
 *         (locale handling)
 ***/

#ifdef KICAD_PYTHON
#   include <pyhandler.h>
#endif

#include "fctsys.h"
#include "gr_basic.h"
#include "wx/html/htmlwin.h"
#include "wx/fs_zip.h"
#include <wx/dir.h>
#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <wx/apptrait.h>
#include <wx/snglinst.h>

#include "appl_wxstruct.h"
#include "common.h"
#include "param_config.h"
#include "worksheet.h"
#include "id.h"
#include "build_version.h"
#include "hotkeys_basic.h"
#include "macros.h"
#include "online_help.h"
#include "bitmaps.h"
#include "gestfich.h"


static const wxChar* CommonConfigPath = wxT( "kicad_common" );


#ifdef __UNIX__
#   define TMP_FILE "/tmp/kicad.tmp"
#endif

/* Just add new languages to the list.  This macro will properly recalculate
 * the size of the array. */
#define LANGUAGE_DESCR_COUNT ( sizeof( s_Language_List ) /     \
                               sizeof( struct LANGUAGE_DESCR ) )

/* Default font size */
#define FONT_DEFAULT_SIZE 10    /* Default font size. */


/**
 * The real font size will be computed at run time
 *   A small class to handle the list on existing translations.
 *   the locale translation is automatic.
 *   the selection of languages is mainly for maintainer's convenience
 *   To add a support to a new translation:
 *   create a new icon (flag of the country) (see Lang_Fr.xpm as an example)
 *   add a new item to s_Language_List[LANGUAGE_DESCR_COUNT]
 *   and set LANGUAGE_DESCR_COUNT to the new value
 */
struct LANGUAGE_DESCR
{
    /* wxWidgets locale identifier (See wxWidgets doc) */
    int m_WX_Lang_Identifier;

    /* KiCad identifier used in menu selection (See id.h) */
    int m_KI_Lang_Identifier;

    /* The menu language icons */
    const char** m_Lang_Icon;

    /* Labels used in menus */
    const wxChar* m_Lang_Label;

    /* Set to true if the m_Lang_Label must not be translated */
    bool m_DoNotTranslate;
};


/**
 * Language list struct
 */
static struct LANGUAGE_DESCR s_Language_List[] =
{
    /* Default language */
    {
        wxLANGUAGE_DEFAULT,
        ID_LANGUAGE_DEFAULT,
        lang_def_xpm,
        _( "Default" )
    },

    /* English language */
    {
        wxLANGUAGE_ENGLISH,
        ID_LANGUAGE_ENGLISH,
        lang_en_xpm,
        wxT( "English" ),
        true
    },

    /* French language */
    {
        wxLANGUAGE_FRENCH,
        ID_LANGUAGE_FRENCH,
        lang_fr_xpm,
        _( "French" )
    },

    /* Spanish language */
    {
        wxLANGUAGE_SPANISH,
        ID_LANGUAGE_SPANISH,
        lang_es_xpm,
        _( "Spanish" )
    },

    /* Portuguese language */
    {
        wxLANGUAGE_PORTUGUESE,
        ID_LANGUAGE_PORTUGUESE,
        lang_pt_xpm,
        _( "Portuguese" )
    },

    /* Italian language */
    {
        wxLANGUAGE_ITALIAN,
        ID_LANGUAGE_ITALIAN,
        lang_it_xpm,
        _( "Italian" )
    },

    /* German language */
    {
        wxLANGUAGE_GERMAN,
        ID_LANGUAGE_GERMAN,
        lang_de_xpm,
        _( "German" )
    },

    /* Slovenian language */
    {
        wxLANGUAGE_SLOVENIAN,
        ID_LANGUAGE_SLOVENIAN,
        lang_sl_xpm,
        _( "Slovenian" )
    },

    /* Hungarian language */
    {
        wxLANGUAGE_HUNGARIAN,
        ID_LANGUAGE_HUNGARIAN,
        lang_hu_xpm,
        _( "Hungarian" )
    },

    /* Polish language */
    {
        wxLANGUAGE_POLISH,
        ID_LANGUAGE_POLISH,
        lang_pl_xpm,
        _( "Polish" )
    },

    /* Czech language */
    {
        wxLANGUAGE_CZECH,
        ID_LANGUAGE_CZECH,
        lang_cs_xpm,
        _( "Czech" )
    },

    /* Russian language */
    {
        wxLANGUAGE_RUSSIAN,
        ID_LANGUAGE_RUSSIAN,
        lang_ru_xpm,
        _( "Russian" )
    },

    /* Korean language */
    {
        wxLANGUAGE_KOREAN,
        ID_LANGUAGE_KOREAN,
        lang_ko_xpm,
        _( "Korean" )
    },

    /* Chinese simplified */
    {
        wxLANGUAGE_CHINESE_SIMPLIFIED,
        ID_LANGUAGE_CHINESE_SIMPLIFIED,
        lang_chinese_xpm,
        _( "Chinese simplified" )
    },

    /* Catalan language */
    {
        wxLANGUAGE_CATALAN,
        ID_LANGUAGE_CATALAN,
        lang_catalan_xpm,
        _( "Catalan" )
    },

    /* Dutch language */
    {
        wxLANGUAGE_DUTCH,
        ID_LANGUAGE_DUTCH,
        lang_nl_xpm,
        _( "Dutch" )
    }
};


/**
 * WinEDA_App Constructor
 */
WinEDA_App::WinEDA_App()
{
    m_Checker  = NULL;
    m_HtmlCtrl = NULL;
    m_EDA_Config  = NULL;
    m_Env_Defined = FALSE;
    m_LanguageId  = wxLANGUAGE_DEFAULT;
    m_PdfBrowserIsDefault = TRUE;
    m_Locale = NULL;
    m_ProjectConfig = NULL;
    m_EDA_CommonConfig = NULL;
}


/**
 * WinEDA_App Destructor
 */
WinEDA_App::~WinEDA_App()
{
    SaveSettings();

    /* delete user datas */
    if( m_ProjectConfig )
        delete m_ProjectConfig;
    if( m_EDA_CommonConfig )
        delete m_EDA_CommonConfig;
    delete m_EDA_Config;
    delete g_StdFont;
    delete g_DialogFont;
    delete g_ItalicFont;
    delete g_FixedFont;
    delete g_MsgFont;
    if( m_Checker )
        delete m_Checker;
    delete m_Locale;
}


/** Function InitEDA_Appl
 * initialise some general parameters
 *  - Default paths (help, libs, bin)and configuration flies names
 *  - Language and locale
 *  - fonts
 * @param aName : used as paths in configuration files
 * @param aId = flag : APP_TYPE_EESCHEMA, APP_TYPE_PCBNEW..
 *    used to choose what default library path must be used
 */
void WinEDA_App::InitEDA_Appl( const wxString& aName, id_app_type aId )
{
    wxString EnvLang;
    m_Id = aId;
    m_Checker = new wxSingleInstanceChecker( aName.Lower() + wxT( "-" ) +
                                             wxGetUserId() );

    /* Init kicad environment
     * the environment variable KICAD (if exists) gives the kicad path:
     * something like set KICAD=d:\kicad
     */
    m_Env_Defined = wxGetEnv( wxT( "KICAD" ), &m_KicadEnv );
    if( m_Env_Defined )    // ensure m_KicadEnv ends by "/"
    {
        m_KicadEnv.Replace( WIN_STRING_DIR_SEP, UNIX_STRING_DIR_SEP );
        if( m_KicadEnv.Last() != '/' )
            m_KicadEnv += UNIX_STRING_DIR_SEP;
    }

/* Prepare On Line Help. Use only lower case for help filenames, in order to
 * avoid problems with upper/lower case filenames under windows and unix */
#if defined ONLINE_HELP_FILES_FORMAT_IS_HTML
    m_HelpFileName = aName.Lower() + wxT( ".html" );
#elif defined ONLINE_HELP_FILES_FORMAT_IS_PDF
    m_HelpFileName = aName.Lower() + wxT( ".pdf" );
#else
    #error Help files format not defined
#endif

    /* Init parameters for configuration */
    SetVendorName( wxT( "kicad" ) );
    SetAppName( aName.Lower() );
    SetTitle( aName );
    m_EDA_Config = new wxConfig( );
    wxASSERT( m_EDA_Config != NULL );
    m_EDA_CommonConfig = new wxConfig( CommonConfigPath );
    wxASSERT( m_EDA_CommonConfig != NULL );

    /* Create the fonts used in dialogs and messages */
    g_StdFontPointSize    = FONT_DEFAULT_SIZE;
    g_MsgFontPointSize    = FONT_DEFAULT_SIZE;
    g_DialogFontPointSize = FONT_DEFAULT_SIZE;
    g_FixedFontPointSize  = FONT_DEFAULT_SIZE;

    g_StdFont = new wxFont( g_StdFontPointSize, wxFONTFAMILY_ROMAN,
                            wxNORMAL, wxNORMAL );

    g_MsgFont = new wxFont( g_StdFontPointSize, wxFONTFAMILY_ROMAN,
                            wxNORMAL, wxNORMAL );

    g_DialogFont = new wxFont( g_DialogFontPointSize, wxFONTFAMILY_ROMAN,
                               wxNORMAL, wxNORMAL );

    g_ItalicFont = new wxFont( g_DialogFontPointSize, wxFONTFAMILY_ROMAN,
                               wxFONTSTYLE_ITALIC, wxNORMAL );

    g_FixedFont = new wxFont( g_FixedFontPointSize, wxFONTFAMILY_MODERN,
                              wxNORMAL, wxNORMAL );

    /* Install some image handlers, mainly for help */
    wxImage::AddHandler( new wxPNGHandler );
    wxImage::AddHandler( new wxGIFHandler );
    wxImage::AddHandler( new wxJPEGHandler );
    wxFileSystem::AddHandler( new wxZipFSHandler );

    // Analise the command line & init binary path
    SetBinDir();
    SetDefaultSearchPaths();
    SetLanguagePath();
    ReadPdfBrowserInfos();

    // Internationalization: loading the kicad suitable Dictionary
    m_EDA_CommonConfig->Read( wxT( "Language" ), &m_LanguageId,
                              wxLANGUAGE_DEFAULT );

    bool succes = SetLanguage( TRUE );
    if( !succes )
    {
    }

    /* Set locale option for separator used in float numbers */
    SetLocaleTo_Default();

#ifdef KICAD_PYTHON
    PyHandler::GetInstance()->SetAppName( name );
#endif
}


/**
 * Init online help
 *
 * @return  none
 */
void WinEDA_App::InitOnLineHelp()
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
        m_HtmlCtrl->UseConfig( m_EDA_CommonConfig );
        m_HtmlCtrl->SetTitleFormat( wxT( "Kicad Help" ) );
        m_HtmlCtrl->AddBook( fullfilename );
    }

#elif defined ONLINE_HELP_FILES_FORMAT_IS_PDF
    m_HtmlCtrl = NULL;

#else
    #error Help files format not defined
#endif
}


/**
 * Find the path to the executable and store it in WinEDA_App::m_BinDir
 *
 * @return TODO
 */
bool WinEDA_App::SetBinDir()
{
/* Apple MacOSx */
#ifdef __APPLE__

    /* Derive path from location of the app bundle */
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
    m_BinDir = CONV_FROM_UTF8( native_str );
    delete[] native_str;

/* Linux and Unix */
#elif defined(__UNIX__)

    // Under Linux, if argv[0] doesn't the complete path to the executable,
    // it's necessary to obtain it using "which <filename>".
    FILE*    ftmp;

    char     Line[1024];
    char     FileName[1024];
    wxString str_arg0;
    int      ii;

    FileName[0] = 0;
    str_arg0    = argv[0];
    if( strchr( (const char*) argv[0], '/' ) == NULL ) // no path
    {
        sprintf( FileName, "which %s > %s", CONV_TO_UTF8( str_arg0 ), TMP_FILE );
        ii = system( FileName );

        if( ( ftmp = fopen( TMP_FILE, "rt" ) ) != NULL )
        {
            fgets( Line, 1000, ftmp );
            fclose( ftmp );
            remove( TMP_FILE );
        }
        m_BinDir = CONV_FROM_UTF8( Line );
    }
    else
        m_BinDir = argv[0];

#else
    m_BinDir = argv[0];
#endif /* __UNIX__ */

    /* Use unix notation for paths. I am not sure this is a good idea,
     * but it simplify compatibility between Windows and Unices
     * However it is a potential problem in path handling under Windows
    */
    m_BinDir.Replace( WIN_STRING_DIR_SEP, UNIX_STRING_DIR_SEP );

    // Remove filename form command line:
    while( m_BinDir.Last() != '/' && !m_BinDir.IsEmpty() )
        m_BinDir.RemoveLast();

    wxFileName pfn( wxT( "/posix/path/specification" ), wxT( "filename" ) );
    wxFileName wfn( wxT( "\\windows\\path\\specification" ), wxT( "filename" ) );
    wxLogDebug( wxT( "Posix path: " ) + pfn.GetFullPath() );
    wxLogDebug( wxT( "Windows path: " ) + wfn.GetFullPath() );
    wxLogDebug( wxT( "Executable path the Kicad way: " ) + m_BinDir );
    wxLogDebug( wxT( "Executable path the wxWidgets way: " ) +
                GetTraits()->GetStandardPaths().GetExecutablePath() );

    return TRUE;
}


/**
 * Set search paths for libraries, modules, internationalization files, etc.
 */
void WinEDA_App::SetDefaultSearchPaths( void )
{
    size_t     i;
    wxString   path = m_BinDir;

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
        m_searchPaths.AddEnvList( wxT( "KICAD" ) );

    /* Add the user's home path. */
    m_searchPaths.Add( GetTraits()->GetStandardPaths().GetUserDataDir() );

    /* Standard application data path if it is different from the binary
     * path. */
    if( fn.GetPath() != GetTraits()->GetStandardPaths().GetDataDir() )
    {
        m_searchPaths.Add( GetTraits()->GetStandardPaths().GetDataDir() );
    }

    /* Up one level relative to binary path with "share" appended for Windows. */
    fn.RemoveLastDir();
    m_searchPaths.Add( fn.GetPath() );
    fn.AppendDir( wxT( "share" ) );
    m_searchPaths.Add( fn.GetPath() );
    fn.AppendDir( wxT( "kicad" ) );
    m_searchPaths.Add( fn.GetPath() );

    /* Remove all non-existent paths from the list. */
    for( i = 0; i < m_searchPaths.GetCount(); i++ )
    {
        wxLogDebug( wxT( "Checking if search path <" ) +
                    m_searchPaths[i] + wxT( "> exists." ) );
        if( !wxFileName::IsDirReadable( m_searchPaths[i] ) )
        {
            wxLogDebug( wxT( "Removing non-existent path <" ) +
                        m_searchPaths[i] + wxT( "> from search path list." ) );
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
            if ( m_Id == APP_TYPE_EESCHEMA )
            {
                fn.AppendDir( wxT( "library") );
                if( fn.IsDirReadable() )
                {
                    wxLogDebug( wxT( "Adding <%s> to library search path list" ),
                                fn.GetPath().c_str() );
                    m_libSearchPaths.Add( fn.GetPath() );
                }

                /* Add schematic doc file path (library/doc)to search path list. */
                fn.AppendDir( wxT( "doc") );
                if( fn.IsDirReadable() )
                {
                    wxLogDebug( wxT( "Adding <%s> to library search path list" ),
                                fn.GetPath().c_str() );
                    m_libSearchPaths.Add( fn.GetPath() );
                }
                fn.RemoveLastDir();
                fn.RemoveLastDir(); // point to <kicad path>
            }

            /* Add kicad template file path to search path list. */
            fn.AppendDir( wxT( "template" ) );

            if( fn.IsDirReadable() )
            {
                wxLogDebug( wxT( "Adding <%s> to library search path list" ),
                            fn.GetPath().c_str() );
                m_libSearchPaths.Add( fn.GetPath() );
            }
            fn.RemoveLastDir();

            /* Add PCB library file path to search path list. */
            if ( (m_Id == APP_TYPE_PCBNEW) ||  (m_Id == APP_TYPE_CVPCB) )
            {
                fn.AppendDir( wxT( "modules" ) );

                if( fn.IsDirReadable() )
                {
                    wxLogDebug( wxT( "Adding <%s> to library search path list" ),
                                fn.GetPath().c_str() );
                    m_libSearchPaths.Add( fn.GetPath() );
                }

                /* Add 3D module library file path to search path list. */
                fn.AppendDir( wxT( "packages3d" ) );

                if( fn.IsDirReadable() )
                {
                    wxLogDebug( wxT( "Adding <%s> to library search path list" ),
                                fn.GetPath().c_str() );
                    m_libSearchPaths.Add( fn.GetPath() );
                }
            }
        }
    }
}


/**
 * Get application settings
 *
 * @return  none
 */
void WinEDA_App::GetSettings()
{
    wxASSERT( m_EDA_Config != NULL && m_EDA_CommonConfig != NULL );

    wxString Line;
    unsigned ii;

    m_HelpSize.x = 500;
    m_HelpSize.y = 400;

    m_LanguageId = m_EDA_CommonConfig->Read( wxT( "Language" ),
                                             wxLANGUAGE_DEFAULT );
    m_EditorName = m_EDA_CommonConfig->Read( wxT( "Editor" ) );
    g_ConfigFileLocationChoice = m_EDA_CommonConfig->Read( HOTKEY_CFG_PATH_OPT,
                                                           0L );

    m_fileHistory.Load( *m_EDA_Config );

    /* Set default font sizes */
    g_StdFontPointSize = m_EDA_Config->Read( wxT( "SdtFontSize" ),
                                             FONT_DEFAULT_SIZE );
    g_MsgFontPointSize = m_EDA_Config->Read( wxT( "MsgFontSize" ),
                                             FONT_DEFAULT_SIZE );
    g_DialogFontPointSize = m_EDA_Config->Read( wxT( "DialogFontSize" ),
                                                FONT_DEFAULT_SIZE );
    g_FixedFontPointSize = m_EDA_Config->Read( wxT( "FixedFontSize" ),
                                               FONT_DEFAULT_SIZE );

    /* Std font type */
    Line = m_EDA_Config->Read( wxT( "SdtFontType" ), wxEmptyString );
    if( !Line.IsEmpty() )
        g_StdFont->SetFaceName( Line );

    /* Sdt font style */
    ii = m_EDA_Config->Read( wxT( "SdtFontStyle" ), wxFONTFAMILY_ROMAN );
    g_StdFont->SetStyle( ii );

    /* Sdt font weight */
    ii = m_EDA_Config->Read( wxT( "SdtFontWeight" ), wxNORMAL );
    g_StdFont->SetWeight( ii );
    g_StdFont->SetPointSize( g_StdFontPointSize );

    /* Msg font type */
    Line = m_EDA_Config->Read( wxT( "MsgFontType" ), wxEmptyString );
    if( !Line.IsEmpty() )
        g_MsgFont->SetFaceName( Line );

    /* Msg font style */
    ii = m_EDA_Config->Read( wxT( "MsgFontStyle" ), wxFONTFAMILY_ROMAN );
    g_MsgFont->SetStyle( ii );

    /* Msg font weight */
    ii = m_EDA_Config->Read( wxT( "MsgFontWeight" ), wxNORMAL );
    g_MsgFont->SetWeight( ii );
    g_MsgFont->SetPointSize( g_MsgFontPointSize );


    Line = m_EDA_Config->Read( wxT( "DialogFontType" ), wxEmptyString );
    if( !Line.IsEmpty() )
        g_DialogFont->SetFaceName( Line );

    ii = m_EDA_Config->Read( wxT( "DialogFontStyle" ), wxFONTFAMILY_ROMAN );
    g_DialogFont->SetStyle( ii );
    ii = m_EDA_Config->Read( wxT( "DialogFontWeight" ), wxNORMAL );
    g_DialogFont->SetWeight( ii );
    g_DialogFont->SetPointSize( g_DialogFontPointSize );

    g_FixedFont->SetPointSize( g_FixedFontPointSize );

    m_EDA_Config->Read( wxT( "ShowPageLimits" ), &g_ShowPageLimits );

    if( m_EDA_Config->Read( wxT( "WorkingDir" ), &Line ) && wxDirExists( Line ) )
    {
        wxSetWorkingDirectory( Line );
    }

    m_EDA_Config->Read( wxT( "BgColor" ), &g_DrawBgColor );
}


/**
 * Save application settings
 *
 * @return  none
 */
void WinEDA_App::SaveSettings()
{
    wxASSERT( m_EDA_Config != NULL );

    /* Sdt font settings */
    m_EDA_Config->Write( wxT( "SdtFontSize" ), g_StdFontPointSize );
    m_EDA_Config->Write( wxT( "SdtFontType" ), g_StdFont->GetFaceName() );

#if wxCHECK_VERSION( 2, 9, 0 )
#warning under wxWidgets 3.0, see how to replace the next lines
#else
    m_EDA_Config->Write( wxT( "SdtFontStyle" ), g_StdFont->GetStyle() );
    m_EDA_Config->Write( wxT( "SdtFontWeight" ), g_StdFont->GetWeight() );

    /* Msg font settings */
    m_EDA_Config->Write( wxT( "MsgFontSize" ), g_MsgFontPointSize );
    m_EDA_Config->Write( wxT( "MsgFontType" ), g_MsgFont->GetFaceName() );
    m_EDA_Config->Write( wxT( "MsgFontStyle" ), g_MsgFont->GetStyle() );
    m_EDA_Config->Write( wxT( "MsgFontWeight" ), g_MsgFont->GetWeight() );

    /* Dialog font settings */
    m_EDA_Config->Write( wxT( "DialogFontSize" ), g_DialogFontPointSize );
    m_EDA_Config->Write( wxT( "DialogFontType" ), g_DialogFont->GetFaceName() );
    m_EDA_Config->Write( wxT( "DialogFontStyle" ), g_DialogFont->GetStyle() );
    m_EDA_Config->Write( wxT( "DialogFontWeight" ), g_DialogFont->GetWeight() );

    /* Misc settings */
    m_EDA_Config->Write( wxT( "FixedFontSize" ), g_FixedFontPointSize );
    m_EDA_Config->Write( wxT( "ShowPageLimits" ), g_ShowPageLimits );
    m_EDA_Config->Write( wxT( "WorkingDir" ), wxGetCwd() );
#endif // wxCHECK_VERSION

    /* Save the file history list */
    m_fileHistory.Save( *m_EDA_Config );
}


/**
 * Set the dictionary file name for internationalization
 * the files are in kicad/internat/xx or kicad/internat/xx_XX
 * and are named kicad.mo
 *
 * @param   first_time  must be set to true the first time this funct is
 *          called, false otherwise
 * @return  true if the language can be set (i.e. if the locale is available)
 */
bool WinEDA_App::SetLanguage( bool first_time )
{
    bool     retv = true;

    // dictionary file name without extend (full name is kicad.mo)
    wxString DictionaryName( wxT( "kicad" ) );

    if( m_Locale )
        delete m_Locale;
    m_Locale = new wxLocale;

    if( !m_Locale->Init( m_LanguageId, wxLOCALE_CONV_ENCODING ) )
    {
        wxLogDebug( wxT( "Failed to initialize " ) +
                    wxLocale::GetLanguageInfo( m_LanguageId )->Description );

        m_LanguageId = wxLANGUAGE_DEFAULT;
        delete m_Locale;
        m_Locale = new wxLocale;
        m_Locale->Init();
        retv = false;
    }

    if( !first_time )
    {
        m_EDA_CommonConfig->Write( wxT( "Language" ), m_LanguageId );
    }

    if( !m_Locale->IsLoaded( DictionaryName ) )
        m_Locale->AddCatalog( DictionaryName );

    if( !retv )
        return retv;

    return m_Locale->IsOk();
}


/**
 * Function SetLanguageIdentifier
 *
 * Set in .m_LanguageId member the wxWidgets language identifier Id  from
 * the kicad menu id (internal menu identifier)
 *
 * @param   menu_id = the kicad menuitem id (returned by Menu Event, when
 *          clicking on a menu item)
 * @return  none
 */
void WinEDA_App::SetLanguageIdentifier( int menu_id )
{
    unsigned int ii;

    wxLogDebug( wxT( "Select language ID %d from %d possible languages." ),
                menu_id, LANGUAGE_DESCR_COUNT );

    for( ii = 0; ii < LANGUAGE_DESCR_COUNT; ii++ )
    {
        if( menu_id == s_Language_List[ii].m_KI_Lang_Identifier )
        {
            m_LanguageId = s_Language_List[ii].m_WX_Lang_Identifier;
            break;
        }
    }
}


void WinEDA_App::SetLanguagePath( void )
{
    size_t i;

    /* Add defined search paths to locale paths */
    if( !m_searchPaths.IsEmpty() )
    {
        for( i = 0; i < m_searchPaths.GetCount(); i++ )
        {
            wxFileName fn( m_searchPaths[i], wxEmptyString );

            // Append path for Windows and unix kicad pack install
            fn.AppendDir( wxT( "share" ) );
            fn.AppendDir( wxT( "internat" ) );
            if( fn.DirExists() )
            {
                wxLogDebug( wxT( "Adding locale lookup path: " ) +
                            fn.GetPath() );
                wxLocale::AddCatalogLookupPathPrefix( fn.GetPath() );
            }

             // Append path for unix standard install
            fn.RemoveLastDir();

             // Append path for unix standard install
            fn.AppendDir( wxT( "kicad" ) );
            fn.AppendDir( wxT( "internat" ) );

            if( fn.DirExists() )
            {
                wxLogDebug( wxT( "Adding locale lookup path: " ) +
                            fn.GetPath() );
                wxLocale::AddCatalogLookupPathPrefix( fn.GetPath() );
            }
        }
    }
}


/** Function AddMenuLanguageList
 * Create menu list for language choice, and add it as submenu to a main menu
 * @param   MasterMenu : The main menu. The sub menu list will be accessible
 *          from the menu item with id ID_LANGUAGE_CHOICE
 * @return  none
 */
void WinEDA_App::AddMenuLanguageList( wxMenu* MasterMenu )
{
    wxMenu*      menu = NULL;
    wxMenuItem*  item;
    unsigned int ii;

    item = MasterMenu->FindItem( ID_LANGUAGE_CHOICE );

    if( item )     // This menu exists, do nothing
        return;

    menu = new wxMenu;
    for( ii = 0; ii < LANGUAGE_DESCR_COUNT; ii++ )
    {
        wxString label;
        if ( s_Language_List[ii].m_DoNotTranslate )
            label = s_Language_List[ii].m_Lang_Label;
        else
            label = wxGetTranslation( s_Language_List[ii].m_Lang_Label );

        item = new wxMenuItem( menu,
                               s_Language_List[ii].m_KI_Lang_Identifier,
                               label, wxEmptyString, wxITEM_CHECK );

        SETBITMAPS( s_Language_List[ii].m_Lang_Icon );
        menu->Append( item );
    }

    ADD_MENUITEM_WITH_HELP_AND_SUBMENU( MasterMenu, menu,
                                        ID_LANGUAGE_CHOICE,
                                        _( "Language" ),
                                        _( "Select application language (only for testing!)" ),
                                        language_xpm );

    // Set Check mark on current selected language
    for( ii = 0; ii < LANGUAGE_DESCR_COUNT; ii++ )
    {
        if( m_LanguageId == s_Language_List[ii].m_WX_Lang_Identifier )
            menu->Check( s_Language_List[ii].m_KI_Lang_Identifier, true );
        else
            menu->Check( s_Language_List[ii].m_KI_Lang_Identifier, false );
    }
}


/**
 * Look in search paths for requested file.
 *
 */
wxString WinEDA_App::FindFileInSearchPaths( const wxString&      filename,
                                            const wxArrayString* subdirs )
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
            wxLogDebug( _T( "Adding <" ) + fn.GetPath() + _T( "> to " ) +
                        _T( "file \"" ) + filename + _T( "\" search path." ) );
            paths.Add( fn.GetPath() );
        }
    }

    return paths.FindValidPath( filename );
}


/**
 * Get the help file path.
 *
 * Return the Kicad help file with path.  The base paths defined in
 * m_searchPaths are tested for a valid file.  The path returned can
 * be relative depending on the paths added to m_searchPaths.  See the
 * documentation for wxPathList for more information. If the help file
 * for the current locale is not found, an attempt to find the English
 * version of the help file is made.  wxEmptyString is returned if the
 * help file is not found.
 *  Help file is searched in directories in this order:
 *  help/<canonical name> like help/en_GB
 *  help/<short name> like help/en
 *  help/en
 */
wxString WinEDA_App::GetHelpFile( void )
{
    wxString      fn;
    wxArrayString subdirs, altsubdirs;

    /* FIXME: This is not the ideal way to handle this.  Unfortunately, the
     *        CMake install paths seem to be a moving target so this crude
     *        hack solve the problem of install path differences between
     *        Windows and non-Windows platforms. */

    // Partially fixed, but must be enhanced

    // Create subdir tree for "standard" linux distributions, when kicad comes
    // from a distribution files are in /usr/share/doc/kicad/help and binaries
    // in /usr/bin or /usr/local/bin
    subdirs.Add( wxT( "share" ) );
    subdirs.Add( _T( "doc" ) );
    subdirs.Add( wxT( "kicad" ) );
    subdirs.Add( _T( "help" ) );

    // Create subdir tree for linux and Windows kicad pack.
    // Note  the pack form under linux is also useful if a user wants to
    // install kicad to a server because there is only one path to mount
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
        if ( ! fn )
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
        if ( ! fn )
            fn = FindFileInSearchPaths( m_HelpFileName, &subdirs );
    }

    return fn;
}


wxString WinEDA_App::GetLibraryFile( const wxString& filename )
{
    wxArrayString subdirs;

    subdirs.Add( wxT( "share" ) );
#ifndef __WXMSW__

    /* Up on level relative to binary path with "share/kicad" appended for
     * all other platforms. */
    subdirs.Add( wxT( "kicad" ) );
#endif
    return FindFileInSearchPaths( filename, &subdirs );
}


/**
 * Kicad saves user defined library files that are not in the standard
 * library search path list with the full file path.  Calling the library
 * search path list with a user library file will fail.  This helper method
 * solves that problem.
 *
 * Returns a wxEmptyString if library file is not found.
 */
wxString WinEDA_App::FindLibraryPath( const wxString& fileName )
{
    if( wxFileName::FileExists( fileName ) )
        return fileName;
    else
        return m_libSearchPaths.FindValidPath( fileName );
}

void WinEDA_App::RemoveLibraryPath( const wxString& path )
{
    if( m_libSearchPaths.Index( path, wxFileName::IsCaseSensitive() ) != wxNOT_FOUND )
    {
        wxLogDebug( wxT( "Removing path <%s> from library path search list." ),
                    path.c_str() );
        m_libSearchPaths.Remove( path );
    }
}

void WinEDA_App::InsertLibraryPath( const wxString& path, size_t index )
{
    if( wxFileName::DirExists( path )
        && m_libSearchPaths.Index( path, wxFileName::IsCaseSensitive() ) == wxNOT_FOUND )
    {
        if( index >= m_libSearchPaths.GetCount() )
        {
            wxLogDebug( wxT( "Adding path <%s> to library path search list." ),
                        path.c_str() );
            m_libSearchPaths.Add( path );
        }
        else
        {
            wxLogDebug( wxT( "Inserting path <%s> in library path search " \
                             "list at index position %d." ),
                        path.c_str(), index );
            m_libSearchPaths.Insert( path, index );
        }
    }
}


/**
 * Run Python scripts
 *
 * @return  The default OnRun() value (exit codes not used in kicad, so value
 *          has no special meaning)
 */
int WinEDA_App::OnRun()
{
#ifdef KICAD_PYTHON
    PyHandler::GetInstance()->RunScripts();
#endif
    return wxApp::OnRun();
}
