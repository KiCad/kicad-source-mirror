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

#include <dialogs/git/workspace_tree.h>

#include <string_utils.h>

#include <wx/intl.h>

#include <algorithm>
#include <cstdint>
#include <map>


namespace
{
using NODE = WORKSPACE_TREE_NODE;


bool matches( const wxString& aName, const wxString& aLowerFilter )
{
    return aLowerFilter.IsEmpty() || aName.Lower().Contains( aLowerFilter );
}


void sortChildren( std::vector<NODE>& aNodes )
{
    std::stable_sort( aNodes.begin(), aNodes.end(),
                      []( const NODE& a, const NODE& b )
                      {
                          bool aFolder = a.kind == NODE::KIND::FOLDER;
                          bool bFolder = b.kind == NODE::KIND::FOLDER;

                          if( aFolder != bFolder )
                              return aFolder;

                          return StrNumCmp( a.name, b.name, CMP_T::IGNORE_CASE ) < 0;
                      } );
}


class BUILDER
{
public:
    BUILDER( const std::vector<MDW_FOLDER>& aFolders, const std::vector<MDW_ITEM_INFO>& aItems,
             const wxString& aFilter ) :
            m_folders( aFolders ),
            m_items( aItems ),
            m_filter( aFilter.Lower() )
    {
        for( size_t i = 0; i < m_folders.size(); ++i )
            m_index.emplace( m_folders[i].folder_id, i );

        m_childFolders.resize( m_folders.size() );
        m_folderItems.resize( m_folders.size() );

        std::vector<bool> inLoop = loopMembers();

        for( size_t i = 0; i < m_folders.size(); ++i )
        {
            // The first folder carrying an id owns it
            if( m_index[m_folders[i].folder_id] != i )
                continue;

            auto parent = m_index.find( m_folders[i].parent_folder_id );

            if( parent != m_index.end() && !inLoop[i] )
                m_childFolders[parent->second].push_back( i );
            else
                m_rootFolders.push_back( i );
        }

        for( size_t i = 0; i < m_items.size(); ++i )
        {
            auto folder = m_index.find( m_items[i].folder_id );

            if( folder != m_index.end() )
                m_folderItems[folder->second].push_back( i );
            else
                m_rootItems.push_back( i );
        }
    }

    std::vector<NODE> Root() const
    {
        std::vector<NODE> out;

        for( size_t f : m_rootFolders )
            buildFolder( f, false, out );

        for( size_t i : m_rootItems )
            buildItem( i, false, out );

        sortChildren( out );
        return out;
    }

private:
    // One colouring pass over the parent links, so only folders on a loop are marked
    std::vector<bool> loopMembers() const
    {
        enum COLOUR : uint8_t { UNVISITED, ON_PATH, DONE };

        std::vector<COLOUR> colour( m_folders.size(), UNVISITED );
        std::vector<bool>   inLoop( m_folders.size(), false );
        std::vector<size_t> path;

        for( size_t start = 0; start < m_folders.size(); ++start )
        {
            size_t cur = start;

            while( colour[cur] == UNVISITED )
            {
                colour[cur] = ON_PATH;
                path.push_back( cur );

                auto parent = m_index.find( m_folders[cur].parent_folder_id );

                if( parent == m_index.end() )
                {
                    cur = m_folders.size();
                    break;
                }

                cur = parent->second;
            }

            if( cur < m_folders.size() && colour[cur] == ON_PATH )
            {
                // Walk back from the end of the path to the folder the loop closed on
                for( size_t i = path.size(); i-- > 0; )
                {
                    inLoop[path[i]] = true;

                    if( path[i] == cur )
                        break;
                }
            }

            for( size_t f : path )
                colour[f] = DONE;

            path.clear();
        }

        return inLoop;
    }

    void buildItem( size_t aItem, bool aKeepAll, std::vector<NODE>& aOut ) const
    {
        const MDW_ITEM_INFO& info = m_items[aItem];

        if( !aKeepAll && !matches( info.display_name, m_filter ) )
            return;

        NODE node;
        node.kind = NODE::KIND::ITEM;
        node.key = info.item_id;
        node.name = info.display_name;
        node.item = aItem;
        aOut.push_back( std::move( node ) );
    }

    void buildFolder( size_t aFolder, bool aKeepAll, std::vector<NODE>& aOut ) const
    {
        const MDW_FOLDER& folder = m_folders[aFolder];
        const wxString&   name = folder.display_name.IsEmpty() ? folder.folder_id : folder.display_name;
        bool              keepAll = aKeepAll || matches( name, m_filter );

        NODE node;
        node.kind = NODE::KIND::FOLDER;
        node.key = folder.folder_id;
        node.name = name;

        for( size_t child : m_childFolders[aFolder] )
            buildFolder( child, keepAll, node.children );

        for( size_t item : m_folderItems[aFolder] )
            buildItem( item, keepAll, node.children );

        if( !keepAll && node.children.empty() )
            return;

        sortChildren( node.children );
        aOut.push_back( std::move( node ) );
    }

    const std::vector<MDW_FOLDER>&    m_folders;
    const std::vector<MDW_ITEM_INFO>& m_items;
    wxString                          m_filter;

    std::map<wxString, size_t>        m_index;
    std::vector<std::vector<size_t>>  m_childFolders;
    std::vector<std::vector<size_t>>  m_folderItems;
    std::vector<size_t>               m_rootFolders;
    std::vector<size_t>               m_rootItems;
};


void buildLibraries( const std::vector<WORKSPACE_TREE_LIBRARY>& aLibraries, const wxString& aLowerFilter,
                     std::vector<NODE>& aOut )
{
    NODE group;
    group.kind = NODE::KIND::LIBRARIES;
    group.name = _( "Team libraries" );

    for( const WORKSPACE_TREE_LIBRARY& lib : aLibraries )
    {
        bool keepAll = matches( lib.name, aLowerFilter );

        NODE node;
        node.kind = NODE::KIND::LIBRARY;
        node.key = lib.key;
        node.name = lib.name;

        for( const wxString& member : lib.members )
        {
            if( !keepAll && !matches( member, aLowerFilter ) )
                continue;

            NODE leaf;
            leaf.kind = NODE::KIND::MEMBER;
            leaf.key = lib.key + wxT( "/" ) + member;
            leaf.name = member;
            node.children.push_back( std::move( leaf ) );
        }

        if( !keepAll && node.children.empty() )
            continue;

        sortChildren( node.children );

        if( !lib.listed )
        {
            NODE unlisted;
            unlisted.kind = NODE::KIND::UNLISTED;
            unlisted.key = lib.key + wxT( "/" );
            unlisted.name = _( "Not listed yet" );
            node.children.push_back( std::move( unlisted ) );
        }

        group.children.push_back( std::move( node ) );
    }

    if( group.children.empty() )
        return;

    sortChildren( group.children );
    aOut.push_back( std::move( group ) );
}


bool findPath( const NODE& aNode, const WORKSPACE_TREE_KEY& aKey, std::vector<const NODE*>& aPath )
{
    for( const NODE& child : aNode.children )
    {
        aPath.push_back( &child );

        if( ( child.kind == aKey.first && child.key == aKey.second ) || findPath( child, aKey, aPath ) )
            return true;

        aPath.pop_back();
    }

    return false;
}
} // namespace


WORKSPACE_TREE_NODE BuildWorkspaceTree( const std::vector<MDW_FOLDER>& aFolders,
                                        const std::vector<MDW_ITEM_INFO>& aItems,
                                        const std::vector<WORKSPACE_TREE_LIBRARY>& aLibraries,
                                        const wxString& aFilter, bool aItemsMatched )
{
    NODE root;
    root.children = BUILDER( aFolders, aItems, aItemsMatched ? wxString() : aFilter ).Root();

    buildLibraries( aLibraries, aFilter.Lower(), root.children );
    return root;
}


std::vector<const WORKSPACE_TREE_NODE*> WorkspaceTreePath( const WORKSPACE_TREE_NODE& aRoot,
                                                           const WORKSPACE_TREE_KEY&  aKey )
{
    std::vector<const WORKSPACE_TREE_NODE*> path;

    findPath( aRoot, aKey, path );
    return path;
}
