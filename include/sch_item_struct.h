/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2008-2011 Wayne Stambaugh <stambaughw@verizon.net>
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
 * @file sch_item_struct.h
 * @brief Base schematic object class definition.
 */

#ifndef SCH_ITEM_STRUCT_H
#define SCH_ITEM_STRUCT_H

#include <vector>
#include <class_base_screen.h>

using namespace std;


class SCH_ITEM;
class SCH_SHEET_PATH;
class LINE_READER;
class SCH_EDIT_FRAME;
class wxFindReplaceData;
class PLOTTER;
class NETLIST_OBJECT;


typedef boost::ptr_vector< SCH_ITEM > SCH_ITEMS;
typedef SCH_ITEMS::iterator SCH_ITEMS_ITR;
typedef vector< SCH_ITEMS_ITR > SCH_ITEMS_ITRS;


/* used to calculate the pen size from default value
 * the actual pen size is default value * BUS_WIDTH_EXPAND
 */
#if defined(KICAD_GOST)
#define BUS_WIDTH_EXPAND 3.6
#else
#define BUS_WIDTH_EXPAND 1.4
#endif


/// Flag to enable find and replace tracing using the WXTRACE environment variable.
extern const wxString traceFindReplace;

/// Flag to enable find item tracing using the WXTRACE environment variable.  This
/// flag generates a lot of debug output.
extern const wxString traceFindItem;


enum DANGLING_END_T {
    UNKNOWN = 0,
    WIRE_START_END,
    WIRE_END_END,
    BUS_START_END,
    BUS_END_END,
    JUNCTION_END,
    PIN_END,
    LABEL_END,
    ENTRY_END,
    SHEET_LABEL_END
};


/**
 * Class DANLIGN_END_ITEM
 * is a helper class used to store the state of schematic  items that can be connected to
 * other schematic items.
 */
class DANGLING_END_ITEM
{
    /// A pointer to the connectable ojbect.
    const void*    m_item;

    /// The position of the connection point.
    wxPoint        m_pos;

    /// The type of connection of #m_item.
    DANGLING_END_T m_type;

public:
    DANGLING_END_ITEM( DANGLING_END_T aType, const void* aItem, const wxPoint& aPosition )
    {
        m_item = aItem;
        m_type = aType;
        m_pos = aPosition;
    }

    wxPoint GetPosition() const { return m_pos; }
    const void* GetItem() const { return m_item; }
    DANGLING_END_T GetType() const { return m_type; }
};


/**
 * Class SCH_ITEM
 * is a base class for any item which can be embedded within the SCHEMATIC
 * container class, and therefore instances of derived classes should only be
 * found in EESCHEMA or other programs that use class SCHEMATIC and its contents.
 * The corresponding class in Pcbnew is BOARD_ITEM.
 */
class SCH_ITEM : public EDA_ITEM
{
protected:
    int            m_Layer;
    EDA_ITEMS      m_connections;   ///< List of items connected to this item.

public:
    SCH_ITEM( EDA_ITEM* aParent, KICAD_T aType );

    SCH_ITEM( const SCH_ITEM& aItem );

    ~SCH_ITEM();

    virtual wxString GetClass() const
    {
        return wxT( "SCH_ITEM" );
    }

    SCH_ITEM* Clone() const { return ( SCH_ITEM* ) EDA_ITEM::Clone(); }

    /**
     * Function SwapData
     * swap the internal data structures \a aItem with the schematic item.
     * Obviously, aItem must have the same type than me
     * @param aItem The item to swap the data structures with.
     */
    virtual void SwapData( SCH_ITEM* aItem );

    SCH_ITEM* Next() { return (SCH_ITEM*) Pnext; }
    SCH_ITEM* Back() { return (SCH_ITEM*) Pback; }

    /**
     * Function GetLayer
     * returns the layer this item is on.
     */
    int GetLayer() const { return m_Layer; }

    /**
     * Function SetLayer
     * sets the layer this item is on.
     * @param aLayer The layer number.
     */
    void SetLayer( int aLayer )  { m_Layer = aLayer; }

    /**
     * Function GetPenSize virtual pure
     * @return the size of the "pen" that be used to draw or plot this item
     */
    virtual int GetPenSize() const { return 0; }

    /**
     * Function Draw
     */
    virtual void Draw( EDA_DRAW_PANEL* aPanel,
                       wxDC*           aDC,
                       const wxPoint&  aOffset,
                       int             aDrawMode,
                       int             aColor = -1 ) = 0;

    /**
     * Function Move
     * moves the item by \a aMoveVector to a new position.
     * @param aMoveVector = the displacement vector
     */
    virtual void Move( const wxPoint& aMoveVector ) = 0;

    /**
     * Function Mirror_Y
     * mirrors item relative to an Y axis about \a aYaxis_position.
     * @param aYaxis_position The Y axis position to mirror around.
     */
    virtual void Mirror_Y( int aYaxis_position ) = 0;

    virtual void Mirror_X( int aXaxis_position ) = 0;

    virtual void Rotate( wxPoint rotationPoint ) = 0;

    /**
     * Function Save
     * writes the data structures for this object out to a FILE in "*.sch" format.
     * @param aFile The FILE to write to.
     * @return bool - true if success writing else false.
     */
    virtual bool Save( FILE* aFile ) const = 0;

    /**
     * Function Load
     * reads a schematic item from \a aLine in a .sch file.
     *
     * @param aLine - Essentially this is file to read the object from.
     * @param aErrorMsg - Description of the error if an error occurs while loading the object.
     * @return True if the object loaded successfully.
     */
    virtual bool Load( LINE_READER& aLine, wxString& aErrorMsg ) { return false; }

    /**
     * Function GetEndPoints
     * adds the schematic item end points to \a aItemList if the item has end points.
     *
     * The default version doesn't do anything since many of the schematic object cannot
     * be tested for dangling ends.  If you add a new schematic item that can have a
     * dangling end ( no connect ), override this method to provide the correct end
     * points.
     *
     * @param aItemList - List of DANGLING_END_ITEMS to add to.
     */
    virtual void GetEndPoints( vector< DANGLING_END_ITEM >& aItemList ) {}

    /**
     * Function IsDanglingStateChanged
     * tests the schematic item to \a aItemList to check if it's dangling state has changed.
     *
     * Note that the return value only true when the state of the test has changed.  Use
     * the IsDangling() method to get the current dangling state of the item.  Some of
     * the schematic objects cannot be tested for a dangling state, the default method
     * always returns false.  Only override the method if the item can be tested for a
     * dangling state.
     *
     * @param aItemList - List of items to test item against.
     * @return True if the dangling state has changed from it's current setting.
     */
    virtual bool IsDanglingStateChanged( vector< DANGLING_END_ITEM >& aItemList ) { return false; }

    virtual bool IsDangling() const { return false; }

    /**
     * Function IsSelectStateChanged
     * checks if the selection state of an item inside \a aRect has changed.
     *
     * This is used by the block selection code to verify if an item is selected or not.
     * True is be return anytime the select state changes.  If you need to know the
     * the current selection state, use the IsSelected() method.
     *
     * @param aRect - Rectangle to test against.
     */
    virtual bool IsSelectStateChanged( const wxRect& aRect ) { return false; }

    /**
     * Function IsConnectable
     * returns true if the schematic item can connect to another schematic item.
     */
    virtual bool IsConnectable() const { return false; }

    /**
     * Function GetConnectionPoints
     * add all the connection points for this item to \a aPoints.
     *
     * Not all schematic items have connection points so the default method does nothing.
     *
     * @param aPoints List of connection points to add to.
     */
    virtual void GetConnectionPoints( vector< wxPoint >& aPoints ) const { }

    /**
     * Function ClearConnections
     * clears all of the connection items from the list.
     *
     * The vector release method is used to prevent the item pointers from being deleted.
     * Do not use the vector erase method on the connection list.
     */
    void ClearConnections() { m_connections.clear(); }

    /**
     * Function IsConnected
     * tests the item to see if it is connected to \a aPoint.
     *
     * @param aPoint - Position to test for connection.
     * @return True if connection to \a aPoint exists.
     */
    bool IsConnected( const wxPoint& aPoint ) const;

    virtual bool HitTest( const wxPoint& aPosition ) { return HitTest( aPosition, 0 ); }

    /**
     * Function HitTest
     * tests if \a aPoint is contained within or on the bounding box of an item.
     *
     * @param aPoint - Point to test.
     * @param aAccuracy - Increase the item bounding box by this amount.
     * @return True if \a aPoint is within the item bounding box.
     */
    bool HitTest( const wxPoint& aPoint, int aAccuracy = 0 ) const
    {
        return doHitTest( aPoint, aAccuracy );
    }

    /**
     * Function HitTest
     * tests if \a aRect intersects or is contained within the bounding box of an item.
     *
     * @param aRect - Rectangle to test.
     * @param aContained - Set to true to test for containment instead of an intersection.
     * @param aAccuracy - Increase aRect by this amount.
     * @return True if \a aRect contains or intersects the item bounding box.
     */
    bool HitTest( const EDA_RECT& aRect, bool aContained = false, int aAccuracy = 0 ) const
    {
        return doHitTest( aRect, aContained, aAccuracy );
    }

    virtual bool CanIncrementLabel() const { return false; }

    void Plot( PLOTTER* aPlotter ) { doPlot( aPlotter ); }

    /**
     * Function GetNetListItem
     * creates a new #NETLIST_OBJECT for the schematic object and adds it to
     * \a aNetListItems.
     * <p>
     * Not all schematic objects have net list items associated with them.  This
     * method only needs to be overridden for those schematic objects that have
     * net list objects associated with them.
     */
    virtual void GetNetListItem( vector<NETLIST_OBJECT*>& aNetListItems,
                                 SCH_SHEET_PATH*          aSheetPath ) { }

    /**
     * Function GetPosition
     * @return the schematic item position.
     */
    wxPoint GetPosition() const { return doGetPosition(); }

    /**
     * Function SetPosition
     * set the schematic item position to \a aPosition.
     *
     * @param aPosition A reference to a wxPoint object containing the new position.
     */
    void SetPosition( const wxPoint& aPosition ) { doSetPosition( aPosition ); }

    virtual bool operator <( const SCH_ITEM& aItem ) const;

    virtual SCH_ITEM& operator=( const SCH_ITEM& aItem );

    /**
     * @note - The DoXXX() functions below are used to enforce the interface while retaining
     *         the ability of change the implementation behavior of derived classes.  See
     *         Herb Sutters explanation of virtuality as to why you might want to do this at:
     *         http://www.gotw.ca/publications/mill18.htm.
     */
private:
    virtual bool doHitTest( const wxPoint& aPoint, int aAccuracy ) const
    {
        return false;
    }

    virtual bool doHitTest( const EDA_RECT& aRect, bool aContained, int aAccuracy ) const
    {
        return false;
    }

    virtual bool doIsConnected( const wxPoint& aPosition ) const { return false; }

    virtual void doPlot( PLOTTER* aPlotter );

    virtual wxPoint doGetPosition() const = 0;

    virtual void doSetPosition( const wxPoint& aPosition ) = 0;
};


extern bool sort_schematic_items( const SCH_ITEM* aItem1, const SCH_ITEM* aItem2 );


#endif /* SCH_ITEM_STRUCT_H */
