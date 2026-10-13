/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009-2019 Jean-Pierre Charras, jp.charras at wanadoo.fr
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <eda_units.h>

#define DEFAULT_SILK_LINE_WIDTH       0.1
#define DEFAULT_COPPER_LINE_WIDTH     0.2
#define DEFAULT_EDGE_WIDTH            0.05
#define DEFAULT_COURTYARD_WIDTH       0.05
#define DEFAULT_LINE_WIDTH            0.10

#define DEFAULT_SILK_TEXT_SIZE        1.0
#define DEFAULT_COPPER_TEXT_SIZE      1.5
#define DEFAULT_TEXT_SIZE             1.0

#define DEFAULT_SILK_TEXT_WIDTH       0.1
#define DEFAULT_COPPER_TEXT_WIDTH     0.30
#define DEFAULT_TEXT_WIDTH            0.15

#define DEFAULT_DIMENSION_ARROW_LENGTH         50 // mils, for legacy purposes
#define DEFAULT_DIMENSION_EXTENSION_OFFSET     0.5

// Largest microvia stack pitch, in mm. Bounds the hop position arithmetic.
#define MAX_MICROVIA_STACK_PITCH_MM 25.0

// Board thickness, mainly for 3D view:
#define DEFAULT_BOARD_THICKNESS_MM             1.6

#define DEFAULT_PCB_EDGE_THICKNESS             0.15

// soldermask to pad clearance. The default is 0 because usually board houses
// create a clearance depending on their fab process: mask material, color, price, etc.
#define DEFAULT_SOLDERMASK_EXPANSION           0.0

#define DEFAULT_SOLDERMASK_TO_COPPER_CLEARANCE 0.0

#define DEFAULT_SOLDERMASK_MIN_WIDTH           0.0

#define DEFAULT_SOLDERPASTE_CLEARANCE          0.0
#define DEFAULT_SOLDERPASTE_RATIO              0.0

#define DEFAULT_CUSTOMTRACKWIDTH      0.2
#define DEFAULT_CUSTOMDPAIRWIDTH      0.125
#define DEFAULT_CUSTOMDPAIRGAP        0.18
#define DEFAULT_CUSTOMDPAIRVIAGAP     0.18

#define DEFAULT_MEANDER_SPACING       0.6
#define DEFAULT_DP_MEANDER_SPACING    1.0

#define DEFAULT_MINCLEARANCE          0.0     // overall min clearance
#define DEFAULT_MINCONNECTION         0.0     // overall min connection width
#define DEFAULT_TRACKMINWIDTH         0.2     // track width min value (mm)
#define DEFAULT_VIASMINSIZE           0.5     // vias (not micro vias) min diameter
#define DEFAULT_MINTHROUGHDRILL       0.3     // through holes (not micro vias) min drill diameter
#define DEFAULT_MICROVIASMINSIZE      0.2     // micro vias (not vias) min diameter
#define DEFAULT_MICROVIASMINDRILL     0.1     // micro vias (not vias) min drill diameter
#define DEFAULT_HOLETOHOLEMIN         0.25    // minimum web thickness between two drilled holes
#define DEFAULT_HOLECLEARANCE         0.25    // copper-to-hole clearance (from IPC level A)

#define DEFAULT_COPPEREDGECLEARANCE   0.5     // clearance between copper items and edge cuts
#define LEGACY_COPPEREDGECLEARANCE   -0.01    // A flag to indicate the legacy method (based
                                              // on edge cut line thicknesses) should be used.
#define DEFAULT_SILKCLEARANCE         0.0
#define DEFAULT_MINGROOVEWIDTH        0.0

#define DEFAULT_MINRESOLVEDSPOKES     2       // Fewer resolved spokes indicates a starved thermal

#define MINIMUM_ERROR_SIZE_MM         0.001   // For arc approximation
#define MAXIMUM_ERROR_SIZE_MM         0.1     // For arc approximation

#define MAXIMUM_CLEARANCE             pcbIUScale.mmToIU( 500 )  // to prevent int-overflows

// Min/max values used in dialogs to validate settings
#define MINIMUM_LINE_WIDTH_MM         0.005   // minimal line width entered in a dialog
#define MAXIMUM_LINE_WIDTH_MM         100.0   // max line width entered in a dialog

// Default pad properties
#define DEFAULT_PAD_THT_WIDTH_MM      2.54
#define DEFAULT_PAD_THT_HEIGHT_MM     1.27
#define DEFAULT_PAD_DRILL_DIAMETER_MM 0.8
#define DEFAULT_PAD_SMD_WIDTH_MM      2.0
#define DEFAULT_PAD_SMD_HEIGHT_MM     1.0
#define DEFAULT_PAD_RR_RADIUS_RATIO   0.25    // IPC standard
