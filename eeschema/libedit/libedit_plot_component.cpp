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


#include <fctsys.h>
#include <gr_basic.h>
#include <sch_draw_panel.h>
#include <sch_screen.h>
#include <general.h>
#include <lib_edit_frame.h>
#include <class_library.h>
#include <plotter.h>

void LIB_EDIT_FRAME::SVG_PlotComponent( const wxString& aFullFileName )
{
    const bool plotColor = true;
    const PAGE_INFO& pageInfo = GetScreen()->GetPageSettings();

    SVG_PLOTTER* plotter = new SVG_PLOTTER();
    plotter->SetPageSettings( pageInfo );
    plotter->SetDefaultLineWidth( GetDefaultLineThickness() );
    plotter->SetColorMode( plotColor );

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

    LIB_PART*      part = GetCurPart();

    if( part )
    {
        TRANSFORM   temp;     // Uses default transform
        wxPoint     plotPos;

        plotPos.x = pageInfo.GetWidthIU() /2;
        plotPos.y = pageInfo.GetHeightIU()/2;

        part->Plot( plotter, GetUnit(), GetConvert(), plotPos, temp );

        // Plot lib fields, not plotted by m_component->Plot():
        part->PlotLibFields( plotter, GetUnit(), GetConvert(), plotPos, temp );
    }

    plotter->EndPlot();
    delete plotter;
}


void LIB_EDIT_FRAME::PrintPage( wxDC* aDC )
{
    LIB_PART*      part = GetCurPart();

    if( !part )
        return;

    wxSize pagesize = GetScreen()->GetPageSettings().GetSizeIU();

    /* Plot item centered to the page
     * In libedit, the component is centered at 0,0 coordinates.
     * So we must plot it with an offset = pagesize/2.
     */
    wxPoint plot_offset;
    plot_offset.x = pagesize.x / 2;
    plot_offset.y = pagesize.y / 2;

    part->Print( aDC, plot_offset, m_unit, m_convert, PART_DRAW_OPTIONS::Default() );
}
