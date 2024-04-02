/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <symbol.h>


void SYMBOL::ViewGetLayers( int aLayers[], int& aCount ) const
{
    aCount = 0;

    // Pins are drawn by their parent symbol, so the parent must draw to LAYER_DANGLING
    if( Type() == SCH_SYMBOL_T )
        aLayers[aCount++] = LAYER_DANGLING;

    // Same for operating point currents
    if( Type() == SCH_SYMBOL_T )
        aLayers[aCount++] = LAYER_OP_CURRENTS;

    aLayers[aCount++] = LAYER_DEVICE;
    aLayers[aCount++] = LAYER_REFERENCEPART;
    aLayers[aCount++] = LAYER_VALUEPART;
    aLayers[aCount++] = LAYER_FIELDS;

    if( Type() == LIB_SYMBOL_T )
        aLayers[aCount++] = LAYER_PRIVATE_NOTES;

    aLayers[aCount++] = LAYER_DEVICE_BACKGROUND;
    aLayers[aCount++] = LAYER_NOTES_BACKGROUND;
    aLayers[aCount++] = LAYER_SELECTION_SHADOWS;
}


