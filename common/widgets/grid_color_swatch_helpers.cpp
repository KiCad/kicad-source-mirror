/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <widgets/grid_color_swatch_helpers.h>

#include <settings/color_settings.h>
#include <dialogs/dialog_color_picker.h>
#include <wx/checkbox.h>
#include <wx/dc.h>


//-------- Custom wxGridCellRenderers --------------------------------------------------


GRID_CELL_COLOR_RENDERER::GRID_CELL_COLOR_RENDERER( wxWindow* aParent, SWATCH_SIZE aSize,
                                                    const KIGFX::COLOR4D& aBackground ) :
        wxGridCellRenderer(),
        m_parent( aParent ),
        m_background( aBackground )
{
    switch( aSize )
    {
    case SWATCH_MEDIUM: m_size = m_parent->ConvertDialogToPixels( SWATCH_SIZE_MEDIUM_DU ); break;
    case SWATCH_SMALL:  m_size = m_parent->ConvertDialogToPixels( SWATCH_SIZE_SMALL_DU );  break;
    case SWATCH_LARGE:  m_size = m_parent->ConvertDialogToPixels( SWATCH_SIZE_LARGE_DU );  break;
    case SWATCH_EXPAND: m_size = wxDefaultSize; break;
    }

    m_checkerboardSize = m_parent->ConvertDialogToPixels( CHECKERBOARD_SIZE_DU );
    m_checkerboardBg   = m_parent->GetBackgroundColour();
}


GRID_CELL_COLOR_RENDERER::GRID_CELL_COLOR_RENDERER( const GRID_CELL_COLOR_RENDERER& aOther )
{
    m_parent           = aOther.m_parent;
    m_background       = aOther.m_background;
    m_size             = aOther.m_size;
    m_checkerboardSize = aOther.m_checkerboardSize;
    m_checkerboardBg   = aOther.m_checkerboardBg;
}


GRID_CELL_COLOR_RENDERER::~GRID_CELL_COLOR_RENDERER()
{
}


wxGridCellRenderer* GRID_CELL_COLOR_RENDERER::Clone() const
{
    return new GRID_CELL_COLOR_RENDERER( *this );
}


wxSize GRID_CELL_COLOR_RENDERER::GetBestSize( wxGrid& grid, wxGridCellAttr& attr, wxDC& dc,
                                              int row, int col )
{
    if( m_size != wxDefaultSize )
        return m_size;

    wxSize bestSize;

    dc.SetFont( attr.GetFont() );
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
    COLOR4D color( aGrid.GetTable()->GetValue( aRow, aCol ) );
    wxSize size = ( m_size == wxDefaultSize ) ? aRect.GetSize() : m_size;
    wxBitmap bitmap = COLOR_SWATCH::MakeBitmap( color, m_background, size, m_checkerboardSize,
                                                m_checkerboardBg );

    wxPoint origin = rect.GetTopLeft();

    if( m_size != wxDefaultSize )
    {
        int x = std::max( 0, ( aRect.GetWidth() - m_size.x ) / 2 );
        int y = std::max( 0, ( aRect.GetHeight() - m_size.y ) / 2 );
        origin += wxPoint( x, y );
    }

    aDC.DrawBitmap( bitmap, origin, true );
}


void GRID_CELL_COLOR_RENDERER::OnDarkModeToggle()
{
    m_checkerboardBg = m_parent ? m_parent->GetBackgroundColour() : wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW );
}


//-------- Custom wxGridCellEditors ----------------------------------------------------
//
// Note: this implementation is an adaptation of wxGridCellBoolEditor


GRID_CELL_COLOR_SELECTOR::GRID_CELL_COLOR_SELECTOR( wxWindow* aParent, wxGrid* aGrid ) :
        m_parent( aParent ),
        m_grid( aGrid ),
        m_value( COLOR4D::UNSPECIFIED )
{
}


wxGridCellEditor* GRID_CELL_COLOR_SELECTOR::Clone() const
{
    return new GRID_CELL_COLOR_SELECTOR( m_parent, m_grid );
}


void GRID_CELL_COLOR_SELECTOR::Create( wxWindow* aParent, wxWindowID aId,
                                       wxEvtHandler* aEventHandler )
{
    // wxWidgets needs a control to hold on to the event handler
    m_control = new wxTextCtrl( aParent, wxID_ANY, wxEmptyString );

    wxGridCellEditor::Create( aParent, aId, aEventHandler );
}


wxString GRID_CELL_COLOR_SELECTOR::GetValue() const
{
    return m_value.ToCSSString();
}


void GRID_CELL_COLOR_SELECTOR::BeginEdit( int row, int col, wxGrid* grid )
{
    m_value.SetFromWxString( grid->GetTable()->GetValue( row, col ) );

    grid->CallAfter(
            [this, row, col]()
            {
                DIALOG_COLOR_PICKER dialog( m_parent, m_value, false );

                if( dialog.ShowModal() == wxID_OK )
                    m_value = dialog.GetColor();

                m_grid->GetTable()->SetValue( row, col, GetValue() );
                m_grid->ForceRefresh();

                // Let any clients know
                wxGridEvent event( m_grid->GetId(), wxEVT_GRID_CELL_CHANGED, m_grid, row, col );
                event.SetString( GetValue() );
                m_grid->GetEventHandler()->ProcessEvent( event );
            } );

    // That's it; we're all done
    m_grid->HideCellEditControl();
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


