/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009-2014 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2014 KiCad Developers, see CHANGELOG.TXT for contributors.
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
 * Function Bezier2Poly
 * convert a Bezier curve to a polyline
 * @param c1 - First point to convert.
 * @param c2 - Second point to convert.
 * @param c3 - Third point to convert.
 * @return a std::vector<wxPoint> containing the points of the polyline
 */
std::vector<wxPoint> Bezier2Poly(wxPoint c1, wxPoint c2, wxPoint c3);

std::vector<wxPoint> Bezier2Poly(int x1, int y1, int x2, int y2, int x3, int y3);

/**
 * Function Bezier2Poly
 * convert a Bezier curve to a polyline
 * @return a std::vector<wxPoint> containing the points of the polyline
 * @param c1 - First point to convert.
 * @param c2 - Second point to convert.
 * @param c3 - Third point to convert.
 * @param c4 - Fourth point to convert.
 * @return a std::vector<wxPoint> containing the points of the polyline
 */
std::vector<wxPoint> Bezier2Poly( wxPoint c1, wxPoint c2, wxPoint c3,wxPoint c4 );

std::vector<wxPoint> Bezier2Poly(int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4);


#endif  // BEZIER_CURVES_H
