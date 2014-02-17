/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2013  CERN
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
 * with this program.  If not, see <http://www.gnu.or/licenses/>.
 */

#ifndef __DIRECTION_H
#define __DIRECTION_H

#include <geometry/seg.h>
#include <geometry/shape_line_chain.h>

/**
 * Class DIRECTION_45.
 * Represents route directions & corner angles in a 45-degree metric.
 */

class DIRECTION_45
{
public:

    /**
     * Enum Directions
     * Represents available directions - there are 8 of them, as on a rectilinear map (north = up) +
     * an extra undefined direction, reserved for traces that don't respect 45-degree routing regime.
     */
    enum Directions
    {
        N           = 0,
        NE          = 1,
        E           = 2,
        SE          = 3,
        S           = 4,
        SW          = 5,
        W           = 6,
        NW          = 7,
        UNDEFINED   = -1
    };

    /**
     * Enum AngleType
     * Represents kind of angle formed by vectors heading in two DIRECTION_45s.
     */
    enum AngleType
    {
        ANG_OBTUSE      = 0x01,
        ANG_RIGHT       = 0x02,
        ANG_ACUTE       = 0x04,
        ANG_STRAIGHT    = 0x08,
        ANG_HALF_FULL   = 0x10,
        ANG_UNDEFINED   = 0x20
    };

    DIRECTION_45( Directions aDir = UNDEFINED ) : m_dir( aDir ) {};

    /**
     * Constructor
     * @param aVec vector, whose direction will be translated into a DIRECTION_45.
     */
    DIRECTION_45( const VECTOR2I& aVec )
    {
        construct( aVec );
    }

    /**
     * Constructor
     * @param aSeg segment, whose direction will be translated into a DIRECTION_45.
     */
    DIRECTION_45( const SEG& aSeg )
    {
        construct( aSeg.B - aSeg.A );
    }

    /**
     * Function Format()
     * Formats the direction in a human readable word.
     * @return name of the direction
     */
    const std::string Format() const
    {
        switch( m_dir )
        {
        case N:
            return "north";

        case NE:
            return "north-east";

        case E:
            return "east";

        case SE:
            return "south-east";

        case S:
            return "south";

        case SW:
            return "south-west";

        case W:
            return "west";

        case NW:
            return "north-west";

        case UNDEFINED:
            return "undefined";

        default:
            return "<Error>";
        }
    }

    /**
     * Function Opposite()
     * Returns a direction opposite (180 degree) to (this)
     * @return opposite direction
     */
    DIRECTION_45 Opposite() const
    {
        const Directions OppositeMap[] = { S, SW, W, NW, N, NE, E, SE, UNDEFINED };
        return OppositeMap[m_dir];
    }

    /**
     * Function Angle()
     * Returns the type of angle between directions (this) and aOther.
     * @param aOther direction to compare angle with
     */
    AngleType Angle( const DIRECTION_45& aOther ) const
    {
        if( m_dir == UNDEFINED || aOther.m_dir == UNDEFINED )
            return ANG_UNDEFINED;

        int d = std::abs( m_dir - aOther.m_dir );

        if( d == 1 || d == 7 )
            return ANG_OBTUSE;
        else if( d == 2 || d == 6 )
            return ANG_RIGHT;
        else if( d == 3 || d == 5 )
            return ANG_ACUTE;
        else if( d == 4 )
            return ANG_HALF_FULL;
        else
            return ANG_STRAIGHT;
    }

    /**
     * Function IsObtuse()
     * @return true, when (this) forms an obtuse angle with aOther
     */
    bool IsObtuse( const DIRECTION_45& aOther ) const
    {
        return Angle( aOther ) == ANG_OBTUSE;
    }

    /**
     * Function IsDiagonal()
     * Returns true if the direction is diagonal (e.g. North-West, South-East, etc)
     * @return true, when diagonal.
     */
    bool IsDiagonal() const
    {
        return ( m_dir % 2 ) == 1;
    }

    /**
     * Function BuildInitialTrace()
     *
     * Builds a 2-segment line chain between points aP0 and aP1 and following 45-degree routing
     * regime. If aStartDiagonal is true, the trace starts with a diagonal segment.
     * @param aP0 starting point
     * @param aP1 ending point
     * @param aStartDiagonal whether the first segment has to be diagonal
     * @return the trace
     */
    const SHAPE_LINE_CHAIN BuildInitialTrace( const VECTOR2I& aP0,
            const VECTOR2I& aP1,
            bool aStartDiagonal = false ) const
    {
        int w = abs( aP1.x - aP0.x );
        int h = abs( aP1.y - aP0.y );
        int sw  = sign( aP1.x - aP0.x );
        int sh  = sign( aP1.y - aP0.y );

        VECTOR2I mp0, mp1;

        // we are more horizontal than vertical?
        if( w > h )
        {
            mp0 = VECTOR2I( (w - h) * sw, 0 );      // direction: E
            mp1 = VECTOR2I( h * sw, h * sh );       // direction: NE
        }
        else
        {
            mp0 = VECTOR2I( 0, sh * (h - w) );      // direction: N
            mp1 = VECTOR2I( sw * w, sh * w );       // direction: NE
        }

        bool start_diagonal;

        if( m_dir == UNDEFINED )
            start_diagonal = aStartDiagonal;
        else
            start_diagonal = IsDiagonal();

        SHAPE_LINE_CHAIN pl;

        pl.Append( aP0 );

        if( start_diagonal )
            pl.Append( aP0 + mp1 );
        else
            pl.Append( aP0 + mp0 );

        pl.Append( aP1 );
        pl.Simplify();
        return pl;
    };

    bool operator==( const DIRECTION_45& aOther ) const
    {
        return aOther.m_dir == m_dir;
    }

    bool operator!=( const DIRECTION_45& aOther ) const
    {
        return aOther.m_dir != m_dir;
    }

    const DIRECTION_45 Right() const
    {
        DIRECTION_45 r;

        r.m_dir = (Directions) (m_dir + 1);

        if( r.m_dir == NW )
            r.m_dir = N;

        return r;
    }

private:

    template <typename T>
    int sign( T val ) const
    {
        return (T( 0 ) < val) - ( val < T( 0 ) );
    }

    /**
     * Function construct()
     * Calculates the direction from a vector. If the vector's angle is not a multiple of 45
     * degrees, the direction is rounded to the nearest octant.
     * @param aVec our vector
     */
    void construct( const VECTOR2I& aVec )
    {
        m_dir = UNDEFINED;

        if( aVec.x == 0 && aVec.y == 0 )
            return;

        double mag = 360.0 - ( 180.0 / M_PI * atan2( (double) aVec.y, (double) aVec.x ) ) + 90.0;

        if( mag >= 360.0 )
            mag -= 360.0;

        if( mag < 0.0 )
            mag += 360.0;

        m_dir = (Directions)( ( mag + 22.5 ) / 45.0 );

        if( m_dir >= 8 )
            m_dir = (Directions)( m_dir - 8 );

        if( m_dir < 0 )
            m_dir = (Directions)( m_dir + 8 );

        return;

        if( aVec.y < 0 )
        {
            if( aVec.x > 0 )
                m_dir = NE;
            else if( aVec.x < 0 )
                m_dir = NW;
            else
                m_dir = N;
        }
        else if( aVec.y == 0 )
        {
            if( aVec.x > 0 )
                m_dir = E;
            else
                m_dir = W;
        }
        else    // aVec.y>0
        {
            if( aVec.x > 0 )
                m_dir = SE;
            else if( aVec.x < 0 )
                m_dir = SW;
            else
                m_dir = S;
        }
    }

    Directions m_dir;    ///> our actual direction
};

#endif    // __DIRECTION_H
