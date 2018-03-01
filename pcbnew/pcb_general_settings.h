/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2012-2017 Kicad Developers, see AUTHORS.txt for contributors.
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

#ifndef __PCBNEW_GENERAL_SETTINGS_H
#define __PCBNEW_GENERAL_SETTINGS_H

#include <colors_design_settings.h>

class wxConfigBase;
class wxString;

enum MAGNETIC_PAD_OPTION_VALUES
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

    COLORS_DESIGN_SETTINGS& Colors()
    {
        return m_colorsSettings;
    }

    bool    m_legacyDrcOn = true;                   // Not stored, always true when starting pcbnew,
                                                    // false only on request during routing, and
                                                    // always for temporary use
    bool    m_legacyAutoDeleteOldTrack = true;
    bool    m_legacyUse45DegreeTracks = true;       // True to allow horiz, vert. and 45deg only tracks
    static bool m_use45DegreeGraphicSegments;       // True to allow horizontal, vertical and
                                                    // 45deg only graphic segments
    bool    m_legacyUseTwoSegmentTracks = true;

    bool    m_editActionChangesTrackWidth = false;
    static bool m_dragSelects;                  // True: Drag gesture always draws a selection box,
                                                // False: Drag will preselect an item and move it

    MAGNETIC_PAD_OPTION_VALUES  m_magneticPads  = CAPTURE_CURSOR_IN_TRACK_TOOL;
    MAGNETIC_PAD_OPTION_VALUES  m_magneticTracks = CAPTURE_CURSOR_IN_TRACK_TOOL;

protected:
    const FRAME_T m_frameType;
    COLORS_DESIGN_SETTINGS m_colorsSettings;
};

#endif
