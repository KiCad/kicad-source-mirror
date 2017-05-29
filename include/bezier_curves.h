/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009-2014 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2017 KiCad Developers, see CHANGELOG.TXT for contributors.
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

#ifndef BEZIER_CURVES_H
#define BEZIER_CURVES_H

#include <vector>
#include <wx/gdicmn.h>

/**
 * Bezier curves to polygon converter.
 */
class BEZIER_POLY
{
public:
    BEZIER_POLY( int x1, int y1, int x2, int y2, int x3, int y3 )
    {
        m_ctrlPts.emplace_back( x1, y1 );
        m_ctrlPts.emplace_back( x2, y2 );
        m_ctrlPts.emplace_back( x3, y3 );
        m_output = nullptr;
    }

    BEZIER_POLY( int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4 )
    {
        m_ctrlPts.emplace_back( x1, y1 );
        m_ctrlPts.emplace_back( x2, y2 );
        m_ctrlPts.emplace_back( x3, y3 );
        m_ctrlPts.emplace_back( x4, y4 );
        m_output = nullptr;
    }

    BEZIER_POLY( const std::vector<wxPoint>& aControlPoints )
        : m_ctrlPts( aControlPoints )
    {
        m_output = nullptr;
    }

    /**
     * Converts Bezier curve to a polygon.
     * @param aOutput will be used as an output vector storing polygon points.
     */
    void GetPoly( std::vector<wxPoint>& aOutput );

private:
    ///> Control points
    std::vector<wxPoint> m_ctrlPts;

    ///> Pointer to the output vector
    std::vector<wxPoint>* m_output;

    void addSegment( const wxPoint& aSegment )
    {
        if( m_output->back() != aSegment )
            m_output->push_back( aSegment );
    }

    void recursiveBezier( int x1, int y1, int x2, int y2, int x3, int y3, unsigned int level );
    void recursiveBezier( int x1, int y1, int x2, int y2,
            int x3, int y3, int x4, int y4, unsigned int level );


    // Conversion parameters
    constexpr static double angle_tolerance     = 0.0;
    constexpr static double cusp_limit          = 0.0;
    constexpr static int    recursion_limit     = 12;
    constexpr static double approximation_scale = 0.5;  // 1
    constexpr static double distance_tolerance_square = ( 0.5 / approximation_scale ) * ( 0.5 / approximation_scale );

    constexpr static double curve_collinearity_epsilon    = 1e-30;
    constexpr static double curve_angle_tolerance_epsilon = 0.0001;
};

#endif  // BEZIER_CURVES_H
