/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 <janvi@veith.net>
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

#include <array>
#include <vector>
#include <string>
#include <cstdint>

/**
 * E-Values derived from a geometric sequence formula by Charles Renard were already
 * accepted and widely used before the ISO recommendation no. 3 has been published.
 * For this historical reason, rounding rules of some values are sometimes irregular.
 * The current list of values is recorded in IEC 60063:2015.
 * Previously it was in IEC publication 63.
 * Although all E-Values could be calculated at runtime, we initialize them in a lookup table
 * what seems the most easy way to consider any inconvenient irregular rules. Same table is
 * also used to lookup non calculable but readable BOM value strings.
 */

// The resistor calculator cannot operate on series larger than E24 due to calculation time

// Values are stored in the 100-999 decade. This is so that all values are integers
// and can be stored precisely. If the values are real values with a fraction part then
// the fractional part is typically imprecisely stores. In the 100 decade the values
// can be stored as precise values in integers. If used in floating point types
// they are as precise as before

// If you want the values in the first decade then simply divide every value in
// the list by the first value in the list.

// E96 is a proper subset of E192. It is every 2nd value. E48 is every 4th value of E192.
// That is, all the series with 3 significant figures are subsets of the same series, E192.

// E24 is not a subset of E48 or E192. All series below E48 have only 2 significant figures
// and are differently from the series with 3 significant figures.

// E12, E6 and E3 are proper subsets of E24. Specifically they are evenly spaced
// values selected from E24. E12 is every 2nd value, E6 every 4th, E3 every 8th.

// E1 is not in the IEC standard.

// The value 0 is not present in any series. It does not fit in any decade.
// It must be special cased in any calcuation or method of selection of
// values.

namespace ESERIES
{

enum
{
    E1,
    E3,
    E6,
    E12,
    E24,
    E48,
    E96,
    E192
};

/* \brief Creates a vector of integers of E series values
 *
 */
class ESERIES_VALUES : public std::vector<uint16_t>
{
public:
    ESERIES_VALUES( int aESeries );

private:
    static const std::vector<uint16_t> s_e24table;
    static const std::vector<uint16_t> s_e192table;
};

/*! \brief Creates a vector of integers of the E1 series values.
    *
    */
class E1_VALUES : public ESERIES_VALUES
{
public:
    E1_VALUES() : ESERIES_VALUES( E1 ) {}
};

/*! \brief Creates a vector of integers of the E3 series values.
    *
    */
class E3_VALUES : public ESERIES_VALUES
{
public:
    E3_VALUES() : ESERIES_VALUES( E3 ) {}
};

/*! \brief Creates a vector of integers of the E6 series values.
    *
    */
class E6_VALUES : public ESERIES_VALUES
{
public:
    E6_VALUES() : ESERIES_VALUES( E6 ) {}
};

/*! \brief Creates a vector of integers of the E12 series values.
    *
    */
class E12_VALUES : public ESERIES_VALUES
{
public:
    E12_VALUES() : ESERIES_VALUES( E12 ) {}
};

/*! \brief Creates a vector of integers of the E24 series values.
    *
    */
class E24_VALUES : public ESERIES_VALUES
{
public:
    E24_VALUES() : ESERIES_VALUES( E24 ) {}
};

/*! \brief Creates a vector of integers of the E48 series values.
    *
    */
class E48_VALUES : public ESERIES_VALUES
{
public:
    E48_VALUES() : ESERIES_VALUES( E48 ) {}
};

/*! \brief Creates a vector of integers of the E96 series values.
    *
    */
class E96_VALUES : public ESERIES_VALUES
{
public:
    E96_VALUES() : ESERIES_VALUES( E96 ) {}
};

/*! \brief Creates a vector of integers of the E192 series values.
    *
    */
class E192_VALUES : public ESERIES_VALUES
{
public:
    E192_VALUES() : ESERIES_VALUES( E192 ) {}
};

/*! \brief Creates a vector of doubles of the values in the requested eseries and decade.
     *
     * The eSeries to select is a specified using the enumeration values above.
     * The decade is specified as an integer exponent to the base 10.
     * To receive vales between 100 and 1000 specify 2 as the exponent as each value will
     * be normalized to betwen 1.0 and 10.0 and then multiplied by 10^2.
     */
class ESERIES_IN_DECADE : public std::vector<double>
{
public:
    ESERIES_IN_DECADE( int eSeries, int decadeExponent );
};
} // namespace ESERIES
