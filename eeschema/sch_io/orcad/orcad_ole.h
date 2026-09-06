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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef ORCAD_OLE_H_
#define ORCAD_OLE_H_

#include <cstdint>
#include <vector>

#include <math/vector2d.h>
#include <wx/string.h>

class wxImage;

enum class ORCAD_OLE_PREVIEW_TYPE
{
    NONE,
    BMP,
    DIB,
    WMF
};

struct ORCAD_OLE_PREVIEW
{
    ORCAD_OLE_PREVIEW_TYPE type = ORCAD_OLE_PREVIEW_TYPE::NONE;
    std::vector<uint8_t>   data;
};

ORCAD_OLE_PREVIEW OrcadExtractOlePreview( const std::vector<uint8_t>& aPayload );

/** Reassemble an EMF carried by WMF META_ESCAPE_ENHANCED_METAFILE records. */
std::vector<uint8_t> OrcadExtractEmbeddedEmf( const std::vector<uint8_t>& aWmf );

/** Extract the full raster appended after an OrCAD ~~CI_IMAGE~~ preview. */
std::vector<uint8_t> OrcadExtractCiImage( const std::vector<uint8_t>& aPayload );

/** Include leading bytes when the payload format is unknown. */
wxString OrcadDescribeImagePayload( const std::vector<uint8_t>& aPayload );

bool OrcadRenderWmf( const std::vector<uint8_t>& aWmf, int aMaxWidth, int aMaxHeight, wxImage& aImage,
                     double aTargetAspect = 0.0 );

bool OrcadRenderMetafilePreview( const std::vector<uint8_t>& aWmf, int aMaxWidth, int aMaxHeight,
                                 wxImage& aImage, double aTargetAspect = 0.0,
                                 bool* aUsedEmbeddedEmf = nullptr );

#endif
