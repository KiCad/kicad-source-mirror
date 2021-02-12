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

extern double DoubleFromString( const wxString& TextValue );

/**
 * If BENCHMARK is defined, any 4R E12 calculations will print its execution time to console
 * My Hasswell Enthusiast reports 225 mSec what are reproducable within plusminus 2 percent
 */

//#define BENCHMARK

/**
 * E-Values derived from a geometric sequence formula by Charles Renard were already
 * accepted and widely used before the ISO recommendation no. 3 has been published.
 * For this historical reason, rounding rules of some values are sometimes irregular.
 * Although all E-Values could be calculated at runtime, we initialize them in a lookup table
 * what seems the most easy way to consider any inconvenient irregular rules. Same table is
 * also used to lookup non calculatable but readable BOM value strings. Supported E-series are:
 */

enum             { E1, E3, E6, E12 };

/**
 * This calculator suggests solutions for 2R, 3R and 4R replacement combinations
 */

enum             { S2R, S3R, S4R };

/**
 * 6 decade E-series values from 10 Ohms to 1M and its associated BOM strings.
 * Series E3,E6,E12 are defined by additional values for cumulative use with previous series
 */

#define E1_VAL   { true, "1K", 1000 },\
                 { true, "10K", 10000 },\
                 { true, "100K", 100000 },\
                 { true, "10R", 10 },\
                 { true, "100R", 100 },\
                 { true, "1M", 1000000 }

#define E3_ADD   { true, "22R", 22 },\
                 { true, "47R", 47 },\
                 { true, "220R", 220 },\
                 { true, "470R", 470 },\
                 { true, "2K2", 2200 },\
                 { true, "4K7", 4700 },\
                 { true, "22K", 22000 },\
                 { true, "47K", 47000 },\
                 { true, "220K", 220000 },\
                 { true, "470K", 470000 }

#define E6_ADD   { true, "15R", 15 },\
                 { true, "33R", 33 },\
                 { true, "68R", 68 },\
                 { true, "150R", 150 },\
                 { true, "330R", 330 },\
                 { true, "680R", 680 },\
                 { true, "1K5", 1500 },\
                 { true, "3K3", 3300 },\
                 { true, "6K8", 6800 },\
                 { true, "15K", 15000 },\
                 { true, "33K", 33000 },\
                 { true, "68K", 68000 },\
                 { true, "150K", 150000 },\
                 { true, "330K", 330000 },\
                 { true, "680K", 680000 }

#define E12_ADD  { true, "12R", 12 },\
                 { true, "18R", 18 },\
                 { true, "27R", 27 },\
                 { true, "39R", 39 },\
                 { true, "56R", 56 },\
                 { true, "82R", 82 },\
                 { true, "120R", 120 },\
                 { true, "180R", 180 },\
                 { true, "270R", 270 },\
                 { true, "390R", 390 },\
                 { true, "560R", 560 },\
                 { true, "820R", 820 },\
                 { true, "1K2", 1200 },\
                 { true, "1K8", 1800 },\
                 { true, "2K7", 2700 },\
                 { true, "3K9", 3900 },\
                 { true, "5K6", 5600 },\
                 { true, "8K2", 8200 },\
                 { true, "12K", 12000 },\
                 { true, "18K", 18000 },\
                 { true, "27K", 27000 },\
                 { true, "39K", 39000 },\
                 { true, "56K", 56000 },\
                 { true, "82K", 82000 },\
                 { true, "120K", 120000 },\
                 { true, "180K", 180000 },\
                 { true, "270K", 270000 },\
                 { true, "390K", 390000 },\
                 { true, "560K", 560000 },\
                 { true, "820K", 820000 }

struct r_data {
                  bool        e_use;
                  std::string e_name;
                  double      e_value;
              };

class eserie
{
public:
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
    void NewCalc( void );

    /**
     * called on calculate button to execute all the 2R, 3R and 4R calculations
     */
    void Calculate( void );

    /**
     * Interface for CheckBox, RadioButton, RequriedResistor and calculated Results
     */
    void SetSeries( uint32_t aSeries ) { m_series = aSeries; }
    void SetRequiredValue( double aValue ) { m_required_value = aValue; }

    std::array<r_data,S4R+1> get_rslt( void ) { return m_results; }

private:
    /**
     * Build all 2R combinations from the selected E-serie values
     *
     * Pre-calculated value combinations are saved in intermediate look up table m_cmb_lut
     * @return is the number of found combinations what also depends from exclude values
    */
    uint32_t combine2( void );

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
    void strip3( void );

    /*
     * Strip redundant braces from four component result
     *
     * Example: (R1+R2)+(R3+R4) become R1+R2+R3+R4
     * and      (R1|R2)|(R2|R3) become R1|R2|R3|R4
     * while    (R1+R2)|(R3+R4) remains untouched
     */
    void strip4( void );

private:
    std::vector<std::vector<r_data>> luts {
                                              { E1_VAL },
                                              { E1_VAL, E3_ADD },
                                              { E1_VAL, E3_ADD, E6_ADD },
                                              { E1_VAL, E3_ADD, E6_ADD, E12_ADD }
                                          };
    /*
     * TODO: Manual array size calculation is dangerous. Unlike legacy ANSI-C Arrays
     * std::array can not drop length param by providing aggregate init list up
     * to C++17. Reserved array size should be 2*E12² of std::vector primary list.
     * Exceeding memory limit 7442 will crash the calculator without any warnings !
     * Compare to previous MAX_COMB macro for legacy ANSI-C array automatic solution
     * #define E12_SIZE sizeof ( e12_lut ) / sizeof ( r_data )
     *  #define MAX_COMB (2 * E12_SIZE * E12_SIZE)
     * 2 component combinations including redundant swappable terms are for the moment
     * 72 combinations for E1
     * 512 combinations for E3
     * 1922 combinations for E6
     * 7442 combinations for E12
     */

#define MAX_CMB 7442			// maximum combinations for E12

    std::array<r_data, MAX_CMB> m_cmb_lut;	            // intermediate 2R combinations
    std::array<r_data, S4R+1>   m_results;	            // 2R, 3R and 4R results
    uint32_t                    m_series = E6;		    // Radio Button State
    uint32_t                    m_enable_4R = false;    // Check Box 4R enable
    double                      m_required_value;	    // required Resistor
};
