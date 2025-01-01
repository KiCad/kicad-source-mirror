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

#include <geometry/eda_angle.h>


EDA_ANGLE EDA_ANGLE::KeepUpright() const
{
    EDA_ANGLE inAngle( *this );
    inAngle.Normalize();

    double inDegrees = inAngle.AsDegrees();
    double outDegrees;

    if( inDegrees <= 45 || inDegrees >= 315 || ( inDegrees > 135 && inDegrees <= 225 ) )
        outDegrees = 0;
    else
        outDegrees = 90;

    return EDA_ANGLE( outDegrees, DEGREES_T );
}


bool EDA_ANGLE::IsCardinal() const
{
    double test = m_value;

    while( test < 0.0 )
        test += 90.0;

    while( test >= 90.0 )
        test -= 90.0;

    return test == 0.0;
}


bool EDA_ANGLE::IsCardinal90() const
{
    // return true if angle is one of the two cardinal directions (90/270 degrees),
    double test = std::abs( m_value );

    while( test >= 180.0 )
        test -= 180.0;

    return test == 90.0;
}
