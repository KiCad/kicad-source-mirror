/**************************/
/* Graphic Body Item: Arc */
/**************************/

#ifndef _LIB_ARC_H_
#define _LIB_ARC_H_


#include "lib_draw_item.h"


class TRANSFORM;


class LIB_ARC : public LIB_ITEM
{
    enum SELECT_T
    {
        START,
        END,
        OUTLINE,
    };

    int      m_Radius;
    int      m_t1;              /* First radius angle of the arc in 0.1 degrees. */
    int      m_t2;              /* Second radius angle of the arc in 0.1 degrees. */
    wxPoint  m_ArcStart;
    wxPoint  m_ArcEnd;          /* Arc end position. */
    wxPoint  m_Pos;             /* Radius center point. */
    int      m_Width;           /* Line width */
    double   m_editCenterDistance;
    SELECT_T m_editSelectPoint;
    int      m_editState;
    int      m_editDirection;
    int      m_lastEditState;

    /**
     * Draws the arc.
     */
    void drawGraphic( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aOffset,
                      int aColor, int aDrawMode, void* aData, const TRANSFORM& aTransform );

    /**
     * Draw the graphics when the arc is being edited.
     */
    void drawEditGraphics( EDA_RECT* aClipBox, wxDC* aDC, int aColor );

    /**
     * Calculates the center, radius, and angles at \a aPosition when the arc is being edited.
     *
     * Note: The center may not necessarily be on the grid.
     *
     * @param aPosition - The current mouse position in drawing coordinates.
     */
    void calcEdit( const wxPoint& aPosition );

    /**
     * Calculate the radius and angle of an arc using the start, end, and center points.
     */
    void calcRadiusAngles();

public:
    LIB_ARC( LIB_COMPONENT * aParent );
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
     * @param aPosition - Coordinates to test
     * @return - True if a hit, else false
     */
    virtual bool HitTest( const wxPoint& aPosition );

     /**
     * @param aPosition - a wxPoint to test
     * @param aThreshold - max distance to this object (usually the half
     *                     thickness of a line)
     * @param aTransform - the transform matrix
     * @return - True if the point \a aPosition is near this object
     */
    virtual bool HitTest( wxPoint aPosition, int aThreshold, const TRANSFORM& aTransform );

    virtual EDA_RECT GetBoundingBox() const;
    virtual void DisplayInfo( EDA_DRAW_FRAME* frame );

    /**
     * @return the size of the "pen" that be used to draw or plot this item
     */
    virtual int GetPenSize( ) const;

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

    virtual const char** GetMenuImage() const { return (const char**) add_arc_xpm; }

protected:
    virtual EDA_ITEM* doClone() const;

    /**
     * Provide the arc draw object specific comparison.
     *
     * The sort order is as follows:
     *      - Arc horizontal (X) position.
     *      - Arc vertical (Y) position.
     *      - Arc start angle.
     *      - Arc end angle.
     */
    virtual int DoCompare( const LIB_ITEM& aOther ) const;
    virtual void DoOffset( const wxPoint& aOffset );
    virtual bool DoTestInside( EDA_RECT& aRect ) const;
    virtual void DoMove( const wxPoint& aPosition );
    virtual wxPoint DoGetPosition() const { return m_Pos; }
    virtual void DoMirrorHorizontal( const wxPoint& aCenter );
    virtual void DoMirrorVertical( const wxPoint& aCenter );
    virtual void DoRotate( const wxPoint& aCenter, bool aRotateCCW = true );
    virtual void DoPlot( PLOTTER* aPlotter, const wxPoint& aOffset, bool aFill,
                         const TRANSFORM& aTransform );
    virtual int DoGetWidth() const { return m_Width; }
    virtual void DoSetWidth( int aWidth ) { m_Width = aWidth; }
};


#endif    // _LIB_ARC_H_
