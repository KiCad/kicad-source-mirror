/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <wx/control.h>
#include <wx/propgrid/propgrid.h>
#include <properties/pg_cell_renderer.h>


PG_CELL_RENDERER::PG_CELL_RENDERER() :
        wxPGDefaultRenderer()
{}


bool PG_CELL_RENDERER::Render( wxDC &aDC, const wxRect &aRect, const wxPropertyGrid *aGrid,
                               wxPGProperty *aProperty, int aColumn, int aItem, int aFlags ) const
{
    // Default behavior for value column
    if( aColumn > 0 )
        return wxPGDefaultRenderer::Render( aDC, aRect, aGrid, aProperty, aColumn, aItem, aFlags );

    wxPGCell cell = aGrid->GetUnspecifiedValueAppearance();

    wxString text;
    int      preDrawFlags = aFlags;

    aProperty->GetDisplayInfo( aColumn, aItem, aFlags, &text, &cell );

    text = wxControl::Ellipsize( text, aDC, wxELLIPSIZE_MIDDLE, aRect.GetWidth() );

    int imageWidth  = PreDrawCell( aDC, aRect, aGrid, cell, preDrawFlags );
    int imageOffset = aProperty->GetImageOffset( imageWidth );

    DrawEditorValue( aDC, aRect, imageOffset, text, aProperty, nullptr );

    PostDrawCell( aDC, aGrid, cell, preDrawFlags );

    return !text.IsEmpty();
}