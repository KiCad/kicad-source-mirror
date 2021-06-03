/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2013-2014 CERN
 * Copyright (C) 2016 KiCad Developers, see AUTHORS.txt for contributors.
 * Author: Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "pns_utils.h"
#include "pns_line.h"
#include "pns_via.h"
#include "pns_router.h"

#include <geometry/shape_arc.h>
#include <geometry/shape_segment.h>
#include <math/box2.h>

#include <cmath>

namespace PNS {

const SHAPE_LINE_CHAIN OctagonalHull( const VECTOR2I& aP0, const VECTOR2I& aSize,
                                      int aClearance, int aChamfer )
{
    SHAPE_LINE_CHAIN s;

    s.SetClosed( true );

    s.Append( aP0.x - aClearance, aP0.y - aClearance + aChamfer );
    s.Append( aP0.x - aClearance + aChamfer, aP0.y - aClearance );
    s.Append( aP0.x + aSize.x + aClearance - aChamfer, aP0.y - aClearance );
    s.Append( aP0.x + aSize.x + aClearance, aP0.y - aClearance + aChamfer );
    s.Append( aP0.x + aSize.x + aClearance, aP0.y + aSize.y + aClearance - aChamfer );
    s.Append( aP0.x + aSize.x + aClearance - aChamfer, aP0.y + aSize.y + aClearance );
    s.Append( aP0.x - aClearance + aChamfer, aP0.y + aSize.y + aClearance );
    s.Append( aP0.x - aClearance, aP0.y + aSize.y + aClearance - aChamfer );

    return s;
}


const SHAPE_LINE_CHAIN ArcHull( const SHAPE_ARC& aSeg, int aClearance,
            int aWalkaroundThickness )
{
    int d = aSeg.GetWidth() / 2 + aClearance + aWalkaroundThickness / 2 + HULL_MARGIN;
    int x = (int)( 2.0 / ( 1.0 + M_SQRT2 ) * d ) / 2;

    auto line = aSeg.ConvertToPolyline();

    SHAPE_LINE_CHAIN s;
    s.SetClosed( true );
    std::vector<VECTOR2I> reverse_line;

    auto seg = line.Segment( 0 );
    VECTOR2I dir = seg.B - seg.A;
    VECTOR2I p0 = dir.Perpendicular().Resize( d );
    VECTOR2I ds = dir.Perpendicular().Resize( x );
    VECTOR2I pd = dir.Resize( x );
    VECTOR2I dp = dir.Resize( d );

    // Append the first curve
    s.Append( seg.A + p0 - pd );
    s.Append( seg.A - dp + ds );
    s.Append( seg.A - dp - ds );
    s.Append( seg.A - p0 - pd );

    for( int i = 1; i < line.SegmentCount(); i++ )
    {
        auto old_seg = seg;
        auto endpt = ( old_seg.A - old_seg.B ).Resize( seg.Length() );
        old_seg.A = old_seg.B + endpt;

        seg = line.Segment( i );
        auto dir2 = old_seg.A - seg.B;

        p0 = dir2.Perpendicular().Resize( d );
        s.Append( seg.A - p0 );
        reverse_line.push_back( seg.A + p0 );
    }

    pd = dir.Resize( x );
    dp = dir.Resize( d );
    s.Append( seg.B - p0 + pd );
    s.Append( seg.B + dp - ds );
    s.Append( seg.B + dp + ds );
    s.Append( seg.B + p0 + pd );

    for( int i = reverse_line.size() - 1; i >= 0; i-- )
        s.Append( reverse_line[i] );

    // make sure the hull outline is always clockwise
    if( s.CSegment( 0 ).Side( line.Segment( 0 ).A ) < 0 )
        return s.Reverse();
    else
        return s;
}


const SHAPE_LINE_CHAIN SegmentHull ( const SHAPE_SEGMENT& aSeg, int aClearance,
                                     int aWalkaroundThickness )
{
    int cl = aClearance + aWalkaroundThickness / 2 + HULL_MARGIN;
    int d = aSeg.GetWidth() / 2 + cl;
    int x = (int)( 2.0 / ( 1.0 + M_SQRT2 ) * d );

    const VECTOR2I a = aSeg.GetSeg().A;
    const VECTOR2I b = aSeg.GetSeg().B;

    if( a == b )
    {
        return OctagonalHull( a - VECTOR2I( aSeg.GetWidth() / 2, aSeg.GetWidth() / 2 ),
                              VECTOR2I( aSeg.GetWidth(), aSeg.GetWidth() ),
                              cl + 1,
                              0.52 * d );
    }

    VECTOR2I dir = b - a;
    VECTOR2I p0 = dir.Perpendicular().Resize( d );
    VECTOR2I ds = dir.Perpendicular().Resize( x / 2 );
    VECTOR2I pd = dir.Resize( x / 2 );
    VECTOR2I dp = dir.Resize( d );

    SHAPE_LINE_CHAIN s;

    s.SetClosed( true );

    s.Append( b + p0 + pd );
    s.Append( b + dp + ds );
    s.Append( b + dp - ds );
    s.Append( b - p0 + pd );
    s.Append( a - p0 - pd );
    s.Append( a - dp - ds );
    s.Append( a - dp + ds );
    s.Append( a + p0 - pd );

    // make sure the hull outline is always clockwise
    if( s.CSegment( 0 ).Side( a ) < 0 )
        return s.Reverse();
    else
        return s;
}


static void MoveDiagonal( SEG& aDiagonal, const SHAPE_LINE_CHAIN& aVertices, int aClearance )
{
    int dist;

    aVertices.NearestPoint( aDiagonal, dist );
    dist -= HULL_MARGIN;
    VECTOR2I moveBy = ( aDiagonal.A - aDiagonal.B ).Perpendicular().Resize( dist - aClearance );
    aDiagonal.A += moveBy;
    aDiagonal.B += moveBy;
}


const SHAPE_LINE_CHAIN ConvexHull( const SHAPE_SIMPLE& aConvex, int aClearance )
{
    // this defines the horizontal and vertical lines in the hull octagon
    BOX2I box = aConvex.BBox( aClearance + HULL_MARGIN );
    box.Normalize();

    SEG topline = SEG( VECTOR2I( box.GetX(), box.GetY() + box.GetHeight() ),
                       VECTOR2I( box.GetX() + box.GetWidth(), box.GetY() + box.GetHeight() ) );
    SEG rightline = SEG( VECTOR2I( box.GetX() + box.GetWidth(), box.GetY() + box.GetHeight() ),
                         VECTOR2I( box.GetX() + box.GetWidth(), box.GetY() ) );
    SEG bottomline = SEG( VECTOR2I( box.GetX() + box.GetWidth(), box.GetY() ),
             box.GetOrigin() );
    SEG leftline = SEG( box.GetOrigin(), VECTOR2I( box.GetX(), box.GetY() + box.GetHeight() ) );

    const SHAPE_LINE_CHAIN& vertices = aConvex.Vertices();

    // top right diagonal
    VECTOR2I corner = box.GetOrigin() + box.GetSize();
    SEG toprightline = SEG( corner,
                            corner + VECTOR2I( box.GetHeight(), -box.GetHeight() ) );
    MoveDiagonal( toprightline, vertices, aClearance );

    // bottom right diagonal
    corner = box.GetOrigin() + VECTOR2I( box.GetWidth(), 0 );
    SEG bottomrightline = SEG( corner + VECTOR2I( box.GetHeight(), box.GetHeight() ),
                               corner );
    MoveDiagonal( bottomrightline, vertices, aClearance );

    // bottom left diagonal
    corner = box.GetOrigin();
    SEG bottomleftline = SEG( corner,
                              corner + VECTOR2I( -box.GetHeight(), box.GetHeight() ) );
    MoveDiagonal( bottomleftline, vertices, aClearance );

    // top left diagonal
    corner = box.GetOrigin() + VECTOR2I( 0, box.GetHeight() );
    SEG topleftline = SEG( corner + VECTOR2I( -box.GetHeight(), -box.GetHeight() ),
                           corner );
    MoveDiagonal( topleftline, vertices, aClearance );

    SHAPE_LINE_CHAIN octagon;
    octagon.SetClosed( true );

    octagon.Append( *leftline.IntersectLines( bottomleftline ) );
    octagon.Append( *bottomline.IntersectLines( bottomleftline ) );
    octagon.Append( *bottomline.IntersectLines( bottomrightline ) );
    octagon.Append( *rightline.IntersectLines( bottomrightline ) );
    octagon.Append( *rightline.IntersectLines( toprightline ) );
    octagon.Append( *topline.IntersectLines( toprightline ) );
    octagon.Append( *topline.IntersectLines( topleftline ) );
    octagon.Append( *leftline.IntersectLines( topleftline ) );

    return octagon;
}


SHAPE_RECT ApproximateSegmentAsRect( const SHAPE_SEGMENT& aSeg )
{
    SHAPE_RECT r;

    VECTOR2I delta( aSeg.GetWidth() / 2, aSeg.GetWidth() / 2 );
    VECTOR2I p0( aSeg.GetSeg().A - delta );
    VECTOR2I p1( aSeg.GetSeg().B + delta );

    return SHAPE_RECT( std::min( p0.x, p1.x ), std::min( p0.y, p1.y ),
                       std::abs( p1.x - p0.x ), std::abs( p1.y - p0.y ) );
}


OPT_BOX2I ChangedArea( const ITEM* aItemA, const ITEM* aItemB )
{
    if( aItemA->OfKind( ITEM::VIA_T ) && aItemB->OfKind( ITEM::VIA_T ) )
    {
        const VIA* va = static_cast<const VIA*>( aItemA );
        const VIA* vb = static_cast<const VIA*>( aItemB );

        return va->ChangedArea( vb );
    }
    else if( aItemA->OfKind( ITEM::LINE_T ) && aItemB->OfKind( ITEM::LINE_T ) )
    {
        const LINE* la = static_cast<const LINE*> ( aItemA );
        const LINE* lb = static_cast<const LINE*> ( aItemB );

        return la->ChangedArea( lb );
    }

    return OPT_BOX2I();
}

OPT_BOX2I ChangedArea( const LINE& aLineA, const LINE& aLineB )
{
    return aLineA.ChangedArea( &aLineB );
}


void HullIntersection( const SHAPE_LINE_CHAIN& hull, const SHAPE_LINE_CHAIN& line,
                       SHAPE_LINE_CHAIN::INTERSECTIONS& ips )
{
    SHAPE_LINE_CHAIN::INTERSECTIONS ips_raw;

    if( line.PointCount() < 2 )
        return;

    hull.Intersect( line, ips_raw );

    for( auto& p : ips_raw )
    {
        SHAPE_LINE_CHAIN::INTERSECTION ipp;

        SEG      d1[2];
        VECTOR2I d2[2];
        int      d1_idx = 0, d2_idx = 0;

        ipp = p;
        ipp.valid = false;

        if( !p.is_corner_our && !p.is_corner_their )
        {
            ipp.valid = true;
            ips.push_back( ipp );
            continue;
        }

        if( p.index_our >= hull.SegmentCount() )
            p.index_our -= hull.SegmentCount();

        if( p.is_corner_our )
        {
            d1[0] = hull.CSegment( p.index_our );
            d1[1] = hull.CSegment( p.index_our - 1 );
            d1_idx = 2;
        }
        else
        {
            d1[0] = hull.CSegment( p.index_our );
            d1_idx = 1;
        }

        if( p.is_corner_their )
        {
            if( p.index_their > 0 )
            {
                d2[d2_idx++] = line.CSegment( p.index_their - 1 ).A;
            }
            if( p.index_their < line.PointCount() - 1 )
            {
                d2[d2_idx++] = line.CSegment( p.index_their ).B;
            }
        }
        else
        {
            d2[d2_idx++] = line.CSegment( p.index_their ).A;
            d2[d2_idx++] = line.CSegment( p.index_their ).B;
        }

        for( int i = 0; i < d1_idx; i++ )
        {
            for( int j = 0; j < d2_idx; j++ )
            {
                if( d1[i].Side( d2[j] ) > 0 )
                {
                    ipp.valid = true;
                }
            }
        }

#ifdef TOM_EXTRA_DEBUG
        printf("p %d %d hi %d their %d co %d ct %d ipv %d\n", p.p.x, p.p.y, p.index_our, p.index_their, p.is_corner_our?1:0, p.is_corner_their?1:0, ipp.valid ?1:0);
        printf("d1 %d d2 %d\n", d1_idx, d2_idx );
#endif
        if( ipp.valid )
        {
            ips.push_back( ipp );
        }
    }
}

}
