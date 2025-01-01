/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007-2014 Jean-Pierre Charras, jp.charras at wanadoo.fr
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

#ifndef PCB_DISPLAY_OPTIONS_H_
#define PCB_DISPLAY_OPTIONS_H_

#include <project/board_project_settings.h>

class PCB_DISPLAY_OPTIONS
{
public:
    PCB_DISPLAY_OPTIONS()
    {
        m_ZoneDisplayMode     = ZONE_DISPLAY_MODE::SHOW_FILLED;
        m_ContrastModeDisplay = HIGH_CONTRAST_MODE::NORMAL;
        m_NetColorMode        = NET_COLOR_MODE::RATSNEST;

        m_TrackOpacity        = 1.0;
        m_ViaOpacity          = 1.0;
        m_PadOpacity          = 1.0;
        m_ZoneOpacity         = 1.0;
        m_ImageOpacity        = 1.0;
        m_FilledShapeOpacity        = 1.0;
    }

    /// @see ZONE_DISPLAY_MODE - stored in the project
    ZONE_DISPLAY_MODE  m_ZoneDisplayMode;

    /// How inactive layers are displayed.  @see HIGH_CONTRAST_MODE - stored in the project
    HIGH_CONTRAST_MODE m_ContrastModeDisplay;

    /// How to use color overrides on specific nets and netclasses
    NET_COLOR_MODE     m_NetColorMode;

    // These opacity overrides multiply with any opacity in the base layer color

    double             m_TrackOpacity;     ///< Opacity override for all tracks
    double             m_ViaOpacity;       ///< Opacity override for all types of via
    double             m_PadOpacity;       ///< Opacity override for SMD pads and PTHs
    double             m_ZoneOpacity;      ///< Opacity override for filled zone areas
    double             m_ImageOpacity;     ///< Opacity override for user images
    double             m_FilledShapeOpacity;     ///< Opacity override for graphic shapes
};

#endif // PCBSTRUCT_H_
