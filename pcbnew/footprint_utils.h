/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * @file footprint_utils.h
 *
 * Collection of reusable/testable functions for footprint manipulation and validation.
 */

#pragma once

#include <functional>

#include <wx/string.h>

#include <geometry/eda_angle.h>
#include <math/vector2d.h>
#include <pad_utils.h>

class BOARD_ITEM;
class FOOTPRINT;
class PAD;
class UNITS_PROVIDER;


/**
 * Compute position and angle shift between two footprints.
 *
 * This will only happen when the new footprint has a different origin or has been
 * rotated compared to the existing footprint. Most of the time, this returns
 * a null shift.
 *
 * The shift is computed based on pad positions and assumes that there are at least
 * two non-coincident pads with unique numbers that are the same in both footprints.
 *
 * Pad positions are compared on the side of the existing footprint.  A footprint on
 * the back of the board stores its pads mirrored in its library frame, so if the new
 * footprint is on the other side (and will be flipped onto the existing footprint's
 * side when it is exchanged in), its pad positions are mirrored to match before the
 * shift is computed.
 */
bool ComputeFootprintShift( const FOOTPRINT& aExisting, const FOOTPRINT& aNew, VECTOR2I& aShift,
                            EDA_ANGLE& aAngleShift );


namespace FOOTPRINT_UTILS
{

using PAD_CHECK_HANDLER = std::function<void( const PAD&, const PAD_UTILS::PAD_CHECK_RESULT& )>;


/**
 * Check that the footprint's SMD/through-hole attribute matches its pads.
 *
 * @param aFootprint is the footprint to check.
 * @param aErrorHandler receives each finding message.
 */
void CheckFootprintAttributes( const FOOTPRINT& aFootprint,
                               const std::function<void( const wxString& )>& aErrorHandler );


/**
 * Run non-board-specific DRC checks on footprint's pads.  These are the checks supported by
 * both the PCB DRC and the Footprint Editor Footprint Checker.
 *
 * @param aFootprint is the footprint whose pads are checked.
 * @param aUnitsProvider provides unit formatting for the messages.
 * @param aErrorHandler receives each finding.
 */
void CheckPads( const FOOTPRINT& aFootprint, const UNITS_PROVIDER& aUnitsProvider,
                const PAD_CHECK_HANDLER& aErrorHandler );


/**
 * Check for overlapping, different-numbered, non-net-tie pads.
 */
void CheckShortingPads( const FOOTPRINT& aFootprint,
                        const std::function<void( const PAD*, const PAD*, int aErrorCode,
                                                  const VECTOR2I& )>& aErrorHandler );


/**
 * Check for un-allowed shorting of pads in net-tie footprints.  If two pads are shorted,
 * they must both appear in one of the allowed-shorting lists.
 */
void CheckNetTies( const FOOTPRINT& aFootprint,
                   const std::function<void( const BOARD_ITEM*, const BOARD_ITEM*,
                                             const BOARD_ITEM*, const VECTOR2I& )>& aErrorHandler );


/**
 * Sanity check net-tie pad groups.  Pads cannot be listed more than once, and pad numbers
 * must correspond to a pad.
 */
void CheckNetTiePadGroups( const FOOTPRINT& aFootprint,
                           const std::function<void( const wxString& )>& aErrorHandler );


/**
 * Check for silkscreen graphics clipped by solder mask apertures.
 */
void CheckClippedSilk( const FOOTPRINT& aFootprint,
                       const std::function<void( BOARD_ITEM*, BOARD_ITEM*,
                                                 const VECTOR2I& )>& aErrorHandler );

} // namespace FOOTPRINT_UTILS