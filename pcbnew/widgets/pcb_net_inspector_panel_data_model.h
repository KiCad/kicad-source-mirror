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

#ifndef PCB_NET_INSPECTOR_PANEL_DATA_MODEL
#define PCB_NET_INSPECTOR_PANEL_DATA_MODEL

#include <eda_pattern_match.h>
#include <string_utils.h>


/**
 * Primary data item for entries in the Net Inspector list.
 *
 * This class tracks all data for a given net entry in the net inspector list.
 */
class PCB_NET_INSPECTOR_PANEL::LIST_ITEM
{
public:
    enum class GROUP_TYPE
    {
        NONE,
        USER_DEFINED,
        NETCLASS
    };

    LIST_ITEM( unsigned int aGroupNumber, const wxString& aGroupName, GROUP_TYPE aGroupType ) :
            m_group_type( aGroupType ),
            m_group_number( aGroupNumber ),
            m_net_name( aGroupName )
    {
        m_group_name = aGroupName;
        m_column_changed.resize( COLUMN_LAST_STATIC_COL + 1 + 2, 0 ); // 2 for default layer count
    }

    LIST_ITEM( NETINFO_ITEM* aNet ) :
            m_group_type( GROUP_TYPE::NONE ),
            m_net( aNet )
    {
        wxASSERT( aNet );
        m_net_name = UnescapeString( aNet->GetNetname() );
        m_net_class = UnescapeString( aNet->GetNetClass()->GetHumanReadableName() );
        m_column_changed.resize( COLUMN_LAST_STATIC_COL + 1 + 2, 0 );
    }

    LIST_ITEM()
    {
        m_column_changed.resize( COLUMN_LAST_STATIC_COL + 1 + 2, 0 );
    }

    LIST_ITEM& operator=( const LIST_ITEM& ) = delete;

    bool            GetIsGroup() const { return m_group_type != GROUP_TYPE::NONE; }
    const wxString& GetGroupName() const { return m_group_name; }
    GROUP_TYPE      GetGroupType() const { return m_group_type; }
    int             GetGroupNumber() const { return m_group_number; }

    auto         ChildrenBegin() const { return m_children.begin(); }
    auto         ChildrenEnd() const { return m_children.end(); }
    unsigned int ChildrenCount() const { return m_children.size(); }

    void SetLayerCount( unsigned int aValue )
    {
        m_column_changed.resize( COLUMN_LAST_STATIC_COL + 1 + aValue, 0 );
    }

    NETINFO_ITEM* GetNet() const { return m_net; }

    int GetNetCode() const
    {
        return GetIsGroup() ? ( 0 - int( m_group_number ) - 1 ) : m_net->GetNetCode();
    }

    const wxString& GetNetName() const { return m_net_name; }
    const wxString& GetNetclassName() const { return m_net_class; }

    void ResetColumnChangedBits()
    {
        std::fill( m_column_changed.begin(), m_column_changed.end(), 0 );
    }

    unsigned int GetPadCount() const { return m_pad_count; }

    bool PadCountChanged() const { return m_column_changed[COLUMN_PAD_COUNT]; }

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

    unsigned GetViaCount() const { return m_via_count; }

    bool ViaCountChanged() const { return m_column_changed[COLUMN_VIA_COUNT]; }

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

    int64_t GetViaLength() const { return m_via_length; }

    bool ViaLengthChanged() const { return m_column_changed[COLUMN_VIA_LENGTH]; }

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

    void SubViaLength( int64_t aValue )
    {
        if( m_parent )
            m_parent->SubViaLength( aValue );

        m_column_changed[COLUMN_VIA_LENGTH] |= ( aValue != 0 );
        m_via_length -= aValue;
    }

    int64_t GetViaDelay() const { return m_via_delay; }

    void SetViaDelay( unsigned int aValue )
    {
        if( m_parent )
            m_parent->SetViaDelay( m_parent->GetViaDelay() - m_via_delay + aValue );

        m_column_changed[COLUMN_VIA_LENGTH] |= ( m_via_delay != aValue );
        m_via_delay = aValue;
    }

    void AddViaDelay( unsigned int aValue )
    {
        if( m_parent )
            m_parent->AddViaDelay( aValue );

        m_column_changed[COLUMN_VIA_LENGTH] |= ( aValue != 0 );
        m_via_delay += aValue;
    }

    void SubViaDelay( int64_t aValue )
    {
        if( m_parent )
            m_parent->SubViaDelay( aValue );

        m_column_changed[COLUMN_VIA_LENGTH] |= ( aValue != 0 );
        m_via_delay -= aValue;
    }

    int64_t GetBoardWireLength() const
    {
        int64_t retval = 0;

        for( auto& [layer, length] : m_layer_wire_length )
            retval += length;

        return retval;
    }

    int64_t GetBoardWireDelay() const
    {
        int64_t retval = 0;

        for( auto& [layer, delay] : m_layer_wire_delay )
            retval += delay;

        return retval;
    }

    int64_t GetLayerWireLength( PCB_LAYER_ID aLayer ) const
    {
        auto it = m_layer_wire_length.find( aLayer );
        return it != m_layer_wire_length.end() ? it->second : 0;
    }

    int64_t GetLayerWireDelay( PCB_LAYER_ID aLayer ) const
    {
        auto it = m_layer_wire_delay.find( aLayer );
        return it != m_layer_wire_delay.end() ? it->second : 0;
    }

    bool BoardWireLengthChanged() const { return m_column_changed[COLUMN_BOARD_LENGTH]; }

    void SetLayerWireLength( const int64_t aValue, PCB_LAYER_ID aLayer )
    {
        auto it = m_layer_wire_length.find( aLayer );

        wxCHECK_RET( it != m_layer_wire_length.end(), wxT( "Invalid layer specified" ) );

        auto& [_, length] = *it;

        if( m_parent )
        {
            m_parent->SetLayerWireLength( m_parent->GetBoardWireLength() - length + aValue,
                                           aLayer );
        }

        m_column_changed[COLUMN_BOARD_LENGTH] |= ( length != aValue );
        length = aValue;
    }

    std::map<PCB_LAYER_ID, int64_t> GetLayerWireLengths() const { return m_layer_wire_length; }

    std::map<PCB_LAYER_ID, int64_t> GetLayerWireDelays() const { return m_layer_wire_delay; }

    void SetLayerWireLengths( const std::map<PCB_LAYER_ID, int64_t>& aValue )
    {
        if( m_parent )
        {
            for( auto& [oldLayer, oldLength] : m_layer_wire_length )
                m_parent->SubLayerWireLength( oldLength, oldLayer );

            for( auto& [newLayer, newLength] : aValue )
                m_parent->AddLayerWireLength( newLength, newLayer );
        }

        m_layer_wire_length = aValue;
    }

    void AddLayerWireLength( const int64_t aValue, PCB_LAYER_ID aLayer )
    {
        if( m_parent )
            m_parent->AddLayerWireLength( aValue, aLayer );

        m_column_changed[COLUMN_BOARD_LENGTH] |= ( m_layer_wire_length[aLayer] != 0 );
        m_layer_wire_length[aLayer] += aValue;
    }

    void SubLayerWireLength( const int64_t aValue, PCB_LAYER_ID aLayer )
    {
        if( m_parent )
            m_parent->SubLayerWireLength( aValue, aLayer );

        m_column_changed[COLUMN_BOARD_LENGTH] |= ( m_layer_wire_length[aLayer] != 0 );
        m_layer_wire_length[aLayer] -= aValue;
    }

    void SetLayerWireDelays( const std::map<PCB_LAYER_ID, int64_t>& aValue )
    {
        if( m_parent )
        {
            for( auto& [oldLayer, oldLength] : m_layer_wire_delay )
                m_parent->SubLayerWireDelay( oldLength, oldLayer );

            for( auto& [newLayer, newLength] : aValue )
                m_parent->AddLayerWireDelay( newLength, newLayer );
        }

        m_layer_wire_delay = aValue;
    }

    void AddLayerWireDelay( const int64_t aValue, PCB_LAYER_ID aLayer )
    {
        if( m_parent )
            m_parent->AddLayerWireDelay( aValue, aLayer );

        m_column_changed[COLUMN_BOARD_LENGTH] |= ( m_layer_wire_delay[aLayer] != 0 );
        m_layer_wire_delay[aLayer] += aValue;
    }

    void SubLayerWireDelay( const int64_t aValue, PCB_LAYER_ID aLayer )
    {
        if( m_parent )
            m_parent->SubLayerWireDelay( aValue, aLayer );

        m_column_changed[COLUMN_BOARD_LENGTH] |= ( m_layer_wire_delay[aLayer] != 0 );
        m_layer_wire_delay[aLayer] -= aValue;
    }

    int64_t GetPadDieLength() const { return m_pad_die_length; }

    bool PadDieLengthChanged() const { return m_column_changed[COLUMN_PAD_DIE_LENGTH]; }

    void SetPadDieLength( int64_t aValue )
    {
        if( m_parent )
            m_parent->SetPadDieLength( m_parent->GetPadDieLength() - m_pad_die_length + aValue );

        m_column_changed[COLUMN_PAD_DIE_LENGTH] |= ( m_pad_die_length != aValue );
        m_pad_die_length = aValue;
    }

    void AddPadDieLength( int64_t aValue )
    {
        if( m_parent )
            m_parent->AddPadDieLength( aValue );

        m_column_changed[COLUMN_PAD_DIE_LENGTH] |= ( aValue != 0 );
        m_pad_die_length += aValue;
    }

    void SubPadDieLength( int64_t aValue )
    {
        if( m_parent )
            m_parent->SubPadDieLength( aValue );

        m_column_changed[COLUMN_PAD_DIE_LENGTH] |= ( aValue != 0 );
        m_pad_die_length -= aValue;
    }

    int64_t GetPadDieDelay() const { return m_pad_die_delay; }

    void SetPadDieDelay( int64_t aValue )
    {
        if( m_parent )
            m_parent->SetPadDieDelay( m_parent->GetPadDieDelay() - m_pad_die_delay + aValue );

        m_column_changed[COLUMN_PAD_DIE_LENGTH] |= ( m_pad_die_delay != aValue );
        m_pad_die_delay = aValue;
    }

    void AddPadDieDelay( int64_t aValue )
    {
        if( m_parent )
            m_parent->AddPadDieDelay( aValue );

        m_column_changed[COLUMN_PAD_DIE_LENGTH] |= ( aValue != 0 );
        m_pad_die_delay += aValue;
    }

    void SubPadDieDelay( int64_t aValue )
    {
        if( m_parent )
            m_parent->SubPadDieDelay( aValue );

        m_column_changed[COLUMN_PAD_DIE_LENGTH] |= ( aValue != 0 );
        m_pad_die_delay -= aValue;
    }

    // the total length column is always computed, never stored.
    unsigned long long int GetTotalLength() const
    {
        return GetBoardWireLength() + GetViaLength() + GetPadDieLength();
    }

    unsigned long long int GetTotalDelay() const { return GetBoardWireDelay() + GetViaDelay() + GetPadDieDelay(); }

    bool TotalLengthChanged() const
    {
        return BoardWireLengthChanged() || ViaLengthChanged() || PadDieLengthChanged();
    }

    LIST_ITEM* Parent() const { return m_parent; }

    void SetParent( LIST_ITEM* aParent )
    {
        if( m_parent == aParent )
            return;

        if( m_parent != nullptr )
        {
            m_parent->SubPadCount( GetPadCount() );
            m_parent->SubViaCount( GetViaCount() );
            m_parent->SubViaLength( GetViaLength() );
            m_parent->SubViaDelay( GetViaDelay() );

            for( auto& [layer, length] : m_layer_wire_length )
                m_parent->SubLayerWireLength( length, layer );

            for( auto& [layer, delay] : m_layer_wire_delay )
                m_parent->SubLayerWireDelay( delay, layer );

            m_parent->SubPadDieLength( GetPadDieLength() );
            m_parent->SubPadDieDelay( GetPadDieDelay() );

            m_parent->m_children.erase( std::find( m_parent->m_children.begin(),
                                                   m_parent->m_children.end(), this ) );
        }

        m_parent = aParent;

        if( m_parent != nullptr )
        {
            m_parent->AddPadCount( GetPadCount() );
            m_parent->AddViaCount( GetViaCount() );
            m_parent->AddViaLength( GetViaLength() );
            m_parent->AddViaDelay( GetViaDelay() );

            for( auto& [layer, length] : m_layer_wire_length )
                m_parent->AddLayerWireLength( length, layer );

            for( auto& [layer, delay] : m_layer_wire_delay )
                m_parent->AddLayerWireDelay( delay, layer );

            m_parent->AddPadDieLength( GetPadDieLength() );
            m_parent->AddPadDieDelay( GetPadDieDelay() );

            m_parent->m_children.push_back( this );
        }
    }

private:
    LIST_ITEM*              m_parent = nullptr;
    std::vector<LIST_ITEM*> m_children;

    GROUP_TYPE    m_group_type = GROUP_TYPE::NONE;
    unsigned int  m_group_number = 0;
    NETINFO_ITEM* m_net = nullptr;
    unsigned int  m_pad_count = 0;
    unsigned int  m_via_count = 0;
    int64_t       m_via_length = 0;
    int64_t       m_via_delay = 0;
    int64_t       m_pad_die_length = 0;
    int64_t       m_pad_die_delay = 0;

    std::map<PCB_LAYER_ID, int64_t> m_layer_wire_length{};
    std::map<PCB_LAYER_ID, int64_t> m_layer_wire_delay{};

    // Dirty bits to record when some attribute has changed, in order to avoid unnecessary sort
    // operations.
    // The values are semantically bools, but STL auto-promotes a std::vector<bool> to a bitset,
    // and then operator|= doesn't work.
    std::vector<int> m_column_changed;

    // cached formatted names for faster display sorting
    wxString m_net_name;
    wxString m_net_class;
    wxString m_group_name;
};


struct PCB_NET_INSPECTOR_PANEL::LIST_ITEM_NETCODE_CMP_LESS
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


struct PCB_NET_INSPECTOR_PANEL::LIST_ITEM_GROUP_NUMBER_CMP_LESS
{
    template <typename T>
    bool operator()( const T& a, const T& b ) const
    {
        return a->GetGroupNumber() < b->GetGroupNumber();
    }

    template <typename T>
    bool operator()( const T& a, int b ) const
    {
        return a->GetGroupNumber() < b;
    }

    template <typename T>
    bool operator()( int a, const T& b ) const
    {
        return a < b->GetGroupNumber();
    }
};


/**
 * Data model for display in the Net Inspector panel.
*/
class PCB_NET_INSPECTOR_PANEL::DATA_MODEL : public wxDataViewModel
{
public:
    DATA_MODEL( PCB_NET_INSPECTOR_PANEL& parent ) :
            m_parent( parent )
    {}

    unsigned int columnCount() const { return m_parent.m_columns.size(); }

    unsigned int itemCount() const { return m_items.size(); }

    wxVariant valueAt( unsigned int aCol, unsigned int aRow ) const
    {
        wxVariant r;
        GetValue( r, wxDataViewItem( const_cast<LIST_ITEM*>( &*( m_items[aRow] ) ) ), aCol );
        return r;
    }

    const LIST_ITEM& itemAt( unsigned int aRow ) const { return *m_items.at( aRow ); }

    std::vector<std::pair<wxString, wxDataViewItem>> getGroupDataViewItems()
    {
        std::vector<std::pair<wxString, wxDataViewItem>> ret;

        for( std::unique_ptr<LIST_ITEM>& item : m_items )
        {
            if( item->GetIsGroup() )
                ret.push_back( std::make_pair( item->GetGroupName(), wxDataViewItem( item.get() ) ) );
        }

        return ret;
    }

    std::optional<LIST_ITEM_ITER> findItem( int aNetCode )
    {
        auto i = std::lower_bound( m_items.begin(), m_items.end(), aNetCode,
                                   LIST_ITEM_NETCODE_CMP_LESS() );

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

    std::optional<LIST_ITEM_ITER> findGroupItem( int aGroupNumber )
    {
        auto i = std::lower_bound( m_items.begin(), m_items.end(), aGroupNumber,
                                   LIST_ITEM_GROUP_NUMBER_CMP_LESS() );

        if( i == m_items.end() || ( *i )->GetGroupNumber() != aGroupNumber )
            return std::nullopt;

        return { i };
    }

    LIST_ITEM* addGroup( LIST_ITEM_ITER groupsBegin, LIST_ITEM_ITER groupsEnd,
                         wxString groupName, LIST_ITEM::GROUP_TYPE groupType )
    {
        LIST_ITEM_ITER group = std::find_if( groupsBegin, groupsEnd,
                                             [&]( const std::unique_ptr<LIST_ITEM>& x )
                                             {
                                                 return x->GetGroupName() == groupName
                                                        && x->GetGroupType() == groupType;
                                             } );

        if( group == groupsEnd )
        {
            int                        dist = std::distance( groupsBegin, groupsEnd );
            std::unique_ptr<LIST_ITEM> groupItem = std::make_unique<LIST_ITEM>( dist, groupName, groupType );

            group = m_items.insert( groupsEnd, std::move( groupItem ) );
            ItemAdded( wxDataViewItem( ( *group )->Parent() ), wxDataViewItem( ( *group ).get() ) );
        }

        return ( *group ).get();
    }

    std::optional<LIST_ITEM_ITER> addItem( std::unique_ptr<LIST_ITEM> aItem )
    {
        if( aItem == nullptr )
            return {};

        bool groupMatched = false;

        // First see if item matches a group-by rule
        if( m_parent.m_custom_group_rules.size() > 0 )
        {
            wxString searchName = aItem->GetNetName();

            for( const std::unique_ptr<EDA_COMBINED_MATCHER>& rule : m_parent.m_custom_group_rules )
            {
                if( rule->Find( searchName ) )
                {
                    aItem->SetParent( m_custom_group_map[ rule->GetPattern() ] );
                    groupMatched = true;
                    break;
                }
            }
        }

        // Then add any netclass groups required by this item
        if( m_parent.m_groupByNetclass && !groupMatched )
        {
            LIST_ITEM_ITER groups_begin = m_items.begin();
            LIST_ITEM_ITER groups_end = std::find_if_not( m_items.begin(), m_items.end(),
                                                          []( const std::unique_ptr<LIST_ITEM>& x )
                                                          {
                                                              return x->GetIsGroup();
                                                          } );

            wxString match_str = aItem->GetNetclassName();
            LIST_ITEM* group = addGroup( groups_begin, groups_end, match_str, LIST_ITEM::GROUP_TYPE::NETCLASS );
            aItem->SetParent( group );
        }

        // Now add the item itself. Usually when new nets are added,
        // they always get a higher netcode number than the already existing ones.
        // however, if we've got filtering enabled, we might not have all the nets in
        // our list, so do a sorted insertion.
        auto new_iter = std::lower_bound( m_items.begin(), m_items.end(), aItem->GetNetCode(),
                                          LIST_ITEM_NETCODE_CMP_LESS() );

        new_iter = m_items.insert( new_iter, std::move( aItem ) );
        const std::unique_ptr<LIST_ITEM>& new_item = *new_iter;

        ItemAdded( wxDataViewItem( new_item->Parent() ), wxDataViewItem( new_item.get() ) );

        return { new_iter };
    }

    void addItems( std::vector<std::unique_ptr<LIST_ITEM>>& aItems )
    {
        m_items.reserve( m_items.size() + aItems.size() );

        for( std::unique_ptr<LIST_ITEM>& i : aItems )
            addItem( std::move( i ) );
    }

    std::unique_ptr<LIST_ITEM> deleteItem( const std::optional<LIST_ITEM_ITER>& aRow )
    {
        if( !aRow )
            return {};

        std::unique_ptr<LIST_ITEM> i = std::move( **aRow );

        LIST_ITEM* parent = i->Parent();
        i->SetParent( nullptr );

        m_items.erase( *aRow );
        ItemDeleted( wxDataViewItem( parent ), wxDataViewItem( &*i ) );

        if( parent )
        {
            ItemChanged( wxDataViewItem( parent ) );

            if( m_parent.m_groupByNetclass && parent != nullptr && parent->ChildrenCount() == 0 )
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

    /**
     * Adds all custom group-by entries to the items table
     *
     * Note this assumes that m_items is empty prior to adding these groups
     */
    void addCustomGroups()
    {
        m_custom_group_map.clear();
        int groupId = 0;

        for( const std::unique_ptr<EDA_COMBINED_MATCHER>& rule : m_parent.m_custom_group_rules )
        {
            std::unique_ptr<LIST_ITEM>& group = m_items.emplace_back( std::make_unique<LIST_ITEM>(
                    groupId, rule->GetPattern(), LIST_ITEM::GROUP_TYPE::USER_DEFINED ) );
            m_custom_group_map[ rule->GetPattern() ] = group.get();
            group->SetLayerCount( m_parent.m_board->GetCopperLayerCount() );
            ItemAdded( wxDataViewItem( group->Parent() ), wxDataViewItem( group.get() ) );
            ++groupId;
        }
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

        else if( aCol == COLUMN_PAD_DIE_LENGTH )
            return aItem->PadDieLengthChanged();

        else if( aCol == COLUMN_TOTAL_LENGTH )
            return aItem->TotalLengthChanged();

        else if( aCol > COLUMN_LAST_STATIC_COL )
            return aItem->BoardWireLengthChanged();


        return false;
    }

    void SetIsTimeDomain( const bool aIsTimeDomain ) { m_show_time_domain_details = aIsTimeDomain; }

    // implementation of wxDataViewModel interface
    // these are used to query the data model by the GUI view implementation.
    // these are not supposed to be used to modify the data model.  for that
    // use the public functions above.

protected:
    unsigned int GetColumnCount() const override { return columnCount(); }

    void GetValue( wxVariant& aOutValue, const wxDataViewItem& aItem,
                   unsigned int aCol ) const override
    {
        if( LIST_ITEM* i = static_cast<LIST_ITEM*>( aItem.GetID() ) )
        {
            if( aCol == COLUMN_NAME )
            {
                if( i->GetIsGroup() )
                {
                    switch( i->GetGroupType() )
                    {
                    case LIST_ITEM::GROUP_TYPE::NETCLASS:
                        aOutValue = _( "Netclass" ) + ": " + i->GetGroupName();
                        break;
                    case LIST_ITEM::GROUP_TYPE::USER_DEFINED:
                        aOutValue = _( "Custom" ) + ": " + i->GetGroupName();
                        break;
                    default:
                        aOutValue = i->GetGroupName();
                        break;
                    }
                }
                else
                {
                    aOutValue = i->GetNetName();
                }
            }
            else if( aCol == COLUMN_NETCLASS )
            {
                aOutValue = i->GetNetclassName();
            }
            else if( aCol == COLUMN_PAD_COUNT )
            {
                aOutValue = m_parent.formatCount( i->GetPadCount() );
            }
            else if( aCol == COLUMN_VIA_COUNT )
            {
                aOutValue = m_parent.formatCount( i->GetViaCount() );
            }
            else if( aCol == COLUMN_VIA_LENGTH )
            {
                if( m_show_time_domain_details )
                    aOutValue = m_parent.formatDelay( i->GetViaDelay() );
                else
                    aOutValue = m_parent.formatLength( i->GetViaLength() );
            }
            else if( aCol == COLUMN_BOARD_LENGTH )
            {
                if( m_show_time_domain_details )
                    aOutValue = m_parent.formatDelay( i->GetBoardWireDelay() );
                else
                    aOutValue = m_parent.formatLength( i->GetBoardWireLength() );
            }
            else if( aCol == COLUMN_PAD_DIE_LENGTH )
            {
                if( m_show_time_domain_details )
                    aOutValue = m_parent.formatDelay( i->GetPadDieDelay() );
                else
                    aOutValue = m_parent.formatLength( i->GetPadDieLength() );
            }
            else if( aCol == COLUMN_TOTAL_LENGTH )
            {
                if( m_show_time_domain_details )
                    aOutValue = m_parent.formatDelay( i->GetTotalDelay() );
                else
                    aOutValue = m_parent.formatLength( i->GetTotalLength() );
            }
            else if( aCol > COLUMN_LAST_STATIC_COL && aCol <= m_parent.m_columns.size() )
            {
                if( m_show_time_domain_details )
                    aOutValue = m_parent.formatDelay( i->GetLayerWireDelay( m_parent.m_columns[aCol].layer ) );
                else
                    aOutValue = m_parent.formatLength( i->GetLayerWireLength( m_parent.m_columns[aCol].layer ) );
            }
            else
            {
                aOutValue = "";
            }
        }
    }

    static int compareUInt( int64_t aValue1, int64_t aValue2, bool aAsc )
    {
        if( aAsc )
            return aValue1 < aValue2 ? -1 : 1;
        else
            return aValue2 < aValue1 ? -1 : 1;
    }

    int Compare( const wxDataViewItem& aItem1, const wxDataViewItem& aItem2, unsigned int aCol,
                 bool aAsc ) const override
    {
        const LIST_ITEM& i1 = *static_cast<const LIST_ITEM*>( aItem1.GetID() );
        const LIST_ITEM& i2 = *static_cast<const LIST_ITEM*>( aItem2.GetID() );

        if( i1.GetIsGroup() && !i2.GetIsGroup() )
            return -1;

        if( i2.GetIsGroup() && !i1.GetIsGroup() )
            return 1;

        if( aCol == COLUMN_NAME )
        {
            const wxString& s1 = i1.GetNetName();
            const wxString& s2 = i2.GetNetName();

            int res = aAsc ? ValueStringCompare( s1, s2 ) : ValueStringCompare( s2, s1 );

            if( res != 0 )
                return res;
        }
        else if( aCol == COLUMN_NETCLASS )
        {
            const wxString& s1 = i1.GetNetclassName();
            const wxString& s2 = i2.GetNetclassName();

            int res = aAsc ? ValueStringCompare( s1, s2 ) : ValueStringCompare( s2, s1 );

            if( res != 0 )
                return res;
        }
        else if( aCol == COLUMN_PAD_COUNT && i1.GetPadCount() != i2.GetPadCount() )
        {
            return compareUInt( i1.GetPadCount(), i2.GetPadCount(), aAsc );
        }
        else if( aCol == COLUMN_VIA_COUNT && i1.GetViaCount() != i2.GetViaCount() )
        {
            return compareUInt( i1.GetViaCount(), i2.GetViaCount(), aAsc );
        }
        else if( aCol == COLUMN_VIA_LENGTH )
        {
            if( m_show_time_domain_details && i1.GetViaDelay() != i2.GetViaDelay() )
                return compareUInt( i1.GetViaDelay(), i2.GetViaDelay(), aAsc );

            if( !m_show_time_domain_details && i1.GetViaLength() != i2.GetViaLength() )
                return compareUInt( i1.GetViaLength(), i2.GetViaLength(), aAsc );
        }
        else if( aCol == COLUMN_BOARD_LENGTH )
        {
            if( m_show_time_domain_details && i1.GetBoardWireDelay() != i2.GetBoardWireDelay() )
                return compareUInt( i1.GetBoardWireDelay(), i2.GetBoardWireDelay(), aAsc );

            if( !m_show_time_domain_details && i1.GetBoardWireLength() != i2.GetBoardWireLength() )
                return compareUInt( i1.GetBoardWireLength(), i2.GetBoardWireLength(), aAsc );
        }
        else if( aCol == COLUMN_PAD_DIE_LENGTH )
        {
            if( m_show_time_domain_details && i1.GetPadDieDelay() != i2.GetPadDieDelay() )
                return compareUInt( i1.GetPadDieDelay(), i2.GetPadDieDelay(), aAsc );

            if( !m_show_time_domain_details && i1.GetPadDieLength() != i2.GetPadDieLength() )
                return compareUInt( i1.GetPadDieLength(), i2.GetPadDieLength(), aAsc );
        }
        else if( aCol == COLUMN_TOTAL_LENGTH )
        {
            if( m_show_time_domain_details && i1.GetTotalDelay() != i2.GetTotalDelay() )
                return compareUInt( i1.GetTotalDelay(), i2.GetTotalDelay(), aAsc );

            if( !m_show_time_domain_details && i1.GetTotalLength() != i2.GetTotalLength() )
                return compareUInt( i1.GetTotalLength(), i2.GetTotalLength(), aAsc );
        }
        else if( aCol > COLUMN_LAST_STATIC_COL && aCol < m_parent.m_columns.size() )
        {
            if( m_show_time_domain_details
                && i1.GetLayerWireDelay( m_parent.m_columns[aCol].layer )
                           != i2.GetLayerWireDelay( m_parent.m_columns[aCol].layer ) )
            {
                return compareUInt( i1.GetLayerWireDelay( m_parent.m_columns[aCol].layer ),
                                    i2.GetLayerWireDelay( m_parent.m_columns[aCol].layer ), aAsc );
            }

            if( !m_show_time_domain_details
                && i1.GetLayerWireLength( m_parent.m_columns[aCol].layer )
                           != i2.GetLayerWireLength( m_parent.m_columns[aCol].layer ) )
            {
                return compareUInt( i1.GetLayerWireLength( m_parent.m_columns[aCol].layer ),
                                    i2.GetLayerWireLength( m_parent.m_columns[aCol].layer ), aAsc );
            }
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
                              wxDataViewItemArray&  aChildren ) const override
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

    wxString GetColumnType( unsigned int /* aCol */ ) const override { return wxS( "string" ); }

private:
    PCB_NET_INSPECTOR_PANEL& m_parent;

    // primary container, sorted by netcode number.
    // groups have netcode < 0, so they always come first, in the order
    // of the filter strings as input by the user
    std::vector<std::unique_ptr<LIST_ITEM>> m_items;

    /// Map of custom group names to their representative list item
    std::map<wxString, LIST_ITEM*> m_custom_group_map;

    bool m_show_time_domain_details{ false };
};

#endif
