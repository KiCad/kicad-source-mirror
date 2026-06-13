/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/gpl-3.0.html
 */

#ifndef KICAD_DIFF_TYPES_H
#define KICAD_DIFF_TYPES_H

#include <kicommon.h>
#include <kiid.h>
#include <math/box2.h>
#include <math/vector2d.h>
#include <gal/color4d.h>
#include <layer_ids.h>

#include <nlohmann/json_fwd.hpp>
#include <wx/string.h>

#ifdef __WINDOWS__
extern template class KICOMMON_API nlohmann::basic_json<>;
#endif

#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <variant>
#include <vector>


enum class EDA_UNITS;
struct EDA_IU_SCALE;


namespace KICAD_DIFF
{

/**
 * How a numeric DIFF_VALUE should be interpreted when rendered for humans.
 *
 * Derived from the source property's PROPERTY_DISPLAY (PT_SIZE / PT_COORD /
 * PT_DEGREE) so distance, coordinate and angle deltas render in mm / degrees
 * instead of raw internal units. Plain numbers carry NONE and are printed
 * verbatim. The hint is display-only and deliberately not serialized: it is a
 * presentation aid recoverable from the property metadata, not part of the
 * canonical value.
 */
enum class DISPLAY_HINT
{
    NONE,
    DISTANCE,   ///< Length in internal units (PT_SIZE)
    COORD,      ///< Coordinate in internal units (PT_COORD)
    ANGLE       ///< Angle in degrees (PT_DEGREE)
};

/**
 * Coarse classification of a single item-level change between two documents.
 *
 * The engine never carries display severity on the change record; consumers compute
 * their own ranking from (typeName, kind, properties).
 */
enum class CHANGE_KIND
{
    ADDED,
    REMOVED,
    MODIFIED,
    COLLISION,       // independent add of same KIID on both sides
    DUPLICATE_UUID   // same KIID appears twice in one document
};


/// Property-name keys for the synthetic document-level ITEM_CHANGE (empty
/// KIID_PATH). Shared between PCB and SCH differs / appliers so a typo on one
/// side doesn't silently desync from the other.
inline const wxString DOC_PROP_PAGE_FORMAT      = wxS( "Page Format" );
inline const wxString DOC_PROP_PAGE_ORIENTATION = wxS( "Page Orientation" );
inline const wxString DOC_PROP_BOARD_THICKNESS  = wxS( "Board Thickness" );
inline const wxString DOC_PROP_LAYER_STACKUP    = wxS( "Layer Stackup" );
inline const wxString DOC_PROP_DRC_SEVERITIES   = wxS( "DRC Severity Overrides" );
inline const wxString DOC_PROP_ERC_SEVERITIES   = wxS( "ERC Severity Overrides" );
inline const wxString DOC_PROP_DRAWING_SHEET    = wxS( "Drawing Sheet File" );
inline const wxString DOC_PROP_NET_CLASSES      = wxS( "Net Classes" );
inline const wxString DOC_PROP_CUSTOM_RULES     = wxS( "Custom DRC Rules" );
inline const wxString DOC_PROP_FP_LIB_TABLE     = wxS( "Footprint Library Table" );
inline const wxString DOC_PROP_SYM_LIB_TABLE    = wxS( "Symbol Library Table" );


/// Sentinel KIID appended to a sheet's KIID_PATH to mark a per-sheet
/// SCH_SCREEN resolution (page format / orientation override on that sheet's
/// screen). Distinguishes a "this is a SCH_SCREEN doc-property delta on the
/// sheet at this path" resolution from "this is the SCH_SHEET symbol item at
/// this path" — those two would otherwise share the same KIID_PATH and the
/// applier would route the SCH_SCREEN resolution into the (intentionally
/// skipped) sheet-symbol applier branch. UUID is hard-coded as a valid v4
/// shape (so KIID's string_generator parses it deterministically), distinct
/// from KIID::niluuid, and unlikely to collide with any real item KIID.
inline const KIID& SchScreenSentinelKiid()
{
    static const KIID sentinel{ std::string( "5c50ee00-0000-4000-8000-000000000000" ) };
    return sentinel;
}


/// Build a deterministic synthetic KIID_PATH from a library item name (symbol
/// name or footprint name). Library files aren't UUID-keyed; the engine API
/// is. The naive `KIID_PATH(name)` falls into `KIID(string)`'s random-UUID
/// fallback for any non-hex name, so two callsites produce different IDs for
/// the same name. `KIID::FromDeterministicString` hashes the name into a
/// UUID-shaped string so both callsites produce identical bytes.
///
/// Used by SYM_LIB_DIFFER / FP_LIB_DIFFER (differ side) and
/// SYM_LIB_MERGE_APPLIER / FP_LIB_MERGE_APPLIER (applier side); they MUST
/// share this mapping or item-resolution lookups silently miss.
inline KIID_PATH LibraryItemKiidPath( const wxString& aName )
{
    KIID_PATH path;
    path.emplace_back( KIID::FromDeterministicString( aName ) );

    return path;
}


/// Format a severity-override map (DRC or ERC, keyed by error code, value is a
/// SEVERITY enum) as a short summary with a content hash, so two maps with the
/// same size but different contents render as distinct before/after strings.
template<typename SeverityMap>
inline std::string SummarizeSeverities( const SeverityMap& aMap )
{
    std::size_t h = 0;

    for( const auto& [code, sev] : aMap )
        h ^= std::hash<int>{}( code ) + std::hash<int>{}( static_cast<int>( sev ) );

    return wxString::Format( wxS( "%zu override(s) (hash %zx)" ),
                             aMap.size(), h ).ToStdString();
}


/**
 * A typed sum value used to carry the before/after of any single property.
 *
 * Designed to round-trip through JSON deterministically so the engine output is
 * bit-stable across runs and trivially comparable in tests. Avoids wxAny because
 * wxAny does not serialise.
 */
class KICOMMON_API DIFF_VALUE
{
public:
    enum class T
    {
        NONE,
        BOOL,
        INT,
        INT64,
        DOUBLE,
        STRING,
        KIID,
        VECTOR2I,
        BOX2I,
        COLOR,
        LAYER,
        ENUM,
        POLYGON_SET
    };

    /// Enum payload: (numeric_value, label) so JSON output is human-readable while
    /// the integer is the canonical comparison key.
    using EnumValue = std::pair<int, std::string>;

    using PolygonSet = std::vector<std::vector<std::vector<VECTOR2I>>>;

    using Storage = std::variant<std::monostate, bool, int, int64_t, double, std::string, KIID, VECTOR2I, BOX2I,
                                 KIGFX::COLOR4D, PCB_LAYER_ID, EnumValue, PolygonSet>;

    DIFF_VALUE() : m_type( T::NONE ), m_value( std::monostate{} ) {}

    static DIFF_VALUE FromBool( bool aValue );
    static DIFF_VALUE FromInt( int aValue );
    static DIFF_VALUE FromInt64( int64_t aValue );
    static DIFF_VALUE FromDouble( double aValue );
    static DIFF_VALUE FromString( const wxString& aValue );
    static DIFF_VALUE FromString( const std::string& aValue );
    static DIFF_VALUE FromKiid( const KIID& aValue );
    static DIFF_VALUE FromVector2I( const VECTOR2I& aValue );
    static DIFF_VALUE FromBox2I( const BOX2I& aValue );
    static DIFF_VALUE FromColor( const KIGFX::COLOR4D& aValue );
    static DIFF_VALUE FromLayer( PCB_LAYER_ID aLayer );
    static DIFF_VALUE FromEnum( int aValue, const std::string& aLabel );
    static DIFF_VALUE FromPolygonSet( PolygonSet aValue );

    T              GetType() const { return m_type; }
    const Storage& GetStorage() const { return m_value; }

    bool      AsBool() const;
    int       AsInt() const;
    int64_t   AsInt64() const;
    double    AsDouble() const;
    wxString  AsString() const;
    KIID      AsKiid() const;
    VECTOR2I  AsVector2I() const;
    BOX2I     AsBox2I() const;
    KIGFX::COLOR4D AsColor() const;
    PCB_LAYER_ID   AsLayer() const;
    EnumValue AsEnum() const;
    const PolygonSet& AsPolygonSet() const;

    /// Tag this value with a display hint and return a copy, so call sites can
    /// chain `DIFF_VALUE::FromInt( w ).WithDisplayHint( DISPLAY_HINT::DISTANCE )`.
    DIFF_VALUE WithDisplayHint( DISPLAY_HINT aHint ) const
    {
        DIFF_VALUE v = *this;
        v.m_displayHint = aHint;
        return v;
    }

    DISPLAY_HINT GetDisplayHint() const { return m_displayHint; }

    /// Human-readable representation with no unit context. Distance/coord/angle
    /// hints fall back to raw internal units here; use the unit-aware overload
    /// for user-facing output.
    wxString ToDisplayString() const;

    /// Human-readable representation in @p aUnits using @p aScale. Distance and
    /// coordinate values carrying a DISTANCE/COORD hint are converted from
    /// internal units; ANGLE-hinted values are formatted in degrees. All other
    /// values render identically to the no-arg overload.
    wxString ToDisplayString( EDA_UNITS aUnits, const EDA_IU_SCALE& aScale ) const;

    bool operator==( const DIFF_VALUE& aOther ) const;
    bool operator!=( const DIFF_VALUE& aOther ) const { return !( *this == aOther ); }

    nlohmann::json    ToJson() const;
    static DIFF_VALUE FromJson( const nlohmann::json& aJson );

private:
    T            m_type;
    Storage      m_value;

    /// Presentation aid only; excluded from operator== and JSON (see DISPLAY_HINT).
    /// Not serialized, so a value reconstructed via FromJson carries
    /// DISPLAY_HINT::NONE and only freshly-converted values render with units.
    DISPLAY_HINT m_displayHint = DISPLAY_HINT::NONE;
};


/**
 * Single (name, before, after) triple for one mutated property on an item.
 *
 * Property identity is the canonical PROPERTY_MANAGER name; consumers must be able
 * to look the property metadata up again from that string.
 */
struct KICOMMON_API PROPERTY_DELTA
{
    wxString   name;
    DIFF_VALUE before;
    DIFF_VALUE after;

    bool operator==( const PROPERTY_DELTA& aOther ) const;
    nlohmann::json        ToJson() const;
    static PROPERTY_DELTA FromJson( const nlohmann::json& aJson );
};


/**
 * One change record on a single item.
 *
 * The id is a KIID_PATH so that hierarchical context (sheet path for schematics,
 * footprint path for child pads) is preserved across the diff/merge pipeline.
 * The refdes field is presentation only and is not used to key items in the engine.
 */
struct KICOMMON_API ITEM_CHANGE
{
    KIID_PATH                   id;
    wxString                    typeName;
    CHANGE_KIND                 kind;
    std::vector<PROPERTY_DELTA> properties;
    BOX2I                       bbox;
    std::optional<wxString>     refdes;
    std::vector<ITEM_CHANGE>    children;

    bool operator==( const ITEM_CHANGE& aOther ) const;
    nlohmann::json     ToJson() const;
    static ITEM_CHANGE FromJson( const nlohmann::json& aJson );
};


/**
 * The full set of changes between two parsed documents of one type.
 */
struct KICOMMON_API DOCUMENT_DIFF
{
    wxString                 path;       // identifier for the document being diffed
    wxString                 docType;    // "kicad_pcb", "kicad_sch", "kicad_sym", "pretty", ...
    std::vector<ITEM_CHANGE> changes;

    bool   Empty() const { return changes.empty(); }
    size_t Size()  const { return changes.size(); }

    nlohmann::json       ToJson() const;
    static DOCUMENT_DIFF FromJson( const nlohmann::json& aJson );
};


/**
 * Aggregated project-level diff covering many documents.
 */
struct KICOMMON_API PROJECT_DIFF
{
    std::vector<DOCUMENT_DIFF> documents;

    bool Empty() const;

    nlohmann::json      ToJson() const;
    static PROJECT_DIFF FromJson( const nlohmann::json& aJson );
};


KICOMMON_API const char* ChangeKindToString( CHANGE_KIND aKind );
KICOMMON_API CHANGE_KIND ChangeKindFromString( const std::string& aKind );


/**
 * Render a DOCUMENT_DIFF as the human-readable text report shared by the diff
 * jobs and CLI: a `diff <labelA> <labelB>` header, the change count, then one
 * indented line per change (recursing into footprint/sheet children) with
 * property deltas printed as `name: before -> after`.
 *
 * Property values are formatted in @p aUnits using @p aScale, so distance,
 * coordinate and angle deltas render in user units (e.g. "0.25 mm") rather than
 * raw internal units. Use IuScaleForDocKind() to pick the scale matching the
 * source document.
 */
KICOMMON_API std::string FormatDiffAsText( const DOCUMENT_DIFF& aDiff, const wxString& aLabelA,
                                           const wxString& aLabelB, EDA_UNITS aUnits,
                                           const EDA_IU_SCALE& aScale );


/**
 * Write diff/merge text output to @p aOutputPath, or to stdout when the path is
 * empty. Returns false only when a non-empty path cannot be opened for writing.
 */
KICOMMON_API bool WriteDiffOutput( const std::string& aContent, const wxString& aOutputPath );


/**
 * Flatten a `DOCUMENT_DIFF`'s ITEM_CHANGE tree into a `KIID_PATH ->
 * ITEM_CHANGE*` map, recursing into child changes. Without the recursive
 * walk, child conflicts (e.g. a pad edit inside a footprint with no
 * top-level conflict) are invisible to downstream conflict detection.
 *
 * Dedup: last-write-wins on duplicate keys (uses `operator[]` assignment).
 * KIID_PATHs are unique by construction so duplicates shouldn't occur in
 * practice, but a corrupt or hand-crafted diff with collisions surfaces
 * the deeper / later entry.
 *
 * Pointers reference items owned by `aDiff` — the map must not outlive
 * the diff.
 */
KICOMMON_API std::map<KIID_PATH, const ITEM_CHANGE*>
IndexChangesByKiid( const DOCUMENT_DIFF& aDiff );


/**
 * Index property deltas inside one ITEM_CHANGE by property name. Last
 * entry wins on duplicate names — callers must not rely on stability of
 * which entry "wins" because the differ never emits duplicates.
 */
KICOMMON_API std::map<wxString, const PROPERTY_DELTA*>
IndexPropertiesByName( const ITEM_CHANGE& aChange );


/**
 * Whether a change to an item of the given type invalidates any overlapping
 * filled zones. Drives MERGE_PLAN::requiresZoneRefill so the merge driver
 * knows to re-run the filler before saving.
 *
 * Pure function of the type name — does not look at the change contents,
 * because even a property edit on (say) a track changes its keepout
 * influence on the zone fill. Returns false for classes whose
 * zone-affecting status depends on layer (PCB_TEXT, PCB_SHAPE on copper
 * are knockouts but on silkscreen are not); the type-name-only signature
 * can't distinguish, and silkscreen edits are common enough that the
 * conservative choice is to skip the (slow) refill and let the user
 * trigger one manually if they edited copper graphics.
 */
inline bool ChangeInvalidatesZone( const ITEM_CHANGE& aChange )
{
    return aChange.typeName == wxS( "ZONE" )
        || aChange.typeName == wxS( "PCB_TRACK" )
        || aChange.typeName == wxS( "PCB_ARC" )
        || aChange.typeName == wxS( "PCB_VIA" )
        || aChange.typeName == wxS( "FOOTPRINT" )
        || aChange.typeName == wxS( "PAD" );
}


/**
 * Whether a change to an item of the given type requires the connectivity
 * graph to be rebuilt. Drives MERGE_PLAN::requiresConnectivityRebuild so
 * the merge driver knows to re-derive netlists / from-tos before saving.
 *
 * Like ChangeInvalidatesZone, this is a pure function of the type name —
 * any change to a connectable object can shift connectivity.
 *
 * @note The set below is the canonical list of net-carrying / connection-
 *       marker class names. Keep `common/diff_merge/README.md` in sync
 *       (the "Pure index + predicate helpers" section claims this is the
 *       complete list); the QA test `ChangeRequiresConnectivityRebuild_NetCarriers`
 *       enumerates every entry and trips if one is dropped.
 */
inline bool ChangeRequiresConnectivityRebuild( const ITEM_CHANGE& aChange )
{
    // Class names match SCH_ITEM::GetClass() / BOARD_ITEM::GetClass() exactly,
    // because the differ stores `GetClass()` in `ITEM_CHANGE::typeName`. Net
    // carriers (anything that can change net membership at its endpoints) and
    // connection markers (junction, no-connect) all belong here — missing one
    // means the merge engine skips the connectivity recompute and the saved
    // board / schematic ships with a stale netlist.
    return aChange.typeName == wxS( "PCB_TRACK" )
        || aChange.typeName == wxS( "PCB_ARC" )
        || aChange.typeName == wxS( "PCB_VIA" )
        || aChange.typeName == wxS( "PCB_SHAPE" )    // copper-layer shapes carry nets
        || aChange.typeName == wxS( "PAD" )
        || aChange.typeName == wxS( "SCH_LINE" )
        || aChange.typeName == wxS( "SCH_PIN" )
        || aChange.typeName == wxS( "SCH_SYMBOL" )   // pin positions move with the symbol
        || aChange.typeName == wxS( "SCH_LABEL" )
        || aChange.typeName == wxS( "SCH_GLOBALLABEL" )
        || aChange.typeName == wxS( "SCH_HIERLABEL" )
        || aChange.typeName == wxS( "SCH_DIRECTIVE_LABEL" )
        || aChange.typeName == wxS( "SCH_JUNCTION" )
        || aChange.typeName == wxS( "SCH_NO_CONNECT" )
        || aChange.typeName == wxS( "SCH_SHEET_PIN" )
        || aChange.typeName == wxS( "SCH_BUS_WIRE_ENTRY" )
        || aChange.typeName == wxS( "SCH_BUS_BUS_ENTRY" )
        || aChange.typeName == wxS( "SCH_SHEET" )         // pin / file / pos affects nets
        || aChange.typeName == wxS( "SCH_RULE_AREA" );    // feeds resolved netclass caches
}

} // namespace KICAD_DIFF

#endif // KICAD_DIFF_TYPES_H
