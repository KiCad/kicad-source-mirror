/****************************************************************/
/*  Headers for library definition and lib component definitions */
/****************************************************************/

/* Definitions of graphic items used to create shapes in component libraries.
 */
#ifndef _LIB_DRAW_ITEM_H_
#define _LIB_DRAW_ITEM_H_

#include "base_struct.h"

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
 * use a Boost pointer class so the ojbect pointers do not accidently get
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
class LIB_DRAW_ITEM : public EDA_BaseStruct
{
    wxPoint m_lastPosition;  ///< Last position when moving the draw item.

public:
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

    wxString m_typeName;   ///< Name of object displayed in the message panel.

public:
    LIB_DRAW_ITEM* Next()
    {
        return (LIB_DRAW_ITEM*) Pnext;
    }


    LIB_DRAW_ITEM( KICAD_T        aType,
                   LIB_COMPONENT* aComponent = NULL,
                   int            aUnit      = 0,
                   int            aConvert   = 0,
                   FILL_T         aFillType  = NO_FILL );

    LIB_DRAW_ITEM( const LIB_DRAW_ITEM& aItem );

    virtual ~LIB_DRAW_ITEM() { }

    /**
     * Begin an editing a component library draw item in \a aEditMode at \a aStartPoint.
     *
     * This is used to start an editing action such as resize or move a draw object.
     * It typically would be called on a left click when a draw tool is selected in
     * the component library editor and one of the graphics tools is selected.  It
     * allows the draw item to maintian it's own internal state while it is being
     * edited. Call AbortEdit() to quit the editing mode.
     *
     * @param aEditMode - The editing mode being performed.  See base_struct.h for a list
     *                    of mode flags.
     * @param aStartPoint - The where the editing mode was started.  This may or may not
     *                      be required depending on the item being edited and the edit
     *                      mode.
     */
    virtual void BeginEdit( int aEditMode, const wxPoint aStartPoint = wxPoint( 0, 0 ) ) {}

    /**
     * Continue an edit in progress at \a aNextPoint.
     *
     * This is used to perform the next action while editing a draw item.  This would be
     * called for each additional left click when the mouse is captured while the item
     * is being edited.
     */
    virtual void ContinueEdit( const wxPoint aNextPoint ) {}

    /**
     * End an object editing action.
     *
     * This is used to abort an edit action in progress initiated by BeginEdit().
     */
    virtual void AbortEdit() { m_Flags = 0; }

    /**
     * Draw a body item
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
     * @param aTransformMatrix - Transform Matrix (rotation, mirror ..)
     */
    virtual void Draw( WinEDA_DrawPanel * aPanel, wxDC * aDC,
                       const wxPoint &aOffset, int aColor, int aDrawMode,
                       void* aData, const int aTransformMatrix[2][2] ) = 0;

    /**
     * @return the size of the "pen" that be used to draw or plot this item
     */
    virtual int GetPenSize() = 0;

    /**
     * Write draw item object to /a aFile in "*.lib" format.
     *
     * @param aFile - The file to write to.
     * @param aErrorMsg - Error message if write fails.
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
     * @param aPosition - The coordinats to test.
     * @return - true if a hit, else false
     */
    virtual bool HitTest( const wxPoint& aPosition )
    {
        return false;
    }

    /**
     * @param aPosRef - a wxPoint to test
     * @param aThreshold - max distance to this object (usually the half
     *                     thickness of a line)
     * @param aTransMat - the transform matrix
     * @return - true if the point aPosRef is near this object
     */
    virtual bool HitTest( wxPoint aPosRef, int aThreshold, const int aTransMat[2][2] ) = 0;

   /**
     * @return the boundary box for this, in library coordinates
     */
    virtual EDA_Rect GetBoundingBox()
    {
        return EDA_BaseStruct::GetBoundingBox();
    }

    virtual void DisplayInfo( WinEDA_DrawFrame* aFrame );

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
     * @param aOffset - Cooridinates to offset position.
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
    bool Inside( EDA_Rect& aRect ) { return DoTestInside( aRect ); }

    /**
     * Move a draw object to a new /a aPosition.
     *
     * The real work is done by the DoMove method for each derived object type.
     *
     * @param aPosition - Position to move draw item to.
     */
    void Move( const wxPoint& aPosition ) { DoMove( aPosition ); }

    /**
     * Return the current draw object start position.
     */
    wxPoint GetPosition() { return DoGetPosition(); }

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
     * Plot the draw item using the plot object.
     *
     * @param aPlotter - The plot object to plot to.
     * @param aOffset - Plot offset position.
     * @param aFill - Flag to indicate whether or not the object is filled.
     * @param aTransform - The plot transform.
     */
    void Plot( PLOTTER* aPlotter, const wxPoint& aOffset, bool aFill,
               const int aTransform[2][2] )
    {
        DoPlot( aPlotter, aOffset, aFill, aTransform );
    }

    /**
     * Return the width of the draw item.
     *
     * @return Width of draw object.
     */
    int GetWidth() { return DoGetWidth(); }
    void SetWidth( int aWidth ) { DoSetWidth( aWidth ); }

    /**
     * Check if draw object can be filled.
     *
     * The default setting is false.  If the derived object support filling,
     * set the m_isFillable member to true.
     *
     * @return - True if draw object can be fill.  Default is false.
     */
    bool IsFillable() { return m_isFillable; }

    /**
     * Return the modified status of the draw object.
     *
     * @return - True if the draw object has been modified.
     */
    bool IsModified() { return ( m_Flags & IS_CHANGED ) != 0; }

    /**
     * Return the new item status of the draw object.
     *
     * @return - True if the draw item has been added to the parent component.
     */
    bool IsNew() { return ( m_Flags & IS_NEW ) != 0; }

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
    virtual bool DoTestInside( EDA_Rect& aRect ) = 0;
    virtual void DoMove( const wxPoint& aPosition ) = 0;
    virtual wxPoint DoGetPosition() = 0;
    virtual void DoMirrorHorizontal( const wxPoint& aCenter ) = 0;
    virtual void DoPlot( PLOTTER* aPlotter, const wxPoint& aOffset, bool aFill,
                         const int aTransform[2][2] ) = 0;
    virtual int DoGetWidth() = 0;
    virtual void DoSetWidth( int aWidth ) = 0;

    /** Flag to indicate if draw item is fillable.  Default is false. */
    bool m_isFillable;
};


/*********************************************/
/* Graphic Body Item: Text                   */
/* This is only a graphic text.              */
/* Fields like Ref , value... are not Text,  */
/* they are a separate class                 */
/*********************************************/
class LIB_TEXT : public LIB_DRAW_ITEM, public EDA_TextStruct
{
public:
    LIB_TEXT(LIB_COMPONENT * aParent);
    LIB_TEXT( const LIB_TEXT& aText );
    ~LIB_TEXT() { }
    virtual wxString GetClass() const
    {
        return wxT( "LIB_TEXT" );
    }


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
     * @param refPos - A wxPoint to test
     * @return - true if a hit, else false
     */
    virtual bool HitTest( const wxPoint& refPos );

     /**
      * @param aPosRef = a wxPoint to test, in eeschema coordinates
      * @param aThreshold = max distance to a segment
      * @param aTransMat = the transform matrix
      * @return true if the point aPosRef is near a segment
      */
    virtual bool HitTest( wxPoint aPosRef, int aThreshold,
                          const int aTransMat[2][2] );

    /**
     * Test if the given rectangle intersects this object.
     *
     * For now, an ending point must be inside this rect.
     *
     * @param aRect - the given EDA_Rect
     * @return - true if a hit, else false
     */
    virtual bool HitTest( EDA_Rect& aRect )
    {
        return TextHitTest( aRect );
    }

    /**
     * @return the size of the "pen" that be used to draw or plot this item
     */
    virtual int GetPenSize( );

    void Draw( WinEDA_DrawPanel * aPanel, wxDC * aDC, const wxPoint &aOffset,
               int aColor, int aDrawMode, void* aData,
               const int aTransformMatrix[2][2] );

    virtual void DisplayInfo( WinEDA_DrawFrame* aFrame );

    virtual EDA_Rect GetBoundingBox();

protected:
    virtual LIB_DRAW_ITEM* DoGenCopy();

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


#endif  //  _LIB_DRAW_ITEM_H_
