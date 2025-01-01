/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2011-2014 Jean-Pierre Charras
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

/**
 * @file units_scales.h
 */

// Inside calculations, units are always meter, Hz, Ohm and Radian
// These units are not very easy to handle, so we use conversion constants

#ifndef UNITS_SCALES_H
#define UNITS_SCALES_H

#define UNIT_KM 1e3             // km to meter
#define UNIT_M 1                // m to meter
#define UNIT_CM 1e-2            // cm to meter
#define UNIT_MM 1e-3            // mm to meter
#define UNIT_MICRON 1e-6        // um to meter
#define UNIT_INCH (1e-2*2.54)   // inch to meter
#define UNIT_MIL (1e-5*2.54)    // mil (or thou) to meter
#define UNIT_FEET 0.3048        // feet to meter
#define UNIT_OZSQFT (34.40*UNIT_MICRON) // 1 oz/ft^2 is 34.30 microns nominal, 30.90 minimum

#define UNIT_GHZ 1e9
#define UNIT_MHZ 1e6
#define UNIT_KHZ 1e3

#define UNIT_DEGREE (M_PI/180.0)    // degree to radian
#define UNIT_RADIAN 1.0             // Radian to radian

#define UNIT_OHM 1.0                // Ohm to Ohm
#define UNIT_KOHM 1e3               // KOhm to Ohm

#define UNIT_OHM_PER_METER 1.0           // Ohm per meter to Ohm per meter
#define UNIT_OHM_PER_KILOMETER 1e-3      // Ohm per kilometer to Ohm per meter
#define UNIT_OHM_PER_FEET 3.28084        // Ohm per feet to Ohm per meter
#define UNIT_OHM_PER_1000FEET 3.28084e-3 // Ohm per 1000feet to Ohm per meter

#define UNIT_KILOVOLT 1e-3  // Kilovolt to Volt
#define UNIT_VOLT 1.0       // Volt to Volt
#define UNIT_MILLIVOLT 1e+3 // Millivolt to Volt

#define UNIT_MEGAWATT 1e-6  // Kilowatt to Watt
#define UNIT_KILOWATT 1e-3  // Kilowatt to Watt
#define UNIT_WATT 1.0       // Watt to Watt
#define UNIT_MILLIWATT 1e+3 // Milliwatt to Watt

#define UNIT_METER_PER_SECOND 1.0           // meter per second to meter per second
#define UNIT_KILOMETER_PER_HOUR ( 1 / 3.6 ) // km/h to m/s
#define UNIT_FEET_PER_SECOND 0.3048         // ft/s to m/s
#define UNIT_MILES_PER_HOUR 1609.34         // mi/h to m/s

#define UNIT_SECOND 1.0   // second to second
#define UNIT_MSECOND 1e-3 // millisecond to second
#define UNIT_USECOND 1e-6 // microsecond to second
#define UNIT_NSECOND 1e-9 // nanosecond to second
#define UNIT_PSECOND 1e-12 // picosecond to second

#endif  // UNITS_SCALES_H
