/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
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

#pragma once

/**
 * @file
 * @brief Declarations of types for tracks and vias.
 */

// Flag used in locate routines (from which endpoint work)
enum ENDPOINT_T : int
{
    ENDPOINT_START = 0,
    ENDPOINT_END = 1
};

// Note that this enum must be synchronized to GAL_LAYER_ID
enum class VIATYPE : int
{
    THROUGH      = 4, /* Always a through hole via */
    BURIED       = 3, /* this via can be on internal layers */
    BLIND        = 2, /* this via can be on internal layers */
    MICROVIA     = 1, /* this via which connect from an external layer
                       * to the near neighbor internal layer */
    NOT_DEFINED  = 0  /* not yet used */
};

enum class TENTING_MODE
{
    FROM_BOARD = 0,
    TENTED = 1,
    NOT_TENTED = 2
};

enum class COVERING_MODE
{
    FROM_BOARD = 0,
    COVERED = 1,
    NOT_COVERED = 2
};

enum class PLUGGING_MODE
{
    FROM_BOARD = 0,
    PLUGGED = 1,
    NOT_PLUGGED = 2
};

enum class CAPPING_MODE
{
    FROM_BOARD = 0,
    CAPPED = 1,
    NOT_CAPPED = 2
};

enum class FILLING_MODE
{
    FROM_BOARD = 0,
    FILLED = 1,
    NOT_FILLED = 2
};

#define UNDEFINED_DRILL_DIAMETER  -1       //< Undefined via drill diameter.

class PCB_TRACK;
class PCB_VIA;
class PCB_ARC;
