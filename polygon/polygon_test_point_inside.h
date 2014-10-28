/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007-2014 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2007-2014 KiCad Developers, see CHANGELOG.TXT for contributors.
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

#ifndef __WXWINDOWS__
// define here wxPoint if we want to compile outside wxWidgets
class wxPoint
{
public:
    int x, y;
};
#endif
class CPOLYGONS_LIST;

/**
 * Function TestPointInsidePolygon
 * test if a point is inside or outside a polygon.
 * @param aPolysList: the list of polygons
 * @param aIdxstart: the starting point of a given polygon in m_FilledPolysList.
 * @param aIdxend: the ending point of the polygon in m_FilledPolysList.
 * @param aRefx, aRefy: the point coordinate to test
 * @return true if the point is inside, false for outside
 */
bool TestPointInsidePolygon( const CPOLYGONS_LIST& aPolysList,
                             int             aIdxstart,
                             int             aIdxend,
                             int             aRefx,
                             int             aRefy);
/**
 * Function TestPointInsidePolygon (overlaid)
 * same as previous, but mainly use wxPoint
 * @param aPolysList: the list of polygons
 * @param aCount: corners count in aPolysList.
 * @param aRefPoint: the point coordinate to test
 * @return true if the point is inside, false for outside
 */
bool TestPointInsidePolygon( const wxPoint* aPolysList,
                             int      aCount,
                             const wxPoint  &aRefPoint );
