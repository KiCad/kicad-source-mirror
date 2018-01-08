/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017-2018 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <dialog_modedit_display_options.h>

#include <class_drawpanel.h>
#include <module_editor_frame.h>

#include <view/view.h>

#include <widgets/gal_options_panel.h>
#include <widgets/stepped_slider.h>
#include <wx/checkbox.h>
#include <wx/sizer.h>


bool DIALOG_MODEDIT_DISPLAY_OPTIONS::Invoke( FOOTPRINT_EDIT_FRAME& aCaller )
{
    DIALOG_MODEDIT_DISPLAY_OPTIONS dlg( aCaller );

    int ret = dlg.ShowModal();

    return ret == wxID_OK;
}


DIALOG_MODEDIT_DISPLAY_OPTIONS::DIALOG_MODEDIT_DISPLAY_OPTIONS( FOOTPRINT_EDIT_FRAME& aParent ) :
    DIALOG_SHIM( &aParent, wxID_ANY, _( "Display Options" ) ),
    m_parent( aParent ),
    m_last_scale( -1 )
{
    auto mainSizer = new wxBoxSizer( wxVERTICAL );
    SetSizer( mainSizer );

    // install GAL options pane
    KIGFX::GAL_DISPLAY_OPTIONS& galOptions = m_parent.GetGalDisplayOptions();

    m_galOptsPanel = new GAL_OPTIONS_PANEL( this, galOptions );
    mainSizer->Add( m_galOptsPanel, 1, wxEXPAND, 0 );

    auto fgsizer = new wxFlexGridSizer( 3 );
    fgsizer->AddGrowableCol( 1 );
    fgsizer->SetFlexibleDirection( wxBOTH );
    fgsizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
    fgsizer->Add(
            new wxStaticText( this, wxID_ANY, _( "Icon scale:" ) ),
            0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 3 );
    m_scaleSlider = new STEPPED_SLIDER( this, wxID_ANY, 50, 50, 275,
            wxDefaultPosition, wxDefaultSize,
            wxSL_AUTOTICKS | wxSL_HORIZONTAL | wxSL_LABELS );
    m_scaleSlider->SetStep( 25 );
    fgsizer->Add( m_scaleSlider, 1, wxLEFT | wxRIGHT | wxEXPAND, 3 );
    fgsizer->Add(
            new wxStaticText( this, wxID_ANY, "%" ),
            0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 3 );
    fgsizer->AddSpacer( 0 );
    m_scaleAuto = new wxCheckBox( this, wxID_ANY, _( "Auto" ) );
    fgsizer->Add( m_scaleAuto, wxLEFT | wxRIGHT | wxEXPAND, 3 );
    fgsizer->AddSpacer( 0 );

    mainSizer->Add( fgsizer, 0, wxEXPAND | wxBOTTOM | wxLEFT | wxRIGHT, 5 );

    auto btnSizer = new wxStdDialogButtonSizer();
    mainSizer->Add( btnSizer, 0, wxEXPAND | wxBOTTOM | wxLEFT | wxRIGHT, 5 );

    btnSizer->AddButton( new wxButton( this, wxID_OK ) );
    btnSizer->AddButton( new wxButton( this, wxID_CANCEL ) );

    btnSizer->Realize();

    std::vector<wxEventTypeTag<wxScrollEvent>> scroll_events = {
        wxEVT_SCROLL_TOP, wxEVT_SCROLL_BOTTOM, wxEVT_SCROLL_LINEUP, wxEVT_SCROLL_LINEDOWN,
        wxEVT_SCROLL_PAGEUP, wxEVT_SCROLL_PAGEDOWN, wxEVT_SCROLL_THUMBTRACK,
        wxEVT_SCROLL_THUMBRELEASE };

    for( auto evt : scroll_events )
        m_scaleSlider->Connect(
                evt, wxScrollEventHandler( DIALOG_MODEDIT_DISPLAY_OPTIONS::OnScaleSlider ),
                NULL, this );

    m_scaleAuto->Connect(
            wxEVT_COMMAND_CHECKBOX_CLICKED,
            wxCommandEventHandler( DIALOG_MODEDIT_DISPLAY_OPTIONS::OnScaleAuto ),
            NULL, this );

    GetSizer()->SetSizeHints( this );
    Centre();
}


bool DIALOG_MODEDIT_DISPLAY_OPTIONS::TransferDataToWindow()
{
    // update GAL options
    m_galOptsPanel->TransferDataToWindow();

    const int scale_fourths = m_parent.GetIconScale();

    if( scale_fourths <= 0 )
    {
        m_scaleAuto->SetValue( true );
        m_scaleSlider->SetValue( 25 * KiIconScale( &m_parent ) );
    }
    else
    {
        m_scaleAuto->SetValue( false );
        m_scaleSlider->SetValue( scale_fourths * 25 );
    }

    return true;
}


bool DIALOG_MODEDIT_DISPLAY_OPTIONS::TransferDataFromWindow()
{
    // update GAL options
    m_galOptsPanel->TransferDataFromWindow();

    const int scale_fourths = m_scaleAuto->GetValue() ? -1 : m_scaleSlider->GetValue() / 25;

    if( m_parent.GetIconScale() != scale_fourths )
        m_parent.SetIconScale( scale_fourths );

    // refresh view
    KIGFX::VIEW* view = m_parent.GetGalCanvas()->GetView();
    view->RecacheAllItems();
    view->MarkTargetDirty( KIGFX::TARGET_NONCACHED );
    m_parent.GetCanvas()->Refresh();

    return true;
}


void DIALOG_MODEDIT_DISPLAY_OPTIONS::OnScaleSlider( wxScrollEvent& aEvent )
{
    m_scaleAuto->SetValue( false );
    aEvent.Skip();
}


void DIALOG_MODEDIT_DISPLAY_OPTIONS::OnScaleAuto( wxCommandEvent& aEvent )
{
    if( m_scaleAuto->GetValue() )
    {
        m_last_scale = m_scaleSlider->GetValue();
        m_scaleSlider->SetValue( 25 * KiIconScale( GetParent() ) );
    }
    else
    {
        if( m_last_scale >= 0 )
            m_scaleSlider->SetValue( m_last_scale );
    }
}
