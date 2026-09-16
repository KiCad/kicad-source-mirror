/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include "conn_publish.h"

namespace SCH_CONNECTIVITY
{
/**
 * A pin with a footprint that is not dangling, or that ignores dangling state. The engine cannot
 * load footprints, so ERC_TESTER::TestPinMap() looks up the pads and raises
 * ERCE_PIN_MAP_UNMAPPED_PIN.
 */
struct UNMAPPED_PIN_CANDIDATE
{
    KIID_PATH sheet;
    KIID      item;
    VECTOR2I  position;
    wxString  number;
    wxString  footprint;
};

/**
 * Two different strong names on one island. The first name drives the net, and the marker sits on
 * the second source. ERC raises ERCE_DRIVER_CONFLICT.
 */
struct DRIVER_CONFLICT
{
    KIID_PATH sheet;
    KIID      first;
    KIID      second;
    wxString  firstName;
    wxString  secondName;
    VECTOR2I  position;
};

/**
 * One dangling end of a wire or a bus wire entry. ERC raises ERCE_UNCONNECTED_WIRE_ENDPOINT.
 */
struct WIRE_ENDPOINT
{
    KIID_PATH sheet;
    KIID      item;
    VECTOR2I  position;
    bool      busEntry = false;
};

/**
 * An island that mixes net and bus items. The marker sits on the net item. ERC raises
 * ERCE_BUS_TO_NET_CONFLICT.
 */
struct BUS_NET_CONFLICT
{
    KIID_PATH sheet;
    KIID      net;
    KIID      bus;
    VECTOR2I  position;
};

/**
 * Two bus claims on one island that share no member, or a bus vector joined to a bus group. ERC
 * raises ERCE_BUS_TO_BUS_CONFLICT.
 */
struct BUS_BUS_CONFLICT
{
    KIID_PATH sheet;
    KIID      canonical;
    KIID      other;
    VECTOR2I  position;
    bool      mixedShapes = false;
};

/**
 * A bus wire entry whose net is not a member of the bus it touches. ERC raises
 * ERCE_BUS_ENTRY_CONFLICT.
 */
struct BUS_ENTRY_CONFLICT
{
    KIID_PATH sheet;
    KIID      entry;
    KIID      bus;
    wxString  netName;
    wxString  busName;
    VECTOR2I  position;
};

/**
 * Pins of type no-connect that touch other items at one position. A no-connect flag, another
 * no-connect pin or a power flag is not a conflict. ERC raises ERCE_NOCONNECT_CONNECTED.
 */
struct NO_CONNECT_PIN_CONFLICT
{
    KIID_PATH         sheet;
    std::vector<KIID> pins;     ///< No-connect pins at the position, sorted.
    std::vector<KIID> others;   ///< Items that touch those pins, sorted.
    VECTOR2I          position;
};

/**
 * A no-connect flag on a net with more than one pin, or on a net with no pin and no label. ERC
 * raises ERCE_NOCONNECT_CONNECTED or ERCE_NOCONNECT_NOT_CONNECTED.
 */
struct NO_CONNECT_FLAG_ERROR
{
    KIID_PATH sheet;
    KIID      flag;
    KIID      pin;                 ///< If connected, the smallest pin that is not a power flag. Else niluuid.
    VECTOR2I  position;            ///< The pin position if connected and a pin exists, else the flag.
    bool      connected = false;   ///< True for more than one pin, false for an unused flag.
};

/**
 * One item on one sheet instance, for a diagnostic that needs only a location.
 */
struct SOURCE_LOCATION
{
    KIID_PATH sheet;
    KIID      item;
    VECTOR2I  position;
};

using UNCONNECTED_PIN = SOURCE_LOCATION;
using LABEL_LOCATION = SOURCE_LOCATION;

// Orders sheet-local positions by instance path, then by x and y.
struct SHEET_POSITION_LESS
{
    bool operator()( const std::pair<KIID_PATH, VECTOR2I>& a, const std::pair<KIID_PATH, VECTOR2I>& b ) const
    {
        // Tuple comparison would bypass KIID_PATH's length-first ordering
        if( a.first != b.first )
            return a.first < b.first;

        // VECTOR2I orders by length, which merges distinct positions at the same radius
        return std::tie( a.second.x, a.second.y ) < std::tie( b.second.x, b.second.y );
    }
};

/**
 * The resolved footprint and the library footprint filters of one symbol on one sheet instance.
 */
struct FOOTPRINT_SOURCE
{
    KIID_PATH     sheet;
    KIID          item = niluuid;
    VECTOR2I      position;
    wxString      footprint;
    wxArrayString filters;
};

/**
 * One placed unit of an annotated multi-unit symbol.
 */
struct MULTI_UNIT_INSTANCE
{
    KIID_PATH sheet;
    KIID      item;
    VECTOR2I  position;
    wxString  name;
    wxString  footprint;
    int       unit = 0;
};

/**
 * All placed units that share one reference, with the unit table of the library symbol.
 */
struct MULTI_UNIT_GROUP
{
    wxString                         reference;
    std::vector<MULTI_UNIT_INSTANCE> instances;
    std::vector<UNIT_FACT>           units;
};

/**
 * Two child sheets of one sheet instance whose names differ only in case. ERC raises
 * ERCE_DUPLICATE_SHEET_NAME.
 */
struct DUPLICATE_SHEET_ERROR
{
    KIID_PATH sheet;
    KIID      main;
    KIID      auxiliary;
    VECTOR2I  position;
};

/**
 * A symbol or sheet field name with leading or trailing white space. ERC raises
 * ERCE_FIELD_NAME_WHITESPACE.
 */
struct FIELD_NAME_ERROR
{
    KIID_PATH       sheet;
    FIELD_NAME_FACT field;
};

/**
 * A wire end, a bus entry end or a symbol pin that is not on the connection grid. ERC raises
 * ERCE_ENDPOINT_OFF_GRID.
 */
struct OFF_GRID_ENDPOINT
{
    KIID_PATH                              sheet;
    KIID                                   item;
    VECTOR2I                               position;

    /** All off-grid pins of the symbol. A saved exclusion on any of them still matches. */
    std::vector<std::pair<KIID, VECTOR2I>> equivalentPins;
};

/**
 * A label or a power pin with its resolved net name. ERC compares these names to find similar names,
 * and local names that match global names.
 */
struct NAMED_ITEM
{
    KIID_PATH sheet;
    KIID      item;
    VECTOR2I  position;
    wxString  name;
    KICAD_T   type = TYPE_NOT_INIT;
    bool      global = false;
};

/**
 * One active symbol pin on one sheet instance, with the reference of its symbol.
 */
struct ERC_PIN
{
    KIID_PATH sheet;
    KIID      owner;
    wxString  reference;
    PIN_FACT  pin;
};

/**
 * All active pins that the publication places on one named net. ERC checks each net against the
 * pin conflict matrix.
 */
struct ERC_PIN_NET
{
    wxString             name;
    std::vector<ERC_PIN> pins;
    bool                 noConnect = false;
    bool                 powerDriven = false;
};

/**
 * A pin number of one multi-unit symbol that different units connect to different nets. ERC
 * raises ERCE_DIFFERENT_UNIT_NET.
 */
struct MULTI_UNIT_CONFLICT
{
    LABEL_LOCATION first;
    LABEL_LOCATION other;
    wxString       number;
    wxString       firstNet;
    wxString       otherNet;
};

/**
 * A label that connects to nothing, or to only one pin. ERC raises ERCE_LABEL_NOT_CONNECTED or
 * ERCE_LABEL_SINGLE_PIN.
 */
struct LABEL_CONNECTION_ERROR
{
    KIID_PATH sheet;
    KIID      item;
    VECTOR2I  position;
    bool      singlePin = false;
};

/**
 * A label that sits on the interior of two or more wires. ERC raises ERCE_LABEL_MULTIPLE_WIRES.
 */
struct LABEL_WIRE_CONFLICT
{
    KIID_PATH         sheet;
    KIID              label;
    VECTOR2I          position;
    std::vector<KIID> wires;
};

/**
 * A power pin with a ground name that is not on a ground net, on a symbol with another pin on a
 * ground net. ERC raises ERCE_GROUND_PIN_NOT_GROUND.
 */
struct GROUND_PIN_ERROR
{
    KIID_PATH sheet;
    KIID      pin;
    VECTOR2I  position;
    wxString  name;
};

/**
 * Four or more contacts at one position. Wire ends count once each, and the pins of one symbol
 * count once. ERC raises ERCE_FOUR_WAY_JUNCTION.
 */
struct FOUR_WAY_JUNCTION
{
    KIID_PATH         sheet;
    VECTOR2I          position;
    std::vector<KIID> items;           ///< One pin for each symbol, then the lines; ERC reports four.

    /** Every pin and line at the position, sorted. A saved exclusion may name any of them. */
    std::vector<KIID> equivalentItems;
};

/**
 * A hierarchical label or sheet pin with no partner, or a dangling sheet pin. ERC raises
 * ERCE_HIERACHICAL_LABEL for a missing partner and ERCE_PIN_NOT_CONNECTED for the other kinds.
 */
struct HIERARCHY_ERROR
{
    enum class        KIND { ROOT_LABEL, DANGLING_PIN, MISSING_LABEL, MISSING_PIN };
    KIID_PATH         sheet;
    KIID              item;
    VECTOR2I          position;
    wxString          name;
    KIND              kind = KIND::ROOT_LABEL;

    /** For a missing partner, all same-named labels or sheet pins. An exclusion may name any of them. */
    std::vector<KIID> equivalentItems;
};

/**
 * Up to four wires and bus entries of one island that has no driver. ERC raises
 * ERCE_WIRE_DANGLING.
 */
struct FLOATING_WIRE
{
    KIID_PATH         sheet;
    std::vector<KIID> items;
    VECTOR2I          position;
};

/**
 * Owns one key/version session and its current stage evaluations; main-thread use only.
 *
 * @see @ref schematic_connectivity for the stage order and the update flows.
 */
class ENGINE
{
public:
    ENGINE();
    ENGINE( const ENGINE& ) = delete;
    ENGINE& operator=( const ENGINE& ) = delete;

    /**
     * Refresh inputs, bundle slots and signal summaries with the source text epoch and effective aliases.
     * Rebuilding discards stage caches but retains the published history used for continuity.
     * Failure discards every stage output, including the previous successful evaluation.
     */
    void Update( const SCH_SHEET_LIST& aPaths, uint64_t aTextEpoch, const BUS_ALIASES& aAliases,
                 bool aRebuild = false );

    // Compare retained external text sources before the next Update, in its fresh text frame.
    bool ExternalSourcesChanged( const SCH_SHEET_LIST& aPaths, bool aContextUnchanged = false ) const
    {
        return m_inputs.ExternalSourcesChanged( aPaths, aContextUnchanged );
    }

    // Discard evaluations while preserving session identities and version monotonicity.
    void Clear();

    const SESSION_KEYS&                   Keys() const { return m_keys; }
    const COMPONENT_CACHE<SIGNAL_RESULT>&  Signals() const { return m_signals; }
    const PUBLICATION&                    Published() const { return m_published; }

    /**
     * Owned eligible claims of one published component, strongest first. Repeated slot claims stay in the
     * result. UnconnectedPins() uses them to find a driver elsewhere on the net.
     */
    std::vector<CLAIM> DriverCandidates( NODE_ID aComponent ) const;

    /**
     * @name ERC diagnostics
     * Owned results that describe the last publication, in a deterministic order. Call them on the main
     * thread after SCH_CONNECTIVITY::FACADE::Recalculate() and before any later update. They do not check
     * screen revisions. Accessors that walk islands report a shared screen once, on its first instance in
     * page order. ERC_TESTER turns each result into markers.
     *
     * @{
     */

    /** Islands with two different strong names; one result for each reported island. */
    std::vector<DRIVER_CONFLICT>         DriverConflicts() const;

    /** Dangling ends of wires and bus wire entries on reported islands. */
    std::vector<WIRE_ENDPOINT>           DanglingWireEndpoints() const;

    /** Wires and bus entries of each island whose net has no driver, on every instance. */
    std::vector<FLOATING_WIRE>           FloatingWires() const;

    /** Reported islands whose ERC_ATOMS::kindConflict is set and that hold both a net and a bus item. */
    std::vector<BUS_NET_CONFLICT>        BusNetConflicts() const;

    /** Reported bus islands with mixed bus shapes, or with two claims that share no member. */
    std::vector<BUS_BUS_CONFLICT>        BusBusConflicts() const;

    /** Bus wire entries on reported islands whose net is not a member of the touched bus. */
    std::vector<BUS_ENTRY_CONFLICT>      BusEntryConflicts() const;

    /** No-connect pins that touch other items, grouped by instance and position. */
    std::vector<NO_CONNECT_PIN_CONFLICT> NoConnectPinConflicts() const;

    /** No-connect flags on reported islands that connect to many pins or to nothing. */
    std::vector<NO_CONNECT_FLAG_ERROR>   NoConnectFlagErrors() const;

    /** Unconnected symbol pins on reported islands; each power symbol pin reports on its own. */
    std::vector<UNCONNECTED_PIN>         UnconnectedPins() const;

    /** Dangling directive labels on every instance. */
    std::vector<LABEL_LOCATION>          DanglingDirectives() const;

    /** Global labels whose name occurs only once in the hierarchy. */
    std::vector<LABEL_LOCATION>          SingleGlobalLabels() const;

    /** Every label and power pin with its resolved name, on every instance. */
    std::vector<NAMED_ITEM>              NamedItems() const;

    /** One conflict for each multi-unit reference and pin number with more than one net. */
    std::vector<MULTI_UNIT_CONFLICT>     MultiUnitPinConflicts() const;

    /** Active pins for each named net, in net name order. */
    std::vector<ERC_PIN_NET>             PinNets() const;

    /** Unconnected and single-pin labels on reported islands. */
    std::vector<LABEL_CONNECTION_ERROR>  LabelConnectionErrors() const;

    /** Labels that touch the interior of two or more wires; one result for each position. */
    std::vector<LABEL_WIRE_CONFLICT>     LabelWireConflicts() const;

    /** Positions with four or more contacts, sorted by instance and position. */
    std::vector<FOUR_WAY_JUNCTION>       FourWayJunctions() const;

    /** Labels whose raw text is empty or only white space, on every instance. */
    std::vector<SOURCE_LOCATION>         EmptyLabels() const;

    /** Field names with leading or trailing white space, sorted by instance, owner and field. */
    std::vector<FIELD_NAME_ERROR>        InvalidFieldNames() const;

    /** Child sheet pairs whose names differ only in case, sorted by instance and pair. */
    std::vector<DUPLICATE_SHEET_ERROR>   DuplicateSheetNames() const;

    /** Captured library links of the symbols on the screen of one instance; empty for an unknown path. */
    std::vector<LIBRARY_SYMBOL_FACT>     LibrarySymbols( const KIID_PATH& aPath ) const;

    /** Captured variant symbol overrides of one instance; empty for an unknown path. */
    std::vector<VARIANT_SYMBOL_FACT>     VariantSymbols( const KIID_PATH& aPath ) const;

    /** Resolved footprint and filters of every symbol on every instance. */
    std::vector<FOOTPRINT_SOURCE>        FootprintSources() const;

    /** Annotated multi-unit symbols grouped by reference, with instances in page order. */
    std::vector<MULTI_UNIT_GROUP>        MultiUnitSymbols() const;

    /** Captured pin maps of one screen; empty for an unknown screen. */
    std::vector<PIN_MAP_FACT>            PinMapSymbols( SCREEN_ID aScreen ) const;

    /** Pins that need a pad check. See UNMAPPED_PIN_CANDIDATE. */
    std::vector<UNMAPPED_PIN_CANDIDATE>  UnmappedPinCandidates() const;

    /** Pins whose shown number looks like stacked pin notation but does not parse. */
    std::vector<SOURCE_LOCATION>         InvalidPinNotation() const;

    /** Hidden global power input pins that a wire also joins; one pin for each symbol. */
    std::vector<SOURCE_LOCATION>         WiredImplicitPowerPins() const;

    /** Off-grid endpoints on the first instance of each screen; one pin for each symbol. */
    std::vector<OFF_GRID_ENDPOINT>       OffGridEndpoints( int aGrid ) const;

    /** Ground-named power pins off a ground net, sorted by instance and pin. */
    std::vector<GROUND_PIN_ERROR>        GroundPinErrors() const;

    /** Netclass field references of every instance, sorted by instance, item and name. */
    std::vector<NETCLASS_REFERENCE>      NetclassReferences() const;

    /** Hierarchical label and sheet pin mismatches, sorted by instance, item and kind. */
    std::vector<HIERARCHY_ERROR>         HierarchyErrors() const;

    /** @} */

    // Current parser presentation, independent of electrical schema reuse.
    std::shared_ptr<const BUS_SCHEMA::NODE> FindBusTree( const wxString& aText ) const
    {
        return m_records.FindTree( aText );
    }

private:
    void clearStages( bool aRetainSourceValues = false );

    /** All captured instances, sorted by page order and then by instance path. */
    std::vector<INST_ID> instancesInPageOrder() const;

    /**
     * Islands that ERC reports. A screen with many instances has one driven island on each instance,
     * and all of them share the same strongest source item. Only the instance first in page order is
     * kept, as CONNECTION_GRAPH::RunERC() does. Every undriven island is kept.
     */
    std::set<RECORD_KEY, KEY_LESS> ercIslands() const;

    SESSION_KEYS                    m_keys;
    CACHE_VERSIONS                  m_versions;
    INPUT_STORE                     m_inputs;
    RECORD_STORE                    m_records;
    PARTITIONER                     m_partitioner;
    COMPONENT_CACHE<BUNDLE_BINDING> m_bundles;
    SLOT_STORE                      m_slots;
    COMPONENT_CACHE<SIGNAL_RESULT>  m_signals;
    PUBLICATION                     m_published;
};
} // namespace SCH_CONNECTIVITY
