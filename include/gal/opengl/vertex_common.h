/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
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

#include <GL/glew.h>

#include <cstddef>

namespace KIGFX
{
// Possible types of shaders
enum SHADER_MODE
{
    SHADER_NONE = 0,
    SHADER_LINE,
    SHADER_FILLED_CIRCLE,
    SHADER_STROKED_CIRCLE,
};

typedef struct
{
    GLfloat x, y, z;        // Coordinates
    GLubyte r, g, b, a;     // Color
    GLfloat shader[4];      // Shader type & params
} VERTEX;

///< Data structure for vertices {X,Y,Z,R,G,B,A,shader&param} (@see VERTEX).
const unsigned int VertexSize   = sizeof(VERTEX);
const unsigned int VertexStride = VertexSize / sizeof(GLfloat);

const unsigned int CoordSize    = sizeof(VERTEX().x) + sizeof(VERTEX().y) + sizeof(VERTEX().z);
const unsigned int CoordStride  = CoordSize / sizeof(GLfloat);

// Offset of color data from the beginning of each vertex data
const unsigned int ColorOffset  = offsetof(VERTEX, r);
const unsigned int ColorSize    = sizeof(VERTEX().r) + sizeof(VERTEX().g) +
                                  sizeof(VERTEX().b) + sizeof(VERTEX().a);
const unsigned int ColorStride  = ColorSize / sizeof(GLubyte);

// Shader attributes
const unsigned int ShaderOffset = offsetof(VERTEX, shader);
const unsigned int ShaderSize   = sizeof(VERTEX().shader);
const unsigned int ShaderStride = ShaderSize / sizeof(GLfloat);

const unsigned int IndexSize    = sizeof(GLuint);
} // namespace KIGFX

#endif /* VERTEX_COMMON_H_ */
