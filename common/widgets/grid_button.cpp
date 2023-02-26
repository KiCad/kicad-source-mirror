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

#include <widgets/grid_button.h>
#include <wx/dc.h>
#include <wx/renderer.h>


GRID_BUTTON_RENDERER::GRID_BUTTON_RENDERER( const wxString& aLabel ) :
        wxGridCellRenderer(),
        m_button( nullptr, wxID_ANY, aLabel )
{
}


GRID_BUTTON_RENDERER* GRID_BUTTON_RENDERER::Clone() const
{
    GRID_BUTTON_RENDERER* clone = new GRID_BUTTON_RENDERER( m_button.GetLabel() );
    return clone;
}


void GRID_BUTTON_RENDERER::Draw( wxGrid& aGrid, wxGridCellAttr& aAttr, wxDC& aDc,
                                        const wxRect& aRect, int aRow, int aCol, bool aIsSelected )
{
    // erase background
    wxGridCellRenderer::Draw( aGrid, aAttr, aDc, aRect, aRow, aCol, aIsSelected );

    wxRendererNative::Get().DrawPushButton( &aGrid, aDc, aRect );
}


wxSize GRID_BUTTON_RENDERER::GetBestSize( wxGrid& aGrid, wxGridCellAttr& aAttr, wxDC& aDc,
                                                 int aRow, int aCol)
{
    return m_button.GetSize();
}
