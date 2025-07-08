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
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef EDA_ANGLE_H
#define EDA_ANGLE_H

#include <cassert>
#include <cmath>
#include <math/vector2d.h>  // for VECTOR2I


enum EDA_ANGLE_T
{
    TENTHS_OF_A_DEGREE_T,
    DEGREES_T,
    RADIANS_T
};


class EDA_ANGLE
{
public:
    /**
     * Angles can be created in degrees, 1/10ths of a degree, or radians, and read as any of
     * the angle types.
     *
     * Angle type must be explicitly specified at creation, because there is no other way of
     * knowing what an int or a double represents.
     */
    EDA_ANGLE( double aValue, EDA_ANGLE_T aAngleType )
    {
        switch( aAngleType )
        {
        case RADIANS_T:
            m_value = aValue / DEGREES_TO_RADIANS;
            break;

        case TENTHS_OF_A_DEGREE_T:
            m_value = aValue / 10.0;
            break;

        default:
            m_value = aValue;
        }
    }

    /**
     * Construct an EDA_ANGLE in degrees.  This is really only for the statically-defined
     * global angles.  All other code should use EDA_ANGLE( xxx, DEGREES_T ).
     * @param aAngleInDegrees
     */
    constexpr explicit EDA_ANGLE( double aAngleInDegrees ) :
            m_value( aAngleInDegrees )
    {}

    explicit EDA_ANGLE( const VECTOR2D& aVector )
    {
        if( aVector.x == 0.0 && aVector.y == 0.0 )
        {
            m_value = 0.0;
        }
        else if( aVector.y == 0.0 )
        {
            if( aVector.x >= 0 )
                m_value = 0.0;
            else
                m_value = -180.0;
        }
        else if( aVector.x == 0.0 )
        {
            if( aVector.y >= 0.0 )
                m_value = 90.0;
            else
                m_value = -90.0;
        }
        else if( aVector.x == aVector.y )
        {
            if( aVector.x >= 0.0 )
                m_value = 45.0;
            else
                m_value = -180.0 + 45.0;
        }
        else if( aVector.x == -aVector.y )
        {
            if( aVector.x >= 0.0 )
                m_value = -45.0;
            else
                m_value = 180.0 - 45.0;
        }
        else
        {
            *this = EDA_ANGLE( atan2( aVector.y, aVector.x ), RADIANS_T );
        }
    }

    EDA_ANGLE() :
            m_value( 0.0 )
    {}

    inline double AsDegrees() const { return m_value; }

    inline int AsTenthsOfADegree() const { return KiROUND( m_value * 10.0 ); }

    inline double AsRadians() const { return m_value * DEGREES_TO_RADIANS; }

    static constexpr double DEGREES_TO_RADIANS = M_PI / 180.0;

    /**
     * @return true if angle is one of the four cardinal directions (0/90/180/270 degrees),
     *         otherwise false
     */
    bool IsCardinal() const;

    /**
     * @return true if angle is one of the two cardinal directions (90/270 degrees),
     *         otherwise false
     */
    bool IsCardinal90() const;

    bool IsZero() const
    {
        return m_value == 0.0;
        // return equals( m_value, 0.0 );
    }

    bool IsHorizontal() const
    {
        return m_value == 0.0 || m_value == 180.0;
        // return equals( m_value, 0.0 ) || equals( m_value, 180.0 );
    }

    bool IsVertical() const
    {
        return m_value == 90.0 || m_value == 270.0;
        //return equals( m_value, 90.0 ) || equals( m_value, 270.0 );
    }

    bool IsParallelTo( EDA_ANGLE aAngle ) const
    {
        EDA_ANGLE thisNormalized = *this;

        // Normalize90 is inclusive on both ends [-90, +90]
        // but we need it to be (-90, +90] for this test to work
        thisNormalized.Normalize90();

        if( equals( thisNormalized.AsDegrees(), -90.0 ) )
            thisNormalized = EDA_ANGLE( 90.0, DEGREES_T );

        aAngle.Normalize90();

        if( equals( aAngle.AsDegrees(), -90.0 ) )
            aAngle = EDA_ANGLE( 90.0, DEGREES_T );

        return ( equals( thisNormalized.AsDegrees(), aAngle.AsDegrees() ) );
    }

    EDA_ANGLE Invert() const
    {
        return EDA_ANGLE( -AsDegrees(), DEGREES_T );
    }

    double Sin() const
    {
        EDA_ANGLE test = *this;
        test.Normalize();

        if( test.m_value == 0.0 || test.m_value == 180.0 )
            return 0.0;
        else if( test.m_value == 45.0 || test.m_value == 135.0 )
            return M_SQRT1_2; // sqrt(2)/2
        else if( test.m_value == 225.0 || test.m_value == 315.0 )
            return -M_SQRT1_2;
        else if( test.m_value == 90.0 )
            return 1.0;
        else if( test.m_value == 270.0 )
            return -1.0;
        else
            return sin( AsRadians() );
    }

    double Cos() const
    {
        EDA_ANGLE test = *this;
        test.Normalize();

        if( test.m_value == 0.0 )
            return 1.0;
        else if( test.m_value == 180.0 )
            return -1.0;
        else if( test.m_value == 90.0 || test.m_value == 270.0 )
            return 0.0;
        else if( test.m_value == 45.0 || test.m_value == 315.0 )
            return M_SQRT1_2; // sqrt(2)/2
        else if( test.m_value == 135.0 || test.m_value == 225.0 )
            return -M_SQRT1_2;
        else
            return cos( AsRadians() );
    }

    double Tan() const { return tan( AsRadians() ); }

    static EDA_ANGLE Arccos( double x ) { return EDA_ANGLE( acos( x ), RADIANS_T ); }

    static EDA_ANGLE Arcsin( double x ) { return EDA_ANGLE( asin( x ), RADIANS_T ); }

    static EDA_ANGLE Arctan( double x ) { return EDA_ANGLE( atan( x ), RADIANS_T ); }

    static EDA_ANGLE Arctan2( double y, double x )
    {
        return EDA_ANGLE( atan2( y, x ), RADIANS_T );
    }

    inline EDA_ANGLE Normalize()
    {
        while( m_value < -0.0 )
            m_value += 360.0;

        while( m_value >= 360.0 )
            m_value -= 360.0;

        return *this;
    }

    EDA_ANGLE Normalized() const
    {
        EDA_ANGLE ret( *this );
        return ret.Normalize();
    }

    inline EDA_ANGLE NormalizeNegative()
    {
        while( m_value <= -360.0 )
            m_value += 360.0;

        while( m_value > 0.0 )
            m_value -= 360.0;

        return *this;
    }

    inline EDA_ANGLE Normalize90()
    {
        while( m_value < -90.0 )
            m_value += 180.0;

        while( m_value > 90.0 )
            m_value -= 180.0;

        return *this;
    }

    inline EDA_ANGLE Normalize180()
    {
        while( m_value <= -180.0 )
            m_value += 360.0;

        while( m_value > 180.0 )
            m_value -= 360.0;

        return *this;
    }

    inline EDA_ANGLE Normalize720()
    {
        while( m_value < -360.0 )
            m_value += 360.0;

        while( m_value >= 360.0 )
            m_value -= 360.0;

        return *this;
    }

    EDA_ANGLE KeepUpright() const;

    EDA_ANGLE Round( int digits ) const
    {
        EDA_ANGLE angle( *this );
        double rounded = KiROUND( angle.AsDegrees() * pow( 10.0, digits ) ) / pow( 10.0, digits );
        angle = EDA_ANGLE( rounded, DEGREES_T );
        return angle;
    }

    EDA_ANGLE& operator+=( const EDA_ANGLE& aAngle )
    {
        *this = EDA_ANGLE( AsDegrees() + aAngle.AsDegrees(), DEGREES_T );
        return *this;
    }

    EDA_ANGLE& operator-=( const EDA_ANGLE& aAngle )
    {
        *this = EDA_ANGLE( AsDegrees() - aAngle.AsDegrees(), DEGREES_T );
        return *this;
    }

private:
    double  m_value;           ///< value in degrees

};


inline EDA_ANGLE operator-( const EDA_ANGLE& aAngle )
{
    return aAngle.Invert();
}


inline EDA_ANGLE operator-( const EDA_ANGLE& aAngleA, const EDA_ANGLE& aAngleB )
{
    return EDA_ANGLE( aAngleA.AsDegrees() - aAngleB.AsDegrees(), DEGREES_T );
}


inline EDA_ANGLE operator+( const EDA_ANGLE& aAngleA, const EDA_ANGLE& aAngleB )
{
    return EDA_ANGLE( aAngleA.AsDegrees() + aAngleB.AsDegrees(), DEGREES_T );
}


inline EDA_ANGLE operator*( const EDA_ANGLE& aAngleA, double aOperator )
{
    return EDA_ANGLE( aAngleA.AsDegrees() * aOperator, DEGREES_T );
}


inline EDA_ANGLE operator/( const EDA_ANGLE& aAngleA, double aOperator )
{
    return EDA_ANGLE( aAngleA.AsDegrees() / aOperator, DEGREES_T );
}


inline double operator/( const EDA_ANGLE& aAngleA, const EDA_ANGLE& aOperator )
{
    return aAngleA.AsDegrees() / aOperator.AsDegrees();
}


inline bool operator==( const EDA_ANGLE& aAngleA, const EDA_ANGLE& aAngleB )
{
    return aAngleA.AsDegrees() == aAngleB.AsDegrees();
    //return equals( aAngleA.AsDegrees(), aAngleB.AsDegrees() );
}


inline bool operator!=( const EDA_ANGLE& aAngleA, const EDA_ANGLE& aAngleB )
{
    return aAngleA.AsDegrees() != aAngleB.AsDegrees();
    // return !equals( aAngleA.AsDegrees(), aAngleB.AsDegrees() );
}


inline bool operator>( const EDA_ANGLE& aAngleA, const EDA_ANGLE& aAngleB )
{
    return aAngleA.AsDegrees() > aAngleB.AsDegrees();
}


inline bool operator<( const EDA_ANGLE& aAngleA, const EDA_ANGLE& aAngleB )
{
    return aAngleA.AsDegrees() < aAngleB.AsDegrees();
}


inline bool operator<=( const EDA_ANGLE& aAngleA, const EDA_ANGLE& aAngleB )
{
    return aAngleA.AsDegrees() <= aAngleB.AsDegrees();
}


inline bool operator>=( const EDA_ANGLE& aAngleA, const EDA_ANGLE& aAngleB )
{
    return aAngleA.AsDegrees() >= aAngleB.AsDegrees();
}


inline std::ostream& operator<<( std::ostream& aStream, const EDA_ANGLE& aAngle )
{
    return aStream << aAngle.AsDegrees();
}


namespace std
{
inline EDA_ANGLE abs( const EDA_ANGLE& aAngle )
{
    return EDA_ANGLE( std::abs( aAngle.AsDegrees() ), DEGREES_T );
}
}


static constexpr EDA_ANGLE ANGLE_HORIZONTAL{ 0 };
static constexpr EDA_ANGLE ANGLE_VERTICAL{ 90 };
static constexpr EDA_ANGLE FULL_CIRCLE{ 360 };

static constexpr EDA_ANGLE ANGLE_0{ 0 };
static constexpr EDA_ANGLE ANGLE_45{ 45 };
static constexpr EDA_ANGLE ANGLE_90{ 90 };
static constexpr EDA_ANGLE ANGLE_135{ 135 };
static constexpr EDA_ANGLE ANGLE_180{ 180 };
static constexpr EDA_ANGLE ANGLE_270{ 270 };
static constexpr EDA_ANGLE ANGLE_360{ 360 };


#endif // EDA_ANGLE_H
