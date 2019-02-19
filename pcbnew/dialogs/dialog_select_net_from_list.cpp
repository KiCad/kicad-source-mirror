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

/**
 * @file dialog_select_net_from_list.cpp
 * @brief methods to show available net names and select and highligth a net
 */

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
#include <connectivity/connectivity_data.h>

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
     * @param aNetName is the name of net to be highlighted.  An empty string will unhighlight
     * any currently highlighted net.
     */
    void HighlightNet( const wxString& aNetName );

private:
    void onSelChanged( wxDataViewEvent& event ) override;
    void onFilterChange( wxCommandEvent& event ) override;
    void onListSize( wxSizeEvent& event ) override;

    void buildNetsList();
    wxString getListColumnHeaderNet() { return _( "Net" ); };
    wxString getListColumnHeaderName() { return _( "Name" ); };
    wxString getListColumnHeaderCount() { return _( "Pad Count" ); };
    void adjustListColumns();

    wxString        m_selection;
    bool            m_wasSelected;
    BOARD*          m_brd;
    PCB_EDIT_FRAME* m_frame;
};


void PCB_EDIT_FRAME::ListNetsAndSelect( wxCommandEvent& event )
{
    DIALOG_SELECT_NET_FROM_LIST dlg( this );
    wxString netname;

    if( dlg.ShowModal() == wxID_CANCEL )
    {
        // Clear highlight
        dlg.HighlightNet( "" );
    }
}


DIALOG_SELECT_NET_FROM_LIST::DIALOG_SELECT_NET_FROM_LIST( PCB_EDIT_FRAME* aParent )
    : DIALOG_SELECT_NET_FROM_LIST_BASE( aParent ), m_frame( aParent )
{
    m_brd = aParent->GetBoard();
    m_wasSelected = false;

    m_netsList->AppendTextColumn( getListColumnHeaderNet(),   wxDATAVIEW_CELL_INERT, 0, wxALIGN_LEFT, 0 );
    m_netsList->AppendTextColumn( getListColumnHeaderName(),  wxDATAVIEW_CELL_INERT, 0, wxALIGN_LEFT, 0 );
    m_netsList->AppendTextColumn( getListColumnHeaderCount(), wxDATAVIEW_CELL_INERT, 0, wxALIGN_CENTER, 0 );

    // The fact that we're a list should keep the control from reserving space for the
    // expander buttons... but it doesn't.  Fix by forcing the indent to 0.
    m_netsList->SetIndent( 0 );

    buildNetsList();

    adjustListColumns();

    m_sdbSizerOK->SetDefault();

    FinishDialogSettings();
}


void DIALOG_SELECT_NET_FROM_LIST::buildNetsList()
{
    wxString                   netFilter = m_textCtrlFilter->GetValue();
    EDA_PATTERN_MATCH_WILDCARD filter;

    filter.SetPattern( netFilter.MakeUpper() );

    m_netsList->DeleteAllItems();

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

        wxVector<wxVariant> dataLine;

        dataLine.push_back( wxVariant( wxString::Format( "%.3d", netcode ) ) );
        dataLine.push_back( wxVariant( net->GetNetname() ) );

        if( netcode )
            dataLine.push_back( wxVariant( wxString::Format( "%u", nodes ) ) );
        else    // For the net 0 (unconnected pads), the pad count is not known
            dataLine.push_back( wxVariant( wxT( "---" ) ) );

        m_netsList->AppendItem( dataLine );
    }

    m_wasSelected = false;
}


void DIALOG_SELECT_NET_FROM_LIST::HighlightNet( const wxString& aNetName )
{
    NETINFO_ITEM* net = nullptr;
    int           netCode = -1;

    if( !aNetName.IsEmpty() )
    {
        net = m_brd->FindNet( aNetName );
        netCode = net->GetNet();
    }

    if( m_frame->IsGalCanvasActive() )
    {
        auto galCanvas = m_frame->GetGalCanvas();
        KIGFX::RENDER_SETTINGS* render = galCanvas->GetView()->GetPainter()->GetSettings();
        render->SetHighlight( netCode >= 0, netCode );

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


DIALOG_SELECT_NET_FROM_LIST::~DIALOG_SELECT_NET_FROM_LIST()
{
}


void DIALOG_SELECT_NET_FROM_LIST::onFilterChange( wxCommandEvent& event )
{
    buildNetsList();
}


void DIALOG_SELECT_NET_FROM_LIST::onSelChanged( wxDataViewEvent&  )
{
    int selected_row = m_netsList->GetSelectedRow();

    if( selected_row >= 0 )
    {
        m_selection = m_netsList->GetTextValue( selected_row, 1 );
        m_wasSelected = true;

        HighlightNet( m_selection );
    }
    else
    {
        HighlightNet( "" );
        m_wasSelected = false;
    }
}


void DIALOG_SELECT_NET_FROM_LIST::adjustListColumns()
{
    int w0, w1, w2;

    /**
     * Calculating optimal width of the first (Net) and
     * the last (Pad Count) columns. That width must be
     * enough to fit column header label and be not less
     * than width of four chars (0000).
     */

    wxClientDC dc( GetParent() );
    int h, minw;

    dc.GetTextExtent( getListColumnHeaderNet()+"MM", &w0, &h );
    dc.GetTextExtent( getListColumnHeaderCount()+"MM", &w2, &h );
    dc.GetTextExtent( "M0000M", &minw, &h );

    // Considering left and right margins.
    // For wxRenderGeneric it is 5px.
    w0 = std::max( w0+10, minw);
    w2 = std::max( w2+10, minw);

    m_netsList->GetColumn( 0 )->SetWidth( w0 );
    m_netsList->GetColumn( 2 )->SetWidth( w2 );

    // At resizing of the list the width of middle column (Net Names) changes only.
    int width = m_netsList->GetClientSize().x;
    w1 = width - w0 - w2;

    m_netsList->GetColumn( 1 )->SetWidth( w1 );
}


void DIALOG_SELECT_NET_FROM_LIST::onListSize( wxSizeEvent& aEvent )
{
    aEvent.Skip();
    adjustListColumns();
}


bool DIALOG_SELECT_NET_FROM_LIST::GetNetName( wxString& aName )
{
    aName = m_selection;
    return m_wasSelected;
}
