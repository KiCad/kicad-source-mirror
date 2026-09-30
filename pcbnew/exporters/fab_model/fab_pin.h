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

#include <cstddef>
#include <wx/string.h>

class PAD;

enum class FAB_PAD_ROLE
{
    NONE,
    BGA,
    FIDUCIAL_GLOBAL,
    FIDUCIAL_LOCAL,
    TESTPOINT,
    HEATSINK,
    CASTELLATED,
    MECHANICAL,
    PRESSFIT,
    TOOLING_HOLE
};

enum class FAB_MOUNT
{
    SMT,
    THT,
    HOLE,
    OTHER
};

enum class FAB_ELECTRICAL
{
    ELECTRICAL,
    MECHANICAL,
    UNDEFINED
};

struct FAB_PIN
{
    wxString       m_name;
    FAB_PAD_ROLE   m_role = FAB_PAD_ROLE::NONE;
    FAB_MOUNT      m_mount = FAB_MOUNT::OTHER;
    FAB_ELECTRICAL m_electrical = FAB_ELECTRICAL::UNDEFINED;
};

/// aIndexInFootprint is the pad's position in its footprint, used to name NPTH and unnumbered pads
FAB_PIN MakeFabPin( const PAD& aPad, size_t aIndexInFootprint );

/// Explicit fabrication properties take precedence over the unnumbered NPTH fallback
FAB_PAD_ROLE GetFabPadRole( const PAD& aPad );
