/********************************/
/* Graphic Body Item: Rectangle */
/********************************/

#ifndef _LIB_RECTANGLE_H_
#define _LIB_RECTANGLE_H_


#include "lib_draw_item.h"


class LIB_RECTANGLE  : public LIB_DRAW_ITEM
{
    wxPoint m_savedEndPos;   ///< Tempory storage of the current end position before editing.

    /**
     * Draw the rectangle.
     */
    void drawGraphic( WinEDA_DrawPanel* aPanel, wxDC* aDC, const wxPoint& aOffset,
                      int aColor, int aDrawMode, void* aData, const TRANSFORM& aTransform );

    /**
     * See LIB_DRAW_ITEM::saveAttributes().
     */
    void saveAttributes();

    /**
     * See LIB_DRAW_ITEM::restoreAttributes().
     */
    void restoreAttributes();

    /**
     * Calculate the rectangle attrubites ralative to \a aPosition while editing.
     *
     * @param aPosition - Edit position in drawing units.
     */
    void calcEdit( const wxPoint& aPosition );

public:
    wxPoint m_End;     /* Rectangle end point. */
    wxPoint m_Pos;     /* Rectangle start point. */
    int     m_Width;   /* Line width */
    bool	m_isWidthLocked; /* Flag: Keep width locked */
    bool	m_isHeightLocked; /* Flag: Keep height locked */
    bool	m_isStartPointSelected; /* Flag: is the upper left edge selected ? */

public:
    LIB_RECTANGLE( LIB_COMPONENT * aParent );
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
     * @param aTransform - the transform matrix
     * @return true if the point aPosRef is near this object
     */
    virtual bool HitTest( wxPoint aPosRef, int aThreshold, const TRANSFORM& aTransform );

    /**
     * @return the size of the "pen" that be used to draw or plot this item
     */
    virtual int GetPenSize( );

    virtual EDA_Rect GetBoundingBox();
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
                         const TRANSFORM& aTransform );
    virtual int DoGetWidth() { return m_Width; }
    virtual void DoSetWidth( int aWidth ) { m_Width = aWidth; }
};


#endif    // _LIB_REACTANGLE_H_
