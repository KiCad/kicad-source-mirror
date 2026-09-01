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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */


#include <sch_painter.h>
#include <symbol_edit_frame.h>
#include <sch_plotter.h>

void SYMBOL_EDIT_FRAME::SVGPlotSymbol( const wxString& aFullFileName )
{
    if( !m_symbol )
        return;

    SCH_RENDER_SETTINGS renderSettings;
    renderSettings.LoadColors( GetColorSettings() );
    renderSettings.SetDefaultPenWidth( GetRenderSettings()->GetDefaultPenWidth() );

    // The symbol is exported in color with its origin at the SVG origin; the page and viewBox
    // are sized to the symbol's bounding box, with hidden fields excluded from the box (they
    // are still plotted).
    BOX2I symbolBB = GetSymbolPlotBBox( *m_symbol, GetUnit(), GetBodyStyle(), false );

    PlotSymbolToSVG( *m_symbol, *m_symbol, GetUnit(), GetBodyStyle(), symbolBB, renderSettings, false, aFullFileName );
}
