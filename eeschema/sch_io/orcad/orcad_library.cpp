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


ORCAD_PAGE_SETTINGS OrcadParsePageSettings( ORCAD_STREAM& aStream )
{
    ORCAD_PAGE_SETTINGS settings;

    settings.createTimestamp = aStream.ReadU32();
    settings.modifyTimestamp = aStream.ReadU32();
    aStream.Skip( 16 );                 // unknown

    settings.width = aStream.ReadU32();
    settings.height = aStream.ReadU32();
    settings.pinToPin = aStream.ReadU32();

    aStream.Skip( 2 );
    settings.horizontalCount = aStream.ReadU16();
    settings.verticalCount = aStream.ReadU16();
    aStream.Skip( 2 );
    settings.horizontalWidth = aStream.ReadU32();
    settings.verticalWidth = aStream.ReadU32();
    aStream.Skip( 48 );                 // unknown
    settings.horizontalChar = aStream.ReadU32() != 0;
    aStream.Skip( 4 );
    settings.horizontalAscending = aStream.ReadU32() != 0;
    settings.verticalChar = aStream.ReadU32() != 0;
    aStream.Skip( 4 );
    settings.verticalAscending = aStream.ReadU32() != 0;

    // 8 x u32 flags; flags[0] set = metric units (width/height in um, not mils)
    uint32_t flags[8];

    for( uint32_t& flag : flags )
        flag = aStream.ReadU32();

    settings.isMetric = flags[0] != 0;
    settings.borderDisplayed = flags[1] != 0;
    settings.borderPrinted = flags[2] != 0;
    settings.gridRefDisplayed = flags[3] != 0;
    settings.gridRefPrinted = flags[4] != 0;
    settings.titleblockDisplayed = flags[5] != 0;
    settings.titleblockPrinted = flags[6] != 0;
    settings.ansiGridRefs = flags[7] != 0;

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
    lib.createTimestamp = stream.ReadU32();
    lib.modifyTimestamp = stream.ReadU32();
    stream.Skip( 4 );                   // zeros

    // u16 font count stores count+1; count-1 LOGFONTA records follow
    int fontCount = stream.ReadU16();

    for( int i = 0; i < fontCount - 1; i++ )
    {
        // 60-byte LOGFONTA; numeric fields through lfWeight, flags, then lfFaceName char[32].
        ORCAD_FONT font;
        font.height = stream.ReadI32();
        font.width = stream.ReadI32();
        font.escapement = stream.ReadI32();
        font.orientation = stream.ReadI32();
        font.bold = stream.ReadI32() >= 600;
        font.italic = stream.ReadU8() != 0;
        stream.Skip( 6 ); // underline, strikeout, charset and precision bytes
        font.pitchAndFamily = stream.ReadU8();

        std::vector<uint8_t> face = stream.ReadBytes( 32 );
        size_t               faceLen = 0;

        while( faceLen < face.size() && face[faceLen] != 0 )
            faceLen++;

        font.face.assign( reinterpret_cast<const char*>( face.data() ), faceLen );

        lib.fonts.push_back( std::move( font ) );
    }

    // Design Template font indices, then reserved slots and flags; v1.x has 17 slots and no count
    uint16_t templateFontCount = lib.versionMajor >= 2 ? stream.ReadU16() : 17;
    lib.templateFonts.resize( templateFontCount );

    for( uint16_t i = 0; i < templateFontCount; ++i )
    {
        int fontIdx = stream.ReadU16();
        lib.templateFonts[i] = fontIdx;

        if( i == 10 && fontIdx > 0 )
            lib.pinNameFont = i;
        else if( i == 11 && fontIdx > 0 )
            lib.pinNumberFont = i;
    }

    stream.Skip( 8 );

    // 8 named part fields (Part Reference, Value, ...)
    for( int i = 0; i < 8; i++ )
        lib.partFields.push_back( stream.ReadLzt() );

    ORCAD_PAGE_SETTINGS settings = OrcadParsePageSettings( stream );
    lib.pinToPin = settings.pinToPin;

    // The count width belongs to the format version: pre-2003 files store u16, later ones u32.
    uint32_t stringCount = lib.versionMajor < 3 ? stream.ReadU16() : stream.ReadU32();

    if( stringCount > 2000000
        || static_cast<uint64_t>( stringCount ) * 3 > static_cast<uint64_t>( stream.Remaining() ) + 16 )
    {
        THROW_IO_ERRORF( wxS( "OrCAD library: string table count %u does not fit the stream" ), stringCount );
    }

    lib.strings.reserve( stringCount );

    for( uint32_t i = 0; i < stringCount; i++ )
        lib.strings.push_back( stream.ReadLzt() );

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
