/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012-2014 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2012-2014 KiCad Developers, see CHANGELOG.TXT for contributors.
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

/*
 * file polygons_defs.h
 * definitions to use boost::polygon in KiCad.
 */

#ifndef _POLYGONS_DEFS_H_
#define _POLYGONS_DEFS_H_

#include <boost/polygon/polygon.hpp>

// Define some types used here from boost::polygon
namespace bpl = boost::polygon;         // bpl = boost polygon library
using namespace bpl::operators;         // +, -, =, ...

// Definitions needed by boost::polygon
typedef int                    coordinate_type;

/**
 * KI_POLYGON defines a single polygon ( boost::polygon_data type.
 * When holes are created in a KPolygon, they are
 * linked to main outline by overlapping segments,
 * so there is always one polygon and one list of corners
 * coordinates are int
 */
typedef bpl::polygon_data<int> KI_POLYGON;

/**
 * KI_POLYGON_SET defines a set of single KI_POLYGON.
 * A KI_POLYGON_SET is used to store a set of polygons
 * when performing operations between 2 polygons
 * or 2 sets of polygons
 * The result of operations like and, xor... between 2 polygons
 * is always stored in a KI_POLYGON_SET, because these operations
 * can create many polygons
 */
typedef std::vector<KI_POLYGON>  KI_POLYGON_SET;

/**
 * KI_POLY_POINT defines a point for boost::polygon.
 * KI_POLY_POINT store x and y coordinates (int)
 */
typedef bpl::point_data<int>   KI_POLY_POINT;

/**
 * KI_POLYGON_WITH_HOLES defines a single polygon with holes
 * When holes are created in a KI_POLYGON_WITH_HOLES, they are
 * stored as separate single polygons,
 * KI_POLYGON_WITH_HOLES store always one polygon for the external outline
 * and one list of polygons (holes) which can be empty
 */
typedef bpl::polygon_with_holes_data<int> KI_POLYGON_WITH_HOLES;

/**
 * KI_POLYGON_WITH_HOLES_SET defines a set of KI_POLYGON_WITH_HOLES.
 * A KI_POLYGON_WITH_HOLES_SET is used to store a set of polygons with holes
 * when performing operations between 2 polygons
 * or 2 sets of polygons with holes
 * The result of operations like and, xor... between 2 polygons with holes
 * is always stored in a KI_POLYGON_WITH_HOLES_SET, because these operations
 * can create many separate polygons with holespolygons
 */
typedef std::vector<KI_POLYGON_WITH_HOLES>  KI_POLYGON_WITH_HOLES_SET;


#endif          // #ifndef _POLYGONS_DEFS_H_
