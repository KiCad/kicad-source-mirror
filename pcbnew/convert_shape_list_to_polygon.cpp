/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2015 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 1992-2023 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <convert_shape_list_to_polygon.h>

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
static PCB_SHAPE* findNext( PCB_SHAPE* aShape, const VECTOR2I& aPoint,
                            const std::vector<PCB_SHAPE*>& aList, unsigned aLimit )
{
    // Look for an unused, exact hit
    for( PCB_SHAPE* graphic : aList )
    {
        if( graphic == aShape || ( graphic->GetFlags() & SKIP_STRUCT ) != 0 )
            continue;

        if( aPoint == graphic->GetStart() || aPoint == graphic->GetEnd() )
            return graphic;
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

    return closest_graphic;     // Note: will be nullptr if nothing within aLimit
}


bool isCopperOutside( const FOOTPRINT* aFootprint, SHAPE_POLY_SET& aShape )
{
    bool padOutside = false;

    for( PAD* pad : aFootprint->Pads() )
    {
        SHAPE_POLY_SET poly = aShape.CloneDropTriangulation();

        poly.BooleanIntersection( *pad->GetEffectivePolygon(), SHAPE_POLY_SET::PM_FAST );

        if( poly.OutlineCount() == 0 )
        {
            VECTOR2I padPos = pad->GetPosition();
            wxLogTrace( traceBoardOutline, wxT( "Tested pad (%d, %d): outside" ),
                        padPos.x, padPos.y );
            padOutside = true;
            break;
        }

        VECTOR2I padPos = pad->GetPosition();
        wxLogTrace( traceBoardOutline, wxT( "Tested pad (%d, %d): not outside" ),
                    padPos.x, padPos.y );
    }

    return padOutside;
}


/* Build a polygon (with holes) from a PCB_SHAPE list, which is expected to be a closed main
 * outline with perhaps closed inner outlines.  These closed inner outlines are considered as
 * holes in the main outline.
 * @param aShapeList the initial list of SHAPEs (only lines, circles and arcs).
 * @param aPolygons will contain the complex polygon.
 * @param aErrorMax is the max error distance when polygonizing a curve (internal units)
 * @param aChainingEpsilon is the max error distance when polygonizing a curve (internal units)
 * @param aAllowDisjoint indicates multiple top-level outlines are allowed
 * @param aErrorHandler = an optional error handler
 * @param aAllowUseArcsInPolygons = an optional option to allow adding arcs in
 *  SHAPE_LINE_CHAIN polylines/polygons when building outlines from aShapeList
 */
bool ConvertOutlineToPolygon( std::vector<PCB_SHAPE*>& aShapeList, SHAPE_POLY_SET& aPolygons,
                              int aErrorMax, int aChainingEpsilon, bool aAllowDisjoint,
                              OUTLINE_ERROR_HANDLER* aErrorHandler, bool aAllowUseArcsInPolygons )
{
    if( aShapeList.size() == 0 )
        return true;

    bool selfIntersecting = false;

    wxString   msg;
    PCB_SHAPE* graphic = nullptr;

    std::set<PCB_SHAPE*> startCandidates( aShapeList.begin(), aShapeList.end() );

    // Keep a list of where the various shapes came from so after doing our combined-polygon
    // tests we can still report errors against the individual graphic items.
    std::map<std::pair<VECTOR2I, VECTOR2I>, PCB_SHAPE*> shapeOwners;

    auto fetchOwner =
            [&]( const SEG& seg ) -> PCB_SHAPE*
            {
                auto it = shapeOwners.find( std::make_pair( seg.A, seg.B ) );
                return it == shapeOwners.end() ? nullptr : it->second;
            };

    PCB_SHAPE* prevGraphic = nullptr;
    VECTOR2I   prevPt;

    std::vector<SHAPE_LINE_CHAIN> contours;

    for( PCB_SHAPE* shape : startCandidates )
        shape->ClearFlags( SKIP_STRUCT );

    while( startCandidates.size() )
    {
        graphic = (PCB_SHAPE*) *startCandidates.begin();
        graphic->SetFlags( SKIP_STRUCT );
        startCandidates.erase( startCandidates.begin() );

        contours.emplace_back();

        SHAPE_LINE_CHAIN& currContour = contours.back();
        bool firstPt = true;

        // Circles, rects and polygons are closed shapes unto themselves (and do not combine
        // with other shapes), so process them separately.
        if( graphic->GetShape() == SHAPE_T::POLY )
        {
            for( auto it = graphic->GetPolyShape().CIterate(); it; it++ )
            {
                VECTOR2I pt = *it;

                currContour.Append( pt );

                if( firstPt )
                    firstPt = false;
                else
                    shapeOwners[ std::make_pair( prevPt, pt ) ] = graphic;

                prevPt = pt;
            }

            currContour.SetClosed( true );
        }
        else if( graphic->GetShape() == SHAPE_T::CIRCLE )
        {
            VECTOR2I center = graphic->GetCenter();
            int      radius  = graphic->GetRadius();
            VECTOR2I start = center;
            start.x += radius;

            // Add 360 deg Arc in currContour
            SHAPE_ARC arc360( center, start, ANGLE_360, 0 );
            currContour.Append( arc360, aErrorMax );
            currContour.SetClosed( true );

            // set shapeOwners for currContour points created by appending the arc360:
            for( int ii = 1; ii < currContour.PointCount(); ++ii )
            {
                shapeOwners[ std::make_pair( currContour.CPoint( ii-1 ),
                                             currContour.CPoint( ii ) ) ] = graphic;
            }

            if( !aAllowUseArcsInPolygons )
                currContour.ClearArcs();
        }
        else if( graphic->GetShape() == SHAPE_T::RECT )
        {
            std::vector<VECTOR2I> pts = graphic->GetRectCorners();

            for( const VECTOR2I& pt : pts )
            {
                currContour.Append( pt );

                if( firstPt )
                    firstPt = false;
                else
                    shapeOwners[ std::make_pair( prevPt, pt ) ] = graphic;

                prevPt = pt;
            }

            currContour.SetClosed( true );
        }
        else
        {
            // Polygon start point. Arbitrarily chosen end of the segment and build the poly
            // from here.

            VECTOR2I startPt = graphic->GetEnd();
            prevPt = startPt;
            currContour.Append( prevPt );

            // do not append the other end point yet, this first 'graphic' might be an arc
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
                    break;
                }

                case SHAPE_T::SEGMENT:
                    {
                        VECTOR2I nextPt;

                        // Use the line segment end point furthest away from prevPt as we assume
                        // the other end to be ON prevPt or very close to it.

                        if( closer_to_first( prevPt, graphic->GetStart(), graphic->GetEnd()) )
                            nextPt = graphic->GetEnd();
                        else
                            nextPt = graphic->GetStart();

                        currContour.Append( nextPt );
                        shapeOwners[ std::make_pair( prevPt, nextPt ) ] = graphic;
                        prevPt = nextPt;
                    }
                    break;

                case SHAPE_T::ARC:
                    // We do not support arcs in polygons, so approximate an arc with a series of
                    // short lines and put those line segments into the !same! PATH.
                    {
                        VECTOR2I  pstart  = graphic->GetStart();
                        VECTOR2I  pend    = graphic->GetEnd();
                        VECTOR2I  pcenter = graphic->GetCenter();
                        EDA_ANGLE angle   = -graphic->GetArcAngle();
                        int       radius  = graphic->GetRadius();
                        int       steps   = GetArcToSegmentCount( radius, aErrorMax, angle );

                        if( !close_enough( prevPt, pstart, aChainingEpsilon ) )
                        {
                            wxASSERT( close_enough( prevPt, graphic->GetEnd(), aChainingEpsilon ) );

                            angle = -angle;
                            std::swap( pstart, pend );
                        }

                        // Create intermediate points between start and end:
                        for( int step = 1; step < steps; ++step )
                        {
                            EDA_ANGLE rotation = ( angle * step ) / steps;
                            VECTOR2I  pt = pstart;

                            RotatePoint( pt, pcenter, rotation );

                            currContour.Append( pt );
                            shapeOwners[ std::make_pair( prevPt, pt ) ] = graphic;
                            prevPt = pt;
                        }

                        // Append the last arc end point
                        currContour.Append( pend );
                        shapeOwners[ std::make_pair( prevPt, pend ) ] = graphic;
                        prevPt = pend;
                    }
                    break;

                case SHAPE_T::BEZIER:
                    // We do not support Bezier curves in polygons, so approximate with a series
                    // of short lines and put those line segments into the !same! PATH.
                    {
                        VECTOR2I nextPt;
                        bool    reverse = false;

                        // Use the end point furthest away from  prevPt as we assume the other
                        // end to be ON prevPt or very close to it.

                        if( closer_to_first( prevPt, graphic->GetStart(), graphic->GetEnd()) )
                        {
                            nextPt = graphic->GetEnd();
                        }
                        else
                        {
                            nextPt = graphic->GetStart();
                            reverse = true;
                        }

                        // Ensure the approximated Bezier shape is built
                        // a good value is between (Bezier curve width / 2) and (Bezier curve width)
                        // ( and at least 0.05 mm to avoid very small segments)
                        int min_segm_lenght = std::max( pcbIUScale.mmToIU( 0.05 ), graphic->GetWidth() );
                        graphic->RebuildBezierToSegmentsPointsList( min_segm_lenght );

                        if( reverse )
                        {
                            for( int jj = graphic->GetBezierPoints().size()-1; jj >= 0; jj-- )
                            {
                                const VECTOR2I& pt = graphic->GetBezierPoints()[jj];

                                if( prevPt == pt )
                                    continue;

                                currContour.Append( pt );
                                shapeOwners[ std::make_pair( prevPt, pt ) ] = graphic;
                                prevPt = pt;
                            }
                        }
                        else
                        {
                            for( const VECTOR2I& pt : graphic->GetBezierPoints() )
                            {
                                if( prevPt == pt )
                                    continue;

                                currContour.Append( pt );
                                shapeOwners[ std::make_pair( prevPt, pt ) ] = graphic;
                                prevPt = pt;
                            }
                        }

                        prevPt = nextPt;
                    }
                    break;

                default:
                    UNIMPLEMENTED_FOR( graphic->SHAPE_T_asString() );
                    return false;
                }

                // Get next closest segment.

                PCB_SHAPE* nextGraphic = findNext( graphic, prevPt, aShapeList, aChainingEpsilon );

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
                    currContour.SetClosed( true );
                    break;
                }
                else if( nextGraphic )  // encountered already-used segment, but not at the start
                {
                    if( aErrorHandler )
                        (*aErrorHandler)( _( "(self-intersecting)" ), graphic, nextGraphic, prevPt );

                    break;
                }
                else                    // encountered discontinuity
                {
                    if( aErrorHandler )
                        (*aErrorHandler)( _( "(not a closed shape)" ), graphic, nullptr, prevPt );

                    break;
                }
            }
        }
    }

    for( const SHAPE_LINE_CHAIN& contour : contours )
    {
        if( !contour.IsClosed() )
            return false;
    }

    // First, collect the parents of each contour
    //
    std::map<int, std::vector<int>> contourToParentIndexesMap;

    for( size_t ii = 0; ii < contours.size(); ++ii )
    {
        VECTOR2I         firstPt = contours[ii].GetPoint( 0 );
        std::vector<int> parents;

        for( size_t jj = 0; jj < contours.size(); ++jj )
        {
            if( jj == ii )
                continue;

            const SHAPE_LINE_CHAIN& parentCandidate = contours[jj];

            if( parentCandidate.PointInside( firstPt ) )
                parents.push_back( jj );
        }

        contourToParentIndexesMap[ii] = parents;
    }

    // Next add those that are top-level outlines to the SHAPE_POLY_SET
    //
    std::map<int, int> contourToOutlineIdxMap;

    for( const auto& [ contourIndex, parentIndexes ] : contourToParentIndexesMap )
    {
        if( parentIndexes.size() %2 == 0 )
        {
            // Even number of parents; top-level outline
            if( !aAllowDisjoint && !aPolygons.IsEmpty() )
            {
                if( aErrorHandler )
                {
                    BOARD_ITEM* a = fetchOwner( aPolygons.Outline( 0 ).GetSegment( 0 ) );
                    BOARD_ITEM* b = fetchOwner( contours[ contourIndex ].GetSegment( 0 ) );

                    if( a && b )
                    {
                        (*aErrorHandler)( _( "(multiple board outlines not supported)" ), a, b,
                                          contours[ contourIndex ].GetPoint( 0 ) );
                    }
                }

                return false;
            }

            aPolygons.AddOutline( contours[ contourIndex ] );
            contourToOutlineIdxMap[ contourIndex ] = aPolygons.OutlineCount() - 1;
        }
    }

    // And finally add the holes
    //
    for( const auto& [ contourIndex, parentIndexes ] : contourToParentIndexesMap )
    {
        if( parentIndexes.size() %2 == 1 )
        {
            // Odd number of parents; we're a hole in the parent which has one fewer parents
            // than we have.
            const SHAPE_LINE_CHAIN& hole = contours[ contourIndex ];

            for( int parentContourIdx : parentIndexes )
            {
                if( contourToParentIndexesMap[ parentContourIdx ].size() == parentIndexes.size() - 1 )
                {
                    int outlineIdx = contourToOutlineIdxMap[ parentContourIdx ];
                    aPolygons.AddHole( hole, outlineIdx );
                    break;
                }
            }
        }
    }

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
                        (*aErrorHandler)( _( "(self-intersecting)" ), a, b, ( *seg1 ).A );
                }

                selfIntersecting = true;
            }

            if( OPT_VECTOR2I pt = seg1.Get().Intersect( seg2.Get(), true ) )
            {
                if( aErrorHandler )
                {
                    BOARD_ITEM* a = fetchOwner( *seg1 );
                    BOARD_ITEM* b = fetchOwner( *seg2 );

                    if( a && b )
                        (*aErrorHandler)( _( "(self-intersecting)" ), a, b, *pt );
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
                                int aChainingEpsilon, OUTLINE_ERROR_HANDLER* aErrorHandler,
                                bool aAllowUseArcsInPolygons )
{
    PCB_TYPE_COLLECTOR  items;
    bool                success = false;

    SHAPE_POLY_SET      fpHoles;

    // Get all the shapes into 'items', then keep only those on layer == Edge_Cuts.
    items.Collect( aBoard, { PCB_SHAPE_T } );

    for( int ii = 0; ii < items.GetCount(); ++ii )
        items[ii]->ClearFlags( SKIP_STRUCT );

    for( FOOTPRINT* fp : aBoard->Footprints() )
    {
        PCB_TYPE_COLLECTOR fpItems;
        fpItems.Collect( fp, { PCB_SHAPE_T } );

        std::vector<PCB_SHAPE*> fpSegList;

        for( int ii = 0; ii < fpItems.GetCount(); ii++ )
        {
            PCB_SHAPE* fpSeg = static_cast<PCB_SHAPE*>( fpItems[ii] );

            if( fpSeg->GetLayer() == Edge_Cuts )
                fpSegList.push_back( fpSeg );
        }

        if( !fpSegList.empty() )
        {
            SHAPE_POLY_SET fpOutlines;
            success = ConvertOutlineToPolygon( fpSegList, fpOutlines, aErrorMax, aChainingEpsilon,
                                               false,
                                               // don't report errors here; the second pass also
                                               // gets an opportunity to use these segments
                                               nullptr, aAllowUseArcsInPolygons );

            // Here, we test to see if we should make holes or outlines.  Holes are made if the footprint
            // has copper outside of a single, closed outline.  If there are multiple outlines, we assume
            // that the footprint edges represent holes as we do not support multiple boards.  Similarly, if
            // any of the footprint pads are located outside of the edges, then the edges are holes
            if( success && ( isCopperOutside( fp, fpOutlines ) || fpOutlines.OutlineCount() > 1 ) )
            {
                fpHoles.Append( fpOutlines );
            }
            else
            {
                // If it wasn't a closed area, or wasn't a hole, the we want to keep the fpSegs
                // in contention for the board outline builds.
                for( int ii = 0; ii < fpItems.GetCount(); ++ii )
                    fpItems[ii]->ClearFlags( SKIP_STRUCT );
            }
        }
    }

    // Make a working copy of aSegList, because the list is modified during calculations
    std::vector<PCB_SHAPE*> segList;

    for( int ii = 0; ii < items.GetCount(); ii++ )
    {
        PCB_SHAPE* seg = static_cast<PCB_SHAPE*>( items[ii] );

        // Skip anything already used to generate footprint holes (above)
        if( seg->GetFlags() & SKIP_STRUCT )
            continue;

        if( seg->GetLayer() == Edge_Cuts )
            segList.push_back( seg );
    }

    if( segList.size() )
    {
        success = ConvertOutlineToPolygon( segList, aOutlines, aErrorMax, aChainingEpsilon,
                                           true, aErrorHandler, aAllowUseArcsInPolygons );
    }

    if( !success || !aOutlines.OutlineCount() )
    {
        // Couldn't create a valid polygon outline.  Use the board edge cuts bounding box to
        // create a rectangular outline, or, failing that, the bounding box of the items on
        // the board.

        BOX2I bbbox = aBoard->GetBoardEdgesBoundingBox();

        // If null area, uses the global bounding box.
        if( ( bbbox.GetWidth() ) == 0 || ( bbbox.GetHeight() == 0 ) )
            bbbox = aBoard->ComputeBoundingBox();

        // Ensure non null area. If happen, gives a minimal size.
        if( ( bbbox.GetWidth() ) == 0 || ( bbbox.GetHeight() == 0 ) )
            bbbox.Inflate( pcbIUScale.mmToIU( 1.0 ) );

        aOutlines.RemoveAllContours();
        aOutlines.NewOutline();

        VECTOR2I corner;
        aOutlines.Append( bbbox.GetOrigin() );

        corner.x = bbbox.GetOrigin().x;
        corner.y = bbbox.GetEnd().y;
        aOutlines.Append( corner );

        aOutlines.Append( bbbox.GetEnd() );

        corner.x = bbbox.GetEnd().x;
        corner.y = bbbox.GetOrigin().y;
        aOutlines.Append( corner );
    }

    for( int ii = 0; ii < fpHoles.OutlineCount(); ++ii )
    {
        const VECTOR2I holePt = fpHoles.Outline( ii ).CPoint( 0 );

        for( int jj = 0; jj < aOutlines.OutlineCount(); ++jj )
        {
            if( aOutlines.Outline( jj ).PointInside( holePt ) )
            {
                aOutlines.AddHole( fpHoles.Outline( ii ), jj );
                break;
            }
        }
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
    BOX2I            bbbox = aBoard->GetBoundingBox();
    SHAPE_LINE_CHAIN chain;

    // If null area, uses the global bounding box.
    if( ( bbbox.GetWidth() ) == 0 || ( bbbox.GetHeight() == 0 ) )
        bbbox = aBoard->ComputeBoundingBox();

    // Ensure non null area. If happen, gives a minimal size.
    if( ( bbbox.GetWidth() ) == 0 || ( bbbox.GetHeight() == 0 ) )
        bbbox.Inflate( pcbIUScale.mmToIU( 1.0 ) );

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
            wxLogTrace( traceBoardOutline, wxT( "Found start segment: (%d, %d)-(%d, %d)" ),
                        seg.A.x, seg.A.y, seg.B.x, seg.B.y );
            aStartSeg = seg;
            foundSegs++;
        }
        else
        {
            // Once we find both start and end, we can stop
            wxLogTrace( traceBoardOutline, wxT( "Found end segment: (%d, %d)-(%d, %d)" ),
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
        wxLogTrace( traceBoardOutline, wxT( "No footprint found on board" ) );
        return false;
    }

    PCB_TYPE_COLLECTOR  items;
    SHAPE_POLY_SET      outlines;
    bool                success = false;

    // Get all the SHAPEs into 'items', then keep only those on layer == Edge_Cuts.
    items.Collect( aBoard, { PCB_SHAPE_T } );

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
                                           true, aErrorHandler );
    }

    // A closed outline was found on Edge_Cuts
    if( success )
    {
        wxLogTrace( traceBoardOutline, wxT( "Closed outline found" ) );

        // If copper is outside a closed polygon, treat it as a hole
        // If there are multiple outlines in the footprint, they are also holes
        if( isCopperOutside( footprint, outlines ) || outlines.OutlineCount() > 1 )
        {
            wxLogTrace( traceBoardOutline, wxT( "Treating outline as a hole" ) );

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
            wxLogTrace( traceBoardOutline, wxT( "Treating outline as board edge" ) );
            aOutlines = outlines;
        }

        return true;
    }
    // No board outlines were found, so use the bounding box
    else if( outlines.OutlineCount() == 0 )
    {
        wxLogTrace( traceBoardOutline, wxT( "Using footprint bounding box" ) );
        buildBoardBoundingBoxPoly( aBoard, aOutlines );

        return true;
    }
    // There is an outline present, but it is not closed
    else
    {
        wxLogTrace( traceBoardOutline, wxT( "Trying to build outline" ) );

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
                wxLogTrace( traceBoardOutline, wxT( "Found closed hole" ) );
                closedChains.push_back( hole );
            }
            else
            {
                wxLogTrace( traceBoardOutline, wxT( "Found open hole" ) );
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
            wxLogTrace( traceBoardOutline, wxT( "No line segments in provided outline" ) );
            aOutlines = bbox;
            return true;
        }
        else if( chain.SegmentCount() == 1 )
        {
            // This case means there is only 1 line segment making up the edge cuts of the
            // footprint, so we just need to use it to cut the bounding box in half.
            wxLogTrace( traceBoardOutline, wxT( "Only 1 line segment in provided outline" ) );

            startSeg = chain.Segment( 0 );

            // Intersect with all the sides of the rectangle
            OPT_VECTOR2I inter0 = startSeg.IntersectLines( rect.Segment( 0 ) );
            OPT_VECTOR2I inter1 = startSeg.IntersectLines( rect.Segment( 1 ) );
            OPT_VECTOR2I inter2 = startSeg.IntersectLines( rect.Segment( 2 ) );
            OPT_VECTOR2I inter3 = startSeg.IntersectLines( rect.Segment( 3 ) );

            if( inter0 && inter2 && !inter1 && !inter3 )
            {
                // Intersects the vertical rectangle sides only
                wxLogTrace( traceBoardOutline, wxT( "Segment intersects only vertical bbox "
                                                    "sides" ) );

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
                wxLogTrace( traceBoardOutline, wxT( "Segment intersects only horizontal bbox "
                                                    "sides" ) );

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
                wxLogTrace( traceBoardOutline, wxT( "Segment intersects two perpendicular bbox "
                                                    "sides" ) );

                // Figure out which actual lines are intersected, since IntersectLines assumes
                // an infinite line
                bool hit0 = rect.Segment( 0 ).Contains( *inter0 );
                bool hit1 = rect.Segment( 1 ).Contains( *inter1 );
                bool hit2 = rect.Segment( 2 ).Contains( *inter2 );
                bool hit3 = rect.Segment( 3 ).Contains( *inter3 );

                if( hit0 && hit1 )
                {
                    // Cut across the upper left corner
                    wxLogTrace( traceBoardOutline, wxT( "Segment cuts upper left corner" ) );

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
                    wxLogTrace( traceBoardOutline, wxT( "Segment cuts upper right corner" ) );

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
                    wxLogTrace( traceBoardOutline, wxT( "Segment cuts lower right corner" ) );

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
                    wxLogTrace( traceBoardOutline, wxT( "Segment cuts upper left corner" ) );

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
            wxLogTrace( traceBoardOutline, wxT( "Multiple segments in outline" ) );

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
            wxLogTrace( traceBoardOutline, wxT( "Using lower shape" ) );
            aOutlines = poly2;
        }
        else
        {
            wxLogTrace( traceBoardOutline, wxT( "Using upper shape" ) );
            aOutlines = poly1;
        }

        // Add all closed polys as holes to the main outline
        for( SHAPE_LINE_CHAIN& closedChain : closedChains )
        {
            wxLogTrace( traceBoardOutline, wxT( "Adding hole to main outline" ) );
            aOutlines.AddHole( closedChain, -1 );
        }

        return true;
    }

    // We really shouldn't reach this point
    return false;
}
