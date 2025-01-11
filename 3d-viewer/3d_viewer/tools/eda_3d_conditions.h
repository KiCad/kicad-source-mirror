/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Ian McInerney <ian.s.mcinerney at ieee.org>
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

#ifndef _3D_CONDITIONS_H_
#define _3D_CONDITIONS_H_

#include <3d_enums.h>
#include <tool/selection.h>
#include <tool/selection_conditions.h>


class BOARD_ADAPTER;

class EDA_3D_CONDITIONS : public SELECTION_CONDITIONS
{
public:
    /**
     * Define conditions for a 3D viewer frame.
     *
     * @param aAdapter is the board adapter to query for information.
     */
    EDA_3D_CONDITIONS( BOARD_ADAPTER* aAdapter ) :
        m_adapter( aAdapter )
    {}

    /**
     * Creates a functor that tests the current grid size.
     *
     * @param aAdapter is the board adapter the setting is in
     * @param aGridSize is the grid size to test for.
     * @return Functor testing if the flag is set.
     */
    SELECTION_CONDITION GridSize( GRID3D_TYPE aGridSize );

private:
    /// Helper function used by GridDize().
    static bool gridSizeFunction( const SELECTION& aSelection, BOARD_ADAPTER* aAdapter,
                                  GRID3D_TYPE aGridSize );

    /// The board adapter to read the 3D viewer state from.
    BOARD_ADAPTER* m_adapter;
};

#endif /* _3D_CONDITIONS_H_ */
