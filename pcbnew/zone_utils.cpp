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

#include <zone.h>
#include <geometry/shape_poly_set.h>


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
