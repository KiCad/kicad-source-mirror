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

#ifndef R_DIVIDER_SOLVER_H
#define R_DIVIDER_SOLVER_H

#include <vector>


/// What the impedance target of a divider applies to
enum class R_DIVIDER_IMPEDANCE
{
    SERIES,  ///< R1 + R2, the load the divider puts on its source
    PARALLEL ///< R1 || R2, the output impedance of the divider
};


/// The quantity the error of a divider is measured on
enum class R_DIVIDER_TARGET
{
    VOLTAGE,       ///< Vout / Vin = R2 / ( R1 + R2 )
    RESISTOR_RATIO ///< R1 / R2
};


/// One side of a divider, a single resistor or two in series or in parallel. Values in ohms.
struct R_DIVIDER_LEG
{
    double Value = 0; ///< resistance of the whole leg
    double A = 0;     ///< the resistor, or the larger of the two
    double B = 0;     ///< the smaller of the two, 0 for a single resistor
    bool   Parallel = false;

    bool operator==( const R_DIVIDER_LEG& aOther ) const = default;
};


struct R_DIVIDER
{
    R_DIVIDER_LEG Top;    ///< R1, between the input and the output
    R_DIVIDER_LEG Bottom; ///< R2, between the output and ground

    double Impedance = 0;      ///< R1 + R2 or R1 || R2, whichever the target applies to
    double TargetError = 0;    ///< signed relative error of Vout / Vin or of R1 / R2
    double ImpedanceError = 0; ///< signed relative error of the impedance

    bool operator==( const R_DIVIDER& aOther ) const = default;
};


/**
 * Find dividers with the resistor ratio R1 / R2 = \a aRatio built from \a aResistors.
 *
 * Every divider has a single resistor on one side and, on the other, the single resistor or pair
 * of resistors closest to what the ratio calls for. For each limit on the impedance error (none,
 * 30 %, 10 % and 1 %, widened to three, two and zero steps of the series) the divider with the
 * smallest ratio error is returned, along with the best one of only two resistors if that differs.
 *
 * @param aRatio must be between 1e-6 and 1e6.
 * @param aResistors are the available resistors in ascending order.
 * @return the dividers, those of two resistors first, or nothing if the inputs are not usable or
 *         call for resistors outside the range of \a aResistors.
 */
std::vector<R_DIVIDER> FindResistorDividers( double aRatio, double aImpedance, R_DIVIDER_IMPEDANCE aImpedanceType,
                                             const std::vector<double>& aResistors, R_DIVIDER_TARGET aTarget );

#endif // R_DIVIDER_SOLVER_H
