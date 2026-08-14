/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <dialogs/git/workspace_item_files.h>

#include <mdw/client/mdw_lifecycle.h>

#include <wildcards_and_files_ext.h>
#include <wx_filename.h>

#include <wx/file.h>
#include <wx/filename.h>
#include <wx/intl.h>

#include <algorithm>


wxString WorkspaceCheckoutPath( const wxString& aProjectDir, const MDW_ITEM_INFO& aItem )
{
    wxFileName file;

    // A relative path is backend data, so it must not name a file outside the project
    if( aProjectDir.IsEmpty() || !WX_FILENAME::ResolveArchiveEntryPath( aProjectDir, aItem.relative_path, file ) )
        return wxString();

    // Local settings hold this copy's checkouts, and a teammate's uploaded through the server would replace them
    if( file.GetExt().IsSameAs( FILEEXT::ProjectLocalSettingsFileExtension, false ) )
        return wxString();

    return file.GetFullPath();
}


wxString WorkspaceCheckinPath( const std::map<wxString, wxString>& aRecorded, const wxString& aProjectDir,
                               const MDW_ITEM_INFO& aItem )
{
    wxString checkout = WorkspaceCheckoutPath( aProjectDir, aItem );
    wxString fallback = checkout;

    for( const auto& [path, itemId] : aRecorded )
    {
        if( path.IsEmpty() )
            continue;

        wxFileName file( path );

        if( file.IsRelative() )
            file.MakeAbsolute( aProjectDir );

        if( itemId == aItem.item_id )
            return file.GetFullPath();

        // Another item's checkout must never be uploaded as this one
        if( !checkout.IsEmpty() && file.SameAs( wxFileName( checkout ) ) )
            fallback.clear();
    }

    return fallback;
}


wxString WorkspaceRevisionText( const MDW_REVISION& aRevision )
{
    if( wxString text = MdwRevisionText( aRevision ); !text.IsEmpty() )
        return text;

    return aRevision.label.IsEmpty() ? aRevision.revision_id : aRevision.label;
}


wxString MdwItemFileName( const MDW_ITEM_REF& aItem )
{
    const wxString& source = aItem.path_hint.IsEmpty() ? aItem.item_id : aItem.path_hint;

    // A display path may have spaces around its separators
    return source.AfterLast( '/' ).Trim( false ).Trim( true );
}


wxString MdwVersionSideLabel( const MDW_ITEM_REF& aItem, const MDW_REVISION& aRevision )
{
    const wxString name = MdwItemFileName( aItem );
    const wxString version = WorkspaceRevisionText( aRevision );

    // A release already reads "Rev 3"
    return MdwRevisionText( aRevision ).IsEmpty() ? wxString::Format( wxS( "%s v%s" ), name, version )
                                                  : wxString::Format( wxS( "%s %s" ), name, version );
}


wxString WorkspaceFolderName( const wxString& aParentName, const MDW_FOLDER& aFolder )
{
    const wxString& name = aFolder.display_name.IsEmpty() ? aFolder.folder_id : aFolder.display_name;

    return aParentName.IsEmpty() ? name : aParentName + wxS( " / " ) + name;
}


std::optional<std::pair<long, long>> MdwComparePair( long aRows, const std::vector<long>& aSelected,
                                                     bool aHasWorkingRow )
{
    long total = aRows + ( aHasWorkingRow ? 1 : 0 );

    for( long row : aSelected )
    {
        if( row < 0 || row >= total )
            return std::nullopt;
    }

    if( aSelected.size() == 2 && aSelected[0] != aSelected[1] )
        return std::make_pair( std::max( aSelected[0], aSelected[1] ), std::min( aSelected[0], aSelected[1] ) );

    if( aSelected.size() != 1 )
        return std::nullopt;

    long row = aSelected[0];

    if( row + 1 < total )
        return std::make_pair( row + 1, row );

    // The oldest row has no predecessor, so it meets the working copy above the versions
    if( aHasWorkingRow && row > 0 )
        return std::make_pair( row, 0L );

    return std::nullopt;
}


bool MdwFilesDiffer( const wxString& aPath, const wxString& aOther )
{
    auto read = []( const wxString& aFile, std::string& aBytes )
    {
        wxFile file;

        if( !wxFileName::FileExists( aFile ) || !file.Open( aFile ) )
            return false;

        wxFileOffset length = file.Length();

        if( length < 0 )
            return false;

        aBytes.assign( static_cast<size_t>( length ), '\0' );
        return file.Read( aBytes.data(), aBytes.size() ) == static_cast<ssize_t>( aBytes.size() );
    };

    std::string first;
    std::string second;

    return read( aPath, first ) && read( aOther, second ) && first != second;
}


wxString WorkspaceCheckinReport( const MDW_ITEM_INFO& aItem, const MDW_REVISION& aRevision )
{
    const wxString& name = aItem.display_name.IsEmpty() ? aItem.item_id : aItem.display_name;

    return wxString::Format( _( "Checked in %s as %s" ), name, WorkspaceRevisionText( aRevision ) );
}


wxString WorkspaceKeepBothPath( const wxString& aPath )
{
    // Past the KiCad extension no project tree lists the copy, and Commit All leaves it out as a transient file
    const wxString mine = aPath + wxS( ".mine" );
    wxString       path = mine;

    for( int ii = 2; wxFileName::FileExists( path ); ++ii )
        path = wxString::Format( wxS( "%s.%d" ), mine, ii );

    return path;
}
