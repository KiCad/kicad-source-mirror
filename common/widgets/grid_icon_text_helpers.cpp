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

#include <widgets/grid_icon_text_helpers.h>

#include <wx/textctrl.h>
#include <wx/dc.h>

#include <bitmaps.h>


//---- Grid helpers: custom wxGridCellRenderer that renders icon and a label ------------


GRID_CELL_ICON_TEXT_RENDERER::GRID_CELL_ICON_TEXT_RENDERER( const std::vector<BITMAPS>& icons,
                                                            const wxArrayString& names ) :
    m_icons( icons ),
    m_names( names )
{
}

void GRID_CELL_ICON_TEXT_RENDERER::Draw( wxGrid& aGrid, wxGridCellAttr& aAttr, wxDC& aDC,
                                         const wxRect& aRect, int aRow, int aCol, bool isSelected )
{
    wxString value  = aGrid.GetCellValue( aRow, aCol );
    wxBitmap bitmap;

    wxRect rect = aRect;
    rect.Inflate( -1 );

    // erase background
    wxGridCellRenderer::Draw( aGrid, aAttr, aDC, aRect, aRow, aCol, isSelected );

    // draw the icon
    // note that the set of icons might be smaller than the set of labels if the last
    // label is <...>.
    auto position = m_names.Index( value );
    if( position < (int) m_icons.size() && position != wxNOT_FOUND )
    {
        bitmap = KiBitmap( m_icons[ position ] );
        aDC.DrawBitmap( bitmap, rect.GetLeft() + 3, rect.GetTop() + 2, true );
    }
    // still need a bitmap to fetch the width
    else
        bitmap = KiBitmap( m_icons[ 0 ] );

    // draw the text
    rect.SetLeft( rect.GetLeft() + bitmap.GetWidth() + 7 );
    SetTextColoursAndFont( aGrid, aAttr, aDC, isSelected );
    aGrid.DrawTextRectangle( aDC, value, rect, wxALIGN_LEFT, wxALIGN_CENTRE );
}

wxSize GRID_CELL_ICON_TEXT_RENDERER::GetBestSize( wxGrid& grid, wxGridCellAttr& attr, wxDC& dc,
                                                  int row, int col )
{
    wxBitmap bitmap = KiBitmap( m_icons[ row ] );
    wxString text = grid.GetCellValue( row, col );
    wxSize   size = wxGridCellStringRenderer::DoGetBestSize( attr, dc, text );

    size.x += bitmap.GetWidth() + 6;

    return size;
}


//---- Grid helpers: custom wxGridCellRenderer that renders just an icon ----------------
//
// Note: this renderer is supposed to be used with read only cells

GRID_CELL_ICON_RENDERER::GRID_CELL_ICON_RENDERER(const wxBitmap& icon) : m_icon( icon )
{
}


void GRID_CELL_ICON_RENDERER::Draw( wxGrid& aGrid, wxGridCellAttr& aAttr, wxDC& aDC,
                                    const wxRect& aRect, int aRow, int aCol, bool isSelected )
{
    wxRect rect = aRect;
    rect.Inflate( -1 );

    // erase background
    wxGridCellRenderer::Draw( aGrid, aAttr, aDC, aRect, aRow, aCol, isSelected );

    // Draw icon
    if( m_icon.IsOk() )
    {
        aDC.DrawBitmap( m_icon,
                        rect.GetLeft() + ( rect.GetWidth() - m_icon.GetWidth() ) / 2,
                        rect.GetTop() + ( rect.GetHeight() - m_icon.GetHeight() ) / 2,
                        true );
    }
}


wxSize GRID_CELL_ICON_RENDERER::GetBestSize( wxGrid& grid, wxGridCellAttr& attr, wxDC& dc, int row, int col )
{
    return wxSize( m_icon.GetWidth() + 6, m_icon.GetHeight() + 4 );
}


wxGridCellRenderer* GRID_CELL_ICON_RENDERER::Clone() const
{
    return new GRID_CELL_ICON_RENDERER( m_icon );
}


//---- Grid helpers: custom wxGridCellEditor ------------------------------------------
//
// Note: this implementation is an adaptation of wxGridCellChoiceEditor


GRID_CELL_ICON_TEXT_POPUP::GRID_CELL_ICON_TEXT_POPUP( const std::vector<BITMAPS>& icons,
                                                      const wxArrayString& names ) :
    m_icons( icons ),
    m_names( names )
{
}


wxGridCellEditor* GRID_CELL_ICON_TEXT_POPUP::Clone() const
{
    return new GRID_CELL_ICON_TEXT_POPUP( m_icons, m_names );
}


void GRID_CELL_ICON_TEXT_POPUP::Create( wxWindow* aParent, wxWindowID aId,
                                        wxEvtHandler* aEventHandler )
{
    m_control = new wxBitmapComboBox(
                    aParent, aId, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL,
                    wxCB_READONLY | wxTE_PROCESS_ENTER | wxTE_PROCESS_TAB | wxBORDER_NONE );

    for( unsigned i = 0; i < m_names.size(); ++i )
    {
        // note that the set of icons might be smaller than the set of labels if
        // the last label is <...>.
        if( i < m_icons.size() )
            Combo()->Append( m_names[ i ], KiBitmap( m_icons[ i ] ) );
        else
            Combo()->Append( m_names[ i ] );
    }

    wxGridCellEditor::Create(aParent, aId, aEventHandler);
}

wxString GRID_CELL_ICON_TEXT_POPUP::GetValue() const
{
    return Combo()->GetValue();
}

void GRID_CELL_ICON_TEXT_POPUP::SetSize( const wxRect& aRect )
{
    wxRect rect( aRect );
    rect.Inflate( -1 );

#if !defined( __WXMSW__ ) && !defined( __WXGTK20__ )
    // Only implemented in generic wxBitmapComboBox; MSW and GTK use native controls
    Combo()->SetButtonPosition( 0, 0, wxRIGHT, 2 );
#endif

#if defined( __WXMAC__ )
    rect.Inflate( 3 );      // no FOCUS_RING, even on Mac
#endif

    Combo()->SetSize( rect, wxSIZE_ALLOW_MINUS_ONE );
}


void GRID_CELL_ICON_TEXT_POPUP::BeginEdit( int aRow, int aCol, wxGrid* aGrid )
{
    auto evtHandler = static_cast<wxGridCellEditorEvtHandler*>( m_control->GetEventHandler() );

    // Don't immediately end if we get a kill focus event within BeginEdit
    evtHandler->SetInSetFocus( true );

    m_value = aGrid->GetTable()->GetValue( aRow, aCol );

    Combo()->SetSelection( Combo()->FindString( m_value ) );
    Combo()->SetFocus();

#ifdef __WXOSX_COCOA__
    // This is a work around for the combobox being simply dismissed when a
    // choice is made in it under OS X. The bug is almost certainly due to a
    // problem in focus events generation logic but it's not obvious to fix and
    // for now this at least allows to use wxGrid.
    Combo()->Popup();
#endif

    // When dropping down the menu, a kill focus event
    // happens after this point, so we can't reset the flag yet.
#if !defined(__WXGTK20__)
    evtHandler->SetInSetFocus( false );
#endif
}


bool GRID_CELL_ICON_TEXT_POPUP::EndEdit( int , int , const wxGrid* , const wxString& ,
                                         wxString *aNewVal )
{
    const wxString value = Combo()->GetValue();

    if( value == m_value )
        return false;

    m_value = value;

    if( aNewVal )
        *aNewVal = value;

    return true;
}


void GRID_CELL_ICON_TEXT_POPUP::ApplyEdit( int aRow, int aCol, wxGrid* aGrid )
{
    aGrid->GetTable()->SetValue( aRow, aCol, m_value );
}


void GRID_CELL_ICON_TEXT_POPUP::Reset()
{
    Combo()->SetSelection( Combo()->FindString( m_value ) );
}


