/*
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this package; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street - Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef TRANSLINE_CALCULATION_UNITS_SCALES_H
#define TRANSLINE_CALCULATION_UNITS_SCALES_H


namespace TRANSLINE_CALCULATIONS
{
constexpr double UNIT_KM = 1e3;                         // km to meter
constexpr double UNIT_M = 1;                            // m to meter
constexpr double UNIT_CM = 1e-2;                        // cm to meter
constexpr double UNIT_MM = 1e-3;                        // mm to meter
constexpr double UNIT_MICRON = 1e-6;                    // um to meter
constexpr double UNIT_INCH = ( 1e-2 * 2.54 );           // inch to meter
constexpr double UNIT_MIL = ( 1e-5 * 2.54 );            // mil (or thou) to meter
constexpr double UNIT_FEET = 0.3048;                    // feet to meter
constexpr double UNIT_OZSQFT = ( 34.40 * UNIT_MICRON ); // 1 oz/ft^2 is 34.30 microns nominal, 30.90 minimum

constexpr double UNIT_GHZ = 1e9;
constexpr double UNIT_MHZ = 1e6;
constexpr double UNIT_KHZ = 1e3;

constexpr double UNIT_DEGREE = M_PI / 180.0; // degree to radian
constexpr double UNIT_RADIAN = 1.0;          // Radian to radian

constexpr double UNIT_OHM = 1.0;  // Ohm to Ohm
constexpr double UNIT_KOHM = 1e3; // KOhm to Ohm

constexpr double UNIT_OHM_PER_METER = 1.0;           // Ohm per meter to Ohm per meter
constexpr double UNIT_OHM_PER_KILOMETER = 1e-3;      // Ohm per kilometer to Ohm per meter
constexpr double UNIT_OHM_PER_FEET = 3.28084;        // Ohm per feet to Ohm per meter
constexpr double UNIT_OHM_PER_1000FEET = 3.28084e-3; // Ohm per 1000feet to Ohm per meter

constexpr double UNIT_KILOVOLT = 1e-3;  // Kilovolt to Volt
constexpr double UNIT_VOLT = 1.0;       // Volt to Volt
constexpr double UNIT_MILLIVOLT = 1e+3; // Millivolt to Volt

constexpr double UNIT_MEGAWATT = 1e-6;  // Kilowatt to Watt
constexpr double UNIT_KILOWATT = 1e-3;  // Kilowatt to Watt
constexpr double UNIT_WATT = 1.0;       // Watt to Watt
constexpr double UNIT_MILLIWATT = 1e+3; // Milliwatt to Watt

constexpr double UNIT_METER_PER_SECOND = 1.0;           // meter per second to meter per second
constexpr double UNIT_KILOMETER_PER_HOUR = ( 1 / 3.6 ); // km/h to m/s
constexpr double UNIT_FEET_PER_SECOND = 0.3048;         // ft/s to m/s
constexpr double UNIT_MILES_PER_HOUR = 1609.34;         // mi/h to m/s

constexpr double UNIT_SECOND = 1.0;    // second to second
constexpr double UNIT_MSECOND = 1e-3;  // millisecond to second
constexpr double UNIT_USECOND = 1e-6;  // microsecond to second
constexpr double UNIT_NSECOND = 1e-9;  // nanosecond to second
constexpr double UNIT_PSECOND = 1e-12; // picosecond to second
}; // namespace TRANSLINE_CALCULATIONS

#endif // TRANSLINE_CALCULATION_UNITS_SCALES_H
