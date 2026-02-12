/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright Quilter
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

#include <string>
#include <bit>
#include <cstdint>
#include <cstring>
#include <algorithm>

#include <ki_exception.h>
#include <convert/allegro_pcb_structs.h>

namespace ALLEGRO
{

/**
 * Stream that reads primitive types from a memory buffer containing
 * Allegro .brd (or .dra) file data. All multi-byte values are little-endian.
 */
class FILE_STREAM
{
    static constexpr bool m_kNeedsSwap = std::endian::native != std::endian::little;

public:
    FILE_STREAM( const uint8_t* aData, size_t aSize ) :
            m_data( aData ), m_size( aSize ), m_pos( 0 )
    {}

    size_t Position() const { return m_pos; }

    void Seek( size_t aPos )
    {
        if( aPos > m_size )
        {
            THROW_IO_ERROR( wxString::Format(
                    "Seek past end of file: offset %zu exceeds file size %zu", aPos, m_size ) );
        }

        m_pos = aPos;
    }

    void Skip( size_t aBytes )
    {
        if( aBytes > m_size - m_pos )
        {
            THROW_IO_ERROR( wxString::Format(
                    "Skip past end of file at offset %zu", m_pos ) );
        }

        m_pos += aBytes;
    }

    bool Eof() const { return m_pos >= m_size; }

    /**
     * Read a number of bytes from the stream into the destination buffer.
     *
     * It is your responsibility to ensure that the destination buffer is large enough
     * to hold the requested number of bytes.
     */
    void ReadBytes( void* aDest, size_t aSize )
    {
        if( aSize > m_size || m_pos > m_size - aSize )
        {
            THROW_IO_ERROR( wxString::Format(
                    "Failed to read requested %zu bytes at offset %zu (file size %zu)",
                    aSize, m_pos, m_size ) );
        }

        std::memcpy( aDest, m_data + m_pos, aSize );
        m_pos += aSize;
    }

    // Primitive type reading (file is little-endian)
    template <typename T>
    T Read()
    {
        static_assert( std::is_fundamental_v<T>, "Read() only works for fundamental types" );

        T value;
        ReadBytes( &value, sizeof( T ) );

        if constexpr( ( sizeof( T ) > 1 ) && m_kNeedsSwap )
        {
            swapBytes( value );
        }

        return value;
    }

    std::string ReadString( bool aRoundToNextU32 )
    {
        if( m_pos > m_size )
        {
            THROW_IO_ERROR( wxString::Format(
                    "ReadString at invalid offset %zu (file size %zu)", m_pos, m_size ) );
        }

        const void* found = std::memchr( m_data + m_pos, '\0', m_size - m_pos );

        if( !found )
        {
            THROW_IO_ERROR( wxString::Format(
                    "Unterminated string at offset %zu", m_pos ) );
        }

        const uint8_t* end = static_cast<const uint8_t*>( found );
        std::string str( reinterpret_cast<const char*>( m_data + m_pos ), end - ( m_data + m_pos ) );
        m_pos = static_cast<size_t>( end - m_data ) + 1;

        if( aRoundToNextU32 && m_pos % sizeof( uint32_t ) != 0 )
        {
            const size_t padding = sizeof( uint32_t ) - ( m_pos % sizeof( uint32_t ) );
            Skip( padding );
        }

        return str;
    }

    std::string ReadStringFixed( size_t aLen, bool aRoundToNextU32 )
    {
        if( aLen > m_size || m_pos > m_size - aLen )
        {
            THROW_IO_ERROR( wxString::Format(
                    "Failed to read fixed string of %zu bytes at offset %zu (file size %zu)",
                    aLen, m_pos, m_size ) );
        }

        const char* start = reinterpret_cast<const char*>( m_data + m_pos );
        size_t      strLen = strnlen( start, aLen );
        std::string str( start, strLen );
        m_pos += aLen;

        if( aRoundToNextU32 && m_pos % sizeof( uint32_t ) != 0 )
        {
            const size_t padding = sizeof( uint32_t ) - ( m_pos % sizeof( uint32_t ) );
            Skip( padding );
        }

        return str;
    }

    // Read a single byte from the stream, returning false if the stream is at EOF
    bool GetU8( uint8_t& value )
    {
        if( m_pos >= m_size )
            return false;

        value = m_data[m_pos++];
        return true;
    }

    uint8_t  ReadU8() { return Read<uint8_t>(); }
    uint16_t ReadU16() { return Read<uint16_t>(); }
    int16_t  ReadS16() { return Read<int16_t>(); }
    uint32_t ReadU32() { return Read<uint32_t>(); }
    int32_t  ReadS32() { return Read<int32_t>(); }

    void SkipU32( size_t n = 1 ) { Skip( sizeof( uint32_t ) * n ); }

private:
    template <typename T>
    static void swapBytes( T& value )
    {
        char* bytes = reinterpret_cast<char*>( &value );
        std::reverse( bytes, bytes + sizeof( T ) );
    }

    const uint8_t* m_data;
    size_t         m_size;
    size_t         m_pos;
};

} // namespace ALLEGRO
