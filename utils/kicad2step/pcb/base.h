/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Cirilo Bernardo <cirilo.bernardo@gmail.com>
 * Copyright (C) 2021 KiCad Developers, see AUTHORS.txt for contributors.
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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file base.h
 * Provide declarations of items which are basic to all kicad2mcad code.
 */

#ifndef KICADBASE_H
#define KICADBASE_H

#include <core/optional.h>

#include <ostream>

///< Default minimum distance between points to treat them as separate ones (mm)
static constexpr double MIN_DISTANCE = 0.01;

namespace SEXPR
{
    class SEXPR;
}

enum CURVE_TYPE
{
    CURVE_NONE = 0, // invalid curve
    CURVE_LINE,
    CURVE_ARC,
    CURVE_CIRCLE,
    CURVE_BEZIER
};

/*
 * Layers of importance to MCAD export:
 *  - LAYER_TOP: specifies that a footprint is on the top of the PCB
 *  - LAYER_BOTTOM: specifies that a footprint is on the bottom of the PCB
 *  - LAYER_EDGE: specifies that a Curve is associated with the PCB edge
 */
enum LAYERS
{
    LAYER_NONE = 0, // no layer specified (bad object)
    LAYER_TOP,      // top side
    LAYER_BOTTOM,   // bottom side
    LAYER_EDGE      // edge data
};

struct DOUBLET
{
    double x;
    double y;

    DOUBLET() : x( 0.0 ), y( 0.0 ) { return; }
    DOUBLET( double aX, double aY ) : x( aX ), y( aY ) { return; }
};

std::ostream& operator<<( std::ostream& aStream, const DOUBLET& aDoublet );

struct TRIPLET
{
    double x;
    double y;

    union
    {
        double z;
        double angle;
    };

    TRIPLET() : x( 0.0 ), y( 0.0 ), z( 0.0 ) { return; }
    TRIPLET( double aX, double aY, double aZ ) : x( aX ), y( aY ), z( aZ ) { return; }
};

std::ostream& operator<<( std::ostream& aStream, const TRIPLET& aTriplet );

bool Get2DPositionAndRotation( const SEXPR::SEXPR* data, DOUBLET& aPosition, double& aRotation );
bool Get2DCoordinate( const SEXPR::SEXPR* data, DOUBLET& aCoordinate );
bool Get3DCoordinate( const SEXPR::SEXPR* data, TRIPLET& aCoordinate );
bool GetXYZRotation( const SEXPR::SEXPR* data, TRIPLET& aRotation );

/**
 * Get the layer name from a layer element, if the layer is syntactically valid.
 *
 * E.g. (layer "Edge.Cuts") -> "Edge.Cuts"
 *
 * @param aLayerElem the s-expr element to get the name from.
 * @return the layer name if valid, else empty.
 */
OPT<std::string> GetLayerName( const SEXPR::SEXPR& aLayerElem );

#endif  // KICADBASE_H
