/**********************************************************/
/* Graphic Body Item: Polygon and polyline (set of lines) */
/**********************************************************/

#ifndef _LIB_POLYLINE_H_
#define _LIB_POLYLINE_H_


#include "lib_draw_item.h"


class LIB_POLYLINE : public LIB_ITEM
{
    int m_Width;                              // Line width
    std::vector<wxPoint> m_PolyPoints;        // list of points (>= 2)

    int m_ModifyIndex;                        // Index of the polyline point to modify

    /**
     * Draw the polyline.
     */
    void drawGraphic( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aOffset,
                      int aColor, int aDrawMode, void* aData, const TRANSFORM& aTransform );

    /**
     * Calculate the polyline attributes relative to \a aPosition while editing.
     *
     * @param aPosition - Edit position in drawing units.
     */
    void calcEdit( const wxPoint& aPosition );

public:
public:
    LIB_POLYLINE( LIB_COMPONENT * aParent );
    LIB_POLYLINE( const LIB_POLYLINE& aPolyline );
    ~LIB_POLYLINE() { }

    virtual wxString GetClass() const
    {
        return wxT( "LIB_POLYLINE" );
    }


    /**
     * Write polyline object out to a FILE in "*.lib" format.
     *
     * @param aFile - The FILE to write to.
     * @return - true if success writing else false.
     */
    virtual bool Save( FILE* aFile );
    virtual bool Load( char* aLine, wxString& aErrorMsg );

    void AddPoint( const wxPoint& aPoint );

    /**
     * Delete the segment at \a aPosition.
     */
    void DeleteSegment( const wxPoint aPosition );

    /**
     * @return the number of corners
     */
    unsigned GetCornerCount() const { return m_PolyPoints.size(); }

     /**
     * Test if the given point is within the bounds of this object.
     *
     * @param aPosition - A wxPoint to test
     * @return - true if a hit, else false
     */
    virtual bool HitTest( const wxPoint& aPosition );

    /**
     * @param aPosition = a wxPoint to test
     * @param aThreshold = max distance to a segment
     * @param aTransform = the transform matrix
     * @return true if the point \a aPosition is near a segment
     */
    virtual bool HitTest( wxPoint aPosition, int aThreshold, const TRANSFORM& aTransform );

    /**
     * @return the boundary box for this, in library coordinates
     */
    virtual EDA_RECT GetBoundingBox() const;

    /**
     * @return the size of the "pen" that be used to draw or plot this item
     */
    virtual int GetPenSize( ) const;

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

    virtual const char** GetMenuImage() const { return (const char**) add_polygon_xpm; }

protected:
    virtual EDA_ITEM* doClone() const;

    /**
     * Provide the polyline segment draw object specific comparison.
     *
     * The sort order for each polyline segment point is as follows:
     *      - Line segment point horizontal (X) position.
     *      - Line segment point vertical (Y) position.
     */
    virtual int DoCompare( const LIB_ITEM& aOther ) const;

    virtual void DoOffset( const wxPoint& aOffset );
    virtual bool DoTestInside( EDA_RECT& aRect ) const;
    virtual void DoMove( const wxPoint& aPosition );
    virtual wxPoint DoGetPosition() const { return m_PolyPoints[0]; }
    virtual void DoMirrorHorizontal( const wxPoint& aCenter );
    virtual void DoMirrorVertical( const wxPoint& aCenter );
    virtual void DoRotate( const wxPoint& aCenter );
    virtual void DoPlot( PLOTTER* aPlotter, const wxPoint& aOffset, bool aFill,
                         const TRANSFORM& aTransform );
    virtual int DoGetWidth() const { return m_Width; }
    virtual void DoSetWidth( int aWidth ) { m_Width = aWidth; }
};


#endif   // _LIB_POLYLINE_H_
