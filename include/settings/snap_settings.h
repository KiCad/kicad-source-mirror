/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
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

#ifndef SETTINGS_SNAP_SETTINGS_H
#define SETTINGS_SNAP_SETTINGS_H

struct SNAP_INFERENCE_SETTINGS
{
    bool objectGeometry = true;
    bool constructionExtensions = true;
    bool tangentNormal = false;
    bool alignmentDistribution = false;
};


enum class MAGNETIC_OPTIONS
{
    NO_EFFECT = 0,
    CAPTURE_CURSOR_IN_TRACK_TOOL,
    CAPTURE_ALWAYS
};


struct MAGNETIC_SETTINGS
{
    MAGNETIC_OPTIONS pads = MAGNETIC_OPTIONS::CAPTURE_CURSOR_IN_TRACK_TOOL;
    MAGNETIC_OPTIONS tracks = MAGNETIC_OPTIONS::CAPTURE_CURSOR_IN_TRACK_TOOL;
    bool             graphics = false;
    bool             allLayers = false;
};

#endif
