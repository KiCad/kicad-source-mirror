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

#ifndef ORCAD_CIS_H_
#define ORCAD_CIS_H_

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>


using ORCAD_CIS_PROPERTIES = std::map<std::string, std::string>;
using ORCAD_CIS_SCHEMATIC_INFO = std::map<std::string, std::map<uint32_t, ORCAD_CIS_PROPERTIES>>;


std::vector<std::string> OrcadCisParseCountedList( const std::vector<char>& aData, uint8_t aSeparator );

std::map<uint32_t, std::map<std::string, std::string>> OrcadCisParsePropertyUpdates( const std::vector<char>& aData );

std::map<uint32_t, bool> OrcadCisParseMemberships( const std::vector<char>& aData );

ORCAD_CIS_SCHEMATIC_INFO OrcadCisParseSchematicInfo( const std::vector<char>& aData );

std::string OrcadCisSelectVariant( const std::vector<std::string>&   aNames,
                                   const std::optional<std::string>& aRequested );

#endif // ORCAD_CIS_H_
