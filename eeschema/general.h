/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007 Jean-Pierre Charras, jp.charras at wanadoo.fr
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

#ifndef _GENERAL_H_
#define _GENERAL_H_

#include <gal/color4d.h>
#include <layer_ids.h>

using KIGFX::COLOR4D;

class TRANSFORM;

#define EESCHEMA_VERSION 5
#define SCHEMATIC_HEAD_STRING "Schematic File Version"

/* Rotation, mirror of graphic items in symbol bodies are handled by a
 * transform matrix.  The default matrix is useful to draw lib entries with
 * using this default matrix ( no rotation, no mirror but Y axis is bottom to top, and
 * Y draw axis is to to bottom so we must have a default matrix that reverses
 * the Y coordinate and keeps the X coordinate
 */
extern TRANSFORM DefaultTransform;

#endif    // _GENERAL_H_
