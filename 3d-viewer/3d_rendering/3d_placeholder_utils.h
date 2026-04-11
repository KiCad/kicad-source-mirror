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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef PLACEHOLDER_3D_UTILS_H
#define PLACEHOLDER_3D_UTILS_H

#include <math/box2.h>
#include <layer_ids.h>
#include <plugins/3dapi/xv3d_types.h>

class FOOTPRINT;
class SHAPE_POLY_SET;
class EXTRUDED_3D_BODY;
enum class EXTRUSION_MATERIAL;

struct EXTRUSION_MATERIAL_PROPS
{
    SFVEC3F m_Ambient;
    SFVEC3F m_Specular;
    float   m_Shininess;
};

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
 * @param aLayerOverride when not UNDEFINED_LAYER, use this layer instead of
 *                       the footprint's extruded body layer setting.
 * @return true if outline was found.
 */
bool GetExtrusionOutline( const FOOTPRINT* aFootprint, SHAPE_POLY_SET& aOutline,
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

EXTRUSION_MATERIAL_PROPS GetMaterialProps( EXTRUSION_MATERIAL aMaterial, const SFVEC3F& aDiffuse );

/**
 * Apply 2D extrusion transforms (rotation, scale, offset) to an outline.
 */
void ApplyExtrusionTransform( SHAPE_POLY_SET& aOutline, const EXTRUDED_3D_BODY* aBody, const VECTOR2I& aFpPos );

#endif // PLACEHOLDER_3D_UTILS_H
