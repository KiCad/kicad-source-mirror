/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
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

/**
 * @file geometry_utils.cpp
 * @brief a few functions useful in geometry calculations.
 */

#include <cstdint>
#include <algorithm>         // for max, min

#include <geometry/geometry_utils.h>
#include <math/util.h>      // for KiROUND

// To approximate a circle by segments, a minimal seg count is mandatory.
// Note that this is rarely used as the maxError constraint will yield a higher
// segment count on everything but very small circles.  (Even a 0.125mm track
// with a 0.01mm maximum deviation yields 11 segments.)
#define MIN_SEGCOUNT_FOR_CIRCLE 8

int GetArcToSegmentCount( int aRadius, int aErrorMax, const EDA_ANGLE& aArcAngle )
{
    // calculate the number of segments to approximate a circle by segments
    // given the max distance between the middle of a segment and the circle

    // avoid divide-by-zero
    aRadius = std::max( 1, aRadius );
    aErrorMax = std::max( 1, aErrorMax );

    // error relative to the radius value:
    double rel_error = (double)aErrorMax / aRadius;
    // minimal arc increment in degrees:
    double arc_increment = 180 / M_PI * acos( 1.0 - rel_error ) * 2;

    // Ensure a minimal arc increment reasonable value for a circle
    // (360.0 degrees). For very small radius values, this is mandatory.
    arc_increment = std::min( 360.0/MIN_SEGCOUNT_FOR_CIRCLE, arc_increment );

    int segCount = KiROUND( fabs( aArcAngle.AsDegrees() ) / arc_increment );

    // Ensure at least two segments are used for algorithmic safety
    return std::max( segCount, 2 );
}


int CircleToEndSegmentDeltaRadius( int aRadius, int aSegCount )
{
    // The minimal seg count is 3, otherwise we cannot calculate the result
    // in practice, the min count is clamped to 8 in kicad
    if( aSegCount <= 2 )
        aSegCount = 3;

    // The angle between the center of the segment and one end of the segment
    // when the circle is approximated by aSegCount segments
    double alpha = M_PI / aSegCount;

    // aRadius is the radius of the circle tangent to the middle of each segment
    // and aRadius/cos(aplha) is the radius of the circle defined by seg ends
    int delta = KiROUND( std::abs( aRadius * ( 1 - 1/cos( alpha ) ) ) );

    return delta;
}

// When creating polygons to create a clearance polygonal area, the polygon must
// be same or bigger than the original shape.
// Polygons are bigger if the original shape has arcs (round rectangles, ovals,
// circles...).  However, when building the solder mask layer modifying the shapes
// when converting them to polygons is not acceptable (the modification can break
// calculations).
// So one can disable the shape expansion within a particular scope by allocating
// a DISABLE_ARC_CORRECTION.

static bool s_disable_arc_correction = false;

DISABLE_ARC_RADIUS_CORRECTION::DISABLE_ARC_RADIUS_CORRECTION()
{
    s_disable_arc_correction = true;
}

DISABLE_ARC_RADIUS_CORRECTION::~DISABLE_ARC_RADIUS_CORRECTION()
{
    s_disable_arc_correction = false;
}

int GetCircleToPolyCorrection( int aMaxError )
{
    // Push all the error to the outside by increasing the radius
    return s_disable_arc_correction ? 0 : aMaxError;
}


/***
 * Utility for the line clipping code, returns the boundary code of
 * a point. Bit allocation is arbitrary
 */
inline int clipOutCode( const BOX2I *aClipBox, int x, int y )
{
    int code;

    if( y < aClipBox->GetY() )
        code = 2;
    else if( y > aClipBox->GetBottom() )
        code = 1;
    else
        code = 0;

    if( x < aClipBox->GetX() )
        code |= 4;
    else if( x > aClipBox->GetRight() )
        code |= 8;

    return code;
}


bool ClipLine( const BOX2I *aClipBox, int &x1, int &y1, int &x2, int &y2 )
{
    // Stock Cohen-Sutherland algorithm; check *any* CG book for details
    int outcode1 = clipOutCode( aClipBox, x1, y1 );
    int outcode2 = clipOutCode( aClipBox, x2, y2 );

    while( outcode1 || outcode2 )
    {
        // Fast reject
        if( outcode1 & outcode2 )
            return true;

        // Choose a side to clip
        int thisoutcode, x, y;

        if( outcode1 )
            thisoutcode = outcode1;
        else
            thisoutcode = outcode2;

        /* One clip round
         * Since we use the full range of 32 bit ints, the proportion
         * computation has to be done in 64 bits to avoid horrible
         * results */
        if( thisoutcode & 1 ) // Clip the bottom
        {
            y = aClipBox->GetBottom();
            x = x1 + (x2 - x1) * std::int64_t(y - y1) / (y2 - y1);
        }
        else if( thisoutcode & 2 ) // Clip the top
        {
            y = aClipBox->GetY();
            x = x1 + ( x2 - x1 ) * std::int64_t( y - y1 ) / ( y2 - y1 );
        }
        else if( thisoutcode & 8 ) // Clip the right
        {
            x = aClipBox->GetRight();
            y = y1 + ( y2 - y1 ) * std::int64_t( x - x1 ) / ( x2 - x1 );
        }
        else // if( thisoutcode & 4), obviously, clip the left
        {
            x = aClipBox->GetX();
            y = y1 + ( y2 - y1 ) * std::int64_t( x - x1 ) / ( x2 - x1 );
        }

        // Put the result back and update the boundary code
        // No ambiguity, otherwise it would have been a fast reject
        if( thisoutcode == outcode1 )
        {
            x1 = x;
            y1 = y;
            outcode1 = clipOutCode( aClipBox, x1, y1 );
        }
        else
        {
            x2 = x;
            y2 = y;
            outcode2 = clipOutCode( aClipBox, x2, y2 );
        }
    }

    return false;
}


bool KIGEOM::BoxHitTest( const VECTOR2I& aHitter, const BOX2I& aHittee, int aAccuracy )
{
    const BOX2I hittee = aHittee.GetInflated( aAccuracy );
    return hittee.Contains( aHitter );
}


bool KIGEOM::BoxHitTest( const BOX2I& aHitter, const BOX2I& aHittee, bool aHitteeContained,
                         int aAccuracy )
{
    const BOX2I hitter = aHitter.GetInflated( aAccuracy );

    if( aHitteeContained )
        return hitter.Contains( aHittee );

    return hitter.Intersects( aHittee );
}


bool KIGEOM::BoxHitTest( const SHAPE_LINE_CHAIN& aHitter, const BOX2I& aHittee, bool aHitteeContained )
{
    SHAPE_RECT bbox( aHittee );

    return KIGEOM::ShapeHitTest( aHitter, bbox, aHitteeContained );
}


bool KIGEOM::BoxHitTest( const SHAPE_LINE_CHAIN& aHitter, const BOX2I& aHittee, const EDA_ANGLE& aHitteeRotation,
                         const VECTOR2I& aHitteeRotationCenter, bool aHitteeContained )
{
    // Optimization: use SHAPE_RECT collision test if possible
    if( aHitteeRotation.IsZero() )
    {
        return KIGEOM::BoxHitTest( aHitter, aHittee, aHitteeContained );
    }
    else if( aHitteeRotation.IsCardinal() )
    {
        BOX2I box = aHittee.GetBoundingBoxRotated( aHitteeRotationCenter, aHitteeRotation );
        return KIGEOM::BoxHitTest( aHitter, box, aHitteeContained );
    }

    // Non-cardinal angle: convert to simple polygon and rotate
    const std::vector<VECTOR2I> corners =
    {
        aHittee.GetOrigin(),
        VECTOR2I( aHittee.GetRight(), aHittee.GetTop() ),
        aHittee.GetEnd(),
        VECTOR2I( aHittee.GetLeft(), aHittee.GetBottom() )
    };

    SHAPE_SIMPLE shape( corners );
    shape.Rotate( aHitteeRotation, aHitteeRotationCenter );

    return KIGEOM::ShapeHitTest( aHitter, shape, aHitteeContained );
}


bool KIGEOM::ShapeHitTest( const SHAPE_LINE_CHAIN& aHitter, const SHAPE& aHittee, bool aHitteeContained )
{
    // Check if the selection polygon collides with any of the hittee's subshapes.
    auto collidesAny =
            [&]()
            {
                return aHittee.Collide( &aHitter );
            };

    // Check if the selection polygon collides with all of the hittee's subshapes.
    auto collidesAll =
            [&]()
            {
                if( const auto compoundHittee = dynamic_cast<const SHAPE_COMPOUND*>( &aHittee ) )
                {
                    // If the hittee is a compound shape, all subshapes must collide.
                    return std::ranges::all_of(
                                compoundHittee->Shapes(),
                                [&]( const SHAPE* subshape )
                                {
                                    return subshape && subshape->Collide( &aHitter );
                                } );
                }
                else
                {
                    // If the hittee is a simple shape, we can check it directly.
                    return aHittee.Collide( &aHitter );
                }
            };

    // Check if the selection polygon outline collides with the hittee's shape.
    auto intersectsAny =
            [&]()
            {
                const int count = aHitter.SegmentCount();

                for( int i = 0; i < count; ++i )
                {
                    if( aHittee.Collide( aHitter.CSegment( i ) ) )
                        return true;
                }

                return false;
            };

    if( aHitter.IsClosed() )
    {
        if( aHitteeContained )
            // Containing polygon - all of the subshapes must collide with the selection polygon,
            // but none of them can intersect its outline.
            return collidesAll() && !intersectsAny();
        else
            // Touching polygon - any of the subshapes should collide with the selection polygon.
            return collidesAny();
    }
    else
    {
        // Touching (poly)line - any of the subshapes should intersect the selection polyline.
        return intersectsAny();
    }
}
