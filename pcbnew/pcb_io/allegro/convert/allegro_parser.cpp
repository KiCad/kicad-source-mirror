/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "convert/allegro_parser.h"

#include <cstring>
#include <iostream>  // TOOD remove

#include <wx/sstream.h>
#include <wx/log.h>
#include <wx/translation.h>

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


static FMT_VER GetFormatVer( uint32_t aMagic )
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
    default: break;
    }

    // This is an unknown version, so we can't parse it
    // Because the struct sizes depend on the version, we can't
    // really do anything with the file other than throw an error
    // and hope the user sends the file in for analysis.
    THROW_IO_ERROR( wxString::Format( "Unknown Allegro file version %#010x", aMagic ) );
}


static FILE_HEADER::LINKED_LIST ReadLL( FILE_STREAM& aStream )
{
    return FILE_HEADER::LINKED_LIST{
        .m_Tail = aStream.ReadU32(),
        .m_Head = aStream.ReadU32(),
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
    else if constexpr( std::is_same_v<T, std::array<uint32_t, field.size()>> )
    {
        for( size_t i = 0; i < field.size(); ++i )
        {
            field[i] = aStream.ReadU32();
        }
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


static std::unique_ptr<ALLEGRO::FILE_HEADER> ReadHeader( FILE_STREAM& stream )
{
    auto header = std::make_unique<ALLEGRO::FILE_HEADER>();

    uint32_t fileMagic = stream.ReadU32();

    header->m_Magic = fileMagic;
    ReadArrayU32( stream, header->m_Unknown1 );
    header->m_ObjectCount = stream.ReadU32();
    ReadArrayU32( stream, header->m_Unknown2 );
    header->m_LL_0x04 = ReadLL( stream );
    header->m_LL_0x06 = ReadLL( stream );
    header->m_LL_0x0C = ReadLL( stream );
    header->m_LL_Shapes = ReadLL( stream );
    header->m_LL_0x14 = ReadLL( stream );
    header->m_LL_0x1B_Nets = ReadLL( stream );
    header->m_LL_0x1C = ReadLL( stream );
    header->m_LL_0x24_0x28 = ReadLL( stream );
    header->m_LL_Unknown1 = ReadLL( stream );
    header->m_LL_0x2B = ReadLL( stream );
    header->m_LL_0x03_0x30 = ReadLL( stream );
    header->m_LL_0x0A = ReadLL( stream );
    header->m_LL_0x1D_0x1E_0x1F = ReadLL( stream );
    header->m_LL_Unknown2 = ReadLL( stream );
    header->m_LL_0x38 = ReadLL( stream );
    header->m_LL_0x2C = ReadLL( stream );
    header->m_LL_0x0C_2 = ReadLL( stream );
    header->m_LL_Unknown3 = ReadLL( stream );
    header->m_0x35_Start = stream.ReadU32();
    header->m_0x35_End = stream.ReadU32();
    header->m_LL_0x36 = ReadLL( stream );
    header->m_LL_Unknown5 = ReadLL( stream );
    header->m_LL_Unknown6 = ReadLL( stream );
    header->m_LL_0x0A_2 = ReadLL( stream );
    header->m_Unknown3 = stream.ReadU32();
    stream.ReadBytes( header->m_AllegroVersion.data(), header->m_AllegroVersion.size() );
    header->m_Unknown4 = stream.ReadU32();
    header->m_MaxKey = stream.ReadU32();
    ReadArrayU32( stream, header->m_Unknown5 );

    {
        uint8_t units = stream.ReadU8();

        switch( units )
        {
        case BOARD_UNITS::IMPERIAL:
        case BOARD_UNITS::METRIC: header->m_BoardUnits = static_cast<BOARD_UNITS>( units ); break;
        default: THROW_IO_ERROR( wxString::Format( "Unknown board units %d", units ) );
        }

        stream.Skip( 3 );
    }

    header->m_Unknown6 = stream.ReadU32();
    header->m_Unknown7 = stream.ReadU32();
    header->m_0x27_End = stream.ReadU32();
    header->m_Unknown8 = stream.ReadU32();
    header->m_StringsCount = stream.ReadU32();

    ReadArrayU32( stream, header->m_Unknown9 );

    header->m_UnitsDivisor = stream.ReadU32();

    stream.SkipU32( 110 );

    for( size_t i = 0; i < header->m_LayerMap.size(); ++i )
    {
        header->m_LayerMap[i].m_A = stream.ReadU32();
        header->m_LayerMap[i].m_LayerList0x2A = stream.ReadU32();
    }

    wxLogTrace( traceAllegroParser, wxT( "Parsed header: %d objects, %d strings" ), header->m_ObjectCount,
                header->m_StringsCount );

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
    auto block = std::make_unique<BLOCK<BLK_0x03>>( 0x03, aStream.Position() );

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

    wxLogTrace( traceAllegroParser, wxT( "  Parsed block 0x03: subtype %#02x, size %d" ), data.m_SubType, data.m_Size );

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
        BLK_0x03::SUB_0x6C sub;
        sub.m_NumEntries = aStream.ReadU32();

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
        BLK_0x03::SUB_0x70_0x74 sub;

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
        BLK_0x03::SUB_0xF6 sub;
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
    auto block = std::make_unique<BLOCK<BLK_0x06>>( 0x06, stream.Position() );

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
    auto block = std::make_unique<BLOCK<BLK_0x07>>( 0x07, stream.Position() );

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
    auto block = std::make_unique<BLOCK<BLK_0x08>>( 0x08, aStream.Position() );

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
    auto block = std::make_unique<BLOCK<BLK_0x09>>( 0x09, aStream.Position() );

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
    auto block = std::make_unique<BLOCK<BLK_0x0C>>( 0x0C, aStream.Position() );

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
    auto block = std::make_unique<BLOCK<BLK_0x0E>>( 0x0E, aStream.Position() );

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
    auto block = std::make_unique<BLOCK<BLK_0x0F>>( 0x0F, stream.Position() );

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
    auto block = std::make_unique<BLOCK<BLK_0x10>>( 0x10, stream.Position() );

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
    auto block = std::make_unique<BLOCK<BLK_0x11>>( 0x11, aStream.Position() );

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
    auto block = std::make_unique<BLOCK<BLK_0x12>>( 0x12, aStream.Position() );

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
    auto block = std::make_unique<BLOCK<BLK_0x14>>( 0x14, aStream.Position() );

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
    data.m_UnknownPtr3 = stream.ReadU32();
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
    ReadCond( aStream, aVer, data.m_Unknown11 );

    ReadArrayU32( aStream, data.m_UnknownArr8 );

    ReadCond( aStream, aVer, data.m_UnknownArr28 );
    ReadCond( aStream, aVer, data.m_UnknownArr8_2 );

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
    auto block = std::make_unique<BLOCK<BLK_0x1D>>( 0x1D, aStream.Position() );

    auto& data = block->GetData();

    aStream.Skip( 3 );

    data.m_Key = aStream.ReadU32();
    data.m_Unknown1 = aStream.ReadU32();
    data.m_Unknown2 = aStream.ReadU32();
    data.m_Unknown3 = aStream.ReadU32();

    data.m_SizeA = aStream.ReadU16();
    data.m_SizeB = aStream.ReadU16();

    data.m_DataB.reserve( data.m_SizeB );
    for( size_t i = 0; i < data.m_SizeB; ++i )
    {
        auto& item = data.m_DataB.emplace_back();
        for( size_t j = 0; j < item.size(); ++j )
        {
            item[j] = aStream.ReadU8();
        }
    }

    data.m_DataA.reserve( data.m_SizeA );
    for( size_t i = 0; i < data.m_SizeA; ++i )
    {
        auto& item = data.m_DataA.emplace_back();
        for( size_t j = 0; j < item.size(); ++j )
        {
            item[j] = aStream.ReadU8();
        }
    }

    ReadCond( aStream, aVer, data.m_Unknown4 );

    return block;
}


static std::unique_ptr<BLOCK_BASE> ParseBlock_0x1E( FILE_STREAM& aStream, FMT_VER aVer )
{
    auto block = std::make_unique<BLOCK<BLK_0x1E>>( 0x1E, aStream.Position() );

    auto& data = block->GetData();

    data.m_Type = aStream.ReadU8();
    data.m_T2 = aStream.ReadU16();
    data.m_Key = aStream.ReadU32();
    data.m_Unknown1 = aStream.ReadU32();

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
    auto block = std::make_unique<BLOCK<BLK_0x1F>>( 0x1F, aStream.Position() );

    auto& data = block->GetData();

    aStream.Skip( 3 );

    data.m_Key = aStream.ReadU32();

    data.m_Unknown1 = aStream.ReadU32();
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

    data.m_Substruct.reserve( substructSize );
    for( size_t i = 0; i < substructSize; i++ )
    {
        data.m_Substruct.push_back( aStream.ReadU8() );
    }

    return block;
}


static std::unique_ptr<BLOCK_BASE> ParseBlock_0x21( FILE_STREAM& aStream, FMT_VER aVer )
{
    auto block = std::make_unique<BLOCK<BLK_0x21>>( 0x21, aStream.Position() );

    auto& data = block->GetData();

    data.m_Type = aStream.ReadU8();
    data.m_R = aStream.ReadU16();

    data.m_Size = aStream.ReadU32();

    data.m_Key = aStream.ReadU32();

    const size_t nBytes = data.m_Size - 12;
    data.m_Data.reserve( nBytes );
    for( size_t i = 0; i < nBytes; ++i )
    {
        data.m_Data.push_back( aStream.ReadU8() );
    }

    return block;
}


static std::unique_ptr<BLOCK_BASE> ParseBlock_0x22( FILE_STREAM& aStream, FMT_VER aVer )
{
    auto block = std::make_unique<BLOCK<BLK_0x22>>( 0x22, aStream.Position() );

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
    auto block = std::make_unique<BLOCK<BLK_0x26>>( 0x26, aStream.Position() );

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
    auto block = std::make_unique<BLOCK<BLK_0x27>>( 0x27, aStream.Position() );

    auto& data = block->GetData();

    const size_t size = aEndOff - 1 - aStream.Position();

    data.m_Data.resize( size );
    aStream.ReadBytes( data.m_Data.data(), size );

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
    auto block = std::make_unique<BLOCK<BLK_0x2B>>( 0x2B, stream.Position() );

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
    auto block = std::make_unique<BLOCK<BLK_0x2D>>( 0x2D, stream.Position() );

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
    auto block = std::make_unique<BLOCK<BLK_0x2E>>( 0x2E, aStream.Position() );

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
    auto block = std::make_unique<BLOCK<BLK_0x2F>>( 0x2F, aStream.Position() );

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
    auto block = std::make_unique<BLOCK<BLK_0x35>>( 0x35, aStream.Position() );

    auto& data = block->GetData();

    data.m_T2 = aStream.ReadU8();
    data.m_T3 = aStream.ReadU16();
    aStream.ReadBytes( data.m_Content.data(), data.m_Content.size() );

    return block;
}


static std::unique_ptr<BLOCK_BASE> ParseBlock_0x36( FILE_STREAM& aStream, FMT_VER aVer )
{
    auto block = std::make_unique<BLOCK<BLK_0x36>>( 0x36, aStream.Position() );

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

    data.m_Items.reserve( data.m_NumItems );
    for( uint32_t i = 0; i < data.m_NumItems; ++i )
    {
        switch( data.m_Code )
        {
        case 0x02:
        {
            BLK_0x36::X02 item;

            item.m_String = aStream.ReadStringFixed( 32, true );
            ReadArrayU32( aStream, item.m_Xs );
            ReadCond( aStream, aVer, item.m_Ys );
            ReadCond( aStream, aVer, item.m_Zs );

            data.m_Items.emplace_back( std::move( item ) );
            break;
        }
        case 0x03:
        {
            BLK_0x36::X03 item;
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
            BLK_0x36::X05 item;

            aStream.ReadBytes( item.m_Unknown.data(), item.m_Unknown.size() );

            data.m_Items.emplace_back( std::move( item ) );
            break;
        }
        case 0x06:
        {
            BLK_0x36::X06 item;

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
            BLK_0x36::FontDef_X08 item;

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
            BLK_0x36::X0B item;
            aStream.ReadBytes( item.m_Unknown.data(), item.m_Unknown.size() );
            data.m_Items.emplace_back( std::move( item ) );
            break;
        }
        case 0x0C:
        {
            BLK_0x36::X0C item;
            aStream.ReadBytes( item.m_Unknown.data(), item.m_Unknown.size() );
            data.m_Items.emplace_back( std::move( item ) );
            break;
        }
        case 0x0D:
        {
            BLK_0x36::X0D item;
            aStream.ReadBytes( item.m_Unknown.data(), item.m_Unknown.size() );
            data.m_Items.emplace_back( std::move( item ) );
            break;
        }
        case 0x0F:
        {
            BLK_0x36::X0F item;
            item.m_Key = aStream.ReadU32();
            ReadArrayU32( aStream, item.m_Ptrs );
            item.m_Ptr2 = aStream.ReadU32();
            data.m_Items.emplace_back( std::move( item ) );
            break;
        }
        case 0x10:
        {
            BLK_0x36::X10 item;
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
    auto block = std::make_unique<BLOCK<BLK_0x37>>( 0x37, aStream.Position() );

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
    auto block = std::make_unique<BLOCK<TYPE_3A_FILM_LIST_NODE>>( 0x3A, aStream.Position() );

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
    auto block = std::make_unique<BLOCK<BLK_0x3B>>( 0x3B, aStream.Position() );

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
    auto block = std::make_unique<BLOCK<BLK_0x3C>>( 0x3C, aStream.Position() );

    auto& data = block->GetData();

    data.m_T = aStream.ReadU8();
    data.m_T2 = aStream.ReadU16();
    data.m_Key = aStream.ReadU32();

    ReadCond( aStream, aVer, data.m_Unknown );

    data.m_NumEntries = aStream.ReadU32();
    data.m_Entries.reserve( data.m_NumEntries );
    for( uint32_t i = 0; i < data.m_NumEntries; ++i )
    {
        data.m_Entries.push_back( aStream.ReadU32() );
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

    while( true )
    {
        const size_t offset = m_stream.Position();

        // This seems to be always true and is quite useful for debugging out-of-sync objects
        wxASSERT_MSG( offset % 4 == 0,
                      wxString::Format( "Allegro object at %#010zx, offset not aligned to 4 bytes", offset ) );

        // Read the type of the object
        // The file can end here without error.
        uint8_t type = 0x00;
        if( !m_stream.GetU8( type ) )
        {
            break;
        }

        std::unique_ptr<BLOCK_BASE> block;

        switch( type )
        {
        case 0x01:
        {
            block = ParseBlock_0x01_ARC( m_stream, ver );
            break;
        }
        case 0x03:
        {
            block = ParseBlock_0x03( m_stream, ver );
            break;
        }
        case 0x04:
        {
            block = ParseBlock_0x04_NET_ASSIGNMENT( m_stream, ver );
            break;
        }
        case 0x05:
        {
            block = ParseBlock_0x05_TRACK( m_stream, ver );
            break;
        }
        case 0x06:
        {
            block = ParseBlock_0x06( m_stream, ver );
            break;
        }
        case 0x07:
        {
            block = ParseBlock_0x07( m_stream, ver );
            break;
        }
        case 0x08:
        {
            block = ParseBlock_0x08( m_stream, ver );
            break;
        }
        case 0x09:
        {
            block = ParseBlock_0x09( m_stream, ver );
            break;
        }
        case 0x0A:
        {
            block = ParseBlock_0x0A_DRC( m_stream, ver );
            break;
        }
        case 0x0C:
        {
            block = ParseBlock_0x0C( m_stream, ver );
            break;
        }
        case 0x0D:
        {
            block = ParseBlock_0x0D_PAD( m_stream, ver );
            break;
        }
        case 0x0E:
        {
            block = ParseBlock_0x0E( m_stream, ver );
            break;
        }
        case 0x0F:
        {
            block = ParseBlock_0x0F( m_stream, ver );
            break;
        }
        case 0x10:
        {
            block = ParseBlock_0x10( m_stream, ver );
            break;
        }
        case 0x11:
        {
            block = ParseBlock_0x11( m_stream, ver );
            break;
        }
        case 0x12:
        {
            block = ParseBlock_0x12( m_stream, ver );
            break;
        }
        case 0x14:
        {
            block = ParseBlock_0x14( m_stream, ver );
            break;
        }
        case 0x15:
        case 0x16:
        case 0x17:
        {
            block = ParseBlock_0x15_16_17_SEGMENT( m_stream, ver, type );
            break;
        }
        case 0x1B:
        {
            block = ParseBlock_0x1B_NET( m_stream, ver );
            break;
        }
        case 0x1C:
        {
            block = ParseBlock_0x1C_PADSTACK( m_stream, ver );
            break;
        }
        case 0x1D:
        {
            block = ParseBlock_0x1D( m_stream, ver );
            break;
        }
        case 0x1E:
        {
            block = ParseBlock_0x1E( m_stream, ver );
            break;
        }
        case 0x1F:
        {
            block = ParseBlock_0x1F( m_stream, ver );
            break;
        }
        case 0x21:
        {
            block = ParseBlock_0x21( m_stream, ver );
            break;
        }
        case 0x22:
        {
            block = ParseBlock_0x22( m_stream, ver );
            break;
        }
        case 0x23:
        {
            block = ParseBlock_0x23_RATLINE( m_stream, ver );
            break;
        }
        case 0x24:
        {
            block = ParseBlock_0x24_RECT( m_stream, ver );
            break;
        }
        case 0x26:
        {
            block = ParseBlock_0x26( m_stream, ver );
            break;
        }
        case 0x27:
        {
            size_t endOffset = aBoard.m_Header->m_0x27_End;
            block = ParseBlock_0x27( m_stream, ver, endOffset );
            break;
        }
        case 0x28:
        {
            block = ParseBlock_0x28_SHAPE( m_stream, ver );
            break;
        }
        case 0x29:
        {
            block = ParseBlock_0x29_PIN( m_stream, ver );
            break;
        }
        case 0x2A:
        {
            block = ParseBlock_0x2A( m_stream, ver );
            break;
        }
        case 0x2B:
        {
            block = ParseBlock_0x2B( m_stream, ver );
            break;
        }
        case 0x2C:
        {
            block = ParseBlock_0x2C_TABLE( m_stream, ver );
            break;
        }
        case 0x2D:
        {
            block = ParseBlock_0x2D( m_stream, ver );
            break;
        }
        case 0x2E:
        {
            block = ParseBlock_0x2E( m_stream, ver );
            break;
        }
        case 0x2F:
        {
            block = ParseBlock_0x2F( m_stream, ver );
            break;
        }
        case 0x30:
        {
            block = ParseBlock_0x30_STR_WRAPPER( m_stream, ver );
            break;
        }
        case 0x31:
        {
            block = ParseBlock_0x31_SGRAPHIC( m_stream, ver );
            break;
        }
        case 0x32:
        {
            block = ParseBlock_0x32_PLACED_PAD( m_stream, ver );
            break;
        }
        case 0x33:
        {
            block = ParseBlock_0x33_VIA( m_stream, ver );
            break;
        }
        case 0x34:
        {
            block = ParseBlock_0x34_KEEPOUT( m_stream, ver );
            break;
        }
        case 0x35:
        {
            block = ParseBlock_0x35( m_stream, ver );
            break;
        }
        case 0x36:
        {
            block = ParseBlock_0x36( m_stream, ver );
            break;
        }
        case 0x37:
        {
            block = ParseBlock_0x37( m_stream, ver );
            break;
        }
        case 0x38:
        {
            block = ParseBlock_0x38_FILM( m_stream, ver );
            break;
        }
        case 0x39:
        {
            block = ParseBlock_0x39_FILM_LAYER_LIST( m_stream, ver );
            break;
        }
        case 0x3A:
        {
            block = ParseBlock_0x3A_FILM_LIST_NODE( m_stream, ver );
            break;
        }
        case 0x3B:
        {
            block = ParseBlock_0x3B( m_stream, ver );
            break;
        }
        case 0x3C:
        {
            block = ParseBlock_0x3C( m_stream, ver );
            break;
        }
        case 0x00:
        {
            // Block type 0x00 marks the end of the objects section
            wxLogTrace( traceAllegroParser,
                        wxString::Format( "End of objects marker (0x00) at index %zu, offset %#010zx",
                                          aBoard.GetObjectCount(), offset ) );
            return;
        }
        default:
        {
            if( !m_endAtUnknownBlock )
            {
                THROW_IO_ERROR( wxString::Format(
                        "Do not have parser for block index %zu type %#02x available at offset %#010zx",
                        aBoard.GetObjectCount(), type, offset ) );
            }

            wxLogTrace( traceAllegroParser,
                        wxString::Format( "Ending at unknown block, index %zu type %#04x at offset %#010zx",
                                          aBoard.GetObjectCount(), type, offset ) );
            return;
        }
        }

        if( block )
        {
            wxLogTrace( traceAllegroParser,
                        wxString::Format( "Added block %zu, type %#04x from %#010zx to %#010zx",
                                          aBoard.GetObjectCount(), type, offset, m_stream.Position() ) );

            // Turn the binary-ish data into database objects
            aBoard.InsertBlock( std::move( block ) );

            // if( m_progressReporter )
            // {
            //     m_progressReporter->AdvanceProgress();
            // }
        }
        else
        {
            wxFAIL_MSG( "Failed to create block" );
            return;
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

    try
    {
        board->m_Header = ReadHeader( m_stream );
        board->m_FmtVer = GetFormatVer( board->m_Header->m_Magic );

        ReadStringMap( m_stream, *board, board->m_Header->m_StringsCount );

        readObjects( *board );
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

    return board;
}


ALLEGRO::RAW_BOARD::RAW_BOARD() :
    m_FmtVer( FMT_VER::V_UNKNOWN )
{}
