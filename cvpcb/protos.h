		/**************************************/
		/**	protos.h     liste des fonctions **/
		/**************************************/

#ifndef PROTOS_H
#define PROTOS_H

class DESCR_EDGE;

int reaffect( char *ib, char *net) ;
int GenNetlistPcbnew() ;
int loadcmp() ;
int listlib() ;
STOREMOD * GetModuleDescrByName(const wxString & FootprintName);

	/***********/
	/* CVPCB.CPP */
	/***********/
void ModListProcedure(wxListBox& list, wxCommandEvent& event);

	/***********/
	/* CFG.CPP */
	/***********/

void Save_Config(wxWindow * parent);
void Read_Config( const wxString & FullFileName );  /* lit la configuration */

	/**************/
	/* VISUMOD.CC */
	/**************/

void AfficheModule(WinEDA_DrawPanel * panel, wxDC * DC);  /* Affiche le module courant dans un cadre */
void Set_Rectangle_Encadrement(MODULE * PtModule);
		/* Mise a jour du rectangle d'encadrement du module
			  Entree : pointeur sur module */

void DeleteStructure( void * GenericStructure );
	/* Supprime de la liste chainee la stucture pointee par GenericStructure
	et libere la memoire correspondante */

	/**************/
	/* MEMOIRE.CC */
	/**************/
void FreeMemoryComponants();
	/* Routine de liberation memoire de la liste des composants
		 - remet a NULL BaseListeMod
		 - remet a 0 NbComp */

void FreeMemoryModules();
	/* Routine de liberation memoire de la liste des modules
		 - remet a NULL g_BaseListePkg
		 - rement a 0 NbLib; */


	/***************/
	/* TRACEMOD.CC */
	/***************/

void Trace_empreinte(WinEDA_DrawPanel * panel, wxDC * DC,
			MODULE * Module,int ox, int oy,int mode_color);
void Trace_1_Pad(WinEDA_DrawPanel * panel, wxDC * DC,
			D_PAD* ptr_pad,int ox, int oy,int mode_color);
void Trace_Ancre_Module(WinEDA_DrawPanel * panel,
			wxDC * DC,  MODULE * Module, int ox, int oy, int dim, int mode_color);
void Trace_DrawSegmentModule(WinEDA_DrawPanel * panel, wxDC * DC,  DESCR_EDGE* PtDrawSegment,
								 int ox, int oy, int mode_color);
void Gr_E_texte(WinEDA_DrawPanel * panel, wxDC * DC,
			MODULE * Module, TEXTE_MODULE * ptr,int ox,int oy,int mode_color) ;
void Trace_Contour_Module(WinEDA_DrawPanel * panel, wxDC * DC,
								EDA_BaseStruct * PtStruct,
								int ox, int oy, int mode_color);


void Affiche_1_Segment(WinEDA_DrawPanel * panel, wxDC * DC,
						int ux0, int uy0, int dx, int dy,
						int width, int mode, int color);

/**************/
/* RDORCAD.CC */
/**************/

STORECMP * TriListeComposantss(STORECMP * BaseListe, int nbitems);
		/* Tri la liste des composants par ordre alphabetique et me a jour
			le nouveau chainage avant/arriere
				   retourne un pointeur sur le 1er element de la liste */

int CmpCompare( void * cmp1, void * cmp2); /* routine pour qsort()
										 de tri de liste des composants */


/***************/
/* viewlogi.cc */
/***************/
int ReadViewlogicWirList();

/***************/
/* viewlnet.cc */
/***************/
int ReadViewlogicNetList();

/***************/
/* TRACEMOD.CC */
/***************/
void Display_1_Texte(WinEDA_DrawPanel * panel, wxDC * DC,
					const char * Text, int Nmax, int ox, int oy,
					int size_h, int size_v, int width, int orient, int Color);
	/* Affichage de 1 texte a l'ecran. Parametres:
			char * Text = pointeur sur le texte a afficher
			  int Nmax = Nombre max de caracteres a afficher
			  int ox, int oy = coord de trace relatives ( absolue - offset )
			  int size_h = taille H, si < 0: affichage en miroir.
			  int size_v = taille V
			  int width = epaisseur; si = 0 ou 1: affichage en filaire.
			  int orient = orientation en 0,1 degre
			  int Color = couleur | mode */  

/**********/
/* COLORS */
/**********/
void BuildPensBrushes();
void FreePensBrushes();
int GetNewColor(wxWindow * Frame, int OldColor = -1);	/* Routine de selection d'une couleur */

/***********/
/* OPTIONS */
/***********/
void CreateOptionsWindow(WinEDA_DrawFrame * parent);
			/* Creation de la fenetre d'options de la fenetre de visu */

#endif	// PROTOS_H

