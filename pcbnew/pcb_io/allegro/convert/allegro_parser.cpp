/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright Quilter
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "convert/allegro_parser.h"

#include <array>
#include <chrono>
#include <cstring>

#include <wx/sstream.h>
#include <wx/log.h>
#include <wx/translation.h>

#include <core/profile.h>
#include <core/type_helpers.h>
#include <ki_exception.h>

using namespace ALLEGRO;


/**
 * Flag to enable debug output of Allegro parsing
 *
 * Use "KICAD_ALLEGRO_PARSER" to enable debug output.
 *
 * @ingroup trace_env_vars
 */
static const wxChar* const traceAllegroParser = wxT( "KICAD_ALLEGRO_PARSER" );
static const wxChar* const traceAllegroParserBlocks = wxT( "KICAD_ALLEGRO_PARSER_BLOCKS" );
static const wxChar* const traceAllegroPerf = wxT( "KICAD_ALLEGRO_PERF" );


static FILE_HEADER::LINKED_LIST ReadLL( FILE_STREAM& aStream, FMT_VER aVer = FMT_VER::V_160 )
{
    uint32_t w1 = aStream.ReadU32();
    uint32_t w2 = aStream.ReadU32();

    if( aVer >= FMT_VER::V_180 )
    {
        // V18 stores head (chain start key) first, tail (sentinel key) second
        return FILE_HEADER::LINKED_LIST{
            .m_Tail = w2,
            .m_Head = w1,
        };
    }

    // V16/V17 stores tail (sentinel pointer) first, head (chain start) second
    return FILE_HEADER::LINKED_LIST{
        .m_Tail = w1,
        .m_Head = w2,
    };
}


static double ReadAllegroFloat( FILE_STREAM& aStream )
{
    const uint32_t a = aStream.ReadU32();
    const uint32_t b = aStream.ReadU32();

    // Combine into a 64-bit integer
    const uint64_t combined = ( static_cast<uint64_t>( a ) << 32 ) + b;

    // Copy the bits into a double
    double result = 0;
    std::memcpy( &result, &combined, sizeof( result ) );
    return result;
}


template <typename ARRAY>
static void ReadArrayU32( FILE_STREAM& aStream, ARRAY& aArray )
{
    for( size_t i = 0; i < aArray.size(); i++ )
    {
        aArray[i] = aStream.ReadU32();
    }
}


template <typename T>
static T ReadField( FILE_STREAM& aStream )
{
    T field;
    if constexpr( std::is_same_v<T, uint32_t> )
    {
        field = aStream.ReadU32();
    }
    else if constexpr( std::is_same_v<T, uint8_t> )
    {
        field = aStream.ReadU8();
    }
    else if constexpr( std::is_same_v<T, uint16_t> )
    {
        field = aStream.ReadU16();
    }
    else if constexpr( std::is_same_v<T, int16_t> )
    {
        field = aStream.ReadS16();
    }
    else if constexpr( requires { std::tuple_size<T>::value; } )
    {
        for( size_t i = 0; i < std::tuple_size_v<T>; ++i )
            field[i] = aStream.ReadU32();
    }
    else
    {
        static_assert( always_false<T>::value, "Unsupported type for ReadField" );
    }

    return field;
}


/**
 * Read a single conditional field from the stream, if it exists at the current
 * version.
 */
template <typename COND_T>
static void ReadCond( FILE_STREAM& aStream, FMT_VER aVer, COND_T& aField )
{
    if( aField.exists( aVer ) )
    {
        aField = ReadField<typename COND_T::value_type>( aStream );
    }
}


FMT_VER HEADER_PARSER::FormatFromMagic( uint32_t aMagic )
{
    uint32_t masked = aMagic & 0xFFFFFF00;

    switch( masked )
    {
    case 0x00130000: return FMT_VER::V_160;
    case 0x00130400: return FMT_VER::V_162;
    case 0x00130C00: return FMT_VER::V_164;
    case 0x00131000: return FMT_VER::V_165;
    case 0x00131500: return FMT_VER::V_166;
    case 0x00140400:
    case 0x00140500:
    case 0x00140600:
    case 0x00140700: return FMT_VER::V_172;
    case 0x00140900:
    case 0x00140E00: return FMT_VER::V_174;
    case 0x00141500: return FMT_VER::V_175;
    case 0x00150000: return FMT_VER::V_180;
    default: break;
    }

    // Pre-v16 Allegro files use a fundamentally different binary format
    // that cannot be parsed by this importer. The header is still compatible enough to read
    // the Allegro version string for a helpful error message.
    uint32_t majorVer = ( aMagic >> 16 ) & 0xFFFF;

    if( majorVer <= 0x0012 )
        return FMT_VER::V_PRE_V16;

    // Struct sizes depend on version, so we can't do anything useful with
    // unrecognized formats. Report the magic and ask the user to report it.
    THROW_IO_ERROR( wxString::Format( "Unknown Allegro file version %#010x (rev %d)", aMagic, majorVer - 3 ) );
}


std::unique_ptr<ALLEGRO::FILE_HEADER> HEADER_PARSER::ParseHeader()
{
    auto header = std::make_unique<ALLEGRO::FILE_HEADER>();

    uint32_t fileMagic = m_stream.ReadU32();
    m_fmtVer = FormatFromMagic( fileMagic );

    header->m_Magic = fileMagic;
    ReadArrayU32( m_stream, header->m_Unknown1 );
    header->m_ObjectCount = m_stream.ReadU32();
    ReadArrayU32( m_stream, header->m_Unknown2 );

    if( m_fmtVer >= FMT_VER::V_180 )
    {
        // V18 has 28 linked lists: 5 new at the front, then the same 22 as v16, then 1 new.
        // Positions 0-4 are new v18-only LLs
        header->m_LL_V18_1 = ReadLL( m_stream, m_fmtVer );
        header->m_LL_V18_2 = ReadLL( m_stream, m_fmtVer );
        header->m_LL_V18_3 = ReadLL( m_stream, m_fmtVer );
        header->m_LL_V18_4 = ReadLL( m_stream, m_fmtVer );
        header->m_LL_V18_5 = ReadLL( m_stream, m_fmtVer );

        // Positions 5-22 match v16 positions 0-17
        header->m_LL_0x04 = ReadLL( m_stream, m_fmtVer );
        header->m_LL_0x06 = ReadLL( m_stream, m_fmtVer );
        header->m_LL_0x0C = ReadLL( m_stream, m_fmtVer );
        header->m_LL_Shapes = ReadLL( m_stream, m_fmtVer );
        header->m_LL_0x14 = ReadLL( m_stream, m_fmtVer );
        header->m_LL_0x1B_Nets = ReadLL( m_stream, m_fmtVer );
        header->m_LL_0x1C = ReadLL( m_stream, m_fmtVer );
        header->m_LL_0x24_0x28 = ReadLL( m_stream, m_fmtVer );
        header->m_LL_Unknown1 = ReadLL( m_stream, m_fmtVer );
        header->m_LL_0x2B = ReadLL( m_stream, m_fmtVer );
        header->m_LL_0x03_0x30 = ReadLL( m_stream, m_fmtVer );
        header->m_LL_0x0A = ReadLL( m_stream, m_fmtVer );
        header->m_LL_0x1D_0x1E_0x1F = ReadLL( m_stream, m_fmtVer );
        header->m_LL_Unknown2 = ReadLL( m_stream, m_fmtVer );
        header->m_LL_0x38 = ReadLL( m_stream, m_fmtVer );
        header->m_LL_0x2C = ReadLL( m_stream, m_fmtVer );
        header->m_LL_0x0C_2 = ReadLL( m_stream, m_fmtVer );
        header->m_LL_Unknown3 = ReadLL( m_stream, m_fmtVer );

        // Positions 23-26 match v16 positions 18-21, but 0x36 and Unknown5 are swapped
        header->m_LL_Unknown5 = ReadLL( m_stream, m_fmtVer );
        header->m_LL_0x36 = ReadLL( m_stream, m_fmtVer );
        header->m_LL_Unknown6 = ReadLL( m_stream, m_fmtVer );
        header->m_LL_0x0A_2 = ReadLL( m_stream, m_fmtVer );

        // Position 27: new v18 LL
        header->m_LL_V18_6 = ReadLL( m_stream, m_fmtVer );

        // V18 stores 0x35 extents as two scalars after the linked lists
        header->m_0x35_Start = m_stream.ReadU32();
        header->m_0x35_End = m_stream.ReadU32();

        // 0x27_End and StringsCount relocated to Unknown2 in v18
        header->m_0x27_End = header->m_Unknown2[4];
        header->m_StringsCount = header->m_Unknown2[7];
    }
    else
    {
        // 18 linked lists in the shared header region
        header->m_LL_0x04 = ReadLL( m_stream );
        header->m_LL_0x06 = ReadLL( m_stream );
        header->m_LL_0x0C = ReadLL( m_stream );
        header->m_LL_Shapes = ReadLL( m_stream );
        header->m_LL_0x14 = ReadLL( m_stream );
        header->m_LL_0x1B_Nets = ReadLL( m_stream );
        header->m_LL_0x1C = ReadLL( m_stream );
        header->m_LL_0x24_0x28 = ReadLL( m_stream );
        header->m_LL_Unknown1 = ReadLL( m_stream );
        header->m_LL_0x2B = ReadLL( m_stream );
        header->m_LL_0x03_0x30 = ReadLL( m_stream );
        header->m_LL_0x0A = ReadLL( m_stream );
        header->m_LL_0x1D_0x1E_0x1F = ReadLL( m_stream );
        header->m_LL_Unknown2 = ReadLL( m_stream );
        header->m_LL_0x38 = ReadLL( m_stream );
        header->m_LL_0x2C = ReadLL( m_stream );
        header->m_LL_0x0C_2 = ReadLL( m_stream );
        header->m_LL_Unknown3 = ReadLL( m_stream );

        header->m_0x35_Start = m_stream.ReadU32();
        header->m_0x35_End = m_stream.ReadU32();
        header->m_LL_0x36 = ReadLL( m_stream );
        header->m_LL_Unknown5 = ReadLL( m_stream );
        header->m_LL_Unknown6 = ReadLL( m_stream );
        header->m_LL_0x0A_2 = ReadLL( m_stream );
        header->m_Unknown3 = m_stream.ReadU32();
    }

    m_stream.ReadBytes( header->m_AllegroVersion.data(), header->m_AllegroVersion.size() );
    header->m_Unknown4 = m_stream.ReadU32();
    header->m_MaxKey = m_stream.ReadU32();

    if( m_fmtVer >= FMT_VER::V_180 )
    {
        header->m_Unknown5_V18 = ReadField<std::array<uint32_t, 9>>( m_stream );
    }
    else
    {
        header->m_Unknown5 = ReadField<std::array<uint32_t, 17>>( m_stream );
    }

    {
        uint8_t units = m_stream.ReadU8();

        switch( units )
        {
        case BOARD_UNITS::IMPERIAL:
        case BOARD_UNITS::METRIC: header->m_BoardUnits = static_cast<BOARD_UNITS>( units ); break;
        default: THROW_IO_ERROR( wxString::Format( "Unknown board units %d", units ) );
        }

        m_stream.Skip( 3 );
    }

    header->m_Unknown6 = m_stream.ReadU32();

    if( m_fmtVer < FMT_VER::V_180 )
    {
        header->m_Unknown7 = m_stream.ReadU32();
        header->m_0x27_End = m_stream.ReadU32();
    }

    header->m_Unknown8 = m_stream.ReadU32();

    if( m_fmtVer < FMT_VER::V_180 )
    {
        header->m_StringsCount = m_stream.ReadU32();
    }

    ReadArrayU32( m_stream, header->m_Unknown9 );

    header->m_UnitsDivisor = m_stream.ReadU32();

    m_stream.SkipU32( 110 );

    for( size_t i = 0; i < header->m_LayerMap.size(); ++i )
    {
        header->m_LayerMap[i].m_A = m_stream.ReadU32();
        header->m_LayerMap[i].m_LayerList0x2A = m_stream.ReadU32();
    }

    wxLogTrace( traceAllegroParser, wxT( "Parsed header: %d objects, %d strings" ),
                header->m_ObjectCount, header->m_StringsCount );

    return header;
}


static void ReadStringMap( FILE_STREAM& stream, DB& aDb, uint32_t count )
{
    stream.Seek( RAW_BOARD::STRING_TABLE_OFFSET );

    for( uint32_t i = 0; i < count; ++i )
    {
        uint32_t id = stream.ReadU32();
        wxString str = stream.ReadString( true );

        aDb.AddString( id, std::move( str ) );
    }
}


static LAYER_INFO ParseLayerInfo( FILE_STREAM& aStream )
{
    uint8_t classCode = aStream.ReadU8();
    uint8_t subclassCode = aStream.ReadU8();

    // Don't try to be clever and assign enums here - there are loads of them,
    // and we don't have a perfect map yet.
    return LAYER_INFO{
        .m_Class = classCode,
        .m_Subclass = subclassCode,
    };
}


static std::unique_ptr<BLOCK_BASE> ParseBlock_0x01_ARC( FILE_STREAM& aStream, FMT_VER aVer )
{
    auto block = std::make_unique<BLOCK<BLK_0x01_ARC>>( 0x01, aStream.Position() );

    auto& data = block->GetData();

    aStream.Skip( 1 );

    data.m_UnknownByte = aStream.ReadU8();
    data.m_SubType = aStream.ReadU8();
    data.m_Key = aStream.ReadU32();
    data.m_Next = aStream.ReadU32();
    data.m_Parent = aStream.ReadU32();
    data.m_Unknown1 = aStream.ReadU32();

    ReadCond( aStream, aVer, data.m_Unknown6 );

    data.m_Width = aStream.ReadU32();

    data.m_StartX = aStream.ReadS32();
    data.m_StartY = aStream.ReadS32();
    data.m_EndX = aStream.ReadS32();
    data.m_EndY = aStream.ReadS32();

    data.m_CenterX = ReadAllegroFloat( aStream );
    data.m_CenterY = ReadAllegroFloat( aStream );
    data.m_Radius = ReadAllegroFloat( aStream );

    for( size_t i = 0; i < data.m_BoundingBoxCoords.size(); ++i )
    {
        data.m_BoundingBoxCoords[i] = aStream.ReadS32();
    }

    return block;
}


static std::unique_ptr<BLOCK_BASE> ParseBlock_0x03( FILE_STREAM& aStream, FMT_VER aVer )
{
    auto block = std::make_unique<BLOCK<BLK_0x03_FIELD>>( 0x03, aStream.Position() );

    auto& data = block->GetData();

    aStream.Skip( 1 );

    data.m_Hdr1 = aStream.ReadU16();
    data.m_Key = aStream.ReadU32();
    data.m_Next = aStream.ReadU32();

    ReadCond( aStream, aVer, data.m_Unknown1 );

    data.m_SubType = aStream.ReadU8();
    data.m_Hdr2 = aStream.ReadU8();
    data.m_Size = aStream.ReadU16();

    ReadCond( aStream, aVer, data.m_Unknown2 );

    wxLogTrace( traceAllegroParserBlocks, wxT( "  Parsed block 0x03: subtype %#02x, size %d" ), data.m_SubType,
                data.m_Size );

    switch( data.m_SubType )
    {
    case 0x65:
        // Nothing for this one?
        break;
    case 0x64:
    case 0x66:
    case 0x67:
    case 0x6A:
    {
        data.m_Substruct = aStream.ReadU32();
        break;
    }
    case 0x69:
    {
        data.m_Substruct = std::array<uint32_t, 2>{
            aStream.ReadU32(),
            aStream.ReadU32(),
        };
        break;
    }
    case 0x68:
    case 0x6B:
    case 0x6D:
    case 0x6E:
    case 0x6F:
    case 0x71:
    case 0x73:
    case 0x78:
    {
        data.m_Substruct = aStream.ReadStringFixed( data.m_Size, true );
        break;
    }
    case 0x6C:
    {
        BLK_0x03_FIELD::SUB_0x6C sub;
        sub.m_NumEntries = aStream.ReadU32();

        if( sub.m_NumEntries > 1000000 )
        {
            THROW_IO_ERROR( wxString::Format(
                    "Block 0x03 subtype 0x6C entry count %u exceeds limit at offset %#010zx",
                    sub.m_NumEntries, aStream.Position() ) );
        }

        sub.m_Entries.reserve( sub.m_NumEntries );
        for( uint32_t i = 0; i < sub.m_NumEntries; ++i )
        {
            sub.m_Entries.push_back( aStream.ReadU32() );
        }

        data.m_Substruct = std::move( sub );
        break;
    }
    case 0x70:
    case 0x74:
    {
        BLK_0x03_FIELD::SUB_0x70_0x74 sub;

        sub.m_X0 = aStream.ReadU16();
        sub.m_X1 = aStream.ReadU16();

        const size_t numEntries = ( sub.m_X1 + 4 * sub.m_X0 );
        sub.m_Entries.reserve( numEntries );
        for( size_t i = 0; i < numEntries; ++i )
        {
            sub.m_Entries.push_back( aStream.ReadU8() );
        }
        data.m_Substruct = std::move( sub );
        break;
    }
    case 0xF6:
    {
        BLK_0x03_FIELD::SUB_0xF6 sub;
        for( size_t i = 0; i < sub.m_Entries.size(); ++i )
        {
            sub.m_Entries[i] = aStream.ReadU32();
        }
        data.m_Substruct = std::move( sub );
        break;
    }
    default:
    {
        if( data.m_Size == 4 )
        {
            data.m_Substruct = aStream.ReadU32();
        }
        else if( data.m_Size == 8 )
        {
            std::array<uint32_t, 2> sub;
            sub[0] = aStream.ReadU32();
            sub[1] = aStream.ReadU32();
            data.m_Substruct = std::move( sub );
        }
        else
        {
            THROW_IO_ERROR(
                    wxString::Format( "Unknown substruct type %#02x with size %d", data.m_SubType, data.m_Size ) );
        }
        break;
    }
    }

    return block;
}


static std::unique_ptr<BLOCK_BASE> ParseBlock_0x04_NET_ASSIGNMENT( FILE_STREAM& aStream, FMT_VER aVer )
{
    auto block = std::make_unique<BLOCK<BLK_0x04_NET_ASSIGNMENT>>( 0x04, aStream.Position() );

    auto& data = block->GetData();

    data.m_Type = aStream.ReadU8();
    data.m_R = aStream.ReadU16();
    data.m_Key = aStream.ReadU32();
    data.m_Next = aStream.ReadU32();
    data.m_Net = aStream.ReadU32();
    data.m_ConnItem = aStream.ReadU32();

    ReadCond( aStream, aVer, data.m_Unknown );

    return block;
}


static std::unique_ptr<BLOCK_BASE> ParseBlock_0x05_TRACK( FILE_STREAM& aStream, FMT_VER aVer )
{
    auto block = std::make_unique<BLOCK<BLK_0x05_TRACK>>( 0x05, aStream.Position() );

    auto& data = block->GetData();

    aStream.Skip( 1 );

    data.m_Layer = ParseLayerInfo( aStream );
    data.m_Key = aStream.ReadU32();
    data.m_Next = aStream.ReadU32();
    data.m_NetAssignment = aStream.ReadU32();
    data.m_UnknownPtr1 = aStream.ReadU32();
    data.m_Unknown2 = aStream.ReadU32();
    data.m_Unknown3 = aStream.ReadU32();
    data.m_UnknownPtr2a = aStream.ReadU32();
    data.m_UnknownPtr2b = aStream.ReadU32();
    data.m_Unknown4 = aStream.ReadU32();
    data.m_UnknownPtr3a = aStream.ReadU32();
    data.m_UnknownPtr3b = aStream.ReadU32();

    ReadCond( aStream, aVer, data.m_Unknown5a );
    ReadCond( aStream, aVer, data.m_Unknown5b );

    data.m_FirstSegPtr = aStream.ReadU32();
    data.m_UnknownPtr5 = aStream.ReadU32();
    data.m_Unknown6 = aStream.ReadU32();

    return block;
}


static std::unique_ptr<BLOCK_BASE> ParseBlock_0x06( FILE_STREAM& stream, FMT_VER aVer )
{
    auto block = std::make_unique<BLOCK<BLK_0x06_COMPONENT>>( 0x06, stream.Position() );

    auto& data = block->GetData();

    stream.Skip( 3 );

    data.m_Key = stream.ReadU32();
    data.m_Next = stream.ReadU32();
    data.m_CompDeviceType = stream.ReadU32();
    data.m_SymbolName = stream.ReadU32();
    data.m_FirstInstPtr = stream.ReadU32();
    data.m_PtrFunctionSlot = stream.ReadU32();
    data.m_PtrPinNumber = stream.ReadU32();
    data.m_Fields = stream.ReadU32();

    ReadCond( stream, aVer, data.m_Unknown1 );

    return block;
}


static std::unique_ptr<BLOCK_BASE> ParseBlock_0x07( FILE_STREAM& stream, FMT_VER aVer )
{
    auto block = std::make_unique<BLOCK<BLK_0x07_COMPONENT_INST>>( 0x07, stream.Position() );

    auto& data = block->GetData();

    stream.Skip( 3 );

    data.m_Key = stream.ReadU32();
    data.m_Next = stream.ReadU32();

    ReadCond( stream, aVer, data.m_UnknownPtr1 );
    ReadCond( stream, aVer, data.m_Unknown2 );
    ReadCond( stream, aVer, data.m_Unknown3 );

    data.m_FpInstPtr = stream.ReadU32();

    ReadCond( stream, aVer, data.m_Unknown4 );

    data.m_RefDesStrPtr = stream.ReadU32();
    data.m_FunctionInstPtr = stream.ReadU32();
    data.m_X03Ptr = stream.ReadU32();
    data.m_Unknown5 = stream.ReadU32();
    data.m_FirstPadPtr = stream.ReadU32();

    return block;
}


static std::unique_ptr<BLOCK_BASE> ParseBlock_0x08( FILE_STREAM& aStream, FMT_VER aVer )
{
    auto block = std::make_unique<BLOCK<BLK_0x08_PIN_NUMBER>>( 0x08, aStream.Position() );

    auto& data = block->GetData();

    data.m_Type = aStream.ReadU8();
    data.m_R = aStream.ReadU16();
    data.m_Key = aStream.ReadU32();

    ReadCond( aStream, aVer, data.m_Previous );
    ReadCond( aStream, aVer, data.m_StrPtr16x );

    data.m_Next = aStream.ReadU32();

    ReadCond( aStream, aVer, data.m_StrPtr );

    data.m_PinNamePtr = aStream.ReadU32();

    ReadCond( aStream, aVer, data.m_Unknown1 );

    data.m_Ptr4 = aStream.ReadU32();

    return block;
}


static std::unique_ptr<BLOCK_BASE> ParseBlock_0x09( FILE_STREAM& aStream, FMT_VER aVer )
{
    auto block = std::make_unique<BLOCK<BLK_0x09_FILL_LINK>>( 0x09, aStream.Position() );

    auto& data = block->GetData();

    aStream.Skip( 3 );

    data.m_Key = aStream.ReadU32();

    for( size_t i = 0; i < data.m_UnknownArray.size(); ++i )
    {
        data.m_UnknownArray[i] = aStream.ReadU32();
    }

    ReadCond( aStream, aVer, data.m_Unknown1 );

    data.m_UnknownPtr1 = aStream.ReadU32();
    data.m_UnknownPtr2 = aStream.ReadU32();
    data.m_Unknown2 = aStream.ReadU32();
    data.m_UnknownPtr3 = aStream.ReadU32();
    data.m_UnknownPtr4 = aStream.ReadU32();

    ReadCond( aStream, aVer, data.m_Unknown3 );

    return block;
}


static std::unique_ptr<BLOCK_BASE> ParseBlock_0x0A_DRC( FILE_STREAM& aStream, FMT_VER aVer )
{
    auto block = std::make_unique<BLOCK<BLK_0x0A_DRC>>( 0x0A, aStream.Position() );

    auto& data = block->GetData();

    data.m_T = aStream.ReadU8();
    data.m_Layer = ParseLayerInfo( aStream );
    data.m_Key = aStream.ReadU32();
    data.m_Next = aStream.ReadU32();
    data.m_Unknown1 = aStream.ReadU32();

    ReadCond( aStream, aVer, data.m_Unknown2 );

    for( size_t i = 0; i < data.m_Coords.size(); ++i )
    {
        data.m_Coords[i] = aStream.ReadS32();
    }

    ReadArrayU32( aStream, data.m_Unknown4 );
    ReadArrayU32( aStream, data.m_Unknown5 );

    ReadCond( aStream, aVer, data.m_Unknown6 );

    return block;
}


static std::unique_ptr<BLOCK_BASE> ParseBlock_0x0C( FILE_STREAM& aStream, FMT_VER aVer )
{
    auto block = std::make_unique<BLOCK<BLK_0x0C_PIN_DEF>>( 0x0C, aStream.Position() );

    auto& data = block->GetData();

    data.m_T = aStream.ReadU8();
    data.m_Layer = ParseLayerInfo( aStream );

    data.m_Key = aStream.ReadU32();
    data.m_Next = aStream.ReadU32();

    data.m_Unknown1 = aStream.ReadU32();
    data.m_Unknown2 = aStream.ReadU32();

    // Older packed format
    ReadCond( aStream, aVer, data.m_Shape );
    ReadCond( aStream, aVer, data.m_DrillChar );
    ReadCond( aStream, aVer, data.m_UnknownPadding );

    // V17.2+
    ReadCond( aStream, aVer, data.m_Shape16x );
    ReadCond( aStream, aVer, data.m_DrillChars );
    ReadCond( aStream, aVer, data.m_Unknown_16x );

    data.m_Unknown4 = aStream.ReadU32();

    for( size_t i = 0; i < data.m_Coords.size(); ++i )
    {
        data.m_Coords[i] = aStream.ReadS32();
    }

    for( size_t i = 0; i < data.m_Size.size(); ++i )
    {
        data.m_Size[i] = aStream.ReadS32();
    }

    for( size_t i = 0; i < data.m_UnknownArray.size(); ++i )
    {
        data.m_UnknownArray[i] = aStream.ReadU32();
    }

    ReadCond( aStream, aVer, data.m_Unknown6 );

    return block;
}


static std::unique_ptr<BLOCK_BASE> ParseBlock_0x0D_PAD( FILE_STREAM& aStream, FMT_VER aVer )
{
    auto block = std::make_unique<BLOCK<BLK_0x0D_PAD>>( 0x0D, aStream.Position() );

    auto& data = block->GetData();

    aStream.Skip( 3 );

    data.m_Key = aStream.ReadU32();
    data.m_NameStrId = aStream.ReadU32();
    data.m_Next = aStream.ReadU32();

    ReadCond( aStream, aVer, data.m_Unknown1 );

    data.m_CoordsX = aStream.ReadS32();
    data.m_CoordsY = aStream.ReadS32();

    data.m_PadStack = aStream.ReadU32();
    data.m_Unknown2 = aStream.ReadU32();

    ReadCond( aStream, aVer, data.m_Unknown3 );

    data.m_Flags = aStream.ReadU32();
    data.m_Rotation = aStream.ReadU32();

    return block;
}


static std::unique_ptr<BLOCK_BASE> ParseBlock_0x0E( FILE_STREAM& aStream, FMT_VER aVer )
{
    auto block = std::make_unique<BLOCK<BLK_0x0E_SHAPE_SEG>>( 0x0E, aStream.Position() );

    auto& data = block->GetData();

    data.m_T = aStream.ReadU8();
    data.m_T2 = aStream.ReadU16();
    data.m_Key = aStream.ReadU32();
    data.m_Next = aStream.ReadU32();
    data.m_FpPtr = aStream.ReadU32();

    data.m_Unknown1 = aStream.ReadU32();
    data.m_Unknown2 = aStream.ReadU32();
    data.m_Unknown3 = aStream.ReadU32();

    ReadCond( aStream, aVer, data.m_Unknown4 );
    ReadCond( aStream, aVer, data.m_Unknown5 );

    for( size_t i = 0; i < data.m_Coords.size(); ++i )
    {
        data.m_Coords[i] = aStream.ReadS32();
    }

    for( size_t i = 0; i < data.m_UnknownArr.size(); ++i )
    {
        data.m_UnknownArr[i] = aStream.ReadU32();
    }

    return block;
}


static std::unique_ptr<BLOCK_BASE> ParseBlock_0x0F( FILE_STREAM& stream, FMT_VER aVer )
{
    auto block = std::make_unique<BLOCK<BLK_0x0F_FUNCTION_SLOT>>( 0x0F, stream.Position() );

    auto& data = block->GetData();

    stream.Skip( 3 );

    data.m_Key = stream.ReadU32();
    data.m_SlotName = stream.ReadU32();
    stream.ReadBytes( data.m_CompDeviceType.data(), data.m_CompDeviceType.size() );

    data.m_Ptr0x06 = stream.ReadU32();
    data.m_Ptr0x11 = stream.ReadU32();
    data.m_Unknown1 = stream.ReadU32();

    ReadCond( stream, aVer, data.m_Unknown2 );
    ReadCond( stream, aVer, data.m_Unknown3 );

    return block;
}


static std::unique_ptr<BLOCK_BASE> ParseBlock_0x10( FILE_STREAM& stream, FMT_VER aVer )
{
    auto block = std::make_unique<BLOCK<BLK_0x10_FUNCTION_INST>>( 0x10, stream.Position() );

    auto& data = block->GetData();

    stream.Skip( 3 );

    data.m_Key = stream.ReadU32();

    ReadCond( stream, aVer, data.m_Unknown1 );
    data.m_ComponentInstPtr = stream.ReadU32();
    ReadCond( stream, aVer, data.m_Unknown2 );
    data.m_PtrX12 = stream.ReadU32();
    data.m_Unknown3 = stream.ReadU32();
    data.m_FunctionName = stream.ReadU32();
    data.m_Slots = stream.ReadU32();
    data.m_Fields = stream.ReadU32();

    return block;
}


static std::unique_ptr<BLOCK_BASE> ParseBlock_0x11( FILE_STREAM& aStream, FMT_VER aVer )
{
    auto block = std::make_unique<BLOCK<BLK_0x11_PIN_NAME>>( 0x11, aStream.Position() );

    auto& data = block->GetData();

    data.m_Type = aStream.ReadU8();
    data.m_R = aStream.ReadU16();
    data.m_Key = aStream.ReadU32();
    data.m_PinNameStrPtr = aStream.ReadU32();
    data.m_Next = aStream.ReadU32();
    data.m_PinNumberPtr = aStream.ReadU32();
    data.m_Unknown1 = aStream.ReadU32();

    ReadCond( aStream, aVer, data.m_Unknown2 );

    return block;
}


static std::unique_ptr<BLOCK_BASE> ParseBlock_0x12( FILE_STREAM& aStream, FMT_VER aVer )
{
    auto block = std::make_unique<BLOCK<BLK_0x12_XREF>>( 0x12, aStream.Position() );

    auto& data = block->GetData();

    data.m_Type = aStream.ReadU8();
    data.m_R = aStream.ReadU16();
    data.m_Key = aStream.ReadU32();
    data.m_Ptr1 = aStream.ReadU32();
    data.m_Ptr2 = aStream.ReadU32();
    data.m_Ptr3 = aStream.ReadU32();
    data.m_Unknown1 = aStream.ReadU32();

    ReadCond( aStream, aVer, data.m_Unknown2 );
    ReadCond( aStream, aVer, data.m_Unknown3 );

    return block;
}


static std::unique_ptr<BLOCK_BASE> ParseBlock_0x14( FILE_STREAM& aStream, FMT_VER aVer )
{
    auto block = std::make_unique<BLOCK<BLK_0x14_GRAPHIC>>( 0x14, aStream.Position() );

    auto& data = block->GetData();

    data.m_Type = aStream.ReadU8();
    data.m_Layer = ParseLayerInfo( aStream );
    data.m_Key = aStream.ReadU32();
    data.m_Next = aStream.ReadU32();
    data.m_Parent = aStream.ReadU32();
    data.m_Flags = aStream.ReadU32();

    ReadCond( aStream, aVer, data.m_Unknown2 );

    data.m_SegmentPtr = aStream.ReadU32();
    data.m_Ptr0x03 = aStream.ReadU32();
    data.m_Ptr0x26 = aStream.ReadU32();

    return block;
}


static std::unique_ptr<BLOCK_BASE> ParseBlock_0x15_16_17_SEGMENT( FILE_STREAM& aStream, FMT_VER aVer, uint8_t aType )
{
    auto block = std::make_unique<BLOCK<BLK_0x15_16_17_SEGMENT>>( aType, aStream.Position() );

    auto& data = block->GetData();

    aStream.Skip( 3 );

    data.m_Key = aStream.ReadU32();
    data.m_Next = aStream.ReadU32();
    data.m_Parent = aStream.ReadU32();
    data.m_Flags = aStream.ReadU32();

    ReadCond( aStream, aVer, data.m_Unknown2 );

    data.m_Width = aStream.ReadU32();

    data.m_StartX = aStream.ReadS32();
    data.m_StartY = aStream.ReadS32();
    data.m_EndX = aStream.ReadS32();
    data.m_EndY = aStream.ReadS32();

    return block;
}


static std::unique_ptr<BLOCK_BASE> ParseBlock_0x1B_NET( FILE_STREAM& stream, FMT_VER aVer )
{
    auto block = std::make_unique<BLOCK<BLK_0x1B_NET>>( 0x1B, stream.Position() );

    auto& data = block->GetData();

    stream.Skip( 3 );

    data.m_Key = stream.ReadU32();
    data.m_Next = stream.ReadU32();
    data.m_NetName = stream.ReadU32();
    data.m_Unknown1 = stream.ReadU32();

    ReadCond( stream, aVer, data.m_Unknown2 );

    data.m_Type = stream.ReadU32();
    data.m_Assignment = stream.ReadU32();
    data.m_Ratline = stream.ReadU32();
    data.m_FieldsPtr = stream.ReadU32();
    data.m_MatchGroupPtr = stream.ReadU32();
    data.m_ModelPtr = stream.ReadU32();
    data.m_UnknownPtr4 = stream.ReadU32();
    data.m_UnknownPtr5 = stream.ReadU32();
    data.m_UnknownPtr6 = stream.ReadU32();

    return block;
}


static std::unique_ptr<BLOCK_BASE> ParseBlock_0x1C_PADSTACK( FILE_STREAM& aStream, FMT_VER aVer )
{
    auto block = std::make_unique<BLOCK<BLK_0x1C_PADSTACK>>( 0x1C, aStream.Position() );

    auto& data = block->GetData();

    data.m_UnknownByte1 = aStream.ReadU8();
    data.m_N = aStream.ReadU8();
    data.m_UnknownByte2 = aStream.ReadU8();

    data.m_Key = aStream.ReadU32();
    data.m_Next = aStream.ReadU32();

    data.m_PadStr = aStream.ReadU32();
    data.m_Drill = aStream.ReadU32();
    data.m_Unknown2 = aStream.ReadU32();
    data.m_PadPath = aStream.ReadU32();

    ReadCond( aStream, aVer, data.m_Unknown3 );
    ReadCond( aStream, aVer, data.m_Unknown4 );
    ReadCond( aStream, aVer, data.m_Unknown5 );
    ReadCond( aStream, aVer, data.m_Unknown6 );

    {
        const uint8_t t = aStream.ReadU8();

        // clang-format off
        switch( (t & 0xF0) >> 4 )
        {
        case 0x00:
            data.m_Type = PAD_TYPE::THROUGH_VIA;
            break;
        case 0x01:
            data.m_Type = PAD_TYPE::VIA;
            break;
        case 0x02:
        case 0x0a: // Unclear what the difference is
            data.m_Type = PAD_TYPE::SMD_PIN;
            break;
        case 0x03:
            data.m_Type = PAD_TYPE::SLOT;
            break;
        case 0x08:
            data.m_Type = PAD_TYPE::NPTH;
            break;
        default:
            THROW_IO_ERROR( wxString::Format( "Unknown padstack type 0x%x", t ) );
            break;
        }
        // clang-format on
        data.m_A = ( t & 0x0F );
    }

    data.m_B = aStream.ReadU8();
    data.m_C = aStream.ReadU8();
    data.m_D = aStream.ReadU8();

    ReadCond( aStream, aVer, data.m_Unknown7 );
    ReadCond( aStream, aVer, data.m_Unknown8 );
    ReadCond( aStream, aVer, data.m_Unknown9 );

    ReadCond( aStream, aVer, data.m_Unknown10 );
    data.m_LayerCount = aStream.ReadU16();

    if( data.m_LayerCount > 256 )
    {
        THROW_IO_ERROR( wxString::Format( "Padstack layer count %u exceeds maximum at offset %#010zx",
                                          data.m_LayerCount, aStream.Position() ) );
    }

    ReadCond( aStream, aVer, data.m_Unknown11 );

    ReadArrayU32( aStream, data.m_DrillArr );

    ReadCond( aStream, aVer, data.m_SlotAndUnknownArr );
    ReadCond( aStream, aVer, data.m_UnknownArr8_2 );

    // V180 has 8 extra uint32s between the fixed arrays and the component table
    ReadCond( aStream, aVer, data.m_V180Trailer );

    // Work out how many fixed slots we have, and how many per-layer slots
    data.m_NumFixedCompEntries = aVer < FMT_VER::V_172 ? 10 : 21;
    data.m_NumCompsPerLayer = aVer < FMT_VER::V_172 ? 3 : 4;

    const size_t nComps = data.m_NumFixedCompEntries + ( data.m_LayerCount * data.m_NumCompsPerLayer );

    data.m_Components.reserve( nComps );
    for( size_t i = 0; i < nComps; ++i )
    {
        PADSTACK_COMPONENT& comp = data.m_Components.emplace_back();

        comp.m_Type = aStream.ReadU8();

        comp.m_UnknownByte1 = aStream.ReadU8();
        comp.m_UnknownByte2 = aStream.ReadU8();
        comp.m_UnknownByte3 = aStream.ReadU8();

        ReadCond( aStream, aVer, comp.m_Unknown1 );

        comp.m_W = aStream.ReadS32();
        comp.m_H = aStream.ReadS32();

        ReadCond( aStream, aVer, comp.m_Z1 );

        comp.m_X3 = aStream.ReadS32();
        comp.m_X4 = aStream.ReadS32();

        ReadCond( aStream, aVer, comp.m_Z );

        comp.m_StrPtr = aStream.ReadU32();

        // The last component has a different size only in < 17.2
        if( !( aVer < FMT_VER::V_172 && i == nComps - 1 ) )
        {
            comp.m_Z2 = aStream.ReadU32();
        }
    }

    {
        const size_t nElems = data.m_N * ( ( aVer < FMT_VER::V_172 ) ? 8 : 10 );

        data.m_UnknownArrN.reserve( nElems );
        for( size_t i = 0; i < nElems; ++i )
        {
            data.m_UnknownArrN.push_back( aStream.ReadU32() );
        }
    }

    return block;
}


static std::unique_ptr<BLOCK_BASE> ParseBlock_0x1D( FILE_STREAM& aStream, FMT_VER aVer )
{
    auto block = std::make_unique<BLOCK<BLK_0x1D_CONSTRAINT_SET>>( 0x1D, aStream.Position() );

    auto& data = block->GetData();

    aStream.Skip( 3 );

    data.m_Key = aStream.ReadU32();
    data.m_Next = aStream.ReadU32();
    data.m_NameStrKey = aStream.ReadU32();
    data.m_FieldPtr = aStream.ReadU32();

    data.m_SizeA = aStream.ReadU16();
    data.m_SizeB = aStream.ReadU16();

    data.m_DataB.resize( data.m_SizeB );

    for( auto& item : data.m_DataB )
        aStream.ReadBytes( item.data(), item.size() );

    data.m_DataA.resize( data.m_SizeA );

    for( auto& item : data.m_DataA )
        aStream.ReadBytes( item.data(), item.size() );

    ReadCond( aStream, aVer, data.m_Unknown4 );

    return block;
}


static std::unique_ptr<BLOCK_BASE> ParseBlock_0x1E( FILE_STREAM& aStream, FMT_VER aVer )
{
    auto block = std::make_unique<BLOCK<BLK_0x1E_SI_MODEL>>( 0x1E, aStream.Position() );

    auto& data = block->GetData();

    data.m_Type = aStream.ReadU8();
    data.m_T2 = aStream.ReadU16();
    data.m_Key = aStream.ReadU32();
    data.m_Next = aStream.ReadU32();

    ReadCond( aStream, aVer, data.m_Unknown2 );
    ReadCond( aStream, aVer, data.m_Unknown3 );

    data.m_StrPtr = aStream.ReadU32();
    data.m_Size = aStream.ReadU32();

    data.m_String = aStream.ReadStringFixed( data.m_Size, true );

    ReadCond( aStream, aVer, data.m_Unknown4 );

    return block;
}


static std::unique_ptr<BLOCK_BASE> ParseBlock_0x1F( FILE_STREAM& aStream, FMT_VER aVer )
{
    auto block = std::make_unique<BLOCK<BLK_0x1F_PADSTACK_DIM>>( 0x1F, aStream.Position() );

    auto& data = block->GetData();

    aStream.Skip( 3 );

    data.m_Key = aStream.ReadU32();

    data.m_Next = aStream.ReadU32();
    data.m_Unknown2 = aStream.ReadU32();
    data.m_Unknown3 = aStream.ReadU32();
    data.m_Unknown4 = aStream.ReadU32();
    data.m_Unknown5 = aStream.ReadU16();

    data.m_Size = aStream.ReadU16();

    size_t substructSize = 0;
    if( aVer >= FMT_VER::V_175 )
        substructSize = data.m_Size * 384 + 8;
    else if( aVer >= FMT_VER::V_172 )
        substructSize = data.m_Size * 280 + 8;
    else if( aVer >= FMT_VER::V_162 )
        substructSize = data.m_Size * 280 + 4;
    else
        substructSize = data.m_Size * 240 + 4;

    data.m_Substruct.resize( substructSize );
    aStream.ReadBytes( data.m_Substruct.data(), substructSize );

    return block;
}


static std::unique_ptr<BLOCK_BASE> ParseBlock_0x20( FILE_STREAM& aStream, FMT_VER aVer )
{
    auto block = std::make_unique<BLOCK<BLK_0x20_UNKNOWN>>( 0x20, aStream.Position() );

    auto& data = block->GetData();

    data.m_Type = aStream.ReadU8();
    data.m_R = aStream.ReadU16();
    data.m_Key = aStream.ReadU32();
    data.m_Next = aStream.ReadU32();

    ReadArrayU32( aStream, data.m_UnknownArray1 );
    ReadCond( aStream, aVer, data.m_UnknownArray2 );

    return block;
}


static std::unique_ptr<BLOCK_BASE> ParseBlock_0x21( FILE_STREAM& aStream, FMT_VER aVer )
{
    auto block = std::make_unique<BLOCK<BLK_0x21_BLOB>>( 0x21, aStream.Position() );

    auto& data = block->GetData();

    data.m_Type = aStream.ReadU8();
    data.m_R = aStream.ReadU16();

    data.m_Size = aStream.ReadU32();

    if( data.m_Size < 12 )
    {
        THROW_IO_ERROR( wxString::Format( "Block 0x21 size %u too small (minimum 12) at offset %#010zx",
                                          data.m_Size, aStream.Position() ) );
    }

    data.m_Key = aStream.ReadU32();

    const size_t nBytes = data.m_Size - 12;
    data.m_Data.resize( nBytes );
    aStream.ReadBytes( data.m_Data.data(), nBytes );

    return block;
}


static std::unique_ptr<BLOCK_BASE> ParseBlock_0x22( FILE_STREAM& aStream, FMT_VER aVer )
{
    auto block = std::make_unique<BLOCK<BLK_0x22_UNKNOWN>>( 0x22, aStream.Position() );

    auto& data = block->GetData();

    data.m_Type = aStream.ReadU8();
    data.m_T2 = aStream.ReadU16();
    data.m_Key = aStream.ReadU32();

    ReadCond( aStream, aVer, data.m_Unknown1 );

    ReadArrayU32( aStream, data.m_UnknownArray );

    return block;
}


static std::unique_ptr<BLOCK_BASE> ParseBlock_0x23_RATLINE( FILE_STREAM& aStream, FMT_VER aVer )
{
    auto block = std::make_unique<BLOCK<BLK_0x23_RATLINE>>( 0x23, aStream.Position() );

    auto& data = block->GetData();

    data.m_Type = aStream.ReadU8();
    data.m_Layer = ParseLayerInfo( aStream );
    data.m_Key = aStream.ReadU32();
    data.m_Next = aStream.ReadU32();

    ReadArrayU32( aStream, data.m_Flags );

    data.m_Ptr1 = aStream.ReadU32();
    data.m_Ptr2 = aStream.ReadU32();
    data.m_Ptr3 = aStream.ReadU32();

    for( size_t i = 0; i < data.m_Coords.size(); ++i )
    {
        data.m_Coords[i] = aStream.ReadS32();
    }

    ReadArrayU32( aStream, data.m_Unknown1 );

    ReadCond( aStream, aVer, data.m_Unknown2 );
    ReadCond( aStream, aVer, data.m_Unknown3 );

    return block;
}


static std::unique_ptr<BLOCK_BASE> ParseBlock_0x24_RECT( FILE_STREAM& aStream, FMT_VER aVer )
{
    auto block = std::make_unique<BLOCK<BLK_0x24_RECT>>( 0x24, aStream.Position() );

    auto& data = block->GetData();

    data.m_Type = aStream.ReadU8();
    data.m_Layer = ParseLayerInfo( aStream );
    data.m_Key = aStream.ReadU32();
    data.m_Next = aStream.ReadU32();
    data.m_Parent = aStream.ReadU32();
    data.m_Unknown1 = aStream.ReadU32();

    ReadCond( aStream, aVer, data.m_Unknown2 );

    for( size_t i = 0; i < data.m_Coords.size(); ++i )
    {
        data.m_Coords[i] = aStream.ReadS32();
    }

    data.m_Ptr2 = aStream.ReadU32();

    data.m_Unknown3 = aStream.ReadU32();
    data.m_Unknown4 = aStream.ReadU32();
    data.m_Unknown5 = aStream.ReadU32();

    return block;
}


static std::unique_ptr<BLOCK_BASE> ParseBlock_0x26( FILE_STREAM& aStream, FMT_VER aVer )
{
    auto block = std::make_unique<BLOCK<BLK_0x26_MATCH_GROUP>>( 0x26, aStream.Position() );

    auto& data = block->GetData();

    data.m_Type = aStream.ReadU8();
    data.m_R = aStream.ReadU16();
    data.m_Key = aStream.ReadU32();
    data.m_MemberPtr = aStream.ReadU32();

    ReadCond( aStream, aVer, data.m_Unknown1 );

    data.m_GroupPtr = aStream.ReadU32();
    data.m_ConstPtr = aStream.ReadU32();

    ReadCond( aStream, aVer, data.m_Unknown2 );

    return block;
}


static std::unique_ptr<BLOCK_BASE> ParseBlock_0x27( FILE_STREAM& aStream, FMT_VER aVer, size_t aEndOff )
{
    auto block = std::make_unique<BLOCK<BLK_0x27_CSTRMGR_XREF>>( 0x27, aStream.Position() );

    auto& data = block->GetData();

    const size_t totalBytes = aEndOff - 1 - aStream.Position();

    // The blob starts with 3 bytes of padding, then uint32 LE values
    constexpr size_t kPadding = 3;

    if( totalBytes <= kPadding )
    {
        aStream.Skip( totalBytes );
        return block;
    }

    aStream.Skip( kPadding );

    const size_t payloadBytes = totalBytes - kPadding;
    const size_t numValues = payloadBytes / 4;
    const size_t remainder = payloadBytes % 4;

    data.m_Refs.resize( numValues );

    for( size_t i = 0; i < numValues; i++ )
        data.m_Refs[i] = aStream.ReadU32();

    if( remainder > 0 )
        aStream.Skip( remainder );

    return block;
}


static std::unique_ptr<BLOCK_BASE> ParseBlock_0x28_SHAPE( FILE_STREAM& aStream, FMT_VER aVer )
{
    auto block = std::make_unique<BLOCK<BLK_0x28_SHAPE>>( 0x28, aStream.Position() );

    auto& data = block->GetData();

    data.m_Type = aStream.ReadU8();
    data.m_Layer = ParseLayerInfo( aStream );
    data.m_Key = aStream.ReadU32();
    data.m_Next = aStream.ReadU32();
    data.m_Ptr1 = aStream.ReadU32();
    data.m_Unknown1 = aStream.ReadU32();

    ReadCond( aStream, aVer, data.m_Unknown2 );
    ReadCond( aStream, aVer, data.m_Unknown3 );

    data.m_Ptr2 = aStream.ReadU32();
    data.m_Ptr3 = aStream.ReadU32();
    data.m_Ptr4 = aStream.ReadU32();
    data.m_FirstSegmentPtr = aStream.ReadU32();
    data.m_Unknown4 = aStream.ReadU32();
    data.m_Unknown5 = aStream.ReadU32();

    ReadCond( aStream, aVer, data.m_Ptr7 );

    data.m_Ptr6 = aStream.ReadU32();

    ReadCond( aStream, aVer, data.m_Ptr7_16x );

    for( size_t i = 0; i < data.m_Coords.size(); ++i )
    {
        data.m_Coords[i] = aStream.ReadS32();
    }

    return block;
}


static std::unique_ptr<BLOCK_BASE> ParseBlock_0x29_PIN( FILE_STREAM& aStream, FMT_VER aVer )
{
    auto block = std::make_unique<BLOCK<BLK_0x29_PIN>>( 0x29, aStream.Position() );

    auto& data = block->GetData();

    data.m_Type = aStream.ReadU8();
    data.m_T = aStream.ReadU16();
    data.m_Key = aStream.ReadU32();

    data.m_Ptr1 = aStream.ReadU32();
    data.m_Ptr2 = aStream.ReadU32();

    data.m_Null = aStream.ReadU32();

    data.m_Ptr3 = aStream.ReadU32();

    data.m_Coord1 = aStream.ReadS32();
    data.m_Coord2 = aStream.ReadS32();

    data.m_PtrPadstack = aStream.ReadU32();

    data.m_Unknown1 = aStream.ReadU32();

    data.m_PtrX30 = aStream.ReadU32();

    data.m_Unknown2 = aStream.ReadU32();
    data.m_Unknown3 = aStream.ReadU32();
    data.m_Unknown4 = aStream.ReadU32();

    return block;
}


static std::unique_ptr<BLOCK_BASE> ParseBlock_0x2A( FILE_STREAM& aStream, FMT_VER aVer )
{
    auto block = std::make_unique<BLOCK<BLK_0x2A_LAYER_LIST>>( 0x2A, aStream.Position() );

    auto& data = block->GetData();

    aStream.Skip( 1 );

    data.m_NumEntries = aStream.ReadU16();

    ReadCond( aStream, aVer, data.m_Unknown );

    if( data.m_NonRefEntries.exists( aVer ) )
    {
        data.m_NonRefEntries = std::vector<BLK_0x2A_LAYER_LIST::NONREF_ENTRY>();
        data.m_NonRefEntries->reserve( data.m_NumEntries );
        for( size_t i = 0; i < data.m_NumEntries; ++i )
        {
            BLK_0x2A_LAYER_LIST::NONREF_ENTRY& entry = data.m_NonRefEntries->emplace_back();

            entry.m_Name = aStream.ReadStringFixed( 36, true );
        }
    }
    else
    {
        data.m_RefEntries = std::vector<BLK_0x2A_LAYER_LIST::REF_ENTRY>();
        data.m_RefEntries->reserve( data.m_NumEntries );
        for( size_t i = 0; i < data.m_NumEntries; ++i )
        {
            BLK_0x2A_LAYER_LIST::REF_ENTRY& entry = data.m_RefEntries->emplace_back();

            entry.mLayerNameId = aStream.ReadU32();
            entry.m_Properties = aStream.ReadU32();
            entry.m_Unknown = aStream.ReadU32();
        }
    }

    data.m_Key = aStream.ReadU32();

    return block;
}


static std::unique_ptr<BLOCK_BASE> ParseBlock_0x2B( FILE_STREAM& stream, FMT_VER aVer )
{
    auto block = std::make_unique<BLOCK<BLK_0x2B_FOOTPRINT_DEF>>( 0x2B, stream.Position() );

    auto& data = block->GetData();

    stream.Skip( 3 );

    data.m_Key = stream.ReadU32();
    data.m_FpStrRef = stream.ReadU32();
    data.m_Unknown1 = stream.ReadU32();
    ReadArrayU32( stream, data.m_Coords );
    data.m_Next = stream.ReadU32();
    data.m_FirstInstPtr = stream.ReadU32();
    data.m_UnknownPtr3 = stream.ReadU32();
    data.m_UnknownPtr4 = stream.ReadU32();
    data.m_UnknownPtr5 = stream.ReadU32();
    data.m_SymLibPathPtr = stream.ReadU32();
    data.m_UnknownPtr6 = stream.ReadU32();
    data.m_UnknownPtr7 = stream.ReadU32();
    data.m_UnknownPtr8 = stream.ReadU32();

    ReadCond( stream, aVer, data.m_Unknown2 );
    ReadCond( stream, aVer, data.m_Unknown3 );

    return block;
}


static std::unique_ptr<BLOCK_BASE> ParseBlock_0x2C_TABLE( FILE_STREAM& aStream, FMT_VER aVer )
{
    auto block = std::make_unique<BLOCK<BLK_0x2C_TABLE>>( 0x2C, aStream.Position() );

    auto& data = block->GetData();

    data.m_Type = aStream.ReadU8();
    data.m_T2 = aStream.ReadU16();
    data.m_Key = aStream.ReadU32();
    data.m_Next = aStream.ReadU32();

    ReadCond( aStream, aVer, data.m_Unknown1 );
    ReadCond( aStream, aVer, data.m_Unknown2 );
    ReadCond( aStream, aVer, data.m_Unknown3 );

    data.m_StringPtr = aStream.ReadU32();

    ReadCond( aStream, aVer, data.m_Unknown4 );

    data.m_Ptr1 = aStream.ReadU32();
    data.m_Ptr2 = aStream.ReadU32();
    data.m_Ptr3 = aStream.ReadU32();

    data.m_Flags = aStream.ReadU32();

    return block;
}


static std::unique_ptr<BLOCK_BASE> ParseBlock_0x2D( FILE_STREAM& stream, FMT_VER aVer )
{
    auto block = std::make_unique<BLOCK<BLK_0x2D_FOOTPRINT_INST>>( 0x2D, stream.Position() );

    auto& data = block->GetData();

    data.m_UnknownByte1 = stream.ReadU8();
    data.m_Layer = stream.ReadU8();
    data.m_UnknownByte2 = stream.ReadU8();

    data.m_Key = stream.ReadU32();
    data.m_Next = stream.ReadU32();

    ReadCond( stream, aVer, data.m_Unknown1 );

    ReadCond( stream, aVer, data.m_InstRef16x );

    data.m_Unknown2 = stream.ReadU16();
    data.m_Unknown3 = stream.ReadU16();

    ReadCond( stream, aVer, data.m_Unknown4 );

    data.m_Flags = stream.ReadU32();

    data.m_Rotation = stream.ReadU32();
    data.m_CoordX = stream.ReadS32();
    data.m_CoordY = stream.ReadS32();

    ReadCond( stream, aVer, data.m_InstRef );

    data.m_GraphicPtr = stream.ReadU32();
    data.m_FirstPadPtr = stream.ReadU32();
    data.m_TextPtr = stream.ReadU32();
    ReadArrayU32( stream, data.m_UnknownPtrs1 );

    // ReadCond( stream, aVer, data.m_groupAssignmentPtr );

    return block;
}


static std::unique_ptr<BLOCK_BASE> ParseBlock_0x2E( FILE_STREAM& aStream, FMT_VER aVer )
{
    auto block = std::make_unique<BLOCK<BLK_0x2E_CONNECTION>>( 0x2E, aStream.Position() );

    auto& data = block->GetData();

    data.m_Type = aStream.ReadU8();
    data.m_T2 = aStream.ReadU16();
    data.m_Key = aStream.ReadU32();
    data.m_Next = aStream.ReadU32();
    data.m_NetAssignment = aStream.ReadU32();
    data.m_Unknown1 = aStream.ReadU32();
    data.m_CoordX = aStream.ReadU32();
    data.m_CoordY = aStream.ReadU32();
    data.m_Connection = aStream.ReadU32();
    data.m_Unknown2 = aStream.ReadU32();

    ReadCond( aStream, aVer, data.m_Unknown3 );

    return block;
}


static std::unique_ptr<BLOCK_BASE> ParseBlock_0x2F( FILE_STREAM& aStream, FMT_VER aVer )
{
    auto block = std::make_unique<BLOCK<BLK_0x2F_UNKNOWN>>( 0x2F, aStream.Position() );

    auto& data = block->GetData();

    data.m_Type = aStream.ReadU8();
    data.m_T2 = aStream.ReadU16();
    data.m_Key = aStream.ReadU32();

    ReadArrayU32( aStream, data.m_UnknownArray );

    return block;
}


static BLK_0x30_STR_WRAPPER::TEXT_PROPERTIES ParseTextProps( FILE_STREAM& aStream )
{
    BLK_0x30_STR_WRAPPER::TEXT_PROPERTIES props;

    props.m_Key = aStream.ReadU8();
    props.m_Flags = aStream.ReadU8();

    uint8_t alignment = aStream.ReadU8();
    switch( alignment )
    {
    case 0x01: props.m_Alignment = BLK_0x30_STR_WRAPPER::TEXT_ALIGNMENT::LEFT; break;
    case 0x02: props.m_Alignment = BLK_0x30_STR_WRAPPER::TEXT_ALIGNMENT::RIGHT; break;
    case 0x03: props.m_Alignment = BLK_0x30_STR_WRAPPER::TEXT_ALIGNMENT::CENTER; break;
    case 0x1a:
    default:
        props.m_Alignment = BLK_0x30_STR_WRAPPER::TEXT_ALIGNMENT::UNKNOWN;
        // THROW_IO_ERROR( wxString::Format( "Unknown text alignment value: %#02x", alignment ) );
    }

    uint8_t reversal = aStream.ReadU8();
    switch( reversal )
    {
    case 0x00: props.m_Reversal = BLK_0x30_STR_WRAPPER::TEXT_REVERSAL::STRAIGHT; break;
    case 0x01: props.m_Reversal = BLK_0x30_STR_WRAPPER::TEXT_REVERSAL::REVERSED; break;
    case 0x0c:
    default:
        props.m_Reversal = BLK_0x30_STR_WRAPPER::TEXT_REVERSAL::UNKNOWN;
        //THROW_IO_ERROR( wxString::Format( "Unknown text reversal value: %#02x", reversal ) );
    }
    return props;
}


static std::unique_ptr<BLOCK_BASE> ParseBlock_0x30_STR_WRAPPER( FILE_STREAM& aStream, FMT_VER aVer )
{
    auto block = std::make_unique<BLOCK<BLK_0x30_STR_WRAPPER>>( 0x30, aStream.Position() );

    auto& data = block->GetData();

    data.m_Type = aStream.ReadU8();
    data.m_Layer = ParseLayerInfo( aStream );
    data.m_Key = aStream.ReadU32();
    data.m_Next = aStream.ReadU32();

    ReadCond( aStream, aVer, data.m_Unknown1 );
    ReadCond( aStream, aVer, data.m_Unknown2 );

    if( data.m_Font.exists( aVer ) )
    {
        data.m_Font = ParseTextProps( aStream );
    }

    ReadCond( aStream, aVer, data.m_Ptr1 );
    ReadCond( aStream, aVer, data.m_Unknown3 );

    data.m_StrGraphicPtr = aStream.ReadU32();
    data.m_Unknown4 = aStream.ReadU32();

    if( data.m_Font16x.exists( aVer ) )
    {
        data.m_Font16x = ParseTextProps( aStream );
    }

    ReadCond( aStream, aVer, data.m_Ptr2 );

    data.m_CoordsX = aStream.ReadU32();
    data.m_CoordsY = aStream.ReadU32();

    data.m_Unknown5 = aStream.ReadU32();
    data.m_Rotation = aStream.ReadU32();

    ReadCond( aStream, aVer, data.m_Ptr3_16x );

    return block;
}


static std::unique_ptr<BLOCK_BASE> ParseBlock_0x31_SGRAPHIC( FILE_STREAM& aStream, FMT_VER aVer )
{
    auto block = std::make_unique<BLOCK<BLK_0x31_SGRAPHIC>>( 0x31, aStream.Position() );

    auto& data = block->GetData();

    data.m_T = aStream.ReadU8();

    {
        const uint16_t layer = aStream.ReadU16();

        switch( layer )
        {
        case 0xF001: data.m_Layer = BLK_0x31_SGRAPHIC::STRING_LAYER::BOT_TEXT; break;
        case 0xF101: data.m_Layer = BLK_0x31_SGRAPHIC::STRING_LAYER::TOP_TEXT; break;
        case 0xF609: data.m_Layer = BLK_0x31_SGRAPHIC::STRING_LAYER::BOT_PIN; break;
        case 0xF709: data.m_Layer = BLK_0x31_SGRAPHIC::STRING_LAYER::TOP_PIN; break;
        case 0xF801: data.m_Layer = BLK_0x31_SGRAPHIC::STRING_LAYER::TOP_PIN_LABEL; break;
        case 0xFA0D: data.m_Layer = BLK_0x31_SGRAPHIC::STRING_LAYER::BOT_REFDES; break;
        case 0xFB0D: data.m_Layer = BLK_0x31_SGRAPHIC::STRING_LAYER::TOP_REFDES; break;
        default: data.m_Layer = BLK_0x31_SGRAPHIC::STRING_LAYER::UNKNOWN; break;
        }
    }

    data.m_Key = aStream.ReadU32();
    data.m_StrGraphicWrapperPtr = aStream.ReadU32();

    data.m_CoordsX = aStream.ReadU32();
    data.m_CoordsY = aStream.ReadU32();

    data.m_Unknown = aStream.ReadU16();
    data.m_Len = aStream.ReadU16();

    ReadCond( aStream, aVer, data.m_Un2 );

    data.m_Value = aStream.ReadStringFixed( data.m_Len, true );

    return block;
}


static std::unique_ptr<BLOCK_BASE> ParseBlock_0x32_PLACED_PAD( FILE_STREAM& aStream, FMT_VER aVer )
{
    auto block = std::make_unique<BLOCK<BLK_0x32_PLACED_PAD>>( 0x32, aStream.Position() );

    auto& data = block->GetData();

    data.m_Type = aStream.ReadU8();
    data.m_Layer = ParseLayerInfo( aStream );
    data.m_Key = aStream.ReadU32();
    data.m_Next = aStream.ReadU32();
    data.m_NetPtr = aStream.ReadU32();
    data.m_Flags = aStream.ReadU32();

    ReadCond( aStream, aVer, data.m_Prev );

    data.m_NextInFp = aStream.ReadU32();
    data.m_ParentFp = aStream.ReadU32();
    data.m_Track = aStream.ReadU32();
    data.m_PadPtr = aStream.ReadU32();
    data.m_Ptr6 = aStream.ReadU32();
    data.m_Ratline = aStream.ReadU32();
    data.m_PtrPinNumber = aStream.ReadU32();
    data.m_NextInCompInst = aStream.ReadU32();

    ReadCond( aStream, aVer, data.m_Unknown2 );

    data.m_NameText = aStream.ReadU32();
    data.m_Ptr11 = aStream.ReadU32();

    for( size_t i = 0; i < data.m_Coords.size(); ++i )
    {
        data.m_Coords[i] = aStream.ReadS32();
    }

    return block;
}


static std::unique_ptr<BLOCK_BASE> ParseBlock_0x33_VIA( FILE_STREAM& aStream, FMT_VER aVer )
{
    auto block = std::make_unique<BLOCK<BLK_0x33_VIA>>( 0x33, aStream.Position() );

    auto& data = block->GetData();

    aStream.Skip( 1 );

    data.m_LayerInfo = ParseLayerInfo( aStream );
    data.m_Key = aStream.ReadU32();
    data.m_Next = aStream.ReadU32();
    data.m_NetPtr = aStream.ReadU32();
    data.m_Unknown2 = aStream.ReadU32();

    ReadCond( aStream, aVer, data.m_Unknown3 );

    data.m_UnknownPtr1 = aStream.ReadU32();

    ReadCond( aStream, aVer, data.m_UnknownPtr2 );

    data.m_CoordsX = aStream.ReadS32();
    data.m_CoordsY = aStream.ReadS32();

    data.m_Connection = aStream.ReadU32();
    data.m_Padstack = aStream.ReadU32();
    data.m_UnknownPtr5 = aStream.ReadU32();
    data.m_UnknownPtr6 = aStream.ReadU32();

    data.m_Unknown4 = aStream.ReadU32();
    data.m_Unknown5 = aStream.ReadU32();

    for( size_t i = 0; i < data.m_BoundingBoxCoords.size(); ++i )
    {
        data.m_BoundingBoxCoords[i] = aStream.ReadS32();
    }

    return block;
}


static std::unique_ptr<BLOCK_BASE> ParseBlock_0x34_KEEPOUT( FILE_STREAM& aStream, FMT_VER aVer )
{
    auto block = std::make_unique<BLOCK<BLK_0x34_KEEPOUT>>( 0x34, aStream.Position() );

    auto& data = block->GetData();

    data.m_T = aStream.ReadU8();
    data.m_Layer = ParseLayerInfo( aStream );
    data.m_Key = aStream.ReadU32();
    data.m_Next = aStream.ReadU32();
    data.m_Ptr1 = aStream.ReadU32();

    ReadCond( aStream, aVer, data.m_Unknown1 );

    data.m_Flags = aStream.ReadU32();
    data.m_Ptr2 = aStream.ReadU32();
    data.m_Ptr3 = aStream.ReadU32();
    data.m_Unknown2 = aStream.ReadU32();

    return block;
}


static std::unique_ptr<BLOCK_BASE> ParseBlock_0x35( FILE_STREAM& aStream, FMT_VER aVer )
{
    auto block = std::make_unique<BLOCK<BLK_0x35_FILE_REF>>( 0x35, aStream.Position() );

    auto& data = block->GetData();

    data.m_T2 = aStream.ReadU8();
    data.m_T3 = aStream.ReadU16();
    aStream.ReadBytes( data.m_Content.data(), data.m_Content.size() );

    return block;
}


static std::unique_ptr<BLOCK_BASE> ParseBlock_0x36( FILE_STREAM& aStream, FMT_VER aVer )
{
    auto block = std::make_unique<BLOCK<BLK_0x36_DEF_TABLE>>( 0x36, aStream.Position() );

    auto& data = block->GetData();

    aStream.Skip( 1 );

    data.m_Code = aStream.ReadU16();
    data.m_Key = aStream.ReadU32();
    data.m_Next = aStream.ReadU32();

    ReadCond( aStream, aVer, data.m_Unknown1 );

    data.m_NumItems = aStream.ReadU32();
    data.m_Count = aStream.ReadU32();
    data.m_LastIdx = aStream.ReadU32();
    data.m_Unknown2 = aStream.ReadU32();

    ReadCond( aStream, aVer, data.m_Unknown3 );

    if( data.m_NumItems > 1000000 )
    {
        THROW_IO_ERROR( wxString::Format(
                "Block 0x36 item count %u exceeds limit at offset %#010zx",
                data.m_NumItems, aStream.Position() ) );
    }

    data.m_Items.reserve( data.m_NumItems );
    for( uint32_t i = 0; i < data.m_NumItems; ++i )
    {
        switch( data.m_Code )
        {
        case 0x02:
        {
            BLK_0x36_DEF_TABLE::X02 item;

            item.m_String = aStream.ReadStringFixed( 32, true );
            ReadArrayU32( aStream, item.m_Xs );
            ReadCond( aStream, aVer, item.m_Ys );
            ReadCond( aStream, aVer, item.m_Zs );

            data.m_Items.emplace_back( std::move( item ) );
            break;
        }
        case 0x03:
        {
            BLK_0x36_DEF_TABLE::X03 item;
            if( aVer >= FMT_VER::V_172 )
                item.m_Str = aStream.ReadStringFixed( 64, true );
            else
                item.m_Str16x = aStream.ReadStringFixed( 32, true );

            ReadCond( aStream, aVer, item.m_Unknown1 );

            data.m_Items.emplace_back( std::move( item ) );
            break;
        }
        case 0x05:
        {
            BLK_0x36_DEF_TABLE::X05 item;

            aStream.ReadBytes( item.m_Unknown.data(), item.m_Unknown.size() );

            data.m_Items.emplace_back( std::move( item ) );
            break;
        }
        case 0x06:
        {
            BLK_0x36_DEF_TABLE::X06 item;

            item.m_N = aStream.ReadU16();
            item.m_R = aStream.ReadU8();
            item.m_S = aStream.ReadU8();
            item.m_Unknown1 = aStream.ReadU32();

            ReadCond( aStream, aVer, item.m_Unknown2 );

            data.m_Items.emplace_back( std::move( item ) );
            break;
        }
        case 0x08:
        {
            BLK_0x36_DEF_TABLE::FontDef_X08 item;

            item.m_A = aStream.ReadU32();
            item.m_B = aStream.ReadU32();
            item.m_CharHeight = aStream.ReadU32();
            item.m_CharWidth = aStream.ReadU32();

            ReadCond( aStream, aVer, item.m_Unknown2 );
            ReadArrayU32( aStream, item.m_Xs );
            ReadCond( aStream, aVer, item.m_Ys );

            data.m_Items.emplace_back( std::move( item ) );
            break;
        }
        case 0x0B:
        {
            BLK_0x36_DEF_TABLE::X0B item;
            aStream.ReadBytes( item.m_Unknown.data(), item.m_Unknown.size() );
            data.m_Items.emplace_back( std::move( item ) );
            break;
        }
        case 0x0C:
        {
            BLK_0x36_DEF_TABLE::X0C item;
            aStream.ReadBytes( item.m_Unknown.data(), item.m_Unknown.size() );
            data.m_Items.emplace_back( std::move( item ) );
            break;
        }
        case 0x0D:
        {
            BLK_0x36_DEF_TABLE::X0D item;
            aStream.ReadBytes( item.m_Unknown.data(), item.m_Unknown.size() );
            data.m_Items.emplace_back( std::move( item ) );
            break;
        }
        case 0x0F:
        {
            BLK_0x36_DEF_TABLE::X0F item;
            item.m_Key = aStream.ReadU32();
            ReadArrayU32( aStream, item.m_Ptrs );
            item.m_Ptr2 = aStream.ReadU32();
            data.m_Items.emplace_back( std::move( item ) );
            break;
        }
        case 0x10:
        {
            BLK_0x36_DEF_TABLE::X10 item;
            aStream.ReadBytes( item.m_Unknown.data(), item.m_Unknown.size() );
            data.m_Items.emplace_back( std::move( item ) );
            break;
        }
        default: THROW_IO_ERROR( wxString::Format( "Unknown substruct type %#02x in block 0x36", data.m_Code ) );
        }
    }

    return block;
}


static std::unique_ptr<BLOCK_BASE> ParseBlock_0x37( FILE_STREAM& aStream, FMT_VER aVer )
{
    auto block = std::make_unique<BLOCK<BLK_0x37_PTR_ARRAY>>( 0x37, aStream.Position() );

    auto& data = block->GetData();

    data.m_T = aStream.ReadU8();
    data.m_T2 = aStream.ReadU16();
    data.m_Key = aStream.ReadU32();
    data.m_Ptr1 = aStream.ReadU32();
    data.m_Unknown1 = aStream.ReadU32();
    data.m_Capacity = aStream.ReadU32();
    data.m_Count = aStream.ReadU32();
    data.m_Unknown2 = aStream.ReadU32();

    ReadArrayU32( aStream, data.m_Ptrs );

    ReadCond( aStream, aVer, data.m_UnknownArr );

    return block;
}


static std::unique_ptr<BLOCK_BASE> ParseBlock_0x38_FILM( FILE_STREAM& aStream, FMT_VER aVer )
{
    auto block = std::make_unique<BLOCK<BLK_0x38_FILM>>( 0x38, aStream.Position() );

    auto& data = block->GetData();

    aStream.Skip( 3 );

    data.m_Key = aStream.ReadU32();
    data.m_Next = aStream.ReadU32();
    data.m_LayerList = aStream.ReadU32();

    if( data.m_FilmName.exists( aVer ) )
    {
        data.m_FilmName = aStream.ReadStringFixed( 20, true );
    }

    ReadCond( aStream, aVer, data.m_LayerNameStr );
    ReadCond( aStream, aVer, data.m_Unknown2 );

    for( size_t i = 0; i < data.m_UnknownArray1.size(); ++i )
    {
        data.m_UnknownArray1[i] = aStream.ReadU32();
    }

    ReadCond( aStream, aVer, data.m_Unknown3 );

    return block;
}


static std::unique_ptr<BLOCK_BASE> ParseBlock_0x39_FILM_LAYER_LIST( FILE_STREAM& aStream, FMT_VER aVer )
{
    auto block = std::make_unique<BLOCK<BLK_0x39_FILM_LAYER_LIST>>( 0x39, aStream.Position() );

    auto& data = block->GetData();

    aStream.Skip( 3 );

    data.m_Key = aStream.ReadU32();
    data.m_Parent = aStream.ReadU32();
    data.m_Head = aStream.ReadU32();

    for( size_t i = 0; i < data.m_X.size(); ++i )
    {
        data.m_X[i] = aStream.ReadU16();
    }

    return block;
}


static std::unique_ptr<BLOCK_BASE> ParseBlock_0x3A_FILM_LIST_NODE( FILE_STREAM& aStream, FMT_VER aVer )
{
    auto block = std::make_unique<BLOCK<BLK_0x3A_FILM_LIST_NODE>>( 0x3A, aStream.Position() );

    auto& data = block->GetData();

    aStream.Skip( 1 );

    data.m_Layer = ParseLayerInfo( aStream );
    data.m_Key = aStream.ReadU32();
    data.m_Next = aStream.ReadU32();
    data.m_Unknown = aStream.ReadU32();

    ReadCond( aStream, aVer, data.m_Unknown1 );

    return block;
}


static std::unique_ptr<BLOCK_BASE> ParseBlock_0x3B( FILE_STREAM& aStream, FMT_VER aVer )
{
    auto block = std::make_unique<BLOCK<BLK_0x3B_PROPERTY>>( 0x3B, aStream.Position() );

    auto& data = block->GetData();

    data.m_T = aStream.ReadU8();
    data.m_SubType = aStream.ReadU16();
    data.m_Len = aStream.ReadU32();

    data.m_Name = aStream.ReadStringFixed( 128, true );
    data.m_Type = aStream.ReadStringFixed( 32, true );

    data.m_Unknown1 = aStream.ReadU32();
    data.m_Unknown2 = aStream.ReadU32();

    ReadCond( aStream, aVer, data.m_Unknown3 );

    data.m_Value = aStream.ReadStringFixed( data.m_Len, true );

    return block;
}


static std::unique_ptr<BLOCK_BASE> ParseBlock_0x3C( FILE_STREAM& aStream, FMT_VER aVer )
{
    auto block = std::make_unique<BLOCK<BLK_0x3C_KEY_LIST>>( 0x3C, aStream.Position() );

    auto& data = block->GetData();

    data.m_T = aStream.ReadU8();
    data.m_T2 = aStream.ReadU16();
    data.m_Key = aStream.ReadU32();

    ReadCond( aStream, aVer, data.m_Unknown );

    data.m_NumEntries = aStream.ReadU32();

    if( data.m_NumEntries > 1000000 )
    {
        THROW_IO_ERROR( wxString::Format(
                "Block 0x3C entry count %u exceeds limit at offset %#010zx",
                data.m_NumEntries, aStream.Position() ) );
    }

    data.m_Entries.reserve( data.m_NumEntries );
    for( uint32_t i = 0; i < data.m_NumEntries; ++i )
    {
        data.m_Entries.push_back( aStream.ReadU32() );
    }

    return block;
}


std::unique_ptr<BLOCK_BASE> ALLEGRO::BLOCK_PARSER::ParseBlock( bool& aEndOfObjectsMarker )
{
    const size_t offset = m_stream.Position();

    // Read the type of the object
    // The file can end here without error.
    uint8_t type = 0x00;
    if( !m_stream.GetU8( type ) )
    {
        aEndOfObjectsMarker = true;
        return nullptr;
    }

    std::unique_ptr<BLOCK_BASE> block;

    switch( type )
    {
    case 0x01:
    {
        block = ParseBlock_0x01_ARC( m_stream, m_ver );
        break;
    }
    case 0x03:
    {
        block = ParseBlock_0x03( m_stream, m_ver );
        break;
    }
    case 0x04:
    {
        block = ParseBlock_0x04_NET_ASSIGNMENT( m_stream, m_ver );
        break;
    }
    case 0x05:
    {
        block = ParseBlock_0x05_TRACK( m_stream, m_ver );
        break;
    }
    case 0x06:
    {
        block = ParseBlock_0x06( m_stream, m_ver );
        break;
    }
    case 0x07:
    {
        block = ParseBlock_0x07( m_stream, m_ver );
        break;
    }
    case 0x08:
    {
        block = ParseBlock_0x08( m_stream, m_ver );
        break;
    }
    case 0x09:
    {
        block = ParseBlock_0x09( m_stream, m_ver );
        break;
    }
    case 0x0A:
    {
        block = ParseBlock_0x0A_DRC( m_stream, m_ver );
        break;
    }
    case 0x0C:
    {
        block = ParseBlock_0x0C( m_stream, m_ver );
        break;
    }
    case 0x0D:
    {
        block = ParseBlock_0x0D_PAD( m_stream, m_ver );
        break;
    }
    case 0x0E:
    {
        block = ParseBlock_0x0E( m_stream, m_ver );
        break;
    }
    case 0x0F:
    {
        block = ParseBlock_0x0F( m_stream, m_ver );
        break;
    }
    case 0x10:
    {
        block = ParseBlock_0x10( m_stream, m_ver );
        break;
    }
    case 0x11:
    {
        block = ParseBlock_0x11( m_stream, m_ver );
        break;
    }
    case 0x12:
    {
        block = ParseBlock_0x12( m_stream, m_ver );
        break;
    }
    case 0x14:
    {
        block = ParseBlock_0x14( m_stream, m_ver );
        break;
    }
    case 0x15:
    case 0x16:
    case 0x17:
    {
        block = ParseBlock_0x15_16_17_SEGMENT( m_stream, m_ver, type );
        break;
    }
    case 0x1B:
    {
        block = ParseBlock_0x1B_NET( m_stream, m_ver );
        break;
    }
    case 0x1C:
    {
        block = ParseBlock_0x1C_PADSTACK( m_stream, m_ver );
        break;
    }
    case 0x1D:
    {
        block = ParseBlock_0x1D( m_stream, m_ver );
        break;
    }
    case 0x1E:
    {
        block = ParseBlock_0x1E( m_stream, m_ver );
        break;
    }
    case 0x1F:
    {
        block = ParseBlock_0x1F( m_stream, m_ver );
        break;
    }
    case 0x20:
    {
        block = ParseBlock_0x20( m_stream, m_ver );
        break;
    }
    case 0x21:
    {
        block = ParseBlock_0x21( m_stream, m_ver );
        break;
    }
    case 0x22:
    {
        block = ParseBlock_0x22( m_stream, m_ver );
        break;
    }
    case 0x23:
    {
        block = ParseBlock_0x23_RATLINE( m_stream, m_ver );
        break;
    }
    case 0x24:
    {
        block = ParseBlock_0x24_RECT( m_stream, m_ver );
        break;
    }
    case 0x26:
    {
        block = ParseBlock_0x26( m_stream, m_ver );
        break;
    }
    case 0x27:
    {
        if( m_x27_end <= m_stream.Position() )
        {
            THROW_IO_ERROR(
                    wxString::Format( "Current offset %#010zx is at or past the expected end of block 0x27 at %#010zx",
                                      m_stream.Position(), m_x27_end ) );
        }
        block = ParseBlock_0x27( m_stream, m_ver, m_x27_end );
        break;
    }
    case 0x28:
    {
        block = ParseBlock_0x28_SHAPE( m_stream, m_ver );
        break;
    }
    case 0x29:
    {
        block = ParseBlock_0x29_PIN( m_stream, m_ver );
        break;
    }
    case 0x2A:
    {
        block = ParseBlock_0x2A( m_stream, m_ver );
        break;
    }
    case 0x2B:
    {
        block = ParseBlock_0x2B( m_stream, m_ver );
        break;
    }
    case 0x2C:
    {
        block = ParseBlock_0x2C_TABLE( m_stream, m_ver );
        break;
    }
    case 0x2D:
    {
        block = ParseBlock_0x2D( m_stream, m_ver );
        break;
    }
    case 0x2E:
    {
        block = ParseBlock_0x2E( m_stream, m_ver );
        break;
    }
    case 0x2F:
    {
        block = ParseBlock_0x2F( m_stream, m_ver );
        break;
    }
    case 0x30:
    {
        block = ParseBlock_0x30_STR_WRAPPER( m_stream, m_ver );
        break;
    }
    case 0x31:
    {
        block = ParseBlock_0x31_SGRAPHIC( m_stream, m_ver );
        break;
    }
    case 0x32:
    {
        block = ParseBlock_0x32_PLACED_PAD( m_stream, m_ver );
        break;
    }
    case 0x33:
    {
        block = ParseBlock_0x33_VIA( m_stream, m_ver );
        break;
    }
    case 0x34:
    {
        block = ParseBlock_0x34_KEEPOUT( m_stream, m_ver );
        break;
    }
    case 0x35:
    {
        block = ParseBlock_0x35( m_stream, m_ver );
        break;
    }
    case 0x36:
    {
        block = ParseBlock_0x36( m_stream, m_ver );
        break;
    }
    case 0x37:
    {
        block = ParseBlock_0x37( m_stream, m_ver );
        break;
    }
    case 0x38:
    {
        block = ParseBlock_0x38_FILM( m_stream, m_ver );
        break;
    }
    case 0x39:
    {
        block = ParseBlock_0x39_FILM_LAYER_LIST( m_stream, m_ver );
        break;
    }
    case 0x3A:
    {
        block = ParseBlock_0x3A_FILM_LIST_NODE( m_stream, m_ver );
        break;
    }
    case 0x3B:
    {
        block = ParseBlock_0x3B( m_stream, m_ver );
        break;
    }
    case 0x3C:
    {
        block = ParseBlock_0x3C( m_stream, m_ver );
        break;
    }
    case 0x00:
    {
        // Block type 0x00 marks the end of the objects section
        aEndOfObjectsMarker = true;
        break;
    }
    default:
        break;
    }

    return block;
}


void ALLEGRO::PARSER::readObjects( BRD_DB& aBoard )
{
    const uint32_t magic = aBoard.m_Header->m_Magic;
    const FMT_VER  ver = aBoard.m_FmtVer;

    if( m_progressReporter )
    {
        m_progressReporter->AdvancePhase( _( "Parsing Allegro objects" ) );
        // This isn't exactly the number we seem to parse, but it's very close
        m_progressReporter->SetMaxProgress( static_cast<int>( aBoard.m_Header->m_ObjectCount ) );
    }

    BLOCK_PARSER blockParser( m_stream, ver, aBoard.m_Header->m_0x27_End );

    auto lastRefresh = std::chrono::steady_clock::now();

    while( true )
    {
        const size_t offset = m_stream.Position();

        // This seems to be always true and is quite useful for debugging out-of-sync objects
        wxASSERT_MSG( offset % 4 == 0,
                      wxString::Format( "Allegro object at %#010zx, offset not aligned to 4 bytes", offset ) );

        // Peek at the block type byte before ParseBlock consumes it so we
        // have the value available for error messages if the type is unknown
        // and ParseBlock returns nullptr.
        uint8_t blockTypeByte = 0;
        m_stream.GetU8( blockTypeByte );
        m_stream.Seek( offset );

        bool                        endOfObjectsMarker = false;
        std::unique_ptr<BLOCK_BASE> block = blockParser.ParseBlock( endOfObjectsMarker );

        if( endOfObjectsMarker )
        {
            if( ver >= FMT_VER::V_180 )
            {
                // V18 files can have zero-padded gaps between block groups. Skip
                // consecutive zero bytes and check if more blocks follow.
                size_t scanPos = m_stream.Position();
                uint8_t nextByte = 0;

                while( m_stream.GetU8( nextByte ) && nextByte == 0x00 )
                    scanPos = m_stream.Position();

                if( nextByte > 0x00 && nextByte <= 0x3C )
                {
                    size_t blockStart = scanPos;

                    if( blockStart % 4 != 0 )
                        blockStart -= ( blockStart % 4 );

                    // After backward alignment the byte at blockStart may
                    // differ from the non-zero byte we found. Verify it
                    // still looks like a valid block type before continuing.
                    uint8_t alignedByte = 0;
                    m_stream.Seek( blockStart );
                    m_stream.GetU8( alignedByte );
                    m_stream.Seek( blockStart );

                    if( alignedByte == 0x00 || alignedByte > 0x3C )
                        break;

                    wxLogTrace( traceAllegroParser,
                                wxString::Format( "V18 zero gap from %#010zx to %#010zx, "
                                                  "continuing at block type %#04x",
                                                  offset, blockStart, nextByte ) );
                    continue;
                }
            }

            if( wxLog::IsAllowedTraceMask( traceAllegroParser ) )
            {
                wxLogTrace( traceAllegroParser,
                            wxString::Format( "End of objects marker (0x00) at index %zu, offset %#010zx",
                                              aBoard.GetObjectCount(), offset ) );
            }

            break;
        }

        if( !block )
        {
            if( !m_endAtUnknownBlock )
            {
                THROW_IO_ERROR( wxString::Format(
                        "Do not have parser for block index %zu type %#02x available at offset %#010zx",
                        aBoard.GetObjectCount(), blockTypeByte, offset ) );
            }
            else
            {
                wxLogTrace( traceAllegroParser,
                            wxString::Format( "Ending at unknown block, index %zu type %#04x at offset %#010zx",
                                              aBoard.GetObjectCount(), blockTypeByte, offset ) );

                wxFAIL_MSG( "Failed to create block" );
                return;
            }
        }
        else
        {
            if( wxLog::IsAllowedTraceMask( traceAllegroParser ) )
            {
                wxLogTrace( traceAllegroParserBlocks,
                            wxString::Format( "Added block %zu, type %#04x from %#010zx to %#010zx",
                                              aBoard.GetObjectCount(), block->GetBlockType(), offset,
                                              m_stream.Position() ) );
            }

            aBoard.InsertBlock( std::move( block ) );

            if( m_progressReporter )
            {
                m_progressReporter->AdvanceProgress();

                if( ( aBoard.GetObjectCount() & 0x3F ) == 0 )
                {
                    auto now = std::chrono::steady_clock::now();

                    if( now - lastRefresh >= std::chrono::milliseconds( 100 ) )
                    {
                        m_progressReporter->KeepRefreshing();
                        lastRefresh = now;
                    }
                }
            }
        }
    }
}


std::unique_ptr<BRD_DB> ALLEGRO::PARSER::Parse()
{
    std::unique_ptr<BRD_DB> board = std::make_unique<BRD_DB>();

    if( m_progressReporter )
    {
        m_progressReporter->AddPhases( 2 );
        m_progressReporter->AdvancePhase( "Reading file header" );
    }

    PROF_TIMER parseTimer;
    HEADER_PARSER headerParser( m_stream );

    try
    {
        board->m_Header = headerParser.ParseHeader();

        if( !board->m_Header )
        {
            THROW_IO_ERROR( "Failed to parse file header" );
        }

        board->m_FmtVer = headerParser.GetFormatVersion();

        {
            auto dumpLL = [&]( const char* name, const FILE_HEADER::LINKED_LIST& ll )
            {
                wxLogTrace( traceAllegroParser, "  LL %-20s head=%#010x tail=%#010x",
                            name, ll.m_Head, ll.m_Tail );
            };

            wxLogTrace( traceAllegroParser, "Header linked lists (ver=%#010x):",
                        board->m_Header->m_Magic );
            dumpLL( "0x04", board->m_Header->m_LL_0x04 );
            dumpLL( "0x06", board->m_Header->m_LL_0x06 );
            dumpLL( "0x0C", board->m_Header->m_LL_0x0C );
            dumpLL( "Shapes", board->m_Header->m_LL_Shapes );
            dumpLL( "0x14", board->m_Header->m_LL_0x14 );
            dumpLL( "0x1B_Nets", board->m_Header->m_LL_0x1B_Nets );
            dumpLL( "0x1C", board->m_Header->m_LL_0x1C );
            dumpLL( "0x24_0x28", board->m_Header->m_LL_0x24_0x28 );
            dumpLL( "Unknown1", board->m_Header->m_LL_Unknown1 );
            dumpLL( "0x2B", board->m_Header->m_LL_0x2B );
            dumpLL( "0x03_0x30", board->m_Header->m_LL_0x03_0x30 );
            dumpLL( "0x0A", board->m_Header->m_LL_0x0A );
            dumpLL( "0x1D_0x1E_0x1F", board->m_Header->m_LL_0x1D_0x1E_0x1F );
            dumpLL( "Unknown2", board->m_Header->m_LL_Unknown2 );
            dumpLL( "0x38", board->m_Header->m_LL_0x38 );
            dumpLL( "0x2C", board->m_Header->m_LL_0x2C );
            dumpLL( "0x0C_2", board->m_Header->m_LL_0x0C_2 );
            dumpLL( "Unknown3", board->m_Header->m_LL_Unknown3 );
            dumpLL( "Unknown5", board->m_Header->m_LL_Unknown5 );
            dumpLL( "0x36", board->m_Header->m_LL_0x36 );
            dumpLL( "Unknown6", board->m_Header->m_LL_Unknown6 );
            dumpLL( "0x0A_2", board->m_Header->m_LL_0x0A_2 );
        }
    }
    catch( const IO_ERROR& e )
    {
        wxString s;
        s += wxString::Format( "Error parsing Allegro file: %s\n", e.What() );
        s += wxString::Format( "Stream position: %#010lx\n", m_stream.Position() );

        if( board->m_Header )
            s += wxString::Format( "File magic: %#010x\n", board->m_Header->m_Magic );
        else
            s += wxString::Format( "File magic: Unknown\n" );

        THROW_IO_ERROR( s );
    }

    wxLogTrace( traceAllegroPerf, wxT( "  ReadHeader: %.3f ms" ), parseTimer.msecs( true ) ); //format:allow

    if( board->m_FmtVer == FMT_VER::V_PRE_V16 )
    {
        wxString verStr( board->m_Header->m_AllegroVersion.data(), 60 );
        verStr.Trim();

        THROW_IO_ERROR( wxString::Format(
                _( "This file was created with %s, which uses a binary format that "
                   "predates Allegro 16.0 and is not supported by this importer.\n\n"
                   "To import this design, open it in Cadence Allegro PCB Editor "
                   "version 16.0 or later and re-save, then import the resulting file." ),
                verStr ) );
    }

    board->ReserveCapacity( board->m_Header->m_ObjectCount, board->m_Header->m_StringsCount );

    // Skip DB_OBJ creation for high-volume types (segments, graphics, arcs) that the
    // BOARD_BUILDER accesses only through raw BLOCK_BASE. Saves millions of allocations.
    board->SetLeanMode( true );

    try
    {
        ReadStringMap( m_stream, *board, board->m_Header->m_StringsCount );

        wxLogTrace( traceAllegroPerf, wxT( "  ReadStringMap (%u strings): %.3f ms" ), //format:allow
                    board->m_Header->m_StringsCount, parseTimer.msecs( true ) );

        readObjects( *board );

        wxLogTrace( traceAllegroPerf, wxT( "  readObjects (%zu objects): %.3f ms" ), //format:allow
                    board->GetObjectCount(), parseTimer.msecs( true ) );
    }
    catch( const IO_ERROR& e )
    {
        wxString s;
        s += wxString::Format( "Error parsing Allegro file: %s\n", e.What() );
        s += wxString::Format( "Stream position: %#010lx\n", m_stream.Position() );

        if( board->m_Header )
            s += wxString::Format( "File magic: %#010x\n", board->m_Header->m_Magic );
        else
            s += wxString::Format( "File magic: Unknown\n" );

        THROW_IO_ERROR( s );
    }

    // Now the object are read, resolve the DB links
    board->ResolveAndValidate();

    wxLogTrace( traceAllegroPerf, wxT( "  ResolveAndValidate: %.3f ms" ), parseTimer.msecs( true ) ); //format:allow
    wxLogTrace( traceAllegroPerf, wxT( "  Phase 1 total: %.3f ms" ), parseTimer.msecs() ); //format:allow

    return board;
}


ALLEGRO::RAW_BOARD::RAW_BOARD() :
    m_FmtVer( FMT_VER::V_UNKNOWN )
{}
