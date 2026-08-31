/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 Jean-Pierre Charras, jp.charras at wanadoo.fr
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

#include "teardrop/teardrop.h"

#include <confirm.h>

#include <board_design_settings.h>
#include <pcb_track.h>
#include <pad.h>
#include <zone_filler.h>
#include <board_commit.h>

#include <connectivity/connectivity_data.h>
#include <drc/drc_rtree.h>
#include <geometry/shape_line_chain.h>
#include <geometry/rtree.h>
#include <convert_basic_shapes_to_polygon.h>
#include <bezier_curves.h>

#include <algorithm>
#include <limits>
#include <unordered_map>
#include <unordered_set>

#include <wx/log.h>

// The first priority level of a teardrop area (arbitrary value)
#define MAGIC_TEARDROP_ZONE_ID 30000


TEARDROP_MANAGER::TEARDROP_MANAGER( BOARD* aBoard, TOOL_MANAGER* aToolManager ) :
        m_board( aBoard ),
        m_toolManager( aToolManager ),
        m_copperIndexed( false )
{
    m_prmsList = m_board->GetDesignSettings().GetTeadropParamsList();
    m_tolerance = 0;
}


KIID TEARDROP_MANAGER::teardropUuid( const PCB_TRACK* aTrack, const BOARD_ITEM* aCandidate,
                                     int aSlot )
{
    // Deriving the UUID from the pair keeps teardrop ordering stable across save/load.  A
    // crossing track yields two from one pair, so each takes a slot, and a slot spans two UUIDs.
    KIID uuid = KIID::Combine( aTrack->m_Uuid, aCandidate->m_Uuid );

    for( int ii = 0; ii < 2 * aSlot; ++ii )
        uuid.Increment();

    return uuid;
}


KIID TEARDROP_MANAGER::maskUuidFor( const KIID& aCopperUuid )
{
    KIID uuid = aCopperUuid;
    uuid.Increment();

    return uuid;
}


void TEARDROP_MANAGER::buildCrossingStub( PCB_TRACK& aStub, const PCB_TRACK* aTrack,
                                          const VECTOR2I& aEnd )
{
    // Copying aTrack would slice a PCB_ARC while the copy kept PCB_ARC_T, so the arc branches
    // would read a mid-point that is not there.
    aStub.SetLayer( aTrack->GetLayer() );
    aStub.SetWidth( aTrack->GetWidth() );
    aStub.SetNet( aTrack->GetNet() );
    aStub.SetHasSolderMask( aTrack->HasSolderMask() );
    aStub.SetLocalSolderMaskMargin( aTrack->GetLocalSolderMaskMargin() );
    aStub.SetEnd( aEnd );
}


ZONE* TEARDROP_MANAGER::createTeardrop( TEARDROP_VARIANT aTeardropVariant,
                                        std::vector<VECTOR2I>& aPoints, PCB_TRACK* aSourceTrack,
                                        const KIID& aUuid ) const
{
    ZONE* teardrop = new ZONE( m_board );

    teardrop->SetUuidDirect( aUuid );

    // Pristine rather than the board's, so nothing the user set up for a pour (hatch fill,
    // rule area, locking) leaks into a teardrop.
    ZONE_SETTINGS::GetDefaultSettings().ExportSetting( *teardrop, false );

    // Add zone properties (priority will be fixed later)
    teardrop->SetTeardropAreaType( aTeardropVariant == TD_TYPE_PADVIA ? TEARDROP_TYPE::TD_VIAPAD
                                                                      : TEARDROP_TYPE::TD_TRACKEND );
    teardrop->SetLayer( aSourceTrack->GetLayer() );
    teardrop->SetNetCode( aSourceTrack->GetNetCode() );
    teardrop->SetLocalClearance( 0 );
    teardrop->SetMinThickness( pcbIUScale.mmToIU( 0.0254 ) );  // The minimum zone thickness
    teardrop->SetPadConnection( ZONE_CONNECTION::FULL );
    teardrop->SetIsFilled( false );
    teardrop->SetIslandRemovalMode( ISLAND_REMOVAL_MODE::NEVER );
    teardrop->SetBorderDisplayStyle( ZONE_BORDER_DISPLAY_STYLE::INVISIBLE_BORDER, 0, false );

    SHAPE_POLY_SET* outline = teardrop->Outline();
    outline->NewOutline();

    for( const VECTOR2I& pt: aPoints )
        outline->Append( pt.x, pt.y );

    // Until we know better (ie: pay for a potentially very expensive zone refill), the teardrop
    // fill is the same as its outline.
    teardrop->SetFilledPolysList( aSourceTrack->GetLayer(), *teardrop->Outline() );
    teardrop->SetIsFilled( true );

    // Used in priority calculations:
    teardrop->CalculateFilledArea();

    return teardrop;
}


ZONE* TEARDROP_MANAGER::createTeardropMask( TEARDROP_VARIANT aTeardropVariant,
                                            std::vector<VECTOR2I>& aPoints,
                                            PCB_TRACK* aSourceTrack, const KIID& aUuid ) const
{
    ZONE* teardrop = new ZONE( m_board );

    // The second UUID of the slot, so the mask differs from the copper it covers.
    teardrop->SetUuidDirect( maskUuidFor( aUuid ) );

    // As for the copper teardrop.  The ZONE constructor imports the board's zone defaults,
    // which follow the last pour the user set up.
    ZONE_SETTINGS::GetDefaultSettings().ExportSetting( *teardrop, false );

    teardrop->SetTeardropAreaType( aTeardropVariant == TD_TYPE_PADVIA ? TEARDROP_TYPE::TD_VIAPAD
                                                                      : TEARDROP_TYPE::TD_TRACKEND );
    teardrop->SetLayer( aSourceTrack->GetLayer() == F_Cu ? F_Mask : B_Mask );
    teardrop->SetMinThickness( pcbIUScale.mmToIU( 0.0254 ) );  // The minimum zone thickness
    teardrop->SetIsFilled( false );
    teardrop->SetIslandRemovalMode( ISLAND_REMOVAL_MODE::NEVER );
    teardrop->SetBorderDisplayStyle( ZONE_BORDER_DISPLAY_STYLE::INVISIBLE_BORDER, 0, false );

    SHAPE_POLY_SET* outline = teardrop->Outline();
    outline->NewOutline();

    for( const VECTOR2I& pt: aPoints )
        outline->Append( pt.x, pt.y );

    if( int expansion = aSourceTrack->GetSolderMaskExpansion() )
    {
        // The zone-min-thickness deflate/reinflate is going to round corners, so it's more
        // efficient to allow acute corners on the solder mask expansion here, and delegate the
        // rounding to the deflate/reinflate.
        teardrop->SetMinThickness( std::max( teardrop->GetMinThickness(), expansion ) );

        outline->Inflate( expansion, CORNER_STRATEGY::ALLOW_ACUTE_CORNERS,
                          m_board->GetDesignSettings().m_MaxError );
    }

    // Until we know better (ie: pay for a potentially very expensive zone refill), the teardrop
    // fill is the same as its outline.
    teardrop->SetFilledPolysList( teardrop->GetLayer(), *teardrop->Outline() );
    teardrop->SetIsFilled( true );

    return teardrop;
}


void TEARDROP_MANAGER::createAndAddTeardropWithMask( BOARD_COMMIT& aCommit,
                                                     TEARDROP_VARIANT aTeardropVariant,
                                                     std::vector<VECTOR2I>& aPoints,
                                                     PCB_TRACK* aSourceTrack, const KIID& aUuid )
{
    ZONE* new_teardrop = createTeardrop( aTeardropVariant, aPoints, aSourceTrack, aUuid );
    m_board->Add( new_teardrop, ADD_MODE::BULK_INSERT );
    m_createdTdList.push_back( new_teardrop );

    // The next teardrop has to see this one, or two of them flare into the same gap.
    ensureCopperIndex();
    m_copperRTree.Insert( new_teardrop, new_teardrop->GetLayer() );

    aCommit.Added( new_teardrop );

    if( aSourceTrack->HasSolderMask() && IsExternalCopperLayer( aSourceTrack->GetLayer() ) )
    {
        ZONE* new_teardrop_mask = createTeardropMask( aTeardropVariant, aPoints, aSourceTrack,
                                                      aUuid );
        m_board->Add( new_teardrop_mask, ADD_MODE::BULK_INSERT );
        aCommit.Added( new_teardrop_mask );
    }
}


bool TEARDROP_MANAGER::tryCreateTrackTeardrop( BOARD_COMMIT& aCommit,
                                               const TEARDROP_PARAMETERS& aParams,
                                               TEARDROP_MANAGER::TEARDROP_VARIANT aTeardropVariant,
                                               PCB_TRACK* aTrack, PCB_TRACK* aSourceTrack,
                                               BOARD_ITEM* aCandidate, const VECTOR2I& aPos,
                                               const KIID& aUuid )
{
    std::vector<VECTOR2I> points;

    if( computeFittedTeardropPolygon( aParams, points, aTrack, aSourceTrack, aCandidate, aPos ) )
    {
        createAndAddTeardropWithMask( aCommit, aTeardropVariant, points, aSourceTrack, aUuid );
        return true;
    }

    return false;
}


void TEARDROP_MANAGER::RemoveTeardrops( BOARD_COMMIT& aCommit,
                                        std::vector<BOARD_ITEM*>* dirtyPadsAndVias,
                                        std::set<PCB_TRACK*>* dirtyTracks,
                                        const std::vector<BOARD_ITEM*>* dirtyCopper )
{
    std::shared_ptr<CONNECTIVITY_DATA> connectivity = m_board->GetConnectivity();

    struct TEARDROP_ANCHORS
    {
        std::vector<PAD*>       pads;
        std::vector<PCB_VIA*>   vias;
        std::vector<PCB_TRACK*> tracks;
    };

    std::vector<ZONE*>                masks;
    std::vector<ZONE*>                copperTeardrops;
    std::map<ZONE*, TEARDROP_ANCHORS> anchors;

    for( ZONE* zone : m_board->Zones() )
    {
        if( !zone->IsTeardropArea() )
            continue;

        // Connectivity knows nothing of a mask layer, so a mask teardrop is never stale on its
        // own.  It goes when the copper it covers goes.
        if( !zone->IsOnCopperLayer() )
        {
            masks.push_back( zone );
            continue;
        }

        copperTeardrops.push_back( zone );

        TEARDROP_ANCHORS& zoneAnchors = anchors[zone];

        connectivity->GetConnectedPadsAndVias( zone, &zoneAnchors.pads, &zoneAnchors.vias );
        zoneAnchors.tracks = connectivity->GetConnectedTracks( zone );
    }

    // A footprint move pushes every copper descendant, and the test below runs against the whole
    // list once per teardrop.  PCB_ARC rebuilds a SHAPE_ARC every time it is asked for its box.
    struct DIRTY_COPPER
    {
        BOX2I bbox;
        int   netcode;
    };

    std::map<PCB_LAYER_ID, std::vector<DIRTY_COPPER>> dirtyCopperByLayer;

    if( dirtyCopper )
    {
        for( BOARD_ITEM* item : *dirtyCopper )
        {
            DIRTY_COPPER entry = { item->GetBoundingBox(), copperNetcode( item ) };

            for( PCB_LAYER_ID layer : item->GetLayerSet().CuStack() )
                dirtyCopperByLayer[layer].push_back( entry );
        }
    }

    int maxClearance = m_board->GetMaxClearanceValue();

    // A width fitted to the neighbours goes stale when one of them moves, though the teardrop
    // anchors on neither.  Pre- and post-edit geometry both count, so moving away counts too.
    auto foreignNeighbourMoved =
            [&]( ZONE* zone ) -> bool
            {
                PCB_LAYER_ID layer = zone->GetFirstLayer();

                auto it = dirtyCopperByLayer.find( layer );

                if( it == dirtyCopperByLayer.end() )
                    return false;

                // The fit resolves clearance per pair, so no one number bounds the neighbourhood.
                // Take the widest anything can demand; over-retiring only costs a rebuild.
                BOX2I reach = zone->GetBoundingBox();

                reach.Inflate( maxClearance );

                for( const DIRTY_COPPER& item : it->second )
                {
                    // Net 0 is "no net", not a net that every unassigned item shares.
                    if( zone->GetNetCode() > 0 && item.netcode == zone->GetNetCode() )
                        continue;

                    if( reach.Intersects( item.bbox ) )
                        return true;
                }

                return false;
            };

    std::unordered_set<BOARD_ITEM*> dirtyPadViaSet( dirtyPadsAndVias->begin(),
                                                    dirtyPadsAndVias->end() );

    auto isStale =
            [&]( const TEARDROP_ANCHORS& zoneAnchors )
            {
                auto anchorDirty = [&]( BOARD_ITEM* aItem )
                                   {
                                       return dirtyPadViaSet.count( aItem ) > 0;
                                   };

                return std::any_of( zoneAnchors.pads.begin(), zoneAnchors.pads.end(),
                                    anchorDirty )
                       || std::any_of( zoneAnchors.vias.begin(), zoneAnchors.vias.end(),
                                       anchorDirty )
                       || std::any_of( zoneAnchors.tracks.begin(), zoneAnchors.tracks.end(),
                                       [&]( PCB_TRACK* aTrack )
                                       {
                                           return dirtyTracks->contains( aTrack );
                                       } );
            };

    // Dirty the anchors first, or the rebuild passes these teardrops by and they are lost.
    // Doing it here also lets the staleness pass below see the lists UpdateTeardrops() will.
    for( ZONE* zone : copperTeardrops )
    {
        if( !foreignNeighbourMoved( zone ) )
            continue;

        const TEARDROP_ANCHORS& zoneAnchors = anchors[zone];

        for( PAD* pad : zoneAnchors.pads )
        {
            if( dirtyPadViaSet.insert( pad ).second )
                dirtyPadsAndVias->push_back( pad );
        }

        for( PCB_VIA* via : zoneAnchors.vias )
        {
            if( dirtyPadViaSet.insert( via ).second )
                dirtyPadsAndVias->push_back( via );
        }

        for( PCB_TRACK* track : zoneAnchors.tracks )
            dirtyTracks->insert( track );
    }

    std::map<PCB_LAYER_ID, std::vector<ZONE*>> survivingCopper;
    std::unordered_map<KIID, bool>             maskSurvives;

    for( ZONE* zone : copperTeardrops )
    {
        bool stale = isStale( anchors[zone] );

        // A slot spans both UUIDs, so the pairing is exact rather than guessed from geometry.
        maskSurvives[maskUuidFor( zone->m_Uuid )] = !stale;

        if( stale )
            zone->SetFlags( STRUCT_DELETED );
        else
            survivingCopper[zone->GetFirstLayer()].push_back( zone );
    }

    for( ZONE* mask : masks )
    {
        bool covers;

        if( auto it = maskSurvives.find( mask->m_Uuid ); it != maskSurvives.end() )
        {
            covers = it->second;
        }
        else
        {
            // A mask predating the UUID spacing pairs with nothing, so fall back to concentricity
            // (the expansion can be negative).  Erring towards a spare mask, not a lost opening.
            PCB_LAYER_ID copperLayer = mask->GetFirstLayer() == F_Mask ? F_Cu : B_Cu;
            BOX2I        maskBBox = mask->Outline()->BBox();

            covers = false;

            for( ZONE* copper : survivingCopper[copperLayer] )
            {
                BOX2I copperBBox = copper->GetBoundingBox();

                if( maskBBox.Contains( copperBBox.GetCenter() )
                    && copperBBox.Contains( maskBBox.GetCenter() ) )
                {
                    covers = true;
                    break;
                }
            }
        }

        if( !covers )
            mask->SetFlags( STRUCT_DELETED );
    }

    m_board->BulkRemoveStaleTeardrops( aCommit );
}


void TEARDROP_MANAGER::UpdateTeardrops( BOARD_COMMIT& aCommit,
                                        const std::vector<BOARD_ITEM*>* dirtyPadsAndVias,
                                        const std::set<PCB_TRACK*>* dirtyTracks,
                                        bool aForceFullUpdate )
{
    if( m_board->LegacyTeardrops() )
        return;

    // Init parameters:
    m_tolerance = pcbIUScale.mmToIU( 0.01 );

    // Old teardrops must be removed, to ensure a clean teardrop rebuild.  Before the caches are
    // built, or they index zones that are no longer on the board.
    if( aForceFullUpdate )
    {
        for( ZONE* zone : m_board->Zones() )
        {
            if( zone->IsTeardropArea() )
                zone->SetFlags( STRUCT_DELETED );
        }

        m_board->BulkRemoveStaleTeardrops( aCommit );
    }

    BuildTrackCaches();

    std::shared_ptr<CONNECTIVITY_DATA> connectivity = m_board->GetConnectivity();
    std::unordered_set<BOARD_ITEM*>    dirtyPadViaSet;

    if( dirtyPadsAndVias )
        dirtyPadViaSet.insert( dirtyPadsAndVias->begin(), dirtyPadsAndVias->end() );

    for( PCB_TRACK* track : m_board->Tracks() )
    {
        if( ! ( track->Type() == PCB_TRACE_T || track->Type() == PCB_ARC_T ) )
            continue;

        std::vector<PAD*>     connectedPads;
        std::vector<PCB_VIA*> connectedVias;

        connectivity->GetConnectedPadsAndVias( track, &connectedPads, &connectedVias );

        bool forceUpdate = aForceFullUpdate || dirtyTracks->contains( track );

        for( PAD* pad : connectedPads )
        {
            if( !forceUpdate && !dirtyPadViaSet.count( pad ) )
                continue;

            TEARDROP_PARAMETERS& tdParams = pad->GetTeardropParams();
            VECTOR2I padSize = pad->GetSize( track->GetLayer() );
            int annularWidth = std::min( padSize.x, padSize.y );

            if( !tdParams.m_Enabled )
                continue;

            // Ensure a teardrop shape can be built: track width must be < teardrop width and
            // filter width.  A max width of 0 means no limit, not "nothing fits".
            if( ( tdParams.m_TdMaxWidth > 0 && track->GetWidth() >= tdParams.m_TdMaxWidth )
                || track->GetWidth() >= annularWidth * tdParams.m_BestWidthRatio
                || track->GetWidth() >= annularWidth * tdParams.m_WidthtoSizeFilterRatio )
            {
                continue;
            }

            bool startHitsPad = pad->HitTest( track->GetStart(), 0, track->GetLayer() );
            bool endHitsPad = pad->HitTest( track->GetEnd(), 0, track->GetLayer() );

            // The track is entirely inside the pad; cannot create a teardrop
            if( startHitsPad && endHitsPad )
                continue;

            // Reject tangential grazes, but keep short radial entries.
            if( startHitsPad != endHitsPad
                && computeChordThroughShape( track, pad, track->GetLayer(),
                                             startHitsPad ? track->GetStart() : track->GetEnd() )
                           < track->GetWidth() )
            {
                continue;
            }

            // Skip case where pad and the track are within a copper zone with the same net
            // (and the pad can be connected to the zone)
            if( !tdParams.m_TdOnPadsInZones && areItemsInSameZone( pad, track ) )
                continue;

            // A track crossing the pad earns one teardrop per side, not one over the whole track.
            if( !startHitsPad && !endHitsPad && track->HitTest( pad->GetPosition() ) )
            {
                PCB_TRACK stub( m_board );

                buildCrossingStub( stub, track, pad->GetPosition() );

                stub.SetStart( track->GetEnd() );
                tryCreateTrackTeardrop( aCommit, tdParams, TEARDROP_MANAGER::TD_TYPE_PADVIA, &stub,
                                        track, pad, pad->GetPosition(),
                                        teardropUuid( track, pad, 0 ) );
                stub.SetStart( track->GetStart() );
                tryCreateTrackTeardrop( aCommit, tdParams, TEARDROP_MANAGER::TD_TYPE_PADVIA, &stub,
                                        track, pad, pad->GetPosition(),
                                        teardropUuid( track, pad, 1 ) );
            }
            else
            {
                tryCreateTrackTeardrop( aCommit, tdParams, TEARDROP_MANAGER::TD_TYPE_PADVIA, track,
                                        track, pad, pad->GetPosition(),
                                        teardropUuid( track, pad, 0 ) );
            }
        }

        for( PCB_VIA* via : connectedVias )
        {
            if( !forceUpdate && !dirtyPadViaSet.count( via ) )
                continue;

            TEARDROP_PARAMETERS tdParams = via->GetTeardropParams();
            int                 annularWidth = via->GetWidth( track->GetLayer() );

            if( !tdParams.m_Enabled )
                continue;

            // Ensure a teardrop shape can be built: track width must be < teardrop width and
            // filter width.  A max width of 0 means no limit, not "nothing fits".
            if( ( tdParams.m_TdMaxWidth > 0 && track->GetWidth() >= tdParams.m_TdMaxWidth )
                || track->GetWidth() >= annularWidth * tdParams.m_BestWidthRatio
                || track->GetWidth() >= annularWidth * tdParams.m_WidthtoSizeFilterRatio )
            {
                continue;
            }

            bool startHitsVia = via->HitTest( track->GetStart() );
            bool endHitsVia = via->HitTest( track->GetEnd() );

            // The track is entirely inside the via; cannot create a teardrop
            if( startHitsVia && endHitsVia )
                continue;

            // Reject tangential grazes, but keep short radial entries.
            if( startHitsVia != endHitsVia
                && computeChordThroughShape( track, via, track->GetLayer(),
                                             startHitsVia ? track->GetStart() : track->GetEnd() )
                           < track->GetWidth() )
            {
                continue;
            }

            // As for pads, a track that merely crosses the via earns a teardrop on each side.
            if( !startHitsVia && !endHitsVia && track->HitTest( via->GetPosition() ) )
            {
                PCB_TRACK stub( m_board );

                buildCrossingStub( stub, track, via->GetPosition() );

                stub.SetStart( track->GetEnd() );
                tryCreateTrackTeardrop( aCommit, tdParams, TEARDROP_MANAGER::TD_TYPE_PADVIA, &stub,
                                        track, via, via->GetPosition(),
                                        teardropUuid( track, via, 0 ) );
                stub.SetStart( track->GetStart() );
                tryCreateTrackTeardrop( aCommit, tdParams, TEARDROP_MANAGER::TD_TYPE_PADVIA, &stub,
                                        track, via, via->GetPosition(),
                                        teardropUuid( track, via, 1 ) );
            }
            else
            {
                tryCreateTrackTeardrop( aCommit, tdParams, TEARDROP_MANAGER::TD_TYPE_PADVIA, track,
                                        track, via, via->GetPosition(),
                                        teardropUuid( track, via, 0 ) );
            }
        }
    }

    if( ( aForceFullUpdate || !dirtyTracks->empty() )
        && m_prmsList->GetParameters( TARGET_TRACK )->m_Enabled )
    {
        AddTeardropsOnTracks( aCommit, dirtyTracks, aForceFullUpdate, false );
    }

    // Now set priority of teardrops now all teardrops are added
    setTeardropPriorities();
}


void TEARDROP_MANAGER::DeleteTrackToTrackTeardrops( BOARD_COMMIT& aCommit )
{
    for( ZONE* zone : m_board->Zones() )
    {
        if( zone->IsTeardropArea() && zone->GetTeardropAreaType() == TEARDROP_TYPE::TD_TRACKEND )
            zone->SetFlags( STRUCT_DELETED );
    }

    m_board->BulkRemoveStaleTeardrops( aCommit );
}


void TEARDROP_MANAGER::setTeardropPriorities()
{
    // Note: a teardrop area is on only one layer, so using GetFirstLayer() is OK
    // to know the zone layer of a teardrop

    unsigned priority_base = MAGIC_TEARDROP_ZONE_ID;

    // The sort function to sort by increasing copper layers. Group by layers.
    // For same layers sort by decreasing areas
    struct
    {
        bool operator()(ZONE* a, ZONE* b) const
            {
                if( a->GetFirstLayer() == b->GetFirstLayer() )
                {
                    if( a->GetOutlineArea() != b->GetOutlineArea() )
                        return a->GetOutlineArea() > b->GetOutlineArea();
                    return a->m_Uuid < b->m_Uuid;  // stable tiebreak
                }
                return a->GetFirstLayer() < b->GetFirstLayer();

            }
    } compareLess;

    for( ZONE* td: m_createdTdList )
        td->CalculateOutlineArea();

    std::sort( m_createdTdList.begin(), m_createdTdList.end(), compareLess );

    // Survivors of an incremental update keep their priorities, and equal-priority zones of
    // different nets do not clear each other, so hand out what the layer still has free.
    std::set<ZONE*>                   created( m_createdTdList.begin(), m_createdTdList.end() );
    std::map<int, std::set<unsigned>> taken;

    for( ZONE* zone : m_board->Zones() )
    {
        if( zone->IsTeardropArea() && !created.count( zone ) )
            taken[zone->GetFirstLayer()].insert( zone->GetAssignedPriority() );
    }

    int curr_layer = -1;

    for( ZONE* td: m_createdTdList )
    {
        if( td->GetFirstLayer() != curr_layer )
        {
            curr_layer = td->GetFirstLayer();
            priority_base = MAGIC_TEARDROP_ZONE_ID;
        }

        const std::set<unsigned>& layerTaken = taken[curr_layer];

        while( layerTaken.count( priority_base )
               && priority_base < std::numeric_limits<unsigned>::max() )
        {
            priority_base++;
        }

        td->SetAssignedPriority( priority_base );

        if( priority_base < std::numeric_limits<unsigned>::max() )
            priority_base++;
    }
}


void TEARDROP_MANAGER::AddTeardropsOnTracks( BOARD_COMMIT& aCommit,
                                             const std::set<PCB_TRACK*>* aTracks,
                                             bool aForceFullUpdate, bool aSetPriorities )
{
    std::shared_ptr<CONNECTIVITY_DATA> connectivity = m_board->GetConnectivity();
    TEARDROP_PARAMETERS                params = *m_prmsList->GetParameters( TARGET_TRACK );

    // Explore groups (a group is a set of tracks on the same layer and the same net):
    for( auto& grp : m_trackLookupList.GetBuffer() )
    {
        int layer, netcode;
        TRACK_BUFFER::GetNetcodeAndLayerFromIndex( grp.first, &layer, &netcode );

        std::vector<PCB_TRACK*>* sublist = &grp.second;

        if( sublist->size() <= 1 )  // We need at least 2 track segments
            continue;

        // The sort function to sort by increasing track widths
        struct
        {
            bool operator()(PCB_TRACK* a, PCB_TRACK* b) const
                { return a->GetWidth() < b->GetWidth(); }
        } compareLess;

        std::sort( sublist->begin(), sublist->end(), compareLess );
        int min_width = sublist->front()->GetWidth();
        int max_width = sublist->back()->GetWidth();

        // Skip groups having the same track thickness
        if( max_width == min_width )
            continue;

        for( unsigned ii = 0; ii < sublist->size()-1; ii++ )
        {
            PCB_TRACK* track = (*sublist)[ii];
            int        track_len = (int) track->GetLength();
            bool       track_needs_update = aForceFullUpdate || aTracks->contains( track );
            min_width = track->GetWidth();

            // to avoid creating a teardrop between 2 tracks having similar widths give a threshold
            params.m_WidthtoSizeFilterRatio = std::max( params.m_WidthtoSizeFilterRatio, 0.1 );
            const double th = 1.0 / params.m_WidthtoSizeFilterRatio;
            min_width = KiROUND( min_width * th );

            for( unsigned jj = ii+1; jj < sublist->size(); jj++ )
            {
                // Search candidates with thickness > curr thickness
                PCB_TRACK* candidate = (*sublist)[jj];

                if( min_width >= candidate->GetWidth() )
                    continue;

                // Cannot build a teardrop on a too short track segment.
                // The min len is > candidate radius
                if( track_len <= candidate->GetWidth() /2 )
                    continue;

                // Now test end to end connection:
                EDA_ITEM_FLAGS match_points;    // to return the end point EDA_ITEM_FLAGS:
                                                // 0, STARTPOINT, ENDPOINT

                VECTOR2I pos = candidate->GetStart();
                match_points = track->IsPointOnEnds( pos, m_tolerance );

                if( !match_points )
                {
                    pos = candidate->GetEnd();
                    match_points = track->IsPointOnEnds( pos, m_tolerance );
                }

                if( !match_points )
                    continue;

                // An untouched pair's teardrop was not removed as stale, so building another
                // would duplicate it, and its UUID, on every edit elsewhere on the board.
                if( !track_needs_update && !aTracks->contains( candidate ) )
                    continue;

                // Pads/vias have priority for teardrops; ensure there isn't one at our position
                bool                  existingPadOrVia = false;
                std::vector<PAD*>     connectedPads;
                std::vector<PCB_VIA*> connectedVias;

                connectivity->GetConnectedPadsAndVias( track, &connectedPads, &connectedVias );

                for( PAD* pad : connectedPads )
                {
                    if( pad->HitTest( pos ) )
                        existingPadOrVia = true;
                }

                for( PCB_VIA* via : connectedVias )
                {
                    if( via->HitTest( pos ) )
                        existingPadOrVia = true;
                }

                if( existingPadOrVia )
                    continue;

                tryCreateTrackTeardrop( aCommit, params, TEARDROP_MANAGER::TD_TYPE_TRACKEND, track,
                                        track, candidate, pos,
                                        teardropUuid( track, candidate, 0 ) );
            }
        }
    }

    // The global edit dialog calls this directly, and a teardrop left at the default priority is
    // outranked by every pour.  UpdateTeardrops() has more to add, so it numbers them itself.
    if( aSetPriorities )
        setTeardropPriorities();
}
