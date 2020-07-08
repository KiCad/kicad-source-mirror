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

#include <list>
#include <thread>
#include <algorithm>
#include <future>
#include <vector>
#include <unordered_map>
#include <profile.h>
#include <common.h>
#include <erc.h>
#include <sch_bus_entry.h>
#include <sch_component.h>
#include <sch_edit_frame.h>
#include <sch_line.h>
#include <sch_marker.h>
#include <sch_pin.h>
#include <sch_sheet.h>
#include <sch_sheet_path.h>
#include <sch_text.h>
#include <schematic.h>
#include <connection_graph.h>
#include <widgets/ui_common.h>

#include <advanced_config.h> // for realtime connectivity switch


bool CONNECTION_SUBGRAPH::ResolveDrivers( bool aCreateMarkers )
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
                && !static_cast<SCH_PIN*>( item )->GetParentComponent()->IsInNetlist() )
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

                for( auto c : candidates )
                {
                    auto p = static_cast<SCH_SHEET_PIN*>( c );

                    if( p->GetShape() == PINSHEETLABEL_SHAPE::PS_OUTPUT )
                    {
                        m_driver = c;
                        break;
                    }
                }
            }
            else
            {
                // For all other driver types, sort by name
                std::sort( candidates.begin(), candidates.end(),
                           [&]( SCH_ITEM* a, SCH_ITEM* b ) -> bool
                           {
                               SCH_CONNECTION* ac = a->Connection( m_sheet );
                               SCH_CONNECTION* bc = b->Connection( m_sheet );

                               // Ensure we don't pick the subset over the superset
                               if( ac->IsBus() && bc->IsBus() )
                                   return bc->IsSubsetOf( ac );

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
        m_driver_connection = m_driver->Connection( m_sheet );
    else
        m_driver_connection = nullptr;

    if( aCreateMarkers && m_multiple_drivers )
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
            wxPoint pos = candidates[0]->Type() == SCH_PIN_T ?
                              static_cast<SCH_PIN*>( candidates[0] )->GetTransformedPosition() :
                              candidates[0]->GetPosition();

            ERC_ITEM* ercItem = ERC_ITEM::Create( ERCE_DRIVER_CONFLICT );
            ercItem->SetItems( candidates[0], second_item );

            SCH_MARKER* marker = new SCH_MARKER( ercItem, pos );
            m_sheet.LastScreen()->Append( marker );

            // If aCreateMarkers is true, then this is part of ERC check, so we
            // should return false even if the driver was assigned
            return false;
        }
    }

    return aCreateMarkers || ( m_driver != nullptr );
}


wxString CONNECTION_SUBGRAPH::GetNetName() const
{
    if( !m_driver || m_dirty )
        return "";

    if( !m_driver->Connection( m_sheet ) )
    {
#ifdef CONNECTIVITY_DEBUG
        wxASSERT_MSG( false, "Tried to get the net name of an item with no connection" );
#endif

        return "";
    }

    return m_driver->Connection( m_sheet )->Name();
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
            SCH_CONNECTION* label_conn = item->Connection( m_sheet );

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


wxString CONNECTION_SUBGRAPH::GetNameForDriver( SCH_ITEM* aItem ) const
{
    wxString name;

    switch( aItem->Type() )
    {
    case SCH_PIN_T:
    {
        SCH_PIN* pin = static_cast<SCH_PIN*>( aItem );
        name = pin->GetDefaultNetName( m_sheet );
        break;
    }

    case SCH_LABEL_T:
    case SCH_GLOBAL_LABEL_T:
    case SCH_HIER_LABEL_T:
    case SCH_SHEET_PIN_T:
    {
        name = EscapeString( static_cast<SCH_TEXT*>( aItem )->GetShownText(), CTX_NETNAME );
        break;
    }

    default:
        break;
    }

    return name;
}


void CONNECTION_SUBGRAPH::Absorb( CONNECTION_SUBGRAPH* aOther )
{
    wxASSERT( m_sheet == aOther->m_sheet );

    for( SCH_ITEM* item : aOther->m_items )
    {
        item->Connection( m_sheet )->SetSubgraphCode( m_code );
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

    if( aItem->Connection( m_sheet )->IsDriver() )
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
        SCH_CONNECTION* item_conn = item->Connection( m_sheet );

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


void CONNECTION_GRAPH::Recalculate( const SCH_SHEET_LIST& aSheetList, bool aUnconditional )
{
    PROF_COUNTER recalc_time;
    PROF_COUNTER update_items;

    if( aUnconditional )
        Reset();

    for( const SCH_SHEET_PATH& sheet : aSheetList )
    {
        std::vector<SCH_ITEM*> items;

        for( auto item : sheet.LastScreen()->Items() )
        {
            if( item->IsConnectable() && ( aUnconditional || item->IsConnectivityDirty() ) )
                items.push_back( item );
        }

        updateItemConnectivity( sheet, items );

        // UpdateDanglingState() also adds connected items for SCH_TEXT
        sheet.LastScreen()->TestDanglingEnds( &sheet );
    }

    update_items.Stop();
    wxLogTrace( "CONN_PROFILE", "UpdateItemConnectivity() %0.4f ms", update_items.msecs() );

    PROF_COUNTER build_graph;

    buildConnectionGraph();

    build_graph.Stop();
    wxLogTrace( "CONN_PROFILE", "BuildConnectionGraph() %0.4f ms", build_graph.msecs() );

    recalc_time.Stop();
    wxLogTrace( "CONN_PROFILE", "Recalculate time %0.4f ms", recalc_time.msecs() );

#ifndef DEBUG
    // Pressure relief valve for release builds
    const double max_recalc_time_msecs = 250.;

    if( m_allowRealTime && ADVANCED_CFG::GetCfg().m_realTimeConnectivity &&
        recalc_time.msecs() > max_recalc_time_msecs )
    {
        m_allowRealTime = false;
    }
#endif
}


void CONNECTION_GRAPH::updateItemConnectivity( const SCH_SHEET_PATH& aSheet,
                                               const std::vector<SCH_ITEM*>& aItemList )
{
    std::unordered_map< wxPoint, std::vector<SCH_ITEM*> > connection_map;

    for( SCH_ITEM* item : aItemList )
    {
        std::vector< wxPoint > points;
        item->GetConnectionPoints( points );
        item->ConnectedItems( aSheet ).clear();

        if( item->Type() == SCH_SHEET_T )
        {
            for( SCH_SHEET_PIN* pin : static_cast<SCH_SHEET*>( item )->GetPins() )
            {
                if( !pin->Connection( aSheet ) )
                    pin->InitializeConnection( aSheet, this );

                pin->ConnectedItems( aSheet ).clear();
                pin->Connection( aSheet )->Reset();

                connection_map[ pin->GetTextPos() ].push_back( pin );
                m_items.insert( pin );
            }
        }
        else if( item->Type() == SCH_COMPONENT_T )
        {
            SCH_COMPONENT* component = static_cast<SCH_COMPONENT*>( item );

            // TODO(JE) right now this relies on GetSchPins() returning good SCH_PIN pointers
            // that contain good LIB_PIN pointers.  Since these get invalidated whenever the
            // library component is refreshed, the current solution as of ed025972 is to just
            // rebuild the SCH_PIN list when the component is refreshed, and then re-run the
            // connectivity calculations.  This is slow and should be improved before release.
            // See https://gitlab.com/kicad/code/kicad/issues/3784

            for( SCH_PIN* pin : component->GetSchPins( &aSheet ) )
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
                m_items.insert( pin );
            }
        }
        else
        {
            m_items.insert( item );
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
                    if( test_item->Connection( aSheet )->IsBus() )
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
                auto subgraph = new CONNECTION_SUBGRAPH( this );

                subgraph->m_code = m_last_subgraph_code++;
                subgraph->m_sheet = sheet;

                subgraph->AddItem( item );

                connection->SetSubgraphCode( subgraph->m_code );
                m_item_to_subgraph_map[item] = subgraph;

                std::list<SCH_ITEM*> members;

                auto get_items =
                        [&]( SCH_ITEM* aItem ) -> bool
                        {
                            auto* conn = aItem->Connection( sheet );

                            if( !conn )
                                conn = aItem->InitializeConnection( sheet, this );

                            return ( conn->SubgraphCode() == 0 );
                        };

                std::copy_if( item->ConnectedItems( sheet ).begin(),
                              item->ConnectedItems( sheet ).end(),
                              std::back_inserter( members ), get_items );

                for( auto connected_item : members )
                {
                    if( connected_item->Type() == SCH_NO_CONNECT_T )
                        subgraph->m_no_connect = connected_item;

                    auto connected_conn = connected_item->Connection( sheet );

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
     * TODO(JE)
     *
     * It would be good if net codes were preserved as much as possible when
     * generating netlists, so that unnamed nets don't keep shifting around when
     * you regenerate.
     *
     * Right now, we are clearing out the old connections up in
     * UpdateItemConnectivity(), but that is useful information, so maybe we
     * need to just set the dirty flag or something.
     *
     * That way, ResolveDrivers() can check what the driver of the subgraph was
     * previously, and if it is in the situation of choosing between equal
     * candidates for an auto-generated net name, pick the previous one.
     *
     * N.B. the old algorithm solves this by sorting the possible net names
     * alphabetically, so as long as the same refdes components are involved,
     * the net will be the same.
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

            if( !subgraph->ResolveDrivers() )
            {
                subgraph->m_dirty = false;
            }
            else
            {
                // Now the subgraph has only one driver
                SCH_ITEM* driver = subgraph->m_driver;
                SCH_SHEET_PATH sheet = subgraph->m_sheet;
                SCH_CONNECTION* connection = driver->Connection( sheet );

                // TODO(JE) This should live in SCH_CONNECTION probably
                switch( driver->Type() )
                {
                case SCH_LABEL_T:
                case SCH_GLOBAL_LABEL_T:
                case SCH_HIER_LABEL_T:
                {
                    auto text = static_cast<SCH_TEXT*>( driver );
                    connection->ConfigureFromLabel( EscapeString( text->GetShownText(), CTX_NETNAME ) );
                    break;
                }
                case SCH_SHEET_PIN_T:
                {
                    auto pin = static_cast<SCH_SHEET_PIN*>( driver );
                    connection->ConfigureFromLabel( EscapeString( pin->GetShownText(), CTX_NETNAME ) );
                    break;
                }
                case SCH_PIN_T:
                {
                    auto pin = static_cast<SCH_PIN*>( driver );
                    // NOTE(JE) GetDefaultNetName is not thread-safe.
                    connection->ConfigureFromLabel( pin->GetDefaultNetName( sheet ) );

                    break;
                }
                default:
                    wxLogTrace( "CONN", "Driver type unsupported: %s",
                            driver->GetSelectMenuText( EDA_UNITS::MILLIMETRES ) );
                    break;
                }

                connection->SetDriver( driver );
                connection->ClearDirty();

                subgraph->m_dirty = false;
            }
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
                wxLogTrace( "CONN", "Unexpected strong driver %s",
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

        SCH_CONNECTION* connection = pin->Connection( sheet );

        if( !connection )
            connection = pin->InitializeConnection( sheet, this );

        // If this pin already has a subgraph, don't need to process
        if( connection->SubgraphCode() > 0 )
            continue;

        connection->SetName( pin->GetName() );

        int code = assignNewNetCode( *connection );

        connection->SetNetCode( code );

        CONNECTION_SUBGRAPH* subgraph;

        if( invisible_pin_subgraphs.count( code ) )
        {
            subgraph = invisible_pin_subgraphs.at( code );
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

    for( auto it : invisible_pin_subgraphs )
        it.second->UpdateItemConnections();

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

        auto create_new_name = [&] ( SCH_CONNECTION* aConn, wxString aName ) -> wxString
                               {
                                   wxString new_name = wxString::Format( "%s_%u", aName, suffix );
                                   aConn->SetSuffix( wxString::Format( "_%u", suffix ) );
                                   suffix++;
                                   return new_name;
                               };

        if( !subgraph->m_strong_driver )
        {
            auto& vec = m_net_name_to_subgraphs_map.at( name );

            if( vec.size() > 1 )
            {
                wxString new_name = create_new_name( connection, name );

                while( m_net_name_to_subgraphs_map.count( new_name ) )
                    new_name = create_new_name( connection, name );

                wxLogTrace( "CONN", "%ld (%s) is weakly driven and not unique. Changing to %s.",
                            subgraph->m_code, name, new_name );

                vec.erase( std::remove( vec.begin(), vec.end(), subgraph ), vec.end() );

                m_net_name_to_subgraphs_map[new_name].emplace_back( subgraph );

                name = new_name;

                subgraph->UpdateItemConnections();
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
                    size_t   count       = m_net_name_to_subgraphs_map.count( global_name );

                    if( count )
                    {
                        // A global will conflict if it is on the same sheet as this subgraph, since
                        // it would be connected by implicit local label linking
                        auto& candidates = m_net_name_to_subgraphs_map.at( global_name );

                        for( const auto& candidate : candidates )
                        {
                            if( candidate->m_sheet == sheet )
                                conflict = true;
                        }
                    }

                    if( conflict )
                    {
                        wxLogTrace( "CONN",
                                "%ld (%s) skipped for promotion due to potential conflict",
                                subgraph->m_code, name );
                    }
                    else
                    {
                        wxLogTrace( "CONN",
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
            int code = -1;

            if( m_bus_name_to_code_map.count( name ) )
            {
                code = m_bus_name_to_code_map.at( name );
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

        subgraph->UpdateItemConnections();

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

                auto c = getDefaultConnection( possible_driver, aSubgraph->m_sheet );

                if( c )
                {
                    if( c->Type() != aSubgraph->m_driver_connection->Type() )
                        continue;

                    if( c->Name( true ) == aSubgraph->m_driver_connection->Name( true ) )
                        continue;

                    connections_to_check.push_back( c );
                    wxLogTrace( "CONN", "%lu (%s): Adding secondary driver %s", aSubgraph->m_code,
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

                            if( pin->IsPowerConnection() && pin->GetName() == test_name )
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

                            auto text = static_cast<SCH_TEXT*>( driver );

                            if( EscapeString( text->GetShownText(), CTX_NETNAME ) == test_name )
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
                         wxLogTrace( "CONN", "%lu (%s) has bus child %lu (%s)", subgraph->m_code,
                                     connection->Name(), candidate->m_code, member->Name() );

                        subgraph->m_bus_neighbors[member].insert( candidate );
                        candidate->m_bus_parents[member].insert( subgraph );
                    }
                    else
                    {
                        wxLogTrace( "CONN", "%lu (%s) absorbs neighbor %lu (%s)",
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
    for( auto subgraph : invalidated_subgraphs )
    {
        if( subgraph->m_absorbed )
            continue;

        subgraph->ResolveDrivers();

        if( subgraph->m_driver_connection->IsBus() )
            assignNetCodesToBus( subgraph->m_driver_connection );
        else
            assignNewNetCode( *subgraph->m_driver_connection );

        subgraph->UpdateItemConnections();

        wxLogTrace( "CONN", "Re-resolving drivers for %lu (%s)", subgraph->m_code,
                    subgraph->m_driver_connection->Name() );
    }

    // Absorbed subgraphs should no longer be considered
    m_driver_subgraphs.erase( std::remove_if( m_driver_subgraphs.begin(), m_driver_subgraphs.end(),
                              [&] ( const CONNECTION_SUBGRAPH* candidate ) -> bool
                              {
                                  return candidate->m_absorbed;
                              } ),
                              m_driver_subgraphs.end() );

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
    for( auto subgraph : m_driver_subgraphs )
        m_sheet_to_subgraphs_map[ subgraph->m_sheet ].emplace_back( subgraph );

    // Next time through the subgraphs, we do some post-processing to handle things like
    // connecting bus members to their neighboring subgraphs, and then propagate connections
    // through the hierarchy

    for( auto subgraph : m_driver_subgraphs )
    {
        if( !subgraph->m_dirty )
            continue;

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
                        wxLogTrace( "CONN", "Global %lu (%s) promoted to %s", candidate->m_code,
                                    conn->Name(), subgraph->m_driver_connection->Name() );

                        conn->Clone( *subgraph->m_driver_connection );
                        candidate->UpdateItemConnections();

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
        if( subgraph->m_bus_parents.size() < 2 )
            continue;

        SCH_CONNECTION* conn = subgraph->m_driver_connection;

        wxLogTrace( "CONN", "%lu (%s) has multiple bus parents", subgraph->m_code, conn->Name() );

        wxASSERT( conn->IsNet() );

        for( const auto& it : subgraph->m_bus_parents )
        {
            SCH_CONNECTION* link_member = it.first.get();

            for( CONNECTION_SUBGRAPH* parent : it.second )
            {
                while( parent->m_absorbed )
                    parent = parent->m_absorbed_by;

                SCH_CONNECTION* match = matchBusMember( parent->m_driver_connection, link_member );

                if( !match )
                {
                    wxLogTrace( "CONN", "Warning: could not match %s inside %lu (%s)", conn->Name(),
                            parent->m_code, parent->m_driver_connection->Name() );
                    continue;
                }

                if( conn->Name() != match->Name() )
                {
                    wxString old_name = match->Name();

                    wxLogTrace( "CONN", "Updating %lu (%s) member %s to %s", parent->m_code,
                            parent->m_driver_connection->Name(), old_name, conn->Name() );

                    match->Clone( *conn );

                    if( !m_net_name_to_subgraphs_map.count( old_name ) )
                        continue;

                    for( CONNECTION_SUBGRAPH* old_sg : m_net_name_to_subgraphs_map.at( old_name ) )
                    {
                        while( old_sg->m_absorbed )
                            old_sg = old_sg->m_absorbed_by;

                        old_sg->m_driver_connection->Clone( *conn );
                        old_sg->UpdateItemConnections();
                    }
                }
            }
        }
    }

    m_net_code_to_subgraphs_map.clear();
    m_net_name_to_subgraphs_map.clear();

    for( CONNECTION_SUBGRAPH* subgraph : m_driver_subgraphs )
    {
        // Every driven subgraph should have been marked by now
        if( subgraph->m_dirty )
        {
            // TODO(JE) this should be caught by hierarchical sheet port/pin ERC, check this
            // Reset to false so no complaints come up later
            subgraph->m_dirty = false;
        }

        if( subgraph->m_driver_connection->IsBus() )
        {
            // No other processing to do on buses
            continue;
        }
        else
        {
            // As a visual aid, we can check sheet pins that are driven by themselves to see
            // if they should be promoted to buses

            if( subgraph->m_driver->Type() == SCH_SHEET_PIN_T )
            {
                SCH_SHEET_PIN* pin = static_cast<SCH_SHEET_PIN*>( subgraph->m_driver );

                if( SCH_SHEET* sheet = pin->GetParent() )
                {
                    wxString pinText = pin->GetText();

                    for( auto item : sheet->GetScreen()->Items().OfType( SCH_HIER_LABEL_T ) )
                    {
                        auto label = static_cast<SCH_HIERLABEL*>( item );

                        if( label->GetText() == pinText )
                        {
                            SCH_SHEET_PATH path = subgraph->m_sheet;
                            path.push_back( sheet );

                            SCH_CONNECTION* parent_conn = label->Connection( path );

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

        auto key = std::make_pair( subgraph->GetNetName(),
                                   subgraph->m_driver_connection->NetCode() );
        m_net_code_to_subgraphs_map[ key ].push_back( subgraph );

        m_net_name_to_subgraphs_map[subgraph->m_driver_connection->Name()].push_back( subgraph );
    }

    // Clean up and deallocate stale subgraphs
    m_subgraphs.erase( std::remove_if( m_subgraphs.begin(), m_subgraphs.end(),
            [&]( const CONNECTION_SUBGRAPH* sg )
            {
                if( sg->m_absorbed )
                {
                    delete sg;
                    return true;
                }
                else
                {
                    return false;
                }
            } ),
            m_subgraphs.end() );
}


int CONNECTION_GRAPH::assignNewNetCode( SCH_CONNECTION& aConnection )
{
    int code;

    if( m_net_name_to_code_map.count( aConnection.Name() ) )
    {
        code = m_net_name_to_code_map.at( aConnection.Name() );
    }
    else
    {
        code = m_last_net_code++;
        m_net_name_to_code_map[ aConnection.Name() ] = code;
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

    auto visit = [&] ( CONNECTION_SUBGRAPH* aParent ) {
        for( SCH_SHEET_PIN* pin : aParent->m_hier_pins )
        {
            SCH_SHEET_PATH path = aParent->m_sheet;
            path.push_back( pin->GetParent() );

            if( !m_sheet_to_subgraphs_map.count( path ) )
                continue;

            for( auto candidate : m_sheet_to_subgraphs_map.at( path ) )
            {
                if( !candidate->m_strong_driver ||
                    candidate->m_hier_ports.empty() ||
                    visited.count( candidate ) )
                    continue;

                for( SCH_HIERLABEL* label : candidate->m_hier_ports )
                {
                    if( label->GetShownText() == pin->GetShownText() )
                    {
                        wxLogTrace( "CONN", "%lu: found child %lu (%s)", aParent->m_code,
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

            if( !m_sheet_to_subgraphs_map.count( path ) )
                continue;

            for( auto candidate : m_sheet_to_subgraphs_map.at( path ) )
            {
                if( candidate->m_hier_pins.empty() ||
                    visited.count( candidate ) ||
                    ( candidate->m_driver_connection->Type() !=
                      aParent->m_driver_connection->Type() ) )
                    continue;

                for( SCH_SHEET_PIN* pin : candidate->m_hier_pins )
                {
                    SCH_SHEET_PATH pin_path = path;
                    pin_path.push_back( pin->GetParent() );

                    if( pin_path != aParent->m_sheet )
                        continue;

                    if( label->GetShownText() == pin->GetShownText() )
                    {
                        wxLogTrace( "CONN", "%lu: found additional parent %lu (%s)",
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
                                auto c = getDefaultConnection( driver, sheet );
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
                    wxLogTrace( "CONN", "Could not match bus member %s in %s",
                                kv.first->Name(), parent->Name() );
                    continue;
                }

                auto neighbor_conn = neighbor->m_driver_connection;
                auto neighbor_name = neighbor_conn->Name();

                // Matching name: no update needed
                if( neighbor_name == member->Name() )
                    continue;

                // Safety check against infinite recursion
                wxASSERT( neighbor_conn->IsNet() );

                wxLogTrace( "CONN", "%lu (%s) connected to bus member %s (local %s)",
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
                    neighbor->UpdateItemConnections();

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

    // If we don't have any hier pins (i.e. no children), nothing to do
    if( aSubgraph->m_hier_pins.empty() )
    {
        // If we also don't have any parents, we'll never be visited again
        if( aSubgraph->m_hier_ports.empty() )
            aSubgraph->m_dirty = false;

        return;
    }

    // If we do have hier ports, skip this subgraph as it will be visited by a parent
    // TODO(JE) this will leave the subgraph dirty if there is no matching parent subgraph,
    // which should be flagged as an ERC error
    if( !aSubgraph->m_hier_ports.empty() )
        return;

    visited.insert( aSubgraph );

    wxLogTrace( "CONN", "Propagating %lu (%s) to subsheets",
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
    CONNECTION_SUBGRAPH* driver = aSubgraph;
    CONNECTION_SUBGRAPH::PRIORITY highest =
            CONNECTION_SUBGRAPH::GetDriverPriority( aSubgraph->m_driver );

    // Check if a subsheet has a higher-priority connection to the same net
    if( highest < CONNECTION_SUBGRAPH::PRIORITY::POWER_PIN )
    {
        for( CONNECTION_SUBGRAPH* subgraph : visited )
        {
            CONNECTION_SUBGRAPH::PRIORITY priority =
                    CONNECTION_SUBGRAPH::GetDriverPriority( subgraph->m_driver );

            // Upgrade driver to be this subgraph if this subgraph has a power pin or global
            // Also upgrade if we found something with a shorter sheet path (higher in hierarchy)
            // but with an equivalent priority

            if( ( priority >= CONNECTION_SUBGRAPH::PRIORITY::POWER_PIN ) ||
                ( priority >= highest && subgraph->m_sheet.size() < aSubgraph->m_sheet.size() ) )
                driver = subgraph;
        }
    }

    if( driver != aSubgraph )
    {
        wxLogTrace( "CONN", "%lu (%s) overridden by new driver %lu (%s)",
                    aSubgraph->m_code, aSubgraph->m_driver_connection->Name(),
                    driver->m_code, driver->m_driver_connection->Name() );
    }

    conn = driver->m_driver_connection;

    for( CONNECTION_SUBGRAPH* subgraph : visited )
    {
        wxString old_name = subgraph->m_driver_connection->Name();

        subgraph->m_driver_connection->Clone( *conn );
        subgraph->UpdateItemConnections();

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
                wxASSERT( member );

                wxLogTrace( "CONN", "Updating %lu (%s) member %s to %s", subgraph->m_code,
                            subgraph->m_driver_connection->Name(), member->LocalName(),
                            stale_member->Name() );

                member->Clone( *stale_member );

                propagate_bus_neighbors( subgraph );
            }
        }
    }

    aSubgraph->m_dirty = false;
}


std::shared_ptr<SCH_CONNECTION>
CONNECTION_GRAPH::getDefaultConnection( SCH_ITEM* aItem, const SCH_SHEET_PATH& aSheet )
{
    auto c = std::shared_ptr<SCH_CONNECTION>( nullptr );

    switch( aItem->Type() )
    {
    case SCH_PIN_T:
    {
        auto pin = static_cast<SCH_PIN*>( aItem );

        if( pin->IsPowerConnection() )
        {
            c = std::make_shared<SCH_CONNECTION>( aItem, aSheet );
            c->SetGraph( this );
            c->ConfigureFromLabel( pin->GetName() );
        }
        break;
    }

    case SCH_GLOBAL_LABEL_T:
    case SCH_HIER_LABEL_T:
    case SCH_LABEL_T:
    {
        auto text = static_cast<SCH_TEXT*>( aItem );

        c = std::make_shared<SCH_CONNECTION>( aItem, aSheet );
        c->SetGraph( this );
        c->ConfigureFromLabel( EscapeString( text->GetShownText(), CTX_NETNAME ) );
        break;
    }

    default:
        break;
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


void CONNECTION_GRAPH::recacheSubgraphName(
        CONNECTION_SUBGRAPH* aSubgraph, const wxString& aOldName )
{
    if( m_net_name_to_subgraphs_map.count( aOldName ) )
    {
        auto& vec = m_net_name_to_subgraphs_map.at( aOldName );
        vec.erase( std::remove( vec.begin(), vec.end(), aSubgraph ), vec.end() );
    }

    wxLogTrace( "CONN", "recacheSubgraphName: %s => %s", aOldName,
        aSubgraph->m_driver_connection->Name() );

    m_net_name_to_subgraphs_map[aSubgraph->m_driver_connection->Name()].push_back( aSubgraph );
}


std::shared_ptr<BUS_ALIAS> CONNECTION_GRAPH::GetBusAlias( const wxString& aName )
{
    if( m_bus_alias_cache.count( aName ) )
        return m_bus_alias_cache.at( aName );

    return nullptr;
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
        auto connection = subgraph->m_driver->Connection( sheet );

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

            wxLogTrace( "CONN", "SG %ld (%s) has multiple bus labels", subgraph->m_code,
                        connection->Name() );

            ret.push_back( subgraph );
        }
    }

    return ret;
}


CONNECTION_SUBGRAPH* CONNECTION_GRAPH::FindSubgraphByName(
        const wxString& aNetName, const SCH_SHEET_PATH& aPath )
{
    if( !m_net_name_to_subgraphs_map.count( aNetName ) )
        return nullptr;

    for( auto sg : m_net_name_to_subgraphs_map.at( aNetName ) )
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
    if( !m_net_name_to_subgraphs_map.count( aNetName ) )
        return nullptr;

    wxASSERT( !m_net_name_to_subgraphs_map.at( aNetName ).empty() );

    return m_net_name_to_subgraphs_map.at( aNetName )[0];
}


CONNECTION_SUBGRAPH* CONNECTION_GRAPH::GetSubgraphForItem( SCH_ITEM* aItem )
{
    if( !m_item_to_subgraph_map.count( aItem ) )
        return nullptr;

    CONNECTION_SUBGRAPH* ret = m_item_to_subgraph_map.at( aItem );

    while( ret->m_absorbed )
        ret = ret->m_absorbed_by;

    return ret;
}


int CONNECTION_GRAPH::RunERC()
{
    int error_count = 0;

    wxCHECK_MSG( m_schematic, true, "Null m_schematic in CONNECTION_GRAPH::ercCheckLabels" );

    ERC_SETTINGS& settings = m_schematic->ErcSettings();

    for( auto&& subgraph : m_subgraphs )
    {
        // Graph is supposed to be up-to-date before calling RunERC()
        wxASSERT( !subgraph->m_dirty );

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

        if( settings.IsTestEnabled( ERCE_DRIVER_CONFLICT ) && !subgraph->ResolveDrivers() )
            error_count++;

        if( settings.IsTestEnabled( ERCE_BUS_TO_NET_CONFLICT )
                && !ercCheckBusToNetConflicts( subgraph ) )
            error_count++;

        if( settings.IsTestEnabled( ERCE_BUS_ENTRY_CONFLICT )
                && !ercCheckBusToBusEntryConflicts( subgraph ) )
            error_count++;

        if( settings.IsTestEnabled( ERCE_BUS_TO_BUS_CONFLICT )
                && !ercCheckBusToBusConflicts( subgraph ) )
            error_count++;

        // The following checks are always performed since they don't currently
        // have an option exposed to the user

        if( !ercCheckNoConnects( subgraph ) )
            error_count++;

        if( ( settings.IsTestEnabled( ERCE_LABEL_NOT_CONNECTED )
                || settings.IsTestEnabled( ERCE_GLOBLABEL ) ) && !ercCheckLabels( subgraph ) )
            error_count++;
    }

    return error_count;
}


bool CONNECTION_GRAPH::ercCheckBusToNetConflicts( const CONNECTION_SUBGRAPH* aSubgraph )
{
    auto sheet = aSubgraph->m_sheet;
    auto screen = sheet.LastScreen();

    SCH_ITEM* net_item = nullptr;
    SCH_ITEM* bus_item = nullptr;
    SCH_CONNECTION conn( this );

    for( auto item : aSubgraph->m_items )
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
        ERC_ITEM* ercItem = ERC_ITEM::Create( ERCE_BUS_TO_NET_CONFLICT );
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
            if( !label && item->Connection( sheet )->IsBus() )
                label = item;
            break;
        }

        case SCH_SHEET_PIN_T:
        case SCH_HIER_LABEL_T:
        {
            if( !port && item->Connection( sheet )->IsBus() )
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

        for( const auto& member : label->Connection( sheet )->Members() )
        {
            for( const auto& test : port->Connection( sheet )->Members() )
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
            ERC_ITEM* ercItem = ERC_ITEM::Create( ERCE_BUS_TO_BUS_CONFLICT );
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
        if( bus_entry->Connection( sheet ) && bus_wire->Type() == SCH_LINE_T
            && bus_wire->Connection( sheet ) )
        {
            conflict = true;

            auto test_name = bus_entry->Connection( sheet )->Name( true );

            for( const auto& member : bus_wire->Connection( sheet )->Members() )
            {
                if( member->Type() == CONNECTION_TYPE::BUS )
                {
                    for( const auto& sub_member : member->Members() )
                    {
                        if( sub_member->Name( true ) == test_name )
                            conflict = false;
                    }
                }
                else if( member->Name( true ) == test_name )
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
        ERC_ITEM* ercItem = ERC_ITEM::Create( ERCE_BUS_ENTRY_CONFLICT );
        ercItem->SetItems( bus_entry, bus_wire );

        SCH_MARKER* marker = new SCH_MARKER( ercItem, bus_entry->GetPosition() );
        screen->Append( marker );

        return false;
    }

    return true;
}


// TODO(JE) Check sheet pins here too?
bool CONNECTION_GRAPH::ercCheckNoConnects( const CONNECTION_SUBGRAPH* aSubgraph )
{
    wxString msg;
    auto sheet = aSubgraph->m_sheet;
    auto screen = sheet.LastScreen();

    if( aSubgraph->m_no_connect != nullptr )
    {
        bool has_invalid_items = false;
        bool has_other_items = false;
        SCH_PIN* pin = nullptr;
        std::vector<SCH_ITEM*> invalid_items;

        // Any subgraph that contains both a pin and a no-connect should not
        // contain any other driving items.

        for( auto item : aSubgraph->m_items )
        {
            switch( item->Type() )
            {
            case SCH_PIN_T:
                pin = static_cast<SCH_PIN*>( item );
                has_other_items = true;
                break;

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

        if( pin && has_invalid_items )
        {
            ERC_ITEM* ercItem = ERC_ITEM::Create( ERCE_NOCONNECT_CONNECTED );
            ercItem->SetItems( pin );

            SCH_MARKER* marker = new SCH_MARKER( ercItem, pin->GetTransformedPosition() );
            screen->Append( marker );

            return false;
        }

        if( !has_other_items )
        {
            ERC_ITEM* ercItem = ERC_ITEM::Create( ERCE_NOCONNECT_NOT_CONNECTED );
            ercItem->SetItems( aSubgraph->m_no_connect );

            SCH_MARKER* marker = new SCH_MARKER( ercItem, aSubgraph->m_no_connect->GetPosition() );
            screen->Append( marker );

            return false;
        }
    }
    else
    {
        bool has_other_connections = false;
        SCH_PIN* pin = nullptr;

        // Any subgraph that lacks a no-connect and contains a pin should also
        // contain at least one other connectable item.

        for( auto item : aSubgraph->m_items )
        {
            switch( item->Type() )
            {
            case SCH_PIN_T:
                if( !pin )
                    pin = static_cast<SCH_PIN*>( item );
                else
                    has_other_connections = true;

                break;

            default:
                if( item->IsConnectable() )
                    has_other_connections = true;

                break;
            }
        }

        // Check if invisible power pins connect to anything else.
        // Note this won't catch a component with multiple invisible power pins but these don't
        // connect to any other net; maybe that should be added as a further optional ERC check?

        if( pin && !has_other_connections && pin->IsPowerConnection() && !pin->IsVisible() )
        {
            wxString name = pin->Connection( sheet )->Name();
            wxString local_name = pin->Connection( sheet )->Name( true );

            if( m_global_label_cache.count( name )  ||
                ( m_local_label_cache.count( std::make_pair( sheet, local_name ) ) ) )
            {
                has_other_connections = true;
            }
        }

        if( pin && !has_other_connections && pin->GetType() != ELECTRICAL_PINTYPE::PT_NC )
        {
            ERC_ITEM* ercItem = ERC_ITEM::Create( ERCE_PIN_NOT_CONNECTED );
            ercItem->SetItems( pin );

            SCH_MARKER* marker = new SCH_MARKER( ercItem, pin->GetTransformedPosition() );
            screen->Append( marker );

            return false;
        }
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

    SCH_TEXT* text = nullptr;
    bool has_other_connections = false;

    for( auto item : aSubgraph->m_items )
    {
        switch( item->Type() )
        {
        case SCH_LABEL_T:
        case SCH_GLOBAL_LABEL_T:
        case SCH_HIER_LABEL_T:
            text = static_cast<SCH_TEXT*>( item );
            break;

        case SCH_PIN_T:
        case SCH_SHEET_PIN_T:
            has_other_connections = true;
            break;

        default:
            break;
        }
    }

    if( !text )
        return true;

    bool is_global = text->Type() == SCH_GLOBAL_LABEL_T;

    wxCHECK_MSG( m_schematic, true, "Null m_schematic in CONNECTION_GRAPH::ercCheckLabels" );

    // Global label check can be disabled independently
    if( !m_schematic->ErcSettings().IsTestEnabled( ERCE_GLOBLABEL ) && is_global )
        return true;

    wxString name = EscapeString( text->GetShownText(), CTX_NETNAME );

    if( is_global )
    {
        // This will be set to true if the global is connected to a pin above, but we
        // want to reset this to false so that globals get flagged if they only have a
        // single instance
        has_other_connections = false;

        if( m_net_name_to_subgraphs_map.count( name )
                && m_net_name_to_subgraphs_map.at( name ).size() > 1 )
            has_other_connections = true;
    }
    else if( text->Type() == SCH_HIER_LABEL_T )
    {
        // For a hier label, check if the parent pin is connected
        if( aSubgraph->m_hier_parent &&
            ( aSubgraph->m_hier_parent->m_strong_driver ||
                aSubgraph->m_hier_parent->m_drivers.size() > 1))
        {
            // For now, a simple check: if there is more than one driver, the parent is probably
            // connected elsewhere (because at least one driver will be the hier pin itself)
            has_other_connections = true;
        }
    }
    else
    {
        auto pair = std::make_pair( aSubgraph->m_sheet, name );

        if( m_local_label_cache.count( pair ) && m_local_label_cache.at( pair ).size() > 1 )
            has_other_connections = true;
    }

    if( !has_other_connections )
    {
        ERC_ITEM* ercItem = ERC_ITEM::Create( is_global ? ERCE_GLOBLABEL : ERCE_LABEL_NOT_CONNECTED );
        ercItem->SetItems( text );

        SCH_MARKER* marker = new SCH_MARKER( ercItem, text->GetPosition() );
        aSubgraph->m_sheet.LastScreen()->Append( marker );

        return false;
    }

    return true;
}
