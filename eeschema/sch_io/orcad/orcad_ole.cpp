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


wxString OrcadDescribeImagePayload( const std::vector<uint8_t>& aPayload )
{
    if( aPayload.empty() )
        return wxS( "empty" );

    auto starts = [&]( std::initializer_list<uint8_t> aSig, size_t aOffset = 0 )
    {
        if( aPayload.size() < aOffset + aSig.size() )
            return false;

        return std::equal( aSig.begin(), aSig.end(), aPayload.begin() + aOffset );
    };

    if( starts( { 0x89, 'P', 'N', 'G' } ) )         return wxS( "PNG" );
    if( starts( { 0xFF, 0xD8, 0xFF } ) )            return wxS( "JPEG" );
    if( starts( { 'G', 'I', 'F', '8' } ) )          return wxS( "GIF" );
    if( starts( { 'B', 'M' } ) )                    return wxS( "BMP" );
    if( starts( { 'I', 'I', 0x2A, 0x00 } ) )        return wxS( "TIFF" );
    if( starts( { 'M', 'M', 0x00, 0x2A } ) )        return wxS( "TIFF" );
    if( starts( { 0xD7, 0xCD, 0xC6, 0x9A } ) )      return wxS( "placeable WMF" );
    if( starts( { 0x01, 0x00, 0x09, 0x00 } ) )      return wxS( "WMF" );
    if( starts( { 'E', 'M', 'F', 0x20 }, 40 ) )     return wxS( "EMF" );
    if( starts( { 0xD0, 0xCF, 0x11, 0xE0 } ) )      return wxS( "OLE compound document" );

    wxString head;

    for( size_t i = 0; i < std::min<size_t>( 8, aPayload.size() ); ++i )
        head += wxString::Format( wxS( "%02X" ), aPayload[i] );

    return wxString::Format( wxS( "unrecognized, %zu bytes starting %s" ), aPayload.size(), head );
}
