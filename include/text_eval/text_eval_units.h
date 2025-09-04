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
#include <magic_enum.hpp>
#include <string>
#include <string_view>
#include <vector>
#include <array>
#include <algorithm>
#include <optional>

namespace text_eval_units {

/**
 * @brief Enumeration of all supported units in the text evaluation system
 *
 * This enum defines all units that can be parsed and converted by the text evaluator.
 * The order matters for parsing - longer unit strings should come first to ensure
 * proper matching (e.g., "ps/mm" before "ps", "thou" before "th").
 */
enum class Unit {
    // Multi-character composite units (longest first for proper parsing)
    PS_PER_MM,      // "ps/mm" - picoseconds per millimeter
    PS_PER_CM,      // "ps/cm" - picoseconds per centimeter
    PS_PER_IN,      // "ps/in" - picoseconds per inch

    // Multi-character simple units
    THOU,           // "thou" - thousandths of an inch (same as mil)
    DEG,            // "deg" - degrees

    // Common single and double character units
    MM,             // "mm" - millimeters
    CM,             // "cm" - centimeters
    INCH,           // "in" - inches
    MIL,            // "mil" - mils (thousandths of an inch)
    UM,             // "um" - micrometers
    PS,             // "ps" - picoseconds
    FS,             // "fs" - femtoseconds

    // Single character units
    INCH_QUOTE,     // "\"" - inches (using quote character)
    DEGREE_SYMBOL,  // "°" - degrees (using degree symbol)

    // Invalid/unknown unit
    INVALID
};

/**
 * @brief Unit registry that provides centralized unit string mapping and conversion
 *
 * This class uses magic_enum to provide compile-time unit string mapping and
 * runtime unit parsing/conversion capabilities. All unit-related operations
 * in the text evaluator should use this registry to ensure consistency.
 */
class KICOMMON_API UnitRegistry {
public:
    /**
     * @brief Unit information structure
     */
    struct UnitInfo {
        Unit unit;
        std::string_view unitString;
        std::string_view description;
        double conversionToMM;  // Conversion factor to millimeters (base unit)
    };

private:
    // Static unit information table ordered by parsing priority (longest first)
    static constexpr std::array<UnitInfo, 15> s_unitTable = {{
        // Multi-character composite units first (longest matches)
        {Unit::PS_PER_MM,     "ps/mm", "Picoseconds per millimeter", 1.0},
        {Unit::PS_PER_CM,     "ps/cm", "Picoseconds per centimeter", 1.0},
        {Unit::PS_PER_IN,     "ps/in", "Picoseconds per inch", 1.0},

        // Multi-character simple units
        {Unit::THOU,          "thou",  "Thousandths of an inch", 25.4 / 1000.0},
        {Unit::DEG,           "deg",   "Degrees", 1.0},

        // Common units
        {Unit::MM,            "mm",    "Millimeters", 1.0},
        {Unit::CM,            "cm",    "Centimeters", 10.0},
        {Unit::INCH,          "in",    "Inches", 25.4},
        {Unit::MIL,           "mil",   "Mils (thousandths of an inch)", 25.4 / 1000.0},
        {Unit::UM,            "um",    "Micrometers", 1.0 / 1000.0},
        {Unit::PS,            "ps",    "Picoseconds", 1.0},
        {Unit::FS,            "fs",    "Femtoseconds", 1.0},

        // Single character units (must be last for proper parsing)
        {Unit::INCH_QUOTE,    "\"",    "Inches (quote notation)", 25.4},
        {Unit::DEGREE_SYMBOL, "°",     "Degrees (symbol)", 1.0},

        // Invalid marker
        {Unit::INVALID,       "",      "Invalid/unknown unit", 1.0}
    }};

public:
    /**
     * @brief Parse a unit string and return the corresponding Unit enum
     * @param unitStr The unit string to parse
     * @return The Unit enum value, or Unit::INVALID if not recognized
     */
    static constexpr Unit parseUnit(std::string_view unitStr) noexcept {
        if (unitStr.empty()) {
            return Unit::INVALID;
        }

        // Search through unit table (ordered by priority)
        for (const auto& info : s_unitTable) {
            if (info.unit != Unit::INVALID && info.unitString == unitStr) {
                return info.unit;
            }
        }

        return Unit::INVALID;
    }

    /**
     * @brief Get the unit string for a given Unit enum
     * @param unit The Unit enum value
     * @return The unit string, or empty string if invalid
     */
    static constexpr std::string_view getUnitString(Unit unit) noexcept {
        for (const auto& info : s_unitTable) {
            if (info.unit == unit) {
                return info.unitString;
            }
        }
        return "";
    }

    /**
     * @brief Get all unit strings in parsing order (longest first)
     * @return Vector of all supported unit strings
     */
    static std::vector<std::string> getAllUnitStrings() {
        std::vector<std::string> units;
        units.reserve(s_unitTable.size() - 1); // Exclude INVALID

        for (const auto& info : s_unitTable) {
            if (info.unit != Unit::INVALID && !info.unitString.empty()) {
                units.emplace_back(info.unitString);
            }
        }

        return units;
    }

    /**
     * @brief Get conversion factor from one unit to another
     * @param fromUnit Source unit
     * @param toUnit Target unit
     * @return Conversion factor, or 1.0 if conversion not supported
     */
    static constexpr double getConversionFactor(Unit fromUnit, Unit toUnit) noexcept {
        if (fromUnit == toUnit) {
            return 1.0;
        }

        // Find conversion factors for both units
        double fromToMM = 1.0;
        double toFromMM = 1.0;

        for (const auto& info : s_unitTable) {
            if (info.unit == fromUnit) {
                fromToMM = info.conversionToMM;
            } else if (info.unit == toUnit) {
                toFromMM = 1.0 / info.conversionToMM;
            }
        }

        return fromToMM * toFromMM;
    }

    /**
     * @brief Convert EDA_UNITS to text evaluator Unit enum
     * @param edaUnits The EDA_UNITS value
     * @return Corresponding Unit enum value
     */
    static constexpr Unit fromEdaUnits(EDA_UNITS edaUnits) noexcept {
        switch (edaUnits) {
            case EDA_UNITS::MM:          return Unit::MM;
            case EDA_UNITS::CM:          return Unit::CM;
            case EDA_UNITS::MILS:        return Unit::MIL;
            case EDA_UNITS::INCH:        return Unit::INCH;
            case EDA_UNITS::DEGREES:     return Unit::DEG;
            case EDA_UNITS::FS:          return Unit::FS;
            case EDA_UNITS::PS:          return Unit::PS;
            case EDA_UNITS::PS_PER_INCH: return Unit::PS_PER_IN;
            case EDA_UNITS::PS_PER_CM:   return Unit::PS_PER_CM;
            case EDA_UNITS::PS_PER_MM:   return Unit::PS_PER_MM;
            case EDA_UNITS::UM:          return Unit::UM;
            default:                     return Unit::MM; // Default fallback
        }
    }

    /**
     * @brief Convert a value with units to target units
     * @param value The value to convert
     * @param fromUnit Source unit
     * @param toUnit Target unit
     * @return Converted value
     */
    static constexpr double convertValue(double value, Unit fromUnit, Unit toUnit) noexcept {
        return value * getConversionFactor(fromUnit, toUnit);
    }

    /**
     * @brief Convert a value with unit string to target EDA_UNITS
     * @param value The value to convert
     * @param unitStr Source unit string
     * @param targetUnits Target EDA_UNITS
     * @return Converted value
     */
    static double convertToEdaUnits(double value, std::string_view unitStr, EDA_UNITS targetUnits) {
        Unit fromUnit = parseUnit(unitStr);
        if (fromUnit == Unit::INVALID) {
            return value; // No conversion for invalid units
        }

        Unit toUnit = fromEdaUnits(targetUnits);
        return convertValue(value, fromUnit, toUnit);
    }

    /**
     * @brief Check if a string is a valid unit
     * @param unitStr The string to check
     * @return True if the string represents a valid unit
     */
    static constexpr bool isValidUnit(std::string_view unitStr) noexcept {
        return parseUnit(unitStr) != Unit::INVALID;
    }

    /**
     * @brief Get unit information for debugging/display purposes
     * @param unit The unit to get information for
     * @return Optional UnitInfo structure, nullopt if unit is invalid
     */
    static std::optional<UnitInfo> getUnitInfo(Unit unit) noexcept {
        for (const auto& info : s_unitTable) {
            if (info.unit == unit) {
                return info;
            }
        }
        return std::nullopt;
    }
};

} // namespace text_eval_units
