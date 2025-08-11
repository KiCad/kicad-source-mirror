/*
 * This program source code file
 * is part of KiCad, a free EDA CAD application.
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

#include "eseries.h"
#include <array>
#include <optional>
#include <string>
#include <utility>
#include <vector>

const double epsilon = 1e-12; // machine epsilon for floating-point equality testing

// First value of resistor in Ohm
// It should be first value of the decade, i.e. power of 10
// This value is only pertinent to the resistor calculator.
// It is used to reduce the computational complexity of its calculations.
// There are valid resistor values using E-series numbers below this
// value and above the below LAST_VALUE.
#define RES_EQUIV_CALC_FIRST_VALUE 10

// Last value of resistor in Ohm
// This value is only pertinent to the resistor calculator. See above.
#define RES_EQUIV_CALC_LAST_VALUE 1e6

// Struct representing resistance value together with its composition, e.g. {20.0, "10R + 10R"}
struct RESISTANCE
{
    double              value;
    std::string         name;
    std::vector<double> parts;

    RESISTANCE( double aValue, const std::string& aName, std::vector<double> aParts = {} ) :
            value( aValue ),
            name( aName ),
            parts( std::move( aParts ) )
    {
        if( parts.empty() )
            parts.push_back( aValue );
    }
};


class RES_EQUIV_CALC
/*! \brief Performs calculations on E-series values primarily to find target values
 * as combinations (serial, parallel) of them.
 *
 * RES_EQUIV_CALC class stores and performs calcuations on E-series values. It currently
 * is targeted toward the resistor calculator and hard codes some limitations
 * to optimize its use in the resistor calculator.
 *
 * At this time these limitations are that this class handles only E-series up to
 * E24 and it does not consider resistor values below 10 Ohm or above 1M Ohm.
 */
{
public:
    RES_EQUIV_CALC();

    enum
    {
        S2R,
        S3R,
        S4R,
        NUMBER_OF_LEVELS
    };

    /**
     * Set E-series to be used in calculations.
     * Correct values are from 0 to 4 inclusive,
     * representing series (consecutively) E1, E3, E6, E12, E24.
     * After changing the series, NewCalc must be called before running calculations.
     */
    void SetSeries( uint32_t aSeries );

    /**
     * Initialize next calculation, clear exclusion mask
     * and erase results from previous calculation.
     *
     * @param aTargetValue is the value (in Ohms) to be looked for
     */
    void NewCalc( double aTargetValue );

    /**
     * If any value of the selected E-series not available, it can be entered as an exclude value.
     *
     * @param aValue is the value (in Ohms) to exclude from calculation
     * Values to exclude are set to true in the current exclusion mask and will not be
     * considered during calculations.
     */
    void Exclude( double aValue );

    /**
     * Executes all the calculations.
     * Results are to be retrieved using GetResults (below).
     */
    void Calculate();

    /**
     * Accessor to calculation results.
     * Empty std::optional means that the exact value can be achieved using fewer resistors.
     */
    const std::array<std::optional<RESISTANCE>, NUMBER_OF_LEVELS>& GetResults() { return m_results; }

private:
    /**
     * Add values from aList to m_e_series tables.
     * Covers all decades between FIRST_VALUE and LAST_VALUE.
     */
    std::vector<RESISTANCE> buildSeriesData( const ESERIES::ESERIES_VALUES& aList );

    /**
     * Build 1R buffer, which is selected E-series table with excluded values removed.
     */
    void prepare1RBuffer();

    /**
     * Build 2R buffer, which consists of all possible combinations of two resistors
     * from 1R buffer (serial and parallel), sorted by value.
     */
    void prepare2RBuffer();

    /**
     * Find in 2R buffer two values nearest to the given value (one smaller and one larger).
     * It always returns two valid values, even for input out of range or Nan.
     */
    std::pair<RESISTANCE&, RESISTANCE&> findIn2RBuffer( double aTargetValue );

    /**
     * Calculate the best combination consisting of exactly 2, 3 or 4 resistors.
     */
    RESISTANCE calculate2RSolution();
    RESISTANCE calculate3RSolution();
    RESISTANCE calculate4RSolution();

private:
    std::vector<std::vector<RESISTANCE>> m_e_series;
    std::vector<bool>                    m_exclude_mask;
    std::vector<RESISTANCE>              m_buffer_1R;
    std::vector<RESISTANCE>              m_buffer_2R;

    uint32_t m_series = ESERIES::E6;
    double   m_target = 0;

    std::array<std::optional<RESISTANCE>, NUMBER_OF_LEVELS> m_results;
};