/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <track_width_calculations.h>

#include <cmath>

namespace
{
/**
 * Coefficients of the Brooks & Adam fit dT = K * I^a * W^b * Th^c to the IPC-2152 data
 * (Table 3-1).
 *
 * External (all copper weights) uses a single fit.  The internal fits vary by copper weight,
 * mostly in the constant K, so the coefficients are selected from the nearest nominal copper
 * weight based on the supplied thickness.  Range constants in the table are taken at their
 * midpoint.
 */
struct BA_COEFFICIENTS
{
    double K;
    double a;
    double b;
    double c;
};


constexpr BA_COEFFICIENTS EXTERNAL_COEFFS{ 215.3, 2.0, -1.15, -1.0 };

constexpr BA_COEFFICIENTS INTERNAL_HALF_OZ_COEFFS{ 120.0, 2.0, -1.10, -1.52 };
constexpr BA_COEFFICIENTS INTERNAL_1OZ_COEFFS{ 200.0, 1.9, -1.10, -1.52 };
constexpr BA_COEFFICIENTS INTERNAL_2OZ_COEFFS{ 300.0, 2.0, -1.15, -1.52 };
constexpr BA_COEFFICIENTS INTERNAL_3OZ_COEFFS{ 262.5, 1.9, -1.15, -1.52 };

// Nominal copper weight thicknesses in mils (1 oz/ft^2 is about 1.378 mil).
constexpr double HALF_OZ_MILS = 0.689;
constexpr double ONE_OZ_MILS = 1.378;
constexpr double TWO_OZ_MILS = 2.756;
constexpr double THREE_OZ_MILS = 4.134;


const BA_COEFFICIENTS& coefficients( bool aUseInternalLayer, double aThicknessMils )
{
    if( !aUseInternalLayer )
        return EXTERNAL_COEFFS;

    if( aThicknessMils < ( HALF_OZ_MILS + ONE_OZ_MILS ) / 2.0 )
        return INTERNAL_HALF_OZ_COEFFS;

    if( aThicknessMils < ( ONE_OZ_MILS + TWO_OZ_MILS ) / 2.0 )
        return INTERNAL_1OZ_COEFFS;

    if( aThicknessMils < ( TWO_OZ_MILS + THREE_OZ_MILS ) / 2.0 )
        return INTERNAL_2OZ_COEFFS;

    return INTERNAL_3OZ_COEFFS;
}
} // namespace


double TRACK_WIDTH_CALCULATIONS::CurrentFromWidth( double aWidthMils, double aThicknessMils,
                                                   double aDeltaT_C, bool aUseInternalLayer )
{
    const BA_COEFFICIENTS& k = coefficients( aUseInternalLayer, aThicknessMils );

    // dT = K * I^a * W^b * Th^c  ->  I = ( dT / ( K * W^b * Th^c ) )^(1/a)
    double denom = k.K * std::pow( aWidthMils, k.b ) * std::pow( aThicknessMils, k.c );

    return std::pow( aDeltaT_C / denom, 1.0 / k.a );
}


double TRACK_WIDTH_CALCULATIONS::WidthFromCurrent( double aCurrentA, double aThicknessMils,
                                                   double aDeltaT_C, bool aUseInternalLayer )
{
    const BA_COEFFICIENTS& k = coefficients( aUseInternalLayer, aThicknessMils );

    // dT = K * I^a * W^b * Th^c  ->  W = ( dT / ( K * I^a * Th^c ) )^(1/b)
    double denom = k.K * std::pow( aCurrentA, k.a ) * std::pow( aThicknessMils, k.c );

    return std::pow( aDeltaT_C / denom, 1.0 / k.b );
}
