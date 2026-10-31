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

#pragma once

#include <kiid.h>
#include <wx/string.h>

#include <map>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

class SCHEMATIC;
enum class CONNECTION_TYPE;

namespace SCH_CONNECTIVITY
{
class FACADE;

/** One connection or bus member. The driver path is empty without a driver. */
struct DUMP_VALUE
{
    wxString                name;
    wxString                localName;
    CONNECTION_TYPE         type{};
    std::optional<KIID>     driver;
    KIID_PATH               driverPath;
    std::vector<DUMP_VALUE> members;
    std::set<wxString>      netclasses;

    bool operator==( const DUMP_VALUE& ) const = default;
};

struct DUMP_ROW
{
    DUMP_VALUE        value;
    bool              dangling = false;
    std::vector<bool> danglingEnds;
    std::set<KIID>    connected;

    bool operator==( const DUMP_ROW& ) const = default;
};

/** Rows by sheet path and item. Equal rows serialize to equal dumps and the reverse. */
using DUMP_ROWS = std::map<std::pair<KIID_PATH, KIID>, DUMP_ROW>;

/**
 * Canonical, pointer-free connectivity rows for migration and rebuild comparisons.
 * Session-local net and subgraph codes are deliberately omitted. The legacy `<NO NET>`
 * placeholder, including its sheet prefix, is normalized only for driverless connections.
 */
DUMP_ROWS CaptureRows( SCHEMATIC& aSchematic );

/** Same rows from a current facade publication, with isolated netclass resolution. */
DUMP_ROWS CaptureRows( SCHEMATIC& aSchematic, const FACADE& aFacade );

std::string Dump( const DUMP_ROWS& aRows );

/** @return the rows of CaptureRows() as canonical JSON. */
std::string Dump( SCHEMATIC& aSchematic );

/** @return the rows of CaptureRows() for a facade as canonical JSON. */
std::string Dump( SCHEMATIC& aSchematic, const FACADE& aFacade );
}
