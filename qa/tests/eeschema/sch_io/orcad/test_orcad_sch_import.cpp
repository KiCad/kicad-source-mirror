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

#include <boost/test/unit_test.hpp>
#include <qa_utils/wx_utils/unit_test_utils.h>

#include <sch_io/orcad/sch_io_orcad.h>
#include <sch_io/orcad/orcad_cache.h>
#include <sch_io/orcad/orcad_cis.h>
#include <sch_io/orcad/orcad_converter.h>
#include <sch_io/orcad/orcad_library.h>
#include <sch_io/ole_image.h>
#include <sch_io/orcad/orcad_page.h>
#include <sch_io/ole_image.h>

#include <schematic.h>
#include <connection_graph.h>
#include <sch_screen.h>
#include <sch_sheet.h>
#include <sch_sheet_pin.h>
#include <sch_sheet_path.h>
#include <sch_symbol.h>
#include <sch_label.h>
#include <sch_line.h>
#include <sch_junction.h>
#include <sch_no_connect.h>
#include <sch_bitmap.h>
#include <sch_shape.h>
#include <sch_text.h>
#include <sch_textbox.h>
#include <sch_pin.h>
#include <lib_symbol.h>
#include <geometry/shape_compound.h>
#include <reporter.h>
#include <settings/settings_manager.h>
#include <bitmap_base.h>

#include <wx/ffile.h>
#include <wx/filefn.h>
#include <wx/filename.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <string>


namespace
{

static void appendLe32( std::vector<uint8_t>& aBytes, uint32_t aValue )
{
    for( int shift = 0; shift < 32; shift += 8 )
        aBytes.push_back( static_cast<uint8_t>( aValue >> shift ) );
}


static void appendLe16( std::vector<uint8_t>& aBytes, uint16_t aValue )
{
    aBytes.push_back( static_cast<uint8_t>( aValue ) );
    aBytes.push_back( static_cast<uint8_t>( aValue >> 8 ) );
}


static void appendLzt( std::vector<uint8_t>& aBytes, const std::string& aValue )
{
    appendLe16( aBytes, static_cast<uint16_t>( aValue.size() ) );
    aBytes.insert( aBytes.end(), aValue.begin(), aValue.end() );
    aBytes.push_back( 0 );
}


// aLongPrefixCount long prefixes, one short prefix with no properties, preamble, empty trailer.
static void appendFramedHeader( std::vector<uint8_t>& aBytes, uint8_t aType, size_t aLongPrefixCount )
{
    size_t start = aBytes.size();
    size_t end = start + 9 * aLongPrefixCount + 3 + 8;

    for( size_t i = 0; i < aLongPrefixCount; ++i )
    {
        aBytes.push_back( aType );
        appendLe32( aBytes, static_cast<uint32_t>( end - ( start + 9 * i + 9 ) ) );
        appendLe32( aBytes, 0 );
    }

    aBytes.push_back( aType );
    appendLe16( aBytes, 0 );
    aBytes.insert( aBytes.end(), { 0xFF, 0xE4, 0x5C, 0x39 } );
    appendLe32( aBytes, 0 );
}


// Smallest Library stream that reaches the string table: header, one font slot, the eight part
// field names and the 156-byte page settings block.
static std::vector<char> makeLibraryStream( uint16_t aVersionMajor, const std::vector<std::string>& aStrings )
{
    std::vector<uint8_t> bytes( 32, 0 );
    const std::string    introduction = "OrCAD Windows Design";
    std::copy( introduction.begin(), introduction.end(), bytes.begin() );

    appendLe16( bytes, aVersionMajor );
    appendLe16( bytes, 0 );
    bytes.resize( bytes.size() + 12, 0 );
    appendLe16( bytes, 1 );

    appendLe16( bytes, 1 );
    appendLe16( bytes, 0 );
    bytes.resize( bytes.size() + 8, 0 );

    for( int i = 0; i < 8; ++i )
        appendLzt( bytes, "" );

    bytes.resize( bytes.size() + 156, 0 );

    if( aVersionMajor < 3 )
        appendLe16( bytes, static_cast<uint16_t>( aStrings.size() ) );
    else
        appendLe32( bytes, static_cast<uint32_t>( aStrings.size() ) );

    for( const std::string& text : aStrings )
        appendLzt( bytes, text );

    appendLe16( bytes, 0 );
    bytes.resize( bytes.size() + 8, 0 );
    appendLzt( bytes, "SCHEMATIC1" );

    return std::vector<char>( bytes.begin(), bytes.end() );
}


static std::vector<char> cisFramed( const std::vector<uint8_t>& aPayload )
{
    std::vector<char> bytes;
    uint32_t          length = static_cast<uint32_t>( aPayload.size() );

    for( int shift = 0; shift < 32; shift += 8 )
        bytes.push_back( static_cast<char>( length >> shift ) );

    bytes.insert( bytes.end(), aPayload.begin(), aPayload.end() );
    return bytes;
}


static void writeLe16( std::vector<uint8_t>& aBytes, size_t aOffset, uint16_t aValue )
{
    aBytes[aOffset] = static_cast<uint8_t>( aValue );
    aBytes[aOffset + 1] = static_cast<uint8_t>( aValue >> 8 );
}


static void writeLe32( std::vector<uint8_t>& aBytes, size_t aOffset, uint32_t aValue )
{
    for( int shift = 0; shift < 32; shift += 8 )
        aBytes[aOffset++] = static_cast<uint8_t>( aValue >> shift );
}


static std::vector<uint8_t> makeGlyphIndexWmf()
{
    std::vector<uint8_t> bytes;
    appendLe16( bytes, 1 );
    appendLe16( bytes, 9 );
    appendLe16( bytes, 0x0300 );
    appendLe32( bytes, 0 );
    appendLe16( bytes, 1 );
    appendLe32( bytes, 10 );
    appendLe16( bytes, 0 );

    auto record = [&]( uint16_t aFunction, std::initializer_list<uint16_t> aParams )
    {
        appendLe32( bytes, static_cast<uint32_t>( aParams.size() + 3 ) );
        appendLe16( bytes, aFunction );

        for( uint16_t param : aParams )
            appendLe16( bytes, param );
    };

    record( 0x0103, { 8 } );
    record( 0x020B, { 0, 0 } );
    record( 0x020C, { 100, 100 } );
    record( 0x012E, { 0 } );
    record( 0x0102, { 1 } );
    record( 0x041B, { 90, 90, 10, 10 } );
    record( 0x0A32, { 50, 50, 2, 0x0010, 0x4241, 8, 8 } );
    record( 0x02FC, { 0, 0xFFFF, 0x00FF, 0 } );
    record( 0x012D, { 0 } );
    record( 0x0940, { 0x0029, 0x00AA, 0, 0, 0, 100, 100, 0, 0 } );
    record( 0, {} );
    writeLe32( bytes, 6, static_cast<uint32_t>( bytes.size() / 2 ) );
    return bytes;
}


static std::vector<uint8_t> makeFlippedDibWmf()
{
    std::vector<uint8_t> bytes;
    appendLe16( bytes, 1 );
    appendLe16( bytes, 9 );
    appendLe16( bytes, 0x0300 );
    appendLe32( bytes, 0 );
    appendLe16( bytes, 0 );
    appendLe32( bytes, 0 );
    appendLe16( bytes, 0 );

    auto record = [&]( uint16_t aFunction, std::initializer_list<uint16_t> aParams )
    {
        appendLe32( bytes, static_cast<uint32_t>( aParams.size() + 3 ) );
        appendLe16( bytes, aFunction );

        for( uint16_t param : aParams )
            appendLe16( bytes, param );
    };

    record( 0x0103, { 8 } );
    record( 0x020B, { 0, 0 } );
    record( 0x020C, { 100, 100 } );

    const size_t dibRecord = bytes.size();
    appendLe32( bytes, 0 );
    appendLe16( bytes, 0x0B41 );
    appendLe16( bytes, 0x0020 );
    appendLe16( bytes, 0x00CC );
    appendLe16( bytes, 2 );
    appendLe16( bytes, 2 );
    appendLe16( bytes, 0 );
    appendLe16( bytes, 0 );
    appendLe16( bytes, static_cast<uint16_t>( -80 ) );
    appendLe16( bytes, 80 );
    appendLe16( bytes, 90 );
    appendLe16( bytes, 10 );

    appendLe32( bytes, 40 );
    appendLe32( bytes, 2 );
    appendLe32( bytes, 2 );
    appendLe16( bytes, 1 );
    appendLe16( bytes, 24 );
    appendLe32( bytes, 0 );
    appendLe32( bytes, 16 );
    appendLe32( bytes, 0 );
    appendLe32( bytes, 0 );
    appendLe32( bytes, 0 );
    appendLe32( bytes, 0 );

    bytes.insert( bytes.end(), { 0, 0, 255, 0, 255, 0, 0, 0 } );
    bytes.insert( bytes.end(), { 255, 0, 0, 255, 255, 255, 0, 0 } );
    writeLe32( bytes, dibRecord, static_cast<uint32_t>( ( bytes.size() - dibRecord ) / 2 ) );

    record( 0, {} );
    writeLe32( bytes, 6, static_cast<uint32_t>( bytes.size() / 2 ) );
    writeLe32( bytes, 12, static_cast<uint32_t>( ( bytes.size() - dibRecord ) / 2 ) );
    return bytes;
}


static std::vector<uint8_t> makeEmbeddedEmfWmf( const std::vector<uint8_t>& aEmf = {} )
{
    std::vector<uint8_t> emf = aEmf;

    if( emf.empty() )
    {
        emf.resize( 100, 0 );
        writeLe32( emf, 0, 1 );
        writeLe32( emf, 4, 88 );
        writeLe32( emf, 40, 0x464D4520 );
        writeLe32( emf, 48, emf.size() );
    }

    std::vector<uint8_t> wmf;
    appendLe16( wmf, 1 );
    appendLe16( wmf, 9 );
    appendLe16( wmf, 0x0300 );
    appendLe32( wmf, 0 );
    appendLe16( wmf, 0 );
    appendLe32( wmf, 0 );
    appendLe16( wmf, 0 );

    size_t sourceOffset = 0;
    const size_t chunkSizes[] = { emf.size() / 2, emf.size() - emf.size() / 2 };

    for( size_t chunkSize : chunkSizes )
    {
        size_t recordStart = wmf.size();
        appendLe32( wmf, 0 );
        appendLe16( wmf, 0x0626 );
        appendLe16( wmf, 0x000F );
        appendLe16( wmf, static_cast<uint16_t>( 34 + chunkSize ) );
        appendLe32( wmf, 0x43464D57 );
        appendLe32( wmf, 1 );
        appendLe32( wmf, 0x00010000 );
        appendLe16( wmf, 0 );
        appendLe32( wmf, 0 );
        appendLe32( wmf, std::size( chunkSizes ) );
        appendLe32( wmf, chunkSize );
        appendLe32( wmf, emf.size() - sourceOffset - chunkSize );
        appendLe32( wmf, emf.size() );
        wmf.insert( wmf.end(), emf.begin() + sourceOffset, emf.begin() + sourceOffset + chunkSize );

        if( wmf.size() % 2 )
            wmf.push_back( 0 );

        writeLe32( wmf, recordStart, ( wmf.size() - recordStart ) / 2 );
        sourceOffset += chunkSize;
    }

    appendLe32( wmf, 3 );
    appendLe16( wmf, 0 );
    writeLe32( wmf, 6, wmf.size() / 2 );
    return wmf;
}


static std::vector<uint8_t> makeRenderableEmf()
{
    std::vector<uint8_t> emf( 108, 0 );
    writeLe32( emf, 0, 1 );
    writeLe32( emf, 4, 108 );
    writeLe32( emf, 16, 100 );
    writeLe32( emf, 20, 100 );
    writeLe32( emf, 32, 2646 );
    writeLe32( emf, 36, 2646 );
    writeLe32( emf, 40, 0x464D4520 );
    writeLe32( emf, 44, 0x00010000 );
    writeLe32( emf, 52, 23 );
    writeLe16( emf, 56, 4 );
    writeLe32( emf, 72, 100 );
    writeLe32( emf, 76, 100 );
    writeLe32( emf, 80, 26 );
    writeLe32( emf, 84, 26 );
    writeLe32( emf, 100, 26000 );
    writeLe32( emf, 104, 26000 );

    auto pointRecord = [&]( uint32_t aType, int32_t aX, int32_t aY )
    {
        appendLe32( emf, aType );
        appendLe32( emf, 16 );
        appendLe32( emf, static_cast<uint32_t>( aX ) );
        appendLe32( emf, static_cast<uint32_t>( aY ) );
    };

    pointRecord( 27, 10, 10 );
    pointRecord( 54, 90, 10 );

    appendLe32( emf, 82 );
    appendLe32( emf, 104 );
    appendLe32( emf, 1 );
    appendLe32( emf, static_cast<uint32_t>( -20 ) );
    appendLe32( emf, 0 );
    appendLe32( emf, 900 );
    appendLe32( emf, 900 );
    appendLe32( emf, 400 );
    emf.insert( emf.end(), 8, 0 );
    const std::u16string face = u"Source Sans Pro";

    for( size_t i = 0; i < 32; ++i )
        appendLe16( emf, i < face.size() ? face[i] : 0 );

    appendLe32( emf, 37 );
    appendLe32( emf, 12 );
    appendLe32( emf, 1 );
    appendLe32( emf, 24 );
    appendLe32( emf, 12 );
    appendLe32( emf, 0x00CC0000 );
    appendLe32( emf, 22 );
    appendLe32( emf, 12 );
    appendLe32( emf, 0x18 );

    appendLe32( emf, 84 );
    appendLe32( emf, 88 );
    emf.insert( emf.end(), 16, 0 );
    appendLe32( emf, 1 );
    appendLe32( emf, 0x3F800000 );
    appendLe32( emf, 0x3F800000 );
    appendLe32( emf, 40 );
    appendLe32( emf, 70 );
    appendLe32( emf, 2 );
    appendLe32( emf, 76 );
    appendLe32( emf, 0 );
    emf.insert( emf.end(), 16, 0 );
    appendLe32( emf, 80 );
    appendLe16( emf, 'H' );
    appendLe16( emf, 'i' );
    appendLe32( emf, 12 );
    appendLe32( emf, 12 );

    appendLe32( emf, 82 );
    appendLe32( emf, 104 );
    appendLe32( emf, 2 );
    appendLe32( emf, static_cast<uint32_t>( -20 ) );
    appendLe32( emf, 0 );
    appendLe32( emf, 0 );
    appendLe32( emf, 0 );
    appendLe32( emf, 400 );
    emf.insert( emf.end(), 8, 0 );

    for( size_t i = 0; i < 32; ++i )
        appendLe16( emf, i < face.size() ? face[i] : 0 );

    appendLe32( emf, 37 );
    appendLe32( emf, 12 );
    appendLe32( emf, 2 );

    appendLe32( emf, 84 );
    appendLe32( emf, 88 );
    emf.insert( emf.end(), 16, 0 );
    appendLe32( emf, 1 );
    appendLe32( emf, 0x3F800000 );
    appendLe32( emf, 0x3F800000 );
    appendLe32( emf, 70 );
    appendLe32( emf, 30 );
    appendLe32( emf, 2 );
    appendLe32( emf, 76 );
    appendLe32( emf, 0x10 );
    emf.insert( emf.end(), 16, 0 );
    appendLe32( emf, 80 );
    appendLe16( emf, 1211 );
    appendLe16( emf, 3 );
    appendLe32( emf, 20 );
    appendLe32( emf, 5 );

    appendLe32( emf, 82 );
    appendLe32( emf, 104 );
    appendLe32( emf, 3 );
    appendLe32( emf, static_cast<uint32_t>( -20 ) );
    appendLe32( emf, 0 );
    appendLe32( emf, 0 );
    appendLe32( emf, 0 );
    appendLe32( emf, 400 );
    emf.insert( emf.end(), 8, 0 );
    const std::u16string calibri = u"Calibri";

    for( size_t i = 0; i < 32; ++i )
        appendLe16( emf, i < calibri.size() ? calibri[i] : 0 );

    appendLe32( emf, 37 );
    appendLe32( emf, 12 );
    appendLe32( emf, 3 );
    appendLe32( emf, 84 );
    appendLe32( emf, 84 );
    emf.insert( emf.end(), 16, 0 );
    appendLe32( emf, 1 );
    appendLe32( emf, 0x3F800000 );
    appendLe32( emf, 0x3F800000 );
    appendLe32( emf, 5 );
    appendLe32( emf, 95 );
    appendLe32( emf, 1 );
    appendLe32( emf, 76 );
    appendLe32( emf, 0x10 );
    emf.insert( emf.end(), 16, 0 );
    appendLe32( emf, 80 );
    appendLe16( emf, 3 );
    appendLe16( emf, 0 );
    appendLe32( emf, 10 );

    appendLe32( emf, 85 );
    appendLe32( emf, 44 );
    emf.insert( emf.end(), 16, 0 );
    appendLe32( emf, 4 );

    for( const std::pair<uint16_t, uint16_t>& point :
         { std::pair<uint16_t, uint16_t>{ 10, 40 }, { 20, 20 }, { 30, 60 }, { 40, 40 } } )
    {
        appendLe16( emf, point.first );
        appendLe16( emf, point.second );
    }

    appendLe32( emf, 59 );
    appendLe32( emf, 8 );
    pointRecord( 27, 10, 90 );
    pointRecord( 54, 90, 90 );
    appendLe32( emf, 60 );
    appendLe32( emf, 8 );
    appendLe32( emf, 64 );
    appendLe32( emf, 24 );
    emf.insert( emf.end(), 16, 0 );

    appendLe32( emf, 91 );
    appendLe32( emf, 52 );
    emf.insert( emf.end(), 16, 0 );
    appendLe32( emf, 1 );
    appendLe32( emf, 4 );
    appendLe32( emf, 4 );

    for( const std::pair<uint16_t, uint16_t>& point :
         { std::pair<uint16_t, uint16_t>{ 60, 60 }, { 70, 50 }, { 80, 60 }, { 70, 70 } } )
    {
        appendLe16( emf, point.first );
        appendLe16( emf, point.second );
    }

    appendLe32( emf, 58 );
    appendLe32( emf, 12 );
    appendLe32( emf, 0x40400000 );

    appendLe32( emf, 14 );
    appendLe32( emf, 20 );
    emf.insert( emf.end(), 12, 0 );
    writeLe32( emf, 48, emf.size() );
    return emf;
}


static std::vector<uint8_t> makeCalibriMetricEmf( const std::u16string& aText = u"UMC" )
{
    std::vector<uint8_t> emf( 108, 0 );
    writeLe32( emf, 0, 1 );
    writeLe32( emf, 4, 108 );
    writeLe32( emf, 16, 100 );
    writeLe32( emf, 20, 50 );
    writeLe32( emf, 32, 2646 );
    writeLe32( emf, 36, 1323 );
    writeLe32( emf, 40, 0x464D4520 );
    writeLe32( emf, 44, 0x00010000 );
    writeLe32( emf, 52, 6 );
    writeLe16( emf, 56, 2 );
    writeLe32( emf, 72, 100 );
    writeLe32( emf, 76, 50 );
    writeLe32( emf, 80, 26 );
    writeLe32( emf, 84, 13 );
    writeLe32( emf, 100, 26000 );
    writeLe32( emf, 104, 13000 );

    appendLe32( emf, 82 );
    appendLe32( emf, 104 );
    appendLe32( emf, 1 );
    appendLe32( emf, static_cast<uint32_t>( -20 ) );
    appendLe32( emf, 0 );
    appendLe32( emf, 0 );
    appendLe32( emf, 0 );
    appendLe32( emf, 400 );
    emf.insert( emf.end(), 8, 0 );
    const std::u16string calibri = u"Calibri";

    for( size_t i = 0; i < 32; ++i )
        appendLe16( emf, i < calibri.size() ? calibri[i] : 0 );

    appendLe32( emf, 37 );
    appendLe32( emf, 12 );
    appendLe32( emf, 1 );
    appendLe32( emf, 22 );
    appendLe32( emf, 12 );
    appendLe32( emf, 0x18 );

    size_t stringBytes = ( aText.size() * 2 + 3 ) & ~size_t( 3 );
    appendLe32( emf, 84 );
    appendLe32( emf, 76 + stringBytes + aText.size() * 4 );
    emf.insert( emf.end(), 16, 0 );
    appendLe32( emf, 1 );
    appendLe32( emf, 0x3F800000 );
    appendLe32( emf, 0x3F800000 );
    appendLe32( emf, 5 );
    appendLe32( emf, 5 );
    appendLe32( emf, aText.size() );
    appendLe32( emf, 76 );
    appendLe32( emf, 0 );
    emf.insert( emf.end(), 16, 0 );
    appendLe32( emf, 76 + stringBytes );

    for( char16_t c : aText )
        appendLe16( emf, c );

    emf.insert( emf.end(), stringBytes - aText.size() * 2, 0 );

    for( size_t i = 0; i < aText.size(); ++i )
        appendLe32( emf, i == 1 ? 16 : i == 2 ? 10 : 12 );

    appendLe32( emf, 14 );
    appendLe32( emf, 20 );
    emf.insert( emf.end(), 12, 0 );
    writeLe32( emf, 48, emf.size() );
    return emf;
}


// Wrap a compound file in the 26-byte prologue an OrCAD OLE payload carries: the compound
// file's length plus 22 at offset 0, and the length itself at offset 22.
static std::vector<uint8_t> makeOlePayload( const std::vector<uint8_t>& aCfb, uint32_t aDeclaredLength )
{
    std::vector<uint8_t> payload( 26, 0 );
    writeLe32( payload, 0, aDeclaredLength + 22 );
    writeLe32( payload, 22, aDeclaredLength );
    payload.insert( payload.end(), aCfb.begin(), aCfb.end() );

    return payload;
}


static std::vector<uint8_t> makeOlePreviewCfb( const std::vector<uint16_t>& aName, const std::vector<uint8_t>& aStream )
{
    constexpr uint32_t FREE_SECTOR = 0xFFFFFFFF;
    constexpr uint32_t END_OF_CHAIN = 0xFFFFFFFE;
    constexpr uint32_t FAT_SECTOR = 0xFFFFFFFD;
    constexpr size_t   SECTOR_SIZE = 512;
    constexpr size_t   STREAM_SECTORS = 8;

    std::vector<uint8_t> cfb( SECTOR_SIZE * ( 3 + STREAM_SECTORS ), 0 );
    const uint8_t        magic[] = { 0xD0, 0xCF, 0x11, 0xE0, 0xA1, 0xB1, 0x1A, 0xE1 };
    std::copy( std::begin( magic ), std::end( magic ), cfb.begin() );
    writeLe16( cfb, 24, 0x003E );
    writeLe16( cfb, 26, 3 );
    writeLe16( cfb, 28, 0xFFFE );
    writeLe16( cfb, 30, 9 );
    writeLe16( cfb, 32, 6 );
    writeLe32( cfb, 44, 1 );
    writeLe32( cfb, 48, 1 );
    writeLe32( cfb, 56, 4096 );
    writeLe32( cfb, 60, END_OF_CHAIN );
    writeLe32( cfb, 68, END_OF_CHAIN );

    for( size_t i = 0; i < 109; ++i )
        writeLe32( cfb, 76 + 4 * i, i == 0 ? 0 : FREE_SECTOR );

    size_t fat = SECTOR_SIZE;
    writeLe32( cfb, fat, FAT_SECTOR );
    writeLe32( cfb, fat + 4, END_OF_CHAIN );

    for( size_t i = 0; i < STREAM_SECTORS; ++i )
        writeLe32( cfb, fat + 4 * ( 2 + i ), i + 1 == STREAM_SECTORS ? END_OF_CHAIN : 3 + i );

    for( size_t i = 2 + STREAM_SECTORS; i < SECTOR_SIZE / 4; ++i )
        writeLe32( cfb, fat + 4 * i, FREE_SECTOR );

    size_t root = 2 * SECTOR_SIZE;
    writeLe16( cfb, root, 'R' );
    writeLe16( cfb, root + 2, 0 );
    writeLe16( cfb, root + 64, 4 );
    cfb[root + 66] = 5;
    writeLe32( cfb, root + 68, FREE_SECTOR );
    writeLe32( cfb, root + 72, FREE_SECTOR );
    writeLe32( cfb, root + 76, 1 );
    writeLe32( cfb, root + 116, END_OF_CHAIN );

    size_t entry = root + 128;

    for( size_t i = 0; i < aName.size(); ++i )
        writeLe16( cfb, entry + 2 * i, aName[i] );

    writeLe16( cfb, entry + 64, static_cast<uint16_t>( 2 * aName.size() ) );
    cfb[entry + 66] = 2;
    writeLe32( cfb, entry + 68, FREE_SECTOR );
    writeLe32( cfb, entry + 72, FREE_SECTOR );
    writeLe32( cfb, entry + 76, FREE_SECTOR );
    writeLe32( cfb, entry + 116, 2 );
    writeLe32( cfb, entry + 120, STREAM_SECTORS * SECTOR_SIZE );

    std::copy_n( aStream.begin(), std::min( aStream.size(), STREAM_SECTORS * SECTOR_SIZE ),
                 cfb.begin() + 3 * SECTOR_SIZE );
    return cfb;
}


static std::vector<uint8_t> makeOleWmfPreview( const std::vector<uint8_t>& aWmf )
{
    std::vector<uint8_t> presentation( 40, 0 );
    writeLe32( presentation, 4, 14 );
    presentation.insert( presentation.end(), aWmf.begin(), aWmf.end() );

    const std::vector<uint16_t> name = { 2, 'O', 'l', 'e', 'P', 'r', 'e', 's', '0', '0', '0', 0 };
    std::vector<uint8_t> cfb = makeOlePreviewCfb( name, presentation );
    return makeOlePayload( cfb, cfb.size() );
}

/// Throw-away temp file; impostor fixtures synthesized at runtime since user designs not redistributable.
struct TEMP_TEST_FILE
{
    TEMP_TEST_FILE( const wxString& aFileName, const wxString& aContents ) :
            m_path( wxFileName( wxFileName::GetTempDir(), aFileName ).GetFullPath() )
    {
        wxFFile file( m_path, wxS( "w" ) );

        if( file.IsOpened() )
            file.Write( aContents );
    }

    ~TEMP_TEST_FILE() { wxRemoveFile( m_path ); }

    wxString m_path;
};

} // namespace


struct ORCAD_SCH_IMPORT_FIXTURE
{
    ORCAD_SCH_IMPORT_FIXTURE() :
            m_schematic( new SCHEMATIC( nullptr ) )
    {
        m_manager.LoadProject( "" );
        m_schematic->SetProject( &m_manager.Prj() );
        m_schematic->CurrentSheet().clear();
        m_schematic->CurrentSheet().push_back( &m_schematic->Root() );
    }

    ~ORCAD_SCH_IMPORT_FIXTURE() { m_schematic.reset(); }

    std::string dataPath( const std::string& aRelPath ) const
    {
        return KI_TEST::GetEeschemaTestDataDir() + "io/orcad/" + aRelPath;
    }

    SCH_SHEET* LoadOrcadSchematic( const std::string& aRelPath )
    {
        return m_plugin.LoadSchematicFile( dataPath( aRelPath ), m_schematic.get() );
    }

    SCH_IO_ORCAD               m_plugin;
    std::unique_ptr<SCHEMATIC> m_schematic;
    SETTINGS_MANAGER           m_manager;
};


BOOST_FIXTURE_TEST_SUITE( OrcadSchImport, ORCAD_SCH_IMPORT_FIXTURE )


BOOST_AUTO_TEST_CASE( CisVariantFallbackSortsBomNamesBytewise )
{
    std::vector<uint8_t> payload = { '2', 0xF9, '5', 'V', 0xF9, '1', '2', 'V' };
    std::vector<std::string> names = OrcadCisParseCountedList( cisFramed( payload ), 0xF9 );

    BOOST_REQUIRE_EQUAL( names.size(), 2u );
    BOOST_CHECK_EQUAL( OrcadCisSelectVariant( names, std::nullopt ), "12V" );
    BOOST_CHECK_EQUAL( OrcadCisSelectVariant( names, std::optional<std::string>( "5V" ) ), "5V" );
    BOOST_CHECK_THROW( OrcadCisSelectVariant( names, std::optional<std::string>( "9V" ) ), IO_ERROR );
}


BOOST_AUTO_TEST_CASE( CisLegacyEmptyVariantListHasNoSelection )
{
    std::vector<std::string> names = OrcadCisParseCountedList( cisFramed( { 0xFB } ), 0xF9 );

    BOOST_CHECK( names.empty() );
    BOOST_CHECK( OrcadCisSelectVariant( names, std::nullopt ).empty() );
    BOOST_CHECK_THROW( OrcadCisParseCountedList( cisFramed( {} ), 0xF9 ), IO_ERROR );
}


BOOST_AUTO_TEST_CASE( CisVariantPropertyUpdatesParseRealRecordShape )
{
    std::vector<uint8_t> payload;

    auto appendText = [&]( const std::string& aText ) { payload.insert( payload.end(), aText.begin(), aText.end() ); };
    auto separator = [&]( uint8_t aByte ) { payload.push_back( aByte ); };

    appendText( "2835" );
    separator( 0xB0 );
    appendText( "SiLabsPN^Value^Voltage" );
    separator( 0xC0 );
    appendText( "NVMFS5C680NLT1G^NVMFS5C680NLT1G^60V~2829" );
    separator( 0xB0 );
    appendText( "Part Number^Value" );
    separator( 0xC0 );
    appendText( "PDS5100H-13^PDS5100~" );

    auto updates = OrcadCisParsePropertyUpdates( cisFramed( payload ) );

    BOOST_REQUIRE_EQUAL( updates.size(), 2u );
    BOOST_CHECK_EQUAL( updates.at( 2835 ).at( "Value" ), "NVMFS5C680NLT1G" );
    BOOST_CHECK_EQUAL( updates.at( 2835 ).at( "Voltage" ), "60V" );
    BOOST_CHECK_EQUAL( updates.at( 2829 ).at( "Value" ), "PDS5100" );
}


BOOST_AUTO_TEST_CASE( CisVariantMembershipRetainsInstalledState )
{
    std::vector<uint8_t> payload = { '1', 0xB0, '2', '8', '3', '5', '~',
                                     '0', 0xB0, '2', '2', '1', '2', '~' };
    auto memberships = OrcadCisParseMemberships( cisFramed( payload ) );

    BOOST_REQUIRE_EQUAL( memberships.size(), 2u );
    BOOST_CHECK( memberships.at( 2835 ) );
    BOOST_CHECK( !memberships.at( 2212 ) );
}


BOOST_AUTO_TEST_CASE( CisSchematicInfoMapsVariantPropertiesToPageDatabaseIds )
{
    std::vector<uint8_t> bytes;
    appendLe32( bytes, 1 );
    appendLe32( bytes, 1656123 );
    appendLe32( bytes, 2 );
    appendLzt( bytes, "Output Voltage-12V" );
    appendLzt( bytes, "Output Voltage-5V" );
    appendLe32( bytes, 2 );
    appendLzt( bytes, "1" );
    appendLzt( bytes, "1" );
    appendLe32( bytes, 2 );

    for( const auto& [value, voltage] :
         { std::pair<std::string, std::string>( "NVMFS5C680NLT1G", "60V" ),
           std::pair<std::string, std::string>( "SiR422DP-T1", "40V" ) } )
    {
        appendLe32( bytes, 2 );
        appendLzt( bytes, "Value" );
        appendLzt( bytes, "Voltage" );
        appendLe32( bytes, 2 );
        appendLzt( bytes, value );
        appendLzt( bytes, voltage );
    }

    appendLe32( bytes, 1 );
    std::vector<char> data( bytes.begin(), bytes.end() );
    auto groups = OrcadCisParseSchematicInfo( data );

    BOOST_CHECK_EQUAL( groups.at( "Output Voltage-12V" ).at( 1656123 ).at( "Value" ),
                       "NVMFS5C680NLT1G" );
    BOOST_CHECK_EQUAL( groups.at( "Output Voltage-5V" ).at( 1656123 ).at( "Voltage" ), "40V" );
}


BOOST_AUTO_TEST_CASE( StructurePrefixDepthIsTypeOwned )
{
    const std::map<int, int> expected = { { 9, 1 },  { 10, 2 }, { 2, 3 },  { 13, 4 }, { 24, 5 },
                                          { 66, 1 }, { 16, 2 }, { 12, 3 }, { 64, 4 } };

    for( const auto& [type, depth] : expected )
    {
        std::optional<size_t> actual = OrcadLongPrefixCount( type );
        BOOST_REQUIRE_MESSAGE( actual, "type " << type );
        BOOST_CHECK_EQUAL( *actual, static_cast<size_t>( depth ) );
    }

    BOOST_CHECK( !OrcadLongPrefixCount( 0 ) );
    BOOST_CHECK( !OrcadLongPrefixCount( 255 ) );
}


BOOST_AUTO_TEST_CASE( StructurePrefixDepthDoesNotRetry )
{
    // A LibraryPart chain still carries its 41 short-prefix property pairs at the registered depth.
    std::vector<uint8_t> bytes;
    std::vector<size_t>  lengthOffsets;

    for( int i = 0; i < 5; ++i )
    {
        bytes.push_back( ORCAD_ST_LIBRARY_PART );
        lengthOffsets.push_back( bytes.size() );
        appendLe32( bytes, 0 );
        appendLe32( bytes, 0 );
    }

    bytes.push_back( ORCAD_ST_LIBRARY_PART );
    appendLe16( bytes, 41 );

    for( int i = 0; i < 41; ++i )
    {
        appendLe32( bytes, 0 );
        appendLe32( bytes, 0 );
    }

    bytes.insert( bytes.end(), { 0xFF, 0xE4, 0x5C, 0x39 } );
    appendLe32( bytes, 0 );

    for( size_t offset : lengthOffsets )
        writeLe32( bytes, offset, static_cast<uint32_t>( bytes.size() - offset - 8 ) );

    ORCAD_STREAM        stream( bytes.data(), bytes.size() );
    ORCAD_STRUCT_READER reader( stream );
    ORCAD_PREFIXES      prefixes = reader.ReadPrefixes( ORCAD_ST_LIBRARY_PART );

    BOOST_CHECK_EQUAL( prefixes.bodyLens.size(), 5u );
    BOOST_CHECK_EQUAL( prefixes.props.size(), 41u );
    BOOST_CHECK_EQUAL( stream.GetOffset(), bytes.size() );

    // A chain one prefix deeper than the type owns must not be accepted by probing.
    std::vector<uint8_t> tooDeep;
    appendFramedHeader( tooDeep, ORCAD_ST_DRAWN_INSTANCE, 4 );
    ORCAD_STREAM        tooDeepStream( tooDeep.data(), tooDeep.size() );
    ORCAD_STRUCT_READER tooDeepReader( tooDeepStream );

    BOOST_CHECK_THROW( tooDeepReader.ReadPrefixes( ORCAD_ST_DRAWN_INSTANCE ), IO_ERROR );

    std::vector<uint8_t> correct;
    appendFramedHeader( correct, ORCAD_ST_DRAWN_INSTANCE, 3 );
    ORCAD_STREAM        correctStream( correct.data(), correct.size() );
    ORCAD_STRUCT_READER correctReader( correctStream );

    BOOST_CHECK_EQUAL( correctReader.ReadPrefixes( ORCAD_ST_DRAWN_INSTANCE ).bodyLens.size(), 3u );
}


BOOST_AUTO_TEST_CASE( StructureBodyLimitIsEnforced )
{
    const uint8_t bytes[] = { 1, 2, 3, 4 };
    ORCAD_STREAM  stream( bytes, sizeof( bytes ) );

    {
        ORCAD_STREAM::LIMIT_GUARD limit( stream, 2 );
        BOOST_CHECK_THROW( stream.ReadU32(), IO_ERROR );
    }

    BOOST_CHECK_EQUAL( stream.ReadU32(), 0x04030201u );
}


BOOST_AUTO_TEST_CASE( CacheEmptyStreamIsMarkerPlusFourZeroSections )
{
    // The ten-byte cache seen in the wild is not a special case: it is the u16 marker and the
    // four section counts, all zero, consumed exactly.
    std::vector<char>                       data( 10, 0 );
    std::map<std::string, ORCAD_SYMBOL_DEF> symbols;
    std::map<std::string, ORCAD_PACKAGE>    packages;
    std::vector<wxString>                   warnings;

    ORCAD_WARN_FN warn = [&]( const wxString& aMsg )
    {
        warnings.push_back( aMsg );
    };

    BOOST_CHECK_NO_THROW( OrcadParseCache( data, {}, warn, symbols, packages ) );
    BOOST_CHECK( warnings.empty() );
    BOOST_CHECK( symbols.empty() );
    BOOST_CHECK( packages.empty() );
}


BOOST_AUTO_TEST_CASE( CacheTrailingBytesAfterFourSectionsAreReported )
{
    // Exactly four sections are consumed. Anything after them means the walk lost alignment,
    // which must be reported rather than ignored or thrown out of the import.
    std::vector<char>                       data( 11, 0 );
    std::map<std::string, ORCAD_SYMBOL_DEF> symbols;
    std::map<std::string, ORCAD_PACKAGE>    packages;
    std::vector<wxString>                   warnings;

    ORCAD_WARN_FN warn = [&]( const wxString& aMsg )
    {
        warnings.push_back( aMsg );
    };

    BOOST_CHECK_NO_THROW( OrcadParseCache( data, {}, warn, symbols, packages ) );
    BOOST_CHECK_EQUAL( warnings.size(), 1u );
    BOOST_CHECK( symbols.empty() );
}


BOOST_AUTO_TEST_CASE( CacheSectionRejectsAForeignStructureType )
{
    // Section 0 holds loose symbols. A Package there means the section counts no longer
    // describe the stream, so the walk stops instead of decoding a record at the wrong offset.
    std::vector<uint8_t> bytes;
    appendLe16( bytes, 0 ); // marker
    appendLe16( bytes, 1 ); // section 0 group count
    appendLzt( bytes, "GROUP" );
    appendLe16( bytes, 1 ); // variant count
    appendLzt( bytes, "SOURCE.OLB" );
    appendLe32( bytes, 0 ); // created
    appendLe32( bytes, 0 ); // modified
    bytes.push_back( ORCAD_ST_PACKAGE );
    bytes.push_back( 0 );

    std::vector<char>                       data( bytes.begin(), bytes.end() );
    std::map<std::string, ORCAD_SYMBOL_DEF> symbols;
    std::map<std::string, ORCAD_PACKAGE>    packages;
    std::vector<wxString>                   warnings;

    ORCAD_WARN_FN warn = [&]( const wxString& aMsg )
    {
        warnings.push_back( aMsg );
    };

    BOOST_CHECK_NO_THROW( OrcadParseCache( data, {}, warn, symbols, packages ) );
    BOOST_CHECK_EQUAL( warnings.size(), 1u );
    BOOST_CHECK( symbols.empty() );
    BOOST_CHECK( packages.empty() );
}


BOOST_AUTO_TEST_CASE( LibraryStringTableCountWidthFollowsVersion )
{
    ORCAD_LIBRARY_INFO legacy = OrcadParseLibrary( makeLibraryStream( 2, { "SIZE", "N/A" } ) );
    BOOST_REQUIRE_EQUAL( legacy.strings.size(), 2u );
    BOOST_CHECK_EQUAL( legacy.strings[1], "N/A" );

    ORCAD_LIBRARY_INFO modern = OrcadParseLibrary( makeLibraryStream( 3, { "SIZE", "N/A" } ) );
    BOOST_REQUIRE_EQUAL( modern.strings.size(), 2u );
    BOOST_CHECK_EQUAL( modern.strings[1], "N/A" );

    // A u16 table under a version-3 header is rejected rather than retried at the other width.
    std::vector<char> mislabelled = makeLibraryStream( 2, { "SIZE", "N/A" } );
    mislabelled[32] = 3;

    BOOST_CHECK_THROW( OrcadParseLibrary( mislabelled ), IO_ERROR );
}


BOOST_AUTO_TEST_CASE( PrimitiveRecordLengthConventionIsChecked )
{
    // The legacy convention excludes the u32 byteLength and its pad from the stored count.
    std::vector<uint8_t> legacy = { ORCAD_PRIM_LINE, ORCAD_PRIM_LINE };
    appendLe32( legacy, 24 );
    appendLe32( legacy, 0 );

    for( uint32_t value : { 10u, 20u, 30u, 40u, 1u, 7u } )
        appendLe32( legacy, value );

    ORCAD_STREAM                   legacyStream( legacy.data(), legacy.size() );
    std::optional<ORCAD_PRIMITIVE> primitive = OrcadReadPrimitive( legacyStream );

    BOOST_REQUIRE( primitive );
    BOOST_CHECK_EQUAL( primitive->lineWidth, 7 );
    BOOST_CHECK_EQUAL( legacyStream.GetOffset(), legacy.size() );

    // Neither convention explains a byteLength of 25, so the record is refused.
    std::vector<uint8_t> mismatched = legacy;
    mismatched.resize( mismatched.size() + 8, 0 );
    writeLe32( mismatched, 2, 25 );

    ORCAD_STREAM mismatchedStream( mismatched.data(), mismatched.size() );
    BOOST_CHECK_THROW( OrcadReadPrimitive( mismatchedStream ), IO_ERROR );
}


BOOST_AUTO_TEST_CASE( CommentTextLengthOwnsItsRecordExtent )
{
    // CommentText carries padding past the string; the exclusive byteLength ends the record.
    std::vector<uint8_t> bytes = { ORCAD_PRIM_COMMENT_TEXT, ORCAD_PRIM_COMMENT_TEXT };
    size_t               lengthOffset = bytes.size();
    appendLe32( bytes, 0 );
    appendLe32( bytes, 0 );

    for( uint32_t value : { 10u, 20u, 30u, 40u, 10u, 20u } )
        appendLe32( bytes, value );

    appendLe16( bytes, 1 );
    appendLe16( bytes, 0 );
    appendLzt( bytes, "NOTE" );

    bytes.resize( bytes.size() + 20, 0 );
    writeLe32( bytes, lengthOffset, static_cast<uint32_t>( bytes.size() - 2 - 8 ) );

    ORCAD_STREAM                   stream( bytes.data(), bytes.size() );
    std::optional<ORCAD_PRIMITIVE> primitive = OrcadReadPrimitive( stream );

    BOOST_REQUIRE( primitive );
    BOOST_CHECK( primitive->kind == ORCAD_PRIM_KIND::TEXT );
    BOOST_CHECK_EQUAL( primitive->text, "NOTE" );
    BOOST_CHECK_EQUAL( stream.GetOffset(), bytes.size() );
}


BOOST_AUTO_TEST_CASE( PrimitiveLineWidth )
{
    std::vector<uint8_t> bytes = { ORCAD_PRIM_LINE, ORCAD_PRIM_LINE };
    appendLe32( bytes, 32 );
    appendLe32( bytes, 0 );
    appendLe32( bytes, 10 );
    appendLe32( bytes, 20 );
    appendLe32( bytes, 30 );
    appendLe32( bytes, 40 );
    appendLe32( bytes, 1 );
    appendLe32( bytes, 2 );

    ORCAD_STREAM                   stream( bytes.data(), bytes.size() );
    std::optional<ORCAD_PRIMITIVE> primitive = OrcadReadPrimitive( stream );

    BOOST_REQUIRE( primitive );
    BOOST_CHECK( primitive->kind == ORCAD_PRIM_KIND::LINE );
    BOOST_CHECK_EQUAL( primitive->lineWidth, 2 );
}


BOOST_AUTO_TEST_CASE( LibraryPartGeneralPropertiesPreserveImplementationPath )
{
    std::vector<uint8_t> bytes;
    appendLzt( bytes, "JMB582QH.Normal" );
    appendLzt( bytes, "IC.OLB" );
    appendLe32( bytes, 48 );
    appendLe16( bytes, 0 );

    for( int coordinate : { 0, 0, 100, 100 } )
        appendLe16( bytes, static_cast<uint16_t>( coordinate ) );

    size_t bboxEnd = bytes.size();
    appendLe16( bytes, 0 );
    appendLe16( bytes, 0 );

    // The fifth prefix bounds GeneralProperties; the innermost stop ends the symbol body.
    size_t tailStart = bytes.size();
    appendLzt( bytes, "JMB582" );
    appendLzt( bytes, "" );
    appendLzt( bytes, "U" );
    appendLzt( bytes, "JMB582QH" );
    appendLe16( bytes, 3 );

    ORCAD_STREAM        stream( bytes.data(), bytes.size() );
    ORCAD_STRUCT_READER reader( stream );
    ORCAD_PREFIXES      prefixes;
    prefixes.typeId = ORCAD_ST_LIBRARY_PART;
    prefixes.stops = { bytes.size(), tailStart, bboxEnd };
    prefixes.end = bytes.size();

    ORCAD_SYMBOL_DEF symbol = OrcadReadSymbolDef( reader, prefixes, true );

    BOOST_CHECK_EQUAL( symbol.props.at( "Implementation Path" ), "JMB582" );
    BOOST_CHECK_EQUAL( symbol.generalFlags, 3 );
}


BOOST_AUTO_TEST_CASE( ModernSymbolPinRetainsDisplayOverride )
{
    auto framed = []( uint8_t aType, const std::vector<uint8_t>& aBody )
    {
        std::vector<uint8_t> bytes;
        std::vector<size_t>  lengthOffsets;

        for( int i = 0; i < 2; ++i )
        {
            bytes.push_back( aType );
            lengthOffsets.push_back( bytes.size() );
            appendLe32( bytes, 0 );
            appendLe32( bytes, 0 );
        }

        bytes.push_back( aType );
        appendLe16( bytes, 0xFFFF );
        bytes.insert( bytes.end(), { 0xFF, 0xE4, 0x5C, 0x39 } );
        appendLe32( bytes, 0 );
        bytes.insert( bytes.end(), aBody.begin(), aBody.end() );

        for( size_t offset : lengthOffsets )
            writeLe32( bytes, offset, static_cast<uint32_t>( bytes.size() - offset - 8 ) );

        return bytes;
    };

    std::vector<uint8_t> displayBody;
    appendLe32( displayBody, 0 );
    appendLe16( displayBody, 20 );
    appendLe16( displayBody, 0 );
    appendLe16( displayBody, 0 );
    displayBody.push_back( 48 );
    appendLe16( displayBody, 0x01E9 );
    displayBody.push_back( 0 );
    std::vector<uint8_t> display = framed( ORCAD_ST_SYMBOL_DISPLAY_PROP, displayBody );

    std::vector<uint8_t> pinBody;
    appendLzt( pinBody, "F1" );

    for( int coordinate : { 20, 0, 20, -10 } )
        appendLe32( pinBody, static_cast<uint32_t>( coordinate ) );

    appendLe16( pinBody, 0x21 );
    appendLe16( pinBody, 0 );
    appendLe32( pinBody, static_cast<uint32_t>( ORCAD_PORT_TYPE::PASSIVE ) );
    pinBody.push_back( ORCAD_ST_SYMBOL_PIN_SCALAR );
    pinBody.insert( pinBody.end(), 3, 0 );
    appendLe16( pinBody, 1 );
    pinBody.insert( pinBody.end(), display.begin(), display.end() );
    std::vector<uint8_t> bytes = framed( ORCAD_ST_SYMBOL_PIN_SCALAR, pinBody );
    std::vector<std::string> strings = { "Pin Name" };
    ORCAD_STREAM              stream( bytes.data(), bytes.size() );
    ORCAD_STRUCT_READER       reader( stream, &strings );
    std::optional<ORCAD_SYMBOL_PIN> pin = OrcadReadSymbolPin( reader );

    BOOST_REQUIRE( pin );
    BOOST_REQUIRE_EQUAL( pin->displayProps.size(), 1u );
    BOOST_CHECK_EQUAL( pin->displayProps.front().name, "Pin Name" );
    BOOST_CHECK_EQUAL( pin->displayProps.front().x, 20 );
    BOOST_CHECK_EQUAL( pin->displayProps.front().rotation, 0 );
}


BOOST_AUTO_TEST_CASE( BusEntryCoordinatesPrecedeReservedWords )
{
    std::vector<uint8_t> bytes;
    appendLe32( bytes, 6 );
    appendLe32( bytes, 10 );
    appendLe32( bytes, 20 );
    appendLe32( bytes, 30 );
    appendLe32( bytes, 40 );
    appendLe32( bytes, 0 );
    appendLe32( bytes, 0 );

    ORCAD_STREAM    stream( bytes.data(), bytes.size() );
    ORCAD_BUS_ENTRY entry = OrcadReadBusEntryBody( stream );

    BOOST_CHECK_EQUAL( entry.color, 6 );
    BOOST_CHECK_EQUAL( entry.x1, 10 );
    BOOST_CHECK_EQUAL( entry.y1, 20 );
    BOOST_CHECK_EQUAL( entry.x2, 30 );
    BOOST_CHECK_EQUAL( entry.y2, 40 );
}


BOOST_AUTO_TEST_CASE( S593487_V2CommentTextRetainsBoundingBox )
{
    // A legacy Cache is a u16 zero marker then four counted sections: loose symbols,
    // LibraryParts, PartCells, Packages. A group names the definition and counts its variants.
    std::vector<uint8_t> cache;
    appendLe16( cache, 0 ); // marker
    appendLe16( cache, 0 ); // section 0, loose symbols
    appendLe16( cache, 1 ); // section 1, LibraryParts
    appendLzt( cache, "THERMISTOR_2.Normal" );
    appendLe16( cache, 2 ); // two variants of the same definition
    appendLzt( cache, "SOURCE.OLB" );
    appendLe32( cache, 0 );
    appendLe32( cache, 0 );
    cache.push_back( ORCAD_ST_LIBRARY_PART );
    cache.push_back( 0 );
    cache.push_back( ORCAD_ST_LIBRARY_PART );
    appendLe16( cache, 0 );
    appendLzt( cache, "THERMISTOR_2.Normal" );
    appendLzt( cache, "SOURCE.OLB" );
    appendLe32( cache, 48 );
    appendLe16( cache, 1 );
    cache.push_back( ORCAD_PRIM_COMMENT_TEXT );

    for( int coordinate : { 12, -8, 30, 7, 12, -8 } )
        appendLe32( cache, static_cast<uint32_t>( coordinate ) );

    appendLe16( cache, 27 );
    appendLe16( cache, 0 );
    appendLzt( cache, "t" );

    for( int coordinate : { 0, 0, 20, 30 } )
        appendLe16( cache, static_cast<uint16_t>( coordinate ) );

    appendLe16( cache, 0 );
    appendLe16( cache, 0 );
    appendLzt( cache, "" );
    appendLzt( cache, "" );
    appendLzt( cache, "Q" );
    appendLzt( cache, "" );
    appendLe16( cache, 6 );

    // Second variant, with its own header. Only the first variant of a group is preceded by
    // the group name and count; every later one starts straight at its source library.
    appendLzt( cache, "SOURCE.OLB" );
    appendLe32( cache, 0 );
    appendLe32( cache, 0 );
    cache.push_back( ORCAD_ST_LIBRARY_PART );
    cache.push_back( 0 );
    cache.push_back( ORCAD_ST_LIBRARY_PART );
    appendLe16( cache, 0 );
    appendLzt( cache, "THERMISTOR_2.Normal" );
    appendLzt( cache, "SOURCE.OLB" );
    appendLe32( cache, 48 );
    appendLe16( cache, 0 );

    for( int coordinate : { 0, 0, 30, 30 } )
        appendLe16( cache, static_cast<uint16_t>( coordinate ) );

    appendLe16( cache, 0 );
    appendLe16( cache, 0 );
    appendLzt( cache, "" );
    appendLzt( cache, "" );
    appendLzt( cache, "Q" );
    appendLzt( cache, "" );
    appendLe16( cache, 6 );

    appendLe16( cache, 0 ); // section 2, PartCells
    appendLe16( cache, 0 ); // section 3, Packages

    std::map<std::string, ORCAD_SYMBOL_DEF> symbols;
    std::map<std::string, ORCAD_PACKAGE>    packages;
    OrcadParseCacheV2(
            std::vector<char>( cache.begin(), cache.end() ), {},
            []( const wxString& )
            {
            },
            symbols, packages );

    BOOST_REQUIRE_EQUAL( symbols.size(), 1u );
    BOOST_REQUIRE_EQUAL( symbols.begin()->second.primitives.size(), 1u );
    const ORCAD_PRIMITIVE& text = symbols.begin()->second.primitives.front();
    BOOST_CHECK_EQUAL( text.x1, 12 );
    BOOST_CHECK_EQUAL( text.y1, -8 );
    BOOST_CHECK_EQUAL( text.x2, 30 );
    BOOST_CHECK_EQUAL( text.y2, 7 );
    BOOST_CHECK_EQUAL( symbols.begin()->second.generalFlags, 6 );
    BOOST_REQUIRE_EQUAL( symbols.begin()->second.variants.size(), 1u );
    BOOST_CHECK_EQUAL( symbols.begin()->second.variants.front().bbox->x2, 30 );
    BOOST_CHECK_EQUAL( symbols.begin()->second.variants.front().generalFlags, 6 );
}


BOOST_AUTO_TEST_CASE( CaptureColorPalette )
{
    BOOST_CHECK( OrcadColor( 8 ) == KIGFX::COLOR4D( 1.0, 0.0, 0.0, 1.0 ) );
    BOOST_CHECK( OrcadColor( 18 ) == KIGFX::COLOR4D( 0.0, 1.0, 0.0, 1.0 ) );
    BOOST_CHECK( OrcadColor( 28 ) == KIGFX::COLOR4D( 0.0, 0.0, 1.0, 1.0 ) );
    BOOST_CHECK( OrcadColor( 40 ) == KIGFX::COLOR4D( 0.0, 0.0, 0.0, 1.0 ) );
    BOOST_CHECK( OrcadColor( 47 ) == KIGFX::COLOR4D( 1.0, 1.0, 1.0, 1.0 ) );
    BOOST_CHECK( OrcadColor( 48 ) == KIGFX::COLOR4D::UNSPECIFIED );
    BOOST_CHECK( OrcadColor( 0 ) == KIGFX::COLOR4D::UNSPECIFIED );
}


BOOST_AUTO_TEST_CASE( PlacedPinSignedIndexCarriesNoConnectFlag )
{
    ORCAD_PIN_INST pin;

    pin.pinIndex = 7;
    BOOST_CHECK( !pin.IsNoConnect() );

    pin.pinIndex = -7;
    BOOST_CHECK( pin.IsNoConnect() );
}


BOOST_AUTO_TEST_CASE( CaptureCompoundFileName )
{
    BOOST_CHECK_EQUAL( OrcadNormalizeCfbName( std::string( "Serial I" ) + '\x02' + "O" ), "Serial I/O" );
    BOOST_CHECK_EQUAL( OrcadNormalizeCfbName( std::string( "Sch 2" ) + '\x03' + " PCI Connector" ),
                       "Sch 2: PCI Connector" );
}


BOOST_AUTO_TEST_CASE( CaptureStrokeAndFillSemantics )
{
    BOOST_CHECK_EQUAL( OrcadLineWidthIu( 0 ), schIUScale.MilsToIU( 10 ) );
    BOOST_CHECK_EQUAL( OrcadLineWidthIu( 1 ), schIUScale.MilsToIU( 30 ) );
    BOOST_CHECK_EQUAL( OrcadLineWidthIu( 2 ), schIUScale.MilsToIU( 50 ) );
    BOOST_CHECK_EQUAL( OrcadLineWidthIu( 3 ), schIUScale.MilsToIU( 10 ) );
    BOOST_CHECK_EQUAL( OrcadPageGraphicLineWidthIu( 0 ), schIUScale.MilsToIU( 5 ) );
    BOOST_CHECK_EQUAL( OrcadPageGraphicLineWidthIu( 1 ), schIUScale.MilsToIU( 30 ) );

    BOOST_CHECK( OrcadLineStyle( 0 ) == LINE_STYLE::SOLID );
    BOOST_CHECK( OrcadLineStyle( 4 ) == LINE_STYLE::DASHDOTDOT );
    BOOST_CHECK( OrcadLineStyle( 5 ) == LINE_STYLE::DEFAULT );

    BOOST_CHECK_EQUAL( OrcadDashRatios( 2 ).first, 67.0 );
    BOOST_CHECK_EQUAL( OrcadDashRatios( 2 ).second, 21.0 );
    BOOST_CHECK_EQUAL( OrcadDashRatios( 3 ).first, 3.0 );
    BOOST_CHECK_EQUAL( OrcadDashRatios( 3 ).second, 1.0 );

    BOOST_CHECK( OrcadFillType( 0, 0 ) == FILL_T::FILLED_SHAPE );
    BOOST_CHECK( OrcadFillType( 1, 0 ) == FILL_T::NO_FILL );
    BOOST_CHECK( OrcadFillType( 2, 0 ) == FILL_T::HATCH );
    BOOST_CHECK( OrcadFillType( 2, 3 ) == FILL_T::REVERSE_HATCH );
    BOOST_CHECK( OrcadFillType( 2, 4 ) == FILL_T::CROSS_HATCH );
    BOOST_CHECK( OrcadFillType( 2, 5 ) == FILL_T::CROSS_HATCH );
    constexpr uint32_t legacyModified = 0x56A2631C;
    constexpr uint32_t currentModified = 0x652CEBA8;
    BOOST_CHECK_EQUAL( OrcadHatchPitchIu( legacyModified ), schIUScale.MilsToIU( 25 ) );
    BOOST_CHECK_EQUAL( OrcadHatchPitchIu( currentModified ), schIUScale.MilsToIU( 80 ) );
    BOOST_CHECK_EQUAL( OrcadHatchLineWidthIu( legacyModified ), schIUScale.MilsToIU( 3 ) );
    BOOST_CHECK_EQUAL( OrcadHatchLineWidthIu( currentModified ), schIUScale.MilsToIU( 10 ) );

    SCH_SHAPE rect( SHAPE_T::RECTANGLE, LAYER_DEVICE );
    rect.SetPosition( VECTOR2I( 0, 0 ) );
    rect.SetEnd( VECTOR2I( schIUScale.MilsToIU( 600 ), schIUScale.MilsToIU( 500 ) ) );

    std::vector<SEG> reverseDiagonal = OrcadHatchLines( rect, 3, OrcadHatchPitchIu( legacyModified ) );
    BOOST_CHECK_GT( reverseDiagonal.size(), 40u );
    BOOST_CHECK_LT( reverseDiagonal.size(), 48u );
    BOOST_CHECK( std::all_of( reverseDiagonal.begin(), reverseDiagonal.end(),
                              []( const SEG& aLine )
                              {
                                  return static_cast<int64_t>( aLine.B.x - aLine.A.x )
                                             * ( aLine.B.y - aLine.A.y )
                                         < 0;
                              } ) );

    std::vector<SEG> diagonalCross = OrcadHatchLines( rect, 5, OrcadHatchPitchIu( legacyModified ) );
    BOOST_CHECK_GT( diagonalCross.size(), 80u );
    BOOST_CHECK_LT( diagonalCross.size(), 96u );

    std::vector<SEG> orthogonalCross = OrcadHatchLines( rect, 4, OrcadHatchPitchIu( legacyModified ) );
    BOOST_CHECK_GT( orthogonalCross.size(), 40u );
    BOOST_CHECK_LT( orthogonalCross.size(), 48u );
    BOOST_CHECK( std::any_of( orthogonalCross.begin(), orthogonalCross.end(),
                              []( const SEG& aLine ) { return aLine.A.x == aLine.B.x; } ) );
    BOOST_CHECK( std::any_of( orthogonalCross.begin(), orthogonalCross.end(),
                              []( const SEG& aLine ) { return aLine.A.y == aLine.B.y; } ) );

    std::vector<SEG> currentCross = OrcadHatchLines( rect, 5, OrcadHatchPitchIu( currentModified ) );
    BOOST_CHECK_GT( currentCross.size(), 20u );
    BOOST_CHECK_LT( currentCross.size(), 40u );
}


BOOST_AUTO_TEST_CASE( CapturePageOrder )
{
    wxString dashed = wxS( "03 - CAN" );
    wxString dotted = wxS( "02.uC" );
    wxString colon = wxS( "13:IMU" );
    wxString folder = wxS( "Sch 7: CAN Drivers" );
    wxString pager = wxS( "PAGER 8" );
    wxString underscored = wxS( "PAGE_03_HSMC CONNECTOR" );
    wxString compact = wxS( "PAGE01 OVERALL BLOCK DIAGRAM" );
    wxString plain = wxS( "Overview" );

    BOOST_CHECK_EQUAL( OrcadPageOrder( dashed ), 3 );
    BOOST_CHECK_EQUAL( dashed, wxS( "CAN" ) );
    BOOST_CHECK_EQUAL( OrcadPageOrder( dotted ), 2 );
    BOOST_CHECK_EQUAL( dotted, wxS( "02.uC" ) );
    BOOST_CHECK_EQUAL( OrcadPageOrder( colon ), 13 );
    BOOST_CHECK_EQUAL( colon, wxS( "13:IMU" ) );
    BOOST_CHECK_EQUAL( OrcadPageOrder( folder ), 7 );
    BOOST_CHECK_EQUAL( folder, wxS( "Sch 7: CAN Drivers" ) );
    BOOST_CHECK_EQUAL( OrcadPageOrder( pager ), -1 );
    BOOST_CHECK_EQUAL( pager, wxS( "PAGER 8" ) );
    BOOST_CHECK_EQUAL( OrcadPageOrder( underscored ), 3 );
    BOOST_CHECK_EQUAL( underscored, wxS( "PAGE_03_HSMC CONNECTOR" ) );
    BOOST_CHECK_EQUAL( OrcadPageOrder( compact ), 1 );
    BOOST_CHECK_EQUAL( compact, wxS( "PAGE01 OVERALL BLOCK DIAGRAM" ) );
    BOOST_CHECK_EQUAL( OrcadPageOrder( plain ), -1 );
    BOOST_CHECK_EQUAL( plain, wxS( "Overview" ) );
}


BOOST_AUTO_TEST_CASE( ModernSchematicStreamReversesStoredPageOrder )
{
    std::vector<uint8_t> bytes;
    appendFramedHeader( bytes, ORCAD_ST_SCH_LIB, 1 );
    appendLzt( bytes, "SCHEMATIC1" );
    appendLe32( bytes, 0 );
    appendLe16( bytes, 3 );
    appendLzt( bytes, "Block Diagram" );
    appendLzt( bytes, "I2C-USB" );
    appendLzt( bytes, "Street Fighter" );

    std::vector<char> stream( bytes.begin(), bytes.end() );
    std::vector<std::string> pages = OrcadParsePageOrder( stream );

    BOOST_REQUIRE_EQUAL( pages.size(), 3u );
    BOOST_CHECK_EQUAL( pages[0], "Street Fighter" );
    BOOST_CHECK_EQUAL( pages[1], "I2C-USB" );
    BOOST_CHECK_EQUAL( pages[2], "Block Diagram" );
}


BOOST_AUTO_TEST_CASE( V2SchematicStreamReversesStoredPageOrder )
{
    std::vector<uint8_t> bytes = { ORCAD_ST_PAGE, 0, 0 };
    appendLzt( bytes, "Low Power" );
    appendLe32( bytes, 0 );
    appendLe16( bytes, 3 );
    appendLzt( bytes, "Pre Regulator 100V" );
    appendLzt( bytes, "ZVS Flyback" );
    appendLzt( bytes, "Pre Regulator 12V" );

    std::vector<char>        stream( bytes.begin(), bytes.end() );
    std::vector<std::string> pages = OrcadParsePageOrderV2( stream, {} );

    BOOST_REQUIRE_EQUAL( pages.size(), 3u );
    BOOST_CHECK_EQUAL( pages[0], "Pre Regulator 12V" );
    BOOST_CHECK_EQUAL( pages[1], "ZVS Flyback" );
    BOOST_CHECK_EQUAL( pages[2], "Pre Regulator 100V" );
}


BOOST_AUTO_TEST_CASE( V2OccurrenceReferencesUseTargetObjectIds )
{
    auto appendPrefix = []( std::vector<uint8_t>& aBytes, uint8_t aType )
    {
        aBytes.push_back( aType );
        appendLe16( aBytes, 0 );
    };
    auto appendEmptyScope = []( std::vector<uint8_t>& aBytes )
    {
        appendLe16( aBytes, 0 );
        appendLe16( aBytes, 0 );
        appendLe16( aBytes, 0 );
    };

    std::vector<uint8_t> bytes;
    appendLzt( bytes, "SCHEMATIC1" );
    bytes.insert( bytes.end(), 5, 0 );
    appendLe16( bytes, 0 );
    appendLe16( bytes, 0 );
    appendLe16( bytes, 0 );
    appendLe16( bytes, 2 );

    appendPrefix( bytes, 0x42 );
    appendLe32( bytes, 0x12345678 );
    appendLe32( bytes, 0x00001234 );
    appendLzt( bytes, "" );
    appendLzt( bytes, "C1" );
    appendLe16( bytes, 0xFFFF );
    appendLe16( bytes, 0 );
    appendEmptyScope( bytes );

    appendPrefix( bytes, 0x42 );
    appendLe32( bytes, 0x87654321 );
    appendLe32( bytes, 0x00004321 );
    appendLzt( bytes, "CHILD" );
    appendLzt( bytes, "" );
    appendLe16( bytes, 0xFFFF );
    appendLe16( bytes, 0 );
    appendLe16( bytes, 0 );
    appendLe16( bytes, 0 );
    appendLe16( bytes, 1 );
    appendPrefix( bytes, 0x42 );
    appendLe32( bytes, 0xABCDEF01 );
    appendLe32( bytes, 0x00005678 );
    appendLzt( bytes, "" );
    appendLzt( bytes, "R1" );
    appendLe16( bytes, 0xFFFF );
    appendLe16( bytes, 0 );
    appendEmptyScope( bytes );

    std::vector<char> stream( bytes.begin(), bytes.end() );
    ORCAD_OCC_SCOPE   root = OrcadReadOccurrenceTreeV2( stream, {} );

    BOOST_REQUIRE_EQUAL( root.partRefs.size(), 1u );
    BOOST_CHECK_EQUAL( root.partRefs.at( 0x00001234 ), "C1" );
    BOOST_REQUIRE_EQUAL( root.blocks.size(), 1u );
    BOOST_CHECK_EQUAL( root.blocks[0].targetDbId, 0x00004321 );
    BOOST_REQUIRE_EQUAL( root.blocks[0].scope.partRefs.size(), 1u );
    BOOST_CHECK_EQUAL( root.blocks[0].scope.partRefs.at( 0x00005678 ), "R1" );
}


BOOST_AUTO_TEST_CASE( V2OccurrencePropertiesSurviveBlankReference )
{
    auto appendPrefix = []( std::vector<uint8_t>& aBytes, uint8_t aType,
                            const std::vector<std::pair<uint16_t, uint16_t>>& aProperties )
    {
        aBytes.push_back( aType );
        appendLe16( aBytes, static_cast<uint16_t>( aProperties.size() ) );

        for( const auto& [name, value] : aProperties )
        {
            appendLe16( aBytes, name );
            appendLe16( aBytes, value );
        }
    };

    std::vector<uint8_t> bytes;
    appendLzt( bytes, "SCHEMATIC1" );
    bytes.insert( bytes.end(), 5, 0 );
    appendLe16( bytes, 0 );
    appendLe16( bytes, 0 );
    appendLe16( bytes, 0 );
    appendLe16( bytes, 1 );

    appendPrefix( bytes, 0x42, { { 0, 1 }, { 2, 3 }, { 4, 0xFFFF } } );
    appendLe32( bytes, 0x12345678 );
    appendLe32( bytes, 0x00001234 );
    appendLzt( bytes, "" );
    appendLzt( bytes, "" );
    appendLe16( bytes, 0xFFFF );
    appendLe16( bytes, 0 );
    appendLe16( bytes, 0 );
    appendLe16( bytes, 0 );
    appendLe16( bytes, 0 );

    std::vector<std::string> strings = { "Value", "434 123 050 816", "Manufacturer",
                                         "Wurth Electronics Inc", "Clear Me" };
    std::vector<char>        stream( bytes.begin(), bytes.end() );
    ORCAD_OCC_SCOPE          root = OrcadReadOccurrenceTreeV2( stream, strings );

    BOOST_CHECK( root.partRefs.empty() );
    BOOST_REQUIRE_EQUAL( root.partProps.size(), 1u );
    BOOST_CHECK_EQUAL( root.partProps.at( 0x00001234 ).at( "Value" ), "434 123 050 816" );
    BOOST_CHECK_EQUAL( root.partProps.at( 0x00001234 ).at( "Manufacturer" ), "Wurth Electronics Inc" );
    BOOST_CHECK_EQUAL( root.partProps.at( 0x00001234 ).at( "Clear Me" ), "" );
}


BOOST_AUTO_TEST_CASE( ModernOccurrencePropertiesSurviveBlankReference )
{
    std::vector<uint8_t> bytes;
    bytes.push_back( 0x42 );
    size_t rootLengthOffset = bytes.size();
    appendLe32( bytes, 0 );
    appendLe32( bytes, 0 );
    appendLzt( bytes, "SCHEMATIC1" );
    bytes.insert( bytes.end(), 7, 0 );
    appendLe16( bytes, 0 );
    appendLe16( bytes, 0 );
    appendLe16( bytes, 0 );
    appendLe32( bytes, 0 );
    appendLe16( bytes, 1 );

    size_t occurrenceStart = bytes.size();
    bytes.push_back( 0x42 );
    size_t occurrenceLengthOffset = bytes.size();
    appendLe32( bytes, 0 );
    appendLe32( bytes, 0 );
    bytes.push_back( 0x42 );
    appendLe16( bytes, 3 );
    appendLe32( bytes, 0 );
    appendLe32( bytes, 1 );
    appendLe32( bytes, 2 );
    appendLe32( bytes, 3 );
    appendLe32( bytes, 4 );
    appendLe32( bytes, 0xFFFFFFFF );
    bytes.insert( bytes.end(), { 0xFF, 0xE4, 0x5C, 0x39 } );
    appendLe32( bytes, 0 );
    appendLe32( bytes, 0x12345678 );
    appendLe32( bytes, 0x00001234 );
    bytes.push_back( 0x42 );
    appendLe32( bytes, 0 );
    appendLe32( bytes, 0 );
    appendLzt( bytes, "" );
    appendLzt( bytes, "" );
    appendLe32( bytes, 0xFFFFFFFF );
    appendLe16( bytes, 0 );
    appendLe16( bytes, 0 );
    appendLe16( bytes, 0 );
    appendLe32( bytes, 0 );
    appendLe16( bytes, 0 );

    writeLe32( bytes, occurrenceLengthOffset,
               static_cast<uint32_t>( bytes.size() - occurrenceStart - 9 ) );
    writeLe32( bytes, rootLengthOffset, static_cast<uint32_t>( bytes.size() - 9 ) );

    std::vector<std::string> strings = { "Value", "434 123 050 816", "Manufacturer",
                                         "Wurth Electronics Inc", "Clear Me" };
    std::vector<char>        stream( bytes.begin(), bytes.end() );
    ORCAD_OCC_SCOPE          root = OrcadReadOccurrenceTree( stream, strings, []( const wxString& ) {} );

    BOOST_CHECK( root.partRefs.empty() );
    BOOST_REQUIRE_EQUAL( root.partProps.size(), 1u );
    BOOST_CHECK_EQUAL( root.partProps.at( 0x00001234 ).at( "Value" ), "434 123 050 816" );
    BOOST_CHECK_EQUAL( root.partProps.at( 0x00001234 ).at( "Manufacturer" ), "Wurth Electronics Inc" );
    BOOST_CHECK_EQUAL( root.partProps.at( 0x00001234 ).at( "Clear Me" ), "" );
}


BOOST_AUTO_TEST_CASE( HierarchyLinksComeFromSequentialOccurrenceRecords )
{
    std::vector<uint8_t> bytes;
    bytes.push_back( 0x42 );
    size_t rootLengthOffset = bytes.size();
    appendLe32( bytes, 0 );
    appendLe32( bytes, 0 );
    appendLzt( bytes, "SCHEMATIC1" );
    bytes.insert( bytes.end(), 7, 0 );
    appendLe16( bytes, 0 );
    appendLe16( bytes, 0 );
    appendLe16( bytes, 0 );
    appendLe32( bytes, 0 );
    appendLe16( bytes, 1 );

    size_t occurrenceStart = bytes.size();
    bytes.push_back( 0x42 );
    size_t occurrenceLengthOffset = bytes.size();
    appendLe32( bytes, 0 );
    appendLe32( bytes, 0 );
    bytes.push_back( 0x42 );
    appendLe16( bytes, 0 );
    bytes.insert( bytes.end(), { 0xFF, 0xE4, 0x5C, 0x39 } );
    appendLe32( bytes, 0 );
    appendLe32( bytes, 0x12345678 );
    appendLe32( bytes, 0x0000BEEF );
    bytes.push_back( 0x42 );
    appendLe32( bytes, 0 );
    appendLe32( bytes, 0 );
    appendLzt( bytes, "PAG_2" );
    appendLzt( bytes, "" );
    appendLe32( bytes, 0xFFFFFFFF );
    appendLe16( bytes, 0 );
    appendLe16( bytes, 0 );
    appendLe16( bytes, 0 );
    appendLe32( bytes, 0 );
    appendLe16( bytes, 0 );

    writeLe32( bytes, occurrenceLengthOffset, static_cast<uint32_t>( bytes.size() - occurrenceStart - 9 ) );
    writeLe32( bytes, rootLengthOffset, static_cast<uint32_t>( bytes.size() - 9 ) );

    std::vector<char> stream( bytes.begin(), bytes.end() );
    ORCAD_OCC_SCOPE   root = OrcadReadOccurrenceTree( stream, {}, []( const wxString& ) {} );

    BOOST_REQUIRE_EQUAL( root.blocks.size(), 1u );
    BOOST_CHECK_EQUAL( root.blocks[0].targetDbId, 0x0000BEEFu );
    BOOST_CHECK_EQUAL( root.blocks[0].childFolder, "PAG_2" );
}


BOOST_AUTO_TEST_CASE( V2SymbolStreamRetainsDefinitionProperties )
{
    std::vector<uint8_t> bytes;
    bytes.push_back( ORCAD_ST_TITLEBLOCK_SYMBOL );
    appendLe16( bytes, 1 );
    appendLe16( bytes, 0 );
    appendLe16( bytes, 1 );
    appendLzt( bytes, "TITLEBLK" );
    appendLzt( bytes, "" );
    appendLe32( bytes, 0 );
    appendLe16( bytes, 0 );
    appendLe16( bytes, 0 );
    appendLe16( bytes, 0 );
    appendLe16( bytes, 100 );
    appendLe16( bytes, 50 );
    appendLe16( bytes, 0 );
    appendLe16( bytes, 0 );

    std::map<std::string, ORCAD_SYMBOL_DEF> symbols;
    OrcadParseOlbSymbolStreamV2( std::vector<char>( bytes.begin(), bytes.end() ), { "SIZE", "N/A" }, symbols );

    BOOST_REQUIRE_EQUAL( symbols.size(), 1u );
    BOOST_CHECK_EQUAL( symbols.at( "TITLEBLK" ).props.at( "SIZE" ), "N/A" );
}


BOOST_AUTO_TEST_CASE( ViewsDirectoryLimitsImportedSchematicFolders )
{
    std::vector<uint8_t> bytes;
    appendLe32( bytes, 0x5351BBF2 );
    appendLe16( bytes, 2 );

    for( const std::string& name : { "DC1987A", "SCHEMATIC1" } )
    {
        appendLzt( bytes, name );
        appendLe16( bytes, 9 );
        bytes.insert( bytes.end(), 20, 0 );
    }

    std::vector<char>        stream( bytes.begin(), bytes.end() );
    std::vector<std::string> folders = OrcadParseSchematicFolderOrder( stream );

    BOOST_REQUIRE_EQUAL( folders.size(), 2u );
    BOOST_CHECK_EQUAL( folders[0], "DC1987A" );
    BOOST_CHECK_EQUAL( folders[1], "SCHEMATIC1" );
}


BOOST_AUTO_TEST_CASE( PrimitiveStrokeAndFillStyles )
{
    std::vector<uint8_t> bytes = { ORCAD_PRIM_RECT, ORCAD_PRIM_RECT };
    appendLe32( bytes, 40 );
    appendLe32( bytes, 0 );
    appendLe32( bytes, 10 );
    appendLe32( bytes, 20 );
    appendLe32( bytes, 30 );
    appendLe32( bytes, 40 );
    appendLe32( bytes, 2 );
    appendLe32( bytes, 3 );
    appendLe32( bytes, 2 );
    appendLe32( bytes, 5 );

    ORCAD_STREAM                   stream( bytes.data(), bytes.size() );
    std::optional<ORCAD_PRIMITIVE> primitive = OrcadReadPrimitive( stream );

    BOOST_REQUIRE( primitive );
    BOOST_CHECK_EQUAL( primitive->lineStyle, 2 );
    BOOST_CHECK_EQUAL( primitive->lineWidth, 3 );
    BOOST_CHECK_EQUAL( primitive->fillStyle, 2 );
    BOOST_CHECK_EQUAL( primitive->hatchStyle, 5 );
}


BOOST_AUTO_TEST_CASE( DesignTemplatePinFonts )
{
    std::vector<uint8_t> bytes( 32, 0 );
    const std::string    introduction = "OrCAD Windows Design";
    std::copy( introduction.begin(), introduction.end(), bytes.begin() );
    appendLe16( bytes, 3 );
    appendLe16( bytes, 2 );
    bytes.resize( bytes.size() + 12, 0 );
    appendLe16( bytes, 3 );

    for( int height : { -9, -12 } )
    {
        size_t offset = bytes.size();
        bytes.resize( offset + 60, 0 );
        writeLe32( bytes, offset, static_cast<uint32_t>( height ) );
        writeLe32( bytes, offset + 8, height == -9 ? 900 : 0 );
        writeLe32( bytes, offset + 12, height == -9 ? 0 : 1800 );
        std::copy_n( "Arial", 5, bytes.begin() + offset + 28 );
    }

    appendLe16( bytes, 24 );

    for( int i = 0; i < 24; ++i )
        appendLe16( bytes, i == 10 ? 1 : i == 11 ? 2 : 0 );

    bytes.resize( bytes.size() + 8, 0 );

    for( int i = 0; i < 8; ++i )
        appendLzt( bytes, "" );

    bytes.resize( bytes.size() + 156, 0 );
    appendLe32( bytes, 0 );
    appendLe16( bytes, 0 );

    writeLe32( bytes, 36, 0x53A88A14 );
    writeLe32( bytes, 40, 0x56A2631C );

    ORCAD_LIBRARY_INFO library = OrcadParseLibrary( std::vector<char>( bytes.begin(), bytes.end() ) );
    BOOST_REQUIRE_EQUAL( library.templateFonts.size(), 24u );
    BOOST_CHECK_EQUAL( library.templateFonts[10], 1 );
    BOOST_CHECK_EQUAL( library.templateFonts[11], 2 );
    BOOST_CHECK_EQUAL( library.pinNameFont, 10 );
    BOOST_CHECK_EQUAL( library.pinNumberFont, 11 );
    BOOST_REQUIRE_EQUAL( library.fonts.size(), 2u );
    BOOST_CHECK_EQUAL( library.fonts[0].escapement, 900 );
    BOOST_CHECK_EQUAL( library.fonts[0].orientation, 0 );
    BOOST_CHECK_EQUAL( library.fonts[1].escapement, 0 );
    BOOST_CHECK_EQUAL( library.fonts[1].orientation, 1800 );
    BOOST_CHECK_EQUAL( library.createTimestamp, 0x53A88A14 );
    BOOST_CHECK_EQUAL( library.modifyTimestamp, 0x56A2631C );
}


BOOST_AUTO_TEST_CASE( PageSettingsRetainPrintableFrameConfiguration )
{
    std::vector<uint8_t> bytes( 156, 0 );
    writeLe32( bytes, 24, 9700 );
    writeLe32( bytes, 28, 7200 );
    writeLe32( bytes, 32, 100 );
    writeLe16( bytes, 38, 5 );
    writeLe16( bytes, 40, 4 );
    writeLe32( bytes, 44, 100 );
    writeLe32( bytes, 48, 100 );
    writeLe32( bytes, 112, 1 );

    for( size_t offset = 128; offset < 156; offset += 4 )
        writeLe32( bytes, offset, 1 );

    ORCAD_STREAM stream( bytes.data(), bytes.size() );
    ORCAD_PAGE_SETTINGS settings = OrcadParsePageSettings( stream );
    BOOST_CHECK_EQUAL( settings.horizontalCount, 5 );
    BOOST_CHECK_EQUAL( settings.verticalCount, 4 );
    BOOST_CHECK_EQUAL( settings.horizontalWidth, 100 );
    BOOST_CHECK_EQUAL( settings.verticalWidth, 100 );
    BOOST_CHECK( !settings.horizontalChar );
    BOOST_CHECK( !settings.horizontalAscending );
    BOOST_CHECK( settings.verticalChar );
    BOOST_CHECK( !settings.verticalAscending );
    BOOST_CHECK( !settings.isMetric );
    BOOST_CHECK( settings.borderPrinted );
    BOOST_CHECK( settings.gridRefPrinted );
    BOOST_CHECK( settings.titleblockPrinted );
    BOOST_CHECK( settings.ansiGridRefs );
}


BOOST_AUTO_TEST_CASE( PrimitivePolygonStylesBeforePoints )
{
    std::vector<uint8_t> bytes = { ORCAD_PRIM_POLYGON, ORCAD_PRIM_POLYGON };
    appendLe32( bytes, 34 );
    appendLe32( bytes, 0 );
    appendLe32( bytes, 1 );
    appendLe32( bytes, 2 );
    appendLe32( bytes, 2 );
    appendLe32( bytes, 4 );
    appendLe16( bytes, 2 );
    appendLe16( bytes, 20 );
    appendLe16( bytes, 10 );
    appendLe16( bytes, 40 );
    appendLe16( bytes, 30 );

    ORCAD_STREAM                   stream( bytes.data(), bytes.size() );
    std::optional<ORCAD_PRIMITIVE> primitive = OrcadReadPrimitive( stream );

    BOOST_REQUIRE( primitive );
    BOOST_CHECK_EQUAL( primitive->lineStyle, 1 );
    BOOST_CHECK_EQUAL( primitive->lineWidth, 2 );
    BOOST_CHECK_EQUAL( primitive->fillStyle, 2 );
    BOOST_CHECK_EQUAL( primitive->hatchStyle, 4 );
    BOOST_REQUIRE_EQUAL( primitive->points.size(), 2u );
    BOOST_CHECK( ( primitive->points[0] == ORCAD_POINT{ 10, 20 } ) );
    BOOST_CHECK( ( primitive->points[1] == ORCAD_POINT{ 30, 40 } ) );
}


BOOST_AUTO_TEST_CASE( PrimitiveSymbolVectorContents )
{
    std::vector<uint8_t> bytes = { ORCAD_PRIM_SYMBOL_VECTOR, ORCAD_PRIM_SYMBOL_VECTOR };
    appendLe32( bytes, 63 );
    appendLe32( bytes, 0 );
    bytes.push_back( ORCAD_PRIM_SYMBOL_VECTOR );
    appendLe16( bytes, 0 );
    bytes.insert( bytes.end(), std::begin( ORCAD_STREAM::PREAMBLE ), std::end( ORCAD_STREAM::PREAMBLE ) );
    appendLe32( bytes, 0 );
    appendLe16( bytes, 12 );
    appendLe16( bytes, 26 );
    appendLe16( bytes, 1 );
    bytes.insert( bytes.end(), { ORCAD_PRIM_POLYLINE, 0, ORCAD_PRIM_POLYLINE } );
    appendLe32( bytes, 30 );
    appendLe32( bytes, 0 );
    appendLe32( bytes, 0 );
    appendLe32( bytes, 0 );
    appendLe16( bytes, 3 );
    appendLe16( bytes, 8 );
    appendLe16( bytes, 4 );
    appendLe16( bytes, 0 );
    appendLe16( bytes, 4 );
    appendLe16( bytes, 0 );
    appendLe16( bytes, 16 );
    appendLe16( bytes, 10 );
    bytes.insert( bytes.end(), { 'H', 'y', 's', 't', 'e', 'r', 'e', 's', 'i', 's', 0 } );

    ORCAD_STREAM                   stream( bytes.data(), bytes.size() );
    std::optional<ORCAD_PRIMITIVE> primitive = OrcadReadPrimitive( stream );

    BOOST_REQUIRE( primitive );
    BOOST_CHECK( primitive->kind == ORCAD_PRIM_KIND::GROUP );
    BOOST_CHECK_EQUAL( primitive->x1, 12 );
    BOOST_CHECK_EQUAL( primitive->y1, 26 );
    BOOST_REQUIRE_EQUAL( primitive->children.size(), 1u );
    BOOST_CHECK( primitive->children[0].kind == ORCAD_PRIM_KIND::POLYLINE );
    BOOST_REQUIRE_EQUAL( primitive->children[0].points.size(), 3u );
    BOOST_CHECK( ( primitive->children[0].points[2] == ORCAD_POINT{ 16, 0 } ) );
}


BOOST_AUTO_TEST_CASE( OleMetafilePreviewExtraction )
{
    std::vector<uint8_t> presentation( 4096, 0 );
    writeLe32( presentation, 4, 14 );
    presentation[40] = 1;
    presentation[41] = 0;
    presentation[42] = 9;
    presentation[43] = 0;

    std::vector<uint16_t> name = { 2, 'O', 'l', 'e', 'P', 'r', 'e', 's', '0', '0', '0', 0 };
    std::vector<uint8_t>  cfb = makeOlePreviewCfb( name, presentation );
    OLE_IMAGE_PAYLOAD     preview = ExtractOleImageFromPayload( makeOlePayload( cfb, cfb.size() ) );

    BOOST_CHECK( preview.type == OLE_IMAGE_TYPE::WMF );
    BOOST_REQUIRE_EQUAL( preview.data.size(), presentation.size() - 40 );
    BOOST_CHECK_EQUAL( preview.data[0], 1 );
    BOOST_CHECK_EQUAL( preview.data[2], 9 );
}


BOOST_AUTO_TEST_CASE( TruncatedEmbeddedOleContainerIsPadded )
{
    std::vector<uint8_t> presentation( 4096, 0 );
    writeLe32( presentation, 4, 14 );
    presentation[40] = 1;
    presentation[42] = 9;

    std::vector<uint16_t> name = { 2, 'O', 'l', 'e', 'P', 'r', 'e', 's', '0', '0', '0', 0 };
    std::vector<uint8_t>  cfb = makeOlePreviewCfb( name, presentation );
    uint32_t              declared = static_cast<uint32_t>( cfb.size() );
    cfb.resize( cfb.size() - 200 );

    OLE_IMAGE_PAYLOAD preview = ExtractOleImageFromPayload( makeOlePayload( cfb, declared ) );

    BOOST_CHECK( preview.type == OLE_IMAGE_TYPE::WMF );
    BOOST_REQUIRE_EQUAL( preview.data.size(), presentation.size() - 40 );
    BOOST_CHECK_EQUAL( preview.data[0], 1 );
    BOOST_CHECK_EQUAL( preview.data[2], 9 );
}


BOOST_AUTO_TEST_CASE( OleNativeBitmapExtraction )
{
    // The stream states its own payload length, which real files set to the stream size less
    // the length word itself.
    std::vector<uint8_t> native( 4096, 0 );
    writeLe32( native, 0, static_cast<uint32_t>( native.size() - 4 ) );
    native[4] = 'B';
    native[5] = 'M';
    writeLe32( native, 6, 58 );

    std::vector<uint16_t> name = { 1, 'O', 'l', 'e', '1', '0', 'N', 'a', 't', 'i', 'v', 'e', 0 };
    std::vector<uint8_t>  cfb = makeOlePreviewCfb( name, native );
    OLE_IMAGE_PAYLOAD     preview = ExtractOleImageFromPayload( makeOlePayload( cfb, cfb.size() ) );

    BOOST_CHECK( preview.type == OLE_IMAGE_TYPE::BMP );
    BOOST_REQUIRE_EQUAL( preview.data.size(), native.size() - 4 );
    BOOST_CHECK_EQUAL( preview.data[0], 'B' );
    BOOST_CHECK_EQUAL( preview.data[1], 'M' );

    for( const std::vector<uint8_t>& signature :
         { std::vector<uint8_t>{ 0x89, 'P', 'N', 'G' }, std::vector<uint8_t>{ 0xFF, 0xD8, 0xFF } } )
    {
        std::copy( signature.begin(), signature.end(), native.begin() + 4 );
        cfb = makeOlePreviewCfb( name, native );
        preview = ExtractOleImageFromPayload( makeOlePayload( cfb, cfb.size() ) );

        BOOST_CHECK( preview.type == OLE_IMAGE_TYPE::BMP );
        BOOST_CHECK_EQUAL_COLLECTIONS( preview.data.begin(), preview.data.end(), native.begin() + 4, native.end() );
    }
}


BOOST_AUTO_TEST_CASE( SharedOleContentsBitmapExtraction )
{
    std::vector<uint8_t> contents( 4096, 0 );
    contents[0] = 'B';
    contents[1] = 'M';
    writeLe32( contents, 2, 58 );

    std::vector<uint16_t> name = { 'C', 'O', 'N', 'T', 'E', 'N', 'T', 'S', 0 };
    OLE_IMAGE_PAYLOAD     image = ExtractOleImage( makeOlePreviewCfb( name, contents ) );

    BOOST_CHECK( image.type == OLE_IMAGE_TYPE::BMP );
    BOOST_CHECK_EQUAL( image.streamName, "CONTENTS" );
    BOOST_REQUIRE_EQUAL( image.data.size(), contents.size() );
    BOOST_CHECK_EQUAL( image.data[0], 'B' );
    BOOST_CHECK_EQUAL( image.data[1], 'M' );
}


// A directory entry that refers to itself must terminate without recursion.
BOOST_AUTO_TEST_CASE( OleDirectoryCycleTerminates )
{
    constexpr size_t SECTOR_SIZE = 512;
    constexpr size_t RIGHT_SIBLING_OFFSET = 72;

    std::vector<uint16_t> name = { 'C', 'O', 'N', 'T', 'E', 'N', 'T', 'S', 0 };
    std::vector<uint8_t>  contents( 64, 0 );

    contents[0] = 'B';
    contents[1] = 'M';

    std::vector<uint8_t> cfb = makeOlePreviewCfb( name, contents );

    // The stream entry is directory index 1, immediately after the root
    const size_t streamEntry = 2 * SECTOR_SIZE + 128;

    writeLe32( cfb, streamEntry + RIGHT_SIBLING_OFFSET, 1 );

    OLE_IMAGE_PAYLOAD image = ExtractOleImage( cfb.data(), cfb.size() );

    // The walk must terminate AND still reach the stream, so a visited set that pruned too much
    // would fail here rather than pass quietly
    BOOST_CHECK( image.type == OLE_IMAGE_TYPE::BMP );
}


// A header size alone must not identify a DIB and hide a valid OlePres000 preview.
BOOST_AUTO_TEST_CASE( OleContentsRejectsBogusDib )
{
    std::vector<uint16_t> name = { 'C', 'O', 'N', 'T', 'E', 'N', 'T', 'S', 0 };
    std::vector<uint8_t>  contents( 64, 0 );

    writeLe32( contents, 0, 40 );    // biSize, the weak signature
    writeLe32( contents, 4, 16 );    // biWidth
    writeLe32( contents, 8, 16 );    // biHeight
    writeLe16( contents, 12, 7 );    // biPlanes, only 1 is ever valid
    writeLe16( contents, 14, 24 );   // biBitCount

    OLE_IMAGE_PAYLOAD image = ExtractOleImage( makeOlePreviewCfb( name, contents ) );

    BOOST_CHECK( image.type == OLE_IMAGE_TYPE::NONE );
}


BOOST_AUTO_TEST_CASE( CiImageFullRasterExtraction )
{
    // The payload opens with the preview DIB, and the marker follows it. Capture writes a
    // 55x15 monochrome placeholder: 40-byte header, two palette entries, 8-byte rows.
    std::vector<uint8_t> payload( 40 + 2 * 4 + 8 * 15, 0 );
    writeLe32( payload, 0, 40 );  // biSize
    writeLe32( payload, 4, 55 );  // biWidth
    writeLe32( payload, 8, 15 );  // biHeight
    writeLe16( payload, 12, 1 );  // biPlanes
    writeLe16( payload, 14, 1 );  // biBitCount

    const std::string    marker = "~~CI_IMAGE~~";
    payload.insert( payload.end(), marker.begin(), marker.end() );
    payload.insert( payload.end(), { 0, 2, '1', '1' } );

    const std::vector<uint8_t> png = { 0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A, 1, 2, 3 };
    payload.insert( payload.end(), png.begin(), png.end() );

    BOOST_CHECK( OleExtractCiImage( payload ) == png );

    payload.resize( 40 );
    BOOST_CHECK( OleExtractCiImage( payload ).empty() );

    writeLe32( payload, 4, 0x7FFFFFFF );
    writeLe32( payload, 8, 0x80000000 );
    writeLe16( payload, 14, 32 );
    BOOST_CHECK( OleExtractCiImage( payload ).empty() );
}


BOOST_AUTO_TEST_CASE( CiImageScannerDoesNotConsumeOleNativeCache )
{
    std::vector<uint8_t> payload;
    constexpr char       marker[] = "~~CI_IMAGE~~";
    payload.insert( payload.end(), marker, marker + sizeof( marker ) - 1 );
    payload.insert( payload.end(), { 0xD0, 0xCF, 0x11, 0xE0, 0xA1, 0xB1, 0x1A, 0xE1 } );
    payload.insert( payload.end(), { 0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A } );

    BOOST_CHECK( OleExtractCiImage( payload ).empty() );
}


BOOST_AUTO_TEST_CASE( EmbeddedImageFillsNonSquareSourceBox )
{
    BOOST_CHECK_EQUAL( OrcadStretchedImageSize( 718, 720, 830, 360 ), VECTOR2I( 1660, 720 ) );
    BOOST_CHECK_EQUAL( OrcadStretchedImageSize( 1702, 528, 1702, 528 ), VECTOR2I( 1702, 528 ) );
    BOOST_CHECK_EQUAL( OrcadStretchedImageSize( 800, 400, 300, 600 ), VECTOR2I( 800, 1600 ) );
}


BOOST_AUTO_TEST_CASE( WmfRenderUsesEmbeddedBoxAspectBeforeRasterization )
{
    BOOST_CHECK_EQUAL( OleWmfRenderSize( 718, 720, 4096, 720, 830.0 / 360.0 ), VECTOR2I( 1660, 720 ) );
    BOOST_CHECK_EQUAL( OleWmfRenderSize( 1702, 528, 2048, 2048, 0.0 ), VECTOR2I( 1702, 528 ) );
    BOOST_CHECK_EQUAL( OleWmfRenderSize( 1702, 528, 1000, 1000, 0.0 ), VECTOR2I( 1000, 310 ) );
}


BOOST_AUTO_TEST_CASE( WmfGlyphIndexTextAndDestinationCopyRender )
{
    wxImage image;

    BOOST_REQUIRE( OleRenderWmf( makeGlyphIndexWmf(), 200, 200, image ) );

    const unsigned char* pixels = image.GetData();
    size_t               nonWhite = 0;

    for( size_t i = 0; i < static_cast<size_t>( image.GetWidth() ) * image.GetHeight(); ++i )
    {
        if( pixels[3 * i] != 255 || pixels[3 * i + 1] != 255 || pixels[3 * i + 2] != 255 )
            ++nonWhite;
    }

    BOOST_CHECK_GT( nonWhite, 0u );
}


BOOST_AUTO_TEST_CASE( WmfNegativeDestinationHeightFlipsDibWithoutClipping )
{
    wxImage image;

    BOOST_REQUIRE( OleRenderWmf( makeFlippedDibWmf(), 200, 200, image ) );
    BOOST_CHECK_LE( image.GetWidth(), 200 );
    BOOST_CHECK_LE( image.GetHeight(), 200 );
    BOOST_CHECK_GE( image.GetWidth(), 50 );
    BOOST_CHECK_GE( image.GetHeight(), 50 );

    const int left = image.GetWidth() / 4;
    const int top = image.GetHeight() / 4;
    const int bottom = image.GetHeight() * 3 / 4;
    const int topRed = image.GetRed( left, top );
    const int topBlue = image.GetBlue( left, top );
    const int bottomRed = image.GetRed( left, bottom );
    const int bottomBlue = image.GetBlue( left, bottom );

    BOOST_CHECK_GT( topRed, topBlue + 50 );
    BOOST_CHECK_GT( bottomBlue, bottomRed + 50 );
}


BOOST_AUTO_TEST_CASE( EmbeddedEmfChunksExcludeWmfFraming )
{
    std::vector<uint8_t> emf = OleExtractEmbeddedEmf( makeEmbeddedEmfWmf() );

    BOOST_REQUIRE_EQUAL( emf.size(), 100u );
    BOOST_CHECK_EQUAL( emf[0], 1 );
    BOOST_CHECK_EQUAL( emf[40], 0x20 );
    BOOST_CHECK_EQUAL( emf[41], 'E' );
    BOOST_CHECK_EQUAL( emf[42], 'M' );
    BOOST_CHECK_EQUAL( emf[43], 'F' );
}


BOOST_AUTO_TEST_CASE( EmbeddedEmfRendersPathsAndRotatedTextDeterministically )
{
    wxImage              first;
    wxImage              second;
    std::vector<uint8_t> emf = makeRenderableEmf();

    BOOST_REQUIRE( OleRenderEmf( emf, 200, 200, first ) );
    BOOST_REQUIRE( OleRenderEmf( emf, 200, 200, second ) );
    BOOST_REQUIRE_EQUAL( first.GetWidth(), 100 );
    BOOST_REQUIRE_EQUAL( first.GetHeight(), 100 );
    BOOST_REQUIRE_EQUAL( second.GetWidth(), first.GetWidth() );
    BOOST_REQUIRE_EQUAL( second.GetHeight(), first.GetHeight() );

    size_t nonWhite = 0;
    size_t blue = 0;
    size_t byteCount = static_cast<size_t>( first.GetWidth() ) * first.GetHeight() * 3;

    for( size_t i = 0; i < byteCount; i += 3 )
    {
        const unsigned char* pixel = first.GetData() + i;

        if( pixel[0] != 255 || pixel[1] != 255 || pixel[2] != 255 )
            ++nonWhite;

        if( pixel[2] > pixel[0] + 64 && pixel[2] > pixel[1] + 64 )
            ++blue;
    }

    BOOST_CHECK_GT( nonWhite, 50u );
    BOOST_CHECK_GT( blue, 10u );
    BOOST_CHECK_EQUAL_COLLECTIONS( first.GetData(), first.GetData() + byteCount, second.GetData(),
                                   second.GetData() + byteCount );

    for( int y = 30; y < 36; ++y )
    {
        for( int x = 70; x < 84; ++x )
        {
            BOOST_CHECK_MESSAGE( first.GetBlue( x, y ) < 160 || first.GetRed( x, y ) > 160,
                                 "glyph-index trademark descends below its baseline at " << x << ',' << y );
        }
    }

    size_t wideGlyphPixels = 0;

    for( int y = 15; y < 26; ++y )
    {
        for( int x = 77; x < 84; ++x )
        {
            if( first.GetBlue( x, y ) > first.GetRed( x, y ) + 64 )
                ++wideGlyphPixels;
        }
    }

    BOOST_CHECK_GT( wideGlyphPixels, 2u );

    size_t substitutedSpacePixels = 0;

    for( int y = 70; y < 100; ++y )
    {
        for( int x = 0; x < 25; ++x )
        {
            if( first.GetBlue( x, y ) > first.GetRed( x, y ) + 64 )
                ++substitutedSpacePixels;
        }
    }

    BOOST_CHECK_EQUAL( substitutedSpacePixels, 0u );
}


BOOST_AUTO_TEST_CASE( MalformedEmfRecordsAreRejected )
{
    std::vector<uint8_t> emf = makeRenderableEmf();
    emf.resize( 136 );
    writeLe32( emf, 48, emf.size() );
    writeLe32( emf, 52, 3 );
    writeLe32( emf, 108, 35 );
    writeLe32( emf, 112, 8 );
    writeLe32( emf, 116, 14 );
    writeLe32( emf, 120, 20 );
    std::fill( emf.begin() + 124, emf.end(), 0 );
    wxImage image;

    BOOST_CHECK( !OleRenderEmf( emf, 200, 200, image ) );

    writeLe32( emf, 108, 27 );
    BOOST_CHECK( !OleRenderEmf( emf, 200, 200, image ) );

    writeLe32( emf, 112, 256 );
    BOOST_CHECK( !OleRenderEmf( emf, 200, 200, image ) );
}


BOOST_AUTO_TEST_CASE( CalibriTextUsesMetricCompatibleOutlines )
{
    wxImage image;

    BOOST_REQUIRE( OleRenderEmf( makeCalibriMetricEmf(), 100, 50, image ) );

    size_t nonWhite = 0;

    for( int y = 0; y < image.GetHeight(); ++y )
    {
        for( int x = 0; x < image.GetWidth(); ++x )
        {
            if( image.GetRed( x, y ) != 255 || image.GetGreen( x, y ) != 255 || image.GetBlue( x, y ) != 255 )
                ++nonWhite;
        }
    }

    BOOST_CHECK_GE( nonWhite, 110u );
}


BOOST_AUTO_TEST_CASE( EmfImageTagInTextRendersAsText )
{
    wxImage image;

    BOOST_REQUIRE( OleRenderEmf( makeCalibriMetricEmf( u"<image " ), 100, 50, image ) );
    BOOST_CHECK( image.IsOk() );
}


BOOST_AUTO_TEST_CASE( MetafilePreviewPrefersEmbeddedEmfAndFallsBackToWmf )
{
    wxImage              directEmfImage;
    wxImage              emfImage;
    wxImage              wmfImage;
    std::vector<uint8_t> emf = makeRenderableEmf();

    BOOST_REQUIRE( OleRenderEmf( emf, 200, 200, directEmfImage ) );
    BOOST_REQUIRE( OleRenderMetafilePreview( makeEmbeddedEmfWmf( emf ), 200, 200, emfImage ) );
    BOOST_REQUIRE( OleRenderMetafilePreview( makeGlyphIndexWmf(), 200, 200, wmfImage ) );
    BOOST_CHECK_EQUAL( emfImage.GetWidth(), 100 );
    BOOST_CHECK_EQUAL( emfImage.GetHeight(), 100 );
    size_t byteCount = static_cast<size_t>( emfImage.GetWidth() ) * emfImage.GetHeight() * 3;
    BOOST_CHECK_EQUAL_COLLECTIONS( emfImage.GetData(), emfImage.GetData() + byteCount, directEmfImage.GetData(),
                                   directEmfImage.GetData() + byteCount );
    BOOST_CHECK( wmfImage.IsOk() );
}


BOOST_AUTO_TEST_CASE( ExternalEmbeddedEmfRendersWhenProvided )
{
    const char* emfPath = std::getenv( "KICAD_ORCAD_EMF" );

    if( !emfPath || !*emfPath )
    {
        BOOST_TEST_MESSAGE( "KICAD_ORCAD_EMF not set; skipping external embedded-EMF check." );
        return;
    }

    std::ifstream input( emfPath, std::ios::binary );
    BOOST_REQUIRE( input );
    std::vector<uint8_t> emf( std::istreambuf_iterator<char>( input ), {} );
    wxImage              image;

    BOOST_REQUIRE( OleRenderEmf( emf, 2048, 2048, image ) );
    BOOST_CHECK_GT( image.GetWidth(), 0 );
    BOOST_CHECK_GT( image.GetHeight(), 0 );

    if( const char* outputPath = std::getenv( "KICAD_ORCAD_EMF_OUTPUT" ) )
        BOOST_REQUIRE( image.SaveFile( wxString::FromUTF8( outputPath ), wxBITMAP_TYPE_PNG ) );
}


BOOST_AUTO_TEST_CASE( ExternalWmfEmbeddedEmfRendersWhenProvided )
{
    const char* wmfPath = std::getenv( "KICAD_ORCAD_WMF" );

    if( !wmfPath || !*wmfPath )
    {
        BOOST_TEST_MESSAGE( "KICAD_ORCAD_WMF not set; skipping external WMF/EMF check." );
        return;
    }

    std::ifstream input( wmfPath, std::ios::binary );
    BOOST_REQUIRE( input );
    std::vector<uint8_t> wmf( std::istreambuf_iterator<char>( input ), {} );
    std::vector<uint8_t> emf = OleExtractEmbeddedEmf( wmf );
    wxImage              image;
    wxImage              directImage;

    BOOST_REQUIRE( !emf.empty() );
    BOOST_REQUIRE( OleRenderEmf( emf, 2200, 1360, directImage, 2200.0 / 1360.0 ) );
    BOOST_REQUIRE( OleRenderMetafilePreview( wmf, 2200, 1360, image, 2200.0 / 1360.0 ) );
    BOOST_REQUIRE_EQUAL( image.GetWidth(), directImage.GetWidth() );
    BOOST_REQUIRE_EQUAL( image.GetHeight(), directImage.GetHeight() );
    size_t byteCount = static_cast<size_t>( image.GetWidth() ) * image.GetHeight() * 3;
    BOOST_CHECK_EQUAL_COLLECTIONS( image.GetData(), image.GetData() + byteCount, directImage.GetData(),
                                   directImage.GetData() + byteCount );

    if( const char* outputPath = std::getenv( "KICAD_ORCAD_WMF_OUTPUT" ) )
        BOOST_REQUIRE( image.SaveFile( wxString::FromUTF8( outputPath ), wxBITMAP_TYPE_PNG ) );
}


BOOST_AUTO_TEST_CASE( OlePreviewWithMultiplePresentationStreams )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
    {
        BOOST_TEST_MESSAGE( "KICAD_ORCAD_CORPUS not set; skipping multi-preview OLE check." );
        return;
    }

    std::filesystem::path dsn;

    for( const std::filesystem::directory_entry& entry : std::filesystem::recursive_directory_iterator( corpusEnv ) )
    {
        if( entry.is_regular_file() && entry.path().filename() == "reServer industrial J401 Carrier Board v11.DSN" )
        {
            dsn = entry.path();
            break;
        }
    }

    if( dsn.empty() )
    {
        BOOST_TEST_MESSAGE( "reServer industrial J401 design not present; skipping multi-preview OLE check." );
        return;
    }

    std::ifstream                               stream( dsn, std::ios::binary );
    std::vector<uint8_t>                        bytes( std::istreambuf_iterator<char>( stream ), {} );
    const std::array<std::array<uint8_t, 8>, 2> markers = {
        std::array<uint8_t, 8>{ 0x6A, 0x31, 0x04, 0x00, 0x00, 0x01, 0x00, 0x00 },
        std::array<uint8_t, 8>{ 0x16, 0x94, 0x05, 0x00, 0x00, 0x01, 0x00, 0x00 }
    };

    for( size_t markerIndex = 0; markerIndex < markers.size(); ++markerIndex )
    {
        const auto& marker = markers[markerIndex];
        auto markerPos = std::search( bytes.begin(), bytes.end(), marker.begin(), marker.end() );
        BOOST_REQUIRE( markerPos != bytes.end() );

        size_t offset = static_cast<size_t>( std::distance( bytes.begin(), markerPos ) );
        size_t size = static_cast<size_t>( bytes[offset] ) | ( static_cast<size_t>( bytes[offset + 1] ) << 8 )
                      | ( static_cast<size_t>( bytes[offset + 2] ) << 16 )
                      | ( static_cast<size_t>( bytes[offset + 3] ) << 24 );
        BOOST_REQUIRE_LE( offset + size + 4, bytes.size() );

        OLE_IMAGE_PAYLOAD preview =
                ExtractOleImageFromPayload( { bytes.begin() + offset, bytes.begin() + offset + size + 4 } );
        BOOST_CHECK( preview.type == OLE_IMAGE_TYPE::WMF );
        BOOST_CHECK_GT( preview.data.size(), 200000u );

        wxImage image;
        BOOST_REQUIRE_MESSAGE( OleRenderWmf( preview.data, 2048, 2048, image ),
                               "marker " << markerIndex << ", preview bytes " << preview.data.size() << ", "
                                         << OleDescribeImagePayload( preview.data ) );
        BOOST_CHECK_GT( image.GetWidth(), 1000 );
        BOOST_CHECK_GT( image.GetHeight(), 500 );
    }
}


BOOST_AUTO_TEST_CASE( LegacyPageNetGroups )
{
    std::vector<uint8_t> bytes = { ORCAD_ST_PAGE, 0, 0 };
    appendLzt( bytes, "PAGE" );
    appendLzt( bytes, "C" );
    bytes.resize( bytes.size() + 156 );
    appendLe16( bytes, 0 );
    appendLe16( bytes, 0 );
    appendLe16( bytes, 1 );
    appendLe32( bytes, 0x12345678 );
    appendLzt( bytes, "BUS[1:0]" );
    appendLe16( bytes, 2 );
    appendLe32( bytes, 0x11111111 );
    appendLe32( bytes, 0x22222222 );

    for( int i = 0; i < 10; ++i )
        appendLe16( bytes, 0 );

    std::vector<char> data( bytes.begin(), bytes.end() );
    ORCAD_RAW_PAGE    page = OrcadParsePageV2( data, {},
                                               []( const wxString& )
                                               {
                                            } );

    BOOST_REQUIRE_EQUAL( page.netGroups.size(), 1u );
    BOOST_CHECK_EQUAL( page.netGroups[0].id, 0x12345678u );
    BOOST_CHECK_EQUAL( page.netGroups[0].name, "BUS[1:0]" );
    BOOST_REQUIRE_EQUAL( page.netGroups[0].members.size(), 2u );
    BOOST_CHECK_EQUAL( page.netGroups[0].members[1], 0x22222222u );
}


static std::string         terminalToken( const std::string& aRef, const std::string& aPin );
static std::pair<int, int> checkConnectivity( SCHEMATIC& aSchematic, const std::vector<std::set<std::string>>& aNets,
                                              std::vector<std::set<std::string>>* aInconsistent = nullptr );


static SCH_SHEET* convertRawDesign( ORCAD_DESIGN& aDesign, SCHEMATIC& aSchematic, REPORTER* aReporter = nullptr )
{
    SCH_SHEET*  rootSheet = new SCH_SHEET( &aSchematic );
    SCH_SCREEN* rootScreen = new SCH_SCREEN( &aSchematic );
    rootSheet->SetScreen( rootScreen );
    aSchematic.SetTopLevelSheets( { rootSheet } );
    aSchematic.CurrentSheet().clear();
    aSchematic.CurrentSheet().push_back( rootSheet );

    ORCAD_CONVERTER converter( aDesign, &aSchematic, aReporter );
    converter.Convert( rootSheet );
    return rootSheet;
}


BOOST_AUTO_TEST_CASE( SameNetInteriorWireCrossingGetsJunction )
{
    ORCAD_RAW_PAGE page;
    page.name = "CONNECTED CROSSING";

    ORCAD_WIRE horizontal;
    horizontal.id = 1;
    horizontal.x1 = 0;
    horizontal.y1 = 10;
    horizontal.x2 = 20;
    horizontal.y2 = 10;
    page.wires.push_back( horizontal );

    ORCAD_WIRE vertical = horizontal;
    vertical.x1 = 10;
    vertical.y1 = 0;
    vertical.x2 = 10;
    vertical.y2 = 20;
    page.wires.push_back( vertical );
    page.netmap.emplace( 1, "GND_SIGNAL" );

    ORCAD_DESIGN design;
    design.sourceId = "same-net-interior-crossing";
    design.pages.push_back( std::move( page ) );

    SETTINGS_MANAGER manager;
    manager.LoadProject( "" );
    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    schematic->SetProject( &manager.Prj() );
    SCH_SHEET* root = convertRawDesign( design, *schematic );
    size_t junctionCount = 0;

    for( SCH_ITEM* item : root->GetScreen()->Items().OfType( SCH_JUNCTION_T ) )
        ++junctionCount;

    BOOST_CHECK_EQUAL( junctionCount, 1u );
}


BOOST_AUTO_TEST_CASE( ExplicitPageDimensionsRemainAuthoritative )
{
    ORCAD_RAW_PAGE page;
    page.name = "OVERSIZED B";
    page.pageSize = "B";
    page.width = 20000;
    page.height = 12700;

    ORCAD_WIRE wire;
    wire.x1 = 100;
    wire.y1 = 100;
    wire.x2 = 2200;
    wire.y2 = 100;
    page.wires.push_back( wire );

    ORCAD_DESIGN design;
    design.sourceId = "inconsistent-named-page-size";
    design.pages.push_back( std::move( page ) );

    SETTINGS_MANAGER manager;
    manager.LoadProject( "" );
    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    schematic->SetProject( &manager.Prj() );
    SCH_SHEET* root = convertRawDesign( design, *schematic );
    const PAGE_INFO& paper = root->GetScreen()->GetPageSettings();

    BOOST_CHECK( paper.GetType() == PAGE_SIZE_TYPE::User );
    BOOST_CHECK_CLOSE( paper.GetWidthMils(), 20000.0, 0.001 );
    BOOST_CHECK_CLOSE( paper.GetHeightMils(), 12700.0, 0.001 );
    BOOST_CHECK_LT( paper.GetWidthIU( schIUScale.IU_PER_MILS ), OrcadDbuToIu( wire.x2, wire.y2 ).x );
}


BOOST_AUTO_TEST_CASE( VisiblePageOuterBorderIsImported )
{
    ORCAD_RAW_PAGE page;
    page.name = "OUTER BORDER";
    page.pageSize = "A";
    page.width = 9700;
    page.height = 7200;
    page.horizontalCount = 5;
    page.verticalCount = 4;
    page.horizontalWidth = 100;
    page.verticalWidth = 100;
    page.verticalChar = true;
    page.borderPrinted = true;
    page.gridRefPrinted = true;

    ORCAD_DESIGN design;
    design.sourceId = "outer-border";
    design.pages.push_back( std::move( page ) );

    SETTINGS_MANAGER manager;
    manager.LoadProject( "" );
    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    schematic->SetProject( &manager.Prj() );
    SCH_SHEET* root = convertRawDesign( design, *schematic );

    const SCH_SHAPE* border = nullptr;

    for( const SCH_ITEM* item : root->GetScreen()->Items().OfType( SCH_SHAPE_T ) )
    {
        const SCH_SHAPE* shape = static_cast<const SCH_SHAPE*>( item );

        if( shape->GetPolyPoints().size() == 5 && shape->GetPolyPoints()[1] == OrcadDbuToIu( 970, 0 ) )
            border = shape;
    }

    BOOST_REQUIRE( border );
    BOOST_CHECK( border->GetShape() == SHAPE_T::POLY );
    const std::vector<VECTOR2I> points = border->GetPolyPoints();
    BOOST_REQUIRE_EQUAL( points.size(), 5u );
    BOOST_CHECK( points[0] == OrcadDbuToIu( 0, 0 ) );
    BOOST_CHECK( points[1] == OrcadDbuToIu( 970, 0 ) );
    BOOST_CHECK( points[2] == OrcadDbuToIu( 970, 720 ) );
    BOOST_CHECK( points[3] == OrcadDbuToIu( 0, 720 ) );
    BOOST_CHECK( points[4] == points[0] );
    BOOST_CHECK( border->GetStroke().GetColor() == KIGFX::COLOR4D( 0.0, 0.0, 0.0, 1.0 ) );

    std::set<wxString> labels;

    for( const SCH_ITEM* item : root->GetScreen()->Items().OfType( SCH_TEXT_T ) )
        labels.insert( static_cast<const SCH_TEXT*>( item )->GetText() );

    for( const wxString& expected : { wxS( "5" ), wxS( "4" ), wxS( "3" ), wxS( "2" ), wxS( "1" ),
                                      wxS( "D" ), wxS( "C" ), wxS( "B" ), wxS( "A" ) } )
    {
        BOOST_CHECK( labels.count( expected ) );
    }
}


BOOST_AUTO_TEST_CASE( TitleBlockDisplayPropertiesAreRendered )
{
    ORCAD_SYMBOL_DEF definition;
    definition.name = "TITLE";
    definition.bbox = ORCAD_BBOX{ 0, 0, 100, 50 };
    definition.props["SIZE"] = "N/A";

    ORCAD_GRAPHIC_INST titleBlock;
    titleBlock.name = definition.name;
    titleBlock.x = 0;
    titleBlock.y = 0;
    titleBlock.bbox = ORCAD_BBOX{ 100, 200, 200, 250 };
    titleBlock.props["Title"] = "Amplifier";
    titleBlock.props["Page Number"] = "7";
    titleBlock.props["Page Count"] = "9";
    titleBlock.props["Page Modify Date"] = "Wednesday, January 25, 2023";

    ORCAD_DISPLAY_PROP displayedTitle;
    displayedTitle.name = "Title";
    displayedTitle.x = 10;
    displayedTitle.y = 20;
    displayedTitle.rotation = 1;
    displayedTitle.dispMode = 0x100;
    displayedTitle.color = 5;
    titleBlock.displayProps.push_back( displayedTitle );

    ORCAD_DISPLAY_PROP displayedPageSize;
    displayedPageSize.name = "SIZE";
    displayedPageSize.x = 30;
    displayedPageSize.y = 40;
    displayedPageSize.dispMode = 0x100;
    titleBlock.displayProps.push_back( displayedPageSize );

    ORCAD_DISPLAY_PROP displayedDate;
    displayedDate.name = "Page Modify Date";
    displayedDate.x = 40;
    displayedDate.y = 45;
    displayedDate.dispMode = 0x100;
    titleBlock.displayProps.push_back( displayedDate );

    ORCAD_DISPLAY_PROP displayedPageNumber;
    displayedPageNumber.name = "Page Number";
    displayedPageNumber.x = 50;
    displayedPageNumber.y = 50;
    displayedPageNumber.dispMode = 0x100;
    titleBlock.displayProps.push_back( displayedPageNumber );

    ORCAD_DISPLAY_PROP displayedPageCount;
    displayedPageCount.name = "Page Count";
    displayedPageCount.x = 60;
    displayedPageCount.y = 55;
    displayedPageCount.dispMode = 0x100;
    titleBlock.displayProps.push_back( displayedPageCount );

    ORCAD_RAW_PAGE page;
    page.name = "SHEET";
    page.pageSize = "B";
    page.sourcePageNumber = 2;
    page.sourcePageCount = 3;
    page.width = 1000;
    page.height = 1000;
    page.modifyTimestamp = 1674605190;
    page.titleBlocks.push_back( std::move( titleBlock ) );

    ORCAD_DESIGN design;
    design.sourceId = "title-block-properties";
    design.symbols.emplace( definition.name, std::move( definition ) );
    design.pages.push_back( std::move( page ) );

    SETTINGS_MANAGER manager;
    manager.LoadProject( "" );
    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    schematic->SetProject( &manager.Prj() );
    SCH_SHEET* root = convertRawDesign( design, *schematic );

    const SCH_TEXT* title = nullptr;
    const SCH_TEXT* pageSize = nullptr;
    const SCH_TEXT* date = nullptr;

    for( const SCH_ITEM* item : root->GetScreen()->Items() )
    {
        if( item->Type() == SCH_TEXT_T && static_cast<const SCH_TEXT*>( item )->GetText() == wxS( "Amplifier" ) )
            title = static_cast<const SCH_TEXT*>( item );
        else if( item->Type() == SCH_TEXT_T && static_cast<const SCH_TEXT*>( item )->GetText() == wxS( "N/A" ) )
            pageSize = static_cast<const SCH_TEXT*>( item );
        else if( item->Type() == SCH_TEXT_T
                 && static_cast<const SCH_TEXT*>( item )->GetText() == wxS( "Tuesday, January 24, 2023" ) )
            date = static_cast<const SCH_TEXT*>( item );
    }

    BOOST_REQUIRE( title );
    BOOST_CHECK_EQUAL( title->GetPosition().y, OrcadDbuToIu( 110, 220 ).y );
    BOOST_CHECK_GT( title->GetPosition().x, OrcadDbuToIu( 110, 220 ).x );
    BOOST_CHECK_EQUAL( title->GetTextAngle(), ANGLE_VERTICAL );
    BOOST_CHECK( title->GetTextColor() == KIGFX::COLOR4D( 0.0, 0.0, 0.0, 1.0 ) );
    BOOST_REQUIRE( pageSize );
    BOOST_REQUIRE( date );

    std::set<wxString> renderedText;

    for( const SCH_ITEM* item : root->GetScreen()->Items() )
    {
        if( item->Type() == SCH_TEXT_T )
            renderedText.insert( static_cast<const SCH_TEXT*>( item )->GetText() );
    }

    BOOST_CHECK( renderedText.count( wxS( "2" ) ) );
    BOOST_CHECK( renderedText.count( wxS( "3" ) ) );
    BOOST_CHECK( !renderedText.count( wxS( "7" ) ) );
    BOOST_CHECK( !renderedText.count( wxS( "9" ) ) );
    BOOST_CHECK( !renderedText.count( wxS( "<Title>" ) ) );
}


BOOST_AUTO_TEST_CASE( TitleBlockLibraryDefaultSurvivesEmptyInstanceProperty )
{
    ORCAD_SYMBOL_DEF definition;
    definition.name = "TITLE";
    definition.props["Title"] = "<Title>";

    ORCAD_GRAPHIC_INST titleBlock;
    titleBlock.name = definition.name;
    titleBlock.props["Title"] = "";

    ORCAD_DISPLAY_PROP displayedTitle;
    displayedTitle.name = "Title";
    displayedTitle.dispMode = 0x100;
    titleBlock.displayProps.push_back( displayedTitle );

    ORCAD_RAW_PAGE page;
    page.name = "SHEET";
    page.width = 1000;
    page.height = 1000;
    page.titleBlocks.push_back( std::move( titleBlock ) );

    ORCAD_DESIGN design;
    design.sourceId = "title-block-placeholder";
    design.symbols.emplace( definition.name, std::move( definition ) );
    design.pages.push_back( std::move( page ) );

    SETTINGS_MANAGER manager;
    manager.LoadProject( "" );
    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    schematic->SetProject( &manager.Prj() );
    SCH_SHEET* root = convertRawDesign( design, *schematic );

    bool found = false;

    for( const SCH_ITEM* item : root->GetScreen()->Items().OfType( SCH_TEXT_T ) )
        found |= static_cast<const SCH_TEXT*>( item )->GetText() == wxS( "<Title>" );

    BOOST_CHECK( found );
}


BOOST_AUTO_TEST_CASE( SchematicModifyDateUsesPageTimestamp )
{
    ORCAD_SYMBOL_DEF definition;
    definition.name = "TITLE";

    ORCAD_GRAPHIC_INST titleBlock;
    titleBlock.name = definition.name;

    ORCAD_DISPLAY_PROP displayedDate;
    displayedDate.name = "Schematic Modify Date";
    displayedDate.dispMode = 0x100;
    titleBlock.displayProps.push_back( displayedDate );

    ORCAD_RAW_PAGE page;
    page.name = "SHEET";
    page.width = 1000;
    page.height = 1000;
    page.modifyTimestamp = 1676607901;
    page.titleBlocks.push_back( std::move( titleBlock ) );

    ORCAD_DESIGN design;
    design.sourceId = "schematic-modify-date";
    design.symbols.emplace( definition.name, std::move( definition ) );
    design.pages.push_back( std::move( page ) );

    SETTINGS_MANAGER manager;
    manager.LoadProject( "" );
    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    schematic->SetProject( &manager.Prj() );
    SCH_SHEET* root = convertRawDesign( design, *schematic );

    std::set<wxString> renderedText;

    for( const SCH_ITEM* item : root->GetScreen()->Items().OfType( SCH_TEXT_T ) )
        renderedText.insert( static_cast<const SCH_TEXT*>( item )->GetText() );

    BOOST_CHECK( renderedText.count( wxS( "Thursday, February 16, 2023" ) ) );
    BOOST_CHECK( !renderedText.count( wxS( "<Schematic Modify Date>" ) ) );
}


BOOST_AUTO_TEST_CASE( RudyTitleBlockUsesCaptureShortDate )
{
    ORCAD_SYMBOL_DEF definition;
    definition.name = "TITLEBLK/Rudy";

    ORCAD_GRAPHIC_INST titleBlock;
    titleBlock.name = definition.name;

    ORCAD_DISPLAY_PROP displayedDate;
    displayedDate.name = "Page Modify Date";
    displayedDate.dispMode = 0x100;
    titleBlock.displayProps.push_back( displayedDate );

    ORCAD_RAW_PAGE page;
    page.name = "SHEET";
    page.width = 1000;
    page.height = 1000;
    page.modifyTimestamp = 1676607901;
    page.titleBlocks.push_back( std::move( titleBlock ) );

    ORCAD_DESIGN design;
    design.sourceId = "rudy-title-block-date";
    design.symbols.emplace( definition.name, std::move( definition ) );
    design.pages.push_back( std::move( page ) );

    SETTINGS_MANAGER manager;
    manager.LoadProject( "" );
    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    schematic->SetProject( &manager.Prj() );
    SCH_SHEET* root = convertRawDesign( design, *schematic );

    std::set<wxString> renderedText;

    for( const SCH_ITEM* item : root->GetScreen()->Items().OfType( SCH_TEXT_T ) )
        renderedText.insert( static_cast<const SCH_TEXT*>( item )->GetText() );

    BOOST_CHECK( renderedText.count( wxS( "Feb 16, 2023" ) ) );
    BOOST_CHECK( !renderedText.count( wxS( "Thursday, February 16, 2023" ) ) );
}


BOOST_AUTO_TEST_CASE( EmptyTitleBlockDisplayPropertiesUseCapturePlaceholders )
{
    ORCAD_SYMBOL_DEF definition;
    definition.name = "TITLE";

    ORCAD_GRAPHIC_INST titleBlock;
    titleBlock.name = definition.name;
    titleBlock.props["Title"] = "";
    titleBlock.props["Doc"] = "";
    titleBlock.props["RevCode"] = "?";

    for( const std::string& name : { "Title", "Doc", "RevCode" } )
    {
        ORCAD_DISPLAY_PROP displayed;
        displayed.name = name;
        displayed.dispMode = 0x100;
        titleBlock.displayProps.push_back( displayed );
    }

    ORCAD_RAW_PAGE page;
    page.name = "SHEET";
    page.width = 1000;
    page.height = 1000;
    page.titleBlocks.push_back( std::move( titleBlock ) );

    ORCAD_DESIGN design;
    design.sourceId = "empty-title-block-placeholders";
    design.symbols.emplace( definition.name, std::move( definition ) );
    design.pages.push_back( std::move( page ) );

    SETTINGS_MANAGER manager;
    manager.LoadProject( "" );
    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    schematic->SetProject( &manager.Prj() );
    SCH_SHEET* root = convertRawDesign( design, *schematic );

    std::set<wxString> renderedText;

    for( const SCH_ITEM* item : root->GetScreen()->Items().OfType( SCH_TEXT_T ) )
        renderedText.insert( static_cast<const SCH_TEXT*>( item )->GetText() );

    BOOST_CHECK( renderedText.count( wxS( "<Title>" ) ) );
    BOOST_CHECK( renderedText.count( wxS( "<Doc>" ) ) );
    BOOST_CHECK( renderedText.count( wxS( "<RevCode>" ) ) );
    BOOST_CHECK( root->GetScreen()->GetTitleBlock().GetRevision().IsEmpty() );
}


BOOST_AUTO_TEST_CASE( MultipleTitleBlocksAreAllRendered )
{
    ORCAD_SYMBOL_DEF revision;
    revision.name = "REVISION";
    revision.primitives.push_back( ORCAD_PRIMITIVE{ .kind = ORCAD_PRIM_KIND::TEXT,
                                                     .text = "REVISION HISTORY" } );

    ORCAD_SYMBOL_DEF main;
    main.name = "MAIN";
    main.primitives.push_back( ORCAD_PRIMITIVE{ .kind = ORCAD_PRIM_KIND::TEXT,
                                                 .text = "CUSTOMER NOTICE" } );

    ORCAD_GRAPHIC_INST revisionBlock;
    revisionBlock.name = revision.name;
    revisionBlock.props["RevCode"] = "2";

    ORCAD_GRAPHIC_INST mainBlock;
    mainBlock.name = main.name;
    mainBlock.props["Title"] = "Power Supply";

    ORCAD_RAW_PAGE page;
    page.name = "SHEET";
    page.width = 1000;
    page.height = 1000;
    page.titleBlocks.push_back( std::move( revisionBlock ) );
    page.titleBlocks.push_back( std::move( mainBlock ) );

    ORCAD_DESIGN design;
    design.sourceId = "multiple-title-blocks";
    design.symbols.emplace( revision.name, std::move( revision ) );
    design.symbols.emplace( main.name, std::move( main ) );
    design.pages.push_back( std::move( page ) );

    SETTINGS_MANAGER manager;
    manager.LoadProject( "" );
    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    schematic->SetProject( &manager.Prj() );
    SCH_SHEET* root = convertRawDesign( design, *schematic );

    std::set<wxString> renderedText;

    for( const SCH_ITEM* item : root->GetScreen()->Items().OfType( SCH_TEXT_T ) )
        renderedText.insert( static_cast<const SCH_TEXT*>( item )->GetText() );

    BOOST_CHECK( renderedText.count( wxS( "REVISION HISTORY" ) ) );
    BOOST_CHECK( renderedText.count( wxS( "CUSTOMER NOTICE" ) ) );
}


BOOST_AUTO_TEST_CASE( TitleBlockVariantMatchesEncodedDimensions )
{
    ORCAD_SYMBOL_DEF primary;
    primary.name = "TITLE";
    primary.bbox = ORCAD_BBOX{ 0, 0, 100, 50 };
    primary.primitives.push_back( ORCAD_PRIMITIVE{ .kind = ORCAD_PRIM_KIND::TEXT,
                                                   .text = "PRIMARY" } );

    ORCAD_SYMBOL_DEF matching;
    matching.name = primary.name;
    matching.bbox = ORCAD_BBOX{ 0, 0, 200, 50 };
    matching.primitives.push_back( ORCAD_PRIMITIVE{ .kind = ORCAD_PRIM_KIND::TEXT,
                                                    .text = "MATCHED" } );
    primary.variants.push_back( std::move( matching ) );

    ORCAD_GRAPHIC_INST titleBlock;
    titleBlock.name = primary.name;
    titleBlock.bbox = ORCAD_BBOX{ 300, 400, 200, 50 };

    ORCAD_RAW_PAGE page;
    page.name = "SHEET";
    page.titleBlocks.push_back( std::move( titleBlock ) );

    ORCAD_DESIGN design;
    design.sourceId = "title-block-variant-bounds";
    design.symbols.emplace( primary.name, std::move( primary ) );
    design.pages.push_back( std::move( page ) );

    SETTINGS_MANAGER manager;
    manager.LoadProject( "" );
    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    schematic->SetProject( &manager.Prj() );
    SCH_SHEET* root = convertRawDesign( design, *schematic );

    std::set<wxString> renderedText;

    for( const SCH_ITEM* item : root->GetScreen()->Items().OfType( SCH_TEXT_T ) )
        renderedText.insert( static_cast<const SCH_TEXT*>( item )->GetText() );

    BOOST_CHECK( renderedText.count( wxS( "MATCHED" ) ) );
    BOOST_CHECK( !renderedText.count( wxS( "PRIMARY" ) ) );
}


BOOST_AUTO_TEST_CASE( TitleBlockVectorTextBoundsDoNotCauseWrapping )
{
    ORCAD_SYMBOL_DEF definition;
    definition.name = "TITLE";
    definition.bbox = ORCAD_BBOX{ 0, 0, 790, 180 };
    definition.primitives.push_back( ORCAD_PRIMITIVE{ .kind = ORCAD_PRIM_KIND::LINE,
                                                       .x1 = 0,
                                                       .y1 = 0,
                                                       .x2 = 100,
                                                       .y2 = 0,
                                                       .lineWidth = 0 } );
    definition.primitives.push_back( ORCAD_PRIMITIVE{ .kind = ORCAD_PRIM_KIND::TEXT,
                                                       .x1 = 53,
                                                       .y1 = 4,
                                                       .x2 = 215,
                                                       .y2 = 24,
                                                       .text = "CUSTOMER NOTICE",
                                                       .fontIdx = 1,
                                                       .textBoundsStart = ORCAD_POINT{ 20, 4 } } );

    ORCAD_GRAPHIC_INST titleBlock;
    titleBlock.name = definition.name;
    titleBlock.bbox = ORCAD_BBOX{ 440, 720, 1230, 900 };

    ORCAD_RAW_PAGE page;
    page.name = "SHEET";
    page.titleBlocks.push_back( std::move( titleBlock ) );

    ORCAD_DESIGN design;
    design.sourceId = "title-block-vector-text";
    design.library.fonts = { ORCAD_FONT{ .height = -20, .face = "Arial Narrow", .bold = true } };
    design.symbols.emplace( definition.name, std::move( definition ) );
    design.pages.push_back( std::move( page ) );

    SETTINGS_MANAGER manager;
    manager.LoadProject( "" );
    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    schematic->SetProject( &manager.Prj() );
    SCH_SHEET* root = convertRawDesign( design, *schematic );

    BOOST_CHECK( root->GetScreen()->Items().OfType( SCH_TEXTBOX_T ).empty() );

    const SCH_TEXT* vectorText = nullptr;

    for( const SCH_ITEM* item : root->GetScreen()->Items().OfType( SCH_TEXT_T ) )
        vectorText = static_cast<const SCH_TEXT*>( item );

    BOOST_REQUIRE( vectorText );
    BOOST_CHECK_EQUAL( vectorText->GetText(), wxString( "CUSTOMER NOTICE" ) );
    BOOST_CHECK_EQUAL( vectorText->GetTextHeight(), KiROUND( schIUScale.mmToIU( 3.31 ) * 6.0 / 5.0 ) );
    BOOST_CHECK_EQUAL( vectorText->GetTextWidth(), KiROUND( schIUScale.mmToIU( 3.31 ) * 35.0 / 32.0 ) );
    BOOST_CHECK_EQUAL( vectorText->GetFont()->GetName(), wxString( "Arial Narrow" ) );
    BOOST_CHECK_EQUAL( vectorText->GetPosition().x, OrcadDbuToIu( 493, 724 ).x );
    BOOST_CHECK_EQUAL( vectorText->GetPosition().y,
                       OrcadDbuToIu( 493, 724 ).y
                               + OrcadTextBaselineOffset( vectorText->GetTextHeight() ) );

    const SCH_SHAPE* vectorLine = nullptr;

    for( const SCH_ITEM* item : root->GetScreen()->Items().OfType( SCH_SHAPE_T ) )
        vectorLine = static_cast<const SCH_SHAPE*>( item );

    BOOST_REQUIRE( vectorLine );
    BOOST_CHECK_EQUAL( vectorLine->GetStroke().GetWidth(), OrcadLineWidthIu( 0 ) );
}


BOOST_AUTO_TEST_CASE( NumericPageNamesOverrideRotatedStorageOrder )
{
    ORCAD_DESIGN design;
    design.sourceId = "rotated-page-order";

    for( const std::string& name : { "PAGE_02_CONTENT", "PAGE_03_CONTENT", "PAGE_01_INDEX" } )
    {
        ORCAD_RAW_PAGE page;
        page.name = name;
        page.pageSize = "A";
        page.width = 9700;
        page.height = 7200;
        page.sourcePageNumber = design.pages.size() + 1;
        page.sourcePageCount = 3;
        design.pages.push_back( std::move( page ) );
    }

    SETTINGS_MANAGER manager;
    manager.LoadProject( "" );
    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    schematic->SetProject( &manager.Prj() );
    convertRawDesign( design, *schematic );

    std::vector<SCH_SHEET*> sheets = schematic->GetTopLevelSheets();
    BOOST_REQUIRE_EQUAL( sheets.size(), 3u );
    BOOST_CHECK_EQUAL( sheets[0]->GetField( FIELD_T::SHEET_NAME )->GetText(), wxS( "PAGE_01_INDEX" ) );
    BOOST_CHECK_EQUAL( sheets[1]->GetField( FIELD_T::SHEET_NAME )->GetText(), wxS( "PAGE_02_CONTENT" ) );
    BOOST_CHECK_EQUAL( sheets[2]->GetField( FIELD_T::SHEET_NAME )->GetText(), wxS( "PAGE_03_CONTENT" ) );

    std::map<std::string, size_t> pageNumbers;

    for( const ORCAD_RAW_PAGE& page : design.pages )
        pageNumbers[page.name] = page.sourcePageNumber;

    BOOST_CHECK_EQUAL( pageNumbers["PAGE_01_INDEX"], 1u );
    BOOST_CHECK_EQUAL( pageNumbers["PAGE_02_CONTENT"], 2u );
    BOOST_CHECK_EQUAL( pageNumbers["PAGE_03_CONTENT"], 3u );
}


BOOST_AUTO_TEST_CASE( TitleBlockPageNumbersOverrideStorageOrder )
{
    ORCAD_DESIGN design;
    design.sourceId = "title-block-page-order";

    for( const auto& [name, pageNumber] : { std::pair{ "SECOND", "2" }, std::pair{ "FIRST", "1" } } )
    {
        ORCAD_RAW_PAGE page;
        page.name = name;
        page.pageSize = "A";
        page.width = 9700;
        page.height = 7200;
        page.sourcePageNumber = design.pages.size() + 1;
        page.sourcePageCount = 2;
        ORCAD_GRAPHIC_INST titleBlock;
        titleBlock.props["Page Number"] = pageNumber;
        page.titleBlocks.push_back( std::move( titleBlock ) );
        design.pages.push_back( std::move( page ) );
    }

    SETTINGS_MANAGER manager;
    manager.LoadProject( "" );
    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    schematic->SetProject( &manager.Prj() );
    convertRawDesign( design, *schematic );

    std::vector<SCH_SHEET*> sheets = schematic->GetTopLevelSheets();
    BOOST_REQUIRE_EQUAL( sheets.size(), 2u );
    BOOST_CHECK_EQUAL( sheets[0]->GetField( FIELD_T::SHEET_NAME )->GetText(), wxS( "FIRST" ) );
    BOOST_CHECK_EQUAL( sheets[1]->GetField( FIELD_T::SHEET_NAME )->GetText(), wxS( "SECOND" ) );
}


static SCH_SYMBOL* findConvertedSymbol( SCH_SCREEN& aScreen, const SCH_SHEET_PATH& aPath, const wxString& aReference )
{
    for( SCH_ITEM* item : aScreen.Items().OfType( SCH_SYMBOL_T ) )
    {
        SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

        if( symbol->GetRef( &aPath, false ) == aReference )
            return symbol;
    }

    return nullptr;
}


BOOST_AUTO_TEST_CASE( PageGraphicUuidsFollowSourceOrder )
{
    ORCAD_GRAPHIC_INST comment;
    comment.typeId = ORCAD_ST_GRAPHIC_COMMENT_TEXT_INST;
    comment.nested = std::make_unique<ORCAD_SYMBOL_DEF>();
    comment.nested->primitives.push_back(
            ORCAD_PRIMITIVE{ .kind = ORCAD_PRIM_KIND::TEXT, .x1 = 10, .y1 = 30, .text = "SOURCE FIRST" } );
    comment.nested->primitives.push_back(
            ORCAD_PRIMITIVE{ .kind = ORCAD_PRIM_KIND::TEXT, .x1 = 10, .y1 = 10, .text = "SOURCE SECOND" } );

    ORCAD_RAW_PAGE page;
    page.name = "PAGE GRAPHIC ORDER";
    page.graphics.push_back( std::move( comment ) );

    ORCAD_DESIGN design;
    design.sourceId = "page-graphic-order";
    design.pages.push_back( std::move( page ) );

    SETTINGS_MANAGER manager;
    manager.LoadProject( "" );
    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    schematic->SetProject( &manager.Prj() );
    SCH_SHEET* root = convertRawDesign( design, *schematic );

    std::map<wxString, KIID> textUuids;

    for( SCH_ITEM* item : root->GetScreen()->Items().OfType( SCH_TEXT_T ) )
    {
        SCH_TEXT* text = static_cast<SCH_TEXT*>( item );
        textUuids.emplace( text->GetText(), text->m_Uuid );
    }

    BOOST_REQUIRE_EQUAL( textUuids.size(), 2u );
    std::string role = "orcad-import:page-graphic-order:page:0:item:"
                       + std::to_string( static_cast<int>( SCH_TEXT_T ) );
    BOOST_CHECK( textUuids.at( wxS( "SOURCE FIRST" ) ) == KIID::FromName( role + ":0" ) );
    BOOST_CHECK( textUuids.at( wxS( "SOURCE SECOND" ) ) == KIID::FromName( role + ":1" ) );
}


BOOST_AUTO_TEST_CASE( PageCommentSourceBoundsDefineWrapWidth )
{
    ORCAD_GRAPHIC_INST comment;
    comment.typeId = ORCAD_ST_GRAPHIC_COMMENT_TEXT_INST;
    comment.nested = std::make_unique<ORCAD_SYMBOL_DEF>();
    comment.nested->primitives.push_back( ORCAD_PRIMITIVE{ .kind = ORCAD_PRIM_KIND::TEXT,
                                                           .x1 = 10,
                                                           .y1 = 20,
                                                           .x2 = 166,
                                                           .y2 = 35,
                                                           .text = "PCA ADDITIONAL PARTS",
                                                           .fontIdx = 1,
                                                           .textBoundsStart = ORCAD_POINT{ 10, 20 } } );

    ORCAD_RAW_PAGE page;
    page.name = "BOXED COMMENT";
    page.graphics.push_back( std::move( comment ) );

    ORCAD_DESIGN design;
    design.sourceId = "boxed-comment";
    design.library.fonts = { ORCAD_FONT{ .height = -17, .face = "Arial Narrow", .bold = true } };
    design.pages.push_back( std::move( page ) );

    SETTINGS_MANAGER manager;
    manager.LoadProject( "" );
    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    schematic->SetProject( &manager.Prj() );
    SCH_SHEET* root = convertRawDesign( design, *schematic );

    BOOST_CHECK( root->GetScreen()->Items().OfType( SCH_TEXTBOX_T ).empty() );

    const SCH_TEXT* sheetText = nullptr;

    for( const SCH_ITEM* item : root->GetScreen()->Items().OfType( SCH_TEXT_T ) )
        sheetText = static_cast<const SCH_TEXT*>( item );

    BOOST_REQUIRE( sheetText );
    BOOST_CHECK_EQUAL( sheetText->GetText(), wxString( "PCA ADDITIONAL\nPARTS" ) );
    BOOST_CHECK_LE( std::abs( sheetText->GetTextBox( nullptr ).GetWidth() - OrcadDbuToIu( 156, 0 ).x ),
                    OrcadDbuToIu( 1, 0 ).x );
    BOOST_CHECK_EQUAL( sheetText->GetHorizJustify(), GR_TEXT_H_ALIGN_LEFT );
    BOOST_REQUIRE( sheetText->GetFont() );
    BOOST_CHECK_EQUAL( sheetText->GetFont()->GetName(), wxString( "Arial Narrow" ) );
}


BOOST_AUTO_TEST_CASE( BoldArialNarrowCommentIgnoresSmallMetricOverflow )
{
    ORCAD_GRAPHIC_INST comment;
    comment.typeId = ORCAD_ST_GRAPHIC_COMMENT_TEXT_INST;
    comment.nested = std::make_unique<ORCAD_SYMBOL_DEF>();
    comment.nested->primitives.push_back( ORCAD_PRIMITIVE{ .kind = ORCAD_PRIM_KIND::TEXT,
                                                           .x1 = 400,
                                                           .y1 = 30,
                                                           .x2 = 658,
                                                           .y2 = 63,
                                                           .text = "DC590 SPI INTERFACE",
                                                           .fontIdx = 1,
                                                           .textBoundsStart = ORCAD_POINT{ 400, 30 } } );

    ORCAD_RAW_PAGE page;
    page.name = "BOLD NARROW STORED WIDTH";
    page.graphics.push_back( std::move( comment ) );

    ORCAD_DESIGN design;
    design.sourceId = "bold-narrow-small-overflow";
    design.library.fonts = {
        ORCAD_FONT{ .height = -28, .face = "Arial Narrow", .bold = true }
    };
    design.pages.push_back( std::move( page ) );

    SETTINGS_MANAGER manager;
    manager.LoadProject( "" );
    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    schematic->SetProject( &manager.Prj() );
    SCH_SHEET* root = convertRawDesign( design, *schematic );

    const SCH_TEXT* sheetText = nullptr;

    for( const SCH_ITEM* item : root->GetScreen()->Items().OfType( SCH_TEXT_T ) )
        sheetText = static_cast<const SCH_TEXT*>( item );

    BOOST_REQUIRE( sheetText );
    BOOST_CHECK_EQUAL( sheetText->GetText(), wxString( "DC590 SPI INTERFACE" ) );
}


BOOST_AUTO_TEST_CASE( BoldArialNarrowNoteIgnoresSmallMetricOverflow )
{
    ORCAD_GRAPHIC_INST comment;
    comment.typeId = ORCAD_ST_GRAPHIC_COMMENT_TEXT_INST;
    comment.nested = std::make_unique<ORCAD_SYMBOL_DEF>();
    comment.nested->primitives.push_back( ORCAD_PRIMITIVE{ .kind = ORCAD_PRIM_KIND::TEXT,
                                                           .x1 = 140,
                                                           .y1 = 960,
                                                           .x2 = 390,
                                                           .y2 = 980,
                                                           .text = "1. ALL RESISTORS ARE IN OHMS, 0402",
                                                           .fontIdx = 1,
                                                           .textBoundsStart = ORCAD_POINT{ 140, 960 } } );

    ORCAD_RAW_PAGE page;
    page.name = "BOLD NARROW NOTE";
    page.graphics.push_back( std::move( comment ) );

    ORCAD_DESIGN design;
    design.sourceId = "bold-narrow-note-small-overflow";
    design.library.fonts = {
        ORCAD_FONT{ .height = -16, .face = "Arial Narrow", .bold = true }
    };
    design.pages.push_back( std::move( page ) );

    SETTINGS_MANAGER manager;
    manager.LoadProject( "" );
    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    schematic->SetProject( &manager.Prj() );
    SCH_SHEET* root = convertRawDesign( design, *schematic );

    const SCH_TEXT* sheetText = nullptr;

    for( const SCH_ITEM* item : root->GetScreen()->Items().OfType( SCH_TEXT_T ) )
        sheetText = static_cast<const SCH_TEXT*>( item );

    BOOST_REQUIRE( sheetText );
    BOOST_CHECK_EQUAL( sheetText->GetText(), wxString( "1. ALL RESISTORS ARE IN OHMS, 0402" ) );
}


BOOST_AUTO_TEST_CASE( PageCommentUsesSourceAnchorWhenBoundsArePresent )
{
    ORCAD_GRAPHIC_INST comment;
    comment.typeId = ORCAD_ST_GRAPHIC_COMMENT_TEXT_INST;
    comment.nested = std::make_unique<ORCAD_SYMBOL_DEF>();
    comment.nested->primitives.push_back( ORCAD_PRIMITIVE{ .kind = ORCAD_PRIM_KIND::TEXT,
                                                           .x1 = 50,
                                                           .y1 = 130,
                                                           .x2 = 118,
                                                           .y2 = 145,
                                                           .text = "REF DES",
                                                           .fontIdx = 1,
                                                           .textBoundsStart = ORCAD_POINT{ 50, 130 } } );

    ORCAD_RAW_PAGE page;
    page.name = "SOURCE-ANCHORED COMMENT";
    page.width = 1000;
    page.height = 1000;
    page.graphics.push_back( std::move( comment ) );

    ORCAD_DESIGN design;
    design.sourceId = "source-anchored-comment";
    design.library.fonts = { ORCAD_FONT{ .height = -13, .face = "Arial", .italic = true } };
    design.pages.push_back( std::move( page ) );

    SETTINGS_MANAGER manager;
    manager.LoadProject( "" );
    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    schematic->SetProject( &manager.Prj() );
    SCH_SHEET* root = convertRawDesign( design, *schematic );

    const SCH_TEXT* sheetText = nullptr;

    for( const SCH_ITEM* item : root->GetScreen()->Items().OfType( SCH_TEXT_T ) )
        sheetText = static_cast<const SCH_TEXT*>( item );

    BOOST_REQUIRE( sheetText );
    BOOST_CHECK_EQUAL( sheetText->GetPosition().x, OrcadDbuToIu( 50, 130 ).x );
    BOOST_CHECK_EQUAL( sheetText->GetHorizJustify(), GR_TEXT_H_ALIGN_LEFT );
    BOOST_CHECK_LE( std::abs( sheetText->GetTextBox( nullptr ).GetWidth() - OrcadDbuToIu( 68, 0 ).x ),
                    OrcadDbuToIu( 1, 0 ).x );
}


BOOST_AUTO_TEST_CASE( ElephantPageCommentUsesSourceBoundsForBaseline )
{
    ORCAD_GRAPHIC_INST comment;
    comment.typeId = ORCAD_ST_GRAPHIC_COMMENT_TEXT_INST;
    comment.nested = std::make_unique<ORCAD_SYMBOL_DEF>();
    comment.nested->primitives.push_back(
            ORCAD_PRIMITIVE{ .kind = ORCAD_PRIM_KIND::TEXT,
                             .x1 = 840,
                             .y1 = 610,
                             .x2 = 1024,
                             .y2 = 623,
                             .text = "*All Test Points are No Load",
                             .fontIdx = 1,
                             .textBoundsStart = ORCAD_POINT{ 840, 610 } } );

    ORCAD_RAW_PAGE page;
    page.name = "ELEPHANT COMMENT";
    page.graphics.push_back( std::move( comment ) );

    ORCAD_DESIGN design;
    design.sourceId = "elephant-comment";
    design.library.fonts = { ORCAD_FONT{ .height = -13, .face = "Elephant" } };
    design.pages.push_back( std::move( page ) );

    SETTINGS_MANAGER manager;
    manager.LoadProject( "" );
    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    schematic->SetProject( &manager.Prj() );
    SCH_SHEET* root = convertRawDesign( design, *schematic );

    const SCH_TEXT* sheetText = nullptr;

    for( const SCH_ITEM* item : root->GetScreen()->Items().OfType( SCH_TEXT_T ) )
        sheetText = static_cast<const SCH_TEXT*>( item );

    BOOST_REQUIRE( sheetText );
    BOOST_CHECK_EQUAL( sheetText->GetText(), wxS( "*All Test Points are No Load" ) );
    BOOST_REQUIRE( sheetText->GetFont() );
    BOX2I ink = sheetText->GetEffectiveTextShape( false, BOX2I(), ANGLE_0 )->BBox();
    ink.Offset( sheetText->GetSchematicTextOffset( nullptr )
                + sheetText->GetOffsetToMatchSCH_FIELD( nullptr ) );
    BOOST_CHECK_SMALL( ink.GetY() - OrcadDbuToIu( 0, 613 ).y, OrcadDbuToIu( 0, 1 ).y );
    BOOST_CHECK_SMALL( ink.GetBottom() - OrcadDbuToIu( 0, 623 ).y, OrcadDbuToIu( 0, 1 ).y );
}


BOOST_AUTO_TEST_CASE( ArialNarrowCommentBoundsDoNotImplyWrapping )
{
    ORCAD_GRAPHIC_INST comment;
    comment.typeId = ORCAD_ST_GRAPHIC_COMMENT_TEXT_INST;
    comment.nested = std::make_unique<ORCAD_SYMBOL_DEF>();
    comment.nested->primitives.push_back( ORCAD_PRIMITIVE{ .kind = ORCAD_PRIM_KIND::TEXT,
                                                           .x1 = 80,
                                                           .y1 = 940,
                                                           .x2 = 243,
                                                           .y2 = 956,
                                                           .text = "OPTIONAL COMPONENTS",
                                                           .fontIdx = 1 } );

    ORCAD_RAW_PAGE page;
    page.name = "NARROW WRAPPED COMMENT";
    page.graphics.push_back( std::move( comment ) );

    ORCAD_DESIGN design;
    design.sourceId = "narrow-wrapped-comment";
    design.library.fonts = { ORCAD_FONT{ .height = -16, .face = "Arial Narrow" } };
    design.pages.push_back( std::move( page ) );

    SETTINGS_MANAGER manager;
    manager.LoadProject( "" );
    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    schematic->SetProject( &manager.Prj() );
    SCH_SHEET* root = convertRawDesign( design, *schematic );

    const SCH_TEXT* sheetText = nullptr;

    for( const SCH_ITEM* item : root->GetScreen()->Items().OfType( SCH_TEXT_T ) )
        sheetText = static_cast<const SCH_TEXT*>( item );

    BOOST_REQUIRE( sheetText );
    BOOST_CHECK_EQUAL( sheetText->GetText(), wxString( "OPTIONAL COMPONENTS" ) );
}


BOOST_AUTO_TEST_CASE( BoldArialNarrowCommentBoundsDoNotImplyWrapping )
{
    ORCAD_GRAPHIC_INST comment;
    comment.typeId = ORCAD_ST_GRAPHIC_COMMENT_TEXT_INST;
    comment.nested = std::make_unique<ORCAD_SYMBOL_DEF>();
    comment.nested->primitives.push_back( ORCAD_PRIMITIVE{ .kind = ORCAD_PRIM_KIND::TEXT,
                                                           .x1 = 926,
                                                           .y1 = 917,
                                                           .x2 = 1210,
                                                           .y2 = 933,
                                                           .text = "FOR USE WITH DC2321A DUST DEMOBOARD",
                                                           .fontIdx = 1 } );

    ORCAD_RAW_PAGE page;
    page.name = "BOLD NARROW SINGLE LINE COMMENT";
    page.graphics.push_back( std::move( comment ) );

    ORCAD_DESIGN design;
    design.sourceId = "bold-narrow-single-line-comment";
    design.library.fonts = { ORCAD_FONT{ .height = -16, .face = "Arial Narrow", .bold = true } };
    design.pages.push_back( std::move( page ) );

    SETTINGS_MANAGER manager;
    manager.LoadProject( "" );
    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    schematic->SetProject( &manager.Prj() );
    SCH_SHEET* root = convertRawDesign( design, *schematic );

    const SCH_TEXT* sheetText = nullptr;

    for( const SCH_ITEM* item : root->GetScreen()->Items().OfType( SCH_TEXT_T ) )
        sheetText = static_cast<const SCH_TEXT*>( item );

    BOOST_REQUIRE( sheetText );
    BOOST_CHECK_EQUAL( sheetText->GetText(), wxString( "FOR USE WITH DC2321A DUST DEMOBOARD" ) );
}


BOOST_AUTO_TEST_CASE( PageCommentRenderedOverflowDoesNotImplyWrapping )
{
    ORCAD_GRAPHIC_INST comment;
    comment.typeId = ORCAD_ST_GRAPHIC_COMMENT_TEXT_INST;
    comment.nested = std::make_unique<ORCAD_SYMBOL_DEF>();
    comment.nested->primitives.push_back( ORCAD_PRIMITIVE{ .kind = ORCAD_PRIM_KIND::TEXT,
                                                           .x1 = 10,
                                                           .y1 = 20,
                                                           .x2 = 110,
                                                           .y2 = 60,
                                                           .text = "NOTE: UNLESS OTHERWISE SPECIFIED",
                                                           .fontIdx = 1 } );

    ORCAD_RAW_PAGE page;
    page.name = "WRAPPED COMMENT";
    page.graphics.push_back( std::move( comment ) );

    ORCAD_DESIGN design;
    design.sourceId = "wrapped-comment";
    design.library.fonts = { ORCAD_FONT{ .height = -20, .face = "Arial", .bold = true } };
    design.pages.push_back( std::move( page ) );

    SETTINGS_MANAGER manager;
    manager.LoadProject( "" );
    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    schematic->SetProject( &manager.Prj() );
    SCH_SHEET* root = convertRawDesign( design, *schematic );

    const SCH_TEXT* sheetText = nullptr;

    for( const SCH_ITEM* item : root->GetScreen()->Items().OfType( SCH_TEXT_T ) )
        sheetText = static_cast<const SCH_TEXT*>( item );

    BOOST_REQUIRE( sheetText );
    BOOST_CHECK_EQUAL( sheetText->GetText(), wxString( "NOTE: UNLESS OTHERWISE SPECIFIED" ) );
}


BOOST_AUTO_TEST_CASE( PageCommentSmallMetricOverflowRemainsSingleLine )
{
    ORCAD_GRAPHIC_INST comment;
    comment.typeId = ORCAD_ST_GRAPHIC_COMMENT_TEXT_INST;
    comment.nested = std::make_unique<ORCAD_SYMBOL_DEF>();
    comment.nested->primitives.push_back( ORCAD_PRIMITIVE{ .kind = ORCAD_PRIM_KIND::TEXT,
                                                           .x1 = 10,
                                                           .y1 = 20,
                                                           .x2 = 51,
                                                           .y2 = 34,
                                                           .text = "SPI BUS",
                                                           .fontIdx = 1 } );

    ORCAD_RAW_PAGE page;
    page.name = "SMALL METRIC OVERFLOW";
    page.graphics.push_back( std::move( comment ) );

    ORCAD_DESIGN design;
    design.sourceId = "small-metric-overflow";
    design.library.fonts = { ORCAD_FONT{ .height = -11, .width = 5, .face = "Arial" } };
    design.pages.push_back( std::move( page ) );

    SETTINGS_MANAGER manager;
    manager.LoadProject( "" );
    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    schematic->SetProject( &manager.Prj() );
    SCH_SHEET* root = convertRawDesign( design, *schematic );

    const SCH_TEXT* sheetText = nullptr;

    for( const SCH_ITEM* item : root->GetScreen()->Items().OfType( SCH_TEXT_T ) )
        sheetText = static_cast<const SCH_TEXT*>( item );

    BOOST_REQUIRE( sheetText );
    BOOST_CHECK_EQUAL( sheetText->GetText(), wxString( "SPI BUS" ) );
}


BOOST_AUTO_TEST_CASE( LegacySymbolFontsAreConvertedToUnicode )
{
    ORCAD_GRAPHIC_INST comment;
    comment.typeId = ORCAD_ST_GRAPHIC_COMMENT_TEXT_INST;
    comment.nested = std::make_unique<ORCAD_SYMBOL_DEF>();
    comment.nested->primitives = {
        ORCAD_PRIMITIVE{ .kind = ORCAD_PRIM_KIND::TEXT, .x1 = 10, .y1 = 20, .text = "m", .fontIdx = 1 },
        ORCAD_PRIMITIVE{ .kind = ORCAD_PRIM_KIND::TEXT, .x1 = 30, .y1 = 20, .text = "b", .fontIdx = 2 },
    };

    ORCAD_RAW_PAGE page;
    page.name = "LEGACY SYMBOL FONTS";
    page.graphics.push_back( std::move( comment ) );

    ORCAD_DESIGN design;
    design.sourceId = "legacy-symbol-fonts";
    design.library.fonts = { ORCAD_FONT{ .height = -13, .face = "GreekC" },
                             ORCAD_FONT{ .height = -13, .face = "CommercialPi BT" } };
    design.pages.push_back( std::move( page ) );

    SETTINGS_MANAGER manager;
    manager.LoadProject( "" );
    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    schematic->SetProject( &manager.Prj() );
    SCH_SHEET* root = convertRawDesign( design, *schematic );

    std::set<wxString> texts;

    for( const SCH_ITEM* item : root->GetScreen()->Items().OfType( SCH_TEXT_T ) )
        texts.insert( static_cast<const SCH_TEXT*>( item )->GetText() );

    BOOST_CHECK( texts.count( wxString::FromUTF8( "µ" ) ) );
    BOOST_CHECK( texts.count( wxString::FromUTF8( "®" ) ) );
}


BOOST_AUTO_TEST_CASE( PageCommentTextThatFitsRemainsSingleLine )
{
    ORCAD_GRAPHIC_INST comment;
    comment.typeId = ORCAD_ST_GRAPHIC_COMMENT_TEXT_INST;
    comment.nested = std::make_unique<ORCAD_SYMBOL_DEF>();
    comment.nested->primitives.push_back(
            ORCAD_PRIMITIVE{ .kind = ORCAD_PRIM_KIND::TEXT,
                             .x1 = 387,
                             .y1 = 360,
                             .x2 = 647,
                             .y2 = 373,
                             .text = "SENZA resistenza di terminazione",
                             .fontIdx = 1 } );

    ORCAD_RAW_PAGE page;
    page.name = "SINGLE-LINE COMMENT";
    page.pageSize = "A";
    page.width = 9700;
    page.height = 7200;
    page.graphics.push_back( std::move( comment ) );

    ORCAD_DESIGN design;
    design.sourceId = "single-line-comment";
    design.library.fonts = { ORCAD_FONT{ .height = -13, .face = "Courier New" } };
    design.pages.push_back( std::move( page ) );

    SETTINGS_MANAGER manager;
    manager.LoadProject( "" );
    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    schematic->SetProject( &manager.Prj() );
    SCH_SHEET* root = convertRawDesign( design, *schematic );

    BOOST_CHECK( root->GetScreen()->Items().OfType( SCH_TEXTBOX_T ).empty() );

    const SCH_TEXT* sheetText = nullptr;

    for( const SCH_ITEM* item : root->GetScreen()->Items().OfType( SCH_TEXT_T ) )
        sheetText = static_cast<const SCH_TEXT*>( item );

    BOOST_REQUIRE( sheetText );
    BOOST_CHECK_EQUAL( sheetText->GetText(), wxString( "SENZA resistenza di terminazione" ) );
    BOOST_CHECK_EQUAL( sheetText->GetHorizJustify(), GR_TEXT_H_ALIGN_CENTER );
    BOOST_CHECK_EQUAL( sheetText->GetPosition().x, OrcadDbuToIu( ( 387 + 647 ) / 2, 0 ).x );
}


BOOST_AUTO_TEST_CASE( LargePageGraphicTextIsNotClamped )
{
    ORCAD_GRAPHIC_INST comment;
    comment.typeId = ORCAD_ST_GRAPHIC_COMMENT_TEXT_INST;
    comment.nested = std::make_unique<ORCAD_SYMBOL_DEF>();
    comment.nested->primitives.push_back( ORCAD_PRIMITIVE{ .kind = ORCAD_PRIM_KIND::TEXT,
                                                           .x1 = 100,
                                                           .y1 = 100,
                                                           .text = "LARGE TITLE",
                                                           .fontIdx = 1 } );

    ORCAD_RAW_PAGE page;
    page.name = "LARGE PAGE TEXT";
    page.pageSize = "A";
    page.width = 9700;
    page.height = 7200;
    page.graphics.push_back( std::move( comment ) );

    ORCAD_DESIGN design;
    design.sourceId = "large-page-text";
    design.library.fonts = { ORCAD_FONT{ .height = -48, .face = "Verdana", .bold = true } };
    design.pages.push_back( std::move( page ) );

    SETTINGS_MANAGER manager;
    manager.LoadProject( "" );
    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    schematic->SetProject( &manager.Prj() );
    SCH_SHEET* root = convertRawDesign( design, *schematic );

    const SCH_TEXT* title = nullptr;

    for( const SCH_ITEM* item : root->GetScreen()->Items().OfType( SCH_TEXT_T ) )
    {
        BOOST_REQUIRE( !title );
        title = static_cast<const SCH_TEXT*>( item );
    }

    BOOST_REQUIRE( title );
    BOOST_CHECK_EQUAL( title->GetTextHeight(), schIUScale.mmToIU( 8.70 ) );
    BOOST_CHECK_EQUAL( title->GetPosition().y,
                       OrcadDbuToIu( 100, 100 ).y + KiROUND( title->GetTextHeight() * 0.675 ) );
}


BOOST_AUTO_TEST_CASE( FixedPitchPageGraphicUsesCharacterCellWidthForWrapping )
{
    ORCAD_GRAPHIC_INST comment;
    comment.typeId = ORCAD_ST_GRAPHIC_COMMENT_TEXT_INST;
    comment.nested = std::make_unique<ORCAD_SYMBOL_DEF>();
    comment.nested->primitives.push_back( ORCAD_PRIMITIVE{ .kind = ORCAD_PRIM_KIND::TEXT,
                                                           .x1 = 177,
                                                           .y1 = 48,
                                                           .x2 = 604,
                                                           .y2 = 80,
                                                           .text = "POWER SUPPLY REGULATOR",
                                                           .fontIdx = 1 } );

    ORCAD_RAW_PAGE page;
    page.name = "FIXED PITCH PAGE TEXT";
    page.graphics.push_back( std::move( comment ) );

    ORCAD_DESIGN design;
    design.sourceId = "fixed-pitch-page-text";
    design.library.fonts = {
        ORCAD_FONT{ .height = -32, .pitchAndFamily = 0x31, .face = "Courier New" }
    };
    design.pages.push_back( std::move( page ) );

    SETTINGS_MANAGER manager;
    manager.LoadProject( "" );
    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    schematic->SetProject( &manager.Prj() );
    SCH_SHEET* root = convertRawDesign( design, *schematic );

    BOOST_CHECK( root->GetScreen()->Items().OfType( SCH_TEXTBOX_T ).empty() );

    const SCH_TEXT* title = nullptr;

    for( const SCH_ITEM* item : root->GetScreen()->Items().OfType( SCH_TEXT_T ) )
        title = static_cast<const SCH_TEXT*>( item );

    BOOST_REQUIRE( title );
    BOOST_CHECK_EQUAL( title->GetText(), wxString( "POWER SUPPLY REGULATOR" ) );
}


BOOST_AUTO_TEST_CASE( DefaultPageGraphicColorIsBlack )
{
    ORCAD_GRAPHIC_INST graphic;
    graphic.typeId = ORCAD_ST_GRAPHIC_ELLIPSE_INST;
    graphic.color = 48;
    graphic.nested = std::make_unique<ORCAD_SYMBOL_DEF>();
    graphic.nested->primitives.push_back(
            ORCAD_PRIMITIVE{ .kind = ORCAD_PRIM_KIND::ELLIPSE, .x1 = 10, .y1 = 20, .x2 = 50, .y2 = 40 } );

    ORCAD_RAW_PAGE page;
    page.name = "DEFAULT GRAPHIC COLOR";
    page.graphics.push_back( std::move( graphic ) );

    ORCAD_DESIGN design;
    design.sourceId = "default-page-graphic-color";
    design.pages.push_back( std::move( page ) );

    SETTINGS_MANAGER manager;
    manager.LoadProject( "" );
    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    schematic->SetProject( &manager.Prj() );
    SCH_SHEET* root = convertRawDesign( design, *schematic );

    std::vector<SCH_SHAPE*> shapes;

    for( SCH_ITEM* item : root->GetScreen()->Items().OfType( SCH_SHAPE_T ) )
        shapes.push_back( static_cast<SCH_SHAPE*>( item ) );

    BOOST_REQUIRE_EQUAL( shapes.size(), 1u );
    BOOST_CHECK( shapes.front()->GetStroke().GetColor() == KIGFX::COLOR4D( 0.0, 0.0, 0.0, 1.0 ) );
}


BOOST_AUTO_TEST_CASE( EmbeddedEmfOleFrameDefaultsToRed )
{
    ORCAD_GRAPHIC_INST graphic;
    graphic.typeId = ORCAD_ST_GRAPHIC_OLE_INST;
    graphic.color = 48;
    graphic.nested = std::make_unique<ORCAD_SYMBOL_DEF>();
    graphic.nested->primitives.push_back( ORCAD_PRIMITIVE{ .kind = ORCAD_PRIM_KIND::IMAGE,
                                                            .x1 = 10,
                                                            .y1 = 20,
                                                            .x2 = 50,
                                                            .y2 = 40,
                                                            .data = makeOleWmfPreview(
                                                                    makeEmbeddedEmfWmf( makeRenderableEmf() ) ) } );

    ORCAD_RAW_PAGE page;
    page.name = "DEFAULT OLE FRAME COLOR";
    page.graphics.push_back( std::move( graphic ) );

    ORCAD_DESIGN design;
    design.sourceId = "default-ole-frame-color";
    design.pages.push_back( std::move( page ) );

    SETTINGS_MANAGER manager;
    manager.LoadProject( "" );
    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    schematic->SetProject( &manager.Prj() );
    SCH_SHEET* root = convertRawDesign( design, *schematic );

    auto bitmaps = root->GetScreen()->Items().OfType( SCH_BITMAP_T );
    BOOST_REQUIRE_EQUAL( std::distance( bitmaps.begin(), bitmaps.end() ), 1 );

    std::vector<SCH_SHAPE*> shapes;

    for( SCH_ITEM* item : root->GetScreen()->Items().OfType( SCH_SHAPE_T ) )
        shapes.push_back( static_cast<SCH_SHAPE*>( item ) );

    BOOST_REQUIRE_EQUAL( shapes.size(), 1u );
    BOOST_CHECK( shapes.front()->GetStroke().GetColor() == OrcadColor( 8 ) );
}


BOOST_AUTO_TEST_CASE( PlainWmfOleFrameDefaultsToBlack )
{
    ORCAD_GRAPHIC_INST graphic;
    graphic.typeId = ORCAD_ST_GRAPHIC_OLE_INST;
    graphic.color = 48;
    graphic.nested = std::make_unique<ORCAD_SYMBOL_DEF>();
    graphic.nested->primitives.push_back( ORCAD_PRIMITIVE{ .kind = ORCAD_PRIM_KIND::IMAGE,
                                                            .x1 = 10,
                                                            .y1 = 20,
                                                            .x2 = 50,
                                                            .y2 = 40,
                                                            .data = makeOleWmfPreview( makeGlyphIndexWmf() ) } );

    ORCAD_RAW_PAGE page;
    page.name = "DEFAULT OLE FRAME COLOR";
    page.graphics.push_back( std::move( graphic ) );

    ORCAD_DESIGN design;
    design.sourceId = "default-ole-frame-color";
    design.pages.push_back( std::move( page ) );

    SETTINGS_MANAGER manager;
    manager.LoadProject( "" );
    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    schematic->SetProject( &manager.Prj() );
    SCH_SHEET* root = convertRawDesign( design, *schematic );

    auto bitmaps = root->GetScreen()->Items().OfType( SCH_BITMAP_T );
    BOOST_REQUIRE_EQUAL( std::distance( bitmaps.begin(), bitmaps.end() ), 1 );

    std::vector<SCH_SHAPE*> shapes;

    for( SCH_ITEM* item : root->GetScreen()->Items().OfType( SCH_SHAPE_T ) )
        shapes.push_back( static_cast<SCH_SHAPE*>( item ) );

    BOOST_REQUIRE_EQUAL( shapes.size(), 1u );
    BOOST_CHECK( shapes.front()->GetStroke().GetColor() == KIGFX::COLOR4D( 0.0, 0.0, 0.0, 1.0 ) );
}


BOOST_AUTO_TEST_CASE( WhiteFilledPageGraphicPrintsBlack )
{
    ORCAD_GRAPHIC_INST graphic;
    graphic.typeId = ORCAD_ST_GRAPHIC_ELLIPSE_INST;
    graphic.color = 47;
    graphic.nested = std::make_unique<ORCAD_SYMBOL_DEF>();
    graphic.nested->primitives.push_back( ORCAD_PRIMITIVE{ .kind = ORCAD_PRIM_KIND::ELLIPSE,
                                                            .x1 = 10,
                                                            .y1 = 20,
                                                            .x2 = 50,
                                                            .y2 = 40,
                                                            .fillStyle = 0 } );

    ORCAD_RAW_PAGE page;
    page.name = "WHITE FILLED GRAPHIC";
    page.graphics.push_back( std::move( graphic ) );

    ORCAD_DESIGN design;
    design.sourceId = "white-filled-page-graphic";
    design.pages.push_back( std::move( page ) );

    SETTINGS_MANAGER manager;
    manager.LoadProject( "" );
    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    schematic->SetProject( &manager.Prj() );
    SCH_SHEET* root = convertRawDesign( design, *schematic );

    std::vector<SCH_SHAPE*> shapes;

    for( SCH_ITEM* item : root->GetScreen()->Items().OfType( SCH_SHAPE_T ) )
        shapes.push_back( static_cast<SCH_SHAPE*>( item ) );

    BOOST_REQUIRE_EQUAL( shapes.size(), 1u );
    BOOST_CHECK( shapes.front()->GetFillMode() == FILL_T::FILLED_SHAPE );
    BOOST_CHECK( shapes.front()->GetStroke().GetColor() == KIGFX::COLOR4D( 0.0, 0.0, 0.0, 1.0 ) );
}


BOOST_AUTO_TEST_CASE( DegeneratePageArcDoesNotBecomeFullEllipse )
{
    ORCAD_GRAPHIC_INST graphic;
    graphic.typeId = ORCAD_ST_GRAPHIC_ARC_INST;
    graphic.nested = std::make_unique<ORCAD_SYMBOL_DEF>();
    graphic.nested->primitives.push_back( ORCAD_PRIMITIVE{ .kind = ORCAD_PRIM_KIND::ARC,
                                                            .x1 = -20,
                                                            .y1 = 0,
                                                            .x2 = 20,
                                                            .y2 = 40,
                                                            .start = ORCAD_POINT{ 0, 0 },
                                                            .end = ORCAD_POINT{ 0, 0 } } );

    ORCAD_RAW_PAGE page;
    page.name = "DEGENERATE PAGE ARC";
    page.graphics.push_back( std::move( graphic ) );

    ORCAD_DESIGN design;
    design.sourceId = "degenerate-page-arc";
    design.pages.push_back( std::move( page ) );

    SETTINGS_MANAGER manager;
    manager.LoadProject( "" );
    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    schematic->SetProject( &manager.Prj() );
    SCH_SHEET* root = convertRawDesign( design, *schematic );

    BOOST_CHECK( root->GetScreen()->Items().OfType( SCH_SHAPE_T ).empty() );
}


BOOST_AUTO_TEST_CASE( DesignTemplateFontIdsResolveToLogfonts )
{
    ORCAD_SYMBOL_DEF definition;
    definition.typeId = ORCAD_ST_LIBRARY_PART;
    definition.name = "MAPPED_FONT.Normal";
    definition.bbox = ORCAD_BBOX{ 0, 0, 20, 20 };

    ORCAD_PLACED_INSTANCE placed;
    placed.pkgName = definition.name;
    placed.reference = "C1";
    placed.value = "10nF";
    placed.x = 100;
    placed.y = 100;
    placed.displayProps = {
        ORCAD_DISPLAY_PROP{ .name = "Part Reference", .dispMode = 0x101 },
        ORCAD_DISPLAY_PROP{ .name = "Value", .y = 20, .dispMode = 0x101 },
    };

    ORCAD_RAW_PAGE page;
    page.name = "MAPPED FONT";
    page.instances.push_back( std::move( placed ) );
    ORCAD_WIRE wire{ .id = 1, .x1 = 50, .y1 = 50, .x2 = 150, .y2 = 50 };
    wire.aliases.push_back( ORCAD_ALIAS{ .name = "FONT_NET", .x = 50, .y = 50, .fontIdx = 0x20650200 } );
    page.netmap[wire.id] = "FONT_NET";
    page.wires.push_back( std::move( wire ) );
    ORCAD_GRAPHIC_INST comment;
    comment.typeId = ORCAD_ST_GRAPHIC_COMMENT_TEXT_INST;
    comment.nested = std::make_unique<ORCAD_SYMBOL_DEF>();
    comment.nested->primitives.push_back( ORCAD_PRIMITIVE{
            .kind = ORCAD_PRIM_KIND::TEXT, .x1 = 10, .y1 = 10, .text = "RAW_FONT", .fontIdx = 2 } );
    page.graphics.push_back( std::move( comment ) );

    ORCAD_DESIGN design;
    design.sourceId = "design-template-font-map";
    design.library.fonts = { ORCAD_FONT{ .height = -9, .face = "Arial" },
                             ORCAD_FONT{ .height = -9, .face = "Courier New" },
                             ORCAD_FONT{ .height = -12, .face = "Arial Narrow", .bold = true } };
    design.library.templateFonts.resize( 24, 1 );
    design.library.templateFonts[2] = 3;
    design.library.templateFonts[5] = 3;
    design.library.templateFonts[9] = 3;
    design.symbols.emplace( definition.name, std::move( definition ) );
    design.pages.push_back( std::move( page ) );

    SETTINGS_MANAGER manager;
    manager.LoadProject( "" );
    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    schematic->SetProject( &manager.Prj() );
    SCH_SHEET*     root = convertRawDesign( design, *schematic );
    SCH_SHEET_PATH path;
    path.push_back( root );
    SCH_SYMBOL* converted = findConvertedSymbol( *root->GetScreen(), path, wxS( "C1" ) );
    BOOST_REQUIRE( converted );
    SCH_FIELD* reference = converted->GetField( FIELD_T::REFERENCE );
    BOOST_REQUIRE( reference );
    BOOST_REQUIRE( reference->GetFont() );
    BOOST_CHECK_EQUAL( reference->GetFont()->GetName(), wxS( "Arial Narrow" ) );
    BOOST_CHECK( reference->IsBold() );
    BOOST_CHECK_EQUAL( reference->GetTextHeight(), schIUScale.mmToIU( 1.98 ) );
    VECTOR2I pageOffset = converted->GetPosition() - OrcadDbuToIu( placed.x, placed.y );
    BOOST_CHECK_EQUAL( reference->GetPosition().y,
                       OrcadDbuToIu( placed.x, placed.y ).y + pageOffset.y
                               + KiROUND( reference->GetTextHeight() * 0.62 ) );

    SCH_LABEL* netLabel = nullptr;

    for( SCH_ITEM* item : root->GetScreen()->Items().OfType( SCH_LABEL_T ) )
    {
        SCH_LABEL* label = static_cast<SCH_LABEL*>( item );

        if( label->GetText() == wxS( "FONT_NET" ) && label->GetFont() )
            netLabel = label;
    }

    BOOST_REQUIRE( netLabel );
    BOOST_REQUIRE( netLabel->GetFont() );
    BOOST_CHECK_EQUAL( netLabel->GetFont()->GetName(), wxS( "Arial Narrow" ) );
    BOOST_CHECK( netLabel->IsBold() );
    BOOST_CHECK_EQUAL( netLabel->GetTextHeight(), schIUScale.mmToIU( 1.98 ) );

    const SCH_TEXT* rawText = nullptr;

    for( const SCH_ITEM* item : root->GetScreen()->Items() )
    {
        if( item->Type() == SCH_TEXT_T && static_cast<const SCH_TEXT*>( item )->GetText() == wxS( "RAW_FONT" ) )
            rawText = static_cast<const SCH_TEXT*>( item );
    }

    BOOST_REQUIRE( rawText );
    BOOST_REQUIRE( rawText->GetFont() );
    BOOST_CHECK_EQUAL( rawText->GetFont()->GetName(), wxS( "Courier New" ) );
    BOOST_CHECK( !rawText->IsBold() );
}


BOOST_AUTO_TEST_CASE( ExplicitDisplayFontBypassesTemplateMapping )
{
    ORCAD_SYMBOL_DEF definition;
    definition.typeId = ORCAD_ST_LIBRARY_PART;
    definition.name = "EXPLICIT_FONT.Normal";
    definition.bbox = ORCAD_BBOX{ 0, 0, 20, 20 };

    ORCAD_PLACED_INSTANCE placed;
    placed.pkgName = definition.name;
    placed.reference = "U1";
    placed.value = "LARGE";
    placed.x = 100;
    placed.y = 100;
    placed.displayProps = {
        ORCAD_DISPLAY_PROP{ .name = "Part Reference", .fontIdx = 2, .dispMode = 0x100 },
        ORCAD_DISPLAY_PROP{ .name = "Value", .y = 20, .fontIdx = 2, .dispMode = 0x100 },
    };

    ORCAD_RAW_PAGE page;
    page.name = "EXPLICIT FONT";
    page.instances.push_back( std::move( placed ) );

    ORCAD_DESIGN design;
    design.sourceId = "explicit-display-font";
    design.library.fonts = { ORCAD_FONT{ .height = -9, .face = "Arial Narrow" },
                             ORCAD_FONT{ .height = -20, .face = "Arial Narrow", .bold = true } };
    design.library.templateFonts.resize( 12, 1 );
    design.symbols.emplace( definition.name, std::move( definition ) );
    design.pages.push_back( std::move( page ) );

    SETTINGS_MANAGER manager;
    manager.LoadProject( "" );
    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    schematic->SetProject( &manager.Prj() );
    SCH_SHEET*     root = convertRawDesign( design, *schematic );
    SCH_SHEET_PATH path;
    path.push_back( root );
    SCH_SYMBOL* converted = findConvertedSymbol( *root->GetScreen(), path, wxS( "U1" ) );
    BOOST_REQUIRE( converted );

    for( SCH_FIELD* field : { converted->GetField( FIELD_T::REFERENCE ), converted->GetField( FIELD_T::VALUE ) } )
    {
        BOOST_REQUIRE( field );
        BOOST_CHECK_EQUAL( field->GetTextHeight(), schIUScale.mmToIU( 3.31 ) );
        BOOST_CHECK( field->IsBold() );
    }
}


BOOST_AUTO_TEST_CASE( BoxedSymbolTextRemainsCenteredInItsSourceBounds )
{
    ORCAD_SYMBOL_DEF definition;
    definition.typeId = ORCAD_ST_LIBRARY_PART;
    definition.name = "BOXED_TEXT.Normal";
    definition.bbox = ORCAD_BBOX{ 0, 0, 100, 20 };
    definition.primitives.push_back( ORCAD_PRIMITIVE{ .kind = ORCAD_PRIM_KIND::RECT,
                                                       .x1 = 0,
                                                       .y1 = 0,
                                                       .x2 = 100,
                                                       .y2 = 20 } );
    definition.primitives.push_back( ORCAD_PRIMITIVE{ .kind = ORCAD_PRIM_KIND::TEXT,
                                                       .x1 = 0,
                                                       .y1 = 0,
                                                       .x2 = 100,
                                                       .y2 = 20,
                                                       .text = "CENTERED",
                                                       .fontIdx = 1 } );

    ORCAD_PLACED_INSTANCE placed;
    placed.pkgName = definition.name;
    placed.reference = "LB1";
    placed.x = 100;
    placed.y = 100;

    ORCAD_RAW_PAGE page;
    page.name = "BOXED TEXT";
    page.instances.push_back( std::move( placed ) );

    ORCAD_DESIGN design;
    design.sourceId = "boxed-symbol-text";
    design.library.fonts = {
        ORCAD_FONT{ .height = -20, .width = 11, .pitchAndFamily = 0x31, .face = "Courier New" }
    };
    design.symbols.emplace( definition.name, std::move( definition ) );
    design.pages.push_back( std::move( page ) );

    SETTINGS_MANAGER manager;
    manager.LoadProject( "" );
    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    schematic->SetProject( &manager.Prj() );
    SCH_SHEET*     root = convertRawDesign( design, *schematic );
    SCH_SHEET_PATH path;
    path.push_back( root );
    SCH_SYMBOL* converted = findConvertedSymbol( *root->GetScreen(), path, wxS( "LB1" ) );
    BOOST_REQUIRE( converted );

    const SCH_SHAPE* rectangle = nullptr;
    const SCH_TEXT*  text = nullptr;

    for( const SCH_ITEM& item : converted->GetLibSymbolRef()->GetDrawItems() )
    {
        if( item.Type() == SCH_SHAPE_T )
            rectangle = static_cast<const SCH_SHAPE*>( &item );
        else if( item.Type() == SCH_TEXT_T && static_cast<const SCH_TEXT*>( &item )->GetText() == wxS( "CENTERED" ) )
            text = static_cast<const SCH_TEXT*>( &item );
    }

    BOOST_REQUIRE( rectangle );
    BOOST_REQUIRE( text );
    VECTOR2I target( 50 * ORCAD_IU_PER_DBU, 10 * ORCAD_IU_PER_DBU + 3 * ORCAD_IU_PER_DBU / 2 );
    BOX2I    glyphBox = text->GetEffectiveTextShape( false )->BBox();
    BOOST_CHECK_EQUAL( glyphBox.Centre().x, target.x );
    BOOST_CHECK_EQUAL( glyphBox.Centre().y, target.y );
    BOOST_CHECK_LE( std::abs( glyphBox.GetWidth() - 89 * ORCAD_IU_PER_DBU ), ORCAD_IU_PER_DBU / 4 );
    BOOST_CHECK_LE( std::abs( glyphBox.GetHeight() - 14 * ORCAD_IU_PER_DBU ), ORCAD_IU_PER_DBU / 4 );
}


BOOST_AUTO_TEST_CASE( BoxedMultilineSymbolTextRemainsLeftAligned )
{
    ORCAD_SYMBOL_DEF definition;
    definition.typeId = ORCAD_ST_LIBRARY_PART;
    definition.name = "MULTILINE_TEXT.Normal";
    definition.bbox = ORCAD_BBOX{ 0, 0, 110, 104 };
    definition.primitives.push_back( ORCAD_PRIMITIVE{ .kind = ORCAD_PRIM_KIND::TEXT,
                                                       .x1 = 0,
                                                       .y1 = 60,
                                                       .x2 = 110,
                                                       .y2 = 104,
                                                       .text = "DEVICE\nVERSION : 1\nPAGE : 1 of 1\nDATE : TODAY\n",
                                                       .fontIdx = 1 } );

    ORCAD_PLACED_INSTANCE placed;
    placed.pkgName = definition.name;
    placed.reference = "U1";
    placed.x = 100;
    placed.y = 100;

    ORCAD_RAW_PAGE page;
    page.name = "MULTILINE TEXT";
    page.instances.push_back( std::move( placed ) );

    ORCAD_DESIGN design;
    design.sourceId = "multiline-symbol-text";
    design.library.fonts = { ORCAD_FONT{ .height = -11, .face = "Arial" } };
    design.symbols.emplace( definition.name, std::move( definition ) );
    design.pages.push_back( std::move( page ) );

    SETTINGS_MANAGER manager;
    manager.LoadProject( "" );
    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    schematic->SetProject( &manager.Prj() );
    SCH_SHEET*     root = convertRawDesign( design, *schematic );
    SCH_SHEET_PATH path;
    path.push_back( root );
    SCH_SYMBOL* converted = findConvertedSymbol( *root->GetScreen(), path, wxS( "U1" ) );
    BOOST_REQUIRE( converted );

    const SCH_TEXT* text = nullptr;

    for( const SCH_ITEM& item : converted->GetLibSymbolRef()->GetDrawItems() )
    {
        if( item.Type() == SCH_TEXT_T && static_cast<const SCH_TEXT&>( item ).GetText().StartsWith( wxS( "DEVICE" ) ) )
            text = static_cast<const SCH_TEXT*>( &item );
    }

    BOOST_REQUIRE( text );
    BOOST_CHECK( text->GetHorizJustify() == GR_TEXT_H_ALIGN_LEFT );
    BOOST_CHECK( text->GetVertJustify() == GR_TEXT_V_ALIGN_CENTER );
    BOOST_CHECK_EQUAL( text->GetPosition().x, 0 );
    BOOST_CHECK_EQUAL( text->GetEffectiveTextShape( false )->BBox().Centre().y, 82 * ORCAD_IU_PER_DBU );
}


BOOST_AUTO_TEST_CASE( LaterBoxStrokeOccludesSymbolTextUnderscores )
{
    ORCAD_SYMBOL_DEF definition;
    definition.typeId = ORCAD_ST_LIBRARY_PART;
    definition.name = "OCCLUDED_UNDERSCORES.Normal";
    definition.bbox = ORCAD_BBOX{ 0, 0, 100, 20 };
    definition.primitives.push_back( ORCAD_PRIMITIVE{ .kind = ORCAD_PRIM_KIND::TEXT,
                                                       .x1 = 0,
                                                       .y1 = 0,
                                                       .x2 = 80,
                                                       .y2 = 19,
                                                       .text = "ISO_EVB_LABEL",
                                                       .fontIdx = 1 } );
    definition.primitives.push_back( ORCAD_PRIMITIVE{ .kind = ORCAD_PRIM_KIND::RECT,
                                                       .x1 = 0,
                                                       .y1 = 0,
                                                       .x2 = 100,
                                                       .y2 = 20,
                                                       .lineWidth = 2 } );

    ORCAD_PLACED_INSTANCE placed;
    placed.pkgName = definition.name;
    placed.reference = "LB1";

    ORCAD_RAW_PAGE page;
    page.name = "OCCLUDED UNDERSCORES";
    page.instances.push_back( std::move( placed ) );

    ORCAD_DESIGN design;
    design.sourceId = "occluded-symbol-text-underscores";
    design.library.fonts = {
        ORCAD_FONT{ .height = -20, .width = 11, .pitchAndFamily = 0x31, .face = "Courier New" }
    };
    design.symbols.emplace( definition.name, std::move( definition ) );
    design.pages.push_back( std::move( page ) );

    SETTINGS_MANAGER manager;
    manager.LoadProject( "" );
    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    schematic->SetProject( &manager.Prj() );
    SCH_SHEET*     root = convertRawDesign( design, *schematic );
    SCH_SHEET_PATH path;
    path.push_back( root );
    SCH_SYMBOL* converted = findConvertedSymbol( *root->GetScreen(), path, wxS( "LB1" ) );
    BOOST_REQUIRE( converted );

    const SCH_TEXT* primitiveText = nullptr;

    for( const SCH_ITEM& item : converted->GetLibSymbolRef()->GetDrawItems() )
    {
        if( item.Type() == SCH_TEXT_T )
            primitiveText = static_cast<const SCH_TEXT*>( &item );
    }

    BOOST_REQUIRE( primitiveText );
    BOOST_CHECK_EQUAL( primitiveText->GetText(), wxS( "ISO EVB LABEL" ) );
}


BOOST_AUTO_TEST_CASE( RotatedSymbolPreservesPrimitiveTextOrientation )
{
    ORCAD_SYMBOL_DEF definition;
    definition.typeId = ORCAD_ST_LIBRARY_PART;
    definition.name = "VERTICAL_TEXT.Normal";
    definition.bbox = ORCAD_BBOX{ 0, 0, 30, 20 };
    definition.primitives.push_back( ORCAD_PRIMITIVE{ .kind = ORCAD_PRIM_KIND::TEXT,
                                                       .x2 = 9,
                                                       .y2 = 3,
                                                       .text = "0603",
                                                       .fontIdx = 1 } );

    ORCAD_PLACED_INSTANCE placed;
    placed.pkgName = definition.name;
    placed.reference = "D1";
    placed.x = 100;
    placed.y = 100;
    placed.rotation = 1;

    ORCAD_RAW_PAGE page;
    page.name = "ROTATED SYMBOL TEXT";
    page.instances.push_back( std::move( placed ) );

    ORCAD_DESIGN design;
    design.sourceId = "rotated-symbol-text";
    design.library.fonts = { ORCAD_FONT{ .height = -9, .face = "Courier New" } };
    design.symbols.emplace( definition.name, std::move( definition ) );
    design.pages.push_back( std::move( page ) );

    SETTINGS_MANAGER manager;
    manager.LoadProject( "" );
    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    schematic->SetProject( &manager.Prj() );
    SCH_SHEET*     root = convertRawDesign( design, *schematic );
    SCH_SHEET_PATH path;
    path.push_back( root );
    SCH_SYMBOL* converted = findConvertedSymbol( *root->GetScreen(), path, wxS( "D1" ) );
    BOOST_REQUIRE( converted );

    const SCH_TEXT* primitiveText = nullptr;

    for( const SCH_ITEM& item : converted->GetLibSymbolRef()->GetDrawItems() )
    {
        if( item.Type() == SCH_TEXT_T && static_cast<const SCH_TEXT&>( item ).GetText() == wxS( "0603" ) )
            primitiveText = static_cast<const SCH_TEXT*>( &item );
    }

    BOOST_REQUIRE( primitiveText );
    BOOST_CHECK( primitiveText->GetDrawRotation() == ANGLE_HORIZONTAL );
}


BOOST_AUTO_TEST_CASE( BoxedSymbolTextUsesFontEscapement )
{
    ORCAD_SYMBOL_DEF definition;
    definition.typeId = ORCAD_ST_LIBRARY_PART;
    definition.name = "VERTICAL_BOXED_TEXT.Normal";
    definition.bbox = ORCAD_BBOX{ 0, 0, 20, 100 };
    definition.primitives.push_back( ORCAD_PRIMITIVE{ .kind = ORCAD_PRIM_KIND::TEXT,
                                                       .x1 = 0,
                                                       .y1 = 0,
                                                       .x2 = 20,
                                                       .y2 = 100,
                                                       .text = "ISOLATION BARRIER",
                                                       .fontIdx = 1 } );

    ORCAD_PLACED_INSTANCE placed;
    placed.pkgName = definition.name;
    placed.reference = "U1";
    placed.x = 100;
    placed.y = 100;

    ORCAD_RAW_PAGE page;
    page.name = "VERTICAL BOXED TEXT";
    page.instances.push_back( std::move( placed ) );

    ORCAD_DESIGN design;
    design.sourceId = "vertical-boxed-symbol-text";
    design.library.fonts = { ORCAD_FONT{ .height = -17, .width = 7, .escapement = 900, .face = "Arial" } };
    design.symbols.emplace( definition.name, std::move( definition ) );
    design.pages.push_back( std::move( page ) );

    SETTINGS_MANAGER manager;
    manager.LoadProject( "" );
    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    schematic->SetProject( &manager.Prj() );
    SCH_SHEET*     root = convertRawDesign( design, *schematic );
    SCH_SHEET_PATH path;
    path.push_back( root );
    SCH_SYMBOL* converted = findConvertedSymbol( *root->GetScreen(), path, wxS( "U1" ) );
    BOOST_REQUIRE( converted );

    const SCH_TEXT* primitiveText = nullptr;

    for( const SCH_ITEM& item : converted->GetLibSymbolRef()->GetDrawItems() )
    {
        if( item.Type() == SCH_TEXT_T
            && static_cast<const SCH_TEXT&>( item ).GetText() == wxS( "ISOLATION BARRIER" ) )
        {
            primitiveText = static_cast<const SCH_TEXT*>( &item );
        }
    }

    BOOST_REQUIRE( primitiveText );
    BOOST_CHECK( primitiveText->GetDrawRotation().IsVertical() );
    BOOST_CHECK_LE( std::abs( primitiveText->GetEffectiveTextShape( false )->BBox().Centre().x
                             - 10 * ORCAD_IU_PER_DBU ),
                    ORCAD_IU_PER_DBU / 4 );
    BOOST_CHECK_LE( std::abs( primitiveText->GetEffectiveTextShape( false )->BBox().Centre().y
                             - 50 * ORCAD_IU_PER_DBU ),
                    ORCAD_IU_PER_DBU / 4 );
}


BOOST_AUTO_TEST_CASE( SymbolTextDoesNotStealConnectedContactCircle )
{
    ORCAD_SYMBOL_DEF definition;
    definition.typeId = ORCAD_ST_LIBRARY_PART;
    definition.name = "SWITCH_CONTACT_TEXT.Normal";
    definition.bbox = ORCAD_BBOX{ 0, 0, 70, 160 };
    definition.primitives.push_back( ORCAD_PRIMITIVE{ .kind = ORCAD_PRIM_KIND::LINE,
                                                       .x1 = 0,
                                                       .y1 = 100,
                                                       .x2 = 50,
                                                       .y2 = 100 } );
    definition.primitives.push_back( ORCAD_PRIMITIVE{ .kind = ORCAD_PRIM_KIND::ELLIPSE,
                                                       .x1 = 49,
                                                       .y1 = 99,
                                                       .x2 = 51,
                                                       .y2 = 101 } );
    definition.primitives.push_back( ORCAD_PRIMITIVE{ .kind = ORCAD_PRIM_KIND::TEXT,
                                                       .x1 = 50,
                                                       .y1 = 90,
                                                       .x2 = 57,
                                                       .y2 = 99,
                                                       .text = "0",
                                                       .fontIdx = 1 } );

    ORCAD_PLACED_INSTANCE placed;
    placed.pkgName = definition.name;
    placed.reference = "SW1";
    placed.x = 100;
    placed.y = 100;

    ORCAD_RAW_PAGE page;
    page.name = "SWITCH CONTACT TEXT";
    page.instances.push_back( std::move( placed ) );

    ORCAD_DESIGN design;
    design.sourceId = "switch-contact-text";
    design.library.fonts = { ORCAD_FONT{ .height = -9, .face = "Arial" } };
    design.symbols.emplace( definition.name, std::move( definition ) );
    design.pages.push_back( std::move( page ) );

    SETTINGS_MANAGER manager;
    manager.LoadProject( "" );
    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    schematic->SetProject( &manager.Prj() );
    SCH_SHEET*     root = convertRawDesign( design, *schematic );
    SCH_SHEET_PATH path;
    path.push_back( root );
    SCH_SYMBOL* converted = findConvertedSymbol( *root->GetScreen(), path, wxS( "SW1" ) );
    BOOST_REQUIRE( converted );

    const SCH_SHAPE* contact = nullptr;

    for( const SCH_ITEM& item : converted->GetLibSymbolRef()->GetDrawItems() )
    {
        if( item.Type() == SCH_SHAPE_T && static_cast<const SCH_SHAPE&>( item ).GetShape() == SHAPE_T::CIRCLE )
            contact = static_cast<const SCH_SHAPE*>( &item );
    }

    BOOST_REQUIRE( contact );
    BOOST_CHECK_EQUAL( contact->GetPosition().x, 50 * ORCAD_IU_PER_DBU );
    BOOST_CHECK_EQUAL( contact->GetPosition().y, 100 * ORCAD_IU_PER_DBU );
}


BOOST_AUTO_TEST_CASE( DegenerateSymbolArcDoesNotBecomeFullEllipse )
{
    ORCAD_SYMBOL_DEF definition;
    definition.typeId = ORCAD_ST_LIBRARY_PART;
    definition.name = "DEGENERATE_ARC.Normal";
    definition.bbox = ORCAD_BBOX{ -20, 0, 20, 40 };
    definition.primitives.push_back( ORCAD_PRIMITIVE{ .kind = ORCAD_PRIM_KIND::ARC,
                                                       .x1 = -20,
                                                       .y1 = 0,
                                                       .x2 = 20,
                                                       .y2 = 40,
                                                       .start = ORCAD_POINT{ 0, 0 },
                                                       .end = ORCAD_POINT{ 0, 0 } } );

    ORCAD_PLACED_INSTANCE placed;
    placed.pkgName = definition.name;
    placed.reference = "A1";
    placed.x = 100;
    placed.y = 100;

    ORCAD_RAW_PAGE page;
    page.name = "DEGENERATE ARC";
    page.instances.push_back( std::move( placed ) );

    ORCAD_DESIGN design;
    design.sourceId = "degenerate-symbol-arc";
    design.symbols.emplace( definition.name, std::move( definition ) );
    design.pages.push_back( std::move( page ) );

    SETTINGS_MANAGER manager;
    manager.LoadProject( "" );
    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    schematic->SetProject( &manager.Prj() );
    SCH_SHEET*     root = convertRawDesign( design, *schematic );
    SCH_SHEET_PATH path;
    path.push_back( root );
    SCH_SYMBOL* converted = findConvertedSymbol( *root->GetScreen(), path, wxS( "A1" ) );
    BOOST_REQUIRE( converted );

    for( const SCH_ITEM& item : converted->GetLibSymbolRef()->GetDrawItems() )
        BOOST_CHECK( item.Type() != SCH_SHAPE_T || static_cast<const SCH_SHAPE&>( item ).GetShape() != SHAPE_T::POLY );
}


BOOST_AUTO_TEST_CASE( ZeroLengthPinRetainsNativeNameAndNumberData )
{
    ORCAD_SYMBOL_DEF definition;
    definition.typeId = ORCAD_ST_LIBRARY_PART;
    definition.name = "HIDDEN_PIN.Normal";
    definition.bbox = ORCAD_BBOX{ 0, 0, 100, 20 };
    definition.generalFlags = 3;
    definition.primitives.push_back(
            ORCAD_PRIMITIVE{ .kind = ORCAD_PRIM_KIND::RECT, .x1 = 0, .y1 = 0, .x2 = 40, .y2 = 20 } );
    definition.pins.push_back( ORCAD_SYMBOL_PIN{ .name = "LABEL",
                                                 .position = 0,
                                                 .startX = 0,
                                                 .startY = 10,
                                                 .hotptX = 0,
                                                 .hotptY = 10,
                                                 .portType = ORCAD_PORT_TYPE::POWER_IN,
                                                 .shapeBits = 0 } );
    definition.pins.push_back( ORCAD_SYMBOL_PIN{ .name = "RIGHT",
                                                 .position = 1,
                                                 .startX = 40,
                                                 .startY = 10,
                                                 .hotptX = 40,
                                                 .hotptY = 10,
                                                 .portType = ORCAD_PORT_TYPE::POWER_IN,
                                                 .shapeBits = 0 } );

    ORCAD_PLACED_INSTANCE placed;
    placed.pkgName = definition.name;
    placed.reference = "LB1";
    placed.x = 100;
    placed.y = 100;
    placed.pins.push_back( ORCAD_PIN_INST{ .pinIndex = 1, .x = 100, .y = 110 } );
    placed.pins.push_back( ORCAD_PIN_INST{ .pinIndex = 2, .x = 140, .y = 110 } );

    ORCAD_RAW_PAGE page;
    page.name = "HIDDEN PIN";
    page.instances.push_back( std::move( placed ) );

    ORCAD_DESIGN design;
    design.sourceId = "zero-length-pin-text";
    design.symbols.emplace( definition.name, std::move( definition ) );
    design.pages.push_back( std::move( page ) );

    SETTINGS_MANAGER manager;
    manager.LoadProject( "" );
    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    schematic->SetProject( &manager.Prj() );
    SCH_SHEET*     root = convertRawDesign( design, *schematic );
    SCH_SHEET_PATH path;
    path.push_back( root );
    SCH_SYMBOL* converted = findConvertedSymbol( *root->GetScreen(), path, wxS( "LB1" ) );
    BOOST_REQUIRE( converted );
    BOOST_REQUIRE_EQUAL( converted->GetPins().size(), 2u );

    std::map<wxString, wxString> pinNames;

    for( const SCH_PIN* pin : converted->GetPins() )
    {
        BOOST_CHECK( pin->IsVisible() );
        BOOST_CHECK_EQUAL( pin->GetLength(), 0 );
        BOOST_CHECK_GT( pin->GetNameTextSize(), 0 );
        BOOST_CHECK_GT( pin->GetNumberTextSize(), 0 );
        pinNames.emplace( pin->GetNumber(), pin->GetName() );
    }

    BOOST_CHECK_EQUAL( pinNames[wxS( "1" )], wxS( "LABEL" ) );
    BOOST_CHECK_EQUAL( pinNames[wxS( "2" )], wxS( "RIGHT" ) );

    for( const SCH_ITEM& item : converted->GetLibSymbolRef()->GetDrawItems() )
    {
        if( item.Type() == SCH_TEXT_T )
        {
            const wxString& text = static_cast<const SCH_TEXT&>( item ).GetText();
            BOOST_CHECK( text != wxS( "LABEL" ) && text != wxS( "RIGHT" )
                         && text != wxS( "1" ) && text != wxS( "2" ) );
        }
    }
}


BOOST_AUTO_TEST_CASE( PowerStylePinRemainsVisible )
{
    ORCAD_SYMBOL_DEF definition;
    definition.typeId = ORCAD_ST_LIBRARY_PART;
    definition.name = "POWER_STYLE_PIN.Normal";
    definition.bbox = ORCAD_BBOX{ 0, 0, 100, 20 };
    definition.generalFlags = 3;
    definition.pins.push_back( ORCAD_SYMBOL_PIN{ .name = "1",
                                                 .position = 0,
                                                 .startX = 10,
                                                 .startY = 10,
                                                 .hotptX = 10,
                                                 .hotptY = 20,
                                                 .portType = ORCAD_PORT_TYPE::PASSIVE,
                                                 .shapeBits = 0x81 } );

    ORCAD_PLACED_INSTANCE placed;
    placed.pkgName = definition.name;
    placed.reference = "J1";
    placed.x = 100;
    placed.y = 100;
    placed.pins.push_back( ORCAD_PIN_INST{ .pinIndex = 1, .x = 110, .y = 120 } );

    ORCAD_RAW_PAGE page;
    page.name = "POWER STYLE PIN";
    page.instances.push_back( std::move( placed ) );

    ORCAD_DESIGN design;
    design.sourceId = "power-style-pin";
    design.symbols.emplace( definition.name, std::move( definition ) );
    design.pages.push_back( std::move( page ) );

    SETTINGS_MANAGER manager;
    manager.LoadProject( "" );
    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    schematic->SetProject( &manager.Prj() );
    SCH_SHEET*     root = convertRawDesign( design, *schematic );
    SCH_SHEET_PATH path;
    path.push_back( root );
    SCH_SYMBOL* converted = findConvertedSymbol( *root->GetScreen(), path, wxS( "J1" ) );
    BOOST_REQUIRE( converted );
    BOOST_REQUIRE_EQUAL( converted->GetPins().size(), 1u );
    const SCH_PIN* pin = converted->GetPins().front();
    BOOST_CHECK( pin->IsVisible() );
    BOOST_CHECK_EQUAL( pin->GetName(), wxS( "1" ) );
    BOOST_CHECK_EQUAL( pin->GetNumber(), wxS( "1" ) );
    BOOST_CHECK_GT( pin->GetNameTextSize(), 0 );
    BOOST_CHECK_GT( pin->GetNumberTextSize(), 0 );

    for( const SCH_ITEM& item : converted->GetLibSymbolRef()->GetDrawItems() )
    {
        if( item.Type() == SCH_TEXT_T && static_cast<const SCH_TEXT&>( item ).GetText() == wxS( "1" ) )
            BOOST_ERROR( "Pin data duplicated as SCH_TEXT" );
    }
}


BOOST_AUTO_TEST_CASE( ZeroLengthPowerStylePinHidesImplicitText )
{
    ORCAD_SYMBOL_DEF definition;
    definition.typeId = ORCAD_ST_LIBRARY_PART;
    definition.name = "ZERO_LENGTH_POWER_STYLE_PIN.Normal";
    definition.bbox = ORCAD_BBOX{ 0, 0, 100, 20 };
    definition.generalFlags = 3;
    definition.pins.push_back( ORCAD_SYMBOL_PIN{ .name = "GND",
                                                 .position = 0,
                                                 .startX = 10,
                                                 .startY = 10,
                                                 .hotptX = 10,
                                                 .hotptY = 10,
                                                 .portType = ORCAD_PORT_TYPE::POWER_IN,
                                                 .shapeBits = 0x81 } );

    ORCAD_PLACED_INSTANCE placed;
    placed.pkgName = definition.name;
    placed.reference = "J1";
    placed.x = 100;
    placed.y = 100;
    placed.pins.push_back( ORCAD_PIN_INST{ .pinIndex = 1, .x = 110, .y = 110 } );

    ORCAD_RAW_PAGE page;
    page.name = "ZERO LENGTH POWER STYLE PIN";
    page.instances.push_back( std::move( placed ) );

    ORCAD_DESIGN design;
    design.sourceId = "zero-length-power-style-pin";
    design.symbols.emplace( definition.name, std::move( definition ) );
    design.pages.push_back( std::move( page ) );

    SETTINGS_MANAGER manager;
    manager.LoadProject( "" );
    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    schematic->SetProject( &manager.Prj() );
    SCH_SHEET*     root = convertRawDesign( design, *schematic );
    SCH_SHEET_PATH path;
    path.push_back( root );
    SCH_SYMBOL* converted = findConvertedSymbol( *root->GetScreen(), path, wxS( "J1" ) );
    BOOST_REQUIRE( converted );
    BOOST_REQUIRE_EQUAL( converted->GetPins().size(), 1u );
    BOOST_CHECK( !converted->GetPins().front()->IsVisible() );

    for( const SCH_ITEM& item : converted->GetLibSymbolRef()->GetDrawItems() )
        BOOST_CHECK( item.Type() != SCH_TEXT_T );
}


BOOST_AUTO_TEST_CASE( InputPinUsesCaptureBodyWedge )
{
    ORCAD_SYMBOL_DEF definition;
    definition.typeId = ORCAD_ST_LIBRARY_PART;
    definition.name = "INPUT_WEDGE.Normal";
    definition.bbox = ORCAD_BBOX{ 0, 0, 40, 20 };
    definition.pins = {
        ORCAD_SYMBOL_PIN{ .name = "IN", .position = 0, .startX = 0, .startY = 10,
                          .hotptX = -30, .hotptY = 10, .portType = ORCAD_PORT_TYPE::INPUT,
                          .shapeBits = 0x21 },
        ORCAD_SYMBOL_PIN{ .name = "OUT", .position = 1, .startX = 40, .startY = 10,
                          .hotptX = 70, .hotptY = 10, .portType = ORCAD_PORT_TYPE::OUTPUT,
                          .shapeBits = 0x21 },
    };

    ORCAD_PLACED_INSTANCE placed;
    placed.pkgName = definition.name;
    placed.reference = "U1";
    placed.x = 100;
    placed.y = 100;
    placed.pins = { ORCAD_PIN_INST{ .pinIndex = 1, .x = 70, .y = 110 },
                    ORCAD_PIN_INST{ .pinIndex = 2, .x = 170, .y = 110 } };

    ORCAD_RAW_PAGE page;
    page.name = "INPUT WEDGE";
    page.instances.push_back( std::move( placed ) );

    ORCAD_DESIGN design;
    design.sourceId = "input-wedge";
    design.symbols.emplace( definition.name, std::move( definition ) );
    design.pages.push_back( std::move( page ) );

    SETTINGS_MANAGER manager;
    manager.LoadProject( "" );
    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    schematic->SetProject( &manager.Prj() );
    SCH_SHEET*     root = convertRawDesign( design, *schematic );
    SCH_SHEET_PATH path;
    path.push_back( root );
    SCH_SYMBOL* converted = findConvertedSymbol( *root->GetScreen(), path, wxS( "U1" ) );
    BOOST_REQUIRE( converted );

    std::vector<const SCH_SHAPE*> filledPolygons;

    for( const SCH_ITEM& item : converted->GetLibSymbolRef()->GetDrawItems() )
    {
        if( item.Type() != SCH_SHAPE_T )
            continue;

        const SCH_SHAPE& shape = static_cast<const SCH_SHAPE&>( item );

        if( shape.GetShape() == SHAPE_T::POLY && shape.GetFillMode() == FILL_T::FILLED_SHAPE )
            filledPolygons.push_back( &shape );
    }

    BOOST_REQUIRE_EQUAL( filledPolygons.size(), 1u );
    const std::vector<VECTOR2I>& points = filledPolygons.front()->GetPolyPoints();
    BOOST_REQUIRE_EQUAL( points.size(), 4u );
    BOOST_CHECK( points.front() == OrcadDbuToIu( 0, 10 ) );
    BOOST_CHECK( points.back() == points.front() );
}


BOOST_AUTO_TEST_CASE( HorizontalPinNumberRemainsNativePinData )
{
    ORCAD_SYMBOL_DEF definition;
    definition.typeId = ORCAD_ST_LIBRARY_PART;
    definition.name = "HORIZONTAL_PIN_NUMBER.Normal";
    definition.bbox = ORCAD_BBOX{ 0, 0, 20, 80 };
    definition.generalFlags = 3;
    definition.pins.push_back( ORCAD_SYMBOL_PIN{ .name = "3",
                                                 .position = 0,
                                                 .startX = 20,
                                                 .startY = 40,
                                                 .hotptX = 50,
                                                 .hotptY = 40,
                                                 .portType = ORCAD_PORT_TYPE::PASSIVE,
                                                 .shapeBits = 1 } );

    ORCAD_PLACED_INSTANCE placed;
    placed.pkgName = definition.name;
    placed.reference = "J1";
    placed.x = 100;
    placed.y = 100;
    placed.pins.push_back( ORCAD_PIN_INST{ .pinIndex = 1, .x = 150, .y = 140 } );

    ORCAD_RAW_PAGE page;
    page.name = "HORIZONTAL PIN NUMBER";
    page.instances.push_back( std::move( placed ) );

    ORCAD_DESIGN design;
    design.sourceId = "horizontal-pin-number";
    design.symbols.emplace( definition.name, std::move( definition ) );
    design.pages.push_back( std::move( page ) );

    SETTINGS_MANAGER manager;
    manager.LoadProject( "" );
    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    schematic->SetProject( &manager.Prj() );
    SCH_SHEET*     root = convertRawDesign( design, *schematic );
    SCH_SHEET_PATH path;
    path.push_back( root );
    SCH_SYMBOL* converted = findConvertedSymbol( *root->GetScreen(), path, wxS( "J1" ) );
    BOOST_REQUIRE( converted );

    BOOST_REQUIRE_EQUAL( converted->GetPins().size(), 1u );
    const SCH_PIN* pin = converted->GetPins().front();
    BOOST_CHECK_EQUAL( pin->GetName(), wxS( "3" ) );
    BOOST_CHECK_EQUAL( pin->GetNumber(), wxS( "1" ) );
    BOOST_CHECK_GT( pin->GetNameTextSize(), 0 );
    BOOST_CHECK_GT( pin->GetNumberTextSize(), 0 );

    for( const SCH_ITEM& item : converted->GetLibSymbolRef()->GetDrawItems() )
    {
        if( item.Type() == SCH_TEXT_T && static_cast<const SCH_TEXT&>( item ).GetText() == pin->GetNumber() )
            BOOST_ERROR( "Pin number duplicated as SCH_TEXT" );
    }
}


BOOST_AUTO_TEST_CASE( DisplayedImplementationPropertyIsPreserved )
{
    ORCAD_SYMBOL_DEF definition;
    definition.typeId = ORCAD_ST_LIBRARY_PART;
    definition.name = "PSPICE_TRANSISTOR.Normal";
    definition.bbox = ORCAD_BBOX{ 0, 0, 20, 20 };

    ORCAD_PLACED_INSTANCE placed;
    placed.pkgName = definition.name;
    placed.reference = "Q1";
    placed.value = "MMBT3904";
    placed.x = 100;
    placed.y = 100;
    placed.displayProps = {
        ORCAD_DISPLAY_PROP{ .name = "Part Reference", .dispMode = 0x101 },
        ORCAD_DISPLAY_PROP{ .name = "Implementation", .x = -10, .y = -30, .dispMode = 0x101 },
    };

    ORCAD_RAW_PAGE page;
    page.name = "DISPLAYED IMPLEMENTATION";
    page.instances.push_back( std::move( placed ) );

    ORCAD_DESIGN design;
    design.sourceId = "displayed-implementation";
    design.library.fonts = { ORCAD_FONT{ .height = -9, .face = "Arial" } };
    design.symbols.emplace( definition.name, std::move( definition ) );
    design.pages.push_back( std::move( page ) );

    SETTINGS_MANAGER manager;
    manager.LoadProject( "" );
    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    schematic->SetProject( &manager.Prj() );
    SCH_SHEET*     root = convertRawDesign( design, *schematic );
    SCH_SHEET_PATH path;
    path.push_back( root );
    SCH_SYMBOL* converted = findConvertedSymbol( *root->GetScreen(), path, wxS( "Q1" ) );
    BOOST_REQUIRE( converted );
    SCH_FIELD* implementation = converted->GetField( wxS( "Implementation" ) );
    BOOST_REQUIRE( implementation );
    BOOST_CHECK_EQUAL( implementation->GetText(), wxS( "MMBT3904" ) );
    BOOST_CHECK( implementation->IsVisible() );
    VECTOR2I pageOffset = converted->GetPosition() - OrcadDbuToIu( 100, 100 );
    BOOST_CHECK( implementation->GetPosition()
                 == OrcadDbuToIu( 90, 70 ) + pageOffset
                            + VECTOR2I( 0, OrcadTextBaselineOffset( implementation->GetTextSize().y ) ) );
}


BOOST_AUTO_TEST_CASE( DisplayedPropertyNameUsesCaptureEqualsSeparator )
{
    ORCAD_SYMBOL_DEF definition;
    definition.typeId = ORCAD_ST_LIBRARY_PART;
    definition.name = "VPULSE.Normal";
    definition.bbox = ORCAD_BBOX{ 0, 0, 20, 20 };

    ORCAD_PLACED_INSTANCE placed;
    placed.pkgName = definition.name;
    placed.reference = "V1";
    placed.x = 100;
    placed.y = 100;
    placed.props["V1"] = "0";
    placed.displayProps = {
        ORCAD_DISPLAY_PROP{ .name = "Part Reference", .dispMode = 0x101 },
        ORCAD_DISPLAY_PROP{ .name = "V1", .x = -50, .y = -8, .dispMode = 0x201 },
    };

    ORCAD_RAW_PAGE page;
    page.name = "DISPLAYED PROPERTY NAME";
    page.instances.push_back( std::move( placed ) );

    ORCAD_DESIGN design;
    design.sourceId = "displayed-property-name";
    design.symbols.emplace( definition.name, std::move( definition ) );
    design.pages.push_back( std::move( page ) );

    SETTINGS_MANAGER manager;
    manager.LoadProject( "" );
    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    schematic->SetProject( &manager.Prj() );
    SCH_SHEET*     root = convertRawDesign( design, *schematic );
    SCH_SHEET_PATH path;
    path.push_back( root );
    SCH_SYMBOL* converted = findConvertedSymbol( *root->GetScreen(), path, wxS( "V1" ) );
    BOOST_REQUIRE( converted );
    SCH_FIELD* parameter = converted->GetField( wxS( "V1" ) );
    BOOST_REQUIRE( parameter );
    BOOST_CHECK_EQUAL( parameter->GetText(), wxS( "0" ) );
    BOOST_CHECK( !parameter->IsVisible() );

    const SCH_TEXT* display = nullptr;

    for( SCH_ITEM* item : root->GetScreen()->Items().OfType( SCH_TEXT_T ) )
    {
        SCH_TEXT* text = static_cast<SCH_TEXT*>( item );

        if( text->GetText() == wxS( "V1 = 0" ) )
            display = text;
    }

    BOOST_REQUIRE( display );
}


BOOST_AUTO_TEST_CASE( DisplayedValueNameUsesCaptureEqualsSeparator )
{
    ORCAD_SYMBOL_DEF definition;
    definition.typeId = ORCAD_ST_LIBRARY_PART;
    definition.name = "RES_27.Normal";
    definition.bbox = ORCAD_BBOX{ 0, 0, 20, 20 };

    ORCAD_PLACED_INSTANCE placed;
    placed.pkgName = definition.name;
    placed.reference = "R1";
    placed.value = "27";
    placed.x = 100;
    placed.y = 100;
    placed.displayProps = {
        ORCAD_DISPLAY_PROP{ .name = "Part Reference", .dispMode = 0x101 },
        ORCAD_DISPLAY_PROP{ .name = "Value", .x = -50, .y = -8, .dispMode = 0x201 },
    };

    ORCAD_RAW_PAGE page;
    page.name = "DISPLAYED VALUE NAME";
    page.instances.push_back( std::move( placed ) );

    ORCAD_DESIGN design;
    design.sourceId = "displayed-value-name";
    design.symbols.emplace( definition.name, std::move( definition ) );
    design.pages.push_back( std::move( page ) );

    SETTINGS_MANAGER manager;
    manager.LoadProject( "" );
    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    schematic->SetProject( &manager.Prj() );
    SCH_SHEET*     root = convertRawDesign( design, *schematic );
    SCH_SHEET_PATH path;
    path.push_back( root );
    SCH_SYMBOL* converted = findConvertedSymbol( *root->GetScreen(), path, wxS( "R1" ) );
    BOOST_REQUIRE( converted );
    SCH_FIELD* value = converted->GetField( FIELD_T::VALUE );
    BOOST_REQUIRE( value );
    BOOST_CHECK_EQUAL( value->GetText(), wxS( "27" ) );
    BOOST_CHECK( !value->IsVisible() );

    const SCH_TEXT* display = nullptr;

    for( SCH_ITEM* item : root->GetScreen()->Items().OfType( SCH_TEXT_T ) )
    {
        SCH_TEXT* text = static_cast<SCH_TEXT*>( item );

        if( text->GetText() == wxS( "Value = 27" ) )
            display = text;
    }

    BOOST_REQUIRE( display );
}


BOOST_AUTO_TEST_CASE( HiddenImplementationPathPropertyIsPreserved )
{
    ORCAD_SYMBOL_DEF definition;
    definition.typeId = ORCAD_ST_LIBRARY_PART;
    definition.name = "IMPLEMENTATION_PATH.Normal";
    definition.bbox = ORCAD_BBOX{ 0, 0, 20, 20 };
    definition.props["Implementation Path"] = "LIBRARY/PART";

    ORCAD_PACKAGE package;
    package.name = "IMPLEMENTATION_PATH";
    package.props["Implementation Path"] = "";
    package.devices.push_back( ORCAD_DEVICE{} );

    ORCAD_PLACED_INSTANCE placed;
    placed.pkgName = definition.name;
    placed.reference = "U1";
    placed.x = 100;
    placed.y = 100;

    ORCAD_RAW_PAGE page;
    page.name = "HIDDEN IMPLEMENTATION PATH";
    page.instances.push_back( std::move( placed ) );

    ORCAD_DESIGN design;
    design.sourceId = "hidden-implementation-path";
    design.symbols.emplace( definition.name, std::move( definition ) );
    design.packages.emplace( package.name, std::move( package ) );
    design.pages.push_back( std::move( page ) );

    SETTINGS_MANAGER manager;
    manager.LoadProject( "" );
    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    schematic->SetProject( &manager.Prj() );
    SCH_SHEET*     root = convertRawDesign( design, *schematic );
    SCH_SHEET_PATH path;
    path.push_back( root );
    SCH_SYMBOL* converted = findConvertedSymbol( *root->GetScreen(), path, wxS( "U1" ) );
    BOOST_REQUIRE( converted );
    SCH_FIELD* implementationPath = converted->GetField( wxS( "Implementation Path" ) );
    BOOST_REQUIRE( implementationPath );
    BOOST_CHECK_EQUAL( implementationPath->GetText(), wxS( "LIBRARY/PART" ) );
    BOOST_CHECK( !implementationPath->IsVisible() );
}


BOOST_AUTO_TEST_CASE( OccurrencePropertiesOverrideReusablePartFields )
{
    ORCAD_SYMBOL_DEF definition;
    definition.typeId = ORCAD_ST_LIBRARY_PART;
    definition.name = "SWITCH.Normal";
    definition.bbox = ORCAD_BBOX{ 0, 0, 20, 20 };
    definition.props["Manufacturer"] = "Panasonic Electronic Components";

    ORCAD_PACKAGE package;
    package.name = "SWITCH";
    package.pcbFootprint = "EVQ-PE105K";
    package.devices.push_back( ORCAD_DEVICE{} );

    ORCAD_PLACED_INSTANCE replaced;
    replaced.dbId = 42;
    replaced.pkgName = definition.name;
    replaced.sourcePackage = package.name;
    replaced.reference = "SW1";
    replaced.value = "EVQ-PE105K";
    replaced.x = 100;
    replaced.y = 100;
    replaced.displayProps = {
        ORCAD_DISPLAY_PROP{ .name = "Part Reference", .dispMode = 0x101 },
        ORCAD_DISPLAY_PROP{ .name = "Value", .y = 10, .dispMode = 0x101 },
        ORCAD_DISPLAY_PROP{ .name = "PCB Footprint", .y = 20, .dispMode = 0x101 },
        ORCAD_DISPLAY_PROP{ .name = "Manufacturer", .y = 30, .dispMode = 0x101 },
    };

    ORCAD_PLACED_INSTANCE cleared = replaced;
    cleared.dbId = 43;
    cleared.reference = "SW2";
    cleared.x = 200;

    ORCAD_RAW_PAGE page;
    page.name = "OCCURRENCE PROPERTIES";
    page.instances = { replaced, cleared };

    ORCAD_DESIGN design;
    design.sourceId = "occurrence-properties";
    design.symbols.emplace( definition.name, std::move( definition ) );
    design.packages.emplace( package.name, std::move( package ) );
    design.pages.push_back( std::move( page ) );
    design.occurrenceRoot.partProps[42] = {
        { "Value", "434 123 050 816" },
        { "Manufacturer", "Wurth Electronics Inc" },
        { "PCB Footprint", "434 123 050 816" },
    };
    design.occurrenceRoot.partProps[43] = {
        { "Value", "" },
        { "Manufacturer", "" },
        { "PCB Footprint", "" },
    };

    SETTINGS_MANAGER manager;
    manager.LoadProject( "" );
    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    schematic->SetProject( &manager.Prj() );
    SCH_SHEET* root = convertRawDesign( design, *schematic );
    SCH_SHEET_PATH path;
    path.push_back( root );

    SCH_SYMBOL* replacedSymbol = findConvertedSymbol( *root->GetScreen(), path, wxS( "SW1" ) );
    BOOST_REQUIRE( replacedSymbol );
    BOOST_CHECK_EQUAL( replacedSymbol->GetField( FIELD_T::VALUE )->GetText(), wxS( "434 123 050 816" ) );
    BOOST_REQUIRE( replacedSymbol->GetField( wxS( "Manufacturer" ) ) );
    BOOST_CHECK_EQUAL( replacedSymbol->GetField( wxS( "Manufacturer" ) )->GetText(),
                       wxS( "Wurth Electronics Inc" ) );
    BOOST_REQUIRE( replacedSymbol->GetField( wxS( "OrCAD Footprint" ) ) );
    BOOST_CHECK_EQUAL( replacedSymbol->GetField( wxS( "OrCAD Footprint" ) )->GetText(),
                       wxS( "434 123 050 816" ) );

    SCH_SYMBOL* clearedSymbol = findConvertedSymbol( *root->GetScreen(), path, wxS( "SW2" ) );
    BOOST_REQUIRE( clearedSymbol );
    BOOST_CHECK( clearedSymbol->GetField( FIELD_T::VALUE )->GetText().IsEmpty() );
    BOOST_CHECK( !clearedSymbol->GetField( wxS( "Manufacturer" ) ) );
    BOOST_CHECK( !clearedSymbol->GetField( wxS( "OrCAD Footprint" ) ) );
}


BOOST_AUTO_TEST_CASE( UnrelatedOccurrenceNetNamesAreIgnored )
{
    ORCAD_RAW_PAGE page;
    page.name = "OCCURRENCE NETS";
    page.netmap[11643] = "LED1_R";

    ORCAD_WIRE named;
    named.dbId = 10396;
    named.id = 11643;
    named.x2 = 100;
    page.wires.push_back( named );

    ORCAD_WIRE generated;
    generated.dbId = 6941990;
    generated.id = 20000;
    generated.y1 = 100;
    generated.x2 = 100;
    generated.y2 = 100;
    page.wires.push_back( generated );

    ORCAD_DESIGN design;
    design.sourceId = "occurrence-net-names";
    design.pages.push_back( std::move( page ) );
    design.occurrenceRoot.netNames[19574] = "LED1_R_G13";
    design.occurrenceRoot.netNames[19575] = "N6941990_DSP_UL,_LL,_UR,_LR_ATDSP0";

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    SCH_SHEET*         root = convertRawDesign( design, *schematic );
    std::set<wxString> labels;

    for( SCH_ITEM* item : root->GetScreen()->Items().OfType( SCH_LABEL_T ) )
        labels.insert( static_cast<SCH_LABEL*>( item )->GetText() );

    BOOST_CHECK( !labels.contains( wxS( "LED1_R_G13" ) ) );
    BOOST_CHECK( !labels.contains( wxS( "N6941990_DSP_UL,_LL,_UR,_LR_ATDSP0" ) ) );
}


BOOST_AUTO_TEST_CASE( OccurrenceNetPrefixDoesNotRenameInterfaceNet )
{
    ORCAD_RAW_PAGE page;
    page.name = "INTERFACE NETS";
    page.netmap[1] = "WL_REG_ON";

    ORCAD_WIRE wire;
    wire.id = 1;
    wire.x2 = 100;
    page.wires.push_back( wire );

    ORCAD_GRAPHIC_INST offpage;
    offpage.logicalName = "WL_REG_ON";
    page.offpage.push_back( std::move( offpage ) );

    ORCAD_RAW_PAGE peerPage;
    peerPage.name = "PEER";
    ORCAD_GRAPHIC_INST peerOffpage;
    peerOffpage.logicalName = "WL_REG_ON_M2";
    peerPage.offpage.push_back( std::move( peerOffpage ) );

    ORCAD_DESIGN design;
    design.sourceId = "occurrence-interface-prefix";
    design.pages.push_back( std::move( page ) );
    design.pages.push_back( std::move( peerPage ) );
    design.occurrenceRoot.netNames[10] = "BT_REG_ON";
    design.occurrenceRoot.netNames[11] = "WL_REG_ON_M2";

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    SCH_SHEET*         root = convertRawDesign( design, *schematic );
    std::set<wxString> labels;

    for( SCH_ITEM* item : root->GetScreen()->Items().OfType( SCH_GLOBAL_LABEL_T ) )
        labels.insert( static_cast<SCH_GLOBALLABEL*>( item )->GetText() );

    BOOST_CHECK( labels.contains( wxS( "WL_REG_ON" ) ) );
    BOOST_CHECK( !labels.contains( wxS( "WL_REG_ON_M2" ) ) );
}


BOOST_AUTO_TEST_CASE( OccurrenceAliasRenamesInterfaceNet )
{
    ORCAD_RAW_PAGE page;
    page.name = "INTERFACE ALIAS";
    page.netmap[1] = "UART0_CTS";
    page.netAliases[1] = { "TWRPI_GPIO4", "EBI_AD1/PTD5/FTM0_CH5", "UART0_CTS" };

    ORCAD_WIRE wire;
    wire.dbId = 20;
    wire.id = 1;
    wire.x2 = 100;
    ORCAD_ALIAS connectorAlias;
    connectorAlias.name = "TWRPI_GPIO4";
    connectorAlias.x = 10;
    wire.aliases.push_back( std::move( connectorAlias ) );
    ORCAD_ALIAS interfaceAlias;
    interfaceAlias.name = "EBI_AD1/PTD5/FTM0_CH5";
    interfaceAlias.x = 15;
    wire.aliases.push_back( std::move( interfaceAlias ) );
    ORCAD_ALIAS secondaryAlias;
    secondaryAlias.name = "UART0_CTS";
    secondaryAlias.x = 50;
    wire.aliases.push_back( std::move( secondaryAlias ) );
    page.wires.push_back( wire );

    ORCAD_PLACED_INSTANCE connector;
    connector.reference = "J8";
    ORCAD_PIN_INST pin;
    pin.wordA = wire.dbId;
    connector.pins.push_back( pin );
    page.instances.push_back( std::move( connector ) );

    ORCAD_GRAPHIC_INST offpage;
    offpage.logicalName = "EBI_AD1/PTD5/FTM0_CH5";
    offpage.x = 100;
    ORCAD_DISPLAY_PROP displayedName;
    displayedName.name = "Name";
    displayedName.x = -85;
    offpage.displayProps.push_back( std::move( displayedName ) );
    page.offpage.push_back( std::move( offpage ) );

    ORCAD_DESIGN design;
    design.sourceId = "occurrence-interface-alias";
    design.pages.push_back( std::move( page ) );
    design.occurrenceRoot.netNames[10] = "EBI_AD1/PTD5/FTM0_CH5";

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    SCH_SHEET*         root = convertRawDesign( design, *schematic );
    std::set<wxString> labels;

    for( SCH_ITEM* item : root->GetScreen()->Items().OfType( SCH_GLOBAL_LABEL_T ) )
        labels.insert( static_cast<SCH_GLOBALLABEL*>( item )->GetText() );

    BOOST_CHECK( labels.contains( wxS( "TWRPI_GPIO4" ) ) );
}


BOOST_AUTO_TEST_CASE( OffpageDisplayedNamePreservesClockwiseVerticalRotation )
{
    ORCAD_RAW_PAGE page;
    page.name = "VERTICAL OFFPAGE";
    page.netmap[1] = "VBUS_P_CTRL0_CON";

    ORCAD_WIRE wire;
    wire.id = 1;
    wire.x1 = 100;
    wire.y1 = 50;
    wire.x2 = 100;
    wire.y2 = 100;
    page.wires.push_back( wire );

    ORCAD_GRAPHIC_INST offpage;
    offpage.name = "OFFPAGELEFT-L";
    offpage.logicalName = "VBUS_P_CTRL0_CON";
    offpage.x = 100;
    offpage.y = 100;
    offpage.bbox = ORCAD_BBOX{ 90, 90, 110, 110 };
    offpage.displayProps.push_back( ORCAD_DISPLAY_PROP{
            .name = "Name", .x = 4, .y = 10, .rotation = 1, .fontIdx = 1, .dispMode = 0x101 } );
    page.offpage.push_back( std::move( offpage ) );

    ORCAD_DESIGN design;
    design.sourceId = "vertical-offpage-display-name";
    design.library.fonts.push_back( ORCAD_FONT{ .height = -9, .face = "Arial" } );
    design.symbols.emplace( "OFFPAGELEFT-L", ORCAD_SYMBOL_DEF{ .name = "OFFPAGELEFT-L" } );
    design.pages.push_back( std::move( page ) );

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    SCH_SHEET* root = convertRawDesign( design, *schematic );

    const SCH_TEXT* displayedName = nullptr;

    for( const SCH_ITEM* item : root->GetScreen()->Items().OfType( SCH_TEXT_T ) )
    {
        const SCH_TEXT* text = static_cast<const SCH_TEXT*>( item );

        if( text->GetText() == wxS( "VBUS_P_CTRL0_CON" ) )
            displayedName = text;
    }

    BOOST_REQUIRE( displayedName );
    BOOST_CHECK( displayedName->GetTextAngle() == ANGLE_270 );
    BOOST_CHECK_EQUAL( displayedName->GetHorizJustify(), GR_TEXT_H_ALIGN_RIGHT );
}


BOOST_AUTO_TEST_CASE( OffpageIntersheetReferencePreservesStoredDisplay )
{
    ORCAD_RAW_PAGE page;
    page.name = "INTERSHEET REFERENCE";
    page.netmap[1] = "PWR_LED_CTRL";

    ORCAD_WIRE wire;
    wire.id = 1;
    wire.x1 = 50;
    wire.y1 = 100;
    wire.x2 = 100;
    wire.y2 = 100;
    page.wires.push_back( wire );

    ORCAD_GRAPHIC_INST offpage;
    offpage.name = "OFFPAGELEFT-L";
    offpage.logicalName = "PWR_LED_CTRL";
    offpage.x = 100;
    offpage.y = 100;
    offpage.bbox = ORCAD_BBOX{ 90, 90, 110, 110 };
    offpage.props["IREF"] = "[5,20]";
    offpage.displayProps.push_back( ORCAD_DISPLAY_PROP{
            .name = "Name", .x = 4, .y = 5, .fontIdx = 1, .dispMode = 0x101 } );
    offpage.displayProps.push_back( ORCAD_DISPLAY_PROP{
            .name = "IREF", .x = 83, .y = 5, .fontIdx = 1, .dispMode = 0x101 } );
    page.offpage.push_back( std::move( offpage ) );

    ORCAD_DESIGN design;
    design.sourceId = "offpage-intersheet-reference";
    design.library.fonts.push_back( ORCAD_FONT{ .height = -9, .face = "Arial" } );
    design.symbols.emplace( "OFFPAGELEFT-L", ORCAD_SYMBOL_DEF{ .name = "OFFPAGELEFT-L" } );
    design.pages.push_back( std::move( page ) );

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    SCH_SHEET* root = convertRawDesign( design, *schematic );

    const SCH_TEXT* intersheetRef = nullptr;

    for( const SCH_ITEM* item : root->GetScreen()->Items().OfType( SCH_TEXT_T ) )
    {
        const SCH_TEXT* text = static_cast<const SCH_TEXT*>( item );

        if( text->GetText() == wxS( "[5,20]" ) )
            intersheetRef = text;
    }

    BOOST_REQUIRE( intersheetRef );
    BOOST_CHECK( intersheetRef->IsVisible() );
    BOOST_CHECK( intersheetRef->GetTextAngle() == ANGLE_0 );
    BOOST_CHECK_EQUAL( intersheetRef->GetHorizJustify(), GR_TEXT_H_ALIGN_LEFT );
    BOOST_CHECK( intersheetRef->GetTextColor() == OrcadColor( 8 ) );
    int baseX = std::min( design.pages[0].offpage[0].bbox.x1, design.pages[0].offpage[0].bbox.x2 );
    int baseY = std::min( design.pages[0].offpage[0].bbox.y1, design.pages[0].offpage[0].bbox.y2 );
    int baseline = OrcadTextBaselineOffset( intersheetRef->GetTextSize().y )
                   + KiROUND( intersheetRef->GetTextSize().y * 7.0 / 21.0 );
    BOOST_CHECK_EQUAL( intersheetRef->GetPosition().x, OrcadDbuToIu( baseX + 83, 0 ).x );
    BOOST_CHECK_EQUAL( intersheetRef->GetPosition().y, OrcadDbuToIu( 0, baseY + 5 ).y + baseline );
}


BOOST_AUTO_TEST_CASE( HiddenOffpageIntersheetReferenceIsNotRendered )
{
    ORCAD_RAW_PAGE page;
    page.name = "HIDDEN INTERSHEET REFERENCE";
    page.netmap[1] = "SWDIO";
    page.wires.push_back( ORCAD_WIRE{ .id = 1, .x2 = 100, .y2 = 100 } );

    ORCAD_GRAPHIC_INST offpage;
    offpage.name = "OFFPAGELEFT-L";
    offpage.logicalName = "SWDIO";
    offpage.x = 100;
    offpage.y = 100;
    offpage.bbox = ORCAD_BBOX{ 90, 90, 110, 110 };
    offpage.props["IREF"] = "0";
    offpage.displayProps.push_back( ORCAD_DISPLAY_PROP{
            .name = "Name", .x = 4, .y = 5, .fontIdx = 1, .dispMode = 0x101 } );
    offpage.displayProps.push_back( ORCAD_DISPLAY_PROP{
            .name = "IREF", .x = 20, .y = 30, .fontIdx = 1, .dispMode = 0x001 } );
    page.offpage.push_back( std::move( offpage ) );

    ORCAD_DESIGN design;
    design.sourceId = "hidden-offpage-intersheet-reference";
    design.library.fonts.push_back( ORCAD_FONT{ .height = -9, .face = "Arial" } );
    design.symbols.emplace( "OFFPAGELEFT-L", ORCAD_SYMBOL_DEF{ .name = "OFFPAGELEFT-L" } );
    design.pages.push_back( std::move( page ) );

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    SCH_SHEET* root = convertRawDesign( design, *schematic );

    for( const SCH_ITEM* item : root->GetScreen()->Items().OfType( SCH_TEXT_T ) )
        BOOST_CHECK_NE( static_cast<const SCH_TEXT*>( item )->GetText(), wxS( "0" ) );
}


BOOST_AUTO_TEST_CASE( PortRetainsSourceGraphicsAndDisplayedName )
{
    ORCAD_SYMBOL_DEF definition;
    definition.typeId = ORCAD_ST_PORT_SYMBOL;
    definition.name = "Rudy-PortRight";
    definition.bbox = ORCAD_BBOX{ 0, 0, 70, 20 };
    definition.primitives = {
        ORCAD_PRIMITIVE{ .kind = ORCAD_PRIM_KIND::LINE, .x1 = 0, .y1 = 10, .x2 = 10, .y2 = 0 },
        ORCAD_PRIMITIVE{ .kind = ORCAD_PRIM_KIND::LINE, .x1 = 10, .y1 = 0, .x2 = 70, .y2 = 0 },
        ORCAD_PRIMITIVE{ .kind = ORCAD_PRIM_KIND::LINE, .x1 = 70, .y1 = 0, .x2 = 70, .y2 = 20 },
        ORCAD_PRIMITIVE{ .kind = ORCAD_PRIM_KIND::LINE, .x1 = 70, .y1 = 20, .x2 = 10, .y2 = 20 },
        ORCAD_PRIMITIVE{ .kind = ORCAD_PRIM_KIND::LINE, .x1 = 10, .y1 = 20, .x2 = 0, .y2 = 10 },
    };
    definition.pins.push_back( ORCAD_SYMBOL_PIN{ .hotptX = 70, .hotptY = 10 } );

    ORCAD_GRAPHIC_INST port;
    port.name = definition.name;
    port.logicalName = "*SHORT";
    port.bbox = ORCAD_BBOX{ 100, 100, 170, 120 };
    port.displayProps.push_back(
            ORCAD_DISPLAY_PROP{ .name = "Name", .x = 16, .y = 3, .fontIdx = 1, .dispMode = 0x101 } );

    ORCAD_RAW_PAGE page;
    page.name = "SOURCE PORT GRAPHICS";
    page.ports.push_back( std::move( port ) );
    page.wires.push_back( ORCAD_WIRE{ .id = 1, .x1 = 170, .y1 = 110, .x2 = 220, .y2 = 110 } );
    page.netmap[1] = "*SHORT";

    ORCAD_DESIGN design;
    design.sourceId = "source-port-graphics";
    design.library.fonts.push_back( ORCAD_FONT{ .height = -9, .face = "Arial" } );
    design.symbols.emplace( definition.name, std::move( definition ) );
    design.pages.push_back( std::move( page ) );

    SETTINGS_MANAGER manager;
    manager.LoadProject( "" );
    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    schematic->SetProject( &manager.Prj() );
    SCH_SHEET* root = convertRawDesign( design, *schematic );
    BOX2I      graphicsBox;
    size_t     shapeCount = 0;
    const SCH_TEXT* displayedName = nullptr;

    for( const SCH_ITEM* item : root->GetScreen()->Items().OfType( SCH_SHAPE_T ) )
    {
        graphicsBox.Merge( item->GetBoundingBox() );
        ++shapeCount;
    }

    for( const SCH_ITEM* item : root->GetScreen()->Items().OfType( SCH_TEXT_T ) )
    {
        const SCH_TEXT* text = static_cast<const SCH_TEXT*>( item );

        if( text->GetText() == wxS( "*SHORT" ) )
            displayedName = text;
    }

    BOOST_CHECK_EQUAL( shapeCount, 5u );
    BOOST_CHECK_GE( graphicsBox.GetWidth(), OrcadDbuToIu( 70, 0 ).x );
    BOOST_CHECK_LE( graphicsBox.GetWidth(), OrcadDbuToIu( 72, 0 ).x );
    BOOST_REQUIRE( displayedName );
    BOOST_CHECK_EQUAL( displayedName->GetTextHeight(), schIUScale.mmToIU( 1.70 ) );
    BOOST_CHECK( displayedName->IsVisible() );
}


BOOST_AUTO_TEST_CASE( OccurrenceNameCanonicalizesCrossPageInterfaceAliasGroup )
{
    ORCAD_RAW_PAGE sensePage;
    sensePage.name = "SENSE";
    sensePage.netmap[1] = "SENSE-";
    sensePage.netAliases[1] = { "SENSE-", "VOUT" };
    ORCAD_WIRE senseWire;
    senseWire.id = 1;
    senseWire.x2 = 100;
    sensePage.wires.push_back( senseWire );
    ORCAD_GRAPHIC_INST senseOffpage;
    senseOffpage.logicalName = "SENSE-";
    sensePage.offpage.push_back( std::move( senseOffpage ) );

    ORCAD_RAW_PAGE isnPage;
    isnPage.name = "ISN";
    isnPage.netmap[2] = "ISN1_DCR";
    isnPage.netAliases[2] = { "ISN1_DCR", "VOUT" };
    ORCAD_WIRE isnWire;
    isnWire.id = 2;
    isnWire.x2 = 100;
    isnPage.wires.push_back( isnWire );
    ORCAD_GRAPHIC_INST isnOffpage;
    isnOffpage.logicalName = "ISN1_DCR";
    isnPage.offpage.push_back( std::move( isnOffpage ) );

    ORCAD_DESIGN design;
    design.sourceId = "occurrence-cross-page-interface-alias";
    design.pages.push_back( std::move( sensePage ) );
    design.pages.push_back( std::move( isnPage ) );
    design.occurrenceRoot.netNames[10] = "SENSE-";

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    convertRawDesign( design, *schematic );
    std::set<wxString> labels;

    for( const SCH_SHEET_PATH& path : schematic->Hierarchy() )
    {
        for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_GLOBAL_LABEL_T ) )
            labels.insert( static_cast<SCH_GLOBALLABEL*>( item )->GetText() );
    }

    BOOST_CHECK( labels.contains( wxS( "SENSE-" ) ) );
    BOOST_CHECK( !labels.contains( wxS( "ISN1_DCR" ) ) );
}


BOOST_AUTO_TEST_CASE( DistinctOccurrenceInterfaceNamesRemainSeparate )
{
    ORCAD_RAW_PAGE page;
    page.name = "DISTINCT INTERFACES";
    page.netmap[1] = "PCIE_USIM_DATA";
    page.netAliases[1] = { "PCIE_USIM_DATA", "USIM_DATA" };
    ORCAD_WIRE wire;
    wire.id = 1;
    wire.x2 = 100;
    page.wires.push_back( wire );

    ORCAD_GRAPHIC_INST pcieOffpage;
    pcieOffpage.logicalName = "PCIE_USIM_DATA";
    page.offpage.push_back( std::move( pcieOffpage ) );
    ORCAD_GRAPHIC_INST usimOffpage;
    usimOffpage.logicalName = "USIM_DATA";
    page.offpage.push_back( std::move( usimOffpage ) );

    ORCAD_DESIGN design;
    design.sourceId = "distinct-occurrence-interface-names";
    design.pages.push_back( std::move( page ) );
    design.occurrenceRoot.netNames[10] = "PCIE_USIM_DATA";
    design.occurrenceRoot.netNames[11] = "USIM_DATA";

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    SCH_SHEET*         root = convertRawDesign( design, *schematic );
    std::set<wxString> labels;

    for( SCH_ITEM* item : root->GetScreen()->Items().OfType( SCH_GLOBAL_LABEL_T ) )
        labels.insert( static_cast<SCH_GLOBALLABEL*>( item )->GetText() );

    BOOST_CHECK( labels.contains( wxS( "PCIE_USIM_DATA" ) ) );
    BOOST_CHECK( labels.contains( wxS( "USIM_DATA" ) ) );
}


BOOST_AUTO_TEST_CASE( OccurrenceWireNameOverridesOffpageDisplayName )
{
    ORCAD_RAW_PAGE page;
    page.name = "OCCURRENCE OFFPAGE";
    page.netmap[1] = "VOUT";
    page.netAliases[1] = { "VOUT" };
    ORCAD_WIRE wire;
    wire.dbId = 22608001;
    wire.id = 1;
    wire.x2 = 100;
    page.wires.push_back( wire );
    ORCAD_GRAPHIC_INST offpage;
    offpage.logicalName = "VOUT";
    offpage.x = 100;
    page.offpage.push_back( std::move( offpage ) );

    ORCAD_DESIGN design;
    design.sourceId = "occurrence-wire-offpage-name";
    design.pages.push_back( std::move( page ) );
    design.occurrenceRoot.netNames[10] = "VOUT_22608001";

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    SCH_SHEET*         root = convertRawDesign( design, *schematic );
    std::set<wxString> labels;

    for( SCH_ITEM* item : root->GetScreen()->Items().OfType( SCH_GLOBAL_LABEL_T ) )
        labels.insert( static_cast<SCH_GLOBALLABEL*>( item )->GetText() );

    BOOST_CHECK( labels.contains( wxS( "VOUT_22608001" ) ) );
    BOOST_CHECK( !labels.contains( wxS( "VOUT" ) ) );
}


BOOST_AUTO_TEST_CASE( OccurrencePowerNameGlobalizesMatchingWire )
{
    ORCAD_SYMBOL_DEF power;
    power.typeId = ORCAD_ST_GLOBAL_SYMBOL;
    power.name = "VCC_BAR";
    power.pins.push_back( ORCAD_SYMBOL_PIN() );

    ORCAD_GRAPHIC_INST global;
    global.typeId = ORCAD_ST_GLOBAL;
    global.name = power.name;
    global.logicalName = "VDD";
    global.x = 1000;

    ORCAD_RAW_PAGE wirePage;
    wirePage.name = "OCCURRENCE POWER WIRE";
    wirePage.netmap[1] = "VDD";
    ORCAD_WIRE wire;
    wire.id = 1;
    wire.x2 = 100;
    wirePage.wires.push_back( wire );

    ORCAD_RAW_PAGE globalPage;
    globalPage.name = "OCCURRENCE POWER SYMBOL";
    globalPage.globals.push_back( std::move( global ) );

    ORCAD_DESIGN design;
    design.sourceId = "occurrence-power-global-wire";
    design.symbols.emplace( power.name, std::move( power ) );
    design.pages.push_back( std::move( wirePage ) );
    design.pages.push_back( std::move( globalPage ) );
    design.occurrenceRoot.netNames[10] = "VDD";

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    convertRawDesign( design, *schematic );
    int labelCount = 0;

    for( const SCH_SHEET_PATH& path : schematic->Hierarchy() )
    {
        for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_GLOBAL_LABEL_T ) )
        {
            ++labelCount;
            BOOST_CHECK_EQUAL( static_cast<SCH_GLOBALLABEL*>( item )->GetText(), wxS( "VDD" ) );
        }
    }

    BOOST_REQUIRE_EQUAL( labelCount, 1 );
}


BOOST_AUTO_TEST_CASE( PowerSymbolDisplayNameDoesNotOverrideLogicalNet )
{
    ORCAD_SYMBOL_DEF power;
    power.typeId = ORCAD_ST_GLOBAL_SYMBOL;
    power.name = "VCC_CIRCLE";
    power.bbox = ORCAD_BBOX{ 0, 0, 30, 10 };
    ORCAD_SYMBOL_PIN powerPin;
    powerPin.hotptX = 20;
    power.pins.push_back( powerPin );

    ORCAD_GRAPHIC_INST global;
    global.typeId = ORCAD_ST_GLOBAL;
    global.name = power.name;
    global.logicalName = "0";
    global.props["Name"] = "DISPLAY_ONLY";
    global.x = 950;
    global.y = 320;
    global.bbox = ORCAD_BBOX{ 950, 320, 980, 330 };
    global.displayProps.push_back( ORCAD_DISPLAY_PROP{ 0, "NODENAME", -45, 4, 0, 0, 0, 0x101 } );

    ORCAD_GRAPHIC_INST aliasedGlobal;
    aliasedGlobal.typeId = ORCAD_ST_GLOBAL;
    aliasedGlobal.name = power.name;
    aliasedGlobal.logicalName = "VDD3";
    aliasedGlobal.x = 1090;
    aliasedGlobal.y = 320;
    aliasedGlobal.bbox = ORCAD_BBOX{ 1090, 320, 1120, 330 };

    ORCAD_RAW_PAGE page;
    page.name = "POWER";
    ORCAD_WIRE wire;
    wire.id = 1;
    wire.x1 = 960;
    wire.y1 = 270;
    wire.x2 = 960;
    wire.y2 = 250;
    page.wires.push_back( wire );
    page.netmap.emplace( wire.id, "0" );
    ORCAD_WIRE aliasedWire;
    aliasedWire.id = 2;
    aliasedWire.x1 = 1110;
    aliasedWire.y1 = 320;
    aliasedWire.x2 = 1110;
    aliasedWire.y2 = 300;
    page.wires.push_back( aliasedWire );
    page.netmap.emplace( aliasedWire.id, "0" );
    page.netAliases[aliasedWire.id] = { "0", "VDD3" };
    page.globals.push_back( std::move( global ) );
    page.globals.push_back( std::move( aliasedGlobal ) );

    ORCAD_DESIGN design;
    design.sourceId = "power-display-name";
    design.symbols.emplace( power.name, std::move( power ) );
    design.pages.push_back( std::move( page ) );

    SETTINGS_MANAGER manager;
    manager.LoadProject( "" );
    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    schematic->SetProject( &manager.Prj() );
    SCH_SHEET*     root = convertRawDesign( design, *schematic );
    SCH_SHEET_PATH path;
    path.push_back( root );

    SCH_SYMBOL* symbol = findConvertedSymbol( *root->GetScreen(), path, wxS( "#PWR0001" ) );
    BOOST_REQUIRE( symbol );
    BOOST_CHECK_EQUAL( symbol->GetValue( false, &path, false ), wxS( "0" ) );
    BOOST_REQUIRE_EQUAL( symbol->GetPins().size(), 1u );
    BOOST_CHECK_EQUAL( symbol->GetPins().front()->GetName(), wxS( "0" ) );
    BOOST_CHECK( symbol->GetPins().front()->GetPosition() == OrcadDbuToIu( 960, 270 ) );
    SCH_FIELD* value = symbol->GetField( FIELD_T::VALUE );
    BOOST_REQUIRE( value );
    BOOST_CHECK( !value->IsVisible() );
    SCH_FIELD* displayName = symbol->GetField( wxS( "NODENAME" ) );
    BOOST_REQUIRE( displayName );
    BOOST_CHECK_EQUAL( displayName->GetText(), wxS( "DISPLAY_ONLY" ) );
    BOOST_CHECK( displayName->IsVisible() );
    BOOST_CHECK( displayName->GetPosition()
                 == OrcadDbuToIu( 950 - 45, 320 + 4 )
                            + VECTOR2I( 0, OrcadTextBaselineOffset( displayName->GetTextHeight() ) ) );

    SCH_SYMBOL* aliased = nullptr;

    for( SCH_ITEM* item : root->GetScreen()->Items().OfType( SCH_SYMBOL_T ) )
    {
        SCH_SYMBOL* candidate = static_cast<SCH_SYMBOL*>( item );

        if( !candidate->GetPins().empty() && candidate->GetPins().front()->GetPosition() == OrcadDbuToIu( 1110, 320 ) )
        {
            aliased = candidate;
            break;
        }
    }

    BOOST_REQUIRE( aliased );
    BOOST_REQUIRE_EQUAL( aliased->GetPins().size(), 1u );
    BOOST_CHECK_EQUAL( aliased->GetValue( false, &path, false ), wxS( "VDD3" ) );
    BOOST_CHECK_EQUAL( aliased->GetPins().front()->GetName(), wxS( "VDD3" ) );

    schematic->ConnectionGraph()->Recalculate( schematic->BuildSheetListSortedByPageNumbers(), true );

    BOOST_REQUIRE( symbol->GetPins().front()->Connection( &path ) );
    BOOST_REQUIRE( aliased->GetPins().front()->Connection( &path ) );
    BOOST_CHECK_NE( symbol->GetPins().front()->Connection( &path )->Name(),
                    aliased->GetPins().front()->Connection( &path )->Name() );
}


BOOST_AUTO_TEST_CASE( ExplicitPowerStylePinUsesSourceNetInsteadOfImplicitGlobalName )
{
    ORCAD_SYMBOL_DEF part;
    part.typeId = ORCAD_ST_LIBRARY_PART;
    part.name = "LOCAL_POWER.Normal";
    part.bbox = ORCAD_BBOX{ 0, 0, 20, 20 };
    part.pins.push_back( ORCAD_SYMBOL_PIN{ .name = "V-",
                                           .position = 0,
                                           .startX = 10,
                                           .hotptX = 0,
                                           .portType = ORCAD_PORT_TYPE::POWER_IN,
                                           .shapeBits = 0x80 } );

    ORCAD_PLACED_INSTANCE placed;
    placed.pkgName = part.name;
    placed.reference = "U1";
    placed.x = 100;
    placed.y = 100;
    placed.pins.push_back( ORCAD_PIN_INST{ .pinIndex = 0,
                                           .x = 100,
                                           .y = 100,
                                           .wordA = std::numeric_limits<uint32_t>::max(),
                                           .wordB = 1 } );

    ORCAD_RAW_PAGE page;
    page.name = "EXPLICIT HIDDEN POWER PIN";
    page.netmap[1] = "GND";
    page.instances.push_back( std::move( placed ) );

    ORCAD_DESIGN design;
    design.sourceId = "explicit-power-style-pin";
    design.symbols.emplace( part.name, std::move( part ) );
    design.pages.push_back( std::move( page ) );

    SETTINGS_MANAGER manager;
    manager.LoadProject( "" );
    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    schematic->SetProject( &manager.Prj() );
    SCH_SHEET*     root = convertRawDesign( design, *schematic );
    SCH_SHEET_PATH path;
    path.push_back( root );

    SCH_SYMBOL* symbol = findConvertedSymbol( *root->GetScreen(), path, wxS( "U1" ) );
    BOOST_REQUIRE( symbol );
    BOOST_REQUIRE_EQUAL( symbol->GetPins().size(), 1u );
    BOOST_CHECK( symbol->GetPins().front()->IsVisible() );
    BOOST_CHECK( symbol->GetPins().front()->GetType() == ELECTRICAL_PINTYPE::PT_POWER_IN );

    schematic->ConnectionGraph()->Recalculate( schematic->BuildSheetListSortedByPageNumbers(), true );
    BOOST_REQUIRE( symbol->GetPins().front()->Connection( &path ) );
    BOOST_CHECK_EQUAL( symbol->GetPins().front()->Connection( &path )->Name(), wxS( "/GND" ) );
}


BOOST_AUTO_TEST_CASE( PowerSymbolsSharingNetKeepSourceGraphicIdentity )
{
    ORCAD_SYMBOL_DEF ground;
    ground.typeId = ORCAD_ST_GLOBAL_SYMBOL;
    ground.name = "GND_POWER";
    ground.bbox = ORCAD_BBOX{ 0, 0, 30, 10 };
    ORCAD_SYMBOL_PIN groundPin;
    groundPin.hotptX = 20;
    ground.pins.push_back( groundPin );

    ORCAD_SYMBOL_DEF supply = ground;
    supply.name = "VCC_CIRCLE";

    ORCAD_GRAPHIC_INST groundGlobal;
    groundGlobal.typeId = ORCAD_ST_GLOBAL;
    groundGlobal.name = ground.name;
    groundGlobal.logicalName = "0";
    groundGlobal.bbox = ORCAD_BBOX{ 940, 270, 970, 280 };

    ORCAD_GRAPHIC_INST supplyGlobal;
    supplyGlobal.typeId = ORCAD_ST_GLOBAL;
    supplyGlobal.name = supply.name;
    supplyGlobal.logicalName = "VDD3";
    supplyGlobal.bbox = ORCAD_BBOX{ 1090, 320, 1120, 330 };

    ORCAD_RAW_PAGE page;
    page.name = "POWER GRAPHICS";
    ORCAD_WIRE groundWire;
    groundWire.id = 1;
    groundWire.x1 = 960;
    groundWire.y1 = 270;
    groundWire.x2 = 960;
    groundWire.y2 = 250;
    page.wires.push_back( groundWire );
    ORCAD_WIRE supplyWire;
    supplyWire.id = 2;
    supplyWire.x1 = 1110;
    supplyWire.y1 = 320;
    supplyWire.x2 = 1110;
    supplyWire.y2 = 300;
    page.wires.push_back( supplyWire );
    page.netmap.emplace( 1, "0" );
    page.netmap.emplace( 2, "0" );
    page.netAliases[2] = { "0", "VDD3" };
    page.globals.push_back( std::move( groundGlobal ) );
    page.globals.push_back( std::move( supplyGlobal ) );

    ORCAD_DESIGN design;
    design.sourceId = "power-graphic-identity";
    design.symbols.emplace( ground.name, std::move( ground ) );
    design.symbols.emplace( supply.name, std::move( supply ) );
    design.pages.push_back( std::move( page ) );
    design.occurrenceRoot.netNames[2] = "0";

    SETTINGS_MANAGER manager;
    manager.LoadProject( "" );
    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    schematic->SetProject( &manager.Prj() );
    SCH_SHEET*     root = convertRawDesign( design, *schematic );
    SCH_SHEET_PATH path;
    path.push_back( root );

    SCH_SYMBOL* groundSymbol = findConvertedSymbol( *root->GetScreen(), path, wxS( "#PWR0001" ) );
    SCH_SYMBOL* supplySymbol = findConvertedSymbol( *root->GetScreen(), path, wxS( "#PWR0002" ) );
    BOOST_REQUIRE( groundSymbol );
    BOOST_REQUIRE( supplySymbol );
    BOOST_CHECK( groundSymbol->GetLibId() != supplySymbol->GetLibId() );
    BOOST_REQUIRE_EQUAL( groundSymbol->GetPins().size(), 1u );
    BOOST_REQUIRE_EQUAL( supplySymbol->GetPins().size(), 1u );
    BOOST_CHECK_EQUAL( groundSymbol->GetPins().front()->GetName(), wxS( "0" ) );
    BOOST_CHECK_EQUAL( supplySymbol->GetPins().front()->GetName(), wxS( "0" ) );
    BOOST_CHECK_EQUAL( supplySymbol->GetValue( false, &path, false ), wxS( "0" ) );
}


static std::filesystem::path findCorpusDesign( const std::filesystem::path& aRoot, const std::string& aFileName );


BOOST_AUTO_TEST_CASE( OccurrencePowerAliasChainUsesAuthoritativeName )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "DC2331A-4.DSN" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    SCH_SHEET_LIST sheets = schematic->BuildSheetListSortedByPageNumbers();
    schematic->ConnectionGraph()->Recalculate( sheets, true );
    wxString c1Pin1Net;

    for( const auto& [key, subgraphs] : schematic->ConnectionGraph()->GetNetMap() )
    {
        for( CONNECTION_SUBGRAPH* subgraph : subgraphs )
        {
            for( SCH_ITEM* item : subgraph->GetItems() )
            {
                if( item->Type() != SCH_PIN_T )
                    continue;

                SCH_PIN*    pin = static_cast<SCH_PIN*>( item );
                SCH_SYMBOL* symbol = dynamic_cast<SCH_SYMBOL*>( pin->GetParentSymbol() );

                if( symbol && symbol->GetRef( &subgraph->GetSheet(), false ) == wxS( "C1" )
                    && pin->GetNumber() == wxS( "1" ) )
                {
                    c1Pin1Net = key.Name;
                }
            }
        }
    }

    BOOST_CHECK_EQUAL( c1Pin1Net.AfterLast( '/' ), wxS( "GND" ) );
}


BOOST_AUTO_TEST_CASE( OccurrencePowerAliasJoinsDistinctChildNetNames )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "M5275EVB.DSN" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    SCH_SHEET_LIST sheets = schematic->BuildSheetListSortedByPageNumbers();
    schematic->ConnectionGraph()->Recalculate( sheets, true );
    std::map<std::pair<wxString, wxString>, CONNECTION_SUBGRAPH*> pinNets;

    for( const auto& [key, subgraphs] : schematic->ConnectionGraph()->GetNetMap() )
    {
        for( CONNECTION_SUBGRAPH* subgraph : subgraphs )
        {
            if( !subgraph->GetSheet().LastScreen()->GetFileName().Contains( wxS( "P08_PSU" ) ) )
                continue;

            for( SCH_ITEM* item : subgraph->GetItems() )
            {
                if( item->Type() != SCH_PIN_T )
                    continue;

                SCH_PIN*    pin = static_cast<SCH_PIN*>( item );
                SCH_SYMBOL* symbol = dynamic_cast<SCH_SYMBOL*>( pin->GetParentSymbol() );

                if( symbol )
                    pinNets[{ symbol->GetRef( &subgraph->GetSheet(), false ), pin->GetNumber() }] = subgraph;
            }
        }
    }

    BOOST_REQUIRE( pinNets.count( { wxS( "C126" ), wxS( "1" ) } ) );
    BOOST_REQUIRE( pinNets.count( { wxS( "U15" ), wxS( "3" ) } ) );
    BOOST_REQUIRE( pinNets.count( { wxS( "U15" ), wxS( "8" ) } ) );
    BOOST_CHECK_EQUAL( pinNets.at( { wxS( "C126" ), wxS( "1" ) } ),
                       pinNets.at( { wxS( "U15" ), wxS( "3" ) } ) );
    BOOST_CHECK_EQUAL( pinNets.at( { wxS( "U15" ), wxS( "8" ) } ),
                       pinNets.at( { wxS( "U15" ), wxS( "3" ) } ) );
}


BOOST_AUTO_TEST_CASE( LegacyGraphicBoundingBoxUsesDefinitionExtent )
{
    ORCAD_SYMBOL_DEF power;
    power.typeId = ORCAD_ST_GLOBAL_SYMBOL;
    power.name = "VCC_BAR";
    power.bbox = ORCAD_BBOX{ 0, 0, 80, 70 };
    ORCAD_SYMBOL_PIN powerPin;
    powerPin.hotptX = 30;
    powerPin.hotptY = 20;
    power.pins.push_back( powerPin );

    ORCAD_GRAPHIC_INST global;
    global.typeId = ORCAD_ST_GLOBAL;
    global.name = power.name;
    global.logicalName = "12V";
    global.x = 60;
    global.y = 60;
    global.bbox = ORCAD_BBOX{ 1040, 580, 80, 70 };

    ORCAD_RAW_PAGE page;
    page.name = "LEGACY POWER";
    ORCAD_WIRE wire;
    wire.id = 1;
    wire.x1 = 990;
    wire.y1 = 530;
    wire.x2 = 990;
    wire.y2 = 550;
    page.wires.push_back( wire );
    page.netmap.emplace( wire.id, "12V" );
    page.globals.push_back( std::move( global ) );

    ORCAD_DESIGN design;
    design.sourceId = "legacy-power-bbox";
    design.symbols.emplace( power.name, std::move( power ) );
    design.pages.push_back( std::move( page ) );

    SETTINGS_MANAGER manager;
    manager.LoadProject( "" );
    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    schematic->SetProject( &manager.Prj() );
    SCH_SHEET*     root = convertRawDesign( design, *schematic );
    SCH_SHEET_PATH path;
    path.push_back( root );

    SCH_SYMBOL* symbol = findConvertedSymbol( *root->GetScreen(), path, wxS( "#PWR0001" ) );
    BOOST_REQUIRE( symbol );
    BOOST_REQUIRE_EQUAL( symbol->GetPins().size(), 1u );
    VECTOR2I expected = OrcadDbuToIu( 990, 530 );
    BOOST_CHECK_EQUAL( symbol->GetPins().front()->GetPosition().x, expected.x );
    BOOST_CHECK_EQUAL( symbol->GetPins().front()->GetPosition().y, expected.y );
}


BOOST_AUTO_TEST_CASE( NamedPageNetOverridesPowerLogicalName )
{
    ORCAD_SYMBOL_DEF power;
    power.typeId = ORCAD_ST_GLOBAL_SYMBOL;
    power.name = "GND_POWER";
    power.bbox = ORCAD_BBOX{ 0, 0, 30, 10 };
    ORCAD_SYMBOL_PIN powerPin;
    powerPin.hotptX = 20;
    power.pins.push_back( powerPin );

    ORCAD_GRAPHIC_INST global;
    global.typeId = ORCAD_ST_GLOBAL;
    global.name = power.name;
    global.logicalName = "GND";
    global.x = 940;
    global.y = 270;
    global.bbox = ORCAD_BBOX{ 940, 270, 970, 280 };
    ORCAD_RAW_PAGE page;
    page.name = "POWER ALIAS";
    ORCAD_WIRE wire;
    wire.id = 1;
    wire.x1 = 960;
    wire.y1 = 270;
    wire.x2 = 960;
    wire.y2 = 250;
    page.wires.push_back( wire );
    page.netmap.emplace( wire.id, "GND" );
    page.netAliases[wire.id] = { "GND", "VPORTN" };
    page.globals.push_back( std::move( global ) );

    ORCAD_DESIGN design;
    design.sourceId = "power-page-net";
    design.symbols.emplace( power.name, std::move( power ) );
    design.pages.push_back( std::move( page ) );
    design.occurrenceRoot.netNames[2] = "VPORTN";

    SETTINGS_MANAGER manager;
    manager.LoadProject( "" );
    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    schematic->SetProject( &manager.Prj() );
    SCH_SHEET*     root = convertRawDesign( design, *schematic );
    SCH_SHEET_PATH path;
    path.push_back( root );

    SCH_SYMBOL* symbol = findConvertedSymbol( *root->GetScreen(), path, wxS( "#PWR0001" ) );
    BOOST_REQUIRE( symbol );
    BOOST_REQUIRE_EQUAL( symbol->GetPins().size(), 1u );
    BOOST_CHECK_EQUAL( symbol->GetPins().front()->GetName(), wxS( "VPORTN" ) );
}


BOOST_AUTO_TEST_CASE( GlobalConnectorDoesNotPromoteWireAlias )
{
    ORCAD_SYMBOL_DEF power;
    power.typeId = ORCAD_ST_GLOBAL_SYMBOL;
    power.name = "VDD_POWER";
    power.bbox = ORCAD_BBOX{ 0, 0, 30, 10 };
    ORCAD_SYMBOL_PIN powerPin;
    powerPin.hotptX = 20;
    power.pins.push_back( powerPin );

    ORCAD_GRAPHIC_INST global;
    global.typeId = ORCAD_ST_GLOBAL;
    global.dbId = 1;
    global.name = power.name;
    global.logicalName = "VDD";
    global.x = 940;
    global.y = 270;
    global.bbox = ORCAD_BBOX{ 940, 270, 970, 280 };
    ORCAD_GRAPHIC_INST duplicateGlobal;
    duplicateGlobal.typeId = global.typeId;
    duplicateGlobal.dbId = 2;
    duplicateGlobal.name = global.name;
    duplicateGlobal.logicalName = global.logicalName;
    duplicateGlobal.x = global.x;
    duplicateGlobal.y = global.y;
    duplicateGlobal.bbox = global.bbox;

    ORCAD_RAW_PAGE page;
    page.name = "GLOBAL ALIAS";
    ORCAD_WIRE wire;
    wire.id = 1;
    wire.x1 = 960;
    wire.y1 = 270;
    wire.x2 = 960;
    wire.y2 = 250;
    ORCAD_ALIAS alias;
    alias.name = "AUX";
    wire.aliases.push_back( alias );
    page.wires.push_back( wire );
    page.netmap.emplace( wire.id, "AUX" );
    page.netAliases[wire.id] = { "AUX" };
    page.globals.push_back( std::move( global ) );
    page.globals.push_back( std::move( duplicateGlobal ) );

    ORCAD_DESIGN design;
    design.sourceId = "canonical-global-alias";
    design.symbols.emplace( power.name, std::move( power ) );
    design.pages.push_back( std::move( page ) );

    SETTINGS_MANAGER manager;
    manager.LoadProject( "" );
    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    schematic->SetProject( &manager.Prj() );
    SCH_SHEET* root = convertRawDesign( design, *schematic );

    std::set<wxString> globalLabels;
    std::set<wxString> localLabels;

    for( SCH_ITEM* item : root->GetScreen()->Items().OfType( SCH_GLOBAL_LABEL_T ) )
        globalLabels.insert( static_cast<SCH_GLOBALLABEL*>( item )->GetText() );

    for( SCH_ITEM* item : root->GetScreen()->Items().OfType( SCH_LABEL_T ) )
        localLabels.insert( static_cast<SCH_LABEL*>( item )->GetText() );

    BOOST_CHECK( !globalLabels.contains( wxS( "AUX" ) ) );
    BOOST_CHECK( localLabels.contains( wxS( "AUX" ) ) );
}


BOOST_AUTO_TEST_CASE( InterfaceAliasDoesNotRenameConnectorOnDistinctNet )
{
    ORCAD_RAW_PAGE page;
    page.name = "DISTINCT INTERFACE NET";
    page.netmap[1] = "USIM_DATA";
    page.netAliases[1] = { "USIM_DATA", "PCIE_USIM_DATA" };
    page.wires.push_back( ORCAD_WIRE{ .id = 1, .x2 = 100 } );
    page.netmap[2] = "PCIE_USIM_DATA";
    page.netAliases[2] = { "PCIE_USIM_DATA" };
    page.wires.push_back( ORCAD_WIRE{ .id = 2, .x1 = 1000, .x2 = 1100 } );

    ORCAD_GRAPHIC_INST aliased;
    aliased.typeId = ORCAD_ST_OFFPAGE_CONNECTOR;
    aliased.logicalName = "PCIE_USIM_DATA";
    aliased.x = 0;
    aliased.y = 0;
    ORCAD_GRAPHIC_INST distinct;
    distinct.typeId = aliased.typeId;
    distinct.logicalName = aliased.logicalName;
    distinct.x = 1000;
    page.offpage.push_back( std::move( aliased ) );
    page.offpage.push_back( std::move( distinct ) );

    ORCAD_DESIGN design;
    design.sourceId = "distinct-interface-net";
    design.pages.push_back( std::move( page ) );
    design.occurrenceRoot.netNames[1] = "USIM_DATA";

    SETTINGS_MANAGER manager;
    manager.LoadProject( "" );
    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    schematic->SetProject( &manager.Prj() );
    SCH_SHEET*         root = convertRawDesign( design, *schematic );
    std::set<wxString> labels;

    for( SCH_ITEM* item : root->GetScreen()->Items().OfType( SCH_GLOBAL_LABEL_T ) )
        labels.insert( static_cast<SCH_GLOBALLABEL*>( item )->GetText() );

    BOOST_CHECK( labels.contains( wxS( "PCIE_USIM_DATA" ) ) );
}


BOOST_AUTO_TEST_CASE( LeadingSlashNetNameRemainsDistinct )
{
    ORCAD_RAW_PAGE page;
    page.name = "ACTIVE LOW NET";
    page.netmap[1] = "STATUS";
    page.netAliases[1] = { "STATUS" };
    ORCAD_WIRE high;
    high.id = 1;
    high.x2 = 100;
    high.aliases.push_back( ORCAD_ALIAS{ .name = "STATUS" } );
    page.wires.push_back( std::move( high ) );
    page.netmap[2] = "/STATUS";
    page.netAliases[2] = { "/STATUS" };
    ORCAD_WIRE low;
    low.id = 2;
    low.x1 = 200;
    low.x2 = 300;
    low.aliases.push_back( ORCAD_ALIAS{ .name = "/STATUS", .x = 200 } );
    page.wires.push_back( std::move( low ) );

    ORCAD_DESIGN design;
    design.sourceId = "active-low-net";
    design.pages.push_back( std::move( page ) );

    SETTINGS_MANAGER manager;
    manager.LoadProject( "" );
    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    schematic->SetProject( &manager.Prj() );
    SCH_SHEET*         root = convertRawDesign( design, *schematic );
    std::set<wxString> labels;

    for( SCH_ITEM* item : root->GetScreen()->Items().OfType( SCH_LABEL_T ) )
        labels.insert( static_cast<SCH_LABEL*>( item )->GetText() );

    BOOST_CHECK( labels.contains( wxS( "STATUS" ) ) );
    BOOST_CHECK( labels.contains( wxS( "{slash}STATUS" ) ) );
}


BOOST_AUTO_TEST_CASE( HierarchicalInternalNetUsesBlockName )
{
    ORCAD_RAW_PAGE rootPage;
    rootPage.name = "ROOT";

    ORCAD_DRAWN_INSTANCE drawn;
    drawn.dbId = 100;
    drawn.name = "G13";
    drawn.reference = "G1";
    rootPage.blocks.push_back( drawn );

    ORCAD_RAW_PAGE childPage;
    childPage.name = "CHILD";
    childPage.netmap[1] = "LED1_R";

    ORCAD_WIRE wire;
    wire.id = 1;
    wire.x2 = 100;
    ORCAD_ALIAS alias;
    alias.name = "LED1_R";
    wire.aliases.push_back( alias );
    childPage.wires.push_back( wire );

    childPage.netmap[2] = "LOCAL_PORT_NAME";
    ORCAD_WIRE portWire;
    portWire.id = 2;
    portWire.y1 = 100;
    portWire.x2 = 100;
    portWire.y2 = 100;
    childPage.wires.push_back( portWire );

    ORCAD_GRAPHIC_INST port;
    port.name = "PORTBOTH-R";
    port.logicalName = "PARENT_NET_NAME";
    port.color = 8;
    port.bbox = ORCAD_BBOX{ 0, 90, 70, 110 };
    port.x = 0;
    port.y = 100;
    port.displayProps.push_back(
            ORCAD_DISPLAY_PROP{ .name = "Name", .x = 10, .y = 3, .fontIdx = 1, .dispMode = 0x101 } );
    childPage.ports.push_back( std::move( port ) );

    childPage.netmap[4] = "N6941990";
    ORCAD_WIRE generatedWire;
    generatedWire.dbId = 6941990;
    generatedWire.id = 4;
    generatedWire.y1 = 300;
    generatedWire.x2 = 100;
    generatedWire.y2 = 300;
    childPage.wires.push_back( generatedWire );


    ORCAD_OCC_BLOCK occurrence;
    occurrence.targetDbId = 100;
    occurrence.childFolder = "CHILD";
    occurrence.scope.netNames[10] = "LED1_R";
    occurrence.scope.netNames[11] = "LOCAL_PORT_NAME";
    occurrence.scope.netNames[12] = "N6941990";

    ORCAD_DESIGN design;
    design.sourceId = "hierarchical-flat-net-name";
    design.library.fonts.push_back( ORCAD_FONT{ .height = -9, .face = "Arial" } );
    ORCAD_SYMBOL_DEF portDefinition;
    portDefinition.typeId = ORCAD_ST_PORT_SYMBOL;
    portDefinition.name = "PORTBOTH-R";
    portDefinition.bbox = ORCAD_BBOX{ 0, 0, 70, 20 };
    portDefinition.primitives.push_back(
            ORCAD_PRIMITIVE{ .kind = ORCAD_PRIM_KIND::LINE, .x1 = 0, .y1 = 10, .x2 = 70, .y2 = 10 } );
    portDefinition.pins.push_back( ORCAD_SYMBOL_PIN{ .hotptX = 0, .hotptY = 10 } );
    design.symbols.emplace( portDefinition.name, std::move( portDefinition ) );
    design.pages.push_back( std::move( rootPage ) );
    design.childFolderPages["child"].push_back( std::move( childPage ) );
    design.occurrenceRoot.blocks.push_back( std::move( occurrence ) );

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    SCH_SHEET* root = convertRawDesign( design, *schematic );
    SCH_SHEET* child = nullptr;

    for( SCH_ITEM* item : root->GetScreen()->Items().OfType( SCH_SHEET_T ) )
        child = static_cast<SCH_SHEET*>( item );

    BOOST_REQUIRE( child );
    BOOST_CHECK( !child->GetField( FIELD_T::SHEET_NAME )->IsVisible() );
    BOOST_CHECK( !child->GetField( FIELD_T::SHEET_FILENAME )->IsVisible() );
    std::set<wxString> labels;

    for( SCH_ITEM* item : child->GetScreen()->Items().OfType( SCH_LABEL_T ) )
        labels.insert( static_cast<SCH_LABEL*>( item )->GetText() );

    for( SCH_ITEM* item : child->GetScreen()->Items().OfType( SCH_GLOBAL_LABEL_T ) )
        labels.insert( static_cast<SCH_GLOBALLABEL*>( item )->GetText() );

    BOOST_CHECK( labels.contains( wxS( "LED1_R" ) ) );
    BOOST_CHECK( !labels.contains( wxS( "LED1_R_G13" ) ) );
    BOOST_CHECK( !labels.contains( wxS( "LOCAL_PORT_NAME_G13" ) ) );
    BOOST_CHECK( !labels.contains( wxS( "N6941990_G13" ) ) );

    int exactPortLabels = 0;

    for( SCH_ITEM* item : child->GetScreen()->Items().OfType( SCH_HIER_LABEL_T ) )
    {
        SCH_LABEL_BASE* label = static_cast<SCH_LABEL_BASE*>( item );

        if( label->GetText() == wxS( "LOCAL_PORT_NAME" ) )
        {
            ++exactPortLabels;
            BOOST_CHECK_EQUAL( label->GetTextColor().a, 0.0 );
        }
    }

    BOOST_CHECK_EQUAL( exactPortLabels, 1 );
}


BOOST_AUTO_TEST_CASE( HierarchicalBlockDisplayFieldsPreserveSourceGeometry )
{
    ORCAD_RAW_PAGE rootPage;
    rootPage.name = "ROOT";

    ORCAD_DRAWN_INSTANCE drawn;
    drawn.dbId = 100;
    drawn.reference = "BoM";
    drawn.x1 = 100;
    drawn.y1 = 200;
    drawn.w = 150;
    drawn.h = 60;
    drawn.displayProps = {
        ORCAD_DISPLAY_PROP{ .name = "Reference", .y = -10, .fontIdx = 2, .dispMode = 0x101 },
        ORCAD_DISPLAY_PROP{ .name = "Value", .x = 85, .y = -10, .fontIdx = 9, .dispMode = 0x101 },
    };
    drawn.pins.push_back( ORCAD_BLOCK_PIN{ .name = "Gate", .portType = ORCAD_PORT_TYPE::INPUT,
                                           .x = 100, .y = 230 } );
    rootPage.blocks.push_back( std::move( drawn ) );

    ORCAD_RAW_PAGE childPage;
    childPage.name = "Channel_BoM";

    ORCAD_OCC_BLOCK occurrence;
    occurrence.targetDbId = 100;
    occurrence.childFolder = "Channel_BoM";

    ORCAD_DESIGN design;
    design.sourceId = "hierarchical-display-fields";
    design.pages.push_back( std::move( rootPage ) );
    design.childFolderPages["channel_bom"].push_back( std::move( childPage ) );
    design.occurrenceRoot.blocks.push_back( std::move( occurrence ) );

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    SCH_SHEET* root = convertRawDesign( design, *schematic );
    SCH_SHEET* child = nullptr;

    for( SCH_ITEM* item : root->GetScreen()->Items().OfType( SCH_SHEET_T ) )
        child = static_cast<SCH_SHEET*>( item );

    BOOST_REQUIRE( child );
    SCH_FIELD* reference = child->GetField( FIELD_T::SHEET_NAME );
    SCH_FIELD* value = child->GetField( wxS( "Implementation" ) );
    BOOST_CHECK( reference->IsVisible() );
    BOOST_CHECK_EQUAL( reference->GetText(), wxS( "BoM" ) );
    BOOST_CHECK_EQUAL( reference->GetPosition().x, schIUScale.MilsToIU( 1000 ) );
    BOOST_REQUIRE( value );
    BOOST_CHECK( value->IsVisible() );
    BOOST_CHECK_EQUAL( value->GetText(), wxS( "Channel_BoM" ) );
    BOOST_CHECK_EQUAL( value->GetPosition().x, schIUScale.MilsToIU( 1850 ) );
    BOOST_CHECK( !child->GetField( FIELD_T::SHEET_FILENAME )->IsVisible() );

    SCH_SHAPE* pinFill = nullptr;

    for( SCH_ITEM* item : root->GetScreen()->Items().OfType( SCH_SHAPE_T ) )
        pinFill = static_cast<SCH_SHAPE*>( item );

    BOOST_REQUIRE( pinFill );
    BOOST_CHECK( pinFill->GetShape() == SHAPE_T::POLY );
    BOOST_CHECK( pinFill->GetFillMode() == FILL_T::FILLED_WITH_COLOR );
}


BOOST_AUTO_TEST_CASE( MultiPageHierarchyFollowsCaptureFolderOrder )
{
    ORCAD_RAW_PAGE titlePage;
    titlePage.name = "0-TITLE";

    ORCAD_RAW_PAGE mainPage;
    mainPage.name = "1-MAIN";

    ORCAD_DRAWN_INSTANCE gateBlock;
    gateBlock.dbId = 600;
    gateBlock.reference = "GATE";
    gateBlock.w = 100;
    gateBlock.h = 100;
    mainPage.blocks.push_back( gateBlock );

    ORCAD_DRAWN_INSTANCE microBlock;
    microBlock.dbId = 200;
    microBlock.reference = "MICRO";
    microBlock.x1 = 200;
    microBlock.w = 100;
    microBlock.h = 100;
    mainPage.blocks.push_back( microBlock );

    ORCAD_OCC_BLOCK gateOccurrence;
    gateOccurrence.targetDbId = 600;
    gateOccurrence.childFolder = "Z_GATE";

    ORCAD_OCC_BLOCK microOccurrence;
    microOccurrence.targetDbId = 200;
    microOccurrence.childFolder = "A_MICRO";

    ORCAD_RAW_PAGE gatePage;
    gatePage.name = "6-GATE";
    ORCAD_RAW_PAGE microPage;
    microPage.name = "2-MICRO";

    ORCAD_DESIGN design;
    design.sourceId = "multi-page-hierarchy-order";
    design.pages.push_back( std::move( titlePage ) );
    design.pages.push_back( std::move( mainPage ) );
    design.childFolderPages["z_gate"].push_back( std::move( gatePage ) );
    design.childFolderPages["a_micro"].push_back( std::move( microPage ) );
    design.occurrenceRoot.blocks.push_back( std::move( gateOccurrence ) );
    design.occurrenceRoot.blocks.push_back( std::move( microOccurrence ) );

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    convertRawDesign( design, *schematic );

    std::vector<SCH_SHEET*> topLevelSheets = schematic->GetTopLevelSheets();
    BOOST_REQUIRE_EQUAL( topLevelSheets.size(), 2u );
    BOOST_CHECK_EQUAL( topLevelSheets[0]->GetName(), wxS( "0-TITLE" ) );
    BOOST_CHECK_EQUAL( topLevelSheets[1]->GetName(), wxS( "1-MAIN" ) );

    SCH_SHEET_LIST sheets = schematic->BuildSheetListSortedByPageNumbers();
    BOOST_REQUIRE_EQUAL( sheets.size(), 4u );

    for( size_t i = 0; i < sheets.size(); ++i )
        BOOST_CHECK_EQUAL( sheets[i].GetPageNumber(), wxString::Format( wxS( "%zu" ), i + 1 ) );

    BOOST_CHECK_EQUAL( design.pages[0].sourcePageNumber, 1u );
    BOOST_CHECK_EQUAL( design.pages[1].sourcePageNumber, 2u );
    BOOST_CHECK_EQUAL( design.childFolderPages["a_micro"][0].sourcePageNumber, 3u );
    BOOST_CHECK_EQUAL( design.childFolderPages["z_gate"][0].sourcePageNumber, 4u );

    for( const ORCAD_RAW_PAGE* page : { &design.pages[0], &design.pages[1], &design.childFolderPages["a_micro"][0],
                                        &design.childFolderPages["z_gate"][0] } )
    {
        BOOST_CHECK_EQUAL( page->sourcePageCount, 4u );
    }

    std::vector<wxString> childNames;

    for( const SCH_SHEET_PATH& path : sheets )
    {
        if( path.Last()->GetName() == wxS( "MICRO" ) || path.Last()->GetName() == wxS( "GATE" ) )
            childNames.push_back( path.Last()->GetName() );
    }

    BOOST_REQUIRE_EQUAL( childNames.size(), 2u );
    BOOST_CHECK_EQUAL( childNames[0], wxS( "MICRO" ) );
    BOOST_CHECK_EQUAL( childNames[1], wxS( "GATE" ) );
}


BOOST_AUTO_TEST_CASE( GeneratedParentNetUsesHierarchicalBlockPinName )
{
    ORCAD_RAW_PAGE rootPage;
    rootPage.name = "ROOT";
    rootPage.netmap[1] = "N12345";

    ORCAD_WIRE parentWire;
    parentWire.id = 1;
    parentWire.x2 = 100;
    rootPage.wires.push_back( parentWire );

    ORCAD_DRAWN_INSTANCE drawn;
    drawn.dbId = 100;
    drawn.reference = "G1";
    drawn.w = 100;
    drawn.h = 100;
    drawn.pins.push_back( ORCAD_BLOCK_PIN{ .name = "SIGNAL", .x = 0, .y = 0 } );
    rootPage.blocks.push_back( std::move( drawn ) );

    ORCAD_RAW_PAGE childPage;
    childPage.name = "CHILD";

    ORCAD_OCC_BLOCK occurrence;
    occurrence.targetDbId = 100;
    occurrence.childFolder = "CHILD";

    ORCAD_DESIGN design;
    design.sourceId = "hierarchical-parent-net-name";
    design.pages.push_back( std::move( rootPage ) );
    design.childFolderPages["child"].push_back( std::move( childPage ) );
    design.occurrenceRoot.blocks.push_back( std::move( occurrence ) );

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    SCH_SHEET*         root = convertRawDesign( design, *schematic );
    std::set<wxString> labels;

    for( SCH_ITEM* item : root->GetScreen()->Items().OfType( SCH_LABEL_T ) )
        labels.insert( static_cast<SCH_LABEL*>( item )->GetText() );

    BOOST_CHECK( labels.contains( wxS( "SIGNAL" ) ) );
    BOOST_CHECK( !labels.contains( wxS( "N12345" ) ) );
}


BOOST_AUTO_TEST_CASE( GeneratedParentNetUsesFirstConnectedHierarchicalBlockPinName )
{
    ORCAD_RAW_PAGE rootPage;
    rootPage.name = "ROOT";
    rootPage.netmap[1] = "N12345";

    ORCAD_WIRE parentWire;
    parentWire.id = 1;
    parentWire.x2 = 100;
    rootPage.wires.push_back( parentWire );

    ORCAD_DRAWN_INSTANCE drawn;
    drawn.dbId = 100;
    drawn.reference = "G1";
    drawn.w = 100;
    drawn.h = 100;
    drawn.pins.push_back( ORCAD_BLOCK_PIN{ .name = "G1", .x = 0, .y = 0 } );
    drawn.pins.push_back( ORCAD_BLOCK_PIN{ .name = "G5", .x = 100, .y = 0 } );
    rootPage.blocks.push_back( std::move( drawn ) );

    ORCAD_RAW_PAGE childPage;
    childPage.name = "CHILD";

    ORCAD_OCC_BLOCK occurrence;
    occurrence.targetDbId = 100;
    occurrence.childFolder = "CHILD";

    ORCAD_DESIGN design;
    design.sourceId = "hierarchical-parent-shared-net-name";
    design.pages.push_back( std::move( rootPage ) );
    design.childFolderPages["child"].push_back( std::move( childPage ) );
    design.occurrenceRoot.blocks.push_back( std::move( occurrence ) );

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    SCH_SHEET*         root = convertRawDesign( design, *schematic );
    std::set<wxString> labels;

    for( SCH_ITEM* item : root->GetScreen()->Items().OfType( SCH_LABEL_T ) )
        labels.insert( static_cast<SCH_LABEL*>( item )->GetText() );

    BOOST_CHECK( labels.contains( wxS( "G1" ) ) );
    BOOST_CHECK( !labels.contains( wxS( "N12345" ) ) );
}


BOOST_AUTO_TEST_CASE( S593487_StaleCachedPinGeometry )
{
    ORCAD_SYMBOL_DEF cached;
    cached.name = "THERMISTOR.Normal";
    cached.bbox = ORCAD_BBOX{ 0, 0, 20, 30 };
    cached.primitives.push_back( ORCAD_PRIMITIVE{} );
    cached.pins.resize( 4 );
    for( size_t i = 0; i < cached.pins.size(); ++i )
    {
        cached.pins[i].hotptX = static_cast<int>( i ) * 10;
        cached.pins[i].hotptY = 10;
        cached.pins[i].startX = cached.pins[i].hotptX;
        cached.pins[i].startY = 10;
    }

    ORCAD_PLACED_INSTANCE placed;
    placed.x = 60;
    placed.y = 1060;
    placed.rotation = 1;
    placed.pins.resize( 2 );
    placed.pins[0].pinIndex = 2;
    placed.pins[0].x = 70;
    placed.pins[0].y = 1060;
    placed.pins[1].pinIndex = -4;
    placed.pins[1].x = 70;
    placed.pins[1].y = 1080;

    placed.pkgName = cached.name;
    placed.sourcePackage = "THERMISTOR";
    placed.reference = "U1";
    placed.dbId = 1;

    ORCAD_SYMBOL_DEF load;
    load.typeId = ORCAD_ST_LIBRARY_PART;
    load.name = "LOAD.Normal";
    load.bbox = ORCAD_BBOX{ 0, 0, 10, 10 };
    load.pins.resize( 1 );
    load.pins[0].hotptX = 0;
    load.pins[0].hotptY = 0;
    load.pins[0].startX = 0;
    load.pins[0].startY = 0;

    ORCAD_PLACED_INSTANCE loadPlaced;
    loadPlaced.pkgName = load.name;
    loadPlaced.sourcePackage = "LOAD";
    loadPlaced.reference = "U2";
    loadPlaced.dbId = 2;
    loadPlaced.x = 100;
    loadPlaced.y = 1060;
    loadPlaced.pins.push_back( ORCAD_PIN_INST{ 1, 100, 1060 } );

    ORCAD_RAW_PAGE page;
    page.name = "PIN GEOMETRY";
    page.instances.push_back( placed );
    page.instances.push_back( loadPlaced );
    page.wires.push_back( ORCAD_WIRE{ 10, 10, 70, 1060, 100, 1060 } );

    ORCAD_DESIGN design;
    design.sourceId = "s593487-stale-pin-geometry";
    design.symbols.emplace( cached.name, cached );
    design.symbols.emplace( load.name, load );
    design.pages.push_back( std::move( page ) );

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );

    SCH_SHEET*  rootSheet = new SCH_SHEET( schematic.get() );
    SCH_SCREEN* rootScreen = new SCH_SCREEN( schematic.get() );
    rootSheet->SetScreen( rootScreen );
    schematic->SetTopLevelSheets( { rootSheet } );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( rootSheet );

    ORCAD_CONVERTER converter( design, schematic.get(), nullptr );
    converter.Convert( rootSheet );

    auto [consistent, checkable] =
            checkConnectivity( *schematic, { { terminalToken( "U1", "2" ), terminalToken( "U2", "1" ) } } );
    BOOST_CHECK_EQUAL( checkable, 1 );
    BOOST_CHECK_EQUAL( consistent, 1 );

    SCH_SHEET_PATH rootPath;
    rootPath.push_back( rootSheet );
    SCH_SYMBOL*     converted = nullptr;
    SCH_NO_CONNECT* noConnect = nullptr;

    for( SCH_ITEM* item : rootScreen->Items() )
    {
        if( item->Type() == SCH_SYMBOL_T )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

            if( symbol->GetRef( &rootPath, false ) == wxS( "U1" ) )
                converted = symbol;
        }
        else if( item->Type() == SCH_NO_CONNECT_T )
        {
            noConnect = static_cast<SCH_NO_CONNECT*>( item );
        }
    }

    BOOST_REQUIRE( converted );
    BOOST_REQUIRE( noConnect );
    std::set<std::pair<int, int>> convertedHotpoints;
    SCH_PIN*                      noConnectPin = nullptr;

    for( SCH_PIN* pin : converted->GetPins() )
    {
        convertedHotpoints.emplace( pin->GetPosition().x, pin->GetPosition().y );

        if( pin->GetNumber() == wxS( "4" ) )
            noConnectPin = pin;
    }

    BOOST_CHECK_EQUAL( convertedHotpoints.size(), converted->GetPins().size() );
    BOOST_REQUIRE( noConnectPin );
    BOOST_CHECK( noConnectPin->GetPosition() == noConnect->GetPosition() );
}


BOOST_AUTO_TEST_CASE( PlacedPinLocationIsTheConnectionPoint )
{
    ORCAD_SYMBOL_DEF cached;
    cached.typeId = ORCAD_ST_LIBRARY_PART;
    cached.name = "TESTPOINT.Normal";
    cached.bbox = ORCAD_BBOX{ 0, 0, 20, 20 };
    cached.pins.resize( 1 );
    cached.pins[0].position = 0;
    cached.pins[0].startX = 0;
    cached.pins[0].startY = 10;
    cached.pins[0].hotptX = -10;
    cached.pins[0].hotptY = 10;

    ORCAD_PLACED_INSTANCE placed;
    placed.pkgName = cached.name;
    placed.sourcePackage = "TESTPOINT";
    placed.reference = "TP1";
    placed.x = 100;
    placed.y = 100;
    placed.pins = { ORCAD_PIN_INST{ 1, 90, 110 } };

    ORCAD_RAW_PAGE page;
    page.name = "PIN BODY END";
    page.instances.push_back( placed );

    ORCAD_DESIGN design;
    design.sourceId = "placed-pin-body-end";
    design.symbols.emplace( cached.name, cached );
    design.pages.push_back( std::move( page ) );

    SETTINGS_MANAGER manager;
    manager.LoadProject( "" );
    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    schematic->SetProject( &manager.Prj() );
    SCH_SHEET* root = convertRawDesign( design, *schematic );
    SCH_SHEET_PATH path;
    path.push_back( root );
    SCH_SYMBOL* symbol = findConvertedSymbol( *root->GetScreen(), path, wxS( "TP1" ) );

    BOOST_REQUIRE( symbol );
    BOOST_REQUIRE_EQUAL( symbol->GetPins().size(), 1u );
    VECTOR2I relative = symbol->GetPins().front()->GetPosition() - symbol->GetPosition();
    BOOST_CHECK_EQUAL( relative.x, -10 * ORCAD_IU_PER_DBU );
    BOOST_CHECK_EQUAL( relative.y, 10 * ORCAD_IU_PER_DBU );
}


BOOST_AUTO_TEST_CASE( SynthesizedSinglePinBodyUsesRotatedPlacementExtent )
{
    ORCAD_PLACED_INSTANCE placed;
    placed.pkgName = "MISSING_TESTPOINT.Normal";
    placed.sourcePackage = "MISSING_TESTPOINT";
    placed.reference = "TP1";
    placed.x = 100;
    placed.y = 100;
    placed.pins = { ORCAD_PIN_INST{ 1, 90, 110 } };

    ORCAD_RAW_PAGE page;
    page.name = "MISSING TESTPOINT";
    page.instances.push_back( placed );
    placed.reference = "TP2";
    placed.x = 200;
    placed.rotation = 2;
    placed.pins = { ORCAD_PIN_INST{ 1, 230, 110 } };
    page.instances.push_back( placed );

    ORCAD_DESIGN design;
    design.sourceId = "synthesized-single-pin";
    design.pages.push_back( std::move( page ) );

    SETTINGS_MANAGER manager;
    manager.LoadProject( "" );
    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    schematic->SetProject( &manager.Prj() );
    convertRawDesign( design, *schematic );

    const ORCAD_SYMBOL_DEF& generated = design.symbols.at( placed.pkgName );
    BOOST_REQUIRE( generated.bbox );
    BOOST_CHECK_EQUAL( generated.bbox->x1, 0 );
    BOOST_CHECK_EQUAL( generated.bbox->y1, 0 );
    BOOST_CHECK_EQUAL( generated.bbox->x2, 20 );
    BOOST_CHECK_EQUAL( generated.bbox->y2, 20 );
    BOOST_REQUIRE_EQUAL( generated.pins.size(), 1u );
    BOOST_CHECK_EQUAL( generated.pins.front().hotptX, -10 );
    BOOST_CHECK_EQUAL( generated.pins.front().hotptY, 10 );
    BOOST_CHECK_EQUAL( generated.pins.front().startX, 0 );
    BOOST_CHECK_EQUAL( generated.pins.front().startY, 10 );
}


BOOST_AUTO_TEST_CASE( S593487_ClosestCachedBodyIsFitted )
{
    ORCAD_SYMBOL_DEF primary;
    primary.typeId = ORCAD_ST_LIBRARY_PART;
    primary.name = "BODY.Normal";
    primary.bbox = ORCAD_BBOX{ 0, 0, 20, 20 };
    primary.primitives.push_back( ORCAD_PRIMITIVE{ .kind = ORCAD_PRIM_KIND::RECT, .x2 = 20, .y2 = 20 } );
    primary.pins.resize( 2 );
    primary.pins[0].hotptX = 0;
    primary.pins[1].hotptX = 20;
    primary.pins[0].startX = primary.pins[0].hotptX;
    primary.pins[1].startX = primary.pins[1].hotptX;

    ORCAD_SYMBOL_DEF alternate = primary;
    alternate.bbox = ORCAD_BBOX{ 0, 0, 40, 20 };
    alternate.primitives.front().x2 = 40;
    alternate.pins[0].hotptX = 99;
    alternate.pins[1].hotptX = 119;
    alternate.pins[0].startX = alternate.pins[0].hotptX;
    alternate.pins[1].startX = alternate.pins[1].hotptX;
    primary.variants.push_back( alternate );

    ORCAD_PLACED_INSTANCE placed;
    placed.pkgName = primary.name;
    placed.sourcePackage = "BODY";
    placed.reference = "U1";
    placed.x = 100;
    placed.y = 100;
    placed.pins = { ORCAD_PIN_INST{ 1, 200, 100 }, ORCAD_PIN_INST{ 2, 220, 100 } };

    ORCAD_RAW_PAGE page;
    page.name = "CLOSEST BODY";
    page.instances.push_back( placed );

    ORCAD_DESIGN design;
    design.sourceId = "s593487-closest-body";
    design.symbols.emplace( primary.name, primary );
    design.pages.push_back( std::move( page ) );

    SETTINGS_MANAGER manager;
    manager.LoadProject( "" );
    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    schematic->SetProject( &manager.Prj() );
    convertRawDesign( design, *schematic );

    const ORCAD_SYMBOL_DEF& converted = design.symbols.at( primary.name );
    BOOST_REQUIRE_EQUAL( converted.variants.size(), 2u );
    BOOST_REQUIRE_EQUAL( converted.variants.back().primitives.size(), 1u );
    BOOST_REQUIRE( converted.variants.back().bbox );
    BOOST_CHECK_EQUAL( converted.variants.back().bbox->x2, 40 );
    BOOST_CHECK_EQUAL( converted.variants.back().primitives.front().x2, 40 );
}


BOOST_AUTO_TEST_CASE( S593487_PrimaryGraphicsUsePlacedVariantGeometry )
{
    ORCAD_SYMBOL_DEF primary;
    primary.typeId = ORCAD_ST_LIBRARY_PART;
    primary.name = "MIXED.Normal";
    primary.bbox = ORCAD_BBOX{ 0, 0, 20, 20 };
    primary.primitives.push_back( ORCAD_PRIMITIVE{
            .kind = ORCAD_PRIM_KIND::TEXT, .x1 = 10, .y1 = -20, .x2 = 30, .y2 = -5, .text = "primary cue" } );
    primary.pins.resize( 2 );
    primary.pins[0].hotptX = 10;
    primary.pins[0].hotptY = -10;
    primary.pins[0].startX = primary.pins[0].hotptX;
    primary.pins[0].startY = primary.pins[0].hotptY;
    primary.pins[1].hotptX = 10;
    primary.pins[1].hotptY = 40;
    primary.pins[1].startX = primary.pins[1].hotptX;
    primary.pins[1].startY = primary.pins[1].hotptY;

    ORCAD_SYMBOL_DEF alternate = primary;
    alternate.bbox = ORCAD_BBOX{ 0, 0, 30, 20 };
    alternate.primitives.front().text = "alternate cue";
    ORCAD_SYMBOL_DEF stale = primary;
    stale.pins[0].hotptX = 0;
    stale.pins[1].hotptX = 0;
    stale.pins[0].startX = stale.pins[0].hotptX;
    stale.pins[1].startX = stale.pins[1].hotptX;
    stale.synthesized = true;
    primary.variants = { stale, alternate };

    ORCAD_PLACED_INSTANCE placed;
    placed.pkgName = primary.name;
    placed.sourcePackage = "MIXED";
    placed.reference = "RT1";
    placed.x = 100;
    placed.y = 100;
    placed.rotation = 1;
    placed.pins = { ORCAD_PIN_INST{ 1, 90, 120 }, ORCAD_PIN_INST{ 2, 140, 120 } };

    ORCAD_RAW_PAGE page;
    page.name = "MIXED GRAPHICS";
    page.instances.push_back( placed );

    ORCAD_DESIGN design;
    design.sourceId = "s593487-mixed-graphics";
    design.symbols.emplace( primary.name, primary );
    design.pages.push_back( std::move( page ) );

    SETTINGS_MANAGER manager;
    manager.LoadProject( "" );
    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    schematic->SetProject( &manager.Prj() );
    convertRawDesign( design, *schematic );

    const ORCAD_SYMBOL_DEF& converted = design.symbols.at( primary.name );
    BOOST_REQUIRE_EQUAL( converted.variants.size(), 3u );
    BOOST_CHECK( converted.variants.front().synthesized );
    BOOST_REQUIRE_EQUAL( converted.variants.front().primitives.size(), 1u );
    BOOST_CHECK_EQUAL( converted.variants.front().primitives.front().text, "primary cue" );
    BOOST_REQUIRE( converted.variants.front().bbox );
    BOOST_CHECK_EQUAL( converted.variants.front().bbox->x2, 30 );
    BOOST_CHECK_EQUAL( converted.variants.front().pins[0].hotptX, 10 );
    BOOST_CHECK_EQUAL( converted.variants.front().pins[1].hotptX, 10 );
}


BOOST_AUTO_TEST_CASE( SourceMatchedVariantPreservesItsGraphics )
{
    ORCAD_SYMBOL_DEF primary;
    primary.typeId = ORCAD_ST_LIBRARY_PART;
    primary.name = "RESISTOR.Normal";
    primary.sourceLib = "PSPICE_ELEM.OLB";
    primary.bbox = ORCAD_BBOX{ 0, 0, 30, 20 };
    primary.primitives.push_back(
            ORCAD_PRIMITIVE{ .kind = ORCAD_PRIM_KIND::LINE, .x1 = 4, .y1 = 10, .x2 = 28, .y2 = 10 } );
    primary.pins.resize( 2 );
    primary.pins[0].hotptX = -10;
    primary.pins[0].hotptY = 10;
    primary.pins[1].hotptX = 40;
    primary.pins[1].hotptY = 10;

    ORCAD_SYMBOL_DEF alternate = primary;
    alternate.sourceLib = "DISCRETE.OLB";
    alternate.bbox = ORCAD_BBOX{ 0, 0, 20, 30 };
    alternate.primitives.front() =
            ORCAD_PRIMITIVE{ .kind = ORCAD_PRIM_KIND::LINE, .x1 = 10, .y1 = 3, .x2 = 10, .y2 = 27 };
    alternate.pins[0].hotptX = 10;
    alternate.pins[0].hotptY = -10;
    alternate.pins[1].hotptX = 10;
    alternate.pins[1].hotptY = 40;
    primary.variants.push_back( alternate );

    ORCAD_PLACED_INSTANCE placed;
    placed.pkgName = primary.name;
    placed.sourcePackage = "RESISTOR";
    placed.sourceLibrary = alternate.sourceLib;
    placed.reference = "R1";
    placed.x = 100;
    placed.y = 100;
    placed.pins = { ORCAD_PIN_INST{ 1, 110, 90 }, ORCAD_PIN_INST{ 2, 110, 140 } };

    ORCAD_RAW_PAGE page;
    page.name = "SOURCE MATCHED GRAPHICS";
    page.instances.push_back( placed );

    ORCAD_DESIGN design;
    design.sourceId = "source-matched-graphics";
    design.symbols.emplace( primary.name, primary );
    design.pages.push_back( std::move( page ) );

    SETTINGS_MANAGER manager;
    manager.LoadProject( "" );
    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    schematic->SetProject( &manager.Prj() );
    convertRawDesign( design, *schematic );

    const ORCAD_SYMBOL_DEF& converted = design.symbols.at( primary.name );
    BOOST_REQUIRE_EQUAL( converted.variants.size(), 1u );
    BOOST_REQUIRE_EQUAL( converted.variants.front().primitives.size(), 1u );
    BOOST_CHECK_EQUAL( converted.variants.front().primitives.front().x1, 10 );
    BOOST_CHECK_EQUAL( converted.variants.front().primitives.front().x2, 10 );
}


BOOST_AUTO_TEST_CASE( S593487_StackedPlacedPinsRemainStacked )
{
    ORCAD_SYMBOL_DEF cached;
    cached.typeId = ORCAD_ST_LIBRARY_PART;
    cached.name = "STACK.Normal";
    cached.bbox = ORCAD_BBOX{ 0, 0, 20, 20 };
    cached.pins.resize( 3 );

    for( size_t i = 0; i < cached.pins.size(); ++i )
    {
        cached.pins[i].hotptX = static_cast<int>( i ) * 10;
        cached.pins[i].startX = cached.pins[i].hotptX;
    }

    ORCAD_PLACED_INSTANCE placed;
    placed.pkgName = cached.name;
    placed.sourcePackage = "STACK";
    placed.reference = "U1";
    placed.x = 100;
    placed.y = 100;
    placed.pins = { ORCAD_PIN_INST{ 1, 120, 100 }, ORCAD_PIN_INST{ 2, 120, 100 } };

    ORCAD_RAW_PAGE page;
    page.name = "STACKED";
    page.instances.push_back( placed );

    ORCAD_DESIGN design;
    design.sourceId = "s593487-stacked";
    design.symbols.emplace( cached.name, cached );
    design.pages.push_back( std::move( page ) );

    SETTINGS_MANAGER manager;
    manager.LoadProject( "" );
    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    schematic->SetProject( &manager.Prj() );
    SCH_SHEET*     root = convertRawDesign( design, *schematic );
    SCH_SHEET_PATH path;
    path.push_back( root );
    SCH_SYMBOL* symbol = findConvertedSymbol( *root->GetScreen(), path, wxS( "U1" ) );
    BOOST_REQUIRE( symbol );

    std::map<wxString, VECTOR2I> positions;
    std::map<wxString, bool>     visibility;

    for( SCH_PIN* pin : symbol->GetPins() )
    {
        positions[pin->GetNumber()] = pin->GetPosition();
        visibility[pin->GetNumber()] = pin->IsVisible();
    }

    BOOST_REQUIRE_EQUAL( positions.size(), 3u );
    BOOST_CHECK( positions[wxS( "1" )] == positions[wxS( "2" )] );
    BOOST_CHECK( positions[wxS( "1" )] != positions[wxS( "3" )] );
    BOOST_CHECK( visibility[wxS( "1" )] );
    BOOST_CHECK( visibility[wxS( "2" )] );
    BOOST_CHECK( visibility[wxS( "3" )] );
}


BOOST_AUTO_TEST_CASE( DistinctNetsOnStackedPinsRemainDistinct )
{
    ORCAD_SYMBOL_DEF stacked;
    stacked.typeId = ORCAD_ST_LIBRARY_PART;
    stacked.name = "STACKED.Normal";
    stacked.bbox = ORCAD_BBOX{ 0, 0, 20, 20 };
    stacked.pins = { ORCAD_SYMBOL_PIN{ .name = "1", .position = 0 }, ORCAD_SYMBOL_PIN{ .name = "2", .position = 1 } };

    ORCAD_SYMBOL_DEF load;
    load.typeId = ORCAD_ST_LIBRARY_PART;
    load.name = "LOAD.Normal";
    load.bbox = ORCAD_BBOX{ 0, 0, 10, 10 };
    load.pins.push_back( ORCAD_SYMBOL_PIN{ .name = "1", .position = 0 } );

    auto makePlaced = []( const std::string& aPackage, const std::string& aReference, int aX,
                          std::initializer_list<uint32_t> aNets )
    {
        ORCAD_PLACED_INSTANCE placed;
        placed.pkgName = aPackage;
        placed.sourcePackage = aPackage.substr( 0, aPackage.find( '.' ) );
        placed.reference = aReference;
        placed.x = aX;

        int pinIndex = 1;

        for( uint32_t net : aNets )
            placed.pins.push_back(
                    ORCAD_PIN_INST{ .pinIndex = static_cast<int16_t>( pinIndex++ ), .x = aX, .wordB = net } );

        return placed;
    };

    ORCAD_RAW_PAGE page;
    page.name = "DISTINCT STACKED NETS";
    page.netmap = { { 1, "LEFT" }, { 2, "COMMON" }, { 3, "RIGHT" } };
    page.instances.push_back( makePlaced( stacked.name, "Q1", 100, { 1, 2 } ) );
    page.instances.push_back( makePlaced( stacked.name, "Q2", 200, { 3, 2 } ) );
    page.instances.push_back( makePlaced( load.name, "R1", 300, { 1 } ) );
    page.instances.push_back( makePlaced( load.name, "R2", 400, { 3 } ) );

    ORCAD_DESIGN design;
    design.sourceId = "distinct-stacked-nets";
    design.symbols.emplace( stacked.name, std::move( stacked ) );
    design.symbols.emplace( load.name, std::move( load ) );
    design.pages.push_back( std::move( page ) );

    SETTINGS_MANAGER manager;
    manager.LoadProject( "" );
    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    schematic->SetProject( &manager.Prj() );
    convertRawDesign( design, *schematic );

    auto [consistent, checkable] =
            checkConnectivity( *schematic, { { terminalToken( "Q1", "1" ), terminalToken( "R1", "1" ) },
                                             { terminalToken( "Q2", "1" ), terminalToken( "R2", "1" ) },
                                             { terminalToken( "Q1", "2" ), terminalToken( "Q2", "2" ) } } );
    BOOST_CHECK_EQUAL( checkable, 3 );
    BOOST_CHECK_EQUAL( consistent, 3 );
}


BOOST_AUTO_TEST_CASE( S593487_ReorderedSignedPinsValidateExactly )
{
    ORCAD_SYMBOL_DEF cached;
    cached.typeId = ORCAD_ST_LIBRARY_PART;
    cached.name = "ORDER.Normal";
    cached.bbox = ORCAD_BBOX{ 0, 0, 30, 20 };
    cached.pins.resize( 4 );

    for( size_t i = 0; i < cached.pins.size(); ++i )
    {
        cached.pins[i].hotptX = static_cast<int>( i ) * 10;
        cached.pins[i].startX = cached.pins[i].hotptX;
    }

    ORCAD_PLACED_INSTANCE placed;
    placed.pkgName = cached.name;
    placed.sourcePackage = "ORDER";
    placed.reference = "U1";
    placed.x = 100;
    placed.y = 100;
    placed.pins = { ORCAD_PIN_INST{ -4, 130, 100 }, ORCAD_PIN_INST{ 2, 110, 100 }, ORCAD_PIN_INST{ 1, 100, 100 },
                    ORCAD_PIN_INST{ 3, 120, 100 } };

    ORCAD_RAW_PAGE page;
    page.name = "REORDERED";
    page.instances.push_back( placed );

    ORCAD_DESIGN design;
    design.sourceId = "s593487-reordered";
    design.symbols.emplace( cached.name, cached );
    design.pages.push_back( std::move( page ) );

    SETTINGS_MANAGER manager;
    manager.LoadProject( "" );
    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    schematic->SetProject( &manager.Prj() );
    WX_STRING_REPORTER reporter;
    SCH_SHEET*         root = convertRawDesign( design, *schematic, &reporter );
    BOOST_CHECK( !reporter.GetMessages().Contains( wxS( "pin positions mismatch" ) ) );

    SCH_SHEET_PATH path;
    path.push_back( root );
    SCH_SYMBOL* symbol = findConvertedSymbol( *root->GetScreen(), path, wxS( "U1" ) );
    BOOST_REQUIRE( symbol );
    std::map<int, VECTOR2I> positions;

    for( SCH_PIN* pin : symbol->GetPins() )
        positions[std::stoi( pin->GetNumber().ToStdString() )] = pin->GetPosition();

    BOOST_REQUIRE_EQUAL( positions.size(), 4u );

    for( int pin = 2; pin <= 4; ++pin )
        BOOST_CHECK_EQUAL( positions[pin].x - positions[1].x, ( pin - 1 ) * 10 * ORCAD_IU_PER_DBU );

    SCH_NO_CONNECT* noConnect = nullptr;

    for( SCH_ITEM* item : root->GetScreen()->Items().OfType( SCH_NO_CONNECT_T ) )
        noConnect = static_cast<SCH_NO_CONNECT*>( item );

    BOOST_REQUIRE( noConnect );
    BOOST_CHECK( noConnect->GetPosition() == positions[4] );
}


BOOST_AUTO_TEST_CASE( S593487_FittedVariantImportIsDeterministic )
{
    auto makeDesign = []
    {
        ORCAD_SYMBOL_DEF cached;
        cached.typeId = ORCAD_ST_LIBRARY_PART;
        cached.name = "DETERMINISTIC.Normal";
        cached.bbox = ORCAD_BBOX{ 0, 0, 20, 20 };
        cached.pins.resize( 3 );

        for( size_t i = 0; i < cached.pins.size(); ++i )
            cached.pins[i].hotptX = static_cast<int>( i ) * 10;

        ORCAD_PLACED_INSTANCE placed;
        placed.pkgName = cached.name;
        placed.sourcePackage = "DETERMINISTIC";
        placed.reference = "U1";
        placed.x = 100;
        placed.y = 100;
        placed.pins = { ORCAD_PIN_INST{ 1, 120, 100 }, ORCAD_PIN_INST{ -2, 120, 100 } };

        ORCAD_RAW_PAGE page;
        page.name = "DETERMINISTIC";
        page.instances.push_back( placed );
        placed.reference = "U2";
        placed.x = 200;
        placed.y = 200;
        placed.pins = { ORCAD_PIN_INST{ 1, 220, 200 }, ORCAD_PIN_INST{ -2, 220, 200 } };
        page.instances.push_back( placed );

        ORCAD_DESIGN design;
        design.sourceId = "s593487-deterministic";
        design.symbols.emplace( cached.name, cached );
        design.pages.push_back( std::move( page ) );
        return design;
    };

    SETTINGS_MANAGER manager;
    manager.LoadProject( "" );

    auto importSignature = [&]( ORCAD_DESIGN& aDesign )
    {
        std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
        schematic->SetProject( &manager.Prj() );
        SCH_SHEET*     root = convertRawDesign( aDesign, *schematic );
        SCH_SHEET_PATH path;
        path.push_back( root );
        SCH_SYMBOL* symbol = findConvertedSymbol( *root->GetScreen(), path, wxS( "U1" ) );
        BOOST_REQUIRE( symbol );
        std::string signature;

        for( SCH_PIN* pin : symbol->GetPins() )
        {
            signature += pin->GetNumber().ToStdString() + "@" + std::to_string( pin->GetPosition().x ) + ","
                         + std::to_string( pin->GetPosition().y ) + ";";
        }

        signature += "variants=" + std::to_string( aDesign.symbols.at( "DETERMINISTIC.Normal" ).variants.size() );
        return signature;
    };

    ORCAD_DESIGN first = makeDesign();
    ORCAD_DESIGN second = makeDesign();
    BOOST_CHECK_EQUAL( importSignature( first ), importSignature( second ) );
    BOOST_CHECK_EQUAL( first.symbols.at( "DETERMINISTIC.Normal" ).variants.size(), 1u );
    BOOST_CHECK_EQUAL( second.symbols.at( "DETERMINISTIC.Normal" ).variants.size(), 1u );
}


BOOST_AUTO_TEST_CASE( PartReferenceDisplayPreservesUnitDesignator )
{
    ORCAD_SYMBOL_DEF definition;
    definition.typeId = ORCAD_ST_LIBRARY_PART;
    definition.name = "MULTIPARTA.Normal";
    definition.bbox = ORCAD_BBOX{ 0, 0, 20, 20 };

    ORCAD_PACKAGE package;
    package.name = "MULTIPART";
    package.devices.push_back( ORCAD_DEVICE{ .unitRef = "A" } );

    ORCAD_PLACED_INSTANCE placed;
    placed.pkgName = definition.name;
    placed.sourcePackage = package.name;
    placed.reference = "U14";
    placed.value = "74HC125";
    placed.x = 100;
    placed.y = 100;
    placed.props["Reference"] = "U14";
    placed.displayProps = {
        ORCAD_DISPLAY_PROP{ .name = "Part Reference", .x = 30, .y = 10, .fontIdx = 0, .dispMode = 0x101 },
        ORCAD_DISPLAY_PROP{ .name = "Value", .x = 30, .y = 20, .fontIdx = 0, .dispMode = 0x101 },
    };

    ORCAD_RAW_PAGE page;
    page.name = "MULTIPART REFERENCE";
    page.instances.push_back( placed );

    ORCAD_DESIGN design;
    design.sourceId = "multipart-reference";
    design.symbols.emplace( definition.name, std::move( definition ) );
    design.packages.emplace( package.name, std::move( package ) );
    design.pages.push_back( std::move( page ) );

    SETTINGS_MANAGER manager;
    manager.LoadProject( "" );
    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    schematic->SetProject( &manager.Prj() );
    SCH_SHEET* root = convertRawDesign( design, *schematic );
    SCH_SHEET_PATH path;
    path.push_back( root );

    SCH_SYMBOL* symbol = findConvertedSymbol( *root->GetScreen(), path, wxS( "U14" ) );
    BOOST_REQUIRE( symbol );
    BOOST_CHECK_EQUAL( symbol->GetRef( &path, false ), wxS( "U14" ) );
    BOOST_CHECK( !symbol->GetField( FIELD_T::REFERENCE )->IsVisible() );

    SCH_FIELD* displayedReference = symbol->GetField( wxS( "Part Reference" ) );
    BOOST_REQUIRE( displayedReference );
    BOOST_CHECK_EQUAL( displayedReference->GetText(), wxS( "U14A" ) );
    BOOST_CHECK( displayedReference->IsVisible() );
    VECTOR2I pageOffset = symbol->GetPosition() - OrcadDbuToIu( placed.x, placed.y );
    BOOST_CHECK_EQUAL( displayedReference->GetPosition().x, OrcadDbuToIu( 130, 110 ).x + pageOffset.x );
    BOOST_CHECK_EQUAL( displayedReference->GetPosition().y,
                       OrcadDbuToIu( 130, 110 ).y + pageOffset.y
                               + OrcadTextBaselineOffset( displayedReference->GetTextHeight() ) );
}


BOOST_AUTO_TEST_CASE( OccurrenceReferenceOverridesDisplayedTemplateReference )
{
    ORCAD_SYMBOL_DEF definition;
    definition.typeId = ORCAD_ST_LIBRARY_PART;
    definition.name = "TESTPOINT.Normal";
    definition.bbox = ORCAD_BBOX{ 0, 0, 20, 20 };
    definition.props["Part Reference"] = "TP119";

    ORCAD_PACKAGE package;
    package.name = "TESTPOINT";
    package.devices.push_back( ORCAD_DEVICE{} );

    ORCAD_PLACED_INSTANCE placed;
    placed.dbId = 42;
    placed.pkgName = definition.name;
    placed.sourcePackage = package.name;
    placed.reference = "TP119";
    placed.x = 100;
    placed.y = 100;
    placed.displayProps = {
        ORCAD_DISPLAY_PROP{ .name = "Part Reference", .x = 30, .y = 10, .fontIdx = 0, .dispMode = 0x101 },
    };

    ORCAD_RAW_PAGE page;
    page.name = "OCCURRENCE REFERENCE";
    page.instances.push_back( placed );

    ORCAD_DESIGN design;
    design.sourceId = "occurrence-reference-display";
    design.symbols.emplace( definition.name, std::move( definition ) );
    design.packages.emplace( package.name, std::move( package ) );
    design.pages.push_back( std::move( page ) );
    design.occurrenceRoot.partRefs[42] = "TP17";

    SETTINGS_MANAGER manager;
    manager.LoadProject( "" );
    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    schematic->SetProject( &manager.Prj() );
    SCH_SHEET* root = convertRawDesign( design, *schematic );
    SCH_SHEET_PATH path;
    path.push_back( root );

    SCH_SYMBOL* converted = findConvertedSymbol( *root->GetScreen(), path, wxS( "TP17" ) );
    BOOST_REQUIRE( converted );
    BOOST_CHECK_EQUAL( converted->GetRef( &path, false ), wxS( "TP17" ) );
    BOOST_REQUIRE( converted->GetField( FIELD_T::REFERENCE ) );
    BOOST_CHECK( converted->GetField( FIELD_T::REFERENCE )->IsVisible() );

    SCH_FIELD* templateReference = converted->GetField( wxS( "Part Reference" ) );
    BOOST_CHECK( !templateReference || !templateReference->IsVisible() );
}


BOOST_AUTO_TEST_CASE( SingleUnitReferenceDoesNotUseCacheNameAsUnitDesignator )
{
    ORCAD_SYMBOL_DEF connectorDefinition;
    connectorDefinition.typeId = ORCAD_ST_LIBRARY_PART;
    connectorDefinition.name = "BANANA, KEYSTONE-575-4.Normal";
    connectorDefinition.bbox = ORCAD_BBOX{ 0, 0, 20, 20 };

    ORCAD_PACKAGE connectorPackage;
    connectorPackage.name = "BANANA_KEYSTONE-575-4";
    connectorPackage.devices.push_back( ORCAD_DEVICE{} );

    ORCAD_PLACED_INSTANCE connector;
    connector.pkgName = connectorDefinition.name;
    connector.sourcePackage = connectorPackage.name;
    connector.reference = "J1";
    connector.x = 100;
    connector.y = 100;
    connector.displayProps = {
        ORCAD_DISPLAY_PROP{ .name = "Part Reference", .x = 30, .fontIdx = 0, .dispMode = 0x100 },
    };

    ORCAD_SYMBOL_DEF padDefinition;
    padDefinition.typeId = ORCAD_ST_LIBRARY_PART;
    padDefinition.name = "PAD_2.Normal";
    padDefinition.bbox = ORCAD_BBOX{ 0, 0, 20, 20 };

    ORCAD_PACKAGE padPackage;
    padPackage.name = "PAD_2";
    padPackage.devices.push_back( ORCAD_DEVICE{} );

    ORCAD_PLACED_INSTANCE pad;
    pad.pkgName = padDefinition.name;
    pad.sourcePackage = padPackage.name;
    pad.reference = "SW";
    pad.x = 200;
    pad.y = 200;
    pad.displayProps = {
        ORCAD_DISPLAY_PROP{ .name = "Part Reference", .y = 26, .fontIdx = 0, .dispMode = 0x000 },
        ORCAD_DISPLAY_PROP{ .name = "Reference", .x = 5, .y = 15, .fontIdx = 2, .dispMode = 0x100 },
    };

    ORCAD_RAW_PAGE page;
    page.name = "SINGLE UNIT REFERENCE";
    page.instances = { connector, pad };

    ORCAD_DESIGN design;
    design.sourceId = "single-unit-reference";
    design.symbols.emplace( connectorDefinition.name, std::move( connectorDefinition ) );
    design.symbols.emplace( padDefinition.name, std::move( padDefinition ) );
    design.packages.emplace( connectorPackage.name, std::move( connectorPackage ) );
    design.packages.emplace( padPackage.name, std::move( padPackage ) );
    design.pages.push_back( std::move( page ) );

    SETTINGS_MANAGER manager;
    manager.LoadProject( "" );
    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    schematic->SetProject( &manager.Prj() );
    SCH_SHEET* root = convertRawDesign( design, *schematic );
    SCH_SHEET_PATH path;
    path.push_back( root );

    SCH_SYMBOL* convertedConnector = findConvertedSymbol( *root->GetScreen(), path, wxS( "J1" ) );
    BOOST_REQUIRE( convertedConnector );
    SCH_FIELD* connectorReference = convertedConnector->GetField( FIELD_T::REFERENCE );
    BOOST_REQUIRE( connectorReference );
    BOOST_CHECK( connectorReference->IsVisible() );
    BOOST_CHECK_EQUAL( convertedConnector->GetRef( &path, false ), wxS( "J1" ) );
    BOOST_CHECK( !convertedConnector->GetField( wxS( "Part Reference" ) ) );

    SCH_SYMBOL* convertedPad = findConvertedSymbol( *root->GetScreen(), path, wxS( "SW" ) );
    BOOST_REQUIRE( convertedPad );
    SCH_FIELD* padReference = convertedPad->GetField( FIELD_T::REFERENCE );
    BOOST_REQUIRE( padReference );
    BOOST_CHECK( padReference->IsVisible() );
    BOOST_CHECK_EQUAL( convertedPad->GetRef( &path, false ), wxS( "SW" ) );
}


BOOST_AUTO_TEST_CASE( S593487_RT44DisplayedFootprintAndPassivePins )
{
    ORCAD_SYMBOL_DEF thermistor;
    thermistor.typeId = ORCAD_ST_LIBRARY_PART;
    thermistor.name = "THERMISTOR_2.Normal";
    thermistor.bbox = ORCAD_BBOX{ 0, 0, 20, 30 };
    thermistor.pins.resize( 2 );
    thermistor.pins[0].position = 0;
    thermistor.pins[0].startX = 10;
    thermistor.pins[0].hotptX = 10;
    thermistor.pins[0].hotptY = -10;
    thermistor.pins[0].portType = ORCAD_PORT_TYPE::PASSIVE;
    thermistor.pins[1].position = 1;
    thermistor.pins[1].startX = 10;
    thermistor.pins[1].startY = 30;
    thermistor.pins[1].hotptX = 10;
    thermistor.pins[1].hotptY = 40;
    thermistor.pins[1].portType = ORCAD_PORT_TYPE::PASSIVE;
    thermistor.primitives.push_back( ORCAD_PRIMITIVE{
            .kind = ORCAD_PRIM_KIND::TEXT, .x1 = 12, .y1 = -8, .x2 = 30, .y2 = 7, .text = "t", .fontIdx = 1 } );
    thermistor.primitives.push_back(
            ORCAD_PRIMITIVE{ .kind = ORCAD_PRIM_KIND::ELLIPSE, .x1 = 16, .y1 = 1, .x2 = 18, .y2 = 3 } );
    thermistor.props["2ND PART FIELD"] = "%";

    ORCAD_PACKAGE package;
    package.name = "THERMISTOR_2";
    package.refDes = "RT";
    package.pcbFootprint = "2920";
    package.devices.push_back( ORCAD_DEVICE{ .pinNumbers = { "1", "2" }, .pinIgnore = { false, false } } );

    ORCAD_PLACED_INSTANCE placed;
    placed.pkgName = thermistor.name;
    placed.sourcePackage = package.name;
    placed.reference = "RT44";
    placed.value = "3A";
    placed.x = 70;
    placed.y = 460;
    placed.rotation = 1;
    placed.props["Assembly"] = "Fitted";
    placed.props["Datasheet"] = "rt44.pdf";
    placed.props["2nd Part Field"] = "(OPT)";
    placed.displayProps = {
        ORCAD_DISPLAY_PROP{ .name = "Part Reference", .x = -20, .y = 10, .fontIdx = 0, .dispMode = 0x101 },
        ORCAD_DISPLAY_PROP{ .name = "Reference", .x = 30, .y = 14, .fontIdx = 2, .dispMode = 0x001 },
        ORCAD_DISPLAY_PROP{ .name = "Value", .x = 30, .y = 20, .fontIdx = 0, .dispMode = 0x101 },
        ORCAD_DISPLAY_PROP{ .name = "PCB Footprint", .x = -20, .y = 20, .fontIdx = 2, .dispMode = 0x101 },
        ORCAD_DISPLAY_PROP{ .name = "2nd Part Field", .x = -20, .y = 30, .fontIdx = 0, .dispMode = 0x101 },
    };
    placed.pins = { ORCAD_PIN_INST{ 1, 60, 470 }, ORCAD_PIN_INST{ 2, 110, 470 } };

    ORCAD_RAW_PAGE page;
    page.name = "RT44 FIDELITY";
    page.instances.push_back( placed );

    ORCAD_PLACED_INSTANCE resistor = placed;
    resistor.reference = "R31";
    resistor.value = "100k";
    resistor.x = 900;
    resistor.y = 690;
    resistor.rotation = 0;
    resistor.displayProps = {
        ORCAD_DISPLAY_PROP{ .name = "Reference", .x = 20, .fontIdx = 0, .dispMode = 0x101 },
        ORCAD_DISPLAY_PROP{ .name = "Value", .x = 20, .y = 10, .fontIdx = 3, .dispMode = 0x101 },
        ORCAD_DISPLAY_PROP{ .name = "PCB Footprint", .x = 20, .y = 20, .fontIdx = 4, .dispMode = 0x101 },
    };
    resistor.pins = { ORCAD_PIN_INST{ 1, 900, 680 }, ORCAD_PIN_INST{ 2, 900, 730 } };
    page.instances.push_back( resistor );

    ORCAD_SYMBOL_DEF q9Definition = thermistor;
    q9Definition.name = "IRF7416_0.Normal";
    q9Definition.generalFlags = 0;
    ORCAD_PLACED_INSTANCE q9 = placed;
    q9.pkgName = q9Definition.name;
    q9.sourcePackage.clear();
    q9.reference = "Q9";
    q9.x = 1200;
    q9.y = 500;
    q9.rotation = 0;
    q9.pins = { ORCAD_PIN_INST{ -1, 1210, 490, 1, 0 }, ORCAD_PIN_INST{ -2, 1210, 540 } };
    page.instances.push_back( q9 );

    ORCAD_SYMBOL_DEF connectorDefinition = thermistor;
    connectorDefinition.name = "CON24_46.Normal";
    connectorDefinition.generalFlags = 5;
    ORCAD_PACKAGE connectorPackage = package;
    connectorPackage.name = "CON24_46";
    connectorPackage.refDes = "J";
    connectorPackage.devices.front().pinNumbers = { "1", "2" };
    ORCAD_PLACED_INSTANCE connector = placed;
    connector.pkgName = connectorDefinition.name;
    connector.sourcePackage = connectorPackage.name;
    connector.reference = "JCA3";
    connector.x = 1400;
    connector.y = 500;
    connector.rotation = 0;
    connector.pins = { ORCAD_PIN_INST{ -1, 1410, 490, 1, 0 }, ORCAD_PIN_INST{ -2, 1410, 540, 1, 0 } };
    page.instances.push_back( connector );

    ORCAD_PLACED_INSTANCE mirrored = placed;
    mirrored.reference = "RTM";
    mirrored.x = 1600;
    mirrored.y = 500;
    mirrored.mirror = true;
    mirrored.pins = { ORCAD_PIN_INST{ 1, 1590, 510 }, ORCAD_PIN_INST{ 2, 1640, 510 } };
    page.instances.push_back( mirrored );

    ORCAD_SYMBOL_DEF blankMappedDefinition = thermistor;
    blankMappedDefinition.name = "BLANK_MAPPED.Normal";
    blankMappedDefinition.generalFlags = 0;
    blankMappedDefinition.pins[0].name = "1";
    blankMappedDefinition.pins[1].name = "2";
    ORCAD_PACKAGE blankMappedPackage = package;
    blankMappedPackage.name = "BLANK_MAPPED";
    blankMappedPackage.devices.front().pinNumbers = { "", "" };
    ORCAD_PLACED_INSTANCE blankMapped = placed;
    blankMapped.pkgName = blankMappedDefinition.name;
    blankMapped.sourcePackage = blankMappedPackage.name;
    blankMapped.reference = "RBLANK";
    blankMapped.x = 1800;
    blankMapped.y = 500;
    blankMapped.pins = { ORCAD_PIN_INST{ 1, 1810, 490 }, ORCAD_PIN_INST{ 2, 1810, 540 } };
    page.instances.push_back( blankMapped );

    ORCAD_SYMBOL_DEF dualPinTextDefinition = blankMappedDefinition;
    dualPinTextDefinition.name = "DUAL_PIN_TEXT.Normal";
    dualPinTextDefinition.generalFlags = 1;
    ORCAD_PACKAGE dualPinTextPackage = blankMappedPackage;
    dualPinTextPackage.name = "DUAL_PIN_TEXT";
    dualPinTextPackage.devices.front().pinNumbers = { "1", "2" };
    ORCAD_PLACED_INSTANCE dualPinText = blankMapped;
    dualPinText.pkgName = dualPinTextDefinition.name;
    dualPinText.sourcePackage = dualPinTextPackage.name;
    dualPinText.reference = "JDUAL";
    dualPinText.x = 2000;
    dualPinText.pins = { ORCAD_PIN_INST{ 1, 2010, 490 }, ORCAD_PIN_INST{ 2, 2010, 540 } };
    page.instances.push_back( dualPinText );

    ORCAD_GRAPHIC_INST comment;
    comment.typeId = ORCAD_ST_GRAPHIC_COMMENT_TEXT_INST;
    comment.color = 48;
    comment.nested = std::make_unique<ORCAD_SYMBOL_DEF>();
    comment.nested->primitives.push_back( ORCAD_PRIMITIVE{
            .kind = ORCAD_PRIM_KIND::TEXT, .x1 = 840, .y1 = 410, .x2 = 859, .y2 = 426,
            .text = ".5W\nNEXT", .fontIdx = 1 } );
    page.graphics.push_back( std::move( comment ) );

    ORCAD_GRAPHIC_INST ercMarker;
    ercMarker.typeId = ORCAD_ST_ERC_OBJECT;
    ercMarker.name = "ERC";
    ercMarker.x = 2200;
    ercMarker.y = 500;
    page.ercObjects.push_back( std::move( ercMarker ) );

    ORCAD_DESIGN design;
    design.sourceId = "s593487-rt44-fidelity";
    design.library.fonts.push_back( ORCAD_FONT{ .height = -9, .face = "Arial" } );
    design.library.fonts.push_back(
            ORCAD_FONT{ .height = -9, .width = 4, .pitchAndFamily = 0x31, .face = "Courier New" } );
    design.library.fonts.push_back( ORCAD_FONT{ .height = -9, .face = "Arial" } );
    design.library.fonts.push_back( ORCAD_FONT{ .height = -9, .face = "Arial", .italic = true } );
    design.symbols.emplace( thermistor.name, thermistor );
    design.symbols.emplace( q9Definition.name, q9Definition );
    design.symbols.emplace( connectorDefinition.name, connectorDefinition );
    design.symbols.emplace( blankMappedDefinition.name, blankMappedDefinition );
    design.symbols.emplace( dualPinTextDefinition.name, dualPinTextDefinition );
    design.packages.emplace( package.name, package );
    design.packages.emplace( connectorPackage.name, connectorPackage );
    design.packages.emplace( blankMappedPackage.name, blankMappedPackage );
    design.packages.emplace( dualPinTextPackage.name, dualPinTextPackage );

    std::vector<uint8_t> packageStream;
    appendLe16( packageStream, 1 );
    packageStream.push_back( ORCAD_ST_PART_CELL );
    appendLe16( packageStream, 0 );
    appendLzt( packageStream, "THERMISTOR_2" );
    appendLzt( packageStream, "" );
    appendLe16( packageStream, 1 );
    appendLzt( packageStream, "THERMISTOR_2.Normal" );
    appendLe16( packageStream, 1 );
    packageStream.push_back( ORCAD_ST_LIBRARY_PART );
    appendLe16( packageStream, 0 );
    appendLzt( packageStream, "THERMISTOR_2.Normal" );
    appendLzt( packageStream, "" );
    appendLe32( packageStream, 0 );
    appendLe16( packageStream, 0 );

    for( int coordinate : { 0, 0, 20, 30 } )
        appendLe16( packageStream, static_cast<uint16_t>( coordinate ) );

    appendLe16( packageStream, 0 );
    appendLe16( packageStream, 0 );
    appendLzt( packageStream, "" );
    appendLzt( packageStream, "" );
    appendLzt( packageStream, "RT" );
    appendLzt( packageStream, "" );
    appendLe16( packageStream, 6 );
    packageStream.push_back( ORCAD_ST_PACKAGE );
    appendLe16( packageStream, 0 );
    appendLzt( packageStream, "THERMISTOR_2" );
    appendLzt( packageStream, "" );
    appendLzt( packageStream, "RT" );
    appendLzt( packageStream, "" );
    appendLzt( packageStream, "2920" );
    appendLe16( packageStream, 0 );

    std::map<std::string, ORCAD_SYMBOL_DEF> packageSymbols;
    std::map<std::string, ORCAD_PACKAGE>    packageDefinitions;
    OrcadParseOlbPackageStreamV2( std::vector<char>( packageStream.begin(), packageStream.end() ), {}, packageSymbols,
                                  packageDefinitions );
    OrcadMergeSymbolGeneralProperties( design.symbols, packageSymbols );
    design.pages.push_back( std::move( page ) );

    SETTINGS_MANAGER manager;
    manager.LoadProject( "" );
    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    schematic->SetProject( &manager.Prj() );
    SCH_SHEET*     root = convertRawDesign( design, *schematic );
    SCH_SHEET_PATH path;
    path.push_back( root );

    const SCH_TEXT* commentText = nullptr;

    for( const SCH_ITEM* item : root->GetScreen()->Items() )
    {
        if( item->Type() == SCH_TEXT_T
            && static_cast<const SCH_TEXT*>( item )->GetText() == wxS( ".5W\nNEXT" ) )
            commentText = static_cast<const SCH_TEXT*>( item );
    }

    BOOST_REQUIRE( commentText );
    BOOST_CHECK_CLOSE( commentText->GetLineSpacing(), 5.0 / 6.0, 0.1 );
    BOOST_CHECK_EQUAL( commentText->GetInterline( nullptr ), schIUScale.mmToIU( 9.0 * 25.4 / 96.0 ) );
    BOOST_CHECK_EQUAL( commentText->GetPosition().x, OrcadDbuToIu( 900, 470 ).x );
    BOOST_CHECK_EQUAL( commentText->GetPosition().y,
                       OrcadDbuToIu( 900, 470 ).y
                               + OrcadTextBaselineOffset( schIUScale.mmToIU( 1.70 ) ) );

    SCH_SYMBOL* symbol = findConvertedSymbol( *root->GetScreen(), path, wxS( "RT44" ) );
    BOOST_REQUIRE( symbol );
    BOOST_CHECK( !symbol->GetShowPinNumbers() );
    BOOST_CHECK_EQUAL( symbol->GetField( FIELD_T::DATASHEET )->GetText(), wxS( "rt44.pdf" ) );

    const SCH_TEXT*  cueText = nullptr;
    const SCH_SHAPE* cueDegree = nullptr;

    for( const SCH_ITEM& item : symbol->GetLibSymbolRef()->GetDrawItems() )
    {
        if( item.Type() == SCH_TEXT_T )
            cueText = static_cast<const SCH_TEXT*>( &item );
        else if( item.Type() == SCH_SHAPE_T && static_cast<const SCH_SHAPE&>( item ).GetShape() == SHAPE_T::CIRCLE )
            cueDegree = static_cast<const SCH_SHAPE*>( &item );
    }

    BOOST_REQUIRE( cueText );
    BOOST_REQUIRE( cueDegree );
    int cueBaseline = KiROUND( cueText->GetTextSize().y * 8.0 / 21.0 );
    BOOST_CHECK_EQUAL( cueText->GetText(), wxS( "t" ) );
    BOOST_CHECK( cueText->GetDrawRotation() == ANGLE_VERTICAL );
    VECTOR2I cueTextPage = symbol->GetTransform().TransformCoordinate( cueText->GetPosition() ) + symbol->GetPosition();
    VECTOR2I pageOffset = symbol->GetPosition() - OrcadDbuToIu( placed.x, placed.y + 20 );
    VECTOR2I expectedCuePage = OrcadDbuToIu( placed.x + 21, placed.y ) + pageOffset + VECTOR2I( 0, cueBaseline );
    BOOST_CHECK_EQUAL( cueTextPage.x, expectedCuePage.x );
    BOOST_CHECK_EQUAL( cueTextPage.y, expectedCuePage.y );
    BOX2I    cueTextBox = symbol->GetTransform().TransformCoordinate( cueText->GetBoundingBox() );
    int      degreeRadius = cueDegree->GetRadius();
    VECTOR2I cueDegreePage = symbol->GetTransform().TransformCoordinate( cueDegree->GetPosition() );
    BOOST_CHECK_EQUAL( cueDegreePage.x, cueTextBox.GetRight() + degreeRadius + ORCAD_IU_PER_DBU );
    BOOST_CHECK_EQUAL( cueDegreePage.y, cueTextBox.Centre().y );

    SCH_FIELD* reference = symbol->GetField( FIELD_T::REFERENCE );
    SCH_FIELD* value = symbol->GetField( FIELD_T::VALUE );
    SCH_FIELD* footprint = symbol->GetField( wxS( "OrCAD Footprint" ) );
    BOOST_REQUIRE( footprint );
    int fieldBaseline = KiROUND( schIUScale.mmToIU( 1.70 ) * 8.0 / 21.0 );
    int valueBaseline = fieldBaseline;
    BOOST_CHECK_EQUAL( reference->GetPosition().x, OrcadDbuToIu( 50, 470 ).x + pageOffset.x );
    BOOST_CHECK_EQUAL( reference->GetPosition().y, OrcadDbuToIu( 50, 470 ).y + pageOffset.y + fieldBaseline );
    BOOST_CHECK_EQUAL( value->GetPosition().x, OrcadDbuToIu( 100, 480 ).x + pageOffset.x );
    BOOST_CHECK_EQUAL( value->GetPosition().y, OrcadDbuToIu( 100, 480 ).y + pageOffset.y + valueBaseline );
    BOOST_CHECK_EQUAL( footprint->GetPosition().x, OrcadDbuToIu( 50, 480 ).x + pageOffset.x );
    BOOST_CHECK_EQUAL( footprint->GetPosition().y, OrcadDbuToIu( 50, 480 ).y + pageOffset.y + fieldBaseline );
    BOOST_CHECK( reference->GetDrawRotation() == ANGLE_HORIZONTAL );
    BOOST_CHECK( value->GetDrawRotation() == ANGLE_HORIZONTAL );
    BOOST_CHECK( footprint->GetDrawRotation() == ANGLE_HORIZONTAL );
    BOOST_CHECK_EQUAL( reference->GetBoundingBox().GetOrigin().x, OrcadDbuToIu( 50, 470 ).x + pageOffset.x );
    BOOST_CHECK_LE(
            std::abs( reference->GetBoundingBox().GetOrigin().y - ( OrcadDbuToIu( 50, 470 ).y + pageOffset.y ) ),
            schIUScale.mmToIU( 0.7 ) );
    BOOST_CHECK_EQUAL( value->GetBoundingBox().GetOrigin().x, OrcadDbuToIu( 100, 480 ).x + pageOffset.x );
    BOOST_CHECK_LE( std::abs( value->GetBoundingBox().GetOrigin().y - ( OrcadDbuToIu( 100, 480 ).y + pageOffset.y ) ),
                    schIUScale.mmToIU( 0.7 ) );
    BOOST_CHECK_EQUAL( footprint->GetBoundingBox().GetOrigin().x, OrcadDbuToIu( 50, 480 ).x + pageOffset.x );
    BOOST_CHECK_LE(
            std::abs( footprint->GetBoundingBox().GetOrigin().y - ( OrcadDbuToIu( 50, 480 ).y + pageOffset.y ) ),
            schIUScale.mmToIU( 0.7 ) );
    BOOST_CHECK_EQUAL( reference->GetEffectiveHorizJustify(), GR_TEXT_H_ALIGN_LEFT );
    BOOST_CHECK_EQUAL( reference->GetEffectiveVertJustify(), GR_TEXT_V_ALIGN_TOP );
    BOOST_CHECK_EQUAL( value->GetEffectiveHorizJustify(), GR_TEXT_H_ALIGN_LEFT );
    BOOST_CHECK_EQUAL( value->GetEffectiveVertJustify(), GR_TEXT_V_ALIGN_TOP );
    BOOST_CHECK_EQUAL( footprint->GetEffectiveHorizJustify(), GR_TEXT_H_ALIGN_LEFT );
    BOOST_CHECK_EQUAL( footprint->GetEffectiveVertJustify(), GR_TEXT_V_ALIGN_TOP );
    BOOST_CHECK( reference->IsVisible() );
    BOOST_CHECK( value->IsVisible() );
    BOOST_CHECK( footprint->IsVisible() );
    BOOST_CHECK_EQUAL( footprint->GetText(), wxS( "2920" ) );
    int sourceFontSize = schIUScale.mmToIU( 1.70 );
    int sourceFontWidth = schIUScale.mmToIU( 0.95 );
    BOOST_CHECK_EQUAL( reference->GetTextSize().x, sourceFontSize );
    BOOST_CHECK_EQUAL( reference->GetTextSize().y, sourceFontSize );
    BOOST_CHECK_EQUAL( value->GetTextSize().x, sourceFontSize );
    BOOST_CHECK_EQUAL( value->GetTextSize().y, sourceFontSize );
    BOOST_CHECK_EQUAL( footprint->GetTextSize().x, sourceFontWidth );
    BOOST_CHECK_EQUAL( footprint->GetTextSize().y, sourceFontSize );
    BOOST_REQUIRE( reference->GetFont() );
    BOOST_REQUIRE( value->GetFont() );
    BOOST_REQUIRE( footprint->GetFont() );
    BOOST_CHECK_EQUAL( reference->GetFont()->GetName(), wxS( "Arial" ) );
    BOOST_CHECK_EQUAL( value->GetFont()->GetName(), wxS( "Arial" ) );
    BOOST_CHECK_EQUAL( footprint->GetFont()->GetName(), wxS( "Courier New" ) );
    BOOST_CHECK( !reference->IsItalic() );
    BOOST_CHECK( !value->IsItalic() );
    BOOST_CHECK( !footprint->IsItalic() );

    size_t secondPartFields = std::count_if(
            symbol->GetFields().begin(), symbol->GetFields().end(),
            []( const SCH_FIELD& aField ) { return aField.GetName().CmpNoCase( wxS( "2nd Part Field" ) ) == 0; } );
    BOOST_CHECK_EQUAL( secondPartFields, 1u );
    SCH_FIELD* secondPartField = symbol->GetField( wxS( "2ND PART FIELD" ) );
    BOOST_REQUIRE( secondPartField );
    BOOST_CHECK_EQUAL( secondPartField->GetText(), wxS( "(OPT)" ) );
    BOOST_CHECK( secondPartField->IsVisible() );

    SCH_SYMBOL* r31 = findConvertedSymbol( *root->GetScreen(), path, wxS( "R31" ) );
    BOOST_REQUIRE( r31 );
    BOOST_CHECK_NE( symbol->GetLibSymbolRef().get(), r31->GetLibSymbolRef().get() );
    const SCH_TEXT* r31Cue = nullptr;

    for( const SCH_ITEM& item : r31->GetLibSymbolRef()->GetDrawItems() )
    {
        if( item.Type() == SCH_TEXT_T )
            r31Cue = static_cast<const SCH_TEXT*>( &item );
    }

    BOOST_REQUIRE( r31Cue );
    int      r31CueBaseline = KiROUND( r31Cue->GetTextSize().y * 8.0 / 21.0 );
    VECTOR2I r31CuePage = r31->GetTransform().TransformCoordinate( r31Cue->GetPosition() ) + r31->GetPosition();
    VECTOR2I expectedR31CuePage =
            OrcadDbuToIu( resistor.x + 21, resistor.y ) + pageOffset + VECTOR2I( 0, r31CueBaseline );
    BOOST_CHECK_EQUAL( r31CuePage.x, expectedR31CuePage.x );
    BOOST_CHECK_EQUAL( r31CuePage.y, expectedR31CuePage.y );
    SCH_FIELD* r31Value = r31->GetField( FIELD_T::VALUE );
    SCH_FIELD* r31Reference = r31->GetField( FIELD_T::REFERENCE );
    SCH_FIELD* r31Footprint = r31->GetField( wxS( "OrCAD Footprint" ) );
    BOOST_REQUIRE( r31Footprint );
    BOOST_CHECK( r31Reference->IsVisible() );
    BOOST_CHECK_EQUAL( r31Reference->GetPosition().x, OrcadDbuToIu( 920, 690 ).x + pageOffset.x );
    BOOST_CHECK_EQUAL( r31Reference->GetPosition().y,
                       OrcadDbuToIu( 920, 690 ).y + pageOffset.y + fieldBaseline );
    BOOST_CHECK_EQUAL( r31Value->GetFont()->GetName(), wxS( "Arial" ) );
    BOOST_CHECK_EQUAL( r31Footprint->GetFont()->GetName(), wxS( "Arial" ) );
    BOOST_CHECK( !r31Value->IsItalic() );
    BOOST_CHECK( r31Footprint->IsItalic() );
    BOOST_CHECK_EQUAL( r31Value->GetTextSize().x, sourceFontSize );
    BOOST_CHECK_EQUAL( r31Footprint->GetTextSize().x, sourceFontSize );
    BOOST_CHECK_EQUAL( r31Value->GetBoundingBox().GetOrigin().x, r31Value->GetPosition().x );
    BOOST_CHECK_EQUAL( r31Footprint->GetBoundingBox().GetOrigin().x, r31Footprint->GetPosition().x );

    SCH_SYMBOL* q9Symbol = findConvertedSymbol( *root->GetScreen(), path, wxS( "Q9" ) );
    BOOST_REQUIRE( q9Symbol );
    BOOST_CHECK( !q9Symbol->GetShowPinNames() );
    BOOST_CHECK( q9Symbol->GetShowPinNumbers() );
    std::set<VECTOR2I> noConnects;

    for( SCH_ITEM* item : root->GetScreen()->Items().OfType( SCH_NO_CONNECT_T ) )
        noConnects.insert( item->GetPosition() );

    BOOST_CHECK_EQUAL( noConnects.size(), 1u );

    SCH_SYMBOL* jca3 = findConvertedSymbol( *root->GetScreen(), path, wxS( "JCA3" ) );
    BOOST_REQUIRE( jca3 );
    BOOST_CHECK( jca3->GetShowPinNames() );
    BOOST_CHECK( !jca3->GetShowPinNumbers() );
    BOOST_REQUIRE_EQUAL( jca3->GetPins().size(), 2u );
    BOOST_CHECK_EQUAL( jca3->GetPins()[0]->GetName(), jca3->GetPins()[0]->GetNumber() );
    BOOST_CHECK_EQUAL( jca3->GetPins()[1]->GetName(), jca3->GetPins()[1]->GetNumber() );

    SCH_SYMBOL* blankMappedSymbol = findConvertedSymbol( *root->GetScreen(), path, wxS( "RBLANK" ) );
    BOOST_REQUIRE( blankMappedSymbol );
    BOOST_REQUIRE_EQUAL( blankMappedSymbol->GetPins().size(), 2u );
    BOOST_CHECK_EQUAL( blankMappedSymbol->GetPins()[0]->GetNumber(), wxS( "1" ) );
    BOOST_CHECK_EQUAL( blankMappedSymbol->GetPins()[1]->GetNumber(), wxS( "2" ) );
    size_t visiblePinNumbers = 0;
    std::set<wxString> blankMappedNumbers = { wxS( "1" ), wxS( "2" ) };

    for( const SCH_ITEM& item : blankMappedSymbol->GetLibSymbolRef()->GetDrawItems() )
    {
        if( item.Type() == SCH_TEXT_T
            && blankMappedNumbers.contains( static_cast<const SCH_TEXT&>( item ).GetText() ) )
        {
            ++visiblePinNumbers;
        }
    }

    BOOST_CHECK_EQUAL( visiblePinNumbers, 0u );

    SCH_SYMBOL* dualPinTextSymbol = findConvertedSymbol( *root->GetScreen(), path, wxS( "JDUAL" ) );
    BOOST_REQUIRE( dualPinTextSymbol );
    BOOST_CHECK( dualPinTextSymbol->GetShowPinNames() );
    BOOST_CHECK( dualPinTextSymbol->GetShowPinNumbers() );

    for( const SCH_PIN* pin : dualPinTextSymbol->GetPins() )
    {
        BOOST_CHECK_EQUAL( pin->GetName(), pin->GetNumber() );
        BOOST_CHECK_GT( pin->GetNameTextSize(), 0 );
        BOOST_CHECK_GT( pin->GetNumberTextSize(), 0 );
    }

    for( const SCH_ITEM& item : dualPinTextSymbol->GetLibSymbolRef()->GetDrawItems() )
    {
        if( item.Type() == SCH_TEXT_T
            && ( static_cast<const SCH_TEXT&>( item ).GetText() == wxS( "1" )
                 || static_cast<const SCH_TEXT&>( item ).GetText() == wxS( "2" ) ) )
        {
            BOOST_ERROR( "Pin data duplicated as SCH_TEXT" );
        }
    }

    SCH_SYMBOL* mirroredSymbol = findConvertedSymbol( *root->GetScreen(), path, wxS( "RTM" ) );
    BOOST_REQUIRE( mirroredSymbol );
    const SCH_TEXT* mirroredCue = nullptr;

    for( const SCH_ITEM& item : mirroredSymbol->GetLibSymbolRef()->GetDrawItems() )
    {
        if( item.Type() == SCH_TEXT_T )
            mirroredCue = static_cast<const SCH_TEXT*>( &item );
    }

    BOOST_REQUIRE( mirroredCue );
    int      mirroredBaseline = KiROUND( mirroredCue->GetTextSize().y * 8.0 / 21.0 );
    VECTOR2I mirroredCuePage = mirroredSymbol->GetTransform().TransformCoordinate( mirroredCue->GetPosition() )
                               + mirroredSymbol->GetPosition();
    VECTOR2I expectedMirroredCuePage =
            OrcadDbuToIu( mirrored.x + 21, mirrored.y ) + pageOffset + VECTOR2I( 0, mirroredBaseline );
    BOOST_CHECK_EQUAL( mirroredCuePage.x, expectedMirroredCuePage.x );
    BOOST_CHECK_EQUAL( mirroredCuePage.y, expectedMirroredCuePage.y );
}


// OrCAD and SPECCTRA share the .dsn extension. Require an OrCAD compound document.

BOOST_AUTO_TEST_CASE( RejectsSpecctraTextDsn )
{
    TEMP_TEST_FILE specctra( wxS( "qa_orcad_specctra_impostor.dsn" ),
                             wxS( "(pcb \"impostor.dsn\"\n  (parser\n    (string_quote \")\n  )\n)\n" ) );

    BOOST_REQUIRE( wxFileName::FileExists( specctra.m_path ) );
    BOOST_CHECK( !m_plugin.CanReadSchematicFile( specctra.m_path ) );
}


BOOST_AUTO_TEST_CASE( RejectsNonexistentFile )
{
    wxFileName missing( wxFileName::GetTempDir(), wxS( "qa_orcad_does_not_exist.dsn" ) );

    BOOST_REQUIRE( !missing.FileExists() );
    BOOST_CHECK( !m_plugin.CanReadSchematicFile( missing.GetFullPath() ) );
}


BOOST_AUTO_TEST_CASE( RejectsWrongExtension )
{
    TEMP_TEST_FILE textFile( wxS( "qa_orcad_impostor.txt" ), wxS( "Just some text, not a schematic.\n" ) );

    BOOST_REQUIRE( wxFileName::FileExists( textFile.m_path ) );
    BOOST_CHECK( !m_plugin.CanReadSchematicFile( textFile.m_path ) );
}


// No positive-load test until a redistributable .dsn fixture exists under qa/data/eeschema/io/orcad/.


// Set KICAD_ORCAD_CORPUS to test private DSN files against adjacent NET and BOM exports.

static std::string trimCell( std::string aText )
{
    auto notSpace = []( unsigned char c )
    {
        return !std::isspace( c );
    };
    aText.erase( aText.begin(), std::find_if( aText.begin(), aText.end(), notSpace ) );
    aText.erase( std::find_if( aText.rbegin(), aText.rend(), notSpace ).base(), aText.end() );

    if( aText.size() >= 2 && aText.front() == '"' && aText.back() == '"' )
        aText = aText.substr( 1, aText.size() - 2 );

    return aText;
}


static std::vector<std::string> splitRefs( const std::string& aCell )
{
    std::vector<std::string> refs;
    std::string              token;

    for( char c : aCell )
    {
        if( c == ',' )
        {
            std::string r = trimCell( token );

            if( !r.empty() )
                refs.push_back( r );

            token.clear();
        }
        else
        {
            token += c;
        }
    }

    std::string r = trimCell( token );

    if( !r.empty() )
        refs.push_back( r );

    return refs;
}


static std::set<std::string> parseBomRefs( const std::string& aPath )
{
    std::set<std::string> refs;
    std::ifstream         in( aPath );
    std::string           line;
    int                   refCol = -1;

    while( std::getline( in, line ) )
    {
        if( !line.empty() && line.back() == '\r' )
            line.pop_back();

        std::vector<std::string> cols;
        std::string              cell;

        for( char c : line )
        {
            if( c == '\t' )
            {
                cols.push_back( cell );
                cell.clear();
            }
            else
            {
                cell += c;
            }
        }

        cols.push_back( cell );

        // Header row names reference column; capture index once
        if( refCol < 0 )
        {
            for( size_t i = 0; i < cols.size(); ++i )
            {
                if( trimCell( cols[i] ) == "Reference" )
                {
                    refCol = static_cast<int>( i );
                    break;
                }
            }

            continue;
        }

        if( refCol < static_cast<int>( cols.size() ) )
        {
            for( const std::string& r : splitRefs( trimCell( cols[refCol] ) ) )
                refs.insert( r );
        }
    }

    return refs;
}


static std::set<std::string> parseNetComs( const std::string& aPath )
{
    std::set<std::string> refs;
    std::ifstream         in( aPath );
    std::string           line;

    while( std::getline( in, line ) )
    {
        if( line.rfind( ".ADD_COM", 0 ) != 0 )
            continue;

        // .ADD_COM <ref> "<footprint>"
        std::string rest = trimCell( line.substr( 8 ) );
        std::string ref;

        for( char c : rest )
        {
            if( std::isspace( static_cast<unsigned char>( c ) ) )
                break;

            ref += c;
        }

        if( !ref.empty() )
            refs.insert( ref );
    }

    return refs;
}


/// Unique refdes of real (non-power) parts. Annotated refs live in per-sheet-path instance data,
/// so walk sheet list and query GetRef() per path (same screen may recur on several paths).
static std::set<std::string> collectImportedRefs( SCHEMATIC& aSchematic )
{
    std::set<std::string> refs;
    SCH_SHEET_LIST        sheets = aSchematic.BuildSheetListSortedByPageNumbers();

    for( const SCH_SHEET_PATH& path : sheets )
    {
        SCH_SCREEN* screen = path.LastScreen();

        if( !screen )
            continue;

        for( SCH_ITEM* item : screen->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );
            wxString    ref = symbol->GetRef( &path, false );

            // Leading '#' = power/hidden pseudo-part, not BOM; trailing '?' = unannotated.
            if( ref.IsEmpty() || ref.StartsWith( wxS( "#" ) ) || ref.EndsWith( wxS( "?" ) ) )
                continue;

            refs.insert( std::string( ref.ToUTF8() ) );
        }
    }

    return refs;
}


static std::string terminalToken( const std::string& aRef, const std::string& aPin )
{
    return trimCell( aRef ) + "." + trimCell( aPin );
}


/// Net terminal sets from .NET. Net starts at `.ADD_TER <ref> <pin>`, gathers following
/// `.TER`/indented `<ref> <pin>` continuations until next net.
static std::vector<std::set<std::string>> parseNetTerminals( const std::string& aPath )
{
    std::vector<std::set<std::string>> nets;
    std::set<std::string>              current;
    std::ifstream                      in( aPath );
    std::string                        line;

    auto flush = [&]()
    {
        if( current.size() >= 2 )
            nets.push_back( current );

        current.clear();
    };

    while( std::getline( in, line ) )
    {
        if( !line.empty() && line.back() == '\r' )
            line.pop_back();

        bool addTer = line.rfind( ".ADD_TER", 0 ) == 0;
        bool ter = line.rfind( ".TER", 0 ) == 0;
        bool cont = !line.empty() && std::isspace( static_cast<unsigned char>( line[0] ) );

        if( line.rfind( ".END", 0 ) == 0 )
            break;

        if( addTer )
            flush();

        if( addTer || ter || cont )
        {
            std::istringstream ss( addTer ? line.substr( 8 ) : ter ? line.substr( 4 ) : line );
            std::string        ref, pin;

            if( ss >> ref >> pin )
                current.insert( terminalToken( ref, pin ) );
        }
    }

    flush();
    return nets;
}


/// Count ground-truth nets whose resolvable terminals all land on one KiCad net after
/// connectivity rebuild. Returns {consistent, checkable}.
static std::pair<int, int> checkConnectivity( SCHEMATIC& aSchematic, const std::vector<std::set<std::string>>& aNets,
                                              std::vector<std::set<std::string>>* aInconsistent )
{
    SCH_SHEET_LIST sheets = aSchematic.BuildSheetListSortedByPageNumbers();
    aSchematic.ConnectionGraph()->Recalculate( sheets, true );

    std::map<std::string, int> pinNet;
    int                        netId = 0;

    for( const auto& [key, subgraphs] : aSchematic.ConnectionGraph()->GetNetMap() )
    {
        for( CONNECTION_SUBGRAPH* subgraph : subgraphs )
        {
            for( SCH_ITEM* item : subgraph->GetItems() )
            {
                if( item->Type() != SCH_PIN_T )
                    continue;

                SCH_PIN*    pin = static_cast<SCH_PIN*>( item );
                SCH_SYMBOL* symbol = dynamic_cast<SCH_SYMBOL*>( pin->GetParentSymbol() );

                if( !symbol )
                    continue;

                wxString ref = symbol->GetRef( &subgraph->GetSheet(), false );

                if( ref.IsEmpty() || ref.StartsWith( wxS( "#" ) ) || ref.EndsWith( wxS( "?" ) ) )
                    continue;

                pinNet[terminalToken( std::string( ref.ToUTF8() ), std::string( pin->GetNumber().ToUTF8() ) )] = netId;
            }
        }

        ++netId;
    }

    struct CHECKED_NET
    {
        const std::set<std::string>* terminals;
        std::set<int>                ids;
    };

    std::vector<CHECKED_NET> checkedNets;
    std::map<int, int>       sourceNetsPerImportedNet;

    for( const std::set<std::string>& net : aNets )
    {
        std::set<int> ids;
        int           resolved = 0;

        for( const std::string& term : net )
        {
            auto it = pinNet.find( term );

            if( it != pinNet.end() )
            {
                ids.insert( it->second );
                ++resolved;
            }
        }

        if( resolved >= 2 )
        {
            if( ids.size() == 1 )
                sourceNetsPerImportedNet[*ids.begin()]++;

            checkedNets.push_back( { &net, std::move( ids ) } );
        }
    }

    int consistent = 0;

    for( const CHECKED_NET& net : checkedNets )
    {
        bool exact = net.ids.size() == 1 && sourceNetsPerImportedNet[*net.ids.begin()] == 1;

        if( exact )
            ++consistent;
        else if( aInconsistent )
            aInconsistent->push_back( *net.terminals );
    }

    return { consistent, static_cast<int>( checkedNets.size() ) };
}


static wxString terminalNetName( SCHEMATIC& aSchematic, const wxString& aReference, const wxString& aPinNumber )
{
    SCH_SHEET_LIST sheets = aSchematic.BuildSheetListSortedByPageNumbers();
    aSchematic.ConnectionGraph()->Recalculate( sheets, true );

    for( const auto& [key, subgraphs] : aSchematic.ConnectionGraph()->GetNetMap() )
    {
        for( CONNECTION_SUBGRAPH* subgraph : subgraphs )
        {
            for( SCH_ITEM* item : subgraph->GetItems() )
            {
                if( item->Type() != SCH_PIN_T )
                    continue;

                SCH_PIN*    pin = static_cast<SCH_PIN*>( item );
                SCH_SYMBOL* symbol = dynamic_cast<SCH_SYMBOL*>( pin->GetParentSymbol() );

                if( symbol && symbol->GetRef( &subgraph->GetSheet(), false ) == aReference
                    && pin->GetNumber() == aPinNumber )
                {
                    return key.Name;
                }
            }
        }
    }

    return {};
}


/// Ground-truth companion beside .DSN, .NET preferred over .BOM.
static std::set<std::string> expectedRefsFor( const std::filesystem::path& aDsn, std::string& aSource )
{
    for( const char* ext : { ".NET", ".net", ".BOM", ".bom" } )
    {
        std::filesystem::path candidate = aDsn;
        candidate.replace_extension( ext );

        if( std::filesystem::exists( candidate ) )
        {
            aSource = candidate.filename().string();

            bool isNet = std::string( ext ) == ".NET" || std::string( ext ) == ".net";
            return isNet ? parseNetComs( candidate.string() ) : parseBomRefs( candidate.string() );
        }
    }

    aSource.clear();
    return {};
}


BOOST_AUTO_TEST_CASE( CorpusValidation )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
    {
        BOOST_TEST_MESSAGE( "KICAD_ORCAD_CORPUS not set; skipping OrCAD corpus validation." );
        return;
    }

    namespace fs = std::filesystem;
    fs::path root( corpusEnv );

    BOOST_REQUIRE_MESSAGE( fs::exists( root ), "KICAD_ORCAD_CORPUS path does not exist." );

    std::vector<fs::path> designs;

    for( auto it = fs::recursive_directory_iterator( root, fs::directory_options::skip_permission_denied );
         it != fs::recursive_directory_iterator(); ++it )
    {
        if( !it->is_regular_file() )
            continue;

        std::string ext = it->path().extension().string();
        std::transform( ext.begin(), ext.end(), ext.begin(),
                        []( unsigned char c )
                        {
                            return std::tolower( c );
                        } );

        if( ext == ".dsn" )
            designs.push_back( it->path() );
    }

    std::sort( designs.begin(), designs.end() );

    BOOST_TEST_MESSAGE( "OrCAD corpus: " << designs.size() << " .DSN files under " << root );

    int          imported = 0, crashed = 0, unsupported = 0, rejected = 0, checked = 0;
    unsigned int totalExpected = 0, totalMatched = 0, totalMissing = 0, totalExtra = 0;
    int          netConsistent = 0, netCheckable = 0, netTotal = 0;
    uint64_t     importedPages = 0, importedComponents = 0, importedPowerSymbols = 0;
    uint64_t     importedPins = 0, importedWires = 0, importedBuses = 0;
    uint64_t     importedLabels = 0, importedShapes = 0, importedTexts = 0, importedBitmaps = 0;

    const char* debugEnv = std::getenv( "KICAD_ORCAD_DEBUG" );
    std::string debugFilter = debugEnv ? debugEnv : "";
    const char* filterEnv = std::getenv( "KICAD_ORCAD_FILTER" );
    std::string designFilter = filterEnv ? filterEnv : "";

    for( const fs::path& dsn : designs )
    {
        std::string rel = fs::relative( dsn, root ).string();

        if( !designFilter.empty() && rel.find( designFilter ) == std::string::npos )
            continue;

        SCH_IO_ORCAD plugin;
        uint64_t     perDesignPages = 0, perDesignComponents = 0, perDesignPowerSymbols = 0;
        uint64_t     perDesignPins = 0, perDesignWires = 0, perDesignBuses = 0;
        uint64_t     perDesignLabels = 0, perDesignShapes = 0, perDesignTexts = 0;
        uint64_t     perDesignBitmaps = 0;
        uint64_t     perDesignRedWires = 0;
        uint64_t     perDesignVddmPowerSymbols = 0;

        bool debug = !debugFilter.empty() && rel.find( debugFilter ) != std::string::npos;

        if( !plugin.CanReadSchematicFile( dsn.string() ) )
        {
            ++rejected;
            continue;
        }

        SETTINGS_MANAGER           manager;
        std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
        manager.LoadProject( "" );
        schematic->SetProject( &manager.Prj() );
        schematic->CurrentSheet().clear();
        schematic->CurrentSheet().push_back( &schematic->Root() );

        WX_STRING_REPORTER reporter;
        plugin.SetReporter( &reporter );

        try
        {
            plugin.LoadSchematicFile( dsn.string(), schematic.get() );
            schematic->CurrentSheet().UpdateAllScreenReferences();
        }
        catch( const std::exception& e )
        {
            // Pre-2003 designs out of scope, rejected cleanly
            if( std::string( e.what() ).find( "pre-2003" ) != std::string::npos )
                ++unsupported;
            else
                ++crashed;

            BOOST_TEST_MESSAGE( "  THROW   " << rel << " : " << e.what() );
            continue;
        }

        ++imported;

        for( const SCH_SHEET_PATH& path : schematic->BuildSheetListSortedByPageNumbers() )
        {
            SCH_SCREEN* screen = path.LastScreen();

            if( !screen )
                continue;

            ++importedPages;
            ++perDesignPages;

            for( SCH_ITEM* item : screen->Items() )
            {
                switch( item->Type() )
                {
                case SCH_SYMBOL_T:
                {
                    SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );
                    wxString    ref = symbol->GetRef( &path, false );

                    if( ref.StartsWith( wxS( "#" ) ) )
                    {
                        ++importedPowerSymbols;
                        ++perDesignPowerSymbols;

                        if( symbol->GetValue( false, &path, false ) == wxS( "VDDM" ) )
                            ++perDesignVddmPowerSymbols;
                    }
                    else
                    {
                        ++importedComponents;
                        ++perDesignComponents;
                    }

                    size_t pinCount = symbol->GetPins( &path ).size();
                    importedPins += pinCount;
                    perDesignPins += pinCount;
                    break;
                }

                case SCH_LINE_T:
                {
                    SCH_LINE* line = static_cast<SCH_LINE*>( item );

                    if( line->GetLayer() == LAYER_BUS )
                    {
                        ++importedBuses;
                        ++perDesignBuses;
                    }
                    else if( line->GetLayer() == LAYER_WIRE )
                    {
                        ++importedWires;
                        ++perDesignWires;

                        if( line->GetLineColor() == OrcadColor( 8 ) )
                            ++perDesignRedWires;
                    }

                    break;
                }

                case SCH_LABEL_T:
                case SCH_GLOBAL_LABEL_T:
                case SCH_HIER_LABEL_T:
                    ++importedLabels;
                    ++perDesignLabels;
                    break;

                case SCH_SHAPE_T:
                    ++importedShapes;
                    ++perDesignShapes;
                    break;

                case SCH_TEXT_T:
                    ++importedTexts;
                    ++perDesignTexts;
                    break;

                case SCH_BITMAP_T:
                    ++importedBitmaps;
                    ++perDesignBitmaps;
                    break;

                default: break;
                }
            }
        }

        BOOST_TEST_MESSAGE( "  AUDIT   " << rel << "|pages=" << perDesignPages << "|components=" << perDesignComponents
                                         << "|power=" << perDesignPowerSymbols << "|pins=" << perDesignPins
                                         << "|wires=" << perDesignWires << "|buses=" << perDesignBuses
                                         << "|labels=" << perDesignLabels << "|shapes=" << perDesignShapes
                                         << "|texts=" << perDesignTexts << "|bitmaps=" << perDesignBitmaps );

        if( rel == "allegro/beagleboard-xm/SCH/BeagleBoard-xM_ORCAD.DSN" )
        {
            BOOST_CHECK_EQUAL( perDesignBitmaps, 10u );
            BOOST_CHECK_EQUAL( perDesignShapes, 108u );
            BOOST_CHECK_EQUAL( perDesignTexts, 175u );
        }

        if( rel
            == "allegro/OpenCellular-LED/Rev-C/schematic/"
               "OpenCellular_Connect-1_LED_Life-3_Schematic.DSN" )
        {
            BOOST_CHECK_EQUAL( perDesignVddmPowerSymbols, 31u );
        }

        if( rel
            == "orcad/OpenCellular-GBC-Elgon_ARM/Rev-A/schematics/"
               "CN81XX_GBCV2_sch_0530.DSN" )
        {
            BOOST_CHECK_EQUAL( perDesignRedWires, 35u );
        }

        if( debug )
            BOOST_TEST_MESSAGE( "  DEBUG   " << rel << " warnings:\n"
                                             << std::string( reporter.GetMessages().ToUTF8() ) );

        std::set<std::string> got = collectImportedRefs( *schematic );
        std::string           source;
        std::set<std::string> expected = expectedRefsFor( dsn, source );

        if( debug )
            BOOST_TEST_MESSAGE( "  DEBUG   " << rel << " imported " << got.size() << " refs" );

        if( expected.empty() )
            continue;

        std::set<std::string> missing, extra;
        std::set_difference( expected.begin(), expected.end(), got.begin(), got.end(),
                             std::inserter( missing, missing.begin() ) );
        std::set_difference( got.begin(), got.end(), expected.begin(), expected.end(),
                             std::inserter( extra, extra.begin() ) );

        unsigned int matched = static_cast<unsigned int>( expected.size() - missing.size() );

        ++checked;
        totalExpected += expected.size();
        totalMatched += matched;
        totalMissing += missing.size();
        totalExtra += extra.size();

        BOOST_TEST_MESSAGE( "  CHECK   " << rel << " : " << matched << "/" << expected.size() << " refs ("
                                         << int( 100.0 * matched / expected.size() ) << "%), extra " << extra.size()
                                         << " [" << source << "]" );

        if( debug )
        {
            for( const std::string& ref : missing )
                BOOST_TEST_MESSAGE( "          missing ref: " << ref );

            for( const std::string& ref : extra )
                BOOST_TEST_MESSAGE( "          extra ref: " << ref );
        }

        // .NET ground truth carries terminal connectivity; verify pins group per net after rebuild.
        std::filesystem::path net = dsn;
        net.replace_extension( source.size() >= 4 && source.substr( source.size() - 4 ) == ".net" ? ".net" : ".NET" );

        if( std::filesystem::exists( net ) )
        {
            std::vector<std::set<std::string>> nets = parseNetTerminals( net.string() );

            if( !nets.empty() )
            {
                std::vector<std::set<std::string>> inconsistent;
                auto [consistent, checkableNets] = checkConnectivity( *schematic, nets, &inconsistent );
                netConsistent += consistent;
                netCheckable += checkableNets;
                netTotal += static_cast<int>( nets.size() );

                BOOST_TEST_MESSAGE( "          connectivity: " << consistent << "/" << checkableNets
                                                               << " nets consistent" );

                if( debug )
                {
                    for( const std::set<std::string>& terminals : inconsistent )
                    {
                        std::string joined;

                        for( const std::string& terminal : terminals )
                        {
                            if( !joined.empty() )
                                joined += ", ";

                            joined += terminal;
                        }

                        BOOST_TEST_MESSAGE( "          inconsistent net: " << joined );
                    }
                }
            }
        }
    }

    BOOST_TEST_MESSAGE( "==== OrCAD corpus summary ====" );
    BOOST_TEST_MESSAGE( "  designs: " << designs.size() << "  imported: " << imported << "  crashed: " << crashed
                                      << "  unsupported: " << unsupported << "  rejected: " << rejected );
    BOOST_TEST_MESSAGE( "  objects: pages "
                        << importedPages << "  components " << importedComponents << "  power " << importedPowerSymbols
                        << "  pins " << importedPins << "  wires " << importedWires << "  buses " << importedBuses
                        << "  labels " << importedLabels << "  shapes " << importedShapes << "  texts " << importedTexts
                        << "  bitmaps " << importedBitmaps );

    if( checked )
    {
        BOOST_TEST_MESSAGE( "  refdes coverage: " << totalMatched << "/" << totalExpected << " ("
                                                  << int( 100.0 * totalMatched / totalExpected )
                                                  << "%)  missing: " << totalMissing << "  extra: " << totalExtra );
    }

    if( netCheckable )
    {
        BOOST_TEST_MESSAGE( "  net connectivity: " << netConsistent << "/" << netCheckable << " ("
                                                   << int( 100.0 * netConsistent / netCheckable ) << "%)" );
    }

    // Only pre-2003 format may throw; anything else is a crash
    BOOST_CHECK_MESSAGE( crashed == 0, crashed << " design(s) crashed during import." );

    const char* snapshotEnv = std::getenv( "KICAD_ORCAD_CORPUS_SNAPSHOT" );

    // Guard against vacuous pass when no companion files present.
    if( designFilter.empty() )
        BOOST_REQUIRE_MESSAGE( checked > 0, "No ground-truth .BOM/.NET companions were validated." );

    if( designFilter.empty() && snapshotEnv && *snapshotEnv )
    {
        BOOST_CHECK_EQUAL( imported, 92 );
        BOOST_CHECK_EQUAL( rejected, 1 );
        BOOST_CHECK_EQUAL( importedPages, 854u );
        BOOST_CHECK_EQUAL( importedBitmaps, 616u );
    }

    // Occurrence-annotation decode holds this above 95%; dropped Hierarchy-stream ref overlay collapses it.
    if( checked )
        BOOST_CHECK_GE( 100.0 * totalMatched / totalExpected, 95.0 );

    // Pin-placement/geometry regression breaking connectivity collapses this. Checkable floor
    // (>= 2 resolvable terminals per net) stops broad pin loss passing vacuously.
    if( netTotal )
    {
        BOOST_CHECK_GE( 100.0 * netCheckable / netTotal, 80.0 );
        BOOST_CHECK_EQUAL( netConsistent, netCheckable );
    }
}


static std::filesystem::path findCorpusDesign( const std::filesystem::path& aRoot, const std::string& aFileName )
{
    for( const std::filesystem::directory_entry& entry : std::filesystem::recursive_directory_iterator( aRoot ) )
    {
        if( entry.is_regular_file() && entry.path().filename() == aFileName )
            return entry.path();
    }

    return {};
}


BOOST_AUTO_TEST_CASE( CisVariantFallbackUsesBytewiseFirstBomName )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn =
            std::filesystem::path( corpusEnv ) / "PADS" / "skyworks-eval" / "SI34061FB12V4KIT"
            / "si34061-evb-ext_1.7.1.20220111" / "schematic" / "SI34061-EVB-EXT.DSN";

    if( !std::filesystem::exists( dsn ) )
    {
        BOOST_TEST_MESSAGE( "SI34061-EVB-EXT 12 V design not present; skipping CIS variant check." );
        return;
    }

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    std::map<wxString, SCH_SYMBOL*> symbols;

    for( const SCH_SHEET_PATH& path : schematic->BuildSheetListSortedByPageNumbers() )
    {
        for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );
            symbols.emplace( symbol->GetRef( &path, false ), symbol );
        }
    }

    BOOST_REQUIRE( symbols.count( wxS( "U1" ) ) );
    BOOST_REQUIRE( symbols.count( wxS( "D13" ) ) );
    BOOST_REQUIRE( symbols.count( wxS( "T2" ) ) );
    BOOST_REQUIRE( symbols.count( wxS( "LB1" ) ) );
    BOOST_CHECK_EQUAL( symbols[wxS( "U1" )]->GetField( FIELD_T::VALUE )->GetText(),
                       wxS( "NVMFS5C680NLT1G" ) );
    BOOST_CHECK_EQUAL( symbols[wxS( "D13" )]->GetField( FIELD_T::VALUE )->GetText(), wxS( "PDS5100" ) );
    BOOST_CHECK_EQUAL( symbols[wxS( "T2" )]->GetField( FIELD_T::VALUE )->GetText(), wxS( "LDT1026-50R" ) );
    BOOST_CHECK_EQUAL( symbols[wxS( "LB1" )]->GetField( FIELD_T::VALUE )->GetText(),
                       wxS( "LABEL-Si34061-EVB-EXT-BOM-R1.7-12V" ) );
    BOOST_REQUIRE( symbols[wxS( "U1" )]->GetField( wxS( "Voltage" ) ) );
    BOOST_CHECK_EQUAL( symbols[wxS( "U1" )]->GetField( wxS( "Voltage" ) )->GetText(), wxS( "60V" ) );
    BOOST_REQUIRE( symbols.count( wxS( "D15" ) ) );
    BOOST_REQUIRE( symbols.count( wxS( "R35" ) ) );
    BOOST_REQUIRE( symbols.count( wxS( "TP1" ) ) );
    BOOST_CHECK( symbols[wxS( "D15" )]->GetDNP() );
    BOOST_CHECK( symbols[wxS( "R35" )]->GetDNP() );
    BOOST_CHECK( symbols[wxS( "TP1" )]->GetDNP() );
    BOOST_CHECK_EQUAL( symbols[wxS( "D15" )]->GetField( FIELD_T::VALUE )->GetText(), wxS( "NI" ) );
    for( const SCH_SHEET_PATH& path : schematic->BuildSheetListSortedByPageNumbers() )
    {
        bool variantNameFound = false;

        for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_TEXT_T ) )
        {
            const wxString& text = static_cast<SCH_TEXT*>( item )->GetText();
            BOOST_CHECK_NE( text, wxS( "<Core Design>" ) );
            variantNameFound |= text == wxS( "12V" );
        }

        BOOST_CHECK( variantNameFound );
        BOOST_CHECK_EQUAL( path.LastScreen()->GetTitleBlock().GetComment( 1 ), wxS( "Variant Name: 12V" ) );
    }
}


BOOST_AUTO_TEST_CASE( UnreferencedSchematicFoldersRemainVisibleButExcludedFromBoard )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "PROJET INDUS.DSN" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    SCH_SHEET_LIST sheets = schematic->BuildSheetListSortedByPageNumbers();
    size_t         excluded = 0;
    size_t         excludedSymbols = 0;

    for( const SCH_SHEET_PATH& path : sheets )
    {
        excluded += path.GetExcludedFromBoard();

        if( !path.GetExcludedFromBoard() )
            continue;

        for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            ++excludedSymbols;
            BOOST_CHECK( static_cast<SCH_SYMBOL*>( item )->GetExcludedFromBoard() );
        }
    }

    BOOST_CHECK_EQUAL( sheets.size(), 5u );
    BOOST_CHECK_EQUAL( excluded, 4u );
    BOOST_CHECK_GT( excludedSymbols, 0u );
}


BOOST_AUTO_TEST_CASE( ViewsDirectoryIgnoresStaleStoredFolders )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "DC1987A-2.DSN" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    SCH_SHEET_LIST sheets = schematic->BuildSheetListSortedByPageNumbers();
    BOOST_REQUIRE_EQUAL( sheets.size(), 1u );
}


BOOST_AUTO_TEST_CASE( OccurrenceFlatNetConnectsAcrossPagesWithoutOffpageSymbols )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "M5275EVB.DSN" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    auto [consistent, checkable] =
            checkConnectivity( *schematic, { { terminalToken( "J3", "38" ), terminalToken( "RP49", "7" ),
                                                terminalToken( "U6", "D10" ) } } );
    BOOST_CHECK_EQUAL( checkable, 1 );
    BOOST_CHECK_EQUAL( consistent, 1 );
}


static std::vector<std::string> collectImportedUuids( const SCHEMATIC& aSchematic )
{
    std::vector<std::string> uuids;

    for( SCH_SHEET* sheet : aSchematic.GetTopLevelSheets() )
    {
        uuids.push_back( sheet->m_Uuid.AsStdString() );

        if( SCH_SCREEN* screen = sheet->GetScreen() )
        {
            uuids.push_back( screen->GetUuid().AsStdString() );

            for( SCH_ITEM* item : screen->Items() )
            {
                uuids.push_back( item->m_Uuid.AsStdString() );

                if( item->Type() == SCH_SYMBOL_T )
                {
                    for( const std::unique_ptr<SCH_PIN>& pin : static_cast<SCH_SYMBOL*>( item )->GetRawPins() )
                    {
                        uuids.push_back( pin->m_Uuid.AsStdString() );
                    }
                }
                else if( item->Type() == SCH_SHEET_T )
                {
                    for( const SCH_SHEET_PIN* pin : static_cast<SCH_SHEET*>( item )->GetPins() )
                        uuids.push_back( pin->m_Uuid.AsStdString() );
                }
            }
        }
    }

    return uuids;
}


BOOST_AUTO_TEST_CASE( RepeatedImportHasDeterministicUuids )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
    {
        BOOST_TEST_MESSAGE( "KICAD_ORCAD_CORPUS not set; skipping OrCAD determinism check." );
        return;
    }

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "mc33163.dsn" );

    if( dsn.empty() )
    {
        BOOST_TEST_MESSAGE( "mc33163.dsn not present in corpus; skipping OrCAD determinism check." );
        return;
    }

    auto importUuids = [&]( const std::filesystem::path& aDsn )
    {
        std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
        SETTINGS_MANAGER           manager;
        manager.LoadProject( "" );
        schematic->SetProject( &manager.Prj() );
        schematic->CurrentSheet().clear();
        schematic->CurrentSheet().push_back( &schematic->Root() );

        SCH_IO_ORCAD plugin;
        plugin.LoadSchematicFile( aDsn.string(), schematic.get() );
        return collectImportedUuids( *schematic );
    };

    std::vector<std::string> first = importUuids( dsn );
    std::vector<std::string> second = importUuids( dsn );

    BOOST_REQUIRE( !first.empty() );
    BOOST_CHECK_EQUAL_COLLECTIONS( first.begin(), first.end(), second.begin(), second.end() );
}


BOOST_AUTO_TEST_CASE( AutoGeneratedNetNamesArePreserved )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
    {
        BOOST_TEST_MESSAGE( "KICAD_ORCAD_CORPUS not set; skipping generated net-name check." );
        return;
    }

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "CutiePi_V2.3-20210409.DSN" );

    if( dsn.empty() )
    {
        BOOST_TEST_MESSAGE( "CutiePi_V2.3-20210409.DSN not present in corpus; skipping generated "
                            "net-name check." );
        return;
    }

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    BOOST_CHECK_EQUAL( terminalNetName( *schematic, wxS( "D10" ), wxS( "A" ) ), wxS( "N12720539" ) );
    BOOST_CHECK_EQUAL( terminalNetName( *schematic, wxS( "D23" ), wxS( "1" ) ), wxS( "N132252170" ) );
}


BOOST_AUTO_TEST_CASE( CaptureNetIdsAndBlankUnitLettersAreAuthoritative )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
    {
        BOOST_TEST_MESSAGE( "KICAD_ORCAD_CORPUS not set; skipping Capture net/unit regression check." );
        return;
    }

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "BEAGLEBONEBLK_C3.DSN" );

    if( dsn.empty() )
    {
        BOOST_TEST_MESSAGE( "BEAGLEBONEBLK_C3.DSN not present in corpus; skipping Capture net/unit "
                            "regression check." );
        return;
    }

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    SCH_SHEET_LIST sheets = schematic->BuildSheetListSortedByPageNumbers();
    schematic->ConnectionGraph()->Recalculate( sheets, true );

    std::map<std::tuple<std::string, std::string, std::string>, int> terminalNets;
    int                                                              netId = 0;

    for( const auto& [key, subgraphs] : schematic->ConnectionGraph()->GetNetMap() )
    {
        for( CONNECTION_SUBGRAPH* subgraph : subgraphs )
        {
            std::string page = subgraph->GetSheet().LastScreen()->GetFileName().ToStdString();

            for( SCH_ITEM* item : subgraph->GetItems() )
            {
                if( item->Type() != SCH_PIN_T )
                    continue;

                SCH_PIN*    pin = static_cast<SCH_PIN*>( item );
                SCH_SYMBOL* symbol = dynamic_cast<SCH_SYMBOL*>( pin->GetParentSymbol() );

                if( symbol )
                {
                    terminalNets[{ page, symbol->GetRef( &subgraph->GetSheet(), false ).ToStdString(),
                                   pin->GetNumber().ToStdString() }] = netId;
                }
            }
        }

        ++netId;
    }

    auto findNet = [&]( const std::string& aPage, const std::string& aRef, const std::string& aPin )
    {
        for( const auto& [terminal, id] : terminalNets )
        {
            if( std::get<0>( terminal ).find( aPage ) != std::string::npos && std::get<1>( terminal ) == aRef
                && std::get<2>( terminal ) == aPin )
            {
                return id;
            }
        }

        return -1;
    };

    int ground = findNet( "AM335x 2_3, USB", "J1", "1" );
    int rx = findNet( "AM335x 2_3, USB", "J1", "4" );
    int tx = findNet( "AM335x 2_3, USB", "J1", "5" );

    BOOST_REQUIRE_NE( ground, -1 );
    BOOST_REQUIRE_NE( rx, -1 );
    BOOST_REQUIRE_NE( tx, -1 );
    BOOST_CHECK_NE( ground, rx );
    BOOST_CHECK_NE( ground, tx );
    BOOST_CHECK_NE( rx, tx );

    BOOST_CHECK_EQUAL( findNet( "LED, Config", "D3", "1" ), findNet( "LED, Config", "Q1", "3" ) );
    BOOST_CHECK_EQUAL( findNet( "LED, Config", "R77", "1" ), findNet( "LED, Config", "Q1", "5" ) );

    std::set<int> q1Units;
    std::set<wxString> q1ShownReferences;

    for( const SCH_SHEET_PATH& path : sheets )
    {
        if( path.LastScreen()->GetFileName().Find( wxS( "LED, Config" ) ) == wxNOT_FOUND )
            continue;

        for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

            if( symbol->GetRef( &path, false ) == wxS( "Q1" ) )
            {
                q1Units.insert( symbol->GetUnit() );
                q1ShownReferences.insert( symbol->GetRef( &path, true ) );
            }
        }
    }

    const std::set<int> expectedUnits = { 1, 2 };
    BOOST_CHECK_EQUAL_COLLECTIONS( q1Units.begin(), q1Units.end(), expectedUnits.begin(), expectedUnits.end() );

    const std::set<wxString> expectedShownReferences = { wxS( "Q1" ) };
    BOOST_CHECK_EQUAL_COLLECTIONS( q1ShownReferences.begin(), q1ShownReferences.end(),
                                   expectedShownReferences.begin(), expectedShownReferences.end() );
}


BOOST_AUTO_TEST_CASE( NamedWirelessPinUsesOccurrenceNetName )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
    {
        BOOST_TEST_MESSAGE( "KICAD_ORCAD_CORPUS not set; skipping named wireless-pin check." );
        return;
    }

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "CURRENT_SENSOR.DSN" );

    if( dsn.empty() )
    {
        BOOST_TEST_MESSAGE( "CURRENT_SENSOR.DSN not present in corpus; skipping named wireless-pin check." );
        return;
    }

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    BOOST_CHECK_EQUAL( terminalNetName( *schematic, wxS( "U1" ), wxS( "7" ) ), wxS( "VCC" ) );
}


BOOST_AUTO_TEST_CASE( ReservedDatasheetPropertyUsesStandardField )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "reComputer J202_V1.0.DSN" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );
    SCH_SYMBOL* resistor = nullptr;

    for( const SCH_SHEET_PATH& path : schematic->BuildSheetListSortedByPageNumbers() )
    {
        for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

            if( symbol->GetRef( &path, false ) == wxS( "R48" ) )
            {
                resistor = symbol;
                break;
            }
        }

        if( resistor )
            break;
    }

    BOOST_REQUIRE( resistor );
    size_t datasheetFields = std::count_if(
            resistor->GetFields().begin(), resistor->GetFields().end(),
            []( const SCH_FIELD& field ) { return field.GetName() == wxS( "Datasheet" ); } );
    BOOST_CHECK_EQUAL( datasheetFields, 1u );
    BOOST_CHECK_EQUAL(
            resistor->GetField( FIELD_T::DATASHEET )->GetText(),
            wxS( "Y:\\01_Cadence_Library\\05_Datasheet\\301010000_YAGEO_RC0402JR-070RL_Datasheet.pdf" ) );
}


BOOST_AUTO_TEST_CASE( ReservedFootprintPropertyUsesDistinctMetadataField )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "CY4532 Power Board Schematic.DSN" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );
    SCH_SYMBOL* capacitor = nullptr;

    for( const SCH_SHEET_PATH& path : schematic->BuildSheetListSortedByPageNumbers() )
    {
        for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

            if( symbol->GetRef( &path, false ) == wxS( "C45" ) )
            {
                capacitor = symbol;
                break;
            }
        }

        if( capacitor )
            break;
    }

    BOOST_REQUIRE( capacitor );
    size_t footprintFields = std::count_if(
            capacitor->GetFields().begin(), capacitor->GetFields().end(),
            []( const SCH_FIELD& field ) { return field.GetName() == wxS( "Footprint" ); } );
    BOOST_CHECK_EQUAL( footprintFields, 1u );
    BOOST_CHECK( capacitor->GetField( FIELD_T::FOOTPRINT )->GetText().IsEmpty() );

    SCH_FIELD* metadata = capacitor->GetField( wxS( "OrCAD Footprint Property" ) );
    BOOST_REQUIRE( metadata );
    BOOST_CHECK_EQUAL( metadata->GetText(), wxS( "0402" ) );
}


BOOST_AUTO_TEST_CASE( DisplayedPropertiesUseCaptureNameMatchingAndStandardFields )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "CY4532 Power Board Schematic.DSN" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );
    SCH_SYMBOL* capacitor = nullptr;
    SCH_SYMBOL* resistor = nullptr;
    SCH_SYMBOL* testPoint = nullptr;

    for( const SCH_SHEET_PATH& path : schematic->BuildSheetListSortedByPageNumbers() )
    {
        for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );
            wxString    reference = symbol->GetRef( &path, false );

            if( reference == wxS( "C104" ) )
                capacitor = symbol;
            else if( reference == wxS( "R58" ) )
                resistor = symbol;
            else if( reference == wxS( "TP8" ) )
                testPoint = symbol;
        }
    }

    BOOST_REQUIRE( capacitor );
    SCH_FIELD* voltage = capacitor->GetField( wxS( "Voltage" ) );
    BOOST_REQUIRE( voltage );
    BOOST_CHECK( voltage->IsVisible() );
    BOOST_CHECK( voltage->GetDrawRotation() == ANGLE_HORIZONTAL );
    BOOST_CHECK( voltage->GetPosition()
                 == OrcadDbuToIu( 722, 929 )
                            + VECTOR2I( 0, OrcadTextBaselineOffset( voltage->GetTextSize().y ) ) );

    BOOST_REQUIRE( resistor );
    BOOST_CHECK( resistor->GetField( FIELD_T::VALUE )->IsVisible() );

    BOOST_REQUIRE( testPoint );
    SCH_FIELD* description = testPoint->GetField( FIELD_T::DESCRIPTION );
    BOOST_REQUIRE( description );
    BOOST_CHECK( description->IsVisible() );
    BOOST_CHECK( description->GetDrawRotation() == ANGLE_HORIZONTAL );
    BOOST_CHECK( description->GetPosition()
                 == OrcadDbuToIu( 810, 945 )
                            + VECTOR2I( 0, OrcadTextBaselineOffset( description->GetTextSize().y ) ) );
}


BOOST_AUTO_TEST_CASE( DisplayedInheritedPartFieldsArePreserved )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "DC1859A-2.DSN" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );
    SCH_SYMBOL* capacitor = nullptr;

    for( const SCH_SHEET_PATH& path : schematic->BuildSheetListSortedByPageNumbers() )
    {
        for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

            if( symbol->GetRef( &path, false ) == wxS( "C2" ) )
            {
                capacitor = symbol;
                break;
            }
        }

        if( capacitor )
            break;
    }

    BOOST_REQUIRE( capacitor );
    SCH_FIELD* voltage = capacitor->GetField( wxS( "1st Part Field" ) );
    BOOST_REQUIRE( voltage );
    BOOST_CHECK_EQUAL( voltage->GetText(), wxS( "10V" ) );
    BOOST_CHECK( voltage->IsVisible() );
}


BOOST_AUTO_TEST_CASE( DisplayTypeTwoPropertiesRenderNamesAndValues )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "SCH-48278.DSN" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );
    SCH_FIELD* jumper = nullptr;
    bool       renderedJumper = false;

    for( const SCH_SHEET_PATH& path : schematic->BuildSheetListSortedByPageNumbers() )
    {
        for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

            if( symbol->GetRef( &path, false ) == wxS( "J2" ) )
            {
                jumper = symbol->GetField( wxS( "JUMPER(DEFAULT)" ) );
                break;
            }
        }

        for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_TEXT_T ) )
        {
            SCH_TEXT* text = static_cast<SCH_TEXT*>( item );
            renderedJumper = renderedJumper
                             || text->GetText() == wxS( "JUMPER(DEFAULT) = OFF:POLARITY_SEL_L" );
        }

        if( jumper )
            break;
    }

    BOOST_REQUIRE( jumper );
    BOOST_CHECK_EQUAL( jumper->GetText(), wxS( "OFF:POLARITY_SEL_L" ) );
    BOOST_CHECK( renderedJumper );
}


BOOST_AUTO_TEST_CASE( DisplayTypeFourPropertiesShowOnlyTheirValues )
{
    ORCAD_DISPLAY_PROP property;
    property.dispMode = 0x400;

    BOOST_CHECK( OrcadDisplayPropVisible( property ) );
    BOOST_CHECK( !OrcadDisplayPropShowsName( property ) );
    BOOST_CHECK( OrcadDisplayPropShowsValue( property ) );
}


BOOST_AUTO_TEST_CASE( DisplayTypeThreePropertiesShowOnlyTheirNames )
{
    ORCAD_DISPLAY_PROP property;
    property.dispMode = 0x300;

    BOOST_CHECK( OrcadDisplayPropVisible( property ) );
    BOOST_CHECK( OrcadDisplayPropShowsName( property ) );
    BOOST_CHECK( !OrcadDisplayPropShowsValue( property ) );
}


BOOST_AUTO_TEST_CASE( SimulationResultPropertiesAreNotPersistentGraphics )
{
    ORCAD_DISPLAY_PROP property;
    property.dispMode = 0x100;

    property.name = "BiasValue Power";
    BOOST_CHECK( !OrcadDisplayPropVisible( property ) );

    property.name = "BiasValue Current";
    BOOST_CHECK( !OrcadDisplayPropVisible( property ) );

    property.name = "BiasValue Voltage";
    BOOST_CHECK( !OrcadDisplayPropVisible( property ) );
}


BOOST_AUTO_TEST_CASE( GeneratedWirelessNetOverridesPeerPinName )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
    {
        BOOST_TEST_MESSAGE( "KICAD_ORCAD_CORPUS not set; skipping wireless peer-net check." );
        return;
    }

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "CY3280_MBR3 EVK Schematic.DSN" );

    if( dsn.empty() )
    {
        BOOST_TEST_MESSAGE( "CY3280_MBR3 EVK Schematic.DSN not present; skipping wireless peer-net check." );
        return;
    }

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    SCH_SHEET_LIST sheets = schematic->BuildSheetListSortedByPageNumbers();
    schematic->ConnectionGraph()->Recalculate( sheets, true );
    std::map<std::pair<wxString, wxString>, CONNECTION_SUBGRAPH*> pinNets;

    for( const auto& [key, subgraphs] : schematic->ConnectionGraph()->GetNetMap() )
    {
        for( CONNECTION_SUBGRAPH* subgraph : subgraphs )
        {
            for( SCH_ITEM* item : subgraph->GetItems() )
            {
                if( item->Type() != SCH_PIN_T )
                    continue;

                SCH_PIN*    pin = static_cast<SCH_PIN*>( item );
                SCH_SYMBOL* symbol = dynamic_cast<SCH_SYMBOL*>( pin->GetParentSymbol() );

                if( symbol )
                    pinNets[{ symbol->GetRef( &subgraph->GetSheet(), false ), pin->GetNumber() }] = subgraph;
            }
        }
    }

    BOOST_REQUIRE_EQUAL( pinNets.count( { wxS( "R51" ), wxS( "1" ) } ), 1u );
    BOOST_REQUIRE_EQUAL( pinNets.count( { wxS( "R51" ), wxS( "2" ) } ), 1u );
    BOOST_REQUIRE_EQUAL( pinNets.count( { wxS( "R52" ), wxS( "1" ) } ), 1u );
    BOOST_REQUIRE_EQUAL( pinNets.count( { wxS( "R52" ), wxS( "2" ) } ), 1u );
    CONNECTION_SUBGRAPH* r51Pin1 = pinNets.at( { wxS( "R51" ), wxS( "1" ) } );
    CONNECTION_SUBGRAPH* r51Pin2 = pinNets.at( { wxS( "R51" ), wxS( "2" ) } );
    CONNECTION_SUBGRAPH* r52Pin1 = pinNets.at( { wxS( "R52" ), wxS( "1" ) } );
    CONNECTION_SUBGRAPH* r52Pin2 = pinNets.at( { wxS( "R52" ), wxS( "2" ) } );

    BOOST_CHECK_NE( r51Pin1, r51Pin2 );
    BOOST_CHECK_NE( r52Pin1, r52Pin2 );
}


BOOST_AUTO_TEST_CASE( GlobalNetNamesAreCaseInsensitive )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
    {
        BOOST_TEST_MESSAGE( "KICAD_ORCAD_CORPUS not set; skipping global-net case check." );
        return;
    }

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "BeagleBoard-xM_ORCAD.DSN" );

    if( dsn.empty() )
    {
        BOOST_TEST_MESSAGE( "BeagleBoard-xM_ORCAD.DSN not present in corpus; skipping global-net case check." );
        return;
    }

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    SCH_SHEET_LIST sheets = schematic->BuildSheetListSortedByPageNumbers();
    schematic->ConnectionGraph()->Recalculate( sheets, true );

    std::map<std::tuple<std::string, std::string, std::string>, int> terminalNets;
    int                                                              netId = 0;

    for( const auto& [key, subgraphs] : schematic->ConnectionGraph()->GetNetMap() )
    {
        for( CONNECTION_SUBGRAPH* subgraph : subgraphs )
        {
            std::string page = subgraph->GetSheet().LastScreen()->GetFileName().ToStdString();

            for( SCH_ITEM* item : subgraph->GetItems() )
            {
                if( item->Type() != SCH_PIN_T )
                    continue;

                SCH_PIN*    pin = static_cast<SCH_PIN*>( item );
                SCH_SYMBOL* symbol = dynamic_cast<SCH_SYMBOL*>( pin->GetParentSymbol() );

                if( symbol )
                {
                    terminalNets[{ page, symbol->GetRef( &subgraph->GetSheet(), false ).ToStdString(),
                                   pin->GetNumber().ToStdString() }] = netId;
                }
            }
        }

        ++netId;
    }

    auto findNet = [&]( const std::string& aPage, const std::string& aRef, const std::string& aPin )
    {
        for( const auto& [terminal, id] : terminalNets )
        {
            if( std::get<0>( terminal ).find( aPage ) != std::string::npos && std::get<1>( terminal ) == aRef
                && std::get<2>( terminal ) == aPin )
            {
                return id;
            }
        }

        return -1;
    };

    int processor = findNet( "PROCESSOR_C", "C75", "1" );
    int power = findNet( "PMIC _POWER", "C122", "1" );

    BOOST_REQUIRE_NE( processor, -1 );
    BOOST_REQUIRE_NE( power, -1 );
    BOOST_CHECK_EQUAL( processor, power );
}


BOOST_AUTO_TEST_CASE( CaptureBusRangesUseKiCadSyntax )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
    {
        BOOST_TEST_MESSAGE( "KICAD_ORCAD_CORPUS not set; skipping bus-range syntax check." );
        return;
    }

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "parallella_e16_z7020_schematic.dsn" );

    if( dsn.empty() )
    {
        BOOST_TEST_MESSAGE( "parallella_e16_z7020_schematic.dsn not present in corpus; skipping bus-range check." );
        return;
    }

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    bool found = false;

    for( const SCH_SHEET_PATH& path : schematic->BuildSheetListSortedByPageNumbers() )
    {
        for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_GLOBAL_LABEL_T ) )
        {
            wxString text = static_cast<SCH_GLOBALLABEL*>( item )->GetText();
            BOOST_CHECK_NE( text, wxS( "DDR_DQ[31:0]" ) );
            found |= text == wxS( "DDR_DQ[31..0]" );
        }
    }

    BOOST_CHECK( found );
}


BOOST_AUTO_TEST_CASE( CanonicalPropertiesIgnoreCaseInsensitiveDuplicates )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "parallella_e16_z7020_schematic.dsn" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );
    SCH_SYMBOL* platedHole = nullptr;

    for( const SCH_SHEET_PATH& path : schematic->BuildSheetListSortedByPageNumbers() )
    {
        for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

            if( symbol->GetRef( &path, false ) == wxS( "PTH1" ) )
            {
                platedHole = symbol;
                break;
            }
        }

        if( platedHole )
            break;
    }

    BOOST_REQUIRE( platedHole );
    size_t valueFields = std::count_if(
            platedHole->GetFields().begin(), platedHole->GetFields().end(),
            []( const SCH_FIELD& aField ) { return aField.GetName().CmpNoCase( wxS( "Value" ) ) == 0; } );
    BOOST_CHECK_EQUAL( valueFields, 1u );
    BOOST_CHECK_EQUAL( platedHole->GetField( FIELD_T::VALUE )->GetShownText( true ), wxS( "PTH125_200PAD" ) );
}


BOOST_AUTO_TEST_CASE( CollidingOccurrenceAliasesRemainSeparate )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "parallella_e16_z7020_schematic.dsn" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    SCH_SHEET_LIST sheets = schematic->BuildSheetListSortedByPageNumbers();
    schematic->ConnectionGraph()->Recalculate( sheets, true );

    std::map<std::string, int> terminalNets;
    int                        netId = 0;

    for( const auto& [key, subgraphs] : schematic->ConnectionGraph()->GetNetMap() )
    {
        for( CONNECTION_SUBGRAPH* subgraph : subgraphs )
        {
            for( SCH_ITEM* item : subgraph->GetItems() )
            {
                if( item->Type() != SCH_PIN_T )
                    continue;

                SCH_PIN*    pin = static_cast<SCH_PIN*>( item );
                SCH_SYMBOL* symbol = dynamic_cast<SCH_SYMBOL*>( pin->GetParentSymbol() );

                if( symbol )
                {
                    terminalNets[terminalToken( symbol->GetRef( &subgraph->GetSheet(), false ).ToStdString(),
                                                 pin->GetNumber().ToStdString() )] = netId;
                }
            }
        }

        ++netId;
    }

    BOOST_REQUIRE( terminalNets.count( "R41.2" ) );
    BOOST_REQUIRE( terminalNets.count( "U26.53" ) );
    BOOST_REQUIRE( terminalNets.count( "R56.2" ) );
    BOOST_REQUIRE( terminalNets.count( "U11.1" ) );
    BOOST_REQUIRE( terminalNets.count( "U11.3" ) );
    BOOST_REQUIRE( terminalNets.count( "U14.1" ) );
    BOOST_CHECK_EQUAL( terminalNets["R41.2"], terminalNets["U26.53"] );
    BOOST_CHECK_EQUAL( terminalNets["R56.2"], terminalNets["U11.1"] );
    BOOST_CHECK_EQUAL( terminalNets["R56.2"], terminalNets["U11.3"] );
    BOOST_CHECK_EQUAL( terminalNets["R56.2"], terminalNets["U14.1"] );
    BOOST_CHECK_NE( terminalNets["R41.2"], terminalNets["R56.2"] );
}


BOOST_AUTO_TEST_CASE( CollidingOffpageOccurrenceAliasesRemainSeparate )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "parallella_gen0.dsn" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    auto [consistent, checkable] = checkConnectivity(
            *schematic,
            { { terminalToken( "R41", "2" ), terminalToken( "U26", "53" ) },
              { terminalToken( "R57", "2" ), terminalToken( "R97", "1" ), terminalToken( "U14", "1" ) } } );
    BOOST_CHECK_EQUAL( checkable, 2 );
    BOOST_CHECK_EQUAL( consistent, 2 );
}


BOOST_AUTO_TEST_CASE( FlatFolderOccurrenceAliasesConnectDisplacedPins )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "SCH-20380.DSN" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    auto [consistent, checkable] = checkConnectivity(
            *schematic, { { terminalToken( "J10", "29" ), terminalToken( "RP15", "3" ), terminalToken( "U10", "R9" ),
                            terminalToken( "U33", "30" ) },
                          { terminalToken( "J10", "30" ), terminalToken( "JP62", "1" ), terminalToken( "RP16", "1" ),
                            terminalToken( "U10", "P9" ) },
                          { terminalToken( "J7", "50" ), terminalToken( "RP14", "5" ), terminalToken( "U10", "D12" ),
                            terminalToken( "U33", "21" ), terminalToken( "U5", "C" ) } } );
    BOOST_CHECK_EQUAL( checkable, 3 );
    BOOST_CHECK_EQUAL( consistent, 3 );
}


BOOST_AUTO_TEST_CASE( PowerNetNameWinsOverSecondaryOccurrencePortAlias )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "SCH-38863_CX1.DSN" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    BOOST_CHECK_EQUAL( terminalNetName( *schematic, wxS( "J16" ), wxS( "1" ) ).Lower(), wxString( wxS( "gndisohu" ) ) );
    BOOST_CHECK_EQUAL( terminalNetName( *schematic, wxS( "J17" ), wxS( "1" ) ).Lower(), wxString( wxS( "gndisohv" ) ) );
    BOOST_CHECK_EQUAL( terminalNetName( *schematic, wxS( "J18" ), wxS( "1" ) ).Lower(), wxString( wxS( "gndisohw" ) ) );

    std::set<wxString> globalNames;

    for( const SCH_SHEET_PATH& path : schematic->BuildSheetListSortedByPageNumbers() )
    {
        for( SCH_ITEM* item : path.LastScreen()->Items() )
        {
            if( item->Type() == SCH_GLOBAL_LABEL_T )
                globalNames.insert( static_cast<SCH_LABEL_BASE*>( item )->GetText().Lower() );
        }
    }

    BOOST_CHECK( globalNames.count( wxS( "gndisohu" ) ) );
    BOOST_CHECK( globalNames.count( wxS( "gndisohv" ) ) );
    BOOST_CHECK( globalNames.count( wxS( "gndisohw" ) ) );
    BOOST_CHECK( !globalNames.count( wxS( "ntcu2" ) ) );
    BOOST_CHECK( !globalNames.count( wxS( "ntcv2" ) ) );
    BOOST_CHECK( !globalNames.count( wxS( "ntcw2" ) ) );
}


BOOST_AUTO_TEST_CASE( DuplicatePageNetIdsPreserveAliases )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
    {
        BOOST_TEST_MESSAGE( "KICAD_ORCAD_CORPUS not set; skipping duplicate page-net check." );
        return;
    }

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "MTB4.DSN" );

    if( dsn.empty() )
    {
        BOOST_TEST_MESSAGE( "MTB4.DSN not present in corpus; skipping duplicate page-net check." );
        return;
    }

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    SCH_SHEET_LIST sheets = schematic->BuildSheetListSortedByPageNumbers();
    schematic->ConnectionGraph()->Recalculate( sheets, true );

    std::map<std::string, wxString> pinNets;

    for( const auto& [key, subgraphs] : schematic->ConnectionGraph()->GetNetMap() )
    {
        for( CONNECTION_SUBGRAPH* subgraph : subgraphs )
        {
            if( subgraph->GetSheet().LastScreen()->GetFileName().Find( wxS( "PAGE 2 - MCU" ) ) == wxNOT_FOUND )
                continue;

            for( SCH_ITEM* item : subgraph->GetItems() )
            {
                if( item->Type() != SCH_PIN_T )
                    continue;

                SCH_PIN*    pin = static_cast<SCH_PIN*>( item );
                SCH_SYMBOL* symbol = dynamic_cast<SCH_SYMBOL*>( pin->GetParentSymbol() );

                if( symbol && symbol->GetRef( &subgraph->GetSheet(), false ) == wxS( "U1" ) )
                    pinNets[pin->GetNumber().ToStdString()] = key.Name;
            }
        }
    }

    BOOST_REQUIRE_EQUAL( pinNets.count( "F3" ), 1u );
    BOOST_REQUIRE_EQUAL( pinNets.count( "F8" ), 1u );
    BOOST_REQUIRE_EQUAL( pinNets.count( "G3" ), 1u );
    BOOST_REQUIRE_EQUAL( pinNets.count( "G7" ), 1u );
    BOOST_CHECK( pinNets["F3"].EndsWith( wxS( "I2C2_SDA" ) ) );
    BOOST_CHECK( pinNets["F8"].EndsWith( wxS( "I2C2_SDA" ) ) );
    BOOST_CHECK( pinNets["G3"].EndsWith( wxS( "I2C2_SCL" ) ) );
    BOOST_CHECK( pinNets["G7"].EndsWith( wxS( "I2C2_SCL" ) ) );
}


BOOST_AUTO_TEST_CASE( DuplicatePageNetIdsDoNotShortDistinctNets )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
    {
        BOOST_TEST_MESSAGE( "KICAD_ORCAD_CORPUS not set; skipping duplicate page-net isolation check." );
        return;
    }

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "DC2125A-2.DSN" );

    if( dsn.empty() )
    {
        BOOST_TEST_MESSAGE( "DC2125A-2.DSN not present; skipping duplicate page-net isolation check." );
        return;
    }

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    SCH_SHEET_LIST sheets = schematic->BuildSheetListSortedByPageNumbers();
    schematic->ConnectionGraph()->Recalculate( sheets, true );
    std::map<std::pair<wxString, wxString>, CONNECTION_SUBGRAPH*> pinNets;

    for( const auto& [key, subgraphs] : schematic->ConnectionGraph()->GetNetMap() )
    {
        for( CONNECTION_SUBGRAPH* subgraph : subgraphs )
        {
            for( SCH_ITEM* item : subgraph->GetItems() )
            {
                if( item->Type() != SCH_PIN_T )
                    continue;

                SCH_PIN*    pin = static_cast<SCH_PIN*>( item );
                SCH_SYMBOL* symbol = dynamic_cast<SCH_SYMBOL*>( pin->GetParentSymbol() );

                if( symbol )
                    pinNets[{ symbol->GetRef( &subgraph->GetSheet(), false ), pin->GetNumber() }] = subgraph;
            }
        }
    }

    CONNECTION_SUBGRAPH* earth = pinNets.at( { wxS( "J2" ), wxS( "9" ) } );
    CONNECTION_SUBGRAPH* vportn = pinNets.at( { wxS( "C1" ), wxS( "2" ) } );
    BOOST_CHECK_NE( earth, vportn );
}


BOOST_AUTO_TEST_CASE( AliasAtCrossingDoesNotShortCaptureNets )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
    {
        BOOST_TEST_MESSAGE( "KICAD_ORCAD_CORPUS not set; skipping alias-at-crossing isolation check." );
        return;
    }

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "OC_CONNECT1_FRONTEND_REV_C_V1P1.DSN" );

    if( dsn.empty() )
    {
        BOOST_TEST_MESSAGE( "OC_CONNECT1_FRONTEND_REV_C_V1P1.DSN not present; skipping alias-at-crossing check." );
        return;
    }

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    SCH_SHEET_LIST sheets = schematic->BuildSheetListSortedByPageNumbers();
    schematic->ConnectionGraph()->Recalculate( sheets, true );
    std::map<std::pair<wxString, wxString>, CONNECTION_SUBGRAPH*> pinNets;

    for( const auto& [key, subgraphs] : schematic->ConnectionGraph()->GetNetMap() )
    {
        for( CONNECTION_SUBGRAPH* subgraph : subgraphs )
        {
            for( SCH_ITEM* item : subgraph->GetItems() )
            {
                if( item->Type() != SCH_PIN_T )
                    continue;

                SCH_PIN*    pin = static_cast<SCH_PIN*>( item );
                SCH_SYMBOL* symbol = dynamic_cast<SCH_SYMBOL*>( pin->GetParentSymbol() );

                if( symbol )
                    pinNets[{ symbol->GetRef( &subgraph->GetSheet(), false ), pin->GetNumber() }] = subgraph;
            }
        }
    }

    CONNECTION_SUBGRAPH* a0 = pinNets.at( { wxS( "U7157" ), wxS( "3" ) } );
    CONNECTION_SUBGRAPH* a2 = pinNets.at( { wxS( "U7157" ), wxS( "5" ) } );
    CONNECTION_SUBGRAPH* io1 = pinNets.at( { wxS( "U7157" ), wxS( "7" ) } );
    BOOST_CHECK_EQUAL( a0, pinNets.at( { wxS( "R1150" ), wxS( "2" ) } ) );
    BOOST_CHECK_EQUAL( a0, pinNets.at( { wxS( "R1153" ), wxS( "1" ) } ) );
    BOOST_CHECK_EQUAL( a2, pinNets.at( { wxS( "R1152" ), wxS( "2" ) } ) );
    BOOST_CHECK_EQUAL( a2, pinNets.at( { wxS( "R1155" ), wxS( "1" ) } ) );
    BOOST_CHECK_EQUAL( io1, pinNets.at( { wxS( "R954" ), wxS( "2" ) } ) );
    BOOST_CHECK_NE( a0, a2 );
    BOOST_CHECK_NE( a0, io1 );
    BOOST_CHECK_NE( a2, io1 );
}


BOOST_AUTO_TEST_CASE( WirelessPinUsesPageNetId )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
    {
        BOOST_TEST_MESSAGE( "KICAD_ORCAD_CORPUS not set; skipping wireless page-net check." );
        return;
    }

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "Si828X-BW-GDB.DSN" );

    if( dsn.empty() )
    {
        BOOST_TEST_MESSAGE( "Si828X-BW-GDB.DSN not present; skipping wireless page-net check." );
        return;
    }

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    SCH_SHEET_LIST sheets = schematic->BuildSheetListSortedByPageNumbers();
    schematic->ConnectionGraph()->Recalculate( sheets, true );
    std::map<std::pair<wxString, wxString>, wxString> pinNames;

    for( const auto& [key, subgraphs] : schematic->ConnectionGraph()->GetNetMap() )
    {
        for( CONNECTION_SUBGRAPH* subgraph : subgraphs )
        {
            for( SCH_ITEM* item : subgraph->GetItems() )
            {
                if( item->Type() != SCH_PIN_T )
                    continue;

                SCH_PIN*    pin = static_cast<SCH_PIN*>( item );
                SCH_SYMBOL* symbol = dynamic_cast<SCH_SYMBOL*>( pin->GetParentSymbol() );

                if( symbol )
                    pinNames[{ symbol->GetRef( &subgraph->GetSheet(), false ), pin->GetNumber() }] = key.Name;
            }
        }
    }

    BOOST_CHECK_EQUAL( pinNames.at( { wxS( "CB28" ), wxS( "2" ) } ), wxS( "5V" ) );
    BOOST_CHECK_EQUAL( pinNames.at( { wxS( "RT17" ), wxS( "2" ) } ), wxS( "5V" ) );
}


BOOST_AUTO_TEST_CASE( AmbiguousWirelessPinUsesOccurrenceNetName )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
    {
        BOOST_TEST_MESSAGE( "KICAD_ORCAD_CORPUS not set; skipping ambiguous wireless-net check." );
        return;
    }

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "DC2047A-3-A.DSN" );

    if( dsn.empty() )
    {
        BOOST_TEST_MESSAGE( "DC2047A-3-A.DSN not present; skipping ambiguous wireless-net check." );
        return;
    }

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    SCH_SHEET_LIST sheets = schematic->BuildSheetListSortedByPageNumbers();
    schematic->ConnectionGraph()->Recalculate( sheets, true );
    wxString d15Pin2;

    for( const auto& [key, subgraphs] : schematic->ConnectionGraph()->GetNetMap() )
    {
        for( CONNECTION_SUBGRAPH* subgraph : subgraphs )
        {
            for( SCH_ITEM* item : subgraph->GetItems() )
            {
                if( item->Type() != SCH_PIN_T )
                    continue;

                SCH_PIN*    pin = static_cast<SCH_PIN*>( item );
                SCH_SYMBOL* symbol = dynamic_cast<SCH_SYMBOL*>( pin->GetParentSymbol() );

                if( symbol && symbol->GetRef( &subgraph->GetSheet(), false ) == wxS( "D15" )
                    && pin->GetNumber() == wxS( "2" ) )
                {
                    d15Pin2 = key.Name;
                }
            }
        }
    }

    BOOST_CHECK_EQUAL( d15Pin2, wxS( "AUX_RECTIFIED-" ) );
}


BOOST_AUTO_TEST_CASE( BlankPackagePinNumbersUseLogicalNames )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "DC2084A-3.DSN" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    std::map<wxString, std::set<wxString>> pinNumbers;

    for( const SCH_SHEET_PATH& sheet : schematic->BuildSheetListSortedByPageNumbers() )
    {
        for( SCH_ITEM* item : sheet.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

            wxString reference = symbol->GetRef( &sheet, false );

            if( reference != wxS( "C31" ) && reference != wxS( "C42" ) )
                continue;

            for( const std::unique_ptr<SCH_PIN>& pin : symbol->GetRawPins() )
                pinNumbers[reference].insert( pin->GetNumber() );
        }
    }

    BOOST_CHECK( pinNumbers[wxS( "C31" )] == std::set<wxString>( { wxS( "1" ), wxS( "2" ) } ) );
    BOOST_CHECK( pinNumbers[wxS( "C42" )] == std::set<wxString>( { wxS( "1" ), wxS( "2" ) } ) );
}


BOOST_AUTO_TEST_CASE( FallbackPackagePrefersNumberedPins )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "DC2091A-3.DSN" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    std::map<wxString, std::set<wxString>> pinNumbers;

    for( const SCH_SHEET_PATH& sheet : schematic->BuildSheetListSortedByPageNumbers() )
    {
        for( SCH_ITEM* item : sheet.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );
            wxString    reference = symbol->GetRef( &sheet, false );

            if( reference != wxS( "C1" ) && reference != wxS( "C2" ) )
                continue;

            for( const std::unique_ptr<SCH_PIN>& pin : symbol->GetRawPins() )
                pinNumbers[reference].insert( pin->GetNumber() );
        }
    }

    const std::set<wxString> expected = { wxS( "1" ), wxS( "2" ) };
    BOOST_CHECK( pinNumbers[wxS( "C1" )] == expected );
    BOOST_CHECK( pinNumbers[wxS( "C2" )] == expected );
}


BOOST_AUTO_TEST_CASE( FallbackPackageUsesLogicalPinOrder )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "DC2228A-3.DSN" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    std::vector<std::set<std::string>> expected = { { terminalToken( "R188", "2" ), terminalToken( "C100", "2" ) } };
    auto [consistent, checkable] = checkConnectivity( *schematic, expected );
    BOOST_CHECK_EQUAL( checkable, 1 );
    BOOST_CHECK_EQUAL( consistent, 1 );
}


BOOST_AUTO_TEST_CASE( NumericUnitNamesUseNaturalOrder )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "DC2228A-3.DSN" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    std::map<int, size_t> pinCounts;

    for( const SCH_SHEET_PATH& sheet : schematic->BuildSheetListSortedByPageNumbers() )
    {
        for( SCH_ITEM* item : sheet.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

            if( symbol->GetRef( &sheet, false ) == wxS( "U9" ) )
                pinCounts[symbol->GetUnit()] = symbol->GetPins().size();
        }
    }

    const std::map<int, size_t> expected = {
        { 1, 2 },   { 2, 21 }, { 3, 14 }, { 4, 9 },  { 5, 11 }, { 6, 8 },   { 7, 13 },
        { 8, 8 },   { 9, 9 },  { 10, 7 }, { 11, 5 }, { 12, 7 }, { 13, 15 }, { 14, 17 },
        { 15, 16 }, { 16, 12 }, { 17, 32 }, { 18, 6 }, { 19, 22 }, { 20, 22 },
    };
    BOOST_CHECK( pinCounts == expected );
}


BOOST_AUTO_TEST_CASE( OccurrenceReferencesOverridePlacedTemplateReferences )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "SI82AX-CX_NB8_EVB.DSN" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    bool     foundR2 = false;
    bool     foundR17 = false;
    wxString refsAtR2Position;
    wxString refsAtR17Position;

    for( const SCH_SHEET_PATH& sheet : schematic->BuildSheetListSortedByPageNumbers() )
    {
        for( SCH_ITEM* item : sheet.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );
            wxString    reference = symbol->GetRef( &sheet, false );

            if( symbol->GetPosition() == OrcadDbuToIu( 890, 200 ) )
            {
                refsAtR2Position += reference + wxS( " " );
                foundR2 = foundR2 || reference == wxS( "R2" );
            }

            if( symbol->GetPosition() == OrcadDbuToIu( 1060, 395 ) )
            {
                refsAtR17Position += reference + wxS( " " );
                foundR17 = foundR17 || reference == wxS( "R17" );
            }
        }
    }

    BOOST_CHECK_MESSAGE( foundR2, "references at R2 position: " << refsAtR2Position );
    BOOST_CHECK_MESSAGE( foundR17, "references at R17 position: " << refsAtR17Position );
}


BOOST_AUTO_TEST_CASE( RootOccurrenceTargetIdsOverridePlacedTemplateReferences )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "Low EMI demo board.DSN" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    std::set<std::string> refs = collectImportedRefs( *schematic );

    BOOST_CHECK( refs.count( "C1" ) );
    BOOST_CHECK( refs.count( "U1" ) );
    BOOST_CHECK( !refs.count( "C71" ) );
    BOOST_CHECK( !refs.count( "U12" ) );
}


BOOST_AUTO_TEST_CASE( FlatModernOccurrenceReferencesOverridePlacedReferences )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn;

    for( const std::filesystem::directory_entry& entry :
         std::filesystem::recursive_directory_iterator( corpusEnv ) )
    {
        if( entry.is_regular_file() && entry.path().filename() == "BDC.DSN"
            && entry.path().string().find( "backpack-bdc" ) != std::string::npos )
        {
            dsn = entry.path();
            break;
        }
    }

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    wxString reference;

    for( const SCH_SHEET_PATH& sheet : schematic->BuildSheetListSortedByPageNumbers() )
    {
        for( SCH_ITEM* item : sheet.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

            if( symbol->GetPosition() == OrcadDbuToIu( 485, 270 ) )
                reference = symbol->GetRef( &sheet, false );
        }
    }

    BOOST_CHECK_EQUAL( reference, wxString( "U10" ) );
}


BOOST_AUTO_TEST_CASE( SuperSpeedOccurrencePropertiesOverrideReusableComponents )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "SuperSpeed Explorer Kit Schematic.DSN" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    std::map<wxString, SCH_SYMBOL*> switches;

    for( const SCH_SHEET_PATH& sheet : schematic->BuildSheetListSortedByPageNumbers() )
    {
        for( SCH_ITEM* item : sheet.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );
            wxString    reference = symbol->GetRef( &sheet, false );

            if( reference == wxS( "SW1" ) || reference == wxS( "SW2" ) )
                switches[reference] = symbol;
        }
    }

    BOOST_REQUIRE_EQUAL( switches.size(), 2u );

    for( const wxString& reference : { wxS( "SW1" ), wxS( "SW2" ) } )
    {
        SCH_SYMBOL* symbol = switches.at( reference );
        BOOST_CHECK_EQUAL( symbol->GetField( FIELD_T::VALUE )->GetText(), wxS( "434 123 050 816" ) );
        BOOST_REQUIRE( symbol->GetField( wxS( "Manufacturer" ) ) );
        BOOST_CHECK_EQUAL( symbol->GetField( wxS( "Manufacturer" ) )->GetText(),
                           wxS( "Wurth Electronics Inc" ) );
        BOOST_REQUIRE( symbol->GetField( wxS( "OrCAD Footprint" ) ) );
        BOOST_CHECK_EQUAL( symbol->GetField( wxS( "OrCAD Footprint" ) )->GetText(), wxS( "EVQ-PE105K" ) );
    }
}


BOOST_AUTO_TEST_CASE( EmptyOccurrencePropertiesClearTemplateMetadata )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    auto checkFieldCleared = [&]( const wxString& aFileName, const wxString& aReference,
                                  const wxString& aFieldName )
    {
        std::filesystem::path dsn = findCorpusDesign( corpusEnv, aFileName.ToStdString() );
        BOOST_REQUIRE( !dsn.empty() );

        std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
        SETTINGS_MANAGER           manager;
        manager.LoadProject( "" );
        schematic->SetProject( &manager.Prj() );
        schematic->CurrentSheet().clear();
        schematic->CurrentSheet().push_back( &schematic->Root() );

        SCH_IO_ORCAD plugin;
        plugin.LoadSchematicFile( dsn.string(), schematic.get() );

        SCH_FIELD* field = nullptr;

        for( const SCH_SHEET_PATH& sheet : schematic->BuildSheetListSortedByPageNumbers() )
        {
            for( SCH_ITEM* item : sheet.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
            {
                SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

                if( symbol->GetRef( &sheet, false ) == aReference )
                    field = symbol->GetField( aFieldName );
            }
        }

        BOOST_CHECK( !field || field->GetText().IsEmpty() );
    };

    checkFieldCleared( wxS( "DC2596A-3.DSN" ), wxS( "L2" ), wxS( "4th Part Field" ) );
    checkFieldCleared( wxS( "CY4532 Power Board Schematic.DSN" ), wxS( "J2" ), wxS( "PART_NUMBER" ) );
}


BOOST_AUTO_TEST_CASE( CapturePseudoGlobalWirelessPinsConnectAcrossPages )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "DC2641A3-SCH.DSN" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    std::vector<std::set<std::string>> expected = { { terminalToken( "L1", "3" ), terminalToken( "L4", "3" ) } };
    auto [consistent, checkable] = checkConnectivity( *schematic, expected );
    BOOST_CHECK_EQUAL( checkable, 1 );
    BOOST_CHECK_EQUAL( consistent, 1 );
}


BOOST_AUTO_TEST_CASE( PowerAliasesConnectAcrossPages )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "SCH-54852_A5.DSN" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    std::vector<std::set<std::string>> expected = { { terminalToken( "R711", "2" ), terminalToken( "C104", "1" ) } };
    auto [consistent, checkable] = checkConnectivity( *schematic, expected );
    BOOST_CHECK_EQUAL( checkable, 1 );
    BOOST_CHECK_EQUAL( consistent, 1 );
}


BOOST_AUTO_TEST_CASE( ReusedPowerNetIdsDoNotJoinDisconnectedNets )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "SCH-31399_C4.DSN" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    std::vector<std::set<std::string>> expected = { { terminalToken( "C105", "1" ), terminalToken( "C106", "1" ) },
                                                    { terminalToken( "C127", "1" ), terminalToken( "C128", "1" ) } };
    auto [consistent, checkable] = checkConnectivity( *schematic, expected );
    BOOST_CHECK_EQUAL( checkable, 2 );
    BOOST_CHECK_EQUAL( consistent, 2 );
}


BOOST_AUTO_TEST_CASE( ReusedPowerNetAliasesRemainPhysicallyScoped )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "DC607A.DSN" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    std::vector<std::set<std::string>> expected = { { terminalToken( "C1", "2" ), terminalToken( "C13", "2" ) },
                                                    { terminalToken( "C14", "2" ), terminalToken( "C2", "2" ) },
                                                    { terminalToken( "C3", "1" ), terminalToken( "C4", "1" ) },
                                                    { terminalToken( "C13", "1" ), terminalToken( "C31", "1" ) } };
    auto [consistent, checkable] = checkConnectivity( *schematic, expected );
    BOOST_CHECK_EQUAL( checkable, 4 );
    BOOST_CHECK_EQUAL( consistent, 4 );
}


BOOST_AUTO_TEST_CASE( SinglePowerMeaningPropagatesAcrossReusedNetId )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn =
            findCorpusDesign( corpusEnv, "630-60651-01_04_CYW9BTM2BASE3_20829_BaseBoard_Schematics.DSN" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    std::vector<std::set<std::string>> expected = { { terminalToken( "C1", "2" ), terminalToken( "U5", "62" ),
                                                      terminalToken( "U5", "63" ), terminalToken( "U5", "65" ) } };
    auto [consistent, checkable] = checkConnectivity( *schematic, expected );
    BOOST_CHECK_EQUAL( checkable, 1 );
    BOOST_CHECK_EQUAL( consistent, 1 );
}


BOOST_AUTO_TEST_CASE( RepeatedLocalNetNamesRemainSheetScoped )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "parallella_gen0.dsn" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    std::vector<std::set<std::string>> expected = { { terminalToken( "R41", "2" ), terminalToken( "U26", "53" ) },
                                                    { terminalToken( "R57", "2" ), terminalToken( "R97", "1" ),
                                                      terminalToken( "U14", "1" ) } };
    auto [consistent, checkable] = checkConnectivity( *schematic, expected );
    BOOST_CHECK_EQUAL( checkable, 2 );
    BOOST_CHECK_EQUAL( consistent, 2 );
}


BOOST_AUTO_TEST_CASE( RepeatedHierarchicalPortNamesRemainSheetScoped )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "SI347X-DC-EB.DSN" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    std::vector<std::set<std::string>> expected;

    for( int channel = 1; channel <= 8; ++channel )
    {
        std::string q = "Q" + std::to_string( channel );
        std::string r = "R" + std::to_string( channel );
        std::string gatePin = std::to_string( std::array{ 1, 7, 8, 14, 29, 35, 36, 42 }[channel - 1] );
        std::string sourcePin = std::to_string( std::array{ 2, 6, 9, 13, 30, 34, 37, 41 }[channel - 1] );
        expected.push_back( { terminalToken( q, "G" ), terminalToken( "U1", gatePin ) } );
        expected.push_back( { terminalToken( q, "2" ), terminalToken( q, "3" ), terminalToken( q, "S" ),
                              terminalToken( r, "2" ), terminalToken( "U1", sourcePin ) } );
    }

    auto [consistent, checkable] = checkConnectivity( *schematic, expected );
    BOOST_CHECK_EQUAL( checkable, 16 );
    BOOST_CHECK_EQUAL( consistent, 16 );
}


BOOST_AUTO_TEST_CASE( RepeatedHierarchicalOffpageNamesRemainSheetScoped )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "Si828X-BW-GDB.DSN" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    std::vector<std::set<std::string>> expected = {
        { terminalToken( "D317", "K" ), terminalToken( "JT1", "2" ), terminalToken( "Q13", "E" ) },
        { terminalToken( "D322", "K" ), terminalToken( "JT6", "2" ), terminalToken( "Q12", "E" ) },
        { terminalToken( "Q6", "C" ), terminalToken( "Q7", "C" ), terminalToken( "Q15", "C" ),
          terminalToken( "U204", "9" ) },
        { terminalToken( "Q10", "C" ), terminalToken( "Q11", "C" ), terminalToken( "Q20", "C" ),
          terminalToken( "U2", "13" ) }
    };
    auto [consistent, checkable] = checkConnectivity( *schematic, expected );
    BOOST_CHECK_EQUAL( checkable, 4 );
    BOOST_CHECK_EQUAL( consistent, 4 );
}


BOOST_AUTO_TEST_CASE( NestedOccurrenceReferencesOverridePlacedTemplateReferences )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "SI8284v2-EVB.DSN" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    std::set<std::string> refs = collectImportedRefs( *schematic );

    BOOST_CHECK( refs.count( "Q1-1" ) );
    BOOST_CHECK( refs.count( "Q1-2" ) );
    BOOST_CHECK( refs.count( "Q8-1" ) );
    BOOST_CHECK( refs.count( "Q8-2" ) );
}


BOOST_AUTO_TEST_CASE( SharedOccurrenceNetWithoutTerminalPeerKeepsBaseName )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "SI8284v2-EVB.DSN" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    BOOST_CHECK_EQUAL( terminalNetName( *schematic, wxS( "J15" ), wxS( "2" ) ).AfterLast( '/' ).Lower(),
                       wxString( wxS( "s3" ) ) );
}


BOOST_AUTO_TEST_CASE( PowerNetNameOverridesLocalWireAlias )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "SI8284v2-EVB.DSN" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    SCH_SHEET_LIST sheets = schematic->BuildSheetListSortedByPageNumbers();
    schematic->ConnectionGraph()->Recalculate( sheets, true );
    wxString c36Pin1Net;

    for( const auto& [key, subgraphs] : schematic->ConnectionGraph()->GetNetMap() )
    {
        for( CONNECTION_SUBGRAPH* subgraph : subgraphs )
        {
            for( SCH_ITEM* item : subgraph->GetItems() )
            {
                if( item->Type() != SCH_PIN_T )
                    continue;

                SCH_PIN*    pin = static_cast<SCH_PIN*>( item );
                SCH_SYMBOL* symbol = dynamic_cast<SCH_SYMBOL*>( pin->GetParentSymbol() );

                if( symbol && symbol->GetRef( &subgraph->GetSheet(), false ) == wxS( "C36" )
                    && pin->GetNumber() == wxS( "1" ) )
                {
                    c36Pin1Net = key.Name;
                }
            }
        }
    }

    BOOST_CHECK_EQUAL( c36Pin1Net.AfterLast( '/' ), wxS( "VDDB_16939716" ) );
}


BOOST_AUTO_TEST_CASE( HiddenWireLabelsAvoidBusCrossings )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "CN81XX_GBCV2_sch_0530.DSN" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    SCH_SHEET_LIST sheets = schematic->BuildSheetListSortedByPageNumbers();
    schematic->ConnectionGraph()->Recalculate( sheets, true );
    wxString r13Pin1Net;

    for( const auto& [key, subgraphs] : schematic->ConnectionGraph()->GetNetMap() )
    {
        for( CONNECTION_SUBGRAPH* subgraph : subgraphs )
        {
            for( SCH_ITEM* item : subgraph->GetItems() )
            {
                if( item->Type() != SCH_PIN_T )
                    continue;

                SCH_PIN*    pin = static_cast<SCH_PIN*>( item );
                SCH_SYMBOL* symbol = dynamic_cast<SCH_SYMBOL*>( pin->GetParentSymbol() );

                if( symbol && symbol->GetRef( &subgraph->GetSheet(), false ) == wxS( "R13" )
                    && pin->GetNumber() == wxS( "1" ) )
                {
                    r13Pin1Net = key.Name;
                }
            }
        }
    }

    BOOST_CHECK_EQUAL( r13Pin1Net.AfterLast( '/' ), wxS( "DDR0_DM1" ) );
}


BOOST_AUTO_TEST_CASE( IncompleteCachedSymbolsUsePlacedPins )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "1979A.DSN" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    std::vector<std::set<std::string>> expected = { { terminalToken( "J1", "1" ), terminalToken( "R5", "1" ) },
                                                    { terminalToken( "J2", "1" ), terminalToken( "R6", "2" ) },
                                                    { terminalToken( "J1", "2" ), terminalToken( "J1", "3" ),
                                                      terminalToken( "J2", "2" ) } };
    auto [consistent, checkable] = checkConnectivity( *schematic, expected );
    BOOST_CHECK_EQUAL( checkable, 3 );
    BOOST_CHECK_EQUAL( consistent, 3 );
}


BOOST_AUTO_TEST_CASE( LegacyOffpageConnectorsKeepDistinctPinPositions )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "DC726A-1.DSN" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    std::vector<std::set<std::string>> expected = { { terminalToken( "J1", "4" ), terminalToken( "R40", "1" ),
                                                      terminalToken( "R9", "1" ), terminalToken( "U5", "14" ) },
                                                    { terminalToken( "J1", "6" ), terminalToken( "R10", "1" ),
                                                      terminalToken( "U5", "13" ) },
                                                    { terminalToken( "J1", "7" ), terminalToken( "R39", "1" ),
                                                      terminalToken( "R5", "1" ), terminalToken( "U5", "16" ) } };
    auto [consistent, checkable] = checkConnectivity( *schematic, expected );
    BOOST_CHECK_EQUAL( checkable, 3 );
    BOOST_CHECK_EQUAL( consistent, 3 );
}


BOOST_AUTO_TEST_CASE( EmptyPackagePinNumbersUseLogicalPinNames )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "DC2283A-2.DSN" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    std::vector<std::set<std::string>> expected = { { terminalToken( "C41", "1" ), terminalToken( "C42", "1" ) },
                                                    { terminalToken( "C42", "2" ), terminalToken( "J12", "1" ) } };
    auto [consistent, checkable] = checkConnectivity( *schematic, expected );
    BOOST_CHECK_EQUAL( checkable, 2 );
    BOOST_CHECK_EQUAL( consistent, 2 );
}


BOOST_AUTO_TEST_CASE( EmptyPackagePinNumbersPreserveAlphabeticNames )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "EMS4_0.DSN" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    std::vector<std::set<std::string>> expected = { { terminalToken( "DL1", "C" ), terminalToken( "R14", "1" ) },
                                                    { terminalToken( "TP1", "A" ), terminalToken( "U1", "7" ) },
                                                    { terminalToken( "U10", "PAD" ), terminalToken( "U11", "PAD" ) } };
    auto [consistent, checkable] = checkConnectivity( *schematic, expected );
    BOOST_CHECK_EQUAL( checkable, 3 );
    BOOST_CHECK_EQUAL( consistent, 3 );
}


BOOST_AUTO_TEST_CASE( EmbeddedTwoPinPackageNamesDoNotReplacePhysicalNumbers )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "DC2793A-3.DSN" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    std::vector<std::set<std::string>> expected = { { terminalToken( "D1", "1" ), terminalToken( "R6", "2" ) },
                                                    { terminalToken( "D1", "2" ), terminalToken( "R4", "2" ) } };
    auto [consistent, checkable] = checkConnectivity( *schematic, expected );
    BOOST_CHECK_EQUAL( checkable, 2 );
    BOOST_CHECK_EQUAL( consistent, 2 );
}


BOOST_AUTO_TEST_CASE( DirectPowerSymbolSharesComponentPin )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "CY4532 Power Board Schematic.DSN" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    std::vector<std::set<std::string>> expected = { { terminalToken( "C70", "2" ), terminalToken( "C72", "2" ) } };
    auto [consistent, checkable] = checkConnectivity( *schematic, expected );
    BOOST_CHECK_EQUAL( checkable, 1 );
    BOOST_CHECK_EQUAL( consistent, 1 );
}


BOOST_AUTO_TEST_CASE( DirectPowerSymbolUsesComponentPinNet )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "DC2047A-3-A.DSN" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    std::vector<std::set<std::string>> expected = { { terminalToken( "D15", "2" ), terminalToken( "D16", "2" ) } };
    auto [consistent, checkable] = checkConnectivity( *schematic, expected );
    BOOST_CHECK_EQUAL( checkable, 1 );
    BOOST_CHECK_EQUAL( consistent, 1 );
}


BOOST_AUTO_TEST_CASE( NearbyOffpageConnectorsRemainDistinct )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn =
            findCorpusDesign( corpusEnv, "630-60651-01_04_CYW9BTM2BASE3_20829_BaseBoard_Schematics.DSN" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    std::vector<std::set<std::string>> expected = { { terminalToken( "J14", "1" ), terminalToken( "J16", "17" ),
                                                      terminalToken( "R110", "2" ) },
                                                    { terminalToken( "J16", "19" ), terminalToken( "J2", "2" ) },
                                                    { terminalToken( "J16", "67" ), terminalToken( "J2", "3" ) } };
    auto [consistent, checkable] = checkConnectivity( *schematic, expected );
    BOOST_CHECK_EQUAL( checkable, 3 );
    BOOST_CHECK_EQUAL( consistent, 3 );
}


BOOST_AUTO_TEST_CASE( MatchingSymbolAndPackageVariantsPreservePhysicalPinNames )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "CY4532 Power Board Schematic.DSN" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    std::vector<std::set<std::string>> expected = { { terminalToken( "D5", "A" ), terminalToken( "U2", "24" ) },
                                                    { terminalToken( "D5", "K" ), terminalToken( "U2", "20" ) } };
    auto [consistent, checkable] = checkConnectivity( *schematic, expected );
    BOOST_CHECK_EQUAL( checkable, 2 );
    BOOST_CHECK_EQUAL( consistent, 2 );
}


BOOST_AUTO_TEST_CASE( OffpageDisplayNameDoesNotChangeConnectivity )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
    {
        BOOST_TEST_MESSAGE( "KICAD_ORCAD_CORPUS not set; skipping off-page display-name check." );
        return;
    }

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "DC2509A-1.DSN" );

    if( dsn.empty() )
    {
        BOOST_TEST_MESSAGE( "DC2509A-1.DSN not present in corpus; skipping off-page display-name check." );
        return;
    }

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    SCH_SHEET_LIST sheets = schematic->BuildSheetListSortedByPageNumbers();
    schematic->ConnectionGraph()->Recalculate( sheets, true );

    std::map<std::string, CONNECTION_SUBGRAPH*> pinNets;

    for( const auto& [key, subgraphs] : schematic->ConnectionGraph()->GetNetMap() )
    {
        for( CONNECTION_SUBGRAPH* subgraph : subgraphs )
        {
            for( SCH_ITEM* item : subgraph->GetItems() )
            {
                if( item->Type() != SCH_PIN_T )
                    continue;

                SCH_PIN*    pin = static_cast<SCH_PIN*>( item );
                SCH_SYMBOL* symbol = dynamic_cast<SCH_SYMBOL*>( pin->GetParentSymbol() );

                if( !symbol || pin->GetNumber() != wxS( "1" ) )
                    continue;

                wxString ref = symbol->GetRef( &subgraph->GetSheet(), false );

                if( ref == wxS( "U3" ) || ref == wxS( "U4" ) )
                    pinNets[ref.ToStdString()] = subgraph;
            }
        }
    }

    BOOST_REQUIRE_EQUAL( pinNets.count( "U3" ), 1u );
    BOOST_REQUIRE_EQUAL( pinNets.count( "U4" ), 1u );
    BOOST_CHECK_EQUAL( pinNets["U3"], pinNets["U4"] );
}


BOOST_AUTO_TEST_CASE( SingleLeafOccurrencePreservesNamedNet )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
    {
        BOOST_TEST_MESSAGE( "KICAD_ORCAD_CORPUS not set; skipping single leaf occurrence check." );
        return;
    }

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "SCH-28988.DSN" );

    if( dsn.empty() )
    {
        BOOST_TEST_MESSAGE( "SCH-28988.DSN not present in corpus; skipping single leaf occurrence check." );
        return;
    }

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    SCH_SHEET_LIST sheets = schematic->BuildSheetListSortedByPageNumbers();
    schematic->ConnectionGraph()->Recalculate( sheets, true );
    wxString netName;

    for( const auto& [key, subgraphs] : schematic->ConnectionGraph()->GetNetMap() )
    {
        for( CONNECTION_SUBGRAPH* subgraph : subgraphs )
        {
            for( SCH_ITEM* item : subgraph->GetItems() )
            {
                if( item->Type() != SCH_PIN_T )
                    continue;

                SCH_PIN*    pin = static_cast<SCH_PIN*>( item );
                SCH_SYMBOL* symbol = dynamic_cast<SCH_SYMBOL*>( pin->GetParentSymbol() );

                if( symbol && symbol->GetRef( &subgraph->GetSheet(), false ) == wxS( "C3" )
                    && pin->GetNumber() == wxS( "1" ) )
                {
                    netName = key.Name;
                }
            }
        }
    }

    BOOST_CHECK_EQUAL( netName, wxS( "ANALOG5V" ) );
    BOOST_CHECK( !netName.Contains( wxS( "_BRKTSTBCDP5004" ) ) );
}


BOOST_AUTO_TEST_CASE( PackageVariantsAndIgnoredPinsArePreserved )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
    {
        BOOST_TEST_MESSAGE( "KICAD_ORCAD_CORPUS not set; skipping package-variant check." );
        return;
    }

    auto load = [&]( const char* aName )
    {
        std::filesystem::path dsn = findCorpusDesign( corpusEnv, aName );
        BOOST_REQUIRE_MESSAGE( !dsn.empty(), aName << " not present in corpus." );

        std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
        SETTINGS_MANAGER*          manager = new SETTINGS_MANAGER;
        manager->LoadProject( "" );
        schematic->SetProject( &manager->Prj() );
        schematic->CurrentSheet().clear();
        schematic->CurrentSheet().push_back( &schematic->Root() );

        SCH_IO_ORCAD plugin;
        plugin.LoadSchematicFile( dsn.string(), schematic.get() );
        return std::pair( std::move( schematic ), std::unique_ptr<SETTINGS_MANAGER>( manager ) );
    };

    auto [breakout, breakoutManager] = load( "OC_CONNECT_1_BRKOUT_BRD.DSN" );
    std::map<wxString, std::set<wxString>> switchPins;

    for( const SCH_SHEET_PATH& path : breakout->BuildSheetListSortedByPageNumbers() )
    {
        for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );
            wxString    ref = symbol->GetRef( &path, false );

            if( ref != wxS( "S1" ) && ref != wxS( "S3" ) )
                continue;

            for( SCH_PIN* pin : symbol->GetPins( &path ) )
                switchPins[ref].insert( pin->GetNumber() );
        }
    }

    const std::set<wxString> expectedS1 = { wxS( "1" ), wxS( "2" ), wxS( "3" ), wxS( "4" ) };
    const std::set<wxString> expectedS3 = { wxS( "1" ), wxS( "2" ), wxS( "3" ) };
    BOOST_CHECK_EQUAL_COLLECTIONS( switchPins[wxS( "S1" )].begin(), switchPins[wxS( "S1" )].end(), expectedS1.begin(),
                                   expectedS1.end() );
    BOOST_CHECK_EQUAL_COLLECTIONS( switchPins[wxS( "S3" )].begin(), switchPins[wxS( "S3" )].end(), expectedS3.begin(),
                                   expectedS3.end() );

    auto [j401, j401Manager] = load( "reServer industrial J401 Carrier Board v11.DSN" );
    std::set<wxString> j10Pins;

    for( const SCH_SHEET_PATH& path : j401->BuildSheetListSortedByPageNumbers() )
    {
        for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

            if( symbol->GetRef( &path, false ) != wxS( "J10" ) )
                continue;

            for( SCH_PIN* pin : symbol->GetPins( &path ) )
                j10Pins.insert( pin->GetNumber() );
        }
    }

    BOOST_CHECK_EQUAL( j10Pins.size(), 53u );
    BOOST_CHECK_EQUAL( j10Pins.count( wxS( "SS1" ) ), 0u );
    BOOST_CHECK_EQUAL( j10Pins.count( wxS( "SS2" ) ), 0u );
}


BOOST_AUTO_TEST_CASE( LegacyDesignCachePackagePinMapsArePreserved )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
    {
        BOOST_TEST_MESSAGE( "KICAD_ORCAD_CORPUS not set; skipping legacy package-map check." );
        return;
    }

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "EXAMPLE.DSN" );

    if( dsn.empty() )
    {
        BOOST_TEST_MESSAGE( "EXAMPLE.DSN not present in corpus; skipping legacy package-map check." );
        return;
    }

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    std::set<wxString> pins;

    for( const SCH_SHEET_PATH& path : schematic->BuildSheetListSortedByPageNumbers() )
    {
        for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

            if( symbol->GetRef( &path, false ) != wxS( "U4" ) )
                continue;

            for( SCH_PIN* pin : symbol->GetPins( &path ) )
                pins.insert( pin->GetNumber() );
        }
    }

    const std::set<wxString> expected = { wxS( "2" ), wxS( "3" ), wxS( "4" ), wxS( "5" ), wxS( "12" ) };
    BOOST_CHECK_EQUAL_COLLECTIONS( pins.begin(), pins.end(), expected.begin(), expected.end() );
}


BOOST_AUTO_TEST_CASE( S593487_PartialConnectorPinOrderIsPreserved )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
    {
        BOOST_TEST_MESSAGE( "KICAD_ORCAD_CORPUS not set; skipping S-593487 connector check." );
        return;
    }

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "S-593487-REV-B.DSN" );
    BOOST_REQUIRE_MESSAGE( !dsn.empty(), "S-593487-REV-B.DSN not present in corpus." );

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    auto [consistent, checkable] =
            checkConnectivity( *schematic, { { terminalToken( "JCA2", "1" ), terminalToken( "RT1", "1" ) },
                                             { terminalToken( "JCA2", "2" ), terminalToken( "RT2", "1" ) },
                                             { terminalToken( "JCA2", "3" ), terminalToken( "RT3", "1" ) },
                                             { terminalToken( "JCA2", "4" ), terminalToken( "RT4", "1" ) } } );
    BOOST_CHECK_EQUAL( checkable, 4 );
    BOOST_CHECK_EQUAL( consistent, 4 );
}


BOOST_AUTO_TEST_CASE( M5275_ExplicitPowerPinsRemainVisible )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
    {
        BOOST_TEST_MESSAGE( "KICAD_ORCAD_CORPUS not set; skipping M5275 visible-power-pin check." );
        return;
    }

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "M5275EVB.DSN" );
    BOOST_REQUIRE_MESSAGE( !dsn.empty(), "M5275EVB.DSN not present in corpus." );

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    std::map<wxString, bool> visible;

    for( const SCH_SHEET_PATH& path : schematic->BuildSheetListSortedByPageNumbers() )
    {
        for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

            if( symbol->GetRef( &path, false ) != wxS( "U21" ) )
                continue;

            for( SCH_PIN* pin : symbol->GetPins( &path ) )
                visible[pin->GetNumber()] = pin->IsVisible();
        }
    }

    BOOST_REQUIRE_EQUAL( visible.size(), 6u );
    BOOST_CHECK( visible[wxS( "2" )] );
    BOOST_CHECK( visible[wxS( "5" )] );
}


BOOST_AUTO_TEST_CASE( SI34062_StackedSwitchPinsShareNet )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
    {
        BOOST_TEST_MESSAGE( "KICAD_ORCAD_CORPUS not set; skipping SI34062 stacked-pin check." );
        return;
    }

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "SI34062-ISO-FB-EVB.DSN" );
    BOOST_REQUIRE_MESSAGE( !dsn.empty(), "SI34062-ISO-FB-EVB.DSN not present in corpus." );

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    auto [consistent, checkable] = checkConnectivity(
            *schematic, { { terminalToken( "S2", "1" ), terminalToken( "S2", "2" ), terminalToken( "U5", "K" ) } } );
    BOOST_CHECK_EQUAL( checkable, 1 );
    BOOST_CHECK_EQUAL( consistent, 1 );
}


BOOST_AUTO_TEST_CASE( DuplicatePowerAliasTextDoesNotMergeDistinctNets )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
    {
        BOOST_TEST_MESSAGE( "KICAD_ORCAD_CORPUS not set; skipping duplicate power-alias check." );
        return;
    }

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "SI8284v2-EVB.DSN" );
    BOOST_REQUIRE_MESSAGE( !dsn.empty(), "SI8284v2-EVB.DSN not present in corpus." );

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    auto [consistent, checkable] =
            checkConnectivity( *schematic, { { terminalToken( "C12", "1" ), terminalToken( "U2", "20" ) },
                                             { terminalToken( "C36", "1" ), terminalToken( "Q4", "C" ) } } );
    BOOST_CHECK_EQUAL( checkable, 2 );
    BOOST_CHECK_EQUAL( consistent, 2 );
}


BOOST_AUTO_TEST_CASE( LogicalPowerPinNameDoesNotOverrideConnectedWire )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
    {
        BOOST_TEST_MESSAGE( "KICAD_ORCAD_CORPUS not set; skipping logical power-pin check." );
        return;
    }

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "CY8CKIT-041-41XX Schematic.DSN" );
    BOOST_REQUIRE_MESSAGE( !dsn.empty(), "CY8CKIT-041-41XX Schematic.DSN not present in corpus." );

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    bool checkedHeaderPin = false;

    for( const SCH_SHEET_PATH& path : schematic->Hierarchy() )
    {
        for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

            if( symbol->GetRef( &path, false ) != wxS( "J8" )
                || !path.Last()->GetName().Contains( wxS( "PSoC 5LP Programmer" ) ) )
            {
                continue;
            }

            for( SCH_PIN* pin : symbol->GetPins() )
            {
                if( pin->GetNumber() == wxS( "3" ) )
                {
                    BOOST_CHECK( pin->GetPosition() == OrcadDbuToIu( 1192, 1006 ) );
                    checkedHeaderPin = true;
                }
            }
        }
    }

    BOOST_REQUIRE( checkedHeaderPin );

    auto [consistent, checkable] =
            checkConnectivity( *schematic, { { terminalToken( "L4", "2" ), terminalToken( "U1", "40" ) },
                                             { terminalToken( "C59", "1" ), terminalToken( "U15", "44" ) },
                                             { terminalToken( "J8", "3" ), terminalToken( "U15", "28" ),
                                               terminalToken( "R40", "1" ) } } );
    BOOST_CHECK_EQUAL( checkable, 3 );
    BOOST_CHECK_EQUAL( consistent, 3 );
}


BOOST_AUTO_TEST_CASE( SourceLibrarySelectsPackageVariantBeforeSharedFootprint )
{
    ORCAD_SYMBOL_DEF symbol;
    symbol.typeId = ORCAD_ST_LIBRARY_PART;
    symbol.name = "PART.Normal";
    symbol.sourceLib = "right.dsn";
    symbol.bbox = ORCAD_BBOX{ 0, 0, 30, 20 };
    symbol.pins = { ORCAD_SYMBOL_PIN{ .name = "A", .position = 0, .hotptX = 0, .hotptY = 10 },
                    ORCAD_SYMBOL_PIN{ .name = "K", .position = 1, .hotptX = 30, .hotptY = 10 } };

    ORCAD_PACKAGE package;
    package.name = "PART";
    package.sourceLib = "wrong.dsn";
    package.pcbFootprint = "SOT23";
    package.devices.push_back( ORCAD_DEVICE{ .pinNumbers = { "1", "2" }, .pinIgnore = { false, false } } );
    ORCAD_PACKAGE variant = package;
    variant.sourceLib = symbol.sourceLib;
    variant.devices.front().pinNumbers = { "1", "3" };
    package.variants.push_back( std::move( variant ) );

    ORCAD_PLACED_INSTANCE placed;
    placed.pkgName = symbol.name;
    placed.sourcePackage = package.name;
    placed.sourceLibrary = symbol.sourceLib;
    placed.reference = "D1";
    placed.props["PCB Footprint"] = package.pcbFootprint;
    placed.x = 100;
    placed.y = 100;
    placed.pins = { ORCAD_PIN_INST{ 1, 100, 110 }, ORCAD_PIN_INST{ 2, 130, 110 } };

    ORCAD_RAW_PAGE page;
    page.name = "PACKAGE SOURCE";
    page.instances.push_back( std::move( placed ) );

    ORCAD_DESIGN design;
    design.sourceId = "package-source-before-footprint";
    design.symbols.emplace( symbol.name, std::move( symbol ) );
    design.packages.emplace( package.name, std::move( package ) );
    design.pages.push_back( std::move( page ) );

    SETTINGS_MANAGER manager;
    manager.LoadProject( "" );
    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    schematic->SetProject( &manager.Prj() );
    SCH_SHEET*     root = convertRawDesign( design, *schematic );
    SCH_SHEET_PATH path;
    path.push_back( root );
    SCH_SYMBOL* converted = findConvertedSymbol( *root->GetScreen(), path, wxS( "D1" ) );
    BOOST_REQUIRE( converted );
    std::set<wxString> pinNumbers;

    for( SCH_PIN* pin : converted->GetPins() )
        pinNumbers.insert( pin->GetNumber() );

    const std::set<wxString> expected = { wxS( "1" ), wxS( "3" ) };
    BOOST_CHECK_EQUAL_COLLECTIONS( pinNumbers.begin(), pinNumbers.end(), expected.begin(), expected.end() );
}


BOOST_AUTO_TEST_CASE( PinNamesAndNumbersRemainNativePinData )
{
    ORCAD_SYMBOL_DEF symbol;
    symbol.typeId = ORCAD_ST_LIBRARY_PART;
    symbol.name = "VERTICAL.Normal";
    symbol.bbox = ORCAD_BBOX{ 0, 10, 20, 30 };
    symbol.generalFlags = 1;
    symbol.pins = { ORCAD_SYMBOL_PIN{ .name = "2", .position = 0, .startX = 10, .startY = 10,
                                      .hotptX = 10, .hotptY = 0 },
                    ORCAD_SYMBOL_PIN{ .name = "HIN", .position = 1, .startX = 0, .startY = 20,
                                      .hotptX = -10, .hotptY = 20 } };
    ORCAD_SYMBOL_DEF rotated = symbol;
    rotated.name = "ROTATED.Normal";
    rotated.generalFlags = 3;

    ORCAD_PACKAGE package;
    package.name = "VERTICAL";
    package.devices.push_back( ORCAD_DEVICE{ .pinNumbers = { "2", "3" }, .pinIgnore = { false, false } } );
    ORCAD_PACKAGE rotatedPackage = package;
    rotatedPackage.name = "ROTATED";

    ORCAD_PLACED_INSTANCE placed;
    placed.pkgName = symbol.name;
    placed.sourcePackage = package.name;
    placed.reference = "U1";
    placed.x = 100;
    placed.y = 100;
    placed.pins = { ORCAD_PIN_INST{ 1, 110, 100 }, ORCAD_PIN_INST{ 2, 90, 120 } };
    ORCAD_PLACED_INSTANCE rotatedPlaced = placed;
    rotatedPlaced.pkgName = rotated.name;
    rotatedPlaced.sourcePackage = rotatedPackage.name;
    rotatedPlaced.reference = "U2";
    rotatedPlaced.x = 200;
    rotatedPlaced.pins = { ORCAD_PIN_INST{ 1, 210, 100 } };

    ORCAD_RAW_PAGE page;
    page.name = "VERTICAL PIN TEXT";
    page.instances.push_back( std::move( placed ) );
    page.instances.push_back( std::move( rotatedPlaced ) );

    ORCAD_DESIGN design;
    design.sourceId = "vertical-pin-text";
    design.library.fonts = { ORCAD_FONT{ .height = -12, .width = 5, .pitchAndFamily = 0x22,
                                         .face = "Arial Narrow" },
                             ORCAD_FONT{ .height = -8, .face = "Arial" } };
    design.library.templateFonts.resize( 12 );
    design.library.templateFonts[10] = 1;
    design.library.templateFonts[11] = 2;
    design.library.pinNameFont = 10;
    design.library.pinNumberFont = 11;
    design.symbols.emplace( symbol.name, std::move( symbol ) );
    design.symbols.emplace( rotated.name, std::move( rotated ) );
    design.packages.emplace( package.name, std::move( package ) );
    design.packages.emplace( rotatedPackage.name, std::move( rotatedPackage ) );
    design.pages.push_back( std::move( page ) );

    SETTINGS_MANAGER manager;
    manager.LoadProject( "" );
    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    schematic->SetProject( &manager.Prj() );
    SCH_SHEET*     root = convertRawDesign( design, *schematic );
    SCH_SHEET_PATH path;
    path.push_back( root );
    SCH_SYMBOL* converted = findConvertedSymbol( *root->GetScreen(), path, wxS( "U1" ) );
    BOOST_REQUIRE( converted );
    BOOST_REQUIRE( converted->GetLibSymbolRef() );

    const SCH_PIN* verticalPin = converted->GetLibSymbolRef()->GetPin( wxS( "2" ) );
    BOOST_REQUIRE( verticalPin );
    BOOST_CHECK_EQUAL( verticalPin->GetName(), wxS( "2" ) );
    BOOST_CHECK_EQUAL( verticalPin->GetNameTextSize(), schIUScale.mmToIU( 2.17 ) );
    BOOST_CHECK_EQUAL( verticalPin->GetNumberTextSize(), schIUScale.mmToIU( 1.51 ) );

    for( const SCH_ITEM& item : converted->GetLibSymbolRef()->GetDrawItems() )
    {
        if( item.Type() == SCH_TEXT_T )
        {
            const SCH_TEXT& text = static_cast<const SCH_TEXT&>( item );
            BOOST_CHECK_NE( text.GetText(), wxS( "2" ) );
        }
    }

    SCH_SYMBOL* rotatedConverted = findConvertedSymbol( *root->GetScreen(), path, wxS( "U2" ) );
    BOOST_REQUIRE( rotatedConverted );
    BOOST_REQUIRE( rotatedConverted->GetLibSymbolRef() );
    const SCH_PIN* rotatedPin = rotatedConverted->GetLibSymbolRef()->GetPin( wxS( "2" ) );
    BOOST_REQUIRE( rotatedPin );
    BOOST_CHECK_EQUAL( rotatedPin->GetName(), wxS( "2" ) );
    BOOST_CHECK_EQUAL( rotatedPin->GetNameTextSize(), schIUScale.mmToIU( 2.17 ) );
    BOOST_CHECK_EQUAL( rotatedPin->GetNumberTextSize(), schIUScale.mmToIU( 1.51 ) );

    for( const SCH_ITEM& item : rotatedConverted->GetLibSymbolRef()->GetDrawItems() )
    {
        if( item.Type() == SCH_TEXT_T )
        {
            const SCH_TEXT& text = static_cast<const SCH_TEXT&>( item );
            BOOST_CHECK_NE( text.GetText(), wxS( "2" ) );
        }
    }
}


BOOST_AUTO_TEST_CASE( PinNameOverbarsUseKiCadMarkup )
{
    BOOST_CHECK_EQUAL( OrcadPinNameMarkup( wxS( "\\\\C\\S\\ ADD1" ) ), wxS( "~{CS} ADD1" ) );
    BOOST_CHECK_EQUAL( OrcadPinNameMarkup( wxS( "\\\\I\\N\\T\\" ) ), wxS( "~{INT}" ) );
    BOOST_CHECK_EQUAL( OrcadPinNameMarkup( wxS( "READY" ) ), wxS( "READY" ) );
}


BOOST_AUTO_TEST_CASE( PinDisplayOverrideRetainsNativePinName )
{
    ORCAD_DISPLAY_PROP display{ .name = "Name", .x = 20, .y = 0, .rotation = 0,
                                .fontIdx = 1, .color = 48, .dispMode = 0x01E9 };
    ORCAD_SYMBOL_DEF symbol;
    symbol.typeId = ORCAD_ST_LIBRARY_PART;
    symbol.name = "PIN_OVERRIDE.Normal";
    symbol.bbox = ORCAD_BBOX{ 0, 0, 30, 50 };
    symbol.generalFlags = 7;
    symbol.pins = { ORCAD_SYMBOL_PIN{ .name = "F1",
                                      .position = 0,
                                      .startX = 20,
                                      .startY = 0,
                                      .hotptX = 20,
                                      .hotptY = -10,
                                      .displayProps = { display } } };

    ORCAD_PACKAGE package;
    package.name = "PIN_OVERRIDE";
    package.devices.push_back( ORCAD_DEVICE{ .pinNumbers = { "1" }, .pinIgnore = { false } } );

    ORCAD_PLACED_INSTANCE placed;
    placed.pkgName = symbol.name;
    placed.sourcePackage = package.name;
    placed.reference = "J1";
    placed.x = 100;
    placed.y = 100;
    placed.pins = { ORCAD_PIN_INST{ 1, 120, 90 } };

    ORCAD_RAW_PAGE page;
    page.name = "PIN DISPLAY OVERRIDE";
    page.instances.push_back( std::move( placed ) );

    ORCAD_DESIGN design;
    design.sourceId = "pin-display-override";
    design.library.fonts = { ORCAD_FONT{ .height = -12, .width = 5, .pitchAndFamily = 0x22,
                                         .face = "Arial Narrow" } };
    design.library.pinNameFont = 1;
    design.symbols.emplace( symbol.name, std::move( symbol ) );
    design.packages.emplace( package.name, std::move( package ) );
    design.pages.push_back( std::move( page ) );

    SETTINGS_MANAGER manager;
    manager.LoadProject( "" );
    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    schematic->SetProject( &manager.Prj() );
    SCH_SHEET*     root = convertRawDesign( design, *schematic );
    SCH_SHEET_PATH path;
    path.push_back( root );
    SCH_SYMBOL* converted = findConvertedSymbol( *root->GetScreen(), path, wxS( "J1" ) );
    BOOST_REQUIRE( converted );
    BOOST_REQUIRE( converted->GetLibSymbolRef() );
    const SCH_PIN* pin = converted->GetLibSymbolRef()->GetPin( wxS( "1" ) );
    BOOST_REQUIRE( pin );
    BOOST_CHECK_EQUAL( pin->GetName(), wxS( "F1" ) );
    BOOST_CHECK_EQUAL( pin->GetNameTextSize(), schIUScale.mmToIU( 2.17 ) );
    BOOST_CHECK_EQUAL( pin->GetNumberTextSize(), 0 );

    for( const SCH_ITEM& item : converted->GetLibSymbolRef()->GetDrawItems() )
    {
        if( item.Type() == SCH_TEXT_T && static_cast<const SCH_TEXT&>( item ).GetText() == wxS( "F1" ) )
            BOOST_ERROR( "Pin name duplicated as SCH_TEXT" );
    }
}


BOOST_AUTO_TEST_CASE( RotatedSymbolRetainsNativePinNumber )
{
    ORCAD_SYMBOL_DEF symbol;
    symbol.typeId = ORCAD_ST_LIBRARY_PART;
    symbol.name = "ROTATED_PIN_NUMBER.Normal";
    symbol.bbox = ORCAD_BBOX{ 0, 0, 30, 20 };
    symbol.generalFlags = 3;
    symbol.pins = { ORCAD_SYMBOL_PIN{ .position = 0, .startX = 10, .startY = 10,
                                      .hotptX = 0, .hotptY = 10 } };

    ORCAD_PACKAGE package;
    package.name = "ROTATED_PIN_NUMBER";
    package.devices.push_back( ORCAD_DEVICE{ .pinNumbers = { "1" }, .pinIgnore = { false } } );

    ORCAD_PLACED_INSTANCE placed;
    placed.pkgName = symbol.name;
    placed.sourcePackage = package.name;
    placed.reference = "Q1";
    placed.x = 100;
    placed.y = 100;
    placed.rotation = 1;

    ORCAD_RAW_PAGE page;
    page.name = "ROTATED PIN NUMBER";
    page.instances.push_back( std::move( placed ) );

    ORCAD_DESIGN design;
    design.sourceId = "rotated-pin-number";
    design.symbols.emplace( symbol.name, std::move( symbol ) );
    design.packages.emplace( package.name, std::move( package ) );
    design.pages.push_back( std::move( page ) );

    SETTINGS_MANAGER manager;
    manager.LoadProject( "" );
    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    schematic->SetProject( &manager.Prj() );
    SCH_SHEET*     root = convertRawDesign( design, *schematic );
    SCH_SHEET_PATH path;
    path.push_back( root );
    SCH_SYMBOL* converted = findConvertedSymbol( *root->GetScreen(), path, wxS( "Q1" ) );
    BOOST_REQUIRE( converted );
    const SCH_PIN* pin = converted->GetLibSymbolRef()->GetPin( wxS( "1" ) );
    BOOST_REQUIRE( pin );
    BOOST_CHECK_GT( pin->GetNumberTextSize(), 0 );

    for( const SCH_ITEM& item : converted->GetLibSymbolRef()->GetDrawItems() )
    {
        if( item.Type() == SCH_TEXT_T && static_cast<const SCH_TEXT&>( item ).GetText() == wxS( "1" ) )
            BOOST_ERROR( "Pin number duplicated as SCH_TEXT" );
    }
}


BOOST_AUTO_TEST_CASE( NumericCachePinsOverrideStructurallyIncompatiblePackage )
{
    ORCAD_SYMBOL_DEF symbol;
    symbol.typeId = ORCAD_ST_LIBRARY_PART;
    symbol.name = "ORDERED.Normal";
    symbol.sourceLib = "desired.dsn";
    symbol.bbox = ORCAD_BBOX{ 0, 0, 30, 10 };
    symbol.pins = { ORCAD_SYMBOL_PIN{ .name = "3", .hotptX = 0 }, ORCAD_SYMBOL_PIN{ .name = "4", .hotptX = 10 },
                    ORCAD_SYMBOL_PIN{ .name = "2", .hotptX = 20 }, ORCAD_SYMBOL_PIN{ .name = "1", .hotptX = 30 } };

    for( ORCAD_SYMBOL_PIN& pin : symbol.pins )
        pin.startX = pin.hotptX;

    ORCAD_PACKAGE package;
    package.name = "ORDERED";
    package.sourceLib = "stale.dsn";
    package.devices.push_back( ORCAD_DEVICE{ .pinNumbers = { "1", "2", "3", "4", "5", "6", "7", "8", "9", "10" },
                                             .pinIgnore = std::vector<bool>( 10, false ) } );

    ORCAD_PLACED_INSTANCE placed;
    placed.pkgName = symbol.name;
    placed.sourcePackage = package.name;
    placed.sourceLibrary = symbol.sourceLib;
    placed.reference = "J1";
    placed.x = 100;
    placed.y = 100;

    for( int pin = 1; pin <= 4; ++pin )
        placed.pins.push_back( ORCAD_PIN_INST{ static_cast<int16_t>( pin ), 90 + 10 * pin, 100 } );

    ORCAD_RAW_PAGE page;
    page.name = "ORDERED";
    page.instances.push_back( std::move( placed ) );

    ORCAD_DESIGN design;
    design.sourceId = "numeric-cache-pins";
    design.symbols.emplace( symbol.name, std::move( symbol ) );
    design.packages.emplace( package.name, std::move( package ) );
    design.pages.push_back( std::move( page ) );

    SETTINGS_MANAGER manager;
    manager.LoadProject( "" );
    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    schematic->SetProject( &manager.Prj() );
    SCH_SHEET*     root = convertRawDesign( design, *schematic );
    SCH_SHEET_PATH path;
    path.push_back( root );
    SCH_SYMBOL* converted = findConvertedSymbol( *root->GetScreen(), path, wxS( "J1" ) );
    BOOST_REQUIRE( converted );
    std::map<wxString, VECTOR2I> positions;

    for( SCH_PIN* pin : converted->GetPins() )
        positions[pin->GetNumber()] = pin->GetPosition();

    BOOST_REQUIRE_EQUAL( positions.size(), 4u );
    BOOST_CHECK_EQUAL( positions[wxS( "4" )].x - positions[wxS( "3" )].x, 10 * ORCAD_IU_PER_DBU );
    BOOST_CHECK_EQUAL( positions[wxS( "2" )].x - positions[wxS( "3" )].x, 20 * ORCAD_IU_PER_DBU );
    BOOST_CHECK_EQUAL( positions[wxS( "1" )].x - positions[wxS( "3" )].x, 30 * ORCAD_IU_PER_DBU );
}


BOOST_AUTO_TEST_CASE( RenamedPowerNetsRemainElectricallyDistinct )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "CY4605_Schematic.dsn" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    std::vector<std::set<std::string>> expected = { { terminalToken( "C16", "1" ), terminalToken( "TP4", "1" ) },
                                                    { terminalToken( "C14", "1" ), terminalToken( "TP2", "1" ) } };
    auto [consistent, checkable] = checkConnectivity( *schematic, expected );
    BOOST_CHECK_EQUAL( checkable, 2 );
    BOOST_CHECK_EQUAL( consistent, 2 );
}


BOOST_AUTO_TEST_CASE( MinorityPowerOccurrenceAliasDoesNotMergeGlobalNets )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "CYW920706WCDEVAL Evaluation Kit Schematics.DSN" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    std::vector<std::set<std::string>> expected = { { terminalToken( "C1", "2" ), terminalToken( "C10", "2" ) },
                                                    { terminalToken( "C26", "2" ), terminalToken( "C27", "2" ) } };
    auto [consistent, checkable] = checkConnectivity( *schematic, expected );
    BOOST_CHECK_EQUAL( checkable, 2 );
    BOOST_CHECK_EQUAL( consistent, 2 );
}


BOOST_AUTO_TEST_CASE( OccurrenceSuffixedPowerNetsRemainDistinct )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "DC2263A-2.DSN" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    std::vector<std::set<std::string>> expected = { { terminalToken( "C13", "1" ), terminalToken( "C18", "1" ) },
                                                    { terminalToken( "C40", "1" ), terminalToken( "R61", "1" ) } };
    auto [consistent, checkable] = checkConnectivity( *schematic, expected );
    BOOST_CHECK_EQUAL( checkable, 2 );
    BOOST_CHECK_EQUAL( consistent, 2 );
}


BOOST_AUTO_TEST_CASE( GeneratedOccurrenceNetNameRemainsDistinct )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "OC_CONNECT-1_BB_BOARD_20072023_01.DSN" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    std::vector<std::set<std::string>> expected = { { terminalToken( "C1", "2" ), terminalToken( "C10", "2" ) },
                                                    { terminalToken( "LED14", "1" ), terminalToken( "R304", "1" ) } };
    auto [consistent, checkable] = checkConnectivity( *schematic, expected );
    BOOST_CHECK_EQUAL( checkable, 2 );
    BOOST_CHECK_EQUAL( consistent, 2 );
}


BOOST_AUTO_TEST_CASE( PackageVariantPreservesPhysicalPinMap )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "710-DC2693A_REV02_PCA_SCHEMATIC.DSN" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    std::vector<std::set<std::string>> expected = { { terminalToken( "C101", "1" ), terminalToken( "U101", "6" ) },
                                                    { terminalToken( "R105", "2" ), terminalToken( "U101", "3" ) },
                                                    { terminalToken( "R106", "1" ), terminalToken( "U101", "4" ) } };
    auto [consistent, checkable] = checkConnectivity( *schematic, expected );
    BOOST_CHECK_EQUAL( checkable, 3 );
    BOOST_CHECK_EQUAL( consistent, 3 );
}


BOOST_AUTO_TEST_CASE( DistinctPowerAndOffpageInterfaceNamesRemainDistinct )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "OpenCellular_Connect-1_GBC_Life-3_Schematic_v1.2.DSN" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    std::vector<std::set<std::string>> expected = { { terminalToken( "C1559", "2" ), terminalToken( "D8", "2" ) },
                                                    { terminalToken( "L28", "3" ), terminalToken( "R1009", "1" ) } };
    auto [consistent, checkable] = checkConnectivity( *schematic, expected );
    BOOST_CHECK_EQUAL( checkable, 2 );
    BOOST_CHECK_EQUAL( consistent, 2 );
}


BOOST_AUTO_TEST_CASE( FlatTopLevelOffpageConnectsAcrossPages )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "DC1931B.DSN" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    std::vector<std::set<std::string>> expected = { { terminalToken( "J4", "H26" ), terminalToken( "U2", "N1" ) } };
    auto [consistent, checkable] = checkConnectivity( *schematic, expected );
    BOOST_CHECK_EQUAL( checkable, 1 );
    BOOST_CHECK_EQUAL( consistent, 1 );
}


BOOST_AUTO_TEST_CASE( PowerAliasConnectsAcrossPages )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "710-DC2222A_REV07_PCA_SCHEMATIC.DSN" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    std::vector<std::set<std::string>> expected = { { terminalToken( "C10", "2" ), terminalToken( "C29", "1" ),
                                                      terminalToken( "C30", "1" ), terminalToken( "C9", "2" ),
                                                      terminalToken( "E2", "1" ), terminalToken( "R19", "1" ),
                                                      terminalToken( "U1", "3" ), terminalToken( "U14", "1" ) } };
    auto [consistent, checkable] = checkConnectivity( *schematic, expected );
    BOOST_CHECK_EQUAL( checkable, 1 );
    BOOST_CHECK_EQUAL( consistent, 1 );
}


BOOST_AUTO_TEST_CASE( PhysicalConnectorPinsUseDefinitionNumbers )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "DC2382A-1.DSN" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    std::set<wxString> pinNumbers;

    for( const SCH_SHEET_PATH& path : schematic->BuildSheetListSortedByPageNumbers() )
    {
        for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

            if( symbol->GetRef( &path, false ) != wxS( "J3" ) )
                continue;

            for( SCH_PIN* pin : symbol->GetPins( &path ) )
                pinNumbers.insert( pin->GetNumber() );
        }
    }

    std::set<wxString> expected;

    for( int pin = 1; pin <= 20; ++pin )
        expected.insert( wxString::Format( wxS( "%d" ), pin ) );

    BOOST_CHECK_EQUAL_COLLECTIONS( pinNumbers.begin(), pinNumbers.end(), expected.begin(), expected.end() );

    std::vector<std::set<std::string>> expectedNets = { { terminalToken( "J3", "1" ), terminalToken( "Q1", "3" ) },
                                                        { terminalToken( "J3", "2" ), terminalToken( "J1", "2" ) },
                                                        { terminalToken( "J3", "3" ), terminalToken( "J1", "3" ) },
                                                        { terminalToken( "J3", "10" ), terminalToken( "J1", "10" ) },
                                                        { terminalToken( "J3", "13" ), terminalToken( "R19", "1" ) } };
    auto [consistent, checkable] = checkConnectivity( *schematic, expectedNets );
    BOOST_CHECK_EQUAL( checkable, 5 );
    BOOST_CHECK_EQUAL( consistent, 5 );
}


BOOST_AUTO_TEST_CASE( ModernPackageStreamsSupplyEmbeddedSymbolGeometry )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "DC2382A-1.DSN" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    SCH_SYMBOL* jumper = nullptr;

    for( const SCH_SHEET_PATH& path : schematic->BuildSheetListSortedByPageNumbers() )
    {
        for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

            if( symbol->GetRef( &path, false ) == wxS( "JP4" ) )
                jumper = symbol;
        }
    }

    BOOST_REQUIRE( jumper );
    BOOST_REQUIRE( jumper->GetLibSymbolRef() );

    int bodyRectangles = 0;

    for( const SCH_ITEM& item : jumper->GetLibSymbolRef()->GetDrawItems() )
    {
        if( item.Type() == SCH_SHAPE_T && static_cast<const SCH_SHAPE&>( item ).GetShape() == SHAPE_T::RECTANGLE )
            ++bodyRectangles;
    }

    BOOST_CHECK_EQUAL( bodyRectangles, 3 );
}


BOOST_AUTO_TEST_CASE( LegacyDisplayTypesRemainVisible )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "X375D_VER72.DSN" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    int checked = 0;

    for( const SCH_SHEET_PATH& path : schematic->BuildSheetListSortedByPageNumbers() )
    {
        for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

            if( symbol->GetRef( &path, false ) != wxS( "CLKOUT0" ) )
                continue;

            BOOST_CHECK( symbol->GetField( FIELD_T::REFERENCE )->IsVisible() );
            ++checked;
        }
    }

    BOOST_CHECK_EQUAL( checked, 1 );
}


BOOST_AUTO_TEST_CASE( LegacyHierarchicalBlockImport )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
    {
        BOOST_TEST_MESSAGE( "KICAD_ORCAD_CORPUS not set; skipping legacy hierarchy check." );
        return;
    }

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "SCH-20380.DSN" );

    if( dsn.empty() )
    {
        BOOST_TEST_MESSAGE( "SCH-20380.DSN not present in corpus; skipping legacy hierarchy check." );
        return;
    }

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );
    schematic->CurrentSheet().UpdateAllScreenReferences();

    size_t pages = 0;
    size_t components = 0;
    size_t wires = 0;

    for( const SCH_SHEET_PATH& path : schematic->BuildSheetListSortedByPageNumbers() )
    {
        SCH_SCREEN* screen = path.LastScreen();

        if( !screen )
            continue;

        ++pages;

        for( SCH_ITEM* item : screen->Items() )
        {
            if( item->Type() == SCH_SYMBOL_T
                && !static_cast<SCH_SYMBOL*>( item )->GetRef( &path, false ).StartsWith( wxS( "#" ) ) )
            {
                ++components;
            }
            else if( item->Type() == SCH_LINE_T
                     && ( static_cast<SCH_LINE*>( item )->GetLayer() == LAYER_WIRE
                          || static_cast<SCH_LINE*>( item )->GetLayer() == LAYER_BUS ) )
            {
                ++wires;
            }
        }
    }

    BOOST_CHECK_EQUAL( pages, 17u );
    BOOST_CHECK_EQUAL( components, 491u );
    BOOST_CHECK_EQUAL( wires, 9696u );
}


BOOST_AUTO_TEST_CASE( LegacyDsnDiodePinsUseLogicalPolarity )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "SCH-20380.DSN" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    const std::vector<std::set<std::string>> expected = {
        { terminalToken( "D1", "2" ), terminalToken( "C37", "1" ) },
        { terminalToken( "D1", "1" ), terminalToken( "R15", "1" ) }
    };
    auto [consistent, checkable] = checkConnectivity( *schematic, expected );
    BOOST_CHECK_EQUAL( checkable, expected.size() );
    BOOST_CHECK_EQUAL( consistent, expected.size() );
}


BOOST_AUTO_TEST_CASE( LegacyDsnEmbeddedSlashNetNameIsAuthoritative )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "SCH-20380.DSN" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    SCH_SHEET_LIST sheets = schematic->BuildSheetListSortedByPageNumbers();
    schematic->ConnectionGraph()->Recalculate( sheets, true );
    wxString j1Pin7Net;

    for( const auto& [key, subgraphs] : schematic->ConnectionGraph()->GetNetMap() )
    {
        for( CONNECTION_SUBGRAPH* subgraph : subgraphs )
        {
            for( SCH_ITEM* item : subgraph->GetItems() )
            {
                if( item->Type() != SCH_PIN_T )
                    continue;

                SCH_PIN*    pin = static_cast<SCH_PIN*>( item );
                SCH_SYMBOL* symbol = dynamic_cast<SCH_SYMBOL*>( pin->GetParentSymbol() );

                if( symbol && symbol->GetRef( &subgraph->GetSheet(), false ) == wxS( "J1" )
                    && pin->GetNumber() == wxS( "7" ) )
                {
                    j1Pin7Net = key.Name;
                }
            }
        }
    }

    BOOST_CHECK_EQUAL( j1Pin7Net, wxString( wxS( "BDM_{slash}RSTIN" ) ) );
}


BOOST_AUTO_TEST_CASE( UninstantiatedLegacyPageIsExcludedFromBoard )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "SCH-21095.DSN" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );
    schematic->CurrentSheet().UpdateAllScreenReferences();

    SCH_SHEET* mram = nullptr;
    SCH_SHEET* reset = nullptr;

    for( const SCH_SHEET_PATH& path : schematic->BuildSheetListSortedByPageNumbers() )
    {
        SCH_SHEET* sheet = path.Last();

        if( path.LastScreen()->GetFileName().Upper().Contains( wxS( "MRAM" ) ) )
            mram = sheet;
        else if( path.LastScreen()->GetFileName().Upper().Contains( wxS( "RESET" ) ) )
            reset = sheet;
    }

    BOOST_REQUIRE( mram );
    BOOST_REQUIRE( reset );
    BOOST_CHECK( mram->GetExcludedFromBoard() );
    BOOST_CHECK( !reset->GetExcludedFromBoard() );
}


BOOST_AUTO_TEST_CASE( LegacyFlatPageImport )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
    {
        BOOST_TEST_MESSAGE( "KICAD_ORCAD_CORPUS not set; skipping legacy flat-page check." );
        return;
    }

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "X375D_VER72.DSN" );

    if( dsn.empty() )
    {
        BOOST_TEST_MESSAGE( "X375D_VER72.DSN not present in corpus; skipping legacy flat-page check." );
        return;
    }

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD       plugin;
    WX_STRING_REPORTER reporter;
    plugin.SetReporter( &reporter );

    try
    {
        plugin.LoadSchematicFile( dsn.string(), schematic.get() );
    }
    catch( const std::exception& e )
    {
        BOOST_FAIL( e.what() << "\n" << reporter.GetMessages() );
        return;
    }

    size_t pages = 0;
    size_t components = 0;
    size_t segments = 0;

    for( const SCH_SHEET_PATH& path : schematic->BuildSheetListSortedByPageNumbers() )
    {
        SCH_SCREEN* screen = path.LastScreen();

        if( !screen )
            continue;

        ++pages;

        for( SCH_ITEM* item : screen->Items() )
        {
            if( item->Type() == SCH_SYMBOL_T
                && !static_cast<SCH_SYMBOL*>( item )->GetRef( &path, false ).StartsWith( wxS( "#" ) ) )
            {
                ++components;
            }
            else if( item->Type() == SCH_LINE_T
                     && ( static_cast<SCH_LINE*>( item )->GetLayer() == LAYER_WIRE
                          || static_cast<SCH_LINE*>( item )->GetLayer() == LAYER_BUS ) )
            {
                ++segments;
            }
        }
    }

    BOOST_CHECK_EQUAL( pages, 1u );
    BOOST_CHECK_EQUAL( components, 256u );
    BOOST_CHECK_EQUAL( segments, 1756u );
}


BOOST_AUTO_TEST_CASE( LegacyHierarchyPowerTableImport )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
    {
        BOOST_TEST_MESSAGE( "KICAD_ORCAD_CORPUS not set; skipping legacy power-table check." );
        return;
    }

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "DC1414B.DSN" );

    if( dsn.empty() )
    {
        BOOST_TEST_MESSAGE( "DC1414B.DSN not present in corpus; skipping legacy power-table check." );
        return;
    }

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );
    schematic->CurrentSheet().UpdateAllScreenReferences();

    size_t pages = 0;
    size_t components = 0;
    size_t segments = 0;

    for( const SCH_SHEET_PATH& path : schematic->BuildSheetListSortedByPageNumbers() )
    {
        SCH_SCREEN* screen = path.LastScreen();

        if( !screen )
            continue;

        ++pages;

        for( SCH_ITEM* item : screen->Items() )
        {
            if( item->Type() == SCH_SYMBOL_T
                && !static_cast<SCH_SYMBOL*>( item )->GetRef( &path, false ).StartsWith( wxS( "#" ) ) )
            {
                ++components;
            }
            else if( item->Type() == SCH_LINE_T
                     && ( static_cast<SCH_LINE*>( item )->GetLayer() == LAYER_WIRE
                          || static_cast<SCH_LINE*>( item )->GetLayer() == LAYER_BUS ) )
            {
                ++segments;
            }
        }
    }

    BOOST_CHECK_EQUAL( pages, 1u );
    BOOST_CHECK_EQUAL( components, 74u );
    BOOST_CHECK_EQUAL( segments, 458u );
}


BOOST_AUTO_TEST_CASE( Issue25005Hierarchy )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
    {
        BOOST_TEST_MESSAGE( "KICAD_ORCAD_CORPUS not set; skipping issue 25005." );
        return;
    }

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "CFW-002.DSN" );

    if( dsn.empty() )
    {
        BOOST_TEST_MESSAGE( "CFW-002.DSN not present in corpus; skipping issue 25005." );
        return;
    }

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    std::vector<SCH_SHEET*> topSheets = schematic->GetTopLevelSheets();
    BOOST_REQUIRE_EQUAL( topSheets.size(), 1u );

    SCH_SCREEN* rootScreen = topSheets.front()->GetScreen();
    size_t      sheets = 0;
    size_t      sheetPins = 0;

    for( SCH_ITEM* item : rootScreen->Items().OfType( SCH_SHEET_T ) )
    {
        SCH_SHEET* sheet = static_cast<SCH_SHEET*>( item );
        ++sheets;
        sheetPins += sheet->GetPins().size();
    }

    const std::vector<wxString> expectedNames = { wxS( "PAG_2" ), wxS( "PAG_3" ), wxS( "PAG_4" ), wxS( "PAG_5" ),
                                                  wxS( "PAG_6" ), wxS( "PAG_7" ), wxS( "PAG_8" ), wxS( "PAG_9" ) };
    const std::vector<size_t>   expectedPinCounts = { 31, 24, 39, 47, 40, 33, 26, 10 };
    SCH_SHEET_LIST              hierarchy = schematic->BuildSheetListSortedByPageNumbers();
    std::vector<wxString>       sheetNames;
    std::vector<size_t>         pinCounts;

    for( auto it = std::next( hierarchy.begin() ); it != hierarchy.end(); ++it )
    {
        SCH_SHEET*         sheet = it->Last();
        std::set<wxString> sheetPinNames;
        std::set<wxString> hierarchicalLabelNames;

        sheetNames.push_back( sheet->GetField( FIELD_T::SHEET_NAME )->GetText() );
        pinCounts.push_back( sheet->GetPins().size() );

        for( const SCH_SHEET_PIN* pin : sheet->GetPins() )
            sheetPinNames.insert( pin->GetText() );

        for( SCH_ITEM* item : sheet->GetScreen()->Items().OfType( SCH_HIER_LABEL_T ) )
            hierarchicalLabelNames.insert( static_cast<SCH_HIERLABEL*>( item )->GetText() );

        BOOST_CHECK_EQUAL_COLLECTIONS( sheetPinNames.begin(), sheetPinNames.end(), hierarchicalLabelNames.begin(),
                                       hierarchicalLabelNames.end() );
    }

    schematic->ConnectionGraph()->Recalculate( hierarchy, true );

    BOOST_CHECK_EQUAL( hierarchy.size(), 9u );
    BOOST_CHECK_EQUAL( sheets, 8u );
    BOOST_CHECK_EQUAL( sheetPins, 250u );
    BOOST_CHECK_EQUAL_COLLECTIONS( sheetNames.begin(), sheetNames.end(), expectedNames.begin(), expectedNames.end() );
    BOOST_CHECK_EQUAL_COLLECTIONS( pinCounts.begin(), pinCounts.end(), expectedPinCounts.begin(),
                                   expectedPinCounts.end() );
}


BOOST_AUTO_TEST_CASE( Issue25009PageOrderAndGraphics )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
    {
        BOOST_TEST_MESSAGE( "KICAD_ORCAD_CORPUS not set; skipping issue 25009." );
        return;
    }

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "SE_NGFOC-L_01.DSN" );

    if( dsn.empty() )
    {
        BOOST_TEST_MESSAGE( "SE_NGFOC-L_01.DSN not present in corpus; skipping issue 25009." );
        return;
    }

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    const std::vector<wxString> expectedNames = { wxS( "01.REV.HISTORY" ), wxS( "02.uC" ),         wxS( "03.CAN" ),
                                                  wxS( "04. Ethercat" ),   wxS( "05.EtherSynch" ), wxS( "06.RS-485" ),
                                                  wxS( "11.GPIO" ),        wxS( "12.Analog" ),     wxS( "13:IMU" ),
                                                  wxS( "14.Bridge" ),      wxS( "15.Encoder" ),    wxS( "29.uCPower" ),
                                                  wxS( "30.PowerSupply" ), wxS( "31.Expansion" ) };

    std::vector<SCH_SHEET*> sheets = schematic->GetTopLevelSheets();
    BOOST_REQUIRE_EQUAL( sheets.size(), expectedNames.size() );

    size_t wires = 0;
    size_t shapes = 0;
    size_t texts = 0;
    size_t tables = 0;

    for( size_t i = 0; i < sheets.size(); ++i )
    {
        BOOST_CHECK_EQUAL( sheets[i]->GetField( FIELD_T::SHEET_NAME )->GetText(), expectedNames[i] );

        for( SCH_ITEM* item : sheets[i]->GetScreen()->Items() )
        {
            if( item->Type() == SCH_LINE_T )
            {
                SCH_LINE* line = static_cast<SCH_LINE*>( item );
                BOOST_CHECK_EQUAL( line->GetLineWidth(), 0 );
                ++wires;
            }
            else if( item->Type() == SCH_SHAPE_T )
            {
                SCH_SHAPE* shape = static_cast<SCH_SHAPE*>( item );

                if( i == 0 )
                    BOOST_CHECK( shape->GetFillMode() == FILL_T::NO_FILL );

                ++shapes;
            }
            else if( item->Type() == SCH_TEXT_T )
            {
                ++texts;
            }
            else if( item->Type() == SCH_TABLE_T )
            {
                ++tables;
            }
        }
    }

    BOOST_CHECK_EQUAL( wires, 1921u );
    BOOST_CHECK_EQUAL( shapes, 168u );
    BOOST_CHECK_EQUAL( texts, 206u );
    BOOST_CHECK_EQUAL( tables, 0u );
}


// Set KICAD_ORCAD_CORPUS to verify that private OLB files yield pins or graphics.

BOOST_AUTO_TEST_CASE( OlbLibraryImport )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
    {
        BOOST_TEST_MESSAGE( "KICAD_ORCAD_CORPUS not set; skipping OrCAD OLB library import." );
        return;
    }

    namespace fs = std::filesystem;
    std::vector<fs::path> libs;

    for( auto it = fs::recursive_directory_iterator( fs::path( corpusEnv ),
                                                     fs::directory_options::skip_permission_denied );
         it != fs::recursive_directory_iterator(); ++it )
    {
        if( !it->is_regular_file() )
            continue;

        std::string ext = it->path().extension().string();
        std::transform( ext.begin(), ext.end(), ext.begin(),
                        []( unsigned char c )
                        {
                            return std::tolower( c );
                        } );

        if( ext == ".olb" )
            libs.push_back( it->path() );
    }

    std::sort( libs.begin(), libs.end() );
    BOOST_TEST_MESSAGE( "OrCAD OLB libraries: " << libs.size() );

    int totalSymbols = 0, emptySymbols = 0, checkedLibs = 0, rejectedLibs = 0, crashedLibs = 0;

    for( const fs::path& lib : libs )
    {
        SCH_IO_ORCAD             plugin;
        std::vector<LIB_SYMBOL*> symbols;

        if( !plugin.CanReadLibrary( lib.string() ) )
        {
            ++rejectedLibs;
            continue;
        }

        try
        {
            // Vector overload materializes all symbols O(n); per-name LoadSymbol rescans O(n^2).
            plugin.EnumerateSymbolLib( symbols, lib.string() );
        }
        catch( const std::exception& e )
        {
            ++crashedLibs;
            BOOST_TEST_MESSAGE( "  THROW  " << lib.filename().string() << " : " << e.what() );
            continue;
        }

        ++checkedLibs;
        int withGeometry = 0;

        for( LIB_SYMBOL* symbol : symbols )
        {
            BOOST_REQUIRE( symbol );
            ++totalSymbols;

            if( symbol->GetPinCount() > 0 || !symbol->GetDrawItems().empty() )
                ++withGeometry;
            else
                ++emptySymbols;
        }

        BOOST_TEST_MESSAGE( "  " << lib.filename().string() << " : " << symbols.size() << " symbols, " << withGeometry
                                 << " with pins/graphics" );
    }

    BOOST_TEST_MESSAGE( "OLB summary: " << checkedLibs << " libs, " << rejectedLibs << " rejected, " << crashedLibs
                                        << " crashed, " << totalSymbols << " symbols, " << emptySymbols << " empty" );

    // Bad streams must degrade gracefully, not throw; wholesale empty result means decode broke.
    BOOST_CHECK_EQUAL( crashedLibs, 0 );
    BOOST_CHECK_GT( totalSymbols, 0 );

    if( totalSymbols )
        BOOST_CHECK_LT( emptySymbols, totalSymbols / 2 );
}


// CutiePi (3 pages) imports as three sibling top-level sheets; off-page connectors keep own
// names; reference/value fields honor source display positions.
BOOST_AUTO_TEST_CASE( MultiPageFlatImport )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
    {
        BOOST_TEST_MESSAGE( "KICAD_ORCAD_CORPUS not set; skipping OrCAD multi-page import." );
        return;
    }

    namespace fs = std::filesystem;
    fs::path dsn = fs::path( corpusEnv ) / "cutiepi-board" / "CutiePi_V2.3-20210409.DSN";

    if( !fs::exists( dsn ) )
    {
        BOOST_TEST_MESSAGE( "CutiePi design not present in corpus; skipping." );
        return;
    }

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    BOOST_CHECK_EQUAL( schematic->Settings().m_DashedLineDashRatio, 3.0 );
    BOOST_CHECK_EQUAL( schematic->Settings().m_DashedLineGapRatio, 1.0 );

    // Pages become flat ordered top-level sheets, not a stitching root w/ children; "N - " prefix orders them.
    std::vector<SCH_SHEET*> tops = schematic->GetTopLevelSheets();
    BOOST_REQUIRE_EQUAL( tops.size(), 3u );

    BOOST_CHECK_EQUAL( tops[0]->GetField( FIELD_T::SHEET_NAME )->GetText(), wxS( "CONTENTS" ) );
    BOOST_CHECK_EQUAL( tops[1]->GetField( FIELD_T::SHEET_NAME )->GetText(), wxS( "CM4,USB HUB,AUDIO,MIC" ) );
    BOOST_CHECK_EQUAL( tops[2]->GetField( FIELD_T::SHEET_NAME )->GetText(), wxS( "CSI, DSI, HDMI, MCU" ) );

    std::set<wxString> globalLabels;

    for( SCH_SHEET* top : tops )
    {
        for( SCH_ITEM* item : top->GetScreen()->Items().OfType( SCH_GLOBAL_LABEL_T ) )
            globalLabels.insert( static_cast<SCH_LABEL_BASE*>( item )->GetText() );
    }

    // Off-page connectors carry own name (CAM0_IO1), not the local wire net (GPIO19) they sit on.
    BOOST_CHECK( globalLabels.count( wxS( "CAM0_IO1" ) ) );
    BOOST_CHECK( globalLabels.count( wxS( "AMP_SHUTDOWN" ) ) );

    // R3197 reference honors OrCAD display position (left of body), not computed fallback (right).
    bool checkedField = false;

    for( const SCH_SHEET_PATH& path : schematic->BuildSheetListSortedByPageNumbers() )
    {
        for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* sym = static_cast<SCH_SYMBOL*>( item );

            if( sym->GetRef( &path, false ) == wxS( "R3197" ) )
            {
                BOOST_CHECK_LT( sym->GetField( FIELD_T::REFERENCE )->GetPosition().x, sym->GetPosition().x );
                checkedField = true;
            }
        }
    }

    BOOST_CHECK( checkedField );
}


BOOST_AUTO_TEST_CASE( HierarchicalSymbolInstancesUseCanonicalSheetPaths )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
    {
        BOOST_TEST_MESSAGE( "KICAD_ORCAD_CORPUS not set; skipping flat-page instance-path check." );
        return;
    }

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "OCTOPAES_10.DSN" );

    if( dsn.empty() )
    {
        BOOST_TEST_MESSAGE( "OCTOPAES_10.DSN not present in corpus; skipping instance-path check." );
        return;
    }

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    int checked = 0;

    for( const SCH_SHEET_PATH& path : schematic->BuildSheetListSortedByPageNumbers() )
    {
        if( !path.LastScreen()->GetFileName().Contains( wxS( "CPLD Power" ) ) )
            continue;

        for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

            if( symbol->GetRef( &path, false ) != wxS( "P?" ) )
                continue;

            bool        canonical = std::any_of( symbol->GetInstances().begin(), symbol->GetInstances().end(),
                                                 [&]( const SCH_SYMBOL_INSTANCE& aInstance )
                                                 {
                                              return aInstance.m_Path == path.Path();
                                          } );
            std::string stored;

            for( const SCH_SYMBOL_INSTANCE& instance : symbol->GetInstances() )
                stored += instance.m_Path.AsString().ToStdString() + " ";

            BOOST_CHECK_MESSAGE( canonical, "expected=" << path.Path().AsString() << " stored=" << stored );
            ++checked;
        }
    }

    BOOST_CHECK_EQUAL( checked, 3 );
}


BOOST_AUTO_TEST_CASE( MultiPageHierarchyPreservesPortConnectivity )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
    {
        BOOST_TEST_MESSAGE( "KICAD_ORCAD_CORPUS not set; skipping multi-page hierarchy check." );
        return;
    }

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "HB1A-AAFM.DSN" );

    if( dsn.empty() )
    {
        BOOST_TEST_MESSAGE( "HB1A-AAFM.DSN not present in corpus; skipping multi-page hierarchy check." );
        return;
    }

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    SCH_SHEET_LIST sheets = schematic->BuildSheetListSortedByPageNumbers();
    schematic->ConnectionGraph()->Recalculate( sheets, true );

    std::map<std::tuple<std::string, std::string, std::string>, int> terminalNets;
    int                                                              netId = 0;

    for( const auto& [key, subgraphs] : schematic->ConnectionGraph()->GetNetMap() )
    {
        for( CONNECTION_SUBGRAPH* subgraph : subgraphs )
        {
            std::string page = subgraph->GetSheet().LastScreen()->GetFileName().ToStdString();

            for( SCH_ITEM* item : subgraph->GetItems() )
            {
                if( item->Type() != SCH_PIN_T )
                    continue;

                SCH_PIN*    pin = static_cast<SCH_PIN*>( item );
                SCH_SYMBOL* symbol = dynamic_cast<SCH_SYMBOL*>( pin->GetParentSymbol() );

                if( symbol )
                {
                    terminalNets[{ page, symbol->GetRef( &subgraph->GetSheet(), false ).ToStdString(),
                                   pin->GetNumber().ToStdString() }] = netId;
                }
            }
        }

        ++netId;
    }

    auto findNet = [&]( const std::string& aPage, const std::string& aRef, const std::string& aPin )
    {
        for( const auto& [terminal, id] : terminalNets )
        {
            if( std::get<0>( terminal ).find( aPage ) != std::string::npos && std::get<1>( terminal ) == aRef
                && std::get<2>( terminal ) == aPin )
            {
                return id;
            }
        }

        return -1;
    };

    int clockNet = findNet( "Clock Generator", "J1", "1" );
    int fmcNet = findNet( "FMC Connector", "P1", "H38" );
    BOOST_REQUIRE_NE( clockNet, -1 );
    BOOST_REQUIRE_NE( fmcNet, -1 );
    BOOST_CHECK_EQUAL( clockNet, fmcNet );

    BOOST_CHECK_EQUAL( findNet( "Clock Generator", "J1", "2" ), findNet( "FMC Connector", "P1", "G37" ) );
    BOOST_CHECK_EQUAL( findNet( "Clock Generator", "U12", "7" ), findNet( "MAX II CPLD", "U9", "73" ) );
    BOOST_CHECK_EQUAL( findNet( "Current Sense", "U2", "2" ), findNet( "PROM & Misc", "U6", "4" ) );

    int mvddUr = findNet( "Power & Control", "TP13", "1" );
    int mvddUl = findNet( "Power & Control", "TP30", "1" );
    int mvddLr = findNet( "Power & Control", "TP14", "1" );
    int mvddLl = findNet( "Power & Control", "TP31", "1" );
    BOOST_REQUIRE_NE( mvddUr, -1 );
    BOOST_REQUIRE_NE( mvddUl, -1 );
    BOOST_REQUIRE_NE( mvddLr, -1 );
    BOOST_REQUIRE_NE( mvddLl, -1 );
    BOOST_CHECK_NE( mvddUr, mvddUl );
    BOOST_CHECK_NE( mvddUr, mvddLr );
    BOOST_CHECK_NE( mvddUr, mvddLl );
    BOOST_CHECK_NE( mvddUl, mvddLr );
    BOOST_CHECK_NE( mvddUl, mvddLl );
    BOOST_CHECK_NE( mvddLr, mvddLl );

    int urLclk = findNet( "Link Ports NORTH_SOUTH", "U4", "V6" );
    int lrLclk = findNet( "Link Ports NORTH_SOUTH", "U8", "A13" );
    BOOST_REQUIRE_NE( urLclk, -1 );
    BOOST_REQUIRE_NE( lrLclk, -1 );
    BOOST_CHECK_EQUAL( urLclk, lrLclk );
    BOOST_CHECK_NE( urLclk, findNet( "Link Ports NORTH_SOUTH", "U4", "V7" ) );

    int p1c35 = findNet( "FMC Connector", "P1", "C35" );
    int p1c37 = findNet( "FMC Connector", "P1", "C37" );
    BOOST_REQUIRE_NE( p1c35, -1 );
    BOOST_REQUIRE_NE( p1c37, -1 );
    BOOST_CHECK_EQUAL( p1c35, p1c37 );
    BOOST_CHECK_EQUAL( terminalNetName( *schematic, wxS( "R265" ), wxS( "1" ) ).Lower(),
                       wxString( wxS( "ll_ul_ns_data_p_0" ) ) );
}


BOOST_AUTO_TEST_CASE( PowerSymbolPinSharesPartNet )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
    {
        BOOST_TEST_MESSAGE( "KICAD_ORCAD_CORPUS not set; skipping power-symbol connectivity check." );
        return;
    }

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "POWER_SOURCE_BOARD_20180717.DSN" );

    if( dsn.empty() )
    {
        BOOST_TEST_MESSAGE( "POWER_SOURCE_BOARD_20180717.DSN not present; skipping power-symbol connectivity check." );
        return;
    }

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    std::vector<std::set<std::string>> expected = {
        { terminalToken( "R410", "2" ), terminalToken( "R122", "1" ) }
    };
    auto [consistent, checkable] = checkConnectivity( *schematic, expected );
    BOOST_CHECK_EQUAL( checkable, 1 );
    BOOST_CHECK_EQUAL( consistent, 1 );
}


BOOST_AUTO_TEST_CASE( RepeatedHierarchicalBusPinsRemainScoped )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "meta_carrier_sch_rev1.dsn" );

    if( dsn.empty() )
    {
        BOOST_TEST_MESSAGE( "meta_carrier_sch_rev1.dsn not present in corpus; skipping repeated bus check." );
        return;
    }

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    std::vector<std::set<std::string>> expected = {
        { "J1.1", "J11.1" },
        { "J12.239", "J5.239" },
        { "J15.239", "J3.239" },
        { "J4.1", "J6.1" },
    };

    auto [consistent, checkable] = checkConnectivity( *schematic, expected );
    BOOST_CHECK_EQUAL( checkable, 4 );
    BOOST_CHECK_EQUAL( consistent, 4 );
    BOOST_CHECK_EQUAL( terminalNetName( *schematic, wxS( "C103" ), wxS( "1" ) ).Lower(),
                       wxString( wxS( "lf2_ext_cap_clock" ) ) );
}


BOOST_AUTO_TEST_CASE( NestedHierarchicalBusRangesPreserveConnectivity )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "meta_module_sch_rev1.dsn" );

    if( dsn.empty() )
    {
        BOOST_TEST_MESSAGE( "meta_module_sch_rev1.dsn not present in corpus; skipping nested bus check." );
        return;
    }

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    std::vector<std::set<std::string>> expected = {
        { terminalToken( "J1", "10" ), terminalToken( "U5", "B15" ) },
        { terminalToken( "J1", "100" ), terminalToken( "U7", "D11" ) },
    };

    auto [consistent, checkable] = checkConnectivity( *schematic, expected );
    BOOST_CHECK_EQUAL( checkable, 2 );
    BOOST_CHECK_EQUAL( consistent, 2 );
    BOOST_CHECK_EQUAL( terminalNetName( *schematic, wxS( "R138" ), wxS( "1" ) ).Lower(),
                       wxString( wxS( "we_wait_wr_p0_snow4-1" ) ) );
}


BOOST_AUTO_TEST_CASE( RenamedHierarchicalBusMembersPreserveConnectivity )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "HB1A-AAFM.DSN" );

    if( dsn.empty() )
    {
        BOOST_TEST_MESSAGE( "HB1A-AAFM.DSN not present in corpus; skipping renamed bus check." );
        return;
    }

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    std::vector<std::set<std::string>> expected = {
        { terminalToken( "P1", "G36" ), terminalToken( "U9", "36" ) },
        { terminalToken( "P1", "H37" ), terminalToken( "U9", "35" ) },
    };

    auto [consistent, checkable] = checkConnectivity( *schematic, expected );
    BOOST_CHECK_EQUAL( checkable, 2 );
    BOOST_CHECK_EQUAL( consistent, 2 );
}


BOOST_AUTO_TEST_CASE( RepeatedMultiPageFoldersKeepLeafNetsScoped )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "HB1A-AAFM.DSN" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    auto [consistent, checkable] =
            checkConnectivity( *schematic, { { terminalToken( "P1", "C10" ), terminalToken( "U4", "E16" ) },
                                             { terminalToken( "P1", "K10" ), terminalToken( "U8", "E16" ) },
                                             { terminalToken( "P1", "C11" ), terminalToken( "U4", "D16" ) },
                                             { terminalToken( "P1", "K11" ), terminalToken( "U8", "D16" ) } } );
    BOOST_CHECK_EQUAL( checkable, 4 );
    BOOST_CHECK_EQUAL( consistent, 4 );
}


BOOST_AUTO_TEST_CASE( DegenerateHierarchicalPinPlacementsUseDefinitionGeometry )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "buddy_sch_rev1.dsn" );

    if( dsn.empty() )
    {
        BOOST_TEST_MESSAGE( "buddy_sch_rev1.dsn not present in corpus; skipping block-pin geometry check." );
        return;
    }

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    std::vector<std::set<std::string>> expected = {
        { "J1.C10", "U1.J19" }, { "J1.C11", "U1.K19" }, { "J3.C10", "U1.W33" },
        { "J3.F28", "U1.J35" }, { "J1.E33", "U1.B38" }, { "J3.E33", "U1.AV40" },
    };

    auto [consistent, checkable] = checkConnectivity( *schematic, expected );
    BOOST_CHECK_EQUAL( checkable, 6 );
    BOOST_CHECK_EQUAL( consistent, 6 );
    BOOST_CHECK_EQUAL( terminalNetName( *schematic, wxS( "J6" ), wxS( "F35" ) ).Lower(),
                       wxString( wxS( "ctrl_n3_ea2" ) ) );

    int generatedLabels = 0;

    for( const SCH_SHEET_PATH& path : schematic->BuildSheetListSortedByPageNumbers() )
    {
        for( SCH_ITEM* item : path.LastScreen()->Items() )
        {
            if( item->Type() != SCH_LABEL_T )
                continue;

            SCH_LABEL_BASE* label = static_cast<SCH_LABEL_BASE*>( item );

            if( !label->GetText().Contains( wxS( "_ORCAD_" ) ) )
                continue;

            ++generatedLabels;
            BOOST_CHECK( label->GetTextColor() != KIGFX::COLOR4D::UNSPECIFIED );
            BOOST_CHECK_EQUAL( label->GetTextColor().a, 0.0 );
        }
    }

    BOOST_CHECK_GT( generatedLabels, 0 );
}


BOOST_AUTO_TEST_CASE( PlacedUnitsSelectPackagePinMaps )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
    {
        BOOST_TEST_MESSAGE( "KICAD_ORCAD_CORPUS not set; skipping placed-unit check." );
        return;
    }

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "MC2_REV1_16_2.DSN" );

    if( dsn.empty() )
    {
        BOOST_TEST_MESSAGE( "MC2_REV1_16_2.DSN not present; skipping placed-unit check." );
        return;
    }

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    std::vector<std::set<std::string>> expected = { { terminalToken( "CC2", "5" ), terminalToken( "FL11", "1" ) },
                                                    { terminalToken( "CC2", "6" ), terminalToken( "FL11", "2" ) } };
    auto [consistent, checkable] = checkConnectivity( *schematic, expected );
    BOOST_CHECK_EQUAL( checkable, 2 );
    BOOST_CHECK_EQUAL( consistent, 2 );
}


BOOST_AUTO_TEST_CASE( Dc2693aSmaOnlyDisplaysCenterPinNumber )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "710-DC2693A_REV02_PCA_SCHEMATIC.DSN" );

    if( dsn.empty() )
        return;

    SETTINGS_MANAGER           manager;
    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    SCH_SYMBOL* j1 = nullptr;

    for( const SCH_SHEET_PATH& path : schematic->BuildSheetListSortedByPageNumbers() )
    {
        j1 = findConvertedSymbol( *path.LastScreen(), path, wxS( "J1" ) );

        if( j1 )
            break;
    }

    BOOST_REQUIRE( j1 );
    BOOST_REQUIRE_EQUAL( j1->GetPins().size(), 5u );

    std::vector<wxString> displayedNumbers;

    for( const SCH_PIN* pin : j1->GetPins() )
    {
        if( j1->GetShowPinNumbers() && pin->IsVisible() && pin->GetNumberTextSize() > 0 )
            displayedNumbers.push_back( pin->GetNumber() );
    }

    BOOST_REQUIRE_EQUAL( displayedNumbers.size(), 1u );
    BOOST_CHECK_EQUAL( displayedNumbers.front(), wxS( "1" ) );
}


BOOST_AUTO_TEST_CASE( Cy8cproto040tDisplaysComponentPinNumbers )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign(
            corpusEnv, "CY8CPROTO-040T_PSoC_4000T_CapSense_Prototyping_Board_Schematic.DSN" );

    if( dsn.empty() )
        return;

    SETTINGS_MANAGER           manager;
    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    auto displayedPinNumbers = []( SCH_SYMBOL& aSymbol )
    {
        std::set<wxString> numbers;

        if( aSymbol.GetShowPinNumbers() )
        {
            for( const SCH_PIN* pin : aSymbol.GetPins() )
            {
                if( pin->IsVisible() && pin->GetNumberTextSize() > 0 )
                    numbers.insert( pin->GetNumber() );
            }
        }

        return numbers;
    };

    SCH_SYMBOL* j1 = nullptr;
    SCH_SYMBOL* u1 = nullptr;
    SCH_SYMBOL* j11 = nullptr;

    for( const SCH_SHEET_PATH& path : schematic->BuildSheetListSortedByPageNumbers() )
    {
        if( !j1 )
            j1 = findConvertedSymbol( *path.LastScreen(), path, wxS( "J1" ) );

        if( !u1 )
            u1 = findConvertedSymbol( *path.LastScreen(), path, wxS( "U1" ) );

        if( !j11 )
            j11 = findConvertedSymbol( *path.LastScreen(), path, wxS( "J11" ) );
    }

    BOOST_REQUIRE( j1 );
    BOOST_REQUIRE( u1 );
    BOOST_REQUIRE( j11 );

    BOOST_CHECK( j1->GetShowPinNames() );
    BOOST_CHECK( j1->GetShowPinNumbers() );
    BOOST_REQUIRE_EQUAL( j1->GetPins().size(), 20u );

    std::set<wxString> j1PinText;

    for( const SCH_PIN* pin : j1->GetPins() )
    {
        BOOST_CHECK( !pin->GetName().IsEmpty() );
        BOOST_CHECK( !pin->GetNumber().IsEmpty() );
        BOOST_CHECK_GT( pin->GetNameTextSize(), 0 );
        BOOST_CHECK_GT( pin->GetNumberTextSize(), 0 );
        j1PinText.insert( pin->GetName() );
        j1PinText.insert( pin->GetNumber() );
    }

    for( const SCH_ITEM& item : j1->GetLibSymbolRef()->GetDrawItems() )
    {
        if( item.Type() == SCH_TEXT_T )
            BOOST_CHECK( !j1PinText.contains( static_cast<const SCH_TEXT&>( item ).GetText() ) );
    }

    BOOST_CHECK_EQUAL( displayedPinNumbers( *u1 ).size(), 25u );
    BOOST_CHECK_EQUAL( displayedPinNumbers( *j11 ).size(), 10u );
    BOOST_CHECK( displayedPinNumbers( *u1 ).contains( wxS( "23" ) ) );
    BOOST_CHECK( displayedPinNumbers( *u1 ).contains( wxS( "H" ) ) );
    BOOST_CHECK( displayedPinNumbers( *j11 ).contains( wxS( "1" ) ) );
    BOOST_CHECK( displayedPinNumbers( *j11 ).contains( wxS( "10" ) ) );
}


BOOST_AUTO_TEST_CASE( Cy8ckit149EmbeddedBlockDiagramIsComplete )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "CY8CKIT-149 Schematic.DSN" );

    if( dsn.empty() )
        return;

    SETTINGS_MANAGER           manager;
    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    const wxImage* blockDiagram = nullptr;
    size_t         redShapesOnBlockDiagram = 0;
    size_t         elephantNotes = 0;

    for( const SCH_SHEET_PATH& path : schematic->BuildSheetListSortedByPageNumbers() )
    {
        if( path.Last()->GetName().Contains( wxS( "Block Diagram" ) ) )
        {
            for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_SHAPE_T ) )
            {
                const SCH_SHAPE& shape = static_cast<const SCH_SHAPE&>( *item );

                if( shape.GetStroke().GetColor() == KIGFX::COLOR4D( 1.0, 0.0, 0.0, 1.0 ) )
                    ++redShapesOnBlockDiagram;
            }
        }

        for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_TEXT_T ) )
        {
            const SCH_TEXT& text = static_cast<const SCH_TEXT&>( *item );

            if( text.GetText() == wxS( "*All Test Points are No Load" ) )
            {
                BOOST_REQUIRE( text.GetFont() );
                BOOST_CHECK_EQUAL( text.GetFont()->GetName(), wxS( "KiCad OrCAD Elephant" ) );
                int sourceBottom = path.Last()->GetName().Contains( wxS( "PSoC 4100S" ) ) ? 623 : 613;
                BOX2I ink = text.GetEffectiveTextShape( false, BOX2I(), ANGLE_0 )->BBox();
                ink.Offset( text.GetSchematicTextOffset( nullptr )
                            + text.GetOffsetToMatchSCH_FIELD( nullptr ) );
                BOOST_CHECK_SMALL( ink.GetY() - OrcadDbuToIu( 0, sourceBottom - 10 ).y,
                                   OrcadDbuToIu( 0, 1 ).y );
                BOOST_CHECK_SMALL( ink.GetBottom() - OrcadDbuToIu( 0, sourceBottom ).y,
                                   OrcadDbuToIu( 0, 1 ).y );
                ++elephantNotes;
            }
        }

        for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_BITMAP_T ) )
        {
            const wxImage* image = static_cast<SCH_BITMAP*>( item )->GetReferenceImage().GetImage().GetImageData();

            if( image && image->IsOk() && image->GetWidth() >= 2500 && image->GetHeight() >= 1000
                && ( !blockDiagram
                     || image->GetWidth() * image->GetHeight()
                                > blockDiagram->GetWidth() * blockDiagram->GetHeight() ) )
            {
                blockDiagram = image;
            }
        }
    }

    BOOST_REQUIRE_MESSAGE( blockDiagram, "CY8CKIT-149 block diagram was not rendered at full size" );
    BOOST_CHECK_EQUAL( redShapesOnBlockDiagram, 0u );
    BOOST_CHECK_EQUAL( elephantNotes, 2u );
    BOOST_CHECK( schematic->GetEmbeddedFiles()->HasFile( wxS( "KiCadOrCADElephant-Black.ttf" ) ) );
    BOOST_CHECK( schematic->GetAreFontsEmbedded() );

    std::array<size_t, 3> blueColumns{};
    std::array<size_t, 3> blueRows{};
    const int             width = blockDiagram->GetWidth();
    const int             height = blockDiagram->GetHeight();

    for( int y = 0; y < height; ++y )
    {
        for( int x = 0; x < width; ++x )
        {
            int red = blockDiagram->GetRed( x, y );
            int green = blockDiagram->GetGreen( x, y );
            int blue = blockDiagram->GetBlue( x, y );

            if( blue > red + 30 && blue > green + 20 )
            {
                ++blueColumns[std::min( 2, x * 3 / width )];
                ++blueRows[std::min( 2, y * 3 / height )];
            }
        }
    }

    const size_t minimumBluePixels = static_cast<size_t>( width ) * height / 500;

    BOOST_TEST_MESSAGE( "block diagram size=" << width << 'x' << height << " columns=" << blueColumns[0] << ','
                                                << blueColumns[1] << ',' << blueColumns[2] << " rows="
                                                << blueRows[0] << ',' << blueRows[1] << ',' << blueRows[2] );

    for( size_t count : blueColumns )
        BOOST_CHECK_GE( count, minimumBluePixels );

    for( size_t count : blueRows )
        BOOST_CHECK_GE( count, minimumBluePixels );
}


// CutiePi component fidelity: pin number/name visibility, off-page label orientation, hidden
// fields, no-connect markers.
BOOST_AUTO_TEST_CASE( ComponentDetailImport )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
    {
        BOOST_TEST_MESSAGE( "KICAD_ORCAD_CORPUS not set; skipping OrCAD component detail." );
        return;
    }

    namespace fs = std::filesystem;
    fs::path dsn = fs::path( corpusEnv ) / "cutiepi-board" / "CutiePi_V2.3-20210409.DSN";

    if( !fs::exists( dsn ) )
    {
        BOOST_TEST_MESSAGE( "CutiePi design not present in corpus; skipping." );
        return;
    }

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    std::map<wxString, SCH_SYMBOL*> symbols;
    std::multimap<wxString, int>    labelSpins;
    int                             noConnects = 0;

    for( SCH_SHEET* top : schematic->GetTopLevelSheets() )
    {
        SCH_SHEET_PATH path;
        path.push_back( top );

        for( SCH_ITEM* item : top->GetScreen()->Items() )
        {
            if( item->Type() == SCH_SYMBOL_T )
            {
                SCH_SYMBOL* sym = static_cast<SCH_SYMBOL*>( item );
                symbols[sym->GetRef( &path, false )] = sym;
            }
            else if( item->Type() == SCH_GLOBAL_LABEL_T )
            {
                SCH_LABEL_BASE* lbl = static_cast<SCH_LABEL_BASE*>( item );
                labelSpins.emplace( lbl->GetText(), (int) lbl->GetSpinStyle() );
            }
            else if( item->Type() == SCH_NO_CONNECT_T )
            {
                ++noConnects;
            }
        }
    }

    // Pin numbers/names show on ICs (flags 0x3), hide on passives (0x6)
    BOOST_REQUIRE( symbols.count( wxS( "U3" ) ) );
    BOOST_CHECK( symbols[wxS( "U3" )]->GetShowPinNumbers() );
    BOOST_CHECK( symbols[wxS( "U3" )]->GetShowPinNames() );
    BOOST_REQUIRE( symbols.count( wxS( "R3174" ) ) );
    BOOST_CHECK( !symbols[wxS( "R3174" )]->GetShowPinNumbers() );
    BOOST_CHECK( !symbols[wxS( "R3174" )]->GetShowPinNames() );

    // Ferrite bead value hidden, reference visible
    BOOST_REQUIRE( symbols.count( wxS( "FB8" ) ) );
    BOOST_CHECK( !symbols[wxS( "FB8" )]->GetField( FIELD_T::VALUE )->IsVisible() );
    BOOST_CHECK( symbols[wxS( "FB8" )]->GetField( FIELD_T::REFERENCE )->IsVisible() );

    // Display-prop field positions are canvas-space (anchor + offset), not through body-orientation
    // transform. FB8 (90-deg ferrite) reference lands right of origin; rotation transform would flip left.
    SCH_FIELD* fb8Ref = symbols[wxS( "FB8" )]->GetField( FIELD_T::REFERENCE );
    BOOST_CHECK_GT( fb8Ref->GetPosition().x, symbols[wxS( "FB8" )]->GetPosition().x );
    BOOST_CHECK( fb8Ref->GetHorizJustify() == GR_TEXT_H_ALIGN_LEFT );

    // FB8 stored angle compensates for KiCad re-rotating fields on 90-deg symbol, so text stays horizontal.
    BOOST_CHECK( fb8Ref->GetDrawRotation() == ANGLE_HORIZONTAL );

    // References render horizontal even on rotated symbols (ferrites, vertical R/C).
    for( const wxString& ref : { wxS( "R3186" ), wxS( "C2517" ), wxS( "R3189" ), wxS( "FB13" ), wxS( "FB9" ) } )
    {
        BOOST_REQUIRE_MESSAGE( symbols.count( ref ), ref );
        BOOST_CHECK( symbols[ref]->GetField( FIELD_T::REFERENCE )->GetDrawRotation() == ANGLE_HORIZONTAL );
    }

    // Value rotation is per-field from source: FB9 part number horizontal, C2517 "47pF" stays vertical.
    BOOST_CHECK( symbols[wxS( "FB9" )]->GetField( FIELD_T::VALUE )->GetDrawRotation() == ANGLE_HORIZONTAL );
    BOOST_CHECK( symbols[wxS( "C2517" )]->GetField( FIELD_T::VALUE )->GetDrawRotation() == ANGLE_VERTICAL );

    // Power net names read horizontal even on rotated power symbols (REG1V8/REG3V3).
    bool checkedPower = false;

    for( SCH_SHEET* top : schematic->GetTopLevelSheets() )
    {
        for( SCH_ITEM* item : top->GetScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* sym = static_cast<SCH_SYMBOL*>( item );
            wxString    val = sym->GetField( FIELD_T::VALUE )->GetText();

            if( val == wxS( "REG1V8" ) || val == wxS( "REG3V3" ) )
            {
                BOOST_CHECK( sym->GetField( FIELD_T::VALUE )->GetDrawRotation() == ANGLE_HORIZONTAL );
                checkedPower = true;
            }
        }
    }

    BOOST_CHECK( checkedPower );

    // Unconnected IC pins get no-connect markers (U3 15 NC + U580 NC/ORG)
    BOOST_CHECK_GE( noConnects, 17 );

    // Off-page connectors on vertical wires point up/down, not left/right.
    auto hasSpin = [&]( const wxString& aText, SPIN_STYLE::SPIN aSpin )
    {
        auto range = labelSpins.equal_range( aText );

        for( auto it = range.first; it != range.second; ++it )
        {
            if( it->second == (int) aSpin )
                return true;
        }

        return false;
    };

    BOOST_CHECK( hasSpin( wxS( "VOLDN" ), SPIN_STYLE::UP ) );
    BOOST_CHECK( hasSpin( wxS( "MUTEP" ), SPIN_STYLE::BOTTOM ) );
}


BOOST_AUTO_TEST_SUITE_END()
