/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <advanced_config.h>
#include <board_design_settings.h>
#include <confirm.h>
#include <connectivity/connectivity_algo.h>
#include <dialogs/dialog_text_entry.h>
#include <footprint.h>
#include <length_delay_calculation/length_delay_calculation.h>
#include <length_delay_calculation/length_delay_calculation_item.h>
#include <pad.h>
#include <pcb_edit_frame.h>
#include <pcb_painter.h>
#include <pgm_base.h>
#include <settings/settings_manager.h>
#include <string_utils.h>
#include <validators.h>
#include <wildcards_and_files_ext.h>
#include <eda_pattern_match.h>

#include <wx/wupdlock.h>
#include <wx/filedlg.h>

#include <algorithm>
#include <thread_pool.h>

PCB_NET_INSPECTOR_PANEL::PCB_NET_INSPECTOR_PANEL( wxWindow* parent, PCB_EDIT_FRAME* aFrame ) :
        NET_INSPECTOR_PANEL( parent, aFrame ),
        m_frame( aFrame ),
        m_dataModel( new DATA_MODEL( *this ) )
{
    m_board = m_frame->GetBoard();

    m_netsList->AssociateModel( &*m_dataModel );

    // Rebuild nets list
    buildNetsList( true );

    // Register the panel to receive board change notifications
    if( m_board != nullptr )
    {
        PCB_NET_INSPECTOR_PANEL::OnBoardHighlightNetChanged( *m_board );
        m_board->AddListener( this );
    }

    // Connect to board events
    m_frame->Bind( EDA_EVT_UNITS_CHANGED, &PCB_NET_INSPECTOR_PANEL::onUnitsChanged, this );

    // Connect to wxDataViewCtrl events
    m_netsList->Bind( wxEVT_DATAVIEW_ITEM_EXPANDED, &PCB_NET_INSPECTOR_PANEL::OnExpandCollapseRow, this );
    m_netsList->Bind( wxEVT_DATAVIEW_ITEM_COLLAPSED, &PCB_NET_INSPECTOR_PANEL::OnExpandCollapseRow, this );
    m_netsList->Bind( wxEVT_DATAVIEW_COLUMN_HEADER_RIGHT_CLICK, &PCB_NET_INSPECTOR_PANEL::OnHeaderContextMenu, this );
    m_netsList->Bind( wxEVT_DATAVIEW_ITEM_CONTEXT_MENU, &PCB_NET_INSPECTOR_PANEL::OnNetsListContextMenu, this );
    m_netsList->Bind( wxEVT_DATAVIEW_ITEM_ACTIVATED, &PCB_NET_INSPECTOR_PANEL::OnNetsListItemActivated, this );
    m_netsList->Bind( wxEVT_DATAVIEW_COLUMN_SORTED, &PCB_NET_INSPECTOR_PANEL::OnColumnSorted, this );
}

PCB_NET_INSPECTOR_PANEL::~PCB_NET_INSPECTOR_PANEL()
{
    PCB_NET_INSPECTOR_PANEL::SaveSettings();

    m_netsList->AssociateModel( nullptr );

    // Disconnect from board events
    m_frame->Unbind( EDA_EVT_UNITS_CHANGED, &PCB_NET_INSPECTOR_PANEL::onUnitsChanged, this );

    // Connect to wxDataViewCtrl events
    m_netsList->Unbind( wxEVT_DATAVIEW_ITEM_EXPANDED, &PCB_NET_INSPECTOR_PANEL::OnExpandCollapseRow, this );
    m_netsList->Unbind( wxEVT_DATAVIEW_ITEM_COLLAPSED, &PCB_NET_INSPECTOR_PANEL::OnExpandCollapseRow, this );
    m_netsList->Unbind( wxEVT_DATAVIEW_COLUMN_HEADER_RIGHT_CLICK, &PCB_NET_INSPECTOR_PANEL::OnHeaderContextMenu, this );
    m_netsList->Unbind( wxEVT_DATAVIEW_ITEM_CONTEXT_MENU, &PCB_NET_INSPECTOR_PANEL::OnNetsListContextMenu, this );
    m_netsList->Unbind( wxEVT_DATAVIEW_ITEM_ACTIVATED, &PCB_NET_INSPECTOR_PANEL::OnNetsListItemActivated, this );
    m_netsList->Unbind( wxEVT_DATAVIEW_COLUMN_SORTED, &PCB_NET_INSPECTOR_PANEL::OnColumnSorted, this );
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
    m_columns.emplace_back( 0u, UNDEFINED_LAYER, _( "Name" ), _( "Net Name" ), CSV_COLUMN_DESC::CSV_QUOTE, false );
    m_columns.emplace_back( 1u, UNDEFINED_LAYER, _( "Netclass" ), _( "Netclass" ), CSV_COLUMN_DESC::CSV_QUOTE, false );

    if( m_showTimeDomainDetails )
    {
        m_columns.emplace_back( 2u, UNDEFINED_LAYER, _( "Total Delay" ), _( "Net Delay" ), CSV_COLUMN_DESC::CSV_NONE,
                                true );
    }
    else
    {
        m_columns.emplace_back( 2u, UNDEFINED_LAYER, _( "Total Length" ), _( "Net Length" ), CSV_COLUMN_DESC::CSV_NONE,
                                true );
    }

    m_columns.emplace_back( 3u, UNDEFINED_LAYER, _( "Via Count" ), _( "Via Count" ), CSV_COLUMN_DESC::CSV_NONE, false );

    if( m_showTimeDomainDetails )
    {
        m_columns.emplace_back( 4u, UNDEFINED_LAYER, _( "Via Delay" ), _( "Via Delay" ), CSV_COLUMN_DESC::CSV_NONE,
                                true );
        m_columns.emplace_back( 5u, UNDEFINED_LAYER, _( "Track Delay" ), _( "Track Delay" ), CSV_COLUMN_DESC::CSV_NONE,
                                true );
        m_columns.emplace_back( 6u, UNDEFINED_LAYER, _( "Die Delay" ), _( "Die Delay" ), CSV_COLUMN_DESC::CSV_NONE,
                                true );
    }
    else
    {
        m_columns.emplace_back( 4u, UNDEFINED_LAYER, _( "Via Length" ), _( "Via Length" ), CSV_COLUMN_DESC::CSV_NONE,
                                true );
        m_columns.emplace_back( 5u, UNDEFINED_LAYER, _( "Track Length" ), _( "Track Length" ),
                                CSV_COLUMN_DESC::CSV_NONE, true );
        m_columns.emplace_back( 6u, UNDEFINED_LAYER, _( "Die Length" ), _( "Die Length" ), CSV_COLUMN_DESC::CSV_NONE,
                                true );
    }

    m_columns.emplace_back( 7u, UNDEFINED_LAYER, _( "Pad Count" ), _( "Pad Count" ), CSV_COLUMN_DESC::CSV_NONE, false );

    const std::vector<std::function<void( void )>> add_col{
        [&]()
        {
            m_netsList->AppendTextColumn( m_columns[COLUMN_NAME].display_name, m_columns[COLUMN_NAME],
                                          wxDATAVIEW_CELL_INERT, -1, wxALIGN_LEFT,
                                          wxDATAVIEW_COL_RESIZABLE|wxDATAVIEW_COL_SORTABLE );
        },
        [&]()
        {
            m_netsList->AppendTextColumn( m_columns[COLUMN_NETCLASS].display_name, m_columns[COLUMN_NETCLASS],
                                          wxDATAVIEW_CELL_INERT, -1, wxALIGN_LEFT,
                                          wxDATAVIEW_COL_RESIZABLE|wxDATAVIEW_COL_REORDERABLE|wxDATAVIEW_COL_SORTABLE );
        },
        [&]()
        {
            m_netsList->AppendTextColumn( m_columns[COLUMN_TOTAL_LENGTH].display_name, m_columns[COLUMN_TOTAL_LENGTH],
                                          wxDATAVIEW_CELL_INERT, -1, wxALIGN_CENTER,
                                          wxDATAVIEW_COL_RESIZABLE|wxDATAVIEW_COL_REORDERABLE|wxDATAVIEW_COL_SORTABLE );
        },
        [&]()
        {
            m_netsList->AppendTextColumn( m_columns[COLUMN_VIA_COUNT].display_name, m_columns[COLUMN_VIA_COUNT],
                                          wxDATAVIEW_CELL_INERT, -1, wxALIGN_CENTER,
                                          wxDATAVIEW_COL_RESIZABLE|wxDATAVIEW_COL_REORDERABLE|wxDATAVIEW_COL_SORTABLE );
        },
        [&]()
        {
            m_netsList->AppendTextColumn( m_columns[COLUMN_VIA_LENGTH].display_name, m_columns[COLUMN_VIA_LENGTH],
                                          wxDATAVIEW_CELL_INERT, -1, wxALIGN_CENTER,
                                          wxDATAVIEW_COL_RESIZABLE|wxDATAVIEW_COL_REORDERABLE|wxDATAVIEW_COL_SORTABLE );
        },
        [&]()
        {
            m_netsList->AppendTextColumn( m_columns[COLUMN_BOARD_LENGTH].display_name, m_columns[COLUMN_BOARD_LENGTH],
                                          wxDATAVIEW_CELL_INERT, -1, wxALIGN_CENTER,
                                          wxDATAVIEW_COL_RESIZABLE|wxDATAVIEW_COL_REORDERABLE|wxDATAVIEW_COL_SORTABLE );
        },
        [&]()
        {
            m_netsList->AppendTextColumn( m_columns[COLUMN_PAD_DIE_LENGTH].display_name,
                                          m_columns[COLUMN_PAD_DIE_LENGTH], wxDATAVIEW_CELL_INERT, -1, wxALIGN_CENTER,
                                          wxDATAVIEW_COL_RESIZABLE|wxDATAVIEW_COL_REORDERABLE|wxDATAVIEW_COL_SORTABLE );
        },
        [&]()
        {
            m_netsList->AppendTextColumn( m_columns[COLUMN_PAD_COUNT].display_name, m_columns[COLUMN_PAD_COUNT],
                                          wxDATAVIEW_CELL_INERT, -1, wxALIGN_CENTER,
                                          wxDATAVIEW_COL_RESIZABLE|wxDATAVIEW_COL_REORDERABLE|wxDATAVIEW_COL_SORTABLE );
        }
    };

    // If we have not yet loaded the first board, use a dummy local settings object to ensure we
    // don't over-write existing board settings (note that PCB_EDIT_FRAME loads the local settings
    // object prior to loading the board; the two are not synced and we need to account for that)
    PANEL_NET_INSPECTOR_SETTINGS* cfg = nullptr;

    if( m_boardLoaded )
    {
        PROJECT_LOCAL_SETTINGS& localSettings = Pgm().GetSettingsManager().Prj().GetLocalSettings();
        cfg = &localSettings.m_NetInspectorPanel;
    }
    else
    {
        cfg = new PANEL_NET_INSPECTOR_SETTINGS();
    }

    // Reset the column display settings if column count doesn't match
    const int totalNumColumns = (int) add_col.size() + m_board->GetCopperLayerCount();

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
            cfg->col_order[i] = static_cast<int>( i );
    }

    // Add column records for copper layers
    for( PCB_LAYER_ID layer : m_board->GetEnabledLayers().Seq() )
    {
        if( !IsCopperLayer( layer ) )
            continue;

        m_columns.emplace_back( m_columns.size(), layer, m_board->GetLayerName( layer ), m_board->GetLayerName( layer ),
                                CSV_COLUMN_DESC::CSV_NONE, true );
    }

    // Add display columns in settings order
    for( const int i : cfg->col_order )
    {
        const int addModelColumn = i;

        if( addModelColumn >= (int) add_col.size() )
        {
            m_netsList->AppendTextColumn( m_board->GetLayerName( m_columns[addModelColumn].layer ),
                                          m_columns[addModelColumn], wxDATAVIEW_CELL_INERT, -1, wxALIGN_CENTER,
                                          wxDATAVIEW_COL_RESIZABLE|wxDATAVIEW_COL_REORDERABLE|wxDATAVIEW_COL_SORTABLE );
        }
        else
        {
            add_col.at( i )();
        }
    }

    // Set the name column as the expander row
    if( wxDataViewColumn* col = getDisplayedColumnForModelField( COLUMN_NAME ) )
    {
        m_netsList->SetExpanderColumn( col );
    }

    adjustListColumnSizes( cfg );

    // Delete the temporary config if used
    if( !m_boardLoaded )
        delete cfg;
}


void PCB_NET_INSPECTOR_PANEL::adjustListColumnSizes( PANEL_NET_INSPECTOR_SETTINGS* cfg ) const
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
        constexpr int margins = 15;
        constexpr int extra_width = 30;

        auto getTargetWidth =
                [&]( int columnID )
                {
                    switch( columnID )
                    {
                    case COLUMN_NAME:      return minNameWidth + extra_width;
                    case COLUMN_NETCLASS:  return minNameWidth + margins;
                    case COLUMN_VIA_COUNT: return minNumberWidth + margins;
                    case COLUMN_PAD_COUNT: return minNumberWidth + margins;
                    default:               return minValueWidth + margins;
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


bool PCB_NET_INSPECTOR_PANEL::restoreSortColumn( const int sortingColumnId, const bool sortOrderAsc ) const
{
    if( sortingColumnId != -1 )
    {
        if( wxDataViewColumn* col = getDisplayedColumnForModelField( sortingColumnId ) )
        {
            col->SetSortOrder( sortOrderAsc );
            m_dataModel->Resort();
            return true;
        }
    }

    return false;
}


wxDataViewColumn* PCB_NET_INSPECTOR_PANEL::getDisplayedColumnForModelField( const int columnId ) const
{
    for( unsigned int i = 0; i < m_netsList->GetColumnCount(); ++i )
    {
        wxDataViewColumn* col = m_netsList->GetColumn( i );

        if( static_cast<int>( col->GetModelColumn() ) == columnId )
            return col;
    }

    return nullptr;
}


/*****************************************************************************************
 *
 * Nets list generation
 *
 * ***************************************************************************************/


void PCB_NET_INSPECTOR_PANEL::buildNetsList( const bool rebuildColumns )
{
    // Only build the list of nets if there is a board present
    if( !m_board )
        return;

    m_inBuildNetsList = true;

    m_netsList->Freeze();

    m_dataModel->SetIsTimeDomain( m_showTimeDomainDetails );

    PROJECT_LOCAL_SETTINGS& localSettings = Pgm().GetSettingsManager().Prj().GetLocalSettings();
    PANEL_NET_INSPECTOR_SETTINGS* cfg = &localSettings.m_NetInspectorPanel;

    // Refresh all filtering / grouping settings
    m_filterByNetName = cfg->filter_by_net_name;
    m_filterByNetclass = cfg->filter_by_netclass;
    m_showZeroPadNets = cfg->show_zero_pad_nets;
    m_showTimeDomainDetails = cfg->show_time_domain_details;
    m_groupByNetclass = cfg->group_by_netclass;
    m_groupByConstraint = cfg->group_by_constraint;

    // Attempt to keep any expanded groups open
    if( m_boardLoaded && !m_boardLoading )
    {
        cfg->expanded_rows.clear();
        DATA_MODEL* model = static_cast<DATA_MODEL*>( m_netsList->GetModel() );

        for( const auto& [groupName, groupItem] : model->getGroupDataViewItems() )
        {
            if( m_netsList->IsExpanded( groupItem ) )
                cfg->expanded_rows.push_back( groupName );
        }
    }

    // When rebuilding the netlist, try to keep the row selection
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
        if( !m_boardLoading )
        {
            sorting_column_id = static_cast<int>( sorting_column->GetModelColumn() );
            sort_order_asc = sorting_column->IsSortOrderAscending();
        }

        // On GTK, wxDVC will crash if we rebuild with a sorting column set.
        sorting_column->UnsetAsSortKey();
    }

    if( rebuildColumns )
    {
        m_netsList->ClearColumns();
        buildColumns();
    }

    m_dataModel->deleteAllItems();
    m_custom_group_rules.clear();

    for( const wxString& rule : cfg->custom_group_rules )
        m_custom_group_rules.push_back( std::make_unique<EDA_COMBINED_MATCHER>( rule, CTX_NET ) );

    m_dataModel->addCustomGroups();

    std::vector<NETINFO_ITEM*> netCodes;

    for( NETINFO_ITEM* ni : m_board->GetNetInfo() )
    {
        if( netFilterMatches( ni, cfg ) )
            netCodes.emplace_back( ni );
    }

    std::ranges::sort( netCodes,
                       []( const NETINFO_ITEM* a, const NETINFO_ITEM* b )
                       {
                           return a->GetNetCode() < b->GetNetCode();
                       } );

    std::vector<std::unique_ptr<LIST_ITEM>> lengths = calculateNets( netCodes, m_showZeroPadNets );

    m_dataModel->addItems( lengths );

    // Try to re-enable the sorting column
    if( !restoreSortColumn( sorting_column_id, sort_order_asc ))
    {
        // By default, sort by Name column
        restoreSortColumn( COLUMN_NAME, true );
    }

    // Try to restore the expanded groups
    if( m_boardLoaded )
    {
        m_rowExpanding = true;

        std::vector<std::pair<wxString, wxDataViewItem>> groupItems = m_dataModel->getGroupDataViewItems();

        for( wxString& groupName : cfg->expanded_rows )
        {
            auto pred =
                    [&groupName]( const std::pair<wxString, wxDataViewItem>& item )
                    {
                        return groupName == item.first;
                    };

            auto tableItem = std::ranges::find_if( groupItems, pred );

            if( tableItem != groupItems.end() )
                m_netsList->Expand( tableItem->second );
        }

        m_rowExpanding = false;
    }

    // Try to restore the selected rows
    sel.Clear();

    for( const int& nc : prev_selected_netcodes )
    {
        if( std::optional<LIST_ITEM_ITER> r = m_dataModel->findItem( nc ) )
        {
            const std::unique_ptr<LIST_ITEM>& list_item = *r.value();
            sel.Add( wxDataViewItem( list_item.get() ) );
        }
    }

    m_netsList->Thaw(); // Must thaw before reselecting to avoid windows selection bug

    if( !sel.IsEmpty() )
    {
        m_netsList->SetSelections( sel );
        m_netsList->EnsureVisible( sel.Item( 0 ) );
    }
    else
    {
        m_netsList->UnselectAll();
    }

    m_inBuildNetsList = false;
}


bool PCB_NET_INSPECTOR_PANEL::netFilterMatches( NETINFO_ITEM*                 aNet,
                                                PANEL_NET_INSPECTOR_SETTINGS* cfg ) const
{
    if( cfg == nullptr )
    {
        PROJECT_LOCAL_SETTINGS& localSettings = Pgm().GetSettingsManager().Prj().GetLocalSettings();
        cfg = &localSettings.m_NetInspectorPanel;
    }

    // Never show an unconnected net
    if( aNet->GetNetCode() <= 0 )
        return false;

    const wxString  filterString = UnescapeString( m_searchCtrl->GetValue() ).Upper();
    const wxString  netName = UnescapeString( aNet->GetNetname() ).Upper();
    const NETCLASS* netClass = aNet->GetNetClass();
    const wxString  netClassName = UnescapeString( netClass->GetName() ).Upper();

    bool matched = false;

    // No filter - match all
    if( filterString.Length() == 0 )
        matched = true;

    // Search on net class
    if( !matched && cfg->filter_by_netclass && netClassName.Find( filterString ) != wxNOT_FOUND )
        matched = true;

    // Search on net name
    if( !matched && cfg->filter_by_net_name && netName.Find( filterString ) != wxNOT_FOUND )
        matched = true;

    // Remove unconnected nets if required
    if( matched )
    {
        if( !m_showUnconnectedNets )
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


std::vector<CN_ITEM*> PCB_NET_INSPECTOR_PANEL::relevantConnectivityItems() const
{
    // Pre-filter the connectivity items and sort them by netcode. This avoids quadratic runtime when building the whole
    // net list.
    const auto type_bits = std::bitset<MAX_STRUCT_TYPE_ID>().set( PCB_TRACE_T )
                                                            .set( PCB_ARC_T )
                                                            .set( PCB_VIA_T )
                                                            .set( PCB_PAD_T );

    std::vector<CN_ITEM*> cn_items;
    cn_items.reserve( 1024 );

    for( CN_ITEM* cn_item : m_board->GetConnectivity()->GetConnectivityAlgo()->ItemList() )
    {
        if( cn_item->Valid() && type_bits[cn_item->Parent()->Type()] )
            cn_items.push_back( cn_item );
    }

    std::ranges::sort( cn_items, NETCODE_CMP_LESS() );

    return cn_items;
}


std::vector<std::unique_ptr<PCB_NET_INSPECTOR_PANEL::LIST_ITEM>>
PCB_NET_INSPECTOR_PANEL::calculateNets( const std::vector<NETINFO_ITEM*>& aNetCodes, bool aIncludeZeroPadNets ) const
{
    std::vector<std::unique_ptr<LIST_ITEM>> results;

    LENGTH_DELAY_CALCULATION*   calc = m_board->GetLengthCalculation();
    const std::vector<CN_ITEM*> conItems = relevantConnectivityItems();

    // First assemble the LENGTH_CALCULATION_ITEMs for board items which match the nets we need to recompute
    // Precondition: conItems and aNetCodes are sorted in increasing netcode value
    // Functionality: This extracts any items from conItems which have a netcode which is present in aNetCodes
    std::unordered_map<int, std::vector<LENGTH_DELAY_CALCULATION_ITEM>> netItemsMap;
    std::vector<NETINFO_ITEM*>                                          foundNets;

    auto itemItr = conItems.begin();
    auto netCodeItr = aNetCodes.begin();

    while( itemItr != conItems.end() && netCodeItr != aNetCodes.end() )
    {
        const int curNetCode = ( *netCodeItr )->GetNetCode();
        const int curItemNetCode = ( *itemItr )->Net();

        if( curItemNetCode == curNetCode )
        {
            if( foundNets.empty() || foundNets.back() != *netCodeItr )
                foundNets.emplace_back( *netCodeItr );

            // Take the item
            LENGTH_DELAY_CALCULATION_ITEM lengthItem = calc->GetLengthCalculationItem( ( *itemItr )->Parent() );
            netItemsMap[curItemNetCode].emplace_back( std::move( lengthItem ) );
            ++itemItr;
        }
        else if( curItemNetCode < curNetCode )
        {
            // Fast-forward through items
            while( itemItr != conItems.end() && ( *itemItr )->Net() < curNetCode )
                ++itemItr;
        }
        else if( curItemNetCode > curNetCode )
        {
            // Fast-forward through required net codes
            while( netCodeItr != aNetCodes.end() && curItemNetCode > ( *netCodeItr )->GetNetCode() )
                ++netCodeItr;
        }
    }

    // Now calculate the length statistics for each net. This includes potentially expensive path optimisations, so
    // parallelize this work.
    std::mutex   resultsMutex;
    thread_pool& tp = GetKiCadThreadPool();

    auto resultsFuture = tp.submit_loop(
            0, foundNets.size(),
            [&, this, calc]( const int i )
            {
                int netCode = foundNets[i]->GetNetCode();

                constexpr PATH_OPTIMISATIONS opts = { .OptimiseViaLayers = true,
                                                      .MergeTracks = true,
                                                      .OptimiseTracesInPads = true,
                                                      .InferViaInPad = false };

                LENGTH_DELAY_STATS lengthDetails = calc->CalculateLengthDetails(
                                        netItemsMap[netCode],
                                        opts,
                                        nullptr,
                                        nullptr,
                                        LENGTH_DELAY_LAYER_OPT::WITH_LAYER_DETAIL,
                                        m_showTimeDomainDetails ? LENGTH_DELAY_DOMAIN_OPT::WITH_DELAY_DETAIL
                                                                : LENGTH_DELAY_DOMAIN_OPT::NO_DELAY_DETAIL );

                if( aIncludeZeroPadNets || lengthDetails.NumPads > 0 )
                {
                    std::unique_ptr<LIST_ITEM> new_item = std::make_unique<LIST_ITEM>( foundNets[i] );

                    new_item->SetPadCount( lengthDetails.NumPads );
                    new_item->SetLayerCount( m_board->GetCopperLayerCount() );
                    new_item->SetPadDieLength( lengthDetails.PadToDieLength );
                    new_item->SetPadDieDelay( lengthDetails.PadToDieDelay );
                    new_item->SetViaCount( lengthDetails.NumVias );
                    new_item->SetViaLength( lengthDetails.ViaLength );
                    new_item->SetViaDelay( lengthDetails.ViaDelay );
                    new_item->SetLayerWireLengths( *lengthDetails.LayerLengths );

                    if( m_showTimeDomainDetails )
                        new_item->SetLayerWireDelays( *lengthDetails.LayerDelays );

                    std::scoped_lock lock( resultsMutex );
                    results.emplace_back( std::move( new_item ) );
                }
            } );

    resultsFuture.get();

    return results;
}


/*****************************************************************************************
 *
 * Formatting helpers
 *
 * ***************************************************************************************/


wxString PCB_NET_INSPECTOR_PANEL::formatNetCode( const NETINFO_ITEM* aNet )
{
    return wxString::Format( wxT( "%.3d" ), aNet->GetNetCode() );
}


wxString PCB_NET_INSPECTOR_PANEL::formatNetName( const NETINFO_ITEM* aNet )
{
    return UnescapeString( aNet->GetNetname() );
}


wxString PCB_NET_INSPECTOR_PANEL::formatCount( const unsigned int aValue )
{
    return wxString::Format( wxT( "%u" ), aValue );
}


wxString PCB_NET_INSPECTOR_PANEL::formatLength( const int64_t aValue ) const
{
    return m_frame->MessageTextFromValue( aValue,
                                          // don't include unit label in the string when reporting
                                          !m_inReporting );
}

wxString PCB_NET_INSPECTOR_PANEL::formatDelay( const int64_t aValue ) const
{
    return m_frame->MessageTextFromValue( aValue,
                                          // don't include unit label in the string when reporting
                                          !m_inReporting, EDA_DATA_TYPE::TIME );
}


void PCB_NET_INSPECTOR_PANEL::updateDisplayedRowValues( const std::optional<LIST_ITEM_ITER>& aRow ) const
{
    if( !aRow )
        return;

    wxDataViewItemArray sel;
    m_netsList->GetSelections( sel );

    m_dataModel->updateItem( aRow );

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
    m_board = m_frame->GetBoard();

    if( m_board )
        m_board->AddListener( this );

    m_boardLoaded = true;
    m_boardLoading = true;

    const PROJECT_LOCAL_SETTINGS& localSettings = Pgm().GetSettingsManager().Prj().GetLocalSettings();
    auto&                   cfg = localSettings.m_NetInspectorPanel;
    m_searchCtrl->SetValue( cfg.filter_text );

    buildNetsList( true );

    m_boardLoading = false;
}


void PCB_NET_INSPECTOR_PANEL::OnBoardItemAdded( BOARD& aBoard, BOARD_ITEM* aBoardItem )
{
    const std::vector<BOARD_ITEM*> item{ aBoardItem };
    updateBoardItems( item );
}


void PCB_NET_INSPECTOR_PANEL::OnBoardItemsAdded( BOARD& aBoard, std::vector<BOARD_ITEM*>& aBoardItems )
{
    updateBoardItems( aBoardItems );
}


void PCB_NET_INSPECTOR_PANEL::updateBoardItems( const std::vector<BOARD_ITEM*>& aBoardItems )
{
    if( !IsShownOnScreen() )
        return;

    // Rebuild full list for large changes
    if( aBoardItems.size()
        > static_cast<size_t>( ADVANCED_CFG::GetCfg().m_NetInspectorBulkUpdateOptimisationThreshold ) )
    {
        buildNetsList();
    }
    else
    {
        std::vector<NETINFO_ITEM*> changedNets;

        for( BOARD_ITEM* boardItem : aBoardItems )
        {
            if( NETINFO_ITEM* net = dynamic_cast<NETINFO_ITEM*>( boardItem ) )
            {
                // A new net has been added to the board. Add it to our list if it passes the netname filter test.
                if( netFilterMatches( net ) )
                    changedNets.emplace_back( net );
            }
            else if( BOARD_CONNECTED_ITEM* i = dynamic_cast<BOARD_CONNECTED_ITEM*>( boardItem ) )
            {
                changedNets.emplace_back( i->GetNet() );
            }
            else if( FOOTPRINT* footprint = dynamic_cast<FOOTPRINT*>( boardItem ) )
            {
                for( const PAD* pad : footprint->Pads() )
                {
                    if( netFilterMatches( pad->GetNet() ) )
                        changedNets.emplace_back( pad->GetNet() );
                }
            }
        }

        std::ranges::sort( changedNets,
                           []( const NETINFO_ITEM* a, const NETINFO_ITEM* b )
                           {
                               return a->GetNetCode() < b->GetNetCode();
                           } );

        updateNets( changedNets );
    }

    m_netsList->Refresh();
}


void PCB_NET_INSPECTOR_PANEL::updateNets( const std::vector<NETINFO_ITEM*>& aNets ) const
{
    std::vector<NETINFO_ITEM*>        netsToUpdate;
    std::unordered_set<NETINFO_ITEM*> netsToDelete;

    for( NETINFO_ITEM* net : aNets )
    {
        // Add all nets to the deletion list - we will prune this later to only contain unhandled nets
        netsToDelete.insert( net );

        // Only calculate nets that match the current filter
        if( netFilterMatches( net ) )
            netsToUpdate.emplace_back( net );
    }

    wxWindowUpdateLocker updateLocker( m_netsList );

    std::vector<std::unique_ptr<LIST_ITEM>> newListItems = calculateNets( aNets, true );

    for( std::unique_ptr<LIST_ITEM>& newListItem : newListItems )
    {
        // Remove the handled net from the deletion list
        netsToDelete.erase( newListItem->GetNet() );

        std::optional<LIST_ITEM_ITER> curNetRow = m_dataModel->findItem( newListItem->GetNetCode() );

        if( !m_showZeroPadNets && newListItem->GetPadCount() == 0 )
        {
            m_dataModel->deleteItem( curNetRow );
            continue;
        }

        if( !curNetRow )
        {
            m_dataModel->addItem( std::move( newListItem ) );
            continue;
        }

        const std::unique_ptr<LIST_ITEM>& curListItem = *curNetRow.value();

        if( curListItem->GetNetName() != newListItem->GetNetName() )
        {
            // If the name has changed, it might require re-grouping. It's easier to remove and re-insert it.
            m_dataModel->deleteItem( curNetRow );
            m_dataModel->addItem( std::move( newListItem ) );
        }
        else
        {
            curListItem->SetPadCount( newListItem->GetPadCount() );
            curListItem->SetPadDieLength( newListItem->GetPadDieLength() );
            curListItem->SetPadDieDelay( newListItem->GetPadDieDelay() );
            curListItem->SetViaCount( newListItem->GetViaCount() );
            curListItem->SetViaLength( newListItem->GetViaLength() );
            curListItem->SetViaDelay( newListItem->GetViaDelay() );
            curListItem->SetLayerWireLengths( newListItem->GetLayerWireLengths() );

            if( m_showTimeDomainDetails )
                curListItem->SetLayerWireDelays( newListItem->GetLayerWireDelays() );

            updateDisplayedRowValues( curNetRow );
        }
    }

    // Delete any nets we have not yet handled
    for( const NETINFO_ITEM* netToDelete : netsToDelete )
        m_dataModel->deleteItem( m_dataModel->findItem( netToDelete->GetNetCode() ) );
}


void PCB_NET_INSPECTOR_PANEL::OnBoardItemRemoved( BOARD& aBoard, BOARD_ITEM* aBoardItem )
{
    const std::vector<BOARD_ITEM*> item{ aBoardItem };
    updateBoardItems( item );
}


void PCB_NET_INSPECTOR_PANEL::OnBoardItemsRemoved( BOARD& aBoard, std::vector<BOARD_ITEM*>& aBoardItems )
{
    updateBoardItems( aBoardItems );
}


void PCB_NET_INSPECTOR_PANEL::OnBoardNetSettingsChanged( BOARD& aBoard )
{
    if( !IsShownOnScreen() )
        return;

    buildNetsList();
}


void PCB_NET_INSPECTOR_PANEL::OnBoardItemChanged( BOARD& aBoard, BOARD_ITEM* aBoardItem )
{
    const std::vector<BOARD_ITEM*> item{ aBoardItem };
    updateBoardItems( item );
}


void PCB_NET_INSPECTOR_PANEL::OnBoardItemsChanged( BOARD& aBoard, std::vector<BOARD_ITEM*>& aBoardItems )
{
    updateBoardItems( aBoardItems );
}


void PCB_NET_INSPECTOR_PANEL::OnBoardCompositeUpdate( BOARD&                    aBoard,
                                                      std::vector<BOARD_ITEM*>& aAddedItems,
                                                      std::vector<BOARD_ITEM*>& aRemovedItems,
                                                      std::vector<BOARD_ITEM*>& aChangedItems )
{
    if( !IsShownOnScreen() )
        return;

    std::vector<BOARD_ITEM*> allItems{ aAddedItems.begin(), aAddedItems.end() };
    allItems.insert( allItems.end(), aRemovedItems.begin(), aRemovedItems.end() );
    allItems.insert( allItems.end(), aChangedItems.begin(), aChangedItems.end() );
    updateBoardItems( allItems );
}


void PCB_NET_INSPECTOR_PANEL::OnBoardHighlightNetChanged( BOARD& aBoard )
{
    if( m_highlightingNets || !IsShownOnScreen() )
        return;

    if( !m_board->IsHighLightNetON() )
    {
        m_netsList->UnselectAll();
    }
    else
    {
        const std::set<int>& selected_codes = m_board->GetHighLightNetCodes();

        wxDataViewItemArray new_selection;
        new_selection.Alloc( selected_codes.size() );

        for( const int code : selected_codes )
        {
            if( std::optional<LIST_ITEM_ITER> r = m_dataModel->findItem( code ) )
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
    OnBoardHighlightNetChanged( *m_board );
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
    wxMenuItem* highlightNet = new wxMenuItem( &menu, ID_HIGHLIGHT_SELECTED_NETS, _( "Highlight Selected Net" ),
                                               wxEmptyString, wxITEM_NORMAL );
    menu.Append( highlightNet );

    wxMenuItem* clearHighlighting = new wxMenuItem( &menu, ID_CLEAR_HIGHLIGHTING, _( "Clear Net Highlighting" ),
                                                    wxEmptyString, wxITEM_NORMAL );
    menu.Append( clearHighlighting );

    RENDER_SETTINGS* renderSettings = m_frame->GetCanvas()->GetView()->GetPainter()->GetSettings();
    const std::set<int>& selected_codes = renderSettings->GetHighlightNetCodes();

    if( selected_codes.size() == 0 )
        clearHighlighting->Enable( false );

    menu.AppendSeparator();

    wxMenuItem* renameNet = new wxMenuItem( &menu, ID_RENAME_NET, _( "Rename Selected Net..." ), wxEmptyString,
                                            wxITEM_NORMAL );
    menu.Append( renameNet );

    wxMenuItem* deleteNet = new wxMenuItem( &menu, ID_DELETE_NET, _( "Delete Selected Net" ), wxEmptyString,
                                            wxITEM_NORMAL );
    menu.Append( deleteNet );

    menu.AppendSeparator();

    wxMenuItem* addNet = new wxMenuItem( &menu, ID_ADD_NET, _( "Add Net..." ), wxEmptyString, wxITEM_NORMAL );
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

    wxMenuItem* removeSelectedGroup = new wxMenuItem( &menu, ID_REMOVE_SELECTED_GROUP,
                                                      _( "Remove Selected Custom Group" ),
                                                      wxEmptyString, wxITEM_NORMAL );
    menu.Append( removeSelectedGroup );

    if( !selItem || !selItem->GetIsGroup() )
        removeSelectedGroup->Enable( false );

    menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &PCB_NET_INSPECTOR_PANEL::onContextMenuSelection, this );

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
    wxStaticText* help = new wxStaticText( &dlg, wxID_ANY, _( "(Use /.../ to indicate a regular expression.)" ) );
    help->SetFont( KIUI::GetInfoFont( this ).Italic() );
   	dlg.m_ContentSizer->Add( help, 0, wxALL|wxEXPAND, 5 );
    dlg.SetTextValidator( validator );
    dlg.GetSizer()->SetSizeHints( &dlg );

    if( dlg.ShowModal() != wxID_OK || dlg.GetValue().IsEmpty() )
        return; //Aborted by user

    newGroupName = UnescapeString( dlg.GetValue() );

    if( newGroupName == "" )
        return;

    if( std::ranges::find_if( m_custom_group_rules,
                              [&]( std::unique_ptr<EDA_COMBINED_MATCHER>& rule )
                              {
                                  return rule->GetPattern() == newGroupName;
                              } )
        == m_custom_group_rules.end() )
    {
        m_custom_group_rules.push_back( std::make_unique<EDA_COMBINED_MATCHER>( newGroupName, CTX_NET ) );
        SaveSettings();
    }

    buildNetsList();
}


void PCB_NET_INSPECTOR_PANEL::onClearHighlighting()
{
    m_highlightingNets = true;

    m_frame->GetCanvas()->GetView()->GetPainter()->GetSettings()->SetHighlight( false );
    m_frame->GetCanvas()->GetView()->UpdateAllLayersColor();
    m_frame->GetCanvas()->Refresh();

    m_highlightingNets = false;
}


void PCB_NET_INSPECTOR_PANEL::OnExpandCollapseRow( wxCommandEvent& event )
{
    if( !m_rowExpanding )
        SaveSettings();
}


void PCB_NET_INSPECTOR_PANEL::OnHeaderContextMenu( wxCommandEvent& event )
{
    wxMenu menu;
    generateShowHideColumnMenu( &menu );
    menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &PCB_NET_INSPECTOR_PANEL::onContextMenuSelection, this );
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
    wxMenuItem* filterByNetName = new wxMenuItem( &menu, ID_FILTER_BY_NET_NAME, _( "Filter by Net Name" ),
                                                  wxEmptyString, wxITEM_CHECK );
    menu.Append( filterByNetName );
    filterByNetName->Check( cfg.filter_by_net_name );

    wxMenuItem* filterByNetclass = new wxMenuItem( &menu, ID_FILTER_BY_NETCLASS, _( "Filter by Netclass" ),
                                                   wxEmptyString, wxITEM_CHECK );
    menu.Append( filterByNetclass );
    filterByNetclass->Check( cfg.filter_by_netclass );

    menu.AppendSeparator();

    // Grouping menu items
    //wxMenuItem* groupConstraint =
    //        new wxMenuItem( &menu, ID_GROUP_BY_CONSTRAINT, _( "Group by DRC Constraint" ),
    //                        wxEmptyString, wxITEM_CHECK );
    //groupConstraint->Check( m_group_by_constraint );
    //menu.Append( groupConstraint );

    wxMenuItem* groupNetclass = new wxMenuItem( &menu, ID_GROUP_BY_NETCLASS, _( "Group by Netclass" ),
                                                wxEmptyString, wxITEM_CHECK );
    menu.Append( groupNetclass );
    groupNetclass->Check( m_groupByNetclass );

    menu.AppendSeparator();

    wxMenuItem* addGroup = new wxMenuItem( &menu, ID_ADD_GROUP, _( "Add Custom Group..." ),
                                           wxEmptyString, wxITEM_NORMAL );
    menu.Append( addGroup );

    wxMenuItem* removeSelectedGroup = new wxMenuItem( &menu, ID_REMOVE_SELECTED_GROUP,
                                                      _( "Remove Selected Custom Group" ),
                                                      wxEmptyString, wxITEM_NORMAL );
    menu.Append( removeSelectedGroup );

    if( !selItem || !selItem->GetIsGroup() )
        removeSelectedGroup->Enable( false );

    wxMenuItem* removeCustomGroups = new wxMenuItem( &menu, ID_REMOVE_GROUPS, _( "Remove All Custom Groups" ),
                                                     wxEmptyString, wxITEM_NORMAL );
    menu.Append( removeCustomGroups );
    removeCustomGroups->Enable( m_custom_group_rules.size() != 0 );

    menu.AppendSeparator();

    wxMenuItem* showZeroNetPads = new wxMenuItem( &menu, ID_SHOW_ZERO_NET_PADS, _( "Show Zero Pad Nets" ),
                                                  wxEmptyString, wxITEM_CHECK );
    menu.Append( showZeroNetPads );
    showZeroNetPads->Check( m_showZeroPadNets );

    wxMenuItem* showUnconnectedNets = new wxMenuItem( &menu, ID_SHOW_UNCONNECTED_NETS, _( "Show Unconnected Nets" ),
                                                      wxEmptyString, wxITEM_CHECK );
    menu.Append( showUnconnectedNets );
    showUnconnectedNets->Check( m_showUnconnectedNets );

    menu.AppendSeparator();

    wxMenuItem* showTimeDomainDetails = new wxMenuItem( &menu, ID_SHOW_TIME_DOMAIN_DETAILS,
                                                        _( "Show Time Domain Details" ),
                                                        wxEmptyString, wxITEM_CHECK );
    menu.Append( showTimeDomainDetails );
    showTimeDomainDetails->Check( m_showTimeDomainDetails );

    menu.AppendSeparator();

    // Report generation
    wxMenuItem* generateReport = new wxMenuItem( &menu, ID_GENERATE_REPORT, _( "Save Net Inspector Report..." ),
                                                 wxEmptyString, wxITEM_NORMAL );
    menu.Append( generateReport );

    menu.AppendSeparator();

    // Show / hide columns menu items
    wxMenu* colsMenu = new wxMenu();
    generateShowHideColumnMenu( colsMenu );
    menu.AppendSubMenu( colsMenu, _( "Show / Hide Columns" ) );

    menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &PCB_NET_INSPECTOR_PANEL::onContextMenuSelection, this );

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


void PCB_NET_INSPECTOR_PANEL::onContextMenuSelection( wxCommandEvent& event )
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
        m_groupByConstraint = !m_groupByConstraint;
        break;

    case ID_GROUP_BY_NETCLASS:
        m_groupByNetclass = !m_groupByNetclass;
        break;

    case ID_FILTER_BY_NET_NAME:
        m_filterByNetName = !m_filterByNetName;
        break;

    case ID_FILTER_BY_NETCLASS:
        m_filterByNetclass = !m_filterByNetclass;
        break;

    case ID_REMOVE_SELECTED_GROUP:
        onRemoveSelectedGroup();
        break;

    case ID_REMOVE_GROUPS:
        m_custom_group_rules.clear();
        break;

    case ID_SHOW_ZERO_NET_PADS:
        m_showZeroPadNets = !m_showZeroPadNets;
        break;

    case ID_SHOW_UNCONNECTED_NETS:
        m_showUnconnectedNets = !m_showUnconnectedNets;
        break;

    case ID_SHOW_TIME_DOMAIN_DETAILS:
        m_showTimeDomainDetails = !m_showTimeDomainDetails;
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
            const wxString groupName = selItem->GetGroupName();
            const auto     groupIter = std::ranges::find_if( m_custom_group_rules,
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

    m_inReporting = true;

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
    const unsigned int num_rows = m_dataModel->itemCount();

    for( unsigned int row = 0; row < num_rows; row++ )
    {
        auto& i = m_dataModel->itemAt( row );

        if( i.GetIsGroup() || i.GetNetCode() == 0 )
            continue;

        txt = "";

        for( auto&& col : m_columns )
        {
            if( static_cast<int>( col.csv_flags ) & static_cast<int>( CSV_COLUMN_DESC::CSV_QUOTE ) )
                txt += '"' + m_dataModel->valueAt( col.num, row ).GetString() + wxT( "\";" );
            else
                txt += m_dataModel->valueAt( col.num, row ).GetString() + ';';
        }

        f.AddLine( txt );
    }

    m_inReporting = false;

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
    if( m_inBuildNetsList )
        return;

    m_highlightingNets = true;

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

    m_highlightingNets = false;
}


void PCB_NET_INSPECTOR_PANEL::OnColumnSorted( wxDataViewEvent& event )
{
    if( !m_inBuildNetsList )
        SaveSettings();
}


void PCB_NET_INSPECTOR_PANEL::OnParentSetupChanged()
{
    // Rebuilt the nets list, and force rebuild of columns in case the stackup has changed
    buildNetsList( true );
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

        if( m_board->FindNet( newNetName ) )
        {
            DisplayError( this, wxString::Format( _( "Net name '%s' is already in use." ), newNetName ) );
            newNetName = wxEmptyString;
        }
        else
        {
            break;
        }
    }

    NETINFO_ITEM* newnet = new NETINFO_ITEM( m_board, dlg.GetValue(), 0 );

    m_board->Add( newnet );

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
                DisplayError( this, _( "Net name cannot be empty." ) );
                continue;
            }

            shortNetName = EscapeString( unescapedShortName, CTX_NETNAME );
            fullNetName = netPath + shortNetName;

            if( m_board->FindNet( shortNetName ) || m_board->FindNet( fullNetName ) )
            {
                DisplayError( this, wxString::Format( _( "Net name '%s' is already in use." ), unescapedShortName ) );
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
        auto removed_item = m_dataModel->deleteItem( m_dataModel->findItem( net ) );

        m_board->Remove( net );
        net->SetNetname( fullNetName );
        m_board->Add( net );

        for( BOARD_CONNECTED_ITEM* boardItem : m_frame->GetBoard()->AllConnectedItems() )
        {
            if( boardItem->GetFlags() & CANDIDATE )
                boardItem->SetNet( net );
        }

        buildNetsList();

        if( std::optional<LIST_ITEM_ITER> r = m_dataModel->findItem( net ) )
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
            || IsOK( this, wxString::Format( _( "Net '%s' is in use.  Delete anyway?" ), i->GetNetName() ) ) )
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

            m_board->Remove( i->GetNet() );
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
                && IsOK( this, wxString::Format( _( "Delete all nets in group '%s'?" ), ii->GetGroupName() ) ) )
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
    m_dataModel->updateAllItems();
}


void PCB_NET_INSPECTOR_PANEL::onUnitsChanged( wxCommandEvent& event )
{
    m_dataModel->updateAllItems();
    event.Skip();
}


/*****************************************************************************************
 *
 * Settings persistence
 *
 * ***************************************************************************************/


void PCB_NET_INSPECTOR_PANEL::SaveSettings()
{
    // Don't save settings if a board has not yet been loaded or the panel hasn't been displayed.
    // Events fire while we set up the panel which overwrite the settings we haven't yet loaded.
    bool displayed = false;

    for( unsigned int ii = 0; ii < m_dataModel->columnCount() && !displayed; ++ii )
    {
        if( m_netsList->GetColumn( ii )->GetWidth() > 0 )
            displayed = true;
    }

    if( !displayed || !m_boardLoaded || m_boardLoading )
        return;

    PROJECT_LOCAL_SETTINGS& localSettings = Pgm().GetSettingsManager().Prj().GetLocalSettings();
    auto&                   cfg = localSettings.m_NetInspectorPanel;

    // User-defined filters / grouping
    cfg.filter_text = m_searchCtrl->GetValue();
    cfg.filter_by_net_name = m_filterByNetName;
    cfg.filter_by_netclass = m_filterByNetclass;
    cfg.group_by_netclass = m_groupByNetclass;
    cfg.group_by_constraint = m_groupByConstraint;
    cfg.show_zero_pad_nets = m_showZeroPadNets;
    cfg.show_unconnected_nets = m_showUnconnectedNets;
    cfg.show_time_domain_details = m_showTimeDomainDetails;

    // Grid sorting
    wxDataViewColumn* sortingCol = m_netsList->GetSortingColumn();
    cfg.sorting_column = sortingCol ? static_cast<int>( sortingCol->GetModelColumn() ) : -1;
    cfg.sort_order_asc = sortingCol ? sortingCol->IsSortOrderAscending() : true;

    // Column arrangement / sizes
    cfg.col_order.resize( m_dataModel->columnCount() );
    cfg.col_widths.resize( m_dataModel->columnCount() );
    cfg.col_hidden.resize( m_dataModel->columnCount() );

    for( unsigned int ii = 0; ii < m_dataModel->columnCount(); ++ii )
    {
        cfg.col_order[ii] = (int) m_netsList->GetColumn( ii )->GetModelColumn();
        cfg.col_widths[ii] = m_netsList->GetColumn( ii )->GetWidth();
        cfg.col_hidden[ii] = m_netsList->GetColumn( ii )->IsHidden();
    }

    // Expanded rows
    cfg.expanded_rows.clear();
    std::vector<std::pair<wxString, wxDataViewItem>> groupItems = m_dataModel->getGroupDataViewItems();

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
