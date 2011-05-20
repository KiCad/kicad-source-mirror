/********************************/
/* Graphic Body Item: Rectangle */
/********************************/

#ifndef _LIB_RECTANGLE_H_
#define _LIB_RECTANGLE_H_


#include "lib_draw_item.h"


class LIB_RECTANGLE  : public LIB_ITEM
{
    wxPoint m_End;                  // Rectangle end point.
    wxPoint m_Pos;                  // Rectangle start point.
    int     m_Width;                // Line width
    bool    m_isWidthLocked;        // Flag: Keep width locked
    bool    m_isHeightLocked;       // Flag: Keep height locked
    bool    m_isStartPointSelected; // Flag: is the upper left edge selected?

    /**
     * Draw the rectangle.
     */
    void drawGraphic( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aOffset,
                      int aColor, int aDrawMode, void* aData, const TRANSFORM& aTransform );

    /**
     * Calculate the rectangle attributes relative to \a aPosition while editing.
     *
     * @param aPosition - Edit position in drawing units.
     */
    void calcEdit( const wxPoint& aPosition );

public:
public:
    LIB_RECTANGLE( LIB_COMPONENT * aParent );
    LIB_RECTANGLE( const LIB_RECTANGLE& aRect );
    ~LIB_RECTANGLE() { }
    virtual wxString GetClass() const
    {
        return wxT( "LIB_RECTANGLE" );
    }

    void SetEndPosition( const wxPoint& aPosition ) { m_End = aPosition; }

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
     * @param aPosition - A wxPoint to test
     * @return - true if a hit, else false
     */
    virtual bool HitTest( const wxPoint& aPosition );

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
    virtual int GetPenSize( ) const;

    virtual EDA_RECT GetBoundingBox() const;

    virtual void DisplayInfo( EDA_DRAW_FRAME* aFrame );

    /**
     * See LIB_ITEM::BeginEdit().
     */
    void BeginEdit( int aEditMode, const wxPoint aStartPoint = wxPoint( 0, 0 ) );

    /**
     * See LIB_ITEM::ContinueEdit().
     */
    bool ContinueEdit( const wxPoint aNextPoint );

    /**
     * See LIB_ITEM::AbortEdit().
     */
    void EndEdit( const wxPoint& aPosition, bool aAbort = false );

    virtual wxString GetSelectMenuText() const;

    virtual const char** GetMenuImage() const { return (const char**) add_rectangle_xpm; }

protected:
    virtual EDA_ITEM* doClone() const;

    /**
     * Provide the rectangle draw object specific comparison.
     *
     * The sort order is as follows:
     *      - Rectangle horizontal (X) start position.
     *      - Rectangle vertical (Y) start position.
     *      - Rectangle horizontal (X) end position.
     *      - Rectangle vertical (Y) end position.
     */
    virtual int DoCompare( const LIB_ITEM& aOther ) const;

    virtual void DoOffset( const wxPoint& aOffset );
    virtual bool DoTestInside( EDA_RECT& aRect ) const;
    virtual void DoMove( const wxPoint& aPosition );
    virtual wxPoint DoGetPosition() const { return m_Pos; }
    virtual void DoMirrorHorizontal( const wxPoint& aCenter );
    virtual void DoMirrorVertical( const wxPoint& aCenter );
    virtual void DoRotate( const wxPoint& aCenter );
    virtual void DoPlot( PLOTTER* aPlotter, const wxPoint& aOffset, bool aFill,
                         const TRANSFORM& aTransform );
    virtual int DoGetWidth() const { return m_Width; }
    virtual void DoSetWidth( int aWidth ) { m_Width = aWidth; }
};


#endif    // _LIB_RECTANGLE_H_
