/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 Ola Rinta-Koski.
 * Copyright (C) 2021-2022 KiCad Developers, see AUTHORS.txt for contributors.
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


enum EDA_ANGLE_T
{
    TENTHS_OF_A_DEGREE_T = 1,
    DEGREES_T            = 10,
    RADIANS_T                     ///< enum value does not matter
};


class EDA_ANGLE
{
public:
    // Angles can be created in degrees, 1/10ths of a degree, and radians,
    // and read as any of the angle types
    //
    // Angle type must be explicitly specified at creation, because
    // there is no other way of knowing what an int or a double represents
    EDA_ANGLE( int aValue, EDA_ANGLE_T aAngleType ) :
            m_value( 0 ),
            m_radians( 0.0 ),
            m_initial_type( aAngleType )
    {
        switch( aAngleType )
        {
        case RADIANS_T:
            m_radians = aValue;
            m_value = int( aValue / TENTHS_OF_A_DEGREE_TO_RADIANS );
            break;

        default:
            m_value = aValue * aAngleType;
        }
    }

    EDA_ANGLE( double aValue, EDA_ANGLE_T aAngleType ) :
            m_value( 0 ),
            m_radians( 0.0 ),
            m_initial_type( aAngleType )
    {
        switch( aAngleType )
        {
        case RADIANS_T:
            m_radians = aValue;
            m_value = int( aValue / TENTHS_OF_A_DEGREE_TO_RADIANS );
            break;

        default:
            m_value = int( aValue * aAngleType );
        }
    }

    EDA_ANGLE() :
            m_value( 0 ),
            m_radians( 0.0 ),
            m_initial_type( RADIANS_T )
    {}

    inline double AsDegrees() const { return m_value / (double) DEGREES_T; }

    inline int AsTenthsOfADegree() const { return m_value; }

    inline double AsRadians() const
    {
        if( m_initial_type == RADIANS_T )
        {
            // if this was initialized with radians, return exact initial value
            return m_radians;
        }
        else
        {
            // otherwise compute from value stored as 1/10ths of a degree
            return m_value * TENTHS_OF_A_DEGREE_TO_RADIANS;
        }
    }

    inline double AsAngleType( EDA_ANGLE_T aAngleType ) const
    {
        switch( aAngleType )
        {
        case TENTHS_OF_A_DEGREE_T: return AsTenthsOfADegree();
        case DEGREES_T:            return AsDegrees();
        case RADIANS_T:            return AsRadians();
        default: assert( 1 == 0 );
        }
    }

    static constexpr double TENTHS_OF_A_DEGREE_TO_RADIANS = M_PI / 1800;

    /**
     * @return true if angle is one of the four cardinal directions (0/90/180/270 degrees),
     *         otherwise false
     */
    bool IsCardinal() const
    {
        return AsTenthsOfADegree() % 900 == 0;
    }

    bool IsZero() const
    {
        return AsTenthsOfADegree() == 0;
    }

    bool IsHorizontal() const
    {
        return AsTenthsOfADegree() == 0 || AsTenthsOfADegree() == 1800;
    }

    bool IsVertical() const
    {
        return AsTenthsOfADegree() == 900 || AsTenthsOfADegree() == 2700;
    }

    EDA_ANGLE Add( const EDA_ANGLE& aAngle ) const
    {
        EDA_ANGLE_T initialType = GetInitialAngleType();

        // if both were given in radians, addition is exact
        if( initialType == RADIANS_T
            && aAngle.GetInitialAngleType() == RADIANS_T )
        {
            //double newAngle = normalize( AsRadians() + aAngle.AsRadians(), RADIANS_T );
            double newAngle = AsRadians() + aAngle.AsRadians();
            return EDA_ANGLE( newAngle, RADIANS_T );
        }

        // if both were not given in radians, addition is done using
        // 1/10ths of a degree, then converted to original angle type
        // of this angle
        //int newAngle = normalize( AsTenthsOfADegree() + aAngle.AsTenthsOfADegree(),
        //TENTHS_OF_A_DEGREE_T );
        int newAngle = AsTenthsOfADegree() + aAngle.AsTenthsOfADegree();

        switch( initialType )
        {
        case DEGREES_T:
            return EDA_ANGLE( newAngle / DEGREES_T, DEGREES_T );

        case RADIANS_T:
            return EDA_ANGLE( newAngle / TENTHS_OF_A_DEGREE_TO_RADIANS, RADIANS_T );

        default:
        case TENTHS_OF_A_DEGREE_T:
            return EDA_ANGLE( newAngle, TENTHS_OF_A_DEGREE_T );
        }
    }

    EDA_ANGLE Invert() const
    {
        switch( GetInitialAngleType() )
        {
        case RADIANS_T:
            return EDA_ANGLE( -m_radians, RADIANS_T );
        default:
            return EDA_ANGLE( -m_value / GetInitialAngleType(), GetInitialAngleType() );
        }
    }

    EDA_ANGLE Subtract( const EDA_ANGLE& aAngle ) const { return Add( aAngle.Invert() ); }

    inline EDA_ANGLE_T GetInitialAngleType() const { return m_initial_type; }

    double Sin() const { return sin( AsRadians() ); }

    double Cos() const { return cos( AsRadians() ); }

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
        normalize( false );
        return *this;
    }

    inline EDA_ANGLE Normalize180()
    {
        int angle = AsTenthsOfADegree();

        while( angle <= -1800 )
            angle += 3600;

        while( angle > 1800 )
            angle -= 3600;

        *this = EDA_ANGLE( angle, TENTHS_OF_A_DEGREE_T );

        return *this;
    }

    inline EDA_ANGLE Normalize720()
    {
        normalize( true );
        return *this;
    }

    EDA_ANGLE KeepUpright() const;

    EDA_ANGLE& operator+=( const EDA_ANGLE& aAngle )
    {
        *this = Add( aAngle );
        return *this;
    }

    EDA_ANGLE& operator-=( const EDA_ANGLE& aAngle )
    {
        EDA_ANGLE angle( aAngle );
        *this = Add( angle.Invert() );
        return *this;
    }

private:
    void   normalize( bool n720 = false );
    int    normalize( int aValue, EDA_ANGLE_T aAngleType, bool n720 = false ) const;
    double normalize( double aValue, EDA_ANGLE_T aAngleType, bool n720 = false ) const;

private:

    int          m_value;             ///< value is always stored in 1/10ths of a degree
    double       m_radians;           ///< only used with as-radians constructor
    EDA_ANGLE_T  m_initial_type;

    static constexpr int    TENTHS_OF_A_DEGREE_FULL_CIRCLE = 3600;
    static constexpr int    DEGREES_FULL_CIRCLE = 360;
    static constexpr double RADIANS_FULL_CIRCLE = 2 * M_PI;

public:
    static EDA_ANGLE m_Angle0;
    static EDA_ANGLE m_Angle45;
    static EDA_ANGLE m_Angle90;
    static EDA_ANGLE m_Angle135;
    static EDA_ANGLE m_Angle180;
    static EDA_ANGLE m_Angle270;
    static EDA_ANGLE m_Angle360;
};


inline EDA_ANGLE operator-( const EDA_ANGLE& aAngle )
{
    return aAngle.Invert();
}


inline EDA_ANGLE operator-( const EDA_ANGLE& aAngleA, const EDA_ANGLE& aAngleB )
{
    return aAngleA.Add( aAngleB.Invert() );
}


inline EDA_ANGLE operator+( const EDA_ANGLE& aAngleA, const EDA_ANGLE& aAngleB )
{
    return aAngleA.Add( aAngleB );
}


inline bool operator==( const EDA_ANGLE& aAngleA, const EDA_ANGLE& aAngleB )
{
    return aAngleA.AsTenthsOfADegree() == aAngleB.AsTenthsOfADegree();
}


inline bool operator!=( const EDA_ANGLE& aAngleA, const EDA_ANGLE& aAngleB )
{
    return aAngleA.AsTenthsOfADegree() != aAngleB.AsTenthsOfADegree();
}


inline bool operator>( const EDA_ANGLE& aAngleA, const EDA_ANGLE& aAngleB )
{
    return aAngleA.AsTenthsOfADegree() > aAngleB.AsTenthsOfADegree();
}

inline bool operator<( const EDA_ANGLE& aAngleA, const EDA_ANGLE& aAngleB )
{
    return aAngleA.AsTenthsOfADegree() < aAngleB.AsTenthsOfADegree();
}

inline bool operator<=( const EDA_ANGLE& aAngleA, const EDA_ANGLE& aAngleB )
{
    return aAngleA.AsTenthsOfADegree() <= aAngleB.AsTenthsOfADegree();
}

inline bool operator>=( const EDA_ANGLE& aAngleA, const EDA_ANGLE& aAngleB )
{
    return aAngleA.AsTenthsOfADegree() >= aAngleB.AsTenthsOfADegree();
}


static constexpr EDA_ANGLE& ANGLE_HORIZONTAL  = EDA_ANGLE::m_Angle0;
static constexpr EDA_ANGLE& ANGLE_VERTICAL    = EDA_ANGLE::m_Angle90;
static constexpr EDA_ANGLE& FULL_CIRCLE       = EDA_ANGLE::m_Angle360;

static constexpr EDA_ANGLE& ANGLE_0   = EDA_ANGLE::m_Angle0;
static constexpr EDA_ANGLE& ANGLE_45  = EDA_ANGLE::m_Angle45;
static constexpr EDA_ANGLE& ANGLE_90  = EDA_ANGLE::m_Angle90;
static constexpr EDA_ANGLE& ANGLE_135 = EDA_ANGLE::m_Angle135;
static constexpr EDA_ANGLE& ANGLE_180 = EDA_ANGLE::m_Angle180;
static constexpr EDA_ANGLE& ANGLE_270 = EDA_ANGLE::m_Angle270;
static constexpr EDA_ANGLE& ANGLE_360 = EDA_ANGLE::m_Angle360;


#endif // EDA_ANGLE_H
