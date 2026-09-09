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

#ifndef IMPORT_NET_MAP_H
#define IMPORT_NET_MAP_H

#include <kiid.h>
#include <wx/string.h>
#include <cstdint>
#include <iosfwd>
#include <map>
#include <vector>

class REPORTER;

/** How completely a source net survived conversion.  Only RESOLVED nets can be renamed on a board. */
enum class IMPORT_NET_STATUS
{
    RESOLVED,    ///< exactly one physical net carries the source net
    SPLIT,       ///< the source net reaches more than one physical net
    BUS,         ///< the source net is carried by a bus
    NO_CONNECT,  ///< the source marks the net as deliberately unconnected
    UNCONNECTED  ///< the source net reaches no physical net
};

/// Name the status, so test diagnostics read as a status rather than an ordinal.
std::ostream& operator<<( std::ostream& aStream, IMPORT_NET_STATUS aStatus );

struct IMPORT_NET_TERMINAL
{
    KIID symbolUuid;
    int unit = 0;
    uint32_t sourcePinId = 0;
    wxString pinNumber;
    unsigned duplicateIndex = 0;
};

struct IMPORT_NET_MAP_ENTRY
{
    wxString view;
    std::vector<wxString> occurrence;
    uint32_t sourceNetId = 0;
    wxString originalName;
    wxString generatedName;
    wxString nameAtImport;
    IMPORT_NET_STATUS status = IMPORT_NET_STATUS::UNCONNECTED;
    std::vector<IMPORT_NET_TERMINAL> terminals;
    std::vector<KIID> itemUuids;
};

/**
 * Import provenance, held only for the lifetime of the importing SCHEMATIC.
 *
 * Connectivity must never depend on this data, and it is never written to the project.  The
 * schematic import hands the reduced name map to the board import in memory, so nothing is left
 * beside the converted files.
 */
struct IMPORT_NET_MAP
{
    std::vector<IMPORT_NET_MAP_ENTRY> entries;
};

/**
 * Reduce the map to the board net renames it justifies.
 *
 * Only a source net that one physical net carries can be renamed, and only where the source name
 * was machine-generated, since that is the name the paired board also uses.  A generated name that
 * resolved to more than one KiCad name is reported and omitted rather than guessed at.
 */
std::map<wxString, wxString> GetBoardNetNameMap( const IMPORT_NET_MAP& aMap, REPORTER& aReporter );

#endif
