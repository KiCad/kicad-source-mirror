/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2004-2011 KiCad Developers, see change_log.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file class_libentry.h
 * @brief Class LIB_ITEM definition.
 */

#ifndef _LIB_ITEM_H_
#define _LIB_ITEM_H_

#include <base_struct.h>
#include <transform.h>
#include <gr_basic.h>

#include <boost/ptr_container/ptr_vector.hpp>


class LINE_READER;
class OUTPUTFORMATTER;
class LIB_PART;
class PLOTTER;
class LIB_ITEM;
class LIB_PIN;
class MSG_PANEL_ITEM;


extern const int fill_tab[];


#define MINIMUM_SELECTION_DISTANCE 2 // Minimum selection distance in internal units


/**
 * Helper for defining a list of library draw object pointers.  The Boost
 * pointer containers are responsible for deleting object pointers placed
 * in them.  If you access a object pointer from the list, do not delete
 * it directly.
 */
typedef boost::ptr_vector< LIB_ITEM > LIB_ITEMS;

/**
 * Helper for defining a list of pin object pointers.  The list does not
 * use a Boost pointer class so the object pointers do not accidentally get
 * deleted when the container is deleted.
 */
typedef std::vector< LIB_PIN* > LIB_PINS;


/**
 * Class LIB_ITEM
 * is the base class for drawable items used by schematic library components.
 */
class LIB_ITEM : public EDA_ITEM
{
    /**
     * Function drawGraphic
     *
     * draws the item on \a aPanel.
     *
     * @param aPanel A pointer to the panel to draw the object upon.
     * @param aDC A pointer to the device context used to draw the object.
     * @param aOffset A reference to a wxPoint object containing the offset where to draw
     *                from the object's current position.
     * @param aColor An #EDA_COLOR_T to draw the object or -1 to draw the object in it's
     *               default color.
     * @param aDrawMode The mode used to perform the draw (#GR_OR, #GR_COPY, etc.).
     * @param aData A pointer to any object specific data required to perform the draw.
     * @param aTransform A reference to a #TRANSFORM object containing drawing transform.
     */
    virtual void drawGraphic( EDA_DRAW_PANEL* aPanel, wxDC* aDC,
                              const wxPoint& aOffset, EDA_COLOR_T aColor,
                              GR_DRAWMODE aDrawMode, void* aData,
                              const TRANSFORM& aTransform ) = 0;

    /**
     * Draw any editing specific graphics when the item is being edited.
     *
     * @param aClipBox Clip box of the current device context.
     * @param aDC The device context to draw on.
     * @param aColor The index of the color to draw.
     */
    virtual void drawEditGraphics( EDA_RECT* aClipBox, wxDC* aDC, EDA_COLOR_T aColor ) {}

    /**
     * Calculates the attributes of an item at \a aPosition when it is being edited.
     *
     * This method gets called by the Draw() method when the item is being edited.  This
     * probably should be a pure virtual method but bezier curves are not yet editable in
     * the component library editor.  Therefore, the default method does nothing.
     *
     * @param aPosition The current mouse position in drawing coordinates.
     */
    virtual void calcEdit( const wxPoint& aPosition ) {}

    bool    m_eraseLastDrawItem; ///< Used when editing a new draw item to prevent drawing
                                 ///< artifacts.

    friend class LIB_PART;

protected:
    /**
     * Unit identification for multiple parts per package.  Set to 0 if the
     * item is common to all units.
     */
    int      m_Unit;

    /**
     * Shape identification for alternate body styles.  Set 0 if the item
     * is common to all body styles.  This is commonly referred to as
     * DeMorgan style and this is typically how it is used in KiCad.
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

    /** Flag to indicate if draw item is fillable.  Default is false. */
    bool m_isFillable;

public:

    LIB_ITEM( KICAD_T        aType,
              LIB_PART*      aComponent = NULL,
              int            aUnit      = 0,
              int            aConvert   = 0,
              FILL_T         aFillType  = NO_FILL );

    // Do not create a copy constructor.  The one generated by the compiler is adequate.

    virtual ~LIB_ITEM() { }

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
     * @param aEditMode The editing mode being performed.  See base_struct.h for a list
     *                  of mode flags.
     * @param aPosition The position in drawing coordinates where the editing mode was
     *                  started.  This may or may not be required depending on the item
     *                  being edited and the edit mode.
     */
    virtual void BeginEdit( STATUS_FLAGS aEditMode, const wxPoint aPosition = wxPoint( 0, 0 ) ) {}

    /**
     * Continue an edit in progress at \a aPosition.
     *
     * This is used to perform the next action while editing a draw item.  This would be
     * called for each additional left click when the mouse is captured while the item
     * is being edited.
     *
     * @param aPosition The position of the mouse left click in drawing coordinates.
     * @return True if additional mouse clicks are required to complete the edit in progress.
     */
    virtual bool ContinueEdit( const wxPoint aPosition ) { return false; }

    /**
     * End an object editing action.
     *
     * This is used to end or abort an edit action in progress initiated by BeginEdit().
     *
     * @param aPosition The position of the last edit event in drawing coordinates.
     * @param aAbort Set to true to abort the current edit in progress.
     */
    virtual void EndEdit( const wxPoint& aPosition, bool aAbort = false ) { m_Flags = 0; }

    /**
     * Draw an item
     *
     * @param aPanel DrawPanel to use (can be null) mainly used for clipping purposes.
     * @param aDC Device Context (can be null)
     * @param aOffset Offset to draw
     * @param aColor -1 to use the normal body item color, or use this color if >= 0
     * @param aDrawMode GR_OR, GR_XOR, ...
     * @param aData Value or pointer used to pass others parameters, depending on body items.
     *              Used for some items to force to force no fill mode ( has meaning only for
     *              items what can be filled ). used in printing or moving objects mode or to
     *              pass reference to the lib component for pins.
     * @param aTransform Transform Matrix (rotation, mirror ..)
     */
    virtual void Draw( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint &aOffset,
                       EDA_COLOR_T aColor, GR_DRAWMODE aDrawMode, void* aData,
                       const TRANSFORM& aTransform );

    /**
     * Function GetPenSize
     *
     * @return the size of the "pen" that be used to draw or plot this item
     */
    virtual int GetPenSize() const = 0;

    /**
     * Function Save
     * writes draw item object to \a aFormatter in component library "*.lib" format.
     *
     * @param aFormatter A reference to an #OUTPUTFORMATTER object to write the
     *                   component library item to.
     * @return True if success writing else false.
     */
    virtual bool Save( OUTPUTFORMATTER& aFormatter ) = 0;

    virtual bool Load( LINE_READER& aLine, wxString& aErrorMsg ) = 0;

    LIB_PART*      GetParent() const
    {
        return (LIB_PART *)m_Parent;
    }

    virtual bool HitTest( const wxPoint& aPosition ) const
    {
        return EDA_ITEM::HitTest( aPosition );
    }

    /**
     * @param aPosition A wxPoint to test.
     * @param aThreshold Maximum distance to this object (usually the half thickness of a line)
     *                   if < 0, it will be automatically set to half pen size when locating
     *                   lines or arcs and set to 0 for other items.
     * @param aTransform The transform matrix.
     * @return True if the point \a aPosition is near this object
     */
    virtual bool HitTest( const wxPoint &aPosition, int aThreshold, const TRANSFORM& aTransform ) const = 0;

   /**
     * @return the boundary box for this, in library coordinates
     */
    virtual const EDA_RECT GetBoundingBox() const { return EDA_ITEM::GetBoundingBox(); }

    /**
     * Function GetMsgPanelInfo
     * displays basic info (type, part and convert) about the current item
     * in message panel.
     * <p>
     * This base function is used to display the information common to the
     * all library items.  Call the base class from the derived class or the
     * common information will not be updated in the message panel.
     * </p>
     * @param aList is the list to populate.
     */
    virtual void GetMsgPanelInfo( std::vector< MSG_PANEL_ITEM >& aList );

    /**
     * Test LIB_ITEM objects for equivalence.
     *
     * @param aOther Object to test against.
     * @return True if object is identical to this object.
     */
    bool operator==( const LIB_ITEM& aOther ) const;
    bool operator==( const LIB_ITEM* aOther ) const
    {
        return *this == *aOther;
    }

    /**
     * Test if another draw item is less than this draw object.
     *
     * @param aOther - Draw item to compare against.
     * @return - True if object is less than this object.
     */
    bool operator<( const LIB_ITEM& aOther) const;

    /**
     * Function Offset
     * sets the drawing object by \a aOffset from the current position.
     *
     * @param aOffset Coordinates to offset the item position.
     */
    virtual void SetOffset( const wxPoint& aOffset ) = 0;

    /**
     * Function Inside
     * tests if any part of the draw object is inside rectangle bounds of \a aRect.
     *
     * @param aRect Rectangle to check against.
     * @return True if object is inside rectangle.
     */
    virtual bool Inside( EDA_RECT& aRect ) const = 0;

    /**
     * Function Move
     * moves a draw object to \a aPosition.
     *
     * @param aPosition Position to move draw item to.
     */
    virtual void Move( const wxPoint& aPosition ) = 0;

    /**
     * Function GetPosition
     * returns the current draw object position.
     *
     * @return A wxPoint object containing the position of the object.
     */
    virtual wxPoint GetPosition() const = 0;

    void SetPosition( const wxPoint& aPosition ) { Move( aPosition ); }

    /**
     * Function MirrorHorizontal
     * mirrors the draw object along the horizontal (X) axis about \a aCenter point.
     *
     * @param aCenter Point to mirror around.
     */
    virtual void MirrorHorizontal( const wxPoint& aCenter ) = 0;

    /**
     * Function MirrorVertical
     * mirrors the draw object along the MirrorVertical (Y) axis about \a aCenter point.
     *
     * @param aCenter Point to mirror around.
     */
    virtual void MirrorVertical( const wxPoint& aCenter ) = 0;

    /**
     * Function Rotate
     * rotates the object about \a aCenter point.
     *
     * @param aCenter Point to rotate around.
     * @param aRotateCCW True to rotate counter clockwise.  False to rotate clockwise.
     */
    virtual void Rotate( const wxPoint& aCenter, bool aRotateCCW = true ) = 0;

    /**
     * Rotate the draw item.
     */
    virtual void Rotate() {}

    /**
     * Plot the draw item using the plot object.
     *
     * @param aPlotter The plot object to plot to.
     * @param aOffset Plot offset position.
     * @param aFill Flag to indicate whether or not the object is filled.
     * @param aTransform The plot transform.
     */
    virtual void Plot( PLOTTER* aPlotter, const wxPoint& aOffset, bool aFill,
                       const TRANSFORM& aTransform ) = 0;

    /**
     * Function GetWidth
     * return the width of the draw item.
     *
     * @return Width of draw object.
     */
    virtual int GetWidth() const = 0;

    /**
     * Function SetWidth
     * sets the width of the draw item to \a aWidth.
     */
    virtual void SetWidth( int aWidth ) = 0;

    /**
     * Check if draw object can be filled.
     *
     * The default setting is false.  If the derived object support filling,
     * set the m_isFillable member to true.
     *
     * @return True if draw object can be filled.  Default is false.
     */
    bool IsFillable() const { return m_isFillable; }

    /**
     * Return the draw item editing mode status.
     *
     * @return True if the item is being edited.
     */
    bool InEditMode() const { return ( m_Flags & ( IS_NEW | IS_DRAGGED | IS_MOVED | IS_RESIZED ) ) != 0; }

    void SetEraseLastDrawItem( bool aErase = true ) { m_eraseLastDrawItem = aErase; }

    virtual EDA_COLOR_T GetDefaultColor();

    void SetUnit( int aUnit ) { m_Unit = aUnit; }

    int GetUnit() const { return m_Unit; }

    void SetConvert( int aConvert ) { m_Convert = aConvert; }

    int GetConvert() const { return m_Convert; }

    void SetFillMode( FILL_T aFillMode ) { m_Fill = aFillMode; }

    FILL_T GetFillMode() const { return m_Fill; }

#if defined(DEBUG)
    void Show( int nestLevel, std::ostream& os ) const { ShowDummy( os ); } // override
#endif

private:

    /**
     * Function compare
     * provides the draw object specific comparison called by the == and < operators.
     *
     * The base object sort order which always proceeds the derived object sort order
     * is as follows:
     *      - Component alternate part (DeMorgan) number.
     *      - Component part number.
     *      - KICAD_T enum value.
     *      - Result of derived classes comparison.
     *
     * @param aOther A reference to the other #LIB_ITEM to compare the arc against.
     * @return An integer value less than 0 if the object is less than \a aOther ojbect,
     *         zero if the object is equal to \a aOther object, or greater than 0 if the
     *         object is greater than \a aOther object.
     */
    virtual int compare( const LIB_ITEM& aOther ) const = 0;
};


#endif  //  _LIB_ITEM_H_
