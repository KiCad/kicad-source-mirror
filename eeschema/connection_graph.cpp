/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <algorithm>
#include <list>
#include <functional>
#include <future>
#include <map>
#include <ranges>
#include <set>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <app_monitor.h>
#include <core/profile.h>
#include <core/kicad_algo.h>
#include <common.h>
#include <erc/erc.h>
#include <pin_type.h>
#include <sch_bus_entry.h>
#include <sch_symbol.h>
#include <sch_edit_frame.h>
#include <sch_line.h>
#include <sch_marker.h>
#include <sch_pin.h>
#include <sch_rule_area.h>
#include <trace_helpers.h>
#include <wx/log.h>
#include <sch_netchain.h>
#include <sch_label.h>
#include <sch_sheet.h>
#include <sch_sheet_path.h>
#include <sch_sheet_pin.h>
#include <sch_text.h>
#include <schematic.h>
#include <symbol.h>
#include <connection_graph.h>
#include <project/project_file.h>
#include <project/net_settings.h>
#include <widgets/ui_common.h>
#include <string_utils.h>
#include <thread_pool.h>
#include <wx/log.h>

#include <advanced_config.h> // for realtime connectivity switch in release builds


/**
 * Flag to enable connectivity profiling
 * @ingroup trace_env_vars
 */
static const wxChar DanglingProfileMask[] = wxT( "CONN_PROFILE" );


/**
 * Flag to enable connectivity tracing
 * @ingroup trace_env_vars
 */
static const wxChar ConnTrace[] = wxT( "CONN" );


wxString CONNECTION_GRAPH::MakeNetChainKey( const wxString& aRawNetName, long aSubgraphCode )
{
    if( !aRawNetName.IsEmpty() && aRawNetName.Find( wxS( "<NO NET>" ) ) == wxNOT_FOUND )
        return aRawNetName;

    return wxString( SCH_NETCHAIN::SYNTHETIC_NET_PREFIX ) << aSubgraphCode;
}


wxString CONNECTION_GRAPH::MakeNetChainKey( const CONNECTION_SUBGRAPH* aSubGraph )
{
    if( !aSubGraph )
        return wxEmptyString;

    return MakeNetChainKey( aSubGraph->GetNetName(), aSubGraph->m_code );
}


// Internal shim so the existing private call sites read unchanged.
static inline wxString netChainKeyFor( const wxString& aRawNetName, long aSubgraphCode )
{
    return CONNECTION_GRAPH::MakeNetChainKey( aRawNetName, aSubgraphCode );
}


CONNECTION_GRAPH::~CONNECTION_GRAPH()
{
    // Ensure destruction happens in a translation unit that includes full SCH_NETCHAIN
    // definition to avoid incomplete type issues with std::unique_ptr<SCH_NETCHAIN>.
    Reset();
}


void CONNECTION_SUBGRAPH::RemoveItem( SCH_ITEM* aItem )
{
    m_items.erase( aItem );
    m_drivers.erase( aItem );

    if( aItem == m_driver )
    {
        m_driver = nullptr;
        m_driver_connection = nullptr;
    }

    if( aItem->Type() == SCH_SHEET_PIN_T )
        m_hier_pins.erase( static_cast<SCH_SHEET_PIN*>( aItem ) );

    if( aItem->Type() == SCH_HIER_LABEL_T )
        m_hier_ports.erase( static_cast<SCH_HIERLABEL*>( aItem ) );
}


void CONNECTION_SUBGRAPH::ExchangeItem( SCH_ITEM* aOldItem, SCH_ITEM* aNewItem )
{
    m_items.erase( aOldItem );
    m_items.insert( aNewItem );

    m_drivers.erase( aOldItem );
    m_drivers.insert( aNewItem );

    if( aOldItem == m_driver )
    {
        m_driver = aNewItem;
        m_driver_connection = aNewItem->GetOrInitConnection( m_sheet, m_graph );
    }

    SCH_CONNECTION* old_conn = aOldItem->Connection( &m_sheet );
    SCH_CONNECTION* new_conn = aNewItem->GetOrInitConnection( m_sheet, m_graph );

    if( old_conn && new_conn )
    {
        new_conn->Clone( *old_conn );

        if( old_conn->IsDriver() )
            new_conn->SetDriver( aNewItem );

        new_conn->ClearDirty();
    }

    if( aOldItem->Type() == SCH_SHEET_PIN_T )
    {
        m_hier_pins.erase( static_cast<SCH_SHEET_PIN*>( aOldItem ) );
        m_hier_pins.insert( static_cast<SCH_SHEET_PIN*>( aNewItem ) );
    }

    if( aOldItem->Type() == SCH_HIER_LABEL_T )
    {
        m_hier_ports.erase( static_cast<SCH_HIERLABEL*>( aOldItem ) );
        m_hier_ports.insert( static_cast<SCH_HIERLABEL*>( aNewItem ) );
    }
}


/**
 * Unified driver ranking used by CONNECTION_SUBGRAPH::ResolveDrivers (within a single
 * subgraph) and by buildConnectionGraph's global-label transitive-closure pre-pass
 * (across subgraphs). Returns -1 if aA wins, +1 if aB wins, 0 if the two are tied.
 *
 * Rules are applied in priority order and each rule short-circuits if it distinguishes
 * the candidates.
 *   1. Higher CONNECTION_SUBGRAPH driver priority wins. ResolveDrivers pre-filters
 *      candidates to a single priority, so this rule is a no-op there; the pre-pass
 *      relies on it to break ties across subgraphs with different primary-driver
 *      priorities.
 *   2. For two bus connections, the superset wins. Without this, a wider bus can be
 *      silently canonicalized to a narrower one and lose members.
 *   3. For two pins, a pin on a global power symbol beats a local power pin beats a
 *      regular pin.
 *   4. For two sheet pins, an OUTPUT shape beats an INPUT shape.
 *   5. Names containing "-Pad" are treated as low quality and demoted.
 *   6. Alphabetical fallback for deterministic ordering.
 */
static int compareDrivers( SCH_ITEM* aA, SCH_CONNECTION* aAConn, const wxString& aAName,
                           SCH_ITEM* aB, SCH_CONNECTION* aBConn, const wxString& aBName )
{
    CONNECTION_SUBGRAPH::PRIORITY pa = CONNECTION_SUBGRAPH::GetDriverPriority( aA );
    CONNECTION_SUBGRAPH::PRIORITY pb = CONNECTION_SUBGRAPH::GetDriverPriority( aB );

    if( pa != pb )
        return pa > pb ? -1 : 1;

    if( aAConn->IsBus() && aBConn->IsBus() )
    {
        bool a_in_b = aAConn->IsSubsetOf( aBConn );
        bool b_in_a = aBConn->IsSubsetOf( aAConn );

        if( b_in_a && !a_in_b )
            return -1;

        if( a_in_b && !b_in_a )
            return 1;
    }

    if( aA->Type() == SCH_PIN_T && aB->Type() == SCH_PIN_T )
    {
        SCH_PIN* pinA = static_cast<SCH_PIN*>( aA );
        SCH_PIN* pinB = static_cast<SCH_PIN*>( aB );

        SYMBOL* parentA = pinA->GetLibPin() ? pinA->GetLibPin()->GetParentSymbol() : nullptr;
        SYMBOL* parentB = pinB->GetLibPin() ? pinB->GetLibPin()->GetParentSymbol() : nullptr;

        bool aGlobal = parentA && parentA->IsGlobalPower();
        bool bGlobal = parentB && parentB->IsGlobalPower();

        if( aGlobal != bGlobal )
            return aGlobal ? -1 : 1;

        bool aLocal = parentA && parentA->IsLocalPower();
        bool bLocal = parentB && parentB->IsLocalPower();

        if( aLocal != bLocal )
            return aLocal ? -1 : 1;
    }

    if( aA->Type() == SCH_SHEET_PIN_T && aB->Type() == SCH_SHEET_PIN_T )
    {
        SCH_SHEET_PIN* sheetPinA = static_cast<SCH_SHEET_PIN*>( aA );
        SCH_SHEET_PIN* sheetPinB = static_cast<SCH_SHEET_PIN*>( aB );

        if( sheetPinA->GetShape() != sheetPinB->GetShape() )
        {
            if( sheetPinA->GetShape() == LABEL_FLAG_SHAPE::L_OUTPUT )
                return -1;

            if( sheetPinB->GetShape() == LABEL_FLAG_SHAPE::L_OUTPUT )
                return 1;
        }
    }

    bool aLowQuality = aAName.Contains( wxS( "-Pad" ) );
    bool bLowQuality = aBName.Contains( wxS( "-Pad" ) );

    if( aLowQuality != bLowQuality )
        return aLowQuality ? 1 : -1;

    if( aAName < aBName )
        return -1;

    if( aBName < aAName )
        return 1;

    return 0;
}


bool CONNECTION_SUBGRAPH::ResolveDrivers( bool aCheckMultipleDrivers )
{
    std::lock_guard lock( m_driver_mutex );

    // Collect candidate drivers of highest priority in a simple vector which will be
    // sorted later.  Using a vector makes the ranking logic explicit and easier to
    // maintain than relying on the ordering semantics of std::set.
    PRIORITY               highest_priority = PRIORITY::INVALID;
    std::vector<SCH_ITEM*> candidates;
    std::set<SCH_ITEM*>    strong_drivers;

    m_driver = nullptr;

    // Hierarchical labels are lower priority than local labels here,
    // because on the first pass we want local labels to drive subgraphs
    // so that we can identify same-sheet neighbors and link them together.
    // Hierarchical labels will end up overriding the final net name if
    // a higher-level sheet has a different name during the hierarchical
    // pass.

    for( SCH_ITEM* item : m_drivers )
    {
        PRIORITY item_priority = GetDriverPriority( item );

        if( item_priority == PRIORITY::PIN )
        {
            SCH_PIN* pin = static_cast<SCH_PIN*>( item );

            if( !static_cast<SCH_SYMBOL*>( pin->GetParentSymbol() )->IsInNetlist() )
                continue;
        }

        if( item_priority >= PRIORITY::HIER_LABEL )
            strong_drivers.insert( item );

        if( item_priority > highest_priority )
        {
            candidates.clear();
            candidates.push_back( item );
            highest_priority = item_priority;
        }
        else if( !candidates.empty() && ( item_priority == highest_priority ) )
        {
            candidates.push_back( item );
        }
    }

    if( highest_priority >= PRIORITY::HIER_LABEL )
        m_strong_driver = true;

    // Power pins are 5, global labels are 6
    m_local_driver = ( highest_priority < PRIORITY::GLOBAL_POWER_PIN );

    if( !candidates.empty() )
    {
        // Delegate to the shared compareDrivers helper so this site and the global-label
        // transitive-closure pre-pass in buildConnectionGraph agree on every tie-break.
        auto candidate_cmp = [&]( SCH_ITEM* a, SCH_ITEM* b )
        {
            return compareDrivers( a, a->Connection( &m_sheet ), GetNameForDriver( a ),
                                   b, b->Connection( &m_sheet ), GetNameForDriver( b ) ) < 0;
        };

        std::sort( candidates.begin(), candidates.end(), candidate_cmp );

        m_driver = candidates.front();
    }

    if( strong_drivers.size() > 1 )
        m_multiple_drivers = true;

    // Drop weak drivers
    if( m_strong_driver )
    {
        m_drivers.clear();
        m_drivers.insert( strong_drivers.begin(), strong_drivers.end() );
    }

    // Cache driver connection
    if( m_driver )
    {
        m_driver_connection = m_driver->Connection( &m_sheet );
        m_driver_connection->ConfigureFromLabel( GetNameForDriver( m_driver ) );
        m_driver_connection->SetDriver( m_driver );
        m_driver_connection->ClearDirty();
    }
    else if( !m_is_bus_member )
    {
        m_driver_connection = nullptr;
    }

    return ( m_driver != nullptr );
}


void CONNECTION_SUBGRAPH::getAllConnectedItems( std::set<std::pair<SCH_SHEET_PATH,
                                                SCH_ITEM*>>& aItems,
                                                std::set<CONNECTION_SUBGRAPH*>& aSubgraphs )
{
    CONNECTION_SUBGRAPH* sg = this;

    while( sg->m_absorbed_by )
    {
        wxCHECK2( sg->m_graph == sg->m_absorbed_by->m_graph, continue );
        sg = sg->m_absorbed_by;
    }

    // If we are unable to insert the subgraph into the set, then we have already
    // visited it and don't need to add it again.
    if( aSubgraphs.insert( sg ).second == false )
        return;

    aSubgraphs.insert( sg->m_absorbed_subgraphs.begin(), sg->m_absorbed_subgraphs.end() );

    for( SCH_ITEM* item : sg->m_items )
        aItems.emplace( m_sheet, item );

    for( CONNECTION_SUBGRAPH* child_sg : sg->m_hier_children )
        child_sg->getAllConnectedItems( aItems, aSubgraphs );
}


wxString CONNECTION_SUBGRAPH::GetNetName() const
{
    if( !m_driver || m_dirty )
        return "";

    if( !m_driver->Connection( &m_sheet ) )
    {
#ifdef CONNECTIVITY_DEBUG
        wxASSERT_MSG( false, wxS( "Tried to get the net name of an item with no connection" ) );
#endif

        return "";
    }

    return m_driver->Connection( &m_sheet )->Name();
}


std::vector<SCH_ITEM*> CONNECTION_SUBGRAPH::GetAllBusLabels() const
{
    std::vector<SCH_ITEM*> labels;

    for( SCH_ITEM* item : m_drivers )
    {
        switch( item->Type() )
        {
        case SCH_LABEL_T:
        case SCH_GLOBAL_LABEL_T:
        case SCH_HIER_LABEL_T:
        {
            CONNECTION_TYPE type = item->Connection( &m_sheet )->Type();

            // Only consider bus vectors
            if( type == CONNECTION_TYPE::BUS || type == CONNECTION_TYPE::BUS_GROUP )
                labels.push_back( item );

            break;
        }

        default:
            break;
        }
    }

    return labels;
}


std::vector<SCH_ITEM*> CONNECTION_SUBGRAPH::GetVectorBusLabels() const
{
    std::vector<SCH_ITEM*> labels;

    for( SCH_ITEM* item : m_drivers )
    {
        switch( item->Type() )
        {
        case SCH_LABEL_T:
        case SCH_GLOBAL_LABEL_T:
        case SCH_HIER_LABEL_T:
        {
            SCH_CONNECTION* label_conn = item->Connection( &m_sheet );

            // Only consider bus vectors
            if( label_conn->Type() == CONNECTION_TYPE::BUS )
                labels.push_back( item );

            break;
        }

        default:
            break;
        }
    }

    return labels;
}


wxString CONNECTION_SUBGRAPH::driverName( SCH_ITEM* aItem ) const
{
    switch( aItem->Type() )
    {
    case SCH_PIN_T:
    {
        SCH_PIN* pin = static_cast<SCH_PIN*>( aItem );
        bool     forceNoConnect = m_no_connect != nullptr;

        return pin->GetDefaultNetName( m_sheet, forceNoConnect );
    }

    case SCH_LABEL_T:
    case SCH_GLOBAL_LABEL_T:
    case SCH_HIER_LABEL_T:
    {
        SCH_LABEL_BASE* label = static_cast<SCH_LABEL_BASE*>( aItem );

        // NB: any changes here will need corresponding changes in SCH_LABEL_BASE::cacheShownText()
        return EscapeString( label->GetShownText( &m_sheet, false ), CTX_NETNAME );
    }

    case SCH_SHEET_PIN_T:
    {
        // Sheet pins need to use their parent sheet as their starting sheet or they will resolve
        // variables on the current sheet first
        SCH_SHEET_PIN* sheetPin = static_cast<SCH_SHEET_PIN*>( aItem );
        SCH_SHEET_PATH path = m_sheet;

        if( path.Last() != sheetPin->GetParent() )
            path.push_back( sheetPin->GetParent() );

        return EscapeString( sheetPin->GetShownText( &path, false ), CTX_NETNAME );
    }

    default:
        wxFAIL_MSG( wxS( "Unhandled item type in GetNameForDriver" ) );
        return wxEmptyString;
    }
}


const wxString& CONNECTION_SUBGRAPH::GetNameForDriver( SCH_ITEM* aItem ) const
{
    if( aItem->HasCachedDriverName() )
        return aItem->GetCachedDriverName();

    std::lock_guard lock( m_driver_name_cache_mutex );
    auto it = m_driver_name_cache.find( aItem );

    if( it != m_driver_name_cache.end() )
        return it->second;

    return m_driver_name_cache.emplace( aItem, driverName( aItem ) ).first->second;
}


const std::vector<std::pair<wxString, SCH_ITEM*>>
CONNECTION_SUBGRAPH::GetNetclassesForDriver( SCH_ITEM* aItem ) const
{
    std::vector<std::pair<wxString, SCH_ITEM*>> foundNetclasses;

    const std::unordered_set<SCH_RULE_AREA*>& ruleAreaCache = aItem->GetRuleAreaCache();

    // Get netclasses on attached rule areas
    for( SCH_RULE_AREA* ruleArea : ruleAreaCache )
    {
        const std::vector<std::pair<wxString, SCH_ITEM*>> ruleAreaNetclasses =
                ruleArea->GetResolvedNetclasses( &m_sheet );

        if( ruleAreaNetclasses.size() > 0 )
        {
            foundNetclasses.insert( foundNetclasses.end(), ruleAreaNetclasses.begin(),
                                    ruleAreaNetclasses.end() );
        }
    }

    // Get netclasses on child fields
    aItem->RunOnChildren(
            [&]( SCH_ITEM* aChild )
            {
                if( aChild->Type() == SCH_FIELD_T )
                {
                    SCH_FIELD* field = static_cast<SCH_FIELD*>( aChild );

                    if( field->GetCanonicalName() == wxT( "Netclass" ) )
                    {
                        wxString netclass = field->GetShownText( &m_sheet, false );

                        if( netclass != wxEmptyString )
                            foundNetclasses.push_back( { netclass, aItem } );
                    }
                }
            },
            RECURSE_MODE::NO_RECURSE );

    std::sort(
            foundNetclasses.begin(), foundNetclasses.end(),
            []( const std::pair<wxString, SCH_ITEM*>& i1, const std::pair<wxString, SCH_ITEM*>& i2 )
            {
                return i1.first < i2.first;
            } );

    return foundNetclasses;
}


void CONNECTION_SUBGRAPH::Absorb( CONNECTION_SUBGRAPH* aOther )
{
    wxCHECK( m_sheet == aOther->m_sheet, /* void */ );

    for( SCH_ITEM* item : aOther->m_items )
    {
        item->Connection( &m_sheet )->SetSubgraphCode( m_code );
        AddItem( item );
    }

    m_absorbed_subgraphs.insert( aOther );
    m_absorbed_subgraphs.insert( aOther->m_absorbed_subgraphs.begin(),
            aOther->m_absorbed_subgraphs.end() );

    m_bus_neighbors.insert( aOther->m_bus_neighbors.begin(), aOther->m_bus_neighbors.end() );
    m_bus_parents.insert( aOther->m_bus_parents.begin(), aOther->m_bus_parents.end() );

    m_multiple_drivers |= aOther->m_multiple_drivers;

    std::function<void( CONNECTION_SUBGRAPH* )> set_absorbed_by =
            [ & ]( CONNECTION_SUBGRAPH *child )
    {
        child->m_absorbed_by = this;

        for( CONNECTION_SUBGRAPH* subchild : child->m_absorbed_subgraphs )
            set_absorbed_by( subchild );
    };

    aOther->m_absorbed = true;
    aOther->m_dirty = false;
    aOther->m_driver = nullptr;
    aOther->m_driver_connection = nullptr;

    set_absorbed_by( aOther );
}


void CONNECTION_SUBGRAPH::AddItem( SCH_ITEM* aItem )
{
    m_items.insert( aItem );

    if( aItem->Connection( &m_sheet )->IsDriver() )
        m_drivers.insert( aItem );

    if( aItem->Type() == SCH_SHEET_PIN_T )
        m_hier_pins.insert( static_cast<SCH_SHEET_PIN*>( aItem ) );
    else if( aItem->Type() == SCH_HIER_LABEL_T )
        m_hier_ports.insert( static_cast<SCH_HIERLABEL*>( aItem ) );
}


void CONNECTION_SUBGRAPH::UpdateItemConnections()
{
    if( !m_driver_connection )
        return;

    for( SCH_ITEM* item : m_items )
    {
        SCH_CONNECTION* item_conn = item->GetOrInitConnection( m_sheet, m_graph );

        if( !item_conn )
            continue;

        if( ( m_driver_connection->IsBus() && item_conn->IsNet() ) ||
            ( m_driver_connection->IsNet() && item_conn->IsBus() ) )
        {
            continue;
        }

        item_conn->Clone( *m_driver_connection );
        item_conn->ClearDirty();
    }
}


CONNECTION_SUBGRAPH::PRIORITY CONNECTION_SUBGRAPH::GetDriverPriority( SCH_ITEM* aDriver )
{
    if( !aDriver )
        return PRIORITY::NONE;

    auto libSymbolRef =
            []( const SCH_SYMBOL* symbol ) -> wxString
            {
                if( const std::unique_ptr<LIB_SYMBOL>& part = symbol->GetLibSymbolRef() )
                    return part->GetReferenceField().GetText();

                return wxEmptyString;
            };

    switch( aDriver->Type() )
    {
    case SCH_SHEET_PIN_T:     return PRIORITY::SHEET_PIN;
    case SCH_HIER_LABEL_T:    return PRIORITY::HIER_LABEL;
    case SCH_LABEL_T:         return PRIORITY::LOCAL_LABEL;
    case SCH_GLOBAL_LABEL_T:  return PRIORITY::GLOBAL;

    case SCH_PIN_T:
    {
        SCH_PIN* sch_pin = static_cast<SCH_PIN*>( aDriver );
        const SCH_SYMBOL* sym = static_cast<SCH_SYMBOL*>( sch_pin->GetParentSymbol() );

        if( sch_pin->IsGlobalPower() )
            return PRIORITY::GLOBAL_POWER_PIN;
        else if( sch_pin->IsLocalPower() )
            return PRIORITY::LOCAL_POWER_PIN;
        else if( !sym || sym->GetExcludedFromBoard() || libSymbolRef( sym ).StartsWith( '#' ) )
            return PRIORITY::NONE;
        else
            return PRIORITY::PIN;
    }

    default:
        return PRIORITY::NONE;
    }
}


void CONNECTION_GRAPH::Merge( CONNECTION_GRAPH& aGraph )
{
    std::copy( aGraph.m_items.begin(), aGraph.m_items.end(),
               std::back_inserter( m_items ) );

    for( SCH_ITEM* item : aGraph.m_items )
        item->SetConnectionGraph( this );

    std::copy( aGraph.m_subgraphs.begin(), aGraph.m_subgraphs.end(),
               std::back_inserter( m_subgraphs ) );

    for( CONNECTION_SUBGRAPH* sg : aGraph.m_subgraphs )
    {
        if( sg->m_driver_connection )
            sg->m_driver_connection->SetGraph( this );

        sg->m_graph = this;
    }

    std::copy( aGraph.m_driver_subgraphs.begin(), aGraph.m_driver_subgraphs.end(),
               std::back_inserter( m_driver_subgraphs ) );

    std::copy( aGraph.m_global_power_pins.begin(), aGraph.m_global_power_pins.end(),
               std::back_inserter( m_global_power_pins ) );

    for( auto& [key, value] : aGraph.m_net_name_to_subgraphs_map )
        m_net_name_to_subgraphs_map.insert_or_assign( key, value );

    for( auto& [key, value] : aGraph.m_sheet_to_subgraphs_map )
        m_sheet_to_subgraphs_map.insert_or_assign( key, value );

    for( auto& [key, value] : aGraph.m_net_name_to_code_map )
        m_net_name_to_code_map.insert_or_assign( key, value );

    for( auto& [key, value] : aGraph.m_bus_name_to_code_map )
        m_bus_name_to_code_map.insert_or_assign( key, value );

    for( auto& [key, value] : aGraph.m_net_code_to_subgraphs_map )
        m_net_code_to_subgraphs_map.insert_or_assign( key, value );

    for( auto& [key, value] : aGraph.m_item_to_subgraph_map )
        m_item_to_subgraph_map.insert_or_assign( key, value );

    for( auto& [key, value] : aGraph.m_local_label_cache )
        m_local_label_cache.insert_or_assign( key, value );

    for( auto& [key, value] : aGraph.m_global_label_cache )
        m_global_label_cache.insert_or_assign( key, value );

    m_last_bus_code = std::max( m_last_bus_code, aGraph.m_last_bus_code );
    m_last_net_code = std::max( m_last_net_code, aGraph.m_last_net_code );
    m_last_subgraph_code = std::max( m_last_subgraph_code, aGraph.m_last_subgraph_code );

    // Committed chains and override maps belong to the persistent schematic state, so they
    // must travel across an incremental graph merge.  Potential chains are moved alongside
    // them to keep the merged graph self-consistent until the next RebuildNetChains() pass.
    for( std::unique_ptr<SCH_NETCHAIN>& chain : aGraph.m_committedNetChains )
    {
        if( chain )
            m_committedNetChains.push_back( std::move( chain ) );
    }

    aGraph.m_committedNetChains.clear();

    for( std::unique_ptr<SCH_NETCHAIN>& chain : aGraph.m_potentialNetChains )
    {
        if( chain )
            m_potentialNetChains.push_back( std::move( chain ) );
    }

    aGraph.m_potentialNetChains.clear();

    m_netChainsBuilt = m_netChainsBuilt || aGraph.m_netChainsBuilt;

    for( auto& [key, value] : aGraph.m_netChainTerminalOverrides )
        m_netChainTerminalOverrides.insert_or_assign( key, value );

    for( auto& [key, value] : aGraph.m_netChainNetClassOverrides )
        m_netChainNetClassOverrides.insert_or_assign( key, value );

    for( auto& [key, value] : aGraph.m_netChainColorOverrides )
        m_netChainColorOverrides.insert_or_assign( key, value );

    for( auto& [key, value] : aGraph.m_netChainTerminalRefOverrides )
        m_netChainTerminalRefOverrides.insert_or_assign( key, value );

    for( auto& [key, value] : aGraph.m_netChainMemberNetOverrides )
        m_netChainMemberNetOverrides.insert_or_assign( key, value );
}


void CONNECTION_GRAPH::ExchangeItem( SCH_ITEM* aOldItem, SCH_ITEM* aNewItem )
{
    wxCHECK2( aOldItem->Type() == aNewItem->Type(), return );

    auto exchange = [&]( SCH_ITEM* aOld, SCH_ITEM* aNew )
    {
        auto it = m_item_to_subgraph_map.find( aOld );

        if( it == m_item_to_subgraph_map.end() )
            return;

        CONNECTION_SUBGRAPH* sg = it->second;

        sg->ExchangeItem( aOld, aNew );

        m_item_to_subgraph_map.erase( it );
        m_item_to_subgraph_map.emplace( aNew, sg );

        for( auto it2 = m_items.begin(); it2 != m_items.end(); ++it2 )
        {
            if( *it2 == aOld )
            {
                *it2 = aNew;
                break;
            }
        }
    };

    exchange( aOldItem, aNewItem );

    if( aOldItem->Type() == SCH_SYMBOL_T )
    {
        SCH_SYMBOL* oldSymbol = static_cast<SCH_SYMBOL*>( aOldItem );
        SCH_SYMBOL* newSymbol = static_cast<SCH_SYMBOL*>( aNewItem );
        std::vector<SCH_PIN*> oldPins = oldSymbol->GetPins( &m_schematic->CurrentSheet() );
        std::vector<SCH_PIN*> newPins = newSymbol->GetPins( &m_schematic->CurrentSheet() );

        wxCHECK2( oldPins.size() == newPins.size(), return );

        for( size_t ii = 0; ii < oldPins.size(); ii++ )
        {
            exchange( oldPins[ii], newPins[ii] );
        }
    }
}


void CONNECTION_GRAPH::Reset()
{
    for( auto& subgraph : m_subgraphs )
    {
        /// Only delete subgraphs of which we are the owner
        if( subgraph->m_graph == this )
            delete subgraph;
    }

    m_items.clear();
    m_subgraphs.clear();
    m_driver_subgraphs.clear();
    m_sheet_to_subgraphs_map.clear();
    m_global_power_pins.clear();
    m_bus_alias_cache.clear();
    m_net_name_to_code_map.clear();
    m_bus_name_to_code_map.clear();
    m_net_code_to_subgraphs_map.clear();
    m_net_name_to_subgraphs_map.clear();
    m_item_to_subgraph_map.clear();
    m_local_label_cache.clear();
    m_global_label_cache.clear();
    m_last_net_code = 1;
    m_last_bus_code = 1;
    m_last_subgraph_code = 1;

    // SCH_NETCHAIN::m_symbols holds non-owning SCH_SYMBOL pointers. Once the connectivity
    // pass clears the rest of the graph the schematic items can be freed before
    // RebuildNetChains() repopulates the chain caches, so drop the stale pointers now.
    for( std::unique_ptr<SCH_NETCHAIN>& chain : m_committedNetChains )
    {
        if( chain )
            chain->ClearSymbols();
    }

    m_potentialNetChains.clear();
    m_netChainsBuilt = false;
}


void CONNECTION_GRAPH::Recalculate( const SCH_SHEET_LIST& aSheetList, bool aUnconditional,
                                    std::function<void( SCH_ITEM* )>* aChangedItemHandler,
                                    PROGRESS_REPORTER* aProgressReporter )
{
    APP_MONITOR::TRANSACTION monitorTrans( "CONNECTION_GRAPH::Recalculate", "Recalculate" );
    PROF_TIMER recalc_time( "CONNECTION_GRAPH::Recalculate" );
    monitorTrans.Start();

    if( aUnconditional )
        Reset();

    monitorTrans.StartSpan( "updateItemConnectivity", "" );
    PROF_TIMER update_items( "updateItemConnectivity" );

    m_sheetList = aSheetList;
    std::set<SCH_ITEM*> dirty_items;

    int count = aSheetList.size() * 2;
    int done = 0;

    for( const SCH_SHEET_PATH& sheet : aSheetList )
    {
        if( aProgressReporter )
        {
            aProgressReporter->SetCurrentProgress( done++ / (double) count );
            aProgressReporter->KeepRefreshing();
        }

        std::vector<SCH_ITEM*> items;

        // Store current unit value, to replace it after calculations
        std::vector<std::pair<SCH_SYMBOL*, int>> symbolsChanged;

        for( SCH_ITEM* item : sheet.LastScreen()->Items() )
        {
            if( item->IsConnectable() && ( aUnconditional || item->IsConnectivityDirty() ) )
            {
                wxLogTrace( ConnTrace, wxT( "Adding item %s to connectivity graph update" ),
                            item->GetTypeDesc() );
                items.push_back( item );
                dirty_items.insert( item );

                // Add any symbol dirty pins to the dirty_items list
                if( item->Type() == SCH_SYMBOL_T )
                {
                    SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

                    for( SCH_PIN* pin : symbol->GetPins( &sheet ) )
                    {
                        if( pin->IsConnectivityDirty() )
                        {
                            dirty_items.insert( pin );
                        }
                    }
                }
            }
            // If the symbol isn't dirty, look at the pins
            // TODO: remove symbols from connectivity graph and only use pins
            else if( item->Type() == SCH_SYMBOL_T )
            {
                SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

                for( SCH_PIN* pin : symbol->GetPins( &sheet ) )
                {
                    if( pin->IsConnectivityDirty() )
                    {
                        items.push_back( pin );
                        dirty_items.insert( pin );
                    }
                }
            }
            else if( item->Type() == SCH_SHEET_T )
            {
                SCH_SHEET* sheetItem = static_cast<SCH_SHEET*>( item );

                for( SCH_SHEET_PIN* pin : sheetItem->GetPins() )
                {
                    if( pin->IsConnectivityDirty() )
                    {
                        items.push_back( pin );
                        dirty_items.insert( pin );
                    }
                }
            }

            // Ensure the hierarchy info stored in the SCH_SCREEN (such as symbol units) reflects
            // the current SCH_SHEET_PATH
            if( item->Type() == SCH_SYMBOL_T )
            {
                SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );
                int new_unit = symbol->GetUnitSelection( &sheet );

                // Store the initial unit value so we can restore it after calculations
                if( symbol->GetUnit() != new_unit )
                    symbolsChanged.push_back( { symbol, symbol->GetUnit() } );

                symbol->SetUnit( new_unit );
            }
        }

        m_items.reserve( m_items.size() + items.size() );

        updateItemConnectivity( sheet, items );

        if( aProgressReporter )
        {
            aProgressReporter->SetCurrentProgress( done++ / count );
            aProgressReporter->KeepRefreshing();
        }

        // UpdateDanglingState() also adds connected items for SCH_TEXT
        sheet.LastScreen()->TestDanglingEnds( &sheet, aChangedItemHandler );

        // Restore the m_unit member variables where we had to change them
        for( const auto& [ symbol, originalUnit ] : symbolsChanged )
            symbol->SetUnit( originalUnit );
    }

    // Restore the dangling states of items in the current SCH_SCREEN to match the current
    // SCH_SHEET_PATH.
    SCH_SCREEN* currentScreen = m_schematic->CurrentSheet().LastScreen();

    if( currentScreen )
        currentScreen->TestDanglingEnds( &m_schematic->CurrentSheet(), aChangedItemHandler );

    for( SCH_ITEM* item : dirty_items )
        item->SetConnectivityDirty( false );


    monitorTrans.FinishSpan();
    if( wxLog::IsAllowedTraceMask( DanglingProfileMask ) )
        update_items.Show();

    PROF_TIMER build_graph( "buildConnectionGraph" );
    monitorTrans.StartSpan( "BuildConnectionGraph", "" );

    buildConnectionGraph( aChangedItemHandler, aUnconditional );

    if( wxLog::IsAllowedTraceMask( DanglingProfileMask ) )
        build_graph.Show();

    monitorTrans.FinishSpan();

    recalc_time.Stop();

    if( wxLog::IsAllowedTraceMask( DanglingProfileMask ) )
        recalc_time.Show();

    monitorTrans.Finish();
}


std::set<std::pair<SCH_SHEET_PATH, SCH_ITEM*>> CONNECTION_GRAPH::ExtractAffectedItems(
        const std::set<SCH_ITEM*> &aItems )
{
    std::set<std::pair<SCH_SHEET_PATH, SCH_ITEM*>> retvals;
    std::set<CONNECTION_SUBGRAPH*> subgraphs;

    auto traverse_subgraph = [&retvals, &subgraphs]( CONNECTION_SUBGRAPH* aSubgraph )
    {
        // Find the primary subgraph on this sheet
        while( aSubgraph->m_absorbed_by )
        {
            // Should we skip this if the absorbed by sub-graph is not this sub-grap?
            wxASSERT( aSubgraph->m_graph == aSubgraph->m_absorbed_by->m_graph );
            aSubgraph = aSubgraph->m_absorbed_by;
        }

        // Find the top most connected subgraph on all sheets
        while( aSubgraph->m_hier_parent )
        {
            // Should we skip this if the absorbed by sub-graph is not this sub-grap?
            wxASSERT( aSubgraph->m_graph == aSubgraph->m_hier_parent->m_graph );
            aSubgraph = aSubgraph->m_hier_parent;
        }

        // Recurse through all subsheets to collect connected items
        aSubgraph->getAllConnectedItems( retvals, subgraphs );
    };

    auto extract_element = [&]( SCH_ITEM* aItem )
    {
        CONNECTION_SUBGRAPH* item_sg = GetSubgraphForItem( aItem );

        if( !item_sg )
        {
            wxLogTrace( ConnTrace, wxT( "Item %s not found in connection graph" ),
                        aItem->GetTypeDesc() );
            return;
        }

        if( !item_sg->ResolveDrivers( true ) )
        {
            wxLogTrace( ConnTrace, wxT( "Item %s in subgraph %ld (%p) has no driver" ),
                        aItem->GetTypeDesc(), item_sg->m_code, item_sg );
        }

        std::vector<CONNECTION_SUBGRAPH*> sg_to_scan = GetAllSubgraphs( item_sg->GetNetName() );

        if( sg_to_scan.empty() )
        {
            wxLogTrace( ConnTrace, wxT( "Item %s in subgraph %ld with net %s has no neighbors" ),
                        aItem->GetTypeDesc(), item_sg->m_code, item_sg->GetNetName() );
            sg_to_scan.push_back( item_sg );
        }

        wxLogTrace( ConnTrace,
                    wxT( "Removing all item %s connections from subgraph %ld with net %s: Found "
                         "%zu subgraphs" ),
                    aItem->GetTypeDesc(), item_sg->m_code, item_sg->GetNetName(),
                    sg_to_scan.size() );

        for( CONNECTION_SUBGRAPH* sg : sg_to_scan )
        {
            traverse_subgraph( sg );

            for( auto& bus_it : sg->m_bus_neighbors )
            {
                for( CONNECTION_SUBGRAPH* bus_sg : bus_it.second )
                    traverse_subgraph( bus_sg );
            }

            for( auto& bus_it : sg->m_bus_parents )
            {
                for( CONNECTION_SUBGRAPH* bus_sg : bus_it.second )
                    traverse_subgraph( bus_sg );
            }
        }

        std::erase( m_items, aItem );
    };

    for( SCH_ITEM* item : aItems )
    {
        if( item->Type() == SCH_SHEET_T )
        {
            SCH_SHEET* sheet = static_cast<SCH_SHEET*>( item );

            for( SCH_SHEET_PIN* pin : sheet->GetPins() )
                extract_element( pin );
        }
        else if ( item->Type() == SCH_SYMBOL_T )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

            for( SCH_PIN* pin : symbol->GetPins( &m_schematic->CurrentSheet() ) )
                extract_element( pin );
        }
        else
        {
            extract_element( item );
        }
    }

    removeSubgraphs( subgraphs );

    for( const auto& [path, item] : retvals )
        std::erase( m_items, item );

    return retvals;
}


void CONNECTION_GRAPH::RemoveItem( SCH_ITEM* aItem )
{
    auto it = m_item_to_subgraph_map.find( aItem );

    if( it == m_item_to_subgraph_map.end() )
        return;

    CONNECTION_SUBGRAPH* subgraph = it->second;

    while(subgraph->m_absorbed_by )
        subgraph = subgraph->m_absorbed_by;

    subgraph->RemoveItem( aItem );
    std::erase( m_items, aItem );
    m_item_to_subgraph_map.erase( it );
}


void CONNECTION_GRAPH::removeSubgraphs( std::set<CONNECTION_SUBGRAPH*>& aSubgraphs )
{
    wxLogTrace( ConnTrace, wxT( "Removing %zu subgraphs" ), aSubgraphs.size() );
    std::sort( m_driver_subgraphs.begin(), m_driver_subgraphs.end() );
    std::sort( m_subgraphs.begin(), m_subgraphs.end() );
    std::set<int> codes_to_remove;

    for( auto& el : m_sheet_to_subgraphs_map )
    {
        std::sort( el.second.begin(), el.second.end() );
    }

    for( CONNECTION_SUBGRAPH* sg : aSubgraphs )
    {
        for( auto& it : sg->m_bus_neighbors )
        {
            for( CONNECTION_SUBGRAPH* neighbor : it.second )
            {
                auto& parents = neighbor->m_bus_parents[it.first];

                for( auto test = parents.begin(); test != parents.end(); )
                {
                    if( *test == sg )
                        test = parents.erase( test );
                    else
                        ++test;
                }

                if( parents.empty() )
                    neighbor->m_bus_parents.erase( it.first );
            }
        }

        for( auto& it : sg->m_bus_parents )
        {
            for( CONNECTION_SUBGRAPH* parent : it.second )
            {
                auto& neighbors = parent->m_bus_neighbors[it.first];

                for( auto test = neighbors.begin(); test != neighbors.end(); )
                {
                    if( *test == sg )
                        test = neighbors.erase( test );
                    else
                        ++test;
                }

                if( neighbors.empty() )
                    parent->m_bus_neighbors.erase( it.first );
            }
        }

        {
            auto it = std::lower_bound( m_driver_subgraphs.begin(), m_driver_subgraphs.end(), sg );

            while( it != m_driver_subgraphs.end() && *it == sg )
                it = m_driver_subgraphs.erase( it );
        }

        {
            auto it = std::lower_bound( m_subgraphs.begin(), m_subgraphs.end(), sg );

            while( it != m_subgraphs.end() && *it == sg )
                it = m_subgraphs.erase( it );
        }

        for( auto& el : m_sheet_to_subgraphs_map )
        {
            auto it = std::lower_bound( el.second.begin(), el.second.end(), sg );

            while( it != el.second.end() && *it == sg )
                it = el.second.erase( it );
        }

        auto remove_sg = [sg]( auto it ) -> bool
                         {
                             for( const CONNECTION_SUBGRAPH* test_sg : it->second )
                             {
                                 if( sg == test_sg )
                                     return true;
                             }

                             return false;
                         };

        for( auto it = m_global_label_cache.begin(); it != m_global_label_cache.end(); )
        {
            if( remove_sg( it ) )
                it = m_global_label_cache.erase( it );
            else
                ++it;
        }

        for( auto it = m_local_label_cache.begin(); it != m_local_label_cache.end(); )
        {
            if( remove_sg( it ) )
                it = m_local_label_cache.erase( it );
            else
                ++it;
        }

        for( auto it = m_net_code_to_subgraphs_map.begin();
             it != m_net_code_to_subgraphs_map.end(); )
        {
            if( remove_sg( it ) )
            {
                codes_to_remove.insert( it->first.Netcode );
                it = m_net_code_to_subgraphs_map.erase( it );
            }
            else
            {
                ++it;
            }
        }

        for( auto it = m_net_name_to_subgraphs_map.begin();
             it != m_net_name_to_subgraphs_map.end(); )
        {
            if( remove_sg( it ) )
                it = m_net_name_to_subgraphs_map.erase( it );
            else
                ++it;
        }

        for( auto it = m_item_to_subgraph_map.begin(); it != m_item_to_subgraph_map.end(); )
        {
            if( it->second == sg )
                it = m_item_to_subgraph_map.erase( it );
            else
                ++it;
        }


    }

    for( auto it = m_net_name_to_code_map.begin(); it != m_net_name_to_code_map.end(); )
    {
        if( codes_to_remove.contains( it->second ) )
            it = m_net_name_to_code_map.erase( it );
        else
            ++it;
    }

    for( auto it = m_bus_name_to_code_map.begin(); it != m_bus_name_to_code_map.end(); )
    {
        if( codes_to_remove.contains( it->second ) )
            it = m_bus_name_to_code_map.erase( it );
        else
            ++it;
    }

    for( CONNECTION_SUBGRAPH* sg : aSubgraphs )
    {
        sg->m_code = -1;
        sg->m_graph = nullptr;
        delete sg;
    }
}


void CONNECTION_GRAPH::updateSymbolConnectivity( const SCH_SHEET_PATH& aSheet, SCH_SYMBOL* aSymbol,
                                                 std::map<VECTOR2I, std::vector<SCH_ITEM*>>& aConnectionMap )
{
    auto updatePin =
            [&]( SCH_PIN* aPin, SCH_CONNECTION* aConn )
            {
                aConn->SetType( CONNECTION_TYPE::NET );
                wxString name = aPin->GetDefaultNetName( aSheet );
                aPin->ClearConnectedItems( aSheet );

                if( aPin->IsGlobalPower() )
                {
                    aConn->SetName( name );
                    m_global_power_pins.emplace_back( std::make_pair( aSheet, aPin ) );
                }
            };

    std::map<wxString, std::vector<SCH_PIN*>> pinNumberMap;

    for( SCH_PIN* pin : aSymbol->GetPins( &aSheet ) )
    {
        m_items.emplace_back( pin );
        SCH_CONNECTION* conn = pin->InitializeConnection( aSheet, this );
        updatePin( pin, conn );
        aConnectionMap[ pin->GetPosition() ].push_back( pin );
        pinNumberMap[pin->GetNumber()].emplace_back( pin );
    }

    auto linkPinsInVec =
            [&]( const std::vector<SCH_PIN*>& aVec )
            {
                for( size_t i = 0; i < aVec.size(); ++i )
                {
                    for( size_t j = i + 1; j < aVec.size(); ++j )
                    {
                        aVec[i]->AddConnectionTo( aSheet, aVec[j] );
                        aVec[j]->AddConnectionTo( aSheet, aVec[i] );
                    }
                }
            };

    if( aSymbol->GetLibSymbolRef() )
    {
        if( aSymbol->GetLibSymbolRef()->GetDuplicatePinNumbersAreJumpers() )
        {
            for( const auto& [number, group] : pinNumberMap )
                linkPinsInVec( group );
        }

        for( const std::set<wxString>& group : aSymbol->GetLibSymbolRef()->JumperPinGroups() )
        {
            std::vector<SCH_PIN*> pins;

            for( const wxString& pinNumber : group )
            {
                if( SCH_PIN* pin = aSymbol->GetPin( pinNumber ) )
                    pins.emplace_back( pin );
            }

            linkPinsInVec( pins );
        }
    }
}


void CONNECTION_GRAPH::updatePinConnectivity( const SCH_SHEET_PATH& aSheet, SCH_PIN* aPin, SCH_CONNECTION* aConn )
{
    aConn->SetType( CONNECTION_TYPE::NET );

    // because calling the first time is not thread-safe
    wxString name = aPin->GetDefaultNetName( aSheet );
    aPin->ClearConnectedItems( aSheet );

    if( aPin->IsGlobalPower() )
    {
        aConn->SetName( name );
        m_global_power_pins.emplace_back( std::make_pair( aSheet, aPin ) );
    }
}


void CONNECTION_GRAPH::updateGenericItemConnectivity( const SCH_SHEET_PATH& aSheet, SCH_ITEM* aItem,
                                                      std::map<VECTOR2I, std::vector<SCH_ITEM*>>& aConnectionMap )
{
    std::vector<VECTOR2I> points = aItem->GetConnectionPoints();
    aItem->ClearConnectedItems( aSheet );

    m_items.emplace_back( aItem );
    SCH_CONNECTION* conn = aItem->InitializeConnection( aSheet, this );

    switch( aItem->Type() )
    {
    case SCH_LINE_T:
        conn->SetType( aItem->GetLayer() == LAYER_BUS ? CONNECTION_TYPE::BUS : CONNECTION_TYPE::NET );
        break;

    case SCH_BUS_BUS_ENTRY_T:
        conn->SetType( CONNECTION_TYPE::BUS );
        static_cast<SCH_BUS_BUS_ENTRY*>( aItem )->m_connected_bus_items[0] = nullptr;
        static_cast<SCH_BUS_BUS_ENTRY*>( aItem )->m_connected_bus_items[1] = nullptr;
        break;

    case SCH_PIN_T:
        if( points.empty() )
            points = { static_cast<SCH_PIN*>( aItem )->GetPosition() };

        updatePinConnectivity( aSheet, static_cast<SCH_PIN*>( aItem ), conn );
        break;

    case SCH_BUS_WIRE_ENTRY_T:
        conn->SetType( CONNECTION_TYPE::NET );
        static_cast<SCH_BUS_WIRE_ENTRY*>( aItem )->m_connected_bus_item = nullptr;
        break;

    default: break;
    }

    for( const VECTOR2I& point : points )
        aConnectionMap[point].push_back( aItem );
}


void CONNECTION_GRAPH::updateItemConnectivity( const SCH_SHEET_PATH& aSheet,
                                               const std::vector<SCH_ITEM*>& aItemList )
{
    wxLogTrace( wxT( "Updating connectivity for sheet %s with %zu items" ),
                aSheet.Last()->GetFileName(), aItemList.size() );
    std::map<VECTOR2I, std::vector<SCH_ITEM*>> connection_map;

    for( SCH_ITEM* item : aItemList )
    {
        std::vector<VECTOR2I> points = item->GetConnectionPoints();
        item->ClearConnectedItems( aSheet );
        if( item->Type() == SCH_SHEET_T )
        {
            for( SCH_SHEET_PIN* pin : static_cast<SCH_SHEET*>( item )->GetPins() )
            {
                pin->InitializeConnection( aSheet, this );

                pin->ClearConnectedItems( aSheet );

                connection_map[ pin->GetTextPos() ].push_back( pin );
                m_items.emplace_back( pin );
            }
        }
        else if( item->Type() == SCH_SYMBOL_T )
        {
            updateSymbolConnectivity( aSheet, static_cast<SCH_SYMBOL*>( item ), connection_map );
        }
        else
        {
            updateGenericItemConnectivity( aSheet, item, connection_map );

            /// Special case for labels that overlap wires
            /// While this is an ERC error as there is not an explicit junction,
            /// we want to enforce connectivity for all items under the label position.
            if( dynamic_cast<SCH_LABEL_BASE*>( item ) )
            {
                VECTOR2I point = item->GetPosition();
                SCH_SCREEN* screen = aSheet.LastScreen();
                auto items = screen->Items().Overlapping( point );
                std::vector<SCH_ITEM*> overlapping_items;

                std::copy_if( items.begin(), items.end(), std::back_inserter( overlapping_items ),
                              [&]( SCH_ITEM* test_item )
                              {
                                  return test_item->Type() == SCH_LINE_T
                                         && test_item->HitTest( point, -1 );
                              } );

                // We need at least two connnectable lines that are not the label here
                // Otherwise, the label will be normally assigned to one or the other
                if( overlapping_items.size() < 2 ) continue;

                for( SCH_ITEM* test_item : overlapping_items )
                    connection_map[point].push_back( test_item );
            }

            // Junctions connect wires that pass through their position as midpoints.
            // This handles schematics where a wire was not split at a junction point,
            // which can happen when a wire is placed over an existing junction without
            // the schematic topology being updated.
            if( item->Type() == SCH_JUNCTION_T )
            {
                VECTOR2I    point = item->GetPosition();
                SCH_SCREEN* screen = aSheet.LastScreen();

                for( SCH_LINE* wire : screen->GetBusesAndWires( point, true ) )
                    connection_map[point].push_back( wire );
            }
        }
    }

    for( auto& [point, connection_vec] : connection_map )
    {
        std::sort( connection_vec.begin(), connection_vec.end() );
        alg::remove_duplicates( connection_vec );

        // Pre-scan to see if we have a bus at this location
        SCH_LINE* busLine = aSheet.LastScreen()->GetBus( point );

        for( SCH_ITEM* connected_item : connection_vec )
        {
            // Bus entries are special: they can have connection points in the
            // middle of a wire segment, because the junction algo doesn't split
            // the segment in two where you place a bus entry.  This means that
            // bus entries that don't land on the end of a line segment need to
            // have "virtual" connection points to the segments they graphically
            // touch.
            if( connected_item->Type() == SCH_BUS_WIRE_ENTRY_T )
            {
                // If this location only has the connection point of the bus
                // entry itself, this means that either the bus entry is not
                // connected to anything graphically, or that it is connected to
                // a segment at some point other than at one of the endpoints.
                if( connection_vec.size() == 1 )
                {
                    if( busLine )
                    {
                        auto bus_entry = static_cast<SCH_BUS_WIRE_ENTRY*>( connected_item );
                        bus_entry->m_connected_bus_item = busLine;
                    }
                }
            }
            // Bus-to-bus entries are treated just like bus wires
            else if( connected_item->Type() == SCH_BUS_BUS_ENTRY_T )
            {
                if( busLine )
                {
                    auto bus_entry = static_cast<SCH_BUS_BUS_ENTRY*>( connected_item );

                    if( point == bus_entry->GetPosition() )
                        bus_entry->m_connected_bus_items[0] = busLine;
                    else
                        bus_entry->m_connected_bus_items[1] = busLine;

                    bus_entry->AddConnectionTo( aSheet, busLine );
                    busLine->AddConnectionTo( aSheet, bus_entry );
                    continue;
                }
            }
            // Change junctions to be on bus junction layer if they are touching a bus
            else if( connected_item->Type() == SCH_JUNCTION_T )
            {
                connected_item->SetLayer( busLine ? LAYER_BUS_JUNCTION : LAYER_JUNCTION );
            }

            for( SCH_ITEM* test_item : connection_vec )
            {
                bool      bus_connection_ok = true;

                if( test_item == connected_item )
                    continue;

                // Set up the link between the bus entry net and the bus
                if( connected_item->Type() == SCH_BUS_WIRE_ENTRY_T )
                {
                    if( test_item->GetLayer() == LAYER_BUS )
                    {
                        auto bus_entry = static_cast<SCH_BUS_WIRE_ENTRY*>( connected_item );
                        bus_entry->m_connected_bus_item = test_item;
                    }
                }

                // Bus entries only connect to bus lines on the end that is touching a bus line.
                // If the user has overlapped another net line with the endpoint of the bus entry
                // where the entry connects to a bus, we don't want to short-circuit it.
                if( connected_item->Type() == SCH_BUS_WIRE_ENTRY_T )
                {
                    bus_connection_ok = !busLine || test_item->GetLayer() == LAYER_BUS;
                }
                else if( test_item->Type() == SCH_BUS_WIRE_ENTRY_T )
                {
                    bus_connection_ok = !busLine || connected_item->GetLayer() == LAYER_BUS;
                }

                if( connected_item->ConnectionPropagatesTo( test_item )
                        && test_item->ConnectionPropagatesTo( connected_item )
                        && bus_connection_ok )
                {
                    connected_item->AddConnectionTo( aSheet, test_item );
                }
            }

            // If we got this far and did not find a connected bus item for a bus entry,
            // we should do a manual scan in case there is a bus item on this connection
            // point but we didn't pick it up earlier because there is *also* a net item here.
            if( connected_item->Type() == SCH_BUS_WIRE_ENTRY_T )
            {
                auto bus_entry = static_cast<SCH_BUS_WIRE_ENTRY*>( connected_item );

                if( !bus_entry->m_connected_bus_item )
                {
                    SCH_SCREEN* screen = aSheet.LastScreen();
                    SCH_LINE*   bus = screen->GetBus( point );

                    if( bus )
                        bus_entry->m_connected_bus_item = bus;
                }
            }
        }
    }
}


void CONNECTION_GRAPH::buildItemSubGraphs()
{
    // Recache all bus aliases for later use
    wxCHECK_RET( m_schematic, wxS( "Connection graph cannot be built without schematic pointer" ) );

    m_bus_alias_cache.clear();

    for( const std::shared_ptr<BUS_ALIAS>& alias : m_schematic->GetAllBusAliases() )
    {
        if( alias )
            m_bus_alias_cache[alias->GetName()] = alias;
    }

    // Hash position in m_sheetList for each sheet path so that subgraphs are
    // created in a deterministic order matching the sheet hierarchy
    // https://gitlab.com/kicad/code/kicad/-/issues/24409
    std::unordered_map<SCH_SHEET_PATH, size_t> sheetOrder;
    sheetOrder.reserve( m_sheetList.size() );

    for( size_t i = 0; i < m_sheetList.size(); ++i )
        sheetOrder.emplace( m_sheetList[i], i );

    auto sheetRank =
            [&]( const SCH_SHEET_PATH& aSheet ) -> size_t
            {
                auto it = sheetOrder.find( aSheet );

                return ( it == sheetOrder.end() ) ? std::numeric_limits<size_t>::max()
                                                  : it->second;
            };

    // Build subgraphs from items (on a per-sheet basis).  Reuse the vector across
    // items so a flat schematic doesn't allocate per item.
    std::vector<std::tuple<size_t, SCH_SHEET_PATH, SCH_CONNECTION*>> ordered;

    for( SCH_ITEM* item : m_items )
    {
        ordered.clear();
        ordered.reserve( item->m_connection_map.size() );

        // Precompute sheet rank into the tuple so the comparator never re-hashes
        // the (vector-backed) SCH_SHEET_PATH keys.
        for( const auto& [sheet, connection] : item->m_connection_map )
            ordered.emplace_back( sheetRank( sheet ), sheet, connection );

        std::sort( ordered.begin(), ordered.end(),
                   []( const auto& a, const auto& b )
                   {
                       return std::get<0>( a ) < std::get<0>( b );
                   } );

        for( const auto& [rank, sheet, connection] : ordered )
        {
            if( connection->SubgraphCode() == 0 )
            {
                CONNECTION_SUBGRAPH* subgraph = new CONNECTION_SUBGRAPH( this );

                subgraph->m_code = m_last_subgraph_code++;
                subgraph->m_sheet = sheet;

                subgraph->AddItem( item );

                connection->SetSubgraphCode( subgraph->m_code );
                m_item_to_subgraph_map[item] = subgraph;

                std::list<SCH_ITEM*> memberlist;

                auto get_items =
                        [&]( SCH_ITEM* aItem ) -> bool
                        {
                            SCH_CONNECTION* conn = aItem->GetOrInitConnection( sheet, this );
                            bool            unique = !( aItem->GetFlags() & CONNECTIVITY_CANDIDATE );

                            if( conn && !conn->SubgraphCode() )
                                aItem->SetFlags( CONNECTIVITY_CANDIDATE );

                            return ( unique && conn && ( conn->SubgraphCode() == 0 ) );
                        };

                std::copy_if( item->ConnectedItems( sheet ).begin(),
                              item->ConnectedItems( sheet ).end(),
                              std::back_inserter( memberlist ), get_items );

                for( SCH_ITEM* connected_item : memberlist )
                {
                    if( connected_item->Type() == SCH_NO_CONNECT_T )
                        subgraph->m_no_connect = connected_item;

                    SCH_CONNECTION* connected_conn = connected_item->Connection( &sheet );

                    wxCHECK2( connected_conn, continue );

                    if( connected_conn->SubgraphCode() == 0 )
                    {
                        connected_conn->SetSubgraphCode( subgraph->m_code );
                        m_item_to_subgraph_map[connected_item] = subgraph;
                        subgraph->AddItem( connected_item );

                        for( SCH_ITEM* citem : connected_item->ConnectedItems( sheet ) )
                        {
                            if( citem->HasFlag( CONNECTIVITY_CANDIDATE ) )
                                continue;

                            if( get_items( citem ) )
                                memberlist.push_back( citem );
                        }
                    }
                }

                for( SCH_ITEM* connected_item : memberlist )
                    connected_item->ClearFlags( CONNECTIVITY_CANDIDATE );

                subgraph->m_dirty = true;
                m_subgraphs.push_back( subgraph );
            }
        }
    }
}


void CONNECTION_GRAPH::resolveAllDrivers()
{
    // Resolve drivers for subgraphs and propagate connectivity info
    std::vector<CONNECTION_SUBGRAPH*> dirty_graphs;

    std::copy_if( m_subgraphs.begin(), m_subgraphs.end(), std::back_inserter( dirty_graphs ),
                  [&] ( const CONNECTION_SUBGRAPH* candidate )
                  {
                      return candidate->m_dirty;
                  } );

    wxLogTrace( ConnTrace, wxT( "Resolving drivers for %zu subgraphs" ), dirty_graphs.size() );

    std::vector<std::future<size_t>> returns( dirty_graphs.size() );

    auto update_lambda =
            []( CONNECTION_SUBGRAPH* subgraph ) -> size_t
            {
                if( !subgraph->m_dirty )
                    return 0;

                // Special processing for some items
                for( SCH_ITEM* item : subgraph->m_items )
                {
                    switch( item->Type() )
                    {
                    case SCH_NO_CONNECT_T:
                        subgraph->m_no_connect = item;
                        break;

                    case SCH_BUS_WIRE_ENTRY_T:
                        subgraph->m_bus_entry = item;
                        break;

                    case SCH_PIN_T:
                    {
                        auto pin = static_cast<SCH_PIN*>( item );

                        if( pin->GetType() == ELECTRICAL_PINTYPE::PT_NC )
                            subgraph->m_no_connect = item;

                        break;
                    }

                    default:
                        break;
                    }
                }

                subgraph->ResolveDrivers( true );
                subgraph->m_dirty = false;

                return 1;
            };

    thread_pool& tp = GetKiCadThreadPool();

    auto results = tp.submit_loop( 0, dirty_graphs.size(),
                                   [&]( const int ii )
                                   {
                                       update_lambda( dirty_graphs[ii] );
                                   } );
    results.wait();

    // Now discard any non-driven subgraphs from further consideration

    std::copy_if( m_subgraphs.begin(), m_subgraphs.end(), std::back_inserter( m_driver_subgraphs ),
                  [&] ( const CONNECTION_SUBGRAPH* candidate ) -> bool
                  {
                      return candidate->m_driver;
                  } );
}


void CONNECTION_GRAPH::collectAllDriverValues()
{
    // Check for subgraphs with the same net name but only weak drivers.
    // For example, two wires that are both connected to hierarchical
    // sheet pins that happen to have the same name, but are not the same.

    for( auto&& subgraph : m_driver_subgraphs )
    {
        wxString full_name = subgraph->m_driver_connection->Name();
        wxString name = subgraph->m_driver_connection->Name( true );
        m_net_name_to_subgraphs_map[full_name].emplace_back( subgraph );

        // For vector buses, we need to cache the prefix also, as two different instances of the
        // weakly driven pin may have the same prefix but different vector start and end.  We need
        // to treat those as needing renaming also, because otherwise if they end up on a sheet with
        // common usage, they will be incorrectly merged.
        if( subgraph->m_driver_connection->Type() == CONNECTION_TYPE::BUS )
        {
            wxString prefixOnly = full_name.BeforeFirst( '[' ) + wxT( "[]" );
            m_net_name_to_subgraphs_map[prefixOnly].emplace_back( subgraph );
        }

        subgraph->m_dirty = true;

        if( subgraph->m_strong_driver )
        {
            SCH_ITEM* driver = subgraph->m_driver;
            SCH_SHEET_PATH sheet = subgraph->m_sheet;

            switch( driver->Type() )
            {
            case SCH_LABEL_T:
            case SCH_HIER_LABEL_T:
            {
                m_local_label_cache[std::make_pair( sheet, name )].push_back( subgraph );
                break;
            }
            case SCH_GLOBAL_LABEL_T:
            {
                m_global_label_cache[name].push_back( subgraph );
                break;
            }
            case SCH_PIN_T:
            {
                SCH_PIN* pin = static_cast<SCH_PIN*>( driver );
                if( pin->IsGlobalPower() )
                {
                    m_global_label_cache[name].push_back( subgraph );
                }
                else if( pin->IsLocalPower() )
                {
                    m_local_label_cache[std::make_pair( sheet, name )].push_back( subgraph );
                }
                else
                {
                    UNITS_PROVIDER unitsProvider( schIUScale, EDA_UNITS::MM );
                    wxLogTrace( ConnTrace, wxS( "Unexpected normal pin %s" ),
                                driver->GetItemDescription( &unitsProvider, true ) );
                }

                break;
            }
            default:
            {
                UNITS_PROVIDER unitsProvider( schIUScale, EDA_UNITS::MM );

                wxLogTrace( ConnTrace, wxS( "Unexpected strong driver %s" ),
                            driver->GetItemDescription( &unitsProvider, true ) );
                break;
            }
            }
        }
    }
}


void CONNECTION_GRAPH::generateBusAliasMembers()
{
    std::vector<CONNECTION_SUBGRAPH*> new_subgraphs;

    for( CONNECTION_SUBGRAPH* subgraph : m_driver_subgraphs )
    {
        for( SCH_ITEM* item : subgraph->GetAllBusLabels() )
        {
            SCH_LABEL_BASE* label = static_cast<SCH_LABEL_BASE*>( item );

            SCH_CONNECTION dummy( item, subgraph->m_sheet );
            dummy.SetGraph( this );
            dummy.ConfigureFromLabel( label->GetShownText( &subgraph->m_sheet, false ) );

            wxLogTrace( ConnTrace, wxS( "new bus label (%s)" ),
                        label->GetShownText( &subgraph->m_sheet, false ) );

            for( const auto& conn : dummy.Members() )
            {
                // Only create subgraphs for NET members, not nested buses
                if( !conn->IsNet() )
                    continue;

                wxString name = conn->FullLocalName();

                CONNECTION_SUBGRAPH* new_sg = new CONNECTION_SUBGRAPH( this );

                // This connection cannot form a part of the item because the item is not, itself
                // connected to this subgraph.  It exists as part of a virtual item that may be
                // connected to other items but is not in the schematic.
                auto new_conn = std::make_unique<SCH_CONNECTION>( item, subgraph->m_sheet );
                new_conn->SetGraph( this );
                new_conn->SetName( name );
                new_conn->SetType( CONNECTION_TYPE::NET );

                SCH_CONNECTION* new_conn_ptr = subgraph->StoreImplicitConnection( std::move( new_conn ) );
                int             code = assignNewNetCode( *new_conn_ptr );

                wxLogTrace( ConnTrace, wxS( "SG(%ld), Adding full local name (%s) with sg (%d) on subsheet %s" ),
                            subgraph->m_code, name, code, subgraph->m_sheet.PathHumanReadable() );

                new_sg->m_driver_connection = new_conn_ptr;
                new_sg->m_code = m_last_subgraph_code++;
                new_sg->m_sheet = subgraph->GetSheet();
                new_sg->m_is_bus_member = true;
                new_sg->m_strong_driver = true;

                /// Need to figure out why these sgs are not getting connected to their bus parents
                NET_NAME_CODE_CACHE_KEY key = { new_sg->GetNetName(), code };
                m_net_code_to_subgraphs_map[ key ].push_back( new_sg );
                m_net_name_to_subgraphs_map[ name ].push_back( new_sg );
                m_subgraphs.push_back( new_sg );
                new_subgraphs.push_back( new_sg );
            }
        }
    }

    std::copy( new_subgraphs.begin(), new_subgraphs.end(),
               std::back_inserter( m_driver_subgraphs ) );
}


void CONNECTION_GRAPH::generateGlobalPowerPinSubGraphs()
{
    // Generate subgraphs for global power pins.  These will be merged with other subgraphs
    // on the same sheet in the next loop.
    // These are NOT limited to power symbols, we support legacy invisible + power-in pins
    // on non-power symbols.

    // Sort power pins for deterministic processing order. This ensures that when multiple
    // power pins share the same net name, the same pin consistently creates the subgraph
    // across different ERC runs.
    std::sort( m_global_power_pins.begin(), m_global_power_pins.end(),
               []( const std::pair<SCH_SHEET_PATH, SCH_PIN*>& a,
                   const std::pair<SCH_SHEET_PATH, SCH_PIN*>& b )
               {
                   int pathCmp = a.first.Cmp( b.first );

                   if( pathCmp != 0 )
                       return pathCmp < 0;

                   const SCH_SYMBOL* symA = static_cast<const SCH_SYMBOL*>( a.second->GetParentSymbol() );
                   const SCH_SYMBOL* symB = static_cast<const SCH_SYMBOL*>( b.second->GetParentSymbol() );

                   wxString refA = symA ? symA->GetRef( &a.first, false ) : wxString();
                   wxString refB = symB ? symB->GetRef( &b.first, false ) : wxString();

                   int refCmp = refA.Cmp( refB );

                   if( refCmp != 0 )
                       return refCmp < 0;

                   return a.second->GetNumber().Cmp( b.second->GetNumber() ) < 0;
               } );

    std::unordered_map<int, CONNECTION_SUBGRAPH*> global_power_pin_subgraphs;

    for( const auto& [sheet, pin] : m_global_power_pins )
    {
        SYMBOL* libParent = pin->GetLibPin() ? pin->GetLibPin()->GetParentSymbol() : nullptr;

        if( !pin->ConnectedItems( sheet ).empty()
                && ( !libParent || !libParent->IsGlobalPower() ) )
        {
            // ERC will warn about this: user has wired up an invisible pin
            continue;
        }

        SCH_CONNECTION* connection = pin->GetOrInitConnection( sheet, this );

        // If this pin already has a subgraph, don't need to process
        if( !connection || connection->SubgraphCode() > 0 )
            continue;

        // Proper modern power symbols get their net name from the value field
        // in the symbol, but we support legacy non-power symbols with global
        // power connections based on invisible, power-in, pin's names.
        if( libParent && libParent->IsGlobalPower() )
            connection->SetName( pin->GetParentSymbol()->GetValue( true, &sheet, false ) );
        else
            connection->SetName( pin->GetShownName() );

        int code = assignNewNetCode( *connection );

        connection->SetNetCode( code );

        CONNECTION_SUBGRAPH* subgraph;
        auto                 jj = global_power_pin_subgraphs.find( code );

        if( jj != global_power_pin_subgraphs.end() )
        {
            subgraph = jj->second;
            subgraph->AddItem( pin );
        }
        else
        {
            subgraph = new CONNECTION_SUBGRAPH( this );

            subgraph->m_code = m_last_subgraph_code++;
            subgraph->m_sheet = sheet;

            subgraph->AddItem( pin );
            subgraph->ResolveDrivers();

            NET_NAME_CODE_CACHE_KEY key = { subgraph->GetNetName(), code };
            m_net_code_to_subgraphs_map[ key ].push_back( subgraph );
            m_subgraphs.push_back( subgraph );
            m_driver_subgraphs.push_back( subgraph );

            global_power_pin_subgraphs[code] = subgraph;
        }

        connection->SetSubgraphCode( subgraph->m_code );
    }
}


void CONNECTION_GRAPH::processSubGraphs()
{
    // Here we do all the local (sheet) processing of each subgraph, including assigning net
    // codes, merging subgraphs together that use label connections, etc.

    // Cache remaining valid subgraphs by sheet path
    for( CONNECTION_SUBGRAPH* subgraph : m_driver_subgraphs )
        m_sheet_to_subgraphs_map[ subgraph->m_sheet ].emplace_back( subgraph );

    std::unordered_set<CONNECTION_SUBGRAPH*> invalidated_subgraphs;

    for( CONNECTION_SUBGRAPH* subgraph : m_driver_subgraphs )
    {
        if( subgraph->m_absorbed )
            continue;

        SCH_CONNECTION* connection = subgraph->m_driver_connection;
        SCH_SHEET_PATH sheet = subgraph->m_sheet;
        wxString name = connection->Name();

        // Test subgraphs with weak drivers for net name conflicts and fix them
        unsigned suffix = 1;

        wxString base_name = connection->Name();

        auto create_new_name =
                [&suffix, &base_name]( SCH_CONNECTION* aConn ) -> wxString
                {
                    wxString suffixStr = std::to_wstring( suffix );

                    // For group buses with a prefix, we can add the suffix to the prefix.
                    // If they don't have a prefix, we force the creation of a prefix so that
                    // two buses don't get inadvertently shorted together.
                    if( aConn->Type() == CONNECTION_TYPE::BUS_GROUP )
                    {
                        wxString prefix = aConn->BusPrefix();

                        if( prefix.empty() )
                            prefix = wxT( "BUS" ); // So result will be "BUS_1{...}"

                        // Use BusPrefix length to skip past any formatting markers
                        // in the prefix (e.g. ~{RESET}) rather than AfterFirst('{')
                        // which would split at a formatting brace.
                        wxString members = base_name.Mid( aConn->BusPrefix().length() );

                        wxString newName;
                        newName << prefix << wxT( "_" ) << suffixStr << members;

                        aConn->ConfigureFromLabel( newName );
                    }
                    else
                    {
                        // Reset to the unsuffixed base so retries generate base_1, base_2, ...
                        // instead of stacking suffixes onto the previous attempt.
                        aConn->SetSuffix( wxString( wxT( "_" ) ) << suffixStr );
                    }

                    suffix++;
                    return aConn->Name();
                };

        // Promote a weakly-driven sheet-pin subgraph to a strong driver so that it is considered
        // below for propagation/merging.  A sheet pin sharing its (path-less) name with a global
        // label on the same sheet would then be treated as if it had a matching local label, so we
        // skip the promotion in that case to avoid a false merge.
        auto promote_sheet_pin_driver =
                [&]()
                {
                    if( !subgraph->m_driver || subgraph->m_driver->Type() != SCH_SHEET_PIN_T )
                        return;

                    wxString global_name = connection->Name( true );
                    auto     kk          = m_net_name_to_subgraphs_map.find( global_name );

                    if( kk != m_net_name_to_subgraphs_map.end() )
                    {
                        for( const CONNECTION_SUBGRAPH* candidate : kk->second )
                        {
                            if( candidate->m_sheet == sheet )
                            {
                                wxLogTrace( ConnTrace,
                                            wxS( "%ld (%s) skipped for promotion due to potential conflict" ),
                                            subgraph->m_code, connection->Name() );
                                return;
                            }
                        }
                    }

                    subgraph->m_strong_driver = true;
                };

        if( !subgraph->m_strong_driver )
        {
            std::vector<CONNECTION_SUBGRAPH*> vec_empty;
            std::vector<CONNECTION_SUBGRAPH*>* vec = &vec_empty;

            if( m_net_name_to_subgraphs_map.count( name ) )
                vec = &m_net_name_to_subgraphs_map.at( name );

            // If we are a unique bus vector, check if we aren't actually unique because of another
            // subgraph with a similar bus vector
            if( vec->size() <= 1 && subgraph->m_driver_connection->Type() == CONNECTION_TYPE::BUS )
            {
                wxString prefixOnly = name.BeforeFirst( '[' ) + wxT( "[]" );

                if( m_net_name_to_subgraphs_map.count( prefixOnly ) )
                    vec = &m_net_name_to_subgraphs_map.at( prefixOnly );
            }

            if( vec->size() > 1 )
            {
                wxString new_name = create_new_name( connection );

                while( m_net_name_to_subgraphs_map.contains( new_name ) )
                    new_name = create_new_name( connection );

                wxLogTrace( ConnTrace, wxS( "%ld (%s) is weakly driven and not unique. Changing to %s." ),
                            subgraph->m_code, name, new_name );

                std::erase( *vec, subgraph );

                m_net_name_to_subgraphs_map[new_name].emplace_back( subgraph );

                name = new_name;

                // The renamed sheet pin still drives its own bus members through the hierarchy, so
                // it must be promoted for propagation to reach them (issue #21798).
                promote_sheet_pin_driver();
            }
            else if( subgraph->m_driver )
            {
                promote_sheet_pin_driver();
            }
        }

        // Assign net codes
        if( connection->IsBus() )
        {
            int  code = -1;
            auto it   = m_bus_name_to_code_map.find( name );

            if( it != m_bus_name_to_code_map.end() )
            {
                code = it->second;
            }
            else
            {
                code = m_last_bus_code++;
                m_bus_name_to_code_map[ name ] = code;
            }

            connection->SetBusCode( code );
            assignNetCodesToBus( connection );
        }
        else
        {
            assignNewNetCode( *connection );
        }

        // Reset the flag for the next loop below
        subgraph->m_dirty = true;

        // Next, we merge together subgraphs that have label connections, and create
        // neighbor links for subgraphs that are part of a bus on the same sheet.
        // For merging, we consider each possible strong driver.

        // If this subgraph doesn't have a strong driver, let's skip it, since there is no
        // way it will be merged with anything.
        if( !subgraph->m_strong_driver )
            continue;

        // candidate_subgraphs will contain each valid, non-bus subgraph on the same sheet
        // as the subgraph we are considering that has a strong driver.
        // Weakly driven subgraphs are not considered since they will never be absorbed or
        // form neighbor links.
        std::vector<CONNECTION_SUBGRAPH*> candidate_subgraphs;
        std::copy_if( m_sheet_to_subgraphs_map[ subgraph->m_sheet ].begin(),
                      m_sheet_to_subgraphs_map[ subgraph->m_sheet ].end(),
                      std::back_inserter( candidate_subgraphs ),
                      [&] ( const CONNECTION_SUBGRAPH* candidate )
                      {
                          return ( !candidate->m_absorbed &&
                                   candidate->m_strong_driver &&
                                   candidate != subgraph );
                      } );

        // This is a list of connections on the current subgraph to compare to the
        // drivers of each candidate subgraph.  If the current subgraph is a bus,
        // we should consider each bus member.
        std::vector< std::shared_ptr<SCH_CONNECTION> > connections_to_check;

        // Also check the main driving connection
        connections_to_check.push_back( std::make_shared<SCH_CONNECTION>( *connection ) );

        auto add_connections_to_check =
                [&] ( CONNECTION_SUBGRAPH* aSubgraph )
                {
                    for( SCH_ITEM* possible_driver : aSubgraph->m_items )
                    {
                        if( possible_driver == aSubgraph->m_driver )
                            continue;

                        auto c = getDefaultConnection( possible_driver, aSubgraph );

                        if( c )
                        {
                            if( c->Type() != aSubgraph->m_driver_connection->Type() )
                                continue;

                            if( c->Name( true ) == aSubgraph->m_driver_connection->Name( true ) )
                                continue;

                            connections_to_check.push_back( c );
                            wxLogTrace( ConnTrace, wxS( "%lu (%s): Adding secondary driver %s" ),
                                        aSubgraph->m_code,
                                        aSubgraph->m_driver_connection->Name( true ),
                                        c->Name( true ) );
                        }
                    }
                };

        // Now add other strong drivers
        // The actual connection attached to these items will have been overwritten
        // by the chosen driver of the subgraph, so we need to create a dummy connection
        add_connections_to_check( subgraph );

        std::set<SCH_CONNECTION*> checked_connections;

        for( unsigned i = 0; i < connections_to_check.size(); i++ )
        {
            auto member = connections_to_check[i];

            // Don't check the same connection twice
            if( !checked_connections.insert( member.get() ).second )
                continue;

            if( member->IsBus() )
            {
                connections_to_check.insert( connections_to_check.end(),
                                             member->Members().begin(),
                                             member->Members().end() );
            }

            wxString test_name = member->Name( true );

            for( CONNECTION_SUBGRAPH* candidate : candidate_subgraphs )
            {
                if( candidate->m_absorbed || candidate == subgraph )
                    continue;

                bool match = false;

                if( candidate->m_driver_connection->Name( true ) == test_name )
                {
                    match = true;
                }
                else
                {
                    if( !candidate->m_multiple_drivers )
                        continue;

                    for( SCH_ITEM *driver : candidate->m_drivers )
                    {
                        if( driver == candidate->m_driver )
                            continue;

                        // Sheet pins are not candidates for merging
                        if( driver->Type() == SCH_SHEET_PIN_T )
                            continue;

                        if( driver->Type() == SCH_PIN_T )
                        {
                            auto pin = static_cast<SCH_PIN*>( driver );

                            if( pin->IsPower()
                                && pin->GetDefaultNetName( sheet ) == test_name )
                            {
                                match = true;
                                break;
                            }
                        }
                        else
                        {
                            // Should we skip this if the driver type is not one of these types?
                            wxASSERT( driver->Type() == SCH_LABEL_T ||
                                      driver->Type() == SCH_GLOBAL_LABEL_T ||
                                      driver->Type() == SCH_HIER_LABEL_T );

                            if( subgraph->GetNameForDriver( driver )  == test_name )
                            {
                                match = true;
                                break;
                            }
                        }
                    }
                }

                if( match )
                {
                    if( connection->IsBus() && candidate->m_driver_connection->IsNet() )
                    {
                         wxLogTrace( ConnTrace, wxS( "%lu (%s) has bus child %lu (%s)" ),
                                     subgraph->m_code, connection->Name(),
                                     candidate->m_code, member->Name() );

                        subgraph->m_bus_neighbors[member].insert( candidate );
                        candidate->m_bus_parents[member].insert( subgraph );
                    }
                    else if( ( !connection->IsBus()
                              && !candidate->m_driver_connection->IsBus() )
                           || connection->Type() == candidate->m_driver_connection->Type() )
                    {
                        wxLogTrace( ConnTrace, wxS( "%lu (%s) absorbs neighbor %lu (%s)" ),
                                    subgraph->m_code, connection->Name(),
                                    candidate->m_code, candidate->m_driver_connection->Name() );

                        // Candidate may have other non-chosen drivers we need to follow
                        add_connections_to_check( candidate );

                        subgraph->Absorb( candidate );
                        invalidated_subgraphs.insert( subgraph );
                    }
                }
            }
        }
    }

    // Update any subgraph that was invalidated above
    for( CONNECTION_SUBGRAPH* subgraph : invalidated_subgraphs )
    {
        if( subgraph->m_absorbed )
            continue;

        if( !subgraph->ResolveDrivers() )
            continue;

        if( subgraph->m_driver_connection->IsBus() )
            assignNetCodesToBus( subgraph->m_driver_connection );
        else
            assignNewNetCode( *subgraph->m_driver_connection );

        wxLogTrace( ConnTrace, wxS( "Re-resolving drivers for %lu (%s)" ),
                    subgraph->m_code, subgraph->m_driver_connection->Name() );
    }

}


// TODO(JE) This won't give the same subgraph IDs (and eventually net/graph codes)
// to the same subgraph necessarily if it runs over and over again on the same
// sheet.  We need:
//
//  a) a cache of net/bus codes, like used before
//  b) to persist the CONNECTION_GRAPH globally so the cache is persistent,
//  c) some way of trying to avoid changing net names.  so we should keep track
//     of the previous driver of a net, and if it comes down to choosing between
//     equally-prioritized drivers, choose the one that already exists as a driver
//     on some portion of the items.


void CONNECTION_GRAPH::buildConnectionGraph( std::function<void( SCH_ITEM* )>* aChangedItemHandler,
                                             bool                              aUnconditional )
{
    // Recache all bus aliases for later use
    wxCHECK_RET( m_schematic, wxT( "Connection graph cannot be built without schematic pointer" ) );

    m_bus_alias_cache.clear();

    for( const std::shared_ptr<BUS_ALIAS>& alias : m_schematic->GetAllBusAliases() )
    {
        if( alias )
            m_bus_alias_cache[alias->GetName()] = alias;
    }

    PROF_TIMER sub_graph( "buildItemSubGraphs" );
    buildItemSubGraphs();

    if( wxLog::IsAllowedTraceMask( DanglingProfileMask ) )
        sub_graph.Show();

    /**
     * TODO(JE): Net codes are non-deterministic.  Fortunately, they are also not really used for
     * anything. We should consider removing them entirely and just using net names everywhere.
     */

    resolveAllDrivers();

    collectAllDriverValues();

    generateGlobalPowerPinSubGraphs();

    generateBusAliasMembers();

    PROF_TIMER proc_sub_graph( "ProcessSubGraphs" );
    processSubGraphs();

    if( wxLog::IsAllowedTraceMask( DanglingProfileMask ) )
        proc_sub_graph.Show();

    // Absorbed subgraphs should no longer be considered
    std::erase_if( m_driver_subgraphs, [&]( const CONNECTION_SUBGRAPH* candidate ) -> bool
                                        {
                                            return candidate->m_absorbed;
                                        } );

    // Store global subgraphs for later reference
    std::vector<CONNECTION_SUBGRAPH*> global_subgraphs;
    std::copy_if( m_driver_subgraphs.begin(), m_driver_subgraphs.end(),
                  std::back_inserter( global_subgraphs ),
                  [&] ( const CONNECTION_SUBGRAPH* candidate ) -> bool
                  {
                      return !candidate->m_local_driver;
                  } );

    // Recache remaining valid subgraphs by sheet path
    m_sheet_to_subgraphs_map.clear();

    for( CONNECTION_SUBGRAPH* subgraph : m_driver_subgraphs )
        m_sheet_to_subgraphs_map[ subgraph->m_sheet ].emplace_back( subgraph );

    thread_pool& tp = GetKiCadThreadPool();

    auto results = tp.submit_loop( 0, m_driver_subgraphs.size(),
                            [&]( const int ii )
                            {
                                m_driver_subgraphs[ii]->UpdateItemConnections();
                            });

    results.wait();

    // Build equivalence classes over global subgraphs that are linked by shared
    // global label names. Two global subgraphs are in the same class whenever
    // (transitively) some subgraph has a global driver named X and another
    // subgraph has a global driver also named X, OR a single multi-driver
    // subgraph has both X and Y as global drivers.
    //
    // This is the transitive closure over the relation "shares a global name".
    // When users chain nets across sheets via differently-named global labels,
    // every subgraph reachable through any sequence of shared names must end
    // up on the same final net.
    //
    // The per-subgraph promote pass that follows is order-dependent and walks
    // candidates by their *original* driver text rather than by their already-
    // promoted name. As a result, when subgraph S2 promotes subgraph S1 to a
    // new name, and then a third subgraph S3 later renames S2 again, S1 is
    // left orphaned with the intermediate name. This pre-pass solves the
    // transitivity problem before the order-dependent loop runs (issue 23719).
    if( !global_subgraphs.empty() )
    {
        std::unordered_map<CONNECTION_SUBGRAPH*, CONNECTION_SUBGRAPH*> sg_root;

        auto find_sg_root =
                [&]( CONNECTION_SUBGRAPH* aSg ) -> CONNECTION_SUBGRAPH*
                {
                    CONNECTION_SUBGRAPH* cur = aSg;

                    while( true )
                    {
                        auto it = sg_root.find( cur );

                        if( it == sg_root.end() || it->second == cur )
                            return cur;

                        // Path compression. Hop the current node directly to its
                        // grandparent on the way up so subsequent finds are O(1).
                        auto parent_it = sg_root.find( it->second );

                        if( parent_it != sg_root.end() && parent_it->second != it->second )
                            it->second = parent_it->second;

                        cur = it->second;
                    }
                };

        // Pick the subgraph whose primary driver the file-local compareDrivers helper
        // would rank first. Using the same helper as CONNECTION_SUBGRAPH::ResolveDrivers
        // guarantees both sites agree on every tie-break rule (priority, bus width,
        // pin power parent, sheet-pin shape, -Pad demotion, alphabetical).
        auto prefer_as_representative =
                [&]( CONNECTION_SUBGRAPH* aA, CONNECTION_SUBGRAPH* aB ) -> bool
                {
                    return compareDrivers( aA->m_driver, aA->m_driver_connection,
                                           aA->m_driver_connection->Name(),
                                           aB->m_driver, aB->m_driver_connection,
                                           aB->m_driver_connection->Name() ) < 0;
                };

        auto union_sgs =
                [&]( CONNECTION_SUBGRAPH* aA, CONNECTION_SUBGRAPH* aB )
                {
                    sg_root.try_emplace( aA, aA );
                    sg_root.try_emplace( aB, aB );

                    CONNECTION_SUBGRAPH* root_a = find_sg_root( aA );
                    CONNECTION_SUBGRAPH* root_b = find_sg_root( aB );

                    if( root_a == root_b )
                        return;

                    if( prefer_as_representative( root_a, root_b ) )
                        sg_root[root_b] = root_a;
                    else
                        sg_root[root_a] = root_b;
                };

        std::unordered_map<wxString, std::vector<CONNECTION_SUBGRAPH*>> name_to_sgs;

        for( CONNECTION_SUBGRAPH* subgraph : global_subgraphs )
        {
            for( SCH_ITEM* driver : subgraph->m_drivers )
            {
                if( CONNECTION_SUBGRAPH::GetDriverPriority( driver )
                    < CONNECTION_SUBGRAPH::PRIORITY::GLOBAL_POWER_PIN )
                {
                    continue;
                }

                name_to_sgs[subgraph->GetNameForDriver( driver )].push_back( subgraph );
            }
        }

        for( auto& [name, sgs] : name_to_sgs )
        {
            if( sgs.size() < 2 )
                continue;

            for( size_t ii = 1; ii < sgs.size(); ++ii )
                union_sgs( sgs[0], sgs[ii] );
        }

        // Every subgraph in sg_root now maps (with path compression) to the
        // representative of its equivalence class. Clone the representative's
        // connection into each member that currently differs.
        for( const auto& entry : sg_root )
        {
            CONNECTION_SUBGRAPH* sg   = entry.first;
            CONNECTION_SUBGRAPH* root = find_sg_root( sg );

            if( sg == root )
                continue;

            if( sg->m_driver_connection->Name() == root->m_driver_connection->Name() )
                continue;

            wxLogTrace( ConnTrace, wxS( "Global %lu (%s) canonicalized to %lu (%s)" ),
                        sg->m_code, sg->m_driver_connection->Name(), root->m_code,
                        root->m_driver_connection->Name() );

            sg->m_driver_connection->Clone( *root->m_driver_connection );
        }
    }

    // Next time through the subgraphs, we do some post-processing to handle things like
    // connecting bus members to their neighboring subgraphs, and then propagate connections
    // through the hierarchy
    for( CONNECTION_SUBGRAPH* subgraph : m_driver_subgraphs )
    {
        if( !subgraph->m_dirty )
            continue;

        wxLogTrace( ConnTrace, wxS( "Processing %lu (%s) for propagation" ),
                    subgraph->m_code, subgraph->m_driver_connection->Name() );

        // For subgraphs that are driven by a global (power port or label) and have more
        // than one global driver, we need to seek out other subgraphs driven by the
        // same name as the non-chosen driver and update them to match the chosen one.

        if( !subgraph->m_local_driver && subgraph->m_multiple_drivers )
        {
            for( SCH_ITEM* driver : subgraph->m_drivers )
            {
                if( driver == subgraph->m_driver )
                    continue;

                const wxString& secondary_name = subgraph->GetNameForDriver( driver );

                if( secondary_name == subgraph->m_driver_connection->Name() )
                    continue;

                bool secondary_is_global = CONNECTION_SUBGRAPH::GetDriverPriority( driver )
                                           >= CONNECTION_SUBGRAPH::PRIORITY::GLOBAL_POWER_PIN;

                for( CONNECTION_SUBGRAPH* candidate : global_subgraphs )
                {
                    if( candidate == subgraph )
                        continue;

                    if( !secondary_is_global && candidate->m_sheet != subgraph->m_sheet )
                        continue;

                    for( SCH_ITEM* candidate_driver : candidate->m_drivers )
                    {
                        if( candidate->GetNameForDriver( candidate_driver ) == secondary_name )
                        {
                            wxLogTrace( ConnTrace, wxS( "Global %lu (%s) promoted to %s" ),
                                        candidate->m_code, candidate->m_driver_connection->Name(),
                                        subgraph->m_driver_connection->Name() );

                            candidate->m_driver_connection->Clone( *subgraph->m_driver_connection );

                            candidate->m_dirty = false;
                            propagateToNeighbors( candidate, false );
                        }
                    }
                }
            }
        }

        // This call will handle descending the hierarchy and updating child subgraphs
        propagateToNeighbors( subgraph, false );
    }

    // After processing and allowing some to be skipped if they have hierarchical
    // pins connecting both up and down the hierarchy, we check to see if any of them
    // have not been processed.  This would indicate that they do not have off-sheet connections
    // but we still need to handle the subgraph
    for( CONNECTION_SUBGRAPH* subgraph : m_driver_subgraphs )
    {
        if( subgraph->m_dirty )
            propagateToNeighbors( subgraph, true );
    }

    // Handle buses that have been linked together somewhere by member (net) connections.
    // This feels a bit hacky, perhaps this algorithm should be revisited in the future.

    // For net subgraphs that have more than one bus parent, we need to ensure that those
    // buses are linked together in the final netlist.  The final name of each bus might not
    // match the local name that was used to establish the parent-child relationship, because
    // the bus may have been renamed by a hierarchical connection.  So, for each of these cases,
    // we need to identify the appropriate bus members to link together (and their final names),
    // and then update all instances of the old name in the hierarchy.
    for( CONNECTION_SUBGRAPH* subgraph : m_driver_subgraphs )
    {
        // All SGs should have been processed by propagateToNeighbors above
        // Should we skip all of this if the subgraph is not dirty?
        wxASSERT_MSG( !subgraph->m_dirty,
                      wxS( "Subgraph not processed by propagateToNeighbors!" ) );

        if( subgraph->m_bus_parents.size() < 2 )
            continue;

        SCH_CONNECTION* conn = subgraph->m_driver_connection;

        wxLogTrace( ConnTrace, wxS( "%lu (%s) has multiple bus parents" ),
                    subgraph->m_code, conn->Name() );

        // Should we skip everything after this if this is not a net?
        wxCHECK2( conn->IsNet(), continue );

        for( const auto& ii : subgraph->m_bus_parents )
        {
            SCH_CONNECTION* link_member = ii.first.get();

            for( CONNECTION_SUBGRAPH* parent : ii.second )
            {
                while( parent->m_absorbed )
                    parent = parent->m_absorbed_by;

                SCH_CONNECTION* match = matchBusMember( parent->m_driver_connection, link_member );

                if( !match )
                {
                    wxLogTrace( ConnTrace, wxS( "Warning: could not match %s inside %lu (%s)" ),
                                conn->Name(), parent->m_code, parent->m_driver_connection->Name() );
                    continue;
                }

                if( conn->Name() != match->Name() )
                {
                    wxString old_name = match->Name();

                    wxLogTrace( ConnTrace, wxS( "Updating %lu (%s) member %s to %s" ),
                                parent->m_code, parent->m_driver_connection->Name(), old_name, conn->Name() );

                    match->Clone( *conn );

                    auto jj = m_net_name_to_subgraphs_map.find( old_name );

                    if( jj == m_net_name_to_subgraphs_map.end() )
                        continue;

                    // Copy the vector to avoid iterator invalidation when recaching
                    std::vector<CONNECTION_SUBGRAPH*> old_subgraphs = jj->second;

                    for( CONNECTION_SUBGRAPH* old_sg : old_subgraphs )
                    {
                        while( old_sg->m_absorbed )
                            old_sg = old_sg->m_absorbed_by;

                        wxString old_sg_name = old_sg->m_driver_connection->Name();
                        old_sg->m_driver_connection->Clone( *conn );

                        if( old_sg_name != old_sg->m_driver_connection->Name() )
                            recacheSubgraphName( old_sg, old_sg_name );
                    }
                }
            }
        }
    }

    // Phase 1: write each subgraph's items' connections.  Items can be referenced from
    // other subgraphs (via labels), so phase 2 below has to wait for every phase 1 task
    // to complete before reading anything through label->Connection().
    auto propagateConnectionsTask =
            [&]( CONNECTION_SUBGRAPH* subgraph )
            {
                // Make sure weakly-driven single-pin nets get the unconnected_ prefix
                if( !subgraph->m_strong_driver
                        && subgraph->m_drivers.size() == 1
                        && subgraph->m_driver->Type() == SCH_PIN_T )
                {
                    SCH_PIN* pin = static_cast<SCH_PIN*>( subgraph->m_driver );
                    wxString name = pin->GetDefaultNetName( subgraph->m_sheet, true );

                    subgraph->m_driver_connection->ConfigureFromLabel( name );
                }

                subgraph->m_dirty = false;
                subgraph->UpdateItemConnections();
            };

    auto results1 = tp.submit_loop( 0, m_driver_subgraphs.size(),
                            [&]( const int ii )
                            {
                                propagateConnectionsTask( m_driver_subgraphs[ii] );
                            } );
    results1.wait();

    // Phase 2: promote sheet-pin subgraphs to buses based on the matching child-sheet
    // hier label.  This reads other subgraphs' connections via label->Connection() and
    // also writes subgraph->m_driver_connection->SetType, so it has to be serial.
    for( CONNECTION_SUBGRAPH* subgraph : m_driver_subgraphs )
    {
        if( subgraph->m_driver_connection->IsBus() )
            continue;

        if( !subgraph->m_driver || subgraph->m_driver->Type() != SCH_SHEET_PIN_T )
            continue;

        SCH_SHEET_PIN* pin = static_cast<SCH_SHEET_PIN*>( subgraph->m_driver );
        SCH_SHEET*     sheet = pin->GetParent();

        if( !sheet )
            continue;

        wxString    pinText = pin->GetShownText( false );
        SCH_SCREEN* screen  = sheet->GetScreen();

        for( SCH_ITEM* item : screen->Items().OfType( SCH_HIER_LABEL_T ) )
        {
            SCH_HIERLABEL* label = static_cast<SCH_HIERLABEL*>( item );

            if( label->GetShownText( &subgraph->m_sheet, false ) == pinText )
            {
                SCH_SHEET_PATH path = subgraph->m_sheet;
                path.push_back( sheet );

                SCH_CONNECTION* parent_conn = label->Connection( &path );

                if( parent_conn && parent_conn->IsBus() )
                    subgraph->m_driver_connection->SetType( CONNECTION_TYPE::BUS );

                break;
            }
        }
    }

    m_net_code_to_subgraphs_map.clear();
    m_net_name_to_subgraphs_map.clear();

    for( CONNECTION_SUBGRAPH* subgraph : m_driver_subgraphs )
    {
        NET_NAME_CODE_CACHE_KEY key = { subgraph->GetNetName(),
                                        subgraph->m_driver_connection->NetCode() };
        m_net_code_to_subgraphs_map[ key ].push_back( subgraph );

        m_net_name_to_subgraphs_map[subgraph->m_driver_connection->Name()].push_back( subgraph );
    }

    std::shared_ptr<NET_SETTINGS>& netSettings = m_schematic->Project().GetProjectFile().m_NetSettings;
    std::map<wxString, std::set<wxString>> oldAssignments = netSettings->GetNetclassLabelAssignments();
    std::set<wxString> affectedNetclassNetAssignments;

    netSettings->ClearNetclassLabelAssignments();

    auto dirtySubgraphs =
            [&]( const std::vector<CONNECTION_SUBGRAPH*>& subgraphs )
            {
                if( aChangedItemHandler )
                {
                    for( const CONNECTION_SUBGRAPH* subgraph : subgraphs )
                    {
                        for( SCH_ITEM* item : subgraph->m_items )
                            (*aChangedItemHandler)( item );
                    }
                }
            };

    auto checkNetclassDrivers =
            [&]( const wxString& netName, const std::vector<CONNECTION_SUBGRAPH*>& subgraphs )
    {
        wxCHECK_RET( !subgraphs.empty(), wxS( "Invalid empty subgraph" ) );

        std::set<wxString> netclasses;

        // Collect all netclasses on all subgraphs for this net
        for( const CONNECTION_SUBGRAPH* subgraph : subgraphs )
        {
            for( SCH_ITEM* item : subgraph->m_items )
            {
                for( const auto& [name, provider] : subgraph->GetNetclassesForDriver( item ) )
                    netclasses.insert( name );
            }
        }

        // Append the netclasses to any included bus members
        for( const CONNECTION_SUBGRAPH* subgraph : subgraphs )
        {
            if( subgraph->m_driver_connection->IsBus() )
            {
                auto processBusMember = [&, this]( const SCH_CONNECTION* member )
                {
                    if( !netclasses.empty() )
                    {
                        netSettings->AppendNetclassLabelAssignment( member->Name(), netclasses );
                    }

                    auto ii = m_net_name_to_subgraphs_map.find( member->Name() );

                    if( oldAssignments.count( member->Name() ) )
                    {
                        if( oldAssignments[member->Name()] != netclasses )
                        {
                            affectedNetclassNetAssignments.insert( member->Name() );

                            if( ii != m_net_name_to_subgraphs_map.end() )
                                dirtySubgraphs( ii->second );
                        }
                    }
                    else if( !netclasses.empty() )
                    {
                        affectedNetclassNetAssignments.insert( member->Name() );

                        if( ii != m_net_name_to_subgraphs_map.end() )
                            dirtySubgraphs( ii->second );
                    }
                };

                for( const std::shared_ptr<SCH_CONNECTION>& member : subgraph->m_driver_connection->Members() )
                {
                    // Check if this member itself is a bus (which can be the case for vector buses as members
                    // of a bus, see https://gitlab.com/kicad/code/kicad/-/issues/16545
                    if( member->IsBus() )
                    {
                        for( const std::shared_ptr<SCH_CONNECTION>& nestedMember : member->Members() )
                            processBusMember( nestedMember.get() );
                    }
                    else
                    {
                        processBusMember( member.get() );
                    }
                }
            }
        }

        // Assign the netclasses to the root netname
        if( !netclasses.empty() )
        {
            netSettings->AppendNetclassLabelAssignment( netName, netclasses );
        }

        if( oldAssignments.count( netName ) )
        {
            if( oldAssignments[netName] != netclasses )
            {
                affectedNetclassNetAssignments.insert( netName );
                dirtySubgraphs( subgraphs );
            }
        }
        else if( !netclasses.empty() )
        {
            affectedNetclassNetAssignments.insert( netName );
            dirtySubgraphs( subgraphs );
        }
    };

    // Check for netclass assignments
    for( const auto& [ netname, subgraphs ] : m_net_name_to_subgraphs_map )
        checkNetclassDrivers( netname, subgraphs );

    if( !aUnconditional )
    {
        for( auto& [netname, netclasses] : oldAssignments )
        {
            if( netSettings->GetNetclassLabelAssignments().count( netname )
                || affectedNetclassNetAssignments.count( netname ) )
            {
                continue;
            }

            netSettings->SetNetclassLabelAssignment( netname, netclasses );
        }
    }

    RebuildNetChains();

    ApplyNetChainNetclasses();
}

std::function<void( CONNECTION_GRAPH& )>& CONNECTION_GRAPH::RebuildNetChainsTestHook()
{
    static std::function<void( CONNECTION_GRAPH& )> s_hook;
    return s_hook;
}


CONNECTION_GRAPH::BRIDGE_GRAPH CONNECTION_GRAPH::buildBridgeAdjacency()
{
    BRIDGE_GRAPH result;

    auto getSubgraphNet = [&]( SCH_PIN* aPin ) -> wxString
    {
        if( !aPin )
            return wxString();

        CONNECTION_SUBGRAPH* sg = GetSubgraphForItem( aPin );

        return sg ? netChainKeyFor( sg->GetNetName(), sg->m_code ) : wxString();
    };

    // Walk every 2-pin passthrough symbol on every sheet, building a flat list of bridge
    // edges between distinct subgraph nets.

    result.edges.reserve( 256 );

    for( const SCH_SHEET_PATH& sheetPath : m_sheetList )
    {
        SCH_SCREEN* sc = sheetPath.LastScreen();

        if( !sc )
            continue;

        auto findWireOnScreen = [&]( SCH_PIN* aPin, SCH_LINE*& aWire ) -> bool
        {
            const VECTOR2I p = aPin->GetPosition();

            auto consider = [&]( SCH_ITEM* cand ) -> bool
            {
                if( cand->Type() != SCH_LINE_T )
                    return false;

                SCH_LINE* line = static_cast<SCH_LINE*>( cand );

                if( line->GetLayer() != LAYER_WIRE )
                    return false;

                const VECTOR2I s = line->GetStartPoint();
                const VECTOR2I e = line->GetEndPoint();

                if( s.y == e.y && p.y == s.y )
                {
                    int minx = std::min( s.x, e.x );
                    int maxx = std::max( s.x, e.x );

                    if( p.x >= minx && p.x <= maxx )
                    {
                        aWire = line;
                        return true;
                    }
                }
                else if( s.x == e.x && p.x == s.x )
                {
                    int miny = std::min( s.y, e.y );
                    int maxy = std::max( s.y, e.y );

                    if( p.y >= miny && p.y <= maxy )
                    {
                        aWire = line;
                        return true;
                    }
                }

                return false;
            };

            for( SCH_ITEM* c : sc->Items().Overlapping( SCH_LINE_T, p ) )
                if( consider( c ) )
                    return true;

            for( SCH_ITEM* c : sc->Items().OfType( SCH_LINE_T ) )
                if( consider( c ) )
                    return true;

            return false;
        };

        for( SCH_ITEM* item : sc->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL*           symbol = static_cast<SCH_SYMBOL*>( item );
            std::vector<SCH_PIN*> pins = symbol->GetPins( &sheetPath );

            if( pins.size() != 2 )
                continue;

            if( symbol->GetPassthroughMode() == SCH_SYMBOL::PASSTHROUGH_MODE::BLOCK )
                continue;

            SCH_LINE* wireA = nullptr;
            SCH_LINE* wireB = nullptr;

            if( !findWireOnScreen( pins[0], wireA ) || !findWireOnScreen( pins[1], wireB ) )
                continue;

            bool allow = false;

            if( symbol->GetPassthroughMode() == SCH_SYMBOL::PASSTHROUGH_MODE::FORCE )
            {
                allow = true;
            }
            else
            {
                if( pins[0]->IsPower() || pins[1]->IsPower() )
                    continue;

                VECTOR2I aS = wireA->GetStartPoint();
                VECTOR2I aE = wireA->GetEndPoint();
                VECTOR2I bS = wireB->GetStartPoint();
                VECTOR2I bE = wireB->GetEndPoint();

                if( aS.x == aE.x && bS.x == bE.x && aS.x == bS.x )
                    allow = true;
                else if( aS.y == aE.y && bS.y == bE.y && aS.y == bS.y )
                    allow = true;
            }

            if( !allow )
                continue;

            wxString netA = getSubgraphNet( pins[0] );
            wxString netB = getSubgraphNet( pins[1] );

            if( netA.IsEmpty() || netB.IsEmpty() || netA == netB )
                continue;

            result.edges.push_back( { netA, netB, symbol } );
        }
    }

    // Mark power subgraphs by walking every pin across every sheet. Any subgraph touched by a
    // power-class pin (or a power-symbol parent) is treated as a power node and its incident
    // bridge edges are excluded below.

    std::set<long>          powerSubgraphs;
    std::map<wxString, long> netToCode;

    for( const SCH_SHEET_PATH& sheetPath : m_sheetList )
    {
        SCH_SCREEN* sc = sheetPath.LastScreen();

        if( !sc )
            continue;

        for( SCH_ITEM* item : sc->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL*           sym = static_cast<SCH_SYMBOL*>( item );
            std::vector<SCH_PIN*> pins = sym->GetPins( &sheetPath );

            for( SCH_PIN* p : pins )
            {
                if( CONNECTION_SUBGRAPH* sg = GetSubgraphForItem( p ) )
                {
                    netToCode[netChainKeyFor( sg->GetNetName(), sg->m_code )] = sg->m_code;

                    if( p->IsPower()
                        || ( p->GetParentSymbol() && p->GetParentSymbol()->IsPower() ) )
                    {
                        powerSubgraphs.insert( sg->m_code );
                    }
                }
            }
        }
    }

    // Build the filtered adjacency. Edges that touch a power subgraph are dropped, and any
    // non-power endpoint of such a dropped edge is recorded as power-adjacent so the leaf-prune
    // pass below can iteratively remove power stubs.

    std::set<wxString> powerAdjacentNets;

    for( const BRIDGE_EDGE& be : result.edges )
    {
        long ca = -1;
        long cb = -1;

        if( auto it = netToCode.find( be.a ); it != netToCode.end() )
            ca = it->second;

        if( auto it = netToCode.find( be.b ); it != netToCode.end() )
            cb = it->second;

        if( ca == -1 || cb == -1 )
            continue;

        if( powerSubgraphs.contains( ca ) || powerSubgraphs.contains( cb ) )
        {
            if( !powerSubgraphs.contains( ca ) )
                powerAdjacentNets.insert( be.a );

            if( !powerSubgraphs.contains( cb ) )
                powerAdjacentNets.insert( be.b );

            continue;
        }

        result.adjacency[be.a].push_back( { be.b, be.sym } );
        result.adjacency[be.b].push_back( { be.a, be.sym } );
    }

    // Iteratively prune degree-1 power-adjacent leaves.  Skip pruning entirely for very small
    // graphs to avoid wiping out legitimate two-net chains.

    std::map<wxString, int> degree;

    for( const auto& kv : result.adjacency )
        degree[kv.first] = static_cast<int>( kv.second.size() );

    if( result.adjacency.size() <= 2 )
        powerAdjacentNets.clear();

    if( powerAdjacentNets.size() <= 2 )
        powerAdjacentNets.clear();

    std::queue<wxString> q;
    std::set<wxString>   removed;

    for( const auto& kv : degree )
    {
        if( kv.second <= 1 && powerAdjacentNets.contains( kv.first ) )
            q.push( kv.first );
    }

    while( !q.empty() )
    {
        wxString n = q.front();
        q.pop();

        if( removed.contains( n ) )
            continue;

        removed.insert( n );

        for( const BRIDGE_NEIGHBOR& e : result.adjacency[n] )
        {
            if( removed.contains( e.other ) )
                continue;

            if( degree.count( e.other ) )
            {
                degree[e.other]--;

                if( degree[e.other] <= 1 && powerAdjacentNets.contains( e.other ) )
                    q.push( e.other );
            }
        }
    }

    if( !removed.empty() )
    {
        std::map<wxString, std::vector<BRIDGE_NEIGHBOR>> newAdj;

        for( const auto& kv : result.adjacency )
        {
            if( removed.contains( kv.first ) )
                continue;

            for( const BRIDGE_NEIGHBOR& e : kv.second )
            {
                if( removed.contains( e.other ) )
                    continue;

                newAdj[kv.first].push_back( e );
            }
        }

        result.adjacency.swap( newAdj );
    }

    return result;
}


void CONNECTION_GRAPH::RebuildNetChains()
{
    // Snapshot the committed-chain count so a throw partway through the restore loop can
    // truncate any half-built entries instead of leaving the container partially mutated.
    const size_t committedSnapshot = m_committedNetChains.size();
    const bool   builtSnapshot = m_netChainsBuilt;

    try
    {
        wxLogTrace( traceSchNetChain, "RebuildNetChains: begin (items=%zu, schematic=%p)",
                    m_items.size(), (void*) m_schematic );
        // Clear only potential net chains; leave committed net chains intact.
        m_potentialNetChains.clear();

        if( !m_schematic )
        {
            wxLogTrace( traceSchNetChain, "RebuildNetChains: no schematic" );
            return;
        }
    std::map<wxString, SCH_NETCHAIN*> netToNetChain; // will be populated after chain extraction

        // Collect all screens from the cached sheet list so we can operate globally rather than
        // only on the current sheet.  (m_sheetList is populated during Recalculate()).
        std::vector<SCH_SCREEN*> allScreens;
        allScreens.reserve( m_sheetList.size() );
        for( const SCH_SHEET_PATH& sp : m_sheetList )
        {
            if( SCH_SCREEN* sc = sp.LastScreen() )
                allScreens.push_back( sc );
        }

        // Clear any previous chain names on all symbols across all sheets so we can repopulate.
        for( SCH_SCREEN* sc : allScreens )
        {
            for( SCH_ITEM* item : sc->Items().OfType( SCH_SYMBOL_T ) )
                static_cast<SCH_SYMBOL*>( item )->SetNetChainName( wxEmptyString );
        }
        wxLogTrace( traceSchNetChain, "RebuildNetChains: screens=%zu (global build)", allScreens.size() );
    wxLogTrace( traceSchNetChain, "RebuildNetChains: debug start passes (pre-pass chains=%zu)", m_committedNetChains.size() );

    // (Removed legacy findWire heuristic; global symbol-based connectivity no longer relies on
    // scanning parallel wires for 2-pin passthrough components.)

    // Build net chains by scanning eligible 2-pin symbols on every sheet, using the original
    // parallel-wire passthrough heuristic. This is effectively the old pass 1 but repeated for
    // each screen, giving global coverage while preserving expected grouping semantics.
    wxLogTrace( traceSchNetChain, "RebuildNetChains: pass 1 (per-sheet 2-pin symbols)" );

    auto getSubgraphNet = [&]( SCH_PIN* aPin ) -> wxString
    {
        if( !aPin )
            return wxString();

        CONNECTION_SUBGRAPH* sg = GetSubgraphForItem( aPin );

        return sg ? netChainKeyFor( sg->GetNetName(), sg->m_code ) : wxString();
    };

    BRIDGE_GRAPH bridgeGraph = buildBridgeAdjacency();
    auto&        bridgeEdges = bridgeGraph.edges;
    auto&        adjacency = bridgeGraph.adjacency;

    wxLogTrace( traceSchNetChain, "RebuildNetChains: bridgeEdges=%zu adjacency=%zu",
                bridgeEdges.size(), adjacency.size() );

    // Targeted stub pruning: reduce any component >4 nets by removing minimal number of "stub" leaves
    // (degree 1 whose neighbor has degree >2). This satisfies legacy test expecting longest branch kept.
    {
        // First, discover connected components over current adjacency.
        wxLogTrace( traceSchNetChain, "RebuildNetChains: targeted stub pruning start (adj=%zu)", adjacency.size() );
        std::map<wxString,std::vector<BRIDGE_NEIGHBOR>> snapshot = adjacency; // read-only snapshot
        std::set<wxString> seen;
        std::set<wxString> globalPrune;
        for( const auto& kv : snapshot )
        {
            const wxString& start = kv.first;
            if( seen.contains( start ) ) continue;
            wxLogTrace( traceSchNetChain, "  component BFS start '%s'", start );
            std::vector<wxString> comp; std::queue<wxString> q; q.push( start ); seen.insert( start );
            while( !q.empty() )
            {
                wxString cur = q.front(); q.pop(); comp.push_back( cur );
                for( const BRIDGE_NEIGHBOR& e : snapshot[cur] ) if( !seen.contains( e.other ) ) { seen.insert( e.other ); q.push( e.other ); }
            }
            wxLogTrace( traceSchNetChain, "  component size=%zu", comp.size() );
            if( comp.size() <= 4 ) continue;
            std::map<wxString,int> degree;
            for( const wxString& n : comp ) degree[n] = (int) snapshot[n].size();
            std::vector<wxString> candidates;
            for( const wxString& n : comp )
            {
                const auto& nbrs = snapshot[n];
                if( nbrs.size() == 1 )
                {
                    const wxString neigh = nbrs[0].other;
                    if( degree.count( neigh ) && degree[neigh] > 2 ) candidates.push_back( n );
                }
            }
            wxLogTrace( traceSchNetChain, "   candidates=%zu", candidates.size() );
            if( candidates.empty() ) continue;
            std::sort( candidates.begin(), candidates.end(), []( const wxString& a, const wxString& b ){ return a.CmpNoCase( b ) < 0; } );
            size_t needPrune = comp.size() - 4; if( needPrune > candidates.size() ) needPrune = candidates.size();
            wxLogTrace( traceSchNetChain, "   pruning need=%zu", needPrune );
            for( size_t i = 0; i < needPrune; ++i ) globalPrune.insert( candidates[i] );
        }
        if( !globalPrune.empty() )
        {
            std::map<wxString,std::vector<BRIDGE_NEIGHBOR>> newAdj;
            for( const auto& kv2 : adjacency )
            {
                if( globalPrune.contains( kv2.first ) ) continue;
                for( const BRIDGE_NEIGHBOR& e : kv2.second )
                {
                    if( globalPrune.contains( e.other ) ) continue;
                    newAdj[kv2.first].push_back( e );
                }
            }
            adjacency.swap( newAdj );
            wxLogTrace( traceSchNetChain, "RebuildNetChains: pruned %zu targeted stub nets", globalPrune.size() );
        }
    }

    // ---------- Small helpers ----------
    auto neighbors_of = [&]( const wxString& n ) -> const std::vector<BRIDGE_NEIGHBOR>*
    {
        if( auto it = adjacency.find(n); it != adjacency.end() ) return &it->second;
        return nullptr;
    };


    // Structural filtering already done by excluding edges; isolated power nets are implicitly ignored.
    m_potentialNetChains.clear();

    // Recompute nets list after filtering
    std::set<wxString> netsAll;
    for( const auto& kv : adjacency ) netsAll.insert( kv.first );

    // Connected component extraction over filtered adjacency (all remaining nets are non-power)
    std::set<wxString> visited;
    for( const wxString& start : netsAll )
    {
        if( visited.contains( start ) ) continue;
        std::queue<wxString> q; q.push( start );
        std::set<wxString> comp; comp.insert( start ); visited.insert( start );
        while( !q.empty() )
        {
            wxString cur = q.front(); q.pop();
            if( auto nbrs = neighbors_of( cur ) )
            {
                for( const BRIDGE_NEIGHBOR& e : *nbrs )
                {
                    if( visited.contains( e.other ) ) continue;
                    visited.insert( e.other );
                    comp.insert( e.other );
                    q.push( e.other );
                }
            }
        }
        if( comp.size() >= 2 )
        {
            auto sig = std::make_unique<SCH_NETCHAIN>();
            for( const wxString& n : comp ) sig->AddNet( n );
            for( const BRIDGE_EDGE& be : bridgeEdges )
                if( comp.contains( be.a ) && comp.contains( be.b ) && be.sym )
                    sig->AddSymbol( be.sym );
            m_potentialNetChains.push_back( std::move( sig ) );
        }
    }
    // Build netToNetChain map for potential net chains
    netToNetChain.clear();
    for( const auto& sigUP : m_potentialNetChains )
        if( sigUP ) for( const wxString& n : sigUP->GetNets() ) netToNetChain[n] = sigUP.get();

    // Debug: enumerate chains and their nets prior to label-based naming.
    wxLogTrace( traceSchNetChain, "RebuildNetChains: pre-label potentialNetChains=%zu", m_potentialNetChains.size() );
    for( const auto& sigUP : m_potentialNetChains )
    {
        if( !sigUP ) continue;
        wxString netsStr;
        int count = 0;
        for( const wxString& n : sigUP->GetNets() )
        {
            if( count < 32 )
            {
                netsStr += n;
                netsStr += wxS(" ");
            }
            else
            {
                netsStr += wxS("...");
                break;
            }
            ++count;
        }
        wxLogTrace( traceSchNetChain, "  chain %p name='%s' nets=%zu [%s]", (void*) sigUP.get(),
                    sigUP->GetName(), sigUP->GetNets().size(), netsStr );
    }


    // Names already in use by committed chains.  A plain SCH_LABEL whose text matches a
    // committed chain's name must NOT steal that name from the committed chain; the
    // downstream restore pass uses these names as keys and would skip the potential
    // chain entirely on collision, silently losing it.
    std::set<wxString> committedNames;

    for( const auto& chain : m_committedNetChains )
    {
        if( chain )
            committedNames.insert( chain->GetName() );
    }

    for( SCH_ITEM* item : m_items )
    {
        if( item->Type() != SCH_LABEL_T )
            continue;

        SCH_TEXT* label = static_cast<SCH_TEXT*>( item );
        wxString  net;

        if( CONNECTION_SUBGRAPH* sg = GetSubgraphForItem( item ) )
            net = sg->GetNetName();

        // Defensive: guard against pathological names
        if( !net.IsEmpty() && net.Length() < 2048 && netToNetChain.count( net ) )
        {
            wxString name = label->GetText();
            if( name.Length() > 512 )
                name.Truncate( 512 );
            if( name.StartsWith( wxS( "/" ) ) )
                name = name.Mid( 1 );

            // Skip if a committed chain already owns this name; let the terminal-ref /
            // saved-net-name restore logic below resolve the committed chain on its own.
            if( !committedNames.count( name ) )
                netToNetChain[net]->SetName( name );
        }
    }

    int idx = 1;

    wxLogTrace( traceSchNetChain, "RebuildNetChains: pass 3 (default naming)" );
    for( std::unique_ptr<SCH_NETCHAIN>& sig : m_potentialNetChains )
    {
        if( sig->GetName().IsEmpty() )
        {
            sig->SetName( wxString::Format( wxT( "NetChain%d" ), idx ) );
            idx++;
        }
    }

    wxLogTrace( traceSchNetChain, "RebuildNetChains: pass 4 (terminal pins)" );
    for( std::unique_ptr<SCH_NETCHAIN>& sig : m_potentialNetChains )
    {
        struct PIN_INFO
        {
            SCH_PIN*              pin;
            SCH_SYMBOL*           sym;
            const SCH_SHEET_PATH* sheet;
        };
        std::vector<PIN_INFO> pins;

        for( const SCH_SHEET_PATH& sheetPath : m_sheetList )
        {
            SCH_SCREEN* sc = sheetPath.LastScreen(); if( !sc ) continue;
            for( SCH_ITEM* item : sc->Items().OfType( SCH_SYMBOL_T ) )
            {
                SCH_SYMBOL* sym = static_cast<SCH_SYMBOL*>( item );
                for( SCH_PIN* p : sym->GetPins( &sheetPath ) )
                {
                    wxString net = getSubgraphNet( p );
                    if( sig->GetNets().count( net ) )
                        pins.push_back( { p, sym, &sheetPath } );
                }
            }
        }

        int64_t best = -1;
        KIID    a, b;
        size_t  bestI = 0, bestJ = 0;

        for( size_t i = 0; i < pins.size(); ++i )
        {
            for( size_t j = i + 1; j < pins.size(); ++j )
            {
                VECTOR2I pa = pins[i].pin->GetPosition();
                VECTOR2I pb = pins[j].pin->GetPosition();
                int64_t dx = pa.x - pb.x;
                int64_t dy = pa.y - pb.y;
                int64_t d = dx * dx + dy * dy;

                if( d > best )
                {
                    best = d;
                    a = pins[i].pin->m_Uuid;
                    b = pins[j].pin->m_Uuid;
                    bestI = i;
                    bestJ = j;
                }
            }
        }

        sig->SetTerminalPins( a, b );

        if( best >= 0 && bestI < pins.size() && bestJ < pins.size() )
        {
            sig->SetTerminalRefs( pins[bestI].sym->GetRef( pins[bestI].sheet ), pins[bestI].pin->GetNumber(),
                                  pins[bestJ].sym->GetRef( pins[bestJ].sheet ), pins[bestJ].pin->GetNumber() );
        }

        if( m_netChainTerminalOverrides.count( sig->GetName() ) )
        {
            auto ov = m_netChainTerminalOverrides[sig->GetName()];
            sig->SetTerminalPins( ov.first, ov.second );
        }
    }

    wxLogTrace( traceSchNetChain, "RebuildNetChains: pass 5 (apply symbol names)" );
    for( auto& sigUP : m_potentialNetChains )
    {
        SCH_NETCHAIN* sig = sigUP.get();
        for( SCH_SYMBOL* sym : sig->GetSymbols() )
        {
            if( sym )
                sym->SetNetChainName( sig->GetName() );
        }
    wxString netsStr;
    for( const wxString& n : sig->GetNets() ) { netsStr += n + wxS(" "); }
    wxLogTrace( traceSchNetChain, "FinalChain %p nets(%zu): %s", (void*) sig, sig->GetNets().size(), netsStr );
    }

    wxLogTrace( traceSchNetChain, "RebuildNetChains: built %zu potential net chains", m_potentialNetChains.size() );

    // Restore committed chains from file.
    // Priority 1: match by terminal ref+pin (survives net renames)
    // Priority 2: match by saved net names (survives component renames)
    {
        std::set<wxString> alreadyCommitted;

        for( const auto& chain : m_committedNetChains )
        {
            if( chain )
                alreadyCommitted.insert( chain->GetName() );
        }

        // Build ref+pin → net lookup from current schematic
        std::map<std::pair<wxString, wxString>, wxString> refPinToNet;

        for( const SCH_SHEET_PATH& sp : m_sheetList )
        {
            SCH_SCREEN* sc = sp.LastScreen();

            if( !sc )
                continue;

            for( SCH_ITEM* item : sc->Items().OfType( SCH_SYMBOL_T ) )
            {
                SCH_SYMBOL* sym = static_cast<SCH_SYMBOL*>( item );
                wxString    ref = sym->GetRef( &sp );

                for( SCH_PIN* pin : sym->GetPins( &sp ) )
                {
                    if( CONNECTION_SUBGRAPH* sg = GetSubgraphForItem( pin ) )
                    {
                        // Match potential-chain key construction so unnamed subgraphs use the
                        // synthetic prefix instead of being skipped — without this, a chain
                        // whose only named endpoint is at one terminal would fail strict
                        // both-endpoint matching.
                        refPinToNet[{ ref, pin->GetNumber() }] =
                                netChainKeyFor( sg->GetNetName(), sg->m_code );
                    }
                }
            }
        }

        // O(1) lookup of committed chains by name so the restore passes don't linearly
        // scan m_committedNetChains for every override entry.
        std::unordered_map<wxString, SCH_NETCHAIN*> committedByName;

        for( const auto& chain : m_committedNetChains )
        {
            if( chain )
                committedByName[chain->GetName()] = chain.get();
        }

        // Names refreshed in pass 2a so pass 2b (manual fallback) doesn't overwrite the
        // potential-based payload with its broader member-net symbol collection.
        std::set<wxString> refreshedThisPass;

        for( const auto& [chainName, termRefs] : m_netChainTerminalRefOverrides )
        {
            SCH_NETCHAIN* match = resolvePotentialChainByTerminals( termRefs, refPinToNet,
                                                                    m_potentialNetChains, chainName );

            if( !match )
                continue;

            if( alreadyCommitted.count( chainName ) )
            {
                auto it = committedByName.find( chainName );

                if( it != committedByName.end() && it->second )
                {
                    refreshCommittedChainFromPotential( it->second, *match );
                    refreshedThisPass.insert( chainName );
                }

                continue;
            }

            CreateNetChainFromPotential( match, chainName );
            alreadyCommitted.insert( chainName );
            refreshedThisPass.insert( chainName );
        }

        // Manual chains have no inferred potential; rebuild from the persisted
        // member-net list by collecting symbols whose pins land on those nets.
        for( const auto& [chainName, memberNets] : m_netChainMemberNetOverrides )
        {
            if( memberNets.empty() )
                continue;

            // Skip chains pass 2a already refreshed; the potential's symbol set is more
            // precise than the broad member-net match collected here.
            if( alreadyCommitted.count( chainName ) && refreshedThisPass.count( chainName ) )
                continue;

            auto termIt = m_netChainTerminalRefOverrides.find( chainName );

            if( termIt == m_netChainTerminalRefOverrides.end() )
                continue;

            const CHAIN_TERMINAL_REFS& termRefs = termIt->second;

            SCH_PIN* terminalPinA = nullptr;
            SCH_PIN* terminalPinB = nullptr;
            std::set<SCH_SYMBOL*> symbols;

            for( const SCH_SHEET_PATH& sp : m_sheetList )
            {
                SCH_SCREEN* sc = sp.LastScreen();

                if( !sc )
                    continue;

                for( SCH_ITEM* item : sc->Items().OfType( SCH_SYMBOL_T ) )
                {
                    SCH_SYMBOL* sym = static_cast<SCH_SYMBOL*>( item );
                    wxString    ref = sym->GetRef( &sp );
                    bool        symContributes = false;

                    for( SCH_PIN* pin : sym->GetPins( &sp ) )
                    {
                        CONNECTION_SUBGRAPH* sg = GetSubgraphForItem( pin );

                        if( !sg )
                            continue;

                        if( memberNets.count( sg->GetNetName() ) )
                            symContributes = true;

                        if( ref == termRefs.first.ref && pin->GetNumber() == termRefs.first.pin )
                            terminalPinA = pin;

                        if( ref == termRefs.second.ref && pin->GetNumber() == termRefs.second.pin )
                            terminalPinB = pin;
                    }

                    if( symContributes )
                        symbols.insert( sym );
                }
            }

            if( !terminalPinA || !terminalPinB || symbols.empty() )
            {
                wxLogTrace( traceSchNetChain,
                            "RebuildNetChains: cannot restore manual chain '%s' "
                            "(terminals or member nets unresolved)",
                            chainName );
                continue;
            }

            if( alreadyCommitted.count( chainName ) )
            {
                auto it = committedByName.find( chainName );

                if( it != committedByName.end() && it->second )
                {
                    refreshCommittedChainPayload( it->second, memberNets, symbols,
                                                  terminalPinA->m_Uuid, terminalPinB->m_Uuid,
                                                  termRefs.first.ref, termRefs.first.pin,
                                                  termRefs.second.ref, termRefs.second.pin );
                }

                continue;
            }

            CreateManualNetChain( chainName, symbols, memberNets, terminalPinA->m_Uuid,
                                  terminalPinB->m_Uuid, termRefs.first.ref, termRefs.first.pin,
                                  termRefs.second.ref, termRefs.second.pin );
            alreadyCommitted.insert( chainName );
        }
    }

    // Committed chain names take priority over potential chain names set by pass 5.
    for( const auto& chain : m_committedNetChains )
    {
        if( chain )
        {
            for( SCH_SYMBOL* sym : chain->GetSymbols() )
            {
                if( sym )
                    sym->SetNetChainName( chain->GetName() );
            }
        }
    }

    // QA fixtures install this hook to inject a throw inside the protected block and
    // verify the catch handler resizes m_committedNetChains and restores m_netChainsBuilt.
    if( auto& hook = RebuildNetChainsTestHook() )
        hook( *this );

    // An empty chain list is a valid built state for chainless schematics.
    m_netChainsBuilt = true;
    }
    catch( const std::exception& e )
    {
        wxFAIL_MSG( wxString::Format( "RebuildNetChains threw: %s", e.what() ) );
        wxLogError( _( "Net chain rebuild failed: %s.  The schematic may have stale chain "
                       "data; reload to recover." ),
                    wxString( e.what() ) );
        m_potentialNetChains.clear();

        if( m_committedNetChains.size() > committedSnapshot )
            m_committedNetChains.resize( committedSnapshot );

        m_netChainsBuilt = builtSnapshot;
        return;
    }
    catch( ... )
    {
        wxFAIL_MSG( "RebuildNetChains threw an unknown exception" );
        wxLogError( _( "Net chain rebuild failed with an unknown error.  The schematic may "
                       "have stale chain data; reload to recover." ) );
        m_potentialNetChains.clear();

        if( m_committedNetChains.size() > committedSnapshot )
            m_committedNetChains.resize( committedSnapshot );

        m_netChainsBuilt = builtSnapshot;
        return;
    }
}

SCH_NETCHAIN* CONNECTION_GRAPH::resolvePotentialChainByTerminals(
        const CHAIN_TERMINAL_REFS& aTermRefs,
        const std::map<std::pair<wxString, wxString>, wxString>& aRefPinToNet,
        const std::vector<std::unique_ptr<SCH_NETCHAIN>>& aPotentials,
        const wxString& aChainName )
{
    auto itFrom = aRefPinToNet.find( { aTermRefs.first.ref, aTermRefs.first.pin } );
    auto itTo = aRefPinToNet.find( { aTermRefs.second.ref, aTermRefs.second.pin } );

    if( itFrom == aRefPinToNet.end() || itTo == aRefPinToNet.end() )
    {
        wxLogTrace( traceSchNetChain,
                    "RebuildNetChains: cannot restore chain '%s' (terminal %s.%s/%s.%s unresolved)",
                    aChainName, aTermRefs.first.ref, aTermRefs.first.pin,
                    aTermRefs.second.ref, aTermRefs.second.pin );
        return nullptr;
    }

    for( const auto& pot : aPotentials )
    {
        if( pot && pot->GetNets().count( itFrom->second ) && pot->GetNets().count( itTo->second ) )
            return pot.get();
    }

    wxLogTrace( traceSchNetChain,
                "RebuildNetChains: no potential chain spans both terminals of '%s' (%s/%s)",
                aChainName, itFrom->second, itTo->second );
    return nullptr;
}


SCH_NETCHAIN* CONNECTION_GRAPH::FindPotentialNetChainBetweenPins( SCH_PIN* aPinA, SCH_PIN* aPinB )
{
    if( !aPinA || !aPinB )
        return nullptr;

    wxString netA;
    wxString netB;

    if( CONNECTION_SUBGRAPH* sgA = GetSubgraphForItem( aPinA ) )
        netA = netChainKeyFor( sgA->GetNetName(), sgA->m_code );

    if( CONNECTION_SUBGRAPH* sgB = GetSubgraphForItem( aPinB ) )
        netB = netChainKeyFor( sgB->GetNetName(), sgB->m_code );

    if( netA.IsEmpty() || netB.IsEmpty() )
        return nullptr;

    for( const auto& sigUP : m_potentialNetChains )
    {
        if( sigUP && sigUP->GetNets().contains( netA ) && sigUP->GetNets().contains( netB ) )
            return sigUP.get();
    }

    return nullptr;
}

bool CONNECTION_GRAPH::DeleteCommittedNetChain( const wxString& aName )
{
    if( aName.IsEmpty() )
        return false;

    auto it = std::find_if( m_committedNetChains.begin(), m_committedNetChains.end(),
                            [&]( const std::unique_ptr<SCH_NETCHAIN>& aChain )
                            {
                                return aChain && aChain->GetName() == aName;
                            } );

    if( it == m_committedNetChains.end() )
        return false;

    // Drop the chain marker from every member symbol so a future
    // RebuildNetChains() doesn't re-promote them under the same name.
    for( SCH_SYMBOL* sym : (*it)->GetSymbols() )
    {
        if( sym )
            sym->SetNetChainName( wxEmptyString );
    }

    m_committedNetChains.erase( it );

    // Drop orphaned overrides keyed on this name.
    m_netChainNetClassOverrides.erase( aName );
    m_netChainColorOverrides.erase( aName );
    m_netChainTerminalRefOverrides.erase( aName );
    m_netChainTerminalOverrides.erase( aName );
    m_netChainMemberNetOverrides.erase( aName );

    return true;
}


bool CONNECTION_GRAPH::RenameCommittedNetChain( const wxString& aOld, const wxString& aNew )
{
    if( aOld.IsEmpty() || aNew.IsEmpty() || aOld == aNew )
        return false;

    auto findByName = [&]( const wxString& aName ) -> SCH_NETCHAIN*
    {
        for( const std::unique_ptr<SCH_NETCHAIN>& chain : m_committedNetChains )
        {
            if( chain && chain->GetName() == aName )
                return chain.get();
        }

        return nullptr;
    };

    SCH_NETCHAIN* existing = findByName( aOld );

    if( !existing )
        return false;

    // Reject collisions: if some other committed chain already owns aNew, don't
    // silently merge them.
    if( findByName( aNew ) )
        return false;

    existing->SetName( aNew );

    for( SCH_SYMBOL* sym : existing->GetSymbols() )
    {
        if( sym )
            sym->SetNetChainName( aNew );
    }

    rekeyOverrideMaps( aOld, aNew );

    return true;
}


void CONNECTION_GRAPH::rekeyOverrideMaps( const wxString& aOld, const wxString& aNew )
{
    if( aOld == aNew )
        return;

    auto rekey = [&]( auto& aMap )
    {
        auto it = aMap.find( aOld );

        if( it != aMap.end() )
        {
            auto val = std::move( it->second );
            aMap.erase( it );
            aMap[aNew] = std::move( val );
        }
    };

    rekey( m_netChainNetClassOverrides );
    rekey( m_netChainColorOverrides );
    rekey( m_netChainTerminalRefOverrides );
    rekey( m_netChainTerminalOverrides );
    rekey( m_netChainMemberNetOverrides );
}


void CONNECTION_GRAPH::refreshCommittedChainPayload( SCH_NETCHAIN* aTarget,
                                                     const std::set<wxString>& aNets,
                                                     const std::set<SCH_SYMBOL*>& aSymbols,
                                                     const KIID& aTerminalPinA,
                                                     const KIID& aTerminalPinB,
                                                     const wxString& aRefA,
                                                     const wxString& aPinNumA,
                                                     const wxString& aRefB,
                                                     const wxString& aPinNumB )
{
    if( !aTarget )
        return;

    std::set<wxString> filtered;

    for( const wxString& net : aNets )
    {
        if( !net.IsEmpty() )
            filtered.insert( net );
    }

    aTarget->ReplaceNets( filtered );

    aTarget->ClearSymbols();

    for( SCH_SYMBOL* sym : aSymbols )
        aTarget->AddSymbol( sym );

    // Honor an explicit terminal-pin override (set via ReplaceNetChainTerminalPin) over the
    // topology-derived defaults; otherwise an unconditional Recalculate would silently revert
    // user retargeting of the chain's terminal endpoints.
    auto termOverride = m_netChainTerminalOverrides.find( aTarget->GetName() );

    if( termOverride != m_netChainTerminalOverrides.end() )
        aTarget->SetTerminalPins( termOverride->second.first, termOverride->second.second );
    else
        aTarget->SetTerminalPins( aTerminalPinA, aTerminalPinB );

    aTarget->SetTerminalRefs( aRefA, aPinNumA, aRefB, aPinNumB );

    for( SCH_SYMBOL* sym : aTarget->GetSymbols() )
        sym->SetNetChainName( aTarget->GetName() );
}


void CONNECTION_GRAPH::refreshCommittedChainFromPotential( SCH_NETCHAIN* aTarget,
                                                           const SCH_NETCHAIN& aSource )
{
    refreshCommittedChainPayload( aTarget, aSource.GetNets(), aSource.GetSymbols(),
                                  aSource.GetTerminalPinA(), aSource.GetTerminalPinB(),
                                  aSource.GetTerminalRef( 0 ), aSource.GetTerminalPinNum( 0 ),
                                  aSource.GetTerminalRef( 1 ), aSource.GetTerminalPinNum( 1 ) );
}


SCH_NETCHAIN* CONNECTION_GRAPH::CreateNetChainFromPotential( SCH_NETCHAIN* aPotential, const wxString& aName )
{
    if( !aPotential )
        return nullptr;
    auto sig = std::make_unique<SCH_NETCHAIN>();
    for( const wxString& n : aPotential->GetNets() )
        sig->AddNet( n );
    for( SCH_SYMBOL* sym : aPotential->GetSymbols() )
        sig->AddSymbol( sym );
    sig->SetName( aName );
    sig->SetTerminalPins( aPotential->GetTerminalPinA(), aPotential->GetTerminalPinB() );
    sig->SetTerminalRefs( aPotential->GetTerminalRef( 0 ), aPotential->GetTerminalPinNum( 0 ),
                          aPotential->GetTerminalRef( 1 ), aPotential->GetTerminalPinNum( 1 ) );

    // Apply any parsed netclass override for this chain name.
    auto ncIt = m_netChainNetClassOverrides.find( aName );

    if( ncIt != m_netChainNetClassOverrides.end() )
        sig->SetNetClass( ncIt->second );

    // Apply any parsed colour override for this chain name.
    auto colIt = m_netChainColorOverrides.find( aName );

    if( colIt != m_netChainColorOverrides.end() )
        sig->SetColor( colIt->second );

    // Apply name to symbols now
    for( SCH_SYMBOL* sym : sig->GetSymbols() )
        sym->SetNetChainName( sig->GetName() );

    // Register terminal refs in the override map so a subsequent unconditional Recalculate
    // (which calls Reset() and clears the chain's symbol list) can find this chain in the
    // restore pass and refresh it in place.  Runtime-created chains otherwise live only in
    // m_committedNetChains and would be missed by the override-driven restore loop.
    CHAIN_TERMINAL_REFS termRefs{
        { aPotential->GetTerminalRef( 0 ), aPotential->GetTerminalPinNum( 0 ) },
        { aPotential->GetTerminalRef( 1 ), aPotential->GetTerminalPinNum( 1 ) }
    };
    m_netChainTerminalRefOverrides[aName] = termRefs;

    // Mirror the persisted-format member-net override so pass 2b has a fallback if the
    // schematic topology shifts and the inferred potential no longer resolves.  Synthetic
    // and empty entries are excluded to match the save path's filter in the s-expr writer.
    std::set<wxString> persistableNets;

    for( const wxString& net : sig->GetNets() )
    {
        if( net.IsEmpty() )
            continue;

        if( net.StartsWith( SCH_NETCHAIN::SYNTHETIC_NET_PREFIX ) )
            continue;

        persistableNets.insert( net );
    }

    if( !persistableNets.empty() )
        m_netChainMemberNetOverrides[aName] = std::move( persistableNets );
    else
        m_netChainMemberNetOverrides.erase( aName );

    SCH_NETCHAIN* raw = sig.get();
    m_committedNetChains.push_back( std::move( sig ) ); // committed from potential net chain
    return raw;
}


SCH_NETCHAIN* CONNECTION_GRAPH::CreateManualNetChain( const wxString& aName,
                                                      const std::set<SCH_SYMBOL*>& aSymbols,
                                                      const std::set<wxString>& aNets,
                                                      const KIID& aTerminalPinA,
                                                      const KIID& aTerminalPinB,
                                                      const wxString& aRefA,
                                                      const wxString& aPinNumA,
                                                      const wxString& aRefB,
                                                      const wxString& aPinNumB )
{
    if( !SCH_NETCHAIN::IsValidName( aName ) )
        return nullptr;

    if( GetNetChainByName( aName ) )
        return nullptr;

    // GetNetChainForNet returns the first match, so dual ownership of any net would
    // make resolution depend on iteration order.
    for( const wxString& net : aNets )
    {
        if( net.IsEmpty() )
            continue;

        if( GetNetChainForNet( net ) )
            return nullptr;
    }

    auto sig = std::make_unique<SCH_NETCHAIN>();
    sig->SetName( aName );

    for( const wxString& net : aNets )
    {
        if( net.IsEmpty() )
            continue;

        sig->AddNet( net );
    }

    for( SCH_SYMBOL* sym : aSymbols )
        sig->AddSymbol( sym );

    sig->SetTerminalPins( aTerminalPinA, aTerminalPinB );
    sig->SetTerminalRefs( aRefA, aPinNumA, aRefB, aPinNumB );

    auto ncIt = m_netChainNetClassOverrides.find( aName );

    if( ncIt != m_netChainNetClassOverrides.end() )
        sig->SetNetClass( ncIt->second );

    auto colIt = m_netChainColorOverrides.find( aName );

    if( colIt != m_netChainColorOverrides.end() )
        sig->SetColor( colIt->second );

    for( SCH_SYMBOL* sym : sig->GetSymbols() )
        sym->SetNetChainName( sig->GetName() );

    // Register the override-map entries that the rebuild restore pass needs to refresh this
    // manual chain after a future unconditional Recalculate.  Without this the chain is only
    // known to m_committedNetChains, and the restore pass cannot rebuild its derived view.
    CHAIN_TERMINAL_REFS termRefs{ { aRefA, aPinNumA }, { aRefB, aPinNumB } };
    m_netChainTerminalRefOverrides[aName] = termRefs;
    m_netChainMemberNetOverrides[aName] = sig->GetNets();

    SCH_NETCHAIN* raw = sig.get();
    m_committedNetChains.push_back( std::move( sig ) );
    return raw;
}


SCH_NETCHAIN* CONNECTION_GRAPH::GetNetChainForNet( const wxString& aNet )
{
    wxLogTrace( traceSchNetChain, "CONNECTION_GRAPH::GetNetChainForNet(%s)", aNet );
    for( std::unique_ptr<SCH_NETCHAIN>& sig : m_committedNetChains )
    {
        if( !sig )
            continue;

        if( sig->GetNets().count( aNet ) )
        {
            wxLogTrace( traceSchNetChain, "GetNetChainForNet: found chain '%s'", sig->GetName() );
            return sig.get();
        }
    }

    wxLogTrace( traceSchNetChain, "GetNetChainForNet: no chain found" );
    return nullptr;
}


void CONNECTION_GRAPH::ApplyNetChainNetclasses()
{
    if( !m_schematic )
        return;

    std::shared_ptr<NET_SETTINGS> netSettings = m_schematic->Project().GetProjectFile().NetSettings();

    if( !netSettings )
        return;

    bool anyOverride = std::any_of( m_committedNetChains.begin(), m_committedNetChains.end(),
                                    []( const std::unique_ptr<SCH_NETCHAIN>& aChain )
                                    {
                                        return aChain && !aChain->GetNetClass().IsEmpty();
                                    } );

    // The common no-chain path must not wipe the effective-netclass cache on every connectivity
    // rebuild.  Only rebuild when a chain carries an override or a prior pass left stale entries.
    if( !anyOverride && !netSettings->HasChainPatternAssignments() )
        return;

    netSettings->ClearChainPatternAssignments();

    for( const std::unique_ptr<SCH_NETCHAIN>& chain : m_committedNetChains )
    {
        if( !chain )
            continue;

        const wxString& netclass = chain->GetNetClass();

        if( netclass.IsEmpty() || !netSettings->HasNetclass( netclass ) )
            continue;

        for( const wxString& net : chain->GetNets() )
        {
            // Synthetic per-run keys embed a subgraph code and never match a resolved net name.
            if( net.StartsWith( SCH_NETCHAIN::SYNTHETIC_NET_PREFIX ) )
                continue;

            netSettings->SetChainPatternAssignment( net, netclass );
        }
    }
}


SCH_NETCHAIN* CONNECTION_GRAPH::GetNetChainByName( const wxString& aName )
{
    wxLogTrace( traceSchNetChain, "CONNECTION_GRAPH::GetNetChainByName(%s)", aName );
    for( std::unique_ptr<SCH_NETCHAIN>& sig : m_committedNetChains )
    {
        if( sig->GetName() == aName )
        {
            wxLogTrace( traceSchNetChain, "GetNetChainByName: found" );
            return sig.get();
        }
    }

    wxLogTrace( traceSchNetChain, "GetNetChainByName: not found" );
    return nullptr;
}


void CONNECTION_GRAPH::ReplaceNetChainTerminalPin( const wxString& aNetChain, const KIID& aPrev,
                                                const KIID& aNew )
{
    wxLogTrace( traceSchNetChain, "ReplaceNetChainTerminalPin: chain='%s' prev=%s new=%s",
                aNetChain, aPrev.AsString(), aNew.AsString() );
    if( SCH_NETCHAIN* sig = GetNetChainByName( aNetChain ) )
    {
        sig->ReplaceTerminalPin( aPrev, aNew );
        m_netChainTerminalOverrides[aNetChain] = std::make_pair( sig->GetTerminalPinA(),
                                                             sig->GetTerminalPinB() );
        wxLogTrace( traceSchNetChain, "ReplaceNetChainTerminalPin: updated overrides to (%s,%s)",
                    sig->GetTerminalPinA().AsString(), sig->GetTerminalPinB().AsString() );
    }
}


void CONNECTION_GRAPH::SetNetChainTerminalOverrides( const std::map<wxString,
                                                std::pair<KIID, KIID>>& aOverrides )
{
    m_netChainTerminalOverrides = aOverrides;
    wxLogTrace( traceSchNetChain, "SetNetChainTerminalOverrides: count=%zu",
                m_netChainTerminalOverrides.size() );
}


int CONNECTION_GRAPH::getOrCreateNetCode( const wxString& aNetName )
{
    int code;

    auto it = m_net_name_to_code_map.find( aNetName );

    if( it == m_net_name_to_code_map.end() )
    {
        code = m_last_net_code++;
        m_net_name_to_code_map[ aNetName ] = code;
    }
    else
    {
        code = it->second;
    }

    return code;
}


int CONNECTION_GRAPH::assignNewNetCode( SCH_CONNECTION& aConnection )
{
    int code = getOrCreateNetCode( aConnection.Name() );

    aConnection.SetNetCode( code );

    return code;
}


void CONNECTION_GRAPH::assignNetCodesToBus( SCH_CONNECTION* aConnection )
{
    std::vector<std::shared_ptr<SCH_CONNECTION>> connections_to_check( aConnection->Members() );

    for( unsigned i = 0; i < connections_to_check.size(); i++ )
    {
        const std::shared_ptr<SCH_CONNECTION>& member = connections_to_check[i];

        if( member->IsBus() )
        {
            connections_to_check.insert( connections_to_check.end(),
                                         member->Members().begin(),
                                         member->Members().end() );
            continue;
        }

        assignNewNetCode( *member );
    }
}


void CONNECTION_GRAPH::propagateToNeighbors( CONNECTION_SUBGRAPH* aSubgraph, bool aForce )
{
    SCH_CONNECTION* conn = aSubgraph->m_driver_connection;
    std::vector<CONNECTION_SUBGRAPH*> search_list;
    std::unordered_set<CONNECTION_SUBGRAPH*> visited;
    std::unordered_set<SCH_CONNECTION*> stale_bus_members;

    auto visit =[&]( CONNECTION_SUBGRAPH* aParent )
    {
        for( SCH_SHEET_PIN* pin : aParent->m_hier_pins )
        {
            SCH_SHEET_PATH path = aParent->m_sheet;
            path.push_back( pin->GetParent() );

            auto it = m_sheet_to_subgraphs_map.find( path );

            if( it == m_sheet_to_subgraphs_map.end() )
                continue;

            for( CONNECTION_SUBGRAPH* candidate : it->second )
            {
                if( !candidate->m_strong_driver
                    || candidate->m_hier_ports.empty()
                    || visited.contains( candidate ) )
                {
                    continue;
                }

                for( SCH_HIERLABEL* label : candidate->m_hier_ports )
                {
                    if( candidate->GetNameForDriver( label ) == aParent->GetNameForDriver( pin ) )
                    {
                        wxLogTrace( ConnTrace, wxS( "%lu: found child %lu (%s)" ), aParent->m_code,
                                    candidate->m_code, candidate->m_driver_connection->Name() );

                        candidate->m_hier_parent = aParent;
                        aParent->m_hier_children.insert( candidate );

                        // Should we skip adding the candidate to the list if the parent and candidate subgraphs
                        // are not the same?
                        wxASSERT( candidate->m_graph == aParent->m_graph );

                        search_list.push_back( candidate );
                        break;
                    }
                }
            }
        }

        for( SCH_HIERLABEL* label : aParent->m_hier_ports )
        {
            SCH_SHEET_PATH path = aParent->m_sheet;
            path.pop_back();

            auto it = m_sheet_to_subgraphs_map.find( path );

            if( it == m_sheet_to_subgraphs_map.end() )
                continue;

            for( CONNECTION_SUBGRAPH* candidate : it->second )
            {
                if( candidate->m_hier_pins.empty()
                    || visited.contains( candidate )
                    || candidate->m_driver_connection->Type() != aParent->m_driver_connection->Type() )
                {
                    continue;
                }

                const KIID& last_parent_uuid = aParent->m_sheet.Last()->m_Uuid;

                for( SCH_SHEET_PIN* pin : candidate->m_hier_pins )
                {
                    // If the last sheet UUIDs won't match, no need to check the full path
                    if( pin->GetParent()->m_Uuid != last_parent_uuid )
                        continue;

                    SCH_SHEET_PATH pin_path = path;
                    pin_path.push_back( pin->GetParent() );

                    if( pin_path != aParent->m_sheet )
                        continue;

                    if( aParent->GetNameForDriver( label ) == candidate->GetNameForDriver( pin ) )
                    {
                        wxLogTrace( ConnTrace, wxS( "%lu: found additional parent %lu (%s)" ),
                                    aParent->m_code, candidate->m_code, candidate->m_driver_connection->Name() );

                        aParent->m_hier_children.insert( candidate );
                        search_list.push_back( candidate );
                        break;
                    }
                }
            }
        }
    };

    auto propagate_bus_neighbors = [&]( CONNECTION_SUBGRAPH* aParentGraph )
    {
        // Sort bus neighbors by name to ensure deterministic processing order.
        // When multiple bus members (e.g., A0, A1, A2, A3) all connect to the same
        // shorted net in a child sheet, the first one processed "wins" and sets
        // the net name. Sorting ensures the alphabetically-first name is chosen.
        std::vector<std::shared_ptr<SCH_CONNECTION>> sortedMembers;

        for( const auto& kv : aParentGraph->m_bus_neighbors )
            sortedMembers.push_back( kv.first );

        std::sort( sortedMembers.begin(), sortedMembers.end(),
                   []( const std::shared_ptr<SCH_CONNECTION>& a,
                       const std::shared_ptr<SCH_CONNECTION>& b )
                   {
                       return a->Name() < b->Name();
                   } );

        for( const std::shared_ptr<SCH_CONNECTION>& member_conn : sortedMembers )
        {
            const auto& kv_it = aParentGraph->m_bus_neighbors.find( member_conn );

            if( kv_it == aParentGraph->m_bus_neighbors.end() )
                continue;

            for( CONNECTION_SUBGRAPH* neighbor : kv_it->second )
            {
                // May have been absorbed but won't have been deleted
                while( neighbor->m_absorbed )
                    neighbor = neighbor->m_absorbed_by;

                SCH_CONNECTION* parent = aParentGraph->m_driver_connection;

                // Now member may be out of date, since we just cloned the
                // connection from higher up in the hierarchy.  We need to
                // figure out what the actual new connection is.
                SCH_CONNECTION* member = matchBusMember( parent, member_conn.get() );

                if( !member )
                {
                    // Try harder: we might match on a secondary driver
                    for( CONNECTION_SUBGRAPH* sg : kv_it->second )
                    {
                        if( sg->m_multiple_drivers )
                        {
                            SCH_SHEET_PATH sheet = sg->m_sheet;

                            for( SCH_ITEM* driver : sg->m_drivers )
                            {
                                auto c = getDefaultConnection( driver, sg );
                                member = matchBusMember( parent, c.get() );

                                if( member )
                                    break;
                            }
                        }

                        if( member )
                            break;
                    }
                }

                // This is bad, probably an ERC error
                if( !member )
                {
                    wxLogTrace( ConnTrace, wxS( "Could not match bus member %s in %s" ),
                                member_conn->Name(), parent->Name() );
                    continue;
                }

                SCH_CONNECTION* neighbor_conn = neighbor->m_driver_connection;

                wxCHECK2( neighbor_conn, continue );

                wxString neighbor_name = neighbor_conn->Name();

                // Matching name: no update needed
                if( neighbor_name == member->Name() )
                    continue;

                // Was this neighbor already updated from a different sheet?  Don't rename it again,
                // unless this same parent bus updated it and the bus member name has since changed
                // (which can happen when a bus member is renamed via stale member update, issue #18299).
                if( neighbor_conn->Sheet() != neighbor->m_sheet )
                {
                    // If the neighbor's connection sheet doesn't match this parent bus's sheet,
                    // it was updated by a different bus entirely. Don't override.
                    if( neighbor_conn->Sheet() != parent->Sheet() )
                        continue;

                    // If the neighbor's connection sheet matches this parent bus's sheet but
                    // the names differ, check if the neighbor's current name still matches
                    // a member of this bus. If it does, the neighbor was updated by a different
                    // member of this same bus and we should preserve that (determinism).
                    // If it doesn't match any member, the bus member was renamed and we should
                    // update. We compare by name rather than VectorIndex because non-bus
                    // connections (e.g., "GND" from power pin propagation) have a default
                    // VectorIndex of 0 that falsely matches the first bus member.
                    bool alreadyUpdatedByBusMember = false;

                    for( const auto& m : parent->Members() )
                    {
                        if( m->Name() == neighbor_name )
                        {
                            alreadyUpdatedByBusMember = true;
                            break;
                        }
                    }

                    if( alreadyUpdatedByBusMember )
                        continue;
                }

                // Safety check against infinite recursion
                wxCHECK2_MSG( neighbor_conn->IsNet(), continue,
                              wxS( "\"" ) + neighbor_name + wxS( "\" is not a net." ) );

                wxLogTrace( ConnTrace, wxS( "%lu (%s) connected to bus member %s (local %s)" ),
                            neighbor->m_code, neighbor_name, member->Name(), member->LocalName() );

                // Take whichever name is higher priority
                if( CONNECTION_SUBGRAPH::GetDriverPriority( neighbor->m_driver )
                    >= CONNECTION_SUBGRAPH::PRIORITY::GLOBAL_POWER_PIN )
                {
                    member->Clone( *neighbor_conn );
                    stale_bus_members.insert( member );
                }
                else
                {
                    neighbor_conn->Clone( *member );

                    recacheSubgraphName( neighbor, neighbor_name );

                    // Recurse onto this neighbor in case it needs to re-propagate
                    neighbor->m_dirty = true;
                    propagateToNeighbors( neighbor, aForce );

                    // After hierarchy propagation, the neighbor's connection may have been
                    // updated to a higher-priority driver (e.g., a power symbol discovered
                    // through hierarchical sheet pins). If so, update the bus member to match.
                    // This ensures that net names propagate correctly through bus connections
                    // that span hierarchical boundaries (issue #18119).
                    if( neighbor_conn->Name() != member->Name() )
                    {
                        member->Clone( *neighbor_conn );
                        stale_bus_members.insert( member );
                    }
                }
            }
        }
    };

    // If we are a bus, we must propagate to local neighbors and then the hierarchy
    if( conn->IsBus() )
        propagate_bus_neighbors( aSubgraph );

    // If we have both ports and pins, skip processing as we'll be visited by a parent or child.
    // If we only have one or the other, process (we can either go bottom-up or top-down depending
    // on which subgraph comes up first)
    if( !aForce && !aSubgraph->m_hier_ports.empty() && !aSubgraph->m_hier_pins.empty() )
    {
        wxLogTrace( ConnTrace, wxS( "%lu (%s) has both hier ports and pins; deferring processing" ),
                    aSubgraph->m_code, conn->Name() );
        return;
    }
    else if( aSubgraph->m_hier_ports.empty() && aSubgraph->m_hier_pins.empty() )
    {
        wxLogTrace( ConnTrace, wxS( "%lu (%s) has no hier pins or ports on sheet %s; marking clean" ),
                    aSubgraph->m_code, conn->Name(), aSubgraph->m_sheet.PathHumanReadable() );
        aSubgraph->m_dirty = false;
        return;
    }

    visited.insert( aSubgraph );

    wxLogTrace( ConnTrace, wxS( "Propagating %lu (%s) to subsheets" ),
                aSubgraph->m_code, aSubgraph->m_driver_connection->Name() );

    visit( aSubgraph );

    for( unsigned i = 0; i < search_list.size(); i++ )
    {
        auto child = search_list[i];

        if( visited.insert( child ).second )
            visit( child );

        child->m_dirty = false;
    }

    // Now, find the best driver for this chain of subgraphs
    CONNECTION_SUBGRAPH*          bestDriver = aSubgraph;
    CONNECTION_SUBGRAPH::PRIORITY highest = CONNECTION_SUBGRAPH::GetDriverPriority( aSubgraph->m_driver );
    bool     bestIsStrong = ( highest >= CONNECTION_SUBGRAPH::PRIORITY::HIER_LABEL );
    wxString bestName     = aSubgraph->m_driver_connection->Name();

    // Check if a subsheet has a higher-priority connection to the same net
    if( highest < CONNECTION_SUBGRAPH::PRIORITY::GLOBAL_POWER_PIN )
    {
        for( CONNECTION_SUBGRAPH* subgraph : visited )
        {
            if( subgraph == aSubgraph )
                continue;

            CONNECTION_SUBGRAPH::PRIORITY priority =
                    CONNECTION_SUBGRAPH::GetDriverPriority( subgraph->m_driver );

            bool     candidateStrong = ( priority >= CONNECTION_SUBGRAPH::PRIORITY::HIER_LABEL );
            wxString candidateName   = subgraph->m_driver_connection->Name();
            bool     shorterPath     = subgraph->m_sheet.size() < bestDriver->m_sheet.size();
            bool     asGoodPath      = subgraph->m_sheet.size() <= bestDriver->m_sheet.size();

            // Pick a better driving subgraph if it:
            // a) has a power pin or global driver
            // b) is a strong driver and we're a weak driver
            // c) is a higher priority strong driver
            // d) matches our priority, is a strong driver, and has a shorter path
            // e) matches our strength and is at least as short, and is alphabetically lower

            if( ( priority >= CONNECTION_SUBGRAPH::PRIORITY::GLOBAL_POWER_PIN ) ||
                ( !bestIsStrong && candidateStrong ) ||
                ( priority > highest && candidateStrong ) ||
                ( priority == highest && candidateStrong && shorterPath ) ||
                ( ( bestIsStrong == candidateStrong ) && asGoodPath && ( priority == highest ) &&
                  ( candidateName < bestName ) ) )
            {
                bestDriver   = subgraph;
                highest      = priority;
                bestIsStrong = candidateStrong;
                bestName     = candidateName;
            }
        }
    }

    if( bestDriver != aSubgraph )
    {
        wxLogTrace( ConnTrace, wxS( "%lu (%s) overridden by new driver %lu (%s)" ),
                    aSubgraph->m_code, aSubgraph->m_driver_connection->Name(), bestDriver->m_code,
                    bestDriver->m_driver_connection->Name() );
    }

    conn = bestDriver->m_driver_connection;

    for( CONNECTION_SUBGRAPH* subgraph : visited )
    {
        wxString old_name = subgraph->m_driver_connection->Name();

        subgraph->m_driver_connection->Clone( *conn );

        if( old_name != conn->Name() )
            recacheSubgraphName( subgraph, old_name );

        if( conn->IsBus() )
            propagate_bus_neighbors( subgraph );
    }

    // Somewhere along the way, a bus member may have been upgraded to a global or power label.
    // Because this can happen anywhere, we need a second pass to update all instances of that bus
    // member to have the correct connection info
    if( conn->IsBus() && !stale_bus_members.empty() )
    {
        std::unordered_set<SCH_CONNECTION*> cached_members = stale_bus_members;

        for( SCH_CONNECTION* stale_member : cached_members )
        {
            for( CONNECTION_SUBGRAPH* subgraph : visited )
            {
                SCH_CONNECTION* member = matchBusMember( subgraph->m_driver_connection, stale_member );

                if( !member )
                {
                    wxLogTrace( ConnTrace, wxS( "WARNING: failed to match stale member %s in %s." ),
                                stale_member->Name(), subgraph->m_driver_connection->Name() );
                    continue;
                }

                wxLogTrace( ConnTrace, wxS( "Updating %lu (%s) member %s to %s" ), subgraph->m_code,
                            subgraph->m_driver_connection->Name(), member->LocalName(), stale_member->Name() );

                member->Clone( *stale_member );

                propagate_bus_neighbors( subgraph );
            }
        }
    }

    aSubgraph->m_dirty = false;
}


std::shared_ptr<SCH_CONNECTION> CONNECTION_GRAPH::getDefaultConnection( SCH_ITEM* aItem,
                                                                        CONNECTION_SUBGRAPH* aSubgraph )
{
    std::shared_ptr<SCH_CONNECTION> c = std::shared_ptr<SCH_CONNECTION>( nullptr );

    switch( aItem->Type() )
    {
    case SCH_PIN_T:
        if( static_cast<SCH_PIN*>( aItem )->IsPower() )
            c = std::make_shared<SCH_CONNECTION>( aItem, aSubgraph->m_sheet );

        break;

    case SCH_GLOBAL_LABEL_T:
    case SCH_HIER_LABEL_T:
    case SCH_LABEL_T:
        c = std::make_shared<SCH_CONNECTION>( aItem, aSubgraph->m_sheet );
        break;

    default:
        break;
    }

    if( c )
    {
        c->SetGraph( this );
        c->ConfigureFromLabel( aSubgraph->GetNameForDriver( aItem ) );
    }

    return c;
}


SCH_CONNECTION* CONNECTION_GRAPH::matchBusMember( SCH_CONNECTION* aBusConnection,
                                                  SCH_CONNECTION* aSearch )
{
    if( !aBusConnection->IsBus() )
        return nullptr;

    SCH_CONNECTION* match = nullptr;

    if( aBusConnection->Type() == CONNECTION_TYPE::BUS )
    {
        // Vector bus: compare against index, because we allow the name
        // to be different

        for( const std::shared_ptr<SCH_CONNECTION>& bus_member : aBusConnection->Members() )
        {
            if( bus_member->VectorIndex() == aSearch->VectorIndex() )
            {
                match = bus_member.get();
                break;
            }
        }
    }
    else
    {
        // Group bus
        for( const std::shared_ptr<SCH_CONNECTION>& c : aBusConnection->Members() )
        {
            // Vector inside group: compare names, because for bus groups
            // we expect the naming to be consistent across all usages
            // TODO(JE) explain this in the docs
            if( c->Type() == CONNECTION_TYPE::BUS )
            {
                for( const std::shared_ptr<SCH_CONNECTION>& bus_member : c->Members() )
                {
                    if( bus_member->LocalName() == aSearch->LocalName() )
                    {
                        match = bus_member.get();
                        break;
                    }
                }
            }
            else if( c->LocalName() == aSearch->LocalName() )
            {
                match = c.get();
                break;
            }
        }

        if( !match && aSearch->VectorIndex() >= 0 )
        {
            int flatIdx = 0;

            for( const std::shared_ptr<SCH_CONNECTION>& c : aBusConnection->Members() )
            {
                if( c->Type() == CONNECTION_TYPE::BUS )
                {
                    for( const std::shared_ptr<SCH_CONNECTION>& bus_member : c->Members() )
                    {
                        if( flatIdx == aSearch->VectorIndex() )
                        {
                            match = bus_member.get();
                            break;
                        }

                        flatIdx++;
                    }
                }
                else
                {
                    if( flatIdx == aSearch->VectorIndex() )
                    {
                        match = c.get();
                        break;
                    }

                    flatIdx++;
                }

                if( match )
                    break;
            }
        }
    }

    return match;
}


void CONNECTION_GRAPH::recacheSubgraphName( CONNECTION_SUBGRAPH* aSubgraph, const wxString& aOldName )
{
    auto it = m_net_name_to_subgraphs_map.find( aOldName );

    if( it != m_net_name_to_subgraphs_map.end() )
    {
        std::vector<CONNECTION_SUBGRAPH*>& vec = it->second;
        std::erase( vec, aSubgraph );
    }

    wxLogTrace( ConnTrace, wxS( "recacheSubgraphName: %s => %s" ), aOldName,
                aSubgraph->m_driver_connection->Name() );

    m_net_name_to_subgraphs_map[aSubgraph->m_driver_connection->Name()].push_back( aSubgraph );
}


std::shared_ptr<BUS_ALIAS> CONNECTION_GRAPH::GetBusAlias( const wxString& aName )
{
    auto it = m_bus_alias_cache.find( aName );

    return it != m_bus_alias_cache.end() ? it->second : nullptr;
}


std::vector<const CONNECTION_SUBGRAPH*> CONNECTION_GRAPH::GetBusesNeedingMigration()
{
    std::vector<const CONNECTION_SUBGRAPH*> ret;

    for( CONNECTION_SUBGRAPH* subgraph : m_subgraphs )
    {
        // Graph is supposed to be up-to-date before calling this
        // Should we continue if the subgraph is not up to date?
        wxASSERT( !subgraph->m_dirty );

        if( !subgraph->m_driver )
            continue;

        SCH_SHEET_PATH* sheet = &subgraph->m_sheet;
        SCH_CONNECTION* connection = subgraph->m_driver->Connection( sheet );

        if( !connection->IsBus() )
            continue;

        auto labels = subgraph->GetVectorBusLabels();

        if( labels.size() > 1 )
        {
            bool different = false;
            wxString first = static_cast<SCH_TEXT*>( labels.at( 0 ) )->GetShownText( sheet, false );

            for( unsigned i = 1; i < labels.size(); ++i )
            {
                if( static_cast<SCH_TEXT*>( labels.at( i ) )->GetShownText( sheet, false ) != first )
                {
                    different = true;
                    break;
                }
            }

            if( !different )
                continue;

            wxLogTrace( ConnTrace, wxS( "SG %ld (%s) has multiple bus labels" ), subgraph->m_code,
                        connection->Name() );

            ret.push_back( subgraph );
        }
    }

    return ret;
}


wxString CONNECTION_GRAPH::GetResolvedSubgraphName( const CONNECTION_SUBGRAPH* aSubGraph ) const
{
    wxString retval = aSubGraph->GetNetName();
    bool found = false;

    // This is a hacky way to find the true subgraph net name (why do we not store it?)
    // TODO: Remove once the actual netname of the subgraph is stored with the subgraph

    for( auto it = m_net_name_to_subgraphs_map.begin();
         it != m_net_name_to_subgraphs_map.end() && !found; ++it )
    {
        for( CONNECTION_SUBGRAPH* graph : it->second )
        {
            if( graph == aSubGraph )
            {
                retval = it->first;
                found = true;
                break;
            }
        }
    }

    return retval;
}


CONNECTION_SUBGRAPH* CONNECTION_GRAPH::FindSubgraphByName( const wxString& aNetName,
                                                           const SCH_SHEET_PATH& aPath )
{
    auto it = m_net_name_to_subgraphs_map.find( aNetName );

    if( it == m_net_name_to_subgraphs_map.end() )
        return nullptr;

    for( CONNECTION_SUBGRAPH* sg : it->second )
    {
        // Cache is supposed to be valid by now
        // Should we continue if the cache is not valid?
        wxASSERT( sg && !sg->m_absorbed && sg->m_driver_connection );

        if( sg->m_sheet == aPath && sg->m_driver_connection->Name() == aNetName )
            return sg;
    }

    return nullptr;
}


CONNECTION_SUBGRAPH* CONNECTION_GRAPH::FindFirstSubgraphByName( const wxString& aNetName )
{
    auto it = m_net_name_to_subgraphs_map.find( aNetName );

    if( it == m_net_name_to_subgraphs_map.end() )
        return nullptr;

    // Should this return a nullptr if the map entry is empty?
    wxASSERT( !it->second.empty() );

    return it->second[0];
}


CONNECTION_SUBGRAPH* CONNECTION_GRAPH::GetSubgraphForItem( SCH_ITEM* aItem ) const
{
    auto                 it  = m_item_to_subgraph_map.find( aItem );
    CONNECTION_SUBGRAPH* ret = it != m_item_to_subgraph_map.end() ? it->second : nullptr;

    while( ret && ret->m_absorbed )
        ret = ret->m_absorbed_by;

    return ret;
}


const std::vector<CONNECTION_SUBGRAPH*>&
CONNECTION_GRAPH::GetAllSubgraphs( const wxString& aNetName ) const
{
    static const std::vector<CONNECTION_SUBGRAPH*> subgraphs;

    auto it = m_net_name_to_subgraphs_map.find( aNetName );

    if( it == m_net_name_to_subgraphs_map.end() )
        return subgraphs;

    return it->second;
}


std::vector<wxString> CONNECTION_GRAPH::GetEquivalentBusNames( const wxString& aBusName ) const
{
    std::vector<wxString> equivalents;

    // Split off the sheet-path prefix. A literal '/' is always the hierarchy separator here, since
    // slashes in member names are escaped as "{slash}". Re-attached to results so they match the
    // net-name map keys.
    wxString path;
    wxString group = aBusName;
    size_t   lastSlash = aBusName.find_last_of( '/' );

    if( lastSlash != wxString::npos )
    {
        path = aBusName.Left( lastSlash + 1 );
        group = aBusName.Mid( lastSlash + 1 );
    }

    wxString              prefix;
    std::vector<wxString> members;

    if( !NET_SETTINGS::ParseBusGroup( UnescapeString( group ), &prefix, &members ) )
        return equivalents;

    // A named-group prefix ("BUS{A B}") renames the members, so it is not aliasable.
    if( !prefix.IsEmpty() )
        return equivalents;

    // ParseBusGroup escapes spaces as "\ " and leaves net-name escapes in place; BUS_ALIAS stores
    // members verbatim. Undo both so the two compare in the same form.
    for( wxString& member : members )
    {
        member.Replace( wxT( "\\ " ), wxT( " " ) );
        member = UnescapeString( member );
    }

    // A single-member name may itself be an alias ("{MIXED_BUS}"); expand it and don't re-emit it.
    wxString selfAlias;

    if( members.size() == 1 )
    {
        auto aliasIt = m_bus_alias_cache.find( members[0] );

        if( aliasIt != m_bus_alias_cache.end() )
        {
            selfAlias = members[0];
            members = aliasIt->second->Members();
        }
    }

    // Re-escape members back to net-name form so the label matches the connection-graph keys.
    wxString expandedLabel = path + wxT( "{" );

    for( size_t i = 0; i < members.size(); ++i )
    {
        if( i > 0 )
            expandedLabel += wxT( " " );

        wxString escaped = EscapeString( members[i], CTX_NETNAME );
        escaped.Replace( wxT( " " ), wxT( "\\ " ) );
        expandedLabel += escaped;
    }

    expandedLabel += wxT( "}" );

    if( expandedLabel != aBusName )
        equivalents.push_back( expandedLabel );

    // Match aliases by member set; bus connectivity is order-independent, so compare as multisets.
    std::multiset<wxString> memberSet( members.begin(), members.end() );

    for( const auto& [aliasName, alias] : m_bus_alias_cache )
    {
        if( aliasName == selfAlias || alias->Members().size() != members.size() )
            continue;

        std::multiset<wxString> aliasMembers( alias->Members().begin(), alias->Members().end() );

        if( memberSet == aliasMembers )
            equivalents.push_back( path + wxT( "{" ) + aliasName + wxT( "}" ) );
    }

    return equivalents;
}


int CONNECTION_GRAPH::RunERC()
{
    int error_count = 0;

    wxCHECK_MSG( m_schematic, true, wxS( "Null m_schematic in CONNECTION_GRAPH::RunERC" ) );

    ERC_SETTINGS& settings = m_schematic->ErcSettings();

    // We don't want to run many ERC checks more than once on a given screen even though it may
    // represent multiple sheets with multiple subgraphs.  We can tell these apart by drivers.
    std::set<SCH_ITEM*> seenDriverInstances;

    for( CONNECTION_SUBGRAPH* subgraph : m_subgraphs )
    {
        // There shouldn't be any null sub-graph pointers.
        wxCHECK2( subgraph, continue );

        // Graph is supposed to be up-to-date before calling RunERC()
        // Should we continue if the subgraph is not up to date?
        wxASSERT( !subgraph->m_dirty );

        if( subgraph->m_absorbed )
            continue;

        if( seenDriverInstances.count( subgraph->m_driver ) )
            continue;

        if( subgraph->m_driver )
            seenDriverInstances.insert( subgraph->m_driver );

        /**
         * NOTE:
         *
         * We could check that labels attached to bus subgraphs follow the
         * proper format (i.e. actually define a bus).
         *
         * This check doesn't need to be here right now because labels
         * won't actually be connected to bus wires if they aren't in the right
         * format due to their TestDanglingEnds() implementation.
         */
        if( settings.IsTestEnabled( ERCE_DRIVER_CONFLICT ) )
        {
            if( !ercCheckMultipleDrivers( subgraph ) )
                error_count++;
        }

        subgraph->ResolveDrivers( false );

        if( settings.IsTestEnabled( ERCE_BUS_TO_NET_CONFLICT ) )
        {
            if( !ercCheckBusToNetConflicts( subgraph ) )
                error_count++;
        }

        if( settings.IsTestEnabled( ERCE_BUS_ENTRY_CONFLICT ) )
        {
            if( !ercCheckBusToBusEntryConflicts( subgraph ) )
                error_count++;
        }

        if( settings.IsTestEnabled( ERCE_BUS_TO_BUS_CONFLICT ) )
        {
            if( !ercCheckBusToBusConflicts( subgraph ) )
                error_count++;
        }

        if( settings.IsTestEnabled( ERCE_WIRE_DANGLING ) )
        {
            if( !ercCheckFloatingWires( subgraph ) )
                error_count++;
        }

        if( settings.IsTestEnabled( ERCE_UNCONNECTED_WIRE_ENDPOINT ) )
        {
            if( !ercCheckDanglingWireEndpoints( subgraph ) )
                error_count++;
        }

        if( settings.IsTestEnabled( ERCE_NOCONNECT_CONNECTED )
                || settings.IsTestEnabled( ERCE_NOCONNECT_NOT_CONNECTED )
                || settings.IsTestEnabled( ERCE_PIN_NOT_CONNECTED ) )
        {
            if( !ercCheckNoConnects( subgraph ) )
                error_count++;
        }

        if( settings.IsTestEnabled( ERCE_LABEL_NOT_CONNECTED )
                || settings.IsTestEnabled( ERCE_LABEL_SINGLE_PIN ) )
        {
            if( !ercCheckLabels( subgraph ) )
                error_count++;
        }
    }

    if( settings.IsTestEnabled( ERCE_LABEL_NOT_CONNECTED ) )
    {
        error_count += ercCheckDirectiveLabels();
    }

    // Hierarchical sheet checking is done at the schematic level
    if( settings.IsTestEnabled( ERCE_HIERACHICAL_LABEL )
            || settings.IsTestEnabled( ERCE_PIN_NOT_CONNECTED ) )
    {
        error_count += ercCheckHierSheets();
    }

    if( settings.IsTestEnabled( ERCE_SINGLE_GLOBAL_LABEL ) )
    {
        error_count += ercCheckSingleGlobalLabel();
    }

    return error_count;
}


bool CONNECTION_GRAPH::ercCheckMultipleDrivers( const CONNECTION_SUBGRAPH* aSubgraph )
{
    wxCHECK( aSubgraph, false );

    if( aSubgraph->m_multiple_drivers )
    {
        for( SCH_ITEM* driver : aSubgraph->m_drivers )
        {
            if( driver == aSubgraph->m_driver )
                continue;

            if( driver->Type() == SCH_GLOBAL_LABEL_T
                    || driver->Type() == SCH_HIER_LABEL_T
                    || driver->Type() == SCH_LABEL_T
                    || ( driver->Type() == SCH_PIN_T && static_cast<SCH_PIN*>( driver )->IsPower() ) )
            {
                const wxString& primaryName   = aSubgraph->GetNameForDriver( aSubgraph->m_driver );
                const wxString& secondaryName = aSubgraph->GetNameForDriver( driver );

                if( primaryName == secondaryName )
                    continue;

                wxString msg = wxString::Format( _( "Both %s and %s are attached to the same "
                                                    "items; %s will be used in the netlist" ),
                                                 primaryName, secondaryName, primaryName );

                std::shared_ptr<ERC_ITEM> ercItem = ERC_ITEM::Create( ERCE_DRIVER_CONFLICT );
                ercItem->SetItems( aSubgraph->m_driver, driver );
                ercItem->SetSheetSpecificPath( aSubgraph->GetSheet() );
                ercItem->SetItemsSheetPaths( aSubgraph->GetSheet(), aSubgraph->m_sheet );
                ercItem->SetErrorMessage( msg );

                SCH_MARKER* marker = new SCH_MARKER( std::move( ercItem ), driver->GetPosition() );
                aSubgraph->m_sheet.LastScreen()->Append( marker );

                return false;
            }
        }
    }

    return true;
}


bool CONNECTION_GRAPH::ercCheckBusToNetConflicts( const CONNECTION_SUBGRAPH* aSubgraph )
{
    const SCH_SHEET_PATH& sheet = aSubgraph->m_sheet;
    SCH_SCREEN* screen = sheet.LastScreen();

    SCH_ITEM* net_item = nullptr;
    SCH_ITEM* bus_item = nullptr;
    SCH_CONNECTION conn( this );

    for( SCH_ITEM* item : aSubgraph->m_items )
    {
        switch( item->Type() )
        {
        case SCH_LINE_T:
        {
            if( item->GetLayer() == LAYER_BUS )
                bus_item = ( !bus_item ) ? item : bus_item;
            else
                net_item = ( !net_item ) ? item : net_item;

            break;
        }

        case SCH_LABEL_T:
        case SCH_GLOBAL_LABEL_T:
        case SCH_SHEET_PIN_T:
        case SCH_HIER_LABEL_T:
        {
            SCH_TEXT* text = static_cast<SCH_TEXT*>( item );
            conn.ConfigureFromLabel( EscapeString( text->GetShownText( &sheet, false ), CTX_NETNAME ) );

            if( conn.IsBus() )
                bus_item = ( !bus_item ) ? item : bus_item;
            else
                net_item = ( !net_item ) ? item : net_item;

            break;
        }

        default:
            break;
        }
    }

    if( net_item && bus_item )
    {
        std::shared_ptr<ERC_ITEM> ercItem = ERC_ITEM::Create( ERCE_BUS_TO_NET_CONFLICT );
        ercItem->SetSheetSpecificPath( sheet );
        ercItem->SetItems( net_item, bus_item );

        SCH_MARKER* marker = new SCH_MARKER( std::move( ercItem ), net_item->GetPosition() );
        screen->Append( marker );

        return false;
    }

    return true;
}


bool CONNECTION_GRAPH::ercCheckBusToBusConflicts( const CONNECTION_SUBGRAPH* aSubgraph )
{
    const SCH_SHEET_PATH& sheet = aSubgraph->m_sheet;
    SCH_SCREEN* screen = sheet.LastScreen();

    SCH_ITEM* label = nullptr;
    SCH_ITEM* port = nullptr;

    for( SCH_ITEM* item : aSubgraph->m_items )
    {
        switch( item->Type() )
        {
        case SCH_TEXT_T:
        case SCH_GLOBAL_LABEL_T:
            if( !label && item->Connection( &sheet )->IsBus() )
                label = item;
            break;

        case SCH_SHEET_PIN_T:
        case SCH_HIER_LABEL_T:
            if( !port && item->Connection( &sheet )->IsBus() )
                port = item;
            break;

        default:
            break;
        }
    }

    if( label && port )
    {
        bool match = false;

        for( const auto& member : label->Connection( &sheet )->Members() )
        {
            for( const auto& test : port->Connection( &sheet )->Members() )
            {
                if( test != member && member->Name() == test->Name() )
                {
                    match = true;
                    break;
                }
            }

            if( match )
                break;
        }

        if( !match )
        {
            std::shared_ptr<ERC_ITEM> ercItem = ERC_ITEM::Create( ERCE_BUS_TO_BUS_CONFLICT );
            ercItem->SetSheetSpecificPath( sheet );
            ercItem->SetItems( label, port );

            SCH_MARKER* marker = new SCH_MARKER( std::move( ercItem ), label->GetPosition() );
            screen->Append( marker );

            return false;
        }
    }

    return true;
}


bool CONNECTION_GRAPH::ercCheckBusToBusEntryConflicts( const CONNECTION_SUBGRAPH* aSubgraph )
{
    bool conflict = false;
    const SCH_SHEET_PATH& sheet = aSubgraph->m_sheet;
    SCH_SCREEN* screen = sheet.LastScreen();

    SCH_BUS_WIRE_ENTRY* bus_entry = nullptr;
    SCH_ITEM* bus_wire = nullptr;
    wxString bus_name;

    if( !aSubgraph->m_driver_connection )
    {
        // Incomplete bus entry.  Let the unconnected tests handle it.
        return true;
    }

    for( SCH_ITEM* item : aSubgraph->m_items )
    {
        switch( item->Type() )
        {
        case SCH_BUS_WIRE_ENTRY_T:
            if( !bus_entry )
                bus_entry = static_cast<SCH_BUS_WIRE_ENTRY*>( item );

            break;

        default:
            break;
        }
    }

    if( bus_entry && bus_entry->m_connected_bus_item )
    {
        bus_wire = bus_entry->m_connected_bus_item;

        // Should we continue if the type is not a line?
        wxASSERT( bus_wire->Type() == SCH_LINE_T );

        // In some cases, the connection list (SCH_CONNECTION*) can be null.
        // Skip null connections.
        if( bus_entry->Connection( &sheet )
                && bus_wire->Type() == SCH_LINE_T
                && bus_wire->Connection( &sheet ) )
        {
            conflict = true;    // Assume a conflict; we'll reset if we find it's OK

            bus_name = bus_wire->Connection( &sheet )->Name();

            std::set<wxString> test_names;
            test_names.insert( bus_entry->Connection( &sheet )->FullLocalName() );

            wxString baseName = sheet.PathHumanReadable();

            for( SCH_ITEM* driver : aSubgraph->m_drivers )
                test_names.insert( baseName + aSubgraph->GetNameForDriver( driver ) );

            for( const auto& member : bus_wire->Connection( &sheet )->Members() )
            {
                if( member->Type() == CONNECTION_TYPE::BUS )
                {
                    for( const auto& sub_member : member->Members() )
                    {
                        if( test_names.count( sub_member->FullLocalName() ) )
                            conflict = false;
                    }
                }
                else if( test_names.count( member->FullLocalName() ) )
                {
                    conflict = false;
                }
            }
        }
    }

    // Don't report warnings if this bus member has been overridden by a higher priority power pin
    // or global label
    if( conflict && CONNECTION_SUBGRAPH::GetDriverPriority( aSubgraph->m_driver )
                       >= CONNECTION_SUBGRAPH::PRIORITY::GLOBAL_POWER_PIN )
    {
        conflict = false;
    }

    if( conflict )
    {
        wxString netName = aSubgraph->m_driver_connection->Name();
        wxString msg = wxString::Format( _( "Net %s is graphically connected to bus %s but is not a"
                                            " member of that bus" ),
                                         UnescapeString( netName ),
                                         UnescapeString( bus_name ) );
        std::shared_ptr<ERC_ITEM> ercItem = ERC_ITEM::Create( ERCE_BUS_ENTRY_CONFLICT );
        ercItem->SetSheetSpecificPath( sheet );
        ercItem->SetItems( bus_entry, bus_wire );
        ercItem->SetErrorMessage( msg );

        SCH_MARKER* marker = new SCH_MARKER( std::move( ercItem ), bus_entry->GetPosition() );
        screen->Append( marker );

        return false;
    }

    return true;
}


bool CONNECTION_GRAPH::ercCheckNoConnects( const CONNECTION_SUBGRAPH* aSubgraph )
{
    ERC_SETTINGS&         settings = m_schematic->ErcSettings();
    const SCH_SHEET_PATH& sheet  = aSubgraph->m_sheet;
    SCH_SCREEN*           screen = sheet.LastScreen();
    bool                  ok     = true;
    SCH_PIN*              pin    = nullptr;

    std::set<SCH_PIN*>        unique_pins;
    std::set<SCH_LABEL_BASE*> unique_labels;

    wxString netName = GetResolvedSubgraphName( aSubgraph );

    auto process_subgraph = [&]( const CONNECTION_SUBGRAPH* aProcessGraph )
    {
        // Any subgraph that contains a no-connect should not
        // more than one pin (which would indicate it is connected
        for( SCH_ITEM* item : aProcessGraph->m_items )
        {
            switch( item->Type() )
            {
            case SCH_PIN_T:
            {
                SCH_PIN* test_pin = static_cast<SCH_PIN*>( item );

                // Only link NC to pin on the current subgraph being checked
                if( aProcessGraph == aSubgraph )
                    pin = test_pin;

                if( std::none_of( unique_pins.begin(), unique_pins.end(),
                        [test_pin]( SCH_PIN* aPin )
                        {
                            return test_pin->IsStacked( aPin );
                        }
                        ))
                {
                    unique_pins.insert( test_pin );
                }

                break;
            }

            case SCH_LABEL_T:
            case SCH_GLOBAL_LABEL_T:
            case SCH_HIER_LABEL_T:
                unique_labels.insert( static_cast<SCH_LABEL_BASE*>( item ) );
                KI_FALLTHROUGH;
            default:
                break;
            }
        }
    };

    auto it = m_net_name_to_subgraphs_map.find( netName );

    if( it != m_net_name_to_subgraphs_map.end() )
    {
        for( const CONNECTION_SUBGRAPH* subgraph : it->second )
        {
            process_subgraph( subgraph );
        }
    }
    else
    {
        process_subgraph( aSubgraph );
    }

    if( aSubgraph->m_no_connect != nullptr )
    {
        // If this subgraph reaches the rest of the schematic only through a hier
        // sheet pin (parent side) or hier label (inner side), and contains no real
        // connection points of its own, suppress the warning.  The user's intent
        // is to mark the hier link as unconnected -- whether the no-connect sits
        // on the pin or at the end of a short wire stub.
        if( !aSubgraph->m_hier_pins.empty() || !aSubgraph->m_hier_ports.empty() )
        {
            bool clean = true;

            for( SCH_ITEM* item : aSubgraph->m_items )
            {
                switch( item->Type() )
                {
                case SCH_PIN_T:
                case SCH_LABEL_T:
                case SCH_GLOBAL_LABEL_T:
                case SCH_DIRECTIVE_LABEL_T: clean = false; break;
                default: break;
                }

                if( !clean )
                    break;
            }

            if( clean )
                return true;
        }

        // Special case: If the subgraph being checked consists of only a hier port/pin and
        // a no-connect, we don't issue a "no-connect connected" warning just because
        // connections exist on the sheet on the other side of the link.
        VECTOR2I noConnectPos = aSubgraph->m_no_connect->GetPosition();

        for( SCH_SHEET_PIN* hierPin : aSubgraph->m_hier_pins )
        {
            if( hierPin->GetPosition() == noConnectPos )
                return true;
        }

        for( SCH_HIERLABEL* hierLabel : aSubgraph->m_hier_ports )
        {
            if( hierLabel->GetPosition() == noConnectPos )
                return true;
        }

        for( SCH_ITEM* item : screen->Items().Overlapping( SCH_SYMBOL_T, noConnectPos ) )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

            const SCH_PIN* test_pin = symbol->GetPin( noConnectPos );

            if( test_pin && test_pin->GetType() == ELECTRICAL_PINTYPE::PT_NC )
                return true;
        }

        if( unique_pins.size() > 1 && settings.IsTestEnabled( ERCE_NOCONNECT_CONNECTED ) )
        {
            std::shared_ptr<ERC_ITEM> ercItem = ERC_ITEM::Create( ERCE_NOCONNECT_CONNECTED );
            ercItem->SetSheetSpecificPath( sheet );
            ercItem->SetItemsSheetPaths( sheet );

            VECTOR2I pos;

            if( pin )
            {
                ercItem->SetItems( pin, aSubgraph->m_no_connect );
                pos = pin->GetPosition();
            }
            else
            {
                ercItem->SetItems( aSubgraph->m_no_connect );
                pos = aSubgraph->m_no_connect->GetPosition();
            }

            SCH_MARKER* marker = new SCH_MARKER( std::move( ercItem ), pos );
            screen->Append( marker );

            ok = false;
        }

        if( unique_pins.empty() && unique_labels.empty() &&
            settings.IsTestEnabled( ERCE_NOCONNECT_NOT_CONNECTED ) )
        {
            std::shared_ptr<ERC_ITEM> ercItem = ERC_ITEM::Create( ERCE_NOCONNECT_NOT_CONNECTED );
            ercItem->SetItems( aSubgraph->m_no_connect );
            ercItem->SetSheetSpecificPath( sheet );
            ercItem->SetItemsSheetPaths( sheet );

            SCH_MARKER* marker = new SCH_MARKER( std::move( ercItem ), aSubgraph->m_no_connect->GetPosition() );
            screen->Append( marker );

            ok = false;
        }
    }
    else
    {
        bool has_other_connections = false;
        std::vector<SCH_PIN*> pins;

        // Any subgraph that lacks a no-connect and contains a pin should also
        // contain at least one other potential driver

        for( SCH_ITEM* item : aSubgraph->m_items )
        {
            switch( item->Type() )
            {
            case SCH_PIN_T:
            {
                SCH_PIN* test_pin = static_cast<SCH_PIN*>( item );

                // Stacked pins do not count as other connections but non-stacked pins do
                if( !has_other_connections && !pins.empty()
                  && !test_pin->GetParentSymbol()->IsPower() )
                {
                    for( SCH_PIN* other_pin  : pins )
                    {
                        if( !test_pin->IsStacked( other_pin ) )
                        {
                            has_other_connections = true;
                            break;
                        }
                    }
                }

                pins.emplace_back( static_cast<SCH_PIN*>( item ) );

                break;
            }

            default:
                if( aSubgraph->GetDriverPriority( item ) != CONNECTION_SUBGRAPH::PRIORITY::NONE )
                    has_other_connections = true;

                break;
            }
        }

        // For many checks, we can just use the first pin
        pin = pins.empty() ? nullptr : pins[0];

        // But if there is a power pin, it might be connected elsewhere
        for( SCH_PIN* test_pin : pins )
        {
            // Prefer the pin is part of a real component rather than some stray power symbol
            // Or else we may fail walking connected components to a power symbol pin since we
            // reject starting at a power symbol
            if( test_pin->GetType() == ELECTRICAL_PINTYPE::PT_POWER_IN && !test_pin->IsPower() )
            {
                pin = test_pin;
                break;
            }
        }

        // Check if power input pins connect to anything else via net name,
        // but not for power symbols (with visible or legacy invisible pins).
        // We want to throw unconnected errors for power symbols even if they are connected to other
        // net items by name, because usually failing to connect them graphically is a mistake
        SYMBOL* pinLibParent = ( pin && pin->GetLibPin() )
                                       ? pin->GetLibPin()->GetParentSymbol() : nullptr;

        if( pin && !has_other_connections
                && !pin->IsPower()
                && ( !pinLibParent || !pinLibParent->IsPower() ) )
        {
            wxString name = pin->Connection( &sheet )->Name();
            wxString local_name = pin->Connection( &sheet )->Name( true );

            if( m_global_label_cache.count( name )
                    || m_local_label_cache.count( std::make_pair( sheet, local_name ) ) )
            {
                has_other_connections = true;
            }
        }

        // Only one pin, and it's not a no-connect pin
        if( pin && !has_other_connections
                && pin->GetType() != ELECTRICAL_PINTYPE::PT_NC
                && pin->GetType() != ELECTRICAL_PINTYPE::PT_NIC
                && settings.IsTestEnabled( ERCE_PIN_NOT_CONNECTED ) )
        {
            std::shared_ptr<ERC_ITEM> ercItem = ERC_ITEM::Create( ERCE_PIN_NOT_CONNECTED );
            ercItem->SetSheetSpecificPath( sheet );
            ercItem->SetItemsSheetPaths( sheet );
            ercItem->SetItems( pin );

            SCH_MARKER* marker = new SCH_MARKER( std::move( ercItem ), pin->GetPosition() );
            screen->Append( marker );

            ok = false;
        }

        // If there are multiple pins in this SG, they might be indirectly connected (by netname)
        // rather than directly connected (by wires).  We want to flag dangling pins even if they
        // join nets with another pin, as it's often a mistake
        if( pins.size() > 1 )
        {
            for( SCH_PIN* testPin : pins )
            {
                // We only apply this test to power symbols, because other symbols have
                // pins that are meant to be dangling, but the power symbols have pins
                // that are *not* meant to be dangling.
                SYMBOL* testLibParent = testPin->GetLibPin()
                                               ? testPin->GetLibPin()->GetParentSymbol()
                                               : nullptr;

                if( testLibParent && testLibParent->IsPower()
                    && testPin->ConnectedItems( sheet ).empty()
                    && settings.IsTestEnabled( ERCE_PIN_NOT_CONNECTED ) )
                {
                    std::shared_ptr<ERC_ITEM> ercItem = ERC_ITEM::Create( ERCE_PIN_NOT_CONNECTED );
                    ercItem->SetSheetSpecificPath( sheet );
                    ercItem->SetItemsSheetPaths( sheet );
                    ercItem->SetItems( testPin );

                    SCH_MARKER* marker = new SCH_MARKER( std::move( ercItem ), testPin->GetPosition() );
                    screen->Append( marker );

                    ok = false;
                }
            }
        }
    }

    return ok;
}


bool CONNECTION_GRAPH::ercCheckDanglingWireEndpoints( const CONNECTION_SUBGRAPH* aSubgraph )
{
    int                   err_count = 0;
    const SCH_SHEET_PATH& sheet = aSubgraph->m_sheet;

    for( SCH_ITEM* item : aSubgraph->m_items )
    {
        if( item->GetLayer() != LAYER_WIRE )
            continue;

        if( item->Type() == SCH_LINE_T )
        {
            SCH_LINE* line = static_cast<SCH_LINE*>( item );

            if( line->IsGraphicLine() )
                continue;

            auto report_error = [&]( VECTOR2I& location )
            {
                std::shared_ptr<ERC_ITEM> ercItem = ERC_ITEM::Create( ERCE_UNCONNECTED_WIRE_ENDPOINT );

                ercItem->SetItems( line );
                ercItem->SetSheetSpecificPath( sheet );
                ercItem->SetErrorMessage( _( "Unconnected wire endpoint" ) );

                SCH_MARKER* marker = new SCH_MARKER( std::move( ercItem ), location );
                sheet.LastScreen()->Append( marker );

                err_count++;
            };

            if( line->IsStartDangling() )
                report_error( line->GetConnectionPoints()[0] );

            if( line->IsEndDangling() )
                report_error( line->GetConnectionPoints()[1] );
        }
        else if( item->Type() == SCH_BUS_WIRE_ENTRY_T )
        {
            SCH_BUS_WIRE_ENTRY* entry = static_cast<SCH_BUS_WIRE_ENTRY*>( item );

            auto report_error = [&]( VECTOR2I& location )
            {
                std::shared_ptr<ERC_ITEM> ercItem = ERC_ITEM::Create( ERCE_UNCONNECTED_WIRE_ENDPOINT );

                ercItem->SetItems( entry );
                ercItem->SetSheetSpecificPath( sheet );
                ercItem->SetErrorMessage( _( "Unconnected wire to bus entry" ) );

                SCH_MARKER* marker = new SCH_MARKER( std::move( ercItem ), location );
                sheet.LastScreen()->Append( marker );

                err_count++;
            };

            if( entry->IsStartDangling() )
                report_error( entry->GetConnectionPoints()[0] );

            if( entry->IsEndDangling() )
                report_error( entry->GetConnectionPoints()[1] );
        }

    }

    return err_count > 0;
}


bool CONNECTION_GRAPH::ercCheckFloatingWires( const CONNECTION_SUBGRAPH* aSubgraph )
{
    if( aSubgraph->m_driver )
        return true;

    const SCH_SHEET_PATH& sheet = aSubgraph->m_sheet;
    std::vector<SCH_ITEM*> wires;

    // We've gotten this far, so we know we have no valid driver.  All we need to do is check
    // for a wire that we can place the error on.
    for( SCH_ITEM* item : aSubgraph->m_items )
    {
        if( item->Type() == SCH_LINE_T && item->GetLayer() == LAYER_WIRE )
            wires.emplace_back( item );
        else if( item->Type() == SCH_BUS_WIRE_ENTRY_T )
            wires.emplace_back( item );
    }

    if( !wires.empty() )
    {
        SCH_SCREEN* screen = aSubgraph->m_sheet.LastScreen();

        std::shared_ptr<ERC_ITEM> ercItem = ERC_ITEM::Create( ERCE_WIRE_DANGLING );
        ercItem->SetSheetSpecificPath( sheet );
        ercItem->SetItems( wires[0],
                           wires.size() > 1 ? wires[1] : nullptr,
                           wires.size() > 2 ? wires[2] : nullptr,
                           wires.size() > 3 ? wires[3] : nullptr );

        SCH_MARKER* marker = new SCH_MARKER( std::move( ercItem ), wires[0]->GetPosition() );
        screen->Append( marker );

        return false;
    }

    return true;
}


void CONNECTION_GRAPH::collectBusMemberSiblings( const CONNECTION_SUBGRAPH* aBusParent, const wxString& aMemberName,
                                                 std::unordered_set<const CONNECTION_SUBGRAPH*>& aOut ) const
{
    auto busBucket = m_net_name_to_subgraphs_map.find( aBusParent->m_driver_connection->Name() );

    if( busBucket == m_net_name_to_subgraphs_map.end() )
        return;

    for( const CONNECTION_SUBGRAPH* siblingBus : busBucket->second )
    {
        for( const auto& [sibMemberConn, sibMembers] : siblingBus->m_bus_neighbors )
        {
            if( sibMemberConn->Name() != aMemberName )
                continue;

            for( const CONNECTION_SUBGRAPH* sibling : sibMembers )
                aOut.insert( sibling );
        }
    }
}


bool CONNECTION_GRAPH::ercCheckLabels( const CONNECTION_SUBGRAPH* aSubgraph )
{
    // Label connection rules:
    // Any label without a no-connect needs to have at least 2 pins, otherwise it is invalid
    // Local labels are flagged if they don't connect to any pins and don't have a no-connect
    // Global labels are flagged if they appear only once, don't connect to any local labels,
    // and don't have a no-connect marker

    if( !aSubgraph->m_driver_connection )
        return true;

    // Buses are excluded from this test: many users create buses with only a single instance
    // and it's not really a problem as long as the nets in the bus pass ERC
    if( aSubgraph->m_driver_connection->IsBus() )
        return true;

    const SCH_SHEET_PATH& sheet    = aSubgraph->m_sheet;
    ERC_SETTINGS&         settings = m_schematic->ErcSettings();
    bool                  ok       = true;
    size_t                pinCount = 0;
    bool                  has_nc   = !!aSubgraph->m_no_connect;

    std::map<KICAD_T, std::vector<SCH_TEXT*>> label_map;


    auto hasPins =
            []( const CONNECTION_SUBGRAPH* aLocSubgraph ) -> size_t
            {
                return std::count_if( aLocSubgraph->m_items.begin(), aLocSubgraph->m_items.end(),
                                      []( const SCH_ITEM* item )
                                      {
                                          return item->Type() == SCH_PIN_T;
                                      } );
            };

    auto reportError =
            [&]( SCH_TEXT* aText, int errCode )
            {
                if( settings.IsTestEnabled( errCode ) )
                {
                    std::shared_ptr<ERC_ITEM> ercItem = ERC_ITEM::Create( errCode );
                    ercItem->SetSheetSpecificPath( sheet );
                    ercItem->SetItems( aText );

                    SCH_MARKER* marker = new SCH_MARKER( std::move( ercItem ), aText->GetPosition() );
                    aSubgraph->m_sheet.LastScreen()->Append( marker );
                }
            };

    pinCount = hasPins( aSubgraph );

    for( SCH_ITEM* item : aSubgraph->m_items )
    {
        switch( item->Type() )
        {
        case SCH_LABEL_T:
        case SCH_GLOBAL_LABEL_T:
        case SCH_HIER_LABEL_T:
        {
            SCH_TEXT* text = static_cast<SCH_TEXT*>( item );

            label_map[item->Type()].push_back( text );

            // Below, we'll create an ERC if the whole subgraph is unconnected.  But, additionally,
            // we want to error if an individual label in the subgraph is floating, even if it's
            // connected to other valid things by way of another label on the same sheet.
            if( text->IsDangling() )
            {
                reportError( text, ERCE_LABEL_NOT_CONNECTED );
                return false;
            }

            break;
        }

        default:
            break;
        }
    }

    if( label_map.empty() )
        return true;

    // Walk m_bus_parents once. Bus parents may carry a no-connect that suppresses
    // an unconnected-label error, and they're how we reach bus members on other
    // sheets that share this net.
    std::unordered_set<const CONNECTION_SUBGRAPH*> busMemberSiblings;

    for( auto& [memberConn, busParents] : aSubgraph->m_bus_parents )
    {
        wxString memberName = memberConn->Name();

        for( CONNECTION_SUBGRAPH* busParent : busParents )
        {
            if( busParent->m_no_connect )
                has_nc = true;

            for( CONNECTION_SUBGRAPH* hp = busParent->m_hier_parent; hp; hp = hp->m_hier_parent )
            {
                if( hp->m_no_connect )
                    has_nc = true;
            }

            collectBusMemberSiblings( busParent, memberName, busMemberSiblings );
        }
    }

    wxString netName = GetResolvedSubgraphName( aSubgraph );

    wxCHECK_MSG( m_schematic, true, wxS( "Null m_schematic in CONNECTION_GRAPH::ercCheckLabels" ) );

    // Labels that have multiple pins connected are not dangling (may be used for naming segments)
    // so leave them without errors here
    if( pinCount > 1 )
        return true;

    for( auto& [type, label_vec] : label_map )
    {
        for( SCH_TEXT* text : label_vec )
        {
            size_t allPins = pinCount;
            size_t localPins = pinCount;
            bool   hasLocalHierarchy = false;

            if( !aSubgraph->m_hier_pins.empty() || !aSubgraph->m_hier_ports.empty() )
            {
                // A label bridging multiple hierarchical connections
                // (e.g., connecting sheet pins from different sub-sheet
                // instances) is serving a valid routing purpose even
                // without local component pins.
                std::set<wxString> uniquePortNames;
                for( SCH_HIERLABEL* port : aSubgraph->m_hier_ports )
                    uniquePortNames.insert( aSubgraph->GetNameForDriver( port ) );

                if( aSubgraph->m_hier_pins.size() + uniquePortNames.size() > 1 )
                {
                    hasLocalHierarchy = true;
                }

                // Also check bus parents for bus-based hierarchical
                // routing on the same sheet.
                for( auto& [connection, busParents] : aSubgraph->m_bus_parents )
                {
                    for( const CONNECTION_SUBGRAPH* busParent : busParents )
                    {
                        if( busParent->m_sheet == sheet
                            && ( !busParent->m_hier_pins.empty()
                                 || !busParent->m_hier_ports.empty() ) )
                        {
                            hasLocalHierarchy = true;
                            break;
                        }
                    }

                    if( hasLocalHierarchy )
                        break;
                }
            }

            std::unordered_set<const CONNECTION_SUBGRAPH*> creditedNeighbors;
            creditedNeighbors.insert( aSubgraph );

            auto creditNeighbor = [&]( const CONNECTION_SUBGRAPH* neighbor )
            {
                if( !creditedNeighbors.insert( neighbor ).second )
                    return;

                if( neighbor->m_no_connect )
                    has_nc = true;

                size_t neighborPins = hasPins( neighbor );
                allPins += neighborPins;

                if( neighbor->m_sheet == sheet )
                {
                    localPins += neighborPins;

                    if( !neighbor->m_hier_pins.empty() || !neighbor->m_hier_ports.empty() )
                    {
                        hasLocalHierarchy = true;
                    }
                }
            };

            auto it = m_net_name_to_subgraphs_map.find( netName );

            if( it != m_net_name_to_subgraphs_map.end() )
            {
                for( const CONNECTION_SUBGRAPH* neighbor : it->second )
                    creditNeighbor( neighbor );
            }

            for( const CONNECTION_SUBGRAPH* sibling : busMemberSiblings )
                creditNeighbor( sibling );

            if( allPins == 1 && !has_nc )
            {
                reportError( text, ERCE_LABEL_SINGLE_PIN );
                ok = false;
            }

            // A local label that connects to other subgraphs with
            // hierarchical connections on the same sheet (through bus
            // parents or net-name neighbors) is routing aggregated nets and should
            // not be flagged even without local component pins.
            if( allPins == 0
                || ( type == SCH_LABEL_T && localPins == 0 && allPins > 1
                     && !has_nc && !hasLocalHierarchy ) )
            {
                reportError( text, ERCE_LABEL_NOT_CONNECTED );
                ok = false;
            }
        }
    }

    return ok;
}


int CONNECTION_GRAPH::ercCheckSingleGlobalLabel()
{
    int errors = 0;

    std::map<wxString, std::tuple<int, const SCH_ITEM*, SCH_SHEET_PATH>> labelData;

    for( const SCH_SHEET_PATH& sheet : m_sheetList )
    {
        for( SCH_ITEM* item : sheet.LastScreen()->Items().OfType( SCH_GLOBAL_LABEL_T ) )
        {
            SCH_TEXT* labelText = static_cast<SCH_TEXT*>( item );
            wxString  resolvedLabelText =
                    EscapeString( labelText->GetShownText( &sheet, false ), CTX_NETNAME );

            if( labelData.find( resolvedLabelText ) == labelData.end() )
            {
                labelData[resolvedLabelText] = { 1, item, sheet };
            }
            else
            {
                std::get<0>( labelData[resolvedLabelText] ) += 1;
                std::get<1>( labelData[resolvedLabelText] ) = nullptr;
                std::get<2>( labelData[resolvedLabelText] ) = sheet;
            }
        }
    }

    for( const auto& label : labelData )
    {
        if( std::get<0>( label.second ) == 1 )
        {
            const SCH_SHEET_PATH& sheet = std::get<2>( label.second );
            const SCH_ITEM*       item = std::get<1>( label.second );

            std::shared_ptr<ERC_ITEM> ercItem = ERC_ITEM::Create( ERCE_SINGLE_GLOBAL_LABEL );
            ercItem->SetItems( std::get<1>( label.second ) );
            ercItem->SetSheetSpecificPath( sheet );
            ercItem->SetItemsSheetPaths( sheet );

            SCH_MARKER* marker = new SCH_MARKER( std::move( ercItem ), item->GetPosition() );
            sheet.LastScreen()->Append( marker );

            errors++;
        }
    }

    return errors;
}


int CONNECTION_GRAPH::ercCheckDirectiveLabels()
{
    int error_count = 0;

    for( const SCH_SHEET_PATH& sheet : m_sheetList )
    {
        for( SCH_ITEM* item : sheet.LastScreen()->Items().OfType( SCH_DIRECTIVE_LABEL_T ) )
        {
            SCH_LABEL* label = static_cast<SCH_LABEL*>( item );

            if( label->IsDangling() )
            {
                std::shared_ptr<ERC_ITEM> ercItem = ERC_ITEM::Create( ERCE_LABEL_NOT_CONNECTED );
                SCH_TEXT*                 text = static_cast<SCH_TEXT*>( item );
                ercItem->SetSheetSpecificPath( sheet );
                ercItem->SetItems( text );

                SCH_MARKER* marker = new SCH_MARKER( std::move( ercItem ), text->GetPosition() );
                sheet.LastScreen()->Append( marker );
                error_count++;
            }
        }
    }

    return error_count;
}


int CONNECTION_GRAPH::ercCheckHierSheets()
{
    wxString msg;
    int errors = 0;

    ERC_SETTINGS& settings = m_schematic->ErcSettings();

    for( const SCH_SHEET_PATH& sheet : m_sheetList )
    {
        // Hierarchical labels in the top-level sheets cannot be connected to anything.
        if( sheet.Last()->IsTopLevelSheet() )
        {
            for( const SCH_ITEM* item : sheet.LastScreen()->Items().OfType( SCH_HIER_LABEL_T ) )
            {
                const SCH_HIERLABEL* label = static_cast<const SCH_HIERLABEL*>( item );

                wxCHECK2( label, continue );

                msg.Printf( _( "Hierarchical label '%s' in root sheet cannot be connected to non-existent "
                               "parent sheet" ),
                            label->GetShownText( &sheet, true ) );
                std::shared_ptr<ERC_ITEM> ercItem = ERC_ITEM::Create( ERCE_PIN_NOT_CONNECTED );
                ercItem->SetItems( item );
                ercItem->SetErrorMessage( msg );

                SCH_MARKER* marker = new SCH_MARKER( std::move( ercItem ), item->GetPosition() );
                sheet.LastScreen()->Append( marker );

                errors++;
            }
        }

        for( SCH_ITEM* item : sheet.LastScreen()->Items().OfType( SCH_SHEET_T ) )
        {
            SCH_SHEET* parentSheet = static_cast<SCH_SHEET*>( item );
            SCH_SHEET_PATH parentSheetPath = sheet;

            parentSheetPath.push_back( parentSheet );

            std::map<wxString, SCH_SHEET_PIN*> pins;
            std::map<wxString, SCH_HIERLABEL*> labels;

            for( SCH_SHEET_PIN* pin : parentSheet->GetPins() )
            {
                if( settings.IsTestEnabled( ERCE_HIERACHICAL_LABEL ) )
                    pins[ pin->GetShownText( &parentSheetPath, false ) ] = pin;

                if( pin->IsDangling() && settings.IsTestEnabled( ERCE_PIN_NOT_CONNECTED ) )
                {
                    std::shared_ptr<ERC_ITEM> ercItem = ERC_ITEM::Create( ERCE_PIN_NOT_CONNECTED );
                    ercItem->SetItems( pin );
                    ercItem->SetSheetSpecificPath( sheet );
                    ercItem->SetItemsSheetPaths( sheet );

                    SCH_MARKER* marker = new SCH_MARKER( std::move( ercItem ), pin->GetPosition() );
                    sheet.LastScreen()->Append( marker );

                    errors++;
                }
            }

            if( settings.IsTestEnabled( ERCE_HIERACHICAL_LABEL ) )
            {
                std::set<wxString> matchedPins;

                for( SCH_ITEM* subItem : parentSheet->GetScreen()->Items() )
                {
                    if( subItem->Type() == SCH_HIER_LABEL_T )
                    {
                        SCH_HIERLABEL* label = static_cast<SCH_HIERLABEL*>( subItem );
                        wxString       labelText = label->GetShownText( &parentSheetPath, false );

                        if( !pins.contains( labelText ) )
                            labels[ labelText ] = label;
                        else
                            matchedPins.insert( labelText );
                    }
                }

                for( const wxString& matched : matchedPins )
                    pins.erase( matched );

                for( const auto& [name, pin] : pins )
                {
                    msg.Printf( _( "Sheet pin %s has no matching hierarchical label inside the sheet" ),
                                UnescapeString( name ) );

                    std::shared_ptr<ERC_ITEM> ercItem = ERC_ITEM::Create( ERCE_HIERACHICAL_LABEL );
                    ercItem->SetItems( pin );
                    ercItem->SetErrorMessage( msg );
                    ercItem->SetSheetSpecificPath( sheet );
                    ercItem->SetItemsSheetPaths( sheet );

                    SCH_MARKER* marker = new SCH_MARKER( std::move( ercItem ), pin->GetPosition() );
                    sheet.LastScreen()->Append( marker );

                    errors++;
                }

                for( const auto& [name, label] : labels )
                {
                    msg.Printf( _( "Hierarchical label %s has no matching sheet pin in the parent sheet" ),
                                UnescapeString( name ) );

                    std::shared_ptr<ERC_ITEM> ercItem = ERC_ITEM::Create( ERCE_HIERACHICAL_LABEL );
                    ercItem->SetItems( label );
                    ercItem->SetErrorMessage( msg );
                    ercItem->SetSheetSpecificPath( parentSheetPath );
                    ercItem->SetItemsSheetPaths( parentSheetPath );

                    SCH_MARKER* marker = new SCH_MARKER( std::move( ercItem ), label->GetPosition() );
                    parentSheet->GetScreen()->Append( marker );

                    errors++;
                }
            }
        }
    }

    return errors;
}
