/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include "widgets/symbol_diff_widget.h"

#include <wx/bmpbuttn.h>
#include <wx/sizer.h>
#include <wx/slider.h>
#include <wx/stattext.h>

#include <bitmaps.h>
#include <hotkeys_basic.h>
#include <lib_symbol.h>
#include <sch_painter.h>
#include <eeschema_settings.h>
#include <settings/settings_manager.h>
#include <sch_view.h>


SYMBOL_DIFF_WIDGET::SYMBOL_DIFF_WIDGET( wxWindow* aParent,
                                        EDA_DRAW_PANEL_GAL::GAL_TYPE aCanvasType ) :
        SYMBOL_PREVIEW_WIDGET( aParent, nullptr, false, aCanvasType ),
        m_libraryItem( nullptr ),
        m_slider( nullptr )
{
    wxBoxSizer* bottomSizer = new wxBoxSizer( wxHORIZONTAL );

    wxStaticText* schLabel = new wxStaticText( this, wxID_ANY, _( "Schematic" ) );
    wxStaticText* libLabel = new wxStaticText( this, wxID_ANY, _( "Library" ) );
    m_slider = new wxSlider( this, wxID_ANY, 50, 0, 100 );

    bottomSizer->Add( schLabel, 0, wxLEFT | wxRIGHT | wxBOTTOM | wxALIGN_CENTRE_VERTICAL, 6 );
    bottomSizer->Add( m_slider, 1, wxLEFT | wxRIGHT | wxALIGN_BOTTOM, 30 );
    bottomSizer->Add( libLabel, 0, wxLEFT | wxRIGHT | wxBOTTOM | wxALIGN_CENTRE_VERTICAL, 6 );

    m_toggleButton = new wxBitmapButton( this, wxID_ANY, KiBitmapBundle( BITMAPS::swap ) );
    wxString toggleTooltip = _( "Toggle between A and B display" );
    toggleTooltip = AddHotkeyName( toggleTooltip, '/', HOTKEY_ACTION_TYPE::IS_COMMENT );
    m_toggleButton->SetToolTip( toggleTooltip );

    bottomSizer->Add( m_toggleButton, 0, wxLEFT | wxRIGHT | wxALIGN_CENTRE_VERTICAL, 6 );

    m_outerSizer->Add( bottomSizer, 0, wxTOP | wxLEFT | wxRIGHT | wxEXPAND, 10 );

    Layout();

    m_slider->Bind( wxEVT_SCROLL_TOP, &SYMBOL_DIFF_WIDGET::onSlider, this );
   	m_slider->Bind( wxEVT_SCROLL_BOTTOM, &SYMBOL_DIFF_WIDGET::onSlider, this );
   	m_slider->Bind( wxEVT_SCROLL_LINEUP, &SYMBOL_DIFF_WIDGET::onSlider, this );
   	m_slider->Bind( wxEVT_SCROLL_LINEDOWN, &SYMBOL_DIFF_WIDGET::onSlider, this );
   	m_slider->Bind( wxEVT_SCROLL_PAGEUP, &SYMBOL_DIFF_WIDGET::onSlider, this );
   	m_slider->Bind( wxEVT_SCROLL_PAGEDOWN, &SYMBOL_DIFF_WIDGET::onSlider, this );
   	m_slider->Bind( wxEVT_SCROLL_THUMBTRACK, &SYMBOL_DIFF_WIDGET::onSlider, this );
   	m_slider->Bind( wxEVT_SCROLL_THUMBRELEASE, &SYMBOL_DIFF_WIDGET::onSlider, this );
   	m_slider->Bind( wxEVT_SCROLL_CHANGED, &SYMBOL_DIFF_WIDGET::onSlider, this );

    Bind( wxEVT_CHAR_HOOK, &SYMBOL_DIFF_WIDGET::onCharHook, this );

    m_toggleButton->Bind( wxEVT_BUTTON,
            [this]( wxCommandEvent& aEvent )
            {
                ToggleAB();
            } );
}


SYMBOL_DIFF_WIDGET::~SYMBOL_DIFF_WIDGET()
{
    delete m_libraryItem;
}


void SYMBOL_DIFF_WIDGET::DisplayDiff( LIB_SYMBOL* aSchSymbol, LIB_SYMBOL* aLibSymbol, int aUnit,
                                      int aBodyStyle )
{
    KIGFX::VIEW* view = m_preview->GetView();

    if( m_previewItem )
    {
        view->Remove( m_previewItem );
        delete m_previewItem;
        m_previewItem = nullptr;

        wxASSERT( m_libraryItem );

        view->Remove( m_libraryItem );
        delete m_libraryItem;
        m_libraryItem = nullptr;
    }

    if( aSchSymbol )
    {
        m_previewItem = aSchSymbol;

        // For symbols having a De Morgan body style, use the first style
        auto settings = static_cast<SCH_RENDER_SETTINGS*>( view->GetPainter()->GetSettings() );

        settings->m_ShowUnit = ( m_previewItem->IsMultiUnit() && !aUnit ) ? 1 : aUnit;
        settings->m_ShowBodyStyle = ( m_previewItem->IsMultiBodyStyle() && !aBodyStyle ) ? 1 : aBodyStyle;

        view->Add( m_previewItem );

        // Get the symbol size, in internal units
        m_itemBBox = m_previewItem->GetUnitBoundingBox( settings->m_ShowUnit, settings->m_ShowBodyStyle );

        // Calculate the draw scale to fit the drawing area
        fitOnDrawArea();

        wxASSERT( aLibSymbol );

        m_libraryItem = aLibSymbol;
        view->Add( m_libraryItem );
    }

    wxScrollEvent dummy;
    onSlider( dummy );

    m_preview->Show();
    Layout();
}


void SYMBOL_DIFF_WIDGET::ToggleAB()
{
    const int val = m_slider->GetValue();

    if( val == 0 )
        m_slider->SetValue( 100 );
    else
        m_slider->SetValue( 0 );

    wxScrollEvent dummy;
    onSlider( dummy );
}


void SYMBOL_DIFF_WIDGET::onSlider( wxScrollEvent& aEvent )
{
    KIGFX::VIEW* view = m_preview->GetView();
    double       pct = (double) m_slider->GetValue() / 100.0;

    if( m_previewItem )
    {
        double val;

        if( pct < 0.5  )
            val = 0.0;
        else
            val = ( pct - 0.5 ) * 2;

        m_previewItem->SetForcedTransparency( val );
        view->Update( m_previewItem );

        for( SCH_ITEM& child : m_previewItem->GetDrawItems() )
        {
            child.SetForcedTransparency( val );
            view->Update( &child );
        }
    }

    if( m_libraryItem )
    {
        double val;

        if( pct > 0.5  )
            val = 0.0;
        else
            val = 1.0 - ( pct * 2 );

        m_libraryItem->SetForcedTransparency( val );
        view->Update( m_libraryItem );

        for( SCH_ITEM& child : m_libraryItem->GetDrawItems() )
        {
            child.SetForcedTransparency( val );
            view->Update( &child );
        }
    }

    m_preview->ForceRefresh();

    aEvent.Skip();
}


void SYMBOL_DIFF_WIDGET::onCharHook( wxKeyEvent& aEvent )
{
    if( aEvent.GetKeyCode() == '/' )
    {
        ToggleAB();
    }
    else
    {
        aEvent.Skip();
    }
}