/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * Based on the dsn2kicad reference implementation and on OrCAD file format
 * documentation from the OpenOrCadParser project (MIT licensed).
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


#ifndef ORCAD_ORIENT_H_
#define ORCAD_ORIENT_H_

#include <cstdint>

#include <math/vector2d.h>


/** OrCAD rotates about the bounding box; KiCad rotates about the anchor.
 * Offset selectors use 0 for zero, 1 for width, and 2 for height. */
struct ORCAD_ORIENT_ENTRY
{
    int    angle;  ///< KiCad placement angle in degrees (0/90/180/270, CCW)
    char   mirror; ///< 0 = none, 'x' or 'y' = KiCad mirror axis
    int8_t txSel;  ///< X offset selector
    int8_t tySel;  ///< Y offset selector
    int8_t a;
    int8_t b;
    int8_t c;
    int8_t d;
};


/** Indexed by orientation bits: angle, mirror, offset selectors, then matrix coefficients. */
inline constexpr ORCAD_ORIENT_ENTRY ORCAD_ORIENT_TABLE[8] = {
    { 0, 0, 0, 0, 1, 0, 0, 1 },    { 90, 0, 0, 1, 0, 1, -1, 0 },    { 180, 0, 1, 2, -1, 0, 0, -1 },
    { 270, 0, 2, 0, 0, -1, 1, 0 }, { 0, 'y', 1, 0, -1, 0, 0, 1 },   { 90, 'x', 0, 0, 0, 1, 1, 0 },
    { 0, 'x', 0, 2, 1, 0, 0, -1 }, { 270, 'x', 2, 1, 0, -1, -1, 0 }
};


/** Compose the 3-bit orientation code from the rotation bits and mirror bit. */
inline int OrcadOrientOf( int aRotation, bool aMirror )
{
    return ( aRotation & 3 ) | ( aMirror ? 0x4 : 0 );
}


inline int OrcadOrientDim( int aSelector, int aWidth, int aHeight )
{
    return aSelector == 1 ? aWidth : aSelector == 2 ? aHeight : 0;
}


/** Bbox re-anchoring offset for the given orientation and body size (DBU). */
inline VECTOR2I OrcadOrientOffset( int aOrient, int aWidth, int aHeight )
{
    const ORCAD_ORIENT_ENTRY& e = ORCAD_ORIENT_TABLE[aOrient & 7];
    return VECTOR2I( OrcadOrientDim( e.txSel, aWidth, aHeight ), OrcadOrientDim( e.tySel, aWidth, aHeight ) );
}


/** Parts use the instance anchor as the base. Power symbols, ports, and connectors use the box minimum. */
inline VECTOR2I OrcadTransformPoint( int aOrient, int aWidth, int aHeight, int aBaseX, int aBaseY, int aPx, int aPy )
{
    const ORCAD_ORIENT_ENTRY& e = ORCAD_ORIENT_TABLE[aOrient & 7];
    VECTOR2I                  t = OrcadOrientOffset( aOrient, aWidth, aHeight );

    return VECTOR2I( aBaseX + t.x + e.a * aPx + e.b * aPy, aBaseY + t.y + e.c * aPx + e.d * aPy );
}

#endif // ORCAD_ORIENT_H_
