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
    case 0x2017:
    case 0x2019:
    case 0x2021:
    case 0x2022:
    case 0x2024:
    case 0x2025:
    case 0x2026:
    case 0x2027: return true;
    default: return false;
    }
}


void PADS_SDB::Load( std::vector<uint8_t> aBytes )
{
    m_data = std::move( aBytes );

    parseHeader();
    verifyFooter();
    parseDirectory();
    parsePhysicalFraming();
    readBoardSetup();
}


int PADS_SDB::directoryEntryCount() const
{
    return static_cast<int>( m_cursor.U32At( HEADER_SIZE + DIR_ENTRY_SIZE ) );
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

    m_sections.clear();
    m_sections.reserve( entryCount );

    for( int i = 0; i < entryCount; ++i )
    {
        size_t entryOffset = HEADER_SIZE + static_cast<size_t>( i ) * DIR_ENTRY_SIZE;

        SDB_SECTION section;
        section.index = i;
        section.count = m_cursor.U32At( entryOffset );
        section.totalBytes = m_cursor.U32At( entryOffset + 4 );

        if( section.count > 0 && section.totalBytes > 0 )
            section.stride = section.totalBytes / section.count;

        m_sections.push_back( section );
    }
}


void PADS_SDB::parsePhysicalFraming()
{
    static constexpr int PAGE_DESCRIPTOR_BYTES = 12;
    static constexpr int PAGE_TABLE_TAGS[] = { 65, 66, 45, 46, 47, 48, 41, 42, 74 };
    static constexpr int PAGED_READ_ORDER[] = { 41, 42, 45, 46, 47, 48 };

    auto recordStride = [this]( int aTag ) -> uint32_t
    {
        switch( aTag )
        {
        case 41: return m_version == 0x2017 ? 180 : 188;
        case 42: return 80;
        case 45: return m_version == 0x2017 ? 116 : 124;
        case 46: return m_version <= 0x2019 ? 32 : 40;
        case 47: return 24;
        case 48:
            if( m_version <= 0x2019 )
                return 48;

            return m_version <= 0x2022 ? 856 : 864;

        case 65: return 28;
        case 66: return m_version <= 0x2022 ? 28 : 280;
        case 74: return 276;

        default: return 0;
        }
    };

    uint64_t cursor = 6 + static_cast<uint64_t>( m_sections.size() ) * DIR_ENTRY_SIZE;

    for( int tag = 2; tag <= 25; ++tag )
    {
        SDB_SECTION& section = m_sections.at( tag );

        uint32_t fixedStride = 0;

        if( tag == 10 )
            fixedStride = m_version <= 0x2022 ? 100 : 112;
        else if( tag == 11 )
            fixedStride = m_version <= 0x2024 ? 16 : 20;
        else if( tag == 12 )
            fixedStride = 12;
        else if( tag == 24 )
            fixedStride = 68;

        if( fixedStride != 0 && static_cast<uint64_t>( section.count ) * fixedStride != section.totalBytes )
            THROW_IO_ERROR( "Invalid PADS flat-controller extent" );

        if( tag == 15 )
        {
            const uint32_t stride = m_version <= 0x2019 ? 20 : 36;

            if( static_cast<uint64_t>( section.count ) * stride != section.totalBytes )
                THROW_IO_ERROR( "Invalid PADS terminal-controller extent" );
        }

        section.physicalOffset = static_cast<uint32_t>( cursor );
        section.physicalBytes = section.totalBytes;
        section.physicalCount = section.count;
        section.physicalLiveCount = section.count;
        cursor += section.totalBytes;
    }

    for( int tag : { 26, 27 } )
    {
        SDB_SECTION& section = m_sections.at( tag );
        section.physicalOffset = static_cast<uint32_t>( cursor );
        section.physicalBytes = section.totalBytes;
        section.physicalCount = section.count;
        section.physicalLiveCount = section.count;
        cursor += section.totalBytes;
    }

    SDB_SECTION& pageDirectory = m_sections.at( 26 );
    uint64_t     numPageDescriptors = 0;

    for( int tag : PAGE_TABLE_TAGS )
    {
        if( tag < static_cast<int>( m_sections.size() ) )
            numPageDescriptors += m_sections[tag].totalBytes;
    }

    if( pageDirectory.totalBytes != static_cast<uint64_t>( pageDirectory.count ) * PAGE_DESCRIPTOR_BYTES
        || numPageDescriptors > pageDirectory.count )
    {
        THROW_IO_ERROR( "Invalid PADS page-controller directory" );
    }

    uint64_t descriptor =
            pageDirectory.physicalOffset + ( pageDirectory.count - numPageDescriptors ) * PAGE_DESCRIPTOR_BYTES;

    for( int tag : PAGE_TABLE_TAGS )
    {
        if( tag >= static_cast<int>( m_sections.size() ) )
            continue;

        SDB_SECTION& section = m_sections[tag];
        uint64_t     records = 0;

        for( uint32_t page = 0; page < section.totalBytes; ++page )
        {
            if( descriptor + PAGE_DESCRIPTOR_BYTES > m_data.size() )
                THROW_IO_ERROR( "Truncated PADS page-controller directory" );

            records += m_cursor.U32At( descriptor + 8 );
            descriptor += PAGE_DESCRIPTOR_BYTES;
        }

        if( records > UINT32_MAX )
            THROW_IO_ERROR( "Invalid PADS page-controller record count" );

        section.physicalCount = static_cast<uint32_t>( records );
        section.physicalLiveCount = static_cast<uint32_t>( records );
    }

    SDB_SECTION& ordinals = m_sections.at( 29 );
    ordinals.physicalOffset = static_cast<uint32_t>( cursor );
    ordinals.physicalBytes = ordinals.totalBytes;
    ordinals.physicalCount = ordinals.count;
    ordinals.physicalLiveCount = ordinals.count;
    cursor += ordinals.totalBytes;

    for( int tag : PAGED_READ_ORDER )
    {
        SDB_SECTION& section = m_sections.at( tag );
        uint64_t     bytes = static_cast<uint64_t>( section.physicalCount ) * recordStride( tag );

        if( cursor + bytes > m_data.size() || bytes > UINT32_MAX )
            THROW_IO_ERROR( "Invalid PADS paged-controller extent" );

        section.physicalOffset = static_cast<uint32_t>( cursor );
        section.physicalBytes = static_cast<uint32_t>( bytes );
        cursor += bytes;
    }

    SDB_SECTION& routeRules = m_sections.at( 46 );
    uint32_t     liveRouteRules = 0;
    uint32_t     routeRuleStride = recordStride( 46 );

    for( uint32_t index = 0; index < routeRules.physicalCount; ++index )
    {
        uint64_t offset =
                static_cast<uint64_t>( routeRules.physicalOffset ) + static_cast<uint64_t>( index ) * routeRuleStride;

        if( m_cursor.U32At( offset ) < 0x80000000 && m_cursor.U32At( offset + 4 ) != 0 )
            ++liveRouteRules;
    }

    routeRules.physicalLiveCount = liveRouteRules;

    SDB_SECTION& relationships = m_sections.at( 49 );

    if( cursor + relationships.totalBytes > m_data.size() - FOOTER_SIZE )
        THROW_IO_ERROR( "Invalid PADS relationship-controller extent" );

    relationships.physicalOffset = static_cast<uint32_t>( cursor );
    relationships.physicalBytes = relationships.totalBytes;
    relationships.physicalCount = relationships.count;
    relationships.physicalLiveCount = relationships.count;
    cursor += relationships.totalBytes;

    uint64_t routeRuleStateBytes = static_cast<uint64_t>( liveRouteRules ) * 4;

    if( cursor + routeRuleStateBytes > m_data.size() - FOOTER_SIZE )
        THROW_IO_ERROR( "Invalid PADS route-rule state extent" );

    cursor += routeRuleStateBytes;

    for( int tag : { 51, 50, 52, 53, 54, 55, 56, 57 } )
    {
        SDB_SECTION& section = m_sections.at( tag );

        if( cursor + section.totalBytes > m_data.size() - FOOTER_SIZE )
            THROW_IO_ERROR( "Invalid PADS flat-controller extent" );

        section.physicalOffset = static_cast<uint32_t>( cursor );
        section.physicalBytes = section.totalBytes;
        section.physicalCount = section.count;
        section.physicalLiveCount = section.count;
        cursor += section.totalBytes;
    }

    for( int tag = 58; tag <= 64; ++tag )
    {
        SDB_SECTION& section = m_sections.at( tag );

        if( cursor + section.totalBytes > m_data.size() - FOOTER_SIZE )
            THROW_IO_ERROR( "Invalid PADS flat-controller extent" );

        section.physicalOffset = static_cast<uint32_t>( cursor );
        section.physicalBytes = section.totalBytes;
        section.physicalCount = section.count;
        section.physicalLiveCount = section.count;
        cursor += section.totalBytes;
    }

    for( int tag : { 65, 66 } )
    {
        SDB_SECTION& section = m_sections.at( tag );
        uint64_t     bytes = static_cast<uint64_t>( section.physicalCount ) * recordStride( tag );

        if( cursor + bytes > m_data.size() - FOOTER_SIZE || bytes > UINT32_MAX )
            THROW_IO_ERROR( "Invalid PADS paged-controller extent" );

        section.physicalOffset = static_cast<uint32_t>( cursor );
        section.physicalBytes = static_cast<uint32_t>( bytes );
        section.physicalLiveCount = section.physicalCount;
        cursor += bytes;
    }

    for( int tag : { 67, 68 } )
    {
        SDB_SECTION& section = m_sections.at( tag );

        if( cursor + section.totalBytes > m_data.size() - FOOTER_SIZE )
            THROW_IO_ERROR( "Invalid PADS flat-controller extent" );

        section.physicalOffset = static_cast<uint32_t>( cursor );
        section.physicalBytes = section.totalBytes;
        section.physicalCount = section.count;
        section.physicalLiveCount = section.count;
        cursor += section.totalBytes;
    }

    SDB_SECTION& layers = m_sections.at( 69 );
    const uint64_t layerBytes = 12 + static_cast<uint64_t>( layers.totalBytes );

    if( cursor + layerBytes > m_data.size() - FOOTER_SIZE || layerBytes > UINT32_MAX )
        THROW_IO_ERROR( "Invalid PADS layer-controller extent" );

    layers.physicalOffset = static_cast<uint32_t>( cursor );
    layers.physicalBytes = static_cast<uint32_t>( layerBytes );
    layers.physicalCount = layers.count;
    layers.physicalLiveCount = layers.count;
    cursor += layerBytes;

    SDB_SECTION& layerState = m_sections.at( 70 );

    if( cursor + 4 > m_data.size() - FOOTER_SIZE )
        THROW_IO_ERROR( "Invalid PADS layer-state extent" );

    layerState.physicalOffset = static_cast<uint32_t>( cursor );
    layerState.physicalBytes = 4;
    layerState.physicalCount = layerState.count;
    layerState.physicalLiveCount = layerState.count;
    cursor += 4;

    SDB_SECTION& displayPreferences = m_sections.at( 71 );

    if( displayPreferences.totalBytes < 4 )
        THROW_IO_ERROR( "Invalid PADS display-preferences extent" );

    const uint64_t preferenceBytes = displayPreferences.totalBytes - 4;

    if( cursor + preferenceBytes > m_data.size() - FOOTER_SIZE )
        THROW_IO_ERROR( "Invalid PADS display-preferences extent" );

    displayPreferences.physicalOffset = static_cast<uint32_t>( cursor );
    displayPreferences.physicalBytes = static_cast<uint32_t>( preferenceBytes );
    displayPreferences.physicalCount = displayPreferences.count;
    displayPreferences.physicalLiveCount = displayPreferences.count;
    cursor += preferenceBytes;

    for( int tag = 72; tag <= 73 && tag < static_cast<int>( m_sections.size() ); ++tag )
    {
        SDB_SECTION& section = m_sections[tag];

        if( cursor + section.totalBytes > m_data.size() - FOOTER_SIZE )
            THROW_IO_ERROR( "Invalid PADS flat-controller extent" );

        section.physicalOffset = static_cast<uint32_t>( cursor );
        section.physicalBytes = section.totalBytes;
        section.physicalCount = section.count;
        section.physicalLiveCount = section.count;
        cursor += section.totalBytes;
    }

    if( m_sections.size() > 74 )
    {
        SDB_SECTION& section = m_sections[74];
        uint64_t     bytes = static_cast<uint64_t>( section.physicalCount ) * recordStride( 74 );

        if( cursor + bytes > m_data.size() - FOOTER_SIZE || bytes > UINT32_MAX )
            THROW_IO_ERROR( "Invalid PADS paged-controller extent" );

        section.physicalOffset = static_cast<uint32_t>( cursor );
        section.physicalBytes = static_cast<uint32_t>( bytes );
        section.physicalLiveCount = section.physicalCount;
        cursor += bytes;
    }

    const uint32_t containerItemsOffset = m_cursor.U32At( m_data.size() - 4 );

    if( containerItemsOffset > m_data.size() - FOOTER_SIZE || cursor > containerItemsOffset
        || containerItemsOffset - cursor < 15 )
        THROW_IO_ERROR( "Invalid PADS post-layer database extent" );

    static constexpr char POWER_SYS_TITLE[] = "PowerSYS";

    if( m_cursor.U8At( cursor + 4 ) != sizeof( POWER_SYS_TITLE ) - 1
        || !std::equal( std::begin( POWER_SYS_TITLE ), std::end( POWER_SYS_TITLE ) - 1,
                        m_data.begin() + static_cast<std::ptrdiff_t>( cursor + 5 ) ) )
    {
        THROW_IO_ERROR( "Invalid PADS post-layer database header" );
    }
}


const SDB_SECTION* PADS_SDB::Section( int aIndex ) const
{
    if( aIndex >= 0 && aIndex < static_cast<int>( m_sections.size() ) )
        return &m_sections[aIndex];

    return nullptr;
}


void PADS_SDB::readBoardSetup()
{
    // Section 1 is the serialized *PCB* board-setup parameter block. Its rotated logical view
    // starts before the physical section-2/3 stream, and the displacement is accounted for by
    // two declared quantities, so the block is computed rather than searched for.
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
    // This resolves all 597 unique files in the four-root corpus. Every displacement the old
    // field-range search absorbed is an exact multiple of 48 once the stored slot count is used.
    const SDB_SECTION* directory = Section( 1 );
    const SDB_SECTION* viewStates = Section( 2 );

    if( !directory || !viewStates || directory->count == 0 )
        THROW_IO_ERROR( "Invalid PADS board-setup controllers" );

    const uint64_t base = static_cast<uint64_t>( HEADER_SIZE )
                          + static_cast<uint64_t>( directory->count - 1 ) * DIR_ENTRY_SIZE
                          + static_cast<uint64_t>( viewStates->count ) * VIEW_STATE_RECORD_BYTES;

    if( base + 100 > m_data.size() )
        THROW_IO_ERROR( "Invalid PADS board-setup extent" );

    // These fields validate the block at its declared location; they never choose another
    // location. They mirror the *PCB* section's ASCII-export order: SCALE, BACKUPTIME,
    // REAL WIDTH, ALLSIGONOFF, REFNAMESIZE.
    const float scale = m_cursor.F32At( base + 56 );

    if( !( scale > 0.01f && scale < 1e6f ) )
        THROW_IO_ERROR( "Invalid PADS board-setup scale" );

    const int32_t backupTime = m_cursor.I32At( base + 76 );

    if( backupTime < 0 || backupTime > 100000 )
        THROW_IO_ERROR( "Invalid PADS board-setup backup interval" );

    // REAL WIDTH is legitimately zero on boards that never set a default trace width, so the
    // lower bound the old search used rejected 23 of the corpus outright. Every one of those
    // rejects failed on this predicate alone and every one of them reads zero here.
    const int32_t realWidth = m_cursor.I32At( base + 80 );

    if( realWidth < 0 || realWidth >= 100000000 )
        THROW_IO_ERROR( "Invalid PADS board-setup display width" );

    const int32_t allSigOnOff = m_cursor.I32At( base + 84 );

    if( allSigOnOff != 0 && allSigOnOff != 1 )
        THROW_IO_ERROR( "Invalid PADS board-setup signal-display flag" );

    const int32_t refHeight = m_cursor.I32At( base + 92 );
    const int32_t refWidth = m_cursor.I32At( base + 96 );

    if( !( refWidth > 100000 && refWidth <= refHeight && refHeight < 100000000 ) )
        THROW_IO_ERROR( "Invalid PADS board-setup reference-text size" );

    m_coords.m_originX = m_cursor.I32At( base + 60 );
    m_coords.m_originY = m_cursor.I32At( base + 64 );
    m_coords.m_found = true;
    m_coords.m_headerBase = static_cast<uint32_t>( base );
}

} // namespace PADS_IO
