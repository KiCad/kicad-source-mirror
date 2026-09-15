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

#include <cstdint>
#include <map>
#include <optional>
#include <vector>
#include <geometry/seg.h>
#include <geometry/shape_poly_set.h>
#include <kiid.h>
#include <lib_id.h>
#include <lib_symbol_attributes.h>
#include <eda_shape.h>
#include <font/text_attributes.h>
#include <template_fieldnames.h>
#include <core/typeinfo.h>
#include <jumper_group.h>
#include <pin_type.h>
#include <pin_map.h>
#include <pin_comparison.h>
#include <sim/sim_model_input.h>
#include <set>
#include <wx/string.h>
#include <wx/arrstr.h>

class LIB_SYMBOL;
class SCH_ITEM;
class SCH_SCREEN;
class SCH_SYMBOL;
class SCH_FIELD;
class SCH_SHEET_PATH;

namespace SCH_CONNECTIVITY
{
enum class PORT_KIND
{
    ANCHOR,
    WIRE,
    BUS,
    ENTRY,
    ENTRY_BUS
};

struct PORT_FACT
{
    VECTOR2I  position;
    PORT_KIND kind = PORT_KIND::ANCHOR;
    bool      operator==( const PORT_FACT& ) const = default;
};

struct PIN_FACT
{
    KIID               id = niluuid;
    VECTOR2I           position;
    int                unit = 0;
    ELECTRICAL_PINTYPE type = ELECTRICAL_PINTYPE::PT_UNSPECIFIED;
    wxString           libraryName;
    wxString           name;
    wxString           shownName;
    wxString           shownNumber;
    wxString           number;
    wxString           padNumber;
    bool               canDrive = false;
    bool               globalPower = false;
    bool               localPower = false;
    bool               invisible = false;
    bool               globalPowerParent = false;
    bool               localPowerParent = false;
    bool               operator==( const PIN_FACT& ) const = default;
};

struct ITEM_FACT
{
    KIID                   id = niluuid;
    KICAD_T                type = TYPE_NOT_INIT;
    KIID                   owner = niluuid;
    std::vector<PORT_FACT> ports;
    std::optional<SEG>     segment;
    int                    lineWidth = 0;
    std::vector<PIN_FACT>  pins;
    std::vector<KIID>      jumperedWith;
    wxString               rawText;
    std::vector<wxString>  netclassFields;
    bool                   outputShape = false;
    bool                   multiUnit = false;
    bool                   operator==( const ITEM_FACT& ) const = default;
};

struct RULE_AREA_FACT
{
    KIID              id = niluuid;
    SHAPE_POLY_SET    polygon;
    std::vector<KIID> containedItems;
    std::vector<KIID> attachedDirectives;
    bool              operator==( const RULE_AREA_FACT& aOther ) const;
};

struct INSTANCE_RULE_AREA_FACT
{
    KIID                  id = niluuid;
    std::vector<KIID>     containedItems;
    std::vector<KIID>     attachedDirectives;
    std::vector<wxString> netclasses;
    bool                  operator==( const INSTANCE_RULE_AREA_FACT& ) const = default;
};

struct FIELD_NAME_FACT
{
    KIID     owner;
    KIID     field;
    VECTOR2I position;
    wxString name;
    bool     operator==( const FIELD_NAME_FACT& ) const = default;
};

struct UNIT_FACT
{
    wxString name;
    bool     powerInput = false;
    bool     input = false;
    bool     bidirectional = false;
    bool     operator==( const UNIT_FACT& ) const = default;
};

struct MULTI_UNIT_FACT
{
    KIID                   id;
    VECTOR2I               position;
    std::vector<UNIT_FACT> units;
    bool                   operator==( const MULTI_UNIT_FACT& ) const = default;
};

struct MULTI_UNIT_REFERENCE
{
    KIID     id;
    wxString reference;
    wxString name;
    wxString footprint;
    int      unit = 0;
    bool     operator==( const MULTI_UNIT_REFERENCE& ) const = default;
};

struct PIN_MAP_FACT
{
    KIID                              id = niluuid;
    VECTOR2I                          position;
    PIN_MAP_SET                       maps;
    std::vector<ASSOCIATED_FOOTPRINT> footprints;
    std::set<wxString>                pinNumbers;
    JUMPER_GROUP_SET                  jumperGroups;
    bool                              operator==( const PIN_MAP_FACT& ) const = default;
};

struct FOOTPRINT_FACT
{
    KIID          id = niluuid;
    VECTOR2I      position;
    wxArrayString filters;
    bool          operator==( const FOOTPRINT_FACT& ) const = default;
};

struct LIBRARY_SHAPE_FACT
{
    KIID      id = niluuid;
    int       unit = 0;
    int       bodyStyle = 0;
    bool      isPrivate = false;
    VECTOR2I  position;
    EDA_SHAPE geometry{ SHAPE_T::SEGMENT, 0, FILL_T::NO_FILL };

    int  Compare( const LIBRARY_SHAPE_FACT& aOther, int aCompareFlags ) const;
    bool operator==( const LIBRARY_SHAPE_FACT& aOther ) const;
};

struct LIBRARY_FIELD_FACT
{
    FIELD_T         id = FIELD_T::USER;
    bool            mandatory = false;
    wxString        name;
    wxString        text;
    VECTOR2I        position;
    bool            visible = false;
    bool            nameShown = false;
    bool            isPrivate = false;
    uintptr_t       fontIdentity = 0;
    TEXT_ATTRIBUTES style;

    bool Matches( const LIBRARY_FIELD_FACT& aOther, int aCompareFlags ) const;
    bool operator==( const LIBRARY_FIELD_FACT& ) const = default;
};

LIBRARY_FIELD_FACT ExtractLibraryFieldFact( const SCH_FIELD& aField );

struct LIBRARY_SYMBOL_FACT
{
    KIID                                            id = niluuid;
    VECTOR2I                                        position;
    LIB_ID                                          library;
    bool                                            hasEmbeddedSymbol = false;
    int                                             unitCount = 1;
    int                                             bodyStyleCount = 1;
    std::vector<PIN_COMPARISON_DATA>                pins;
    std::vector<LIBRARY_SHAPE_FACT>                 shapes;
    LIB_SYMBOL_ATTRIBUTES                           attributes;
    std::vector<LIBRARY_FIELD_FACT>                 fields;
    std::optional<std::vector<PIN_COMPARISON_DATA>> inheritedPins;

    // Content comparison for ERC; object/parent identity is excluded by SymbolCompareFlags.
    bool Matches( const LIBRARY_SYMBOL_FACT& aOther, int aCompareFlags ) const;
    bool HasDuplicatePins() const;
    bool operator==( const LIBRARY_SYMBOL_FACT& ) const = default;
};

LIBRARY_SYMBOL_FACT ExtractLibrarySymbolFact( const LIB_SYMBOL& aSymbol );
using LIBRARY_SYMBOL_FACTS = std::vector<LIBRARY_SYMBOL_FACT>;

struct SCREEN_FACTS
{
    std::shared_ptr<const LIBRARY_SYMBOL_FACTS> librarySymbols;
    std::vector<FOOTPRINT_FACT>                 footprints;
    std::vector<PIN_MAP_FACT>                   pinMaps;
    std::vector<MULTI_UNIT_FACT>                multiUnits;
    std::vector<ITEM_FACT>                      items;
    std::vector<RULE_AREA_FACT>                 ruleAreas;
    std::vector<KIID>                           netclassOwners;
    std::vector<FIELD_NAME_FACT>                invalidFieldNames;
    const LIBRARY_SYMBOL_FACTS&                 LibrarySymbols() const;
    bool                                        operator==( const SCREEN_FACTS& aOther ) const;
};

struct ITEM_TEXT_FACT
{
    KIID                  id = niluuid;
    wxString              name;
    wxString              ncName;
    std::vector<wxString> netclasses;
    bool                  canDrive = false;
    wxString              reference;
    bool                  operator==( const ITEM_TEXT_FACT& ) const = default;
};

struct NETCLASS_REFERENCE
{
    KIID_PATH sheet;
    KIID      item;
    VECTOR2I  position;
    wxString  name;
    bool      operator==( const NETCLASS_REFERENCE& ) const = default;
};

struct CHILD_SHEET_FACT
{
    KIID     id = niluuid;
    VECTOR2I position;
    wxString name;
    bool     operator==( const CHILD_SHEET_FACT& ) const = default;
};

struct PIN_MAP_CANDIDATE_FACT
{
    KIID     id = niluuid;
    VECTOR2I position;
    wxString number;
    wxString footprint;
    bool     ignoresDangling = false;
    bool     operator==( const PIN_MAP_CANDIDATE_FACT& ) const = default;
};

struct TEXT_ASSERTION
{
    bool     warning = false;
    wxString message;
    bool     operator==( const TEXT_ASSERTION& ) const = default;
};

struct TEXT_CHECK_FACT
{
    KIID                        assertionItem = niluuid;
    KIID                        item = niluuid;
    VECTOR2I                    assertionPosition;
    VECTOR2I                    position;
    std::vector<TEXT_ASSERTION> assertions;
    wxString                    shownText;
    bool                        operator==( const TEXT_CHECK_FACT& ) const = default;
};

struct VARIANT_SYMBOL_FACT
{
    KIID                       id = niluuid;
    std::map<wxString, LIB_ID> overrides;
    bool                       operator==( const VARIANT_SYMBOL_FACT& ) const = default;
};

std::optional<VARIANT_SYMBOL_FACT> ExtractVariantSymbolFact( const SCH_SYMBOL& aSymbol,
                                                           const SCH_SHEET_PATH& aPath );

struct SIMULATION_MODEL_FACT
{
    KIID            id = niluuid;
    VECTOR2I        position;
    SIM_MODEL_INPUT input;
    bool            operator==( const SIMULATION_MODEL_FACT& ) const = default;
};

// Capture after current connectivity, netclasses and rule-area membership have been applied.
std::vector<SIMULATION_MODEL_FACT> ExtractSimulationModelFacts( const SCH_SHEET_PATH& aPath,
                                                               const wxString& aVariantName );

struct INSTANCE_FACTS
{
    std::vector<std::pair<KIID, wxString>> footprints;
    std::vector<VARIANT_SYMBOL_FACT>       variantSymbols;
    std::vector<PIN_MAP_CANDIDATE_FACT>    pinMapCandidates;
    int                                    pageOrder = 0;
    std::vector<MULTI_UNIT_REFERENCE>      multiUnits;
    std::vector<std::pair<KIID, int>>      units;
    std::vector<ITEM_TEXT_FACT>            items;
    std::vector<INSTANCE_RULE_AREA_FACT>   ruleAreas;
    std::vector<NETCLASS_REFERENCE>        netclassReferences;
    std::vector<CHILD_SHEET_FACT>          childSheets;
    bool                                   operator==( const INSTANCE_FACTS& ) const = default;
};

std::vector<TEXT_ASSERTION> ExtractTextAssertions( const wxString& aText );

std::vector<TEXT_CHECK_FACT> ExtractTextChecks( const SCH_SCREEN& aScreen, const SCH_SHEET_PATH& aPath );

// The caller must install a private drawing layout so rendering cannot replace canvas-owned objects.
std::vector<TEXT_CHECK_FACT> ExtractDrawingSheetTextChecks( const SCH_SCREEN& aScreen, const SCH_SHEET_PATH& aPath );

std::optional<PIN_MAP_FACT> ExtractPinMapFacts( const SCH_SYMBOL& aSymbol );

std::vector<UNIT_FACT> ExtractUnitFacts( const LIB_SYMBOL& aSymbol );

/**
 * Main-thread snapshots; no derived connectivity or model pointers survive extraction.
 * aUnchangedNonLines requires a verified unchanged non-line source revision.
 */
SCREEN_FACTS   ExtractScreenFacts( const SCH_SCREEN& aScreen,
                                  std::shared_ptr<const LIBRARY_SYMBOL_FACTS> aLibrarySymbols = {},
                                  const SCREEN_FACTS* aUnchangedNonLines = nullptr );
INSTANCE_FACTS ExtractInstanceFacts( const SCREEN_FACTS& aFacts, const SCH_SCREEN& aScreen,
                                     const SCH_SHEET_PATH& aPath );
} // namespace SCH_CONNECTIVITY
