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
#include <schematic.h>
#include <connection_graph.h>
#include <widgets/wx_aui_utils.h>
#include <tools/ee_actions.h>


class NET_NAVIGATOR_ITEM_DATA : public wxTreeItemData
{
public:
    NET_NAVIGATOR_ITEM_DATA( const SCH_SHEET_PATH& aSheetPath, const VECTOR2I& aItemCenterPos ) :
        m_sheetPath( aSheetPath ),
        m_itemCenterPos( aItemCenterPos )
    {
    }

    NET_NAVIGATOR_ITEM_DATA() {}

    SCH_SHEET_PATH& GetSheetPath() { return m_sheetPath; }
    VECTOR2I& GetItemCenterPos() { return m_itemCenterPos; }

    bool operator==( const NET_NAVIGATOR_ITEM_DATA& aRhs ) const
    {
        return ( m_sheetPath == aRhs.m_sheetPath ) && ( m_itemCenterPos == aRhs.m_itemCenterPos );
    }

    NET_NAVIGATOR_ITEM_DATA& operator=( const NET_NAVIGATOR_ITEM_DATA& aItemData )
    {
        if( this == &aItemData )
            return *this;

        m_sheetPath = aItemData.m_sheetPath;
        m_itemCenterPos = aItemData.m_itemCenterPos;

        return *this;
    }

private:
    SCH_SHEET_PATH m_sheetPath;
    VECTOR2I m_itemCenterPos;
};


void SCH_EDIT_FRAME::MakeNetNavigatorNode( const wxString& aNetName, wxTreeItemId aParentId )
{
    wxCHECK( !aNetName.IsEmpty(), /* void */ );
    wxCHECK( m_schematic, /* void */ );
    wxCHECK( m_netNavigator, /* void */ );

    CONNECTION_GRAPH* connectionGraph = m_schematic->ConnectionGraph();

    wxCHECK( connectionGraph, /* void */ );

    wxString sheetPathPrefix;
    const std::vector<CONNECTION_SUBGRAPH*> subgraphs =
            connectionGraph->GetAllSubgraphs( aNetName );

    for( const CONNECTION_SUBGRAPH* subGraph : subgraphs )
    {
        SCH_SHEET_PATH sheetPath = subGraph->GetSheet();

        // if( subgraphs.size() > 1 )
            sheetPathPrefix = _( "Sheet: " ) + sheetPath.PathHumanReadable() + wxS( ", " );

        for( const SCH_ITEM* item : subGraph->GetItems() )
        {
            VECTOR2I itemCenterPos = item->GetBoundingBox().Centre();
            m_netNavigator->AppendItem( aParentId,
                                        sheetPathPrefix + item->GetItemDescription( this ),
                                        -1, -1,
                                        new NET_NAVIGATOR_ITEM_DATA( sheetPath, itemCenterPos ) );
        }
    }
}


void SCH_EDIT_FRAME::RefreshNetNavigator()
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

            MakeNetNavigatorNode( m_highlightedConn, rootId );
            m_netNavigator->Expand( rootId );
        }
        else
        {
            // If it's the same net, we have to manually check to make sure the net has
            // not changed.  This is an ugly hack because we have no way to track a
            // single connection object change in the connection graph code.
            wxTreeItemData* treeItemData = nullptr;
            NET_NAVIGATOR_ITEM_DATA* tmp = nullptr;
            wxTreeItemId selectedId = m_netNavigator->GetSelection();

            wxString selectedText;
            NET_NAVIGATOR_ITEM_DATA selectedItemData;

            if( ( selectedId != m_netNavigator->GetRootItem() ) && selectedId.IsOk() )
            {
                selectedText = m_netNavigator->GetItemText( selectedId );

                treeItemData = m_netNavigator->GetItemData( selectedId );
                tmp = static_cast<NET_NAVIGATOR_ITEM_DATA*>( treeItemData );

                if( tmp )
                    selectedItemData = *tmp;
            }

            m_netNavigator->DeleteAllItems();
            wxTreeItemId rootId = m_netNavigator->AddRoot( m_highlightedConn, 0 );

            MakeNetNavigatorNode( m_highlightedConn, rootId );
            m_netNavigator->Expand( rootId );

            if( ( selectedId != m_netNavigator->GetRootItem() ) && !selectedText.IsEmpty() )
            {
                wxTreeItemIdValue cookie;

                wxTreeItemId id = m_netNavigator->GetFirstChild( rootId, cookie );

                while( id.IsOk() )
                {
                    wxString treeItemText = m_netNavigator->GetItemText( id );
                    treeItemData = m_netNavigator->GetItemData( id );
                    tmp = static_cast<NET_NAVIGATOR_ITEM_DATA*>( treeItemData );

                    if( ( treeItemText == selectedText )
                      && ( tmp && ( *tmp == selectedItemData ) ) )
                    {
                        m_netNavigator->SetFocusedItem( id );
                        break;
                    }

                    id = m_netNavigator->GetNextChild( rootId, cookie );
                }
            }
        }
    }
    else
    {
        wxTreeItemId rootId = m_netNavigator->AddRoot( m_highlightedConn, 0 );

        MakeNetNavigatorNode( m_highlightedConn, rootId );
        m_netNavigator->Expand( rootId );
    }
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
        // GetToolManager()->RunAction( ACTIONS::cancelInteractive, true );
        // GetToolManager()->RunAction( EE_ACTIONS::clearSelection, true );
        Schematic().SetCurrentSheet( itemData->GetSheetPath() );
        DisplayCurrentSheet();
    }

    FocusOnLocation( itemData->GetItemCenterPos() );
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
