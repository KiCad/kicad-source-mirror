/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * Based on the dsn2kicad reference implementation and on OrCAD file format
 * documentation from the OpenOrCadParser project (MIT licensed).
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

#include <sch_io/orcad/orcad_library.h>

#include <cstdint>
#include <utility>

#include <ki_exception.h>


namespace
{

int32_t i32At( const std::vector<uint8_t>& aBytes, size_t aOffset )
{
    return static_cast<int32_t>( static_cast<uint32_t>( aBytes[aOffset] )
                                 | static_cast<uint32_t>( aBytes[aOffset + 1] ) << 8
                                 | static_cast<uint32_t>( aBytes[aOffset + 2] ) << 16
                                 | static_cast<uint32_t>( aBytes[aOffset + 3] ) << 24 );
}

} // namespace


ORCAD_PAGE_SETTINGS OrcadParsePageSettings( ORCAD_STREAM& aStream )
{
    ORCAD_PAGE_SETTINGS settings;

    aStream.Skip( 8 );                  // create/modify dates
    aStream.Skip( 16 );                 // unknown

    settings.width = aStream.ReadU32();
    settings.height = aStream.ReadU32();
    settings.pinToPin = aStream.ReadU32();

    aStream.Skip( 2 );
    aStream.Skip( 4 );                  // horizontal/vertical count
    aStream.Skip( 2 );
    aStream.Skip( 8 );                  // horizontal/vertical width
    aStream.Skip( 48 );                 // unknown
    aStream.Skip( 24 );                 // grid-reference character settings

    // 8 x u32 flags; flags[0] set = metric units (width/height in um, not mils)
    uint32_t flags[8];

    for( uint32_t& flag : flags )
        flag = aStream.ReadU32();

    settings.isMetric = flags[0] != 0;

    return settings;
}


ORCAD_LIBRARY_INFO OrcadParseLibrary( const std::vector<char>& aData )
{
    ORCAD_STREAM       stream( aData );
    ORCAD_LIBRARY_INFO lib;

    // Introduction NUL-terminated in fixed 32-byte buffer; version words at offset 32
    lib.introduction = stream.ReadZt();
    stream.Seek( 32 );

    lib.versionMajor = stream.ReadU16();
    lib.versionMinor = stream.ReadU16();
    stream.Skip( 8 );                   // create/modify dates
    stream.Skip( 4 );                   // zeros

    // u16 font count stores count+1; count-1 LOGFONTA records follow
    int fontCount = stream.ReadU16();

    for( int i = 0; i < fontCount - 1; i++ )
    {
        // 60-byte LOGFONTA; lfHeight i32 +0, lfWeight i32 +16, lfItalic u8 +20, lfFaceName char[32] +28
        std::vector<uint8_t> rec = stream.ReadBytes( 60 );

        ORCAD_FONT font;
        font.height = i32At( rec, 0 );
        font.italic = rec[20] != 0;
        font.bold = i32At( rec, 16 ) >= 600;

        size_t faceLen = 0;

        while( faceLen < 32 && rec[28 + faceLen] != 0 )
            faceLen++;

        font.face.assign( reinterpret_cast<const char*>( rec.data() + 28 ), faceLen );

        lib.fonts.push_back( std::move( font ) );
    }

    if( lib.versionMajor >= 2 )
    {
        // u16 length (always 24), 2 * length bytes, 8 bytes
        uint16_t someLen = stream.ReadU16();
        stream.Skip( 2 * static_cast<size_t>( someLen ) );
        stream.Skip( 8 );
    }
    else
    {
        // v1.x fixed 42 bytes (17 x u16 + 2 x u32, no count field)
        stream.Skip( 42 );
    }

    // 8 named part fields (Part Reference, Value, ...)
    for( int i = 0; i < 8; i++ )
        lib.partFields.push_back( stream.ReadLzt() );

    ORCAD_PAGE_SETTINGS settings = OrcadParsePageSettings( stream );
    lib.pinToPin = settings.pinToPin;

    size_t tablePos = stream.GetOffset();

    // Count = u32 (modern) or u16 (legacy). Wrong width yields implausible count, so
    // sanity-check each vs remaining bytes; failed read keeps parsed strings, never aborts file
    auto readStringTable = [&]( bool aU16Count ) -> bool
    {
        stream.Seek( tablePos );
        lib.strings.clear();

        uint32_t count = aU16Count ? stream.ReadU16() : stream.ReadU32();

        if( count > 2000000
            || static_cast<uint64_t>( count ) * 3
                       > static_cast<uint64_t>( stream.Remaining() ) + 16 )
        {
            return false;
        }

        for( uint32_t i = 0; i < count; i++ )
            lib.strings.push_back( stream.ReadLzt() );

        return true;
    };

    // Legacy v2.x always u16; u32-first can pass sanity w/ bogus large count -> huge alloc
    bool u16First = lib.versionMajor < 3;

    try
    {
        if( !readStringTable( u16First ) && !readStringTable( !u16First ) )
        {
            stream.Seek( tablePos );
            lib.strings.clear();
        }
    }
    catch( const IO_ERROR& )
    {
        try
        {
            if( !readStringTable( !u16First ) )
            {
                stream.Seek( tablePos );
                lib.strings.clear();
            }
        }
        catch( const IO_ERROR& )
        {
            stream.Seek( tablePos );
            lib.strings.clear();
        }
    }

    // Alias pairs and root schematic folder follow; both optional, so read failure
    // must not sink whole library
    try
    {
        uint16_t aliasCount = stream.ReadU16();

        if( static_cast<uint64_t>( aliasCount ) * 2 <= stream.Remaining() + 4 )
        {
            for( uint16_t i = 0; i < aliasCount; i++ )
            {
                std::string alias = stream.ReadLzt();
                std::string part = stream.ReadLzt();

                lib.partAliases.emplace_back( std::move( alias ), std::move( part ) );
            }
        }

        // Design files (not standalone libs) carry root schematic folder
        if( lib.introduction.rfind( "OrCAD Windows Design", 0 ) == 0 )
        {
            stream.Skip( 8 );
            lib.schematicName = stream.ReadLzt();
        }
    }
    catch( const IO_ERROR& )
    {
    }

    return lib;
}
