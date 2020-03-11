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
#include <widgets/ui_common.h>

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


DRC_TREE_MODEL::DRC_TREE_MODEL( PCB_BASE_FRAME* aParentFrame, wxDataViewCtrl* aView ) :
    m_parentFrame( aParentFrame ),
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

    for( DRC_TREE_NODE* topLevelNode : m_tree )
        delete topLevelNode;
}


void DRC_TREE_MODEL::rebuildModel( DRC_ITEMS_PROVIDER* aProvider, int aSeverities )
{
    wxWindowUpdateLocker updateLock( m_view );

    // Even with the updateLock, wxWidgets sometimes ties its knickers in
    // a knot when trying to run a wxdataview_selection_changed_callback()
    // on a row that has been deleted.
    if( m_view )
        m_view->UnselectAll();

    if( aProvider != m_drcItemsProvider )
    {
        delete m_drcItemsProvider;
        m_drcItemsProvider = aProvider;
    }

    if( aSeverities != m_severities )
        m_severities = aSeverities;

    if( m_drcItemsProvider )
        m_drcItemsProvider->SetSeverities( m_severities );

    m_tree.clear();

    for( int i = 0; m_drcItemsProvider && i < m_drcItemsProvider->GetCount(); ++i )
    {
        DRC_ITEM* drcItem = m_drcItemsProvider->GetItem( i );

        m_tree.push_back( new DRC_TREE_NODE( nullptr, drcItem, DRC_TREE_NODE::MARKER ) );
        DRC_TREE_NODE* n = m_tree.back();

        n->m_Children.push_back( new DRC_TREE_NODE( n, drcItem, DRC_TREE_NODE::MAIN_ITEM ) );

        if( drcItem->GetAuxItemID() != niluuid )
            n->m_Children.push_back( new DRC_TREE_NODE( n, drcItem, DRC_TREE_NODE::AUX_ITEM ) );
    }

    // Must be called after a significant change of items to force the
    // wxDataViewModel to reread all of them, repopulating itself entirely.
    Cleared();

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


void DRC_TREE_MODEL::SetProvider( DRC_ITEMS_PROVIDER* aProvider )
{
    rebuildModel( aProvider, m_severities );
}


void DRC_TREE_MODEL::SetSeverities( int aSeverities )
{
    rebuildModel( m_drcItemsProvider, aSeverities );
}


void DRC_TREE_MODEL::ExpandAll()
{
    for( DRC_TREE_NODE* topLevelNode : m_tree )
        m_view->Expand( ToItem( topLevelNode ) );
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
    const DRC_TREE_NODE* node = ToNode( aItem );
    const std::vector<DRC_TREE_NODE*>& children = node ? node->m_Children : m_tree;

    for( const DRC_TREE_NODE* child: children )
        aChildren.push_back( ToItem( child ) );

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
    const DRC_ITEM*      drcItem = node->m_DrcItem;

    switch( node->m_Type )
    {
    case DRC_TREE_NODE::MARKER:
    {
        auto&    bds = m_parentFrame->GetBoard()->GetDesignSettings();
        bool     excluded = drcItem->GetParent() && drcItem->GetParent()->IsExcluded();
        bool     error = bds.m_DRCSeverities[ drcItem->GetErrorCode() ] == RPT_SEVERITY_ERROR;
        wxString prefix = wxString::Format( wxT( "%s%s" ),
                                            excluded ? _( "Excluded " ) : wxString( "" ),
                                            error  ? _( "Error: " ) : _( "Warning: " ) );

        aVariant = prefix + drcItem->GetErrorText();
    }
        break;

    case DRC_TREE_NODE::MAIN_ITEM:
        aVariant = drcItem->GetMainText();
        break;

    case DRC_TREE_NODE::AUX_ITEM:
        aVariant = drcItem->GetAuxiliaryText();
        break;
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

    bool ret = false;
    bool heading = node->m_Type == DRC_TREE_NODE::MARKER;

    if( heading )
    {
        aAttr.SetBold( true );
        ret = true;
    }

    if( node->m_DrcItem->GetParent() && node->m_DrcItem->GetParent()->IsExcluded() )
    {
        wxColour textColour = wxSystemSettings::GetColour( wxSYS_COLOUR_LISTBOXTEXT );

        if( KIGFX::COLOR4D( textColour ).GetBrightness() > 0.5 )
            aAttr.SetColour( textColour.ChangeLightness( heading ? 30 : 35 ) );
        else
            aAttr.SetColour( textColour.ChangeLightness( heading ? 170 : 165 ) );

        aAttr.SetItalic( true );   // Strikethrough would be better, if wxWidgets supported it
        ret = true;
    }

    return ret;
}


void DRC_TREE_MODEL::ValueChanged( DRC_TREE_NODE* aNode )
{
    if( aNode->m_Type == DRC_TREE_NODE::MAIN_ITEM || aNode->m_Type == DRC_TREE_NODE::AUX_ITEM )
    {
        ValueChanged( aNode->m_Parent );
    }

    if( aNode->m_Type == DRC_TREE_NODE::MARKER )
    {
        wxDataViewModel::ValueChanged( ToItem( aNode ), 0 );

        for( auto & child : aNode->m_Children )
            wxDataViewModel::ValueChanged( ToItem( child ), 0 );
    }
}


void DRC_TREE_MODEL::DeleteCurrentItem( bool aDeep )
{
    DRC_TREE_NODE*  tree_node = ToNode( m_view->GetCurrentItem() );
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
            wxDataViewItem      markerItem = ToItem( m_tree[i] );
            wxDataViewItemArray childItems;
            wxDataViewItem      parentItem = ToItem( m_tree[i]->m_Parent );

            for( DRC_TREE_NODE* child : m_tree[i]->m_Children )
            {
                childItems.push_back( ToItem( child ) );
                delete child;
            }

            m_tree[i]->m_Children.clear();
            ItemsDeleted( markerItem, childItems );

            delete m_tree[i];
            m_tree.erase( m_tree.begin() + i );
            ItemDeleted( parentItem, markerItem );

            m_drcItemsProvider->DeleteItem( i, aDeep );
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

    // Pass size event to other widgets
    aEvent.Skip();
}
