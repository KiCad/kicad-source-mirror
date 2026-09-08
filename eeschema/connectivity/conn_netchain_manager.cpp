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

#include "conn_netchain_manager.h"
#include "conn_netchain_input.h"
#include <queue>
#include <stdexcept>
#include <type_traits>
#include <unordered_map>
#include <sch_label.h>
#include <sch_line.h>
#include <sch_pin.h>
#include <sch_screen.h>
#include <algorithm>
#include <sch_symbol.h>
#include <schematic.h>
#include <project.h>
#include <project/project_file.h>
#include <project/net_settings.h>
#include <trace_helpers.h>
#include <wx/log.h>

void SCH_CONNECTIVITY::NETCHAIN_MANAGER::ClearDerived()
{
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

void SCH_CONNECTIVITY::NETCHAIN_MANAGER::Merge( NETCHAIN_MANAGER& aOther )
{
    if( this == &aOther )
        return;

    // Committed chains and override maps belong to the persistent schematic state, so they
    // must travel across an incremental graph merge.  Potential chains are moved alongside
    // them to keep the merged graph self-consistent until the next RebuildNetChains() pass.
    for( std::unique_ptr<SCH_NETCHAIN>& chain : aOther.m_committedNetChains )
    {
        if( chain )
            m_committedNetChains.push_back( std::move( chain ) );
    }

    aOther.m_committedNetChains.clear();

    for( std::unique_ptr<SCH_NETCHAIN>& chain : aOther.m_potentialNetChains )
    {
        if( chain )
            m_potentialNetChains.push_back( std::move( chain ) );
    }

    aOther.m_potentialNetChains.clear();

    m_netChainsBuilt = m_netChainsBuilt || aOther.m_netChainsBuilt;

    for( auto& [key, value] : aOther.m_netChainTerminalOverrides )
        m_netChainTerminalOverrides.insert_or_assign( key, value );

    for( auto& [key, value] : aOther.m_netChainNetClassOverrides )
        m_netChainNetClassOverrides.insert_or_assign( key, value );

    for( auto& [key, value] : aOther.m_netChainColorOverrides )
        m_netChainColorOverrides.insert_or_assign( key, value );

    for( auto& [key, value] : aOther.m_netChainTerminalRefOverrides )
        m_netChainTerminalRefOverrides.insert_or_assign( key, value );

    for( auto& [key, value] : aOther.m_netChainMemberNetOverrides )
        m_netChainMemberNetOverrides.insert_or_assign( key, value );

}

SCH_CONNECTIVITY::NETCHAIN_MANAGER::BRIDGE_GRAPH SCH_CONNECTIVITY::NETCHAIN_MANAGER::buildBridgeAdjacency( const NETCHAIN_INPUT& aConnectivity )
{
    BRIDGE_GRAPH result;

    auto getSubgraphNet = [&]( SCH_PIN* aPin ) -> wxString
    {
        if( !aPin )
            return wxString();

        const auto* sg = aConnectivity.Find( aPin );

        return sg ? sg->key : wxString();
    };

    // Walk every 2-pin passthrough symbol on every sheet, building a flat list of bridge
    // edges between distinct subgraph nets.

    result.edges.reserve( 256 );

    for( const SCH_SHEET_PATH& sheetPath : aConnectivity.sheets )
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

    for( const SCH_SHEET_PATH& sheetPath : aConnectivity.sheets )
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
                if( const auto* sg = aConnectivity.Find( p ) )
                {
                    netToCode[sg->key] = sg->component;

                    if( p->IsPower()
                        || ( p->GetParentSymbol() && p->GetParentSymbol()->IsPower() ) )
                    {
                        powerSubgraphs.insert( sg->component );
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

void SCH_CONNECTIVITY::NETCHAIN_MANAGER::Rebuild(
        const NETCHAIN_INPUT& aConnectivity,
        const std::function<void( NETCHAIN_MANAGER& )>& aBeforePublish )
{
    if( m_pendingSymbolNames )
        throw std::logic_error( "Cannot publish a nested netchain candidate" );

    if( !m_schematic )
        return;

    std::unordered_map<SCH_SYMBOL*, wxString> symbolNames;
    NETCHAIN_MANAGER candidate( m_schematic );
    candidate.m_pendingSymbolNames = &symbolNames;
    candidate.m_netChainTerminalOverrides = m_netChainTerminalOverrides;
    candidate.m_netChainTerminalRefOverrides = m_netChainTerminalRefOverrides;
    candidate.m_netChainNetClassOverrides = m_netChainNetClassOverrides;
    candidate.m_netChainColorOverrides = m_netChainColorOverrides;
    candidate.m_netChainMemberNetOverrides = m_netChainMemberNetOverrides;
    candidate.m_committedNetChains.reserve( m_committedNetChains.size() );

    for( const auto& chain : m_committedNetChains )
    {
        candidate.m_committedNetChains.push_back( chain ? std::make_unique<SCH_NETCHAIN>( *chain ) : nullptr );

        // Source items may have been removed since the last successful refresh.
        if( candidate.m_committedNetChains.back() )
            candidate.m_committedNetChains.back()->ClearSymbols();
    }

    candidate.rebuild( aConnectivity );

    if( aBeforePublish )
        aBeforePublish( candidate );

    const size_t existing = m_committedNetChains.size();

    if( candidate.m_committedNetChains.size() < existing )
        throw std::logic_error( "Committed netchains removed during rebuild" );

    for( size_t i = 0; i < existing; ++i )
    {
        const auto& current = m_committedNetChains[i];
        const auto& updated = candidate.m_committedNetChains[i];

        if( bool( current ) != bool( updated )
            || ( current && current->GetName() != updated->GetName() ) )
        {
            throw std::logic_error( "Committed netchain identity changed during rebuild" );
        }
    }

    m_committedNetChains.reserve( candidate.m_committedNetChains.size() );
    using std::swap;

    static_assert( std::is_nothrow_swappable_v<SCH_NETCHAIN> );
    static_assert( std::is_nothrow_move_constructible_v<wxString> );

    // Keep committed object addresses stable; nothing below may allocate or invoke a callback.
    for( size_t i = 0; i < existing; ++i )
    {
        if( m_committedNetChains[i] )
            swap( *m_committedNetChains[i], *candidate.m_committedNetChains[i] );
    }

    for( size_t i = existing; i < candidate.m_committedNetChains.size(); ++i )
        m_committedNetChains.push_back( std::move( candidate.m_committedNetChains[i] ) );

    m_potentialNetChains.swap( candidate.m_potentialNetChains );
    m_netChainTerminalOverrides.swap( candidate.m_netChainTerminalOverrides );
    m_netChainTerminalRefOverrides.swap( candidate.m_netChainTerminalRefOverrides );
    m_netChainNetClassOverrides.swap( candidate.m_netChainNetClassOverrides );
    m_netChainColorOverrides.swap( candidate.m_netChainColorOverrides );
    m_netChainMemberNetOverrides.swap( candidate.m_netChainMemberNetOverrides );

    for( auto& [symbol, name] : symbolNames )
        symbol->SetNetChainName( std::move( name ) );

    m_netChainsBuilt = true;
}

void SCH_CONNECTIVITY::NETCHAIN_MANAGER::setSymbolName( SCH_SYMBOL* aSymbol, const wxString& aName )
{
    if( !aSymbol )
        return;

    if( m_pendingSymbolNames )
        ( *m_pendingSymbolNames )[aSymbol] = aName;
    else
        aSymbol->SetNetChainName( aName );
}

void SCH_CONNECTIVITY::NETCHAIN_MANAGER::rebuild( const NETCHAIN_INPUT& aConnectivity )
{
        wxLogTrace( traceSchNetChain, "RebuildNetChains: begin (items=%zu, schematic=%p)",
                    aConnectivity.items.size(), (void*) m_schematic );
        // Clear only potential net chains; leave committed net chains intact.
        m_potentialNetChains.clear();

        if( !m_schematic )
        {
            wxLogTrace( traceSchNetChain, "RebuildNetChains: no schematic" );
            return;
        }
    std::map<wxString, SCH_NETCHAIN*> netToNetChain; // will be populated after chain extraction

        // Collect all screens from the cached sheet list so we can operate globally rather than
        // only on the current sheet.  (aConnectivity.sheets is populated during Recalculate()).
        std::vector<SCH_SCREEN*> allScreens;
        allScreens.reserve( aConnectivity.sheets.size() );
        for( const SCH_SHEET_PATH& sp : aConnectivity.sheets )
        {
            if( SCH_SCREEN* sc = sp.LastScreen() )
                allScreens.push_back( sc );
        }

        // Clear any previous chain names on all symbols across all sheets so we can repopulate.
        for( SCH_SCREEN* sc : allScreens )
        {
            for( SCH_ITEM* item : sc->Items().OfType( SCH_SYMBOL_T ) )
                setSymbolName( static_cast<SCH_SYMBOL*>( item ), wxEmptyString );
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

        const auto* sg = aConnectivity.Find( aPin );

        return sg ? sg->key : wxString();
    };

    BRIDGE_GRAPH bridgeGraph = buildBridgeAdjacency( aConnectivity );
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

    for( SCH_ITEM* item : aConnectivity.items )
    {
        if( item->Type() != SCH_LABEL_T )
            continue;

        SCH_TEXT* label = static_cast<SCH_TEXT*>( item );
        wxString  net;

        if( const auto* sg = aConnectivity.Find( item ) )
            net = sg->name;

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
            SCH_NETCHAIN* chain = netToNetChain[net];

            if( !committedNames.contains( name )
                && ( chain->GetName().IsEmpty() || name < chain->GetName() ) )
            {
                chain->SetName( name );
            }
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

        for( const SCH_SHEET_PATH& sheetPath : aConnectivity.sheets )
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
                setSymbolName( sym, sig->GetName() );
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

        for( const SCH_SHEET_PATH& sp : aConnectivity.sheets )
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
                    if( const auto* sg = aConnectivity.Find( pin ) )
                    {
                        // Match potential-chain key construction so unnamed subgraphs use the
                        // synthetic prefix instead of being skipped — without this, a chain
                        // whose only named endpoint is at one terminal would fail strict
                        // both-endpoint matching.
                        refPinToNet[{ ref, pin->GetNumber() }] =
                                sg->key;
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

            for( const SCH_SHEET_PATH& sp : aConnectivity.sheets )
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
                        const auto* sg = aConnectivity.Find( pin );

                        if( !sg )
                            continue;

                        if( memberNets.count( sg->name ) )
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
                    setSymbolName( sym, chain->GetName() );
            }
        }
    }

    // An empty chain list is a valid built state for chainless schematics.
    m_netChainsBuilt = true;
}

SCH_NETCHAIN* SCH_CONNECTIVITY::NETCHAIN_MANAGER::resolvePotentialChainByTerminals(
        const CHAIN_TERMINAL_REFS& aTermRefs, const std::map<std::pair<wxString, wxString>, wxString>& aRefPinToNet,
        const std::vector<std::unique_ptr<SCH_NETCHAIN>>& aPotentials, const wxString& aChainName )
{
    auto itFrom = aRefPinToNet.find( { aTermRefs.first.ref, aTermRefs.first.pin } );
    auto itTo = aRefPinToNet.find( { aTermRefs.second.ref, aTermRefs.second.pin } );

    if( itFrom == aRefPinToNet.end() || itTo == aRefPinToNet.end() )
    {
        wxLogTrace( traceSchNetChain, "RebuildNetChains: cannot restore chain '%s' (terminal %s.%s/%s.%s unresolved)",
                    aChainName, aTermRefs.first.ref, aTermRefs.first.pin, aTermRefs.second.ref, aTermRefs.second.pin );
        return nullptr;
    }

    for( const auto& pot : aPotentials )
    {
        if( pot && pot->GetNets().count( itFrom->second ) && pot->GetNets().count( itTo->second ) )
            return pot.get();
    }

    wxLogTrace( traceSchNetChain, "RebuildNetChains: no potential chain spans both terminals of '%s' (%s/%s)",
                aChainName, itFrom->second, itTo->second );
    return nullptr;
}


bool SCH_CONNECTIVITY::NETCHAIN_MANAGER::DeleteCommittedNetChain( const wxString& aName )
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

    // Otherwise RebuildNetChains() re-promotes these symbols under the deleted name
    for( SCH_SYMBOL* sym : ( *it )->GetSymbols() )
    {
        if( sym )
            setSymbolName( sym, wxEmptyString );
    }

    m_committedNetChains.erase( it );

    m_netChainNetClassOverrides.erase( aName );
    m_netChainColorOverrides.erase( aName );
    m_netChainTerminalRefOverrides.erase( aName );
    m_netChainTerminalOverrides.erase( aName );
    m_netChainMemberNetOverrides.erase( aName );

    return true;
}


bool SCH_CONNECTIVITY::NETCHAIN_MANAGER::RenameCommittedNetChain( const wxString& aOld, const wxString& aNew )
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

    if( findByName( aNew ) )
        return false;

    existing->SetName( aNew );

    for( SCH_SYMBOL* sym : existing->GetSymbols() )
    {
        if( sym )
            setSymbolName( sym, aNew );
    }

    rekeyOverrideMaps( aOld, aNew );

    return true;
}


void SCH_CONNECTIVITY::NETCHAIN_MANAGER::rekeyOverrideMaps( const wxString& aOld, const wxString& aNew )
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


void SCH_CONNECTIVITY::NETCHAIN_MANAGER::refreshCommittedChainPayload(
        SCH_NETCHAIN* aTarget, const std::set<wxString>& aNets, const std::set<SCH_SYMBOL*>& aSymbols,
        const KIID& aTerminalPinA, const KIID& aTerminalPinB, const wxString& aRefA, const wxString& aPinNumA,
        const wxString& aRefB, const wxString& aPinNumB )
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

    // Keep a user-retargeted terminal across an unconditional Recalculate
    auto termOverride = m_netChainTerminalOverrides.find( aTarget->GetName() );

    if( termOverride != m_netChainTerminalOverrides.end() )
        aTarget->SetTerminalPins( termOverride->second.first, termOverride->second.second );
    else
        aTarget->SetTerminalPins( aTerminalPinA, aTerminalPinB );

    aTarget->SetTerminalRefs( aRefA, aPinNumA, aRefB, aPinNumB );

    for( SCH_SYMBOL* sym : aTarget->GetSymbols() )
        setSymbolName( sym, aTarget->GetName() );
}


void SCH_CONNECTIVITY::NETCHAIN_MANAGER::refreshCommittedChainFromPotential( SCH_NETCHAIN*       aTarget,
                                                                             const SCH_NETCHAIN& aSource )
{
    refreshCommittedChainPayload( aTarget, aSource.GetNets(), aSource.GetSymbols(), aSource.GetTerminalPinA(),
                                  aSource.GetTerminalPinB(), aSource.GetTerminalRef( 0 ),
                                  aSource.GetTerminalPinNum( 0 ), aSource.GetTerminalRef( 1 ),
                                  aSource.GetTerminalPinNum( 1 ) );
}


SCH_NETCHAIN* SCH_CONNECTIVITY::NETCHAIN_MANAGER::CreateNetChainFromPotential( SCH_NETCHAIN*   aPotential,
                                                                               const wxString& aName )
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

    auto ncIt = m_netChainNetClassOverrides.find( aName );

    if( ncIt != m_netChainNetClassOverrides.end() )
        sig->SetNetClass( ncIt->second );

    auto colIt = m_netChainColorOverrides.find( aName );

    if( colIt != m_netChainColorOverrides.end() )
        sig->SetColor( colIt->second );

    for( SCH_SYMBOL* sym : sig->GetSymbols() )
        setSymbolName( sym, sig->GetName() );

    // The restore pass after an unconditional Recalculate finds chains only through these maps
    CHAIN_TERMINAL_REFS termRefs{ { aPotential->GetTerminalRef( 0 ), aPotential->GetTerminalPinNum( 0 ) },
                                  { aPotential->GetTerminalRef( 1 ), aPotential->GetTerminalPinNum( 1 ) } };
    m_netChainTerminalRefOverrides[aName] = termRefs;

    // Restore fallback if the topology shifts, filtered to match what the s-expr writer saves
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
    m_committedNetChains.push_back( std::move( sig ) );
    return raw;
}


SCH_NETCHAIN* SCH_CONNECTIVITY::NETCHAIN_MANAGER::CreateManualNetChain(
        const wxString& aName, const std::set<SCH_SYMBOL*>& aSymbols, const std::set<wxString>& aNets,
        const KIID& aTerminalPinA, const KIID& aTerminalPinB, const wxString& aRefA, const wxString& aPinNumA,
        const wxString& aRefB, const wxString& aPinNumB )
{
    if( !SCH_NETCHAIN::IsValidName( aName ) )
        return nullptr;

    if( GetNetChainByName( aName ) )
        return nullptr;

    // GetNetChainForNet returns the first match, so a net may belong to only one chain
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
        setSymbolName( sym, sig->GetName() );

    // The restore pass after an unconditional Recalculate finds chains only through these maps
    CHAIN_TERMINAL_REFS termRefs{ { aRefA, aPinNumA }, { aRefB, aPinNumB } };
    m_netChainTerminalRefOverrides[aName] = termRefs;
    m_netChainMemberNetOverrides[aName] = sig->GetNets();

    SCH_NETCHAIN* raw = sig.get();
    m_committedNetChains.push_back( std::move( sig ) );
    return raw;
}


SCH_NETCHAIN* SCH_CONNECTIVITY::NETCHAIN_MANAGER::GetNetChainForNet( const wxString& aNet )
{
    wxLogTrace( traceSchNetChain, "SCH_CONNECTIVITY::NETCHAIN_MANAGER::GetNetChainForNet(%s)", aNet );
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


void SCH_CONNECTIVITY::NETCHAIN_MANAGER::ApplyNetChainNetclasses()
{
    // Staged and temporary graphs must not publish project netclass assignments
    if( !m_schematic || this != &m_schematic->NetChains() )
        return;

    std::shared_ptr<NET_SETTINGS> netSettings = m_schematic->Project().GetProjectFile().NetSettings();

    if( !netSettings )
        return;

    bool anyOverride = std::any_of( m_committedNetChains.begin(), m_committedNetChains.end(),
                                    []( const std::unique_ptr<SCH_NETCHAIN>& aChain )
                                    {
                                        return aChain && !aChain->GetNetClass().IsEmpty();
                                    } );

    // Leave the effective-netclass cache alone on chainless rebuilds
    if( !anyOverride && !netSettings->HasChainPatternAssignments( NET_CHAIN_SOURCE::SCHEMATIC ) )
        return;

    netSettings->ClearChainPatternAssignments( NET_CHAIN_SOURCE::SCHEMATIC );

    for( const std::unique_ptr<SCH_NETCHAIN>& chain : m_committedNetChains )
    {
        if( !chain )
            continue;

        const wxString& netclass = chain->GetNetClass();

        if( netclass.IsEmpty() || !netSettings->HasNetclass( netclass ) )
            continue;

        for( const wxString& net : chain->GetNets() )
        {
            // Synthetic per-run keys embed a subgraph code and never match a resolved net name
            if( net.StartsWith( SCH_NETCHAIN::SYNTHETIC_NET_PREFIX ) )
                continue;

            netSettings->SetChainPatternAssignment( NET_CHAIN_SOURCE::SCHEMATIC, net, netclass );
        }
    }
}


SCH_NETCHAIN* SCH_CONNECTIVITY::NETCHAIN_MANAGER::GetNetChainByName( const wxString& aName )
{
    wxLogTrace( traceSchNetChain, "SCH_CONNECTIVITY::NETCHAIN_MANAGER::GetNetChainByName(%s)", aName );
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


void SCH_CONNECTIVITY::NETCHAIN_MANAGER::ReplaceNetChainTerminalPin( const wxString& aNetChain, const KIID& aPrev,
                                                                     const KIID& aNew )
{
    wxLogTrace( traceSchNetChain, "ReplaceNetChainTerminalPin: chain='%s' prev=%s new=%s", aNetChain, aPrev.AsString(),
                aNew.AsString() );
    if( SCH_NETCHAIN* sig = GetNetChainByName( aNetChain ) )
    {
        sig->ReplaceTerminalPin( aPrev, aNew );
        m_netChainTerminalOverrides[aNetChain] = std::make_pair( sig->GetTerminalPinA(), sig->GetTerminalPinB() );
        wxLogTrace( traceSchNetChain, "ReplaceNetChainTerminalPin: updated overrides to (%s,%s)",
                    sig->GetTerminalPinA().AsString(), sig->GetTerminalPinB().AsString() );
    }
}


void SCH_CONNECTIVITY::NETCHAIN_MANAGER::SetNetChainTerminalOverrides(
        const std::map<wxString, std::pair<KIID, KIID>>& aOverrides )
{
    m_netChainTerminalOverrides = aOverrides;
    wxLogTrace( traceSchNetChain, "SetNetChainTerminalOverrides: count=%zu", m_netChainTerminalOverrides.size() );
}
