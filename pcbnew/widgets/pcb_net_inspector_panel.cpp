/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <widgets/pcb_net_inspector_panel.h>
#include <widgets/pcb_net_inspector_panel_data_model.h>

#include <board_design_settings.h>
#include <board_stackup_manager/board_stackup.h>
#include <confirm.h>
#include <connectivity/connectivity_algo.h>
#include <dialogs/dialog_text_entry.h>
#include <footprint.h>
#include <pad.h>
#include <pcb_edit_frame.h>
#include <pcb_painter.h>
#include <pcb_track.h>
#include <pgm_base.h>
#include <settings/settings_manager.h>
#include <validators.h>
#include <wildcards_and_files_ext.h>
#include <eda_pattern_match.h>

#include <wx/wupdlock.h>
#include <wx/headerctrl.h>
#include <wx/filedlg.h>

#include <algorithm>

PCB_NET_INSPECTOR_PANEL::PCB_NET_INSPECTOR_PANEL( wxWindow* parent, PCB_EDIT_FRAME* aFrame ) :
        NET_INSPECTOR_PANEL( parent, aFrame ), m_zero_netitem( nullptr ), m_frame( aFrame )
{
    m_brd = m_frame->GetBoard();

    m_data_model = new DATA_MODEL( *this );
    m_netsList->AssociateModel( &*m_data_model );

    // Rebuild nets list
    buildNetsList( true );

    // Register the panel to receive board change notifications
    if( m_brd != nullptr )
    {
        OnBoardHighlightNetChanged( *m_brd );
        m_brd->AddListener( this );
    }

    // Connect to board events
    m_frame->Bind( EDA_EVT_UNITS_CHANGED, &PCB_NET_INSPECTOR_PANEL::onUnitsChanged, this );

    // Connect to wxDataViewCtrl events
    m_netsList->Bind( wxEVT_DATAVIEW_ITEM_EXPANDED, &PCB_NET_INSPECTOR_PANEL::OnExpandCollapseRow,
                      this );
    m_netsList->Bind( wxEVT_DATAVIEW_ITEM_COLLAPSED, &PCB_NET_INSPECTOR_PANEL::OnExpandCollapseRow,
                      this );
    m_netsList->Bind( wxEVT_DATAVIEW_COLUMN_HEADER_RIGHT_CLICK,
                      &PCB_NET_INSPECTOR_PANEL::OnHeaderContextMenu, this );
    m_netsList->Bind( wxEVT_DATAVIEW_ITEM_CONTEXT_MENU,
                      &PCB_NET_INSPECTOR_PANEL::OnNetsListContextMenu, this );
    m_netsList->Bind( wxEVT_DATAVIEW_ITEM_ACTIVATED,
                      &PCB_NET_INSPECTOR_PANEL::OnNetsListItemActivated, this );
    m_netsList->Bind( wxEVT_DATAVIEW_COLUMN_SORTED,
                      &PCB_NET_INSPECTOR_PANEL::OnColumnSorted, this );
}

PCB_NET_INSPECTOR_PANEL::~PCB_NET_INSPECTOR_PANEL()
{
    SaveSettings();

    m_netsList->AssociateModel( nullptr );

    if( m_brd != nullptr )
        m_brd->RemoveListener( this );

    // Disconnect from board events
    m_frame->Unbind( EDA_EVT_UNITS_CHANGED, &PCB_NET_INSPECTOR_PANEL::onUnitsChanged, this );

    // Connect to wxDataViewCtrl events
    m_netsList->Unbind( wxEVT_DATAVIEW_ITEM_EXPANDED, &PCB_NET_INSPECTOR_PANEL::OnExpandCollapseRow,
                        this );
    m_netsList->Unbind( wxEVT_DATAVIEW_ITEM_COLLAPSED,
                        &PCB_NET_INSPECTOR_PANEL::OnExpandCollapseRow, this );
    m_netsList->Unbind( wxEVT_DATAVIEW_COLUMN_HEADER_RIGHT_CLICK,
                        &PCB_NET_INSPECTOR_PANEL::OnHeaderContextMenu, this );
    m_netsList->Unbind( wxEVT_DATAVIEW_ITEM_CONTEXT_MENU,
                        &PCB_NET_INSPECTOR_PANEL::OnNetsListContextMenu, this );
    m_netsList->Unbind( wxEVT_DATAVIEW_ITEM_ACTIVATED,
                        &PCB_NET_INSPECTOR_PANEL::OnNetsListItemActivated, this );
    m_netsList->Unbind( wxEVT_DATAVIEW_COLUMN_SORTED,
                      &PCB_NET_INSPECTOR_PANEL::OnColumnSorted, this );
}


/*****************************************************************************************
 * 
 * Grid / model columns configuration
 * 
 * ***************************************************************************************/


void PCB_NET_INSPECTOR_PANEL::buildColumns()
{
    m_columns.clear();

    // Set up the column display vector
    m_columns.emplace_back( 0u, UNDEFINED_LAYER, _( "Name" ), _( "Net Name" ),
                            CSV_COLUMN_DESC::CSV_QUOTE, false );
    m_columns.emplace_back( 1u, UNDEFINED_LAYER, _( "Netclass" ), _( "Netclass" ),
                            CSV_COLUMN_DESC::CSV_QUOTE, false );
    m_columns.emplace_back( 2u, UNDEFINED_LAYER, _( "Total Length" ), _( "Net Length" ),
                            CSV_COLUMN_DESC::CSV_NONE, true );
    m_columns.emplace_back( 3u, UNDEFINED_LAYER, _( "Via Count" ), _( "Via Count" ),
                            CSV_COLUMN_DESC::CSV_NONE, false );
    m_columns.emplace_back( 4u, UNDEFINED_LAYER, _( "Via Length" ), _( "Via Length" ),
                            CSV_COLUMN_DESC::CSV_NONE, true );
    m_columns.emplace_back( 5u, UNDEFINED_LAYER, _( "Track Length" ), _( "Track Length" ),
                            CSV_COLUMN_DESC::CSV_NONE, true );
    m_columns.emplace_back( 6u, UNDEFINED_LAYER, _( "Die Length" ), _( "Die Length" ),
                            CSV_COLUMN_DESC::CSV_NONE, true );
    m_columns.emplace_back( 7u, UNDEFINED_LAYER, _( "Pad Count" ), _( "Pad Count" ),
                            CSV_COLUMN_DESC::CSV_NONE, false );

    std::vector<std::function<void( void )>> add_col{
        [&]()
        {
            m_netsList->AppendTextColumn( m_columns[COLUMN_NAME].display_name,
                                          m_columns[COLUMN_NAME], wxDATAVIEW_CELL_INERT, -1,
                                          wxALIGN_LEFT,
                                          wxDATAVIEW_COL_RESIZABLE | wxDATAVIEW_COL_SORTABLE );
        },
        [&]()
        {
            m_netsList->AppendTextColumn( m_columns[COLUMN_NETCLASS].display_name,
                                          m_columns[COLUMN_NETCLASS], wxDATAVIEW_CELL_INERT, -1,
                                          wxALIGN_LEFT,
                                          wxDATAVIEW_COL_RESIZABLE | wxDATAVIEW_COL_REORDERABLE
                                                  | wxDATAVIEW_COL_SORTABLE );
        },
        [&]()
        {
            m_netsList->AppendTextColumn( m_columns[COLUMN_TOTAL_LENGTH].display_name,
                                          m_columns[COLUMN_TOTAL_LENGTH], wxDATAVIEW_CELL_INERT, -1,
                                          wxALIGN_CENTER,
                                          wxDATAVIEW_COL_RESIZABLE | wxDATAVIEW_COL_REORDERABLE
                                                  | wxDATAVIEW_COL_SORTABLE );
        },
        [&]()
        {
            m_netsList->AppendTextColumn( m_columns[COLUMN_VIA_COUNT].display_name,
                                          m_columns[COLUMN_VIA_COUNT], wxDATAVIEW_CELL_INERT, -1,
                                          wxALIGN_CENTER,
                                          wxDATAVIEW_COL_RESIZABLE | wxDATAVIEW_COL_REORDERABLE
                                                  | wxDATAVIEW_COL_SORTABLE );
        },
        [&]()
        {
            m_netsList->AppendTextColumn( m_columns[COLUMN_VIA_LENGTH].display_name,
                                          m_columns[COLUMN_VIA_LENGTH], wxDATAVIEW_CELL_INERT, -1,
                                          wxALIGN_CENTER,
                                          wxDATAVIEW_COL_RESIZABLE | wxDATAVIEW_COL_REORDERABLE
                                                  | wxDATAVIEW_COL_SORTABLE );
        },
        [&]()
        {
            m_netsList->AppendTextColumn( m_columns[COLUMN_BOARD_LENGTH].display_name,
                                          m_columns[COLUMN_BOARD_LENGTH], wxDATAVIEW_CELL_INERT, -1,
                                          wxALIGN_CENTER,
                                          wxDATAVIEW_COL_RESIZABLE | wxDATAVIEW_COL_REORDERABLE
                                                  | wxDATAVIEW_COL_SORTABLE );
        },
        [&]()
        {
            m_netsList->AppendTextColumn( m_columns[COLUMN_PAD_DIE_LENGTH].display_name,
                                          m_columns[COLUMN_PAD_DIE_LENGTH], wxDATAVIEW_CELL_INERT,
                                          -1, wxALIGN_CENTER,
                                          wxDATAVIEW_COL_RESIZABLE | wxDATAVIEW_COL_REORDERABLE
                                                  | wxDATAVIEW_COL_SORTABLE );
        },
        [&]()
        {
            m_netsList->AppendTextColumn( m_columns[COLUMN_PAD_COUNT].display_name,
                                          m_columns[COLUMN_PAD_COUNT], wxDATAVIEW_CELL_INERT, -1,
                                          wxALIGN_CENTER,
                                          wxDATAVIEW_COL_RESIZABLE | wxDATAVIEW_COL_REORDERABLE
                                                  | wxDATAVIEW_COL_SORTABLE );
        }
    };

    // If we have not yet loaded the first board, use a dummy local settings object to ensure we
    // don't over-write existing board settings (note that PCB_EDIT_FRAME loads the local settings
    // object prior to loading the board; the two are not synced and we need to account for that)
    PANEL_NET_INSPECTOR_SETTINGS* cfg = nullptr;

    if( m_board_loaded )
    {
        PROJECT_LOCAL_SETTINGS& localSettings = Pgm().GetSettingsManager().Prj().GetLocalSettings();
        cfg = &localSettings.m_NetInspectorPanel;
    }
    else
    {
        cfg = new PANEL_NET_INSPECTOR_SETTINGS();
    }

    // Count number of copper layers
    m_num_copper_layers = 0;

    for( PCB_LAYER_ID layer : m_brd->GetEnabledLayers().Seq() )
    {
        if( IsCopperLayer( layer ) )
            ++m_num_copper_layers;
    }

    // Reset the column display settings if column count doesn't match
    const int totalNumColumns = add_col.size() + m_num_copper_layers;

    if( (int) cfg->col_order.size() != totalNumColumns
        || (int) cfg->col_hidden.size() != totalNumColumns )
    {
        cfg->col_order.resize( totalNumColumns );
        cfg->col_hidden.resize( totalNumColumns );

        for( int i = 0; i < totalNumColumns; ++i )
        {
            cfg->col_order[i] = i;
            cfg->col_hidden[i] = false;
        }
    }

    // Check that all rows are unique to protect against corrupted settings data
    std::set<int> col_order_set( cfg->col_order.begin(), cfg->col_order.end() );
    if( col_order_set.size() != cfg->col_order.size() )
    {
        for( std::size_t i = 0; i < cfg->col_order.size(); ++i )
            cfg->col_order[i] = i;
    }

    // Add column records for copper layers
    for( PCB_LAYER_ID layer : m_brd->GetEnabledLayers().Seq() )
    {
        if( !IsCopperLayer( layer ) )
            continue;

        m_columns.emplace_back( m_columns.size(), layer, m_brd->GetLayerName( layer ),
                                m_brd->GetLayerName( layer ), CSV_COLUMN_DESC::CSV_NONE, true );
    }

    // Add display columns in settings order
    for( std::size_t i = 0; i < cfg->col_order.size(); ++i )
    {
        const int addModelColumn = cfg->col_order[i];

        if( addModelColumn >= (int) add_col.size() )
        {
            m_netsList->AppendTextColumn( m_brd->GetLayerName( m_columns[addModelColumn].layer ),
                                          m_columns[addModelColumn], wxDATAVIEW_CELL_INERT, -1,
                                          wxALIGN_CENTER,
                                          wxDATAVIEW_COL_RESIZABLE | wxDATAVIEW_COL_REORDERABLE
                                                  | wxDATAVIEW_COL_SORTABLE );
        }
        else
        {
            add_col.at( cfg->col_order[i] )();
        }
    }

    // Set the name column as the expander row
    if( wxDataViewColumn* col = getDisplayedColumnForModelField( COLUMN_NAME ) )
    {
        m_netsList->SetExpanderColumn( col );
    }

    adjustListColumnSizes( cfg );

    // Delete the temporary config if used
    if( !m_board_loaded )
    {
        delete cfg;
    }
}


void PCB_NET_INSPECTOR_PANEL::adjustListColumnSizes( PANEL_NET_INSPECTOR_SETTINGS* cfg )
{
    wxWindowUpdateLocker locker( m_netsList );

    if( cfg->col_widths.size() != m_columns.size() )
    {
        int minValueWidth = GetTextExtent( wxT( "00000,000 mm" ) ).x;
        int minNumberWidth = GetTextExtent( wxT( "000" ) ).x;
        int minNameWidth = GetTextExtent( wxT( "MMMMMMMMMMMM" ) ).x;

        // Considering left and right margins.
        // For wxRenderGeneric it is 5px.
        // Also account for the sorting arrow in the column header.
        // Column 0 also needs space for any potential expander icons.
        const int margins = 15;
        const int extra_width = 30;

        auto getTargetWidth = [&]( int columnID )
        {
            switch( columnID )
            {
            case COLUMN_NAME: return minNameWidth + extra_width;
            case COLUMN_NETCLASS: return minNameWidth + margins;
            case COLUMN_VIA_COUNT: return minNumberWidth + margins;
            case COLUMN_PAD_COUNT: return minNumberWidth + margins;
            default: return minValueWidth + margins;
            }
        };

        wxASSERT( m_columns.size() == cfg->col_order.size() );

        for( size_t i = 0; i < m_columns.size(); ++i )
        {
            const int modelColumn = cfg->col_order[i];
            int       titleSize = GetTextExtent( m_columns[modelColumn].display_name ).x;
            titleSize = modelColumn == COLUMN_NAME ? titleSize + extra_width : titleSize + margins;
            const int valSize = getTargetWidth( modelColumn );
            m_netsList->GetColumn( i )->SetWidth( std::max( titleSize, valSize ) );
        }
    }
    else
    {
        wxASSERT( m_columns.size() == cfg->col_hidden.size() );
        wxASSERT( m_columns.size() == cfg->col_widths.size() );

        for( size_t ii = 0; ii < m_columns.size(); ++ii )
        {
            const int newWidth = cfg->col_widths[ii];
            // Make sure we end up with something non-zero so we can resize it
            m_netsList->GetColumn( ii )->SetWidth( std::max( newWidth, 10 ) );
            m_netsList->GetColumn( ii )->SetHidden( cfg->col_hidden[ii] );
        }
    }

    m_netsList->Refresh();
}


bool PCB_NET_INSPECTOR_PANEL::restoreSortColumn( int sortingColumnId, bool sortOrderAsc )
{
    if( sortingColumnId != -1 )
    {
        if( wxDataViewColumn* col = getDisplayedColumnForModelField( sortingColumnId ) )
        {
            col->SetSortOrder( sortOrderAsc );
            m_data_model->Resort();
            return true;
        }
    }

    return false;
}


wxDataViewColumn* PCB_NET_INSPECTOR_PANEL::getDisplayedColumnForModelField( int columnId )
{
    for( unsigned int i = 0; i < m_netsList->GetColumnCount(); ++i )
    {
        wxDataViewColumn* col = m_netsList->GetColumn( i );

        if( (int) col->GetModelColumn() == columnId )
        {
            return col;
        }
    }

    return nullptr;
}


/*****************************************************************************************
 * 
 * Nets list generation
 * 
 * ***************************************************************************************/


void PCB_NET_INSPECTOR_PANEL::buildNetsList( bool rebuildColumns )
{
    // Only build the list of nets if there is a board present
    if( !m_brd )
        return;

    m_in_build_nets_list = true;

    PROJECT_LOCAL_SETTINGS& localSettings = Pgm().GetSettingsManager().Prj().GetLocalSettings();
    PANEL_NET_INSPECTOR_SETTINGS* cfg = &localSettings.m_NetInspectorPanel;

    // Refresh all filtering / grouping settings
    m_filter_by_net_name = cfg->filter_by_net_name;
    m_filter_by_netclass = cfg->filter_by_netclass;
    m_show_zero_pad_nets = cfg->show_zero_pad_nets;
    m_group_by_netclass = cfg->group_by_netclass;
    m_group_by_constraint = cfg->group_by_constraint;

    // when rebuilding the netlist, try to keep the row selection
    wxDataViewItemArray sel;
    m_netsList->GetSelections( sel );

    std::vector<int> prev_selected_netcodes;
    prev_selected_netcodes.reserve( sel.GetCount() );

    for( unsigned int i = 0; i < sel.GetCount(); ++i )
    {
        const LIST_ITEM* item = static_cast<const LIST_ITEM*>( sel.Item( i ).GetID() );
        prev_selected_netcodes.push_back( item->GetNetCode() );
    }

    int  sorting_column_id = cfg->sorting_column;
    bool sort_order_asc = cfg->sort_order_asc;

    if( wxDataViewColumn* sorting_column = m_netsList->GetSortingColumn() )
    {
        if( !m_board_loading )
        {
            sorting_column_id = static_cast<int>( sorting_column->GetModelColumn() );
            sort_order_asc = sorting_column->IsSortOrderAscending();
        }

        // On GTK, wxDVC will crash if you rebuild with a sorting column set.
        sorting_column->UnsetAsSortKey();
    }

    if( rebuildColumns )
    {
        m_netsList->ClearColumns();
        buildColumns();
    }

    m_data_model->deleteAllItems();

    m_custom_group_rules.clear();

    for( const wxString& rule : cfg->custom_group_rules )
        m_custom_group_rules.push_back( std::make_unique<EDA_COMBINED_MATCHER>( rule, CTX_NET ) );

    m_data_model->addCustomGroups();

    std::vector<std::unique_ptr<LIST_ITEM>> new_items;

    std::vector<CN_ITEM*> prefiltered_cn_items = relevantConnectivityItems();

    struct NET_INFO
    {
        int           netcode;
        NETINFO_ITEM* net;
        unsigned int  pad_count;
    };

    struct NET_INFO_CMP_LESS
    {
        bool operator()( const NET_INFO& a, const NET_INFO& b ) const
        {
            return a.netcode < b.netcode;
        }
        bool operator()( const NET_INFO& a, int b ) const { return a.netcode < b; }
        bool operator()( int a, const NET_INFO& b ) const { return a < b.netcode; }
    };

    std::vector<NET_INFO> nets;
    nets.reserve( m_brd->GetNetInfo().NetsByNetcode().size() );

    for( const std::pair<int, NETINFO_ITEM*> ni : m_brd->GetNetInfo().NetsByNetcode() )
    {
        if( ni.first == 0 )
            m_zero_netitem = ni.second;

        if( netFilterMatches( ni.second, cfg ) )
            nets.emplace_back( NET_INFO{ ni.first, ni.second, 0 } );
    }

    // count the pads for each net.  since the nets are sorted by netcode
    // iterating over the footprints' pads is faster.
    for( FOOTPRINT* footprint : m_brd->Footprints() )
    {
        for( PAD* pad : footprint->Pads() )
        {
            auto i = std::lower_bound( nets.begin(), nets.end(), pad->GetNetCode(),
                                       NET_INFO_CMP_LESS() );

            if( i != nets.end() && i->netcode == pad->GetNetCode() )
                i->pad_count += 1;
        }
    }

    for( NET_INFO& ni : nets )
    {
        if( m_show_zero_pad_nets || ni.pad_count > 0 )
            new_items.emplace_back( buildNewItem( ni.net, ni.pad_count, prefiltered_cn_items ) );
    }

    m_data_model->addItems( std::move( new_items ) );

    // Re-enable the sorting column
    if( !restoreSortColumn( sorting_column_id, sort_order_asc ))
    {
        // By default sort by Name column
        restoreSortColumn( COLUMN_NAME, true );
    }

    // Try to restore the expanded groups
    if( m_board_loaded )
    {
        m_row_expanding = true;

        std::vector<std::pair<wxString, wxDataViewItem>> groupItems =
                m_data_model->getGroupDataViewItems();

        for( wxString& groupName : cfg->expanded_rows )
        {
            auto pred = [&groupName]( const std::pair<wxString, wxDataViewItem>& item )
            {
                return groupName == item.first;
            };

            auto tableItem = std::find_if( groupItems.begin(), groupItems.end(), pred );

            if( tableItem != groupItems.end() )
            {
                m_netsList->Expand( tableItem->second );
            }
        }

        m_row_expanding = false;
    }

    // try to restore the selected rows. Set the ones that we can't find any more to -1.
    sel.Clear();

    for( int& nc : prev_selected_netcodes )
    {
        std::optional<LIST_ITEM_ITER> r = m_data_model->findItem( nc );

        if( r )
        {
            const std::unique_ptr<LIST_ITEM>& list_item = *r.value();
            sel.Add( wxDataViewItem( list_item.get() ) );
        }
        else
        {
            nc = -1;
        }
    }

    if( !sel.IsEmpty() )
    {
        m_netsList->SetSelections( sel );
        m_netsList->EnsureVisible( sel.Item( 0 ) );
    }
    else
    {
        m_netsList->UnselectAll();
    }

    alg::delete_matching( prev_selected_netcodes, -1 );

    m_in_build_nets_list = false;
}


bool PCB_NET_INSPECTOR_PANEL::netFilterMatches( NETINFO_ITEM*                 aNet,
                                                PANEL_NET_INSPECTOR_SETTINGS* cfg ) const
{
    if( cfg == nullptr )
    {
        PROJECT_LOCAL_SETTINGS& localSettings = Pgm().GetSettingsManager().Prj().GetLocalSettings();
        cfg = &localSettings.m_NetInspectorPanel;
    }

    // Never show the unconnected net
    if( aNet->GetNetCode() <= 0 )
        return false;

    wxString  filterString = UnescapeString( m_searchCtrl->GetValue() ).Upper();
    wxString  netName = UnescapeString( aNet->GetNetname() ).Upper();
    NETCLASS* netClass = aNet->GetNetClass();
    wxString  netClassName = UnescapeString( netClass->GetName() ).Upper();

    bool matched = false;

    // No filter - match all
    if( filterString.Length() == 0 )
        matched = true;

    // Search on Netclass
    if( !matched && cfg->filter_by_netclass && netClassName.Find( filterString ) != wxNOT_FOUND )
        matched = true;

    // Search on Net name
    if( !matched && cfg->filter_by_net_name && netName.Find( filterString ) != wxNOT_FOUND )
        matched = true;

    // Remove unconnected nets if required
    if( matched )
    {
        if( !m_show_unconnected_nets )
            matched = !netName.StartsWith( wxT( "UNCONNECTED-(" ) );
    }

    return matched;
}


struct NETCODE_CMP_LESS
{
    bool operator()( const CN_ITEM* a, const CN_ITEM* b ) const { return a->Net() < b->Net(); }

    bool operator()( const CN_ITEM* a, int b ) const { return a->Net() < b; }

    bool operator()( int a, const CN_ITEM* b ) const { return a < b->Net(); }
};


std::unique_ptr<PCB_NET_INSPECTOR_PANEL::LIST_ITEM>
PCB_NET_INSPECTOR_PANEL::buildNewItem( NETINFO_ITEM* aNet, unsigned int aPadCount,
                                       const std::vector<CN_ITEM*>& aCNItems )
{
    std::unique_ptr<LIST_ITEM> new_item = std::make_unique<LIST_ITEM>( aNet );

    new_item->SetPadCount( aPadCount );

    const auto cn_items = std::equal_range( aCNItems.begin(), aCNItems.end(), aNet->GetNetCode(),
                                            NETCODE_CMP_LESS() );

    for( auto i = cn_items.first; i != cn_items.second; ++i )
    {
        BOARD_CONNECTED_ITEM* item = ( *i )->Parent();

        if( item->Type() == PCB_PAD_T )
            new_item->AddPadDieLength( static_cast<PAD*>( item )->GetPadToDieLength() );

        else if( PCB_TRACK* track = dynamic_cast<PCB_TRACK*>( item ) )
        {
            new_item->AddLayerWireLength( track->GetLength(),
                                          static_cast<int>( track->GetLayer() ) );

            if( item->Type() == PCB_VIA_T )
            {
                new_item->AddViaCount( 1 );
                new_item->AddViaLength( calculateViaLength( track ) );
            }
        }
    }

    return new_item;
}


std::vector<CN_ITEM*> PCB_NET_INSPECTOR_PANEL::relevantConnectivityItems() const
{
    // pre-filter the connectivity items and sort them by netcode.
    // this avoids quadratic runtime when building the whole net list and
    // calculating the total length for each net.
    const auto type_bits = std::bitset<MAX_STRUCT_TYPE_ID>()
                                   .set( PCB_TRACE_T )
                                   .set( PCB_ARC_T )
                                   .set( PCB_VIA_T )
                                   .set( PCB_PAD_T );

    std::vector<CN_ITEM*> cn_items;
    cn_items.reserve( 1024 );

    for( CN_ITEM* cn_item : m_brd->GetConnectivity()->GetConnectivityAlgo()->ItemList() )
    {
        if( cn_item->Valid() && type_bits[cn_item->Parent()->Type()] )
            cn_items.push_back( cn_item );
    }

    std::sort( cn_items.begin(), cn_items.end(), NETCODE_CMP_LESS() );

    return cn_items;
}


unsigned int PCB_NET_INSPECTOR_PANEL::calculateViaLength( const PCB_TRACK* aTrack ) const
{
    const PCB_VIA* via = dynamic_cast<const PCB_VIA*>( aTrack );

    if( !via )
        return 0;

    BOARD_DESIGN_SETTINGS& bds = m_brd->GetDesignSettings();

    // Must be static to keep from raising its ugly head in performance profiles
    static std::initializer_list<KICAD_T> traceAndPadTypes = { PCB_TRACE_T, PCB_ARC_T, PCB_PAD_T };

    // calculate the via length individually from the board stackup and via's start and end layer.
    if( bds.m_HasStackup )
    {
        PCB_LAYER_ID top_layer = UNDEFINED_LAYER;
        PCB_LAYER_ID bottom_layer = UNDEFINED_LAYER;

        for( int layer = via->TopLayer(); layer <= via->BottomLayer(); ++layer )
        {
            if( m_brd->GetConnectivity()->IsConnectedOnLayer( via, layer, traceAndPadTypes ) )
            {
                if( top_layer == UNDEFINED_LAYER )
                    top_layer = PCB_LAYER_ID( layer );
                else
                    bottom_layer = PCB_LAYER_ID( layer );
            }
        }

        if( top_layer == UNDEFINED_LAYER )
            top_layer = via->TopLayer();
        if( bottom_layer == UNDEFINED_LAYER )
            bottom_layer = via->BottomLayer();

        const BOARD_STACKUP& stackup = bds.GetStackupDescriptor();
        return stackup.GetLayerDistance( top_layer, bottom_layer );
    }
    else
    {
        int dielectricLayers = bds.GetCopperLayerCount() - 1;
        // TODO: not all dielectric layers are the same thickness!
        int layerThickness = bds.GetBoardThickness() / dielectricLayers;
        int effectiveBottomLayer;

        if( via->BottomLayer() == B_Cu )
            effectiveBottomLayer = F_Cu + dielectricLayers;
        else
            effectiveBottomLayer = via->BottomLayer();

        int layerCount = effectiveBottomLayer - via->TopLayer();

        return layerCount * layerThickness;
    }
}


void PCB_NET_INSPECTOR_PANEL::updateNet( NETINFO_ITEM* aNet )
{
    // something for the specified net has changed, update that row.
    // ignore nets that are not in our list because the filter doesn't match.
    if( !netFilterMatches( aNet ) )
    {
        m_data_model->deleteItem( m_data_model->findItem( aNet ) );
        return;
    }

    // if the net had no pads before, it might not be in the displayed list yet.
    // if it had pads and now doesn't anymore, we might need to remove it from the list.
    std::optional<LIST_ITEM_ITER> cur_net_row = m_data_model->findItem( aNet );

    const unsigned int node_count = m_brd->GetNodesCount( aNet->GetNetCode() );

    if( node_count == 0 && !m_show_zero_pad_nets )
    {
        m_data_model->deleteItem( cur_net_row );
        return;
    }

    std::unique_ptr<LIST_ITEM> new_list_item =
            buildNewItem( aNet, node_count, relevantConnectivityItems() );

    if( !cur_net_row )
    {
        m_data_model->addItem( std::move( new_list_item ) );
        return;
    }

    const std::unique_ptr<LIST_ITEM>& cur_list_item = *cur_net_row.value();

    if( cur_list_item->GetNetName() != new_list_item->GetNetName() )
    {
        // if the name has changed, it might require re-grouping.
        // it's easier to remove and re-insert it
        m_data_model->deleteItem( cur_net_row );
        m_data_model->addItem( std::move( new_list_item ) );
    }
    else
    {
        // update fields only
        cur_list_item->SetPadCount( new_list_item->GetPadCount() );
        cur_list_item->SetViaCount( new_list_item->GetViaCount() );

        for( size_t ii = 0; ii < MAX_CU_LAYERS; ++ii )
            cur_list_item->SetLayerWireLength( new_list_item->GetLayerWireLength( ii ), ii );

        cur_list_item->SetPadDieLength( new_list_item->GetPadDieLength() );

        updateDisplayedRowValues( cur_net_row );
    }
}


/*****************************************************************************************
 * 
 * Formatting helpers
 * 
 * ***************************************************************************************/


wxString PCB_NET_INSPECTOR_PANEL::formatNetCode( const NETINFO_ITEM* aNet ) const
{
    return wxString::Format( wxT( "%.3d" ), aNet->GetNetCode() );
}


wxString PCB_NET_INSPECTOR_PANEL::formatNetName( const NETINFO_ITEM* aNet ) const
{
    return UnescapeString( aNet->GetNetname() );
}


wxString PCB_NET_INSPECTOR_PANEL::formatCount( unsigned int aValue ) const
{
    return wxString::Format( wxT( "%u" ), aValue );
}


wxString PCB_NET_INSPECTOR_PANEL::formatLength( int64_t aValue ) const
{
    return m_frame->MessageTextFromValue(
            static_cast<long long int>( aValue ),
            !m_in_reporting /* Don't include unit label in the string when reporting */ );
}


void PCB_NET_INSPECTOR_PANEL::updateDisplayedRowValues( const std::optional<LIST_ITEM_ITER>& aRow )
{
    if( !aRow )
        return;

    wxDataViewItemArray sel;
    m_netsList->GetSelections( sel );

    m_data_model->updateItem( aRow );

    if( !sel.IsEmpty() )
    {
        m_netsList->SetSelections( sel );
        m_netsList->EnsureVisible( sel.Item( 0 ) );
    }
}


/*****************************************************************************************
 * 
 * BOARD_LISTENER event handling
 * 
 * ***************************************************************************************/


void PCB_NET_INSPECTOR_PANEL::OnBoardChanged()
{
    m_brd = m_frame->GetBoard();

    if( m_brd != nullptr )
        m_brd->AddListener( this );

    m_board_loaded = true;
    m_board_loading = true;

    PROJECT_LOCAL_SETTINGS& localSettings = Pgm().GetSettingsManager().Prj().GetLocalSettings();
    auto&                   cfg = localSettings.m_NetInspectorPanel;
    m_searchCtrl->SetValue( cfg.filter_text );

    buildNetsList( true );

    m_board_loading = false;
}

void PCB_NET_INSPECTOR_PANEL::OnBoardItemAdded( BOARD& aBoard, BOARD_ITEM* aBoardItem )
{
    if( !IsShownOnScreen() )
        return;

    if( NETINFO_ITEM* net = dynamic_cast<NETINFO_ITEM*>( aBoardItem ) )
    {
        // a new net has been added to the board.  add it to our list if it
        // passes the netname filter test.

        if( netFilterMatches( net ) )
        {
            std::unique_ptr<LIST_ITEM> new_item = std::make_unique<LIST_ITEM>( net );

            // the new net could have some pads already assigned, count them.
            new_item->SetPadCount( m_brd->GetNodesCount( net->GetNetCode() ) );

            m_data_model->addItem( std::move( new_item ) );
        }
    }
    else if( BOARD_CONNECTED_ITEM* i = dynamic_cast<BOARD_CONNECTED_ITEM*>( aBoardItem ) )
    {
        std::optional<LIST_ITEM_ITER> r = m_data_model->findItem( i->GetNet() );

        if( r )
        {
            // try to handle frequent operations quickly.
            if( PCB_TRACK* track = dynamic_cast<PCB_TRACK*>( i ) )
            {
                const std::unique_ptr<LIST_ITEM>& list_item = *r.value();
                int                               len = track->GetLength();

                list_item->AddLayerWireLength( len, static_cast<int>( track->GetLayer() ) );

                if( track->Type() == PCB_VIA_T )
                {
                    list_item->AddViaCount( 1 );
                    list_item->AddViaLength( calculateViaLength( track ) );
                }

                updateDisplayedRowValues( r );
                return;
            }
        }

        // resort to generic slower net update otherwise.
        updateNet( i->GetNet() );
    }
    else if( FOOTPRINT* footprint = dynamic_cast<FOOTPRINT*>( aBoardItem ) )
    {
        for( const PAD* pad : footprint->Pads() )
        {
            std::optional<LIST_ITEM_ITER> r = m_data_model->findItem( pad->GetNet() );

            if( !r )
            {
                // if show-zero-pads is off, we might not have this net
                // in our list yet, so add it first.
                // notice that at this point we are very certain that this net
                // will have at least one pad.

                if( netFilterMatches( pad->GetNet() ) )
                    r = m_data_model->addItem( std::make_unique<LIST_ITEM>( pad->GetNet() ) );
            }

            if( r )
            {
                const std::unique_ptr<LIST_ITEM>& list_item = *r.value();
                int                               len = pad->GetPadToDieLength();

                list_item->AddPadCount( 1 );
                list_item->AddPadDieLength( len );

                if( list_item->GetPadCount() == 0 && !m_show_zero_pad_nets )
                    m_data_model->deleteItem( r );
                else
                    updateDisplayedRowValues( r );
            }
        }
    }
}


void PCB_NET_INSPECTOR_PANEL::OnBoardItemsAdded( BOARD&                    aBoard,
                                                 std::vector<BOARD_ITEM*>& aBoardItems )
{
    if( !IsShownOnScreen() )
        return;

    // Rebuild full netlist for large changes
    if( aBoardItems.size() > 25 )
    {
        buildNetsList();
        m_netsList->Refresh();
    }
    else
    {
        for( BOARD_ITEM* item : aBoardItems )
        {
            OnBoardItemAdded( aBoard, item );
        }
    }
}


void PCB_NET_INSPECTOR_PANEL::OnBoardItemRemoved( BOARD& aBoard, BOARD_ITEM* aBoardItem )
{
    if( !IsShownOnScreen() )
        return;

    if( NETINFO_ITEM* net = dynamic_cast<NETINFO_ITEM*>( aBoardItem ) )
    {
        m_data_model->deleteItem( m_data_model->findItem( net ) );
    }
    else if( FOOTPRINT* footprint = dynamic_cast<FOOTPRINT*>( aBoardItem ) )
    {
        for( const PAD* pad : footprint->Pads() )
        {
            std::optional<LIST_ITEM_ITER> r = m_data_model->findItem( pad->GetNet() );

            if( r )
            {
                const std::unique_ptr<LIST_ITEM>& list_item = *r.value();
                int                               len = pad->GetPadToDieLength();

                list_item->SubPadCount( 1 );
                list_item->SubPadDieLength( len );

                if( list_item->GetPadCount() == 0 && !m_show_zero_pad_nets )
                    m_data_model->deleteItem( r );
                else
                    updateDisplayedRowValues( r );
            }
        }
    }
    else if( BOARD_CONNECTED_ITEM* i = dynamic_cast<BOARD_CONNECTED_ITEM*>( aBoardItem ) )
    {
        std::optional<LIST_ITEM_ITER> r = m_data_model->findItem( i->GetNet() );

        if( r )
        {
            // try to handle frequent operations quickly.
            if( PCB_TRACK* track = dynamic_cast<PCB_TRACK*>( i ) )
            {
                const std::unique_ptr<LIST_ITEM>& list_item = *r.value();
                int                               len = track->GetLength();

                list_item->SubLayerWireLength( len, static_cast<int>( track->GetLayer() ) );

                if( track->Type() == PCB_VIA_T )
                {
                    list_item->SubViaCount( 1 );
                    list_item->SubViaLength( calculateViaLength( track ) );
                }

                updateDisplayedRowValues( r );
                return;
            }

            // resort to generic slower net update otherwise.
            updateNet( i->GetNet() );
        }
    }
}


void PCB_NET_INSPECTOR_PANEL::OnBoardItemsRemoved( BOARD&                    aBoard,
                                                   std::vector<BOARD_ITEM*>& aBoardItems )
{
    if( !IsShownOnScreen() )
        return;

    if( aBoardItems.size() > 25 )
    {
        buildNetsList();
        m_netsList->Refresh();
    }
    else
    {
        for( BOARD_ITEM* item : aBoardItems )
        {
            OnBoardItemRemoved( aBoard, item );
        }
    }
}


void PCB_NET_INSPECTOR_PANEL::OnBoardNetSettingsChanged( BOARD& aBoard )
{
    if( !IsShownOnScreen() )
        return;

    buildNetsList();
    m_netsList->Refresh();
}


void PCB_NET_INSPECTOR_PANEL::OnBoardItemChanged( BOARD& aBoard, BOARD_ITEM* aBoardItem )
{
    if( !IsShownOnScreen() )
        return;

    if( dynamic_cast<BOARD_CONNECTED_ITEM*>( aBoardItem ) != nullptr
        || dynamic_cast<FOOTPRINT*>( aBoardItem ) != nullptr )
    {
        buildNetsList();
        m_netsList->Refresh();
    }
}


void PCB_NET_INSPECTOR_PANEL::OnBoardItemsChanged( BOARD&                    aBoard,
                                                   std::vector<BOARD_ITEM*>& aBoardItems )
{
    if( !IsShownOnScreen() )
        return;

    buildNetsList();
    m_netsList->Refresh();
}


void PCB_NET_INSPECTOR_PANEL::OnBoardCompositeUpdate( BOARD&                    aBoard,
                                                      std::vector<BOARD_ITEM*>& aAddedItems,
                                                      std::vector<BOARD_ITEM*>& aRemovedItems,
                                                      std::vector<BOARD_ITEM*>& aDeletedItems )
{
    if( !IsShownOnScreen() )
        return;

    buildNetsList();
    m_netsList->Refresh();
}


void PCB_NET_INSPECTOR_PANEL::OnBoardHighlightNetChanged( BOARD& aBoard )
{
    if( m_highlighting_nets || !IsShownOnScreen() )
        return;

    if( !m_brd->IsHighLightNetON() )
    {
        m_netsList->UnselectAll();
    }
    else
    {
        const std::set<int>& selected_codes = m_brd->GetHighLightNetCodes();

        wxDataViewItemArray new_selection;
        new_selection.Alloc( selected_codes.size() );

        for( int code : selected_codes )
        {
            if( std::optional<LIST_ITEM_ITER> r = m_data_model->findItem( code ) )
                new_selection.Add( wxDataViewItem( &***r ) );
        }

        m_netsList->SetSelections( new_selection );

        if( !new_selection.IsEmpty() )
            m_netsList->EnsureVisible( new_selection.Item( 0 ) );
    }
}


/*****************************************************************************************
 * 
 * UI-generated event handling
 * 
 * ***************************************************************************************/

void PCB_NET_INSPECTOR_PANEL::OnShowPanel()
{
    buildNetsList();
    OnBoardHighlightNetChanged( *m_brd );
}


void PCB_NET_INSPECTOR_PANEL::OnNetsListContextMenu( wxDataViewEvent& event )
{
    bool             multipleSelections = false;
    const LIST_ITEM* selItem = nullptr;

    if( m_netsList->GetSelectedItemsCount() == 1 )
    {
        selItem = static_cast<const LIST_ITEM*>( m_netsList->GetSelection().GetID() );
    }
    else
    {
        if( m_netsList->GetSelectedItemsCount() > 1 )
            multipleSelections = true;
    }

    wxMenu menu;

    // Net edit menu items
    wxMenuItem* highlightNet = new wxMenuItem( &menu, ID_HIGHLIGHT_SELECTED_NETS,
                                               _( "Highlight Selected Net" ),
                                               wxEmptyString, wxITEM_NORMAL );
    menu.Append( highlightNet );

    wxMenuItem* clearHighlighting = new wxMenuItem( &menu, ID_CLEAR_HIGHLIGHTING,
                                                    _( "Clear Net Highlighting" ),
                                                    wxEmptyString, wxITEM_NORMAL );
    menu.Append( clearHighlighting );

    RENDER_SETTINGS* renderSettings = m_frame->GetCanvas()->GetView()->GetPainter()->GetSettings();
    const std::set<int>& selected_codes = renderSettings->GetHighlightNetCodes();

    if( selected_codes.size() == 0 )
        clearHighlighting->Enable( false );

    menu.AppendSeparator();

    wxMenuItem* renameNet = new wxMenuItem( &menu, ID_RENAME_NET, _( "Rename Selected Net" ),
                                            wxEmptyString, wxITEM_NORMAL );
    menu.Append( renameNet );

    wxMenuItem* deleteNet = new wxMenuItem( &menu, ID_DELETE_NET, _( "Delete Selected Net" ),
                                            wxEmptyString, wxITEM_NORMAL );
    menu.Append( deleteNet );

    menu.AppendSeparator();

    wxMenuItem* addNet =
            new wxMenuItem( &menu, ID_ADD_NET, _( "Add Net" ), wxEmptyString, wxITEM_NORMAL );
    menu.Append( addNet );

    if( !selItem && !multipleSelections )
    {
        highlightNet->Enable( false );
        deleteNet->Enable( false );
        renameNet->Enable( false );
    }
    else
    {
        if( multipleSelections || selItem->GetIsGroup() )
        {
            highlightNet->SetItemLabel( _( "Highlight Selected Nets" ) );
            renameNet->Enable( false );
            deleteNet->SetItemLabel( _( "Delete Selected Nets" ) );
        }
    }

    menu.AppendSeparator();

    wxMenuItem* removeSelectedGroup =
            new wxMenuItem( &menu, ID_REMOVE_SELECTED_GROUP, _( "Remove Selected Custom Group" ),
                            wxEmptyString, wxITEM_NORMAL );
    menu.Append( removeSelectedGroup );

    if( !selItem || !selItem->GetIsGroup() )
    {
        removeSelectedGroup->Enable( false );
    }

    menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &PCB_NET_INSPECTOR_PANEL::onSettingsMenu, this );

    PopupMenu( &menu );
}


void PCB_NET_INSPECTOR_PANEL::OnSearchTextChanged( wxCommandEvent& event )
{
    SaveSettings();
    buildNetsList();
}


void PCB_NET_INSPECTOR_PANEL::onAddGroup()
{
    wxString          newGroupName;
    NETNAME_VALIDATOR validator( &newGroupName );

    WX_TEXT_ENTRY_DIALOG dlg( this, _( "Group name / pattern:" ), _( "New Group" ), newGroupName );
    wxStaticText* help = new wxStaticText( &dlg, wxID_ANY,
                                           _( "(Use /.../ to indicate a regular expression.)" ) );
    help->SetFont( KIUI::GetInfoFont( this ).Italic() );
   	dlg.m_ContentSizer->Add( help, 0, wxALL|wxEXPAND, 5 );
    dlg.SetTextValidator( validator );
    dlg.GetSizer()->SetSizeHints( &dlg );

    if( dlg.ShowModal() != wxID_OK || dlg.GetValue().IsEmpty() )
        return; //Aborted by user

    newGroupName = UnescapeString( dlg.GetValue() );

    if( newGroupName == "" )
        return;

    if( std::find_if( m_custom_group_rules.begin(), m_custom_group_rules.end(),
                      [&]( std::unique_ptr<EDA_COMBINED_MATCHER>& rule )
                      {
                          return rule->GetPattern().Upper() == newGroupName.Upper();
                      } ) == m_custom_group_rules.end() )
    {
        m_custom_group_rules.push_back( std::make_unique<EDA_COMBINED_MATCHER>( newGroupName,
                                                                                CTX_NET ) );
        SaveSettings();
    }

    buildNetsList();
}


void PCB_NET_INSPECTOR_PANEL::onClearHighlighting()
{
    m_highlighting_nets = true;

    m_frame->GetCanvas()->GetView()->GetPainter()->GetSettings()->SetHighlight( false );
    m_frame->GetCanvas()->GetView()->UpdateAllLayersColor();
    m_frame->GetCanvas()->Refresh();

    m_highlighting_nets = false;
}


void PCB_NET_INSPECTOR_PANEL::OnExpandCollapseRow( wxCommandEvent& event )
{
    if( !m_row_expanding )
        SaveSettings();
}


void PCB_NET_INSPECTOR_PANEL::OnHeaderContextMenu( wxCommandEvent& event )
{
    wxMenu menu;
    generateShowHideColumnMenu( &menu );
    menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &PCB_NET_INSPECTOR_PANEL::onSettingsMenu, this );
    PopupMenu( &menu );
}


void PCB_NET_INSPECTOR_PANEL::OnConfigButton( wxCommandEvent& event )
{
    PROJECT_LOCAL_SETTINGS& localSettings = Pgm().GetSettingsManager().Prj().GetLocalSettings();
    auto&                   cfg = localSettings.m_NetInspectorPanel;

    const LIST_ITEM* selItem = nullptr;

    if( m_netsList->GetSelectedItemsCount() == 1 )
    {
        selItem = static_cast<const LIST_ITEM*>( m_netsList->GetSelection().GetID() );
    }

    wxMenu menu;

    // Filtering menu items
    wxMenuItem* filterByNetName = new wxMenuItem( &menu, ID_FILTER_BY_NET_NAME,
                                                  _( "Filter by Net Name" ),
                                                  wxEmptyString, wxITEM_CHECK );
    filterByNetName->Check( cfg.filter_by_net_name );
    menu.Append( filterByNetName );

    wxMenuItem* filterByNetclass = new wxMenuItem( &menu, ID_FILTER_BY_NETCLASS,
                                                   _( "Filter by Netclass" ),
                                                   wxEmptyString, wxITEM_CHECK );
    filterByNetclass->Check( cfg.filter_by_netclass );
    menu.Append( filterByNetclass );

    menu.AppendSeparator();

    // Grouping menu items
    //wxMenuItem* groupConstraint =
    //        new wxMenuItem( &menu, ID_GROUP_BY_CONSTRAINT, _( "Group by DRC Constraint" ),
    //                        wxEmptyString, wxITEM_CHECK );
    //groupConstraint->Check( m_group_by_constraint );
    //menu.Append( groupConstraint );

    wxMenuItem* groupNetclass = new wxMenuItem(
            &menu, ID_GROUP_BY_NETCLASS, _( "Group by Netclass" ), wxEmptyString, wxITEM_CHECK );
    groupNetclass->Check( m_group_by_netclass );
    menu.Append( groupNetclass );

    menu.AppendSeparator();

    wxMenuItem* addGroup = new wxMenuItem( &menu, ID_ADD_GROUP, _( "Add Custom Group" ),
                                           wxEmptyString, wxITEM_NORMAL );
    menu.Append( addGroup );

    wxMenuItem* removeSelectedGroup = new wxMenuItem( &menu, ID_REMOVE_SELECTED_GROUP,
                                                      _( "Remove Selected Custom Group" ),
                                                      wxEmptyString, wxITEM_NORMAL );
    menu.Append( removeSelectedGroup );

    if( !selItem || !selItem->GetIsGroup() )
    {
        removeSelectedGroup->Enable( false );
    }

    wxMenuItem* removeCustomGroups = new wxMenuItem( &menu, ID_REMOVE_GROUPS,
                                                     _( "Remove All Custom Groups" ),
                                                     wxEmptyString, wxITEM_NORMAL );
    menu.Append( removeCustomGroups );
    removeCustomGroups->Enable( m_custom_group_rules.size() != 0 );

    menu.AppendSeparator();

    wxMenuItem* showZeroNetPads = new wxMenuItem( &menu, ID_SHOW_ZERO_NET_PADS,
                                                  _( "Show Zero Pad Nets" ),
                                                  wxEmptyString, wxITEM_CHECK );
    showZeroNetPads->Check( m_show_zero_pad_nets );
    menu.Append( showZeroNetPads );

    wxMenuItem* showUnconnectedNets = new wxMenuItem( &menu, ID_SHOW_UNCONNECTED_NETS,
                                                      _( "Show Unconnected Nets" ),
                                                      wxEmptyString, wxITEM_CHECK );
    showUnconnectedNets->Check( m_show_unconnected_nets );
    menu.Append( showUnconnectedNets );

    menu.AppendSeparator();

    // Report generation
    wxMenuItem* generateReport = new wxMenuItem( &menu, ID_GENERATE_REPORT,
                                                 _( "Save Net Inspector Report" ),
                                                 wxEmptyString, wxITEM_NORMAL );
    menu.Append( generateReport );

    menu.AppendSeparator();

    // Show / hide columns menu items
    wxMenu* colsMenu = new wxMenu();
    generateShowHideColumnMenu( colsMenu );
    menu.AppendSubMenu( colsMenu, _( "Show / Hide Columns" ) );

    menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &PCB_NET_INSPECTOR_PANEL::onSettingsMenu, this );

    PopupMenu( &menu );
}


void PCB_NET_INSPECTOR_PANEL::generateShowHideColumnMenu( wxMenu* target )
{
    for( int i = 1; i <= COLUMN_LAST_STATIC_COL; ++i )
    {
        wxMenuItem* opt = new wxMenuItem( target, ID_HIDE_COLUMN + i, m_columns[i].display_name,
                                          wxEmptyString, wxITEM_CHECK );
        wxDataViewColumn* col = getDisplayedColumnForModelField( i );
        target->Append( opt );
        opt->Check( !col->IsHidden() );
    }

    target->AppendSeparator();

    for( std::size_t i = COLUMN_LAST_STATIC_COL + 1; i < m_columns.size(); ++i )
    {
        wxMenuItem* opt = new wxMenuItem( target, ID_HIDE_COLUMN + i, m_columns[i].display_name,
                                          wxEmptyString, wxITEM_CHECK );
        wxDataViewColumn* col = getDisplayedColumnForModelField( i );
        target->Append( opt );
        opt->Check( !col->IsHidden() );
    }
}


void PCB_NET_INSPECTOR_PANEL::onSettingsMenu( wxCommandEvent& event )
{
    bool saveAndRebuild = true;

    switch( event.GetId() )
    {
    case ID_ADD_NET:
        onAddNet();
        break;

    case ID_RENAME_NET:
        onRenameSelectedNet();
        break;

    case ID_DELETE_NET:
        onDeleteSelectedNet();
        break;

    case ID_ADD_GROUP:
        onAddGroup();
        break;

    case ID_GROUP_BY_CONSTRAINT:
        m_group_by_constraint = !m_group_by_constraint;
        break;

    case ID_GROUP_BY_NETCLASS:
        m_group_by_netclass = !m_group_by_netclass;
        break;

    case ID_FILTER_BY_NET_NAME:
        m_filter_by_net_name = !m_filter_by_net_name;
        break;

    case ID_FILTER_BY_NETCLASS:
        m_filter_by_netclass = !m_filter_by_netclass;
        break;

    case ID_REMOVE_SELECTED_GROUP:
        onRemoveSelectedGroup();
        break;

    case ID_REMOVE_GROUPS:
        m_custom_group_rules.clear();
        break;

    case ID_SHOW_ZERO_NET_PADS:
        m_show_zero_pad_nets = !m_show_zero_pad_nets;
        break;

    case ID_SHOW_UNCONNECTED_NETS:
        m_show_unconnected_nets = !m_show_unconnected_nets;
        break;

    case ID_GENERATE_REPORT:
        generateReport();
        saveAndRebuild = false;
        break;

    case ID_HIGHLIGHT_SELECTED_NETS:
        highlightSelectedNets();
        saveAndRebuild = false;
        break;

    case ID_CLEAR_HIGHLIGHTING:
        onClearHighlighting();
        saveAndRebuild = false;
        break;

    default:
        if( event.GetId() >= ID_HIDE_COLUMN )
        {
            const int         columnId = event.GetId() - ID_HIDE_COLUMN;
            wxDataViewColumn* col = getDisplayedColumnForModelField( columnId );
            // Make sure we end up with something non-zero so we can resize it
            col->SetWidth( std::max( col->GetWidth(), 10 ) );
            col->SetHidden( !col->IsHidden() );
        }
        break;
    }

    if( saveAndRebuild )
    {
        SaveSettings();
        buildNetsList();
    }
}


void PCB_NET_INSPECTOR_PANEL::onRemoveSelectedGroup()
{
    if( m_netsList->GetSelectedItemsCount() == 1 )
    {
        auto* selItem = static_cast<const LIST_ITEM*>( m_netsList->GetSelection().GetID() );

        if( selItem->GetIsGroup() )
        {
            wxString groupName = selItem->GetGroupName();
            auto groupIter = std::find_if( m_custom_group_rules.begin(), m_custom_group_rules.end(),
                                           [&]( std::unique_ptr<EDA_COMBINED_MATCHER>& rule )
                                           {
                                               return rule->GetPattern() == groupName;
                                           } );

            if( groupIter != m_custom_group_rules.end() )
            {
                m_custom_group_rules.erase( groupIter );
                SaveSettings();
                buildNetsList();
            }
        }
    }
}


void PCB_NET_INSPECTOR_PANEL::generateReport()
{
    wxFileDialog dlg( this, _( "Save Net Inspector Report File" ), "", "",
                      _( "Report file" ) + AddFileExtListToFilter( { "csv" } ),
                      wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    wxTextFile f( dlg.GetPath() );

    f.Create();

    wxString txt;

    m_in_reporting = true;

    // Print Header:
    for( auto&& col : m_columns )
    {
        txt += '"';

        if( col.has_units )
        {
            txt += wxString::Format( _( "%s (%s)" ),
                                     col.csv_name,
                                     EDA_UNIT_UTILS::GetLabel( m_frame->GetUserUnits() ) );
        }
        else
        {
            txt += col.csv_name;
        }

        txt += wxT( "\";" );
    }

    f.AddLine( txt );

    // Print list of nets:
    const unsigned int num_rows = m_data_model->itemCount();

    for( unsigned int row = 0; row < num_rows; row++ )
    {
        auto& i = m_data_model->itemAt( row );

        if( i.GetIsGroup() || i.GetNetCode() == 0 )
            continue;

        txt = "";

        for( auto&& col : m_columns )
        {
            if( static_cast<int>( col.csv_flags ) & static_cast<int>( CSV_COLUMN_DESC::CSV_QUOTE ) )
                txt += '"' + m_data_model->valueAt( col.num, row ).GetString() + wxT( "\";" );
            else
                txt += m_data_model->valueAt( col.num, row ).GetString() + ';';
        }

        f.AddLine( txt );
    }

    m_in_reporting = false;

    f.Write();
    f.Close();
}


void PCB_NET_INSPECTOR_PANEL::OnNetsListItemActivated( wxDataViewEvent& event )
{
    highlightSelectedNets();
}


void PCB_NET_INSPECTOR_PANEL::highlightSelectedNets()
{
    // ignore selection changes while the whole list is being rebuilt.
    if( m_in_build_nets_list )
        return;

    m_highlighting_nets = true;

    RENDER_SETTINGS* renderSettings = m_frame->GetCanvas()->GetView()->GetPainter()->GetSettings();

    if( m_netsList->HasSelection() )
    {
        wxDataViewItemArray sel;
        m_netsList->GetSelections( sel );

        renderSettings->SetHighlight( false );

        for( unsigned int i = 0; i < sel.GetCount(); ++i )
        {
            const LIST_ITEM* ii = static_cast<const LIST_ITEM*>( sel.Item( i ).GetID() );

            if( ii->GetIsGroup() )
            {
                for( auto c = ii->ChildrenBegin(), end = ii->ChildrenEnd(); c != end; ++c )
                    renderSettings->SetHighlight( true, ( *c )->GetNetCode(), true );
            }
            else
            {
                renderSettings->SetHighlight( true, ii->GetNetCode(), true );
            }
        }
    }
    else
    {
        renderSettings->SetHighlight( false );
    }

    m_frame->GetCanvas()->GetView()->UpdateAllLayersColor();
    m_frame->GetCanvas()->Refresh();

    m_highlighting_nets = false;
}


void PCB_NET_INSPECTOR_PANEL::OnColumnSorted( wxDataViewEvent& event )
{
    SaveSettings();
}


void PCB_NET_INSPECTOR_PANEL::OnParentSetupChanged()
{
    // Rebuilt the nets list, and force rebuild of columns in case the stackup has chanaged
    buildNetsList( true );
    m_netsList->Refresh();
}


void PCB_NET_INSPECTOR_PANEL::onAddNet()
{
    wxString          newNetName;
    NETNAME_VALIDATOR validator( &newNetName );

    WX_TEXT_ENTRY_DIALOG dlg( this, _( "Net name:" ), _( "New Net" ), newNetName );
    dlg.SetTextValidator( validator );

    while( true )
    {
        if( dlg.ShowModal() != wxID_OK || dlg.GetValue().IsEmpty() )
            return; //Aborted by user

        newNetName = dlg.GetValue();

        if( m_brd->FindNet( newNetName ) )
        {
            DisplayError( this,
                          wxString::Format( _( "Net name '%s' is already in use." ), newNetName ) );
            newNetName = wxEmptyString;
        }
        else
        {
            break;
        }
    }

    NETINFO_ITEM* newnet = new NETINFO_ITEM( m_brd, dlg.GetValue(), 0 );

    m_brd->Add( newnet );

    // We'll get an OnBoardItemAdded callback from this to update our listbox
    m_frame->OnModify();
}


void PCB_NET_INSPECTOR_PANEL::onRenameSelectedNet()
{
    if( m_netsList->GetSelectedItemsCount() == 1 )
    {
        const LIST_ITEM* sel = static_cast<const LIST_ITEM*>( m_netsList->GetSelection().GetID() );

        if( sel->GetIsGroup() )
            return;

        NETINFO_ITEM* net = sel->GetNet();
        wxString      fullNetName = net->GetNetname();
        wxString      netPath;
        wxString      shortNetName;

        if( fullNetName.Contains( wxT( "/" ) ) )
        {
            netPath = fullNetName.BeforeLast( '/' ) + '/';
            shortNetName = fullNetName.AfterLast( '/' );
        }
        else
        {
            shortNetName = fullNetName;
        }

        wxString unescapedShortName = UnescapeString( shortNetName );

        WX_TEXT_ENTRY_DIALOG dlg( this, _( "Net name:" ), _( "Rename Net" ), unescapedShortName );
        NETNAME_VALIDATOR    validator( &unescapedShortName );
        dlg.SetTextValidator( validator );

        while( true )
        {
            if( dlg.ShowModal() != wxID_OK || dlg.GetValue() == unescapedShortName )
                return;

            unescapedShortName = dlg.GetValue();

            if( unescapedShortName.IsEmpty() )
            {
                DisplayError( this, wxString::Format( _( "Net name cannot be empty." ),
                                                      unescapedShortName ) );
                continue;
            }

            shortNetName = EscapeString( unescapedShortName, CTX_NETNAME );
            fullNetName = netPath + shortNetName;

            if( m_brd->FindNet( shortNetName ) || m_brd->FindNet( fullNetName ) )
            {
                DisplayError( this, wxString::Format( _( "Net name '%s' is already in use." ),
                                                      unescapedShortName ) );
                unescapedShortName = wxEmptyString;
            }
            else
            {
                break;
            }
        }

        for( BOARD_CONNECTED_ITEM* boardItem : m_frame->GetBoard()->AllConnectedItems() )
        {
            if( boardItem->GetNet() == net )
                boardItem->SetFlags( CANDIDATE );
            else
                boardItem->ClearFlags( CANDIDATE );
        }

        // the changed name might require re-grouping.  remove/re-insert is easier.
        auto removed_item = m_data_model->deleteItem( m_data_model->findItem( net ) );

        m_brd->Remove( net );
        net->SetNetname( fullNetName );
        m_brd->Add( net );

        for( BOARD_CONNECTED_ITEM* boardItem : m_frame->GetBoard()->AllConnectedItems() )
        {
            if( boardItem->GetFlags() & CANDIDATE )
                boardItem->SetNet( net );
        }

        buildNetsList();

        if( std::optional<LIST_ITEM_ITER> r = m_data_model->findItem( net ) )
            m_netsList->Select( wxDataViewItem( r.value()->get() ) );

        m_frame->OnModify();

        // Currently only tracks and pads have netname annotations and need to be redrawn,
        // but zones are likely to follow.  Since we don't have a way to ask what is current,
        // just refresh all items.
        m_frame->GetCanvas()->GetView()->UpdateAllItems( KIGFX::REPAINT );
        m_frame->GetCanvas()->Refresh();
    }
}


void PCB_NET_INSPECTOR_PANEL::onDeleteSelectedNet()
{
    if( !m_netsList->HasSelection() )
        return;

    wxDataViewItemArray sel;
    m_netsList->GetSelections( sel );

    auto delete_one = [this]( const LIST_ITEM* i )
    {
        if( i->GetPadCount() == 0
            || IsOK( this, wxString::Format( _( "Net '%s' is in use.  Delete anyway?" ),
                                             i->GetNetName() ) ) )
        {
            // This is a bit hacky, but it will do for now, since this is the only path
            // outside the netlist updater where you can remove a net from a BOARD.
            int removedCode = i->GetNetCode();

            m_frame->GetCanvas()->GetView()->UpdateAllItemsConditionally(
                    [removedCode]( KIGFX::VIEW_ITEM* aItem ) -> int
                    {
                        auto boardItem = dynamic_cast<BOARD_CONNECTED_ITEM*>( aItem );

                        if( boardItem && boardItem->GetNetCode() == removedCode )
                            return KIGFX::REPAINT;

                        EDA_TEXT* text = dynamic_cast<EDA_TEXT*>( aItem );

                        if( text && text->HasTextVars() )
                        {
                            text->ClearRenderCache();
                            text->ClearBoundingBoxCache();
                            return KIGFX::GEOMETRY | KIGFX::REPAINT;
                        }

                        return 0;
                    } );

            m_brd->Remove( i->GetNet() );
            m_frame->OnModify();

            // We'll get an OnBoardItemRemoved callback from this to update our listbox
        }
    };

    for( unsigned int i = 0; i < sel.GetCount(); ++i )
    {
        const LIST_ITEM* ii = static_cast<const LIST_ITEM*>( sel.Item( i ).GetID() );

        if( ii->GetIsGroup() )
        {
            if( ii->ChildrenCount() != 0
                && IsOK( this, wxString::Format( _( "Delete all nets in group '%s'?" ),
                                                 ii->GetGroupName() ) ) )
            {
                // we can't be iterating the children container and deleting items from
                // it at the same time.  thus take a copy of it first.
                std::vector<const LIST_ITEM*> children;
                children.reserve( ii->ChildrenCount() );
                std::copy( ii->ChildrenBegin(), ii->ChildrenEnd(), std::back_inserter( children ) );

                for( const LIST_ITEM* c : children )
                    delete_one( c );
            }
        }
        else
        {
            delete_one( ii );
        }
    }
}

/*****************************************************************************************
 * 
 * Application-generated event handling
 * 
 * ***************************************************************************************/


void PCB_NET_INSPECTOR_PANEL::OnLanguageChangedImpl()
{
    SaveSettings();
    buildNetsList( true );
    m_data_model->updateAllItems();
}


void PCB_NET_INSPECTOR_PANEL::onUnitsChanged( wxCommandEvent& event )
{
    m_data_model->updateAllItems();
    event.Skip();
}


/*****************************************************************************************
 * 
 * Settings persistence
 * 
 * ***************************************************************************************/


void PCB_NET_INSPECTOR_PANEL::SaveSettings()
{
    // Don't save settings if a board has not yet been loaded - events fire while we set up the
    // panel which overwrites the settings we haven't yet loaded
    if( !m_board_loaded || m_board_loading )
        return;

    PROJECT_LOCAL_SETTINGS& localSettings = Pgm().GetSettingsManager().Prj().GetLocalSettings();
    auto&                   cfg = localSettings.m_NetInspectorPanel;

    // User-defined filters / grouping
    cfg.filter_text = m_searchCtrl->GetValue();
    cfg.filter_by_net_name = m_filter_by_net_name;
    cfg.filter_by_netclass = m_filter_by_netclass;
    cfg.group_by_netclass = m_group_by_netclass;
    cfg.group_by_constraint = m_group_by_constraint;
    cfg.show_zero_pad_nets = m_show_zero_pad_nets;
    cfg.show_unconnected_nets = m_show_unconnected_nets;

    // Grid sorting
    wxDataViewColumn* sortingCol = m_netsList->GetSortingColumn();
    cfg.sorting_column = sortingCol ? static_cast<int>( sortingCol->GetModelColumn() ) : -1;
    cfg.sort_order_asc = sortingCol ? sortingCol->IsSortOrderAscending() : true;

    // Column arrangement / sizes
    cfg.col_order.resize( m_data_model->columnCount() );
    cfg.col_widths.resize( m_data_model->columnCount() );
    cfg.col_hidden.resize( m_data_model->columnCount() );

    for( unsigned int ii = 0; ii < m_data_model->columnCount(); ++ii )
    {
        cfg.col_order[ii] = (int) m_netsList->GetColumn( ii )->GetModelColumn();
        cfg.col_widths[ii] = m_netsList->GetColumn( ii )->GetWidth();
        cfg.col_hidden[ii] = m_netsList->GetColumn( ii )->IsHidden();
    }

    // Expanded rows
    cfg.expanded_rows.clear();
    std::vector<std::pair<wxString, wxDataViewItem>> groupItems =
            m_data_model->getGroupDataViewItems();

    for( std::pair<wxString, wxDataViewItem>& item : groupItems )
    {
        if( m_netsList->IsExpanded( item.second ) )
            cfg.expanded_rows.push_back( item.first );
    }

    // Customer group rules
    cfg.custom_group_rules.clear();

    for( const std::unique_ptr<EDA_COMBINED_MATCHER>& rule : m_custom_group_rules )
        cfg.custom_group_rules.push_back( rule->GetPattern() );
}
