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


glm::mat4 CalcModelMatrix( const SFVEC3F& aOffset, const SFVEC3F& aRotation, const SFVEC3F& aScale )
{
    glm::mat4 matrix( 1.0f );

    matrix = glm::translate( matrix, aOffset );
    matrix = glm::rotate( matrix, glm::radians( -aRotation.z ), { 0.0f, 0.0f, 1.0f } );
    matrix = glm::rotate( matrix, glm::radians( -aRotation.y ), { 0.0f, 1.0f, 0.0f } );
    matrix = glm::rotate( matrix, glm::radians( -aRotation.x ), { 1.0f, 0.0f, 0.0f } );

    return glm::scale( matrix, aScale );
}
