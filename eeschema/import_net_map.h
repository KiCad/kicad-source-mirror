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
#include <optional>
#include <vector>

class REPORTER;

struct IMPORT_NET_TERMINAL
{
    KIID symbolUuid;
    int unit = 0;
    uint32_t sourcePinId = 0;
    wxString pinNumber;
    unsigned duplicateIndex = 0;
    std::optional<KIID> pinUuid;
};

struct IMPORT_NET_MAP_ENTRY
{
    wxString view;
    std::vector<wxString> occurrence;
    uint32_t sourceNetId = 0;
    wxString originalName;
    wxString nameAtImport;
    wxString status;
    std::vector<IMPORT_NET_TERMINAL> terminals;
    std::vector<KIID> itemUuids;
};

/** Import provenance only. Connectivity must never depend on this data. */
struct IMPORT_NET_MAP
{
    wxString sourceDesignDigest;
    std::vector<IMPORT_NET_MAP_ENTRY> entries;
};

wxString ImportNetMapPath( const wxString& aRootPath );
bool WriteImportNetMap( const IMPORT_NET_MAP& aMap, const KIID& aRootUuid,
                        const wxString& aRootPath, REPORTER& aReporter );
std::optional<IMPORT_NET_MAP> ReadImportNetMap( const KIID& aRootUuid,
                                              const wxString& aRootPath, REPORTER& aReporter );

#endif
