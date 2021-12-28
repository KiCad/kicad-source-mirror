/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 Ola Rinta-Koski
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

#include <eda_angle.h>

EDA_ANGLE EDA_ANGLE::m_angle0 = EDA_ANGLE( 0, EDA_ANGLE::DEGREES );
EDA_ANGLE EDA_ANGLE::m_angle90 = EDA_ANGLE( 90, EDA_ANGLE::DEGREES );
EDA_ANGLE EDA_ANGLE::m_angle180 = EDA_ANGLE( 180, EDA_ANGLE::DEGREES );
EDA_ANGLE EDA_ANGLE::m_angle270 = EDA_ANGLE( 270, EDA_ANGLE::DEGREES );
EDA_ANGLE EDA_ANGLE::m_angle360 = EDA_ANGLE( 360, EDA_ANGLE::DEGREES );

EDA_ANGLE EDA_ANGLE::KeepUpright() const
{
    EDA_ANGLE inAngle( *this );
    inAngle.Normalize();

    int inDegrees = inAngle.AsDegrees();
    int outDegrees;

    if( inDegrees <= 45 || inDegrees >= 315 || ( inDegrees > 135 && inDegrees <= 225 ) )
        outDegrees = 0;
    else
        outDegrees = 90;

    return EDA_ANGLE( outDegrees, EDA_ANGLE::DEGREES );
}


void EDA_ANGLE::normalize( bool n720 )
{
    if( GetInitialAngleType() == EDA_ANGLE::RADIANS )
    {
        m_radians = normalize( m_radians, EDA_ANGLE::RADIANS, n720 );
        m_value = int( m_radians / TENTHS_OF_A_DEGREE_TO_RADIANS );
    }
    else
    {
        m_value = normalize( m_value, EDA_ANGLE::TENTHS_OF_A_DEGREE, n720 );
    }
}


int EDA_ANGLE::normalize( int aValue, ANGLE_TYPE aAngleType, bool n720 ) const
{
    int full_circle_upper;

    switch( aAngleType )
    {
    case DEGREES:
        full_circle_upper = DEGREES_FULL_CIRCLE;
        break;

    case TENTHS_OF_A_DEGREE:
        full_circle_upper = TENTHS_OF_A_DEGREE_FULL_CIRCLE;
        break;

    case RADIANS:
        /* ?? should not get here */
        assert( 1 == 0 );
    }

    /* if n720 == false, clamp between 0..full_circle_upper
         * if n720 == true, clamp between +/- full_circle_upper
         */
    int full_circle_lower = n720 ? 0 : -full_circle_upper;

    while( aValue < full_circle_lower )
        aValue += full_circle_upper;

    while( aValue > full_circle_upper )
        aValue -= full_circle_upper;

    return aValue;
}


double EDA_ANGLE::normalize( double aValue, ANGLE_TYPE aAngleType, bool n720 ) const
{
    double full_circle_upper;

    switch( aAngleType )
    {
    case DEGREES:            full_circle_upper = DEGREES_FULL_CIRCLE;            break;
    case TENTHS_OF_A_DEGREE: full_circle_upper = TENTHS_OF_A_DEGREE_FULL_CIRCLE; break;
    case RADIANS:            full_circle_upper = RADIANS_FULL_CIRCLE;            break;
    }

    double full_circle_lower = n720 ? 0 : -full_circle_upper;

    while( aValue < full_circle_lower )
        aValue += full_circle_upper;

    while( aValue > full_circle_upper )
        aValue -= full_circle_upper;

    return aValue;
}
