
#ifndef _LIB_TEXT_H_
#define _LIB_TEXT_H_


#include "lib_draw_item.h"


/*********************************************/
/* Graphic Body Item: Text                   */
/* This is only a graphic text.              */
/* Fields like Ref , value... are not Text,  */
/* they are a separate class                 */
/*********************************************/
class LIB_TEXT : public LIB_ITEM, public EDA_TEXT
{
    wxString m_savedText;         ///< Temporary storage for the string when edition.
    bool m_rotate;                ///< Flag to indicate a rotation occurred while editing.
    bool m_updateText;            ///< Flag to indicate text change occurred while editing.

    /**
     * Draw the polyline.
     */
    void drawGraphic( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aOffset,
                      int aColor, int aDrawMode, void* aData, const TRANSFORM& aTransform );

    /**
     * Calculate the text attributes ralative to \a aPosition while editing.
     *
     * @param aPosition - Edit position in drawing units.
     */
    void calcEdit( const wxPoint& aPosition );

public:
    LIB_TEXT( LIB_COMPONENT * aParent );
    LIB_TEXT( const LIB_TEXT& aText );
    ~LIB_TEXT() { }

    virtual wxString GetClass() const
    {
        return wxT( "LIB_TEXT" );
    }

    /**
     * Sets the text item string to \a aText.
     *
     * This method does more than juat set the set the text string.  There are special
     * cases when changing the text string alone is not enough.  If the text item is
     * being moved, the name change must be delayed until the next redraw to prevent
     * drawing artifacts.
     *
     * @param aText - New text value.
     */
    void SetText( const wxString& aText );

    /**
     * Write text object out to a FILE in "*.lib" format.
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
      * @param aPosition = a wxPoint to test, in Eeschema coordinates
      * @param aThreshold = max distance to a segment
      * @param aTransform = the transform matrix
      * @return true if the point \a aPosition is near a segment
      */
    virtual bool HitTest( wxPoint aPosition, int aThreshold, const TRANSFORM& aTransform );

    /**
     * Test if the given rectangle intersects this object.
     *
     * For now, an ending point must be inside this rect.
     *
     * @param aRect - the given EDA_RECT
     * @return - true if a hit, else false
     */
    virtual bool HitTest( EDA_RECT& aRect )
    {
        return TextHitTest( aRect );
    }

    /**
     * @return the size of the "pen" that be used to draw or plot this item
     */
    virtual int GetPenSize( ) const;

    virtual void DisplayInfo( EDA_DRAW_FRAME* aFrame );

    virtual EDA_RECT GetBoundingBox() const;

    void Rotate();

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

    virtual BITMAP_DEF GetMenuImage() const { return  add_text_xpm; }

protected:
    virtual EDA_ITEM* doClone() const;

    /**
     * Provide the text draw object specific comparison.
     *
     * The sort order is as follows:
     *      - Text string, case insensitive compare.
     *      - Text horizontal (X) position.
     *      - Text vertical (Y) position.
     *      - Text width.
     *      - Text height.
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
    virtual int DoGetWidth() const { return m_Thickness; }
    virtual void DoSetWidth( int aWidth ) { m_Thickness = aWidth; }
};


#endif    // _LIB_TEXT_H_
