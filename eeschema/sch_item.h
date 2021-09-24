/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2004-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <eda_item.h>
#include <plotters/plotter.h>      // for PLOT_DASH_TYPE definition

#include <default_values.h>
#include <sch_sheet_path.h>
#include <netclass.h>

class CONNECTION_GRAPH;
class SCH_CONNECTION;
class SCH_SHEET_PATH;
class SCHEMATIC;
class LINE_READER;
class SCH_EDIT_FRAME;
class wxFindReplaceData;
class PLOTTER;
class NETLIST_OBJECT;
class NETLIST_OBJECT_LIST;

using KIGFX::RENDER_SETTINGS;


enum FIELDS_AUTOPLACED
{
    FIELDS_AUTOPLACED_NO = 0,
    FIELDS_AUTOPLACED_AUTO,
    FIELDS_AUTOPLACED_MANUAL
};


enum DANGLING_END_T
{
    UNKNOWN = 0,
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
    DANGLING_END_ITEM( DANGLING_END_T aType, EDA_ITEM* aItem, const wxPoint& aPosition )
    {
        m_item = aItem;
        m_type = aType;
        m_pos = aPosition;
        m_parent = aItem;
    }

    DANGLING_END_ITEM( DANGLING_END_T aType, EDA_ITEM* aItem, const wxPoint& aPosition,
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

    wxPoint GetPosition() const { return m_pos; }
    EDA_ITEM* GetItem() const { return m_item; }
    const EDA_ITEM* GetParent() const { return m_parent; }
    DANGLING_END_T GetType() const { return m_type; }

private:
    EDA_ITEM*       m_item;         /// A pointer to the connectable object.
    wxPoint         m_pos;          /// The position of the connection point.
    DANGLING_END_T  m_type;         /// The type of connection of #m_item.
    const EDA_ITEM* m_parent;       /// A pointer to the parent object (in the case of pins)
};


typedef std::unordered_set<SCH_ITEM*> SCH_ITEM_SET;


/**
 * Simple container to manage line stroke parameters.
 */
class STROKE_PARAMS
{
public:
    STROKE_PARAMS( int aWidth = Mils2iu( DEFAULT_LINE_WIDTH_MILS ),
                   PLOT_DASH_TYPE aPlotStyle = PLOT_DASH_TYPE::DEFAULT,
                   const COLOR4D& aColor = COLOR4D::UNSPECIFIED ) :
            m_width( aWidth ),
            m_plotstyle( aPlotStyle ),
            m_color( aColor )
    {
    }

    int GetWidth() const { return m_width; }
    void SetWidth( int aWidth ) { m_width = aWidth; }

    PLOT_DASH_TYPE GetPlotStyle() const { return m_plotstyle; }
    void SetPlotStyle( PLOT_DASH_TYPE aPlotStyle ) { m_plotstyle = aPlotStyle; }

    COLOR4D GetColor() const { return m_color; }
    void SetColor( const COLOR4D& aColor ) { m_color = aColor; }

    bool operator!=( const STROKE_PARAMS& aOther )
    {
        return m_width != aOther.m_width
                || m_plotstyle != aOther.m_plotstyle
                || m_color != aOther.m_color;
    }

private:
    int            m_width;
    PLOT_DASH_TYPE m_plotstyle;
    COLOR4D        m_color;
};


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

    virtual ~SCH_ITEM();

    virtual wxString GetClass() const override
    {
        return wxT( "SCH_ITEM" );
    }

    /**
     * Swap the internal data structures \a aItem with the schematic item.
     * Obviously, aItem must have the same type than me.
     * @param aItem The item to swap the data structures with.
     */
    virtual void SwapData( SCH_ITEM* aItem );

    /**
     * Routine to create a new copy of given item.
     * The new object is not put in draw list (not linked).
     *
     * @param doClone (default = false) indicates unique values (such as timestamp and
     *                sheet name) should be duplicated.  Use only for undo/redo operations.
     */
    SCH_ITEM* Duplicate( bool doClone = false ) const;

    /**
     * @return true for items which are moved with the anchor point at mouse cursor
     *  and false for items moved with no reference to anchor
     * Usually return true for small items (labels, junctions) and false for
     * items which can be large (hierarchical sheets, symbols)
     */
    virtual bool IsMovableFromAnchorPoint() const { return true; }

    wxPoint& GetStoredPos() { return m_storedPos; }
    void     SetStoredPos( const wxPoint& aPos ) { m_storedPos = aPos; }

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

    virtual void DoHypertextMenu( EDA_DRAW_FRAME* aFrame ) { }

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

    /**
     * Print a schematic item.
     *
     * Each schematic item should have its own method
     *
     * @param aOffset is the drawing offset (usually {0,0} but can be different when moving an
     *                object).
     */
    virtual void Print( const RENDER_SETTINGS* aSettings, const wxPoint& aOffset ) = 0;

    /**
     * Move the item by \a aMoveVector to a new position.
     */
    virtual void Move( const wxPoint& aMoveVector ) = 0;

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
    virtual void Rotate( const wxPoint& aCenter ) = 0;

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
     * @param aItemList is the list of items to test item against.
     * @param aSheet is the sheet path to update connections for.
     * @return True if the dangling state has changed from it's current setting.
     */
    virtual bool UpdateDanglingState( std::vector<DANGLING_END_ITEM>& aItemList,
                                      const SCH_SHEET_PATH* aPath = nullptr )
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
    virtual bool IsPointClickableAnchor( const wxPoint& aPos ) const { return false; }

    /**
     * Add all the connection points for this item to \a aPoints.
     *
     * Not all schematic items have connection points so the default method does nothing.
     *
     * @param aPoints is the list of connection points to add to.
     */
    virtual std::vector<wxPoint> GetConnectionPoints() const { return {}; }

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
     * @param aPoint is a reference to a wxPoint object containing the coordinates to test.
     * @return True if connection to \a aPoint exists.
     */
    bool IsConnected( const wxPoint& aPoint ) const;

    /**
     * Retrieve the connection associated with this object in the given sheet.
     *
     * @note The returned value can be nullptr.
     */
    SCH_CONNECTION* Connection( const SCH_SHEET_PATH* aSheet = nullptr ) const;

    /**
     * Retrieve the set of items connected to this item on the given sheet.
     */
    SCH_ITEM_SET& ConnectedItems( const SCH_SHEET_PATH& aPath );

    /**
     * Add a connection link between this item and another.
     */
    void AddConnectionTo( const SCH_SHEET_PATH& aPath, SCH_ITEM* aItem );

    /**
     * Create a new connection object associated with this object.
     *
     * @param aPath is the sheet path to initialize.
     */
    SCH_CONNECTION* InitializeConnection( const SCH_SHEET_PATH& aPath, CONNECTION_GRAPH* aGraph );

    /**
     * Return true if this item should propagate connection info to \a aItem.
     */
    virtual bool ConnectionPropagatesTo( const EDA_ITEM* aItem ) const { return true; }

    bool IsConnectivityDirty() { return m_connectivity_dirty; }

    void SetConnectivityDirty( bool aDirty = true ) { m_connectivity_dirty = aDirty; }

    NETCLASSPTR NetClass( const SCH_SHEET_PATH* aSheet = nullptr ) const;

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
     */
    virtual void Plot( PLOTTER* aPlotter ) const;

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
     * @param aPosition is a reference to a wxPoint object containing the test position.
     * @return True if connection to \a aPosition exists.
     */
    virtual bool doIsConnected( const wxPoint& aPosition ) const { return false; }

protected:
    SCH_LAYER_ID      m_layer;
    EDA_ITEMS         m_connections;      // List of items connected to this item.
    FIELDS_AUTOPLACED m_fieldsAutoplaced; // indicates status of field autoplacement
    wxPoint           m_storedPos;        // a temporary variable used in some move commands
                                          // to store a initial pos of the item or mouse cursor

    /// Store pointers to other items that are connected to this one, per sheet.
    std::unordered_map<SCH_SHEET_PATH, SCH_ITEM_SET> m_connected_items;

    /// Store connectivity information, per sheet.
    std::unordered_map<SCH_SHEET_PATH, SCH_CONNECTION*> m_connection_map;

    bool              m_connectivity_dirty;
};

#endif /* SCH_ITEM_H */
