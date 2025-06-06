/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012-2014 Jean-Pierre Charras  jp.charras at wanadoo.fr
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
#include <layer_range.h>
#include <lset.h>
#include <dialogs/dialog_map_gerber_layers_to_pcb.h>

#include <wx/radiobox.h>


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


BEGIN_EVENT_TABLE( SELECT_LAYER_DIALOG, DIALOG_SHIM )
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
    wxArrayString  layerList;
    int selected = -1;

    // Store the passed default layer in case the user hits Cancel
    m_selectedLayer = aDefaultLayer;

    // Build the layer list; first build copper layers list
    LSET layers = LSET::AllCuMask( aCopperLayerCount ) | LSET::AllTechMask() | LSET::UserMask();

    for( auto copper_it = layers.copper_layers_begin(); copper_it != layers.copper_layers_end(); ++copper_it )
    {
        layerList.Add( LSET::Name( *copper_it ) );

        if( aDefaultLayer == *copper_it )
            selected = m_layerId.size();

        m_layerId.push_back( *copper_it );
    }

    for( auto non_copper_it = layers.non_copper_layers_begin(); non_copper_it != layers.non_copper_layers_end(); ++non_copper_it )
    {
        layerList.Add( LSET::Name( *non_copper_it ) );

        if( aDefaultLayer == *non_copper_it )
            selected = m_layerId.size();

        m_layerId.push_back( *non_copper_it );
    }

    layerList.Add( _( "Hole data" ) );

    if( aDefaultLayer == UNDEFINED_LAYER )
        selected = m_layerId.size();

    m_layerId.push_back( UNDEFINED_LAYER );

    layerList.Add( _( "Do not export" ) );

    if( aDefaultLayer == UNSELECTED_LAYER )
        selected = m_layerId.size();

    m_layerId.push_back( UNSELECTED_LAYER );

    m_layerRadioBox = new wxRadioBox( this, ID_LAYER_SELECT, _( "Layer" ), wxDefaultPosition,
                                      wxDefaultSize, layerList, std::min( int( m_layerId.size() ), 12 ),
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
