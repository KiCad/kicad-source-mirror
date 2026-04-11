/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Mario Luzeiro <mrluzeiro@ua.pt>
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
 * @file  xv3d_types.h
 * @brief
 */

#ifndef XV3D_TYPES_H
#define XV3D_TYPES_H

#define GLM_FORCE_RADIANS

#include <glm/glm.hpp>
#include <glm/ext.hpp>

typedef glm::uvec2      SFVEC2UI;
typedef glm::ivec2      SFVEC2I;
typedef glm::u64vec2    SFVEC2UI64;
typedef glm::i64vec2    SFVEC2I64;
typedef glm::vec2       SFVEC2F;
typedef glm::dvec2      SFVEC2D;
typedef glm::vec3       SFVEC3F;
typedef glm::dvec3      SFVEC3D;
typedef glm::vec4       SFVEC4F;
typedef glm::uvec3      SFVEC3UI;
typedef glm::dvec3      SFVEC3D;

#define CLASS_ALIGNMENT 16

#endif // XV3D_TYPES_H
