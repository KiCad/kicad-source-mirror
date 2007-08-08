/**************************************************************/
/* class_edge_module.h : description des contours d'un module */
/**************************************************************/

class Pcb3D_GLCanvas;


/* description des contours (empreintes ) et TYPES des CONTOURS : */

class EDGE_MODULE : public EDA_BaseLineStruct
{
public:
    int     m_Shape;        // voir "enum Track_Shapes"
    wxPoint m_Start0;       // coord relatives a l'ancre du point de depart(Orient 0)
    wxPoint m_End0;         // coord relatives a l'ancre du point de fin (Orient 0)
    
    int     m_Angle;        // pour les arcs de cercle: longueur de l'arc en 0,1 degres
    
    int     m_PolyCount;    // For polygons: number of points (> 2)
    int*    m_PolyList;     // For polygons: coord list (1 point = 2 coord)
                            // Coord are relative to Origin, orient 0

public:
    EDGE_MODULE( MODULE* parent );
    EDGE_MODULE( EDGE_MODULE* edge );
    ~EDGE_MODULE();

    /* supprime du chainage la structure Struct */
    void    UnLink( void );

    void    Copy( EDGE_MODULE* source );    // copy structure

    /* Reading and writing data on files */
    int     WriteDescr( FILE* File );
    int     ReadDescr( char* Line, FILE* File, int* LineNum = NULL );

    // Mise a jour des coordon�s pour l'affichage
    void    SetDrawCoord( void );

    /* drawing functions */
    void    Draw( WinEDA_DrawPanel* panel, wxDC* DC, const wxPoint& offset,
                  int draw_mode );
    void    Draw3D( Pcb3D_GLCanvas* glcanvas );

    /**
     * Function HitTest
     * tests if the given wxPoint is within the bounds of this object.
     * @param refPos A wxPoint to test
     * @return bool - true if a hit, else false
     */
    bool    HitTest( const wxPoint& refPos );
    
#if defined(DEBUG)
    /**
     * Function GetClass
     * returns the class name.
     * @return wxString
     */
    virtual wxString GetClass() const
    {
        return wxT( "MGRAPHIC" );
        // return wxT( "EDGE" );  ?
    }

    /**
     * Function Show
     * is used to output the object tree, currently for debugging only.
     * @param nestLevel An aid to prettier tree indenting, and is the level 
     *          of nesting of this object within the overall tree.
     * @param os The ostream& to output to.
     */
    virtual void Show( int nestLevel, std::ostream& os );
#endif
};
