/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
#include <trigo.h>


const SHAPE_LINE_CHAIN DIRECTION_45::BuildInitialTrace( const VECTOR2I& aP0, const VECTOR2I& aP1,
                                                        bool        aStartDiagonal,
                                                        CORNER_MODE aMode ) const

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

    bool is90mode = aMode == CORNER_MODE::ROUNDED_90 || aMode == CORNER_MODE::MITERED_90;
    SHAPE_LINE_CHAIN pl;

    // Shortcut where we can generate just one segment and quit.  Avoids more complicated handling
    // of precision errors if filleting is enabled
    if( w == 0 || h == 0 || ( !is90mode && h == w ) )
    {
        pl.Append( aP0 );
        pl.Append( aP1 );
        return pl;
    }

    VECTOR2I mp0, mp1;
    int      tangentLength = 0;

    if( is90mode )
    {
        if( startDiagonal == ( h >= w ) )
        {
            mp0 = VECTOR2I( w * sw, 0 ); // direction: E
        }
        else
        {
            mp0 = VECTOR2I( 0, sh * h ); // direction: N
        }
    }
    else
    {
        if( w > h )
        {
            mp0 = VECTOR2I( ( w - h ) * sw, 0 ); // direction: E
            mp1 = VECTOR2I( h * sw, h * sh );    // direction: NE
            tangentLength = ( w - h ) - mp1.EuclideanNorm();
        }
        else
        {
            mp0 = VECTOR2I( 0, sh * ( h - w ) ); // direction: N
            mp1 = VECTOR2I( sw * w, sh * w );    // direction: NE
            tangentLength = ( h - w ) - mp1.EuclideanNorm();
        }
    }

    SHAPE_ARC arc;
    VECTOR2I  arcEndpoint;

    switch( aMode )
    {
    case CORNER_MODE::MITERED_45:
        /*
         * For width greater than height, we're calculating something like this.
         * mp0 will be used if we start straight; mp1 if we start diagonal.
         *
         * aP0 ----------------- mp0
         *  .                     \
         *   .                     \
         *    .                     \
         *     mp1 . . . . . . . .  aP1
         *
         */
        pl.Append( aP0 );
        pl.Append( startDiagonal ? ( aP0 + mp1 ) : ( aP0 + mp0 ) );
        pl.Append( aP1 );
        break;

    case CORNER_MODE::ROUNDED_45:
    {
        /*
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
        if( w == h )
        {
            pl.Append( aP0 );
            pl.Append( aP1 );
            break;
        }

        double diag2 = tangentLength >= 0 ? mp1.SquaredEuclideanNorm() : mp0.SquaredEuclideanNorm();
        double diagLength = std::sqrt( ( 2 * diag2 ) - ( 2 * diag2 * std::cos( 3 * M_PI_4 ) ) );
        int    arcRadius  = KiROUND( diagLength / ( 2.0 * std::cos( 67.5 * M_PI / 180.0 ) ) );

        // There are four different ways to build an arc, depending on whether or not we are
        // starting diagonally and whether or not we have a negative tangent length (meaning the
        // arc has to be on the opposite end of the line from normal).  This math could probably
        // be condensed and optimized but I'm tired of staring at it for now (and it works!)

        if( startDiagonal )
        {
            int rotationSign = ( w > h ) ? ( sw * sh * -1 ) : ( sw * sh );

            if( tangentLength >= 0 )
            {
                // Positive tangentLength, diagonal start: arc goes at the start
                arcEndpoint = aP1 - mp0.Resize( tangentLength );
                arc.ConstructFromStartEndAngle( aP0, arcEndpoint, ANGLE_45 * rotationSign );

                if( arc.GetP0() == arc.GetP1() )
                    pl.Append( aP0 );
                else
                    pl.Append( arc );

                pl.Append( aP1 );
            }
            else
            {
                // Negative tangentLength, diagonal start: arc goes at the end
                arcEndpoint = aP0 + mp1.Resize( std::abs( tangentLength ) );
                arc.ConstructFromStartEndAngle( arcEndpoint, aP1, ANGLE_45 * rotationSign );

                pl.Append( aP0 );

                if( arc.GetP0() == arc.GetP1() )
                    pl.Append( aP1 );
                else
                    pl.Append( arc );
            }
        }
        else
        {
            int      rotationSign = ( w > h ) ? ( sw * sh * -1 ) : ( sw * sh );
            VECTOR2D centerDir( mp0 );

            RotatePoint( centerDir, ANGLE_90 * rotationSign );

            if( tangentLength >= 0 )
            {
                // Positive tangentLength: arc goes at the end
                arcEndpoint = aP0 + mp0.Resize( tangentLength );
                arc.ConstructFromStartEndAngle( arcEndpoint, aP1, - ANGLE_45 * rotationSign );

                pl.Append( aP0 );

                if( arc.GetP0() == arc.GetP1() )
                    pl.Append( aP1 );
                else
                    pl.Append( arc );
            }
            else
            {
                // Negative tangentLength: arc goes at the start
                VECTOR2I  arcCenter = aP0 + centerDir.Resize( arcRadius );
                SHAPE_ARC ca( arcCenter, aP0, -ANGLE_45 * rotationSign );


                // Constructing with a center can lead to imprecise endpoint.  We need to guarantee
                // tangency of the endpoint.
                // TODO: update the math above to calculate the proper endpoint directly
                VECTOR2I endpoint( ca.GetP1() );

                if( std::abs( endpoint.y - aP1.y ) < SHAPE_ARC::MIN_PRECISION_IU )
                {
                    VECTOR2I fixedEnd( endpoint.x, aP1.y );
                    ca.ConstructFromStartEndAngle( ca.GetP0(), fixedEnd, ANGLE_45 * rotationSign );
                }
                else if( std::abs( endpoint.x - aP1.x ) < SHAPE_ARC::MIN_PRECISION_IU )
                {
                    VECTOR2I fixedEnd( aP1.x, endpoint.y );
                    ca.ConstructFromStartEndAngle( ca.GetP0(), fixedEnd, ANGLE_45 * rotationSign );
                }

                if( ca.GetP0() == ca.GetP1() )
                    pl.Append( aP0 );
                else
                    pl.Append( ca );

                pl.Append( aP1 );
            }
        }
        break;
    }
    case CORNER_MODE::MITERED_90:
        /*
         * For width greater than height, we're calculating something like this.
         *
         *          <-mp0->
         * aP0 -------------------+
         *                        |
         *                        |
         *                        |
         *                       aP1
         */
        pl.Append( aP0 );
        pl.Append( aP0 + mp0 );
        pl.Append( aP1 );
        break;

    case CORNER_MODE::ROUNDED_90:
        /*
         * For a fillet, we need to know the arc end point
         * A straight segment will be needed between aP0 and arcEnd in case distance aP0,mp0 is bigger
         * than the distance mp0,aP1, if the distance is shorter the straigth segment is  between
         * arcEnd and aP1. If both distances are equal, we don't need a straight segment.
         *
         * aP0 ----- arcEnd ---__
         *                       --
         *                          \
         *                           |
         *          arcCenter       aP1
         *
         * For the length of the radius we use the shorter of the horizontal and vertical distance.
         */

        if( w == h ) // we only need one arc without a straigth line.
        {
            arc.ConstructFromStartEndCenter( aP0, aP1, aP1 - mp0, ( sh == sw ) != startDiagonal );
            pl.Append( arc );
            return pl;
        }

        VECTOR2I arcEnd; // Arc position that is not at aP0 nor aP1
        VECTOR2I arcCenter;

        if( startDiagonal ) //Means start with the arc first
        {
            if( h > w ) // Arc followed by a vertical line
            {
                int y = aP0.y + ( w * sh );
                arcEnd = VECTOR2I( aP1.x, y );
                arcCenter = VECTOR2I( aP0.x, y );
                arc.ConstructFromStartEndCenter( aP0, arcEnd, arcCenter, sh != sw );
                pl.Append( arc );
                pl.Append( aP1 );
            }
            else        // Arc followed by a horizontal line
            {
                int x = aP0.x + ( h * sw );
                arcEnd = VECTOR2I( x, aP1.y );
                arcCenter = VECTOR2I( x, aP0.y );
                arc.ConstructFromStartEndCenter( aP0, arcEnd, arcCenter, sh == sw );
                pl.Append( arc );
                pl.Append( aP1 );
            }
        }
        else
        {
            if( w > h ) // Horizontal line followed by the arc
            {
                int x = aP1.x - ( h * sw );
                arcEnd = VECTOR2I( x, aP0.y );
                arcCenter = VECTOR2I( x, aP1.y );
                pl.Append( aP0 );
                arc.ConstructFromStartEndCenter( arcEnd, aP1, arcCenter, sh != sw );
                pl.Append( arc );
            }
            else        // Vertical line followed by the arc
            {
                int y = aP1.y - ( w * sh );
                arcEnd = VECTOR2I( aP0.x, y );
                arcCenter = VECTOR2I( aP1.x, y );
                pl.Append( aP0 );
                arc.ConstructFromStartEndCenter( arcEnd, aP1, arcCenter, sh == sw );
                pl.Append( arc );
            }
        }
        break;
    }

    pl.Simplify();
    return pl;
}
