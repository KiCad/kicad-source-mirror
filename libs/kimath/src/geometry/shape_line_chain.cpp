/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2017 CERN
 * Copyright (C) 2021 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 * Copyright (C) 2013-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <limits.h>          // for INT_MAX
#include <math.h>            // for hypot
#include <map>
#include <string>            // for basic_string

#include <clipper.hpp>
#include <core/kicad_algo.h> // for alg::run_on_pair
#include <geometry/seg.h>    // for SEG, OPT_VECTOR2I
#include <geometry/circle.h>    // for CIRCLE
#include <geometry/shape_line_chain.h>
#include <math/box2.h>       // for BOX2I
#include <math/util.h>       // for rescale
#include <math/vector2d.h>   // for VECTOR2, VECTOR2I
#include <trigo.h>  // for RAD2DECIDEG, CalcArcAngle

class SHAPE;

const ssize_t                     SHAPE_LINE_CHAIN::SHAPE_IS_PT = -1;
const std::pair<ssize_t, ssize_t> SHAPE_LINE_CHAIN::SHAPES_ARE_PT = { SHAPE_IS_PT, SHAPE_IS_PT };

SHAPE_LINE_CHAIN::SHAPE_LINE_CHAIN( const std::vector<int>& aV)
    : SHAPE_LINE_CHAIN_BASE( SH_LINE_CHAIN ), m_closed( false ), m_width( 0 )
{
    for(size_t i = 0; i < aV.size(); i+= 2 )
    {
        Append( aV[i], aV[i+1] );
    }
}

SHAPE_LINE_CHAIN::SHAPE_LINE_CHAIN( const ClipperLib::Path&             aPath,
                                    const std::vector<CLIPPER_Z_VALUE>& aZValueBuffer,
                                    const std::vector<SHAPE_ARC>&       aArcBuffer ) :
        SHAPE_LINE_CHAIN_BASE( SH_LINE_CHAIN ),
        m_closed( true ), m_width( 0 )
{
    std::map<ssize_t, ssize_t> loadedArcs;
    m_points.reserve( aPath.size() );
    m_shapes.reserve( aPath.size() );

    auto loadArc =
        [&]( ssize_t aArcIndex ) -> ssize_t
        {
            if( aArcIndex == SHAPE_IS_PT )
            {
                return SHAPE_IS_PT;
            }
            else if( loadedArcs.count( aArcIndex ) == 0 )
            {
                loadedArcs.insert( { aArcIndex, m_arcs.size() } );
                m_arcs.push_back( aArcBuffer.at( aArcIndex ) );
            }

            return loadedArcs.at(aArcIndex);
        };

    for( size_t ii = 0; ii < aPath.size(); ++ii )
    {
        Append( aPath[ii].X, aPath[ii].Y );

        m_shapes[ii].first = loadArc( aZValueBuffer[aPath[ii].Z].m_FirstArcIdx );
        m_shapes[ii].second = loadArc( aZValueBuffer[aPath[ii].Z].m_SecondArcIdx );
    }
}

ClipperLib::Path SHAPE_LINE_CHAIN::convertToClipper( bool aRequiredOrientation,
                                                     std::vector<CLIPPER_Z_VALUE>& aZValueBuffer,
                                                     std::vector<SHAPE_ARC>& aArcBuffer ) const
{
    ClipperLib::Path c_path;
    SHAPE_LINE_CHAIN input;
    bool             orientation = Area( false ) >= 0;
    ssize_t          shape_offset = aArcBuffer.size();

    if( orientation != aRequiredOrientation )
        input = Reverse();
    else
        input = *this;

    for( int i = 0; i < input.PointCount(); i++ )
    {
        const VECTOR2I& vertex = input.CPoint( i );

        CLIPPER_Z_VALUE z_value( input.m_shapes[i], shape_offset );
        size_t          z_value_ptr = aZValueBuffer.size();
        aZValueBuffer.push_back( z_value );

        c_path.push_back( ClipperLib::IntPoint( vertex.x, vertex.y, z_value_ptr ) );
    }

    aArcBuffer.insert( aArcBuffer.end(), input.m_arcs.begin(), input.m_arcs.end() );

    return c_path;
}


void SHAPE_LINE_CHAIN::convertArc( ssize_t aArcIndex )
{
    if( aArcIndex < 0 )
        aArcIndex += m_arcs.size();

    if( aArcIndex >= static_cast<ssize_t>( m_arcs.size() ) )
        return;

    // Clear the shapes references
    for( auto& sh : m_shapes )
    {
        alg::run_on_pair( sh,
            [&]( ssize_t& aShapeIndex )
            {
                if( aShapeIndex == aArcIndex )
                    aShapeIndex = SHAPE_IS_PT;

                if( aShapeIndex > aArcIndex )
                    --aShapeIndex;
            } );

        if( sh.second != SHAPE_IS_PT && sh.first == SHAPE_IS_PT )
            std::swap( sh.first, sh.second );
    }

    m_arcs.erase( m_arcs.begin() + aArcIndex );
}


void SHAPE_LINE_CHAIN::amendArc( size_t aArcIndex, const VECTOR2I& aNewStart,
                                 const VECTOR2I& aNewEnd )
{
    wxCHECK_MSG( aArcIndex <  m_arcs.size(), /* void */,
                 "Invalid arc index requested." );

    SHAPE_ARC& theArc = m_arcs[aArcIndex];

    // Try to preseve the centre of the original arc
    SHAPE_ARC newArc;
    newArc.ConstructFromStartEndCenter( aNewStart, aNewEnd, theArc.GetCenter(),
                                        theArc.IsClockwise() );

    m_arcs[aArcIndex] = newArc;
}


void SHAPE_LINE_CHAIN::splitArc( ssize_t aPtIndex, bool aCoincident )
{
    if( aPtIndex < 0 )
        aPtIndex += m_shapes.size();

    if( !IsSharedPt( aPtIndex ) && IsArcStart( aPtIndex ) )
        return; // Nothing to do

    if( !IsPtOnArc( aPtIndex ) )
        return; // Nothing to do

    wxCHECK_MSG( aPtIndex < static_cast<ssize_t>( m_shapes.size() ), /* void */,
                 "Invalid point index requested." );

    if( IsSharedPt( aPtIndex ) || IsArcEnd( aPtIndex ) )
    {
        if( aCoincident || aPtIndex == 0 )
            return; // nothing to do

        ssize_t firstArcIndex = m_shapes[aPtIndex].first;

        const VECTOR2I& newStart = m_arcs[firstArcIndex].GetP0(); // don't amend the start
        const VECTOR2I& newEnd   = m_points[aPtIndex - 1];
        amendArc( firstArcIndex, newStart, newEnd );

        if( IsSharedPt( aPtIndex ) )
        {
            m_shapes[aPtIndex].first  = m_shapes[aPtIndex].second;
            m_shapes[aPtIndex].second = SHAPE_IS_PT;
        }
        else
        {
            m_shapes[aPtIndex] = SHAPES_ARE_PT;
        }

        return;
    }

    ssize_t    currArcIdx = ArcIndex( aPtIndex );
    SHAPE_ARC& currentArc = m_arcs[currArcIdx];

    SHAPE_ARC newArc1;
    SHAPE_ARC newArc2;

    VECTOR2I arc1End = ( aCoincident ) ? m_points[aPtIndex] : m_points[aPtIndex - 1];
    VECTOR2I arc2Start = m_points[aPtIndex];

    newArc1.ConstructFromStartEndCenter( currentArc.GetP0(), arc1End, currentArc.GetCenter(),
                                         currentArc.IsClockwise() );

    newArc2.ConstructFromStartEndCenter( arc2Start, currentArc.GetP1(), currentArc.GetCenter(),
                                         currentArc.IsClockwise() );

    if( !aCoincident && ArcIndex( aPtIndex - 1 ) != currArcIdx )
    {
        //Ignore newArc1 as it has zero points
        m_arcs[currArcIdx] = newArc2;
    }
    else
    {
        m_arcs[currArcIdx] = newArc1;
        m_arcs.insert( m_arcs.begin() + currArcIdx + 1, newArc2 );

        if( aCoincident )
        {
            m_shapes[aPtIndex].second = currArcIdx + 1;
            aPtIndex++;
        }

        // Only change the arc indices for the second half of the point range
        for( int i = aPtIndex; i < PointCount(); i++ )
        {
            alg::run_on_pair( m_shapes[i], [&]( ssize_t& aIndex ) {
                if( aIndex != SHAPE_IS_PT )
                    aIndex++;
            } );
        }
    }
}


bool SHAPE_LINE_CHAIN_BASE::Collide( const VECTOR2I& aP, int aClearance, int* aActual,
                                     VECTOR2I* aLocation ) const
{
    if( IsClosed() && PointInside( aP, aClearance ) )
    {
        if( aLocation )
            *aLocation = aP;

        if( aActual )
            *aActual = 0;

        return true;
    }

    SEG::ecoord closest_dist_sq = VECTOR2I::ECOORD_MAX;
    SEG::ecoord clearance_sq = SEG::Square( aClearance );
    VECTOR2I nearest;

    for( size_t i = 0; i < GetSegmentCount(); i++ )
    {
        const SEG& s = GetSegment( i );
        VECTOR2I pn = s.NearestPoint( aP );
        SEG::ecoord dist_sq = ( pn - aP ).SquaredEuclideanNorm();

        if( dist_sq < closest_dist_sq )
        {
            nearest = pn;
            closest_dist_sq = dist_sq;

            if( closest_dist_sq == 0 )
                break;

            // If we're not looking for aActual then any collision will do
            if( closest_dist_sq < clearance_sq && !aActual )
                break;
        }
    }

    if( closest_dist_sq == 0 || closest_dist_sq < clearance_sq )
    {
        if( aLocation )
            *aLocation = nearest;

        if( aActual )
            *aActual = sqrt( closest_dist_sq );

        return true;
    }

    return false;
}


bool SHAPE_LINE_CHAIN::Collide( const VECTOR2I& aP, int aClearance, int* aActual,
                                VECTOR2I* aLocation ) const
{
    if( IsClosed() && PointInside( aP, aClearance ) )
    {
        if( aLocation )
            *aLocation = aP;

        if( aActual )
            *aActual = 0;

        return true;
    }

    SEG::ecoord closest_dist_sq = VECTOR2I::ECOORD_MAX;
    SEG::ecoord clearance_sq = SEG::Square( aClearance );
    VECTOR2I    nearest;

    // Collide line segments
    for( size_t i = 0; i < GetSegmentCount(); i++ )
    {
        if( IsArcSegment( i ) )
            continue;

        const SEG&  s = GetSegment( i );
        VECTOR2I    pn = s.NearestPoint( aP );
        SEG::ecoord dist_sq = ( pn - aP ).SquaredEuclideanNorm();

        if( dist_sq < closest_dist_sq )
        {
            nearest = pn;
            closest_dist_sq = dist_sq;

            if( closest_dist_sq == 0 )
                break;

            // If we're not looking for aActual then any collision will do
            if( closest_dist_sq < clearance_sq && !aActual )
                break;
        }
    }

    if( closest_dist_sq == 0 || closest_dist_sq < clearance_sq )
    {
        if( aLocation )
            *aLocation = nearest;

        if( aActual )
            *aActual = sqrt( closest_dist_sq );

        return true;
    }

    // Collide arc segments
    for( size_t i = 0; i < ArcCount(); i++ )
    {
        const SHAPE_ARC& arc = Arc( i );

        // The arcs in the chain should have zero width
        wxASSERT_MSG( arc.GetWidth() == 0, "Invalid arc width - should be zero" );

        if( arc.Collide( aP, aClearance, aActual, aLocation ) )
            return true;
    }

    return false;
}


void SHAPE_LINE_CHAIN::Rotate( double aAngle, const VECTOR2I& aCenter )
{
    for( auto& pt : m_points )
    {
        pt -= aCenter;
        pt = pt.Rotate( aAngle );
        pt += aCenter;
    }

    for( auto& arc : m_arcs )
        arc.Rotate( aAngle, aCenter );
}


bool SHAPE_LINE_CHAIN_BASE::Collide( const SEG& aSeg, int aClearance, int* aActual,
                                     VECTOR2I* aLocation ) const
{
    if( IsClosed() && PointInside( aSeg.A ) )
    {
        if( aLocation )
            *aLocation = aSeg.A;

        if( aActual )
            *aActual = 0;

        return true;
    }

    SEG::ecoord closest_dist_sq = VECTOR2I::ECOORD_MAX;
    SEG::ecoord clearance_sq = SEG::Square( aClearance );
    VECTOR2I nearest;

    for( size_t i = 0; i < GetSegmentCount(); i++ )
    {
        const SEG& s = GetSegment( i );
        SEG::ecoord dist_sq = s.SquaredDistance( aSeg );

        if( dist_sq < closest_dist_sq )
        {
            if( aLocation )
                nearest = s.NearestPoint( aSeg );

            closest_dist_sq = dist_sq;

            if( closest_dist_sq == 0)
                break;

            // If we're not looking for aActual then any collision will do
            if( closest_dist_sq < clearance_sq && !aActual )
                break;
        }
    }

    if( closest_dist_sq == 0 || closest_dist_sq < clearance_sq )
    {
        if( aLocation )
            *aLocation = nearest;

        if( aActual )
            *aActual = sqrt( closest_dist_sq );

        return true;
    }

    return false;
}


bool SHAPE_LINE_CHAIN::Collide( const SEG& aSeg, int aClearance, int* aActual,
                                VECTOR2I* aLocation ) const
{
    if( IsClosed() && PointInside( aSeg.A ) )
    {
        if( aLocation )
            *aLocation = aSeg.A;

        if( aActual )
            *aActual = 0;

        return true;
    }

    SEG::ecoord closest_dist_sq = VECTOR2I::ECOORD_MAX;
    SEG::ecoord clearance_sq = SEG::Square( aClearance );
    VECTOR2I    nearest;

    // Collide line segments
    for( size_t i = 0; i < GetSegmentCount(); i++ )
    {
        if( IsArcSegment( i ) )
            continue;

        const SEG&  s = GetSegment( i );
        SEG::ecoord dist_sq = s.SquaredDistance( aSeg );

        if( dist_sq < closest_dist_sq )
        {
            if( aLocation )
                nearest = s.NearestPoint( aSeg );

            closest_dist_sq = dist_sq;

            if( closest_dist_sq == 0 )
                break;

            // If we're not looking for aActual then any collision will do
            if( closest_dist_sq < clearance_sq && !aActual )
                break;
        }
    }

    if( closest_dist_sq == 0 || closest_dist_sq < clearance_sq )
    {
        if( aLocation )
            *aLocation = nearest;

        if( aActual )
            *aActual = sqrt( closest_dist_sq );

        return true;
    }

    // Collide arc segments
    for( size_t i = 0; i < ArcCount(); i++ )
    {
        const SHAPE_ARC& arc = Arc( i );

        // The arcs in the chain should have zero width
        wxASSERT_MSG( arc.GetWidth() == 0, "Invalid arc width - should be zero" );

        if( arc.Collide( aSeg, aClearance, aActual, aLocation ) )
            return true;
    }

    return false;
}


const SHAPE_LINE_CHAIN SHAPE_LINE_CHAIN::Reverse() const
{
    SHAPE_LINE_CHAIN a( *this );

    reverse( a.m_points.begin(), a.m_points.end() );
    reverse( a.m_shapes.begin(), a.m_shapes.end() );
    reverse( a.m_arcs.begin(), a.m_arcs.end() );

    for( auto& sh : a.m_shapes )
    {
        if( sh != SHAPES_ARE_PT )
        {
            alg::run_on_pair( sh,
                [&]( ssize_t& aShapeIndex )
                {
                    if( aShapeIndex != SHAPE_IS_PT )
                        aShapeIndex = a.m_arcs.size() - aShapeIndex - 1;
                } );

            if( sh.second != SHAPE_IS_PT )
            {
                // If the second element is populated, the first one should be too!
                assert( sh.first != SHAPE_IS_PT );

                // Switch round first and second in shared points, as part of reversing the chain
                std::swap( sh.first, sh.second );
            }
        }
    }

    for( SHAPE_ARC& arc : a.m_arcs )
        arc.Reverse();

    a.m_closed = m_closed;

    return a;
}


void SHAPE_LINE_CHAIN::ClearArcs()
{
    for( ssize_t arcIndex = m_arcs.size() - 1; arcIndex >= 0; --arcIndex )
        convertArc( arcIndex );
}


long long int SHAPE_LINE_CHAIN::Length() const
{
    long long int l = 0;

    for( int i = 0; i < SegmentCount(); i++ )
    {
        // Only include segments that aren't part of arc shapes
        if( !IsArcSegment(i) )
            l += CSegment( i ).Length();
    }

    for( int i = 0; i < ArcCount(); i++ )
        l += CArcs()[i].GetLength();

    return l;
}


void SHAPE_LINE_CHAIN::Mirror( bool aX, bool aY, const VECTOR2I& aRef )
{
    for( auto& pt : m_points )
    {
        if( aX )
            pt.x = -pt.x + 2 * aRef.x;

        if( aY )
            pt.y = -pt.y + 2 * aRef.y;
    }

    for( auto& arc : m_arcs )
        arc.Mirror( aX, aY, aRef );
}


void SHAPE_LINE_CHAIN::Mirror( const SEG& axis )
{
    for( auto& pt : m_points )
        pt = axis.ReflectPoint( pt );

    for( auto& arc : m_arcs )
        arc.Mirror( axis );
}


void SHAPE_LINE_CHAIN::Replace( int aStartIndex, int aEndIndex, const VECTOR2I& aP )
{
    Remove( aStartIndex, aEndIndex );
    Insert( aStartIndex, aP );
    assert( m_shapes.size() == m_points.size() );
}


void SHAPE_LINE_CHAIN::Replace( int aStartIndex, int aEndIndex, const SHAPE_LINE_CHAIN& aLine )
{
    if( aEndIndex < 0 )
        aEndIndex += PointCount();

    if( aStartIndex < 0 )
        aStartIndex += PointCount();

    // We only process lines in order in this house
    wxASSERT( aStartIndex <= aEndIndex );
    wxASSERT( aEndIndex < m_points.size() );

    SHAPE_LINE_CHAIN newLine = aLine;

    // Zero points to add?
    if( newLine.PointCount() == 0 )
    {
        Remove( aStartIndex, aEndIndex );
        return;
    }

    // Remove coincident points in the new line
    if( newLine.m_points.front() == m_points[aStartIndex] )
    {
        aStartIndex++;
        newLine.Remove( 0 );

        // Zero points to add?
        if( newLine.PointCount() == 0 )
        {
            Remove( aStartIndex, aEndIndex );
            return;
        }
    }

    if( newLine.m_points.back() == m_points[aEndIndex] && aEndIndex > 0 )
    {
        aEndIndex--;
        newLine.Remove( -1 );
    }

    Remove( aStartIndex, aEndIndex );

    // Zero points to add?
    if( newLine.PointCount() == 0 )
        return;

    // The total new arcs index is added to the new arc indices
    size_t prev_arc_count = m_arcs.size();
    std::vector<std::pair<ssize_t, ssize_t>> new_shapes = newLine.m_shapes;

    for( std::pair<ssize_t, ssize_t>& shape_pair : new_shapes )
    {
        alg::run_on_pair( shape_pair,
            [&]( ssize_t& aShape )
            {
                if( aShape != SHAPE_IS_PT )
                    aShape += prev_arc_count;
            } );
    }

    m_shapes.insert( m_shapes.begin() + aStartIndex, new_shapes.begin(), new_shapes.end() );
    m_points.insert( m_points.begin() + aStartIndex, newLine.m_points.begin(),
                     newLine.m_points.end() );
    m_arcs.insert( m_arcs.end(), newLine.m_arcs.begin(), newLine.m_arcs.end() );

    assert( m_shapes.size() == m_points.size() );
}


void SHAPE_LINE_CHAIN::Remove( int aStartIndex, int aEndIndex )
{
    assert( m_shapes.size() == m_points.size() );

    if( aEndIndex < 0 )
        aEndIndex += PointCount();

    if( aStartIndex < 0 )
        aStartIndex += PointCount();

    if( aStartIndex >= PointCount() )
        return;

    aEndIndex = std::min( aEndIndex, PointCount() - 1 );

    // Split arcs at start index and end just after the end index
    if( IsPtOnArc( aStartIndex ) )
        splitArc( aStartIndex );

    size_t nextIndex = static_cast<size_t>( aEndIndex ) + 1;

    if( IsPtOnArc( nextIndex ) )
        splitArc( nextIndex );

    std::set<size_t> extra_arcs;
    auto logArcIdxRemoval = [&]( ssize_t& aShapeIndex )
                            {
                                if( aShapeIndex != SHAPE_IS_PT )
                                    extra_arcs.insert( aShapeIndex );
                            };

    // Remove any overlapping arcs in the point range
    for( int i = aStartIndex; i <= aEndIndex; i++ )
    {
        if( IsSharedPt( i ) )
        {
            if( i == aStartIndex )
            {
                logArcIdxRemoval( m_shapes[i].second ); // Only remove the arc on the second index

                // Ensure that m_shapes has been built correctly.
                assert( i < aEndIndex || m_shapes[i + 1].first == m_shapes[i].second );

                continue;
            }
            else if( i == aEndIndex )
            {
                logArcIdxRemoval( m_shapes[i].first ); // Only remove the arc on the first index

                // Ensure that m_shapes has been built correctly.
                assert( i > aStartIndex || IsSharedPt( i - 1 )
                                ? m_shapes[i - 1].second == m_shapes[i].first
                                : m_shapes[i - 1].first == m_shapes[i].first );
                continue;
            }
        }

        alg::run_on_pair( m_shapes[i], logArcIdxRemoval );
    }

    for( auto arc : extra_arcs )
        convertArc( arc );

    m_shapes.erase( m_shapes.begin() + aStartIndex, m_shapes.begin() + aEndIndex + 1 );
    m_points.erase( m_points.begin() + aStartIndex, m_points.begin() + aEndIndex + 1 );
    assert( m_shapes.size() == m_points.size() );
}


int SHAPE_LINE_CHAIN::Distance( const VECTOR2I& aP, bool aOutlineOnly ) const
{
    return sqrt( SquaredDistance( aP, aOutlineOnly ) );
}


SEG::ecoord SHAPE_LINE_CHAIN_BASE::SquaredDistance( const VECTOR2I& aP, bool aOutlineOnly ) const
{
    ecoord d = VECTOR2I::ECOORD_MAX;

    if( IsClosed() && PointInside( aP ) && !aOutlineOnly )
        return 0;

    for( size_t s = 0; s < GetSegmentCount(); s++ )
        d = std::min( d, GetSegment( s ).SquaredDistance( aP ) );

    return d;
}


int SHAPE_LINE_CHAIN::Split( const VECTOR2I& aP )
{
    int ii = -1;
    int min_dist = 2;

    int found_index = Find( aP );

    for( int s = 0; s < SegmentCount(); s++ )
    {
        const SEG seg = CSegment( s );
        int dist = seg.Distance( aP );

        // make sure we are not producing a 'slightly concave' primitive. This might happen
        // if aP lies very close to one of already existing points.
        if( dist < min_dist && seg.A != aP && seg.B != aP )
        {
            min_dist = dist;
            if( found_index < 0 )
                ii = s;
            else if( s < found_index )
                ii = s;
        }
    }

    if( ii < 0 )
        ii = found_index;

    if( ii >= 0 )
    {
        // Don't create duplicate points
        if( GetPoint( ii ) == aP )
            return ii;

        size_t newIndex = static_cast<size_t>( ii ) + 1;

        if( IsArcSegment( ii ) )
        {
            m_points.insert( m_points.begin() + newIndex, aP );
            m_shapes.insert( m_shapes.begin() + newIndex, { ArcIndex( ii ), SHAPE_IS_PT } );
            splitArc( newIndex, true ); // Make the inserted point a shared point
        }
        else
        {
            Insert( newIndex, aP );
        }

        return newIndex;
    }

    return -1;
}


int SHAPE_LINE_CHAIN::Find( const VECTOR2I& aP, int aThreshold ) const
{
    for( int s = 0; s < PointCount(); s++ )
    {
        if( aThreshold == 0 )
        {
            if( CPoint( s ) == aP )
                return s;
        }
        else
        {
            if( (CPoint( s ) - aP).EuclideanNorm() <= aThreshold )
                return s;
        }
    }

    return -1;
}


int SHAPE_LINE_CHAIN::FindSegment( const VECTOR2I& aP, int aThreshold ) const
{
    for( int s = 0; s < SegmentCount(); s++ )
    {
        if( CSegment( s ).Distance( aP ) <= aThreshold )
            return s;
    }

    return -1;
}


int SHAPE_LINE_CHAIN::ShapeCount() const
{
    if( m_points.empty() )
        return 0;

    int numPoints = static_cast<int>( m_shapes.size() );
    int numShapes = 0;
    int arcIdx    = -1;

    for( int i = 0; i < m_points.size() - 1; i++ )
    {
        if( m_shapes[i] == SHAPES_ARE_PT )
        {
            numShapes++;
        }
        else
        {
            // Expect that the second index only gets populated when the point is shared between
            // two shapes. Otherwise, the shape index should always go on the first element of
            // the pair.
            assert( m_shapes[i].first != SHAPE_IS_PT );

            // Start assuming the point is shared with the previous arc
            // If so, the new/next arc index should be located at the second
            // element in the pair
            arcIdx = m_shapes[i].second;

            if( arcIdx == SHAPE_IS_PT )
                arcIdx = m_shapes[i].first; // Not a shared point

            numShapes++;

            // Now skip the rest of the arc
            while( i < numPoints && m_shapes[i].first == arcIdx )
                i++;

            // Add the "hidden" segment at the end of the arc, if it exists
            if( i < numPoints && m_points[i] != m_points[i - 1] )
            {
                numShapes++;
            }

            i--;
        }
    }

    return numShapes;
}


int SHAPE_LINE_CHAIN::NextShape( int aPointIndex, bool aForwards ) const
{
    if( aPointIndex < 0 )
        aPointIndex += PointCount();

    int lastIndex = PointCount() - 1;

    // First or last point?
    if( ( aForwards && aPointIndex == lastIndex ) ||
        ( !aForwards && aPointIndex == 0 ) )
    {
            return -1; // we don't want to wrap around
    }

    int delta = aForwards ? 1 : -1;

    if( m_shapes[aPointIndex] == SHAPES_ARE_PT )
        return aPointIndex + delta;

    int arcStart = aPointIndex;

    // The second element should only get populated when the point is shared between two shapes.
    // If not a shared point, then the index should always go on the first element.
    assert( m_shapes[aPointIndex].first != SHAPE_IS_PT );

    // Start with the assumption the point is shared
    auto arcIndex = [&]( int aIndex ) -> ssize_t
                    {
                        if( aForwards )
                            return ArcIndex( aIndex );
                        else
                            return reversedArcIndex( aIndex );
                    };

    ssize_t currentArcIdx = arcIndex( aPointIndex );

    // Now skip the rest of the arc
    while( aPointIndex < lastIndex && aPointIndex >= 0 && arcIndex( aPointIndex ) == currentArcIdx )
        aPointIndex += delta;

    if( aPointIndex == lastIndex )
    {
        if( !m_closed )
            return -1;
        else
            return lastIndex; // Segment between last point and the start
    }

    bool indexStillOnArc = alg::pair_contains( m_shapes[aPointIndex], currentArcIdx );

    // We want the last vertex of the arc if the initial point was the start of one
    // Well-formed arcs should generate more than one point to travel above
    if( aPointIndex - arcStart > 1 && !indexStillOnArc )
        aPointIndex -= delta;

    return aPointIndex;
}


void SHAPE_LINE_CHAIN::SetPoint( int aIndex, const VECTOR2I& aPos )
{
    if( aIndex < 0 )
        aIndex += PointCount();
    else if( aIndex >= PointCount() )
        aIndex -= PointCount();

    m_points[aIndex] = aPos;

    alg::run_on_pair( m_shapes[aIndex],
        [&]( ssize_t& aIdx )
        {
            if( aIdx != SHAPE_IS_PT )
                convertArc( aIdx );
        } );
}


void SHAPE_LINE_CHAIN::RemoveShape( int aPointIndex )
{
    if( aPointIndex < 0 )
        aPointIndex += PointCount();

    if( m_shapes[aPointIndex] == SHAPES_ARE_PT )
    {
        Remove( aPointIndex );
        return;
    }

    //@todo should this be replaced to use NextShape() / PrevShape()?
    int start  = aPointIndex;
    int end    = aPointIndex;
    int arcIdx = ArcIndex( aPointIndex );

    if( !IsSharedPt( aPointIndex ) )
    {
        // aPointIndex is not a shared point, so iterate backwards to find the start of the arc
        while( start >= 0 && m_shapes[start].first == arcIdx )
            start--;

        // Check if the previous point might be a shared point and decrement 'start' if so
        if( start >= 1 && m_shapes[static_cast<ssize_t>( start ) - 1].second == arcIdx )
            start--;
    }

    // For the end point we only need to check the first element in m_shapes (the second one is only
    // populated if there is an arc after the current one sharing the same point).
    while( end < static_cast<int>( m_shapes.size() ) - 1 && m_shapes[end].first == arcIdx )
        end++;

    Remove( start, end );
}


const SHAPE_LINE_CHAIN SHAPE_LINE_CHAIN::Slice( int aStartIndex, int aEndIndex ) const
{
    SHAPE_LINE_CHAIN rv;

    if( aEndIndex < 0 )
        aEndIndex += PointCount();

    if( aStartIndex < 0 )
        aStartIndex += PointCount();

    int numPoints = static_cast<int>( m_points.size() );


    if( IsArcSegment( aStartIndex ) && !IsArcStart( aStartIndex ) )
    {
        // Cutting in middle of an arc, lets split it
        ssize_t          arcIndex = ArcIndex( aStartIndex );
        const SHAPE_ARC& currentArc = Arc( arcIndex );

        // Copy the points as arc points
        for( size_t i = aStartIndex; arcIndex == ArcIndex( i ); i++ )
        {
            rv.m_points.push_back( m_points[i] );
            rv.m_shapes.push_back( { rv.m_arcs.size(), SHAPE_IS_PT } );
            rv.m_bbox.Merge( m_points[i] );
        }

        // Create a new arc from the existing one, with different start point.
        SHAPE_ARC newArc;

        VECTOR2I newArcStart = m_points[aStartIndex];

        newArc.ConstructFromStartEndCenter( newArcStart, currentArc.GetP1(),
                                             currentArc.GetCenter(),
                                             currentArc.IsClockwise() );


        rv.m_arcs.push_back( newArc );

        aStartIndex += rv.PointCount();
    }

    for( int i = aStartIndex; i <= aEndIndex && i < numPoints; i = NextShape( i ) )
    {
        if( i == -1 )
            return rv; // NextShape reached the end

        if( IsArcStart( i ) )
        {
            const SHAPE_ARC &currentArc = Arc( ArcIndex( i ) );
            int  nextShape = NextShape( i );
            bool isLastShape = nextShape < 0;

            if(  ( isLastShape && aEndIndex != ( numPoints - 1 ) )
                     || ( nextShape > aEndIndex ) )
            {
                if( i == aEndIndex )
                {
                    // Single point on an arc, just append the single point
                    rv.Append( m_points[i] );
                    return rv;
                }

                // Cutting in middle of an arc, lets split it
                ssize_t          arcIndex = ArcIndex( i );
                const SHAPE_ARC& currentArc = Arc( arcIndex );

                // Copy the points as arc points
                for( ; i <= aEndIndex && i < numPoints; i++ )
                {
                    if( arcIndex != ArcIndex( i ) )
                        break;

                    rv.m_points.push_back( m_points[i] );
                    rv.m_shapes.push_back( { rv.m_arcs.size(), SHAPE_IS_PT } );
                    rv.m_bbox.Merge( m_points[i] );
                }

                // Create a new arc from the existing one, with different end point.
                SHAPE_ARC newArc;

                VECTOR2I newArcEnd = m_points[aEndIndex];

                newArc.ConstructFromStartEndCenter( currentArc.GetP0(), newArcEnd,
                                                    currentArc.GetCenter(),
                                                    currentArc.IsClockwise() );


                rv.m_arcs.push_back( newArc );

                return rv;
            }
            else
            {
                // append the whole arc
                rv.Append( currentArc );
            }

            if( isLastShape )
                return rv;
        }
        else
        {
            wxASSERT_MSG( !IsArcSegment( i ), "Still on an arc segment, we missed something..." );

            rv.Append( m_points[i] );
        }
    }

    return rv;
}


void SHAPE_LINE_CHAIN::Append( const SHAPE_LINE_CHAIN& aOtherLine )
{
    assert( m_shapes.size() == m_points.size() );

    if( aOtherLine.PointCount() == 0 )
    {
        return;
    }

    size_t num_arcs = m_arcs.size();
    m_arcs.insert( m_arcs.end(), aOtherLine.m_arcs.begin(), aOtherLine.m_arcs.end() );

    auto fixShapeIndices =
            [&]( const std::pair<ssize_t, ssize_t>& aShapeIndices ) -> std::pair<ssize_t, ssize_t>
            {
                std::pair<ssize_t, ssize_t> retval =  aShapeIndices;

                alg::run_on_pair( retval, [&]( ssize_t& aIndex )
                                          {
                                              if( aIndex != SHAPE_IS_PT )
                                                  aIndex = aIndex + num_arcs;
                                          } );

                return retval;
            };

    if( PointCount() == 0 || aOtherLine.CPoint( 0 ) != CPoint( -1 ) )
    {
        const VECTOR2I p = aOtherLine.CPoint( 0 );
        m_points.push_back( p );
        m_shapes.push_back( fixShapeIndices( aOtherLine.CShapes()[0] ) );
        m_bbox.Merge( p );
    }
    else if( aOtherLine.IsArcSegment( 0 ) )
    {
        // Associate the new arc shape with the last point of this chain
        if( m_shapes.back() == SHAPES_ARE_PT )
            m_shapes.back().first = aOtherLine.CShapes()[0].first + num_arcs;
        else
            m_shapes.back().second = aOtherLine.CShapes()[0].first + num_arcs;
    }


    for( int i = 1; i < aOtherLine.PointCount(); i++ )
    {
        const VECTOR2I p = aOtherLine.CPoint( i );
        m_points.push_back( p );

        ssize_t arcIndex = aOtherLine.ArcIndex( i );

        if( arcIndex != ssize_t( SHAPE_IS_PT ) )
        {
            m_shapes.push_back( fixShapeIndices( aOtherLine.m_shapes[i] ) );
        }
        else
            m_shapes.push_back( SHAPES_ARE_PT );

        m_bbox.Merge( p );
    }

    assert( m_shapes.size() == m_points.size() );
}


void SHAPE_LINE_CHAIN::Append( const SHAPE_ARC& aArc )
{
    SEG startToEnd( aArc.GetP0(), aArc.GetP1() );

    if( startToEnd.Distance( aArc.GetArcMid() ) < 1 )
    {
        // Not really a valid arc. Add as a straight line segment instead
        Append( aArc.GetP0() );
        Append( aArc.GetP1() );
    }
    else
    {
        SHAPE_LINE_CHAIN chain = aArc.ConvertToPolyline();

        // @todo should the below 4 LOC be moved to SHAPE_ARC::ConvertToPolyline ?
        chain.m_arcs.push_back( aArc );
        chain.m_arcs.back().SetWidth( 0 );

        for( auto& sh : chain.m_shapes )
            sh.first = 0;

        Append( chain );
    }

    assert( m_shapes.size() == m_points.size() );
}


void SHAPE_LINE_CHAIN::Insert( size_t aVertex, const VECTOR2I& aP )
{
    if( aVertex == m_points.size() )
    {
        Append( aP );
        return;
    }

    wxCHECK( aVertex < m_points.size(), /* void */ );

    if( aVertex > 0 && IsPtOnArc( aVertex ) )
        splitArc( aVertex );

    //@todo need to check we aren't creating duplicate points
    m_points.insert( m_points.begin() + aVertex, aP );
    m_shapes.insert( m_shapes.begin() + aVertex, SHAPES_ARE_PT );

    assert( m_shapes.size() == m_points.size() );
}


void SHAPE_LINE_CHAIN::Insert( size_t aVertex, const SHAPE_ARC& aArc )
{
    wxCHECK( aVertex < m_points.size(), /* void */ );

    if( aVertex > 0 && IsPtOnArc( aVertex ) )
        splitArc( aVertex );

    /// Step 1: Find the position for the new arc in the existing arc vector
    ssize_t arc_pos = m_arcs.size();

    for( auto arc_it = m_shapes.rbegin() ;
              arc_it != m_shapes.rend() + aVertex;
              arc_it++ )
    {
        if( *arc_it != SHAPES_ARE_PT )
        {
            arc_pos = std::max( ( *arc_it ).first, ( *arc_it ).second );
            arc_pos++;
        }
    }

    //Increment all arc indices before inserting the new arc
    for( auto& sh : m_shapes )
    {
        alg::run_on_pair( sh,
            [&]( ssize_t& aIndex )
            {
                if( aIndex >= arc_pos )
                    aIndex++;
            } );
    }

    SHAPE_ARC arcCopy( aArc );
    arcCopy.SetWidth( 0 );
    m_arcs.insert( m_arcs.begin() + arc_pos, arcCopy );

    /// Step 2: Add the arc polyline points to the chain
    //@todo need to check we aren't creating duplicate points at start or end
    auto& chain = aArc.ConvertToPolyline();
    m_points.insert( m_points.begin() + aVertex, chain.CPoints().begin(), chain.CPoints().end() );

    /// Step 3: Add the vector of indices to the shape vector
    //@todo need to check we aren't creating duplicate points at start or end
    std::vector<std::pair<ssize_t, ssize_t>> new_points( chain.PointCount(),
                                                         { arc_pos, SHAPE_IS_PT } );

    m_shapes.insert( m_shapes.begin() + aVertex, new_points.begin(), new_points.end() );
    assert( m_shapes.size() == m_points.size() );
}


struct compareOriginDistance
{
    compareOriginDistance( const VECTOR2I& aOrigin ) :
        m_origin( aOrigin ) {};

    bool operator()( const SHAPE_LINE_CHAIN::INTERSECTION& aA,
                     const SHAPE_LINE_CHAIN::INTERSECTION& aB ) const
    {
        return ( m_origin - aA.p ).EuclideanNorm() < ( m_origin - aB.p ).EuclideanNorm();
    }

    VECTOR2I m_origin;
};


int SHAPE_LINE_CHAIN::Intersect( const SEG& aSeg, INTERSECTIONS& aIp ) const
{
    for( int s = 0; s < SegmentCount(); s++ )
    {
        OPT_VECTOR2I p = CSegment( s ).Intersect( aSeg );

        if( p )
        {
            INTERSECTION is;
            is.valid = true;
            is.index_our = s;
            is.index_their = -1;
            is.is_corner_our = is.is_corner_their = false;
            is.p = *p;
            aIp.push_back( is );
        }
    }

    compareOriginDistance comp( aSeg.A );
    sort( aIp.begin(), aIp.end(), comp );

    return aIp.size();
}


static inline void addIntersection( SHAPE_LINE_CHAIN::INTERSECTIONS& aIps, int aPc,
                                    const SHAPE_LINE_CHAIN::INTERSECTION& aP )
{
    if( aIps.size() == 0 )
    {
        aIps.push_back( aP );
        return;
    }

    const auto& last = aIps.back();

    aIps.push_back( aP );
}


int SHAPE_LINE_CHAIN::Intersect( const SHAPE_LINE_CHAIN& aChain, INTERSECTIONS& aIp,
                                 bool aExcludeColinearAndTouching ) const
{
    BOX2I bb_other = aChain.BBox();

    for( int s1 = 0; s1 < SegmentCount(); s1++ )
    {
        const SEG& a = CSegment( s1 );
        const BOX2I bb_cur( a.A, a.B - a.A );

        if( !bb_other.Intersects( bb_cur ) )
            continue;

        for( int s2 = 0; s2 < aChain.SegmentCount(); s2++ )
        {
            const SEG& b = aChain.CSegment( s2 );
            INTERSECTION is;

            is.index_our = s1;
            is.index_their = s2;
            is.is_corner_our = false;
            is.is_corner_their = false;
            is.valid = true;

            OPT_VECTOR2I p = a.Intersect( b );

            bool coll = a.Collinear( b );

            if( coll && ! aExcludeColinearAndTouching )
            {
                if( a.Contains( b.A ) )
                {
                    is.p = b.A;
                    is.is_corner_their = true;
                    addIntersection(aIp, PointCount(), is);
                }

                if( a.Contains( b.B ) )
                {
                    is.p = b.B;
                    is.index_their++;
                    is.is_corner_their = true;
                    addIntersection( aIp, PointCount(), is );
                }

                if( b.Contains( a.A ) )
                {
                    is.p = a.A;
                    is.is_corner_our = true;
                    addIntersection( aIp, PointCount(), is );
                }

                if( b.Contains( a.B ) )
                {
                    is.p = a.B;
                    is.index_our++;
                    is.is_corner_our = true;
                    addIntersection( aIp, PointCount(), is );
                }
            }
            else if( p )
            {
                is.p = *p;
                is.is_corner_our = false;
                is.is_corner_their = false;

                int distA = ( b.A - *p ).EuclideanNorm();
                int distB = ( b.B - *p ).EuclideanNorm();

                if( p == a.A )
                {
                    is.is_corner_our = true;
                }

                if( p == a.B )
                {
                    is.is_corner_our = true;
                    is.index_our++;
                }

                if( p == b.A )
                {
                    is.is_corner_their = true;
                }

                if( p == b.B )
                {
                    is.is_corner_their = true;
                    is.index_their++;
                }

                addIntersection( aIp, PointCount(), is );
            }
        }
    }

    return aIp.size();
}


int SHAPE_LINE_CHAIN::PathLength( const VECTOR2I& aP, int aIndex ) const
{
    int sum = 0;

    for( int i = 0; i < SegmentCount(); i++ )
    {
        const SEG seg = CSegment( i );
        int d = seg.Distance( aP );

        bool indexMatch = true;

        if( aIndex >= 0 )
        {
            if( aIndex == SegmentCount() )
            {
                indexMatch = ( i == SegmentCount() - 1 );
            }
            else
            {
                indexMatch = ( i == aIndex );
            }
        }

        if( indexMatch )
        {
            sum += ( aP - seg.A ).EuclideanNorm();
            return sum;
        }
        else
            sum += seg.Length();
    }

    return -1;
}


bool SHAPE_LINE_CHAIN_BASE::PointInside( const VECTOR2I& aPt, int aAccuracy,
                                         bool aUseBBoxCache ) const
{
    /*
     * Don't check the bounding box unless it's cached.  Building it is about the same speed as
     * the rigorous test below and so just slows things down by doing potentially two tests.
     */
    if( aUseBBoxCache && GetCachedBBox() && !GetCachedBBox()->Contains( aPt ) )
        return false;

    if( !IsClosed() || GetPointCount() < 3 )
        return false;

    bool inside = false;

    /*
     * To check for interior points, we draw a line in the positive x direction from
     * the point.  If it intersects an even number of segments, the point is outside the
     * line chain (it had to first enter and then exit).  Otherwise, it is inside the chain.
     *
     * Note: slope might be denormal here in the case of a horizontal line but we require our
     * y to move from above to below the point (or vice versa)
     *
     * Note: we open-code CPoint() here so that we don't end up calculating the size of the
     * vector number-of-points times.  This has a non-trivial impact on zone fill times.
     */
    int pointCount = GetPointCount();

    for( int i = 0; i < pointCount; )
    {
        const auto p1 = GetPoint( i++ );
        const auto p2 = GetPoint( i == pointCount ? 0 : i );
        const auto diff = p2 - p1;

        if( diff.y != 0 )
        {
            const int d = rescale( diff.x, ( aPt.y - p1.y ), diff.y );

            if( ( ( p1.y > aPt.y ) != ( p2.y > aPt.y ) ) && ( aPt.x - p1.x < d ) )
                inside = !inside;
        }
    }

    // If accuracy is <= 1 (nm) then we skip the accuracy test for performance.  Otherwise
    // we use "OnEdge(accuracy)" as a proxy for "Inside(accuracy)".
    if( aAccuracy <= 1 )
        return inside;
    else
        return inside || PointOnEdge( aPt, aAccuracy );
}


bool SHAPE_LINE_CHAIN_BASE::PointOnEdge( const VECTOR2I& aPt, int aAccuracy ) const
{
	return EdgeContainingPoint( aPt, aAccuracy ) >= 0;
}


int SHAPE_LINE_CHAIN_BASE::EdgeContainingPoint( const VECTOR2I& aPt, int aAccuracy ) const
{
    if( !GetPointCount() )
    {
		return -1;
    }
	else if( GetPointCount() == 1 )
    {
	    VECTOR2I dist = GetPoint(0) - aPt;
	    return ( hypot( dist.x, dist.y ) <= aAccuracy + 1 ) ? 0 : -1;
    }

    for( size_t i = 0; i < GetSegmentCount(); i++ )
    {
        const SEG s = GetSegment( i );

        if( s.A == aPt || s.B == aPt )
            return i;

        if( s.Distance( aPt ) <= aAccuracy + 1 )
            return i;
    }

    return -1;
}


bool SHAPE_LINE_CHAIN::CheckClearance( const VECTOR2I& aP, const int aDist ) const
{
    if( !PointCount() )
        return false;
    else if( PointCount() == 1 )
        return m_points[0] == aP;

    for( int i = 0; i < SegmentCount(); i++ )
    {
        const SEG s = CSegment( i );

        if( s.A == aP || s.B == aP )
            return true;

        if( s.Distance( aP ) <= aDist )
            return true;
    }

    return false;
}


const OPT<SHAPE_LINE_CHAIN::INTERSECTION> SHAPE_LINE_CHAIN::SelfIntersecting() const
{
    for( int s1 = 0; s1 < SegmentCount(); s1++ )
    {
        for( int s2 = s1 + 1; s2 < SegmentCount(); s2++ )
        {
            const VECTOR2I s2a = CSegment( s2 ).A, s2b = CSegment( s2 ).B;

            if( s1 + 1 != s2 && CSegment( s1 ).Contains( s2a ) )
            {
                INTERSECTION is;
                is.index_our = s1;
                is.index_their = s2;
                is.p = s2a;
                return is;
            }
            else if( CSegment( s1 ).Contains( s2b ) &&
                     // for closed polylines, the ending point of the
                     // last segment == starting point of the first segment
                     // this is a normal case, not self intersecting case
                     !( IsClosed() && s1 == 0 && s2 == SegmentCount()-1 ) )
            {
                INTERSECTION is;
                is.index_our = s1;
                is.index_their = s2;
                is.p = s2b;
                return is;
            }
            else
            {
                OPT_VECTOR2I p = CSegment( s1 ).Intersect( CSegment( s2 ), true );

                if( p )
                {
                    INTERSECTION is;
                    is.index_our = s1;
                    is.index_their = s2;
                    is.p = *p;
                    return is;
                }
            }
        }
    }

    return OPT<SHAPE_LINE_CHAIN::INTERSECTION>();
}


SHAPE_LINE_CHAIN& SHAPE_LINE_CHAIN::Simplify( bool aRemoveColinear )
{
    std::vector<VECTOR2I> pts_unique;
    std::vector<std::pair<ssize_t, ssize_t>> shapes_unique;

    if( PointCount() < 2 )
    {
        return *this;
    }
    else if( PointCount() == 2 )
    {
        if( m_points[0] == m_points[1] )
            m_points.pop_back();

        return *this;
    }

    int i = 0;
    int np = PointCount();

    // stage 1: eliminate duplicate vertices
    while( i < np )
    {
        int j = i + 1;

        // We can eliminate duplicate vertices as long as they are part of the same shape, OR if
        // one of them is part of a shape and one is not.
        while( j < np && m_points[i] == m_points[j] &&
               ( m_shapes[i] == m_shapes[j] ||
                 m_shapes[i] == SHAPES_ARE_PT ||
                 m_shapes[j] == SHAPES_ARE_PT ) )
        {
            j++;
        }

        std::pair<ssize_t,ssize_t> shapeToKeep = m_shapes[i];

        if( shapeToKeep == SHAPES_ARE_PT )
            shapeToKeep = m_shapes[j - 1];

        assert( shapeToKeep.first < static_cast<int>( m_arcs.size() ) );
        assert( shapeToKeep.second < static_cast<int>( m_arcs.size() ) );

        pts_unique.push_back( CPoint( i ) );
        shapes_unique.push_back( shapeToKeep );

        i = j;
    }

    m_points.clear();
    m_shapes.clear();
    np = pts_unique.size();

    i = 0;

    // stage 2: eliminate colinear segments
    while( i < np - 2 )
    {
        const VECTOR2I p0 = pts_unique[i];
        const VECTOR2I p1 = pts_unique[i + 1];
        int n = i;

        if( aRemoveColinear && shapes_unique[i] == SHAPES_ARE_PT
            && shapes_unique[i + 1] == SHAPES_ARE_PT )
        {
            while( n < np - 2
                    && ( SEG( p0, p1 ).LineDistance( pts_unique[n + 2] ) <= 1
                            || SEG( p0, p1 ).Collinear( SEG( p1, pts_unique[n + 2] ) ) ) )
                n++;
        }

        m_points.push_back( p0 );
        m_shapes.push_back( shapes_unique[i] );

        if( n > i )
            i = n;

        if( n == np - 2 )
        {
            m_points.push_back( pts_unique[np - 1] );
            m_shapes.push_back( shapes_unique[np - 1] );
            return *this;
        }

        i++;
    }

    if( np > 1 )
    {
        m_points.push_back( pts_unique[np - 2] );
        m_shapes.push_back( shapes_unique[np - 2] );
    }

    m_points.push_back( pts_unique[np - 1] );
    m_shapes.push_back( shapes_unique[np - 1] );

    assert( m_points.size() == m_shapes.size() );

    return *this;
}


const VECTOR2I SHAPE_LINE_CHAIN::NearestPoint( const VECTOR2I& aP,
                                               bool aAllowInternalShapePoints ) const
{
    int min_d = INT_MAX;
    int nearest = 0;

    for( int i = 0; i < SegmentCount(); i++ )
    {
        int d = CSegment( i ).Distance( aP );

        if( d < min_d )
        {
            min_d = d;
            nearest = i;
        }
    }

    if( !aAllowInternalShapePoints )
    {
        //Snap to arc end points if the closest found segment is part of an arc segment
        if( nearest > 0 && nearest < PointCount() && IsArcSegment( nearest ) )
        {
            VECTOR2I ptToSegStart = CSegment( nearest ).A - aP;
            VECTOR2I ptToSegEnd = CSegment( nearest ).B - aP;

            if( ptToSegStart.EuclideanNorm() > ptToSegEnd.EuclideanNorm() )
                nearest++;

            // Is this the start or end of an arc?  If so, return it directly
            if( IsArcStart( nearest ) || IsArcEnd( nearest ) )
            {
                return m_points[nearest];
            }
            else
            {
                const SHAPE_ARC& nearestArc = Arc( ArcIndex( nearest ) );
                VECTOR2I         ptToArcStart = nearestArc.GetP0() - aP;
                VECTOR2I         ptToArcEnd = nearestArc.GetP1() - aP;

                if( ptToArcStart.EuclideanNorm() > ptToArcEnd.EuclideanNorm() )
                    return nearestArc.GetP1();
                else
                    return nearestArc.GetP0();
            }

        }
    }

    return CSegment( nearest ).NearestPoint( aP );
}


const VECTOR2I SHAPE_LINE_CHAIN::NearestPoint( const SEG& aSeg, int& dist ) const
{
    int nearest = 0;

    dist = INT_MAX;

    for( int i = 0; i < PointCount(); i++ )
    {
        int d = aSeg.LineDistance( CPoint( i ) );

        if( d < dist )
        {
            dist = d;
            nearest = i;
        }
    }

    return CPoint( nearest );
}


int SHAPE_LINE_CHAIN::NearestSegment( const VECTOR2I& aP ) const
{
    int min_d = INT_MAX;
    int nearest = 0;

    for( int i = 0; i < SegmentCount(); i++ )
    {
        int d = CSegment( i ).Distance( aP );

        if( d < min_d )
        {
            min_d = d;
            nearest = i;
        }
    }

    return nearest;
}


const std::string SHAPE_LINE_CHAIN::Format() const
{
    std::stringstream ss;

    ss << "SHAPE_LINE_CHAIN( { ";

    for( int i = 0; i < PointCount(); i++ )
    {
        ss << "VECTOR2I( " << m_points[i].x << ", " << m_points[i].y << ")";

        if( i != PointCount() -1 )
            ss << ", ";
    }

    ss << "}, " << ( m_closed ? "true" : "false" );
    ss << " );";

    return ss.str();

   /* fixme: arcs
    for( size_t i = 0; i < m_arcs.size(); i++ )
        ss << m_arcs[i].GetCenter().x << " " << m_arcs[i].GetCenter().y << " "
        << m_arcs[i].GetP0().x << " " << m_arcs[i].GetP0().y << " "
        << m_arcs[i].GetCentralAngle();

    return ss.str();*/
}


bool SHAPE_LINE_CHAIN::CompareGeometry( const SHAPE_LINE_CHAIN & aOther ) const
{
    SHAPE_LINE_CHAIN a(*this), b( aOther );
    a.Simplify();
    b.Simplify();

    if( a.m_points.size() != b.m_points.size() )
        return false;

    for( int i = 0; i < a.PointCount(); i++ )
    {
        if( a.CPoint( i ) != b.CPoint( i ) )
            return false;
    }

    return true;
}


bool SHAPE_LINE_CHAIN::Intersects( const SHAPE_LINE_CHAIN& aChain ) const
{
    INTERSECTIONS dummy;
    return Intersect( aChain, dummy ) != 0;
}


SHAPE* SHAPE_LINE_CHAIN::Clone() const
{
    return new SHAPE_LINE_CHAIN( *this );
}


bool SHAPE_LINE_CHAIN::Parse( std::stringstream& aStream )
{
    size_t n_pts;
    size_t n_arcs;

    m_points.clear();
    aStream >> n_pts;

    // Rough sanity check, just make sure the loop bounds aren't absolutely outlandish
    if( n_pts > aStream.str().size() )
        return false;

    aStream >> m_closed;
    aStream >> n_arcs;

    if( n_arcs > aStream.str().size() )
        return false;

    for( size_t i = 0; i < n_pts; i++ )
    {
        int x, y;
        ssize_t ind;
        aStream >> x;
        aStream >> y;
        m_points.emplace_back( x, y );

        aStream >> ind;
        m_shapes.emplace_back( std::make_pair( ind, SHAPE_IS_PT ) );
    }

    for( size_t i = 0; i < n_arcs; i++ )
    {
        VECTOR2I p0, pc;
        double angle;

        aStream >> pc.x;
        aStream >> pc.y;
        aStream >> p0.x;
        aStream >> p0.y;
        aStream >> angle;

        m_arcs.emplace_back( pc, p0, angle );
    }

    return true;
}


const VECTOR2I SHAPE_LINE_CHAIN::PointAlong( int aPathLength ) const
{
    int total = 0;

    if( aPathLength == 0 )
        return CPoint( 0 );

    for( int i = 0; i < SegmentCount(); i++ )
    {
        const SEG& s = CSegment( i );
        int l = s.Length();

        if( total + l >= aPathLength )
        {
            VECTOR2I d( s.B - s.A );
            return s.A + d.Resize( aPathLength - total );
        }

        total += l;
    }

    return CPoint( -1 );
}


double SHAPE_LINE_CHAIN::Area( bool aAbsolute ) const
{
    // see https://www.mathopenref.com/coordpolygonarea2.html

    if( !m_closed )
        return 0.0;

    double area = 0.0;
    int size = m_points.size();

    for( int i = 0, j = size - 1; i < size; ++i )
    {
        area += ( (double) m_points[j].x + m_points[i].x ) *
                ( (double) m_points[j].y - m_points[i].y );
        j = i;
    }

    if( aAbsolute )
        return std::fabs( area * 0.5 ); // The result would be negative if points are anti-clockwise
    else
        return -area * 0.5; // The result would be negative if points are anti-clockwise
}


SHAPE_LINE_CHAIN::POINT_INSIDE_TRACKER::POINT_INSIDE_TRACKER( const VECTOR2I& aPoint ) :
    m_point( aPoint ),
    m_finished( false ),
    m_state( 0 ),
    m_count( 0 )
{
}


bool SHAPE_LINE_CHAIN::POINT_INSIDE_TRACKER::processVertex(
        const VECTOR2I& ip, const VECTOR2I& ipNext )
{
    if( ipNext.y == m_point.y )
    {
        if( ( ipNext.x == m_point.x )
                || ( ip.y == m_point.y && ( ( ipNext.x > m_point.x ) == ( ip.x < m_point.x ) ) ) )
        {
            m_finished = true;
            m_state    = -1;
            return false;
        }
    }

    if( ( ip.y < m_point.y ) != ( ipNext.y < m_point.y ) )
    {
        if( ip.x >= m_point.x )
        {
            if( ipNext.x > m_point.x )
            {
                m_state = 1 - m_state;
            }
            else
            {
                double d = (double) ( ip.x - m_point.x ) * ( ipNext.y - m_point.y )
                           - (double) ( ipNext.x - m_point.x ) * ( ip.y - m_point.y );

                if( !d )
                {
                    m_finished = true;
                    m_state    = -1;
                    return false;
                }

                if( ( d > 0 ) == ( ipNext.y > ip.y ) )
                    m_state = 1 - m_state;
            }
        }
        else
        {
            if( ipNext.x > m_point.x )
            {
                double d = (double) ( ip.x - m_point.x ) * ( ipNext.y - m_point.y )
                           - (double) ( ipNext.x - m_point.x ) * ( ip.y - m_point.y );

                if( !d )
                {
                    m_finished = true;
                    m_state    = -1;
                    return false;
                }

                if( ( d > 0 ) == ( ipNext.y > ip.y ) )
                    m_state = 1 - m_state;
            }
        }
    }

    return true;
}


void SHAPE_LINE_CHAIN::POINT_INSIDE_TRACKER::AddPolyline( const SHAPE_LINE_CHAIN& aPolyline )
{
    if( !m_count )
    {
        m_lastPoint = aPolyline.CPoint( 0 );
        m_firstPoint = aPolyline.CPoint( 0 );
    }

    m_count += aPolyline.PointCount();

    for( int i = 1; i < aPolyline.PointCount(); i++ )
    {
        auto p = aPolyline.CPoint( i );

        if( !processVertex( m_lastPoint, p ) )
            return;

        m_lastPoint = p;
    }

}


bool SHAPE_LINE_CHAIN::POINT_INSIDE_TRACKER::IsInside()
{
    processVertex( m_lastPoint, m_firstPoint );
    return m_state > 0;
}
