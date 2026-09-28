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

#include <cstdint>
#include <vector>


/**
 * Which combination of the two divider resistors the impedance target applies to.
 */
enum class R_DIVIDER_IMPEDANCE
{
    SERIES,  ///< R1 + R2, the load the divider puts on its source
    PARALLEL ///< R1 || R2, the output impedance of the divider
};


/**
 * The quantity that the ratio error of a divider is measured on.
 *
 * The same divider is off by a different amount on each, and when the target falls between two
 * dividers they may disagree on which one is closer.
 */
enum class R_DIVIDER_TARGET
{
    VOLTAGE,       ///< Vout / Vin = R2 / ( R1 + R2 )
    RESISTOR_RATIO ///< R1 / R2
};


/**
 * How the resistors of a divider are arranged.
 */
enum class R_DIVIDER_TOPOLOGY
{
    TWO_RESISTORS,   ///< R1 on top, R2 to ground
    SPLIT_BOTTOM,    ///< R1 on top, R2 and R3 in series to ground
    SPLIT_TOP,       ///< R1 and R3 in series on top, R2 to ground
    PARALLEL_BOTTOM, ///< R1 on top, R2 and R3 in parallel to ground
    PARALLEL_TOP     ///< R1 and R3 in parallel on top, R2 to ground
};


/**
 * The resistors forming a divider. Values in ohms.
 */
struct R_DIVIDER_SOLUTION
{
    double R1 = 0;
    double R2 = 0;
    double R3 = 0; ///< 0 when the divider only has two resistors, else the smaller one of its leg

    R_DIVIDER_TOPOLOGY Topology = R_DIVIDER_TOPOLOGY::TWO_RESISTORS;

    /// Resistance between the input and the output
    double Top() const
    {
        switch( Topology )
        {
        case R_DIVIDER_TOPOLOGY::SPLIT_TOP:    return R1 + R3;
        case R_DIVIDER_TOPOLOGY::PARALLEL_TOP: return R1 * R3 / ( R1 + R3 );
        default:                               return R1;
        }
    }

    /// Resistance between the output and ground
    double Bottom() const
    {
        switch( Topology )
        {
        case R_DIVIDER_TOPOLOGY::SPLIT_BOTTOM:    return R2 + R3;
        case R_DIVIDER_TOPOLOGY::PARALLEL_BOTTOM: return R2 * R3 / ( R2 + R3 );
        default:                                  return R2;
        }
    }

    bool operator==( const R_DIVIDER_SOLUTION& aOther ) const = default;
};


/**
 * Find pairs of standard resistors that divide \a aVin down to \a aVout.
 *
 * Up to five distinct solutions are returned, each one the best pair under a different trade-off
 * between the error of the divider ratio and the error of the impedance:
 *  -# the closest ratio, whatever the impedance;
 *  -# the closest ratio with the impedance loosely held (30 %, or three steps of the series);
 *  -# the closest ratio with the impedance tightly held (10 %, or two steps of the series);
 *  -# the lowest weighted sum of ratio and impedance error, impedance weighted lightly;
 *  -# the same, with the impedance weighted more heavily.
 *
 * The last two are steered away from the resistor values already used by the first three.
 * A pair that wins more than one of these is only returned once.
 *
 * Only the ratio of the two voltages matters. To request a resistor ratio k = R1 / R2 instead,
 * pass \a aVin = k + 1 and \a aVout = 1, with \a aTarget set to RESISTOR_RATIO so that the
 * dividers are ranked by their error on R1 / R2.
 *
 * @param aVin is the voltage across the whole divider.
 * @param aVout is the voltage across R2. Must be positive and less than \a aVin.
 * @param aImpedance is the target impedance in ohms.
 * @param aImpedanceType selects what \a aImpedance constrains.
 * @param aSeries is one decade of the resistor series to choose from, as ascending three digit
 *                values (100 to 999).
 * @param aTarget selects the quantity that the ratio error is measured on.
 * @return the solutions, best ratio first, or an empty list if the inputs are not usable. Inputs
 *         are not usable either if the ideal R1 or R2 is outside the range of resistors on offer,
 *         1 mOhm to 1 TOhm.
 */
std::vector<R_DIVIDER_SOLUTION> SolveResistorDivider( double aVin, double aVout, double aImpedance,
                                                      R_DIVIDER_IMPEDANCE          aImpedanceType,
                                                      const std::vector<uint16_t>& aSeries,
                                                      R_DIVIDER_TARGET aTarget = R_DIVIDER_TARGET::VOLTAGE );


/**
 * Find sets of three standard resistors that divide \a aVin down to \a aVout, with two of them
 * in series or in parallel forming one leg of the divider. Either leg may be the split one.
 *
 * Every single resistor near its ideal value is paired with every resistor making up between 5 %
 * and 95 % of the other leg that it calls for (of its resistance in series, of its conductance in
 * parallel), and the resistor closest to what is then left of that leg, measured the same way.
 *
 * Only that closest resistor is tried for the last one, not its neighbour on the other side of
 * the ideal value. The neighbour always gives a worse ratio, but now and then it would bring the
 * impedance inside a limit that the closest one misses, so a solution held to a narrow impedance
 * limit may be absent although such a divider exists.
 *
 * Up to four distinct solutions are returned, chosen among all four arrangements together, each
 * one the closest ratio under a different limit on the impedance error:
 *  -# whatever the impedance;
 *  -# the impedance loosely held (30 %, or three steps of the series);
 *  -# the impedance tightly held (10 %, or two steps of the series);
 *  -# the impedance within 1 %.
 *
 * The parameters are those of SolveResistorDivider(). The impedance is that of the two legs in
 * series; an R1 || R2 target is not implemented and gives no solutions.
 *
 * @return the solutions, best ratio first, or an empty list if the inputs are not usable,
 *         including when the ideal R1 or R2 of a two resistor divider at that impedance is outside
 *         1 mOhm to 1 TOhm.
 */
std::vector<R_DIVIDER_SOLUTION> SolveThreeResistorDivider( double aVin, double aVout, double aImpedance,
                                                           R_DIVIDER_IMPEDANCE          aImpedanceType,
                                                           const std::vector<uint16_t>& aSeries,
                                                           R_DIVIDER_TARGET aTarget = R_DIVIDER_TARGET::VOLTAGE );


/**
 * A divider specification along with the key performance metrics
 */
struct R_DIVIDER_RESULT
{
    R_DIVIDER_SOLUTION Solution;

    double Impedance = 0;      ///< R1 + R2 or R1 || R2 of the divider, whichever the target applies to
    double TargetError = 0;    ///< signed relative error of Vout / Vin or of R1 / R2, as requested
    double ImpedanceError = 0; ///< signed relative error of the impedance
};


/**
 * Find the dividers worth offering for a request: the two resistor dividers of
 * SolveResistorDivider(), followed by those of SolveThreeResistorDivider() that have
 * some merit
 *
 * A three resistor divider is only listed if its ratio error or its impedance error is smaller
 * than that of every two resistor divider.
 *
 * The parameters are those of SolveResistorDivider().
 *
 * @return the dividers with their errors, or an empty list if the inputs are not usable.
 */
std::vector<R_DIVIDER_RESULT> FindResistorDividers( double aVin, double aVout, double aImpedance,
                                                    R_DIVIDER_IMPEDANCE          aImpedanceType,
                                                    const std::vector<uint16_t>& aSeries,
                                                    R_DIVIDER_TARGET             aTarget = R_DIVIDER_TARGET::VOLTAGE );

#endif // R_DIVIDER_SOLVER_H
