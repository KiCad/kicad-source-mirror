/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Author Jean-Pierre Charras, jp.charras at wanadoo.fr
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

#include <sch_label.h>
#include <sch_pin.h>

class LIB_SYMBOL;
class SCH_SYMBOL;

/**
 * Rotate and/or mirror graphic objects of LIB_SYMBOL aSymbol according to aOrientMirror.
 * @param aLibSymbol is the LIB_SYMBOL to modify
 * @param aOrientation is the orientation+mirror value like returned by SCH_SYMBOL::GetOrientation()
 */
void OrientAndMirrorSymbolItems( LIB_SYMBOL* aLibSymbol, int aOrientation );


/**
 * Rotate and/or mirror a SCH_PIN according to aOrientMirror.
 * aOrientMirror is usually the orientation/mirror of the parent symbol.
 * The modified pin orientation is the actual pin orientation/mirror
 * when the parent symbol is drawn.
 * @param aPin is the SCH_PIN to modify
 * @param aOrientation is the orientation+mirror value like returned by SCH_SYMBOL::GetOrientation()
 */
void RotateAndMirrorPin( SCH_PIN& aPin, int aOrientMirror );

/**
 * Get the spin style for a pin's label, taking into account the pin's orientation,
 * as well as the given symbol's orientation.
 *
 * For example, pin with PIN_RIGHT (i.e. a pin on the left side of a symbol, probably)
 * and no symbol rotation/mirror will return SPIN_STYLE::LEFT.
 */
SPIN_STYLE GetPinSpinStyle( const SCH_PIN& aPin, const SCH_SYMBOL& aSymbol );