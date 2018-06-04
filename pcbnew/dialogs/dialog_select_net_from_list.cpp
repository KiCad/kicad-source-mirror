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
#include <pcb_edit_frame.h>
#include <class_board.h>
#include <dialog_select_net_from_list_base.h>
#include <eda_pattern_match.h>

#include <view/view.h>
#include <view/view_controls.h>
#include <pcb_painter.h>
#include <connectivity_data.h>

#define COL_NETNAME 0
#define COL_NETINFO 1

class DIALOG_SELECT_NET_FROM_LIST: public DIALOG_SELECT_NET_FROM_LIST_BASE
{
private:
public:
    DIALOG_SELECT_NET_FROM_LIST( PCB_EDIT_FRAME* aParent );
    ~DIALOG_SELECT_NET_FROM_LIST();

    // returns true if a net was selected, and its name in aName
    bool GetNetName( wxString& aName );

    /**
     * Visually highlights a net.
     * @param aEnabled true to enable highlight mode, false to disable.
     * @param aNetName is the name of net to be highlighted.
     */
    void HighlightNet( bool aEnabled, const wxString& aNetName );

private:
    void onCellClick( wxGridEvent& event ) override;
    void onFilterChange( wxCommandEvent& event ) override;
    void onColumnResize( wxGridSizeEvent& event ) override;
    void onSelectCell( wxGridEvent& event ) override;
    void updateSize( wxSizeEvent& event ) override;

    void setColumnSize();
    void buildNetsList();

    wxString m_selection;
    int m_firstWidth;
    bool m_wasSelected;
    BOARD* m_brd;
    PCB_EDIT_FRAME* m_frame;
};


void PCB_EDIT_FRAME::ListNetsAndSelect( wxCommandEvent& event )
{
    DIALOG_SELECT_NET_FROM_LIST dlg( this );
    wxString netname;

    if( dlg.ShowModal() == wxID_CANCEL )
    {
        // Clear highlight
        dlg.HighlightNet( false, "" );
    }
}


DIALOG_SELECT_NET_FROM_LIST::DIALOG_SELECT_NET_FROM_LIST( PCB_EDIT_FRAME* aParent )
    : DIALOG_SELECT_NET_FROM_LIST_BASE( aParent ), m_frame( aParent )
{
    m_brd = aParent->GetBoard();
    m_wasSelected = false;

    // Choose selection mode
    m_netsListGrid->SetSelectionMode( wxGrid::wxGridSelectRows );
    buildNetsList();

    m_sdbSizerOK->SetDefault();
    GetSizer()->SetSizeHints( this );
    Center();

    m_firstWidth = m_netsListGrid->GetColSize( 0 );
    setColumnSize();
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

        unsigned nodes = m_brd->GetNodesCount( netcode );

        if( !m_cbShowZeroPad->IsChecked() && nodes == 0 )
            continue;

        if( m_netsListGrid->GetNumberRows() <= row_idx )
            m_netsListGrid->AppendRows( 1 );

        txt.Printf( _( "net %.3d" ), net->GetNet() );
        m_netsListGrid->SetRowLabelValue( row_idx, txt );

        m_netsListGrid->SetCellValue( row_idx, COL_NETNAME, net->GetNetname() );

        if( netcode )
        {
            txt.Printf( wxT( "%u" ), nodes );
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


void DIALOG_SELECT_NET_FROM_LIST::HighlightNet( bool aEnabled, const wxString& aNetName )
{
    // Search for the net selected.
    NETINFO_ITEM* net = aEnabled ? m_brd->FindNet( aNetName ) : nullptr;
    int netCode = net ? net->GetNet() : -1;

    if( m_frame->IsGalCanvasActive() )
    {
        auto galCanvas = m_frame->GetGalCanvas();
        KIGFX::RENDER_SETTINGS* render = galCanvas->GetView()->GetPainter()->GetSettings();
        render->SetHighlight( aEnabled, netCode );

        galCanvas->GetView()->UpdateAllLayersColor();
        galCanvas->Refresh();
    }
    else
    {
        INSTALL_UNBUFFERED_DC( dc, m_frame->GetCanvas() );

        if( m_brd->IsHighLightNetON() )
            m_frame->HighLight( &dc );

        m_brd->SetHighLightNet( netCode );
        m_frame->HighLight( &dc );
    }
}

void DIALOG_SELECT_NET_FROM_LIST::setColumnSize()
{
    auto size = m_netsListGrid->GetGridWindow()->GetSize();

    m_netsListGrid->SetColSize( 0, m_firstWidth );
    m_netsListGrid->SetColSize( 1, size.x - m_firstWidth );
}

DIALOG_SELECT_NET_FROM_LIST::~DIALOG_SELECT_NET_FROM_LIST()
{
}


void DIALOG_SELECT_NET_FROM_LIST::onFilterChange( wxCommandEvent& event )
{
    buildNetsList();
}


void DIALOG_SELECT_NET_FROM_LIST::updateSize( wxSizeEvent& event )
{
    setColumnSize();
    this->Refresh();
    event.Skip();
}


void DIALOG_SELECT_NET_FROM_LIST::onColumnResize( wxGridSizeEvent& event )
{
    m_firstWidth = m_netsListGrid->GetColSize( 0 );
    setColumnSize();
}


void DIALOG_SELECT_NET_FROM_LIST::onSelectCell( wxGridEvent& event )
{

    int selected_row = event.GetRow();
    m_selection = m_netsListGrid->GetCellValue( selected_row, COL_NETNAME );

    // Select the full row when clicking on any cell off the row
    m_netsListGrid->SelectRow( selected_row, false );

    HighlightNet( true, m_selection );
}


void DIALOG_SELECT_NET_FROM_LIST::onCellClick( wxGridEvent& event )
{
    int selected_row = event.GetRow();

    m_selection = m_netsListGrid->GetCellValue( selected_row, COL_NETNAME );
    m_wasSelected = true;

    // Select the full row when clicking on any cell off the row
    m_netsListGrid->SelectRow( selected_row, false );
    m_netsListGrid->SetGridCursor(selected_row, COL_NETNAME );

    HighlightNet( true, m_selection );
}


bool DIALOG_SELECT_NET_FROM_LIST::GetNetName( wxString& aName )
{
    aName = m_selection;
    return m_wasSelected;
}
