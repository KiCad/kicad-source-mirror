/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 Ola Rinta-Koski.
 * Copyright (C) 2021 KiCad Developers, see AUTHORS.txt for contributors.
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

class EDA_ANGLE
{
public:
    enum ANGLE_TYPE
    {
        TENTHS_OF_A_DEGREE = 1,
        DEGREES = 10,
        RADIANS                     ///< enum value does not matter
    };

    // Angles can be created in degrees, 1/10ths of a degree, and radians,
    // and read as any of the angle types
    //
    // Angle type must be explicitly specified at creation, because
    // there is no other way of knowing what an int or a double represents
    EDA_ANGLE( int aValue, ANGLE_TYPE aAngleType ) :
            m_initial_type( aAngleType )
    {
        switch( aAngleType )
        {
        case RADIANS:
            m_radians = aValue;
            m_value = int( aValue / TENTHS_OF_A_DEGREE_TO_RADIANS );
            break;

        default:
            m_value = aValue * aAngleType;
        }
    }

    EDA_ANGLE( double aValue, ANGLE_TYPE aAngleType ) :
            m_initial_type( aAngleType )
    {
        switch( aAngleType )
        {
        case RADIANS:
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
            m_initial_type( EDA_ANGLE::RADIANS )
    {}

    inline double AsDegrees() const { return m_value / (double) EDA_ANGLE::DEGREES; }

    inline int AsTenthsOfADegree() const { return m_value; }

    inline double AsRadians() const
    {
        if( m_initial_type == EDA_ANGLE::RADIANS )
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

    inline double AsAngleType( ANGLE_TYPE aAngleType ) const
    {
        switch( aAngleType )
        {
        case TENTHS_OF_A_DEGREE: return AsTenthsOfADegree();
        case DEGREES:            return AsDegrees();
        case RADIANS:            return AsRadians();
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
        ANGLE_TYPE initialType = GetInitialAngleType();

        // if both were given in radians, addition is exact
        if( initialType == EDA_ANGLE::RADIANS
            && aAngle.GetInitialAngleType() == EDA_ANGLE::RADIANS )
        {
            //double newAngle = normalize( AsRadians() + aAngle.AsRadians(), EDA_ANGLE::RADIANS );
            double newAngle = AsRadians() + aAngle.AsRadians();
            return EDA_ANGLE( newAngle, EDA_ANGLE::RADIANS );
        }

        // if both were not given in radians, addition is done using
        // 1/10ths of a degree, then converted to original angle type
        // of this angle
        //int newAngle = normalize( AsTenthsOfADegree() + aAngle.AsTenthsOfADegree(),
        //EDA_ANGLE::TENTHS_OF_A_DEGREE );
        int newAngle = AsTenthsOfADegree() + aAngle.AsTenthsOfADegree();

        switch( initialType )
        {
        case DEGREES:
            return EDA_ANGLE( newAngle / EDA_ANGLE::DEGREES, EDA_ANGLE::DEGREES );

        case RADIANS:
            return EDA_ANGLE( newAngle / TENTHS_OF_A_DEGREE_TO_RADIANS, EDA_ANGLE::RADIANS );

        default:
        case TENTHS_OF_A_DEGREE:
            return EDA_ANGLE( newAngle, EDA_ANGLE::TENTHS_OF_A_DEGREE );
        }
    }

    EDA_ANGLE Invert() const
    {
        switch( GetInitialAngleType() )
        {
        case RADIANS:
            return EDA_ANGLE( -m_radians, EDA_ANGLE::RADIANS );
        default:
            return EDA_ANGLE( -m_value / GetInitialAngleType(), GetInitialAngleType() );
        }
    }

    EDA_ANGLE Subtract( const EDA_ANGLE& aAngle ) const { return Add( aAngle.Invert() ); }

    inline ANGLE_TYPE GetInitialAngleType() const { return m_initial_type; }

    double Sin() const { return sin( AsRadians() ); }

    double Cos() const { return cos( AsRadians() ); }

    double Tan() const { return tan( AsRadians() ); }

    static EDA_ANGLE Arccos( double x ) { return EDA_ANGLE( acos( x ), EDA_ANGLE::RADIANS ); }

    static EDA_ANGLE Arcsin( double x ) { return EDA_ANGLE( asin( x ), EDA_ANGLE::RADIANS ); }

    static EDA_ANGLE Arctan( double x ) { return EDA_ANGLE( atan( x ), EDA_ANGLE::RADIANS ); }

    static EDA_ANGLE Arctan2( double y, double x )
    {
        return EDA_ANGLE( atan2( y, x ), EDA_ANGLE::RADIANS );
    }

    inline EDA_ANGLE Normalize()
    {
        normalize( false );
        return *this;
    }

    inline EDA_ANGLE Normalize720()
    {
        normalize( true );
        return *this;
    }

    EDA_ANGLE KeepUpright() const;

private:
    // value is always stored in 1/10ths of a degree
    int m_value;

    double m_radians; //< only used with as-radians constructor

    ANGLE_TYPE m_initial_type;

    void   normalize( bool n720 = false );
    int    normalize( int aValue, ANGLE_TYPE aAngleType, bool n720 = false ) const;
    double normalize( double aValue, ANGLE_TYPE aAngleType, bool n720 = false ) const;

    static constexpr int    TENTHS_OF_A_DEGREE_FULL_CIRCLE = 3600;
    static constexpr int    DEGREES_FULL_CIRCLE = 360;
    static constexpr double RADIANS_FULL_CIRCLE = 2 * M_PI;

    static EDA_ANGLE m_angle0;
    static EDA_ANGLE m_angle90;
    static EDA_ANGLE m_angle180;
    static EDA_ANGLE m_angle270;
    static EDA_ANGLE m_angle360;

public:
    static constexpr EDA_ANGLE& HORIZONTAL = m_angle0;
    static constexpr EDA_ANGLE& VERTICAL = m_angle90;
    static constexpr EDA_ANGLE& FULL_CIRCLE = m_angle360;
    static constexpr EDA_ANGLE& ANGLE_0 = m_angle0;
    static constexpr EDA_ANGLE& ANGLE_90 = m_angle90;
    static constexpr EDA_ANGLE& ANGLE_180 = m_angle180;
    static constexpr EDA_ANGLE& ANGLE_270 = m_angle270;
};


inline EDA_ANGLE operator-( const EDA_ANGLE& aAngle )
{
    return aAngle.Invert();
}


inline bool operator==( const EDA_ANGLE& aAngleA, const EDA_ANGLE& aAngleB )
{
    return aAngleA.AsTenthsOfADegree() == aAngleB.AsTenthsOfADegree();
}


inline bool operator!=( const EDA_ANGLE& aAngleA, const EDA_ANGLE& aAngleB )
{
    return aAngleA.AsTenthsOfADegree() != aAngleB.AsTenthsOfADegree();
}


#endif // EDA_ANGLE_H
