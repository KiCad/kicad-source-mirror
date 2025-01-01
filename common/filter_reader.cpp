
/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007-2010 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
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

#include <cstring>
#include <filter_reader.h>
#include <richio.h>


FILTER_READER::FILTER_READER( LINE_READER& aReader ) :
    LINE_READER( 1 ),
    reader( aReader )
{
    // Not using our own m_line buffer, will be using aReader's.  This changes
    // the meaning of this->m_line to be merely a pointer to aReader's m_line, which of course
    // is not owned here.
    delete [] m_line;

    m_line = nullptr;
}


FILTER_READER::~FILTER_READER()
{
    // Our 'm_line' points to aReader's, and he will delete that buffer.
    // Prevent subsequent call to ~LINE_READER() from deleting a buffer we do not own.
    m_line = nullptr;
}


char* FILTER_READER::ReadLine()
{
    char* s;

    while( ( s = reader.ReadLine() ) != nullptr )
    {
        if( !strchr( "#\n\r", s[0] ) )
            break;
    }

    m_line   = reader.Line();
    m_length = reader.Length();

    return m_length ? m_line : nullptr;
}


WHITESPACE_FILTER_READER::WHITESPACE_FILTER_READER( LINE_READER& aReader ) :
    LINE_READER( 1 ),
    reader( aReader )
{
    // Not using our own m_line buffer, will be using aReader's.  This changes
    // the meaning of this->m_line to be merely a pointer to aReader's m_line, which of course
    // is not owned here.
    delete [] m_line;

    m_line = nullptr;
}


WHITESPACE_FILTER_READER::~WHITESPACE_FILTER_READER()
{
    // Our 'm_line' points to aReader's, and he will delete that buffer.
    // Prevent subsequent call to ~LINE_READER() from deleting a buffer we do not own.
    m_line = nullptr;
}


char* WHITESPACE_FILTER_READER::ReadLine()
{
    char* s;

    while( ( s = reader.ReadLine() ) != nullptr )
    {
        while( s != nullptr && strchr( " \t", *s ) )
            s++;

        if( s != nullptr && !strchr( "#\n\r", *s ) )
            break;
    }

    m_line   = s;
    m_length = reader.Length();

    return m_length ? m_line : nullptr;
}
