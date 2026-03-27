/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef PLACEHOLDER_3D_UTILS_H
#define PLACEHOLDER_3D_UTILS_H

#include <math/box2.h>
#include <layer_ids.h>

class FOOTPRINT;
class SHAPE_POLY_SET;

/**
 * Calculate a local space bounding box for a placeholder 3D model.
 *
 * Attempts pads first, then graphical items on fab/courtyard/silk layers,
 * then falls back to the footprint bounding box.
 */
BOX2I CalcPlaceholderLocalBox( const FOOTPRINT* aFootprint );

/**
 * Get the extrusion outline polygon for a footprint in board coordinates.
 *
 * @param aFootprint the footprint to extract an outline from.
 * @param aOutline output polygon set.
 * @param aForceFilled when true, treat all closed contours as filled (for STEP export).
 *                     When false, use per-shape TransformShapeToPolygon which respects
 *                     fill state and handles all shape types including arcs.
 * @param aLayerOverride when not UNDEFINED_LAYER, use this layer instead of
 *                       the footprint's extruded body layer setting.
 * @return true if outline was found.
 */
bool GetExtrusionOutline( const FOOTPRINT* aFootprint, SHAPE_POLY_SET& aOutline, bool aForceFilled = false,
                          PCB_LAYER_ID aLayerOverride = UNDEFINED_LAYER );

/**
 * Get the pin outline polygons for extruded THT pin rendering.
 *
 * Collects drill hole shapes from all pads with holes, shrunk to ~90% of the
 * drill diameter so pins sit inside the hole.
 *
 * @return true if at least one hole polygon was generated.
 */
bool GetExtrusionPinOutline( const FOOTPRINT* aFootprint, SHAPE_POLY_SET& aPinPoly );

#endif // PLACEHOLDER_3D_UTILS_H
