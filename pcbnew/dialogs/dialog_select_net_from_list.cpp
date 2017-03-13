/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2017 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file dialog_select_net_from_list.cpp
 * @brief methods to show available net names and select and highligth a net
 */

#include <wx/grid.h>

#include <fctsys.h>
#include <kicad_string.h>
#include <kicad_device_context.h>
#include <class_drawpanel.h>
#include <pcbnew.h>
#include <wxPcbStruct.h>
#include <class_board.h>
#include <dialog_select_net_from_list_base.h>
#include <eda_pattern_match.h>

#include <view/view.h>
#include <view/view_controls.h>
#include <pcb_painter.h>

#define COL_NETNAME 0
#define COL_NETINFO 1

class DIALOG_SELECT_NET_FROM_LIST: public DIALOG_SELECT_NET_FROM_LIST_BASE
{
private:
    wxString m_selection;
    bool m_wasSelected;
    BOARD* m_brd;

public:
    DIALOG_SELECT_NET_FROM_LIST( PCB_EDIT_FRAME * aParent );
    ~DIALOG_SELECT_NET_FROM_LIST();

    // returns true if a net was selected, and its name in aName
    bool GetNetName( wxString& aName );

private:
    void onCellClick( wxGridEvent& event ) override;
	void onFilterChange( wxCommandEvent& event ) override;

    void buildNetsList();
};


void PCB_EDIT_FRAME::ListNetsAndSelect( wxCommandEvent& event )
{
    DIALOG_SELECT_NET_FROM_LIST dlg( this );
    wxString netname;

    if( dlg.ShowModal() == wxID_CANCEL || !dlg.GetNetName( netname ) )
        return;

    // Search for the net selected.
    NETINFO_ITEM* net = GetBoard()->FindNet( netname );

    if( net == NULL )   // Should not occur.
        return;

    if( IsGalCanvasActive() )
    {
        KIGFX::RENDER_SETTINGS* render = GetGalCanvas()->GetView()->GetPainter()->GetSettings();
        render->SetHighlight( true, net->GetNet() );

        GetGalCanvas()->GetView()->UpdateAllLayersColor();
        GetGalCanvas()->Refresh();
    }
    else
    {
        INSTALL_UNBUFFERED_DC( dc, m_canvas );

        if( GetBoard()->IsHighLightNetON() )
            HighLight( &dc );

        GetBoard()->SetHighLightNet( net->GetNet() );
        HighLight( &dc );
    }
}


DIALOG_SELECT_NET_FROM_LIST::DIALOG_SELECT_NET_FROM_LIST( PCB_EDIT_FRAME* aParent )
    : DIALOG_SELECT_NET_FROM_LIST_BASE( aParent )
{
    m_brd = aParent->GetBoard();
    m_wasSelected = false;

    // Choose selection mode
    m_netsListGrid->SetSelectionMode( wxGrid::wxGridSelectRows );

    buildNetsList();

    m_sdbSizerOK->SetDefault();
    GetSizer()->SetSizeHints( this );
    Center();
}

void DIALOG_SELECT_NET_FROM_LIST::buildNetsList()
{
    wxString netFilter = m_textCtrlFilter->GetValue();
    EDA_PATTERN_MATCH_WILDCARD filter;
    filter.SetPattern( netFilter.MakeUpper() );
    wxString txt;

    int row_idx = 0;

    // Populate the nets list with nets names matching the filters:
    // Note: the filtering is case insensitive.
    for( unsigned netcode = 0; netcode < m_brd->GetNetCount(); netcode++ )
    {
        NETINFO_ITEM* net = m_brd->GetNetInfo().GetNetItem( netcode );

        if( !netFilter.IsEmpty() )
        {
            wxString netname = net->GetNetname();
            if( filter.Find( netname.MakeUpper() ) == EDA_PATTERN_NOT_FOUND )
                continue;
        }

        if( !m_cbShowZeroPad->IsChecked() && net->m_PadInNetList.size() == 0 )
            continue;

        if( m_netsListGrid->GetNumberRows() <= row_idx )
            m_netsListGrid->AppendRows( 1 );

        txt.Printf( _( "net %.3d" ), net->GetNet() );
        m_netsListGrid->SetRowLabelValue( row_idx, txt );

        m_netsListGrid->SetCellValue( row_idx, COL_NETNAME, net->GetNetname() );

        if( netcode )
        {
            txt.Printf( wxT( "%u" ), (unsigned) net->m_PadInNetList.size() );
            m_netsListGrid->SetCellValue( row_idx, COL_NETINFO, txt );
        }
        else    // For the net 0 (unconnected pads), the pad count is not known
            m_netsListGrid->SetCellValue( row_idx, COL_NETINFO, "---" );

        row_idx++;
    }

    // Remove extra rows, if any:
    int extra_row_idx = m_netsListGrid->GetNumberRows() - row_idx;

    if( extra_row_idx > 0 )
        m_netsListGrid->DeleteRows( row_idx, extra_row_idx );

    m_netsListGrid->SetColLabelSize( wxGRID_AUTOSIZE );
    m_netsListGrid->SetRowLabelSize( wxGRID_AUTOSIZE );

    m_netsListGrid->ClearSelection();
    m_wasSelected = false;
}


DIALOG_SELECT_NET_FROM_LIST::~DIALOG_SELECT_NET_FROM_LIST()
{
}

void DIALOG_SELECT_NET_FROM_LIST::onFilterChange( wxCommandEvent& event )
{
    buildNetsList();
}


void DIALOG_SELECT_NET_FROM_LIST::onCellClick( wxGridEvent& event )
{
    int selected_row = event.GetRow();
    m_selection = m_netsListGrid->GetCellValue( selected_row, COL_NETNAME );
    m_wasSelected = true;

    // Select the full row when clicking on any cell off the row
    m_netsListGrid->SelectRow( selected_row, false );
    m_netsListGrid->SetGridCursor(selected_row, COL_NETNAME );
}


bool DIALOG_SELECT_NET_FROM_LIST::GetNetName( wxString& aName )
{
    aName = m_selection;
    return m_wasSelected;
}
