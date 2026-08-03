/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

class wxImage;
class wxMemoryBuffer;

enum class OLE_IMAGE_TYPE
{
    NONE,
    BMP,
    DIB,
    WMF
};


struct OLE_IMAGE_PAYLOAD
{
    OLE_IMAGE_TYPE       type = OLE_IMAGE_TYPE::NONE;
    std::vector<uint8_t> data;
    std::string          streamName;
};


OLE_IMAGE_PAYLOAD ExtractOleImage( const uint8_t* aCfb, size_t aSize );

inline OLE_IMAGE_PAYLOAD ExtractOleImage( const std::vector<uint8_t>& aCfb )
{
    return ExtractOleImage( aCfb.data(), aCfb.size() );
}

bool OleMakeBmpFromDib( const std::vector<uint8_t>& aDib, wxMemoryBuffer& aOut );
bool OleRenderWmf( const std::vector<uint8_t>& aWmf, int aMaxWidth, int aMaxHeight, wxImage& aImage );
