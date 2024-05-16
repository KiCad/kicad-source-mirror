/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Author Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2024 KiCad Developers, see AUTHORS.txt for contributors.
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

class LIB_SYMBOL;
class LIB_PIN;

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
void RotateAndMirrorPin( LIB_PIN& aPin, int aOrientMirror );
