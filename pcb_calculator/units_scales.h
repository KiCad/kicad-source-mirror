/*
 * @file units_scales.h
 */

 // Inside calculations, units are always meter, Hz, Ohm and Radian
// These units are not very easy to handle, so we use conversion constants

#ifndef UNITS_SCALES_H
#define UNITS_SCALES_H

#define UNIT_MM 1e-3            // mm to meter
#define UNIT_CM 1e-2            // cm to meter
#define UNIT_MICRON 1e-6        // µm to meter
#define UNIT_INCH (1e-2*2.54)   // inch to meter
#define UNIT_MIL (1e-5*2.54)    // mil (or thou) to meter

#define UNIT_GHZ 1e9
#define UNIT_MHZ 1e6
#define UNIT_KHZ 1e3

#define UNIT_DEGREE (M_PI/180.0)    // degree to radian
#define UNIT_RADIAN 1.0             // Radian to radian

#define UNIT_OHM 1.0                // Ohm to Ohm
#define UNIT_KOHM 1e3               // KOhm to Ohm

#endif  // UNITS_SCALES_H
