	/************************************************************/
	/*						appl_wxstruct.h:					*/
	/* descriptions des principales classes derivees utilisees: */
	/*    Class "EDA_Appl: classe de l'application generale		*/
	/************************************************************/

/* Ce fichier doit etre inclus dans "wxstruct.h"
*/

#ifndef  APPL_WXSTRUCT_H
#define  APPL_WXSTRUCT_H

#ifndef eda_global
#define eda_global extern
#endif


	/**********************************************/
	/*  Class representing the entire Application */
	/**********************************************/
eda_global WinEDA_App * EDA_Appl;  /* application representant le programme */


class WinEDA_App: public wxApp
{
public:
	wxString m_Project;
	wxSingleInstanceChecker * m_Checker;
	WinEDA_MainFrame * m_MainFrame;
	WinEDA_PcbFrame * m_PcbFrame;
	WinEDA_ModuleEditFrame * m_ModuleEditFrame;
	WinEDA_GerberFrame * m_GerberFrame;
	WinEDA_SchematicFrame * SchematicFrame;	// Edition des Schemas
	WinEDA_LibeditFrame * LibeditFrame;		// Edition des composants
	WinEDA_ViewlibFrame * ViewlibFrame;		// Visualisation des composants
	WinEDA_CvpcbFrame * m_CvpcbFrame;

	wxPoint m_HelpPos;
	wxSize m_HelpSize;
	wxHtmlHelpController * m_HtmlCtrl;
	wxConfig * m_EDA_Config;		// Config courante (tailles et positions fenetres ...*/
	wxConfig * m_EDA_CommonConfig;		// common setup (language ...) */
	wxString m_HelpFileName;
	wxString m_CurrentOptionFile;	// dernier fichier .cnf utilisé
	wxString m_CurrentOptionFileDateAndTime;

	wxString m_BinDir;				/* Chemin ou reside l'executable
									(utilisé si KICAD non défini)*/
	wxArrayString m_LastProject;	/* liste des derniers projets chargés */
	unsigned int m_LastProjectMaxCount;		/* Max histhory file length */
	wxString m_KicadEnv;			/* Chemin de kicad défini dans la variable
									d'environnement KICAD,
									typiquement /usr/local/kicad ou c:\kicad */
	bool m_Env_Defined;				// TRUE si variable d'environnement KICAD definie

	wxLocale * m_Locale;				// Gestion de la localisation
	int m_LanguageId;				// indicateur de choix du langage ( 0 = defaut)
	wxMenu * m_Language_Menu;		// List menu for languages
	wxString m_PdfBrowser;			// Name of the selected browser, for browsing pdf datasheets
	bool m_PdfBrowserIsDefault;		// True if the pdf browser is the default (m_PdfBrowser not used)

public:
	WinEDA_App(void);
	~WinEDA_App(void);
	bool OnInit(void);

	bool SetBinDir(void);
	void InitEDA_Appl(const wxString & name);
	bool SetLanguage(bool first_time = FALSE);
	wxMenu * SetLanguageList(wxMenu * MasterMenu);
	void SetLanguageIdentifier(int menu_id);
	void InitOnLineHelp(void);

	// Sauvegarde de configurations et options:
	void GetSettings(void);
	void SaveSettings(void);
	void SetLastProject(const wxString & FullFileName);
	void WriteProjectConfig(const wxString & local_config_filename,
			const wxString & GroupName, PARAM_CFG_BASE ** List);

	bool ReadProjectConfig(const wxString & local_config_filename,
			const wxString & GroupName, PARAM_CFG_BASE ** List,
			bool Load_Only_if_New);

	void ReadPdfBrowserInfos(void);
	void WritePdfBrowserInfos(void);

};


#endif  /* APPL_WXSTRUCT_H */

