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

#include <sch_io/orcad/orcad_ole.h>

#include <algorithm>
#include <array>


namespace
{

constexpr std::array<uint8_t, 8> CFB_MAGIC = { 0xD0, 0xCF, 0x11, 0xE0, 0xA1, 0xB1, 0x1A, 0xE1 };

} // namespace


ORCAD_OLE_PREVIEW OrcadExtractOlePreview( const std::vector<uint8_t>& aPayload )
{
    auto cfb = std::search( aPayload.begin(), aPayload.end(), CFB_MAGIC.begin(), CFB_MAGIC.end() );

    if( cfb == aPayload.end() )
        return {};

    return ExtractOleImage( &*cfb, static_cast<size_t>( aPayload.end() - cfb ) );
}


bool OrcadRenderWmf( const std::vector<uint8_t>& aWmf, int aMaxWidth, int aMaxHeight, wxImage& aImage )
{
    return OleRenderWmf( aWmf, aMaxWidth, aMaxHeight, aImage );
}
