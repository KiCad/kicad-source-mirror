/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2004-2019 KiCad Developers, see change_log.txt for contributors.
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

#ifndef SCH_ITEM_H
#define SCH_ITEM_H

#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <base_screen.h>
#include <general.h>
#include <sch_sheet_path.h>

class SCH_ITEM;
class SCH_CONNECTION;
class LINE_READER;
class SCH_EDIT_FRAME;
class wxFindReplaceData;
class PLOTTER;
class NETLIST_OBJECT;
class NETLIST_OBJECT_LIST;


enum DANGLING_END_T {
    UNKNOWN = 0,
    WIRE_START_END,
    WIRE_END_END,
    BUS_START_END,
    BUS_END_END,
    JUNCTION_END,
    PIN_END,
    LABEL_END,
    BUS_ENTRY_END,
    WIRE_ENTRY_END,
    SHEET_LABEL_END,
    NO_CONNECT_END,
};


/**
 * Class DANGLING_END_ITEM
 * is a helper class used to store the state of schematic  items that can be connected to
 * other schematic items.
 */
class DANGLING_END_ITEM
{
private:
    /// A pointer to the connectable object.
    EDA_ITEM* m_item;

    /// The position of the connection point.
    wxPoint        m_pos;

    /// The type of connection of #m_item.
    DANGLING_END_T m_type;

    /// A pointer to the parent object (in the case of pins)
    const EDA_ITEM* m_parent;

public:
    DANGLING_END_ITEM( DANGLING_END_T aType, EDA_ITEM* aItem, const wxPoint& aPosition )
    {
        m_item = aItem;
        m_type = aType;
        m_pos = aPosition;
        m_parent = aItem;
    }

    DANGLING_END_ITEM( DANGLING_END_T aType, EDA_ITEM* aItem,
            const wxPoint& aPosition, const EDA_ITEM* aParent )
    {
        m_item = aItem;
        m_type = aType;
        m_pos = aPosition;
        m_parent = aParent;
    }

    wxPoint GetPosition() const { return m_pos; }
    EDA_ITEM* GetItem() const { return m_item; }
    const EDA_ITEM* GetParent() const { return m_parent; }
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
    friend class CONNECTION_GRAPH;

protected:
    SCH_LAYER_ID   m_Layer;
    EDA_ITEMS      m_connections;   ///< List of items connected to this item.
    wxPoint        m_storedPos;     ///< a temporary variable used in some move commands
                                    ///> to store a initial pos (of the item or mouse cursor)

    /// Stores pointers to other items that are connected to this one (schematic only)
    std::unordered_set<SCH_ITEM*> m_connected_items;

    /// Stores connectivity information, per sheet
    std::unordered_map<SCH_SHEET_PATH, SCH_CONNECTION*> m_connection_map;

    /// True if connectivity info might be out of date
    bool m_connectivity_dirty;

public:
    SCH_ITEM( EDA_ITEM* aParent, KICAD_T aType );

    SCH_ITEM( const SCH_ITEM& aItem );

    ~SCH_ITEM();

    virtual wxString GetClass() const override
    {
        return wxT( "SCH_ITEM" );
    }

    /**
     * Function SwapData
     * swap the internal data structures \a aItem with the schematic item.
     * Obviously, aItem must have the same type than me
     * @param aItem The item to swap the data structures with.
     */
    virtual void SwapData( SCH_ITEM* aItem );

    SCH_ITEM* Next() const { return static_cast<SCH_ITEM*>( Pnext ); }
    SCH_ITEM* Back() const { return static_cast<SCH_ITEM*>( Pback ); }

    /**
     * Routine to create a new copy of given item.
     * The new object is not put in draw list (not linked).
     *
     * @param doClone (default = false) indicates unique values (such as timestamp and
     *     sheet name) should be duplicated.  Use only for undo/redo operations.
     */
    SCH_ITEM* Duplicate( bool doClone = false );

    /**
     * Virtual function IsMovableFromAnchorPoint
     * @return true for items which are moved with the anchor point at mouse cursor
     *  and false for items moved with no reference to anchor
     * Usually return true for small items (labels, junctions) and false for
     * items which can be large (hierarchical sheets, compoments)
     */
    virtual bool IsMovableFromAnchorPoint() { return true; }

    wxPoint& GetStoredPos() { return m_storedPos; }
    void     SetStoredPos( wxPoint aPos ) { m_storedPos = aPos; }

    /**
     * Function IsLocked
     * @return bool - true if the object is locked, else false
     */
    virtual bool IsLocked() const { return false; }

    /**
     * Function SetLocked
     * modifies 'lock' status for of the item.
     */
    virtual void SetLocked( bool aLocked ) {}

    /**
     * Function GetLayer
     * returns the layer this item is on.
     */
    SCH_LAYER_ID GetLayer() const { return m_Layer; }

    /**
     * Function SetLayer
     * sets the layer this item is on.
     * @param aLayer The layer number.
     */
    void SetLayer( SCH_LAYER_ID aLayer )  { m_Layer = aLayer; }

    /**
     * Function ViewGetLayers
     * returns the layers the item is drawn on (which may be more than its "home" layer)
     */
    void ViewGetLayers( int aLayers[], int& aCount ) const override;

    /**
     * Function GetPenSize virtual pure
     * @return the size of the "pen" that be used to draw or plot this item
     */
    virtual int GetPenSize() const { return 0; }

    /**
     * Function Print
     * Print a schematic item. Each schematic item should have its own method
     * @param aDC Device Context (can be null)
     * @param aOffset drawing Offset (usually wxPoint(0,0),
     *  but can be different when moving an object)
     */
    virtual void Print( wxDC* aDC, const wxPoint&  aOffset ) = 0;

    /**
     * Function Move
     * moves the item by \a aMoveVector to a new position.
     * @param aMoveVector = the displacement vector
     */
    virtual void Move( const wxPoint& aMoveVector ) = 0;

    /**
     * Function MirrorY
     * mirrors item relative to the Y axis about \a aYaxis_position.
     * @param aYaxis_position The Y axis position to mirror around.
     */
    virtual void MirrorY( int aYaxis_position ) = 0;

    /**
     * Function MirrorX
     * mirrors item relative to the X axis about \a aXaxis_position.
     * @param aXaxis_position The X axis position to mirror around.
     */
    virtual void MirrorX( int aXaxis_position ) = 0;

    /**
     * Function Rotate
     * rotates the item around \a aPosition 90 degrees in the clockwise direction.
     * @param aPosition A reference to a wxPoint object containing the coordinates to
     *                  rotate around.
     */
    virtual void Rotate( wxPoint aPosition ) = 0;

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
    virtual void GetEndPoints( std::vector< DANGLING_END_ITEM >& aItemList ) {}

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
    virtual bool UpdateDanglingState( std::vector<DANGLING_END_ITEM>& aItemList ) { return false; }

    virtual bool IsDangling() const { return false; }

    virtual bool CanConnect( const SCH_ITEM* aItem ) const { return m_Layer == aItem->GetLayer(); }

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
    virtual void GetConnectionPoints( std::vector< wxPoint >& aPoints ) const { }

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
     * @param aPoint A reference to a wxPoint object containing the coordinates to test.
     * @return True if connection to \a aPoint exists.
     */
    bool IsConnected( const wxPoint& aPoint ) const;

    /**
     * Retrieves the connection associated with this object in the given sheet
     */
    SCH_CONNECTION* Connection( const SCH_SHEET_PATH& aPath ) const;

    /**
     * Retrieves the set of items connected to this item (schematic only)
     */
    std::unordered_set<SCH_ITEM*>& ConnectedItems();

    /**
     * Adds a connection link between this item and another
     */
    void AddConnectionTo( SCH_ITEM* aItem );

    /**
     * Creates a new connection object associated with this object
     *
     * @param aPath is the sheet path to initialize
     */
    SCH_CONNECTION* InitializeConnection( const SCH_SHEET_PATH& aPath );

    /**
     * Returns true if this item should propagate connection info to aItem
     */
    virtual bool ConnectionPropagatesTo( const EDA_ITEM* aItem ) const { return true; }

    bool IsConnectivityDirty() { return m_connectivity_dirty; }

    void SetConnectivityDirty( bool aDirty = true ) { m_connectivity_dirty = aDirty; }

    virtual bool CanIncrementLabel() const { return false; }

    /**
     * Function Plot
     * plots the schematic item to \a aPlotter.
     *
     * @param aPlotter A pointer to a #PLOTTER object.
     */
    virtual void Plot( PLOTTER* aPlotter );

    /**
     * Function GetNetListItem
     * creates a new #NETLIST_OBJECT for the schematic object and adds it to
     * \a aNetListItems.
     * <p>
     * Not all schematic objects have net list items associated with them.  This
     * method only needs to be overridden for those schematic objects that have
     * net list objects associated with them.
     * </p>
     */
    virtual void GetNetListItem( NETLIST_OBJECT_LIST& aNetListItems,
                                 SCH_SHEET_PATH*      aSheetPath ) { }

    /**
     * Function GetPosition
     * @return A wxPoint object containing the schematic item position.
     */
    virtual wxPoint GetPosition() const = 0;

    /**
     * Function SetPosition
     * set the schematic item position to \a aPosition.
     *
     * @param aPosition A reference to a wxPoint object containing the new position.
     */
    virtual void SetPosition( const wxPoint& aPosition ) = 0;

    virtual bool operator <( const SCH_ITEM& aItem ) const;

private:
    /**
     * Function doIsConnected
     * provides the object specific test to see if it is connected to \a aPosition.
     *
     * @note Override this function if the derived object can be connect to another
     *       object such as a wire, bus, or junction.  Do not override this function
     *       for objects that cannot have connections.  The default will always return
     *       false.  This functions is call through the public function IsConnected()
     *       which performs tests common to all schematic items before calling the
     *       item specific connection testing.
     *
     * @param aPosition A reference to a wxPoint object containing the test position.
     * @return True if connection to \a aPosition exists.
     */
    virtual bool doIsConnected( const wxPoint& aPosition ) const { return false; }
};

#endif /* SCH_ITEM_H */
