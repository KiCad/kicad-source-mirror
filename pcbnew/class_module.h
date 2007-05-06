	/*******************************************************/
	/* class_module.h : module description (excepted pads) */
	/*******************************************************/


class Pcb3D_GLCanvas;
class Struct3D_Master;

	/************************************/
	/* Modules (footprints) description */
	/* pad are in class_pad.xx			*/
	/************************************/

/* Format des modules:
	Description generale
	Description segments contour
	Description textes
	Description pastilles
*/

/* Flags :*/

enum Mod_Attribut		/* Attributs d'un module */
	{
	MOD_DEFAULT = 0,	/* Type default */
	MOD_CMS = 1,		/* Pour module apparaissant dans les
						fichiers de placement automatique (principalement modules CMS */
	MOD_VIRTUAL = 2		/* Module virtuel constitue par un dessin sur circuit
						(connecteur, trou de percage..) */
	};

/* flags for autoplace and autoroute (.m_ModuleStatus member) */
#define MODULE_is_LOCKED 0x01	/* module LOCKED: no autoplace allowed */
#define MODULE_is_PLACED 0x02	/* In autoplace: module automatically placed */
#define MODULE_to_PLACE 0x04	/* In autoplace: module waiting fot autoplace */

class MODULE: public EDA_BaseStruct
{
public:
	int m_Layer;				// layer number
	wxPoint m_Pos;				// Real coord on board
	D_PAD * m_Pads;				/* Pad list (linked list) */
	EDA_BaseStruct * m_Drawings;	/* Graphic items list (linked list) */
	Struct3D_Master * m_3D_Drawings;	/* Pointeur sur la liste des elements de trace 3D*/
	TEXTE_MODULE * m_Reference;	// texte reference du composant (U34, R18..)
	TEXTE_MODULE * m_Value;	// texte valeur du composant (74LS00, 22K..)
	wxString m_LibRef;		/* nom du module en librairie */

	int m_Attributs;	 	/* Flags bits a bit ( voir enum Mod_Attribut ) */
	int m_Orient ;	 		/* orientation en 1/10 degres */
	int flag ;				/* flag utilise en trace rastnest et routage auto */
	int m_ModuleStatus;		/* For autoplace: flags (LOCKED, AUTOPLACED) */
	EDA_Rect m_BoundaryBox ;	/* position/taille du cadre de reperage (coord locales)*/
	EDA_Rect m_RealBoundaryBox ; /* position/taille du module (coord relles) */
	int m_PadNum;			// Nombre total de pads
	int m_AltPadNum;		// en placement auto Nombre de pads actifs pour
							// les calculs
	int m_CntRot90;			// Placement auto: cout ( 0..10 ) de la rotation 90 degre
	int m_CntRot180;		// Placement auto: cout ( 0..10 ) de la rotation 180 degre
	wxSize m_Ext;			// marges de "garde": utilise en placement auto.
	float m_Surface;		// surface du rectangle d'encadrement

	long m_Link;			// variable temporaire ( pour editions, ...)
	long m_LastEdit_Time;	// Date de la derniere modification du module (gestion de librairies)

	wxString m_Doc;			// Texte de description du module
	wxString m_KeyWord;		// Liste des mots cles relatifs au module

public:
	MODULE(BOARD * parent);
	MODULE(MODULE * module);
	~MODULE(void);

	void Copy(MODULE * Module);	// Copy structure
	MODULE * Next(void) { return (MODULE *) Pnext; }

	void Set_Rectangle_Encadrement(void); /* mise a jour du rect d'encadrement
							en coord locales (orient 0 et origine = pos  module) */
	void SetRectangleExinscrit(void);	/* mise a jour du rect d'encadrement
							et de la surface en coord reelles */


	// deplacements
	void SetPosition(const wxPoint & newpos);
	void SetOrientation(int newangle);

	/* supprime du chainage la structure Struct */
	void UnLink( void );

	/* Readind and writing data on files */
	int WriteDescr( FILE * File );
	int Write_3D_Descr( FILE * File );
	int ReadDescr( FILE * File, int * LineNum = NULL);
	int Read_3D_Descr( FILE * File, int * LineNum = NULL);

	/* drawing functions */
	void Draw(WinEDA_DrawPanel * panel, wxDC * DC,
				const wxPoint & offset, int draw_mode);
	void Draw3D(Pcb3D_GLCanvas * glcanvas);
	void DrawEdgesOnly(WinEDA_DrawPanel * panel, wxDC * DC,
				const wxPoint & offset,int draw_mode);
	void DrawAncre(WinEDA_DrawPanel * panel, wxDC * DC,
				const wxPoint & offset, int dim_ancre, int draw_mode);

	/* miscellaneous */
	void Display_Infos(WinEDA_BasePcbFrame * frame);
};

