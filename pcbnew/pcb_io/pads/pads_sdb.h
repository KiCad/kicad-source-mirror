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
 * (System DataBase) object store, not an ad-hoc record blob. This module models
 * that database so the section readers above it can speak in database terms
 * (sections, records, design coordinates) instead of raw file offsets.
 *
 * File anatomy:
 *   [Header]    10 B  magic 00 FF, version u16 @ +2.
 *   [Directory] N x 16 B @ +10  one entry per database controller's record stream.
 *   [Sections]  the controllers' payloads, laid out contiguously in index order.
 *   [Footer]    46 B  GUID + a stored size check.
 *
 * Supported versions: 0x2021, 0x2022, 0x2024, 0x2025, 0x2026, 0x2027.
 */

/**
 * One directory section: a single database controller's serialized record stream.
 *
 * @c count is the controller's declared record count and @c totalBytes its payload
 * size; @c perItem is the nominal fixed stride (totalBytes/count) when both are
 * non-zero. @c dataOffset is the absolute file offset of the payload, accumulated
 * across preceding sections.
 */
struct SDB_SECTION
{
    int      index = 0;
    uint32_t count = 0;
    uint32_t totalBytes = 0;
    uint32_t dataOffset = 0;
    uint32_t perItem = 0;

    bool     IsEmpty() const { return totalBytes == 0; }
    uint32_t End() const { return dataOffset + totalBytes; }
};

/**
 * The per-axis coordinate origin and the design <-> raw transform.
 *
 * Every coordinate in a PADS file is stored in BASIC units (1/38100 mil) biased
 * by a per-axis origin, so a stored value is @c design + @c origin and a design
 * value is @c raw - @c origin. The origin is read from the section-1 i32 pair at
 * +60/+64 (see PADS_SDB::locateOrigin for the version coverage and the known
 * SCALE-anchored generalization).
 */
class SDB_COORDS
{
public:
    int32_t OriginX() const { return m_originX; }
    int32_t OriginY() const { return m_originY; }
    bool    Found() const { return m_found; }

    int32_t DesignX( int32_t aRaw ) const { return aRaw - m_originX; }
    int32_t DesignY( int32_t aRaw ) const { return aRaw - m_originY; }

private:
    friend class PADS_SDB;

    int32_t m_originX = 0;
    int32_t m_originY = 0;
    bool    m_found = false;
};

/**
 * A bounds-checked reader positioned at one record inside a section.
 *
 * Field reads are by named offset relative to the record base, so a section reader
 * documents its record layout in one place (named field constants) instead of
 * threading absolute file offsets. @c DesignX / @c DesignY additionally apply the
 * coordinate origin, the one operation every geometry field needs.
 *
 * Holds references to the owning database's cursor and coordinate system; it is a
 * transient view and must not outlive the PADS_SDB it came from.
 */
class SDB_RECORD
{
public:
    SDB_RECORD( const BINARY_CURSOR& aCursor, const SDB_COORDS& aCoords, uint32_t aBase ) :
            m_cursor( aCursor ), m_coords( aCoords ), m_base( aBase )
    {
    }

    uint8_t     U8( uint32_t aOffset ) const { return m_cursor.U8At( m_base + aOffset ); }
    uint16_t    U16( uint32_t aOffset ) const { return m_cursor.U16At( m_base + aOffset ); }
    uint32_t    U32( uint32_t aOffset ) const { return m_cursor.U32At( m_base + aOffset ); }
    int32_t     I32( uint32_t aOffset ) const { return m_cursor.I32At( m_base + aOffset ); }
    double      F64( uint32_t aOffset ) const { return m_cursor.F64At( m_base + aOffset ); }
    std::string Str( uint32_t aOffset, size_t aMaxLen ) const
    {
        return m_cursor.StringAt( m_base + aOffset, aMaxLen );
    }

    int32_t DesignX( uint32_t aOffset ) const { return m_coords.DesignX( I32( aOffset ) ); }
    int32_t DesignY( uint32_t aOffset ) const { return m_coords.DesignY( I32( aOffset ) ); }

    uint32_t Base() const { return m_base; }

private:
    const BINARY_CURSOR& m_cursor;
    const SDB_COORDS&    m_coords;
    uint32_t             m_base;
};

/**
 * The parsed container of a PADS `.pcb` file: header, the section directory, and
 * the coordinate system. Owns the file bytes and the bounds-checked read cursor
 * the section readers use.
 *
 * Non-copyable: the read cursor binds a reference to the owned byte buffer, so a
 * copy would leave the clone's cursor pointing at the source's bytes.
 */
class PADS_SDB
{
public:
    PADS_SDB() = default;

    PADS_SDB( const PADS_SDB& ) = delete;
    PADS_SDB& operator=( const PADS_SDB& ) = delete;

    /**
     * Validate the header and footer, parse the section directory, and locate the
     * coordinate origin. Throws IO_ERROR on a malformed or unsupported file.
     */
    void Load( std::vector<uint8_t> aBytes );

    /// True if the first two bytes match the PADS binary magic (cheap pre-check).
    static bool HasMagic( const std::vector<uint8_t>& aBytes );

    /// The set of PADS binary container versions this decoder understands. The header
    /// validation and the file-type probe share it so the supported set has one home.
    static bool IsSupportedVersion( uint16_t aVersion );

    uint16_t                    Version() const { return m_version; }
    bool                        IsOldFormat() const { return m_version == 0x2021 || m_version == 0x2022; }
    const std::vector<uint8_t>& Bytes() const { return m_data; }
    const BINARY_CURSOR&        Cursor() const { return m_cursor; }
    const SDB_COORDS&           Coords() const { return m_coords; }

    size_t             SectionCount() const { return m_sections.size(); }
    const SDB_SECTION* Section( int aIndex ) const;

    /// A reader for record @p aIndex of @p aSection, treating the section as a run of
    /// fixed-stride @p aStride records. The caller is responsible for staying within
    /// the section; the field reads remain bounds-checked against the file.
    SDB_RECORD Record( const SDB_SECTION& aSection, uint32_t aIndex, uint32_t aStride ) const
    {
        return SDB_RECORD( m_cursor, m_coords, aSection.dataOffset + aIndex * aStride );
    }

    /// A reader positioned at an absolute file offset, for the readers that walk a pool
    /// or a byte region rather than a section's fixed-stride records.
    SDB_RECORD RecordAt( uint32_t aBase ) const { return SDB_RECORD( m_cursor, m_coords, aBase ); }

private:
    void parseHeader();
    void parseDirectory();
    void verifyFooter() const;
    void locateOrigin();
    int  directoryEntryCount() const;

    static constexpr uint8_t MAGIC0 = 0x00;
    static constexpr uint8_t MAGIC1 = 0xFF;
    static constexpr int     HEADER_SIZE = 10;
    static constexpr int     FOOTER_SIZE = 46;
    static constexpr int     DIR_ENTRY_SIZE = 16;

    std::vector<uint8_t> m_data;

    // Declared after m_data so the bound reference targets a live buffer.
    BINARY_CURSOR m_cursor{ m_data };

    uint16_t                 m_version = 0;
    std::vector<SDB_SECTION> m_sections;
    SDB_COORDS               m_coords;
};

} // namespace PADS_IO
