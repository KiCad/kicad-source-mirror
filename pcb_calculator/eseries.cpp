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

#include <cmath>
#include "eseries.h"

namespace ESERIES
{

const std::vector<uint16_t> ESERIES_VALUES::s_e24table = {
    100, 110, 120, 130, 150, 160, 180, 200, 220, 240, 270, 300,
    330, 360, 390, 430, 470, 510, 560, 620, 680, 750, 820, 910
};

const std::vector<uint16_t> ESERIES_VALUES::s_e192table = {
    100, 101, 102, 104, 105, 106, 107, 109, 110, 111, 113, 114, 115, 117, 118, 120, 121, 123,
    124, 126, 127, 129, 130, 132, 133, 135, 137, 138, 140, 142, 143, 145, 147, 149, 150, 152,
    154, 156, 158, 160, 162, 164, 165, 167, 169, 172, 174, 176, 178, 180, 182, 184, 187, 189,
    191, 193, 196, 198, 200, 203, 205, 208, 210, 213, 215, 218, 221, 223, 226, 229, 232, 234,
    237, 240, 243, 246, 249, 252, 255, 258, 261, 264, 267, 271, 274, 277, 280, 284, 287, 291,
    294, 298, 301, 305, 309, 312, 316, 320, 324, 328, 332, 336, 340, 344, 348, 352, 357, 361,
    365, 370, 374, 379, 383, 388, 392, 397, 402, 407, 412, 417, 422, 427, 432, 437, 442, 448,
    453, 459, 464, 470, 475, 481, 487, 493, 499, 505, 511, 517, 523, 530, 536, 542, 549, 556,
    562, 569, 576, 583, 590, 597, 604, 612, 619, 626, 634, 642, 649, 657, 665, 673, 681, 690,
    698, 706, 715, 723, 732, 741, 750, 759, 768, 777, 787, 796, 806, 816, 825, 835, 845, 856,
    866, 876, 887, 898, 909, 920, 931, 942, 953, 965, 976, 988
};


ESERIES_VALUES::ESERIES_VALUES( int aESeries )
{
    const std::vector<uint16_t>* baseSeries = &s_e24table;
    unsigned int                 baseSeriesSkipValue = 24;

    if( ESERIES::E1 == aESeries || ESERIES::E3 == aESeries || ESERIES::E6 == aESeries
        || ESERIES::E12 == aESeries || ESERIES::E24 == aESeries )
    {
        // The below table depends on the values and order of entries
        // in the E1,E3, etc. enum in eseries.h
        const unsigned int skipTableE124[] = { 24, 8, 4, 2, 1 };

        baseSeries = &ESERIES_VALUES::s_e24table;
        baseSeriesSkipValue = skipTableE124[aESeries];
    }
    else if( ESERIES::E48 == aESeries || ESERIES::E96 == aESeries || ESERIES::E192 == aESeries )
    {
        baseSeries = &ESERIES_VALUES::s_e192table;

        // The below calculation depends on the values and order of entries
        // in the E1,E3, etc. enum in eseries.h
        baseSeriesSkipValue = 1 << ( ESERIES::E192 - aESeries );
    }

    unsigned int decadeBaseLen = baseSeries->size();

    reserve( decadeBaseLen / baseSeriesSkipValue );

    for( unsigned int idx = 0; idx < decadeBaseLen; idx += baseSeriesSkipValue )
    {
        emplace_back( ( *baseSeries )[idx] );
    }

    shrink_to_fit();
}


ESERIES_IN_DECADE::ESERIES_IN_DECADE( int aESeries, int aDecadeExponent )
{
    ESERIES::ESERIES_VALUES seriesValues( aESeries );

    uint16_t     decadeBase = seriesValues[0];
    unsigned int decadeBaseLen = seriesValues.size();
    double       decadeMultiplier = std::pow( 10, aDecadeExponent );

    reserve( decadeBaseLen );

    for( const uint16_t seriesValue : seriesValues )
    {
        emplace_back( decadeMultiplier * seriesValue / decadeBase );
    }

    shrink_to_fit();
}

} // namespace ESERIES
