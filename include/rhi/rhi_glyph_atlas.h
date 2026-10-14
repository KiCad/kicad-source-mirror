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

#ifndef RHI_GLYPH_ATLAS_H
#define RHI_GLYPH_ATLAS_H

#include <cstdint>
#include <vector>

/**
 * Length of the shaders' glyph table. It must match GLYPH_TABLE_SIZE in glyph_atlas.glsl and cover
 * BUILTIN_FONT::GlyphCount().
 */
constexpr uint32_t RHI_GLYPH_TABLE_SIZE = 1280;

/**
 * Binding of the glyph table beside the atlas texture at 0, see RHI_UBO_BINDING.
 */
constexpr uint32_t RHI_GLYPH_TABLE_BINDING = 2;

/**
 * The bitmap font's multichannel signed distance atlas, which OPENGL_GAL draws net names and pad numbers
 * from. An RHI_INSTANCE_BITMAP_GLYPH instance indexes the table so one quad draws a glyph.
 */
struct RHI_GLYPH_ATLAS_DATA
{
    uint32_t             width = 0;
    uint32_t             height = 0;
    std::vector<uint8_t> rgba;    /**< width * height texels */

    /**
     * RHI_GLYPH_TABLE_SIZE rects of x, y, width, height in texels, then one entry for the atlas size.
     */
    std::vector<float>   table;
};

#endif // RHI_GLYPH_ATLAS_H
