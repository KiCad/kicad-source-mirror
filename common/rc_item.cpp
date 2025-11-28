/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
#include <rc_json_schema.h>
#include <eda_item.h>
#include <base_units.h>

#define WX_DATAVIEW_WINDOW_PADDING 6


wxString RC_ITEM::GetErrorMessage( bool aTranslate ) const
{
    if( m_errorMessage.IsEmpty() )
        return GetErrorText( aTranslate );
    else
        return m_errorMessage;
}


static wxString showCoord( UNITS_PROVIDER* aUnitsProvider, const VECTOR2I& aPos )
{
    return wxString::Format( wxT( "@(%s, %s)" ),
                             aUnitsProvider->MessageTextFromValue( aPos.x ),
                             aUnitsProvider->MessageTextFromValue( aPos.y ) );
}


void RC_ITEM::AddItem( EDA_ITEM* aItem )
{
    m_ids.push_back( aItem->m_Uuid );
}


void RC_ITEM::SetItems( const EDA_ITEM* aItem, const EDA_ITEM* bItem,
                        const EDA_ITEM* cItem, const EDA_ITEM* dItem )
{
    m_ids.clear();

    if( aItem )
        m_ids.push_back( aItem->m_Uuid );

    if( bItem )
        m_ids.push_back( bItem->m_Uuid );

    if( cItem )
        m_ids.push_back( cItem->m_Uuid );

    if( dItem )
        m_ids.push_back( dItem->m_Uuid );
}


wxString RC_ITEM::getSeverityString( SEVERITY aSeverity )
{
    wxString severity;

    switch( aSeverity )
    {
    case RPT_SEVERITY_ERROR:     severity = wxS( "error" );     break;
    case RPT_SEVERITY_WARNING:   severity = wxS( "warning" );   break;
    case RPT_SEVERITY_ACTION:    severity = wxS( "action" );    break;
    case RPT_SEVERITY_INFO:      severity = wxS( "info" );      break;
    case RPT_SEVERITY_EXCLUSION: severity = wxS( "exclusion" ); break;
    case RPT_SEVERITY_DEBUG:     severity = wxS( "debug" );     break;
    default:;
    };

    return severity;
}


wxString RC_ITEM::ShowReport( UNITS_PROVIDER* aUnitsProvider, SEVERITY aSeverity,
                              const std::map<KIID, EDA_ITEM*>& aItemMap ) const
{
    wxString severity = getSeverityString( aSeverity );

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
    // 3) report settings key (which should be more stable) in addition to message

    wxString msg;

    if( mainItem && auxItem )
    {
        msg.Printf( wxT( "[%s]: %s\n    %s; %s\n    %s: %s\n    %s: %s\n" ),
                    GetSettingsKey(),
                    GetErrorMessage( false ),
                    GetViolatingRuleDesc( false ),
                    severity,
                    showCoord( aUnitsProvider, mainItem->GetPosition()),
                    mainItem->GetItemDescription( aUnitsProvider, true ),
                    showCoord( aUnitsProvider, auxItem->GetPosition()),
                    auxItem->GetItemDescription( aUnitsProvider, true ) );
    }
    else if( mainItem )
    {
        msg.Printf( wxT( "[%s]: %s\n    %s; %s\n    %s: %s\n" ),
                    GetSettingsKey(),
                    GetErrorMessage( false ),
                    GetViolatingRuleDesc( false ),
                    severity,
                    showCoord( aUnitsProvider, mainItem->GetPosition()),
                    mainItem->GetItemDescription( aUnitsProvider, true ) );
    }
    else
    {
        msg.Printf( wxT( "[%s]: %s\n    %s; %s\n" ),
                    GetSettingsKey(),
                    GetErrorMessage( false ),
                    GetViolatingRuleDesc( false ),
                    severity );
    }

    if( m_parent && m_parent->IsExcluded() && !m_parent->GetComment().IsEmpty() )
        msg += wxString::Format( wxS( "    %s\n" ), m_parent->GetComment() );

    return msg;
}


void RC_ITEM::GetJsonViolation( RC_JSON::VIOLATION& aViolation, UNITS_PROVIDER* aUnitsProvider,
                                SEVERITY aSeverity, const std::map<KIID, EDA_ITEM*>& aItemMap ) const
{
    aViolation.severity = getSeverityString( aSeverity );
    aViolation.description = GetErrorMessage( false );
    aViolation.type = GetSettingsKey();

    if( m_parent && m_parent->IsExcluded() )
    {
        aViolation.excluded = true;
        aViolation.comment = m_parent->GetComment();
    }
    else
    {
        aViolation.excluded = false;
    }

    EDA_ITEM* mainItem = nullptr;
    EDA_ITEM* auxItem = nullptr;

    auto ii = aItemMap.find( GetMainItemID() );

    if( ii != aItemMap.end() )
        mainItem = ii->second;

    ii = aItemMap.find( GetAuxItemID() );

    if( ii != aItemMap.end() )
        auxItem = ii->second;

    if( mainItem )
    {
        RC_JSON::AFFECTED_ITEM item;
        item.description = mainItem->GetItemDescription( aUnitsProvider, true );
        item.uuid = mainItem->m_Uuid.AsString();
        item.pos.x = EDA_UNIT_UTILS::UI::ToUserUnit( aUnitsProvider->GetIuScale(),
                                                     aUnitsProvider->GetUserUnits(),
                                                     mainItem->GetPosition().x );
        item.pos.y = EDA_UNIT_UTILS::UI::ToUserUnit( aUnitsProvider->GetIuScale(),
                                                     aUnitsProvider->GetUserUnits(),
                                                     mainItem->GetPosition().y );
        aViolation.items.emplace_back( item );
    }

    if( auxItem )
    {
        RC_JSON::AFFECTED_ITEM item;
        item.description = auxItem->GetItemDescription( aUnitsProvider, true );
        item.uuid = auxItem->m_Uuid.AsString();
        item.pos.x = EDA_UNIT_UTILS::UI::ToUserUnit( aUnitsProvider->GetIuScale(),
                                                     aUnitsProvider->GetUserUnits(),
                                                     auxItem->GetPosition().x );
        item.pos.y = EDA_UNIT_UTILS::UI::ToUserUnit( aUnitsProvider->GetIuScale(),
                                                     aUnitsProvider->GetUserUnits(),
                                                     auxItem->GetPosition().y );
        aViolation.items.emplace_back( item );
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
        case RC_TREE_NODE::COMMENT:
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
}


RC_TREE_MODEL::~RC_TREE_MODEL()
{
    for( RC_TREE_NODE* topLevelNode : m_tree )
        delete topLevelNode;
}


void RC_TREE_MODEL::rebuildModel( std::shared_ptr<RC_ITEMS_PROVIDER> aProvider, int aSeverities )
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

    BeforeReset();

    m_rcItemsProvider = std::move( aProvider );

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

        if( MARKER_BASE* marker = rcItem->GetParent() )
        {
            if( marker->IsExcluded() && !marker->GetComment().IsEmpty() )
                n->m_Children.push_back( new RC_TREE_NODE( n, rcItem, RC_TREE_NODE::COMMENT ) );
        }
    }

    // Must be called after a significant change of items to force the
    // wxDataViewModel to reread all of them, repopulating itself entirely.
    AfterReset();

#ifdef __WXGTK__
    // The fastest method to update wxDataViewCtrl is to rebuild from
    // scratch by calling Cleared(). Linux requires to reassociate model to
    // display data, but Windows will create multiple associations.
    // On MacOS, this crashes KiCad. See https://gitlab.com/kicad/code/kicad/-/issues/3666
    // and https://gitlab.com/kicad/code/kicad/-/issues/3653
    m_view->AssociateModel( this );
#endif

    m_view->ClearColumns();
    m_view->AppendTextColumn( wxEmptyString, 0, wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE );

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


void RC_TREE_MODEL::Update( std::shared_ptr<RC_ITEMS_PROVIDER> aProvider, int aSeverities )
{
    rebuildModel( aProvider, aSeverities );
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


void RC_TREE_MODEL::GetValue( wxVariant& aVariant, wxDataViewItem const& aItem,
                              unsigned int aCol ) const
{
    if( !aItem.IsOk() || m_view->IsFrozen() )
        return;

    const RC_TREE_NODE*            node = ToNode( aItem );
    const std::shared_ptr<RC_ITEM> rcItem = node->m_RcItem;
    MARKER_BASE*                   marker = rcItem->GetParent();
    EDA_ITEM*                      item = nullptr;
    wxString                       msg;

    switch( node->m_Type )
    {
    case RC_TREE_NODE::MARKER:
        if( marker )
        {
            SEVERITY severity = marker->GetSeverity();

            if( severity == RPT_SEVERITY_EXCLUSION )
            {
                if( m_editFrame->GetSeverity( rcItem->GetErrorCode() ) == RPT_SEVERITY_WARNING )
                    msg = _( "Excluded warning: " );
                else
                    msg = _( "Excluded error: " );
            }
            else if( severity == RPT_SEVERITY_WARNING )
            {
                msg = _( "Warning: " );
            }
            else
            {
                msg = _( "Error: " );
            }
        }

        msg += rcItem->GetErrorMessage( true );
        break;

    case RC_TREE_NODE::MAIN_ITEM:
        if( marker && marker->GetMarkerType() == MARKER_BASE::MARKER_DRAWING_SHEET )
            msg = _( "Drawing Sheet" );
        else
            item = m_editFrame->ResolveItem( rcItem->GetMainItemID() );

        break;

    case RC_TREE_NODE::AUX_ITEM:
        item = m_editFrame->ResolveItem( rcItem->GetAuxItemID() );
        break;

    case RC_TREE_NODE::AUX_ITEM2:
        item = m_editFrame->ResolveItem( rcItem->GetAuxItem2ID() );
        break;

    case RC_TREE_NODE::AUX_ITEM3:
        item = m_editFrame->ResolveItem( rcItem->GetAuxItem3ID() );
        break;

    case RC_TREE_NODE::COMMENT:
        if( marker )
            msg = marker->GetComment();

        break;
    }

    if( item )
        msg += item->GetItemDescription( m_editFrame, true );

    msg.Replace( wxS( "\n" ), wxS( " " ) );
    aVariant = msg;
}


bool RC_TREE_MODEL::GetAttr( wxDataViewItem const&   aItem,
                             unsigned int            aCol,
                             wxDataViewItemAttr&     aAttr ) const
{
    if( !aItem.IsOk() || m_view->IsFrozen() )
        return false;

    const RC_TREE_NODE* node = ToNode( aItem );

    bool ret = false;
    bool heading = node->m_Type == RC_TREE_NODE::MARKER;

    if( heading )
    {
        aAttr.SetBold( true );
        ret = true;
    }

    if( node->m_RcItem->GetParent()
            && node->m_RcItem->GetParent()->GetSeverity() == RPT_SEVERITY_EXCLUSION )
    {
        wxColour textColour = wxSystemSettings::GetColour( wxSYS_COLOUR_LISTBOXTEXT );
        double   brightness = KIGFX::COLOR4D( textColour ).GetBrightness();

        if( brightness > 0.5 )
        {
            int lightness = static_cast<int>( brightness * ( heading ? 50 : 60 ) );
            aAttr.SetColour( textColour.ChangeLightness( lightness ) );
        }
        else
        {
            aAttr.SetColour( textColour.ChangeLightness( heading ? 170 : 165 ) );
        }

        aAttr.SetItalic( true );   // Strikethrough would be better, if wxWidgets supported it
        ret = true;
    }

    return ret;
}


void RC_TREE_MODEL::ValueChanged( RC_TREE_NODE* aNode )
{
    if( aNode->m_Type != RC_TREE_NODE::MARKER )
    {
        ValueChanged( aNode->m_Parent );
        return;
    }

    wxDataViewItem markerItem = ToItem( aNode );

    wxDataViewModel::ValueChanged( markerItem, 0 );

    for( const RC_TREE_NODE* child : aNode->m_Children )
        wxDataViewModel::ValueChanged( ToItem( child ), 0 );

    // Comment items can come and go depening on exclusion state and comment content.
    //
    const std::shared_ptr<RC_ITEM> rcItem = aNode->m_RcItem;
    MARKER_BASE*                   marker = rcItem ? rcItem->GetParent() : nullptr;

    if( marker )
    {
        bool          needsCommentNode = marker->IsExcluded() && !marker->GetComment().IsEmpty();
        RC_TREE_NODE* commentNode = aNode->m_Children.back();

        if( commentNode && commentNode->m_Type != RC_TREE_NODE::COMMENT )
            commentNode = nullptr;

        if( needsCommentNode && !commentNode )
        {
            commentNode = new RC_TREE_NODE( aNode, rcItem, RC_TREE_NODE::COMMENT );
            wxDataViewItemArray newItems;
            newItems.push_back( ToItem( commentNode ) );

            aNode->m_Children.push_back( commentNode );
            ItemsAdded( markerItem, newItems );
        }
        else if( commentNode && !needsCommentNode )
        {
            wxDataViewItemArray deletedItems;
            deletedItems.push_back( ToItem( commentNode ) );

            aNode->m_Children.erase( aNode->m_Children.end() - 1 );
            ItemsDeleted( markerItem, deletedItems );
        }
    }
}


void RC_TREE_MODEL::DeleteCurrentItem( bool aDeep )
{
    DeleteItems( true, true, aDeep );
}


void RC_TREE_MODEL::DeleteItems( bool aCurrentOnly, bool aIncludeExclusions, bool aDeep )
{
    RC_TREE_NODE* current_node = m_view ? ToNode( m_view->GetCurrentItem() ) : nullptr;
    const std::shared_ptr<RC_ITEM> current_item = current_node ? current_node->m_RcItem : nullptr;

    /// Keep a vector of elements to free after wxWidgets is definitely done accessing them
    std::vector<RC_TREE_NODE*> to_delete;
    std::vector<RC_TREE_NODE*> expanded;

    if( aCurrentOnly && !current_item )
    {
        wxBell();
        return;
    }

    // wxWidgets 3.1.x on MacOS (at least) loses the expanded state of the tree when deleting
    // items.
    if( m_view && aCurrentOnly )
    {
        for( RC_TREE_NODE* node : m_tree )
        {
            if( m_view->IsExpanded( ToItem( node ) ) )
                expanded.push_back( node );
        }
    }

    int  lastGood = -1;
    bool itemDeleted = false;

    if( m_view )
    {
        m_view->UnselectAll();
        wxSafeYield();
        m_view->Freeze();
    }

    if( !m_rcItemsProvider )
        return;

    for( int i = m_rcItemsProvider->GetCount() - 1; i >= 0; --i )
    {
        std::shared_ptr<RC_ITEM> rcItem = m_rcItemsProvider->GetItem( i );
        MARKER_BASE*             marker = rcItem->GetParent();
        bool                     excluded = false;

        if( marker && marker->GetSeverity() == RPT_SEVERITY_EXCLUSION )
            excluded = true;

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

        // Only deep delete the current item here; others will be done by the caller, which
        // can more efficiently delete all markers on the board.
        m_rcItemsProvider->DeleteItem( i, aDeep && aCurrentOnly );

        if( lastGood > i )
            lastGood--;

        itemDeleted = true;
    }

    if( m_view && aCurrentOnly && lastGood >= 0 )
    {
        for( RC_TREE_NODE* node : expanded )
        {
            wxDataViewItem item = ToItem( node );

            if( item.IsOk() )
                m_view->Expand( item );
        }

        wxDataViewItem selItem = ToItem( m_tree[ lastGood ] );
        m_view->Select( selItem );

        // wxEVT_COMMAND_DATAVIEW_SELECTION_CHANGED doesn't get propogated from the Select()
        // call on (at least) MSW.
        wxDataViewEvent selectEvent( wxEVT_COMMAND_DATAVIEW_SELECTION_CHANGED, m_view, selItem );
        m_view->GetEventHandler()->ProcessEvent( selectEvent );
    }

    for( RC_TREE_NODE* item : to_delete )
        delete( item );

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


void RC_TREE_MODEL::SelectMarker( const MARKER_BASE* aMarker )
{
    wxCHECK( !m_view->IsFrozen(), /* void */ );

    for( RC_TREE_NODE* candidate : m_tree )
    {
        if( candidate->m_RcItem->GetParent() == aMarker )
        {
            m_view->Select( ToItem( candidate ) );
            return;
        }
    }
}


void RC_TREE_MODEL::CenterMarker( const MARKER_BASE* aMarker )
{
    wxCHECK( !m_view->IsFrozen(), /* void */ );

    for( RC_TREE_NODE* candidate : m_tree )
    {
        if( candidate->m_RcItem->GetParent() == aMarker )
        {
            m_view->EnsureVisible( ToItem( candidate ) );
            return;
        }
    }
}
