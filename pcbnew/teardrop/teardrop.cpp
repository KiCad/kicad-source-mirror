/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2023 KiCad Developers, see AUTHORS.txt for contributors.
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


#include <confirm.h>

#include <board_design_settings.h>
#include <pcb_track.h>
#include <pad.h>
#include <zone_filler.h>
#include <board_commit.h>

#include <connectivity/connectivity_data.h>
#include <teardrop/teardrop.h>
#include <drc/drc_rtree.h>
#include <geometry/shape_line_chain.h>
#include <geometry/rtree.h>
#include <convert_basic_shapes_to_polygon.h>
#include <bezier_curves.h>

#include <wx/log.h>

// The first priority level of a teardrop area (arbitrary value)
#define MAGIC_TEARDROP_ZONE_ID 30000


TEARDROP_MANAGER::TEARDROP_MANAGER( BOARD* aBoard, TOOL_MANAGER* aToolManager ) :
        m_board( aBoard ),
        m_toolManager( aToolManager )
{
    m_prmsList = m_board->GetDesignSettings().GetTeadropParamsList();
    m_tolerance = 0;
}


ZONE* TEARDROP_MANAGER::createTeardrop( TEARDROP_VARIANT aTeardropVariant,
                                        std::vector<VECTOR2I>& aPoints, PCB_TRACK* aTrack ) const
{
    ZONE* teardrop = new ZONE( m_board );

    // teardrop settings are the last zone settings used by a zone dialog.
    // override them by default.
    ZONE_SETTINGS::GetDefaultSettings().ExportSetting( *teardrop, false );

    // Add zone properties (priority will be fixed later)
    teardrop->SetTeardropAreaType( aTeardropVariant == TD_TYPE_PADVIA ? TEARDROP_TYPE::TD_VIAPAD
                                                                      : TEARDROP_TYPE::TD_TRACKEND );
    teardrop->SetLayer( aTrack->GetLayer() );
    teardrop->SetNetCode( aTrack->GetNetCode() );
    teardrop->SetLocalClearance( 0 );
    teardrop->SetMinThickness( pcbIUScale.mmToIU( 0.0254 ) );  // The minimum zone thickness
    teardrop->SetPadConnection( ZONE_CONNECTION::FULL );
    teardrop->SetIsFilled( false );
    teardrop->SetZoneName( aTeardropVariant == TD_TYPE_PADVIA ? MAGIC_TEARDROP_PADVIA_NAME
                                                              : MAGIC_TEARDROP_TRACK_NAME );
    teardrop->SetIslandRemovalMode( ISLAND_REMOVAL_MODE::NEVER );
    teardrop->SetBorderDisplayStyle( ZONE_BORDER_DISPLAY_STYLE::DIAGONAL_FULL,
                                     pcbIUScale.mmToIU( 0.1 ), true );

    SHAPE_POLY_SET* outline = teardrop->Outline();
    outline->NewOutline();

    for( const VECTOR2I& pt: aPoints )
        outline->Append( pt.x, pt.y );

    // Until we know better (ie: pay for a potentially very expensive zone refill), the teardrop
    // fill is the same as its outline.
    teardrop->SetFilledPolysList( aTrack->GetLayer(), *teardrop->Outline() );
    teardrop->SetIsFilled( true );

    // Used in priority calculations:
    teardrop->CalculateFilledArea();

    return teardrop;
}


void TEARDROP_MANAGER::RemoveTeardrops( BOARD_COMMIT& aCommit,
                                        const std::vector<BOARD_ITEM*>* dirtyPadsAndVias,
                                        const std::set<PCB_TRACK*>* dirtyTracks )
{
    std::shared_ptr<CONNECTIVITY_DATA> connectivity = m_board->GetConnectivity();
    std::vector<ZONE*>                 stale_teardrops;

    for( ZONE* zone : m_board->Zones() )
    {
        if( zone->IsTeardropArea() )
        {
            bool stale = false;

            std::vector<PAD*>     connectedPads;
            std::vector<PCB_VIA*> connectedVias;

            connectivity->GetConnectedPadsAndVias( zone, &connectedPads, &connectedVias );

            for( PAD* pad : connectedPads )
            {
                if( alg::contains( *dirtyPadsAndVias, pad ) )
                {
                    stale = true;
                    break;
                }
            }

            if( !stale )
            {
                for( PCB_VIA* via : connectedVias )
                {
                    if( alg::contains( *dirtyPadsAndVias, via ) )
                    {
                        stale = true;
                        break;
                    }
                }
            }

            if( stale )
                stale_teardrops.push_back( zone );
        }
    }

    for( ZONE* td : stale_teardrops )
    {
        m_board->Remove( td, REMOVE_MODE::BULK );
        aCommit.Removed( td );
    }
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

    buildTrackCaches();

    // Old teardrops must be removed, to ensure a clean teardrop rebuild
    if( aForceFullUpdate )
    {
        std::vector<ZONE*> teardrops;

        for( ZONE* zone : m_board->Zones() )
        {
            if( zone->IsTeardropArea() )
                teardrops.push_back( zone );
        }

        for( ZONE* td : teardrops )
        {
            m_board->Remove( td, REMOVE_MODE::BULK );
            aCommit.Removed( td );
        }
    }

    std::shared_ptr<CONNECTIVITY_DATA> connectivity = m_board->GetConnectivity();

    for( PCB_TRACK* track : m_board->Tracks() )
    {
        if( ! ( track->Type() == PCB_TRACE_T || track->Type() == PCB_ARC_T ) )
            continue;

        std::vector<PAD*>     connectedPads;
        std::vector<PCB_VIA*> connectedVias;

        connectivity->GetConnectedPadsAndVias( track, &connectedPads, &connectedVias );

        bool forceUpdate = aForceFullUpdate || dirtyTracks->find( track ) != dirtyTracks->end();

        for( PAD* pad : connectedPads )
        {
            if( !forceUpdate && !alg::contains( *dirtyPadsAndVias, pad ) )
                continue;

            if( pad->GetShape() == PAD_SHAPE::CUSTOM )
                // A teardrop shape cannot be built
                continue;

            TEARDROP_PARAMETERS& tdParams = pad->GetTeardropParams();
            int                  annularWidth = std::min( pad->GetSize().x, pad->GetSize().y );

            if( !tdParams.m_Enabled )
                continue;

            // Ensure a teardrop shape can be built: track width must be < teardrop width and
            // filter width
            if( track->GetWidth() >= tdParams.m_TdMaxWidth
                || track->GetWidth() >= annularWidth * tdParams.m_BestWidthRatio
                || track->GetWidth() >= annularWidth * tdParams.m_WidthtoSizeFilterRatio )
            {
                continue;
            }

            if( pad->HitTest( track->GetStart() ) && pad->HitTest( track->GetEnd() ) )
                // The track is entirely inside the pad; cannot create a teardrop
                continue;

            // Skip case where pad and the track are within a copper zone with the same net
            // (and the pad can be connected to the zone)
            if( !tdParams.m_TdOnPadsInZones && areItemsInSameZone( pad, track ) )
                continue;

            std::vector<VECTOR2I> points;

            if( computeTeardropPolygon( tdParams, points, track, pad, pad->GetPosition() ) )
            {
                ZONE* new_teardrop = createTeardrop( TD_TYPE_PADVIA, points, track );
                m_board->Add( new_teardrop, ADD_MODE::BULK_INSERT );
                m_createdTdList.push_back( new_teardrop );

                aCommit.Added( new_teardrop );
            }
        }

        for( PCB_VIA* via : connectedVias )
        {
            if( !forceUpdate && !alg::contains( *dirtyPadsAndVias, via ) )
                continue;

            TEARDROP_PARAMETERS tdParams = via->GetTeardropParams();
            int                 annularWidth = via->GetWidth();

            if( !tdParams.m_Enabled )
                continue;

            // Ensure a teardrop shape can be built: track width must be < teardrop width and
            // filter width
            if( track->GetWidth() >= tdParams.m_TdMaxWidth
                || track->GetWidth() >= annularWidth * tdParams.m_BestWidthRatio
                || track->GetWidth() >= annularWidth * tdParams.m_WidthtoSizeFilterRatio )
            {
                continue;
            }

            if( via->HitTest( track->GetStart() ) && via->HitTest( track->GetEnd() ) )
                // The track is entirely inside the via; cannot create a teardrop
                continue;

            std::vector<VECTOR2I> points;

            if( computeTeardropPolygon( tdParams, points, track, via, via->GetPosition() ) )
            {
                ZONE* new_teardrop = createTeardrop( TD_TYPE_PADVIA, points, track );
                m_board->Add( new_teardrop, ADD_MODE::BULK_INSERT );
                m_createdTdList.push_back( new_teardrop );

                aCommit.Added( new_teardrop );
            }
        }
    }

    if( ( aForceFullUpdate || !dirtyTracks->empty() )
        && m_prmsList->GetParameters( TARGET_TRACK )->m_Enabled )
    {
        AddTeardropsOnTracks( aCommit, dirtyTracks, aForceFullUpdate );
    }

    // Now set priority of teardrops now all teardrops are added
    setTeardropPriorities();
}


void TEARDROP_MANAGER::DeleteTrackToTrackTeardrops( BOARD_COMMIT& aCommit )
{
    std::vector<ZONE*> stale_teardrops;

    for( ZONE* zone : m_board->Zones() )
    {
        if( zone->IsTeardropArea() && zone->GetTeardropAreaType() == TEARDROP_TYPE::TD_TRACKEND )
            stale_teardrops.push_back( zone );
    }

    for( ZONE* td : stale_teardrops )
    {
        m_board->Remove( td, REMOVE_MODE::BULK );
        aCommit.Removed( td );
    }
}


void TEARDROP_MANAGER::setTeardropPriorities()
{
    // Note: a teardrop area is on only one layer, so using GetFirstLayer() is OK
    // to know the zone layer of a teardrop

    int priority_base = MAGIC_TEARDROP_ZONE_ID;

    // The sort function to sort by increasing copper layers. Group by layers.
    // For same layers sort by decreasing areas
    struct
    {
        bool operator()(ZONE* a, ZONE* b) const
            {
                if( a->GetFirstLayer() == b->GetFirstLayer() )
                    return a->GetOutlineArea() > b->GetOutlineArea();

                return a->GetFirstLayer() < b->GetFirstLayer();
            }
    } compareLess;

    for( ZONE* td: m_createdTdList )
        td->CalculateOutlineArea();

    std::sort( m_createdTdList.begin(), m_createdTdList.end(), compareLess );

    int curr_layer = -1;

    for( ZONE* td: m_createdTdList )
    {
        if( td->GetFirstLayer() != curr_layer )
        {
            curr_layer = td->GetFirstLayer();
            priority_base = MAGIC_TEARDROP_ZONE_ID;
        }

        td->SetAssignedPriority( priority_base++ );
    }
}


void TEARDROP_MANAGER::AddTeardropsOnTracks( BOARD_COMMIT& aCommit,
                                             const std::set<PCB_TRACK*>* aTracks,
                                             bool aForceFullUpdate )
{
    std::shared_ptr<CONNECTIVITY_DATA> connectivity = m_board->GetConnectivity();
    TEARDROP_PARAMETERS                params = *m_prmsList->GetParameters( TARGET_TRACK );

    // Explore groups (a group is a set of tracks on the same layer and the same net):
    for( auto& grp : m_trackLookupList.GetBuffer() )
    {
        int layer, netcode;
        TRACK_BUFFER::GetNetcodeAndLayerFromIndex( grp.first, &layer, &netcode );

        std::vector<PCB_TRACK*>* sublist = grp.second;

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
            bool       track_needs_update = aForceFullUpdate || alg::contains( *aTracks, track );
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

                if( !track_needs_update && alg::contains( *aTracks, candidate ) )
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

                std::vector<VECTOR2I> points;

                if( computeTeardropPolygon( params, points, track, candidate, pos ) )
                {
                    ZONE* new_teardrop = createTeardrop( TD_TYPE_TRACKEND, points, track );
                    m_board->Add( new_teardrop, ADD_MODE::BULK_INSERT );
                    m_createdTdList.push_back( new_teardrop );

                    aCommit.Added( new_teardrop );
                }
            }
        }
    }
}


