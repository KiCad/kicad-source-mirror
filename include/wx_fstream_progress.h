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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef WX_FSTREAM_PROGRESS_H
#define WX_FSTREAM_PROGRESS_H

#include <wx/wfstream.h>

class wxFileOutputStreamWithProgress : public wxFileOutputStream
{
public:
    wxFileOutputStreamWithProgress( const wxString& aFileName )
        : wxFileOutputStream( aFileName )
    {
    }

    wxFileOutputStreamWithProgress( wxFile& aFile )
        : wxFileOutputStream( aFile )
    {
    }

    wxFileOutputStreamWithProgress( int aFd )
        : wxFileOutputStream( aFd )
    {
    }

    virtual ~wxFileOutputStreamWithProgress() = default;

    void SetProgressCallback( std::function<void( size_t )> aCallback )
    {
        m_callback = std::move( aCallback );
    }

    virtual size_t OnSysWrite( const void* aBuffer, size_t aSize ) override
    {
        size_t written = wxFileOutputStream::OnSysWrite( aBuffer, aSize );

        if( m_callback )
            m_callback( written );

        return written;
    }

private:
    std::function<void( size_t )> m_callback;
};

#endif // WX_FSTREAM_PROGRESS_H