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

#include <istream>
#include <string>
#include <bit>
#include <memory>
#include <cstdint>
#include <algorithm>

#include <ki_exception.h>
#include <convert/allegro_pcb_structs.h>

namespace ALLEGRO
{

/**
 * This is a simple stream that wraps an input stream and can return
 * basic primitive types from an Allegro .brd (or .dra) file.
 */
class FILE_STREAM
{
    static constexpr bool m_kNeedsSwap = std::endian::native != std::endian::little;

public:
    explicit FILE_STREAM( std::istream& stream ) : m_stream( stream ) {}

    size_t Position() const { return m_stream.tellg(); }
    void   Seek( size_t aPos ) { m_stream.seekg( aPos ); }
    void   Skip( size_t aBytes ) { m_stream.seekg( aBytes, std::ios::cur ); }

    bool Eof() const { return m_stream.eof(); }

    /**
     * Read a number of bytes from the stream into the destination buffer.
     *
     * It is your responsibility to ensure that the destination buffer is large enough
     * to hold the requested number of bytes.
     */
    void ReadBytes( void* aDest, size_t aSize )
    {
        m_stream.read( static_cast<char*>( aDest ), static_cast<std::streamsize>( aSize ) );
        if( m_stream.gcount() != static_cast<std::streamsize>( aSize ) )
        {
            THROW_IO_ERROR( wxString::Format( "Failed to read requested %zu bytes at offset %zu", aSize, Position() ) );
        }
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
        std::string str;
        while( true )
        {
            const char c = Read<char>();
            if( c == '\0' )
                break;
            str += c;
        }

        const size_t pos = Position();

        if( aRoundToNextU32 && pos % sizeof( uint32_t ) != 0 )
        {
            const size_t padding = sizeof( uint32_t ) - ( pos % sizeof( uint32_t ) );
            Skip( padding );
        }

        return str;
    }

    std::string ReadStringFixed( size_t aLen, bool aRoundToNextU32 )
    {
        std::vector<char> buffer( aLen );
        ReadBytes( buffer.data(), aLen );
        buffer.push_back( '\0' );

        const size_t pos = Position();
        if( aRoundToNextU32 && pos % sizeof( uint32_t ) != 0 )
        {
            const size_t padding = sizeof( uint32_t ) - ( pos % sizeof( uint32_t ) );
            Skip( padding );
        }

        return { buffer.data() };
    }

    // Read a single byte from the stream, returning false if the stream is at EOF
    bool GetU8( uint8_t& value )
    {
        const int ch = m_stream.get();

        if( m_stream.eof() )
        {
            return false;
        }

        value = static_cast<uint8_t>( ch );
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

    std::istream& m_stream;
};

} // namespace ALLEGRO
