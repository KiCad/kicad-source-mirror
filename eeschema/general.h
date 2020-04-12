/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2016-2020 KiCad Developers, see CHANGELOG.TXT for contributors.
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

#ifndef _GENERAL_H_
#define _GENERAL_H_

#include <gal/color4d.h>
#include <layers_id_colors_and_visibility.h>
#include <erc_settings.h>

using KIGFX::COLOR4D;

class CONNECTION_GRAPH;
class TRANSFORM;
class SCH_SHEET;
class SCH_SHEET_PATH;
class ERC_SETTINGS;

#define EESCHEMA_VERSION 5
#define SCHEMATIC_HEAD_STRING "Schematic File Version"

#define DANGLING_SYMBOL_SIZE 12


#define TXT_MARGIN 4

///< The default pin len value when creating pins(can be changed in preference menu)
#define DEFAULTPINLENGTH 100

///< The default pin number size when creating pins(can be changed in preference menu)
#define DEFAULTPINNUMSIZE 50

///< The default pin name size when creating pins(can be changed in preference menu)
#define DEFAULTPINNAMESIZE 50

///< The default selection highlight thickness (can be changed in preference menu)
#define DEFAULTSELECTIONTHICKNESS 3

///< The default line width in mils. (can be changed in preference menu)
#define DEFAULT_LINE_THICKNESS 6

///< The default wire width in mils. (can be changed in preference menu)
#define DEFAULT_WIRE_THICKNESS 6

///< The default bus width in mils. (can be changed in preference menu)
#define DEFAULT_BUS_THICKNESS 12

///< The default function diameter in mils. (can be changed in preference menu)
#define DEFAULT_JUNCTION_DIAM 40

/* Rotation, mirror of graphic items in components bodies are handled by a
 * transform matrix.  The default matrix is useful to draw lib entries with
 * using this default matrix ( no rotation, no mirror but Y axis is bottom to top, and
 * Y draw axis is to to bottom so we must have a default matrix that reverses
 * the Y coordinate and keeps the X coordiate
 */
extern TRANSFORM DefaultTransform;

/* First and main (root) screen */
extern SCH_SHEET*   g_RootSheet;

/**
 * With the new connectivity algorithm, many more places than before want to
 * know what the current sheet is.  This was moved here from SCH_EDIT_FRAME
 * but we could refactor things to get rid of this global.
 */
extern SCH_SHEET_PATH* g_CurrentSheet;    ///< which sheet we are presently working on.

/**
 * This also wants to live in the eventual SCHEMATIC object
 */
extern CONNECTION_GRAPH* g_ConnectionGraph;

/**
 * This also wants to live in the eventual SCHEMATIC object
 */
extern ERC_SETTINGS* g_ErcSettings;

int GetSeverity( int aErrorCode );
void SetSeverity( int aErrorCode, int aSeverity );

/**
 * Amount to offset text above/below wires & buses.  Expressed as a ratio of the text size.
 */
double GetTextOffsetRatio();
void SetTextOffsetRatio( double aOffsetRatio );

// Color to draw items flagged invisible, in libedit (they are invisible in Eeschema
COLOR4D GetInvisibleItemColor();

// TODO(JE) Remove this once wxDC printing is gone
COLOR4D GetLayerColor( SCH_LAYER_ID aLayer );

#endif    // _GENERAL_H_
