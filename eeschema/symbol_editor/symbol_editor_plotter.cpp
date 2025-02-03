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


#include <sch_painter.h>
#include <symbol_edit_frame.h>
#include <locale_io.h>
#include <plotters/plotters_pslike.h>
#include <sch_plotter.h>

void SYMBOL_EDIT_FRAME::SVGPlotSymbol( const wxString& aFullFileName, const VECTOR2I& aOffset )
{
    SCH_RENDER_SETTINGS renderSettings;
    renderSettings.LoadColors( GetColorSettings() );
    renderSettings.SetDefaultPenWidth( GetRenderSettings()->GetDefaultPenWidth() );

    const PAGE_INFO& pageInfo = GetScreen()->GetPageSettings();

    SVG_PLOTTER* plotter = new SVG_PLOTTER();
    plotter->SetRenderSettings( &renderSettings );
    plotter->SetPageSettings( pageInfo );
    plotter->SetColorMode( true );

    VECTOR2I plot_offset;
    const double scale = 1.0;

    // Currently, plot units are in decimil
    plotter->SetViewport( plot_offset, schIUScale.IU_PER_MILS/10, scale, false );

    // Init :
    plotter->SetCreator( wxT( "Eeschema-SVG" ) );

    if( !plotter->OpenFile( aFullFileName ) )
    {
        delete plotter;
        return;
    }

    LOCALE_IO     toggle;
    SCH_PLOT_OPTS plotOpts;

    plotter->StartPlot( wxT( "1" ) );

    if( m_symbol )
    {
        constexpr bool background = true;

        m_symbol->Plot( plotter, background, plotOpts, GetUnit(), GetBodyStyle(), aOffset, false );
        m_symbol->Plot( plotter, !background, plotOpts, GetUnit(), GetBodyStyle(), aOffset, false );
        m_symbol->PlotFields( plotter, !background, plotOpts, GetUnit(), GetBodyStyle(), aOffset,
                              false );
    }

    plotter->EndPlot();
    delete plotter;
}
