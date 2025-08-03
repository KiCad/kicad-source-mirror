/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * @author Jon Evans <jon@craftyjon.com>
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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _CONNECTION_GRAPH_H
#define _CONNECTION_GRAPH_H

#include <mutex>
#include <utility>
#include <vector>

#include <erc/erc_settings.h>
#include <sch_connection.h>
#include <sch_item.h>
#include <wx/treectrl.h>
#include <advanced_config.h>
#include <progress_reporter.h>


#ifdef DEBUG
// Uncomment this line to enable connectivity debugging features
// #define CONNECTIVITY_DEBUG
#endif


class CONNECTION_GRAPH;
class SCHEMATIC;
class SCH_EDIT_FRAME;
class SCH_HIERLABEL;
class SCH_PIN;
class SCH_SHEET_PIN;


/**
 * A subgraph is a set of items that are electrically connected on a single sheet.
 *
 * For example, a label connected to a wire and so on.
 * A net is composed of one or more subgraphs.
 *
 * A set of items that appears to be physically connected may actually be more
 * than one subgraph, because some items don't connect electrically.
 *
 * For example, multiple bus wires can come together at a junction but have
 * different labels on each branch.  Each label+wire branch is its own subgraph.
 */
class CONNECTION_SUBGRAPH
{
public:
    enum class PRIORITY
    {
        INVALID = -1,
        NONE    = 0,
        PIN,
        SHEET_PIN,
        HIER_LABEL,
        LOCAL_LABEL,
        LOCAL_POWER_PIN,
        GLOBAL_POWER_PIN,
        GLOBAL
    };

    explicit CONNECTION_SUBGRAPH( CONNECTION_GRAPH* aGraph ) :
              m_graph( aGraph ),
              m_dirty( false ),
              m_absorbed( false ),
              m_is_bus_member( false ),
              m_absorbed_by( nullptr ),
              m_code( -1 ),
              m_multiple_drivers( false ),
              m_strong_driver( false ),
              m_local_driver( false ),
              m_bus_entry( nullptr ),
              m_hier_parent( nullptr ),
              m_driver( nullptr ),
              m_no_connect( nullptr ),
              m_driver_connection( nullptr )
    {}

    ~CONNECTION_SUBGRAPH()
    {
        for( SCH_CONNECTION* connection : m_bus_element_connections )
            delete connection;
    }

    friend class CONNECTION_GRAPH;

    /**
     * Determine which potential driver should drive the subgraph.
     *
     * If multiple possible drivers exist, picks one according to the priority.
     * If multiple "winners" exist, returns false and sets #m_driver to nullptr.
     *
     * @param aCheckMultipleDrivers controls whether the second driver should be captured for ERC.
     * @return true if m_driver was set, or false if a conflict occurred.
     */
    bool ResolveDrivers( bool aCheckMultipleDrivers = false );

    /**
     * Return the fully-qualified net name for this subgraph (if one exists)
     */
    wxString GetNetName() const;

    /// Return all the vector-based bus labels attached to this subgraph (if any).
    std::vector<SCH_ITEM*> GetVectorBusLabels() const;

    /// Return all the all bus labels attached to this subgraph (if any).
    std::vector<SCH_ITEM*> GetAllBusLabels() const;

    /// Return the candidate net name for a driver.
    const wxString& GetNameForDriver( SCH_ITEM* aItem ) const;

    /// Return the resolved netclasses for the item, and the source item providing the netclass
    /// @param aItem the item to query for netclass assignments
    const std::vector<std::pair<wxString, SCH_ITEM*>>
    GetNetclassesForDriver( SCH_ITEM* aItem ) const;

    /// Combine another subgraph on the same sheet into this one.
    void Absorb( CONNECTION_SUBGRAPH* aOther );

    /// Add a new item to the subgraph.
    void AddItem( SCH_ITEM* aItem );

    /// Update all items to match the driver connection.
    void UpdateItemConnections();

    /// Provide a read-only reference to the items in the subgraph.
    const std::set<SCH_ITEM*>& GetItems() const
    {
        return m_items;
    }

    /// Find all items in the subgraph as well as child subgraphs recursively.
    void getAllConnectedItems( std::set<std::pair<SCH_SHEET_PATH, SCH_ITEM*>>& aItems,
                               std::set<CONNECTION_SUBGRAPH*>& aSubgraphs );

    /**
     * Return the priority (higher is more important) of a candidate driver
     *
     * 0: Invalid driver
     * 1: Symbol pin
     * 2: Hierarchical sheet pin
     * 3: Hierarchical label
     * 4: Local label
     * 5: Power pin
     * 6: Global label
     *
     * @param aDriver is the item to inspect
     * @return a PRIORITY
     */
    static PRIORITY GetDriverPriority( SCH_ITEM* aDriver );

    PRIORITY GetDriverPriority()
    {
        if( m_driver )
            return GetDriverPriority( m_driver );
        else
            return PRIORITY::NONE;
    }

    /**
     * @return pointer to the SCH_ITEM whose name sets the subgraph netname.
     *         N.B. This item may not be in the subgraph.
     */
    const SCH_ITEM* GetDriver() const
    {
        return m_driver;
    }

    /**
     * @return #SCH_CONNECTION object for m_driver on #m_sheet.
     */
    const SCH_CONNECTION* GetDriverConnection() const
    {
        return m_driver_connection;
    }

    /**
     * @return pointer to the item causing a no-connect or nullptr if none.
     */
    const SCH_ITEM* GetNoConnect() const
    {
        return m_no_connect;
    }

    const SCH_SHEET_PATH& GetSheet() const
    {
        return m_sheet;
    }

    const std::unordered_map< std::shared_ptr<SCH_CONNECTION>,
                        std::unordered_set<CONNECTION_SUBGRAPH*> >& GetBusParents() const
    {
        return m_bus_parents;
    }

    void RemoveItem( SCH_ITEM* aItem );

    /**
     * Replaces all references to #aOldItem with #aNewItem in the subgraph.
    */
    void ExchangeItem( SCH_ITEM* aOldItem, SCH_ITEM* aNewItem );

    // Use this to keep a connection pointer that is not owned by any item
    // This will be destroyed with the subgraph
    void StoreImplicitConnection( SCH_CONNECTION* aConnection )
    {
        m_bus_element_connections.insert( aConnection );
    }

private:
    wxString driverName( SCH_ITEM* aItem ) const;

    CONNECTION_GRAPH* m_graph;

    bool m_dirty;

    /// True if this subgraph has been absorbed into another.  No pointers here are safe if so!
    bool m_absorbed;

    /**
     *  True if the subgraph is not actually part of a net.  These are created for bus members
     *  to ensure that bus-to-bus connection happens but they don't have any valid data
     */
    bool m_is_bus_member;

    /// If this subgraph is absorbed, points to the absorbing (and valid) subgraph
    CONNECTION_SUBGRAPH* m_absorbed_by;

    /// Set of subgraphs that have been absorbed by this subgraph
    std::set<CONNECTION_SUBGRAPH*> m_absorbed_subgraphs;

    long m_code;

    /**
     * True if this subgraph contains more than one driver that should be
     * shorted together in the netlist.  For example, two labels or
     * two power ports.
     */
    bool m_multiple_drivers;

    /// True if the driver is "strong": a label or power object.
    bool m_strong_driver;

    /// True if the driver is a local (i.e. non-global) type.
    bool m_local_driver;

    /// Bus entry in graph, if any.
    SCH_ITEM* m_bus_entry;

    std::set<SCH_ITEM*> m_drivers;

    /**
     * If a subgraph is a bus, this map contains links between the bus members and any
     * local sheet neighbors with the same connection name.
     *
     * For example, if this subgraph is a bus D[7..0], and on the same sheet there is
     * a net with label D7, this map will contain an entry for the D7 bus member, and
     * the set will contain a pointer to the D7 net subgraph.
     */
    std::unordered_map< std::shared_ptr<SCH_CONNECTION>,
                        std::unordered_set<CONNECTION_SUBGRAPH*> > m_bus_neighbors;

    /**
     * If this is a net, this vector contains links to any same-sheet buses that contain it.
     * The string key is the name of the connection that forms the link (which isn't necessarily
     * the same as the name of the connection driving this subgraph)
     */
    std::unordered_map< std::shared_ptr<SCH_CONNECTION>,
                        std::unordered_set<CONNECTION_SUBGRAPH*> > m_bus_parents;

    /// Cache for lookup of any hierarchical (sheet) pins on this subgraph (for referring down).
    std::set<SCH_SHEET_PIN*> m_hier_pins;

    /// Cache for lookup of any hierarchical ports on this subgraph (for referring up).
    std::set<SCH_HIERLABEL*> m_hier_ports;

    /// If not null, this indicates the subgraph on a higher level sheet that is linked to this one.
    CONNECTION_SUBGRAPH* m_hier_parent;

    /// If not null, this indicates the subgraph(s) on a lower level sheet that are linked to
    /// this one.
    std::unordered_set<CONNECTION_SUBGRAPH*> m_hier_children;

    /// A cache of escaped netnames from schematic items.
    mutable std::mutex m_driver_name_cache_mutex;
    mutable std::unordered_map<SCH_ITEM*, wxString> m_driver_name_cache;

    /// Fully-resolved driver for the subgraph (might not exist in this subgraph).
    SCH_ITEM* m_driver;

    /// Contents of the subgraph.
    std::set<SCH_ITEM*> m_items;

    /// No-connect item in graph, if any.
    SCH_ITEM* m_no_connect;

    /// On which logical sheet is the subgraph contained.
    SCH_SHEET_PATH m_sheet;

    /// Cache for driver connection.
    SCH_CONNECTION* m_driver_connection;

    /// A cache of connections that are part of this subgraph but that don't have
    /// an owning element (i.e. bus members)
    std::set<SCH_CONNECTION*> m_bus_element_connections;

    std::mutex m_driver_mutex;
};

struct NET_NAME_CODE_CACHE_KEY
{
    wxString  Name;
    int       Netcode;

    bool operator==(const NET_NAME_CODE_CACHE_KEY& other) const
    {
        return Name == other.Name && Netcode == other.Netcode;
    }
};

namespace std
{
    template <>
    struct hash<NET_NAME_CODE_CACHE_KEY>
    {
        std::size_t operator()( const NET_NAME_CODE_CACHE_KEY& k ) const
        {
            const std::size_t prime = 19937;

            return hash<wxString>()( k.Name ) ^ ( hash<int>()( k.Netcode ) * prime );
        }
    };
}

/// Associate a #NET_CODE_NAME with all the subgraphs in that net.
typedef std::unordered_map<NET_NAME_CODE_CACHE_KEY, std::vector<CONNECTION_SUBGRAPH*>> NET_MAP;

/**
 * Calculate the connectivity of a schematic and generates netlists.
 */
class CONNECTION_GRAPH
{
public:
    CONNECTION_GRAPH( SCHEMATIC* aSchematic = nullptr ) :
              m_last_net_code( 1 ),
              m_last_bus_code( 1 ),
              m_last_subgraph_code( 1 ),
              m_schematic( aSchematic )
    {}

    ~CONNECTION_GRAPH()
    {
        Reset();
    }

    // We own at least one list of raw pointers.  Don't let the compiler fill in copy c'tors that
    // will only land us in trouble.
    CONNECTION_GRAPH( const CONNECTION_GRAPH& ) = delete;
    CONNECTION_GRAPH& operator=( const CONNECTION_GRAPH& ) = delete;

    // Define QA friend functions to allow testing of private methods
    friend void boost_test_update_symbol_connectivity();
    friend void boost_test_update_generic_connectivity();

    void Reset();

    void SetSchematic( SCHEMATIC* aSchematic )
    {
        m_schematic = aSchematic;
    }

    void SetLastCodes( const CONNECTION_GRAPH* aOther )
    {
        m_last_net_code = aOther->m_last_net_code;
        m_last_bus_code = aOther->m_last_bus_code;
        m_last_subgraph_code = aOther->m_last_subgraph_code;
    }

    /**
     * Update the connection graph for the given list of sheets.
     *
     * @param aSheetList is the list of possibly modified sheets
     * @param aUnconditional is true if an unconditional full recalculation should be done
     * @param aChangedItemHandler an optional handler to receive any changed items
     */
    void Recalculate( const SCH_SHEET_LIST& aSheetList, bool aUnconditional = false,
                      std::function<void( SCH_ITEM* )>* aChangedItemHandler = nullptr,
                      PROGRESS_REPORTER* aProgressReporter = nullptr );

    /**
     * Return a bus alias pointer for the given name if it exists (from cache)
     *
     * CONNECTION_GRAPH caches these, they are owned by the SCH_SCREEN that
     * the alias was defined on.  The cache is only used to update the graph.
     */
    std::shared_ptr<BUS_ALIAS> GetBusAlias( const wxString& aName );

    /**
     * Determine which subgraphs have more than one conflicting bus label.
     *
     * @see DIALOG_MIGRATE_BUSES
     * @return a list of subgraphs that need migration
     */

    std::vector<const CONNECTION_SUBGRAPH*> GetBusesNeedingMigration();

    /**
     * Run electrical rule checks on the connectivity graph.
     *
     * Precondition: graph is up-to-date
     *
     * @return the number of errors found
     */
    int RunERC();

    const NET_MAP& GetNetMap() const { return m_net_code_to_subgraphs_map; }

    /**
     * Return the subgraph for a given net name on a given sheet.
     *
     * @param aNetName is the local net name to look for.
     * @param aPath is a sheet path to look on.
     * @return the subgraph matching the query, or nullptr if none is found.
     */
    CONNECTION_SUBGRAPH* FindSubgraphByName( const wxString& aNetName,
                                             const SCH_SHEET_PATH& aPath );

    /**
     * Retrieve a subgraph for the given net name, if one exists.
     *
     * Search every sheet.
     *
     * @param aNetName is the full net name to search for.
     * @return the subgraph matching the query, or nullptr if none is found.
     */
    CONNECTION_SUBGRAPH* FindFirstSubgraphByName( const wxString& aNetName );

    CONNECTION_SUBGRAPH* GetSubgraphForItem( SCH_ITEM* aItem ) const;

    const std::vector<CONNECTION_SUBGRAPH*>& GetAllSubgraphs( const wxString& aNetName ) const;

    /**
     * Return the fully-resolved netname for a given subgraph.
     *
     * @param aSubGraph Reference to the subgraph.
     * @return Netname string usable with m_net_name_to_subgraphs_map.
     */
    wxString GetResolvedSubgraphName( const CONNECTION_SUBGRAPH* aSubGraph ) const;

    /**
     * For a set of items, this will remove the connected items and their
     * associated data including subgraphs and generated codes from the connection graph.
     *
     * @param aItems A vector of items whose presence should be removed from the graph.
     * @return The full set of all items associated with the input items that were removed.
     */
    std::set<std::pair<SCH_SHEET_PATH, SCH_ITEM*>> ExtractAffectedItems(
            const std::set<SCH_ITEM*> &aItems );

    /**
     * Combine the input graph contents into the current graph.
     *
     * @warning After merging, the original graph is invalid.
     *
     * @param aGraph Input graph reference to add to the current graph.
     */
    void Merge( CONNECTION_GRAPH& aGraph );

    void RemoveItem( SCH_ITEM* aItem );

    /**
     * Replace all references to #aOldItem with #aNewItem in the graph.
    */
    void ExchangeItem( SCH_ITEM* aOldItem, SCH_ITEM* aNewItem );

    /**
     * We modify how we handle the connectivity graph for small graphs vs large
     * graphs.  Partially this is to avoid unneeded complexity for small graphs,
     * where the performance of the graph is not a concern.  This is considered
     * a temporary solution until the connectivity graph is refactored with an
     * eye toward partial updates
    */
    bool IsMinor() const
    {
        return static_cast<ssize_t>( m_items.size() )
               < ADVANCED_CFG::GetCfg().m_MinorSchematicGraphSize;
    }

private:

    /**
     * Update the connectivity of a symbol and its pins.
     * This is called by updateItemConnectivity() for each symbol
     * in the schematic.
     */
    void updateSymbolConnectivity( const SCH_SHEET_PATH& aSheet,
                                   SCH_SYMBOL* aSymbol,
                                   std::map<VECTOR2I, std::vector<SCH_ITEM*>>& aConnectionMap );

    /**
     * Update the connectivity of a pin and its connections.
     * This is called by updateItemConnectivity() for each pin
     * in the schematic.
     */
    void updatePinConnectivity( const SCH_SHEET_PATH& aSheet,
                                SCH_PIN* aPin,
                                SCH_CONNECTION* aConnection );

    /**
     * Update the connectivity of items that are not pins or symbols.
     * This is called by updateItemConnectivity() for each item
     * in the schematic that is not a symbol or pin.
     */
    void updateGenericItemConnectivity( const SCH_SHEET_PATH& aSheet,
                                        SCH_ITEM* aItem,
                                        std::map<VECTOR2I, std::vector<SCH_ITEM*>>& aConnectionMap );

    /**
     * Update the graphical connectivity between items (i.e. where they touch)
     * The items passed in must be on the same sheet.
     *
     * In the first phase, all items in aItemList have their connections
     * initialized for the given sheet (since they may have connections on more
     * than one sheet, and each needs to be calculated individually).  The
     * graphical connection points for the item are added to a map that stores
     * (x, y) -> [list of items].
     *
     * Any item that is stored in the list of items that have a connection point
     * at a given (x, y) location will eventually be electrically connected.
     * This means that we can't store SCH_SYMBOLs in this map -- we must store
     * a structure that links a specific pin on a symbol back to that symbol: a
     * SCH_PIN_CONNECTION.  This wrapper class is a convenience for linking a pin
     * and symbol to a specific (x, y) point.
     *
     * In the second phase, we iterate over each value in the map, which is a
     * vector of items that have overlapping connection points.  After some
     * checks to ensure that the items should actually connect, the items are
     * linked together using ConnectedItems().
     *
     * As a side effect, items are loaded into m_items for BuildConnectionGraph().
     *
     * @param aSheet is the path to the sheet of all items in the list.
     * @param aItemList is a list of items to consider.
     */
    void updateItemConnectivity( const SCH_SHEET_PATH& aSheet,
                                 const std::vector<SCH_ITEM*>& aItemList );

    /**
     * Generate the connection graph (after all item connectivity has been updated).
     *
     * In the first phase, the algorithm iterates over all items, and then over
     * all items that are connected (graphically) to each item, placing them into
     * CONNECTION_SUBGRAPHs.  Items that can potentially drive connectivity (i.e.
     * labels, pins, etc.) are added to the m_drivers vector of the subgraph.
     *
     * In the second phase, each subgraph is resolved.  To resolve a subgraph,
     * the driver is first selected by CONNECTION_SUBGRAPH::ResolveDrivers(),
     * and then the connection for the chosen driver is propagated to all the
     * other items in the subgraph.
     *
     * If the unconitional flag is set, all existing net classes will be removed
     * and re-created.  Otherwise, we will preserve existing net classes that do not
     * conflict with the new net classes.
     */
    void buildConnectionGraph( std::function<void( SCH_ITEM* )>* aChangedItemHandler,
                               bool aUnconditional );

    /**
     * Generate individual item subgraphs on a per-sheet basis.
     */
    void buildItemSubGraphs();

    /**
     * Find all subgraphs in the connection graph and calls ResolveDrivers() in parallel.
     */
    void resolveAllDrivers();

    /**
     * Map the driver values for each subgraph.
     */
    void collectAllDriverValues();

    /**
     * Iterate through the global power pins to collect the global labels as drivers.
     */
    void generateGlobalPowerPinSubGraphs();

    /**
     * Iterate through labels to create placeholders for bus elements.
     */
    void generateBusAliasMembers();

    /**
     * Process all subgraphs to assign netcodes and merge subgraphs based on labels.
     */
    void processSubGraphs();

    /**
     * Helper to assign a new net code to a connection.
     *
     * @return the assigned code
     */
    int assignNewNetCode( SCH_CONNECTION& aConnection );

    /**
     *
     * @param aNetName string with the netname for coding
     * @return existing netcode (if it exists) or newly created one
     */
    int getOrCreateNetCode( const wxString& aNetName );

    /**
     * Ensure all members of the bus connection have a valid net code assigned.
     *
     * @param aConnection is a bus connection.
     */
    void assignNetCodesToBus( SCH_CONNECTION* aConnection );

    /**
     * Update all neighbors of a subgraph with this one's connectivity info.
     *
     * If this subgraph contains hierarchical links, this method will descent the
     * hierarchy and propagate the connectivity across all linked sheets.
     *
     * @param aSubgraph is the subgraph being processed.
     * @param aForce prevents this routine from skipping subgraphs.
     */
    void propagateToNeighbors( CONNECTION_SUBGRAPH* aSubgraph, bool aForce );

    /**
     * Remove references to the given subgraphs from all structures in the connection graph.
     *
     * @param aSubgraphs set of unique subgraphs to find/remove.
     */
    void removeSubgraphs( std::set<CONNECTION_SUBGRAPH*>& aSubgraphs );

    /**
     * Search for a matching bus member inside a bus connection.
     *
     * For bus groups, this returns a bus member that matches aSearch by name.
     * For bus vectors, this returns a bus member that matches by vector index.
     *
     * @param aBusConnection is the bus connection to search.
     * @param aSearch is the net connection to search for.
     * @returns a member of aBusConnection that matches aSearch.
     */
    static SCH_CONNECTION* matchBusMember( SCH_CONNECTION* aBusConnection,
                                           SCH_CONNECTION* aSearch );

    /**
     * Build a new default connection for the given item based on its properties.
     *
     * Handles strong drivers (power pins and labels) only.
     *
     * @param aItem is an item that can generate a connection name.
     * @param aSubgraph is used to determine the sheet to use and retrieve the cached name.
     * @return a connection generated from the item, or nullptr if item is not valid.
     */
    std::shared_ptr<SCH_CONNECTION> getDefaultConnection( SCH_ITEM* aItem,
                                                          CONNECTION_SUBGRAPH* aSubgraph );

    void recacheSubgraphName( CONNECTION_SUBGRAPH* aSubgraph, const wxString& aOldName );

    /**
     * If the subgraph has multiple drivers of equal priority that are graphically connected,
     * ResolveDrivers() will have stored the second driver for use by this function, which actually
     * creates the markers.
     *
     * @param aSubgraph is the subgraph to examine
     * @return  true for no errors, false for errors
     */
    bool ercCheckMultipleDrivers( const CONNECTION_SUBGRAPH* aSubgraph );

    /**
     * Check one subgraph for conflicting connections between net and bus labels.
     *
     * For example, a net wire connected to a bus port/pin, or vice versa
     *
     * @param  aSubgraph      is the subgraph to examine.
     * @return                true for no errors, false for errors.
     */
    bool ercCheckBusToNetConflicts( const CONNECTION_SUBGRAPH* aSubgraph );

    /**
     * Check one subgraph for conflicting connections between two bus items.
     *
     * For example, a labeled bus wire connected to a hierarchical sheet pin
     * where the labeled bus doesn't contain any of the same bus members as the
     * sheet pin.
     *
     * @param  aSubgraph      is the subgraph to examine.
     * @return                true for no errors, false for errors.
     */
    bool ercCheckBusToBusConflicts( const CONNECTION_SUBGRAPH* aSubgraph );

    /**
     * Check one subgraph for conflicting bus entry to bus connections.
     *
     * For example, a wire with label "A0" is connected to a bus labeled "D[8..0]"
     *
     * Will also check for mistakes related to bus group names, for example:
     * A bus group named "USB{DP DM}" should have bus entry connections like
     * "USB.DP" but someone might accidentally just enter "DP".
     *
     * @param  aSubgraph      is the subgraph to examine.
     * @return                true for no errors, false for errors.
     */
    bool ercCheckBusToBusEntryConflicts( const CONNECTION_SUBGRAPH* aSubgraph );

    /**
     * Check one subgraph for proper presence or absence of no-connect symbols.
     *
     * A pin with a no-connect symbol should not have any connections.
     * A pin without a no-connect symbol should have at least one connection.
     *
     * @param  aSubgraph      is the subgraph to examine.
     * @return                true for no errors, false for errors.
     */
    bool ercCheckNoConnects( const CONNECTION_SUBGRAPH* aSubgraph );

    /**
     * Check one subgraph for floating wires.
     *
     * Will throw an error for any subgraph that consists of just wires with no driver.
     *
     * @param  aSubgraph      is the subgraph to examine.
     * @return                true for no errors, false for errors.
     */
    bool ercCheckFloatingWires( const CONNECTION_SUBGRAPH* aSubgraph );

    /**
     * Check one subgraph for dangling wire endpoints.
     *
     * Will throw an error for any subgraph that has wires with only one endpoing
     *
     * @param  aSubgraph      is the subgraph to examine.
     * @return                true for no errors, false for errors.
     */
    bool ercCheckDanglingWireEndpoints( const CONNECTION_SUBGRAPH* aSubgraph );

    /**
     * Check one subgraph for proper connection of labels.
     *
     * Labels should be connected to something.
     *
     * @param  aSubgraph      is the subgraph to examine.
     * @param  aCheckGlobalLabels is true if global labels should be checked for loneliness.
     * @return                true for no errors, false for errors.
     */
    bool ercCheckLabels( const CONNECTION_SUBGRAPH* aSubgraph );

    /**
     * Check directive labels should be connected to something.
     *
     * @return                the number of errors found.
     */
    int ercCheckDirectiveLabels();

    /**
     * Check that a hierarchical sheet has at least one matching label inside the sheet for each
     * port on the parent sheet object.
     *
     * @param  aSubgraph      is the subgraph to examine.
     * @return                the number of errors found.
     */
    int ercCheckHierSheets();

    /**
     * Check that a global label is instantiated more that once across the schematic hierarchy
     */
    int ercCheckSingleGlobalLabel();

    /**
     * Get the number of pins in a given subgraph.
     *
     * @param aLocSubgraph Subgraph to search
     * @return total number of pins in the subgraph
     */
    size_t hasPins( const CONNECTION_SUBGRAPH* aLocSubgraph );


private:
    /// All the sheets in the schematic (as long as we don't have partial updates).
    SCH_SHEET_LIST m_sheetList;

    /// All connectable items in the schematic.
    std::vector<SCH_ITEM*> m_items;

    /// The owner of all #CONNECTION_SUBGRAPH objects.
    std::vector<CONNECTION_SUBGRAPH*> m_subgraphs;

    /// Cache of a subset of #m_subgraphs.
    std::vector<CONNECTION_SUBGRAPH*> m_driver_subgraphs;

    /// Cache to lookup subgraphs in #m_driver_subgraphs by sheet path.
    std::unordered_map<SCH_SHEET_PATH, std::vector<CONNECTION_SUBGRAPH*>> m_sheet_to_subgraphs_map;

    std::vector<std::pair<SCH_SHEET_PATH, SCH_PIN*>> m_global_power_pins;

    std::unordered_map<wxString, std::shared_ptr<BUS_ALIAS>> m_bus_alias_cache;

    std::unordered_map<wxString, int> m_net_name_to_code_map;

    std::unordered_map<wxString, int> m_bus_name_to_code_map;

    std::unordered_map<wxString, std::vector<const CONNECTION_SUBGRAPH*>> m_global_label_cache;

    std::map< std::pair<SCH_SHEET_PATH, wxString>,
              std::vector<const CONNECTION_SUBGRAPH*> > m_local_label_cache;

    std::unordered_map<wxString, std::vector<CONNECTION_SUBGRAPH*>> m_net_name_to_subgraphs_map;

    std::unordered_map<SCH_ITEM*, CONNECTION_SUBGRAPH*> m_item_to_subgraph_map;

    NET_MAP m_net_code_to_subgraphs_map;

    int m_last_net_code;

    int m_last_bus_code;

    int m_last_subgraph_code;

    SCHEMATIC* m_schematic;     ///< The schematic this graph represents.
};

#endif
