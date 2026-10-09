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

#include <dialogs/git/workspace_tree_model.h>

#include <wx/settings.h>


namespace
{
wxDataViewItem toItem( const WORKSPACE_TREE_NODE* aNode )
{
    return wxDataViewItem( const_cast<WORKSPACE_TREE_NODE*>( aNode ) );
}
} // namespace


void WORKSPACE_TREE_MODEL::Reset( WORKSPACE_TREE_NODE aRoot )
{
    // The control lets go of the old nodes here, while they are still alive
    BeforeReset();

    m_root = std::move( aRoot );
    m_parents.clear();
    index( m_root, nullptr );

    AfterReset();
}


void WORKSPACE_TREE_MODEL::index( const WORKSPACE_TREE_NODE& aNode, const WORKSPACE_TREE_NODE* aParent )
{
    for( const WORKSPACE_TREE_NODE& child : aNode.children )
    {
        m_parents.emplace( &child, aParent );
        index( child, &child );
    }
}


const WORKSPACE_TREE_NODE* WORKSPACE_TREE_MODEL::NodeOf( const wxDataViewItem& aItem ) const
{
    return static_cast<const WORKSPACE_TREE_NODE*>( aItem.GetID() );
}


wxDataViewItem WORKSPACE_TREE_MODEL::ItemOf( const WORKSPACE_TREE_NODE& aNode ) const
{
    return toItem( &aNode );
}


void WORKSPACE_TREE_MODEL::SetGreyed( bool aGreyed )
{
    if( aGreyed == m_greyed )
        return;

    m_greyed = aGreyed;
    RefreshCells();
}


void WORKSPACE_TREE_MODEL::RefreshCells()
{
    wxDataViewItemArray items;

    for( const auto& [node, parent] : m_parents )
        items.Add( toItem( node ) );

    if( !items.IsEmpty() )
        ItemsChanged( items );
}


void WORKSPACE_TREE_MODEL::GetValue( wxVariant& aVariant, const wxDataViewItem& aItem, unsigned int aCol ) const
{
    const WORKSPACE_TREE_NODE* node = NodeOf( aItem );

    aVariant = node && m_cell ? m_cell( *node, aCol ) : wxString();
}


bool WORKSPACE_TREE_MODEL::GetAttr( const wxDataViewItem&, unsigned int, wxDataViewItemAttr& aAttr ) const
{
    if( !m_greyed )
        return false;

    aAttr.SetColour( wxSystemSettings::GetColour( wxSYS_COLOUR_GRAYTEXT ) );
    return true;
}


wxDataViewItem WORKSPACE_TREE_MODEL::GetParent( const wxDataViewItem& aItem ) const
{
    auto it = m_parents.find( NodeOf( aItem ) );

    return it == m_parents.end() ? wxDataViewItem() : toItem( it->second );
}


bool WORKSPACE_TREE_MODEL::IsContainer( const wxDataViewItem& aItem ) const
{
    const WORKSPACE_TREE_NODE* node = NodeOf( aItem );

    if( !node )
        return true;

    switch( node->kind )
    {
    case WORKSPACE_TREE_NODE::KIND::ROOT:
    case WORKSPACE_TREE_NODE::KIND::FOLDER:
    case WORKSPACE_TREE_NODE::KIND::LIBRARIES:
    case WORKSPACE_TREE_NODE::KIND::LIBRARY: return true;
    case WORKSPACE_TREE_NODE::KIND::ITEM:
    case WORKSPACE_TREE_NODE::KIND::MEMBER:
    case WORKSPACE_TREE_NODE::KIND::UNLISTED: return false;
    }

    return false;
}


unsigned int WORKSPACE_TREE_MODEL::GetChildren( const wxDataViewItem& aItem, wxDataViewItemArray& aChildren ) const
{
    const WORKSPACE_TREE_NODE* node = aItem.IsOk() ? NodeOf( aItem ) : &m_root;

    for( const WORKSPACE_TREE_NODE& child : node->children )
        aChildren.Add( toItem( &child ) );

    return node->children.size();
}
