/*******************************************************/
/* class_edge_module.h : EDGE_MODULE class definition. */
/*******************************************************/

class Pcb3D_GLCanvas;


class EDGE_MODULE : public BOARD_ITEM
{
public:
    int     m_Width;        // 0 = line, > 0 = tracks, bus ...
    wxPoint m_Start;        // Line start point / circle and arc center
    wxPoint m_End;          // Line end point / circle and arc starting point

    int     m_Shape;        // enum Track_Shapes
    wxPoint m_Start0;       // Start point or centre, relative to module origin, orient 0.
    wxPoint m_End0;         // End point, relative to module origin, orient 0.

    int     m_Angle;        // Arcs: angle in 0.1 degrees

    std::vector<wxPoint> m_PolyPoints;   /* For polygons: number of points (> 2)
                                          *  Coord are relative to Origin, orient 0
                                          *  m_Start0 and m_End0 are not used for polygons
                                          */

public:
    EDGE_MODULE( MODULE* parent );
    EDGE_MODULE( EDGE_MODULE* edge );
    ~EDGE_MODULE();

    EDGE_MODULE* Next() const { return (EDGE_MODULE*) Pnext; }
    EDGE_MODULE* Back() const { return (EDGE_MODULE*) Pback; }


    /**
     * Function GetPosition
     * returns the position of this object.
     * @return const wxPoint& - The position of this object.
     */
    wxPoint& GetPosition()
    {
        return m_Start;
    }


    void             Copy( EDGE_MODULE* source ); // copy structure

    /**
     * Function Save
     * writes the data structures for this object out to a FILE in "*.brd" format.
     * @param aFile The FILE to write to.
     * @return bool - true if success writing else false.
     */
    bool             Save( FILE* aFile ) const;

    int              ReadDescr( char* Line, FILE* File, int* LineNum = NULL );

    void             SetDrawCoord();

    /* drawing functions */
    void             Draw( WinEDA_DrawPanel* panel, wxDC* DC,
                           int aDrawMode, const wxPoint& offset = ZeroOffset );

    void             Draw3D( Pcb3D_GLCanvas* glcanvas );

    /**
     * Function DisplayInfo
     * has knowledge about the frame and how and where to put status information
     * about this object into the frame's message panel.
     * Is virtual from EDA_BaseStruct.
     * @param frame A WinEDA_DrawFrame in which to print status information.
     */
    void             DisplayInfo( WinEDA_DrawFrame* frame );


    /**
     * Function GetBoundingBox
     * returns the orthogonal, bounding box of this object for display purposes.
     * This box should be an enclosing perimeter for visible components of this
     * object, and the units should be in the pcb or schematic coordinate system.
     * It is OK to overestimate the size by a few counts.
     */
    virtual EDA_Rect GetBoundingBox();

    /**
     * Function HitTest
     * tests if the given wxPoint is within the bounds of this object.
     * @param refPos A wxPoint to test
     * @return bool - true if a hit, else false
     */
    bool             HitTest( const wxPoint& refPos );

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
     * Function TransformShapeWithClearanceToPolygon
     * Convert the track shape to a closed polygon
     * Used in filling zones calculations
     * Circles and arcs are approximated by segments
     * @param aCornerBuffer = a buffer to store the polygon
     * @param aClearanceValue = the clearance around the pad
     * @param aCircleToSegmentsCount = the number of segments to approximate a circle
     * @param aCorrectionFactor = the correction to apply to circles radius to keep
     * clearance when the circle is approxiamted by segment bigger or equal
     * to the real clearance value (usually near from 1.0)
     */
    void TransformShapeWithClearanceToPolygon(
        std::vector <CPolyPt>& aCornerBuffer,
        int                    aClearanceValue,
        int
                               aCircleToSegmentsCount,
        double                 aCorrectionFactor );

#if defined(DEBUG)

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
