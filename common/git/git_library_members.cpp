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

#include <git/git_library_members.h>
#include <git/kicad_git_memory.h>
#include <diff_merge/kicad_diff_types.h>

#include <git2.h>

#include <string_utils.h>

#include <wx/ffile.h>
#include <wx/intl.h>

#include <string>


namespace KIGIT
{

std::vector<KICAD_DIFF::ITEM_CHANGE> ChangedSymbolsInWorkdir( git_repository* aRepo, const std::string& aPath,
                                                              const LIBRARY_DIFF& aDiff )
{
    if( !aDiff )
        return {};

    std::string before;
    git_object* object = nullptr;

    // A file HEAD lacks is all additions
    if( git_revparse_single( &object, aRepo, ( "HEAD:" + aPath ).c_str() ) == 0 )
    {
        GitObjectPtr objectPtr( object );

        if( git_object_type( object ) == GIT_OBJECT_BLOB )
        {
            const git_blob* blob = reinterpret_cast<const git_blob*>( object );
            before.assign( static_cast<const char*>( git_blob_rawcontent( blob ) ),
                           static_cast<size_t>( git_blob_rawsize( blob ) ) );
        }
    }

    const char* workdir = git_repository_workdir( aRepo );

    if( !workdir )
        return {};

    // wxFFile, since a narrow stream misreads a UTF-8 path on Windows
    wxFFile     file( wxString::FromUTF8( std::string( workdir ) + aPath ), wxS( "rb" ) );
    std::string after;

    if( file.IsOpened() && file.Length() > 0 )
    {
        after.resize( static_cast<size_t>( file.Length() ) );
        after.resize( file.Read( after.data(), after.size() ) );
    }

    KICAD_DIFF::DOCUMENT_DIFF diff;

    if( !aDiff( before, after, &diff ) )
        return {};

    return std::move( diff.changes );
}


wxString MemberList( const std::vector<KICAD_DIFF::ITEM_CHANGE>& aChanges )
{
    const size_t shown = aChanges.size() > 5 ? 4 : aChanges.size();
    wxString     list;

    for( size_t ii = 0; ii < shown; ++ii )
    {
        wxString name = UnescapeString( aChanges[ii].refdes.value_or( wxString() ) );

        if( aChanges[ii].kind == KICAD_DIFF::CHANGE_KIND::ADDED )
            name = wxString::Format( _( "%s (new)" ), name );
        else if( aChanges[ii].kind == KICAD_DIFF::CHANGE_KIND::REMOVED )
            name = wxString::Format( _( "%s (removed)" ), name );

        if( ii == 0 )
            list = name;
        else if( ii + 1 == shown && shown == aChanges.size() )
            list = wxString::Format( _( "%s and %s" ), list, name );
        else
            list += wxS( ", " ) + name;
    }

    if( shown < aChanges.size() )
        list = wxString::Format( _( "%s and %zu more" ), list, aChanges.size() - shown );

    return list;
}

} // namespace KIGIT
