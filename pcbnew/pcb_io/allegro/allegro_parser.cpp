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

#include <allegro_parser.h>

#include <wx/sstream.h>
#include <wx/log.h>

#include <allegro_pcb_structs.h>

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
    case 0x00140400: return FMT_VER::V_172;
    case 0x00140900: return FMT_VER::V_174;
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
    uint32_t a = aStream.ReadU32();
    uint32_t b = aStream.ReadU32();

    // TODO: not quite sure how this works
    // Start here and figure it out later
    return double( a ) + ( double( b ) / 2e31 );
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
    header->m_LL_Nets = ReadLL( stream );
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
    header->m_LL_Unknown4 = ReadLL( stream );
    header->m_LL_Unknown5 = ReadLL( stream );
    header->m_LL_Unknown6 = ReadLL( stream );
    header->m_LL_Unknown7 = ReadLL( stream );
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
        case BOARD_UNITS::METRIC:
            header->m_BoardUnits = static_cast<BOARD_UNITS>( units );
            break;
        default:
            THROW_IO_ERROR( wxString::Format( "Unknown board units %d", units ) );
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

    for( size_t i = 0; i < header->m_LayerMap.size(); ++i )
    {
        header->m_LayerMap[i].m_A = stream.ReadU32();
        header->m_LayerMap[i].m_B = stream.ReadU32();
    }

    wxLogTrace( traceAllegroParser, wxT( "Parsed header: %d objects, %d strings" ), header->m_ObjectCount,
                header->m_StringsCount );

    return header;
}


static void ReadStringMap( FILE_STREAM& stream, RAW_BOARD& aBoard, uint32_t count )
{
    stream.Seek( RAW_BOARD::STRING_TABLE_OFFSET );

    for( uint32_t i = 0; i < count; ++i )
    {
        uint32_t id = stream.ReadU32();
        aBoard.m_StringTable[id] = stream.ReadString( true );
    }
}


static LAYER_INFO ParseLayerInfo( FILE_STREAM& aStream )
{
    uint8_t famCode = aStream.ReadU8();
    uint8_t ord = aStream.ReadU8();


    LAYER_INFO::FAMILY family = LAYER_INFO::FAMILY::BOARD_GEOM;

    switch( famCode )
    {
    case 0x01: family = LAYER_INFO::FAMILY::BOARD_GEOM; break;
    case 0x06: family = LAYER_INFO::FAMILY::COPPER; break;
    case 0x09: family = LAYER_INFO::FAMILY::SILK; break;
    // In the PreAmp board - TODO: come back to this
    case 0x12: family = LAYER_INFO::FAMILY::UNKNOWN_0x12; break;
    default: THROW_IO_ERROR( wxString::Format( "Unknown LAYER_INFO family code: %#04x", famCode ) );
    };

    return LAYER_INFO{
        .m_Family = family,
        .m_Ordinal = ord,
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

    for( size_t i = 0; i < data.m_Coords.size(); ++i )
    {
        data.m_Coords[i] = aStream.ReadS32();
    }

    data.m_X = ReadAllegroFloat( aStream );
    data.m_Y = ReadAllegroFloat( aStream );
    data.m_R = ReadAllegroFloat( aStream );

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

    aStream.Skip( 3 );

    data.m_Key = aStream.ReadU32();
    data.m_Next = aStream.ReadU32();

    ReadCond( aStream, aVer, data.m_Unknown1 );

    data.m_SubType = aStream.ReadU8();
    data.m_UnknownByte = aStream.ReadU8();
    data.m_Size = aStream.ReadU16();

    ReadCond( aStream, aVer, data.m_Unknown2 );

    switch( data.m_SubType )
    {
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
        data.m_Substruct = aStream.ReadString( true );
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


static std::unique_ptr<BLOCK_BASE> ParseBlock_0x05_TRACK( FILE_STREAM& aStream, FMT_VER aVer )
{
    auto block = std::make_unique<BLOCK<BLK_0x05_TRACK>>( 0x05, aStream.Position() );

    auto& data = block->GetData();

    aStream.Skip( 3 );

    data.m_Key = aStream.ReadU32();
    data.m_Unknown1 = aStream.ReadU32();
    data.m_UnknownPtr1 = aStream.ReadU32();
    data.m_UnknownPtr2 = aStream.ReadU32();
    data.m_Unknown2a = aStream.ReadU32();
    data.m_Unknown2b = aStream.ReadU32();
    data.m_UnknownPtr3 = aStream.ReadU32();
    data.m_UnknownPtr4 = aStream.ReadU32();
    data.m_Unknown3 = aStream.ReadU32();

    data.m_Ptr0x3A = aStream.ReadU32();

    ReadCond( aStream, aVer, data.m_Ptr0x3B );

    ReadCond( aStream, aVer, data.m_PtrA );
    ReadCond( aStream, aVer, data.m_PtrB );
    ReadCond( aStream, aVer, data.m_PtrC );

    data.m_FirstSegPtr = aStream.ReadU32();
    data.m_UnknownPtr5 = aStream.ReadU32();
    data.m_Unknown4 = aStream.ReadU32();

    return block;
}


static std::unique_ptr<BLOCK_BASE> ParseBlock_0x06( FILE_STREAM& stream, FMT_VER aVer )
{
    auto block = std::make_unique<BLOCK<BLK_0x06>>( 0x06, stream.Position() );

    auto& data = block->GetData();

    stream.Skip( 3 );

    data.m_Key = stream.ReadU32();
    data.m_Next = stream.ReadU32();
    data.m_PtrStr = stream.ReadU32();
    data.m_UnknownPtr = stream.ReadU32();
    data.m_PtrInstance = stream.ReadU32();
    data.m_PtrFootprint = stream.ReadU32();
    data.m_Ptr0x08 = stream.ReadU32();
    data.m_PtrSymbol = stream.ReadU32();

    ReadCond( stream, aVer, data.m_UnknownPtr2 );

    return block;
}


static std::unique_ptr<BLOCK_BASE> ParseBlock_0x07( FILE_STREAM& stream, FMT_VER aVer )
{
    auto block = std::make_unique<BLOCK<BLK_0x07>>( 0x07, stream.Position() );

    auto& data = block->GetData();

    stream.Skip( 3 );

    data.m_Key = stream.ReadU32();
    data.m_Unknown1 = stream.ReadU32();
    ReadCond( stream, aVer, data.m_UnknownPtr1 );
    ReadCond( stream, aVer, data.m_Unknown2 );
    ReadCond( stream, aVer, data.m_Unknown3 );

    data.m_Ptr0x2D = stream.ReadU32();

    ReadCond( stream, aVer, data.m_Unknown4 );

    data.m_RefDesRef = stream.ReadU32();
    data.m_UnknownPtr2 = stream.ReadU32();
    data.m_UnknownPtr3 = stream.ReadU32();
    data.m_Unknown5 = stream.ReadU32();
    data.m_UnknownPtr4 = stream.ReadU32();

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


static std::unique_ptr<BLOCK_BASE> ParseBlock_0x0F( FILE_STREAM& stream, FMT_VER aVer )
{
    auto block = std::make_unique<BLOCK<BLK_0x0F>>( 0x0F, stream.Position() );

    auto& data = block->GetData();

    stream.Skip( 3 );

    data.m_Key = stream.ReadU32();
    data.m_Ptr = stream.ReadU32();
    stream.ReadBytes( data.m_Name.data(), data.m_Name.size() );
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

    ReadCond( stream, aVer, data.m_UnknownPtr1 );
    data.m_UnknownPtr2 = stream.ReadU32();
    ReadCond( stream, aVer, data.m_Unknown1 );
    data.m_UnknownPtr3 = stream.ReadU32();
    data.m_Unknown2 = stream.ReadU32();
    data.m_UnknownPtr4 = stream.ReadU32();
    data.m_UnknownPtr5 = stream.ReadU32();
    data.m_PathStr = stream.ReadU32();

    return block;
}


static std::unique_ptr<BLOCK_BASE> ParseBlock_0x15_SEGMENT( FILE_STREAM& aStream, FMT_VER aVer )
{
    auto block = std::make_unique<BLOCK<BLK_0x15_SEGMENT>>( 0x15, aStream.Position() );

    auto& data = block->GetData();

    aStream.Skip( 3 );

    data.m_Key = aStream.ReadU32();
    data.m_Next = aStream.ReadU32();
    data.m_Parent = aStream.ReadU32();
    data.m_Unknown1 = aStream.ReadU32();

    ReadCond( aStream, aVer, data.m_Unknown2 );

    data.m_Width = aStream.ReadU32();

    for( size_t i = 0; i < data.m_Coords.size(); ++i )
    {
        data.m_Coords[i] = aStream.ReadS32();
    }

    return block;
}


static std::unique_ptr<BLOCK_BASE> ParseBlock_0x16_SEGMENT( FILE_STREAM& aStream, FMT_VER aVer )
{
    auto block = std::make_unique<BLOCK<BLK_0x16_SEGMENT>>( 0x16, aStream.Position() );

    auto& data = block->GetData();

    aStream.Skip( 3 );

    data.m_Key = aStream.ReadU32();
    data.m_Next = aStream.ReadU32();
    data.m_Parent = aStream.ReadU32();
    data.m_Flags = aStream.ReadU32();

    ReadCond( aStream, aVer, data.m_Unknown2 );

    data.m_Width = aStream.ReadU32();

    for( size_t i = 0; i < data.m_Coords.size(); ++i )
    {
        data.m_Coords[i] = aStream.ReadS32();
    }

    return block;
}


static std::unique_ptr<BLOCK_BASE> ParseBlock_0x17_SEGMENT( FILE_STREAM& aStream, FMT_VER aVer )
{
    auto block = std::make_unique<BLOCK<BLK_0x17_SEGMENT>>( 0x17, aStream.Position() );

    auto& data = block->GetData();

    aStream.Skip( 3 );

    data.m_Key = aStream.ReadU32();
    data.m_Next = aStream.ReadU32();
    data.m_Parent = aStream.ReadU32();
    data.m_Unknown1 = aStream.ReadU32();

    ReadCond( aStream, aVer, data.m_Unknown2 );

    data.m_Width = aStream.ReadU32();

    for( size_t i = 0; i < data.m_Coords.size(); ++i )
    {
        data.m_Coords[i] = aStream.ReadS32();
    }

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
    data.m_UnknownPtr1 = stream.ReadU32();
    data.m_UnknownPtr2 = stream.ReadU32();
    data.m_PathStrPtr = stream.ReadU32();
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
    data.m_Unknown1 = aStream.ReadU32();
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

    const size_t nComps = aVer < FMT_VER::V_172 ? ( 10 + data.m_LayerCount * 3 ) : ( 21 + data.m_LayerCount * 4 );

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

        if( aVer < FMT_VER::V_172 && i < nComps - 1 )
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


static std::unique_ptr<BLOCK_BASE> ParseBlock_0x2A( FILE_STREAM& aStream, FMT_VER aVer )
{
    auto block = std::make_unique<BLOCK<BLK_0x2A>>( 0x2A, aStream.Position() );

    auto& data = block->GetData();

    aStream.Skip( 1 );

    data.m_NumEntries = aStream.ReadU16();

    ReadCond( aStream, aVer, data.m_Unknown );

    if( data.m_NonRefEntries.exists( aVer ) )
    {
        data.m_NonRefEntries = std::vector<BLK_0x2A::NONREF_ENTRY>();
        data.m_NonRefEntries->reserve( data.m_NumEntries );
        for( size_t i = 0; i < data.m_NumEntries; ++i )
        {
            BLK_0x2A::NONREF_ENTRY& entry = data.m_NonRefEntries->emplace_back();

            aStream.ReadBytes( entry.m_Unknown.data(), entry.m_Unknown.size() );
        }
    }
    else
    {
        data.m_RefEntries = std::vector<BLK_0x2A::REF_ENTRY>();
        data.m_RefEntries->reserve( data.m_NumEntries );
        for( size_t i = 0; i < data.m_NumEntries; ++i )
        {
            BLK_0x2A::REF_ENTRY& entry = data.m_RefEntries->emplace_back();

            entry.mPtr = aStream.ReadU32();
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
    data.m_UnknownPtr2 = stream.ReadU32();
    data.m_UnknownPtr3 = stream.ReadU32();
    data.m_UnknownPtr4 = stream.ReadU32();
    data.m_UnknownPtr5 = stream.ReadU32();
    data.m_StrPtr = stream.ReadU32();
    data.m_UnknownPtr6 = stream.ReadU32();
    data.m_UnknownPtr7 = stream.ReadU32();
    data.m_UnknownPtr8 = stream.ReadU32();

    ReadCond( stream, aVer, data.m_Unknown2 );
    ReadCond( stream, aVer, data.m_Unknown3 );

    return block;
}


static std::unique_ptr<BLOCK_BASE> ParseBlock_0x2D( FILE_STREAM& stream, FMT_VER aVer )
{
    auto block = std::make_unique<BLOCK<BLK_0x2D>>( 0x2D, stream.Position() );

    auto& data = block->GetData();

    stream.Skip( 3 );

    data.m_Key = stream.ReadU32();
    data.m_Next = stream.ReadU32();
    data.m_Unknown1 = stream.ReadU32();

    ReadCond( stream, aVer, data.m_InstRef16x );

    data.m_Unknown2 = stream.ReadU16();
    data.m_Unknown3 = stream.ReadU16();

    ReadCond( stream, aVer, data.m_Unknown );

    data.m_Flags = stream.ReadU32();

    data.m_Rotation = stream.ReadU32();
    data.m_CoordX = stream.ReadS16();
    data.m_CoordY = stream.ReadS16();

    ReadCond( stream, aVer, data.m_InstRef );

    data.m_UnknownPtr1 = stream.ReadU32();
    data.m_FirstPadPtr = stream.ReadU32();
    data.m_UnknownPtr2 = stream.ReadU32();
    ReadArrayU32( stream, data.m_UnknownPtrs1 );

    return block;
}


static std::unique_ptr<BLOCK_BASE> ParseBlock_0x33_VIA( FILE_STREAM& aStream, FMT_VER aVer )
{
    auto block = std::make_unique<BLOCK<BLK_0x33_VIA>>( 0x33, aStream.Position() );

    auto& data = block->GetData();

    aStream.Skip( 1 );

    data.m_LayerInfo = ParseLayerInfo( aStream );
    data.m_Key = aStream.ReadU32();
    data.m_Unknown1 = aStream.ReadU32();
    data.m_NetPtr = aStream.ReadU32();
    data.m_Unknown2 = aStream.ReadU32();

    ReadCond( aStream, aVer, data.m_Unknown3 );

    data.m_UnknownPtr1 = aStream.ReadU32();

    ReadCond( aStream, aVer, data.m_UnknownPtr2 );

    for( size_t i = 0; i < data.m_Coords.size(); ++i )
    {
        data.m_Coords[i] = aStream.ReadS32();
    }

    data.m_UnknownPtr3 = aStream.ReadU32();
    data.m_UnknownPtr4 = aStream.ReadU32();
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

            item.m_String = aStream.ReadStringFixed( 64 );
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
                item.m_Str = aStream.ReadStringFixed( 64 );
            else
                item.m_Str16x = aStream.ReadStringFixed( 32 );

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
            BLK_0x36::X08 item;

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
        data.m_FilmName = aStream.ReadStringFixed( 20 );
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

    aStream.Skip( 3 );

    data.m_Layer = ParseLayerInfo( aStream );
    data.m_Key = aStream.ReadU32();
    data.m_Next = aStream.ReadU32();
    data.m_Unknown = aStream.ReadU32();

    ReadCond( aStream, aVer, data.m_Unknown1 );

    return block;
}


static std::optional<uint32_t> GetBlockKey( const BLOCK_BASE& block )
{
    switch( block.GetBlockType() )
    {
    case 0x01: return static_cast<const BLOCK<BLK_0x01_ARC>&>( block ).GetData().m_Key;
    case 0x03: return static_cast<const BLOCK<BLK_0x03>&>( block ).GetData().m_Key;
    case 0x05: return static_cast<const BLOCK<BLK_0x05_TRACK>&>( block ).GetData().m_Key;
    case 0x06: return static_cast<const BLOCK<BLK_0x06>&>( block ).GetData().m_Key;
    case 0x07: return static_cast<const BLOCK<BLK_0x07>&>( block ).GetData().m_Key;
    case 0x09: return static_cast<const BLOCK<BLK_0x09>&>( block ).GetData().m_Key;
    case 0x0F: return static_cast<const BLOCK<BLK_0x0F>&>( block ).GetData().m_Key;
    case 0x10: return static_cast<const BLOCK<BLK_0x10>&>( block ).GetData().m_Key;
    case 0x15: return static_cast<const BLOCK<BLK_0x15_SEGMENT>&>( block ).GetData().m_Key;
    case 0x16: return static_cast<const BLOCK<BLK_0x16_SEGMENT>&>( block ).GetData().m_Key;
    case 0x17: return static_cast<const BLOCK<BLK_0x17_SEGMENT>&>( block ).GetData().m_Key;
    case 0x1B: return static_cast<const BLOCK<BLK_0x1B_NET>&>( block ).GetData().m_Key;
    case 0x1C: return static_cast<const BLOCK<BLK_0x1C_PADSTACK>&>( block ).GetData().m_Key;
    case 0x1D: return static_cast<const BLOCK<BLK_0x1D>&>( block ).GetData().m_Key;
    case 0x1F: return static_cast<const BLOCK<BLK_0x1F>&>( block ).GetData().m_Key;
    case 0x21: return static_cast<const BLOCK<BLK_0x21>&>( block ).GetData().m_Key;
    case 0x2A: return static_cast<const BLOCK<BLK_0x2A>&>( block ).GetData().m_Key;
    case 0x2B: return static_cast<const BLOCK<BLK_0x2B>&>( block ).GetData().m_Key;
    case 0x2D: return static_cast<const BLOCK<BLK_0x2D>&>( block ).GetData().m_Key;
    case 0x33: return static_cast<const BLOCK<BLK_0x33_VIA>&>( block ).GetData().m_Key;
    case 0x36: return static_cast<const BLOCK<BLK_0x36>&>( block ).GetData().m_Key;
    case 0x38: return static_cast<const BLOCK<BLK_0x38_FILM>&>( block ).GetData().m_Key;
    case 0x39: return static_cast<const BLOCK<BLK_0x39_FILM_LAYER_LIST>&>( block ).GetData().m_Key;
    case 0x3A: return static_cast<const BLOCK<TYPE_3A_FILM_LIST_NODE>&>( block ).GetData().m_Key;
    default: break;
    }

    return std::nullopt;
}


void ALLEGRO::PARSER::readObjects( RAW_BOARD& aBoard )
{
    const uint32_t magic = aBoard.m_Header->m_Magic;
    FMT_VER        ver = GetFormatVer( magic );

    while( !m_stream.Eof() )
    {
        size_t offset = m_stream.Position();

        // This seems to be always true and is quite useful for debugging oit-of-sync objects
        wxASSERT_MSG( offset % 4 == 0,
                      wxString::Format( "Allegro object at %#010lx, offset not aligned to 4 bytes", offset ) );

        // Read the type of the object
        uint8_t type = m_stream.ReadU8();

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
        case 0x09:
        {
            block = ParseBlock_0x09( m_stream, ver );
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
        case 0x15:
        {
            block = ParseBlock_0x15_SEGMENT( m_stream, ver );
            break;
        }
        case 0x16:
        {
            block = ParseBlock_0x16_SEGMENT( m_stream, ver );
            break;
        }
        case 0x17:
        {
            block = ParseBlock_0x17_SEGMENT( m_stream, ver );
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
        case 0x2D:
        {
            block = ParseBlock_0x2D( m_stream, ver );
            break;
        }
        case 0x33:
        {
            block = ParseBlock_0x33_VIA( m_stream, ver );
            break;
        }
        case 0x36:
        {
            block = ParseBlock_0x36( m_stream, ver );
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
        default:
        {
            if( !m_endAtUnknownBlock )
            {
                THROW_IO_ERROR( wxString::Format( "Do not have parser for type %#02x available at offset %#010lx", type,
                                                  offset ) );
            }

            wxLogTrace( traceAllegroParser,
                        wxString::Format( "Ending at unknown block type %#04x at offset %#010lx", type, offset ) );
            return;
        }
        }

        if( block )
        {
            wxLogTrace( traceAllegroParser, wxString::Format( "Added block type %#04x from %#010lx to %#010lx", type,
                                                              offset, m_stream.Position() ) );


            uint8_t type = block->GetBlockType();

            // Creates the vector if it doesn't exist
            aBoard.m_ObjectLists[type].push_back( block.get() );

            if( std::optional<uint32_t> blockKey = GetBlockKey( *block ) )
                aBoard.m_ObjectKeyMap[*blockKey] = block.get();

            aBoard.m_Objects.push_back( std::move( block ) );
        }
    }
}


std::unique_ptr<RAW_BOARD> ALLEGRO::PARSER::Parse()
{
    std::unique_ptr<RAW_BOARD> board = std::make_unique<RAW_BOARD>();

    try
    {
        board->m_Header = ReadHeader( m_stream );

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

    return board;
}
