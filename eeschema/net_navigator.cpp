/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Rivos
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * @author Wayne Stambaugh <stambaughw@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <wx/log.h>
#include <wx/srchctrl.h>
#include <wx/wupdlock.h>
#include <core/profile.h>
#include <tool/tool_manager.h>
#include <kiface_base.h>
#include <confirm.h>
#include <sch_edit_frame.h>
#include <sch_bus_entry.h>
#include <sch_line.h>
#include <sch_junction.h>
#include <sch_no_connect.h>
#include <sch_sheet_pin.h>
#include <string_utils.h>
#include <trace_helpers.h>
#include <advanced_config.h>
#include <connectivity/conn_navigation.h>
#include <connectivity/conn_facade.h>
#include <widgets/wx_aui_utils.h>
#include <tools/sch_actions.h>
#include <mail_type.h>
#include <wx/filename.h>
#include <wildcards_and_files_ext.h>
#include <wx/clntdata.h>
#include <regex>
#include <eeschema_settings.h>


static wxString GetNetNavigatorItemText( const SCH_ITEM* aItem,
                                         const SCH_SHEET_PATH& aSheetPath,
                                         UNITS_PROVIDER* aUnitsProvider )
{
    wxString retv;

    wxCHECK( aItem && aUnitsProvider, retv );

    switch( aItem->Type() )
    {
    case SCH_LINE_T:
    {
        const SCH_LINE* line = static_cast<const SCH_LINE*>( aItem );

        if( aItem->GetLayer() == LAYER_WIRE )
        {
            retv.Printf( _( "Wire from (%s, %s) to (%s, %s)" ),
                         aUnitsProvider->MessageTextFromValue( line->GetStartPoint().x ),
                         aUnitsProvider->MessageTextFromValue( line->GetStartPoint().y ),
                         aUnitsProvider->MessageTextFromValue( line->GetEndPoint().x ),
                         aUnitsProvider->MessageTextFromValue( line->GetEndPoint().y ) );
        }
        else if( aItem->GetLayer() == LAYER_BUS )
        {
            retv.Printf( _( "Bus from (%s, %s) to (%s, %s)" ),
                         aUnitsProvider->MessageTextFromValue( line->GetStartPoint().x ),
                         aUnitsProvider->MessageTextFromValue( line->GetStartPoint().y ),
                         aUnitsProvider->MessageTextFromValue( line->GetEndPoint().x ),
                         aUnitsProvider->MessageTextFromValue( line->GetEndPoint().y ) );
        }
        else
        {
            retv = _( "Graphic line not connectable" );
        }

        break;
    }
    case SCH_PIN_T:
    {
        const SCH_PIN* pin = static_cast<const SCH_PIN*>( aItem );

        if( const SYMBOL* symbol = pin->GetParentSymbol() )
        {
            retv.Printf( _( "Symbol '%s' pin '%s'" ),
                         symbol->GetRef( &aSheetPath, true ),
                         UnescapeString( pin->GetNumber() ) );

            if( wxString pinName = UnescapeString( pin->GetShownName() ); !pinName.IsEmpty() )
            {
                retv += wxString::Format( " (%s)", pinName );
            }
        }

        break;
    }
    case SCH_SHEET_PIN_T:
    {
        const SCH_SHEET_PIN* pin = static_cast<const SCH_SHEET_PIN*>( aItem );

        if( SCH_SHEET* sheet = pin->GetParent() )
        {
            retv.Printf( _( "Sheet '%s' pin '%s'" ),
                         sheet->GetName(),
                         UnescapeString( pin->GetText() ) );
        }

        break;
    }
    case SCH_LABEL_T:
    {
        const SCH_LABEL* label = static_cast<const SCH_LABEL*>( aItem );

        retv.Printf( _( "Label '%s' at (%s, %s)" ),
                     UnescapeString( label->GetText() ),
                     aUnitsProvider->MessageTextFromValue( label->GetPosition().x ),
                     aUnitsProvider->MessageTextFromValue( label->GetPosition().y ) );
        break;
    }
    case SCH_GLOBAL_LABEL_T:
    {
        const SCH_GLOBALLABEL* label = static_cast<const SCH_GLOBALLABEL*>( aItem );

        retv.Printf( _( "Global label '%s' at (%s, %s)" ),
                     UnescapeString( label->GetText() ),
                     aUnitsProvider->MessageTextFromValue( label->GetPosition().x ),
                     aUnitsProvider->MessageTextFromValue( label->GetPosition().y ) );
        break;
    }
    case SCH_HIER_LABEL_T:
    {
        const SCH_HIERLABEL* label = static_cast<const SCH_HIERLABEL*>( aItem );

        retv.Printf( _( "Hierarchical label '%s' at (%s, %s)" ),
                     UnescapeString( label->GetText() ),
                     aUnitsProvider->MessageTextFromValue( label->GetPosition().x ),
                     aUnitsProvider->MessageTextFromValue( label->GetPosition().y ) );
        break;
    }
    case SCH_JUNCTION_T:
    {
        const SCH_JUNCTION* junction = static_cast<const SCH_JUNCTION*>( aItem );

        retv.Printf( _( "Junction at (%s, %s)" ),
                     aUnitsProvider->MessageTextFromValue( junction->GetPosition().x ),
                     aUnitsProvider->MessageTextFromValue( junction->GetPosition().y ) );
        break;
    }
    case SCH_NO_CONNECT_T:
    {
        const SCH_NO_CONNECT* nc = static_cast<const SCH_NO_CONNECT*>( aItem );

        retv.Printf( _( "No-Connect at (%s, %s)" ),
                     aUnitsProvider->MessageTextFromValue( nc->GetPosition().x ),
                     aUnitsProvider->MessageTextFromValue( nc->GetPosition().y ) );
        break;
    }
    case SCH_BUS_WIRE_ENTRY_T:
    {
        const SCH_BUS_WIRE_ENTRY* entry = static_cast<const SCH_BUS_WIRE_ENTRY*>( aItem );

        retv.Printf( _( "Bus to wire entry from (%s, %s) to (%s, %s)" ),
                     aUnitsProvider->MessageTextFromValue( entry->GetPosition().x ),
                     aUnitsProvider->MessageTextFromValue( entry->GetPosition().y ),
                     aUnitsProvider->MessageTextFromValue( entry->GetEnd().x ),
                     aUnitsProvider->MessageTextFromValue( entry->GetEnd().y ) );
        break;
    }
    case SCH_BUS_BUS_ENTRY_T:
    {
        const SCH_BUS_BUS_ENTRY* entry = static_cast<const SCH_BUS_BUS_ENTRY*>( aItem );

        retv.Printf( _( "Bus to bus entry from (%s, %s) to (%s, %s)" ),
                     aUnitsProvider->MessageTextFromValue( entry->GetPosition().x ),
                     aUnitsProvider->MessageTextFromValue( entry->GetPosition().y ),
                     aUnitsProvider->MessageTextFromValue( entry->GetEnd().x ),
                     aUnitsProvider->MessageTextFromValue( entry->GetEnd().y ) );
        break;
    }
    case SCH_DIRECTIVE_LABEL_T:
    {
        const SCH_DIRECTIVE_LABEL* entry = static_cast<const SCH_DIRECTIVE_LABEL*>( aItem );

        retv.Printf( _( "Netclass label '%s' at (%s, %s)" ),
                     UnescapeString( entry->GetText() ),
                     aUnitsProvider->MessageTextFromValue( entry->GetPosition().x ),
                     aUnitsProvider->MessageTextFromValue( entry->GetPosition().y ) );
        break;
    }
    default:
        retv.Printf( _( "Unhandled item type %d" ), aItem->Type() );
    }

    return retv;
}


void SCH_EDIT_FRAME::MakeNetNavigatorNode( const wxString& aNetName, wxTreeItemId aParentId,
                                           const NET_NAVIGATOR_ITEM_DATA* aSelection,
                                           const SCH_CONNECTIVITY::NAVIGATION_QUERY& aQuery )
{
    wxCHECK( !aNetName.IsEmpty(), /* void */ );
    wxCHECK( m_schematic, /* void */ );
    wxCHECK( m_netNavigator, /* void */ );

    const auto itemsBySheet = aQuery.NetItems( aNetName, true );

    for( const auto& [sheetPath, items] : itemsBySheet )
    {
        wxCHECK2( sheetPath.Last(), continue );
        auto* itemData = new NET_NAVIGATOR_ITEM_DATA( sheetPath, nullptr );

        // Build path string for net navigator - include top-level sheet name to distinguish
        // multiple top-level sheets
        wxString txt;

        if( !sheetPath.empty() && sheetPath.at( 0 )->GetScreen() )
        {
            // Get the top-level sheet name
            txt = sheetPath.at( 0 )->GetField( FIELD_T::SHEET_NAME )->GetShownText( FOR_GUI );

            if( txt.IsEmpty() )
            {
                wxFileName fn( sheetPath.at( 0 )->GetScreen()->GetFileName() );
                txt = fn.GetName();
            }

            // Add sub-sheet names
            for( unsigned i = 1; i < sheetPath.size(); i++ )
                txt << wxS( "/" ) << sheetPath.at( i )->GetField( FIELD_T::SHEET_NAME )->GetShownText( FOR_GUI );
        }
        else
        {
            txt = sheetPath.PathHumanReadable( true, true );
        }

        wxTreeItemId sheetId = m_netNavigator->AppendItem( aParentId, txt, -1, -1, itemData );

        if( aSelection && *aSelection == *itemData )
            m_netNavigator->SelectItem( sheetId );

        for( const SCH_ITEM* item : items )
        {
            if( item->Type() == SCH_LINE_T
                    || item->Type() == SCH_JUNCTION_T
                    || item->Type() == SCH_BUS_WIRE_ENTRY_T
                    || item->Type() == SCH_BUS_BUS_ENTRY_T )
            {
                continue;
            }

            itemData = new NET_NAVIGATOR_ITEM_DATA( sheetPath, item );
            wxTreeItemId id = m_netNavigator->AppendItem( sheetId,
                                                          GetNetNavigatorItemText( item, sheetPath, this ),
                                                          -1, -1, itemData );

            if( aSelection && *aSelection == *itemData )
            {
                m_netNavigator->EnsureVisible( id );
                m_netNavigator->SelectItem( id );
            }
        }

        m_netNavigator->SortChildren( sheetId );
    }

    // Sort the items in the tree control alphabetically
    m_netNavigator->SortChildren( aParentId );
    m_netNavigator->Expand( aParentId );
}


void SCH_EDIT_FRAME::RefreshNetNavigator( const NET_NAVIGATOR_ITEM_DATA* aSelection,
                                          const std::vector<wxString>* aChangedNets )
{
    wxCHECK( m_netNavigator && m_schematic, /* void */ );

    if( !m_netNavigator->IsShownOnScreen() || !m_schematic->HasHierarchy() )
    {
        m_netNavigatorStale = true;
        return;
    }

    const bool incremental = aChangedNets && !m_netNavigatorStale && !m_netNavigator->IsEmpty()
                             && m_netNavigatorConnection == m_highlightedConn;

    if( !aSelection && !m_netNavigator->IsEmpty()
        && ( incremental || ( !m_highlightedConn.IsEmpty()
                              && m_netNavigatorConnection == m_highlightedConn ) ) )
    {
        const wxTreeItemId selected = m_netNavigator->GetSelection();

        if( selected.IsOk() )
            aSelection = dynamic_cast<NET_NAVIGATOR_ITEM_DATA*>( m_netNavigator->GetItemData( selected ) );
    }

    // Selection data can belong to the tree that is about to be deleted
    NET_NAVIGATOR_ITEM_DATA selection;

    if( aSelection )
    {
        selection = *aSelection;
        aSelection = &selection;
    }

    if( m_netNavigatorFilter )
        m_netNavigatorFilter->Enable( m_highlightedConn.IsEmpty() );

    const SCH_CONNECTIVITY::NAVIGATION_QUERY query( *m_schematic );
    size_t nodeCnt = 0;

    wxWindowUpdateLocker updateLock( m_netNavigator );
    PROF_TIMER           timer;

    if( !incremental )
    {
        m_netNavigatorNodes.clear();
        m_netNavigator->DeleteAllItems();
        m_netNavigatorConnection = m_highlightedConn;
        m_netNavigatorStale = false;
    }

    const auto rebuildNode = [&]( const wxString& name, wxTreeItemId node )
    {
        wxTreeItemId selected = m_netNavigator->GetSelection();

        while( selected.IsOk() && selected != node )
            selected = m_netNavigator->GetItemParent( selected );

        const bool restoreSelection = selected == node;
        const bool expanded = m_netNavigator->IsExpanded( node );
        std::set<KIID_PATH> expandedSheets;
        wxTreeItemIdValue cookie;

        for( wxTreeItemId sheet = m_netNavigator->GetFirstChild( node, cookie ); sheet.IsOk();
             sheet = m_netNavigator->GetNextChild( node, cookie ) )
        {
            if( m_netNavigator->IsExpanded( sheet ) )
            {
                if( auto* data = dynamic_cast<NET_NAVIGATOR_ITEM_DATA*>( m_netNavigator->GetItemData( sheet ) ) )
                    expandedSheets.insert( data->GetSheetPath().PathRef() );
            }
        }

        m_netNavigator->DeleteChildren( node );
        MakeNetNavigatorNode( name, node, restoreSelection ? aSelection : nullptr, query );

        for( wxTreeItemId sheet = m_netNavigator->GetFirstChild( node, cookie ); sheet.IsOk();
             sheet = m_netNavigator->GetNextChild( node, cookie ) )
        {
            auto* data = dynamic_cast<NET_NAVIGATOR_ITEM_DATA*>( m_netNavigator->GetItemData( sheet ) );

            if( data && expandedSheets.contains( data->GetSheetPath().PathRef() ) )
                m_netNavigator->Expand( sheet );
        }

        if( !expanded )
            m_netNavigator->Collapse( node );
    };

    wxString filter = m_highlightedConn.IsEmpty() ? m_netNavigatorFilterValue : wxString();

    // Determine search mode from settings
    EESCHEMA_SETTINGS* cfg = eeconfig();
    bool useWildcard = cfg ? cfg->m_AuiPanels.net_nav_search_mode_wildcard : true;

    // For wildcard mode, wrap filter with wildcards for substring matching by default
    // unless user has already specified wildcards
    wxString globFilter;
    std::unique_ptr<std::regex> regexFilter;

    if( !filter.IsEmpty() )
    {
        if( useWildcard )
        {
            globFilter = filter;
            if( !globFilter.Contains( wxT( "*" ) ) && !globFilter.Contains( wxT( "?" ) ) )
                globFilter = wxT( "*" ) + globFilter + wxT( "*" );
        }
        else
        {
            // Regex mode - compile the regex pattern
            try
            {
                regexFilter = std::make_unique<std::regex>(
                        filter.ToStdString(),
                        std::regex_constants::icase | std::regex_constants::ECMAScript );
            }
            catch( const std::regex_error& )
            {
                // Invalid regex - no filtering
                regexFilter.reset();
            }
        }
    }

    if( m_highlightedConn.IsEmpty() )
    {
        // Create a tree of all nets in the schematic.
        wxTreeItemId rootId = incremental ? m_netNavigator->GetRootItem()
                                         : m_netNavigator->AddRoot( _( "Nets" ), 0 );
        const auto names = incremental ? *aChangedNets : query.NetNames();
        // Legacy names arrive sorted by escaped name, engine names in publication order
        bool sortRoot = !incremental && ADVANCED_CFG::GetCfg().m_ConnectivityEngine;

        for( const wxString& netName : names )
        {
            if( netName.IsEmpty() )
                continue;

            auto existing = m_netNavigatorNodes.find( netName );

            if( incremental && !m_schematic->Connectivity().NetByName( netName ) )
            {
                if( existing != m_netNavigatorNodes.end() )
                {
                    m_netNavigator->Delete( existing->second );
                    m_netNavigatorNodes.erase( existing );
                }

                continue;
            }

            wxString displayName = UnescapeString( netName );

            // Apply filter based on mode
            if( !filter.IsEmpty() )
            {
                bool matches = false;

                if( useWildcard && !globFilter.IsEmpty() )
                {
                    // Use glob-based matching (supports * and ? wildcards), case-insensitive
                    matches = WildCompareString( globFilter, displayName, false );
                }
                else if( !useWildcard && regexFilter )
                {
                    // Use regex matching
                    try
                    {
                        matches = std::regex_search( displayName.ToStdString(), *regexFilter );
                    }
                    catch( const std::regex_error& )
                    {
                        matches = false;
                    }
                }

                if( !matches )
                    continue;
            }

            nodeCnt++;

            if( existing != m_netNavigatorNodes.end() )
            {
                rebuildNode( netName, existing->second );
            }
            else
            {
                wxTreeItemId netId = m_netNavigator->AppendItem( rootId, displayName, -1, -1 );
                m_netNavigatorNodes.emplace( netName, netId );
                MakeNetNavigatorNode( netName, netId, incremental ? nullptr : aSelection, query );
                sortRoot |= incremental;
            }
        }

        if( sortRoot )
            m_netNavigator->SortChildren( rootId );

        if( !incremental )
            m_netNavigator->Expand( rootId );
    }
    else
    {
        if( !incremental )
        {
            wxTreeItemId rootId = m_netNavigator->AddRoot( UnescapeString( m_highlightedConn ) );
            m_netNavigatorNodes.emplace( m_highlightedConn, rootId );
            MakeNetNavigatorNode( m_highlightedConn, rootId, aSelection, query );
            nodeCnt++;
        }
        else if( std::ranges::find( *aChangedNets, m_highlightedConn ) != aChangedNets->end() )
        {
            rebuildNode( m_highlightedConn, m_netNavigator->GetRootItem() );
            nodeCnt++;
        }
    }

    timer.Stop();

    wxLogTrace( traceUiProfile, wxS( "Adding %zu nodes to net navigator took %s." ),
                nodeCnt, timer.to_string() );
}


const SCH_ITEM* SCH_EDIT_FRAME::SelectNextPrevNetNavigatorItem( bool aNext )
{
    wxCHECK( m_netNavigator, nullptr );

    wxTreeItemId id = m_netNavigator->GetSelection();

    if( !id.IsOk() )
        return nullptr;

    wxTreeItemId nextId;
    wxTreeItemId netNode = m_netNavigator->GetRootItem();

    std::vector<wxTreeItemId> netItems;
    std::list<wxTreeItemId> itemList;
    itemList.push_back( netNode );

    while( !itemList.empty() )
    {
        wxTreeItemId current = itemList.front();
        itemList.pop_front();

        wxTreeItemIdValue cookie;
        wxTreeItemId      child = m_netNavigator->GetFirstChild( current, cookie );

        while( child.IsOk() )
        {
            if( m_netNavigator->ItemHasChildren( child ) )
                itemList.push_back( child );
            else
                netItems.push_back( child );

            child = m_netNavigator->GetNextSibling( child );
        }
    }

    // Locate current item and move forward or backward with wrap
    auto it = std::find( netItems.begin(), netItems.end(), id );

    if( it != netItems.end() )
    {
        if( aNext )
        {
            ++it;
            if( it == netItems.end() )
                it = netItems.begin();
        }
        else
        {
            if( it == netItems.begin() )
                it = netItems.end();
            --it;
        }
        nextId = *it;
    }

    if( nextId.IsOk() )
    {
        if( !m_netNavigator->IsVisible( nextId ) )
        {
            m_netNavigator->CollapseAll();
            m_netNavigator->EnsureVisible( nextId );
        }

        m_netNavigator->UnselectAll();
        m_netNavigator->SelectItem( nextId );

        auto* data = static_cast<NET_NAVIGATOR_ITEM_DATA*>( m_netNavigator->GetItemData( nextId ) );

        if( data && data->GetItem() )
            return data->GetItem();
    }

    return nullptr;
}


void SCH_EDIT_FRAME::SelectNetNavigatorItem( const NET_NAVIGATOR_ITEM_DATA* aSelection )
{
    wxCHECK( m_netNavigator, /* void */ );
    wxCHECK( !m_netNavigator->IsFrozen(), /* void */ );

    // Maybe in the future we can do something like collapse the tree for an empty selection.
    // For now, leave the tree selection in its current state.
    if( !aSelection )
        return;

    wxTreeItemId rootId = m_netNavigator->GetRootItem();

    if( !rootId.IsOk() )
        return;

    wxTreeItemIdValue        sheetCookie;
    NET_NAVIGATOR_ITEM_DATA* itemData = nullptr;
    wxTreeItemId             sheetId = m_netNavigator->GetFirstChild( rootId, sheetCookie );

    while( sheetId.IsOk() )
    {
        if( m_netNavigator->ItemHasChildren( sheetId ) )
        {
            wxTreeItemIdValue itemCookie;
            wxTreeItemId itemId = m_netNavigator->GetFirstChild( sheetId, itemCookie );

            while( itemId.IsOk() )
            {
                itemData = dynamic_cast<NET_NAVIGATOR_ITEM_DATA*>( m_netNavigator->GetItemData( itemId ) );

                wxCHECK2( itemData, continue );

                if( *itemData == *aSelection )
                {
                    if( !m_netNavigator->IsVisible( itemId ) )
                    {
                        m_netNavigator->CollapseAll();
                        m_netNavigator->EnsureVisible( itemId );
                    }

                    m_netNavigator->SelectItem( itemId );
                    return;
                }

                itemId = m_netNavigator->GetNextSibling( itemId );
            }

            sheetId = m_netNavigator->GetNextSibling( sheetId );
        }
    }
}


const SCH_ITEM* SCH_EDIT_FRAME::GetSelectedNetNavigatorItem() const
{
    if( !m_netNavigator || m_netNavigator->IsFrozen() )
        return nullptr;

    wxTreeItemId id = m_netNavigator->GetSelection();

    if( !id.IsOk() || ( id == m_netNavigator->GetRootItem() ) )
        return nullptr;

    auto* itemData = dynamic_cast<NET_NAVIGATOR_ITEM_DATA*>( m_netNavigator->GetItemData( id ) );

    wxCHECK( itemData, nullptr );

    return itemData->GetItem();
}


void SCH_EDIT_FRAME::onNetNavigatorSelection( wxTreeEvent& aEvent )
{
    if( !m_netNavigator || m_netNavigator->IsFrozen() )
        return;

    wxTreeItemId id = aEvent.GetItem();

    // Clicking on the root item (net name ) does nothing.
    if( id == m_netNavigator->GetRootItem() )
        return;

    auto* itemData = dynamic_cast<NET_NAVIGATOR_ITEM_DATA*>( m_netNavigator->GetItemData( id ) );

    // Just a net name when we have all nets displayed.
    if( !itemData )
        return;

    if( GetCurrentSheet() != itemData->GetSheetPath() )
        GetToolManager()->RunAction<SCH_SHEET_PATH*>( SCH_ACTIONS::changeSheet, &itemData->GetSheetPath() );

    // Do not focus on item when a sheet tree node is selected.
    if( m_netNavigator->GetItemParent( id ) != m_netNavigator->GetRootItem() && itemData->GetItem() )
    {
        // Make sure we didn't remove the item and/or the screen it resides on before we access it.
        const SCH_ITEM* item = itemData->GetItem();

        // Don't search for child items in screen r-tree.
        item = ( item->Type() == SCH_SHEET_PIN_T || item->Type() == SCH_PIN_T )
                                                            ? static_cast<const SCH_ITEM*>( item->GetParent() )
                                                            : item;

        const SCH_SCREEN* screen = itemData->GetSheetPath().LastScreen();

        wxCHECK( screen, /* void */ );
        wxCHECK( screen->Items().contains( item, true ), /* void */ );

        FocusOnLocation( itemData->GetItem()->GetBoundingBox().Centre() );
    }

    GetCanvas()->Refresh();
}


void SCH_EDIT_FRAME::onNetNavigatorSelChanging( wxTreeEvent& aEvent )
{
    if( !m_netNavigator || m_netNavigator->IsFrozen() )
        return;

    aEvent.Skip();
}


void SCH_EDIT_FRAME::ToggleNetNavigator()
{
    EESCHEMA_SETTINGS* cfg = eeconfig();

    wxCHECK( cfg, /* void */ );

    wxAuiPaneInfo& netNavigatorPane = m_auimgr.GetPane( NetNavigatorPaneName() );

    netNavigatorPane.Show( !netNavigatorPane.IsShown() );
    updateSelectionFilterVisbility();

    cfg->m_AuiPanels.show_net_nav_panel = netNavigatorPane.IsShown();

    if( netNavigatorPane.IsShown() )
    {
        if( netNavigatorPane.IsFloating() )
        {
            netNavigatorPane.FloatingSize( cfg->m_AuiPanels.net_nav_panel_float_size );
            m_auimgr.Update();
        }
        else if( cfg->m_AuiPanels.net_nav_panel_docked_size.GetWidth() > 0 )
        {
            // SetAuiPaneSize also updates m_auimgr
            SetAuiPaneSize( m_auimgr, netNavigatorPane,
                            cfg->m_AuiPanels.net_nav_panel_docked_size.GetWidth(), -1 );
        }
    }
    else
    {
        if( netNavigatorPane.IsFloating() )
        {
            cfg->m_AuiPanels.net_nav_panel_float_size  = netNavigatorPane.floating_size;
        }
        else
        {
            cfg->m_AuiPanels.net_nav_panel_docked_size = m_netNavigator->GetSize();
        }

        m_auimgr.Update();
    }

    if( netNavigatorPane.IsShown() )
    {
        NET_NAVIGATOR_ITEM_DATA* itemData = nullptr;

        wxTreeItemId selection = m_netNavigator->GetSelection();

        if( selection.IsOk() )
            itemData = dynamic_cast<NET_NAVIGATOR_ITEM_DATA*>( m_netNavigator->GetItemData( selection ) );

        RefreshNetNavigator( itemData );
    }
}


void SCH_EDIT_FRAME::FindNetInInspector( const wxString& aNetName )
{
    if( !m_netNavigator || aNetName.IsEmpty() )
        return;

    // Ensure the net navigator is shown
    wxAuiPaneInfo& netNavigatorPane = m_auimgr.GetPane( NetNavigatorPaneName() );

    if( !netNavigatorPane.IsShown() )
        ToggleNetNavigator();

    // Clear any net highlights
    SetHighlightedConnection( wxEmptyString );
    GetToolManager()->RunAction( SCH_ACTIONS::updateNetHighlighting );

    // Set the search text to the aNetName
    if( m_netNavigatorFilter )
        m_netNavigatorFilter->SetValue( aNetName );

    m_netNavigatorFilterValue = aNetName;

    // Refresh the tree
    RefreshNetNavigator();
}


void SCH_EDIT_FRAME::onResizeNetNavigator( wxSizeEvent& aEvent )
{
    aEvent.Skip();

    // Called when resizing the Hierarchy Navigator panel
    // Store the current pane size
    // It allows to retrieve the last defined pane size when switching between
    // docked and floating pane state
    // Note: *DO NOT* call m_auimgr.Update() here: it crashes KiCad at least on Windows

    EESCHEMA_SETTINGS* cfg = dynamic_cast<EESCHEMA_SETTINGS*>( Kiface().KifaceSettings() );

    // During deletion/cleanup operations, settings may be temporarily unavailable
    if( !cfg )
        return;

    wxAuiPaneInfo& netNavigatorPane = m_auimgr.GetPane( NetNavigatorPaneName() );

    if( m_netNavigator->IsShownOnScreen() )
    {
        cfg->m_AuiPanels.net_nav_panel_float_size  = netNavigatorPane.floating_size;

        // initialize net_nav_panel_docked_width and best size only if the netNavigatorPane
        // width is > 0 (i.e. if its size is already set and has meaning)
        // if it is floating, its size is not initialized (only floating_size is initialized)
        // initializing netNavigatorPane.best_size is useful when switching to float pane and
        // after switching to the docked pane, to retrieve the last docked pane width
        if( netNavigatorPane.rect.width > 50 )    // 50 is a good margin
        {
            cfg->m_AuiPanels.net_nav_panel_docked_size.SetWidth( netNavigatorPane.rect.width );
            netNavigatorPane.best_size.x = netNavigatorPane.rect.width;
        }
    }
}
