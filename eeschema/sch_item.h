/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#pragma once

#include <unordered_map>
#include <unordered_set>
#include <map>
#include <set>

#include <eda_item.h>
#include <sch_sheet_path.h>
#include <netclass.h>
#include <stroke_params.h>
#include <layer_ids.h>
#include <sch_render_settings.h>
#include <plotters/plotter.h>

class CONNECTION_GRAPH;
class SCH_CONNECTION;
class SCH_SHEET_PATH;
class SCHEMATIC;
class SCH_COMMIT;
class SYMBOL;
class LINE_READER;
class SCH_EDIT_FRAME;
class SCH_RULE_AREA;
struct SCH_PLOT_OPTS;

namespace KIFONT
{
class METRICS;
}


enum BODY_STYLE : int
{
    BASE = 1,
    DEMORGAN = 2
};


#define MINIMUM_SELECTION_DISTANCE 2 // Minimum selection distance in mils


enum AUTOPLACE_ALGO
{
    AUTOPLACE_NONE,      // No autoplacement
    AUTOPLACE_AUTO,      // A minimalist placement algorithm.
    AUTOPLACE_MANUAL     // A more involved routine that can be annoying if done from the get go.
                         //   Initiated by a hotkey or menu item.
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
    EDA_ITEM*       m_item;         ///< A pointer to the connectable object.
    VECTOR2I        m_pos;          ///< The position of the connection point.
    DANGLING_END_T  m_type;         ///< The type of connection of #m_item.
    const EDA_ITEM* m_parent;       ///< A pointer to the parent object (in the case of pins).
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
    SCH_ITEM( EDA_ITEM* aParent, KICAD_T aType, int aUnit = 0, int aBodyStyle = 0 );

    SCH_ITEM( const SCH_ITEM& aItem );

    SCH_ITEM& operator=( const SCH_ITEM& aPin );

    virtual ~SCH_ITEM();

    wxString GetClass() const override
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

    bool IsGroupableType() const;

    /**
     * Swap data between \a aItem and \a aImage.
     *
     * \a aItem and \a aImage should have the same type.
     *
     * Used in undo and redo commands to swap values between an item and its copy.
     * Only values like layer, size .. which are modified by editing are swapped.
     *
     * @param aImage the item image which contains data to swap.
     */
    void SwapItemData( SCH_ITEM* aImage );

    /**
     * Swap the non-temp and non-edit flags.
     */
    void SwapFlags( SCH_ITEM* aItem );

    /**
     * Routine to create a new copy of given item.
     *
     * The new object is not put in draw list (not linked).
     *
     * @param addToParentGroup Indicates whether or not the new item is added to the group
     *                         (if any) containing the old item.  If true, aCommit must be
     *                         provided.
     * @param aDoClone (default = false) indicates unique values (such as timestamp and
     *                 sheet name) should be duplicated.  Use only for undo/redo operations.
     */
    SCH_ITEM* Duplicate( bool addToParentGroup, SCH_COMMIT* aCommit = nullptr, bool doClone = false ) const;

    virtual void SetUnit( int aUnit ) { m_unit = aUnit; }
    int GetUnit() const { return m_unit; }

    virtual void SetUnitString( const wxString& aUnit );
    virtual wxString GetUnitString() const;

    virtual wxString GetUnitDisplayName( int aUnit, bool aLabel ) const;
    virtual wxString GetBodyStyleDescription( int aBodyStyle, bool aLabel ) const;

    virtual void SetBodyStyle( int aBodyStyle ) { m_bodyStyle = aBodyStyle; }
    int  GetBodyStyle() const { return m_bodyStyle; }

    virtual void SetBodyStyleProp( const wxString& aBodyStyle );
    virtual wxString GetBodyStyleProp() const;

    void SetPrivate( bool aPrivate ) { m_private = aPrivate; }
    bool IsPrivate() const { return m_private; }

    virtual void SetExcludedFromSim( bool aExclude, const SCH_SHEET_PATH* aInstance = nullptr,
                                     const wxString& aVariantName = wxEmptyString ) { }
    virtual bool GetExcludedFromSim( const SCH_SHEET_PATH* aInstance = nullptr,
                                     const wxString& aVariantName = wxEmptyString ) const { return false; }
    bool ResolveExcludedFromSim( const SCH_SHEET_PATH* aInstance = nullptr,
                                 const wxString& aVariantName = wxEmptyString ) const;

    virtual void SetExcludedFromBOM( bool aExcludeFromBOM, const SCH_SHEET_PATH* aInstance = nullptr,
                                     const wxString& aVariantName = wxEmptyString ) { }
    virtual bool GetExcludedFromBOM( const SCH_SHEET_PATH* aInstance = nullptr,
                                     const wxString& aVariantName = wxEmptyString ) const { return false; }
    bool ResolveExcludedFromBOM( const SCH_SHEET_PATH* aInstance = nullptr,
                                 const wxString& aVariantName = wxEmptyString ) const;

    virtual void SetExcludedFromBoard( bool aExclude, const SCH_SHEET_PATH* aInstance = nullptr,
                                        const wxString& aVariantName = wxEmptyString ) { }
    virtual bool GetExcludedFromBoard( const SCH_SHEET_PATH* aInstance = nullptr,
                                       const wxString& aVariantName = wxEmptyString ) const { return false; }
    bool ResolveExcludedFromBoard( const SCH_SHEET_PATH* aInstance = nullptr,
                                   const wxString& aVariantName = wxEmptyString ) const;

    virtual void SetExcludedFromPosFiles( bool aExclude, const SCH_SHEET_PATH* aInstance = nullptr,
                                          const wxString& aVariantName = wxEmptyString ) { }
    virtual bool GetExcludedFromPosFiles( const SCH_SHEET_PATH* aInstance = nullptr,
                                          const wxString& aVariantName = wxEmptyString ) const { return false; }
    bool ResolveExcludedFromPosFiles( const SCH_SHEET_PATH* aInstance = nullptr,
                                      const wxString& aVariantName = wxEmptyString ) const;

    virtual void SetDNP( bool aDNP, const SCH_SHEET_PATH* aInstance = nullptr,
                         const wxString& aVariantName = wxEmptyString ) { }
    virtual bool GetDNP( const SCH_SHEET_PATH* aInstance = nullptr,
                         const wxString& aVariantName = wxEmptyString ) const { return false; }
    bool ResolveDNP( const SCH_SHEET_PATH* aInstance = nullptr,
                     const wxString& aVariantName = wxEmptyString ) const;

    wxString ResolveText( const wxString& aText, const SCH_SHEET_PATH* aPath, int aDepth = 0 ) const;

    /**
     * Check if object is movable from the anchor point.
     *
     * Usually return true for small items (labels, junctions) and false for
     * items which can be large (hierarchical sheets, symbols).
     *
     * @retval true for items which are moved with the anchor point at mouse cursor.
     * @retval false for items moved with no reference to anchor.
     */
    virtual bool IsMovableFromAnchorPoint() const { return true; }

    VECTOR2I& GetStoredPos() { return m_storedPos; }
    void      SetStoredPos( const VECTOR2I& aPos ) { m_storedPos = aPos; }

    /**
     * Search the item hierarchy to find a #SCHEMATIC.
     *
     * Every #SCH_ITEM that lives on a #SCH_SCREEN should be parented to either that screen
     * or another #SCH_ITEM on the same screen (for example, pins to their symbols).
     *
     * Every #SCH_SCREEN should be parented to the #SCHEMATIC.
     *
     * @note This hierarchy is not the same as the sheet hierarchy!
     *
     * @return the parent schematic this item lives on, or nullptr.
     */
    SCHEMATIC* Schematic() const;

    const SYMBOL* GetParentSymbol() const;
    SYMBOL* GetParentSymbol();

    /**
     * Indicates that the item has at least one hypertext action.  This could be a URL assigned to
     * the item as a whole, or one (or more) urls within the text of the item.
     */
    virtual bool HasHypertext() const { return false; }

    /**
     * Indicates that a hypertext link is currently active.
     * (Note that the default implementation here only handles the simple case.)
     */
    virtual bool HasHoveredHypertext() const { return HasHypertext() && IsRollover(); }

    virtual void DoHypertextAction( EDA_DRAW_FRAME* aFrame, const VECTOR2I& aMousePos ) const { }

    /**
     * Return the layer this item is on.
     */
    SCH_LAYER_ID GetLayer() const { return m_layer; }
    void SetLayer( SCH_LAYER_ID aLayer ) { m_layer = aLayer; }

    /**
     * Return the layers the item is drawn on (which may be more than its "home" layer)
     */
    std::vector<int> ViewGetLayers() const override;

    int GetMaxError() const;

    /**
     * @return the size of the "pen" that be used to draw or plot this item
     */
    virtual int GetPenWidth() const { return 0; }

    int GetEffectivePenWidth( const SCH_RENDER_SETTINGS* aSettings ) const;

    const wxString& GetDefaultFont( const RENDER_SETTINGS* aSettings ) const;

    const KIFONT::METRICS& GetFontMetrics() const;

    bool RenderAsBitmap( double aWorldScale ) const override;

    /**
     * Return a measure of how likely the other object is to represent the same
     * object.
     *
     * The scale runs from 0.0 (definitely different objects) to 1.0 (same)
     */
    virtual double Similarity( const SCH_ITEM& aItem ) const
    {
        wxCHECK_MSG( false, 0.0, wxT( "Similarity not implemented in " ) + GetClass() );
    }

    /**
     * Calculate the boilerplate similarity for all LIB_ITEMs without
     * preventing the use above of a pure virtual function that catches at compile
     * time when a new object has not been fully implemented
    */
    double SimilarityBase( const SCH_ITEM& aItem ) const
    {
        double similarity = 1.0;

        if( m_unit != aItem.m_unit )
            similarity *= 0.9;

        if( m_bodyStyle != aItem.m_bodyStyle )
            similarity *= 0.9;

        if( m_private != aItem.m_private )
            similarity *= 0.9;

        return similarity;
    }

    /**
     * Move the item by \a aMoveVector to a new position.
     */
    virtual void Move( const VECTOR2I& aMoveVector )
    {
        wxCHECK_MSG( false, /*void*/, wxT( "Move not implemented in " ) + GetClass() );
    }

    /**
     * Mirror item horizontally about \a aCenter.
     */
    virtual void MirrorHorizontally( int aCenter )
    {
        wxCHECK_MSG( false, /*void*/, wxT( "MirrorHorizontally not implemented in " ) + GetClass() );
    }

    /**
     * Mirror item vertically about \a aCenter.
     */
    virtual void MirrorVertically( int aCenter )
    {
        wxCHECK_MSG( false, /*void*/, wxT( "MirrorVertically not implemented in " ) + GetClass() );
    }

    /**
     * Rotate the item around \a aCenter 90 degrees in the clockwise direction.
     */
    virtual void Rotate( const VECTOR2I& aCenter, bool aRotateCCW )
    {
        wxCHECK_MSG( false, /*void*/, wxT( "Rotate not implemented in " ) + GetClass() );
    }

    /**
     * Begin drawing a symbol library draw item at \a aPosition.
     *
     * It typically would be called on a left click when a draw tool is selected in
     * the symbol library editor and one of the graphics tools is selected.
     *
     * @param aPosition The position in drawing coordinates where the drawing was started.
     *                  May or may not be required depending on the item being drawn.
     */
    virtual void BeginEdit( const VECTOR2I& aPosition ) {}

    /**
     * Continue an edit in progress at \a aPosition.
     *
     * This is used to perform the next action while drawing an item.  This would be
     * called for each additional left click when the mouse is captured while the item
     * is being drawn.
     *
     * @param aPosition The position of the mouse left click in drawing coordinates.
     * @return True if additional mouse clicks are required to complete the edit in progress.
     */
    virtual bool ContinueEdit( const VECTOR2I& aPosition ) { return false; }

    /**
     * End an object editing action.
     *
     * This is used to end or abort an edit action in progress initiated by BeginEdit().
     */
    virtual void EndEdit( bool aClosed = false ) {}

    /**
     * Calculate the attributes of an item at \a aPosition when it is being edited.
     *
     * This method gets called by the Draw() method when the item is being edited.  This
     * probably should be a pure virtual method but bezier curves are not yet editable in
     * the symbol library editor.  Therefore, the default method does nothing.
     *
     * @param aPosition The current mouse position in drawing coordinates.
     */
    virtual void CalcEdit( const VECTOR2I& aPosition ) {}

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
                                      const SCH_SHEET_PATH*           aSheet = nullptr )
    {
        return false;
    }

    /**
     * Test if \a aPt is an end point of this schematic object.
     *
     * @note The end point test doe **not** imply electrical connectivity.  See IsConnectable().
     *       In other words all connection points are end points but not all end points are
     *       connection points such as graphical lines and arcs.  Override this method for all
     *       objects that have end points.
     *
     * @param aPt is the coordinate to test for an end point.
     * @retval true if \a aPt is an end point.
     * @retval false if \a aPt is not an end point.
     */
    virtual bool IsEndPoint( const VECTOR2I& aPt ) const { return false; }

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
     * This provides granular per object connectivity change testing to prevent the need
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

    /// Update the connection graph for all connections in this item.
    void SetConnectionGraph( CONNECTION_GRAPH* aGraph );

    virtual bool HasCachedDriverName() const { return false; }
    virtual const wxString& GetCachedDriverName() const;

    virtual void SetLastResolvedState( const SCH_ITEM* aItem ) { }

    std::shared_ptr<NETCLASS> GetEffectiveNetClass( const SCH_SHEET_PATH* aSheet = nullptr ) const;

    /**
     * Return whether the fields have been automatically placed.
     */
    AUTOPLACE_ALGO GetFieldsAutoplaced() const { return m_fieldsAutoplaced; }
    void SetFieldsAutoplaced( AUTOPLACE_ALGO aAlgo ) { m_fieldsAutoplaced = aAlgo; }

    virtual void AutoplaceFields( SCH_SCREEN* aScreen, AUTOPLACE_ALGO aAlgo ) { }

    virtual void RunOnChildren( const std::function<void( SCH_ITEM* )>& aFunction, RECURSE_MODE aMode ) { }

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

    void GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList ) override;

    /**
     * Plot the item to \a aPlotter.
     *
     * @param aBackground a poor-man's Z-order.  The routine will get called twice, first with
     *                    aBackground true and then with aBackground false.
     * @param aUnit - which unit to print.
     * @param aBodyStyle - which body style to print.
     * @param aOffset relative offset.
     * @param aDimmed reduce brightness of item.
     */
    virtual void Plot( PLOTTER* aPlotter, bool aBackground, const SCH_PLOT_OPTS& aPlotOpts,
                       int aUnit, int aBodyStyle, const VECTOR2I& aOffset, bool aDimmed)
    {
        wxCHECK_MSG( false, /*void*/, wxT( "Plot not implemented in " ) + GetClass() );
    }

    /**
     * Reset the cache of rule areas (called prior to schematic connectivity computation)
     */
    void ClearRuleAreasCache() { m_rule_areas_cache.clear(); }

    /**
     * Add a rule area to the item's cache.
     */
    void AddRuleAreaToCache( SCH_RULE_AREA* aRuleArea ) { m_rule_areas_cache.insert( aRuleArea ); }

    /**
     * Get the cache of rule areas enclosing this item.
     */
    const std::unordered_set<SCH_RULE_AREA*>& GetRuleAreaCache() const
    {
        return m_rule_areas_cache;
    }

    const std::vector<wxString>* GetEmbeddedFonts() override;

    /**
     * The list of flags used by the #compare function.
     *
     * UNIT  This flag relaxes unit, body-style and pin-number constraints.  It is used for
     *       #SCH_ITEM object unit comparisons.
     *
     * EQUALITY  This flag relaxes ordering constraints so that fields, etc. don't have to
     *           appear in the same order to be considered equal.
     *
     * ERC  This flag relaxes constraints on data that is settable in the schematic editor.
     *      It compares only symbol-editor-only data.
     *
     * SKIP_TST_POS   This flag relaxes comparisons on position (mainly for fields) for ERC.
     */
    enum COMPARE_FLAGS : int
    {
        UNIT          = 0x01,
        EQUALITY      = 0x02,
        ERC           = 0x04,
        SKIP_TST_POS  = 0x08
    };

    virtual bool operator==( const SCH_ITEM& aOther ) const;

    virtual bool operator<( const SCH_ITEM& aItem ) const;

protected:
    /**
     * Swap the internal data structures \a aItem with the schematic item.
     *
     * Obviously, aItem must have the same type than me.
     *
     * @param aItem The item to swap the data structures with.
     */
    virtual void swapData( SCH_ITEM* aItem );

    SCH_RENDER_SETTINGS* getRenderSettings( PLOTTER* aPlotter ) const
    {
        return static_cast<SCH_RENDER_SETTINGS*>( aPlotter->RenderSettings() );
    }

    struct cmp_items
    {
        bool operator()( const SCH_ITEM* aFirst, const SCH_ITEM* aSecond ) const;
    };

    /**
     * Provide the draw object specific comparison called by the == and < operators.
     *
     * The base object sort order which always proceeds the derived object sort order
     * is as follows:
     *      - Symbol alternate part (DeMorgan) number.
     *      - Symbol part number.
     *      - KICAD_T enum value.
     *      - Result of derived classes comparison.
     *
     * @note Make sure you call down to #SCH_ITEM::compare before doing any derived object
     *       comparisons or you will break the sorting using the symbol library file format.
     *
     * @param aOther A reference to the other #SCH_ITEM to compare the arc against.
     * @param aCompareFlags The flags used to perform the comparison.
     *
     * @return An integer value less than 0 if the object is less than \a aOther object,
     *         zero if the object is equal to \a aOther object, or greater than 0 if the
     *         object is greater than \a aOther object.
     */
    virtual int compare( const SCH_ITEM& aOther, int aCompareFlags = 0 ) const;

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
    int               m_unit;               // set to 0 if common to all units
    int               m_bodyStyle;          // set to 0 if common to all body styles
    bool              m_private;            // only shown in Symbol Editor
    AUTOPLACE_ALGO    m_fieldsAutoplaced;   // indicates status of field autoplacement
    VECTOR2I          m_storedPos;          // temp variable used in some move commands to store
                                            // an initial position of the item or mouse cursor

    /// Store pointers to other items that are connected to this one, per sheet.
    std::map<SCH_SHEET_PATH, SCH_ITEM_VEC, SHEET_PATH_CMP> m_connected_items;

    /// Store connectivity information, per sheet.
    std::unordered_map<SCH_SHEET_PATH, SCH_CONNECTION*>    m_connection_map;

    bool                                                   m_connectivity_dirty;

    /// Store pointers to rule areas which this item is contained within
    std::unordered_set<SCH_RULE_AREA*>                     m_rule_areas_cache;

private:
    friend class LIB_SYMBOL;
};

#ifndef SWIG
DECLARE_ENUM_TO_WXANY( SCH_LAYER_ID );
#endif
