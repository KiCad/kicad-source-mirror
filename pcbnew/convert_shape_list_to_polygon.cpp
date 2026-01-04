/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2015 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
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

#include <unordered_set>
#include <deque>

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
#include <geometry/roundrect.h>
#include <convert_shape_list_to_polygon.h>
#include <board.h>
#include <collectors.h>

#include <nanoflann.hpp>

#include <wx/log.h>


/**
 * Flag to enable debug tracing for the board outline creation
 *
 * Use "KICAD_BOARD_OUTLINE" to enable.
 *
 * @ingroup trace_env_vars
 */
const wxChar* traceBoardOutline = wxT( "KICAD_BOARD_OUTLINE" );


class SCOPED_FLAGS_CLEANER : public std::unordered_set<EDA_ITEM*>
{
    EDA_ITEM_FLAGS m_flagsToClear;

public:
    SCOPED_FLAGS_CLEANER( const EDA_ITEM_FLAGS& aFlagsToClear ) : m_flagsToClear( aFlagsToClear ) {}

    ~SCOPED_FLAGS_CLEANER()
    {
        for( EDA_ITEM* item : *this )
            item->ClearFlags( m_flagsToClear );
    }
};


/**
 * Local and tunable method of qualifying the proximity of two points.
 *
 * @param aLeft is the first point.
 * @param aRight is the second point.
 * @param aLimit is a measure of proximity that the caller knows about.
 * @return true if the two points are close enough, else false.
 */
static bool close_enough( VECTOR2I aLeft, VECTOR2I aRight, unsigned aLimit )
{
    return ( aLeft - aRight ).SquaredEuclideanNorm() <= SEG::Square( aLimit );
}


/**
 * Local method which qualifies whether the start or end point of a segment is closest to a point.
 *
 * @param aRef is the reference point
 * @param aFirst is the first point
 * @param aSecond is the second point
 * @return true if the first point is closest to the reference, otherwise false.
 */
static bool closer_to_first( VECTOR2I aRef, VECTOR2I aFirst, VECTOR2I aSecond )
{
    return ( aRef - aFirst ).SquaredEuclideanNorm() < ( aRef - aSecond ).SquaredEuclideanNorm();
}


static bool isCopperOutside( const FOOTPRINT* aFootprint, SHAPE_POLY_SET& aShape )
{
    bool padOutside = false;

    for( PAD* pad : aFootprint->Pads() )
    {
        pad->Padstack().ForEachUniqueLayer(
            [&]( PCB_LAYER_ID aLayer )
            {
                SHAPE_POLY_SET poly = aShape.CloneDropTriangulation();

                poly.ClearArcs();

                poly.BooleanIntersection( *pad->GetEffectivePolygon( aLayer, ERROR_INSIDE ) );

                if( poly.OutlineCount() == 0 )
                {
                    VECTOR2I padPos = pad->GetPosition();
                    wxLogTrace( traceBoardOutline, wxT( "Tested pad (%d, %d): outside" ),
                                padPos.x, padPos.y );
                    padOutside = true;
                }
            } );

        if( padOutside )
            break;

        VECTOR2I padPos = pad->GetPosition();
        wxLogTrace( traceBoardOutline, wxT( "Tested pad (%d, %d): not outside" ),
                    padPos.x, padPos.y );
    }

    return padOutside;
}


struct PCB_SHAPE_ENDPOINTS_ADAPTOR
{
    std::vector<std::pair<VECTOR2I, PCB_SHAPE*>> endpoints;

    PCB_SHAPE_ENDPOINTS_ADAPTOR( const std::vector<PCB_SHAPE*>& shapes )
    {
        endpoints.reserve( shapes.size() * 2 );

        for( PCB_SHAPE* shape : shapes )
        {
            endpoints.emplace_back( shape->GetStart(), shape );
            endpoints.emplace_back( shape->GetEnd(), shape );
        }
    }

    // Required by nanoflann
    size_t kdtree_get_point_count() const { return endpoints.size(); }

    // Returns the dim'th component of the idx'th point
    double kdtree_get_pt( const size_t idx, const size_t dim ) const
    {
        if( dim == 0 )
            return static_cast<double>( endpoints[idx].first.x );
        else
            return static_cast<double>( endpoints[idx].first.y );
    }

    template <class BBOX>
    bool kdtree_get_bbox( BBOX& ) const
    {
        return false;
    }
};

using KDTree = nanoflann::KDTreeSingleIndexAdaptor<nanoflann::L2_Simple_Adaptor<double, PCB_SHAPE_ENDPOINTS_ADAPTOR>,
                                                   PCB_SHAPE_ENDPOINTS_ADAPTOR,
                                                   2 /* dim */ >;

static void processClosedShape( PCB_SHAPE* aShape, SHAPE_LINE_CHAIN& aContour,
                                std::map<std::pair<VECTOR2I, VECTOR2I>, PCB_SHAPE*>& aShapeOwners,
                                int aErrorMax, bool aAllowUseArcsInPolygons )
{
    switch( aShape->GetShape() )
    {
    case SHAPE_T::POLY:
    {
        VECTOR2I prevPt;
        bool firstPt = true;

        for( auto it = aShape->GetPolyShape().CIterate(); it; it++ )
        {
            VECTOR2I pt = *it;
            aContour.Append( pt );

            if( firstPt )
                firstPt = false;
            else
                aShapeOwners[ std::make_pair( prevPt, pt ) ] = aShape;

            prevPt = pt;
        }

        aContour.SetClosed( true );
        break;
    }
    case SHAPE_T::CIRCLE:
    {
        VECTOR2I center = aShape->GetCenter();
        int      radius = aShape->GetRadius();
        VECTOR2I start = center;
        start.x += radius;

        SHAPE_ARC arc360( center, start, ANGLE_360, 0 );
        aContour.Append( arc360, aErrorMax );
        aContour.SetClosed( true );

        for( int ii = 1; ii < aContour.PointCount(); ++ii )
            aShapeOwners[ std::make_pair( aContour.CPoint( ii-1 ), aContour.CPoint( ii ) ) ] = aShape;

        if( !aAllowUseArcsInPolygons )
            aContour.ClearArcs();

        break;
    }
    case SHAPE_T::RECTANGLE:
    {
        if( aShape->GetCornerRadius() > 0 )
        {
            ROUNDRECT rr( SHAPE_RECT( aShape->GetStart(), aShape->GetRectangleWidth(), aShape->GetRectangleHeight() ),
                          aShape->GetCornerRadius(), true /* normalize */ );
            SHAPE_POLY_SET poly;
            rr.TransformToPolygon( poly, aShape->GetMaxError() );
            aContour.Append( poly.Outline( 0 ) );

            for( int ii = 1; ii < aContour.PointCount(); ++ii )
                aShapeOwners[ std::make_pair( aContour.CPoint( ii - 1 ), aContour.CPoint( ii ) ) ] = aShape;

            if( !aAllowUseArcsInPolygons )
                aContour.ClearArcs();

            aContour.SetClosed( true );
            break;
        }

        std::vector<VECTOR2I> pts = aShape->GetRectCorners();
        VECTOR2I prevPt;
        bool firstPt = true;

        for( const VECTOR2I& pt : pts )
        {
            aContour.Append( pt );

            if( firstPt )
                firstPt = false;
            else
                aShapeOwners[ std::make_pair( prevPt, pt ) ] = aShape;

            prevPt = pt;
        }

        aContour.SetClosed( true );
        break;
    }
    default:
        break;
    }
}

static void processShapeSegment( PCB_SHAPE* aShape, SHAPE_LINE_CHAIN& aContour,
                                VECTOR2I& aPrevPt,
                                std::map<std::pair<VECTOR2I, VECTOR2I>, PCB_SHAPE*>& aShapeOwners,
                                int aErrorMax, int aChainingEpsilon, bool aAllowUseArcsInPolygons )
{
    switch( aShape->GetShape() )
    {
    case SHAPE_T::SEGMENT:
    {
        VECTOR2I nextPt;

        if( closer_to_first( aPrevPt, aShape->GetStart(), aShape->GetEnd() ) )
            nextPt = aShape->GetEnd();
        else
            nextPt = aShape->GetStart();

        aContour.Append( nextPt );
        aShapeOwners[ std::make_pair( aPrevPt, nextPt ) ] = aShape;
        aPrevPt = nextPt;
        break;
    }
    case SHAPE_T::ARC:
    {
        VECTOR2I pstart = aShape->GetStart();
        VECTOR2I pmid = aShape->GetArcMid();
        VECTOR2I pend = aShape->GetEnd();

        if( !close_enough( aPrevPt, pstart, aChainingEpsilon ) )
        {
            wxASSERT( close_enough( aPrevPt, aShape->GetEnd(), aChainingEpsilon ) );
            std::swap( pstart, pend );
        }

        pstart = aPrevPt;
        SHAPE_ARC sarc( pstart, pmid, pend, 0 );
        SHAPE_LINE_CHAIN arcChain;
        arcChain.Append( sarc, aErrorMax );

        if( !aAllowUseArcsInPolygons )
            arcChain.ClearArcs();

        for( int ii = 1; ii < arcChain.PointCount(); ++ii )
        {
            aShapeOwners[ std::make_pair( arcChain.CPoint( ii - 1 ),
                                         arcChain.CPoint( ii ) ) ] = aShape;
        }

        aContour.Append( arcChain );
        aPrevPt = pend;
        break;
    }
    case SHAPE_T::BEZIER:
    {
        VECTOR2I nextPt;
        bool     reverse = false;

        if( closer_to_first( aPrevPt, aShape->GetStart(), aShape->GetEnd() ) )
        {
            nextPt = aShape->GetEnd();
        }
        else
        {
            nextPt = aShape->GetStart();
            reverse = true;
        }

        aShape->RebuildBezierToSegmentsPointsList( aErrorMax );

        if( reverse )
        {
            for( int jj = aShape->GetBezierPoints().size() - 1; jj >= 0; jj-- )
            {
                const VECTOR2I& pt = aShape->GetBezierPoints()[jj];

                if( aPrevPt == pt )
                    continue;

                aContour.Append( pt );
                aShapeOwners[ std::make_pair( aPrevPt, pt ) ] = aShape;
                aPrevPt = pt;
            }
        }
        else
        {
            for( const VECTOR2I& pt : aShape->GetBezierPoints() )
            {
                if( aPrevPt == pt )
                    continue;

                aContour.Append( pt );
                aShapeOwners[ std::make_pair( aPrevPt, pt ) ] = aShape;
                aPrevPt = pt;
            }
        }

        aPrevPt = nextPt;
        break;
    }
    default:
        break;
    }
}

static std::map<int, std::vector<int>> buildContourHierarchy( const std::vector<SHAPE_LINE_CHAIN>& aContours )
{
    std::map<int, std::vector<int>> contourToParentIndexesMap;

    for( size_t ii = 0; ii < aContours.size(); ++ii )
    {
        if( aContours[ii].PointCount() < 1 )  // malformed/empty SHAPE_LINE_CHAIN
            continue;

        VECTOR2I         firstPt = aContours[ii].GetPoint( 0 );
        std::vector<int> parents;

        for( size_t jj = 0; jj < aContours.size(); ++jj )
        {
            if( jj == ii )
                continue;

            const SHAPE_LINE_CHAIN& parentCandidate = aContours[jj];

            if( parentCandidate.PointInside( firstPt, 0, true ) )
                parents.push_back( jj );
        }

        contourToParentIndexesMap[ii] = std::move( parents );
    }

    return contourToParentIndexesMap;
}

static bool addOutlinesToPolygon( const std::vector<SHAPE_LINE_CHAIN>& aContours,
                                  const std::map<int, std::vector<int>>& aContourHierarchy,
                                  SHAPE_POLY_SET& aPolygons, bool aAllowDisjoint,
                                  OUTLINE_ERROR_HANDLER* aErrorHandler,
                                  const std::function<PCB_SHAPE*(const SEG&)>& aFetchOwner,
                                  std::map<int, int>& aContourToOutlineIdxMap )
{
    for( const auto& [ contourIndex, parentIndexes ] : aContourHierarchy )
    {
        if( parentIndexes.size() % 2 == 0 )
        {
            // Even number of parents; top-level outline
            if( !aAllowDisjoint && !aPolygons.IsEmpty() )
            {
                if( aErrorHandler )
                {
                    BOARD_ITEM* a = aFetchOwner( aPolygons.Outline( 0 ).GetSegment( 0 ) );
                    BOARD_ITEM* b = aFetchOwner( aContours[ contourIndex ].GetSegment( 0 ) );

                    if( a && b )
                    {
                        (*aErrorHandler)( _( "(multiple board outlines not supported)" ), a, b,
                                          aContours[ contourIndex ].GetPoint( 0 ) );
                        return false;
                    }
                }
            }

            aPolygons.AddOutline( aContours[ contourIndex ] );
            aContourToOutlineIdxMap[ contourIndex ] = aPolygons.OutlineCount() - 1;
        }
    }
    return true;
}

static void addHolesToPolygon( const std::vector<SHAPE_LINE_CHAIN>& aContours,
                               const std::map<int, std::vector<int>>& aContourHierarchy,
                               const std::map<int, int>& aContourToOutlineIdxMap,
                               SHAPE_POLY_SET& aPolygons )
{
    for( const auto& [ contourIndex, parentIndexes ] : aContourHierarchy )
    {
        if( parentIndexes.size() % 2 == 1 )
        {
            // Odd number of parents; we're a hole in the parent which has one fewer parents
            const SHAPE_LINE_CHAIN& hole = aContours[ contourIndex ];

            for( int parentContourIdx : parentIndexes )
            {
                if( aContourHierarchy.at( parentContourIdx ).size() == parentIndexes.size() - 1 )
                {
                    int outlineIdx = aContourToOutlineIdxMap.at( parentContourIdx );
                    aPolygons.AddHole( hole, outlineIdx );
                    break;
                }
            }
        }
    }
}

static bool checkSelfIntersections( SHAPE_POLY_SET& aPolygons,
                                   OUTLINE_ERROR_HANDLER* aErrorHandler,
                                   const std::function<PCB_SHAPE*(const SEG&)>& aFetchOwner )
{
    bool selfIntersecting = false;
    std::vector<SEG> segments;
    size_t total = 0;

    for( int ii = 0; ii < aPolygons.OutlineCount(); ++ii )
    {
        const SHAPE_LINE_CHAIN& contour = aPolygons.Outline( ii );
        total += contour.SegmentCount();

        for( int jj = 0; jj < aPolygons.HoleCount( ii ); ++jj )
        {
            const SHAPE_LINE_CHAIN& hole = aPolygons.Hole( ii, jj );
            total += hole.SegmentCount();
        }
    }

    segments.reserve( total );

    for( auto seg = aPolygons.IterateSegmentsWithHoles(); seg; seg++ )
    {
        SEG segment = *seg;

        if( LexicographicalCompare( segment.A, segment.B ) > 0 )
            std::swap( segment.A, segment.B );

        segments.push_back( segment );
    }

    std::sort( segments.begin(), segments.end(),
               []( const SEG& a, const SEG& b )
               {
                   if( a.A != b.A )
                       return LexicographicalCompare( a.A, b.A ) < 0;
                   return LexicographicalCompare( a.B, b.B ) < 0;
               } );

    for( size_t i = 0; i < segments.size(); ++i )
    {
        const SEG& seg1 = segments[i];

        for( size_t j = i + 1; j < segments.size(); ++j )
        {
            const SEG& seg2 = segments[j];

            if( seg2.A > seg1.B )
                break;

            if( seg1 == seg2 || ( seg1.A == seg2.B && seg1.B == seg2.A ) )
            {
                if( aErrorHandler )
                {
                    BOARD_ITEM* a = aFetchOwner( seg1 );
                    BOARD_ITEM* b = aFetchOwner( seg2 );
                    (*aErrorHandler)( _( "(self-intersecting)" ), a, b, seg1.A );
                }
                selfIntersecting = true;
            }
            else if( OPT_VECTOR2I pt = seg1.Intersect( seg2, true ) )
            {
                if( aErrorHandler )
                {
                    BOARD_ITEM* a = aFetchOwner( seg1 );
                    BOARD_ITEM* b = aFetchOwner( seg2 );
                    (*aErrorHandler)( _( "(self-intersecting)" ), a, b, *pt );
                }
                selfIntersecting = true;
            }
        }
    }

    return !selfIntersecting;
}

// Helper function to find next shape using KD-tree
static PCB_SHAPE* findNext( PCB_SHAPE* aShape, const VECTOR2I& aPoint, const KDTree& kdTree,
                            const PCB_SHAPE_ENDPOINTS_ADAPTOR& adaptor, double aChainingEpsilon )
{
    const double query_pt[2] = { static_cast<double>( aPoint.x ), static_cast<double>( aPoint.y ) };

    uint32_t indices[2];
    double distances[2];
    kdTree.knnSearch( query_pt, 2, indices, distances );

    if( distances[0] == std::numeric_limits<double>::max() )
        return nullptr;

    // Find the closest valid candidate
    PCB_SHAPE* closest_graphic = nullptr;
    double closest_dist_sq = aChainingEpsilon * aChainingEpsilon;

    for( size_t i = 0; i < 2; ++i )
    {
        if( distances[i] == std::numeric_limits<double>::max() )
            continue;

        PCB_SHAPE* candidate = adaptor.endpoints[indices[i]].second;

        if( candidate == aShape )
            continue;

        if( distances[i] < closest_dist_sq )
        {
            closest_dist_sq = distances[i];
            closest_graphic = candidate;
        }
    }

    return closest_graphic;
}

bool doConvertOutlineToPolygon( std::vector<PCB_SHAPE*>& aShapeList, SHAPE_POLY_SET& aPolygons,
                                int aErrorMax, int aChainingEpsilon, bool aAllowDisjoint,
                                OUTLINE_ERROR_HANDLER* aErrorHandler, bool aAllowUseArcsInPolygons,
                                SCOPED_FLAGS_CLEANER& aCleaner )
{
    if( aShapeList.size() == 0 )
        return true;

    bool       selfIntersecting = false;
    PCB_SHAPE* graphic = nullptr;

    std::set<PCB_SHAPE*> startCandidates( aShapeList.begin(), aShapeList.end() );

    // Pre-build KD-tree
    PCB_SHAPE_ENDPOINTS_ADAPTOR adaptor( aShapeList );
    KDTree                      kdTree( 2, adaptor );

    // Keep a list of where the various shapes came from
    std::map<std::pair<VECTOR2I, VECTOR2I>, PCB_SHAPE*> shapeOwners;

    auto fetchOwner =
            [&]( const SEG& seg ) -> PCB_SHAPE*
            {
                auto it = shapeOwners.find( std::make_pair( seg.A, seg.B ) );
                return it == shapeOwners.end() ? nullptr : it->second;
            };

    std::set<std::pair<PCB_SHAPE*, PCB_SHAPE*>> reportedGaps;
    std::vector<SHAPE_LINE_CHAIN> contours;
    contours.reserve( startCandidates.size() );

    for( PCB_SHAPE* shape : startCandidates )
        shape->ClearFlags( SKIP_STRUCT );

    // Process each shape to build contours
    while( startCandidates.size() )
    {
        graphic = *startCandidates.begin();
        graphic->SetFlags( SKIP_STRUCT );
        aCleaner.insert( graphic );
        startCandidates.erase( startCandidates.begin() );

        contours.emplace_back();
        SHAPE_LINE_CHAIN& currContour = contours.back();
        currContour.SetWidth( graphic->GetWidth() );

        // Handle closed shapes (circles, rects, polygons)
        if( graphic->GetShape() == SHAPE_T::POLY || graphic->GetShape() == SHAPE_T::CIRCLE
            || graphic->GetShape() == SHAPE_T::RECTANGLE )
        {
            processClosedShape( graphic, currContour, shapeOwners, aErrorMax, aAllowUseArcsInPolygons );
        }
        else
        {
            // Build chains for open shapes
            std::deque<PCB_SHAPE*> chain;
            chain.push_back( graphic );

            bool     closed = false;
            VECTOR2I frontPt = graphic->GetStart();
            VECTOR2I backPt = graphic->GetEnd();

            auto extendChain = [&]( bool forward )
            {
                PCB_SHAPE* curr = forward ? chain.back() : chain.front();
                VECTOR2I   prev = forward ? backPt : frontPt;

                for( ;; )
                {
                    PCB_SHAPE* next = findNext( curr, prev, kdTree, adaptor, aChainingEpsilon );

                    if( next && !( next->GetFlags() & SKIP_STRUCT ) )
                    {
                        next->SetFlags( SKIP_STRUCT );
                        aCleaner.insert( next );
                        startCandidates.erase( next );

                        if( forward )
                            chain.push_back( next );
                        else
                            chain.push_front( next );

                        if( closer_to_first( prev, next->GetStart(), next->GetEnd() ) )
                            prev = next->GetEnd();
                        else
                            prev = next->GetStart();

                        curr = next;
                        continue;
                    }

                    if( next )
                    {
                        PCB_SHAPE* chainEnd = forward ? chain.front() : chain.back();
                        VECTOR2I   chainPt = forward ? frontPt : backPt;

                        if( next == chainEnd && close_enough( prev, chainPt, aChainingEpsilon ) )
                        {
                            closed = true;
                        }
                        else
                        {
                            if( aErrorHandler )
                                ( *aErrorHandler )( _( "(self-intersecting)" ), curr, next, prev );

                            selfIntersecting = true;
                        }
                    }

                    if( forward )
                        backPt = prev;
                    else
                        frontPt = prev;

                    break;
                }
            };

            extendChain( true );

            if( !closed )
                extendChain( false );

            // Process the chain to build the contour
            PCB_SHAPE* first = chain.front();
            VECTOR2I   startPt;

            if( chain.size() > 1 )
            {
                PCB_SHAPE* second = *( std::next( chain.begin() ) );

                if( close_enough( first->GetStart(), second->GetStart(), aChainingEpsilon )
                        || close_enough( first->GetStart(), second->GetEnd(), aChainingEpsilon ) )
                    startPt = first->GetEnd();
                else
                    startPt = first->GetStart();
            }
            else
            {
                startPt = first->GetStart();
            }

            currContour.Append( startPt );
            VECTOR2I prevPt = startPt;

            for( PCB_SHAPE* shapeInChain : chain )
            {
                processShapeSegment( shapeInChain, currContour, prevPt, shapeOwners,
                                   aErrorMax, aChainingEpsilon, aAllowUseArcsInPolygons );
            }

            // Handle contour closure
            if( close_enough( currContour.CPoint( 0 ), currContour.CLastPoint(), aChainingEpsilon ) )
            {
                if( currContour.CPoint( 0 ) != currContour.CLastPoint() && currContour.PointCount() > 2 )
                {
                    PCB_SHAPE* owner = fetchOwner( currContour.CSegment( -1 ) );

                    if( currContour.IsArcEnd( currContour.PointCount() - 1 ) )
                    {
                        SHAPE_ARC arc = currContour.Arc( currContour.ArcIndex( currContour.PointCount() - 1 ) );

                        SHAPE_ARC sarc( arc.GetP0(), arc.GetArcMid(), currContour.CPoint( 0 ), 0 );

                        SHAPE_LINE_CHAIN arcChain;
                        arcChain.Append( sarc, aErrorMax );

                        if( !aAllowUseArcsInPolygons )
                            arcChain.ClearArcs();

                        for( int ii = 1; ii < arcChain.PointCount(); ++ii )
                            shapeOwners[std::make_pair( arcChain.CPoint( ii - 1 ), arcChain.CPoint( ii ) )] = owner;

                        currContour.RemoveShape( currContour.PointCount() - 1 );
                        currContour.Append( arcChain );
                    }
                    else
                    {
                        currContour.SetPoint( -1, currContour.CPoint( 0 ) );

                        shapeOwners[ std::make_pair( currContour.CPoints()[currContour.PointCount() - 2],
                                                     currContour.CLastPoint() ) ] = owner;
                    }
                }

                currContour.SetClosed( true );
            }
            else
            {
                auto report_gap = [&]( const VECTOR2I& pt )
                {
                    if( !aErrorHandler )
                        return;

                    const double query_pt[2] = { static_cast<double>( pt.x ), static_cast<double>( pt.y ) };
                    uint32_t    indices[2] = { 0, 0 };      // make gcc quiet
                    double      dists[2];

                    // Find the two closest items to the given point using kdtree
                    kdTree.knnSearch( query_pt, 2, indices, dists );

                    PCB_SHAPE* shapeA = adaptor.endpoints[indices[0]].second;
                    PCB_SHAPE* shapeB = adaptor.endpoints[indices[1]].second;

                    // Avoid reporting the same pair twice
                    auto key = std::minmax( shapeA, shapeB );

                    if( !reportedGaps.insert( key ).second )
                        return;

                    // Find the nearest points between the two shapes and calculate midpoint
                    std::shared_ptr<SHAPE> effectiveShapeA = shapeA->GetEffectiveShape();
                    std::shared_ptr<SHAPE> effectiveShapeB = shapeB->GetEffectiveShape();
                    VECTOR2I               ptA, ptB;
                    VECTOR2I               midpoint = pt; // fallback to original point

                    if( effectiveShapeA && effectiveShapeB
                        && effectiveShapeA->NearestPoints( effectiveShapeB.get(), ptA, ptB ) )
                    {
                        midpoint = ( ptA + ptB ) / 2;
                    }

                    ( *aErrorHandler )( _( "(not a closed shape)" ), shapeA, shapeB, midpoint );
                };

                report_gap( currContour.CPoint( 0 ) );
                report_gap( currContour.CLastPoint() );
            }
        }
    }

    // Ensure all contours are closed
    for( const SHAPE_LINE_CHAIN& contour : contours )
    {
        if( !contour.IsClosed() )
            return false;
    }

    // Generate bounding boxes for hierarchy calculations
    for( size_t ii = 0; ii < contours.size(); ++ii )
    {
        SHAPE_LINE_CHAIN& contour = contours[ii];

        if( !contour.GetCachedBBox()->IsValid() )
            contour.GenerateBBoxCache();
    }

    // Build contour hierarchy
    auto contourHierarchy = buildContourHierarchy( contours );

    // Add outlines to polygon set
    std::map<int, int> contourToOutlineIdxMap;
    if( !addOutlinesToPolygon( contours, contourHierarchy, aPolygons, aAllowDisjoint,
                               aErrorHandler, fetchOwner, contourToOutlineIdxMap ) )
    {
        return false;
    }

    // Add holes to polygon set
    addHolesToPolygon( contours, contourHierarchy, contourToOutlineIdxMap, aPolygons );

    // Check for self-intersections
    return checkSelfIntersections( aPolygons, aErrorHandler, fetchOwner );
}


bool ConvertOutlineToPolygon( std::vector<PCB_SHAPE*>& aShapeList, SHAPE_POLY_SET& aPolygons,
                              int aErrorMax, int aChainingEpsilon, bool aAllowDisjoint,
                              OUTLINE_ERROR_HANDLER* aErrorHandler, bool aAllowUseArcsInPolygons )
{
    SCOPED_FLAGS_CLEANER cleaner( SKIP_STRUCT );

    return doConvertOutlineToPolygon( aShapeList, aPolygons, aErrorMax, aChainingEpsilon,
                                      aAllowDisjoint, aErrorHandler, aAllowUseArcsInPolygons,
                                      cleaner );
}


bool TestBoardOutlinesGraphicItems( BOARD* aBoard, int aMinDist,
                                    OUTLINE_ERROR_HANDLER* aErrorHandler )
{
    bool                success = true;
    PCB_TYPE_COLLECTOR  items;
    int                 min_dist = std::max( 0, aMinDist );

    // Get all the shapes into 'items', then keep only those on layer == Edge_Cuts.
    items.Collect( aBoard, { PCB_SHAPE_T } );

    std::vector<PCB_SHAPE*> shapeList;

    for( int ii = 0; ii < items.GetCount(); ii++ )
    {
        PCB_SHAPE* seg = static_cast<PCB_SHAPE*>( items[ii] );

        if( seg->GetLayer() == Edge_Cuts )
            shapeList.push_back( seg );
    }

    // Now Test validity of collected items
    for( PCB_SHAPE* shape : shapeList )
    {
        switch( shape->GetShape() )
        {
        case SHAPE_T::RECTANGLE:
        {
            VECTOR2I seg = shape->GetEnd() - shape->GetStart();
            int dim = seg.EuclideanNorm();

            if( dim <= min_dist )
            {
                success = false;

                if( aErrorHandler )
                {
                    (*aErrorHandler)( wxString::Format( _( "(rectangle has null or very small "
                                                           "size: %d nm)" ), dim ),
                                      shape, nullptr, shape->GetStart() );
                }
            }
            break;
        }

        case SHAPE_T::CIRCLE:
        {
            int r = shape->GetRadius();

            if( r <= min_dist )
            {
                success = false;

                if( aErrorHandler )
                {
                    (*aErrorHandler)( wxString::Format( _( "(circle has null or very small "
                                                           "radius: %d nm)" ), r ),
                                      shape, nullptr, shape->GetStart() );
                }
            }
            break;
        }

        case SHAPE_T::SEGMENT:
        {
            VECTOR2I seg = shape->GetEnd() - shape->GetStart();
            int dim = seg.EuclideanNorm();

            if( dim <= min_dist )
            {
                success = false;

                if( aErrorHandler )
                {
                    (*aErrorHandler)( wxString::Format( _( "(segment has null or very small "
                                                           "length: %d nm)" ), dim ),
                                      shape, nullptr, shape->GetStart() );
                }
            }
            break;
            }

        case SHAPE_T::ARC:
        {
            // Arc size can be evaluated from the distance between arc middle point and arc ends
            // We do not need a precise value, just an idea of its size
            VECTOR2I arcMiddle = shape->GetArcMid();
            VECTOR2I seg1 = arcMiddle - shape->GetStart();
            VECTOR2I seg2 = shape->GetEnd() - arcMiddle;
            int dim = seg1.EuclideanNorm() + seg2.EuclideanNorm();

            if( dim <= min_dist )
            {
                success = false;

                if( aErrorHandler )
                {
                    (*aErrorHandler)( wxString::Format( _( "(arc has null or very small size: "
                                                           "%d nm)" ), dim ),
                                      shape, nullptr, shape->GetStart() );
                }
            }
            break;
            }

        case SHAPE_T::POLY:
            break;

        case SHAPE_T::BEZIER:
            break;

        default:
            UNIMPLEMENTED_FOR( shape->SHAPE_T_asString() );
            return false;
        }
    }

    return success;
}


bool BuildBoardPolygonOutlines( BOARD* aBoard, SHAPE_POLY_SET& aOutlines, int aErrorMax,
                                int aChainingEpsilon, bool aInferOutlineIfNecessary,
                                OUTLINE_ERROR_HANDLER* aErrorHandler, bool aAllowUseArcsInPolygons )
{
    PCB_TYPE_COLLECTOR items;
    SHAPE_POLY_SET     fpHoles;
    bool               success = false;

    SCOPED_FLAGS_CLEANER cleaner( SKIP_STRUCT );

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
            success = doConvertOutlineToPolygon( fpSegList, fpOutlines, aErrorMax, aChainingEpsilon,
                                                 false,
                                                 nullptr, // don't report errors here; the second pass also
                                                          // gets an opportunity to use these segments
                                                 aAllowUseArcsInPolygons,
                                                 cleaner );

            // Test to see if we should make holes or outlines.  Holes are made if the footprint
            // has copper outside of a single, closed outline.  If there are multiple outlines,
            // we assume that the footprint edges represent holes as we do not support multiple
            // boards.  Similarly, if any of the footprint pads are located outside of the edges,
            // then the edges are holes
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
        success = doConvertOutlineToPolygon( segList, aOutlines, aErrorMax, aChainingEpsilon, true,
                                             aErrorHandler, aAllowUseArcsInPolygons, cleaner );
    }

    if( ( !success || !aOutlines.OutlineCount() ) && aInferOutlineIfNecessary )
    {
        // Couldn't create a valid polygon outline.  Use the board edge cuts bounding box to
        // create a rectangular outline, or, failing that, the bounding box of the items on
        // the board.
        BOX2I bbbox = aBoard->GetBoardEdgesBoundingBox();

        // If null area, uses the global bounding box.
        if( ( bbbox.GetWidth() ) == 0 || ( bbbox.GetHeight() == 0 ) )
            bbbox = aBoard->ComputeBoundingBox( false );

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

    if( aAllowUseArcsInPolygons )
    {
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
    }
    else
    {
        aOutlines.BooleanSubtract( fpHoles );
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
        bbbox = aBoard->ComputeBoundingBox( false );

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

    PCB_TYPE_COLLECTOR items;
    SHAPE_POLY_SET     outlines;
    bool               success = false;

    SCOPED_FLAGS_CLEANER cleaner( SKIP_STRUCT );

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
        success = doConvertOutlineToPolygon( segList, outlines, aErrorMax, aChainingEpsilon, true,
                                             aErrorHandler, false, cleaner );
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
            aOutlines = std::move( outlines );
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
            aOutlines = std::move( bbox );
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
                wxLogTrace( traceBoardOutline, wxT( "Segment intersects only vertical bbox sides" ) );

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
                wxLogTrace( traceBoardOutline, wxT( "Segment intersects only horizontal bbox sides" ) );

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
                wxLogTrace( traceBoardOutline, wxT( "Segment intersects two perpendicular bbox sides" ) );

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
            aOutlines = std::move( bbox );
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
            aOutlines = std::move( poly2 );
        }
        else
        {
            wxLogTrace( traceBoardOutline, wxT( "Using upper shape" ) );
            aOutlines = std::move( poly1 );
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
