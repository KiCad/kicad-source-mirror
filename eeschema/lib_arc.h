/**************************/
/* Graphic Body Item: Arc */
/**************************/

#ifndef _LIB_ARC_H_
#define _LIB_ARC_H_


#include "lib_draw_item.h"


class LIB_ARC : public LIB_DRAW_ITEM
{
public:
    int     m_Radius;
    int     m_t1;       /* First radius angle of the arc in 0.1 degrees. */
    int     m_t2;       /* Second radius angle of the arc in 0.1 degrees. */
    wxPoint m_ArcStart;
    wxPoint m_ArcEnd;   /* Arc end position. */
    wxPoint m_Pos;      /* Radius center point. */
    int     m_Width;    /* Line width */

public:
    LIB_ARC(LIB_COMPONENT * aParent);
    LIB_ARC( const LIB_ARC& aArc );
    ~LIB_ARC() { }
    virtual wxString GetClass() const
    {
        return wxT( "LIB_ARC" );
    }


    /**
     * Save arc object to a FILE in "*.lib" format.
     *
     * @param aFile The FILE to write to.
     * @return - True if success writing else false.
     */
    virtual bool Save( FILE* aFile );
    virtual bool Load( char* aLine, wxString& aErrorMsg );

    /**
     * Tests if the given wxPoint is within the bounds of this object.
     *
     * @param aRefPos - Coordinates to test
     * @return - True if a hit, else false
     */
    virtual bool HitTest( const wxPoint& aRefPos );

     /**
     * @param aPosRef - a wxPoint to test
     * @param aThreshold - max distance to this object (usually the half
     *                     thickness of a line)
     * @param aTransMat - the transform matrix
     * @return - True if the point aPosRef is near this object
     */
    virtual bool HitTest( wxPoint aPosRef, int aThreshold, const int aTransMat[2][2] );

    void Draw( WinEDA_DrawPanel * aPanel, wxDC * aDC, const wxPoint &aOffset,
               int aColor, int aDrawMode, void* aData,
               const int aTransformMatrix[2][2] );

    virtual EDA_Rect GetBoundingBox();
    virtual void DisplayInfo( WinEDA_DrawFrame* frame );

    /**
     * @return the size of the "pen" that be used to draw or plot this item
     */
    virtual int GetPenSize( );

protected:
    virtual LIB_DRAW_ITEM* DoGenCopy();

    /**
     * Provide the arc draw object specific comparison.
     *
     * The sort order is as follows:
     *      - Arc horizontal (X) position.
     *      - Arc vertical (Y) position.
     *      - Arc start angle.
     *      - Arc end angle.
     */
    virtual int DoCompare( const LIB_DRAW_ITEM& aOther ) const;
    virtual void DoOffset( const wxPoint& aOffset );
    virtual bool DoTestInside( EDA_Rect& aRect );
    virtual void DoMove( const wxPoint& aPosition );
    virtual wxPoint DoGetPosition() { return m_Pos; }
    virtual void DoMirrorHorizontal( const wxPoint& aCenter );
    virtual void DoPlot( PLOTTER* aPlotter, const wxPoint& aOffset, bool aFill,
                         const int aTransform[2][2] );
    virtual int DoGetWidth() { return m_Width; }
    virtual void DoSetWidth( int aWidth ) { m_Width = aWidth; }
};


#endif    // _LIB_ARC_H_
