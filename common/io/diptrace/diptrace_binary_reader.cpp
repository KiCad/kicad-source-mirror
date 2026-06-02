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

#include "diptrace_binary_reader.h"

#include <algorithm>
#include <cstdio>
#include <cstring>

#include <ki_exception.h>
#include <wx/filename.h>
#include <wx/translation.h>


using namespace DIPTRACE;


BINARY_READER::BINARY_READER( const wxString& aFileName ) :
        m_offset( 0 ),
        m_version( 0 ),
        m_stringEncoding( STRING_ENCODING::BY_VERSION )
{
    FILE* fp = wxFopen( aFileName, wxT( "rb" ) );

    if( fp == nullptr )
    {
        THROW_IO_ERROR( wxString::Format( _( "Cannot open file '%s'." ), aFileName ) );
    }

    fseek( fp, 0, SEEK_END );
    long len = ftell( fp );

    if( len < 0 )
    {
        fclose( fp );
        THROW_IO_ERROR(
                wxString::Format( _( "Cannot determine length of file '%s'." ), aFileName ) );
    }

    m_data.resize( static_cast<size_t>( len ) );

    fseek( fp, 0, SEEK_SET );

    size_t bytesRead = fread( m_data.data(), 1, m_data.size(), fp );
    fclose( fp );

    if( bytesRead != m_data.size() )
    {
        THROW_IO_ERROR( wxString::Format( _( "Error reading file '%s'." ), aFileName ) );
    }
}


BINARY_READER::~BINARY_READER()
{
}


// --- Position management ----------------------------------------------------


void BINARY_READER::SetOffset( size_t aOffset )
{
    if( aOffset > m_data.size() )
    {
        THROW_IO_ERROR( wxString::Format( _( "Seek past end of file (offset %zu, size %zu)." ),
                                          aOffset, m_data.size() ) );
    }

    m_offset = aOffset;
}


void BINARY_READER::Skip( size_t aBytes )
{
    if( m_offset + aBytes > m_data.size() )
        ThrowEOFError( aBytes );

    m_offset += aBytes;
}


// --- Primitive readers ------------------------------------------------------


uint8_t BINARY_READER::ReadByte()
{
    if( m_offset + 1 > m_data.size() )
        ThrowEOFError( 1 );

    uint8_t val = m_data[m_offset];
    m_offset += 1;
    return val;
}


int BINARY_READER::ReadInt3()
{
    if( m_offset + 3 > m_data.size() )
        ThrowEOFError( 3 );

    const uint8_t* p = &m_data[m_offset];
    int raw = ( static_cast<int>( p[0] ) << 16 )
            | ( static_cast<int>( p[1] ) << 8 )
            | ( static_cast<int>( p[2] ) );
    m_offset += 3;
    return raw - INT3_BIAS;
}


int BINARY_READER::ReadInt4()
{
    if( m_offset + 4 > m_data.size() )
        ThrowEOFError( 4 );

    const uint8_t* p = &m_data[m_offset];
    unsigned int raw = ( static_cast<unsigned int>( p[0] ) << 24 )
                     | ( static_cast<unsigned int>( p[1] ) << 16 )
                     | ( static_cast<unsigned int>( p[2] ) << 8 )
                     | ( static_cast<unsigned int>( p[3] ) );
    m_offset += 4;
    return static_cast<int>( raw ) - INT4_BIAS;
}


wxString BINARY_READER::ReadString()
{
    if( m_stringEncoding == STRING_ENCODING::UTF16_BE )
        return ReadStringUTF16();

    if( m_stringEncoding == STRING_ENCODING::LEGACY_ASCII
        || m_version <= LEGACY_STRING_VERSION )
    {
        return ReadStringASCII();
    }

    return ReadStringUTF16();
}


void BINARY_READER::ReadColor( uint8_t& r, uint8_t& g, uint8_t& b )
{
    r = ReadByte();
    g = ReadByte();
    b = ReadByte();
}


void BINARY_READER::ReadBytes( uint8_t* aDst, size_t aCount )
{
    if( m_offset + aCount > m_data.size() )
        ThrowEOFError( aCount );

    std::memcpy( aDst, &m_data[m_offset], aCount );
    m_offset += aCount;
}


// --- Peek methods -----------------------------------------------------------


int BINARY_READER::PeekInt3() const
{
    if( m_offset + 3 > m_data.size() )
    {
        THROW_IO_ERROR( wxString::Format(
                _( "Unexpected end of file at offset 0x%06zX: need 3 bytes for int3, "
                   "have %zu remaining." ),
                m_offset, m_data.size() - m_offset ) );
    }

    const uint8_t* p = &m_data[m_offset];
    int raw = ( static_cast<int>( p[0] ) << 16 )
            | ( static_cast<int>( p[1] ) << 8 )
            | ( static_cast<int>( p[2] ) );
    return raw - INT3_BIAS;
}


int BINARY_READER::PeekInt4() const
{
    if( m_offset + 4 > m_data.size() )
    {
        THROW_IO_ERROR( wxString::Format(
                _( "Unexpected end of file at offset 0x%06zX: need 4 bytes for int4, "
                   "have %zu remaining." ),
                m_offset, m_data.size() - m_offset ) );
    }

    const uint8_t* p = &m_data[m_offset];
    unsigned int raw = ( static_cast<unsigned int>( p[0] ) << 24 )
                     | ( static_cast<unsigned int>( p[1] ) << 16 )
                     | ( static_cast<unsigned int>( p[2] ) << 8 )
                     | ( static_cast<unsigned int>( p[3] ) );
    return static_cast<int>( raw ) - INT4_BIAS;
}


uint8_t BINARY_READER::PeekByte() const
{
    if( m_offset >= m_data.size() )
    {
        THROW_IO_ERROR( wxString::Format(
                _( "Unexpected end of file at offset 0x%06zX: need 1 byte." ), m_offset ) );
    }

    return m_data[m_offset];
}


// --- Coordinate conversion --------------------------------------------------


int BINARY_READER::DipTraceToKiCadNm( int aDipTraceCoord )
{
    return static_cast<int>( static_cast<int64_t>( aDipTraceCoord ) * 100 / 3 );
}


double BINARY_READER::DipTraceToMM( int aDipTraceCoord )
{
    return static_cast<double>( aDipTraceCoord ) * DIPTRACE_COORD_TO_MM;
}


// --- Search helpers ---------------------------------------------------------


size_t BINARY_READER::FindPattern( const uint8_t* aPattern, size_t aPatternLen,
                                   size_t aStart, size_t aEnd ) const
{
    if( aEnd == 0 || aEnd > m_data.size() )
        aEnd = m_data.size();

    if( aStart >= aEnd || aPatternLen == 0 || aPatternLen > ( aEnd - aStart ) )
        return std::string::npos;

    auto it = std::search( m_data.begin() + aStart,
                           m_data.begin() + aEnd,
                           aPattern,
                           aPattern + aPatternLen );

    if( it == m_data.begin() + aEnd )
        return std::string::npos;

    return static_cast<size_t>( it - m_data.begin() );
}


size_t BINARY_READER::FindString( const wxString& aStr, size_t aStart, size_t aEnd ) const
{
    if( aStr.IsEmpty() )
        return std::string::npos;

    // Encode the string as UTF-16-BE, which is the v39+ on-disk representation.
    // The on-disk format has a 2-byte length prefix before the encoded characters.
    wxMBConvUTF16BE conv;

    // wxMBConvUTF16BE::FromWChar includes a BOM; we must skip it.
    // We encode manually: each wxChar becomes 2 bytes in UTF-16-BE.
    size_t charCount = aStr.length();
    std::vector<uint8_t> encoded( charCount * 2 );

    for( size_t i = 0; i < charCount; i++ )
    {
        wxChar ch = aStr[i];
        encoded[i * 2]     = static_cast<uint8_t>( ( ch >> 8 ) & 0xFF );
        encoded[i * 2 + 1] = static_cast<uint8_t>( ch & 0xFF );
    }

    // Search for the encoded character data in the file buffer.
    size_t matchPos = FindPattern( encoded.data(), encoded.size(), aStart, aEnd );

    if( matchPos == std::string::npos )
        return std::string::npos;

    // The length prefix sits 2 bytes before the encoded character data.
    if( matchPos < 2 )
        return std::string::npos;

    return matchPos - 2;
}


// --- Try-read methods -------------------------------------------------------


bool BINARY_READER::TryReadString( wxString& aResult )
{
    if( m_stringEncoding == STRING_ENCODING::UTF16_BE )
        return TryReadStringUTF16( aResult );

    if( m_stringEncoding == STRING_ENCODING::LEGACY_ASCII
        || m_version <= LEGACY_STRING_VERSION )
    {
        return TryReadStringASCII( aResult );
    }

    return TryReadStringUTF16( aResult );
}


void BINARY_READER::DetectStringEncoding( size_t aProbeOffset )
{
    size_t          savedOffset = m_offset;
    STRING_ENCODING savedEncoding = m_stringEncoding;

    // The probe helpers below dispatch on m_stringEncoding, so force each framing explicitly.
    m_offset = aProbeOffset;
    m_stringEncoding = STRING_ENCODING::LEGACY_ASCII;
    wxString asciiStr;
    bool     asciiOk = TryReadStringASCII( asciiStr ) && !asciiStr.IsEmpty();

    m_offset = aProbeOffset;
    m_stringEncoding = STRING_ENCODING::UTF16_BE;
    wxString utf16Str;
    bool     utf16Ok = TryReadStringUTF16( utf16Str ) && !utf16Str.IsEmpty();

    m_offset = savedOffset;
    m_stringEncoding = savedEncoding;

    // Only commit when exactly one framing yields a printable string; otherwise leave the
    // version-based default in place.
    if( asciiOk && !utf16Ok )
        m_stringEncoding = STRING_ENCODING::LEGACY_ASCII;
    else if( utf16Ok && !asciiOk )
        m_stringEncoding = STRING_ENCODING::UTF16_BE;
}


// --- Private string readers -------------------------------------------------


wxString BINARY_READER::ReadStringUTF16()
{
    if( m_offset + 2 > m_data.size() )
        ThrowEOFError( 2 );

    const uint8_t* p = &m_data[m_offset];
    int charCount = ( static_cast<int>( p[0] ) << 8 ) | static_cast<int>( p[1] );
    m_offset += 2;

    if( charCount == 0 )
        return wxString();

    if( charCount < 0 || charCount > MAX_STRING_CHARS )
    {
        THROW_IO_ERROR( wxString::Format(
                _( "Unreasonable string length %d at offset 0x%06zX." ),
                charCount, m_offset - 2 ) );
    }

    size_t byteCount = static_cast<size_t>( charCount ) * 2;

    if( m_offset + byteCount > m_data.size() )
        ThrowEOFError( byteCount );

    // wxMBConvUTF16BE converts from a big-endian byte stream.
    wxMBConvUTF16BE conv;
    wxString result = wxString( reinterpret_cast<const char*>( &m_data[m_offset] ),
                                conv, byteCount );

    m_offset += byteCount;
    return result;
}


wxString BINARY_READER::ReadStringASCII()
{
    int byteCount = ReadInt3();

    if( byteCount == 0 )
        return wxString();

    if( byteCount < 0 || byteCount > MAX_STRING_CHARS )
    {
        THROW_IO_ERROR( wxString::Format(
                _( "Unreasonable v37 string length %d at offset 0x%06zX." ),
                byteCount, m_offset - 3 ) );
    }

    size_t count = static_cast<size_t>( byteCount );

    if( m_offset + count > m_data.size() )
        ThrowEOFError( count );

    wxString result = wxString::From8BitData(
            reinterpret_cast<const char*>( &m_data[m_offset] ), count );
    m_offset += count;
    return result;
}


bool BINARY_READER::TryReadStringUTF16( wxString& aResult )
{
    size_t savedOffset = m_offset;

    if( m_offset + 2 > m_data.size() )
        return false;

    const uint8_t* p = &m_data[m_offset];
    int charCount = ( static_cast<int>( p[0] ) << 8 ) | static_cast<int>( p[1] );
    m_offset += 2;

    if( charCount == 0 )
    {
        aResult = wxString();
        return true;
    }

    if( charCount < 0 || charCount > 500 )
    {
        m_offset = savedOffset;
        return false;
    }

    size_t byteCount = static_cast<size_t>( charCount ) * 2;

    if( m_offset + byteCount > m_data.size() )
    {
        m_offset = savedOffset;
        return false;
    }

    wxMBConvUTF16BE conv;
    wxString candidate = wxString( reinterpret_cast<const char*>( &m_data[m_offset] ),
                                   conv, byteCount );

    if( !IsPrintableString( candidate ) )
    {
        m_offset = savedOffset;
        return false;
    }

    m_offset += byteCount;
    aResult = candidate;
    return true;
}


bool BINARY_READER::TryReadStringASCII( wxString& aResult )
{
    size_t savedOffset = m_offset;

    if( m_offset + 3 > m_data.size() )
        return false;

    const uint8_t* p = &m_data[m_offset];
    int byteCount = ( static_cast<int>( p[0] ) << 16 )
                  | ( static_cast<int>( p[1] ) << 8 )
                  | ( static_cast<int>( p[2] ) );
    byteCount -= INT3_BIAS;
    m_offset += 3;

    if( byteCount == 0 )
    {
        aResult = wxString();
        return true;
    }

    if( byteCount < 0 || byteCount > 500 )
    {
        m_offset = savedOffset;
        return false;
    }

    size_t count = static_cast<size_t>( byteCount );

    if( m_offset + count > m_data.size() )
    {
        m_offset = savedOffset;
        return false;
    }

    wxString candidate = wxString::From8BitData(
            reinterpret_cast<const char*>( &m_data[m_offset] ), count );

    if( !IsPrintableString( candidate ) )
    {
        m_offset = savedOffset;
        return false;
    }

    m_offset += count;
    aResult = candidate;
    return true;
}


bool BINARY_READER::IsPrintableString( const wxString& aStr )
{
    for( size_t i = 0; i < aStr.length(); i++ )
    {
        wxChar ch = aStr[i];

        if( ch == '\r' || ch == '\n' || ch == '\t' )
            continue;

        if( ch < 0x20 )
            return false;
    }

    return true;
}


void BINARY_READER::ThrowEOFError( size_t aBytesNeeded ) const
{
    size_t remaining = ( m_offset < m_data.size() ) ? ( m_data.size() - m_offset ) : 0;

    THROW_IO_ERROR( wxString::Format(
            _( "Unexpected end of file at offset 0x%06zX: need %zu bytes, have %zu remaining." ),
            m_offset, aBytesNeeded, remaining ) );
}
