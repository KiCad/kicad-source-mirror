/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <string>
#include <vector>
#include <algorithm>
#include <cassert>
#include <future>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>

#include <thread_pool.h>
#include <cctype>

#include <core/profile.h>
#include <pad.h>
#include <footprint.h>
#include <refdes_utils.h>
#include <board.h>
#include <wx/string.h>
#include <wx/log.h>

#include "topo_match.h"


static const wxString traceTopoMatch = wxT( "TOPO_MATCH" );
static const wxString traceTopoMatchDetail = wxT( "TOPO_MATCH_DETAIL" );


namespace TMATCH
{

bool PIN::IsIsomorphic( const PIN& b, TOPOLOGY_MISMATCH_REASON& aReason ) const
{
    if( m_conns.size() != b.m_conns.size() )
    {
        wxLogTrace( traceTopoMatch,
                    wxT( "[conns mismatch n1 %d n2 %d c-ref %d c-other %d thispin %s-%s "
                         "otherpin %s-%s" ),
            m_netcode,
            b.m_netcode,
            (int) m_conns.size(),
            (int) b.m_conns.size(),
            m_parent->m_reference,
            m_ref,
            b.m_parent->m_reference,
            b.m_ref );

        aReason.m_reference = m_parent->GetParent()->GetReferenceAsString();
        aReason.m_candidate = b.m_parent->GetParent()->GetReferenceAsString();
        aReason.m_reason = wxString::Format(
                _( "Pad %s of %s connects to %lu pads, but candidate pad %s of %s connects to %lu." ), m_ref,
                aReason.m_reference, static_cast<unsigned long>( m_conns.size() ), b.m_ref, aReason.m_candidate,
                static_cast<unsigned long>( b.m_conns.size() ) );

        for( auto c : m_conns )
        {
            wxLogTrace( traceTopoMatch, wxT( "%s-%s " ), c->m_parent->m_reference, c->m_ref );
        }

        wxLogTrace( traceTopoMatch, wxT( "||" ) );

        for( auto c : b.m_conns )
        {
            wxLogTrace( traceTopoMatch, wxT( "%s-%s " ), c->m_parent->m_reference, c->m_ref );
        }


        wxLogTrace( traceTopoMatch, wxT( "] " ) );
        return false;
    }

    if( m_conns.empty() )
    {
        wxLogTrace( traceTopoMatch, wxT( "[conns empty]" ) );
        return true;
    }

    std::vector<bool> matches( m_conns.size() );

    for( int i = 0; i < m_conns.size(); i++ )
        matches[i] = false;

    int nref = 0;

    for( auto& cref : m_conns )
    {
        for( int i = 0; i < m_conns.size(); i++ )
        {
            if( b.m_conns[i]->IsTopologicallySimilar( *cref ) )
            {
                matches[nref] = true;
                break;
            }
        }

        nref++;
    }

    for( int i = 0; i < m_conns.size(); i++ )
    {
        if( !matches[i] )
        {
            aReason.m_reference = m_parent->GetParent()->GetReferenceAsString();
            aReason.m_candidate = b.m_parent->GetParent()->GetReferenceAsString();
            aReason.m_reason = wxString::Format(
                    _( "Pad %s of %s cannot match candidate pad %s of %s due to differing connectivity." ), m_ref,
                    aReason.m_reference, b.m_ref, aReason.m_candidate );

            return false;
        }
    }

    return true;
}


std::unordered_map<int, int> buildBaseNetMapping( const BACKTRACK_STAGE& aMatches )
{
    std::unordered_map<int, int> mapping;
    mapping.reserve( aMatches.GetMatchingComponentPairs().size() * 4 );

    for( const auto& [tgtCmp, refCmp] : aMatches.GetMatchingComponentPairs() )
    {
        auto& refPins = refCmp->Pins();
        auto& tgtPins = tgtCmp->Pins();

        for( size_t i = 0; i < refPins.size() && i < tgtPins.size(); i++ )
            mapping[refPins[i]->GetNetCode()] = tgtPins[i]->GetNetCode();
    }

    return mapping;
}


bool checkCandidateNetConsistency( const std::unordered_map<int, int>& aBaseMapping,
                                   COMPONENT* aRef, COMPONENT* aTgt,
                                   TOPOLOGY_MISMATCH_REASON& aReason )
{
    if( aRef->Pins().size() != aTgt->Pins().size() )
    {
        aReason.m_reference = aRef->GetParent()->GetReferenceAsString();
        aReason.m_candidate = aTgt->GetParent()->GetReferenceAsString();
        aReason.m_reason =
                wxString::Format( _( "Component %s expects %lu matching pads but candidate %s provides %lu." ),
                                  aReason.m_reference, static_cast<unsigned long>( aRef->Pins().size() ),
                                  aReason.m_candidate, static_cast<unsigned long>( aTgt->Pins().size() ) );
        return false;
    }

    // Track net mappings introduced by this candidate's pins that aren't yet in the
    // base mapping.  Two pins sharing the same ref net must map to the same target net.
    std::unordered_map<int, int> candidateAdditions;

    for( size_t i = 0; i < aRef->Pins().size(); i++ )
    {
        int refNet = aRef->Pins()[i]->GetNetCode();
        int tgtNet = aTgt->Pins()[i]->GetNetCode();

        auto baseIt = aBaseMapping.find( refNet );

        if( baseIt != aBaseMapping.end() )
        {
            if( baseIt->second != tgtNet )
            {
                wxLogTrace( traceTopoMatch, wxT( "nets inconsistent\n" ) );

                aReason.m_reference = aRef->GetParent()->GetReferenceAsString();
                aReason.m_candidate = aTgt->GetParent()->GetReferenceAsString();

                wxString refNetName;
                wxString tgtNetName;

                if( const BOARD* board = aRef->GetParent()->GetBoard() )
                {
                    if( const NETINFO_ITEM* net = board->FindNet( refNet ) )
                        refNetName = net->GetNetname();
                }

                if( const BOARD* board = aTgt->GetParent()->GetBoard() )
                {
                    if( const NETINFO_ITEM* net = board->FindNet( tgtNet ) )
                        tgtNetName = net->GetNetname();
                }

                if( refNetName.IsEmpty() )
                    refNetName = wxString::Format( _( "net %d" ), refNet );

                if( tgtNetName.IsEmpty() )
                    tgtNetName = wxString::Format( _( "net %d" ), tgtNet );

                aReason.m_reason = wxString::Format(
                        _( "Pad %s of %s is on net %s but its match in candidate %s is on net %s." ),
                        aRef->Pins()[i]->GetReference(), aReason.m_reference, refNetName,
                        aReason.m_candidate, tgtNetName );

                return false;
            }

            continue;
        }

        auto localIt = candidateAdditions.find( refNet );

        if( localIt != candidateAdditions.end() )
        {
            if( localIt->second != tgtNet )
            {
                wxLogTrace( traceTopoMatch, wxT( "nets inconsistent (candidate internal)\n" ) );

                aReason.m_reference = aRef->GetParent()->GetReferenceAsString();
                aReason.m_candidate = aTgt->GetParent()->GetReferenceAsString();
                aReason.m_reason = wxString::Format(
                        _( "Pad %s of %s has inconsistent net mapping in candidate %s." ),
                        aRef->Pins()[i]->GetReference(), aReason.m_reference,
                        aReason.m_candidate );

                return false;
            }

            continue;
        }

        candidateAdditions[refNet] = tgtNet;
    }

    return true;
}


std::vector<COMPONENT*>
CONNECTION_GRAPH::findMatchingComponents( COMPONENT*                             aRef,
                                          const std::vector<COMPONENT*>&         aStructuralMatches,
                                          const BACKTRACK_STAGE&                 partialMatches,
                                          std::vector<TOPOLOGY_MISMATCH_REASON>& aMismatchReasons,
                                          const std::atomic<bool>*               aCancelled )
{
    if( aCancelled && aCancelled->load( std::memory_order_relaxed ) )
        return {};

    PROF_TIMER timerFmc;

    aMismatchReasons.clear();
    std::vector<COMPONENT*> matches;
    int  candidatesChecked = 0;
    int  skippedLocked = 0;

    // Build the net consistency map from locked pairs once for this entire evaluation
    // pass, rather than rebuilding it from scratch for every candidate.
    std::unordered_map<int, int> baseNetMapping = buildBaseNetMapping( partialMatches );

    double netCheckMs = 0.0;

    for( COMPONENT* cmpTarget : aStructuralMatches )
    {
        if( partialMatches.m_locked.find( cmpTarget ) != partialMatches.m_locked.end() )
        {
            skippedLocked++;
            continue;
        }

        candidatesChecked++;

        wxLogTrace( traceTopoMatch, wxT( "Check '%s'/'%s' " ), aRef->m_reference,
                    cmpTarget->m_reference );

        TOPOLOGY_MISMATCH_REASON localReason;
        localReason.m_reference = aRef->GetParent()->GetReferenceAsString();
        localReason.m_candidate = cmpTarget->GetParent()->GetReferenceAsString();

        PROF_TIMER timerNet;
        bool netResult = checkCandidateNetConsistency( baseNetMapping, aRef, cmpTarget, localReason );
        timerNet.Stop();
        netCheckMs += timerNet.msecs();

        if( netResult )
        {
            wxLogTrace( traceTopoMatch, wxT( "match!\n" ) );
            matches.push_back( cmpTarget );
        }
        else
        {
            wxLogTrace( traceTopoMatch, wxT( "Reject [net topo mismatch]\n" ) );
            aMismatchReasons.push_back( localReason );
        }
    }

    PROF_TIMER timerScore;

    std::unordered_map<COMPONENT*, double> simScores;
    simScores.reserve( matches.size() );

    for( COMPONENT* match : matches )
    {
        int n = 0;

        for( size_t i = 0; i < aRef->m_pins.size(); i++ )
        {
            if( aRef->m_pins[i]->GetNetCode() == match->m_pins[i]->GetNetCode() )
                n++;
        }

        simScores[match] = static_cast<double>( n ) / static_cast<double>( aRef->m_pins.size() );
    }

    std::sort( matches.begin(), matches.end(),
               [&]( COMPONENT* a, COMPONENT* b ) -> bool
               {
                   double simA = simScores[a];
                   double simB = simScores[b];

                   if( simA != simB )
                       return simA > simB;

                   return a->GetParent()->GetReferenceAsString()
                          < b->GetParent()->GetReferenceAsString();
               } );

    timerScore.Stop();

    if( matches.empty() )
    {
        TOPOLOGY_MISMATCH_REASON reason;
        reason.m_reference = aRef->GetParent()->GetReferenceAsString();
        reason.m_reason = _( "No compatible component found in the target area." );

        if( aMismatchReasons.empty() )
            aMismatchReasons.push_back( reason );
    }

    timerFmc.Stop();

    wxLogTrace( traceTopoMatchDetail,
                wxT( "  findMatch '%s' (%d pins): %s total, checked %d/%d structural, "
                     "netCheck %0.3f ms, score %0.3f ms, %d matches" ),
                aRef->m_reference, aRef->GetPinCount(), timerFmc.to_string(),
                candidatesChecked, (int) aStructuralMatches.size(),
                netCheckMs, timerScore.msecs(),
                (int) matches.size() );

    return matches;
}


void CONNECTION_GRAPH::breakTie( COMPONENT* aRef, std::vector<COMPONENT*>& aMatches ) const
{
    if( aMatches.size() <= 1 )
        return;

    wxString candidateRefs;

    for( size_t i = 0; i < aMatches.size(); i++ )
    {
        if( i > 0 )
            candidateRefs += wxT( ", " );

        candidateRefs += aMatches[i]->GetParent()->GetReferenceAsString();
    }

    wxLogTrace( traceTopoMatch, wxT( "Topology tie for %s: %s" ),
                aRef->GetParent()->GetReferenceAsString(), candidateRefs );

    if( breakTieBySymbolUuid( aRef, aMatches ) )
    {
        wxLogTrace( traceTopoMatchDetail, wxT( "Broke tie with symbol UUID match for %s" ),
                    aRef->GetParent()->GetReferenceAsString() );
    }
    // TODO: other tie breakers can be added, e.g. based on position or reference designators,
    // just waiting for actual user test cases
    else
    {
        wxLogTrace( traceTopoMatchDetail, wxT( "No tie breakers worked for %s, leaving match order alone." ),
                    aRef->GetParent()->GetReferenceAsString() );
    }
}


bool CONNECTION_GRAPH::breakTieBySymbolUuid( COMPONENT* aRef, std::vector<COMPONENT*>& aMatches ) const
{
    auto getSymbolInstanceUuid =
            []( const FOOTPRINT* aFootprint ) -> KIID
            {
                if( !aFootprint )
                    return niluuid;

                const KIID_PATH& path = aFootprint->GetPath();

                if( path.empty() )
                    return niluuid;

                const KIID& symbolUuid = path.back();

                return symbolUuid;
            };

    FOOTPRINT* refFp = aRef ? aRef->GetParent() : nullptr;
    const KIID refSymbolUuid = getSymbolInstanceUuid( refFp );
    wxString   candidateSymbolUuids;
    wxString   matchingSymbolCandidates;
    int        symbolUuidHitCount = 0;
    int        uniqueMatchIdx = -1;

    if( refSymbolUuid == niluuid )
    {
        wxLogTrace( traceTopoMatchDetail, wxT( "Tie symbol UUID unavailable for %s" ),
                    refFp ? refFp->GetReferenceAsString() : wxString( wxT( "<null>" ) ) );
        return false;
    }

    // Inspect every tied candidate and collect:
    // 1) a detailed ref->symbol UUID mapping string for traces, and
    // 2) the subset of candidates whose symbol-path tail UUID matches the reference.
    for( size_t i = 0; i < aMatches.size(); i++ )
    {
        FOOTPRINT*     candidateFp = aMatches[i]->GetParent();
        const wxString candidateRef = candidateFp->GetReferenceAsString();
        const KIID     candidateSymbolUuid = getSymbolInstanceUuid( candidateFp );

        if( i > 0 )
            candidateSymbolUuids += wxT( ", " );

        if( candidateSymbolUuid == niluuid )
            candidateSymbolUuids += candidateRef + wxT( "=<none>" );
        else
            candidateSymbolUuids += candidateRef + wxT( "=" ) + candidateSymbolUuid.AsString();

        if( candidateSymbolUuid == refSymbolUuid )
        {
            if( uniqueMatchIdx < 0 )
                uniqueMatchIdx = static_cast<int>( i );

            symbolUuidHitCount++;

            if( !matchingSymbolCandidates.IsEmpty() )
                matchingSymbolCandidates += wxT( ", " );

            matchingSymbolCandidates += candidateRef;
        }
    }

    wxLogTrace( traceTopoMatchDetail, wxT( "Tie reference symbol UUID for %s: %s (hits=%d)" ),
                refFp->GetReferenceAsString(), refSymbolUuid.AsString(), symbolUuidHitCount );

    wxLogTrace( traceTopoMatchDetail, wxT( "Tie candidate symbol UUIDs: %s" ), candidateSymbolUuids );

    // One match is what we want, we should have one match between the source symbol instance
    // and the destination only since in theory we are repeating across two instances of the same sheet
    if( symbolUuidHitCount == 1 )
    {
        wxLogTrace( traceTopoMatchDetail, wxT( "Symbol UUID unique match (usable) for %s: %s" ),
                    refFp->GetReferenceAsString(), matchingSymbolCandidates );

        std::rotate( aMatches.begin(), aMatches.begin() + uniqueMatchIdx, aMatches.begin() + uniqueMatchIdx + 1 );

        wxLogTrace( traceTopoMatchDetail, wxT( "Applied symbol UUID tie-break for %s: selected %s" ),
                    refFp->GetReferenceAsString(), aMatches.front()->GetParent()->GetReferenceAsString() );

        return true;
    }
    // Copy and pasting footprints can result in multiple matches
    else if( symbolUuidHitCount > 1 )
    {
        wxLogTrace( traceTopoMatchDetail, wxT( "Symbol UUID multiple matches (not usable) for %s: %s" ),
                    refFp->GetReferenceAsString(), matchingSymbolCandidates );
        return false;
    }
    // Probably not sheet instances, break the tie some other way
    else
    {
        wxLogTrace( traceTopoMatchDetail, wxT( "No symbol UUID candidate match (not usable) for %s" ),
                    refFp->GetReferenceAsString() );
        return false;
    }
}


void COMPONENT::sortPinsByName()
{
    std::sort( m_pins.begin(), m_pins.end(),
               []( PIN* a, PIN* b )
               {
                   return a->GetReference() < b->GetReference();
               } );
}


void CONNECTION_GRAPH::sortByPinCount()
{
    std::sort( m_components.begin(), m_components.end(),
               []( COMPONENT* a, COMPONENT* b )
               {
                   if( a->GetPinCount() != b->GetPinCount() )
                       return a->GetPinCount() > b->GetPinCount();

                   return a->GetParent()->GetReferenceAsString() < b->GetParent()->GetReferenceAsString();
               } );
}


void CONNECTION_GRAPH::BuildConnectivity()
{
    std::map<int, std::vector<PIN*>> nets;

    sortByPinCount();

    for( auto c : m_components )
    {
        c->sortPinsByName();

        for( auto p : c->Pins() )
        {
            if( p->GetNetCode() > 0 )
                nets[p->GetNetCode()].push_back( p );
        }
    }

    for( auto& [netcode, pins] : nets )
    {
        wxLogTrace( traceTopoMatch, wxT( "net %d: %d connections\n" ), netcode,
                    (int) pins.size() );

        for( PIN* p : pins )
        {
            p->m_conns.reserve( pins.size() - 1 );

            for( PIN* p2 : pins )
            {
                if( p != p2 )
                    p->m_conns.push_back( p2 );
            }
        }
    }

/*    for( auto c : m_components )
        for( auto p : c->Pins() )
        {
            printf("pin %s: \n", p->m_ref.c_str().AsChar() );

            for( auto c : p->m_conns )
                printf( "%s ", c->m_ref.c_str().AsChar() );
            printf("\n");
        }
         */
}


bool CONNECTION_GRAPH::FindIsomorphism( CONNECTION_GRAPH* aTarget, COMPONENT_MATCHES& aResult,
                                        std::vector<TOPOLOGY_MISMATCH_REASON>& aMismatchReasons,
                                        const ISOMORPHISM_PARAMS& aParams )
{
    std::vector<BACKTRACK_STAGE> stack;
    BACKTRACK_STAGE              top;

    aMismatchReasons.clear();

    if( aParams.m_totalComponents )
        aParams.m_totalComponents->store( (int) m_components.size(), std::memory_order_relaxed );

    PROF_TIMER timerTotal;
    int        backtrackCount = 0;
    double     mrvTotalMs = 0.0;

    std::vector<TOPOLOGY_MISMATCH_REASON> localReasons;

    if( m_components.empty()|| aTarget->m_components.empty() )
    {
        TOPOLOGY_MISMATCH_REASON reason;
        reason.m_reason = _( "One or both of the areas has no components assigned." );
        aMismatchReasons.push_back( reason );
        return false;
    }

    if( m_components.size() != aTarget->m_components.size() )
    {
        TOPOLOGY_MISMATCH_REASON reason;
        reason.m_reason = _( "Component count mismatch" );
        aMismatchReasons.push_back( reason );
        return false;
    }

    // Structural compatibility (MatchesWith) depends only on pin count, footprint ID, and
    // pin connection topology -- all immutable graph properties.  Precompute it once per
    // source component so the backtracking loop never repeats these comparisons.
    size_t numRef = m_components.size();
    std::vector<std::vector<COMPONENT*>> structuralMatches( numRef );

    PROF_TIMER timerPrecompute;
    {
        thread_pool& tp = GetKiCadThreadPool();
        std::vector<std::future<void>> futures;
        futures.reserve( numRef );

        const std::atomic<bool>* cancelled = aParams.m_cancelled;

        for( size_t i = 0; i < numRef; i++ )
        {
            futures.emplace_back( tp.submit_task(
                    [this, i, aTarget, &structuralMatches, cancelled]()
                    {
                        if( cancelled && cancelled->load( std::memory_order_relaxed ) )
                            return;

                        COMPONENT* ref = m_components[i];
                        TOPOLOGY_MISMATCH_REASON reason;

                        for( COMPONENT* tgt : aTarget->m_components )
                        {
                            if( ref->MatchesWith( tgt, reason ) )
                                structuralMatches[i].push_back( tgt );
                        }
                    } ) );
        }

        for( auto& f : futures )
            f.wait();
    }
    timerPrecompute.Stop();

    wxLogTrace( traceTopoMatchDetail,
                wxT( "Structural precomputation: %s (%d source x %d target)" ),
                timerPrecompute.to_string(), (int) numRef,
                (int) aTarget->m_components.size() );

    if( aParams.m_cancelled && aParams.m_cancelled->load( std::memory_order_relaxed ) )
        return false;

    top.m_ref = m_components.front();
    top.m_refIndex = 0;

    stack.push_back( top );

    bool matchFound = false;
    int  nloops = 0;

    while( !stack.empty() )
    {
        if( aParams.m_cancelled && aParams.m_cancelled->load( std::memory_order_relaxed ) )
            return false;

        nloops++;
        auto& current = stack.back();

        for( auto it = current.m_locked.begin(); it != current.m_locked.end(); it++ )
        {
            if (it->second == current.m_ref)
            {
                wxLogTrace( traceTopoMatch, wxT( "stk: Remove %s from locked\n" ),
                            current.m_ref->m_reference );
                current.m_locked.erase( it );
                break;
            }
        }

        if( nloops >= c_ITER_LIMIT )
        {
            wxLogTrace( traceTopoMatch, wxT( "stk: Iter cnt exceeded\n" ) );

            TOPOLOGY_MISMATCH_REASON reason;
            reason.m_reason = _( "Iteration count exceeded (timeout)" );

            if( aMismatchReasons.empty() )
                aMismatchReasons.push_back( reason );
            else
                aMismatchReasons.insert( aMismatchReasons.begin(), reason );

            return false;
        }

        if( current.m_currentMatch < 0 )
        {
            PROF_TIMER timerInitMatch;

            localReasons.clear();
            current.m_matches = aTarget->findMatchingComponents(
                    current.m_ref, structuralMatches[current.m_refIndex],
                    current, localReasons, aParams.m_cancelled );

            timerInitMatch.Stop();

            wxLogTrace( traceTopoMatchDetail,
                        wxT( "iter %d: initial match for '%s' (%d pins): %s, %d candidates" ),
                        nloops, current.m_ref->m_reference, current.m_ref->GetPinCount(),
                        timerInitMatch.to_string(), (int) current.m_matches.size() );

            if( current.m_matches.empty() && aMismatchReasons.empty() && !localReasons.empty() )
                aMismatchReasons = localReasons;

            current.m_currentMatch = 0;
        }

        wxLogTrace( traceTopoMatch, wxT( "stk: Current '%s' stack %d cm %d/%d locked %d/%d\n" ),
                    current.m_ref->m_reference, (int) stack.size(), current.m_currentMatch,
                    (int) current.m_matches.size(), (int) current.m_locked.size(),
                    (int) m_components.size() );

        if( current.m_currentMatch == 0 && current.m_matches.size() > 1 )
            breakTie( current.m_ref, current.m_matches );

        if ( current.m_matches.empty() )
        {
            wxLogTrace( traceTopoMatch, wxT( "stk: No matches at all, going up [level=%d]\n" ),
                        (int) stack.size() );
            stack.pop_back();
            backtrackCount++;
            continue;
        }

        if( current.m_currentMatch >= 0 && current.m_currentMatch >= current.m_matches.size() )
        {
            wxLogTrace( traceTopoMatch, wxT( "stk: No more matches, going up [level=%d]\n" ),
                        (int) stack.size() );
            stack.pop_back();
            backtrackCount++;
            continue;
        }

        auto& match = current.m_matches[current.m_currentMatch];

        wxLogTrace( traceTopoMatch, wxT( "stk: candidate '%s', match list : ( " ),
                    current.m_matches[current.m_currentMatch]->m_reference, current.m_refIndex );

        for( auto m : current.m_matches )
            wxLogTrace( traceTopoMatch, wxT( "%s " ), m->GetParent()->GetReferenceAsString() );

        wxLogTrace( traceTopoMatch, wxT( "\n" ) );

        current.m_currentMatch++;
        current.m_locked[match] = current.m_ref;

        if( aParams.m_matchedComponents )
        {
            aParams.m_matchedComponents->store( (int) current.m_locked.size(),
                                                std::memory_order_relaxed );
        }

        if( current.m_locked.size() == m_components.size() )
        {
            current.m_nloops = nloops;

            aResult.clear();
            aMismatchReasons.clear();

            for( auto iter : current.m_locked )
                aResult[ iter.second->GetParent() ] = iter.first->GetParent();

            timerTotal.Stop();
            wxLogTrace( traceTopoMatch,
                        wxT( "Isomorphism: %s, %d iterations, %d backtracks, "
                             "MRV total %0.1f ms (%d candidates)" ),
                        timerTotal.to_string(), nloops, backtrackCount, mrvTotalMs,
                        (int) m_components.size() );

            return true;
        }


        // MRV heuristic: find the unlocked component with the fewest candidate matches.
        // Collect unlocked components, then evaluate them in parallel since each
        // findMatchingComponents call is independent (read-only on graphs and current stage).
        struct MRV_CANDIDATE
        {
            COMPONENT*                            m_cmp;
            size_t                                m_index;
            std::vector<COMPONENT*>               m_matches;
            std::vector<TOPOLOGY_MISMATCH_REASON> m_reasons;
        };

        std::vector<MRV_CANDIDATE> mrvCandidates;

        // Build a set of all ref-components already locked so we can skip them in O(1)
        // instead of scanning m_locked values for each component.
        std::unordered_set<COMPONENT*> lockedRefs;
        lockedRefs.reserve( current.m_locked.size() );

        for( const auto& [tgt, ref] : current.m_locked )
            lockedRefs.insert( ref );

        for( size_t i = 0; i < m_components.size(); i++ )
        {
            COMPONENT* cmp = m_components[i];

            if( cmp != current.m_ref && lockedRefs.find( cmp ) == lockedRefs.end() )
                mrvCandidates.push_back( { cmp, i, {}, {} } );
        }

        static const size_t MRV_PARALLEL_THRESHOLD = 4;

        PROF_TIMER timerMrv;

        if( mrvCandidates.size() >= MRV_PARALLEL_THRESHOLD )
        {
            thread_pool& tp = GetKiCadThreadPool();
            std::vector<std::future<void>> futures;
            futures.reserve( mrvCandidates.size() );

            const std::atomic<bool>* cancelled = aParams.m_cancelled;

            for( MRV_CANDIDATE& c : mrvCandidates )
            {
                futures.emplace_back( tp.submit_task(
                        [&c, aTarget, &current, &structuralMatches, cancelled]()
                        {
                            c.m_matches = aTarget->findMatchingComponents(
                                    c.m_cmp, structuralMatches[c.m_index],
                                    current, c.m_reasons, cancelled );
                        } ) );
            }

            for( auto& f : futures )
                f.wait();
        }
        else
        {
            for( MRV_CANDIDATE& c : mrvCandidates )
            {
                c.m_matches = aTarget->findMatchingComponents(
                        c.m_cmp, structuralMatches[c.m_index],
                        current, c.m_reasons, aParams.m_cancelled );
            }
        }

        timerMrv.Stop();
        double mrvMs = timerMrv.msecs();
        mrvTotalMs += mrvMs;

        wxLogTrace( traceTopoMatchDetail,
                    wxT( "iter %d: MRV scan %0.3f ms, %d unlocked candidates" ),
                    nloops, mrvMs, (int) mrvCandidates.size() );

        if( aParams.m_cancelled && aParams.m_cancelled->load( std::memory_order_relaxed ) )
            return false;

        int                     minMatches = std::numeric_limits<int>::max();
        COMPONENT*              altNextRef = nullptr;
        COMPONENT*              bestNextRef = nullptr;
        int                     bestRefIndex = 0;
        int                     altRefIndex = 0;
        std::vector<COMPONENT*> bestMatches;

        for( MRV_CANDIDATE& c : mrvCandidates )
        {
            int nMatches = static_cast<int>( c.m_matches.size() );

            if( nMatches == 1 )
            {
                bestNextRef = c.m_cmp;
                bestRefIndex = static_cast<int>( c.m_index );
                bestMatches = std::move( c.m_matches );
                break;
            }
            else if( nMatches == 0 )
            {
                altNextRef = c.m_cmp;
                altRefIndex = static_cast<int>( c.m_index );

                if( aMismatchReasons.empty() && !c.m_reasons.empty() )
                    aMismatchReasons = c.m_reasons;
            }
            else if( nMatches < minMatches )
            {
                minMatches = nMatches;
                bestNextRef = c.m_cmp;
                bestRefIndex = static_cast<int>( c.m_index );
                bestMatches = std::move( c.m_matches );
            }
        }

        BACKTRACK_STAGE next( current );

        if( bestNextRef )
        {
            wxLogTrace( traceTopoMatchDetail,
                        wxT( "iter %d: MRV picked '%s' (%d matches, best of %d)" ),
                        nloops, bestNextRef->m_reference,
                        (int) bestMatches.size(), (int) mrvCandidates.size() );

            next.m_ref = bestNextRef;
            next.m_refIndex = bestRefIndex;
            next.m_matches = std::move( bestMatches );
            next.m_currentMatch = 0;
        }
        else
        {
            wxLogTrace( traceTopoMatchDetail,
                        wxT( "iter %d: MRV dead end, alt='%s'" ),
                        nloops, altNextRef ? altNextRef->m_reference : wxString( "(none)" ) );

            next.m_ref = altNextRef;
            next.m_refIndex = altRefIndex;
            next.m_currentMatch = -1;
        }

        stack.push_back( next );
    };

    timerTotal.Stop();
    wxLogTrace( traceTopoMatch,
                wxT( "Isomorphism: %s, %d iterations, %d backtracks, "
                     "MRV total %0.1f ms (%d candidates)" ),
                timerTotal.to_string(), nloops, backtrackCount, mrvTotalMs,
                (int) m_components.size() );

    return false;
}


#if 0
int main()
{
    FILE * f = fopen("connectivity.dump","rb" );
    auto cgRef = loadCGraph(f);
    auto cgTarget = loadCGraph(f);

    cgRef->buildConnectivity();
    cgTarget->buildConnectivity();

    int attempts = 0;
    int max_loops = 0;

    for( ;; )
    {
        cgRef->shuffle();
        cgTarget->shuffle();

        const BacktrackStage latest = cgRef->matchCGraphs( cgTarget );

        if( !latest.locked.size() )
        {
            printf("MATCH FAIL\n");
            break;
        }

        //printf("loops: %d\n", latest.nloops );
        //printf("Locked: %d\n", latest.locked.size() );

        //if (matchFound)
        //{
          //  for( auto& iter : latest.locked )
            //{
             //   printf("%-10s : %-10s\n", iter.first->reference.c_str(), iter.second->reference.c_str() );
            //}

        //}

        if( latest.nloops > max_loops )
        {
            max_loops = latest.nloops;
        }

        if (attempts % 10000 == 0)
        {
            printf("attempts: %d maxloops: %d\n", attempts, max_loops );
        }

        attempts++;

    }

    fclose(f);

    return 0;
}

#endif


COMPONENT::COMPONENT( const wxString& aRef, FOOTPRINT* aParentFp,
                      std::optional<VECTOR2I> aRaOffset ) :
        m_raOffset( aRaOffset ),
        m_reference( aRef ),
        m_parentFootprint( aParentFp )
{
    m_prefix = UTIL::GetRefDesPrefix( aRef );
}


bool COMPONENT::isChannelSuffix( const wxString& aSuffix )
{
    if( aSuffix.IsEmpty() )
        return true;

    for( wxUniChar ch : aSuffix )
    {
        if( std::isalpha( static_cast<int>( ch ) ) )
            return false;
    }

    return true;
}


bool COMPONENT::prefixesShareCommonBase( const wxString& aPrefixA, const wxString& aPrefixB )
{
    if( aPrefixA == aPrefixB )
        return true;

    size_t commonLen = 0;
    size_t minLen = std::min( aPrefixA.length(), aPrefixB.length() );

    while( commonLen < minLen && aPrefixA[commonLen] == aPrefixB[commonLen] )
        commonLen++;

    if( commonLen == 0 )
        return false;

    wxString suffixA = aPrefixA.Mid( commonLen );
    wxString suffixB = aPrefixB.Mid( commonLen );

    return isChannelSuffix( suffixA ) && isChannelSuffix( suffixB );
}


bool COMPONENT::IsSameKind( const COMPONENT& b ) const
{
    if( !prefixesShareCommonBase( m_prefix, b.m_prefix ) )
        return false;

    return ( m_parentFootprint->GetFPID() == b.m_parentFootprint->GetFPID() )
           || ( m_parentFootprint->GetFPID().empty() && b.m_parentFootprint->GetFPID().empty() );
}


void COMPONENT::AddPin( PIN* aPin )
{
    m_pins.push_back( aPin );
    aPin->SetParent( this );
}


bool COMPONENT::MatchesWith( COMPONENT* b, TOPOLOGY_MISMATCH_REASON& aReason )
{
    if( GetPinCount() != b->GetPinCount() )
    {
        aReason.m_reference = GetParent()->GetReferenceAsString();
        aReason.m_candidate = b->GetParent()->GetReferenceAsString();
        aReason.m_reason =
                wxString::Format( _( "Component %s has %d pads but candidate %s has %d." ), aReason.m_reference,
                                  GetPinCount(), aReason.m_candidate, b->GetPinCount() );
        return false;
    }

    if( !IsSameKind( *b ) )
    {
        aReason.m_reference = GetParent()->GetReferenceAsString();
        aReason.m_candidate = b->GetParent()->GetReferenceAsString();

        if( !prefixesShareCommonBase( m_prefix, b->m_prefix ) )
        {
            aReason.m_reason = wxString::Format(
                    _( "Reference prefix mismatch: %s uses prefix '%s' but candidate %s uses '%s'." ),
                    aReason.m_reference, m_prefix, aReason.m_candidate, b->m_prefix );
        }
        else
        {
            wxString refFootprint = GetParent()->GetFPIDAsString();
            wxString candFootprint = b->GetParent()->GetFPIDAsString();

            if( refFootprint.IsEmpty() )
                refFootprint = _( "(no library ID)" );

            if( candFootprint.IsEmpty() )
                candFootprint = _( "(no library ID)" );

            aReason.m_reason =
                    wxString::Format( _( "Library link mismatch: %s expects '%s' but candidate %s is '%s'." ),
                                      aReason.m_reference, refFootprint, aReason.m_candidate, candFootprint );
        }

        return false;
    }

    for( int pin = 0; pin < b->GetPinCount(); pin++ )
    {
        if( !b->m_pins[pin]->IsIsomorphic( *m_pins[pin], aReason ) )
        {
            if( aReason.m_reason.IsEmpty() )
            {
                aReason.m_reference = GetParent()->GetReferenceAsString();
                aReason.m_candidate = b->GetParent()->GetReferenceAsString();
                aReason.m_reason = wxString::Format( _( "Component pads differ between %s and %s." ),
                                                     aReason.m_reference, aReason.m_candidate );
            }

            return false;
        }

    }

    return true;
}


void CONNECTION_GRAPH::AddFootprint( FOOTPRINT* aFp, const VECTOR2I& aOffset )
{
    auto cmp = new COMPONENT( aFp->GetReference(), aFp );

    for( auto pad : aFp->Pads() )
    {
        auto pin = new PIN( );
        pin->m_netcode = pad->GetNetCode();
        pin->m_ref = pad->GetNumber();
        cmp->AddPin( pin );
    }

    m_components.push_back( cmp );
}


std::unique_ptr<CONNECTION_GRAPH>
CONNECTION_GRAPH::BuildFromFootprintSet( const std::set<FOOTPRINT*>& aFps )
{
    auto cgraph = std::make_unique<CONNECTION_GRAPH>();
    VECTOR2I ref(0, 0);

    if( aFps.size() > 0 )
        ref = (*aFps.begin())->GetPosition();

    for( auto fp : aFps )
    {
        cgraph->AddFootprint( fp, fp->GetPosition() - ref );
    }

    cgraph->BuildConnectivity();

    return std::move(cgraph);
}


CONNECTION_GRAPH::CONNECTION_GRAPH()
{

}


CONNECTION_GRAPH::~CONNECTION_GRAPH()
{
    for( COMPONENT* fp : m_components )
    {
        delete fp;
    }
}


COMPONENT::~COMPONENT()
{
    for( PIN* p : m_pins )
    {
        delete p;
    }
}


}; // namespace TMATCH
