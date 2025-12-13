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

#include <length_delay_calculation/length_delay_calculation.h>

#include <board.h>
#include <board_design_settings.h>
#include <geometry/geometry_utils.h>
#include <wx/log.h>


void LENGTH_DELAY_CALCULATION::clipLineToPad( SHAPE_LINE_CHAIN& aLine, const PAD* aPad, PCB_LAYER_ID aLayer,
                                              bool aForward )
{
    const int start = aForward ? 0 : aLine.PointCount() - 1;
    const int delta = aForward ? 1 : -1;

    // Note: we don't apply the clip-to-pad optimization if an arc ends in a pad
    // Room for future improvement.
    if( aLine.IsPtOnArc( start ) )
        return;

    const auto& shape = aPad->GetEffectivePolygon( aLayer, ERROR_INSIDE );

    // Skip the "first" (or last) vertex, we already know it's contained in the pad
    int clip = start;

    for( int vertex = start + delta; aForward ? vertex < aLine.PointCount() : vertex >= 0; vertex += delta )
    {
        SEG seg( aLine.GetPoint( vertex ), aLine.GetPoint( vertex - delta ) );

        bool containsA = shape->Contains( seg.A );
        bool containsB = shape->Contains( seg.B );

        if( containsA && containsB )
        {
            // Whole segment is inside: clip out this segment
            clip = vertex;
        }
        else if( containsB )
        {
            // Only one point inside: Find the intersection
            VECTOR2I loc;

            if( shape->Collide( seg, 0, nullptr, &loc ) )
            {
                aLine.Replace( vertex - delta, vertex - delta, loc );
            }
        }
    }

    if( !aForward && clip < start )
        aLine.Remove( clip + 1, start );
    else if( clip > start )
        aLine.Remove( start, clip - 1 );

    // Now connect the dots
    aLine.Insert( aForward ? 0 : aLine.PointCount(), aPad->GetPosition() );
}


LENGTH_DELAY_STATS LENGTH_DELAY_CALCULATION::CalculateLengthDetails( std::vector<LENGTH_DELAY_CALCULATION_ITEM>& aItems,
                                                                     const PATH_OPTIMISATIONS aOptimisations,
                                                                     const PAD* aStartPad, const PAD* aEndPad,
                                                                     const LENGTH_DELAY_LAYER_OPT  aLayerOpt,
                                                                     const LENGTH_DELAY_DOMAIN_OPT aDomain ) const
{
    wxLogTrace( wxT( "PNS_TUNE" ), wxT( "" ) );
    wxLogTrace( wxT( "PNS_TUNE" ), wxT( "========== CalculateLengthDetails START ==========" ) );
    wxLogTrace( wxT( "PNS_TUNE" ), wxT( "CalculateLengthDetails: input has %zu items" ), aItems.size() );
    wxLogTrace( wxT( "PNS_TUNE" ), wxT( "CalculateLengthDetails: optimisations - OptimiseViaLayers=%d, MergeTracks=%d, OptimiseTracesInPads=%d, InferViaInPad=%d" ),
                aOptimisations.OptimiseViaLayers, aOptimisations.MergeTracks,
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

    // If this set of items has not been optimised, optimise for shortest electrical path
    if( aOptimisations.OptimiseViaLayers || aOptimisations.MergeTracks || aOptimisations.MergeTracks )
    {
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

        if( aOptimisations.OptimiseViaLayers )
        {
            wxLogTrace( wxT( "PNS_TUNE" ), wxT( "CalculateLengthDetails: optimising via layers (%zu vias)" ), vias.size() );
            optimiseViaLayers( vias, lines, linesPositionMap, padsPositionMap );
        }

        if( aOptimisations.MergeTracks )
        {
            wxLogTrace( wxT( "PNS_TUNE" ), wxT( "CalculateLengthDetails: merging tracks (%zu lines)" ), lines.size() );
            mergeLines( lines, linesPositionMap );
        }

        if( aOptimisations.OptimiseTracesInPads )
        {
            wxLogTrace( wxT( "PNS_TUNE" ), wxT( "CalculateLengthDetails: optimising traces in pads" ) );
            optimiseTracesInPads( pads, lines );
        }
    }

    LENGTH_DELAY_STATS details;

    // Create the layer detail maps if required
    if( aLayerOpt == LENGTH_DELAY_LAYER_OPT::WITH_LAYER_DETAIL )
    {
        wxLogTrace( wxT( "PNS_TUNE" ), wxT( "CalculateLengthDetails: creating layer detail maps" ) );
        details.LayerLengths = std::make_unique<std::map<PCB_LAYER_ID, int64_t>>();

        if( aDomain == LENGTH_DELAY_DOMAIN_OPT::WITH_DELAY_DETAIL )
            details.LayerDelays = std::make_unique<std::map<PCB_LAYER_ID, int64_t>>();
    }

    const bool useHeight = m_board->GetDesignSettings().m_UseHeightForLengthCalcs;
    wxLogTrace( wxT( "PNS_TUNE" ), wxT( "CalculateLengthDetails: useHeight=%d" ), useHeight );

    // If this is a contiguous set of items, check if we have an inferred fanout via at either end. Note that this
    // condition only arises as a result of how PNS assembles tuning paths - for DRC / net inspector calculations these
    // fanout vias will be present in the object set and therefore do not need to be inferred
    if( aOptimisations.InferViaInPad && useHeight )
    {
        wxLogTrace( wxT( "PNS_TUNE" ), wxT( "CalculateLengthDetails: inferring vias in pads" ) );
        inferViaInPad( aStartPad, aItems.front(), details );
        inferViaInPad( aEndPad, aItems.back(), details );
    }

    // Add stats for each item
    int processedPads = 0, processedVias = 0, processedLines = 0;
    int mergedRetired = 0, unknownType = 0;

    wxLogTrace( wxT( "PNS_TUNE" ), wxT( "CalculateLengthDetails: processing %zu items..." ), aItems.size() );

    for( const LENGTH_DELAY_CALCULATION_ITEM& item : aItems )
    {
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

            details.TrackLength += length;
            processedLines++;

            if( details.LayerLengths )
                ( *details.LayerLengths )[item.GetStartLayer()] += length;
        }
        else if( item.Type() == LENGTH_DELAY_CALCULATION_ITEM::TYPE::VIA && useHeight )
        {
            const auto [layerStart, layerEnd] = item.GetLayers();
            int64_t viaHeight = StackupHeight( layerStart, layerEnd );
            details.ViaLength += viaHeight;
            details.NumVias += 1;
            processedVias++;

            wxLogTrace( wxT( "PNS_TUNE" ), wxT( "CalculateLengthDetails: via from layer %d to %d, height=%lld" ),
                        layerStart, layerEnd, viaHeight );
        }
        else if( item.Type() == LENGTH_DELAY_CALCULATION_ITEM::TYPE::PAD )
        {
            int64_t padToDie = item.GetPad()->GetPadToDieLength();
            details.PadToDieLength += padToDie;
            details.NumPads += 1;
            processedPads++;

            if( padToDie > 0 )
                wxLogTrace( wxT( "PNS_TUNE" ), wxT( "CalculateLengthDetails: pad with pad-to-die length=%lld" ), padToDie );
        }
    }

    wxLogTrace( wxT( "PNS_TUNE" ), wxT( "CalculateLengthDetails: processed items - PADs=%d, VIAs=%d, LINEs=%d" ),
                processedPads, processedVias, processedLines );
    wxLogTrace( wxT( "PNS_TUNE" ), wxT( "CalculateLengthDetails: skipped items - merged/retired=%d, unknown=%d" ),
                mergedRetired, unknownType );

    // Calculate the time domain statistics if required
    if( aDomain == LENGTH_DELAY_DOMAIN_OPT::WITH_DELAY_DETAIL && !aItems.empty() )
    {
        wxLogTrace( wxT( "PNS_TUNE" ), wxT( "CalculateLengthDetails: calculating time domain statistics" ) );

        // TODO(JJ): Populate this
        TUNING_PROFILE_GEOMETRY_CONTEXT ctx;
        ctx.NetClass = aItems.front().GetEffectiveNetClass(); // We don't care if this is merged for net class lookup

        const std::vector<int64_t> itemDelays = m_tuningProfileParameters->GetPropagationDelays( aItems, ctx );

        wxASSERT( itemDelays.size() == aItems.size() );

        for( size_t i = 0; i < aItems.size(); ++i )
        {
            const LENGTH_DELAY_CALCULATION_ITEM& item = aItems[i];

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

        wxLogTrace( wxT( "PNS_TUNE" ), wxT( "CalculateLengthDetails: total delays - Track=%lld, Via=%lld, PadToDie=%lld" ),
                    details.TrackDelay, details.ViaDelay, details.PadToDieDelay );
    }

    wxLogTrace( wxT( "PNS_TUNE" ), wxT( "CalculateLengthDetails: RESULTS:" ) );
    wxLogTrace( wxT( "PNS_TUNE" ), wxT( "  Track length: %lld" ), details.TrackLength );
    wxLogTrace( wxT( "PNS_TUNE" ), wxT( "  Via length: %d (from %d vias)" ), details.ViaLength, details.NumVias );
    wxLogTrace( wxT( "PNS_TUNE" ), wxT( "  Pad-to-die length: %d (from %d pads)" ), details.PadToDieLength, details.NumPads );
    wxLogTrace( wxT( "PNS_TUNE" ), wxT( "  TOTAL LENGTH: %lld" ), details.TotalLength() );
    wxLogTrace( wxT( "PNS_TUNE" ), wxT( "========== CalculateLengthDetails END ==========" ) );
    wxLogTrace( wxT( "PNS_TUNE" ), wxT( "" ) );

    return details;
}


void LENGTH_DELAY_CALCULATION::inferViaInPad( const PAD* aPad, const LENGTH_DELAY_CALCULATION_ITEM& aItem,
                                              LENGTH_DELAY_STATS& aDetails ) const
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
            aDetails.ViaLength += StackupHeight( startBottomLayer, padLayer );
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


void LENGTH_DELAY_CALCULATION::optimiseViaLayers(
        const std::vector<LENGTH_DELAY_CALCULATION_ITEM*>& aVias, std::vector<LENGTH_DELAY_CALCULATION_ITEM*>& aLines,
        std::map<VECTOR2I, std::unordered_set<LENGTH_DELAY_CALCULATION_ITEM*>>&       aLinesPositionMap,
        const std::map<VECTOR2I, std::unordered_set<LENGTH_DELAY_CALCULATION_ITEM*>>& aPadsPositionMap )
{
    for( LENGTH_DELAY_CALCULATION_ITEM* via : aVias )
    {
        auto lineItr = aLinesPositionMap.find( via->GetVia()->GetPosition() );

        if( lineItr == aLinesPositionMap.end() )
            continue;

        std::unordered_set<LENGTH_DELAY_CALCULATION_ITEM*>& connectedLines = lineItr->second;

        if( connectedLines.empty() )
        {
            // No connected lines - this via is floating. Set both layers to the same
            via->SetLayers( via->GetVia()->GetLayer(), via->GetVia()->GetLayer() );
        }
        else if( connectedLines.size() == 1 )
        {
            // This is either a via stub, or a via-in-pad
            bool               isViaInPad = false;
            const PCB_LAYER_ID lineLayer = ( *connectedLines.begin() )->GetStartLayer();

            auto padItr = aPadsPositionMap.find( via->GetVia()->GetPosition() );

            if( padItr != aPadsPositionMap.end() )
            {
                // This could be a via-in-pad - check for overlapping pads which are not on the line layer
                const std::unordered_set<LENGTH_DELAY_CALCULATION_ITEM*>& pads = padItr->second;

                if( pads.size() == 1 )
                {
                    const LENGTH_DELAY_CALCULATION_ITEM* padItem = *pads.begin();

                    if( !padItem->GetPad()->Padstack().LayerSet().Contains( lineLayer ) )
                    {
                        // This is probably a via-in-pad
                        isViaInPad = true;
                        via->SetLayers( lineLayer, padItem->GetStartLayer() );
                    }
                }
            }

            if( !isViaInPad )
            {
                // This is a via stub - make its electrical length 0
                via->SetLayers( lineLayer, lineLayer );
            }
        }
        else
        {
            // This via has more than one track ending at it. Calculate the connected layer span (which may be shorter
            // than the overall via span)
            LSET layers;

            for( const LENGTH_DELAY_CALCULATION_ITEM* lineItem : connectedLines )
                layers.set( lineItem->GetStartLayer() );

            LSEQ cuStack = layers.CuStack();

            PCB_LAYER_ID firstLayer = UNDEFINED_LAYER;
            PCB_LAYER_ID lastLayer = UNDEFINED_LAYER;

            for( PCB_LAYER_ID layer : cuStack )
            {
                if( firstLayer == UNDEFINED_LAYER )
                    firstLayer = layer;
                else
                    lastLayer = layer;
            }

            if( lastLayer == UNDEFINED_LAYER )
                via->SetLayers( firstLayer, firstLayer );
            else
                via->SetLayers( firstLayer, lastLayer );
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
