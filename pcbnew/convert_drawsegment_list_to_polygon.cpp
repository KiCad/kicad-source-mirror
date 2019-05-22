/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2015 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 1992-2019 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file convert_drawsegment_list_to_polygon.cpp
 * @brief functions to convert a shape built with DRAWSEGMENTS to a polygon.
 * expecting the shape describes shape similar to a polygon
 */

#include <trigo.h>
#include <macros.h>

#include <math/vector2d.h>
#include <class_drawsegment.h>
#include <class_module.h>
#include <base_units.h>
#include <convert_basic_shapes_to_polygon.h>
#include <geometry/shape_poly_set.h>
#include <geometry/geometry_utils.h>


/**
 * Function close_ness
 * is a non-exact distance (also called Manhattan distance) used to approximate
 * the distance between two points.
 * The distance is very in-exact, but can be helpful when used
 * to pick between alternative neighboring points.
 * @param aLeft is the first point
 * @param aRight is the second point
 * @return unsigned - a measure of proximity that the caller knows about, in BIU,
 *  but remember it is only an approximation.
 */

static unsigned close_ness(  const wxPoint& aLeft, const wxPoint& aRight )
{
    // Don't need an accurate distance calculation, just something
    // approximating it, for relative ordering.
    return unsigned( std::abs( aLeft.x - aRight.x ) + abs( aLeft.y - aRight.y ) );
}

/**
 * Function close_enough
 * is a local and tunable method of qualifying the proximity of two points.
 *
 * @param aLeft is the first point
 * @param aRight is the second point
 * @param aLimit is a measure of proximity that the caller knows about.
 * @return bool - true if the two points are close enough, else false.
 */
inline bool close_enough( const wxPoint& aLeft, const wxPoint& aRight, unsigned aLimit )
{
    // We don't use an accurate distance calculation, just something
    // approximating it, since aLimit is non-exact anyway except when zero.
    return close_ness( aLeft, aRight ) <= aLimit;
}

/**
 * Function close_st
 * is a local method of qualifying if either the start of end point of a segment is closest to a point.
 *
 * @param aReference is the reference point
 * @param aFirst is the first point
 * @param aSecond is the second point
 * @return bool - true if the the first point is closest to the reference, otherwise false.
 */
inline bool close_st( const wxPoint& aReference, const wxPoint& aFirst, const wxPoint& aSecond )
{
    // We don't use an accurate distance calculation, just something
    // approximating to find the closest to the reference.
    return close_ness( aReference, aFirst ) <= close_ness( aReference, aSecond );
}


/**
 * Searches for a DRAWSEGMENT matching a given end point or start point in a list, and
 * if found, removes it from the TYPE_COLLECTOR and returns it, else returns NULL.
 * @param aPoint The starting or ending point to search for.
 * @param aList The list to remove from.
 * @param aLimit is the distance from \a aPoint that still constitutes a valid find.
 * @return DRAWSEGMENT* - The first DRAWSEGMENT that has a start or end point matching
 *   aPoint, otherwise NULL if none.
 */
static DRAWSEGMENT* findPoint( const wxPoint& aPoint, std::vector< DRAWSEGMENT* >& aList, unsigned aLimit )
{
    unsigned min_d = INT_MAX;
    int      ndx_min = 0;

    // find the point closest to aPoint and perhaps exactly matching aPoint.
    for( size_t i = 0; i < aList.size(); ++i )
    {
        DRAWSEGMENT*    graphic = aList[i];
        unsigned        d;

        switch( graphic->GetShape() )
        {
        case S_ARC:
            if( aPoint == graphic->GetArcStart() || aPoint == graphic->GetArcEnd() )
            {
                aList.erase( aList.begin() + i );
                return graphic;
            }

            d = close_ness( aPoint, graphic->GetArcStart() );
            if( d < min_d )
            {
                min_d = d;
                ndx_min = i;
            }

            d = close_ness( aPoint, graphic->GetArcEnd() );
            if( d < min_d )
            {
                min_d = d;
                ndx_min = i;
            }
            break;

        default:
            if( aPoint == graphic->GetStart() || aPoint == graphic->GetEnd() )
            {
                aList.erase( aList.begin() + i );
                return graphic;
            }

            d = close_ness( aPoint, graphic->GetStart() );
            if( d < min_d )
            {
                min_d = d;
                ndx_min = i;
            }

            d = close_ness( aPoint, graphic->GetEnd() );
            if( d < min_d )
            {
                min_d = d;
                ndx_min = i;
            }
        }
    }

    if( min_d <= aLimit )
    {
        DRAWSEGMENT* graphic = aList[ndx_min];
        aList.erase( aList.begin() + ndx_min );
        return graphic;
    }

    return NULL;
}


/**
 * Function ConvertOutlineToPolygon
 * build a polygon (with holes) from a DRAWSEGMENT list, which is expected to be
 * a outline, therefore a closed main outline with perhaps closed inner outlines.
 * These closed inner outlines are considered as holes in the main outline
 * @param aSegList the initial list of drawsegments (only lines, circles and arcs).
 * @param aPolygons will contain the complex polygon.
 * @param aTolerance is the max distance between points that is still accepted as connected (internal units)
 * @param aErrorText is a wxString to return error message.
 * @param aErrorLocation is the optional position of the error in the outline
 */
bool ConvertOutlineToPolygon( std::vector<DRAWSEGMENT*>& aSegList, SHAPE_POLY_SET& aPolygons,
                              wxString* aErrorText, unsigned int aTolerance, wxPoint* aErrorLocation )
{
    if( aSegList.size() == 0 )
        return true;

    wxString msg;

    // Make a working copy of aSegList, because the list is modified during calculations
    std::vector< DRAWSEGMENT* > segList = aSegList;

    DRAWSEGMENT* graphic;
    wxPoint prevPt;

    // Find edge point with minimum x, this should be in the outer polygon
    // which will define the perimeter polygon polygon.
    wxPoint xmin    = wxPoint( INT_MAX, 0 );
    int     xmini   = 0;

    for( size_t i = 0; i < segList.size(); i++ )
    {
        graphic = (DRAWSEGMENT*) segList[i];

        switch( graphic->GetShape() )
        {
        case S_SEGMENT:
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

        case S_ARC:
            // Freerouter does not yet understand arcs, so approximate
            // an arc with a series of short lines and put those
            // line segments into the !same! PATH.
            {
                wxPoint  pstart = graphic->GetArcStart();
                wxPoint  center = graphic->GetCenter();
                double   angle  = -graphic->GetAngle();
                double   radius = graphic->GetRadius();
                int      steps  = GetArcToSegmentCount( radius, ARC_LOW_DEF, angle / 10.0 );
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

        case S_CIRCLE:
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

        case S_CURVE:
            {
                graphic->RebuildBezierToSegmentsPointsList( graphic->GetWidth() );

                for( unsigned int jj = 0; jj < graphic->GetBezierPoints().size(); jj++ )
                {
                    wxPoint pt = graphic->GetBezierPoints()[jj];

                    if( pt.x < xmin.x )
                    {
                        xmin  = pt;
                        xmini = i;
                    }
                }
            }
            break;

        case S_POLYGON:
            {
                const auto poly = graphic->GetPolyShape();
                MODULE* module = aSegList[0]->GetParentModule();
                double orientation = module ? module->GetOrientation() : 0.0;
                VECTOR2I offset = module ? module->GetPosition() : VECTOR2I( 0, 0 );

                for( auto iter = poly.CIterate(); iter; iter++ )
                {
                    auto pt = *iter;
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

    // Grab the left most point, assume its on the board's perimeter, and see if we
    // can put enough graphics together by matching endpoints to formulate a cohesive
    // polygon.

    graphic = (DRAWSEGMENT*) segList[xmini];

    // The first DRAWSEGMENT is in 'graphic', ok to remove it from 'items'
    segList.erase( segList.begin() + xmini );

    // Output the outline perimeter as polygon.
    if( graphic->GetShape() == S_CIRCLE )
    {
        TransformCircleToPolygon( aPolygons, graphic->GetCenter(), graphic->GetRadius(), aTolerance );
    }
    else if( graphic->GetShape() == S_POLYGON )
    {
        MODULE* module = graphic->GetParentModule();     // NULL for items not in footprints
        double orientation = module ? module->GetOrientation() : 0.0;
        VECTOR2I offset = module ? module->GetPosition() : VECTOR2I( 0, 0 );

        aPolygons.NewOutline();

        for( auto it = graphic->GetPolyShape().CIterate( 0 ); it; it++ )
        {
            auto pt = *it;
            RotatePoint( pt, orientation );
            pt += offset;
            aPolygons.Append( pt );
        }
    }
    else
    {
        // Polygon start point. Arbitrarily chosen end of the
        // segment and build the poly from here.

        wxPoint startPt = wxPoint( graphic->GetEnd() );
        prevPt = graphic->GetEnd();
        aPolygons.NewOutline();
        aPolygons.Append( prevPt );

        // Do not append the other end point yet of this 'graphic', this first
        // 'graphic' might be an arc or a curve.

        for(;;)
        {
            switch( graphic->GetShape() )
            {
            case S_SEGMENT:
                {
                    wxPoint  nextPt;

                    // Use the line segment end point furthest away from
                    // prevPt as we assume the other end to be ON prevPt or
                    // very close to it.

                    if( close_st( prevPt, graphic->GetStart(), graphic->GetEnd() ) )
                        nextPt = graphic->GetEnd();
                    else
                        nextPt = graphic->GetStart();

                    aPolygons.Append( nextPt );
                    prevPt = nextPt;
                }
                break;

            case S_ARC:
                // We do not support arcs in polygons, so approximate
                // an arc with a series of short lines and put those
                // line segments into the !same! PATH.
                {
                    wxPoint pstart  = graphic->GetArcStart();
                    wxPoint pend    = graphic->GetArcEnd();
                    wxPoint pcenter = graphic->GetCenter();
                    double  angle   = -graphic->GetAngle();
                    double  radius  = graphic->GetRadius();
                    int     steps   = GetArcToSegmentCount( radius, aTolerance, angle / 10.0 );

                    if( !close_enough( prevPt, pstart, aTolerance ) )
                    {
                        wxASSERT( close_enough( prevPt, graphic->GetArcEnd(), aTolerance ) );

                        angle = -angle;
                        std::swap( pstart, pend );
                    }

                    wxPoint nextPt;

                    for( int step = 1; step<=steps; ++step )
                    {
                        double rotation = ( angle * step ) / steps;
                        nextPt = pstart;
                        RotatePoint( &nextPt, pcenter, rotation );

                        aPolygons.Append( nextPt );
                    }

                    prevPt = nextPt;
                }
                break;

            case S_CURVE:
                // We do not support Bezier curves in polygons, so approximate
                // with a series of short lines and put those
                // line segments into the !same! PATH.
                {
                    wxPoint  nextPt;
                    bool reverse = false;

                    // Use the end point furthest away from
                    // prevPt as we assume the other end to be ON prevPt or
                    // very close to it.

                    if( close_st( prevPt, graphic->GetStart(), graphic->GetEnd() ) )
                        nextPt = graphic->GetEnd();
                    else
                    {
                        nextPt = graphic->GetStart();
                        reverse = true;
                    }

                    if( reverse )
                    {
                        for( int jj = graphic->GetBezierPoints().size()-1; jj >= 0; jj-- )
                            aPolygons.Append( graphic->GetBezierPoints()[jj] );
                    }
                    else
                    {
                        for( size_t jj = 0; jj < graphic->GetBezierPoints().size(); jj++ )
                            aPolygons.Append( graphic->GetBezierPoints()[jj] );
                    }

                    prevPt = nextPt;
                }
                break;

            default:
                if( aErrorText )
                {
                    msg.Printf( "Unsupported DRAWSEGMENT type %s.",
                                BOARD_ITEM::ShowShape( graphic->GetShape() ) );

                    *aErrorText << msg << "\n";
                }

                if( aErrorLocation )
                    *aErrorLocation = graphic->GetPosition();

                return false;
            }

            // Get next closest segment.

            graphic = findPoint( prevPt, segList, aTolerance );

            // If there are no more close segments, check if the board
            // outline polygon can be closed.

            if( !graphic )
            {
                if( close_enough( startPt, prevPt, aTolerance ) )
                {
                    // Close the polygon back to start point
                    // aPolygons.Append( startPt ); // not needed
                }
                else
                {
                    if( aErrorText )
                    {
                        msg.Printf( _( "Unable to find segment with an endpoint of (%s, %s)." ),
                                    StringFromValue( MILLIMETRES, prevPt.x, true ),
                                    StringFromValue( MILLIMETRES, prevPt.y, true ) );

                        *aErrorText << msg << "\n";
                    }

                    if( aErrorLocation )
                        *aErrorLocation = prevPt;

                    return false;
                }
                break;
            }
        }
    }

    while( segList.size() )
    {
        // emit a signal layers keepout for every interior polygon left...
        int hole = aPolygons.NewHole();

        graphic = (DRAWSEGMENT*) segList[0];
        segList.erase( segList.begin() );

        // Both circles and polygons on the edge cuts layer are closed items that
        // do not connect to other elements, so we process them independently
        if( graphic->GetShape() == S_POLYGON )
        {
            MODULE* module = graphic->GetParentModule();     // NULL for items not in footprints
            double orientation = module ? module->GetOrientation() : 0.0;
            VECTOR2I offset = module ? module->GetPosition() : VECTOR2I( 0, 0 );

            for( auto it = graphic->GetPolyShape().CIterate(); it; it++ )
            {
                auto val = *it;
                RotatePoint( val, orientation );
                val += offset;

                aPolygons.Append( val, -1, hole );
            }
        }
        else if( graphic->GetShape() == S_CIRCLE )
        {
            // make a circle by segments;
            wxPoint  center  = graphic->GetCenter();
            double   angle   = 3600.0;
            wxPoint  start   = center;
            int      radius  = graphic->GetRadius();
            int      steps   = std::max<int>( 4, GetArcToSegmentCount( radius, aTolerance, 360.0 ) );
            wxPoint  nextPt;

            start.x += radius;

            for( int step = 0; step < steps; ++step )
            {
                double rotation = ( angle * step ) / steps;
                nextPt = start;
                RotatePoint( &nextPt.x, &nextPt.y, center.x, center.y, rotation );
                aPolygons.Append( nextPt, -1, hole );
            }
        }
        else
        {
            // Polygon start point. Arbitrarily chosen end of the
            // segment and build the poly from here.

            wxPoint startPt( graphic->GetEnd() );
            prevPt = graphic->GetEnd();
            aPolygons.Append( prevPt, -1, hole );

            // do not append the other end point yet, this first 'graphic' might be an arc
            for(;;)
            {
                switch( graphic->GetShape() )
                {
                case S_SEGMENT:
                    {
                        wxPoint nextPt;

                        // Use the line segment end point furthest away from
                        // prevPt as we assume the other end to be ON prevPt or
                        // very close to it.

                        if( close_st( prevPt, graphic->GetStart(), graphic->GetEnd() ) )
                        {
                            nextPt = graphic->GetEnd();
                        }
                        else
                        {
                            nextPt = graphic->GetStart();
                        }

                        prevPt = nextPt;
                        aPolygons.Append( prevPt, -1, hole );
                    }
                    break;

                case S_ARC:
                    // Freerouter does not yet understand arcs, so approximate
                    // an arc with a series of short lines and put those
                    // line segments into the !same! PATH.
                    {
                        wxPoint pstart  = graphic->GetArcStart();
                        wxPoint pend    = graphic->GetArcEnd();
                        wxPoint pcenter = graphic->GetCenter();
                        double  angle   = -graphic->GetAngle();
                        int     radius  = graphic->GetRadius();
                        int     steps = GetArcToSegmentCount( radius, aTolerance, angle / 10.0 );

                        if( !close_enough( prevPt, pstart, aTolerance ) )
                        {
                            wxASSERT( close_enough( prevPt, graphic->GetArcEnd(), aTolerance ) );

                            angle = -angle;
                            std::swap( pstart, pend );
                        }

                        wxPoint nextPt;

                        for( int step = 1; step <= steps; ++step )
                        {
                            double rotation = ( angle * step ) / steps;

                            nextPt = pstart;
                            RotatePoint( &nextPt, pcenter, rotation );

                            aPolygons.Append( nextPt, -1, hole );
                        }

                        prevPt = nextPt;
                    }
                    break;

                case S_CURVE:
                    // We do not support Bezier curves in polygons, so approximate
                    // with a series of short lines and put those
                    // line segments into the !same! PATH.
                    {
                        wxPoint  nextPt;
                        bool reverse = false;

                        // Use the end point furthest away from
                        // prevPt as we assume the other end to be ON prevPt or
                        // very close to it.

                        if( close_st( prevPt, graphic->GetStart(), graphic->GetEnd() ) )
                            nextPt = graphic->GetEnd();
                        else
                        {
                            nextPt = graphic->GetStart();
                            reverse = true;
                        }

                        if( reverse )
                        {
                            for( int jj = graphic->GetBezierPoints().size()-1; jj >= 0; jj-- )
                                aPolygons.Append( graphic->GetBezierPoints()[jj], -1, hole );
                        }
                        else
                        {
                            for( size_t jj = 0; jj < graphic->GetBezierPoints().size(); jj++ )
                                aPolygons.Append( graphic->GetBezierPoints()[jj], -1, hole );
                        }

                        prevPt = nextPt;
                    }
                    break;

                default:
                    if( aErrorText )
                    {
                        msg.Printf( "Unsupported DRAWSEGMENT type %s.",
                                    BOARD_ITEM::ShowShape( graphic->GetShape() ) );

                        *aErrorText << msg << "\n";
                    }

                    if( aErrorLocation )
                        *aErrorLocation = graphic->GetPosition();

                    return false;
                }

                // Get next closest segment.

                graphic = findPoint( prevPt, segList, aTolerance );

                // If there are no more close segments, check if polygon
                // can be closed.

                if( !graphic )
                {
                    if( close_enough( startPt, prevPt, aTolerance ) )
                    {
                        // Close the polygon back to start point
                        // aPolygons.Append( startPt, -1, hole );   // not needed
                    }
                    else
                    {
                        if( aErrorText )
                        {
                            msg.Printf( _( "Unable to find segment with an endpoint of (%s, %s)." ),
                                        StringFromValue( MILLIMETRES, prevPt.x, true ),
                                        StringFromValue( MILLIMETRES, prevPt.y, true ) );

                            *aErrorText << msg << "\n";
                        }

                        if( aErrorLocation )
                            *aErrorLocation = prevPt;

                        return false;
                    }
                    break;
                }
            }
        }
    }

    // All of the silliness that follows is to work around the segment iterator
    // while checking for collisions.
    // TODO: Implement proper segment and point iterators that follow std
    for( auto seg1 = aPolygons.IterateSegmentsWithHoles(); seg1; seg1++ )
    {
        auto seg2 = seg1;

        for( ++seg2; seg2; seg2++ )
        {
            // Check for exact overlapping segments.  This is not viewed
            // as an intersection below
            if( *seg1 == *seg2 ||
                    ( ( *seg1 ).A == ( *seg2 ).B && ( *seg1 ).B == ( *seg2 ).A ) )
            {
                if( aErrorLocation )
                {
                    aErrorLocation->x = ( *seg1 ).A.x;
                    aErrorLocation->y = ( *seg1 ).A.y;
                }

                return false;
            }

            if( auto pt = seg1.Get().Intersect( seg2.Get(), true ) )
            {
                if( aErrorLocation )
                {
                    aErrorLocation->x = pt->x;
                    aErrorLocation->y = pt->y;
                }

                return false;
            }
        }
    }

    return true;
}

#include <class_board.h>
#include <collectors.h>

/* This function is used to extract a board outlines (3D view, automatic zones build ...)
 * Any closed outline inside the main outline is a hole
 * All contours should be closed, i.e. valid closed polygon vertices
 */
bool BuildBoardPolygonOutlines( BOARD* aBoard, SHAPE_POLY_SET& aOutlines,
        wxString* aErrorText, unsigned int aTolerance, wxPoint* aErrorLocation )
{
    PCB_TYPE_COLLECTOR  items;

    // Get all the DRAWSEGMENTS and module graphics into 'items',
    // then keep only those on layer == Edge_Cuts.
    static const KICAD_T  scan_graphics[] = { PCB_LINE_T, PCB_MODULE_EDGE_T, EOT };
    items.Collect( aBoard, scan_graphics );

    // Make a working copy of aSegList, because the list is modified during calculations
    std::vector< DRAWSEGMENT* > segList;

    for( int ii = 0; ii < items.GetCount(); ii++ )
    {
        if( items[ii]->GetLayer() == Edge_Cuts )
            segList.push_back( static_cast< DRAWSEGMENT* >( items[ii] ) );
    }

    bool success = ConvertOutlineToPolygon( segList, aOutlines, aErrorText, aTolerance, aErrorLocation );

    if( !success || !aOutlines.OutlineCount() )
    {
        // Creates a valid polygon outline is not possible.
        // So uses the board edge cuts bounding box to create a
        // rectangular outline
        // When no edge cuts items, build a contour
        // from global bounding box

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
