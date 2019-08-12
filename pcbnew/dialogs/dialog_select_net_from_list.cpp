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

#include <fctsys.h>
#include <kicad_string.h>
#include <pcbnew.h>
#include <tools/pcb_inspection_tool.h>
#include <class_board.h>
#include <dialog_select_net_from_list_base.h>
#include <eda_pattern_match.h>
#include <wildcards_and_files_ext.h>
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


int PCB_INSPECTION_TOOL::ListNets( const TOOL_EVENT& aEvent )
{
    DIALOG_SELECT_NET_FROM_LIST dlg( m_frame );
    wxString netname;

    if( dlg.ShowModal() == wxID_CANCEL )
    {
        // Clear highlight
        dlg.HighlightNet( "" );
    }

    return 0;
}


DIALOG_SELECT_NET_FROM_LIST::DIALOG_SELECT_NET_FROM_LIST( PCB_EDIT_FRAME* aParent )
    : DIALOG_SELECT_NET_FROM_LIST_BASE( aParent ), m_frame( aParent )
{
    m_brd = aParent->GetBoard();
    m_wasSelected = false;

    m_netsList->AppendTextColumn( getListColumnHeaderNet(),    wxDATAVIEW_CELL_INERT, 0, wxALIGN_LEFT, 0 );
    m_netsList->AppendTextColumn( getListColumnHeaderName(),   wxDATAVIEW_CELL_INERT, 0, wxALIGN_LEFT, 0 );
    m_netsList->AppendTextColumn( getListColumnHeaderCount(),  wxDATAVIEW_CELL_INERT, 0, wxALIGN_CENTER, 0 );
    m_netsList->AppendTextColumn( getListColumnHeaderVias(),   wxDATAVIEW_CELL_INERT, 0, wxALIGN_CENTER, 0 );
    m_netsList->AppendTextColumn( getListColumnHeaderBoard(),  wxDATAVIEW_CELL_INERT, 0, wxALIGN_CENTER, 0 );
    m_netsList->AppendTextColumn( getListColumnHeaderDie(),    wxDATAVIEW_CELL_INERT, 0, wxALIGN_CENTER, 0 );
    m_netsList->AppendTextColumn( getListColumnHeaderLength(), wxDATAVIEW_CELL_INERT, 0, wxALIGN_CENTER, 0 );

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

    constexpr KICAD_T types[] = { PCB_TRACE_T, PCB_VIA_T, PCB_PAD_T, EOT };

    filter.SetPattern( netFilter.MakeUpper() );

    m_netsList->DeleteAllItems();
    m_netsInitialNames.Clear();

    auto connectivity = m_brd->GetConnectivity();

    auto units = GetUserUnits();

    // Populate the nets list with nets names matching the filters:
    // Note: the filtering is case insensitive.
    for( unsigned netcode = 0; netcode < m_brd->GetNetCount(); netcode++ )
    {
        NETINFO_ITEM* net = m_brd->GetNetInfo().GetNetItem( netcode );

        if( !netFilter.IsEmpty() )
        {
            wxString netname = UnescapeString( net->GetNetname() );

            if( filter.Find( netname.MakeUpper() ) == EDA_PATTERN_NOT_FOUND )
                continue;
        }

        unsigned nodes = m_brd->GetNodesCount( netcode );

        if( !m_cbShowZeroPad->IsChecked() && nodes == 0 )
            continue;

        wxVector<wxVariant> dataLine;

        dataLine.push_back( wxVariant( wxString::Format( "%.3d", netcode ) ) );
        dataLine.push_back( wxVariant( UnescapeString( net->GetNetname() ) ) );
        m_netsInitialNames.Add( net->GetNetname() );

        if( netcode )
        {
            dataLine.push_back( wxVariant( wxString::Format( "%u", nodes ) ) );

            int lenPadToDie = 0;
            int len = 0;
            int viaCount = 0;

            for( auto item : connectivity->GetNetItems( netcode, types ) )
            {

                if( item->Type() == PCB_PAD_T )
                {
                    D_PAD *pad = dyn_cast<D_PAD*>( item );
                    lenPadToDie += pad->GetPadToDieLength();
                }
                else if( item->Type() == PCB_TRACE_T )
                {
                    TRACK *track = dyn_cast<TRACK*>( item );
                    len += track->GetLength();
                }
                else if( item->Type() == PCB_VIA_T )
                {
                    viaCount++;
                }
            }

            dataLine.push_back( wxVariant( wxString::Format( "%u", viaCount ) ) );
            dataLine.push_back( wxVariant( MessageTextFromValue( units, len ) ) );
            dataLine.push_back( wxVariant( MessageTextFromValue( units, lenPadToDie ) ) );
            dataLine.push_back( wxVariant( MessageTextFromValue( units, len + lenPadToDie ) ) );
        }
        else    // For the net 0 (unconnected pads), the pad count is not known
        {
            dataLine.push_back( wxVariant( "---" ) );
            dataLine.push_back( wxVariant( "---" ) );   // vias
            dataLine.push_back( wxVariant( "---" ) );   // board
            dataLine.push_back( wxVariant( "---" ) );   // die
            dataLine.push_back( wxVariant( "---" ) );   // length
        }

        m_netsList->AppendItem( dataLine );
    }

    m_wasSelected = false;
}


void DIALOG_SELECT_NET_FROM_LIST::HighlightNet( const wxString& aNetName )
{
    int           netCode = -1;

    if( !aNetName.IsEmpty() )
    {
        NETINFO_ITEM* net = m_brd->FindNet( aNetName );

        if( net )
            netCode = net->GetNet();
    }

    KIGFX::RENDER_SETTINGS* render = m_frame->GetCanvas()->GetView()->GetPainter()->GetSettings();
    render->SetHighlight( netCode >= 0, netCode );

    m_frame->GetCanvas()->GetView()->UpdateAllLayersColor();
    m_frame->GetCanvas()->Refresh();
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
        // We no not use the displayed net name returnded by
        // m_netsList->GetTextValue( selected_row, 1 ); because we need the initial escaped net name
        m_selection = m_netsInitialNames[ selected_row ];
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
    int w0, w1, w2, w3, w4, w5, w6;

    /**
     * Calculating optimal width of the first (Net) and
     * the last (Pad Count) columns. That width must be
     * enough to fit column header label and be not less
     * than width of four chars (0000).
     */

    wxClientDC dc( GetParent() );
    int h, minw, minw_col0;

    dc.GetTextExtent( getListColumnHeaderNet()+"MM", &w0, &h );
    dc.GetTextExtent( getListColumnHeaderCount()+"MM", &w2, &h );
    dc.GetTextExtent( getListColumnHeaderVias()+"MM", &w3, &h );
    dc.GetTextExtent( getListColumnHeaderBoard()+"MM", &w4, &h );
    dc.GetTextExtent( getListColumnHeaderDie()+"MM", &w5, &h );
    dc.GetTextExtent( getListColumnHeaderLength()+"MM", &w6, &h );
    dc.GetTextExtent( "M00000,000 mmM", &minw, &h );
    dc.GetTextExtent( "M00000M", &minw_col0, &h );

    // Considering left and right margins.
    // For wxRenderGeneric it is 5px.
    w0 = std::max( w0+10, minw_col0);
    w2 = std::max( w2+10, minw);
    w3 = std::max( w3+10, minw);
    w4 = std::max( w4+10, minw);
    w5 = std::max( w5+10, minw);
    w6 = std::max( w6+10, minw);

    m_netsList->GetColumn( 0 )->SetWidth( w0 );
    m_netsList->GetColumn( 2 )->SetWidth( w2 );
    m_netsList->GetColumn( 3 )->SetWidth( w3 );
    m_netsList->GetColumn( 4 )->SetWidth( w4 );
    m_netsList->GetColumn( 5 )->SetWidth( w5 );
    m_netsList->GetColumn( 6 )->SetWidth( w6 );

    // At resizing of the list the width of middle column (Net Names) changes only.
    int width = m_netsList->GetClientSize().x;
    w1 = width - w0 - w2 - w3 - w4 - w5 - w6;

    // Column 1 (net names) need a minimal width to display net names
    dc.GetTextExtent( "MMMMMMMMMMMMMMMMMMMM", &minw, &h );
    w1 = std::max( w1, minw );

    m_netsList->GetColumn( 1 )->SetWidth( w1 );

    m_netsList->Refresh();
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
                    m_netsList->GetTextValue( row, 0 ),     // net id
                    m_netsList->GetTextValue( row, 1 ),     // net name
                    m_netsList->GetTextValue( row, 2 ),     // Pad count
                    m_netsList->GetTextValue( row, 3 ),     // Via count
                    m_netsList->GetTextValue( row, 4 ),     // Board length
                    m_netsList->GetTextValue( row, 5 ),     // Die length
                    m_netsList->GetTextValue( row, 6 ) );   // net length

        f.AddLine( txt );
   }

   f.Write();
   f.Close();
}

