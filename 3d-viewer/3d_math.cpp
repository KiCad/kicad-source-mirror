/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Mario Luzeiro <mrluzeiro@ua.pt>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * @file  3d_math.cpp
 * @brief
 */


#include "3d_math.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <cmath>


glm::mat4 CalcModelMatrix( const SFVEC3F& aOffset, const SFVEC3F& aRotation, const SFVEC3F& aScale )
{
    glm::mat4 matrix( 1.0f );

    matrix = glm::translate( matrix, aOffset );
    matrix = glm::rotate( matrix, glm::radians( -aRotation.z ), { 0.0f, 0.0f, 1.0f } );
    matrix = glm::rotate( matrix, glm::radians( -aRotation.y ), { 0.0f, 1.0f, 0.0f } );
    matrix = glm::rotate( matrix, glm::radians( -aRotation.x ), { 1.0f, 0.0f, 0.0f } );

    return glm::scale( matrix, aScale );
}


SFVEC3F CalcModelRotation( const glm::mat3& aRotation )
{
    // CalcModelMatrix() builds Rz( -rz ) * Ry( -ry ) * Rx( -rx ), so this is the standard
    // ZYX decomposition run on the negated angles.
    const float sinBeta = -aRotation[0][2];
    const float beta = std::asin( glm::clamp( sinBeta, -1.0f, 1.0f ) );
    const float cosBeta = std::cos( beta );
    float       alpha;
    float       gamma;

    if( std::abs( cosBeta ) < 1e-6f )
    {
        // Gimbal lock leaves only the sum or difference of the outer angles observable.
        alpha = 0.0f;
        gamma = std::atan2( sinBeta * aRotation[2][1], aRotation[1][1] );
    }
    else
    {
        alpha = std::atan2( aRotation[1][2], aRotation[2][2] );
        gamma = std::atan2( aRotation[0][1], aRotation[0][0] );
    }

    return SFVEC3F( -glm::degrees( alpha ), -glm::degrees( beta ), -glm::degrees( gamma ) );
}


SFVEC3F InterpolateModelRotation( const SFVEC3F& aStart, const SFVEC3F& aEnd, float aT )
{
    const SFVEC3F zero( 0.0f );
    const SFVEC3F one( 1.0f );
    const glm::quat start( glm::mat3( CalcModelMatrix( zero, aStart, one ) ) );
    const glm::quat end( glm::mat3( CalcModelMatrix( zero, aEnd, one ) ) );

    return CalcModelRotation( glm::mat3_cast( glm::slerp( start, end, glm::clamp( aT, 0.0f, 1.0f ) ) ) );
}
