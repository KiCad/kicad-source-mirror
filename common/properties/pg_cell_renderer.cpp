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
#include <wx/control.h>
#include <wx/dc.h>
#include <wx/propgrid/propgrid.h>
#include <properties/pg_cell_renderer.h>
#include <properties/pg_properties.h>
#include <widgets/color_swatch.h>


PG_CELL_RENDERER::PG_CELL_RENDERER() :
        wxPGDefaultRenderer()
{}


bool PG_CELL_RENDERER::Render( wxDC &aDC, const wxRect &aRect, const wxPropertyGrid *aGrid,
                               wxPGProperty *aProperty, int aColumn, int aItem, int aFlags ) const
{
    wxPGCell cell = aGrid->GetUnspecifiedValueAppearance();

    if( aColumn > 0 )
    {
        if( PGPROPERTY_COLOR4D* colorProp = dynamic_cast<PGPROPERTY_COLOR4D*>( aProperty ) )
        {
            wxAny av = colorProp->GetValue().GetAny();
            KIGFX::COLOR4D color = av.IsNull() ? KIGFX::COLOR4D::UNSPECIFIED
                                               : av.As<KIGFX::COLOR4D>();

            if( !color.m_text.has_value() )
            {
                wxSize swatchSize = aGrid->ConvertDialogToPixels( wxSize( 24, 16 ) );
                int offset = ( aRect.GetHeight() - swatchSize.GetHeight() ) / 2;
                wxRect swatch( aRect.GetPosition() + wxPoint( offset, offset ), swatchSize );

                aDC.Clear();    // Ensure the "old" background is erased.
                COLOR_SWATCH::RenderToDC( &aDC, color, colorProp->GetBackgroundColor(), swatch,
                                          aGrid->ConvertDialogToPixels( CHECKERBOARD_SIZE_DU ),
                                          aGrid->GetBackgroundColour() );

                return true;
            }
        }

        // Default behavior for value column
        return wxPGDefaultRenderer::Render( aDC, aRect, aGrid, aProperty, aColumn, aItem, aFlags );
    }

    wxString text;
    int      preDrawFlags = aFlags;

    aProperty->GetDisplayInfo( aColumn, aItem, aFlags, &text, &cell );

    text = wxControl::Ellipsize( text, aDC, wxELLIPSIZE_END, aRect.GetWidth() );

    int imageWidth = PreDrawCell( aDC, aRect, aGrid, cell, preDrawFlags );

    int imageOffset = aProperty->GetImageOffset( imageWidth );

    DrawEditorValue( aDC, aRect, imageOffset, text, aProperty, nullptr );

    PostDrawCell( aDC, aGrid, cell, preDrawFlags );

    return !text.IsEmpty();
}
