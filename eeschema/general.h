/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2016 KiCad Developers, see CHANGELOG.TXT for contributors.
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
 * @file general.h
 */

#ifndef _GENERAL_H_
#define _GENERAL_H_

#include <gal/color4d.h>
#include <layers_id_colors_and_visibility.h>

using KIGFX::COLOR4D;

class CONNECTION_GRAPH;
class TRANSFORM;
class SCH_SHEET;
class SCH_SHEET_PATH;

#define EESCHEMA_VERSION 5
#define SCHEMATIC_HEAD_STRING "Schematic File Version"

#define DANGLING_SYMBOL_SIZE 12


#define DEFAULT_REPEAT_OFFSET_X 0       ///< the default X value (overwritten by the eeschema config)
#define DEFAULT_REPEAT_OFFSET_Y 100     ///< the default Y value (overwritten by the eeschema config)
#define REPEAT_OFFSET_MAX 1000          ///< the max value of repeat offset value
#define DEFAULT_REPEAT_LABEL_INC 1      ///< the default value (overwritten by the eeschema config)
#define DEFAULT_REPEAT_OFFSET_PIN 100   ///< the default value (overwritten by the eeschema config)
                                        ///< when repeating a pin

///< The thickness to draw busses that do not have a specific width
///< (can be changed in preference menu)
#define DEFAULTBUSTHICKNESS 12

///< The thickness to draw lines that thickness is set to 0 (default thickness)
///< (can be changed in preference menu)
#define DEFAULTDRAWLINETHICKNESS 6

///< The default pin len value when creating pins(can be changed in preference menu)
#define DEFAULTPINLENGTH 100

///< The default pin number size when creating pins(can be changed in preference menu)
#define DEFAULTPINNUMSIZE 50

///< The default pin name size when creating pins(can be changed in preference menu)
#define DEFAULTPINNAMESIZE 50

///< The default library pane width
#define DEFAULTLIBWIDTH 250

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
 * Default line thickness used to draw/plot items having a
 * default thickness line value (i.e. = 0 ).
 */
int GetDefaultLineThickness();
void SetDefaultLineThickness( int aThickness );

/**
 * Default size for text in general
 */
int GetDefaultTextSize();
void SetDefaultTextSize( int aSize );

/**
 * Default line thickness used to draw/plot busses.
 */
int GetDefaultBusThickness();
void SetDefaultBusThickness( int aThickness );

/**
 * Default line thickness used to draw/plot wires.
 */
int GetDefaultWireThickness();
void SetDefaultWireThickness( int aThickness );

COLOR4D  GetLayerColor( SCH_LAYER_ID aLayer );
void     SetLayerColor( COLOR4D aColor, SCH_LAYER_ID aLayer );

// Color to draw selected items
COLOR4D GetItemSelectedColor();

// Color to draw items flagged invisible, in libedit (they are invisible in Eeschema
COLOR4D GetInvisibleItemColor();

#endif    // _GENERAL_H_
