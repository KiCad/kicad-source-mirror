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

#include "fp_digest.h"

#include <board.h>
#include <diff_merge/library_digest.h>
#include <footprint.h>
#include <netinfo.h>
#include <pad.h>
#include <pcb_group.h>
#include <pcb_point.h>
#include <constraints/pcb_constraint.h>
#include <zone.h>
#include <pcb_io/kicad_sexpr/pcb_io_kicad_sexpr.h>

#include <fmt/format.h>

#include <algorithm>
#include <functional>
#include <map>
#include <string>
#include <utility>
#include <vector>


namespace KICAD_DIFF
{

namespace
{
    std::string textWithoutUuid( BOARD_ITEM* aItem )
    {
        const KIID id = aItem->m_Uuid;
        aItem->SetUuidDirect( niluuid );

        PCB_IO_KICAD_SEXPR writer( CTL_FOR_LIBRARY );
        writer.Format( aItem );
        aItem->SetUuidDirect( id );
        return writer.GetStringOutput( true );
    }


    // Any fixed order will do, and an item's own text is one that a fresh load's UUIDs cannot change
    template <typename ITEMS>
    void moveInTextOrder( FOOTPRINT& aFrom, const ITEMS& aItems, FOOTPRINT& aTo )
    {
        std::vector<std::pair<std::string, BOARD_ITEM*>> keyed;

        for( BOARD_ITEM* item : aItems )
            keyed.emplace_back( textWithoutUuid( item ), item );

        std::stable_sort( keyed.begin(), keyed.end(),
                          []( const auto& aA, const auto& aB )
                          {
                              return aA.first < aB.first;
                          } );

        for( const auto& [text, item] : keyed )
        {
            aFrom.Remove( item );
            aTo.Add( item, ADD_MODE::APPEND );
        }
    }


    // Items naming others share out the numbers they hold in the order of their text, a level at a time so
    // what each level names is settled before it is read
    void numberInTextOrder( const std::vector<std::vector<BOARD_ITEM*>>& aLevels )
    {
        std::vector<KIID> numbers;

        for( const std::vector<BOARD_ITEM*>& level : aLevels )
        {
            for( BOARD_ITEM* item : level )
                numbers.push_back( item->m_Uuid );
        }

        std::sort( numbers.begin(), numbers.end() );
        auto next = numbers.begin();

        for( const std::vector<BOARD_ITEM*>& level : aLevels )
        {
            std::vector<std::pair<std::string, BOARD_ITEM*>> keyed;

            for( BOARD_ITEM* item : level )
                keyed.emplace_back( textWithoutUuid( item ), item );

            std::stable_sort( keyed.begin(), keyed.end(),
                              []( const auto& aA, const auto& aB )
                              {
                                  return aA.first < aB.first;
                              } );

            for( const auto& [text, item] : keyed )
                item->SetUuidDirect( *next++ );
        }
    }


    // Groups by height, so a group comes after the groups it holds
    std::vector<std::vector<BOARD_ITEM*>> groupLevels( const FOOTPRINT& aPart )
    {
        std::map<const PCB_GROUP*, size_t>        heights;
        std::function<size_t( const PCB_GROUP* )> height;
        std::vector<std::vector<BOARD_ITEM*>>     levels;

        height = [&]( const PCB_GROUP* aGroup ) -> size_t
        {
            if( auto known = heights.find( aGroup ); known != heights.end() )
                return known->second;

            size_t tallest = 0;

            for( EDA_ITEM* member : aGroup->GetItems() )
            {
                if( member->Type() == PCB_GROUP_T )
                    tallest = std::max( tallest, height( static_cast<const PCB_GROUP*>( member ) ) + 1 );
            }

            return heights[aGroup] = tallest;
        };

        for( PCB_GROUP* group : aPart.Groups() )
        {
            const size_t level = height( group );

            if( levels.size() <= level )
                levels.resize( level + 1 );

            levels[level].push_back( group );
        }

        return levels;
    }


    std::string numberedText( FOOTPRINT& aPart )
    {
        int                  next = 0;
        std::map<KIID, KIID> renumbered;

        auto renumber =
                [&]( BOARD_ITEM* aItem )
                {
                    KIID id( wxString::Format( wxS( "00000000-0000-0000-0000-%012x" ), ++next ) );
                    renumbered[aItem->m_Uuid] = id;
                    aItem->SetUuidDirect( id );
                };

        renumber( &aPart );
        aPart.RunOnChildren( renumber, RECURSE_MODE::RECURSE );

        std::vector<BOARD_ITEM*> constraints;

        for( PCB_CONSTRAINT* constraint : aPart.Constraints() )
        {
            constraint->RemapKIIDs( renumbered );
            constraints.push_back( constraint );
        }

        // Numbered in container order, which the editor and a reopened file need not share
        numberInTextOrder( { constraints } );
        numberInTextOrder( groupLevels( aPart ) );

        PCB_IO_KICAD_SEXPR writer( CTL_FOR_LIBRARY | CTL_OMIT_INITIAL_COMMENTS | CTL_OMIT_FOOTPRINT_VERSION );
        writer.Format( &aPart );
        return writer.GetStringOutput( true );
    }


    wxString memberDigest( const FOOTPRINT& aFootprint )
    {
        FOOTPRINT part( aFootprint );
        part.SetParentGroup( nullptr );
        part.SetParent( nullptr );
        part.ClearAllNets();
        PCB_IO_KICAD_SEXPR::SyncEmbeddedFonts( &part );

        moveInTextOrder( part, part.GraphicalItems(), part );
        moveInTextOrder( part, part.Pads(), part );
        moveInTextOrder( part, part.Zones(), part );
        moveInTextOrder( part, part.Points(), part );

        return LibraryDigest( numberedText( part ) );
    }
} // namespace


wxString FootprintDigest( const FOOTPRINT& aFootprint, FOOTPRINT_DIGEST aScope )
{
    if( aScope == FOOTPRINT_DIGEST::MEMBER )
        return memberDigest( aFootprint );

    // A frame placed as aFootprint is, so copies of its items keep their place in it
    FOOTPRINT frame( nullptr );
    frame.SetLayer( aFootprint.GetLayer() );
    frame.SetTransformScale( aFootprint.GetScaleX(), aFootprint.GetScaleY() );
    frame.SetOrientation( aFootprint.GetOrientation() );
    frame.SetPosition( aFootprint.GetPosition() );

    auto adopt =
            [&]( BOARD_ITEM* aCopy )
            {
                // A pad flip evicts the board's clearance cache by UUID, so keep the copy off the original's
                aCopy->ResetUuidDirect();
                aCopy->SetParentGroup( nullptr );
                aCopy->SetLocked( false );
                frame.Add( aCopy, ADD_MODE::APPEND );

                if( BOARD_CONNECTED_ITEM* connected = dynamic_cast<BOARD_CONNECTED_ITEM*>( aCopy ) )
                    connected->SetNetCode( NETINFO_LIST::ORPHANED, true );
            };

    // Only what the library parity check compares; fields, text, 3D models and overrides are each placement's own
    for( BOARD_ITEM* item : aFootprint.GraphicalItems() )
    {
        if( item->Type() == PCB_SHAPE_T || item->Type() == PCB_BARCODE_T )
            adopt( static_cast<BOARD_ITEM*>( item->Clone() ) );
    }

    for( PAD* pad : aFootprint.Pads() )
    {
        PAD* copy = static_cast<PAD*>( pad->Clone() );
        copy->SetPinFunction( wxEmptyString );
        copy->SetPinType( wxEmptyString );
        copy->SetLocalClearance( std::nullopt );
        copy->SetLocalSolderMaskMargin( std::nullopt );
        copy->SetLocalSolderPasteMargin( std::nullopt );
        copy->SetLocalSolderPasteMarginRatio( std::nullopt );
        copy->Padstack().ForEachUniqueLayer(
                [&]( PCB_LAYER_ID aLayer )
                {
                    copy->SetLocalZoneConnection( aLayer, ZONE_CONNECTION::INHERITED );
                    copy->SetLocalThermalGapOverride( aLayer, std::nullopt );
                    copy->SetLocalThermalSpokeWidthOverride( aLayer, std::nullopt );
                } );

        adopt( copy );
    }

    for( ZONE* zone : aFootprint.Zones() )
    {
        ZONE* copy = static_cast<ZONE*>( zone->Clone() );
        copy->UnFill();
        adopt( copy );
    }

    // Unflip on the board, whose layer stack pairs inner copper and user layers with their opposites.
    // The frame is not in the board's index, so its children stay out of it too
    if( frame.IsFlipped() )
    {
        frame.SetParent( dynamic_cast<BOARD*>( aFootprint.GetParent() ) );
        frame.Flip( frame.GetPosition(), FLIP_DIRECTION::LEFT_RIGHT );
        frame.SetParent( nullptr );
    }

    frame.SetOrientation( ANGLE_0 );
    frame.SetPosition( VECTOR2I( 0, 0 ) );

    for( BOARD_ITEM* item : frame.GraphicalItems() )
        item->NormalizeForCompare();

    FOOTPRINT part( nullptr );
    part.SetAttributes( aFootprint.GetAttributes() & ( FP_THROUGH_HOLE | FP_SMD ) );
    part.SetAllowSolderMaskBridges( aFootprint.AllowSolderMaskBridges() );
    part.SetStackupMode( aFootprint.GetStackupMode() );

    if( aFootprint.GetStackupMode() == FOOTPRINT_STACKUP::CUSTOM_LAYERS )
        part.SetStackupLayers( aFootprint.GetStackupLayers() );

    for( const wxString& group : aFootprint.GetNetTiePadGroups() )
        part.AddNetTiePadGroup( group );

    moveInTextOrder( frame, frame.GraphicalItems(), part );
    moveInTextOrder( frame, frame.Pads(), part );
    moveInTextOrder( frame, frame.Zones(), part );

    std::string text = numberedText( part );

    // The library text omits the placement, yet DRC compares sizes as scaled
    if( aFootprint.GetScaleX() != 1.0 || aFootprint.GetScaleY() != 1.0 )
        text += fmt::format( "(scale {} {})", aFootprint.GetScaleX(), aFootprint.GetScaleY() );

    return LibraryDigest( text );
}

} // namespace KICAD_DIFF
