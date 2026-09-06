/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <math/vector2d.h>
#include <wx/string.h>

class wxImage;
class wxMemoryBuffer;


enum class OLE_IMAGE_TYPE
{
    NONE,
    BMP, // Encoded raster data, including PNG and JPEG.
    DIB,
    WMF
};


struct OLE_IMAGE_PAYLOAD
{
    OLE_IMAGE_TYPE       type = OLE_IMAGE_TYPE::NONE;
    std::vector<uint8_t> data;
    std::string          streamName;
};


/** Prefer CONTENTS, then OlePres000, then the native stream. */
OLE_IMAGE_PAYLOAD ExtractOleImage( const uint8_t* aCfb, size_t aSize );

inline OLE_IMAGE_PAYLOAD ExtractOleImage( const std::vector<uint8_t>& aCfb )
{
    return ExtractOleImage( aCfb.data(), aCfb.size() );
}

/** The 26-byte prologue stores length at offset 22 and length plus 22 at offset 0. */
std::optional<std::pair<size_t, size_t>> OleEmbeddedCompoundFile( const std::vector<uint8_t>& aPayload );

/// Read the picture out of an OLE object payload that carries the 26-byte prologue.
OLE_IMAGE_PAYLOAD ExtractOleImageFromPayload( const std::vector<uint8_t>& aPayload );

/// Reassemble an EMF carried by WMF META_ESCAPE_ENHANCED_METAFILE records.
std::vector<uint8_t> OleExtractEmbeddedEmf( const std::vector<uint8_t>& aWmf );

/** The CI marker follows the preview DIB; the raster has a counted decimal length. */
std::vector<uint8_t> OleExtractCiImage( const std::vector<uint8_t>& aPayload );

/** Include leading bytes when the payload format is unknown. */
wxString OleDescribeImagePayload( const std::vector<uint8_t>& aPayload );

bool OleMakeBmpFromDib( const std::vector<uint8_t>& aDib, wxMemoryBuffer& aOut );

bool OleRenderWmf( const std::vector<uint8_t>& aWmf, int aMaxWidth, int aMaxHeight, wxImage& aImage,
                   double aTargetAspect = 0.0 );

/// Render a metafile preview, preferring an EMF the WMF carries over the WMF itself.
bool OleRenderMetafilePreview( const std::vector<uint8_t>& aWmf, int aMaxWidth, int aMaxHeight,
                               wxImage& aImage, double aTargetAspect = 0.0,
                               bool* aUsedEmbeddedEmf = nullptr );

bool OleRenderEmf( const std::vector<uint8_t>& aEmf, int aMaxWidth, int aMaxHeight, wxImage& aImage,
                   double aTargetAspect = 0.0 );

VECTOR2I OleWmfRenderSize( int aNaturalWidth, int aNaturalHeight, int aMaxWidth, int aMaxHeight,
                           double aTargetAspect );
