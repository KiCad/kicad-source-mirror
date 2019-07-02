/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 CERN
 * Copyright (C) 2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <symbol_tree_synchronizing_adapter.h>
#include <lib_manager.h>
#include <symbol_lib_table.h>
#include <class_libentry.h>
#include <tool/tool_manager.h>
#include <tools/lib_control.h>


LIB_TREE_MODEL_ADAPTER::PTR SYMBOL_TREE_SYNCHRONIZING_ADAPTER::Create( LIB_EDIT_FRAME* aParent,
                                                                       LIB_MANAGER* aLibMgr )
{
    return PTR( new SYMBOL_TREE_SYNCHRONIZING_ADAPTER( aParent, aLibMgr ) );
}


SYMBOL_TREE_SYNCHRONIZING_ADAPTER::SYMBOL_TREE_SYNCHRONIZING_ADAPTER( LIB_EDIT_FRAME* aParent,
                                                                      LIB_MANAGER* aLibMgr ) :
        m_frame( aParent ),
        m_libMgr( aLibMgr ),
        m_lastSyncHash( -1 )
{
}


TOOL_INTERACTIVE* SYMBOL_TREE_SYNCHRONIZING_ADAPTER::GetContextMenuTool()
{
    return m_frame->GetToolManager()->GetTool<LIB_CONTROL>();
}


bool SYMBOL_TREE_SYNCHRONIZING_ADAPTER::IsContainer( const wxDataViewItem& aItem ) const
{
    const LIB_TREE_NODE* node = ToNode( aItem );
    return node ? node->Type == LIB_TREE_NODE::LIB : true;
}


#define PROGRESS_INTERVAL_MILLIS 120

void SYMBOL_TREE_SYNCHRONIZING_ADAPTER::Sync( bool aForce,
        std::function<void( int, int, const wxString& )> aProgressCallback )
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

        // There is a bug in LIB_MANAGER::LibraryExists() that uses the buffered modified
        // libraries before the symbol library table which prevents the library from being
        // removed from the tree control.
        if( !m_libMgr->LibraryExists( name, true )
          || !m_frame->Prj().SchSymbolLibTable()->HasLibrary( name, true ) )
        {
            it = deleteLibrary( it );
            continue;
        }
        else if( m_libMgr->GetLibraryHash( name ) != m_libHashes[name] )
        {
            updateLibrary( *(LIB_TREE_NODE_LIB*) it->get() );
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
        }
    }

    m_tree.AssignIntrinsicRanks();
}


int SYMBOL_TREE_SYNCHRONIZING_ADAPTER::GetLibrariesCount() const
{
    int count = LIB_TREE_MODEL_ADAPTER::GetLibrariesCount();

    for( const auto& libName : m_libMgr->GetLibraryNames() )
    {
        if( m_libHashes.count( libName ) == 0 )
            ++count;
    }

    return count;
}


void SYMBOL_TREE_SYNCHRONIZING_ADAPTER::updateLibrary( LIB_TREE_NODE_LIB& aLibNode )
{
    auto hashIt = m_libHashes.find( aLibNode.Name );

    if( hashIt == m_libHashes.end() )
    {
        // add a new library
        for( auto alias : m_libMgr->GetAliases( aLibNode.Name ) )
            aLibNode.AddItem( alias );
    }
    else if( hashIt->second != m_libMgr->GetLibraryHash( aLibNode.Name ) )
    {
        // update an existing library
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
                static_cast<LIB_TREE_NODE_LIB_ID*>( nodeIt->get() )->Update( *aliasIt );
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
            aLibNode.AddItem( alias );
    }

    aLibNode.AssignIntrinsicRanks();
    m_libHashes[aLibNode.Name] = m_libMgr->GetLibraryHash( aLibNode.Name );
}


LIB_TREE_NODE::PTR_VECTOR::iterator SYMBOL_TREE_SYNCHRONIZING_ADAPTER::deleteLibrary(
            LIB_TREE_NODE::PTR_VECTOR::iterator& aLibNodeIt )
{
    LIB_TREE_NODE* node = aLibNodeIt->get();
    m_libHashes.erase( node->Name );
    auto it = m_tree.Children.erase( aLibNodeIt );
    return it;
}


void SYMBOL_TREE_SYNCHRONIZING_ADAPTER::GetValue( wxVariant& aVariant, wxDataViewItem const& aItem,
                                                  unsigned int aCol ) const
{
    if( IsFrozen() )
    {
        aVariant = wxEmptyString;
        return;
    }

    auto node = ToNode( aItem );
    wxASSERT( node );

    switch( aCol )
    {
    case 0:
        aVariant = node->Name;

        // mark modified libs with an asterisk
        if( node->Type == LIB_TREE_NODE::LIB && m_libMgr->IsLibraryModified( node->Name ) )
            aVariant = node->Name + " *";

        // mark modified parts with an aster-ix
        if( node->Type == LIB_TREE_NODE::LIBID
                && m_libMgr->IsPartModified( node->Name, node->Parent->Name ) )
            aVariant = node->Name + " *";

        break;

    case 1:
        if( node->LibId == m_libMgr->GetCurrentLibId() )
        {
            LIB_ALIAS* alias = nullptr;

            // When the node parent name is empty, the node is a lib name, not a symbol name
            if( !node->Parent->Name.IsEmpty() )
                alias = m_libMgr->GetAlias( node->Name, node->Parent->Name );

            if( alias )
                aVariant = alias->GetDescription();
            else
                aVariant = node->Desc;
        }
        else
            aVariant = node->Desc;
        break;

    default:    // column == -1 is used for default Compare function
        aVariant = node->Name;
        break;
    }
}


bool SYMBOL_TREE_SYNCHRONIZING_ADAPTER::GetAttr( wxDataViewItem const& aItem, unsigned int aCol,
                                                 wxDataViewItemAttr& aAttr ) const
{
    if( IsFrozen() )
        return false;

    // change attributes only for the name field
    if( aCol != 0 )
        return false;

    auto node = ToNode( aItem );
    wxCHECK( node, false );

    switch( node->Type )
    {
    case LIB_TREE_NODE::LIB:
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
        {
            aAttr.SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_HIGHLIGHT  ) );
            aAttr.SetColour( wxSystemSettings::GetColour( wxSYS_COLOUR_LISTBOXHIGHLIGHTTEXT  ) );
        }
#endif
        break;

    case LIB_TREE_NODE::LIBID:
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
        {
            aAttr.SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_HIGHLIGHT ) );
            aAttr.SetColour( wxSystemSettings::GetColour( wxSYS_COLOUR_LISTBOXHIGHLIGHTTEXT  ) );
        }
#endif
        break;

    default:
        return false;
    }

    return true;
}
