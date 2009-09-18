//*****************************************/
/* prototypage des fonctions de EESchema */
/*****************************************/

LibEDA_BaseStruct* LocatePin( const wxPoint& RefPos,
                              LIB_COMPONENT* Entry,
                              int            Unit,
                              int            Convert,
                              SCH_COMPONENT* DrawItem = NULL );

/* Routine de localisation d'une PIN de la PartLib pointee par Entry */

wxString ReturnDefaultFieldName( int aFieldNdx );


/****************/
/* DATABASE.CPP */
/****************/
void DisplayCmpDoc( wxString& Name );
wxString DataBaseGetName( WinEDA_DrawFrame* frame, wxString& Keys,
                          wxString& BufName );

/*********************/
/* DANGLING_ENDS.CPP */
/*********************/
bool SegmentIntersect( int Sx1, int Sy1, int Sx2, int Sy2, int Px1, int Py1 );

/****************/
/* BUS_WIRE_JUNCTION.CPP */
/****************/
void IncrementLabelMember( wxString& name );

/****************/
/* EDITPART.CPP */
/****************/
void InstallCmpeditFrame( WinEDA_SchematicFrame* parent, wxPoint& pos,
                          SCH_COMPONENT* m_Cmp );


/******************************/
/* EELIBS_DRAW_COMPONENTS.CPP */
/******************************/
void DrawingLibInGhost( WinEDA_DrawPanel* panel,
                        wxDC*             DC,
                        LIB_COMPONENT*    LibEntry,
                        SCH_COMPONENT*    DrawLibItem,
                        int               PartX,
                        int               PartY,
                        int               Multi,
                        int               convert,
                        int               Color,
                        bool              DrawPinText );

bool MapAngles( int*      Angle1,
                int*      Angle2,
                const int TransMat[2][2] );


/**
 * Calculate new coordinate according to the transform matrix.
 *
 * @param aTransformMatrix = rotation, mirror .. matrix
 * @param aPosition = the position to transform
 *
 * @return the new coordinate
 */
wxPoint        TransformCoordinate( const int      aTransformMatrix[2][2],
                                    const wxPoint& aPosition );

void           SnapLibItemPoint( int            OrigX,
                                 int            OrigY,
                                 int*           ClosestX,
                                 int*           ClosestY,
                                 SCH_COMPONENT* DrawLibItem );
bool           LibItemInBox( int x1, int y1, int x2, int y2,
                             SCH_COMPONENT* DrawLibItem );
char*          StrPurge( char* text );

/* Supprime les caracteres Space en debut de la ligne text
 *  retourne un pointeur sur le 1er caractere non Space de text */

/************/
/* BLOCK.CPP */
/************/
SCH_ITEM* DuplicateStruct( SCH_ITEM* DrawStruct );
void      DeleteStruct( WinEDA_DrawPanel* panel,
                        wxDC*             DC,
                        SCH_ITEM*         DrawStruct );

/*************/
/* LOCATE.CPP */
/*************/
LibDrawPin*        LocatePinByNumber( const wxString& ePin_Number,
                                      SCH_COMPONENT*  eComponent );

SCH_COMPONENT*     LocateSmallestComponent( SCH_SCREEN* Screen );

/* Recherche du plus petit (en surface) composant pointe par la souris */

int                PickItemsInBlock( BLOCK_SELECTOR& aBlock,
                                     BASE_SCREEN*    screen );

/* function PickStruct:
 *   Search at location pos
 *
 *   SearchMask = (bitwise OR):
 *   LIBITEM
 *   WIREITEM
 *   BUSITEM
 *   RACCORDITEM
 *   JUNCTIONITEM
 *   DRAWITEM
 *   TEXTITEM
 *   LABELITEM
 *   SHEETITEM
 *   MARKERITEM
 *   NOCONNECTITEM
 *   SEARCH_PINITEM
 *   SHEETLABELITEM
 *   FIELDCMPITEM
 *
 *   if EXCLUDE_WIRE_BUS_ENDPOINTS is set, in wire ou bus search and locate,
 *   start and end points are not included in search
 *   if WIRE_BUS_ENDPOINTS_ONLY is set, in wire ou bus search and locate,
 *   only start and end points are included in search
 *
 *
 *   Return:
 *       -Bloc search:
 *           pointeur sur liste de pointeurs de structures si Plusieurs
 *                   structures selectionnees.
 *           pointeur sur la structure si 1 seule
 *
 *       Positon search:
 *           pointeur sur la structure.
 *       Si pas de structures selectionnees: retourne NULL */
SCH_ITEM*          PickStruct( const wxPoint& refpos,
                               BASE_SCREEN*   screen,
                               int            SearchMask );


LibEDA_BaseStruct* LocateDrawItem( SCH_SCREEN*    Screen,
                                   const wxPoint& refpoint,
                                   LIB_COMPONENT* LibEntry,
                                   int            Unit,
                                   int            Convert,
                                   int            masque );

Hierarchical_PIN_Sheet_Struct* LocateSheetLabel( DrawSheetStruct* Sheet,
                                                 const wxPoint&   pos );
LibDrawPin*                    LocateAnyPin( SCH_ITEM*       DrawList,
                                             const wxPoint&  RefPos,
                                             SCH_COMPONENT** libpart = NULL );

Hierarchical_PIN_Sheet_Struct* LocateAnyPinSheet( const wxPoint& RefPos,
                                                  SCH_ITEM*      DrawList );


/***************/
/* EEREDRAW.CPP */
/***************/
void DrawDanglingSymbol( WinEDA_DrawPanel* panel, wxDC* DC,
                         const wxPoint& pos, int Color );

void DrawStructsInGhost( WinEDA_DrawPanel* aPanel,
                         wxDC*             aDC,
                         SCH_ITEM*         aItem,
                         const wxPoint&    aOffset );
void SetHighLightStruct( SCH_ITEM* HighLight );
void RedrawActiveWindow( WinEDA_DrawPanel* panel, wxDC* DC );
void RedrawStructList( WinEDA_DrawPanel* panel,
                       wxDC*             DC,
                       SCH_ITEM*         Structs,
                       int               DrawMode,
                       int               Color = -1 );
void RedrawOneStruct( WinEDA_DrawPanel* panel,
                      wxDC*             DC,
                      SCH_ITEM*         Struct,
                      int               DrawMode,
                      int               Color = -1 );

/**************/
/* EELAYER.CPP */
/**************/
void       SeedLayers();
EDA_Colors ReturnLayerColor( int Layer );
void       DisplayColorSetupFrame( WinEDA_DrawFrame* parent,
                                   const wxPoint&    pos );

/*************/
/* EELOAD.CPP */
/*************/
int  CountCmpNumber();

/***************/
/* EECONFIG.CPP */
/***************/
bool Read_Hotkey_Config( WinEDA_DrawFrame* frame, bool verbose );


/**************/
/* SAVELIB.CPP */
/**************/

LIB_COMPONENT* CopyLibEntryStruct( LIB_COMPONENT* OldEntry );

/* Routine de copie d'une partlib
 *      Parametres d'entree: pointeur sur la structure de depart
 *      Parametres de sortie: pointeur sur la structure creee */


/**************/
/* NETLIST.CPP */
/**************/
int  IsBusLabel( const wxString& LabelDrawList );

/***************/
/* ANNOTATE.CPP */
/***************/
void ReAnnotatePowerSymbolsOnly();


/************/
/* PLOT.CPP */
/************/
void PlotDrawlist( PLOTTER* plotter, SCH_ITEM* drawlist );

/***************/
/* DELSHEET.CPP */
/***************/
void DeleteSubHierarchy( DrawSheetStruct* Sheet, bool confirm_deletion );
bool ClearProjectDrawList( SCH_SCREEN* FirstWindow, bool confirm_deletion );

/* free the draw list screen->EEDrawList and the subhierarchies
 *   clear the screen datas (filenames ..)
 */

/*************/
/* DELETE.CPP */
/*************/

bool LocateAndDeleteItem( WinEDA_SchematicFrame* frame, wxDC* DC );
void EraseStruct( SCH_ITEM* DrawStruct, SCH_SCREEN* Window );
void DeleteAllMarkers( int type );


/**************/
/* GETPART.CPP */
/**************/

int LookForConvertPart( LIB_COMPONENT* LibEntry );

/* Retourne la plus grande valeur trouvee dans la liste des elements
 *    "drawings" du composant LibEntry, pour le membre .Convert
 *    Si il n'y a pas de representation type "convert", la valeur
 *    retournee est 0 ou 1
 *    Si il y a une representation type "convert",
 *    la valeur retournee est > 1 (typiquement 2) */


/**************/
/* PINEDIT.CPP */
/**************/
void InstallPineditFrame( WinEDA_LibeditFrame* parent,
                          wxDC*                DC,
                          const wxPoint&       pos );


/**************/
/* SELPART.CPP */
/**************/

/**
 * Function DisplayComponentsNamesInLib
 * Routine de selection d'un composant en librairie, par affichage de la
 *   liste des composants de cette librairie
 *   Si Library == NULL, selection de librairie demandee
 *   sinon recherche uniquement dans library
 *   Retourne
 *       1 si composant selectionne
 *       0 si commande annulee
 */
int            DisplayComponentsNamesInLib( WinEDA_DrawFrame* frame,
                                            CMP_LIBRARY*      Library,
                                            wxString&         Buffer,
                                            wxString&         OldName );

/**
 * Function SelectLibraryFromList
 * displays a list of current loaded libraries, and allows the user to select
 * a library
 * This list is sorted, with the library cache always at end of the list
 */
CMP_LIBRARY* SelectLibraryFromList( WinEDA_DrawFrame* frame );

/**
 * Function GetNameOfPartToLoad
 *   Routine de selection du nom d'un composant en librairie pour chargement,
 *   dans la librairie Library.
 *   Si Library == NULL, il y aura demande de selection d'une librairie
 *  Retourne
 *   1 si composant selectionne
 *   0 si commande annulee
 *   place le nom du composant a charger, selectionne a partir d'une liste dans
 *   BufName
 */
int            GetNameOfPartToLoad( WinEDA_DrawFrame* frame,
                                    CMP_LIBRARY*      Lib,
                                    wxString&         BufName );

/**************/
/* LIBARCH.CPP */
/**************/

bool LibArchive( wxWindow* frame, const wxString& ArchFullFileName );

/**************/
/* CLEANUP.CPP */
/**************/

void SchematicCleanUp( SCH_SCREEN* screen, wxDC* DC );

/* Routine de nettoyage:
 *    - regroupe les segments de fils (ou de bus) alignes en 1 seul segment
 *    - Detecte les objets identiques superposes
 */

void BreakSegmentOnJunction( SCH_SCREEN* Screen );

/* Routine creant des debuts / fin de segment (BUS ou WIRES) sur les jonctions
 *   et les raccords */

/* Break a segment ( BUS, WIRE ) int 2 segments at location aBreakpoint,
 * if aBreakpoint in on segment segment
 * ( excluding ends)
 */
void BreakSegment(SCH_SCREEN * aScreen, wxPoint aBreakpoint );

/**************/
/* EECLASS.CPP */
/**************/

void SetaParent( EDA_BaseStruct* Struct, BASE_SCREEN* Screen );

/***************/
/* OPTIONS.CPP */
/***************/
void DisplayOptionFrame( WinEDA_SchematicFrame* parent,
                         const wxPoint&         framepos );

/****************/
/* CONTROLE.CPP */
/****************/
void RemoteCommand( const char* cmdline );
