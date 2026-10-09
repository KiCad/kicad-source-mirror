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

#include <dialogs/git/workspace_listing.h>

#include <dialogs/git/workspace_item_files.h>
#include <mdw/client/mdw_op.h>
#include <mdw/client/mdw_paged_read.h>
#include <mdw/client/mdw_source.h>

#include <deque>
#include <iterator>
#include <set>
#include <utility>


namespace
{
template <typename ROW>
using PAGE_FETCH = std::function<MDW_OP( const MDW_PAGE&, MDW_DONE<MDW_PAGED<ROW>> )>;


template <typename ROW>
MDW_RESULT readAll( const PAGE_FETCH<ROW>& aFetch, std::vector<ROW>& aOut )
{
    auto [result, read] = MdwSync<MDW_PAGED<ROW>>(
            [&]( MDW_DONE<MDW_PAGED<ROW>> aDone )
            {
                return MdwReadAllPages<ROW>( aFetch, 0, std::move( aDone ) );
            } );

    aOut.insert( aOut.end(), std::make_move_iterator( read.rows.begin() ), std::make_move_iterator( read.rows.end() ) );
    return result;
}
} // namespace


WORKSPACE_LISTING ReadWorkspaceListing( const std::shared_ptr<MDW_SOURCE>&       aSource,
                                        std::shared_ptr<const std::atomic<bool>> aCancelled,
                                        const std::function<bool()>&             aStopped )
{
    if( !aSource )
    {
        WORKSPACE_LISTING listing;
        listing.status = MDW_RESULT::Fail( MDW_ERROR_CODE::BACKEND_LOST );
        return listing;
    }

    MDW_CANCEL_SCOPE        cancelScope( aCancelled );
    WORKSPACE_LISTING_READS reads;

    reads.rootFolders = [&]( std::vector<MDW_FOLDER>& aOut )
    {
        return readAll<MDW_FOLDER>(
                [&]( const MDW_PAGE& aPage, MDW_DONE<MDW_PAGED<MDW_FOLDER>> aDone )
                {
                    return aSource->RootFolders( aPage, std::move( aDone ) );
                },
                aOut );
    };

    reads.childFolders = [&]( const wxString& aFolderId, std::vector<MDW_FOLDER>& aOut )
    {
        return readAll<MDW_FOLDER>(
                [&]( const MDW_PAGE& aPage, MDW_DONE<MDW_PAGED<MDW_FOLDER>> aDone )
                {
                    return aSource->ChildFolders( aFolderId, aPage, std::move( aDone ) );
                },
                aOut );
    };

    reads.items = [&]( const wxString& aFolderId, std::vector<MDW_ITEM_INFO>& aOut )
    {
        return readAll<MDW_ITEM_INFO>(
                [&]( const MDW_PAGE& aPage, MDW_DONE<MDW_PAGED<MDW_ITEM_INFO>> aDone )
                {
                    return aSource->Items( aFolderId, wxEmptyString, aPage, std::move( aDone ) );
                },
                aOut );
    };

    return ReadWorkspaceListing( reads,
                                 [&]()
                                 {
                                     return ( aCancelled && aCancelled->load() ) || ( aStopped && aStopped() );
                                 } );
}


WORKSPACE_LISTING ReadWorkspaceListing( const WORKSPACE_LISTING_READS& aReads, const std::function<bool()>& aStopped )
{
    WORKSPACE_LISTING listing;

    // Answers whether to go on, so a request is never started for a listing nobody wants
    auto proceed = [&]()
    {
        if( aStopped && aStopped() )
            listing.status = MDW_RESULT::Fail( MDW_ERROR_CODE::CANCELLED );

        return listing.status.IsOk();
    };

    std::vector<MDW_FOLDER> roots;

    if( proceed() )
        listing.status = aReads.rootFolders( roots );

    std::deque<wxString> queue;
    std::set<wxString>   visited;

    // The walk knows each folder's parent, whether or not the backend filled it in
    auto visit = [&]( MDW_FOLDER& aFolder, const wxString& aParentId )
    {
        if( !visited.insert( aFolder.folder_id ).second )
            return;

        queue.push_back( aFolder.folder_id );

        // An empty id is the workspace itself, whose contents belong at the top of the tree
        if( aFolder.folder_id.IsEmpty() )
            return;

        auto     parent = listing.folderNames.find( aParentId );
        wxString parentName = parent == listing.folderNames.end() ? wxString() : parent->second;

        aFolder.parent_folder_id = aParentId;
        listing.folderNames[aFolder.folder_id] = WorkspaceFolderName( parentName, aFolder );
        listing.folders.push_back( std::move( aFolder ) );
    };

    for( MDW_FOLDER& folder : roots )
        visit( folder, wxEmptyString );

    while( !queue.empty() && proceed() )
    {
        wxString                   folderId = queue.front();
        std::vector<MDW_ITEM_INFO> items;
        std::vector<MDW_FOLDER>    children;

        queue.pop_front();

        listing.status = aReads.items( folderId, items );

        if( proceed() )
            listing.status = aReads.childFolders( folderId, children );

        for( MDW_ITEM_INFO& item : items )
        {
            if( item.is_folder )
                continue;

            // An item the backend left without a folder would otherwise sit at the top of the tree
            if( item.folder_id.IsEmpty() )
                item.folder_id = folderId;

            listing.items.push_back( std::move( item ) );
        }

        for( MDW_FOLDER& child : children )
            visit( child, folderId );
    }

    return listing;
}
