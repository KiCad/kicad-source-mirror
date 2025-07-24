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

#include <qa_utils/stdstream_line_reader.h>

#include <ios>

STDISTREAM_LINE_READER::STDISTREAM_LINE_READER() :
    LINE_READER( 0 ),
    m_stream( nullptr )
{
    m_line = nullptr;
    m_lineNum = 0;
}


STDISTREAM_LINE_READER::~STDISTREAM_LINE_READER()
{
    // this is only a view into a string, it can't be deleted by the base
    m_line = nullptr;
}


char* STDISTREAM_LINE_READER::ReadLine()
{
    getline( *m_stream, m_buffer );

    m_buffer.append( 1, '\n' );

    m_length = m_buffer.size();
    m_line = (char*) m_buffer.data(); //ew why no const??

    // lineNum is incremented even if there was no line read, because this
    // leads to better error reporting when we hit an end of file.
    ++m_lineNum;

    return m_stream->eof() ? nullptr : m_line;
}


void STDISTREAM_LINE_READER::SetStream( std::istream& aStream )
{
    // Could be done with a virtual getStream function, but the
    // virtual function call is a noticeable (but minor) penalty within
    // ReadLine() in tight loops
    m_stream = &aStream;
}


IFSTREAM_LINE_READER::IFSTREAM_LINE_READER( const wxFileName& aFileName )  :
        m_fStream( aFileName.GetFullPath().fn_str() )
{
    if( !m_fStream.is_open() )
    {
        wxString msg = wxString::Format(
            _( "Unable to open filename '%s' for reading" ), aFileName.GetFullPath().GetData() );
        THROW_IO_ERROR( msg );
    }

    SetStream( m_fStream );

    m_source = aFileName.GetFullPath();
}


void IFSTREAM_LINE_READER::Rewind()
{
    m_fStream.clear() ;
    m_fStream.seekg(0, std::ios::beg );
}
