/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2015 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 1992-2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <trigo.h>
#include <macros.h>

#include <math/vector2d.h>
#include <pcb_shape.h>
#include <footprint.h>
#include <pad.h>
#include <base_units.h>
#include <convert_basic_shapes_to_polygon.h>
#include <geometry/shape_poly_set.h>
#include <geometry/geometry_utils.h>
#include <convert_drawsegment_list_to_polygon.h>

#include <wx/log.h>


/**
 * Flag to enable debug tracing for the board outline creation
 *
 * Use "KICAD_BOARD_OUTLINE" to enable.
 *
 * @ingroup trace_env_vars
 */
const wxChar* traceBoardOutline = wxT( "KICAD_BOARD_OUTLINE" );

/**
 * Function close_enough
 * is a local and tunable method of qualifying the proximity of two points.
 *
 * @param aLeft is the first point
 * @param aRight is the second point
 * @param aLimit is a measure of proximity that the caller knows about.
 * @return bool - true if the two points are close enough, else false.
 */
bool close_enough( VECTOR2I aLeft, VECTOR2I aRight, unsigned aLimit )
{
    return ( aLeft - aRight ).SquaredEuclideanNorm() <= SEG::Square( aLimit );
}

/**
 * Function closer_to_first
 * Local method which qualifies whether the start or end point of a segment is closest to a point.
 *
 * @param aRef is the reference point
 * @param aFirst is the first point
 * @param aSecond is the second point
 * @return bool - true if the first point is closest to the reference, otherwise false.
 */
bool closer_to_first( VECTOR2I aRef, VECTOR2I aFirst, VECTOR2I aSecond )
{
    return ( aRef - aFirst ).SquaredEuclideanNorm() < ( aRef - aSecond ).SquaredEuclideanNorm();
}


/**
 * Searches for a PCB_SHAPE matching a given end point or start point in a list.
 * @param aShape The starting shape.
 * @param aPoint The starting or ending point to search for.
 * @param aList The list to remove from.
 * @param aLimit is the distance from \a aPoint that still constitutes a valid find.
 * @return PCB_SHAPE* - The first PCB_SHAPE that has a start or end point matching
 *   aPoint, otherwise NULL if none.
 */
static PCB_SHAPE* findNext( PCB_SHAPE* aShape, const wxPoint& aPoint,
                            const std::vector<PCB_SHAPE*>& aList, unsigned aLimit )
{
    // Look for an unused, exact hit
    for( PCB_SHAPE* graphic : aList )
    {
        if( graphic == aShape || ( graphic->GetFlags() & SKIP_STRUCT ) != 0 )
            continue;

        switch( graphic->GetShape() )
        {
        case SHAPE_T::ARC:
            if( aPoint == graphic->GetArcStart() || aPoint == graphic->GetArcEnd() )
                return graphic;

            break;

        default:
            if( aPoint == graphic->GetStart() || aPoint == graphic->GetEnd() )
                return graphic;
        }
    }

    // Search again for anything that's close, even something already used.  (The latter is
    // important for error reporting.)
    VECTOR2I    pt( aPoint );
    SEG::ecoord closest_dist_sq = SEG::Square( aLimit );
    PCB_SHAPE*  closest_graphic = nullptr;
    SEG::ecoord d_sq;

    for( PCB_SHAPE* graphic : aList )
    {
        if( graphic == aShape )
            continue;

        switch( graphic->GetShape() )
        {
        case SHAPE_T::ARC:
            d_sq = ( pt - graphic->GetArcStart() ).SquaredEuclideanNorm();

            if( d_sq < closest_dist_sq )
            {
                closest_dist_sq = d_sq;
                closest_graphic = graphic;
            }

            d_sq = ( pt - graphic->GetArcEnd() ).SquaredEuclideanNorm();

            if( d_sq < closest_dist_sq )
            {
                closest_dist_sq = d_sq;
                closest_graphic = graphic;
            }
            break;

        default:
            d_sq = ( pt - graphic->GetStart() ).SquaredEuclideanNorm();

            if( d_sq < closest_dist_sq )
            {
                closest_dist_sq = d_sq;
                closest_graphic = graphic;
            }

            d_sq = ( pt - graphic->GetEnd() ).SquaredEuclideanNorm();

            if( d_sq < closest_dist_sq )
            {
                closest_dist_sq = d_sq;
                closest_graphic = graphic;
            }
        }
    }

    return closest_graphic;     // Note: will be nullptr if nothing within aLimit
}


/**
 * Function ConvertOutlineToPolygon
 * Build a polygon (with holes) from a PCB_SHAPE list, which is expected to be a closed main
 * outline with perhaps closed inner outlines.  These closed inner outlines are considered as
 * holes in the main outline.
 * @param aSegList the initial list of drawsegments (only lines, circles and arcs).
 * @param aPolygons will contain the complex polygon.
 * @param aErrorMax is the max error distance when polygonizing a curve (internal units)
 * @param aChainingEpsilon is the max error distance when polygonizing a curve (internal units)
 * @param aErrorHandler = an optional error handler
 */
bool ConvertOutlineToPolygon( std::vector<PCB_SHAPE*>& aSegList, SHAPE_POLY_SET& aPolygons,
                              int aErrorMax, int aChainingEpsilon,
                              OUTLINE_ERROR_HANDLER* aErrorHandler )
{
    if( aSegList.size() == 0 )
        return true;

    bool polygonComplete = false;
    bool selfIntersecting = false;

    wxString   msg;
    PCB_SHAPE* graphic = nullptr;

    std::set<PCB_SHAPE*> startCandidates( aSegList.begin(), aSegList.end() );

    // Find edge point with minimum x, this should be in the outer polygon
    // which will define the perimeter polygon polygon.
    wxPoint xmin    = wxPoint( INT_MAX, 0 );
    int     xmini   = 0;

    for( size_t i = 0; i < aSegList.size(); i++ )
    {
        graphic = (PCB_SHAPE*) aSegList[i];
        graphic->ClearFlags( SKIP_STRUCT );

        switch( graphic->GetShape() )
        {
        case SHAPE_T::RECT:
        case SHAPE_T::SEGMENT:
            {
                if( graphic->GetStart().x < xmin.x )
                {
                    xmin    = graphic->GetStart();
                    xmini   = i;
                }

                if( graphic->GetEnd().x < xmin.x )
                {
                    xmin    = graphic->GetEnd();
                    xmini   = i;
                }
            }
            break;

        case SHAPE_T::ARC:
            {
                wxPoint  pstart = graphic->GetArcStart();
                wxPoint  center = graphic->GetCenter();
                double   angle  = -graphic->GetAngle();
                double   radius = graphic->GetRadius();
                int      steps  = GetArcToSegmentCount( radius, aErrorMax, angle / 10.0 );
                wxPoint  pt;

                for( int step = 1; step<=steps; ++step )
                {
                    double rotation = ( angle * step ) / steps;

                    pt = pstart;

                    RotatePoint( &pt, center, rotation );

                    if( pt.x < xmin.x )
                    {
                        xmin  = pt;
                        xmini = i;
                    }
                }
            }
            break;

        case SHAPE_T::CIRCLE:
            {
                wxPoint pt = graphic->GetCenter();

                // pt has minimum x point
                pt.x -= graphic->GetRadius();

                // when the radius <= 0, this is a mal-formed circle. Skip it
                if( graphic->GetRadius() > 0 && pt.x < xmin.x )
                {
                    xmin  = pt;
                    xmini = i;
                }
            }
            break;

        case SHAPE_T::BEZIER:
            {
                graphic->RebuildBezierToSegmentsPointsList( graphic->GetWidth() );

                for( const wxPoint& pt : graphic->GetBezierPoints() )
                {
                    if( pt.x < xmin.x )
                    {
                        xmin  = pt;
                        xmini = i;
                    }
                }
            }
            break;

        case SHAPE_T::POLY:
            {
                const SHAPE_POLY_SET poly = graphic->GetPolyShape();
                double               orientation = 0.0;
                VECTOR2I             offset = VECTOR2I( 0, 0 );

                if( graphic->GetParentFootprint() )
                {
                    orientation = graphic->GetParentFootprint()->GetOrientation();
                    offset = graphic->GetParentFootprint()->GetPosition();
                }

                for( auto iter = poly.CIterate(); iter; iter++ )
                {
                    VECTOR2I pt = *iter;
                    RotatePoint( pt, orientation );
                    pt += offset;

                    if( pt.x < xmin.x )
                    {
                        xmin.x = pt.x;
                        xmin.y = pt.y;
                        xmini = i;
                    }
                }
            }
            break;

        default:
            break;
        }
    }

    // Keep a list of where the various segments came from so after doing our combined-polygon
    // tests we can still report errors against the individual graphic items.
    std::map<std::pair<VECTOR2I, VECTOR2I>, PCB_SHAPE*> segOwners;

    auto fetchOwner =
            [&]( const SEG& seg ) -> PCB_SHAPE*
            {
                auto it = segOwners.find( std::make_pair( seg.A, seg.B ) );
                return it == segOwners.end() ? nullptr : it->second;
            };

    // Grab the left most point, assume its on the board's perimeter, and see if we can put
    // enough graphics together by matching endpoints to formulate a cohesive polygon.

    PCB_SHAPE* prevGraphic = nullptr;
    wxPoint    prevPt;

    graphic = (PCB_SHAPE*) aSegList[xmini];
    graphic->SetFlags( SKIP_STRUCT );
    startCandidates.erase( graphic );

    // Output the outline perimeter as polygon.
    if( graphic->GetShape() == SHAPE_T::CIRCLE )
    {
        TransformCircleToPolygon( aPolygons, graphic->GetCenter(), graphic->GetRadius(),
                                  ARC_LOW_DEF, ERROR_INSIDE );
        polygonComplete = true;
    }
    else if( graphic->GetShape() == SHAPE_T::RECT )
    {
        std::vector<wxPoint> pts = graphic->GetRectCorners();

        aPolygons.NewOutline();

        for( const wxPoint& pt : pts )
            aPolygons.Append( pt );

        segOwners[ std::make_pair( pts[0], pts[1] ) ] = graphic;
        segOwners[ std::make_pair( pts[1], pts[2] ) ] = graphic;
        segOwners[ std::make_pair( pts[2], pts[3] ) ] = graphic;
        segOwners[ std::make_pair( pts[3], pts[0] ) ] = graphic;

        polygonComplete = true;
    }
    else if( graphic->GetShape() == SHAPE_T::POLY )
    {
        double   orientation = 0.0;
        VECTOR2I offset = VECTOR2I( 0, 0 );

        if( graphic->GetParentFootprint() )
        {
            orientation = graphic->GetParentFootprint()->GetOrientation();
            offset = graphic->GetParentFootprint()->GetPosition();
        }

        aPolygons.NewOutline();
        bool first = true;

        for( auto it = graphic->GetPolyShape().CIterate( 0 ); it; it++ )
        {
            VECTOR2I pt = *it;
            RotatePoint( pt, orientation );
            pt += offset;
            aPolygons.Append( pt );

            if( first )
                first = false;
            else
                segOwners[ std::make_pair( prevPt, pt ) ] = graphic;

            prevPt = (wxPoint) pt;
        }

        polygonComplete = true;
    }
    else
    {
        // Polygon start point. Arbitrarily chosen end of the
        // segment and build the poly from here.

        wxPoint startPt = graphic->GetShape() == SHAPE_T::ARC ? graphic->GetArcEnd()
                                                              : graphic->GetEnd();

        prevPt = startPt;
        aPolygons.NewOutline();
        aPolygons.Append( prevPt );

        // Do not append the other end point yet of this 'graphic', this first
        // 'graphic' might be an arc or a curve.

        for(;;)
        {
            switch( graphic->GetShape() )
            {
            case SHAPE_T::RECT:
            case SHAPE_T::CIRCLE:
            {
                // As a non-first item, closed shapes can't be anything but self-intersecting

                if( aErrorHandler )
                {
                    wxASSERT( prevGraphic );
                    (*aErrorHandler)( _( "(self-intersecting)" ), prevGraphic, graphic, prevPt );
                }

                selfIntersecting = true;

                // A closed shape will finish where it started, so no point in updating prevPt
            }
                break;

            case SHAPE_T::SEGMENT:
            {
                wxPoint  nextPt;

                // Use the line segment end point furthest away from prevPt as we assume
                // the other end to be ON prevPt or very close to it.

                if( closer_to_first( prevPt, graphic->GetStart(), graphic->GetEnd()) )
                    nextPt = graphic->GetEnd();
                else
                    nextPt = graphic->GetStart();

                aPolygons.Append( nextPt );
                segOwners[ std::make_pair( prevPt, nextPt ) ] = graphic;
                prevPt = nextPt;
            }
                break;

            case SHAPE_T::ARC:
            {
                // We do not support arcs in polygons, so approximate an arc with a series of
                // short lines and put those line segments into the !same! PATH.

                wxPoint pstart  = graphic->GetArcStart();
                wxPoint pend    = graphic->GetArcEnd();
                wxPoint pcenter = graphic->GetCenter();
                double  angle   = -graphic->GetAngle();
                double  radius  = graphic->GetRadius();
                int     steps   = GetArcToSegmentCount( radius, aErrorMax, angle / 10.0 );

                if( !close_enough( prevPt, pstart, aChainingEpsilon ) )
                {
                    wxASSERT( close_enough( prevPt, graphic->GetArcEnd(), aChainingEpsilon ) );

                    angle = -angle;
                    std::swap( pstart, pend );
                }

                // Create intermediate points between start and end:
                for( int step = 1; step < steps; ++step )
                {
                    double rotation = ( angle * step ) / steps;
                    wxPoint pt = pstart;
                    RotatePoint( &pt, pcenter, rotation );

                    aPolygons.Append( pt );
                    segOwners[ std::make_pair( prevPt, pt ) ] = graphic;
                    prevPt = pt;
                }

                // Append the last arc end point
                aPolygons.Append( pend );
                segOwners[ std::make_pair( prevPt, pend ) ] = graphic;
                prevPt = pend;
            }
                break;

            case SHAPE_T::BEZIER:
            {
                // We do not support Bezier curves in polygons, so approximate with a series
                // of short lines and put those line segments into the !same! PATH.

                wxPoint nextPt;
                bool    first = true;
                bool    reverse = false;

                // Use the end point furthest away from
                // prevPt as we assume the other end to be ON prevPt or
                // very close to it.

                if( closer_to_first( prevPt, graphic->GetStart(), graphic->GetEnd()) )
                {
                    nextPt = graphic->GetEnd();
                }
                else
                {
                    nextPt = graphic->GetStart();
                    reverse = true;
                }

                if( reverse )
                {
                    for( int jj = graphic->GetBezierPoints().size()-1; jj >= 0; jj-- )
                    {
                        const wxPoint& pt = graphic->GetBezierPoints()[jj];
                        aPolygons.Append( pt );

                        if( first )
                            first = false;
                        else
                            segOwners[ std::make_pair( prevPt, pt ) ] = graphic;

                        prevPt = pt;
                    }
                }
                else
                {
                    for( const wxPoint& pt : graphic->GetBezierPoints() )
                    {
                        aPolygons.Append( pt );

                        if( first )
                            first = false;
                        else
                            segOwners[ std::make_pair( prevPt, pt ) ] = graphic;

                        prevPt = pt;
                    }
                }

                prevPt = nextPt;
            }
                break;

            default:
                wxFAIL_MSG( "Unsupported PCB_SHAPE type "
                                + BOARD_ITEM::ShowShape( graphic->GetShape() ) );
                return false;
            }

            // Get next closest segment.

            PCB_SHAPE* nextGraphic = findNext( graphic, prevPt, aSegList, aChainingEpsilon );

            if( nextGraphic && !( nextGraphic->GetFlags() & SKIP_STRUCT ) )
            {
                prevGraphic = graphic;
                graphic = nextGraphic;
                graphic->SetFlags( SKIP_STRUCT );
                startCandidates.erase( graphic );
                continue;
            }

            // Finished, or ran into trouble...

            if( close_enough( startPt, prevPt, aChainingEpsilon ) )
            {
                polygonComplete = true;
                break;
            }
            else if( nextGraphic )  // encountered already-used segment, but not at the start
            {
                if( aErrorHandler )
                    (*aErrorHandler)( _( "(self-intersecting)" ), graphic, nextGraphic, prevPt );

                polygonComplete = false;
                break;
            }
            else                    // encountered discontinuity
            {
                if( aErrorHandler )
                    (*aErrorHandler)( _( "(not a closed shape)" ), graphic, nullptr, prevPt );

                polygonComplete = false;
                break;
            }
        }
    }

    int holeNum = -1;

    while( startCandidates.size() )
    {
        int  hole = aPolygons.NewHole();
        bool firstPt = true;
        holeNum++;

        graphic = (PCB_SHAPE*) *startCandidates.begin();
        graphic->SetFlags( SKIP_STRUCT );
        startCandidates.erase( startCandidates.begin() );

        // Both circles and polygons on the edge cuts layer are closed items that
        // do not connect to other elements, so we process them independently
        if( graphic->GetShape() == SHAPE_T::POLY )
        {
            double   orientation = 0.0;
            VECTOR2I offset = VECTOR2I( 0, 0 );

            if( graphic->GetParentFootprint() )
            {
                orientation = graphic->GetParentFootprint()->GetOrientation();
                offset = graphic->GetParentFootprint()->GetPosition();
            }

            for( auto it = graphic->GetPolyShape().CIterate(); it; it++ )
            {
                VECTOR2I pt = *it;
                RotatePoint( pt, orientation );
                pt += offset;

                aPolygons.Append( pt, -1, hole );

                if( firstPt )
                    firstPt = false;
                else
                    segOwners[ std::make_pair( prevPt, pt ) ] = graphic;

                prevPt = (wxPoint) pt;
            }
        }
        else if( graphic->GetShape() == SHAPE_T::CIRCLE )
        {
            // make a circle by segments;
            wxPoint  center  = graphic->GetCenter();
            double   angle   = 3600.0;
            wxPoint  start   = center;
            int      radius  = graphic->GetRadius();
            int      steps   = GetArcToSegmentCount( radius, aErrorMax, 360.0 );
            wxPoint  nextPt;

            start.x += radius;

            for( int step = 0; step < steps; ++step )
            {
                double rotation = ( angle * step ) / steps;
                nextPt = start;
                RotatePoint( &nextPt.x, &nextPt.y, center.x, center.y, rotation );
                aPolygons.Append( nextPt, -1, hole );

                if( firstPt )
                    firstPt = false;
                else
                    segOwners[ std::make_pair( prevPt, nextPt ) ] = graphic;

                prevPt = nextPt;
            }
        }
        else if( graphic->GetShape() == SHAPE_T::RECT )
        {
            std::vector<wxPoint> pts = graphic->GetRectCorners();

            for( const wxPoint& pt : pts )
            {
                aPolygons.Append( pt, -1, hole );

                if( firstPt )
                    firstPt = false;
                else
                    segOwners[ std::make_pair( prevPt, pt ) ] = graphic;

                prevPt = (wxPoint) pt;
            }
        }
        else
        {
            // Polygon start point. Arbitrarily chosen end of the segment and build the poly
            // from here.

            wxPoint startPt( graphic->GetEnd() );
            prevPt = graphic->GetEnd();
            aPolygons.Append( prevPt, -1, hole );

            // do not append the other end point yet, this first 'graphic' might be an arc
            for(;;)
            {
                switch( graphic->GetShape() )
                {
                case SHAPE_T::SEGMENT:
                    {
                        wxPoint nextPt;

                        // Use the line segment end point furthest away from
                        // prevPt as we assume the other end to be ON prevPt or
                        // very close to it.

                        if( closer_to_first( prevPt, graphic->GetStart(), graphic->GetEnd()) )
                            nextPt = graphic->GetEnd();
                        else
                            nextPt = graphic->GetStart();

                        aPolygons.Append( nextPt, -1, hole );
                        segOwners[ std::make_pair( prevPt, nextPt ) ] = graphic;
                        prevPt = nextPt;
                    }
                    break;

                case SHAPE_T::ARC:
                    // We do not support arcs in polygons, so approximate an arc with a series of
                    // short lines and put those line segments into the !same! PATH.
                    {
                        wxPoint pstart  = graphic->GetArcStart();
                        wxPoint pend    = graphic->GetArcEnd();
                        wxPoint pcenter = graphic->GetCenter();
                        double  angle   = -graphic->GetAngle();
                        int     radius  = graphic->GetRadius();
                        int     steps = GetArcToSegmentCount( radius, aErrorMax, angle / 10.0 );

                        if( !close_enough( prevPt, pstart, aChainingEpsilon ) )
                        {
                            wxASSERT( close_enough( prevPt, graphic->GetArcEnd(),
                                                    aChainingEpsilon ) );

                            angle = -angle;
                            std::swap( pstart, pend );
                        }

                        // Create intermediate points between start and end:
                         for( int step = 1; step < steps; ++step )
                        {
                            double rotation = ( angle * step ) / steps;
                            wxPoint pt = pstart;
                            RotatePoint( &pt, pcenter, rotation );

                            aPolygons.Append( pt, -1, hole );
                            segOwners[ std::make_pair( prevPt, pt ) ] = graphic;
                            prevPt = pt;
                        }

                        // Append the last arc end point
                        aPolygons.Append( pend, -1, hole );
                        segOwners[ std::make_pair( prevPt, pend ) ] = graphic;
                        prevPt = pend;
                    }
                    break;

                case SHAPE_T::BEZIER:
                    // We do not support Bezier curves in polygons, so approximate with a series
                    // of short lines and put those line segments into the !same! PATH.
                    {
                        wxPoint nextPt;
                        bool    reverse = false;

                        // Use the end point furthest away from
                        // prevPt as we assume the other end to be ON prevPt or
                        // very close to it.

                        if( closer_to_first( prevPt, graphic->GetStart(), graphic->GetEnd()) )
                        {
                            nextPt = graphic->GetEnd();
                        }
                        else
                        {
                            nextPt = graphic->GetStart();
                            reverse = true;
                        }

                        if( reverse )
                        {
                            for( int jj = graphic->GetBezierPoints().size()-1; jj >= 0; jj-- )
                            {
                                const wxPoint& pt = graphic->GetBezierPoints()[jj];
                                aPolygons.Append( pt, -1, hole );
                                segOwners[ std::make_pair( prevPt, pt ) ] = graphic;
                                prevPt = pt;
                            }
                        }
                        else
                        {
                            for( const wxPoint& pt : graphic->GetBezierPoints() )
                            {
                                aPolygons.Append( pt, -1, hole );
                                segOwners[ std::make_pair( prevPt, pt ) ] = graphic;
                                prevPt = pt;
                            }
                        }

                        prevPt = nextPt;
                    }
                    break;

                default:
                    wxFAIL_MSG( "Unsupported PCB_SHAPE type "
                                    + BOARD_ITEM::ShowShape( graphic->GetShape() ) );

                    return false;
                }

                // Get next closest segment.

                PCB_SHAPE* nextGraphic = findNext( graphic, prevPt, aSegList, aChainingEpsilon );

                if( nextGraphic && !( nextGraphic->GetFlags() & SKIP_STRUCT ) )
                {
                    graphic = nextGraphic;
                    graphic->SetFlags( SKIP_STRUCT );
                    startCandidates.erase( graphic );
                    continue;
                }

                // Finished, or ran into trouble...

                if( close_enough( startPt, prevPt, aChainingEpsilon ) )
                {
                    break;
                }
                else if( nextGraphic )  // encountered already-used segment, but not at the start
                {
                    if( aErrorHandler )
                        (*aErrorHandler)( _( "(self-intersecting)" ), graphic, nextGraphic, prevPt );

                    polygonComplete = false;
                    break;
                }
                else                    // encountered discontinuity
                {
                    if( aErrorHandler )
                        (*aErrorHandler)( _( "(not a closed shape)" ), graphic, nullptr, prevPt );

                    polygonComplete = false;
                    break;
                }
            }
        }
    }

    if( !polygonComplete )
        return false;

    // All of the silliness that follows is to work around the segment iterator while checking
    // for collisions.
    // TODO: Implement proper segment and point iterators that follow std

    for( auto seg1 = aPolygons.IterateSegmentsWithHoles(); seg1; seg1++ )
    {
        auto seg2 = seg1;

        for( ++seg2; seg2; seg2++ )
        {
            // Check for exact overlapping segments.
            if( *seg1 == *seg2 || ( ( *seg1 ).A == ( *seg2 ).B && ( *seg1 ).B == ( *seg2 ).A ) )
            {
                if( aErrorHandler )
                {
                    BOARD_ITEM* a = fetchOwner( *seg1 );
                    BOARD_ITEM* b = fetchOwner( *seg2 );

                    if( a && b )
                        (*aErrorHandler)( _( "(self-intersecting)" ), a, b, (wxPoint) ( *seg1 ).A );
                }

                selfIntersecting = true;
            }

            if( boost::optional<VECTOR2I> pt = seg1.Get().Intersect( seg2.Get(), true ) )
            {
                if( aErrorHandler )
                {
                    BOARD_ITEM* a = fetchOwner( *seg1 );
                    BOARD_ITEM* b = fetchOwner( *seg2 );

                    if( a && b )
                        (*aErrorHandler)( _( "(self-intersecting)" ), a, b, (wxPoint) pt.get() );
                }

                selfIntersecting = true;
            }
        }
    }

    return !selfIntersecting;
}


#include <board.h>
#include <collectors.h>

/* This function is used to extract a board outlines (3D view, automatic zones build ...)
 * Any closed outline inside the main outline is a hole
 * All contours should be closed, i.e. valid closed polygon vertices
 */
bool BuildBoardPolygonOutlines( BOARD* aBoard, SHAPE_POLY_SET& aOutlines, int aErrorMax,
                                int aChainingEpsilon, OUTLINE_ERROR_HANDLER* aErrorHandler )
{
    PCB_TYPE_COLLECTOR  items;
    bool                success = false;

    // Get all the PCB and FP shapes into 'items', then keep only those on layer == Edge_Cuts.
    static const KICAD_T  scan_graphics[] = { PCB_SHAPE_T, PCB_FP_SHAPE_T, EOT };
    items.Collect( aBoard, scan_graphics );

    // Make a working copy of aSegList, because the list is modified during calculations
    std::vector<PCB_SHAPE*> segList;

    for( int ii = 0; ii < items.GetCount(); ii++ )
    {
        if( items[ii]->GetLayer() == Edge_Cuts )
            segList.push_back( static_cast<PCB_SHAPE*>( items[ii] ) );
    }

    if( segList.size() )
    {
        success = ConvertOutlineToPolygon( segList, aOutlines, aErrorMax, aChainingEpsilon,
                                           aErrorHandler );
    }

    if( !success || !aOutlines.OutlineCount() )
    {
        // Couldn't create a valid polygon outline.  Use the board edge cuts bounding box to
        // create a rectangular outline, or, failing that, the bounding box of the items on
        // the board.

        EDA_RECT bbbox = aBoard->GetBoardEdgesBoundingBox();

        // If null area, uses the global bounding box.
        if( ( bbbox.GetWidth() ) == 0 || ( bbbox.GetHeight() == 0 ) )
            bbbox = aBoard->ComputeBoundingBox();

        // Ensure non null area. If happen, gives a minimal size.
        if( ( bbbox.GetWidth() ) == 0 || ( bbbox.GetHeight() == 0 ) )
            bbbox.Inflate( Millimeter2iu( 1.0 ) );

        aOutlines.RemoveAllContours();
        aOutlines.NewOutline();

        wxPoint corner;
        aOutlines.Append( bbbox.GetOrigin() );

        corner.x = bbbox.GetOrigin().x;
        corner.y = bbbox.GetEnd().y;
        aOutlines.Append( corner );

        aOutlines.Append( bbbox.GetEnd() );

        corner.x = bbbox.GetEnd().x;
        corner.y = bbbox.GetOrigin().y;
        aOutlines.Append( corner );
    }

    return success;
}


/**
 * Get the complete bounding box of the board (including all items).
 *
 * The vertex numbers and segment numbers of the rectangle returned.
 *              1
 *      *---------------*
 *      |1             2|
 *     0|               |2
 *      |0             3|
 *      *---------------*
 *              3
 */
void buildBoardBoundingBoxPoly( const BOARD* aBoard, SHAPE_POLY_SET& aOutline )
{
    EDA_RECT         bbbox = aBoard->GetBoundingBox();
    SHAPE_LINE_CHAIN chain;

    // If null area, uses the global bounding box.
    if( ( bbbox.GetWidth() ) == 0 || ( bbbox.GetHeight() == 0 ) )
        bbbox = aBoard->ComputeBoundingBox();

    // Ensure non null area. If happen, gives a minimal size.
    if( ( bbbox.GetWidth() ) == 0 || ( bbbox.GetHeight() == 0 ) )
        bbbox.Inflate( Millimeter2iu( 1.0 ) );

    // Inflate slightly (by 1/10th the size of the box)
    bbbox.Inflate( bbbox.GetWidth() / 10, bbbox.GetHeight() / 10 );

    chain.Append( bbbox.GetOrigin() );
    chain.Append( bbbox.GetOrigin().x, bbbox.GetEnd().y );
    chain.Append( bbbox.GetEnd() );
    chain.Append( bbbox.GetEnd().x, bbbox.GetOrigin().y );
    chain.SetClosed( true );

    aOutline.RemoveAllContours();
    aOutline.AddOutline( chain );
}


bool isCopperOutside( const FOOTPRINT* aMod, SHAPE_POLY_SET& aShape )
{
    bool padOutside = false;

    for( PAD* pad : aMod->Pads() )
    {
        SHAPE_POLY_SET poly = aShape;

        poly.BooleanIntersection( *pad->GetEffectivePolygon(), SHAPE_POLY_SET::PM_FAST );

        if( poly.OutlineCount() == 0 )
        {
            wxPoint padPos = pad->GetPosition();
            wxLogTrace( traceBoardOutline, "Tested pad (%d, %d): outside", padPos.x, padPos.y );
            padOutside = true;
            break;
        }

        wxPoint padPos = pad->GetPosition();
        wxLogTrace( traceBoardOutline, "Tested pad (%d, %d): not outside", padPos.x, padPos.y );
    }

    return padOutside;
}


VECTOR2I projectPointOnSegment( const VECTOR2I& aEndPoint, const SHAPE_POLY_SET& aOutline,
        int aOutlineNum = 0 )
{
    int      minDistance = -1;
    VECTOR2I projPoint;

    for( auto it = aOutline.CIterateSegments( aOutlineNum ); it; it++ )
    {
        auto seg = it.Get();
        int dis = seg.Distance( aEndPoint );

        if( minDistance < 0 || ( dis < minDistance ) )
        {
            minDistance = dis;
            projPoint   = seg.NearestPoint( aEndPoint );
        }
    }

    return projPoint;
}


int findEndSegments( SHAPE_LINE_CHAIN& aChain, SEG& aStartSeg, SEG& aEndSeg )
{
    int foundSegs = 0;

    for( int i = 0; i < aChain.SegmentCount(); i++ )
    {
        SEG seg = aChain.Segment( i );

        bool foundA = false;
        bool foundB = false;

        for( int j = 0; j < aChain.SegmentCount(); j++ )
        {
            // Don't test the segment against itself
            if( i == j )
                continue;

            SEG testSeg = aChain.Segment( j );

            if( testSeg.Contains( seg.A ) )
                foundA = true;

            if( testSeg.Contains( seg.B ) )
                foundB = true;
        }

        // This segment isn't a start or end
        if( foundA && foundB )
            continue;

        if( foundSegs == 0 )
        {
            // The first segment we encounter is the "start" segment
            wxLogTrace( traceBoardOutline, "Found start segment: (%d, %d)-(%d, %d)",
                        seg.A.x, seg.A.y, seg.B.x, seg.B.y );
            aStartSeg = seg;
            foundSegs++;
        }
        else
        {
            // Once we find both start and end, we can stop
            wxLogTrace( traceBoardOutline, "Found end segment: (%d, %d)-(%d, %d)",
                        seg.A.x, seg.A.y, seg.B.x, seg.B.y );
            aEndSeg = seg;
            foundSegs++;
            break;
        }
    }

    return foundSegs;
}


/**
 * This function is used to extract a board outline for a footprint view.
 *
 * Notes:
 * * Incomplete outlines will be closed by joining the end of the outline onto the bounding box
 *   (by simply projecting the end points) and then take the area that contains the copper.
 * * If all copper lies inside a closed outline, than that outline will be treated as an external
 *   board outline.
 * * If copper is located outside a closed outline, then that outline will be treated as a hole,
 *   and the outer edge will be formed using the bounding box.
 */
bool BuildFootprintPolygonOutlines( BOARD* aBoard, SHAPE_POLY_SET& aOutlines, int aErrorMax,
                                    int aChainingEpsilon, OUTLINE_ERROR_HANDLER* aErrorHandler )

{
    FOOTPRINT* footprint = aBoard->GetFirstFootprint();

    // No footprint loaded
    if( !footprint )
    {
        wxLogTrace( traceBoardOutline, "No footprint found on board" );
        return false;
    }

    PCB_TYPE_COLLECTOR  items;
    SHAPE_POLY_SET      outlines;
    bool                success = false;

    // Get all the SHAPEs into 'items', then keep only those on layer == Edge_Cuts.
    static const KICAD_T scan_graphics[] = { PCB_SHAPE_T, PCB_FP_SHAPE_T, EOT };
    items.Collect( aBoard, scan_graphics );

    // Make a working copy of aSegList, because the list is modified during calculations
    std::vector<PCB_SHAPE*> segList;

    for( int ii = 0; ii < items.GetCount(); ii++ )
    {
        if( items[ii]->GetLayer() == Edge_Cuts )
            segList.push_back( static_cast<PCB_SHAPE*>( items[ii] ) );
    }

    if( !segList.empty() )
    {
        success = ConvertOutlineToPolygon( segList, outlines, aErrorMax, aChainingEpsilon,
                                           aErrorHandler );
    }

    // A closed outline was found on Edge_Cuts
    if( success )
    {
        wxLogTrace( traceBoardOutline, "Closed outline found" );

        // If copper is outside a closed polygon, treat it as a hole
        if( isCopperOutside( footprint, outlines ) )
        {
            wxLogTrace( traceBoardOutline, "Treating outline as a hole" );

            buildBoardBoundingBoxPoly( aBoard, aOutlines );

            // Copy all outlines from the conversion as holes into the new outline
            for( int i = 0; i < outlines.OutlineCount(); i++ )
            {
                SHAPE_LINE_CHAIN& out = outlines.Outline( i );

                if( out.IsClosed() )
                    aOutlines.AddHole( out, -1 );

                for( int j = 0; j < outlines.HoleCount( i ); j++ )
                {
                    SHAPE_LINE_CHAIN& hole = outlines.Hole( i, j );

                    if( hole.IsClosed() )
                        aOutlines.AddHole( hole, -1 );
                }
            }
        }
        // If all copper is inside, then the computed outline is the board outline
        else
        {
            wxLogTrace( traceBoardOutline, "Treating outline as board edge" );
            aOutlines = outlines;
        }

        return true;
    }
    // No board outlines were found, so use the bounding box
    else if( outlines.OutlineCount() == 0 )
    {
        wxLogTrace( traceBoardOutline, "Using footprint bounding box" );
        buildBoardBoundingBoxPoly( aBoard, aOutlines );

        return true;
    }
    // There is an outline present, but it is not closed
    else
    {
        wxLogTrace( traceBoardOutline, "Trying to build outline" );

        std::vector<SHAPE_LINE_CHAIN> closedChains;
        std::vector<SHAPE_LINE_CHAIN> openChains;

        // The ConvertOutlineToPolygon function returns only one main outline and the rest as
        // holes, so we promote the holes and process them
        openChains.push_back( outlines.Outline( 0 ) );

        for( int j = 0; j < outlines.HoleCount( 0 ); j++ )
        {
            SHAPE_LINE_CHAIN hole = outlines.Hole( 0, j );

            if( hole.IsClosed() )
            {
                wxLogTrace( traceBoardOutline, "Found closed hole" );
                closedChains.push_back( hole );
            }
            else
            {
                wxLogTrace( traceBoardOutline, "Found open hole" );
                openChains.push_back( hole );
            }
        }

        SHAPE_POLY_SET bbox;
        buildBoardBoundingBoxPoly( aBoard, bbox );

        // Treat the open polys as the board edge
        SHAPE_LINE_CHAIN chain = openChains[0];
        SHAPE_LINE_CHAIN rect  = bbox.Outline( 0 );

        // We know the outline chain is open, so set to non-closed to get better segment count
        chain.SetClosed( false );

        SEG startSeg;
        SEG endSeg;

        // The two possible board outlines
        SHAPE_LINE_CHAIN upper;
        SHAPE_LINE_CHAIN lower;

        findEndSegments( chain, startSeg, endSeg );

        if( chain.SegmentCount() == 0 )
        {
            // Something is wrong, bail out with the overall footprint bounding box
            wxLogTrace( traceBoardOutline, "No line segments in provided outline" );
            aOutlines = bbox;
            return true;
        }
        else if( chain.SegmentCount() == 1 )
        {
            // This case means there is only 1 line segment making up the edge cuts of the
            // footprint, so we just need to use it to cut the bounding box in half.
            wxLogTrace( traceBoardOutline, "Only 1 line segment in provided outline" );

            startSeg = chain.Segment( 0 );

            // Intersect with all the sides of the rectangle
            OPT_VECTOR2I inter0 = startSeg.IntersectLines( rect.Segment( 0 ) );
            OPT_VECTOR2I inter1 = startSeg.IntersectLines( rect.Segment( 1 ) );
            OPT_VECTOR2I inter2 = startSeg.IntersectLines( rect.Segment( 2 ) );
            OPT_VECTOR2I inter3 = startSeg.IntersectLines( rect.Segment( 3 ) );

            if( inter0 && inter2 && !inter1 && !inter3 )
            {
                // Intersects the vertical rectangle sides only
                wxLogTrace( traceBoardOutline, "Segment intersects only vertical bbox sides" );

                // The upper half
                upper.Append( *inter0 );
                upper.Append( rect.GetPoint( 1 ) );
                upper.Append( rect.GetPoint( 2 ) );
                upper.Append( *inter2 );
                upper.SetClosed( true );

                // The lower half
                lower.Append( *inter0 );
                lower.Append( rect.GetPoint( 0 ) );
                lower.Append( rect.GetPoint( 3 ) );
                lower.Append( *inter2 );
                lower.SetClosed( true );
            }
            else if( inter1 && inter3 && !inter0 && !inter2 )
            {
                // Intersects the horizontal rectangle sides only
                wxLogTrace( traceBoardOutline, "Segment intersects only horizontal bbox sides" );

                // The left half
                upper.Append( *inter1 );
                upper.Append( rect.GetPoint( 1 ) );
                upper.Append( rect.GetPoint( 0 ) );
                upper.Append( *inter3 );
                upper.SetClosed( true );

                // The right half
                lower.Append( *inter1 );
                lower.Append( rect.GetPoint( 2 ) );
                lower.Append( rect.GetPoint( 3 ) );
                lower.Append( *inter3 );
                lower.SetClosed( true );
            }
            else
            {
                // Angled line segment that cuts across a corner
                wxLogTrace( traceBoardOutline, "Segment intersects two perpendicular bbox sides" );

                // Figure out which actual lines are intersected, since IntersectLines assumes
                // an infinite line
                bool hit0 = rect.Segment( 0 ).Contains( *inter0 );
                bool hit1 = rect.Segment( 1 ).Contains( *inter1 );
                bool hit2 = rect.Segment( 2 ).Contains( *inter2 );
                bool hit3 = rect.Segment( 3 ).Contains( *inter3 );

                if( hit0 && hit1 )
                {
                    // Cut across the upper left corner
                    wxLogTrace( traceBoardOutline, "Segment cuts upper left corner" );

                    // The upper half
                    upper.Append( *inter0 );
                    upper.Append( rect.GetPoint( 1 ) );
                    upper.Append( *inter1 );
                    upper.SetClosed( true );

                    // The lower half
                    lower.Append( *inter0 );
                    lower.Append( rect.GetPoint( 0 ) );
                    lower.Append( rect.GetPoint( 3 ) );
                    lower.Append( rect.GetPoint( 2 ) );
                    lower.Append( *inter1 );
                    lower.SetClosed( true );
                }
                else if( hit1 && hit2 )
                {
                    // Cut across the upper right corner
                    wxLogTrace( traceBoardOutline, "Segment cuts upper right corner" );

                    // The upper half
                    upper.Append( *inter1 );
                    upper.Append( rect.GetPoint( 2 ) );
                    upper.Append( *inter2 );
                    upper.SetClosed( true );

                    // The lower half
                    lower.Append( *inter1 );
                    lower.Append( rect.GetPoint( 1 ) );
                    lower.Append( rect.GetPoint( 0 ) );
                    lower.Append( rect.GetPoint( 3 ) );
                    lower.Append( *inter2 );
                    lower.SetClosed( true );
                }
                else if( hit2 && hit3 )
                {
                    // Cut across the lower right corner
                    wxLogTrace( traceBoardOutline, "Segment cuts lower right corner" );

                    // The upper half
                    upper.Append( *inter2 );
                    upper.Append( rect.GetPoint( 2 ) );
                    upper.Append( rect.GetPoint( 1 ) );
                    upper.Append( rect.GetPoint( 0 ) );
                    upper.Append( *inter3 );
                    upper.SetClosed( true );

                    // The bottom half
                    lower.Append( *inter2 );
                    lower.Append( rect.GetPoint( 3 ) );
                    lower.Append( *inter3 );
                    lower.SetClosed( true );
                }
                else
                {
                    // Cut across the lower left corner
                    wxLogTrace( traceBoardOutline, "Segment cuts upper left corner" );

                    // The upper half
                    upper.Append( *inter0 );
                    upper.Append( rect.GetPoint( 1 ) );
                    upper.Append( rect.GetPoint( 2 ) );
                    upper.Append( rect.GetPoint( 3 ) );
                    upper.Append( *inter3 );
                    upper.SetClosed( true );

                    // The bottom half
                    lower.Append( *inter0 );
                    lower.Append( rect.GetPoint( 0 ) );
                    lower.Append( *inter3 );
                    lower.SetClosed( true );
                }
            }
        }
        else
        {
            // More than 1 segment
            wxLogTrace( traceBoardOutline, "Multiple segments in outline" );

            // Just a temporary thing
            aOutlines = bbox;
            return true;
        }

        // Figure out which is the correct outline
        SHAPE_POLY_SET poly1;
        SHAPE_POLY_SET poly2;

        poly1.NewOutline();
        poly1.Append( upper );

        poly2.NewOutline();
        poly2.Append( lower );

        if( isCopperOutside( footprint, poly1 ) )
        {
            wxLogTrace( traceBoardOutline, "Using lower shape" );
            aOutlines = poly2;
        }
        else
        {
            wxLogTrace( traceBoardOutline, "Using upper shape" );
            aOutlines = poly1;
        }

        // Add all closed polys as holes to the main outline
        for( SHAPE_LINE_CHAIN& closedChain : closedChains )
        {
            wxLogTrace( traceBoardOutline, "Adding hole to main outline" );
            aOutlines.AddHole( closedChain, -1 );
        }

        return true;
    }

    // We really shouldn't reach this point
    return false;
}
