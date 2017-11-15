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
#include <widgets/color4Dpickerdlg.h>
#include <dialog_edit_line_style.h>

const int BUTT_COLOR_MINSIZE_X = 32;
const int BUTT_COLOR_MINSIZE_Y = 20;

DIALOG_EDIT_LINE_STYLE::DIALOG_EDIT_LINE_STYLE( wxWindow* parent ) :
    DIALOG_EDIT_LINE_STYLE_BASE( parent )
{
    m_sdbSizerApply->SetLabel( _( "Default" ) );
    m_lineStyle->SetSelection( 0 );
    m_lineWidth->SetFocus();

    m_defaultStyle = 0;

    wxBitmap bitmap( std::max( m_colorButton->GetSize().x, BUTT_COLOR_MINSIZE_X ),
                     std::max( m_colorButton->GetSize().y, BUTT_COLOR_MINSIZE_Y ) );
    m_colorButton->SetBitmap( bitmap );

    m_sdbSizerOK->SetDefault();

    // Now all widgets have the size fixed, call FinishDialogSettings
    FinishDialogSettings();
}

void DIALOG_EDIT_LINE_STYLE::onColorButtonClicked( wxCommandEvent& event )
{
    COLOR4D newColor = COLOR4D::UNSPECIFIED;
    COLOR4D_PICKER_DLG dialog( this, m_selectedColor, false );

    if( dialog.ShowModal() == wxID_OK )
        newColor = dialog.GetColor();

    if( newColor == COLOR4D::UNSPECIFIED || m_selectedColor == newColor )
        return;

    SetColor( newColor, true );

}

void DIALOG_EDIT_LINE_STYLE::updateColorButton( COLOR4D& aColor )
{
    wxMemoryDC iconDC;

    wxBitmap bitmap = m_colorButton->GetBitmapLabel();
    iconDC.SelectObject( bitmap );
    iconDC.SetPen( *wxBLACK_PEN );

    wxBrush  brush( aColor.ToColour() );
    iconDC.SetBrush( brush );

    // Paint the full bitmap in aColor:
    iconDC.SetBackground( brush );
    iconDC.Clear();

    m_colorButton->SetBitmapLabel( bitmap );
    m_colorButton->Refresh();

    Refresh( false );
}


void DIALOG_EDIT_LINE_STYLE::resetDefaults( wxCommandEvent& event )
{
    SetStyle( m_defaultStyle );
    SetWidth( m_defaultWidth );
    SetColor( m_defaultColor, true );
    Refresh();
}


void DIALOG_EDIT_LINE_STYLE::SetColor( const COLOR4D& aColor, bool aRefresh )
{
    m_selectedColor = aColor;

    if( aRefresh )
        updateColorButton( m_selectedColor );
}


void DIALOG_EDIT_LINE_STYLE::SetDefaultColor( const COLOR4D& aColor )
{
    m_defaultColor = aColor;
}


void DIALOG_EDIT_LINE_STYLE::SetStyle( const int aStyle )
{
    wxASSERT( aStyle >= 0 && aStyle < 4 );

    m_lineStyle->SetSelection( aStyle );
}


int DIALOG_EDIT_LINE_STYLE::GetStyle()
{
    return m_lineStyle->GetSelection();
}
