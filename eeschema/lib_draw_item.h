/****************************************************************/
/*  Headers for library definition and lib component definitions */
/****************************************************************/

/* Definitions of graphic items used to create shapes in component libraries.
 */
#ifndef _LIB_DRAW_ITEM_H_
#define _LIB_DRAW_ITEM_H_

#include "base_struct.h"
#include "transform.h"

#include <boost/ptr_container/ptr_vector.hpp>


class LIB_COMPONENT;
class PLOTTER;
class LIB_DRAW_ITEM;
class LIB_PIN;


extern const int fill_tab[];


#define MINIMUM_SELECTION_DISTANCE 15 // Minimum selection distance in mils


/**
 * Helper for defining a list of library draw object pointers.  The Boost
 * pointer containers are responsible for deleting object pointers placed
 * in them.  If you access a object pointer from the list, do not delete
 * it directly.
 */
typedef boost::ptr_vector< LIB_DRAW_ITEM > LIB_DRAW_ITEM_LIST;

/**
 * Helper for defining a list of pin object pointers.  The list does not
 * use a Boost pointer class so the object pointers do not accidently get
 * deleted when the container is deleted.
 */
typedef std::vector< LIB_PIN* > LIB_PIN_LIST;


/****************************************************************************/
/* Classes for handle the body items of a component: pins add graphic items */
/****************************************************************************/


/**
 * Base class for drawable items used in library components.
 *  (graphic shapes, texts, fields, pins)
 */
class LIB_DRAW_ITEM : public EDA_ITEM
{
    /**
     * Draws the item.
     */
    virtual void drawGraphic( EDA_DRAW_PANEL* aPanel, wxDC* aDC,
                              const wxPoint& aOffset, int aColor,
                              int aDrawMode, void* aData, const TRANSFORM& aTransform ) = 0;

    /**
     * Draw any editing specific graphics when the item is being edited.
     *
     * @param aClipBox - Clip box of the current device context.
     * @param aDC - The device context to draw on.
     * @param aColor - The index of the color to draw.
     */
    virtual void drawEditGraphics( EDA_RECT* aClipBox, wxDC* aDC, int aColor ) {}

    /**
     * Calculates the attributes of an item  at \a aPosition when it is being edited.
     *
     * This method gets called by the Draw() method when the item is being edited.  This
     * probably should be a pure virtual method but bezier curves are not yet editable in
     * the component library editor.  Therefore, the default method does nothing.
     *
     * @param aPosition - The current mouse position in drawing coordinates.
     */
    virtual void calcEdit( const wxPoint& aPosition ) {}

    bool    m_eraseLastDrawItem; ///< Used when editing a new draw item to prevent drawing
                                 ///< artifacts.

    friend class LIB_COMPONENT;

protected:
    /**
     * Unit identification for multiple parts per package.  Set to 0 if the
     * item is common to all units.
     */
    int      m_Unit;

    /**
     * Shape identification for alternate body styles.  Set 0 if the item
     * is common to all body styles.  This is commonly referred to as
     * DeMorgan style and this is typically how it is used in Kicad.
     */
    int      m_Convert;

    /**
     * The body fill type.  This has meaning only for some items.  For a list of
     * fill types see #FILL_T.
     */
    FILL_T   m_Fill;

    wxString m_typeName;          ///< Name of object displayed in the message panel.

    wxPoint  m_initialPos;        ///< Temporary position when moving an existing item.
    wxPoint  m_initialCursorPos;  ///< Initial cursor position at the beginning of a move.

public:

    LIB_DRAW_ITEM( KICAD_T        aType,
                   LIB_COMPONENT* aComponent = NULL,
                   int            aUnit      = 0,
                   int            aConvert   = 0,
                   FILL_T         aFillType  = NO_FILL );

    LIB_DRAW_ITEM( const LIB_DRAW_ITEM& aItem );

    virtual ~LIB_DRAW_ITEM() { }

    wxString GetTypeName() { return m_typeName; }

    /**
     * Begin an editing a component library draw item in \a aEditMode at \a aPosition.
     *
     * This is used to start an editing action such as resize or move a draw object.
     * It typically would be called on a left click when a draw tool is selected in
     * the component library editor and one of the graphics tools is selected.  It
     * allows the draw item to maintain it's own internal state while it is being
     * edited. Call AbortEdit() to quit the editing mode.
     *
     * @param aEditMode - The editing mode being performed.  See base_struct.h for a list
     *                    of mode flags.
     * @param aPosition - The position in drawing coordinates where the editing mode was
     *                    started.  This may or may not be required depending on the item
     *                    being edited and the edit mode.
     */
    virtual void BeginEdit( int aEditMode, const wxPoint aPosition = wxPoint( 0, 0 ) ) {}

    /**
     * Continue an edit in progress at \a aPosition.
     *
     * This is used to perform the next action while editing a draw item.  This would be
     * called for each additional left click when the mouse is captured while the item
     * is being edited.
     *
     * @param aPosition - The position of the mouse left click in drawing coordinates.
     * @return True if additional mouse clicks are required to complete the edit in progress.
     */
    virtual bool ContinueEdit( const wxPoint aPosition ) { return false; }

    /**
     * End an object editing action.
     *
     * This is used to end or abort an edit action in progress initiated by BeginEdit().
     *
     * @param aPosition - The position of the last edit event in drawing coordinates.
     * @param aAbort - Set to true to abort the current edit in progress.
     */
    virtual void EndEdit( const wxPoint& aPosition, bool aAbort = false ) { m_Flags = 0; }

    /**
     * Draw an item
     *
     * @param aPanel - DrawPanel to use (can be null) mainly used for clipping
     *                 purposes
     * @param aDC - Device Context (can be null)
     * @param aOffset - offset to draw
     * @param aColor - -1 to use the normal body item color, or use this color
     *                 if >= 0
     * @param aDrawMode - GR_OR, GR_XOR, ...
     * @param aData - value or pointer used to pass others parameters,
     *                depending on body items. used for some items to force
     *                to force no fill mode ( has meaning only for items what
     *                can be filled ). used in printing or moving objects mode
     *                or to pass reference to the lib component for pins
     * @param aTransform - Transform Matrix (rotation, mirror ..)
     */
    virtual void Draw( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint &aOffset, int aColor,
                       int aDrawMode, void* aData, const TRANSFORM& aTransform );

    /**
     * @return the size of the "pen" that be used to draw or plot this item
     */
    virtual int GetPenSize() = 0;

    /**
     * Write draw item object to \a aFile in "*.lib" format.
     *
     * @param aFile - The file to write to.
     * @return - true if success writing else false.
     */
    virtual bool Save( FILE* aFile ) = 0;
    virtual bool Load( char* aLine, wxString& aErrorMsg ) = 0;

    LIB_COMPONENT* GetParent()
    {
        return (LIB_COMPONENT *)m_Parent;
    }

    /**
     * Tests if the given point is within the bounds of this object.
     *
     * Derived classes should override this function.
     *
     * @param aPosition - The coordinates to test.
     * @return - true if a hit, else false
     */
    virtual bool HitTest( const wxPoint& aPosition )
    {
        return false;
    }

    /**
     * @param aPosition - a wxPoint to test
     * @param aThreshold - max distance to this object (usually the half
     *                     thickness of a line)
     * @param aTransform - the transform matrix
     * @return - true if the point \a aPosition is near this object
     */
    virtual bool HitTest( wxPoint aPosition, int aThreshold, const TRANSFORM& aTransform ) = 0;

   /**
     * @return the boundary box for this, in library coordinates
     */
    virtual EDA_RECT GetBoundingBox() const { return EDA_ITEM::GetBoundingBox(); }

    /**
     * Displays basic info (type, part and convert) about item
     * in msg panel
     * @param aFrame = main frame where the message manel info is.
     */
    virtual void DisplayInfo( EDA_DRAW_FRAME* aFrame );

    /**
     * Make a copy of this draw item.
     *
     * Classes derived from LIB_DRAW_ITEM must implement DoGenCopy().
     * This is just a placeholder for the derived class.
     *
     * @return Copy of this draw item.
     */
    LIB_DRAW_ITEM* GenCopy() { return DoGenCopy(); }

    /**
     * Test LIB_DRAW_ITEM objects for equivalence.
     *
     * @param aOther - Object to test against.
     * @return - True if object is identical to this object.
     */
    bool operator==( const LIB_DRAW_ITEM& aOther ) const;
    bool operator==( const LIB_DRAW_ITEM* aOther ) const
    {
        return *this == *aOther;
    }

    /**
     * Test if another draw item is less than this draw object.
     *
     * @param aOther - Draw item to compare against.
     * @return - True if object is less than this object.
     */
    bool operator<( const LIB_DRAW_ITEM& aOther) const;

    /**
     * Set drawing object offset from the current position.
     *
     * @param aOffset - Coordinates to offset position.
     */
    void SetOffset( const wxPoint& aOffset ) { DoOffset( aOffset ); }

    /**
     * Test if any part of the draw object is inside rectangle bounds.
     *
     * This is used for block selection.  The real work is done by the
     * DoTestInside method for each derived object type.
     *
     * @param aRect - Rectangle to check against.
     * @return - True if object is inside rectangle.
     */
    bool Inside( EDA_RECT& aRect ) const { return DoTestInside( aRect ); }

    /**
     * Move a draw object to a new \a aPosition.
     *
     * The real work is done by the DoMove method for each derived object type.
     *
     * @param aPosition - Position to move draw item to.
     */
    void Move( const wxPoint& aPosition ) { DoMove( aPosition ); }

    /**
     * Return the current draw object start position.
     */
    wxPoint GetPosition() const { return DoGetPosition(); }

    void SetPosition( const wxPoint& aPosition ) { DoMove( aPosition ); }

    /**
     * Mirror the draw object along the horizontal (X) axis about a point.
     *
     * @param aCenter - Point to mirror around.
     */
    void MirrorHorizontal( const wxPoint& aCenter )
    {
        DoMirrorHorizontal( aCenter );
    }

    /**
     * Rotate the draw item.
     */
    virtual void Rotate() {}

    /**
     * Plot the draw item using the plot object.
     *
     * @param aPlotter - The plot object to plot to.
     * @param aOffset - Plot offset position.
     * @param aFill - Flag to indicate whether or not the object is filled.
     * @param aTransform - The plot transform.
     */
    void Plot( PLOTTER* aPlotter, const wxPoint& aOffset, bool aFill, const TRANSFORM& aTransform )
    {
        DoPlot( aPlotter, aOffset, aFill, aTransform );
    }

    /**
     * Return the width of the draw item.
     *
     * @return Width of draw object.
     */
    int GetWidth() const { return DoGetWidth(); }
    void SetWidth( int aWidth ) { DoSetWidth( aWidth ); }

    /**
     * Check if draw object can be filled.
     *
     * The default setting is false.  If the derived object support filling,
     * set the m_isFillable member to true.
     *
     * @return - True if draw object can be fill.  Default is false.
     */
    bool IsFillable() const { return m_isFillable; }

    /**
     * Return the draw item editing mode status.
     *
     * @return - True if the item is being edited.
     */
    bool InEditMode() const { return ( m_Flags & ( IS_NEW | IS_MOVED | IS_RESIZED ) ) != 0; }

    void SetEraseLastDrawItem( bool aErase = true ) { m_eraseLastDrawItem = aErase; }

    virtual int GetDefaultColor();

    void SetUnit( int aUnit ) { m_Unit = aUnit; }

    int GetUnit() const { return m_Unit; }

    void SetConvert( int aConvert ) { m_Convert = aConvert; }

    int GetConvert() const { return m_Convert; }

    void SetFillMode( FILL_T aFillMode ) { m_Fill = aFillMode; }

    FILL_T GetFillMode() const { return m_Fill; }

protected:
    virtual LIB_DRAW_ITEM* DoGenCopy() = 0;

    /**
     * Provide the draw object specific comparison.
     *
     * This is called by the == and < operators.
     *
     * The sort order is as follows:
     *      - Component alternate part (DeMorgan) number.
     *      - Component part number.
     *      - KICAD_T enum value.
     *      - Result of derived classes comparison.
     */
    virtual int DoCompare( const LIB_DRAW_ITEM& aOther ) const = 0;
    virtual void DoOffset( const wxPoint& aOffset ) = 0;
    virtual bool DoTestInside( EDA_RECT& aRect ) const = 0;
    virtual void DoMove( const wxPoint& aPosition ) = 0;
    virtual wxPoint DoGetPosition() const = 0;
    virtual void DoMirrorHorizontal( const wxPoint& aCenter ) = 0;
    virtual void DoPlot( PLOTTER* aPlotter, const wxPoint& aOffset, bool aFill,
                         const TRANSFORM& aTransform ) = 0;
    virtual int DoGetWidth() const = 0;
    virtual void DoSetWidth( int aWidth ) = 0;

    /** Flag to indicate if draw item is fillable.  Default is false. */
    bool m_isFillable;
};


#endif  //  _LIB_DRAW_ITEM_H_
