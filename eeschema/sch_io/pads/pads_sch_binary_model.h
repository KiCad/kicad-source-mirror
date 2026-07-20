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
    uint16_t version = 0;
    wxString objectClass;
    int      controller = -1;
    size_t   recordIndex = 0;
    size_t   absoluteOffset = 0;
    size_t   length = 0;
    int      sheet = -1;

    bool operator==( const SOURCE_PROVENANCE& ) const = default;
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
    uint32_t               codePage = 65001;
    wxString               codePageName = wxS( "UTF-8" );

    bool operator==( const SOURCE_STRING& ) const = default;
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

    bool operator==( const PARSER_DIAGNOSTIC& ) const = default;
};

wxString FormatParserError( const SOURCE_PROVENANCE& aSource, const wxString& aMessage );


struct SOURCE_PROPERTY
{
    SOURCE_STRING        name;
    SOURCE_STRING        value;
    PROPERTY_DISPOSITION disposition = PROPERTY_DISPOSITION::EXACT;
    SOURCE_PROVENANCE    source;

    bool operator==( const SOURCE_PROPERTY& ) const = default;
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
struct FIELD_ID_TAG;
struct PART_TYPE_ID_TAG;
struct GATE_ID_TAG;
struct PLACEMENT_ID_TAG;
struct NET_ID_TAG;
struct BUS_ID_TAG;

using SHEET_ID = CONTROLLER_ID<SHEET_ID_TAG>;
using DEFINITION_ID = CONTROLLER_ID<DEFINITION_ID_TAG>;
using PIN_ID = CONTROLLER_ID<PIN_ID_TAG>;
using FIELD_ID = CONTROLLER_ID<FIELD_ID_TAG>;
using PART_TYPE_ID = CONTROLLER_ID<PART_TYPE_ID_TAG>;
using GATE_ID = CONTROLLER_ID<GATE_ID_TAG>;
using PLACEMENT_ID = CONTROLLER_ID<PLACEMENT_ID_TAG>;
using NET_ID = CONTROLLER_ID<NET_ID_TAG>;
using BUS_ID = CONTROLLER_ID<BUS_ID_TAG>;

template <typename Id>
struct CONTROLLER_REFERENCE
{
    Id                id;
    SOURCE_PROVENANCE source;

    bool operator==( const CONTROLLER_REFERENCE& ) const = default;
};

using SHEET_REFERENCE = CONTROLLER_REFERENCE<SHEET_ID>;
using DEFINITION_REFERENCE = CONTROLLER_REFERENCE<DEFINITION_ID>;
using PIN_REFERENCE = CONTROLLER_REFERENCE<PIN_ID>;
using PART_TYPE_REFERENCE = CONTROLLER_REFERENCE<PART_TYPE_ID>;
using GATE_REFERENCE = CONTROLLER_REFERENCE<GATE_ID>;
using PLACEMENT_REFERENCE = CONTROLLER_REFERENCE<PLACEMENT_ID>;
using NET_REFERENCE = CONTROLLER_REFERENCE<NET_ID>;

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
    int64_t           x = 0;
    int64_t           y = 0;
    SOURCE_PROVENANCE source;

    bool operator==( const SOURCE_POINT& ) const = default;
};


enum class MODEL_LINE_STYLE
{
    DEFAULT,
    SOLID,
    DASH,
    DOT,
    DASH_DOT
};


enum class MODEL_FILL_STYLE
{
    NONE,
    FILLED,
    HATCHED
};


enum class MODEL_JUSTIFICATION
{
    LEFT,
    CENTER,
    RIGHT
};


struct MODEL_TEXT_PRESENTATION
{
    SOURCE_PROVENANCE            source;
    int64_t                      height = 0;
    int64_t                      width = 0;
    SOURCE_STRING                font;
    MODEL_JUSTIFICATION          horizontalJustification = MODEL_JUSTIFICATION::LEFT;
    MODEL_JUSTIFICATION          verticalJustification = MODEL_JUSTIFICATION::CENTER;
    bool                         bold = false;
    bool                         italic = false;
    bool                         underline = false;
    bool                         visible = true;
    std::vector<SOURCE_PROPERTY> properties;

    bool operator==( const MODEL_TEXT_PRESENTATION& ) const = default;
};


struct DESIGN_SETTINGS
{
    SOURCE_PROVENANCE            source;
    uint32_t                     codePage = 1252;
    int64_t                      coordinateUnitsPerMil = 2;
    SOURCE_POINT                 pageSize;
    int64_t                      defaultLineWidth = 0;
    int64_t                      defaultBusWidth = 0;
    std::vector<SOURCE_PROPERTY> properties;

    bool operator==( const DESIGN_SETTINGS& ) const = default;
};


struct MODEL_FIELD
{
    FIELD_ID                     id;
    SOURCE_PROVENANCE            source;
    SOURCE_STRING                name;
    SOURCE_STRING                value;
    SOURCE_POINT                 position;
    int                          angle = 0;
    bool                         visible = true;
    PROPERTY_DISPOSITION         disposition = PROPERTY_DISPOSITION::EXACT;
    MODEL_TEXT_PRESENTATION      presentation;
    std::vector<SOURCE_PROPERTY> properties;

    bool operator==( const MODEL_FIELD& ) const = default;
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
    SOURCE_PROVENANCE            source;
    MODEL_GRAPHIC_KIND           kind = MODEL_GRAPHIC_KIND::LINE;
    std::vector<SOURCE_POINT>    points;
    SOURCE_STRING                text;
    MODEL_LINE_STYLE             lineStyle = MODEL_LINE_STYLE::DEFAULT;
    int64_t                      strokeWidth = 0;
    MODEL_FILL_STYLE             fill = MODEL_FILL_STYLE::NONE;
    MODEL_TEXT_PRESENTATION      presentation;
    int                          angle = 0;
    SOURCE_POINT                 arcCenter;
    SOURCE_POINT                 arcBoundsStart;
    SOURCE_POINT                 arcBoundsEnd;
    int                          arcSweepAngle = 0;
    bool                         arcClockwise = false;
    std::vector<SOURCE_PROPERTY> properties;

    bool operator==( const MODEL_GRAPHIC& ) const = default;
};


struct MODEL_PAGE_GRAPHIC
{
    SOURCE_PROVENANCE source;
    SHEET_REFERENCE   sheet;
    MODEL_GRAPHIC     graphic;

    bool operator==( const MODEL_PAGE_GRAPHIC& ) const = default;
};


struct MODEL_PIN_DEFINITION
{
    PIN_ID                       id;
    SOURCE_PROVENANCE            source;
    SOURCE_STRING                number;
    SOURCE_STRING                name;
    SOURCE_POINT                 position;
    uint32_t                     side = 0;
    int                          angle = 0;
    uint32_t                     electricalType = 0;
    uint32_t                     graphicStyle = 0;
    int64_t                      length = 0;
    MODEL_TEXT_PRESENTATION      presentation;
    MODEL_TEXT_PRESENTATION      namePresentation;
    MODEL_TEXT_PRESENTATION      numberPresentation;
    SOURCE_POINT                 nameOffset;
    SOURCE_POINT                 numberOffset;
    int                          nameAngle = 0;
    int                          numberAngle = 0;
    uint16_t                     nameJustification = 0;
    uint16_t                     numberJustification = 0;
    int                          nameOffsetAngle = 0;
    int                          numberOffsetAngle = 0;
    uint16_t                     nameOffsetJustification = 0;
    uint16_t                     numberOffsetJustification = 0;
    uint16_t                     visibilityFlags = 0;
    std::vector<SOURCE_PROPERTY> properties;

    bool operator==( const MODEL_PIN_DEFINITION& ) const = default;
};


struct MODEL_SYMBOL_DEFINITION
{
    DEFINITION_ID                     id;
    SOURCE_PROVENANCE                 source;
    SOURCE_STRING                     name;
    std::vector<MODEL_GRAPHIC>        graphics;
    std::vector<MODEL_PIN_DEFINITION> pins;
    std::vector<MODEL_FIELD>          fields;
    std::vector<SOURCE_PROPERTY>      properties;

    bool operator==( const MODEL_SYMBOL_DEFINITION& ) const = default;
};


struct MODEL_CONNECTOR_PIN
{
    SOURCE_PROVENANCE source;
    SOURCE_STRING     number;
    SOURCE_STRING     name;
    uint32_t          electricalType = 0;
    uint8_t           swapGroup = 0;
    uint16_t          flags = 0;

    bool operator==( const MODEL_CONNECTOR_PIN& ) const = default;
};


struct MODEL_GATE
{
    GATE_ID                           id;
    SOURCE_PROVENANCE                 source;
    DEFINITION_REFERENCE              definition;
    uint32_t                          unit = 1;
    std::vector<PIN_REFERENCE>        pins;
    std::vector<DEFINITION_REFERENCE> alternateDefinitions;
    std::vector<DEFINITION_REFERENCE> decalGroupMembers;
    std::vector<MODEL_CONNECTOR_PIN>   connectorPins;
    std::vector<SOURCE_PROPERTY>      properties;

    bool operator==( const MODEL_GATE& ) const = default;
};


struct MODEL_SIGNAL_PIN
{
    SOURCE_PROVENANCE source;
    SOURCE_STRING     number;
    SOURCE_STRING     name;

    bool operator==( const MODEL_SIGNAL_PIN& ) const = default;
};


struct MODEL_PART_TYPE
{
    PART_TYPE_ID                  id;
    SOURCE_PROVENANCE             source;
    SOURCE_STRING                 name;
    std::vector<MODEL_GATE>       gates;
    std::vector<MODEL_FIELD>      fields;
    std::vector<SOURCE_PROPERTY>  properties;
    std::vector<MODEL_SIGNAL_PIN> signalPins;

    bool operator==( const MODEL_PART_TYPE& ) const = default;
};


struct MODEL_SHEET
{
    SHEET_ID                       id;
    size_t                         index = 0;
    SOURCE_PROVENANCE              source;
    SOURCE_STRING                  name;
    std::optional<SHEET_REFERENCE> parent;
    SOURCE_POINT                   pageSize;
    int64_t                        defaultLineWidth = 0;
    int64_t                        defaultBusWidth = 0;
    SOURCE_STRING                  title;
    std::vector<MODEL_GRAPHIC>     border;
    std::vector<MODEL_FIELD>       titleBlockFields;
    std::vector<SOURCE_PROPERTY>   properties;

    bool operator==( const MODEL_SHEET& ) const = default;
};


struct MODEL_PLACEMENT
{
    PLACEMENT_ID                  id;
    SOURCE_PROVENANCE             source;
    SHEET_REFERENCE               sheet;
    PART_TYPE_REFERENCE           partType;
    std::optional<GATE_REFERENCE> gate;
    uint32_t                      unit = 1;
    SOURCE_STRING                 reference;
    SOURCE_POINT                  position;
    int                           angle = 0;
    bool                          mirrored = false;
    std::vector<MODEL_FIELD>      fields;
    std::vector<SOURCE_PROPERTY>  properties;

    bool operator==( const MODEL_PLACEMENT& ) const = default;
};


enum class MODEL_ENDPOINT_KIND
{
    INVALID,
    POINT,
    PIN
};


struct MODEL_CONNECTION_ENDPOINT
{
    MODEL_ENDPOINT_KIND                kind = MODEL_ENDPOINT_KIND::INVALID;
    SOURCE_PROVENANCE                  source;
    std::optional<PLACEMENT_REFERENCE> placement;
    std::optional<PIN_REFERENCE>       pin;
    SOURCE_POINT                       point;
    std::vector<SOURCE_PROPERTY>       properties;

    bool operator==( const MODEL_CONNECTION_ENDPOINT& ) const = default;
};


struct MODEL_CONNECTION
{
    SOURCE_PROVENANCE                      source;
    std::vector<MODEL_CONNECTION_ENDPOINT> endpoints;
    std::vector<SOURCE_POINT>              vertices;
    std::vector<SOURCE_PROPERTY>           properties;

    bool operator==( const MODEL_CONNECTION& ) const = default;
};


struct MODEL_NET
{
    NET_ID                        id;
    SOURCE_PROVENANCE             source;
    SHEET_REFERENCE               sheet;
    SOURCE_STRING                 name;
    std::vector<MODEL_CONNECTION> connections;
    std::vector<SOURCE_PROPERTY>  properties;

    bool operator==( const MODEL_NET& ) const = default;
};


struct MODEL_BUS_ENTRY
{
    SOURCE_PROVENANCE            source;
    SOURCE_POINT                 position;
    NET_REFERENCE                memberNet;
    std::vector<SOURCE_PROPERTY> properties;

    bool operator==( const MODEL_BUS_ENTRY& ) const = default;
};


struct MODEL_BUS
{
    BUS_ID                       id;
    SOURCE_PROVENANCE            source;
    SHEET_REFERENCE              sheet;
    SOURCE_STRING                name;
    std::vector<SOURCE_POINT>    vertices;
    std::vector<MODEL_BUS_ENTRY> entries;
    std::vector<SOURCE_STRING>   aliases;
    std::vector<NET_REFERENCE>   memberNets;
    std::vector<SOURCE_PROPERTY> properties;

    bool operator==( const MODEL_BUS& ) const = default;
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
    SOURCE_PROVENANCE            source;
    SHEET_REFERENCE              sheet;
    MODEL_LABEL_KIND             kind = MODEL_LABEL_KIND::LOCAL;
    SOURCE_STRING                text;
    SOURCE_POINT                 position;
    int                          angle = 0;
    MODEL_TEXT_PRESENTATION      presentation;
    std::vector<SOURCE_PROPERTY> properties;

    bool operator==( const MODEL_LABEL& ) const = default;
};


struct MODEL_JUNCTION
{
    SOURCE_PROVENANCE            source;
    SHEET_REFERENCE              sheet;
    SOURCE_POINT                 position;
    std::vector<SOURCE_PROPERTY> properties;

    bool operator==( const MODEL_JUNCTION& ) const = default;
};


struct MODEL_TEXT
{
    SOURCE_PROVENANCE            source;
    SHEET_REFERENCE              sheet;
    SOURCE_STRING                text;
    SOURCE_POINT                 position;
    int                          angle = 0;
    MODEL_TEXT_PRESENTATION      presentation;
    std::vector<SOURCE_PROPERTY> properties;

    bool operator==( const MODEL_TEXT& ) const = default;
};


struct PRESERVED_CONTROLLER_PAYLOAD
{
    SOURCE_PROVENANCE    source;
    PROPERTY_DISPOSITION disposition = PROPERTY_DISPOSITION::PRESERVED;
    std::vector<uint8_t> bytes;

    bool operator==( const PRESERVED_CONTROLLER_PAYLOAD& ) const = default;
};


struct PADS_SCH_MODEL
{
    uint16_t                                  version = 0;
    uint16_t                                  subversion = 0;
    SOURCE_PROVENANCE                         source;
    DESIGN_SETTINGS                           settings;
    std::vector<MODEL_SHEET>                  sheets;
    std::vector<MODEL_SYMBOL_DEFINITION>      definitions;
    std::vector<MODEL_PART_TYPE>              partTypes;
    std::vector<MODEL_PLACEMENT>              placements;
    std::vector<MODEL_NET>                    nets;
    std::vector<MODEL_BUS>                    buses;
    std::vector<MODEL_LABEL>                  labels;
    std::vector<MODEL_JUNCTION>               junctions;
    std::vector<MODEL_TEXT>                   texts;
    std::vector<MODEL_PAGE_GRAPHIC>           graphics;
    std::vector<PRESERVED_CONTROLLER_PAYLOAD> preservedControllerPayloads;
    std::vector<PARSER_DIAGNOSTIC>            diagnostics;

    bool HasUniqueTypedIds() const;
    bool AllReferencesResolved() const;
    void ValidateOrThrow() const;

    bool operator==( const PADS_SCH_MODEL& ) const = default;
};

} // namespace PADS_SCH_BINARY
