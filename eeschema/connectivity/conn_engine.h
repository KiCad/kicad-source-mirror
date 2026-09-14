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
struct UNMAPPED_PIN_CANDIDATE
{
    KIID_PATH sheet;
    KIID      item;
    VECTOR2I  position;
    wxString  number;
    wxString  footprint;
};

struct DRIVER_CONFLICT
{
    KIID_PATH sheet;
    KIID      first;
    KIID      second;
    wxString  firstName;
    wxString  secondName;
    VECTOR2I  position;
};

struct WIRE_ENDPOINT
{
    KIID_PATH sheet;
    KIID      item;
    VECTOR2I  position;
    bool      busEntry = false;
};

struct BUS_NET_CONFLICT
{
    KIID_PATH sheet;
    KIID      net;
    KIID      bus;
    VECTOR2I  position;
};

struct BUS_BUS_CONFLICT
{
    KIID_PATH sheet;
    KIID      canonical;
    KIID      other;
    VECTOR2I  position;
    bool      mixedShapes = false;
};

struct BUS_ENTRY_CONFLICT
{
    KIID_PATH sheet;
    KIID      entry;
    KIID      bus;
    wxString  netName;
    wxString  busName;
    VECTOR2I  position;
};

struct NO_CONNECT_PIN_CONFLICT
{
    KIID_PATH         sheet;
    std::vector<KIID> pins;
    std::vector<KIID> others;
    VECTOR2I          position;
};

struct NO_CONNECT_FLAG_ERROR
{
    KIID_PATH sheet;
    KIID      flag;
    KIID      pin;
    VECTOR2I  position;
    bool      connected = false;
};

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

struct FOOTPRINT_SOURCE
{
    KIID_PATH     sheet;
    KIID          item = niluuid;
    VECTOR2I      position;
    wxString      footprint;
    wxArrayString filters;
};

struct MULTI_UNIT_INSTANCE
{
    KIID_PATH sheet;
    KIID      item;
    VECTOR2I  position;
    wxString  name;
    wxString  footprint;
    int       unit = 0;
};

struct MULTI_UNIT_GROUP
{
    wxString                         reference;
    std::vector<MULTI_UNIT_INSTANCE> instances;
    std::vector<UNIT_FACT>           units;
};

struct DUPLICATE_SHEET_ERROR
{
    KIID_PATH sheet;
    KIID      main;
    KIID      auxiliary;
    VECTOR2I  position;
};

struct FIELD_NAME_ERROR
{
    KIID_PATH       sheet;
    FIELD_NAME_FACT field;
};

struct OFF_GRID_ENDPOINT
{
    KIID_PATH                              sheet;
    KIID                                   item;
    VECTOR2I                               position;
    std::vector<std::pair<KIID, VECTOR2I>> equivalentPins;
};

struct NAMED_ITEM
{
    KIID_PATH sheet;
    KIID      item;
    VECTOR2I  position;
    wxString  name;
    KICAD_T   type = TYPE_NOT_INIT;
    bool      global = false;
};

struct ERC_PIN
{
    KIID_PATH sheet;
    KIID      owner;
    wxString  reference;
    PIN_FACT  pin;
};

struct ERC_PIN_NET
{
    wxString             name;
    std::vector<ERC_PIN> pins;
    bool                 noConnect = false;
    bool                 powerDriven = false;
};

struct MULTI_UNIT_CONFLICT
{
    LABEL_LOCATION first;
    LABEL_LOCATION other;
    wxString       number;
    wxString       firstNet;
    wxString       otherNet;
};

struct LABEL_CONNECTION_ERROR
{
    KIID_PATH sheet;
    KIID      item;
    VECTOR2I  position;
    bool      singlePin = false;
};

struct LABEL_WIRE_CONFLICT
{
    KIID_PATH         sheet;
    KIID              label;
    VECTOR2I          position;
    std::vector<KIID> wires;
};

struct GROUND_PIN_ERROR
{
    KIID_PATH sheet;
    KIID      pin;
    VECTOR2I  position;
    wxString  name;
};

struct FOUR_WAY_JUNCTION
{
    KIID_PATH         sheet;
    VECTOR2I          position;
    std::vector<KIID> items;
    std::vector<KIID> equivalentItems;
};

struct HIERARCHY_ERROR
{
    enum class        KIND { ROOT_LABEL, DANGLING_PIN, MISSING_LABEL, MISSING_PIN };
    KIID_PATH         sheet;
    KIID              item;
    VECTOR2I          position;
    wxString          name;
    KIND              kind = KIND::ROOT_LABEL;
    std::vector<KIID> equivalentItems;
};

struct FLOATING_WIRE
{
    KIID_PATH         sheet;
    std::vector<KIID> items;
    VECTOR2I          position;
};

// Owns one key/version session and its current stage evaluations; main-thread use only.
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

    // Owned eligible component claims, strongest first; repeated slot claims are retained.
    std::vector<CLAIM> DriverCandidates( NODE_ID aComponent ) const;

    // Owned diagnostics from the last publication, in canonical order.
    std::vector<DRIVER_CONFLICT>         DriverConflicts() const;
    std::vector<WIRE_ENDPOINT>           DanglingWireEndpoints() const;
    std::vector<FLOATING_WIRE>           FloatingWires() const;
    std::vector<BUS_NET_CONFLICT>        BusNetConflicts() const;
    std::vector<BUS_BUS_CONFLICT>        BusBusConflicts() const;
    std::vector<BUS_ENTRY_CONFLICT>      BusEntryConflicts() const;
    std::vector<NO_CONNECT_PIN_CONFLICT> NoConnectPinConflicts() const;
    std::vector<NO_CONNECT_FLAG_ERROR>   NoConnectFlagErrors() const;
    std::vector<UNCONNECTED_PIN>         UnconnectedPins() const;
    std::vector<LABEL_LOCATION>          DanglingDirectives() const;
    std::vector<LABEL_LOCATION>          SingleGlobalLabels() const;
    std::vector<NAMED_ITEM>              NamedItems() const;
    std::vector<MULTI_UNIT_CONFLICT>     MultiUnitPinConflicts() const;
    std::vector<ERC_PIN_NET>             PinNets() const;
    std::vector<LABEL_CONNECTION_ERROR>  LabelConnectionErrors() const;
    std::vector<LABEL_WIRE_CONFLICT>     LabelWireConflicts() const;
    std::vector<FOUR_WAY_JUNCTION>       FourWayJunctions() const;
    std::vector<SOURCE_LOCATION>         EmptyLabels() const;
    std::vector<FIELD_NAME_ERROR>        InvalidFieldNames() const;
    std::vector<DUPLICATE_SHEET_ERROR>   DuplicateSheetNames() const;
    std::vector<LIBRARY_SYMBOL_FACT>     LibrarySymbols( const KIID_PATH& aPath ) const;
    std::vector<VARIANT_SYMBOL_FACT>     VariantSymbols( const KIID_PATH& aPath ) const;
    std::vector<FOOTPRINT_SOURCE>        FootprintSources() const;
    std::vector<MULTI_UNIT_GROUP>        MultiUnitSymbols() const;
    std::vector<PIN_MAP_FACT>            PinMapSymbols( SCREEN_ID aScreen ) const;
    std::vector<UNMAPPED_PIN_CANDIDATE>  UnmappedPinCandidates() const;
    std::vector<SOURCE_LOCATION>         InvalidPinNotation() const;
    std::vector<SOURCE_LOCATION>         WiredImplicitPowerPins() const;
    std::vector<OFF_GRID_ENDPOINT>       OffGridEndpoints( int aGrid ) const;
    std::vector<GROUND_PIN_ERROR>        GroundPinErrors() const;
    std::vector<NETCLASS_REFERENCE>      NetclassReferences() const;
    std::vector<HIERARCHY_ERROR>         HierarchyErrors() const;

    // Current parser presentation, independent of electrical schema reuse.
    std::shared_ptr<const BUS_SCHEMA::NODE> FindBusTree( const wxString& aText ) const
    {
        return m_records.FindTree( aText );
    }

private:
    void clearStages( bool aRetainSourceValues = false );

    std::vector<INST_ID> instancesInPageOrder() const;

    // Driven islands repeated across instances of one screen report once, on the first instance in page order.
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
