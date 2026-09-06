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

/** The caller owns the buffer. Reads and cursor moves throw IO_ERROR on overrun.
 * Strings retain raw Windows-1252 bytes until conversion with FromOrcadString. */
class ORCAD_STREAM
{
public:
    /** Magic preceding every framed structure body: FF E4 5C 39. */
    static constexpr uint8_t PREAMBLE[4] = { 0xFF, 0xE4, 0x5C, 0x39 };

    /** Generic invalid offset sentinel. */
    static constexpr size_t npos = static_cast<size_t>( -1 );

    /** Nesting limit of the recursive readers; real files stay far below it. */
    static constexpr int MAX_NESTING = 64;

    /** Bound shared parser recursion; entering a level above MAX_NESTING throws IO_ERROR. */
    class NEST_GUARD
    {
    public:
        NEST_GUARD( ORCAD_STREAM& aStream, const wxString& aWhat );
        ~NEST_GUARD();

        NEST_GUARD( const NEST_GUARD& ) = delete;
        NEST_GUARD& operator=( const NEST_GUARD& ) = delete;

    private:
        ORCAD_STREAM& m_stream;
    };

    /** Limit reads to one record so a malformed body cannot consume the next record. */
    class LIMIT_GUARD
    {
    public:
        LIMIT_GUARD( ORCAD_STREAM& aStream, size_t aEnd );
        ~LIMIT_GUARD();

        LIMIT_GUARD( const LIMIT_GUARD& ) = delete;
        LIMIT_GUARD& operator=( const LIMIT_GUARD& ) = delete;

    private:
        ORCAD_STREAM& m_stream;
        size_t        m_savedSize;
    };

    ORCAD_STREAM( const void* aData, size_t aLength );
    explicit ORCAD_STREAM( const std::vector<char>& aData );

    /** -- scalars (little-endian, bounds-checked, throw IO_ERROR on overrun) ------- */

    uint8_t  ReadU8();
    int8_t   ReadI8();
    uint16_t ReadU16();
    int16_t  ReadI16();
    uint32_t ReadU32();
    int32_t  ReadI32();

    /** -- strings ------------------------------------------------------------------- */

    /** Reads u16 length, raw CP-1252 bytes, and a required NUL. Invalid data throws IO_ERROR. */
    std::string ReadLzt();

    /** Consumes the NUL; throws IO_ERROR if no terminator remains. */
    std::string ReadZt();

    /** -- buffers / cursor ------------------------------------------------------------ */

    /** Read exactly aCount bytes; throws IO_ERROR on overrun. */
    std::vector<uint8_t> ReadBytes( size_t aCount );

    /** Advance the cursor; throws IO_ERROR when aCount exceeds the remaining bytes. */
    void Skip( size_t aCount );

    /** Set the absolute cursor position; throws IO_ERROR when aOffset exceeds Size(). */
    void Seek( size_t aOffset );

    size_t GetOffset() const { return m_offset; }
    size_t Size() const { return m_size; }

    /** Bytes left; 0 when the cursor is at or past the end. */
    size_t Remaining() const { return m_offset >= m_size ? 0 : m_size - m_offset; }

    bool AtEnd() const { return m_offset >= m_size; }

    /** Raw buffer access for bounded lexical lookahead and payload extraction. */
    const uint8_t* Data() const { return m_data; }

    /** -- non-throwing lookahead ------------------------------------------------------ */

    /** Returns -1 when cursor + aAhead is outside the stream. */
    int PeekU8( size_t aAhead = 0 ) const;

    /** Returns false if fewer than aCount bytes remain; does not advance the cursor. */
    bool PeekMatches( const uint8_t* aBytes, size_t aCount, size_t aAhead = 0 ) const;

    /** True when the 4 preamble bytes FF E4 5C 39 sit at cursor + aAhead. */
    bool AtPreamble( size_t aAhead = 0 ) const;

    /** -- validated reads --------------------------------------------------------------- */

    /** Consumes the bytes before checking them. A mismatch throws IO_ERROR with expected and actual bytes. */
    void Expect( const uint8_t* aBytes, size_t aCount, const wxString& aWhat );

    /** Consume one byte and require the given value. */
    void ExpectByte( uint8_t aValue, const wxString& aWhat );

    /** Consume the 4 preamble bytes; throws IO_ERROR naming aWhat when absent. */
    void ExpectPreamble( const wxString& aWhat );

    /** Legacy primitives have no trailing preamble. Skip one only when it is present. */
    void SkipOptionalPreambleBlock();

private:
    /** Throw IO_ERROR unless aCount bytes remain at the cursor. */
    void requireBytes( size_t aCount ) const;

    const uint8_t* m_data;
    size_t         m_size;
    size_t         m_offset;
    int            m_nesting;
};


/** Use an 8-bit fallback if Windows-1252 decoding fails. */
wxString FromOrcadString( const std::string& aText );

#endif // ORCAD_STREAM_H_
