/*****************************************/
/* prototypage des fonctions de EESchema */
/*****************************************/
void FreeLibraryEntry(LibCmpEntry * Entry);

LibEDA_BaseStruct * LocatePin(const wxPoint & RefPos,
			EDA_LibComponentStruct * Entry,
			int Unit, int Convert, EDA_SchComponentStruct * DrawItem = NULL);
		/* Routine de localisation d'une PIN de la PartLib pointee par Entry */

wxString ReturnDefaultFieldName(int FieldNumber);


/***************/
/* FILE_IO.CPP */
/***************/
void SaveProject(WinEDA_SchematicFrame * frame);


/****************/
/* DATABASE.CPP */
/****************/
void DisplayCmpDoc(wxString & Name);
bool DataBaseGetName(WinEDA_DrawFrame * frame,
		wxString & Keys, wxString & BufName);

/*********************/
/* DANGLING_ENDS.CPP */
/*********************/
bool SegmentIntersect(int Sx1, int Sy1, int Sx2, int Sy2, int Px1, int Py1);

/****************/
/* BUS_WIRE_JUNCTION.CPP */
/****************/
void IncrementLabelMember(wxString & name);

/****************/
/* EDITPART.CPP */
/****************/
void InstallCmpeditFrame(WinEDA_SchematicFrame * parent, wxPoint & pos,
			EDA_SchComponentStruct * m_Cmp);


	/**************/
	/* EELIBS2.CPP */
	/**************/

/* Functions common to all EELibs?.c modules: */
int LibraryEntryCompare(EDA_LibComponentStruct *LE1, EDA_LibComponentStruct *LE2);
int NumOfLibraries(void);
EDA_LibComponentStruct *FindLibPart(const wxChar *Name, const wxString & LibName, int Alias);

void DrawingLibInGhost(WinEDA_DrawPanel * panel, wxDC * DC, EDA_LibComponentStruct *LibEntry,
						EDA_SchComponentStruct * DrawLibItem, int PartX, int PartY,
						int Multi, int convert,
						int Color, bool DrawPinText);

void DrawLibEntry(WinEDA_DrawPanel * panel, wxDC * DC,
							EDA_LibComponentStruct *LibEntry, int posX, int posY,
							int Multi, int convert,
							int DrawMode, int Color = -1);

void DrawPinSymbol(WinEDA_DrawPanel * panel, wxDC * DC,
							int posX, int posY, int len, int orient, int SymbolType,
							int DrawMode, int Color = -1);

void DrawLibraryDrawStruct(WinEDA_DrawPanel * panel, wxDC * DC,
							EDA_LibComponentStruct *LibEntry, int PartX, int PartY,
						  LibEDA_BaseStruct *DrawItem, int Multi,
						  int DrawMode, int Color = -1);

bool MapAngles(int *Angle1, int *Angle2, int TransMat[2][2]);


	/**************/
	/* EELIBS1.CPP */
	/**************/
EDA_LibComponentStruct * Read_Component_Definition(WinEDA_DrawFrame * frame, char * Line,
		FILE *f, int *LineNum);
/* Routine to Read a DEF/ENDDEF part entry from given open file. */

LibraryStruct *FindLibrary(const wxString & Name);
int LoadDocLib(WinEDA_DrawFrame * frame, const wxString & FullDocLibName, const wxString & Libname);
PriorQue *LoadLibraryAux(WinEDA_DrawFrame * frame, LibraryStruct * library,
			FILE *f, int *NumOfParts);
LibraryStruct * LoadLibraryName(WinEDA_DrawFrame * frame,
				const wxString & FullLibName, const wxString & LibName);
void LoadLibraries(WinEDA_DrawFrame * frame);
void FreeCmpLibrary(wxWindow * frame, const wxString & LibName);
const wxChar **GetLibNames(void);

void SnapLibItemPoint(int OrigX, int OrigY, int *ClosestX, int *ClosestY,
						 EDA_SchComponentStruct *DrawLibItem);
bool LibItemInBox(int x1, int y1, int x2, int y2,
					EDA_SchComponentStruct *DrawLibItem);
void DrawTextField(WinEDA_DrawPanel * panel, wxDC * DC, PartTextStruct * Field, int IsMulti, int DrawMode);
			/* Routine de trace des textes type Field du composant.
				   entree:
				  Field: champ
				  IsMulti: flag Non Null si il y a plusieurs parts par boitier.
						  n'est utile que pour le champ reference pour ajouter a celui ci
						  l'identification de la part ( A, B ... )
				  DrawMode: mode de trace */

char * StrPurge(char * text);
				/* Supprime les caracteres Space en debut de la ligne text
				   retourne un pointeur sur le 1er caractere non Space de text */

	/************/
	/* BLOCK.CPP */
	/************/
EDA_BaseStruct * DuplicateStruct(EDA_BaseStruct *DrawStruct);
void MoveOneStruct(EDA_BaseStruct *DrawStructs, const wxPoint & move_vector);
						/* Given a structure move it by move_vector.x, move_vector.y. */

bool PlaceStruct(BASE_SCREEN * screen, EDA_BaseStruct *DrawStruct);
bool MoveStruct(WinEDA_DrawPanel * panel, wxDC * DC, EDA_BaseStruct *DrawStruct);
void DeleteStruct(WinEDA_DrawPanel * panel, wxDC * DC, EDA_BaseStruct *DrawStruct);
bool DrawStructInBox(int x1, int y1, int x2, int y2,
						EDA_BaseStruct *DrawStruct);

	/*************/
	/* LOCATE.CPP */
	/*************/

EDA_SchComponentStruct * LocateSmallestComponent( SCH_SCREEN * Screen );
/* Recherche du plus petit (en surface) composant pointe par la souris */

EDA_BaseStruct * PickStruct(EDA_Rect & block,
		EDA_BaseStruct *DrawList, int SearchMask );
EDA_BaseStruct * PickStruct(const wxPoint & refpos,
		EDA_BaseStruct *DrawList, int SearchMask );
/* 2 functions EDA_BaseStruct * PickStruct:
	Search in  block, or Serach at location pos

	SearchMask = (bitwise OR):
	LIBITEM
	WIREITEM
	BUSITEM
	RACCORDITEM
	JUNCTIONITEM
	DRAWITEM
	TEXTITEM
	LABELITEM
	SHEETITEM
	MARKERITEM
	NOCONNECTITEM
	SEARCH_PINITEM
	SHEETLABELITEM
	FIELDCMPITEM

	if EXCLUDE_WIRE_BUS_ENDPOINTS is set, in wire ou bus search and locate,
	start and end points are not included in search
	if WIRE_BUS_ENDPOINTS_ONLY is set, in wire ou bus search and locate,
	only start and end points are included in search


	Return:
		-Bloc searc:
			pointeur sur liste de pointeurs de structures si Plusieurs
					structures selectionnees.
			pointeur sur la structure si 1 seule

		Positon serach:
			pointeur sur la structure.
		Si pas de structures selectionnees: retourne NULL */



LibEDA_BaseStruct * LocateDrawItem(SCH_SCREEN * Screen,
		EDA_LibComponentStruct* LibEntry, int Unit, int Convert, int masque);

DrawSheetLabelStruct * LocateSheetLabel(DrawSheetStruct *Sheet, const wxPoint & pos);
LibDrawPin * LocateAnyPin(EDA_BaseStruct *DrawList, const wxPoint & RefPos,
		EDA_SchComponentStruct ** libpart = NULL );

DrawSheetLabelStruct * LocateAnyPinSheet(const wxPoint & RefPos,
					EDA_BaseStruct *DrawList);

int distance(int dx, int dy, int spot_cX, int spot_cY, int seuil);
	/* Calcul de la distance du point spot_cx,spot_cy a un segment de droite,
		d'origine 0,0 et d'extremite dx, dy;
		retourne:
			 0 si distance > seuil
			 1 si distance <= seuil
		 Variables utilisees ( sont ramenees au repere centre sur l'origine du segment)
			 dx, dy = coord de l'extremite segment.
			 spot_cX,spot_cY = coord du curseur souris
		 la recherche se fait selon 4 cas:
			 segment horizontal
			 segment vertical
			 segment quelconque */

/***************/
/* EEREDRAW.CPP */
/***************/
void  DrawDanglingSymbol(WinEDA_DrawPanel * panel,wxDC * DC,
			const wxPoint & pos, int Color);

void Draw_Marqueur(WinEDA_DrawPanel * panel, wxDC * DC,
			wxPoint pos, char* pt_bitmap, int DrawMode, int Color);

void DrawStructsInGhost(WinEDA_DrawPanel * panel, wxDC * DC,
									EDA_BaseStruct * DrawStruct, int dx, int dy );
void SetHighLightStruct(EDA_BaseStruct *HighLight);
void RedrawActiveWindow(WinEDA_DrawPanel * panel, wxDC * DC);
void RedrawStructList(WinEDA_DrawPanel * panel, wxDC * DC, EDA_BaseStruct *Structs, int DrawMode,
									int Color = -1);
void RedrawOneStruct(WinEDA_DrawPanel * panel, wxDC * DC, EDA_BaseStruct *Struct, int DrawMode,
									int Color = -1);

/**************/
/* EELAYER.CPP */
/**************/
void SeedLayers(void);
int ReturnLayerColor(int Layer);
void DisplayColorSetupFrame(WinEDA_DrawFrame * parent, const wxPoint & pos);

/*************/
/* EELOAD.CPP */
/*************/
int CountCmpNumber(void);


/***************/
/* EESTRING.CPP */
/***************/

void PutTextInfo(WinEDA_DrawPanel * panel, wxDC * DC, int Orient, const wxPoint & PosX,
							const wxSize& size,
							const wxString & Str, int DrawMode, int color);

/***************/
/* EECONFIG.CPP */
/***************/
bool Read_Config( const wxString & CfgFileName, bool ForceRereadConfig );


/**************/
/* SAVELIB.CPP */
/**************/

LibEDA_BaseStruct * CopyDrawEntryStruct( wxWindow * frame, LibEDA_BaseStruct * DrawItem);
	/* Routine de Duplication d'une structure DrawLibItem d'une partlib
		 Parametres d'entree:
			 DrawEntry = pointeur sur la structure a dupliquer
		 La structure nouvelle est creee, mais n'est pas inseree dans le
		 chainage
		 Retourne:
			 Pointeur sur la structure creee (ou NULL si impossible) */

int WriteOneLibEntry(wxWindow * frame, FILE * ExportFile, EDA_LibComponentStruct * LibEntry);
			/* Routine d'ecriture du composant pointe par LibEntry
				dans le fichier ExportFile( qui doit etre deja ouvert)
		 		return: FALSE si Ok, TRUE si err write */

EDA_LibComponentStruct * CopyLibEntryStruct (wxWindow * frame, EDA_LibComponentStruct * OldEntry);
			/* Routine de copie d'une partlib
				   Parametres d'entree: pointeur sur la structure de depart
				   Parametres de sortie: pointeur sur la structure creee */

int WriteOneDocLibEntry(FILE * ExportFile, EDA_LibComponentStruct * LibEntry);
		/* Routine d'ecriture de la doc du composant pointe par LibEntry
		 dans le fichier ExportFile( qui doit etre deja ouvert)
		 return: 0 si Ok
			1 si err write */


int SaveOneLibrary(wxWindow * frame, const wxString & FullFileName, LibraryStruct * Library);
	/* Sauvegarde en fichier la librairie pointee par Library, sous le nom
	FullFileName.
	2 fichiers sont crees
	 - La librarie
	 - le fichier de documentation

	une sauvegarde .bak de l'ancien fichier librairie est cree
	une sauvegarde .bck de l'ancien fichier documentation est cree

	return:
		0 si OK
		1 si erreur */


/***************/
/* SYMBEDIT.CPP */
/***************/
void SuppressDuplicateDrawItem(EDA_LibComponentStruct * LibEntry);
		/* Routine de suppression des elements de trace dupliques, situation
		frequente lorsque l'on charge des symboles predessines plusieurs fois
		pour definir un composant */

/***************/
/* SYMBTEXT.CPP */
/***************/


/**************/
/* NETLIST.CPP */
/**************/
int IsBusLabel(const wxString & LabelDrawList);
void InstallNetlistFrame(WinEDA_SchematicFrame *parent, wxPoint &pos);

/***************/
/* ANNOTATE.CPP */
/***************/
void ReAnnotatePowerSymbolsOnly( void );

void InstallAnnotateFrame(WinEDA_SchematicFrame * parent, wxPoint &pos);
int CheckAnnotate(WinEDA_SchematicFrame * frame, bool OneSheetOnly);
				/* Retourne le nombre de composants non annotes ou erronés
					Si OneSheetOnly : recherche sur le schema courant
					else: recherche sur toute la hierarchie */


/************/
/* PLOT.CPP */
/************/
void PlotArc(wxPoint centre, int StAngle, int EndAngle, int rayon);
void PlotCercle(wxPoint centre, int diametre );

void PlotNoConnectStruct(DrawNoConnectStruct * Struct);
void PlotLibPart( EDA_SchComponentStruct *DrawLibItem );
					/* Genere le trace d'un composant */
void PlotSheetStruct(DrawSheetStruct *Struct);
					/* Routine de dessin du bloc type hierarchie */
void PlotTextStruct(EDA_BaseStruct *Struct);

/***************/
/* DELSHEET.CPP */
/***************/
void DeleteSubHierarchy(DrawSheetStruct * Sheet, bool confirm_deletion);
void ClearDrawList(EDA_BaseStruct *DrawList, bool confirm_deletion); /* free the draw list DrawList and the subhierarchies */
bool ClearProjectDrawList(SCH_SCREEN * FirstWindow, bool confirm_deletion);
/* free the draw list screen->EEDrawList and the subhierarchies
	clear the screen datas (filenames ..)
*/

/*************/
/* DELETE.CPP */
/*************/

void LocateAndDeleteItem(WinEDA_SchematicFrame * frame, wxDC * DC);
void EraseStruct(EDA_BaseStruct *DrawStruct, SCH_SCREEN * Window);
void DeleteAllMarkers(int type);
						/* Effacement des marqueurs du type "type" */

void DeleteOneLibraryDrawStruct(WinEDA_DrawPanel * panel,
							wxDC *DC, EDA_LibComponentStruct * LibEntry,
							LibEDA_BaseStruct * DrawItem, int Affiche);
	/* Routine d'effacement d'un "LibraryDrawStruct"
		(d'un element de dessin d'un composant )
		 Parametres d'entree
			Pointeur sur le composant comportant la structure
				 (Si NULL la structure a effacer est supposee non rattachee
				a un composant)
			Pointeur sur la structure a effacer
		 	Affiche (si != 0 Efface le graphique correspondant de l'ecran) */


/**********/
/* ERC.CPP */
/**********/
void InstallErcFrame(WinEDA_SchematicFrame *parent, wxPoint & pos);


/**************/
/* GETPART.CPP */
/**************/

int LookForConvertPart( EDA_LibComponentStruct * LibEntry );
	/* Retourne la plus grande valeur trouvee dans la liste des elements
		 "drawings" du composant LibEntry, pour le membre .Convert
		 Si il n'y a pas de representation type "convert", la valeur
		 retournee est 0 ou 1
		 Si il y a une representation type "convert",
		 la valeur retournee est > 1 (typiquement 2) */


/**************/
/* PINEDIT.CPP */
/**************/
void InstallPineditFrame(WinEDA_LibeditFrame * parent, wxDC * DC, const wxPoint & pos);


/**************/
/* SELPART.CPP */
/**************/

int DisplayComponentsNamesInLib(WinEDA_DrawFrame * frame,
				 LibraryStruct *Library, wxString & Buffer, wxString &  OldName);
LibraryStruct * SelectLibraryFromList(WinEDA_DrawFrame * frame);
		/* Routine pour selectionner une librairie a partir d'une liste */

int GetNameOfPartToLoad(WinEDA_DrawFrame * frame, LibraryStruct * Lib,
		wxString & BufName);
	/* Routine de selection du nom d'un composant en librairie pour chargement,
	dans la librairie Library.
	Si Library == NULL, il y aura demande de selection d'une librairie
	Retourne
	1 si composant selectionne
	0 si commande annulee
	place le nom du composant a charger, selectionne a partir d'une liste dans
	BufName */

	/**************/
	/* LIBARCH.CPP */
	/**************/

bool LibArchive(wxWindow * frame, const wxString & ArchFullFileName);

	/***************/
	/* GENLISTE.CPP */
	/***************/
void InstallToolsFrame(WinEDA_DrawFrame *parent, wxPoint &pos);
int GenListeCmp( EDA_BaseStruct ** List );

	/**************/
	/* CLEANUP.CPP */
	/**************/

void SchematicCleanUp(SCH_SCREEN * screen, wxDC * DC);
	/* Routine de nettoyage:
		 - regroupe les segments de fils (ou de bus) alignes en 1 seul segment
		 - Detecte les objets identiques superposes
	*/

void BreakSegmentOnJunction( SCH_SCREEN * Screen );
	/* Routine creant des debuts / fin de segment (BUS ou WIRES) sur les jonctions
		et les raccords */
DrawPickedStruct * BreakSegment(SCH_SCREEN * screen, wxPoint breakpoint,
			bool PutInUndoList = FALSE);
	/* Coupe un segment ( BUS, WIRE ) en 2 au point breakpoint,
		- si ce point est sur le segment
		- extremites non comprises */

	/**************/
	/* EECLASS.CPP */
	/**************/

void SetStructFather(EDA_BaseStruct * Struct, BASE_SCREEN * Screen);

	/***************/
	/* LIBALIAS.CPP */
	/***************/

bool BuildAliasData(LibraryStruct * Lib, EDA_LibComponentStruct * component);
	/* Create the alias data for the lib component to edit */
int LocateAlias( const wxArrayString & AliasData, const wxString & Name);
	/* Return an index in alias data list ( -1 if not found ) */


/************/
/* FIND.CPP */
/************/
void InstallFindFrame(WinEDA_SchematicFrame *parent, wxPoint &pos);


/***************/
/* OPTIONS.CPP */
/***************/
void DisplayOptionFrame(WinEDA_DrawFrame * parent, const wxPoint & framepos);


