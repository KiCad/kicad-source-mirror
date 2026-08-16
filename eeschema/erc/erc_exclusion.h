/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * @author Jon Evans <jon@craftyjon.com>
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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef KICAD_ERC_EXCLUSION_H
#define KICAD_ERC_EXCLUSION_H

#include <memory>
#include <nlohmann/json_fwd.hpp>
#include <wx/string.h>

class SCH_MARKER;
class SCH_SHEET_LIST;

namespace kiapi::schematic
{
class ErcExclusion;
}


/**
 * Container for an ERC exclusion, which is a SCH_MARKER plus an optional comment.
 * This handles serialization to protobuf for the API and to JSON (via that protobuf)
 * for storage in settings.
 */
class ERC_EXCLUSION
{
public:
    ERC_EXCLUSION();
    ERC_EXCLUSION( const ERC_EXCLUSION& aOther );
    ERC_EXCLUSION& operator=( const ERC_EXCLUSION& aOther );
    ~ERC_EXCLUSION();

    static ERC_EXCLUSION FromMarker( const SCH_MARKER& aMarker );
    static ERC_EXCLUSION FromProto( const kiapi::schematic::ErcExclusion& aMessage );
    static ERC_EXCLUSION FromLegacyStrings( const SCH_SHEET_LIST& aSheetList, const wxString& aMarkerData,
                                            const wxString& aComment );

    const kiapi::schematic::ErcExclusion& ToProto() const;
    std::string GetSortKey() const;

    wxString GetComment() const;
    void     SetComment( const wxString& aComment );

    friend bool operator==( const ERC_EXCLUSION& a, const ERC_EXCLUSION& b )
    {
        return a.GetSortKey() == b.GetSortKey();
    }

    friend bool operator!=( const ERC_EXCLUSION& a, const ERC_EXCLUSION& b )
    {
        return !( a == b );
    }

private:
    struct IMPL;
    std::unique_ptr<IMPL> m_impl;

    friend void to_json( nlohmann::json& aJson, const ERC_EXCLUSION& aEx );
    friend void from_json( const nlohmann::json& aJson, ERC_EXCLUSION& aEx );
};

void to_json( nlohmann::json& aJson, const ERC_EXCLUSION& aEx );
void from_json( const nlohmann::json& aJson, ERC_EXCLUSION& aEx );

struct ERC_EXCLUSION_COMPARE
{
    bool operator()( const ERC_EXCLUSION& a, const ERC_EXCLUSION& b ) const
    {
        return a.GetSortKey() < b.GetSortKey();
    }
};

#endif //KICAD_ERC_EXCLUSION_H
