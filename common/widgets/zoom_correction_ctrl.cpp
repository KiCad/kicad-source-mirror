/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Inspired by the Inkscape preferences widget, which is
 * Authors:
 *   Marco Scholten
 *   Bruno Dilly <bruno.dilly@gmail.com>
 *
 * Copyright (C) 2004, 2006, 2007 Authors
 *
 * Released under GNU GPL v2+
 */

#include <advanced_config.h>
#include <widgets/zoom_correction_ctrl.h>
#include <widgets/ui_common.h>

#include <wx/dcclient.h>
#include <wx/sizer.h>
#include <wx/spinctrl.h>

class ZOOM_CORRECTION_RULER : public wxPanel
{
public:
    ZOOM_CORRECTION_RULER( wxWindow* aParent ) :
            wxPanel( aParent, wxID_ANY, wxDefaultPosition, wxSize( 200, 40 ) )
    {
        Bind( wxEVT_PAINT, &ZOOM_CORRECTION_RULER::OnPaint, this );
    }

private:
    void OnPaint( wxPaintEvent& )
    {
        wxPaintDC dc( this );
        wxSize    size = GetClientSize();

        // Draw baseline
        dc.DrawLine( 0, size.y - 1, size.x, size.y - 1 );
        ZOOM_CORRECTION_CTRL* parent = static_cast<ZOOM_CORRECTION_CTRL*>( GetParent() );

        if( !parent )
            return;

        double value = parent->GetValue();
        ZOOM_CORRECTION_UNITS units = static_cast<ZOOM_CORRECTION_UNITS>( parent->GetUnitsSelection() );

        double dpi = ADVANCED_CFG::GetCfg().m_ScreenDPI;
        double unitsPerInch = 25.4;

        if( units == ZOOM_CORRECTION_UNITS::CM )
            unitsPerInch = 2.54;
        else if( units == ZOOM_CORRECTION_UNITS::INCH )
            unitsPerInch = 1.0;

        double pxPerUnit = dpi / unitsPerInch * value;

        // Minimum spacing between number labels to prevent overlap (in pixels)
        const int MIN_LABEL_SPACING = 30;
        double lastLabelX = -MIN_LABEL_SPACING;

        // Draw major and minor tick marks with numbering
        if( units == ZOOM_CORRECTION_UNITS::INCH )
        {
            double pxPerMinorTick = pxPerUnit / 4.0; // 1/4 inch intervals

            for( double x = 0; x <= size.x; x += pxPerMinorTick )
            {
                int tickCount = (int)round( x / pxPerMinorTick );

                if( tickCount % 4 == 0 )
                {
                    // Major tick (inch mark)
                    dc.DrawLine( x, size.y - 1, x, size.y - 16 );

                    int inchNum = tickCount / 4;

                    if( inchNum > 0 && x < size.x - 10 && ( x - lastLabelX ) >= MIN_LABEL_SPACING )
                    {
                        wxString label = wxString::Format( wxT("%d"), inchNum );
                        wxSize textSize = dc.GetTextExtent( label );
                        dc.DrawText( label, x - textSize.x / 2, size.y - 40 );
                        lastLabelX = x;
                    }
                }
                else
                {
                    // Minor tick (1/4 inch mark)
                    dc.DrawLine( x, size.y - 1, x, size.y - 8 );
                }
            }
        }
        else if( units == ZOOM_CORRECTION_UNITS::CM )
        {
            double pxPerMinorTick = pxPerUnit / 10.0; // 1mm intervals

            for( double x = 0; x <= size.x; x += pxPerMinorTick )
            {
                int tickCount = (int)round( x / pxPerMinorTick );

                if( tickCount % 10 == 0 )
                {
                    // Major tick (cm mark)
                    dc.DrawLine( x, size.y - 1, x, size.y - 16 );

                    int cmNum = tickCount / 10;

                    if( cmNum > 0 && x < size.x - 10 && ( x - lastLabelX ) >= MIN_LABEL_SPACING )
                    {
                        wxString label = wxString::Format( wxT("%d"), cmNum );
                        wxSize textSize = dc.GetTextExtent( label );
                        dc.DrawText( label, x - textSize.x / 2, size.y - 40 );
                        lastLabelX = x;
                    }
                }
                else
                {
                    // Minor tick (mm mark)
                    dc.DrawLine( x, size.y - 1, x, size.y - 8 );
                }
            }
        }
        else // MM
        {
            // For mm: Same as cm ruler but numbers count by 10 (so 10mm, 20mm, etc.)
            double pxPerMinorTick = pxPerUnit;

            for( double x = 0; x <= size.x; x += pxPerMinorTick )
            {
                int tickCount = (int)round( x / pxPerMinorTick );

                if( tickCount % 10 == 0 )
                {
                    // Major tick (10mm mark)
                    dc.DrawLine( x, size.y - 1, x, size.y - 16 );

                    if( tickCount > 0 && x < size.x - 15 && ( x - lastLabelX ) >= MIN_LABEL_SPACING )
                    {
                        wxString label = wxString::Format( wxT("%d"), tickCount );
                        wxSize textSize = dc.GetTextExtent( label );
                        dc.DrawText( label, x - textSize.x / 2, size.y - 40 );
                        lastLabelX = x;
                    }
                }
                else
                {
                    dc.DrawLine( x, size.y - 1, x, size.y - 8 );
                }
            }
        }
    }
};

ZOOM_CORRECTION_CTRL::ZOOM_CORRECTION_CTRL( wxWindow* aParent, double& aValue ) :
        wxPanel( aParent, wxID_ANY ),
        m_value( &aValue )
{
    wxBoxSizer* topSizer = new wxBoxSizer( wxVERTICAL );

    // Top section: Slider and spinner
    wxBoxSizer* controlsSizer = new wxBoxSizer( wxHORIZONTAL );
    m_slider = new wxSlider( this, wxID_ANY, (int)( aValue * 100 ), 10, 1000, wxDefaultPosition, wxDefaultSize,
                             wxSL_HORIZONTAL | wxSL_VALUE_LABEL );
    controlsSizer->Add( m_slider, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, KIUI::GetStdMargin() );

    m_spinner = new wxSpinCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
                                wxSP_ARROW_KEYS, 10, 1000, (int)( aValue * 100 ) );
    controlsSizer->Add( m_spinner, 0, wxALIGN_CENTER_VERTICAL );

    topSizer->Add( controlsSizer, 0, wxEXPAND | wxALL, KIUI::GetStdMargin() );

    // Middle section: Ruler and units choice
    wxBoxSizer* rulerSizer = new wxBoxSizer( wxHORIZONTAL );

    m_ruler = new ZOOM_CORRECTION_RULER( this );
    rulerSizer->Add( m_ruler, 1, wxEXPAND | wxRIGHT, KIUI::GetStdMargin() );

    wxArrayString choices;
    choices.Add( wxT( "mm" ) );
    choices.Add( wxT( "cm" ) );
    choices.Add( wxT( "in" ) );
    m_unitsChoice = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, choices );
    m_unitsChoice->SetSelection( 0 ); // Default to MM
    rulerSizer->Add( m_unitsChoice, 0, wxALIGN_CENTER_VERTICAL );

    topSizer->Add( rulerSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, KIUI::GetStdMargin() );

    SetSizer( topSizer );

    m_slider->Bind( wxEVT_SLIDER, &ZOOM_CORRECTION_CTRL::sliderChanged, this );
    m_spinner->Bind( wxEVT_SPINCTRL, &ZOOM_CORRECTION_CTRL::spinnerChanged, this );
    m_unitsChoice->Bind( wxEVT_CHOICE, &ZOOM_CORRECTION_CTRL::unitsChanged, this );

    // Ensure initial synchronization of all controls
    m_ruler->Refresh();
}

void ZOOM_CORRECTION_CTRL::SetDisplayedValue( double aValue )
{
    m_slider->SetValue( (int)( aValue * 100 ) );
    m_spinner->SetValue( (int)( aValue * 100 ) );
    m_ruler->Refresh();
}

double ZOOM_CORRECTION_CTRL::GetValue() const
{
    return m_slider->GetValue() / 100.0;
}

int ZOOM_CORRECTION_CTRL::GetUnitsSelection() const
{
    return m_unitsChoice->GetSelection();
}


bool ZOOM_CORRECTION_CTRL::TransferDataToWindow()
{
    m_slider->SetValue( (int)( *m_value * 100 ) );
    m_spinner->SetValue( (int)( *m_value * 100 ) );
    m_ruler->Refresh();
    return true;
}

bool ZOOM_CORRECTION_CTRL::TransferDataFromWindow()
{
    *m_value = GetValue();
    return true;
}

void ZOOM_CORRECTION_CTRL::sliderChanged( wxCommandEvent& )
{
    *m_value = GetValue();
    m_spinner->SetValue( m_slider->GetValue() );
    m_ruler->Refresh();
}

void ZOOM_CORRECTION_CTRL::spinnerChanged( wxSpinEvent& )
{
    *m_value = m_spinner->GetValue() / 100.0;
    m_slider->SetValue( m_spinner->GetValue() );
    m_ruler->Refresh();
}

void ZOOM_CORRECTION_CTRL::unitsChanged( wxCommandEvent& )
{
    m_ruler->Refresh();
}
