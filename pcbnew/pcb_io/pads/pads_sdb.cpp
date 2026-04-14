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

#include "pads_sdb.h"

#include <ki_exception.h>

namespace PADS_IO
{

// The fixed GUID PADS writes into every binary footer (38 chars, no terminator
// compared); its presence is the strongest single check that a file is a PADS
// binary database.
static const char       FOOTER_GUID[] = "{2FE18320-6448-11d1-A412-000000000000}";
static constexpr size_t FOOTER_GUID_LEN = 38;


bool PADS_SDB::HasMagic( const std::vector<uint8_t>& aBytes )
{
    return HasSdbMagic( aBytes, MAGIC1 );
}


bool PADS_SDB::IsSupportedVersion( uint16_t aVersion )
{
    switch( aVersion )
    {
    case 0x2021:
    case 0x2022:
    case 0x2024:
    case 0x2025:
    case 0x2026:
    case 0x2027: return true;
    default:     return false;
    }
}


void PADS_SDB::Load( std::vector<uint8_t> aBytes )
{
    m_data = std::move( aBytes );

    parseHeader();
    verifyFooter();
    parseDirectory();
    locateOrigin();
}


int PADS_SDB::directoryEntryCount() const
{
    // The directory gained one controller (74 entries) in v0x2022; v0x2021 has 73.
    return m_version == 0x2021 ? 73 : 74;
}


void PADS_SDB::parseHeader()
{
    if( m_data.size() < static_cast<size_t>( HEADER_SIZE ) + FOOTER_SIZE )
        THROW_IO_ERROR( "File too small for PADS binary format" );

    if( !HasMagic( m_data ) )
        THROW_IO_ERROR( "Invalid PADS binary magic bytes" );

    m_version = m_cursor.U16At( 2 );

    if( !IsSupportedVersion( m_version ) )
        THROW_IO_ERROR( wxString::Format( "Unsupported PADS binary version 0x%04X", m_version ) );
}


void PADS_SDB::verifyFooter() const
{
    ValidateSdbFooter( m_data, m_data.size() - FOOTER_SIZE, FOOTER_GUID, FOOTER_GUID_LEN );
}


void PADS_SDB::parseDirectory()
{
    int    entryCount = directoryEntryCount();
    size_t dirSize = static_cast<size_t>( entryCount ) * DIR_ENTRY_SIZE;

    if( HEADER_SIZE + dirSize > m_data.size() )
        THROW_IO_ERROR( "File too small for the PADS section directory" );

    // Section payloads follow the directory contiguously; each section's offset is the
    // running total of the preceding payload sizes. Section 0 is a pre-sizing header
    // with no payload region of its own, so payload accumulation starts at section 1.
    uint32_t payloadOffset = static_cast<uint32_t>( HEADER_SIZE + dirSize );

    m_sections.clear();
    m_sections.reserve( entryCount );

    for( int i = 0; i < entryCount; ++i )
    {
        size_t entryOffset = HEADER_SIZE + static_cast<size_t>( i ) * DIR_ENTRY_SIZE;

        SDB_SECTION section;
        section.index = i;
        section.count = m_cursor.U32At( entryOffset );
        section.totalBytes = m_cursor.U32At( entryOffset + 4 );

        if( i > 0 )
        {
            section.dataOffset = payloadOffset;

            if( section.count > 0 && section.totalBytes > 0 )
                section.perItem = section.totalBytes / section.count;

            payloadOffset += section.totalBytes;
        }

        m_sections.push_back( section );
    }
}


const SDB_SECTION* PADS_SDB::Section( int aIndex ) const
{
    if( aIndex >= 0 && aIndex < static_cast<int>( m_sections.size() ) )
        return &m_sections[aIndex];

    return nullptr;
}


void PADS_SDB::locateOrigin()
{
    const SDB_SECTION* setup = Section( 1 );

    if( !setup || setup->totalBytes < 68 || setup->End() > m_data.size() )
        return;

    // The per-axis origin is the i32 pair at section-1 offset +60/+64 on the supported
    // v0x2025..v0x2027 boards and the v0x2021 boards exercised by the importer.
    //
    // A more general "SCALE-anchored" locator (the i32 pair immediately after the f32
    // SCALE field) is known to be correct for v0x2024 and the view-array section-1
    // variant where this fixed offset is wrong, but it false-positives on boards whose
    // true origin is (0, 0); adopting it needs its own route-coordinate validation, so
    // it is deferred rather than risk shifting coordinates on the boards that work today.
    m_coords.m_originX = m_cursor.I32At( setup->dataOffset + 60 );
    m_coords.m_originY = m_cursor.I32At( setup->dataOffset + 64 );
    m_coords.m_found = true;
}

} // namespace PADS_IO
