/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 CERN
 * Copyright (C) 2021-2022 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <project/project_file.h>
#include <project/net_settings.h>
#include <widgets/ui_common.h>
#include <string_utils.h>
#include <thread_pool.h>
#include <wx/log.h>

#include <advanced_config.h> // for realtime connectivity switch in release builds


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

                    if( p->GetShape() == LABEL_FLAG_SHAPE::L_OUTPUT )
                    {
                        m_driver = c;
                        break;
                    }
                }
            }
            else
            {
                // For all other driver types, sort by quality of name
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

                               // Ensure we don't pick a hidden power pin on a regular symbol over
                               // one on a power symbol
                               if( a->Type() == SCH_PIN_T && b->Type() == SCH_PIN_T )
                               {
                                   SCH_PIN* pa = static_cast<SCH_PIN*>( a );
                                   SCH_PIN* pb = static_cast<SCH_PIN*>( b );

                                   bool aPower = pa->GetLibPin()->GetParent()->IsPower();
                                   bool bPower = pb->GetLibPin()->GetParent()->IsPower();

                                   if( aPower && !bPower )
                                       return true;
                                   else if( bPower && !aPower )
                                       return false;
                               }

                               const wxString& a_name = GetNameForDriver( a );
                               const wxString& b_name = GetNameForDriver( b );
                               bool     a_lowQualityName = a_name.Contains( "-Pad" );
                               bool     b_lowQualityName = b_name.Contains( "-Pad" );

                               if( a_lowQualityName && !b_lowQualityName )
                                   return false;
                               else if( b_lowQualityName && !a_lowQualityName )
                                   return true;
                               else
                                   return a_name < b_name;
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
    else if( !m_is_bus_member )
    {
        m_driver_connection = nullptr;
    }

    if( aCheckMultipleDrivers && m_multiple_drivers )
    {
        // First check if all the candidates are actually the same
        bool same = true;
        const wxString& first = GetNameForDriver( candidates[0] );
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


void CONNECTION_SUBGRAPH::getAllConnectedItems( std::set<std::pair<SCH_SHEET_PATH, SCH_ITEM*>>& aItems, std::set<CONNECTION_SUBGRAPH*>& aSubgraphs )
{
    CONNECTION_SUBGRAPH* sg = this;

    while( sg->m_absorbed_by )
        sg = sg->m_absorbed_by;

    aSubgraphs.insert( sg );
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
        bool forceNoConnect = m_no_connect != nullptr;
        SCH_PIN* pin = static_cast<SCH_PIN*>( aItem );
        return pin->GetDefaultNetName( m_sheet, forceNoConnect );
        break;
    }

    case SCH_LABEL_T:
    case SCH_GLOBAL_LABEL_T:
    case SCH_HIER_LABEL_T:
        return EscapeString( static_cast<SCH_TEXT*>( aItem )->GetShownText( &m_sheet, false ),
                             CTX_NETNAME );

    case SCH_SHEET_PIN_T:
        // Sheet pins need to use their parent sheet as their starting sheet or they will
        // resolve variables on the current sheet first
        return EscapeString( static_cast<SCH_TEXT*>( aItem )->GetShownText( nullptr, false ),
                             CTX_NETNAME );

    default:
        wxFAIL_MSG( wxS( "Unhandled item type in GetNameForDriver" ) );
        break;
    }

    return wxEmptyString;
}


const wxString& CONNECTION_SUBGRAPH::GetNameForDriver( SCH_ITEM* aItem ) const
{
    auto [it, success] = m_driver_name_cache.try_emplace( aItem, driverName( aItem ) );

    return it->second;
}


const wxString CONNECTION_SUBGRAPH::GetNetclassForDriver( SCH_ITEM* aItem ) const
{
    wxString netclass;

    aItem->RunOnChildren(
            [&]( SCH_ITEM* aChild )
            {
                if( aChild->Type() == SCH_FIELD_T )
                {
                    SCH_FIELD* field = static_cast<SCH_FIELD*>( aChild );

                    if( field->GetCanonicalName() == wxT( "Netclass" ) )
                    {
                        netclass = field->GetText();
                        return false;
                    }
                }

                return true;
            } );

    return netclass;
}


void CONNECTION_SUBGRAPH::Absorb( CONNECTION_SUBGRAPH* aOther )
{
    wxASSERT( m_sheet == aOther->m_sheet );

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
        SCH_CONNECTION* item_conn = item->GetOrInitConnection( m_sheet, m_graph );

        if( !item_conn )
            continue;

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
        SCH_PIN* sch_pin = static_cast<SCH_PIN*>( aDriver );
        SCH_SYMBOL* sym = sch_pin->GetParentSymbol();

        if( sch_pin->IsGlobalPower() )
            return PRIORITY::POWER_PIN;
        else if( !sym || sym->GetExcludedFromBoard() || sym->GetLibSymbolRef()->GetReferenceField().GetText().StartsWith( '#' ) )
            return PRIORITY::NONE;
        else
            return PRIORITY::PIN;
    }

    default: return PRIORITY::NONE;
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
        sg->m_graph = this;

    std::copy( aGraph.m_driver_subgraphs.begin(),
            aGraph.m_driver_subgraphs.end(),
            std::back_inserter( m_driver_subgraphs ) );

    std::copy( aGraph.m_global_power_pins.begin(),
            aGraph.m_global_power_pins.end(),
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
}


void CONNECTION_GRAPH::Recalculate( const SCH_SHEET_LIST& aSheetList, bool aUnconditional,
                                    std::function<void( SCH_ITEM* )>* aChangedItemHandler )
{
    PROF_TIMER recalc_time( "CONNECTION_GRAPH::Recalculate" );

    if( aUnconditional )
        Reset();

    PROF_TIMER update_items( "updateItemConnectivity" );

    m_sheetList = aSheetList;

    for( const SCH_SHEET_PATH& sheet : aSheetList )
    {
        std::vector<SCH_ITEM*> items;
        // Store current unit value, to regenerate it after calculations
        // (useful in complex hierarchies)
        std::vector<std::pair<SCH_SYMBOL*, int>> symbolsChanged;

        for( SCH_ITEM* item : sheet.LastScreen()->Items() )
        {
            if( item->IsConnectable() && ( aUnconditional || item->IsConnectivityDirty() ) )
                items.push_back( item );

            // Ensure the hierarchy info stored in SCREENS is built and up to date
            // (multi-unit symbols)
            if( item->Type() == SCH_SYMBOL_T )
            {
                SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );
                int new_unit = symbol->GetUnitSelection( &sheet );

                // Store the initial unit value, to regenerate it after calculations,
                // if modified
                if( symbol->GetUnit() != new_unit )
                    symbolsChanged.push_back( { symbol, symbol->GetUnit() } );

                symbol->UpdateUnit( new_unit );
            }
        }

        m_items.reserve( m_items.size() + items.size() );

        updateItemConnectivity( sheet, items );

        // UpdateDanglingState() also adds connected items for SCH_TEXT
        sheet.LastScreen()->TestDanglingEnds( &sheet, aChangedItemHandler );

        // Restore the m_unit member, to avoid changes in current active sheet path
        // after calculations
        for( auto& item : symbolsChanged )
        {
            item.first->UpdateUnit( item.second );
        }
    }

    if( wxLog::IsAllowedTraceMask( ConnProfileMask ) )
        update_items.Show();

    PROF_TIMER build_graph( "buildConnectionGraph" );

    buildConnectionGraph( aChangedItemHandler );

    if( wxLog::IsAllowedTraceMask( ConnProfileMask ) )
        build_graph.Show();

    recalc_time.Stop();

    if( wxLog::IsAllowedTraceMask( ConnProfileMask ) )
        recalc_time.Show();
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
            aSubgraph = aSubgraph->m_absorbed_by;

        // Find the top most connected subgraph on all sheets
        while( aSubgraph->m_hier_parent )
            aSubgraph = aSubgraph->m_hier_parent;

        // Recurse through all subsheets to collect connected items
        aSubgraph->getAllConnectedItems( retvals, subgraphs );
    };

    for( SCH_ITEM* item : aItems )
    {
        auto it = m_item_to_subgraph_map.find( item );

        if( it == m_item_to_subgraph_map.end() )
            continue;

        CONNECTION_SUBGRAPH* sg = it->second;

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

        alg::delete_matching( m_items, item );
    }

    removeSubgraphs( subgraphs );

    return retvals;
}


void CONNECTION_GRAPH::removeSubgraphs( std::set<CONNECTION_SUBGRAPH*>& aSubgraphs )
{
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

            if( it != m_driver_subgraphs.end() )
                m_driver_subgraphs.erase( it );
        }

        {
            auto it = std::lower_bound( m_subgraphs.begin(), m_subgraphs.end(), sg );

            if( it != m_subgraphs.end() )
                m_subgraphs.erase( it );
        }

        for( auto& el : m_sheet_to_subgraphs_map )
        {
            auto it = std::lower_bound( el.second.begin(), el.second.end(), sg );

            if( it != el.second.end() )
                el.second.erase( it );
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

        for( auto it = m_net_code_to_subgraphs_map.begin(); it != m_net_code_to_subgraphs_map.end(); )
        {
            if( remove_sg( it ) )
            {
                codes_to_remove.insert( it->first.Netcode );
                it = m_net_code_to_subgraphs_map.erase( it );
            }
            else
                ++it;
        }

        for( auto it = m_net_name_to_subgraphs_map.begin(); it != m_net_name_to_subgraphs_map.end(); )
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
        if( codes_to_remove.find( it->second ) != codes_to_remove.end() )
            it = m_net_name_to_code_map.erase( it );
        else
            ++it;
    }

    for( auto it = m_bus_name_to_code_map.begin(); it != m_bus_name_to_code_map.end(); )
    {
        if( codes_to_remove.find( it->second ) != codes_to_remove.end() )
            it = m_bus_name_to_code_map.erase( it );
        else
            ++it;
    }

    for( CONNECTION_SUBGRAPH* sg : aSubgraphs )
    {
        sg->m_code = -1;
        delete sg;
    }
}


void CONNECTION_GRAPH::updateItemConnectivity( const SCH_SHEET_PATH& aSheet,
                                               const std::vector<SCH_ITEM*>& aItemList )
{
    std::map<VECTOR2I, std::vector<SCH_ITEM*>> connection_map;

    for( SCH_ITEM* item : aItemList )
    {
        std::vector<VECTOR2I> points = item->GetConnectionPoints();
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
                SCH_CONNECTION* conn = pin->InitializeConnection( aSheet, this );

                VECTOR2I pos = pin->GetPosition();

                // because calling the first time is not thread-safe
                wxString name = pin->GetDefaultNetName( aSheet );
                pin->ConnectedItems( aSheet ).clear();

                // power symbol pins need to be post-processed later
                if( pin->IsGlobalPower() )
                {
                    conn->SetName( name );
                    m_global_power_pins.emplace_back( std::make_pair( aSheet, pin ) );
                }

                connection_map[ pos ].push_back( pin );
                m_items.emplace_back( pin );
            }
        }
        else
        {
            m_items.emplace_back( item );
            SCH_CONNECTION* conn = item->InitializeConnection( aSheet, this );

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

            for( const VECTOR2I& point : points )
                connection_map[ point ].push_back( item );
        }

        item->SetConnectivityDirty( false );
    }

    for( const auto& it : connection_map )
    {
        std::vector<SCH_ITEM*> connection_vec = it.second;
        std::sort( connection_vec.begin(), connection_vec.end() );
        connection_vec.erase( std::unique( connection_vec.begin(), connection_vec.end() ),
                connection_vec.end() );

        // Pre-scan to see if we have a bus at this location
        SCH_LINE* busLine = aSheet.LastScreen()->GetBus( it.first );

        std::mutex update_mutex;

        auto update_lambda = [&]( SCH_ITEM* connected_item ) -> size_t
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
                if( connection_vec.size() < 2 )
                {
                    if( busLine )
                    {
                        auto bus_entry = static_cast<SCH_BUS_BUS_ENTRY*>( connected_item );

                        if( it.first == bus_entry->GetPosition() )
                            bus_entry->m_connected_bus_items[0] = busLine;
                        else
                            bus_entry->m_connected_bus_items[1] = busLine;

                        std::lock_guard<std::mutex> lock( update_mutex );
                        bus_entry->AddConnectionTo( aSheet, busLine );
                        busLine->AddConnectionTo( aSheet, bus_entry );
                    }
                }
            }

            // Change junctions to be on bus junction layer if they are touching a bus
            else if( connected_item->Type() == SCH_JUNCTION_T )
            {
                connected_item->SetLayer( busLine ? LAYER_BUS_JUNCTION : LAYER_JUNCTION );
            }

            SCH_ITEM_SET& connected_set = connected_item->ConnectedItems( aSheet );
            connected_set.reserve( connection_vec.size() );

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

                if( connected_item->ConnectionPropagatesTo( test_item ) &&
                    test_item->ConnectionPropagatesTo( connected_item ) &&
                    bus_connection_ok )
                {
                    connected_set.push_back( test_item );
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
                    SCH_LINE*   bus = screen->GetBus( it.first );

                    if( bus )
                        bus_entry->m_connected_bus_item = bus;
                }
            }

            return 1;
        };

        thread_pool& tp = GetKiCadThreadPool();

        tp.push_loop( connection_vec.size(),
                [&]( const int a, const int b)
                {
                    for( int ii = a; ii < b; ++ii )
                        update_lambda( connection_vec[ii] );
                });
        tp.wait_for_tasks();
    }
}


void CONNECTION_GRAPH::buildItemSubGraphs()
{
    // Recache all bus aliases for later use
    wxCHECK_RET( m_schematic, wxS( "Connection graph cannot be built without schematic pointer" ) );

    SCH_SHEET_LIST all_sheets = m_schematic->GetSheets();

    for( unsigned i = 0; i < all_sheets.size(); i++ )
    {
        for( const std::shared_ptr<BUS_ALIAS>& alias : all_sheets[i].LastScreen()->GetBusAliases() )
            m_bus_alias_cache[ alias->GetName() ] = alias;
    }

    // Build subgraphs from items (on a per-sheet basis)

    for( SCH_ITEM* item : m_items )
    {
        for( const auto& it : item->m_connection_map )
        {
            const SCH_SHEET_PATH& sheet = it.first;
            SCH_CONNECTION*       connection = it.second;

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
                            bool unique = !( aItem->GetFlags() & CANDIDATE );

                            if( conn && !conn->SubgraphCode() )
                                aItem->SetFlags( CANDIDATE );

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

                    wxASSERT( connected_conn );

                    if( connected_conn->SubgraphCode() == 0 )
                    {
                        connected_conn->SetSubgraphCode( subgraph->m_code );
                        m_item_to_subgraph_map[connected_item] = subgraph;
                        subgraph->AddItem( connected_item );
                        SCH_ITEM_SET& citemset = connected_item->ConnectedItems( sheet );

                        for( SCH_ITEM* citem : citemset )
                        {
                            if( citem->HasFlag( CANDIDATE ) )
                                continue;

                            if( get_items( citem ) )
                                memberlist.push_back( citem );
                        }
                    }
                }

                for( SCH_ITEM* connected_item : memberlist )
                    connected_item->ClearFlags( CANDIDATE );

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

    std::vector<std::future<size_t>> returns( dirty_graphs.size() );

    auto update_lambda = []( CONNECTION_SUBGRAPH* subgraph ) -> size_t
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

    tp.push_loop( dirty_graphs.size(),
            [&]( const int a, const int b)
            {
                for( int ii = a; ii < b; ++ii )
                    update_lambda( dirty_graphs[ii] );
            });
    tp.wait_for_tasks();

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
                wxASSERT( pin->IsGlobalPower() );
                m_global_label_cache[name].push_back( subgraph );
                break;
            }
            default:
            {
                UNITS_PROVIDER unitsProvider( schIUScale, EDA_UNITS::MILLIMETRES );

                wxLogTrace( ConnTrace, wxS( "Unexpected strong driver %s" ),
                            driver->GetItemDescription( &unitsProvider ) );
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
        SCH_ITEM_SET vec = subgraph->GetAllBusLabels();

        for( SCH_ITEM* item : vec )
        {
            SCH_LABEL_BASE* label = static_cast<SCH_LABEL_BASE*>( item );

            SCH_CONNECTION dummy( item, subgraph->m_sheet );
            dummy.SetGraph( this );
            dummy.ConfigureFromLabel( label->GetText() );

            wxLogTrace( ConnTrace, wxS( "new bus label (%s)" ), label->GetText() );

            for( const auto& conn : dummy.Members() )
            {
                wxString name = conn->FullLocalName();

                CONNECTION_SUBGRAPH* new_sg = new CONNECTION_SUBGRAPH( this );
                SCH_CONNECTION* new_conn = new SCH_CONNECTION( item, subgraph->m_sheet );
                new_conn->SetGraph( this );

                new_conn->SetName( name );
                new_conn->SetType( CONNECTION_TYPE::NET );
                int code = assignNewNetCode( *new_conn );

                wxLogTrace( ConnTrace, wxS( "SG(%ld), Adding full local name (%s) with sg (%d) on subsheet %s" ), subgraph->m_code,
                            name, code, subgraph->m_sheet.PathHumanReadable() );

                new_sg->m_driver_connection = new_conn;
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

    std::copy( new_subgraphs.begin(), new_subgraphs.end(), std::back_inserter( m_driver_subgraphs ) );
}

void CONNECTION_GRAPH::generateGlobalPowerPinSubGraphs()
{
    // Generate subgraphs for global power pins.  These will be merged with other subgraphs
    // on the same sheet in the next loop.
    // These are NOT limited to power symbols, we support legacy invisible + power-in pins
    // on non-power symbols.

    std::unordered_map<int, CONNECTION_SUBGRAPH*> global_power_pin_subgraphs;

    for( const auto& it : m_global_power_pins )
    {
        SCH_SHEET_PATH sheet = it.first;
        SCH_PIN*       pin   = it.second;

        if( !pin->ConnectedItems( sheet ).empty() && !pin->GetLibPin()->GetParent()->IsPower() )
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
        if( pin->GetLibPin()->GetParent()->IsPower() )
            connection->SetName( pin->GetParentSymbol()->GetValueFieldText( true, &sheet, false ) );
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

        auto create_new_name =
                [&suffix]( SCH_CONNECTION* aConn ) -> wxString
                {
                    wxString newName;
                    wxString suffixStr = std::to_wstring( suffix );

                    // For group buses with a prefix, we can add the suffix to the prefix.
                    // If they don't have a prefix, we force the creation of a prefix so that
                    // two buses don't get inadvertently shorted together.
                    if( aConn->Type() == CONNECTION_TYPE::BUS_GROUP )
                    {
                        wxString prefix = aConn->BusPrefix();

                        if( prefix.empty() )
                            prefix = wxT( "BUS" ); // So result will be "BUS_1{...}"

                        wxString oldName = aConn->Name().AfterFirst( '{' );

                        newName << prefix << wxT( "_" ) << suffixStr << wxT( "{" ) << oldName;

                        aConn->ConfigureFromLabel( newName );
                    }
                    else
                    {
                        newName << aConn->Name() << wxT( "_" ) << suffixStr;
                        aConn->SetSuffix( wxString( wxT( "_" ) ) << suffixStr );
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

                wxLogTrace( ConnTrace, wxS( "%ld (%s) is weakly driven and not unique. Changing to %s." ),
                            subgraph->m_code, name, new_name );

                alg::delete_matching( *vec, subgraph );

                m_net_name_to_subgraphs_map[new_name].emplace_back( subgraph );

                name = new_name;
            }
            else if( subgraph->m_driver )
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
                                    wxS( "%ld (%s) skipped for promotion due to potential conflict" ),
                                    subgraph->m_code, name );
                    }
                    else
                    {
                        UNITS_PROVIDER unitsProvider( schIUScale, EDA_UNITS::MILLIMETRES );

                        wxLogTrace( ConnTrace,
                                    wxS( "%ld (%s) weakly driven by unique sheet pin %s, promoting" ),
                                    subgraph->m_code, name,
                                    subgraph->m_driver->GetItemDescription( &unitsProvider ) );

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
                            wxLogTrace( ConnTrace,
                                        wxS( "%lu (%s): Adding secondary driver %s" ), aSubgraph->m_code,
                                        aSubgraph->m_driver_connection->Name( true ),
                                        c->Name( true ) );
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

                            if( pin->IsGlobalPower()
                                && pin->GetDefaultNetName( sheet ) == test_name )
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
                         wxLogTrace( ConnTrace, wxS( "%lu (%s) has bus child %lu (%s)" ), subgraph->m_code,
                                     connection->Name(), candidate->m_code, member->Name() );

                        subgraph->m_bus_neighbors[member].insert( candidate );
                        candidate->m_bus_parents[member].insert( subgraph );
                    }
                    else
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

        wxLogTrace( ConnTrace, wxS( "Re-resolving drivers for %lu (%s)" ), subgraph->m_code,
                    subgraph->m_driver_connection->Name() );
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


void CONNECTION_GRAPH::buildConnectionGraph( std::function<void( SCH_ITEM* )>* aChangedItemHandler )
{
    // Recache all bus aliases for later use
    wxCHECK_RET( m_schematic, wxT( "Connection graph cannot be built without schematic pointer" ) );

    SCH_SHEET_LIST all_sheets = m_schematic->GetSheets();

    for( unsigned i = 0; i < all_sheets.size(); i++ )
    {
        for( const std::shared_ptr<BUS_ALIAS>& alias : all_sheets[i].LastScreen()->GetBusAliases() )
            m_bus_alias_cache[ alias->GetName() ] = alias;
    }

    PROF_TIMER sub_graph( "buildItemSubGraphs" );
    buildItemSubGraphs();

    if( wxLog::IsAllowedTraceMask( ConnProfileMask ) )
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

    if( wxLog::IsAllowedTraceMask( ConnProfileMask ) )
        proc_sub_graph.Show();

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

    thread_pool& tp = GetKiCadThreadPool();

    tp.push_loop( m_driver_subgraphs.size(),
            [&]( const int a, const int b)
            {
                for( int ii = a; ii < b; ++ii )
                    m_driver_subgraphs[ii]->UpdateItemConnections();
            });
    tp.wait_for_tasks();

    // Next time through the subgraphs, we do some post-processing to handle things like
    // connecting bus members to their neighboring subgraphs, and then propagate connections
    // through the hierarchy

    for( CONNECTION_SUBGRAPH* subgraph : m_driver_subgraphs )
    {
        if( !subgraph->m_dirty )
            continue;

        wxLogTrace( ConnTrace, wxS( "Processing %lu (%s) for propagation" ), subgraph->m_code,
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

                const wxString& secondary_name = subgraph->GetNameForDriver( driver );

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
                        wxLogTrace( ConnTrace, wxS( "Global %lu (%s) promoted to %s" ), candidate->m_code,
                                    conn->Name(), subgraph->m_driver_connection->Name() );

                        conn->Clone( *subgraph->m_driver_connection );

                        candidate->m_dirty = false;
                        propagateToNeighbors( candidate, false );
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
        wxASSERT_MSG( !subgraph->m_dirty, wxS( "Subgraph not processed by propagateToNeighbors!" ) );

        if( subgraph->m_bus_parents.size() < 2 )
            continue;

        SCH_CONNECTION* conn = subgraph->m_driver_connection;

        wxLogTrace( ConnTrace, wxS( "%lu (%s) has multiple bus parents" ),
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
                    wxLogTrace( ConnTrace, wxS( "Warning: could not match %s inside %lu (%s)" ),
                                conn->Name(), parent->m_code, parent->m_driver_connection->Name() );
                    continue;
                }

                if( conn->Name() != match->Name() )
                {
                    wxString old_name = match->Name();

                    wxLogTrace( ConnTrace, wxS( "Updating %lu (%s) member %s to %s" ), parent->m_code,
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


    auto updateItemConnectionsTask =
            [&]( CONNECTION_SUBGRAPH* subgraph ) -> size_t
            {
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
                    return 0;

                // As a visual aid, we can check sheet pins that are driven by themselves to see
                // if they should be promoted to buses

                if( subgraph->m_driver && subgraph->m_driver->Type() == SCH_SHEET_PIN_T )
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
                            return 0;
                    }
                }

                return 1;
            };

    tp.push_loop( m_driver_subgraphs.size(),
            [&]( const int a, const int b)
            {
                for( int ii = a; ii < b; ++ii )
                    updateItemConnectionsTask( m_driver_subgraphs[ii] );
            });
    tp.wait_for_tasks();

    m_net_code_to_subgraphs_map.clear();
    m_net_name_to_subgraphs_map.clear();

    for( CONNECTION_SUBGRAPH* subgraph : m_driver_subgraphs )
    {
        NET_NAME_CODE_CACHE_KEY key = { subgraph->GetNetName(),
                                        subgraph->m_driver_connection->NetCode() };
        m_net_code_to_subgraphs_map[ key ].push_back( subgraph );

        m_net_name_to_subgraphs_map[subgraph->m_driver_connection->Name()].push_back( subgraph );
    }

    std::shared_ptr<NET_SETTINGS>& netSettings = m_schematic->Prj().GetProjectFile().m_NetSettings;
    std::map<wxString, wxString>   oldAssignments = netSettings->m_NetClassLabelAssignments;

    netSettings->m_NetClassLabelAssignments.clear();

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
            [&]( const std::vector<CONNECTION_SUBGRAPH*>& subgraphs )
            {
                const CONNECTION_SUBGRAPH* driverSubgraph = nullptr;
                wxString                   netclass;

                wxCHECK_RET( !subgraphs.empty(), wxT("Invalid empty subgraph" ) );

                for( const CONNECTION_SUBGRAPH* subgraph : subgraphs )
                {
                    for( SCH_ITEM* item : subgraph->m_items )
                    {
                        netclass = subgraph->GetNetclassForDriver( item );

                        if( !netclass.IsEmpty() )
                            break;
                    }

                    if( !netclass.IsEmpty() )
                    {
                        driverSubgraph = subgraph;
                        break;
                    }
                }

                if( netclass.IsEmpty() )
                    return;

                if( !driverSubgraph )
                    driverSubgraph = subgraphs.front();

                const wxString netname = driverSubgraph->GetNetName();

                if( driverSubgraph->m_driver_connection->IsBus() )
                {
                    for( const auto& member : driverSubgraph->m_driver_connection->Members() )
                    {
                        netSettings->m_NetClassLabelAssignments[ member->Name() ] = netclass;

                        auto ii = m_net_name_to_subgraphs_map.find( member->Name() );

                        if( ii != m_net_name_to_subgraphs_map.end() )
                            dirtySubgraphs( ii->second );
                    }
                }

                netSettings->m_NetClassLabelAssignments[ netname ] = netclass;

                if( oldAssignments[ netname ] != netclass )
                    dirtySubgraphs( subgraphs );
            };

    for( const auto& [ netname, subgraphs ] : m_net_name_to_subgraphs_map )
        checkNetclassDrivers( subgraphs );
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
                                wxLogTrace( ConnTrace, wxS( "%lu: found child %lu (%s)" ), aParent->m_code,
                                            candidate->m_code, candidate->m_driver_connection->Name() );

                                candidate->m_hier_parent = aParent;
                                aParent->m_hier_children.insert( candidate );

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
                    wxLogTrace( ConnTrace, wxS( "Could not match bus member %s in %s" ),
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

                wxLogTrace( ConnTrace, wxS( "%lu (%s) connected to bus member %s (local %s)" ),
                            neighbor->m_code, neighbor_name, member->Name(), member->LocalName() );

                // Take whichever name is higher priority
                if( CONNECTION_SUBGRAPH::GetDriverPriority( neighbor->m_driver )
                    >= CONNECTION_SUBGRAPH::PRIORITY::POWER_PIN )
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
        for( SCH_CONNECTION* stale_member : stale_bus_members )
        {
            for( CONNECTION_SUBGRAPH* subgraph : visited )
            {
                SCH_CONNECTION* member = matchBusMember( subgraph->m_driver_connection,
                                                         stale_member );

                if( !member )
                {
                    wxLogTrace( ConnTrace, wxS( "WARNING: failed to match stale member %s in %s." ),
                                stale_member->Name(), subgraph->m_driver_connection->Name() );
                    continue;
                }

                wxLogTrace( ConnTrace, wxS( "Updating %lu (%s) member %s to %s" ), subgraph->m_code,
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
    std::shared_ptr<SCH_CONNECTION> c = std::shared_ptr<SCH_CONNECTION>( nullptr );

    switch( aItem->Type() )
    {
    case SCH_PIN_T:
    {
        SCH_PIN* pin = static_cast<SCH_PIN*>( aItem );

        if( pin->IsGlobalPower() )
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
        wxASSERT( !subgraph->m_dirty );

        if( !subgraph->m_driver )
            continue;

        SCH_CONNECTION* connection = subgraph->m_driver->Connection( &subgraph->m_sheet );

        if( !connection->IsBus() )
            continue;

        auto labels = subgraph->GetVectorBusLabels();

        if( labels.size() > 1 )
        {
            bool different = false;
            wxString first = static_cast<SCH_TEXT*>( labels.at( 0 ) )->GetShownText( false );

            for( unsigned i = 1; i < labels.size(); ++i )
            {
                if( static_cast<SCH_TEXT*>( labels.at( i ) )->GetShownText( false ) != first )
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

    for( auto it = m_net_name_to_subgraphs_map.begin(); it != m_net_name_to_subgraphs_map.end() && !found; ++it )
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


const std::vector<CONNECTION_SUBGRAPH*> CONNECTION_GRAPH::GetAllSubgraphs(
        const wxString& aNetName ) const
{
    std::vector<CONNECTION_SUBGRAPH*> subgraphs;

    auto it = m_net_name_to_subgraphs_map.find( aNetName );

    if( it == m_net_name_to_subgraphs_map.end() )
        return subgraphs;

    return it->second;
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

    if( settings.IsTestEnabled( ERCE_NETCLASS_CONFLICT ) )
    {
        for( const auto& [ netname, subgraphs ] : m_net_name_to_subgraphs_map )
        {
            if( !ercCheckNetclassConflicts( subgraphs ) )
                error_count++;
        }
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

        const wxString& primaryName   = aSubgraph->GetNameForDriver( primary );
        const wxString& secondaryName = aSubgraph->GetNameForDriver( secondary );

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
                    || driver->Type() == SCH_LABEL_T
                    || ( driver->Type() == SCH_PIN_T
                         && static_cast<SCH_PIN*>( driver )->IsGlobalPower() ) )
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


bool CONNECTION_GRAPH::ercCheckNetclassConflicts( const std::vector<CONNECTION_SUBGRAPH*>& subgraphs )
{
    wxString  firstNetclass;
    SCH_ITEM* firstNetclassDriver = nullptr;

    for( const CONNECTION_SUBGRAPH* subgraph : subgraphs )
    {
        for( SCH_ITEM* item : subgraph->m_items )
        {
            const wxString netclass = subgraph->GetNetclassForDriver( item );

            if( netclass.IsEmpty() )
                continue;

            if( netclass != firstNetclass )
            {
                if( !firstNetclassDriver )
                {
                    firstNetclass = netclass;
                    firstNetclassDriver = item;
                    continue;
                }

                std::shared_ptr<ERC_ITEM> ercItem = ERC_ITEM::Create( ERCE_NETCLASS_CONFLICT );
                ercItem->SetItems( firstNetclassDriver, item );

                SCH_MARKER* marker = new SCH_MARKER( ercItem, item->GetPosition() );
                subgraph->m_sheet.LastScreen()->Append( marker );

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
            conn.ConfigureFromLabel( EscapeString( text->GetShownText( false ), CTX_NETNAME ) );

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

            std::set<wxString> test_names;
            test_names.insert( bus_entry->Connection( &sheet )->Name() );

            wxString baseName = sheet.PathHumanReadable();

            for( SCH_ITEM* driver : aSubgraph->m_drivers )
                test_names.insert( baseName + aSubgraph->GetNameForDriver( driver ) );

            for( const auto& member : bus_wire->Connection( &sheet )->Members() )
            {
                if( member->Type() == CONNECTION_TYPE::BUS )
                {
                    for( const auto& sub_member : member->Members() )
                    {
                        if( test_names.count( sub_member->Name() ) )
                            conflict = false;
                    }
                }
                else if( test_names.count( member->Name() ) )
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
        if( unique_pins.size() > 1 && settings.IsTestEnabled( ERCE_NOCONNECT_CONNECTED ) )
        {
            std::shared_ptr<ERC_ITEM> ercItem = ERC_ITEM::Create( ERCE_NOCONNECT_CONNECTED );
            VECTOR2I pos;

            if( pin )
            {
                ercItem->SetItems( pin, aSubgraph->m_no_connect );
                pos = pin->GetTransformedPosition();
            }
            else
            {
                ercItem->SetItems( aSubgraph->m_no_connect );
                pos = aSubgraph->m_no_connect->GetPosition();
            }

            SCH_MARKER* marker = new SCH_MARKER( ercItem, pos );
            screen->Append( marker );

            ok = false;
        }

        if( unique_pins.empty() && unique_labels.empty() && settings.IsTestEnabled( ERCE_NOCONNECT_NOT_CONNECTED ) )
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
                // Stacked pins do not count as other connections but non-stacked pins do
                if( !has_other_connections && !pins.empty() )
                {
                    SCH_PIN* test_pin = static_cast<SCH_PIN*>( item );

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
            if( test_pin->GetType() == ELECTRICAL_PINTYPE::PT_POWER_IN )
            {
                pin = test_pin;
                break;
            }
        }

        // Check if power input pins connect to anything else via net name,
        // but not for power symbols (with visible or legacy invisible pins).
        // We want to throw unconnected errors for power symbols even if they are connected to other
        // net items by name, because usually failing to connect them graphically is a mistake
        if( pin && !has_other_connections
                && pin->IsGlobalPower()
                && !pin->GetLibPin()->GetParent()->IsPower() )
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
                // We only apply this test to power symbols, because other symbols have
                // pins that are meant to be dangling, but the power symbols have pins
                // that are *not* meant to be dangling.
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

    ERC_SETTINGS& settings = m_schematic->ErcSettings();
    bool          ok       = true;
    int           pinCount = 0;
    bool          has_nc   = !!aSubgraph->m_no_connect;

    std::map<KICAD_T, std::vector<SCH_TEXT*>> label_map;


    auto hasPins =
            []( const CONNECTION_SUBGRAPH* aLocSubgraph ) -> int
            {
                return
                std::count_if( aLocSubgraph->m_items.begin(), aLocSubgraph->m_items.end(), []( const SCH_ITEM* item )
                        {   return item->Type() == SCH_PIN_T; } );
            };

    auto reportError = [&]( SCH_TEXT* aText, int errCode )
    {
        if( settings.IsTestEnabled( errCode ) )
        {
            std::shared_ptr<ERC_ITEM> ercItem = ERC_ITEM::Create( errCode );
            ercItem->SetItems( aText );

            SCH_MARKER* marker = new SCH_MARKER( ercItem, aText->GetPosition() );
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

    wxString netName = GetResolvedSubgraphName( aSubgraph );

    wxCHECK_MSG( m_schematic, true, wxS( "Null m_schematic in CONNECTION_GRAPH::ercCheckLabels" ) );

    // Labels that have multiple pins connected are not dangling (may be used for naming segments)
    // so leave them without errors here
    if( pinCount > 1 )
        return true;

    for( auto& [type, label_vec] : label_map )
    {

        switch( type )
        {
        case SCH_GLOBAL_LABEL_T:
            if( !settings.IsTestEnabled( ERCE_GLOBLABEL ) )
                continue;

            break;
        default:
            if( !settings.IsTestEnabled( ERCE_LABEL_NOT_CONNECTED ) )
                continue;

            break;
        }

        for( SCH_TEXT* text : label_vec )
        {
            int allPins = pinCount;

            auto it = m_net_name_to_subgraphs_map.find( netName );

            if( it != m_net_name_to_subgraphs_map.end() )
            {
                for( const CONNECTION_SUBGRAPH* neighbor : it->second )
                {
                    if( neighbor == aSubgraph )
                        continue;

                    if( neighbor->m_no_connect )
                        has_nc = true;

                    allPins += hasPins( neighbor );
                }
            }

            if( allPins == 1 && !has_nc )
            {
                reportError( text,
                        type == SCH_GLOBAL_LABEL_T ? ERCE_GLOBLABEL : ERCE_LABEL_NOT_CONNECTED );
                ok = false;
            }

            if( allPins == 0 )
            {
                reportError( text,
                        type == SCH_GLOBAL_LABEL_T ? ERCE_GLOBLABEL : ERCE_LABEL_NOT_CONNECTED );
                ok = false;
            }
        }
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
                    ercItem->SetSheetSpecificPath( sheet );
                    ercItem->SetItemsSheetPaths( sheet );

                    SCH_MARKER* marker = new SCH_MARKER( ercItem, unmatched.second->GetPosition() );
                    sheet.LastScreen()->Append( marker );

                    errors++;
                }

                for( const std::pair<const wxString, SCH_HIERLABEL*>& unmatched : labels )
                {
                    wxString msg = wxString::Format( _( "Hierarchical label %s has no matching "
                                                        "sheet pin in the parent sheet" ),
                                                     UnescapeString( unmatched.first ) );

                    SCH_SHEET_PATH parentSheetPath = sheet;
                    parentSheetPath.push_back( parentSheet );

                    std::shared_ptr<ERC_ITEM> ercItem = ERC_ITEM::Create( ERCE_HIERACHICAL_LABEL );
                    ercItem->SetItems( unmatched.second );
                    ercItem->SetErrorMessage( msg );
                    ercItem->SetSheetSpecificPath( parentSheetPath );
                    ercItem->SetItemsSheetPaths( parentSheetPath );

                    SCH_MARKER* marker = new SCH_MARKER( ercItem, unmatched.second->GetPosition() );
                    parentSheet->GetScreen()->Append( marker );

                    errors++;
                }
            }
        }
    }

    return errors;
}
