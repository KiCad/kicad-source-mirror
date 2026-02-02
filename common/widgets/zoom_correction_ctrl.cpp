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
#include <wx/display.h>
#include <wx/math.h>
#include <wx/sizer.h>
#include <wx/spinctrl.h>

class ZOOM_CORRECTION_RULER : public wxPanel
{
public:
    ZOOM_CORRECTION_RULER( wxWindow* aParent ) :
            wxPanel( aParent, wxID_ANY, wxDefaultPosition, aParent->FromDIP( wxSize( 200, 30 ) ),
                     wxTAB_TRAVERSAL | wxNO_BORDER | wxFULL_REPAINT_ON_RESIZE )
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
        int    minLabelSpacing = GetTextExtent( wxT( "000_" ) ).x;
        double lastLabelX = -minLabelSpacing;
        int    majorTickEvery = 10;
        double tickLabelDiv = 1;
        double pxPerMinorTick;

        // Draw major and minor tick marks with numbering
        if( units == ZOOM_CORRECTION_UNITS::INCH )
        {
            majorTickEvery = 4;
            tickLabelDiv = 4;
            pxPerMinorTick = pxPerUnit / 4.0; // 1/4 inch intervals
        }
        else if( units == ZOOM_CORRECTION_UNITS::CM )
        {
            majorTickEvery = 10;
            tickLabelDiv = 10;
            pxPerMinorTick = pxPerUnit / 10.0; // 1mm intervals
        }
        else // MM
        {
            // For mm: Same as cm ruler but numbers count by 10 (so 10mm, 20mm, etc.)
            majorTickEvery = 10;
            tickLabelDiv = 1;
            pxPerMinorTick = pxPerUnit;
        }

        if( pxPerMinorTick < 3 )
        {
            pxPerMinorTick *= 2;
            majorTickEvery /= 2;
            tickLabelDiv /= 2;
        }

        for( double x = 0; x <= size.x; x += pxPerMinorTick )
        {
            int tickCount = (int) round( x / pxPerMinorTick );

            if( tickCount % majorTickEvery == 0 )
            {
                // Major tick
                dc.DrawLine( x, size.y - 1, x, size.y - 16 );

                int labelNum = wxRound(tickCount / tickLabelDiv);

                if( labelNum > 0 && x < size.x - 10 && ( x - lastLabelX ) >= minLabelSpacing )
                {
                    wxString label = wxString::Format( wxT( "%d" ), labelNum );
                    wxSize   textSize = dc.GetTextExtent( label );
                    dc.DrawText( label, x - textSize.x / 2, 0 );
                    lastLabelX = x;
                }
            }
            else
            {
                // Minor tick
                dc.DrawLine( x, size.y - 1, x, size.y - 8 );
            }
        }
    }
};


ZOOM_CORRECTION_CTRL::ZOOM_CORRECTION_CTRL( wxWindow* aParent, double& aValue, double aBaseValue ) :
        wxPanel( aParent, wxID_ANY ),
        m_baseValue( aBaseValue ),
        m_value( &aValue )
{
    wxBoxSizer* topSizer = new wxBoxSizer( wxVERTICAL );

    // Top section: Label, spinner, auto button
    wxBoxSizer* controlsSizer = new wxBoxSizer( wxHORIZONTAL );

    m_label = new wxStaticText( this, wxID_ANY, _( "Display PPI: " ) );
    controlsSizer->Add( m_label, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, KIUI::GetStdMargin() );

    m_spinner = new wxSpinCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
                                wxSP_ARROW_KEYS, 10, 1000, (int)( aValue * m_baseValue ) );
    controlsSizer->Add( m_spinner, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, KIUI::GetStdMargin() );

    m_autoButton = new wxButton( this, wxID_ANY, _( "Detect" ) );
    controlsSizer->Add( m_autoButton, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, KIUI::GetStdMargin() );

    topSizer->Add( controlsSizer, 0, wxEXPAND, KIUI::GetStdMargin() );

    // Middle section: Ruler and units choice
    wxBoxSizer* rulerSizer = new wxBoxSizer( wxHORIZONTAL );

    m_ruler = new ZOOM_CORRECTION_RULER( this );
    rulerSizer->Add( m_ruler, 1, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, KIUI::GetStdMargin() );

    wxArrayString choices;
    choices.Add( wxT( "mm" ) );
    choices.Add( wxT( "cm" ) );
    choices.Add( wxT( "in" ) );
    m_unitsChoice = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, choices );
    m_unitsChoice->SetSelection( 0 ); // Default to MM
    rulerSizer->Add( m_unitsChoice, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT | wxTOP, KIUI::GetStdMargin() );

    topSizer->Add( rulerSizer, 0, wxEXPAND | wxTOP, KIUI::GetStdMargin() );

    SetSizer( topSizer );

    m_autoButton->Bind( wxEVT_BUTTON, &ZOOM_CORRECTION_CTRL::autoPressed, this );
    m_spinner->Bind( wxEVT_SPINCTRL, &ZOOM_CORRECTION_CTRL::spinnerChanged, this );
    m_unitsChoice->Bind( wxEVT_CHOICE, &ZOOM_CORRECTION_CTRL::unitsChanged, this );

    // Ensure initial synchronization of all controls
    m_ruler->Refresh();
}


void ZOOM_CORRECTION_CTRL::SetDisplayedValue( double aValue )
{
    m_spinner->SetValue( (int)( aValue * m_baseValue ) );
    m_ruler->Refresh();
}


double ZOOM_CORRECTION_CTRL::GetValue() const
{
    return (double) m_spinner->GetValue() / m_baseValue;
}


int ZOOM_CORRECTION_CTRL::GetUnitsSelection() const
{
    return m_unitsChoice->GetSelection();
}


bool ZOOM_CORRECTION_CTRL::TransferDataToWindow()
{
    m_spinner->SetValue( (int)( *m_value * m_baseValue ) );
    m_ruler->Refresh();
    return true;
}


bool ZOOM_CORRECTION_CTRL::TransferDataFromWindow()
{
    *m_value = GetValue();
    return true;
}


void ZOOM_CORRECTION_CTRL::spinnerChanged( wxSpinEvent& )
{
    *m_value = m_spinner->GetValue() / m_baseValue;
    m_ruler->Refresh();
}


void ZOOM_CORRECTION_CTRL::unitsChanged( wxCommandEvent& )
{
    m_ruler->Refresh();
}


void ZOOM_CORRECTION_CTRL::autoPressed( wxCommandEvent& aEvent )
{
    wxDisplay dpy( this );
    double    val = 0.0;

#if wxCHECK_VERSION( 3, 3, 2 )
    wxSize rawPPI = dpy.GetRawPPI();
    val = wxRound( ( rawPPI.x + rawPPI.y ) / 2.0 );
#endif

    if( val < 10 )
    {
#ifdef wxHAS_DPI_INDEPENDENT_PIXELS
        val = dpy.GetStdPPIValue() / dpy.GetScaleFactor();
#else
        wxSize ppi = dpy.GetPPI();
        val = wxRound( ( ppi.x + ppi.y ) / 2.0 );
#endif
    }

    m_spinner->SetValue( val );
    m_ruler->Refresh();
}
