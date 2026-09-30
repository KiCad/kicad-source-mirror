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

#include <math/vector2d.h>
#include <vector>

class BOARD;
class PAD;
class PCB_VIA;


struct FAB_TEST_POINT
{
    const PAD*     m_pad = nullptr;
    const PCB_VIA* m_via = nullptr;
    int            m_netCode = 0;
    VECTOR2I       m_position;
    bool           m_front = false;
    bool           m_back = false;
    bool           m_frontMask = false;
    bool           m_backMask = false;
    int            m_drill = 0;
    bool           m_netEnd = true;
};


/// The caller must build board connectivity before enumerating net endpoints
std::vector<FAB_TEST_POINT> FabTestPoints( const BOARD& aBoard );
