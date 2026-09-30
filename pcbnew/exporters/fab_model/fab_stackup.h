/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
 *
 * This program is free software: you can redistribute it and/or
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

#include <board_stackup_manager/board_stackup.h>

class BOARD;


/** Board stackup snapshot synchronized without changing the source board */
class FAB_STACKUP
{
public:
    explicit FAB_STACKUP( const BOARD& aBoard );

    FAB_STACKUP( const FAB_STACKUP& ) = delete;
    FAB_STACKUP& operator=( const FAB_STACKUP& ) = delete;
    FAB_STACKUP( FAB_STACKUP&& ) = delete;
    FAB_STACKUP& operator=( FAB_STACKUP&& ) = delete;

    const BOARD_STACKUP&      Stackup() const { return m_stackup; }
    const BOARD_STACKUP_ITEM* ItemForLayer( PCB_LAYER_ID aLayer ) const;
    int                       Thickness() const { return m_stackup.BuildBoardThicknessFromStackup(); }

private:
    BOARD_STACKUP m_stackup;
};
