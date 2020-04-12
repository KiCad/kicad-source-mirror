/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

class PCB_EDIT_FRAME;
class NETINFO_ITEM;
class BOARD;

class DIALOG_SELECT_NET_FROM_LIST : public DIALOG_SELECT_NET_FROM_LIST_BASE
{
private:
public:
    DIALOG_SELECT_NET_FROM_LIST( PCB_EDIT_FRAME* aParent );
    ~DIALOG_SELECT_NET_FROM_LIST();

    // returns true if a net was selected, and its name in aName
    bool GetNetName( wxString& aName );

    /**
     * Visually highlights a net.
     * @param aNetName is the name of net to be highlighted.  An empty string will unhighlight
     * any currently highlighted net.
     */
    void HighlightNet( const wxString& aNetName );

private:
    void onSelChanged( wxDataViewEvent& event ) override;
    void onFilterChange( wxCommandEvent& event ) override;
    void onListSize( wxSizeEvent& event ) override;
    void onReport( wxCommandEvent& event ) override;

    void buildNetsList();
    wxString getListColumnHeaderNet() { return _( "Net" ); };
    wxString getListColumnHeaderName() { return _( "Name" ); };
    wxString getListColumnHeaderCount() { return _( "Pad Count" ); };
    wxString getListColumnHeaderVias() { return _( "Via Count" ); };
    wxString getListColumnHeaderBoard() { return _( "Board Length" ); };
    wxString getListColumnHeaderDie() { return _( "Die Length" ); };
    wxString getListColumnHeaderLength() { return _( "Length" ); };
    void adjustListColumns();

    wxArrayString   m_netsInitialNames;   // The list of escaped netnames (original names)
    wxString        m_selection;
    bool            m_wasSelected;
    BOARD*          m_brd;
    PCB_EDIT_FRAME* m_frame;
};
