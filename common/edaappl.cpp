/***************/
/* edaappl.cpp */
/***************/

/*
 *   ROLE: methodes relative a la classe winEDA_App, communes
 *   aux environements window et linux
 */
#define EDA_BASE
#define COMMON_GLOBL

#ifdef KICAD_PYTHON
#include <pyhandler.h>
#endif

#include "fctsys.h"
#include <wx/image.h>
#include "wx/html/htmlwin.h"
#include "wx/fs_zip.h"


#include "wxstruct.h"

#include "gr_basic.h"
#include "common.h"
#include "worksheet.h"
#include "id.h"
#include "build_version.h"
#include "hotkeys_basic.h"
#include "macros.h"
#include "online_help.h"

#include "bitmaps.h"


#define FONT_DEFAULT_SIZE 10    /* Default font size.
                                 *  The real font size will be computed at run time */

/* A small class to handle the list od existing translations.
 * the locale translation is automatic.
 * the selection of languages is mainly for mainteners's convenience (tests...)
 * To add a support to a new tranlation:
 * create a new icon (flag of the country) (see Lang_Fr.xpm as an exemple)
 *  add a new item to s_Language_List[LANGUAGE_DESCR_COUNT]
 * and set LANGUAGE_DESCR_COUNT to the new value
 */
struct LANGUAGE_DESCR
{
    int           m_WX_Lang_Identifier;                 // wxWidget locale identifier (see wxWidget doc)
    int           m_KI_Lang_Identifier;                 // kicad identifier used in menu selection (see id.h)
    const char**  m_Lang_Icon;                          // the icon used in menus
    const wxChar* m_Lang_Label;                         // Label used in menus
    bool          m_DoNotTranslate;                     // set to true if the m_Lang_Label must not be translated
};

#define LANGUAGE_DESCR_COUNT 15
static struct LANGUAGE_DESCR s_Language_List[LANGUAGE_DESCR_COUNT] =
{
    {
        wxLANGUAGE_DEFAULT,
        ID_LANGUAGE_DEFAULT,
        lang_def_xpm,
        _( "Default" )
    },
    {
        wxLANGUAGE_ENGLISH,
        ID_LANGUAGE_ENGLISH,
        lang_en_xpm,
        wxT( "English" ),
        true
    },
    {
        wxLANGUAGE_FRENCH,
        ID_LANGUAGE_FRENCH,
        lang_fr_xpm,
        _( "French" )
    },
    {
        wxLANGUAGE_SPANISH,
        ID_LANGUAGE_SPANISH,
        lang_es_xpm,
        _( "Spanish" )
    },
    {
        wxLANGUAGE_PORTUGUESE,
        ID_LANGUAGE_PORTUGUESE,
        lang_pt_xpm,
        _( "Portuguese" )
    },
    {
        wxLANGUAGE_ITALIAN,
        ID_LANGUAGE_ITALIAN,
        lang_it_xpm,
        _( "Italian" )
    },
    {
        wxLANGUAGE_GERMAN,
        ID_LANGUAGE_GERMAN,
        lang_de_xpm,
        _( "German" )
    },
    {
        wxLANGUAGE_SLOVENIAN,
        ID_LANGUAGE_SLOVENIAN,
        lang_sl_xpm,
        _( "Slovenian" )
    },
    {
        wxLANGUAGE_HUNGARIAN,
        ID_LANGUAGE_HUNGARIAN,
        lang_hu_xpm,
        _( "Hungarian" )
    },
    {
        wxLANGUAGE_POLISH,
        ID_LANGUAGE_POLISH,
        lang_pl_xpm,
        _( "Polish" )
    },
    {
        wxLANGUAGE_CZECH,
        ID_LANGUAGE_CZECH,
        lang_cs_xpm,
        _( "Czech" )
    },
    {
        wxLANGUAGE_RUSSIAN,
        ID_LANGUAGE_RUSSIAN,
        lang_ru_xpm,
        _( "Russian" )
    },
    {
        wxLANGUAGE_KOREAN,
        ID_LANGUAGE_KOREAN,
        lang_ko_xpm,
        _( "Korean" )
    },
    {
        wxLANGUAGE_CHINESE_SIMPLIFIED,
        ID_LANGUAGE_CHINESE_SIMPLIFIED,
        lang_chinese_xpm,
        _( "Chinese simplified" )
    },
    {
        wxLANGUAGE_CATALAN,
        ID_LANGUAGE_CATALAN,
        lang_catalan_xpm,
        _( "Catalan" )
    },
    {
        wxLANGUAGE_DUTCH,
        ID_LANGUAGE_DUTCH,
        lang_nl_xpm,
        _( "Dutch" )
    }
};

/**************************/
/* WinEDA_App Constructor */
/**************************/

WinEDA_App::WinEDA_App()
{
    m_Checker   = NULL;
    m_MainFrame = NULL;
    m_PcbFrame  = NULL;
    m_ModuleEditFrame = NULL;       // Frame for footprint edition
    m_SchematicFrame  = NULL;       // Frame for schematic edition
    m_LibeditFrame    = NULL;       // Frame for component edition
    m_ViewlibFrame    = NULL;       // Frame for browsing component libraries
    m_CvpcbFrame  = NULL;
    m_GerberFrame = NULL;           // Frame for the gerber viewer GERBVIEW

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


/*************************/
/* WinEDA_App Destructor */
/*************************/

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


/**************************************************/
void WinEDA_App::InitEDA_Appl( const wxString& name )
/***************************************************/
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

    /* Prepare On Line Help */
#if defined ONLINE_HELP_FILES_FORMAT_IS_HTML
    m_HelpFileName = name + wxT( ".html" );
#elif defined ONLINE_HELP_FILES_FORMAT_IS_PDF
    m_HelpFileName = name + wxT( ".pdf" );
#else
    #error Help files format not defined
#endif

    // Init parameters for configuration
    SetVendorName( wxT( "kicad" ) );
    SetAppName( name );
    m_EDA_Config = new wxConfig( name );
    m_EDA_CommonConfig = new wxConfig( wxT( "kicad_common" ) );

    /* Create the fontes used in dialogs and messages */
    g_StdFontPointSize    = FONT_DEFAULT_SIZE;
    g_MsgFontPointSize    = FONT_DEFAULT_SIZE;
    g_DialogFontPointSize = FONT_DEFAULT_SIZE;
    g_FixedFontPointSize  = FONT_DEFAULT_SIZE;
    g_StdFont    = new wxFont( g_StdFontPointSize, wxFONTFAMILY_ROMAN, wxNORMAL, wxNORMAL );
    g_MsgFont    = new wxFont( g_StdFontPointSize, wxFONTFAMILY_ROMAN, wxNORMAL, wxNORMAL );
    g_DialogFont = new wxFont( g_DialogFontPointSize, wxFONTFAMILY_ROMAN, wxNORMAL, wxNORMAL );
    g_ItalicFont = new wxFont( g_DialogFontPointSize,
        wxFONTFAMILY_ROMAN,
        wxFONTSTYLE_ITALIC,
        wxNORMAL );
    g_FixedFont = new wxFont( g_FixedFontPointSize, wxFONTFAMILY_MODERN, wxNORMAL, wxNORMAL );

    /* installation des gestionnaires de visu d'images (pour help) */
    wxImage::AddHandler( new wxPNGHandler );
    wxImage::AddHandler( new wxGIFHandler );
    wxImage::AddHandler( new wxJPEGHandler );
    wxFileSystem::AddHandler( new wxZipFSHandler );

    // Analyse the command line & init binary path
    SetBinDir();

    ReadPdfBrowserInfos();

    // Internationalisation: loading the kicad suitable Dictionnary
    m_EDA_CommonConfig->Read( wxT( "Language" ), &m_LanguageId, wxLANGUAGE_DEFAULT );

    bool succes = SetLanguage( TRUE );
    if( !succes )
    {
    }

    SetLocaleTo_Default( );     // Set locale option for separator used in float numbers

#ifdef KICAD_PYTHON
    PyHandler::GetInstance()->SetAppName( name );
#endif
}


/*****************************************/
void WinEDA_App::InitOnLineHelp()
/*****************************************/

/* Init On Line Help
 */
{
    wxString fullfilename = FindKicadHelpPath();

#if defined ONLINE_HELP_FILES_FORMAT_IS_HTML
    m_HelpFileName = fullfilename + wxT( ".html" );
    fullfilename += wxT( "kicad.hhp" );
    if( wxFileExists( fullfilename ) )
    {
        m_HtmlCtrl = new wxHtmlHelpController( wxHF_TOOLBAR |
            wxHF_CONTENTS | wxHF_PRINT | wxHF_OPEN_FILES
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


/*******************************/
bool WinEDA_App::SetBinDir()
/*******************************/

// Find the path to the executable and store it in WinEDA_App::m_BinDir
{
#ifdef __APPLE__

    // Derive path from location of the app bundle
    CFBundleRef mainBundle = CFBundleGetMainBundle();
    if( mainBundle == NULL )
        return false;
    CFURLRef    urlref = CFBundleCopyBundleURL( mainBundle );
    if( urlref == NULL )
        return false;
    CFStringRef str = CFURLCopyFileSystemPath( urlref, kCFURLPOSIXPathStyle );
    if( str == NULL )
        return false;
    char*       native_str = NULL;
    int         len = CFStringGetMaximumSizeForEncoding( CFStringGetLength( str ),
        kCFStringEncodingUTF8 ) + 1;
    native_str = new char[len];
    CFStringGetCString( str, native_str, len, kCFStringEncodingUTF8 );
    m_BinDir = CONV_FROM_UTF8( native_str );
    delete[] native_str;

#elif defined (__UNIX__)

    // Under Linux, if argv[0] doesn't the complete path to the executable,
    // it's necessary to obtain it using "which <filename>".
    FILE*    ftmp;

#define TMP_FILE "/tmp/kicad.tmp"
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
#endif

    m_BinDir.Replace( WIN_STRING_DIR_SEP, UNIX_STRING_DIR_SEP );
    while( m_BinDir.Last() != '/' )
        m_BinDir.RemoveLast();

    return TRUE;
}


/*********************************/
void WinEDA_App::GetSettings()
/*********************************/

/* Get the last setup used (fontes, files opened...)
 */
{
    wxString Line, Ident;
    unsigned ii;

    m_HelpSize.x = 500;
    m_HelpSize.y = 400;

    if( m_EDA_CommonConfig )
    {
        m_LanguageId = m_EDA_CommonConfig->Read( wxT( "Language" ), wxLANGUAGE_DEFAULT );
        g_EditorName = m_EDA_CommonConfig->Read( wxT( "Editor" ) );
        g_ConfigFileLocationChoice = m_EDA_CommonConfig->Read( HOTKEY_CFG_PATH_OPT, 0L );
    }

    if( !m_EDA_Config )
        return;


    for( ii = 0; ii < 10; ii++ )
    {
        Ident = wxT( "LastProject" );

        if( ii )
            Ident << ii;

        if( m_EDA_Config->Read( Ident, &Line ) )
            m_LastProject.Add( Line );
    }

    g_StdFontPointSize    = m_EDA_Config->Read( wxT( "SdtFontSize" ), FONT_DEFAULT_SIZE );
    g_MsgFontPointSize    = m_EDA_Config->Read( wxT( "MsgFontSize" ), FONT_DEFAULT_SIZE );
    g_DialogFontPointSize = m_EDA_Config->Read( wxT( "DialogFontSize" ), FONT_DEFAULT_SIZE );
    g_FixedFontPointSize  = m_EDA_Config->Read( wxT( "FixedFontSize" ), FONT_DEFAULT_SIZE );

    Line = m_EDA_Config->Read( wxT( "SdtFontType" ), wxEmptyString );
    if( !Line.IsEmpty() )
        g_StdFont->SetFaceName( Line );

    ii = m_EDA_Config->Read( wxT( "SdtFontStyle" ), wxFONTFAMILY_ROMAN );
    g_StdFont->SetStyle( ii );
    ii = m_EDA_Config->Read( wxT( "SdtFontWeight" ), wxNORMAL );
    g_StdFont->SetWeight( ii );
    g_StdFont->SetPointSize( g_StdFontPointSize );


    Line = m_EDA_Config->Read( wxT( "MsgFontType" ), wxEmptyString );
    if( !Line.IsEmpty() )
        g_MsgFont->SetFaceName( Line );

    ii = m_EDA_Config->Read( wxT( "MsgFontStyle" ), wxFONTFAMILY_ROMAN );
    g_MsgFont->SetStyle( ii );
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


/**********************************/
void WinEDA_App::SaveSettings()
/**********************************/
{
    unsigned int ii;

    if( m_EDA_Config == NULL )
        return;

    m_EDA_Config->Write( wxT( "SdtFontSize" ), g_StdFontPointSize );
    m_EDA_Config->Write( wxT( "SdtFontType" ), g_StdFont->GetFaceName() );
    m_EDA_Config->Write( wxT( "SdtFontStyle" ), g_StdFont->GetStyle() );
    m_EDA_Config->Write( wxT( "SdtFontWeight" ), g_StdFont->GetWeight() );

    m_EDA_Config->Write( wxT( "MsgFontSize" ), g_MsgFontPointSize );
    m_EDA_Config->Write( wxT( "MsgFontType" ), g_MsgFont->GetFaceName() );
    m_EDA_Config->Write( wxT( "MsgFontStyle" ), g_MsgFont->GetStyle() );
    m_EDA_Config->Write( wxT( "MsgFontWeight" ), g_MsgFont->GetWeight() );

    m_EDA_Config->Write( wxT( "DialogFontSize" ), g_DialogFontPointSize );
    m_EDA_Config->Write( wxT( "DialogFontType" ), g_DialogFont->GetFaceName() );
    m_EDA_Config->Write( wxT( "DialogFontStyle" ), g_DialogFont->GetStyle() );
    m_EDA_Config->Write( wxT( "DialogFontWeight" ), g_DialogFont->GetWeight() );

    m_EDA_Config->Write( wxT( "FixedFontSize" ), g_FixedFontPointSize );

    m_EDA_Config->Write( wxT( "ShowPageLimits" ), g_ShowPageLimits );

    m_EDA_Config->Write( wxT( "WorkingDir" ), wxGetCwd() );

    for( ii = 0; ii < 10; ii++ )
    {
        wxString msg = wxT( "LastProject" );
        if( ii )
            msg << ii;

        if( ii < m_LastProject.GetCount() )
            m_EDA_Config->Write( msg, m_LastProject[ii] );
        else
            m_EDA_Config->Write( msg, wxEmptyString );
    }
}


/*********************************************/
bool WinEDA_App::SetLanguage( bool first_time )
/*********************************************/

/* Set the dictionary file name for internationalization
 *  the files are in kicad/internat/xx or kicad/internat/xx_XX
 *  and are named kicad.mo
 */
{
    wxString DictionaryName( wxT( "kicad" ) );          // dictionary file name without extend (full name is kicad.mo)
    wxString BaseDictionaryPath( wxT( "internat" ) );   // Real path is kicad/internat/xx_XX or kicad/internat/xx
    wxString dic_path;

    if( m_Locale != NULL )
        delete m_Locale;
    m_Locale = new wxLocale();
    m_Locale->Init( m_LanguageId );
    dic_path = ReturnKicadDatasPath() + BaseDictionaryPath;
    m_Locale->AddCatalogLookupPathPrefix( dic_path );

    if( !first_time )
    {
        if( m_EDA_CommonConfig )
            m_EDA_CommonConfig->Write( wxT( "Language" ), m_LanguageId );
    }

    if( !m_Locale->IsLoaded( DictionaryName ) )
        m_Locale->AddCatalog( DictionaryName );
    SetLanguageList( NULL );


    return m_Locale->IsOk();
}


/**************************************************/
void WinEDA_App::SetLanguageIdentifier( int menu_id )
/**************************************************/

/* return in m_LanguageId the wxWidgets language identifier Id
 *  from the kicad menu id (internal menu identifier)
 */
{
    unsigned int ii;

    for( ii = 0; ii < LANGUAGE_DESCR_COUNT; ii++ )
    {
        if( menu_id == s_Language_List[ii].m_KI_Lang_Identifier )
        {
            m_LanguageId = s_Language_List[ii].m_WX_Lang_Identifier;
            break;
        }
    }
}


/*********************************************************/
wxMenu* WinEDA_App::SetLanguageList( wxMenu* MasterMenu )
/*********************************************************/

/* Create menu list for language choice.
 */
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
                                 wxGetTranslation( s_Language_List[ii].m_Lang_Label );
            item = new wxMenuItem( m_Language_Menu, s_Language_List[ii].m_KI_Lang_Identifier,
                MenuLabel, wxEmptyString, wxITEM_CHECK );
            SETBITMAPS( s_Language_List[ii].m_Lang_Icon );
            m_Language_Menu->Append( item );
        }
    }

    for( ii = 0; ii < LANGUAGE_DESCR_COUNT; ii++ )
    {
        if( m_LanguageId == s_Language_List[ii].m_WX_Lang_Identifier )
            m_Language_Menu->Check( s_Language_List[ii].m_KI_Lang_Identifier, true );
        else
            m_Language_Menu->Check( s_Language_List[ii].m_KI_Lang_Identifier, false );
    }

    if( MasterMenu )
    {
        ADD_MENUITEM_WITH_HELP_AND_SUBMENU( MasterMenu, m_Language_Menu,
            ID_LANGUAGE_CHOICE, _( "Language" ),
            wxT( "For test only, use Default setup for normal use" ),
            language_xpm );
    }
    return m_Language_Menu;
}


/**********************/
int WinEDA_App::OnRun()
/**********************/

/* Run init scripts
 */
{
    #ifdef KICAD_PYTHON
    PyHandler::GetInstance()->RunScripts();
    #endif
    return wxApp::OnRun();
}
