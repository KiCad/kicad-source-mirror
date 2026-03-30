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
            m_dir = constructFromVector( aSeg.B - aSeg.A );
        }

        DIRECTION( const SHAPE_SEGMENT& aSeg )
        {
            m_dir = constructFromVector( aSeg.GetSeg().B - aSeg.GetSeg().A );
        }

        DIRECTION( const SHAPE_BICONNECTED& aShape, bool aTakeFirstPoint )
        {
            switch( aShape.Type() )
            {
                case SH_SEGMENT:
                {
                    const auto& shSeg = static_cast<const SHAPE_SEGMENT& >( aShape );
                    m_dir = constructFromVector( shSeg.GetSeg().B - shSeg.GetSeg().A );
                }                    
                // case SH_ARC:  fixme
                default:
                    m_dir = UNDEFINED;
            }
        }

        DIRECTION( EDA_ANGLE aAngle )
        {
            wxASSERT( false );
        }

        wxString Format() const
        {
            switch( m_dir )
            {
            case N: return wxT( "north" );
            case NE: return wxT( "north-east" );
            case E: return wxT( "east" );
            case SE: return wxT( "south-east" );
            case S: return wxT( "south" );
            case SW: return wxT( "south-west" );
            case W: return wxT( "west" );
            case NW: return wxT( "north-west" );
            case UNDEFINED: return wxT( "undefined" );
            default: return wxT( "<Error>" );
            }
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
        const DIRECTION Right() const
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
                                              CORNER_MODE aMode = CORNER_MODE::MITERED_45 );
    
    
    static const wxString Format( ANGLE_TYPE aAngle );

    static ANGLE_TYPE Angle( const SHAPE_BICONNECTED& aA, const SHAPE_BICONNECTED& aB ) { wxASSERT( false ); return ANG_UNDEFINED; } // fixme implement
    static ANGLE_TYPE Angle( const SHAPE_CHAIN& aLine, int aVertex ) { wxASSERT( false ); return ANG_UNDEFINED; } // fixme implement
    static ANGLE_TYPE Angle( const DIRECTION& aA, const DIRECTION& aB ) 
    { 
        if( aA.m_dir == UNDEFINED || aB.m_dir == UNDEFINED )
            return ANG_UNDEFINED;

        int d = std::abs( static_cast<int>( aA.m_dir ) - static_cast<int>( aB.m_dir ) );

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

    static DIRS constructFromVector( const VECTOR2I& aVec )
    {
        if( aVec.x == 0 && aVec.y == 0 )
            return UNDEFINED;

        double mag = 360.0 - ( 180.0 / M_PI * atan2( (double) (-aVec.y), (double) aVec.x ) ) + 90.0;

        if( mag >= 360.0 )
            mag -= 360.0;

        if( mag < 0.0 )
            mag += 360.0;

        int dir = ( mag + 22.5 ) / 45.0;

        if( dir >= LAST )
            dir -= LAST;

        if( dir < 0 )
            dir += LAST;

        return (DIRS) dir;
    }

};

};

