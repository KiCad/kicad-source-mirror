/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 CERN
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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

#include "sim_plot_panel.h"

static SIM_PLOT_PANEL* panel = NULL;

static int drawPlotFunc( mglGraph* aGraph )
{
    printf( "DrawPlot [%lu traces]!\n", panel->m_traces.size() );

    aGraph->Clf();
    //aGraph->SetRanges(-10e-3,10e-3,-2,2);
    aGraph->Axis( "x" );
    aGraph->Label( 'x', "Time", 0 );
    aGraph->SetRange( 'x', 0, 10e-3 );

    aGraph->Axis( "y" );
    aGraph->Label( 'y', "Voltage", 0 );
    aGraph->SetRange( 'y', -1.5, 1.5 );

    for( auto t : panel->m_traces )
    {
        aGraph->AddLegend( (const char*) t.name.c_str(), "" );
        aGraph->Plot( t.y );
    }

    aGraph->Box();
    aGraph->Grid();

    if( panel->m_traces.size() )
        aGraph->Legend( 1, "-#" );

    return 0;
}


SIM_PLOT_PANEL::SIM_PLOT_PANEL( wxWindow* parent, wxWindowID id, const wxPoint& pos,
                const wxSize& size, long style, const wxString& name )
    : wxMathGL( parent, id, pos, size, style, name )
{
    panel = this;

    AutoResize = true;
    SetDraw( drawPlotFunc );
//    Update();
}


SIM_PLOT_PANEL::~SIM_PLOT_PANEL()
{
}


void SIM_PLOT_PANEL::AddTrace( const wxString& aName, int aPoints,
                                double* aT, double* aX, int aFlags )
{
    TRACE trace;

    trace.name = aName;
    trace.x.Set( aT, aPoints );
    trace.y.Set( aX, aPoints );

    m_traces.push_back( trace );
    Update();
}


void SIM_PLOT_PANEL::DeleteTraces()
{
    m_traces.clear();
    Update();
}
