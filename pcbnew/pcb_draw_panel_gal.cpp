/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014-2017 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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

#include "pcb_draw_panel_gal.h"

#include <pcb_view.h>
#include <view/wx_view_controls.h>
#include <pcb_painter.h>
#include <drawing_sheet/ds_proxy_view_item.h>
#include <connectivity/connectivity_data.h>

#include <board.h>
#include <footprint.h>
#include <pcb_track.h>
#include <macros.h>
#include <pcb_generator.h>
#include <pcb_marker.h>
#include <pcb_point.h>
#include <pcb_base_frame.h>
#include <pcbnew_settings.h>
#include <ratsnest/ratsnest_data.h>
#include <ratsnest/ratsnest_view_item.h>
#include <pcb_board_outline.h>

#include <pgm_base.h>
#include <settings/settings_manager.h>
#include <confirm.h>
#include <progress_reporter.h>
#include <string_utils.h>

#include <gal/graphics_abstraction_layer.h>
#include <zoom_defines.h>

#include <functional>
#include <memory>
#include <thread>
#include <fmt/format.h>

using namespace std::placeholders;


const int GAL_LAYER_ORDER[] =
{
    LAYER_UI_START + 9,
    LAYER_UI_START + 8,
    LAYER_UI_START + 7,
    LAYER_UI_START + 6,
    LAYER_UI_START + 5,
    LAYER_UI_START + 4,
    LAYER_UI_START + 3,
    LAYER_UI_START + 2,
    LAYER_UI_START + 1,
    LAYER_UI_START,

    LAYER_GP_OVERLAY,
    LAYER_SELECT_OVERLAY,
    LAYER_CONFLICTS_SHADOW,

    LAYER_DRC_ERROR, LAYER_DRC_WARNING, LAYER_DRC_EXCLUSION, LAYER_MARKER_SHADOWS, LAYER_DRC_SHAPES,
    LAYER_PAD_NETNAMES, LAYER_VIA_NETNAMES,
    Dwgs_User,  ZONE_LAYER_FOR( Dwgs_User ), POINT_LAYER_FOR( Dwgs_User ),
    Cmts_User,  ZONE_LAYER_FOR( Cmts_User ), POINT_LAYER_FOR( Cmts_User ),
    Eco1_User,  ZONE_LAYER_FOR( Eco1_User ), POINT_LAYER_FOR( Eco1_User ),
    Eco2_User,  ZONE_LAYER_FOR( Eco2_User ), POINT_LAYER_FOR( Eco2_User ),
    Edge_Cuts,  ZONE_LAYER_FOR( Edge_Cuts ), POINT_LAYER_FOR( Edge_Cuts ),
    Margin,     ZONE_LAYER_FOR( Margin ),    POINT_LAYER_FOR( Margin ),

    User_1,     ZONE_LAYER_FOR( User_1 ),    POINT_LAYER_FOR( User_1 ),
    User_2,     ZONE_LAYER_FOR( User_2 ),    POINT_LAYER_FOR( User_2 ),
    User_3,     ZONE_LAYER_FOR( User_3 ),    POINT_LAYER_FOR( User_3 ),
    User_4,     ZONE_LAYER_FOR( User_4 ),    POINT_LAYER_FOR( User_4 ),
    User_5,     ZONE_LAYER_FOR( User_5 ),    POINT_LAYER_FOR( User_5 ),
    User_6,     ZONE_LAYER_FOR( User_6 ),    POINT_LAYER_FOR( User_6 ),
    User_7,     ZONE_LAYER_FOR( User_7 ),    POINT_LAYER_FOR( User_7 ),
    User_8,     ZONE_LAYER_FOR( User_8 ),    POINT_LAYER_FOR( User_8 ),
    User_9,     ZONE_LAYER_FOR( User_9 ),    POINT_LAYER_FOR( User_9 ),
    User_10,    ZONE_LAYER_FOR( User_10 ),   POINT_LAYER_FOR( User_10 ),
    User_11,    ZONE_LAYER_FOR( User_11 ),   POINT_LAYER_FOR( User_11 ),
    User_12,    ZONE_LAYER_FOR( User_12 ),   POINT_LAYER_FOR( User_12 ),
    User_13,    ZONE_LAYER_FOR( User_13 ),   POINT_LAYER_FOR( User_13 ),
    User_14,    ZONE_LAYER_FOR( User_14 ),   POINT_LAYER_FOR( User_14 ),
    User_15,    ZONE_LAYER_FOR( User_15 ),   POINT_LAYER_FOR( User_15 ),
    User_16,    ZONE_LAYER_FOR( User_16 ),   POINT_LAYER_FOR( User_16 ),
    User_17,    ZONE_LAYER_FOR( User_17 ),   POINT_LAYER_FOR( User_17 ),
    User_18,    ZONE_LAYER_FOR( User_18 ),   POINT_LAYER_FOR( User_18 ),
    User_19,    ZONE_LAYER_FOR( User_19 ),   POINT_LAYER_FOR( User_19 ),
    User_20,    ZONE_LAYER_FOR( User_20 ),   POINT_LAYER_FOR( User_20 ),
    User_21,    ZONE_LAYER_FOR( User_21 ),   POINT_LAYER_FOR( User_21 ),
    User_22,    ZONE_LAYER_FOR( User_22 ),   POINT_LAYER_FOR( User_22 ),
    User_23,    ZONE_LAYER_FOR( User_23 ),   POINT_LAYER_FOR( User_23 ),
    User_24,    ZONE_LAYER_FOR( User_24 ),   POINT_LAYER_FOR( User_24 ),
    User_25,    ZONE_LAYER_FOR( User_25 ),   POINT_LAYER_FOR( User_25 ),
    User_26,    ZONE_LAYER_FOR( User_26 ),   POINT_LAYER_FOR( User_26 ),
    User_27,    ZONE_LAYER_FOR( User_27 ),   POINT_LAYER_FOR( User_27 ),
    User_28,    ZONE_LAYER_FOR( User_28 ),   POINT_LAYER_FOR( User_28 ),
    User_29,    ZONE_LAYER_FOR( User_29 ),   POINT_LAYER_FOR( User_29 ),
    User_30,    ZONE_LAYER_FOR( User_30 ),   POINT_LAYER_FOR( User_30 ),
    User_31,    ZONE_LAYER_FOR( User_31 ),   POINT_LAYER_FOR( User_31 ),
    User_32,    ZONE_LAYER_FOR( User_32 ),   POINT_LAYER_FOR( User_32 ),
    User_33,    ZONE_LAYER_FOR( User_33 ),   POINT_LAYER_FOR( User_33 ),
    User_34,    ZONE_LAYER_FOR( User_34 ),   POINT_LAYER_FOR( User_34 ),
    User_35,    ZONE_LAYER_FOR( User_35 ),   POINT_LAYER_FOR( User_35 ),
    User_36,    ZONE_LAYER_FOR( User_36 ),   POINT_LAYER_FOR( User_36 ),
    User_37,    ZONE_LAYER_FOR( User_37 ),   POINT_LAYER_FOR( User_37 ),
    User_38,    ZONE_LAYER_FOR( User_38 ),   POINT_LAYER_FOR( User_38 ),
    User_39,    ZONE_LAYER_FOR( User_39 ),   POINT_LAYER_FOR( User_39 ),
    User_40,    ZONE_LAYER_FOR( User_40 ),   POINT_LAYER_FOR( User_40 ),
    User_41,    ZONE_LAYER_FOR( User_41 ),   POINT_LAYER_FOR( User_41 ),
    User_42,    ZONE_LAYER_FOR( User_42 ),   POINT_LAYER_FOR( User_42 ),
    User_43,    ZONE_LAYER_FOR( User_43 ),   POINT_LAYER_FOR( User_43 ),
    User_44,    ZONE_LAYER_FOR( User_44 ),   POINT_LAYER_FOR( User_44 ),
    User_45,    ZONE_LAYER_FOR( User_45 ),   POINT_LAYER_FOR( User_45 ),

    POINT_LAYER_FOR( F_Cu ),

    LAYER_FP_TEXT, LAYER_FP_REFERENCES, LAYER_FP_VALUES,

    LAYER_RATSNEST,
    LAYER_ANCHOR,
    LAYER_POINTS,
    LAYER_LOCKED_ITEM_SHADOW,
    LAYER_VIA_HOLES, LAYER_VIA_HOLEWALLS,
    LAYER_PAD_PLATEDHOLES, LAYER_PAD_HOLEWALLS, LAYER_NON_PLATEDHOLES,
    LAYER_VIA_THROUGH, LAYER_VIA_BLIND, LAYER_VIA_BURIED, LAYER_VIA_MICROVIA,

    LAYER_PAD_FR_NETNAMES,
    NETNAMES_LAYER_INDEX( F_Cu ),
    PAD_COPPER_LAYER_FOR( F_Cu ),
    VIA_COPPER_LAYER_FOR( F_Cu ),
    CLEARANCE_LAYER_FOR( F_Cu ),
    // POINT_LAYER_FOR( F_Cu ),
    F_Cu, ZONE_LAYER_FOR( F_Cu ),
    F_Mask, ZONE_LAYER_FOR( F_Mask ),
    F_SilkS, ZONE_LAYER_FOR( F_SilkS ),
    F_Paste, ZONE_LAYER_FOR( F_Paste ),
    F_Adhes, ZONE_LAYER_FOR( F_Adhes ),
    F_CrtYd, ZONE_LAYER_FOR( F_CrtYd ),
    F_Fab, ZONE_LAYER_FOR( F_Fab ),

    NETNAMES_LAYER_INDEX( In1_Cu ), PAD_COPPER_LAYER_FOR( In1_Cu ),
    VIA_COPPER_LAYER_FOR( In1_Cu ), CLEARANCE_LAYER_FOR( In1_Cu ),
    POINT_LAYER_FOR( In1_Cu ),
    In1_Cu,   ZONE_LAYER_FOR( In1_Cu ),
    NETNAMES_LAYER_INDEX( In2_Cu ), PAD_COPPER_LAYER_FOR( In2_Cu ),
    VIA_COPPER_LAYER_FOR( In2_Cu ), CLEARANCE_LAYER_FOR( In2_Cu ),
    POINT_LAYER_FOR( In2_Cu ),
    In2_Cu,   ZONE_LAYER_FOR( In2_Cu ),
    NETNAMES_LAYER_INDEX( In3_Cu ), PAD_COPPER_LAYER_FOR( In3_Cu ),
    VIA_COPPER_LAYER_FOR( In3_Cu ), CLEARANCE_LAYER_FOR( In3_Cu ),
    POINT_LAYER_FOR( In3_Cu ),
    In3_Cu,   ZONE_LAYER_FOR( In3_Cu ),
    NETNAMES_LAYER_INDEX( In4_Cu ), PAD_COPPER_LAYER_FOR( In4_Cu ),
    VIA_COPPER_LAYER_FOR( In4_Cu ), CLEARANCE_LAYER_FOR( In4_Cu ),
    POINT_LAYER_FOR( In4_Cu ),
    In4_Cu,   ZONE_LAYER_FOR( In4_Cu ),
    NETNAMES_LAYER_INDEX( In5_Cu ), PAD_COPPER_LAYER_FOR( In5_Cu ),
    VIA_COPPER_LAYER_FOR( In5_Cu ), CLEARANCE_LAYER_FOR( In5_Cu ),
    POINT_LAYER_FOR( In5_Cu ),
    In5_Cu,   ZONE_LAYER_FOR( In5_Cu ),
    NETNAMES_LAYER_INDEX( In6_Cu ), PAD_COPPER_LAYER_FOR( In6_Cu ),
    VIA_COPPER_LAYER_FOR( In6_Cu ), CLEARANCE_LAYER_FOR( In6_Cu ),
    POINT_LAYER_FOR( In6_Cu ),
    In6_Cu,   ZONE_LAYER_FOR( In6_Cu ),
    NETNAMES_LAYER_INDEX( In7_Cu ), PAD_COPPER_LAYER_FOR( In7_Cu ),
    VIA_COPPER_LAYER_FOR( In7_Cu ), CLEARANCE_LAYER_FOR( In7_Cu ),
    POINT_LAYER_FOR( In7_Cu ),
    In7_Cu,   ZONE_LAYER_FOR( In7_Cu ),
    NETNAMES_LAYER_INDEX( In8_Cu ), PAD_COPPER_LAYER_FOR( In8_Cu ),
    VIA_COPPER_LAYER_FOR( In8_Cu ), CLEARANCE_LAYER_FOR( In8_Cu ),
    POINT_LAYER_FOR( In8_Cu ),
    In8_Cu,   ZONE_LAYER_FOR( In8_Cu ),
    NETNAMES_LAYER_INDEX( In9_Cu ), PAD_COPPER_LAYER_FOR( In9_Cu ),
    VIA_COPPER_LAYER_FOR( In9_Cu ), CLEARANCE_LAYER_FOR( In9_Cu ),
    POINT_LAYER_FOR( In9_Cu ),
    In9_Cu,   ZONE_LAYER_FOR( In9_Cu ),
    NETNAMES_LAYER_INDEX( In10_Cu ), PAD_COPPER_LAYER_FOR( In10_Cu ),
    VIA_COPPER_LAYER_FOR( In10_Cu ), CLEARANCE_LAYER_FOR( In10_Cu ),
    POINT_LAYER_FOR( In10_Cu ),
    In10_Cu,  ZONE_LAYER_FOR( In10_Cu ),
    NETNAMES_LAYER_INDEX( In11_Cu ), PAD_COPPER_LAYER_FOR( In11_Cu ),
    VIA_COPPER_LAYER_FOR( In11_Cu ), CLEARANCE_LAYER_FOR( In11_Cu ),
    POINT_LAYER_FOR( In11_Cu ),
    In11_Cu,  ZONE_LAYER_FOR( In11_Cu ),
    NETNAMES_LAYER_INDEX( In12_Cu ), PAD_COPPER_LAYER_FOR( In12_Cu ),
    VIA_COPPER_LAYER_FOR( In12_Cu ), CLEARANCE_LAYER_FOR( In12_Cu ),
    POINT_LAYER_FOR( In12_Cu ),
    In12_Cu,  ZONE_LAYER_FOR( In12_Cu ),
    NETNAMES_LAYER_INDEX( In13_Cu ), PAD_COPPER_LAYER_FOR( In13_Cu ),
    VIA_COPPER_LAYER_FOR( In13_Cu ), CLEARANCE_LAYER_FOR( In13_Cu ),
    POINT_LAYER_FOR( In13_Cu ),
    In13_Cu,  ZONE_LAYER_FOR( In13_Cu ),
    NETNAMES_LAYER_INDEX( In14_Cu ), PAD_COPPER_LAYER_FOR( In14_Cu ),
    VIA_COPPER_LAYER_FOR( In14_Cu ), CLEARANCE_LAYER_FOR( In14_Cu ),
    POINT_LAYER_FOR( In14_Cu ),
    In14_Cu,  ZONE_LAYER_FOR( In14_Cu ),
    NETNAMES_LAYER_INDEX( In15_Cu ), PAD_COPPER_LAYER_FOR( In15_Cu ),
    VIA_COPPER_LAYER_FOR( In15_Cu ), CLEARANCE_LAYER_FOR( In15_Cu ),
    POINT_LAYER_FOR( In15_Cu ),
    In15_Cu,  ZONE_LAYER_FOR( In15_Cu ),
    NETNAMES_LAYER_INDEX( In16_Cu ), PAD_COPPER_LAYER_FOR( In16_Cu ),
    VIA_COPPER_LAYER_FOR( In16_Cu ), CLEARANCE_LAYER_FOR( In16_Cu ),
    POINT_LAYER_FOR( In16_Cu ),
    In16_Cu,  ZONE_LAYER_FOR( In16_Cu ),
    NETNAMES_LAYER_INDEX( In17_Cu ), PAD_COPPER_LAYER_FOR( In17_Cu ),
    VIA_COPPER_LAYER_FOR( In17_Cu ), CLEARANCE_LAYER_FOR( In17_Cu ),
    POINT_LAYER_FOR( In17_Cu ),
    In17_Cu,  ZONE_LAYER_FOR( In17_Cu ),
    NETNAMES_LAYER_INDEX( In18_Cu ), PAD_COPPER_LAYER_FOR( In18_Cu ),
    VIA_COPPER_LAYER_FOR( In18_Cu ), CLEARANCE_LAYER_FOR( In18_Cu ),
    POINT_LAYER_FOR( In18_Cu ),
    In18_Cu,  ZONE_LAYER_FOR( In18_Cu ),
    NETNAMES_LAYER_INDEX( In19_Cu ), PAD_COPPER_LAYER_FOR( In19_Cu ),
    VIA_COPPER_LAYER_FOR( In19_Cu ), CLEARANCE_LAYER_FOR( In19_Cu ),
    POINT_LAYER_FOR( In19_Cu ),
    In19_Cu,  ZONE_LAYER_FOR( In19_Cu ),
    NETNAMES_LAYER_INDEX( In20_Cu ), PAD_COPPER_LAYER_FOR( In20_Cu ),
    VIA_COPPER_LAYER_FOR( In20_Cu ), CLEARANCE_LAYER_FOR( In20_Cu ),
    POINT_LAYER_FOR( In20_Cu ),
    In20_Cu,  ZONE_LAYER_FOR( In20_Cu ),
    NETNAMES_LAYER_INDEX( In21_Cu ), PAD_COPPER_LAYER_FOR( In21_Cu ),
    VIA_COPPER_LAYER_FOR( In21_Cu ), CLEARANCE_LAYER_FOR( In21_Cu ),
    POINT_LAYER_FOR( In21_Cu ),
    In21_Cu,  ZONE_LAYER_FOR( In21_Cu ),
    NETNAMES_LAYER_INDEX( In22_Cu ), PAD_COPPER_LAYER_FOR( In22_Cu ),
    VIA_COPPER_LAYER_FOR( In22_Cu ), CLEARANCE_LAYER_FOR( In22_Cu ),
    POINT_LAYER_FOR( In22_Cu ),
    In22_Cu,  ZONE_LAYER_FOR( In22_Cu ),
    NETNAMES_LAYER_INDEX( In23_Cu ), PAD_COPPER_LAYER_FOR( In23_Cu ),
    VIA_COPPER_LAYER_FOR( In23_Cu ), CLEARANCE_LAYER_FOR( In23_Cu ),
    POINT_LAYER_FOR( In23_Cu ),
    In23_Cu,  ZONE_LAYER_FOR( In23_Cu ),
    NETNAMES_LAYER_INDEX( In24_Cu ), PAD_COPPER_LAYER_FOR( In24_Cu ),
    VIA_COPPER_LAYER_FOR( In24_Cu ), CLEARANCE_LAYER_FOR( In24_Cu ),
    POINT_LAYER_FOR( In24_Cu ),
    In24_Cu,  ZONE_LAYER_FOR( In24_Cu ),
    NETNAMES_LAYER_INDEX( In25_Cu ), PAD_COPPER_LAYER_FOR( In25_Cu ),
    VIA_COPPER_LAYER_FOR( In25_Cu ), CLEARANCE_LAYER_FOR( In25_Cu ),
    POINT_LAYER_FOR( In25_Cu ),
    In25_Cu,  ZONE_LAYER_FOR( In25_Cu ),
    NETNAMES_LAYER_INDEX( In26_Cu ), PAD_COPPER_LAYER_FOR( In26_Cu ),
    VIA_COPPER_LAYER_FOR( In26_Cu ), CLEARANCE_LAYER_FOR( In26_Cu ),
    POINT_LAYER_FOR( In26_Cu ),
    In26_Cu,  ZONE_LAYER_FOR( In26_Cu ),
    NETNAMES_LAYER_INDEX( In27_Cu ), PAD_COPPER_LAYER_FOR( In27_Cu ),
    VIA_COPPER_LAYER_FOR( In27_Cu ), CLEARANCE_LAYER_FOR( In27_Cu ),
    POINT_LAYER_FOR( In27_Cu ),
    In27_Cu,  ZONE_LAYER_FOR( In27_Cu ),
    NETNAMES_LAYER_INDEX( In28_Cu ), PAD_COPPER_LAYER_FOR( In28_Cu ),
    VIA_COPPER_LAYER_FOR( In28_Cu ), CLEARANCE_LAYER_FOR( In28_Cu ),
    POINT_LAYER_FOR( In28_Cu ),
    In28_Cu,  ZONE_LAYER_FOR( In28_Cu ),
    NETNAMES_LAYER_INDEX( In29_Cu ), PAD_COPPER_LAYER_FOR( In29_Cu ),
    VIA_COPPER_LAYER_FOR( In29_Cu ), CLEARANCE_LAYER_FOR( In29_Cu ),
    POINT_LAYER_FOR( In29_Cu ),
    In29_Cu,  ZONE_LAYER_FOR( In29_Cu ),
    NETNAMES_LAYER_INDEX( In30_Cu ), PAD_COPPER_LAYER_FOR( In30_Cu ),
    VIA_COPPER_LAYER_FOR( In30_Cu ), CLEARANCE_LAYER_FOR( In30_Cu ),
    POINT_LAYER_FOR( In30_Cu ),
    In30_Cu,  ZONE_LAYER_FOR( In30_Cu ),

    LAYER_PAD_BK_NETNAMES,
    NETNAMES_LAYER_INDEX( B_Cu ),
    PAD_COPPER_LAYER_FOR( B_Cu ),
    VIA_COPPER_LAYER_FOR( B_Cu ),
    CLEARANCE_LAYER_FOR( B_Cu ),
    POINT_LAYER_FOR( B_Cu ),
    B_Cu, ZONE_LAYER_FOR( B_Cu ),
    B_Mask, ZONE_LAYER_FOR( B_Mask ),
    B_SilkS, ZONE_LAYER_FOR( B_SilkS ),
    B_Paste, ZONE_LAYER_FOR( B_Paste ),
    B_Adhes, ZONE_LAYER_FOR( B_Adhes ),
    B_CrtYd, ZONE_LAYER_FOR( B_CrtYd ),
    B_Fab, ZONE_LAYER_FOR( B_Fab ),

    BITMAP_LAYER_FOR( Dwgs_User ),
    BITMAP_LAYER_FOR( Cmts_User ),
    BITMAP_LAYER_FOR( Eco1_User ), BITMAP_LAYER_FOR( Eco2_User ),
    BITMAP_LAYER_FOR( Edge_Cuts ), BITMAP_LAYER_FOR( Margin ),

    BITMAP_LAYER_FOR( User_1 ),
    BITMAP_LAYER_FOR( User_2 ),
    BITMAP_LAYER_FOR( User_3 ),
    BITMAP_LAYER_FOR( User_4 ),
    BITMAP_LAYER_FOR( User_5 ),
    BITMAP_LAYER_FOR( User_6 ),
    BITMAP_LAYER_FOR( User_7 ),
    BITMAP_LAYER_FOR( User_8 ),
    BITMAP_LAYER_FOR( User_9 ),
    BITMAP_LAYER_FOR( User_10 ),
    BITMAP_LAYER_FOR( User_11 ),
    BITMAP_LAYER_FOR( User_12 ),
    BITMAP_LAYER_FOR( User_13 ),
    BITMAP_LAYER_FOR( User_14 ),
    BITMAP_LAYER_FOR( User_15 ),
    BITMAP_LAYER_FOR( User_16 ),
    BITMAP_LAYER_FOR( User_17 ),
    BITMAP_LAYER_FOR( User_18 ),
    BITMAP_LAYER_FOR( User_19 ),
    BITMAP_LAYER_FOR( User_20 ),
    BITMAP_LAYER_FOR( User_21 ),
    BITMAP_LAYER_FOR( User_22 ),
    BITMAP_LAYER_FOR( User_23 ),
    BITMAP_LAYER_FOR( User_24 ),
    BITMAP_LAYER_FOR( User_25 ),
    BITMAP_LAYER_FOR( User_26 ),
    BITMAP_LAYER_FOR( User_27 ),
    BITMAP_LAYER_FOR( User_28 ),
    BITMAP_LAYER_FOR( User_29 ),
    BITMAP_LAYER_FOR( User_30 ),
    BITMAP_LAYER_FOR( User_31 ),
    BITMAP_LAYER_FOR( User_32 ),
    BITMAP_LAYER_FOR( User_33 ),
    BITMAP_LAYER_FOR( User_34 ),
    BITMAP_LAYER_FOR( User_35 ),
    BITMAP_LAYER_FOR( User_36 ),
    BITMAP_LAYER_FOR( User_37 ),
    BITMAP_LAYER_FOR( User_38 ),
    BITMAP_LAYER_FOR( User_39 ),
    BITMAP_LAYER_FOR( User_40 ),
    BITMAP_LAYER_FOR( User_41 ),
    BITMAP_LAYER_FOR( User_42 ),
    BITMAP_LAYER_FOR( User_43 ),
    BITMAP_LAYER_FOR( User_44 ),
    BITMAP_LAYER_FOR( User_45 ),

    BITMAP_LAYER_FOR( F_Cu ),
    BITMAP_LAYER_FOR( F_Mask ),
    BITMAP_LAYER_FOR( F_SilkS ),
    BITMAP_LAYER_FOR( F_Paste ),
    BITMAP_LAYER_FOR( F_Adhes ),
    BITMAP_LAYER_FOR( F_CrtYd ),
    BITMAP_LAYER_FOR( F_Fab ),

    BITMAP_LAYER_FOR( In1_Cu ),
    BITMAP_LAYER_FOR( In2_Cu ),
    BITMAP_LAYER_FOR( In3_Cu ),
    BITMAP_LAYER_FOR( In4_Cu ),
    BITMAP_LAYER_FOR( In5_Cu ),
    BITMAP_LAYER_FOR( In6_Cu ),
    BITMAP_LAYER_FOR( In7_Cu ),
    BITMAP_LAYER_FOR( In8_Cu ),
    BITMAP_LAYER_FOR( In9_Cu ),
    BITMAP_LAYER_FOR( In10_Cu ),
    BITMAP_LAYER_FOR( In11_Cu ),
    BITMAP_LAYER_FOR( In12_Cu ),
    BITMAP_LAYER_FOR( In13_Cu ),
    BITMAP_LAYER_FOR( In14_Cu ),
    BITMAP_LAYER_FOR( In15_Cu ),
    BITMAP_LAYER_FOR( In16_Cu ),
    BITMAP_LAYER_FOR( In17_Cu ),
    BITMAP_LAYER_FOR( In18_Cu ),
    BITMAP_LAYER_FOR( In19_Cu ),
    BITMAP_LAYER_FOR( In20_Cu ),
    BITMAP_LAYER_FOR( In21_Cu ),
    BITMAP_LAYER_FOR( In22_Cu ),
    BITMAP_LAYER_FOR( In23_Cu ),
    BITMAP_LAYER_FOR( In24_Cu ),
    BITMAP_LAYER_FOR( In25_Cu ),
    BITMAP_LAYER_FOR( In26_Cu ),
    BITMAP_LAYER_FOR( In27_Cu ),
    BITMAP_LAYER_FOR( In28_Cu ),
    BITMAP_LAYER_FOR( In29_Cu ),
    BITMAP_LAYER_FOR( In30_Cu ),

    BITMAP_LAYER_FOR( B_Cu ),
    BITMAP_LAYER_FOR( B_Mask ),
    BITMAP_LAYER_FOR( B_SilkS ),
    BITMAP_LAYER_FOR( B_Paste ),
    BITMAP_LAYER_FOR( B_Adhes ),
    BITMAP_LAYER_FOR( B_CrtYd ),
    BITMAP_LAYER_FOR( B_Fab ),

    LAYER_BOARD_OUTLINE_AREA,

    LAYER_DRAWINGSHEET
};


PCB_DRAW_PANEL_GAL::PCB_DRAW_PANEL_GAL( wxWindow* aParentWindow, wxWindowID aWindowId,
                                        const wxPoint& aPosition, const wxSize& aSize,
                                        KIGFX::GAL_DISPLAY_OPTIONS& aOptions, GAL_TYPE aGalType ) :
        EDA_DRAW_PANEL_GAL( aParentWindow, aWindowId, aPosition, aSize, aOptions, aGalType )
{
    m_view = new KIGFX::PCB_VIEW();
    m_view->SetGAL( m_gal );

    FRAME_T frameType = FRAME_FOOTPRINT_PREVIEW;

    if( EDA_BASE_FRAME* frame = dynamic_cast<EDA_BASE_FRAME*>( aParentWindow ) )
        frameType = frame->GetFrameType();

    m_painter = std::make_unique<KIGFX::PCB_PAINTER>( m_gal, frameType );
    m_view->SetPainter( m_painter.get() );

    // This fixes the zoom in and zoom out limits:
    m_view->SetScaleLimits( ZOOM_MAX_LIMIT_PCBNEW, ZOOM_MIN_LIMIT_PCBNEW );

    setDefaultLayerOrder();
    setDefaultLayerDeps();

    // View controls is the first in the event handler chain, so the Tool Framework operates
    // on updated viewport data.
    m_viewControls = new KIGFX::WX_VIEW_CONTROLS( m_view, this );

    // Load display options (such as filled/outline display of items).
    // Can be made only if the parent window is an EDA_DRAW_FRAME (or a derived class)
    // which is not always the case (namely when it is used from a wxDialog like the pad editor)
    if( !IsDialogPreview() )
    {
        KIGFX::PCB_VIEW* view = static_cast<KIGFX::PCB_VIEW*>( m_view );
        PCB_BASE_FRAME*  frame = dynamic_cast<PCB_BASE_FRAME*>( GetParentEDAFrame() );

        if( frame )
            view->UpdateDisplayOptions( frame->GetDisplayOptions() );
    }
}


PCB_DRAW_PANEL_GAL::~PCB_DRAW_PANEL_GAL()
{
}


void PCB_DRAW_PANEL_GAL::DisplayBoard( BOARD* aBoard, PROGRESS_REPORTER* aReporter )
{
    m_view->Clear();

    aBoard->CacheTriangulation( aReporter );

    if( m_drawingSheet )
        m_drawingSheet->SetFileName( TO_UTF8( aBoard->GetFileName() ) );

    // Load drawings
    for( BOARD_ITEM* drawing : aBoard->Drawings() )
        m_view->Add( drawing );

    // Load tracks
    for( PCB_TRACK* track : aBoard->Tracks() )
        m_view->Add( track );

    // Load footprints and its additional elements
    for( FOOTPRINT* footprint : aBoard->Footprints() )
        m_view->Add( footprint );

    // DRC markers
    for( PCB_MARKER* marker : aBoard->Markers() )
        m_view->Add( marker );

    // Load points
    for( PCB_POINT* point : aBoard->Points() )
        m_view->Add( point );

    // Load zones
    for( ZONE* zone : aBoard->Zones() )
        m_view->Add( zone );

    for( PCB_GENERATOR* generator : aBoard->Generators() )
        m_view->Add( generator );

    aBoard->UpdateBoardOutline();
    m_view->Add( aBoard->BoardOutline() );

    // Ratsnest
    if( !aBoard->IsFootprintHolder() )
    {
        m_ratsnest = std::make_unique<RATSNEST_VIEW_ITEM>( aBoard->GetConnectivity() );
        m_view->Add( m_ratsnest.get() );
    }
}


void PCB_DRAW_PANEL_GAL::SetDrawingSheet( DS_PROXY_VIEW_ITEM* aDrawingSheet )
{
    m_drawingSheet.reset( aDrawingSheet );
    m_view->Add( m_drawingSheet.get() );
}


void PCB_DRAW_PANEL_GAL::UpdateColors()
{
    COLOR_SETTINGS* cs = ::GetColorSettings( DEFAULT_THEME );
    PCB_BASE_FRAME* frame = dynamic_cast<PCB_BASE_FRAME*>( GetParentEDAFrame() );

    if( frame )
        cs = frame->GetColorSettings();
    else if( PCBNEW_SETTINGS* cfg = GetAppSettings<PCBNEW_SETTINGS>( "pcbnew" ) )
        cs = ::GetColorSettings( cfg->m_ColorTheme );

    wxCHECK_RET( cs, wxT( "null COLOR_SETTINGS" ) );

    auto rs = static_cast<KIGFX::PCB_RENDER_SETTINGS*>( m_view->GetPainter()->GetSettings() );
    rs->LoadColors( cs );

    m_gal->SetGridColor( cs->GetColor( LAYER_GRID ) );
    m_gal->SetAxesColor( cs->GetColor( LAYER_GRID_AXES ) );
    m_gal->SetCursorColor( cs->GetColor( LAYER_CURSOR ) );
}


void PCB_DRAW_PANEL_GAL::SetHighContrastLayer( PCB_LAYER_ID aLayer )
{
    // Set display settings for high contrast mode
    KIGFX::RENDER_SETTINGS* rSettings = m_view->GetPainter()->GetSettings();

    SetTopLayer( aLayer );
    rSettings->SetActiveLayer( aLayer );

    rSettings->ClearHighContrastLayers();
    rSettings->SetLayerIsHighContrast( aLayer );

    if( IsCopperLayer( aLayer ) )
    {
        // Bring some other layers to the front in case of copper layers and make them colored
        // fixme do not like the idea of storing the list of layers here,
        // should be done in some other way I guess..
        int layers[] = {
                LAYER_CONFLICTS_SHADOW,
                GetNetnameLayer( aLayer ),
                LAYER_PAD_FR_NETNAMES, LAYER_PAD_BK_NETNAMES, LAYER_PAD_NETNAMES,
                LAYER_VIA_NETNAMES,
                PAD_COPPER_LAYER_FOR( aLayer ),
                VIA_COPPER_LAYER_FOR( aLayer ),
                ZONE_LAYER_FOR( aLayer ),
                BITMAP_LAYER_FOR( aLayer ),
                POINT_LAYER_FOR( aLayer ),
                LAYER_PAD_PLATEDHOLES, LAYER_PAD_HOLEWALLS, LAYER_NON_PLATEDHOLES,
                LAYER_VIA_THROUGH, LAYER_VIA_BLIND, LAYER_VIA_BURIED, LAYER_VIA_MICROVIA, LAYER_VIA_HOLES,
                LAYER_VIA_HOLEWALLS,
                LAYER_DRC_ERROR, LAYER_DRC_WARNING, LAYER_DRC_EXCLUSION, LAYER_MARKER_SHADOWS,
                LAYER_DRC_SHAPES,
                LAYER_SELECT_OVERLAY, LAYER_GP_OVERLAY,
                LAYER_RATSNEST, LAYER_CURSOR,
                LAYER_ANCHOR,
                LAYER_LOCKED_ITEM_SHADOW
        };

        for( int i : layers )
            rSettings->SetLayerIsHighContrast( i );

        for( int i = LAYER_UI_START; i < LAYER_UI_END; ++i )
            rSettings->SetLayerIsHighContrast( i );

        // Pads should be shown too
        if( aLayer == B_Cu )
        {
            rSettings->SetLayerIsHighContrast( LAYER_FOOTPRINTS_BK );
        }
        else if( aLayer == F_Cu )
        {
            rSettings->SetLayerIsHighContrast( LAYER_FOOTPRINTS_FR );
        }
    }

    m_view->UpdateAllLayersColor();
}


void PCB_DRAW_PANEL_GAL::SetTopLayer( PCB_LAYER_ID aLayer )
{
    m_view->ClearTopLayers();
    setDefaultLayerOrder();
    m_view->SetTopLayer( aLayer );

    // Layers that should always have on-top attribute enabled
    const std::vector<int> layers = {
            LAYER_VIA_THROUGH, LAYER_VIA_BLIND, LAYER_VIA_BURIED, LAYER_VIA_MICROVIA, LAYER_VIA_HOLES,
            LAYER_VIA_HOLEWALLS,
            LAYER_PAD_PLATEDHOLES, LAYER_PAD_HOLEWALLS, LAYER_NON_PLATEDHOLES,
            LAYER_PAD_NETNAMES, LAYER_VIA_NETNAMES,
            LAYER_SELECT_OVERLAY, LAYER_GP_OVERLAY,
            LAYER_RATSNEST,
            LAYER_DRC_ERROR, LAYER_DRC_WARNING, LAYER_DRC_EXCLUSION, LAYER_MARKER_SHADOWS, LAYER_DRC_SHAPES,
            LAYER_CONFLICTS_SHADOW
    };

    for( auto layer : layers )
        m_view->SetTopLayer( layer );

    for( int i = LAYER_UI_START; i < LAYER_UI_END; i++ )
        m_view->SetTopLayer( i );

    // Extra layers that are brought to the top if a F.* or B.* is selected
    const std::vector<int> frontLayers = {
        F_Cu, F_Adhes, F_Paste, F_SilkS, F_Mask, F_Fab, F_CrtYd,
        LAYER_PAD_FR_NETNAMES, NETNAMES_LAYER_INDEX( F_Cu )
    };

    const std::vector<int> backLayers = {
        B_Cu, B_Adhes, B_Paste, B_SilkS, B_Mask, B_Fab, B_CrtYd,
        LAYER_PAD_BK_NETNAMES, NETNAMES_LAYER_INDEX( B_Cu )
    };

    const std::vector<int>* extraLayers = nullptr;

    // Bring a few more extra layers to the top depending on the selected board side
    if( IsFrontLayer( aLayer ) )
        extraLayers = &frontLayers;
    else if( IsBackLayer( aLayer ) )
        extraLayers = &backLayers;

    if( extraLayers )
    {
        for( auto layer : *extraLayers )
        {
            m_view->SetTopLayer( layer );

            if( layer < PCB_LAYER_ID_COUNT )
            {
                m_view->SetTopLayer( ZONE_LAYER_FOR( layer ) );
                m_view->SetTopLayer( PAD_COPPER_LAYER_FOR( layer ) );
                m_view->SetTopLayer( VIA_COPPER_LAYER_FOR( layer ) );
                m_view->SetTopLayer( CLEARANCE_LAYER_FOR( layer ) );
                m_view->SetTopLayer( POINT_LAYER_FOR( layer ) );
            }
        }

        // Move the active layer to the top of the stack but below all the overlay layers
        if( !IsCopperLayer( aLayer ) )
        {
            m_view->SetLayerOrder( aLayer, m_view->GetLayerOrder( LAYER_MARKER_SHADOWS ) + 1 );
            m_view->SetLayerOrder( ZONE_LAYER_FOR( aLayer ),
                                   m_view->GetLayerOrder( LAYER_MARKER_SHADOWS ) + 2 );
            m_view->SetLayerOrder( POINT_LAYER_FOR( aLayer ),
                                   m_view->GetLayerOrder( LAYER_MARKER_SHADOWS ) + 3 );

            // Fix up pad and via netnames to be below.  This is hacky, we need a rethink
            // of layer ordering...
            m_view->SetLayerOrder( LAYER_PAD_NETNAMES,
                                   m_view->GetLayerOrder( LAYER_MARKER_SHADOWS ) + 4 );
            m_view->SetLayerOrder( LAYER_VIA_NETNAMES,
                                   m_view->GetLayerOrder( LAYER_MARKER_SHADOWS ) + 5 );
        }
    }

    if( IsCopperLayer( aLayer ) )
    {
        m_view->SetTopLayer( ZONE_LAYER_FOR( aLayer ) );
        m_view->SetTopLayer( PAD_COPPER_LAYER_FOR( aLayer ) );
        m_view->SetTopLayer( VIA_COPPER_LAYER_FOR( aLayer ) );
        m_view->SetTopLayer( CLEARANCE_LAYER_FOR( aLayer ) );

        // Display labels for copper layers on the top
        m_view->SetTopLayer( GetNetnameLayer( aLayer ) );
    }

    m_view->SetTopLayer( POINT_LAYER_FOR( aLayer ) );
    m_view->SetTopLayer( BITMAP_LAYER_FOR( aLayer ) );
    m_view->EnableTopLayer( true );
    m_view->UpdateAllLayersOrder();
}


void PCB_DRAW_PANEL_GAL::SyncLayersVisibility( const BOARD* aBoard )
{
    // Load layer & elements visibility settings
    for( int i = 0; i < PCB_LAYER_ID_COUNT; ++i )
        m_view->SetLayerVisible( i, aBoard->IsLayerVisible( PCB_LAYER_ID( i ) ) );

    for( GAL_LAYER_ID i = GAL_LAYER_ID_START; i < GAL_LAYER_ID_END; ++i )
        m_view->SetLayerVisible( i, aBoard->IsElementVisible( i ) );

    // Via layers controlled by dependencies
    m_view->SetLayerVisible( LAYER_VIA_MICROVIA, true );
    m_view->SetLayerVisible( LAYER_VIA_BLIND, true );
    m_view->SetLayerVisible( LAYER_VIA_BURIED, true );
    m_view->SetLayerVisible( LAYER_VIA_THROUGH, true );

    // Always enable netname layers, as their visibility is controlled by layer dependencies
    for( int i = NETNAMES_LAYER_ID_START; i < NETNAMES_LAYER_ID_END; ++i )
        m_view->SetLayerVisible( i, true );

    for( int i = LAYER_ZONE_START; i < LAYER_ZONE_END; i++ )
        m_view->SetLayerVisible( i, true );

    for( int i = LAYER_PAD_COPPER_START; i < LAYER_PAD_COPPER_END; i++ )
        m_view->SetLayerVisible( i, true );

    for( int i = LAYER_VIA_COPPER_START; i < LAYER_VIA_COPPER_END; i++ )
        m_view->SetLayerVisible( i, true );

    for( int i = LAYER_CLEARANCE_START; i < LAYER_CLEARANCE_END; i++ )
        m_view->SetLayerVisible( i, false );

    for( int i = LAYER_POINT_START; i < LAYER_POINT_END; i++ )
        m_view->SetLayerVisible( i, true );

    for( int i = LAYER_BITMAP_START; i < LAYER_BITMAP_END; i++ )
        m_view->SetLayerVisible( i, true );

    for( int i = LAYER_UI_START; i < LAYER_UI_END; i++ )
        m_view->SetLayerVisible( i, true );

    // Enable some layers that are GAL specific
    m_view->SetLayerVisible( LAYER_PAD_PLATEDHOLES, true );
    m_view->SetLayerVisible( LAYER_NON_PLATEDHOLES, true );
    m_view->SetLayerVisible( LAYER_PAD_HOLEWALLS, true );
    m_view->SetLayerVisible( LAYER_VIA_HOLES, true );
    m_view->SetLayerVisible( LAYER_VIA_HOLEWALLS, true );
    m_view->SetLayerVisible( LAYER_GP_OVERLAY, true );
    m_view->SetLayerVisible( LAYER_SELECT_OVERLAY, true );
    m_view->SetLayerVisible( LAYER_RATSNEST, true );
    m_view->SetLayerVisible( LAYER_MARKER_SHADOWS, true );
    m_view->SetLayerVisible( LAYER_DRC_SHAPES, true );
}


void PCB_DRAW_PANEL_GAL::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame,
                                          std::vector<MSG_PANEL_ITEM>& aList )
{
    BOARD*        board = static_cast<PCB_BASE_FRAME*>( GetParentEDAFrame() )->GetBoard();
    int           padCount = 0;
    int           viaCount = 0;
    int           trackSegmentCount = 0;
    std::set<int> netCodes;
    int           unconnected = (int) board->GetConnectivity()->GetUnconnectedCount( true );

    for( PCB_TRACK* item : board->Tracks() )
    {
        if( item->Type() == PCB_VIA_T )
            viaCount++;
        else
            trackSegmentCount++;

        if( item->GetNetCode() > 0 )
            netCodes.insert( item->GetNetCode() );
    }

    for( FOOTPRINT* footprint : board->Footprints() )
    {
        for( PAD* pad : footprint->Pads() )
        {
            padCount++;

            if( pad->GetNetCode() > 0 )
                netCodes.insert( pad->GetNetCode() );
        }
    }

    aList.emplace_back( _( "Pads" ), fmt::format( "{}", padCount ) );
    aList.emplace_back( _( "Vias" ), fmt::format( "{}", viaCount ) );
    aList.emplace_back( _( "Track Segments" ), fmt::format( "{}", trackSegmentCount ) );
    aList.emplace_back( _( "Nets" ), fmt::format( "{}", (int) netCodes.size() ) );
    aList.emplace_back( _( "Unrouted" ), fmt::format( "{}", unconnected ) );
}


void PCB_DRAW_PANEL_GAL::OnShow()
{
    PCB_BASE_FRAME* frame = nullptr;

    if( !IsDialogPreview() )
        frame = dynamic_cast<PCB_BASE_FRAME*>( GetParentEDAFrame() );

    try
    {
        // Check if the current rendering back end can be properly initialized
        m_view->UpdateItems();
    }
    catch( const std::runtime_error& e )
    {
        DisplayError( GetParent(), e.what() );

        // Use the fallback if we have one
        if( GAL_FALLBACK != m_backend )
        {
            SwitchBackend( GAL_FALLBACK );

            if( frame )
                frame->ActivateGalCanvas();
        }
    }

    if( frame )
    {
        SetTopLayer( frame->GetActiveLayer() );

        KIGFX::PCB_PAINTER* painter = static_cast<KIGFX::PCB_PAINTER*>( m_view->GetPainter() );
        KIGFX::PCB_RENDER_SETTINGS* settings = painter->GetSettings();

        settings->LoadDisplayOptions( frame->GetDisplayOptions() );
        settings->m_ForceShowFieldsWhenFPSelected = frame->GetPcbNewSettings()->m_Display.m_ForceShowFieldsWhenFPSelected;
    }
}


void PCB_DRAW_PANEL_GAL::setDefaultLayerOrder()
{
    for( int i = 0; (unsigned) i < sizeof( GAL_LAYER_ORDER ) / sizeof( int ); ++i )
    {
        int layer = GAL_LAYER_ORDER[i];
        wxASSERT( layer < KIGFX::VIEW::VIEW_MAX_LAYERS );

        // MW: Gross hack to make SetTopLayer bring the correct bitmap layer to
        // the top of the other bitmaps, but still below all the other layers
        if( layer >= LAYER_BITMAP_START && layer < LAYER_BITMAP_END )
            m_view->SetLayerOrder( layer, i - KIGFX::VIEW::TOP_LAYER_MODIFIER );
        else
            m_view->SetLayerOrder( layer, i );
    }
}


bool PCB_DRAW_PANEL_GAL::SwitchBackend( GAL_TYPE aGalType )
{
    bool rv = EDA_DRAW_PANEL_GAL::SwitchBackend( aGalType );
    setDefaultLayerDeps();
    m_gal->SetWorldUnitLength( 1e-9 /* 1 nm */ / 0.0254 /* 1 inch in meters */ );
    return rv;
}


void PCB_DRAW_PANEL_GAL::RedrawRatsnest()
{
    if( m_ratsnest )
        m_view->Update( m_ratsnest.get() );
}


BOX2I PCB_DRAW_PANEL_GAL::GetDefaultViewBBox() const
{
    if( m_drawingSheet && m_view->IsLayerVisible( LAYER_DRAWINGSHEET ) )
        return m_drawingSheet->ViewBBox();

    return BOX2I();
}


void PCB_DRAW_PANEL_GAL::setDefaultLayerDeps()
{
    // caching makes no sense for Cairo and other software renderers
    auto target = m_backend == GAL_TYPE_OPENGL ? KIGFX::TARGET_CACHED : KIGFX::TARGET_NONCACHED;

    for( int i = 0; i < KIGFX::VIEW::VIEW_MAX_LAYERS; i++ )
        m_view->SetLayerTarget( i, target );

    for( int i = 0; (unsigned) i < sizeof( GAL_LAYER_ORDER ) / sizeof( int ); ++i )
    {
        int layer = GAL_LAYER_ORDER[i];
        wxASSERT( layer < KIGFX::VIEW::VIEW_MAX_LAYERS );

        // Set layer display dependencies & targets
        if( IsCopperLayer( layer ) )
        {
            m_view->SetRequired( ZONE_LAYER_FOR( layer ), layer );
            m_view->SetRequired( PAD_COPPER_LAYER_FOR( layer ), layer );
            m_view->SetRequired( VIA_COPPER_LAYER_FOR( layer ), layer );
            m_view->SetRequired( CLEARANCE_LAYER_FOR( layer ), layer );
            m_view->SetRequired( POINT_LAYER_FOR( layer ), layer );

            m_view->SetRequired( BITMAP_LAYER_FOR( layer ), layer );
            m_view->SetLayerTarget( BITMAP_LAYER_FOR( layer ), KIGFX::TARGET_NONCACHED );
            m_view->SetRequired( GetNetnameLayer( layer ), layer );
        }
        else if( IsNonCopperLayer( layer ) )
        {
            m_view->SetRequired( POINT_LAYER_FOR( layer ), layer );
            m_view->SetRequired( ZONE_LAYER_FOR( layer ), layer );
            m_view->SetLayerTarget( BITMAP_LAYER_FOR( layer ), KIGFX::TARGET_NONCACHED );
            m_view->SetRequired( BITMAP_LAYER_FOR( layer ), layer );
        }
        else if( IsNetnameLayer( layer ) )
        {
            m_view->SetLayerDisplayOnly( layer );
        }
    }

    m_view->SetLayerTarget( LAYER_ANCHOR, KIGFX::TARGET_NONCACHED );
    m_view->SetLayerDisplayOnly( LAYER_ANCHOR );

    // Use TARGET_OVERLAY for LAYER_CONFLICTS_SHADOW, it is for items
    // that may change while the view stays the same.
    m_view->SetLayerTarget( LAYER_CONFLICTS_SHADOW, KIGFX::TARGET_OVERLAY );

    m_view->SetLayerDisplayOnly( LAYER_LOCKED_ITEM_SHADOW );
    m_view->SetLayerDisplayOnly( LAYER_CONFLICTS_SHADOW );
    m_view->SetLayerDisplayOnly( LAYER_BOARD_OUTLINE_AREA );

    // Some more required layers settings
    m_view->SetRequired( LAYER_PAD_NETNAMES, LAYER_PADS );

    // Holes can be independent of their host objects (cf: printing drill marks)
    m_view->SetRequired( LAYER_VIA_HOLES, LAYER_VIAS );
    m_view->SetRequired( LAYER_VIA_HOLEWALLS, LAYER_VIAS );
    m_view->SetRequired( LAYER_PAD_PLATEDHOLES, LAYER_PADS );
    m_view->SetRequired( LAYER_PAD_HOLEWALLS, LAYER_PADS );
    m_view->SetRequired( LAYER_NON_PLATEDHOLES, LAYER_PADS );

    // Via visibility
    m_view->SetRequired( LAYER_VIA_MICROVIA, LAYER_VIAS );
    m_view->SetRequired( LAYER_VIA_BLIND, LAYER_VIAS );
    m_view->SetRequired( LAYER_VIA_BURIED, LAYER_VIAS );
    m_view->SetRequired( LAYER_VIA_THROUGH, LAYER_VIAS );
    m_view->SetRequired( LAYER_VIA_NETNAMES, LAYER_VIAS );

    m_view->SetLayerTarget( LAYER_SELECT_OVERLAY, KIGFX::TARGET_OVERLAY );
    m_view->SetLayerDisplayOnly( LAYER_SELECT_OVERLAY ) ;
    m_view->SetLayerTarget( LAYER_GP_OVERLAY, KIGFX::TARGET_OVERLAY );
    m_view->SetLayerDisplayOnly( LAYER_GP_OVERLAY ) ;
    m_view->SetLayerTarget( LAYER_RATSNEST, KIGFX::TARGET_OVERLAY );
    m_view->SetLayerDisplayOnly( LAYER_RATSNEST );

    m_view->SetLayerTarget( LAYER_DRC_ERROR, KIGFX::TARGET_OVERLAY );
    m_view->SetLayerTarget( LAYER_DRC_WARNING, KIGFX::TARGET_OVERLAY );
    m_view->SetLayerTarget( LAYER_DRC_EXCLUSION, KIGFX::TARGET_OVERLAY );
    m_view->SetLayerTarget( LAYER_MARKER_SHADOWS, KIGFX::TARGET_OVERLAY );
    m_view->SetLayerDisplayOnly( LAYER_MARKER_SHADOWS );
    m_view->SetLayerTarget( LAYER_DRC_SHAPES, KIGFX::TARGET_OVERLAY );
    m_view->SetLayerDisplayOnly( LAYER_DRC_SHAPES );    // markers can't be selected through shapes

    m_view->SetLayerTarget( LAYER_DRAWINGSHEET, KIGFX::TARGET_NONCACHED );
    m_view->SetLayerDisplayOnly( LAYER_DRAWINGSHEET ) ; // drawing sheet can't be selected
    m_view->SetLayerDisplayOnly( LAYER_GRID );          // grid can't be selected

    for( int i = LAYER_UI_START; i < LAYER_UI_END; ++i )
    {
        m_view->SetLayerTarget( i, KIGFX::TARGET_OVERLAY );
        m_view->SetLayerDisplayOnly( i );
    }
}


KIGFX::PCB_VIEW* PCB_DRAW_PANEL_GAL::GetView() const
{
    return static_cast<KIGFX::PCB_VIEW*>( m_view );
}
