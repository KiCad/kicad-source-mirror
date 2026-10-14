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

#ifndef RHI_VERTEX_H
#define RHI_VERTEX_H

#include <rhi/rhi_pipeline.h>

#include <cmath>
#include <cstdint>
#include <cstring>

#include <glm/gtc/packing.hpp>


/**
 * Position is in world coordinates, color is premultiplied alpha and u, v hold half floats. There is
 * no Z, so draw order within the render pass decides what lands on top.
 */
struct RHI_VERTEX
{
    float    x = 0.0f;
    float    y = 0.0f;
    uint8_t  r = 255;
    uint8_t  g = 255;
    uint8_t  b = 255;
    uint8_t  a = 255;
    uint16_t u = 0;
    uint16_t v = 0;

    void SetPosition( float aX, float aY ) { x = aX; y = aY; }
    void SetColor( uint8_t aR, uint8_t aG, uint8_t aB, uint8_t aA = 255 )
    {
        r = aR; g = aG; b = aB; a = aA;
    }
    void SetUV( float aU, float aV ) { u = glm::packHalf1x16( aU ); v = glm::packHalf1x16( aV ); }
};

static_assert( sizeof( RHI_VERTEX ) == 16, "RHI_VERTEX must be exactly 16 bytes" );
static_assert( alignof( RHI_VERTEX ) <= 4, "RHI_VERTEX must not require more than 4-byte alignment" );


/**
 * Shaders read position at location 0, color at 1 and texture coordinates at 2.
 */
inline RHI_VERTEX_BUFFER_LAYOUT_DESC RhiStandardVertexLayout()
{
    RHI_VERTEX_BUFFER_LAYOUT_DESC layout;
    layout.stride   = sizeof( RHI_VERTEX );
    layout.stepMode = RHI_VERTEX_STEP_MODE::VERTEX;
    layout.attributes = {
        { 0, RHI_VERTEX_FORMAT::FLOAT32X2,   0 },
        { 1, RHI_VERTEX_FORMAT::UINT8X4_NORM, 8 },
        { 2, RHI_VERTEX_FORMAT::FLOAT16X2,   12 }
    };
    return layout;
}


#endif // RHI_VERTEX_H
