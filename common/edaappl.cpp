	/***************/
	/* edaappl.cpp */
	/***************/
/*
	 ROLE: methodes relative a la classe winEDA_App, communes
	 aux environements window et linux
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
#include "macros.h"

#include "gr_basic.h"
#include "common.h"
#include "worksheet.h"
#include "id.h"
#include "build_version.h"

#include "bitmaps.h"
#include "Language.xpm"

#ifdef __WINDOWS__
/* Icons for language choice (only for Windows)*/
#include "Lang_Default.xpm"
#include "Lang_En.xpm"
#include "Lang_Es.xpm"
#include "Lang_Fr.xpm"
#include "Lang_Pt.xpm"
#include "Lang_It.xpm"
#include "Lang_De.xpm"
#include "Lang_Sl.xpm"
#include "Lang_Hu.xpm"
#include "Lang_Po.xpm"
#include "Lang_Ko.xpm"
#include "Lang_Ru.xpm"
#endif


#define FONT_DEFAULT_SIZE 10	/* Default font size.
				The real font size will be computed at run time */


	/*****************************/
	/* Constructeur de WinEDA_App */
	/*****************************/

WinEDA_App::WinEDA_App(void)
{
	m_Checker = NULL;
	m_MainFrame = NULL;
	m_PcbFrame = NULL;
	m_ModuleEditFrame = NULL;	// Edition des modules
	SchematicFrame = NULL;		// Edition des Schemas
	LibeditFrame = NULL;		// Edition des composants
	ViewlibFrame = NULL;		// Visualisation des composants
	m_CvpcbFrame = NULL;
	m_GerberFrame = NULL;		// ecran de visualisation GERBER

	m_LastProjectMaxCount = 10;
	m_HtmlCtrl = NULL;
	m_EDA_CommonConfig = NULL;
	m_EDA_Config = NULL;
	m_Env_Defined = FALSE;
	m_LanguageId = wxLANGUAGE_DEFAULT;
	m_Language_Menu = NULL;
	m_Locale = NULL;

	m_PdfBrowserIsDefault = TRUE;

	/* Init de variables globales d'interet general: */
	g_FloatSeparator = '.';		// Nombres flottants = 0.1 par exemple
}

	/*****************************/
	/* Destructeur de WinEDA_App */
	/*****************************/

WinEDA_App::~WinEDA_App(void)
{
	SaveSettings();

	/* delete data non directement geree par wxAppl */
	delete g_Prj_Config;
	delete m_EDA_Config;
	delete m_EDA_CommonConfig;
	delete g_StdFont;
	delete g_DialogFont;
	delete g_ItalicFont;
	delete g_FixedFont;
	delete g_MsgFont;
	if ( m_Checker ) delete m_Checker;
	delete m_Locale;
}


/**************************************************/
void WinEDA_App::InitEDA_Appl(const wxString & name)
/***************************************************/
{
wxString ident;
wxString EnvLang;

	ident = name + wxT("-") + wxGetUserId();
	m_Checker = new wxSingleInstanceChecker(ident);

	/* Init environnement
	(KICAD definit le chemin de kicad ex: set KICAD=d:\kicad) */
	m_Env_Defined = wxGetEnv( wxT("KICAD"), &m_KicadEnv);
	if ( m_Env_Defined )	// m_KicadEnv doit finir par "/" ou "\"
	{
		m_KicadEnv.Replace(WIN_STRING_DIR_SEP, UNIX_STRING_DIR_SEP);
		if ( m_KicadEnv.Last() != '/' ) m_KicadEnv += UNIX_STRING_DIR_SEP;
	}

	/* Prepare On Line Help */
	m_HelpFileName = name + wxT(".html");

	// Init parametres pour configuration
	SetVendorName(wxT("kicad"));
	SetAppName(name);
	m_EDA_Config = new wxConfig(name);
	m_EDA_CommonConfig = new wxConfig(wxT("kicad_common"));
	
	/* Creation des fontes utiles */
	g_StdFontPointSize = FONT_DEFAULT_SIZE;
	g_MsgFontPointSize = FONT_DEFAULT_SIZE;
	g_DialogFontPointSize = FONT_DEFAULT_SIZE;
	g_FixedFontPointSize = FONT_DEFAULT_SIZE;
	g_StdFont = new wxFont(g_StdFontPointSize, wxFONTFAMILY_ROMAN, wxNORMAL, wxNORMAL);
	g_MsgFont = new wxFont(g_StdFontPointSize, wxFONTFAMILY_ROMAN, wxNORMAL, wxNORMAL);
	g_DialogFont = new wxFont(g_DialogFontPointSize, wxFONTFAMILY_ROMAN, wxNORMAL, wxNORMAL);
	g_ItalicFont = new wxFont(g_DialogFontPointSize, wxFONTFAMILY_ROMAN, wxFONTSTYLE_ITALIC,  wxNORMAL);
	g_FixedFont = new wxFont(g_FixedFontPointSize, wxFONTFAMILY_MODERN, wxNORMAL, wxNORMAL);

	/* installation des gestionnaires de visu d'images (pour help) */
	wxImage::AddHandler(new wxPNGHandler);
	wxImage::AddHandler(new wxGIFHandler);
	wxImage::AddHandler(new wxJPEGHandler);
    wxFileSystem::AddHandler(new wxZipFSHandler);

	// Analyse command line & init binary path
	SetBinDir();

	ReadPdfBrowserInfos();
	// Internationalisation: chargement du Dictionnaire de kicad
	m_EDA_CommonConfig->Read(wxT("Language"), &m_LanguageId, wxLANGUAGE_DEFAULT);

bool succes = SetLanguage(TRUE);
	if ( ! succes )
		{
		}

	if ( atof("0,1") ) g_FloatSeparator = ','; // Nombres flottants = 0,1
	else  g_FloatSeparator = '.';

#ifdef KICAD_PYTHON
	PyHandler::GetInstance()->SetAppName( name );
#endif
}


/*****************************************/
void WinEDA_App::InitOnLineHelp(void)
/*****************************************/
/* Init On Line Help
*/
{
wxString fullfilename = FindKicadHelpPath();

	fullfilename += wxT("kicad.hhp");
	if ( wxFileExists(fullfilename) )
	{
		m_HtmlCtrl = new wxHtmlHelpController(wxHF_TOOLBAR |
				wxHF_CONTENTS | wxHF_PRINT | wxHF_OPEN_FILES
				/*| wxHF_SEARCH */);
		m_HtmlCtrl->UseConfig(m_EDA_CommonConfig);
		m_HtmlCtrl->SetTitleFormat( wxT("Kicad Help") );
		m_HtmlCtrl->AddBook(fullfilename);
	}
}


/*******************************/
bool WinEDA_App::SetBinDir(void)
/*******************************/
/*
 Analyse la ligne de commande pour retrouver le chemin de l'executable
 Sauve en WinEDA_App::m_BinDir le repertoire de l'executable
*/

{
	/* Calcul du chemin ou se trouve l'executable */

#ifdef __UNIX__
	/* Sous LINUX ptarg[0] ne donne pas le chemin complet de l'executable,
	il faut le retrouver par la commande "which <filename> si aucun
	chemin n'est donne */
FILE * ftmp;

#define TMP_FILE "/tmp/kicad.tmp"
char Line[1024];
char FileName[1024];
wxString str_arg0;
int ii;

	FileName[0] = 0;
	str_arg0 = argv[0];
	if( strchr( (const char *)argv[0],'/') == NULL ) /* pas de chemin */
	{
		sprintf( FileName, "which %s > %s", CONV_TO_UTF8(str_arg0), TMP_FILE);
		ii = system(FileName);
		if( (ftmp = fopen(TMP_FILE, "rt")) != NULL )
		{
			fgets(Line,1000,ftmp);
			fclose(ftmp);
			remove(TMP_FILE);
		}
	m_BinDir = CONV_FROM_UTF8(Line);
	}
	else  m_BinDir = argv[0];
#else
	m_BinDir = argv[0];
#endif

	m_BinDir.Replace(WIN_STRING_DIR_SEP, UNIX_STRING_DIR_SEP);
	while ( m_BinDir.Last() != '/' ) m_BinDir.RemoveLast();

	return TRUE;
}


/*********************************/
void WinEDA_App::GetSettings(void)
/*********************************/
/* Lit les infos utiles sauvees lors de la derniere utilisation du logiciel
*/
{
wxString Line, Ident;
unsigned ii;

	m_HelpSize.x = 500; m_HelpSize.y = 400;

	if ( m_EDA_CommonConfig )
	{
		m_LanguageId = m_EDA_CommonConfig->Read(wxT("Language"), wxLANGUAGE_DEFAULT);
		g_EditorName = m_EDA_CommonConfig->Read(wxT("Editor"));
	}

	if ( ! m_EDA_Config ) return;


	for ( ii = 0; ii < 10; ii++ )
	{
	Ident = wxT("LastProject"); if ( ii ) Ident << ii;
	if( m_EDA_Config->Read(Ident, &Line) )
		m_LastProject.Add(Line);
	}

	g_StdFontPointSize = m_EDA_Config->Read(wxT("SdtFontSize"), FONT_DEFAULT_SIZE);
	g_MsgFontPointSize = m_EDA_Config->Read(wxT("MsgFontSize"), FONT_DEFAULT_SIZE);
	g_DialogFontPointSize = m_EDA_Config->Read(wxT("DialogFontSize"), FONT_DEFAULT_SIZE);
	g_FixedFontPointSize = m_EDA_Config->Read(wxT("FixedFontSize"), FONT_DEFAULT_SIZE);

	Line = m_EDA_Config->Read(wxT("SdtFontType"), wxEmptyString);
	if ( ! Line.IsEmpty() ) g_StdFont->SetFaceName(Line);
	ii = m_EDA_Config->Read(wxT("SdtFontStyle"), wxFONTFAMILY_ROMAN);
	g_StdFont->SetStyle(ii);
	ii = m_EDA_Config->Read(wxT("SdtFontWeight"), wxNORMAL);
	g_StdFont->SetWeight(ii);
	g_StdFont->SetPointSize(g_StdFontPointSize);
	

	Line = m_EDA_Config->Read(wxT("MsgFontType"), wxEmptyString);
	if ( ! Line.IsEmpty() ) g_MsgFont->SetFaceName(Line);
	ii = m_EDA_Config->Read(wxT("MsgFontStyle"), wxFONTFAMILY_ROMAN);
	g_MsgFont->SetStyle(ii);
	ii = m_EDA_Config->Read(wxT("MsgFontWeight"), wxNORMAL);
	g_MsgFont->SetWeight(ii);
	g_MsgFont->SetPointSize(g_MsgFontPointSize);

	Line = m_EDA_Config->Read(wxT("DialogFontType"), wxEmptyString);
	if ( ! Line.IsEmpty() ) g_DialogFont->SetFaceName(Line);
	ii = m_EDA_Config->Read(wxT("DialogFontStyle"), wxFONTFAMILY_ROMAN);
	g_DialogFont->SetStyle(ii);
	ii = m_EDA_Config->Read(wxT("DialogFontWeight"), wxNORMAL);
	g_DialogFont->SetWeight(ii);
	g_DialogFont->SetPointSize(g_DialogFontPointSize);

	g_FixedFont->SetPointSize(g_FixedFontPointSize);

	m_EDA_Config->Read(wxT("ShowPageLimits"), & g_ShowPageLimits);

	if( m_EDA_Config->Read(wxT("WorkingDir"), &Line) )
	{
		if ( wxDirExists(Line) ) wxSetWorkingDirectory(Line);
	}
	m_EDA_Config->Read( wxT("BgColor"), &g_DrawBgColor);
}


/**********************************/
void WinEDA_App::SaveSettings(void)
/**********************************/
{
unsigned int ii;


	if( m_EDA_Config == NULL ) return;

	m_EDA_Config->Write(wxT("SdtFontSize"), g_StdFontPointSize);
	m_EDA_Config->Write(wxT("SdtFontType"), g_StdFont->GetFaceName());
	m_EDA_Config->Write(wxT("SdtFontStyle"), g_StdFont->GetStyle());
	m_EDA_Config->Write(wxT("SdtFontWeight"), g_StdFont->GetWeight());

	m_EDA_Config->Write(wxT("MsgFontSize"), g_MsgFontPointSize);
	m_EDA_Config->Write(wxT("MsgFontType"), g_MsgFont->GetFaceName());
	m_EDA_Config->Write(wxT("MsgFontStyle"), g_MsgFont->GetStyle());
	m_EDA_Config->Write(wxT("MsgFontWeight"), g_MsgFont->GetWeight());

	m_EDA_Config->Write(wxT("DialogFontSize"), g_DialogFontPointSize);
	m_EDA_Config->Write(wxT("DialogFontType"), g_DialogFont->GetFaceName());
	m_EDA_Config->Write(wxT("DialogFontStyle"), g_DialogFont->GetStyle());
	m_EDA_Config->Write(wxT("DialogFontWeight"), g_DialogFont->GetWeight());

	m_EDA_Config->Write(wxT("FixedFontSize"), g_FixedFontPointSize);

	m_EDA_Config->Write(wxT("ShowPageLimits"), g_ShowPageLimits);
	
	m_EDA_Config->Write(wxT("WorkingDir"), wxGetCwd());

	for( ii = 0; ii < 10; ii++ )
	{
		wxString msg = wxT("LastProject");
		if ( ii ) msg << ii;

		if ( ii < m_LastProject.GetCount() )
			m_EDA_Config->Write(msg, m_LastProject[ii]);
		else
			m_EDA_Config->Write(msg, wxEmptyString);
	}
}

/*********************************************/
bool WinEDA_App::SetLanguage(bool first_time)
/*********************************************/
/* Set the dictionary file name for internationalization
	the files are in kicad/internat/xx or kicad/internat/xx_XX
	and are named kicad.mo
*/
{
wxString DictionaryName( wxT("kicad"));	// dictionary file name without extend (full name is kicad.mo)
wxString BaseDictionaryPath( wxT("internat"));	// Real path is kicad/internat/xx_XX or kicad/internat/xx
wxString dic_path;

	if ( m_Locale != NULL ) delete m_Locale;
	m_Locale = new wxLocale();
	m_Locale->Init(m_LanguageId);
	dic_path = ReturnKicadDatasPath() + BaseDictionaryPath;
	m_Locale->AddCatalogLookupPathPrefix(dic_path);

	if ( ! first_time )
	{
		if ( m_EDA_CommonConfig )
			m_EDA_CommonConfig->Write( wxT("Language"), m_LanguageId);
	}

	if ( ! m_Locale->IsLoaded(DictionaryName) )
		m_Locale->AddCatalog(DictionaryName);
	SetLanguageList(NULL);


	if ( atof("0,1") ) g_FloatSeparator = ','; // Nombres flottants = 0,1
	else  g_FloatSeparator = '.';

	return m_Locale->IsOk();
}


/**************************************************/
void WinEDA_App::SetLanguageIdentifier(int menu_id)
/**************************************************/
/* return in m_LanguageId the language id (wxWidgets language identifier)
	from menu id (internal menu identifier)
*/
{
	switch (menu_id)
	{
		case ID_LANGUAGE_ITALIAN:
			m_LanguageId = wxLANGUAGE_ITALIAN;
			break;

		case ID_LANGUAGE_PORTUGUESE:
			m_LanguageId = wxLANGUAGE_PORTUGUESE;
			break;

		case ID_LANGUAGE_RUSSIAN:
			m_LanguageId = wxLANGUAGE_RUSSIAN;
			break;

		case ID_LANGUAGE_GERMAN:
			m_LanguageId = wxLANGUAGE_GERMAN;
			break;

		case ID_LANGUAGE_SPANISH:
			m_LanguageId = wxLANGUAGE_SPANISH;
			break;

		case ID_LANGUAGE_ENGLISH:
			m_LanguageId = wxLANGUAGE_ENGLISH;
			break;

		case ID_LANGUAGE_FRENCH:
			m_LanguageId = wxLANGUAGE_FRENCH;
			break;

		case ID_LANGUAGE_SLOVENIAN:
			m_LanguageId = wxLANGUAGE_SLOVENIAN;
			break;

		case ID_LANGUAGE_HUNGARIAN:
			m_LanguageId = wxLANGUAGE_HUNGARIAN ;
			break;

		case ID_LANGUAGE_POLISH:
			m_LanguageId = wxLANGUAGE_POLISH ;
			break;

		case ID_LANGUAGE_KOREAN:
			m_LanguageId = wxLANGUAGE_KOREAN;
			break;

		default:
			m_LanguageId = wxLANGUAGE_DEFAULT;
			break;

	}
}



/*********************************************************/
wxMenu * WinEDA_App::SetLanguageList(wxMenu * MasterMenu)
/*********************************************************/
/* Create menu list for language choice.
*/
{
wxMenuItem * item;

	if ( m_Language_Menu == NULL )
	{
		m_Language_Menu = new wxMenu;
		item = new wxMenuItem(m_Language_Menu, ID_LANGUAGE_DEFAULT,
				_("Default"), wxEmptyString, wxITEM_CHECK );
		SETBITMAPS(lang_def_xpm);
		m_Language_Menu->Append(item);

		item = new wxMenuItem(m_Language_Menu, ID_LANGUAGE_ENGLISH,
			wxT("English"), wxEmptyString, wxITEM_CHECK);
		SETBITMAPS(lang_en_xpm);
		m_Language_Menu->Append(item);

		item = new wxMenuItem(m_Language_Menu, ID_LANGUAGE_FRENCH,
			_("French"), wxEmptyString, wxITEM_CHECK);
		SETBITMAPS(lang_fr_xpm);
		m_Language_Menu->Append(item);

		item = new wxMenuItem(m_Language_Menu, ID_LANGUAGE_SPANISH,
			_("Spanish"), wxEmptyString, wxITEM_CHECK);
		SETBITMAPS(lang_es_xpm);
		m_Language_Menu->Append(item);

		item = new wxMenuItem(m_Language_Menu, ID_LANGUAGE_PORTUGUESE,
			_("Portuguese"), wxEmptyString, wxITEM_CHECK);
		SETBITMAPS(lang_pt_xpm);
		m_Language_Menu->Append(item);


		item = new wxMenuItem(m_Language_Menu, ID_LANGUAGE_ITALIAN,
			_("Italian"), wxEmptyString, wxITEM_CHECK);
		SETBITMAPS(lang_it_xpm);
		m_Language_Menu->Append(item);

		item = new wxMenuItem(m_Language_Menu, ID_LANGUAGE_GERMAN,
			_("German"), wxEmptyString, wxITEM_CHECK);
		SETBITMAPS(lang_de_xpm);
		m_Language_Menu->Append(item);

		item = new wxMenuItem(m_Language_Menu, ID_LANGUAGE_SLOVENIAN,
			_("Slovenian"), wxEmptyString, wxITEM_CHECK);
		SETBITMAPS(lang_sl_xpm);
		m_Language_Menu->Append(item);

		item = new wxMenuItem(m_Language_Menu, ID_LANGUAGE_HUNGARIAN,
			_("Hungarian"), wxEmptyString, wxITEM_CHECK);
		SETBITMAPS(lang_hu_xpm);
		m_Language_Menu->Append(item);

		item = new wxMenuItem(m_Language_Menu, ID_LANGUAGE_POLISH,
			_("Polish"), wxEmptyString, wxITEM_CHECK);
		SETBITMAPS(lang_po_xpm);
		m_Language_Menu->Append(item);

		item = new wxMenuItem(m_Language_Menu, ID_LANGUAGE_RUSSIAN,
			_("Russian"), wxEmptyString, wxITEM_CHECK);
		SETBITMAPS(lang_ru_xpm);
		m_Language_Menu->Append(item);

	item = new wxMenuItem(m_Language_Menu, ID_LANGUAGE_KOREAN,
			_("Korean"), wxEmptyString, wxITEM_CHECK);
		SETBITMAPS(lang_ko_xpm);
		m_Language_Menu->Append(item);
	}

	m_Language_Menu->Check(ID_LANGUAGE_KOREAN, FALSE);
	m_Language_Menu->Check(ID_LANGUAGE_RUSSIAN, FALSE);
	m_Language_Menu->Check(ID_LANGUAGE_POLISH, FALSE);
	m_Language_Menu->Check(ID_LANGUAGE_HUNGARIAN, FALSE);
	m_Language_Menu->Check(ID_LANGUAGE_SLOVENIAN, FALSE);
	m_Language_Menu->Check(ID_LANGUAGE_ITALIAN, FALSE);
	m_Language_Menu->Check(ID_LANGUAGE_PORTUGUESE, FALSE);
	m_Language_Menu->Check(ID_LANGUAGE_GERMAN, FALSE);
	m_Language_Menu->Check(ID_LANGUAGE_SPANISH, FALSE);
	m_Language_Menu->Check(ID_LANGUAGE_FRENCH, FALSE);
	m_Language_Menu->Check(ID_LANGUAGE_ENGLISH, FALSE);
	m_Language_Menu->Check(ID_LANGUAGE_DEFAULT, FALSE);

	switch ( m_LanguageId )
	{
		case wxLANGUAGE_KOREAN:
			m_Language_Menu->Check(ID_LANGUAGE_KOREAN, TRUE);
			break;
		case wxLANGUAGE_RUSSIAN:
			m_Language_Menu->Check(ID_LANGUAGE_RUSSIAN, TRUE);
			break;
		case wxLANGUAGE_GERMAN:
			m_Language_Menu->Check(ID_LANGUAGE_GERMAN, TRUE);
			break;
		case wxLANGUAGE_FRENCH:
			m_Language_Menu->Check(ID_LANGUAGE_FRENCH, TRUE);
			break;
		case wxLANGUAGE_ENGLISH:
			m_Language_Menu->Check(ID_LANGUAGE_ENGLISH, TRUE);
			break;
		case wxLANGUAGE_SPANISH:
			m_Language_Menu->Check(ID_LANGUAGE_SPANISH, TRUE);
			break;
		case wxLANGUAGE_PORTUGUESE:
			m_Language_Menu->Check(ID_LANGUAGE_PORTUGUESE, TRUE);
			break;

		case wxLANGUAGE_ITALIAN:
			m_Language_Menu->Check(ID_LANGUAGE_ITALIAN, TRUE);
			break;

		case wxLANGUAGE_SLOVENIAN:
			m_Language_Menu->Check(ID_LANGUAGE_SLOVENIAN, TRUE);
			break;
		
		case wxLANGUAGE_HUNGARIAN:
			m_Language_Menu->Check(ID_LANGUAGE_HUNGARIAN, TRUE);
			break;

		case wxLANGUAGE_POLISH:
			m_Language_Menu->Check(ID_LANGUAGE_POLISH, TRUE);
			break;

		default:
			m_Language_Menu->Check(ID_LANGUAGE_DEFAULT, TRUE);
			break;
	}

	if ( MasterMenu )
	{
		ADD_MENUITEM_WITH_HELP_AND_SUBMENU(MasterMenu, m_Language_Menu,
			ID_LANGUAGE_CHOICE,  _("Language"),
			wxT("For test only, use Default setup for normal use"),
			language_xpm);
	}
	return m_Language_Menu;
}


int WinEDA_App::OnRun(void)
/* Run init scripts */
{
	#ifdef KICAD_PYTHON
	PyHandler::GetInstance()->RunScripts();
	#endif
	return wxApp::OnRun();
}

