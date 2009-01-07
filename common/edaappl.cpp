/* TODO ENGLISH BRIEF TODO */

/***
 * @file edaapl.cpp
 * @brief  methodes relative a la classe winEDA_App, communes
 *          aux environements window et linux
 ***/

#define EDA_BASE
#define COMMON_GLOBL

#ifdef KICAD_PYTHON
#   include <pyhandler.h>
#endif

#include "fctsys.h"
#include "wx/html/htmlwin.h"
#include "wx/fs_zip.h"
#include <wx/filename.h>

#include "common.h"
#include "worksheet.h"
#include "id.h"
#include "build_version.h"
#include "hotkeys_basic.h"
#include "macros.h"
#include "online_help.h"
#include "bitmaps.h"


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
 *   A small class to handle the list od existing translations.
 *   the locale translation is automatic.
 *   the selection of languages is mainly for mainteners's convenience (tests...)\n
 *   To add a support to a new tranlation:
 *   create a new icon (flag of the country) (see Lang_Fr.xpm as an exemple)
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

    /* Portugese language */
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
    m_Checker = NULL;
    m_LastProjectMaxCount = 10;
    m_HtmlCtrl = NULL;
    m_EDA_CommonConfig = NULL;
    m_EDA_Config    = NULL;
    m_Env_Defined   = FALSE;
    m_LanguageId    = wxLANGUAGE_DEFAULT;
    m_Language_Menu = NULL;
    m_Locale = NULL;
    m_PdfBrowserIsDefault = TRUE;
}


/**
 * WinEDA_App Destructor
 */
WinEDA_App::~WinEDA_App()
{
    SaveSettings();

    /* delete user datas */
    delete g_Prj_Config;
    delete m_EDA_Config;
    delete m_EDA_CommonConfig;
    delete g_StdFont;
    delete g_DialogFont;
    delete g_ItalicFont;
    delete g_FixedFont;
    delete g_MsgFont;
    if( m_Checker )
        delete m_Checker;
    delete m_Locale;
}


/**
 * TODO brief
 */
void WinEDA_App::InitEDA_Appl( const wxString& name )
{
    wxString ident;
    wxString EnvLang;

    ident     = name + wxT( "-" ) + wxGetUserId();
    m_Checker = new wxSingleInstanceChecker( ident );

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
    m_HelpFileName = name.Lower() + wxT( ".html" );
#elif defined ONLINE_HELP_FILES_FORMAT_IS_PDF
    m_HelpFileName = name.Lower() + wxT( ".pdf" );
#else
    #error Help files format not defined
#endif

    /* Init parameters for configuration */
    SetVendorName( wxT( "kicad" ) );
    SetAppName( name );
    m_EDA_Config = new wxConfig( name );
    m_EDA_CommonConfig = new wxConfig( wxT( "kicad_common" ) );

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

    /* TODO installation des gestionnaires de visu d'images (pour help) TODO*/
    wxImage::AddHandler( new wxPNGHandler );
    wxImage::AddHandler( new wxGIFHandler );
    wxImage::AddHandler( new wxJPEGHandler );
    wxFileSystem::AddHandler( new wxZipFSHandler );

    // Analyse the command line & init binary path
    SetBinDir();
    ReadPdfBrowserInfos();

    // Internationalisation: loading the kicad suitable Dictionnary
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
 * @return  none
 */
/*****************************************/
void WinEDA_App::InitOnLineHelp()
/*****************************************/
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
 * @return TODO
 */
/*******************************/
bool WinEDA_App::SetBinDir()
/*******************************/
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
#elif defined (__UNIX__)

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

    m_BinDir.Replace( WIN_STRING_DIR_SEP, UNIX_STRING_DIR_SEP );
    while( m_BinDir.Last() != '/' )
        m_BinDir.RemoveLast();

    return TRUE;
}


/**
 * Get application settings
 * @return  none
 */
/*********************************/
void WinEDA_App::GetSettings()
/*********************************/
{
    wxString Line, Ident;
    unsigned ii;

    m_HelpSize.x = 500;
    m_HelpSize.y = 400;

    if( m_EDA_CommonConfig )
    {
        m_LanguageId = m_EDA_CommonConfig->Read( wxT( "Language" ),
                                                 wxLANGUAGE_DEFAULT );
        g_EditorName = m_EDA_CommonConfig->Read( wxT( "Editor" ) );
        g_ConfigFileLocationChoice = m_EDA_CommonConfig->Read( HOTKEY_CFG_PATH_OPT,
                                                               0L );
    }

    if( !m_EDA_Config )
        return;

    /* Last 10 project settings */
    for( ii = 0; ii < 10; ii++ )
    {
        Ident = wxT( "LastProject" );

        if( ii )
            Ident << ii;

        if( m_EDA_Config->Read( Ident, &Line ) )
            m_LastProject.Add( Line );
    }

    /* Set default font sizes */
    g_StdFontPointSize = m_EDA_Config->Read( wxT( "SdtFontSize" ),
                                             FONT_DEFAULT_SIZE );
    g_MsgFontPointSize = m_EDA_Config->Read( wxT( "MsgFontSize" ),
                                             FONT_DEFAULT_SIZE );
    g_DialogFontPointSize = m_EDA_Config->Read( wxT( "DialogFontSize" ),
                                                FONT_DEFAULT_SIZE );
    g_FixedFontPointSize = m_EDA_Config->Read( wxT( "FixedFontSize" ),
                                               FONT_DEFAULT_SIZE );

    /* Sdt font type */
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

    if( m_EDA_Config->Read( wxT( "WorkingDir" ), &Line ) )
    {
        if( wxDirExists( Line ) )
            wxSetWorkingDirectory( Line );
    }
    m_EDA_Config->Read( wxT( "BgColor" ), &g_DrawBgColor );
}


/**
 * Save application settings
 * @return  none
 */
/**********************************/
void WinEDA_App::SaveSettings()
/**********************************/
{
    unsigned int i;

    if( m_EDA_Config == NULL )
        return;

    /* Sdt font settings */
    m_EDA_Config->Write( wxT( "SdtFontSize" ), g_StdFontPointSize );
    m_EDA_Config->Write( wxT( "SdtFontType" ), g_StdFont->GetFaceName() );
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

    /* Save last 10 project list */
    for( i = 0; i < 10; i++ )
    {
        wxString msg = wxT( "LastProject" );
        if( i )
            msg << i;

        if( i < m_LastProject.GetCount() )
            m_EDA_Config->Write( msg, m_LastProject[i] );
        else
            m_EDA_Config->Write( msg, wxEmptyString );
    }
}


/**
 * Set the dictionary file name for internationalization
 *  the files are in kicad/internat/xx or kicad/internat/xx_XX
 *  and are named kicad.mo
 *
 * @param   first_time  TODO
 * @return  TODO
 */
/*********************************************/
bool WinEDA_App::SetLanguage( bool first_time )
/*********************************************/
{
    // dictionary file name without extend (full name is kicad.mo)
    wxString DictionaryName( wxT( "kicad" ) );

    // Real path is kicad/internat/xx_XX or kicad/internat/xx
    wxString BaseDictionaryPath( wxT( "internat" ) );
    wxString dic_path;
    bool     retv = true;

    if( m_Locale != NULL )
        delete m_Locale;
    m_Locale = new wxLocale();

    dic_path = ReturnKicadDatasPath() + BaseDictionaryPath;

    wxLogDebug( wxT( "Adding prefix <" ) + dic_path +
                wxT( "> to language lookup path." ) );

    m_Locale->AddCatalogLookupPathPrefix( dic_path );

    /*
     * Add binary path minus the current subdirectory ( typically /bin ) to
     * the locale search path.  This way the locales can be found when using
     * custom CMake install paths.
     *
     * FIXME:  This should be changed when configurable data path support is
     *         added to Kicad.
     */
    if( !m_BinDir.IsEmpty() )
    {
        wxFileName fn( m_BinDir, wxEmptyString );
        dic_path = fn.GetPath();
        int        n = dic_path.Find( wxFileName::GetPathSeparator(), true );

        if( n != wxNOT_FOUND )
        {
            dic_path = dic_path( 0, n );
        }

        wxLogDebug( wxT( "Adding prefix <" ) + dic_path +
                    wxT( "> to language lookup path." ) );
        m_Locale->AddCatalogLookupPathPrefix( dic_path );
    }

    if( !m_Locale->Init( m_LanguageId, wxLOCALE_CONV_ENCODING ) )
    {
        wxLogDebug( wxT( "Failed to initialize " ) + m_Locale->GetName() );

        delete m_Locale;
        m_Locale     = new wxLocale();
        m_LanguageId = wxLANGUAGE_DEFAULT;
        m_Locale->Init();
        retv = false;
    }

    if( !first_time )
    {
        if( m_EDA_CommonConfig )
            m_EDA_CommonConfig->Write( wxT( "Language" ), m_LanguageId );
    }

    if( !m_Locale->IsLoaded( DictionaryName ) )
        m_Locale->AddCatalog( DictionaryName );

    if( !retv )
        return retv;

    return m_Locale->IsOk();
}


/**
 * Return in m_LanguageId the wxWidgets language identifier Id
 *   from the kicad menu id (internal menu identifier)
 * @param   menu_id TODO
 * @return  none
 */
/**************************************************/
void WinEDA_App::SetLanguageIdentifier( int menu_id )
/**************************************************/
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


/**
 * Create menu list for language choice.
 * @param   MasterMenu  TODO
 * @return  TODO
 */
/*********************************************************/
wxMenu* WinEDA_App::SetLanguageList( wxMenu* MasterMenu )
/*********************************************************/
{
    wxMenuItem*  item;
    unsigned int ii;

    if( m_Language_Menu == NULL )
    {
        m_Language_Menu = new wxMenu;
        for( ii = 0; ii < LANGUAGE_DESCR_COUNT; ii++ )
        {
            wxString MenuLabel = s_Language_List[ii].m_DoNotTranslate ?
                                 s_Language_List[ii].m_Lang_Label :
                                 wxGetTranslation(
                s_Language_List[ii].m_Lang_Label );

            item = new wxMenuItem( m_Language_Menu,
                                   s_Language_List[ii].m_KI_Lang_Identifier,
                                   MenuLabel,
                                   wxEmptyString,
                                   wxITEM_CHECK );

            SETBITMAPS( s_Language_List[ii].m_Lang_Icon );
            m_Language_Menu->Append( item );
        }
    }

    for( ii = 0; ii < LANGUAGE_DESCR_COUNT; ii++ )
    {
        if( m_LanguageId == s_Language_List[ii].m_WX_Lang_Identifier )
            m_Language_Menu->Check( s_Language_List[ii].m_KI_Lang_Identifier,
                                    true );
        else
            m_Language_Menu->Check( s_Language_List[ii].m_KI_Lang_Identifier,
                                    false );
    }

    if( MasterMenu )
    {
        ADD_MENUITEM_WITH_HELP_AND_SUBMENU( MasterMenu,
                                            m_Language_Menu,
                                            ID_LANGUAGE_CHOICE,
                                            _( "Language" ),
                                            _( "Select application language (only for testing!)" ),
                                            language_xpm );
    }

    return m_Language_Menu;
}


/**
 * Run init scripts
 * @return  TODO
 */
/**********************/
int WinEDA_App::OnRun()
/**********************/
{
#ifdef KICAD_PYTHON
    PyHandler::GetInstance()->RunScripts();
#endif
    return wxApp::OnRun();
}
