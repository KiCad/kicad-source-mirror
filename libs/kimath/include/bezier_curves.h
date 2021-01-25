/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014-2021 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <math/vector2d.h>

/**
 * Bezier curves to polygon converter.
 *
 * Only quadratic and cubic Bezier curves are handled
 */
class BEZIER_POLY
{
public:
    BEZIER_POLY( const std::vector<wxPoint>& aControlPoints );

    BEZIER_POLY( const std::vector<VECTOR2D>& aControlPoints )
        : m_ctrlPts( aControlPoints )
    {
        m_minSegLen = 0.0;
    }

    /**
     * Convert a Bezier curve to a polygon.
     *
     * @param aOutput will be used as an output vector storing polygon points.
     * @param aMinSegLen is the min dist between 2 successive points.
     * It can be used to reduce the number of points.
     * (the last point is always generated)
     */
    void GetPoly( std::vector<wxPoint>& aOutput, int aMinSegLen = 0 );
    void GetPoly( std::vector<VECTOR2D>& aOutput, double aMinSegLen = 0.0 );

private:
    double m_minSegLen;

    ///< Control points
    std::vector<VECTOR2D> m_ctrlPts;
};

#endif  // BEZIER_CURVES_H
