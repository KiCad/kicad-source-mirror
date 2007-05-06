	/**************************************************************/
	/* class_edge_module.h : description des contours d'un module */
	/**************************************************************/

class Pcb3D_GLCanvas;


/* description des contours (empreintes ) et TYPES des CONTOURS : */

class EDGE_MODULE: public EDA_BaseLineStruct
	{
	public:
	int m_Shape ;			// voir "enum Track_Shapes" 
	wxPoint m_Start0;		// coord relatives a l'ancre du point de depart(Orient 0)
	wxPoint m_End0;			// coord relatives a l'ancre du point de fin (Orient 0)
	int m_Angle;			// pour les arcs de cercle: longueur de l'arc en 0,1 degres
	int m_PolyCount;		// For polygons : number of points (> 2)
	int * m_PolyList;		// For polygons: coord list (1 point = 2 coord)
							// Coord are relative to Origine, orient 0

	public:
	EDGE_MODULE(MODULE * parent );
	EDGE_MODULE(EDGE_MODULE * edge );
	~EDGE_MODULE();

	/* supprime du chainage la structure Struct */
	void UnLink( void );

    void Copy(EDGE_MODULE * source);		// copy structure

	/* Readind and writing data on files */
	int WriteDescr( FILE * File );
	int ReadDescr( char * Line, FILE * File, int * LineNum = NULL);

	// Mise a jour des coordonées pour l'affichage
	void SetDrawCoord(void);

	/* drawing functions */
	void Draw(WinEDA_DrawPanel * panel, wxDC * DC, const wxPoint & offset,
							int draw_mode);
	void Draw3D(Pcb3D_GLCanvas * glcanvas);
};

