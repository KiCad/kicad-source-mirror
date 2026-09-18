/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-3.0.html
 */

#include "vrml_line_reader.h"

#include "../model_import_internal.h"

#include <wx/wxcrt.h>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <stdexcept>


VRML_LINE_READER::VRML_LINE_READER( const wxString& aFileName, unsigned aStartingLineNumber, unsigned aMaxLineLength,
                                    const S3D::MODEL_IMPORT_OPTIONS& aOptions ) :
        m_file( wxFopen( aFileName, wxS( "rb" ) ) ),
        m_source( aFileName ),
        m_buffer( 64 * 1024 ),
        m_lineNumber( aStartingLineNumber ),
        m_maxLineLength( aMaxLineLength ),
        m_options( aOptions )
{
    if( !m_file )
        throw std::runtime_error( "Unable to open model file" );

    m_line.reserve( std::min( aMaxLineLength, 4096U ) + 1U );
}


VRML_LINE_READER::~VRML_LINE_READER()
{
    std::fclose( m_file );
}


bool VRML_LINE_READER::fill()
{
    m_bufferPos = 0;
    m_bufferEnd = std::fread( m_buffer.data(), 1, m_buffer.size(), m_file );

    if( m_bufferEnd == 0 && std::ferror( m_file ) )
        throw std::runtime_error( "Unable to read model file" );

    return m_bufferEnd != 0;
}


char* VRML_LINE_READER::ReadLine()
{
    if( m_options.IsCanceled() )
        throw MODEL_IMPORT_CANCELED();

    m_line.clear();

    for( ;; )
    {
        if( m_bufferPos == m_bufferEnd && !fill() )
            break;

        const char* start = m_buffer.data() + m_bufferPos;
        const char* found = static_cast<const char*>( std::memchr( start, '\n', m_bufferEnd - m_bufferPos ) );
        const std::size_t span = found ? static_cast<std::size_t>( found - start ) + 1U : m_bufferEnd - m_bufferPos;

        if( m_line.size() + span > m_maxLineLength )
            throw std::runtime_error( "Maximum model line length exceeded" );

        m_line.append( start, span );
        m_bufferPos += span;

        if( found )
            break;
    }

    ++m_lineNumber;

    if( m_line.empty() )
        return nullptr;

    m_line.push_back( '\0' );
    return m_line.data();
}
