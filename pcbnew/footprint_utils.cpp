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

#include "footprint_utils.h"

#include <algorithm>
#include <map>
#include <set>
#include <unordered_map>

#include <wx/intl.h>

#include <board.h>
#include <core/kicad_algo.h>
#include <core/mirror.h>
#include <drc/drc_item.h>
#include <footprint.h>
#include <geometry/shape_segment.h>
#include <item_realignment.h>
#include <pad.h>
#include <pad_utils.h>
#include <pcb_field.h>
#include <trigo.h>
#include <zone.h>


bool ComputeFootprintShift( const FOOTPRINT& aExisting, const FOOTPRINT& aNew, VECTOR2I& aShift,
                            EDA_ANGLE& aAngleShift )
{
    // First, we will collect a list of "useful" corresponding points in the two footprints.
    // To be useful, a point from a pad must have a unique pad number, as in the general case, we
    // won't know which of multiple same-named pads in one footprint correspond to which in the
    // other.
    //
    // Theoretically, we could use graphics items and other hints like pad shape matching
    // but these would be hard to unambiguously match in the general case and pad numbers should
    // cover 99% of cases.

    const auto getUniquelyNumberedPads =
            []( const FOOTPRINT& fp ) -> std::unordered_map<wxString, VECTOR2I>
            {
                std::unordered_map<wxString, VECTOR2I> result;
                std::unordered_set<wxString>           seenDuplicate;

                for( PAD* pad : fp.Pads() )
                {
                    const wxString& number = pad->GetNumber();

                    // Already seen a pad with this number, so not unique
                    // (and it won't be found in result)
                    if( seenDuplicate.find( number ) != seenDuplicate.end() )
                        continue;

                    // Already have a pad with this number, so not unique.
                    // Remove the previous entry from the result and mark this number as seen.
                    if( result.find( number ) != result.end() )
                    {
                        result.erase( number );
                        seenDuplicate.insert( number );
                        continue;
                    }

                    // Use the exact library-frame position. PAD::GetFPRelativePosition()
                    // applies the footprint transform in integer coordinates, which can
                    // cause rounding errors.
                    result[number] = pad->GetLibraryPosition();
                }

                return result;
            };

    std::unordered_map<wxString, VECTOR2I> existingPads = getUniquelyNumberedPads( aExisting );
    std::unordered_map<wxString, VECTOR2I> newPads = getUniquelyNumberedPads( aNew );

    // Pads of footprints on opposite sides are mirror images in the library frame, so they
    // cannot be aligned by a rotation and translation.  Mirror the new pads onto the existing
    // footprint's side to match. FOOTPRINT::Flip() always mirrors children about the
    // footprint's library origin, whatever the flip centre, so this mirror is about (0,0).
    if( aExisting.IsFlipped() != aNew.IsFlipped() )
    {
        for( auto& [number, pos] : newPads )
            MIRROR( pos.y, 0 );
    }

    std::vector<VECTOR2I> existingPoints;
    std::vector<VECTOR2I> newPoints;

    // The matching points are the ones with the same unique pad number in both footprints
    // If a pad is missing in the new footprint, it won't be included.
    for( const auto& [number, pos] : existingPads )
    {
        auto it = newPads.find( number );

        if( it == newPads.end() )
            continue;

        existingPoints.push_back( pos );
        newPoints.push_back( it->second );
    }

    ORTHO_ITEM_REALIGNER aligner;

    std::optional<ITEM_REALIGNER_BASE::TRANSFORM> transform =
            aligner.GetTransform( existingPoints, newPoints );

    if( !transform.has_value() )
    {
        // Failed to compute a transform - perhaps there were no useful pads
        aShift = VECTOR2I( 0, 0 );
        aAngleShift = ANGLE_0;
        return false;
    }

    aShift = GetRotated( transform->m_Translation, aExisting.GetOrientation() );
    aAngleShift = transform->m_Rotation;

    return true;
}


void FOOTPRINT_UTILS::CheckFootprintAttributes(
        const FOOTPRINT& aFootprint, const std::function<void( const wxString& )>& aErrorHandler )
{
    int likelyAttr = ( aFootprint.GetLikelyAttribute() & ( FP_SMD | FP_THROUGH_HOLE ) );
    int setAttr = ( aFootprint.GetAttributes() & ( FP_SMD | FP_THROUGH_HOLE ) );

    if( setAttr && likelyAttr && setAttr != likelyAttr )
    {
        wxString msg;

        switch( likelyAttr )
        {
        case FP_THROUGH_HOLE:
            msg.Printf( _( "(expected 'Through hole'; actual '%s')" ), aFootprint.GetTypeName() );
            break;
        case FP_SMD:
            msg.Printf( _( "(expected 'SMD'; actual '%s')" ), aFootprint.GetTypeName() );
            break;
        }

        if( aErrorHandler )
            aErrorHandler( msg );
    }
}


void FOOTPRINT_UTILS::CheckPads( const FOOTPRINT& aFootprint, const UNITS_PROVIDER& aUnitsProvider,
                                 const PAD_CHECK_HANDLER& aErrorHandler )
{
    if( aErrorHandler == nullptr )
        return;

    for( const PAD* pad : aFootprint.Pads() )
    {
        PAD_UTILS::CheckPad( *pad, aUnitsProvider, false,
                [&]( const PAD_UTILS::PAD_CHECK_RESULT& aResult )
                {
                    aErrorHandler( *pad, aResult );
                } );
    }
}


void FOOTPRINT_UTILS::CheckShortingPads(
        const FOOTPRINT& aFootprint,
        const std::function<void( const PAD*, const PAD*, int aErrorCode,
                                  const VECTOR2I& )>& aErrorHandler )
{
    std::unordered_map<PTR_PTR_CACHE_KEY, int> checkedPairs;

    for( PAD* pad : aFootprint.Pads() )
    {
        std::vector<PAD*> netTiePads = aFootprint.GetNetTiePads( pad );

        for( PAD* other : aFootprint.Pads() )
        {
            if( other == pad )
                continue;

            // store canonical order so we don't collide in both directions (a:b and b:a)
            PAD* a = pad;
            PAD* b = other;

            if( static_cast<void*>( a ) > static_cast<void*>( b ) )
                std::swap( a, b );

            if( checkedPairs.find( { a, b } ) == checkedPairs.end() )
            {
                checkedPairs[ { a, b } ] = 1;

                if( pad->HasDrilledHole() && other->HasDrilledHole() )
                {
                    VECTOR2I pos = pad->GetPosition();

                    if( pad->GetPosition() == other->GetPosition() )
                    {
                        aErrorHandler( pad, other, DRCE_DRILLED_HOLES_COLOCATED, pos );
                    }
                    else
                    {
                        std::shared_ptr<SHAPE_SEGMENT> holeA = pad->GetEffectiveHoleShape( UNDEFINED_LAYER,
                                                                                           HOLE_TO_HOLE_CONSTRAINT );
                        std::shared_ptr<SHAPE_SEGMENT> holeB = other->GetEffectiveHoleShape( UNDEFINED_LAYER,
                                                                                             HOLE_TO_HOLE_CONSTRAINT );

                        if( holeA->Collide( holeB->GetSeg(), 0 ) )
                            aErrorHandler( pad, other, DRCE_DRILLED_HOLES_TOO_CLOSE, pos );
                    }
                }

                if( pad->SameLogicalPadAs( other ) || alg::contains( netTiePads, other ) )
                    continue;

                if( !( ( pad->GetLayerSet() & other->GetLayerSet() ) & LSET::AllCuMask() ).any() )
                    continue;

                if( pad->GetBoundingBox().Intersects( other->GetBoundingBox() ) )
                {
                    VECTOR2I pos;

                    for( PCB_LAYER_ID l : pad->Padstack().RelevantShapeLayers( other->Padstack() ) )
                    {
                        SHAPE*   padShape = pad->GetEffectiveShape( l ).get();
                        SHAPE*   otherShape = other->GetEffectiveShape( l ).get();

                        if( padShape->Collide( otherShape, 0, nullptr, &pos ) )
                            aErrorHandler( pad, other, DRCE_SHORTING_ITEMS, pos );
                    }
                }
            }
        }
    }
}


void FOOTPRINT_UTILS::CheckNetTies(
        const FOOTPRINT& aFootprint,
        const std::function<void( const BOARD_ITEM*, const BOARD_ITEM*, const BOARD_ITEM*,
                                  const VECTOR2I& )>& aErrorHandler )
{
    // First build a map from pad numbers to allowed-shorting-group indexes.  This ends up being
    // something like O(3n), but it still beats O(n^2) for large numbers of pads.

    std::map<wxString, int> padNumberToGroupIdxMap = aFootprint.MapPadNumbersToNetTieGroups();

    // Now collect all the footprint items which are on copper layers

    std::vector<BOARD_ITEM*> copperItems;

    for( BOARD_ITEM* item : aFootprint.GraphicalItems() )
    {
        if( item->IsOnCopperLayer() )
            copperItems.push_back( item );

        item->RunOnChildren(
                [&]( BOARD_ITEM* descendent )
                {
                    if( descendent->IsOnCopperLayer() )
                        copperItems.push_back( descendent );
                },
                RECURSE_MODE::RECURSE );
    }

    for( ZONE* zone : aFootprint.Zones() )
    {
        if( !zone->GetIsRuleArea() && zone->IsOnCopperLayer() )
            copperItems.push_back( zone );
    }

    for( PCB_FIELD* field : aFootprint.GetFields() )
    {
        if( field->IsOnCopperLayer() )
            copperItems.push_back( field );
    }

    for( PCB_LAYER_ID layer : { F_Cu, In1_Cu, B_Cu } )
    {
        // Next, build a polygon-set for the copper on this layer.  We don't really care about
        // nets here, we just want to end up with a set of outlines describing the distinct
        // copper polygons of the footprint.

        SHAPE_POLY_SET                         copperOutlines;
        std::map<int, std::vector<const PAD*>> outlineIdxToPadsMap;

        for( BOARD_ITEM* item : copperItems )
        {
            if( item->IsOnLayer( layer ) )
            {
                item->TransformShapeToPolygon( copperOutlines, layer, 0, aFootprint.GetMaxError(),
                                               ERROR_OUTSIDE );
            }
        }

        copperOutlines.Simplify();

        // Index each pad to the outline in the set that it is part of.

        for( const PAD* pad : aFootprint.Pads() )
        {
            for( int ii = 0; ii < copperOutlines.OutlineCount(); ++ii )
            {
                if( pad->GetEffectiveShape( layer )->Collide( &copperOutlines.Outline( ii ), 0 ) )
                    outlineIdxToPadsMap[ ii ].emplace_back( pad );
            }
        }

        // Finally, ensure that each outline which contains multiple pads has all its pads
        // listed in an allowed-shorting group.

        for( const auto& [ outlineIdx, pads ] : outlineIdxToPadsMap )
        {
            if( pads.size() > 1 )
            {
                const PAD* firstPad = pads[0];
                int        firstGroupIdx = padNumberToGroupIdxMap[ firstPad->GetNumber() ];

                for( size_t ii = 1; ii < pads.size(); ++ii )
                {
                    const PAD* thisPad = pads[ii];
                    int        thisGroupIdx = padNumberToGroupIdxMap[ thisPad->GetNumber() ];

                    if( thisGroupIdx < 0 || thisGroupIdx != firstGroupIdx )
                    {
                        BOARD_ITEM* shortingItem = nullptr;
                        VECTOR2I    pos = ( firstPad->GetPosition() + thisPad->GetPosition() ) / 2;

                        pos = copperOutlines.Outline( outlineIdx ).NearestPoint( pos );

                        for( BOARD_ITEM* item : copperItems )
                        {
                            if( item->HitTest( pos, 1 ) )
                            {
                                shortingItem = item;
                                break;
                            }
                        }

                        if( shortingItem )
                            aErrorHandler( shortingItem, firstPad, thisPad, pos );
                        else
                            aErrorHandler( firstPad, thisPad, nullptr, pos );
                    }
                }
            }
        }
    }
}


void FOOTPRINT_UTILS::CheckNetTiePadGroups(
        const FOOTPRINT& aFootprint, const std::function<void( const wxString& )>& aErrorHandler )
{
    std::set<wxString> padNumbers;
    wxString           msg;

    for( const auto& [ padNumber, _ ] : aFootprint.MapPadNumbersToNetTieGroups() )
    {
        const PAD* pad = aFootprint.FindPadByNumber( padNumber );

        if( !pad )
        {
            msg.Printf( _( "(net-tie pad group contains unknown pad number %s)" ), padNumber );
            aErrorHandler( msg );
        }
        else if( !padNumbers.insert( pad->GetNumber() ).second )
        {
            msg.Printf( _( "(pad %s appears in more than one net-tie pad group)" ), padNumber );
            aErrorHandler( msg );
        }
    }
}


void FOOTPRINT_UTILS::CheckClippedSilk(
        const FOOTPRINT& aFootprint,
        const std::function<void( BOARD_ITEM*, BOARD_ITEM*, const VECTOR2I& )>& aErrorHandler )
{
    auto checkColliding =
            [&]( BOARD_ITEM* item, BOARD_ITEM* other )
            {
                for( PCB_LAYER_ID silk : { F_SilkS, B_SilkS } )
                {
                    PCB_LAYER_ID mask = silk == F_SilkS ? F_Mask : B_Mask;

                    if( !item->IsOnLayer( silk ) || !other->IsOnLayer( mask ) )
                        continue;

                    std::shared_ptr<SHAPE> itemShape = item->GetEffectiveShape( silk );
                    std::shared_ptr<SHAPE> otherShape = other->GetEffectiveShape( mask );
                    int                    actual;
                    VECTOR2I               pos;

                    if( itemShape->Collide( otherShape.get(), 0, &actual, &pos ) )
                        aErrorHandler( item, other, pos );
                }
            };

    for( BOARD_ITEM* item : aFootprint.GraphicalItems() )
    {
        for( BOARD_ITEM* other : aFootprint.GraphicalItems() )
        {
            if( other != item )
                checkColliding( item, other );
        }

        for( PAD* pad : aFootprint.Pads() )
            checkColliding( item, pad );
    }
}
