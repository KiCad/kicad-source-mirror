/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007-2014 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2020 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file pcb_display_options.h
 * @brief Definition of PCB_DISPLAY_OPTIONS class
 */

#ifndef PCB_DISPLAY_OPTIONS_H_
#define PCB_DISPLAY_OPTIONS_H_

#include <project/board_project_settings.h>

/**
 * Container for display options like enable/disable some optional drawings.
 */
class PCB_DISPLAY_OPTIONS
{
public:
    PCB_DISPLAY_OPTIONS();

    /**
     * The set of values for DISPLAY_OPTIONS.ShowTrackClearanceMode parameter option.
     *
     * This parameter controls how to show tracks and vias clearance area.
     */
    enum TRACE_CLEARANCE_DISPLAY_MODE_T {
        DO_NOT_SHOW_CLEARANCE = 0,
        SHOW_TRACK_CLEARANCE_WHILE_ROUTING,
        SHOW_TRACK_CLEARANCE_WITH_VIA_WHILE_ROUTING,
        SHOW_WHILE_ROUTING_OR_DRAGGING,
        SHOW_TRACK_CLEARANCE_WITH_VIA_ALWAYS
    };

    bool m_DisplayPadFill;
    bool m_DisplayViaFill;
    bool m_DisplayPadNum;           // show pads numbers
    bool m_DisplayPadClearance;
    bool m_DisplayGraphicsFill;     // How to display fp drawings ( sketch/ filled )
    bool m_DisplayTextFill;         // How to display fp texts ( sketch/ filled )
    bool m_DisplayPcbTrackFill;     // false : tracks are show in sketch mode, true = filled.

    /// How trace clearances are displayed.  @see TRACE_CLEARANCE_DISPLAY_MODE_T.
    TRACE_CLEARANCE_DISPLAY_MODE_T  m_ShowTrackClearanceMode;

    /// @see ZONE_DISPLAY_MODE - stored in the project
    ZONE_DISPLAY_MODE m_ZoneDisplayMode;

    int  m_DisplayNetNamesMode;     /* 0 do not show netnames,
                                     * 1 show netnames on pads
                                     * 2 show netnames on tracks
                                     * 3 show netnames on tracks and pads
                                     */

    /// How inactive layers are displayed.  @see HIGH_CONTRAST_MODE - stored in the project
    HIGH_CONTRAST_MODE m_ContrastModeDisplay;

    /// How to use color overrides on specific nets and netclasses
    NET_COLOR_MODE m_NetColorMode;

    /// Ratsnest draw mode (all layers vs only visible layers)
    RATSNEST_MODE m_RatsnestMode;

    int  m_MaxLinksShowed;              // in track creation: number of airwires shown
    bool m_ShowModuleRatsnest;          // When moving a footprint: allows displaying a ratsnest
    bool m_ShowGlobalRatsnest;          // If true, show all
    bool m_DisplayRatsnestLinesCurved;  // Airwires can be drawn as straight lines (false)
                                        // or curved lines (true)

    // These opacity overrides multiply with any opacity in the base layer color

    double m_TrackOpacity;     ///< Opacity override for all tracks
    double m_ViaOpacity;       ///< Opacity override for all types of via
    double m_PadOpacity;       ///< Opacity override for SMD pads and PTHs
    double m_ZoneOpacity;      ///< Opacity override for filled zone areas

    /**
     * The set of values for DISPLAY_OPTIONS.DisplayOrigin parameter option.
     *
     * This parameter controls what is used as the origin point for location values
     */
    enum PCB_DISPLAY_ORIGIN_OPTIONS_T {
        PCB_ORIGIN_PAGE = 0,
        PCB_ORIGIN_AUX,
        PCB_ORIGIN_GRID,
    };

    /// Which origin is used for display transforms
    PCB_DISPLAY_ORIGIN_OPTIONS_T m_DisplayOrigin;
    bool m_DisplayInvertXAxis;          //< true: Invert the X axis for display
    bool m_DisplayInvertYAxis;          //< true: Invert the Y axis for display

};

#endif // PCBSTRUCT_H_
