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
    m_payloadOffset = 0;
    m_footerOffset = 0;

    parseHeader();
    parseDirectory();
    verifyFooter();
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

    try
    {
        PADS_IO::ValidateSdbFooter( m_data, m_footerOffset, FOOTER_GUID, FOOTER_GUID_SIZE );
    }
    catch( const IO_ERROR& error )
    {
        size_t failingOffset = m_footerOffset;

        if( std::memcmp( m_data.data() + m_footerOffset, FOOTER_GUID, FOOTER_GUID_SIZE ) == 0 )
            failingOffset += FOOTER_GUID_SIZE;

        throwAt( failingOffset, error.What() );
    }
}


void PADS_SCH_SDB::throwAt( size_t aOffset, const wxString& aDetail ) const
{
    THROW_IO_ERROR(
            wxString::Format( "PADS Logic binary v0x%04X at offset 0x%zX: %s", m_version, aOffset, aDetail.c_str() ) );
}

} // namespace PADS_SCH_BINARY
