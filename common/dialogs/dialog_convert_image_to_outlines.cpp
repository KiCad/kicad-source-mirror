/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "dialogs/dialog_convert_image_to_outlines.h"

#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/sizer.h>
#include <wx/slider.h>
#include <wx/stattext.h>


DIALOG_CONVERT_IMAGE_TO_OUTLINES::DIALOG_CONVERT_IMAGE_TO_OUTLINES( wxWindow* aParent, const wxArrayString& aLayerNames,
                                                                    int                                 aSelectedLayer,
                                                                    std::function<void( double, bool )> aOnChange ) :
        DIALOG_SHIM( aParent, wxID_ANY, _( "Convert to Outlines" ) ),
        m_threshold( nullptr ),
        m_invert( nullptr ),
        m_layer( nullptr ),
        m_onChange( std::move( aOnChange ) )
{
    wxBoxSizer* mainSizer = new wxBoxSizer( wxVERTICAL );

    wxStaticText* thresholdLabel = new wxStaticText( this, wxID_ANY, _( "Black / white threshold:" ) );
    mainSizer->Add( thresholdLabel, 0, wxTOP | wxRIGHT | wxLEFT, 5 );

    m_threshold =
            new wxSlider( this, wxID_ANY, 50, 0, 100, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_LABELS );
    m_threshold->SetMinSize( wxSize( FromDIP( 360 ), -1 ) );
    mainSizer->Add( m_threshold, 0, wxEXPAND | wxRIGHT | wxLEFT, 5 );

    m_invert = new wxCheckBox( this, wxID_ANY, _( "Invert Colors" ) );
    mainSizer->Add( m_invert, 0, wxALL, 5 );

    if( !aLayerNames.IsEmpty() )
    {
        wxBoxSizer* layerSizer = new wxBoxSizer( wxHORIZONTAL );

        wxStaticText* layerLabel = new wxStaticText( this, wxID_ANY, _( "Layer:" ) );
        layerSizer->Add( layerLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5 );

        m_layer = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, aLayerNames );
        m_layer->SetSelection( aSelectedLayer );

        // A remembered index would be a different layer on a board with another layer set.
        OptOut( m_layer );

        layerSizer->Add( m_layer, 1, wxALIGN_CENTER_VERTICAL, 5 );

        mainSizer->Add( layerSizer, 0, wxEXPAND | wxALL, 5 );
    }

    wxStdDialogButtonSizer* buttonSizer = new wxStdDialogButtonSizer();
    buttonSizer->AddButton( new wxButton( this, wxID_OK ) );
    buttonSizer->AddButton( new wxButton( this, wxID_CANCEL ) );
    buttonSizer->Realize();

    mainSizer->Add( buttonSizer, 0, wxALL | wxEXPAND, 5 );

    m_hash_key = "DIALOG_CONVERT_IMAGE_TO_OUTLINES";

    SetSizer( mainSizer );
    SetupStandardButtons();
    Layout();

    m_threshold->Bind( wxEVT_SCROLL_CHANGED, &DIALOG_CONVERT_IMAGE_TO_OUTLINES::onThresholdScrolled, this );
    m_threshold->Bind( wxEVT_SCROLL_THUMBTRACK, &DIALOG_CONVERT_IMAGE_TO_OUTLINES::onThresholdScrolled, this );
    m_invert->Bind( wxEVT_CHECKBOX, &DIALOG_CONVERT_IMAGE_TO_OUTLINES::onSettingChanged, this );

    finishDialogSettings();
}


double DIALOG_CONVERT_IMAGE_TO_OUTLINES::GetThreshold() const
{
    return (double) m_threshold->GetValue() / m_threshold->GetMax();
}


bool DIALOG_CONVERT_IMAGE_TO_OUTLINES::GetInvert() const
{
    return m_invert->IsChecked();
}


int DIALOG_CONVERT_IMAGE_TO_OUTLINES::GetLayerSelection() const
{
    return m_layer ? m_layer->GetSelection() : -1;
}


void DIALOG_CONVERT_IMAGE_TO_OUTLINES::onSettingChanged( wxCommandEvent& aEvent )
{
    if( m_onChange )
        m_onChange( GetThreshold(), GetInvert() );
}


void DIALOG_CONVERT_IMAGE_TO_OUTLINES::onThresholdScrolled( wxScrollEvent& aEvent )
{
    if( m_onChange )
        m_onChange( GetThreshold(), GetInvert() );
}
