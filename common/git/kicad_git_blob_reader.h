/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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

#include <streambuf>
#include <istream>
#include <string>
#include <git2.h>
#include <import_export.h>

#include <richio.h>


class APIEXPORT BLOB_BUFFER_STREAM : public std::streambuf
{
public:
    BLOB_BUFFER_STREAM( git_blob* aBlob )
    {
        // Yay C++
        setg( const_cast<char*>( static_cast<const char*>( git_blob_rawcontent( aBlob ) ) ),
              const_cast<char*>( static_cast<const char*>( git_blob_rawcontent( aBlob ) ) ),
              const_cast<char*>( static_cast<const char*>( git_blob_rawcontent( aBlob ) ) ) +
              git_blob_rawsize( aBlob ) );
    }

    ~BLOB_BUFFER_STREAM() override
    {
    }

    int overflow( int c ) override
    {
        return traits_type::eof();
    }

    std::streamsize xsputn( const char* s, std::streamsize n ) override
    {
        return 0;
    }
};

// Build a class that implements LINE_READER for git_blobs
class APIEXPORT BLOB_READER : public LINE_READER
{
public:
    BLOB_READER( git_blob* aBlob ) : m_blob( aBlob )
    {
        m_stream = new BLOB_BUFFER_STREAM( m_blob );
        m_istream = new std::istream( m_stream );
        m_line = nullptr;
        m_lineNum = 0;
    }

    ~BLOB_READER() override
    {
        delete m_istream;
        delete m_stream;
    }

    char* ReadLine() override
    {
        getline( *m_istream, m_buffer );

        m_buffer.append( 1, '\n' );

        m_length = m_buffer.size();
        m_line = (char*) m_buffer.data(); //ew why no const??

        // lineNum is incremented even if there was no line read, because this
        // leads to better error reporting when we hit an end of file.
        ++m_lineNum;

        return m_istream->eof() ? nullptr : m_line;
    }

private:
    git_blob* m_blob;
    BLOB_BUFFER_STREAM* m_stream;
    std::istream* m_istream;
    std::string   m_buffer;
};
