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

#include <wx_filename.h>
#include <string_utils.h>


WX_FILENAME::WX_FILENAME( const wxString& aPath, const wxString& aFilename )
        : m_fn( aPath, aFilename ), m_path( aPath ), m_fullName( aFilename )
{
}


void WX_FILENAME::SetFullName( const wxString& aFileNameAndExtension )
{
    m_fullName = aFileNameAndExtension;
}


void WX_FILENAME::SetPath( const wxString& aPath )
{
    m_fn.SetPath( aPath );
    m_path = aPath;
}


wxString WX_FILENAME::GetName() const
{
    size_t dot = m_fullName.find_last_of( wxT( '.' ) );
    return m_fullName.substr( 0, dot );
}


wxString WX_FILENAME::GetFullName() const
{
    return m_fullName;
}


wxString WX_FILENAME::GetPath() const
{
    return m_path;
}


wxString WX_FILENAME::GetFullPath() const
{
    return m_path + wxT( '/' ) + m_fullName;
}


void WX_FILENAME::resolve()
{
    size_t dot = m_fullName.find_last_of( wxT( '.' ) );
    m_fn.SetName( m_fullName.substr( 0, dot ) );
    m_fn.SetExt( m_fullName.substr( dot + 1 ) );
}


long long WX_FILENAME::GetTimestamp()
{
    resolve();

    if( m_fn.FileExists() )
        return m_fn.GetModificationTime().GetValue().GetValue();

    return 0;
}


void WX_FILENAME::ResolvePossibleSymlinks( wxFileName& aFilename )
{
#ifndef __WINDOWS__
    if( aFilename.Exists( wxFILE_EXISTS_SYMLINK ) )
    {
        char buffer[PATH_MAX];
        char* realPath = realpath( TO_UTF8( aFilename.GetFullPath() ), buffer );

        if( realPath )
            aFilename.Assign( wxString::FromUTF8( realPath ) );
    }
#endif
}
