/*
* This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LAYER_SETTINGS_UTILS_H
#define LAYER_SETTINGS_UTILS_H

#include <optional>
#include <layer_ids.h>

/**
 * The set of things that can have visibility settings stored in a project file
 * (for example in a view preset).  This is maintained separately from the enums in
 * layer_ids.h because not all GAL layers get visibility controls, and these are
 * turned into strings for storing in JSON settings files.
 */
enum class VISIBILITY_LAYER
{
    TRACKS,
    VIAS,
    PADS,
    ZONES,
    SHAPES,
    BITMAPS,
    FOOTPRINTS_FRONT,
    FOOTPRINTS_BACK,
    FOOTPRINT_VALUES,
    FOOTPRINT_REFERENCES,
    FOOTPRINT_TEXT,
    FOOTPRINT_ANCHORS,
    POINTS,
    RATSNEST,
    DRC_WARNINGS,
    DRC_ERRORS,
    DRC_EXCLUSIONS,
    LOCKED_ITEM_SHADOWS,
    CONFLICT_SHADOWS,
    BOARD_OUTLINE_AREA,
    DRAWING_SHEET,
    GRID
};

/**
 * The set of GAL_LAYER_IDs that correspond to VISIBILITY_LAYERS
 */
GAL_SET UserVisbilityLayers();

GAL_LAYER_ID RenderLayerFromVisibilityLayer( VISIBILITY_LAYER aLayer );
std::optional<VISIBILITY_LAYER> VisibilityLayerFromRenderLayer( GAL_LAYER_ID aLayerId );

std::optional<GAL_LAYER_ID> RenderLayerFromVisbilityString( const std::string& aLayer );
std::string VisibilityLayerToString( VISIBILITY_LAYER aLayerId );


#endif //LAYER_SETTINGS_UTILS_H
