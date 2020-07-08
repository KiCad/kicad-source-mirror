/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <grid_color_swatch_helpers.h>

#include <settings/color_settings.h>
#include <dialogs/dialog_color_picker.h>
#include <dialog_shim.h>


//-------- Custom wxGridCellRenderers --------------------------------------------------


GRID_CELL_COLOR_RENDERER::GRID_CELL_COLOR_RENDERER()
{
}


GRID_CELL_COLOR_RENDERER::~GRID_CELL_COLOR_RENDERER()
{
}


wxGridCellRenderer* GRID_CELL_COLOR_RENDERER::Clone() const
{
    return new GRID_CELL_COLOR_RENDERER;
}


wxSize GRID_CELL_COLOR_RENDERER::GetBestSize( wxGrid& grid, wxGridCellAttr& attr, wxDC& dc,
                                              int row, int col )
{
    wxSize bestSize;

    dc.SetFont(attr.GetFont());
    dc.GetTextExtent( "WWW", &bestSize.x, &bestSize.y );

    return bestSize;
}


void GRID_CELL_COLOR_RENDERER::Draw( wxGrid& aGrid, wxGridCellAttr& aAttr, wxDC& aDC,
                                     const wxRect& aRect, int aRow, int aCol, bool isSelected )
{
    wxRect rect = aRect;

    // erase background
    wxGridCellRenderer::Draw( aGrid, aAttr, aDC, aRect, aRow, aCol, isSelected );

    // draw the swatch
    wxBitmap   bitmap( aRect.GetWidth() + 1, aRect.GetHeight() + 1 );
    wxMemoryDC bmpDC;
    wxBrush    brush;
    wxColour   color;

    // Prepare Bitmap
    bmpDC.SelectObject( bitmap );

    color.Set( aGrid.GetTable()->GetValue( aRow, aCol ) );
    brush.SetStyle( wxBRUSHSTYLE_SOLID );
    brush.SetColour( color );
    bmpDC.SetBrush( brush );
    bmpDC.DrawRectangle( -1, -1, bitmap.GetWidth()+1, bitmap.GetHeight()+1 );

    aDC.DrawBitmap( bitmap, rect.GetTopLeft(), true );
}



//-------- Custom wxGridCellEditors ----------------------------------------------------
//
// Note: this implementation is an adaptation of wxGridCellBoolEditor


GRID_CELL_COLOR_SELECTOR::GRID_CELL_COLOR_SELECTOR( DIALOG_SHIM* aDialog, wxGrid* aGrid ) :
        m_dialog( aDialog ),
        m_grid( aGrid ),
        m_value( COLOR4D::UNSPECIFIED )
{
}


wxGridCellEditor* GRID_CELL_COLOR_SELECTOR::Clone() const
{
    return new GRID_CELL_COLOR_SELECTOR( m_dialog, m_grid );
}


void GRID_CELL_COLOR_SELECTOR::Create( wxWindow* aParent, wxWindowID aId,
                                       wxEvtHandler* aEventHandler )
{
    // wxWidgets needs a control to hold on to the event handler
    m_control = new wxCheckBox( aParent, wxID_ANY, wxEmptyString );

    wxGridCellEditor::Create( aParent, aId, aEventHandler );
}


wxString GRID_CELL_COLOR_SELECTOR::GetValue() const
{
    return m_value.ToWxString( wxC2S_CSS_SYNTAX );
}


void GRID_CELL_COLOR_SELECTOR::BeginEdit( int row, int col, wxGrid* grid )
{
    m_value.SetFromWxString( grid->GetTable()->GetValue( row, col ) );

    DIALOG_COLOR_PICKER dialog( m_dialog, m_value, false );

    if( dialog.ShowModal() == wxID_OK )
        m_value = dialog.GetColor();

    m_grid->GetTable()->SetValue( row, col, GetValue() );

    // That's it; we're all done
    m_grid->HideCellEditControl();
    m_grid->ForceRefresh();
}


bool GRID_CELL_COLOR_SELECTOR::EndEdit( int row, int col, const wxGrid* grid,
                                        const wxString& oldval, wxString *newval )
{
    if ( newval )
        *newval = GetValue();

    return true;
}


void GRID_CELL_COLOR_SELECTOR::ApplyEdit( int aRow, int aCol, wxGrid* aGrid )
{
    aGrid->GetTable()->SetValue( aRow, aCol, GetValue() );
}


void GRID_CELL_COLOR_SELECTOR::Reset()
{
}


