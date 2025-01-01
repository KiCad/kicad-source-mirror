/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 1992-2011 jean-pierre.charras
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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <bitmaps.h>
#include <calculator_panels/panel_color_code.h>
#include <pcb_calculator_settings.h>


PANEL_COLOR_CODE::PANEL_COLOR_CODE( wxWindow* parent, wxWindowID id, const wxPoint& pos,
                                    const wxSize& size, long style, const wxString& name ) :
        PANEL_COLOR_CODE_BASE( parent, id, pos, size, style, name )
{
    ToleranceSelection( m_rbToleranceSelection->GetSelection() );

    // Needed on wxWidgets 3.0 to ensure sizers are correctly set
    GetSizer()->SetSizeHints( this );
}


PANEL_COLOR_CODE::~PANEL_COLOR_CODE()
{
}


void PANEL_COLOR_CODE::ThemeChanged()
{
    // Update the bitmaps
    m_Band1bitmap->SetBitmap( KiBitmapBundle( BITMAPS::color_code_value_and_name ) );
    m_Band2bitmap->SetBitmap( KiBitmapBundle( BITMAPS::color_code_value ) );
    m_Band3bitmap->SetBitmap( KiBitmapBundle( BITMAPS::color_code_value ) );
    m_Band4bitmap->SetBitmap( KiBitmapBundle( BITMAPS::color_code_value ) );
    m_Band_mult_bitmap->SetBitmap( KiBitmapBundle( BITMAPS::color_code_multiplier ) );
    m_Band_tol_bitmap->SetBitmap( KiBitmapBundle( BITMAPS::color_code_tolerance ) );

    Refresh();
}


void PANEL_COLOR_CODE::LoadSettings( PCB_CALCULATOR_SETTINGS* aCfg )
{
    m_rbToleranceSelection->SetSelection( aCfg->m_ColorCodeTolerance );
    ToleranceSelection( m_rbToleranceSelection->GetSelection() );
}


void PANEL_COLOR_CODE::SaveSettings( PCB_CALCULATOR_SETTINGS* aCfg )
{
    aCfg->m_ColorCodeTolerance = m_rbToleranceSelection->GetSelection();
}


void PANEL_COLOR_CODE::OnToleranceSelection( wxCommandEvent& event )
{
    ToleranceSelection( event.GetSelection() );
}


void PANEL_COLOR_CODE::ToleranceSelection( int aSelection )
{
    /* For tolerance = 5 or 10 %, there are 3 bands for the value
     * but for tolerance < 5 %, there are 4 bands
     */
    bool show4thBand = aSelection != 0;

    m_Band4bitmap->Show(show4thBand);
    m_staticTextBand4->Show(show4thBand);

    // m_Band4Label visibility has changed:
    // The new size must be taken in account
    GetSizer()->Layout();

    // All this shouldn't be necessary but if you want the bitmaps to show up on OSX it is.
    m_Band1bitmap->SetBitmap( KiBitmapBundle( BITMAPS::color_code_value_and_name ) );
    m_Band2bitmap->SetBitmap( KiBitmapBundle( BITMAPS::color_code_value ) );
    m_Band3bitmap->SetBitmap( KiBitmapBundle( BITMAPS::color_code_value ) );
    m_Band4bitmap->SetBitmap( KiBitmapBundle( BITMAPS::color_code_value ) );
    m_Band_mult_bitmap->SetBitmap( KiBitmapBundle( BITMAPS::color_code_multiplier ) );
    m_Band_tol_bitmap->SetBitmap( KiBitmapBundle( BITMAPS::color_code_tolerance ) );

    Refresh();
}
