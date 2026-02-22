/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef DRC_RULE_EDITOR_ENUMS_H_
#define DRC_RULE_EDITOR_ENUMS_H_

#include <wx/string.h>
#include <vector>


/**
 * Result of a validation operation.
 * Used by constraint data classes to report validation errors without GUI dependencies.
 */
struct VALIDATION_RESULT
{
    bool                     isValid = true;
    std::vector<wxString>    errors;

    void AddError( const wxString& aError )
    {
        isValid = false;
        errors.push_back( aError );
    }

    void Merge( const VALIDATION_RESULT& aOther )
    {
        if( !aOther.isValid )
        {
            isValid = false;
            errors.insert( errors.end(), aOther.errors.begin(), aOther.errors.end() );
        }
    }
};


struct RULE_GENERATION_CONTEXT
{
    wxString ruleName;
    wxString comment;
    wxString conditionExpression;
    wxString layerClause;
    wxString constraintCode;
};


enum DRC_RULE_EDITOR_ITEM_TYPE
{
    ROOT = 0,
    CATEGORY,
    RULE_TYPE,
    CONSTRAINT,
    RULE,
};

enum DRC_RULE_EDITOR_CONSTRAINT_NAME
{
    MINIMUM_CLEARANCE = 0,
    CREEPAGE_DISTANCE,
    MINIMUM_CONNECTION_WIDTH,
    COPPER_TO_HOLE_CLEARANCE,
    HOLE_TO_HOLE_CLEARANCE,
    MINIMUM_THERMAL_RELIEF_SPOKE_COUNT,
    MINIMUM_ANNULAR_WIDTH,
    COPPER_TO_EDGE_CLEARANCE,
    COURTYARD_CLEARANCE,
    PHYSICAL_CLEARANCE,
    MINIMUM_DRILL_SIZE,
    HOLE_SIZE,
    HOLE_TO_HOLE_DISTANCE,
    MINIMUM_UVIA_HOLE,
    MINIMUM_UVIA_DIAMETER,
    MINIMUM_VIA_DIAMETER,
    VIA_STYLE,
    MINIMUM_TEXT_HEIGHT_AND_THICKNESS,
    SILK_TO_SILK_CLEARANCE,
    SILK_TO_SOLDERMASK_CLEARANCE,
    MINIMUM_SOLDERMASK_SILVER,
    SOLDERMASK_EXPANSION,
    SOLDERPASTE_EXPANSION,
    MAXIMUM_ALLOWED_DEVIATION,
    MINIMUM_ANGULAR_RING,
    MATCHED_LENGTH_DIFF_PAIR,
    ROUTING_DIFF_PAIR,
    ROUTING_WIDTH,
    MAXIMUM_VIA_COUNT,
    ABSOLUTE_LENGTH,
    PERMITTED_LAYERS,
    ALLOWED_ORIENTATION,
    VIAS_UNDER_SMD,
    CUSTOM_RULE
};


/**
 * Layer categories for filtering the layer selector dropdown.
 * Each constraint type maps to one category that determines which layers are shown.
 */
enum class DRC_LAYER_CATEGORY
{
    COPPER_ONLY,           ///< Copper layers + inner/outer synthetic
    SILKSCREEN_ONLY,       ///< F_SilkS, B_SilkS
    SOLDERMASK_ONLY,       ///< F_Mask, B_Mask
    SOLDERPASTE_ONLY,      ///< F_Paste, B_Paste
    TOP_BOTTOM_ANY,        ///< Simplified top/bottom/any selector with custom translation
    GENERAL_ANY_LAYER,     ///< All layers + inner/outer synthetic
    NO_LAYER_SELECTOR      ///< Hide layer selector entirely
};


/**
 * Synthetic layer pseudo-IDs for the layer selector.
 * Negative values avoid collision with PCB_LAYER_ID (which are >= 0).
 * Pattern follows UNDEFINED_LAYER = -1, UNSELECTED_LAYER = -2 in layer_ids.h.
 */
enum DRC_LAYER_SELECTOR_ID : int
{
    LAYER_SEL_ANY    = -10,   ///< No layer filter (default "Any" selection)
    LAYER_SEL_OUTER  = -11,   ///< External copper layers (F_Cu + B_Cu)
    LAYER_SEL_INNER  = -12,   ///< Internal copper layers (In1_Cu through In30_Cu)
    LAYER_SEL_TOP    = -13,   ///< Context-dependent front/top layer
    LAYER_SEL_BOTTOM = -14,   ///< Context-dependent back/bottom layer
};


#endif // DRC_RULE_EDITOR_ENUMS_H_
