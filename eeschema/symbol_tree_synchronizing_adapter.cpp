/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 CERN
 * Copyright (C) 2019-2021 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <symbol_library_manager.h>
#include <symbol_lib_table.h>
#include <tools/symbol_editor_control.h>


wxObjectDataPtr<LIB_TREE_MODEL_ADAPTER>
SYMBOL_TREE_SYNCHRONIZING_ADAPTER::Create( SYMBOL_EDIT_FRAME* aParent,
                                           SYMBOL_LIBRARY_MANAGER* aLibMgr )
{
    auto* adapter = new SYMBOL_TREE_SYNCHRONIZING_ADAPTER( aParent, aLibMgr );
    return wxObjectDataPtr<LIB_TREE_MODEL_ADAPTER>( adapter );
}


SYMBOL_TREE_SYNCHRONIZING_ADAPTER::SYMBOL_TREE_SYNCHRONIZING_ADAPTER( SYMBOL_EDIT_FRAME* aParent,
                                                                      SYMBOL_LIBRARY_MANAGER* aLibMgr ) :
        LIB_TREE_MODEL_ADAPTER( aParent, "pinned_symbol_libs" ),
        m_frame( aParent ),
        m_libMgr( aLibMgr ),
        m_lastSyncHash( -1 )
{
}


TOOL_INTERACTIVE* SYMBOL_TREE_SYNCHRONIZING_ADAPTER::GetContextMenuTool()
{
    return m_frame->GetToolManager()->GetTool<SYMBOL_EDITOR_CONTROL>();
}


bool SYMBOL_TREE_SYNCHRONIZING_ADAPTER::IsContainer( const wxDataViewItem& aItem ) const
{
    const LIB_TREE_NODE* node = ToNode( aItem );
    return node ? node->m_Type == LIB_TREE_NODE::LIB : true;
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
    for( auto it = m_tree.m_Children.begin(); it != m_tree.m_Children.end(); /* iteration inside */ )
    {
        const wxString& name = it->get()->m_Name;

        if( wxGetUTCTimeMillis() > nextUpdate )
        {
            aProgressCallback( i, max, name );
            nextUpdate = wxGetUTCTimeMillis() + PROGRESS_INTERVAL_MILLIS;
        }

        // There is a bug in SYMBOL_LIBRARY_MANAGER::LibraryExists() that uses the buffered modified
        // libraries before the symbol library table which prevents the library from being
        // removed from the tree control.
        if( !m_libMgr->LibraryExists( name, true )
          || !m_frame->Prj().SchSymbolLibTable()->HasLibrary( name, true )
          || ( m_frame->Prj().SchSymbolLibTable()->FindRow( name, true ) !=
               m_frame->Prj().SchSymbolLibTable()->FindRow( name, false ) ) )
        {
            it = deleteLibrary( it );
            continue;
        }
        else if( aForce || m_libMgr->GetLibraryHash( name ) != m_libHashes[name] )
        {
            updateLibrary( *(LIB_TREE_NODE_LIB*) it->get() );
        }

        ++it;
        ++i;
    }

    // Look for new libraries
    for( const wxString& libName : m_libMgr->GetLibraryNames() )
    {
        if( m_libHashes.count( libName ) == 0 )
        {
            if( wxGetUTCTimeMillis() > nextUpdate )
            {
                aProgressCallback( i++, max, libName );
                nextUpdate = wxGetUTCTimeMillis() + PROGRESS_INTERVAL_MILLIS;
            }

            SYMBOL_LIB_TABLE_ROW* library = m_libMgr->GetLibrary( libName );
            LIB_TREE_NODE_LIB&    lib_node = DoAddLibraryNode( libName, library->GetDescr() );

            updateLibrary( lib_node );
        }
    }

    m_tree.AssignIntrinsicRanks();
}


int SYMBOL_TREE_SYNCHRONIZING_ADAPTER::GetLibrariesCount() const
{
    int count = LIB_TREE_MODEL_ADAPTER::GetLibrariesCount();

    for( const wxString& libName : m_libMgr->GetLibraryNames() )
    {
        if( m_libHashes.count( libName ) == 0 )
            ++count;
    }

    return count;
}


void SYMBOL_TREE_SYNCHRONIZING_ADAPTER::updateLibrary( LIB_TREE_NODE_LIB& aLibNode )
{
    auto hashIt = m_libHashes.find( aLibNode.m_Name );

    if( hashIt == m_libHashes.end() )
    {
        // add a new library
        for( LIB_PART* alias : m_libMgr->GetAliases( aLibNode.m_Name ) )
            aLibNode.AddItem( alias );
    }
    else if( hashIt->second != m_libMgr->GetLibraryHash( aLibNode.m_Name ) )
    {
        // update an existing library
        std::list<LIB_PART*> aliases = m_libMgr->GetAliases( aLibNode.m_Name );

        // remove the common part from the aliases list
        for( auto nodeIt = aLibNode.m_Children.begin(); nodeIt != aLibNode.m_Children.end(); /**/ )
        {
            auto aliasIt = std::find_if( aliases.begin(), aliases.end(),
                                         [&] ( const LIB_PART* a )
                                         {
                                             return a->GetName() == (*nodeIt)->m_Name;
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
                nodeIt = aLibNode.m_Children.erase( nodeIt );
            }
        }

        // now the aliases list contains only new aliases that need to be added to the tree
        for( LIB_PART* alias : aliases )
            aLibNode.AddItem( alias );
    }

    aLibNode.AssignIntrinsicRanks();
    m_libHashes[aLibNode.m_Name] = m_libMgr->GetLibraryHash( aLibNode.m_Name );
}


LIB_TREE_NODE::PTR_VECTOR::iterator SYMBOL_TREE_SYNCHRONIZING_ADAPTER::deleteLibrary(
            LIB_TREE_NODE::PTR_VECTOR::iterator& aLibNodeIt )
{
    LIB_TREE_NODE* node = aLibNodeIt->get();
    m_libHashes.erase( node->m_Name );
    auto it = m_tree.m_Children.erase( aLibNodeIt );
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

    LIB_TREE_NODE* node = ToNode( aItem );
    wxASSERT( node );

    switch( aCol )
    {
    case 0:
        if( m_frame->GetCurPart() && m_frame->GetCurPart()->GetLibId() == node->m_LibId )
            node->m_Name = m_frame->GetCurPart()->GetLibId().GetLibItemName();

        if( node->m_Pinned )
            aVariant = GetPinningSymbol() + node->m_Name;
        else
            aVariant = node->m_Name;

        // mark modified items with an asterisk
        if( node->m_Type == LIB_TREE_NODE::LIB )
        {
            if( m_libMgr->IsLibraryModified( node->m_Name ) )
                aVariant = aVariant.GetString() + " *";
        }
        else if( node->m_Type == LIB_TREE_NODE::LIBID )
        {
            if( m_libMgr->IsPartModified( node->m_Name, node->m_Parent->m_Name ) )
                aVariant = aVariant.GetString() + " *";
        }

        break;

    case 1:
        if( m_frame->GetCurPart() && m_frame->GetCurPart()->GetLibId() == node->m_LibId )
            node->m_Desc = m_frame->GetCurPart()->GetDescription();

        aVariant = node->m_Desc;

        // Annotate that the library failed to load in the description column
        if( node->m_Type == LIB_TREE_NODE::LIB )
        {
            if( !m_libMgr->IsLibraryLoaded( node->m_Name ) )
                aVariant = _( "(failed to load)" ) + wxS( " " ) + aVariant.GetString();
        }

        break;

    default:    // column == -1 is used for default Compare function
        aVariant = node->m_Name;
        break;
    }
}


bool SYMBOL_TREE_SYNCHRONIZING_ADAPTER::GetAttr( wxDataViewItem const& aItem, unsigned int aCol,
                                                 wxDataViewItemAttr& aAttr ) const
{
    if( IsFrozen() )
        return false;

    LIB_TREE_NODE* node = ToNode( aItem );
    wxCHECK( node, false );

    // Mark both columns of unloaded libraries using grey text color (to look disabled)
    if( node->m_Type == LIB_TREE_NODE::LIB && !m_libMgr->IsLibraryLoaded( node->m_Name ) )
    {
        aAttr.SetColour( wxSystemSettings::GetColour( wxSYS_COLOUR_GRAYTEXT  ) );
        return true;
    }

    // The remaining attributes are only for the name column
    if( aCol != 0 )
        return false;

    LIB_PART* curPart = m_frame->GetCurPart();

    switch( node->m_Type )
    {
    case LIB_TREE_NODE::LIB:
        // mark modified libs with bold font
        aAttr.SetBold( m_libMgr->IsLibraryModified( node->m_Name ) );

        // mark the current library with background color
        if( curPart && curPart->GetLibId().GetLibNickname() == node->m_LibId.GetLibNickname() )
        {
#ifdef __WXGTK__
            // The native wxGTK+ impl ignores background colour, so set the text colour instead.
            // This works reasonably well in dark themes, and quite poorly in light ones....
            aAttr.SetColour( wxSystemSettings::GetColour( wxSYS_COLOUR_HIGHLIGHT ) );
#else
            aAttr.SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_HIGHLIGHT  ) );
            aAttr.SetColour( wxSystemSettings::GetColour( wxSYS_COLOUR_LISTBOXHIGHLIGHTTEXT  ) );
#endif
        }
        break;

    case LIB_TREE_NODE::LIBID:
        // mark modified part with bold font
        aAttr.SetBold( m_libMgr->IsPartModified( node->m_Name, node->m_Parent->m_Name ) );

        // mark aliases with italic font
        aAttr.SetItalic( !node->m_IsRoot );

        // mark the current part with background color
        if( curPart && curPart->GetLibId() == node->m_LibId )
        {
#ifdef __WXGTK__
        // The native wxGTK+ impl ignores background colour, so set the text colour instead.
        // This works reasonably well in dark themes, and quite poorly in light ones....
            aAttr.SetColour( wxSystemSettings::GetColour( wxSYS_COLOUR_HIGHLIGHT ) );
#else
            aAttr.SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_HIGHLIGHT ) );
            aAttr.SetColour( wxSystemSettings::GetColour( wxSYS_COLOUR_LISTBOXHIGHLIGHTTEXT  ) );
#endif
        }
        break;

    default:
        return false;
    }

    return true;
}
