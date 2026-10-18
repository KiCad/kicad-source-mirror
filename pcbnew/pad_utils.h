/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#include <functional>

#include <wx/string.h>

#include <layer_ids.h>

class PAD;
class UNITS_PROVIDER;

namespace PAD_UTILS
{

/**
 * @brief Returns true if the pad's rounding ratio is valid (i.e. the pad
 * has a shape where that is meaningful)
 */
bool PadHasMeaningfulRoundingRadius( const PAD& aPad, PCB_LAYER_ID aLayer );


/**
 * @brief Get a sensible default for a rounded rectangle pad's rounding ratio
 *
 * According to IPC-7351C, this is 25%, or 0.25mm, whichever is smaller
 */
double GetDefaultIpcRoundingRatio( const PAD& aPad, PCB_LAYER_ID aLayer );


/**
 * A specific pad check finding reason.
 *
 * Deliberately independent of the DRC error codes: this is an internal check
 * function with granular, but not API-stable, findings. There is a function
 * #ConvertPadCheckToDrcCode() that can map these findings to DRC error codes
 * for reporting and processing as fully-fledged DRC errors.
 *
 * Any of these code can then be remapped to different or new DRC error codes
 * more freely, and new pad codes can be added as needed without affecting
 * existing DRCE codes.
 */
enum class PAD_CHECK_CODE
{
    PAD_SIZE_NOT_POSITIVE,
    PTH_HOLE_TOO_SMALL,
    PTH_HOLE_LEAVES_NO_COPPER,
    PTH_HOLE_NOT_INSIDE_COPPER,
    HOLE_NOT_INSIDE_PAD,
    NEGATIVE_LOCAL_CLEARANCE,
    NEGATIVE_MASK_CLEARANCE_TOO_LARGE,
    NEGATIVE_PASTE_MARGIN_TOO_LARGE,
    ROUNDRECT_RADIUS_NEGATIVE,
    ROUNDRECT_RADIUS_TOO_LARGE,
    CHAMFER_NEGATIVE,
    CHAMFER_TOO_LARGE,
    TRAPEZOID_DELTA_TOO_LARGE,
    CUSTOM_SHAPE_NOT_SINGLE_POLYGON,
    TH_NO_HOLE,
    PTH_NO_OUTER_COPPER,
    SMD_HAS_HOLE,
    SMD_BOTH_OUTER_SIDES,
    SMD_MASK_WRONG_SIDE,
    SMD_PASTE_WRONG_SIDE,
    PAD_NO_OUTER_COPPER,
    CONN_HAS_PASTE,
    FIDUCIAL_NOT_PLATED,
    TESTPOINT_NOT_PLATED,
    HEATSINK_NOT_PLATED,
    CASTELLATED_NOT_PTH,
    BGA_NOT_SMD,
    MECHANICAL_NOT_PTH,
    PRESSFIT_NOT_PTH,
};


/**
 * A single finding from #CheckPad().
 */
struct PAD_CHECK_RESULT
{
    PAD_CHECK_CODE code;
    wxString       message;
};


using PAD_CHECK_HANDLER = std::function<void( const PAD_CHECK_RESULT& )>;


/**
 * Run non-board-specific checks on a pad.
 *
 * These are the checks shared by the PCB DRC, the Footprint Editor Footprint Checker, and
 * the pad properties dialog.
 *
 * @param aPad is the pad to check.
 * @param aUnitsProvider provides unit formatting for the messages.
 * @param aForPadProperties enables checks that are only meaningful while editing pad
 *                          properties, such as the hole being fully inside the copper.
 * @param aErrorHandler receives each finding.
 */
void CheckPad( const PAD& aPad, const UNITS_PROVIDER& aUnitsProvider, bool aForPadProperties,
               const PAD_CHECK_HANDLER& aErrorHandler );

} // namespace PAD_UTILS
