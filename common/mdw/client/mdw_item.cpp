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

#include <mdw/client/mdw_item.h>

#include <filesystem>
#include <system_error>

#include <wx/filename.h>
#include <wx/arrstr.h>


namespace
{

#ifdef _WIN32
/// The drive, or the server and share, aPath lives on; empty where paths have no volume
wxString volumeOf( const wxString& aPath )
{
    wxString path = aPath;
    path.Replace( wxS( "/" ), wxS( "\\" ) );

    // Extended-length prefixes name the same volumes as the plain forms
    if( path.Left( 8 ).CmpNoCase( wxS( "\\\\?\\UNC\\" ) ) == 0 )
    {
        path = wxS( "\\\\" ) + path.Mid( 8 );
    }
    else if( path.StartsWith( wxS( "\\\\?\\" ) ) )
    {
        wxString device = path.Mid( 4 ).BeforeFirst( '\\' );

        // A volume named by GUID has no drive letter to compare
        if( device.Left( 7 ).CmpNoCase( wxS( "Volume{" ) ) == 0 )
            return ( wxS( "\\\\?\\" ) + device ).Lower();

        path = path.Mid( 4 );
    }

    if( path.StartsWith( wxS( "\\\\" ) ) )
    {
        wxArrayString parts = wxSplit( path.Mid( 2 ), '\\', '\0' );

        while( parts.size() > 2 )
            parts.RemoveAt( parts.size() - 1 );

        return ( wxS( "\\\\" ) + wxJoin( parts, '\\', '\0' ) ).Lower();
    }

    if( path.Length() >= 2 && path[1] == ':' )
        return path.Left( 2 ).Upper();

    return wxEmptyString;
}
#endif


/// aLocalPath relative to aRoot, both canonical, or empty when they share no root
std::filesystem::path canonicalRelative( const wxString& aRoot, const wxString& aLocalPath )
{
    namespace fs = std::filesystem;

    if( aRoot.IsEmpty() || aLocalPath.IsEmpty() )
        return fs::path();

    std::error_code ec;

    // Canonical on both sides, so a project opened through a symlink still lands inside the root
    fs::path root = fs::weakly_canonical( fs::path( aRoot.ToStdWstring() ), ec );

    if( ec )
        return fs::path();

    fs::path file = fs::weakly_canonical( fs::path( aLocalPath.ToStdWstring() ), ec );

    if( ec )
        return fs::path();

#ifdef _WIN32
    // MSVC would relate two shares of one server, or two \\?\UNC\ paths, as a climb
    if( volumeOf( wxString( root.native() ) ) != volumeOf( wxString( file.native() ) ) )
        return fs::path();
#endif

    fs::path relative = file.lexically_relative( root );

    if( relative == "." || relative.is_absolute() )
        return fs::path();

    return relative;
}


wxString genericText( const std::filesystem::path& aPath )
{
    std::u8string text = aPath.generic_u8string();

    return wxString::FromUTF8( std::string( text.begin(), text.end() ) );
}

} // namespace


wxString MdwRootRelative( const wxString& aRoot, const wxString& aLocalPath )
{
    std::filesystem::path relative = canonicalRelative( aRoot, aLocalPath );

    if( relative.empty() || *relative.begin() == ".." )
        return wxEmptyString;

    return genericText( relative );
}


wxString MdwRootRelativeUp( const wxString& aRoot, const wxString& aLocalPath )
{
    std::filesystem::path relative = canonicalRelative( aRoot, aLocalPath );

    // An ancestor of the root is a folder, never an item
    if( relative.empty() || relative.lexically_normal().filename() == ".." )
        return wxEmptyString;

    return genericText( relative );
}


wxString MdwLocalPath( const wxString& aRoot, const wxString& aRelative )
{
    // Joined by component, as wx parsing drops the directories or takes a leading "~" for home
    wxFileName    fn = wxFileName::DirName( aRoot );
    wxArrayString parts = wxSplit( aRelative, '/', '\0' );

    for( size_t i = 0; i + 1 < parts.size(); ++i )
    {
        if( !parts[i].IsEmpty() )
            fn.AppendDir( parts[i] );
    }

    if( !parts.IsEmpty() )
        fn.SetFullName( parts.Last() );

    return fn.GetFullPath();
}
