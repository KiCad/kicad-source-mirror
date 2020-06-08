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


#include <wx/wupdlock.h>
#include <wx/dataview.h>
#include <widgets/ui_common.h>
#include <marker_base.h>
#include <eda_draw_frame.h>
#include <rc_item.h>
#include <base_units.h>

#define WX_DATAVIEW_WINDOW_PADDING 6


wxString RC_ITEM::GetErrorMessage() const
{
    if( m_errorMessage.IsEmpty() )
        return GetErrorText();
    else
        return m_errorMessage;
}


wxString RC_ITEM::ShowCoord( EDA_UNITS aUnits, const wxPoint& aPos )
{
    return wxString::Format( "@(%s, %s)",
                             MessageTextFromValue( aUnits, aPos.x ),
                             MessageTextFromValue( aUnits, aPos.y ) );
}


wxString RC_ITEM::ShowReport( EDA_UNITS aUnits, const std::map<KIID, EDA_ITEM*>& aItemMap ) const
{
    EDA_ITEM* mainItem = nullptr;
    EDA_ITEM* auxItem = nullptr;

    if( m_mainItemUuid != niluuid )
        mainItem = aItemMap.at( m_mainItemUuid );

    if( m_auxItemUuid != niluuid )
        auxItem = aItemMap.at( m_auxItemUuid );

    if( mainItem && auxItem )
    {
        return wxString::Format( wxT( "ErrType(%d): %s\n    %s: %s\n    %s: %s\n" ),
                                 GetErrorCode(),
                                 GetErrorMessage(),
                                 ShowCoord( aUnits, mainItem->GetPosition() ),
                                 mainItem->GetSelectMenuText( aUnits ),
                                 ShowCoord( aUnits, auxItem->GetPosition() ),
                                 auxItem->GetSelectMenuText( aUnits ) );
    }
    else if( mainItem )
    {
        return wxString::Format( wxT( "ErrType(%d): %s\n    %s: %s\n" ),
                                 GetErrorCode(),
                                 GetErrorMessage(),
                                 ShowCoord( aUnits, mainItem->GetPosition() ),
                                 mainItem->GetSelectMenuText( aUnits ) );
    }
    else
    {
        return wxString::Format( wxT( "ErrType(%d): %s\n" ),
                                 GetErrorCode(),
                                 GetErrorMessage() );
    }
}


KIID RC_TREE_MODEL::ToUUID( wxDataViewItem aItem )
{
    const RC_TREE_NODE* node = RC_TREE_MODEL::ToNode( aItem );

    if( node )
    {
        const RC_ITEM* rc_item = node->m_RcItem;

        switch( node->m_Type )
        {
        case RC_TREE_NODE::MARKER:
            // rc_item->GetParent() can be null, if the parent is not existing
            // when a RC item has no corresponding ERC/DRC marker
            if( rc_item->GetParent() )
                return rc_item->GetParent()->GetUUID();

            break;

        case RC_TREE_NODE::MAIN_ITEM: return rc_item->GetMainItemID();
        case RC_TREE_NODE::AUX_ITEM:  return rc_item->GetAuxItemID();
        case RC_TREE_NODE::AUX_ITEM2: return rc_item->GetAuxItem2ID();
        case RC_TREE_NODE::AUX_ITEM3: return rc_item->GetAuxItem3ID();
        }
    }

    return niluuid;
}


RC_TREE_MODEL::RC_TREE_MODEL( EDA_DRAW_FRAME* aParentFrame, wxDataViewCtrl* aView ) :
        m_editFrame( aParentFrame ),
        m_view( aView ),
        m_severities( 0 ),
        m_rcItemsProvider( nullptr )
{
    m_view->GetMainWindow()->Connect( wxEVT_SIZE,
                                      wxSizeEventHandler( RC_TREE_MODEL::onSizeView ),
                                      NULL, this );
}


RC_TREE_MODEL::~RC_TREE_MODEL()
{
    delete m_rcItemsProvider;

    for( RC_TREE_NODE* topLevelNode : m_tree )
        delete topLevelNode;
}


void RC_TREE_MODEL::rebuildModel( RC_ITEMS_PROVIDER* aProvider, int aSeverities )
{
    wxWindowUpdateLocker updateLock( m_view );

    // Even with the updateLock, wxWidgets sometimes ties its knickers in
    // a knot when trying to run a wxdataview_selection_changed_callback()
    // on a row that has been deleted.
    if( m_view )
        m_view->UnselectAll();

    if( aProvider != m_rcItemsProvider )
    {
        delete m_rcItemsProvider;
        m_rcItemsProvider = aProvider;
    }

    if( aSeverities != m_severities )
        m_severities = aSeverities;

    if( m_rcItemsProvider )
        m_rcItemsProvider->SetSeverities( m_severities );

    m_tree.clear();

    for( int i = 0; m_rcItemsProvider && i < m_rcItemsProvider->GetCount(); ++i )
    {
        RC_ITEM* rcItem = m_rcItemsProvider->GetItem( i );

        m_tree.push_back( new RC_TREE_NODE( nullptr, rcItem, RC_TREE_NODE::MARKER ) );
        RC_TREE_NODE* n = m_tree.back();

        if( rcItem->GetMainItemID() != niluuid )
            n->m_Children.push_back( new RC_TREE_NODE( n, rcItem, RC_TREE_NODE::MAIN_ITEM ) );

        if( rcItem->GetAuxItemID() != niluuid )
            n->m_Children.push_back( new RC_TREE_NODE( n, rcItem, RC_TREE_NODE::AUX_ITEM ) );

        if( rcItem->GetAuxItem2ID() != niluuid )
            n->m_Children.push_back( new RC_TREE_NODE( n, rcItem, RC_TREE_NODE::AUX_ITEM2 ) );

        if( rcItem->GetAuxItem3ID() != niluuid )
            n->m_Children.push_back( new RC_TREE_NODE( n, rcItem, RC_TREE_NODE::AUX_ITEM3 ) );
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


void RC_TREE_MODEL::SetProvider( RC_ITEMS_PROVIDER* aProvider )
{
    rebuildModel( aProvider, m_severities );
}


void RC_TREE_MODEL::SetSeverities( int aSeverities )
{
    rebuildModel( m_rcItemsProvider, aSeverities );
}


void RC_TREE_MODEL::ExpandAll()
{
    for( RC_TREE_NODE* topLevelNode : m_tree )
        m_view->Expand( ToItem( topLevelNode ) );
}


bool RC_TREE_MODEL::IsContainer( wxDataViewItem const& aItem ) const
{
    if( ToNode( aItem ) == nullptr )    // must be tree root...
        return true;
    else
        return ToNode( aItem )->m_Type == RC_TREE_NODE::MARKER;
}


wxDataViewItem RC_TREE_MODEL::GetParent( wxDataViewItem const& aItem ) const
{
    return ToItem( ToNode( aItem)->m_Parent );
}


unsigned int RC_TREE_MODEL::GetChildren( wxDataViewItem const& aItem,
                                          wxDataViewItemArray&  aChildren ) const
{
    const RC_TREE_NODE* node = ToNode( aItem );
    const std::vector<RC_TREE_NODE*>& children = node ? node->m_Children : m_tree;

    for( const RC_TREE_NODE* child: children )
        aChildren.push_back( ToItem( child ) );

    return children.size();
}


/**
 * Called by the wxDataView to fetch an item's value.
 */
void RC_TREE_MODEL::GetValue( wxVariant&              aVariant,
                              wxDataViewItem const&   aItem,
                              unsigned int            aCol ) const
{
    const RC_TREE_NODE* node = ToNode( aItem );
    const RC_ITEM*      rcItem = node->m_RcItem;

    switch( node->m_Type )
    {
    case RC_TREE_NODE::MARKER:
    {
        wxString prefix;

        if( rcItem->GetParent() && rcItem->GetParent()->IsExcluded() )
            prefix = _( "Excluded " );

        switch( m_editFrame->GetSeverity( rcItem->GetErrorCode() ) )
        {
        case RPT_SEVERITY_ERROR:   prefix += _( "Error: " ); break;
        case RPT_SEVERITY_WARNING: prefix += _( "Warning: " ); break;
        }

        aVariant = prefix + rcItem->GetErrorMessage();
    }
        break;

    case RC_TREE_NODE::MAIN_ITEM:
    {
        EDA_ITEM* item = m_editFrame->GetItem( rcItem->GetMainItemID() );
        aVariant = item->GetSelectMenuText( m_editFrame->GetUserUnits() );
    }
        break;

    case RC_TREE_NODE::AUX_ITEM:
    {
        EDA_ITEM* item = m_editFrame->GetItem( rcItem->GetAuxItemID() );
        aVariant = item->GetSelectMenuText( m_editFrame->GetUserUnits() );
    }
        break;

    case RC_TREE_NODE::AUX_ITEM2:
    {
        EDA_ITEM* item = m_editFrame->GetItem( rcItem->GetAuxItem2ID() );
        aVariant = item->GetSelectMenuText( m_editFrame->GetUserUnits() );
    }
        break;

    case RC_TREE_NODE::AUX_ITEM3:
    {
        EDA_ITEM* item = m_editFrame->GetItem( rcItem->GetAuxItem3ID() );
        aVariant = item->GetSelectMenuText( m_editFrame->GetUserUnits() );
    }
        break;
    }
}


/**
 * Called by the wxDataView to fetch an item's formatting.  Return true iff the
 * item has non-default attributes.
 */
bool RC_TREE_MODEL::GetAttr( wxDataViewItem const&   aItem,
                             unsigned int            aCol,
                             wxDataViewItemAttr&     aAttr ) const
{
    const RC_TREE_NODE* node = ToNode( aItem );
    wxASSERT( node );

    bool ret = false;
    bool heading = node->m_Type == RC_TREE_NODE::MARKER;

    if( heading )
    {
        aAttr.SetBold( true );
        ret = true;
    }

    if( node->m_RcItem->GetParent() && node->m_RcItem->GetParent()->IsExcluded() )
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


void RC_TREE_MODEL::ValueChanged( RC_TREE_NODE* aNode )
{
    if( aNode->m_Type == RC_TREE_NODE::MAIN_ITEM || aNode->m_Type == RC_TREE_NODE::AUX_ITEM )
    {
        ValueChanged( aNode->m_Parent );
    }

    if( aNode->m_Type == RC_TREE_NODE::MARKER )
    {
        wxDataViewModel::ValueChanged( ToItem( aNode ), 0 );

        for( auto & child : aNode->m_Children )
            wxDataViewModel::ValueChanged( ToItem( child ), 0 );
    }
}


void RC_TREE_MODEL::DeleteCurrentItem( bool aDeep )
{
    RC_TREE_NODE*  tree_node = ToNode( m_view->GetCurrentItem() );
    const RC_ITEM* drc_item = tree_node ? tree_node->m_RcItem : nullptr;

    if( !drc_item )
    {
        wxBell();
        return;
    }

    for( int i = 0; i < m_rcItemsProvider->GetCount(); ++i )
    {
        if( m_rcItemsProvider->GetItem( i ) == drc_item )
        {
            wxDataViewItem      markerItem = ToItem( m_tree[i] );
            wxDataViewItemArray childItems;
            wxDataViewItem      parentItem = ToItem( m_tree[i]->m_Parent );

            for( RC_TREE_NODE* child : m_tree[i]->m_Children )
            {
                childItems.push_back( ToItem( child ) );
                delete child;
            }

            m_tree[i]->m_Children.clear();
            ItemsDeleted( markerItem, childItems );

            delete m_tree[i];
            m_tree.erase( m_tree.begin() + i );
            ItemDeleted( parentItem, markerItem );

            m_rcItemsProvider->DeleteItem( i, aDeep );
            break;
        }
    }
}


void RC_TREE_MODEL::DeleteAllItems()
{
    if( m_rcItemsProvider )
    {
        m_rcItemsProvider->DeleteAllItems();

        m_tree.clear();
        Cleared();
    }
}


void RC_TREE_MODEL::onSizeView( wxSizeEvent& aEvent )
{
    int width = m_view->GetMainWindow()->GetRect().GetWidth() - WX_DATAVIEW_WINDOW_PADDING;

    if( m_view->GetColumnCount() > 0 )
        m_view->GetColumn( 0 )->SetWidth( width );

    // Pass size event to other widgets
    aEvent.Skip();
}
