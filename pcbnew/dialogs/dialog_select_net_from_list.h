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

#include <dialog_select_net_from_list_base.h>
#include <eda_pattern_match.h>

class PCB_EDIT_FRAME;
class NETINFO_ITEM;
class BOARD;
class CN_ITEM;

class DIALOG_SELECT_NET_FROM_LIST : public DIALOG_SELECT_NET_FROM_LIST_BASE, public BOARD_LISTENER
{
public:
    struct SETTINGS
    {
        wxString filter_string;
        bool     show_zero_pad_nets = true;
    };

    DIALOG_SELECT_NET_FROM_LIST( PCB_EDIT_FRAME* aParent, const SETTINGS& aSettings );
    ~DIALOG_SELECT_NET_FROM_LIST();

    SETTINGS Settings() const;

    // returns true if a net was selected, and its name in aName
    bool GetNetName( wxString& aName ) const;

    /**
     * Visually highlights a net in the list view.
     * @param aNet is the net item to be highlighted.  Nullptr will unhighlight
     * any currently highlighted net.
     */
    void HighlightNet( NETINFO_ITEM* aNet );

    virtual void OnBoardItemAdded( BOARD& aBoard, BOARD_ITEM* aBoardItem ) override;
    virtual void OnBoardItemRemoved( BOARD& aBoard, BOARD_ITEM* aBoardItem ) override;
    virtual void OnBoardNetSettingsChanged( BOARD& aBoard ) override;
    virtual void OnBoardItemChanged( BOARD& aBoard, BOARD_ITEM* aBoardItem ) override;
    virtual void OnBoardHighlightNetChanged( BOARD& aBoard ) override;

private:
    struct COLUMN_ID;
    static const COLUMN_ID COLUMN_NET;
    static const COLUMN_ID COLUMN_NAME;
    static const COLUMN_ID COLUMN_PAD_COUNT;
    static const COLUMN_ID COLUMN_VIA_COUNT;
    static const COLUMN_ID COLUMN_BOARD_LENGTH;
    static const COLUMN_ID COLUMN_CHIP_LENGTH;
    static const COLUMN_ID COLUMN_TOTAL_LENGTH;

    struct ROW_DESC;

    ROW_DESC findRow( NETINFO_ITEM* aNet );
    ROW_DESC findRow( int aNetCode );

    void deleteRow( const ROW_DESC& aRow );
    void setValue( const ROW_DESC& aRow, const COLUMN_ID& aCol, wxString aVal );

    wxString formatNetCode( const NETINFO_ITEM* aNet ) const;
    wxString formatNetName( const NETINFO_ITEM* aNet ) const;
    wxString formatCount( unsigned int aValue ) const;
    wxString formatLength( int aValue ) const;

    std::vector<CN_ITEM*> relevantConnectivityItems() const;
    bool                  netFilterMatches( NETINFO_ITEM* aNet ) const;
    void                  updateNet( NETINFO_ITEM* aNet );
    void                  highlightNetOnBoard( NETINFO_ITEM* aNet ) const;

    void onSelChanged( wxDataViewEvent& event ) override;
    void onFilterChange( wxCommandEvent& event ) override;
    void onListSize( wxSizeEvent& event ) override;
    void onAddNet( wxCommandEvent& event ) override;
    void onRenameNet( wxCommandEvent& event ) override;
    void onDeleteNet( wxCommandEvent& event ) override;
    void onReport( wxCommandEvent& event ) override;

    void buildNetsList();
    void adjustListColumns();

    void onParentWindowClosed( wxCommandEvent& event );
    void onUnitsChanged( wxCommandEvent& event );
    void onBoardChanged( wxCommandEvent& event );

    // in addition to the displayed list data, we also keep some auxiliary
    // data for each list item in order to speed up update of the displayed list.
    struct LIST_ITEM;
    struct LIST_ITEM_NET_CMP_LESS;

    // primary vector, sorted by rows
    std::vector<LIST_ITEM> m_list_items;

    // we can't keep pointers to the elements in the primary vector because
    // the underlyng storage might change when elements are added or removed.
    // keep indices instead and look the them up in m_list_items.
    std::vector<unsigned int> m_list_items_by_net;


    EDA_PATTERN_MATCH_WILDCARD m_netFilter;

    wxString        m_selection;
    bool            m_wasSelected;
    BOARD*          m_brd;
    PCB_EDIT_FRAME* m_frame;
};
