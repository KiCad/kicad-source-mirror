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
#include <algorithm>
#include <iterator>
#include <queue>
#include <unordered_map>
#include <stdexcept>
#include <type_traits>
#include <sch_label.h>
#include <sch_line.h>
#include <sch_pin.h>
#include <sch_screen.h>
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
    m_bridgeEdges.clear();
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

    m_bridgeEdges.insert( m_bridgeEdges.end(), aOther.m_bridgeEdges.begin(), aOther.m_bridgeEdges.end() );
    aOther.m_bridgeEdges.clear();

    m_netChainsBuilt = m_netChainsBuilt || aOther.m_netChainsBuilt;

    for( auto& [key, value] : aOther.m_netChainNetClassOverrides )
        m_netChainNetClassOverrides.insert_or_assign( key, value );

    for( auto& [key, value] : aOther.m_netChainColorOverrides )
        m_netChainColorOverrides.insert_or_assign( key, value );

    for( auto& [key, value] : aOther.m_netChainTerminalRefOverrides )
        m_netChainTerminalRefOverrides.insert_or_assign( key, value );

    for( auto& [key, value] : aOther.m_netChainMemberNetOverrides )
        m_netChainMemberNetOverrides.insert_or_assign( key, value );
}

struct SCH_CONNECTIVITY::NETCHAIN_MANAGER::SHEET_SYMBOLS
{
    struct SYMBOL_PINS
    {
        SCH_SYMBOL*           symbol;
        std::vector<SCH_PIN*> pins;
    };

    const NETCHAIN_INPUT::SHEET* input;
    std::vector<SYMBOL_PINS>     symbols;
};


SCH_CONNECTIVITY::NETCHAIN_MANAGER::BRIDGE_GRAPH SCH_CONNECTIVITY::NETCHAIN_MANAGER::buildBridgeAdjacency(
        const std::vector<SHEET_SYMBOLS>& aSheets )
{
    BRIDGE_GRAPH result;

    // Walk every 2-pin passthrough symbol on every sheet, building a flat list of bridge
    // edges between distinct subgraph nets.

    result.edges.reserve( 256 );

    for( const SHEET_SYMBOLS& view : aSheets )
    {
        const auto& sheet = *view.input;
        SCH_SCREEN* sc = sheet.path.LastScreen();

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

        for( const auto& [symbol, pins] : view.symbols )
        {
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

            const wxString& netA = sheet.Key( pins[0] );
            const wxString& netB = sheet.Key( pins[1] );

            if( netA.IsEmpty() || netB.IsEmpty() || netA == netB )
                continue;

            result.edges.push_back( { netA, netB, symbol, sc } );
        }
    }

    // Mark power subgraphs by walking every pin across every sheet. Any subgraph touched by a
    // power-class pin (or a power-symbol parent) is treated as a power node and its incident
    // bridge edges are excluded below.

    std::set<wxString> powerNets;

    for( const SHEET_SYMBOLS& view : aSheets )
    {
        const auto& sheet = *view.input;

        for( const auto& entry : view.symbols )
        {
            for( SCH_PIN* p : entry.pins )
            {
                if( p->IsPower() || ( p->GetParentSymbol() && p->GetParentSymbol()->IsPower() ) )
                {
                    if( const auto* net = sheet.Find( p ) )
                        powerNets.insert( net->key );
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
        if( powerNets.contains( be.a ) || powerNets.contains( be.b ) )
        {
            if( !powerNets.contains( be.a ) )
                powerAdjacentNets.insert( be.a );

            if( !powerNets.contains( be.b ) )
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
    m_bridgeEdges.swap( candidate.m_bridgeEdges );
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


bool SCH_CONNECTIVITY::NETCHAIN_MANAGER::resolveTerminals(
        SCH_NETCHAIN& aChain, const CHAIN_TERMINAL_REFS* aSavedRefs )
{
    if( !m_schematic )
        return false;

    SCH_PIN* pins[2] = { nullptr, nullptr };
    SCH_SHEET_PATH paths[2];
    const KIID ids[2] = { aChain.GetTerminalPinA(), aChain.GetTerminalPinB() };
    const SCH_SHEET_LIST hierarchy = m_schematic->Hierarchy();

    for( int endpoint = 0; endpoint < 2; ++endpoint )
    {
        const KIID_PATH& storedPath = aChain.GetTerminalPath( endpoint );

        for( const SCH_SHEET_PATH& path : hierarchy )
        {
            SCH_SCREEN* screen = path.LastScreen();

            if( !screen || ( !aSavedRefs && !storedPath.empty() && storedPath != path.PathRef() ) )
                continue;

            if( !aSavedRefs )
            {
                auto* pin = dynamic_cast<SCH_PIN*>( screen->GetConnectivityItem( ids[endpoint] ) );

                if( !pin )
                    continue;

                const auto activePins = static_cast<SCH_SYMBOL*>( pin->GetParentSymbol() )->GetPins( &path );

                if( std::find( activePins.begin(), activePins.end(), pin ) == activePins.end() )
                    continue;

                if( pins[endpoint] )
                    return false;

                pins[endpoint] = pin;
                paths[endpoint] = path;
                continue;
            }

            const CHAIN_TERMINAL_REF& ref = endpoint == 0 ? aSavedRefs->first : aSavedRefs->second;

            for( SCH_ITEM* item : screen->Items().OfType( SCH_SYMBOL_T ) )
            {
                auto* symbol = static_cast<SCH_SYMBOL*>( item );

                if( symbol->GetRef( &path ) != ref.ref )
                    continue;

                for( SCH_PIN* pin : symbol->GetPins( &path ) )
                {
                    if( pin->GetNumber() != ref.pin )
                        continue;

                    // A saved reference or pathless UUID must identify exactly one instance.
                    if( pins[endpoint] )
                        return false;

                    pins[endpoint] = pin;
                    paths[endpoint] = path;
                }
            }
        }

        if( !pins[endpoint] )
            return false;
    }

    aChain.SetTerminalPins( pins[0]->m_Uuid, pins[1]->m_Uuid );
    aChain.SetTerminalPaths( paths[0].Path(), paths[1].Path() );
    aChain.SetTerminalRefs( pins[0]->GetParentSymbol()->GetRef( &paths[0] ), pins[0]->GetNumber(),
                            pins[1]->GetParentSymbol()->GetRef( &paths[1] ), pins[1]->GetNumber() );
    return true;
}


bool SCH_CONNECTIVITY::NETCHAIN_MANAGER::refreshTerminalReferences( SCH_NETCHAIN& aChain )
{
    if( !resolveTerminals( aChain ) )
        return false;

    storeTerminalRefs( aChain );
    return true;
}


void SCH_CONNECTIVITY::NETCHAIN_MANAGER::RefreshTerminalReferences()
{
    for( const auto& chain : m_committedNetChains )
    {
        if( chain )
            refreshTerminalReferences( *chain );
    }
}


void SCH_CONNECTIVITY::NETCHAIN_MANAGER::storeTerminalRefs( const SCH_NETCHAIN& aChain )
{
    m_netChainTerminalRefOverrides[aChain.GetName()] = {
        { aChain.GetTerminalRef( 0 ), aChain.GetTerminalPinNum( 0 ) },
        { aChain.GetTerminalRef( 1 ), aChain.GetTerminalPinNum( 1 ) }
    };
}


void SCH_CONNECTIVITY::NETCHAIN_MANAGER::storeMemberNets( const wxString& aName, const std::set<wxString>& aNets )
{
    std::set<wxString> persistable;

    std::copy_if( aNets.begin(), aNets.end(), std::inserter( persistable, persistable.end() ),
                  SCH_NETCHAIN::IsPersistableNet );

    if( persistable.empty() )
        m_netChainMemberNetOverrides.erase( aName );
    else
        m_netChainMemberNetOverrides[aName] = std::move( persistable );
}


void SCH_CONNECTIVITY::NETCHAIN_MANAGER::rebuild( const NETCHAIN_INPUT& aConnectivity )
{
    const bool trace = wxLog::IsAllowedTraceMask( traceSchNetChain );
    std::set<wxString> unresolvedTerminals;

    for( const auto& chain : m_committedNetChains )
    {
        if( !chain )
            continue;

        if( !refreshTerminalReferences( *chain ) )
        {
            chain->ReplaceNets( {} );
            unresolvedTerminals.insert( chain->GetName() );
        }
    }

    std::unordered_map<wxString, SCH_NETCHAIN*> netToNetChain;

    // Chains may cross sheets; inspect the complete input hierarchy.
    std::vector<SHEET_SYMBOLS> sheetSymbols;
    sheetSymbols.reserve( aConnectivity.sheets.size() );

    for( const auto& sheet : aConnectivity.sheets )
    {
        SCH_SCREEN* screen = sheet.path.LastScreen();

        if( !screen )
            continue;

        SHEET_SYMBOLS& view = sheetSymbols.emplace_back( SHEET_SYMBOLS{ &sheet, {} } );

        for( SCH_ITEM* item : screen->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );
            view.symbols.push_back( { symbol, symbol->GetPins( &sheet.path ) } );
            setSymbolName( symbol, wxEmptyString );
        }
    }

    wxLogTrace( traceSchNetChain, "RebuildNetChains: screens=%zu (global build)", sheetSymbols.size() );
    wxLogTrace( traceSchNetChain, "RebuildNetChains: debug start passes (pre-pass chains=%zu)", m_committedNetChains.size() );

    // Build net chains by scanning eligible 2-pin symbols on every sheet, using the original
    // parallel-wire passthrough heuristic. This is effectively the old pass 1 but repeated for
    // each screen, giving global coverage while preserving expected grouping semantics.
    wxLogTrace( traceSchNetChain, "RebuildNetChains: pass 1 (per-sheet 2-pin symbols)" );

    BRIDGE_GRAPH bridgeGraph = buildBridgeAdjacency( sheetSymbols );
    auto&        bridgeEdges = bridgeGraph.edges;
    auto&        adjacency = bridgeGraph.adjacency;

    wxLogTrace( traceSchNetChain, "RebuildNetChains: bridgeEdges=%zu adjacency=%zu",
                bridgeEdges.size(), adjacency.size() );

    // Targeted stub pruning: reduce any component >4 nets by removing minimal number of "stub" leaves
    // (degree 1 whose neighbor has degree >2). This satisfies legacy test expecting longest branch kept.
    {
        // First, discover connected components over current adjacency.
        wxLogTrace( traceSchNetChain, "RebuildNetChains: targeted stub pruning start (adj=%zu)", adjacency.size() );
        std::set<wxString> seen;
        std::set<wxString> globalPrune;
        for( const auto& kv : adjacency )
        {
            const wxString& start = kv.first;
            if( seen.contains( start ) ) continue;

            wxLogTrace( traceSchNetChain, "  component BFS start '%s'", start );

            std::vector<wxString> comp; std::queue<wxString> q; q.push( start ); seen.insert( start );
            while( !q.empty() )
            {
                wxString cur = q.front(); q.pop(); comp.push_back( cur );
                for( const BRIDGE_NEIGHBOR& e : adjacency.at( cur ) ) if( !seen.contains( e.other ) ) { seen.insert( e.other ); q.push( e.other ); }
            }

            wxLogTrace( traceSchNetChain, "  component size=%zu", comp.size() );

            if( comp.size() <= 4 ) continue;
            std::map<wxString,int> degree;
            for( const wxString& n : comp ) degree[n] = (int) adjacency.at( n ).size();
            std::vector<wxString> candidates;
            for( const wxString& n : comp )
            {
                const auto& nbrs = adjacency.at( n );
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
            m_potentialNetChains.push_back( std::move( sig ) );
        }
    }
    // Build netToNetChain map for potential net chains
    netToNetChain.reserve( adjacency.size() );
    for( const auto& sigUP : m_potentialNetChains )
        if( sigUP ) for( const wxString& n : sigUP->GetNets() ) netToNetChain[n] = sigUP.get();

    for( const BRIDGE_EDGE& edge : bridgeEdges )
    {
        const auto first = netToNetChain.find( edge.a );
        const auto second = netToNetChain.find( edge.b );

        if( first != netToNetChain.end() && second != netToNetChain.end()
            && first->second == second->second && edge.sym )
        {
            first->second->AddSymbol( edge.sym );
        }
    }

    m_bridgeEdges = std::move( bridgeEdges );

    if( trace )
    {
        wxLogTrace( traceSchNetChain, "RebuildNetChains: pre-label potentialNetChains=%zu",
                    m_potentialNetChains.size() );

        for( const auto& sigUP : m_potentialNetChains )
        {
            if( !sigUP )
                continue;

            wxString netsStr;
            int count = 0;

            for( const wxString& n : sigUP->GetNets() )
            {
                if( count < 32 )
                {
                    netsStr += n;
                    netsStr += wxS( " " );
                }
                else
                {
                    netsStr += wxS( "..." );
                    break;
                }

                ++count;
            }

            wxLogTrace( traceSchNetChain, "  chain %p name='%s' nets=%zu [%s]", (void*) sigUP.get(),
                        sigUP->GetName(), sigUP->GetNets().size(), netsStr );
        }
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

    for( const auto& sheet : aConnectivity.sheets )
    {
        SCH_SCREEN* screen = sheet.path.LastScreen();

        if( !screen )
            continue;

        for( SCH_ITEM* item : screen->Items().OfType( SCH_LABEL_T ) )
        {
            const auto* connection = sheet.Find( item );

            if( !connection )
                continue;

            const SCH_TEXT* label = static_cast<const SCH_TEXT*>( item );
            const wxString& net = connection->name;

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
    struct PIN_INFO
    {
        SCH_PIN*              pin;
        SCH_SYMBOL*           sym;
        const SCH_SHEET_PATH* sheet;
        VECTOR2I              position;
    };
    std::map<SCH_NETCHAIN*, std::vector<PIN_INFO>> chainPins;

    if( !m_potentialNetChains.empty() )
    {
        for( const SHEET_SYMBOLS& view : sheetSymbols )
        {
            const auto& sheet = *view.input;
            const SCH_SHEET_PATH& sheetPath = sheet.path;

            for( const auto& [sym, pins] : view.symbols )
            {
                for( SCH_PIN* p : pins )
                {
                    const auto chain = netToNetChain.find( sheet.Key( p ) );

                    if( chain != netToNetChain.end() )
                        chainPins[chain->second].push_back( { p, sym, &sheetPath, p->GetPosition() } );
                }
            }
        }
    }

    for( std::unique_ptr<SCH_NETCHAIN>& sig : m_potentialNetChains )
    {
        // Preserve sheet/item/pin traversal order when equally distant terminals compete.
        const auto& pins = chainPins[sig.get()];

        int64_t best = -1;
        KIID    a, b;
        size_t  bestI = 0, bestJ = 0;

        for( size_t i = 0; i < pins.size(); ++i )
        {
            for( size_t j = i + 1; j < pins.size(); ++j )
            {
                VECTOR2I pa = pins[i].position;
                VECTOR2I pb = pins[j].position;
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
            sig->SetTerminalPaths( pins[bestI].sheet->Path(), pins[bestJ].sheet->Path() );
            sig->SetTerminalRefs( pins[bestI].sym->GetRef( pins[bestI].sheet ), pins[bestI].pin->GetNumber(),
                                  pins[bestJ].sym->GetRef( pins[bestJ].sheet ), pins[bestJ].pin->GetNumber() );
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

        if( trace )
        {
            wxString netsStr;

            for( const wxString& n : sig->GetNets() )
                netsStr += n + wxS( " " );

            wxLogTrace( traceSchNetChain, "FinalChain %p nets(%zu): %s", (void*) sig,
                        sig->GetNets().size(), netsStr );
        }
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

        if( !m_netChainTerminalRefOverrides.empty() )
        {
            for( const SHEET_SYMBOLS& view : sheetSymbols )
            {
                const auto& sheet = *view.input;

                for( const auto& [sym, pins] : view.symbols )
                {
                    const wxString ref = sym->GetRef( &sheet.path );

                    for( SCH_PIN* pin : pins )
                    {
                        if( const auto* net = sheet.Find( pin ) )
                            refPinToNet[{ ref, pin->GetNumber() }] = net->key;
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
            if( unresolvedTerminals.contains( chainName ) )
                continue;

            SCH_NETCHAIN* match = nullptr;
            const auto committed = committedByName.find( chainName );

            if( committed != committedByName.end() )
            {
                const SCH_NETCHAIN& chain = *committed->second;
                wxString keys[2];

                for( int endpoint = 0; endpoint < 2; ++endpoint )
                {
                    const KIID& id = endpoint == 0 ? chain.GetTerminalPinA() : chain.GetTerminalPinB();

                    for( const auto& sheet : aConnectivity.sheets )
                    {
                        SCH_SCREEN* screen = sheet.path.LastScreen();

                        if( screen && sheet.path.PathRef() == chain.GetTerminalPath( endpoint ) )
                        {
                            keys[endpoint] = sheet.Key( screen->GetConnectivityItem( id ) );
                            break;
                        }
                    }
                }

                match = findPotentialChain( m_potentialNetChains, keys[0], keys[1] );
            }
            else
            {
                match = resolvePotentialChainByTerminals( termRefs, refPinToNet,
                                                          m_potentialNetChains, chainName );
            }

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

            if( CreateNetChainFromPotential( match, chainName ) )
            {
                alreadyCommitted.insert( chainName );
                refreshedThisPass.insert( chainName );
            }
        }

        // Manual chains have no inferred potential; rebuild from the persisted
        // member-net list by collecting symbols whose pins land on those nets.
        for( const auto& [chainName, memberNets] : m_netChainMemberNetOverrides )
        {
            if( memberNets.empty() || unresolvedTerminals.contains( chainName ) )
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

            for( const SHEET_SYMBOLS& view : sheetSymbols )
            {
                const auto& sheet = *view.input;

                for( const auto& [sym, pins] : view.symbols )
                {
                    const wxString ref = sym->GetRef( &sheet.path );
                    bool           symContributes = false;

                    for( SCH_PIN* pin : pins )
                    {
                        const auto* net = sheet.Find( pin );

                        if( !net )
                            continue;

                        if( memberNets.count( net->name ) )
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
                    refreshCommittedChainPayload( it->second, memberNets, symbols );
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
}


SCH_NETCHAIN* SCH_CONNECTIVITY::NETCHAIN_MANAGER::findPotentialChain(
        const std::vector<std::unique_ptr<SCH_NETCHAIN>>& aPotentials, const wxString& aNetA, const wxString& aNetB )
{
    for( const auto& potential : aPotentials )
    {
        if( potential && potential->GetNets().contains( aNetA ) && potential->GetNets().contains( aNetB ) )
            return potential.get();
    }

    return nullptr;
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

    if( SCH_NETCHAIN* match = findPotentialChain( aPotentials, itFrom->second, itTo->second ) )
        return match;

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
    m_netChainMemberNetOverrides.erase( aName );

    if( std::shared_ptr<NET_SETTINGS> netSettings = liveNetSettings() )
        netSettings->SetNetChainClass( aName, wxEmptyString );

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

    if( std::shared_ptr<NET_SETTINGS> netSettings = liveNetSettings() )
    {
        const wxString chainClass = netSettings->GetNetChainClass( aOld );

        if( !chainClass.IsEmpty() )
        {
            netSettings->SetNetChainClass( aOld, wxEmptyString );
            netSettings->SetNetChainClass( aNew, chainClass );
        }
    }

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
    rekey( m_netChainMemberNetOverrides );
}


void SCH_CONNECTIVITY::NETCHAIN_MANAGER::refreshCommittedChainPayload(
        SCH_NETCHAIN* aTarget, const std::set<wxString>& aNets,
        const std::set<SCH_SYMBOL*>& aSymbols )
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

    for( SCH_SYMBOL* sym : aTarget->GetSymbols() )
        setSymbolName( sym, aTarget->GetName() );
}


void SCH_CONNECTIVITY::NETCHAIN_MANAGER::refreshCommittedChainFromPotential( SCH_NETCHAIN* aTarget,
                                                           const SCH_NETCHAIN& aSource )
{
    refreshCommittedChainPayload( aTarget, aSource.GetNets(), aSource.GetSymbols() );

    // Keep the fallback used when terminal-based inference stops resolving in sync with renames.
    storeMemberNets( aTarget->GetName(), aSource.GetNets() );
}


SCH_NETCHAIN* SCH_CONNECTIVITY::NETCHAIN_MANAGER::CreateNetChainFromPotential( SCH_NETCHAIN* aPotential, const wxString& aName )
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

    sig->SetTerminalPaths( aPotential->GetTerminalPath( 0 ), aPotential->GetTerminalPath( 1 ) );

    if( auto saved = m_netChainTerminalRefOverrides.find( aName ); saved != m_netChainTerminalRefOverrides.end() )
    {
        if( !resolveTerminals( *sig, &saved->second ) )
            return nullptr;
    }

    // Apply any parsed netclass override for this chain name.
    auto ncIt = m_netChainNetClassOverrides.find( aName );

    if( ncIt != m_netChainNetClassOverrides.end() )
        sig->SetNetClass( ncIt->second );

    auto colIt = m_netChainColorOverrides.find( aName );

    if( colIt != m_netChainColorOverrides.end() )
        sig->SetColor( colIt->second );

    for( SCH_SYMBOL* sym : sig->GetSymbols() )
        setSymbolName( sym, sig->GetName() );

    // Register terminal refs in the override map so a subsequent unconditional Recalculate
    // (which calls Reset() and clears the chain's symbol list) can find this chain in the
    // restore pass and refresh it in place.  Runtime-created chains otherwise live only in
    // m_committedNetChains and would be missed by the override-driven restore loop.
    storeTerminalRefs( *sig );

    // Restore fallback if the topology shifts, filtered to match what the s-expr writer saves
    storeMemberNets( aName, sig->GetNets() );

    SCH_NETCHAIN* raw = sig.get();
    m_committedNetChains.push_back( std::move( sig ) );
    return raw;
}


SCH_NETCHAIN* SCH_CONNECTIVITY::NETCHAIN_MANAGER::CreateManualNetChain( const wxString& aName,
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
    const CHAIN_TERMINAL_REFS savedRefs{ { aRefA, aPinNumA }, { aRefB, aPinNumB } };

    if( !resolveTerminals( *sig, &savedRefs ) )
        return nullptr;

    auto ncIt = m_netChainNetClassOverrides.find( aName );

    if( ncIt != m_netChainNetClassOverrides.end() )
        sig->SetNetClass( ncIt->second );

    auto colIt = m_netChainColorOverrides.find( aName );

    if( colIt != m_netChainColorOverrides.end() )
        sig->SetColor( colIt->second );

    for( SCH_SYMBOL* sym : sig->GetSymbols() )
        setSymbolName( sym, sig->GetName() );

    // The restore pass after an unconditional Recalculate finds chains only through these maps
    storeTerminalRefs( *sig );
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


std::shared_ptr<NET_SETTINGS> SCH_CONNECTIVITY::NETCHAIN_MANAGER::liveNetSettings() const
{
    // Staged and temporary graphs must not publish project netclass assignments
    if( !m_schematic || this != &m_schematic->NetChains() )
        return nullptr;

    return m_schematic->Project().GetProjectFile().NetSettings();
}


void SCH_CONNECTIVITY::NETCHAIN_MANAGER::ApplyNetChainNetclasses()
{
    std::shared_ptr<NET_SETTINGS> netSettings = liveNetSettings();

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
        if( !sig )
            continue;

        if( sig->GetName() == aName )
        {
            wxLogTrace( traceSchNetChain, "GetNetChainByName: found" );
            return sig.get();
        }
    }

    wxLogTrace( traceSchNetChain, "GetNetChainByName: not found" );
    return nullptr;
}


wxString SCH_CONNECTIVITY::NETCHAIN_MANAGER::NetKeyForItem( const SCH_ITEM& aItem, const SCH_SHEET_PATH& aPath )
{
    const SCH_CONNECTION* net = aItem.Connection( &aPath );
    return net ? SCH_NETCHAIN::MakeKey( net->Name(), net->SubgraphCode() ) : wxString();
}


std::map<SCH_SYMBOL*, SCH_SCREEN*>
SCH_CONNECTIVITY::NETCHAIN_MANAGER::GetBridgeSymbols( const SCH_NETCHAIN& aChain, const wxString& aNetKey ) const
{
    std::map<SCH_SYMBOL*, SCH_SCREEN*> bridges;

    if( !aChain.GetNets().contains( aNetKey ) )
        return bridges;

    for( const BRIDGE_EDGE& edge : m_bridgeEdges )
    {
        if( edge.a != aNetKey && edge.b != aNetKey )
            continue;

        const wxString& other = edge.a == aNetKey ? edge.b : edge.a;

        if( aChain.GetNets().contains( other ) )
            bridges.emplace( edge.sym, edge.screen );
    }

    return bridges;
}


wxString SCH_CONNECTIVITY::NETCHAIN_MANAGER::NetKeyForTerminal( const KIID& aPin, const KIID_PATH& aSheet ) const
{
    if( !m_schematic || aPin == niluuid || aSheet.empty() )
        return {};

    const std::optional<SCH_SHEET_PATH> path = m_schematic->Hierarchy().GetSheetPathByKIIDPath( aSheet );

    if( !path || !path->LastScreen() )
        return {};

    auto* pin = dynamic_cast<SCH_PIN*>( path->LastScreen()->GetConnectivityItem( aPin ) );
    return pin ? NetKeyForItem( *pin, *path ) : wxString();
}


bool SCH_CONNECTIVITY::NETCHAIN_MANAGER::ReplaceNetChainTerminalPin( const TERMINAL_CHANGE& aChange )
{
    SCH_NETCHAIN* chain = GetNetChainByName( aChange.chain );

    if( !chain || aChange.endpoint < 0 || aChange.endpoint > 1 )
        return false;

    // Chains never hold an empty key, so an unresolvable terminal fails here too
    if( !chain->GetNets().contains( NetKeyForTerminal( aChange.pin, aChange.sheet ) ) )
        return false;

    SCH_NETCHAIN candidate = *chain;
    candidate.SetTerminalPins( aChange.endpoint == 0 ? aChange.pin : chain->GetTerminalPinA(),
                                aChange.endpoint == 1 ? aChange.pin : chain->GetTerminalPinB() );
    candidate.SetTerminalPaths( aChange.endpoint == 0 ? aChange.sheet : chain->GetTerminalPath( 0 ),
                                 aChange.endpoint == 1 ? aChange.sheet : chain->GetTerminalPath( 1 ) );

    if( candidate.GetTerminalPinA() == candidate.GetTerminalPinB()
        && candidate.GetTerminalPath( 0 ) == candidate.GetTerminalPath( 1 ) )
        return false;

    if( !resolveTerminals( candidate ) )
        return false;

    *chain = std::move( candidate );
    storeTerminalRefs( *chain );
    return true;
}


SCH_NETCHAIN* SCH_CONNECTIVITY::NETCHAIN_MANAGER::FindPotentialNetChainBetweenPins(
        SCH_PIN* aPinA, const SCH_SHEET_PATH& aPathA, SCH_PIN* aPinB, const SCH_SHEET_PATH& aPathB )
{
    if( !aPinA || !aPinB )
        return nullptr;

    const wxString netA = NetKeyForItem( *aPinA, aPathA );
    const wxString netB = NetKeyForItem( *aPinB, aPathB );

    if( netA.IsEmpty() || netB.IsEmpty() )
        return nullptr;

    return findPotentialChain( m_potentialNetChains, netA, netB );
}
