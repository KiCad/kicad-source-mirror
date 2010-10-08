/*****************************/
/* Graphic Body Item: Circle */
/*****************************/

#ifndef _LIB_CIRCLE_H_
#define _LIB_CIRCLE_H_


#include "lib_draw_item.h"


class LIB_CIRCLE : public LIB_DRAW_ITEM
{
public:
    int     m_Radius;
    wxPoint m_Pos;    /* Position or centre (Arc and Circle) or start point (segments) */
    int     m_Width;  /* Line width */

public:
    LIB_CIRCLE(LIB_COMPONENT * aParent);
    LIB_CIRCLE( const LIB_CIRCLE& aCircle );
    ~LIB_CIRCLE() { }
    virtual wxString GetClass() const
    {
        return wxT( "LIB_CIRCLE" );
    }


    /**
     * Write circle object to a FILE in "*.lib" format.
     *
     * @param aFile - The FILE to write to.
     * @return - true if success writing else false.
     */
    virtual bool Save( FILE* aFile );
    virtual bool Load( char* aLine, wxString& aErrorMsg );

    /**
     * Test if the given point is within the bounds of this object.
     *
     * @param aRefPos - A wxPoint to test
     * @return bool - true if a hit, else false
     */
    virtual bool HitTest( const wxPoint& aRefPos );

     /**
      * @param aPosRef - a wxPoint to test
      * @param aThreshold - max distance to this object (usually the half
      *                     thickness of a line)
      * @param aTransMat - the transform matrix
      * @return true if the point aPosRef is near this object
      */
    virtual bool HitTest( wxPoint aPosRef, int aThreshold, const int aTransMat[2][2] );

    /**
     * @return the size of the "pen" that be used to draw or plot this item
     */
    virtual int GetPenSize( );

    void Draw( WinEDA_DrawPanel* aPanel, wxDC* aDC, const wxPoint &aOffset,
               int aColor, int aDrawMode, void* aData,
               const int aTransformMatrix[2][2] );

    virtual EDA_Rect GetBoundingBox();
    virtual void DisplayInfo( WinEDA_DrawFrame* aFrame );

protected:
    virtual LIB_DRAW_ITEM* DoGenCopy();

    /**
     * Provide the circle draw object specific comparison.
     *
     * The sort order is as follows:
     *      - Circle horizontal (X) position.
     *      - Circle vertical (Y) position.
     *      - Circle radius.
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


#endif    // _LIB_CIRCLE_H_
