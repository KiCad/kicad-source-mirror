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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

/**
 * Board object opacities used when a project's local settings file does not set them.
 *
 * The user edits these in the PCB editor preferences.  They live in the common settings so that
 * whichever application first writes a project's .kicad_prl writes the user's values.
 */
struct BOARD_OPACITY_DEFAULTS
{
    double m_TrackOpacity = 1.0;
    double m_ViaOpacity = 1.0;
    double m_PadOpacity = 1.0;
    double m_ZoneOpacity = 0.6;
    double m_ImageOpacity = 0.6;
    double m_ShapeOpacity = 1.0;
    double m_OutsideGroupOpacity = 1.0;
};
