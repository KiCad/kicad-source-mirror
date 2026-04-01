/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "zone_utils.h"

#include <board.h>
#include <footprint.h>
#include <pad.h>
#include <pcb_track.h>
#include <thread_pool.h>
#include <zone.h>
#include <geometry/shape_poly_set.h>

#include <algorithm>
#include <cmath>
#include <future>
#include <optional>
#include <unordered_map>
#include <unordered_set>


static bool RuleAreasHaveSameProps( const ZONE& a, const ZONE& b )
{
    // This function is only used to compare rule areas, so we can assume that both a and b are rule areas
    wxASSERT( a.GetIsRuleArea() && b.GetIsRuleArea() );

    return a.GetDoNotAllowZoneFills() == b.GetDoNotAllowZoneFills()
           && a.GetDoNotAllowFootprints() == b.GetDoNotAllowFootprints()
           && a.GetDoNotAllowTracks() == b.GetDoNotAllowTracks()
           && a.GetDoNotAllowVias() == b.GetDoNotAllowVias()
           && a.GetDoNotAllowPads() == b.GetDoNotAllowPads();
}


std::vector<std::unique_ptr<ZONE>> MergeZonesWithSameOutline( std::vector<std::unique_ptr<ZONE>>&& aZones )
{
    const auto polygonsAreMergeable = []( const SHAPE_POLY_SET::POLYGON& a, const SHAPE_POLY_SET::POLYGON& b ) -> bool
    {
        if( a.size() != b.size() )
            return false;

        // NOTE: this assumes the polygons have their line chains in the same order
        // But that is not actually required for same geometry (i.e. mergeability)
        for( size_t lineChainId = 0; lineChainId < a.size(); lineChainId++ )
        {
            const SHAPE_LINE_CHAIN& chainA = a[lineChainId];
            const SHAPE_LINE_CHAIN& chainB = b[lineChainId];

            // Note: this assumes the polygons are either already simplified or that it's
            // OK to not merge even if they would be the same after simplification.
            if( chainA.PointCount() != chainB.PointCount() || chainA.BBox() != chainB.BBox()
                || !chainA.CompareGeometry( chainB ) )
            {
                // Different geometry, can't merge
                return false;
            }
        }

        return true;
    };

    const auto zonesAreMergeable = [&]( const ZONE& a, const ZONE& b ) -> bool
    {
        // Can't merge rule areas with zone fills
        if( a.GetIsRuleArea() != b.GetIsRuleArea() )
            return false;

        if( a.GetIsRuleArea() )
        {
            if( !RuleAreasHaveSameProps( a, b ) )
                return false;
        }
        else
        {
            // We could also check clearances and so on
            if( a.GetNetCode() != b.GetNetCode() )
                return false;
        }

        const SHAPE_POLY_SET* polySetA = a.Outline();
        const SHAPE_POLY_SET* polySetB = b.Outline();

        if( polySetA->OutlineCount() != polySetB->OutlineCount() )
            return false;

        if( polySetA->OutlineCount() == 0 )
        {
            // both have no outline, so they are the same, but we must not
            // derefence them, as they are empty
            return true;
        }

        // REVIEW: this assumes the zones only have a single polygon in the
        const SHAPE_POLY_SET::POLYGON& polyA = polySetA->CPolygon( 0 );
        const SHAPE_POLY_SET::POLYGON& polyB = polySetB->CPolygon( 0 );

        return polygonsAreMergeable( polyA, polyB );
    };

    std::vector<std::unique_ptr<ZONE>> deduplicatedZones;

    // Map of zone indexes that we have already merged into a prior zone
    std::vector<bool> merged( aZones.size(), false );

    for( size_t i = 0; i < aZones.size(); i++ )
    {
        // This one has already been subsumed into a prior zone, so skip it
        // and it will be dropped at the end.
        if( merged[i] )
            continue;

        ZONE&                                            primary = *aZones[i];
        LSET                                             layers = primary.GetLayerSet();
        std::unordered_map<PCB_LAYER_ID, SHAPE_POLY_SET> mergedFills;

        for( size_t j = i + 1; j < aZones.size(); j++ )
        {
            // This zone has already been subsumed by a prior zone, so it
            // cannot be merged into another primary
            if( merged[j] )
                continue;

            ZONE& candidate = *aZones[j];
            bool  canMerge = zonesAreMergeable( primary, candidate );

            if( canMerge )
            {
                for( PCB_LAYER_ID layer : candidate.GetLayerSet() )
                {
                    if( SHAPE_POLY_SET* fill = candidate.GetFill( layer ) )
                        mergedFills[layer] = *fill;
                }

                layers |= candidate.GetLayerSet();
                merged[j] = true;
            }
        }

        if( layers != primary.GetLayerSet() )
        {
            for( PCB_LAYER_ID layer : primary.GetLayerSet() )
            {
                if( SHAPE_POLY_SET* fill = primary.GetFill( layer ) )
                    mergedFills[layer] = *fill;
            }

            primary.SetLayerSet( layers );

            for( const auto& [layer, fill] : mergedFills )
                primary.SetFilledPolysList( layer, fill );

            primary.SetNeedRefill( false );
            primary.SetIsFilled( true );
        }

        // Keep this zone - it's a primary (may or may not have had other zones merged into it)
        deduplicatedZones.push_back( std::move( aZones[i] ) );
    }

    return deduplicatedZones;
}


namespace
{

struct ZONE_OVERLAP_PAIR
{
    ZONE* zoneA;
    ZONE* zoneB;
    LSET  sharedLayers;
};


struct ZONE_PRIORITY_EDGE
{
    ZONE* higher;
    ZONE* lower;
    int   countDiff;
    bool  fromArea;
};

} // namespace


static std::vector<ZONE_OVERLAP_PAIR> findOverlappingPairs( BOARD* aBoard )
{
    std::vector<ZONE_OVERLAP_PAIR> pairs;
    const ZONES&                   zones = aBoard->Zones();

    for( size_t i = 0; i < zones.size(); i++ )
    {
        ZONE* a = zones[i];

        if( a->GetIsRuleArea() || a->IsTeardropArea() || !a->IsOnCopperLayer() )
            continue;

        BOX2I bboxA = a->GetBoundingBox();

        for( size_t j = i + 1; j < zones.size(); j++ )
        {
            ZONE* b = zones[j];

            if( b->GetIsRuleArea() || b->IsTeardropArea() || !b->IsOnCopperLayer() )
                continue;

            LSET shared = a->GetLayerSet() & b->GetLayerSet();
            shared &= LSET::AllCuMask();

            if( shared.none() )
                continue;

            if( !b->GetBoundingBox().Intersects( bboxA ) )
                continue;

            bool overlaps = a->Outline()->Collide( b->Outline() )
                            || ( b->Outline()->TotalVertices() > 0
                                 && a->Outline()->Contains( b->Outline()->CVertex( 0 ) ) )
                            || ( a->Outline()->TotalVertices() > 0
                                 && b->Outline()->Contains( a->Outline()->CVertex( 0 ) ) );

            if( overlaps )
                pairs.push_back( { a, b, shared } );
        }
    }

    return pairs;
}


static std::optional<ZONE_PRIORITY_EDGE> computeConstraint( const ZONE_OVERLAP_PAIR& aPair,
                                                             BOARD* aBoard )
{
    SHAPE_POLY_SET polyA = aPair.zoneA->Outline()->CloneDropTriangulation();
    SHAPE_POLY_SET polyB = aPair.zoneB->Outline()->CloneDropTriangulation();
    polyA.ClearArcs();
    polyB.ClearArcs();

    SHAPE_POLY_SET intersection;
    intersection.BooleanIntersection( polyA, polyB );

    if( intersection.IsEmpty() )
        return std::nullopt;

    intersection.BuildBBoxCaches();

    int netCodeA = aPair.zoneA->GetNetCode();
    int netCodeB = aPair.zoneB->GetNetCode();

    // Same-net overlapping zones are cooperative, not competitive. Priority
    // between them is meaningless to the fill engine. Return no constraint
    // here; AutoAssignZonePriorities() groups them to the same priority level.
    if( netCodeA == netCodeB )
        return std::nullopt;

    int countA = 0;
    int countB = 0;

    auto countIfInOverlap = [&]( const VECTOR2I& aPos, int aNetCode, PCB_LAYER_ID aLayer )
    {
        if( !aPair.sharedLayers.test( aLayer ) )
            return;

        if( intersection.Contains( aPos ) )
        {
            if( aNetCode == netCodeA )
                countA++;
            else if( aNetCode == netCodeB )
                countB++;
        }
    };

    for( FOOTPRINT* fp : aBoard->Footprints() )
    {
        for( PAD* pad : fp->Pads() )
        {
            for( PCB_LAYER_ID layer : aPair.sharedLayers.Seq() )
            {
                if( pad->IsOnLayer( layer ) )
                {
                    countIfInOverlap( pad->GetPosition(), pad->GetNetCode(), layer );
                    break;
                }
            }
        }
    }

    for( PCB_TRACK* track : aBoard->Tracks() )
    {
        if( track->Type() != PCB_VIA_T )
            continue;

        PCB_VIA* via = static_cast<PCB_VIA*>( track );

        for( PCB_LAYER_ID layer : aPair.sharedLayers.Seq() )
        {
            if( via->IsOnLayer( layer ) )
            {
                countIfInOverlap( via->GetPosition(), via->GetNetCode(), layer );
                break;
            }
        }
    }

    if( countA == 0 && countB == 0 )
    {
        double areaA = aPair.zoneA->Outline()->Area();
        double areaB = aPair.zoneB->Outline()->Area();

        if( areaA == areaB )
            return std::nullopt;

        ZONE* higher = ( areaA < areaB ) ? aPair.zoneA : aPair.zoneB;
        ZONE* lower  = ( higher == aPair.zoneA ) ? aPair.zoneB : aPair.zoneA;
        return ZONE_PRIORITY_EDGE{ higher, lower, 0, true };
    }

    int    maxCount = std::max( countA, countB );
    int    diff = std::abs( countA - countB );
    double ratio = static_cast<double>( diff ) / maxCount;

    constexpr double SIMILARITY_THRESHOLD = 0.20;

    if( ratio < SIMILARITY_THRESHOLD )
    {
        double areaA = aPair.zoneA->Outline()->Area();
        double areaB = aPair.zoneB->Outline()->Area();

        if( areaA == areaB )
            return std::nullopt;

        ZONE* higher = ( areaA < areaB ) ? aPair.zoneA : aPair.zoneB;
        ZONE* lower  = ( higher == aPair.zoneA ) ? aPair.zoneB : aPair.zoneA;
        return ZONE_PRIORITY_EDGE{ higher, lower, diff, true };
    }

    ZONE* higher = ( countA > countB ) ? aPair.zoneA : aPair.zoneB;
    ZONE* lower  = ( higher == aPair.zoneA ) ? aPair.zoneB : aPair.zoneA;
    return ZONE_PRIORITY_EDGE{ higher, lower, diff, false };
}


static void assignPrioritiesFromGraph( const std::vector<ZONE_PRIORITY_EDGE>& aEdges,
                                       std::vector<ZONE*>&                    aAllZones )
{
    std::unordered_map<ZONE*, std::vector<ZONE*>> adj;
    std::unordered_map<ZONE*, int>                inDegree;
    std::unordered_set<ZONE*>                     inGraph;

    for( ZONE* z : aAllZones )
    {
        inDegree[z] = 0;
        inGraph.insert( z );
    }

    // Sort edges so area-based (weakest) come first, then by ascending countDiff
    std::vector<ZONE_PRIORITY_EDGE> sortedEdges = aEdges;

    std::sort( sortedEdges.begin(), sortedEdges.end(),
               []( const ZONE_PRIORITY_EDGE& a, const ZONE_PRIORITY_EDGE& b )
               {
                   if( a.fromArea != b.fromArea )
                       return a.fromArea;

                   return a.countDiff < b.countDiff;
               } );

    for( const ZONE_PRIORITY_EDGE& edge : sortedEdges )
    {
        adj[edge.higher].push_back( edge.lower );
        inDegree[edge.lower]++;
    }

    // Kahn's algorithm: sources (in-degree 0) have nothing constraining them to be lower,
    // so they are the highest-priority zones. Process them first.
    std::vector<ZONE*> queue;

    for( ZONE* z : aAllZones )
    {
        if( inDegree[z] == 0 )
            queue.push_back( z );
    }

    std::sort( queue.begin(), queue.end(),
               []( const ZONE* a, const ZONE* b )
               {
                   return a->GetAssignedPriority() < b->GetAssignedPriority();
               } );

    std::vector<ZONE*> topoOrder;
    topoOrder.reserve( aAllZones.size() );

    while( !queue.empty() )
    {
        ZONE* current = queue.front();
        queue.erase( queue.begin() );
        topoOrder.push_back( current );

        auto& neighbors = adj[current];

        std::sort( neighbors.begin(), neighbors.end(),
                   []( const ZONE* a, const ZONE* b )
                   {
                       return a->GetAssignedPriority() < b->GetAssignedPriority();
                   } );

        for( ZONE* neighbor : neighbors )
        {
            inDegree[neighbor]--;

            if( inDegree[neighbor] == 0 )
                queue.push_back( neighbor );
        }

        std::sort( queue.begin(), queue.end(),
                   []( const ZONE* a, const ZONE* b )
                   {
                       return a->GetAssignedPriority() < b->GetAssignedPriority();
                   } );
    }

    // Zones stuck in cycles get appended sorted by their current priority
    if( topoOrder.size() < aAllZones.size() )
    {
        std::unordered_set<ZONE*> ordered( topoOrder.begin(), topoOrder.end() );
        std::vector<ZONE*>        remaining;

        for( ZONE* z : aAllZones )
        {
            if( ordered.find( z ) == ordered.end() )
                remaining.push_back( z );
        }

        std::sort( remaining.begin(), remaining.end(),
                   []( const ZONE* a, const ZONE* b )
                   {
                       return a->GetAssignedPriority() < b->GetAssignedPriority();
                   } );

        for( ZONE* z : remaining )
            topoOrder.push_back( z );
    }

    // topoOrder[0] is the highest-priority zone (source node). Assign descending values.
    for( size_t i = 0; i < topoOrder.size(); i++ )
        topoOrder[i]->SetAssignedPriority( static_cast<unsigned>( topoOrder.size() - 1 - i ) );
}


static ZONE* ufFind( std::unordered_map<ZONE*, ZONE*>& aParent, ZONE* aZone )
{
    ZONE*& parent = aParent[aZone];

    if( parent != aZone )
        parent = ufFind( aParent, parent );

    return parent;
}


static void ufUnion( std::unordered_map<ZONE*, ZONE*>& aParent,
                     std::unordered_map<ZONE*, int>&    aRank,
                     ZONE* aA, ZONE* aB )
{
    ZONE* rootA = ufFind( aParent, aA );
    ZONE* rootB = ufFind( aParent, aB );

    if( rootA == rootB )
        return;

    if( aRank[rootA] < aRank[rootB] )
        std::swap( rootA, rootB );

    aParent[rootB] = rootA;

    if( aRank[rootA] == aRank[rootB] )
        aRank[rootA]++;
}


bool AutoAssignZonePriorities( BOARD* aBoard, PROGRESS_REPORTER* aReporter )
{
    std::vector<ZONE*> eligibleZones;

    for( ZONE* zone : aBoard->Zones() )
    {
        if( !zone->GetIsRuleArea() && !zone->IsTeardropArea() && zone->IsOnCopperLayer() )
            eligibleZones.push_back( zone );
    }

    if( eligibleZones.size() < 2 )
        return false;

    std::unordered_map<ZONE*, unsigned> originalPriorities;

    for( ZONE* z : eligibleZones )
        originalPriorities[z] = z->GetAssignedPriority();

    std::vector<ZONE_OVERLAP_PAIR> pairs = findOverlappingPairs( aBoard );

    if( pairs.empty() )
        return false;

    // Build equivalence classes for same-net overlapping zones. These zones
    // are cooperative and must share the same priority after assignment.
    std::unordered_map<ZONE*, ZONE*> ufParent;
    std::unordered_map<ZONE*, int>   ufRank;

    for( ZONE* z : eligibleZones )
    {
        ufParent[z] = z;
        ufRank[z] = 0;
    }

    for( const ZONE_OVERLAP_PAIR& pair : pairs )
    {
        if( pair.zoneA->GetNetCode() == pair.zoneB->GetNetCode() )
            ufUnion( ufParent, ufRank, pair.zoneA, pair.zoneB );
    }

    thread_pool&                                                tp = GetKiCadThreadPool();
    std::vector<std::future<std::optional<ZONE_PRIORITY_EDGE>>> futures;
    futures.reserve( pairs.size() );

    for( const ZONE_OVERLAP_PAIR& pair : pairs )
    {
        if( pair.zoneA->GetNetCode() == pair.zoneB->GetNetCode() )
            continue;

        futures.emplace_back( tp.submit_task(
                [&pair, aBoard]()
                {
                    return computeConstraint( pair, aBoard );
                } ) );
    }

    std::vector<ZONE_PRIORITY_EDGE> edges;

    for( auto& future : futures )
    {
        std::optional<ZONE_PRIORITY_EDGE> result = future.get();

        if( result.has_value() )
            edges.push_back( result.value() );
    }

    if( !edges.empty() )
        assignPrioritiesFromGraph( edges, eligibleZones );

    // Equalize priorities within each same-net equivalence class. Each group
    // gets the maximum priority of any member so ordering constraints from
    // different-net edges propagate to the whole group.
    std::unordered_map<ZONE*, unsigned> groupMax;

    for( ZONE* z : eligibleZones )
    {
        ZONE*    root = ufFind( ufParent, z );
        unsigned pri = z->GetAssignedPriority();
        auto&    maxPri = groupMax[root];

        if( pri > maxPri )
            maxPri = pri;
    }

    for( ZONE* z : eligibleZones )
    {
        ZONE* root = ufFind( ufParent, z );
        z->SetAssignedPriority( groupMax[root] );
    }

    for( ZONE* z : eligibleZones )
    {
        if( z->GetAssignedPriority() != originalPriorities[z] )
            return true;
    }

    return false;
}
