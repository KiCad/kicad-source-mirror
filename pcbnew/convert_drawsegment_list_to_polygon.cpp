/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2015 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 1992-2017 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <class_drawsegment.h>
#include <base_units.h>
#include <convert_basic_shapes_to_polygon.h>


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
 * Function findPoint
 * searches for a DRAWSEGMENT with an end point or start point of aPoint, and
 * if found, removes it from the TYPE_COLLECTOR and returns it, else returns NULL.
 * @param aPoint The starting or ending point to search for.
 * @param items The list to remove from.
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
 * @param aErrorText is a wxString to return error message.
 */
bool ConvertOutlineToPolygon( std::vector< DRAWSEGMENT* >& aSegList,
                              SHAPE_POLY_SET& aPolygons, wxString* aErrorText)
{

    if( aSegList.size() == 0 )
        return true;

    wxString msg;

    // Make a working copy of aSegList, because the list is modified during calculations
    std::vector< DRAWSEGMENT* > segList = aSegList;

    unsigned    prox;           // a proximity BIU metric, not an accurate distance
    const int   STEPS = 16;     // for a segmentation of an arc of 360 degrees
    DRAWSEGMENT* graphic;
    wxPoint prevPt;

    // Find edge point with minimum x, this should be in the outer polygon
    // which will define the perimeter Edge.Cuts polygon.
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
                wxPoint     pstart   = graphic->GetArcStart();
                wxPoint     center  = graphic->GetCenter();
                double      angle   = -graphic->GetAngle();
                int         steps   = STEPS * fabs(angle) /3600.0;

                if( steps == 0 )
                    steps = 1;

                wxPoint     pt;

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

    // Set maximum proximity threshold for point to point nearness metric for
    // board perimeter only, not interior keepouts yet.
    prox = Millimeter2iu( 0.01 );   // should be enough to fix rounding issues
                                    // is arc start and end point calculations

    // Output the Edge.Cuts perimeter as circle or polygon.
    if( graphic->GetShape() == S_CIRCLE )
    {
        TransformCircleToPolygon( aPolygons, graphic->GetCenter(),
                                  graphic->GetRadius(), STEPS );
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
        // 'graphic' might be an arc.

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
                    {
                        nextPt = graphic->GetEnd();
                    }
                    else
                    {
                        nextPt = graphic->GetStart();
                    }

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
                    double  angle  = -graphic->GetAngle();
                    int     steps  = STEPS * fabs(angle) /3600.0;

                    if( steps == 0 )
                        steps = 1;

                    if( !close_enough( prevPt, pstart, prox ) )
                    {
                        wxASSERT( close_enough( prevPt, graphic->GetArcEnd(), prox ) );

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

            default:
                if( aErrorText )
                {
                    msg.Printf( _( "Unsupported DRAWSEGMENT type %s" ),
                        GetChars( BOARD_ITEM::ShowShape( graphic->GetShape() ) ) );

                    *aErrorText << msg << "\n";
                }

                return false;
            }

            // Get next closest segment.

            graphic = findPoint( prevPt, segList, prox );

            // If there are no more close segments, check if the board
            // outline polygon can be closed.

            if( !graphic )
            {
                if( close_enough( startPt, prevPt, prox ) )
                {
                    // Close the polygon back to start point
                    // aPolygons.Append( startPt ); // not needed
                }
                else
                {
                    if( aErrorText )
                    {
                        msg.Printf(
                            _( "Unable to find the next boundary segment with an endpoint of (%s mm, %s mm). "
                                "graphic outline must form a contiguous, closed polygon." ),
                            GetChars( FROM_UTF8( BOARD_ITEM::FormatInternalUnits( prevPt.x ).c_str() ) ),
                            GetChars( FROM_UTF8( BOARD_ITEM::FormatInternalUnits( prevPt.y ).c_str() ) )
                            );

                        *aErrorText << msg << "\n";
                    }

                    return false;
                }
                break;
            }
        }
    }

    // Output the interior Edge.Cuts graphics as keepouts, using same nearness
    // metric as the board edge as otherwise we have trouble completing complex
    // polygons.
    prox = Millimeter2iu( 0.05 );

    while( segList.size() )
    {
        // emit a signal layers keepout for every interior polygon left...
        int hole = aPolygons.NewHole();

        graphic = (DRAWSEGMENT*) segList[0];
        segList.erase( segList.begin() );

        if( graphic->GetShape() == S_CIRCLE )
        {
            // make a circle by segments;
            wxPoint     center  = graphic->GetCenter();
            double      angle   = 3600.0;
            wxPoint     start = center;
            int         radius  = graphic->GetRadius();
            start.x += radius;

            wxPoint nextPt;

            for( int step = 0; step<STEPS; ++step )
            {
                double rotation = ( angle * step ) / STEPS;
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
                        wxPoint     pstart   = graphic->GetArcStart();
                        wxPoint     pend     = graphic->GetArcEnd();
                        wxPoint     pcenter  = graphic->GetCenter();
                        double      angle   = -graphic->GetAngle();
                        int         steps   = STEPS * fabs(angle) /3600.0;

                        if( steps == 0 )
                            steps = 1;

                        if( !close_enough( prevPt, pstart, prox ) )
                        {
                            wxASSERT( close_enough( prevPt, graphic->GetArcEnd(), prox ) );

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

                default:
                    if( aErrorText )
                    {
                        msg.Printf(
                            _( "Unsupported DRAWSEGMENT type %s" ),
                            GetChars( BOARD_ITEM::ShowShape( graphic->GetShape() ) ) );

                        *aErrorText << msg << "\n";
                    }

                    return false;
                }

                // Get next closest segment.

                graphic = findPoint( prevPt, segList, prox );

                // If there are no more close segments, check if polygon
                // can be closed.

                if( !graphic )
                {
                    if( close_enough( startPt, prevPt, prox ) )
                    {
                        // Close the polygon back to start point
                        // aPolygons.Append( startPt, -1, hole );   // not needed
                    }
                    else
                    {
                        if( aErrorText )
                        {
                            msg.Printf(
                                _( "Unable to find the next graphic segment with an endpoint of (%s mm, %s mm).\n"
                                   "Edit graphics, making them contiguous polygons each." ),
                                GetChars( FROM_UTF8( BOARD_ITEM::FormatInternalUnits( prevPt.x ).c_str() ) ),
                                GetChars( FROM_UTF8( BOARD_ITEM::FormatInternalUnits( prevPt.y ).c_str() ) )
                            );

                            *aErrorText << msg << "\n";
                        }

                        return false;
                    }
                    break;
                }
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
bool BuildBoardPolygonOutlines( BOARD* aBoard,
                                SHAPE_POLY_SET& aOutlines,
                                SHAPE_POLY_SET& aHoles,
                                wxString* aErrorText )
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

    bool success = ConvertOutlineToPolygon( segList, aOutlines, aErrorText );

    // Now move holes in aHoles
    // only one main outline is expected
    if( success && aOutlines.OutlineCount() )
    {
        int outlineId = 0;

        int holecount = aOutlines.HoleCount( outlineId );

        if( holecount )
        {
            for( int ii = 0; ii < holecount; ++ii )
            {
                aHoles.AddOutline( aOutlines.Hole( outlineId, ii ) );
            }

            // Remove holes from aOutlines:
            SHAPE_POLY_SET::POLYGON& polygon = aOutlines.Polygon( outlineId );
            polygon.erase( polygon.begin()+1, polygon.end() );
        }
    }
    else
    {
        // Creates a valid polygon outline is not possible.
        // So uses the board edge cuts bounding box to create a
        // rectangular outline
        // (when no edge cuts items, fillBOUNDARY build a contour
        // from global bounding box

        EDA_RECT bbbox = aBoard->GetBoardEdgesBoundingBox();

        // Ensure non null area. If happen, gives a minimal size.
        if( ( bbbox.GetWidth() ) == 0 || ( bbbox.GetHeight() == 0 ) )
            bbbox.Inflate( Millimeter2iu( 1.0 ) );

        aOutlines.RemoveAllContours();
        aOutlines.NewOutline();

        wxPoint corner;
        corner.x = bbbox.GetOrigin().x;
        corner.y = bbbox.GetOrigin().y;
        aOutlines.Append( corner.x, corner.y );

        corner.x = bbbox.GetOrigin().x;
        corner.y = bbbox.GetEnd().y;
        aOutlines.Append( corner.x, corner.y );

        corner.x = bbbox.GetEnd().x;
        corner.y = bbbox.GetEnd().y;
        aOutlines.Append( corner.x, corner.y );

        corner.x = bbbox.GetEnd().x;
        corner.y = bbbox.GetOrigin().y;
        aOutlines.Append( corner.x, corner.y );
    }

    return success;
}
