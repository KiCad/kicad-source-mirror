/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Seth Hillbrand <hillbrand@ucdavis.edu>
 * Copyright (C) 2015 KiCad Developers, see CHANGELOG.TXT for contributors.
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

#include <cassert>
#include <dialog_edit_line_style.h>


DIALOG_EDIT_LINE_STYLE::DIALOG_EDIT_LINE_STYLE( wxWindow* parent ) :
    DIALOG_EDIT_LINE_STYLE_BASE( parent )
{
    m_sdbSizer1Apply->SetLabel( _( "Default" ) );

    m_lineStyle->SetSelection( 0 );

    m_lineWidth->SetFocus();
    defaultStyle = 0;
    defaultWidth = "";
    m_sdbSizer1OK->SetDefault();

    // Now all widgets have the size fixed, call FinishDialogSettings
    FinishDialogSettings();

    // On some windows manager (Unity, XFCE), this dialog is
    // not always raised, depending on this dialog is run.
    // Force it to be raised
    Raise();
}


void DIALOG_EDIT_LINE_STYLE::resetDefaults( wxCommandEvent& event )
{
    SetStyle( defaultStyle );
    SetWidth( defaultWidth );
    SetColor( defaultColor );
    Refresh();
}


void DIALOG_EDIT_LINE_STYLE::SetColor( const COLOR4D& aColor )
{
    assert( aColor.r >= 0.0 && aColor.r <= 1.0 );
    assert( aColor.g >= 0.0 && aColor.g <= 1.0 );
    assert( aColor.b >= 0.0 && aColor.b <= 1.0 );
    assert( aColor.a >= 0.0 && aColor.a <= 1.0 );

    m_colorPicker->SetColour( aColor.ToColour() );
}


void DIALOG_EDIT_LINE_STYLE::SetStyle( const int aStyle )
{
    assert( aStyle >= 0 && aStyle < 4 );

    m_lineStyle->SetSelection( aStyle );
}


int DIALOG_EDIT_LINE_STYLE::GetStyle()
{
    return m_lineStyle->GetSelection();
}
