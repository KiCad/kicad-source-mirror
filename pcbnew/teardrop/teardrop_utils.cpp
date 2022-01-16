/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2021 KiCad Developers, see AUTHORS.txt for contributors.
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

/*
 * Some calculations (mainly computeCurvedForRoundShape) are derived from
 * https://github.com/NilujePerchut/kicad_scripts/tree/master/teardrops
 */

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

#include <wx/log.h>


void TRACK_BUFFER::AddTrack( PCB_TRACK* aTrack, int aLayer, int aNetcode )
{
    auto item = m_map_tracks.find( idxFromLayNet( aLayer, aNetcode ) );
    std::vector<PCB_TRACK*>* buffer;

    if( item == m_map_tracks.end() )
    {
        buffer = new std::vector<PCB_TRACK*>;
        m_map_tracks[idxFromLayNet( aLayer, aNetcode )] = buffer;
    }
    else
        buffer = (*item).second;

    buffer->push_back( aTrack );
}


VIAPAD::VIAPAD( PCB_VIA* aVia ) :
        m_Parent( aVia )
{
    m_Pos = aVia->GetPosition();
    m_Width = aVia->GetWidth();
    m_Drill = aVia->GetDrillValue();
    m_NetCode = aVia->GetNetCode();
    m_IsRound = true;
    m_IsPad = false;
}


VIAPAD::VIAPAD( PAD* aPad ) :
            m_Parent( aPad )
{
    m_Pos = aPad->GetPosition();
    m_Width = std::min( aPad->GetSize().x, aPad->GetSize().y );
    m_Drill = std::min( aPad->GetDrillSizeX(), aPad->GetDrillSizeY() );
    m_NetCode = aPad->GetNetCode();
    m_IsRound = aPad->GetShape() == PAD_SHAPE::CIRCLE ||
                ( aPad->GetShape() == PAD_SHAPE::OVAL
                  && aPad->GetSize().x == aPad->GetSize().y );
    m_IsPad = true;
}


VIAPAD::VIAPAD( PCB_TRACK* aTrack, ENDPOINT_T aEndPoint ) :
            m_Parent( aTrack )
{
    m_Pos = aEndPoint == ENDPOINT_START ? aTrack->GetStart() : aTrack->GetEnd();
    m_Width =aTrack->GetWidth();
    m_Drill = 0;
    m_NetCode = aTrack->GetNetCode();
    m_IsRound = true;
    m_IsPad = false;
}


void TEARDROP_MANAGER::collectVias( std::vector< VIAPAD >& aList ) const
{
    for( PCB_TRACK* item : m_board->Tracks() )
    {
        if( item->Type() != PCB_VIA_T )
            continue;

        aList.emplace_back( static_cast<PCB_VIA*>( item ) );
    }
}


void TEARDROP_MANAGER::collectPadsCandidate( std::vector< VIAPAD >& aList,
                                             bool aDrilledViaPad,
                                             bool aRoundShapesOnly,
                                             bool aIncludeNotDrilled ) const
{
    for( FOOTPRINT* fp : m_board->Footprints() )
    {
        for( PAD* pad : fp->Pads() )
        {
            if( !pad->IsOnCopperLayer() )
                continue;

            if( pad->GetNetCode() <= 0 )    // Not connected, so a teardrop cannot be attached
                continue;

            if( pad->GetShape() == PAD_SHAPE::CUSTOM )  // A teardrop shape cannot be built
                continue;

            if( aRoundShapesOnly )
            {
                bool round_shape = pad->GetShape() == PAD_SHAPE::CIRCLE
                                   || ( pad->GetShape() == PAD_SHAPE::OVAL
                                        && pad->GetSize().x == pad->GetSize().y );
                if( !round_shape )
                    continue;
            }

            bool has_hole = pad->GetDrillSizeX() > 0 && pad->GetDrillSizeY() > 0;

            if( has_hole && !aDrilledViaPad )
                continue;

            if( has_hole || aIncludeNotDrilled )
                aList.emplace_back( pad );
        }
    }
}


void TEARDROP_MANAGER::collectTeardrops( std::vector< ZONE* >& aList ) const
{
    for( ZONE* zone : m_board->Zones() )
    {
        if( zone->IsTeardropArea() )
        {
            aList.push_back( zone );
        }
    }
}


bool TEARDROP_MANAGER::isViaAndTrackInSameZone( VIAPAD& aViapad, PCB_TRACK* aTrack ) const
{
    for( ZONE* zone: m_board->Zones() )
    {
        // Skip teardrops
        if( zone->IsTeardropArea() )
            continue;

        // Only consider zones on the same layer
        if( !zone->IsOnLayer( aTrack->GetLayer() ) )
            continue;

        if( zone->GetNetCode() == aTrack->GetNetCode() )
        {
            if( zone->Outline()->Contains( VECTOR2I( aViapad.m_Pos ) ) )
            {
                // Ensure the pad (if it is a pad) can be connected by the zone
                if( aViapad.m_IsPad )
                {
                    PAD *pad = static_cast< PAD* >( aViapad.m_Parent );

                    if( zone->GetPadConnection() == ZONE_CONNECTION::NONE
                        || pad->GetZoneConnection() == ZONE_CONNECTION::NONE )
                        return false;
                }

                return true;
            }
        }
    }

    return false;
}


PCB_TRACK* TEARDROP_MANAGER::findTouchingTrack( EDA_ITEM_FLAGS& aMatchType, PCB_TRACK* aTrackRef,
                                                const VECTOR2I& aEndPoint,
                                                TRACK_BUFFER& aTrackLookupList ) const
{
    EDA_ITEM_FLAGS match = 0;           // to return the end point EDA_ITEM_FLAGS:
                                        // 0, STARTPOINT, ENDPOINT
    int matches = 0;                    // Count of candidates: only 1 is acceptable
    PCB_TRACK* candidate = nullptr;     // a reference to the track connected

    std::vector<PCB_TRACK*>* currlist = aTrackLookupList.GetTrackList( aTrackRef->GetLayer(),
                                                                   aTrackRef->GetNetCode() );

    for( PCB_TRACK* curr_track: *currlist )
    {
        if( curr_track == aTrackRef )
            continue;

        match = curr_track->IsPointOnEnds( aEndPoint, m_Parameters[TARGET_ROUND].m_tolerance);

        if( match )
        {
            // if faced with a Y junction, stop here
            matches++;

            if( matches > 1 )
            {
                aMatchType = 0;
                return nullptr;
            }

            aMatchType = match;
            candidate = curr_track;
        }
    }

    return candidate;
}



/**
 * @return a vector unit length from aVector
 */
static VECTOR2D NormalizeVector( VECTOR2I aVector )
{
    VECTOR2D vect( aVector );
    double norm = vect.EuclideanNorm();
    return vect / norm;
}


/*
 * Compute the curve part points for teardrops connected to a round shape
 * The Bezier curve control points are optimized for a round pad/via shape,
 * and do not give a good curve shape for other pad shapes
 */
void TEARDROP_MANAGER::computeCurvedForRoundShape( std::vector<VECTOR2I>& aPoly,
                                        int aTrackHalfWidth,
                                        VECTOR2D aTrackDir, VIAPAD& aViaPad,
                                        std::vector<VECTOR2I>& pts ) const
{
    // in pts:
    // A and B are points on the track ( pts[0] and  pts[1] )
    // C and E are points on the aViaPad ( pts[2] and  pts[4] )
    // D is the aViaPad centre ( pts[3] )
    double Vpercent = m_CurrParams->m_heightRatio;
    int td_height = aViaPad.m_Width * Vpercent;

    // First, calculate a aVpercent equivalent to the td_height clamped by aTdMaxHeight
    // We cannot use the initial aVpercent because it gives bad shape with points
    // on aViaPad calculated for a clamped aViaPad size
    if( m_CurrParams->m_tdMaxHeight > 0 && m_CurrParams->m_tdMaxHeight < td_height )
         Vpercent *= (double)m_CurrParams->m_tdMaxHeight / td_height;

    int radius = aViaPad.m_Width / 2;
    double minVpercent = double( aTrackHalfWidth ) / radius;
    double weaken = (Vpercent - minVpercent) / ( 1 - minVpercent ) / radius;

    double biasBC = 0.5 * SEG( pts[1], pts[2] ).Length();
    double biasAE = 0.5 * SEG( pts[4], pts[0] ).Length();

    VECTOR2I vecC = (VECTOR2I)pts[2] - aViaPad.m_Pos;
    VECTOR2I tangentC = VECTOR2I( pts[2].x - vecC.y * biasBC * weaken,
                                pts[2].y + vecC.x * biasBC * weaken );
    VECTOR2I vecE = (VECTOR2I)pts[4] - aViaPad.m_Pos;
    VECTOR2I tangentE = VECTOR2I( pts[4].x + vecE.y * biasAE * weaken,
                                pts[4].y - vecE.x * biasAE * weaken );

    VECTOR2I tangentB = VECTOR2I( pts[1].x - aTrackDir.x * biasBC, pts[1].y - aTrackDir.y * biasBC );
    VECTOR2I tangentA = VECTOR2I( pts[0].x - aTrackDir.x * biasAE, pts[0].y - aTrackDir.y * biasAE );

    std::vector<VECTOR2I> curve_pts;
    curve_pts.reserve( m_CurrParams->m_curveSegCount );
    BEZIER_POLY( pts[1], tangentB, tangentC, pts[2] ).GetPoly( curve_pts, 0,
                                                               m_CurrParams->m_curveSegCount );

    for( VECTOR2I& corner: curve_pts )
        aPoly.push_back( corner );

    aPoly.push_back( pts[3] );

    curve_pts.clear();
    BEZIER_POLY( pts[4], tangentE, tangentA, pts[0] ).GetPoly( curve_pts, 0,
                                                               m_CurrParams->m_curveSegCount );

    for( VECTOR2I& corner: curve_pts )
        aPoly.push_back( corner );
}


/*
 * Compute the curve part points for teardrops connected to a rectangular/polygonal shape
 * The Bezier curve control points are not optimized for a special shape
 */
void TEARDROP_MANAGER::computeCurvedForRectShape( std::vector<VECTOR2I>& aPoly, int aTdHeight,
                                        int aTrackHalfWidth, VIAPAD& aViaPad,
                                        std::vector<VECTOR2I>& aPts ) const
{
    // in aPts:
    // A and B are points on the track ( pts[0] and  pts[1] )
    // C and E are points on the aViaPad ( pts[2] and  pts[4] )
    // D is the aViaPad centre ( pts[3] )

    // side1 is( aPts[1], aPts[2] );  from track to via
    VECTOR2I side1( aPts[2] - aPts[1] );  // vector from track to via
    // side2 is ( aPts[4], aPts[0] ); from via to track
    VECTOR2I side2( aPts[4] - aPts[0] );  // vector from track to via

    std::vector<VECTOR2I> curve_pts;
    curve_pts.reserve( m_CurrParams->m_curveSegCount );

    // Note: This side is from track to via
    VECTOR2I ctrl1 = ( aPts[1] + aPts[1] + aPts[2] ) / 3;
    VECTOR2I ctrl2 = ( aPts[1] + aPts[2] + aPts[2] ) / 3;

    // The control points must be moved toward the polygon inside, in order to give a curved shape
    // The move vector is perpendicular to the vertex (side 1 or side 2), and its
    // value is delta, depending on the sizes of via and track
    int delta = ( aTdHeight / 2 - aTrackHalfWidth );

    delta /= 4;     // A scaling factor giving a fine shape, defined from tests.
    // However for short sides, the value of delta must be reduced, depending
    // on the side lenght
    // We use here a max delta value = side_lenght/8, defined from tests

    int side_lenght = side1.EuclideanNorm();
    int delta_effective = std::min( delta, side_lenght/8 );
    // The move vector depend on the quadrant: it must be always defined to create a
    // curve with a direction toward the track
    double angle1 = side1.Angle();
    int sign = std::abs( angle1 ) >= 90.0*M_PI/180 ? 1 : -1;
    VECTOR2I bias( 0, sign * delta_effective );

    bias.Rotate( angle1 );

    ctrl1.x += bias.x;
    ctrl1.y += bias.y;
    ctrl2.x += bias.x;
    ctrl2.y += bias.y;

    BEZIER_POLY( aPts[1], ctrl1, ctrl2, aPts[2] ).GetPoly( curve_pts, 0,
                                                           m_CurrParams->m_curveSegCount );

    for( VECTOR2I& corner: curve_pts )
        aPoly.push_back( corner );

    aPoly.push_back( aPts[3] );

    // Note: This side is from via to track
    curve_pts.clear();
    ctrl1 = ( aPts[4] + aPts[4] + aPts[0] ) / 3;
    ctrl2 = ( aPts[4] + aPts[0] + aPts[0] ) / 3;

    side_lenght = side2.EuclideanNorm();
    delta_effective = std::min( delta, side_lenght/8 );

    double angle2 = side2.Angle();
    sign = std::abs( angle2 ) <= 90.0*M_PI/180 ? 1 : -1;

    bias = VECTOR2I( 0, sign * delta_effective );

    bias.Rotate( angle2 );

    ctrl1.x += bias.x;
    ctrl1.y += bias.y;
    ctrl2.x += bias.x;
    ctrl2.y += bias.y;

    BEZIER_POLY( aPts[4], ctrl1, ctrl2, aPts[0] ).GetPoly( curve_pts, 0,
                                                           m_CurrParams->m_curveSegCount );

    for( VECTOR2I& corner: curve_pts )
        aPoly.push_back( corner );
}


bool TEARDROP_MANAGER::ComputePointsOnPadVia( PCB_TRACK* aTrack,
                                              VIAPAD& aViaPad,
                                              std::vector<VECTOR2I>& aPts ) const
{
    // Compute the 2 anchor points on pad/via of the teardrop shape

    PAD* pad = dynamic_cast<PAD*>( aViaPad.m_Parent );
    SHAPE_POLY_SET c_buffer;

    // aHeightRatio is the factor to calculate the aViaPad teardrop prefered height
    // teardrop height = aViaPad size * aHeightRatio (aHeightRatio <= 1.0)
    // For rectangular (and similar) shapes, the preferred_height is calculated from the min
    // dim of the rectangle = aViaPad.m_Width

    int preferred_height = aViaPad.m_Width * m_CurrParams->m_heightRatio;

    // force_clip_shape = true to force the via/pad polygon to be clipped to follow
    // contraints
    // Clipping is also needed for rectangular shapes, because the teardrop shape is
    // restricted to a polygonal area smaller than the pad area (the teardrop height
    // use the smaller value of X and Y sizes).
    bool force_clip_shape = m_CurrParams->m_heightRatio < 1.0;

    // To find the anchor points on via/pad shape, we build the polygonal shape, and clip the polygon
    // to the max size (preferred_height or m_tdMaxHeight) by a rectangle centered on the
    // axis of the expected teardrop shape.
    // (only reduce the size of polygonal shape does not give good anchor points)
    if( aViaPad.m_IsRound )
    {
        TransformCircleToPolygon( c_buffer, aViaPad.m_Pos, aViaPad.m_Width/2 ,
                                  ARC_LOW_DEF, ERROR_INSIDE, 16 );
    }
    else
    {
        wxASSERT( pad );
        force_clip_shape = true;

        preferred_height = aViaPad.m_Width * m_CurrParams->m_heightRatio;
        pad->TransformShapeWithClearanceToPolygon( c_buffer, aTrack->GetLayer(), 0,
                                                   ARC_LOW_DEF, ERROR_INSIDE );
    }

    // Clip the pad/via shape to match the m_tdMaxHeight constraint, and for
    // not rounded pad, clip the shape at the aViaPad.m_Width, i.e. the value
    // of the smallest value between size.x and size.y values.
    if( force_clip_shape || ( m_CurrParams->m_tdMaxHeight > 0
        && m_CurrParams->m_tdMaxHeight < preferred_height ) )
    {
        int halfsize = std::min( m_CurrParams->m_tdMaxHeight, preferred_height )/2;

        // teardrop_axis is the line from anchor point on the track and the end point
        // of the teardrop in the pad/via
        // this is the teardrop_axis of the teardrop shape to build
        VECTOR2I ref_on_track = ( aPts[0] + aPts[1] ) / 2;
        VECTOR2I teardrop_axis( aPts[3] - ref_on_track );

        double orient = teardrop_axis.Angle();
        teardrop_axis.Rotate( -orient );
        int len = teardrop_axis.EuclideanNorm();

        // Build the constraint polygon: a rectangle with
        // lenght = dist between the point on track and the pad/via pos
        // height = m_tdMaxHeight or aViaPad.m_Width
        SHAPE_POLY_SET clipping_rect;
        clipping_rect.NewOutline();

        // Build a horizontal rect: it will be rotated later
        clipping_rect.Append( 0, - halfsize );
        clipping_rect.Append( 0, halfsize );
        clipping_rect.Append( len, halfsize );
        clipping_rect.Append( len, - halfsize );

        clipping_rect.Rotate( orient );
        clipping_rect.Move( ref_on_track );

        // Clip the shape to the max allowed teadrop area
        c_buffer.BooleanIntersection( clipping_rect, SHAPE_POLY_SET::PM_FAST );
    }

    /* in aPts:
     * A and B are points on the track ( aPts[0] and  aPts[1] )
     * C and E are points on the aViaPad ( aPts[2] and  aPts[4] )
     * D is midpoint behind the aViaPad centre ( aPts[3] )
     */

    SHAPE_LINE_CHAIN& padpoly = c_buffer.Outline(0);
    std::vector<VECTOR2I> points = padpoly.CPoints();

    std::vector<VECTOR2I> initialPoints;
    initialPoints.push_back( aPts[0] );
    initialPoints.push_back( aPts[1] );

    for( const VECTOR2I& pt: points )
        initialPoints.emplace_back( pt.x, pt.y );

    std::vector<VECTOR2I> hull;
    BuildConvexHull( hull, initialPoints );

    // Search for end points of segments starting at aPts[0] or aPts[1]
    // In some cases, in convex hull, only one point (aPts[0] or aPts[1]) is still in list
    VECTOR2I PointC;
    VECTOR2I PointE;
    int found_start = -1;      // 2 points (one start and one end) should be found
    int found_end   = -1;

    VECTOR2I start = aPts[0];
    VECTOR2I pend  = aPts[1];

    for( unsigned ii = 0, jj = 0; jj < hull.size(); ii++, jj++ )
    {
        unsigned next = ii+ 1;

        if( next >= hull.size() )
            next = 0;

        int prev = ii -1;

        if( prev < 0 )
            prev = hull.size()-1;

        if( hull[ii] == start )
        {
            // the previous or the next point is candidate:
            if( hull[next] != pend )
                PointE = hull[next];
            else
                PointE = hull[prev];

            found_start = ii;
        }

        if( hull[ii] == pend )
        {
            if( hull[next] != start )
                PointC = hull[next];
            else
                PointC = hull[prev];

            found_end = ii;
        }
    }

    if( found_start < 0 )   // PointE was not initalized, because start point does not exit
    {
        int ii = found_end-1;

        if( ii < 0 )
            ii = hull.size()-1;

        PointE = hull[ii];
    }

    if( found_end < 0 )   // PointC was not initalized, because end point does not exit
    {
        int ii = found_start-1;

        if( ii < 0 )
            ii = hull.size()-1;

        PointC = hull[ii];
    }

    aPts[2] = PointC;
    aPts[4] = PointE;

    // Now we have to know if the choice aPts[2] = PointC is the best, or if
    // aPts[2] = PointE is better.
    // A criteria is to calculate the polygon area in these 2 cases, and choose the case
    // that gives the bigger area, because the segments starting at PointC and PointE
    // maximize their distance.
    SHAPE_LINE_CHAIN dummy1( aPts, true );
    double area1 = dummy1.Area();

    std::swap( aPts[2], aPts[4] );
    SHAPE_LINE_CHAIN dummy2( aPts, true );
    double area2 = dummy2.Area();

    if( area1 > area2 )     // The first choice (without swapping) is the better.
        std::swap( aPts[2], aPts[4] );

    return true;
}


bool TEARDROP_MANAGER::findAnchorPointsOnTrack( VECTOR2I& aStartPoint, VECTOR2I& aEndPoint,
                                                PCB_TRACK*& aTrack, VIAPAD& aViaPad,
                                                int* aEffectiveTeardropLen,
                                                bool aFollowTracks,
                                                TRACK_BUFFER& aTrackLookupList ) const
{
    bool found = true;
    VECTOR2I start = aTrack->GetStart();
    VECTOR2I end = aTrack->GetEnd();
    int radius = aViaPad.m_Width / 2;

    // Requested length of the teardrop:
    int targetLength = aViaPad.m_Width * m_CurrParams->m_lenghtRatio;

    if( m_CurrParams->m_tdMaxLen > 0 )
        targetLength = std::min( m_CurrParams->m_tdMaxLen, targetLength );

    int actualTdLen;    // The actual teardrop lenght, limited by the available track lenght

    // ensure that start is at the via/pad end
    if( SEG( end, aViaPad.m_Pos ).Length() < radius )
    {
        std::swap( start, end );
    }

    SHAPE_POLY_SET shapebuffer;

    if( aViaPad.m_IsRound )
    {
        TransformCircleToPolygon( shapebuffer, aViaPad.m_Pos, radius,
                                  ARC_LOW_DEF, ERROR_INSIDE, 16 );
    }
    else
    {
        PAD* pad = static_cast<PAD*>( aViaPad.m_Parent );
        pad->TransformShapeWithClearanceToPolygon( shapebuffer, aTrack->GetLayer(), 0,
                                                   ARC_LOW_DEF, ERROR_INSIDE );
    }

    SHAPE_LINE_CHAIN& outline = shapebuffer.Outline(0);
    outline.SetClosed( true );

    // Search the intersection point between the pad/via shape and the current track
    // This this the starting point to define the teardrop lenght
    SHAPE_LINE_CHAIN::INTERSECTIONS pts;
    int pt_count = outline.Intersect( SEG( start, end ), pts );

    // Ensure a intersection point was found, otherwise we cannot built the teardrop
    // using this track
    if( pt_count < 1 )
        return false;

    VECTOR2I intersect = pts[0].p;
    start.x = intersect.x;
    start.y = intersect.y;
    actualTdLen = std::min( targetLength, SEG( start, end ).Length() );

    // If the first track is too short to allow a teardrop having the requested length
    // explore the connected track(s)
    if( actualTdLen < targetLength && aFollowTracks )
    {
        int consumed = 0;

        while( actualTdLen+consumed < targetLength )
        {
            EDA_ITEM_FLAGS matchType;

            PCB_TRACK* connected_track = findTouchingTrack( matchType, aTrack, end, aTrackLookupList );

            if( connected_track == nullptr )
                break;

            // TODO: stop if angle between old and new segment is > 45 deg to avoid bad shape
            consumed += actualTdLen;
            actualTdLen = std::min( targetLength-consumed, int( connected_track->GetLength() ) );
            aTrack = connected_track;
            end = connected_track->GetEnd();
            start = connected_track->GetStart();

            if( matchType != STARTPOINT )
                std::swap( start, end );

            // If we do not want to explore more than one connected track, stop search here
            break;
        }
    }

    aStartPoint = start;
    aEndPoint = end;
    *aEffectiveTeardropLen = actualTdLen;
    return found;
}


bool TEARDROP_MANAGER::computeTeardropPolygonPoints( std::vector<VECTOR2I>& aCorners,
                                                     PCB_TRACK* aTrack, VIAPAD& aViaPad,
                                                     bool aFollowTracks,
                                                     TRACK_BUFFER& aTrackLookupList ) const
{
    VECTOR2I start, end;     // Start and end points of the track anchor of the teardrop
                            // the start point is inside the teardrop shape
                            // the end point is outside.
    int track_stub_len;     // the dist between the start point and the anchor point
                            // on the track

    // Note: aTrack can be modified if the inital track is too short
    if( !findAnchorPointsOnTrack( start, end, aTrack, aViaPad, &track_stub_len,
                                  aFollowTracks, aTrackLookupList ) )
        return false;

    VECTOR2D vecT = NormalizeVector(end - start);

    // find the 2 points on the track, sharp end of the teardrop
    int track_halfwidth = aTrack->GetWidth() / 2;
    VECTOR2I pointB = start + VECTOR2I( vecT.x * track_stub_len + vecT.y * track_halfwidth,
                                      vecT.y * track_stub_len - vecT.x * track_halfwidth );
    VECTOR2I pointA = start + VECTOR2I( vecT.x * track_stub_len - vecT.y * track_halfwidth,
                                      vecT.y * track_stub_len + vecT.x * track_halfwidth );

    // To build a polygonal valid shape pointA and point B must be outside the pad
    // It can be inside with some pad shapes having very different X and X sizes
    if( !aViaPad.m_IsRound )
    {
        PAD* pad = static_cast<PAD*>( aViaPad.m_Parent );

        if( pad->HitTest( pointA ) )
            return false;

        if( pad->HitTest( pointB ) )
            return false;
    }

    // Introduce a last point to cover the via centre to ensure it is seen as connected
    VECTOR2I pointD = aViaPad.m_Pos;
    // add a small offset in order to have the aViaPad.m_Pos reference point inside
    // the teardrop area, just in case...
    int offset = Millimeter2iu( 0.001 );
    pointD += VECTOR2I( int( -vecT.x*offset), int(-vecT.y*offset) );

    VECTOR2I pointC, pointE;     // Point on PADVIA outlines
    std::vector<VECTOR2I> pts = {pointA, pointB, pointC, pointD, pointE};

    ComputePointsOnPadVia( aTrack, aViaPad, pts );

    if( !m_CurrParams->IsCurved() )
    {
        aCorners = pts;
        return true;
    }

    // See if we can use curved teardrop shape
    if( aViaPad.m_IsRound )
    {
        computeCurvedForRoundShape( aCorners, track_halfwidth, vecT, aViaPad, pts );
    }
    else
    {
        int td_height = aViaPad.m_Width * m_CurrParams->m_heightRatio;

        if( m_CurrParams->m_tdMaxHeight > 0 && m_CurrParams->m_tdMaxHeight < td_height )
            td_height = m_CurrParams->m_tdMaxHeight;

        computeCurvedForRectShape( aCorners, td_height, track_halfwidth,
                                   aViaPad, pts );
    }

    return true;
}
