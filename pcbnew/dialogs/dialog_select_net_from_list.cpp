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

#include <kicad_string.h>
#include <pcbnew.h>
#include <tools/pcb_inspection_tool.h>
#include <class_board.h>
#include <class_track.h>
#include <dialog_select_net_from_list.h>
#include <eda_pattern_match.h>
#include <kicad_string.h>
#include <wildcards_and_files_ext.h>
#include <view/view.h>
#include <view/view_controls.h>
#include <pcb_painter.h>
#include <connectivity/connectivity_data.h>
#include <connectivity/connectivity_algo.h>
#include <dialogs/dialog_text_entry.h>
#include <validators.h>
#include <bitmaps.h>
#include <wx/tokenzr.h>
#include <bitset>

struct DIALOG_SELECT_NET_FROM_LIST::COLUMN_DESC
{
    enum
    {
        CSV_NONE  = 0,
        CSV_QUOTE = 1 << 0
    };

    unsigned int num;
    wxString     display_name;
    wxString     csv_name;
    unsigned int csv_flags;

    operator unsigned int() const
    {
        return num;
    }
};


#define def_col( c, num, name, csv_name, csv_flags )                                             \
    const DIALOG_SELECT_NET_FROM_LIST::COLUMN_DESC DIALOG_SELECT_NET_FROM_LIST::c = { num, name, \
        csv_name, COLUMN_DESC::csv_flags }

def_col( COLUMN_NET,          0, _( "Net" ),          _( "Net Code" ),     CSV_NONE );
def_col( COLUMN_NAME,         1, _( "Name" ),         _( "Net Name" ),     CSV_QUOTE );
def_col( COLUMN_PAD_COUNT,    2, _( "Pad Count" ),    _( "Pad Count" ),    CSV_NONE );
def_col( COLUMN_VIA_COUNT,    3, _( "Via Count" ),    _( "Via Count" ),    CSV_NONE );
def_col( COLUMN_VIA_LENGTH,   4, _( "Via Length" ),   _( "Via Length" ),   CSV_NONE );
def_col( COLUMN_BOARD_LENGTH, 5, _( "Board Length" ), _( "Board Length" ), CSV_NONE );
def_col( COLUMN_CHIP_LENGTH,  6, _( "Die Length" ),   _( "Die Length" ),   CSV_NONE );
def_col( COLUMN_TOTAL_LENGTH, 7, _( "Total Length" ), _( "Net Length" ),   CSV_NONE );

#undef def_col


class DIALOG_SELECT_NET_FROM_LIST::LIST_ITEM
{
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
    uint64_t      m_board_wire_length = 0;
    uint64_t      m_chip_wire_length  = 0;

    // dirty bits to record when some attribute has changed.  this is to
    // avoid unnecessary resort operations.
    std::bitset<5> m_column_changed;

    // cached formatted net name for faster display sorting.
    wxString m_net_name;

public:
    LIST_ITEM( unsigned int aGroupNumber, const wxString& aGroupName )
            : m_is_group( true ), m_group_number( aGroupNumber ), m_net_name( aGroupName )
    {
    }

    LIST_ITEM( NETINFO_ITEM* aNet ) : m_net( aNet )
    {
        m_net_name = UnescapeString( aNet->GetNetname() );
    }

    LIST_ITEM& operator=( const LIST_ITEM& ) = delete;

    bool isGroup() const
    {
        return m_is_group;
    }

    unsigned int groupNumber() const
    {
        return m_group_number;
    }

    auto childrenBegin() const
    {
        return m_children.begin();
    }

    auto childrenEnd() const
    {
        return m_children.end();
    }

    unsigned int childrenCount() const
    {
        return m_children.size();
    }

    NETINFO_ITEM* net() const
    {
        return m_net;
    }

    int netCode() const
    {
        return isGroup() ? ( 0 - int( m_group_number ) - 1 ) : m_net->GetNet();
    }

    const wxString& netName() const
    {
        return m_net_name;
    }

    const wxString& groupName() const
    {
        return m_net_name;
    }

    void resetColumnChangedBits()
    {
        m_column_changed.reset();
    }

#define gen( mvar, chg_bit, get, set, add, sub, changed )                           \
    decltype( mvar ) get() const                                                    \
    {                                                                               \
        return mvar;                                                                \
    }                                                                               \
                                                                                    \
    bool changed() const                                                            \
    {                                                                               \
        return m_column_changed[chg_bit];                                           \
    }                                                                               \
                                                                                    \
    void set( const decltype( mvar )& aValue )                                      \
    {                                                                               \
        if( m_parent )                                                              \
            m_parent->set( m_parent->get() - mvar + aValue );                       \
                                                                                    \
        static_assert( chg_bit < decltype( m_column_changed )().size() );           \
        m_column_changed[chg_bit] = m_column_changed[chg_bit] | ( mvar != aValue ); \
        mvar                      = aValue;                                         \
    }                                                                               \
                                                                                    \
    void add( const decltype( mvar )& aValue )                                      \
    {                                                                               \
        if( m_parent )                                                              \
            m_parent->add( aValue );                                                \
                                                                                    \
        static_assert( chg_bit < decltype( m_column_changed )().size() );           \
        m_column_changed[chg_bit] = m_column_changed[chg_bit] | ( aValue != 0 );    \
        mvar += aValue;                                                             \
    }                                                                               \
                                                                                    \
    void sub( const decltype( mvar )& aValue )                                      \
    {                                                                               \
        if( m_parent )                                                              \
            m_parent->sub( aValue );                                                \
                                                                                    \
        static_assert( chg_bit < decltype( m_column_changed )().size() );           \
        m_column_changed[chg_bit] = m_column_changed[chg_bit] | ( aValue != 0 );    \
        mvar -= aValue;                                                             \
    }

    gen( m_pad_count, 0, padCount, setPadCount, addPadCount, subPadCount, padCountChanged );
    gen( m_via_count, 1, viaCount, setViaCount, addViaCount, subViaCount, viaCountChanged );
    gen( m_via_length, 2, viaLength, setViaLength, addViaLength, subViaLength, viaLengthChanged );
    gen( m_board_wire_length, 3, boardWireLength, setBoardWireLength, addBoardWireLength,
            subBoardWireLength, boardWireLengthChanged );
    gen( m_chip_wire_length, 4, chipWireLength, setChipWireLength, addChipWireLength,
            subChipWireLength, chipWireLengthChanged );

#undef gen

    // the total length column is always computed, never stored.
    auto totalLength() const
    {
        return boardWireLength() + viaLength() + chipWireLength();
    }

    bool totalLengthChanged() const
    {
        return boardWireLengthChanged() | viaLengthChanged() | chipWireLengthChanged();
    }

    LIST_ITEM* parent() const
    {
        return m_parent;
    }

    void setParent( LIST_ITEM* aParent )
    {
        if( m_parent == aParent )
            return;

        if( m_parent != nullptr )
        {
            m_parent->subPadCount( padCount() );
            m_parent->subViaCount( viaCount() );
            m_parent->subViaLength( viaLength() );
            m_parent->subBoardWireLength( boardWireLength() );
            m_parent->subChipWireLength( chipWireLength() );

            m_parent->m_children.erase(
                    std::find( m_parent->m_children.begin(), m_parent->m_children.end(), this ) );
        }

        m_parent = aParent;

        if( m_parent != nullptr )
        {
            m_parent->addPadCount( padCount() );
            m_parent->addViaCount( viaCount() );
            m_parent->addViaLength( viaLength() );
            m_parent->addBoardWireLength( boardWireLength() );
            m_parent->addChipWireLength( chipWireLength() );

            m_parent->m_children.push_back( this );
        }
    }
};


struct DIALOG_SELECT_NET_FROM_LIST::LIST_ITEM_NETCODE_CMP_LESS
{
    template <typename T>
    bool operator()( const T& a, const T& b ) const
    {
        return a->netCode() < b->netCode();
    }

    template <typename T>
    bool operator()( const T& a, int b ) const
    {
        return a->netCode() < b;
    }

    template <typename T>
    bool operator()( int a, const T& b ) const
    {
        return a < b->netCode();
    }
};


class DIALOG_SELECT_NET_FROM_LIST::DATA_MODEL : public wxDataViewModel
{
private:
    DIALOG_SELECT_NET_FROM_LIST& m_parent;

    // primary container, sorted by netcode number.
    // groups have netcode < 0, so they always come first, in the order
    // of the filter strings as input by the user (group mode 0, 1) or
    // in order of occurance (group mode 2, 3).
    std::vector<std::unique_ptr<LIST_ITEM>> m_items;

    bool m_sort_groups_first = false;

public:
    static const auto& columnDesc()
    {
        static const std::array<COLUMN_DESC, 8> r = { { COLUMN_NET, COLUMN_NAME, COLUMN_PAD_COUNT,
                COLUMN_VIA_COUNT, COLUMN_VIA_LENGTH, COLUMN_BOARD_LENGTH, COLUMN_CHIP_LENGTH,
                COLUMN_TOTAL_LENGTH } };

        return r;
    }

    DATA_MODEL( DIALOG_SELECT_NET_FROM_LIST& parent ) : m_parent( parent )
    {
    }

    unsigned int columnCount() const
    {
        return columnDesc().size();
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

    OPT<LIST_ITEM_ITER> findItem( int aNetCode )
    {
        auto i = std::lower_bound(
                m_items.begin(), m_items.end(), aNetCode, LIST_ITEM_NETCODE_CMP_LESS() );

        if( i == m_items.end() || ( *i )->netCode() != aNetCode )
            return {};

        return { i };
    }


    OPT<LIST_ITEM_ITER> findItem( NETINFO_ITEM* aNet )
    {
        if( aNet != nullptr )
            return findItem( aNet->GetNet() );
        else
            return {};
    }


    OPT<LIST_ITEM_ITER> addItem( std::unique_ptr<LIST_ITEM> aItem )
    {
        if( aItem == nullptr )
            return {};

        // make sure that the vector is always sorted.  usually when new nets are added,
        // they always get a higher netcode number than the already existing ones.
        // however, if we've got filtering enabled, we might not have all the nets in
        // our list, so do a sorted insertion.

        auto new_i = std::lower_bound(
                m_items.begin(), m_items.end(), aItem->netCode(), LIST_ITEM_NETCODE_CMP_LESS() );

        new_i = m_items.insert( new_i, std::move( aItem ) );

        if( m_parent.m_groupBy->IsChecked()
                && ( m_parent.m_groupByKind->GetSelection() == 0
                        || m_parent.m_groupByKind->GetSelection() == 1 ) )
        {
            for( unsigned int j = 0; j < m_parent.m_groupFilter.size(); ++j )
            {
                if( m_parent.m_groupFilter[j]->Find( ( *new_i )->netName() ) )
                {
                    ( *new_i )->setParent( &*m_items[j] );
                    break;
                }
            }
        }
        else if( m_parent.m_groupBy->IsChecked()
                 && ( m_parent.m_groupByKind->GetSelection() == 2
                         || m_parent.m_groupByKind->GetSelection() == 3 ) )
        {
            auto groups_begin = m_items.begin();
            auto groups_end   = std::find_if_not(
                    m_items.begin(), m_items.end(), []( const auto& x ) { return x->isGroup(); } );

            for( auto&& f : m_parent.m_groupFilter )
            {
                auto match = f->Find( ( *new_i )->netName() );

                if( match )
                {
                    wxString match_str = ( *new_i )->netName().substr( match.start, match.length );

                    auto g = std::find_if( groups_begin, groups_end,
                            [&]( const auto& x ) { return x->netName() == match_str; } );

                    if( g == groups_end )
                    {
                        g = m_items.insert( groups_end,
                                std::make_unique<LIST_ITEM>(
                                        std::distance( groups_end, groups_begin ), match_str ) );

                        groups_end = g + 1;

                        ItemAdded( wxDataViewItem( ( *g )->parent() ), wxDataViewItem( &**g ) );
                    }

                    ( *new_i )->setParent( &**g );
                    break;
                }
            }
        }

        ItemAdded( wxDataViewItem( ( *new_i )->parent() ), wxDataViewItem( &**new_i ) );

        return { new_i };
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
                        if( m_parent.m_groupFilter[j]->Find( m_items[i]->netName() ) )
                        {
                            m_items[i]->setParent( &*m_items[j] );
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

                for( auto&& i : m_items )
                {
                    for( auto&& f : m_parent.m_groupFilter )
                    {
                        auto match = f->Find( i->netName() );

                        if( match )
                        {
                            wxString match_str = i->netName().substr( match.start, match.length );

                            auto g = std::find_if( groups.begin(), groups.end(),
                                    [&]( const auto& x ) { return x->netName() == match_str; } );

                            if( g == groups.end() )
                            {
                                groups.emplace_back(
                                        std::make_unique<LIST_ITEM>( groups.size(), match_str ) );
                                g = groups.end() - 1;
                            }

                            i->setParent( &**g );
                            break;
                        }
                    }
                }

                // insert the group items at the front of the items list.
                for( auto&& g : groups )
                    m_items.emplace_back( std::move( g ) );

                std::rotate( m_items.begin(), m_items.end() - groups.size(), m_items.end() );
            }

            for( auto&& i : m_items )
                ItemAdded( wxDataViewItem( i->parent() ), wxDataViewItem( &*i ) );
        }
        else
        {
            m_items.reserve( m_items.size() + aItems.size() );

            for( auto&& i : aItems )
                addItem( std::move( i ) );
        }
    }

    std::unique_ptr<LIST_ITEM> deleteItem( const OPT<LIST_ITEM_ITER>& aRow )
    {
        if( !aRow )
            return {};

        std::unique_ptr<LIST_ITEM> i = std::move( **aRow );

        // if the row has a parent, detach it first
        auto parent = i->parent();
        i->setParent( nullptr );

        m_items.erase( *aRow );

        ItemDeleted( wxDataViewItem( parent ), wxDataViewItem( &*i ) );

        ItemChanged( wxDataViewItem( parent ) );

        // for grouping type 2,3 a group item might disappear if it becomes empty.
        if( ( m_parent.m_groupByKind->GetSelection() == 2
                    || m_parent.m_groupByKind->GetSelection() == 3 )
                && parent != nullptr && parent->childrenCount() == 0 )
        {
            auto p = std::find_if(
                    m_items.begin(), m_items.end(), [&]( auto& x ) { return x.get() == parent; } );
            assert( p != m_items.end() );
            m_items.erase( p );

            ItemDeleted( wxDataViewItem( parent->parent() ), wxDataViewItem( parent ) );
        }
        else
            resortIfChanged( parent );

        return i;
    }


    void deleteAllItems()
    {
        m_items.clear();
        Cleared();
    }


    void updateItem( const OPT<LIST_ITEM_ITER>& aRow )
    {
        if( !aRow )
            return;

        if( ( **aRow )->parent() )
            ItemChanged( wxDataViewItem( ( **aRow )->parent() ) );

        ItemChanged( wxDataViewItem( &***aRow ) );
        resortIfChanged( &***aRow );
    }


    void updateAllItems()
    {
        for( auto&& i : m_items )
            ItemChanged( wxDataViewItem( &*i ) );
    }


    void resortIfChanged( LIST_ITEM* aItem )
    {
        if( auto* c = m_parent.m_netsList->GetSortingColumn() )
        {
            bool changed = false;

            for( const LIST_ITEM* i = aItem; i != nullptr; i = i->parent() )
                changed |= itemColumnChanged( i, c->GetModelColumn() );

            for( LIST_ITEM* i = aItem; i != nullptr; i = i->parent() )
                i->resetColumnChangedBits();

            if( changed )
                Resort();
        }
    }


    bool itemColumnChanged( const LIST_ITEM* aItem, unsigned int aCol ) const
    {
        if( aItem == nullptr || aCol >= columnDesc().size() )
            return false;

        if( aCol == COLUMN_PAD_COUNT )
            return aItem->padCountChanged();

        else if( aCol == COLUMN_VIA_COUNT )
            return aItem->viaCountChanged();

        else if( aCol == COLUMN_VIA_LENGTH )
            return aItem->viaLengthChanged();

        else if( aCol == COLUMN_BOARD_LENGTH )
            return aItem->boardWireLengthChanged();

        else if( aCol == COLUMN_CHIP_LENGTH )
            return aItem->chipWireLengthChanged();

        else if( aCol == COLUMN_TOTAL_LENGTH )
            return aItem->totalLengthChanged();

        return false;
    }


    void setSortGroupsFirst( bool aValue )
    {
        if( aValue == m_sort_groups_first )
            return;

        m_sort_groups_first = aValue;

        if( m_parent.m_netsList->GetSortingColumn() )
            Resort();
    }


    bool isSortGroupsFirst() const
    {
        return m_sort_groups_first;
    }

    // implementation of wxDataViewModel interface
    // these are used to query the data model by the GUI view implementation.
    // these are not supposed to be used to modify the data model.  for that
    // use the public functions above.

protected:
    virtual unsigned int GetColumnCount() const override
    {
        return columnCount();
    }

    virtual void GetValue(
            wxVariant& aOutValue, const wxDataViewItem& aItem, unsigned int aCol ) const override
    {
        if( auto&& i = static_cast<LIST_ITEM*>( aItem.GetID() ) )
        {
            if( aCol == COLUMN_NET && !i->isGroup() )
                aOutValue = m_parent.formatNetCode( i->net() );

            else if( aCol == COLUMN_NET && i->isGroup() )
                aOutValue = "";

            else if( aCol == COLUMN_NAME )
                aOutValue = i->netName();

            else if( aCol == COLUMN_PAD_COUNT )
                aOutValue = m_parent.formatCount( i->padCount() );

            else if( aCol == COLUMN_VIA_COUNT )
                aOutValue = m_parent.formatCount( i->viaCount() );

            else if( aCol == COLUMN_VIA_LENGTH )
                aOutValue = m_parent.formatLength( i->viaLength() );

            else if( aCol == COLUMN_BOARD_LENGTH )
                aOutValue = m_parent.formatLength( i->boardWireLength() );

            else if( aCol == COLUMN_CHIP_LENGTH )
                aOutValue = m_parent.formatLength( i->chipWireLength() );

            else if( aCol == COLUMN_TOTAL_LENGTH )
                aOutValue = m_parent.formatLength( i->totalLength() );
        }
    }

    static int compareUInt( uint64_t aValue1, uint64_t aValue2, bool aAsc )
    {
        if( aAsc )
            return aValue1 < aValue2 ? -1 : 1;
        else
            return aValue2 < aValue1 ? -1 : 1;
    }

    virtual int Compare( const wxDataViewItem& aItem1, const wxDataViewItem& aItem2,
            unsigned int aCol, bool aAsc ) const override
    {
        const LIST_ITEM& i1 = *static_cast<const LIST_ITEM*>( aItem1.GetID() );
        const LIST_ITEM& i2 = *static_cast<const LIST_ITEM*>( aItem2.GetID() );

        if( isSortGroupsFirst() )
        {
            if( i1.isGroup() && !i2.isGroup() )
                return -1;
            if( i2.isGroup() && !i1.isGroup() )
                return 1;
        }

        if( aCol == COLUMN_NET && i1.netCode() != i2.netCode() )
            return aAsc ? ( i2.netCode() - i1.netCode() ) : ( i1.netCode() - i2.netCode() );

        else if( aCol == COLUMN_NAME )
        {
            const wxString& s1 = i1.netName();
            const wxString& s2 = i2.netName();

            int res = aAsc ? s1.Cmp( s2 ) : s2.Cmp( s1 );
            if( res != 0 )
                return res;
        }

        else if( aCol == COLUMN_PAD_COUNT && i1.padCount() != i2.padCount() )
            return compareUInt( i1.padCount(), i2.padCount(), aAsc );

        else if( aCol == COLUMN_VIA_COUNT && i1.viaCount() != i2.viaCount() )
            return compareUInt( i1.viaCount(), i2.viaCount(), aAsc );

        else if( aCol == COLUMN_VIA_LENGTH && i1.viaLength() != i2.viaLength() )
            return compareUInt( i1.viaLength(), i2.viaLength(), aAsc );

        else if( aCol == COLUMN_BOARD_LENGTH && i1.boardWireLength() != i2.boardWireLength() )
            return compareUInt( i1.boardWireLength(), i2.boardWireLength(), aAsc );

        else if( aCol == COLUMN_CHIP_LENGTH && i1.chipWireLength() != i2.chipWireLength() )
            return compareUInt( i1.chipWireLength(), i2.chipWireLength(), aAsc );

        else if( aCol == COLUMN_TOTAL_LENGTH && i1.totalLength() != i2.totalLength() )
            return compareUInt( i1.totalLength(), i2.totalLength(), aAsc );

        // when the item values compare equal resort to pointer comparison.
        wxUIntPtr id1 = wxPtrToUInt( aItem1.GetID() );
        wxUIntPtr id2 = wxPtrToUInt( aItem2.GetID() );

        return aAsc ? id1 - id2 : id2 - id1;
    }

    virtual bool SetValue(
            const wxVariant& aInValue, const wxDataViewItem& aItem, unsigned int aCol ) override
    {
        return false;
    }

    virtual wxDataViewItem GetParent( const wxDataViewItem& aItem ) const override
    {
        if( !aItem.IsOk() )
            return wxDataViewItem();

        return wxDataViewItem( static_cast<const LIST_ITEM*>( aItem.GetID() )->parent() );
    }

    virtual bool IsContainer( const wxDataViewItem& aItem ) const override
    {
        if( !aItem.IsOk() )
            return true;

        return static_cast<const LIST_ITEM*>( aItem.GetID() )->isGroup();
    }

    virtual bool HasContainerColumns( const wxDataViewItem& aItem ) const override
    {
        return IsContainer( aItem );
    }

    virtual unsigned int GetChildren(
            const wxDataViewItem& aParent, wxDataViewItemArray& aChildren ) const override
    {
        auto&& p = static_cast<const LIST_ITEM*>( aParent.GetID() );

        if( !aParent.IsOk() )
        {
            aChildren.Alloc( m_items.size() );

            for( auto&& i : m_items )
            {
                if( i->parent() == nullptr )
                    aChildren.Add( wxDataViewItem( &*i ) );
            }

            return aChildren.GetCount();
        }
        else if( p->isGroup() )
        {
            const auto count = p->childrenCount();

            if( count == 0 )
                return 0;

            aChildren.Alloc( count );

            for( auto i = p->childrenBegin(), end = p->childrenEnd(); i != end; ++i )
                aChildren.Add( wxDataViewItem( *i ) );

            return aChildren.GetCount();
        }

        return 0;
    }

    virtual wxString GetColumnType( unsigned int /* aCol */ ) const override
    {
        return wxS( "string" );
    }
};


DIALOG_SELECT_NET_FROM_LIST::DIALOG_SELECT_NET_FROM_LIST( PCB_EDIT_FRAME* aParent,
                                                          const SETTINGS& aSettings ) :
        DIALOG_SELECT_NET_FROM_LIST_BASE( aParent ),
        m_frame( aParent )
{
    m_brd = aParent->GetBoard();

    m_data_model = new DATA_MODEL( *this );
    m_netsList->AssociateModel( &*m_data_model );

    std::array<std::function<void( void )>, 8> add_col = {
        [&]( void ) {
            m_netsList->AppendTextColumn( COLUMN_NET.display_name, COLUMN_NET,
                    wxDATAVIEW_CELL_INERT, -1, wxALIGN_LEFT, wxDATAVIEW_COL_SORTABLE );
        },
        [&]( void ) {
            m_netsList->AppendTextColumn( COLUMN_NAME.display_name, COLUMN_NAME,
                    wxDATAVIEW_CELL_INERT, -1, wxALIGN_LEFT,
                    wxDATAVIEW_COL_RESIZABLE | wxDATAVIEW_COL_REORDERABLE
                            | wxDATAVIEW_COL_SORTABLE );
        },
        [&]( void ) {
            m_netsList->AppendTextColumn( COLUMN_PAD_COUNT.display_name, COLUMN_PAD_COUNT,
                    wxDATAVIEW_CELL_INERT, -1, wxALIGN_CENTER,
                    wxDATAVIEW_COL_REORDERABLE | wxDATAVIEW_COL_SORTABLE );
        },
        [&]( void ) {
            m_netsList->AppendTextColumn( COLUMN_VIA_COUNT.display_name, COLUMN_VIA_COUNT,
                    wxDATAVIEW_CELL_INERT, -1, wxALIGN_CENTER,
                    wxDATAVIEW_COL_REORDERABLE | wxDATAVIEW_COL_SORTABLE );
        },
        [&]( void ) {
            m_netsList->AppendTextColumn( COLUMN_VIA_LENGTH.display_name, COLUMN_VIA_LENGTH,
                    wxDATAVIEW_CELL_INERT, -1, wxALIGN_CENTER,
                    wxDATAVIEW_COL_REORDERABLE | wxDATAVIEW_COL_SORTABLE );
        },
        [&]( void ) {
            m_netsList->AppendTextColumn( COLUMN_BOARD_LENGTH.display_name, COLUMN_BOARD_LENGTH,
                    wxDATAVIEW_CELL_INERT, -1, wxALIGN_CENTER,
                    wxDATAVIEW_COL_REORDERABLE | wxDATAVIEW_COL_SORTABLE );
        },
        [&]( void ) {
            m_netsList->AppendTextColumn( COLUMN_CHIP_LENGTH.display_name, COLUMN_CHIP_LENGTH,
                    wxDATAVIEW_CELL_INERT, -1, wxALIGN_CENTER,
                    wxDATAVIEW_COL_REORDERABLE | wxDATAVIEW_COL_SORTABLE );
        },
        [&]( void ) {
            m_netsList->AppendTextColumn( COLUMN_TOTAL_LENGTH.display_name, COLUMN_TOTAL_LENGTH,
                    wxDATAVIEW_CELL_INERT, -1, wxALIGN_CENTER,
                    wxDATAVIEW_COL_REORDERABLE | wxDATAVIEW_COL_SORTABLE );
        }
    };

    std::vector<int> col_order = aSettings.column_order;

    if( col_order.size() != add_col.size() )
    {
        col_order.resize( add_col.size() );

        for( unsigned int i = 0; i < add_col.size(); ++i )
            col_order[i] = i;
    }

    for( unsigned int i : col_order )
        add_col.at( i )();

    m_netsList->SetExpanderColumn( m_netsList->GetColumn( 0 ) );

    // avoid onFilterChange for each of the settings as it will re-build the
    // list over and over again.
    m_filter_change_no_rebuild = true;

    m_textCtrlFilter->SetValue( aSettings.filter_string );
    m_cbShowZeroPad->SetValue( aSettings.show_zero_pad_nets );
    m_groupBy->SetValue( aSettings.group_by );
    m_groupByKind->SetSelection( aSettings.group_by_kind );
    m_groupByText->SetValue( aSettings.group_by_text );
    m_viaLengthType->SetSelection( aSettings.via_length_type );
    m_constViaLengthValue = aSettings.const_via_length;
    m_constViaLength->SetValue( formatLength( m_constViaLengthValue ) );

    m_filter_change_no_rebuild = false;
    buildNetsList();

    adjustListColumns();

    m_addNet->SetBitmap( KiBitmap( small_plus_xpm ) );
    m_renameNet->SetBitmap( KiBitmap( small_edit_xpm ) );
    m_deleteNet->SetBitmap( KiBitmap( trash_xpm ) );

    m_sdbSizerOK->SetDefault();
    m_renameNet->Disable();
    m_deleteNet->Disable();

    m_data_model->setSortGroupsFirst( m_groupsFirst->IsChecked() );

    if( aSettings.sorting_column != -1 )
    {
        if( auto* c = m_netsList->GetColumn( aSettings.sorting_column ) )
            c->SetSortOrder( aSettings.sort_order_asc );
    }

    FinishDialogSettings();

#define connect_event( e, f ) \
    m_frame->Connect( e, wxCommandEventHandler( DIALOG_SELECT_NET_FROM_LIST::f ), nullptr, this )

    connect_event( wxEVT_CLOSE_WINDOW, onParentWindowClosed );
    connect_event( UNITS_CHANGED, onUnitsChanged );
    connect_event( BOARD_CHANGED, onBoardChanged );

#undef connect_event

    // if the dialog is opened while something is highlighted on the board ...
    OnBoardHighlightNetChanged( *m_brd );

    if( m_brd != nullptr )
        m_brd->AddListener( this );
}


DIALOG_SELECT_NET_FROM_LIST::~DIALOG_SELECT_NET_FROM_LIST()
{
    // the displayed list elements are going to be deleted before the list view itself.
    // in some cases it might still do queries on the data model, which would crash
    // from now on.  so just disassociate it.
    m_netsList->AssociateModel( nullptr );

#define disconnect_event( e, f ) \
    m_frame->Disconnect( e, wxCommandEventHandler( DIALOG_SELECT_NET_FROM_LIST::f ), nullptr, this )

    disconnect_event( wxEVT_CLOSE_WINDOW, onParentWindowClosed );
    disconnect_event( UNITS_CHANGED, onUnitsChanged );
    disconnect_event( BOARD_CHANGED, onBoardChanged );

#undef disconnect_event

    if( m_brd != nullptr )
        m_brd->RemoveListener( this );

    m_frame->GetCanvas()->SetFocus();
}


DIALOG_SELECT_NET_FROM_LIST::SETTINGS DIALOG_SELECT_NET_FROM_LIST::Settings() const
{
    std::vector<int> column_order( m_data_model->columnCount() );

    for( unsigned int i = 0; i < column_order.size(); ++i )
        column_order[i] = m_netsList->GetColumn( i )->GetModelColumn();

    auto* sorting_column = m_netsList->GetSortingColumn();

    SETTINGS r;
    r.filter_string      = m_textCtrlFilter->GetValue();
    r.show_zero_pad_nets = m_cbShowZeroPad->IsChecked();
    r.group_by           = m_groupBy->IsChecked();
    r.group_by_kind      = m_groupByKind->GetSelection();
    r.group_by_text      = m_groupByText->GetValue();
    r.sorting_column   = sorting_column ? static_cast<int>( sorting_column->GetModelColumn() ) : -1;
    r.sort_order_asc   = sorting_column ? sorting_column->IsSortOrderAscending() : true;
    r.column_order     = column_order;
    r.const_via_length = m_constViaLengthValue;
    r.via_length_type  = m_viaLengthType->GetSelection();

    return r;
}


void DIALOG_SELECT_NET_FROM_LIST::onParentWindowClosed( wxCommandEvent& event )
{
    Close();
    event.Skip();
}


void DIALOG_SELECT_NET_FROM_LIST::onUnitsChanged( wxCommandEvent& event )
{
    this->m_units = m_frame->GetUserUnits();

    m_constViaLength->SetValue( formatLength( m_constViaLengthValue ) );

    m_data_model->updateAllItems();

    event.Skip();
}


void DIALOG_SELECT_NET_FROM_LIST::onBoardChanged( wxCommandEvent& event )
{
    if( m_brd != nullptr )
        m_brd->RemoveListener( this );

    m_brd = m_frame->GetBoard();

    if( m_brd != nullptr )
        m_brd->AddListener( this );

    buildNetsList();
    m_netsList->Refresh();

    event.Skip();
}


bool DIALOG_SELECT_NET_FROM_LIST::netFilterMatches( NETINFO_ITEM* aNet ) const
{
    // Note: the filtering is case insensitive.

    if( m_netFilter.empty() )
        return true;

    auto net_str = UnescapeString( aNet->GetNetname() ).Upper();

    for( auto&& f : m_netFilter )
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


void DIALOG_SELECT_NET_FROM_LIST::updateDisplayedRowValues( const OPT<LIST_ITEM_ITER>& aRow )
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


wxString DIALOG_SELECT_NET_FROM_LIST::formatLength( int64_t aValue ) const
{
    return MessageTextFromValue( GetUserUnits(), static_cast<long long int>( aValue ) );
}


void DIALOG_SELECT_NET_FROM_LIST::OnBoardItemAdded( BOARD& aBoard, BOARD_ITEM* aBoardItem )
{
    if( NETINFO_ITEM* net = dynamic_cast<NETINFO_ITEM*>( aBoardItem ) )
    {
        // a new net has been added to the board.  add it to our list if it
        // passes the netname filter test.

        if( netFilterMatches( net ) )
        {
            auto&& new_item = std::make_unique<LIST_ITEM>( net );

            // the new net could have some pads already assigned, count them.
            new_item->setPadCount( m_brd->GetNodesCount( net->GetNet() ) );

            m_data_model->addItem( std::move( new_item ) );
        }
    }
    else if( auto* i = dynamic_cast<BOARD_CONNECTED_ITEM*>( aBoardItem ) )
    {
        auto r = m_data_model->findItem( i->GetNet() );

        if( r )
        {
            // try to handle frequent operations quickly.
            if( auto* track = dynamic_cast<TRACK*>( i ) )
            {
                int len = track->GetLength();
                ( **r )->addBoardWireLength( len );

                if( track->Type() == PCB_VIA_T )
                {
                    ( **r )->addViaCount( 1 );
                    ( **r )->addViaLength( calculateViaLength( track ) );
                }

                updateDisplayedRowValues( r );
                return;
            }
        }

        // resort to generic slower net update otherwise.
        updateNet( i->GetNet() );
    }
    else if( auto* mod = dynamic_cast<MODULE*>( aBoardItem ) )
    {
        for( const D_PAD* pad : mod->Pads() )
        {
            auto r = m_data_model->findItem( pad->GetNet() );

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
                int len = pad->GetPadToDieLength();
                ( **r )->addPadCount( 1 );
                ( **r )->addChipWireLength( len );

                if( ( **r )->padCount() == 0 && !m_cbShowZeroPad->IsChecked() )
                    m_data_model->deleteItem( r );
                else
                    updateDisplayedRowValues( r );
            }
        }
    }
}


void DIALOG_SELECT_NET_FROM_LIST::OnBoardItemRemoved( BOARD& aBoard, BOARD_ITEM* aBoardItem )
{
    if( auto* net = dynamic_cast<NETINFO_ITEM*>( aBoardItem ) )
    {
        m_data_model->deleteItem( m_data_model->findItem( net ) );
    }
    else if( auto* mod = dynamic_cast<MODULE*>( aBoardItem ) )
    {
        for( const D_PAD* pad : mod->Pads() )
        {
            auto r = m_data_model->findItem( pad->GetNet() );

            if( r )
            {
                int len = pad->GetPadToDieLength();
                ( **r )->subPadCount( 1 );
                ( **r )->subChipWireLength( len );

                if( ( **r )->padCount() == 0 && !m_cbShowZeroPad->IsChecked() )
                    m_data_model->deleteItem( r );
                else
                    updateDisplayedRowValues( r );
            }
        }
    }
    else if( auto* i = dynamic_cast<BOARD_CONNECTED_ITEM*>( aBoardItem ) )
    {
        auto r = m_data_model->findItem( i->GetNet() );

        if( r )
        {
            // try to handle frequent operations quickly.
            if( auto* track = dynamic_cast<TRACK*>( i ) )
            {
                int len = track->GetLength();
                ( **r )->subBoardWireLength( len );

                if( track->Type() == PCB_VIA_T )
                {
                    ( **r )->subViaCount( 1 );
                    ( **r )->subViaLength( calculateViaLength( track ) );
                }

                updateDisplayedRowValues( r );
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
    else
    {
        const auto& selected_codes = m_brd->GetHighLightNetCodes();

        wxDataViewItemArray new_selection;
        new_selection.Alloc( selected_codes.size() );

        for( int code : selected_codes )
        {
            if( auto r = m_data_model->findItem( code ) )
                new_selection.Add( wxDataViewItem( &***r ) );
        }

        m_netsList->SetSelections( new_selection );

        if( !new_selection.IsEmpty() )
            m_netsList->EnsureVisible( new_selection.Item( 0 ) );
    }
}


void DIALOG_SELECT_NET_FROM_LIST::OnBoardNetSettingsChanged( BOARD& aBoard )
{
    buildNetsList();
    m_netsList->Refresh();
}


void DIALOG_SELECT_NET_FROM_LIST::updateNet( NETINFO_ITEM* aNet )
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

    auto cur_net_row = m_data_model->findItem( aNet );

    const unsigned int node_count = m_brd->GetNodesCount( aNet->GetNet() );

    if( node_count == 0 && !m_cbShowZeroPad->IsChecked() )
    {
        m_data_model->deleteItem( cur_net_row );
        return;
    }

    auto list_item = buildNewItem( aNet, node_count, relevantConnectivityItems() );

    if( !cur_net_row )
        m_data_model->addItem( std::move( list_item ) );

    else if( ( **cur_net_row )->netName() != list_item->netName() )
    {
        // if the name has changed, it might require re-grouping.
        // it's easier to remove and re-insert it
        m_data_model->deleteItem( cur_net_row );
        m_data_model->addItem( std::move( list_item ) );
    }
    else
    {
        // update fields only
        ( **cur_net_row )->setPadCount( list_item->padCount() );
        ( **cur_net_row )->setViaCount( list_item->viaCount() );
        ( **cur_net_row )->setBoardWireLength( list_item->boardWireLength() );
        ( **cur_net_row )->setChipWireLength( list_item->chipWireLength() );

        updateDisplayedRowValues( cur_net_row );
    }
}


unsigned int DIALOG_SELECT_NET_FROM_LIST::calculateViaLength( const TRACK* aTrack ) const
{
    const VIA& via = dynamic_cast<const VIA&>( *aTrack );

    if( m_viaLengthType->GetSelection() == 0 )
    {
        return m_constViaLengthValue;
    }
    else if( m_viaLengthType->GetSelection() == 1 )
    {
        // calculate the via length individually from the board stackup and
        // via's start and end layer.
        const BOARD_STACKUP& stackup = m_brd->GetDesignSettings().GetStackupDescriptor();

        std::pair<PCB_LAYER_ID, int> layer_dist[2] = { std::make_pair( via.TopLayer(), 0 ),
            std::make_pair( via.BottomLayer(), 0 ) };

        for( const auto& i : stackup.GetList() )
        {
            for( auto& j : layer_dist )
            {
                if( j.first != UNDEFINED_LAYER )
                    j.second += i->GetThickness();

                if( j.first == i->GetBrdLayerId() )
                    j.first = UNDEFINED_LAYER;
            }
        }

        return std::abs( layer_dist[0].second - layer_dist[1].second );
    }
    else
        return 0;
}


std::unique_ptr<DIALOG_SELECT_NET_FROM_LIST::LIST_ITEM> DIALOG_SELECT_NET_FROM_LIST::buildNewItem(
        NETINFO_ITEM* aNet, unsigned int aPadCount, const std::vector<CN_ITEM*>& aCNItems )
{
    auto new_item = std::make_unique<LIST_ITEM>( aNet );

    new_item->setPadCount( aPadCount );

    const auto cn_items = std::equal_range(
            aCNItems.begin(), aCNItems.end(), aNet->GetNet(), NETCODE_CMP_LESS() );

    for( auto i = cn_items.first; i != cn_items.second; ++i )
    {
        auto item = ( *i )->Parent();

        if( item->Type() == PCB_PAD_T )
            new_item->addChipWireLength( static_cast<D_PAD*>( item )->GetPadToDieLength() );

        else if( auto* track = dynamic_cast<TRACK*>( item ) )
        {
            new_item->addBoardWireLength( track->GetLength() );

            if( item->Type() == PCB_VIA_T )
            {
                new_item->addViaCount( 1 );
                new_item->addViaLength( calculateViaLength( track ) );
            }
        }
    }

    return new_item;
}


void DIALOG_SELECT_NET_FROM_LIST::buildNetsList()
{
    m_in_build_nets_list = true;

    // when rebuilding the netlist, try to keep the row selection
    // FIXME: handle group selections, preserve expanded/collapsed group states
    wxDataViewItemArray sel;
    m_netsList->GetSelections( sel );

    std::vector<int> prev_selected_netcodes;
    prev_selected_netcodes.reserve( sel.GetCount() );

    for( unsigned int i = 0; i < sel.GetCount(); ++i )
        prev_selected_netcodes.push_back(
                static_cast<const LIST_ITEM*>( sel.Item( i ).GetID() )->netCode() );

    m_data_model->deleteAllItems();

    std::vector<std::unique_ptr<LIST_ITEM>> new_items;

    // for group mode 0,1 each group filter string represents one displayed group,
    // so just add them first.  for group mode 2,3 the groups are generated dynamically.
    if( m_groupBy->IsChecked()
            && ( m_groupByKind->GetSelection() == 0 || m_groupByKind->GetSelection() == 1 ) )
    {
        for( unsigned int i = 0; i < m_groupFilter.size(); ++i )
            new_items.emplace_back(
                    std::make_unique<LIST_ITEM>( i, m_groupFilter[i]->GetPattern() ) );
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

    for( auto&& ni : m_brd->GetNetInfo().NetsByNetcode() )
    {
        if( ni.first == 0 )
            m_zero_netitem = ni.second;

        if( netFilterMatches( ni.second ) )
            nets.emplace_back( NET_INFO{ ni.first, ni.second, 0 } );
    }

    // count the pads for each net.  since the nets are sorted by netcode
    // iterating over the modules' pads is faster.

    for( auto&& mod : m_brd->Modules() )
    {
        for( auto&& pad : mod->Pads() )
        {
            auto i = std::lower_bound(
                    nets.begin(), nets.end(), pad->GetNetCode(), NET_INFO_CMP_LESS() );

            if( i != nets.end() && i->netcode == pad->GetNetCode() )
                i->pad_count += 1;
        }
    }

    for( auto&& ni : nets )
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
            sel.Add( wxDataViewItem( &***r ) );
        else
            nc = -1;
    }

    if( !sel.IsEmpty() )
    {
        m_netsList->SetSelections( sel );
        m_netsList->EnsureVisible( sel.Item( 0 ) );
    }
    else
        m_netsList->UnselectAll();

    prev_selected_netcodes.erase(
            std::remove( prev_selected_netcodes.begin(), prev_selected_netcodes.end(), -1 ),
            prev_selected_netcodes.end() );

    m_frame->GetCanvas()->GetView()->GetPainter()->GetSettings()->SetHighlight( false );

    for( auto&& i : prev_selected_netcodes )
        m_frame->GetCanvas()->GetView()->GetPainter()->GetSettings()->SetHighlight( true, i, true );

    m_frame->GetCanvas()->GetView()->UpdateAllLayersColor();
    m_frame->GetCanvas()->Refresh();

    m_in_build_nets_list = false;
}


void DIALOG_SELECT_NET_FROM_LIST::onFilterChange( wxCommandEvent& aEvent )
{
    wxStringTokenizer filters( m_textCtrlFilter->GetValue().Upper(), "," );
    m_netFilter.clear();

    while( filters.HasMoreTokens() )
    {
        auto t = filters.GetNextToken();
        t.Trim( false );
        t.Trim( true );

        if( !t.IsEmpty() )
        {
            m_netFilter.emplace_back( std::make_unique<EDA_PATTERN_MATCH_WILDCARD>() );
            m_netFilter.back()->SetPattern( t );
        }
    }

    wxStringTokenizer group_filters( m_groupByText->GetValue(), "," );
    m_groupFilter.clear();

    while( group_filters.HasMoreTokens() )
    {
        auto t = group_filters.GetNextToken();
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


void DIALOG_SELECT_NET_FROM_LIST::onViaLengthChange( wxCommandEvent& event )
{
    m_constViaLengthValue = ValueFromString( GetUserUnits(), m_constViaLength->GetValue() );

    // allow editing of the constant via length value only when in constant via length mode.
    m_constViaLength->Enable( m_viaLengthType->GetSelection() == 0 );

    if( !m_filter_change_no_rebuild )
        buildNetsList();
}


void DIALOG_SELECT_NET_FROM_LIST::onGroupsFirstChanged( wxCommandEvent& event )
{
    m_data_model->setSortGroupsFirst( m_groupsFirst->IsChecked() );
}


void DIALOG_SELECT_NET_FROM_LIST::onSelChanged( wxDataViewEvent&  )
{
    onSelChanged();
}


void DIALOG_SELECT_NET_FROM_LIST::onSelChanged()
{
    // ignore selection changes while the whole list is being rebuilt.
    if( m_in_build_nets_list )
        return;

    bool enable_rename_button = false;
    bool enable_delete_button = false;

    if( m_netsList->HasSelection() )
    {
        wxDataViewItemArray sel;
        m_netsList->GetSelections( sel );

        auto&& ps = m_frame->GetCanvas()->GetView()->GetPainter()->GetSettings();

        ps->SetHighlight( false );

        enable_rename_button = sel.GetCount() == 1;
        enable_delete_button = true;

        for( unsigned int i = 0; i < sel.GetCount(); ++i )
        {
            auto&& ii = static_cast<const LIST_ITEM*>( sel.Item( i ).GetID() );

            if( ii->isGroup() )
            {
                enable_rename_button = false;

                for( auto c = ii->childrenBegin(), end = ii->childrenEnd(); c != end; ++c )
                    ps->SetHighlight( true, ( *c )->netCode(), true );
            }
            else
                ps->SetHighlight( true, ii->netCode(), true );
        }
    }
    else
        m_frame->GetCanvas()->GetView()->GetPainter()->GetSettings()->SetHighlight( false );

    m_frame->GetCanvas()->GetView()->UpdateAllLayersColor();
    m_frame->GetCanvas()->Refresh();

    m_renameNet->Enable( enable_rename_button );
    m_deleteNet->Enable( enable_delete_button );
}


void DIALOG_SELECT_NET_FROM_LIST::onSortingChanged( wxDataViewEvent& aEvent )
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


void DIALOG_SELECT_NET_FROM_LIST::adjustListColumns()
{
    /**
     * Calculating optimal width of the first (Net) and the last (Pad Count) columns.
     * That width must be enough to fit column header label and be not less than width of
     * four chars (0000).
     */

    wxClientDC dc( GetParent() );

    int h, minw, minw_col0, minw_col1;
    int w0, w1, w2, w3, w4, w5, w6, w7;

    dc.GetTextExtent( COLUMN_NET.display_name, &w0, &h );
    dc.GetTextExtent( COLUMN_NAME.display_name, &minw_col1, &h );
    dc.GetTextExtent( COLUMN_PAD_COUNT.display_name, &w2, &h );
    dc.GetTextExtent( COLUMN_VIA_COUNT.display_name, &w3, &h );
    dc.GetTextExtent( COLUMN_VIA_LENGTH.display_name, &w4, &h );
    dc.GetTextExtent( COLUMN_BOARD_LENGTH.display_name, &w5, &h );
    dc.GetTextExtent( COLUMN_CHIP_LENGTH.display_name, &w6, &h );
    dc.GetTextExtent( COLUMN_TOTAL_LENGTH.display_name, &w7, &h );
    dc.GetTextExtent( "00000,000 mm", &minw, &h );
    dc.GetTextExtent( "00000", &minw_col0, &h );

    // Considering left and right margins.
    // For wxRenderGeneric it is 5px.
    // Also account for the sorting arrow in the column header.
    // Column 0 also needs space for any potential expander icons.
    const int extra_width = 30;

    w0        = std::max( w0, minw_col0 ) + extra_width;
    minw_col1 = minw_col1 + extra_width;
    w2        = w2 + extra_width;
    w3        = w3 + extra_width;
    w4        = std::max( w4 + extra_width, minw );
    w5        = std::max( w5 + extra_width, minw );
    w6        = std::max( w6 + extra_width, minw );
    w7        = std::max( w7 + extra_width, minw );

    // the columns might have been reordered.  we work on the column model numbers though.
    std::vector<int> column_order( m_data_model->columnCount() );

    for( unsigned int i = 0; i < column_order.size(); ++i )
        column_order[m_netsList->GetColumn( i )->GetModelColumn()] = i;

    assert( column_order.size() == 8 );

    m_netsList->GetColumn( column_order[0] )->SetWidth( w0 );
    m_netsList->GetColumn( column_order[1] )->SetMinWidth( minw_col1 );
    m_netsList->GetColumn( column_order[2] )->SetWidth( w2 );
    m_netsList->GetColumn( column_order[3] )->SetWidth( w3 );
    m_netsList->GetColumn( column_order[4] )->SetWidth( w4 );
    m_netsList->GetColumn( column_order[5] )->SetWidth( w5 );
    m_netsList->GetColumn( column_order[6] )->SetWidth( w6 );
    m_netsList->GetColumn( column_order[7] )->SetWidth( w7 );

    // At resizing of the list the width of middle column (Net Names) changes only.
    int width = m_netsList->GetClientSize().x - 24;
    w1        = width - w0 - w2 - w3 - w4 - w5 - w6 - w7;

    // Column 1 (net names) need a minimal width to display net names
    dc.GetTextExtent( "MMMMMMMMMMMMMMMM", &minw, &h );
    w1 = std::max( w1, minw );

    m_netsList->GetColumn( column_order[1] )->SetWidth( w1 );

    m_netsList->Refresh();
}


void DIALOG_SELECT_NET_FROM_LIST::onListSize( wxSizeEvent& aEvent )
{
    aEvent.Skip();
    adjustListColumns();
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

    // We'll get an OnBoardItemAdded callback from this to update our listbox
}


void DIALOG_SELECT_NET_FROM_LIST::onRenameNet( wxCommandEvent& aEvent )
{
    if( m_netsList->GetSelectedItemsCount() == 1 )
    {
        auto&& sel = static_cast<const LIST_ITEM*>( m_netsList->GetSelection().GetID() );

        if( sel->isGroup() )
            return;

        NETINFO_ITEM* net         = sel->net();
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

        net->SetNetname( fullNetName );
        m_frame->OnModify();

        if( netFilterMatches( net ) )
        {
            auto new_item = std::make_unique<LIST_ITEM>( net );
            new_item->setPadCount( removed_item->padCount() );
            new_item->setViaCount( removed_item->viaCount() );
            new_item->setBoardWireLength( removed_item->boardWireLength() );
            new_item->setChipWireLength( removed_item->chipWireLength() );

            auto added_row = m_data_model->addItem( std::move( new_item ) );

            wxDataViewItemArray new_sel;
            new_sel.Add( wxDataViewItem( &***added_row ) );
            m_netsList->SetSelections( new_sel );
            onSelChanged();
        }
        else
            DisplayInfoMessage( this,
                    _( "New net name does not match current filter and will disappear from the list." ) );

        // Currently only tracks and pads have netname annotations and need to be redrawn,
        // but zones are likely to follow.  Since we don't have a way to ask what is current,
        // just refresh all items.
        m_frame->GetCanvas()->GetView()->UpdateAllItems( KIGFX::REPAINT );
        m_frame->GetCanvas()->Refresh();
    }
}


void DIALOG_SELECT_NET_FROM_LIST::onDeleteNet( wxCommandEvent& aEvent )
{
    if( !m_netsList->HasSelection() )
        return;

    wxDataViewItemArray sel;
    m_netsList->GetSelections( sel );

    auto delete_one = [this]( const LIST_ITEM* i ) {
        if( i->padCount() == 0
                || IsOK( this, wxString::Format( _( "Net '%s' is in use.  Delete anyway?" ),
                                       i->netName() ) ) )
        {
            m_brd->Remove( i->net() );
            m_frame->OnModify();

            // We'll get an OnBoardItemRemoved callback from this to update our listbox
        }
    };

    for( unsigned int i = 0; i < sel.GetCount(); ++i )
    {
        auto&& ii = static_cast<const LIST_ITEM*>( sel.Item( i ).GetID() );

        if( ii->isGroup() )
        {
            if( ii->childrenCount() != 0
                    && IsOK( this, wxString::Format( _( "Delete all nets in group '%s'?" ),
                                           ii->groupName() ) ) )
            {
                // we can't be iterating the children container and deleting items from
                // it at the same time.  thus take a copy of it first.
                std::vector<const LIST_ITEM*> children;
                children.reserve( ii->childrenCount() );
                std::copy( ii->childrenBegin(), ii->childrenEnd(), std::back_inserter( children ) );

                for( auto&& c : children )
                    delete_one( c );
            }
        }
        else
            delete_one( ii );
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

    wxString txt;

    // Print Header:
    for( auto&& col : m_data_model->columnDesc() )
        txt += '"' + col.csv_name + "\";";

    f.AddLine( txt );

    // Print list of nets:
    const unsigned int num_rows = m_data_model->itemCount();

    for( unsigned int row = 0; row < num_rows; row++ )
    {
        auto& i = m_data_model->itemAt( row );

        if( i.isGroup() || i.netCode() == 0 )
            continue;

        txt = "";

        for( auto&& col : m_data_model->columnDesc() )
        {
            if( col.csv_flags & COLUMN_DESC::CSV_QUOTE )
                txt += '"' + m_data_model->valueAt( col.num, row ).GetString() + "\";";
            else
                txt += m_data_model->valueAt( col.num, row ).GetString() + ';';
        }

        f.AddLine( txt );
    }

    f.Write();
    f.Close();
}
