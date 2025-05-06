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
    else if constexpr( std::is_same_v<T, std::array<uint32_t, 28>> )
    {
        for( size_t i = 0; i < field.size(); ++i )
        {
            field[i] = aStream.ReadU32();
        }
    }
    else if constexpr( std::is_same_v<T, std::array<uint32_t, 8>> )
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


static std::optional<uint32_t> GetBlockKey( const BLOCK_BASE& block )
{
    switch( block.GetBlockType() )
    {
    case 0x06: return static_cast<const BLOCK<BLK_0x06>&>( block ).GetData().m_Key;
    case 0x07: return static_cast<const BLOCK<BLK_0x07>&>( block ).GetData().m_Key;
    case 0x0F: return static_cast<const BLOCK<BLK_0x0F>&>( block ).GetData().m_Key;
    case 0x10: return static_cast<const BLOCK<BLK_0x10>&>( block ).GetData().m_Key;
    case 0x1B: return static_cast<const BLOCK<BLK_0x1B_NET>&>( block ).GetData().m_Key;
    case 0x1c: return static_cast<const BLOCK<BLK_0x1C_PADSTACK>&>( block ).GetData().m_Key;
    case 0x2B: return static_cast<const BLOCK<BLK_0x2B>&>( block ).GetData().m_Key;
    case 0x2D: return static_cast<const BLOCK<BLK_0x2B>&>( block ).GetData().m_Key;
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
        default:
        {
            if( !m_endAtUnknownBlock )
            {
                THROW_IO_ERROR( wxString::Format( "Do not have parser for type %#02x available at offset %#010lx", type,
                                                  offset ) );
            }

            wxLogTrace( traceAllegroParser,
                        wxString::Format( "Ending at unknown block type %#02x at offset %#010lx", type, offset ) );
            return;
        }
        }

        if( block )
        {
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
