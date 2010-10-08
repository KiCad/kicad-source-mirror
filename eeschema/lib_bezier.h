
#ifndef _LIB_BEZIER_H_
#define _LIB_BEZIER_H_


#include "lib_draw_item.h"


/**************************************************/
/* Graphic Body Item: Bezier Curve (set of lines) */
/**************************************************/
class LIB_BEZIER : public LIB_DRAW_ITEM
{
public:
    int m_Width;                            /* Line width */
    std::vector<wxPoint> m_BezierPoints;      // list of parameter (3|4)
    std::vector<wxPoint> m_PolyPoints;      // list of points (>= 2)

public:
    LIB_BEZIER( LIB_COMPONENT * aParent );
    LIB_BEZIER( const LIB_BEZIER& aBezier );
    ~LIB_BEZIER() { }

    virtual wxString GetClass() const
    {
        return wxT( "LIB_BEZIER" );
    }


    /**
     * Write bezier curve object out to a FILE in "*.lib" format.
     *
     * @param aFile - The FILE to write to.
     * @return true if success writing else false.
     */
    virtual bool Save( FILE* aFile );
    virtual bool Load( char* aLine, wxString& aErrorMsg );

    void         AddPoint( const wxPoint& aPoint );

    /**
     * @return the number of corners
     */
    unsigned GetCornerCount() const { return m_PolyPoints.size(); }

    /**
     * Test if the given point is within the bounds of this object.
     *
     * @param aRefPos - A wxPoint to test
     * @return true if a hit, else false
     */
    virtual bool HitTest( const wxPoint& aRefPos );

    /**
     * @param aPosRef = a wxPoint to test
     * @param aThreshold = max distance to a segment
     * @param aTransMat = the transform matrix
     * @return true if the point aPosRef is near a segment
     */
    virtual bool HitTest( wxPoint aPosRef, int aThreshold, const int aTransMat[2][2] );

    /**
     * @return the boundary box for this, in library coordinates
     */
    virtual EDA_Rect GetBoundingBox();

    /**
     * @return the size of the "pen" that be used to draw or plot this item
     */
    virtual int GetPenSize( );

    void Draw( WinEDA_DrawPanel * aPanel, wxDC * aDC, const wxPoint &aOffset,
               int aColor, int aDrawMode, void* aData,
               const int aTransformMatrix[2][2] );

    virtual void DisplayInfo( WinEDA_DrawFrame* aFrame );

protected:
    virtual LIB_DRAW_ITEM* DoGenCopy();

    /**
     * Provide the bezier curve draw object specific comparison.
     *
     * The sort order for each bezier curve segment point is as follows:
     *      - Bezier point horizontal (X) point position.
     *      - Bezier point vertical (Y) point position.
     */
    virtual int DoCompare( const LIB_DRAW_ITEM& aOther ) const;

    virtual void DoOffset( const wxPoint& aOffset );
    virtual bool DoTestInside( EDA_Rect& aRect );
    virtual void DoMove( const wxPoint& aPosition );
    virtual wxPoint DoGetPosition() { return m_PolyPoints[0]; }
    virtual void DoMirrorHorizontal( const wxPoint& aCenter );
    virtual void DoPlot( PLOTTER* aPlotter, const wxPoint& aOffset, bool aFill,
                         const int aTransform[2][2] );
    virtual int DoGetWidth() { return m_Width; }
    virtual void DoSetWidth( int aWidth ) { m_Width = aWidth; }
};


#endif     // _LIB_BEZIER_H_
