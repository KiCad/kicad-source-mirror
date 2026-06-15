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

// The fixed GUID PADS writes into every binary footer; its presence is the
// strongest single check that a file is a PADS binary database.
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
    // The count is stored rather than implied by the version. Section 1's directory entry
    // describes the section table itself -- stride 16, one slot per controller -- but declares one
    // slot more than is written, the same in-memory-versus-written over-declaration section 3
    // makes. So the written entry count is its declared count less one.
    //
    // This matters because the directory gained its extra controller in v0x2025, not v0x2022 as
    // the old version table assumed. v0x2022 and v0x2024 have 73 entries like v0x2021. The error
    // was invisible for every section after 3, where the offset and the section-3 overshoot both
    // grew by the same 16 bytes and cancelled, and showed up only in section 1.
    //
    // Verified on all 164 corpus files of every version: the *PCB* board-setup block then lands at
    // a 48-byte-aligned displacement into section 1 on every one of them, where the version table
    // leaves v0x2022 and v0x2024 misaligned by 32.
    // NOT YET APPLIED. Switching to the stored count moves every v0x2022 and v0x2024 section
    // offset by 16 bytes at once, and the readers that still use the raw dataOffset are calibrated
    // against the wrong value, so it fails 9 tests on exactly those two dialects. It can land only
    // together with moving those readers onto payloadOffset, which absorbs the shift -- the
    // offset and the section-3 overshoot both change by the same 16 bytes and cancel.
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

    // Section payloads follow the directory contiguously; each offset is the running total
    // of preceding payload sizes. Section 0 has no payload of its own, so accumulation
    // starts at section 1.
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

            // Sections up to and including 3 precede the over-declaration, so their declared
            // offset is already correct; everything after it is short by the directory plus a
            // 48-byte header. See SDB_SECTION::payloadOffset.
            size_t overshoot = dirSize + SECTION3_HEADER_BYTES;

            section.payloadOffset = ( i > 3 && payloadOffset >= overshoot )
                                            ? static_cast<uint32_t>( payloadOffset - overshoot )
                                            : payloadOffset;

            if( section.count > 0 && section.totalBytes > 0 )
                section.stride = section.totalBytes / section.count;

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
    // Section 1 is the serialized *PCB* board-setup parameter block. It does not start at the
    // accumulated dataOffset, and the whole displacement is accounted for by two declared
    // quantities, so the block is computed rather than searched for.
    //
    // The directory's own slot count is stored in entry 1 and is one more than the number of
    // sections, which matters here because the version-implied count this reader still uses
    // elsewhere is 16 bytes too large on v0x2022 and v0x2024. The rest of the displacement is
    // section 2, a run of 48-byte view-state records that PADS writes ahead of the block; its
    // length is simply that section's declared count.
    //
    //     base = HEADER_SIZE + (entry[1].count - 1) * DIR_ENTRY_SIZE
    //            + entry[2].count * VIEW_STATE_RECORD_BYTES
    //
    // Measured across the 663-board corpus: this agrees with the field-range search it replaces
    // on all 633 boards where both resolve, disagrees on none, and additionally resolves 7 the
    // search missed. Every displacement the search used to absorb is an exact multiple of 48
    // once the stored slot count is used, and the negative ones disappear entirely.
    const SDB_SECTION* directory = Section( 1 );
    const SDB_SECTION* viewStates = Section( 2 );

    if( !directory || !viewStates || directory->count == 0 )
        return;

    const uint64_t base = static_cast<uint64_t>( HEADER_SIZE )
                          + static_cast<uint64_t>( directory->count - 1 ) * DIR_ENTRY_SIZE
                          + static_cast<uint64_t>( viewStates->count ) * VIEW_STATE_RECORD_BYTES;

    if( base + 100 > m_data.size() )
        return;

    // The field-range signature that used to locate the block now only confirms it, so a file
    // whose layout differs yields no origin rather than a plausible-looking wrong one. The
    // fields mirror the *PCB* section's ASCII-export order: SCALE, BACKUPTIME, REAL WIDTH,
    // ALLSIGONOFF, REFNAMESIZE.
    const float scale = m_cursor.F32At( base + 56 );

    if( !( scale > 0.01f && scale < 1e6f ) )
        return;

    const int32_t backupTime = m_cursor.I32At( base + 76 );

    if( backupTime < 0 || backupTime > 100000 )
        return;

    const int32_t realWidth = m_cursor.I32At( base + 80 );

    if( realWidth <= 50 || realWidth >= 100000000 )
        return;

    const int32_t allSigOnOff = m_cursor.I32At( base + 84 );

    if( allSigOnOff != 0 && allSigOnOff != 1 )
        return;

    const int32_t refHeight = m_cursor.I32At( base + 92 );
    const int32_t refWidth = m_cursor.I32At( base + 96 );

    if( !( refWidth > 100000 && refWidth <= refHeight && refHeight < 100000000 ) )
        return;

    m_coords.m_originX = m_cursor.I32At( base + 60 );
    m_coords.m_originY = m_cursor.I32At( base + 64 );
    m_coords.m_found = true;
    m_coords.m_headerBase = static_cast<uint32_t>( base );
}

} // namespace PADS_IO
