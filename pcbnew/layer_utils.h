/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers.
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

#include <wx/string.h>

#include <board.h>
#include <lseq.h>
#include <lset.h>


/**
 * Utility functions for dealing with layers in the context of a PCB board.s
 *
 * This includes functions that need access to the board to get layer names,
 * and other more complex, but reusable operations that either shouldn't be in LSET/LSEQ's
 * interface or need access to Pcbnew types.
 */
namespace LAYER_UTILS
{

/**
 * Accumulate layer names from a layer set into a comma separated string.
 *
 * @param aLayers is the list of layers to accumulate.
 * @param aBoard is the board to get layer names from, if null the default names
 *               are used.
 */
wxString AccumulateNames( const LSEQ& aLayers, const BOARD* aBoard );

/**
 * Accumulate layer names from a layer set into a comma separated string,
 * in UI order.
 */
inline wxString AccumulateNames( const LSET& aLayers, const BOARD* aBoard )
{
    return AccumulateNames( aLayers.UIOrder(), aBoard );
}

} // namespace LAYER_UTILS
