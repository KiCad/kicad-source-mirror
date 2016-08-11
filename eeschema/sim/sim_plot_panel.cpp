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

#include <limits>

SIM_PLOT_PANEL::SIM_PLOT_PANEL( wxWindow* parent, wxWindowID id, const wxPoint& pos,
                const wxSize& size, long style, const wxString& name )
    : wxMathGL( parent, id, pos, size, style, name ), m_painter( this )
{
    AutoResize = true;
    resetRanges();
    SetDraw( &m_painter );
}


SIM_PLOT_PANEL::~SIM_PLOT_PANEL()
{
}


template<typename T>
static std::pair<T, T> find_minmax( const T* aArray, unsigned int aSize )
{
    std::pair<T, T> result( std::numeric_limits<T>::max(), std::numeric_limits<T>::min() );
    const T* ptr = aArray;

    for( unsigned int i = 0; i < aSize; ++i )
    {
        if( *ptr < result.first )
            result.first = *ptr;

        if( *ptr > result.second )
            result.second = *ptr;

        ++ptr;
    }

    return result;
}


void SIM_PLOT_PANEL::AddTrace( const wxString& aName, int aPoints,
                                double* aT, double* aY, int aFlags )
{
    TRACE trace;

    trace.name = aName;
    trace.style = wxString( '-' ) + m_painter.GenerateColor( SIM_PLOT_PAINTER::DARK );
    trace.x.Set( aT, aPoints );
    trace.y.Set( aY, aPoints );
    m_traces.push_back( trace );

    // Update axis ranges
    std::pair<double, double> traceRangeT = find_minmax( aT, aPoints );
    m_axisRangeX.first = std::min( traceRangeT.first, m_axisRangeX.first );
    m_axisRangeX.second = std::max( traceRangeT.second, m_axisRangeX.second );

    std::pair<double, double> traceRangeY = find_minmax( aY, aPoints );
    m_axisRangeY.first = std::min( traceRangeY.first, m_axisRangeY.first );
    m_axisRangeY.second = std::max( traceRangeY.second, m_axisRangeY.second );

    Update();
}


void SIM_PLOT_PANEL::DeleteTraces()
{
    m_traces.clear();
    resetRanges();
    Update();
}


void SIM_PLOT_PANEL::resetRanges()
{
    // Set ranges to inverted values, so when there is a new plot added, it will
    // overridden with correct values
    m_axisRangeX.first = std::numeric_limits<double>::max();
    m_axisRangeX.second = std::numeric_limits<double>::min();
    m_axisRangeY.first = std::numeric_limits<double>::max();
    m_axisRangeY.second = std::numeric_limits<double>::min();
}


int SIM_PLOT_PAINTER::Draw( mglGraph* aGraph )
{
    const std::vector<SIM_PLOT_PANEL::TRACE>& traces = m_parent->m_traces;
    const std::pair<double, double>& axisRangeX = m_parent->m_axisRangeX;
    std::pair<double, double> axisRangeY = m_parent->m_axisRangeY;

    aGraph->Clf();

    // Axis settings
    // Use autorange values if possible
    if( axisRangeX.first < axisRangeX.second )
        aGraph->SetRange( 'x', axisRangeX.first, axisRangeX.second );
    else
        aGraph->SetRange( 'x', 0, 1 );

    if( axisRangeY.first < axisRangeY.second )
    {
        // Increase the Y axis range, so it is easy to read the extreme values
        axisRangeY.first -= axisRangeY.second * 0.1;
        axisRangeY.second += axisRangeY.second * 0.1;
        aGraph->SetRange( 'y', axisRangeY.first, axisRangeY.second );
    }
    else
    {
        aGraph->SetRange( 'y', 0, 1 );
    }

    aGraph->Axis( "xy" );
    aGraph->Label( 'x', "Time [s]", 0 );
    aGraph->Label( 'y', "Voltage [V]", 0 );

    aGraph->Box();
    aGraph->Grid();

    // Draw traces
    for( auto t : traces )
    {
        aGraph->AddLegend( (const char*) t.name.c_str(), t.style );
        aGraph->Plot( t.y, t.style );
    }

    if( traces.size() )
        aGraph->Legend( 1, "-#" );  // legend entries horizontally + draw a box around legend

    return 0;
}


wxString SIM_PLOT_PAINTER::GenerateColor( COLOR_TYPE aType )
{
    const char colors[] = "rgbcmylenupq";
    const unsigned int colorsNumber = sizeof( colors ) - 1;

    // Safe defaults
    char color = 'k';       // black
    int shade = 5;

    switch( aType )
    {
        case LIGHT:
            color = colors[m_lightColorIdx % colorsNumber];
            shade = 5 + m_lightColorIdx / colorsNumber;
            ++m_lightColorIdx;

            if( shade == 10 )
            {
                // Reached the color limit
                shade = 5;
                m_lightColorIdx = 0;
            }
            break;

        case DARK:
            color = toupper( colors[m_darkColorIdx % colorsNumber] );
            shade = 5 - m_darkColorIdx / colorsNumber;
            ++m_darkColorIdx;

            if( shade == 0 )
            {
                // Reached the color limit
                shade = 5;
                m_darkColorIdx = 0;
            }
            break;
    }

    return wxString::Format( "{%c%d}", color, shade );
}
