/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <eda_rect.h>
#include <geometry/geometry_utils.h>
#include <math/util.h>      // for KiROUND

// To approximate a circle by segments, a minimal seg count is mandatory.
// Note that this is rarely used as the maxError constraint will yield a higher
// segment count on everything but very small circles.  (Even a 0.125mm track
// with a 0.01mm maximum deviation yields 11 segments.)
#define MIN_SEGCOUNT_FOR_CIRCLE 8

int GetArcToSegmentCount( int aRadius, int aErrorMax, double aArcAngleDegree )
{
    // calculate the number of segments to approximate a circle by segments
    // given the max distance between the middle of a segment and the circle

    // avoid divide-by-zero
    aRadius = std::max( 1, aRadius );

    // error relative to the radius value:
    double rel_error = (double)aErrorMax / aRadius;
    // minimal arc increment in degrees:
    double arc_increment = 180 / M_PI * acos( 1.0 - rel_error ) * 2;

    // Ensure a minimal arc increment reasonable value for a circle
    // (360.0 degrees). For very small radius values, this is mandatory.
    arc_increment = std::min( 360.0/MIN_SEGCOUNT_FOR_CIRCLE, arc_increment );

    int segCount = KiROUND( fabs( aArcAngleDegree ) / arc_increment );

    // Ensure at least two segments are used for algorithmic safety
    return std::max( segCount, 2 );
}


int GetCircleToSegmentError( int aRadius, int aSegCount )
{
    // This is similar to the "inverse" of GetArcToSegmentCount()

    // The minimal seg count is 2, giving error = aRadius
    if( aSegCount <= 2 )
        return aRadius;

    double alpha = M_PI / aSegCount;
    int error = KiROUND( aRadius * ( 1.0 - cos( alpha) ) );

    return error;
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
inline int clipOutCode( const EDA_RECT *aClipBox, int x, int y )
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


bool ClipLine( const EDA_RECT *aClipBox, int &x1, int &y1, int &x2, int &y2 )
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

