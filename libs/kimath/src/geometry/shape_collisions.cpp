/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <cmath>
#include <limits>

#include <geometry/seg.h>                         // for SEG
#include <geometry/shape.h>
#include <geometry/shape_arc.h>
#include <geometry/shape_line_chain.h>
#include <geometry/shape_circle.h>
#include <geometry/shape_rect.h>
#include <geometry/shape_segment.h>
#include <geometry/shape_compound.h>
#include <geometry/shape_poly_set.h>
#include <math/vector2d.h>

typedef VECTOR2I::extended_type ecoord;


static inline bool Collide( const SHAPE_CIRCLE& aA, const SHAPE_CIRCLE& aB, int aClearance,
                            int* aActual, VECTOR2I* aLocation, VECTOR2I* aMTV )
{
    ecoord min_dist = aClearance + aA.GetRadius() + aB.GetRadius();
    ecoord min_dist_sq = min_dist * min_dist;

    const VECTOR2I delta = aB.GetCenter() - aA.GetCenter();
    ecoord dist_sq = delta.SquaredEuclideanNorm();

    if( dist_sq == 0 || dist_sq < min_dist_sq )
    {
        if( aActual )
            *aActual = std::max( 0, (int) sqrt( dist_sq ) - aA.GetRadius() - aB.GetRadius() );

        if( aLocation )
            *aLocation = ( aA.GetCenter() + aB.GetCenter() ) / 2;

        if( aMTV )
            *aMTV = delta.Resize( min_dist - sqrt( dist_sq ) + 3 );  // fixme: apparent rounding error

        return true;
    }

    return false;
}


static inline bool Collide( const SHAPE_RECT& aA, const SHAPE_CIRCLE& aB, int aClearance,
                            int* aActual, VECTOR2I* aLocation, VECTOR2I* aMTV )
{
    if( aA.GetRadius() > 0 )
    {
        wxASSERT_MSG( !aMTV, wxT( "MTV not implemented for SHAPE_RECT to SHAPE_CIRCLE collisions when rect "
                                  "has rounded corners" ) );

        const SHAPE_LINE_CHAIN& outline = aA.Outline();
        return outline.SHAPE::Collide( &aB, aClearance, aActual, aLocation );
    }

    const VECTOR2I c = aB.GetCenter();
    const VECTOR2I p0 = aA.GetPosition();
    const VECTOR2I size = aA.GetSize();
    const int      r = aB.GetRadius();
    const int      min_dist = aClearance + r;
    const ecoord   min_dist_sq = SEG::Square( min_dist );

    const VECTOR2I vts[] =
    {
        VECTOR2I( p0.x,          p0.y ),
        VECTOR2I( p0.x,          p0.y + size.y ),
        VECTOR2I( p0.x + size.x, p0.y + size.y ),
        VECTOR2I( p0.x + size.x, p0.y ),
        VECTOR2I( p0.x,          p0.y )
    };

    ecoord   nearest_side_dist_sq = VECTOR2I::ECOORD_MAX;
    VECTOR2I nearest;

    bool inside = c.x >= p0.x && c.x <= ( p0.x + size.x )
                  && c.y >= p0.y && c.y <= ( p0.y + size.y );

    // If we're not looking for MTV or actual, short-circuit once we find a hard collision
    if( inside && !aActual && !aLocation && !aMTV )
        return true;

    for( int i = 0; i < 4; i++ )
    {
        const SEG side( vts[i], vts[ i + 1] );

        VECTOR2I pn = side.NearestPoint( c );
        ecoord side_dist_sq = ( pn - c ).SquaredEuclideanNorm();

        if( side_dist_sq < nearest_side_dist_sq )
        {
            nearest = pn;
            nearest_side_dist_sq = side_dist_sq;

            if( aMTV )
                continue;

            if( nearest_side_dist_sq == 0 )
                break;

            // If we're not looking for aActual then any collision will do
            if( nearest_side_dist_sq < min_dist_sq && !aActual )
                break;
        }
    }

    if( inside || nearest_side_dist_sq == 0 || nearest_side_dist_sq < min_dist_sq )
    {
        if( aLocation )
            *aLocation = nearest;

        if( aActual )
            *aActual = std::max( 0, (int) sqrt( nearest_side_dist_sq ) - r );

        if( aMTV )
        {
            VECTOR2I delta = c - nearest;

            if( inside )
                *aMTV = -delta.Resize( abs( min_dist + 1 + sqrt( nearest_side_dist_sq ) ) + 1 );
            else
                *aMTV = delta.Resize( abs( min_dist + 1 - sqrt( nearest_side_dist_sq ) ) + 1 );
        }

        return true;
    }

    return false;
}


static VECTOR2I pushoutForce( const SHAPE_CIRCLE& aA, const SEG& aB, int aClearance )
{
    VECTOR2I f( 0, 0 );

    const VECTOR2I c = aA.GetCenter();
    const VECTOR2I nearest = aB.NearestPoint( c );

    const int r = aA.GetRadius();

    int dist = ( nearest - c ).EuclideanNorm();
    int min_dist = aClearance + r;

    if( dist < min_dist )
    {
        for( int corr = 0; corr < 5; corr++ )
        {
            f = ( aA.GetCenter() - nearest ).Resize( min_dist - dist + corr );

            if( aB.Distance( c + f ) >= min_dist )
                break;
        }
    }

    return f;
}


static inline bool Collide( const SHAPE_CIRCLE& aA, const SHAPE_LINE_CHAIN_BASE& aB,
                            int aClearance, int* aActual, VECTOR2I* aLocation, VECTOR2I* aMTV )
{
    int closest_dist = std::numeric_limits<int>::max();
    int closest_mtv_dist = std::numeric_limits<int>::max();
    VECTOR2I nearest;
    int closest_mtv_seg = -1;

    if( aB.IsClosed() && aB.PointInside( aA.GetCenter() ) )
    {
        nearest = aA.GetCenter();
        closest_dist = 0;

        if( aMTV )
        {
            for( size_t s = 0; s < aB.GetSegmentCount(); s++ )
            {
                int dist = aB.GetSegment(s).Distance( aA.GetCenter() );

                if( dist < closest_mtv_dist )
                {
                    closest_mtv_dist = dist;
                    closest_mtv_seg = s;
                }
            }
        }

    }
    else
    {
        for( size_t s = 0; s < aB.GetSegmentCount(); s++ )
        {
            int collision_dist = 0;
            VECTOR2I pn;

            if( aA.Collide( aB.GetSegment( s ), aClearance,
                            aActual || aLocation ? &collision_dist : nullptr,
                            aLocation ? &pn : nullptr ) )
            {
                if( collision_dist < closest_dist )
                {
                    nearest = pn;
                    closest_dist = collision_dist;
                }

                if( closest_dist == 0 )
                    break;

                // If we're not looking for aActual then any collision will do
                if( !aActual )
                    break;
            }
        }
    }

    if( closest_dist == 0 || closest_dist < aClearance )
    {
        if( aLocation )
            *aLocation = nearest;

        if( aActual )
            *aActual = closest_dist;

        if( aMTV )
        {
            SHAPE_CIRCLE cmoved( aA );
            VECTOR2I f_total( 0, 0 );

            VECTOR2I f;

            if (closest_mtv_seg >= 0)
            {
                SEG cs = aB.GetSegment( closest_mtv_seg );
                VECTOR2I np = cs.NearestPoint( aA.GetCenter() );
                f = ( np - aA.GetCenter() ) + ( np - aA.GetCenter() ).Resize( aA.GetRadius() );
            }

            cmoved.SetCenter( cmoved.GetCenter() + f );
            f_total += f;

            for( size_t s = 0; s < aB.GetSegmentCount(); s++ )
            {
                f = pushoutForce( cmoved, aB.GetSegment( s ), aClearance );
                cmoved.SetCenter( cmoved.GetCenter() + f );
                f_total += f;
            }

            *aMTV = f_total;
        }

        return true;
    }

    return false;
}


static inline bool Collide( const SHAPE_CIRCLE& aA, const SHAPE_SEGMENT& aSeg, int aClearance,
                            int* aActual, VECTOR2I* aLocation, VECTOR2I* aMTV )
{
    if( aA.Collide( aSeg.GetSeg(), aClearance + aSeg.GetWidth() / 2, aActual, aLocation ) )
    {
        if( aMTV )
            *aMTV = -pushoutForce( aA, aSeg.GetSeg(), aClearance + aSeg.GetWidth() / 2);

        if( aActual )
            *aActual = std::max( 0, *aActual - aSeg.GetWidth() / 2 );

        return true;
    }

    return false;
}


static inline bool Collide( const SHAPE_LINE_CHAIN_BASE& aA, const SHAPE_LINE_CHAIN_BASE& aB,
                            int aClearance, int* aActual, VECTOR2I* aLocation, VECTOR2I* aMTV )
{
    wxASSERT_MSG( !aMTV, wxString::Format( wxT( "MTV not implemented for %s : %s collisions" ),
                                           aA.TypeName(),
                                           aB.TypeName() ) );

    int closest_dist = std::numeric_limits<int>::max();
    VECTOR2I nearest;

    if( aB.IsClosed() && aA.GetPointCount() > 0 && aB.PointInside( aA.GetPoint( 0 ) ) )
    {
        closest_dist = 0;
        nearest = aA.GetPoint( 0 );
    }
    else if( aA.IsClosed() && aB.GetPointCount() > 0 && aA.PointInside( aB.GetPoint( 0 ) ) )
    {
        closest_dist = 0;
        nearest = aB.GetPoint( 0 );
    }
    else
    {
        std::vector<SEG> a_segs;
        std::vector<SEG> b_segs;

        for( size_t ii = 0; ii < aA.GetSegmentCount(); ii++ )
        {
            if( aA.Type() != SH_LINE_CHAIN
                || !static_cast<const SHAPE_LINE_CHAIN*>( &aA )->IsArcSegment( ii ) )
            {
                a_segs.push_back( aA.GetSegment( ii ) );
            }
        }

        for( size_t ii = 0; ii < aB.GetSegmentCount(); ii++ )
        {
            if( aB.Type() != SH_LINE_CHAIN
                || !static_cast<const SHAPE_LINE_CHAIN*>( &aB )->IsArcSegment( ii ) )
            {
                b_segs.push_back( aB.GetSegment( ii ) );
            }
        }

        auto seg_sort = []( const SEG& a, const SEG& b )
        {
            return a.A.x < b.A.x || ( a.A.x == b.A.x && a.A.y < b.A.y );
        };

        std::sort( a_segs.begin(), a_segs.end(), seg_sort );
        std::sort( b_segs.begin(), b_segs.end(), seg_sort );

        for( const SEG& a_seg : a_segs )
        {
            for( const SEG& b_seg : b_segs )
            {
                int dist = 0;

                if( a_seg.Collide( b_seg, aClearance, aActual || aLocation ? &dist : nullptr ) )
                {
                    if( dist < closest_dist )
                    {
                        nearest = a_seg.NearestPoint( b_seg );
                        closest_dist = dist;
                    }

                    if( closest_dist == 0 )
                        break;

                    // If we're not looking for aActual then any collision will do
                    if( !aActual )
                        break;
                }
            }
        }
    }

    if( (!aActual && !aLocation ) || closest_dist > 0 )
    {
        std::vector<const SHAPE_LINE_CHAIN*> chains = {
            dynamic_cast<const SHAPE_LINE_CHAIN*>( &aA ),
            dynamic_cast<const SHAPE_LINE_CHAIN*>( &aB )
        };

        std::vector<const SHAPE*> shapes = { &aA, &aB };

        for( int ii = 0; ii < 2; ii++ )
        {
            const SHAPE_LINE_CHAIN* chain = chains[ii];
            const SHAPE* other = shapes[( ii + 1 ) % 2];

            if( !chain )
                continue;

            for( size_t jj = 0; jj < chain->ArcCount(); jj++ )
            {
                const SHAPE_ARC& arc = chain->Arc( jj );

                if( arc.Collide( other, aClearance, aActual, aLocation ) )
                    return true;
            }
        }
    }

    if( closest_dist == 0 || closest_dist < aClearance )
    {
        if( aLocation )
            *aLocation = nearest;

        if( aActual )
            *aActual = closest_dist;

        return true;
    }

    return false;
}


static inline bool Collide( const SHAPE_RECT& aA, const SHAPE_LINE_CHAIN_BASE& aB, int aClearance,
                            int* aActual, VECTOR2I* aLocation, VECTOR2I* aMTV )
{
    if( aA.GetRadius() > 0 )
        return Collide( aA.Outline(), aB, aClearance, aActual, aLocation, aMTV );

    wxASSERT_MSG( !aMTV, wxString::Format( wxT( "MTV not implemented for %s : %s collisions" ),
                                           aA.TypeName(),
                                           aB.TypeName() ) );

    int closest_dist = std::numeric_limits<int>::max();
    VECTOR2I nearest;

    if( aB.IsClosed() && aB.PointInside( aA.Centre() ) )
    {
        nearest = aA.Centre();
        closest_dist = 0;
    }
    else
    {
        for( size_t s = 0; s < aB.GetSegmentCount(); s++ )
        {
            int collision_dist = 0;
            VECTOR2I pn;

            if( aA.Collide( aB.GetSegment( s ), aClearance,
                            aActual || aLocation ? &collision_dist : nullptr,
                            aLocation ? &pn : nullptr ) )
            {
                if( collision_dist < closest_dist )
                {
                    nearest = pn;
                    closest_dist = collision_dist;
                }

                if( closest_dist == 0 )
                    break;

                // If we're not looking for aActual then any collision will do
                if( !aActual )
                    break;
            }
        }
    }

    if( closest_dist == 0 || closest_dist < aClearance )
    {
        if( aLocation )
            *aLocation = nearest;

        if( aActual )
            *aActual = closest_dist;

        return true;
    }

    return false;
}


static inline bool Collide( const SHAPE_SEGMENT& aA, const SHAPE_SEGMENT& aB, int aClearance,
                            int* aActual, VECTOR2I* aLocation, VECTOR2I* aMTV )
{
    wxASSERT_MSG( !aMTV, wxString::Format( wxT( "MTV not implemented for %s : %s collisions" ),
                                           aA.TypeName(),
                                           aB.TypeName() ) );

    bool rv = aA.Collide( aB.GetSeg(), aClearance + aB.GetWidth() / 2, aActual, aLocation );

    if( rv && aActual )
        *aActual = std::max( 0, *aActual - aB.GetWidth() / 2 );

    return rv;
}


static inline bool Collide( const SHAPE_LINE_CHAIN_BASE& aA, const SHAPE_SEGMENT& aB,
                            int aClearance, int* aActual, VECTOR2I* aLocation, VECTOR2I* aMTV )
{
    wxASSERT_MSG( !aMTV, wxString::Format( wxT( "MTV not implemented for %s : %s collisions" ),
                                           aA.TypeName(),
                                           aB.TypeName() ) );

    bool rv = aA.Collide( aB.GetSeg(), aClearance + aB.GetWidth() / 2, aActual, aLocation );

    if( rv && aActual )
        *aActual = std::max( 0, *aActual - aB.GetWidth() / 2 );

    return rv;
}


static inline bool Collide( const SHAPE_RECT& aA, const SHAPE_SEGMENT& aB, int aClearance,
                            int* aActual, VECTOR2I* aLocation, VECTOR2I* aMTV )
{
    if( aA.GetRadius() > 0 )
        return Collide( aA.Outline(), aB, aClearance, aActual, aLocation, aMTV );

    wxASSERT_MSG( !aMTV, wxString::Format( wxT( "MTV not implemented for %s : %s collisions" ),
                                           aA.TypeName(),
                                           aB.TypeName() ) );

    bool rv = aA.Collide( aB.GetSeg(), aClearance + aB.GetWidth() / 2, aActual, aLocation );

    if( rv && aActual )
        *aActual = std::max( 0, *aActual - aB.GetWidth() / 2 );

    return rv;
}


static inline bool Collide( const SHAPE_RECT& aA, const SHAPE_RECT& aB, int aClearance,
                            int* aActual, VECTOR2I* aLocation, VECTOR2I* aMTV )
{
    if( aClearance || aActual || aLocation || aMTV || aA.GetRadius() > 0 || aB.GetRadius() > 0 )
    {
        return Collide( aA.Outline(), aB.Outline(), aClearance, aActual, aLocation, aMTV );
    }
    else
    {
        BOX2I bboxa = aA.BBox();
        BOX2I bboxb = aB.BBox();

        return bboxa.Intersects( bboxb );
    }
}


static inline bool Collide( const SHAPE_ARC& aA, const SHAPE_CIRCLE& aB, int aClearance,
                            int* aActual, VECTOR2I* aLocation, VECTOR2I* aMTV )
{
    if( aA.IsEffectiveLine() )
    {
        SHAPE_SEGMENT tmp( aA.GetP0(), aA.GetP1(), aA.GetWidth() );
        bool retval = Collide( aB, tmp, aClearance, aActual, aLocation, aMTV );

        if( retval && aMTV )
            *aMTV = - *aMTV;

        return retval;
    }

    VECTOR2I ptA, ptB;
    int64_t  dist_sq = std::numeric_limits<int64_t>::max();
    aA.NearestPoints( aB, ptA, ptB, dist_sq );

    if( dist_sq == 0 || dist_sq < SEG::Square( aClearance ) )
    {
        if( aLocation )
            *aLocation = ( ptA + ptB ) / 2;

        if( aActual )
            *aActual = std::max( 0, KiROUND( std::sqrt( dist_sq ) ) );

        if( aMTV )
        {
            const VECTOR2I delta = ptB - ptA;
            *aMTV = delta.Resize( aClearance - std::sqrt( dist_sq ) + 3 );
        }

        return true;
    }

    return false;
}


static inline bool Collide( const SHAPE_ARC& aA, const SHAPE_LINE_CHAIN& aB, int aClearance,
                            int* aActual, VECTOR2I* aLocation, VECTOR2I* aMTV )
{
    wxASSERT_MSG( !aMTV, wxString::Format( wxT( "MTV not implemented for %s : %s collisions" ),
                                           aA.TypeName(),
                                           aB.TypeName() ) );

    int      closest_dist = std::numeric_limits<int>::max();
    VECTOR2I nearest;

    if( aB.IsClosed() && aB.PointInside( aA.GetP0() ) )
    {
        closest_dist = 0;
        nearest = aA.GetP0();
    }
    else
    {
        int      collision_dist = 0;
        VECTOR2I pn;

        for( size_t i = 0; i < aB.GetSegmentCount(); i++ )
        {
            // ignore arcs - we will collide these separately
            if( aB.IsArcSegment( i ) )
                continue;

            if( aA.Collide( aB.GetSegment( i ), aClearance,
                            aActual || aLocation ? &collision_dist : nullptr,
                            aLocation ? &pn : nullptr ) )
            {
                if( collision_dist < closest_dist )
                {
                    nearest = pn;
                    closest_dist = collision_dist;
                }

                if( closest_dist == 0 )
                    break;

                // If we're not looking for aActual then any collision will do
                if( !aActual )
                    break;
            }
        }

        for( size_t i = 0; i < aB.ArcCount(); i++ )
        {
            const SHAPE_ARC& arc = aB.Arc( i );

            // The arcs in the chain should have zero width
            wxASSERT_MSG( arc.GetWidth() == 0, wxT( "Invalid arc width - should be zero" ) );

            if( aA.Collide( &arc, aClearance, aActual || aLocation ? &collision_dist : nullptr,
                            aLocation ? &pn : nullptr ) )
            {
                if( collision_dist < closest_dist )
                {
                    nearest = pn;
                    closest_dist = collision_dist;
                }

                if( closest_dist == 0 )
                    break;

                if( !aActual )
                    break;
            }
        }
    }

    if( closest_dist == 0 || closest_dist < aClearance )
    {
        if( aLocation )
            *aLocation = nearest;

        if( aActual )
            *aActual = closest_dist;

        return true;
    }

    return false;
}


static inline bool Collide( const SHAPE_ARC& aA, const SHAPE_RECT& aB, int aClearance,
                            int* aActual, VECTOR2I* aLocation, VECTOR2I* aMTV )
{
    if( aB.GetRadius() > 0 )
        return Collide( aA, aB.Outline(), aClearance, aActual, aLocation, aMTV );

    if( aA.IsEffectiveLine() )
    {
        SHAPE_SEGMENT tmp( aA.GetP0(), aA.GetP1(), aA.GetWidth() );
        bool retval = Collide( aB, tmp, aClearance, aActual, aLocation, aMTV );

        if( retval && aMTV )
            *aMTV = - *aMTV;

        return retval;
    }

    VECTOR2I ptA, ptB;
    int64_t  dist_sq = std::numeric_limits<int64_t>::max();
    aA.NearestPoints( aB, ptA, ptB, dist_sq );

    if( dist_sq == 0 || dist_sq < SEG::Square( aClearance ) )
    {
        if( aLocation )
            *aLocation = ( ptA + ptB ) / 2;

        if( aActual )
            *aActual = std::max( 0, KiROUND( std::sqrt( dist_sq ) ) );

        if( aMTV )
        {
            const VECTOR2I delta = ptB - ptA;
            *aMTV = delta.Resize( aClearance - std::sqrt( dist_sq ) + 3 );
        }

        return true;
    }

    return false;
}


static inline bool Collide( const SHAPE_ARC& aA, const SHAPE_SEGMENT& aB, int aClearance,
                            int* aActual, VECTOR2I* aLocation, VECTOR2I* aMTV )
{
    wxASSERT_MSG( !aMTV, wxString::Format( wxT( "MTV not implemented for %s : %s collisions" ),
                                           aA.TypeName(),
                                           aB.TypeName() ) );

    // If the arc radius is too large, it is effectively a line segment
    if( aA.IsEffectiveLine() )
    {
        SHAPE_SEGMENT tmp( aA.GetP0(), aA.GetP1(), aA.GetWidth() );
        return Collide( tmp, aB, aClearance, aActual, aLocation, aMTV );
    }

    bool rv = aA.Collide( aB.GetSeg(), aClearance + aB.GetWidth() / 2, aActual, aLocation );

    if( rv && aActual )
        *aActual = std::max( 0, *aActual - aB.GetWidth() / 2 );

    return rv;
}


static inline bool Collide( const SHAPE_ARC& aA, const SHAPE_LINE_CHAIN_BASE& aB, int aClearance,
                            int* aActual, VECTOR2I* aLocation, VECTOR2I* aMTV )
{
    // If the arc radius is too large, it is effectively a line segment
    if( aA.IsEffectiveLine() )
    {
        SHAPE_SEGMENT tmp( aA.GetP0(), aA.GetP1(), aA.GetWidth() );
        return Collide( aB, tmp, aClearance, aActual, aLocation, aMTV );
    }

    wxASSERT_MSG( !aMTV, wxString::Format( wxT( "MTV not implemented for %s : %s collisions" ),
                                           aA.TypeName(),
                                           aB.TypeName() ) );

    int      closest_dist = std::numeric_limits<int>::max();
    VECTOR2I nearest;

    if( aB.IsClosed() && aB.PointInside( aA.GetP0() ) )
    {
        closest_dist = 0;
        nearest = aA.GetP0();
    }
    else
    {
        for( size_t i = 0; i < aB.GetSegmentCount(); i++ )
        {
            int      collision_dist = 0;
            VECTOR2I pn;

            if( aA.Collide( aB.GetSegment( i ), aClearance,
                            aActual || aLocation ? &collision_dist : nullptr,
                            aLocation ? &pn : nullptr ) )
            {
                if( collision_dist < closest_dist )
                {
                    nearest = pn;
                    closest_dist = collision_dist;
                }

                if( closest_dist == 0 )
                    break;

                // If we're not looking for aActual then any collision will do
                if( !aActual )
                    break;
            }
        }
    }

    if( closest_dist == 0 || closest_dist < aClearance )
    {
        if( aLocation )
            *aLocation = nearest;

        if( aActual )
            *aActual = closest_dist;

        return true;
    }

    return false;
}


static inline bool Collide( const SHAPE_ARC& aA, const SHAPE_ARC& aB, int aClearance,
                            int* aActual, VECTOR2I* aLocation, VECTOR2I* aMTV )
{
    if( aA.IsEffectiveLine() )
    {
        SHAPE_SEGMENT tmp( aA.GetP0(), aA.GetP1(), aA.GetWidth() );
        bool retval = Collide( aB, tmp, aClearance, aActual, aLocation, aMTV );

        if( retval && aMTV )
            *aMTV = - *aMTV;

        return retval;
    }

    if( aB.IsEffectiveLine() )
    {
        SHAPE_SEGMENT tmp( aB.GetP0(), aB.GetP1(), aB.GetWidth() );
        return Collide( aA, tmp, aClearance, aActual, aLocation, aMTV );
    }

    VECTOR2I ptA, ptB;
    int64_t  dist_sq = std::numeric_limits<int64_t>::max();
    aA.NearestPoints( aB, ptA, ptB, dist_sq );

    if( dist_sq == 0 || dist_sq < SEG::Square( aClearance ) )
    {
        if( aLocation )
            *aLocation = ( ptA + ptB ) / 2;

        if( aActual )
            *aActual = std::max( 0, KiROUND( std::sqrt( dist_sq ) ) );

        if( aMTV )
        {
            const VECTOR2I delta = ptB - ptA;
            *aMTV = delta.Resize( aClearance - std::sqrt( dist_sq ) + 3 );
        }

        return true;
    }

    return false;
}


template<class T_a, class T_b>
inline bool CollCase( const SHAPE* aA, const SHAPE* aB, int aClearance, int* aActual,
                      VECTOR2I* aLocation, VECTOR2I* aMTV )

{
    return Collide( *static_cast<const T_a*>( aA ), *static_cast<const T_b*>( aB ),
                    aClearance, aActual, aLocation, aMTV);
}


template<class T_a, class T_b>
inline bool CollCaseReversed ( const SHAPE* aA, const SHAPE* aB, int aClearance, int* aActual,
                               VECTOR2I* aLocation, VECTOR2I* aMTV )
{
    bool rv = Collide( *static_cast<const T_b*>( aB ), *static_cast<const T_a*>( aA ),
                       aClearance, aActual, aLocation, aMTV);

    if( rv && aMTV)
        *aMTV = -  *aMTV;

    return rv;
}


static bool collideSingleShapes( const SHAPE* aA, const SHAPE* aB, int aClearance, int* aActual,
                                 VECTOR2I* aLocation, VECTOR2I* aMTV )
{
    if( aA->Type() == SH_POLY_SET )
    {
        const SHAPE_POLY_SET* polySetA = static_cast<const SHAPE_POLY_SET*>( aA );

        wxASSERT( !aMTV );
        return polySetA->Collide( aB, aClearance, aActual, aLocation );
    }
    else if( aB->Type() == SH_POLY_SET )
    {
        const SHAPE_POLY_SET* polySetB = static_cast<const SHAPE_POLY_SET*>( aB );

        wxASSERT( !aMTV );
        return polySetB->Collide( aA, aClearance, aActual, aLocation );
    }

    switch( aA->Type() )
    {
    case SH_NULL:
        return false;

    case SH_RECT:
        switch( aB->Type() )
        {
        case SH_RECT:
            return CollCase<SHAPE_RECT, SHAPE_RECT>( aA, aB, aClearance, aActual, aLocation, aMTV );

        case SH_CIRCLE:
            return CollCase<SHAPE_RECT, SHAPE_CIRCLE>( aA, aB, aClearance, aActual, aLocation, aMTV );

        case SH_LINE_CHAIN:
            return CollCase<SHAPE_RECT, SHAPE_LINE_CHAIN>( aA, aB, aClearance, aActual, aLocation, aMTV );

        case SH_SEGMENT:
            return CollCase<SHAPE_RECT, SHAPE_SEGMENT>( aA, aB, aClearance, aActual, aLocation, aMTV );

        case SH_SIMPLE:
        case SH_POLY_SET_TRIANGLE:
            return CollCase<SHAPE_RECT, SHAPE_LINE_CHAIN_BASE>( aA, aB, aClearance, aActual, aLocation, aMTV );

        case SH_ARC:
            return CollCaseReversed<SHAPE_RECT, SHAPE_ARC>( aA, aB, aClearance, aActual, aLocation, aMTV );

        case SH_NULL:
            return false;

        default:
            break;
        }
        break;

    case SH_CIRCLE:
        switch( aB->Type() )
        {
        case SH_RECT:
            return CollCaseReversed<SHAPE_CIRCLE, SHAPE_RECT>( aA, aB, aClearance, aActual, aLocation, aMTV );

        case SH_CIRCLE:
            return CollCase<SHAPE_CIRCLE, SHAPE_CIRCLE>( aA, aB, aClearance, aActual, aLocation, aMTV );

        case SH_LINE_CHAIN:
            return CollCase<SHAPE_CIRCLE, SHAPE_LINE_CHAIN>( aA, aB, aClearance, aActual, aLocation, aMTV );

        case SH_SEGMENT:
            return CollCase<SHAPE_CIRCLE, SHAPE_SEGMENT>( aA, aB, aClearance, aActual, aLocation, aMTV );

        case SH_SIMPLE:
        case SH_POLY_SET_TRIANGLE:
            return CollCase<SHAPE_CIRCLE, SHAPE_LINE_CHAIN_BASE>( aA, aB, aClearance, aActual, aLocation, aMTV );

        case SH_ARC:
            return CollCaseReversed<SHAPE_CIRCLE, SHAPE_ARC>( aA, aB, aClearance, aActual, aLocation, aMTV );

        case SH_NULL:
            return false;

        default:
            break;
        }
        break;

    case SH_LINE_CHAIN:
        switch( aB->Type() )
        {
        case SH_RECT:
            return CollCase<SHAPE_RECT, SHAPE_LINE_CHAIN>( aB, aA, aClearance, aActual, aLocation, aMTV );

        case SH_CIRCLE:
            return CollCase<SHAPE_CIRCLE, SHAPE_LINE_CHAIN>( aB, aA, aClearance, aActual, aLocation, aMTV );

        case SH_LINE_CHAIN:
            return CollCase<SHAPE_LINE_CHAIN, SHAPE_LINE_CHAIN>( aA, aB, aClearance, aActual, aLocation, aMTV );

        case SH_SEGMENT:
            return CollCase<SHAPE_LINE_CHAIN, SHAPE_SEGMENT>( aA, aB, aClearance, aActual, aLocation, aMTV );

        case SH_SIMPLE:
        case SH_POLY_SET_TRIANGLE:
            return CollCase<SHAPE_LINE_CHAIN, SHAPE_LINE_CHAIN_BASE>( aA, aB, aClearance, aActual, aLocation, aMTV );

        case SH_ARC:
            return CollCaseReversed<SHAPE_LINE_CHAIN, SHAPE_ARC>( aA, aB, aClearance, aActual, aLocation, aMTV );

        case SH_NULL:
            return false;

        default:
            break;
        }
        break;

    case SH_SEGMENT:
        switch( aB->Type() )
        {
        case SH_RECT:
            return CollCase<SHAPE_RECT, SHAPE_SEGMENT>( aB, aA, aClearance, aActual, aLocation, aMTV );

        case SH_CIRCLE:
            return CollCaseReversed<SHAPE_SEGMENT, SHAPE_CIRCLE>( aA, aB, aClearance, aActual, aLocation, aMTV );

        case SH_LINE_CHAIN:
            return CollCase<SHAPE_LINE_CHAIN, SHAPE_SEGMENT>( aB, aA, aClearance, aActual, aLocation, aMTV );

        case SH_SEGMENT:
            return CollCase<SHAPE_SEGMENT, SHAPE_SEGMENT>( aA, aB, aClearance, aActual, aLocation, aMTV );

        case SH_SIMPLE:
        case SH_POLY_SET_TRIANGLE:
            return CollCase<SHAPE_LINE_CHAIN_BASE, SHAPE_SEGMENT>( aB, aA, aClearance, aActual, aLocation, aMTV );

        case SH_ARC:
            return CollCaseReversed<SHAPE_SEGMENT, SHAPE_ARC>( aA, aB, aClearance, aActual, aLocation, aMTV );

        case SH_NULL:
            return false;

        default:
            break;
        }
        break;

    case SH_SIMPLE:
    case SH_POLY_SET_TRIANGLE:
        switch( aB->Type() )
        {
        case SH_RECT:
            return CollCase<SHAPE_RECT, SHAPE_LINE_CHAIN_BASE>( aB, aA, aClearance, aActual, aLocation, aMTV );

        case SH_CIRCLE:
            return CollCase<SHAPE_CIRCLE, SHAPE_LINE_CHAIN_BASE>( aB, aA, aClearance, aActual, aLocation, aMTV );

        case SH_LINE_CHAIN:
            return CollCase<SHAPE_LINE_CHAIN, SHAPE_LINE_CHAIN_BASE>( aB, aA, aClearance, aActual, aLocation, aMTV );

        case SH_SEGMENT:
            return CollCase<SHAPE_LINE_CHAIN_BASE, SHAPE_SEGMENT>( aA, aB, aClearance, aActual, aLocation, aMTV );

        case SH_SIMPLE:
        case SH_POLY_SET_TRIANGLE:
            return CollCase<SHAPE_LINE_CHAIN_BASE, SHAPE_LINE_CHAIN_BASE>( aA, aB, aClearance, aActual, aLocation, aMTV );

        case SH_ARC:
            return CollCaseReversed<SHAPE_LINE_CHAIN_BASE, SHAPE_ARC>( aA, aB, aClearance, aActual, aLocation, aMTV );

        case SH_NULL:
            return false;

        default:
            break;
        }
        break;

    case SH_ARC:
        switch( aB->Type() )
        {
        case SH_RECT:
            return CollCase<SHAPE_ARC, SHAPE_RECT>( aA, aB, aClearance, aActual, aLocation, aMTV );

        case SH_CIRCLE:
            return CollCase<SHAPE_ARC, SHAPE_CIRCLE>( aA, aB, aClearance, aActual, aLocation, aMTV );

        case SH_LINE_CHAIN:
            return CollCase<SHAPE_ARC, SHAPE_LINE_CHAIN>( aA, aB, aClearance, aActual, aLocation, aMTV );

        case SH_SEGMENT:
            return CollCase<SHAPE_ARC, SHAPE_SEGMENT>( aA, aB, aClearance, aActual, aLocation, aMTV );

        case SH_SIMPLE:
        case SH_POLY_SET_TRIANGLE:
            return CollCase<SHAPE_ARC, SHAPE_LINE_CHAIN_BASE>( aA, aB, aClearance, aActual, aLocation, aMTV );

        case SH_ARC:
            return CollCase<SHAPE_ARC, SHAPE_ARC>( aA, aB, aClearance, aActual, aLocation, aMTV );

        case SH_NULL:
            return false;

        default:
            break;
        }
        break;

    default:
        break;
    }

    wxFAIL_MSG( wxString::Format( wxT( "Unsupported collision: %s with %s" ),
                                  SHAPE_TYPE_asString( aA->Type() ),
                                  SHAPE_TYPE_asString( aB->Type() ) ) );

    return false;
}

static bool collideShapes( const SHAPE* aA, const SHAPE* aB, int aClearance, int* aActual,
                           VECTOR2I* aLocation, VECTOR2I* aMTV )
{
    int currentActual = std::numeric_limits<int>::max();
    VECTOR2I currentLocation;
    VECTOR2I currentMTV(0, 0);
    bool colliding = false;

    auto canExit =
            [&]()
            {
                if( !colliding )
                    return false;

                if( aActual && currentActual > 0 )
                    return false;

                if( aMTV )
                    return false;

                return true;
            };

    auto collideCompoundSubshapes =
            [&]( const SHAPE* elemA, const SHAPE* elemB, int clearance ) -> bool
            {
                int actual = 0;
                VECTOR2I location;
                VECTOR2I mtv;

                if( collideSingleShapes( elemA, elemB, clearance,
                                         aActual || aLocation ? &actual : nullptr,
                                         aLocation ? &location : nullptr,
                                         aMTV ? &mtv : nullptr ) )
                {
                    if( actual < currentActual )
                    {
                        currentActual = actual;
                        currentLocation = location;
                    }

                    if( aMTV && mtv.SquaredEuclideanNorm() > currentMTV.SquaredEuclideanNorm() )
                    {
                        currentMTV = mtv;
                    }

                    return true;
                }

                return false;
            };

    if( aA->Type() == SH_COMPOUND && aB->Type() == SH_COMPOUND )
    {
        const SHAPE_COMPOUND* cmpA = static_cast<const SHAPE_COMPOUND*>( aA );
        const SHAPE_COMPOUND* cmpB = static_cast<const SHAPE_COMPOUND*>( aB );

        for( const SHAPE* elemA : cmpA->Shapes() )
        {
            for( const SHAPE* elemB : cmpB->Shapes() )
            {
                if( collideCompoundSubshapes( elemA, elemB, aClearance ) )
                {
                    colliding = true;

                    if( canExit() )
                        break;
                }
            }

            if( canExit() )
                break;
        }
    }
    else if( aA->Type() == SH_COMPOUND )
    {
        const SHAPE_COMPOUND* cmpA = static_cast<const SHAPE_COMPOUND*>( aA );

        for( const SHAPE* elemA : cmpA->Shapes() )
        {
            if( collideCompoundSubshapes( elemA, aB, aClearance ) )
            {
                colliding = true;

                if( canExit() )
                    break;
            }
        }
    }
    else if( aB->Type() == SH_COMPOUND )
    {
        const SHAPE_COMPOUND* cmpB = static_cast<const SHAPE_COMPOUND*>( aB );

        for( const SHAPE* elemB : cmpB->Shapes() )
        {
            if( collideCompoundSubshapes( aA, elemB, aClearance ) )
            {
                colliding = true;

                if( canExit() )
                    break;
            }
        }
    }
    else
    {
        return collideSingleShapes( aA, aB, aClearance, aActual, aLocation, aMTV );
    }

    if( colliding )
    {
        if( aLocation )
            *aLocation = currentLocation;

        if( aActual )
            *aActual = currentActual;

        if( aMTV )
            *aMTV = currentMTV;
    }

    return colliding;
}


bool SHAPE::Collide( const SHAPE* aShape, int aClearance, VECTOR2I* aMTV ) const
{
    return collideShapes( this, aShape, aClearance, nullptr, nullptr, aMTV );
}


bool SHAPE::Collide( const SHAPE* aShape, int aClearance, int* aActual, VECTOR2I* aLocation ) const
{
    return collideShapes( this, aShape, aClearance, aActual, aLocation, nullptr );
}


