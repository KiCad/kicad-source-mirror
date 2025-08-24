/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file vertex_common.h
 * @brief Common defines and consts used in vertex related classes.
 */

#ifndef VERTEX_COMMON_H_
#define VERTEX_COMMON_H_

#include <gal/opengl/kiglew.h>    // Must be included first

#include <math/vector2d.h>

#include <cstddef>

namespace KIGFX
{
///< Possible types of shaders (keep consistent with the actual shader source in
///< kicad_vert.glsl and kicad_frag.glsl).
enum SHADER_MODE
{
    SHADER_NONE = 0,
    SHADER_FILLED_CIRCLE = 2,
    SHADER_STROKED_CIRCLE = 3,
    SHADER_FONT = 4,
    SHADER_LINE_A = 5,
    SHADER_LINE_B = 6,
    SHADER_LINE_C = 7,
    SHADER_LINE_D = 8,
    SHADER_LINE_E = 9,
    SHADER_LINE_F = 10,
    SHADER_HOLE_WALL = 11
};

///< Data structure for vertices {X,Y,Z,R,G,B,A,shader&param}
struct VERTEX
{
    GLfloat x, y, z;        // Coordinates
    GLubyte r, g, b, a;     // Color
    GLfloat shader[4];      // Shader type & params
};

static constexpr size_t VERTEX_SIZE   = sizeof( VERTEX );
static constexpr size_t VERTEX_STRIDE = VERTEX_SIZE / sizeof( GLfloat );

static constexpr size_t COORD_OFFSET  = offsetof( VERTEX, x );
static constexpr size_t COORD_SIZE    = sizeof( VERTEX::x ) + sizeof( VERTEX::y ) +
                                        sizeof( VERTEX::z );
static constexpr size_t COORD_STRIDE  = COORD_SIZE / sizeof( GLfloat );

static constexpr size_t COLOR_OFFSET = offsetof( VERTEX, r );
static constexpr size_t COLOR_SIZE = sizeof( VERTEX::r ) + sizeof( VERTEX::g ) +
                                     sizeof( VERTEX::b ) + sizeof( VERTEX::a );
static constexpr size_t COLOR_STRIDE = COLOR_SIZE / sizeof( GLubyte );

// Shader attributes
static constexpr size_t SHADER_OFFSET = offsetof( VERTEX, shader );
static constexpr size_t SHADER_SIZE = sizeof( VERTEX::shader );
static constexpr size_t SHADER_STRIDE = SHADER_SIZE / sizeof( GLfloat );

static constexpr size_t INDEX_SIZE = sizeof( GLuint );

} // namespace KIGFX

#endif /* VERTEX_COMMON_H_ */
