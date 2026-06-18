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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <kicommon.h>
#include <lib_id.h>

#include <nlohmann/json_fwd.hpp>

#include <unordered_map>
#include <vector>
#include <wx/string.h>


/**
 * One symbol-pin to footprint-pad mapping inside a PIN_MAP.
 *
 * The pad number is a string so it carries KiCad's bracketed stacked-pin
 * syntax verbatim; e.g. "[4,9]" expresses a single symbol pin connected to
 * footprint pads 4 and 9.  Plain numeric and alphanumeric pad names work
 * unchanged.
 */
struct KICOMMON_API PIN_MAP_ENTRY
{
    wxString m_PinNumber;
    wxString m_PadNumber;

    bool operator==( const PIN_MAP_ENTRY& aOther ) const
    {
        return m_PinNumber == aOther.m_PinNumber && m_PadNumber == aOther.m_PadNumber;
    }
};


/**
 * A named pin map.
 *
 * A pin map is footprint-agnostic: it carries only a name unique within the
 * owning symbol and the pin-to-pad entries.  The link between a footprint and
 * the map it uses lives in ASSOCIATED_FOOTPRINT, so more than one footprint can
 * reference the same named map without copying the entries.
 *
 * Schematic instances do not own PIN_MAP objects; they reference one by name
 * and overlay sparse edits via PIN_MAP_INSTANCE_OVERRIDE.
 */
class KICOMMON_API PIN_MAP
{
public:
    PIN_MAP() = default;
    explicit PIN_MAP( const wxString& aName );

    const wxString& GetName() const { return m_name; }
    void            SetName( const wxString& aName ) { m_name = aName; }

    /**
     * Set the pad number for a symbol pin.  Replaces any existing entry with
     * the same pin number.
     */
    void SetEntry( const wxString& aPinNumber, const wxString& aPadNumber );

    /**
     * Remove the entry for \a aPinNumber.  No-op if no such entry exists.
     */
    void ClearEntry( const wxString& aPinNumber );

    bool HasEntry( const wxString& aPinNumber ) const;

    /**
     * @return the pad number mapped to \a aPinNumber, or an empty string if
     *         no entry exists.
     */
    const wxString& GetPadNumber( const wxString& aPinNumber ) const;

    const std::vector<PIN_MAP_ENTRY>& GetEntries() const { return m_entries; }

    bool IsEmpty() const { return m_entries.empty(); }

    /**
     * @return true when this map remaps none of \a aPinNumbers, i.e. every
     *         listed pin either has no entry (resolves 1:1 by identity) or an
     *         entry whose pad equals the pin number.  Bracketed multi-pad
     *         entries (e.g. "[4,9]") are never identity even when one expanded
     *         pad matches the pin number.
     */
    bool IsIdentity( const std::vector<wxString>& aPinNumbers ) const;

    bool operator==( const PIN_MAP& aOther ) const;

private:
    std::vector<PIN_MAP_ENTRY>::iterator       findEntry( const wxString& aPinNumber );
    std::vector<PIN_MAP_ENTRY>::const_iterator findEntry( const wxString& aPinNumber ) const;

    wxString                   m_name;
    std::vector<PIN_MAP_ENTRY> m_entries;
};


/**
 * A symbol-owned ordered set of named pin maps.
 *
 * Lookup by name is the only access path; the footprint-to-map link lives in
 * the symbol's ASSOCIATED_FOOTPRINT list.  Declaration order is preserved on
 * disk and used only for UI grouping; it does not affect netlist resolution.
 */
class KICOMMON_API PIN_MAP_SET
{
public:
    /**
     * Insert \a aMap, replacing any existing entry with the same name.
     */
    void AddOrReplace( PIN_MAP aMap );

    /**
     * Remove the map with the given name.  No-op if no such map exists.
     */
    void Remove( const wxString& aName );

    const PIN_MAP* FindByName( const wxString& aName ) const;
    PIN_MAP*       FindByName( const wxString& aName );

    const std::vector<PIN_MAP>& GetAll() const { return m_maps; }
    bool                        IsEmpty() const { return m_maps.empty(); }

    bool operator==( const PIN_MAP_SET& aOther ) const { return m_maps == aOther.m_maps; }
    bool operator!=( const PIN_MAP_SET& aOther ) const { return !( *this == aOther ); }

private:
    std::vector<PIN_MAP> m_maps;   // ordered for stable file output
};


/**
 * A first-class footprint choice on a LIB_SYMBOL, tied to a named pin map.
 *
 * Unlike the existing fp_filters glob list, an associated footprint names a
 * concrete footprint LIB_ID and the named map that resolves the symbol's pins
 * onto that footprint's pads.  An empty map name means the footprint resolves
 * by identity/unmapped only.
 */
struct KICOMMON_API ASSOCIATED_FOOTPRINT
{
    LIB_ID   m_FootprintLibId;
    wxString m_MapName;

    bool operator==( const ASSOCIATED_FOOTPRINT& aOther ) const
    {
        return m_FootprintLibId == aOther.m_FootprintLibId && m_MapName == aOther.m_MapName;
    }
};


/**
 * Override mode stored on a SCH_SYMBOL_INSTANCE.
 *
 * The mode is the source of truth.  m_ActiveMapName is read only when
 * m_Mode == USE_NAMED_MAP; m_Edits are ignored when m_Mode == FORCE_IDENTITY
 * and never apply on a DELEGATE_TO_UNIT_1 instance.
 */
enum class PIN_MAP_OVERRIDE_MODE
{
    USE_LIBRARY_DEFAULT,
    USE_NAMED_MAP,
    FORCE_IDENTITY,
    DELEGATE_TO_UNIT_1
};


/**
 * Per-instance override of the active pin map and a sparse delta on top.
 *
 * Stored canonically on the unit-1 SCH_SYMBOL_INSTANCE for a given
 * (project, sheet path, reference).  Units 2..N carry m_Mode =
 * DELEGATE_TO_UNIT_1 with empty name and edits; reads on those units are
 * routed through SCH_SYMBOL::GetPinMapOverride to the unit-1 entry.
 *
 * m_Edits is sparse: only pin numbers whose pad value differs from the
 * resolved active library map are listed.  This guarantees library-side map
 * edits propagate to every placement that has not explicitly overridden the
 * same pin.
 */
struct KICOMMON_API PIN_MAP_INSTANCE_OVERRIDE
{
    PIN_MAP_OVERRIDE_MODE      m_Mode = PIN_MAP_OVERRIDE_MODE::USE_LIBRARY_DEFAULT;
    wxString                   m_ActiveMapName;
    std::vector<PIN_MAP_ENTRY> m_Edits;

    /**
     * @return true when the override is in its default state: library default
     *         mode and no edits.  The serialiser may omit the entire token
     *         when this returns true.
     */
    bool IsDefault() const
    {
        return m_Mode == PIN_MAP_OVERRIDE_MODE::USE_LIBRARY_DEFAULT && m_Edits.empty();
    }

    /**
     * @return true when this instance delegates to the unit-1 entry.  Legal
     *         only on units 2..N of a multi-unit symbol.
     */
    bool IsDelegate() const { return m_Mode == PIN_MAP_OVERRIDE_MODE::DELEGATE_TO_UNIT_1; }

    bool operator==( const PIN_MAP_INSTANCE_OVERRIDE& aOther ) const
    {
        return m_Mode == aOther.m_Mode && m_ActiveMapName == aOther.m_ActiveMapName
               && m_Edits == aOther.m_Edits;
    }

    bool operator!=( const PIN_MAP_INSTANCE_OVERRIDE& aOther ) const { return !( *this == aOther ); }
};


/**
 * Build a single named PIN_MAP from a legacy symbol-pin to footprint-pad(s) assignment table
 * (the flat form used by the database and HTTP library backends, issue #2282).
 *
 * A pin assigned to several pads is encoded with the bracketed stacked-pad notation, e.g.
 * { "4" -> { "4", "9" } } becomes the entry "4" -> "[4,9]".  Empty assignments are skipped.
 */
KICOMMON_API PIN_MAP MakeLegacyPinMap(
        const wxString& aName,
        const std::unordered_map<wxString, std::vector<wxString>>& aAssignments );


/**
 * Parse the legacy flat pin-assignment JSON array (MR !2540 form) into a symbol-pin to
 * footprint-pad(s) table (issue #2282).
 *
 * Each element carries a "sym" (symbol pin number) and an "fp" pad number or array of pad numbers.
 * Returns an empty table when \a aArray is not an array.  Shared by the database and HTTP backends.
 */
KICOMMON_API std::unordered_map<wxString, std::vector<wxString>>
ParseLegacyPinAssignments( const nlohmann::json& aArray );

/**
 * Parse the spec-form named maps from \a aParent["pin_maps"] into a PIN_MAP_SET (issue #2282).
 *
 * Returns an empty set when the key is absent or not an array.  Shared by the database and HTTP
 * backends.
 */
KICOMMON_API PIN_MAP_SET ParsePinMapSet( const nlohmann::json& aParent );

/**
 * Parse the spec-form footprint associations from \a aParent["associated_footprints"] (issue #2282).
 *
 * Returns an empty list when the key is absent or not an array.  Shared by the database and HTTP
 * backends.
 */
KICOMMON_API std::vector<ASSOCIATED_FOOTPRINT>
ParseAssociatedFootprints( const nlohmann::json& aParent );
