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

#ifndef GAL_BITMAP_TEXTURE_H
#define GAL_BITMAP_TEXTURE_H

#include <gal/gal.h>

#include <cstddef>
#include <cstdint>
#include <vector>

class wxImage;


namespace KIGFX
{

/** Texture memory the GPU canvases may hold for placed bitmaps before they evict the oldest */
constexpr size_t BITMAP_CACHE_MAX_BYTES = 256 * 1024 * 1024;

/**
 * Fit an image into the largest texture a GPU can hold.
 *
 * @param aMaxTextureSize is the largest texture width or height the GPU takes, or 0 if unknown.
 * @return aImage when it fits, else a copy scaled down with its aspect ratio kept, invalid if scaling failed.
 */
GAL_API wxImage FitImageToTexture( const wxImage& aImage, int aMaxTextureSize );

/**
 * Expand packed RGB to tightly packed RGBA.
 *
 * @param aAlpha is one alpha byte per pixel, or nullptr for opaque pixels.
 */
GAL_API std::vector<uint8_t> RgbToRgba( const uint8_t* aRgb, const uint8_t* aAlpha, size_t aPixelCount );

/** The pixels of aImage as tightly packed RGBA, transparent where it has alpha or a mask */
GAL_API std::vector<uint8_t> ImageToRgba( const wxImage& aImage );

} // namespace KIGFX

#endif // GAL_BITMAP_TEXTURE_H
