/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 CERN
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
#include <vector>

#include <common.h>
#include <erc_settings.h>
#include <sch_connection.h>
#include <sch_item.h>


#ifdef DEBUG
// Uncomment this line to enable connectivity debugging features
// #define CONNECTIVITY_DEBUG
#endif


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
 *
 */
class CONNECTION_SUBGRAPH
{
public:
    enum PRIORITY {
        PRIORITY_NONE = 0,
        PRIORITY_PIN,
        PRIORITY_SHEET_PIN,
        PRIORITY_HIER_LABEL,
        PRIORITY_LOCAL_LABEL,
        PRIORITY_POWER_PIN,
        PRIORITY_GLOBAL
    };

    explicit CONNECTION_SUBGRAPH( SCH_EDIT_FRAME* aFrame ) :
        m_dirty( false ), m_absorbed( false ), m_absorbed_by( nullptr ), m_code( -1 ),
        m_multiple_drivers( false ), m_strong_driver( false ), m_local_driver( false ),
        m_no_connect( nullptr ), m_bus_entry( nullptr ), m_driver( nullptr ), m_frame( aFrame ),
        m_driver_connection( nullptr )
    {}
    /**
     * Determines which potential driver should drive the subgraph.
     *
     * If multiple possible drivers exist, picks one according to the priority.
     * If multiple "winners" exist, returns false and sets m_driver to nullptr.
     *
     * @param aCreateMarkers controls whether ERC markers should be added for conflicts
     * @return true if m_driver was set, or false if a conflict occurred
     */
    bool ResolveDrivers( bool aCreateMarkers = false );

    /**
     * Returns the fully-qualified net name for this subgraph (if one exists)
     */
    wxString GetNetName() const;

    /// Returns all the bus labels attached to this subgraph (if any)
    std::vector<SCH_ITEM*> GetBusLabels() const;

    /// Returns the candidate net name for a driver
    wxString GetNameForDriver( SCH_ITEM* aItem ) const;

    /// Combines another subgraph on the same sheet into this one.
    void Absorb( CONNECTION_SUBGRAPH* aOther );

    /// Adds a new item to the subgraph
    void AddItem( SCH_ITEM* aItem );

    /// Updates all items to match the driver connection
    void UpdateItemConnections();

    /**
     * Returns the priority (higher is more important) of a candidate driver
     *
     * 0: Invalid driver
     * 1: Component pin
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

    bool m_dirty;

    /// True if this subgraph has been absorbed into another.  No pointers here are safe if so!
    bool m_absorbed;

    /// If this subgraph is absorbed, points to the absorbing (and valid) subgraph
    CONNECTION_SUBGRAPH* m_absorbed_by;

    long m_code;

    /**
     * True if this subgraph contains more than one driver that should be
     * shorted together in the netlist.  For example, two labels or
     * two power ports.
     */
    bool m_multiple_drivers;

    /// True if the driver is "strong": a label or power object
    bool m_strong_driver;

    /// True if the driver is a local (i.e. non-global) type
    bool m_local_driver;

    /// No-connect item in graph, if any
    SCH_ITEM* m_no_connect;

    /// Bus entry in graph, if any
    SCH_ITEM* m_bus_entry;

    std::vector<SCH_ITEM*> m_items;

    std::vector<SCH_ITEM*> m_drivers;

    SCH_ITEM* m_driver;

    SCH_SHEET_PATH m_sheet;

    // Needed for m_userUnits for now; maybe refactor later
    SCH_EDIT_FRAME* m_frame;

    /// Cache for driver connection
    SCH_CONNECTION* m_driver_connection;

    /**
     * If a subgraph is a bus, this map contains links between the bus members and any
     * local sheet neighbors with the same connection name.
     *
     * For example, if this subgraph is a bus D[7..0], and on the same sheet there is
     * a net with label D7, this map will contain an entry for the D7 bus member, and
     * the vector will contain a pointer to the D7 net subgraph.
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

    // Cache for lookup of any hierarchical (sheet) pins on this subgraph (for referring down)
    std::vector<SCH_SHEET_PIN*> m_hier_pins;

    // Cache for lookup of any hierarchical ports on this subgraph (for referring up)
    std::vector<SCH_HIERLABEL*> m_hier_ports;
};


/**
 * Calculates the connectivity of a schematic and generates netlists
 */
class CONNECTION_GRAPH
{
public:
    CONNECTION_GRAPH( SCH_EDIT_FRAME* aFrame) :
        m_frame( aFrame )
    {}

    void Reset();

    /**
     * Updates the connection graph for the given list of sheets.
     *
     * @param aSheetList is the list of possibly modified sheets
     * @param aUnconditional is true if an unconditional full recalculation should be done
     */
    void Recalculate( SCH_SHEET_LIST aSheetList, bool aUnconditional = false );

    /**
     * Returns a bus alias pointer for the given name if it exists (from cache)
     *
     * CONNECTION_GRAPH caches these, they are owned by the SCH_SCREEN that
     * the alias was defined on.  The cache is only used to update the graph.
     */
    std::shared_ptr<BUS_ALIAS> GetBusAlias( const wxString& aName );

    /**
     * Determines which subgraphs have more than one conflicting bus label.
     *
     * @see DIALOG_MIGRATE_BUSES
     * @return a list of subgraphs that need migration
     */

    std::vector<const CONNECTION_SUBGRAPH*> GetBusesNeedingMigration();

    /**
     * Runs electrical rule checks on the connectivity graph.
     *
     * Precondition: graph is up-to-date
     *
     * @param aSettings is used to control which tests to run
     * @param aCreateMarkers controls whether error markers are created
     * @return the number of errors found
     */
    int RunERC( const ERC_SETTINGS& aSettings, bool aCreateMarkers = true );

    // TODO(JE) Remove this when pressure valve is removed
    static bool m_allowRealTime;

    // TODO(JE) firm up API and move to private
    std::map<int, std::vector<CONNECTION_SUBGRAPH*> > m_net_code_to_subgraphs_map;

private:

    std::unordered_set<SCH_ITEM*> m_items;

    // The owner of all CONNECTION_SUBGRAPH objects
    std::vector<CONNECTION_SUBGRAPH*> m_subgraphs;

    // Cache of a subset of m_subgraphs
    std::vector<CONNECTION_SUBGRAPH*> m_driver_subgraphs;

    // Cache to lookup subgraphs in m_driver_subgraphs by sheet path
    std::unordered_map<SCH_SHEET_PATH,
                       std::vector<CONNECTION_SUBGRAPH*>> m_sheet_to_subgraphs_map;

    std::vector<std::pair<SCH_SHEET_PATH, SCH_PIN*>> m_invisible_power_pins;

    std::unordered_map< wxString, std::shared_ptr<BUS_ALIAS> > m_bus_alias_cache;

    std::map<wxString, int> m_net_name_to_code_map;

    std::map<wxString, int> m_bus_name_to_code_map;

    std::map<wxString, std::vector<const CONNECTION_SUBGRAPH*>> m_global_label_cache;

    std::map< std::pair<SCH_SHEET_PATH, wxString>,
              std::vector<const CONNECTION_SUBGRAPH*> > m_local_label_cache;

    std::unordered_map<wxString,
                       std::vector<CONNECTION_SUBGRAPH*>> m_net_name_to_subgraphs_map;

    int m_last_net_code;

    int m_last_bus_code;

    int m_last_subgraph_code;

    std::mutex m_item_mutex;

    // Needed for m_userUnits for now; maybe refactor later
    SCH_EDIT_FRAME* m_frame;

    /**
     * Updates the graphical connectivity between items (i.e. where they touch)
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
     * This means that we can't store SCH_COMPONENTs in this map -- we must store
     * a structure that links a specific pin on a component back to that
     * component: a SCH_PIN_CONNECTION.  This wrapper class is a convenience for
     * linking a pin and component to a specific (x, y) point.
     *
     * In the second phase, we iterate over each value in the map, which is a
     * vector of items that have overlapping connection points.  After some
     * checks to ensure that the items should actually connect, the items are
     * linked together using ConnectedItems().
     *
     * As a side effect, items are loaded into m_items for BuildConnectionGraph()
     *
     * @param aSheet is the path to the sheet of all items in the list
     * @param aItemList is a list of items to consider
     */
    void updateItemConnectivity( SCH_SHEET_PATH aSheet,
                                 std::vector<SCH_ITEM*> aItemList );

    /**
     * Generates the connection graph (after all item connectivity has been updated)
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
     */
    void buildConnectionGraph();

    /**
     * Helper to assign a new net code to a connection
     *
     * @return the assigned code
     */
    int assignNewNetCode( SCH_CONNECTION& aConnection );

    /**
     * Ensures all members of the bus connection have a valid net code assigned
     * @param aConnection is a bus connection
     */
    void assignNetCodesToBus( SCH_CONNECTION* aConnection );

    /**
     * Updates all neighbors of a subgraph with this one's connectivity info
     *
     * If this subgraph contains hierarchical links, this method will descent the
     * hierarchy and propagate the connectivity across all linked sheets.
     */
    void propagateToNeighbors( CONNECTION_SUBGRAPH* aSubgraph );

    /**
     * Search for a matching bus member inside a bus connection
     *
     * For bus groups, this returns a bus member that matches aSearch by name.
     * For bus vectors, this returns a bus member that matches by vector index.
     *
     * @param aBusConnection is the bus connection to search
     * @param aSearch is the net connection to search for
     * @returns a member of aBusConnection that matches aSearch
     */
    static SCH_CONNECTION* matchBusMember( SCH_CONNECTION* aBusConnection,
                                           SCH_CONNECTION* aSearch );

    void recacheSubgraphName( CONNECTION_SUBGRAPH* aSubgraph, const wxString& aOldName );

    /**
     * Checks one subgraph for conflicting connections between net and bus labels
     *
     * For example, a net wire connected to a bus port/pin, or vice versa
     *
     * @param  aSubgraph      is the subgraph to examine
     * @param  aCreateMarkers controls whether error markers are created
     * @return                true for no errors, false for errors
     */
    bool ercCheckBusToNetConflicts( const CONNECTION_SUBGRAPH* aSubgraph,
                                    bool aCreateMarkers );

    /**
     * Checks one subgraph for conflicting connections between two bus items
     *
     * For example, a labeled bus wire connected to a hierarchical sheet pin
     * where the labeled bus doesn't contain any of the same bus members as the
     * sheet pin
     *
     * @param  aSubgraph      is the subgraph to examine
     * @param  aCreateMarkers controls whether error markers are created
     * @return                true for no errors, false for errors
     */
    bool ercCheckBusToBusConflicts( const CONNECTION_SUBGRAPH* aSubgraph,
                                    bool aCreateMarkers );

    /**
     * Checks one subgraph for conflicting bus entry to bus connections
     *
     * For example, a wire with label "A0" is connected to a bus labeled "D[8..0]"
     *
     * Will also check for mistakes related to bus group names, for example:
     * A bus group named "USB{DP DM}" should have bus entry connections like
     * "USB.DP" but someone might accidentally just enter "DP"
     *
     * @param  aSubgraph      is the subgraph to examine
     * @param  aCreateMarkers controls whether error markers are created
     * @return                true for no errors, false for errors
     */
    bool ercCheckBusToBusEntryConflicts( const CONNECTION_SUBGRAPH* aSubgraph,
                                         bool aCreateMarkers );

    /**
     * Checks one subgraph for proper presence or absence of no-connect symbols
     *
     * A pin with a no-connect symbol should not have any connections
     * A pin without a no-connect symbol should have at least one connection
     *
     * @param  aSubgraph      is the subgraph to examine
     * @param  aCreateMarkers controls whether error markers are created
     * @return                true for no errors, false for errors
     */
    bool ercCheckNoConnects( const CONNECTION_SUBGRAPH* aSubgraph,
                             bool aCreateMarkers );

    /**
     * Checks one subgraph for proper connection of labels
     *
     * Labels should be connected to something
     *
     * @param  aSubgraph      is the subgraph to examine
     * @param  aCreateMarkers controls whether error markers are created
     * @param  aCheckGlobalLabels is true if global labels should be checked for loneliness
     * @return                true for no errors, false for errors
     */
    bool ercCheckLabels( const CONNECTION_SUBGRAPH* aSubgraph, bool aCreateMarkers,
                         bool aCheckGlobalLabels );

};

#endif
