/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "pads_sch_sdb.h"

#include <cstring>

#include <ki_exception.h>

namespace PADS_SCH_BINARY
{

namespace
{

    constexpr uint8_t  MAGIC1 = 0xFE;
    constexpr uint16_t VERSION_12 = 0x000C;
    constexpr uint16_t VERSION_13 = 0x000D;
    constexpr size_t   HEADER_SIZE = 0x20;
    constexpr size_t   POOL_COUNT = 20;
    constexpr size_t   POOL_SIZE = 28;
    constexpr size_t   POOL_OFFSET = HEADER_SIZE;
    constexpr size_t   PAYLOAD_OFFSET = POOL_OFFSET + POOL_COUNT * POOL_SIZE;
    constexpr size_t   FOOTER_SIZE = 42;
    constexpr char     FOOTER_GUID[] = "{F4997D70-AF8A-11D0-A373-000000000000}";
    constexpr size_t   FOOTER_GUID_SIZE = 38;
    constexpr uint32_t DATABASE_CONTROLLER = 0x24;
    constexpr size_t   SHEET_HEADER_SIZE = 20;
    constexpr size_t   SHEET_POOL_COUNT = 24;
    constexpr size_t   SHEET_POOL_SIZE = 28;
    constexpr size_t   SHEET_SERIALIZED_POOL_COUNT = 23;
    constexpr size_t   SHEET_POOL_COUNT_OFFSET = 12;
    constexpr size_t   SHEET_POOL_USED_BYTES_OFFSET = 16;
    constexpr size_t   SHEET_POOL_ALLOCATED_BYTES_OFFSET = 8;
    constexpr size_t   FIRST_OLE_FIXED_STATE_SIZE = 18;
    constexpr size_t   BETWEEN_OLE_TRAILER_SIZE = 0x66;
    constexpr size_t   FINAL_OLE_TRAILER_SIZE = 0x4E;
    constexpr size_t   CFB_HEADER_SIZE = 512;
    constexpr uint8_t  MFC_CLASS_MARKER[] = { 0xFF, 0xFF, 0x01, 0x00 };
    constexpr char     OLE_ITEM_CLASS_NAME[] = "CPowerPCBCntrItem";
    constexpr uint8_t  CFB_MAGIC[] = { 0xD0, 0xCF, 0x11, 0xE0, 0xA1, 0xB1, 0x1A, 0xE1 };
    constexpr uint8_t  CFB_ZERO_CLSID[16] = {};
    constexpr uint8_t  CFB_BYTE_ORDER[] = { 0xFE, 0xFF };
    constexpr uint8_t  CFB_ZERO_PADDING[6] = {};

    constexpr size_t POOL_ALLOCATED_BYTES_OFFSET = 4;
    constexpr size_t POOL_COUNT_OFFSET = 8;
    constexpr size_t POOL_USED_BYTES_OFFSET = 12;
    constexpr size_t POOL_HANDLE_OFFSET = 16;

} // namespace


bool PADS_SCH_SDB::HasFamilyMagic( const std::vector<uint8_t>& aBytes )
{
    return PADS_IO::HasSdbMagic( aBytes, MAGIC1 );
}


bool PADS_SCH_SDB::IsSupportedVersion( uint16_t aVersion )
{
    return aVersion == VERSION_12 || aVersion == VERSION_13;
}


void PADS_SCH_SDB::Load( std::vector<uint8_t> aBytes )
{
    m_data = std::move( aBytes );
    m_version = m_data.size() >= 4 ? m_cursor.U16At( 2 ) : 0;
    m_pools = {};
    m_blocks.clear();
    m_oleItems.clear();
    m_payloadOffset = 0;
    m_footerOffset = 0;

    parseHeader();
    parseDirectory();
    verifyFooter();
    parseBlocks();
}


void PADS_SCH_SDB::parseHeader()
{
    if( m_data.size() < 2 )
        throwAt( m_data.size(), "file too small for PADS Logic binary magic" );

    if( m_data[0] != 0x00 )
        throwAt( 0, "invalid PADS Logic binary magic" );

    if( m_data[1] != MAGIC1 )
        throwAt( 1, "invalid PADS Logic binary magic" );

    if( m_data.size() < 4 )
        throwAt( m_data.size(), "file too small for PADS Logic binary version" );

    if( !IsSupportedVersion( m_version ) )
        throwAt( 2, "unsupported PADS Logic binary version" );

    if( m_data.size() < HEADER_SIZE )
        throwAt( m_data.size(), "file too small for PADS Logic binary header" );

    uint16_t expectedRevision = m_version == VERSION_12 ? 1 : 0;

    if( m_cursor.U16At( 4 ) != expectedRevision )
        throwAt( 4, "invalid version-specific PADS Logic format flags" );

    m_payloadOffset = PAYLOAD_OFFSET;
}


void PADS_SCH_SDB::parseDirectory()
{
    if( m_data.size() < PAYLOAD_OFFSET )
        throwAt( m_data.size(), "file too small for PADS Logic pool directory" );

    for( size_t i = 0; i < POOL_COUNT; ++i )
    {
        size_t              offset = POOL_OFFSET + i * POOL_SIZE;
        PADS_IO::SDB_RECORD record( m_cursor, offset );
        SCH_SDB_POOL&       pool = m_pools[i];

        pool.allocatedBytes = record.U32( POOL_ALLOCATED_BYTES_OFFSET );
        pool.count = record.U32( POOL_COUNT_OFFSET );
        pool.usedBytes = record.U32( POOL_USED_BYTES_OFFSET );
        pool.handle = record.U32( POOL_HANDLE_OFFSET );

        if( pool.usedBytes > pool.allocatedBytes )
            throwAt( offset + POOL_USED_BYTES_OFFSET, "pool serialized bytes exceed allocation" );

        if( pool.count == 0 && pool.usedBytes != 0 )
            throwAt( offset + POOL_USED_BYTES_OFFSET, "empty pool has serialized bytes" );
    }
}


void PADS_SCH_SDB::verifyFooter()
{
    if( m_data.size() < PAYLOAD_OFFSET + FOOTER_SIZE )
        throwAt( m_data.size(), "file too small for PADS Logic binary footer" );

    m_footerOffset = m_data.size() - FOOTER_SIZE;

    if( std::optional<PADS_IO::SDB_FOOTER_ERROR> error =
                PADS_IO::CheckSdbFooter( m_data, m_footerOffset, FOOTER_GUID, FOOTER_GUID_SIZE ) )
    {
        throwAt( error->offset, error->detail );
    }

    size_t backPointerOffset = m_footerOffset + FOOTER_GUID_SIZE;

    if( m_cursor.U32At( backPointerOffset ) < m_payloadOffset )
        throwAt( backPointerOffset, "container-item back-pointer precedes schematic payload" );
}


void PADS_SCH_SDB::parseBlocks()
{
    auto checkedAdd = [this]( size_t aBase, size_t aBytes, size_t aSourceOffset, const wxString& aDetail )
    {
        if( aBase > m_footerOffset || aBytes > m_footerOffset - aBase )
            throwAt( aSourceOffset, aDetail );

        return aBase + aBytes;
    };

    auto requireBytes = [this]( size_t aOffset, const auto& aExpected, size_t aBytes, const wxString& aDetail )
    {
        for( size_t i = 0; i < aBytes; ++i )
        {
            if( m_data[aOffset + i] != static_cast<uint8_t>( aExpected[i] ) )
                throwAt( aOffset + i, aDetail );
        }
    };

    size_t offset = m_payloadOffset;

    if( m_cursor.U32At( offset ) != DATABASE_CONTROLLER )
        throwAt( offset, "invalid first schematic controller ordinal" );

    size_t outerStart = offset;
    size_t outerEnd = checkedAdd( offset, 4, offset, "controller ordinal extends past footer" );

    for( size_t i = 1; i < m_pools.size(); ++i )
    {
        size_t source = POOL_OFFSET + i * POOL_SIZE + POOL_USED_BYTES_OFFSET;
        size_t blockEnd =
                checkedAdd( outerEnd, m_pools[i].usedBytes, source, "controller payload extends past footer" );

        outerEnd = blockEnd;
    }

    m_blocks.push_back( { SCH_SDB_BLOCK_KIND::FIXED_CONTROLLER, static_cast<int>( DATABASE_CONTROLLER ), outerStart,
                          outerEnd - outerStart, 19, 0 } );

    offset = outerEnd;

    uint32_t         sheetCount = m_pools[3].count;
    constexpr size_t minimumSheetBytes = SHEET_HEADER_SIZE + SHEET_POOL_COUNT * SHEET_POOL_SIZE;
    size_t           sheetCountOffset = POOL_OFFSET + 3 * POOL_SIZE + POOL_COUNT_OFFSET;

    if( sheetCount > ( m_footerOffset - offset ) / minimumSheetBytes )
        throwAt( sheetCountOffset, "sheet count extent exceeds schematic payload" );

    for( uint32_t sheet = 0; sheet < sheetCount; ++sheet )
    {
        size_t sheetStart = offset;
        size_t descriptors =
                checkedAdd( sheetStart, SHEET_HEADER_SIZE, sheetStart, "sheet header extends past footer" );

        if( m_cursor.U32At( sheetStart ) != SHEET_HEADER_SIZE || m_cursor.U32At( sheetStart + 4 ) != 5
            || m_cursor.U32At( sheetStart + 8 ) != SHEET_HEADER_SIZE )
        {
            throwAt( sheetStart, "invalid sheet database header" );
        }

        offset = checkedAdd( descriptors, SHEET_POOL_COUNT * SHEET_POOL_SIZE, descriptors,
                             "sheet controller directory extends past footer" );

        for( size_t i = 0; i < SHEET_SERIALIZED_POOL_COUNT; ++i )
        {
            size_t   descriptor = descriptors + i * SHEET_POOL_SIZE;
            uint32_t allocatedBytes = m_cursor.U32At( descriptor + SHEET_POOL_ALLOCATED_BYTES_OFFSET );
            uint32_t count = m_cursor.U32At( descriptor + SHEET_POOL_COUNT_OFFSET );
            uint32_t usedBytes = m_cursor.U32At( descriptor + SHEET_POOL_USED_BYTES_OFFSET );

            if( usedBytes > allocatedBytes )
                throwAt( descriptor + SHEET_POOL_USED_BYTES_OFFSET, "sheet pool serialized bytes exceed allocation" );

            if( count == 0 && usedBytes != 0 )
                throwAt( descriptor + SHEET_POOL_USED_BYTES_OFFSET, "empty sheet pool has serialized bytes" );

            offset = checkedAdd( offset, usedBytes, descriptor + SHEET_POOL_USED_BYTES_OFFSET,
                                 "sheet controller payload extends past footer" );
        }

        m_blocks.push_back( { SCH_SDB_BLOCK_KIND::SHEET, static_cast<int>( sheet ), sheetStart, offset - sheetStart,
                              SHEET_SERIALIZED_POOL_COUNT, 0 } );
    }

    size_t backPointerOffset = m_footerOffset + FOOTER_GUID_SIZE;

    if( m_cursor.U32At( backPointerOffset ) != offset )
        throwAt( backPointerOffset, "container-item back-pointer overlaps derived schematic blocks" );

    uint32_t containerItemCount = m_cursor.U32At( offset );
    size_t   containerItemCountOffset = offset;
    offset = checkedAdd( offset, 4, containerItemCountOffset, "embedded OLE item count extends past footer" );
    m_blocks.push_back( { SCH_SDB_BLOCK_KIND::FOOTER_AUX, -1, containerItemCountOffset, 4, containerItemCount, 4 } );

    if( containerItemCount == 0 )
    {
        if( offset != m_footerOffset )
            throwAt( offset, "unowned bytes before schematic footer" );

        return;
    }

    constexpr size_t firstItemFrameSize = 6 + FIRST_OLE_FIXED_STATE_SIZE + 4;
    constexpr size_t finalItemMinimum = firstItemFrameSize + FINAL_OLE_TRAILER_SIZE;
    size_t           remainingItemBytes = m_footerOffset - offset;
    uint64_t minimumItemBytes = finalItemMinimum + uint64_t( containerItemCount - 1 ) * BETWEEN_OLE_TRAILER_SIZE;

    // A u32 item count times the 0x66-byte trailer is bounded well below uint64_t.
    if( minimumItemBytes > remainingItemBytes )
    {
        throwAt( containerItemCountOffset, "embedded OLE item count exceeds schematic payload" );
    }

    size_t itemStart = offset;
    requireBytes( itemStart, MFC_CLASS_MARKER, std::size( MFC_CLASS_MARKER ), "invalid embedded OLE MFC class marker" );
    uint16_t classNameLength = m_cursor.U16At( itemStart + 4 );

    if( classNameLength != std::size( OLE_ITEM_CLASS_NAME ) - 1 )
        throwAt( itemStart + 4, "invalid embedded OLE MFC class name length" );

    size_t lengthOffset = checkedAdd( itemStart, 6, itemStart, "OLE item class header extends past footer" );
    lengthOffset =
            checkedAdd( lengthOffset, classNameLength, itemStart + 4, "OLE item class name extends past footer" );
    requireBytes( itemStart + 6, OLE_ITEM_CLASS_NAME, classNameLength, "invalid embedded OLE MFC class name" );
    lengthOffset =
            checkedAdd( lengthOffset, FIRST_OLE_FIXED_STATE_SIZE, lengthOffset, "OLE item state extends past footer" );
    size_t firstCfbOffset = checkedAdd( lengthOffset, 4, lengthOffset, "OLE item length extends past footer" );
    m_blocks.push_back( { SCH_SDB_BLOCK_KIND::FOOTER_AUX, -1, itemStart, firstCfbOffset - itemStart, 1, 0 } );

    for( uint32_t item = 0; item < containerItemCount; ++item )
    {
        uint32_t cfbBytes = m_cursor.U32At( lengthOffset );

        if( cfbBytes < CFB_HEADER_SIZE )
            throwAt( lengthOffset, "embedded OLE CFB is smaller than its structural header" );

        size_t cfbOffset = checkedAdd( lengthOffset, 4, lengthOffset, "OLE item length extends past footer" );
        size_t cfbEnd = checkedAdd( cfbOffset, cfbBytes, lengthOffset, "OLE item CFB extends past footer" );
        requireBytes( cfbOffset, CFB_MAGIC, std::size( CFB_MAGIC ), "invalid embedded OLE CFB magic" );
        requireBytes( cfbOffset + 8, CFB_ZERO_CLSID, std::size( CFB_ZERO_CLSID ),
                      "invalid embedded OLE CFB reserved class ID" );
        requireBytes( cfbOffset + 28, CFB_BYTE_ORDER, std::size( CFB_BYTE_ORDER ),
                      "invalid embedded OLE CFB byte order" );
        requireBytes( cfbOffset + 34, CFB_ZERO_PADDING, std::size( CFB_ZERO_PADDING ),
                      "invalid embedded OLE CFB header padding" );
        size_t trailer = item + 1 == containerItemCount ? FINAL_OLE_TRAILER_SIZE : BETWEEN_OLE_TRAILER_SIZE;
        size_t itemEnd = checkedAdd( cfbEnd, trailer, cfbEnd, "OLE item trailer extends past footer" );
        requireBytes( cfbEnd, FOOTER_GUID, FOOTER_GUID_SIZE, "invalid embedded OLE trailer class ID" );

        SCH_SDB_BLOCK cfbBlock{
            SCH_SDB_BLOCK_KIND::CFB_CONTAINER_ITEM, static_cast<int>( item ), cfbOffset, cfbBytes, 1, 0
        };
        m_blocks.push_back( cfbBlock );
        m_blocks.push_back( { SCH_SDB_BLOCK_KIND::FOOTER_AUX, -1, cfbEnd, trailer, 1, 0 } );

        constexpr size_t extentOffsetInTrailer = FOOTER_GUID_SIZE;
        constexpr size_t boxOffsetInTrailer = extentOffsetInTrailer + 16;
        constexpr size_t sheetPlaneOffsetInTrailer = boxOffsetInTrailer + 16;
        constexpr size_t flagsOffsetInTrailer = sheetPlaneOffsetInTrailer + 4;
        size_t           extentOffset = cfbEnd + extentOffsetInTrailer;
        size_t           boxOffset = cfbEnd + boxOffsetInTrailer;
        SCH_SDB_OLE_ITEM oleItem;
        oleItem.cfb = cfbBlock;
        oleItem.extent = { m_cursor.I32At( extentOffset ), m_cursor.I32At( extentOffset + 4 ),
                           m_cursor.I32At( extentOffset + 8 ), m_cursor.I32At( extentOffset + 12 ) };
        oleItem.left = m_cursor.I32At( boxOffset );
        oleItem.bottom = m_cursor.I32At( boxOffset + 4 );
        oleItem.right = m_cursor.I32At( boxOffset + 8 );
        oleItem.top = m_cursor.I32At( boxOffset + 12 );
        oleItem.sheetPlane = m_cursor.U32At( cfbEnd + sheetPlaneOffsetInTrailer );
        oleItem.flags = m_cursor.U32At( cfbEnd + flagsOffsetInTrailer );

        if( oleItem.sheetPlane >= sheetCount )
            throwAt( cfbEnd + sheetPlaneOffsetInTrailer, "embedded OLE item sheet index is out of range" );

        oleItem.trailerOffset = cfbEnd;
        oleItem.extentOffset = extentOffset;
        oleItem.boxOffset = boxOffset;
        m_oleItems.push_back( oleItem );

        if( item + 1 < containerItemCount )
            lengthOffset = itemEnd - 4;

        offset = itemEnd;
    }

    if( offset != m_footerOffset )
        throwAt( offset, "embedded OLE items do not end at schematic footer" );
}


void PADS_SCH_SDB::throwAt( size_t aOffset, const wxString& aDetail ) const
{
    THROW_IO_ERROR(
            wxString::Format( "PADS Logic binary v0x%04X at offset 0x%zX: %s", m_version, aOffset, aDetail.c_str() ) );
}

} // namespace PADS_SCH_BINARY
