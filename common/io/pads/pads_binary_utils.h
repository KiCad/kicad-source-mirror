/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef PADS_BINARY_UTILS_H_
#define PADS_BINARY_UTILS_H_

#include <algorithm>
#include <array>
#include <cstdint>
#include <functional>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

#include <wx/string.h>

#include <ki_exception.h>
#include <string_utils.h>

namespace PADS_IO
{

// A live SDB record header carries the FE FF (little-endian 0xFFFE) sentinel; an all-ones 32-bit
// field is a free/unset slot.
constexpr uint16_t SDB_RECORD_SENTINEL = 0xFFFE;
constexpr uint32_t SDB_FIELD_UNSET = 0xFFFFFFFF;

// PADS BASIC coordinate units per mil (one BASIC unit is 1/38100 mil).
constexpr double SDB_BASIC_PER_MIL = 38100.0;

/**
 * Little-endian byte assembly with no bounds checking. Callers that need a
 * bounds-checked policy wrap these.
 */
inline uint8_t getU8( const std::vector<uint8_t>& aData, size_t aOffset )
{
    return aData[aOffset];
}


inline uint16_t getU16LE( const std::vector<uint8_t>& aData, size_t aOffset )
{
    return static_cast<uint16_t>( aData[aOffset] )
           | ( static_cast<uint16_t>( aData[aOffset + 1] ) << 8 );
}


inline uint32_t getU32LE( const std::vector<uint8_t>& aData, size_t aOffset )
{
    return static_cast<uint32_t>( aData[aOffset] )
           | ( static_cast<uint32_t>( aData[aOffset + 1] ) << 8 )
           | ( static_cast<uint32_t>( aData[aOffset + 2] ) << 16 )
           | ( static_cast<uint32_t>( aData[aOffset + 3] ) << 24 );
}


inline int32_t getI32LE( const std::vector<uint8_t>& aData, size_t aOffset )
{
    return static_cast<int32_t>( getU32LE( aData, aOffset ) );
}


inline double getF64LE( const std::vector<uint8_t>& aData, size_t aOffset )
{
    uint64_t bits = 0;

    for( int i = 0; i < 8; ++i )
        bits |= static_cast<uint64_t>( aData[aOffset + i] ) << ( 8 * i );

    double value = 0;
    std::memcpy( &value, &bits, sizeof( value ) );

    return value;
}


inline float getF32LE( const std::vector<uint8_t>& aData, size_t aOffset )
{
    uint32_t bits = getU32LE( aData, aOffset );
    float    value = 0;
    std::memcpy( &value, &bits, sizeof( value ) );

    return value;
}


/// Returned by findSignature when the signature does not occur.
constexpr size_t SIGNATURE_NOT_FOUND = static_cast<size_t>( -1 );

/**
 * First offset >= @p aFrom at which the non-empty signature @p aSig (length @p aSigLen) matches in
 * @p aData, or @ref SIGNATURE_NOT_FOUND when it does not occur. An empty signature is unsupported
 * and yields @ref SIGNATURE_NOT_FOUND. Callers apply their own bounds for any fields trailing the
 * matched signature.
 */
inline size_t findSignature( const std::vector<uint8_t>& aData, const uint8_t* aSig, size_t aSigLen,
                             size_t aFrom = 0 )
{
    if( aSigLen == 0 || aData.size() < aSigLen || aFrom > aData.size() - aSigLen )
        return SIGNATURE_NOT_FOUND;

    auto it = std::search( aData.begin() + static_cast<std::ptrdiff_t>( aFrom ), aData.end(), aSig,
                           aSig + aSigLen );

    return it == aData.end() ? SIGNATURE_NOT_FOUND : static_cast<size_t>( it - aData.begin() );
}


/// Overload for the fixed-size std::array signatures the SCH reader uses.
template <size_t N>
inline size_t findSignature( const std::vector<uint8_t>& aData, const std::array<uint8_t, N>& aSig,
                             size_t aFrom = 0 )
{
    return findSignature( aData, aSig.data(), N, aFrom );
}


/// True iff @p aSig occurs exactly once in @p aData (the uniqueness gate some anchors require).
inline bool signatureIsUnique( const std::vector<uint8_t>& aData, const uint8_t* aSig,
                               size_t aSigLen )
{
    size_t first = findSignature( aData, aSig, aSigLen );

    return first != SIGNATURE_NOT_FOUND
           && findSignature( aData, aSig, aSigLen, first + 1 ) == SIGNATURE_NOT_FOUND;
}


/// A contiguous stride-N record run: its base offset and member count.
struct STRIDE_RUN
{
    size_t base = 0;
    size_t count = 0;
};


/**
 * From a hit anywhere inside a stride-@p aStride record run, back up to the run's first member
 * (staying at or above @p aLo) then count the contiguous members forward (staying within @p aHi).
 *
 * Pure geometry; @p aIsMember decides membership at a candidate offset. The caller keeps whatever
 * it does with each member (dedup, read, emit). Returns the run base and member count.
 */
inline STRIDE_RUN extendStrideRun( size_t aHit, size_t aStride, size_t aLo, size_t aHi,
                                   const std::function<bool( size_t )>& aIsMember )
{
    size_t base = aHit;

    while( base >= aLo + aStride && aIsMember( base - aStride ) )
        base -= aStride;

    size_t count = 0;

    while( base + ( count + 1 ) * aStride <= aHi && aIsMember( base + count * aStride ) )
        ++count;

    return { base, count };
}


/**
 * Read an entire file into @p aOut. Returns false on open error or short read.
 */
inline bool ReadFileToBuffer( const wxString& aFileName, std::vector<uint8_t>& aOut )
{
    std::ifstream file( aFileName.fn_str(), std::ios::binary | std::ios::ate );

    if( !file.is_open() )
        return false;

    std::streamoff len = file.tellg();
    file.seekg( 0, std::ios::beg );

    if( len <= 0 )
        return false;

    aOut.resize( static_cast<size_t>( len ) );
    file.read( reinterpret_cast<char*>( aOut.data() ), len );

    return file.gcount() == len;
}


/**
 * Read a NUL-terminated fixed-width field starting at @p aOffset, scanning at
 * most @p aMaxLen bytes, WITHOUT trimming. Returns an empty string when the
 * field contains a control byte. High bytes are kept because PADS names use a
 * legacy 8-bit code page; decode them with PADS_COMMON::ConvertText. Use this
 * where the field is compared byte-for-byte; use @ref readFixedString to trim.
 */
inline std::string readFixedStringRaw( const std::vector<uint8_t>& aData, size_t aOffset,
                                       size_t aMaxLen )
{
    if( aOffset >= aData.size() )
        return {};

    size_t         available = std::min( aMaxLen, aData.size() - aOffset );
    const uint8_t* start = &aData[aOffset];

    const uint8_t* null_pos = std::find( start, start + available, 0 );
    size_t         len = static_cast<size_t>( null_pos - start );

    for( size_t i = 0; i < len; ++i )
    {
        if( start[i] < 0x20 || start[i] == 0x7F )
            return {};
    }

    return std::string( reinterpret_cast<const char*>( start ), len );
}


/**
 * As @ref readFixedStringRaw, then trim trailing spaces.
 */
inline std::string readFixedString( const std::vector<uint8_t>& aData, size_t aOffset, size_t aMaxLen )
{
    std::string raw = readFixedStringRaw( aData, aOffset, aMaxLen );

    // Trimmed here rather than through StrPurge, which also strips leading whitespace and walks
    // one before the start on an empty string
    while( !raw.empty() && raw.back() == ' ' )
        raw.pop_back();

    return raw;
}


/**
 * Check the PADS SDB container magic, a leading 0x00 followed by a
 * format-specific second byte. The version word at +2 is range-checked by the
 * caller because the supported version set differs per format.
 */
inline bool HasSdbMagic( const std::vector<uint8_t>& aData, uint8_t aMagic1 )
{
    return aData.size() >= 2 && aData[0] == 0x00 && aData[1] == aMagic1;
}


/**
 * Validate a PADS SDB footer at @p aFooterStart, the ASCII GUID at
 * @p aFooterStart + 4 followed by the stored size-check.
 *
 * Throws IO_ERROR when the buffer is too small or the GUID does not match. The
 * size-check (u32 after the GUID) should equal @p aFooterStart; a mismatch is a
 * corruption hint, not fatal, so it is logged rather than thrown. The GUID
 * differs per format, so the caller supplies it.
 */
void ValidateSdbFooter( const std::vector<uint8_t>& aData, size_t aFooterStart, const char* aGuid,
                        size_t aGuidLen );


/**
 * Bounds-checked little-endian read cursor over a PADS binary buffer.
 *
 * The random-access @c *At accessors throw IO_ERROR on an out-of-range read
 * rather than indexing past the buffer. The sequential reads advance an internal
 * position; the decoders read almost entirely by absolute offset.
 *
 * The cursor holds a reference to the caller's buffer, which must outlive the
 * cursor.
 */
class BINARY_CURSOR
{
public:
    explicit BINARY_CURSOR( const std::vector<uint8_t>& aData ) : m_data( aData ) {}

    size_t Tell() const { return m_pos; }
    size_t Size() const { return m_data.size(); }
    void   Seek( size_t aPos ) { m_pos = aPos; }

    bool Remaining( size_t aCount ) const { return InBounds( m_pos, aCount ); }

    // Overflow-safe: a near-SIZE_MAX offset must not wrap aOffset + aCount into range.
    bool InBounds( size_t aOffset, size_t aCount ) const
    {
        return aOffset <= m_data.size() && aCount <= m_data.size() - aOffset;
    }

    uint8_t U8At( size_t aOffset ) const
    {
        check( aOffset, 1 );
        return getU8( m_data, aOffset );
    }

    uint16_t U16At( size_t aOffset ) const
    {
        check( aOffset, 2 );
        return getU16LE( m_data, aOffset );
    }

    uint32_t U32At( size_t aOffset ) const
    {
        check( aOffset, 4 );
        return getU32LE( m_data, aOffset );
    }

    int32_t I32At( size_t aOffset ) const { return static_cast<int32_t>( U32At( aOffset ) ); }

    double F64At( size_t aOffset ) const
    {
        check( aOffset, 8 );
        return getF64LE( m_data, aOffset );
    }

    float F32At( size_t aOffset ) const
    {
        check( aOffset, 4 );
        return getF32LE( m_data, aOffset );
    }

    // Unlike the numeric accessors this does not throw out of range; aMaxLen is
    // clamped to the available bytes and the read returns empty past EOF, so a
    // short field abutting a boundary still reads cleanly.
    std::string StringAt( size_t aOffset, size_t aMaxLen ) const
    {
        return readFixedString( m_data, aOffset, aMaxLen );
    }

    // As StringAt but without trailing-space trimming.
    std::string StringRawAt( size_t aOffset, size_t aMaxLen ) const
    {
        return readFixedStringRaw( m_data, aOffset, aMaxLen );
    }

    uint8_t  U8() { uint8_t v = U8At( m_pos ); m_pos += 1; return v; }
    uint16_t U16() { uint16_t v = U16At( m_pos ); m_pos += 2; return v; }
    uint32_t U32() { uint32_t v = U32At( m_pos ); m_pos += 4; return v; }
    int32_t  I32() { int32_t v = I32At( m_pos ); m_pos += 4; return v; }

private:
    void check( size_t aOffset, size_t aCount ) const
    {
        if( !InBounds( aOffset, aCount ) )
        {
            THROW_IO_ERROR( wxString::Format( "PADS binary read out of bounds at offset %zu (file size %zu)",
                                              aOffset, m_data.size() ) );
        }
    }

    const std::vector<uint8_t>& m_data;
    size_t                      m_pos = 0;
};


/**
 * A bounds-checked reader positioned at one record inside a buffer.
 *
 * Field reads are by offset relative to the record base, so a reader can express
 * its record layout with named field constants instead of absolute file offsets.
 *
 * Holds a reference to the owning cursor; it is a transient view and must not
 * outlive the buffer the cursor binds.
 */
class SDB_RECORD
{
public:
    SDB_RECORD( const BINARY_CURSOR& aCursor, size_t aBase ) : m_cursor( aCursor ), m_base( aBase )
    {
    }

    uint8_t     U8( size_t aOffset ) const { return m_cursor.U8At( m_base + aOffset ); }
    uint16_t    U16( size_t aOffset ) const { return m_cursor.U16At( m_base + aOffset ); }
    uint32_t    U32( size_t aOffset ) const { return m_cursor.U32At( m_base + aOffset ); }
    int32_t     I32( size_t aOffset ) const { return m_cursor.I32At( m_base + aOffset ); }
    double      F64( size_t aOffset ) const { return m_cursor.F64At( m_base + aOffset ); }
    std::string Str( size_t aOffset, size_t aMaxLen ) const
    {
        return m_cursor.StringAt( m_base + aOffset, aMaxLen );
    }

    // As Str but without trailing-space trimming.
    std::string StrRaw( size_t aOffset, size_t aMaxLen ) const
    {
        return m_cursor.StringRawAt( m_base + aOffset, aMaxLen );
    }

    size_t Base() const { return m_base; }

private:
    const BINARY_CURSOR& m_cursor;
    size_t               m_base;
};

} // namespace PADS_IO

#endif // PADS_BINARY_UTILS_H_
