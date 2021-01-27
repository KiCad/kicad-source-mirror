/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2019 KiCad Developers, see AUTHORS.txt for contributors.
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


#include <sch_painter.h>
#include <symbol_edit_frame.h>
#include <locale_io.h>
#include <plotters_specific.h>

void SYMBOL_EDIT_FRAME::SVGPlotSymbol( const wxString& aFullFileName )
{
    KIGFX::SCH_RENDER_SETTINGS renderSettings;
    renderSettings.LoadColors( GetColorSettings() );
    renderSettings.SetDefaultPenWidth( GetRenderSettings()->GetDefaultPenWidth() );

    const PAGE_INFO& pageInfo = GetScreen()->GetPageSettings();

    SVG_PLOTTER* plotter = new SVG_PLOTTER();
    plotter->SetRenderSettings( &renderSettings );
    plotter->SetPageSettings( pageInfo );
    plotter->SetColorMode( true );

    wxPoint plot_offset;
    const double scale = 1.0;

    // Currently, plot units are in decimil
    plotter->SetViewport( plot_offset, IU_PER_MILS/10, scale, false );

    // Init :
    plotter->SetCreator( wxT( "Eeschema-SVG" ) );

    if( ! plotter->OpenFile( aFullFileName ) )
    {
        delete plotter;
        return;
    }

    LOCALE_IO   toggle;

    plotter->StartPlot();

    if( m_my_part )
    {
        TRANSFORM   temp;     // Uses default transform
        wxPoint     plotPos;

        plotPos.x = pageInfo.GetWidthIU() /2;
        plotPos.y = pageInfo.GetHeightIU()/2;

        m_my_part->Plot( plotter, GetUnit(), GetConvert(), plotPos, temp );

        // Plot lib fields, not plotted by m_my_part->Plot():
        m_my_part->PlotLibFields( plotter, GetUnit(), GetConvert(), plotPos, temp );
    }

    plotter->EndPlot();
    delete plotter;
}


void SYMBOL_EDIT_FRAME::PrintPage( const RENDER_SETTINGS* aSettings )
{
    if( !m_my_part )
        return;

    wxSize pagesize = GetScreen()->GetPageSettings().GetSizeIU();

    /* Plot item centered to the page
     * In symbol_editor, the component is centered at 0,0 coordinates.
     * So we must plot it with an offset = pagesize/2.
     */
    wxPoint plot_offset;
    plot_offset.x = pagesize.x / 2;
    plot_offset.y = pagesize.y / 2;

    m_my_part->Print( aSettings, plot_offset, m_unit, m_convert, PART_DRAW_OPTIONS() );
}
