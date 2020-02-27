/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */


#include <drc/drc_tree_model.h>
#include <wx/wupdlock.h>


#define WX_DATAVIEW_WINDOW_PADDING 6


BOARD_ITEM* DRC_TREE_MODEL::ToBoardItem( BOARD* aBoard, wxDataViewItem aItem )
{
    BOARD_ITEM*          board_item = nullptr;
    const DRC_TREE_NODE* node = DRC_TREE_MODEL::ToNode( aItem );

    if( node )
    {
        const DRC_ITEM* drc_item = node->m_DrcItem;

        switch( node->m_Type )
        {
        case DRC_TREE_NODE::MARKER:
            board_item = static_cast<MARKER_PCB*>( drc_item->GetParent() );
            break;
        case DRC_TREE_NODE::MAIN_ITEM:
            board_item = drc_item->GetMainItem( aBoard );
            break;
        case DRC_TREE_NODE::AUX_ITEM:
            board_item = drc_item->GetAuxiliaryItem( aBoard );
            break;
        }
    }

    return board_item;
}


DRC_TREE_MODEL::DRC_TREE_MODEL( wxDataViewCtrl* aView ) :
    m_view( aView ),
    m_drcItemsProvider( nullptr )
{
    m_view->GetMainWindow()->Connect( wxEVT_SIZE,
                                      wxSizeEventHandler( DRC_TREE_MODEL::onSizeView ),
                                      NULL, this );
}


DRC_TREE_MODEL::~DRC_TREE_MODEL()
{
    delete m_drcItemsProvider;
}


void DRC_TREE_MODEL::SetProvider( DRC_ITEMS_PROVIDER* aProvider )
{
    wxWindowUpdateLocker updateLock( m_view );

    // Even with the updateLock, wxWidgets sometimes ties its knickers in
    // a knot when trying to run a wxdataview_selection_changed_callback()
    // on a row that has been deleted.
    m_view->UnselectAll();

    Cleared();

    delete m_drcItemsProvider;
    m_drcItemsProvider = aProvider;
    m_tree.clear();

#define PUSH_NODE( p, item, type ) push_back( std::make_unique<DRC_TREE_NODE>( p, item, type ) )

    for( int i = 0; m_drcItemsProvider && i < m_drcItemsProvider->GetCount(); ++i )
    {
        const DRC_ITEM* drcItem = m_drcItemsProvider->GetItem( i );

        m_tree.PUSH_NODE( nullptr, drcItem, DRC_TREE_NODE::MARKER );
        DRC_TREE_NODE* node = m_tree.back().get();

        node->m_Children.PUSH_NODE( node, drcItem, DRC_TREE_NODE::MAIN_ITEM );

        if( drcItem->HasSecondItem() )
            node->m_Children.PUSH_NODE( node, drcItem, DRC_TREE_NODE::AUX_ITEM );
    }

#ifdef __WXGTK__
    // The fastest method to update wxDataViewCtrl is to rebuild from
    // scratch by calling Cleared(). Linux requires to reassociate model to
    // display data, but Windows will create multiple associations.
    // On MacOS, this crashes kicad. See https://gitlab.com/kicad/code/kicad/issues/3666
    // and https://gitlab.com/kicad/code/kicad/issues/3653
    m_view->AssociateModel( this );
#endif

    m_view->ClearColumns();
    int width = m_view->GetMainWindow()->GetRect().GetWidth() - WX_DATAVIEW_WINDOW_PADDING;
    m_view->AppendTextColumn( wxEmptyString, 0, wxDATAVIEW_CELL_INERT, width );

    ExpandAll();
}


void DRC_TREE_MODEL::ExpandAll()
{
    for( std::unique_ptr<DRC_TREE_NODE>& markerNode : m_tree )
        m_view->Expand( ToItem( markerNode.get() ) );
}


bool DRC_TREE_MODEL::IsContainer( wxDataViewItem const& aItem ) const
{
    if( ToNode( aItem ) == nullptr )    // must be tree root...
        return true;
    else
        return ToNode( aItem )->m_Type == DRC_TREE_NODE::MARKER;
}


wxDataViewItem DRC_TREE_MODEL::GetParent( wxDataViewItem const& aItem ) const
{
    return ToItem( ToNode( aItem)->m_Parent );
}


unsigned int DRC_TREE_MODEL::GetChildren( wxDataViewItem const& aItem,
                                          wxDataViewItemArray&  aChildren ) const
{
    const DRC_TREE_NODE* p = ToNode( aItem );
    const std::vector<std::unique_ptr<DRC_TREE_NODE>>& children = p ? p->m_Children : m_tree;

    for( const std::unique_ptr<DRC_TREE_NODE>& child: children )
        aChildren.Add( ToItem( child.get() ) );

    return children.size();
}


/**
 * Called by the wxDataView to fetch an item's value.
 */
void DRC_TREE_MODEL::GetValue( wxVariant&              aVariant,
                               wxDataViewItem const&   aItem,
                               unsigned int            aCol ) const
{
    const DRC_TREE_NODE* node = ToNode( aItem );
    wxASSERT( node );

    switch( node->m_Type )
    {
    case DRC_TREE_NODE::MARKER:    aVariant = node->m_DrcItem->GetErrorText();     break;
    case DRC_TREE_NODE::MAIN_ITEM: aVariant = node->m_DrcItem->GetMainText();      break;
    case DRC_TREE_NODE::AUX_ITEM:  aVariant = node->m_DrcItem->GetAuxiliaryText(); break;
    }
}


/**
 * Called by the wxDataView to fetch an item's formatting.  Return true iff the
 * item has non-default attributes.
 */
bool DRC_TREE_MODEL::GetAttr( wxDataViewItem const&   aItem,
                              unsigned int            aCol,
                              wxDataViewItemAttr&     aAttr ) const
{
    const DRC_TREE_NODE* node = ToNode( aItem );
    wxASSERT( node );

    switch( node->m_Type )
    {
    case DRC_TREE_NODE::MARKER:     aAttr.SetBold( true );  return true;
    case DRC_TREE_NODE::MAIN_ITEM:                          return false;
    case DRC_TREE_NODE::AUX_ITEM:                           return false;
    }

    return false;
}


void DRC_TREE_MODEL::DeleteCurrentItem()
{
    wxDataViewItem  dataViewItem = m_view->GetCurrentItem();
    DRC_TREE_NODE*  tree_node = ToNode( dataViewItem );
    const DRC_ITEM* drc_item = tree_node ? tree_node->m_DrcItem : nullptr;

    if( !drc_item )
    {
        wxBell();
        return;
    }

    for( int i = 0; i < m_drcItemsProvider->GetCount(); ++i )
    {
        if( m_drcItemsProvider->GetItem( i ) == drc_item )
        {
            m_drcItemsProvider->DeleteItem( i );
            m_tree.erase( m_tree.begin() + i );

            ItemDeleted( ToItem( nullptr ), dataViewItem );
            break;
        }
    }
}


void DRC_TREE_MODEL::DeleteAllItems()
{
    if( m_drcItemsProvider )
    {
        m_drcItemsProvider->DeleteAllItems();

        m_tree.clear();
        Cleared();
    }
}


void DRC_TREE_MODEL::onSizeView( wxSizeEvent& aEvent )
{
    int width = m_view->GetMainWindow()->GetRect().GetWidth() - WX_DATAVIEW_WINDOW_PADDING;

    if( m_view->GetColumnCount() > 0 )
        m_view->GetColumn( 0 )->SetWidth( width );
}
