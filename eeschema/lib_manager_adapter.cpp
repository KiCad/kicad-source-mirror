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

    updateLibrary( *(CMP_TREE_NODE_LIB*) node );
    Resort();
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
    int libMgrHash = m_libMgr->GetHash();

    if( m_lastSyncHash == libMgrHash )
        return;

    m_lastSyncHash = libMgrHash;

    // Process already stored libraries
    for( auto it = m_tree.Children.begin(); it != m_tree.Children.end(); /* iteration inside */ )
    {
        const wxString& name = it->get()->Name;

        if( !m_libMgr->LibraryExists( name ) )
        {
            it = deleteLibrary( it );
            continue;
        }
        else if( m_libMgr->GetLibraryHash( name ) != m_libHashes[name] )
        {
            updateLibrary( *(CMP_TREE_NODE_LIB*) it->get() );
        }

        ++it;
    }

    // Look for new libraries
    for( const auto& libName : m_libMgr->GetLibraryNames() )
    {
        if( m_libHashes.count( libName ) == 0 )
            AddLibrary( libName );
    }

    m_tree.AssignIntrinsicRanks();
    Resort();
}


void LIB_MANAGER_ADAPTER::updateLibrary( CMP_TREE_NODE_LIB& aLibNode )
{
    if( m_libHashes.count( aLibNode.Name ) == 0 )
    {
        // add a new library
        addAliases( aLibNode );
    }
    else
    {
        // update an existing libary
#if 1
        std::list<LIB_ALIAS*> aliases = m_libMgr->GetAliases( aLibNode.Name );
        wxDataViewItem parent = ToItem( &aLibNode );

        // remove the common part from the aliases list
        //for( const auto& node : aLibNode.Children )
        for( auto nodeIt = aLibNode.Children.begin(); nodeIt != aLibNode.Children.end(); /**/ )
        {
            auto aliasIt = std::find_if( aliases.begin(), aliases.end(),
                    [&] ( const LIB_ALIAS* a ) {
                        return a->GetName() == (*nodeIt)->Name;
                    } );

            if( aliasIt != aliases.end() )
            {
                // alias exists both in the component tree and the library manager,
                // no need to update
                aliases.erase( aliasIt );
                ++nodeIt;
            }
            else
            {
                // node does not exist in the library manager, remove the corresponding node
                ItemDeleted( parent, ToItem( nodeIt->get() ) );
                nodeIt = aLibNode.Children.erase( nodeIt );
            }
        }

        // now the aliases list contains only new aliases that need to be added to the tree
        for( auto alias : aliases )
        {
            auto& aliasNode = aLibNode.AddAlias( alias );
            ItemAdded( parent, ToItem( &aliasNode ) );
        }
#else
        // Bruteforce approach - remove everything and rebuild the branch
        wxDataViewItem parent = ToItem( &aLibNode );

        for( const auto& node : aLibNode.Children )
            ItemDeleted( parent, ToItem( node.get() ) );

        aLibNode.Children.clear();
        addAliases( aLibNode );
#endif
    }

    aLibNode.AssignIntrinsicRanks();
    m_libHashes[aLibNode.Name] = m_libMgr->GetLibraryHash( aLibNode.Name );
}


CMP_TREE_NODE::PTR_VECTOR::iterator LIB_MANAGER_ADAPTER::deleteLibrary(
            CMP_TREE_NODE::PTR_VECTOR::iterator& aLibNodeIt )
{
    m_libHashes.erase( aLibNodeIt->get()->Name );
    auto it = m_tree.Children.erase( aLibNodeIt );
    return it;
}


void LIB_MANAGER_ADAPTER::addAliases( CMP_TREE_NODE_LIB& aLibNode )
{
    wxDataViewItem parent = ToItem( &aLibNode );

    for( auto alias : m_libMgr->GetAliases( aLibNode.Name ) )
    {
        auto& aliasNode = aLibNode.AddAlias( alias );
        ItemAdded( parent, ToItem( &aliasNode ) );
    }
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
    : m_libMgr( aLibMgr ), m_lastSyncHash( -1 )
{
}
