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

#include "drc_re_overlay_panel_factory.h"

#include "drc_rule_editor_utils.h"

#include "drc_re_via_style_overlay_panel.h"
#include "drc_re_routing_width_overlay_panel.h"
#include "drc_re_rtg_diff_pair_overlay_panel.h"
#include "drc_re_min_txt_ht_th_overlay_panel.h"
#include "drc_re_abs_length_two_overlay_panel.h"
#include "drc_re_numeric_input_overlay_panel.h"
#include "drc_re_bool_input_overlay_panel.h"
#include "drc_re_allowed_orientation_overlay_panel.h"
#include "drc_re_permitted_layers_overlay_panel.h"

#include "drc_re_via_style_constraint_data.h"
#include "drc_re_routing_width_constraint_data.h"
#include "drc_re_rtg_diff_pair_constraint_data.h"
#include "drc_re_min_txt_ht_th_constraint_data.h"
#include "drc_re_abs_length_two_constraint_data.h"
#include "drc_re_numeric_input_constraint_data.h"
#include "drc_re_bool_input_constraint_data.h"
#include "drc_re_allowed_orientation_constraint_data.h"
#include "drc_re_permitted_layers_constraint_data.h"


DRC_RE_BITMAP_OVERLAY_PANEL*
DRC_RE_OVERLAY_PANEL_FACTORY::CreateOverlayPanel( wxWindow* aParent,
                                                   DRC_RULE_EDITOR_CONSTRAINT_NAME aType,
                                                   DRC_RE_BASE_CONSTRAINT_DATA* aData,
                                                   EDA_UNITS aUnits )
{
    switch( aType )
    {
    case VIA_STYLE:
        return new DRC_RE_VIA_STYLE_OVERLAY_PANEL(
                aParent,
                dynamic_cast<DRC_RE_VIA_STYLE_CONSTRAINT_DATA*>( aData ),
                aUnits );

    case ROUTING_WIDTH:
        return new DRC_RE_ROUTING_WIDTH_OVERLAY_PANEL(
                aParent,
                dynamic_cast<DRC_RE_ROUTING_WIDTH_CONSTRAINT_DATA*>( aData ),
                aUnits );

    case ROUTING_DIFF_PAIR:
        return new DRC_RE_ROUTING_DIFF_PAIR_OVERLAY_PANEL(
                aParent,
                dynamic_cast<DRC_RE_ROUTING_DIFF_PAIR_CONSTRAINT_DATA*>( aData ),
                aUnits );

    case MINIMUM_TEXT_HEIGHT_AND_THICKNESS:
        return new DRC_RE_MIN_TXT_HT_TH_OVERLAY_PANEL(
                aParent,
                dynamic_cast<DRC_RE_MINIMUM_TEXT_HEIGHT_THICKNESS_CONSTRAINT_DATA*>( aData ),
                aUnits );

    case ABSOLUTE_LENGTH:
        return new DRC_RE_ABS_LENGTH_TWO_OVERLAY_PANEL(
                aParent,
                dynamic_cast<DRC_RE_ABSOLUTE_LENGTH_TWO_CONSTRAINT_DATA*>( aData ),
                aUnits );

    case PERMITTED_LAYERS:
        return new DRC_RE_PERMITTED_LAYERS_OVERLAY_PANEL(
                aParent,
                dynamic_cast<DRC_RE_PERMITTED_LAYERS_CONSTRAINT_DATA*>( aData ) );

    case ALLOWED_ORIENTATION:
        return new DRC_RE_ALLOWED_ORIENTATION_OVERLAY_PANEL(
                aParent,
                dynamic_cast<DRC_RE_ALLOWED_ORIENTATION_CONSTRAINT_DATA*>( aData ) );

    default:
        if( DRC_RULE_EDITOR_UTILS::IsNumericInputType( aType ) )
        {
            return new DRC_RE_NUMERIC_INPUT_OVERLAY_PANEL(
                    aParent,
                    dynamic_cast<DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA*>( aData ),
                    aUnits );
        }
        else if( DRC_RULE_EDITOR_UTILS::IsBoolInputType( aType ) )
        {
            return new DRC_RE_BOOL_INPUT_OVERLAY_PANEL(
                    aParent,
                    dynamic_cast<DRC_RE_BOOL_INPUT_CONSTRAINT_DATA*>( aData ) );
        }

        break;
    }

    return nullptr;
}
