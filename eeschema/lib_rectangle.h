/********************************/
/* Graphic Body Item: Rectangle */
/********************************/

#ifndef _LIB_RECTANGLE_H_
#define _LIB_RECTANGLE_H_


#include "lib_draw_item.h"


class LIB_RECTANGLE  : public LIB_DRAW_ITEM
{
public:
    wxPoint m_End;     /* Rectangle end point. */
    wxPoint m_Pos;     /* Rectangle start point. */
    int     m_Width;   /* Line width */
    bool	m_isWidthLocked; /* Flag: Keep width locked */
    bool	m_isHeightLocked; /* Flag: Keep height locked */
    bool	m_isStartPointSelected; /* Flag: is the upper left edge selected ? */

public:
    LIB_RECTANGLE(LIB_COMPONENT * aParent);
    LIB_RECTANGLE( const LIB_RECTANGLE& aRect );
    ~LIB_RECTANGLE() { }
    virtual wxString GetClass() const
    {
        return wxT( "LIB_RECTANGLE" );
    }


    /**
     * Write rectangle object out to a FILE in "*.lib" format.
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
     * @return - true if a hit, else false
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

    void Draw( WinEDA_DrawPanel * aPanel, wxDC * aDC, const wxPoint &aOffset,
               int aColor, int aDrawMode, void* aData,
               const int aTransformMatrix[2][2] );

    virtual EDA_Rect GetBoundingBox();
    virtual void DisplayInfo( WinEDA_DrawFrame* aFrame );

protected:
    virtual LIB_DRAW_ITEM* DoGenCopy();

    /**
     * Provide the rectangle draw object specific comparison.
     *
     * The sort order is as follows:
     *      - Rectangle horizontal (X) start position.
     *      - Rectangle vertical (Y) start position.
     *      - Rectangle horizontal (X) end position.
     *      - Rectangle vertical (Y) end position.
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


#endif    // _LIB_REACTANGLE_H_
