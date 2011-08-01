/*******************************************************/
/* class_edge_module.h : EDGE_MODULE class definition. */
/*******************************************************/

#ifndef CLASS_DRAWSEGMENT_H
#define _CLASS_EDGE_MOD_H_

#include "richio.h"

class Pcb3D_GLCanvas;


class EDGE_MODULE : public BOARD_ITEM
{
public:
    int     m_Width;        // 0 = line, > 0 = tracks, bus ...
    wxPoint m_Start;        // Line start point and circle and arc center
    wxPoint m_End;          // Line end point and circle and arc starting point

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

    /**
     * Function GetStart
     * returns the starting point of the graphic
     */
    wxPoint      GetStart() const;

    /**
     * Function GetEnd
     * returns the ending point of the graphic
     */
    wxPoint      GetEnd() const;

    /**
     * Function GetRadius
     * returns the radius of this item
     * Has meaning only for arc and circle
     */
    int         GetRadius() const
    {
        double radius = hypot( (double) (m_End.x - m_Start.x), (double) (m_End.y - m_Start.y) );
        return wxRound( radius );
    }

    void             Copy( EDGE_MODULE* source ); // copy structure

    /**
     * Function Save
     * writes the data structures for this object out to a FILE in "*.brd" format.
     * @param aFile The FILE to write to.
     * @return bool - true if success writing else false.
     */
    bool             Save( FILE* aFile ) const;

    int              ReadDescr( LINE_READER* aReader );

    void             SetDrawCoord();

    /* drawing functions */
    void             Draw( EDA_DRAW_PANEL* panel, wxDC* DC,
                           int aDrawMode, const wxPoint& offset = ZeroOffset );

    void             Draw3D( Pcb3D_GLCanvas* glcanvas );

    /**
     * Function DisplayInfo
     * has knowledge about the frame and how and where to put status information
     * about this object into the frame's message panel.
     * Is virtual from EDA_ITEM.
     * @param frame A EDA_DRAW_FRAME in which to print status information.
     */
    void             DisplayInfo( EDA_DRAW_FRAME* frame );


    /**
     * Function GetBoundingBox
     * returns the orthogonal, bounding box of this object for display purposes.
     * This box should be an enclosing perimeter for visible components of this
     * object, and the units should be in the pcb or schematic coordinate system.
     * It is OK to overestimate the size by a few counts.
     */
    virtual EDA_RECT GetBoundingBox() const;

    /**
     * Function HitTest
     * tests if the given wxPoint is within the bounds of this object.
     * @param refPos A wxPoint to test
     * @return bool - true if a hit, else false
     */
    bool             HitTest( const wxPoint& refPos );

    /**
     * Function HitTest (overlayed)
     * tests if the given EDA_RECT intersect this object.
     * For now, for segments and arcs, an ending point must be inside this rect.
     * @param refArea the given EDA_RECT to test
     * @return bool - true if a hit, else false
     */
    bool         HitTest( EDA_RECT& refArea );

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
     * clearance when the circle is approximated by segment bigger or equal
     * to the real clearance value (usually near from 1.0)
     */
    void TransformShapeWithClearanceToPolygon( std::vector <CPolyPt>& aCornerBuffer,
                                               int                    aClearanceValue,
                                               int                    aCircleToSegmentsCount,
                                               double                 aCorrectionFactor );

    virtual wxString GetSelectMenuText() const;

    virtual const char** GetMenuImage() const { return (const char**) show_mod_edge_xpm; }

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

#endif    // _CLASS_EDGE_MOD_H_
