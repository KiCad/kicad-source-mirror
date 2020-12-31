/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 KiCad Developers, see AUTHORS.txt for contributors.
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
 * with this program.  If not, see <http://www.gnu.or/licenses/>.
 */

#include <geometry/direction45.h>

const SHAPE_LINE_CHAIN DIRECTION_45::BuildInitialTrace( const VECTOR2I& aP0, const VECTOR2I& aP1,
                                                        bool aStartDiagonal, bool aFillet ) const
{
    bool startDiagonal;

    if( m_dir == UNDEFINED )
        startDiagonal = aStartDiagonal;
    else
        startDiagonal = IsDiagonal();

    int w = abs( aP1.x - aP0.x );
    int h = abs( aP1.y - aP0.y );
    int sw = sign( aP1.x - aP0.x );
    int sh = sign( aP1.y - aP0.y );

    VECTOR2I mp0, mp1;

    /*
     * Non-filleted case:
     *
     * For width greater than height, we're calculating something like this.
     * mp0 will be used if we start straight; mp1 if we start diagonal.
     *
     * aP0 ----------------- mp0
     *  .                     \
     *   .                     \
     *    .                     \
     *     mp1 . . . . . . . .  aP1
     *
     * Filleted case:
     *
     * For a fillet, we need to know the arc start point (A in the diagram below)
     * A straight segment will be needed between aP0 and A if we are starting straight,
     * or between the arc end and aP1 if we are starting diagonally.
     *
     * aP0 -- A --___        mp0
     *  .             ---
     *   .                 --
     *    .                   --
     *     mp1 . . . . . . . .  aP1
     *
     * For the length of this segment (tangentLength), we subtract the length of the "diagonal"
     * line from the "straight" line (i.e. dist(aP0, mp0) - dist(mp0, aP1))
     * In the example above, we will have a straight segment from aP0 to A, and then we can use
     * the distance from A to aP1 (diagLength) to calculate the radius of the arc.
     */

    int tangentLength;

    if( w > h )
    {
        mp0 = VECTOR2I( ( w - h ) * sw, 0 );    // direction: E
        mp1 = VECTOR2I( h * sw, h * sh );       // direction: NE
        tangentLength = ( w - h ) - mp1.EuclideanNorm();
    }
    else
    {
        mp0 = VECTOR2I( 0, sh * ( h - w ) );    // direction: N
        mp1 = VECTOR2I( sw * w, sh * w );       // direction: NE
        tangentLength = ( h - w ) - mp1.EuclideanNorm();
    }

    SHAPE_LINE_CHAIN pl;

    // TODO: if tangentLength zero, we could still place a small arc at the start...
    if( aFillet )
    {
        VECTOR2I arcStart;
        VECTOR2D arcCenter;

        double diag2 = tangentLength >= 0 ? mp1.SquaredEuclideanNorm() : mp0.SquaredEuclideanNorm();
        double diagLength = std::sqrt( ( 2 * diag2 ) - ( 2 * diag2 * std::cos( 3 * M_PI_4 ) ) );
        int    arcRadius  = KiROUND( diagLength / ( 2.0 * std::cos( 67.5 * M_PI / 180.0 ) ) );

        // There are four different ways to build an arc, depending on whether or not we are
        // starting diagonally and whether or not we have a negative tangent length (meaning the
        // arc has to be on the opposite end of the line from normal).  This math could probably
        // be condensed and optimized but I'm tired of staring at it for now (and it works!)

        if( startDiagonal )
        {
            int      rotationSign = ( w > h ) ? ( sw * sh * -1 ) : ( sw * sh );
            VECTOR2D centerDir( mp0.Rotate( M_PI_2 * rotationSign ) );

            if( tangentLength >= 0 )
            {
                // Positive tangentLength, diagonal start: arc goes at the start
                arcStart  = aP0 + mp1 + mp0.Resize( mp1.EuclideanNorm() );
                arcCenter = arcStart + centerDir.Resize( arcRadius );
                SHAPE_ARC new_arc( arcCenter, aP0, 45 * rotationSign );
                pl.Append( new_arc );
                pl.Append( aP1 );
            }
            else
            {
                // Negative tangentLength, diagonal start: arc goes at the end
                arcStart  = aP0 + mp1.Resize( std::abs( tangentLength ) );
                arcCenter = aP1 + centerDir.Resize( arcRadius );

                SHAPE_ARC new_arc;
                new_arc.ConstructFromStartEndAngle( arcStart, aP1, 45 * rotationSign );

                pl.Append( aP0 );
                pl.Append( new_arc );
            }
        }
        else
        {
            int      rotationSign = ( w > h ) ? ( sw * sh ) : ( sw * sh * -1 );
            VECTOR2D centerDir( mp0.Rotate( M_PI_2 * rotationSign ) );

            if( tangentLength >= 0 )
            {
                // Positive tangentLength: arc goes at the end
                arcStart  = aP0 + mp0.Resize( tangentLength );
                arcCenter = arcStart + centerDir.Resize( arcRadius );

                SHAPE_ARC new_arc;
                new_arc.ConstructFromStartEndAngle( arcStart, aP1, 45 * rotationSign );

                pl.Append( aP0 );
                pl.Append( new_arc );
            }
            else
            {
                // Negative tangentLength: arc goes at the start
                arcStart  = aP0;
                arcCenter = arcStart + centerDir.Resize( arcRadius );
                SHAPE_ARC new_arc( arcCenter, arcStart, 45 * rotationSign );

                pl.Append( new_arc );
                pl.Append( aP1 );
            }
        }
    }
    else
    {
        pl.Append( aP0 );
        pl.Append( startDiagonal ? ( aP0 + mp1 ) : ( aP0 + mp0 ) );
        pl.Append( aP1 );
    }

    pl.Simplify();
    return pl;
}
