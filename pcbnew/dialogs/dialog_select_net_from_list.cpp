/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Oleg Endo <olegendo@gcc.gnu.org>
 * Copyright (C) 2019 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <fctsys.h>
#include <tools/pcb_inspection_tool.h>
#include <class_board.h>
#include <class_track.h>
#include <dialog_select_net_from_list.h>
#include <eda_pattern_match.h>
#include <wildcards_and_files_ext.h>
#include <view/view_controls.h>
#include <pcb_painter.h>
#include <connectivity/connectivity_algo.h>
#include <dialogs/dialog_text_entry.h>
#include <validators.h>
#include <bitmaps.h>

struct DIALOG_SELECT_NET_FROM_LIST::COLUMN_ID
{
    int      col_num;
    wxString display_name;

    operator int() const
    {
        return col_num;
    }
};

#define def_col( c, num, name ) \
    const DIALOG_SELECT_NET_FROM_LIST::COLUMN_ID DIALOG_SELECT_NET_FROM_LIST::c = { num, name }

def_col( COLUMN_NET, 0, _( "Net" ) );
def_col( COLUMN_NAME, 1, _( "Name" ) );
def_col( COLUMN_PAD_COUNT, 2, _( "Pad Count" ) );
def_col( COLUMN_VIA_COUNT, 3, _( "Via Count" ) );
def_col( COLUMN_BOARD_LENGTH, 4, _( "Board Length" ) );
def_col( COLUMN_CHIP_LENGTH, 5, _( "Die Length" ) );
def_col( COLUMN_TOTAL_LENGTH, 6, _( "Length" ) );

#undef def_col


struct DIALOG_SELECT_NET_FROM_LIST::LIST_ITEM
{
    LIST_ITEM( NETINFO_ITEM* aNet ) : m_net( aNet )
    {
    }

    NETINFO_ITEM* m_net;
    int           m_pad_count         = 0;
    int           m_via_count         = 0;
    int           m_board_wire_length = 0;
    int           m_chip_wire_length  = 0;
    int           m_total_length      = 0;
};


struct DIALOG_SELECT_NET_FROM_LIST::LIST_ITEM_NET_CMP_LESS
{
    const LIST_ITEM* m_base_ptr;

    LIST_ITEM_NET_CMP_LESS( const std::vector<LIST_ITEM>& container )
            : m_base_ptr( container.data() )
    {
    }

    bool operator()( unsigned int a, unsigned int b ) const
    {
        return m_base_ptr[a].m_net < m_base_ptr[b].m_net;
    }

    bool operator()( unsigned int a, NETINFO_ITEM* b ) const
    {
        return m_base_ptr[a].m_net < b;
    }

    bool operator()( NETINFO_ITEM* a, unsigned int b ) const
    {
        return a < m_base_ptr[b].m_net;
    }
};


struct DIALOG_SELECT_NET_FROM_LIST::ROW_DESC
{
    int                                                                    row_num = -1;
    decltype( DIALOG_SELECT_NET_FROM_LIST::m_list_items )::iterator        by_row;
    decltype( DIALOG_SELECT_NET_FROM_LIST::m_list_items_by_net )::iterator by_net;

    bool valid() const
    {
        return row_num != -1;
    }

    explicit operator bool() const
    {
        return valid();
    }
};


DIALOG_SELECT_NET_FROM_LIST::DIALOG_SELECT_NET_FROM_LIST( PCB_EDIT_FRAME* aParent,
                                                          const SETTINGS& aSettings ) :
        DIALOG_SELECT_NET_FROM_LIST_BASE( aParent ),
        m_frame( aParent )
{
    m_brd = aParent->GetBoard();
    m_wasSelected = false;

#define ADD_COL( name, flag, align ) m_netsList->AppendTextColumn( name, flag, 0, align, 0 );

    ADD_COL( COLUMN_NET.display_name,          wxDATAVIEW_CELL_INERT, wxALIGN_LEFT );
    ADD_COL( COLUMN_NAME.display_name,         wxDATAVIEW_CELL_INERT, wxALIGN_LEFT );
    ADD_COL( COLUMN_PAD_COUNT.display_name,    wxDATAVIEW_CELL_INERT, wxALIGN_CENTER );
    ADD_COL( COLUMN_VIA_COUNT.display_name,    wxDATAVIEW_CELL_INERT, wxALIGN_CENTER );
    ADD_COL( COLUMN_BOARD_LENGTH.display_name, wxDATAVIEW_CELL_INERT, wxALIGN_CENTER );
    ADD_COL( COLUMN_CHIP_LENGTH.display_name,  wxDATAVIEW_CELL_INERT, wxALIGN_CENTER );
    ADD_COL( COLUMN_TOTAL_LENGTH.display_name, wxDATAVIEW_CELL_INERT, wxALIGN_CENTER );

    // The fact that we're a list should keep the control from reserving space for the
    // expander buttons... but it doesn't.  Fix by forcing the indent to 0.
    m_netsList->SetIndent( 0 );

    m_textCtrlFilter->SetValue( aSettings.filter_string );
    m_cbShowZeroPad->SetValue( aSettings.show_zero_pad_nets );

    buildNetsList();

    adjustListColumns();

    m_addNet->SetBitmap( KiBitmap( small_plus_xpm ) );
    m_renameNet->SetBitmap( KiBitmap( small_edit_xpm ) );
    m_deleteNet->SetBitmap( KiBitmap( trash_xpm ) );

    m_sdbSizerOK->SetDefault();

    FinishDialogSettings();

#define connect_event( e, f ) \
    m_frame->Connect( e, wxCommandEventHandler( DIALOG_SELECT_NET_FROM_LIST::f ), nullptr, this )

    connect_event( wxEVT_CLOSE_WINDOW, onParentWindowClosed );
    connect_event( UNITS_CHANGED, onUnitsChanged );
    connect_event( BOARD_CHANGED, onBoardChanged );

#undef connect_event

    if( m_brd != nullptr )
        m_brd->AddListener( this );
}


DIALOG_SELECT_NET_FROM_LIST::~DIALOG_SELECT_NET_FROM_LIST()
{
#define disconnect_event( e, f ) \
    m_frame->Disconnect( e, wxCommandEventHandler( DIALOG_SELECT_NET_FROM_LIST::f ), nullptr, this )

    disconnect_event( wxEVT_CLOSE_WINDOW, onParentWindowClosed );
    disconnect_event( UNITS_CHANGED, onUnitsChanged );
    disconnect_event( BOARD_CHANGED, onBoardChanged );

#undef disconnect_event

    if( m_brd != nullptr )
        m_brd->RemoveListener( this );
}


DIALOG_SELECT_NET_FROM_LIST::SETTINGS DIALOG_SELECT_NET_FROM_LIST::Settings() const
{
    return { m_textCtrlFilter->GetValue(), m_cbShowZeroPad->IsChecked() };
}


void DIALOG_SELECT_NET_FROM_LIST::onParentWindowClosed( wxCommandEvent& event )
{
    Close();
    event.Skip();
}


void DIALOG_SELECT_NET_FROM_LIST::onUnitsChanged( wxCommandEvent& event )
{
    this->m_units = m_frame->GetUserUnits();

    buildNetsList();
    m_netsList->Refresh();

    event.Skip();
}


void DIALOG_SELECT_NET_FROM_LIST::onBoardChanged( wxCommandEvent& event )
{
    if( m_brd != nullptr )
        m_brd->RemoveListener( this );

    m_brd = m_frame->GetBoard();

    if( m_brd != nullptr )
        m_brd->AddListener( this );

    m_wasSelected = false;

    buildNetsList();
    m_netsList->Refresh();

    event.Skip();
}


bool DIALOG_SELECT_NET_FROM_LIST::netFilterMatches( NETINFO_ITEM* aNet ) const
{
    // Note: the filtering is case insensitive.

    if( m_netFilter.GetPattern().IsEmpty() )
        return true;

    return m_netFilter.Find( UnescapeString( aNet->GetNetname() ).Upper() )
           != EDA_PATTERN_NOT_FOUND;
}


struct NETCODE_CMP_LESS
{
    bool operator()( const CN_ITEM* a, const CN_ITEM* b ) const
    {
        return a->Net() < b->Net();
    }

    bool operator()( const CN_ITEM* a, int b ) const
    {
        return a->Net() < b;
    }

    bool operator()( int a, const CN_ITEM* b ) const
    {
        return a < b->Net();
    }
};


std::vector<CN_ITEM*> DIALOG_SELECT_NET_FROM_LIST::relevantConnectivityItems() const
{
    // pre-filter the connectivity items and sort them by netcode.
    // this avoids quadratic runtime when building the whole net list and
    // calculating the total length for each net.

    const auto type_bits = std::bitset<MAX_STRUCT_TYPE_ID>()
        .set( PCB_TRACE_T )
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


DIALOG_SELECT_NET_FROM_LIST::ROW_DESC DIALOG_SELECT_NET_FROM_LIST::findRow( int aNetCode )
{
    return findRow( m_brd->FindNet( aNetCode ) );
}


DIALOG_SELECT_NET_FROM_LIST::ROW_DESC DIALOG_SELECT_NET_FROM_LIST::findRow( NETINFO_ITEM* aNet )
{
    auto i = std::lower_bound( m_list_items_by_net.begin(), m_list_items_by_net.end(), aNet,
            LIST_ITEM_NET_CMP_LESS( m_list_items ) );

    if( i != m_list_items_by_net.end() && m_list_items[*i].m_net == aNet )
        return { static_cast<int>( *i ), m_list_items.begin() + *i, i };
    else
        return {};
}


void DIALOG_SELECT_NET_FROM_LIST::deleteRow( const ROW_DESC& aRow )
{
    if( !aRow )
        return;

    m_netsList->DeleteItem( aRow.row_num );
    m_list_items.erase( aRow.by_row );

    std::iter_swap( aRow.by_net, m_list_items_by_net.end() - 1 );
    m_list_items_by_net.pop_back();

    std::sort( m_list_items_by_net.begin(), m_list_items_by_net.end(),
            LIST_ITEM_NET_CMP_LESS( m_list_items ) );
}


void DIALOG_SELECT_NET_FROM_LIST::setValue( const ROW_DESC& aRow, const COLUMN_ID& aCol,
                                            wxString aVal )
{
    if( aRow )
        m_netsList->SetValue( aVal, aRow.row_num, aCol.col_num );
}


wxString DIALOG_SELECT_NET_FROM_LIST::formatNetCode( const NETINFO_ITEM* aNet ) const
{
    return wxString::Format( "%.3d", aNet->GetNet() );
}


wxString DIALOG_SELECT_NET_FROM_LIST::formatNetName( const NETINFO_ITEM* aNet ) const
{
    return UnescapeString( aNet->GetNetname() );
}


wxString DIALOG_SELECT_NET_FROM_LIST::formatCount( unsigned int aValue ) const
{
    return wxString::Format( "%u", aValue );
}


wxString DIALOG_SELECT_NET_FROM_LIST::formatLength( int aValue ) const
{
    return MessageTextFromValue( GetUserUnits(), aValue );
}


void DIALOG_SELECT_NET_FROM_LIST::OnBoardItemAdded( BOARD& aBoard, BOARD_ITEM* aBoardItem )
{
    if( NETINFO_ITEM* net = dynamic_cast<NETINFO_ITEM*>( aBoardItem ) )
    {
        // a new net has been added to the board.  add it to our list if it
        // passes the netname filter test.
        if( netFilterMatches( net ) )
        {
            m_list_items.emplace_back( net );
            m_list_items_by_net.push_back( m_list_items.size() - 1 );

            std::sort( m_list_items_by_net.begin(), m_list_items_by_net.end(),
                    LIST_ITEM_NET_CMP_LESS( m_list_items ) );

            LIST_ITEM& new_i   = m_list_items.back();
            new_i.m_pad_count = m_brd->GetNodesCount( net->GetNet() );

            wxVector<wxVariant> new_row( 7 );
            new_row[COLUMN_NET]          = formatNetCode( net );
            new_row[COLUMN_NAME]         = formatNetName( net );
            new_row[COLUMN_PAD_COUNT]    = formatCount( new_i.m_pad_count );
            new_row[COLUMN_VIA_COUNT]    = formatCount( new_i.m_via_count );
            new_row[COLUMN_BOARD_LENGTH] = formatLength( new_i.m_board_wire_length );
            new_row[COLUMN_CHIP_LENGTH]  = formatLength( new_i.m_chip_wire_length );
            new_row[COLUMN_TOTAL_LENGTH] = formatLength( new_i.m_total_length );

            m_netsList->AppendItem( new_row );
        }

        return;
    }
    else if( BOARD_CONNECTED_ITEM* i = dynamic_cast<BOARD_CONNECTED_ITEM*>( aBoardItem ) )
    {
        const ROW_DESC& r = findRow( i->GetNet() );

        if( r )
        {
            // try to handle frequent operations quickly.
            if( TRACK* track = dynamic_cast<TRACK*>( i ) )
            {
                int len = track->GetLength();
                r.by_row->m_board_wire_length += len;
                r.by_row->m_total_length += len;

                setValue( r, COLUMN_BOARD_LENGTH, formatLength( r.by_row->m_board_wire_length ) );
                setValue( r, COLUMN_TOTAL_LENGTH, formatLength( r.by_row->m_total_length ) );

                if( track->Type() == PCB_VIA_T )
                {
                    r.by_row->m_via_count += 1;
                    setValue( r, COLUMN_VIA_COUNT, formatCount( r.by_row->m_via_count ) );
                }

                return;
            }
        }

        // resort to generic slower net update otherwise.
        updateNet( i->GetNet() );
    }
}


void DIALOG_SELECT_NET_FROM_LIST::OnBoardItemRemoved( BOARD& aBoard, BOARD_ITEM* aBoardItem )
{
    if( NETINFO_ITEM* net = dynamic_cast<NETINFO_ITEM*>( aBoardItem ) )
    {
        deleteRow( findRow( net ) );
        return;
    }
    else if( MODULE* mod = dynamic_cast<MODULE*>( aBoardItem ) )
    {
        for( const D_PAD* pad : mod->Pads() )
        {
            const ROW_DESC& r = findRow( pad->GetNet() );

            if( r )
            {
                r.by_row->m_pad_count -= 1;

                if( r.by_row->m_pad_count == 0 && !m_cbShowZeroPad->IsChecked() )
                    deleteRow( r );
                else
                    setValue( r, COLUMN_PAD_COUNT, formatCount( r.by_row->m_pad_count ) );
            }
        }
    }
    else if( BOARD_CONNECTED_ITEM* i = dynamic_cast<BOARD_CONNECTED_ITEM*>( aBoardItem ) )
    {
        const ROW_DESC& r = findRow( i->GetNet() );

        if( r )
        {
            // try to handle frequent operations quickly.
            if( TRACK* track = dynamic_cast<TRACK*>( i ) )
            {
                int len = track->GetLength();
                r.by_row->m_board_wire_length -= len;
                r.by_row->m_total_length -= len;

                setValue( r, COLUMN_BOARD_LENGTH, formatLength( r.by_row->m_board_wire_length ) );
                setValue( r, COLUMN_TOTAL_LENGTH, formatLength( r.by_row->m_total_length ) );

                if( track->Type() == PCB_VIA_T )
                {
                    r.by_row->m_via_count -= 1;
                    setValue( r, COLUMN_VIA_COUNT, formatCount( r.by_row->m_via_count ) );
                }

                return;
            }

            // resort to generic slower net update otherwise.
            updateNet( i->GetNet() );
        }
    }
}


void DIALOG_SELECT_NET_FROM_LIST::OnBoardItemChanged( BOARD& aBoard, BOARD_ITEM* aBoardItem )
{
    if( dynamic_cast<BOARD_CONNECTED_ITEM*>( aBoardItem ) != nullptr
            || dynamic_cast<MODULE*>( aBoardItem ) != nullptr )
    {
        buildNetsList();
        m_netsList->Refresh();
    }
}


void DIALOG_SELECT_NET_FROM_LIST::OnBoardHighlightNetChanged( BOARD& aBoard )
{
    if( !m_brd->IsHighLightNetON() )
        m_netsList->UnselectAll();
    else if( !m_brd->GetHighLightNetCodes().empty() )
        HighlightNet( m_brd->FindNet( *m_brd->GetHighLightNetCodes().begin() ) );
}


void DIALOG_SELECT_NET_FROM_LIST::OnBoardNetSettingsChanged( BOARD& aBoard )
{
    buildNetsList();
    m_netsList->Refresh();
}


void DIALOG_SELECT_NET_FROM_LIST::updateNet( NETINFO_ITEM* aNet )
{
    // something for the specified net has changed, update that row.
    if( !netFilterMatches( aNet ) )
        return;

    // if the net had no pads before, it might not be in the displayed list yet.
    // if it had pads and now doesn't anymore, we might need to remove it from the list.

    auto cur_net_row = findRow( aNet );

    const unsigned int node_count = m_brd->GetNodesCount( aNet->GetNet() );

    if( node_count == 0 && !m_cbShowZeroPad->IsChecked() )
    {
        deleteRow( cur_net_row );
        return;
    }

    std::vector<CN_ITEM*> all_cn_items = relevantConnectivityItems();

    LIST_ITEM list_item( aNet );
    list_item.m_pad_count = node_count;

    const auto cn_items = std::equal_range( all_cn_items.begin(), all_cn_items.end(),
                                            aNet->GetNet(), NETCODE_CMP_LESS() );

    for( auto i = cn_items.first; i != cn_items.second; ++i )
    {
        BOARD_CONNECTED_ITEM* item = ( *i )->Parent();

        if( item->Type() == PCB_PAD_T )
            list_item.m_chip_wire_length += static_cast<D_PAD*>( item )->GetPadToDieLength();

        else if( TRACK* track = dynamic_cast<TRACK*>( item ) )
        {
            list_item.m_board_wire_length += track->GetLength();

            if( item->Type() == PCB_VIA_T )
                list_item.m_via_count += 1;
        }
    }

    list_item.m_total_length = list_item.m_board_wire_length + list_item.m_chip_wire_length;

    if( !cur_net_row )
    {
        m_list_items.push_back( list_item );
        m_list_items_by_net.push_back( m_list_items.size() - 1 );
        std::sort( m_list_items_by_net.begin(), m_list_items_by_net.end(),
                LIST_ITEM_NET_CMP_LESS( m_list_items ) );

        wxVector<wxVariant> new_row( 7 );
        new_row[COLUMN_NET]          = formatNetCode( aNet );
        new_row[COLUMN_NAME]         = formatNetName( aNet );
        new_row[COLUMN_PAD_COUNT]    = formatCount( list_item.m_pad_count );
        new_row[COLUMN_VIA_COUNT]    = formatCount( list_item.m_via_count );
        new_row[COLUMN_BOARD_LENGTH] = formatLength( list_item.m_board_wire_length );
        new_row[COLUMN_CHIP_LENGTH]  = formatLength( list_item.m_chip_wire_length );
        new_row[COLUMN_TOTAL_LENGTH] = formatLength( list_item.m_total_length );

        m_netsList->AppendItem( new_row );
    }
    else
    {
        *cur_net_row.by_row = list_item;

        setValue( cur_net_row, COLUMN_PAD_COUNT, formatCount( list_item.m_pad_count ) );
        setValue( cur_net_row, COLUMN_VIA_COUNT, formatCount( list_item.m_via_count ) );
        setValue( cur_net_row, COLUMN_BOARD_LENGTH, formatLength( list_item.m_board_wire_length ) );
        setValue( cur_net_row, COLUMN_CHIP_LENGTH, formatLength( list_item.m_chip_wire_length ) );
        setValue( cur_net_row, COLUMN_TOTAL_LENGTH, formatLength( list_item.m_total_length ) );
    }
}


void DIALOG_SELECT_NET_FROM_LIST::buildNetsList()
{
    // when rebuilding the netlist, try to keep the row selection
    const int prev_selected_row = m_netsList->GetSelectedRow();
    const int prev_selected_netcode =
            prev_selected_row >= 0 ? m_list_items[prev_selected_row].m_net->GetNet() : -1;

    m_netsList->DeleteAllItems();
    m_list_items.clear();

    std::vector<CN_ITEM*> prefiltered_cn_items = relevantConnectivityItems();

    // collect all nets which pass the filter string.

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
        bool operator()( const NET_INFO& a, int b ) const
        {
            return a.netcode < b;
        }
        bool operator()( int a, const NET_INFO& b ) const
        {
            return a < b.netcode;
        }
    };

    std::vector<NET_INFO> nets;
    nets.reserve( m_brd->GetNetInfo().NetsByNetcode().size() );

    for( const std::pair<const int, NETINFO_ITEM*>& ni : m_brd->GetNetInfo().NetsByNetcode() )
    {
        if( netFilterMatches( ni.second ) )
            nets.emplace_back( NET_INFO{ ni.first, ni.second, 0 } );
    }

    // count the pads for each net.  since the nets are sorted by netcode
    // this way around is faster than using counting pads for each net.

    for( MODULE* mod : m_brd->Modules() )
    {
        for( D_PAD* pad : mod->Pads() )
        {
            auto i = std::lower_bound( nets.begin(), nets.end(), pad->GetNetCode(),
                                       NET_INFO_CMP_LESS() );

            if( i != nets.end() && i->netcode == pad->GetNetCode() )
                i->pad_count += 1;
        }
    }

    for( NET_INFO& ni : nets )
    {
        if( !m_cbShowZeroPad->IsChecked() && ni.pad_count == 0 )
            continue;

        m_list_items.emplace_back( ni.net );
        LIST_ITEM& list_item = m_list_items.back();

        const auto cn_items = std::equal_range( prefiltered_cn_items.begin(),
                prefiltered_cn_items.end(), ni.netcode, NETCODE_CMP_LESS() );

        for( auto i = cn_items.first; i != cn_items.second; ++i )
        {
            BOARD_CONNECTED_ITEM* item = ( *i )->Parent();

            if( item->Type() == PCB_PAD_T )
                list_item.m_chip_wire_length += static_cast<D_PAD*>( item )->GetPadToDieLength();

            else if( TRACK* track = dynamic_cast<TRACK*>( item ) )
            {
                list_item.m_board_wire_length += track->GetLength();

                if( track->Type() == PCB_VIA_T )
                    list_item.m_via_count += 1;
            }
        }

        list_item.m_pad_count    = ni.pad_count;
        list_item.m_total_length = list_item.m_board_wire_length + list_item.m_chip_wire_length;
    }

    wxVector<wxVariant> dataLine;
    dataLine.resize( 7 );

    for( LIST_ITEM& i : m_list_items )
    {
        dataLine[COLUMN_NET]          = formatNetCode( i.m_net );
        dataLine[COLUMN_NAME]         = formatNetName( i.m_net );
        dataLine[COLUMN_PAD_COUNT]    = formatCount( i.m_pad_count );
        dataLine[COLUMN_VIA_COUNT]    = formatCount( i.m_via_count );
        dataLine[COLUMN_BOARD_LENGTH] = formatLength( i.m_board_wire_length );
        dataLine[COLUMN_CHIP_LENGTH]  = formatLength( i.m_chip_wire_length );
        dataLine[COLUMN_TOTAL_LENGTH] = formatLength( i.m_total_length );

        m_netsList->AppendItem( dataLine );
    }

    m_list_items_by_net.clear();
    m_list_items_by_net.reserve( m_list_items.size() );

    for( unsigned int i = 0; i < m_list_items.size(); ++i )
        m_list_items_by_net.push_back( i );

    std::sort( m_list_items_by_net.begin(), m_list_items_by_net.end(),
            LIST_ITEM_NET_CMP_LESS( m_list_items ) );

    if( prev_selected_netcode == -1 )
        m_wasSelected = false;
    else
    {
        const ROW_DESC& r = findRow( prev_selected_netcode );

        if( r )
        {
            m_selection   = r.by_row->m_net->GetNetname();
            m_wasSelected = true;

            wxDataViewItem i = m_netsList->RowToItem( r.row_num );
            m_netsList->Select( i );
            m_netsList->EnsureVisible( i );
        }
    }
}


void DIALOG_SELECT_NET_FROM_LIST::HighlightNet( NETINFO_ITEM* aNet )
{
    const ROW_DESC& r = findRow( aNet );

    if( r )
    {
        wxDataViewItem i = m_netsList->RowToItem( r.row_num );
        m_netsList->Select( i );
        m_netsList->EnsureVisible( i );
    }
    else
        m_netsList->UnselectAll();
}


void DIALOG_SELECT_NET_FROM_LIST::highlightNetOnBoard( NETINFO_ITEM* aNet ) const
{
    int netCode = aNet != nullptr ? aNet->GetNet() : -1;

    KIGFX::RENDER_SETTINGS *render = m_frame->GetCanvas()->GetView()->GetPainter()->GetSettings();
    render->SetHighlight( netCode >= 0, netCode );

    m_frame->GetCanvas()->GetView()->UpdateAllLayersColor();
    m_frame->GetCanvas()->Refresh();
}


void DIALOG_SELECT_NET_FROM_LIST::onFilterChange( wxCommandEvent& event )
{
    m_netFilter.SetPattern( m_textCtrlFilter->GetValue().Upper() );
    buildNetsList();
}


void DIALOG_SELECT_NET_FROM_LIST::onSelChanged( wxDataViewEvent&  )
{
    int selected_row = m_netsList->GetSelectedRow();

    if( selected_row >= 0 && selected_row < static_cast<int>( m_list_items.size() ) )
    {
        NETINFO_ITEM* net = m_list_items[selected_row].m_net;

        m_selection   = net->GetNetname();
        m_wasSelected = true;
        highlightNetOnBoard( net );
        return;
    }

    highlightNetOnBoard( nullptr );
    m_wasSelected = false;
}


void DIALOG_SELECT_NET_FROM_LIST::adjustListColumns()
{
    int w0, w1, w2, w3, w4, w5, w6;

    /**
     * Calculating optimal width of the first (Net) and the last (Pad Count) columns.
     * That width must be enough to fit column header label and be not less than width of
     * four chars (0000).
     */

    wxClientDC dc( GetParent() );
    int h, minw, minw_col0;

    dc.GetTextExtent( COLUMN_NET.display_name + "MM", &w0, &h );
    dc.GetTextExtent( COLUMN_PAD_COUNT.display_name + "MM", &w2, &h );
    dc.GetTextExtent( COLUMN_VIA_COUNT.display_name + "MM", &w3, &h );
    dc.GetTextExtent( COLUMN_BOARD_LENGTH.display_name + "MM", &w4, &h );
    dc.GetTextExtent( COLUMN_CHIP_LENGTH.display_name + "MM", &w5, &h );
    dc.GetTextExtent( COLUMN_TOTAL_LENGTH.display_name + "MM", &w6, &h );
    dc.GetTextExtent( "M00000,000 mmM", &minw, &h );
    dc.GetTextExtent( "M00000M", &minw_col0, &h );

    // Considering left and right margins.
    // For wxRenderGeneric it is 5px.
    m_netsList->GetColumn( 0 )->SetWidth( std::max( w0 + 10, minw_col0 ) );
    m_netsList->GetColumn( 2 )->SetWidth( w2 + 10 );
    m_netsList->GetColumn( 3 )->SetWidth( w3 + 10 );
    m_netsList->GetColumn( 4 )->SetWidth( std::max( w4 + 10, minw ) );
    m_netsList->GetColumn( 5 )->SetWidth( std::max( w5 + 10, minw ) );
    m_netsList->GetColumn( 6 )->SetWidth( std::max( w6 + 10, minw ) );

    // At resizing of the list the width of middle column (Net Names) changes only.
    int width = m_netsList->GetClientSize().x;
    w1 = width - w0 - w2 - w3 - w4 - w5 - w6;

    // Column 1 (net names) need a minimal width to display net names
    dc.GetTextExtent( "MMMMMMMMMMMMMMMMMM", &minw, &h );
    w1 = std::max( w1, minw );

    m_netsList->GetColumn( 1 )->SetWidth( w1 );

    m_netsList->Refresh();
}


void DIALOG_SELECT_NET_FROM_LIST::onListSize( wxSizeEvent& aEvent )
{
    aEvent.Skip();
    adjustListColumns();
}


bool DIALOG_SELECT_NET_FROM_LIST::GetNetName( wxString& aName ) const
{
    aName = m_selection;
    return m_wasSelected;
}


void DIALOG_SELECT_NET_FROM_LIST::onAddNet( wxCommandEvent& aEvent )
{
    wxString          newNetName;
    NETNAME_VALIDATOR validator( &newNetName );

    WX_TEXT_ENTRY_DIALOG dlg( this, _( "Net name:" ), _( "New Net" ), newNetName );
    dlg.SetTextValidator( validator );

    while( true )
    {
        if( dlg.ShowModal() != wxID_OK || dlg.GetValue().IsEmpty() )
            return;    //Aborted by user

        newNetName = dlg.GetValue();

        if( m_brd->FindNet( newNetName ) )
        {
            DisplayError( this, wxString::Format( _( "Net name '%s' is already in use." ),
                                                  newNetName ) );
            newNetName = wxEmptyString;
        }
        else
        {
            break;
        }
    }

    NETINFO_ITEM *newnet = new NETINFO_ITEM( m_brd, dlg.GetValue(), 0 );

    m_brd->Add( newnet );
    m_frame->OnModify();

    // We'll get an OnBoardItemChanged callback from this to update our listbox
}


void DIALOG_SELECT_NET_FROM_LIST::onRenameNet( wxCommandEvent& aEvent )
{
    int selected_row = m_netsList->GetSelectedRow();

    if( selected_row >= 0 && selected_row < static_cast<int>( m_list_items.size() ) )
    {
        NETINFO_ITEM* net = m_list_items[selected_row].m_net;
        wxString      fullNetName = net->GetNetname();
        wxString      netPath;
        wxString      shortNetName;

        if( fullNetName.Contains( "/" ) )
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
            if( dlg.ShowModal() != wxID_OK )
                return;    //Aborted by user

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

        net->SetNetname( fullNetName );
        m_frame->OnModify();

        buildNetsList();
        m_netsList->Refresh();
    }
}


void DIALOG_SELECT_NET_FROM_LIST::onDeleteNet( wxCommandEvent& aEvent )
{
    int selected_row = m_netsList->GetSelectedRow();

    if( selected_row >= 0 && selected_row < static_cast<int>( m_list_items.size() ) )
    {
        NETINFO_ITEM* net = m_list_items[selected_row].m_net;

        if( m_list_items[selected_row].m_pad_count > 0 )
        {
            if( !IsOK( this, _( "Net is in use.  Delete anyway?" ) ) )
                return;
        }

        m_brd->Remove( net );
        m_frame->OnModify();

        // We'll get an OnBoardItemChanged callback from this to update our listbox
    }
}


void DIALOG_SELECT_NET_FROM_LIST::onReport( wxCommandEvent& aEvent )
{
    wxFileDialog dlg( this, _( "Report file" ), "", "",
                      _( "Report file" ) + AddFileExtListToFilter( { "csv" } ),
                      wxFD_SAVE );

    if( dlg.ShowModal() == wxID_CANCEL )
       return;

    wxTextFile f( dlg.GetPath() );

    f.Create();

    int rows = m_netsList->GetItemCount();
    wxString txt;

    // Print Header:
    txt.Printf( "\"%s\";\"%s\";\"%s\";\"%s\";\"%s\";\"%s\";\"%s\";",
                _( "Net Id" ), _( "Net name" ),
                _( "Pad count" ), _( "Via count" ),
                _( "Board length" ), _( "Die length" ), _( "Net length" ) );
    f.AddLine( txt );

    // Print list of nets:
   for( int row = 1; row < rows; row++ )
   {
       txt.Printf( "%s;\"%s\";%s;%s;%s;%s;%s;",
                   m_netsList->GetTextValue( row, COLUMN_NET ),
                   m_netsList->GetTextValue( row, COLUMN_NAME ),
                   m_netsList->GetTextValue( row, COLUMN_PAD_COUNT ),
                   m_netsList->GetTextValue( row, COLUMN_VIA_COUNT ),
                   m_netsList->GetTextValue( row, COLUMN_BOARD_LENGTH ),
                   m_netsList->GetTextValue( row, COLUMN_CHIP_LENGTH ),
                   m_netsList->GetTextValue( row, COLUMN_TOTAL_LENGTH ) );

       f.AddLine( txt );
   }

   f.Write();
   f.Close();
}

