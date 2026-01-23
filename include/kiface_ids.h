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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef KIFACE_IDS_H
#define KIFACE_IDS_H

/**
 * IDs of objects that may be returned by KIFACE::IfaceOrAddress.
 */
enum KIFACE_ADDR_ID : int
{
    KIFACE_ID_INVALID,

    KIFACE_FOOTPRINT_LIBRARY_ADAPTER,

    /// Function pointer type: wxString (*)(const wxString& aFilterJson)
    /// Input JSON: {"pin_count": N, "filters": ["pattern1", ...], "zero_filters": bool, "max_results": N}
    /// Output JSON: ["lib:footprint1", "lib:footprint2", ...]
    KIFACE_FILTER_FOOTPRINTS,

    KIFACE_LOAD_SCHEMATIC,
    KIFACE_NETLIST_SCHEMATIC,
    KIFACE_SCRIPTING_LEGACY,
    KIFACE_SCRIPTING,

    KIFACE_TEST_FOOTPRINT_LINK,
    KIFACE_TEST_FOOTPRINT_LINK_NO_LIBRARY,
    KIFACE_TEST_FOOTPRINT_LINK_LIBRARY_NOT_ENABLED,
    KIFACE_TEST_FOOTPRINT_LINK_NO_FOOTPRINT
};

#endif // KIFACE_IDS
