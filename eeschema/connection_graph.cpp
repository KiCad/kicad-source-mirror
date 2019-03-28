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
#include <unordered_map>
#include <profile.h>

#include <common.h>
#include <erc.h>
#include <sch_edit_frame.h>
#include <sch_bus_entry.h>
#include <sch_component.h>
#include <sch_line.h>
#include <sch_pin_connection.h>
#include <sch_screen.h>
#include <sch_sheet.h>
#include <sch_sheet_path.h>
#include <sch_text.h>

#include <connection_graph.h>

using std::map;
using std::unordered_map;
using std::unordered_set;
using std::vector;


bool CONNECTION_SUBGRAPH::ResolveDrivers( bool aCreateMarkers )
{
    int highest_priority = -1;
    vector<SCH_ITEM*> candidates;

    m_driver = nullptr;

    // Hierarchical labels are lower priority than local labels here,
    // because on the first pass we want local labels to drive subgraphs
    // so that we can identify same-sheet neighbors and link them together.
    // Hierarchical labels will end up overriding the final net name if
    // a higher-level sheet has a different name during the hierarchical
    // pass.

    for( auto item : m_drivers )
    {
        int item_priority = 0;

        switch( item->Type() )
        {
        case SCH_SHEET_PIN_T:           item_priority = 2; break;
        case SCH_HIERARCHICAL_LABEL_T:  item_priority = 3; break;
        case SCH_LABEL_T:               item_priority = 4; break;
        case SCH_PIN_CONNECTION_T:
        {
            auto pin_connection = static_cast<SCH_PIN_CONNECTION*>( item );

            if( pin_connection->m_pin->IsPowerConnection() )
                item_priority = 5;
            else
                item_priority = 1;

            // Skip power flags, etc
            if( item_priority == 1 && !pin_connection->m_comp->IsInNetlist() )
                continue;

            break;
        }
        case SCH_GLOBAL_LABEL_T:        item_priority = 6; break;
        default: break;
        }

        if( item_priority > highest_priority )
        {
            candidates.clear();
            candidates.push_back( item );
            highest_priority = item_priority;
        }
        else if( candidates.size() && ( item_priority == highest_priority ) )
        {
            candidates.push_back( item );
        }
    }

    if( highest_priority >= 4 )
        m_strong_driver = true;

    if( candidates.size() )
    {
        if( candidates.size() > 1 )
        {
            if( highest_priority == 1 || highest_priority == 5 )
            {
                // We have multiple options and they are all component pins.
                std::sort( candidates.begin(), candidates.end(),
                           [this]( SCH_ITEM* a, SCH_ITEM* b) -> bool
                            {
                                auto pin_a = static_cast<SCH_PIN_CONNECTION*>( a );
                                auto pin_b = static_cast<SCH_PIN_CONNECTION*>( b );

                                auto name_a = pin_a->GetDefaultNetName( m_sheet );
                                auto name_b = pin_b->GetDefaultNetName( m_sheet );

                                return name_a < name_b;
                            } );
            }

            if( highest_priority == 2 )
            {
                // We have multiple options, and they are all hierarchical
                // sheet pins.  Let's prefer outputs over inputs.

                for( auto c : candidates )
                {
                    auto p = static_cast<SCH_SHEET_PIN*>( c );

                    if( p->GetShape() == NET_OUTPUT )
                    {
                        m_driver = c;
                        break;
                    }
                }
            }
        }

        if( !m_driver )
            m_driver = candidates[0];
    }

    // For power connections, we allow multiple drivers
    if( highest_priority == 5 && candidates.size() > 1 &&
        candidates[0]->Type() == SCH_PIN_CONNECTION_T )
    {
        auto pc = static_cast<SCH_PIN_CONNECTION*>( candidates[0] );

        wxASSERT( pc->m_pin->IsPowerConnection() );

        m_multiple_power_ports = true;
    }

    if( aCreateMarkers && !m_multiple_power_ports &&
        candidates.size() > 1 && highest_priority > 1 )
    {
        // First check if all the candidates are actually the same
        bool same = true;
        auto first = candidates[0]->Connection( m_sheet )->Name();

        for( unsigned i = 1; i < candidates.size(); i++ )
        {
            if( candidates[i]->Connection( m_sheet )->Name() != first )
            {
                same = false;
                break;
            }
        }

        if( !same )
        {
            wxString msg;
            msg.Printf( _( "%s and %s are both attached to the same wires. "
                           "%s was picked as the label to use for netlisting." ),
                        candidates[0]->GetSelectMenuText( m_frame->GetUserUnits() ),
                        candidates[1]->GetSelectMenuText( m_frame->GetUserUnits() ),
                        candidates[0]->Connection( m_sheet )->Name() );

            wxASSERT( candidates[0] != candidates[1] );

            auto marker = new SCH_MARKER();
            marker->SetTimeStamp( GetNewTimeStamp() );
            marker->SetMarkerType( MARKER_BASE::MARKER_ERC );
            marker->SetErrorLevel( MARKER_BASE::MARKER_SEVERITY_WARNING );
            marker->SetData( ERCE_DRIVER_CONFLICT,
                             candidates[0]->GetPosition(), msg,
                             candidates[1]->GetPosition() );

            m_sheet.LastScreen()->Append( marker );

            // If aCreateMarkers is true, then this is part of ERC check, so we
            // should return false even if the driver was assigned
            return false;
        }
    }

    return aCreateMarkers || ( m_driver != nullptr );
}


wxString CONNECTION_SUBGRAPH::GetNetName()
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


std::vector<SCH_ITEM*> CONNECTION_SUBGRAPH::GetBusLabels()
{
    vector<SCH_ITEM*> labels;

    for( auto item : m_drivers )
    {
        switch( item->Type() )
        {
        case SCH_LABEL_T:
        case SCH_GLOBAL_LABEL_T:
        {
            auto label_conn = item->Connection( m_sheet );

            // Only consider bus vectors
            if( label_conn->Type() == CONNECTION_BUS )
                labels.push_back( item );
        }
        default: break;
        }
    }

    return labels;
}


void CONNECTION_GRAPH::Reset()
{
    for( auto sg : m_subgraphs )
        delete sg;

    m_items.clear();
    m_subgraphs.clear();
    m_invisible_power_pins.clear();
    m_bus_alias_cache.clear();
    m_net_name_to_code_map.clear();
    m_bus_name_to_code_map.clear();
    m_net_code_to_subgraphs_map.clear();
    m_last_net_code = 1;
    m_last_bus_code = 1;
    m_last_subgraph_code = 1;
}


void CONNECTION_GRAPH::Recalculate( SCH_SHEET_LIST aSheetList )
{
#ifdef CONNECTIVITY_DEBUG
    PROF_COUNTER phase1;
#endif

    Reset();

    for( const auto& sheet : aSheetList )
    {
        std::vector<SCH_ITEM*> items;

        for( auto item = sheet.LastScreen()->GetDrawItems();
             item; item = item->Next() )
        {
            if( item->IsConnectable() )
            {
                items.push_back( item );
            }
        }

        updateItemConnectivity( sheet, items );
    }

#ifdef CONNECTIVITY_DEBUG
    phase1.Stop();
    std::cout << "UpdateItemConnectivity() " << phase1.msecs() << " ms" << std::endl;
#endif

    // IsDanglingStateChanged() also adds connected items for things like SCH_TEXT
    SCH_SCREENS schematic;
    schematic.TestDanglingEnds();

    buildConnectionGraph();
}


void CONNECTION_GRAPH::updateItemConnectivity( SCH_SHEET_PATH aSheet,
                                               vector<SCH_ITEM*> aItemList )
{
    unordered_map< wxPoint, vector<SCH_ITEM*> > connection_map;

    for( auto item : aItemList )
    {
        vector< wxPoint > points;
        item->GetConnectionPoints( points );
        item->ConnectedItems().clear();

        if( item->Type() == SCH_SHEET_T )
        {
            for( auto& pin : static_cast<SCH_SHEET*>( item )->GetPins() )
            {
                if( !pin.Connection( aSheet ) )
                {
                    pin.InitializeConnection( aSheet );
                }

                pin.ConnectedItems().clear();
                pin.Connection( aSheet )->Reset();

                connection_map[ pin.GetTextPos() ].push_back( &pin );
                m_items.push_back( &pin );
            }
        }
        else if( item->Type() == SCH_COMPONENT_T )
        {
            auto component = static_cast<SCH_COMPONENT*>( item );

            component->UpdatePinConnections( aSheet );

            for( auto it : component->PinConnections() )
            {
                auto pin_connection = it.second;

                // TODO(JE) use cached location from m_Pins
                auto pin_pos = pin_connection->m_pin->GetPosition();
                auto pos = component->GetTransform().TransformCoordinate( pin_pos ) +
                           component->GetPosition();

                // because calling the first time is not thread-safe
                pin_connection->GetDefaultNetName( aSheet );
                pin_connection->ConnectedItems().clear();

                connection_map[ pos ].push_back( pin_connection );
                m_items.push_back( pin_connection );
            }
        }
        else
        {
            m_items.push_back( item );

            if( !item->Connection( aSheet ) )
            {
                item->InitializeConnection( aSheet );
            }

            auto conn = item->Connection( aSheet );

            conn->Reset();

            // Set bus/net property here so that the propagation code uses it
            switch( item->Type() )
            {
            case SCH_LINE_T:
                conn->SetType( ( item->GetLayer() == LAYER_BUS ) ?
                               CONNECTION_BUS : CONNECTION_NET );
                break;

            case SCH_BUS_BUS_ENTRY_T:
                conn->SetType( CONNECTION_BUS );
                break;

            case SCH_PIN_CONNECTION_T:
            case SCH_BUS_WIRE_ENTRY_T:
                conn->SetType( CONNECTION_NET );
                break;

            default:
                break;
            }

            for( auto point : points )
            {
                connection_map[ point ].push_back( item );
            }
        }
    }

    for( auto it : connection_map )
    {
        auto connection_vec = it.second;
        SCH_ITEM* junction = nullptr;

        for( auto connected_item : connection_vec )
        {
            // Look for junctions.  For points that have a junction, we want all
            // items to connect to the junction but not to each other.

            if( connected_item->Type() == SCH_JUNCTION_T )
            {
                junction = connected_item;
            }

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
                    auto screen = aSheet.LastScreen();
                    auto bus = screen->GetBus( it.first );

                    if( bus )
                    {
                        auto bus_entry = static_cast<SCH_BUS_WIRE_ENTRY*>( connected_item );
                        bus_entry->m_connected_bus_item = bus;
                    }
                }
            }

            // Bus-to-bus entries are treated just like bus wires
            if( connected_item->Type() == SCH_BUS_BUS_ENTRY_T )
            {
                if( connection_vec.size() < 2 )
                {
                    auto screen = aSheet.LastScreen();
                    auto bus = screen->GetBus( it.first );

                    if( bus )
                    {
                        auto bus_entry = static_cast<SCH_BUS_BUS_ENTRY*>( connected_item );

                        if( it.first == bus_entry->GetPosition() )
                            bus_entry->m_connected_bus_items[0] = bus;
                        else
                            bus_entry->m_connected_bus_items[1] = bus;

                        bus_entry->ConnectedItems().insert( bus );
                        bus->ConnectedItems().insert( bus_entry );
                    }
                }
            }

            for( auto test_item : connection_vec )
            {
                if( !junction && test_item->Type() == SCH_JUNCTION_T )
                {
                    junction = test_item;
                }

                if( connected_item != test_item &&
                    connected_item != junction &&
                    connected_item->ConnectionPropagatesTo( test_item ) &&
                    test_item->ConnectionPropagatesTo( connected_item ) )
                {
                    connected_item->ConnectedItems().insert( test_item );
                    test_item->ConnectedItems().insert( connected_item );
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
#ifdef CONNECTIVITY_DEBUG
    PROF_COUNTER phase2;
#endif

    // Recache all bus aliases for later use

    SCH_SHEET_LIST all_sheets( g_RootSheet );

    for( unsigned i = 0; i < all_sheets.size(); i++ )
    {
        for( auto alias : all_sheets[i].LastScreen()->GetBusAliases() )
        {
            m_bus_alias_cache[ alias->GetName() ] = alias;
        }
    }

    // Build subgraphs from items (on a per-sheet basis)

    for( auto item : m_items )
    {
        for( auto it : item->m_connection_map )
        {
            const auto sheet = it.first;
            auto connection = it.second;

            if( connection->SubgraphCode() == 0 )
            {
                auto subgraph = new CONNECTION_SUBGRAPH( m_frame );

                subgraph->m_code = m_last_subgraph_code++;
                subgraph->m_sheet = sheet;

                subgraph->m_items.push_back( item );

                if( connection->IsDriver() )
                    subgraph->m_drivers.push_back( item );

                if( item->Type() == SCH_NO_CONNECT_T )
                {
                    subgraph->m_no_connect = item;
                }
                else if( item->Type() == SCH_PIN_CONNECTION_T )
                {
                    auto pc = static_cast<SCH_PIN_CONNECTION*>( item );

                    if( pc->m_pin->GetType() == PIN_NC )
                        subgraph->m_no_connect = item;

                    // Invisible power pins need to be post-processed later

                    if( pc->m_pin->IsPowerConnection() &&
                        !pc->m_pin->IsVisible() )
                    {
                        m_invisible_power_pins.push_back( pc );
                    }
                }

                connection->SetSubgraphCode( subgraph->m_code );

                std::list<SCH_ITEM*> members( item->ConnectedItems().begin(),
                                              item->ConnectedItems().end() );

                for( auto connected_item : members )
                {
                    if( !connected_item->Connection( sheet ) )
                        connected_item->InitializeConnection( sheet );

                    if( connected_item->Type() == SCH_NO_CONNECT_T )
                        subgraph->m_no_connect = connected_item;

                    auto connected_conn = connected_item->Connection( sheet );

                    wxASSERT( connected_conn );

                    if( connected_conn->SubgraphCode() == 0 )
                    {
                        connected_conn->SetSubgraphCode( subgraph->m_code );
                        subgraph->m_items.push_back( connected_item );

                        if( connected_conn->IsDriver() )
                            subgraph->m_drivers.push_back( connected_item );

                        members.insert( members.end(),
                                        connected_item->ConnectedItems().begin(),
                                        connected_item->ConnectedItems().end() );
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

    std::atomic<size_t> nextSubgraph( 0 );
    std::atomic<size_t> threadsFinished( 0 );
    size_t parallelThreadCount = std::max<size_t>( std::thread::hardware_concurrency(), 2 );

    for( size_t ii = 0; ii < parallelThreadCount; ++ii )
    {
        auto t = std::thread( [&]()
        {
            for( size_t subgraphId = nextSubgraph.fetch_add( 1 );
                        subgraphId < static_cast<size_t>( m_subgraphs.size() );
                        subgraphId = nextSubgraph.fetch_add( 1 ) )
            {
                auto subgraph = m_subgraphs[subgraphId];

                if( !subgraph->m_dirty )
                    continue;

                if( !subgraph->ResolveDrivers() )
                {
                    subgraph->m_dirty = false;
                }
                else
                {
                    // Now the subgraph has only one driver
                    auto driver = subgraph->m_driver;
                    auto sheet = subgraph->m_sheet;
                    auto connection = driver->Connection( sheet );

                    // TODO(JE) This should live in SCH_CONNECTION probably
                    switch( driver->Type() )
                    {
                    case SCH_LABEL_T:
                    case SCH_GLOBAL_LABEL_T:
                    case SCH_HIERARCHICAL_LABEL_T:
                    {
                        auto text = static_cast<SCH_TEXT*>( driver );
                        connection->ConfigureFromLabel( text->GetText() );
                        break;
                    }
                    case SCH_SHEET_PIN_T:
                    {
                        auto pin = static_cast<SCH_SHEET_PIN*>( driver );
                        auto txt = pin->GetParent()->GetName() + "/" + pin->GetText();

                        connection->ConfigureFromLabel( txt );
                        break;
                    }
                    case SCH_PIN_CONNECTION_T:
                    {
                        auto pin = static_cast<SCH_PIN_CONNECTION*>( driver );
                        // NOTE(JE) GetDefaultNetName is not thread-safe.
                        connection->ConfigureFromLabel( pin->GetDefaultNetName( sheet ) );

                        break;
                    }
                    default:
                        #ifdef CONNECTIVITY_DEBUG
                        wxLogDebug( "Driver type unsupported: %s",
                                    driver->GetSelectMenuText( MILLIMETRES ) );
                        #endif
                        break;
                    }

                    connection->SetDriver( driver );
                    connection->ClearDirty();

                    subgraph->m_dirty = false;
                }
            }

            threadsFinished++;
        } );

        t.detach();
    }

    while( threadsFinished < parallelThreadCount )
        std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );

    // Check for subgraphs with the same net name but only weak drivers.
    // For example, two wires that are both connected to hierarchical
    // sheet pins that happen to have the same name, but are not the same.

    for( auto subgraph : m_subgraphs )
    {
        subgraph->m_dirty = true;

        if( subgraph->m_strong_driver )
        {
            // Add strong drivers to the cache, for later checking against conflicts

            auto driver = subgraph->m_driver;
            auto conn = subgraph->m_driver->Connection( subgraph->m_sheet );
            auto sheet = subgraph->m_sheet;
            auto name = conn->Name( true );

            switch( driver->Type() )
            {
            case SCH_LABEL_T:
            case SCH_HIERARCHICAL_LABEL_T:
            {
                m_local_label_cache[std::make_pair( sheet, name )].push_back( subgraph );
                break;
            }
            case SCH_GLOBAL_LABEL_T:
            {
                m_global_label_cache[name].push_back( subgraph );
                break;
            }
            case SCH_PIN_CONNECTION_T:
            {
                auto pc = static_cast<SCH_PIN_CONNECTION*>( driver );
                wxASSERT( pc->m_pin->IsPowerConnection() );
                m_global_label_cache[name].push_back( subgraph );
                break;
            }
            default:
                #ifdef CONNECTIVITY_DEBUG
                wxLogDebug( "Unexpected strong driver %s",
                            driver->GetSelectMenuText( MILLIMETRES ) );
                #endif
                break;
            }
        }
    }

    // Test subgraphs for net name conflicts against higher priority subgraphs
    // Suffix is a global increment to make things simpler, that way if we have
    // multiple instances of the same name that needs to get renamed, they will
    // definitely get unique names.  While this will potentially lead to some
    // confusing net names, this is really a corner case and won't happen if
    // users follow best practices to label their nets.
    unsigned suffix = 1;

    for( auto subgraph : m_subgraphs )
    {
        if( !subgraph->m_dirty )
            continue;

        subgraph->m_dirty = false;

        if( !subgraph->m_driver || subgraph->m_strong_driver )
            continue;

        auto conn = subgraph->m_driver->Connection( subgraph->m_sheet );
        auto name = conn->Name();

        bool conflict = false;


        if( name == "/TDO" )
            asm("nop;");

        // First check the caches
        try
        {
            auto v = m_global_label_cache.at( name );
            conflict = true;
        }
        catch( const std::out_of_range& oor )
        {}

        try
        {
            auto local_name = conn->Name( true );
            auto v = m_local_label_cache.at( std::make_pair( subgraph->m_sheet,
                                                             local_name ) );
            conflict = true;
        }
        catch( const std::out_of_range& oor )
        {}

        if( conflict )
        {
            auto new_name = wxString::Format( _( "%s%u" ), name, suffix );

            #ifdef CONNECTIVITY_DEBUG
            wxLogDebug( "Subgraph %ld default name %s conflicts with a label. Changing to %s.",
                        subgraph->m_code, name, new_name );
            #endif

            conn->SetSuffix( wxString::Format( _( "%u" ), suffix ) );
            suffix++;
            name = new_name;
        }

        for( auto candidate : m_subgraphs )
        {
            if( !candidate->m_dirty )
                continue;

            if( candidate == subgraph || !candidate->m_driver || candidate->m_strong_driver )
                continue;

            auto c_conn = candidate->m_driver->Connection( candidate->m_sheet );
            auto check_name = c_conn->Name();

            if( check_name == name )
            {
                auto new_name = wxString::Format( _( "%s%u" ), name, suffix );

                #ifdef CONNECTIVITY_DEBUG
                wxLogDebug( "Subgraph %ld and %ld both have name %s. Changing %ld to %s.",
                            subgraph->m_code, candidate->m_code, name,
                            candidate->m_code, new_name );
                #endif

                c_conn->SetSuffix( wxString::Format( _( "%u" ), suffix ) );

                candidate->m_dirty = false;
                suffix++;
            }
        }
    }

    // Generate net codes

    for( auto subgraph : m_subgraphs )
    {
        if( !subgraph->m_driver )
            continue;

        auto connection = subgraph->m_driver->Connection( subgraph->m_sheet );
        int code;

        auto name = subgraph->GetNetName();

        if( connection->IsBus() )
        {
            try
            {
                code = m_bus_name_to_code_map.at( name );
            }
            catch( const std::out_of_range& oor )
            {
                code = m_last_bus_code++;
                m_bus_name_to_code_map[ name ] = code;
            }

            connection->SetBusCode( code );
        }
        else
        {
            assignNewNetCode( *connection );
        }

        for( auto item : subgraph->m_items )
        {
            auto item_conn = item->Connection( subgraph->m_sheet );

            if( ( connection->IsBus() && item_conn->IsNet() ) ||
                ( connection->IsNet() && item_conn->IsBus() ) )
            {
                continue;
            }

            if( item != subgraph->m_driver )
            {
                item_conn->Clone( *connection );
                item_conn->ClearDirty();
            }
        }

        // Reset the flag for the next loop below
        subgraph->m_dirty = true;

        auto sheet = subgraph->m_sheet;

        auto candidate_subgraphs( m_subgraphs );
        auto connections_to_check( connection->Members() );

        bool contains_hier_stuff = false;

        for( auto item : subgraph->m_items )
        {
            if( item->Type() == SCH_HIERARCHICAL_LABEL_T ||
                item->Type() == SCH_SHEET_PIN_T )
            {
                contains_hier_stuff = true;
                break;
            }
        }

        // TODO(JE) maybe it will be better to form these links eventually,
        // but for now let's only include subgraphs that contain hierarchical
        // links in one direction or another
        if( !contains_hier_stuff )
            continue;

        // For plain nets, just link based on the driver
        if( !connection->IsBus() )
        {
            connections_to_check.push_back( std::make_shared<SCH_CONNECTION>( *connection ) );
        }

        // Look for "neighbors" for subgraphs that have hierarchical connections.
        // These are usually other subgraphs that have local labels on the
        // same sheet and so should be connected together.

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

            for( auto candidate : candidate_subgraphs )
            {
                if( candidate->m_sheet != sheet || !candidate->m_driver || candidate == subgraph )
                    continue;

                auto candidate_connection = candidate->m_driver->Connection( sheet );

                if( !candidate_connection->IsNet() )
                    continue;

                if( candidate_connection->Name( false ) == member->Name( false ) )
                    subgraph->m_neighbor_map[ member ].push_back( candidate );
            }
        }
    }

    // Generate subgraphs for invisible power pins

    for( auto pc : m_invisible_power_pins )
    {
        if( pc->ConnectedItems().size() > 0 && !pc->m_pin->GetParent()->IsPower() )
        {
            // ERC will warn about this: user has wired up an invisible pin
            continue;
        }

        auto name = pc->m_pin->GetName();
        int code = -1;
        auto sheet = all_sheets[0];

        auto connection = pc->Connection( sheet );

        if( !connection )
        {
            pc->InitializeConnection( sheet );
            connection = pc->Connection( sheet );
        }
        else
        {
            continue;
        }

        try
        {
            code = m_net_name_to_code_map.at( name );
        }
        catch( const std::out_of_range& oor )
        {
            code = assignNewNetCode( *connection );
        }

        // Find a subgraph with the same net and just throw this pin on to it.
        // TODO(JE) should there be a dedicated subgraph for invisible pins?
        // Since this is currently done at the very end, the fact that some
        // subgraph will be getting random pins added shouldn't be a problem,
        // but this could be a gotcha if subgraph data is used after the end
        // of this method at some point in the future.

        CONNECTION_SUBGRAPH* subgraph = nullptr;

        try
        {
            auto subgraphs = m_net_code_to_subgraphs_map.at( code );
            subgraph = subgraphs[0];
        }
        catch( const std::out_of_range& oor )
        {
        }

        if( subgraph && subgraph->m_driver )
        {
            auto parent = subgraph->m_driver->Connection( subgraph->m_sheet );
            pc->Connection( sheet )->Clone( *parent );
        }
        else
        {
            subgraph = new CONNECTION_SUBGRAPH( m_frame );
            m_net_code_to_subgraphs_map[ code ].push_back( subgraph );

            subgraph->m_code = m_last_subgraph_code++;
            subgraph->m_sheet = sheet;
            subgraph->m_items.push_back( pc );
            subgraph->m_drivers.push_back( pc );

            subgraph->ResolveDrivers();

            connection->SetSubgraphCode( subgraph->m_code );
        }
    }

    // Collapse net codes between hierarchical sheets

    for( auto it = m_subgraphs.begin(); it < m_subgraphs.end(); it++ )
    {
        auto subgraph = *it;

        if( !subgraph->m_driver || !subgraph->m_dirty )
            continue;

        auto sheet = subgraph->m_sheet;
        auto connection = std::make_shared<SCH_CONNECTION>( *subgraph->m_driver->Connection( sheet ) );

        // Collapse power nets that are shorted together

        if( subgraph->m_multiple_power_ports )
        {
            for( auto obj : subgraph->m_drivers )
            {
                if( obj == subgraph->m_driver )
                    continue;

                auto power_object = dynamic_cast<SCH_PIN_CONNECTION*>( obj );

                wxASSERT( power_object );

                auto name = power_object->GetDefaultNetName( subgraph->m_sheet );
                int code = -1;

                try
                {
                    code = m_net_name_to_code_map.at( name );
                }
                catch( const std::out_of_range& oor )
                {
                    continue;
                }

                for( auto subgraph_to_update : m_subgraphs )
                {
                    if( !subgraph_to_update->m_driver )
                        continue;

                    auto subsheet = subgraph_to_update->m_sheet;
                    auto conn = subgraph_to_update->m_driver->Connection( subsheet );

                    if( conn->IsBus() || conn->NetCode() != code )
                        continue;

                    for( auto item : subgraph_to_update->m_items )
                    {
                        auto item_conn = item->Connection( subsheet );
                        item_conn->Clone( *connection );
                    }
                }
            }
        }

        /**
         * Is this bus in the highest level of hierarchy? That is, does it
         * contain no hierarchical ports to parent sheets?  If so, we process it
         * here.  If not, we continue, since the bus will be reached from one in
         * a higher level sheet.
         */

        bool contains_hier_labels = false;

        for( auto item : subgraph->m_drivers )
        {
            if( item->Type() == SCH_HIERARCHICAL_LABEL_T )
            {
                contains_hier_labels = true;
                break;
            }
        }

        if( contains_hier_labels )
            continue;

        // On the top level sheet, copy the neighbors onto the bus members
        // because the members won't have net codes yet.  Then recurse into the
        // child sheets and propagate those net codes down.

        // TODO(JE) should we assign bus member net codes in the bus first, and
        // then reverse this operation so we overwrite the net codes generated
        // for the neighbors earlier rather than pulling them in?

        if( connection->IsBus() )
        {
            for( auto& kv : subgraph->m_neighbor_map )
            {
                auto member = kv.first;

                int candidate_net_code = 0;

                for( auto neighbor : kv.second )
                {
                    auto neighbor_conn = neighbor->m_driver->Connection( sheet );

                    try
                    {
                        int c = m_net_name_to_code_map.at( neighbor_conn->Name() );

                        if( candidate_net_code == 0 )
                            candidate_net_code = c;
                        else
                        {
                            #ifdef CONNECTIVITY_DEBUG
                            if( c != candidate_net_code )
                                wxASSERT_MSG( false, "More than one net code for a neighbor!" );
                            #endif
                        }
                    }
                    catch( const std::out_of_range& oor )
                    {
                        #ifdef CONNECTIVITY_DEBUG
                        wxASSERT_MSG( false, "No net code found for an existing net" );
                        #endif
                    }

                    member->SetNetCode( candidate_net_code );
                }
            }

            // Some bus members might not have a neighbor to establish a net
            // code, so generate new ones as needed.
            for( auto& member : connection->Members() )
            {
                if( member->IsNet() && member->NetCode() == 0 )
                {
                    assignNewNetCode( *member );
                }
                else if( member->IsBus() )
                {
                    for( auto& sub_member : member->Members() )
                    {
                        if( sub_member->NetCode() == 0 )
                            assignNewNetCode( *sub_member );
                    }
                }
            }
        }

        /**
         * The general plan:
         *
         * Find subsheet subgraphs that match this one (because the driver is a
         * hierarchical label with the same name as a sheet pin on this one).
         *
         * Iterate over the bus members of the subsheet subgraph:
         *
         *     1)  Find the matching bus member of the top level subgraph.
         *         For bus groups this is just a name match (minus path).
         *         For bus vectors the names *don't have to match*, just
         *         the vector index!
         *
         *     2)  Clone the connection of the top level subgraph onto all
         *         the neighbor subgraphs.
         *
         *     3)  Recurse down onto any subsheets connected to the SSSG.
         */

        std::vector<CONNECTION_SUBGRAPH*> child_subgraphs;

        child_subgraphs.push_back( subgraph );

        for( unsigned i = 0; i < child_subgraphs.size(); i++ )
        {
            // child_subgraphs[i] now refers to the "parent" subgraph that we
            // are descending the hierarchy with.  If there are multiple levels
            // of hierarchy, those will get pushed onto child_subgraphs below.

            for( auto item : child_subgraphs[i]->m_items )
            {
                if( item->Type() == SCH_SHEET_PIN_T )
                {
                    auto sp = static_cast<SCH_SHEET_PIN*>( item );
                    auto sp_name = sp->GetText();
                    auto subsheet = child_subgraphs[i]->m_sheet;
                    subsheet.push_back( sp->GetParent() );

                    #ifdef CONNECTIVITY_DEBUG
                    wxLogDebug( wxT("Propagating sheet pin %s on %s with connection %s to subsheet %s"),
                                sp_name, child_subgraphs[i]->m_sheet.PathHumanReadable(),
                                connection->Name(), subsheet.PathHumanReadable() );
                    #endif

                    for( auto candidate : m_subgraphs )
                    {
                        if( !candidate->m_dirty )
                            continue;

                        if( candidate->m_sheet == subsheet && candidate->m_driver )
                        {
                            SCH_ITEM* hier_label = nullptr;

                            for( auto d : candidate->m_drivers )
                            {
                                if( ( d->Type() == SCH_HIERARCHICAL_LABEL_T ) &&
                                    ( static_cast<SCH_HIERLABEL*>( d )->GetText() == sp_name ) )
                                    hier_label = d;
                            }

                            if( hier_label )
                            {
                                #ifdef CONNECTIVITY_DEBUG
                                wxLogDebug( "Found child %s", static_cast<SCH_HIERLABEL*>( hier_label )->GetText() );
                                #endif

                                // We found a subgraph that is a subsheet child of
                                // our top-level subgraph, so let's mark it

                                candidate->m_dirty = false;

                                auto type = hier_label->Connection( subsheet )->Type();

                                bool candidate_has_sheet_pins = false;

                                // Directly update subsheet net connections

                                for( auto c_item : candidate->m_items )
                                {
                                    if( c_item->Type() == SCH_SHEET_PIN_T )
                                        candidate_has_sheet_pins = true;

                                    auto c = c_item->Connection( subsheet );

                                    wxASSERT( c );

                                    if( ( connection->IsBus() && c->IsNet() ) ||
                                        ( connection->IsNet() && c->IsBus() ) )
                                    {
                                        continue;
                                    }

                                    c->Clone( *connection );
                                }

                                // Now propagate to subsheet neighbors
                                for( auto& kv : candidate->m_neighbor_map )
                                {
                                    auto member = kv.first;
                                    std::shared_ptr<SCH_CONNECTION> top_level_conn;

                                    #ifdef CONNECTIVITY_DEBUG
                                    wxLogDebug( "Found child neighbor from member %s",
                                                member->Name() );
                                    #endif

                                    if( type == CONNECTION_BUS_GROUP )
                                    {
                                        // Bus group: match parent by name
                                        for( auto parent_member : connection->Members() )
                                        {
                                            if( parent_member->IsNet() &&
                                                parent_member->Name( true ) == member->Name( true ) )
                                            {
                                                top_level_conn = parent_member;
                                            }
                                            else if( parent_member->IsBus() )
                                            {
                                                for( auto& sub_member : parent_member->Members() )
                                                {
                                                    if( sub_member->Name( true ) == member->Name( true ) )
                                                        top_level_conn = sub_member;
                                                }
                                            }
                                        }
                                    }
                                    else if( type == CONNECTION_BUS )
                                    {
                                        // Bus vector: match parent by index
                                        for( auto parent_member : connection->Members() )
                                        {
                                            if( parent_member->VectorIndex() == member->VectorIndex() )
                                                top_level_conn = parent_member;
                                        }
                                    }
                                    else
                                    {
                                        top_level_conn = connection;
                                    }

                                    // If top_level_conn was not found, probably it's
                                    // an ERC error and will be caught by ERC

                                    if( !top_level_conn )
                                    {
                                        continue;
                                    }

                                    for( auto neighbor : kv.second )
                                    {
                                        #ifdef CONNECTIVITY_DEBUG
                                        wxLogDebug( "Propagating to neighbor driven by %s",
                                                    neighbor->m_driver->GetSelectMenuText( MILLIMETRES ) );
                                        #endif

                                        bool neighbor_has_sheet_pins = false;

                                        for( auto n_item : neighbor->m_items )
                                        {
                                            auto c = n_item->Connection( subsheet );

                                            wxASSERT( c );

                                            c->Clone( *top_level_conn );

                                            if( n_item->Type() == SCH_SHEET_PIN_T )
                                                neighbor_has_sheet_pins = true;
                                        }

                                        if( neighbor_has_sheet_pins )
                                        {
                                            #ifdef CONNECTIVITY_DEBUG
                                            wxLogDebug( "Neighbor driven by %s has subsheet pins",
                                                        neighbor->m_driver->GetSelectMenuText( MILLIMETRES ) );
                                            #endif
                                            child_subgraphs.push_back( neighbor );
                                        }
                                    }
                                }

                                // Now, check to see if the candidate also has
                                // sheet pin members.  If so, add to the queue.
                                if( candidate_has_sheet_pins)
                                {
                                    #ifdef CONNECTIVITY_DEBUG
                                    wxLogDebug( "Candidate %s has subsheet pins",
                                                candidate->m_driver->GetSelectMenuText( MILLIMETRES ) );
                                    #endif
                                    child_subgraphs.push_back( candidate );
                                }
                            }
                        }
                    }
                }
            }
        }

        subgraph->m_dirty = false;
    }

    m_net_code_to_subgraphs_map.clear();

    for( auto subgraph : m_subgraphs )
    {
        if( !subgraph->m_driver )
            continue;

        if( subgraph->m_dirty )
        {
            subgraph->m_dirty = false;
        }

        if( subgraph->m_driver->Connection( subgraph->m_sheet )->IsBus() )
            continue;

        int code = subgraph->m_driver->Connection( subgraph->m_sheet )->NetCode();
        m_net_code_to_subgraphs_map[ code ].push_back( subgraph );
    }

#ifdef CONNECTIVITY_DEBUG
    phase2.Stop();
    std::cout << "BuildConnectionGraph() " <<  phase2.msecs() << " ms" << std::endl;
#endif
}


int CONNECTION_GRAPH::assignNewNetCode( SCH_CONNECTION& aConnection )
{
    int code;

    try
    {
        code = m_net_name_to_code_map.at( aConnection.Name() );
    }
    catch( const std::out_of_range& oor )
    {
        code = m_last_net_code++;
        m_net_name_to_code_map[ aConnection.Name() ] = code;
    }

    aConnection.SetNetCode( code );

    return code;
}


std::shared_ptr<BUS_ALIAS> CONNECTION_GRAPH::GetBusAlias( wxString aName )
{
    try
    {
        return m_bus_alias_cache.at( aName );
    }
    catch( const std::out_of_range& oor )
    {
        return nullptr;
    }
}


std::vector<CONNECTION_SUBGRAPH*> CONNECTION_GRAPH::GetBusesNeedingMigration()
{
    std::vector<CONNECTION_SUBGRAPH*> ret;

    for( auto it = m_subgraphs.begin(); it < m_subgraphs.end(); it++ )
    {
        auto subgraph = *it;

        // Graph is supposed to be up-to-date before calling this
        wxASSERT( !subgraph->m_dirty );

        if( !subgraph->m_driver )
            continue;

        auto sheet = subgraph->m_sheet;
        auto connection = subgraph->m_driver->Connection( sheet );

        if( !connection->IsBus() )
            continue;

        if( subgraph->GetBusLabels().size() > 1 )
        {
            #ifdef CONNECTIVITY_DEBUG
            wxLogDebug( "SG %ld (%s) has multiple bus labels", subgraph->m_code,
                        connection->Name() );
            #endif

            ret.push_back( subgraph );
        }
    }

    return ret;
}


bool CONNECTION_GRAPH::UsesNewBusFeatures() const
{
    for( auto it = m_subgraphs.begin(); it < m_subgraphs.end(); it++ )
    {
        auto subgraph = *it;


        if( !subgraph->m_driver )
            continue;

        auto sheet = subgraph->m_sheet;
        auto connection = subgraph->m_driver->Connection( sheet );

        if( !connection->IsBus() )
            continue;

        if( connection->Type() == CONNECTION_BUS_GROUP )
            return true;
    }

    return false;
}


int CONNECTION_GRAPH::RunERC( const ERC_SETTINGS& aSettings, bool aCreateMarkers )
{
    int error_count = 0;

    for( auto it = m_subgraphs.begin(); it < m_subgraphs.end(); it++ )
    {
        auto subgraph = *it;

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

        if( aSettings.check_bus_driver_conflicts &&
            !subgraph->ResolveDrivers( aCreateMarkers ) )
            error_count++;

        if( aSettings.check_bus_to_net_conflicts &&
            !ercCheckBusToNetConflicts( subgraph, aCreateMarkers ) )
            error_count++;

        if( aSettings.check_bus_entry_conflicts &&
            !ercCheckBusToBusEntryConflicts( subgraph, aCreateMarkers ) )
            error_count++;

        if( aSettings.check_bus_to_bus_conflicts &&
            !ercCheckBusToBusConflicts( subgraph, aCreateMarkers ) )
            error_count++;

        // The following checks are always performed since they don't currently
        // have an option exposed to the user

        if( !ercCheckNoConnects( subgraph, aCreateMarkers ) )
            error_count++;

        if( !ercCheckLabels( subgraph, aCreateMarkers ) )
            error_count++;
    }

    return error_count;
}


bool CONNECTION_GRAPH::ercCheckBusToNetConflicts( CONNECTION_SUBGRAPH* aSubgraph,
                                                  bool aCreateMarkers )
{
    wxString msg;
    auto sheet = aSubgraph->m_sheet;
    auto screen = sheet.LastScreen();

    SCH_ITEM* net_item = nullptr;
    SCH_ITEM* bus_item = nullptr;
    SCH_CONNECTION conn;

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
        case SCH_HIERARCHICAL_LABEL_T:
        {
            auto text = static_cast<SCH_TEXT*>( item )->GetText();
            conn.ConfigureFromLabel( text );

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
        if( aCreateMarkers )
        {
            msg.Printf( _( "%s and %s are graphically connected but cannot"
                           " electrically connect because one is a bus and"
                           " the other is a net." ),
                        bus_item->GetSelectMenuText( m_frame->GetUserUnits() ),
                        net_item->GetSelectMenuText( m_frame->GetUserUnits() ) );

            auto marker = new SCH_MARKER();
            marker->SetTimeStamp( GetNewTimeStamp() );
            marker->SetMarkerType( MARKER_BASE::MARKER_ERC );
            marker->SetErrorLevel( MARKER_BASE::MARKER_SEVERITY_ERROR );
            marker->SetData( ERCE_BUS_TO_NET_CONFLICT,
                             net_item->GetPosition(), msg,
                             bus_item->GetPosition() );

            screen->Append( marker );
        }

        return false;
    }

    return true;
}


bool CONNECTION_GRAPH::ercCheckBusToBusConflicts( CONNECTION_SUBGRAPH* aSubgraph,
                                                  bool aCreateMarkers )
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
        case SCH_HIERARCHICAL_LABEL_T:
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

        for( auto member : label->Connection( sheet )->Members() )
        {
            for( auto test : port->Connection( sheet )->Members() )
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
            if( aCreateMarkers )
            {
                msg.Printf( _( "%s and %s are graphically connected but do "
                               "not share any bus members" ),
                            label->GetSelectMenuText( m_frame->GetUserUnits() ),
                            port->GetSelectMenuText( m_frame->GetUserUnits() ) );

                auto marker = new SCH_MARKER();
                marker->SetTimeStamp( GetNewTimeStamp() );
                marker->SetMarkerType( MARKER_BASE::MARKER_ERC );
                marker->SetErrorLevel( MARKER_BASE::MARKER_SEVERITY_ERROR );
                marker->SetData( ERCE_BUS_TO_BUS_CONFLICT,
                                 label->GetPosition(), msg,
                                 port->GetPosition() );

                screen->Append( marker );
            }

            return false;
        }
    }

    return true;
}


bool CONNECTION_GRAPH::ercCheckBusToBusEntryConflicts( CONNECTION_SUBGRAPH* aSubgraph,
                                                       bool aCreateMarkers )
{
    wxString msg;
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
        conflict = true;

        auto test_name = bus_entry->Connection( sheet )->Name();

        for( auto member : bus_wire->Connection( sheet )->Members() )
        {
            if( member->Type() == CONNECTION_BUS )
            {
                for( const auto& sub_member : member->Members() )
                    if( sub_member->Name() == test_name )
                        conflict = false;
            }
            else if( member->Name() == test_name )
            {
                conflict = false;
            }
        }
    }

    if( conflict )
    {
        if( aCreateMarkers )
        {
            msg.Printf( _( "%s (%s) is connected to %s (%s) but is not a member of the bus" ),
                        bus_entry->GetSelectMenuText( m_frame->GetUserUnits() ),
                        bus_entry->Connection( sheet )->Name(),
                        bus_wire->GetSelectMenuText( m_frame->GetUserUnits() ),
                        bus_wire->Connection( sheet )->Name() );

            auto marker = new SCH_MARKER();
            marker->SetTimeStamp( GetNewTimeStamp() );
            marker->SetMarkerType( MARKER_BASE::MARKER_ERC );
            marker->SetErrorLevel( MARKER_BASE::MARKER_SEVERITY_WARNING );
            marker->SetData( ERCE_BUS_ENTRY_CONFLICT,
                             bus_entry->GetPosition(), msg,
                             bus_entry->GetPosition() );

            screen->Append( marker );
        }

        return false;
    }

    return true;
}


// TODO(JE) Check sheet pins here too?
bool CONNECTION_GRAPH::ercCheckNoConnects( CONNECTION_SUBGRAPH* aSubgraph,
                                           bool aCreateMarkers )
{
    wxString msg;
    auto sheet = aSubgraph->m_sheet;
    auto screen = sheet.LastScreen();

    if( aSubgraph->m_no_connect != nullptr )
    {
        bool has_invalid_items = false;
        SCH_PIN_CONNECTION* pin = nullptr;
        std::vector<SCH_ITEM*> invalid_items;

        // Any subgraph that contains both a pin and a no-connect should not
        // contain any other connectable items.

        for( auto item : aSubgraph->m_items )
        {
            switch( item->Type() )
            {
            case SCH_PIN_CONNECTION_T:
                pin = static_cast<SCH_PIN_CONNECTION*>( item );
                break;

            case SCH_NO_CONNECT_T:
                break;

            default:
                has_invalid_items = true;
                invalid_items.push_back( item );
            }
        }

        // TODO: Should it be an error to have a NC item but no pin?
        if( pin && has_invalid_items )
        {
            auto pos = pin->GetPosition();
            msg.Printf( _( "Pin %s of component %s has a no-connect marker but is connected" ),
                        GetChars( pin->m_pin->GetName() ),
                        GetChars( pin->m_comp->GetRef( &aSubgraph->m_sheet ) ) );

            auto marker = new SCH_MARKER();
            marker->SetTimeStamp( GetNewTimeStamp() );
            marker->SetMarkerType( MARKER_BASE::MARKER_ERC );
            marker->SetErrorLevel( MARKER_BASE::MARKER_SEVERITY_WARNING );
            marker->SetData( ERCE_NOCONNECT_CONNECTED, pos, msg, pos );

            screen->Append( marker );

            return false;
        }
    }
    else
    {
        bool has_other_connections = false;
        SCH_PIN_CONNECTION* pin = nullptr;

        // Any subgraph that lacks a no-connect and contains a pin should also
        // contain at least one other connectable item.

        for( auto item : aSubgraph->m_items )
        {
            switch( item->Type() )
            {
            case SCH_PIN_CONNECTION_T:
                if( !pin )
                    pin = static_cast<SCH_PIN_CONNECTION*>( item );
                else
                    has_other_connections = true;
                break;

            default:
                if( item->IsConnectable() )
                    has_other_connections = true;
                break;
            }
        }

        if( pin && !has_other_connections &&
            pin->m_pin->GetType() != PIN_NC )
        {
            auto pos = pin->GetPosition();
            msg.Printf( _( "Pin %s of component %s is unconnected." ),
                        GetChars( pin->m_pin->GetName() ),
                        GetChars( pin->m_comp->GetRef( &aSubgraph->m_sheet ) ) );

            auto marker = new SCH_MARKER();
            marker->SetTimeStamp( GetNewTimeStamp() );
            marker->SetMarkerType( MARKER_BASE::MARKER_ERC );
            marker->SetErrorLevel( MARKER_BASE::MARKER_SEVERITY_WARNING );
            marker->SetData( ERCE_PIN_NOT_CONNECTED, pos, msg, pos );

            screen->Append( marker );

            return false;
        }
    }

    return true;
}


bool CONNECTION_GRAPH::ercCheckLabels( CONNECTION_SUBGRAPH* aSubgraph,
                                       bool aCreateMarkers )
{
    wxString msg;
    auto sheet = aSubgraph->m_sheet;
    auto screen = sheet.LastScreen();

    SCH_TEXT* text = nullptr;
    bool has_other_connections = false;

    // Any subgraph that contains a label should also contain at least one other
    // connectable item.

    for( auto item : aSubgraph->m_items )
    {
        switch( item->Type() )
        {
        case SCH_LABEL_T:
        case SCH_GLOBAL_LABEL_T:
        case SCH_HIERARCHICAL_LABEL_T:
            text = static_cast<SCH_TEXT*>( item );
            break;

        case SCH_PIN_CONNECTION_T:
            has_other_connections = true;
            break;

        default:
            if( item->IsConnectable() )
                has_other_connections = true;
            break;
        }
    }

    if( text && !has_other_connections )
    {
        auto pos = text->GetPosition();
        msg.Printf( _( "Label %s is unconnected." ),
                    GetChars( text->ShortenedShownText() ) );

        auto marker = new SCH_MARKER();
        marker->SetTimeStamp( GetNewTimeStamp() );
        marker->SetMarkerType( MARKER_BASE::MARKER_ERC );
        marker->SetErrorLevel( MARKER_BASE::MARKER_SEVERITY_WARNING );
        marker->SetData( ERCE_LABEL_NOT_CONNECTED, pos, msg, pos );

        screen->Append( marker );

        return false;
    }

    return true;
}
