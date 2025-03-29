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

#include <boost/algorithm/string/case_conv.hpp>
#include <magic_enum.hpp>

#include <core/arraydim.h>
#include <settings/layer_settings_utils.h>


GAL_SET UserVisbilityLayers()
{
    static const GAL_LAYER_ID layers[] = {
        LAYER_TRACKS,
        LAYER_VIAS,
        LAYER_PADS,
        LAYER_ZONES,
        LAYER_FILLED_SHAPES,
        LAYER_DRAW_BITMAPS,
        LAYER_FOOTPRINTS_FR,
        LAYER_FOOTPRINTS_BK,
        LAYER_FP_VALUES,
        LAYER_FP_REFERENCES,
        LAYER_FP_TEXT,
        LAYER_ANCHOR,
        LAYER_POINTS,
        LAYER_RATSNEST,
        LAYER_DRC_WARNING,
        LAYER_DRC_ERROR,
        LAYER_DRC_EXCLUSION,
        LAYER_LOCKED_ITEM_SHADOW,
        LAYER_CONFLICTS_SHADOW,
        LAYER_BOARD_OUTLINE_AREA,
        LAYER_DRAWINGSHEET,
        LAYER_GRID,
    };

    static const GAL_SET saved( layers, arrayDim( layers ) );
    return saved;
}


GAL_LAYER_ID RenderLayerFromVisibilityLayer( VISIBILITY_LAYER aLayer )
{
    switch( aLayer )
    {
    case VISIBILITY_LAYER::TRACKS:                  return LAYER_TRACKS;
    case VISIBILITY_LAYER::VIAS:                    return LAYER_VIAS;
    case VISIBILITY_LAYER::PADS:                    return LAYER_PADS;
    case VISIBILITY_LAYER::ZONES:                   return LAYER_ZONES;
    case VISIBILITY_LAYER::SHAPES:                  return LAYER_FILLED_SHAPES;
    case VISIBILITY_LAYER::BITMAPS:                 return LAYER_DRAW_BITMAPS;
    case VISIBILITY_LAYER::FOOTPRINTS_FRONT:        return LAYER_FOOTPRINTS_FR;
    case VISIBILITY_LAYER::FOOTPRINTS_BACK:         return LAYER_FOOTPRINTS_BK;
    case VISIBILITY_LAYER::FOOTPRINT_VALUES:        return LAYER_FP_VALUES;
    case VISIBILITY_LAYER::FOOTPRINT_REFERENCES:    return LAYER_FP_REFERENCES;
    case VISIBILITY_LAYER::FOOTPRINT_TEXT:          return LAYER_FP_TEXT;
    case VISIBILITY_LAYER::FOOTPRINT_ANCHORS:       return LAYER_ANCHOR;
    case VISIBILITY_LAYER::POINTS:                  return LAYER_POINTS;
    case VISIBILITY_LAYER::RATSNEST:                return LAYER_RATSNEST;
    case VISIBILITY_LAYER::DRC_WARNINGS:            return LAYER_DRC_WARNING;
    case VISIBILITY_LAYER::DRC_ERRORS:              return LAYER_DRC_ERROR;
    case VISIBILITY_LAYER::DRC_EXCLUSIONS:          return LAYER_DRC_EXCLUSION;
    case VISIBILITY_LAYER::LOCKED_ITEM_SHADOWS:     return LAYER_LOCKED_ITEM_SHADOW;
    case VISIBILITY_LAYER::CONFLICT_SHADOWS:        return LAYER_CONFLICTS_SHADOW;
    case VISIBILITY_LAYER::BOARD_OUTLINE_AREA:      return LAYER_BOARD_OUTLINE_AREA;
    case VISIBILITY_LAYER::DRAWING_SHEET:           return LAYER_DRAWINGSHEET;
    case VISIBILITY_LAYER::GRID:                    return LAYER_GRID;
    }

    wxCHECK_MSG( false, GAL_LAYER_ID_END, "Unhandled layer in RenderLayerFromVisibilityLayer" );
}


std::optional<VISIBILITY_LAYER> VisibilityLayerFromRenderLayer( GAL_LAYER_ID aLayerId )
{
    switch( aLayerId )
    {
    case LAYER_TRACKS:              return VISIBILITY_LAYER::TRACKS;
    case LAYER_VIAS:                return VISIBILITY_LAYER::VIAS;
    case LAYER_PADS:                return VISIBILITY_LAYER::PADS;
    case LAYER_ZONES:               return VISIBILITY_LAYER::ZONES;
    case LAYER_FILLED_SHAPES:       return VISIBILITY_LAYER::SHAPES;
    case LAYER_DRAW_BITMAPS:        return VISIBILITY_LAYER::BITMAPS;
    case LAYER_FOOTPRINTS_FR:       return VISIBILITY_LAYER::FOOTPRINTS_FRONT;
    case LAYER_FOOTPRINTS_BK:       return VISIBILITY_LAYER::FOOTPRINTS_BACK;
    case LAYER_FP_VALUES:           return VISIBILITY_LAYER::FOOTPRINT_VALUES;
    case LAYER_FP_REFERENCES:       return VISIBILITY_LAYER::FOOTPRINT_REFERENCES;
    case LAYER_FP_TEXT:             return VISIBILITY_LAYER::FOOTPRINT_TEXT;
    case LAYER_ANCHOR:              return VISIBILITY_LAYER::FOOTPRINT_ANCHORS;
    case LAYER_POINTS:              return VISIBILITY_LAYER::POINTS;
    case LAYER_RATSNEST:            return VISIBILITY_LAYER::RATSNEST;
    case LAYER_DRC_WARNING:         return VISIBILITY_LAYER::DRC_WARNINGS;
    case LAYER_DRC_ERROR:           return VISIBILITY_LAYER::DRC_ERRORS;
    case LAYER_DRC_EXCLUSION:       return VISIBILITY_LAYER::DRC_EXCLUSIONS;
    case LAYER_LOCKED_ITEM_SHADOW:  return VISIBILITY_LAYER::LOCKED_ITEM_SHADOWS;
    case LAYER_CONFLICTS_SHADOW:    return VISIBILITY_LAYER::CONFLICT_SHADOWS;
    case LAYER_BOARD_OUTLINE_AREA:  return VISIBILITY_LAYER::BOARD_OUTLINE_AREA;
    case LAYER_DRAWINGSHEET:        return VISIBILITY_LAYER::DRAWING_SHEET;
    case LAYER_GRID:                return VISIBILITY_LAYER::GRID;
    default:
        break;
    }

    return std::nullopt;
}


std::optional<GAL_LAYER_ID> RenderLayerFromVisbilityString( const std::string& aLayer )
{
    if( std::optional<VISIBILITY_LAYER> val =
                magic_enum::enum_cast<VISIBILITY_LAYER>( aLayer, magic_enum::case_insensitive ) )
    {
        return RenderLayerFromVisibilityLayer( *val );
    }

    return std::nullopt;
}


std::string VisibilityLayerToString( VISIBILITY_LAYER aLayerId )
{
    std::string ret( magic_enum::enum_name<VISIBILITY_LAYER>( aLayerId ) );
    boost::algorithm::to_lower( ret );
    return ret;
}
