/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 Jean-Pierre Charras.
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

// These flags are used to control the data items that must be disabled when creating
// mainly a netlist but also some other KiCad files.
// They allow skipping specified data in these files.

#define CTL_OMIT_EXTRA              (1 << 0)
#define CTL_OMIT_NETS               (1 << 1)
#define CTL_OMIT_PAD_NETS           (1 << 1)    ///< Omit pads net names (useless in library).
#define CTL_OMIT_UUIDS              (1 << 2)    ///< Omit component unique ids (useless in library)
#define CTL_OMIT_FP_UUID            (1 << 3)    ///< Don't prefix the footprint UUID to the sheet
                                                ///< path.
#define CTL_OMIT_PATH               (1 << 4)    ///< Omit component sheet time stamp (useless in
                                                ///< library).
#define CTL_OMIT_AT                 (1 << 5)    ///< Omit position and rotation. (always saved
                                                ///< with position 0,0 and rotation = 0 in library).
#define CTL_OMIT_LIBNAME            (1 << 7)    ///< Omit lib alias when saving (used for
                                                ///< board/not library)..
#define CTL_OMIT_FOOTPRINT_VERSION  (1 << 8)    ///< Omit the version string from the (footprint)
                                                ///< sexpr group.
#define CTL_OMIT_FILTERS            (1 << 9)    ///< Omit the ki_fp_filters attribute in
                                                ///< .kicad_xxx files.
#define CTL_OMIT_INITIAL_COMMENTS   (1 << 10)   ///< Omit #FOOTPRINT initial comments.

#define CTL_OMIT_COLOR              (1 << 11)   ///< Omit the color attribute in .kicad_xxx files.
#define CTL_OMIT_HYPERLINK          (1 << 12)   ///< Omit the hyperlink attribute in .kicad_xxx
                                                ///< files.
