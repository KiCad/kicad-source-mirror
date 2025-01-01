/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 Sylwester Kocjan <s.kocjan@o2.pl>
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

#ifndef __SIM_PLOT_COLORS__
#define __SIM_PLOT_COLORS__


#include <map>
#include <vector>
#include <wx/colour.h>
#include <wx/string.h>


class TRACE;

/**
 * @file sim_plot_colors.h
 *
 * Class is responsible for providing colors for traces on simulation plot
 */
class SIM_PLOT_COLORS
{
public:
    SIM_PLOT_COLORS(){};
    ~SIM_PLOT_COLORS(){};

    // Identifiers (indexes) for color choice in color table
    enum class COLOR_SET
    {
        BACKGROUND,
        FOREGROUND,
        AXIS,
        TRACE // First index for trace colors list
    };

    /**
     * @return the wxColor selected in color list.
     * @param aColorId is the index in color list
     */
    wxColour GetPlotColor( enum COLOR_SET aColorId );

    /**
     * @return a new color from the palette
     * @param a collection of traces in the plot panel
     */
    wxColour GenerateColor( std::map<wxString, wxColour> aTraceColors );

    /**
     * @brief Fills m_colorList by a default set of colors.
     * @param aWhiteBg = true to use a white (or clear) background
     *                  false to use a dark background
     */
    static void FillDefaultColorList( bool aWhiteBg );

private:
    /**
     * @return the count of colors in color list
     */
    enum COLOR_SET getPlotColorCount() { return static_cast<enum COLOR_SET>( m_colorList.size() ); }

private:
    ///< The color list to draw traces, bg, fg, axis...
    static std::vector<wxColour> m_colorList;

};

inline bool operator<( SIM_PLOT_COLORS::COLOR_SET& x, SIM_PLOT_COLORS::COLOR_SET& y );
inline bool operator>=( SIM_PLOT_COLORS::COLOR_SET& x, SIM_PLOT_COLORS::COLOR_SET& y );
inline bool operator<( SIM_PLOT_COLORS::COLOR_SET& x, int y );
inline bool operator>=( SIM_PLOT_COLORS::COLOR_SET& x, int y );
inline SIM_PLOT_COLORS::COLOR_SET  operator+( SIM_PLOT_COLORS::COLOR_SET x,
                                              SIM_PLOT_COLORS::COLOR_SET y );
inline SIM_PLOT_COLORS::COLOR_SET  operator-( SIM_PLOT_COLORS::COLOR_SET x,
                                              SIM_PLOT_COLORS::COLOR_SET y );
inline SIM_PLOT_COLORS::COLOR_SET  operator%( int x, SIM_PLOT_COLORS::COLOR_SET y );
inline SIM_PLOT_COLORS::COLOR_SET& operator++( SIM_PLOT_COLORS::COLOR_SET& x );


#endif // __SIM_PLOT_COLORS__
