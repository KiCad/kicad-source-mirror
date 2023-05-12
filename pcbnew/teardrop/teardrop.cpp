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

#include "teardrop.h"
#include <geometry/convex_hull.h>
#include <geometry/shape_line_chain.h>
#include <convert_basic_shapes_to_polygon.h>
#include <bezier_curves.h>

#include <progress_reporter.h>

#include <wx/log.h>

// The first priority level of a teardrop area (arbitrary value)
#define MAGIC_TEARDROP_ZONE_ID 30000


TEARDROP_MANAGER::TEARDROP_MANAGER( BOARD* aBoard, PCB_EDIT_FRAME* aFrame, PROGRESS_REPORTER* aReporter )
{
    m_frame = aFrame;
    m_reporter = aReporter;
    m_board = aBoard;
    m_prmsList = m_board->GetDesignSettings().GetTeadropParamsList();
    m_tolerance = 0;
    m_toolManager = m_frame->GetToolManager();
}


// Build a zone teardrop
static ZONE_SETTINGS s_default_settings;      // Use zone default settings for teardrop

ZONE* TEARDROP_MANAGER::createTeardrop( TEARDROP_VARIANT aTeardropVariant,
                                        std::vector<VECTOR2I>& aPoints, PCB_TRACK* aTrack ) const
{
    ZONE* teardrop = new ZONE( m_board );

    // teardrop settings are the last zone settings used by a zone dialog.
    // override them by default.
    s_default_settings.ExportSetting( *teardrop, false );

    // Add zone properties (priority will be fixed later)
    teardrop->SetTeardropAreaType( aTeardropVariant == TD_TYPE_PADVIA ?
                                    TEARDROP_TYPE::TD_VIAPAD :
                                    TEARDROP_TYPE::TD_TRACKEND );
    teardrop->SetLayer( aTrack->GetLayer() );
    teardrop->SetNetCode( aTrack->GetNetCode() );
    teardrop->SetLocalClearance( 0 );
    teardrop->SetMinThickness( pcbIUScale.mmToIU( 0.0254 ) );  // The minimum zone thickness
    teardrop->SetPadConnection( ZONE_CONNECTION::FULL );
    teardrop->SetIsFilled( false );
    teardrop->SetZoneName( aTeardropVariant == TD_TYPE_PADVIA ?
                                MAGIC_TEARDROP_PADVIA_NAME :
                                MAGIC_TEARDROP_TRACK_NAME );
    teardrop->SetIslandRemovalMode( ISLAND_REMOVAL_MODE::NEVER );
    SHAPE_POLY_SET* outline = teardrop->Outline();
    outline->NewOutline();

    for( VECTOR2I pt: aPoints )
        outline->Append(pt.x, pt.y);

    // Used in priority calculations:
    teardrop->CalculateFilledArea();

    return teardrop;
}


int TEARDROP_MANAGER::SetTeardrops( BOARD_COMMIT* aCommitter, bool aFollowTracks, bool aFillAfter )
{
    // Init parameters:
    m_tolerance = pcbIUScale.mmToIU( 0.01 );

    int count = 0;      // Number of created teardrop

    // Old teardrops must be removed, to ensure a clean teardrop rebuild
    int removed_cnt = RemoveTeardrops( aCommitter, false );

    // get vias, PAD_ATTRIB_PTH and others if aIncludeNotDrilled == true
    // (custom pads are not collected)
    std::vector< VIAPAD > viapad_list;

    if( m_prmsList->m_TargetViasPads )
        collectVias( viapad_list );

    collectPadsCandidate( viapad_list, m_prmsList->m_TargetViasPads,
                          m_prmsList->m_UseRoundShapesOnly, m_prmsList->m_TargetPadsWithNoHole );

    TRACK_BUFFER trackLookupList;

    if( aFollowTracks )
    {
        // Build the track list (only straight lines)
        for( PCB_TRACK* track: m_board->Tracks() )
        {
            if( track->Type() == PCB_TRACE_T || track->Type() == PCB_ARC_T)
            {
                int netcode = track->GetNetCode();
                int layer = track->GetLayer();
                trackLookupList.AddTrack( track, layer, netcode );
            }
        }
    }


    std::vector< ZONE*> teardrops;
    collectTeardrops( teardrops );

    for( PCB_TRACK* track : m_board->Tracks() )
    {
        if( ! (track->Type() == PCB_TRACE_T || track->Type() == PCB_ARC_T ) )
            continue;

        // Search for a padvia connected to track, with one end inside and one end outside
        // if both track ends are inside or outside, one cannot build a teadrop
        for( VIAPAD& viapad: viapad_list )
        {
            // Pad and track must be on the same layer
            if( !viapad.IsOnLayer( track->GetLayer() ) )
                continue;

            bool start_in_pad = viapad.m_Parent->HitTest( track->GetStart() );
            bool end_in_pad = viapad.m_Parent->HitTest( track->GetEnd() );

            if( end_in_pad == start_in_pad )
                // the track is inside or outside the via pad. Cannot create a teardrop
                continue;

            // A pointer to one of available m_Parameters items
            TEARDROP_PARAMETERS* currParams;

            if( viapad.m_IsRound )
                currParams = m_prmsList->GetParameters( TARGET_ROUND );
            else
                currParams = m_prmsList->GetParameters( TARGET_RECT );

            // Ensure a teardrop shape can be built:
            // The track width must be < teardrop height
            if( track->GetWidth() >= currParams->m_TdMaxHeight
                || track->GetWidth() >= viapad.m_Width * currParams->m_HeightRatio )
            {
                continue;
            }

            // Ensure also it is not filtered by a too high track->GetWidth()/viapad.m_Width ratio
            if( track->GetWidth() >= viapad.m_Width * currParams->m_WidthtoSizeFilterRatio )
                continue;

            // Skip case where pad/via and the track is within a copper zone with the same net
            // (and the pad can be connected by the zone thermal relief )
            if( !m_prmsList->m_TdOnPadsInZones && isViaAndTrackInSameZone( viapad, track ) )
                continue;

            std::vector<VECTOR2I> points;
            bool success = computeTeardropPolygonPoints( currParams, points, track, viapad,
                                                         aFollowTracks, trackLookupList );

            if( success )
            {
                ZONE* new_teardrop = createTeardrop( TD_TYPE_PADVIA, points, track );
                m_board->Add( new_teardrop, ADD_MODE::BULK_INSERT );
                m_createdTdList.push_back( new_teardrop );

                if( aCommitter )
                    aCommitter->Added( new_teardrop );

                count += 1;
            }
        }
    }

    int track2trackCount = 0;

    if( m_prmsList->m_TargetTrack2Track )
        track2trackCount = addTeardropsOnTracks( aCommitter );

    // Now set priority of teardrops now all teardrops are added
    setTeardropPriorities();

    if( count || removed_cnt || track2trackCount )
    {
        // Note:
        // Refill zones can be made only with clean data, especially connectivity data,
        // therefore only after changes are pushed to avoid crashes in some cases
        //
        // Fill raw teardrop shapes, even if aFillAfter is true: it ensure teardrops
        // will be filled even when undoing refilling zones.
        // This is a rough calculation, just to show a filled
        // shape on screen without the (potentially large) performance hit of a zone refill
        int epsilon = pcbIUScale.mmToIU( 0.001 );
        int allowed_error = pcbIUScale.mmToIU( 0.005 );

        for( ZONE* zone: m_createdTdList )
        {
            int half_min_width = zone->GetMinThickness() / 2;
            int numSegs = GetArcToSegmentCount( half_min_width, allowed_error, FULL_CIRCLE );
            SHAPE_POLY_SET filledPolys = *zone->Outline();

            filledPolys.Deflate( half_min_width - epsilon, numSegs );

            // Re-inflate after pruning of areas that don't meet minimum-width criteria
            if( half_min_width - epsilon > epsilon )
                filledPolys.Inflate( half_min_width - epsilon, numSegs );

            zone->SetFilledPolysList( zone->GetFirstLayer(), filledPolys );
        }

        if( aCommitter )
            aCommitter->Push( _( "Add teardrops" ) );

        if( aFillAfter )
        {
            ZONE_FILLER filler( m_board, aCommitter );

            if( m_reporter )
                filler.SetProgressReporter( m_reporter );

            filler.Fill( m_board->Zones() );

            if( aCommitter )
                aCommitter->Push( _( "Add teardrops and refill zones" ) );
        }
    }

    return count + track2trackCount;
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


int TEARDROP_MANAGER::addTeardropsOnTracks( BOARD_COMMIT* aCommitter )
{
    TRACK_BUFFER trackLookupList;
    int count = 0;

    // Build the track list (only straight lines)
    for( PCB_TRACK* track: m_board->Tracks() )
    {
        if( track->Type() == PCB_TRACE_T || track->Type() == PCB_ARC_T )
        {
            int netcode = track->GetNetCode();
            int layer = track->GetLayer();
            trackLookupList.AddTrack( track, layer, netcode );
        }
    }

    // get vias and pads (custom pads are not collected). We do not create a track to track
    // teardrop inside a pad or via area
    std::vector< VIAPAD > viapad_list;
    collectVias( viapad_list );
    collectPadsCandidate( viapad_list, true, true, true );

    TEARDROP_PARAMETERS* currParams = m_prmsList->GetParameters( TARGET_TRACK );

    // Explore groups (a group is a set of tracks on the same layer and the same net):
    for( auto& grp : trackLookupList.GetBuffer() )
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
            int track_len = track->GetLength();
            min_width = track->GetWidth();

            // to avoid creating a teardrop between 2 tracks having similar widths
            // give a threshold
            const double th = currParams->m_WidthtoSizeFilterRatio > 0.1 ?
                                    1.0 / currParams->m_WidthtoSizeFilterRatio
                                    : 10.0;
            min_width = min_width * th;

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

                VECTOR2I roundshape_pos = candidate->GetStart();
                ENDPOINT_T endPointCandidate = ENDPOINT_START;
                match_points = track->IsPointOnEnds( roundshape_pos, m_tolerance );

                if( !match_points )
                {
                    roundshape_pos = candidate->GetEnd();
                    match_points = track->IsPointOnEnds( roundshape_pos, m_tolerance );
                    endPointCandidate = ENDPOINT_END;
                }

                // Ensure a pad or via is not on test_pos point before creating a teardrop
                // at this location
                for( VIAPAD& viapad : viapad_list )
                {
                    if( viapad.IsOnLayer( track->GetLayer() )
                        && viapad.m_Parent->HitTest( roundshape_pos, 0 ) )
                    {
                        match_points = 0;
                        break;
                    }
                }

                if( match_points )
                {
                    VIAPAD viatrack( candidate, endPointCandidate );
                    std::vector<VECTOR2I> points;
                    bool success = computeTeardropPolygonPoints( currParams,
                                                                 points, track, viatrack,
                                                                 false, trackLookupList );

                    if( success )
                    {
                        ZONE* new_teardrop = createTeardrop( TD_TYPE_TRACKEND, points, track );
                        m_board->Add( new_teardrop, ADD_MODE::BULK_INSERT );
                        m_createdTdList.push_back( new_teardrop );

                        if( aCommitter )
                            aCommitter->Added( new_teardrop );

                        count += 1;
                    }
                }
            }
        }
    }

    return count;
}


int TEARDROP_MANAGER::RemoveTeardrops( BOARD_COMMIT* aCommitter, bool aCommitAfterRemove )
{
    int count = 0;
    std::vector< ZONE*> teardrops;

    collectTeardrops( teardrops );

    for( ZONE* teardrop : teardrops )
    {
        m_board->Remove( teardrop, REMOVE_MODE::BULK );

        if( aCommitter )
            aCommitter->Removed( teardrop );

        count += 1;
    }

    if( count )
    {
        if( aCommitter && aCommitAfterRemove )
            aCommitter->Push( _( "Remove teardrops" ), SKIP_CONNECTIVITY );

        m_board->BuildConnectivity();
    }

    return count;
}

