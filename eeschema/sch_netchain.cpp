/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2026 KiCad Developers
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

#include <sch_netchain.h>

#include <algorithm>
#include <map>
#include <queue>
#include <set>
#include <vector>

#include <connection_graph.h>
#include <sch_pin.h>
#include <sch_symbol.h>
#include <schematic.h>


namespace
{

// Resolve a terminal-pin KIID to the SCH_PIN it names.  Terminals may live on
// pins that are not bridge symbols (RebuildNetChains picks them by physical
// distance across every chain-net pin), so a chain-local symbol scan is not
// sufficient — fall through to the schematic-wide resolver.
SCH_PIN* findPinByUuid( CONNECTION_GRAPH* aGraph, const KIID& aUuid )
{
    if( !aGraph || aUuid == niluuid )
        return nullptr;

    SCHEMATIC* sch = aGraph->GetSchematic();

    if( !sch )
        return nullptr;

    SCH_ITEM* item = sch->ResolveItem( aUuid, /*aPathOut=*/nullptr,
                                       /*aAllowNullptrReturn=*/true );

    return dynamic_cast<SCH_PIN*>( item );
}


wxString netKeyForPin( CONNECTION_GRAPH* aGraph, SCH_PIN* aPin )
{
    if( !aGraph || !aPin )
        return wxEmptyString;

    return CONNECTION_GRAPH::MakeNetChainKey( aGraph->GetSubgraphForItem( aPin ) );
}

}  // namespace


wxString SCH_NETCHAIN::GetTerminalNetName( int aIdx, CONNECTION_GRAPH* aGraph ) const
{
    wxASSERT( aIdx >= 0 && aIdx < 2 );

    if( !aGraph )
        return wxEmptyString;

    SCH_PIN* pin = findPinByUuid( aGraph, m_terminalPins[aIdx] );

    return netKeyForPin( aGraph, pin );
}


const std::vector<wxString>& SCH_NETCHAIN::GetOrderedNets( CONNECTION_GRAPH* aGraph ) const
{
    if( !m_orderedNetsDirty )
        return m_orderedNets;

    m_orderedNets.clear();

    if( !aGraph || m_nets.size() < 2 )
        return m_orderedNets;

    SCH_PIN* pinA = findPinByUuid( aGraph, m_terminalPins[0] );
    SCH_PIN* pinB = findPinByUuid( aGraph, m_terminalPins[1] );

    if( !pinA || !pinB )
        return m_orderedNets;

    wxString startNet = netKeyForPin( aGraph, pinA );
    wxString endNet = netKeyForPin( aGraph, pinB );

    if( startNet.IsEmpty() || endNet.IsEmpty() )
        return m_orderedNets;

    if( !m_nets.count( startNet ) || !m_nets.count( endNet ) )
        return m_orderedNets;

    // Build a chain-local adjacency map: for every passthrough symbol, connect the
    // nets on its two pins.  We rely on the chain's m_symbols rather than rebuilding
    // the schematic-wide bridge graph because the chain already names its symbols.
    std::map<wxString, std::set<wxString>> adjacency;

    for( SCH_SYMBOL* sym : m_symbols )
    {
        if( !sym )
            continue;

        std::vector<SCH_PIN*> pins = sym->GetPins();
        wxString last;

        for( SCH_PIN* p : pins )
        {
            wxString net = netKeyForPin( aGraph, p );

            if( net.IsEmpty() || !m_nets.count( net ) )
                continue;

            if( !last.IsEmpty() && last != net )
            {
                adjacency[last].insert( net );
                adjacency[net].insert( last );
            }

            last = net;
        }
    }

    // BFS shortest path from startNet to endNet, recording predecessors for reconstruction.
    std::map<wxString, wxString> predecessor;
    std::set<wxString>           visited;
    std::queue<wxString>         frontier;

    frontier.push( startNet );
    visited.insert( startNet );

    bool foundEnd = false;

    while( !frontier.empty() && !foundEnd )
    {
        wxString cur = frontier.front();
        frontier.pop();

        if( cur == endNet )
        {
            foundEnd = true;
            break;
        }

        auto it = adjacency.find( cur );

        if( it == adjacency.end() )
            continue;

        for( const wxString& nbr : it->second )
        {
            if( visited.insert( nbr ).second )
            {
                predecessor[nbr] = cur;
                frontier.push( nbr );
            }
        }
    }

    std::vector<wxString> path;

    if( foundEnd )
    {
        // Reconstruct in reverse, then flip.
        for( wxString cur = endNet; cur != startNet; cur = predecessor[cur] )
            path.push_back( cur );

        path.push_back( startNet );
        std::reverse( path.begin(), path.end() );
    }
    else
    {
        // Defensive fallback: chain is internally disconnected.  Place the two
        // terminals at the bookends and the remaining ordering happens below.
        path.push_back( startNet );

        if( startNet != endNet )
            path.push_back( endNet );
    }

    // Append any off-path members alphabetically (m_nets is already a sorted set).
    std::set<wxString> onPath( path.begin(), path.end() );

    m_orderedNets = std::move( path );

    for( const wxString& net : m_nets )
    {
        if( !onPath.count( net ) )
            m_orderedNets.push_back( net );
    }

    m_orderedNetsDirty = false;
    return m_orderedNets;
}
