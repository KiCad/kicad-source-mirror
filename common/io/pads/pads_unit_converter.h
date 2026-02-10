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

#ifndef PADS_UNIT_CONVERTER_H
#define PADS_UNIT_CONVERTER_H

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

/**
 * Unit types supported by PADS file formats.
 */
enum class PADS_UNIT_TYPE
{
    MILS,       ///< Thousandths of an inch (1 mil = 0.001 inch)
    METRIC,     ///< Millimeters
    INCHES      ///< Inches
};

/**
 * Converts PADS file format units to KiCad internal units (nanometers).
 *
 * PADS files can use different unit systems: MILS, METRIC (mm), or INCHES.
 * Additionally, files can use BASIC units which are internal database units.
 * This class handles conversion from any of these to KiCad's internal
 * nanometer representation.
 */
class PADS_UNIT_CONVERTER
{
public:
    PADS_UNIT_CONVERTER();

    /**
     * Set the base units for conversion.
     *
     * @param aUnitType The unit type used in the PADS file.
     */
    void SetBaseUnits( PADS_UNIT_TYPE aUnitType );

    /**
     * Get the current unit type.
     *
     * @return The currently configured unit type.
     */
    PADS_UNIT_TYPE GetUnitType() const { return m_unitType; }

    /**
     * Enable or disable BASIC units mode.
     *
     * BASIC units are PADS internal database units at 1/38100 mil resolution.
     * When enabled, coordinate values are interpreted as BASIC units regardless
     * of the base unit type.
     *
     * @param aEnabled True to enable BASIC units mode.
     */
    void SetBasicUnitsMode( bool aEnabled );

    /**
     * Check if BASIC units mode is enabled.
     *
     * @return True if BASIC units mode is enabled.
     */
    bool IsBasicUnitsMode() const { return m_basicUnitsMode; }

    /**
     * Set a custom scale for BASIC units.
     *
     * Some PADS files may use non-standard BASIC unit scales. The default
     * scale is MILS_TO_NM / 38100.0 (1/38100 mil per unit).
     *
     * @param aScale The scale factor in nanometers per BASIC unit.
     */
    void SetBasicUnitsScale( double aScale );

    /**
     * Get the current BASIC units scale.
     *
     * @return The scale factor in nanometers per BASIC unit.
     */
    double GetBasicUnitsScale() const { return m_basicUnitsScale; }

    /**
     * Parse a PADS file header string and configure units accordingly.
     *
     * Detects BASIC mode from header strings like "!PADS-POWERPCB-V9.0-BASIC!"
     * and unit types from strings like "!PADS-POWERPCB-V9.5-MILS!".
     *
     * @param aHeader The file header string to parse.
     * @return True if the header was successfully parsed and units configured.
     */
    bool ParseFileHeader( const std::string& aHeader );

    /**
     * Parse a PADS unit code and return the corresponding unit type.
     *
     * PADS uses short codes in parts and decals to specify local unit overrides:
     * - "M" = MILS
     * - "MM" = METRIC (millimeters)
     * - "I" = INCHES
     * - "D" = MILS (default)
     * - "N" = No override (returns empty optional)
     *
     * @param aUnitCode The unit code string to parse.
     * @return The corresponding unit type, or empty optional if code is invalid
     *         or indicates no override.
     */
    static std::optional<PADS_UNIT_TYPE> ParseUnitCode( const std::string& aUnitCode );

    /**
     * Push a unit override onto the stack.
     *
     * PADS parts and decals can specify their own units that temporarily
     * override the file's base units. This pushes an override onto the
     * stack, affecting all subsequent conversions until popped.
     *
     * @param aUnitCode The unit code string (e.g., "M", "MM", "I").
     * @return True if a valid override was pushed, false if code was invalid
     *         or indicated no override.
     */
    bool PushUnitOverride( const std::string& aUnitCode );

    /**
     * Pop the most recent unit override from the stack.
     *
     * Removes the most recent override, reverting to the previous unit
     * setting (either another override or the base units).
     */
    void PopUnitOverride();

    /**
     * Check if any unit overrides are currently active.
     *
     * @return True if one or more overrides are on the stack.
     */
    bool HasUnitOverride() const { return !m_unitOverrideStack.empty(); }

    /**
     * Get the current override depth.
     *
     * @return The number of overrides currently on the stack.
     */
    size_t GetOverrideDepth() const { return m_unitOverrideStack.size(); }

    /**
     * Convert a coordinate value to nanometers.
     *
     * This is the primary conversion function for positional values.
     *
     * @param aValue The value in PADS file units.
     * @return The value in nanometers (KiCad internal units).
     */
    int64_t ToNanometers( double aValue ) const;

    /**
     * Convert a size value to nanometers.
     *
     * Size values (widths, heights, radii) use the same conversion as
     * coordinates but may have different rounding behavior in the future.
     *
     * @param aValue The size value in PADS file units.
     * @return The size in nanometers (KiCad internal units).
     */
    int64_t ToNanometersSize( double aValue ) const;

    // Conversion constants
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
