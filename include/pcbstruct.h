/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007-2014 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2014 KiCad Developers, see CHANGELOG.TXT for contributors.
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
 * @file pcbstruct.h
 * @brief Classes and definitions used in Pcbnew.
 */

#ifndef PCBSTRUCT_H_
#define PCBSTRUCT_H_


/// Values for m_DisplayViaMode member:
enum VIA_DISPLAY_MODE_T {
    VIA_HOLE_NOT_SHOW = 0,
    VIA_SPECIAL_HOLE_SHOW,
    ALL_VIA_HOLE_SHOW,
    OPT_VIA_HOLE_END
};


/**
 * Enum TRACE_CLEARANCE_DISPLAY_MODE_T
 * is the set of values for DISPLAY_OPTIONS.ShowTrackClearanceMode parameter option.
 * This parameter controls how to show tracks and vias clearance area.
 */
enum TRACE_CLEARANCE_DISPLAY_MODE_T {
    DO_NOT_SHOW_CLEARANCE = 0,                // Do not show clearance areas
    SHOW_CLEARANCE_NEW_TRACKS,                /* Show clearance areas only for new track
                                               * during track creation. */
    SHOW_CLEARANCE_NEW_TRACKS_AND_VIA_AREAS,  /* Show clearance areas only for new track
                                               * during track creation, and shows a via
                                               * clearance area at end of current new
                                               * segment (guide to place a new via
                                               */
    SHOW_CLEARANCE_NEW_AND_EDITED_TRACKS_AND_VIA_AREAS,
                                                /* Show clearance for new, moving and
                                                 * dragging tracks and vias
                                                 */
    SHOW_CLEARANCE_ALWAYS                      /* Show Always clearance areas
                                                * for track and vias
                                                */
};


/**
 * Class DISPLAY_OPTIONS
 * handles display options like enable/disable some optional drawings.
 */
class DISPLAY_OPTIONS
{
public:
    bool DisplayPadFill;
    bool DisplayViaFill;
    bool DisplayPadNum;
    bool DisplayPadIsol;

    int  DisplayModEdge;
    int  DisplayModText;
    bool DisplayPcbTrackFill;     /* false = sketch , true = filled */

    /// How trace clearances are displayed.  @see TRACE_CLEARANCE_DISPLAY_MODE_T.
    TRACE_CLEARANCE_DISPLAY_MODE_T  ShowTrackClearanceMode;

    VIA_DISPLAY_MODE_T m_DisplayViaMode;  /* 0 do not show via hole,
                                           * 1 show via hole for non default value
                                           * 2 show all via hole */

    bool DisplayPolarCood;
    int  DisplayZonesMode;
    int  DisplayNetNamesMode;   /* 0 do not show netnames,
                                 * 1 show netnames on pads
                                 * 2 show netnames on tracks
                                 * 3 show netnames on tracks and pads
                                 */

    int  DisplayDrawItems;
    bool ContrastModeDisplay;

public:
    DISPLAY_OPTIONS();
};

#endif // PCBSTRUCT_H_
