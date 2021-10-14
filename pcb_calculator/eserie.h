/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 <janvi@veith.net>
 * Copyright (C) 2020-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <array>
#include <vector>
#include <string>

/**
 * E-Values derived from a geometric sequence formula by Charles Renard were already
 * accepted and widely used before the ISO recommendation no. 3 has been published.
 * For this historical reason, rounding rules of some values are sometimes irregular.
 * Although all E-Values could be calculated at runtime, we initialize them in a lookup table
 * what seems the most easy way to consider any inconvenient irregular rules. Same table is
 * also used to lookup non calculable but readable BOM value strings. Supported E-series are:
 */

// List of normalized values between 1 and 10
// The terminal 0.0 value is a end of list value
// Note also due to calculation time the E24 serie is the biggest usable.
#define E24_VALUES  1.0, 1.1, 1.2, 1.3, 1.5, 1.6, 1.8, 2.0, 2.2, 2.4, 2.7, 3.0,\
                    3.3, 3.6, 3.9, 4.3, 4.7, 5.1, 5.6, 6.2, 6.8, 7.5, 8.2, 9.1, 0.0

#define E12_VALUES  1.0, 1.2, 1.5, 1.8, 2.2, 2.7, 3.3, 3.9, 4.7, 5.6, 6.8, 8.2, 0.0

#define E6_VALUES   1.0, 1.5, 2.2, 3.3, 4.7, 6.8, 0.0

#define E3_VALUES   1.0, 2.2, 4.7, 0.0

#define E1_VALUES   1.0, 0.0


// First value of resistor in ohm
#define FIRST_VALUE 10

// last value of resistor in ohm
#define LAST_VALUE 1e6

/**
 * List of handled E series values:
 * Note: series bigger than E24 have no interest because
 *  - probably the user will fing the needed value inside these series
 *  - the calcuation time can be *very high* for series > E24
 */
enum { E1, E3, E6, E12, E24 };

/**
 * This calculator suggests solutions for 2R, 3R and 4R replacement combinations
 */
enum { S2R, S3R, S4R };

// R_DATA handles a resitor: string value, value and allowed to use
struct R_DATA
{
    R_DATA() :
        e_use( true ),
        e_value( 0.0 )
    {}

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

class E_SERIE
{
public:
    E_SERIE();

    /**
     * If any value of the selected E-serie not available, it can be entered as an exclude value.
     *
     * @param aValue is the value to exclude from calculation
     * Values to exclude are set to false in the selected E-serie source lookup table
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
    const std::array<R_DATA,S4R+1>& GetResults() { return m_results; }

private:
    /**
     * Build the list of R_DATA existing for a given serie
     * Series are E1, E6 ..
     * The values are extracted from the E96_VALUES list
     * @return the count of items added in list
     */
    int buildSerieData( int aEserie, double aList[] );

    /**
     * Build all 2R combinations from the selected E-serie values
     *
     * Pre-calculated value combinations are saved in intermediate look up table m_cmb_lut
     * @return is the number of found combinations what also depends from exclude values
    */
    uint32_t combine2();

    /**
     * Search for closest two component solution
     *
     * @param aSize is the number of valid 2R combinations in m_cmb_lut on where to search
     * The 2R result with smallest deviation will be saved in results
    */
    void simple_solution( uint32_t aSize );

    /**
     * Check if there is a better 3 R solution than previous one using only two components.
     *
     * @param aSize gives the number of available combinations to be checked inside m_cmb_lut
     * Therefore m_cmb_lut is combinated with the primary E-serie look up table
     * The 3R result with smallest deviation will be saved in results if better than 2R
     */
    void combine3( uint32_t aSize );

    /**
     * Check if there is a better four component solution.
     *
     * @param aSsize gives the number of 2R combinations to be checked inside m_cmb_lut
     * Occupied calculation time depends from number of available E-serie values
     * with the power of 4 why execution for E12 is conditional with 4R check box
     * for the case the previously found 3R solution is already exact
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
    std::vector<std::vector<R_DATA>> m_luts;

    /* Note: intermediate calculations use m_cmb_lut
     * if the biggest list is En, reserved array size should be 2*En*En of std::vector primary list.
     * 2 component combinations including redundant swappable terms are for the moment
     * ( using values between 10 ohms and 1Mohm )
     * 72 combinations for E1
     * 512 combinations for E3
     * 1922 combinations for E6
     * 7442 combinations for E12
     * 29282 combinations for E24
     */
    std::vector<R_DATA> m_cmb_lut;                      // intermediate 2R combinations

    std::array<R_DATA, S4R+1>   m_results;              // 2R, 3R and 4R results
    uint32_t                    m_series = E6;          // Radio Button State
    uint32_t                    m_enable_4R = false;    // Check Box 4R enable
    double                      m_required_value = 0.0;	// required Resistor
};
