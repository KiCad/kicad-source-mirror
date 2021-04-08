/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <common.h>
#include <pcbnew.h>
#include <board_design_settings.h>
#include <layers_id_colors_and_visibility.h>
#include <pcb_display_options.h>
#include <eda_text.h>

PCB_DISPLAY_OPTIONS::PCB_DISPLAY_OPTIONS()
{
    m_DisplayPadFill          = FILLED;
    m_DisplayViaFill          = FILLED;
    m_DisplayPadNum           = true;
    m_DisplayPadClearance     = true;

    m_DisplayGraphicsFill     = FILLED;
    m_DisplayTextFill         = FILLED;
    m_DisplayPcbTrackFill     = FILLED;   // false = sketch , true = filled
    m_ShowTrackClearanceMode  = SHOW_TRACK_CLEARANCE_WITH_VIA_WHILE_ROUTING;

    m_ZoneDisplayMode         = ZONE_DISPLAY_MODE::SHOW_FILLED;
    m_DisplayNetNamesMode     = 3;      /* 0 do not show netnames,
                                         * 1 show netnames on pads
                                         * 2 show netnames on tracks
                                         * 3 show netnames on tracks and pads */
    m_ContrastModeDisplay     = HIGH_CONTRAST_MODE::NORMAL;
    m_NetColorMode            = NET_COLOR_MODE::RATSNEST;
    m_RatsnestMode            = RATSNEST_MODE::ALL;
    m_MaxLinksShowed          = 3;        // in track creation: number of hairwires shown
    m_ShowModuleRatsnest      = true;     // When moving a footprint: allows displaying a ratsnest
    m_DisplayRatsnestLinesCurved = false;
    m_ShowGlobalRatsnest      = true;

    m_TrackOpacity = 1.0;
    m_ViaOpacity   = 1.0;
    m_PadOpacity   = 1.0;
    m_ZoneOpacity  = 1.0;

    m_DisplayOrigin           = PCB_ORIGIN_PAGE;
    m_DisplayInvertXAxis      = false;
    m_DisplayInvertYAxis      = false;
    m_Live3DRefresh           = false;
}
