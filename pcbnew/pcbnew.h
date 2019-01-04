/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007-2014 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2016 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file pcbnew.h
 */

#ifndef PCBNEW_H
#define PCBNEW_H


#include <fctsys.h>         // wxWidgets include.
#include <base_struct.h>    // IS_DRAGGED and IN_EDIT definitions.
#include <dlist.h>
#include <convert_to_biu.h> // to define Mils2iu() conversion function
#include <layers_id_colors_and_visibility.h>

// Arcs are approximated by segments: define the number of segments per 360 deg (KiCad uses 0.1
// deg approximation).  Be aware 3600 / ARC_APPROX_SEGMENTS_COUNT_LOW_DEF is an integer.
#define ARC_APPROX_SEGMENTS_COUNT_LOW_DEF 16
#define ARC_APPROX_SEGMENTS_COUNT_HIGH_DEF 32

// The new absolute-error-based algorithm uses the stored segment count as a hint on whether
// to use ARC_HIGH_DEF or ARC_LOW_DEF.  This defines the crossover point.
#define SEGMENT_COUNT_CROSSOVER 24

/* Flag used in locate functions. The locate ref point is the on grid cursor or the off
 * grid mouse cursor. */
#define CURSEUR_ON_GRILLE  (0 << 0)
#define CURSEUR_OFF_GRILLE (1 << 0)

#define IGNORE_LOCKED (1 << 1)   ///< if module is locked, do not select for single module operation
#define MATCH_LAYER   (1 << 2)   ///< if module not on current layer, do not select
#define VISIBLE_ONLY  (1 << 3)   ///< if module not on a visible layer, do not select

/// Flag used in locate routines (from which endpoint work)
enum ENDPOINT_T {
    ENDPOINT_START = 0,
    ENDPOINT_END = 1
};

#define DIM_ANCRE_MODULE 3       // Anchor size (footprint center)


// These are only here for algorithmic safety, not to tell the user what to do
#define TEXTS_MIN_SIZE  Mils2iu( 1 )        ///< Minimum text size in internal units (1 mil)
#define TEXTS_MAX_SIZE  Mils2iu( 10000 )    ///< Maximum text size in internal units (10 inches)
#define TEXTS_MAX_WIDTH Mils2iu( 10000 )    ///< Maximum text width in internal units (10 inches)


// Flag to force the SKETCH mode to display items (.m_Flags member)
#define FORCE_SKETCH ( IS_DRAGGED | IN_EDIT )

/* Name of the document footprint list
 * usually located in share/modules/footprints_doc
 * this is of the responsibility to users to create this file
 * if they want to have a list of footprints
 * default is "footprints_doc/footprints.pdf"
 */
extern wxString g_DocModulesFileName;

// variables
extern bool     g_Raccord_45_Auto;
extern bool     g_Alternate_Track_Posture;
// Layer pair for auto routing and switch layers by hotkey
extern PCB_LAYER_ID g_Route_Layer_TOP;
extern PCB_LAYER_ID g_Route_Layer_BOTTOM;

extern wxPoint  g_Offset_Module;         // Offset trace when moving footprint.

/// List of segments of the trace currently being drawn.
class TRACK;
extern DLIST<TRACK> g_CurrentTrackList;
#define g_CurrentTrackSegment g_CurrentTrackList.GetLast()    ///< most recently created segment
#define g_FirstTrackSegment   g_CurrentTrackList.GetFirst()   ///< first segment created



/**
 * Helper function PythonPluginsReloadBase
 * Reload Python plugins if they are newer than
 * the already loaded, and load new plugins if any
 * It calls the LoadPlugins(bundlepath) Python method
 * see kicadplugins.i
 */
void PythonPluginsReloadBase();


#endif // PCBNEW_H
