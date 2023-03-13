/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <widgets/footprint_diff_widget.h>
#include <pcb_painter.h>
#include <footprint.h>
#include <settings/settings_manager.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/slider.h>


FOOTPRINT_DIFF_WIDGET::FOOTPRINT_DIFF_WIDGET( wxWindow* aParent, KIWAY& aKiway ) :
        FOOTPRINT_PREVIEW_WIDGET( aParent, aKiway ),
        m_libraryItem( nullptr ),
        m_slider( nullptr )
{
    wxBoxSizer* bottomSizer = new wxBoxSizer( wxHORIZONTAL );

    wxStaticText* schLabel = new wxStaticText( this, wxID_ANY, _( "Board" ) );
    wxStaticText* libLabel = new wxStaticText( this, wxID_ANY, _( "Library" ) );
    m_slider = new wxSlider( this, wxID_ANY, 50, 0, 100 );

    bottomSizer->Add( schLabel, 0, wxLEFT | wxRIGHT | wxBOTTOM | wxALIGN_CENTRE_VERTICAL, 6 );
    bottomSizer->Add( m_slider, 1, wxLEFT | wxRIGHT | wxALIGN_BOTTOM, 30 );
    bottomSizer->Add( libLabel, 0, wxLEFT | wxRIGHT | wxBOTTOM | wxALIGN_CENTRE_VERTICAL, 6 );

    m_outerSizer->Add( bottomSizer, 0, wxTOP | wxLEFT | wxRIGHT | wxEXPAND, 10 );

    Layout();

    m_slider->Bind( wxEVT_SCROLL_TOP, &FOOTPRINT_DIFF_WIDGET::onSlider, this );
   	m_slider->Bind( wxEVT_SCROLL_BOTTOM, &FOOTPRINT_DIFF_WIDGET::onSlider, this );
   	m_slider->Bind( wxEVT_SCROLL_LINEUP, &FOOTPRINT_DIFF_WIDGET::onSlider, this );
   	m_slider->Bind( wxEVT_SCROLL_LINEDOWN, &FOOTPRINT_DIFF_WIDGET::onSlider, this );
   	m_slider->Bind( wxEVT_SCROLL_PAGEUP, &FOOTPRINT_DIFF_WIDGET::onSlider, this );
   	m_slider->Bind( wxEVT_SCROLL_PAGEDOWN, &FOOTPRINT_DIFF_WIDGET::onSlider, this );
   	m_slider->Bind( wxEVT_SCROLL_THUMBTRACK, &FOOTPRINT_DIFF_WIDGET::onSlider, this );
   	m_slider->Bind( wxEVT_SCROLL_THUMBRELEASE, &FOOTPRINT_DIFF_WIDGET::onSlider, this );
   	m_slider->Bind( wxEVT_SCROLL_CHANGED, &FOOTPRINT_DIFF_WIDGET::onSlider, this );
}


void FOOTPRINT_DIFF_WIDGET::DisplayDiff( FOOTPRINT* aBoardFootprint,
                                         std::shared_ptr<FOOTPRINT>& aLibFootprint )
{
    m_boardItemCopy.reset( static_cast<FOOTPRINT*>( aBoardFootprint->Clone() ) );
    m_boardItemCopy->ClearSelected();
    m_boardItemCopy->ClearBrightened();

    m_boardItemCopy->RunOnChildren(
            [&]( BOARD_ITEM* child )
            {
                child->ClearSelected();
                child->ClearBrightened();
            } );

    m_boardItemCopy->Move( -m_boardItemCopy->GetPosition() );

    if( m_boardItemCopy->IsFlipped() )
        m_boardItemCopy->Flip( {0,0}, false );

    if( m_boardItemCopy->GetOrientation() != ANGLE_0 )
        m_boardItemCopy->Rotate( {0,0}, -m_boardItemCopy->GetOrientation() );

    m_libraryItem = aLibFootprint;

    DisplayFootprints( m_boardItemCopy, m_libraryItem );

    wxScrollEvent dummy;
    onSlider( dummy );
}


void FOOTPRINT_DIFF_WIDGET::onSlider( wxScrollEvent& aEvent )
{
    double pct = (double) m_slider->GetValue() / 100.0;

    if( m_boardItemCopy )
    {
        double val;

        if( pct < 0.5  )
            val = 0.0;
        else
            val = ( pct - 0.5 ) * 2;

        m_boardItemCopy->SetForcedTransparency( val );

        m_boardItemCopy->RunOnChildren(
                [&]( BOARD_ITEM* child )
                {
                    child->SetForcedTransparency( val );
                } );
    }

    if( m_libraryItem )
    {
        double val;

        if( pct > 0.5  )
            val = 0.0;
        else
            val = 1.0 - ( pct * 2 );

        m_libraryItem->SetForcedTransparency( val );

        m_libraryItem->RunOnChildren(
                [&]( BOARD_ITEM* child )
                {
                    child->SetForcedTransparency( val );
                } );
    }

    RefreshAll();
    aEvent.Skip();
}