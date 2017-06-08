
/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007-2010 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2007 KiCad Developers, see change_log.txt for contributors.
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

#include <string.h>
#include <richio.h>
#include <filter_reader.h>


FILTER_READER::FILTER_READER( LINE_READER& aReader ) :
    LINE_READER( 1 ),
    reader( aReader )
{
    // Not using our own line buffer, will be using aReader's.  This changes
    // the meaning of this->line to be merely a pointer to aReader's line, which of course
    // is not owned here.
    delete [] line;

    line = 0;
}


FILTER_READER::~FILTER_READER()
{
    // Our 'line' points to aReader's, and he will delete that buffer.
    // Prevent subsequent call to ~LINE_READER() from deleting a buffer we do not own.
    line = 0;
}


char* FILTER_READER::ReadLine()
{
    char* s;

    while( ( s = reader.ReadLine() ) != NULL )
    {
        if( !strchr( "#\n\r", s[0] ) )
            break;
    }

    line   = reader.Line();
    length = reader.Length();

    return length ? line : NULL;
}


WHITESPACE_FILTER_READER::WHITESPACE_FILTER_READER( LINE_READER& aReader ) :
    LINE_READER( 1 ),
    reader( aReader )
{
    // Not using our own line buffer, will be using aReader's.  This changes
    // the meaning of this->line to be merely a pointer to aReader's line, which of course
    // is not owned here.
    delete [] line;

    line = 0;
}


WHITESPACE_FILTER_READER::~WHITESPACE_FILTER_READER()
{
    // Our 'line' points to aReader's, and he will delete that buffer.
    // Prevent subsequent call to ~LINE_READER() from deleting a buffer we do not own.
    line = 0;
}


char* WHITESPACE_FILTER_READER::ReadLine()
{
    char* s;

    while( ( s = reader.ReadLine() ) != NULL )
    {
        while( s != NULL && strchr( " \t", *s ) )
            s++;

        if( s != NULL && !strchr( "#\n\r", *s ) )
            break;
    }

    line   = s;
    length = reader.Length();

    return length ? line : NULL;
}
