/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Oleg Endo <olegendo@gcc.gnu.org>
 * Copyright (C) 2019 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#pragma once

#include <core/optional.h>
#include <dialog_net_inspector_base.h>

class PCB_EDIT_FRAME;
class NETINFO_ITEM;
class BOARD;
class CN_ITEM;
class EDA_PATTERN_MATCH;

class DIALOG_NET_INSPECTOR : public DIALOG_NET_INSPECTOR_BASE, public BOARD_LISTENER
{
public:
    struct SETTINGS
    {
        wxString filter_string;
        bool     show_zero_pad_nets = true;
        bool     group_by           = false;
        int      group_by_kind      = 0;
        wxString group_by_text;
        int      sorting_column     = -1;
        bool     sort_order_asc     = true;

        std::vector<int> column_order;
    };

    DIALOG_NET_INSPECTOR( PCB_EDIT_FRAME* aParent, const SETTINGS& aSettings );
    ~DIALOG_NET_INSPECTOR();

    SETTINGS Settings() const;

    virtual void OnBoardItemAdded( BOARD& aBoard, BOARD_ITEM* aBoardItem ) override;
    virtual void OnBoardItemsAdded( BOARD& aBoard, std::vector<BOARD_ITEM*>& aBoardItems ) override;
    virtual void OnBoardItemRemoved( BOARD& aBoard, BOARD_ITEM* aBoardItem ) override;
    virtual void OnBoardItemsRemoved( BOARD& aBoard, std::vector<BOARD_ITEM*>& aBoardItems ) override;
    virtual void OnBoardNetSettingsChanged( BOARD& aBoard ) override;
    virtual void OnBoardItemChanged( BOARD& aBoard, BOARD_ITEM* aBoardItem ) override;
    virtual void OnBoardItemsChanged( BOARD& aBoard, std::vector<BOARD_ITEM*>& aBoardItems ) override;
    virtual void OnBoardHighlightNetChanged( BOARD& aBoard ) override;

private:
    struct COLUMN_DESC;
    class LIST_ITEM;
    struct LIST_ITEM_NETCODE_CMP_LESS;

    using LIST_ITEM_ITER       = std::vector<std::unique_ptr<LIST_ITEM>>::iterator;
    using LIST_ITEM_CONST_ITER = std::vector<std::unique_ptr<LIST_ITEM>>::const_iterator;

    static const COLUMN_DESC COLUMN_NET;
    static const COLUMN_DESC COLUMN_NAME;
    static const COLUMN_DESC COLUMN_PAD_COUNT;
    static const COLUMN_DESC COLUMN_VIA_COUNT;
    static const COLUMN_DESC COLUMN_VIA_LENGTH;
    static const COLUMN_DESC COLUMN_BOARD_LENGTH;
    static const COLUMN_DESC COLUMN_CHIP_LENGTH;
    static const COLUMN_DESC COLUMN_TOTAL_LENGTH;

    wxString formatNetCode( const NETINFO_ITEM* aNet ) const;
    wxString formatNetName( const NETINFO_ITEM* aNet ) const;
    wxString formatCount( unsigned int aValue ) const;
    wxString formatLength( int64_t aValue ) const;

    std::vector<CN_ITEM*> relevantConnectivityItems() const;
    bool                  netFilterMatches( NETINFO_ITEM* aNet ) const;
    void                  updateNet( NETINFO_ITEM* aNet );
    unsigned int          calculateViaLength( const PCB_TRACK* ) const;

    void onSelChanged( wxDataViewEvent& event ) override;
    void onSelChanged();
    void onSortingChanged( wxDataViewEvent& event ) override;
    void onFilterChange( wxCommandEvent& event ) override;
    void onListSize( wxSizeEvent& event ) override;
    void onAddNet( wxCommandEvent& event ) override;
    void onRenameNet( wxCommandEvent& event ) override;
    void onDeleteNet( wxCommandEvent& event ) override;
    void onReport( wxCommandEvent& event ) override;

    std::unique_ptr<LIST_ITEM> buildNewItem( NETINFO_ITEM* aNet, unsigned int aPadCount,
                                             const std::vector<CN_ITEM*>& aCNItems );

    void buildNetsList();
    void adjustListColumns();

    void onParentWindowClosed( wxCommandEvent& event );
    void onUnitsChanged( wxCommandEvent& event );
    void onBoardChanged( wxCommandEvent& event );

    void updateDisplayedRowValues( const OPT<LIST_ITEM_ITER>& aRow );

    // special zero-netcode item.  unconnected pads etc might use different
    // (dummy) NETINFO_ITEM.  redirect all of them to this item, which we get
    // from the board object in buildNetsList.
    NETINFO_ITEM* m_zero_netitem;

    std::vector<std::unique_ptr<EDA_PATTERN_MATCH>> m_netFilter;
    std::vector<std::unique_ptr<EDA_PATTERN_MATCH>> m_groupFilter;

    BOARD*          m_brd;
    PCB_EDIT_FRAME* m_frame;
    bool            m_in_build_nets_list = false;
    bool            m_filter_change_no_rebuild = false;

    class DATA_MODEL;
    wxObjectDataPtr<DATA_MODEL> m_data_model;
};
