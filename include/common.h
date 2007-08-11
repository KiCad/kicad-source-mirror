	/****************************/
	/*			common.h		*/
	/****************************/

#ifndef COMMON_H
#define COMMON_H

#include "wx/confbase.h"
#include "wx/fileconf.h"

#ifndef COMMON_GLOBL
#define COMMON_GLOBL extern
#endif

/* Numero de ports TCP/IP utilis�s par KICAD */
#define KICAD_PCB_PORT_SERVICE_NUMBER 4242


/* Etat des touches speciales du clavier */

#define GR_KB_RIGHTSHIFT	0x10000000				/* Keybd states: right shift key depressed */
#define GR_KB_LEFTSHIFT		0x20000000				/* left shift key depressed */
#define GR_KB_CTRL			0x40000000				/* CTRL depressed */
#define GR_KB_ALT			0x80000000				/* ALT depressed */
#define GR_KB_SHIFT			(GR_KB_LEFTSHIFT | GR_KB_RIGHTSHIFT)
#define GR_KB_SHIFTCTRL		(GR_KB_SHIFT | GR_KB_CTRL)
#define MOUSE_MIDDLE		0x10000				/* flag indiquant bouton central souris */

/* Pseudo key codes for commands liske panning */

enum pseudokeys {
	EDA_PANNING_UP_KEY = 2000,
	EDA_PANNING_DOWN_KEY,
	EDA_PANNING_LEFT_KEY,
	EDA_PANNING_RIGHT_KEY
};

#define ESC 27

#ifdef __WINDOWS__
#define CVPCB_EXE wxT("cvpcb.exe")
#define PCBNEW_EXE wxT("pcbnew.exe")
#define EESCHEMA_EXE wxT("eeschema.exe")
#define GERBVIEW_EXE wxT("gerbview.exe")

#else
#ifndef __WXMAC__
#define CVPCB_EXE wxT("cvpcb")
#define PCBNEW_EXE wxT("pcbnew")
#define EESCHEMA_EXE wxT("eeschema")
#define GERBVIEW_EXE wxT("gerbview")
#endif
#ifdef __WXMAC__
#define CVPCB_EXE wxT("cvpcb.app")
#define PCBNEW_EXE wxT("pcbnew.app")
#define EESCHEMA_EXE wxT("eeschema.app")
#define GERBVIEW_EXE wxT("gerbview.app")
#endif
#endif



/* Orientation des textes graphiques en 0.1 degre*/
#define TEXT_ORIENT_HORIZ  0
#define TEXT_ORIENT_VERT  900

/* Affichage ou Effacement d'Item */
#define ON  1		/* Affichage  */
#define OFF 0		/* Effacement */

/* unites d'affichage sur ecran et autres */
#define INCHES  0
#define MILLIMETRE 1
#define CENTIMETRE 2

/* forward declarations: */
class LibNameList;

/* definifition des types de parametre des files de configuration */
enum paramcfg_id	/* type du parametre dans la structure ParamConfig */
{
	PARAM_INT,
	PARAM_SETCOLOR,
	PARAM_DOUBLE,
	PARAM_BOOL,
	PARAM_LIBNAME_LIST,
	PARAM_WXSTRING,
	PARAM_COMMAND_ERASE
};

#define MAX_COLOR 0x8001F
#define INT_MINVAL 0x80000000
#define INT_MAXVAL 0x7FFFFFFF

class PARAM_CFG_BASE
{
public:
	const wxChar * m_Ident;			/* Abreviation de reperage des debuts de lignes */
	paramcfg_id m_Type;				/* flag type des parametres */
	const wxChar * m_Group;			/* Nom du groupe (rubrique) de classement */
	bool m_Setup;					/* TRUE -> inscription en setup (registration base)*/

public:
	PARAM_CFG_BASE(const wxChar * ident, const paramcfg_id type, const wxChar * group = NULL);
	~PARAM_CFG_BASE() {};
};

class PARAM_CFG_INT : public PARAM_CFG_BASE
{
public:
	int * m_Pt_param;				/* pointeur sur le parametre a configurer */
	int m_Min, m_Max;			/* valeurs extremes du parametre */
	int m_Default;			/* valeur par defaut */

public:
	PARAM_CFG_INT(const wxChar * ident, int * ptparam,
				int default_val = 0, int min = INT_MINVAL, int max = INT_MAXVAL,
				const wxChar * group = NULL);
	PARAM_CFG_INT(bool Insetup, const wxChar * ident, int * ptparam,
				int default_val = 0, int min = INT_MINVAL, int max = INT_MAXVAL,
				const wxChar * group = NULL);
};

class PARAM_CFG_SETCOLOR : public PARAM_CFG_BASE
{
public:
	int * m_Pt_param;				/* pointeur sur le parametre a configurer */
	int m_Default;					/* valeur par defaut */

public:
	PARAM_CFG_SETCOLOR(const wxChar * ident, int * ptparam,
				int default_val, const wxChar * group = NULL);
	PARAM_CFG_SETCOLOR(bool Insetup, const wxChar * ident, int * ptparam,
				int default_val, const wxChar * group = NULL);
};

class PARAM_CFG_DOUBLE : public PARAM_CFG_BASE
{
public:
	double * m_Pt_param;				/* pointeur sur le parametre a configurer */
	double m_Default;			/* valeur par defaut */
	double m_Min, m_Max;			/* valeurs extremes du parametre */

public:
	PARAM_CFG_DOUBLE(const wxChar * ident, double * ptparam,
				double default_val = 0.0, double min = 0.0, double max = 10000.0,
				const wxChar * group = NULL);
	PARAM_CFG_DOUBLE(bool Insetup, const wxChar * ident, double * ptparam,
				double default_val = 0.0, double min = 0.0, double max = 10000.0,
				const wxChar * group = NULL);
};

class PARAM_CFG_BOOL : public PARAM_CFG_BASE
{
public:
	bool * m_Pt_param;				/* pointeur sur le parametre a configurer */
	int m_Default;					/* valeur par defaut */

public:
	PARAM_CFG_BOOL(const wxChar * ident, bool * ptparam,
				int default_val = FALSE,const wxChar * group = NULL);
	PARAM_CFG_BOOL(bool Insetup, const wxChar * ident, bool * ptparam,
				int default_val = FALSE,const wxChar * group = NULL);
};


class PARAM_CFG_WXSTRING : public PARAM_CFG_BASE
{
public:
	wxString * m_Pt_param;				/* pointeur sur le parametre a configurer */

public:
	PARAM_CFG_WXSTRING(const wxChar * ident, wxString * ptparam, const wxChar * group = NULL);
	PARAM_CFG_WXSTRING(bool Insetup, const wxChar * ident, wxString * ptparam, const wxChar * group = NULL);
};

class PARAM_CFG_LIBNAME_LIST : public PARAM_CFG_BASE
{
public:
	wxArrayString * m_Pt_param;		/* pointeur sur le parametre a configurer */

public:
	PARAM_CFG_LIBNAME_LIST(const wxChar * ident, wxArrayString * ptparam, const wxChar * group = NULL);
};


	/***********************************/
	/* Classe pour affichage de textes */
	/***********************************/
class WinEDA_TextFrame: public wxDialog
{
private:
	wxWindow * m_Parent;
	wxListBox * m_List;

public:
	WinEDA_TextFrame(wxWindow * parent, const wxString & title);
	void Append( const wxString & text);
private:
	void D_ClickOnList(wxCommandEvent& event);
	void OnClose(wxCloseEvent& event);

	DECLARE_EVENT_TABLE()
};


/* Gestion des feuilles de trac�:
*/
class Ki_PageDescr
{
// All sizes are in 1/1000 inch
public:
	wxSize m_Size ;		/* page size in 1/1000 inch */
	wxPoint m_Offset;	/* plot offset in 1/1000 inch */
	wxString m_Name;
	int m_LeftMargin;
	int m_RightMargin;
	int m_TopMargin;
	int m_BottomMargin;

public:
	Ki_PageDescr(const wxSize & size, const wxPoint & offset, const wxString & name);
};


/* Standard page sizes in 1/1000 inch */
#if defined EDA_BASE
Ki_PageDescr g_Sheet_A4(wxSize(11700,8267), wxPoint(0,0), wxT("A4") );
Ki_PageDescr g_Sheet_A3(wxSize(16535,11700),wxPoint(0,0),wxT("A3") );
Ki_PageDescr g_Sheet_A2(wxSize(23400,16535),wxPoint(0,0),wxT("A2") );
Ki_PageDescr g_Sheet_A1(wxSize(33070,23400),wxPoint(0,0),wxT("A1") );
Ki_PageDescr g_Sheet_A0(wxSize(46800,33070),wxPoint(0,0),wxT("A0") );
Ki_PageDescr g_Sheet_A(wxSize(11000,8500),wxPoint(0,0),wxT("A") );
Ki_PageDescr g_Sheet_B(wxSize(17000,11000),wxPoint(0,0),wxT("B") );
Ki_PageDescr g_Sheet_C(wxSize(22000,17000),wxPoint(0,0),wxT("C") );
Ki_PageDescr g_Sheet_D(wxSize(34000,22000),wxPoint(0,0),wxT("D") );
Ki_PageDescr g_Sheet_E(wxSize(44000,34000),wxPoint(0,0),wxT("E") );
Ki_PageDescr g_Sheet_GERBER(wxSize(32000,32000),wxPoint(0,0),wxT("GERBER") );
Ki_PageDescr g_Sheet_user(wxSize(17000,11000),wxPoint(0,0),wxT("User") );
#else
extern Ki_PageDescr g_Sheet_A4 ;
extern Ki_PageDescr g_Sheet_A3 ;
extern Ki_PageDescr g_Sheet_A2 ;
extern Ki_PageDescr g_Sheet_A1 ;
extern Ki_PageDescr g_Sheet_A0 ;
extern Ki_PageDescr g_Sheet_A ;
extern Ki_PageDescr g_Sheet_B ;
extern Ki_PageDescr g_Sheet_C ;
extern Ki_PageDescr g_Sheet_D ;
extern Ki_PageDescr g_Sheet_E ;
extern Ki_PageDescr g_Sheet_GERBER ;
extern Ki_PageDescr g_Sheet_user ;
#endif


COMMON_GLOBL int g_LastKey;			/* code de la derniere touche actionn�� */
COMMON_GLOBL wxString g_ProductName
#ifdef EDA_BASE
= wxT("KiCad E.D.A.  ")
#endif
;

/* Gestion des librairies */
COMMON_GLOBL wxString g_RealLibDirBuffer;	// Chemin reel des librairies de module
									// = UserLibDirBuffer si non vide
									// = chemin par defaut sinon
COMMON_GLOBL wxString g_UserLibDirBuffer;	// Chemin des librairies de module donne par
									// le file de config

/* variables globales generales */

COMMON_GLOBL int g_DebugLevel;		// 0= Pas de debug */
COMMON_GLOBL int g_MouseOldButtons;
COMMON_GLOBL int g_KeyPressed;
// Font used by kicad.
// these font have a size which do not depend on default size system font
COMMON_GLOBL wxFont * g_StdFont;	/* Standard font used for status display ,in message panel */
COMMON_GLOBL wxFont * g_DialogFont;	/* Normal font used in dialog box */
COMMON_GLOBL wxFont * g_ItalicFont;	/* Italic font used in dialog box */
COMMON_GLOBL wxFont * g_MsgFont;	/* Italic font used in msg panel (lower window) */
COMMON_GLOBL wxFont * g_FixedFont;	/* Affichage de Texte en fenetres de dialogue,
							fonte a pas fixe)*/
COMMON_GLOBL int g_StdFontPointSize;	/* taille de la fonte */
COMMON_GLOBL int g_DialogFontPointSize;	/* taille de la fonte */
COMMON_GLOBL int g_FixedFontPointSize;	/* taille de la fonte */
COMMON_GLOBL int g_MsgFontPointSize;	/* taille de la fonte */
COMMON_GLOBL int g_FontMinPointSize;	/* taille minimum des fontes */

COMMON_GLOBL bool g_IsPrinting;		// TRUE si impression au lieu de trace a l'ecran
COMMON_GLOBL bool g_ShowPageLimits	// TRUE to display the page limits
#ifdef EDA_BASE
= TRUE
#endif
;

/* Gloabl variables for project handling */
COMMON_GLOBL wxString g_Prj_Config_Filename_ext
#ifdef EDA_BASE
= wxT(".pro" )
#endif
;
COMMON_GLOBL wxFileConfig * g_Prj_Config;				// Configuration locale, propre au projet
COMMON_GLOBL wxString g_Prj_Default_Config_FullFilename;	// Nom (full file name) du file Configuration par defaut (kicad.pro)
COMMON_GLOBL wxString g_Prj_Config_LocalFilename;	// Nom du file Configuration local (<curr projet>.pro)

// Handle the preferd editor for browsing report files:
COMMON_GLOBL wxString g_EditorName;


// Gestion de la grille "utilisateur" (User Grid)
#ifdef EDA_BASE
wxRealPoint g_UserGrid(0.01, 0.01);
int g_UserGrid_Unit = INCHES;
#else
extern wxRealPoint g_UserGrid;
extern int g_UserGrid_Unit;
#endif

COMMON_GLOBL int g_UnitMetric;			// display units mm = 1, inches = 0, cm = 2


// shape selector for cursor screen
COMMON_GLOBL int g_CursorShape;

/* Draw color for moving objects: */
COMMON_GLOBL int g_GhostColor;

/* Draw color for grid: */
COMMON_GLOBL int g_GridColor
#ifdef EDA_BASE
	= DARKGRAY
#endif
;

/* Current used screen: */
COMMON_GLOBL BASE_SCREEN * ActiveScreen;



/**************************************/
/* Prototypage des Fonctions Usuelles */
/**************************************/

class WinEDA_DrawFrame;
class WinEDAListBox;
class WinEDA_DrawPanel;


/* COMMON.CPP */
wxString ReturnPcbLayerName(int layer_number, bool is_filename = FALSE,  bool is_gui = FALSE);
/* Return the name of the layer number "layer_number".
	if "is_filename" == TRUE,  the name can be used for a file name
	(not internatinalized, no space)*/


/*********************/
/* PROJET_CONFIG.CPP */
/*********************/

/**************/
/* DRAWTXT.CPP */
/**************/
void DrawGraphicText(WinEDA_DrawPanel * panel, wxDC * DC, const wxPoint & pos,
			int mode_color,  const wxString & Text,
			int orient , const wxSize & char_size,
			int h_justify, int v_justify, int width = 0);

void PlotGraphicText(int format_plot, const wxPoint & Pos, int gcolor,
				 const wxString & Text,
				int orient, const wxSize & Size, int h_justify, int v_justify);

/***************/
/* CONFIRM.CPP */
/***************/
void DisplayError(wxWindow * parent, const wxString & msg, int displaytime = 0);
void DisplayInfo(wxWindow * parent, const wxString & msg, int displaytime = 0);
	/* Routines d'affichage messages ( disparait au bout de displaytime 0.1 secondes) */

bool IsOK(wxWindow * parent, const wxString & msg) ;
				/* Routine affichant la fenetre "CONFIRMATION"
				Retourne 1 ou 0 selon reponse Yes / No */

int Get_Message(const wxString & titre, wxString & buffer, wxWindow * frame) ;
				/* Fonction d'installation du menu de Dialogue
					entree: titre = titre a afficher
					entree/sortie :buffer : contient la reponse
							 si a l'appel buffer n'est pas vide, son contenu est aussi
							 affiche, mais disparait a la 1ere correction */

/************************/
/* file GESTFICH.CPP */
/************************/

wxString GetEditorName(void);	// Return the prefered editor name
void OpenPDF( const wxString & file );
void OpenFile( const wxString & file );


bool EDA_DirectorySelector(const wxString & Title,		/* Titre de la fenetre */
					wxString & Path,			/* Chemin par defaut */
					int flag,					/* reserve */
					wxWindow * Frame,			/* parent frame */
					const wxPoint & Pos);

wxString EDA_FileSelector(const wxString & Title,	/* Window title */
					const wxString & Path,		/* default path */
					const wxString & FileName,	/*  default filename */
					const wxString & Ext,		/* default extension */
					const wxString & Mask,		/* Display filename mask */
					wxWindow * Frame,			/* parent frame */
					int flag,					/* wxSAVE, wxOPEN ..*/
					const bool keep_working_directory,	/* true = do not change the C.W.D. */
					const wxPoint & Pos = wxPoint(-1, -1)
					);

wxString MakeFileName( const wxString & dir,
		const wxString & shortname, const wxString & ext);
/* Calcule le nom complet d'un file d'apres les chaines
	dir = prefixe (chemin)
	shortname = nom avec ou sans chemin ou extension
	ext = extension

	si la chaine name possede deja un chemin ou une extension, elles
	ne seront pas modifiees

	retourne la chaine calculee */

wxString MakeReducedFileName( const wxString & fullfilename,
						const wxString & default_path,
						const wxString & default_ext);
/* Calcule le nom "reduit" d'un file d'apres les chaines
	fullfilename = nom complet
	default_path = prefixe (chemin) par defaut
	default_ext = extension	par defaut

	retourne le nom reduit, c'est a dire:
	sans le chemin si le chemin est default_path
	avec ./ si si le chemin est le chemin courant
	sans l'extension si l'extension est default_ext

	Renvoie un chemin en notation unix ('/' en separateur de repertoire)
*/

WinEDAListBox * GetFileNames(char *Directory, char *Mask);

void ChangeFileNameExt( wxString & FullFileName, const wxString & NewExt );
			/* Change l'extension du "filename FullFileName" en NewExt.
				   Retourne FullFileName */

int ExecuteFile(wxWindow * frame, const wxString & ExecFile,
			const wxString & param = wxEmptyString);
void AddDelimiterString( wxString & string );

void SetRealLibraryPath(const wxString & shortlibname); /* met a jour
 le chemin des librairies RealLibDirBuffer (global)
a partir de UserLibDirBuffer (global):
	Si UserLibDirBuffer non vide RealLibDirBuffer = UserLibDirBuffer.
	Sinon si variable d'environnement KICAD definie (KICAD = chemin pour kicad),
		UserLibDirBuffer = <KICAD>/shortlibname;
	Sinon UserLibDirBuffer = <Chemin des binaires>../shortlibname/
*/
wxString FindKicadHelpPath(void);
/* Find absolute path for kicad/help (or kicad/help/<language>) */

wxString ReturnKicadDatasPath(void);
	/* Retourne le chemin des donnees communes de kicad. */

wxString FindKicadFile(const wxString & shortname);
/* Search the executable file shortname in kicad binary path and return
full file name if found or shortname */


/*************/
/* STRING.CPP */
/*************/
char * strupper(char * Text);

int ReadDelimitedText(char * dest, char * source, int NbMaxChar );
	/* lit et place dans dest la chaine de caractere trouvee dans source,
		 delimitee par " .
		transfere NbMaxChar max
		 retourne le nombre de codes lus dans source
		dest est termine par NULL */

char * GetLine(FILE *File, char *Line, int *LineNum = NULL, int SizeLine = 255);
									/* Routine de lecture de 1 ligne utile
									retourne la 1ere ligne utile lue.
									elimine lignes vides et commentaires */
char * StrPurge(char * text);
				/* Supprime les caracteres Space en debut de la ligne text
				retourne un pointeur sur le 1er caractere non Space de text */

char * DateAndTime(char * line);
wxString DateAndTime(void);
				/* Retourne la chaine de caractere donnant date+heure */

int StrLenNumCmp(const wxChar *str1,const wxChar *str2, int NbMax);
	/*
	routine (compatible qsort() ) de comparaision pour classement alphab�tique
	Analogue a strncmp() mais les nombres sont compar�s selon leur valeur num�rique
	et non pas par leur code ascii */

int StrNumICmp(const wxChar *str1,const wxChar *str2);
	/* routine (compatible qsort() ) de comparaison pour classement alphab�tique,
	avec lower case == upper case.
	Analogue a stricmp() mais les nombres sont compar�s selon leur valeur num�rique
	et non pas par leur code ascii */

int StrLenNumICmp(const wxChar *str1,const wxChar *str2, int NbMax);
	/* routine (compatible qsort() ) de comparaison pour classement alphab�tique,
	avec lower case == upper case.
	Analogue a stricmp() mais les nombres sont compar�s selon leur valeur num�rique
	et non pas par leur code ascii */

bool WildCompareString(const wxString & pattern, const wxString & string_to_tst,
			bool case_sensitive = TRUE);
			/* compare 2 noms de composants, selon regles usuelles
			  ( Jokers * , ? , autoris�s).
			la chaine de reference est "pattern"
			si case_sensitive == TRUE (default), comparaison exacte
			retourne TRUE si match FALSE si differences */

char * to_point(char * Text);
	/* convertit les , en . dans une chaine. utilis� pour compenser la fct printf
	qui genere les flottants avec une virgule au lieu du point en mode international */

/****************/
/* infospgm.cpp */
/****************/
void Print_Kicad_Infos(wxWindow * frame);

/**************/
/* common.cpp */
/**************/
wxString GetBuildVersion(void);	/* Return the build date */

void Affiche_1_Parametre(WinEDA_DrawFrame * frame ,
			int pos_X,const wxString& texte_H,const wxString& texte_L,int color);
/*
 Routine d'affichage d'un parametre.
	pos_X = cadrage horizontal
		si pos_X < 0 : la position horizontale est la derniere
			valeur demandee >= 0
	texte_H = texte a afficher en ligne superieure.
		si "", par d'affichage sur cette ligne
	texte_L = texte a afficher en ligne inferieure.
		si "", par d'affichage sur cette ligne
	color = couleur d'affichage
*/

void AfficheDoc(WinEDA_DrawFrame * frame, const wxString & Doc, const wxString & KeyW);
	/* Routine d'affichage de la documentation associee a un composant */

int GetTimeStamp(void);
	/* Retoure une identification temporelle (Time stamp) differente a chaque appel */
int DisplayColorFrame(wxWindow * parent);
int GetCommandOptions(const int argc, const char **argv, const char * stringtst,
		const char ** optarg, int * optind);


void valeur_param(int valeur,wxString & buf_texte);
/* Retourne pour affichage la valeur d'un parametre, selon type d'unites choisies
	entree : valeur en mils , buffer de texte
	retourne en buffer : texte : valeur exprimee en pouces ou millimetres
						suivie de " ou mm
*/

wxString ReturnUnitSymbol(int Units = g_UnitMetric);
int ReturnValueFromString(int Units, const wxString & TextValue, int Internal_Unit);
wxString ReturnStringFromValue(int Units, int Value, int Internal_Unit);
void AddUnitSymbol(wxStaticText & Stext, int Units = g_UnitMetric);
	/* Add string "  (mm):" or " ("):" to the static text Stext.
		Used in dialog boxes for entering values depending on selected units */
void PutValueInLocalUnits(wxTextCtrl & TextCtr, int Value, int Internal_Unit);
	/* Convert the number Value in a string according to the internal units
		and the selected unit (g_UnitMetric) and put it in the wxTextCtrl TextCtrl */
int ReturnValueFromTextCtrl(const wxTextCtrl & TextCtr, int Internal_Unit);
	/* Convert the Value in the wxTextCtrl TextCtrl in an integer,
		according to the internal units and the selected unit (g_UnitMetric) */

double To_User_Unit(bool is_metric, int val,int internal_unit_value);
int From_User_Unit(bool is_metric, double val,int internal_unit_value);
wxString GenDate(void);
void MyFree (void * pt_mem);
void * MyZMalloc (size_t nb_octets);
void * MyMalloc (size_t nb_octets);


/****************/
/* eda_doc.cpp */
/****************/
int KeyWordOk(const wxString & KeyList, const wxString & Database );
/* Recherche si dans le texte Database on retrouve tous les mots
	cles donnes dans KeyList ( KeyList = suite de mots cles
	separes par des espaces
	Retourne:
		0 si aucun mot cle trouv�
		1 si mot cle trouv�
*/
bool GetAssociatedDocument(wxFrame * frame, const wxString & LibPath,
							const wxString & DocName);


/****************************/
/* get_component_dialog.cpp */
/****************************/
wxString GetComponentName(WinEDA_DrawFrame * frame,
	wxArrayString & HistoryList, const wxString & Title,
	wxString(*AuxTool)(WinEDA_DrawFrame *parent) );
	/* Dialog frame to choose a component name */
void AddHistoryComponentName(wxArrayString & HistoryList, const wxString & Name);
	/* Add the string "Name" to the history list */


/**********************/
/* block_commande.cpp */
/**********************/
void AbortBlockCurrentCommand(WinEDA_DrawPanel * Panel, wxDC * DC);
	/* Cancel Current block operation. */
void InitBlockLocateDatas( WinEDA_DrawPanel * Panel,const wxPoint & startpos );
	/* Init the initial values of a BlockLocate, before starting a block command */
void DrawAndSizingBlockOutlines(WinEDA_DrawPanel * panel, wxDC * DC, bool erase );
	/* Redraw the outlines of the block which shows the search area for block commands
	The first point of the rectangle showing the area is initialised
	by InitBlockLocateDatas().
	The other point of the rectangle is the mouse cursor */



#endif	// COMMON_H


