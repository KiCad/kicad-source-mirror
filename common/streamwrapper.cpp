/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Cirilo Bernardo <cirilo.bernardo@gmail.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file streamwrapper.cpp
 */

#if !defined( _WIN32 ) || !defined( __GNUC__ )
    #error streamwrapper.cpp should not be included in this build
#endif

#include "streamwrapper.h"

#include <wx/string.h>
#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


kicad::stream::stream()
{
    m_buf = nullptr;
    m_stream = nullptr;
}


kicad::stream::~stream()
{
    if( nullptr != m_stream )
        delete m_stream;

    if( nullptr != m_buf )
    {
        m_buf->close(); // ensure file is closed regardless of m_buf's destructor
        delete m_buf;
    }
}


std::iostream* kicad::stream::Open( const char* aFileName, std::ios_base::openmode aMode )
{
    if( nullptr != m_stream )
    {
        delete m_stream;
        m_stream = nullptr;
    }

    if( nullptr != m_buf )
    {
        m_buf->close();
        delete m_buf;
    }

    int flags = 0;

    if( aMode & std::ios_base::app )
        flags |= _O_APPEND;

    if( aMode & std::ios_base::out && aMode & std::ios_base::in )
        flags |= _O_RDWR;
    else if( aMode & std::ios_base::out )
        flags |= _O_WRONLY;
    else if( aMode & std::ios_base::in )
        flags |= _O_RDONLY;

    if( aMode & std::ios_base::binary )
        flags |= _O_BINARY;

    if( aMode & std::ios_base::out && aMode & std::ios_base::trunc
        && !( aMode & std::ios_base::app ) && !( aMode & std::ios_base::ate ) )
        flags |= _O_TRUNC;

    if( aMode & std::ios_base::out )
        flags |= _O_CREAT;

    //int fd = open( "testfile.txt", flags, S_IRUSR | S_IWUSR );
    wxString lstr( wxString::FromUTF8Unchecked( aFileName ) );
    int fd = _wopen( lstr.wc_str(), flags, _S_IREAD | _S_IWRITE );

    if( fd >= 0 && aMode & std::ios_base::ate )
        lseek( fd, 0, SEEK_END );

    // NOTE: _O_RDONLY in Windows, O_RDONLY in Linux
    m_buf = new __gnu_cxx::stdio_filebuf<char>( fd, aMode );

    m_stream = new std::iostream( m_buf );

    return m_stream;
}


void kicad::stream::Close( void )
{
    if( m_buf )
        m_buf->close();
}


std::iostream* kicad::stream::GetStream( void )
{
    return m_stream;
}
