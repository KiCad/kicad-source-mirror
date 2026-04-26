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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
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
class SCH_TEXT;
class SCHEMATIC;

namespace PADS_SCH
{

/**
 * Builds KiCad LIB_SYMBOL objects from parsed PADS symbol definitions.
 */
class PADS_SCH_SYMBOL_BUILDER
{
public:
    PADS_SCH_SYMBOL_BUILDER( const PARAMETERS& aParams );
    ~PADS_SCH_SYMBOL_BUILDER();

    /**
     * Build a KiCad LIB_SYMBOL from a PADS symbol definition. Caller takes ownership.
     */
    LIB_SYMBOL* BuildSymbol( const SYMBOL_DEF& aSymbolDef );

    /**
     * Return the cached symbol for the given definition, building and caching it if needed.
     * The returned pointer is owned by this builder.
     */
    LIB_SYMBOL* GetOrCreateSymbol( const SYMBOL_DEF& aSymbolDef );

    bool HasSymbol( const std::string& aName ) const;

    LIB_SYMBOL* GetSymbol( const std::string& aName ) const;

    static bool IsPowerSymbol( const std::string& aName );

    /**
     * Map a PADS power symbol name to a KiCad power library LIB_ID, or nullopt if unmapped.
     */
    static std::optional<LIB_ID> GetKiCadPowerSymbolId( const std::string& aPadsName );

    /**
     * Build a power symbol using hard-coded KiCad-standard graphics, or nullptr if the name
     * is unrecognized. Caller takes ownership.
     */
    LIB_SYMBOL* BuildKiCadPowerSymbol( const std::string& aKiCadName );

    /**
     * Map a PADS special_variant decal name and pin type to a power symbol style name for
     * BuildKiCadPowerSymbol(), or empty if unrecognized.
     */
    static std::string GetPowerStyleFromVariant( const std::string& aDecalName,
                                                 const std::string& aPinType );

    /**
     * Build a composite multi-unit symbol from a multi-gate PARTTYPE. Pin numbers, names and
     * types come from the GATE_DEF pin list because the decal carries only placeholder pin
     * data. Caller takes ownership.
     */
    LIB_SYMBOL* BuildMultiUnitSymbol( const PARTTYPE_DEF& aPartType,
                                       const std::vector<SYMBOL_DEF>& aSymbolDefs );

    /**
     * Return the cached multi-unit symbol for the PARTTYPE, building it if needed, so all
     * instances of the same multi-gate part share one symbol.
     */
    LIB_SYMBOL* GetOrCreateMultiUnitSymbol( const PARTTYPE_DEF& aPartType,
                                             const std::vector<SYMBOL_DEF>& aSymbolDefs );

    /**
     * Return a single-gate symbol with PARTTYPE pin overrides applied at build time. Cached
     * by PARTTYPE so distinct PARTTYPEs sharing one decal stay separate.
     */
    LIB_SYMBOL* GetOrCreatePartTypeSymbol( const PARTTYPE_DEF& aPartType,
                                            const SYMBOL_DEF& aSymbolDef );

    /**
     * Return a connector symbol variant carrying a specific pin number, since PADS connectors
     * reuse one decal across every pin placement.
     */
    LIB_SYMBOL* GetOrCreateConnectorPinSymbol( const PARTTYPE_DEF& aPartType,
                                                const SYMBOL_DEF&   aSymbolDef,
                                                const std::string&  aPinNumber );

    /**
     * Build a multi-unit connector symbol with one unit per pin, letting all of a connector's
     * individually placed pins share one reference designator. Caller takes ownership.
     */
    LIB_SYMBOL* BuildMultiUnitConnectorSymbol( const PARTTYPE_DEF&            aPartType,
                                                const SYMBOL_DEF&              aSymbolDef,
                                                const std::vector<std::string>& aPinNumbers );

    /**
     * Return the cached multi-unit connector symbol, building it if needed.
     */
    LIB_SYMBOL* GetOrCreateMultiUnitConnectorSymbol(
            const PARTTYPE_DEF&             aPartType,
            const SYMBOL_DEF&               aSymbolDef,
            const std::vector<std::string>& aPinNumbers,
            const std::string&              aCacheKey );

    /**
     * Add hidden PT_POWER_IN pins from PARTTYPE SIGPIN entries, skipping duplicate numbers.
     */
    void AddHiddenPowerPins( LIB_SYMBOL* aSymbol,
                             const std::vector<PARTTYPE_DEF::SIGPIN>& aSigpins );

private:
    int toKiCadUnits( double aPadsValue ) const;

    /**
     * Create a SCH_SHAPE from a PADS graphic element. Returns nullptr for mixed line/arc
     * paths, which must go through createShapes() instead.
     */
    SCH_SHAPE* createShape( const SYMBOL_GRAPHIC& aGraphic );

    std::vector<SCH_SHAPE*> createShapes( const SYMBOL_GRAPHIC& aGraphic );

    SCH_PIN* createPin( const SYMBOL_PIN& aPin, LIB_SYMBOL* aParent );

    /// Build the configured SCH_TEXT for one symbol text field, or nullptr when its content is
    /// empty. A non-zero @p aUnit binds the text to that body unit.
    SCH_TEXT* createSymbolText( const SYMBOL_TEXT& aText, int aUnit = 0 );

    int mapPinType( PIN_TYPE aPadsType );

    const PARAMETERS& m_params;
    std::map<std::string, std::unique_ptr<LIB_SYMBOL>> m_symbolCache;
};

} // namespace PADS_SCH

#endif // PADS_SCH_SYMBOL_BUILDER_H_
