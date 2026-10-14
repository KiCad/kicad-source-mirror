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

#ifndef RHI_INSTANCE_H
#define RHI_INSTANCE_H

#include <rhi/rhi_vertex.h>

#include <cstdint>


/**
 * Instance flags controlling how the shaders draw an element.
 */
enum RHI_INSTANCE_FLAG : uint16_t
{
    RHI_INSTANCE_NEGATIVE     = 1 << 3,
    RHI_INSTANCE_SCREEN_SPACE = 1 << 4,
    RHI_INSTANCE_GROW_OUTWARD = 1 << 5,   /**< a PIXEL_CIRCLE stroke grows outward from its inner edge */
    RHI_INSTANCE_FADE_SMALL   = 1 << 6,   /**< text stroke that fades as its glyph height (param 2) shrinks */
    RHI_INSTANCE_HARD_EDGE    = 1 << 7,   /**< no antialiased edge, for layers that take a pixel once */
    RHI_INSTANCE_UNDIMMED     = 1 << 8,   /**< keeps its colour when its layer is dimmed */
    RHI_INSTANCE_PIXEL_CIRCLE = 1 << 9,   /**< a circle on whole pixels, as the OpenGL canvas draws it */
    RHI_INSTANCE_OVERLAY_ALPHA = 1 << 10, /**< premultiplied with alpha squared, as the OpenGL overlay leaves it */
    RHI_INSTANCE_SCALE_LOD    = 1 << 11,  /**< a track shown only while the view scale is above param 3 */
    RHI_INSTANCE_LENGTH_PAD   = 1 << 14,  /**< a track longer at each end by param 2 pixels */
    RHI_INSTANCE_BITMAP_GLYPH = 1 << 15,  /**< a track that draws the bitmap font glyph named by param 2 */
};


/**
 * Per-instance data for instanced 2D rendering, packed into 40 bytes.
 *
 * Params are float32 because float16 tops out at 65504 nm. Rotation, scale and depth are float16 so they
 * share one FLOAT16X4 attribute.
 */
struct RHI_INSTANCE
{
    float    x        = 0.0f;
    float    y        = 0.0f;
    uint16_t rotation = 0;
    uint16_t scaleX   = glm::packHalf1x16( 1.0f );
    uint16_t scaleY   = glm::packHalf1x16( 1.0f );
    uint16_t depth    = 0;
    float    params[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    uint8_t  r        = 255;
    uint8_t  g        = 255;
    uint8_t  b        = 255;
    uint8_t  a        = 255;
    uint16_t flags    = 0;
    uint16_t pixelPad = 0;     /**< extra stroke width in sixteenths of a pixel */

    void SetPosition( float aX, float aY ) { x = aX; y = aY; }

    void SetRotation( float aRadians ) { rotation = glm::packHalf1x16( aRadians ); }

    void SetDepth( float aNormalized )
    {
        depth = glm::packHalf1x16( aNormalized );
    }

    void SetColor( uint8_t aR, uint8_t aG, uint8_t aB, uint8_t aA = 255 )
    {
        r = aR; g = aG; b = aB; a = aA;
    }

    void SetParam( int aIndex, float aValue )
    {
        if( aIndex >= 0 && aIndex < 4 )
            params[aIndex] = aValue;
    }

    void SetFlag( RHI_INSTANCE_FLAG aFlag )   { flags |= aFlag; }
    void ClearFlag( RHI_INSTANCE_FLAG aFlag ) { flags &= static_cast<uint16_t>( ~aFlag ); }
    bool HasFlag( RHI_INSTANCE_FLAG aFlag ) const { return ( flags & aFlag ) != 0; }
};

static_assert( sizeof( RHI_INSTANCE ) == 40, "RHI_INSTANCE must be exactly 40 bytes" );
static_assert( alignof( RHI_INSTANCE ) <= 4, "RHI_INSTANCE must not require more than 4-byte alignment" );


/**
 * Per-instance vertex layout of RHI_INSTANCE, starting at location 3 after the RHI_VERTEX attributes.
 */
inline RHI_VERTEX_BUFFER_LAYOUT_DESC RhiInstanceLayout()
{
    RHI_VERTEX_BUFFER_LAYOUT_DESC layout;
    layout.stride   = sizeof( RHI_INSTANCE );
    layout.stepMode = RHI_VERTEX_STEP_MODE::INSTANCE;
    layout.attributes = {
        { 3, RHI_VERTEX_FORMAT::FLOAT32X2,    0 },
        { 4, RHI_VERTEX_FORMAT::FLOAT16X4,    8 },
        { 5, RHI_VERTEX_FORMAT::FLOAT32X4,   16 },
        { 6, RHI_VERTEX_FORMAT::UINT8X4_NORM, 32 },
        { 7, RHI_VERTEX_FORMAT::UINT16X2,    36 }
    };
    return layout;
}


#endif // RHI_INSTANCE_H
