/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 Ola Rinta-Koski
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

#include <geometry/eda_angle.h>


EDA_ANGLE EDA_ANGLE::m_Angle0   = EDA_ANGLE( 0, DEGREES_T );
EDA_ANGLE EDA_ANGLE::m_Angle45  = EDA_ANGLE( 45, DEGREES_T );
EDA_ANGLE EDA_ANGLE::m_Angle90  = EDA_ANGLE( 90, DEGREES_T );
EDA_ANGLE EDA_ANGLE::m_Angle135 = EDA_ANGLE( 135, DEGREES_T );
EDA_ANGLE EDA_ANGLE::m_Angle180 = EDA_ANGLE( 180, DEGREES_T );
EDA_ANGLE EDA_ANGLE::m_Angle270 = EDA_ANGLE( 270, DEGREES_T );
EDA_ANGLE EDA_ANGLE::m_Angle360 = EDA_ANGLE( 360, DEGREES_T );


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

    return EDA_ANGLE( outDegrees, DEGREES_T );
}


void EDA_ANGLE::normalize( bool n720 )
{
    if( GetInitialAngleType() == RADIANS_T )
    {
        m_radians = normalize( m_radians, RADIANS_T, n720 );
        m_value = int( m_radians / TENTHS_OF_A_DEGREE_TO_RADIANS );
    }
    else
    {
        m_value = normalize( m_value, TENTHS_OF_A_DEGREE_T, n720 );
    }
}


int EDA_ANGLE::normalize( int aValue, EDA_ANGLE_T aAngleType, bool n720 ) const
{
    int full_circle_upper = DEGREES_FULL_CIRCLE;

    switch( aAngleType )
    {
    case DEGREES_T:
        full_circle_upper = DEGREES_FULL_CIRCLE;
        break;

    case TENTHS_OF_A_DEGREE_T:
        full_circle_upper = TENTHS_OF_A_DEGREE_FULL_CIRCLE;
        break;

    case RADIANS_T:
        wxFAIL_MSG( "should be unreachable..." );
    }

    /*
     * if n720 == false, clamp between 0..full_circle_upper
     * if n720 == true, clamp between +/- full_circle_upper
     */
    int full_circle_lower = n720 ? -full_circle_upper : 0;

    while( aValue < full_circle_lower )
        aValue += full_circle_upper;

    while( aValue >= full_circle_upper )
        aValue -= full_circle_upper;

    return aValue;
}


double EDA_ANGLE::normalize( double aValue, EDA_ANGLE_T aAngleType, bool n720 ) const
{
    double full_circle_upper = DEGREES_FULL_CIRCLE;

    switch( aAngleType )
    {
    case DEGREES_T:            full_circle_upper = DEGREES_FULL_CIRCLE;            break;
    case TENTHS_OF_A_DEGREE_T: full_circle_upper = TENTHS_OF_A_DEGREE_FULL_CIRCLE; break;
    case RADIANS_T:            full_circle_upper = RADIANS_FULL_CIRCLE;            break;
    }

    double full_circle_lower = n720 ? 0 : -full_circle_upper;

    while( aValue < full_circle_lower )
        aValue += full_circle_upper;

    while( aValue >= full_circle_upper )
        aValue -= full_circle_upper;

    return aValue;
}
