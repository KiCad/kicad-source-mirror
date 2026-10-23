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

#include <gestfich.h>
#include <wildcards_and_files_ext.h>
#include <wx_filename.h>

#include <wx/arrstr.h>
#include <wx/file.h>
#include <wx/filename.h>
#include <wx/intl.h>

#include <algorithm>
#include <filesystem>


namespace
{

/// aItemId, a path from the root of the working tree aWorktree, as a local path. Each part is taken as written, so a
/// backslash in a POSIX name stays in the name. Empty without a tree or for an id that would leave it
wxString worktreePath( const wxString& aWorktree, const wxString& aItemId )
{
    if( aWorktree.IsEmpty() || aItemId.IsEmpty() || aItemId.StartsWith( wxS( "/" ) ) )
        return wxString();

#ifdef __WINDOWS__
    // A separator here, so the name would reach another file
    if( aItemId.Contains( wxS( "\\" ) ) )
        return wxString();
#endif

    wxFileName     file = wxFileName::DirName( aWorktree );
    wxArrayString  parts = wxSplit( aItemId, '/', '\0' );
    const wxString name = parts.Last();

    if( name.IsEmpty() || name == wxS( "." ) || name == wxS( ".." ) || name.Contains( wxS( ":" ) ) )
        return wxString();

    parts.RemoveAt( parts.size() - 1 );

    for( const wxString& part : parts )
    {
        if( part.IsEmpty() || part == wxS( "." ) || part == wxS( ".." ) || part.Contains( wxS( ":" ) ) )
            return wxString();

        file.AppendDir( part );
    }

    file.SetFullName( name );
    return file.GetFullPath();
}

} // namespace


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


WORKSPACE_WORKING_FILES::WORKSPACE_WORKING_FILES( const wxString& aEdited ) :
        edited( aEdited )
{
}


std::vector<wxString> WORKSPACE_WORKING_FILES::All() const
{
    std::vector<wxString> files;

    if( !edited.IsEmpty() )
        files.push_back( edited );

    files.insert( files.end(), others.begin(), others.end() );
    return files;
}


WORKSPACE_WORKING_FILES WorkspaceWorkingFiles( const std::map<wxString, wxString>& aRecorded,
                                               const wxString& aProjectDir, const MDW_ITEM_INFO& aItem,
                                               const std::optional<wxString>& aWorktree )
{
    const wxString checkout = WorkspaceCheckoutPath( aProjectDir, aItem );

    WORKSPACE_WORKING_FILES files( WorkspaceCheckinPath( aRecorded, aProjectDir, aItem ) );

    if( files.edited.IsEmpty() )
        files.edited = checkout;
    else if( !checkout.IsEmpty() && checkout != files.edited )
        files.others.push_back( checkout );

    const wxString              tracked = worktreePath( aWorktree.value_or( wxString() ), aItem.item_id );
    const std::vector<wxString> known = files.All();

    // A repository file outside the project has no checkout path, and the project's read of changes leaves it out
    if( !tracked.IsEmpty()
        && std::ranges::none_of( known,
                                 [&]( const wxString& aFile )
                                 {
                                     return wxFileName( aFile ).SameAs( wxFileName( tracked ) );
                                 } ) )
    {
        files.others.push_back( tracked );
    }

    // A recorded checkout elsewhere is still not the repository file the user may edit in place
    files.unresolved = aWorktree && tracked.IsEmpty() && checkout.IsEmpty();
    return files;
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


MDW_PREVIEW_BEFORE MdwPreviewBefore( long aRow, long aRows, bool aHasWorkingRow, bool aOlderUnread )
{
    const long total = aRows + ( aHasWorkingRow ? 1 : 0 );

    if( aRow < 0 || aRow >= total )
        return { MDW_PREVIEW_BEFORE::WHAT::UNREAD };

    if( aRow + 1 < total )
        return { MDW_PREVIEW_BEFORE::WHAT::ROW, aRow + 1 };

    // A partial history's last row is only the oldest listed, not the oldest version
    return { aOlderUnread ? MDW_PREVIEW_BEFORE::WHAT::UNREAD : MDW_PREVIEW_BEFORE::WHAT::ADDED };
}


MDW_COMPARE_PATHS MdwComparePaths( const wxString& aRoot, const wxString& aFileName, bool aHasOlder )
{
    MDW_COMPARE_PATHS paths;

    if( aHasOlder )
        paths.older = wxFileName( aRoot + wxS( "/older/" ) + aFileName ).GetFullPath();

    paths.newer = wxFileName( aRoot + wxS( "/newer/" ) + aFileName ).GetFullPath();
    return paths;
}


std::optional<MDW_COMPARE_ROWS> MdwCompareRows( long aRows, const std::vector<long>& aSelected, bool aHasWorkingRow,
                                                bool aOlderUnread )
{
    if( std::optional<std::pair<long, long>> pair = MdwComparePair( aRows, aSelected, aHasWorkingRow ) )
        return MDW_COMPARE_ROWS{ pair->first, pair->second };

    if( aSelected.size() != 1 )
        return std::nullopt;

    const MDW_PREVIEW_BEFORE before = MdwPreviewBefore( aSelected[0], aRows, aHasWorkingRow, aOlderUnread );

    if( before.what == MDW_PREVIEW_BEFORE::WHAT::ADDED )
        return MDW_COMPARE_ROWS{ std::nullopt, aSelected[0] };

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


wxString MdwRevisionCopyName( const wxString& aFileName, const wxString& aMemberName, const MDW_REVISION& aRevision )
{
    const wxFileName file( aFileName );
    const wxString   label = aRevision.label.IsEmpty() ? aRevision.revision_id.Left( 7 ) : aRevision.label;

    // Backend text, so a separator in it would name another folder
    auto safe = []( const wxString& aText )
    {
        const wxString reserved = wxS( "\\/:*?\"<>|" );
        wxString       text;

        for( const wxUniChar c : aText )
            text << ( reserved.Find( c ) != wxNOT_FOUND || wxIsspace( c ) ? wxUniChar( '_' ) : c );

        return text;
    };

    wxString name = ( aMemberName.IsEmpty() ? file.GetName() : safe( aMemberName ) ) + wxS( "-" ) + safe( label );

    if( !file.GetExt().IsEmpty() )
        name << wxS( "." ) << file.GetExt();

    return name;
}


wxString MdwCopyTargetRefusal( const wxString& aTarget, const WORKSPACE_WORKING_FILES& aWorkingFiles )
{
    namespace fs = std::filesystem;

    const fs::path target = ToFsPath( aTarget );
    const wxString unknown = _( "Could not check whether that is your working file. Choose another file." );

    if( aWorkingFiles.unresolved )
    {
        std::error_code ec;
        const bool      targetExists = fs::exists( target, ec );

        if( ec )
            return unknown;

        if( targetExists )
            return _( "Could not find your working file, so Save Copy replaces no file. Choose a new name." );
    }

    for( const wxString& working : aWorkingFiles.All() )
    {
        if( working.IsEmpty() )
            continue;

        const fs::path  checkout = ToFsPath( working );
        std::error_code ec;

        // An error is no answer, and treating it as absence would let the copy replace the working file
        const bool targetExists = fs::exists( target, ec );

        if( ec )
            return unknown;

        const bool checkoutExists = fs::exists( checkout, ec );

        if( ec )
            return unknown;

        bool same = false;

        // Unnormalized, since dropping ".." after a symlinked folder names another file than the writer opens
        if( targetExists && checkoutExists )
        {
            same = fs::equivalent( target, checkout, ec );

            if( ec )
                return unknown;
        }
        else
        {
            same = wxFileName( aTarget ).SameAs( wxFileName( working ) );
        }

        if( same )
            return _( "That is the file you are working on. Choose another name to keep both." );
    }

    return wxString();
}
