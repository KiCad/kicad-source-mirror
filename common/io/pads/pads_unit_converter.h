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

#ifndef PADS_UNIT_CONVERTER_H
#define PADS_UNIT_CONVERTER_H

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

enum class PADS_UNIT_TYPE
{
    MILS,       ///< Thousandths of an inch
    METRIC,     ///< Millimeters
    INCHES
};

/**
 * Converts PADS file units (MILS, METRIC, INCHES, or internal BASIC database
 * units) to KiCad internal units (nanometers).
 */
class PADS_UNIT_CONVERTER
{
public:
    PADS_UNIT_CONVERTER();

    void SetBaseUnits( PADS_UNIT_TYPE aUnitType );

    PADS_UNIT_TYPE GetUnitType() const { return m_unitType; }

    /**
     * Enable or disable BASIC units mode. BASIC units are PADS internal database
     * units at 1/38100 mil resolution, interpreted regardless of the base unit
     * type.
     */
    void SetBasicUnitsMode( bool aEnabled );

    bool IsBasicUnitsMode() const { return m_basicUnitsMode; }

    /**
     * Set a custom scale for BASIC units, in nanometers per BASIC unit. The
     * default is MILS_TO_NM / 38100.0.
     */
    void SetBasicUnitsScale( double aScale );

    double GetBasicUnitsScale() const { return m_basicUnitsScale; }

    /**
     * Parse a PADS file header string and configure units accordingly. Returns
     * true if a unit type or BASIC mode was recognized.
     */
    bool ParseFileHeader( const std::string& aHeader );

    /**
     * Parse a PADS unit override code ("M"/"D" mils, "MM" metric, "I" inches),
     * returning an empty optional for "N" (no override) or an invalid code.
     */
    static std::optional<PADS_UNIT_TYPE> ParseUnitCode( const std::string& aUnitCode );

    /**
     * Push a unit override onto the stack, temporarily overriding the base units
     * for subsequent conversions until popped. Returns false for an invalid or
     * no-override code.
     */
    bool PushUnitOverride( const std::string& aUnitCode );

    /**
     * Pop the most recent unit override, reverting to the previous setting.
     */
    void PopUnitOverride();

    bool HasUnitOverride() const { return !m_unitOverrideStack.empty(); }

    size_t GetOverrideDepth() const { return m_unitOverrideStack.size(); }

    /**
     * Convert a coordinate value in PADS file units to nanometers.
     */
    int64_t ToNanometers( double aValue ) const;

    /**
     * Convert a size value (width, height, radius) in PADS file units to
     * nanometers.
     */
    int64_t ToNanometersSize( double aValue ) const;

    static constexpr double MILS_TO_NM = 25400.0;           // 1 mil = 25.4 um = 25400 nm
    static constexpr double MM_TO_NM = 1000000.0;           // 1 mm = 1,000,000 nm
    static constexpr double INCHES_TO_NM = 25400000.0;      // 1 inch = 25.4 mm = 25,400,000 nm
    static constexpr double BASIC_TO_NM = MILS_TO_NM / 38100.0; // 1 BASIC unit = 1/38100 mil

private:
    void updateScaleFactor();

    PADS_UNIT_TYPE              m_unitType;
    double                      m_scaleFactor;
    bool                        m_basicUnitsMode;
    double                      m_basicUnitsScale;
    std::vector<PADS_UNIT_TYPE> m_unitOverrideStack;
};

#endif // PADS_UNIT_CONVERTER_H
