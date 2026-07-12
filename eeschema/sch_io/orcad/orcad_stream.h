/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * Based on the dsn2kicad reference implementation and on OrCAD file format
 * documentation from the OpenOrCadParser project (MIT licensed).
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef ORCAD_STREAM_H_
#define ORCAD_STREAM_H_

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <wx/string.h>

/**
 * Little-endian byte cursor over one OrCAD Capture DSN stream.
 *
 * String encoding inside the streams: u16 length + content bytes + NUL terminator
 * ("length + zero-terminated", ReadLzt), or bare NUL-terminated (ReadZt).  Text is
 * Windows-1252 encoded; readers return the raw bytes as std::string and conversion
 * to Unicode happens once at the UI boundary (see FromOrcadString()).
 *
 * The recurring structure framing in Page/Cache/Schematic/Hierarchy streams:
 *
 *   N x long prefix : u8 typeId, u32 bodyLen, u32 zero
 *   1 x short prefix: u8 typeId, i16 count, count x ( u32 nameIdx, u32 valueIdx )
 *   preamble        : FF E4 5C 39, u32 trailLen, trailLen bytes
 *
 * The long-prefix bodyLen values record how many bytes of structure body follow the
 * end of the prefix's own 9-byte record (outermost first), which yields reliable
 * skip offsets for structures whose body cannot be parsed.
 *
 * The stream does not own the buffer; the caller must keep it alive.
 *
 * All reads are bounds-checked and throw IO_ERROR (via THROW_IO_ERROR) on overrun.
 * The cursor itself may legally point past the end of the buffer (structure skip
 * offsets computed from file data can do that); any subsequent read then throws.
 */
class ORCAD_STREAM
{
public:
    /// Magic preceding every framed structure body: FF E4 5C 39.
    static constexpr uint8_t PREAMBLE[4] = { 0xFF, 0xE4, 0x5C, 0x39 };

    /// Returned by FindPreamble() when no further preamble exists.
    static constexpr size_t npos = static_cast<size_t>( -1 );

    ORCAD_STREAM( const void* aData, size_t aLength );
    explicit ORCAD_STREAM( const std::vector<char>& aData );

    // -- scalars (little-endian, bounds-checked, throw IO_ERROR on overrun) -------

    uint8_t  ReadU8();
    int8_t   ReadI8();
    uint16_t ReadU16();
    int16_t  ReadI16();
    uint32_t ReadU32();
    int32_t  ReadI32();

    // -- strings -------------------------------------------------------------------

    /**
     * Read a length-prefixed zero-terminated string: u16 length, `length` content
     * bytes, then a mandatory 0x00 terminator.  Throws IO_ERROR when the string
     * exceeds the stream or the terminator is missing.  Returns raw CP-1252 bytes.
     */
    std::string ReadLzt();

    /**
     * Read a bare zero-terminated string (no length prefix); consumes the NUL.
     * Throws IO_ERROR if no terminator exists before the end of the stream.
     */
    std::string ReadZt();

    // -- buffers / cursor ------------------------------------------------------------

    /// Read exactly aCount bytes; throws IO_ERROR on overrun.
    std::vector<uint8_t> ReadBytes( size_t aCount );

    /// Advance the cursor; throws IO_ERROR when aCount exceeds the remaining bytes.
    void Skip( size_t aCount );

    /**
     * Set the absolute cursor position.  Positions beyond Size() are permitted
     * (skip offsets recorded in the file may point past a truncated stream); any
     * read attempted from such a position throws.
     */
    void Seek( size_t aOffset ) { m_offset = aOffset; }

    size_t GetOffset() const { return m_offset; }
    size_t Size() const { return m_size; }

    /// Bytes left; 0 when the cursor is at or past the end.
    size_t Remaining() const { return m_offset >= m_size ? 0 : m_size - m_offset; }

    bool AtEnd() const { return m_offset >= m_size; }

    /// Raw buffer access for scan-and-backtrack parsers (cache/hierarchy walkers).
    const uint8_t* Data() const { return m_data; }

    // -- non-throwing lookahead ------------------------------------------------------

    /**
     * Peek one byte at cursor + aAhead without advancing.
     * @return the byte value 0..255, or -1 when the position is out of range.
     */
    int PeekU8( size_t aAhead = 0 ) const;

    /**
     * Compare aCount bytes at cursor + aAhead against aBytes without advancing.
     * @return false (never throws) when fewer than aCount bytes remain.
     */
    bool PeekMatches( const uint8_t* aBytes, size_t aCount, size_t aAhead = 0 ) const;

    /// True when the 4 preamble bytes FF E4 5C 39 sit at cursor + aAhead.
    bool AtPreamble( size_t aAhead = 0 ) const;

    /// True when the preamble sits at the given absolute offset (false if out of range).
    bool HasPreambleAt( size_t aAbsoluteOffset ) const;

    /**
     * Find the next preamble at or after the given absolute offset.
     * @return the absolute offset of the magic, or ORCAD_STREAM::npos.
     */
    size_t FindPreamble( size_t aFrom ) const;

    // -- validated reads ---------------------------------------------------------------

    /**
     * Consume aCount bytes and require them to equal aBytes; throws IO_ERROR naming
     * aWhat and showing expected/actual hex on mismatch.  The bytes are consumed
     * even when the check fails (callers recover by seeking).
     */
    void Expect( const uint8_t* aBytes, size_t aCount, const wxString& aWhat );

    /// Consume one byte and require the given value.
    void ExpectByte( uint8_t aValue, const wxString& aWhat );

    /// Consume the 4 preamble bytes; throws IO_ERROR naming aWhat when absent.
    void ExpectPreamble( const wxString& aWhat );

    /**
     * If the preamble sits at the cursor, consume it plus its u32 trailLen and the
     * trailLen trailing bytes; otherwise do nothing.  Legacy primitive records have
     * no trailing preamble, modern ones do.
     */
    void SkipOptionalPreambleBlock();

private:
    /// Throw IO_ERROR unless aCount bytes remain at the cursor.
    void requireBytes( size_t aCount ) const;

    const uint8_t* m_data;
    size_t         m_size;
    size_t         m_offset;
};


/**
 * Decode raw stream bytes (Windows-1252) into a wxString for UI/UX use.  Undecodable
 * bytes degrade to an 8-bit-data fallback rather than an empty string.
 */
wxString FromOrcadString( const std::string& aText );

#endif // ORCAD_STREAM_H_
