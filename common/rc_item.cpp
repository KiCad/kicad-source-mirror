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
#include <wx/settings.h>
#include <widgets/ui_common.h>
#include <marker_base.h>
#include <eda_draw_frame.h>
#include <rc_item.h>
#include <eda_item.h>
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


void RC_ITEM::AddItem( EDA_ITEM* aItem )
{
    m_ids.push_back( aItem->m_Uuid );
}


void RC_ITEM::SetItems( const EDA_ITEM* aItem, const EDA_ITEM* bItem,
                        const EDA_ITEM* cItem, const EDA_ITEM* dItem )
{
    m_ids.clear();

    m_ids.push_back( aItem->m_Uuid );

    if( bItem )
        m_ids.push_back( bItem->m_Uuid );

    if( cItem )
        m_ids.push_back( cItem->m_Uuid );

    if( dItem )
        m_ids.push_back( dItem->m_Uuid );
}


wxString RC_ITEM::ShowReport( EDA_UNITS aUnits, SEVERITY aSeverity,
                              const std::map<KIID, EDA_ITEM*>& aItemMap ) const
{
    wxString severity;

    switch( aSeverity )
    {
    case RPT_SEVERITY_ERROR:   severity = wxT( "Severity: error" );   break;
    case RPT_SEVERITY_WARNING: severity = wxT( "Severity: warning" ); break;
    case RPT_SEVERITY_ACTION:  severity = wxT( "Severity: action" );  break;
    case RPT_SEVERITY_INFO:    severity = wxT( "Severity: info" );    break;
    default:                   ;
    };

    if( m_parent && m_parent->IsExcluded() )
        severity += wxT( " (excluded)" );

    EDA_ITEM* mainItem = nullptr;
    EDA_ITEM* auxItem = nullptr;

    auto ii = aItemMap.find( GetMainItemID() );

    if( ii != aItemMap.end() )
        mainItem = ii->second;

    ii = aItemMap.find( GetAuxItemID() );

    if( ii != aItemMap.end() )
        auxItem = ii->second;

    // Note: some customers machine-process these.  So:
    // 1) don't translate
    // 2) try not to re-order or change syntax
    // 3) report numeric error code (which should be more stable) in addition to message

    if( mainItem && auxItem )
    {
        return wxString::Format( wxT( "[%s]: %s %s\n    %s: %s\n    %s: %s\n" ),
                                 GetSettingsKey(),
                                 GetErrorMessage(),
                                 severity,
                                 ShowCoord( aUnits, mainItem->GetPosition() ),
                                 mainItem->GetSelectMenuText( aUnits ),
                                 ShowCoord( aUnits, auxItem->GetPosition() ),
                                 auxItem->GetSelectMenuText( aUnits ) );
    }
    else if( mainItem )
    {
        return wxString::Format( wxT( "[%s]: %s %s\n    %s: %s\n" ),
                                 GetSettingsKey(),
                                 GetErrorMessage(),
                                 severity,
                                 ShowCoord( aUnits, mainItem->GetPosition() ),
                                 mainItem->GetSelectMenuText( aUnits ) );
    }
    else
    {
        return wxString::Format( wxT( "[%s]: %s %s\n" ),
                                 GetSettingsKey(),
                                 GetErrorMessage(),
                                 severity );
    }
}


KIID RC_TREE_MODEL::ToUUID( wxDataViewItem aItem )
{
    const RC_TREE_NODE* node = RC_TREE_MODEL::ToNode( aItem );

    if( node && node->m_RcItem )
    {
        const std::shared_ptr<RC_ITEM> rc_item = node->m_RcItem;

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

    std::shared_ptr<RC_ITEM> selectedRcItem = nullptr;

    if( m_view )
    {
        RC_TREE_NODE* selectedNode = ToNode( m_view->GetSelection() );
        selectedRcItem = selectedNode ? selectedNode->m_RcItem : nullptr;

        // Even with the updateLock, wxWidgets sometimes ties its knickers in a knot trying
        // to run a wxdataview_selection_changed_callback() on a row that has been deleted.
        m_view->UnselectAll();
    }

    if( aProvider != m_rcItemsProvider )
    {
        delete m_rcItemsProvider;
        m_rcItemsProvider = aProvider;
    }

    if( aSeverities != m_severities )
        m_severities = aSeverities;

    if( m_rcItemsProvider )
        m_rcItemsProvider->SetSeverities( m_severities );

    for( RC_TREE_NODE* topLevelNode : m_tree )
        delete topLevelNode;

    m_tree.clear();

    // wxDataView::ExpandAll() pukes with large lists
    int count = 0;

    if( m_rcItemsProvider )
        count = std::min( 1000, m_rcItemsProvider->GetCount() );

    for( int i = 0; i < count; ++i )
    {
        std::shared_ptr<RC_ITEM> rcItem = m_rcItemsProvider->GetItem( i );

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

    // Most annoyingly wxWidgets won't tell us the scroll position (and no, all the usual
    // routines don't work), so we can only restore the scroll position based on a selection.
    if( selectedRcItem )
    {
        for( RC_TREE_NODE* candidate : m_tree )
        {
            if( candidate->m_RcItem == selectedRcItem )
            {
                m_view->Select( ToItem( candidate ) );
                m_view->EnsureVisible( ToItem( candidate ) );
                break;
            }
        }
    }
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
    const RC_TREE_NODE*            node = ToNode( aItem );
    const std::shared_ptr<RC_ITEM> rcItem = node->m_RcItem;

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

        case RPT_SEVERITY_EXCLUSION:
        case RPT_SEVERITY_UNDEFINED:
        case RPT_SEVERITY_INFO:
        case RPT_SEVERITY_ACTION:
        case RPT_SEVERITY_IGNORE:
            break;
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


void RC_TREE_MODEL::ValueChanged( const RC_TREE_NODE* aNode )
{
    if( aNode->m_Type == RC_TREE_NODE::MAIN_ITEM || aNode->m_Type == RC_TREE_NODE::AUX_ITEM )
    {
        ValueChanged( aNode->m_Parent );
    }

    if( aNode->m_Type == RC_TREE_NODE::MARKER )
    {
        wxDataViewModel::ValueChanged( ToItem( aNode ), 0 );

        for( const RC_TREE_NODE* child : aNode->m_Children )
            wxDataViewModel::ValueChanged( ToItem( child ), 0 );
    }
}


void RC_TREE_MODEL::DeleteCurrentItem( bool aDeep )
{
    DeleteItems( true, true, aDeep );
}


void RC_TREE_MODEL::DeleteItems( bool aCurrentOnly, bool aIncludeExclusions, bool aDeep )
{
    RC_TREE_NODE* current_node = ToNode( m_view->GetCurrentItem() );
    const std::shared_ptr<RC_ITEM> current_item = current_node ? current_node->m_RcItem : nullptr;

    /// Keep a vector of elements to free after wxWidgets is definitely done accessing them
    std::vector<RC_TREE_NODE*> to_delete;

    if( aCurrentOnly && !current_item )
    {
        wxBell();
        return;
    }

    if( !m_rcItemsProvider )
        return;

    int  lastGood = -1;
    bool itemDeleted = false;

    if( m_view )
    {
        m_view->UnselectAll();
        wxYield();
        m_view->Freeze();
    }

    for( int i = m_rcItemsProvider->GetCount() - 1; i >= 0; --i )
    {
        std::shared_ptr<RC_ITEM> rcItem = m_rcItemsProvider->GetItem( i );
        MARKER_BASE*             marker = rcItem->GetParent();
        bool                     excluded = marker ? marker->IsExcluded() : false;

        if( aCurrentOnly && itemDeleted && lastGood >= 0 )
            break;

        if( aCurrentOnly && rcItem != current_item )
        {
            lastGood = i;
            continue;
        }

        if( excluded && !aIncludeExclusions )
            continue;

        if( i < (int) m_tree.size() )   // Careful; tree is truncated for large datasets
        {
            wxDataViewItem      markerItem = ToItem( m_tree[i] );
            wxDataViewItemArray childItems;
            wxDataViewItem      parentItem = ToItem( m_tree[i]->m_Parent );

            for( RC_TREE_NODE* child : m_tree[i]->m_Children )
            {
                childItems.push_back( ToItem( child ) );
                to_delete.push_back( child );
            }

            m_tree[i]->m_Children.clear();
            ItemsDeleted( markerItem, childItems );

            to_delete.push_back( m_tree[i] );
            m_tree.erase( m_tree.begin() + i );
            ItemDeleted( parentItem, markerItem );
        }

        // Only deep delete the current item here; others will be done through the
        // DeleteAllItems() call below, which is more efficient.
        m_rcItemsProvider->DeleteItem( i, aDeep && aCurrentOnly );

        if( lastGood > i )
            lastGood--;

        itemDeleted = true;
    }

    if( m_view && aCurrentOnly && lastGood >= 0 )
        m_view->Select( ToItem( m_tree[ lastGood ] ) );

    for( RC_TREE_NODE* item : to_delete )
        delete( item );

    if( !aCurrentOnly )
        m_rcItemsProvider->DeleteAllItems( aIncludeExclusions, aDeep );

    if( m_view )
        m_view->Thaw();
}


void RC_TREE_MODEL::PrevMarker()
{
    RC_TREE_NODE* currentNode = ToNode( m_view->GetCurrentItem() );
    RC_TREE_NODE* prevMarker = nullptr;

    while( currentNode && currentNode->m_Type != RC_TREE_NODE::MARKER )
        currentNode = currentNode->m_Parent;

    for( RC_TREE_NODE* candidate : m_tree )
    {
        if( candidate == currentNode )
            break;
        else
            prevMarker = candidate;
    }

    if( prevMarker )
        m_view->Select( ToItem( prevMarker ) );
}


void RC_TREE_MODEL::NextMarker()
{
    RC_TREE_NODE* currentNode = ToNode( m_view->GetCurrentItem() );

    while( currentNode && currentNode->m_Type != RC_TREE_NODE::MARKER )
        currentNode = currentNode->m_Parent;

    RC_TREE_NODE* nextMarker = nullptr;
    bool          trigger = currentNode == nullptr;

    for( RC_TREE_NODE* candidate : m_tree )
    {
        if( candidate == currentNode )
        {
            trigger = true;
        }
        else if( trigger )
        {
            nextMarker = candidate;
            break;
        }
    }

    if( nextMarker )
        m_view->Select( ToItem( nextMarker ) );
}


void RC_TREE_MODEL::onSizeView( wxSizeEvent& aEvent )
{
    int width = m_view->GetMainWindow()->GetRect().GetWidth() - WX_DATAVIEW_WINDOW_PADDING;

    if( m_view->GetColumnCount() > 0 )
        m_view->GetColumn( 0 )->SetWidth( width );

    // Pass size event to other widgets
    aEvent.Skip();
}
