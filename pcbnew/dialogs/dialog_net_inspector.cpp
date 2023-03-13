/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Oleg Endo <olegendo@gcc.gnu.org>
 * Copyright (C) 2019 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <string_utils.h>
#include <tools/board_inspection_tool.h>
#include <board_design_settings.h>
#include <confirm.h>
#include <pcb_track.h>
#include <dialog_net_inspector.h>
#include <eda_pattern_match.h>
#include <wildcards_and_files_ext.h>
#include <view/view.h>
#include <view/view_controls.h>
#include <pcb_painter.h>
#include <kiplatform/ui.h>
#include <connectivity/connectivity_algo.h>
#include <dialogs/dialog_text_entry.h>
#include <validators.h>
#include <bitmaps.h>
#include <widgets/std_bitmap_button.h>

#include <wx/tokenzr.h>
#include <wx/filedlg.h>
#include <wx/dcclient.h>
#include <wx/wupdlock.h>

#include <bitset>
#include <vector>


wxDEFINE_EVENT( EDA_EVT_CLOSE_NET_INSPECTOR_DIALOG, wxCommandEvent );


static DIALOG_NET_INSPECTOR::SETTINGS g_settings;


enum class CSV_COLUMN_DESC : int
{
    CSV_NONE  = 0,
    CSV_QUOTE = 1 << 0
};


struct DIALOG_NET_INSPECTOR::COLUMN_DESC
{
    COLUMN_DESC( unsigned aNum, PCB_LAYER_ID aLayer, const wxString& aDisp, const wxString& aCsv,
                 CSV_COLUMN_DESC aFlags ) :
        num( aNum ), layer( aLayer ), display_name( aDisp ), csv_name( aCsv ), csv_flags( aFlags )
    {}

    unsigned int num;
    PCB_LAYER_ID layer;
    wxString     display_name;
    wxString     csv_name;
    CSV_COLUMN_DESC csv_flags;

    operator unsigned int() const
    {
        return num;
    }
};

enum
{
    COLUMN_NET = 0,
    COLUMN_NAME,
    COLUMN_PAD_COUNT,
    COLUMN_VIA_COUNT,
    COLUMN_VIA_LENGTH,
    COLUMN_BOARD_LENGTH,
    COLUMN_CHIP_LENGTH,
    COLUMN_TOTAL_LENGTH,
    COLUMN_NUM_STATIC_COL = COLUMN_TOTAL_LENGTH
};


class DIALOG_NET_INSPECTOR::LIST_ITEM
{
public:
    LIST_ITEM( unsigned int aGroupNumber, const wxString& aGroupName ) :
            m_is_group( true ),
            m_group_number( aGroupNumber ),
            m_net_name( aGroupName )
    {
        m_column_changed.resize( COLUMN_NUM_STATIC_COL + MAX_CU_LAYERS, 0 );
    }

    LIST_ITEM( NETINFO_ITEM* aNet ) :
            m_net( aNet )
    {
        m_net_name = UnescapeString( aNet->GetNetname() );
        m_column_changed.resize( COLUMN_NUM_STATIC_COL + MAX_CU_LAYERS, 0 );
    }

    LIST_ITEM()
    {
        m_column_changed.resize( COLUMN_NUM_STATIC_COL + MAX_CU_LAYERS, 0 );
    }

    LIST_ITEM& operator=( const LIST_ITEM& ) = delete;

    bool GetIsGroup() const { return m_is_group; }

    auto ChildrenBegin() const { return m_children.begin(); }
    auto ChildrenEnd() const { return m_children.end(); }
    unsigned int ChildrenCount() const { return m_children.size(); }

    NETINFO_ITEM* GetNet() const { return m_net; }

    int GetNetCode() const
    {
        return GetIsGroup() ? ( 0 - int( m_group_number ) - 1 ) : m_net->GetNetCode();
    }

    const wxString& GetNetName() const { return m_net_name; }
    const wxString& GetGroupName() const { return m_net_name; }

    void ResetColumnChangedBits()
    {
        std::fill( m_column_changed.begin(), m_column_changed.end(), 0 );
    }

    unsigned int GetPadCount() const
    {
        return m_pad_count;
    }

    bool PadCountChanged() const
    {
        return m_column_changed[COLUMN_PAD_COUNT];
    }

    void SetPadCount( unsigned int aValue )
    {
        if( m_parent )
            m_parent->SetPadCount( m_parent->GetPadCount() - m_pad_count + aValue );

        m_column_changed[COLUMN_PAD_COUNT] |= ( m_pad_count != aValue );
        m_pad_count = aValue;
    }

    void AddPadCount( unsigned int aValue )
    {
        if( m_parent )
            m_parent->AddPadCount( aValue );

        m_column_changed[COLUMN_PAD_COUNT] |= ( aValue != 0 );
        m_pad_count += aValue;
    }

    void SubPadCount( unsigned int aValue )
    {
        if( m_parent )
            m_parent->SubPadCount( aValue );

        m_column_changed[COLUMN_PAD_COUNT] |= ( aValue != 0 );
        m_pad_count -= aValue;
    }

    unsigned GetViaCount() const
    {
        return m_via_count;
    }

    bool ViaCountChanged() const
    {
        return m_column_changed[COLUMN_VIA_COUNT];
    }

    void SetViaCount( unsigned int aValue )
    {
        if( m_parent )
            m_parent->SetViaCount( m_parent->GetViaCount() - m_via_count + aValue );

        m_column_changed[COLUMN_VIA_COUNT] |= ( m_via_count != aValue );
        m_via_count = aValue;
    }

    void AddViaCount( unsigned int aValue )
    {
        if( m_parent )
            m_parent->AddViaCount( aValue );

        m_column_changed[COLUMN_VIA_COUNT] |= ( aValue != 0 );
        m_via_count += aValue;
    }

    void SubViaCount( unsigned int aValue )
    {
        if( m_parent )
            m_parent->SubViaCount( aValue );

        m_column_changed[COLUMN_VIA_COUNT] |= ( aValue != 0 );
        m_via_count -= aValue;
    }

    uint64_t GetViaLength() const
    {
        return m_via_length;
    }

    bool ViaLengthChanged() const
    {
        return m_column_changed[COLUMN_VIA_LENGTH];
    }

    void SetViaLength( unsigned int aValue )
    {
        if( m_parent )
            m_parent->SetViaLength( m_parent->GetViaLength() - m_via_length + aValue );

        m_column_changed[COLUMN_VIA_LENGTH] |= ( m_via_length != aValue );
        m_via_length = aValue;
    }

    void AddViaLength( unsigned int aValue )
    {
        if( m_parent )
            m_parent->AddViaLength( aValue );

        m_column_changed[COLUMN_VIA_LENGTH] |= ( aValue != 0 );
        m_via_length += aValue;
    }

    void SubViaLength( uint64_t aValue )
    {
        if( m_parent )
            m_parent->SubViaLength( aValue );

        m_column_changed[COLUMN_VIA_LENGTH] |= ( aValue != 0 );
        m_via_length -= aValue;
    }

    uint64_t GetBoardWireLength() const
    {
        uint64_t retval = 0;

        for( uint64_t val : m_layer_wire_length )
            retval += val;

        return retval;
    }

    uint64_t GetLayerWireLength( size_t aLayer ) const
    {
        wxCHECK_MSG( aLayer < m_layer_wire_length.size(), 0, wxT( "Invalid layer specified" ) );

        return m_layer_wire_length[aLayer];
    }

    bool BoardWireLengthChanged() const
    {
        return m_column_changed[COLUMN_BOARD_LENGTH];
    }

    void SetLayerWireLength( const uint64_t aValue, size_t aLayer )
    {
        wxCHECK_RET( aLayer < m_layer_wire_length.size(), wxT( "Invalid layer specified" ) );

        if( m_parent )
            m_parent->SetLayerWireLength(
                    m_parent->GetBoardWireLength() - m_layer_wire_length[aLayer] + aValue, aLayer );

        m_column_changed[COLUMN_BOARD_LENGTH] |= ( m_layer_wire_length[aLayer] != aValue );
        m_layer_wire_length[aLayer] = aValue;
    }

    void AddLayerWireLength( const uint64_t aValue, size_t aLayer )
    {
        if( m_parent )
            m_parent->AddLayerWireLength( aValue, aLayer );

        m_column_changed[COLUMN_BOARD_LENGTH] |= ( m_layer_wire_length[aLayer] != 0 );
        m_layer_wire_length[aLayer] += aValue;
    }

    void SubLayerWireLength( const uint64_t aValue, size_t aLayer )
    {
        if( m_parent )
            m_parent->SubLayerWireLength( aValue, aLayer );

        m_column_changed[COLUMN_BOARD_LENGTH] |= ( m_layer_wire_length[aLayer] != 0 );
        m_layer_wire_length[aLayer] -= aValue;
    }

    uint64_t GetChipWireLength() const
    {
        return m_chip_wire_length;
    }

    bool ChipWireLengthChanged() const
    {
        return m_column_changed[COLUMN_CHIP_LENGTH];
    }

    void SetChipWireLength( uint64_t aValue )
    {
        if( m_parent )
            m_parent->SetChipWireLength(
                    m_parent->GetChipWireLength() - m_chip_wire_length + aValue );

        m_column_changed[COLUMN_CHIP_LENGTH] |= ( m_chip_wire_length != aValue );
        m_chip_wire_length = aValue;
    }

    void AddChipWireLength( uint64_t aValue )
    {
        if( m_parent )
            m_parent->AddChipWireLength( aValue );

        m_column_changed[COLUMN_CHIP_LENGTH] |= ( aValue != 0 );
        m_chip_wire_length += aValue;
    }

    void SubChipWireLength( uint64_t aValue )
    {
        if( m_parent )
            m_parent->SubChipWireLength( aValue );

        m_column_changed[COLUMN_CHIP_LENGTH] |= ( aValue != 0 );
        m_chip_wire_length -= aValue;
    }

    // the total length column is always computed, never stored.
    unsigned long long int GetTotalLength() const
    {
        return GetBoardWireLength() + GetViaLength() + GetChipWireLength();
    }

    bool TotalLengthChanged() const
    {
        return BoardWireLengthChanged() || ViaLengthChanged() || ChipWireLengthChanged();
    }

    LIST_ITEM* Parent() const
    {
        return m_parent;
    }

    void SetParent( LIST_ITEM* aParent )
    {
        if( m_parent == aParent )
            return;

        if( m_parent != nullptr )
        {
            m_parent->SubPadCount( GetPadCount() );
            m_parent->SubViaCount( GetViaCount() );
            m_parent->SubViaLength( GetViaLength() );

            for( size_t ii = 0; ii < m_layer_wire_length.size(); ++ii )
                m_parent->SubLayerWireLength( m_layer_wire_length[ii], ii );

            m_parent->SubChipWireLength( GetChipWireLength() );

            m_parent->m_children.erase( std::find( m_parent->m_children.begin(),
                                                   m_parent->m_children.end(), this ) );
        }

        m_parent = aParent;

        if( m_parent != nullptr )
        {
            m_parent->AddPadCount( GetPadCount() );
            m_parent->AddViaCount( GetViaCount() );
            m_parent->AddViaLength( GetViaLength() );

            for( size_t ii = 0; ii < m_layer_wire_length.size(); ++ii )
                m_parent->AddLayerWireLength( m_layer_wire_length[ii], ii );

            m_parent->AddChipWireLength( GetChipWireLength() );

            m_parent->m_children.push_back( this );
        }
    }

private:
    // an item can be the child of only one parent at a time.
    // FIXME: could use a more lightweight container like intrusive forward list.
    LIST_ITEM*              m_parent = nullptr;
    std::vector<LIST_ITEM*> m_children;

    bool          m_is_group          = false;
    unsigned int  m_group_number      = 0;
    NETINFO_ITEM* m_net               = nullptr;
    unsigned int  m_pad_count         = 0;
    unsigned int  m_via_count         = 0;
    uint64_t      m_via_length        = 0;
    uint64_t      m_chip_wire_length  = 0;

    std::array<uint64_t, MAX_CU_LAYERS> m_layer_wire_length{};

    // dirty bits to record when some attribute has changed.  this is to
    // avoid unnecessary resort operations.
    std::vector<char> m_column_changed;

    // cached formatted net name for faster display sorting.
    wxString m_net_name;
};


struct DIALOG_NET_INSPECTOR::LIST_ITEM_NETCODE_CMP_LESS
{
    template <typename T>
    bool operator()( const T& a, const T& b ) const
    {
        return a->GetNetCode() < b->GetNetCode();
    }

    template <typename T>
    bool operator()( const T& a, int b ) const
    {
        return a->GetNetCode() < b;
    }

    template <typename T>
    bool operator()( int a, const T& b ) const
    {
        return a < b->GetNetCode();
    }
};


class DIALOG_NET_INSPECTOR::DATA_MODEL : public wxDataViewModel
{
public:

    DATA_MODEL( DIALOG_NET_INSPECTOR& parent ) : m_parent( parent )
    {
    }

    unsigned int columnCount() const
    {
        return m_parent.m_columns.size();
    }

    unsigned int itemCount() const
    {
        return m_items.size();
    }

    wxVariant valueAt( unsigned int aCol, unsigned int aRow ) const
    {
        wxVariant r;
        GetValue( r, wxDataViewItem( const_cast<LIST_ITEM*>( &*( m_items[aRow] ) ) ), aCol );
        return r;
    }

    const LIST_ITEM& itemAt( unsigned int aRow ) const
    {
        return *m_items.at( aRow );
    }

    std::optional<LIST_ITEM_ITER> findItem( int aNetCode )
    {
        auto i = std::lower_bound(
                m_items.begin(), m_items.end(), aNetCode, LIST_ITEM_NETCODE_CMP_LESS() );

        if( i == m_items.end() || ( *i )->GetNetCode() != aNetCode )
            return std::nullopt;

        return { i };
    }

    std::optional<LIST_ITEM_ITER> findItem( NETINFO_ITEM* aNet )
    {
        if( aNet != nullptr )
            return findItem( aNet->GetNetCode() );
        else
            return std::nullopt;
    }

    std::optional<LIST_ITEM_ITER> addItem( std::unique_ptr<LIST_ITEM> aItem )
    {
        if( aItem == nullptr )
            return {};

        // make sure that the vector is always sorted.  usually when new nets are added,
        // they always get a higher netcode number than the already existing ones.
        // however, if we've got filtering enabled, we might not have all the nets in
        // our list, so do a sorted insertion.

        auto new_iter = std::lower_bound( m_items.begin(), m_items.end(), aItem->GetNetCode(),
                                          LIST_ITEM_NETCODE_CMP_LESS() );

        new_iter = m_items.insert( new_iter, std::move( aItem ) );
        const std::unique_ptr<LIST_ITEM>& new_item = *new_iter;

        if( m_parent.m_groupBy->IsChecked()
                && ( m_parent.m_groupByKind->GetSelection() == 0
                        || m_parent.m_groupByKind->GetSelection() == 1 ) )
        {
            for( unsigned int j = 0; j < m_parent.m_groupFilter.size(); ++j )
            {
                if( m_parent.m_groupFilter[j]->Find( new_item->GetNetName() ) )
                {
                    new_item->SetParent( &*m_items[j] );
                    break;
                }
            }
        }
        else if( m_parent.m_groupBy->IsChecked()
                 && ( m_parent.m_groupByKind->GetSelection() == 2
                         || m_parent.m_groupByKind->GetSelection() == 3 ) )
        {
            auto groups_begin = m_items.begin();
            auto groups_end   = std::find_if_not( m_items.begin(), m_items.end(),
                    []( const std::unique_ptr<LIST_ITEM>& x )
                    {
                        return x->GetIsGroup();
                    } );

            for( std::unique_ptr<EDA_PATTERN_MATCH>& f : m_parent.m_groupFilter )
            {
                EDA_PATTERN_MATCH::FIND_RESULT match = f->Find( new_item->GetNetName() );

                if( match )
                {
                    wxString match_str = new_item->GetNetName().substr( match.start, match.length );

                    auto group = std::find_if( groups_begin, groups_end,
                            [&]( const std::unique_ptr<LIST_ITEM>& x )
                            {
                                return x->GetNetName() == match_str;
                            } );

                    if( group == groups_end )
                    {
                        int dist = std::distance( groups_end, groups_begin );
                        group = m_items.insert( groups_end,
                                                std::make_unique<LIST_ITEM>( dist, match_str ) );

                        groups_end = group + 1;

                        ItemAdded( wxDataViewItem(( *group )->Parent() ),
                                   wxDataViewItem( &**group ) );
                    }

                    new_item->SetParent( &**group );
                    break;
                }
            }
        }

        ItemAdded( wxDataViewItem( new_item->Parent() ), wxDataViewItem( new_item.get() ) );

        return { new_iter };
    }

    void addItems( std::vector<std::unique_ptr<LIST_ITEM>>&& aItems )
    {
        if( m_items.empty() )
        {
            m_items = std::move( aItems );

            if( m_parent.m_groupBy->IsChecked()
                    && ( m_parent.m_groupByKind->GetSelection() == 0
                            || m_parent.m_groupByKind->GetSelection() == 1 ) )
            {
                // assume that there are fewer group filters than nets.
                // walk over the list items and assign them to groups.  note that the
                // first items are group items themselves, so start after those.
                for( unsigned int i = m_parent.m_groupFilter.size(); i < m_items.size(); ++i )
                {
                    for( unsigned int j = 0; j < m_parent.m_groupFilter.size(); ++j )
                    {
                        if( m_parent.m_groupFilter[j]->Find( m_items[ i ]->GetNetName() ) )
                        {
                            m_items[i]->SetParent( &*m_items[j] );
                            break;
                        }
                    }
                }
            }
            else if( m_parent.m_groupBy->IsChecked()
                     && ( m_parent.m_groupByKind->GetSelection() == 2
                             || m_parent.m_groupByKind->GetSelection() == 3 ) )
            {
                // assume that there will be fewer resulting groups than nets.
                // dynamically generate groups for the actual strings of the match result.
                // try out each filter on each item and group by the resulting substrings.
                std::vector<std::unique_ptr<LIST_ITEM>> groups;

                for( std::unique_ptr<LIST_ITEM>& i : m_items )
                {
                    for( std::unique_ptr<EDA_PATTERN_MATCH>& f : m_parent.m_groupFilter )
                    {
                        EDA_PATTERN_MATCH::FIND_RESULT match = f->Find( i->GetNetName() );

                        if( match )
                        {
                            wxString match_str = i->GetNetName().substr( match.start,
                                                                         match.length );

                            auto group = std::find_if( groups.begin(), groups.end(),
                                    [&]( const std::unique_ptr<LIST_ITEM>& x )
                                    {
                                        return x->GetNetName() == match_str;
                                    } );

                            if( group == groups.end() )
                            {
                                groups.emplace_back( std::make_unique<LIST_ITEM>( groups.size(),
                                                                                  match_str ) );
                                group = groups.end() - 1;
                            }

                            i->SetParent( &**group );
                            break;
                        }
                    }
                }

                // insert the group items at the front of the items list.
                for( std::unique_ptr<LIST_ITEM>& g : groups )
                    m_items.emplace_back( std::move( g ) );

                std::rotate( m_items.begin(), m_items.end() - groups.size(), m_items.end() );
            }

            for( std::unique_ptr<LIST_ITEM>& i : m_items )
                ItemAdded( wxDataViewItem( i->Parent() ), wxDataViewItem( &*i ) );
        }
        else
        {
            m_items.reserve( m_items.size() + aItems.size() );

            for( std::unique_ptr<LIST_ITEM>& i : aItems )
                addItem( std::move( i ) );
        }
    }

    std::unique_ptr<LIST_ITEM> deleteItem( const std::optional<LIST_ITEM_ITER>& aRow )
    {
        if( !aRow )
            return {};

        std::unique_ptr<LIST_ITEM> i = std::move( **aRow );

        // if the row has a parent, detach it first
        LIST_ITEM* parent = i->Parent();
        i->SetParent( nullptr );

        m_items.erase( *aRow );
        ItemDeleted( wxDataViewItem( parent ), wxDataViewItem( &*i ) );

        if( parent )
        {
            ItemChanged( wxDataViewItem( parent ) );

            // for grouping type 2,3 a group item might disappear if it becomes empty.
            if( ( m_parent.m_groupByKind->GetSelection() == 2
                        || m_parent.m_groupByKind->GetSelection() == 3 )
                    && parent != nullptr && parent->ChildrenCount() == 0 )
            {
                auto p = std::find_if( m_items.begin(), m_items.end(),
                        [&]( std::unique_ptr<LIST_ITEM>& x )
                        {
                            return x.get() == parent;
                        } );

                wxASSERT( p != m_items.end() );
                m_items.erase( p );

                ItemDeleted( wxDataViewItem( parent->Parent() ), wxDataViewItem( parent ) );
            }
        }

        Resort();
        return i;
    }

    void deleteAllItems()
    {
        BeforeReset();
        m_items.clear();
        AfterReset();
    }

    void updateItem( const std::optional<LIST_ITEM_ITER>& aRow )
    {
        if( aRow )
        {
            const std::unique_ptr<LIST_ITEM>& listItem = *aRow.value();

            if( listItem->Parent() )
                ItemChanged( wxDataViewItem( listItem->Parent() ) );

            ItemChanged( wxDataViewItem( listItem.get() ) );
            resortIfChanged( listItem.get() );
        }
    }

    void updateAllItems()
    {
        for( std::unique_ptr<LIST_ITEM>& i : m_items )
            ItemChanged( wxDataViewItem( i.get() ) );
    }

    void resortIfChanged( LIST_ITEM* aItem )
    {
        if( wxDataViewColumn* column = m_parent.m_netsList->GetSortingColumn() )
        {
            bool changed = false;

            for( const LIST_ITEM* i = aItem; i != nullptr; i = i->Parent() )
                changed |= itemColumnChanged( i, column->GetModelColumn() );

            for( LIST_ITEM* i = aItem; i != nullptr; i = i->Parent() )
                i->ResetColumnChangedBits();

            if( changed )
                Resort();
        }
    }

    bool itemColumnChanged( const LIST_ITEM* aItem, unsigned int aCol ) const
    {
        if( aItem == nullptr || aCol >= m_parent.m_columns.size() )
            return false;

        if( aCol == COLUMN_PAD_COUNT )
            return aItem->PadCountChanged();

        else if( aCol == COLUMN_VIA_COUNT )
            return aItem->ViaCountChanged();

        else if( aCol == COLUMN_VIA_LENGTH )
            return aItem->ViaLengthChanged();

        else if( aCol == COLUMN_BOARD_LENGTH )
            return aItem->BoardWireLengthChanged();

        else if( aCol == COLUMN_CHIP_LENGTH )
            return aItem->ChipWireLengthChanged();

        else if( aCol == COLUMN_TOTAL_LENGTH )
            return aItem->TotalLengthChanged();

        else if( aCol > COLUMN_NUM_STATIC_COL )
            return aItem->BoardWireLengthChanged();


        return false;
    }

    // implementation of wxDataViewModel interface
    // these are used to query the data model by the GUI view implementation.
    // these are not supposed to be used to modify the data model.  for that
    // use the public functions above.

protected:
    unsigned int GetColumnCount() const override
    {
        return columnCount();
    }

    void GetValue( wxVariant& aOutValue, const wxDataViewItem& aItem,
                   unsigned int aCol ) const override
    {
        if( LIST_ITEM* i = static_cast<LIST_ITEM*>( aItem.GetID() ) )
        {
            if( aCol == COLUMN_NET && !i->GetIsGroup() )
                aOutValue = m_parent.formatNetCode( i->GetNet() );

            else if( aCol == COLUMN_NET && i->GetIsGroup() )
                aOutValue = "";

            else if( aCol == COLUMN_NAME )
                aOutValue = i->GetNetName();

            else if( aCol == COLUMN_PAD_COUNT )
                aOutValue = m_parent.formatCount( i->GetPadCount() );

            else if( aCol == COLUMN_VIA_COUNT )
                aOutValue = m_parent.formatCount( i->GetViaCount() );

            else if( aCol == COLUMN_VIA_LENGTH )
                aOutValue = m_parent.formatLength( i->GetViaLength() );

            else if( aCol == COLUMN_BOARD_LENGTH )
                aOutValue = m_parent.formatLength( i->GetBoardWireLength() );

            else if( aCol == COLUMN_CHIP_LENGTH )
                aOutValue = m_parent.formatLength( i->GetChipWireLength() );

            else if( aCol == COLUMN_TOTAL_LENGTH )
                aOutValue = m_parent.formatLength( i->GetTotalLength() );

            else if( aCol > COLUMN_NUM_STATIC_COL )
                aOutValue = m_parent.formatLength( i->GetLayerWireLength( m_parent.m_columns[aCol].layer ) );
        }
    }

    static int compareUInt( uint64_t aValue1, uint64_t aValue2, bool aAsc )
    {
        if( aAsc )
            return aValue1 < aValue2 ? -1 : 1;
        else
            return aValue2 < aValue1 ? -1 : 1;
    }

    int Compare( const wxDataViewItem& aItem1, const wxDataViewItem& aItem2,
                 unsigned int aCol, bool aAsc ) const override
    {
        const LIST_ITEM& i1 = *static_cast<const LIST_ITEM*>( aItem1.GetID() );
        const LIST_ITEM& i2 = *static_cast<const LIST_ITEM*>( aItem2.GetID() );

        if( i1.GetIsGroup() && !i2.GetIsGroup() )
            return -1;

        if( i2.GetIsGroup() && !i1.GetIsGroup() )
            return 1;

        if( aCol == COLUMN_NET && i1.GetNetCode() != i2.GetNetCode() )
        {
            return aAsc ? ( i1.GetNetCode() - i2.GetNetCode() )
                        : ( i2.GetNetCode() - i1.GetNetCode() );
        }
        else if( aCol == COLUMN_NAME )
        {
            const wxString& s1 = i1.GetNetName();
            const wxString& s2 = i2.GetNetName();

            int res = aAsc ? s1.Cmp( s2 ) : s2.Cmp( s1 );

            if( res != 0 )
                return res;
        }

        else if( aCol == COLUMN_PAD_COUNT && i1.GetPadCount() != i2.GetPadCount() )
            return compareUInt( i1.GetPadCount(), i2.GetPadCount(), aAsc );

        else if( aCol == COLUMN_VIA_COUNT && i1.GetViaCount() != i2.GetViaCount() )
            return compareUInt( i1.GetViaCount(), i2.GetViaCount(), aAsc );

        else if( aCol == COLUMN_VIA_LENGTH && i1.GetViaLength() != i2.GetViaLength() )
            return compareUInt( i1.GetViaLength(), i2.GetViaLength(), aAsc );

        else if( aCol == COLUMN_BOARD_LENGTH && i1.GetBoardWireLength() != i2.GetBoardWireLength() )
            return compareUInt( i1.GetBoardWireLength(), i2.GetBoardWireLength(), aAsc );

        else if( aCol == COLUMN_CHIP_LENGTH && i1.GetChipWireLength() != i2.GetChipWireLength() )
            return compareUInt( i1.GetChipWireLength(), i2.GetChipWireLength(), aAsc );

        else if( aCol == COLUMN_TOTAL_LENGTH && i1.GetTotalLength() != i2.GetTotalLength() )
            return compareUInt( i1.GetTotalLength(), i2.GetTotalLength(), aAsc );

        else if( aCol > COLUMN_NUM_STATIC_COL
                && aCol < m_parent.m_columns.size()
                && i1.GetLayerWireLength( m_parent.m_columns[aCol].layer )
                        != i2.GetLayerWireLength( m_parent.m_columns[aCol].layer ) )
        {
            return compareUInt( i1.GetLayerWireLength( m_parent.m_columns[aCol].layer ),
                    i2.GetLayerWireLength( m_parent.m_columns[aCol].layer ), aAsc );
        }

        // when the item values compare equal resort to pointer comparison.
        wxUIntPtr id1 = wxPtrToUInt( aItem1.GetID() );
        wxUIntPtr id2 = wxPtrToUInt( aItem2.GetID() );

        return aAsc ? id1 - id2 : id2 - id1;
    }

    bool SetValue( const wxVariant& aInValue, const wxDataViewItem& aItem,
                   unsigned int aCol ) override
    {
        return false;
    }

    wxDataViewItem GetParent( const wxDataViewItem& aItem ) const override
    {
        if( !aItem.IsOk() )
            return wxDataViewItem();

        return wxDataViewItem( static_cast<const LIST_ITEM*>( aItem.GetID() )->Parent() );
    }

    bool IsContainer( const wxDataViewItem& aItem ) const override
    {
        if( !aItem.IsOk() )
            return true;

        return static_cast<const LIST_ITEM*>( aItem.GetID() )->GetIsGroup();
    }

    bool HasContainerColumns( const wxDataViewItem& aItem ) const override
    {
        return IsContainer( aItem );
    }

    unsigned int GetChildren( const wxDataViewItem& aParent,
                              wxDataViewItemArray& aChildren ) const override
    {
        const LIST_ITEM* p = static_cast<const LIST_ITEM*>( aParent.GetID() );

        if( !aParent.IsOk() )
        {
            aChildren.Alloc( m_items.size() );

            for( const std::unique_ptr<LIST_ITEM>& i : m_items )
            {
                if( i->Parent() == nullptr )
                    aChildren.Add( wxDataViewItem( &*i ) );
            }

            return aChildren.GetCount();
        }
        else if( p->GetIsGroup() )
        {
            const int count = p->ChildrenCount();

            if( count == 0 )
                return 0;

            aChildren.Alloc( count );

            for( auto i = p->ChildrenBegin(), end = p->ChildrenEnd(); i != end; ++i )
                aChildren.Add( wxDataViewItem( *i ) );

            return aChildren.GetCount();
        }

        return 0;
    }

    wxString GetColumnType( unsigned int /* aCol */ ) const override
    {
        return wxS( "string" );
    }

private:
    DIALOG_NET_INSPECTOR& m_parent;

    // primary container, sorted by netcode number.
    // groups have netcode < 0, so they always come first, in the order
    // of the filter strings as input by the user (group mode 0, 1) or
    // in order of occurrence (group mode 2, 3).
    std::vector<std::unique_ptr<LIST_ITEM>> m_items;
};


DIALOG_NET_INSPECTOR::DIALOG_NET_INSPECTOR( PCB_EDIT_FRAME* aParent ) :
        DIALOG_NET_INSPECTOR_BASE( aParent ),
        m_zero_netitem( nullptr ),
        m_frame( aParent )
{
    m_columns.emplace_back( 0u, UNDEFINED_LAYER, _( "Net" ), _( "Net Code" ),
                            CSV_COLUMN_DESC::CSV_NONE  );
    m_columns.emplace_back( 1u, UNDEFINED_LAYER, _( "Name" ), _( "Net Name" ),
                            CSV_COLUMN_DESC::CSV_QUOTE );
    m_columns.emplace_back( 2u, UNDEFINED_LAYER, _( "Pad Count" ), _( "Pad Count" ),
                            CSV_COLUMN_DESC::CSV_NONE );
    m_columns.emplace_back( 3u, UNDEFINED_LAYER, _( "Via Count" ), _( "Via Count" ),
                            CSV_COLUMN_DESC::CSV_NONE );
    m_columns.emplace_back( 4u, UNDEFINED_LAYER, _( "Via Length" ), _( "Via Length" ),
                            CSV_COLUMN_DESC::CSV_NONE );
    m_columns.emplace_back( 5u, UNDEFINED_LAYER, _( "Track Length" ), _( "Track Length" ),
                            CSV_COLUMN_DESC::CSV_NONE );
    m_columns.emplace_back( 6u, UNDEFINED_LAYER, _( "Die Length" ), _( "Die Length" ),
                            CSV_COLUMN_DESC::CSV_NONE );
    m_columns.emplace_back( 7u, UNDEFINED_LAYER, _( "Total Length" ), _( "Net Length" ),
                            CSV_COLUMN_DESC::CSV_NONE );

    m_brd = aParent->GetBoard();

    m_data_model = new DATA_MODEL( *this );
    m_netsList->AssociateModel( &*m_data_model );

    std::vector<std::function<void( void )>> add_col{
        [&]()
        {
            m_netsList->AppendTextColumn( m_columns[COLUMN_NET].display_name, m_columns[COLUMN_NET],
                                          wxDATAVIEW_CELL_INERT, -1, wxALIGN_LEFT,
                                          wxDATAVIEW_COL_SORTABLE );
        },
        [&]()
        {
            m_netsList->AppendTextColumn( m_columns[COLUMN_NAME].display_name,
                                          m_columns[COLUMN_NAME],
                                          wxDATAVIEW_CELL_INERT, -1, wxALIGN_LEFT,
                                          wxDATAVIEW_COL_RESIZABLE | wxDATAVIEW_COL_REORDERABLE |
                                          wxDATAVIEW_COL_SORTABLE );
        },
        [&]()
        {
            m_netsList->AppendTextColumn( m_columns[COLUMN_PAD_COUNT].display_name,
                                          m_columns[COLUMN_PAD_COUNT],
                                          wxDATAVIEW_CELL_INERT, -1, wxALIGN_CENTER,
                                          wxDATAVIEW_COL_REORDERABLE | wxDATAVIEW_COL_SORTABLE );
        },
        [&]()
        {
            m_netsList->AppendTextColumn( m_columns[COLUMN_VIA_COUNT].display_name,
                                          m_columns[COLUMN_VIA_COUNT],
                                          wxDATAVIEW_CELL_INERT, -1, wxALIGN_CENTER,
                                          wxDATAVIEW_COL_REORDERABLE | wxDATAVIEW_COL_SORTABLE );
        },
        [&]()
        {
            m_netsList->AppendTextColumn( m_columns[COLUMN_VIA_LENGTH].display_name,
                                          m_columns[COLUMN_VIA_LENGTH],
                                          wxDATAVIEW_CELL_INERT, -1, wxALIGN_CENTER,
                                          wxDATAVIEW_COL_REORDERABLE | wxDATAVIEW_COL_SORTABLE );
        },
        [&]()
        {
            m_netsList->AppendTextColumn( m_columns[COLUMN_BOARD_LENGTH].display_name,
                                          m_columns[COLUMN_BOARD_LENGTH],
                                          wxDATAVIEW_CELL_INERT, -1, wxALIGN_CENTER,
                                          wxDATAVIEW_COL_REORDERABLE | wxDATAVIEW_COL_SORTABLE );
        },
        [&]()
        {
            m_netsList->AppendTextColumn( m_columns[COLUMN_CHIP_LENGTH].display_name,
                                          m_columns[COLUMN_CHIP_LENGTH],
                                          wxDATAVIEW_CELL_INERT, -1, wxALIGN_CENTER,
                                          wxDATAVIEW_COL_REORDERABLE | wxDATAVIEW_COL_SORTABLE );
        },
        [&]()
        {
            m_netsList->AppendTextColumn( m_columns[COLUMN_TOTAL_LENGTH].display_name,
                                          m_columns[COLUMN_TOTAL_LENGTH],
                                          wxDATAVIEW_CELL_INERT, -1, wxALIGN_CENTER,
                                          wxDATAVIEW_COL_REORDERABLE | wxDATAVIEW_COL_SORTABLE );
        }
    };

    std::vector<int> col_order = g_settings.column_order;

    if( col_order.size() != add_col.size() )
    {
        col_order.resize( add_col.size() );

        for( unsigned int i = 0; i < add_col.size(); ++i )
            col_order[i] = i;
    }

    for( unsigned int i : col_order )
        add_col.at( i )();

    for( PCB_LAYER_ID layer : m_brd->GetEnabledLayers().Seq() )
    {
        if( !IsCopperLayer( layer ) )
            continue;

        m_columns.emplace_back( m_columns.size(), layer, m_brd->GetLayerName( layer ),
                                m_brd->GetLayerName( layer ), CSV_COLUMN_DESC::CSV_NONE );

        m_netsList->AppendTextColumn( m_brd->GetLayerName( layer ), m_columns.back(),
                                      wxDATAVIEW_CELL_INERT, -1, wxALIGN_CENTER,
                                      wxDATAVIEW_COL_REORDERABLE | wxDATAVIEW_COL_SORTABLE );
    }

    m_netsList->SetExpanderColumn( m_netsList->GetColumn( 0 ) );

    // avoid onFilterChange for each of the settings as it will re-build the
    // list over and over again.
    m_filter_change_no_rebuild = true;

    m_textCtrlFilter->SetValue( g_settings.filter_string );
    m_cbShowZeroPad->SetValue( g_settings.show_zero_pad_nets );
    m_groupBy->SetValue( g_settings.group_by );
    m_groupByKind->SetSelection( g_settings.group_by_kind );
    m_groupByText->SetValue( g_settings.group_by_text );

    m_filter_change_no_rebuild = false;
    buildNetsList();

    adjustListColumns();

    m_addNet->SetBitmap( KiBitmap( BITMAPS::small_plus ) );
    m_renameNet->SetBitmap( KiBitmap( BITMAPS::small_edit ) );
    m_deleteNet->SetBitmap( KiBitmap( BITMAPS::small_trash ) );

    SetupStandardButtons();

    m_renameNet->Disable();
    m_deleteNet->Disable();

    if( g_settings.sorting_column != -1 )
    {
        if( wxDataViewColumn* c = m_netsList->GetColumn( g_settings.sorting_column ) )
        {
            c->SetSortOrder( g_settings.sort_order_asc );
            m_data_model->Resort();
        }
    }

    finishDialogSettings();

    Bind( EDA_EVT_UNITS_CHANGED, &DIALOG_NET_INSPECTOR::onUnitsChanged, this );
    Bind( EDA_EVT_BOARD_CHANGED, &DIALOG_NET_INSPECTOR::onBoardChanged, this );

    if( m_brd != nullptr )
    {
        // if the dialog is opened while something is highlighted on the board ...
        OnBoardHighlightNetChanged( *m_brd );

        m_brd->AddListener( this );
    }
}


DIALOG_NET_INSPECTOR::~DIALOG_NET_INSPECTOR()
{
    std::vector<int> column_order( m_data_model->columnCount() );

    for( unsigned int i = 0; i < column_order.size(); ++i )
        column_order[i] = m_netsList->GetColumn( i )->GetModelColumn();

    wxDataViewColumn* sorting_column = m_netsList->GetSortingColumn();

    g_settings.filter_string      = m_textCtrlFilter->GetValue();
    g_settings.show_zero_pad_nets = m_cbShowZeroPad->IsChecked();
    g_settings.group_by           = m_groupBy->IsChecked();
    g_settings.group_by_kind      = m_groupByKind->GetSelection();
    g_settings.group_by_text      = m_groupByText->GetValue();
    g_settings.sorting_column     = sorting_column ?
                                    static_cast<int>( sorting_column->GetModelColumn() ) : -1;
    g_settings.sort_order_asc     = sorting_column ? sorting_column->IsSortOrderAscending() : true;
    g_settings.column_order       = column_order;

    // the displayed list elements are going to be deleted before the list view itself.
    // in some cases it might still do queries on the data model, which would crash
    // from now on.  so just disassociate it.
    m_netsList->AssociateModel( nullptr );

    Unbind( EDA_EVT_UNITS_CHANGED, &DIALOG_NET_INSPECTOR::onUnitsChanged, this );
    Unbind( EDA_EVT_BOARD_CHANGED, &DIALOG_NET_INSPECTOR::onBoardChanged, this );

    if( m_brd != nullptr )
        m_brd->RemoveListener( this );
}


void DIALOG_NET_INSPECTOR::onUnitsChanged( wxCommandEvent& event )
{
    m_units = m_frame->GetUserUnits();

    m_data_model->updateAllItems();

    event.Skip();
}


void DIALOG_NET_INSPECTOR::onBoardChanged( wxCommandEvent& event )
{
    m_brd = m_frame->GetBoard();

    if( m_brd != nullptr )
        m_brd->AddListener( this );

    buildNetsList();
    m_netsList->Refresh();

    event.Skip();
}


bool DIALOG_NET_INSPECTOR::netFilterMatches( NETINFO_ITEM* aNet ) const
{
    // Note: the filtering is case insensitive.

    // Never show the unconnected net
    if( aNet->GetNetCode() <= 0 )
        return false;

    // Show unconnected nets only if specifically asked for by filter
    if( m_netFilter.empty() )
        return !aNet->GetNetname().StartsWith( wxT( "unconnected-(" ) );

    wxString net_str = UnescapeString( aNet->GetNetname() ).Upper();

    for( const std::unique_ptr<EDA_PATTERN_MATCH>& f : m_netFilter )
    {
        if( f->Find( net_str ) )
            return true;
    }

    return false;
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


std::vector<CN_ITEM*> DIALOG_NET_INSPECTOR::relevantConnectivityItems() const
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


void DIALOG_NET_INSPECTOR::updateDisplayedRowValues( const std::optional<LIST_ITEM_ITER>& aRow )
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


wxString DIALOG_NET_INSPECTOR::formatNetCode( const NETINFO_ITEM* aNet ) const
{
    return wxString::Format( wxT( "%.3d" ), aNet->GetNetCode() );
}


wxString DIALOG_NET_INSPECTOR::formatNetName( const NETINFO_ITEM* aNet ) const
{
    return UnescapeString( aNet->GetNetname() );
}


wxString DIALOG_NET_INSPECTOR::formatCount( unsigned int aValue ) const
{
    return wxString::Format( wxT( "%u" ), aValue );
}


wxString DIALOG_NET_INSPECTOR::formatLength( int64_t aValue ) const
{
    return m_frame->MessageTextFromValue( static_cast<long long int>( aValue ) );
}


void DIALOG_NET_INSPECTOR::OnBoardItemAdded( BOARD& aBoard, BOARD_ITEM* aBoardItem )
{
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
                int len = track->GetLength();

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
                int len = pad->GetPadToDieLength();

                list_item->AddPadCount( 1 );
                list_item->AddChipWireLength( len );

                if( list_item->GetPadCount() == 0 && !m_cbShowZeroPad->IsChecked() )
                    m_data_model->deleteItem( r );
                else
                    updateDisplayedRowValues( r );
            }
        }
    }
}


void DIALOG_NET_INSPECTOR::OnBoardItemsAdded( BOARD& aBoard, std::vector<BOARD_ITEM*>& aBoardItem )
{
    for( BOARD_ITEM* item : aBoardItem )
    {
        OnBoardItemAdded( aBoard, item );
    }
}


void DIALOG_NET_INSPECTOR::OnBoardItemRemoved( BOARD& aBoard, BOARD_ITEM* aBoardItem )
{
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
                int len = pad->GetPadToDieLength();

                list_item->SubPadCount( 1 );
                list_item->SubChipWireLength( len );

                if( list_item->GetPadCount() == 0 && !m_cbShowZeroPad->IsChecked() )
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
                int len = track->GetLength();

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


void DIALOG_NET_INSPECTOR::OnBoardItemsRemoved( BOARD& aBoard,
                                                std::vector<BOARD_ITEM*>& aBoardItems )
{
    for( BOARD_ITEM* item : aBoardItems )
    {
        OnBoardItemRemoved( aBoard, item );
    }
}


void DIALOG_NET_INSPECTOR::OnBoardItemChanged( BOARD& aBoard, BOARD_ITEM* aBoardItem )
{
    if( dynamic_cast<BOARD_CONNECTED_ITEM*>( aBoardItem ) != nullptr
            || dynamic_cast<FOOTPRINT*>( aBoardItem ) != nullptr )
    {
        buildNetsList();
        m_netsList->Refresh();
    }
}


void DIALOG_NET_INSPECTOR::OnBoardItemsChanged( BOARD& aBoard,
                                                std::vector<BOARD_ITEM*>& aBoardItems )
{
    buildNetsList();
    m_netsList->Refresh();
}


void DIALOG_NET_INSPECTOR::OnBoardHighlightNetChanged( BOARD& aBoard )
{
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


void DIALOG_NET_INSPECTOR::OnBoardNetSettingsChanged( BOARD& aBoard )
{
    buildNetsList();
    m_netsList->Refresh();
}


void DIALOG_NET_INSPECTOR::updateNet( NETINFO_ITEM* aNet )
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

    if( node_count == 0 && !m_cbShowZeroPad->IsChecked() )
    {
        m_data_model->deleteItem( cur_net_row );
        return;
    }

    std::unique_ptr<LIST_ITEM> new_list_item = buildNewItem( aNet, node_count,
                                                             relevantConnectivityItems() );

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

        cur_list_item->SetChipWireLength( new_list_item->GetChipWireLength() );

        updateDisplayedRowValues( cur_net_row );
    }
}


unsigned int DIALOG_NET_INSPECTOR::calculateViaLength( const PCB_TRACK* aTrack ) const
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
        // FIXME: not all dielectric layers are the same thickness!
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


std::unique_ptr<DIALOG_NET_INSPECTOR::LIST_ITEM>
DIALOG_NET_INSPECTOR::buildNewItem( NETINFO_ITEM* aNet, unsigned int aPadCount,
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
            new_item->AddChipWireLength( static_cast<PAD*>( item )->GetPadToDieLength() );

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


void DIALOG_NET_INSPECTOR::buildNetsList()
{
    // Only build the list of nets if there is a board present
    if( !m_brd )
        return;

    m_in_build_nets_list = true;

    // when rebuilding the netlist, try to keep the row selection
    // FIXME: handle group selections, preserve expanded/collapsed group states
    wxDataViewItemArray sel;
    m_netsList->GetSelections( sel );

    std::vector<int> prev_selected_netcodes;
    prev_selected_netcodes.reserve( sel.GetCount() );

    for( unsigned int i = 0; i < sel.GetCount(); ++i )
    {
        const LIST_ITEM* item = static_cast<const LIST_ITEM*>( sel.Item( i ).GetID() );
        prev_selected_netcodes.push_back( item->GetNetCode() );
    }

    m_data_model->deleteAllItems();

    std::vector<std::unique_ptr<LIST_ITEM>> new_items;

    // for group mode 0,1 each group filter string represents one displayed group,
    // so just add them first.  for group mode 2,3 the groups are generated dynamically.
    if( m_groupBy->IsChecked()
            && ( m_groupByKind->GetSelection() == 0 || m_groupByKind->GetSelection() == 1 ) )
    {
        for( unsigned int i = 0; i < m_groupFilter.size(); ++i )
        {
            const std::unique_ptr<EDA_PATTERN_MATCH>& filter = m_groupFilter[i];
            new_items.emplace_back( std::make_unique<LIST_ITEM>( i, filter->GetPattern() ) );
        }
    }

    std::vector<CN_ITEM*> prefiltered_cn_items = relevantConnectivityItems();


    // collect all nets which pass the filter string and also remember the
    // suffix after the filter match, if any.
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

    for( const std::pair<int, NETINFO_ITEM*> ni : m_brd->GetNetInfo().NetsByNetcode() )
    {
        if( ni.first == 0 )
            m_zero_netitem = ni.second;

        if( netFilterMatches( ni.second ) )
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
        if( m_cbShowZeroPad->IsChecked() || ni.pad_count > 0 )
            new_items.emplace_back( buildNewItem( ni.net, ni.pad_count, prefiltered_cn_items ) );
    }


    m_data_model->addItems( std::move( new_items ) );

    // try to restore the selected rows.  set the ones that we can't find anymore to -1.
    sel.Clear();

    for( int& nc : prev_selected_netcodes )
    {
        auto r = m_data_model->findItem( nc );

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

    m_frame->GetCanvas()->GetView()->GetPainter()->GetSettings()->SetHighlight( false );

    for( int& i : prev_selected_netcodes )
        m_frame->GetCanvas()->GetView()->GetPainter()->GetSettings()->SetHighlight( true, i, true );

    m_frame->GetCanvas()->GetView()->UpdateAllLayersColor();
    m_frame->GetCanvas()->Refresh();

    m_in_build_nets_list = false;
}


void DIALOG_NET_INSPECTOR::onFilterChange( wxCommandEvent& aEvent )
{
    wxStringTokenizer filters( m_textCtrlFilter->GetValue().Upper(), wxT( "," ) );
    m_netFilter.clear();

    while( filters.HasMoreTokens() )
    {
        wxString t = filters.GetNextToken();
        t.Trim( false );
        t.Trim( true );

        if( !t.IsEmpty() )
        {
            m_netFilter.emplace_back( std::make_unique<EDA_PATTERN_MATCH_WILDCARD>() );
            m_netFilter.back()->SetPattern( t );
        }
    }

    wxStringTokenizer group_filters( m_groupByText->GetValue(), wxT( "," ) );
    m_groupFilter.clear();

    while( group_filters.HasMoreTokens() )
    {
        wxString t = group_filters.GetNextToken();
        t.Trim( false );
        t.Trim( true );

        if( !t.IsEmpty() )
        {
            if( m_groupByKind->GetSelection() == 0 || m_groupByKind->GetSelection() == 2 )
            {
                // type 2: wildcard match, use the matching substring as a group key.
                // the number of groups is determined dynamically by the number of
                // resulting matches in the whole set.
                m_groupFilter.emplace_back( std::make_unique<EDA_PATTERN_MATCH_WILDCARD>() );
                m_groupFilter.back()->SetPattern( t );
            }
            else if( m_groupByKind->GetSelection() == 1 || m_groupByKind->GetSelection() == 3 )
            {
                // type 3: regex match, use the matching substring as a group key.
                // the number of groups is determined dynamically by the number of
                // resulting matches in the whole set.
                m_groupFilter.emplace_back( std::make_unique<EDA_PATTERN_MATCH_REGEX>() );
                m_groupFilter.back()->SetPattern( t );
            }
        }
    }

    if( !m_filter_change_no_rebuild )
        buildNetsList();
}


void DIALOG_NET_INSPECTOR::onSelChanged( wxDataViewEvent&  )
{
    onSelChanged();
}


void DIALOG_NET_INSPECTOR::onSelChanged()
{
    // ignore selection changes while the whole list is being rebuilt.
    if( m_in_build_nets_list )
        return;

    RENDER_SETTINGS* ps = m_frame->GetCanvas()->GetView()->GetPainter()->GetSettings();

    bool enable_rename_button = false;
    bool enable_delete_button = false;

    if( m_netsList->HasSelection() )
    {
        wxDataViewItemArray sel;
        m_netsList->GetSelections( sel );

        ps->SetHighlight( false );

        enable_rename_button = sel.GetCount() == 1;
        enable_delete_button = true;

        for( unsigned int i = 0; i < sel.GetCount(); ++i )
        {
            const LIST_ITEM* ii = static_cast<const LIST_ITEM*>( sel.Item( i ).GetID() );

            if( ii->GetIsGroup() )
            {
                enable_rename_button = false;

                for( auto c = ii->ChildrenBegin(), end = ii->ChildrenEnd(); c != end; ++c )
                    ps->SetHighlight( true, ( *c )->GetNetCode(), true );
            }
            else
                ps->SetHighlight( true, ii->GetNetCode(), true );
        }
    }
    else
        ps->SetHighlight( false );

    m_frame->GetCanvas()->GetView()->UpdateAllLayersColor();
    m_frame->GetCanvas()->Refresh();

    m_renameNet->Enable( enable_rename_button );
    m_deleteNet->Enable( enable_delete_button );
}


void DIALOG_NET_INSPECTOR::onSortingChanged( wxDataViewEvent& aEvent )
{
    // FIXME: Whenever the sort criteria changes (sorting column)
    // the visible row-numers of the selection get preserved, not the actual
    // elements.  Don't know at the moment how to preserve the selection,
    // so just clear it for now.

    m_netsList->UnselectAll();

    m_frame->GetCanvas()->GetView()->GetPainter()->GetSettings()->SetHighlight( false );
    m_frame->GetCanvas()->GetView()->UpdateAllLayersColor();
    m_frame->GetCanvas()->Refresh();
}


void DIALOG_NET_INSPECTOR::adjustListColumns()
{
    wxWindowUpdateLocker locker( m_netsList );
    std::vector<int> widths;

    widths.reserve( m_columns.size() );

    for( size_t ii = 0; ii < m_columns.size(); ++ii )
        widths.push_back( GetTextExtent( m_columns[ii].display_name ).x );

    int minValueWidth = GetTextExtent( wxT( "00000,000 mm" ) ).x;
    int minNumberWidth = GetTextExtent( wxT( "000" ) ).x;
    int minNameWidth = GetTextExtent( wxT( "MMMMMM" ) ).x;

    // Considering left and right margins.
    // For wxRenderGeneric it is 5px.
    // Also account for the sorting arrow in the column header.
    // Column 0 also needs space for any potential expander icons.
    const int margins = 15;
    const int extra_width = 30;

    widths[0] = std::max( widths[0], minNumberWidth ) + extra_width;
    widths[1] = std::max( widths[1], minNameWidth )   + margins;
    widths[2] = std::max( widths[2], minNumberWidth ) + margins;
    widths[3] = std::max( widths[3], minNumberWidth ) + margins;

    for( size_t ii = 4; ii < widths.size(); ++ii )
    {
        widths[ii] = std::max( widths[ii], minValueWidth ) + margins;
    }

    // the columns might have been reordered.  we work on the column model numbers though.
    std::vector<int> column_order( m_data_model->columnCount() );

    for( size_t i = 0; i < column_order.size(); ++i )
    {
        column_order[m_netsList->GetColumn( i )->GetModelColumn()] = i;
        m_netsList->GetColumn( column_order[i] )->SetMinWidth( widths[i] );
        m_netsList->GetColumn( column_order[i] )->SetWidth( widths[i] );
    }

    // At resizing of the list the width of middle column (Net Names) changes only.
    int width = m_netsList->GetClientSize().x - 24;
    int remaining = width - std::accumulate( widths.begin() + 2, widths.end(), widths[0] );

    if( remaining > widths[1] )
        m_netsList->GetColumn( column_order[1] )->SetWidth( remaining );

    m_netsList->Refresh();

    // Force refresh on GTK so that horizontal scroll bar won't appear
#ifdef __WXGTK__
    wxPoint pos = m_netsList->GetPosition();
    m_netsList->Move( pos.x, pos.y + 1 );
    m_netsList->Move( pos.x, pos.y );
#endif
}


void DIALOG_NET_INSPECTOR::onListSize( wxSizeEvent& aEvent )
{
    auto new_size = aEvent.GetSize();

    if( new_size != m_size )
    {
        m_size = new_size;
        adjustListColumns();
    }

    aEvent.Skip();
}


void DIALOG_NET_INSPECTOR::onAddNet( wxCommandEvent& aEvent )
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

    // We'll get an OnBoardItemAdded callback from this to update our listbox
}


void DIALOG_NET_INSPECTOR::onRenameNet( wxCommandEvent& aEvent )
{
    if( m_netsList->GetSelectedItemsCount() == 1 )
    {
        const LIST_ITEM* sel = static_cast<const LIST_ITEM*>( m_netsList->GetSelection().GetID() );

        if( sel->GetIsGroup() )
            return;

        NETINFO_ITEM* net         = sel->GetNet();
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

        // the changed name might require re-grouping.  remove and re-insert
        // is easier.
        auto removed_item = m_data_model->deleteItem( m_data_model->findItem( net ) );

        m_brd->GetNetInfo().RemoveNet( net );
        net->SetNetname( fullNetName );
        m_brd->GetNetInfo().AppendNet( net );
        m_frame->OnModify();

        if( netFilterMatches( net ) )
        {
            std::unique_ptr<LIST_ITEM> new_item = std::make_unique<LIST_ITEM>( net );
            new_item->SetPadCount( removed_item->GetPadCount() );
            new_item->SetViaCount( removed_item->GetViaCount() );

            for( int ii = 0; ii < MAX_CU_LAYERS; ++ii )
                new_item->SetLayerWireLength( removed_item->GetLayerWireLength( ii ), ii );

            new_item->SetChipWireLength( removed_item->GetChipWireLength() );

            std::optional<LIST_ITEM_ITER> added_row = m_data_model->addItem( std::move( new_item ) );

            wxDataViewItemArray new_sel;
            new_sel.Add( wxDataViewItem( &***added_row ) );
            m_netsList->SetSelections( new_sel );
            onSelChanged();
        }

        // Currently only tracks and pads have netname annotations and need to be redrawn,
        // but zones are likely to follow.  Since we don't have a way to ask what is current,
        // just refresh all items.
        m_frame->GetCanvas()->GetView()->UpdateAllItems( KIGFX::REPAINT );
        m_frame->GetCanvas()->Refresh();
    }
}


void DIALOG_NET_INSPECTOR::onDeleteNet( wxCommandEvent& aEvent )
{
    if( !m_netsList->HasSelection() )
        return;

    wxDataViewItemArray sel;
    m_netsList->GetSelections( sel );

    auto delete_one =
            [this]( const LIST_ITEM* i )
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


void DIALOG_NET_INSPECTOR::onReport( wxCommandEvent& aEvent )
{
    wxFileDialog dlg( this, _( "Report file" ), "", "",
                      _( "Report file" ) + AddFileExtListToFilter( { "csv" } ),
                      wxFD_SAVE );

    if( dlg.ShowModal() == wxID_CANCEL )
       return;

    wxTextFile f( dlg.GetPath() );

    f.Create();

    wxString txt;

    // Print Header:
    for( auto&& col : m_columns )
        txt += '"' + col.csv_name + wxT( "\";" );

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

    f.Write();
    f.Close();
}


void DIALOG_NET_INSPECTOR::onClose( wxCloseEvent& aEvent )
{
    // Dialog is mode-less so let the parent know that it needs to be destroyed.
    if( !IsModal() && !IsQuasiModal() )
    {
        wxCommandEvent* evt = new wxCommandEvent( EDA_EVT_CLOSE_NET_INSPECTOR_DIALOG, wxID_ANY );

        evt->SetEventObject( this );
        wxWindow* parent = GetParent();

        if( parent )
            wxQueueEvent( parent, evt );
    }

    aEvent.Skip();
}
