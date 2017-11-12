/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 CERN
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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
 * https://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <lib_manager_adapter.h>
#include <lib_manager.h>
#include <class_libentry.h>


CMP_TREE_MODEL_ADAPTER_BASE::PTR LIB_MANAGER_ADAPTER::Create( LIB_MANAGER* aLibMgr )
{
    auto adapter = new LIB_MANAGER_ADAPTER( aLibMgr );
    auto container = CMP_TREE_MODEL_ADAPTER_BASE::PTR( adapter );
    return container;
}


void LIB_MANAGER_ADAPTER::AddLibrary( const wxString& aLibNickname )
{
    auto& lib_node = m_tree.AddLib( aLibNickname );
    ItemAdded( wxDataViewItem( nullptr ), ToItem( &lib_node ) );
    updateLibrary( lib_node );
    lib_node.AssignIntrinsicRanks();
    m_tree.AssignIntrinsicRanks();
}


void LIB_MANAGER_ADAPTER::RemoveLibrary( const wxString& aLibNickname )
{
    auto it = std::find_if( m_tree.Children.begin(), m_tree.Children.end(),
            [&] ( std::unique_ptr<CMP_TREE_NODE>& node ) { return node->Name == aLibNickname; } );

    if( it != m_tree.Children.end() )
    {
        ItemDeleted( wxDataViewItem( nullptr ), ToItem( it->get() ) );
        m_tree.Children.erase( it );
    }
}


void LIB_MANAGER_ADAPTER::UpdateLibrary( const wxString& aLibraryName )
{
    CMP_TREE_NODE* node = findLibrary( aLibraryName );

    if( !node )
        return;

    ItemChanged( ToItem( node ) );
}


void LIB_MANAGER_ADAPTER::AddAliasList( const wxString& aNodeName,
        const wxArrayString& aAliasNameList )
{
    wxASSERT( false );      // TODO
}


bool LIB_MANAGER_ADAPTER::IsContainer( const wxDataViewItem& aItem ) const
{
    const CMP_TREE_NODE* node = ToNode( aItem );
    return node && node->Type == CMP_TREE_NODE::LIB;
}


void LIB_MANAGER_ADAPTER::Sync()
{
    if( getSyncHash() == m_libMgr->GetHash() )
        return;

    wxDataViewItem root( nullptr );

    // Process already stored libraries
    for( auto it = m_tree.Children.begin(); it != m_tree.Children.end(); /* iteration inside */ )
    {
        int mgrHash = m_libMgr->GetLibraryHash( it->get()->Name );

        if( mgrHash < 0 )
        {
            deleteLibrary( * (CMP_TREE_NODE_LIB*) it->get() );
            it = m_tree.Children.erase( it );
            continue;
        }
        else if( mgrHash != m_libHashes[it->get()->Name] )
        {
            updateLibrary( * (CMP_TREE_NODE_LIB*) it->get() );
        }

        ++it;
    }

    // Look for new libraries
    for( const auto& libName : m_libMgr->GetLibraryNames() )
    {
        if( m_libHashes.count( libName ) == 0 )
        {
            auto& libNode = m_tree.AddLib( libName );       // Use AddLibrary?
            ItemAdded( root, ToItem( &libNode ) );
            updateLibrary( libNode );
        }
    }

    Resort();
}


void LIB_MANAGER_ADAPTER::updateLibrary( CMP_TREE_NODE_LIB& aLibNode )
{
    wxDataViewItem parent = ToItem( &aLibNode );
    aLibNode.Children.clear();

    for( auto alias : m_libMgr->GetAliases( aLibNode.Name ) )
    {
        auto& aliasNode = aLibNode.AddAlias( alias );
        ItemAdded( parent, ToItem( &aliasNode ) );
    }

    // TODO faster?
    /*
    wxDataViewItemArray aliasItems;
    aliasItems.reserve( aLibNode.Children.size() );

    for( const auto& child : aLibNode.Children )
        aliasItems.Add( ToItem( child.get() ) );

    ItemsAdded( parent, aliasItems );
    */

    m_libHashes[aLibNode.Name] = m_libMgr->GetLibraryHash( aLibNode.Name );
}


void LIB_MANAGER_ADAPTER::deleteLibrary( CMP_TREE_NODE_LIB& aLibNode )
{
    wxASSERT( false );
}


CMP_TREE_NODE* LIB_MANAGER_ADAPTER::findLibrary( const wxString& aLibNickName )
{
    for( auto& lib : m_tree.Children )
    {
        if( lib->Name == aLibNickName )
            return lib.get();
    }

    return nullptr;
}


bool LIB_MANAGER_ADAPTER::GetAttr( wxDataViewItem const& aItem, unsigned int aCol,
        wxDataViewItemAttr& aAttr ) const
{
    // change attributes only for the name field
    if( aCol != 0 )
        return false;

    auto node = ToNode( aItem );
    wxCHECK( node, false );

    switch( node->Type )
    {
        case CMP_TREE_NODE::LIB:
            // mark modified libs with bold font
            aAttr.SetBold( m_libMgr->IsLibraryModified( node->Name ) );
            break;

        case CMP_TREE_NODE::LIBID:
            // mark modified part with bold font
            aAttr.SetBold( m_libMgr->IsPartModified( node->Name, node->Parent->Name ) );

            // mark aliases with italic font
            aAttr.SetItalic( !node->IsRoot );
            break;

        default:
            return false;
    }

    return true;
}


LIB_MANAGER_ADAPTER::LIB_MANAGER_ADAPTER( LIB_MANAGER* aLibMgr )
    : m_libMgr( aLibMgr )
{
}
