/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2012-2019 Kicad Developers, see AUTHORS.txt for contributors.
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

#ifndef PCBNEW_GENERAL_SETTINGS_H
#define PCBNEW_GENERAL_SETTINGS_H

#include <colors_design_settings.h>
#include <vector>

class wxConfigBase;
class wxString;

enum MAGNETIC_OPTIONS
{
    NO_EFFECT,
    CAPTURE_CURSOR_IN_TRACK_TOOL,
    CAPTURE_ALWAYS
};

class PCB_GENERAL_SETTINGS : public SETTINGS
{
public:
    PCB_GENERAL_SETTINGS( FRAME_T aFrameType );

    void Load( wxConfigBase* aCfg ) override;
    void Save( wxConfigBase* aCfg ) override;

    COLORS_DESIGN_SETTINGS& Colors() { return m_colorsSettings; }

    bool m_Use45DegreeGraphicSegments;   // True to constraint graphic lines to horizontal,
                                         // vertical and 45º
    bool m_EditHotkeyChangesTrackWidth;
    bool m_FlipLeftRight;                // True: Flip footprints across Y axis
                                         // False: Flip footprints across X axis

    MAGNETIC_OPTIONS m_MagneticPads;
    MAGNETIC_OPTIONS m_MagneticTracks;
    bool             m_MagneticGraphics;

#if defined(KICAD_SCRIPTING) && defined(KICAD_SCRIPTING_ACTION_MENU)
    std::vector< std::pair<wxString, wxString> > m_pluginSettings;  // Settings for action plugins
#endif

protected:
    const FRAME_T           m_frameType;
    COLORS_DESIGN_SETTINGS  m_colorsSettings;
};

#endif  // PCBNEW_GENERAL_SETTINGS_H
