/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2025 KiCad Developers, see AUTHORS.txt for contributors.
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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef PADS_SCH_SYMBOL_BUILDER_H_
#define PADS_SCH_SYMBOL_BUILDER_H_

#include <sch_io/pads/pads_sch_parser.h>
#include <lib_id.h>
#include <map>
#include <memory>
#include <optional>
#include <string>

class LIB_SYMBOL;
class SCH_SHAPE;
class SCH_PIN;
class SCHEMATIC;

namespace PADS_SCH
{

/**
 * Builder class to convert PADS symbol definitions to KiCad LIB_SYMBOL objects.
 *
 * This class handles the conversion of parsed PADS symbol definitions (graphics and pins)
 * to KiCad's embedded symbol format for schematic import.
 */
class PADS_SCH_SYMBOL_BUILDER
{
public:
    PADS_SCH_SYMBOL_BUILDER( const PARAMETERS& aParams );
    ~PADS_SCH_SYMBOL_BUILDER();

    /**
     * Build a KiCad LIB_SYMBOL from a PADS symbol definition.
     *
     * @param aSymbolDef The parsed PADS symbol definition.
     * @return A new LIB_SYMBOL object. Caller takes ownership.
     */
    LIB_SYMBOL* BuildSymbol( const SYMBOL_DEF& aSymbolDef );

    /**
     * Get or create a symbol for the given definition.
     *
     * If the symbol has already been created, returns a pointer to the existing symbol.
     * Otherwise creates a new symbol and caches it.
     *
     * @param aSymbolDef The parsed PADS symbol definition.
     * @return Pointer to the symbol (owned by this builder).
     */
    LIB_SYMBOL* GetOrCreateSymbol( const SYMBOL_DEF& aSymbolDef );

    /**
     * Check if a symbol with the given name already exists.
     */
    bool HasSymbol( const std::string& aName ) const;

    /**
     * Get a cached symbol by name.
     *
     * @param aName Symbol name.
     * @return Pointer to the symbol, or nullptr if not found.
     */
    LIB_SYMBOL* GetSymbol( const std::string& aName ) const;

    /**
     * Check if a symbol name indicates a power symbol.
     *
     * @param aName Symbol name to check.
     * @return True if the name matches common power symbol patterns.
     */
    static bool IsPowerSymbol( const std::string& aName );

    /**
     * Get KiCad power library symbol ID for a PADS power symbol.
     *
     * @param aPadsName PADS symbol name.
     * @return LIB_ID for the KiCad power library symbol, or nullopt if no mapping.
     */
    static std::optional<LIB_ID> GetKiCadPowerSymbolId( const std::string& aPadsName );

    /**
     * Build a power symbol using hard-coded KiCad-standard graphics.
     *
     * @param aKiCadName KiCad power symbol name (e.g. "GND", "VCC", "VEE").
     * @return A new LIB_SYMBOL with power graphics, or nullptr if the name is unrecognized.
     *         Caller takes ownership.
     */
    LIB_SYMBOL* BuildKiCadPowerSymbol( const std::string& aKiCadName );

    /**
     * Map a PADS special_variant to a power symbol style name.
     *
     * Uses the variant's decal_name pattern (RAIL, ARROW, BUBBLE) and pin_type
     * to determine the appropriate KiCad power symbol style.
     *
     * @param aDecalName Variant decal name (e.g. "+RAIL", "AGND", "+BUBBLE").
     * @param aPinType   Variant pin type ("G" for ground, "P" for power).
     * @return Internal style name for BuildKiCadPowerSymbol(), or empty if unrecognized.
     */
    static std::string GetPowerStyleFromVariant( const std::string& aDecalName,
                                                 const std::string& aPinType );

    /**
     * Build a composite multi-unit LIB_SYMBOL from a multi-gate PARTTYPE.
     *
     * Each gate becomes a separate unit with its own graphics, pins, and text.
     * Pin numbers/names/types are taken from the GATE_DEF pin list rather than
     * from the CAEDECAL SYMBOL_DEF, which only has placeholder pin data.
     *
     * @param aPartType  The multi-gate PARTTYPE definition.
     * @param aSymbolDefs All parsed CAEDECAL definitions for decal lookup.
     * @return A new multi-unit LIB_SYMBOL. Caller takes ownership.
     */
    LIB_SYMBOL* BuildMultiUnitSymbol( const PARTTYPE_DEF& aPartType,
                                       const std::vector<SYMBOL_DEF>& aSymbolDefs );

    /**
     * Get or create a multi-unit symbol for the given PARTTYPE.
     *
     * Cached by PARTTYPE name so that all instances of the same multi-gate part
     * share one composite LIB_SYMBOL.
     */
    LIB_SYMBOL* GetOrCreateMultiUnitSymbol( const PARTTYPE_DEF& aPartType,
                                             const std::vector<SYMBOL_DEF>& aSymbolDefs );

    /**
     * Get or create a single-gate symbol with PARTTYPE-specific pin remapping.
     *
     * Since mergePartTypeData() no longer mutates shared SYMBOL_DEF pins,
     * this method applies pin number/name/type overrides from GATE_DEF::pins
     * at build time. Cached by PARTTYPE name to keep separate symbols when
     * multiple PARTTYPEs share the same CAEDECAL.
     */
    LIB_SYMBOL* GetOrCreatePartTypeSymbol( const PARTTYPE_DEF& aPartType,
                                            const SYMBOL_DEF& aSymbolDef );

    /**
     * Get or create a single-pin connector symbol with a specific pin number.
     *
     * PADS connectors use one CAEDECAL symbol for all pin placements, but each
     * placement represents a different pin number. This creates a variant with
     * the correct pin number for each connector pin placement.
     *
     * @param aPartType   The connector PARTTYPE definition.
     * @param aSymbolDef  The CAEDECAL symbol definition with graphics.
     * @param aPinNumber  The pin number string for this connector pin (e.g. "15").
     * @return Pointer to the symbol (owned by this builder).
     */
    LIB_SYMBOL* GetOrCreateConnectorPinSymbol( const PARTTYPE_DEF& aPartType,
                                                const SYMBOL_DEF&   aSymbolDef,
                                                const std::string&  aPinNumber );

    /**
     * Add hidden power pins from PARTTYPE SIGPIN entries to an existing symbol.
     *
     * Each SIGPIN becomes an invisible PT_POWER_IN pin at position (0,0).
     * Duplicate pin numbers are skipped.
     */
    void AddHiddenPowerPins( LIB_SYMBOL* aSymbol,
                             const std::vector<PARTTYPE_DEF::SIGPIN>& aSigpins );

private:
    /**
     * Convert PADS coordinate to KiCad internal units.
     */
    int toKiCadUnits( double aPadsValue ) const;

    /**
     * Create a SCH_SHAPE from a PADS graphic element.
     * Returns nullptr for mixed line/arc paths (use createShapes instead).
     */
    SCH_SHAPE* createShape( const SYMBOL_GRAPHIC& aGraphic );

    /**
     * Create one or more SCH_SHAPEs from a PADS graphic element.
     * Handles mixed line/arc paths by emitting individual line and arc shapes.
     */
    std::vector<SCH_SHAPE*> createShapes( const SYMBOL_GRAPHIC& aGraphic );

    /**
     * Create a SCH_PIN from a PADS pin definition.
     */
    SCH_PIN* createPin( const SYMBOL_PIN& aPin, LIB_SYMBOL* aParent );

    /**
     * Map PADS pin type to KiCad electrical type.
     */
    int mapPinType( PIN_TYPE aPadsType );

    const PARAMETERS& m_params;
    std::map<std::string, std::unique_ptr<LIB_SYMBOL>> m_symbolCache;
};

} // namespace PADS_SCH

#endif // PADS_SCH_SYMBOL_BUILDER_H_
