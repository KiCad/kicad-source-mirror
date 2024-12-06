/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2004-2024 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <map>
#include <set>

#include <eda_item.h>
#include <default_values.h>
#include <sch_sheet_path.h>
#include <netclass.h>
#include <stroke_params.h>
#include <layer_ids.h>

class CONNECTION_GRAPH;
class SCH_CONNECTION;
class SCH_SHEET_PATH;
class SCHEMATIC;
class LINE_READER;
class SCH_EDIT_FRAME;
class PLOTTER;
struct SCH_PLOT_SETTINGS;
class NETLIST_OBJECT_LIST;
class PLOTTER;

namespace KIFONT
{
class METRICS;
}

using KIGFX::RENDER_SETTINGS;


enum FIELDS_AUTOPLACED
{
    FIELDS_AUTOPLACED_NO = 0,
    FIELDS_AUTOPLACED_AUTO,
    FIELDS_AUTOPLACED_MANUAL
};


enum DANGLING_END_T
{
    DANGLING_END_UNKNOWN = 0,
    WIRE_END,
    BUS_END,
    JUNCTION_END,
    PIN_END,
    LABEL_END,
    BUS_ENTRY_END,
    WIRE_ENTRY_END,
    SHEET_LABEL_END,
    NO_CONNECT_END,
};


/**
 * Helper class used to store the state of schematic items that can be connected to
 * other schematic items.
 */
class DANGLING_END_ITEM
{
public:
    DANGLING_END_ITEM( DANGLING_END_T aType, EDA_ITEM* aItem, const VECTOR2I& aPosition )
    {
        m_item = aItem;
        m_type = aType;
        m_pos = aPosition;
        m_parent = aItem;
    }

    DANGLING_END_ITEM( DANGLING_END_T aType, EDA_ITEM* aItem, const VECTOR2I& aPosition,
                       const EDA_ITEM* aParent )
    {
        m_item = aItem;
        m_type = aType;
        m_pos = aPosition;
        m_parent = aParent;
    }

    bool operator==( const DANGLING_END_ITEM& aB ) const
    {
        return GetItem() == aB.GetItem()
            && GetPosition() == aB.GetPosition()
            && GetType() == aB.GetType()
            && GetParent() == aB.GetParent();
    }

    bool operator!=( const DANGLING_END_ITEM& aB ) const
    {
        return GetItem() != aB.GetItem()
                || GetPosition() != aB.GetPosition()
                || GetType() != aB.GetType()
                || GetParent() != aB.GetParent();;
    }

    bool operator<( const DANGLING_END_ITEM& rhs ) const
    {
        return( m_pos.x < rhs.m_pos.x || ( m_pos.x == rhs.m_pos.x && m_pos.y < rhs.m_pos.y )
                || ( m_pos == rhs.m_pos && m_item < rhs.m_item ) );
    }

    VECTOR2I GetPosition() const { return m_pos; }
    EDA_ITEM* GetItem() const { return m_item; }
    const EDA_ITEM* GetParent() const { return m_parent; }
    DANGLING_END_T GetType() const { return m_type; }

private:
    EDA_ITEM*       m_item;         /// A pointer to the connectable object.
    VECTOR2I        m_pos;          /// The position of the connection point.
    DANGLING_END_T  m_type;         /// The type of connection of #m_item.
    const EDA_ITEM* m_parent;       /// A pointer to the parent object (in the case of pins)
};


class DANGLING_END_ITEM_HELPER
{
public:
    static std::vector<DANGLING_END_ITEM>::iterator
    get_lower_pos( std::vector<DANGLING_END_ITEM>& aItemListByPos, const VECTOR2I& aPos );

    static std::vector<DANGLING_END_ITEM>::iterator
    get_lower_type( std::vector<DANGLING_END_ITEM>& aItemListByType, const DANGLING_END_T& aType );

    /** Both contain the same information */
    static void sort_dangling_end_items( std::vector<DANGLING_END_ITEM>& aItemListByType,
                                         std::vector<DANGLING_END_ITEM>& aItemListByPos );
};

typedef std::vector<SCH_ITEM*> SCH_ITEM_VEC;


/**
 * Base class for any item which can be embedded within the #SCHEMATIC container class,
 * and therefore instances of derived classes should only be found in EESCHEMA or other
 * programs that use class SCHEMATIC and its contents.
 *
 * The corresponding class in Pcbnew is #BOARD_ITEM.
 */
class SCH_ITEM : public EDA_ITEM
{
public:
    SCH_ITEM( EDA_ITEM* aParent, KICAD_T aType );

    SCH_ITEM( const SCH_ITEM& aItem );

    SCH_ITEM& operator=( const SCH_ITEM& aPin );

    virtual ~SCH_ITEM();

    virtual wxString GetClass() const override
    {
        return wxT( "SCH_ITEM" );
    }

    bool IsType( const std::vector<KICAD_T>& aScanTypes ) const override
    {
        if( EDA_ITEM::IsType( aScanTypes ) )
            return true;

        for( KICAD_T scanType : aScanTypes )
        {
            if( scanType == SCH_ITEM_LOCATE_WIRE_T && m_layer == LAYER_WIRE )
                return true;

            if ( scanType == SCH_ITEM_LOCATE_BUS_T && m_layer == LAYER_BUS )
                return true;

            if ( scanType == SCH_ITEM_LOCATE_GRAPHIC_LINE_T
                    && Type() == SCH_LINE_T && m_layer == LAYER_NOTES )
            {
                return true;
            }
        }

        return false;
    }

    /**
     * Swap the internal data structures \a aItem with the schematic item.
     * Obviously, aItem must have the same type than me.
     * @param aItem The item to swap the data structures with.
     */
    virtual void SwapData( SCH_ITEM* aItem );

    /**
     * Swap the non-temp and non-edit flags.
     */
    void SwapFlags( SCH_ITEM* aItem );

    /**
     * Routine to create a new copy of given item.
     * The new object is not put in draw list (not linked).
     *
     * @param doClone (default = false) indicates unique values (such as timestamp and
     *                sheet name) should be duplicated.  Use only for undo/redo operations.
     */
    SCH_ITEM* Duplicate( bool doClone = false ) const;

    virtual void SetExcludedFromSim( bool aExclude ) { }
    virtual bool GetExcludedFromSim() const { return false; }

    /**
     * @return true for items which are moved with the anchor point at mouse cursor
     *  and false for items moved with no reference to anchor
     * Usually return true for small items (labels, junctions) and false for
     * items which can be large (hierarchical sheets, symbols)
     */
    virtual bool IsMovableFromAnchorPoint() const { return true; }

    VECTOR2I& GetStoredPos() { return m_storedPos; }
    void     SetStoredPos( const VECTOR2I& aPos ) { m_storedPos = aPos; }

    /**
     * Searches the item hierarchy to find a SCHEMATIC.
     *
     * Every SCH_ITEM that lives on a SCH_SCREEN should be parented to either that screen
     * or another SCH_ITEM on the same screen (for example, pins to their symbols).
     *
     * Every SCH_SCREEN should be parented to the SCHEMATIC.
     *
     * @note This hierarchy is not the same as the sheet hierarchy!
     *
     * @return the parent schematic this item lives on, or nullptr.
     */
    SCHEMATIC* Schematic() const;

    /**
     * @return true if the object is locked, else false.
     */
    virtual bool IsLocked() const { return false; }

    /**
     * Set the 'lock' status to \a aLocked for of this item.
     */
    virtual void SetLocked( bool aLocked ) {}

    /**
     * Allow items to support hypertext actions when hovered/clicked.
     */
    virtual bool IsHypertext() const { return false; }

    virtual void DoHypertextAction( EDA_DRAW_FRAME* aFrame ) const { }

    /**
     * Return the layer this item is on.
     */
    SCH_LAYER_ID GetLayer() const { return m_layer; }

    /**
     * Set the layer this item is on.
     *
     * @param aLayer The layer number.
     */
    void SetLayer( SCH_LAYER_ID aLayer ) { m_layer = aLayer; }

    /**
     * Return the layers the item is drawn on (which may be more than its "home" layer)
     */
    void ViewGetLayers( int aLayers[], int& aCount ) const override;

    /**
     * @return the size of the "pen" that be used to draw or plot this item
     */
    virtual int GetPenWidth() const { return 0; }

    const wxString& GetDefaultFont() const;

    const KIFONT::METRICS& GetFontMetrics() const;

    bool RenderAsBitmap( double aWorldScale ) const override;

    /**
     * Return a measure of how likely the other object is to represent the same
     * object.  The scale runs from 0.0 (definitely different objects) to 1.0 (same)
     *
     * This is a pure virtual function.  Derived classes must implement this.
    */
    virtual double Similarity( const SCH_ITEM& aItem ) const = 0;
    virtual bool operator==( const SCH_ITEM& aItem ) const = 0;

    /**
     * Print a schematic item.
     *
     * Each schematic item should have its own method
     *
     * @param aOffset is the drawing offset (usually {0,0} but can be different when moving an
     *                object).
     */
    virtual void Print( const RENDER_SETTINGS* aSettings, const VECTOR2I& aOffset ) = 0;

    /**
     * Print the (optional) backaground elements if they exist
     * @param aSettings Print settings
     * @param aOffset is the drawing offset (usually {0,0} but can be different when moving an
     *                object).
     */

    virtual void PrintBackground( const RENDER_SETTINGS* aSettings, const VECTOR2I& aOffset ) {};

    /**
     * Move the item by \a aMoveVector to a new position.
     */
    virtual void Move( const VECTOR2I& aMoveVector ) = 0;

    /**
     * Mirror item horizontally about \a aCenter.
     */
    virtual void MirrorHorizontally( int aCenter ) = 0;

    /**
     * Mirror item vertically about \a aCenter.
     */
    virtual void MirrorVertically( int aCenter ) = 0;

    /**
     * Rotate the item around \a aCenter 90 degrees in the clockwise direction.
     */
    virtual void Rotate( const VECTOR2I& aCenter ) = 0;

    /**
     * Add the schematic item end points to \a aItemList if the item has end points.
     *
     * The default version doesn't do anything since many of the schematic object cannot
     * be tested for dangling ends.  If you add a new schematic item that can have a
     * dangling end ( no connect ), override this method to provide the correct end
     * points.
     *
     * @param aItemList is the list of DANGLING_END_ITEMS to add to.
     */
    virtual void GetEndPoints( std::vector< DANGLING_END_ITEM >& aItemList ) {}

    /**
     * Test the schematic item to \a aItemList to check if it's dangling state has changed.
     *
     * Note that the return value only true when the state of the test has changed.  Use
     * the IsDangling() method to get the current dangling state of the item.  Some of
     * the schematic objects cannot be tested for a dangling state, the default method
     * always returns false.  Only override the method if the item can be tested for a
     * dangling state.
     *
     * If aSheet is passed a non-null pointer to a SCH_SHEET_PATH, the overridden method can
     * optionally use it to update sheet-local connectivity information
     *
     * @param aItemListByType is the list of items to test item against. It's sorted
     *   by item type, keeping WIRE_END pairs together.
     * @param aItemListByPos is the same list but sorted first by Y then by X.
     * @param aSheet is the sheet path to update connections for.
     * @return True if the dangling state has changed from it's current setting.
     */
    virtual bool UpdateDanglingState( std::vector<DANGLING_END_ITEM>& aItemListByType,
                                      std::vector<DANGLING_END_ITEM>& aItemListByPos,
                                      const SCH_SHEET_PATH*           aPath = nullptr )
    {
        return false;
    }

    virtual bool IsDangling() const { return false; }

    virtual bool CanConnect( const SCH_ITEM* aItem ) const { return m_layer == aItem->GetLayer(); }

    /**
     * @return true if the schematic item can connect to another schematic item.
     */
    virtual bool IsConnectable() const { return false; }

    /**
     * @return true if the given point can start drawing (usually means the anchor is
     *         unused/free/dangling).
     */
    virtual bool IsPointClickableAnchor( const VECTOR2I& aPos ) const { return false; }

    /**
     * Add all the connection points for this item to \a aPoints.
     *
     * Not all schematic items have connection points so the default method does nothing.
     *
     * @param aPoints is the list of connection points to add to.
     */
    virtual std::vector<VECTOR2I> GetConnectionPoints() const { return {}; }

    /**
     * Clears all of the connection items from the list.
     *
     * The vector release method is used to prevent the item pointers from being deleted.
     * Do not use the vector erase method on the connection list.
     */
    void ClearConnections() { m_connections.clear(); }

    /**
     * Test the item to see if it is connected to \a aPoint.
     *
     * @param aPoint is a reference to a VECTOR2I object containing the coordinates to test.
     * @return True if connection to \a aPoint exists.
     */
    bool IsConnected( const VECTOR2I& aPoint ) const;

    /**
     * Retrieve the connection associated with this object in the given sheet.
     *
     * @note The returned value can be nullptr.
     */
    SCH_CONNECTION* Connection( const SCH_SHEET_PATH* aSheet = nullptr ) const;

    /**
     * Retrieve the set of items connected to this item on the given sheet.
     */
    const SCH_ITEM_VEC& ConnectedItems( const SCH_SHEET_PATH& aPath );

    /**
     * Add a connection link between this item and another.
     */
    void AddConnectionTo( const SCH_SHEET_PATH& aPath, SCH_ITEM* aItem );

    /**
     * Clear all connections to this item.
     */
    void ClearConnectedItems( const SCH_SHEET_PATH& aPath );

    /**
     * Create a new connection object associated with this object.
     *
     * @param aPath is the sheet path to initialize.
     */
    SCH_CONNECTION* InitializeConnection( const SCH_SHEET_PATH& aPath, CONNECTION_GRAPH* aGraph );

    SCH_CONNECTION* GetOrInitConnection( const SCH_SHEET_PATH& aPath, CONNECTION_GRAPH* aGraph );

    /**
     * Return true if this item should propagate connection info to \a aItem.
     */
    virtual bool ConnectionPropagatesTo( const EDA_ITEM* aItem ) const { return true; }

    bool IsConnectivityDirty() const { return m_connectivity_dirty; }

    void SetConnectivityDirty( bool aDirty = true ) { m_connectivity_dirty = aDirty; }

    /**
     * Check if \a aItem has connectivity changes against this object.
     *
     * This provides granular per object  connectivity change testing to prevent the need
     * to rebuild the #CONNECTION_GRAPH when object properties that have nothing to do with
     * the schematic connectivity changes i.e. color, thickness, fill type. etc.
     *
     * @note Developers should override this method for all objects that are connectable.
     *
     * @param aItem is the item to test for connectivity changes.
     * @param aInstance is the instance to test for connectivity changes.  This parameter is
     *                  only meaningful for #SCH_SYMBOL objects.
     *
     * @return true if there are connectivity changes otherwise false.
     */
    virtual bool HasConnectivityChanges( const SCH_ITEM* aItem,
                                         const SCH_SHEET_PATH* aInstance = nullptr ) const
    {
        return false;
    }

    /// Updates the connection graph for all connections in this item
    void SetConnectionGraph( CONNECTION_GRAPH* aGraph );

    virtual bool HasCachedDriverName() const { return false; }
    virtual const wxString& GetCachedDriverName() const;

    virtual void SetLastResolvedState( const SCH_ITEM* aItem ) { }

    std::shared_ptr<NETCLASS> GetEffectiveNetClass( const SCH_SHEET_PATH* aSheet = nullptr ) const;

    /**
     * Return whether the fields have been automatically placed.
     */
    FIELDS_AUTOPLACED GetFieldsAutoplaced() const { return m_fieldsAutoplaced; }

    void SetFieldsAutoplaced() { m_fieldsAutoplaced = FIELDS_AUTOPLACED_AUTO; }
    void ClearFieldsAutoplaced() { m_fieldsAutoplaced = FIELDS_AUTOPLACED_NO; }

    /**
     * Autoplace fields only if correct to do so automatically.
     *
     * Fields that have been moved by hand are not automatically placed.
     *
     * @param aScreen is the SCH_SCREEN associated with the current instance of the symbol.
     */
    void AutoAutoplaceFields( SCH_SCREEN* aScreen )
    {
        if( GetFieldsAutoplaced() )
            AutoplaceFields( aScreen, GetFieldsAutoplaced() == FIELDS_AUTOPLACED_MANUAL );
    }

    virtual void AutoplaceFields( SCH_SCREEN* aScreen, bool aManual ) { }

    virtual void RunOnChildren( const std::function<void( SCH_ITEM* )>& aFunction ) { }

    virtual void ClearCaches();

    /**
     * Check if this schematic item has line stoke properties.
     *
     * @see #STROKE_PARAMS
     *
     * @return true if this schematic item support line stroke properties.  Otherwise, false.
     */
    virtual bool HasLineStroke() const { return false; }

    virtual STROKE_PARAMS GetStroke() const { wxCHECK( false, STROKE_PARAMS() ); }

    virtual void SetStroke( const STROKE_PARAMS& aStroke ) { wxCHECK( false, /* void */ ); }

    /**
     * Plot the schematic item to \a aPlotter.
     *
     * @param aPlotter is the #PLOTTER object to plot to.
     * @param aBackground a poor-man's Z-order.  The routine will get called twice, first with
     *                    aBackground true and then with aBackground false.
     */
    virtual void Plot( PLOTTER* aPlotter, bool aBackground,
                       const SCH_PLOT_SETTINGS& aPlotSettings ) const;

    virtual bool operator <( const SCH_ITEM& aItem ) const;

private:
    friend class CONNECTION_GRAPH;

    /**
     * Provide the object specific test to see if it is connected to \a aPosition.
     *
     * @note Override this function if the derived object can be connect to another
     *       object such as a wire, bus, or junction.  Do not override this function
     *       for objects that cannot have connections.  The default will always return
     *       false.  This functions is call through the public function IsConnected()
     *       which performs tests common to all schematic items before calling the
     *       item specific connection testing.
     *
     * @param aPosition is a reference to a VECTOR2I object containing the test position.
     * @return True if connection to \a aPosition exists.
     */
    virtual bool doIsConnected( const VECTOR2I& aPosition ) const { return false; }

protected:
    SCH_LAYER_ID      m_layer;
    EDA_ITEMS         m_connections;      // List of items connected to this item.
    FIELDS_AUTOPLACED m_fieldsAutoplaced; // indicates status of field autoplacement
    VECTOR2I          m_storedPos;        // a temporary variable used in some move commands
                                          // to store a initial pos of the item or mouse cursor

    /// Store pointers to other items that are connected to this one, per sheet.
    std::map<SCH_SHEET_PATH, SCH_ITEM_VEC, SHEET_PATH_CMP> m_connected_items;

    /// Store connectivity information, per sheet.
    std::unordered_map<SCH_SHEET_PATH, SCH_CONNECTION*> m_connection_map;

    bool              m_connectivity_dirty;
};

#ifndef SWIG
DECLARE_ENUM_TO_WXANY( SCH_LAYER_ID );
#endif

#endif /* SCH_ITEM_H */
