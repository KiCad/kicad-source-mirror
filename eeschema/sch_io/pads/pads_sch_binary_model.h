/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <vector>

#include <widgets/report_severity.h>
#include <wx/string.h>

namespace PADS_SCH_BINARY
{

struct SOURCE_PROVENANCE
{
    wxString file;
    size_t   offset = 0;
    size_t   length = 0;
    int      controller = -1;
    int      sheet = -1;
};


enum class STRING_ENCODING_STATUS
{
    UTF8,
    CODE_PAGE,
    INVALID_BYTES,
    UNKNOWN_CODE_PAGE
};


struct SOURCE_STRING
{
    std::vector<uint8_t>   raw;
    wxString               text;
    STRING_ENCODING_STATUS encoding = STRING_ENCODING_STATUS::UTF8;
    SOURCE_PROVENANCE      source;
};


enum class PROPERTY_DISPOSITION
{
    EXACT,
    APPROXIMATE,
    PRESERVED,
    UNSUPPORTED
};


struct PARSER_DIAGNOSTIC
{
    SEVERITY          severity = RPT_SEVERITY_UNDEFINED;
    SOURCE_PROVENANCE source;
    wxString          message;
};


template <typename Tag>
class CONTROLLER_ID
{
public:
    constexpr CONTROLLER_ID() = default;
    explicit constexpr CONTROLLER_ID( uint32_t aValue ) :
            m_value( aValue )
    {
    }

    constexpr uint32_t Value() const { return m_value; }
    constexpr bool     IsValid() const { return m_value != INVALID; }

    auto operator<=>( const CONTROLLER_ID& ) const = default;

private:
    static constexpr uint32_t INVALID = std::numeric_limits<uint32_t>::max();
    uint32_t                  m_value = INVALID;
};


struct SHEET_ID_TAG;
struct DEFINITION_ID_TAG;
struct PIN_ID_TAG;
struct PART_TYPE_ID_TAG;
struct GATE_ID_TAG;
struct PLACEMENT_ID_TAG;
struct NET_ID_TAG;
struct BUS_ID_TAG;

using SHEET_ID = CONTROLLER_ID<SHEET_ID_TAG>;
using DEFINITION_ID = CONTROLLER_ID<DEFINITION_ID_TAG>;
using PIN_ID = CONTROLLER_ID<PIN_ID_TAG>;
using PART_TYPE_ID = CONTROLLER_ID<PART_TYPE_ID_TAG>;
using GATE_ID = CONTROLLER_ID<GATE_ID_TAG>;
using PLACEMENT_ID = CONTROLLER_ID<PLACEMENT_ID_TAG>;
using NET_ID = CONTROLLER_ID<NET_ID_TAG>;
using BUS_ID = CONTROLLER_ID<BUS_ID_TAG>;

// PADS source coordinates remain signed integer half-mils.  Angles are tenths of a degree,
// normalized to [0, 3600).  Conversion to KiCad units belongs to the schematic builder.
constexpr int64_t NormalizeCoordinate( int64_t aCoordinate )
{
    return aCoordinate;
}

constexpr int NormalizeAngle( int aAngle )
{
    int normalized = aAngle % 3600;
    return normalized < 0 ? normalized + 3600 : normalized;
}


struct SOURCE_POINT
{
    int64_t x = 0;
    int64_t y = 0;
};


struct DESIGN_SETTINGS
{
    SOURCE_PROVENANCE source;
    int64_t           coordinateUnitsPerMil = 2;
    SOURCE_POINT      pageSize;
};


struct MODEL_FIELD
{
    SOURCE_PROVENANCE    source;
    SOURCE_STRING        name;
    SOURCE_STRING        value;
    SOURCE_POINT         position;
    int                  angle = 0;
    bool                 visible = true;
    PROPERTY_DISPOSITION disposition = PROPERTY_DISPOSITION::EXACT;
};


enum class MODEL_GRAPHIC_KIND
{
    LINE,
    POLYLINE,
    RECTANGLE,
    CIRCLE,
    ARC,
    TEXT
};


struct MODEL_GRAPHIC
{
    SOURCE_PROVENANCE         source;
    MODEL_GRAPHIC_KIND        kind = MODEL_GRAPHIC_KIND::LINE;
    std::vector<SOURCE_POINT> points;
    SOURCE_STRING             text;
    int64_t                   width = 0;
};


struct MODEL_PIN_DEFINITION
{
    PIN_ID            id;
    SOURCE_PROVENANCE source;
    SOURCE_STRING     number;
    SOURCE_STRING     name;
    SOURCE_POINT      position;
    int               angle = 0;
    uint32_t          electricalType = 0;
    uint32_t          graphicStyle = 0;
};


struct MODEL_SYMBOL_DEFINITION
{
    DEFINITION_ID                     id;
    SOURCE_PROVENANCE                 source;
    SOURCE_STRING                     name;
    std::vector<MODEL_GRAPHIC>        graphics;
    std::vector<MODEL_PIN_DEFINITION> pins;
    std::vector<MODEL_FIELD>          fields;
};


struct MODEL_GATE
{
    GATE_ID             id;
    SOURCE_PROVENANCE   source;
    DEFINITION_ID       definition;
    uint32_t            unit = 1;
    std::vector<PIN_ID> pins;
};


struct MODEL_PART_TYPE
{
    PART_TYPE_ID             id;
    SOURCE_PROVENANCE        source;
    SOURCE_STRING            name;
    std::vector<MODEL_GATE>  gates;
    std::vector<MODEL_FIELD> fields;
};


struct MODEL_SHEET
{
    SHEET_ID                id;
    size_t                  index = 0;
    SOURCE_PROVENANCE       source;
    SOURCE_STRING           name;
    std::optional<SHEET_ID> parent;
};


struct MODEL_PLACEMENT
{
    PLACEMENT_ID             id;
    SOURCE_PROVENANCE        source;
    SHEET_ID                 sheet;
    PART_TYPE_ID             partType;
    std::optional<GATE_ID>   gate;
    SOURCE_STRING            reference;
    SOURCE_POINT             position;
    int                      angle = 0;
    bool                     mirrored = false;
    std::vector<MODEL_FIELD> fields;
};


struct MODEL_CONNECTION_ENDPOINT
{
    std::optional<PLACEMENT_ID> placement;
    std::optional<PIN_ID>       pin;
    SOURCE_POINT                point;
};


struct MODEL_CONNECTION
{
    SOURCE_PROVENANCE                      source;
    std::vector<MODEL_CONNECTION_ENDPOINT> endpoints;
    std::vector<SOURCE_POINT>              vertices;
};


struct MODEL_NET
{
    NET_ID                        id;
    SOURCE_PROVENANCE             source;
    SHEET_ID                      sheet;
    SOURCE_STRING                 name;
    std::vector<MODEL_CONNECTION> connections;
};


struct MODEL_BUS
{
    BUS_ID                    id;
    SOURCE_PROVENANCE         source;
    SHEET_ID                  sheet;
    SOURCE_STRING             name;
    std::vector<SOURCE_POINT> vertices;
};


enum class MODEL_LABEL_KIND
{
    LOCAL,
    GLOBAL,
    HIERARCHICAL,
    POWER,
    BUS
};


struct MODEL_LABEL
{
    SOURCE_PROVENANCE source;
    SHEET_ID          sheet;
    MODEL_LABEL_KIND  kind = MODEL_LABEL_KIND::LOCAL;
    SOURCE_STRING     text;
    SOURCE_POINT      position;
    int               angle = 0;
};


struct MODEL_JUNCTION
{
    SOURCE_PROVENANCE source;
    SHEET_ID          sheet;
    SOURCE_POINT      position;
};


struct MODEL_TEXT
{
    SOURCE_PROVENANCE source;
    SHEET_ID          sheet;
    SOURCE_STRING     text;
    SOURCE_POINT      position;
    int               angle = 0;
};


struct PADS_SCH_MODEL
{
    uint16_t                             version = 0;
    SOURCE_PROVENANCE                    source;
    DESIGN_SETTINGS                      settings;
    std::vector<MODEL_SHEET>             sheets;
    std::vector<MODEL_SYMBOL_DEFINITION> definitions;
    std::vector<MODEL_PART_TYPE>         partTypes;
    std::vector<MODEL_PLACEMENT>         placements;
    std::vector<MODEL_NET>               nets;
    std::vector<MODEL_BUS>               buses;
    std::vector<MODEL_LABEL>             labels;
    std::vector<MODEL_JUNCTION>          junctions;
    std::vector<MODEL_TEXT>              texts;
    std::vector<MODEL_GRAPHIC>           graphics;
    std::vector<PARSER_DIAGNOSTIC>       diagnostics;

    bool HasUniqueTypedIds() const;
    bool AllReferencesResolved() const;
    void ValidateOrThrow() const;
};

} // namespace PADS_SCH_BINARY
