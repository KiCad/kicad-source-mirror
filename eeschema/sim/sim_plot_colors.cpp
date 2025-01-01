/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 Sylwester Kocjan <s.kocjan@o2.pl>
 * Copyright (C) 2023 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * https://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */


#include "sim_plot_colors.h"
#include <sim/sim_plot_tab.h>
#include <wx/stc/stc.h>


std::vector<wxColour> SIM_PLOT_COLORS::m_colorList;


inline bool operator<( SIM_PLOT_COLORS::COLOR_SET& x, SIM_PLOT_COLORS::COLOR_SET& y )
{
    return static_cast<int>( x ) < static_cast<int>( y );
}


inline bool operator>=( SIM_PLOT_COLORS::COLOR_SET& x, SIM_PLOT_COLORS::COLOR_SET& y )
{
    return static_cast<int>( x ) >= static_cast<int>( y );
}


inline bool operator<( SIM_PLOT_COLORS::COLOR_SET& x, int y )
{
    return static_cast<int>( x ) < y;
}


inline bool operator>=( SIM_PLOT_COLORS::COLOR_SET& x, int y )
{
    return static_cast<int>( x ) >= y;
}


inline SIM_PLOT_COLORS::COLOR_SET operator+( SIM_PLOT_COLORS::COLOR_SET x,
                                             SIM_PLOT_COLORS::COLOR_SET y )
{
    return static_cast<SIM_PLOT_COLORS::COLOR_SET>( static_cast<int>( x ) + static_cast<int>( y ) );
}


inline SIM_PLOT_COLORS::COLOR_SET operator-( SIM_PLOT_COLORS::COLOR_SET x,
                                             SIM_PLOT_COLORS::COLOR_SET y )
{
    return static_cast<SIM_PLOT_COLORS::COLOR_SET>( static_cast<int>( x ) - static_cast<int>( y ) );
}


inline SIM_PLOT_COLORS::COLOR_SET operator%( int x, SIM_PLOT_COLORS::COLOR_SET y )
{
    return static_cast<SIM_PLOT_COLORS::COLOR_SET>( x % static_cast<int>( y ) );
}


inline SIM_PLOT_COLORS::COLOR_SET& operator++( SIM_PLOT_COLORS::COLOR_SET& x )
{
    x = static_cast<SIM_PLOT_COLORS::COLOR_SET>( (int) x + 1 );
    return x;
}

wxColour SIM_PLOT_COLORS::GetPlotColor( COLOR_SET aColorId )
{
    // return the wxColor selected in color list or BLACK is not in list
    if( aColorId >= 0 && aColorId < m_colorList.size() )
        return m_colorList[static_cast<int>( aColorId )];

    return wxColour( 0, 0, 0 );
}


void SIM_PLOT_COLORS::FillDefaultColorList( bool aDarkMode )
{
    m_colorList.clear();

    if( aDarkMode )
    {
        m_colorList.emplace_back( 0, 0, 0 );       // Bg color
        m_colorList.emplace_back( 255, 255, 255 ); // Fg color (texts)
        m_colorList.emplace_back( 130, 130, 130 ); // Axis color
    }
    else
    {
        m_colorList.emplace_back( 255, 255, 255 ); // Bg color
        m_colorList.emplace_back( 0, 0, 0 );       // Fg color (texts)
        m_colorList.emplace_back( 130, 130, 130 ); // Axis color
    }

    // Add a list of color for traces, starting at index SIM_TRACE_COLOR
    m_colorList.emplace_back( 0xE4, 0x1A, 0x1C );
    m_colorList.emplace_back( 0x37, 0x7E, 0xB8 );
    m_colorList.emplace_back( 0x4D, 0xAF, 0x4A );
    m_colorList.emplace_back( 0x98, 0x4E, 0xA3 );
    m_colorList.emplace_back( 0xFF, 0x7F, 0x00 );
    m_colorList.emplace_back( 0xFF, 0xFF, 0x33 );
    m_colorList.emplace_back( 0xA6, 0x56, 0x28 );
    m_colorList.emplace_back( 0xF7, 0x81, 0xBF );
    m_colorList.emplace_back( 0x66, 0xC2, 0xA5 );
    m_colorList.emplace_back( 0xFC, 0x8D, 0x62 );
    m_colorList.emplace_back( 0x8D, 0xA0, 0xCB );
    m_colorList.emplace_back( 0xE7, 0x8A, 0xC3 );
    m_colorList.emplace_back( 0xA6, 0xD8, 0x54 );
    m_colorList.emplace_back( 0xFF, 0xD9, 0x2F );
    m_colorList.emplace_back( 0xE5, 0xC4, 0x94 );
    m_colorList.emplace_back( 0xB3, 0xB3, 0xB3 );
}


wxColour SIM_PLOT_COLORS::GenerateColor( std::map<wxString, wxColour> aTraceColors )
{
    for( COLOR_SET i = COLOR_SET::TRACE; i < getPlotColorCount(); ++i )
    {
        bool hasColor = false;

        for( const auto& [ vectorName, traceColor ] : aTraceColors )
        {
            if( traceColor == GetPlotColor( i ) )
            {
                hasColor = true;
                break;
            }
        }

        if( !hasColor )
            return GetPlotColor( i );
    }

    // If all colors are in use, choose a suitable color in list
    COLOR_SET idx = aTraceColors.size() % ( getPlotColorCount() - COLOR_SET::TRACE );
    return GetPlotColor( COLOR_SET::TRACE + idx );
}
