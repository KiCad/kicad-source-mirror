/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 CERN
 * Copyright (C) 2021 KiCad Developers, see AUTHORS.txt for contributors.
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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <list>
#include <thread>
#include <future>
#include <vector>
#include <unordered_map>
#include <profile.h>
#include <common.h>
#include <core/kicad_algo.h>
#include <erc.h>
#include <pin_type.h>
#include <sch_bus_entry.h>
#include <sch_symbol.h>
#include <sch_edit_frame.h>
#include <sch_line.h>
#include <sch_marker.h>
#include <sch_pin.h>
#include <sch_sheet.h>
#include <sch_sheet_path.h>
#include <sch_sheet_pin.h>
#include <sch_text.h>
#include <schematic.h>
#include <connection_graph.h>
#include <widgets/ui_common.h>
#include <string_utils.h>
#include <wx/log.h>

#include <advanced_config.h> // for realtime connectivity switch


/*
 * Flag to enable connectivity profiling
 * @ingroup trace_env_vars
 */
static const wxChar ConnProfileMask[] = wxT( "CONN_PROFILE" );

/*
 * Flag to enable connectivity tracing
 * @ingroup trace_env_vars
 */
static const wxChar ConnTrace[] = wxT( "CONN" );


bool CONNECTION_SUBGRAPH::ResolveDrivers( bool aCheckMultipleDrivers )
{
    PRIORITY               highest_priority = PRIORITY::INVALID;
    std::vector<SCH_ITEM*> candidates;
    std::vector<SCH_ITEM*> strong_drivers;

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

        if( item_priority == PRIORITY::PIN
                && !static_cast<SCH_PIN*>( item )->GetParentSymbol()->IsInNetlist() )
            continue;

        if( item_priority >= PRIORITY::HIER_LABEL )
            strong_drivers.push_back( item );

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
    m_local_driver = ( highest_priority < PRIORITY::POWER_PIN );

    if( !candidates.empty() )
    {
        if( candidates.size() > 1 )
        {
            if( highest_priority == PRIORITY::SHEET_PIN )
            {
                // We have multiple options, and they are all hierarchical
                // sheet pins.  Let's prefer outputs over inputs.

                for( SCH_ITEM* c : candidates )
                {
                    SCH_SHEET_PIN* p = static_cast<SCH_SHEET_PIN*>( c );

                    if( p->GetShape() == PINSHEETLABEL_SHAPE::PS_OUTPUT )
                    {
                        m_driver = c;
                        break;
                    }
                }
            }
            else
            {
                // See if a previous driver is still a candidate
                void* previousDriver = nullptr;

                for( SCH_ITEM* member : m_items )
                {
                    if( SCH_CONNECTION* mc = member->Connection( &m_sheet ) )
                    {
                        if( mc->GetLastDriver() )
                        {
                            previousDriver = mc->GetLastDriver();
                            break;
                        }
                    }
                }

                // For all other driver types, sort by name
                std::sort( candidates.begin(), candidates.end(),
                           [&]( SCH_ITEM* a, SCH_ITEM* b ) -> bool
                           {
                               // meet irreflexive requirements of std::sort
                               if( a == b )
                                   return false;

                               SCH_CONNECTION* ac = a->Connection( &m_sheet );
                               SCH_CONNECTION* bc = b->Connection( &m_sheet );

                               // Ensure we don't pick the subset over the superset
                               if( ac->IsBus() && bc->IsBus() )
                                   return bc->IsSubsetOf( ac );

                               if( a == previousDriver )
                                   return true;
                               else if( b == previousDriver )
                                   return false;
                               else
                                   return GetNameForDriver( a ) < GetNameForDriver( b );
                           } );
            }
        }

        if( !m_driver )
            m_driver = candidates[0];
    }

    if( strong_drivers.size() > 1 )
        m_multiple_drivers = true;

    // Drop weak drivers
    if( m_strong_driver )
        m_drivers = strong_drivers;

    // Cache driver connection
    if( m_driver )
    {
        m_driver_connection = m_driver->Connection( &m_sheet );
        m_driver_connection->ConfigureFromLabel( GetNameForDriver( m_driver ) );
        m_driver_connection->SetDriver( m_driver );
        m_driver_connection->ClearDirty();
    }
    else
    {
        m_driver_connection = nullptr;
    }

    if( aCheckMultipleDrivers && m_multiple_drivers )
    {
        // First check if all the candidates are actually the same
        bool same = true;
        wxString first = GetNameForDriver( candidates[0] );
        SCH_ITEM* second_item = nullptr;

        for( unsigned i = 1; i < candidates.size(); i++ )
        {
            if( GetNameForDriver( candidates[i] ) != first )
            {
                second_item = candidates[i];
                same = false;
                break;
            }
        }

        if( !same )
        {
            m_first_driver  = m_driver;
            m_second_driver = second_item;
        }
    }

    return ( m_driver != nullptr );
}


wxString CONNECTION_SUBGRAPH::GetNetName() const
{
    if( !m_driver || m_dirty )
        return "";

    if( !m_driver->Connection( &m_sheet ) )
    {
#ifdef CONNECTIVITY_DEBUG
        wxASSERT_MSG( false, "Tried to get the net name of an item with no connection" );
#endif

        return "";
    }

    return m_driver->Connection( &m_sheet )->Name();
}


std::vector<SCH_ITEM*> CONNECTION_SUBGRAPH::GetBusLabels() const
{
    std::vector<SCH_ITEM*> labels;

    for( SCH_ITEM* item : m_drivers )
    {
        switch( item->Type() )
        {
        case SCH_LABEL_T:
        case SCH_GLOBAL_LABEL_T:
        {
            SCH_CONNECTION* label_conn = item->Connection( &m_sheet );

            // Only consider bus vectors
            if( label_conn->Type() == CONNECTION_TYPE::BUS )
                labels.push_back( item );

            break;
        }

        default: break;
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
        bool forceNoConnect = m_no_connect != nullptr;
        SCH_PIN* pin = static_cast<SCH_PIN*>( aItem );
        return pin->GetDefaultNetName( m_sheet, forceNoConnect );
        break;
    }

    case SCH_LABEL_T:
    case SCH_GLOBAL_LABEL_T:
    case SCH_HIER_LABEL_T:
    case SCH_SHEET_PIN_T:
    {
        return EscapeString( static_cast<SCH_TEXT*>( aItem )->GetShownText(), CTX_NETNAME );
        break;
    }

    default:
        wxFAIL_MSG( "Unhandled item type in GetNameForDriver" );
        break;
    }

    return wxEmptyString;
}


const wxString& CONNECTION_SUBGRAPH::GetNameForDriver( SCH_ITEM* aItem )
{
    auto it = m_driver_name_cache.find( aItem );

    if( it != m_driver_name_cache.end() )
        return it->second;

    m_driver_name_cache[aItem] = driverName( aItem );

    return m_driver_name_cache.at( aItem );
}


const wxString CONNECTION_SUBGRAPH::GetNameForDriver( SCH_ITEM* aItem ) const
{
    auto it = m_driver_name_cache.find( aItem );

    if( it != m_driver_name_cache.end() )
        return it->second;

    return driverName( aItem );
}


void CONNECTION_SUBGRAPH::Absorb( CONNECTION_SUBGRAPH* aOther )
{
    wxASSERT( m_sheet == aOther->m_sheet );

    for( SCH_ITEM* item : aOther->m_items )
    {
        item->Connection( &m_sheet )->SetSubgraphCode( m_code );
        AddItem( item );
    }

    m_bus_neighbors.insert( aOther->m_bus_neighbors.begin(), aOther->m_bus_neighbors.end() );
    m_bus_parents.insert( aOther->m_bus_parents.begin(), aOther->m_bus_parents.end() );

    m_multiple_drivers |= aOther->m_multiple_drivers;

    aOther->m_absorbed = true;
    aOther->m_dirty = false;
    aOther->m_driver = nullptr;
    aOther->m_driver_connection = nullptr;
    aOther->m_absorbed_by = this;
}


void CONNECTION_SUBGRAPH::AddItem( SCH_ITEM* aItem )
{
    m_items.push_back( aItem );

    if( aItem->Connection( &m_sheet )->IsDriver() )
        m_drivers.push_back( aItem );

    if( aItem->Type() == SCH_SHEET_PIN_T )
        m_hier_pins.push_back( static_cast<SCH_SHEET_PIN*>( aItem ) );
    else if( aItem->Type() == SCH_HIER_LABEL_T )
        m_hier_ports.push_back( static_cast<SCH_HIERLABEL*>( aItem ) );
}


void CONNECTION_SUBGRAPH::UpdateItemConnections()
{
    if( !m_driver_connection )
        return;

    for( SCH_ITEM* item : m_items )
    {
        SCH_CONNECTION* item_conn = item->Connection( &m_sheet );

        if( !item_conn )
            item_conn = item->InitializeConnection( m_sheet, m_graph );

        if( ( m_driver_connection->IsBus() && item_conn->IsNet() ) ||
            ( m_driver_connection->IsNet() && item_conn->IsBus() ) )
        {
            continue;
        }

        if( item != m_driver )
        {
            item_conn->Clone( *m_driver_connection );
            item_conn->ClearDirty();
        }
    }
}


CONNECTION_SUBGRAPH::PRIORITY CONNECTION_SUBGRAPH::GetDriverPriority( SCH_ITEM* aDriver )
{
    if( !aDriver )
        return PRIORITY::NONE;

    switch( aDriver->Type() )
    {
    case SCH_SHEET_PIN_T:     return PRIORITY::SHEET_PIN;
    case SCH_HIER_LABEL_T:    return PRIORITY::HIER_LABEL;
    case SCH_LABEL_T:         return PRIORITY::LOCAL_LABEL;
    case SCH_GLOBAL_LABEL_T:  return PRIORITY::GLOBAL;
    case SCH_PIN_T:
    {
        auto sch_pin = static_cast<SCH_PIN*>( aDriver );

        if( sch_pin->IsPowerConnection() )
            return PRIORITY::POWER_PIN;
        else
            return PRIORITY::PIN;
    }

    default: return PRIORITY::NONE;
    }
}


bool CONNECTION_GRAPH::m_allowRealTime = true;


void CONNECTION_GRAPH::Reset()
{
    for( auto& subgraph : m_subgraphs )
        delete subgraph;

    m_items.clear();
    m_subgraphs.clear();
    m_driver_subgraphs.clear();
    m_sheet_to_subgraphs_map.clear();
    m_invisible_power_pins.clear();
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
}


void CONNECTION_GRAPH::Recalculate( const SCH_SHEET_LIST& aSheetList, bool aUnconditional,
                                    std::function<void( SCH_ITEM* )>* aChangedItemHandler )
{
    PROF_COUNTER recalc_time( "CONNECTION_GRAPH::Recalculate" );

    if( aUnconditional )
        Reset();

    PROF_COUNTER update_items( "updateItemConnectivity" );

    m_sheetList = aSheetList;

    for( const SCH_SHEET_PATH& sheet : aSheetList )
    {
        std::vector<SCH_ITEM*> items;

        for( SCH_ITEM* item : sheet.LastScreen()->Items() )
        {
            if( item->IsConnectable() && ( aUnconditional || item->IsConnectivityDirty() ) )
                items.push_back( item );
        }

        m_items.reserve( m_items.size() + items.size() );

        updateItemConnectivity( sheet, items );

        // UpdateDanglingState() also adds connected items for SCH_TEXT
        sheet.LastScreen()->TestDanglingEnds( &sheet, aChangedItemHandler );
    }

    if( wxLog::IsAllowedTraceMask( ConnProfileMask ) )
        update_items.Show();

    PROF_COUNTER build_graph( "buildConnectionGraph" );

    buildConnectionGraph();

    if( wxLog::IsAllowedTraceMask( ConnProfileMask ) )
        build_graph.Show();

    recalc_time.Stop();

    if( wxLog::IsAllowedTraceMask( ConnProfileMask ) )
        recalc_time.Show();

#ifndef DEBUG
    // Pressure relief valve for release builds
    const double max_recalc_time_msecs = 250.;

    if( m_allowRealTime && ADVANCED_CFG::GetCfg().m_RealTimeConnectivity &&
        recalc_time.msecs() > max_recalc_time_msecs )
    {
        m_allowRealTime = false;
    }
#endif
}


void CONNECTION_GRAPH::updateItemConnectivity( const SCH_SHEET_PATH& aSheet,
                                               const std::vector<SCH_ITEM*>& aItemList )
{
    std::map< wxPoint, std::vector<SCH_ITEM*> > connection_map;

    for( SCH_ITEM* item : aItemList )
    {
        std::vector< wxPoint > points = item->GetConnectionPoints();
        item->ConnectedItems( aSheet ).clear();

        if( item->Type() == SCH_SHEET_T )
        {
            for( SCH_SHEET_PIN* pin : static_cast<SCH_SHEET*>( item )->GetPins() )
            {
                pin->InitializeConnection( aSheet, this );

                pin->ConnectedItems( aSheet ).clear();

                connection_map[ pin->GetTextPos() ].push_back( pin );
                m_items.emplace_back( pin );
            }
        }
        else if( item->Type() == SCH_SYMBOL_T )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

            for( SCH_PIN* pin : symbol->GetPins( &aSheet ) )
            {
                pin->InitializeConnection( aSheet, this );

                wxPoint pos = pin->GetPosition();

                // because calling the first time is not thread-safe
                pin->GetDefaultNetName( aSheet );
                pin->ConnectedItems( aSheet ).clear();

                // Invisible power pins need to be post-processed later

                if( pin->IsPowerConnection() && !pin->IsVisible() )
                    m_invisible_power_pins.emplace_back( std::make_pair( aSheet, pin ) );

                connection_map[ pos ].push_back( pin );
                m_items.emplace_back( pin );
            }
        }
        else
        {
            m_items.emplace_back( item );
            auto conn = item->InitializeConnection( aSheet, this );

            // Set bus/net property here so that the propagation code uses it
            switch( item->Type() )
            {
            case SCH_LINE_T:
                conn->SetType( item->GetLayer() == LAYER_BUS ? CONNECTION_TYPE::BUS :
                                                               CONNECTION_TYPE::NET );
                break;

            case SCH_BUS_BUS_ENTRY_T:
                conn->SetType( CONNECTION_TYPE::BUS );
                // clean previous (old) links:
                static_cast<SCH_BUS_BUS_ENTRY*>( item )->m_connected_bus_items[0] = nullptr;
                static_cast<SCH_BUS_BUS_ENTRY*>( item )->m_connected_bus_items[1] = nullptr;
                break;

            case SCH_PIN_T:
                conn->SetType( CONNECTION_TYPE::NET );
                break;

            case SCH_BUS_WIRE_ENTRY_T:
                conn->SetType( CONNECTION_TYPE::NET );
                // clean previous (old) link:
                static_cast<SCH_BUS_WIRE_ENTRY*>( item )->m_connected_bus_item = nullptr;
                break;

            default:
                break;
            }

            for( const wxPoint& point : points )
                connection_map[ point ].push_back( item );
        }

        item->SetConnectivityDirty( false );
    }

    for( const auto& it : connection_map )
    {
        auto connection_vec = it.second;

        for( auto primary_it = connection_vec.begin(); primary_it != connection_vec.end(); primary_it++ )
        {
            SCH_ITEM* connected_item = *primary_it;

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
                    SCH_SCREEN* screen = aSheet.LastScreen();
                    SCH_LINE*   bus = screen->GetBus( it.first );

                    if( bus )
                    {
                        auto bus_entry = static_cast<SCH_BUS_WIRE_ENTRY*>( connected_item );
                        bus_entry->m_connected_bus_item = bus;
                    }
                }
            }

            // Bus-to-bus entries are treated just like bus wires
            else if( connected_item->Type() == SCH_BUS_BUS_ENTRY_T )
            {
                if( connection_vec.size() < 2 )
                {
                    SCH_SCREEN* screen = aSheet.LastScreen();
                    SCH_LINE*   bus = screen->GetBus( it.first );

                    if( bus )
                    {
                        auto bus_entry = static_cast<SCH_BUS_BUS_ENTRY*>( connected_item );

                        if( it.first == bus_entry->GetPosition() )
                            bus_entry->m_connected_bus_items[0] = bus;
                        else
                            bus_entry->m_connected_bus_items[1] = bus;

                        bus_entry->ConnectedItems( aSheet ).insert( bus );
                        bus->ConnectedItems( aSheet ).insert( bus_entry );
                    }
                }
            }

            // Change junctions to be on bus junction layer if they are touching a bus
            else if( connected_item->Type() == SCH_JUNCTION_T )
            {
                SCH_SCREEN* screen = aSheet.LastScreen();
                SCH_LINE*   bus    = screen->GetBus( it.first );

                connected_item->SetLayer( bus ? LAYER_BUS_JUNCTION : LAYER_JUNCTION );
            }

            for( auto test_it = primary_it + 1; test_it != connection_vec.end(); test_it++ )
            {
                auto test_item = *test_it;

                if( connected_item != test_item &&
                    connected_item->ConnectionPropagatesTo( test_item ) &&
                    test_item->ConnectionPropagatesTo( connected_item ) )
                {
                    connected_item->ConnectedItems( aSheet ).insert( test_item );
                    test_item->ConnectedItems( aSheet ).insert( connected_item );
                }

                // Set up the link between the bus entry net and the bus
                if( connected_item->Type() == SCH_BUS_WIRE_ENTRY_T )
                {
                    if( test_item->Connection( &aSheet )->IsBus() )
                    {
                        auto bus_entry = static_cast<SCH_BUS_WIRE_ENTRY*>( connected_item );
                        bus_entry->m_connected_bus_item = test_item;
                    }
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
                    auto screen = aSheet.LastScreen();
                    auto bus = screen->GetBus( it.first );

                    if( bus )
                        bus_entry->m_connected_bus_item = bus;
                }
            }
        }
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


void CONNECTION_GRAPH::buildConnectionGraph()
{
    // Recache all bus aliases for later use
    wxCHECK_RET( m_schematic, "Connection graph cannot be built without schematic pointer" );

    SCH_SHEET_LIST all_sheets = m_schematic->GetSheets();

    for( unsigned i = 0; i < all_sheets.size(); i++ )
    {
        for( const auto& alias : all_sheets[i].LastScreen()->GetBusAliases() )
            m_bus_alias_cache[ alias->GetName() ] = alias;
    }

    // Build subgraphs from items (on a per-sheet basis)

    for( SCH_ITEM* item : m_items )
    {
        for( const auto& it : item->m_connection_map )
        {
            const auto sheet = it.first;
            auto connection = it.second;

            if( connection->SubgraphCode() == 0 )
            {
                CONNECTION_SUBGRAPH* subgraph = new CONNECTION_SUBGRAPH( this );

                subgraph->m_code = m_last_subgraph_code++;
                subgraph->m_sheet = sheet;

                subgraph->AddItem( item );

                connection->SetSubgraphCode( subgraph->m_code );
                m_item_to_subgraph_map[item] = subgraph;

                std::list<SCH_ITEM*> members;

                auto get_items =
                        [&]( SCH_ITEM* aItem ) -> bool
                        {
                            SCH_CONNECTION* conn = aItem->Connection( &sheet );

                            if( !conn )
                                conn = aItem->InitializeConnection( sheet, this );

                            return ( conn->SubgraphCode() == 0 );
                        };

                std::copy_if( item->ConnectedItems( sheet ).begin(),
                              item->ConnectedItems( sheet ).end(),
                              std::back_inserter( members ), get_items );

                for( SCH_ITEM* connected_item : members )
                {
                    if( connected_item->Type() == SCH_NO_CONNECT_T )
                        subgraph->m_no_connect = connected_item;

                    SCH_CONNECTION* connected_conn = connected_item->Connection( &sheet );

                    wxASSERT( connected_conn );

                    if( connected_conn->SubgraphCode() == 0 )
                    {
                        connected_conn->SetSubgraphCode( subgraph->m_code );
                        m_item_to_subgraph_map[connected_item] = subgraph;
                        subgraph->AddItem( connected_item );

                        std::copy_if( connected_item->ConnectedItems( sheet ).begin(),
                                      connected_item->ConnectedItems( sheet ).end(),
                                      std::back_inserter( members ), get_items );
                    }
                }

                subgraph->m_dirty = true;
                m_subgraphs.push_back( subgraph );
            }
        }
    }

    /**
     * TODO(JE): Net codes are non-deterministic.  Fortunately, they are also not really used for
     * anything. We should consider removing them entirely and just using net names everywhere.
     */

    // Resolve drivers for subgraphs and propagate connectivity info

    // We don't want to spin up a new thread for fewer than 8 nets (overhead costs)
    size_t parallelThreadCount = std::min<size_t>( std::thread::hardware_concurrency(),
            ( m_subgraphs.size() + 3 ) / 4 );

    std::atomic<size_t> nextSubgraph( 0 );
    std::vector<std::future<size_t>> returns( parallelThreadCount );
    std::vector<CONNECTION_SUBGRAPH*> dirty_graphs;

    std::copy_if( m_subgraphs.begin(), m_subgraphs.end(), std::back_inserter( dirty_graphs ),
                  [&] ( const CONNECTION_SUBGRAPH* candidate )
                  {
                      return candidate->m_dirty;
                  } );

    auto update_lambda = [&nextSubgraph, &dirty_graphs]() -> size_t
    {
        for( size_t subgraphId = nextSubgraph++; subgraphId < dirty_graphs.size(); subgraphId = nextSubgraph++ )
        {
            auto subgraph = dirty_graphs[subgraphId];

            if( !subgraph->m_dirty )
                continue;

            // Special processing for some items
            for( auto item : subgraph->m_items )
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
        }

        return 1;
    };

    if( parallelThreadCount == 1 )
        update_lambda();
    else
    {
        for( size_t ii = 0; ii < parallelThreadCount; ++ii )
            returns[ii] = std::async( std::launch::async, update_lambda );

        // Finalize the threads
        for( size_t ii = 0; ii < parallelThreadCount; ++ii )
            returns[ii].wait();
    }

    // Now discard any non-driven subgraphs from further consideration

    std::copy_if( m_subgraphs.begin(), m_subgraphs.end(), std::back_inserter( m_driver_subgraphs ),
                  [&] ( const CONNECTION_SUBGRAPH* candidate ) -> bool
                  {
                      return candidate->m_driver;
                  } );

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
                auto pin = static_cast<SCH_PIN*>( driver );
                wxASSERT( pin->IsPowerConnection() );
                m_global_label_cache[name].push_back( subgraph );
                break;
            }
            default:
                wxLogTrace( ConnTrace, "Unexpected strong driver %s",
                            driver->GetSelectMenuText( EDA_UNITS::MILLIMETRES ) );
                break;
            }
        }
    }

    // Generate subgraphs for invisible power pins.  These will be merged with other subgraphs
    // on the same sheet in the next loop.

    std::unordered_map<int, CONNECTION_SUBGRAPH*> invisible_pin_subgraphs;

    for( const auto& it : m_invisible_power_pins )
    {
        SCH_SHEET_PATH sheet = it.first;
        SCH_PIN*       pin   = it.second;

        if( !pin->ConnectedItems( sheet ).empty() && !pin->GetLibPin()->GetParent()->IsPower() )
        {
            // ERC will warn about this: user has wired up an invisible pin
            continue;
        }

        SCH_CONNECTION* connection = pin->Connection( &sheet );

        if( !connection )
            connection = pin->InitializeConnection( sheet, this );

        // If this pin already has a subgraph, don't need to process
        if( connection->SubgraphCode() > 0 )
            continue;

        connection->SetName( pin->GetShownName() );

        int code = assignNewNetCode( *connection );

        connection->SetNetCode( code );

        CONNECTION_SUBGRAPH* subgraph;
        auto                 jj = invisible_pin_subgraphs.find( code );

        if( jj != invisible_pin_subgraphs.end() )
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

            auto key = std::make_pair( subgraph->GetNetName(), code );
            m_net_code_to_subgraphs_map[ key ].push_back( subgraph );
            m_subgraphs.push_back( subgraph );
            m_driver_subgraphs.push_back( subgraph );

            invisible_pin_subgraphs[code] = subgraph;
        }

        connection->SetSubgraphCode( subgraph->m_code );
    }

    // Here we do all the local (sheet) processing of each subgraph, including assigning net
    // codes, merging subgraphs together that use label connections, etc.

    // Cache remaining valid subgraphs by sheet path
    for( auto subgraph : m_driver_subgraphs )
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

        auto create_new_name =
                [&suffix]( SCH_CONNECTION* aConn ) -> wxString
                {
                    wxString newName;

                    // For group buses with a prefix, we can add the suffix to the prefix.
                    // If they don't have a prefix, we force the creation of a prefix so that
                    // two buses don't get inadvertently shorted together.
                    if( aConn->Type() == CONNECTION_TYPE::BUS_GROUP )
                    {
                        wxString prefix = aConn->BusPrefix();

                        if( prefix.empty() )
                            prefix = wxT( "BUS" ); // So result will be "BUS_1{...}"

                        wxString oldName = aConn->Name().AfterFirst( '{' );

                        newName = wxString::Format( "%s_%u{%s", prefix, suffix, oldName );

                        aConn->ConfigureFromLabel( newName );
                    }
                    else
                    {
                        newName = wxString::Format( "%s_%u", aConn->Name(), suffix );
                        aConn->SetSuffix( wxString::Format( "_%u", suffix ) );
                    }

                    suffix++;
                    return newName;
                };

        if( !subgraph->m_strong_driver )
        {
            std::vector<CONNECTION_SUBGRAPH*>* vec = &m_net_name_to_subgraphs_map.at( name );

            // If we are a unique bus vector, check if we aren't actually unique because of another
            // subgraph with a similar bus vector
            if( vec->size() <= 1 && subgraph->m_driver_connection->Type() == CONNECTION_TYPE::BUS )
            {
                wxString prefixOnly = name.BeforeFirst( '[' ) + wxT( "[]" );
                vec = &m_net_name_to_subgraphs_map.at( prefixOnly );
            }

            if( vec->size() > 1 )
            {
                wxString new_name = create_new_name( connection );

                while( m_net_name_to_subgraphs_map.count( new_name ) )
                    new_name = create_new_name( connection );

                wxLogTrace( ConnTrace, "%ld (%s) is weakly driven and not unique. Changing to %s.",
                            subgraph->m_code, name, new_name );

                alg::delete_matching( *vec, subgraph );

                m_net_name_to_subgraphs_map[new_name].emplace_back( subgraph );

                name = new_name;
            }
            else
            {
                // If there is no conflict, promote sheet pins to be strong drivers so that they
                // will be considered below for propagation/merging.

                // It is possible for this to generate a conflict if the sheet pin has the same
                // name as a global label on the same sheet, because global merging will then treat
                // this subgraph as if it had a matching local label.  So, for those cases, we
                // don't apply this promotion

                if( subgraph->m_driver->Type() == SCH_SHEET_PIN_T )
                {
                    bool     conflict    = false;
                    wxString global_name = connection->Name( true );
                    auto     kk          = m_net_name_to_subgraphs_map.find( global_name );

                    if( kk != m_net_name_to_subgraphs_map.end() )
                    {
                        // A global will conflict if it is on the same sheet as this subgraph, since
                        // it would be connected by implicit local label linking
                        std::vector<CONNECTION_SUBGRAPH*>& candidates = kk->second;

                        for( const CONNECTION_SUBGRAPH* candidate : candidates )
                        {
                            if( candidate->m_sheet == sheet )
                                conflict = true;
                        }
                    }

                    if( conflict )
                    {
                        wxLogTrace( ConnTrace,
                                    "%ld (%s) skipped for promotion due to potential conflict",
                                    subgraph->m_code, name );
                    }
                    else
                    {
                        wxLogTrace( ConnTrace,
                                "%ld (%s) weakly driven by unique sheet pin %s, promoting",
                                subgraph->m_code, name,
                                subgraph->m_driver->GetSelectMenuText( EDA_UNITS::MILLIMETRES ) );

                        subgraph->m_strong_driver = true;
                    }
                }
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

        auto add_connections_to_check = [&] ( CONNECTION_SUBGRAPH* aSubgraph ) {
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
                    wxLogTrace( ConnTrace,
                                "%lu (%s): Adding secondary driver %s", aSubgraph->m_code,
                                aSubgraph->m_driver_connection->Name( true ), c->Name( true ) );
                }
            }
        };

        // Now add other strong drivers
        // The actual connection attached to these items will have been overwritten
        // by the chosen driver of the subgraph, so we need to create a dummy connection
        add_connections_to_check( subgraph );

        for( unsigned i = 0; i < connections_to_check.size(); i++ )
        {
            auto member = connections_to_check[i];

            if( member->IsBus() )
            {
                connections_to_check.insert( connections_to_check.end(),
                                             member->Members().begin(),
                                             member->Members().end() );
            }

            wxString test_name = member->Name( true );

            for( auto candidate : candidate_subgraphs )
            {
                if( candidate->m_absorbed )
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

                            if( pin->IsPowerConnection() && pin->GetShownName() == test_name )
                            {
                                match = true;
                                break;
                            }
                        }
                        else
                        {
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
                         wxLogTrace( ConnTrace, "%lu (%s) has bus child %lu (%s)", subgraph->m_code,
                                     connection->Name(), candidate->m_code, member->Name() );

                        subgraph->m_bus_neighbors[member].insert( candidate );
                        candidate->m_bus_parents[member].insert( subgraph );
                    }
                    else
                    {
                        wxLogTrace( ConnTrace, "%lu (%s) absorbs neighbor %lu (%s)",
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

        subgraph->ResolveDrivers();

        if( subgraph->m_driver_connection->IsBus() )
            assignNetCodesToBus( subgraph->m_driver_connection );
        else
            assignNewNetCode( *subgraph->m_driver_connection );

        wxLogTrace( ConnTrace, "Re-resolving drivers for %lu (%s)", subgraph->m_code,
                    subgraph->m_driver_connection->Name() );
    }

    // Absorbed subgraphs should no longer be considered
    alg::delete_if( m_driver_subgraphs, [&]( const CONNECTION_SUBGRAPH* candidate ) -> bool
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

    // Update item connections at this point so that neighbor propagation works
    nextSubgraph.store( 0 );

    auto preliminaryUpdateTask =
            [&]() -> size_t
            {
                for( size_t subgraphId = nextSubgraph++;
                     subgraphId < m_driver_subgraphs.size();
                     subgraphId = nextSubgraph++ )
                {
                    m_driver_subgraphs[subgraphId]->UpdateItemConnections();
                }

                return 1;
            };

    if( parallelThreadCount == 1 )
        preliminaryUpdateTask();
    else
    {
        for( size_t ii = 0; ii < parallelThreadCount; ++ii )
            returns[ii] = std::async( std::launch::async, preliminaryUpdateTask );

        // Finalize the threads
        for( size_t ii = 0; ii < parallelThreadCount; ++ii )
            returns[ii].wait();
    }

    // Next time through the subgraphs, we do some post-processing to handle things like
    // connecting bus members to their neighboring subgraphs, and then propagate connections
    // through the hierarchy

    for( CONNECTION_SUBGRAPH* subgraph : m_driver_subgraphs )
    {
        if( !subgraph->m_dirty )
            continue;

        wxLogTrace( ConnTrace, "Processing %lu (%s) for propagation", subgraph->m_code,
                    subgraph->m_driver_connection->Name() );

        // For subgraphs that are driven by a global (power port or label) and have more
        // than one global driver, we need to seek out other subgraphs driven by the
        // same name as the non-chosen driver and update them to match the chosen one.

        if( !subgraph->m_local_driver && subgraph->m_multiple_drivers )
        {
            for( SCH_ITEM* driver : subgraph->m_drivers )
            {
                if( driver == subgraph->m_driver )
                    continue;

                wxString secondary_name = subgraph->GetNameForDriver( driver );

                if( secondary_name == subgraph->m_driver_connection->Name() )
                    continue;

                bool secondary_is_global = CONNECTION_SUBGRAPH::GetDriverPriority( driver )
                                           >= CONNECTION_SUBGRAPH::PRIORITY::POWER_PIN;

                for( CONNECTION_SUBGRAPH* candidate : global_subgraphs )
                {
                    if( candidate == subgraph )
                        continue;

                    if( !secondary_is_global && candidate->m_sheet != subgraph->m_sheet )
                        continue;

                    SCH_CONNECTION* conn = candidate->m_driver_connection;

                    if( conn->Name() == secondary_name )
                    {
                        wxLogTrace( ConnTrace, "Global %lu (%s) promoted to %s", candidate->m_code,
                                    conn->Name(), subgraph->m_driver_connection->Name() );

                        conn->Clone( *subgraph->m_driver_connection );

                        candidate->m_dirty = false;
                    }
                }
            }
        }

        // This call will handle descending the hierarchy and updating child subgraphs
        propagateToNeighbors( subgraph );
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
        wxASSERT_MSG( !subgraph->m_dirty, "Subgraph not processed by propagateToNeighbors!" );

        if( subgraph->m_bus_parents.size() < 2 )
            continue;

        SCH_CONNECTION* conn = subgraph->m_driver_connection;

        wxLogTrace( ConnTrace, "%lu (%s) has multiple bus parents",
                    subgraph->m_code, conn->Name() );

        wxASSERT( conn->IsNet() );

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
                    wxLogTrace( ConnTrace, "Warning: could not match %s inside %lu (%s)",
                                conn->Name(), parent->m_code, parent->m_driver_connection->Name() );
                    continue;
                }

                if( conn->Name() != match->Name() )
                {
                    wxString old_name = match->Name();

                    wxLogTrace( ConnTrace, "Updating %lu (%s) member %s to %s", parent->m_code,
                                parent->m_driver_connection->Name(), old_name, conn->Name() );

                    match->Clone( *conn );

                    auto jj = m_net_name_to_subgraphs_map.find( old_name );

                    if( jj == m_net_name_to_subgraphs_map.end() )
                        continue;

                    for( CONNECTION_SUBGRAPH* old_sg : jj->second )
                    {
                        while( old_sg->m_absorbed )
                            old_sg = old_sg->m_absorbed_by;

                        old_sg->m_driver_connection->Clone( *conn );
                    }
                }
            }
        }
    }

    nextSubgraph.store( 0 );

    auto updateItemConnectionsTask =
        [&]() -> size_t
        {
            for( size_t subgraphId = nextSubgraph++;
                 subgraphId < m_driver_subgraphs.size();
                 subgraphId = nextSubgraph++ )
            {
                CONNECTION_SUBGRAPH* subgraph = m_driver_subgraphs[subgraphId];

                // Make sure weakly-driven single-pin nets get the unconnected_ prefix
                if( !subgraph->m_strong_driver && subgraph->m_drivers.size() == 1 &&
                    subgraph->m_driver->Type() == SCH_PIN_T )
                {
                    SCH_PIN* pin = static_cast<SCH_PIN*>( subgraph->m_driver );
                    wxString name = pin->GetDefaultNetName( subgraph->m_sheet, true );

                    subgraph->m_driver_connection->ConfigureFromLabel( name );
                }

                subgraph->m_dirty = false;
                subgraph->UpdateItemConnections();

                // No other processing to do on buses
                if( subgraph->m_driver_connection->IsBus() )
                    continue;

                // As a visual aid, we can check sheet pins that are driven by themselves to see
                // if they should be promoted to buses

                if( subgraph->m_driver->Type() == SCH_SHEET_PIN_T )
                {
                    SCH_SHEET_PIN* pin = static_cast<SCH_SHEET_PIN*>( subgraph->m_driver );

                    if( SCH_SHEET* sheet = pin->GetParent() )
                    {
                        wxString    pinText = pin->GetText();
                        SCH_SCREEN* screen  = sheet->GetScreen();

                        for( SCH_ITEM* item : screen->Items().OfType( SCH_HIER_LABEL_T ) )
                        {
                            SCH_HIERLABEL* label = static_cast<SCH_HIERLABEL*>( item );

                            if( label->GetText() == pinText )
                            {
                                SCH_SHEET_PATH path = subgraph->m_sheet;
                                path.push_back( sheet );

                                SCH_CONNECTION* parent_conn = label->Connection( &path );

                                if( parent_conn && parent_conn->IsBus() )
                                    subgraph->m_driver_connection->SetType( CONNECTION_TYPE::BUS );

                                break;
                            }
                        }

                        if( subgraph->m_driver_connection->IsBus() )
                            continue;
                    }
                }
            }

            return 1;
        };

    if( parallelThreadCount == 1 )
        updateItemConnectionsTask();
    else
    {
        for( size_t ii = 0; ii < parallelThreadCount; ++ii )
            returns[ii] = std::async( std::launch::async, updateItemConnectionsTask );

        // Finalize the threads
        for( size_t ii = 0; ii < parallelThreadCount; ++ii )
            returns[ii].wait();
    }

    m_net_code_to_subgraphs_map.clear();
    m_net_name_to_subgraphs_map.clear();

    for( CONNECTION_SUBGRAPH* subgraph : m_driver_subgraphs )
    {
        auto key = std::make_pair( subgraph->GetNetName(),
                                   subgraph->m_driver_connection->NetCode() );
        m_net_code_to_subgraphs_map[ key ].push_back( subgraph );

        m_net_name_to_subgraphs_map[subgraph->m_driver_connection->Name()].push_back( subgraph );
    }
}


int CONNECTION_GRAPH::assignNewNetCode( SCH_CONNECTION& aConnection )
{
    int code;

    auto it = m_net_name_to_code_map.find( aConnection.Name() );

    if( it == m_net_name_to_code_map.end() )
    {
        code = m_last_net_code++;
        m_net_name_to_code_map[ aConnection.Name() ] = code;
    }
    else
    {
        code = it->second;
    }

    aConnection.SetNetCode( code );

    return code;
}


void CONNECTION_GRAPH::assignNetCodesToBus( SCH_CONNECTION* aConnection )
{
    auto connections_to_check( aConnection->Members() );

    for( unsigned i = 0; i < connections_to_check.size(); i++ )
    {
        auto member = connections_to_check[i];

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


void CONNECTION_GRAPH::propagateToNeighbors( CONNECTION_SUBGRAPH* aSubgraph )
{
    SCH_CONNECTION* conn = aSubgraph->m_driver_connection;
    std::vector<CONNECTION_SUBGRAPH*> search_list;
    std::unordered_set<CONNECTION_SUBGRAPH*> visited;
    std::vector<SCH_CONNECTION*> stale_bus_members;

    auto visit =
            [&]( CONNECTION_SUBGRAPH* aParent )
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
                            || visited.count( candidate ) )
                        {
                            continue;
                        }

                        for( SCH_HIERLABEL* label : candidate->m_hier_ports )
                        {
                            if( candidate->GetNameForDriver( label ) == aParent->GetNameForDriver( pin ) )
                            {
                                wxLogTrace( ConnTrace, "%lu: found child %lu (%s)", aParent->m_code,
                                            candidate->m_code, candidate->m_driver_connection->Name() );

                                candidate->m_hier_parent = aParent;

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
                    || visited.count( candidate )
                    || candidate->m_driver_connection->Type() != aParent->m_driver_connection->Type() )
                {
                    continue;
                }

                for( SCH_SHEET_PIN* pin : candidate->m_hier_pins )
                {
                    SCH_SHEET_PATH pin_path = path;
                    pin_path.push_back( pin->GetParent() );

                    if( pin_path != aParent->m_sheet )
                        continue;

                    if( aParent->GetNameForDriver( label ) == candidate->GetNameForDriver( pin ) )
                    {
                        wxLogTrace( ConnTrace, "%lu: found additional parent %lu (%s)",
                                    aParent->m_code, candidate->m_code,
                                    candidate->m_driver_connection->Name() );

                        search_list.push_back( candidate );
                        break;
                    }
                }
            }
      }
    };

    auto propagate_bus_neighbors = [&]( CONNECTION_SUBGRAPH* aParentGraph ) {
        for( const auto& kv : aParentGraph->m_bus_neighbors )
        {
            for( CONNECTION_SUBGRAPH* neighbor : kv.second )
            {
                // May have been absorbed but won't have been deleted
                while( neighbor->m_absorbed )
                    neighbor = neighbor->m_absorbed_by;

                SCH_CONNECTION* parent = aParentGraph->m_driver_connection;

                // Now member may be out of date, since we just cloned the
                // connection from higher up in the hierarchy.  We need to
                // figure out what the actual new connection is.
                SCH_CONNECTION* member = matchBusMember( parent, kv.first.get() );

                if( !member )
                {
                    // Try harder: we might match on a secondary driver
                    for( CONNECTION_SUBGRAPH* sg : kv.second )
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
                    wxLogTrace( ConnTrace, "Could not match bus member %s in %s",
                                kv.first->Name(), parent->Name() );
                    continue;
                }

                auto neighbor_conn = neighbor->m_driver_connection;
                auto neighbor_name = neighbor_conn->Name();

                // Matching name: no update needed
                if( neighbor_name == member->Name() )
                    continue;

                // Was this neighbor already updated from a different sheet?  Don't rename it again
                if( neighbor_conn->Sheet() != neighbor->m_sheet )
                    continue;

                // Safety check against infinite recursion
                wxASSERT( neighbor_conn->IsNet() );

                wxLogTrace( ConnTrace, "%lu (%s) connected to bus member %s (local %s)",
                            neighbor->m_code, neighbor_name, member->Name(), member->LocalName() );

                // Take whichever name is higher priority
                if( CONNECTION_SUBGRAPH::GetDriverPriority( neighbor->m_driver )
                    >= CONNECTION_SUBGRAPH::PRIORITY::POWER_PIN )
                {
                    member->Clone( *neighbor_conn );
                    stale_bus_members.push_back( member );
                }
                else
                {
                    neighbor_conn->Clone( *member );

                    recacheSubgraphName( neighbor, neighbor_name );

                    // Recurse onto this neighbor in case it needs to re-propagate
                    neighbor->m_dirty = true;
                    propagateToNeighbors( neighbor );
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
    if( !aSubgraph->m_hier_ports.empty() && !aSubgraph->m_hier_pins.empty() )
    {
        wxLogTrace( ConnTrace, "%lu (%s) has both hier ports and pins; deferring processing",
                    aSubgraph->m_code, conn->Name() );
        return;
    }
    else if( aSubgraph->m_hier_ports.empty() && aSubgraph->m_hier_pins.empty() )
    {
        wxLogTrace( ConnTrace, "%lu (%s) has no hier pins or ports; marking clean",
                    aSubgraph->m_code, conn->Name() );
        aSubgraph->m_dirty = false;
        return;
    }

    visited.insert( aSubgraph );

    wxLogTrace( ConnTrace, "Propagating %lu (%s) to subsheets",
                aSubgraph->m_code, aSubgraph->m_driver_connection->Name() );

    visit( aSubgraph );

    for( unsigned i = 0; i < search_list.size(); i++ )
    {
        auto child = search_list[i];

        visited.insert( child );

        visit( child );

        child->m_dirty = false;
    }

    // Now, find the best driver for this chain of subgraphs
    CONNECTION_SUBGRAPH*          bestDriver = aSubgraph;
    CONNECTION_SUBGRAPH::PRIORITY highest =
            CONNECTION_SUBGRAPH::GetDriverPriority( aSubgraph->m_driver );
    bool     bestIsStrong = ( highest >= CONNECTION_SUBGRAPH::PRIORITY::HIER_LABEL );
    wxString bestName     = aSubgraph->m_driver_connection->Name();

    // Check if a subsheet has a higher-priority connection to the same net
    if( highest < CONNECTION_SUBGRAPH::PRIORITY::POWER_PIN )
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

            if( ( priority >= CONNECTION_SUBGRAPH::PRIORITY::POWER_PIN ) ||
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
        wxLogTrace( ConnTrace, "%lu (%s) overridden by new driver %lu (%s)",
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
        for( auto stale_member : stale_bus_members )
        {
            for( CONNECTION_SUBGRAPH* subgraph : visited )
            {
                SCH_CONNECTION* member = matchBusMember( subgraph->m_driver_connection,
                                                         stale_member );

                if( !member )
                {
                    wxLogTrace( ConnTrace, "WARNING: failed to match stale member %s in %s.",
                                stale_member->Name(), subgraph->m_driver_connection->Name() );
                    continue;
                }

                wxLogTrace( ConnTrace, "Updating %lu (%s) member %s to %s", subgraph->m_code,
                            subgraph->m_driver_connection->Name(), member->LocalName(),
                            stale_member->Name() );

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
    auto c = std::shared_ptr<SCH_CONNECTION>( nullptr );

    switch( aItem->Type() )
    {
    case SCH_PIN_T:
    {
        auto pin = static_cast<SCH_PIN*>( aItem );

        if( pin->IsPowerConnection() )
            c = std::make_shared<SCH_CONNECTION>( aItem, aSubgraph->m_sheet );

        break;
    }

    case SCH_GLOBAL_LABEL_T:
    case SCH_HIER_LABEL_T:
    case SCH_LABEL_T:
    {
        c = std::make_shared<SCH_CONNECTION>( aItem, aSubgraph->m_sheet );
        break;
    }

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
    wxASSERT( aBusConnection->IsBus() );

    SCH_CONNECTION* match = nullptr;

    if( aBusConnection->Type() == CONNECTION_TYPE::BUS )
    {
        // Vector bus: compare against index, because we allow the name
        // to be different

        for( const auto& bus_member : aBusConnection->Members() )
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
        for( const auto& c : aBusConnection->Members() )
        {
            // Vector inside group: compare names, because for bus groups
            // we expect the naming to be consistent across all usages
            // TODO(JE) explain this in the docs
            if( c->Type() == CONNECTION_TYPE::BUS )
            {
                for( const auto& bus_member : c->Members() )
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
    }

    return match;
}


void CONNECTION_GRAPH::recacheSubgraphName( CONNECTION_SUBGRAPH* aSubgraph,
                                            const wxString& aOldName )
{
    auto it = m_net_name_to_subgraphs_map.find( aOldName );

    if( it != m_net_name_to_subgraphs_map.end() )
    {
        std::vector<CONNECTION_SUBGRAPH*>& vec = it->second;
        alg::delete_matching( vec, aSubgraph );
    }

    wxLogTrace( ConnTrace, "recacheSubgraphName: %s => %s", aOldName,
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

    for( auto&& subgraph : m_subgraphs )
    {
        // Graph is supposed to be up-to-date before calling this
        wxASSERT( !subgraph->m_dirty );

        if( !subgraph->m_driver )
            continue;

        auto sheet = subgraph->m_sheet;
        auto connection = subgraph->m_driver->Connection( &sheet );

        if( !connection->IsBus() )
            continue;

        auto labels = subgraph->GetBusLabels();

        if( labels.size() > 1 )
        {
            bool different = false;
            wxString first = static_cast<SCH_TEXT*>( labels.at( 0 ) )->GetShownText();

            for( unsigned i = 1; i < labels.size(); ++i )
            {
                if( static_cast<SCH_TEXT*>( labels.at( i ) )->GetShownText() != first )
                {
                    different = true;
                    break;
                }
            }

            if( !different )
                continue;

            wxLogTrace( ConnTrace, "SG %ld (%s) has multiple bus labels", subgraph->m_code,
                        connection->Name() );

            ret.push_back( subgraph );
        }
    }

    return ret;
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

    wxASSERT( !it->second.empty() );

    return it->second[0];
}


CONNECTION_SUBGRAPH* CONNECTION_GRAPH::GetSubgraphForItem( SCH_ITEM* aItem )
{
    auto                 it  = m_item_to_subgraph_map.find( aItem );
    CONNECTION_SUBGRAPH* ret = it != m_item_to_subgraph_map.end() ? it->second : nullptr;

    while( ret && ret->m_absorbed )
        ret = ret->m_absorbed_by;

    return ret;
}


int CONNECTION_GRAPH::RunERC()
{
    int error_count = 0;

    wxCHECK_MSG( m_schematic, true, "Null m_schematic in CONNECTION_GRAPH::RunERC" );

    ERC_SETTINGS& settings = m_schematic->ErcSettings();

    // We don't want to run many ERC checks more than once on a given screen even though it may
    // represent multiple sheets with multiple subgraphs.  We can tell these apart by drivers.
    std::set<SCH_ITEM*> seenDriverInstances;

    for( CONNECTION_SUBGRAPH* subgraph : m_subgraphs )
    {
        // There shouldn't be any null sub-graph pointers.
        wxCHECK2( subgraph, continue );

        // Graph is supposed to be up-to-date before calling RunERC()
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

        if( settings.IsTestEnabled( ERCE_NOCONNECT_CONNECTED )
                || settings.IsTestEnabled( ERCE_NOCONNECT_NOT_CONNECTED )
                || settings.IsTestEnabled( ERCE_PIN_NOT_CONNECTED ) )
        {
            if( !ercCheckNoConnects( subgraph ) )
                error_count++;
        }

        if( settings.IsTestEnabled( ERCE_LABEL_NOT_CONNECTED )
                || settings.IsTestEnabled( ERCE_GLOBLABEL ) )
        {
            if( !ercCheckLabels( subgraph ) )
                error_count++;
        }
    }

    // Hierarchical sheet checking is done at the schematic level
    if( settings.IsTestEnabled( ERCE_HIERACHICAL_LABEL )
            || settings.IsTestEnabled( ERCE_PIN_NOT_CONNECTED ) )
    {
        error_count += ercCheckHierSheets();
    }

    return error_count;
}


bool CONNECTION_GRAPH::ercCheckMultipleDrivers( const CONNECTION_SUBGRAPH* aSubgraph )
{
    wxCHECK( aSubgraph, false );
    /*
     * This was changed late in 6.0 to fix https://gitlab.com/kicad/code/kicad/-/issues/9367
     * so I'm going to leave the original code in for just a little while.  If anyone comes
     * across this in 7.0 development (or later), feel free to delete.
     */
#if 0
    if( aSubgraph->m_second_driver )
    {
        SCH_ITEM* primary   = aSubgraph->m_first_driver;
        SCH_ITEM* secondary = aSubgraph->m_second_driver;

        wxPoint pos = primary->Type() == SCH_PIN_T ?
                      static_cast<SCH_PIN*>( primary )->GetTransformedPosition() :
                      primary->GetPosition();

        wxString primaryName   = aSubgraph->GetNameForDriver( primary );
        wxString secondaryName = aSubgraph->GetNameForDriver( secondary );

        wxString msg = wxString::Format( _( "Both %s and %s are attached to the same "
                                            "items; %s will be used in the netlist" ),
                                         primaryName, secondaryName, primaryName );

        std::shared_ptr<ERC_ITEM> ercItem = ERC_ITEM::Create( ERCE_DRIVER_CONFLICT );
        ercItem->SetItems( primary, secondary );
        ercItem->SetErrorMessage( msg );

        SCH_MARKER* marker = new SCH_MARKER( ercItem, pos );
        aSubgraph->m_sheet.LastScreen()->Append( marker );

        return false;
    }
#else
    if( aSubgraph->m_multiple_drivers )
    {
        for( SCH_ITEM* driver : aSubgraph->m_drivers )
        {
            if( driver == aSubgraph->m_driver )
                continue;

            if( driver->Type() == SCH_GLOBAL_LABEL_T
                    || driver->Type() == SCH_HIER_LABEL_T
                    || driver->Type() == SCH_LABEL_T )
            {
                wxString primaryName   = aSubgraph->GetNameForDriver( aSubgraph->m_driver );
                wxString secondaryName = aSubgraph->GetNameForDriver( driver );

                if( primaryName == secondaryName )
                    continue;

                wxString msg = wxString::Format( _( "Both %s and %s are attached to the same "
                                                    "items; %s will be used in the netlist" ),
                                                 primaryName, secondaryName, primaryName );

                std::shared_ptr<ERC_ITEM> ercItem = ERC_ITEM::Create( ERCE_DRIVER_CONFLICT );
                ercItem->SetItems( aSubgraph->m_driver, driver );
                ercItem->SetErrorMessage( msg );

                SCH_MARKER* marker = new SCH_MARKER( ercItem, driver->GetPosition() );
                aSubgraph->m_sheet.LastScreen()->Append( marker );

                return false;
            }
        }
    }
#endif

    return true;
}


bool CONNECTION_GRAPH::ercCheckBusToNetConflicts( const CONNECTION_SUBGRAPH* aSubgraph )
{
    auto sheet = aSubgraph->m_sheet;
    auto screen = sheet.LastScreen();

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
            conn.ConfigureFromLabel( EscapeString( text->GetShownText(), CTX_NETNAME ) );

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
        ercItem->SetItems( net_item, bus_item );

        SCH_MARKER* marker = new SCH_MARKER( ercItem, net_item->GetPosition() );
        screen->Append( marker );

        return false;
    }

    return true;
}


bool CONNECTION_GRAPH::ercCheckBusToBusConflicts( const CONNECTION_SUBGRAPH* aSubgraph )
{
    wxString msg;
    auto sheet = aSubgraph->m_sheet;
    auto screen = sheet.LastScreen();

    SCH_ITEM* label = nullptr;
    SCH_ITEM* port = nullptr;

    for( auto item : aSubgraph->m_items )
    {
        switch( item->Type() )
        {
        case SCH_TEXT_T:
        case SCH_GLOBAL_LABEL_T:
        {
            if( !label && item->Connection( &sheet )->IsBus() )
                label = item;
            break;
        }

        case SCH_SHEET_PIN_T:
        case SCH_HIER_LABEL_T:
        {
            if( !port && item->Connection( &sheet )->IsBus() )
                port = item;
            break;
        }

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
            ercItem->SetItems( label, port );

            SCH_MARKER* marker = new SCH_MARKER( ercItem, label->GetPosition() );
            screen->Append( marker );

            return false;
        }
    }

    return true;
}


bool CONNECTION_GRAPH::ercCheckBusToBusEntryConflicts( const CONNECTION_SUBGRAPH* aSubgraph )
{
    bool conflict = false;
    auto sheet = aSubgraph->m_sheet;
    auto screen = sheet.LastScreen();

    SCH_BUS_WIRE_ENTRY* bus_entry = nullptr;
    SCH_ITEM* bus_wire = nullptr;
    wxString bus_name;

    if( !aSubgraph->m_driver_connection )
    {
        // Incomplete bus entry.  Let the unconnected tests handle it.
        return true;
    }

    for( auto item : aSubgraph->m_items )
    {
        switch( item->Type() )
        {
        case SCH_BUS_WIRE_ENTRY_T:
        {
            if( !bus_entry )
                bus_entry = static_cast<SCH_BUS_WIRE_ENTRY*>( item );
            break;
        }

        default:
            break;
        }
    }

    if( bus_entry && bus_entry->m_connected_bus_item )
    {
        bus_wire = bus_entry->m_connected_bus_item;

        wxASSERT( bus_wire->Type() == SCH_LINE_T );

        // In some cases, the connection list (SCH_CONNECTION*) can be null.
        // Skip null connections.
        if( bus_entry->Connection( &sheet )
                && bus_wire->Type() == SCH_LINE_T
                && bus_wire->Connection( &sheet ) )
        {
            conflict = true;    // Assume a conflict; we'll reset if we find it's OK

            bus_name = bus_wire->Connection( &sheet )->Name();

            wxString test_name = bus_entry->Connection( &sheet )->Name();

            for( const auto& member : bus_wire->Connection( &sheet )->Members() )
            {
                if( member->Type() == CONNECTION_TYPE::BUS )
                {
                    for( const auto& sub_member : member->Members() )
                    {
                        if( sub_member->Name() == test_name )
                            conflict = false;
                    }
                }
                else if( member->Name() == test_name )
                {
                    conflict = false;
                }
            }
        }
    }

    // Don't report warnings if this bus member has been overridden by a higher priority power pin
    // or global label
    if( conflict && CONNECTION_SUBGRAPH::GetDriverPriority( aSubgraph->m_driver )
                       >= CONNECTION_SUBGRAPH::PRIORITY::POWER_PIN )
        conflict = false;

    if( conflict )
    {
        wxString netName = aSubgraph->m_driver_connection->Name();
        wxString msg = wxString::Format( _( "Net %s is graphically connected to bus %s but is not a"
                                            " member of that bus" ),
                                         UnescapeString( netName ),
                                         UnescapeString( bus_name ) );
        std::shared_ptr<ERC_ITEM> ercItem = ERC_ITEM::Create( ERCE_BUS_ENTRY_CONFLICT );
        ercItem->SetItems( bus_entry, bus_wire );
        ercItem->SetErrorMessage( msg );

        SCH_MARKER* marker = new SCH_MARKER( ercItem, bus_entry->GetPosition() );
        screen->Append( marker );

        return false;
    }

    return true;
}


// TODO(JE) Check sheet pins here too?
bool CONNECTION_GRAPH::ercCheckNoConnects( const CONNECTION_SUBGRAPH* aSubgraph )
{
    ERC_SETTINGS&         settings = m_schematic->ErcSettings();
    wxString              msg;
    const SCH_SHEET_PATH& sheet  = aSubgraph->m_sheet;
    SCH_SCREEN*           screen = sheet.LastScreen();
    bool                  ok     = true;

    if( aSubgraph->m_no_connect != nullptr )
    {
        bool has_invalid_items = false;
        bool has_other_items = false;
        SCH_PIN* pin = nullptr;
        std::vector<SCH_ITEM*> invalid_items;
        wxPoint noConnectPos = aSubgraph->m_no_connect->GetPosition();
        double minDist = 0;

        // Any subgraph that contains both a pin and a no-connect should not
        // contain any other driving items.

        for( SCH_ITEM* item : aSubgraph->m_items )
        {
            switch( item->Type() )
            {
            case SCH_PIN_T:
            {
                SCH_PIN* candidate = static_cast<SCH_PIN*>( item );
                double   dist      = VECTOR2I( candidate->GetTransformedPosition() - noConnectPos )
                                            .SquaredEuclideanNorm();

                if( !pin || dist < minDist )
                {
                    pin = candidate;
                    minDist = dist;
                }

                has_invalid_items |= has_other_items;
                has_other_items = true;
                break;
            }

            case SCH_LINE_T:
            case SCH_JUNCTION_T:
            case SCH_NO_CONNECT_T:
                break;

            default:
                has_invalid_items = true;
                has_other_items = true;
                invalid_items.push_back( item );
            }
        }

        if( pin && has_invalid_items && settings.IsTestEnabled( ERCE_NOCONNECT_CONNECTED ) )
        {
            std::shared_ptr<ERC_ITEM> ercItem = ERC_ITEM::Create( ERCE_NOCONNECT_CONNECTED );
            ercItem->SetItems( pin );

            SCH_MARKER* marker = new SCH_MARKER( ercItem, pin->GetTransformedPosition() );
            screen->Append( marker );

            ok = false;
        }

        if( !has_other_items && settings.IsTestEnabled( ERCE_NOCONNECT_NOT_CONNECTED ) )
        {
            std::shared_ptr<ERC_ITEM> ercItem = ERC_ITEM::Create( ERCE_NOCONNECT_NOT_CONNECTED );
            ercItem->SetItems( aSubgraph->m_no_connect );

            SCH_MARKER* marker = new SCH_MARKER( ercItem, aSubgraph->m_no_connect->GetPosition() );
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
                if( !pins.empty() )
                    has_other_connections = true;

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
        SCH_PIN* pin = pins.empty() ? nullptr : pins[0];

        // Check if invisible power input pins connect to anything else via net name,
        // but not for power symbols as the ones in the standard library all have invisible pins
        // and we want to throw unconnected errors for those even if they are connected to other
        // net items by name, because usually failing to connect them graphically is a mistake
        if( pin && !has_other_connections
                && pin->GetType() == ELECTRICAL_PINTYPE::PT_POWER_IN
                && !pin->IsVisible()
                && !pin->GetLibPin()->GetParent()->IsPower() )
        {
            wxString name = pin->Connection( &sheet )->Name();
            wxString local_name = pin->Connection( &sheet )->Name( true );

            if( m_global_label_cache.count( name )  ||
                ( m_local_label_cache.count( std::make_pair( sheet, local_name ) ) ) )
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
            ercItem->SetItems( pin );

            SCH_MARKER* marker = new SCH_MARKER( ercItem, pin->GetTransformedPosition() );
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
                // We only apply this test to power symbols, because other symbols have invisible
                // pins that are meant to be dangling, but the KiCad standard library power symbols
                // have invisible pins that are *not* meant to be dangling.
                if( testPin->GetLibPin()->GetParent()->IsPower()
                        && testPin->ConnectedItems( sheet ).empty()
                        && settings.IsTestEnabled( ERCE_PIN_NOT_CONNECTED ) )
                {
                    std::shared_ptr<ERC_ITEM> ercItem = ERC_ITEM::Create( ERCE_PIN_NOT_CONNECTED );
                    ercItem->SetItems( testPin );

                    SCH_MARKER* marker = new SCH_MARKER( ercItem,
                                                         testPin->GetTransformedPosition() );
                    screen->Append( marker );

                    ok = false;
                }
            }
        }
    }

    return ok;
}


bool CONNECTION_GRAPH::ercCheckFloatingWires( const CONNECTION_SUBGRAPH* aSubgraph )
{
    if( aSubgraph->m_driver )
        return true;

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
        ercItem->SetItems( wires[0],
                           wires.size() > 1 ? wires[1] : nullptr,
                           wires.size() > 2 ? wires[2] : nullptr,
                           wires.size() > 3 ? wires[3] : nullptr );

        SCH_MARKER* marker = new SCH_MARKER( ercItem, wires[0]->GetPosition() );
        screen->Append( marker );

        return false;
    }

    return true;
}


bool CONNECTION_GRAPH::ercCheckLabels( const CONNECTION_SUBGRAPH* aSubgraph )
{
    // Label connection rules:
    // Local labels are flagged if they don't connect to any pins and don't have a no-connect
    // Global labels are flagged if they appear only once, don't connect to any local labels,
    // and don't have a no-connect marker

    // So, if there is a no-connect, we will never generate a warning here
    if( aSubgraph->m_no_connect )
        return true;

    if( !aSubgraph->m_driver_connection )
        return true;

    // Buses are excluded from this test: many users create buses with only a single instance
    // and it's not really a problem as long as the nets in the bus pass ERC
    if( aSubgraph->m_driver_connection->IsBus() )
        return true;

    ERC_SETTINGS& settings            = m_schematic->ErcSettings();
    bool          ok                  = true;
    SCH_TEXT*     text                = nullptr;
    bool          hasOtherConnections = false;
    int           pinCount            = 0;

    for( auto item : aSubgraph->m_items )
    {
        switch( item->Type() )
        {
        case SCH_LABEL_T:
        case SCH_GLOBAL_LABEL_T:
        case SCH_HIER_LABEL_T:
        {
            text = static_cast<SCH_TEXT*>( item );

            // Below, we'll create an ERC if the whole subgraph is unconnected.  But, additionally,
            // we want to error if an individual label in the subgraph is floating, even if it's
            // connected to other valid things by way of another label on the same sheet.

            if( text->IsDangling() && settings.IsTestEnabled( ERCE_LABEL_NOT_CONNECTED ) )
            {
                std::shared_ptr<ERC_ITEM> ercItem = ERC_ITEM::Create( ERCE_LABEL_NOT_CONNECTED );
                ercItem->SetItems( text );

                SCH_MARKER* marker = new SCH_MARKER( ercItem, text->GetPosition() );
                aSubgraph->m_sheet.LastScreen()->Append( marker );
                ok = false;
            }

            break;
        }

        case SCH_PIN_T:
        case SCH_SHEET_PIN_T:
            hasOtherConnections = true;
            pinCount++;
            break;

        default:
            break;
        }
    }

    if( !text )
        return true;

    bool isGlobal = text->Type() == SCH_GLOBAL_LABEL_T;
    int  errCode = isGlobal ? ERCE_GLOBLABEL : ERCE_LABEL_NOT_CONNECTED;

    wxCHECK_MSG( m_schematic, true, "Null m_schematic in CONNECTION_GRAPH::ercCheckLabels" );

    wxString name = EscapeString( text->GetShownText(), CTX_NETNAME );

    if( isGlobal )
    {
        // This will be set to true if the global is connected to a pin above, but we
        // want to reset this to false so that globals get flagged if they only have a
        // single instance connected to a single pin
        hasOtherConnections = ( pinCount > 1 );

        auto it = m_net_name_to_subgraphs_map.find( name );

        if( it != m_net_name_to_subgraphs_map.end() )
        {
            if( it->second.size() > 1 || aSubgraph->m_multiple_drivers )
                hasOtherConnections = true;
        }
    }
    else if( text->Type() == SCH_HIER_LABEL_T )
    {
        // For a hier label, check if the parent pin is connected
        if( aSubgraph->m_hier_parent &&
            ( aSubgraph->m_hier_parent->m_strong_driver ||
                aSubgraph->m_hier_parent->m_drivers.size() > 1) )
        {
            // For now, a simple check: if there is more than one driver, the parent is probably
            // connected elsewhere (because at least one driver will be the hier pin itself)
            hasOtherConnections = true;
        }
    }
    else
    {
        auto pair = std::make_pair( aSubgraph->m_sheet, name );
        auto it = m_local_label_cache.find( pair );

        if( it != m_local_label_cache.end() && it->second.size() > 1 )
            hasOtherConnections = true;
    }

    if( !hasOtherConnections && settings.IsTestEnabled( errCode ) )
    {
        std::shared_ptr<ERC_ITEM> ercItem = ERC_ITEM::Create( errCode );
        ercItem->SetItems( text );

        SCH_MARKER* marker = new SCH_MARKER( ercItem, text->GetPosition() );
        aSubgraph->m_sheet.LastScreen()->Append( marker );

        return false;
    }

    return ok;
}


int CONNECTION_GRAPH::ercCheckHierSheets()
{
    int errors = 0;

    ERC_SETTINGS& settings = m_schematic->ErcSettings();

    for( const SCH_SHEET_PATH& sheet : m_sheetList )
    {
        for( SCH_ITEM* item : sheet.LastScreen()->Items() )
        {
            if( item->Type() != SCH_SHEET_T )
                continue;

            SCH_SHEET* parentSheet = static_cast<SCH_SHEET*>( item );

            std::map<wxString, SCH_SHEET_PIN*> pins;
            std::map<wxString, SCH_HIERLABEL*> labels;

            for( SCH_SHEET_PIN* pin : parentSheet->GetPins() )
            {
                if( settings.IsTestEnabled( ERCE_HIERACHICAL_LABEL ) )
                    pins[pin->GetText()] = pin;

                if( pin->IsDangling() && settings.IsTestEnabled( ERCE_PIN_NOT_CONNECTED ) )
                {
                    std::shared_ptr<ERC_ITEM> ercItem = ERC_ITEM::Create( ERCE_PIN_NOT_CONNECTED );
                    ercItem->SetItems( pin );

                    SCH_MARKER* marker = new SCH_MARKER( ercItem, pin->GetPosition() );
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

                        if( !pins.count( label->GetText() ) )
                            labels[label->GetText()] = label;
                        else
                            matchedPins.insert( label->GetText() );
                    }
                }

                for( const wxString& matched : matchedPins )
                    pins.erase( matched );

                for( const std::pair<const wxString, SCH_SHEET_PIN*>& unmatched : pins )
                {
                    wxString msg = wxString::Format( _( "Sheet pin %s has no matching hierarchical "
                                                        "label inside the sheet" ),
                                                     UnescapeString( unmatched.first ) );

                    std::shared_ptr<ERC_ITEM> ercItem = ERC_ITEM::Create( ERCE_HIERACHICAL_LABEL );
                    ercItem->SetItems( unmatched.second );
                    ercItem->SetErrorMessage( msg );
                    ercItem->SetIsSheetSpecific();

                    SCH_MARKER* marker = new SCH_MARKER( ercItem, unmatched.second->GetPosition() );
                    sheet.LastScreen()->Append( marker );

                    errors++;
                }

                for( const std::pair<const wxString, SCH_HIERLABEL*>& unmatched : labels )
                {
                    wxString msg = wxString::Format( _( "Hierarchical label %s has no matching "
                                                        "sheet pin in the parent sheet" ),
                                                     UnescapeString( unmatched.first ) );

                    std::shared_ptr<ERC_ITEM> ercItem = ERC_ITEM::Create( ERCE_HIERACHICAL_LABEL );
                    ercItem->SetItems( unmatched.second );
                    ercItem->SetErrorMessage( msg );
                    ercItem->SetIsSheetSpecific();

                    SCH_MARKER* marker = new SCH_MARKER( ercItem, unmatched.second->GetPosition() );
                    parentSheet->GetScreen()->Append( marker );

                    errors++;
                }
            }
        }
    }

    return errors;
}
