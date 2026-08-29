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

#include <wx/arrstr.h>


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


bool WX_FILENAME::SplitArchiveEntryName( const wxString& aEntryName, wxArrayString& aParts )
{
    aParts.Clear();

    if( aEntryName.IsEmpty() )
        return false;

    // A NUL is truncated by the OS: what gets validated would not be what gets written.
    if( aEntryName.find( wxUniChar( 0 ) ) != wxString::npos )
        return false;

    // A ZIP written on Windows can carry backslashes, normalize
    wxString name = aEntryName;
    name.Replace( wxT( "\\" ), wxT( "/" ) );

    // Archive members are relative.  Check DOS volumes everywhere so POSIX rejects "C:/evil".
    if( name.StartsWith( wxT( "/" ) ) )
        return false;

    wxString volume;

    wxFileName::SplitVolume( name, &volume, nullptr, wxPATH_DOS );

    if( !volume.IsEmpty() )
        return false;

    for( const wxString& part : wxSplit( name, '/', (wxChar) 0 ) )
    {
        // Doubled or trailing separators and "." do not change where the entry lands.
        if( part.IsEmpty() || part == wxT( "." ) )
            continue;

        if( part == wxT( ".." ) )
            return false;

#ifdef _WIN32
        // ':' opens an alternate data stream, and Win32 folds "evil. " onto "evil".
        if( part.Contains( wxT( ":" ) ) || part.EndsWith( wxT( "." ) ) || part.EndsWith( wxT( " " ) ) )
        {
            return false;
        }
#endif

        aParts.Add( part );
    }

    return !aParts.IsEmpty();
}


bool WX_FILENAME::ResolveArchiveEntryPath( const wxString& aDestDir, const wxString& aEntryName, wxFileName& aResult )
{
    wxArrayString parts;

    if( !SplitArchiveEntryName( aEntryName, parts ) )
        return false;

    wxFileName dest = wxFileName::DirName( aDestDir );

    // Not FN_NORMALIZE_FLAGS: expanding "~" or shell shortcuts here would reopen the escape.
    dest.Normalize( wxPATH_NORM_DOTS | wxPATH_NORM_ABSOLUTE );

    wxFileName target = dest;

    for( size_t ii = 0; ii + 1 < parts.GetCount(); ++ii )
        target.AppendDir( parts[ii] );

    target.SetFullName( parts.Last() );
    target.Normalize( wxPATH_NORM_DOTS | wxPATH_NORM_ABSOLUTE );

    // ".." is already rejected, but double check the path is below the dest dir
    const wxString destPath = dest.GetPathWithSep();
    const wxString fullPath = target.GetFullPath();

    if( fullPath.length() <= destPath.length() )
        return false;

    if( wxFileName::IsCaseSensitive() )
    {
        if( !fullPath.StartsWith( destPath ) )
            return false;
    }
    else if( !fullPath.Lower().StartsWith( destPath.Lower() ) )
    {
        return false;
    }

    aResult = target;
    return true;
}