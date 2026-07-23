/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef TOOL_SNAP_FRAME_H
#define TOOL_SNAP_FRAME_H

#include <map>
#include <optional>
#include <utility>
#include <vector>

#include <snap/snap_resolver.h>


template <typename Payload>
struct SNAP_FRAME_INPUT
{
    SNAP_SOURCE_CONTEXT                 context;
    std::vector<SNAP_CANDIDATE>         candidates;
    std::optional<SNAP_STABLE_ID>       retainedId;
    std::map<SNAP_STABLE_ID, Payload>   presentation;
    std::vector<SNAP_STABLE_ID>         stickyIds;
    double                              rankingHysteresis = 0.0;
    SNAP_RESOLVER::FEASIBILITY_CALLBACK feasibility;
    SNAP_RESOLVER::TRACE_CALLBACK       trace;
};


template <typename Payload>
struct SNAP_FRAME_PRESENTATION
{
    SNAP_STABLE_ID id;
    Payload        payload;
};


template <typename Payload>
struct SNAP_FRAME_OUTPUT
{
    SNAP_RESULT                                     result;
    std::optional<SNAP_FRAME_PRESENTATION<Payload>> presentation;
};


template <typename Payload>
SNAP_FRAME_OUTPUT<Payload> ResolveSnapFrame( SNAP_FRAME_INPUT<Payload> aInput )
{
    SNAP_RESOLVER resolver;
    resolver.SetRankingHysteresis( aInput.rankingHysteresis );
    resolver.SetFeasibilityCallback( std::move( aInput.feasibility ) );
    resolver.SetTraceCallback( std::move( aInput.trace ) );
    resolver.SetStickyCandidates( std::move( aInput.stickyIds ) );
    resolver.SetRetainedCandidate( aInput.retainedId );

    for( SNAP_CANDIDATE& candidate : aInput.candidates )
        resolver.AddCandidate( std::move( candidate ) );

    SNAP_FRAME_OUTPUT<Payload> output;
    output.result = resolver.Resolve( aInput.context );

    for( const SNAP_STABLE_ID& id : output.result.accepted )
    {
        auto payload = aInput.presentation.find( id );

        if( payload != aInput.presentation.end() )
        {
            output.presentation = { id, std::move( payload->second ) };
            break;
        }
    }

    return output;
}

#endif
