/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Rivos
 * Copyright (C) 2023 KiCad Developers, see AUTHORS.txt for contributors.
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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <wx/log.h>
#include <tool/tool_manager.h>
#include <kiface_base.h>
#include <sch_edit_frame.h>
#include <sch_bus_entry.h>
#include <sch_line.h>
#include <sch_junction.h>
#include <sch_sheet_pin.h>
#include <schematic.h>
#include <connection_graph.h>
#include <widgets/wx_aui_utils.h>
#include <tools/ee_actions.h>


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

        wxCHECK( line, retv );

        if( aItem->GetLayer() == LAYER_WIRE )
        {
            retv.Printf( _( "Wire from %s, %s to %s, %s" ),
                         aUnitsProvider->MessageTextFromValue( line->GetStartPoint().x ),
                         aUnitsProvider->MessageTextFromValue( line->GetStartPoint().y ),
                         aUnitsProvider->MessageTextFromValue( line->GetEndPoint().x ),
                         aUnitsProvider->MessageTextFromValue( line->GetEndPoint().y ) );
        }
        else if( aItem->GetLayer() == LAYER_BUS )
        {
            retv.Printf( _( "Bus from %s, %s to %s, %s" ),
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
        wxCHECK( pin, retv );

        SCH_SYMBOL* symbol = pin->GetParentSymbol();
        wxCHECK( symbol, retv );

        retv.Printf( _( "Symbol '%s' pin '%s'" ), symbol->GetRef( &aSheetPath, true ),
                     pin->GetNumber() );
        break;
    }
    case SCH_SHEET_PIN_T:
    {
        const SCH_SHEET_PIN* pin = static_cast<const SCH_SHEET_PIN*>( aItem );
        wxCHECK( pin, retv );

        SCH_SHEET* sheet = pin->GetParent();
        wxCHECK( sheet, retv );

        retv.Printf( _( "Sheet '%s' pin '%s'" ), sheet->GetName(), pin->GetText() );
        break;
    }
    case SCH_LABEL_T:
    {
        const SCH_LABEL* label = static_cast<const SCH_LABEL*>( aItem );
        wxCHECK( label, retv );

        retv.Printf( _( "Label '%s' at %s, %s" ), label->GetText(),
                     aUnitsProvider->MessageTextFromValue( label->GetPosition().x ),
                     aUnitsProvider->MessageTextFromValue( label->GetPosition().y ) );
        break;
    }
    case SCH_GLOBAL_LABEL_T:
    {
        const SCH_GLOBALLABEL* label = static_cast<const SCH_GLOBALLABEL*>( aItem );
        wxCHECK( label, retv );

        retv.Printf( _( "Global label '%s' at %s, %s" ), label->GetText(),
                     aUnitsProvider->MessageTextFromValue( label->GetPosition().x ),
                     aUnitsProvider->MessageTextFromValue( label->GetPosition().y ) );
        break;
    }
    case SCH_HIER_LABEL_T:
    {
        const SCH_HIERLABEL* label = static_cast<const SCH_HIERLABEL*>( aItem );
        wxCHECK( label, retv );

        retv.Printf( _( "Hierarchical label '%s' at %s, %s" ), label->GetText(),
                     aUnitsProvider->MessageTextFromValue( label->GetPosition().x ),
                     aUnitsProvider->MessageTextFromValue( label->GetPosition().y ) );
        break;
    }
    case SCH_JUNCTION_T:
    {
        const SCH_JUNCTION* junction = static_cast<const SCH_JUNCTION*>( aItem );
        wxCHECK( junction, retv );

        retv.Printf( _( "Junction at %s, %s" ),
                     aUnitsProvider->MessageTextFromValue( junction->GetPosition().x ),
                     aUnitsProvider->MessageTextFromValue( junction->GetPosition().y ) );
        break;
    }
    case SCH_BUS_WIRE_ENTRY_T:
    {
        const SCH_BUS_WIRE_ENTRY* entry = static_cast<const SCH_BUS_WIRE_ENTRY*>( aItem );
        wxCHECK( entry, retv );

        retv.Printf( _( "Bus to wire entry from %s, %s to %s, %s" ),
                     aUnitsProvider->MessageTextFromValue( entry->GetPosition().x ),
                     aUnitsProvider->MessageTextFromValue( entry->GetPosition().y ),
                     aUnitsProvider->MessageTextFromValue( entry->GetEnd().x ),
                     aUnitsProvider->MessageTextFromValue( entry->GetEnd().y ) );
        break;
    }
    case SCH_BUS_BUS_ENTRY_T:
    {
        const SCH_BUS_BUS_ENTRY* entry = static_cast<const SCH_BUS_BUS_ENTRY*>( aItem );
        wxCHECK( entry, retv );

        retv.Printf( _( "Bus to bus entry from %s, %s to %s, %s" ),
                     aUnitsProvider->MessageTextFromValue( entry->GetPosition().x ),
                     aUnitsProvider->MessageTextFromValue( entry->GetPosition().y ),
                     aUnitsProvider->MessageTextFromValue( entry->GetEnd().x ),
                     aUnitsProvider->MessageTextFromValue( entry->GetEnd().y ) );
        break;
    }
    default:
        retv.Printf( _( "Unhandled item type %d" ), aItem->Type() );
    }

    return retv;
}


void SCH_EDIT_FRAME::MakeNetNavigatorNode( const wxString& aNetName, wxTreeItemId aParentId,
                                           const NET_NAVIGATOR_ITEM_DATA* aSelection )
{
    wxCHECK( !aNetName.IsEmpty(), /* void */ );
    wxCHECK( m_schematic, /* void */ );
    wxCHECK( m_netNavigator, /* void */ );

    wxTreeItemId expandId = aParentId;
    CONNECTION_GRAPH* connectionGraph = m_schematic->ConnectionGraph();

    wxCHECK( connectionGraph, /* void */ );

    wxString sheetPathPrefix;
    const std::vector<CONNECTION_SUBGRAPH*> subgraphs =
            connectionGraph->GetAllSubgraphs( aNetName );

    for( const CONNECTION_SUBGRAPH* subGraph : subgraphs )
    {
        NET_NAVIGATOR_ITEM_DATA* itemData = nullptr;
        SCH_SHEET_PATH sheetPath = subGraph->GetSheet();

        wxCHECK2( subGraph && sheetPath.Last(), continue );

        if( subGraph->GetItems().empty() )
            continue;

        itemData = new NET_NAVIGATOR_ITEM_DATA( sheetPath, nullptr );

        bool stripTrailingSeparator = !sheetPath.Last()->IsRootSheet();
        wxString txt =  sheetPath.PathHumanReadable( true, stripTrailingSeparator );
        wxTreeItemId sheetId = m_netNavigator->AppendItem( aParentId, txt, -1, -1, itemData );

        if( aSelection && *aSelection == *itemData )
            m_netNavigator->SelectItem( sheetId );

        // If there is only one sheet in the schematic, always expand the sheet tree.
        if( Schematic().GetSheets().size() == 1 )
            expandId = sheetId;

        for( const SCH_ITEM* item : subGraph->GetItems() )
        {
            KICAD_T type = item->Type();

            if( ( type == SCH_LINE_T ) || ( type == SCH_JUNCTION_T )
              || ( type == SCH_BUS_WIRE_ENTRY_T ) || ( type == SCH_BUS_BUS_ENTRY_T ) )
                continue;

            itemData = new NET_NAVIGATOR_ITEM_DATA( sheetPath, item );
            wxTreeItemId id = m_netNavigator->AppendItem( sheetId,
                                                          GetNetNavigatorItemText( item, sheetPath,
                                                                                   this ),
                                                          -1, -1, itemData );

            if( aSelection && *aSelection == *itemData )
            {
                expandId = sheetId;
                m_netNavigator->EnsureVisible( id );
                m_netNavigator->SelectItem( id );
            }
        }
    }

    m_netNavigator->Expand( aParentId );
}


void SCH_EDIT_FRAME::RefreshNetNavigator( const NET_NAVIGATOR_ITEM_DATA* aSelection )
{
    wxCHECK( m_netNavigator, /* void */ );

    if( m_netNavigator->IsEmpty() && m_highlightedConn.IsEmpty() )
        return;

    if( !m_netNavigator->IsEmpty() && m_highlightedConn.IsEmpty() )
    {
        m_netNavigator->DeleteAllItems();
        return;
    }

    if( !m_netNavigator->IsEmpty() )
    {
        const wxString shownNetName = m_netNavigator->GetItemText( m_netNavigator->GetRootItem() );

        if( shownNetName != m_highlightedConn )
        {
            m_netNavigator->DeleteAllItems();

            wxTreeItemId rootId = m_netNavigator->AddRoot( m_highlightedConn, 0 );

            MakeNetNavigatorNode( m_highlightedConn, rootId, aSelection );
        }
        else
        {
            NET_NAVIGATOR_ITEM_DATA* itemData = nullptr;

            wxTreeItemId selection = m_netNavigator->GetSelection();

            if( selection.IsOk() )
                itemData = dynamic_cast<NET_NAVIGATOR_ITEM_DATA*>( m_netNavigator->GetItemData( selection ) );

            m_netNavigator->DeleteAllItems();
            wxTreeItemId rootId = m_netNavigator->AddRoot( m_highlightedConn, 0 );

            MakeNetNavigatorNode( m_highlightedConn, rootId, itemData );
        }
    }
    else
    {
        wxTreeItemId rootId = m_netNavigator->AddRoot( m_highlightedConn, 0 );

        MakeNetNavigatorNode( m_highlightedConn, rootId, aSelection );
    }
}


void SCH_EDIT_FRAME::SelectNetNavigatorItem( const NET_NAVIGATOR_ITEM_DATA* aSelection )
{
    wxCHECK( m_netNavigator, /* void */ );

    // Maybe in the future we can do something like collapse the tree for an empty selection.
    // For now, leave the tree selection in its current state.
    if( !aSelection )
        return;

    wxTreeItemIdValue sheetCookie;
    NET_NAVIGATOR_ITEM_DATA* itemData = nullptr;
    wxTreeItemId rootId = m_netNavigator->GetRootItem();
    wxTreeItemId sheetId = m_netNavigator->GetFirstChild( rootId, sheetCookie );

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
    if( !m_netNavigator )
        return nullptr;

    wxTreeItemId id = m_netNavigator->GetSelection();

    if( !id.IsOk() || ( id == m_netNavigator->GetRootItem() ) )
        return nullptr;

    NET_NAVIGATOR_ITEM_DATA* itemData =
            dynamic_cast<NET_NAVIGATOR_ITEM_DATA*>( m_netNavigator->GetItemData( id ) );

    wxCHECK( itemData, nullptr );

    return itemData->GetItem();
}


void SCH_EDIT_FRAME::onNetNavigatorSelection( wxTreeEvent& aEvent )
{
    wxCHECK( m_netNavigator, /* void */ );

    wxTreeItemId id = aEvent.GetItem();

    // Clicking on the root item (net name ) does nothing.
    if( id == m_netNavigator->GetRootItem() )
        return;

    NET_NAVIGATOR_ITEM_DATA* itemData =
            dynamic_cast<NET_NAVIGATOR_ITEM_DATA*>( m_netNavigator->GetItemData( id ) );

    wxCHECK( itemData, /* void */ );

    if( GetCurrentSheet() != itemData->GetSheetPath() )
    {
        Schematic().SetCurrentSheet( itemData->GetSheetPath() );
        DisplayCurrentSheet();
    }

    // Do not focus on item when a sheet tree node is selected.
    if( m_netNavigator->GetItemParent( id ) != m_netNavigator->GetRootItem()
      && itemData->GetItem() )
    {
        // Make sure we didn't remove the item and/or the screen it resides on before we access it.
        const SCH_ITEM* item = itemData->GetItem();

        // Don't search for child items in screen r-tree.
        item = ( ( item->Type() == SCH_SHEET_PIN_T ) || ( item->Type() == SCH_PIN_T ) ) ?
               static_cast<const SCH_ITEM*>( item->GetParent() ) : item;

        const SCH_SCREEN* screen = itemData->GetSheetPath().LastScreen();

        wxCHECK( screen, /* void */ );
        wxCHECK( screen->Items().contains( item, true ), /* void */ );

        FocusOnLocation( itemData->GetItem()->GetBoundingBox().Centre() );
    }

    GetCanvas()->Refresh();
}


void SCH_EDIT_FRAME::onNetNavigatorSelChanging( wxTreeEvent& aEvent )
{
    wxCHECK( m_netNavigator, /* void */ );
}


void SCH_EDIT_FRAME::ToggleNetNavigator()
{
    EESCHEMA_SETTINGS* cfg = eeconfig();

    wxCHECK( cfg, /* void */ );

    wxAuiPaneInfo& netNavigatorPane = m_auimgr.GetPane( NetNavigatorPaneName() );

    netNavigatorPane.Show( !netNavigatorPane.IsShown() );

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
}


void SCH_EDIT_FRAME::onResizeNetNavigator( wxSizeEvent& aEvent )
{
    aEvent.Skip();

    // Called when resizing the Hierarchy Navigator panel
    // Store the current pane size
    // It allows to retrieve the last defined pane size when switching between
    // docked and floating pane state
    // Note: *DO NOT* call m_auimgr.Update() here: it crashes Kicad at least on Windows

    EESCHEMA_SETTINGS* cfg = dynamic_cast<EESCHEMA_SETTINGS*>( Kiface().KifaceSettings() );

    wxCHECK( cfg, /* void */ );

    wxAuiPaneInfo& netNavigatorPane = m_auimgr.GetPane( NetNavigatorPaneName() );

    if( m_netNavigator->IsShown() )
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
