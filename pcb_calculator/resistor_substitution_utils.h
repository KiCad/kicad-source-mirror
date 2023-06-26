/*
 * This program source code file
 * is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <string>
#include <cstdint>
#include <vector>
#include <array>
#include "eseries.h"

// First value of resistor in ohm
// This value is only pertinent to the resistor calculator.
// It is used to reduce the computational complexity of its calculations.
// There are valid resistor values using E-series numbers below this
// value and above the below LAST_VALUE.
#define FIRST_VALUE 10

// last value of resistor in ohm
// This value is only pertinent to the resistor calculator. See above.
#define LAST_VALUE 1e6

// R_DATA handles a resistor: string value, value and allowed to use
struct R_DATA
{
    R_DATA() : e_use( true ), e_value( 0.0 ) {}

    R_DATA( const std::string& aName, double aValue )
    {
        e_use = true;
        e_name = aName;
        e_value = aValue;
    }

    bool        e_use;
    std::string e_name;
    double      e_value;
};

class RES_EQUIV_CALC
/*! \brief Performs calculations on E-series values primarily to find target values.
 *
 * E_SERIES class stores and performs calcuations on E-series values. It currently
 * is targeted toward the resistor calculator and hard codes some limitations
 * to optimize its use in the resistor calculator.
 *
 * At this time these limitations are that this class ignores all E-series larger
 * than E24 and it does not consider resistor values below 10 Ohm or above 1M Ohm.
 */
{
public:
    RES_EQUIV_CALC();

    /**
     * This calculator suggests solutions for 2R, 3R and 4R replacement combinations
     */
    enum
    {
        S2R,
        S3R,
        S4R
    };

    /**
     * If any value of the selected E-series not available, it can be entered as an exclude value.
     *
     * @param aValue is the value to exclude from calculation
     * Values to exclude are set to false in the selected E-series source lookup table
     */
    void Exclude( double aValue );

    /**
     *  initialize next calculation and erase results from previous calculation
     */
    void NewCalc();

    /**
     * called on calculate button to execute all the 2R, 3R and 4R calculations
     */
    void Calculate();

    /**
     * Interface for CheckBox, RadioButton, RequriedResistor and calculated Results
     */
    void SetSeries( uint32_t aSeries ) { m_series = aSeries; }
    void SetRequiredValue( double aValue ) { m_required_value = aValue; }

    // Accessor:
    const std::array<R_DATA, S4R + 1>& GetResults() { return m_results; }

private:
    /**
     * Add values from aList to m_tables.  Covers all decades between FIRST_VALUE and LAST_VALUE.
     * @return the count of items added to m_tables.
     */
    int buildSeriesData( const ESERIES::ESERIES_VALUES );

    /**
     * Build all 2R combinations from the selected E-series values
     *
     * Pre-calculated value combinations are saved in intermediate look up table m_combined_table
     * @return is the number of found combinations what also depends from exclude values
    */
    uint32_t combine2();

    /**
     * Search for closest two component solution
     *
     * @param aSize is the number of valid 2R combinations in m_combined_table on where to search
     * The 2R result with smallest deviation will be saved in results
    */
    void simple_solution( uint32_t aSize );

    /**
     * Check if there is a better 3 R solution than previous one using only two components.
     *
     * @param aSize gives the number of available combinations to be checked inside
     *              m_combined_table.  Therefore m_combined_table is combined with the primary
     *              E-series look up table.  The 3R result with smallest deviation will be saved
     *              in results if better than 2R
     */
    void combine3( uint32_t aSize );

    /**
     * Check if there is a better four component solution.
     *
     * @param aSsize gives the number of 2R combinations to be checked inside m_combined_table
     * Occupied calculation time depends from number of available E-series values with the power
     * of 4 why execution for E12 is conditional with 4R check box for the case the previously
     * found 3R solution is already exact
     */
    void combine4( uint32_t aSize );

    /*
     * Strip redundant braces from three component result
     *
     * Example: R1+(R2+R3) become R1+R2+R3
     * and      R1|(R2|R3) become R1|R2|R3
     * while    R1+(R2|R3) or (R1+R2)|R3) remains untouched
     */
    void strip3();

    /*
     * Strip redundant braces from four component result
     *
     * Example: (R1+R2)+(R3+R4) become R1+R2+R3+R4
     * and      (R1|R2)|(R2|R3) become R1|R2|R3|R4
     * while    (R1+R2)|(R3+R4) remains untouched
     */
    void strip4();

private:
    std::vector<std::vector<R_DATA>> m_tables;

    /* Note: intermediate calculations use m_combined_table
     * if the biggest list is En, reserved array size should be 2*En*En of std::vector primary list.
     * 2 component combinations including redundant swappable terms are for the moment
     * ( using values between 10 ohms and 1Mohm )
     * 72 combinations for E1
     * 512 combinations for E3
     * 1922 combinations for E6
     * 7442 combinations for E12
     * 29282 combinations for E24
     */
    std::vector<R_DATA> m_combined_table;               // intermediate 2R combinations

    std::array<R_DATA, S4R + 1> m_results;              // 2R, 3R and 4R results
    uint32_t                    m_series = ESERIES::E6; // Radio Button State
    double                      m_required_value = 0.0; // required Resistor
};
