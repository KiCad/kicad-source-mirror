#pragma once

#include <math/vector2d.h>
#include <math/box2.h>
#include <geometry/shape_chain.h>
#include <geometry/shape_segment.h>
#include <geometry/shape_rect.h>
#include <geometry/shape_simple.h>

namespace PNS {

class ROUTING_REGIME_45
{
public:

    enum DIRS : int
    {
        N           = 0,
        NE          = 1,
        E           = 2,
        SE          = 3,
        S           = 4,
        SW          = 5,
        W           = 6,
        NW          = 7,
        LAST        = 8,
        UNDEFINED   = -1
    };

    class DIRECTION
    {
        public:
        DIRECTION( DIRS aDir = DIRS::N ) : 
            m_dir( aDir )
        {};

        DIRECTION( const SEG& aSeg )
        {

        }

        DIRECTION( const SHAPE_SEGMENT& aSeg )
        {

        }

        DIRECTION( const SHAPE_BICONNECTED& aSeg, bool aTakeFirstPoint )
        {

        }

        DIRECTION( EDA_ANGLE aAngle )
        {

        }

        wxString Format() const
        {
            return wxT("");
        }

        bool operator==( const DIRECTION& aOther ) const
        {
            return aOther.m_dir == m_dir;
        }

        bool operator!=( const DIRECTION& aOther ) const
        {
            return aOther.m_dir != m_dir;
        }


        /**
         * Return the direction on the right side of this (i.e. turns right by 45 deg).
         */
        const DIRECTION Right45() const
        {
            return DIRECTION( static_cast<DIRS>( ( m_dir + 1 ) % LAST ) );
        }

        /**
         * Return the direction on the left side of this (i.e. turns left by 45 deg).
         */
        const DIRECTION Left() const
        {
            return DIRECTION( static_cast<DIRS>( ( m_dir + LAST - 1 ) % LAST ) );
        }

        bool IsDiagonal() const
        {
            return ( m_dir % 2 ) == 1;
        }

        DIRECTION Opposite() const
        {
        const DIRS OppositeMap[] = { S, SW, W, NW, N, NE, E, SE, UNDEFINED };
        return OppositeMap[m_dir];
        }


        DIRS m_dir;
    };

    /**
     * Corner modes.
     * A corner can either be 45° or 90° and can be fillet/rounded or mitered
     */
    enum CORNER_MODE
    {
        MITERED_45 = 0, ///< H/V/45 with mitered corners (default)
        ROUNDED_45 = 1, ///< H/V/45 with filleted corners
        MITERED_90 = 2, ///< H/V only (90-degree corners)
        ROUNDED_90 = 3, ///< H/V with filleted corners
    };

    /**
     * Represent kind of angle formed by vectors heading in two DIRECTION_45s.
     */
    enum ANGLE_TYPE
    {
        ANG_OBTUSE      = 0x01,
        ANG_RIGHT       = 0x02,
        ANG_ACUTE       = 0x04,
        ANG_STRAIGHT    = 0x08,
        ANG_HALF_FULL   = 0x10,
        ANG_UNDEFINED   = 0x20
    };

    static const SHAPE_CHAIN BuildInitialTrace( const VECTOR2I& aP0, const VECTOR2I& aP1,
                                              bool aStartDiagonal = false,
                                              CORNER_MODE aMode = CORNER_MODE::MITERED_45 )
                                              {
                                                return SHAPE_CHAIN();
                                              }
    
    /*                                          static const SHAPE_CHAIN BuildInitialTrace( const VECTOR2I& aP0, const VECTOR2I& aP1,
                                              DIRECTION aInitDirection = DIRECTION(DIRS::N),
                                              CORNER_MODE aMode = CORNER_MODE::MITERED_45 )
                                              {

                                              } */

    static ANGLE_TYPE Angle( const SHAPE_BICONNECTED& aA, const SHAPE_BICONNECTED& aB ) { return ANG_UNDEFINED; } // fixme implement
    static ANGLE_TYPE Angle( const SHAPE_CHAIN& aLine, int aVertex ) { return ANG_UNDEFINED; } // fixme implement
    static ANGLE_TYPE Angle( DIRECTION aA, DIRECTION aB ) { return ANG_UNDEFINED; } // fixme implement


};

};

