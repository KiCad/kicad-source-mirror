/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007-2014 Jean-Pierre Charras  jp.charras at wanadoo.fr
 * Copyright (C) 1992-2014 KiCad Developers, see change_log.txt for contributors.
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

#ifndef GERBVIEW_H
#define GERBVIEW_H

#include <vector>
#include <set>


#define CURSEUR_ON_GRILLE  0
#define CURSEUR_OFF_GRILLE 1

/**
* size of single line of a text from a gerber file.
* warning: some files can have very long lines, so the buffer must be large.
*/
#define GERBER_BUFZ     4000

/// List of page sizes
extern const wxChar* g_GerberPageSizeList[8];

// Type of photoplotter action:
#define GERB_ACTIVE_DRAW 1      // Activate light (lower pen)
#define GERB_STOP_DRAW   2      // Extinguish light (lift pen)
#define GERB_FLASH       3      // Flash

// number fo draw layers in Gerbview
#define GERBER_DRAWLAYERS_COUNT 32

/**
 * Enum GERBER_VISIBLE_ID
 * is a set of visible GERBVIEW elements.
 */
enum GERBER_VISIBLE_ID
{
    DCODES_VISIBLE = 1,         // visible item id cannot be 0
                                // because this id is used as wxWidget id
    GERBER_GRID_VISIBLE,
    NEGATIVE_OBJECTS_VISIBLE,   // use the selected color to draw negative objects
                                // instaed of background color, to make them visible
    END_GERBER_VISIBLE_LIST     // sentinel
};

// Interpolation type
enum Gerb_Interpolation
{
    GERB_INTERPOL_LINEAR_1X = 0,
    GERB_INTERPOL_LINEAR_10X,
    GERB_INTERPOL_LINEAR_01X,
    GERB_INTERPOL_LINEAR_001X,
    GERB_INTERPOL_ARC_NEG,
    GERB_INTERPOL_ARC_POS
};


// Command Type (GCodes)
enum Gerb_GCommand
{
    GC_MOVE                     = 0,
    GC_LINEAR_INTERPOL_1X       = 1,
    GC_CIRCLE_NEG_INTERPOL      = 2,
    GC_CIRCLE_POS_INTERPOL      = 3,
    GC_COMMENT                  = 4,
    GC_LINEAR_INTERPOL_10X      = 10,
    GC_LINEAR_INTERPOL_0P1X     = 11,
    GC_LINEAR_INTERPOL_0P01X    = 12,
    GC_TURN_ON_POLY_FILL        = 36,
    GC_TURN_OFF_POLY_FILL       = 37,
    GC_SELECT_TOOL              = 54,
    GC_PHOTO_MODE               = 55,          // can start a D03 flash command: redundant with D03
    GC_SPECIFY_INCHES           = 70,
    GC_SPECIFY_MILLIMETERS      = 71,
    GC_TURN_OFF_360_INTERPOL    = 74,
    GC_TURN_ON_360_INTERPOL     = 75,
    GC_SPECIFY_ABSOLUES_COORD   = 90,
    GC_SPECIFY_RELATIVEES_COORD = 91
};


enum Gerb_Analyse_Cmd
{
    CMD_IDLE = 0,
    END_BLOCK,
    ENTER_RS274X_CMD
};

#endif  // ifndef GERBVIEW_H
