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

#pragma once

#include <cstdint>
#include <vector>

#include <wx/string.h>

#include <io/pads/pads_binary_utils.h>

namespace PADS_IO
{

/**
 * The PADS PowerPCB `.pcb` file is a serialized snapshot of PADS' in-memory SDB
 * (System DataBase) object store. This module models that database so callers can
 * speak in database terms (sections, records, design coordinates) instead of raw
 * file offsets.
 *
 * File anatomy:
 *   [Header]     6 B  magic 00 FF, version u16 @ +2, subversion u16 @ +4.
 *   [Directory] N x 16 B memory image @ +6. The shifted field view begins at +10; see
 *               directoryEntryCount().
 *   [Sections]  flat and paged controller payloads in loader order.
 *   [Footer]    42 B  GUID + container-item-array back-pointer.
 *
 * Supported versions: 0x2017, 0x2019, 0x2021, 0x2022, 0x2024, 0x2025, 0x2026, 0x2027.
 */

/**
 * One directory section: a single database controller's serialized record stream.
 *
 * @c stride is the nominal fixed stride (totalBytes/count) when both are non-zero.
 */
struct SDB_SECTION
{
    int      index = 0;
    uint32_t count = 0;
    uint32_t totalBytes = 0;
    uint32_t stride = 0;

    uint32_t physicalOffset = 0;
    uint32_t physicalBytes = 0;
    uint32_t physicalCount = 0;
    uint32_t physicalLiveCount = 0;

    bool     IsEmpty() const { return totalBytes == 0; }
};

/**
 * The per-axis coordinate origin and the design <-> raw transform.
 *
 * Every coordinate in a PADS file is stored in BASIC units (1/38100 mil) biased by
 * a per-axis origin, so a design value is @c raw - @c origin.
 */
class SDB_COORDS
{
public:
    int32_t OriginX() const { return m_originX; }
    int32_t OriginY() const { return m_originY; }
    bool    Found() const { return m_found; }

    int32_t DesignX( int32_t aRaw ) const { return aRaw - m_originX; }
    int32_t DesignY( int32_t aRaw ) const { return aRaw - m_originY; }

    /// Absolute file offset of the *PCB* board-setup parameter block read by PADS_SDB
    /// (the origin sits at HeaderBase()+60/+64). Other fields in that same block (e.g.
    /// MAXIMUMLAYER at +16) need this for the same reason the origin does: section 1 is a rotated
    /// logical view over the physical section-2/3 controller stream.
    uint32_t HeaderBase() const { return m_headerBase; }

private:
    friend class PADS_SDB;

    int32_t  m_originX = 0;
    int32_t  m_originY = 0;
    bool     m_found = false;
    uint32_t m_headerBase = 0;
};

/**
 * The parsed container of a PADS `.pcb` file: header, the section directory, and
 * the coordinate system. Owns the file bytes and the bounds-checked read cursor.
 *
 * Non-copyable because the cursor binds a reference to the owned byte buffer, so a
 * copy would leave the clone's cursor pointing at the source's bytes.
 */
class PADS_SDB
{
public:
    PADS_SDB() = default;

    PADS_SDB( const PADS_SDB& ) = delete;
    PADS_SDB& operator=( const PADS_SDB& ) = delete;

    /**
     * Validate the header and footer, parse the section directory, and derive the
     * coordinate origin. Throws IO_ERROR on a malformed or unsupported file.
     */
    void Load( std::vector<uint8_t> aBytes );

    /// True if the first two bytes match the PADS binary magic.
    static bool HasMagic( const std::vector<uint8_t>& aBytes );

    static bool IsSupportedVersion( uint16_t aVersion );

    uint16_t                    Version() const { return m_version; }
    bool                        IsOldFormat() const { return m_version <= 0x2022; }
    const std::vector<uint8_t>& Bytes() const { return m_data; }
    const BINARY_CURSOR&        Cursor() const { return m_cursor; }
    const SDB_COORDS&           Coords() const { return m_coords; }

    size_t             SectionCount() const { return m_sections.size(); }
    const SDB_SECTION* Section( int aIndex ) const;

    /// A reader positioned at an absolute file offset, for walking a pool or byte region
    /// rather than a section's fixed-stride records.
    SDB_RECORD RecordAt( uint32_t aBase ) const { return SDB_RECORD( m_cursor, aBase ); }

private:
    void parseHeader();
    void parseDirectory();
    void parsePhysicalFraming();
    void verifyFooter() const;
    void readBoardSetup();
    int  directoryEntryCount() const;

    static constexpr uint8_t MAGIC0 = 0x00;
    static constexpr uint8_t MAGIC1 = 0xFF;
    static constexpr int     HEADER_SIZE = 10;
    static constexpr int     FOOTER_SIZE = 42;
    static constexpr int     DIR_ENTRY_SIZE = 16;

    /// Stride of the section 2 view-state records that precede the board-setup block.
    static constexpr int     VIEW_STATE_RECORD_BYTES = 48;

    std::vector<uint8_t> m_data;

    // Declared after m_data so the bound reference targets a live buffer.
    BINARY_CURSOR m_cursor{ m_data };

    uint16_t                 m_version = 0;
    std::vector<SDB_SECTION> m_sections;
    SDB_COORDS               m_coords;
};

} // namespace PADS_IO
