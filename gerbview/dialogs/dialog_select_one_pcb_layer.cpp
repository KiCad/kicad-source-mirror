/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012-2014 Jean-Pierre Charras  jp.charras at wanadoo.fr
 * Copyright (C) 1992-2021 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file dialog_select_one_pcb_layer.cpp
 * @brief Set up a dialog to choose a PCB Layer.
 */

#include <gerbview_frame.h>
#include <dialogs/dialog_layers_select_to_pcb.h>

#include <wx/radiobox.h>

#define NB_PCB_LAYERS PCB_LAYER_ID_COUNT
#define FIRST_COPPER_LAYER 0
#define LAST_COPPER_LAYER 31

// Exported function
const wxString GetPCBDefaultLayerName( int aLayerId );


enum layer_sel_id {
    ID_LAYER_SELECT_TOP = 1800,
    ID_LAYER_SELECT_BOTTOM,
    ID_LAYER_SELECT
};


class SELECT_LAYER_DIALOG : public DIALOG_SHIM
{
public:
    SELECT_LAYER_DIALOG( GERBVIEW_FRAME* parent, int aDefaultLayer, int aCopperLayerCount,
                         wxString aGerberName );
    ~SELECT_LAYER_DIALOG() { };

    int GetSelectedLayer() { return m_selectedLayer; }

protected:
    bool TransferDataFromWindow() override;

private:
    void OnLayerSelected( wxCommandEvent& event );

    DECLARE_EVENT_TABLE()

    int               m_selectedLayer;
    wxRadioBox*       m_layerRadioBox;
    std::vector <int> m_layerId;
};


BEGIN_EVENT_TABLE( SELECT_LAYER_DIALOG, wxDialog )
    EVT_RADIOBOX( ID_LAYER_SELECT, SELECT_LAYER_DIALOG::OnLayerSelected )
END_EVENT_TABLE()


int GERBVIEW_FRAME::SelectPCBLayer( int aDefaultLayer, int aCopperLayerCount,
                                    const wxString& aGerberName )
{
    SELECT_LAYER_DIALOG* frame =
            new SELECT_LAYER_DIALOG( this, aDefaultLayer, aCopperLayerCount, aGerberName );

    frame->ShowModal();
    frame->Destroy();
    return frame->GetSelectedLayer();
}


SELECT_LAYER_DIALOG::SELECT_LAYER_DIALOG( GERBVIEW_FRAME* parent, int aDefaultLayer,
                                          int aCopperLayerCount, wxString aGerberName )
    : DIALOG_SHIM( parent, -1, wxString::Format( _( "Select Layer: %s" ), aGerberName ),
                   wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER )
{
    wxButton* button;
    int ii;
    wxArrayString  layerList;
    int selected = -1;

    // Store the passed default layer in case the user hits Cancel
    m_selectedLayer = aDefaultLayer;

    // Build the layer list; first build copper layers list
    int layerCount = 0;

    for( ii = FIRST_COPPER_LAYER; ii <= LAST_COPPER_LAYER; ++ii )
    {
        if( ii == FIRST_COPPER_LAYER || ii == LAST_COPPER_LAYER || ii < aCopperLayerCount-1 )
        {
            layerList.Add( GetPCBDefaultLayerName( ii ) );

            if( aDefaultLayer == ii )
                selected = layerCount;

            m_layerId.push_back( ii );
            layerCount++;
        }
    }

    // Build the layer list; build non copper layers list
    for( ; ; ++ii )
    {
        if( GetPCBDefaultLayerName( ii ) == "" )    // End of list
            break;

        layerList.Add( GetPCBDefaultLayerName( ii ) );

        if( aDefaultLayer == ii )
            selected = layerCount;

        m_layerId.push_back( ii );
        layerCount++;
    }

    layerList.Add( _( "Hole data" ) );

    if( aDefaultLayer == UNDEFINED_LAYER )
        selected = layerCount;

    m_layerId.push_back( UNDEFINED_LAYER );
    layerCount++;

    layerList.Add( _( "Do not export" ) );

    if( aDefaultLayer == UNSELECTED_LAYER )
        selected = layerCount;

    m_layerId.push_back( UNSELECTED_LAYER );
    layerCount++;

    m_layerRadioBox = new wxRadioBox( this, ID_LAYER_SELECT, _( "Layer" ), wxDefaultPosition,
                                      wxDefaultSize, layerList, std::min( layerCount, 12 ),
                                      wxRA_SPECIFY_ROWS );

    if( selected >= 0 )
        m_layerRadioBox->SetSelection( selected );

    wxBoxSizer* mainSizer = new wxBoxSizer( wxHORIZONTAL );
    SetSizer( mainSizer );
    mainSizer->Add( m_layerRadioBox, 1, wxEXPAND | wxALIGN_TOP | wxALL, 5 );
    wxBoxSizer* buttonsSizer = new wxBoxSizer( wxVERTICAL );
    mainSizer->Add( buttonsSizer, 0, wxALIGN_BOTTOM | wxALL, 5 );

    button = new wxButton( this, wxID_OK, _( "OK" ) );
    button->SetDefault();
    buttonsSizer->Add( button, 0, wxGROW | wxALL, 5 );

    button = new wxButton( this, wxID_CANCEL, _( "Cancel" ) );
    buttonsSizer->Add( button, 0, wxGROW | wxALL, 5 );

#ifdef __WXOSX__
    // Hack to fix clipped radio buttons on OSX
    wxSize size = m_layerRadioBox->GetBestSize() + wxSize( 20, 0 );
    m_layerRadioBox->SetMinSize( size );
#endif

    GetSizer()->SetSizeHints( this );

    Center();
}


void SELECT_LAYER_DIALOG::OnLayerSelected( wxCommandEvent& event )
{
    wxPostEvent( this, wxCommandEvent( wxEVT_COMMAND_BUTTON_CLICKED, wxID_OK ) );
}


bool SELECT_LAYER_DIALOG::TransferDataFromWindow()
{
    if( !wxDialog::TransferDataFromWindow() )
        return false;

    m_selectedLayer = m_layerId[m_layerRadioBox->GetSelection()];
    return true;
}


// This function is a duplicate of
// const wxChar* LSET::Name( PCB_LAYER_ID aLayerId )
// However it avoids a dependency to Pcbnew code.
const wxString GetPCBDefaultLayerName( int aLayerId )
{
    const wxChar* txt;

    // using a switch to explicitly show the mapping more clearly
    switch( aLayerId )
    {
    case F_Cu:              txt = wxT( "F.Cu" );            break;
    case In1_Cu:            txt = wxT( "In1.Cu" );          break;
    case In2_Cu:            txt = wxT( "In2.Cu" );          break;
    case In3_Cu:            txt = wxT( "In3.Cu" );          break;
    case In4_Cu:            txt = wxT( "In4.Cu" );          break;
    case In5_Cu:            txt = wxT( "In5.Cu" );          break;
    case In6_Cu:            txt = wxT( "In6.Cu" );          break;
    case In7_Cu:            txt = wxT( "In7.Cu" );          break;
    case In8_Cu:            txt = wxT( "In8.Cu" );          break;
    case In9_Cu:            txt = wxT( "In9.Cu" );          break;
    case In10_Cu:           txt = wxT( "In10.Cu" );         break;
    case In11_Cu:           txt = wxT( "In11.Cu" );         break;
    case In12_Cu:           txt = wxT( "In12.Cu" );         break;
    case In13_Cu:           txt = wxT( "In13.Cu" );         break;
    case In14_Cu:           txt = wxT( "In14.Cu" );         break;
    case In15_Cu:           txt = wxT( "In15.Cu" );         break;
    case In16_Cu:           txt = wxT( "In16.Cu" );         break;
    case In17_Cu:           txt = wxT( "In17.Cu" );         break;
    case In18_Cu:           txt = wxT( "In18.Cu" );         break;
    case In19_Cu:           txt = wxT( "In19.Cu" );         break;
    case In20_Cu:           txt = wxT( "In20.Cu" );         break;
    case In21_Cu:           txt = wxT( "In21.Cu" );         break;
    case In22_Cu:           txt = wxT( "In22.Cu" );         break;
    case In23_Cu:           txt = wxT( "In23.Cu" );         break;
    case In24_Cu:           txt = wxT( "In24.Cu" );         break;
    case In25_Cu:           txt = wxT( "In25.Cu" );         break;
    case In26_Cu:           txt = wxT( "In26.Cu" );         break;
    case In27_Cu:           txt = wxT( "In27.Cu" );         break;
    case In28_Cu:           txt = wxT( "In28.Cu" );         break;
    case In29_Cu:           txt = wxT( "In29.Cu" );         break;
    case In30_Cu:           txt = wxT( "In30.Cu" );         break;
    case B_Cu:              txt = wxT( "B.Cu" );            break;

    // Technicals
    case B_Adhes:           txt = wxT( "B.Adhes" );         break;
    case F_Adhes:           txt = wxT( "F.Adhes" );         break;
    case B_Paste:           txt = wxT( "B.Paste" );         break;
    case F_Paste:           txt = wxT( "F.Paste" );         break;
    case B_SilkS:           txt = wxT( "B.SilkS" );         break;
    case F_SilkS:           txt = wxT( "F.SilkS" );         break;
    case B_Mask:            txt = wxT( "B.Mask" );          break;
    case F_Mask:            txt = wxT( "F.Mask" );          break;

    // Users
    case Dwgs_User:         txt = wxT( "Dwgs.User" );       break;
    case Cmts_User:         txt = wxT( "Cmts.User" );       break;
    case Eco1_User:         txt = wxT( "Eco1.User" );       break;
    case Eco2_User:         txt = wxT( "Eco2.User" );       break;
    case Edge_Cuts:         txt = wxT( "Edge.Cuts" );       break;

    // Pcbnew knows some other layers, but any other layer is not suitable for export.

    default:    // Sentinel
        txt = wxT( "" ); break;
    }

    return wxString( txt );
}
