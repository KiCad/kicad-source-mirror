/*****************************/
/* Graphic Body Item: Circle */
/*****************************/

#ifndef _LIB_CIRCLE_H_
#define _LIB_CIRCLE_H_


#include "lib_draw_item.h"


class LIB_CIRCLE : public LIB_DRAW_ITEM
{
    int     m_Radius;
    wxPoint m_Pos;            // Position or centre (Arc and Circle) or start point (segments).
    int     m_Width;          // Line width.

    /**
     * Draws the arc.
     */
    void drawGraphic( WinEDA_DrawPanel* aPanel, wxDC* aDC, const wxPoint& aOffset,
                      int aColor, int aDrawMode, void* aData, const TRANSFORM& aTransform );

    /**
     * Calculate the new circle at \a aPosition when editing.
     *
     * @param aPosition - The position to edit the circle in drawing coordinates.
     */
    void calcEdit( const wxPoint& aPosition );

public:
    LIB_CIRCLE( LIB_COMPONENT * aParent );
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
     * @param aPosRef - A wxPoint to test
     * @return bool - true if a hit, else false
     */
    virtual bool HitTest( const wxPoint& aPosRef );

     /**
      * @param aPosRef - a wxPoint to test
      * @param aThreshold - max distance to this object (usually the half
      *                     thickness of a line)
      * @param aTransform - the transform matrix
      * @return true if the point aPosRef is near this object
      */
    virtual bool HitTest( wxPoint aPosRef, int aThreshold, const TRANSFORM& aTransform );

    /**
     * @return the size of the "pen" that be used to draw or plot this item
     */
    virtual int GetPenSize( );

    virtual EDA_Rect GetBoundingBox() const;
    virtual void DisplayInfo( WinEDA_DrawFrame* aFrame );

    /**
     * See LIB_DRAW_ITEM::BeginEdit().
     */
    void BeginEdit( int aEditMode, const wxPoint aStartPoint = wxPoint( 0, 0 ) );

    /**
     * See LIB_DRAW_ITEM::ContinueEdit().
     */
    bool ContinueEdit( const wxPoint aNextPoint );

    /**
     * See LIB_DRAW_ITEM::AbortEdit().
     */
    void EndEdit( const wxPoint& aPosition, bool aAbort = false );

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
    virtual bool DoTestInside( EDA_Rect& aRect ) const;
    virtual void DoMove( const wxPoint& aPosition );
    virtual wxPoint DoGetPosition() const { return m_Pos; }
    virtual void DoMirrorHorizontal( const wxPoint& aCenter );
    virtual void DoPlot( PLOTTER* aPlotter, const wxPoint& aOffset, bool aFill,
                         const TRANSFORM& aTransform );
    virtual int DoGetWidth() const { return m_Width; }
    virtual void DoSetWidth( int aWidth ) { m_Width = aWidth; }
};


#endif    // _LIB_CIRCLE_H_
