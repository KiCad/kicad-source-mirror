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
#include <symbol_lib_table.h>
#include <class_libentry.h>


CMP_TREE_MODEL_ADAPTER_BASE::PTR LIB_MANAGER_ADAPTER::Create( LIB_MANAGER* aLibMgr )
{
    auto adapter = new LIB_MANAGER_ADAPTER( aLibMgr );
    auto container = CMP_TREE_MODEL_ADAPTER_BASE::PTR( adapter );
    return container;
}


void LIB_MANAGER_ADAPTER::AddLibrary( const wxString& aLibNickname )
{
}


void LIB_MANAGER_ADAPTER::AddAliasList( const wxString& aNodeName,
        const wxArrayString& aAliasNameList )
{
    wxASSERT( false );      // TODO
}


bool LIB_MANAGER_ADAPTER::IsContainer( const wxDataViewItem& aItem ) const
{
    const CMP_TREE_NODE* node = ToNode( aItem );
    return node ? node->Type == CMP_TREE_NODE::LIB : true;
}


#define PROGRESS_INTERVAL_MILLIS 66

void LIB_MANAGER_ADAPTER::Sync( bool aForce, std::function<void(int, int, const wxString&)> aProgressCallback )
{
    wxLongLong nextUpdate = wxGetUTCTimeMillis() + (PROGRESS_INTERVAL_MILLIS / 2);

    int libMgrHash = m_libMgr->GetHash();

    if( !aForce && m_lastSyncHash == libMgrHash )
        return;

    m_lastSyncHash = libMgrHash;
    int i = 0, max = GetLibrariesCount();

    // Process already stored libraries
    for( auto it = m_tree.Children.begin(); it != m_tree.Children.end(); /* iteration inside */ )
    {
        const wxString& name = it->get()->Name;

        if( wxGetUTCTimeMillis() > nextUpdate )
        {
            aProgressCallback( i, max, name );
            nextUpdate = wxGetUTCTimeMillis() + PROGRESS_INTERVAL_MILLIS;
        }

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
        ++i;
    }

    // Look for new libraries
    for( const auto& libName : m_libMgr->GetLibraryNames() )
    {
        if( m_libHashes.count( libName ) == 0 )
        {
            if( wxGetUTCTimeMillis() > nextUpdate )
            {
                aProgressCallback( i++, max, libName );
                nextUpdate = wxGetUTCTimeMillis() + PROGRESS_INTERVAL_MILLIS;
            }

            SYMBOL_LIB_TABLE_ROW* library = m_libMgr->GetLibrary( libName );

            auto& lib_node = m_tree.AddLib( libName, library->GetDescr() );
            updateLibrary( lib_node );
            m_tree.AssignIntrinsicRanks();
        }
    }
}


int LIB_MANAGER_ADAPTER::GetLibrariesCount() const
{
    int count = CMP_TREE_MODEL_ADAPTER_BASE::GetLibrariesCount();

    for( const auto& libName : m_libMgr->GetLibraryNames() )
    {
        if( m_libHashes.count( libName ) == 0 )
            ++count;
    }

    return count;
}


void LIB_MANAGER_ADAPTER::updateLibrary( CMP_TREE_NODE_LIB& aLibNode )
{
    auto hashIt = m_libHashes.find( aLibNode.Name );

    if( hashIt == m_libHashes.end() )
    {
        // add a new library
        for( auto alias : m_libMgr->GetAliases( aLibNode.Name ) )
            aLibNode.AddAlias( alias );
    }
    else if( hashIt->second != m_libMgr->GetLibraryHash( aLibNode.Name ) )
    {
        // update an existing libary
        std::list<LIB_ALIAS*> aliases = m_libMgr->GetAliases( aLibNode.Name );

        // remove the common part from the aliases list
        for( auto nodeIt = aLibNode.Children.begin(); nodeIt != aLibNode.Children.end(); /**/ )
        {
            auto aliasIt = std::find_if( aliases.begin(), aliases.end(),
                    [&] ( const LIB_ALIAS* a ) {
                        return a->GetName() == (*nodeIt)->Name;
                    } );

            if( aliasIt != aliases.end() )
            {
                // alias exists both in the component tree and the library manager,
                // update only the node data
                static_cast<CMP_TREE_NODE_LIB_ID*>( nodeIt->get() )->Update( *aliasIt );
                aliases.erase( aliasIt );
                ++nodeIt;
            }
            else
            {
                // node does not exist in the library manager, remove the corresponding node
                nodeIt = aLibNode.Children.erase( nodeIt );
            }
        }

        // now the aliases list contains only new aliases that need to be added to the tree
        for( auto alias : aliases )
            aLibNode.AddAlias( alias );
    }

    aLibNode.AssignIntrinsicRanks();
    m_libHashes[aLibNode.Name] = m_libMgr->GetLibraryHash( aLibNode.Name );
}


CMP_TREE_NODE::PTR_VECTOR::iterator LIB_MANAGER_ADAPTER::deleteLibrary(
            CMP_TREE_NODE::PTR_VECTOR::iterator& aLibNodeIt )
{
    CMP_TREE_NODE* node = aLibNodeIt->get();
    m_libHashes.erase( node->Name );
    auto it = m_tree.Children.erase( aLibNodeIt );
    return it;
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


void LIB_MANAGER_ADAPTER::GetValue( wxVariant& aVariant, wxDataViewItem const& aItem,
                                    unsigned int aCol ) const
{
    auto node = ToNode( aItem );
    wxASSERT( node );

    switch( aCol )
    {
    case 0:
        aVariant = node->Name;

        // mark modified libs with an asterix
        if( node->Type == CMP_TREE_NODE::LIB && m_libMgr->IsLibraryModified( node->Name ) )
            aVariant = node->Name + " *";

        // mark modified parts with an asterix
        if( node->Type == CMP_TREE_NODE::LIBID
                && m_libMgr->IsPartModified( node->Name, node->Parent->Name ) )
            aVariant = node->Name + " *";

        break;

    case 1:
        aVariant = node->Desc;
        break;

    default:    // column == -1 is used for default Compare function
        aVariant = node->Name;
        break;
    }
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

#ifdef __WXGTK__
            // The native wxGTK+ impl ignores background colour, so set the text colour instead.
            // This works reasonably well in dark themes, and quite poorly in light ones....
            if( node->Name == m_libMgr->GetCurrentLib() )
                aAttr.SetColour( wxSystemSettings::GetColour( wxSYS_COLOUR_HIGHLIGHT ) );
#else
            // mark the current library with background color
            if( node->Name == m_libMgr->GetCurrentLib() )
                aAttr.SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_HIGHLIGHT ) );
#endif
            break;

        case CMP_TREE_NODE::LIBID:
            // mark modified part with bold font
            aAttr.SetBold( m_libMgr->IsPartModified( node->Name, node->Parent->Name ) );

            // mark aliases with italic font
            aAttr.SetItalic( !node->IsRoot );

#ifdef __WXGTK__
            // The native wxGTK+ impl ignores background colour, so set the text colour instead.
            // This works reasonably well in dark themes, and quite poorly in light ones....
            if( node->LibId == m_libMgr->GetCurrentLibId() )
                aAttr.SetColour( wxSystemSettings::GetColour( wxSYS_COLOUR_HIGHLIGHT ) );
#else
            // mark the current part with background color
            if( node->LibId == m_libMgr->GetCurrentLibId() )
                aAttr.SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_HIGHLIGHT ) );
#endif
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
