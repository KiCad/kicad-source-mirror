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

#ifndef KICAD_DRC_EXCLUSION_H
#define KICAD_DRC_EXCLUSION_H

#include <memory>
#include <nlohmann/json_fwd.hpp>

class PCB_MARKER;

namespace kiapi::board
{
class DrcExclusion;
}


/**
 * Container for an DRC exclusion, which is a PCB_MARKER plus an optional comment.
 * This handles serialization to protobuf for the API and to JSON (via that protobuf)
 * for storage in settings.
 */
class DRC_EXCLUSION
{
public:
    DRC_EXCLUSION();
    DRC_EXCLUSION( const DRC_EXCLUSION& aOther );
    DRC_EXCLUSION& operator=( const DRC_EXCLUSION& aOther );
    ~DRC_EXCLUSION();

    static DRC_EXCLUSION FromMarker( const PCB_MARKER& aMarker );
    static DRC_EXCLUSION FromProto( const kiapi::board::DrcExclusion& aMessage );
    static DRC_EXCLUSION FromLegacyStrings( const wxString& aMarkerData, const wxString& aComment );

    const kiapi::board::DrcExclusion& ToProto() const;
    std::string GetSortKey() const;

    wxString GetComment() const;
    void SetComment( const wxString& aComment );

    friend bool operator==( const DRC_EXCLUSION& a, const DRC_EXCLUSION& b )
    {
        return a.GetSortKey() == b.GetSortKey();
    }

    friend bool operator!=( const DRC_EXCLUSION& a, const DRC_EXCLUSION& b )
    {
        return !( a == b );
    }

private:
    struct IMPL;
    std::unique_ptr<IMPL> m_impl;

    friend void to_json( nlohmann::json& aJson, const DRC_EXCLUSION& aEx );
    friend void from_json( const nlohmann::json& aJson, DRC_EXCLUSION& aEx );
};

void to_json( nlohmann::json& aJson, const DRC_EXCLUSION& aEx );
void from_json( const nlohmann::json& aJson, DRC_EXCLUSION& aEx );

struct DRC_EXCLUSION_COMPARE
{
    bool operator()( const DRC_EXCLUSION& a, const DRC_EXCLUSION& b ) const
    {
        return a.GetSortKey() < b.GetSortKey();
    }
};

#endif //KICAD_DRC_EXCLUSION_H
