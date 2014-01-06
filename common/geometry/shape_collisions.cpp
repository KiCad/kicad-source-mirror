/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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

#include <math/vector2d.h>

#include <geometry/shape.h>
#include <geometry/shape_line_chain.h>
#include <geometry/shape_circle.h>
#include <geometry/shape_rect.h>

typedef VECTOR2I::extended_type ecoord;

static inline bool Collide( const SHAPE_CIRCLE& aA, const SHAPE_CIRCLE& aB, int aClearance,
                            bool aNeedMTV, VECTOR2I& aMTV )
{
    ecoord min_dist = aClearance + aA.GetRadius() + aB.GetRadius();
    ecoord min_dist_sq = min_dist * min_dist;

    const VECTOR2I delta = aB.GetCenter() - aA.GetCenter();

    ecoord dist_sq = delta.SquaredEuclideanNorm();

    if( dist_sq >= min_dist_sq )
        return false;

    if( aNeedMTV )
        aMTV = delta.Resize( sqrt( abs( min_dist_sq - dist_sq ) ) + 1 );

    return true;
}


static inline  bool Collide( const SHAPE_RECT& aA, const SHAPE_CIRCLE& aB, int aClearance,
                             bool aNeedMTV, VECTOR2I& aMTV )
{
    const VECTOR2I c = aB.GetCenter();
    const VECTOR2I p0 = aA.GetPosition();
    const VECTOR2I size = aA.GetSize();
    const ecoord r = aB.GetRadius();
    const ecoord min_dist = aClearance + r;
    const ecoord min_dist_sq = min_dist * min_dist;

    if( aA.BBox( 0 ).Contains( c ) )
        return true;

    const VECTOR2I vts[] =
    {
        VECTOR2I( p0.x,          p0.y ),
        VECTOR2I( p0.x,          p0.y + size.y ),
        VECTOR2I( p0.x + size.x, p0.y + size.y ),
        VECTOR2I( p0.x + size.x, p0.y ),
        VECTOR2I( p0.x,          p0.y )
    };

    ecoord nearest_seg_dist_sq = VECTOR2I::ECOORD_MAX;
    VECTOR2I nearest;

    bool inside = c.x >= p0.x && c.x <= ( p0.x + size.x )
                  && c.y >= p0.y && c.y <= ( p0.y + size.y );

    if( !inside )
    {
        for( int i = 0; i < 4; i++ )
        {
            const SEG seg( vts[i], vts[i + 1] );
            ecoord dist_sq = seg.SquaredDistance( c );

            if( dist_sq < min_dist_sq )
            {
                if( !aNeedMTV )
                    return true;
                else
                {
                    nearest = seg.NearestPoint( c );
                    nearest_seg_dist_sq = dist_sq;
                }
            }
        }
    }

    if( nearest_seg_dist_sq >= min_dist_sq && !inside )
        return false;

    VECTOR2I delta = c - nearest;

    if( !aNeedMTV )
        return true;

    if( inside )
        aMTV = -delta.Resize( sqrt( abs( r * r + nearest_seg_dist_sq ) + 1 ) );
    else
        aMTV = delta.Resize( sqrt( abs( r * r - nearest_seg_dist_sq ) + 1 ) );

    return true;
}


static inline bool Collide( const SHAPE_CIRCLE& aA, const SHAPE_LINE_CHAIN& aB, int aClearance,
                            bool aNeedMTV, VECTOR2I& aMTV )
{
    for( int s = 0; s < aB.SegmentCount(); s++ )
    {
        if( aA.Collide( aB.CSegment( s ), aClearance ) )
            return true;
    }

    return false;
}


static inline bool Collide( const SHAPE_LINE_CHAIN& aA, const SHAPE_LINE_CHAIN& aB, int aClearance,
                            bool aNeedMTV, VECTOR2I& aMTV )
{
    for( int i = 0; i < aB.SegmentCount(); i++ )
        if( aA.Collide( aB.CSegment( i ), aClearance ) )
            return true;

    return false;
}


static inline bool Collide( const SHAPE_RECT& aA, const SHAPE_LINE_CHAIN& aB, int aClearance,
                            bool aNeedMTV, VECTOR2I& aMTV )
{
    for( int s = 0; s < aB.SegmentCount(); s++ )
    {
        SEG seg = aB.CSegment( s );

        if( aA.Collide( seg, aClearance ) )
            return true;
    }

    return false;
}


bool CollideShapes( const SHAPE* aA, const SHAPE* aB, int aClearance,
                    bool aNeedMTV, VECTOR2I& aMTV )
{
    switch( aA->Type() )
    {
    case SH_RECT:
        switch( aB->Type() )
        {
        case SH_CIRCLE:
            return Collide( *static_cast<const SHAPE_RECT*>( aA ),
                *static_cast<const SHAPE_CIRCLE*>( aB ), aClearance, aNeedMTV, aMTV );

        case SH_LINE_CHAIN:
            return Collide( *static_cast<const SHAPE_RECT*>( aA ),
                *static_cast<const SHAPE_LINE_CHAIN*>( aB ), aClearance, aNeedMTV, aMTV );

        default:
            break;
        }
        break;

    case SH_CIRCLE:
        switch( aB->Type() )
        {
        case SH_RECT:
            return Collide( *static_cast<const SHAPE_RECT*>( aB ),
                *static_cast<const SHAPE_CIRCLE*>( aA ), aClearance, aNeedMTV, aMTV );

        case SH_CIRCLE:
            return Collide( *static_cast<const SHAPE_CIRCLE*>( aA ),
                *static_cast<const SHAPE_CIRCLE*>( aB ), aClearance, aNeedMTV, aMTV );

        case SH_LINE_CHAIN:
            return Collide( *static_cast<const SHAPE_CIRCLE*>( aA ),
                *static_cast<const SHAPE_LINE_CHAIN *>( aB ), aClearance, aNeedMTV, aMTV );

        default:
            break;
        }
        break;

    case SH_LINE_CHAIN:
        switch( aB->Type() )
        {
        case SH_RECT:
            return Collide( *static_cast<const SHAPE_RECT*>( aB ),
                *static_cast<const SHAPE_LINE_CHAIN*>( aA ), aClearance, aNeedMTV, aMTV );

        case SH_CIRCLE:
            return Collide( *static_cast<const SHAPE_CIRCLE*>( aB ),
                *static_cast<const SHAPE_LINE_CHAIN*>( aA ), aClearance, aNeedMTV, aMTV );

        case SH_LINE_CHAIN:
            return Collide( *static_cast<const SHAPE_LINE_CHAIN*>( aA ),
                *static_cast<const SHAPE_LINE_CHAIN*>( aB ), aClearance, aNeedMTV, aMTV );

        default:
            break;
        }
        break;

    default:
        break;
    }

    assert( 0 );    // unsupported_collision

    return false;
}


bool SHAPE::Collide( const SHAPE* aShape, int aClerance, VECTOR2I& aMTV ) const
{
    return CollideShapes( this, aShape, aClerance, true, aMTV );
}


bool SHAPE::Collide( const SHAPE* aShape, int aClerance ) const
{
    VECTOR2I dummy;

    return CollideShapes( this, aShape, aClerance, false, dummy );
}
