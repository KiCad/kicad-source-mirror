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

#include "length_delay_calculation/length_delay_calculation.h"

#include <wx/log.h>

#include <board.h>
#include <board_design_settings.h>
#include <footprint.h>
#include <geometry/geometry_utils.h>
#include <geometry/shape_circle.h>
#include <geometry/circle.h>
#include <pad.h>
#include <pcb_track.h>


bool LENGTH_DELAY_CALCULATION::findArcPadIntersection( const SHAPE_ARC&                       aArc,
                                                       const std::shared_ptr<SHAPE_POLY_SET>& aPadShape,
                                                       const VECTOR2I& aInsidePoint, VECTOR2I& aIntersection )
{
    bool    found = false;
    int64_t bestDistSq = std::numeric_limits<int64_t>::max();

    for( int i = 0; i < aPadShape->OutlineCount(); i++ )
    {
        const SHAPE_LINE_CHAIN& outline = aPadShape->Outline( i );

        for( int j = 0; j < outline.SegmentCount(); j++ )
        {
            const SEG&            seg = outline.CSegment( j );
            std::vector<VECTOR2I> intersections;

            if( !aArc.IntersectLine( seg, &intersections ) )
                continue;

            for( const VECTOR2I& pt : intersections )
            {
                if( !seg.Contains( pt ) )
                    continue;

                int64_t distSq = ( pt - aInsidePoint ).SquaredEuclideanNorm();

                if( distSq < bestDistSq )
                {
                    bestDistSq = distSq;
                    aIntersection = pt;
                    found = true;
                }
            }
        }
    }

    return found;
}


void LENGTH_DELAY_CALCULATION::clipLineToPad( SHAPE_LINE_CHAIN& aLine, const PAD* aPad, PCB_LAYER_ID aLayer,
                                              bool aForward )
{
    wxASSERT( aLine.PointCount() >= 2 );

    const int start = aForward ? 0 : aLine.PointCount() - 1;
    const int delta = aForward ? 1 : -1;

    const auto& shape = aPad->GetEffectivePolygon( aLayer, ERROR_INSIDE );

    // Find the first point OUTSIDE the pad
    int      firstOutside = -1;
    VECTOR2I intersectionPt;
    bool     hasIntersection = false;

    for( int vertex = start + delta; aForward ? vertex < aLine.PointCount() : vertex >= 0; vertex += delta )
    {
        if( !shape->Contains( aLine.GetPoint( vertex ) ) )
        {
            firstOutside = vertex;
            int prevVertex = vertex - delta;

            // Check if the crossing segment is part of an arc
            ssize_t arcIdx = aLine.ArcIndex( prevVertex );

            if( arcIdx >= 0 && aLine.ArcIndex( vertex ) == arcIdx )
            {
                hasIntersection = findArcPadIntersection( aLine.Arc( arcIdx ), shape, aLine.GetPoint( prevVertex ),
                                                          intersectionPt );
            }

            // Fallback to segment intersection if arc intersection didn't work
            if( !hasIntersection )
            {
                SEG      seg( aLine.GetPoint( vertex ), aLine.GetPoint( prevVertex ) );
                VECTOR2I loc;

                if( shape->Collide( seg, 0, nullptr, &loc ) )
                {
                    intersectionPt = loc;
                    hasIntersection = true;
                }
            }

            break;
        }
    }

    if( firstOutside < 0 )
        return; // All points inside pad, nothing to clip

    // Build new chain using Slice (preserves arcs correctly without index corruption)
    SHAPE_LINE_CHAIN newChain;

    if( aForward )
    {
        // Chain: padCenter -> intersection -> [firstOutside to end]
        newChain.Append( aPad->GetPosition() );
        if( hasIntersection )
            newChain.Append( intersectionPt );
        newChain.Append( aLine.Slice( firstOutside, -1 ) );
    }
    else
    {
        // Chain: [0 to firstOutside] -> intersection -> padCenter
        newChain.Append( aLine.Slice( 0, firstOutside ) );
        if( hasIntersection )
            newChain.Append( intersectionPt );
        newChain.Append( aPad->GetPosition() );
    }

    aLine = newChain;
}


LENGTH_DELAY_STATS LENGTH_DELAY_CALCULATION::CalculateLengthDetails(
        std::vector<LENGTH_DELAY_CALCULATION_ITEM>& aItems, const PATH_OPTIMISATIONS aOptimisations,
        const PAD* aStartPad, const PAD* aEndPad, const LENGTH_DELAY_LAYER_OPT aLayerOpt,
        const LENGTH_DELAY_DOMAIN_OPT aDomain, LENGTH_DELAY_ITEM_DETAILS* aPerItemLengthDelays ) const
{
    const bool doTrace = wxLog::IsAllowedTraceMask( wxT( "PNS_TUNE" ) );

    if( doTrace )
    {
        wxLogTrace( wxT( "PNS_TUNE" ), wxT( "" ) );
        wxLogTrace( wxT( "PNS_TUNE" ), wxT( "========== CalculateLengthDetails START ==========" ) );
        wxLogTrace( wxT( "PNS_TUNE" ), wxT( "CalculateLengthDetails: input has %zu items" ), aItems.size() );
        wxLogTrace( wxT( "PNS_TUNE" ), wxT( "CalculateLengthDetails: optimisations - OptimiseVias=%d, MergeTracks=%d, OptimiseTracesInPads=%d, InferViaInPad=%d" ),
                    aOptimisations.OptimiseVias, aOptimisations.MergeTracks,
                    aOptimisations.OptimiseTracesInPads, aOptimisations.InferViaInPad );

        // Count initial items by type
        int initialPads = 0, initialVias = 0, initialLines = 0, initialUnknown = 0;

        for( const LENGTH_DELAY_CALCULATION_ITEM& item : aItems )
        {
            switch( item.Type() )
            {
                case LENGTH_DELAY_CALCULATION_ITEM::TYPE::PAD:  initialPads++;    break;
                case LENGTH_DELAY_CALCULATION_ITEM::TYPE::VIA:  initialVias++;    break;
                case LENGTH_DELAY_CALCULATION_ITEM::TYPE::LINE: initialLines++;   break;
                default:                                        initialUnknown++; break;
            }
        }

        wxLogTrace( wxT( "PNS_TUNE" ), wxT( "CalculateLengthDetails: initial items - PADs=%d, VIAs=%d, LINEs=%d, UNKNOWN=%d" ),
                    initialPads, initialVias, initialLines, initialUnknown );
    }

    // If this set of items has not been optimised, optimise for shortest electrical path
    if( aOptimisations.OptimiseVias || aOptimisations.MergeTracks || aOptimisations.MergeTracks )
    {
        if( doTrace )
            wxLogTrace( wxT( "PNS_TUNE" ), wxT( "CalculateLengthDetails: performing optimisations..." ) );

        std::vector<LENGTH_DELAY_CALCULATION_ITEM*> pads;
        std::vector<LENGTH_DELAY_CALCULATION_ITEM*> lines;
        std::vector<LENGTH_DELAY_CALCULATION_ITEM*> vias;

        // Map of line endpoints to line objects
        std::map<VECTOR2I, std::unordered_set<LENGTH_DELAY_CALCULATION_ITEM*>> linesPositionMap;

        // Map of pad positions to pad objects
        std::map<VECTOR2I, std::unordered_set<LENGTH_DELAY_CALCULATION_ITEM*>> padsPositionMap;

        for( LENGTH_DELAY_CALCULATION_ITEM& item : aItems )
        {
            if( item.Type() == LENGTH_DELAY_CALCULATION_ITEM::TYPE::PAD )
            {
                pads.emplace_back( &item );
                padsPositionMap[item.GetPad()->GetPosition()].insert( &item );
            }
            else if( item.Type() == LENGTH_DELAY_CALCULATION_ITEM::TYPE::VIA )
            {
                vias.emplace_back( &item );
            }
            else if( item.Type() == LENGTH_DELAY_CALCULATION_ITEM::TYPE::LINE )
            {
                lines.emplace_back( &item );
                linesPositionMap[item.GetLine().CPoint( 0 )].insert( &item );
                linesPositionMap[item.GetLine().CLastPoint()].insert( &item );
            }
        }

        if( aOptimisations.OptimiseVias )
        {
            if( doTrace )
                wxLogTrace( wxT( "PNS_TUNE" ), wxT( "CalculateLengthDetails: optimising via layers (%zu vias)" ), vias.size() );

            optimiseVias( vias, lines, linesPositionMap, padsPositionMap );
        }

        if( aOptimisations.MergeTracks )
        {
            if( doTrace )
                wxLogTrace( wxT( "PNS_TUNE" ), wxT( "CalculateLengthDetails: merging tracks (%zu lines)" ), lines.size() );

            mergeLines( lines, linesPositionMap );
        }

        // Clip traces inside VIA pads after merging
        if( aOptimisations.OptimiseVias )
        {
            for( LENGTH_DELAY_CALCULATION_ITEM* via : vias )
            {
                const PCB_VIA* pcbVia = via->GetVia();

                for( LENGTH_DELAY_CALCULATION_ITEM* lineItem : lines )
                {
                    if( lineItem->GetMergeStatus() != LENGTH_DELAY_CALCULATION_ITEM::MERGE_STATUS::MERGED_IN_USE )
                    {
                        continue;
                    }

                    OptimiseTraceInVia( lineItem->GetLine(), pcbVia, lineItem->GetStartLayer() );
                }
            }
        }

        if( aOptimisations.OptimiseTracesInPads )
        {
            if( doTrace )
                wxLogTrace( wxT( "PNS_TUNE" ), wxT( "CalculateLengthDetails: optimising traces in pads" ) );

            optimiseTracesInPads( pads, lines );
        }
    }

    LENGTH_DELAY_STATS details;

    // Create the layer detail maps if required
    if( aLayerOpt == LENGTH_DELAY_LAYER_OPT::WITH_LAYER_DETAIL )
    {
        if( doTrace )
            wxLogTrace( wxT( "PNS_TUNE" ), wxT( "CalculateLengthDetails: creating layer detail maps" ) );

        details.LayerLengths = std::make_unique<std::map<PCB_LAYER_ID, int64_t>>();

        if( aDomain == LENGTH_DELAY_DOMAIN_OPT::WITH_DELAY_DETAIL )
            details.LayerDelays = std::make_unique<std::map<PCB_LAYER_ID, int64_t>>();
    }

    const bool useHeight = m_board->GetDesignSettings().m_UseHeightForLengthCalcs;

    if( doTrace )
        wxLogTrace( wxT( "PNS_TUNE" ), wxT( "CalculateLengthDetails: useHeight=%d" ), useHeight );

    // If this is a contiguous set of items, check if we have an inferred fanout via at either end. Note that this
    // condition only arises as a result of how PNS assembles tuning paths - for DRC / net inspector calculations these
    // fanout vias will be present in the object set and therefore do not need to be inferred
    if( aOptimisations.InferViaInPad && useHeight )
    {
        if( doTrace )
            wxLogTrace( wxT( "PNS_TUNE" ), wxT( "CalculateLengthDetails: inferring vias in pads" ) );

        std::pair<int64_t, int64_t> inferredStartPadViaDetails{ 0, 0 };
        std::pair<int64_t, int64_t> inferredEndPadViaDetails{ 0, 0 };

        const bool withDelay = aDomain == LENGTH_DELAY_DOMAIN_OPT::WITH_DELAY_DETAIL;
        inferViaInPad( aStartPad, aItems.front(), details, inferredStartPadViaDetails, withDelay );
        inferViaInPad( aEndPad, aItems.back(), details, inferredEndPadViaDetails, withDelay );

        if( aPerItemLengthDelays )
        {
            aPerItemLengthDelays->InferredStartViaLength = inferredStartPadViaDetails.first;
            aPerItemLengthDelays->InferredStartViaDelay = inferredStartPadViaDetails.second;
            aPerItemLengthDelays->InferredEndViaLength = inferredEndPadViaDetails.first;
            aPerItemLengthDelays->InferredEndViaDelay = inferredEndPadViaDetails.second;
        }
    }

    // Add stats for each item
    int processedPads = 0, processedVias = 0, processedLines = 0;
    int mergedRetired = 0, unknownType = 0;

    // Output per-item details if required
    if( aPerItemLengthDelays )
    {
        aPerItemLengthDelays->LengthsAndDelays.clear();
        aPerItemLengthDelays->LengthsAndDelays.resize( aItems.size(), { 0, 0 } );
    }

    auto setPerItemLengthDetail = [aPerItemLengthDelays]( const size_t idx, const int64_t value )
    {
        if( !aPerItemLengthDelays )
            return;

        aPerItemLengthDelays->LengthsAndDelays[idx].first = value;
    };

    auto setPerItemDelayDetail = [aPerItemLengthDelays]( const size_t idx, const int64_t value )
    {
        if( !aPerItemLengthDelays )
            return;

        aPerItemLengthDelays->LengthsAndDelays[idx].second = value;
    };

    if( doTrace )
        wxLogTrace( wxT( "PNS_TUNE" ), wxT( "CalculateLengthDetails: processing %zu items..." ), aItems.size() );

    for( size_t i = 0; i < aItems.size(); ++i )
    {
        const LENGTH_DELAY_CALCULATION_ITEM& item = aItems[i];

        // Don't include merged items
        if( item.GetMergeStatus() == LENGTH_DELAY_CALCULATION_ITEM::MERGE_STATUS::MERGED_RETIRED
            || item.Type() == LENGTH_DELAY_CALCULATION_ITEM::TYPE::UNKNOWN )
        {
            if( item.GetMergeStatus() == LENGTH_DELAY_CALCULATION_ITEM::MERGE_STATUS::MERGED_RETIRED )
                mergedRetired++;
            else
                unknownType++;
            continue;
        }

        // Calculate the space domain statistics
        if( item.Type() == LENGTH_DELAY_CALCULATION_ITEM::TYPE::LINE )
        {
            const int64_t length = item.GetLine().Length();

            setPerItemLengthDetail( i, length );
            details.TrackLength += length;
            processedLines++;

            if( details.LayerLengths )
                ( *details.LayerLengths )[item.GetStartLayer()] += length;
        }
        else if( item.Type() == LENGTH_DELAY_CALCULATION_ITEM::TYPE::VIA && useHeight )
        {
            const auto [layerStart, layerEnd] = item.GetLayers();
            int64_t viaHeight = StackupHeight( layerStart, layerEnd );
            setPerItemLengthDetail( i, viaHeight );
            details.ViaLength += static_cast<int>( viaHeight );
            details.NumVias += 1;
            processedVias++;

            if( doTrace )
                wxLogTrace( wxT( "PNS_TUNE" ), wxT( "CalculateLengthDetails: via from layer %d to %d, height=%lld" ),
                            layerStart, layerEnd, viaHeight );
        }
        else if( item.Type() == LENGTH_DELAY_CALCULATION_ITEM::TYPE::PAD )
        {
            int64_t padToDie = item.GetPad()->GetPadToDieLength();
            setPerItemLengthDetail( i, padToDie );
            details.PadToDieLength += static_cast<int>( padToDie );
            details.NumPads += 1;
            processedPads++;

            if( doTrace && padToDie > 0 )
                wxLogTrace( wxT( "PNS_TUNE" ), wxT( "CalculateLengthDetails: pad with pad-to-die length=%lld" ), padToDie );
        }
    }

    if( doTrace )
    {
        wxLogTrace( wxT( "PNS_TUNE" ), wxT( "CalculateLengthDetails: processed items - PADs=%d, VIAs=%d, LINEs=%d" ),
                    processedPads, processedVias, processedLines );
        wxLogTrace( wxT( "PNS_TUNE" ), wxT( "CalculateLengthDetails: skipped items - merged/retired=%d, unknown=%d" ),
                    mergedRetired, unknownType );
    }

    // Calculate the time domain statistics if required
    if( aDomain == LENGTH_DELAY_DOMAIN_OPT::WITH_DELAY_DETAIL && !aItems.empty() )
    {
        if( doTrace )
            wxLogTrace( wxT( "PNS_TUNE" ), wxT( "CalculateLengthDetails: calculating time domain statistics" ) );

        // TODO(JJ): Populate this
        TUNING_PROFILE_GEOMETRY_CONTEXT ctx;
        ctx.NetClass = aItems.front().GetEffectiveNetClass(); // We don't care if this is merged for net class lookup

        const std::vector<int64_t> itemDelays = m_tuningProfileParameters->GetPropagationDelays( aItems, ctx );

        wxASSERT( itemDelays.size() == aItems.size() );

        for( size_t i = 0; i < aItems.size(); ++i )
        {
            const LENGTH_DELAY_CALCULATION_ITEM& item = aItems[i];

            if( item.GetMergeStatus() == LENGTH_DELAY_CALCULATION_ITEM::MERGE_STATUS::MERGED_RETIRED
                || item.Type() == LENGTH_DELAY_CALCULATION_ITEM::TYPE::UNKNOWN )
            {
                continue;
            }

            setPerItemDelayDetail( i, itemDelays[i] );

            if( item.Type() == LENGTH_DELAY_CALCULATION_ITEM::TYPE::LINE )
            {
                details.TrackDelay += itemDelays[i];

                if( details.LayerDelays )
                    ( *details.LayerDelays )[item.GetStartLayer()] += itemDelays[i];
            }
            else if( item.Type() == LENGTH_DELAY_CALCULATION_ITEM::TYPE::VIA && useHeight )
            {
                details.ViaDelay += itemDelays[i];
            }
            else if( item.Type() == LENGTH_DELAY_CALCULATION_ITEM::TYPE::PAD )
            {
                details.PadToDieDelay += itemDelays[i];
            }
        }

        if( doTrace )
            wxLogTrace( wxT( "PNS_TUNE" ), wxT( "CalculateLengthDetails: total delays - Track=%lld, Via=%lld, PadToDie=%lld" ),
                        details.TrackDelay, details.ViaDelay, details.PadToDieDelay );
    }

    if( doTrace )
    {
        wxLogTrace( wxT( "PNS_TUNE" ), wxT( "CalculateLengthDetails: RESULTS:" ) );
        wxLogTrace( wxT( "PNS_TUNE" ), wxT( "  Track length: %lld" ), details.TrackLength );
        wxLogTrace( wxT( "PNS_TUNE" ), wxT( "  Via length: %d (from %d vias)" ), details.ViaLength, details.NumVias );
        wxLogTrace( wxT( "PNS_TUNE" ), wxT( "  Pad-to-die length: %d (from %d pads)" ), details.PadToDieLength, details.NumPads );
        wxLogTrace( wxT( "PNS_TUNE" ), wxT( "  TOTAL LENGTH: %lld" ), details.TotalLength() );
        wxLogTrace( wxT( "PNS_TUNE" ), wxT( "========== CalculateLengthDetails END ==========" ) );
        wxLogTrace( wxT( "PNS_TUNE" ), wxT( "" ) );
    }

    return details;
}


void LENGTH_DELAY_CALCULATION::inferViaInPad( const PAD* aPad, const LENGTH_DELAY_CALCULATION_ITEM& aItem,
                                              LENGTH_DELAY_STATS&          aDetails,
                                              std::pair<int64_t, int64_t>& aInferredViaLengthDelay,
                                              const bool                   aWithDelayDetail ) const
{
    if( aPad && aItem.Type() == LENGTH_DELAY_CALCULATION_ITEM::TYPE::LINE )
    {
        const PCB_LAYER_ID startBottomLayer = aItem.GetStartLayer();
        const LSET         padLayers = aPad->Padstack().LayerSet();

        if( !padLayers.Contains( startBottomLayer ) )
        {
            // This must be either F_Cu or B_Cu
            const PCB_LAYER_ID padLayer = padLayers.Contains( F_Cu ) ? F_Cu : B_Cu;

            aDetails.NumVias += 1;
            const int64_t height = StackupHeight( startBottomLayer, padLayer );
            aDetails.ViaLength += height;
            aInferredViaLengthDelay.first = height;

            // Look up via delay details if required
            if( aWithDelayDetail )
            {
                TUNING_PROFILE_GEOMETRY_CONTEXT ctx;
                ctx.NetClass = aItem.GetEffectiveNetClass();
                const int64_t delay = m_tuningProfileParameters->GetViaPropagationDelay( startBottomLayer, padLayer,
                                                                                         F_Cu, B_Cu, ctx );
                aDetails.ViaDelay += delay;
                aInferredViaLengthDelay.second = delay;
            }
        }
    }
}


int64_t LENGTH_DELAY_CALCULATION::CalculateLength( std::vector<LENGTH_DELAY_CALCULATION_ITEM>& aItems,
                                                   const PATH_OPTIMISATIONS aOptimisations, const PAD* aStartPad,
                                                   const PAD* aEndPad ) const
{
    return CalculateLengthDetails( aItems, aOptimisations, aStartPad, aEndPad ).TotalLength();
}


int64_t LENGTH_DELAY_CALCULATION::CalculateDelay( std::vector<LENGTH_DELAY_CALCULATION_ITEM>& aItems,
                                                  const PATH_OPTIMISATIONS aOptimisations, const PAD* aStartPad,
                                                  const PAD* aEndPad ) const
{
    return CalculateLengthDetails( aItems, aOptimisations, aStartPad, aEndPad, LENGTH_DELAY_LAYER_OPT::NO_LAYER_DETAIL,
                                   LENGTH_DELAY_DOMAIN_OPT::WITH_DELAY_DETAIL )
            .TotalDelay();
}


int LENGTH_DELAY_CALCULATION::StackupHeight( const PCB_LAYER_ID aFirstLayer, const PCB_LAYER_ID aSecondLayer ) const
{
    if( !m_board || !m_board->GetDesignSettings().m_UseHeightForLengthCalcs )
        return 0;

    if( m_board->GetDesignSettings().m_HasStackup )
    {
        const BOARD_STACKUP& stackup = m_board->GetDesignSettings().GetStackupDescriptor();
        return stackup.GetLayerDistance( aFirstLayer, aSecondLayer );
    }
    else
    {
        BOARD_STACKUP stackup;
        stackup.BuildDefaultStackupList( &m_board->GetDesignSettings(), m_board->GetCopperLayerCount() );
        return stackup.GetLayerDistance( aFirstLayer, aSecondLayer );
    }
}


void LENGTH_DELAY_CALCULATION::mergeLines(
        std::vector<LENGTH_DELAY_CALCULATION_ITEM*>&                            aLines,
        std::map<VECTOR2I, std::unordered_set<LENGTH_DELAY_CALCULATION_ITEM*>>& aLinesPositionMap )
{
    // Vector of pads, and an associated flag to indicate whether they have been visited by the clustering algorithm
    std::vector<LENGTH_DELAY_CALCULATION_ITEM*> pads;

    auto removeFromPositionMap = [&aLinesPositionMap]( LENGTH_DELAY_CALCULATION_ITEM* line )
    {
        aLinesPositionMap[line->GetLine().CPoint( 0 )].erase( line );
        aLinesPositionMap[line->GetLine().CLastPoint()].erase( line );
    };

    // Attempts to merge unmerged lines in to aPrimaryLine
    auto tryMerge = [&removeFromPositionMap, &aLinesPositionMap]( const MERGE_POINT                    aMergePoint,
                                                                  const VECTOR2I&                      aMergePos,
                                                                  const LENGTH_DELAY_CALCULATION_ITEM* aPrimaryItem,
                                                                  SHAPE_LINE_CHAIN& aPrimaryLine, bool* aDidMerge )
    {
        const auto startItr = aLinesPositionMap.find( aMergePos );

        if( startItr == aLinesPositionMap.end() )
            return;

        std::unordered_set<LENGTH_DELAY_CALCULATION_ITEM*>& startItems = startItr->second;

        if( startItems.size() != 1 )
            return;

        LENGTH_DELAY_CALCULATION_ITEM* lineToMerge = *startItems.begin();

        // Don't merge if line is an arc
        if( !lineToMerge->GetLine().CArcs().empty() )
            return;

        // Don't merge if lines are on different layers
        if( aPrimaryItem->GetStartLayer() != lineToMerge->GetStartLayer() )
            return;

        // Merge the lines
        lineToMerge->SetMergeStatus( LENGTH_DELAY_CALCULATION_ITEM::MERGE_STATUS::MERGED_RETIRED );
        mergeShapeLineChains( aPrimaryLine, lineToMerge->GetLine(), aMergePoint );
        removeFromPositionMap( lineToMerge );
        *aDidMerge = true;
    };

    // Cluster all lines in to contiguous entities
    for( LENGTH_DELAY_CALCULATION_ITEM* primaryItem : aLines )
    {
        // Don't start with an already merged line
        if( primaryItem->GetMergeStatus() != LENGTH_DELAY_CALCULATION_ITEM::MERGE_STATUS::UNMERGED )
            continue;

        // Remove starting line from the position map
        removeFromPositionMap( primaryItem );

        SHAPE_LINE_CHAIN& primaryLine = primaryItem->GetLine();

        // Merge all endpoints
        primaryItem->SetMergeStatus( LENGTH_DELAY_CALCULATION_ITEM::MERGE_STATUS::MERGED_IN_USE );
        bool mergeComplete = false;

        while( !mergeComplete )
        {
            bool startMerged = false;
            bool endMerged = false;

            VECTOR2I startPos = primaryLine.CPoint( 0 );
            VECTOR2I endPos = primaryLine.CLastPoint();

            tryMerge( MERGE_POINT::START, startPos, primaryItem, primaryLine, &startMerged );
            tryMerge( MERGE_POINT::END, endPos, primaryItem, primaryLine, &endMerged );

            mergeComplete = !startMerged && !endMerged;
        }
    }
}


void LENGTH_DELAY_CALCULATION::mergeShapeLineChains( SHAPE_LINE_CHAIN& aPrimary, const SHAPE_LINE_CHAIN& aSecondary,
                                                     const MERGE_POINT aMergePoint )
{
    if( aMergePoint == MERGE_POINT::START )
    {
        if( aSecondary.GetPoint( 0 ) == aPrimary.GetPoint( 0 ) )
        {
            for( auto itr = aSecondary.CPoints().begin() + 1; itr != aSecondary.CPoints().end(); ++itr )
                aPrimary.Insert( 0, *itr );
        }
        else
        {
            wxASSERT( aSecondary.CLastPoint() == aPrimary.GetPoint( 0 ) );

            for( auto itr = aSecondary.CPoints().rbegin() + 1; itr != aSecondary.CPoints().rend(); ++itr )
                aPrimary.Insert( 0, *itr );
        }
    }
    else
    {
        if( aSecondary.GetPoint( 0 ) == aPrimary.CLastPoint() )
        {
            for( auto itr = aSecondary.CPoints().begin() + 1; itr != aSecondary.CPoints().end(); ++itr )
                aPrimary.Append( *itr );
        }
        else
        {
            wxASSERT( aSecondary.CLastPoint() == aPrimary.CLastPoint() );

            for( auto itr = aSecondary.CPoints().rbegin() + 1; itr != aSecondary.CPoints().rend(); ++itr )
                aPrimary.Append( *itr );
        }
    }
}


void LENGTH_DELAY_CALCULATION::optimiseTracesInPads( const std::vector<LENGTH_DELAY_CALCULATION_ITEM*>& aPads,
                                                     const std::vector<LENGTH_DELAY_CALCULATION_ITEM*>& aLines )
{
    for( LENGTH_DELAY_CALCULATION_ITEM* padItem : aPads )
    {
        const PAD* pad = padItem->GetPad();

        for( LENGTH_DELAY_CALCULATION_ITEM* lineItem : aLines )
        {
            // Ignore merged lines
            if( lineItem->GetMergeStatus() != LENGTH_DELAY_CALCULATION_ITEM::MERGE_STATUS::MERGED_IN_USE )
                continue;

            const PCB_LAYER_ID pcbLayer = lineItem->GetStartLayer();
            SHAPE_LINE_CHAIN&  line = lineItem->GetLine();

            OptimiseTraceInPad( line, pad, pcbLayer );
        }
    }
}


void LENGTH_DELAY_CALCULATION::optimiseVias(
        const std::vector<LENGTH_DELAY_CALCULATION_ITEM*>& aVias, std::vector<LENGTH_DELAY_CALCULATION_ITEM*>& aLines,
        std::map<VECTOR2I, std::unordered_set<LENGTH_DELAY_CALCULATION_ITEM*>>&       aLinesPositionMap,
        const std::map<VECTOR2I, std::unordered_set<LENGTH_DELAY_CALCULATION_ITEM*>>& aPadsPositionMap ) const
{
    for( LENGTH_DELAY_CALCULATION_ITEM* via : aVias )
    {
        const PCB_VIA* pcbVia = via->GetVia();

        std::unordered_set<LENGTH_DELAY_CALCULATION_ITEM*> connectedLines;

        const VECTOR2I viaPos = pcbVia->GetPosition();

        auto exactMatch = aLinesPositionMap.find( viaPos );

        if( exactMatch != aLinesPositionMap.end() )
            connectedLines.insert( exactMatch->second.begin(), exactMatch->second.end() );

        int maxRadius = 0;

        pcbVia->Padstack().ForEachUniqueLayer(
                [&]( PCB_LAYER_ID layer )
                {
                    maxRadius = std::max( maxRadius, pcbVia->GetWidth( layer ) / 2 );
                } );

        const int64_t maxRadiusSq = static_cast<int64_t>( maxRadius ) * maxRadius;

        for( const auto& [pos, lineSet] : aLinesPositionMap )
        {
            if( pos == viaPos )
                continue;

            if( ( pos - viaPos ).SquaredEuclideanNorm() > maxRadiusSq )
                continue;

            for( LENGTH_DELAY_CALCULATION_ITEM* lineItem : lineSet )
            {
                PCB_LAYER_ID           layer = lineItem->GetStartLayer();
                std::shared_ptr<SHAPE> shape = pcbVia->GetEffectiveShape( layer, FLASHING::ALWAYS_FLASHED );

                if( shape && shape->Collide( pos, 0 ) )
                    connectedLines.insert( lineItem );
            }
        }

        const PAD* coincidentPad = nullptr;

        for( PAD* pad : m_board->GetConnectivity()->GetConnectedPads( pcbVia ) )
        {
            coincidentPad = pad;
            break;
        }

        LSET spanLayers;

        for( const LENGTH_DELAY_CALCULATION_ITEM* lineItem : connectedLines )
            spanLayers.set( lineItem->GetStartLayer() );

        if( coincidentPad )
        {
            PCB_LAYER_ID padSideLayer;

            if( coincidentPad->GetAttribute() == PAD_ATTRIB::SMD || coincidentPad->GetAttribute() == PAD_ATTRIB::CONN )
            {
                padSideLayer = *coincidentPad->Padstack().LayerSet().CuStack().begin();
            }
            else
            {
                padSideLayer = coincidentPad->GetParentFootprint()->GetLayer();
            }

            spanLayers.set( padSideLayer );
        }

        wxLogTrace( wxT( "PNS_TUNE" ),
                    wxT( "optimiseVias: via@(%d,%d) connectedLines=%zu coincidentPad=%d "
                         "spanLayerCount=%d" ),
                    viaPos.x, viaPos.y, connectedLines.size(), coincidentPad ? 1 : 0,
                    static_cast<int>( spanLayers.count() ) );

        const LSEQ cuStack = spanLayers.CuStack();

        if( cuStack.empty() )
        {
            // Nothing connects to this via
            via->SetLayers( pcbVia->GetLayer(), pcbVia->GetLayer() );
        }
        else if( cuStack.size() == 1 )
        {
            // Stub via
            via->SetLayers( cuStack.front(), cuStack.front() );
        }
        else
        {
            // Signal transitions layers
            via->SetLayers( cuStack.front(), cuStack.back() );
        }
    }
}


void LENGTH_DELAY_CALCULATION::OptimiseTraceInPad( SHAPE_LINE_CHAIN& aLine, const PAD* aPad,
                                                   const PCB_LAYER_ID aPcbLayer )
{
    // Only consider lines which terminate in the pad
    if( aLine.CPoint( 0 ) != aPad->GetPosition() && aLine.CLastPoint() != aPad->GetPosition() )
        return;

    if( !aPad->FlashLayer( aPcbLayer ) )
        return;

    const auto& shape = aPad->GetEffectivePolygon( aPcbLayer, ERROR_INSIDE );

    if( shape->Contains( aLine.CPoint( 0 ) ) )
        clipLineToPad( aLine, aPad, aPcbLayer, true );
    else if( shape->Contains( aLine.CLastPoint() ) )
        clipLineToPad( aLine, aPad, aPcbLayer, false );
}


void LENGTH_DELAY_CALCULATION::clipLineToVia( SHAPE_LINE_CHAIN& aLine, const PCB_VIA* aVia, PCB_LAYER_ID aLayer,
                                              bool aForward )
{
    wxASSERT( aLine.PointCount() >= 2 );

    const int start = aForward ? 0 : aLine.PointCount() - 1;
    const int delta = aForward ? 1 : -1;

    std::shared_ptr<SHAPE> viaShape = aVia->GetEffectiveShape( aLayer, FLASHING::ALWAYS_FLASHED );

    if( !viaShape )
        return;

    const SHAPE_CIRCLE* viaCircle = dynamic_cast<const SHAPE_CIRCLE*>( viaShape.get() );

    if( !viaCircle )
        return;

    // Find the first point OUTSIDE the via pad
    int      firstOutside = -1;
    VECTOR2I intersectionPt;
    bool     hasIntersection = false;

    for( int vertex = start + delta; aForward ? vertex < aLine.PointCount() : vertex >= 0; vertex += delta )
    {
        if( !viaShape->Collide( aLine.GetPoint( vertex ), 0 ) )
        {
            firstOutside = vertex;
            int prevVertex = vertex - delta;

            SEG seg( aLine.GetPoint( vertex ), aLine.GetPoint( prevVertex ) );

            CIRCLE circle( viaCircle->GetCenter(), viaCircle->GetRadius() );

            std::vector<VECTOR2I> pts = circle.Intersect( seg );

            if( !pts.empty() )
            {
                // Pick the intersection closest to the outside vertex
                VECTOR2I outside = aLine.GetPoint( vertex );
                intersectionPt = pts[0];

                for( size_t i = 1; i < pts.size(); i++ )
                {
                    if( ( pts[i] - outside ).SquaredEuclideanNorm()
                        < ( intersectionPt - outside ).SquaredEuclideanNorm() )
                    {
                        intersectionPt = pts[i];
                    }
                }

                hasIntersection = true;
            }

            break;
        }
    }

    if( firstOutside < 0 )
        return;

    SHAPE_LINE_CHAIN newChain;

    if( aForward )
    {
        // viaCenter -> intersection -> [firstOutside to end]
        newChain.Append( aVia->GetPosition() );

        if( hasIntersection )
            newChain.Append( intersectionPt );

        newChain.Append( aLine.Slice( firstOutside, -1 ) );
    }
    else
    {
        // [0 to firstOutside] -> intersection -> viaCenter
        newChain.Append( aLine.Slice( 0, firstOutside ) );

        if( hasIntersection )
            newChain.Append( intersectionPt );

        newChain.Append( aVia->GetPosition() );
    }

    aLine = newChain;
}


void LENGTH_DELAY_CALCULATION::OptimiseTraceInVia( SHAPE_LINE_CHAIN& aLine, const PCB_VIA* aVia, PCB_LAYER_ID aLayer )
{
    std::shared_ptr<SHAPE> viaShape = aVia->GetEffectiveShape( aLayer, FLASHING::ALWAYS_FLASHED );

    if( !viaShape )
        return;

    if( viaShape->Collide( aLine.CPoint( 0 ), 0 ) )
        clipLineToVia( aLine, aVia, aLayer, true );
    else if( viaShape->Collide( aLine.CLastPoint(), 0 ) )
        clipLineToVia( aLine, aVia, aLayer, false );
}


bool LENGTH_DELAY_CALCULATION::IsPointInsideViaPad( const PCB_VIA* aVia, const VECTOR2I& aPoint, PCB_LAYER_ID aLayer )
{
    std::shared_ptr<SHAPE> shape = aVia->GetEffectiveShape( aLayer, FLASHING::ALWAYS_FLASHED );
    return shape && shape->Collide( aPoint, 0 );
}


LENGTH_DELAY_CALCULATION_ITEM
LENGTH_DELAY_CALCULATION::GetLengthCalculationItem( const BOARD_CONNECTED_ITEM* aBoardItem ) const
{
    if( const PCB_TRACK* track = dynamic_cast<const PCB_TRACK*>( aBoardItem ) )
    {
        if( track->Type() == PCB_VIA_T )
        {
            const PCB_VIA* via = static_cast<const PCB_VIA*>( track );

            LENGTH_DELAY_CALCULATION_ITEM item;
            item.SetVia( via );
            item.CalculateViaLayers( m_board );
            item.SetEffectiveNetClass( via->GetEffectiveNetClass() );

            return item;
        }

        if( track->Type() == PCB_ARC_T )
        {
            const PCB_ARC*   arcParent = static_cast<const PCB_ARC*>( track );
            SHAPE_ARC        shapeArc( arcParent->GetStart(), arcParent->GetMid(), arcParent->GetEnd(),
                                       arcParent->GetWidth() );
            SHAPE_LINE_CHAIN chainArc( shapeArc );

            LENGTH_DELAY_CALCULATION_ITEM item;
            item.SetLine( chainArc );
            item.SetLayers( track->GetLayer() );
            item.SetEffectiveNetClass( arcParent->GetEffectiveNetClass() );

            return item;
        }

        if( track->Type() == PCB_TRACE_T )
        {
            std::vector<VECTOR2I> points{ track->GetStart(), track->GetEnd() };
            SHAPE_LINE_CHAIN      shape( points );

            LENGTH_DELAY_CALCULATION_ITEM item;
            item.SetLine( shape );
            item.SetLayers( track->GetLayer() );
            item.SetEffectiveNetClass( track->GetEffectiveNetClass() );

            return item;
        }
    }
    else if( const PAD* pad = dynamic_cast<const PAD*>( aBoardItem ) )
    {
        LENGTH_DELAY_CALCULATION_ITEM item;
        item.SetPad( pad );

        const LSET&  layers = pad->Padstack().LayerSet();
        PCB_LAYER_ID firstLayer = UNDEFINED_LAYER;
        PCB_LAYER_ID secondLayer = UNDEFINED_LAYER;

        for( auto itr = layers.copper_layers_begin(); itr != layers.copper_layers_end(); ++itr )
        {
            if( firstLayer == UNDEFINED_LAYER )
                firstLayer = *itr;
            else
                secondLayer = *itr;
        }

        item.SetLayers( firstLayer, secondLayer );
        item.SetEffectiveNetClass( pad->GetEffectiveNetClass() );

        return item;
    }

    return {};
}


void LENGTH_DELAY_CALCULATION::SetTuningProfileParametersProvider(
        std::unique_ptr<TUNING_PROFILE_PARAMETERS_IFACE>&& aProvider )
{
    m_tuningProfileParameters = std::move( aProvider );
}


void LENGTH_DELAY_CALCULATION::SynchronizeTuningProfileProperties() const
{
    m_tuningProfileParameters->OnSettingsChanged();
}


int64_t LENGTH_DELAY_CALCULATION::CalculateLengthForDelay( const int64_t                          aDesiredDelay,
                                                           const TUNING_PROFILE_GEOMETRY_CONTEXT& aCtx ) const
{
    return m_tuningProfileParameters->GetTrackLengthForPropagationDelay( aDesiredDelay, aCtx );
}


int64_t LENGTH_DELAY_CALCULATION::CalculatePropagationDelayForShapeLineChain(
        const SHAPE_LINE_CHAIN& aShape, const TUNING_PROFILE_GEOMETRY_CONTEXT& aCtx ) const
{
    return m_tuningProfileParameters->CalculatePropagationDelayForShapeLineChain( aShape, aCtx );
}
