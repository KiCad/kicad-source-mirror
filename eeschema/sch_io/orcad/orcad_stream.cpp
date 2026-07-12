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

#include <sch_io/orcad/orcad_stream.h>

#include <cstring>

#include <ki_exception.h>
#include <wx/strconv.h>


static wxString hexOf( const uint8_t* aBytes, size_t aCount )
{
    wxString out;

    for( size_t i = 0; i < aCount; i++ )
        out += wxString::Format( wxS( "%02x" ), aBytes[i] );

    return out;
}


ORCAD_STREAM::ORCAD_STREAM( const void* aData, size_t aLength ) :
        m_data( static_cast<const uint8_t*>( aData ) ),
        m_size( aLength ),
        m_offset( 0 )
{
}


ORCAD_STREAM::ORCAD_STREAM( const std::vector<char>& aData ) :
        ORCAD_STREAM( aData.data(), aData.size() )
{
}


void ORCAD_STREAM::requireBytes( size_t aCount ) const
{
    if( m_offset > m_size || aCount > m_size - m_offset )
    {
        THROW_IO_ERROR( wxString::Format( wxS( "OrCAD stream: read of %zu bytes past end at 0x%zx "
                                               "(stream size 0x%zx)" ),
                                          aCount, m_offset, m_size ) );
    }
}


uint8_t ORCAD_STREAM::ReadU8()
{
    requireBytes( 1 );
    return m_data[m_offset++];
}


int8_t ORCAD_STREAM::ReadI8()
{
    return static_cast<int8_t>( ReadU8() );
}


uint16_t ORCAD_STREAM::ReadU16()
{
    requireBytes( 2 );
    uint16_t v = static_cast<uint16_t>( m_data[m_offset] )
                 | static_cast<uint16_t>( m_data[m_offset + 1] ) << 8;
    m_offset += 2;
    return v;
}


int16_t ORCAD_STREAM::ReadI16()
{
    return static_cast<int16_t>( ReadU16() );
}


uint32_t ORCAD_STREAM::ReadU32()
{
    requireBytes( 4 );
    uint32_t v = static_cast<uint32_t>( m_data[m_offset] )
                 | static_cast<uint32_t>( m_data[m_offset + 1] ) << 8
                 | static_cast<uint32_t>( m_data[m_offset + 2] ) << 16
                 | static_cast<uint32_t>( m_data[m_offset + 3] ) << 24;
    m_offset += 4;
    return v;
}


int32_t ORCAD_STREAM::ReadI32()
{
    return static_cast<int32_t>( ReadU32() );
}


std::string ORCAD_STREAM::ReadLzt()
{
    size_t   lenPos = m_offset;
    uint16_t len = ReadU16();

    if( m_offset > m_size || static_cast<size_t>( len ) + 1 > m_size - m_offset )
    {
        THROW_IO_ERROR( wxString::Format( wxS( "OrCAD stream: string of length %u at 0x%zx "
                                               "exceeds stream" ),
                                          len, lenPos ) );
    }

    std::string s( reinterpret_cast<const char*>( m_data + m_offset ), len );
    m_offset += len;

    if( m_data[m_offset] != 0 )
    {
        THROW_IO_ERROR( wxString::Format( wxS( "OrCAD stream: string at 0x%zx missing NUL "
                                               "terminator" ),
                                          lenPos ) );
    }

    m_offset++;
    return s;
}


std::string ORCAD_STREAM::ReadZt()
{
    if( m_offset >= m_size )
        THROW_IO_ERROR( wxString::Format( wxS( "OrCAD stream: read past end at 0x%zx" ), m_offset ) );

    const void* nul = memchr( m_data + m_offset, 0, m_size - m_offset );

    if( !nul )
    {
        THROW_IO_ERROR( wxString::Format( wxS( "OrCAD stream: unterminated string at 0x%zx" ),
                                          m_offset ) );
    }

    size_t      end = static_cast<const uint8_t*>( nul ) - m_data;
    std::string s( reinterpret_cast<const char*>( m_data + m_offset ), end - m_offset );
    m_offset = end + 1;
    return s;
}


std::vector<uint8_t> ORCAD_STREAM::ReadBytes( size_t aCount )
{
    requireBytes( aCount );
    std::vector<uint8_t> v( m_data + m_offset, m_data + m_offset + aCount );
    m_offset += aCount;
    return v;
}


void ORCAD_STREAM::Skip( size_t aCount )
{
    if( m_offset > m_size || aCount > m_size - m_offset )
    {
        THROW_IO_ERROR( wxString::Format( wxS( "OrCAD stream: skip of %zu bytes past end at 0x%zx "
                                               "(stream size 0x%zx)" ),
                                          aCount, m_offset, m_size ) );
    }

    m_offset += aCount;
}


int ORCAD_STREAM::PeekU8( size_t aAhead ) const
{
    size_t pos = m_offset + aAhead;

    if( pos >= m_size )
        return -1;

    return m_data[pos];
}


bool ORCAD_STREAM::PeekMatches( const uint8_t* aBytes, size_t aCount, size_t aAhead ) const
{
    size_t pos = m_offset + aAhead;

    if( pos > m_size || aCount > m_size - pos )
        return false;

    return memcmp( m_data + pos, aBytes, aCount ) == 0;
}


bool ORCAD_STREAM::AtPreamble( size_t aAhead ) const
{
    return PeekMatches( PREAMBLE, 4, aAhead );
}


bool ORCAD_STREAM::HasPreambleAt( size_t aAbsoluteOffset ) const
{
    if( aAbsoluteOffset > m_size || 4 > m_size - aAbsoluteOffset )
        return false;

    return memcmp( m_data + aAbsoluteOffset, PREAMBLE, 4 ) == 0;
}


size_t ORCAD_STREAM::FindPreamble( size_t aFrom ) const
{
    if( m_size < 4 )
        return npos;

    for( size_t pos = aFrom; pos + 4 <= m_size; pos++ )
    {
        if( m_data[pos] == PREAMBLE[0] && memcmp( m_data + pos, PREAMBLE, 4 ) == 0 )
            return pos;
    }

    return npos;
}


void ORCAD_STREAM::Expect( const uint8_t* aBytes, size_t aCount, const wxString& aWhat )
{
    size_t               pos = m_offset;
    std::vector<uint8_t> got = ReadBytes( aCount );

    if( memcmp( got.data(), aBytes, aCount ) != 0 )
    {
        THROW_IO_ERROR( wxString::Format( wxS( "OrCAD stream: expected %s got %s at 0x%zx (%s)" ),
                                          hexOf( aBytes, aCount ), hexOf( got.data(), aCount ),
                                          pos, aWhat ) );
    }
}


void ORCAD_STREAM::ExpectByte( uint8_t aValue, const wxString& aWhat )
{
    Expect( &aValue, 1, aWhat );
}


void ORCAD_STREAM::ExpectPreamble( const wxString& aWhat )
{
    Expect( PREAMBLE, 4, aWhat );
}


void ORCAD_STREAM::SkipOptionalPreambleBlock()
{
    if( AtPreamble() )
    {
        Skip( 4 );
        uint32_t trail = ReadU32();
        Skip( trail );
    }
}


wxString FromOrcadString( const std::string& aText )
{
    if( aText.empty() )
        return wxEmptyString;

    wxString converted( aText.c_str(), wxCSConv( wxFONTENCODING_CP1252 ) );

    if( converted.IsEmpty() )
        converted = wxString::From8BitData( aText.c_str(), aText.size() );

    // OrCAD multi-line text uses CR LF; raw CR renders as replacement glyph in KiCad.
    converted.Replace( wxS( "\r\n" ), wxS( "\n" ) );
    converted.Replace( wxS( "\r" ), wxS( "\n" ) );

    return converted;
}
